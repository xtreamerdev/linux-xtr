/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 2003 by Ralf Baechle
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/cacheflush.h>
#include <asm/processor.h>
#include <asm/cpu.h>
#include <asm/cpu-features.h>
#include <asm/r4kcache.h>

/* Cache operations. */
void (*flush_cache_all)(void);
void (*__flush_cache_all)(void);
void (*flush_cache_mm)(struct mm_struct *mm);
void (*flush_cache_range)(struct vm_area_struct *vma, unsigned long start,
	unsigned long end);
void (*flush_cache_page)(struct vm_area_struct *vma, unsigned long page,
	unsigned long pfn);
void (*flush_icache_range)(unsigned long __user start,
	unsigned long __user end);
void (*flush_icache_page)(struct vm_area_struct *vma, struct page *page);

/* MIPS specific cache operations */
void (*flush_cache_sigtramp)(unsigned long addr);
void (*flush_data_cache_page)(unsigned long addr);
void (*flush_icache_all)(void);

EXPORT_SYMBOL(flush_data_cache_page);

#ifdef CONFIG_DMA_NONCOHERENT

/* DMA cache operations. */
void (*_dma_cache_wback_inv)(unsigned long start, unsigned long size);
void (*_dma_cache_wback)(unsigned long start, unsigned long size);
void (*_dma_cache_inv)(unsigned long start, unsigned long size);

EXPORT_SYMBOL(_dma_cache_wback_inv);
EXPORT_SYMBOL(_dma_cache_wback);
EXPORT_SYMBOL(_dma_cache_inv);

#endif /* CONFIG_DMA_NONCOHERENT */

/*
 * We could optimize the case where the cache argument is not BCACHE but
 * that seems very atypical use ...
 */
asmlinkage int sys_cacheflush(unsigned long __user addr,
	unsigned long bytes, unsigned int cache)
{
	if (!access_ok(VERIFY_WRITE, (void __user *) addr, bytes))
		return -EFAULT;

	flush_icache_range(addr, addr + bytes);

	return 0;
}

void __flush_dcache_page(struct page *page)
{
	struct address_space *mapping = page_mapping(page);
	unsigned long addr;

	if (mapping && !mapping_mapped(mapping)) {
		SetPageDcacheDirty(page);
		return;
	}

	/*
	 * We could delay the flush for the !page_mapping case too.  But that
	 * case is for exec env/arg pages and those are %99 certainly going to
	 * get faulted into the tlb (and thus flushed) anyways.
	 */
	addr = (unsigned long) page_address(page);
	flush_data_cache_page(addr);
}

EXPORT_SYMBOL(__flush_dcache_page);

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS

#define ENTER_CRITICAL(flags) local_irq_save(flags)
#define EXIT_CRITICAL(flags) local_irq_restore(flags)

void *kmap_coherent(struct page *page, unsigned long *flags)
{
	enum fixed_addresses idx = 0;
	unsigned long vaddr, entrylo;
	unsigned long old_ctx;
	pte_t pte;
	int tlbidx;

/* todo...
	BUG_ON(Page_dcache_dirty(page));
*/

	vaddr = __fix_to_virt(FIX_CMAP_END - idx);
	pte = mk_pte(page, PAGE_KERNEL);
	entrylo = pte_val(pte) >> 6;

	ENTER_CRITICAL(*flags);
	old_ctx = read_c0_entryhi();
	write_c0_entryhi(vaddr & (PAGE_MASK << 1));
	write_c0_entrylo0(entrylo);
	write_c0_entrylo1(entrylo);

	tlbidx = read_c0_wired();
	write_c0_wired(tlbidx + 1);
	write_c0_index(tlbidx);
	mtc0_tlbw_hazard();
	tlb_write_indexed();
	tlbw_use_hazard();
	write_c0_entryhi(old_ctx);
	
	return (void*) vaddr;
}

#define UNIQUE_ENTRYHI(idx) (CKSEG0 + ((idx) << (PAGE_SHIFT + 1)))

