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
#include <linux/device.h>
#include <platform.h>
#include <venus.h>

#include "venus_gpio.h"

#define ioread32 readl
#define iowrite32 writel
#define FIFO_DEPTH 256

extern platform_info_t platform_info;

#ifndef CONFIG_REALTEK_GPIO_RESCUE_FOR_PC_INSTALL
static DECLARE_WAIT_QUEUE_HEAD(venus_gpio_read_wait);
static struct kfifo *venus_gpio_fifo;
static spinlock_t venus_gpio_lock = SPIN_LOCK_UNLOCKED;

/* for Software Debouncing Mechanism */
static unsigned int lastRecvMs;
static unsigned int debounce = 0;

/* for suspend/resume */
static unsigned int backup_gp0ie;
static unsigned int backup_gp1ie;
static unsigned int backup_gp2ie;
static unsigned int backup_gp3ie;

static int _is_pin_enabled(uint8_t num, int a, int b, int c, int d) {
	if(num <= 31) {
		if(a & (0x1 << num))
			return 1;
		else
			return 0;
	}
	else if(num <= 63) {
		if(b & (0x1 << (num - 32)))
			return 1;
		else
			return 0;
	}
	else if(num <= 95) {
		if(c & (0x1 << (num - 64)))
			return 1;
		else
			return 0;
	}
	else if(num <= 127) {
		if(d & (0x1 << (num - 96)))
			return 1;
		else
			return 0;
	}
	else
		return 0;
}

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
			if(gpioNr == 0) {
				if(enable1 & (0x1 << 31)) {
					gpioNr += 31;
					__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));
				}
			}
			else if(enable2 & (0x1 << (gpioNr-1))) {
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
			if(gpioNr == 0) {
				if(enable1 & (0x1 << 31)) {
					gpioNr += 31;
					__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));
				}
			}
			else if(enable2 & (0x1 << (gpioNr-1))) {
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

static irqreturn_t MARS_GPIO_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs) {
	int firstBitNr;
	uint32_t regValue;
	uint32_t statusRegValue;
	char gpioNr;

	regValue = ioread32(MARS_MIS_ISR);

	/* check if the interrupt belongs to us */
	if(regValue & 0x00100000) { // GPIODA_INT
		uint32_t enable1, enable2, enable3, enable4;

#ifdef DEV_DEBUG
		printk(KERN_WARNING "Mars GPIO: (dis-assert)Interrupt Handler Triggered.\n");
#endif
		enable1 = ioread32(MARS_MIS_GP0IE);
		enable2 = ioread32(MARS_MIS_GP1IE);
		enable3 = ioread32(MARS_MIS_GP2IE);
		enable4 = ioread32(MARS_MIS_GP3IE);

		if((jiffies_to_msecs(jiffies)- lastRecvMs) < debounce)
			goto irq_da_handled;

		lastRecvMs = jiffies_to_msecs(jiffies);

		statusRegValue = ioread32(MARS_MIS_UMSK_ISR_GP0DA); // GPIO - 0 [#0~#30]
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = (char)firstBitNr - 1 - 1;

			if(_is_pin_enabled(gpioNr, enable1, enable2, enable3, enable4))
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));

			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

		statusRegValue = ioread32(MARS_MIS_UMSK_ISR_GP1DA);	// GPIO - 1 [#31~#61]
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = 31 + (char)firstBitNr - 1 - 1;

			if(_is_pin_enabled(gpioNr, enable1, enable2, enable3, enable4)) 
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));

			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

		statusRegValue = ioread32(MARS_MIS_UMSK_ISR_GP2DA);	// GPIO - 2 [#62~#92]
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = 62 + (char)firstBitNr - 1 - 1;

			if(_is_pin_enabled(gpioNr, enable1, enable2, enable3, enable4)) 
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));

			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

		statusRegValue = ioread32(MARS_MIS_UMSK_ISR_GP3DA);	// GPIO - 3 [#93~#104]
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = 93 + (char)firstBitNr - 1 - 1;

			if(_is_pin_enabled(gpioNr, enable1, enable2, enable3, enable4)) 
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));

			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

