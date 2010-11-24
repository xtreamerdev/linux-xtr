/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000  Ani Joshi <ajoshi@unixbox.com>
 * Copyright (C) 2000, 2001  Ralf Baechle <ralf@gnu.org>
 * swiped from i386, and cloned for MIPS by Geert, polished by Ralf.
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>

#include <asm/cache.h>
#include <asm/io.h>
#include <asm/highmem.h>

/*
 * Warning on the terminology - Linux calls an uncached area coherent;
 * MIPS terminology calls memory areas with hardware maintained coherency
 * coherent.
 */

void *dma_alloc_noncoherent(struct device *dev, size_t size,
	dma_addr_t * dma_handle, unsigned int __nocast gfp)
{
	void *ret;
	/* ignore region specifiers */
	gfp &= ~(__GFP_DMA | __GFP_HIGHMEM);

	if (dev == NULL || (dev->coherent_dma_mask < 0xffffffff))
		gfp |= GFP_DMA;
	ret = (void *) __get_free_pages(gfp, get_order(size));

	if (ret != NULL) {
		memset(ret, 0, size);
		*dma_handle = virt_to_phys(ret);
	}

	return ret;
}

EXPORT_SYMBOL(dma_alloc_noncoherent);

void *dma_alloc_coherent(struct device *dev, size_t size,
	dma_addr_t * dma_handle, unsigned int __nocast gfp)
{
	void *ret;

	ret = dma_alloc_noncoherent(dev, size, dma_handle, gfp);
	if (ret) {
		dma_cache_wback_inv((unsigned long) ret, size);
		ret = UNCAC_ADDR(ret);
	}

	return ret;
}

EXPORT_SYMBOL(dma_alloc_coherent);

void dma_free_noncoherent(struct device *dev, size_t size, void *vaddr,
	dma_addr_t dma_handle)
{
	free_pages((unsigned long) vaddr, get_order(size));
}

EXPORT_SYMBOL(dma_free_noncoherent);

void dma_free_coherent(struct device *dev, size_t size, void *vaddr,
	dma_addr_t dma_handle)
{
	unsigned long addr = (unsigned long) vaddr;

	addr = CAC_ADDR(addr);
	free_pages(addr, get_order(size));
}

EXPORT_SYMBOL(dma_free_coherent);

static inline void __dma_sync(unsigned long addr, size_t size,
	enum dma_data_direction direction)
{
#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	struct page *page = NULL;
	unsigned long addr1, addr2, flags;

	while ((addr & PAGE_MASK) != ((addr+size-1) & PAGE_MASK)) {
		int remains;

		remains = PAGE_SIZE-(addr & (PAGE_SIZE-1));
		page = virt_to_page(addr);
		addr1 = (unsigned long)kmap_coherent(page, &flags)+(addr & (PAGE_SIZE-1));
		addr2 = addr1+0x1000;
//		printk(" *** 3 page: %x, addr: %x, size: %x, remains: %x \n", page, addr, size, remains);
		
		switch (direction) {
		case DMA_TO_DEVICE:
			dma_cache_wback(addr1, remains);
			dma_cache_wback(addr2, remains);
			break;
	
		case DMA_FROM_DEVICE:
			dma_cache_inv(addr1, remains);
			dma_cache_inv(addr2, remains);
			break;
	
		case DMA_BIDIRECTIONAL:
			dma_cache_wback_inv(addr1, remains);
			dma_cache_wback_inv(addr2, remains);
			break;
	
		default:
			BUG();
		}

		kunmap_coherent(&flags);
		addr += remains;
		size -= remains;
	}

	page = virt_to_page(addr);
	addr1 = (unsigned long)kmap_coherent(page, &flags)+(addr & (PAGE_SIZE-1));
	addr2 = addr1+0x1000;
//	printk(" *** 1 page: %x, addr: %x, size: %x \n", page, addr, size);

