/*
 * Copyright (c) 2000-2004 by David Brownell
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/config.h>

#ifdef CONFIG_USB_DEBUG
	#define DEBUG
#else
	#undef DEBUG
#endif

#include <linux/module.h>
#ifndef CONFIG_REALTEK_VENUS_USB	//cfyeh+ 2005/11/07
#include <linux/pci.h>
#else
#include <linux/device.h>
#endif /* CONFIG_REALTEK_VENUS_USB */	//cfyeh- 2005/11/07
#include <linux/dmapool.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>
#include <linux/usb.h>
#include <linux/moduleparam.h>
#include <linux/dma-mapping.h>

#include "../core/hcd.h"

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <platform.h>	// for get board id

#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING
extern int gUsbEhciHubSuspendResume[4];
static struct usb_hcd *rtd_ehci_hcd = NULL;
static void ehci_hub_thread_init(void);
static void ehci_hub_thread_cleanup(void);

static unsigned long volatile __jiffy_data ehci_port_jiffies[2] = {0,0};

#endif /* USB_MARS_EHCI_CONNECTION_STATE_POLLING */

#ifdef CONFIG_REALTEK_VENUS_USB	//cfyeh+ 2005/11/07
#include <venus.h>
#include <mars.h>
#endif /* CONFIG_REALTEK_VENUS_USB */	//cfyeh- 2005/11/07
//cfyeh+ 2005/10/05

/*-------------------------------------------------------------------------*/

/*
 * EHCI hc_driver implementation ... experimental, incomplete.
 * Based on the final 1.0 register interface specification.
 *
 * USB 2.0 shows up in upcoming www.pcmcia.org technology.
 * First was PCMCIA, like ISA; then CardBus, which is PCI.
 * Next comes "CardBay", using USB 2.0 signals.
 *
 * Contains additional contributions by Brad Hards, Rory Bolt, and others.
 * Special thanks to Intel and VIA for providing host controllers to
 * test this driver on, and Cypress (including In-System Design) for
 * providing early devices for those host controllers to talk to!
 *
 * HISTORY:
 *
 * 2004-05-10 Root hub and PCI suspend/resume support; remote wakeup. (db)
 * 2004-02-24 Replace pci_* with generic dma_* API calls (dsaxena@plexity.net)
 * 2003-12-29 Rewritten high speed iso transfer support (by Michal Sojka,
 *	<sojkam@centrum.cz>, updates by DB).
 *
 * 2002-11-29	Correct handling for hw async_next register.
 * 2002-08-06	Handling for bulk and interrupt transfers is mostly shared;
 *	only scheduling is different, no arbitrary limitations.
 * 2002-07-25	Sanity check PCI reads, mostly for better cardbus support,
 * 	clean up HC run state handshaking.
 * 2002-05-24	Preliminary FS/LS interrupts, using scheduling shortcuts
 * 2002-05-11	Clear TT errors for FS/LS ctrl/bulk.  Fill in some other
 *	missing pieces:  enabling 64bit dma, handoff from BIOS/SMM.
 * 2002-05-07	Some error path cleanups to report better errors; wmb();
 *	use non-CVS version id; better iso bandwidth claim.
 * 2002-04-19	Control/bulk/interrupt submit no longer uses giveback() on
 *	errors in submit path.  Bugfixes to interrupt scheduling/processing.
 * 2002-03-05	Initial high-speed ISO support; reduce ITD memory; shift
 *	more checking to generic hcd framework (db).  Make it work with
 *	Philips EHCI; reduce PCI traffic; shorten IRQ path (Rory Bolt).
 * 2002-01-14	Minor cleanup; version synch.
 * 2002-01-08	Fix roothub handoff of FS/LS to companion controllers.
 * 2002-01-04	Control/Bulk queuing behaves.
 *
 * 2001-12-12	Initial patch version for Linux 2.5.1 kernel.
 * 2001-June	Works with usb-storage and NEC EHCI on 2.4
 */

#define DRIVER_VERSION "10 Dec 2004"
#define DRIVER_AUTHOR "David Brownell"
#define DRIVER_DESC "USB 2.0 'Enhanced' Host Controller (EHCI) Driver"

static const char	hcd_name [] = "ehci_hcd";


#undef EHCI_VERBOSE_DEBUG
#undef EHCI_URB_TRACE

#ifdef DEBUG
#define EHCI_STATS
#endif

/* magic numbers that can affect system performance */
#define	EHCI_TUNE_CERR		3	/* 0-3 qtd retries; 0 == don't stop */
#define	EHCI_TUNE_RL_HS		4	/* nak throttle; see 4.9 */
#define	EHCI_TUNE_RL_TT		0
#define	EHCI_TUNE_MULT_HS	1	/* 1-3 transactions/uframe; 4.10.3 */
#define	EHCI_TUNE_MULT_TT	1
#define	EHCI_TUNE_FLS		2	/* (small) 256 frame schedule */

#define EHCI_IAA_JIFFIES	(HZ/100)	/* arbitrary; ~10 msec */
#define EHCI_IO_JIFFIES		(HZ/10)		/* io watchdog > irq_thresh */
#define EHCI_ASYNC_JIFFIES	(HZ/20)		/* async idle timeout */
#define EHCI_SHRINK_JIFFIES	(HZ/200)	/* async qh unlink delay */

/* Initial IRQ latency:  faster than hw default */
static int log2_irq_thresh = 0;		// 0 to 6
module_param (log2_irq_thresh, int, S_IRUGO);
MODULE_PARM_DESC (log2_irq_thresh, "log2 IRQ latency, 1-64 microframes");

/* initial park setting:  slower than hw default */
static unsigned park = 0;
module_param (park, uint, S_IRUGO);
MODULE_PARM_DESC (park, "park setting; 1-3 back-to-back async packets");

#define	INTR_MASK (STS_IAA | STS_FATAL | STS_PCD | STS_ERR | STS_INT)

/*-------------------------------------------------------------------------*/

#include "ehci.h"
#include "ehci-dbg.c"

/*-------------------------------------------------------------------------*/
//#if CONFIG_REALTEK_VENUS_USB_TEST_MODE
//cfyeh+ : 2006/09/08
//add for sysfs
extern int gUsbGetDescriptor; // cfyeh+ : add for sysfs on usb
#define max_packet_size(mMaxPacketSize) ((mMaxPacketSize) & 0x07ff)
// gUsbGetDescriptor = 1 for the command phase
// gUsbGetDescriptor = 2 for the data phase
// gUsbGetDescriptor = 3 for the status phase
static struct ehci_qtd *ehci_qtd_alloc (struct ehci_hcd *ehci, int flags);
static int qtd_fill (struct ehci_qtd *qtd, dma_addr_t buf, size_t len, int token, int maxpacket);
static void qtd_list_free (struct ehci_hcd *ehci, struct urb *urb, struct list_head	*qtd_list) ;
static u32	backup_token;

