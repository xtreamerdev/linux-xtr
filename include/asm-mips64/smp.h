/*
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2000 - 2001 by Kanoj Sarcar (kanoj@sgi.com)
 * Copyright (C) 2000 - 2001 by Silicon Graphics, Inc.
 */

#ifndef __ASM_SMP_H
#define __ASM_SMP_H

#include <linux/config.h>
#include <linux/threads.h>

#ifdef CONFIG_SMP

#include <linux/irq.h>

#if 0
struct cpuinfo_mips {				/* XXX  */
	unsigned long loops_per_sec;
	unsigned long last_asn;
	unsigned long *pgd_cache;
	unsigned long *pte_cache;
	unsigned long pgtable_cache_sz;
	unsigned long ipi_count;
	unsigned long irq_attempt[NR_IRQS];
	unsigned long smp_local_irq_count;
	unsigned long prof_multiplier;
	unsigned long prof_counter;
} __attribute__((aligned(64)));

extern struct cpuinfo_mips cpu_data[NR_CPUS];
#endif

#define smp_processor_id()	(current->processor)

#define PROC_CHANGE_PENALTY	20

/* Map from cpu id to sequential logical cpu number.  This will only
   not be idempotent when cpus failed to come on-line.  */
extern int __cpu_number_map[NR_CPUS];
#define cpu_number_map(cpu)  __cpu_number_map[cpu]

/* The reverse map from sequential logical cpu number to cpu id.  */
extern int __cpu_logical_map[NR_CPUS];
#define cpu_logical_map(cpu)  __cpu_logical_map[cpu]

#endif /* CONFIG_SMP */

#define NO_PROC_ID	(-1)

typedef unsigned long   cpumask_t;

#define CPUMASK_CLRALL(p)       (p) = 0
#define CPUMASK_SETB(p, bit)    (p) |= 1 << (bit)
#define CPUMASK_CLRB(p, bit)    (p) &= ~(1ULL << (bit))
#define CPUMASK_TSTB(p, bit)    ((p) & (1ULL << (bit)))

#endif /* __ASM_SMP_H */
