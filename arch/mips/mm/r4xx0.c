/* $Id: r4xx0.c,v 1.19 1996/08/02 11:11:36 dm Exp $
 * r4xx0.c: R4000 processor variant specific MMU/Cache routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/config.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/autoconf.h>

#include <asm/sgi.h>
#include <asm/sgimc.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/bootinfo.h>
#include <asm/sgialib.h>
#include <asm/mmu_context.h>

/* CP0 hazard avoidance. */
#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
				     "nop; nop; nop; nop; nop; nop;\n\t" \
				     ".set reorder\n\t")

/* Primary cache parameters. */
static int icache_size, dcache_size; /* Size in bytes */
static int ic_lsize, dc_lsize;       /* LineSize in bytes */

/* Secondary cache (if present) parameters. */
static scache_size, sc_lsize;        /* Again, in bytes */

#include <asm/cacheops.h>
#include <asm/r4kcache.h>

#undef DEBUG_CACHE

/*
 * Zero an entire page.
 */

static void r4k_clear_page_d16(unsigned long page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tcache\t%3,(%0)\n\t"
		"sd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"cache\t%3,16(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"cache\t%3,-32(%0)\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"cache\t%3,-16(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D)
		:"$1","memory");
}

static void r4k_clear_page_d32(unsigned long page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tcache\t%3,(%0)\n\t"
		"sd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"cache\t%3,-32(%0)\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D)
		:"$1","memory");
}


/*
 * This is still inefficient.  We only can do better if we know the
 * virtual address where the copy will be accessed.
 */

static void r4k_copy_page_d16(unsigned long to, unsigned long from)
{
	unsigned long dummy1, dummy2;
	unsigned long reg1, reg2, reg3, reg4;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%8\n"
		"1:\tcache\t%9,(%0)\n\t"
		"lw\t%2,(%1)\n\t"
		"lw\t%3,4(%1)\n\t"
		"lw\t%4,8(%1)\n\t"
		"lw\t%5,12(%1)\n\t"
		"sw\t%2,(%0)\n\t"
		"sw\t%3,4(%0)\n\t"
		"sw\t%4,8(%0)\n\t"
		"sw\t%5,12(%0)\n\t"
		"cache\t%9,16(%0)\n\t"
		"lw\t%2,16(%1)\n\t"
		"lw\t%3,20(%1)\n\t"
		"lw\t%4,24(%1)\n\t"
		"lw\t%5,28(%1)\n\t"
		"sw\t%2,16(%0)\n\t"
		"sw\t%3,20(%0)\n\t"
		"sw\t%4,24(%0)\n\t"
		"sw\t%5,28(%0)\n\t"
		"cache\t%9,32(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"lw\t%2,-32(%1)\n\t"
		"lw\t%3,-28(%1)\n\t"
		"lw\t%4,-24(%1)\n\t"
		"lw\t%5,-20(%1)\n\t"
		"sw\t%2,-32(%0)\n\t"
		"sw\t%3,-28(%0)\n\t"
		"sw\t%4,-24(%0)\n\t"
		"sw\t%5,-20(%0)\n\t"
		"cache\t%9,-16(%0)\n\t"
		"lw\t%2,-16(%1)\n\t"
		"lw\t%3,-12(%1)\n\t"
		"lw\t%4,-8(%1)\n\t"
		"lw\t%5,-4(%1)\n\t"
		"sw\t%2,-16(%0)\n\t"
		"sw\t%3,-12(%0)\n\t"
		"sw\t%4,-8(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sw\t%5,-4(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2),
		 "=&r" (reg1), "=&r" (reg2), "=&r" (reg3), "=&r" (reg4)
		:"0" (to), "1" (from),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D));
}

static void r4k_copy_page_d32(unsigned long to, unsigned long from)
{
	unsigned long dummy1, dummy2;
	unsigned long reg1, reg2, reg3, reg4;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%8\n"
		"1:\tcache\t%9,(%0)\n\t"
		"lw\t%2,(%1)\n\t"
		"lw\t%3,4(%1)\n\t"
		"lw\t%4,8(%1)\n\t"
		"lw\t%5,12(%1)\n\t"
		"sw\t%2,(%0)\n\t"
		"sw\t%3,4(%0)\n\t"
		"sw\t%4,8(%0)\n\t"
		"sw\t%5,12(%0)\n\t"
		"lw\t%2,16(%1)\n\t"
		"lw\t%3,20(%1)\n\t"
		"lw\t%4,24(%1)\n\t"
		"lw\t%5,28(%1)\n\t"
		"sw\t%2,16(%0)\n\t"
		"sw\t%3,20(%0)\n\t"
		"sw\t%4,24(%0)\n\t"
		"sw\t%5,28(%0)\n\t"
		"cache\t%9,32(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"lw\t%2,-32(%1)\n\t"
		"lw\t%3,-28(%1)\n\t"
		"lw\t%4,-24(%1)\n\t"
		"lw\t%5,-20(%1)\n\t"
		"sw\t%2,-32(%0)\n\t"
		"sw\t%3,-28(%0)\n\t"
		"sw\t%4,-24(%0)\n\t"
		"sw\t%5,-20(%0)\n\t"
		"lw\t%2,-16(%1)\n\t"
		"lw\t%3,-12(%1)\n\t"
		"lw\t%4,-8(%1)\n\t"
		"lw\t%5,-4(%1)\n\t"
		"sw\t%2,-16(%0)\n\t"
		"sw\t%3,-12(%0)\n\t"
		"sw\t%4,-8(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sw\t%5,-4(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2),
		 "=&r" (reg1), "=&r" (reg2), "=&r" (reg3), "=&r" (reg4)
		:"0" (to), "1" (from),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D));
}

