/*
 * BRIEF MODULE DESCRIPTION
 *	Au1000 interrupt routines.
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *		ppopov@mvista.com or source@mvista.com
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED	  ``AS	IS'' AND   ANY	EXPRESS OR IMPLIED
 *  WARRANTIES,	  INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO	EVENT  SHALL   THE AUTHOR  BE	 LIABLE FOR ANY	  DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED	  TO, PROCUREMENT OF  SUBSTITUTE GOODS	OR SERVICES; LOSS OF
 *  USE, DATA,	OR PROFITS; OR	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN	 CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timex.h>
#include <linux/malloc.h>
#include <linux/random.h>
#include <linux/delay.h>

#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <asm/au1000.h>

#ifdef CONFIG_MIPS_PB1000
#include <asm/pb1000.h>
#endif

#undef DEBUG_IRQ
#ifdef DEBUG_IRQ
/* note: prints function name for you */
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

#define EXT_INTC0_REQ0 2 /* IP 2 */
#define EXT_INTC0_REQ1 3 /* IP 3 */
#define EXT_INTC1_REQ0 4 /* IP 4 */
#define EXT_INTC1_REQ1 5 /* IP 5 */
#define MIPS_TIMER_IP  7 /* IP 7 */

#ifdef CONFIG_REMOTE_DEBUG
extern void breakpoint(void);
#endif

extern asmlinkage void au1000_IRQ(void);
extern void set_debug_traps(void);
extern irq_cpustat_t irq_stat [NR_CPUS];
unsigned int local_bh_count[NR_CPUS];
unsigned int local_irq_count[NR_CPUS];

static void setup_local_irq(unsigned int irq, int type, int int_req);
static unsigned int startup_irq(unsigned int irq);
static void end_irq(unsigned int irq_nr);
static inline void mask_and_ack_level_irq(unsigned int irq_nr);
static inline void mask_and_ack_rise_edge_irq(unsigned int irq_nr);
static inline void mask_and_ack_fall_edge_irq(unsigned int irq_nr);
inline void local_enable_irq(unsigned int irq_nr);
inline void local_disable_irq(unsigned int irq_nr);

unsigned long spurious_interrupts;
extern unsigned int do_IRQ(int irq, struct pt_regs *regs);
extern void __init init_generic_irq(void);

#ifdef CONFIG_PM
extern void counter0_irq(int irq, void *dev_id, struct pt_regs *regs);
#endif


