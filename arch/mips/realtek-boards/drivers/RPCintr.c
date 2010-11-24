/*
 * $Id: RPCintr.c,v 1.10 2004/8/4 09:25 Jacky Exp $
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

#include <asm/io.h>
#include <asm/system.h>     /* cli(), *_flags */
#include <asm/uaccess.h>    /* copy_to_user() copy_from_user() */

#include "RPCDriver.h"

RPC_INTR_Dev *rpc_intr_devices;

int timeout = HZ/40; // jiffies

int rpc_intr_init(void)
{
	int result = 0, num, i;
	
	num = RPC_NR_DEVS/RPC_NR_PAIR;
	
    // Create corresponding structures for each device.
    rpc_intr_devices = (RPC_INTR_Dev *)RPC_INTR_RECORD_ADDR;
//    rpc_intr_devices = kmalloc(num * sizeof(RPC_INTR_Dev), GFP_KERNEL);
//    if (!rpc_intr_devices) {
//        result = -ENOMEM;
//        goto fail;
//    }
//    memset(rpc_intr_devices, 0, num * sizeof(RPC_INTR_Dev));
//  PDEBUG("   ***rpc_intr_device[0] = 0x%8x\n", (int)&rpc_intr_devices[0]);
//  PDEBUG("   ***rpc_intr_device[0] = 0x%8x\n", (int)&rpc_intr_devices[1]);

    for (i = 0; i < num; i++) {
        PDEBUG("rpc_intr_device %d addr: %x \n", i, (unsigned)&rpc_intr_devices[i]);
        rpc_intr_devices[i].ringBuf = (char *)(RPC_INTR_DEV_ADDR+i*RPC_RING_SIZE*2);
//        rpc_intr_devices[i].ringBuf = kmalloc(RPC_RING_SIZE, GFP_KERNEL);
//        if (!rpc_intr_devices[i].ringBuf) {
//            result = -ENOMEM;
//            goto fail;
//        }

        // Initialize pointers...
        rpc_intr_devices[i].ringStart = rpc_intr_devices[i].ringBuf;
        rpc_intr_devices[i].ringEnd = rpc_intr_devices[i].ringBuf+RPC_RING_SIZE;
        rpc_intr_devices[i].ringIn = rpc_intr_devices[i].ringBuf;
        rpc_intr_devices[i].ringOut = rpc_intr_devices[i].ringBuf;
        
        PDEBUG("The %dth RPC_INTR_Dev:\n", i);
        PDEBUG("RPC ringStart: 0x%8x addr: 0x%8x\n", (int)rpc_intr_devices[i].ringStart, (int)&rpc_intr_devices[i].ringStart);
        PDEBUG("RPC ringEnd:   0x%8x addr: 0x%8x\n", (int)rpc_intr_devices[i].ringEnd, (int)&rpc_intr_devices[i].ringEnd);
        PDEBUG("RPC ringIn:    0x%8x addr: 0x%8x\n", (int)rpc_intr_devices[i].ringIn, (int)&rpc_intr_devices[i].ringIn);
        PDEBUG("RPC ringOut:   0x%8x addr: 0x%8x\n", (int)rpc_intr_devices[i].ringOut, (int)&rpc_intr_devices[i].ringOut);
        PDEBUG("\n");

        // Initialize wait queue...
        init_waitqueue_head(&(rpc_intr_devices[i].waitQueue));
		
        // Initialize sempahores...
        sema_init(&rpc_intr_devices[i].readSem, 1);
        sema_init(&rpc_intr_devices[i].writeSem, 1);
	}
	
fail:
	return result;
}

