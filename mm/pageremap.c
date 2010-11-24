/*
 *  linux/mm/pageremap.c
 *
 *  Copyright (C) 1991, 1992, 1993, 1994  Linus Torvalds
 *
 *  Support of page remapping, I-Chieh Hsu
 */

#include <linux/mm.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>
#include <linux/swap.h>
#include <linux/rmap.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/preempt.h>
#include <linux/buffer_head.h>
#include <linux/writeback.h>
#include <linux/auth.h>
#include <asm/smp.h>

#include <linux/pageremap.h>

//#define USE_KERNEL_THREAD
long		swap_name_addr;

#ifdef CONFIG_SWAP

atomic_t	remapd_count;

EXPORT_SYMBOL(swap_name_addr);

static void print_buffer(struct page* page)
{
	struct address_space* mapping = page_mapping(page);
	struct buffer_head *bh, *head;

	spin_lock(&mapping->private_lock);
	bh = head = page_buffers(page);
	printk("buffers:");
	do {
		printk(" state:%lx count:%d", bh->b_state, atomic_read(&bh->b_count));

		bh = bh->b_this_page;
	} while (bh != head);
	printk("\n");
	spin_unlock(&mapping->private_lock);
}
/*
static void free_more_memory(void)
{
	struct zone **zones;
	pg_data_t *pgdat;
	
	wakeup_bdflush(1024);
	yield();
	
	for_each_pgdat(pgdat) {
		zones = pgdat->node_zonelists[GFP_DVRNOFS&GFP_ZONEMASK].zones;
		if (*zones)
			try_to_free_pages(zones, GFP_DVRNOFS | __GFP_HUGEFREE, 0, NULL);
	}
}
*/
/*
 * Allocate a new page from specified node.
 */
static struct page *
remap_alloc_page(unsigned int gfp_mask)
{
	return alloc_page(gfp_mask);
}

static int
remap_delete_page(struct page *page)
{
	BUG_ON(page_count(page) != 1);
	put_page(page);
	return 0;
}

static int
remap_copy_page(struct page *to, struct page *from)
{
	copy_highpage(to, from);
	return 0;
}

static int
remap_lru_add_page(struct page *page, int active)
{
	if (active)
		lru_cache_add_active(page);
	else
		lru_cache_add(page);
	return 0;
}

static int
stick_mlocked_page(struct list_head *vlist)
{
	struct page_va_list *v1;
	struct vm_area_struct *vma;
	int error;

	while(!list_empty(vlist)) {
		v1 = list_entry(vlist->next, struct page_va_list, list);
		list_del(&v1->list);
		down_read(&v1->mm->mmap_sem);
		vma = find_vma(v1->mm, v1->addr);
		if (vma == NULL || !(vma->vm_flags & VM_LOCKED))
			goto out;
		error = get_user_pages(current, v1->mm, v1->addr, PAGE_SIZE,
		    (vma->vm_flags & VM_WRITE) != 0, 0, NULL, NULL);
	out:
		up_read(&v1->mm->mmap_sem);
		mmput(v1->mm);
		kfree(v1);
	}
	return 0;
}

/* helper function for remap_onepage */
#define	REMAPPREP_WB		1
#define	REMAPPREP_BUFFER	2

/*
 * Try to free buffers if "page" has them.
 */
static int
remap_preparepage(struct page *page, int fastmode)
{
	struct address_space *mapping;
	int waitcnt = fastmode ? 0 : 100;

	BUG_ON(!PageLocked(page));

	mapping = page_mapping(page);

	if (!PagePrivate(page) && PageWriteback(page) && !PageSwapCache(page)) {
		printk("remap: mapping %p page %p\n", page->mapping, page);
		return -REMAPPREP_WB;
	}

	while (PageWriteback(page)) {
		if (!waitcnt)
			return -REMAPPREP_WB;
		__set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(10);
		__set_current_state(TASK_RUNNING);
		waitcnt--;
	}

	if (PagePrivate(page)) {
#ifdef DEBUG_MSG
		printk("rmap: process page with buffers...\n");
#endif
		/* XXX copied from shrink_list() */
		if (PageDirty(page) &&
		    is_page_cache_freeable(page) &&
		    mapping != NULL &&
		    mapping->a_ops->writepage != NULL) {
			spin_lock_irq(&mapping->tree_lock);
			if (clear_page_dirty_for_io(page)) {
				int res;
				struct writeback_control wbc = {
					.sync_mode = WB_SYNC_NONE,
					.nr_to_write = SWAP_CLUSTER_MAX,
					.nonblocking = 1,
					.for_reclaim = 1,
				};

				spin_unlock_irq(&mapping->tree_lock);

				SetPageReclaim(page);
				res = mapping->a_ops->writepage(page, &wbc);

				if (res < 0)
					/* not implemented. help */
					BUG();
				if (res == WRITEPAGE_ACTIVATE) {
					ClearPageReclaim(page);
					return -REMAPPREP_WB;
				}
				if (!PageWriteback(page)) {
					/* synchronous write or broken a_ops? */
					ClearPageReclaim(page);
				}
				lock_page(page);
				mapping = page_mapping(page);
				if (!PagePrivate(page))
					return 0;
			} else
				spin_unlock_irq(&mapping->tree_lock);
		}

		while (1) {
			if (try_to_release_page(page, GFP_KERNEL))
				break;
			if (!waitcnt)
				return -REMAPPREP_BUFFER;
			__set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(10);
			__set_current_state(TASK_RUNNING);
			waitcnt--;
			if (!waitcnt)
				print_buffer(page);
		}
	}

	return 0;
}

