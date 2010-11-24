/*
 * (C) Copyright David Brownell 2000-2002
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/usb.h>
#include "hcd.h"
#include <venus.h>
#include "rl5829.c"	//cfyeh+ 2005/10/05
#include <platform.h>	// for get board id

/* PCI-based HCs are common, but plenty of non-PCI HCs are used too */


/*-------------------------------------------------------------------------*/

/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */
int usb_check_gpio20_board(void)
{
	if(platform_info.board_id == C06_pvr_board
			|| platform_info.board_id == C05_pvrbox_board
			|| platform_info.board_id == C03_pvr_board
			|| platform_info.board_id == C04_pvr_board)
		return 1;
	else
		return 0;
}


int usb_check_gpio6_board(void)
{
	if(platform_info.board_id == realtek_avhdd2_demo_board
			|| platform_info.board_id == C03_pvr2_board
			|| platform_info.board_id == C04_pvr2_board)
		return 1;
	else
		return 0;
}
/**
 * usb_hcd_rbus_probe - initialize PCI-based HCDs
 * @dev: USB Host Controller being probed
 * @id: pci hotplug id connecting controller to HCD framework
 * Context: !in_interrupt()
 *
 * Allocates basic PCI resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 * Store this function in the HCD's struct pci_driver as probe().
 */
int usb_hcd_rbus_probe (const struct hc_driver *driver,
			  struct platform_device *pdev)
{
	struct usb_hcd		*hcd;
	int			retval;
	char			buf [10];
	int			bEHCI=0;
	unsigned int 		tmp;
	
	if (usb_disabled())
		return -ENODEV;
	hcd = usb_create_hcd (driver, &pdev->dev, pdev->name);
	if (!hcd) {
		retval = -ENOMEM;
		goto err1;
	}

	// new setting to power up/down first device on hub for gpio 20 2007/03/6
	if(is_venus_cpu())
	{
	if(usb_check_gpio20_board())
	{
		printk("[USB] Setup GPIO 20 !!!\n"); 
		printk("\tfor platform_info.board_id = 0x%x\n", platform_info.board_id);
		outl(inl(VENUS_MIS_GP0DATO) | (0x1 << 20), VENUS_MIS_GP0DATO); // output latch = 1
		outl(inl(VENUS_MIS_PSELH) & ~(0x3 << 8), VENUS_MIS_PSELH); // GPIO 20 function
		outl(inl(VENUS_MIS_GP0DIR) | (0x1 << 20), VENUS_MIS_GP0DIR); // output enable
	}
	else if(usb_check_gpio6_board())
	{
		printk("[USB] Setup GPIO 6 !!!\n"); 
		printk("\tfor platform_info.board_id = 0x%x\n", platform_info.board_id);
		outl(inl(VENUS_MIS_GP0DATO) | (0x1 << 6), VENUS_MIS_GP0DATO); // output latch = 1
		outl(inl(VENUS_MIS_PSELL) & ~(0x3 << 12), VENUS_MIS_PSELL); // GPIO 6 function
		outl(inl(VENUS_MIS_GP0DIR) | (0x1 << 6), VENUS_MIS_GP0DIR); // output enable
	}
	else
		printk("[USB] NOT Setup GPIO yet, platform_info.board_id = 0x%x!!!\n", platform_info.board_id); 
	}

	//cfyeh+ 2005/10/05
	//setting usb only once
	//setup synopsy ip

	//INSNREH03
	outl(0x00000001, VENUS_USB_EHCI_INSNREG03);
	//in/out packet size
	outl(0x01000040, VENUS_USB_EHCI_INSNREG01);
	
	//SET VENUS_USB_HOST_USBIP_INPUT(BIT 27) TO '1', DEFAULT '0'
	tmp = inl(VENUS_USB_HOST_USBIP_INPUT);
	tmp |= 0x1 << 27; 
	tmp |= 0x1 << 30; // bit 30 must be '1'
	//outl(tmp, VENUS_USB_HOST_USBIP_INPUT);
	