irq_da_handled:
		iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP0DA);
		iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP1DA);
		iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP2DA);
		iowrite32(0x0001fffe, MARS_MIS_UMSK_ISR_GP3DA);

		iowrite32(0x00100000, MARS_MIS_ISR); /* clear interrupt flag */
		return IRQ_HANDLED;
	}
	else if(regValue & 0x00080000) { // GPIOA_INT
		uint32_t enable1, enable2, enable3, enable4;
#ifdef DEV_DEBUG
		printk(KERN_WARNING "Mars GPIO: (assert)Interrupt Handler Triggered.\n");
#endif

		enable1 = ioread32(MARS_MIS_GP0IE);
		enable2 = ioread32(MARS_MIS_GP1IE);
		enable3 = ioread32(MARS_MIS_GP2IE);
		enable4 = ioread32(MARS_MIS_GP3IE);

		if((jiffies_to_msecs(jiffies)- lastRecvMs) < debounce)
			goto irq_a_handled;

		lastRecvMs = jiffies_to_msecs(jiffies);

		statusRegValue = ioread32(MARS_MIS_UMSK_ISR_GP0A); // GPIO - 0 [#0~#30]
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = (char)firstBitNr - 1 - 1;

			if(_is_pin_enabled(gpioNr, enable1, enable2, enable3, enable4))
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));

			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

		statusRegValue = ioread32(MARS_MIS_UMSK_ISR_GP1A); // GPIO - 1 [#31~#61]
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = 31 + (char)firstBitNr - 1 - 1;

			if(_is_pin_enabled(gpioNr, enable1, enable2, enable3, enable4))
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));

			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

		statusRegValue = ioread32(MARS_MIS_UMSK_ISR_GP2A); // GPIO - 2 [#62~#92]
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = 62 + (char)firstBitNr - 1 - 1;

			if(_is_pin_enabled(gpioNr, enable1, enable2, enable3, enable4))
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));

			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

		statusRegValue = ioread32(MARS_MIS_UMSK_ISR_GP3A); // GPIO - 1 [#93~#104]
		while((firstBitNr = ffs(statusRegValue)) != 0) {
			gpioNr = 93 + (char)firstBitNr - 1 - 1;

			if(_is_pin_enabled(gpioNr, enable1, enable2, enable3, enable4))
				__kfifo_put(venus_gpio_fifo, &gpioNr, sizeof(char));

			clear_bit(firstBitNr - 1, (unsigned long *)&statusRegValue);
		}