static void setup_local_irq(unsigned int irq_nr, int type, int int_req)
{
	/* Config2[n], Config1[n], Config0[n] */
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		switch (type) {
			case INTC_INT_RISE_EDGE: /* 0:0:1 */
				outl(1<<(irq_nr-32),INTC1_CONFIG2_CLEAR);
				outl(1<<(irq_nr-32), INTC1_CONFIG1_CLEAR);
				outl(1<<(irq_nr-32), INTC1_CONFIG0_SET);
				break;
			case INTC_INT_FALL_EDGE: /* 0:1:0 */
				outl(1<<(irq_nr-32), INTC1_CONFIG2_CLEAR);
				outl(1<<(irq_nr-32), INTC1_CONFIG1_SET);
				outl(1<<(irq_nr-32), INTC1_CONFIG0_CLEAR);
				break;
			case INTC_INT_HIGH_LEVEL: /* 1:0:1 */
				outl(1<<(irq_nr-32), INTC1_CONFIG2_SET);
				outl(1<<(irq_nr-32), INTC1_CONFIG1_CLEAR);
				outl(1<<(irq_nr-32), INTC1_CONFIG0_SET);
				break;
			case INTC_INT_LOW_LEVEL: /* 1:1:0 */
				outl(1<<(irq_nr-32), INTC1_CONFIG2_SET);
				outl(1<<(irq_nr-32), INTC1_CONFIG1_SET);
				outl(1<<(irq_nr-32), INTC1_CONFIG0_CLEAR);
				break;
			case INTC_INT_DISABLED: /* 0:0:0 */
				outl(1<<(irq_nr-32), INTC1_CONFIG0_CLEAR);
				outl(1<<(irq_nr-32), INTC1_CONFIG1_CLEAR);
				outl(1<<(irq_nr-32), INTC1_CONFIG2_CLEAR);
				break;
			default: /* disable the interrupt */
				printk("unexpected int type %d (irq %d)\n", type, irq_nr);
				outl(1<<(irq_nr-32), INTC1_CONFIG0_CLEAR);
				outl(1<<(irq_nr-32), INTC1_CONFIG1_CLEAR);
				outl(1<<(irq_nr-32), INTC1_CONFIG2_CLEAR);
				return;
		}
		if (int_req) /* assign to interrupt request 1 */
			outl(1<<(irq_nr-32), INTC1_ASSIGN_REQ_CLEAR);
		else	     /* assign to interrupt request 0 */
			outl(1<<(irq_nr-32), INTC1_ASSIGN_REQ_SET);
		outl(1<<(irq_nr-32), INTC1_SOURCE_SET);
		outl(1<<(irq_nr-32), INTC1_MASK_CLEAR);
		outl(1<<(irq_nr-32), INTC1_WAKEUP_CLEAR);
	}
	else {
		switch (type) {
			case INTC_INT_RISE_EDGE: /* 0:0:1 */
				outl(1<<irq_nr,INTC0_CONFIG2_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG0_SET);
				break;
			case INTC_INT_FALL_EDGE: /* 0:1:0 */
				outl(1<<irq_nr, INTC0_CONFIG2_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG1_SET);
				outl(1<<irq_nr, INTC0_CONFIG0_CLEAR);
				break;
			case INTC_INT_HIGH_LEVEL: /* 1:0:1 */
				outl(1<<irq_nr, INTC0_CONFIG2_SET);
				outl(1<<irq_nr, INTC0_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG0_SET);
				break;
			case INTC_INT_LOW_LEVEL: /* 1:1:0 */
				outl(1<<irq_nr, INTC0_CONFIG2_SET);
				outl(1<<irq_nr, INTC0_CONFIG1_SET);
				outl(1<<irq_nr, INTC0_CONFIG0_CLEAR);
				break;
			case INTC_INT_DISABLED: /* 0:0:0 */
				outl(1<<irq_nr, INTC0_CONFIG0_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG2_CLEAR);
				break;
			default: /* disable the interrupt */
				printk("unexpected int type %d (irq %d)\n", type, irq_nr);
				outl(1<<irq_nr, INTC0_CONFIG0_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG2_CLEAR);
				return;
		}
		if (int_req) /* assign to interrupt request 1 */
			outl(1<<irq_nr, INTC0_ASSIGN_REQ_CLEAR);
		else	     /* assign to interrupt request 0 */
			outl(1<<irq_nr, INTC0_ASSIGN_REQ_SET);
		outl(1<<irq_nr, INTC0_SOURCE_SET);
		outl(1<<irq_nr, INTC0_MASK_CLEAR);
		outl(1<<irq_nr, INTC0_WAKEUP_CLEAR);
	}
	au_sync();
}


static unsigned int startup_irq(unsigned int irq_nr)
{
	local_enable_irq(irq_nr);
	return 0; 
}


static void shutdown_irq(unsigned int irq_nr)
{
	local_disable_irq(irq_nr);
	return;
}


inline void local_enable_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<(irq_nr-32), INTC1_MASK_SET);
		outl(1<<(irq_nr-32), INTC1_WAKEUP_SET);
	}
	else {
		outl(1<<irq_nr, INTC0_MASK_SET);
		outl(1<<irq_nr, INTC0_WAKEUP_SET);
	}
	au_sync();
}


inline void local_disable_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<(irq_nr-32), INTC1_MASK_CLEAR);
		outl(1<<(irq_nr-32), INTC1_WAKEUP_CLEAR);
	}
	else {
		outl(1<<irq_nr, INTC0_MASK_CLEAR);
		outl(1<<irq_nr, INTC0_WAKEUP_CLEAR);
	}
	au_sync();
}


static inline void mask_and_ack_rise_edge_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<(irq_nr-32), INTC1_R_EDGE_DETECT_CLEAR);
		outl(1<<(irq_nr-32), INTC1_MASK_CLEAR);
	}
	else {
		outl(1<<irq_nr, INTC0_R_EDGE_DETECT_CLEAR);
		outl(1<<irq_nr, INTC0_MASK_CLEAR);
	}
	au_sync();
}


