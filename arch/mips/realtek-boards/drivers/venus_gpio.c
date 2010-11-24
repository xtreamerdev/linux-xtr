#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kfifo.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <asm/irq.h>
#include <asm/bitops.h>		/* bit operations */
#include <asm/io.h>
#include <asm/ioctl.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>

#include "venus_gpio.h"

#define ioread32 readl
#define iowrite32 writel
#define FIFO_DEPTH 256

#ifndef CONFIG_REALTEK_GPIO_RESCUE_FOR_PC_INSTALL
static DECLARE_WAIT_QUEUE_HEAD(venus_gpio_read_wait);
static struct kfifo *venus_gpio_fifo;
static spinlock_t venus_gpio_lock = SPIN_LOCK_UNLOCKED;

/* for Software Debouncing Mechanism */
static unsigned int lastRecvMs;
static unsigned int debounce = 0;

static irqreturn_t GPIO_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs) {
	int firstBitNr;
	uint32_t regValue;
	uint32_t statusRegValue;
	char gpioNr;

	regValue = ioread32(MIS_ISR);


	/* check if the interrupt belongs to us */
	if(regValue & 0x00100000) { // GPIODA_INT
		uint32_t enable1, enable2;

#ifdef DEV_DEBUG
		printk(KERN_WARNING "Venus GPIO: (dis-assert)Interrupt Handler Triggered.\n");
#endif
		enable1 = ioread32(MIS_GP0IE);
		enable2 = ioread32(MIS_GP1IE);

		if((jiffies_to_msecs(jiffies)- lastRecvMs) < debounce)
			goto irq_da_handled;

		lastRecvMs = jiffies_to_msecs(jiffies);

		statusRegValue = ioread32(MIS_UMSK_ISR_GP0DA); // GPIO - 0
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = (char)firstBitNr - 1 - 1;
			if(enable1 & (0x1 << gpioNr))
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));
			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

		statusRegValue = ioread32(MIS_UMSK_ISR_GP1DA); // GPIO - 1
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = (char)firstBitNr - 1 - 1;
			if(enable2 & (0x1 << gpioNr)) {
				gpioNr += 31;
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));
			}
			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

irq_da_handled:
		iowrite32(0xfffffffe, MIS_UMSK_ISR_GP0DA);
		iowrite32(0x0000003e, MIS_UMSK_ISR_GP1DA);

		iowrite32(0x00100000, MIS_ISR); /* clear interrupt flag */
		return IRQ_HANDLED;
	}
	else if(regValue & 0x00080000) { // GPIOA_INT
		uint32_t enable1, enable2;
#ifdef DEV_DEBUG
		printk(KERN_WARNING "Venus GPIO: (assert)Interrupt Handler Triggered.\n");
#endif

		enable1 = ioread32(MIS_GP0IE);
		enable2 = ioread32(MIS_GP1IE);

		if((jiffies_to_msecs(jiffies)- lastRecvMs) < debounce)
			goto irq_a_handled;

		lastRecvMs = jiffies_to_msecs(jiffies);

		statusRegValue = ioread32(MIS_UMSK_ISR_GP0A); // GPIO - 0
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = (char)firstBitNr - 1 - 1;
			if(enable1 & (0x1 << gpioNr))
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));
			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

		statusRegValue = ioread32(MIS_UMSK_ISR_GP1A); // GPIO - 1
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = (char)firstBitNr - 1 - 1;
			if(enable2 & (0x1 << gpioNr)) {
				gpioNr += 31;
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));
			}
			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

irq_a_handled:
		iowrite32(0xfffffffe, MIS_UMSK_ISR_GP0A);
		iowrite32(0x0000003e, MIS_UMSK_ISR_GP1A);

		iowrite32(0x00080000, MIS_ISR); /* clear interrupt flag */
		return IRQ_HANDLED;
	}
	else
		return IRQ_NONE;
}

ssize_t venus_gpio_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	char uintBuf;
	int i, readCount = count;