/*
 * If you think for one second that this stuff coming up is a lot
 * of bulky code eating too many kernel cache lines.  Think _again_.
 *
 * Consider:
 * 1) Taken branches have a 3 cycle penalty on R4k
 * 2) The branch itself is a real dead cycle on even R4600/R5000.
 * 3) Only one of the following variants of each type is even used by
 *    the kernel based upon the cache parameters we detect at boot time.
 *
 * QED.
 */

static inline void r4k_flush_cache_all_s16d16i16(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache16(); blast_icache16(); blast_scache16();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s32d16i16(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache16(); blast_icache16(); blast_scache32();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s64d16i16(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache16(); blast_icache16(); blast_scache64();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s128d16i16(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache16(); blast_icache16(); blast_scache128();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s16d32i32(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache32(); blast_icache32(); blast_scache16();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s32d32i32(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache32(); blast_icache32(); blast_scache32();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s64d32i32(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache32(); blast_icache32(); blast_scache64();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s128d32i32(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache32(); blast_icache32(); blast_scache128();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_d16i16(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache16(); blast_icache16();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_d32i32(void)
{
	unsigned long flags;

	save_flags(flags); cli();
	blast_dcache32(); blast_icache32();
	restore_flags(flags);
}

static inline struct vm_area_struct *
find_mm_vma(struct mm_struct *mm, unsigned long addr)
{
	struct vm_area_struct * result = NULL;

	if (mm) {
		struct vm_area_struct * tree = mm->mmap_avl;
		for (;;) {
			if (tree == avl_empty)
				break;
			if (tree->vm_end > addr) {
				result = tree;
				if (tree->vm_start <= addr)
					break;
				tree = tree->vm_avl_left;
			} else
				tree = tree->vm_avl_right;
		}
	}
	return result;
}

static void
r4k_flush_cache_range_s16d16i16(struct mm_struct *mm,
                                unsigned long start,
                                unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_mm_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s16d16i16();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_flags(flags); cli();
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache16_page(start);
					if(text)
						blast_icache16_page(start);
					blast_scache16_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void
r4k_flush_cache_range_s32d16i16(struct mm_struct *mm,
                                unsigned long start,
                                unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_mm_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s32d16i16();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_flags(flags); cli();
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache16_page(start);
					if(text)
						blast_icache16_page(start);
					blast_scache32_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s64d16i16(struct mm_struct *mm,
					    unsigned long start,
					    unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_mm_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s64d16i16();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_flags(flags); cli();
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache16_page(start);
					if(text)
						blast_icache16_page(start);
					blast_scache64_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s128d16i16(struct mm_struct *mm,
					     unsigned long start,
					     unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_mm_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s128d16i16();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_flags(flags); cli();
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache16_page(start);
					if(text)
						blast_icache16_page(start);
					blast_scache128_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s16d32i32(struct mm_struct *mm,
					    unsigned long start,
					    unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_mm_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s16d32i32();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_flags(flags); cli();
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache32_page(start);
					if(text)
						blast_icache32_page(start);
					blast_scache16_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s32d32i32(struct mm_struct *mm,
					    unsigned long start,
					    unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_mm_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s32d32i32();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_flags(flags); cli();
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache32_page(start);
					if(text)
						blast_icache32_page(start);
					blast_scache32_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s64d32i32(struct mm_struct *mm,
					    unsigned long start,
					    unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_mm_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s64d32i32();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_flags(flags); cli();
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache32_page(start);
					if(text)
						blast_icache32_page(start);
					blast_scache64_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s128d32i32(struct mm_struct *mm,
					     unsigned long start,
					     unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_mm_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s128d32i32();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_flags(flags); cli();
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache32_page(start);
					if(text)
						blast_icache32_page(start);
					blast_scache128_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_d16i16(struct mm_struct *mm,
					 unsigned long start,
					 unsigned long end)
{
	if(mm->context != 0) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
		save_flags(flags); cli();
		blast_dcache16(); blast_icache16();
		restore_flags(flags);
	}
}

static void r4k_flush_cache_range_d32i32(struct mm_struct *mm,
					 unsigned long start,
					 unsigned long end)
{
	if(mm->context != 0) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
		save_flags(flags); cli();
		blast_dcache32(); blast_icache32();
		restore_flags(flags);
	}
}

/*
 * On architectures like the Sparc, we could get rid of lines in
 * the cache created only by a certain context, but on the MIPS
 * (and actually certain Sparc's) we cannot.
 */
static void r4k_flush_cache_mm_s16d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s16d16i16();
	}
}

static void r4k_flush_cache_mm_s32d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s32d16i16();
	}
}

static void r4k_flush_cache_mm_s64d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s64d16i16();
	}
}

static void r4k_flush_cache_mm_s128d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s128d16i16();
	}
}

static void r4k_flush_cache_mm_s16d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s16d32i32();
	}
}

static void r4k_flush_cache_mm_s32d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s32d32i32();
	}
}

static void r4k_flush_cache_mm_s64d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s64d32i32();
	}
}

static void r4k_flush_cache_mm_s128d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s128d32i32();
	}
}

