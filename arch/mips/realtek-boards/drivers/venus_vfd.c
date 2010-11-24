#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* DBG_PRINT() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/device.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/devfs_fs_kernel.h> /* for devfs */

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/interrupt.h>

#include <asm/bitops.h>		/* bit operations */
#include <asm/delay.h>

#include "venus_vfd.h"

/* Module Variables */
/*
 * Our parameters which can be set at load time.
 */
#define ioread32 readl
#define iowrite32 writel

#define FIFO_DEPTH	256		/* driver queue depth */
//#define DEV_DEBUG

static DECLARE_WAIT_QUEUE_HEAD(venus_vfd_keypad_read_wait);
static DECLARE_WAIT_QUEUE_HEAD(venus_vfd_write_done_wait);
static struct kfifo *vfd_keypad_fifo;

/* Major Number + Minor Number */
static dev_t dev_venus_vfd = 0;

static struct cdev *venus_vfd_vfdo_cdev 	= NULL;
static struct cdev *venus_vfd_wrctl_cdev 	= NULL;
static struct cdev *venus_vfd_keypad_cdev 	= NULL;

spinlock_t venus_vfd_lock 	= SPIN_LOCK_UNLOCKED;

MODULE_AUTHOR("Chih-pin Wu, Realtek Semiconductor");
MODULE_LICENSE("GPL");

struct venus_vfd_dev *venus_vfd_devices;	/* allocated in scull_init_module */

/* Module Functions */
/* *** ALL INITIALIZATION HERE *** */

static int venus_vfd_suspend(struct device *dev, pm_message_t state, u32 level) {
	uint32_t regValue;

	/* [KEYPAD] disable whole key matrix = key1-key4, seg1-seg12 interrupt */
	iowrite32(0x00000000, MIS_VFD_KPADLIE);
	iowrite32(0x00000000, MIS_VFD_KPADHIE);

	/* [KEYPAD] Clear MIS_UMSK_ISR_KPADAH/MIS_UMSK_ISR_KPADAL */
	iowrite32(0xfffffff0, MIS_UMSK_ISR_KPADAH);
	iowrite32(0xfffffff0, MIS_UMSK_ISR_KPADAL);

	return 0;
}

static int venus_vfd_resume(struct device *dev, u32 level) {
	/* [KEYPAD] enable whole key matrix = key1-key4, seg1-seg12 interrupt */
	iowrite32(0xffffffff, MIS_VFD_KPADLIE);
	iowrite32(0x0000ffff, MIS_VFD_KPADHIE);

	return 0;
}

static struct platform_device *venus_vfd_devs;
static struct device_driver venus_vfd_driver = {
    .name       = "VenusVFD",
    .bus        = &platform_bus_type,
    .suspend    = venus_vfd_suspend,
    .resume     = venus_vfd_resume,
};

static irqreturn_t VFD_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs) {
	int kfifoAdded = 0;
	int firstBitNr;
	char keyNr;
	uint32_t regValue;
	uint32_t statusRegValue;

	regValue = ioread32(MIS_ISR);

	/* check if the interrupt belongs to us */
	if(regValue & 0x0007c000) {
		if(regValue & 0x00040000) {	/* SW dis-assert [DONT CARE] */
			iowrite32(0x00040000, MIS_ISR); /* clear interrupt flag */
		}
		
		if(regValue & 0x00020000) { /* SW assert */
			statusRegValue = ioread32(MIS_UMSK_ISR_SW) & 0xf0;
			while((firstBitNr = ffs(statusRegValue)) != 0) {
#ifdef DEV_DEBUG
			printk(KERN_WARNING "Venus VFD: SwitchNr[%d].\n", firstBitNr-1-4);
#endif
				kfifoAdded++;
				keyNr = (char)firstBitNr - 1 - 4 + VENUS_VFD_SWITCH_BASE;
				__kfifo_put(vfd_keypad_fifo, &keyNr, sizeof(char));
				clear_bit(firstBitNr-1, (unsigned long *)&statusRegValue);
			}
			iowrite32(0x00000ff0, MIS_UMSK_ISR_SW);
			iowrite32(0x00020000, MIS_ISR); /* clear interrupt flag */
		}
		
		if(regValue & 0x00010000) { /* Keypad dis-assert [DONT CARE] */
			iowrite32(0x00fffff0, MIS_UMSK_ISR_KPADDAH);
			iowrite32(0xfffffff0, MIS_UMSK_ISR_KPADDAL);
			iowrite32(0x00010000, MIS_ISR); /* clear interrupt flag */
		}
		
		if(regValue & 0x00008000) { /* Keypad assert */
			/* scan for lower 28 bits */
			statusRegValue = ioread32(MIS_UMSK_ISR_KPADAL) & 0xfffffff0;
			while((firstBitNr = ffs(statusRegValue)) != 0) {
#ifdef DEV_DEBUG
printk(KERN_WARNING "Venus VFD: KeyNr[%d].\n", firstBitNr-1-4);
#endif
				kfifoAdded++;
				keyNr = (char)firstBitNr - 1 - 4;
				__kfifo_put(vfd_keypad_fifo, &keyNr, sizeof(char));
				clear_bit(firstBitNr-1, (unsigned long *)&statusRegValue);
			}
			iowrite32(0xfffffff0, MIS_UMSK_ISR_KPADAL);

			/* scan for higher 20 bits */
			statusRegValue = ioread32(MIS_UMSK_ISR_KPADAH) & 0x00fffff0;
			while((firstBitNr = ffs(statusRegValue)) != 0) {
#ifdef DEV_DEBUG
printk(KERN_WARNING "Venus VFD: KeyNr[%d].\n", firstBitNr-1-4+28);
#endif
				kfifoAdded++;
				keyNr = (char)firstBitNr - 1 - 4 + 28;
				__kfifo_put(vfd_keypad_fifo, &keyNr, sizeof(char));
				clear_bit(firstBitNr-1, (unsigned long *)&statusRegValue);
			}
			iowrite32(0x00fffff0, MIS_UMSK_ISR_KPADAH);
			iowrite32(0x00008000, MIS_ISR); /* clear interrupt flag */
		}
		
		if(regValue & 0x00004000) { /* write_done [DONT CARE] */
			iowrite32(0x00004000, MIS_ISR); /* clear interrupt flag */
		}

		if(kfifoAdded != 0)
			wake_up_interruptible(&venus_vfd_keypad_read_wait);

		return IRQ_HANDLED;
	}
	else {
		return IRQ_NONE;
	}
}