	//setup usb phy
	//printk("before USBPHY_Register_Setting()\n");
	USBPHY_Register_Setting();
	//printk("after USBPHY_Register_Setting()\n");

#ifdef USB_MARS_IRQ_CHECK_DATA_READY
	// cfyeh + : set MARS_USB_HOST_VERSION[1] = 1
	if(is_mars_cpu())// for mars
	{
		outl(inl(MARS_USB_HOST_VERSION) & (0x1 << 1), MARS_USB_HOST_VERSION);
	}
	// cfyeh - : set MARS_USB_HOST_VERSION[1] = 1
#endif /* USB_MARS_IRQ_CHECK_DATA_READY */

	//check EHCI or OHCI
	sprintf (buf, "%s", pdev->name);
	if(strstr(buf, "ehci_hcd")){	//EHCI
		bEHCI=1;
		//hcd->rsrc_start = 0;
		hcd->rsrc_start = KSEG1ADDR((u32)VENUS_IO_PORT_BASE + (u32)VENUS_USB_EHCI_USBBASE);
		hcd->rsrc_len = 0x100;
		hcd->regs =
			(void __iomem *)KSEG1ADDR((u32)VENUS_IO_PORT_BASE + (u32)VENUS_USB_EHCI_USBBASE);
	}
	else	//OHCI
	{
		bEHCI=0;
		hcd->rsrc_start = KSEG1ADDR((u32)VENUS_IO_PORT_BASE + (u32)VENUS_USB_OHCI_USBBASE);
		hcd->rsrc_len = 0x100;
		hcd->regs =
			(void __iomem *)KSEG1ADDR((u32)VENUS_IO_PORT_BASE + (u32)VENUS_USB_OHCI_USBBASE);
	}
	
	retval = usb_add_hcd (hcd, VENUS_INT_USB, SA_SHIRQ);
	
	// new setting to power up/down first device on hub for gpio 20 2007/03/6 
	if(is_venus_cpu())
	{
	if(usb_check_gpio20_board())
	{
		outl(inl(VENUS_MIS_GP0DATO) & ~(0x1 << 20), VENUS_MIS_GP0DATO); // output latch = 0
	}
	else if(usb_check_gpio6_board())
	{
		outl(inl(VENUS_MIS_GP0DATO) & ~(0x1 << 6), VENUS_MIS_GP0DATO); // output latch = 0
	}
	}

	if (retval != 0)
		goto err4;

#if 1 // cfyeh : add PPE for to port power down and up about 70ms
	if(strstr(buf, "ehci_hcd")){	//EHCI
		printk("[cfyeh] set PPE = 0\n");
#if 0 // old setting which didn't work
		outl(0x1, VENUS_USB_OHCI_RH_STATUS); // ohci ClearGlobalPower
		outl(0x5, VENUS_USB_EHCI_PORTSC_0); // ehci clear Port Power
		outl(0x10000, VENUS_USB_OHCI_RH_STATUS); // ohci SetGlobalPower
#else // set setting
		outl(0x2001001, VENUS_USB_OHCI_RH_DESC_A); // clear bit 9
		outl(0x8001, VENUS_USB_OHCI_RH_STATUS); // set bit 0
		outl(0x0, VENUS_USB_EHCI_PORTSC_0); // clear bit 12
		mdelay(10);
		outl(0x1000, VENUS_USB_EHCI_PORTSC_0); // set bit 12
#endif
	}
#endif // cfyeh : add PPE for to port power down and up about 70ms

	return retval;
 err4:
	usb_put_hcd (hcd);
 err1:
	dev_err (&pdev->dev, "init %s fail, %d\n", pdev->name, retval);
	return retval;
} 
EXPORT_SYMBOL (usb_hcd_rbus_probe);


/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */

/**
 * usb_hcd_rbus_remove - shutdown processing for PCI-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_pci_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 * Store this function in the HCD's struct pci_driver as remove().
 */
void usb_hcd_rbus_remove (struct usb_hcd *hcd, struct platform_device *pdev)
{
	if (!hcd)
		return;

	usb_remove_hcd (hcd);
	usb_put_hcd (hcd);
}
EXPORT_SYMBOL (usb_hcd_rbus_remove);