static void r4k_flush_cache_mm_d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_d16i16();
	}
}

static void r4k_flush_cache_mm_d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_d32i32();
	}
}

static void r4k_flush_cache_page_s16d16i16(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/* Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache16_page_indexed(page);
		if(text)
			blast_icache16_page_indexed(page);
		blast_scache16_page_indexed(page);
	} else {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
		blast_scache16_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s32d16i16(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/* Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache16_page_indexed(page);
		if(text)
			blast_icache16_page_indexed(page);
		blast_scache32_page_indexed(page);
	} else {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
		blast_scache32_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s64d16i16(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache16_page_indexed(page);
		if(text)
			blast_icache16_page_indexed(page);
		blast_scache64_page_indexed(page);
	} else {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
		blast_scache64_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s128d16i16(struct vm_area_struct *vma,
					    unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/* Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache16_page_indexed(page);
		if(text)
			blast_icache16_page_indexed(page);
		blast_scache128_page_indexed(page);
	} else {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
		blast_scache128_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s16d32i32(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
		blast_scache16_page_indexed(page);
	} else {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
		blast_scache16_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s32d32i32(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
		blast_scache32_page_indexed(page);
	} else {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
		blast_scache32_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s64d32i32(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
		blast_scache64_page_indexed(page);
	} else {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
		blast_scache64_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s128d32i32(struct vm_area_struct *vma,
					    unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
		blast_scache128_page_indexed(page);
	} else {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
		blast_scache128_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_d16i16(struct vm_area_struct *vma,
					unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/* If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm == current->mm) {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
	} else {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache16_page_indexed(page);
		blast_dcache16_page_indexed(page ^ 0x2000);
		if(text) {
			blast_icache16_page_indexed(page);
			blast_icache16_page_indexed(page ^ 0x2000);
		}
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_d32i32(struct vm_area_struct *vma,
					unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_PRESENT))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if((mm == current->mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
	} else {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_d32i32_r4600(struct vm_area_struct *vma,
					      unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_flags(flags); cli();
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_PRESENT))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if((mm == current->mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
	} else {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache32_page_indexed(page);
		blast_dcache32_page_indexed(page ^ 0x2000);
		if(text) {
			blast_icache32_page_indexed(page);
			blast_icache32_page_indexed(page ^ 0x2000);
		}
	}
out:
	restore_flags(flags);
}

/* If the addresses passed to these routines are valid, they are
 * either:
 *
 * 1) In KSEG0, so we can do a direct flush of the page.
 * 2) In KSEG2, and since every process can translate those
 *    addresses all the time in kernel mode we can do a direct
 *    flush.
 * 3) In KSEG1, no flush necessary.
 */
static void r4k_flush_page_to_ram_s16d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache16_page(page);
		blast_scache16_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s32d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache16_page(page);
		blast_scache32_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s64d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache16_page(page);
		blast_scache64_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s128d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache16_page(page);
		blast_scache128_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s16d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache32_page(page);
		blast_scache16_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s32d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache32_page(page);
		blast_scache32_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s64d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache32_page(page);
		blast_scache64_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s128d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache32_page(page);
		blast_scache128_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache16_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_flags(flags); cli();
		blast_dcache32_page(page);
		restore_flags(flags);
	}
}

/*
 * R4600 v2.0 bug: "The CACHE instructions Hit_Writeback_Invalidate_D,
 * Hit_Writeback_D, Hit_Invalidate_D and Create_Dirty_Exclusive_D will only
 * operate correctly if the internal data cache refill buffer is empty.  These
 * CACHE instructions should be separated from any potential data cache miss
 * by a load instruction to an uncached address to empty the response buffer."
 * (Revision 2.0 device errata from IDT available on http://www.idt.com/
 * in .pdf format.)
 */
static void r4k_flush_page_to_ram_d32i32_r4600(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		/* #if 1 */
		printk("r4600_cram[%08lx]", page);
#endif
		/*
		 * Workaround for R4600 bug.  Explanation see above.
		 */
		*(volatile unsigned long *)KSEG1;

		save_flags(flags); cli();
		blast_dcache32_page(page);
		blast_dcache32_page(page ^ 0x2000);
#ifdef CONFIG_SGI
		{
			unsigned long tmp1, tmp2;
			/*
			 * SGI goo.  Have to check this closer ...
			 */
			__asm__ __volatile__("
			.set noreorder
			.set mips3
			li	%0, 0x1
			dsll	%0, 31
			or	%0, %0, %2
			lui	%1, 0x9000
			dsll32	%1, 0
			or	%0, %0, %1
			daddu	%1, %0, 0x0fe0
			li	%2, 0x80
			mtc0	%2, $12
			nop; nop; nop; nop;
1:			sw	$0, 0(%0)
			bltu	%0, %1, 1b
			daddu	%0, 32
			mtc0	$0, $12
			nop; nop; nop; nop;
			mtc0	%3, $12
			nop; nop; nop; nop;
			.set mips0
			.set reorder"
			: "=&r" (tmp1), "=&r" (tmp2),
			  "=&r" (page), "=&r" (flags)
			: "2" (page & 0x0007f000), "3" (flags));
		}
#endif /* CONFIG_SGI */
	}
}