restart:
	if(wait_event_interruptible(venus_gpio_read_wait, __kfifo_len(venus_gpio_fifo) > 0) != 0) {
		if(unlikely(current->flags & PF_FREEZE)) {
			refrigerator(PF_FREEZE);
			goto restart;
		}
		else
			return -ERESTARTSYS;
	}

	if(__kfifo_len(venus_gpio_fifo) < count)
		readCount = __kfifo_len(venus_gpio_fifo);

	for(i = 0 ; i < readCount ; i++) {
		__kfifo_get(venus_gpio_fifo, &uintBuf, sizeof(char));
		copy_to_user(buf, &uintBuf, sizeof(char));
		buf++;
	}

	return readCount;
}

unsigned int venus_gpio_poll(struct file *filp, poll_table *wait) {
	if(__kfifo_len(venus_gpio_fifo) > 0)
		return POLLIN | POLLRDNORM;
	else
		return 0;
}

static int venus_gpio_open(struct inode *inode, struct file *file) {
	/* reinitialize kfifo */
	kfifo_reset(venus_gpio_fifo);

	return 0;
}

static int venus_gpio_release(struct inode *inode, struct file *file) {
	return 0;
}

static int venus_gpio_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg) {

	int err = 0;
	int retval = 0;

	if (_IOC_TYPE(cmd) != VENUS_GPIO_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VENUS_GPIO_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) 
		return -EFAULT;

	if (!capable (CAP_SYS_ADMIN))
		return -EPERM;

	switch(cmd) {
		case VENUS_GPIO_ENABLE_INT:
			if(arg >= 0 && arg <= 31) {
				iowrite32(ioread32(MIS_GP0DIR) & (~(0x01 << arg)), MIS_GP0DIR);
				iowrite32(ioread32(MIS_GP0IE) | (0x1<<arg), MIS_GP0IE);
			}
			else if(arg >= 32 && arg <= 35) {
				iowrite32(ioread32(MIS_GP1DIR) & (~(0x01 << (arg-32))), MIS_GP1DIR);
				iowrite32(ioread32(MIS_GP1IE) | (0x1<<(arg-32)), MIS_GP1IE);
			}
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_DISABLE_INT:
			if(arg >= 0 && arg <= 31)
				iowrite32(ioread32(MIS_GP0IE) & (~(0x1<<arg)), MIS_GP0IE);
			else if(arg >= 32 && arg <= 35)
				iowrite32(ioread32(MIS_GP1IE) & (~~(0x1<<(arg-32))), MIS_GP1IE);
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_SET_ASSERT:
			if(arg >= 0 && arg <= 31)
				iowrite32(ioread32(MIS_GP0DP) | (0x1<<arg), MIS_GP0DP);
			else if(arg >= 32 && arg <= 35)
				iowrite32(ioread32(MIS_GP1DP) | (0x1<<(arg-32)), MIS_GP1DP);
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_SET_DEASSERT:
			if(arg >= 0 && arg <= 31)
				iowrite32(ioread32(MIS_GP0DP) & (~(0x1<<arg)), MIS_GP0DP);
			else if(arg >= 32 && arg <= 35)
				iowrite32(ioread32(MIS_GP1DP) & (~~(0x1<<(arg-32))), MIS_GP1DP);
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_SET_SW_DEBOUNCE:
			debounce = (unsigned int)arg;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB1:
			if(arg >= 0 && arg <= 7)
				iowrite32((ioread32(MIS_GPDEB) & 0xfffffff0) | (0x8|arg), MIS_GPDEB);
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB2:
			if(arg >= 0 && arg <= 7)
				iowrite32((ioread32(MIS_GPDEB) & 0xffffff0f) | ((0x8|arg) << 8), MIS_GPDEB);
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB3:
			if(arg >= 0 && arg <= 7)
				iowrite32((ioread32(MIS_GPDEB) & 0xfffff0ff) | ((0x8|arg) << 16), MIS_GPDEB);
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB4:
			if(arg >= 0 && arg <= 7)
				iowrite32((ioread32(MIS_GPDEB) & 0xffff0fff) | ((0x8|arg) << 24), MIS_GPDEB);
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_RESET_INT_STATUS:
			iowrite32(0xfffffffe, MIS_UMSK_ISR_GP0DA);
			iowrite32(0x0000003e, MIS_UMSK_ISR_GP1DA);
			iowrite32(0xfffffffe, MIS_UMSK_ISR_GP0A);
			iowrite32(0x0000003e, MIS_UMSK_ISR_GP1A);
			break;
		default:
			retval = -ENOIOCTLCMD;
	}

	return retval;
}

