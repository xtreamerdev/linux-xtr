
/*
 *  linux/arch/mips/dec/time.c
 *
 *  Copyright (C) 1991, 1992, 1995  Linus Torvalds
 *  Copyright (C) 2000  Maciej W. Rozycki
 *
 * This file contains the time handling details for PC-style clocks as
 * found in some MIPS systems.
 *
 */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/interrupt.h>

#include <asm/bootinfo.h>
#include <asm/mipsregs.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/dec/machtype.h>
#include <asm/dec/ioasic.h>
#include <asm/dec/ioasic_addrs.h>

#include <linux/mc146818rtc.h>
#include <linux/timex.h>

#include <asm/div64.h>

extern volatile unsigned long wall_jiffies;
extern rwlock_t xtime_lock;

/*
 * Change this if you have some constant time drift
 */
/* This is the value for the PC-style PICs. */
/* #define USECS_PER_JIFFY (1000020/HZ) */

/* This is for machines which generate the exact clock. */
#define USECS_PER_JIFFY (1000000/HZ)
#define USECS_PER_JIFFY_FRAC (0x100000000*1000000/HZ&0xffffffff)

/* Cycle counter value at the previous timer interrupt.. */

static unsigned int timerhi = 0, timerlo = 0;

/*
 * Cached "1/(clocks per usec)*2^32" value.
 * It has to be recalculated once each jiffy.
 */
static unsigned long cached_quotient = 0;

/* Last jiffy when do_fast_gettimeoffset() was called. */
static unsigned long last_jiffies = 0;

/*
 * On MIPS only R4000 and better have a cycle counter.
 *
 * FIXME: Does playing with the RP bit in c0_status interfere with this code?
 */
static unsigned long do_fast_gettimeoffset(void)
{
	u32 count;
	unsigned long res, tmp;
	unsigned long quotient;

	tmp = jiffies;

	quotient = cached_quotient;

	if (last_jiffies != tmp) {
	    last_jiffies = tmp;
	    if (last_jiffies != 0) {
		unsigned long r0;
		__asm__(".set	push\n\t"
			".set	mips3\n\t"
			"lwu	%0,%3\n\t"
			"dsll32	%1,%2,0\n\t"
			"or	%1,%1,%0\n\t"
			"ddivu	$0,%1,%4\n\t"
			"mflo	%1\n\t"
			"dsll32	%0,%5,0\n\t"
			"or	%0,%0,%6\n\t"
			"ddivu	$0,%0,%1\n\t"
			"mflo	%0\n\t"
			".set	pop"
			: "=&r" (quotient), "=&r" (r0)
			: "r" (timerhi), "m" (timerlo),
			  "r" (tmp), "r" (USECS_PER_JIFFY),
			  "r" (USECS_PER_JIFFY_FRAC));
		cached_quotient = quotient;
	    }
	}
	/* Get last timer tick in absolute kernel time */
	count = read_32bit_cp0_register(CP0_COUNT);

	/* .. relative to previous jiffy (32 bits is enough) */
	count -= timerlo;
//printk("count: %08lx, %08lx:%08lx\n", count, timerhi, timerlo);

	__asm__("multu	%2,%3"
		: "=l" (tmp), "=h" (res)
		: "r" (count), "r" (quotient));

	/*
	 * Due to possible jiffies inconsistencies, we need to check 
	 * the result so that we'll get a timer that is monotonic.
	 */
	if (res >= USECS_PER_JIFFY)
		res = USECS_PER_JIFFY - 1;

	return res;
}

static unsigned long do_ioasic_gettimeoffset(void)
{
	u32 count;
	unsigned long res, tmp;
	unsigned long quotient;

	tmp = jiffies;

	quotient = cached_quotient;

	if (last_jiffies != tmp) {
		last_jiffies = tmp;
		if (last_jiffies != 0) {
			unsigned long r0;
			do_div64_32(r0, timerhi, timerlo, tmp);
			do_div64_32(quotient, USECS_PER_JIFFY,
				    USECS_PER_JIFFY_FRAC, r0);
			cached_quotient = quotient;
		}
	}
	/* Get last timer tick in absolute kernel time */
	count = ioasic_read(FCTR);

	/* .. relative to previous jiffy (32 bits is enough) */
	count -= timerlo;
//printk("count: %08x, %08x:%08x\n", count, timerhi, timerlo);

	__asm__("multu	%2,%3"
		: "=l" (tmp), "=h" (res)
		: "r" (count), "r" (quotient));

	/*
	 * Due to possible jiffies inconsistencies, we need to check
	 * the result so that we'll get a timer that is monotonic.
	 */
	if (res >= USECS_PER_JIFFY)
        	res = USECS_PER_JIFFY - 1;

	return res;
}

