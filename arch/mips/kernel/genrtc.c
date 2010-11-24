/*
 * A glue layer that provides RTC read/write to drivers/char/genrtc.c driver
 * based on MIPS internal RTC routines.  It does take care locking
 * issues so that we are SMP/Preemption safe.
 *
 * Copyright (C) 2004 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * Please read the COPYING file for all license details.
 */

//#include <linux/spinlock.h>

#include <asm/rtc.h>
#include <asm/time.h>

// Replace spinlock with mutex because I2C transation will take a longer time to complete.
//static DEFINE_SPINLOCK(mips_rtc_lock);
static DECLARE_MUTEX(mips_rtc_lock);

unsigned int get_rtc_time(struct rtc_time *time)
{
	unsigned long nowtime;

	down(&mips_rtc_lock);
	nowtime = rtc_get_time();
	to_tm(nowtime, time);
	time->tm_year -= 1900;
	up(&mips_rtc_lock);

	return RTC_24H;
}

int set_rtc_time(struct rtc_time *time)
{
	unsigned long nowtime;
	int ret;

	down(&mips_rtc_lock);
	nowtime = mktime(time->tm_year+1900, time->tm_mon+1,
			time->tm_mday, time->tm_hour, time->tm_min,
			time->tm_sec);
	ret = rtc_set_time(nowtime);
	up(&mips_rtc_lock);

	return ret;
}

unsigned int get_rtc_ss(void)
{
	struct rtc_time h;

	get_rtc_time(&h);
	return h.tm_sec;
}

unsigned int get_rtc_alarm_time(struct rtc_time *time)
{
	unsigned long nowtime;

	down(&mips_rtc_lock);
	nowtime = rtc_alarm_get_time();
	to_tm(nowtime, time);
	time->tm_year -= 1900;
	up(&mips_rtc_lock);

	return RTC_24H;
}

int set_rtc_alarm_time(struct rtc_time *time)
{
	unsigned long nowtime;
	int ret;

	down(&mips_rtc_lock);
	nowtime = mktime(time->tm_year+1900, time->tm_mon+1,
			time->tm_mday, time->tm_hour, time->tm_min,
			time->tm_sec);
	ret = rtc_alarm_set_time(nowtime);
	up(&mips_rtc_lock);

	return ret;
}

int get_rtc_pll(struct rtc_pll_info *pll)
{
	return -EINVAL;
}

int set_rtc_pll(struct rtc_pll_info *pll)
{
	return -EINVAL;
}