	switch (direction) {
	case DMA_TO_DEVICE:
		dma_cache_wback(addr1, size);
		dma_cache_wback(addr2, size);
		break;

	case DMA_FROM_DEVICE:
		dma_cache_inv(addr1, size);
		dma_cache_inv(addr2, size);
		break;

	case DMA_BIDIRECTIONAL:
		dma_cache_wback_inv(addr1, size);
		dma_cache_wback_inv(addr2, size);
		break;

	default:
		BUG();
	}

	kunmap_coherent(&flags);
#else
	switch (direction) {
	case DMA_TO_DEVICE:
		dma_cache_wback(addr, size);
		break;

	case DMA_FROM_DEVICE:
		dma_cache_inv(addr, size);
		break;

	case DMA_BIDIRECTIONAL:
		dma_cache_wback_inv(addr, size);
		break;

	default:
		BUG();
	}
#endif
}

dma_addr_t dma_map_single(struct device *dev, void *ptr, size_t size,
	enum dma_data_direction direction)
{
	unsigned long addr = (unsigned long) ptr;

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	struct page *page = NULL;
	unsigned long addr1, addr2, flags;

	while ((addr & PAGE_MASK) != ((addr+size-1) & PAGE_MASK)) {
		int remains;
	
		remains = PAGE_SIZE-(addr & (PAGE_SIZE-1));
		page = virt_to_page(addr);
		addr1 = (unsigned long)kmap_coherent(page, &flags)+(addr & (PAGE_SIZE-1));
		addr2 = addr1+0x1000;
//		printk(" *** 4 page: %x, addr: %x, size: %x, remains: %x \n", page, addr, size, remains);
	
		switch (direction) {
			case DMA_TO_DEVICE:
				dma_cache_wback(addr1, remains);
				dma_cache_wback(addr2, remains);
				break;
	
			case DMA_FROM_DEVICE:
				dma_cache_inv(addr1, remains);
				dma_cache_inv(addr2, remains);
				break;
	
			case DMA_BIDIRECTIONAL:
				dma_cache_wback_inv(addr1, remains);
				dma_cache_wback_inv(addr2, remains);
				break;
	
			default:
				BUG();
		}
	
		kunmap_coherent(&flags);
		addr += remains;
		size -= remains;
	}

	page = virt_to_page(addr);
	addr1 = (unsigned long)kmap_coherent(page, &flags)+(addr & (PAGE_SIZE-1));
	addr2 = addr1+0x1000;
//	printk(" *** 2 page: %x, addr: %x, size: %x \n", page, addr, size);

	switch (direction) {
	case DMA_TO_DEVICE:
		dma_cache_wback(addr1, size);
		dma_cache_wback(addr2, size);
		break;

	case DMA_FROM_DEVICE:
		dma_cache_inv(addr1, size);
		dma_cache_inv(addr2, size);
		break;

	case DMA_BIDIRECTIONAL:
		dma_cache_wback_inv(addr1, size);
		dma_cache_wback_inv(addr2, size);
		break;

	default:
		BUG();
	}

	kunmap_coherent(&flags);
#else
	switch (direction) {
	case DMA_TO_DEVICE:
		dma_cache_wback(addr, size);
		break;

	case DMA_FROM_DEVICE:
		dma_cache_inv(addr, size);
		break;

	case DMA_BIDIRECTIONAL:
		dma_cache_wback_inv(addr, size);
		break;

	default:
		BUG();
	}
#endif

	return virt_to_phys(ptr);
}

EXPORT_SYMBOL(dma_map_single);

void dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size,
	enum dma_data_direction direction)
{
	unsigned long addr;
	addr = dma_addr + PAGE_OFFSET;

	switch (direction) {
	case DMA_TO_DEVICE:
		//dma_cache_wback(addr, size);
		break;

	case DMA_FROM_DEVICE:
		//dma_cache_inv(addr, size);
		break;

	case DMA_BIDIRECTIONAL:
		//dma_cache_wback_inv(addr, size);
		break;

	default:
		BUG();
	}
}

EXPORT_SYMBOL(dma_unmap_single);

int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	for (i = 0; i < nents; i++, sg++) {
		unsigned long addr;

		addr = (unsigned long) page_address(sg->page);
		if (addr)
			__dma_sync(addr + sg->offset, sg->length, direction);
		sg->dma_address = (dma_addr_t)
			(page_to_phys(sg->page) + sg->offset);
	}

	return nents;
}