void rpc_intr_cleanup(void)
{
    int num = RPC_NR_DEVS/RPC_NR_PAIR, i;

    // Clean corresponding structures for each device.
    if (rpc_intr_devices) {
    	// Clean ring buffers.
    	for (i = 0; i < num; i++) {
//          if (rpc_intr_devices[i].ringBuf)
//              kfree(rpc_intr_devices[i].ringBuf);
#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
        	devfs_unregister(rpc_intr_devices[i].handle);
#endif
#endif
    	}

//        kfree(rpc_intr_devices);
    }

    return;
}

int rpc_intr_open(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
	int minor = MINOR(inode->i_rdev)-1;
#endif
#endif
	int minor = MINOR(inode->i_rdev);

    PDEBUG("RPC intr open with minor number: %d\n", minor);

	if (!filp->private_data) {
		filp->private_data = &rpc_intr_devices[minor/RPC_NR_PAIR];
		filp->f_pos = (loff_t)minor;
	}

//    MOD_INC_USE_COUNT; /* Before we maybe sleep */

    return 0;          /* success */
}

int rpc_intr_release(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
	int minor = MINOR(inode->i_rdev)-1;
#endif
#endif
	int minor = MINOR(inode->i_rdev);

    PDEBUG("RPC intr close with minor number: %d\n", minor);

//    MOD_DEC_USE_COUNT;
    
    return 0;          /* success */
}

// We don't need parameter f_pos here...
// note: rpc_intr_read support both blocking & nonblocking modes
ssize_t rpc_intr_read(struct file *filp, char *buf, size_t count,
                loff_t *f_pos)
{
    RPC_INTR_Dev *dev = filp->private_data; /* the first listitem */
    int temp, size;
    ssize_t ret = 0;
    char *ptmp;

    if (down_interruptible(&dev->readSem))
        return -ERESTARTSYS;
	
	if (dev->ringIn == dev->ringOut) {
		if (filp->f_flags & O_NONBLOCK)
			goto out;
wait_again:
		if (wait_event_interruptible_timeout(dev->waitQueue, dev->ringIn != dev->ringOut, timeout) == 0)
			goto out; /* timeout */

		if (current->flags & PF_FREEZE) {
			refrigerator(PF_FREEZE);
			if (!signal_pending(current))
				goto wait_again;
		}

		if (signal_pending(current)) {
			PDEBUG("RPC deblock because of receiving a signal...\n");
			goto out;
		}
	}
	
    if (dev->ringIn > dev->ringOut)
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
    PDEBUG("RPC intr ringOut pointer is : 0x%8x\n", (int)dev->ringOut);
    up(&dev->readSem);
    return ret;
}

// We don't need parameter f_pos here...
// note: rpc_intr_write only support nonblocking mode
ssize_t rpc_intr_write(struct file *filp, const char *buf, size_t count,
                loff_t *f_pos)
{
    RPC_INTR_Dev *dev = filp->private_data; /* the first listitem */
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

	// notify all the processes in the wait queue
//	wake_up_interruptible(&dev->waitQueue);
	temp = (int)*f_pos;	/* use the "f_pos" of file object to store the device number */
	if (temp == 1)
		writel(0x3, (void *)0xb801a104);	// audio
	else if (temp == 5)
		writel(0x5, (void *)0xb801a104);	// video
	else
		printk("error device number...\n");
	
out:
    PDEBUG("RPC intr ringIn pointer is : 0x%8x\n", (int)dev->ringIn);
    up(&dev->writeSem);
    return ret;
}

int rpc_intr_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	int ret = 0;

    switch(cmd) {
    	case RPC_IOCTTIMEOUT:
	        timeout = arg;
	        break;
    	case RPC_IOCQTIMEOUT:
    		return timeout;
		default:  /* redundant, as cmd was checked against MAXNR */
        	return -ENOTTY;
	}
	
	return ret;
}

struct file_operations rpc_intr_fops = {
//    llseek:     scull_llseek,
    ioctl:      rpc_intr_ioctl,
    read:       rpc_intr_read,
    write:      rpc_intr_write,
    open:       rpc_intr_open,
    release:    rpc_intr_release,
};

