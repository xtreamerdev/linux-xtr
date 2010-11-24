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
#include <linux/jiffies.h>
#include <linux/device.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/devfs_fs_kernel.h> /* for devfs */
#include <linux/time.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/interrupt.h>

#include <asm/bitops.h>		/* bit operations */
#include "venus_ir.h"

/* Module Variables */
/*
 * Our parameters which can be set at load time.
 */

//#define DEV_DEBUG
#define FIFO_DEPTH	16		/* driver queue depth */

#define ioread32 readl
#define iowrite32 writel

static int ir_protocol = 1;
static unsigned int lastRecvMs;
static unsigned int debounce = 300;
module_param(ir_protocol, int, S_IRUGO);

enum {
	NEC = 1,
	RC5 = 2,
	SHARP = 3,
	SONY = 4,
	C03 = 5,
	RC6 = 6
};

static DECLARE_WAIT_QUEUE_HEAD(venus_ir_read_wait);
static struct kfifo *venus_ir_fifo;

/* Major Number + Minor Number */
static dev_t dev_venus_ir = 0;

static struct cdev *venus_ir_cdev 	= NULL;

spinlock_t venus_ir_lock 	= SPIN_LOCK_UNLOCKED;

MODULE_AUTHOR("Chih-pin Wu, Realtek Semiconductor");
MODULE_LICENSE("GPL");

struct venus_ir_dev *venus_ir_devices;	/* allocated in scull_init_module */

/* Module Functions */

static int examine_ir_avail(uint32_t *regValue, int *dataRepeat) {
	uint32_t srValue = ioread32(MIS_IR_SR);
	if(srValue & 0x1) {
		if(srValue & 0x2)
			*dataRepeat = 1;
		else
			*dataRepeat = 0;

		iowrite32(0x00000003, MIS_IR_SR); /* clear IRDVF */
		*regValue = ioread32(MIS_IR_RP); /* read IRRP */
		return 0;
	}
	else
		return -ERESTARTSYS;
}

static irqreturn_t IR_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs) {
	int dataRepeat;
	uint32_t regValue;

	regValue = ioread32(MIS_ISR);

	/* check if the interrupt belongs to us */
	if(regValue & 0x00000020) {

#ifdef DEV_DEBUG
		printk(KERN_WARNING "Venus IR: Interrupt Handler Triggered.\n");
#endif
		iowrite32(0x00000020, MIS_ISR); /* clear interrupt flag */

pick_one_more:
		while(examine_ir_avail(&regValue, &dataRepeat) == 0) {
			if(dataRepeat == 1 && (jiffies_to_msecs(jiffies)- lastRecvMs) < debounce) {
#ifdef DEV_DEBUG
				printk(KERN_WARNING "Venus IR: %dms, ignored..\n", jiffies_to_msecs(jiffies) - lastRecvMs);
#endif
				lastRecvMs = jiffies_to_msecs(jiffies);
			}
			else {
#ifdef DEV_DEBUG
				printk(KERN_WARNING "Venus IR: Non-repeated frame [%dms]\n", jiffies_to_msecs(jiffies) - lastRecvMs);
				printk(KERN_WARNING "Venus IR: IRRP = [%08X].\n", regValue);
#endif
				lastRecvMs = jiffies_to_msecs(jiffies);
				if(ir_protocol == RC6)
					regValue = (regValue >> 10) | 0x40; // drop 10 bits from LSB, bit set for trailer bit
				__kfifo_put(venus_ir_fifo, (unsigned char *)&regValue, sizeof(uint32_t));
				wake_up_interruptible(&venus_ir_read_wait);
			}
		}
		return IRQ_HANDLED;
	}
	else {
		return IRQ_NONE;
	}
}