static void r4k_flush_cache_sigtramp(unsigned long addr)
{
	addr &= ~(dc_lsize - 1);
	flush_dcache_line(addr);
	flush_dcache_line(addr + dc_lsize);
	flush_icache_line(addr);
	flush_icache_line(addr + dc_lsize);
}

#undef DEBUG_TLB
#undef DEBUG_TLBUPDATE

#define NTLB_ENTRIES       48  /* Fixed on all R4XX0 variants... */
#define NTLB_ENTRIES_HALF  24  /* Fixed on all R4XX0 variants... */

static inline void r4k_flush_tlb_all(void)
{
	unsigned long flags;
	unsigned long old_ctx;
	int entry;

#ifdef DEBUG_TLB
	printk("[tlball]");
#endif

	save_flags(flags); cli();
	/* Save old context and create impossible VPN2 value */
	old_ctx = (get_entryhi() & 0xff);
	set_entryhi(KSEG0);
	set_entrylo0(0);
	set_entrylo1(0);
	BARRIER;

	entry = 0;

	/* Blast 'em all away. */
	while(entry < NTLB_ENTRIES) {
		set_index(entry);
		BARRIER;
		tlb_write_indexed();
		BARRIER;
		entry++;
	}
	BARRIER;
	set_entryhi(old_ctx);
	restore_flags(flags);
}

static void r4k_flush_tlb_mm(struct mm_struct *mm)
{
	if(mm->context != 0) {
		unsigned long flags;

#ifdef DEBUG_TLB
		printk("[tlbmm<%d>]", mm->context);
#endif
		save_flags(flags); cli();
		get_new_mmu_context(mm, asid_cache);
		if(mm == current->mm)
			set_entryhi(mm->context & 0xff);
		restore_flags(flags);
	}
}

static void r4k_flush_tlb_range(struct mm_struct *mm, unsigned long start,
				unsigned long end)
{
	if(mm->context != 0) {
		unsigned long flags;
		int size;

#ifdef DEBUG_TLB
		printk("[tlbrange<%02x,%08lx,%08lx>]", (mm->context & 0xff),
		       start, end);
#endif
		save_flags(flags); cli();
		size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
		size = (size + 1) >> 1;
		if(size <= NTLB_ENTRIES_HALF) {
			int oldpid = (get_entryhi() & 0xff);
			int newpid = (mm->context & 0xff);

			start &= (PAGE_MASK << 1);
			end += ((PAGE_SIZE << 1) - 1);
			end &= (PAGE_MASK << 1);
			while(start < end) {
				int idx;

				set_entryhi(start | newpid);
				start += (PAGE_SIZE << 1);
				BARRIER;
				tlb_probe();
				BARRIER;
				idx = get_index();
				set_entrylo0(0);
				set_entrylo1(0);
				set_entryhi(KSEG0);
				BARRIER;
				if(idx < 0)
					continue;
				tlb_write_indexed();
				BARRIER;
			}
			set_entryhi(oldpid);
		} else {
			get_new_mmu_context(mm, asid_cache);
			if(mm == current->mm)
				set_entryhi(mm->context & 0xff);
		}
		restore_flags(flags);
	}
}

static void r4k_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	if(vma->vm_mm->context != 0) {
		unsigned long flags;
		int oldpid, newpid, idx;

#ifdef DEBUG_TLB
		printk("[tlbpage<%d,%08lx>]", vma->vm_mm->context, page);
#endif
		newpid = (vma->vm_mm->context & 0xff);
		page &= (PAGE_MASK << 1);
		save_flags(flags); cli();
		oldpid = (get_entryhi() & 0xff);
		set_entryhi(page | newpid);
		BARRIER;
		tlb_probe();
		BARRIER;
		idx = get_index();
		set_entrylo0(0);
		set_entrylo1(0);
		set_entryhi(KSEG0);
		if(idx < 0)
			goto finish;
		BARRIER;
		tlb_write_indexed();

	finish:
		BARRIER;
		set_entryhi(oldpid);
		restore_flags(flags);
	}
}

/* Load a new root pointer into the TLB. */
static void r4k_load_pgd(unsigned long pg_dir)
{
}

static void r4k_pgd_init(unsigned long page)
{
	unsigned long *p = (unsigned long *) page;
	int i;

	for(i = 0; i < 1024; i+=8) {
		p[i + 0] = (unsigned long) invalid_pte_table;
		p[i + 1] = (unsigned long) invalid_pte_table;
		p[i + 2] = (unsigned long) invalid_pte_table;
		p[i + 3] = (unsigned long) invalid_pte_table;
		p[i + 4] = (unsigned long) invalid_pte_table;
		p[i + 5] = (unsigned long) invalid_pte_table;
		p[i + 6] = (unsigned long) invalid_pte_table;
		p[i + 7] = (unsigned long) invalid_pte_table;
	}
}

#ifdef DEBUG_TLBUPDATE
static unsigned long ehi_debug[NTLB_ENTRIES];
static unsigned long el0_debug[NTLB_ENTRIES];
static unsigned long el1_debug[NTLB_ENTRIES];
#endif

