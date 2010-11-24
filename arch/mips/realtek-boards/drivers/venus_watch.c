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
#include <linux/poll.h>
#include <linux/wait.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/devfs_fs_kernel.h> /* for devfs */

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/interrupt.h>

#include <asm/bitops.h>		/* bit operations */

/* Module Variables */
/*
 * Our parameters which can be set at load time.
 */

//#define DEV_DEBUG
#define VENUS_WATCH_MAJOR			235
#define VENUS_WATCH_MINOR			0
#define VENUS_WATCH_DEVICE_NUM		1
#define VENUS_WATCH_DEVICE_FILE		"watch"

typedef struct {
	unsigned long		addr;
	unsigned long		prot;
} watch_struct;

#define WATCH_IOC_MAGIC				'j'
#define WATCH_IOQGETASID			_IO(WATCH_IOC_MAGIC, 1)
#define WATCH_IOSSETWATCH			_IOW(WATCH_IOC_MAGIC, 2, watch_struct)

extern unsigned long dvr_asid;
extern unsigned long watch_addr;

static DECLARE_WAIT_QUEUE_HEAD(venus_watch_read_wait);

/* Major Number + Minor Number */
static dev_t dev_venus_watch = 0;
static struct cdev *venus_watch_cdev = NULL;


MODULE_AUTHOR("I-Chieh Hsu, Realtek Semiconductor");
MODULE_LICENSE("Dual BSD/GPL");

int set_watch_point(unsigned long addr, unsigned long prot)
{
	unsigned long watchhi, watchlo;

	if (addr < 0x80000000) {
		printk("<Warning> Address of watch point should larger than 0x80000000...\n");
		printk("<Warning> Your address: 0x%x...\n", addr);
		return -1;
	}

	watch_addr = addr & 0xfffffffc;

	watchhi = 0x40000000;
	watchlo = (addr & 0xfffffff8) | prot;
	if (current_cpu_data.cputype == CPU_24K) { 
		__asm__ __volatile__ ("mtc0 %0, $19, 2;": : "r"(watchhi));
		__asm__ __volatile__ ("mtc0 %0, $18, 2;": : "r"(watchlo));
	} else {
		__asm__ __volatile__ ("mtc0 %0, $19;": : "r"(watchhi));
		__asm__ __volatile__ ("mtc0 %0, $18;": : "r"(watchlo));
	}

	return 0;
}
EXPORT_SYMBOL(set_watch_point);

int clr_watch_point()
{
	unsigned long watchlo = 0, watchhi = 0;
	
	watch_addr = 0;

	if (current_cpu_data.cputype == CPU_24K) { 
		__asm__ __volatile__ ("mtc0 %0, $19, 2;": : "r"(watchhi));
		__asm__ __volatile__ ("mtc0 %0, $18, 2;": : "r"(watchlo));
	} else {
		__asm__ __volatile__ ("mtc0 %0, $19;": : "r"(watchhi));
		__asm__ __volatile__ ("mtc0 %0, $18;": : "r"(watchlo));
	}
	
	return 0;
}
EXPORT_SYMBOL(clr_watch_point);

/* Module Functions */

int venus_watch_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	watch_struct ws;
	unsigned long watchhi, watchlo;
	int ret = 0;
	
	switch (cmd) {
		case WATCH_IOQGETASID:
			ret = dvr_asid;
			break;
		case WATCH_IOSSETWATCH:
			ret = copy_from_user(&ws, arg, sizeof(ws));
			if (!ret) {
				watch_addr = ws.addr & 0xfffffffc;
				
				watchhi = (dvr_asid & 0xff) << 16;
				watchlo = (ws.addr & 0xfffffff8) | ws.prot;
				if (current_cpu_data.cputype == CPU_24K) { 
					__asm__ __volatile__ ("mtc0 %0, $19, 2;": : "r"(watchhi));
					__asm__ __volatile__ ("mtc0 %0, $18, 2;": : "r"(watchlo));
				} else {
					__asm__ __volatile__ ("mtc0 %0, $19;": : "r"(watchhi));
					__asm__ __volatile__ ("mtc0 %0, $18;": : "r"(watchlo));
				}
			}
			break;
		default:
			return -ENOIOCTLCMD;
	}

	return ret;
}

struct file_operations venus_watch_fops = {
	.owner =    THIS_MODULE,
	.ioctl =    venus_watch_ioctl,
};

int venus_watch_init_module(void) {
	int result;

	/* MKDEV */
	dev_venus_watch = MKDEV(VENUS_WATCH_MAJOR, VENUS_WATCH_MINOR);

	/* Request Device Number */
	result = register_chrdev_region(dev_venus_watch, VENUS_WATCH_DEVICE_NUM, "watch");
	if (result < 0) {
		printk(KERN_WARNING "Venus WATCH: can't register device number...\n");
		goto fail_alloc_dev;
	}

	/* Char Device Registration */
	/* Expose Register MIS_VFDO write interface */
	venus_watch_cdev = cdev_alloc();
	venus_watch_cdev->ops = &venus_watch_fops;
	if (cdev_add(venus_watch_cdev, dev_venus_watch, VENUS_WATCH_DEVICE_NUM)) {
		printk(KERN_ERR "Venus WATCH: can't add character device for watch...\n");
		result = -ENOMEM;
		goto fail_cdev_alloc;
	}
	/* use devfs to create device file! */
	devfs_mk_cdev(dev_venus_watch, S_IFCHR|S_IRUSR|S_IWUSR, VENUS_WATCH_DEVICE_FILE);

	return 0;	/* succeed ! */

fail_cdev_alloc:
	unregister_chrdev_region(dev_venus_watch, VENUS_WATCH_DEVICE_NUM);
fail_alloc_dev:
	return result;
}

void venus_watch_cleanup_module(void) {
	/* remove device file by devfs */
	devfs_remove(VENUS_WATCH_DEVICE_FILE);

	/* Release Character Device Driver */
	cdev_del(venus_watch_cdev);

	/* Return Device Numbers */
	unregister_chrdev_region(dev_venus_watch, VENUS_WATCH_DEVICE_NUM);
}

/* Register Macros */

module_init(venus_watch_init_module);
module_exit(venus_watch_cleanup_module);
