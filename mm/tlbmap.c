#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/auth.h>
#include <asm/tlbflush.h>


#define NEW_VERSION

#ifdef DEBUG_MSG
	#include <linux/smp.h>
	#define cpu_context(cpu, mm)	((mm)->context[cpu])
#endif


#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
                                     "nop; nop; nop; nop; nop; nop;\n\t" \
                                     ".set reorder\n\t")

#define ASID_MASK	0x000000ff
#define PAGE_FRAME_1	0x00000000
#define PAGE_FRAME_2	0xe0000000

unsigned long		dvr_asid = 0xffffffff;
unsigned long		dvr_task = 0;

#ifdef DEBUG_MSG
void my_print_page_entry(unsigned long addr)
{
	pgd_t	*pgd;
	pud_t	*pud;
	pmd_t	*pmd;
	pte_t	*pte;

	pgd = pgd_offset(current->mm, addr);
	pud = pud_offset(pgd, addr);
	pmd = pmd_offset(pud, addr);
	pte = pte_offset(pmd, addr);
	if ((int)(pte->pte) == 0)
		printk(" address: 0x%x has no value...\n", addr);
//	printk("address: 0x%x, value: 0x%x \n", addr, (int)pte->pte);
}
#endif

void my_print_tlb_content(void)
{
	unsigned long flags;
	unsigned long old_ctx;
	unsigned long entryhi;
	unsigned long pagemask;
	int entry;
	
	local_irq_save(flags);
	old_ctx = read_c0_entryhi() & ASID_MASK;
	
	entry = 0;
	while (entry < current_cpu_data.tlbsize) {
		write_c0_index(entry);
		BARRIER;
		tlb_read();
		entryhi = read_c0_entryhi();
		pagemask = read_c0_pagemask();
		printk(" entry %x value 0x%x \tpagemask 0x%x \n", entry, (int)entryhi, (int)pagemask);
		entry++;
	}
	
	write_c0_entryhi(old_ctx);
	local_irq_restore(flags);
}

//static void my_local_flush_tlb_all(void)
//{
//#ifdef NEW_VERSION
//	unsigned long flags;
//	unsigned long old_ctx;
//	int entry;
//
//	local_irq_save(flags);
//	/* Save old context and create impossible VPN2 value */
//	old_ctx = read_c0_entryhi();
//	write_c0_entrylo0(0);
//	write_c0_entrylo1(0);
//
//	entry = read_c0_wired();
//
//	/* Blast 'em all away. */
//	while (entry < current_cpu_data.tlbsize) {
//		/*
//		 * Make sure all entries differ.  If they're not different
//		 * MIPS32 will take revenge ...
//		 */
//		write_c0_entryhi(CKSEG0 + (entry << (PAGE_SHIFT + 1)));
//		write_c0_index(entry);
//		mtc0_tlbw_hazard();
//		tlb_write_indexed();
//		entry++;
//	}
//	tlbw_use_hazard();
//	write_c0_entryhi(old_ctx);
//	local_irq_restore(flags);
//#else
//	unsigned long flags;
//	unsigned long old_ctx;
//	int entry;
//	
//	local_irq_save(flags);
//	/* Save old context and create impossible VPN2 value */
//	old_ctx = read_c0_entryhi() & ASID_MASK;
//	write_c0_entrylo0(0);
//	write_c0_entrylo1(0);
//	BARRIER;
//	
//	entry = read_c0_wired();
//	
//	/* Blast 'em all away. */
//	while (entry < current_cpu_data.tlbsize) {
//		/*
//		 * Make sure all entries differ.  If they're not different
//		 * MIPS32 will take revenge ...
//		 */
//		write_c0_entryhi(CKSEG0 + (entry << (PAGE_SHIFT + 1)));
//		write_c0_index(entry);
//		BARRIER;
//		tlb_write_indexed();
//		BARRIER;
//		entry++;
//	}
//	BARRIER;
//	write_c0_entryhi(old_ctx);
//	local_irq_restore(flags);
//#endif // NEW_VERSION
//}

