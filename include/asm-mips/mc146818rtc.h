/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Machine dependent access functions for RTC registers.
 *
 * Copyright (C) 1996, 1997, 1998, 2000 Ralf Baechle
 * Copyright (C) 2002  Maciej W. Rozycki
 */
#ifndef _ASM_MC146818RTC_H
#define _ASM_MC146818RTC_H

#include <linux/config.h>

#include <asm/addrspace.h>
#include <asm/io.h>

#ifdef CONFIG_DECSTATION
extern char *dec_rtc_base;
#define RTC_PORT(x)	CPHYSADDR(dec_rtc_base)
#define RTC_IO_EXTENT	0x100
#define RTC_IOMAPPED	0
#elif defined(CONFIG_MIPS_ITE8172) || defined(CONFIG_MIPS_IVR)
#define RTC_PORT(x)	(0x14014800 + (x))
#define RTC_IOMAPPED	0
#elif defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
#define RTC_PORT(x)	(0x0c000000 + (x))
#define RTC_IOMAPPED	0
#else
#define RTC_PORT(x)	(0x70 + (x))
#endif

/*
 * Most supported machines access the RTC index register via an ISA
 * port access but the way to access the date register differs ...
 * The DECstation directly maps the RTC memory in the CPU's address
 * space with the chipset generating necessary index write/data access
 * cycles automagically.
 */
#define CMOS_READ(addr) ({ \
rtc_ops->rtc_read_data(addr); \
})
#define CMOS_WRITE(val, addr) ({ \
rtc_ops->rtc_write_data(val, addr); \
})
#define RTC_ALWAYS_BCD \
rtc_ops->rtc_bcd_mode()

/*
 * This structure defines how to access various features of
 * different machine types and how to access them.
 */
struct rtc_ops {
	/* How to access the RTC register in a DS1287.  */
	unsigned char (*rtc_read_data)(unsigned long addr);
	void (*rtc_write_data)(unsigned char data, unsigned long addr);
	int (*rtc_bcd_mode)(void);
};

extern struct rtc_ops *rtc_ops;

#ifdef CONFIG_DECSTATION
#define RTC_IRQ 0
#elif defined(CONFIG_MIPS_ITE8172) || defined(CONFIG_MIPS_IVR)
#include <asm/it8172/it8172_int.h>
#define RTC_IRQ	IT8172_RTC_IRQ
#elif defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
#undef RTC_IRQ
#else
#define RTC_IRQ	8
#endif

#define RTC_DEC_YEAR	0x3f	/* Where we store the real year on DECs.  */

#endif /* _ASM_MC146818RTC_H */