/* This function must be called with interrupts disabled 
 * It was inspired by Steve McCanne's microtime-i386 for BSD.  -- jrs
 * 
 * However, the pc-audio speaker driver changes the divisor so that
 * it gets interrupted rather more often - it loads 64 into the
 * counter rather than 11932! This has an adverse impact on
 * do_gettimeoffset() -- it stops working! What is also not
 * good is that the interval that our timer function gets called
 * is no longer 10.0002 ms, but 9.9767 ms. To get around this
 * would require using a different timing source. Maybe someone
 * could use the RTC - I know that this can interrupt at frequencies
 * ranging from 8192Hz to 2Hz. If I had the energy, I'd somehow fix
 * it so that at startup, the timer code in sched.c would select
 * using either the RTC or the 8253 timer. The decision would be
 * based on whether there was any other device around that needed
 * to trample on the 8253. I'd set up the RTC to interrupt at 1024 Hz,
 * and then do some jiggery to have a version of do_timer that 
 * advanced the clock by 1/1024 s. Every time that reached over 1/100
 * of a second, then do all the old code. If the time was kept correct
 * then do_gettimeoffset could just return 0 - there is no low order
 * divider that can be accessed.
 *
 * Ideally, you would be able to use the RTC for the speaker driver,
 * but it appears that the speaker driver really needs interrupt more
 * often than every 120 us or so.
 *
 * Anyway, this needs more thought....          pjsg (1993-08-28)
 * 
 * If you are really that interested, you should be reading
 * comp.protocols.time.ntp!
 */

#define TICK_SIZE tick

static unsigned long do_slow_gettimeoffset(void)
{
	/*
	 * This is a kludge until I find a way for the
	 * DECstations without bus cycle counter. HK
	 */
	return 0;
}

static unsigned long (*do_gettimeoffset) (void) = do_slow_gettimeoffset;

/*
 * This version of gettimeofday has near microsecond resolution.
 */
void do_gettimeofday(struct timeval *tv)
{
	unsigned long flags;

	read_lock_irqsave(&xtime_lock, flags);
	*tv = xtime;
	tv->tv_usec += do_gettimeoffset();

	/*
	 * xtime is atomically updated in timer_bh. jiffies - wall_jiffies
	 * is nonzero if the timer bottom half hasnt executed yet.
	 */
	if (jiffies - wall_jiffies)
		tv->tv_usec += USECS_PER_JIFFY;

	read_unlock_irqrestore(&xtime_lock, flags);

	if (tv->tv_usec >= 1000000) {
		tv->tv_usec -= 1000000;
		tv->tv_sec++;
	}
}

void do_settimeofday(struct timeval *tv)
{
	write_lock_irq(&xtime_lock);

	/* This is revolting. We need to set the xtime.tv_usec
	 * correctly. However, the value in this location is
	 * is value at the last tick.
	 * Discover what correction gettimeofday
	 * would have done, and then undo it!
	 */
	tv->tv_usec -= do_gettimeoffset();

	if (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		tv->tv_sec--;
	}

	xtime = *tv;
	time_adjust = 0;		/* stop active adjtime() */
	time_status |= STA_UNSYNC;
	time_maxerror = NTP_PHASE_LIMIT;
	time_esterror = NTP_PHASE_LIMIT;

	write_unlock_irq(&xtime_lock);
}

/*
 * In order to set the CMOS clock precisely, set_rtc_mmss has to be
 * called 500 ms after the second nowtime has started, because when
 * nowtime is written into the registers of the CMOS clock, it will
 * jump to the next second precisely 500 ms later. Check the Motorola
 * MC146818A or Dallas DS12887 data sheet for details.
 */
static int set_rtc_mmss(unsigned long nowtime)
{
	int retval = 0;
	int real_seconds, real_minutes, cmos_minutes;
	unsigned char save_control, save_freq_select;

	save_control = CMOS_READ(RTC_CONTROL);	/* tell the clock it's being set */
	CMOS_WRITE((save_control | RTC_SET), RTC_CONTROL);

	save_freq_select = CMOS_READ(RTC_FREQ_SELECT);	/* stop and reset prescaler */
	CMOS_WRITE((save_freq_select | RTC_DIV_RESET2), RTC_FREQ_SELECT);

	cmos_minutes = CMOS_READ(RTC_MINUTES);
	if (!(save_control & RTC_DM_BINARY) || RTC_ALWAYS_BCD)
		BCD_TO_BIN(cmos_minutes);

	/*
	 * since we're only adjusting minutes and seconds,
	 * don't interfere with hour overflow. This avoids
	 * messing with unknown time zones but requires your
	 * RTC not to be off by more than 15 minutes
	 */
	real_seconds = nowtime % 60;
	real_minutes = nowtime / 60;
	if (((abs(real_minutes - cmos_minutes) + 15) / 30) & 1)
		real_minutes += 30;	/* correct for half hour time zone */
	real_minutes %= 60;

	if (abs(real_minutes - cmos_minutes) < 30) {
		if (!(save_control & RTC_DM_BINARY) || RTC_ALWAYS_BCD) {
			BIN_TO_BCD(real_seconds);
			BIN_TO_BCD(real_minutes);
		}
		CMOS_WRITE(real_seconds, RTC_SECONDS);
		CMOS_WRITE(real_minutes, RTC_MINUTES);
	} else {
		printk(KERN_WARNING
		       "set_rtc_mmss: can't update from %d to %d\n",
		       cmos_minutes, real_minutes);
		retval = -1;
	}

	/* The following flags have to be released exactly in this order,
	 * otherwise the DS12887 (popular MC146818A clone with integrated
	 * battery and quartz) will not reset the oscillator and will not
	 * update precisely 500 ms later. You won't find this mentioned in
	 * the Dallas Semiconductor data sheets, but who believes data
	 * sheets anyway ...                           -- Markus Kuhn
	 */
	CMOS_WRITE(save_control, RTC_CONTROL);
	CMOS_WRITE(save_freq_select, RTC_FREQ_SELECT);

	return retval;
}

