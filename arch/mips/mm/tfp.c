/* $Id: tfp.c,v 1.7 1999/08/09 19:43:17 harald Exp $
 *
 * tfp.c: MMU and cache routines specific to the r8000 (TFP).
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/sgialib.h>
#include <asm/mmu_context.h>

extern unsigned long mips_tlb_entries;

/* Cache operations. XXX Write these dave... */
static inline void tfp_flush_cache_all(void)
{
	/* XXX */
}

static void tfp_flush_cache_mm(struct mm_struct *mm)
{
	/* XXX */
}

static void tfp_flush_cache_range(struct mm_struct *mm,
				  unsigned long start,
				  unsigned long end)
{
	/* XXX */
}

static void tfp_flush_cache_page(struct vm_area_struct *vma,
				 unsigned long page)
{
	/* XXX */
}

static void tfp_flush_page_to_ram(unsigned long page)
{
	/* XXX */
}

static void tfp_flush_cache_sigtramp(unsigned long page)
{
	/* XXX */
}

/* TLB operations. XXX Write these dave... */
inline void flush_tlb_all(void)
{
	/* XXX */
}

void flush_tlb_mm(struct mm_struct *mm)
{
	/* XXX */
}

void flush_tlb_range(struct mm_struct *mm, unsigned long start,
				unsigned long end)
{
	/* XXX */
}

void flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	/* XXX */
}

void load_pgd(unsigned long pg_dir)
{
}

void pgd_init(unsigned long page)
{
}

void add_wired_entry(unsigned long entrylo0, unsigned long entrylo1,
				unsigned long entryhi, unsigned long pagemask)
{
        /* XXX */
}

void __init ld_mmu_tfp(void)
{
	flush_cache_all = tfp_flush_cache_all;
	flush_cache_mm = tfp_flush_cache_mm;
	flush_cache_range = tfp_flush_cache_range;
	flush_cache_page = tfp_flush_cache_page;
	flush_cache_sigtramp = tfp_flush_cache_sigtramp;
	flush_page_to_ram = tfp_flush_page_to_ram;

	flush_cache_all();
	flush_tlb_all();
}
