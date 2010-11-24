/*
 * $Id: RPCpoll.c,v 1.10 2004/8/4 09:25 Jacky Exp $
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
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */

#include <asm/system.h>     /* cli(), *_flags */
#include <asm/uaccess.h>    /* copy_to_user() copy_from_user() */

#include "RPCDriver.h"

RPC_POLL_Dev *rpc_poll_devices;

int rpc_poll_init(void)
{
	int result = 0, num, i;
	
	num = RPC_NR_DEVS/RPC_NR_PAIR;
	
    // Create corresponding structures for each device.
    rpc_poll_devices = (RPC_POLL_Dev *)RPC_POLL_RECORD_ADDR;
//    rpc_poll_devices = kmalloc(num * sizeof(RPC_POLL_Dev), GFP_KERNEL);
//    if (!rpc_poll_devices) {
//        result = -ENOMEM;
//        goto fail;
//    }
//    memset(rpc_poll_devices, 0, num * sizeof(RPC_POLL_Dev));
//  PDEBUG("   ***rpc_poll_device[0] = 0x%8x\n", (int)&rpc_poll_devices[0]);
//  PDEBUG("   ***rpc_poll_device[0] = 0x%8x\n", (int)&rpc_poll_devices[1]);

    for (i = 0; i < num; i++) {
        PDEBUG("rpc_poll_device %d addr: %x \n", i, (unsigned)&rpc_poll_devices[i]);
        rpc_poll_devices[i].ringBuf = (char *)(RPC_POLL_DEV_ADDR+i*RPC_RING_SIZE*2);
//        rpc_poll_devices[i].ringBuf = kmalloc(RPC_RING_SIZE, GFP_KERNEL);
//        if (!rpc_poll_devices[i].ringBuf) {
//            result = -ENOMEM;
//            goto fail;
//        }

        // Initialize pointers...
        rpc_poll_devices[i].ringStart = rpc_poll_devices[i].ringBuf;
        rpc_poll_devices[i].ringEnd = rpc_poll_devices[i].ringBuf+RPC_RING_SIZE;
        rpc_poll_devices[i].ringIn = rpc_poll_devices[i].ringBuf;
        rpc_poll_devices[i].ringOut = rpc_poll_devices[i].ringBuf;
        
        PDEBUG("The %dth RPC_POLL_Dev:\n", i);
        PDEBUG("RPC ringStart: 0x%8x\n", (int)rpc_poll_devices[i].ringStart);
        PDEBUG("RPC ringEnd:   0x%8x\n", (int)rpc_poll_devices[i].ringEnd);
        PDEBUG("RPC ringIn:    0x%8x\n", (int)rpc_poll_devices[i].ringIn);
        PDEBUG("RPC ringOut:   0x%8x\n", (int)rpc_poll_devices[i].ringOut);
        PDEBUG("\n");

        // Initialize sempahores...
        sema_init(&rpc_poll_devices[i].readSem, 1);
        sema_init(&rpc_poll_devices[i].writeSem, 1);
	}
	
fail:
	return result;
}

void rpc_poll_cleanup(void)
{
	int num = RPC_NR_DEVS/RPC_NR_PAIR, i;

    // Clean corresponding structures for each device.
    if (rpc_poll_devices) {
    	// Clean ring buffers.
    	for (i = 0; i < num; i++) {
//     	    if (rpc_poll_devices[i].ringBuf)
//            	kfree(rpc_poll_devices[i].ringBuf);
#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
        	devfs_unregister(rpc_poll_devices[i].handle);
#endif
#endif
    	}

//        kfree(rpc_poll_devices);
    }

    return;
}

int rpc_poll_open(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
	int minor = MINOR(inode->i_rdev)-1;
#endif
#endif
	int minor = MINOR(inode->i_rdev);

    if (minor == 100) {
        filp->f_op = &rpc_ctrl_fops;
        return filp->f_op->open(inode, filp); /* dispatch to specific open */
    }
    /*
     * If private data is not valid, we are not using devfs
     * so use the minor number to select a new f_op
     */
    if (!filp->private_data && (minor%RPC_NR_PAIR != 0)) {
        filp->f_op = rpc_fop_array[minor%RPC_NR_PAIR];
        return filp->f_op->open(inode, filp); /* dispatch to specific open */
    }

    PDEBUG("RPC poll open with minor number: %d\n", minor);

	if (!filp->private_data) {
		filp->private_data = &rpc_poll_devices[minor/RPC_NR_PAIR];
	}

//    MOD_INC_USE_COUNT;  /* Before we maybe sleep */

    return 0;          /* success */
}

int rpc_poll_release(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
	int minor = MINOR(inode->i_rdev)-1;
#endif
#endif
	int minor = MINOR(inode->i_rdev);

    PDEBUG("RPC poll close with minor number: %d\n", minor);

//    MOD_DEC_USE_COUNT;
    
    return 0;          /* success */
}