/* last time the cmos clock got updated */
static long last_rtc_update = 0;

/*
 * timer_interrupt() needs to keep up the real-time clock,
 * as well as call the "do_timer()" routine every clocktick
 */
static void inline
timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	volatile unsigned char dummy;

	dummy = CMOS_READ(RTC_REG_C);	/* ACK RTC Interrupt */

	if (!user_mode(regs)) {
		if (prof_buffer && current->pid) {
			extern int _stext;
			unsigned long pc = regs->cp0_epc;

			pc -= (unsigned long) &_stext;
			pc >>= prof_shift;
			/*
			 * Dont ignore out-of-bounds pc values silently,
			 * put them into the last histogram slot, so if
			 * present, they will show up as a sharp peak.
			 */
			if (pc > prof_len - 1)
				pc = prof_len - 1;
			atomic_inc((atomic_t *) & prof_buffer[pc]);
		}
	}
	do_timer(regs);

	/*
	 * If we have an externally synchronized Linux clock, then update
	 * CMOS clock accordingly every ~11 minutes. Set_rtc_mmss() has to be
	 * called as close as possible to 500 ms before the new second starts.
	 */
	read_lock(&xtime_lock);
	if ((time_status & STA_UNSYNC) == 0
	    && xtime.tv_sec > last_rtc_update + 660
	    && xtime.tv_usec >= 500000 - tick / 2
	    && xtime.tv_usec <= 500000 + tick / 2) {
		if (set_rtc_mmss(xtime.tv_sec) == 0)
			last_rtc_update = xtime.tv_sec;
		else
			/* do it again in 60 s */
			last_rtc_update = xtime.tv_sec - 600;
	}
	/* As we return to user mode fire off the other CPU schedulers.. this is
	   basically because we don't yet share IRQ's around. This message is
	   rigged to be safe on the 386 - basically it's a hack, so don't look
	   closely for now.. */
	/*smp_message_pass(MSG_ALL_BUT_SELF, MSG_RESCHEDULE, 0L, 0); */
	read_unlock(&xtime_lock);
}

static void r4k_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int count;

	/*
	 * The cycle counter is only 32 bit which is good for about
	 * a minute at current count rates of upto 150MHz or so.
	 */
	count = read_32bit_cp0_register(CP0_COUNT);
	timerhi += (count < timerlo);	/* Wrap around */
	timerlo = count;

	if (jiffies == ~0) {
		/*
		 * If jiffies is to overflow in this timer_interrupt we must
		 * update the timer[hi]/[lo] to make do_fast_gettimeoffset()
		 * quotient calc still valid. -arca
		 */
		write_32bit_cp0_register(CP0_COUNT, 0);
		timerhi = timerlo = 0;
	}

	timer_interrupt(irq, dev_id, regs);
}

static void ioasic_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int count;

	/*
	 * The free-running counter is 32 bit which is good for about
	 * 2 minutes, 50 seconds at possible count rates of upto 25MHz.
	 */
	count = ioasic_read(FCTR);
	timerhi += (count < timerlo);	/* Wrap around */
	timerlo = count;

	if (jiffies == ~0) {
		/*
		 * If jiffies is to overflow in this timer_interrupt we must
		 * update the timer[hi]/[lo] to make do_fast_gettimeoffset()
		 * quotient calc still valid. -arca
		 */
		ioasic_write(FCTR, 0);
		timerhi = timerlo = 0;
	}

	timer_interrupt(irq, dev_id, regs);
}

/* Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 * Assumes input in normal date format, i.e. 1980-12-31 23:59:59
 * => year=1980, mon=12, day=31, hour=23, min=59, sec=59.
 *
 * [For the Julian calendar (which was used in Russia before 1917,
 * Britain & colonies before 1752, anywhere else before 1582,
 * and is still in use by some communities) leave out the
 * -year/100+year/400 terms, and add 10.]
 *
 * This algorithm was first published by Gauss (I think).
 *
 * WARNING: this function will overflow on 2106-02-07 06:28:16 on
 * machines were long is 32-bit! (However, as time_t is signed, we
 * will already get problems at other places on 2038-01-19 03:14:08)
 */
static inline unsigned long mktime(unsigned int year, unsigned int mon,
				   unsigned int day, unsigned int hour,
				   unsigned int min, unsigned int sec)
{
	if (0 >= (int) (mon -= 2)) {	/* 1..12 -> 11,12,1..10 */
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}
	return (((
			 (unsigned long) (year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) +
			 year * 365 - 719499
		 ) * 24 + hour	/* now have hours */
		) * 60 + min	/* now have minutes */
	    ) * 60 + sec;	/* finally seconds */
}

char cyclecounter_available;

static inline void init_cycle_counter(void)
{
	switch (mips_cputype) {
	case CPU_UNKNOWN:
	case CPU_R2000:
	case CPU_R3000:
	case CPU_R3000A:
	case CPU_R3041:
	case CPU_R3051:
	case CPU_R3052:
	case CPU_R3081:
	case CPU_R3081E:
	case CPU_R6000:
	case CPU_R6000A:
	case CPU_R8000:	/* Not shure about that one, play safe */
		cyclecounter_available = 0;
		break;
	case CPU_R4000PC:
	case CPU_R4000SC:
	case CPU_R4000MC:
	case CPU_R4200:
	case CPU_R4400PC:
	case CPU_R4400SC:
	case CPU_R4400MC:
	case CPU_R4600:
	case CPU_R10000:
	case CPU_R4300:
	case CPU_R4650:
	case CPU_R4700:
	case CPU_R5000:
	case CPU_R5000A:
	case CPU_R4640:
	case CPU_NEVADA:
		cyclecounter_available = 1;
		break;
	}
}

struct irqaction irq0 = {timer_interrupt, SA_INTERRUPT, 0,
			 "timer", NULL, NULL};

void (*board_time_init) (struct irqaction * irq);

void __init time_init(void)
{
	unsigned int year, mon, day, hour, min, sec, real_year;
	int i;

	/* The Linux interpretation of the CMOS clock register contents:
	 * When the Update-In-Progress (UIP) flag goes from 1 to 0, the
	 * RTC registers show the second which has precisely just started.
	 * Let's hope other operating systems interpret the RTC the same way.
	 */
	/* read RTC exactly on falling edge of update flag */
	for (i = 0; i < 1000000; i++)	/* may take up to 1 second... */
		if (CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP)
			break;
	for (i = 0; i < 1000000; i++)	/* must try at least 2.228 ms */
		if (!(CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP))
			break;
	do {			/* Isn't this overkill ? UIP above should guarantee consistency */
		sec = CMOS_READ(RTC_SECONDS);
		min = CMOS_READ(RTC_MINUTES);
		hour = CMOS_READ(RTC_HOURS);
		day = CMOS_READ(RTC_DAY_OF_MONTH);
		mon = CMOS_READ(RTC_MONTH);
		year = CMOS_READ(RTC_YEAR);
	} while (sec != CMOS_READ(RTC_SECONDS));
	if (!(CMOS_READ(RTC_CONTROL) & RTC_DM_BINARY) || RTC_ALWAYS_BCD) {
		BCD_TO_BIN(sec);
		BCD_TO_BIN(min);
		BCD_TO_BIN(hour);
		BCD_TO_BIN(day);
		BCD_TO_BIN(mon);
		BCD_TO_BIN(year);
	}
	/*
	 * The PROM will reset the year to either '72 or '73.
	 * Therefore we store the real year separately, in one
	 * of unused BBU RAM locations.
	 */
	real_year = CMOS_READ(RTC_DEC_YEAR);
	year += real_year - 72 + 2000;

	write_lock_irq(&xtime_lock);
	xtime.tv_sec = mktime(year, mon, day, hour, min, sec);
	xtime.tv_usec = 0;
	write_unlock_irq(&xtime_lock);

	init_cycle_counter();

	if (cyclecounter_available) {
		write_32bit_cp0_register(CP0_COUNT, 0);
		do_gettimeoffset = do_fast_gettimeoffset;
		irq0.handler = r4k_timer_interrupt;
	} else if (IOASIC) {
		ioasic_write(FCTR, 0);
		do_gettimeoffset = do_ioasic_gettimeoffset;
		irq0.handler = ioasic_timer_interrupt;
        }
	board_time_init(&irq0);
}