/* *** ALL INITIALIZATION HERE *** */
static void Venus_IR_Init(int mode) {
	/* Initialize Venus IR Registers*/
	lastRecvMs = jiffies_to_msecs(jiffies);

	/* using HWSD parameters */
	switch(mode) {
		case NEC:
			iowrite32(0x80000000, MIS_IR_CR);
			iowrite32(0x5a13133b, MIS_IR_PSR);
			iowrite32(0x0001162d, MIS_IR_PER);
			iowrite32(0x21b, MIS_IR_SF);	// 50KHz
			iowrite32(0x9c4, MIS_IR_DPIR); // 50ms
			iowrite32(0x5df, MIS_IR_CR);
			break;
		case RC5:
			iowrite32(0x80000000, MIS_IR_CR);
			iowrite32(0x00242424, MIS_IR_PSR);
			iowrite32(0x00010000, MIS_IR_PER);
			iowrite32(0x21b, MIS_IR_SF);
			iowrite32(0x3ff, MIS_IR_DPIR);
			iowrite32(0x50c, MIS_IR_CR);
			break;
		case SHARP:
			iowrite32(0x80000000, MIS_IR_CR);
			iowrite32(0x000c1b54, MIS_IR_PSR);
			iowrite32(0x00010000, MIS_IR_PER);
			iowrite32(0x21b, MIS_IR_SF);
			iowrite32(0xc8, MIS_IR_DPIR);
			iowrite32(0x58e, MIS_IR_CR);
			break;
		case SONY:
			iowrite32(0x80000000, MIS_IR_CR);
			iowrite32(0x18181830, MIS_IR_PSR);
			iowrite32(0x00010006, MIS_IR_PER);
			iowrite32(0x21b, MIS_IR_SF);
			iowrite32(0x1f4, MIS_IR_DPIR);
			iowrite32(0xdd3, MIS_IR_CR);
			break;
		case C03:
			iowrite32(0x80000000, MIS_IR_CR);
			iowrite32(0x25151034, MIS_IR_PSR);
			iowrite32(0x00000025, MIS_IR_PER);
			iowrite32(0x21b, MIS_IR_SF);
			iowrite32(0x3ff, MIS_IR_DPIR);
			iowrite32(0x5ff, MIS_IR_CR);
			break;
		case RC6:
			iowrite32(0x80000000, MIS_IR_CR);
			iowrite32(0x1c0e0e0e, MIS_IR_PSR);
			iowrite32(0x00030008, MIS_IR_PER);
			iowrite32(0x21b, MIS_IR_SF);
			iowrite32(0x3ff, MIS_IR_DPIR);
			iowrite32(0x82a3, MIS_DUMMY); // RC6_EN & TB = 0x23 = 35*20 = 700us
			iowrite32(0x715, MIS_IR_CR); // valid bit length is 21
			break;
	}
	return;
}

static int venus_ir_suspend(struct device *dev, pm_message_t state, u32 level) {
	uint32_t regValue;

	// flush IRRP
	while(ioread32(MIS_IR_SR) & 0x1) {
		iowrite32(0x00000003, MIS_IR_SR); /* clear IRDVF */
		regValue = ioread32(MIS_IR_RP); /* read IRRP */
	}

	// disable interrupt
	regValue = ioread32(MIS_IR_CR);
	regValue = regValue & ~(0x400);
	iowrite32(regValue, MIS_IR_CR);

	return 0;
}

static int venus_ir_resume(struct device *dev, u32 level) {
	uint32_t regValue;

	// enable interrupt
	regValue = ioread32(MIS_IR_CR);
	regValue = regValue | 0x400;
	iowrite32(regValue, MIS_IR_CR);

	// flush IRRP
	while(ioread32(MIS_IR_SR) & 0x1) {
		iowrite32(0x00000003, MIS_IR_SR); /* clear IRDVF */
		regValue = ioread32(MIS_IR_RP); /* read IRRP */
	}

	return 0;
}

static struct platform_device *venus_ir_devs;

static struct device_driver venus_ir_driver = {
    .name       = "VenusIR",
    .bus        = &platform_bus_type,
    .suspend    = venus_ir_suspend,
    .resume     = venus_ir_resume,
};

int venus_ir_open(struct inode *inode, struct file *filp) {
	/* reinitialize kfifo */
	kfifo_reset(venus_ir_fifo);

	return 0;	/* success */
}

ssize_t venus_ir_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	int uintBuf;
	int i, readCount = count;

restart:
	if(wait_event_interruptible(venus_ir_read_wait, __kfifo_len(venus_ir_fifo) > 0) != 0) {
		if(unlikely(current->flags & PF_FREEZE)) {
			refrigerator(PF_FREEZE);
			goto restart;
		}
		else
			return -ERESTARTSYS;
	}

	if(__kfifo_len(venus_ir_fifo) < count)
		readCount = __kfifo_len(venus_ir_fifo);

	for(i = 0 ; i < readCount ; i+=4) {
		__kfifo_get(venus_ir_fifo, &uintBuf, sizeof(uint32_t));
		copy_to_user(buf, &uintBuf, 4);
	}

	return readCount;
}

unsigned int venus_ir_poll(struct file *filp, poll_table *wait) {
	if(__kfifo_len(venus_ir_fifo) > 0)
		return POLLIN | POLLRDNORM;
	else {
#if 0
		uint32_t regValue;
		int dataRepeat;
		if(examine_ir_avail(&regValue, &dataRepeat) == 0) {
			__kfifo_put(venus_ir_fifo, (unsigned char *)&regValue, sizeof(uint32_t));
			wake_up_interruptible(&venus_ir_read_wait);
			return POLLIN | POLLRDNORM;
		}
		else
#endif
			return 0;
	}
}