/*
 * Just assign swap space to a anonymous page if it doesn't have yet,
 * so that the page can be handled like a page in the page cache
 * since it in the swap cache. 
 */
static struct address_space *
make_page_mapped(struct page *page)
{	
	if (!page_mapped(page)) {
		if (page_count(page) > 1)
			printk("remap: page %p not mapped: count %d\n",
			    page, page_count(page));
		return NULL;
	}
	/* The page is an anon page.  Allocate its swap entry. */
//	page_map_unlock(page);
//	if (!add_to_swap(page))
//		printk("remap: swap cache allocation fail...\n");	// ***
	while (!add_to_swap(page, 1)) {
#ifdef DEBUG_MSG
		printk("remap: reinitialize swap area...\n");
#endif
		sys_swapoff((char *)swap_name_addr);
		sys_swapon((char *)swap_name_addr, 0);
	}
//	page_map_lock(page);
	return page_mapping(page);
}

/*
 * Replace "page" with "newpage" on the radix tree.  After that, all
 * new access to "page" will be redirected to "newpage" and it
 * will be blocked until remapping has been done.
 */
static int
radix_tree_replace_pages(struct page *page, struct page *newpage,
			 struct address_space *mapping)
{
	struct buffer_head *bh;

	if (radix_tree_preload(GFP_KERNEL))
		return -1;

	if (PagePrivate(page)) /* XXX */
	{
		bh = (struct buffer_head *)page->private;
		printk("page value: %x, buffer_head->b_page: %x \n", (int)page, (int)bh->b_page);
		if (try_to_free_buffers(page) != 1)
		{
			printk("try_to_free_buffers fail...\n");
			BUG();
		}
		else
			printk("try_to_free_buffers succeed...\n");
	}

	/* should {__add_to,__remove_from}_page_cache be used instead? */
	spin_lock_irq(&mapping->tree_lock);
	if (mapping != page_mapping(page))
		printk("remap: mapping changed %p -> %p, page %p\n",
		    mapping, page_mapping(page), page);
	if (radix_tree_delete(&mapping->page_tree,
	    PageSwapCache(page) ? page->private : page->index) == NULL) {
		/* Page truncated. */
		spin_unlock_irq(&mapping->tree_lock);
		radix_tree_preload_end();
		return -1;
	}
	/* Don't __put_page(page) here.  Truncate may be in progress. */
	newpage->flags |= page->flags & ~(1 << PG_uptodate) &
	    ~(1 << PG_highmem) & ~(1 << PG_active) & ~(~0UL << NODEZONE_SHIFT);

	/* list_del(&page->list); XXX */
	radix_tree_insert(&mapping->page_tree,
	    PageSwapCache(page) ? page->private : page->index, newpage);
	page_cache_get(newpage);
	newpage->mapping = page->mapping;
	newpage->index = page->index;
	if  (PageSwapCache(page)) {
		SetPageSwapCache(newpage);
		newpage->private = page->private;
	}

	spin_unlock_irq(&mapping->tree_lock);
	radix_tree_preload_end();
	return 0;
}

/*
 * Roll back all remapping operations.
 */
static int
radix_tree_rewind_page(struct page *page, struct page *newpage,
		 struct address_space *mapping)
{
	int waitcnt;
	unsigned long index;

	/*
	 * Try to unwind by notifying waiters.  If someone misbehaves,
	 * we die.
	 */
	if (radix_tree_preload(GFP_KERNEL))
		BUG();

