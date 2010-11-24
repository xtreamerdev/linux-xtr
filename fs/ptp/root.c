/*
 * ptpfs filesystem for Linux.
 *
 * This file is released under the GPL.
 */


#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/string.h>
//#include <linux/locks.h>
#include <asm/uaccess.h>
#include <asm-mips/types.h>

#include "ptp.h"              
#include "ptpfs.h"

/*
 * Root of the PTPFS is a read/only directory which 
 * holds directories for each of the storage units.
 *
 * Eventually, we may add a "Albums" or similar sub
 * directory for the album support.
 * 
 * We could also add readonly files like "Camera.txt"
 * for querying camera information and settings
 */
#ifndef HAVE_ARCH_PAGE_BUG
#define PAGE_BUG(page) do { \
        printk("page BUG for page at %p\n", page); \
        BUG(); \
} while (0)
#endif


static void get_root_dir_name(struct ptp_storage_info *storageinfo, char *fsname, int* typeCount) 
{
    int count = 0;
    const char *base;
    switch (storageinfo->storage_type)
    {
    case PTP_ST_FixedROM:
        count = typeCount[0];
        typeCount[0]++;
        base = "Internal_ROM";
        break;
    case PTP_ST_RemovableROM:
        count = typeCount[1];
        typeCount[1]++;
        base = "Removable_ROM";
        break;
    case PTP_ST_FixedRAM:
        count = typeCount[2];
        typeCount[2]++;
        base = "InternalMemory";
        break;
    case PTP_ST_RemovableRAM:
        count = typeCount[3];
        typeCount[3]++;
        base = "MemoryCard";
        break;
    default:
        count = typeCount[4];
        typeCount[4]++;
        base = "Unknown";
        break;
    }
    if (count)
    {
        sprintf(fsname,"%s_%d",base,count);
    }
    else
    {
        sprintf(fsname,base);
    }
}
static int ptpfs_root_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
//	printk(KERN_INFO "%s\n",  __FUNCTION__);
  
    struct inode *inode = filp->f_dentry->d_inode;
    struct dentry *dentry = filp->f_dentry;
    struct ptp_storage_ids storageids;
    int ino;
    int x;
    char fsname[256];
    int typeCount[5];
    memset(typeCount,0,sizeof(typeCount));
	memset(&storageids,0,sizeof(storageids)); 

    int offset = filp->f_pos;

	if (offset)
	{
		return 1;
	}

	switch (offset)
	{
		case 0:
			if (filldir(dirent, ".", 1, offset, inode->i_ino, DT_DIR) < 0)
				return 0;
			offset++;
			filp->f_pos++;
			// fall through 
		case 1:
			spin_lock(&dcache_lock);
			ino = dentry->d_parent->d_inode->i_ino;
			spin_unlock(&dcache_lock);
			if (filldir(dirent, "..", 2, offset, ino, DT_DIR) < 0)
				break;
			filp->f_pos++;
			offset++;
		default:
			if (ptp_getstorageids(PTPFSSB(inode->i_sb), &storageids)!=PTP_RC_OK)
			{
				printk(KERN_INFO "Error getting storage ids\n");
				storageids.n = 0;
				storageids.storage = NULL;
			}
			for (x = 0; x < storageids.n; x++)
			{
				struct ptp_storage_info storageinfo;
				if ((storageids.storage[x]&0x0000ffff)==0) continue;

				memset(&storageinfo,0,sizeof(storageinfo));
				if (ptp_getstorageinfo(PTPFSSB(inode->i_sb), storageids.storage[x],&storageinfo)!=PTP_RC_OK)
				{
					printk(KERN_INFO "Error getting storage info\n");
				}
				else
				{
					get_root_dir_name(&storageinfo, fsname, typeCount) ;
					ino = storageids.storage[x];
					ptp_free_storage_info(&storageinfo);
					if (filldir(dirent, fsname, strlen(fsname), filp->f_pos, ino ,DT_DIR) < 0)
					{
						ptp_free_storage_ids(&storageids);
						return 0;
					}
					filp->f_pos++;
				}
			}
			ptp_free_storage_ids(&storageids);
			return 1;
	}
	return 0;
}
/*
 * Lookup the data. This is trivial - if the dentry didn't already
 * exist, we know it is negative.
 */
static struct dentry * ptpfs_root_lookup(struct inode *dir, struct dentry *dentry)
{
//    printk(KERN_INFO "%s\n",  __FUNCTION__);

    struct ptp_storage_ids storageids;
    int x;
    char fsname[256];
    int typeCount[5];
    memset(&storageids,0,sizeof(storageids)); //add by evan
    memset(typeCount,0,sizeof(typeCount));

    if (ptp_getstorageids(PTPFSSB(dir->i_sb), &storageids)!=PTP_RC_OK)
	{
        printk(KERN_INFO "Error getting storage ids\n");
        storageids.n = 0;
        storageids.storage = NULL;
	}

    int offset = 0;
    for (x = 0; x < storageids.n; x++)
	{
        struct ptp_storage_info storageinfo;
        if ((storageids.storage[x]&0x0000ffff)==0) continue;

        memset(&storageinfo,0,sizeof(storageinfo));
        if (ptp_getstorageinfo(PTPFSSB(dir->i_sb), storageids.storage[x],&storageinfo)!=PTP_RC_OK)
 		{
            printk(KERN_INFO "Error getting storage info\n");
 		}
        else
        	{
            get_root_dir_name(&storageinfo,fsname,typeCount);
            offset++;
            if (!memcmp(fsname,dentry->d_name.name,dentry->d_name.len))
            		{
                struct inode *newi = ptpfs_get_inode(dir->i_sb, S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH , 0,storageids.storage[x]);
              PTPFSINO(newi)->parent = dir; //dir should be root directory
                //newi->i_fop = &ptpfs_stgdir_operations;
                //newi->i_op = &ptpfs_stgdir_inode_operations;
                PTPFSINO(newi)->type = INO_TYPE_STGDIR;
                PTPFSINO(newi)->storage = storageids.storage[x];
                if (newi) d_add(dentry, newi);
                ptp_free_storage_ids(&storageids);
                ptp_free_storage_info(&storageinfo);
                return NULL;
            		}
            ptp_free_storage_info(&storageinfo);
        	}
	}
    ptp_free_storage_ids(&storageids);
    return NULL;

}

static int ptpfs_root_setattr(struct dentry *d, struct iattr * a)
{
    return -EPERM;
}


struct inode_operations ptpfs_rootdir_inode_operations = {
    lookup:     ptpfs_root_lookup,
    setattr:    ptpfs_root_setattr,
};
struct file_operations ptpfs_rootdir_operations = {
    read:       generic_read_dir,  
    readdir:    ptpfs_root_readdir,
};