static inline void mask_and_ack_fall_edge_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<(irq_nr-32), INTC1_F_EDGE_DETECT_CLEAR);
		outl(1<<(irq_nr-32), INTC1_MASK_CLEAR);
	}
	else {
		outl(1<<irq_nr, INTC0_F_EDGE_DETECT_CLEAR);
		outl(1<<irq_nr, INTC0_MASK_CLEAR);
	}
	au_sync();
}


static inline void mask_and_ack_level_irq(unsigned int irq_nr)
{

	local_disable_irq(irq_nr);
	au_sync();
	if (irq_nr == AU1000_GPIO_15) {
		writew(0x8000, AU1000_MDR); /* ack int */
		au_sync();
	}
	return;
}


static void end_irq(unsigned int irq_nr)
{
	if (!(irq_desc[irq_nr].status & (IRQ_DISABLED|IRQ_INPROGRESS))) {
		local_enable_irq(irq_nr);
	}
	else {
		printk("warning: end_irq %d did not enable (%x)\n", 
				irq_nr, irq_desc[irq_nr].status);
	}
	if (irq_nr == AU1000_GPIO_15) {
		writew(0x4000, AU1000_MDR); /* enable int */
		au_sync();
	}
}

unsigned long save_local_and_disable(int controller)
{
	int i;
	unsigned long flags, mask;
	save_and_cli(flags);

	if (controller) {
		mask = readl(INTC1_MASK_SET);
		for (i=32; i<64; i++) {
			local_disable_irq(i);
		}
	}
	else {
		mask = readl(INTC0_MASK_SET);
		for (i=0; i<32; i++) {
			local_disable_irq(i);
		}
	}
	restore_flags(flags);
	return mask;
}

void restore_local_and_enable(int controller, unsigned long mask)
{
	int i;
	unsigned long flags, new_mask;
	save_and_cli(flags);

	for (i=0; i<32; i++) {
		if (mask & (1<<i)) {
			if (controller)
				local_enable_irq(i+32);
			else
				local_enable_irq(i);
		}
	}
	if (controller)
		new_mask = readl(INTC1_MASK_SET);
	else
		new_mask = readl(INTC0_MASK_SET);

	restore_flags(flags);
}


static struct hw_interrupt_type rise_edge_irq_type = {
	"Au1000 Rise Edge",
	startup_irq,
	shutdown_irq,
	local_enable_irq,
	local_disable_irq,
	mask_and_ack_rise_edge_irq,
	end_irq,
	NULL
};


static struct hw_interrupt_type fall_edge_irq_type = {
	"Au1000 Fall Edge",
	startup_irq,
	shutdown_irq,
	local_enable_irq,
	local_disable_irq,
	mask_and_ack_fall_edge_irq,
	end_irq,
	NULL
};


static struct hw_interrupt_type level_irq_type = {
	"Au1000 Level",
	startup_irq,
	shutdown_irq,
	local_enable_irq,
	local_disable_irq,
	mask_and_ack_level_irq,
	end_irq,
	NULL
};

#ifdef CONFIG_PM
void startup_match20_interrupt(void)
{
	local_enable_irq(AU1000_PC0_MATCH2_INT);
}
#endif