static struct list_head *
hub_qh_urb_transaction (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*head,
	int			flags
)
{
	struct ehci_qtd		*qtd = NULL, *qtd_prev = NULL;
	dma_addr_t		buf = 0;
	int			len = 0, maxpacket = 0;
	int			is_input = 0;

	switch(gUsbGetDescriptor)
	{
	case 1:
	printk("Get Descriptor : command phase\n");
		/*
		 * URBs map to sequences of QTDs:  one logical transaction
		 */
		qtd = ehci_qtd_alloc (ehci, flags);
		if (unlikely (!qtd))
			return NULL;
		list_add_tail (&qtd->qtd_list, head);
		qtd->urb = urb;
	
		backup_token = QTD_STS_ACTIVE;
		backup_token |= (EHCI_TUNE_CERR << 10);
		/* for split transactions, SplitXState initialized to zero */
	
		len = urb->transfer_buffer_length;
		is_input = usb_pipein (urb->pipe);
		if (usb_pipecontrol (urb->pipe)) {
			/* SETUP pid */
			qtd_fill (qtd, urb->setup_dma, sizeof (struct usb_ctrlrequest),
				backup_token | (2 /* "setup" */ << 8) | QTD_IOC, 8);
	
			/* ... and always at least one more pid */
			backup_token ^= QTD_TOGGLE;
			qtd_prev = qtd;
			qtd = ehci_qtd_alloc (ehci, flags);
			if (unlikely (!qtd))
				goto cleanup;
			qtd->urb = urb;
			qtd_prev->hw_next = EHCI_LIST_END;
			qtd_prev->hw_alt_next = EHCI_LIST_END;
		} 

		break;
	case 2:
	printk("Get Descriptor : data phase\n");
		/*
		 * URBs map to sequences of QTDs:  one logical transaction
		 */
		qtd = ehci_qtd_alloc (ehci, flags);
		if (unlikely (!qtd))
			return NULL;
		list_add_tail (&qtd->qtd_list, head);
		qtd->urb = urb;

		/*
		 * data transfer stage:  buffer setup
		 */
		buf = urb->transfer_dma;
		backup_token |= (1 /* "in" */ << 8);
	
		maxpacket = max_packet_size(usb_maxpacket(urb->dev, urb->pipe, !is_input));
	
		/*
		 * buffer gets wrapped in one or more qtds;
		 * last one may be "short" (including zero len)
		 * and may serve as a control status ack
		 */
		qtd_fill (qtd, buf, 18, backup_token, maxpacket);
		qtd->hw_next = EHCI_LIST_END;
		qtd->hw_alt_next = EHCI_LIST_END;
	
		break;
	case 3:
	printk("Get Descriptor : status phase\n");
		/*
		 * control requests may need a terminating data "status" ack;
		 * bulk ones may need a terminating short packet (zero length).
		 */
		//gUsbGetDescriptor = 0;
		{
			int	one_more = 0;
	
			if (usb_pipecontrol (urb->pipe)) {
				one_more = 1;
				backup_token ^= 0x0100;	/* "in" <--> "out"  */
				backup_token |= QTD_TOGGLE;	/* force DATA1 */
			} else if (usb_pipebulk (urb->pipe)
					&& (urb->transfer_flags & URB_ZERO_PACKET)
					&& !(urb->transfer_buffer_length % maxpacket)) {
				one_more = 1;
			}
			if (one_more) {
				//qtd_prev = qtd;
				qtd = ehci_qtd_alloc (ehci, flags);
				if (unlikely (!qtd))
					goto cleanup;
				qtd->urb = urb;
				//qtd_prev->hw_next = QTD_NEXT (qtd->qtd_dma);
				list_add_tail (&qtd->qtd_list, head);
	
				/* never any data in such packets */
				qtd_fill (qtd, 0, 0, backup_token | QTD_IOC, 0);
				qtd->hw_next = EHCI_LIST_END;
				qtd->hw_alt_next = EHCI_LIST_END;
			}
		}
		break;
	default:
		break;
	}
	/* by default, enable interrupt on urb completion */
	if (likely (!(urb->transfer_flags & URB_NO_INTERRUPT)))
		qtd->hw_token |= __constant_cpu_to_le32 (QTD_IOC);
	return head;

cleanup:
	qtd_list_free (ehci, urb, head);
	return NULL;
}
//cfyeh- : 2006/09/08
//#endif /* CONFIG_REALTEK_VENUS_USB_TEST_MODE */

/*-------------------------------------------------------------------------*/

/*
 * handshake - spin reading hc until handshake completes or fails
 * @ptr: address of hc register to be read
 * @mask: bits to look at in result of read
 * @done: value of those bits when handshake succeeds
 * @usec: timeout in microseconds
 *
 * Returns negative errno, or zero on success
 *
 * Success happens when the "mask" bits have the specified value (hardware
 * handshake done).  There are two failure modes:  "usec" have passed (major
 * hardware flakeout), or the register reads as all-ones (hardware removed).
 *
 * That last failure should_only happen in cases like physical cardbus eject
 * before driver shutdown. But it also seems to be caused by bugs in cardbus
 * bridge shutdown:  shutting down the bridge before the devices using it.
 */
static int handshake (void __iomem *ptr, u32 mask, u32 done, int usec)
{
	u32	result;

	do {
		result = readl (ptr);
		if (result == ~(u32)0)		/* card removed */
			return -ENODEV;
		result &= mask;
		if (result == done)
			return 0;
		udelay (1);
		usec--;
	} while (usec > 0);
	return -ETIMEDOUT;
}

/* force HC to halt state from unknown (EHCI spec section 2.3) */
static int ehci_halt (struct ehci_hcd *ehci)
{
	u32	temp = readl (&ehci->regs->status);

	if ((temp & STS_HALT) != 0)
		return 0;

	temp = readl (&ehci->regs->command);
	temp &= ~CMD_RUN;
	writel (temp, &ehci->regs->command);
	return handshake (&ehci->regs->status, STS_HALT, STS_HALT, 16 * 125);
}

/* put TDI/ARC silicon into EHCI mode */
static void tdi_reset (struct ehci_hcd *ehci)
{
	u32 __iomem	*reg_ptr;
	u32		tmp;

	reg_ptr = (u32 __iomem *)(((u8 __iomem *)ehci->regs) + 0x68);
	tmp = readl (reg_ptr);
	tmp |= 0x3;
	writel (tmp, reg_ptr);
}

/* reset a non-running (STS_HALT == 1) controller */
static int ehci_reset (struct ehci_hcd *ehci)
{
	int	retval;
	u32	command = readl (&ehci->regs->command);

	command |= CMD_RESET;
	dbg_cmd (ehci, "reset", command);
	writel (command, &ehci->regs->command);
	ehci_to_hcd(ehci)->state = HC_STATE_HALT;
	ehci->next_statechange = jiffies;
	retval = handshake (&ehci->regs->command, CMD_RESET, 0, 250 * 1000);

	//INSNREH03
	outl(0x00000001, VENUS_USB_EHCI_INSNREG03);
	//in/out packet size
	outl(0x01000040, VENUS_USB_EHCI_INSNREG01);

	if (retval)
		return retval;

	if (ehci_is_TDI(ehci))
		tdi_reset (ehci);

	return retval;
}

/* idle the controller (from running) */
static void ehci_quiesce (struct ehci_hcd *ehci)
{
	u32	temp;

#ifdef DEBUG
	if (!HC_IS_RUNNING (ehci_to_hcd(ehci)->state))
		BUG ();
#endif

	/* wait for any schedule enables/disables to take effect */
	temp = readl (&ehci->regs->command) << 10;
	temp &= STS_ASS | STS_PSS;
	if (handshake (&ehci->regs->status, STS_ASS | STS_PSS,
				temp, 16 * 125) != 0) {
		ehci_to_hcd(ehci)->state = HC_STATE_HALT;
		return;
	}

	/* then disable anything that's still active */
	temp = readl (&ehci->regs->command);
	temp &= ~(CMD_ASE | CMD_IAAD | CMD_PSE);
	writel (temp, &ehci->regs->command);

	/* hardware can take 16 microframes to turn off ... */
	if (handshake (&ehci->regs->status, STS_ASS | STS_PSS,
				0, 16 * 125) != 0) {
		ehci_to_hcd(ehci)->state = HC_STATE_HALT;
		return;
	}
}

/*-------------------------------------------------------------------------*/

static void ehci_work(struct ehci_hcd *ehci, struct pt_regs *regs);

#include "ehci-hub.c"
#include "ehci-mem.c"
#include "ehci-q.c"
#include "ehci-sched.c"