	/* should {__add_to,__remove_from}_page_cache be used instead? */
	spin_lock_irq(&mapping->tree_lock);
	/* list_del(&newpage->list); */
	index = PageSwapCache(page) ? page->private : page->index;
	if (radix_tree_delete(&mapping->page_tree, index) == NULL)
		/* Hold extra count to handle truncate */
		page_cache_get(newpage);
	radix_tree_insert(&mapping->page_tree, index, page);
	/* no page_cache_get(page); needed */
	spin_unlock_irq(&mapping->tree_lock);
	radix_tree_preload_end();

	SetPageAgain(newpage);
	newpage->mapping = NULL;
	/* XXX unmap needed?  No, it shouldn't.  Handled by fault handlers. */
	unlock_page(newpage);

	waitcnt = HZ;
	for(; page_count(newpage) > 2; waitcnt--) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(10);
		if (waitcnt == 0) {
			printk("You are hosed.\n");
			printk("newpage %p flags %lx %d, page %p flags %lx %d\n",
			    newpage, newpage->flags, page_count(newpage),
			    page, page->flags, page_count(page));
			BUG();
		}
	}
	BUG_ON(PageUptodate(newpage));
	ClearPageDirty(newpage);
	ClearPageActive(newpage);
	spin_lock_irq(&mapping->tree_lock);
	if (page_count(newpage) == 1) {
		printk("remap: newpage %p truncated. page %p\n", newpage, page);
		BUG();
	}
	spin_unlock_irq(&mapping->tree_lock);
	unlock_page(page);
	BUG_ON(page_count(newpage) != 2);
	ClearPageAgain(newpage);
	__put_page(newpage);
	return 1;
}

/*
 * Remove all PTE mappings to "page".
 */
static int
unmap_page(struct page *page, struct list_head *vlist)
{
	int error = SWAP_SUCCESS;

//	page_map_lock(page);
	while (page_mapped(page) &&
	    (error = try_to_unmap(page, vlist)) == SWAP_AGAIN) {
//		page_map_unlock(page);
		__set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(1);
		__set_current_state(TASK_RUNNING);
//		page_map_lock(page);
	}
//	page_map_unlock(page);
	if (error == SWAP_FAIL) {
		/* either during mremap or mlocked */
		return -1;
	}
	return 0;
}

/*
 * Wait for "page" to become free.  Almost same as waiting for its
 * page count to drop to 2, but truncated pages are special.
 */
static int
wait_on_page_freeable(struct page *page, struct address_space *mapping,
			struct list_head *vlist, int truncated,
			int nretry, struct remap_operations *ops)
{
	struct address_space *mapping1;

	while ((truncated + page_count(page)) > 2) {
		if (nretry <= 0)
			return -1;
		/* no lock needed while waiting page count */
		unlock_page(page);

		while ((truncated + page_count(page)) > 2) {
			nretry--;
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(1);
			if ((nretry % 5000) == 0) {
				printk("remap: still waiting on %p %d\n", page, nretry);
				break;
			}
			if (PagePrivate(page) || page_mapped(page))
				break;		/* see below */
		}

		lock_page(page);
		BUG_ON(page_count(page) == 0);
		mapping1 = page_mapping(page);
		if (mapping != mapping1 && mapping1 != NULL)
			printk("remap: mapping changed %p -> %p, page %p\n",
			    mapping, mapping1, page);
		if (PagePrivate(page))
			ops->remap_release_buffers(page);
		unmap_page(page, vlist);
	}
	return nretry;
}

/*
 * A file which "page" belongs to has been truncated.  Free both pages.
 */
static void
free_truncated_pages(struct page *page, struct page *newpage,
			 struct address_space *mapping)
{
	void *p;
	/* mapping->tree_lock must be held. */
	p = radix_tree_lookup(&mapping->page_tree,
	    PageSwapCache(newpage) ? newpage->private : newpage->index);
	if (p != NULL) {
		/* new cache page appeared after truncation */
		printk("page %p newpage %p radix %p\n",
		    page, newpage, p);
		BUG_ON(p == newpage);
	}
	BUG_ON(page_mapping(page) != NULL);
	put_page(newpage);
	if (page_count(newpage) != 1) {
		printk("newpage count %d != 1, %p\n",
		    page_count(newpage), newpage);
		BUG();
	}
	/* No need to do page->list.  remove_from_page_cache did. */
	newpage->mapping = page->mapping = NULL;
	spin_unlock_irq(&mapping->tree_lock);
	ClearPageActive(page);
	ClearPageActive(newpage);
	ClearPageSwapCache(page);
	ClearPageSwapCache(newpage);
	unlock_page(page);
	unlock_page(newpage);
	put_page(newpage);
}