static struct file_operations venus_gpio_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= venus_gpio_ioctl,
	.open		= venus_gpio_open,
	.poll		= venus_gpio_poll,
	.read		= venus_gpio_read,
	.release	= venus_gpio_release,
};

static struct miscdevice venus_gpio_miscdev = {
	MISC_DYNAMIC_MINOR,
	"venus_gpio",
	&venus_gpio_fops
};

int __init venus_gpio_init(void)
{
	lastRecvMs = jiffies_to_msecs(jiffies);

	if (misc_register(&venus_gpio_miscdev))
		return -ENODEV;

	/* initialize HW registers */
	/* by default, disable all interrupt */
	iowrite32(0, MIS_GP0IE);
	iowrite32(0, MIS_GP1IE);

	/* Initialize kfifo */
	venus_gpio_fifo = kfifo_alloc(FIFO_DEPTH, GFP_KERNEL, &venus_gpio_lock);
	if(IS_ERR(venus_gpio_fifo)) {
		misc_deregister(&venus_gpio_miscdev);
		return -ENOMEM;
	}

	/* Request IRQ */
	if(request_irq(VENUS_GPIO_IRQ, 
						GPIO_interrupt_handler, 
						SA_INTERRUPT|SA_SAMPLE_RANDOM|SA_SHIRQ, 
						"Venus_GPIO", 
						GPIO_interrupt_handler)) {
		printk(KERN_ERR "GPIO: cannot register IRQ %d\n", VENUS_GPIO_IRQ);

		kfifo_free(venus_gpio_fifo);
		misc_deregister(&venus_gpio_miscdev);
		return -EIO;
	}

	return 0;
}	

static void __exit venus_gpio_exit(void)
{
	/* Free IRQ Handler */
	free_irq(VENUS_GPIO_IRQ, GPIO_interrupt_handler);

	/* Free Kernel FIFO */
	kfifo_free(venus_gpio_fifo);

	/* De-register */
	misc_deregister(&venus_gpio_miscdev);
}

#else /* CONFIG_REALTEK_GPIO_RESCUE_FOR_PC_INSTALL */
/* This GPIO interrupt handler is for PC Install. If we detect interrupt from GPIO 28,
   that means the user is plugging in the PC-USB cable and then we have to switch our
   chip/CPU to Test Mode. The Test Mode will bypass the IDE channel to that USB channel. */
#include <linux/syscalls.h>
static inline u32 magic2u32(const char *p) {
	u32 value = 0;

	value = (p[0] & 0x000000FF) |
		((p[1] << 8) & 0x0000FF00) |
		((p[2] << 16) & 0x00FF0000) |
		((p[3] << 24) & 0xFF000000);

	return value;
}

static void magic_deadbeef_check(void *data) {
	const char *dev_path = "/dev/hda";
	unsigned char buf[5];
	u32 magic = 0;
	int fd;
	int ret;

	if((fd = sys_open(dev_path, O_RDONLY, 0)) < 0) {
		printk("%s: Open %s error!\n", __FUNCTION__, dev_path);
		return;
	}

	/* Seek to where the magic '0xdeadbeef' is located. */
	if((ret = sys_lseek(fd, (0x3e*512), 0)) < 0) {
		printk("%s: Seek %s error!\n", __FUNCTION__, dev_path);
		sys_close(fd);
		return;
	}

	memset(buf, 0, sizeof(buf));
	if((ret = sys_read(fd, buf, sizeof(buf))) < 0) {
		printk("%s: Read %s error!\n", __FUNCTION__, dev_path);
		sys_close(fd);
		return;
	}

	magic = magic2u32(buf);

	printk("%s: magic=0x%x\n", __FUNCTION__, magic);

	if(magic == 0xdeadbeef) {
		sys_close(fd);
		printk("%s: Switch to Test Mode!\n", __FUNCTION__);
		iowrite32(0x08, MIS_GP1DIR); //Go to Test Mode.
		return;
	}

	if((ret = sys_close(fd)) < 0) {
		printk("%s: Close %s error!\n", __FUNCTION__, dev_path);
		return;
	}

	return;
}