EXPORT_SYMBOL(dma_map_sg);

dma_addr_t dma_map_page(struct device *dev, struct page *page,
	unsigned long offset, size_t size, enum dma_data_direction direction)
{
#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	unsigned long addr1, addr2, flags;

	addr1 = (unsigned long)kmap_coherent(page, &flags)+offset;
	addr2 = addr1+0x1000;
	BUG_ON((addr1 & PAGE_MASK) != ((addr1+size-1) & PAGE_MASK));

	dma_cache_wback_inv(addr1, size);
	dma_cache_wback_inv(addr2, size);

	kunmap_coherent(&flags);
#else
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long) page_address(page) + offset;
	dma_cache_wback_inv(addr, size);
#endif

	return page_to_phys(page) + offset;
}

EXPORT_SYMBOL(dma_map_page);

void dma_unmap_page(struct device *dev, dma_addr_t dma_address, size_t size,
	enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);

	if (direction != DMA_TO_DEVICE) {
#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
		/* EJ: i think user should not access these memory during the DMA, so... */
#endif
		unsigned long addr;

		addr = dma_address + PAGE_OFFSET;
		dma_cache_wback_inv(addr, size);
	}
}

EXPORT_SYMBOL(dma_unmap_page);

void dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nhwentries,
	enum dma_data_direction direction)
{
	unsigned long addr;
	int i;

	BUG_ON(direction == DMA_NONE);

	if (direction == DMA_TO_DEVICE)
		return;

	for (i = 0; i < nhwentries; i++, sg++) {
		addr = (unsigned long) page_address(sg->page);
		if (!addr)
			continue;
#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
		/* EJ: i think user should not access these memory during the DMA, so... */
#endif
		dma_cache_wback_inv(addr + sg->offset, sg->length);
	}
}

EXPORT_SYMBOL(dma_unmap_sg);

void dma_sync_single_for_cpu(struct device *dev, dma_addr_t dma_handle,
	size_t size, enum dma_data_direction direction)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = dma_handle + PAGE_OFFSET;
	__dma_sync(addr, size, direction);
}

EXPORT_SYMBOL(dma_sync_single_for_cpu);

void dma_sync_single_for_device(struct device *dev, dma_addr_t dma_handle,
	size_t size, enum dma_data_direction direction)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = dma_handle + PAGE_OFFSET;
	__dma_sync(addr, size, direction);
}

EXPORT_SYMBOL(dma_sync_single_for_device);

void dma_sync_single_range_for_cpu(struct device *dev, dma_addr_t dma_handle,
	unsigned long offset, size_t size, enum dma_data_direction direction)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = dma_handle + offset + PAGE_OFFSET;
	__dma_sync(addr, size, direction);
}

EXPORT_SYMBOL(dma_sync_single_range_for_cpu);

void dma_sync_single_range_for_device(struct device *dev, dma_addr_t dma_handle,
	unsigned long offset, size_t size, enum dma_data_direction direction)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = dma_handle + offset + PAGE_OFFSET;
	__dma_sync(addr, size, direction);
}

EXPORT_SYMBOL(dma_sync_single_range_for_device);

void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg, int nelems,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	/* Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nelems; i++, sg++)
		__dma_sync((unsigned long)page_address(sg->page),
		           sg->length, direction);
}

EXPORT_SYMBOL(dma_sync_sg_for_cpu);

void dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg, int nelems,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	/* Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nelems; i++, sg++)
		__dma_sync((unsigned long)page_address(sg->page),
		           sg->length, direction);
}

EXPORT_SYMBOL(dma_sync_sg_for_device);

int dma_mapping_error(dma_addr_t dma_addr)
{
	return 0;
}

EXPORT_SYMBOL(dma_mapping_error);

int dma_supported(struct device *dev, u64 mask)
{
	/*
	 * we fall back to GFP_DMA when the mask isn't all 1s,
	 * so we can't guarantee allocations that must be
	 * within a tighter range than GFP_DMA..
	 */
	if (mask < 0x00ffffff)
		return 0;

	return 1;
}