static inline int
is_page_truncated(struct page *page, struct page *newpage,
			 struct address_space *mapping)
{
	void *p;
	spin_lock_irq(&mapping->tree_lock);
	if (page_count(page) == 1) {
		/* page has been truncated. */
		return 0;
	}
	p = radix_tree_lookup(&mapping->page_tree,
	    PageSwapCache(newpage) ? newpage->private : newpage->index);
	spin_unlock_irq(&mapping->tree_lock);
	if (p == NULL) {
		BUG_ON(page->mapping != NULL);
		return -1;
	}
	return 1;
}

/*
 * Replace "page" with "newpage" on the list of clean/dirty pages.
 */
static void
remap_exchange_pages(struct page *page, struct page *newpage,
			 struct address_space *mapping)
{
	if (PageDirty(page))
		set_page_dirty(newpage);
	page->mapping = NULL;
	unlock_page(page);

	__put_page(page);

	/* We are done.  Finish and let the waiters run. */
	SetPageUptodate(newpage);
}

static int
remap_release_buffer(struct page *page)
{
	try_to_release_page(page, GFP_KERNEL);
	return 0;
}

static struct remap_operations remap_ops = {
	.remap_alloc_page		= remap_alloc_page,
	.remap_delete_page		= remap_delete_page,
	.remap_copy_page		= remap_copy_page,
	.remap_lru_add_page		= remap_lru_add_page,
	.remap_release_buffers		= remap_release_buffer,
	.remap_prepare			= remap_preparepage,
	.remap_stick_page		= stick_mlocked_page
};

/*
 * Try to remap a page.  Returns non-zero on failure.
 */
int remap_onepage(struct page *page, int fastmode, struct remap_operations *ops)
{
	struct page *newpage;
	struct zone *zone;
	struct address_space *mapping;
	LIST_HEAD(vlist);
	int truncated = 0;
	int nretry = fastmode ? HZ/50: HZ*10; /* XXXX */

	zone = zone_table[ZONE_DMA];
	if (!zone_watermark_ok(zone, 0, zone->pages_high, 0, 0, 0))
		return -ENOMEM;
	if ((newpage = ops->remap_alloc_page(GFP_HIGHUSER)) == NULL)
		return -ENOMEM;
	if (TestSetPageLocked(newpage))
		BUG();
	lock_page(page);

	if (ops->remap_prepare && ops->remap_prepare(page, fastmode))
		goto radixfail;

//	page_map_lock(page);
	if (PageAnon(page) && !PageSwapCache(page))
		make_page_mapped(page);
	mapping = page_mapping(page);
//	page_map_unlock(page);
	if (mapping == NULL) {
		printk("remap: mapping is NULL\n");
		goto radixfail;
	}

	if (radix_tree_replace_pages(page, newpage, mapping))
		goto radixfail;
	if (unmap_page(page, &vlist)) {
		printk("remap: unmap fail...\n");
		goto unmapfail;
	}
	if (PagePrivate(page))
		printk("remap: buffer reappeared\n");
wait_again:
	if ((nretry = wait_on_page_freeable(page, mapping, &vlist, truncated, nretry, ops)) < 0) {
		printk("remap: nretry < 0...\n");
		goto unmapfail;
	}

	if (PageReclaim(page) || PageWriteback(page) || PagePrivate(page))
#ifdef CONFIG_KDB
		KDB_ENTER();
#else
		BUG();
#endif
	switch (is_page_truncated(page, newpage, mapping)) {
		case 0:
			/* has been truncated */
			free_truncated_pages(page, newpage, mapping);
			ops->remap_delete_page(page);
			return 0;
		case -1:
			/* being truncated */
			truncated = 1;
			BUG_ON(page_mapping(page) != NULL);
			goto wait_again;
		default:
			/* through */
			;
	}
	
	BUG_ON(mapping != page_mapping(page));

	ops->remap_copy_page(newpage, page);
	remap_exchange_pages(page, newpage, mapping);
	if (ops->remap_lru_add_page)
		ops->remap_lru_add_page(newpage, PageActive(page));
	ClearPageActive(page);
	ClearPageSwapCache(page);
	ops->remap_delete_page(page);

	/*
	 * Wake up all waiters which are waiting for completion
	 * of remapping operations.
	 */
	unlock_page(newpage);

	if (ops->remap_stick_page)
		ops->remap_stick_page(&vlist);
	page_cache_release(newpage);
	return 0;

unmapfail:
	printk("remap: rewind...\n");
	radix_tree_rewind_page(page, newpage, mapping);
	if (ops->remap_stick_page)
		ops->remap_stick_page(&vlist);
	ClearPageActive(newpage);
	ClearPageSwapCache(newpage);
	ops->remap_delete_page(newpage);
	return 1;

radixfail:
	unlock_page(page);
	unlock_page(newpage);
	if (ops->remap_stick_page)
		ops->remap_stick_page(&vlist);
	ops->remap_delete_page(newpage);
	return 1;
}