/*-------------------------------------------------------------------------*/

#ifdef CONFIG_USB_EHCI_INT_ERROR_HACK_1071
static int ehci_int_error_flag = 0;
#endif // CONFIG_USB_EHCI_INT_ERROR_HACK_1071

static void ehci_watchdog (unsigned long param)
{
	struct ehci_hcd		*ehci = (struct ehci_hcd *) param;
	unsigned long		flags;

	spin_lock_irqsave (&ehci->lock, flags);

	/* lost IAA irqs wedge things badly; seen with a vt8235 */
	if (ehci->reclaim) {
		u32		status = readl (&ehci->regs->status);

		if (status & STS_IAA) {
			ehci_vdbg (ehci, "lost IAA\n");
			COUNT (ehci->stats.lost_iaa);
			writel (STS_IAA, &ehci->regs->status);
			ehci->reclaim_ready = 1;
		}
	}

 	/* stop async processing after it's idled a bit */
	if (test_bit (TIMER_ASYNC_OFF, &ehci->actions))
 		start_unlink_async (ehci, ehci->async);

	/* ehci could run by timer, without IRQs ... */
	ehci_work (ehci, NULL);

#ifdef CONFIG_USB_EHCI_INT_ERROR_HACK_1071
	if(ehci_int_error_flag)
	{
		printk("#######[cfyeh-debug] %s(%d) enable ehci interrupt !!!\n", __func__, __LINE__);
		outl(INTR_MASK, VENUS_USB_EHCI_USBINTR);
		ehci_int_error_flag = 0;
	}
#endif // CONFIG_USB_EHCI_INT_ERROR_HACK_1071

	spin_unlock_irqrestore (&ehci->lock, flags);
}

#ifdef	_CONFIG_PCI	//cfyeh+ 2005/10/05

/* EHCI 0.96 (and later) section 5.1 says how to kick BIOS/SMM/...
 * off the controller (maybe it can boot from highspeed USB disks).
 */
static int bios_handoff (struct ehci_hcd *ehci, int where, u32 cap)
{
	if (cap & (1 << 16)) {
		int msec = 5000;
		struct pci_dev *pdev =
				to_pci_dev(ehci_to_hcd(ehci)->self.controller);

		/* request handoff to OS */
		cap |= 1 << 24;
		pci_write_config_dword(pdev, where, cap);

		/* and wait a while for it to happen */
		do {
			msleep(10);
			msec -= 10;
			pci_read_config_dword(pdev, where, &cap);
		} while ((cap & (1 << 16)) && msec);
		if (cap & (1 << 16)) {
			ehci_err (ehci, "BIOS handoff failed (%d, %04x)\n",
				where, cap);
			// some BIOS versions seem buggy...
			// return 1;
			ehci_warn (ehci, "continuing after BIOS bug...\n");
			return 0;
		} 
		ehci_dbg (ehci, "BIOS handoff succeeded\n");
	}
	return 0;
}

#endif

static int
ehci_reboot (struct notifier_block *self, unsigned long code, void *null)
{
	struct ehci_hcd		*ehci;

	ehci = container_of (self, struct ehci_hcd, reboot_notifier);

	/* make BIOS/etc use companion controller during reboot */
	writel (0, &ehci->regs->configured_flag);
	return 0;
}

static void ehci_port_power (struct ehci_hcd *ehci, int is_on)
{
	unsigned port;

	if (!HCS_PPC (ehci->hcs_params))
		return;

	ehci_dbg (ehci, "...power%s ports...\n", is_on ? "up" : "down");
	for (port = HCS_N_PORTS (ehci->hcs_params); port > 0; )
		(void) ehci_hub_control(ehci_to_hcd(ehci),
				is_on ? SetPortFeature : ClearPortFeature,
				USB_PORT_FEAT_POWER,
				port--, NULL, 0);
	msleep(20);
}


/* called by khubd or root hub init threads */

static int ehci_hc_reset (struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	u32			temp;
#ifdef	_CONFIG_PCI
	unsigned		count = 256/4;
#endif /* _CONFIG_PCI */

	spin_lock_init (&ehci->lock);

	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH (readl (&ehci->caps->hc_capbase));
	dbg_hcs_params (ehci, "reset");
	dbg_hcc_params (ehci, "reset");

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl (&ehci->caps->hcs_params);

#ifdef	_CONFIG_PCI
	if (hcd->self.controller->bus == &pci_bus_type) {
		struct pci_dev	*pdev = to_pci_dev(hcd->self.controller);

		switch (pdev->vendor) {
		case PCI_VENDOR_ID_TDI:
			if (pdev->device == PCI_DEVICE_ID_TDI_EHCI) {
				ehci->is_tdi_rh_tt = 1;
				tdi_reset (ehci);
			}
			break;
		case PCI_VENDOR_ID_AMD:
			/* AMD8111 EHCI doesn't work, according to AMD errata */
			if (pdev->device == 0x7463) {
				ehci_info (ehci, "ignoring AMD8111 (errata)\n");
				return -EIO;
			}
			break;
		}

		/* optional debug port, normally in the first BAR */
		temp = pci_find_capability (pdev, 0x0a);
		if (temp) {
			pci_read_config_dword(pdev, temp, &temp);
			temp >>= 16;
			if ((temp & (3 << 13)) == (1 << 13)) {
				temp &= 0x1fff;
				ehci->debug = hcd->regs + temp;
				temp = readl (&ehci->debug->control);
				ehci_info (ehci, "debug port %d%s\n",
					HCS_DEBUG_PORT(ehci->hcs_params),
					(temp & DBGP_ENABLED)
						? " IN USE"
						: "");
				if (!(temp & DBGP_ENABLED))
					ehci->debug = NULL;
			}
		}

		temp = HCC_EXT_CAPS (readl (&ehci->caps->hcc_params));
	} else
		temp = 0;

	/* EHCI 0.96 and later may have "extended capabilities" */
	while (temp && count--) {
		u32		cap;

		pci_read_config_dword (to_pci_dev(hcd->self.controller),
				temp, &cap);
		ehci_dbg (ehci, "capability %04x at %02x\n", cap, temp);
		switch (cap & 0xff) {
		case 1:			/* BIOS/SMM/... handoff */
			if (bios_handoff (ehci, temp, cap) != 0)
				return -EOPNOTSUPP;
			break;
		case 0:			/* illegal reserved capability */
			ehci_warn (ehci, "illegal capability!\n");
			cap = 0;
			/* FALLTHROUGH */
		default:		/* unknown */
			break;
		}
		temp = (cap >> 8) & 0xff;
	}
	if (!count) {
		ehci_err (ehci, "bogus capabilities ... PCI problems!\n");
		return -EIO;
	}
	if (ehci_is_TDI(ehci))
		ehci_reset (ehci);
#endif
	
	ehci_port_power (ehci, 0);

	/* at least the Genesys GL880S needs fixup here */
	temp = HCS_N_CC(ehci->hcs_params) * HCS_N_PCC(ehci->hcs_params);
	temp &= 0x0f;
	if (temp && HCS_N_PORTS(ehci->hcs_params) > temp) {
		ehci_dbg (ehci, "bogus port configuration: "
			"cc=%d x pcc=%d < ports=%d\n",
			HCS_N_CC(ehci->hcs_params),
			HCS_N_PCC(ehci->hcs_params),
			HCS_N_PORTS(ehci->hcs_params));

#ifdef	_CONFIG_PCI
		if (hcd->self.controller->bus == &pci_bus_type) {
			struct pci_dev	*pdev;

			pdev = to_pci_dev(hcd->self.controller);
			switch (pdev->vendor) {
			case 0x17a0:		/* GENESYS */
				/* GL880S: should be PORTS=2 */
				temp |= (ehci->hcs_params & ~0xf);
				ehci->hcs_params = temp;
				break;
			case PCI_VENDOR_ID_NVIDIA:
				/* NF4: should be PCC=10 */
				break;
			}
		}
#endif
	}

	/* force HC to halt state */
	return ehci_halt (ehci);
}