void kunmap_coherent(unsigned long *flags)
{
	unsigned int wired;
	unsigned long old_ctx;

	old_ctx = read_c0_entryhi();
	wired = read_c0_wired() - 1;
	write_c0_wired(wired);
	write_c0_index(wired);
	write_c0_entryhi(UNIQUE_ENTRYHI(wired));
	write_c0_entrylo0(0);
	write_c0_entrylo1(0);
	mtc0_tlbw_hazard();
	tlb_write_indexed();
	tlbw_use_hazard();
	write_c0_entryhi(old_ctx);
	EXIT_CRITICAL(*flags);

	preempt_check_resched();
}

void flush_dcache_page_alias(struct page *page)
{
//	struct address_space *mapping = page_mapping(page);
	unsigned long addr, flags;
	
	if (!cpu_has_dc_aliases)
		return;

/* todo...
	if (mapping && !mapping_mapped(mapping)) {
		SetPageDcacheDirty(page);
		return;
	}
*/

	// we use fix-mapped area to do the flush...
	addr = (unsigned long)kmap_coherent(page, &flags);
	blast_dcache32_page(addr);
	blast_dcache32_page(addr+0x1000);
	kunmap_coherent(&flags);
}

EXPORT_SYMBOL(flush_dcache_page_alias);

#endif

void __update_cache(struct vm_area_struct *vma, unsigned long address,
	pte_t pte)
{
	struct page *page;
	unsigned long pfn, addr;

	pfn = pte_pfn(pte);
	if (pfn_valid(pfn) && (page = pfn_to_page(pfn), page_mapping(page)) &&
	    Page_dcache_dirty(page)) {
		if (pages_do_alias((unsigned long)page_address(page),
		                   address & PAGE_MASK)) {
			addr = (unsigned long) page_address(page);
			flush_data_cache_page(addr);
		}

		ClearPageDcacheDirty(page);
	}
}

extern void ld_mmu_r23000(void);
extern void ld_mmu_r4xx0(void);
extern void ld_mmu_tx39(void);
extern void ld_mmu_r6000(void);
extern void ld_mmu_tfp(void);
extern void ld_mmu_andes(void);
extern void ld_mmu_sb1(void);

void __init cpu_cache_init(void)
{
	if (cpu_has_4ktlb) {
#if defined(CONFIG_CPU_R4X00)  || defined(CONFIG_CPU_VR41XX) || \
    defined(CONFIG_CPU_R4300)  || defined(CONFIG_CPU_R5000)  || \
    defined(CONFIG_CPU_NEVADA) || defined(CONFIG_CPU_R5432)  || \
    defined(CONFIG_CPU_R5500)  || defined(CONFIG_CPU_MIPS32) || \
    defined(CONFIG_CPU_MIPS64) || defined(CONFIG_CPU_TX49XX) || \
    defined(CONFIG_CPU_RM7000) || defined(CONFIG_CPU_RM9000)
		ld_mmu_r4xx0();
#endif
	} else switch (current_cpu_data.cputype) {
#ifdef CONFIG_CPU_R3000
	case CPU_R2000:
	case CPU_R3000:
	case CPU_R3000A:
	case CPU_R3081E:
		ld_mmu_r23000();
		break;
#endif
#ifdef CONFIG_CPU_TX39XX
	case CPU_TX3912:
	case CPU_TX3922:
	case CPU_TX3927:
		ld_mmu_tx39();
		break;
#endif
#ifdef CONFIG_CPU_R10000
	case CPU_R10000:
	case CPU_R12000:
		ld_mmu_r4xx0();
		break;
#endif
#ifdef CONFIG_CPU_SB1
	case CPU_SB1:
		ld_mmu_sb1();
		break;
#endif

	case CPU_R8000:
		panic("R8000 is unsupported");
		break;

	default:
		panic("Yeee, unsupported cache architecture.");
	}
}