#ifdef	CONFIG_PM

/**
 * usb_hcd_pci_suspend - power management suspend of a PCI-based HCD
 * @dev: USB Host Controller being suspended
 * @message: semantics in flux
 *
 * Store this function in the HCD's struct pci_driver as suspend().
 */
int usb_hcd_rbus_suspend (struct usb_hcd *hcd, struct platform_device *pdev, 
				pm_message_t state, u32 level)
{
	int			retval = 0;

	switch (hcd->state) {
	/* entry if root hub wasn't yet suspended ... from sysfs,
	 * without autosuspend, or if USB_SUSPEND isn't configured.
	 */
	case HC_STATE_RUNNING:
		hcd->state = HC_STATE_QUIESCING;
		retval = hcd->driver->suspend (hcd, state);
		if (retval) {
			dev_dbg (hcd->self.controller, 
					"suspend fail, retval %d\n",
					retval);
			break;
		}

#if 1 //cfyeh test
		free_irq(hcd->irq, hcd);
#endif

		hcd->state = HC_STATE_SUSPENDED;
		break;

	/* entry with CONFIG_USB_SUSPEND, or hcds that autosuspend: the
	 * controller and/or root hub will already have been suspended,
	 * but it won't be ready for a PCI resume call.
	 *
	 * FIXME only CONFIG_USB_SUSPEND guarantees hub_suspend() will
	 * have been called, otherwise root hub timers still run ...
	 */
	case HC_STATE_SUSPENDED:
#ifdef USB_FREE_IRQ_AT_SUSPEND_MODE
		if(hcd->irq)
		{
			retval = hcd->driver->suspend (hcd, state);
			free_irq(hcd->irq, hcd);
			hcd->irq = 0;
		}
#endif /* USB_FREE_IRQ_AT_SUSPEND_MODE */
		dev_dbg (hcd->self.controller, "hcd state %d; already suspended\n",
			hcd->state);
		break;
	default:
		dev_dbg (hcd->self.controller, "hcd state %d; not suspended\n",
			hcd->state);
		WARN_ON(1);
		retval = -EINVAL;
		break;
	}

	/* update power_state **ONLY** to make sysfs happier */
	if (retval == 0)
		pdev->dev.power.power_state = state;
	return retval;
}
EXPORT_SYMBOL (usb_hcd_rbus_suspend);

/**
 * usb_hcd_pci_resume - power management resume of a PCI-based HCD
 * @dev: USB Host Controller being resumed
 *
 * Store this function in the HCD's struct pci_driver as resume().
 */
int usb_hcd_rbus_resume (struct usb_hcd *hcd, struct platform_device *pdev, u32 level)
{
	int			retval;

#if 1 //cfyeh test
	unsigned int		tmp;
        //set VENUS_USB_HOST_WRAPPER(bit 6) to '1', default '0'
	tmp = inl(VENUS_USB_HOST_WRAPPER);
	tmp |= 0x1 << 6;
	outl(tmp, VENUS_USB_HOST_WRAPPER);
	mdelay(10);
#endif

	if (hcd->state != HC_STATE_SUSPENDED) {
		dev_dbg (hcd->self.controller, 
				"can't resume, not suspended!\n");
		return 0;
	}

	pdev->dev.power.power_state = PMSG_ON;
	hcd->state = HC_STATE_RESUMING;

#if 1 //cfyeh test
	hcd->saw_irq = 0;
	retval = request_irq (hcd->irq, usb_hcd_irq, SA_SHIRQ,
		hcd->irq_descr, hcd);
	if (retval < 0) {
		dev_err (hcd->self.controller,
				"can't restore IRQ after resume!\n");
		usb_hc_died (hcd);
		return retval;
	}
#endif	

	retval = hcd->driver->resume (hcd);
	if (!HC_IS_RUNNING (hcd->state)) {
		dev_dbg (hcd->self.controller, 
				"resume fail, retval %d\n", retval);
		usb_hc_died (hcd);
	}
	
	return retval;
}
EXPORT_SYMBOL (usb_hcd_rbus_resume);

#endif	/* CONFIG_PM */