static void Venus_VFD_Init(void) {
/* Initialize Venus */
	/* VFD Auto Read Control Register
	   6 bytes Keypad, Switch HIGH active, Keypad HIGH Active, 10ms-period Auto Read */
	iowrite32(0x00000630, MIS_VFD_ARDCTL);

	/* [KEYPAD] enable whole key matrix = key1-key4, seg1-seg12 interrupt */
	iowrite32(0xffffffff, MIS_VFD_KPADLIE);
	iowrite32(0x0000ffff, MIS_VFD_KPADHIE);

	/* [SWITCH] by default disable switch0-switch3 interrupt */
	iowrite32(0x00000000, MIS_VFD_SWIE);

	/* Enable VFD Controller (enable kpad, enable switch, 1.037us Duty Cycle, Enable VFD) */
	iowrite32(0x0000000d, MIS_VFD_CTL);

	/* [KEYPAD] Clear Interrupt Flags */
	iowrite32(0x00fffff0, MIS_UMSK_ISR_KPADDAH);
	iowrite32(0xfffffff0, MIS_UMSK_ISR_KPADDAL);
	iowrite32(0x00fffff0, MIS_UMSK_ISR_KPADAH);
	iowrite32(0xfffffff0, MIS_UMSK_ISR_KPADAL);
	return;
}

ssize_t venus_vfd_vfdo_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
	unsigned char *kBuf;

	if(count != 4) 	/* we only support 32bit access to match register width */
		return -EFAULT;

	kBuf = kmalloc(count, GFP_KERNEL);
	if(!kBuf) 
		return -ENOMEM;

	if(copy_from_user(kBuf, buf, count)) {
		kfree(kBuf);
		return -EFAULT;
	}

	iowrite32(*(uint32_t *)kBuf, MIS_VFDO);
#ifdef DEV_DEBUG
	printk(KERN_WARNING "Venus VFD: writing %08X to MIS_VFDO(%08X)\n", *(uint32_t *)kBuf, MIS_VFDO);
#endif
	kfree(kBuf);
	return count;
}

ssize_t venus_vfd_wrctl_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	uint32_t regValue;

	if(count != 4)
		return -EFAULT;

	regValue = ioread32(MIS_VFD_WRCTL);
#ifdef DEV_DEBUG
        printk(KERN_WARNING "Venus VFD: reading %08X from MIS_VFD_WRCTL(%08X)\n", regValue, MIS_VFD_WRCTL);
#endif

	if(copy_to_user(buf, &regValue, count))
		return -EFAULT;

	return count;
}

ssize_t venus_vfd_wrctl_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
	unsigned char *kBuf;

	if(count != 4) 	/* we only support 32bit access to match register width */
		return -EFAULT;

	kBuf = kmalloc(count, GFP_KERNEL);
	if(!kBuf) 
		return -ENOMEM;

	if(copy_from_user(kBuf, buf, count)) {
		kfree(kBuf);
		return -EFAULT;
	}

	iowrite32(*(uint32_t *)kBuf, MIS_VFD_WRCTL);