/* We will need multiple versions of update_mmu_cache(), one that just
 * updates the TLB with the new pte(s), and another which also checks
 * for the R4k "end of page" hardware bug and does the needy.
 */
static void r4k_update_mmu_cache(struct vm_area_struct * vma,
				 unsigned long address, pte_t pte)
{
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int idx, pid;

	pid = (get_entryhi() & 0xff);

#ifdef DEBUG_TLB
	if((pid != (vma->vm_mm->context & 0xff)) || (vma->vm_mm->context == 0)) {
		printk("update_mmu_cache: Wheee, bogus tlbpid mmpid=%d tlbpid=%d\n",
		       (int) (vma->vm_mm->context & 0xff), pid);
	}
#endif

	save_flags(flags); cli();
	address &= (PAGE_MASK << 1);
	set_entryhi(address | (pid));
	pgdp = pgd_offset(vma->vm_mm, address);
	BARRIER;
	tlb_probe();
	BARRIER;
	pmdp = pmd_offset(pgdp, address);
	idx = get_index();
	ptep = pte_offset(pmdp, address);
	BARRIER;
	set_entrylo0(pte_val(*ptep++) >> 6);
	set_entrylo1(pte_val(*ptep) >> 6);
	set_entryhi(address | (pid));
	BARRIER;
	if(idx < 0) {
		tlb_write_random();
#if 0
		BARRIER;
		printk("[MISS]");
#endif
	} else {
		tlb_write_indexed();
#if 0
		BARRIER;
		printk("[HIT]");
#endif
	}
#if 0
	if(!strcmp(current->comm, "args")) {
		printk("<");
		for(idx = 0; idx < NTLB_ENTRIES; idx++) {
			BARRIER;
			set_index(idx);	BARRIER;
			tlb_read(); BARRIER;
			address = get_entryhi(); BARRIER;
			if((address & 0xff) != 0)
				printk("[%08lx]", address);
		}
		printk(">");
	}
	BARRIER;
#endif
	BARRIER;
	set_entryhi(pid);
	BARRIER;
	restore_flags(flags);
}

#if 0
static void r4k_update_mmu_cache_hwbug(struct vm_area_struct * vma,
				       unsigned long address, pte_t pte)
{
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int idx;

	save_flags(flags); cli();
	address &= (PAGE_MASK << 1);
	set_entryhi(address | (get_entryhi() & 0xff));
	pgdp = pgd_offset(vma->vm_mm, address);
	tlb_probe();
	pmdp = pmd_offset(pgdp, address);
	idx = get_index();
	ptep = pte_offset(pmdp, address);
	set_entrylo0(pte_val(*ptep++) >> 6);
	set_entrylo1(pte_val(*ptep) >> 6);
	BARRIER;
	if(idx < 0)
		tlb_write_random();
	else
		tlb_write_indexed();
	BARRIER;
	restore_flags(flags);
}
#endif