static int ehci_start (struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	u32			temp;
	struct usb_device	*udev;
	struct usb_bus		*bus;
	int			retval;
	u32			hcc_params;
	u8                      sbrn = 0;
	int			first;

	/* skip some things on restart paths */
	first = (ehci->watchdog.data == 0);
	if (first) {
		init_timer (&ehci->watchdog);
		ehci->watchdog.function = ehci_watchdog;
		ehci->watchdog.data = (unsigned long) ehci;
	}

	/*
	 * hw default: 1K periodic list heads, one per frame.
	 * periodic_size can shrink by USBCMD update if hcc_params allows.
	 */
	ehci->periodic_size = DEFAULT_I_TDPS;
	if (first && (retval = ehci_mem_init (ehci, GFP_KERNEL)) < 0)
		return retval;

	/* controllers may cache some of the periodic schedule ... */
	hcc_params = readl (&ehci->caps->hcc_params);
	if (HCC_ISOC_CACHE (hcc_params)) 	// full frame cache
		ehci->i_thresh = 8;
	else					// N microframes cached
		ehci->i_thresh = 2 + HCC_ISOC_THRES (hcc_params);

	ehci->reclaim = NULL;
	ehci->reclaim_ready = 0;
	ehci->next_uframe = -1;

	/* controller state:  unknown --> reset */

	/* EHCI spec section 4.1 */
	if ((retval = ehci_reset (ehci)) != 0) {
		ehci_mem_cleanup (ehci);
		return retval;
	}
	writel (ehci->periodic_dma, &ehci->regs->frame_list);

#ifdef	_CONFIG_PCI
	if (hcd->self.controller->bus == &pci_bus_type) {
		struct pci_dev		*pdev;
		u16			port_wake;

		pdev = to_pci_dev(hcd->self.controller);

		/* Serial Bus Release Number is at PCI 0x60 offset */
		pci_read_config_byte(pdev, 0x60, &sbrn);

		/* port wake capability, reported by boot firmware */
		pci_read_config_word(pdev, 0x62, &port_wake);
		hcd->can_wakeup = (port_wake & 1) != 0;

		/* help hc dma work well with cachelines */
		pci_set_mwi (pdev);
	}
#endif
	
	/*
	 * dedicate a qh for the async ring head, since we couldn't unlink
	 * a 'real' qh without stopping the async schedule [4.8].  use it
	 * as the 'reclamation list head' too.
	 * its dummy is used in hw_alt_next of many tds, to prevent the qh
	 * from automatically advancing to the next td after short reads.
	 */
	if (first) {
		ehci->async->qh_next.qh = NULL;
		ehci->async->hw_next = QH_NEXT (ehci->async->qh_dma);
		ehci->async->hw_info1 = cpu_to_le32 (QH_HEAD);
		ehci->async->hw_token = cpu_to_le32 (QTD_STS_HALT);
		ehci->async->hw_qtd_next = EHCI_LIST_END;
		ehci->async->qh_state = QH_STATE_LINKED;
		ehci->async->hw_alt_next = QTD_NEXT (ehci->async->dummy->qtd_dma);
	}
	writel ((u32)ehci->async->qh_dma, &ehci->regs->async_next);

	/*
	 * hcc_params controls whether ehci->regs->segment must (!!!)
	 * be used; it constrains QH/ITD/SITD and QTD locations.
	 * pci_pool consistent memory always uses segment zero.
	 * streaming mappings for I/O buffers, like pci_map_single(),
	 * can return segments above 4GB, if the device allows.
	 *
	 * NOTE:  the dma mask is visible through dma_supported(), so
	 * drivers can pass this info along ... like NETIF_F_HIGHDMA,
	 * Scsi_Host.highmem_io, and so forth.  It's readonly to all
	 * host side drivers though.
	 */
	if (HCC_64BIT_ADDR (hcc_params)) {
		writel (0, &ehci->regs->segment);
#if 0
// this is deeply broken on almost all architectures
		if (!pci_set_dma_mask (to_pci_dev(hcd->self.controller), 0xffffffffffffffffULL))
			ehci_info (ehci, "enabled 64bit PCI DMA\n");
#endif
	}

	/* clear interrupt enables, set irq latency */
	if (log2_irq_thresh < 0 || log2_irq_thresh > 6)
		log2_irq_thresh = 0;
	temp = 1 << (16 + log2_irq_thresh);
	if (HCC_CANPARK(hcc_params)) {
		/* HW default park == 3, on hardware that supports it (like
		 * NVidia and ALI silicon), maximizes throughput on the async
		 * schedule by avoiding QH fetches between transfers.
		 *
		 * With fast usb storage devices and NForce2, "park" seems to
		 * make problems:  throughput reduction (!), data errors...
		 */
		if (park) {
			park = min (park, (unsigned) 3);
			temp |= CMD_PARK;
			temp |= park << 8;
		}
		ehci_info (ehci, "park %d\n", park);
	}
	if (HCC_PGM_FRAMELISTLEN (hcc_params)) {
		/* periodic schedule size can be smaller than default */
		temp &= ~(3 << 2);
		temp |= (EHCI_TUNE_FLS << 2);
		switch (EHCI_TUNE_FLS) {
		case 0: ehci->periodic_size = 1024; break;
		case 1: ehci->periodic_size = 512; break;
		case 2: ehci->periodic_size = 256; break;
		default:	BUG ();
		}
	}
	// Philips, Intel, and maybe others need CMD_RUN before the
	// root hub will detect new devices (why?); NEC doesn't
	temp |= CMD_RUN;
	writel (temp, &ehci->regs->command);
	dbg_cmd (ehci, "init", temp);

	/* set async sleep time = 10 us ... ? */

	/* wire up the root hub */
	bus = hcd_to_bus (hcd);
	udev = first ? usb_alloc_dev (NULL, bus, 0) : bus->root_hub;
	if (!udev) {
done2:
		ehci_mem_cleanup (ehci);
		return -ENOMEM;
	}
	udev->speed = USB_SPEED_HIGH;
	udev->state = first ? USB_STATE_ATTACHED : USB_STATE_CONFIGURED;

	/*
	 * Start, enabling full USB 2.0 functionality ... usb 1.1 devices
	 * are explicitly handed to companion controller(s), so no TT is
	 * involved with the root hub.  (Except where one is integrated,
	 * and there's no companion controller unless maybe for USB OTG.)
	 */
	if (first) {
		ehci->reboot_notifier.notifier_call = ehci_reboot;
		register_reboot_notifier (&ehci->reboot_notifier);
	}

	hcd->state = HC_STATE_RUNNING;
	writel (FLAG_CF, &ehci->regs->configured_flag);
	readl (&ehci->regs->command);	/* unblock posted write */

//cfyeh+ check FRINDEX
	temp = inl(VENUS_USB_EHCI_FRINDEX); // polling FRINDEX
	mdelay(1);
	if(temp == inl(VENUS_USB_EHCI_FRINDEX))  // wait until FRINDEX is going
		printk("%s %d: EHCI FRINDEX regs error!!!!\n", __FUNCTION__, __LINE__);
//cfyeh-

	temp = HC_VERSION(readl (&ehci->caps->hc_capbase));
	ehci_info (ehci,
		"USB %x.%x %s, EHCI %x.%02x, driver %s\n",
		((sbrn & 0xf0)>>4), (sbrn & 0x0f),
		first ? "initialized" : "restarted",
		temp >> 8, temp & 0xff, DRIVER_VERSION);

	/*
	 * From here on, khubd concurrently accesses the root
	 * hub; drivers will be talking to enumerated devices.
	 * (On restart paths, khubd already knows about the root
	 * hub and could find work as soon as we wrote FLAG_CF.)
	 *
	 * Before this point the HC was idle/ready.  After, khubd
	 * and device drivers may start it running.
	 */
	if (first && usb_hcd_register_root_hub (udev, hcd) != 0) {
		if (hcd->state == HC_STATE_RUNNING)
			ehci_quiesce (ehci);
		ehci_reset (ehci);
		usb_put_dev (udev); 
		retval = -ENODEV;
		goto done2;
	}

	writel (INTR_MASK, &ehci->regs->intr_enable); /* Turn On Interrupts */

	if (first)
		create_debug_files (ehci);

#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING		
	if(is_mars_cpu())// for mars
	{
		// VENUS_SB2_CHIP_INFO: bit[31,16]
		// 0xa0 => A version
		// 0xb0 => B version
		if((inl(VENUS_SB2_CHIP_INFO) >> 16) == 0xa0)
		{
			rtd_ehci_hcd = hcd;
			ehci_hub_thread_init();
		}
	}
#endif /* USB_MARS_EHCI_CONNECTION_STATE_POLLING */

	return 0;
}