#ifdef DEV_DEBUG
	printk(KERN_WARNING "Venus VFD: writing %08X to MIS_VFD_WRCTL(%08X)\n", *(uint32_t *)kBuf, MIS_VFD_WRCTL);
#endif

	kfree(kBuf);
	return count;
}

unsigned int venus_vfd_wrctl_poll(struct file *filp, poll_table *wait) {
    return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
}

int venus_vfd_wrctl_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	int err = 0;
	int retval = 0;

	if (_IOC_TYPE(cmd) != VENUS_VFD_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VENUS_VFD_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) 
		return -EFAULT;

	if (!capable (CAP_SYS_ADMIN))
		return -EPERM;

	switch(cmd) {
		case VENUS_VFD_IOC_DISABLE_AUTOREAD:
			iowrite32((ioread32(MIS_VFD_CTL) & 0xfffffff3), MIS_VFD_CTL);
			break;
		case VENUS_VFD_IOC_ENABLE_AUTOREAD:
			iowrite32((ioread32(MIS_VFD_CTL) | 0x0000000c), MIS_VFD_CTL);
			break;
		default:
			retval = -ENOIOCTLCMD;
	}

	return retval;
}


int venus_vfd_keypad_open(struct inode *inode, struct file *filp) {
	/* clear interrupt flag */
	iowrite32(0x00fffff0, MIS_UMSK_ISR_KPADDAH);
	iowrite32(0xfffffff0, MIS_UMSK_ISR_KPADDAL);
	iowrite32(0x00fffff0, MIS_UMSK_ISR_KPADAH);
	iowrite32(0xfffffff0, MIS_UMSK_ISR_KPADAL);

	/* reinitialize kfifo */
	kfifo_reset(vfd_keypad_fifo);

	return 0;	/* success */
}

ssize_t venus_vfd_keypad_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	char byteBuf;
	int i, readCount = count;

restart:
	if(wait_event_interruptible(venus_vfd_keypad_read_wait, __kfifo_len(vfd_keypad_fifo) > 0) != 0) {
		if(unlikely(current->flags & PF_FREEZE)) {
			refrigerator(PF_FREEZE);
			goto restart;
		}
		else 
			return -ERESTARTSYS;
	}

	if(__kfifo_len(vfd_keypad_fifo) < count)
		readCount = __kfifo_len(vfd_keypad_fifo);

	for(i = 0 ; i < readCount ; i++) {
		__kfifo_get(vfd_keypad_fifo, &byteBuf, 1);
		copy_to_user(buf, &byteBuf, 1);
		buf++;
	}

	return readCount;
}

unsigned int venus_vfd_keypad_poll(struct file *filp, poll_table *wait) {
	poll_wait(filp, &venus_vfd_keypad_read_wait, wait);

	if(__kfifo_len(vfd_keypad_fifo) > 0)
		return POLLIN | POLLRDNORM;
	else
		return 0;
}


struct file_operations venus_vfd_vfdo_fops = {
	.owner =    THIS_MODULE,
	.write =    venus_vfd_vfdo_write,
};

struct file_operations venus_vfd_wrctl_fops = {
	.owner =    THIS_MODULE,
	.read  =    venus_vfd_wrctl_read,
	.write =    venus_vfd_wrctl_write,
	.poll  =    venus_vfd_wrctl_poll,
	.ioctl =    venus_vfd_wrctl_ioctl,
};

struct file_operations venus_vfd_keypad_fops = {
	.owner =    THIS_MODULE,
	.open  =    venus_vfd_keypad_open,
	.read  =    venus_vfd_keypad_read,
	.poll  =    venus_vfd_keypad_poll,
};