// We don't need parameter f_pos here...
ssize_t rpc_poll_read(struct file *filp, char *buf, size_t count,
                loff_t *f_pos)
{
    RPC_POLL_Dev *dev = filp->private_data; /* the first listitem */
    int temp, size;
    ssize_t ret = 0;
    char *ptmp;

    if (down_interruptible(&dev->readSem))
        return -ERESTARTSYS;

    if (dev->ringIn == dev->ringOut)
        goto out;   // the ring is empty...
    else if (dev->ringIn > dev->ringOut)
        size = dev->ringIn - dev->ringOut;
    else
        size = RPC_RING_SIZE + dev->ringIn - dev->ringOut;

	if (count > size)
		count = size;
	
	temp = dev->ringEnd - dev->ringOut;
	if (temp >= count) {
		if (my_copy_user((int *)buf, (int *)dev->ringOut, count)) {
        	ret = -EFAULT;
			goto out;
		}
		ret += count;
		ptmp = dev->ringOut + ((count+3) & 0xfffffffc);
		if (ptmp == dev->ringEnd)
			dev->ringOut = dev->ringStart;
		else
			dev->ringOut = ptmp;
    	
    	PDEBUG("RPC Read is in 1st kind...\n");
	} else {
		if (my_copy_user((int *)buf, (int *)dev->ringOut, temp)) {
        	ret = -EFAULT;
			goto out;
		}
		count -= temp;
		
		if (my_copy_user((int *)(buf+temp), (int *)dev->ringStart, count)) {
        	ret = -EFAULT;
			goto out;
		}
		ret += (temp + count);
		dev->ringOut = dev->ringStart+((count+3) & 0xfffffffc);
    	
    	PDEBUG("RPC Read is in 2nd kind...\n");
	}
out:
    PDEBUG("RPC poll ringOut pointer is : 0x%8x\n", (int)dev->ringOut);
    up(&dev->readSem);
    return ret;
}

// We don't need parameter f_pos here...
ssize_t rpc_poll_write(struct file *filp, const char *buf, size_t count,
                loff_t *f_pos)
{
    RPC_POLL_Dev *dev = filp->private_data; /* the first listitem */
    int temp, size;
    ssize_t ret = 0;
    char *ptmp;

    if (down_interruptible(&dev->writeSem))
        return -ERESTARTSYS;

    if (dev->ringIn == dev->ringOut)
        size = 0;   // the ring is empty
    else if (dev->ringIn > dev->ringOut)
        size = dev->ringIn - dev->ringOut;
    else
        size = RPC_RING_SIZE + dev->ringIn - dev->ringOut;

	if (count > (RPC_RING_SIZE - size - 1))
		goto out;

	temp = dev->ringEnd - dev->ringIn;
	if (temp >= count) {
		if (my_copy_user((int *)dev->ringIn, (int *)buf, count)) {
        	ret = -EFAULT;
			goto out;
		}
		ret += count;
		ptmp = dev->ringIn + ((count+3) & 0xfffffffc);

		__asm__ __volatile__ ("sync;");

		if (ptmp == dev->ringEnd)
			dev->ringIn = dev->ringStart;
		else
			dev->ringIn = ptmp;
    	
    	PDEBUG("RPC Write is in 1st kind...\n");
	} else {
		if (my_copy_user((int *)dev->ringIn, (int *)buf, temp)) {
        	ret = -EFAULT;
			goto out;
		}
		count -= temp;
		
		if (my_copy_user((int *)dev->ringStart, (int *)(buf+temp), count)) {
        	ret = -EFAULT;
			goto out;
		}
		ret += (temp + count);

		__asm__ __volatile__ ("sync;");

		dev->ringIn = dev->ringStart+((count+3) & 0xfffffffc);
    	
    	PDEBUG("RPC Write is in 2nd kind...\n");
	}

out:
    PDEBUG("RPC poll ringIn pointer is : 0x%8x\n", (int)dev->ringIn);
    up(&dev->writeSem);
    return ret;
}

int rpc_poll_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	
	return ret;
}

int rpc_ctrl_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	if (cmd == RPC_IOCTRESET) {
		printk("[RPC]start reset...\n");
		rpc_poll_init();
		rpc_intr_init();

		*((int *)0xb801a104) = 0x0000003e;
		*((int *)0xa00000d0) = 0xffffffff;
		*((int *)0xa00000d4) = 0xffffffff;
		printk("[RPC]done...\n");
	} else {
		printk("[RPC]: error ioctl command...\n");
	}

	return ret;
}

int rpc_ctrl_open(struct inode *inode, struct file *filp)
{
	printk("[RPC]open for RPC ioctl...\n");
	
	return 0;          /* success */
}

struct file_operations rpc_poll_fops = {
//    llseek:     scull_llseek,
    ioctl:      rpc_poll_ioctl,
    read:       rpc_poll_read,
    write:      rpc_poll_write,
    open:       rpc_poll_open,
    release:    rpc_poll_release,
};

struct file_operations rpc_ctrl_fops = {
    ioctl:      rpc_ctrl_ioctl,
    open:       rpc_ctrl_open,
};