/* always called by thread; normally rmmod */

static void ehci_stop (struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);

	ehci_dbg (ehci, "stop\n");

	/* Turn off port power on all root hub ports. */
	ehci_port_power (ehci, 0);

	/* no more interrupts ... */
	del_timer_sync (&ehci->watchdog);

	spin_lock_irq(&ehci->lock);
	if (HC_IS_RUNNING (hcd->state))
		ehci_quiesce (ehci);

	ehci_reset (ehci);
	writel (0, &ehci->regs->intr_enable);
	spin_unlock_irq(&ehci->lock);

	/* let companion controllers work when we aren't */
	writel (0, &ehci->regs->configured_flag);
	unregister_reboot_notifier (&ehci->reboot_notifier);

	remove_debug_files (ehci);

	/* root hub is shut down separately (first, when possible) */
	spin_lock_irq (&ehci->lock);
	if (ehci->async)
		ehci_work (ehci, NULL);
	spin_unlock_irq (&ehci->lock);
	ehci_mem_cleanup (ehci);

#ifdef	EHCI_STATS
	ehci_dbg (ehci, "irq normal %ld err %ld reclaim %ld (lost %ld)\n",
		ehci->stats.normal, ehci->stats.error, ehci->stats.reclaim,
		ehci->stats.lost_iaa);
	ehci_dbg (ehci, "complete %ld unlink %ld\n",
		ehci->stats.complete, ehci->stats.unlink);
#endif

	dbg_status (ehci, "ehci_stop completed", readl (&ehci->regs->status));

#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING
	if(is_mars_cpu())// for mars
	{
		// VENUS_SB2_CHIP_INFO: bit[31,16]
		// 0xa0 => A version
		// 0xb0 => B version
		if((inl(VENUS_SB2_CHIP_INFO) >> 16) == 0xa0)
			ehci_hub_thread_cleanup();
	}
#endif /* USB_MARS_EHCI_CONNECTION_STATE_POLLING */
}

static int ehci_get_frame (struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	return (readl (&ehci->regs->frame_index) >> 3) % ehci->periodic_size;
}

/*-------------------------------------------------------------------------*/

#if     defined(CONFIG_PM) || defined(CONFIG_REALTEK_VENUS_USB) //cfyeh+ 2005/11/07

/* suspend/resume, section 4.3 */

/* These routines rely on the bus (pci, platform, etc)
 * to handle powerdown and wakeup, and currently also on
 * transceivers that don't need any software attention to set up
 * the right sort of wakeup.  
 */

static int ehci_suspend (struct usb_hcd *hcd, pm_message_t message)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);

	if (time_before (jiffies, ehci->next_statechange))
		msleep (100);

#ifdef	CONFIG_USB_SUSPEND
	(void) usb_suspend_device (hcd->self.root_hub, message);
#else
	usb_lock_device (hcd->self.root_hub);
	(void) ehci_hub_suspend (hcd);
	usb_unlock_device (hcd->self.root_hub);
#endif

	// save (PCI) FLADJ in case of Vaux power loss
	// ... we'd only use it to handle clock skew

	return 0;
}

static int ehci_resume (struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	unsigned		port;
	struct usb_device	*root = hcd->self.root_hub;
	int			retval = -EINVAL;

	// maybe restore (PCI) FLADJ

	if (time_before (jiffies, ehci->next_statechange))
		msleep (100);

	/* If any port is suspended, we know we can/must resume the HC. */
	for (port = HCS_N_PORTS (ehci->hcs_params); port > 0; ) {
		u32	status;
		port--;
		status = readl (&ehci->regs->port_status [port]);
		if (status & PORT_SUSPEND) {
			down (&hcd->self.root_hub->serialize);
			retval = ehci_hub_resume (hcd);
			up (&hcd->self.root_hub->serialize);
			break;
		}
		if (!root->children [port])
			continue;
		dbg_port (ehci, __FUNCTION__, port + 1, status);
		usb_set_device_state (root->children[port],
					USB_STATE_NOTATTACHED);
	}

	/* Else reset, to cope with power loss or flush-to-storage
	 * style "resume" having activated BIOS during reboot.
	 */
	if (port == 0) {
		(void) ehci_halt (ehci);
		(void) ehci_reset (ehci);
		(void) ehci_hc_reset (hcd);

		/* emptying the schedule aborts any urbs */
		spin_lock_irq (&ehci->lock);
		if (ehci->reclaim)
			ehci->reclaim_ready = 1;
		ehci_work (ehci, NULL);
		spin_unlock_irq (&ehci->lock);

		/* restart; khubd will disconnect devices */
		retval = ehci_start (hcd);

		/* here we "know" root ports should always stay powered;
		 * but some controllers may lose all power.
		 */
		ehci_port_power (ehci, 1);
	}

	return retval;
}

#endif

/*-------------------------------------------------------------------------*/

/*
 * ehci_work is called from some interrupts, timers, and so on.
 * it calls driver completion functions, after dropping ehci->lock.
 */
static void ehci_work (struct ehci_hcd *ehci, struct pt_regs *regs)
{
	timer_action_done (ehci, TIMER_IO_WATCHDOG);
	if (ehci->reclaim_ready)
		end_unlink_async (ehci, regs);

	/* another CPU may drop ehci->lock during a schedule scan while
	 * it reports urb completions.  this flag guards against bogus
	 * attempts at re-entrant schedule scanning.
	 */
	if (ehci->scanning)
		return;
	ehci->scanning = 1;
	scan_async (ehci, regs);
	if (ehci->next_uframe != -1)
		scan_periodic (ehci, regs);
	ehci->scanning = 0;

	/* the IO watchdog guards against hardware or driver bugs that
	 * misplace IRQs, and should let us run completely without IRQs.
	 * such lossage has been observed on both VT6202 and VT8235. 
	 */
	if (HC_IS_RUNNING (ehci_to_hcd(ehci)->state) &&
			(ehci->async->qh_next.ptr != NULL ||
			 ehci->periodic_sched != 0))
		timer_action (ehci, TIMER_IO_WATCHDOG);
}

/*-------------------------------------------------------------------------*/