int venus_vfd_init_module(void) {
	int result;

	/* MKDEV */
	dev_venus_vfd = MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_VFDO);

	/* Request Major Number */
	result = register_chrdev_region(dev_venus_vfd, VENUS_VFD_DEVICE_NUM, "vfdo");
	if(result < 0) {
		printk(KERN_WARNING "Venus VFD: can't register device number.\n");
		goto fail_alloc_dev_vfd;
	}

	/* Initialize kfifo */
	vfd_keypad_fifo = kfifo_alloc(FIFO_DEPTH, GFP_KERNEL, &venus_vfd_lock);
	if(IS_ERR(vfd_keypad_fifo)) {
		result = -ENOMEM;
		goto fail_alloc_kfifo;
	}

	venus_vfd_devs = platform_device_register_simple("VenusVFD", -1, NULL, 0);
	if(driver_register(&venus_vfd_driver) != 0)
		goto fail_device_register;

	/* Request IRQ */
	if(request_irq(VENUS_VFD_IRQ, 
						VFD_interrupt_handler, 
						SA_INTERRUPT|SA_SAMPLE_RANDOM|SA_SHIRQ, 
						"Venus_VFD", 
						VFD_interrupt_handler)) {
		printk(KERN_ERR "VFD: cannot register IRQ %d\n", VENUS_VFD_IRQ);
		result = -EIO;
		goto fail_alloc_irq;
	}


	/* Char Device Registration */
	/* Expose Register MIS_VFDO write interface */
	venus_vfd_vfdo_cdev = cdev_alloc();
	venus_vfd_vfdo_cdev->ops = &venus_vfd_vfdo_fops;
	if(cdev_add(venus_vfd_vfdo_cdev, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_VFDO), 1)) {
		printk(KERN_ERR "Venus VFD: can't add character device for vfd_vfdo\n");
		result = -ENOMEM;
		goto fail_cdev_alloc_vfdo;
	}

	/* Expose Register MIS_VFD_WRCTL read/write interface */
	venus_vfd_wrctl_cdev = cdev_alloc();
	venus_vfd_wrctl_cdev->ops = &venus_vfd_wrctl_fops;
	if(cdev_add(venus_vfd_wrctl_cdev, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_WRCTL), 1)) {
		printk(KERN_ERR "Venus VFD: can't add character device for vfd_wrctl\n");
		result = -ENOMEM;
		goto fail_cdev_alloc_wrctl;
	}

	/* Keystroke interface on VFD Controller */
	venus_vfd_keypad_cdev = cdev_alloc();
	venus_vfd_keypad_cdev->ops = &venus_vfd_keypad_fops;
	if(cdev_add(venus_vfd_keypad_cdev, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_KEYPAD), 1)) {
		printk(KERN_ERR "Venus VFD: can't add character device for vfd_wrctl\n");
		result = -ENOMEM;
		goto fail_cdev_alloc_keypad;
	}

	/* use devfs to create device file */
	devfs_mk_cdev(MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_VFDO), S_IFCHR|S_IRUSR|S_IWUSR, VENUS_VFD_VFDO_DEVICE);
	devfs_mk_cdev(MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_WRCTL), S_IFCHR|S_IRUSR|S_IWUSR, VENUS_VFD_WRCTL);
	devfs_mk_cdev(MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_KEYPAD), S_IFCHR|S_IRUSR|S_IWUSR, VENUS_VFD_KEYPAD);

	/* Hardware Registers Initialization */
	Venus_VFD_Init();

	return 0;	/* succeed ! */

fail_cdev_alloc_keypad:
	cdev_del(venus_vfd_wrctl_cdev);
fail_cdev_alloc_wrctl:
	cdev_del(venus_vfd_vfdo_cdev);
fail_cdev_alloc_vfdo:
	free_irq(VENUS_VFD_IRQ, VFD_interrupt_handler);
fail_alloc_irq:
	kfifo_free(vfd_keypad_fifo);
fail_device_register:
	platform_device_unregister(venus_vfd_devs);
	driver_unregister(&venus_vfd_driver);
fail_alloc_kfifo:
	unregister_chrdev_region(dev_venus_vfd, VENUS_VFD_DEVICE_NUM);
fail_alloc_dev_vfd:
	return result;
}

void venus_vfd_cleanup_module(void) {
	/* Reset Hardware Registers */
	/* [KEYPAD] enable key1, seg1-seg8 interrupt, and disable other keys */
	iowrite32(0x0, MIS_VFD_KPADLIE);
	iowrite32(0x0, MIS_VFD_KPADHIE);

	/* Venus: Disable VFD Controller */
	iowrite32(0x0, MIS_VFD_CTL);

	/* delete device file by devfs */
	devfs_remove(VENUS_VFD_VFDO_DEVICE);
	devfs_remove(VENUS_VFD_WRCTL);
	devfs_remove(VENUS_VFD_KEYPAD);
	
	/* Release Character Device Driver */
	cdev_del(venus_vfd_keypad_cdev);
	cdev_del(venus_vfd_wrctl_cdev);
	cdev_del(venus_vfd_vfdo_cdev);

	/* Free IRQ Handler */
	free_irq(VENUS_VFD_IRQ, VFD_interrupt_handler);

	/* Free Kernel FIFO */
	kfifo_free(vfd_keypad_fifo);

	/* device driver removal */
	platform_device_unregister(venus_vfd_devs);
	driver_unregister(&venus_vfd_driver);

	/* Return Major Numbers */
	unregister_chrdev_region(dev_venus_vfd, VENUS_VFD_DEVICE_NUM);
}

/* Register Macros */

module_init(venus_vfd_init_module);
module_exit(venus_vfd_cleanup_module);