EXPORT_SYMBOL(dma_supported);

int dma_is_consistent(dma_addr_t dma_addr)
{
	return 1;
}

EXPORT_SYMBOL(dma_is_consistent);

void dma_cache_sync(void *vaddr, size_t size, enum dma_data_direction direction)
{
	if (direction == DMA_NONE)
		return;

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	struct page *page = NULL;
	unsigned long addr1, addr2, flags;

	page = virt_to_page((unsigned long)vaddr);
	addr1 = (unsigned long)kmap_coherent(page, &flags)+((unsigned long)vaddr & (PAGE_SIZE-1));
	addr2 = addr1+0x1000;
	BUG_ON((addr1 & PAGE_MASK) != ((addr1+size-1) & PAGE_MASK));

	dma_cache_wback_inv(addr1, size);
	dma_cache_wback_inv(addr2, size);

	kunmap_coherent(&flags);
#else
	dma_cache_wback_inv((unsigned long)vaddr, size);
#endif
}

EXPORT_SYMBOL(dma_cache_sync);

/* The DAC routines are a PCIism.. */

#ifdef CONFIG_PCI

#include <linux/pci.h>

dma64_addr_t pci_dac_page_to_dma(struct pci_dev *pdev,
	struct page *page, unsigned long offset, int direction)
{
	return (dma64_addr_t)page_to_phys(page) + offset;
}

EXPORT_SYMBOL(pci_dac_page_to_dma);

struct page *pci_dac_dma_to_page(struct pci_dev *pdev,
	dma64_addr_t dma_addr)
{
	return mem_map + (dma_addr >> PAGE_SHIFT);
}

EXPORT_SYMBOL(pci_dac_dma_to_page);

unsigned long pci_dac_dma_to_offset(struct pci_dev *pdev,
	dma64_addr_t dma_addr)
{
	return dma_addr & ~PAGE_MASK;
}

EXPORT_SYMBOL(pci_dac_dma_to_offset);

void pci_dac_dma_sync_single_for_cpu(struct pci_dev *pdev,
	dma64_addr_t dma_addr, size_t len, int direction)
{
	BUG_ON(direction == PCI_DMA_NONE);

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	struct page *page = NULL;
	unsigned long addr, addr1, addr2, flags;

	addr = dma_addr+PAGE_OFFSET;
	page = virt_to_page((unsigned long)addr);
	addr1 = (unsigned long)kmap_coherent(page, &flags)+(addr & (PAGE_SIZE-1));
	addr2 = addr1+0x1000;
	BUG_ON((addr & PAGE_MASK) != ((addr+size-1) & PAGE_MASK));

	dma_cache_wback_inv(addr1, size);
	dma_cache_wback_inv(addr2, size);

	kunmap_coherent(&flags);
#else
	dma_cache_wback_inv(dma_addr + PAGE_OFFSET, len);
#endif
}

EXPORT_SYMBOL(pci_dac_dma_sync_single_for_cpu);

void pci_dac_dma_sync_single_for_device(struct pci_dev *pdev,
	dma64_addr_t dma_addr, size_t len, int direction)
{
	BUG_ON(direction == PCI_DMA_NONE);

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	struct page *page = NULL;
	unsigned long addr, addr1, addr2, flags;

	addr = dma_addr+PAGE_OFFSET;
	page = virt_to_page((unsigned long)addr);
	addr1 = (unsigned long)kmap_coherent(page, &flags)+(addr & (PAGE_SIZE-1));
	addr2 = addr1+0x1000;
	BUG_ON((addr & PAGE_MASK) != ((addr+size-1) & PAGE_MASK));

	dma_cache_wback_inv(addr1, size);
	dma_cache_wback_inv(addr2, size);

	kunmap_coherent(&flags);
#else
	dma_cache_wback_inv(dma_addr + PAGE_OFFSET, len);
#endif
}

EXPORT_SYMBOL(pci_dac_dma_sync_single_for_device);

#endif /* CONFIG_PCI */
