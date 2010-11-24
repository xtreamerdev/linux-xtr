/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000, 2001, 2004 MIPS Technologies, Inc.
 * Copyright (C) 2001 Ralf Baechle
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * Routines for generic manipulation of the interrupts found on the MIPS
 * Malta board.
 * The interrupt controller is located in the South Bridge a PIIX4 device
 * with two internal 82C95 interrupt controllers.
 */
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/random.h>

#include <asm/io.h>
#include <venus.h>
//#include <generic.h>
//#include <msc01_pci.h>

extern asmlinkage void mipsIRQ(void);
extern void mips_sb2_setup();

#define USING_SEARCHING_TABLE

#ifndef USING_SEARCHING_TABLE
// Refer to galileo-boards/ev96100
static inline unsigned int ffz8(unsigned int word)
{
	unsigned long k;
	
	k = 7;
	if (word & 0x0fUL) { k -= 4;  word <<= 4;  }
	if (word & 0x30UL) { k -= 2;  word <<= 2;  }
	if (word & 0x40UL) { k -= 1; }
	
	return k;
}
#endif

int null_perf_irq(struct pt_regs *regs) {
        return 0;
}

int (*perf_irq)(struct pt_regs *regs) = null_perf_irq;

void venus_hw0_irqdispatch(int irq, struct pt_regs *regs)
{
	unsigned int cause;	
#ifdef USING_SEARCHING_TABLE
	int i =0;
	static unsigned short table[]={0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};

#ifdef CONFIG_REALTEK_USE_FAST_INTERRUPT
	if (irq & 0x4) {
		i = 2;
//		printk("irq value: %d \n", irq);
		goto out;
	}
#endif

// This will have problem if some device uses IP7, too.
// Try to fix it.
#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
	if (regs->cp0_status & STATUSF_IP7) {
		/* if we open the oprofile function... */
	        cause = read_c0_cause();
	        if (cause & CAUSEF_IP7) {
	                perf_irq(regs);
	                return;
	        }
	}
#endif		
		
	if(irq&0xf0) {
		i+=4;
		irq=irq>>4;
	}
	i+=table[irq];
#ifdef CONFIG_REALTEK_USE_FAST_INTERRUPT
out:
#endif
	do_IRQ(i, regs);
#else
	do_IRQ(ffz8(irq), regs);
#endif
}

void __init arch_init_irq(void)
{
#ifdef CONFIG_REALTEK_USE_SHADOW_REGISTERS
	unsigned int intctl = 0x20;
#endif

	clear_c0_status(ST0_IM);
	set_except_vector(0, mipsIRQ);
	mips_cpu_irq_init(0);

#ifndef CONFIG_REALTEK_COMPACT
	mips_sb2_setup();
#endif

#ifdef CONFIG_REALTEK_USE_SHADOW_REGISTERS
	__asm__ __volatile__ ("mtc0 %0, $12, 1": : "r"(intctl));
#endif
}