static irqreturn_t ehci_irq (struct usb_hcd *hcd, struct pt_regs *regs)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	u32			status;
	int			bh;

	spin_lock (&ehci->lock);

	status = readl (&ehci->regs->status);

	/* e.g. cardbus physical eject */
	if (status == ~(u32) 0) {
		ehci_dbg (ehci, "device removed\n");
		goto dead;
	}

	status &= INTR_MASK;
	if (!status) {			/* irq sharing? */
		spin_unlock(&ehci->lock);
		return IRQ_NONE;
	}

	/* clear (just) interrupts */
	writel (status, &ehci->regs->status);
	readl (&ehci->regs->command);	/* unblock posted write */
	bh = 0;

#ifdef	EHCI_VERBOSE_DEBUG
	/* unrequested/ignored: Frame List Rollover */
	dbg_status (ehci, "irq", status);
#endif

	/* INT, ERR, and IAA interrupt rates can be throttled */
#ifdef CONFIG_USB_EHCI_INT_ERROR_HACK_1071
	if (unlikely ((status & STS_ERR) != 0))
	{
		ehci_int_error_flag = 1;
		printk("#######[cfyeh-debug] %s(%d) disable ehci interrupt !!!\n", __func__, __LINE__);
		outl(0x0, VENUS_USB_EHCI_USBINTR);
	}
#endif // CONFIG_USB_EHCI_INT_ERROR_HACK_1071

	/* normal [4.15.1.2] or error [4.15.1.1] completion */
	if (likely ((status & (STS_INT|STS_ERR)) != 0)) {
		if (likely ((status & STS_ERR) == 0))
			COUNT (ehci->stats.normal);
		else
			COUNT (ehci->stats.error);
		bh = 1;
	}

	/* complete the unlinking of some qh [4.15.2.3] */
	if (status & STS_IAA) {
		COUNT (ehci->stats.reclaim);
		ehci->reclaim_ready = 1;
		bh = 1;
	}

	/* remote wakeup [4.3.1] */
	if ((status & STS_PCD) && hcd->remote_wakeup) {
		unsigned	i = HCS_N_PORTS (ehci->hcs_params);

		/* resume root hub? */
		status = readl (&ehci->regs->command);
		if (!(status & CMD_RUN))
			writel (status | CMD_RUN, &ehci->regs->command);

		while (i--) {
			status = readl (&ehci->regs->port_status [i]);
			if (status & PORT_OWNER)
				continue;
			if (!(status & PORT_RESUME)
					|| ehci->reset_done [i] != 0)
				continue;

			/* start 20 msec resume signaling from this port,
			 * and make khubd collect PORT_STAT_C_SUSPEND to
			 * stop that signaling.
			 */
			ehci->reset_done [i] = jiffies + msecs_to_jiffies (20);
			mod_timer (&hcd->rh_timer,
					ehci->reset_done [i] + 1);
			ehci_dbg (ehci, "port %d remote wakeup\n", i + 1);
		}
	}

	/* PCI errors [4.15.2.4] */
	if (unlikely ((status & STS_FATAL) != 0)) {
		/* bogus "fatal" IRQs appear on some chips... why?  */
		status = readl (&ehci->regs->status);
		dbg_cmd (ehci, "fatal", readl (&ehci->regs->command));
		dbg_status (ehci, "fatal", status);
		if (status & STS_HALT) {
			ehci_err (ehci, "fatal error\n");
dead:
			ehci_reset (ehci);
			writel (0, &ehci->regs->configured_flag);
			/* generic layer kills/unlinks all urbs, then
			 * uses ehci_stop to clean up the rest
			 */
			bh = 1;
		}
	}

#ifdef USB_MARS_IRQ_CHECK_DATA_READY
	// cfyeh + : check MARS_USB_HOST_VERSION[11] = 0
	if(is_mars_cpu())// for mars
	{
		while(inl(MARS_USB_HOST_VERSION) & (0x1 << 11));
	}
	// cfyeh - : check MARS_USB_HOST_VERSION[11] = 0
#endif /* USB_MARS_IRQ_CHECK_DATA_READY */

	if (bh)
		ehci_work (ehci, regs);
	spin_unlock (&ehci->lock);
	return IRQ_HANDLED;
}

/*-------------------------------------------------------------------------*/

/*
 * non-error returns are a promise to giveback() the urb later
 * we drop ownership so next owner (or urb unlink) can get it
 *
 * urb + dev is in hcd.self.controller.urb_list
 * we're queueing TDs onto software and hardware lists
 *
 * hcd-specific init for hcpriv hasn't been done yet
 *
 * NOTE:  control, bulk, and interrupt share the same code to append TDs
 * to a (possibly active) QH, and the same QH scanning code.
 */
static int ehci_urb_enqueue (
	struct usb_hcd	*hcd,
	struct usb_host_endpoint *ep,
	struct urb	*urb,
	int		mem_flags
) {
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	struct list_head	qtd_list;

	INIT_LIST_HEAD (&qtd_list);

//#if CONFIG_REALTEK_VENUS_USB_TEST_MODE
	//cfyeh+ : 2006/09/08
	//add for sysfs
	if(gUsbGetDescriptor != 0)
	{
		if (!hub_qh_urb_transaction (ehci, urb, &qtd_list, mem_flags))
			return -ENOMEM;
		return submit_async (ehci, ep, urb, &qtd_list, mem_flags);
	}
	//cfyeh- : 2006/09/08
//#endif /* CONFIG_REALTEK_VENUS_USB_TEST_MODE */
	
	switch (usb_pipetype (urb->pipe)) {
	// case PIPE_CONTROL:
	// case PIPE_BULK:
	default:
		if (!qh_urb_transaction (ehci, urb, &qtd_list, mem_flags))
			return -ENOMEM;
		return submit_async (ehci, ep, urb, &qtd_list, mem_flags);

	case PIPE_INTERRUPT:
		if (!qh_urb_transaction (ehci, urb, &qtd_list, mem_flags))
			return -ENOMEM;
		return intr_submit (ehci, ep, urb, &qtd_list, mem_flags);

	case PIPE_ISOCHRONOUS:
		if (urb->dev->speed == USB_SPEED_HIGH)
			return itd_submit (ehci, urb, mem_flags);
		else
			return sitd_submit (ehci, urb, mem_flags);
	}
}

static void unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	/* if we need to use IAA and it's busy, defer */
	if (qh->qh_state == QH_STATE_LINKED
			&& ehci->reclaim
			&& HC_IS_RUNNING (ehci_to_hcd(ehci)->state)) {
		struct ehci_qh		*last;

		for (last = ehci->reclaim;
				last->reclaim;
				last = last->reclaim)
			continue;
		qh->qh_state = QH_STATE_UNLINK_WAIT;
		last->reclaim = qh;

	/* bypass IAA if the hc can't care */
	} else if (!HC_IS_RUNNING (ehci_to_hcd(ehci)->state) && ehci->reclaim)
		end_unlink_async (ehci, NULL);

	/* something else might have unlinked the qh by now */
	if (qh->qh_state == QH_STATE_LINKED)
		start_unlink_async (ehci, qh);
}

/* remove from hardware lists
 * completions normally happen asynchronously
 */

