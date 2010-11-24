/* calibrate.c: default delay calibration
 *
 * Excised from init/main.c
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#ifdef CONFIG_REALTEK_VENUS
#include <platform.h>
#endif

static unsigned long preset_lpj;
static int __init lpj_setup(char *str)
{
	preset_lpj = simple_strtoul(str,NULL,0);
	return 1;
}

__setup("lpj=", lpj_setup);

/*
 * This is the number of bits of precision for the loops_per_jiffy.  Each
 * bit takes on average 1.5/HZ seconds.  This (like the original) is a little
 * better than 1%
 */
#define LPS_PREC 8

#ifdef CONFIG_REALTEK_VENUS
static unsigned long default_lpj() 
{
	switch(platform_info.board_id) {
		case realtek_qa_board:
		case realtek_mk_board:
		case realtek_mk2_board:
		case realtek_1261_demo_board:
		case realtek_1281_demo_board:
		case realtek_neptune_qa_board:
		case realtek_photoviewer_board:
		case realtek_avhdd_demo_board:
		case realtek_pvr_demo_board:
		case realtek_pvrbox_demo_board:
		case realtek_avhdd2_demo_board:
		case realtek_1071_avhdd_mk_demo_board:
		case realtek_1261_avhdd_hdd25_demo_board:
		case realtek_neptuneB_qa_board:
		case realtek_neptune_demo_board:
		case realtek_neptuneB_demo_board:
		case realtek_1282_neptuneB_qa_board:
		case realtek_1282_neptuneB_demo_board:
		case realtek_1262_neptuneB_pc_install_demo_board:
		case realtek_1262_neptuneB_mk_board:
		case realtek_1262_neptuneB_avhdd_ewha10_mk_board:
		case realtek_1283_mars_qa_board:
		case C01_avhdd_board:
		case C01_1262_avhdd_board:
		case C02_avhdd_board:
		case C02_1262_Neptune_avhdd_board:
		case C03_pvr_8m_board:
		case C03_pvr_board:
		case C03_pvr2_board:
		case C04_pvr_board:
		case C04_pvr2_board:
		case C05_pvrbox_board:
		case C05_pvrbox2_board:
		case C06_pvr_board:
		case C07_avhdd_board:
		case C07_pvrbox_board:
		case C07_pvrbox2_board:
		case C08_pvr_board:
		case C09_pvrbox_board:
		case C09_pvrbox2_board:
		case C0A_pvr_board:
		case C0B_dvr_board:
		case C0C_avhdd_board:
		case C0D_pvr_board:
		case C0E_dvr_board:
		case C0F_1061_avhdd_hdd35_board:
		case C0F_1061_avhdd_board:
		case C0F_1262_avhdd_hdd35_board:
		case C0F_1262_avhdd_hdd25_board:
		case C10_1071_avhdd_board:
			return 995328;
		default:
			printk("Warning! Unknown board id.\n");
			return 0;
	}
}
#endif

void __devinit calibrate_delay(void)
{
	unsigned long ticks, loopbit;
	int lps_precision = LPS_PREC;
#ifdef CONFIG_REALTEK_VENUS
	unsigned long offset;

#ifdef CONFIG_REALTEK_COMPACT
	preset_lpj = default_lpj();
#endif
#endif
	if (preset_lpj) {
		loops_per_jiffy = preset_lpj;
		printk("Calibrating delay loop (skipped)... "
			"%lu.%02lu BogoMIPS preset\n",
			loops_per_jiffy/(500000/HZ),
			(loops_per_jiffy/(5000/HZ)) % 100);
	} else {
		loops_per_jiffy = (1<<12);

		printk(KERN_DEBUG "Calibrating delay loop... ");
		while ((loops_per_jiffy <<= 1) != 0) {
			/* wait for "start of" clock tick */
			ticks = jiffies;
			while (ticks == jiffies)
				/* nothing */;
			/* Go .. */
			ticks = jiffies;
			__delay(loops_per_jiffy);
			ticks = jiffies - ticks;
			if (ticks)
				break;
		}

		/*
		 * Do a binary approximation to get loops_per_jiffy set to
		 * equal one clock (up to lps_precision bits)
		 */
		loops_per_jiffy >>= 1;
		loopbit = loops_per_jiffy;
		while (lps_precision-- && (loopbit >>= 1)) {
			loops_per_jiffy |= loopbit;
			ticks = jiffies;
			while (ticks == jiffies)
				/* nothing */;
			ticks = jiffies;
			__delay(loops_per_jiffy);
			if (jiffies != ticks)	/* longer than 1 tick */
				loops_per_jiffy &= ~loopbit;
		}

		/* Round the value and print it */
		printk("%lu.%02lu BogoMIPS (lpj=%lu)\n",
			loops_per_jiffy/(500000/HZ),
			(loops_per_jiffy/(5000/HZ)) % 100,
			loops_per_jiffy);
#ifdef CONFIG_REALTEK_VENUS
		offset = loops_per_jiffy-default_lpj();
		if(offset<0)
			offset = -offset;
		if(offset > 5)
			printk("==================== Warning! The calculated loops_per_jiffy is not similar to the default one. ====================\n");
#endif
	}

}
