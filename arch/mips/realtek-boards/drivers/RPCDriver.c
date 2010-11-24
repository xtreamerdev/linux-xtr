/*
 * $Id: RPCDriver.c,v 1.10 2004/8/4 09:25 Jacky Exp $
 */
#include <linux/config.h>
/*
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
    #define MODVERSIONS
#endif

#ifdef MODVERSIONS
    #include <linux/modversions.h>
#endif

#ifndef MODULE
#define MODULE
#endif
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/system.h>     /* cli(), *_flags */
#include <asm/uaccess.h>    /* copy_to_user() copy_from_user() */

#include "RPCDriver.h"

MODULE_LICENSE("Dual BSD/GPL");

struct file_operations *rpc_fop_array[]={
    &rpc_poll_fops, /* poll */
    &rpc_intr_fops  /* intr */
};

void **rpc_data_ptr_array[]={
	(void **)&rpc_poll_devices, /* poll */
	(void **)&rpc_intr_devices  /* intr */
};

int rpc_data_size_array[]={
	sizeof(RPC_POLL_Dev), /* poll */
	sizeof(RPC_INTR_Dev)  /* intr */
};

int         rpc_major   = RPC_MAJOR;
const char *rpc_name    = "RPC";

/*
 * Finally, the module stuff
 */

#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
devfs_handle_t rpc_devfs_dir;
static char devname[4];
#endif
#endif

int my_copy_user(int *des, int *src, int size)
{
	char *csrc, *cdes;
	int i;

	if (((int)src & 0x3) || ((int)des & 0x3))
		printk("my_copy_user: unaligned happen...\n");

	while (size >= 4) {
		*des++ = *src++;
		size -= 4;
	}

	csrc = (char *)src;
	cdes = (char *)des;
	for (i = 0; i < size; i++)
		cdes[i] = csrc[i];

	return 0;
}

irqreturn_t rpc_isr(int irq, void *dev_id, struct pt_regs *regs)
{
    int itr;
    PDEBUG("irq number %d...\n", irq);

	itr = readl((void *)0xb801a104);
	if (itr & 0x48) {
		while (itr & 0x48) {
			// ack the interrupt
			if (itr & 0x08) {
				writel(0x08, (void *)0xb801a104);
				wake_up_interruptible(&(rpc_intr_devices[1].waitQueue));
			}
			if (itr & 0x40) {
				writel(0x40, (void *)0xb801a104);
				wake_up_interruptible(&(rpc_intr_devices[3].waitQueue));
			}
			
			itr = readl((void *)0xb801a104);
		}
	} else {
//		printk("Not RPC interrupt...\n");
		return IRQ_NONE;
	}
	
	return IRQ_HANDLED;
}

// Destructor of this module.
void RPC_cleanup_module(void)
{
    /* call the cleanup functions for friend devices */
    rpc_poll_cleanup();
    rpc_intr_cleanup();

#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
	devfs_unregister(rpc_devfs_dir);
#else
	devfs_remove("rpc");

    /* cleanup_module is never called if registering failed */
    unregister_chrdev(rpc_major, rpc_name);
#endif
#else
    /* cleanup_module is never called if registering failed */
    unregister_chrdev(rpc_major, rpc_name);
#endif
    
    free_irq(5, (void *)RPC_ID);

    PDEBUG("Goodbye, RPC~\n");
}

// Constructor of this module.
int RPC_init_module(void)
{
    int result; 
#ifdef CONFIG_DEVFS_FS
    int i, j, k;
#ifndef KERNEL2_6
    devfs_handle_t *ptr;
#endif
#endif
	printk("size of RPC_POLL_Dev %d and RPC_INTR_Dev %d...\n", sizeof(RPC_POLL_Dev), sizeof(RPC_INTR_Dev));

//    SET_MODULE_OWNER(&rpc_poll_fops);
//    SET_MODULE_OWNER(&rpc_intr_fops);
    PDEBUG("Hello, RPC~\n");
#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
    /* If we have devfs, create /dev/rpc to put files in there */
    rpc_devfs_dir = devfs_mk_dir(NULL, "rpc", NULL);
    if (!rpc_devfs_dir) return -EBUSY; /* problem */
#else
    /* create /dev/rpc to put files in there */
    devfs_mk_dir("rpc");

    /* register rpc_poll_fops as default file operation */
    result = register_chrdev(rpc_major, rpc_name, &rpc_poll_fops);
    if (result < 0) {
        PDEBUG("Can't get major %d\n", RPC_MAJOR);
        return result;
    }
    if (rpc_major == 0) rpc_major = result; /* dynamic */
    PDEBUG("RPC major number: %d\n", rpc_major);
#endif
#else /* no devfs, do it the "classic" way */
    /* register rpc_poll_fops as default file operation */
    result = register_chrdev(rpc_major, rpc_name, &rpc_poll_fops);
    if (result < 0) {
        PDEBUG("Can't get major %d\n", RPC_MAJOR);
        return result;
    }
    if (rpc_major == 0) rpc_major = result; /* dynamic */
    PDEBUG("RPC major number: %d\n", rpc_major);
#endif /* CONFIG_DEVFS_FS */
    
    /* At this point call the init function for any kind device */
    if ( (result = rpc_poll_init()) )
        goto fail;
    if ( (result = rpc_intr_init()) )
        goto fail;

#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
    // Create corresponding node in device file system
    for (i = 0; i < RPC_NR_DEVS; i++) {
    	j = i/RPC_NR_PAIR;  // ordinal number
    	k = i%RPC_NR_PAIR;  // device kind
        sprintf(devname, "%i", i);
        ptr = (devfs_handle_t *)(*rpc_data_ptr_array[k]+rpc_data_size_array[k]*j);
//      PDEBUG("   ***ptr[%d] = 0x%8x\n", i, (int)ptr);
        *ptr = devfs_register(rpc_devfs_dir, devname,
                       DEVFS_FL_AUTO_DEVNUM,
                       0, 0, S_IFCHR | S_IRUGO | S_IWUGO,
                       rpc_fop_array[k],
                       ptr
                       );
    }
#else
    // Create corresponding node in device file system
    for (i = 0; i < RPC_NR_DEVS; i++) {
        devfs_mk_cdev(MKDEV(rpc_major, i), S_IFCHR|S_IRUSR|S_IWUSR, "rpc/%d", i);
    }
    devfs_mk_cdev(MKDEV(rpc_major, 100), S_IFCHR|S_IRUSR|S_IWUSR, "rpc/100");
#endif
#endif
        
    if (request_irq(5, rpc_isr, SA_SHIRQ, "rpc", (void *)RPC_ID) != 0)
	printk("Can't get assigned irq...\n");

    // Enable the interrupt from system to audio & video
    writel(0x7, (void *)0xb801a108);

    *((int *)0xa00000d0) = 0xffffffff;
    *((int *)0xa00000d4) = 0xffffffff;

    return 0; /* succeed */
    
fail:
    PDEBUG("RPC error number: %d\n", result);
    RPC_cleanup_module();
    return result;
}

module_init(RPC_init_module);
module_exit(RPC_cleanup_module);
