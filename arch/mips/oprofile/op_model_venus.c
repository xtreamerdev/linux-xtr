/*
 *  * This file is subject to the terms and conditions of the GNU General Public
 *   * License.  See the file "COPYING" in the main directory of this archive
 *    * for more details.
 *     *
 *      * Copyright (C) 2007 by Realtek
 *       */


#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/smp.h>
#include <linux/oprofile.h>
#include <linux/profile.h>
#include <linux/init.h>
#include <asm/ptrace.h>
#include <asm/mipsregs.h>

#include "op_impl.h"

struct op_mips_model op_model_venus;

#ifdef CONFIG_VENUS_OPROFILE_FREQ
static unsigned int venus_freq = CONFIG_VENUS_OPROFILE_FREQ;
#else
static unsigned int venus_freq  = 2500;
#endif
static unsigned int est_interval;

extern int (*perf_irq)(struct pt_regs *regs);
extern int null_perf_irq(struct pt_regs *regs);

static volatile unsigned int cnt = 0;
static volatile unsigned int run = 0;
static int venus_perfcount_handler(struct pt_regs *regs) {
        //oprofile_add_sample(regs, 0);
	/* keep the internal interrupt going */

	if (run) 
		oprofile_add_sample(regs, 0);

	write_c0_compare(read_c0_count() + est_interval);
        return 1;
}

static void venus_cpu_start(void *args) 
{
	unsigned int status;
	run = 1;
	est_interval = CONFIG_REALTEK_SYSTEM_CPU_CLOCK_FREQUENCY/venus_freq;
	perf_irq = venus_perfcount_handler;
	write_c0_cause(read_c0_cause() & ~0x08000000);

	write_c0_compare(read_c0_count() + est_interval);
	/* turn on internal timer interrupt */
        status = read_c0_status();
        status |= IE_IRQ5;
        write_c0_status(status);
	printk("venus cpu timer started \n");
}

static void venus_cpu_stop(void *args)
{
	unsigned int status;
	unsigned long flags;
	local_irq_save(flags);
	run = 0;
	clear_c0_cause(IE_IRQ5);
	status = read_c0_status();
	status &= ~(IE_IRQ5);
        write_c0_status(status);
	est_interval = CONFIG_REALTEK_SYSTEM_CPU_CLOCK_FREQUENCY/2;
	local_irq_restore(flags);
	printk("venus cpu timer stopped\n");
}

static void venus_cpu_setup (void *args)
{
	return;
}

static void venus_reg_setup(struct op_counter_config *ctr)
{
	return;
}

static int __init venus_init(void) {
	if (current_cpu_data.cputype != CPU_4KEC) {
		return -ENODEV;
	}
	op_model_venus.cpu_type = "timer";
	return 0;
}

static void venus_exit(void)
{
	/* turn off venus internal clock interrupt */
	clear_c0_status(IE_IRQ5);
        perf_irq = null_perf_irq;
}


struct op_mips_model op_model_venus = {
        .reg_setup      = venus_reg_setup,
        .cpu_setup      = venus_cpu_setup,
        .init           = venus_init,
        .exit           = venus_exit,
        .cpu_start      = venus_cpu_start,
        .cpu_stop       = venus_cpu_stop,
};