irq_a_handled:
		iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP0A);
		iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP1A);
		iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP2A);
		iowrite32(0x0001fffe, MARS_MIS_UMSK_ISR_GP3A);

		iowrite32(0x00080000, MARS_MIS_ISR); /* clear interrupt flag */
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
	poll_wait(filp, &venus_gpio_read_wait, wait);

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

	unsigned long up_limit;

    if(is_venus_cpu())
		up_limit = 35;
	else if(is_neptune_cpu())
		up_limit = 47;
	else if(is_mars_cpu())
		up_limit = 108;
	else
		up_limit = 35;

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
			if(is_mars_cpu()) {
				if(arg >= 0 && arg <= 31) {
					iowrite32(ioread32(MARS_MIS_GP0DIR) & (~(0x01 << arg)), MARS_MIS_GP0DIR);
					iowrite32(ioread32(MARS_MIS_GP0IE) | (0x1<<arg), MARS_MIS_GP0IE);
				}
				else if(arg >= 32 && arg <= 63) {
					iowrite32(ioread32(MARS_MIS_GP1DIR) & (~(0x01 << (arg-32))), MARS_MIS_GP1DIR);
					iowrite32(ioread32(MARS_MIS_GP1IE) | (0x1<<(arg-32)), MARS_MIS_GP1IE);
				}
				else if(arg >= 64 && arg <= 95) {
					iowrite32(ioread32(MARS_MIS_GP2DIR) & (~(0x01 << (arg-64))), MARS_MIS_GP2DIR);
					iowrite32(ioread32(MARS_MIS_GP2IE) | (0x1<<(arg-64)), MARS_MIS_GP2IE);
				}
				else if(arg >= 96 && arg <= up_limit) {
					iowrite32(ioread32(MARS_MIS_GP3DIR) & (~(0x01 << (arg-96))), MARS_MIS_GP3DIR);
					iowrite32(ioread32(MARS_MIS_GP3IE) | (0x1<<(arg-96)), MARS_MIS_GP3IE);
				}
				else
					retval = -EFAULT;					
			}
			else {
				if(arg >= 0 && arg <= 31) {
					iowrite32(ioread32(MIS_GP0DIR) & (~(0x01 << arg)), MIS_GP0DIR);
					iowrite32(ioread32(MIS_GP0IE) | (0x1<<arg), MIS_GP0IE);
				}
				else if(arg >= 32 && arg <= up_limit) {
					iowrite32(ioread32(MIS_GP1DIR) & (~(0x01 << (arg-32))), MIS_GP1DIR);
					iowrite32(ioread32(MIS_GP1IE) | (0x1<<(arg-32)), MIS_GP1IE);
				}
				else
					retval = -EFAULT;
			}
			break;
		case VENUS_GPIO_DISABLE_INT:
			if(is_mars_cpu()) {
				if(arg >= 0 && arg <= 31)
					iowrite32(ioread32(MARS_MIS_GP0IE) & (~(0x1<<arg)), MARS_MIS_GP0IE);
				else if(arg >= 32 && arg <= 63)
					iowrite32(ioread32(MARS_MIS_GP1IE) & (~(0x1<<(arg-32))), MARS_MIS_GP1IE);
				else if(arg >= 64 && arg <= 95)
					iowrite32(ioread32(MARS_MIS_GP2IE) & (~(0x1<<(arg-64))), MARS_MIS_GP2IE);
				else if(arg >= 96 && arg <= up_limit)
					iowrite32(ioread32(MARS_MIS_GP3IE) & (~(0x1<<(arg-96))), MARS_MIS_GP3IE);
				else
					retval = -EFAULT;
			}
			else {
				if(arg >= 0 && arg <= 31)
					iowrite32(ioread32(MIS_GP0IE) & (~(0x1<<arg)), MIS_GP0IE);
				else if(arg >= 32 && arg <= up_limit)
					iowrite32(ioread32(MIS_GP1IE) & (~(0x1<<(arg-32))), MIS_GP1IE);
				else
					retval = -EFAULT;
			}
			break;
		case VENUS_GPIO_SET_ASSERT:
			if(is_mars_cpu()) {
				if(arg >= 0 && arg <= 31)
					iowrite32(ioread32(MARS_MIS_GP0DP) | (0x1<<arg), MARS_MIS_GP0DP);
				else if(arg >= 32 && arg <= 63)
					iowrite32(ioread32(MARS_MIS_GP1DP) | (0x1<<(arg-32)), MARS_MIS_GP1DP);
				else if(arg >= 64 && arg <= 95)
					iowrite32(ioread32(MARS_MIS_GP2DP) | (0x1<<(arg-64)), MARS_MIS_GP2DP);
				else if(arg >= 96 && arg <= up_limit)
					iowrite32(ioread32(MARS_MIS_GP3DP) | (0x1<<(arg-96)), MARS_MIS_GP3DP);
				else
					retval = -EFAULT;
			}
			else {
				if(arg >= 0 && arg <= 31)
					iowrite32(ioread32(MIS_GP0DP) | (0x1<<arg), MIS_GP0DP);
				else if(arg >= 32 && arg <= up_limit)
					iowrite32(ioread32(MIS_GP1DP) | (0x1<<(arg-32)), MIS_GP1DP);
				else
					retval = -EFAULT;
			}
			break;
		case VENUS_GPIO_SET_DEASSERT:
			if(is_mars_cpu()) {
				if(arg >= 0 && arg <= 31)
					iowrite32(ioread32(MARS_MIS_GP0DP) & (~(0x1<<arg)), MARS_MIS_GP0DP);
				else if(arg >= 32 && arg <= 63)
					iowrite32(ioread32(MARS_MIS_GP1DP) & (~(0x1<<(arg-32))), MARS_MIS_GP1DP);
				else if(arg >= 64 && arg <= 95)
					iowrite32(ioread32(MARS_MIS_GP2DP) & (~(0x1<<(arg-64))), MARS_MIS_GP2DP);
				else if(arg >= 96 && arg <= up_limit)
					iowrite32(ioread32(MARS_MIS_GP3DP) & (~(0x1<<(arg-96))), MARS_MIS_GP3DP);
				else
					retval = -EFAULT;
			}
			else {
				if(arg >= 0 && arg <= 31)
					iowrite32(ioread32(MIS_GP0DP) & (~(0x1<<arg)), MIS_GP0DP);
				else if(arg >= 32 && arg <= up_limit)
					iowrite32(ioread32(MIS_GP1DP) & (~(0x1<<(arg-32))), MIS_GP1DP);
				else
					retval = -EFAULT;
			}
			break;
		case VENUS_GPIO_SET_SW_DEBOUNCE:
			debounce = (unsigned int)arg;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB1:
			if(arg >= 0 && arg <= 7) {
				if(is_mars_cpu())
					iowrite32((ioread32(MARS_MIS_GPDEB) & 0xfffffff0) | (0x8|arg), MARS_MIS_GPDEB);
				else
					iowrite32((ioread32(MIS_GPDEB) & 0xfffffff0) | (0x8|arg), MIS_GPDEB);
			}
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB2:
			if(arg >= 0 && arg <= 7) {
				if(is_mars_cpu())
					iowrite32((ioread32(MARS_MIS_GPDEB) & 0xffffff0f) | ((0x8|arg) << 4), MARS_MIS_GPDEB);
				else
					iowrite32((ioread32(MIS_GPDEB) & 0xffffff0f) | ((0x8|arg) << 4), MIS_GPDEB);
			}
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB3:
			if(arg >= 0 && arg <= 7) {
				if(is_mars_cpu())
					iowrite32((ioread32(MARS_MIS_GPDEB) & 0xfffff0ff) | ((0x8|arg) << 8), MARS_MIS_GPDEB);
				else
					iowrite32((ioread32(MIS_GPDEB) & 0xfffff0ff) | ((0x8|arg) << 8), MIS_GPDEB);
			}
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB4:
			if(arg >= 0 && arg <= 7) {
				if(is_mars_cpu())
					iowrite32((ioread32(MARS_MIS_GPDEB) & 0xffff0fff) | ((0x8|arg) << 12), MARS_MIS_GPDEB);
				else
					iowrite32((ioread32(MIS_GPDEB) & 0xffff0fff) | ((0x8|arg) << 12), MIS_GPDEB);
			}
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB5: // Mars Only
			if(is_mars_cpu() && arg >= 0 && arg <= 7)
				iowrite32((ioread32(MARS_MIS_GPDEB) & 0xffff0fff) | ((0x8|arg) << 16), MARS_MIS_GPDEB);
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB6: // Mars Only
			if(is_mars_cpu() && arg >= 0 && arg <= 7)
				iowrite32((ioread32(MARS_MIS_GPDEB) & 0xffff0fff) | ((0x8|arg) << 20), MARS_MIS_GPDEB);
			else
				retval = -EFAULT;
			break;
		case VENUS_GPIO_CHOOSE_GPDEB7: // Mars Only
			if(is_mars_cpu() && arg >= 0 && arg <= 7)
				iowrite32((ioread32(MARS_MIS_GPDEB) & 0xffff0fff) | ((0x8|arg) << 24), MARS_MIS_GPDEB);
			else
				retval = -EFAULT;
			break;			
		case VENUS_GPIO_RESET_INT_STATUS:
			if(is_mars_cpu()) {
				iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP0DA);
				iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP1DA);
				iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP2DA);
				iowrite32(0x0001fffe, MARS_MIS_UMSK_ISR_GP3DA);
				iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP0A);
				iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP1A);
				iowrite32(0xfffffffe, MARS_MIS_UMSK_ISR_GP2A);
				iowrite32(0x0001fffe, MARS_MIS_UMSK_ISR_GP3A);
			}
			else {
				iowrite32(0xfffffffe, MIS_UMSK_ISR_GP0DA);
				iowrite32(0x0000003e, MIS_UMSK_ISR_GP1DA);
				iowrite32(0xfffffffe, MIS_UMSK_ISR_GP0A);
				iowrite32(0x0000003e, MIS_UMSK_ISR_GP1A);
			}
			break;
		default:
			retval = -ENOIOCTLCMD;
	}

	return retval;
}

