/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
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
 * Setting up the clock on the MIPS boards.
 */

//#include <linux/types.h>
#include <linux/config.h>
//#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
//#include <linux/time.h>
//#include <linux/timex.h>
//#include <linux/mc146818rtc.h>

#include <asm/mipsregs.h>
#include <asm/ptrace.h>
//#include <asm/div64.h>
//#include <asm/cpu.h>
#include <asm/time.h>
#include <asm/io.h>
#include <venus.h>

//#include <generic.h>
#include <prom.h>

unsigned long cpu_khz;

#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
#define ALLINTS (IE_SW0 | IE_SW1 | IE_IRQ0 | IE_IRQ1 | IE_IRQ2 | IE_IRQ3 | IE_IRQ4)
#else
#define ALLINTS (IE_SW0 | IE_SW1 | IE_IRQ0 | IE_IRQ1 | IE_IRQ2 | IE_IRQ3 | IE_IRQ5)
#endif

#if defined(CONFIG_REALTEK_VENUS)
static char display_string[] = "        LINUX ON VENUS       ";
#endif
static unsigned int display_count = 0;
#define MAX_DISPLAY_COUNT (sizeof(display_string) - 8)

//#define MIPS_CPU_TIMER_IRQ (NR_IRQS-1)
//#define MIPS_CPU_TIMER_IRQ 6
#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
#define MIPS_CPU_TIMER_IRQ VENUS_INT_EXTERNAL_TIMER
#else
#define MIPS_CPU_TIMER_IRQ VENUS_INT_TIMER
#endif


static unsigned int timer_tick_count=0;

void mips_timer_interrupt(struct pt_regs *regs)
{
//#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
//outl(0x100, VENUS_MIS_ISR);
//#endif
	if ((timer_tick_count++ % HZ) == 0) {
//		mips_display_message(&display_string[display_count++]);
		if (display_count == MAX_DISPLAY_COUNT)
		        display_count = 0;

	}

	ll_timer_interrupt(MIPS_CPU_TIMER_IRQ, regs);
//	ll_timer_interrupt(6, regs);
}

/*
 * Estimate CPU frequency.  Sets mips_counter_frequency as a side-effect
 */
#ifndef CONFIG_REALTEK_DETERMINE_SYSTEM_CPU_CLOCK_FREQUENCY
static unsigned int __init estimate_cpu_frequency(void)
{
	unsigned int count;
	unsigned int sec1, sec2;
	int cause;
//	unsigned int flags;

//	local_irq_save(flags);

	/* Start counter exactly on falling edge of update flag */
//	while (CMOS_READ(RTC_REG_A) & RTC_UIP);
//	while (!(CMOS_READ(RTC_REG_A) & RTC_UIP));
	cause = read_c0_cause();
	write_c0_cause(cause & ~0x08000000);
	sec1 = inl(VENUS_MIS_RTCSEC);
	while((sec2 = inl(VENUS_MIS_RTCSEC)) == sec1) ;

	/* Start r4k counter. */
	write_c0_count(0);

	while(inl(VENUS_MIS_RTCSEC) == sec2) ;

	/* Read counter exactly on falling edge of update flag */
//	while (CMOS_READ(RTC_REG_A) & RTC_UIP);
//	while (!(CMOS_READ(RTC_REG_A) & RTC_UIP));

	count = read_c0_count();
	write_c0_cause(cause);
	/* restore interrupts */
//	local_irq_restore(flags);
	// Every 2 CPU ticks Counter register of MIPS will add 1. One unit in RTC second register represents half second.
	count *= 4;
	count += 5000;
	count -= count % 10000;
//	if ((prid != (PRID_COMP_MIPS | PRID_IMP_20KC)) &&
//	    (prid != (PRID_COMP_MIPS | PRID_IMP_25KF)))
//		count *= 2;

//	count += 5000;    /* round */
//	count -= count%10000;

	return count;
}
#endif

//unsigned long __init mips_rtc_get_time(void)
unsigned long mips_rtc_get_time(void)
{
	unsigned int day, hour, min, sec;
	
	sec = inl(VENUS_MIS_RTCSEC) >> 1;	// One unit represents half second 
	min = inl(VENUS_MIS_RTCMIN);
	hour = inl(VENUS_MIS_RTCHR);
	day = inl(VENUS_MIS_RTCDATE1);
	day += inl(VENUS_MIS_RTCDATE2)<<8;

	return ((((day+10957)*24 + hour)*60 + min)*60 + sec);
}