int remap_DVR_zone(void *p)
{
	struct zone *zone = zone_table[ZONE_DVR];
	struct page *page, *pagetail;
	struct list_head *l;
	int active, i, nr_failed = 0;
	int fastmode = 100, retvalue;
	LIST_HEAD(failedp);
	int map_done = 0, map_fail = 0;

//	preempt_disable();
#ifdef USE_KERNEL_THREAD
	daemonize("remap%d", zone->zone_start_pfn);
#endif //USE_KERNEL_THREAD
	if (atomic_add_return(1, &remapd_count) > 1) {
		printk("remap: remapd already running\n");
		atomic_dec(&remapd_count);
		return 0;
	}
//	on_each_cpu(lru_drain_schedule, NULL, 1, 1);
	printk("1. start remap DVR zone...\n");
	lru_add_drain();							// flush pagevec

	while (nr_failed < 100) {
		spin_lock_irq(&zone->lru_lock);
		for (active = 0; active < 2; active++) {
			l = active ? &zone->active_list :
			    &zone->inactive_list;
			for (i = 0; !list_empty(l) && i < 10; i++) {
				page = list_entry(l->prev, struct page, lru);
				if (fastmode && PageLocked(page)) {
					printk("remap: page in list is locked...\n");
					pagetail = page;
					while (fastmode && PageLocked(page)) {
						page =
						    list_entry(page->lru.prev,
						    struct page, lru);
						fastmode--;
						if (&page->lru == l) {
							/* scanned the whole list */
							printk("remap: use the locked page...\n");
							page = pagetail;
							break;
						}
						if (page == pagetail)
							BUG();
					}
					if (!fastmode) {
						printk("remap: used up fastmode\n");
						page = pagetail;
					}
				}
				if (!TestClearPageLRU(page))
					BUG();
				list_del(&page->lru);
				if (get_page_testone(page)) {
					/* the page is in pagevec_release();
					   shrink_cache says so. */
					__put_page(page);
					SetPageLRU(page);
					list_add(&page->lru, l);
					continue;
				}
				if (active)
					zone->nr_active--;
				else
					zone->nr_inactive--;
				spin_unlock_irq(&zone->lru_lock);
				goto got_page;
			} // search whole list
		} // determine inactive & active list
		spin_unlock_irq(&zone->lru_lock);
		break;

	got_page:
		if (retvalue = remap_onepage(page, fastmode, &remap_ops)) {
			nr_failed++;
			if (fastmode)
				fastmode--;
			list_add(&page->lru, &failedp);
			if (retvalue == -ENOMEM) {
				printk("remap: not enough memory...\n");
				break;
			}
		} else
			map_done++;
	}
	
	if (list_empty(&failedp))
		goto out;

	printk("2. redo mapping pages...\n");
	while (!list_empty(&failedp)) {
		page = list_entry(failedp.prev, struct page, lru);
		list_del(&page->lru);
		map_fail++;	// ***
		if (!TestSetPageLocked(page)) {
			if (remap_preparepage(page, 10 /* XXX */)) {
				unlock_page(page);
			} else {
				ClearPageLocked(page);	/* XXX */
				if (!remap_onepage(page, 0, &remap_ops))
					continue;
			}
		}
		spin_lock_irq(&zone->lru_lock);
		if (PageActive(page)) {
			list_add(&page->lru, &zone->active_list);
			zone->nr_active++;
		} else {
			list_add(&page->lru, &zone->inactive_list);
			zone->nr_inactive++;
		}
		if (TestSetPageLRU(page))
			BUG();
		spin_unlock_irq(&zone->lru_lock);
		page_cache_release(page);
	}

out:
	drain_pcp_pages(smp_processor_id());		// flush per_cpu_pages

	printk("fastmode is %d...\n", fastmode);
	printk("map_done is %d...\n", map_done);
	printk("map_fail is %d...\n", map_fail);
	atomic_dec(&remapd_count);
//	preempt_enable();
	return 0;
}

void start_remap(void)
{
#ifdef USE_KERNEL_THREAD
	kernel_thread(remap_DVR_zone, NULL, CLONE_KERNEL);
#else
	remap_DVR_zone(NULL);
#endif //USE_KERNEL_THREAD
}

#else

void start_remap(void)
{
	printk("CONFIG_SWAP option is not open...\n");
}

#endif

EXPORT_SYMBOL(start_remap);

