/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * ########################################################################
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
 * ########################################################################
 *
 * Reset the MIPS boards.
 *
 */
#include <linux/config.h>

#include <asm/io.h>
#include <asm/reboot.h>
//#include <generic.h>
#include <venus.h>
#include <platform.h>
#include <asm/time.h>
#include <linux/delay.h>

static void mips_machine_restart(char *command);
static void mips_machine_halt(void);

#ifdef	CONFIG_REALTEK_WATCHDOG
extern int kill_watchdog();
#endif

static void mips_machine_restart(char *command)
{
// This jump doesn't reset 3 CPUs and all hardware.
//	__asm__ __volatile__("li $8, 0xbfc00000\n\t"
//				"jr $8\n\t"
//				"nop\n\t");

// It is very easy to use Watchdog to reset the whole system.
	if (platform_info.board_id == C0B_dvr_board) {
// For Verona, STFPC320 will shutdown power when it finds chip reset. Therefore, we will to let it resume later.
// We no need to have semaphores here to protect RTC functions because there should be no other thread excuting when rebooting.
		rtc_alarm_set_time(rtc_get_time()+3);
	}
#ifdef	CONFIG_REALTEK_WATCHDOG
	kill_watchdog();
#else
	outl(0, VENUS_MIS_TCWCR);
#endif
	msleep(5000);		// Watchdog timeout won't happen immediately
}

static void mips_machine_halt(void)
{
	if (platform_info.board_id == C0B_dvr_board) {
#ifdef	CONFIG_REALTEK_WATCHDOG
		kill_watchdog();
#else
		outl(0, VENUS_MIS_TCWCR);
#endif
	}
	msleep(5000);		// Watchdog timeout won't happen immediately
}

void mips_reboot_setup(void)
{
	_machine_restart = mips_machine_restart;
	_machine_halt = mips_machine_halt;
	_machine_power_off = mips_machine_halt;
}