int venus_ir_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	int err = 0;
	int retval = 0;

	if (_IOC_TYPE(cmd) != VENUS_IR_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VENUS_IR_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) 
		return -EFAULT;

	if (!capable (CAP_SYS_ADMIN))
		return -EPERM;

	switch(cmd) {
		case VENUS_IR_IOC_SET_IRPSR:
			iowrite32(arg, MIS_IR_PSR);
			break;
		case VENUS_IR_IOC_SET_IRPER:
			iowrite32(arg, MIS_IR_PER);
			break;
		case VENUS_IR_IOC_SET_IRSF:
			iowrite32(arg, MIS_IR_SF);
			break;
		case VENUS_IR_IOC_SET_IRCR:
			iowrite32(arg, MIS_IR_CR);
			break;
		case VENUS_IR_IOC_SET_IRIOTCDP:
			iowrite32(arg, MIS_IR_DPIR);
			break;
		case VENUS_IR_IOC_SET_PROTOCOL:
			ir_protocol = (int)arg;
		case VENUS_IR_IOC_FLUSH_IRRP:
			Venus_IR_Init(ir_protocol);
			kfifo_reset(venus_ir_fifo);
			break;
		case VENUS_IR_IOC_SET_DEBOUNCE:
			debounce = (unsigned int)arg;
			break;
		default:
			retval = -ENOIOCTLCMD;
	}

	return retval;
}

struct file_operations venus_ir_fops = {
	.owner =    THIS_MODULE,
	.open  =    venus_ir_open,
	.read  =    venus_ir_read,
	.poll  =    venus_ir_poll,
	.ioctl =    venus_ir_ioctl,
};

int venus_ir_init_module(void) {
	int result;

	/* MKDEV */
	dev_venus_ir = MKDEV(VENUS_IR_MAJOR, VENUS_IR_MINOR_RP);

	/* Request Device Number */
	result = register_chrdev_region(dev_venus_ir, VENUS_IR_DEVICE_NUM, "irrp");
	if(result < 0) {
		printk(KERN_WARNING "Venus IR: can't register device number.\n");
		goto fail_alloc_dev;
	}

	/* Hardware Registers Initialization */
	Venus_IR_Init(ir_protocol);

	/* Initialize kfifo */
	venus_ir_fifo = kfifo_alloc(FIFO_DEPTH*sizeof(uint32_t), GFP_KERNEL, &venus_ir_lock);
	if(IS_ERR(venus_ir_fifo)) {
		result = -ENOMEM;
		goto fail_alloc_kfifo;
	}

	venus_ir_devs = platform_device_register_simple("VenusIR", -1, NULL, 0);
	if(driver_register(&venus_ir_driver) != 0)
		goto fail_device_register;

	/* Request IRQ */
	if(request_irq(VENUS_IR_IRQ, 
						IR_interrupt_handler, 
						SA_INTERRUPT|SA_SAMPLE_RANDOM|SA_SHIRQ, 
						"Venus_IR", 
						IR_interrupt_handler)) {
		printk(KERN_ERR "IR: cannot register IRQ %d\n", VENUS_IR_IRQ);
		result = -EIO;
		goto fail_alloc_irq;
	}

	/* Char Device Registration */
	/* Expose Register MIS_VFDO write interface */
	venus_ir_cdev = cdev_alloc();
	venus_ir_cdev->ops = &venus_ir_fops;
	if(cdev_add(venus_ir_cdev, MKDEV(VENUS_IR_MAJOR, VENUS_IR_MINOR_RP), 1)) {
		printk(KERN_ERR "Venus IR: can't add character device for irrp\n");
		result = -ENOMEM;
		goto fail_cdev_alloc;
	}
	/* use devfs to create device file! */
	devfs_mk_cdev(MKDEV(VENUS_IR_MAJOR, VENUS_IR_MINOR_RP), S_IFCHR|S_IRUSR|S_IWUSR, VENUS_IR_DEVICE_FILE);

	return 0;	/* succeed ! */

fail_cdev_alloc:
	free_irq(VENUS_IR_IRQ, IR_interrupt_handler);
fail_alloc_irq:
	kfifo_free(venus_ir_fifo);
fail_device_register:
	platform_device_unregister(venus_ir_devs);
	driver_unregister(&venus_ir_driver);
fail_alloc_kfifo:
#if 0
	release_mem_region(MIS_IR_PSR, (MIS_IR_SR - MIS_IR_PSR));
fail_alloc_mem:
#endif
	unregister_chrdev_region(dev_venus_ir, VENUS_IR_DEVICE_NUM);
fail_alloc_dev:
	return result;
}

void venus_ir_cleanup_module(void) {
	/* Reset Hardware Registers */
	/* Venus: Disable IRUE */
	iowrite32(0x80000000, MIS_IR_CR);

	/* remove device file by devfs */
	devfs_remove(VENUS_IR_DEVICE_FILE);

	/* Release Character Device Driver */
	cdev_del(venus_ir_cdev);

	/* Free IRQ Handler */
	free_irq(VENUS_IR_IRQ, IR_interrupt_handler);

	/* Free Kernel FIFO */
	kfifo_free(venus_ir_fifo);

	/* device driver removal */
	platform_device_unregister(venus_ir_devs);
	driver_unregister(&venus_ir_driver);

	/* Free IO Memory Region */
#if 0
	release_mem_region(MIS_IR_PSR, (MIS_IR_SR - MIS_IR_PSR));
#endif

	/* Return Device Numbers */
	unregister_chrdev_region(dev_venus_ir, VENUS_IR_DEVICE_NUM);
}

/* Register Macros */

module_init(venus_ir_init_module);
module_exit(venus_ir_cleanup_module);