static DECLARE_WORK(magic_deadbeef_check_work, magic_deadbeef_check, NULL);

void do_magic_deadbeef_check(void) {
	schedule_work(&magic_deadbeef_check_work);
	return;
}

static irqreturn_t GPIO_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs) {
	uint32_t regValue;
	uint32_t value;

	regValue = ioread32(MIS_ISR);

	//if(regValue != 0x4)
	//	printk("[DEBUG_MSG] %s, entered, regValue=0x%x\n", __FUNCTION__, regValue);

	if((regValue & 0x00080000)) { // GPIODA_INT
		/* Check GPIO 28 status bit. */
		value = ioread32(MIS_UMSK_ISR_GP0A);
		if(value & 0x20000000) {
			iowrite32(0x20000000, MIS_UMSK_ISR_GP0A);
			iowrite32(0x00080000, MIS_ISR); //clear interrupt flag.

			do_magic_deadbeef_check();

			iowrite32(0xfffffffe, MIS_UMSK_ISR_GP0DA);
			iowrite32(0x0000003e, MIS_UMSK_ISR_GP1DA);

			return IRQ_HANDLED;
		}
	}

	/* check if the interrupt belongs to us */
	if(regValue & 0x00100000) { // GPIODA_INT
#ifdef DEV_DEBUG
		printk(KERN_WARNING "Venus GPIO: (dis-assert)Interrupt Handler Triggered.\n");
#endif
		iowrite32(0xfffffffe, MIS_UMSK_ISR_GP0DA);
		iowrite32(0x0000003e, MIS_UMSK_ISR_GP1DA);

		iowrite32(0x00100000, MIS_ISR); /* clear interrupt flag */
		return IRQ_HANDLED;
	}
	else if(regValue & 0x00080000) { // GPIOA_INT
#ifdef DEV_DEBUG
		printk(KERN_WARNING "Venus GPIO: (assert)Interrupt Handler Triggered.\n");
#endif
		iowrite32(0xfffffffe, MIS_UMSK_ISR_GP0A);
		iowrite32(0x0000003e, MIS_UMSK_ISR_GP1A);

		iowrite32(0x00080000, MIS_ISR); /* clear interrupt flag */
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

int __init venus_gpio_init(void) {

	/* initialize HW registers */
	/* by default, disable all interrupt */
	iowrite32(0, MIS_GP0IE);
	iowrite32(0, MIS_GP1IE);

	printk("GPIO: register IRQ %d... ", VENUS_GPIO_IRQ);
	/* Request IRQ */
	if(request_irq(VENUS_GPIO_IRQ, GPIO_interrupt_handler,
					//SA_INTERRUPT|SA_SAMPLE_RANDOM|SA_SHIRQ, 
					SA_INTERRUPT|SA_SHIRQ, 
					"Venus_GPIO", 
					GPIO_interrupt_handler)) {
		printk(KERN_ERR "GPIO: cannot register IRQ %d\n", VENUS_GPIO_IRQ);
		return -EIO;
	}

	printk("(Success)\n");

	/* Enable interrupt */
	iowrite32(0x10000000, MIS_GP0IE); //Enable for GPIO 28.
	iowrite32(0x0, MIS_GP1IE);

	return 0;
}

static void __exit venus_gpio_exit(void) {

	free_irq(VENUS_GPIO_IRQ, GPIO_interrupt_handler);
}
#endif /* CONFIG_REALTEK_GPIO_RESCUE_FOR_PC_INSTALL */

module_init(venus_gpio_init);
module_exit(venus_gpio_exit);

MODULE_AUTHOR("Chih-pin Wu, Realtek Semiconductor");
MODULE_LICENSE("GPL");