int mips_rtc_set_time(unsigned long second)
{
	unsigned int day, hour, min, sec, hms;
	
	day = second / (24*60*60);
	hms = second % (24*60*60);
	hour = hms / 3600;
	min = (hms % 3600) / 60;
	sec = ((hms % 3600) % 60) * 2;	// One unit represents half second.

	day -= 10957;

	if(day < 0) {
		printk("RTC set time error! The time cannot be set to the date before year 2000.\n");
		return 0;
	}
	if(day > 16383) {
		printk("RTC day field overflow. I am so surprised that the Realtek product has been used for over 40 years. Is it still workable? Hahaha...\n");
		return 0;
	}

	outl(sec, VENUS_MIS_RTCSEC);
	outl(min, VENUS_MIS_RTCMIN);
	outl(hour, VENUS_MIS_RTCHR);
	outl(day&0x00ff, VENUS_MIS_RTCDATE1);
	outl((day&0x3f00)>>8, VENUS_MIS_RTCDATE2);

	return 0;
}

#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
static void realtek_hpt_init(unsigned int count)
{
	return;
}

static unsigned int realtek_hpt_read(void)
{
	return inl(VENUS_MIS_TC2CVR);
}

static void realtek_timer_ack(void)
{
	outl(0x100, VENUS_MIS_ISR);
}
#endif

void __init mips_time_init(void)
{
	unsigned int est_freq, flags;

	local_irq_save(flags);

//#if defined(CONFIG_MIPS_ATLAS) || defined(CONFIG_REALTEK_VENUS)
        /* Set Data mode - binary. */
//        CMOS_WRITE(CMOS_READ(RTC_CONTROL) | RTC_DM_BINARY, RTC_CONTROL);
//#endif
//enable RTC
	outb(0x5A, VENUS_MIS_RTCEN);
//Below 2 reset time
//	outb(0x40, VENUS_MIS_RTCCR);
//	outb(0x0, VENUS_MIS_RTCCR);

#ifdef CONFIG_REALTEK_DETERMINE_SYSTEM_CPU_CLOCK_FREQUENCY
	est_freq = CONFIG_REALTEK_SYSTEM_CPU_CLOCK_FREQUENCY;
	printk("Use default value: ");
#else
	est_freq = estimate_cpu_frequency ();
	printk("Estimate value: ");
#endif

	printk("CPU frequency %d.%02d MHz\n", est_freq/1000000,
	       (est_freq%1000000)*100/1000000);

        cpu_khz = est_freq / 1000;

	local_irq_restore(flags);

#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
	mips_hpt_frequency = 27000000;
	mips_hpt_read = realtek_hpt_read;
	mips_hpt_init = realtek_hpt_init;
	mips_timer_ack = realtek_timer_ack;
#else
	mips_hpt_frequency = est_freq/2;
#endif
	rtc_get_time = mips_rtc_get_time;
        rtc_set_time = mips_rtc_set_time;
}

void __init mips_timer_setup(struct irqaction *irq)
{
	/* we are using the cpu counter for timer interrupts */
//Is this just to show timer interrupt information in /proc/interrupts?
	irq->handler = no_action;     /* we use our own handler */
//	setup_irq(MIPS_CPU_TIMER_IRQ, irq);
	setup_irq(6, irq);
#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
	outl(0, VENUS_MIS_TC2CR);		/* set TC2CVR = 0 */
	outl(27000000/HZ, VENUS_MIS_TC2TVR);
	outl(0X80000000, VENUS_MIS_TC2ICR);
	outl(0xC0000000, VENUS_MIS_TC2CR);
#endif

        /* to generate the first timer interrupt */
#ifndef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
	write_c0_cause(read_c0_cause() & ~0x08000000);
	write_c0_compare (read_c0_count() + mips_hpt_frequency/HZ);
#endif
	set_c0_status(ALLINTS);
#ifndef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
	clear_c0_status(IE_IRQ4);
#endif
	clear_c0_status(IE_IRQ1);
	clear_c0_cause(IE_IRQ0);
	clear_c0_cause(IE_IRQ1);
	clear_c0_cause(IE_IRQ2);
	clear_c0_cause(IE_IRQ3);
#ifndef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
	clear_c0_cause(IE_IRQ4);
#else
	clear_c0_cause(IE_IRQ5);
	clear_c0_status(IE_IRQ5);
#endif

}