static int venus_gpio_suspend(struct device *dev, pm_message_t state, u32 level) {
	if(is_mars_cpu()) {
		// backup status register
		backup_gp0ie = ioread32(MARS_MIS_GP0IE);
		backup_gp1ie = ioread32(MARS_MIS_GP1IE);
		backup_gp2ie = ioread32(MARS_MIS_GP2IE);
		backup_gp3ie = ioread32(MARS_MIS_GP3IE);

		// disable interrupt
		iowrite32(0, MARS_MIS_GP0IE);
		iowrite32(0, MARS_MIS_GP1IE);
		iowrite32(0, MARS_MIS_GP2IE);
		iowrite32(0, MARS_MIS_GP3IE);
	}
	else {
		// backup status register
		backup_gp0ie = ioread32(MIS_GP0IE);
		backup_gp1ie = ioread32(MIS_GP1IE);

		// disable interrupt
		iowrite32(0, MIS_GP0IE);
		iowrite32(0, MIS_GP1IE);
	}

	return 0;
}

static int venus_gpio_resume(struct device *dev, u32 level) {
	if(is_mars_cpu()) {
		// restore interrupt
		iowrite32(backup_gp0ie, MARS_MIS_GP0IE);
		iowrite32(backup_gp1ie, MARS_MIS_GP1IE);
		iowrite32(backup_gp2ie, MARS_MIS_GP2IE);
		iowrite32(backup_gp3ie, MARS_MIS_GP3IE);
	}
	else {
		// restore interrupt
		iowrite32(backup_gp0ie, MIS_GP0IE);
		iowrite32(backup_gp1ie, MIS_GP1IE);
	}

	return 0;
}