void __init init_IRQ(void)
{
	int i;
	unsigned long cp0_status;

	cp0_status = read_32bit_cp0_register(CP0_STATUS);
	memset(irq_desc, 0, sizeof(irq_desc));
	set_except_vector(0, au1000_IRQ);

	init_generic_irq();
	
	for (i = 0; i <= NR_IRQS; i++) {
		switch (i) {
			case AU1000_GPIO_15:
				setup_local_irq(i, INTC_INT_LOW_LEVEL, 0);
				irq_desc[i].handler = &level_irq_type;
				break;
			case AU1000_MAC0_DMA_INT:
			case AU1000_MAC1_DMA_INT:
		        case AU1000_DMA_INT_BASE:
		        case AU1000_DMA_INT_BASE+1:
		        case AU1000_DMA_INT_BASE+2:
		        case AU1000_DMA_INT_BASE+3:
		        case AU1000_DMA_INT_BASE+4:
		        case AU1000_DMA_INT_BASE+5:
		        case AU1000_DMA_INT_BASE+6:
		        case AU1000_DMA_INT_BASE+7:
				setup_local_irq(i, INTC_INT_HIGH_LEVEL, 0);
				irq_desc[i].handler = &level_irq_type;
				break;
		        case AU1000_USB_HOST_INT:
				setup_local_irq(i, INTC_INT_LOW_LEVEL, 0);
				irq_desc[i].handler = &level_irq_type;
                                break;
		        case AU1000_USB_DEV_REQ_INT:
		        case AU1000_USB_DEV_SUS_INT:
			        setup_local_irq(i, INTC_INT_RISE_EDGE, 0);
                                irq_desc[i].handler = &rise_edge_irq_type;
                                break;
				 // Careful if you change match 2 request!
				 // The interrupt handler is called directly
				 // from the low level dispatch code.
			 case AU1000_PC0_MATCH2_INT:
				 setup_local_irq(i, INTC_INT_RISE_EDGE, 1);
				 irq_desc[i].handler = &rise_edge_irq_type;
				  break;
			default: /* active high, level interrupt */
				setup_local_irq(i, INTC_INT_HIGH_LEVEL, 0);
				irq_desc[i].handler = &level_irq_type;
				break;
		}
	}

	set_cp0_status(ALLINTS);
#ifdef CONFIG_REMOTE_DEBUG
	/* If local serial I/O used for debug port, enter kgdb at once */
	puts("Waiting for kgdb to connect...");
	set_debug_traps();
	breakpoint(); 
#endif
}


void mips_spurious_interrupt(struct pt_regs *regs)
{
	spurious_interrupts++;
}


/*
 * Interrupts are nested. Even if an interrupt handler is registered
 * as "fast", we might get another interrupt before we return from
 * intcX_reqX_irqdispatch().
 */

void intc0_req0_irqdispatch(struct pt_regs *regs)
{
	int irq = 0, i;
	static unsigned long intc0_req0 = 0;

	intc0_req0 |= inl(INTC0_REQ0_INT);

	if (!intc0_req0) return;

	for (i=0; i<32; i++) {
		if ((intc0_req0 & (1<<i))) {
			intc0_req0 &= ~(1<<i);
			do_IRQ(irq, regs);
			break;
		}
		irq++;
	}
}


void intc0_req1_irqdispatch(struct pt_regs *regs)
{
	int irq = 0, i;
	static unsigned long intc0_req1 = 0;

	intc0_req1 = inl(INTC0_REQ1_INT);

	if (!intc0_req1) return;

	for (i=0; i<32; i++) {
		if ((intc0_req1 & (1<<i))) {
			intc0_req1 &= ~(1<<i);
#ifdef CONFIG_PM
			if (i == AU1000_PC0_MATCH2_INT) {
				mask_and_ack_rise_edge_irq(irq);
				counter0_irq(irq, NULL, regs);
				local_enable_irq(irq);
			}
			else 
#endif
			{
				do_IRQ(irq, regs);
			}
			break;
		}
		irq++;
	}
}


/*
 * Interrupt Controller 1:
 * interrupts 32 - 63
 */
void intc1_req0_irqdispatch(struct pt_regs *regs)
{
	int irq = 0, i;
	static unsigned long intc1_req0 = 0;
	volatile unsigned short levels, mdr;
	unsigned char ide_status;

	intc1_req0 |= inl(INTC1_REQ0_INT);

	if (!intc1_req0) return;

	writew(1, CPLD_AUX0); /* debug led 0 */
	for (i=0; i<32; i++) {
		if ((intc1_req0 & (1<<i))) {
			intc1_req0 &= ~(1<<i);
			writew(2, CPLD_AUX0); /* turn on debug led 1  */
			do_IRQ(irq+32, regs);
			writew(0, CPLD_AUX0); /* turn off debug led 1 */
			break;
		}
		irq++;
	}
	writew(0, CPLD_AUX0);
}


void intc1_req1_irqdispatch(struct pt_regs *regs)
{
	int irq = 0, i;
	static unsigned long intc1_req1 = 0;

	intc1_req1 |= inl(INTC1_REQ1_INT);

	if (!intc1_req1) return;

	for (i=0; i<32; i++) {
		if ((intc1_req1 & (1<<i))) {
			intc1_req1 &= ~(1<<i);
			do_IRQ(irq+32, regs);
			break;
		}
		irq++;
	}
}