static int ehci_urb_dequeue (struct usb_hcd *hcd, struct urb *urb)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	struct ehci_qh		*qh;
	unsigned long		flags;

	spin_lock_irqsave (&ehci->lock, flags);
	switch (usb_pipetype (urb->pipe)) {
	// case PIPE_CONTROL:
	// case PIPE_BULK:
	default:
		qh = (struct ehci_qh *) urb->hcpriv;
		if (!qh)
			break;
		unlink_async (ehci, qh);
		break;

	case PIPE_INTERRUPT:
		qh = (struct ehci_qh *) urb->hcpriv;
		if (!qh)
			break;
		switch (qh->qh_state) {
		case QH_STATE_LINKED:
			intr_deschedule (ehci, qh);
			/* FALL THROUGH */
		case QH_STATE_IDLE:
			qh_completions (ehci, qh, NULL);
			break;
		default:
			ehci_dbg (ehci, "bogus qh %p state %d\n",
					qh, qh->qh_state);
			goto done;
		}

		/* reschedule QH iff another request is queued */
		if (!list_empty (&qh->qtd_list)
				&& HC_IS_RUNNING (hcd->state)) {
			int status;

			status = qh_schedule (ehci, qh);
			spin_unlock_irqrestore (&ehci->lock, flags);

			if (status != 0) {
				// shouldn't happen often, but ...
				// FIXME kill those tds' urbs
				err ("can't reschedule qh %p, err %d",
					qh, status);
			}
			return status;
		}
		break;

	case PIPE_ISOCHRONOUS:
		// itd or sitd ...

		// wait till next completion, do it then.
		// completion irqs can wait up to 1024 msec,
		break;
	}
done:
	spin_unlock_irqrestore (&ehci->lock, flags);
	return 0;
}

/*-------------------------------------------------------------------------*/

// bulk qh holds the data toggle

static void
ehci_endpoint_disable (struct usb_hcd *hcd, struct usb_host_endpoint *ep)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	unsigned long		flags;
	struct ehci_qh		*qh, *tmp;

	/* ASSERT:  any requests/urbs are being unlinked */
	/* ASSERT:  nobody can be submitting urbs for this any more */

rescan:
	spin_lock_irqsave (&ehci->lock, flags);
	qh = ep->hcpriv;
	if (!qh)
		goto done;

	/* endpoints can be iso streams.  for now, we don't
	 * accelerate iso completions ... so spin a while.
	 */
	if (qh->hw_info1 == 0) {
		ehci_vdbg (ehci, "iso delay\n");
		goto idle_timeout;
	}

	if (!HC_IS_RUNNING (hcd->state))
		qh->qh_state = QH_STATE_IDLE;
	switch (qh->qh_state) {
	case QH_STATE_LINKED:
		for (tmp = ehci->async->qh_next.qh;
				tmp && tmp != qh;
				tmp = tmp->qh_next.qh)
			continue;
		/* periodic qh self-unlinks on empty */
		if (!tmp)
			goto nogood;
		unlink_async (ehci, qh);
		/* FALL THROUGH */
	case QH_STATE_UNLINK:		/* wait for hw to finish? */
idle_timeout:
		spin_unlock_irqrestore (&ehci->lock, flags);
		set_current_state (TASK_UNINTERRUPTIBLE);
		schedule_timeout (1);
		goto rescan;
	case QH_STATE_IDLE:		/* fully unlinked */
		if (list_empty (&qh->qtd_list)) {
			qh_put (qh);
			break;
		}
		/* else FALL THROUGH */
	default:
nogood:
		/* caller was supposed to have unlinked any requests;
		 * that's not our job.  just leak this memory.
		 */
		ehci_err (ehci, "qh %p (#%02x) state %d%s\n",
			qh, ep->desc.bEndpointAddress, qh->qh_state,
			list_empty (&qh->qtd_list) ? "" : "(has tds)");
		break;
	}
	ep->hcpriv = NULL;
done:
	spin_unlock_irqrestore (&ehci->lock, flags);
	return;
}

/*-------------------------------------------------------------------------*/

static const struct hc_driver ehci_driver = {
	.description =		hcd_name,
	.product_desc =		"EHCI Host Controller",
	.hcd_priv_size =	sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ehci_irq,
	.flags =		HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset =		ehci_hc_reset,
	.start =		ehci_start,
#if     defined(CONFIG_PM) || defined(CONFIG_REALTEK_VENUS_USB)	//cfyeh+ 2005/11/07
	.suspend =		ehci_suspend,
	.resume =		ehci_resume,
#endif
	.stop =			ehci_stop,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ehci_urb_enqueue,
	.urb_dequeue =		ehci_urb_dequeue,
	.endpoint_disable =	ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ehci_hub_status_data,
	.hub_control =		ehci_hub_control,
	.hub_suspend =		ehci_hub_suspend,
	.hub_resume =		ehci_hub_resume,
};

/*-------------------------------------------------------------------------*/
#ifdef CONFIG_REALTEK_VENUS_USB	//cfyeh+ 2005/11/07
//cfyeh+ 2005/10/05
static int ehci_hcd_drv_probe(struct device *dev)
{
	return usb_hcd_rbus_probe(&ehci_driver, to_platform_device(dev));
}

static int ehci_hcd_drv_remove(struct device *dev)
{
	struct platform_device	*pdev = to_platform_device(dev);
	struct usb_hcd		*hcd = dev_get_drvdata(dev);
	//struct ehci_hcd		*ehci = hcd_to_ehci (hcd);

	usb_hcd_rbus_remove(hcd, pdev);
	/*
	if (ohci->transceiver) {
		(void) otg_set_host(ohci->transceiver, 0);
		put_device(ohci->transceiver->dev);
	}
	*/
	dev_set_drvdata(dev, NULL);

	return 0;
}

#ifdef	CONFIG_PM
static int ehci_hcd_drv_suspend(struct device *dev, pm_message_t state, u32 level)
{
	struct platform_device	*pdev = to_platform_device(dev);
	struct usb_hcd		*hcd = dev_get_drvdata(dev);
	//struct ehci_hcd		*ehci = hcd_to_ehci (hcd);

	return usb_hcd_rbus_suspend(hcd, pdev, state, level);
}

static int ehci_hcd_drv_resume(struct device *dev, u32 level)
{
	struct platform_device	*pdev = to_platform_device(dev);
	struct usb_hcd		*hcd = dev_get_drvdata(dev);
	//struct ehci_hcd		*ehci = hcd_to_ehci (hcd);

	return usb_hcd_rbus_resume(hcd, pdev, level);
}
#endif

static struct device_driver ehci_hcd_driver = {
	.name =		(char *) hcd_name,
	.bus	=	&platform_bus_type,
	//.id_table =	pci_ids,

	.probe =	ehci_hcd_drv_probe,
	.remove =	ehci_hcd_drv_remove,

#ifdef	CONFIG_PM
	.suspend =	ehci_hcd_drv_suspend,
	.resume =	ehci_hcd_drv_resume,
#endif
};

static struct platform_device *ehci_hcd_devs;

//cfyeh- 2005/10/05
#endif /* CONFIG_REALTEK_VENUS_USB */	//cfyeh- 2005/11/07
/*-------------------------------------------------------------------------*/

/* EHCI 1.0 doesn't require PCI */

#ifdef	_CONFIG_PCI

/* PCI driver selection metadata; PCI hotplugging uses this */
static const struct pci_device_id pci_ids [] = { {
	/* handle any USB 2.0 EHCI controller */
	PCI_DEVICE_CLASS(((PCI_CLASS_SERIAL_USB << 8) | 0x20), ~0),
	.driver_data =	(unsigned long) &ehci_driver,
	},
	{ /* end: all zeroes */ }
};
MODULE_DEVICE_TABLE (pci, pci_ids);

/* pci driver glue; this is a "new style" PCI driver module */
static struct pci_driver ehci_pci_driver = {
	.name =		(char *) hcd_name,
	.id_table =	pci_ids,

	.probe =	usb_hcd_pci_probe,
	.remove =	usb_hcd_pci_remove,

#ifdef	CONFIG_PM
	.suspend =	usb_hcd_pci_suspend,
	.resume =	usb_hcd_pci_resume,
#endif
};

#endif	/* PCI */


#define DRIVER_INFO DRIVER_VERSION " " DRIVER_DESC

MODULE_DESCRIPTION (DRIVER_INFO);
MODULE_AUTHOR (DRIVER_AUTHOR);
MODULE_LICENSE ("GPL");

