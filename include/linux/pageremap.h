#ifndef _LINUX_PAGEREMAP_H
#define _LINUX_PAGEREMAP_H

#include <linux/config.h>
#include <linux/mm.h>

#ifdef __KERNEL__

struct page_va_list {
        struct mm_struct *mm;
        unsigned long addr;
        struct list_head list;
};

struct remap_operations {
        struct page * (*remap_alloc_page)(unsigned int);
        int (*remap_delete_page)(struct page *);
        int (*remap_copy_page)(struct page *, struct page *);
        int (*remap_lru_add_page)(struct page *, int);
        int (*remap_release_buffers)(struct page *);
        int (*remap_prepare)(struct page *page, int fastmode);
        int (*remap_stick_page)(struct list_head *vlist);
};

extern int remapd(void *p);
extern int remap_onepage(struct page *, int, struct remap_operations *);

#endif /* __KERNEL__ */
#endif /* _LINUX_PAGEREMAP_H */