static void r4k_show_regs(struct pt_regs * regs)
{
	/* Saved main processor registers. */
	printk("$0 : %08lx %08lx %08lx %08lx\n",
	       0UL, regs->regs[1], regs->regs[2], regs->regs[3]);
	printk("$4 : %08lx %08lx %08lx %08lx\n",
               regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
	printk("$8 : %08lx %08lx %08lx %08lx\n",
	       regs->regs[8], regs->regs[9], regs->regs[10], regs->regs[11]);
	printk("$12: %08lx %08lx %08lx %08lx\n",
               regs->regs[12], regs->regs[13], regs->regs[14], regs->regs[15]);
	printk("$16: %08lx %08lx %08lx %08lx\n",
	       regs->regs[16], regs->regs[17], regs->regs[18], regs->regs[19]);
	printk("$20: %08lx %08lx %08lx %08lx\n",
               regs->regs[20], regs->regs[21], regs->regs[22], regs->regs[23]);
	printk("$24: %08lx %08lx\n",
	       regs->regs[24], regs->regs[25]);
	printk("$28: %08lx %08lx %08lx %08lx\n",
	       regs->regs[28], regs->regs[29], regs->regs[30], regs->regs[31]);

	/* Saved cp0 registers. */
	printk("epc   : %08lx\nStatus: %08lx\nCause : %08lx\n",
	       regs->cp0_epc, regs->cp0_status, regs->cp0_cause);
}

/* Detect and size the various r4k caches. */
static void probe_icache(unsigned long config)
{
	unsigned long tmp;

	tmp = (config >> 9) & 7;
	icache_size = (1 << (12 + tmp));
	if((config >> 5) & 1)
		ic_lsize = 32;
	else
		ic_lsize = 16;

	printk("Primary ICACHE %dK (linesize %d bytes)\n",
	       (int)(icache_size >> 10), (int)ic_lsize);
}

static void probe_dcache(unsigned long config)
{
	unsigned long tmp;

	tmp = (config >> 6) & 7;
	dcache_size = (1 << (12 + tmp));
	if((config >> 4) & 1)
		dc_lsize = 32;
	else
		dc_lsize = 16;

	printk("Primary DCACHE %dK (linesize %d bytes)\n",
	       (int)(dcache_size >> 10), (int)dc_lsize);
}

static int probe_scache_eeprom(unsigned long config)
{
#ifdef CONFIG_SGI
	volatile unsigned int *cpu_control;
	unsigned short cmd = 0xc220;
	unsigned long data = 0;
	int i, n;

#ifdef __MIPSEB__
	cpu_control = (volatile unsigned int *) KSEG1ADDR(0x1fa00034);
#else
	cpu_control = (volatile unsigned int *) KSEG1ADDR(0x1fa00030);
#endif
#define DEASSERT(bit) (*(cpu_control) &= (~(bit)))
#define ASSERT(bit) (*(cpu_control) |= (bit))
#define DELAY  for(n = 0; n < 100000; n++) __asm__ __volatile__("")
	DEASSERT(SGIMC_EEPROM_PRE);
	DEASSERT(SGIMC_EEPROM_SDATAO);
	DEASSERT(SGIMC_EEPROM_SECLOCK);
	DEASSERT(SGIMC_EEPROM_PRE);
	DELAY;
	ASSERT(SGIMC_EEPROM_CSEL); ASSERT(SGIMC_EEPROM_SECLOCK);
	for(i = 0; i < 11; i++) {
		if(cmd & (1<<15))
			ASSERT(SGIMC_EEPROM_SDATAO);
		else
			DEASSERT(SGIMC_EEPROM_SDATAO);
		DEASSERT(SGIMC_EEPROM_SECLOCK);
		ASSERT(SGIMC_EEPROM_SECLOCK);
		cmd <<= 1;
	}
	DEASSERT(SGIMC_EEPROM_SDATAO);
	for(i = 0; i < (sizeof(unsigned short) * 8); i++) {
		unsigned int tmp;

		DEASSERT(SGIMC_EEPROM_SECLOCK);
		DELAY;
		ASSERT(SGIMC_EEPROM_SECLOCK);
		DELAY;
		data <<= 1;
		tmp = *cpu_control;
		if(tmp & SGIMC_EEPROM_SDATAI)
			data |= 1;
	}
	DEASSERT(SGIMC_EEPROM_SECLOCK);
	DEASSERT(SGIMC_EEPROM_CSEL);
	ASSERT(SGIMC_EEPROM_PRE);
	ASSERT(SGIMC_EEPROM_SECLOCK);
	data <<= PAGE_SHIFT;
	printk("R4600/R5000 SCACHE size %dK ", (int) (data >> 10));
	switch(mips_cputype) {
	case CPU_R4600:
	case CPU_R4640:
		sc_lsize = 32;
		break;

	default:
		sc_lsize = 128;
		break;
	}
	printk("linesize %d bytes\n", sc_lsize);
	scache_size = data;
	if(data) {
		unsigned long addr, tmp1, tmp2;

		/* Enable r4600/r5000 cache.  But flush it first. */
		for(addr = KSEG0; addr < (KSEG0 + dcache_size);
		    addr += dc_lsize)
			flush_dcache_line_indexed(addr);
		for(addr = KSEG0; addr < (KSEG0 + icache_size);
		    addr += ic_lsize)
			flush_icache_line_indexed(addr);
		for(addr = KSEG0; addr < (KSEG0 + scache_size);
		    addr += sc_lsize)
			flush_scache_line_indexed(addr);

		/* R5000 scache enable is in CP0 config, on R4600 variants
		 * the scache is enable by the memory mapped cache controller.
		 */
		if(mips_cputype == CPU_R5000) {
			unsigned long config;

			config = read_32bit_cp0_register(CP0_CONFIG);
			config |= 0x1000;
			write_32bit_cp0_register(CP0_CONFIG, config);
		} else {
			/* This is really cool... */
			printk("Enabling R4600 SCACHE\n");
			__asm__ __volatile__("
			.set noreorder
			.set mips3
			li	%0, 0x1
			dsll	%0, 31
			lui	%1, 0x9000
			dsll32	%1, 0
			or	%0, %1, %0
			mfc0	%2, $12
			nop; nop; nop; nop;
			li	%1, 0x80
			mtc0	%1, $12
			nop; nop; nop; nop;
			sb	$0, 0(%0)
			mtc0	$0, $12
			nop; nop; nop; nop;
			mtc0	%2, $12
			nop; nop; nop; nop;
			.set mips0
			.set reorder
		        " : "=r" (tmp1), "=r" (tmp2), "=r" (addr));
		}

		return 1;
	} else {
		if(mips_cputype == CPU_R5000)
			return -1;
		else
			return 0;
	}
#else
	/*
	 * XXX For now we don't panic and assume that existing chipset
	 * controlled caches are setup correnctly and are completly
	 * transparent.  Works fine for those MIPS machines I know.
	 * Morituri the salutant ...
	 */
	return 0;

	panic("Cannot probe SCACHE on this machine.");
#endif
}

/* If you even _breathe_ on this function, look at the gcc output
 * and make sure it does not pop things on and off the stack for
 * the cache sizing loop that executes in KSEG1 space or else
 * you will crash and burn badly.  You have been warned.
 */
static int probe_scache(unsigned long config)
{
	extern unsigned long stext;
	unsigned long flags, addr, begin, end, pow2;
	int tmp;

	tmp = ((config >> 17) & 1);
	if(tmp)
		return 0;
	tmp = ((config >> 22) & 3);
	switch(tmp) {
	case 0:
		sc_lsize = 16;
		break;
	case 1:
		sc_lsize = 32;
		break;
	case 2:
		sc_lsize = 64;
		break;
	case 3:
		sc_lsize = 128;
		break;
	}

	begin = (unsigned long) &stext;
	begin &= ~((4 * 1024 * 1024) - 1);
	end = begin + (4 * 1024 * 1024);

	/* This is such a bitch, you'd think they would make it
	 * easy to do this.  Away you daemons of stupidity!
	 */
	save_flags(flags); cli();

	/* Fill each size-multiple cache line with a valid tag. */
	pow2 = (64 * 1024);
	for(addr = begin; addr < end; addr = (begin + pow2)) {
		unsigned long *p = (unsigned long *) addr;
		__asm__ __volatile__("nop" : : "r" (*p)); /* whee... */
		pow2 <<= 1;
	}

	/* Load first line with zero (therefore invalid) tag. */
	set_taglo(0);
	set_taghi(0);
	__asm__ __volatile__("nop; nop; nop; nop;"); /* avoid the hazard */
	__asm__ __volatile__("\n\t.set noreorder\n\t"
			     ".set mips3\n\t"
			     "cache 8, (%0)\n\t"
			     ".set mips0\n\t"
			     ".set reorder\n\t" : : "r" (begin));
	__asm__ __volatile__("\n\t.set noreorder\n\t"
			     ".set mips3\n\t"
			     "cache 9, (%0)\n\t"
			     ".set mips0\n\t"
			     ".set reorder\n\t" : : "r" (begin));
	__asm__ __volatile__("\n\t.set noreorder\n\t"
			     ".set mips3\n\t"
			     "cache 11, (%0)\n\t"
			     ".set mips0\n\t"
			     ".set reorder\n\t" : : "r" (begin));

	/* Now search for the wrap around point. */
	pow2 = (128 * 1024);
	tmp = 0;
	for(addr = (begin + (128 * 1024)); addr < (end); addr = (begin + pow2)) {
		__asm__ __volatile__("\n\t.set noreorder\n\t"
				     ".set mips3\n\t"
				     "cache 7, (%0)\n\t"
				     ".set mips0\n\t"
				     ".set reorder\n\t" : : "r" (addr));
		__asm__ __volatile__("nop; nop; nop; nop;"); /* hazard... */
		if(!get_taglo())
			break;
		pow2 <<= 1;
	}
	restore_flags(flags);
	addr -= begin;
	printk("Secondary cache sized at %dK linesize %d\n", (int) (addr >> 10),
	       sc_lsize);
	scache_size = addr;
	return 1;
}

static void setup_noscache_funcs(void)
{
	switch(dc_lsize) {
	case 16:
		clear_page = r4k_clear_page_d16;
		copy_page = r4k_copy_page_d16;
		flush_cache_all = r4k_flush_cache_all_d16i16;
		flush_cache_mm = r4k_flush_cache_mm_d16i16;
		flush_cache_range = r4k_flush_cache_range_d16i16;
		flush_cache_page = r4k_flush_cache_page_d16i16;
		flush_page_to_ram = r4k_flush_page_to_ram_d16i16;
		break;
	case 32:
		clear_page = r4k_clear_page_d32;
		copy_page = r4k_copy_page_d32;
		flush_cache_all = r4k_flush_cache_all_d32i32;
		flush_cache_mm = r4k_flush_cache_mm_d32i32;
		flush_cache_range = r4k_flush_cache_range_d32i32;
		flush_cache_page = r4k_flush_cache_page_d32i32;
		flush_page_to_ram = r4k_flush_page_to_ram_d32i32;
		break;
	}
}

static void setup_scache_funcs(void)
{
	switch(sc_lsize) {
	case 16:
		switch(dc_lsize) {
		case 16:
			clear_page = r4k_clear_page_d16;
			copy_page = r4k_copy_page_d16;
			flush_cache_all = r4k_flush_cache_all_s16d16i16;
			flush_cache_mm = r4k_flush_cache_mm_s16d16i16;
			flush_cache_range = r4k_flush_cache_range_s16d16i16;
			flush_cache_page = r4k_flush_cache_page_s16d16i16;
			flush_page_to_ram = r4k_flush_page_to_ram_s16d16i16;
			break;
		case 32:
			clear_page = r4k_clear_page_d32;
			copy_page = r4k_copy_page_d32;
			flush_cache_all = r4k_flush_cache_all_s16d32i32;
			flush_cache_mm = r4k_flush_cache_mm_s16d32i32;
			flush_cache_range = r4k_flush_cache_range_s16d32i32;
			flush_cache_page = r4k_flush_cache_page_s16d32i32;
			flush_page_to_ram = r4k_flush_page_to_ram_s16d32i32;
			break;
		};
		break;
	case 32:
		switch(dc_lsize) {
		case 16:
			clear_page = r4k_clear_page_d16;
			copy_page = r4k_copy_page_d16;
			flush_cache_all = r4k_flush_cache_all_s32d16i16;
			flush_cache_mm = r4k_flush_cache_mm_s32d16i16;
			flush_cache_range = r4k_flush_cache_range_s32d16i16;
			flush_cache_page = r4k_flush_cache_page_s32d16i16;
			flush_page_to_ram = r4k_flush_page_to_ram_s32d16i16;
			break;
		case 32:
			clear_page = r4k_clear_page_d32;
			copy_page = r4k_copy_page_d32;
			flush_cache_all = r4k_flush_cache_all_s32d32i32;
			flush_cache_mm = r4k_flush_cache_mm_s32d32i32;
			flush_cache_range = r4k_flush_cache_range_s32d32i32;
			flush_cache_page = r4k_flush_cache_page_s32d32i32;
			flush_page_to_ram = r4k_flush_page_to_ram_s32d32i32;
			break;
		};
	case 64:
		switch(dc_lsize) {
		case 16:
			clear_page = r4k_clear_page_d16;
			copy_page = r4k_copy_page_d16;
			flush_cache_all = r4k_flush_cache_all_s64d16i16;
			flush_cache_mm = r4k_flush_cache_mm_s64d16i16;
			flush_cache_range = r4k_flush_cache_range_s64d16i16;
			flush_cache_page = r4k_flush_cache_page_s64d16i16;
			flush_page_to_ram = r4k_flush_page_to_ram_s64d16i16;
			break;
		case 32:
			clear_page = r4k_clear_page_d32;
			copy_page = r4k_copy_page_d32;
			flush_cache_all = r4k_flush_cache_all_s64d32i32;
			flush_cache_mm = r4k_flush_cache_mm_s64d32i32;
			flush_cache_range = r4k_flush_cache_range_s64d32i32;
			flush_cache_page = r4k_flush_cache_page_s64d32i32;
			flush_page_to_ram = r4k_flush_page_to_ram_s64d32i32;
			break;
		};
	case 128:
		switch(dc_lsize) {
		case 16:
			clear_page = r4k_clear_page_d16;
			copy_page = r4k_copy_page_d16;
			flush_cache_all = r4k_flush_cache_all_s128d16i16;
			flush_cache_mm = r4k_flush_cache_mm_s128d16i16;
			flush_cache_range = r4k_flush_cache_range_s128d16i16;
			flush_cache_page = r4k_flush_cache_page_s128d16i16;
			flush_page_to_ram = r4k_flush_page_to_ram_s128d16i16;
			break;
		case 32:
			clear_page = r4k_clear_page_d32;
			copy_page = r4k_copy_page_d32;
			flush_cache_all = r4k_flush_cache_all_s128d32i32;
			flush_cache_mm = r4k_flush_cache_mm_s128d32i32;
			flush_cache_range = r4k_flush_cache_range_s128d32i32;
			flush_cache_page = r4k_flush_cache_page_s128d32i32;
			flush_page_to_ram = r4k_flush_page_to_ram_s128d32i32;
			break;
		};
		break;
	}
}

typedef int (*probe_func_t)(unsigned long);
static probe_func_t probe_scache_kseg1;

void ld_mmu_r4xx0(void)
{
	unsigned long cfg = read_32bit_cp0_register(CP0_CONFIG);
	int sc_present = 0;

	printk("CPU REVISION IS: %08x\n", read_32bit_cp0_register(CP0_PRID));

	probe_icache(cfg);
	probe_dcache(cfg);

	switch(mips_cputype) {
	case CPU_R4000PC:
	case CPU_R4000SC:
	case CPU_R4000MC:
	case CPU_R4400PC:
	case CPU_R4400SC:
	case CPU_R4400MC:
try_again:
		probe_scache_kseg1 = (probe_func_t) (KSEG1ADDR(&probe_scache));
		sc_present = probe_scache_kseg1(cfg);
		break;

	case CPU_R4600:
	case CPU_R4640:
	case CPU_R4700:
	case CPU_R5000:
		probe_scache_kseg1 = (probe_func_t)
			(KSEG1ADDR(&probe_scache_eeprom));
		sc_present = probe_scache_eeprom(cfg);

		/* Try using tags if eeprom give us bogus data. */
		if(sc_present == -1)
			goto try_again;
		break;
	};

	if(!sc_present) {
		/* Lacks secondary cache. */
		setup_noscache_funcs();
	} else {
		/* Has a secondary cache. */
		if(mips_cputype != CPU_R4600 &&
		   mips_cputype != CPU_R4640 &&
		   mips_cputype != CPU_R4700 &&
		   mips_cputype != CPU_R5000) {
			setup_scache_funcs();
		} else {
			setup_noscache_funcs();
			if((mips_cputype != CPU_R5000)) {
				flush_cache_page =
					r4k_flush_cache_page_d32i32_r4600;
				flush_page_to_ram =
					r4k_flush_page_to_ram_d32i32_r4600;
			}
		}
	}

	flush_cache_sigtramp = r4k_flush_cache_sigtramp;

	flush_tlb_all = r4k_flush_tlb_all;
	flush_tlb_mm = r4k_flush_tlb_mm;
	flush_tlb_range = r4k_flush_tlb_range;
	flush_tlb_page = r4k_flush_tlb_page;

	load_pgd = r4k_load_pgd;
	pgd_init = r4k_pgd_init;
	update_mmu_cache = r4k_update_mmu_cache;

	show_regs = r4k_show_regs;

	flush_cache_all();
	write_32bit_cp0_register(CP0_WIRED, 0);
	write_32bit_cp0_register(CP0_PAGEMASK, PM_4K);
	flush_tlb_all();
}
