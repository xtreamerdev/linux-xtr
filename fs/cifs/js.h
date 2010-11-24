#ifndef _LINUX_CIFS_JS_H
#define _LINUX_CIFS_JS_H
// Jason Lee for porting cifs from linux 2.6.19.1 rc3 (cifs version) to 2.6.12(kernel)
#include <linux/fs.h>
#include <linux/slab.h>
static inline void drop_nlink(struct inode *inode)
{
        inode->i_nlink--;
}
static inline void inc_nlink(struct inode *inode)
{
        inode->i_nlink++;
}
static inline void clear_nlink(struct inode *inode)
{
        inode->i_nlink = 0;
}

static inline void *kzalloc(size_t size, unsigned int __nocast flags)
{
        void *ret = kmalloc(size, flags);
        if (ret)
                memset(ret, 0, size);
        return ret;
}
#endif /* _LINUX_GETOPT_H */