static int __init init (void) 
{
#ifdef CONFIG_REALTEK_VENUS_USB	//cfyeh+ 2005/11/07
	int ret;	//cfyeh+ 2005/10/05
#endif /* CONFIG_REALTEK_VENUS_USB */	//cfyeh- 2005/11/07
	
	if (usb_disabled())
		return -ENODEV;

	pr_debug ("%s: block sizes: qh %Zd qtd %Zd itd %Zd sitd %Zd\n",
		hcd_name,
		sizeof (struct ehci_qh), sizeof (struct ehci_qtd),
		sizeof (struct ehci_itd), sizeof (struct ehci_sitd));

#ifdef CONFIG_REALTEK_VENUS_USB	//cfyeh+ 2005/11/07
	//cfyeh+ 2005/10/05
	ehci_hcd_devs = platform_device_register_simple((char *)hcd_name,
							      -1, NULL, 0);

	if (IS_ERR(ehci_hcd_devs)) {
		ret = PTR_ERR(ehci_hcd_devs);
		return ret;
	}

	return driver_register (&ehci_hcd_driver);
	//cfyeh- 2005/10/05
#else
	return pci_register_driver (&ehci_pci_driver);
#endif /* CONFIG_REALTEK_VENUS_USB */	//cfyeh- 2005/11/07
}
module_init (init);

static void __exit cleanup (void) 
{	
#ifdef CONFIG_REALTEK_VENUS_USB	//cfyeh+ 2005/11/07
	//cfyeh+ 2005/10/05
	struct platform_device *hcd_dev = ehci_hcd_devs;
	ehci_hcd_devs = NULL;

	driver_unregister (&ehci_hcd_driver);
	platform_device_unregister(hcd_dev);
	//cfyeh- 2005/10/05
#else
	pci_unregister_driver (&ehci_pci_driver);
#endif /* CONFIG_REALTEK_VENUS_USB */	//cfyeh- 2005/11/07
}
module_exit (cleanup);

#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING
#undef USB_MARS_EHCI_CONNECTION_STATE_POLLING_RETRY
static pid_t kehci_hubd_pid = 0;

static int ehci_hub_thread(void *__unused)
{
	struct usb_hcd *hcd = rtd_ehci_hcd;
	struct ehci_hcd *ehci = hcd_to_ehci (hcd);
	struct usb_device *root = hcd->self.root_hub;
	struct usb_device *udev = NULL;
	unsigned port;
	int ret;
#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING_RETRY
	int retry = 0;
#endif /* USB_MARS_EHCI_CONNECTION_STATE_POLLING_RETRY */
	struct usb_device_descriptor *desc;

	if (!hcd) {
		printk("%s [Error] : hcd is NULL\n",__func__);
		return -1;
	}

	if (!root) {
		printk("%s [Error] : root is NULL\n",__func__);
		return -1;
	}

#ifdef USB_512B_ALIGNMENT
	desc = kmalloc(USB_512B_ALIGNMENT_SIZE, GFP_NOIO);
#else
	desc = kmalloc(sizeof(*desc), GFP_NOIO);
#endif /* USB_512B_ALIGNMENT */

	/*
	* This thread doesn't need any user-level access,
	* so get rid of all our resources
	*/

#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING_RETRY
_kehci_hubd_retry :
	retry++;
#endif /* USB_MARS_EHCI_CONNECTION_STATE_POLLING_RETRY */

	daemonize("kehci_hubd");
	allow_signal(SIGKILL);

	/* Send me a signal to get me die (for debugging) */
	do {
		for (port = HCS_N_PORTS (ehci->hcs_params); port > 0; )
		{
			port--;
			
			usb_lock_device (hcd->self.root_hub);
			udev = root->children [port];
			// printk("port_status[%d] = 0x%x\n",port,readl(&ehci->regs->port_status[port]) );
			if (!udev) {
				usb_unlock_device (hcd->self.root_hub);
				continue;
			}
				
			if ((jiffies - ehci_port_jiffies[port]) <= USB_MARS_EHCI_CONNECTION_STATE_POLLING_TIME) {
				// printk("ehci_port_jiffies[%d] = 0x%x\n",port , ehci_port_jiffies[port]);
				// printk("jiffies - ehci_port_jiffies[%d] = 0x%x\n",port , jiffies - ehci_port_jiffies[port]);
				usb_unlock_device (hcd->self.root_hub);
				continue;
			}

			if(gUsbEhciHubSuspendResume[port] == 1) //Suspend
			{
				usb_unlock_device (hcd->self.root_hub);
				continue;
			}

			// printk("port%d : udev->devnum = %d\n",port,udev->devnum);
			ret = usb_get_descriptor(udev,USB_DT_DEVICE,0, desc, 8/*sizeof(*desc)*/);
			/*
			printk("ret = %d\n",ret);
			printk("desc->bLength = 0x%x\n",desc->bLength);
			printk("desc->bDescriptorType = 0x%x\n",ret,desc->bDescriptorType);
			printk("desc->bcdUSB = 0x%x\n",ret,desc->bcdUSB);
			printk("desc->bDeviceClass = 0x%x\n",ret,desc->bDeviceClass);
			*/
			if (ret < 0) {
				switch(port) {
					case 0:
					case 1:
						printk("#######[cfyeh-debug] %s(%d) calling usb_disconnect()!!!\n", __func__, __LINE__);
#if defined CALL_USB_DISCONNECT_BEFORE_SETTING_REGS
						usb_disconnect(&udev);
						kfree(udev);
						root->children [port] = NULL;
#endif /* CALL_USB_DISCONNECT_BEFORE_SETTING_REGS */
						if(port == 0)
							outl(0x4400a081, VENUS_USB_HOST_USBIP_INPUT);
						else // port == 1
							outl(0x44002000, MARS_USB_HOST_USBIPINPUT_2PORT);

						writel (readl(&ehci->regs->port_status[port]) | (1 << 8), &ehci->regs->port_status[port]);
						// udelay(50*1000);
						writel (readl(&ehci->regs->port_status[port]) & (~(1 << 8)), &ehci->regs->port_status[port]);
						// printk("Port %d Disconnect!\n", port);

						if(port == 0)
							outl(0x4000a081, VENUS_USB_HOST_USBIP_INPUT);
						else // port == 1
							outl(0x40002000, MARS_USB_HOST_USBIPINPUT_2PORT);
#if ! defined CALL_USB_DISCONNECT_BEFORE_SETTING_REGS
						usb_disconnect(&udev);
						kfree(udev);
						root->children [port] = NULL;
#endif /* CALL_USB_DISCONNECT_BEFORE_SETTING_REGS */
						break;
					default:
						break;
				}
			}
			usb_unlock_device (hcd->self.root_hub);
		}

		try_to_freeze(PF_FREEZE);
		set_current_state(TASK_INTERRUPTIBLE);
		// 1 sec
		schedule_timeout(1*HZ);
		// printk("0x%x\n",jiffies);
	} while (!signal_pending(current));

	printk("%s() [Error]: will go to die!\n",__func__);

#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING_RETRY
	if (retry <= 3)
		goto _kehci_hubd_retry;
#endif /* USB_MARS_EHCI_CONNECTION_STATE_POLLING_RETRY */

	kfree(desc);

	return 1;
}

static void ehci_hub_thread_init(void)
{
	pid_t pid;

	pid = kernel_thread(ehci_hub_thread, NULL, CLONE_KERNEL);
	if (pid >= 0) 
		kehci_hubd_pid = pid;
}

static void ehci_hub_thread_cleanup(void)
{
	int ret;

	/* Kill the thread */
	ret = kill_proc(kehci_hubd_pid, SIGKILL, 1);
}	
#endif // USB_MARS_EHCI_CONNECTION_STATE_POLLING