static struct platform_device *venus_gpio_devs;

static struct device_driver venus_gpio_driver = {
    .name       = "Venus GPIO",
    .bus        = &platform_bus_type,
    .suspend    = venus_gpio_suspend,
    .resume     = venus_gpio_resume,
};

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
	int ret;
	lastRecvMs = jiffies_to_msecs(jiffies);

	if (misc_register(&venus_gpio_miscdev))
		return -ENODEV;

	/* initialize HW registers */
	/* by default, disable all interrupt */
	if(is_mars_cpu()) {
		iowrite32(0, MARS_MIS_GP0IE);
		iowrite32(0, MARS_MIS_GP1IE);
		iowrite32(0, MARS_MIS_GP2IE);
		iowrite32(0, MARS_MIS_GP3IE);
	}
	else {
		iowrite32(0, MIS_GP0IE);
		iowrite32(0, MIS_GP1IE);
	}

	/* Initialize kfifo */
	venus_gpio_fifo = kfifo_alloc(FIFO_DEPTH, GFP_KERNEL, &venus_gpio_lock);
	if(IS_ERR(venus_gpio_fifo)) {
		misc_deregister(&venus_gpio_miscdev);
		return -ENOMEM;
	}

	venus_gpio_devs = platform_device_register_simple("Venus GPIO", -1, NULL, 0);

	if(driver_register(&venus_gpio_driver) != 0) {
		platform_device_unregister(venus_gpio_devs);
		kfifo_free(venus_gpio_fifo);
		misc_deregister(&venus_gpio_miscdev);
		return -ENOMEM;
	}

	/* Request IRQ */
	if(is_mars_cpu())
		ret = request_irq(VENUS_GPIO_IRQ, 
						MARS_GPIO_interrupt_handler, 
						SA_INTERRUPT|SA_SAMPLE_RANDOM|SA_SHIRQ, 
						"Venus_GPIO", 
						MARS_GPIO_interrupt_handler);
	else
		ret = request_irq(VENUS_GPIO_IRQ, 
						GPIO_interrupt_handler, 
						SA_INTERRUPT|SA_SAMPLE_RANDOM|SA_SHIRQ, 
						"Venus_GPIO", 
						GPIO_interrupt_handler);
	
	if(ret) {
		printk(KERN_ERR "GPIO: cannot register IRQ %d\n", VENUS_GPIO_IRQ);
		platform_device_unregister(venus_gpio_devs);
		driver_unregister(&venus_gpio_driver);
		kfifo_free(venus_gpio_fifo);
		misc_deregister(&venus_gpio_miscdev);
		return -EIO;
	}

	return 0;
}	