static void my_add_wired_entry(unsigned long entrylo0, unsigned long entrylo1,
         unsigned long entryhi, unsigned long pagemask)
{
#ifdef NEW_VERSION
	unsigned long flags;
	unsigned long wired;
	unsigned long old_pagemask;
	unsigned long old_ctx;

	local_irq_save(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = read_c0_entryhi();
	old_pagemask = read_c0_pagemask();
	wired = read_c0_wired();
	if (wired == 0) {
		write_c0_wired(wired + 1);
		write_c0_index(wired);
	} else {
		write_c0_index(wired - 1);
	}
	BARRIER;
	write_c0_pagemask(pagemask);
	write_c0_entryhi(entryhi);
	write_c0_entrylo0(entrylo0);
	write_c0_entrylo1(entrylo1);
	mtc0_tlbw_hazard();
	tlb_write_indexed();
	tlbw_use_hazard();

#ifdef DEBUG_MSG
	printk(" original pagemask value: 0x%x\n", (int)pagemask);
	pagemask = read_c0_pagemask();
	printk(" written  pagemask value: 0x%x\n", (int)pagemask);
#endif

	write_c0_entryhi(old_ctx);
	BARRIER;
	write_c0_pagemask(old_pagemask);
	local_flush_tlb_all();
	local_irq_restore(flags);
#else
	unsigned long flags;
	unsigned long wired;
	unsigned long old_pagemask;
	unsigned long old_ctx;
	
	local_irq_save(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = read_c0_entryhi() & ASID_MASK;
	old_pagemask = read_c0_pagemask();
	wired = read_c0_wired();
	write_c0_wired(wired + 1);
	write_c0_index(wired);
	BARRIER;
	/* This line may cause exception if this file is built in the form of module */
	write_c0_pagemask(pagemask);
	write_c0_entryhi(entryhi);
	write_c0_entrylo0(entrylo0);
	write_c0_entrylo1(entrylo1);
	BARRIER;
	tlb_write_indexed();
	BARRIER;

#ifdef DEBUG_MSG
	printk(" original pagemask value: 0x%x\n", (int)pagemask);
	pagemask = read_c0_pagemask();
	printk(" written  pagemask value: 0x%x\n", (int)pagemask);
#endif

	write_c0_entryhi(old_ctx);
	BARRIER;
	write_c0_pagemask(old_pagemask);
	local_flush_tlb_all();
	local_irq_restore(flags);
#endif // NEW_VERSION
}

static void my_clr_wired_entry()
{
	unsigned long flags;

	local_irq_save(flags);
	write_c0_wired(0);
	BARRIER;
	local_flush_tlb_all();
	local_irq_restore(flags);
}

void my_tlb_init(void)
{
	printk(KERN_ALERT "   Hello, Realtek TLB Mapper\n");
}

void my_tlb_exit(void)
{
	printk(KERN_ALERT "   Goodbye, Realtek TLB Mapper\n");
}
/*
static inline pgprot_t new_pgprot_noncached_dvr(pgprot_t _prot)
{
        unsigned long prot = pgprot_val(_prot);

        prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;
        prot = prot | _PAGE_WRITE | _PAGE_FILE | _PAGE_VALID | _PAGE_DIRTY;

        return __pgprot(prot);
}
*/
static inline pgprot_t pgprot_noncached_dvr(pgprot_t _prot)
{
        unsigned long prot = pgprot_val(_prot);

        prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;
        prot = prot | _PAGE_WRITE | _PAGE_FILE | _PAGE_VALID | _PAGE_DIRTY;
        prot &= ~_PAGE_PRESENT;

        return __pgprot(prot);
}

unsigned long tlb_mmap(unsigned long addr)
{
	struct vm_area_struct *dvr_area;
	unsigned long startaddr;
	unsigned long entrylo0;
	unsigned long entrylo1;
	unsigned long entryhi;
	unsigned long pagemask;
#ifdef DEBUG_MSG
	unsigned int cpu = smp_processor_id();
	unsigned long i, j;
#endif

	while ((dvr_asid != 0xffffffff) || (dvr_task != 0)) {
		msleep(100);
	}
#ifdef CONFIG_REALTEK_MAP_IO_REGISTERS
	startaddr = do_mmap(NULL, addr, DEF_MAP_SIZE+0x20000, PROT_READ | PROT_WRITE, 
		MAP_FIXED | MAP_PRIVATE | MAP_NORESERVE | MAP_ANONYMOUS, 0);
#else
	startaddr = do_mmap(NULL, addr, DEF_MAP_SIZE, PROT_READ | PROT_WRITE, 
		MAP_FIXED | MAP_PRIVATE | MAP_NORESERVE | MAP_ANONYMOUS, 0);
#endif

	if (startaddr & 0x80000000)
		goto out;

#ifdef DEBUG_MSG
	printk(" get context value: 0x%x\n", (int)cpu_context(cpu, current->mm));
	printk(" get entryhi value: 0x%x\n", (int)read_c0_entryhi());
	printk(" get prid    value: 0x%x\n", (int)read_c0_prid());
#endif

	entrylo0 = (PAGE_FRAME_1 >> 6) | 0x1e;
	entrylo1 = (PAGE_FRAME_2 >> 6) | 0x16;
	entryhi = (startaddr & VPN2_MASK) | (read_c0_entryhi() & ASID_MASK);
	pagemask = PM_64M;

#ifdef DEBUG_MSG
	printk("  entrylo0	value: 0x%x\n", (int)entrylo0);
	printk("  entrylo1	value: 0x%x\n", (int)entrylo1);
	printk("  entryhi	value: 0x%x\n", (int)entryhi);
	printk("  pagemask	value: 0x%x\n", (int)pagemask);
	my_print_tlb_content();
#endif

	dvr_asid = read_c0_entryhi() & ASID_MASK;
	dvr_task = (unsigned long)current;
	my_add_wired_entry(entrylo0, entrylo1, entryhi, pagemask);

#ifdef DEBUG_MSG
	printk(" dvr_asid: 0x%x\n", (int)dvr_asid);
	printk(" get wired value: 0x%x\n", (int)read_c0_wired());
	my_print_tlb_content();
#endif

	dvr_area = find_vma(current->mm, addr);

//	dvr_area->vm_page_prot = new_pgprot_noncached_dvr(dvr_area->vm_page_prot);
//	printk(" vm_page_prot: 0x%x\n", (int)dvr_area->vm_page_prot.pgprot);

//	printk(" vm_flags: 0x%x\n", (int)dvr_area->vm_flags);
	dvr_area->vm_page_prot = pgprot_noncached_dvr(dvr_area->vm_page_prot);
//	if (remap_pfn_range(dvr_area, addr, 0, DEF_MEM_SIZE, dvr_area->vm_page_prot))
//		printk("error in 1st remap_page_range()...\n");
	if (remap_pfn_range(dvr_area, addr+DEF_MEM_SIZE*2, 0, DEF_MEM_SIZE, dvr_area->vm_page_prot))
		printk("error in 2nd remap_page_range()...\n");
#ifdef CONFIG_REALTEK_MAP_IO_REGISTERS
	if (remap_pfn_range(dvr_area, addr+DEF_MEM_SIZE*3, 0x18000, 0x20000, dvr_area->vm_page_prot))
		printk("error in 3nd remap_page_range()...\n");
#endif
//	printk(" vm_flags: 0x%x\n", (int)dvr_area->vm_flags);
	dvr_area->vm_flags = (dvr_area->vm_flags | VM_DVR);
	dvr_area->vm_flags = (dvr_area->vm_flags & ~VM_IO); 
//	printk(" vm_flags: 0x%x\n", (int)dvr_area->vm_flags);
#ifdef DEBUG_MSG
	printk(" uncached address: 0x%x\n", (int)addr+DEF_MEM_SIZE*2);
	printk(" vm_page_prot: 0x%x\n", (int)dvr_area->vm_page_prot.pgprot);
	i = addr+DEF_MEM_SIZE*2;
	j = addr+DEF_MEM_SIZE*3;
	while (i < j) {
		my_print_page_entry((unsigned long)i);
		i += 0x1000;
	}
#endif

out:
	return startaddr;
}

unsigned long tlb_munmap(unsigned long addr)
{
	int ret;

	printk("clear pli setting....\n");
	my_clr_wired_entry();
#ifdef CONFIG_REALTEK_MAP_IO_REGISTERS
	ret = do_munmap(current->mm, addr, DEF_MAP_SIZE+0x20000);
#else
	ret = do_munmap(current->mm, addr, DEF_MAP_SIZE);
#endif
	dvr_asid = 0xffffffff;
	dvr_task = 0;

	return 0;
}