static void __exit venus_gpio_exit(void)
{
	/* disable all interrupt */
	if(is_mars_cpu()) {
		iowrite32(0, MARS_MIS_GP0IE);
		iowrite32(0, MARS_MIS_GP1IE);
		iowrite32(0, MARS_MIS_GP2IE);
		iowrite32(0, MARS_MIS_GP3IE);
	}
	else {
		iowrite32(0, MIS_GP0IE);
		iowrite32(0, MIS_GP1IE);
	}

	/* Free IRQ Handler */
	if(is_mars_cpu())
		free_irq(VENUS_GPIO_IRQ, MARS_GPIO_interrupt_handler);
	else
		free_irq(VENUS_GPIO_IRQ, GPIO_interrupt_handler);


	/* Free Kernel FIFO */
	kfifo_free(venus_gpio_fifo);

	/* Driver Unregister */
	platform_device_unregister(venus_gpio_devs);
	driver_unregister(&venus_gpio_driver);

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

#define NEPTUNE_RESET_ASSERT	0x907e
#define NEPTUNE_ENABLE_ASSERT	0x7f20e
#define NEPTUNE_RESET_DEASSERT	0xffffffff
#define NEPTUNE_ENABLE_DEASSERT	0xffffffff



static irqreturn_t GPIO_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs) {
	uint32_t regValue;
	uint32_t value;

	regValue = ioread32(MIS_ISR);

	//if(regValue != 0x4)
	//	printk("[DEBUG_MSG] %s, entered, regValue=0x%x\n", __FUNCTION__, regValue);

	if((regValue & 0x00080000)) { // GPIODA_INT
		if(is_venus_cpu()) {
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
		} else if(is_neptune_cpu()) {
			/* check GPIO33 assert flag */
			value = ioread32(MIS_UMSK_ISR_GP1A);
			if(value & 0x8) {
				printk("Kernel detects device connect to PC!!!\n");
				printk("Release HD control from Device to PC!!!\n");

				/* 1. set primary ATA as input */
				iowrite32(0x2a, ATA_PRIMARY_DIR);
				/* 2. set GPIO34 direction as input,
					and show notificatino message */
				value = ioread32(MIS_GP1DATO) | 0x00000004;
				iowrite32(value, MIS_GP1DATO);
				//close interrupt flag
				iowrite32(0x08, MIS_UMSK_ISR_GP1A);
			}
		}
		//clear gpio interrupt flag assert
		iowrite32(0x80000, MIS_ISR);
	}

	/* check if the interrupt belongs to us */
	if(regValue & 0x00100000) { // GPIODA_INT
		if(is_neptune_cpu()) {
			/* check GPIO33 dis-assert flag */
			value = ioread32(MIS_UMSK_ISR_GP1DA);
			if(value & 0x8) {
				printk("Kernel detects Device removing from PC!!!\n");
				printk("Perform Software Reset!!!\n");

				/* 1. restore GPIO34 direction to output. */
				value = ioread32(MIS_GP1DATO) & (~0x00000004);
				iowrite32(value, MIS_GP1DATO);
				/* 2. restore primary ATA as output. */
				value = ioread32(ATA_PRIMARY_DIR) & 0xf;
				iowrite32(value, ATA_PRIMARY_DIR);
				/* 3. close interrupt flag. */
				iowrite32(0x08, MIS_UMSK_ISR_GP1A);
				/* 4. perform reset */
				printk("Software-reset device\n");
				iowrite32(NEPTUNE_RESET_ASSERT,
						(volatile unsigned int *)0xb8000004);
				iowrite32(NEPTUNE_ENABLE_ASSERT,
						(volatile unsigned int *)0xb8000000);
				iowrite32(NEPTUNE_RESET_DEASSERT,
						(volatile unsigned int *)0xb8000000);
				iowrite32(NEPTUNE_ENABLE_DEASSERT,
						(volatile unsigned int *)0xb8000004);

				outl(0, VENUS_MIS_TCWCR); //Reset timer to 0.

				/*
				asm("li $8, 0xbfc00000");
				asm("j  $8");
				__asm__ __volatile__("li $8, 0xbfc00000\n\t"
							"jr $8\n\t"
							"nop\n\t");
				while(1);
				*/
			}
		}
		//clear gpio interrupt flag assert
		iowrite32(0x100000, MIS_ISR);
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
	if(is_neptune_cpu()) {
		iowrite32(0x00000002, MIS_GP1IE); //Enable for GPIO 33.
	} else {
		iowrite32(0x0, MIS_GP1IE);
	}

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
