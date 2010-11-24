/*
 * main.c -- the bare se char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

//#define SIMULATOR

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

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/sched.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <platform.h>

#include "se_driver.h"
#include "SeReg.h"

#define SeClearWriteData 0

typedef enum
{
    SeWriteData = BIT0,
    SeGo = BIT1,
    SeEndianSwap = BIT2

} SE_CTRL_REG;

typedef enum
{
    SeIdle = BIT0

} SE_IDLE_REG;


//interrupt status and control bits
typedef enum
{
    SeIntCommandEmpty = BIT3,
    SeIntCommandError = BIT2,
    SeIntSync = BIT1

} SE_INT_STATUS_REG, SE_INT_CTRL_REG;


#define SEINFO_COMMAND_QUEUE0 0                         //The Definition of Command Queue Type: Command Queue 0
#define SEINFO_COMMAND_QUEUE1 1                         //The Definition of Command Queue Type: Command Queue 1
#define SEINFO_COMMAND_QUEUE2 2                         //The Definition of Command Queue Type: Command Queue 2

#include <linux/devfs_fs_kernel.h>

//#define DBG_PRINT(s, args...) printk(s, ## args)
#define DBG_PRINT(s, args...)

#ifdef SIMULATOR
    DWORD SB2_TILE_ACCESS_SET;
	DWORD DC_PICT_SET;

    //Command buffer manupolated registers
    DWORD SE_CmdBase;
    DWORD SE_CmdLimit;
    DWORD SE_CmdRdptr;
    DWORD SE_CmdWrptr;
    DWORD SE_CmdfifoState;
    DWORD SE_CNTL;
    DWORD SE_INT_STATUS;
    DWORD SE_INT_ENABLE;
    DWORD SE_INST_CNT;

    //Streaming engine registers
    DWORD SE_AddrMap;
    DWORD SE_Color_Key;
    DWORD SE_Def_Alpha;
    DWORD SE_Rslt_Alpha;
    DWORD SE_Src_Alpha;
    DWORD SE_Src_Color;
    DWORD SE_Clr_Format;
    DWORD SE_BG_Color;
    DWORD SE_BG_Alpha;

#define write_l(a, b) (((b)) = (a))
#define readl(a) ((a))

#else
#define write_l(a, b) {DBG_PRINT("write (0x%8x to reg(0x%8x)\n", (uint32_t)(a), (uint32_t)(b)); writel(a, b);}
#endif

#define endian_swap_32(a) (((a)>>24) | (((a)>>8) & 0xFF00) | (((a)<<8) & 0xFF0000) | (((a)<<24) & 0xFF000000))

typedef struct vsync_queue {
    uint32_t u32Occupied;
    uint32_t u32Size;
    uint32_t u32RdPtr;
    uint32_t u32WrPtr;
    uint32_t go;
    uint8_t *u8Buf[1];
} vsync_queue_t;

/*
 * Our parameters which can be set at load time.
 */

int se_major =   SE_MAJOR;
int se_minor =   0;
int se_nr_devs = SE_NR_DEVS;	/* number of bare se devices */
int se_quantum = SE_QUANTUM;
int se_qset =    SE_QSET;

module_param(se_major, int, S_IRUGO);
module_param(se_minor, int, S_IRUGO);
module_param(se_nr_devs, int, S_IRUGO);
module_param(se_quantum, int, S_IRUGO);
module_param(se_qset, int, S_IRUGO);

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

#define DG_LOCK
#define DG_UNLOCK
#define SE_IRQ 5


struct se_dev *se_devices;	/* allocated in se_init_module */

void WriteCmd(struct se_dev *dev, uint8_t *pbyCommandBuffer, int32_t lCommandLength, int go)
{
    uint32_t    dwDataCounter = 0;
    uint8_t     *pbyWritePointer = NULL;
    uint8_t     *pWptr = NULL;
    uint8_t     *pWptrLimit = NULL;
    volatile SEREG_INFO  *SeRegInfo = (volatile SEREG_INFO  *)0xB800C000;

    if(lCommandLength == 0) return;

    while(0 == (pbyWritePointer = (uint8_t *) SeRegInfo->SeCmdWritePtr[SEINFO_COMMAND_QUEUE1].Value))
    {
        int ii;
        for(ii=0; ii<64; ii++) ; //add some delay here
    }

    while(1)
    {
        uint8_t *pbyReadPointer;
        while(0 == (pbyReadPointer = (uint8_t *) SeRegInfo->SeCmdReadPtr[SEINFO_COMMAND_QUEUE1].Value))
        {
            int ii;
            for(ii=0; ii<64; ii++) ; //add some delay here
        }

        if(pbyReadPointer <= pbyWritePointer)
        {
            pbyReadPointer += dev->size;
        }

        if((pbyWritePointer + lCommandLength) < pbyReadPointer)
        {
            break;
        }
    }

    pWptrLimit = (uint8_t *) dev->CmdBuf + dev->size;
    pWptr = (uint8_t *) dev->CmdBuf + dev->wrptr;

    //DBG_PRINT("[\n");
    //Start writing command words to the ring buffer.
    for(dwDataCounter = 0; dwDataCounter < (uint32_t) lCommandLength; dwDataCounter += sizeof(uint32_t))
    {
        DBG_PRINT("(%8x-%8x)\n", (uint32_t)pWptr, *(uint32_t *)(pbyCommandBuffer + dwDataCounter));

        //*(uint32_t *)((uint32_t)pWptr) = endian_swap_32(*(uint32_t *)(pbyCommandBuffer + dwDataCounter));
        *(uint32_t *)((uint32_t)pWptr | 0xA0000000) = (*(uint32_t *)(pbyCommandBuffer + dwDataCounter));

        pWptr += sizeof(uint32_t);
        if(pWptr >= pWptrLimit)
        {
            pWptr = pWptr - dev->size;
        }
    }

    //DBG_PRINT("]\n");

    dev->wrptr = pWptr - (uint8_t *)dev->CmdBuf;

    DBG_PRINT("wreg= %x, value=%x\n", SeRegInfo->SeCmdWritePtr[SEINFO_COMMAND_QUEUE1].Value, (uint32_t) pWptr - dev->v_to_p_offset );

    if(go)
    {
        // sync the write buffer
        __asm__ __volatile__ (".set push");
        __asm__ __volatile__ (".set mips32");
        __asm__ __volatile__ ("sync;");
        __asm__ __volatile__ (".set pop");

        //convert to physical address
        SeRegInfo->SeCmdWritePtr[SEINFO_COMMAND_QUEUE1].Value = (uint32_t) pWptr - dev->v_to_p_offset ;
        SeRegInfo->SeCtrl[SEINFO_COMMAND_QUEUE1].Value = (SeGo | SeEndianSwap | SeWriteData);
    }
}

/* This function services keyboard interrupts. It reads the relevant
 *  * information from the keyboard and then scheduales the bottom half
 *   * to run when the kernel considers it safe.
 *    */
irqreturn_t se_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    struct se_dev *dev = dev_id;
    
  if(dev->isMars)
  {
    if((*(volatile uint32_t *)0xA00000DC) & endian_swap_32(0x2))
    {
        int ii;
        int dirty = 0;

        *(volatile uint32_t *)0xA00000DC = endian_swap_32(1);//(*(volatile uint32_t *)0xA00000DC) & endian_swap_32(~0x2);
//DBG_PRINT("@@ %x\n", endian_swap_32(*(volatile uint32_t *)0xA00000DC));

        for(ii=0; ii< dev->u32_max_queue_num; ii++)
        {
            vsync_queue_t *p_vsync_queue = (vsync_queue_t *)dev->p_vsync_queue;

            p_vsync_queue = (vsync_queue_t *)((uint32_t)p_vsync_queue + (uint32_t)(p_vsync_queue->u32Size + offsetof(vsync_queue_t, u8Buf)) * ii);
            DBG_PRINT("1: queue=%d, addr=%p\n", ii, p_vsync_queue);
            if(p_vsync_queue->go)
            {
                uint8_t *buf = (uint8_t *)&p_vsync_queue->u8Buf;
                uint32_t rdptr = p_vsync_queue->u32RdPtr;
                uint32_t u32Size = p_vsync_queue->u32Size;

                DBG_PRINT("1: queue=%d, r=%d, w=%d, size=%d\n", ii, p_vsync_queue->u32RdPtr, p_vsync_queue->u32WrPtr, *(uint32_t *)&buf[rdptr]);
                while(p_vsync_queue->u32RdPtr != p_vsync_queue->u32WrPtr)
                {
                    uint32_t len = *(uint32_t *)&buf[rdptr];
                    dirty = 1;

                DBG_PRINT("2: queue=%d, r=%d, w=%d, size=%d\n", ii, p_vsync_queue->u32RdPtr, p_vsync_queue->u32WrPtr, *(uint32_t *)&buf[rdptr]);

                    if(len == 0)
                    {
                        p_vsync_queue->u32RdPtr += 4;
                        if(p_vsync_queue->u32RdPtr >= p_vsync_queue->u32Size)
                        {
                            p_vsync_queue->u32RdPtr -= p_vsync_queue->u32Size;
                        }
                        break;
                    }
                    else
                    {
                        len -= 4;
                        rdptr += 4;
                        if(rdptr >= u32Size) rdptr -= u32Size;

                        if((rdptr + len) >= u32Size)
                        {
                            WriteCmd(dev, &buf[rdptr], u32Size - rdptr, 0);
                            WriteCmd(dev, &buf[0], rdptr + len - u32Size, 1);
                            rdptr = rdptr + len - u32Size;
                        }
                        else
                        {
                            WriteCmd(dev, &buf[rdptr], len, 1);
                            rdptr += len;
                        }
                        p_vsync_queue->u32RdPtr = rdptr;
                    }
                }
                if(p_vsync_queue->u32RdPtr == p_vsync_queue->u32WrPtr)
                {
                    p_vsync_queue->go = 0;
                }
            }
        }
        if(!dirty)
        {
            *(volatile uint32_t *)0xA00000DC = (*(volatile uint32_t *)0xA00000DC) & endian_swap_32(0);
        }
        return IRQ_HANDLED;
    }
  }
  else
  {
    uint32_t int_status = readl(SE_INT_STATUS);

    DBG_PRINT("[se driver] se int_status = %x\n", int_status);

    if(int_status & INT_SYNC/* interrupt souirce is from SYNC command*/)
    {
        uint32_t tmp;
        DBG_PRINT("se interrupt = %x\n", int_status);
        //clear interrupt
        write_l(SE_CLR_WRITE_DATA | INT_SYNC, SE_REG(SE_INT_STATUS));

        tmp = dev->hw_counter.low;
        dev->hw_counter.low += (INST_CNT_MASK+1);
        if(tmp > dev->hw_counter.low )  //check if overflow happen
            dev->hw_counter.high++;

        DBG_PRINT("[se driver] SE_REG(SE_INST_CNT) = 0x%x\n", readl(SE_REG(SE_INST_CNT)));
        DBG_PRINT("hw_counter.high = 0x%x\n", dev->hw_counter.high);
        DBG_PRINT("hw_counter.low = 0x%x\n", dev->hw_counter.low);
        //enable interrupt
        write_l(SE_CLR_WRITE_DATA | INT_SYNC, SE_REG(SE_INT_ENABLE));
        return IRQ_HANDLED;
    }

  }
  return IRQ_NONE;
}
/*
 * Open and close
 */

int se_open(struct inode *inode, struct file *filp)
{
	struct se_dev *dev; /* device information */
    int result;

	DBG_PRINT(KERN_INFO "se open\n");

	dev = container_of(inode->i_cdev, struct se_dev, cdev);
	filp->private_data = dev; /* for other methods */

    DBG_PRINT("se se_dev = %x\n", (uint32_t)dev);
	/* now trim to 0 the length of the device if open was write-only */
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

    if(!dev->initialized)
    {
        result = request_irq(SE_IRQ, se_irq_handler, SA_INTERRUPT|SA_SHIRQ, "se", dev);
        if(result)
	    {
            DBG_PRINT(KERN_INFO "se: can't get assigned irq%i\n", SE_IRQ);
	    }

        dev->initialized = 1;
        dev->p_vsync_queue = 0;
        dev->isMars = is_mars_cpu() ? 1 : 0;
        *(volatile uint32_t *)0xA00000DC = 0;
    }
        
    if(dev->isMars)
    {
		if(!dev->CmdBuf)
        {
			uint32_t dwPhysicalAddress = 0;
            volatile SEREG_INFO  *SeRegInfo = (volatile SEREG_INFO  *)0xB800C000;
        
        
			dev->size = (SE_COMMAND_ENTRIES * sizeof(uint32_t));
        
			dev->CmdBuf = kmalloc((SE_COMMAND_ENTRIES * sizeof(uint32_t)), GFP_KERNEL);

            dwPhysicalAddress = (uint32_t)__pa(dev->CmdBuf);
        
			DBG_PRINT("Command Buffer Address = %x, Physical Address = %x\n", (uint32_t) dev->CmdBuf, (uint32_t) dwPhysicalAddress);
        
            dev->v_to_p_offset = (int32_t) ((uint32_t)dev->CmdBuf - (uint32_t)dwPhysicalAddress);
            dev->wrptr = 0;
			dev->CmdBase = (void *) dwPhysicalAddress;
			dev->CmdLimit = (void *) (dwPhysicalAddress + SE_COMMAND_ENTRIES * sizeof(uint32_t));
        
            //Stop Streaming Engine
            SeRegInfo->SeCtrl[SEINFO_COMMAND_QUEUE1].Value = (SeGo | SeEndianSwap | SeClearWriteData);
            SeRegInfo->SeCtrl[SEINFO_COMMAND_QUEUE1].Value = (SeEndianSwap | SeWriteData);
        
			SeRegInfo->SeCmdBase[SEINFO_COMMAND_QUEUE1].Value = (uint32_t) dev->CmdBase;
			SeRegInfo->SeCmdLimit[SEINFO_COMMAND_QUEUE1].Value = (uint32_t) dev->CmdLimit;
			SeRegInfo->SeCmdReadPtr[SEINFO_COMMAND_QUEUE1].Value = (uint32_t) dev->CmdBase;
			SeRegInfo->SeCmdWritePtr[SEINFO_COMMAND_QUEUE1].Value = (uint32_t) dev->CmdBase;
			SeRegInfo->SeInstCnt[SEINFO_COMMAND_QUEUE1].Value = 0;
        }
    }
	up(&dev->sem);
	return 0;          /* success */
}

int se_release(struct inode *inode, struct file *filp)
{
	struct se_dev *dev = filp->private_data; /* for other methods */

    dev->initialized = 0;
    dev->hw_counter.low = 0;
    dev->hw_counter.high = 0;
    write_l(SE_CLR_WRITE_DATA | INT_SYNC, SE_REG(SE_INT_ENABLE));

	//stop SE
	write_l(SE_GO | SE_CLR_WRITE_DATA, SE_REG(SE_CNTL));

    free_irq(SE_IRQ, dev);

	DBG_PRINT(KERN_INFO "se release\n");

	return 0;
}

/*
 * Data management: read and write
 */

ssize_t se_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	return -EFAULT;
}

ssize_t se_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    char data[32];
	struct se_dev *dev = filp->private_data;
	ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

    if(count & 0x3)
    {
		retval = -EFAULT;
		goto out;
    }

	if (copy_from_user(data, buf, count))
    {
		retval = -EFAULT;
		goto out;
    }
    WriteCmd(dev, (uint8_t *)data, count, 1);

	*f_pos += count;
	retval = count;

  out:
	up(&dev->sem);
	return retval;
}

/*
 * The ioctl() implementation
 */

int se_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	struct se_dev *dev = filp->private_data;
	int retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
    
    //DBG_PRINT("se ioctl code = %d\n", cmd);
	switch(cmd) {
    case SE_IOC_SET_VSYNC_QUEUE:
        {
        vsync_queue_param_t vsync_queue_param;

        if (copy_from_user((void *)&vsync_queue_param, (const void __user *)arg, sizeof(vsync_queue_param_t))) {
            retval = -EFAULT;
            DBG_PRINT("se ioctl code=SE_IOC_SET_DCU_INFO failed!!!!!!!!!!!!!!!1\n");
            goto out;
        }
        //convert to non cached virtual
        dev->p_vsync_queue = (void *)((uint32_t)vsync_queue_param.vsync_queue_phy + 0xA0000000); 
        dev->u32_max_queue_num = vsync_queue_param.u32_max_queue_num;
        }
		break;
    case SE_IOC_READ_HW_CMD_COUNTER:
        {
            se_cmd_counter counter;
            counter.low = dev->hw_counter.low | (readl(SE_REG(SE_INST_CNT)) & INST_CNT_MASK);
            counter.high = dev->hw_counter.high;
            if (copy_to_user((void __user *)arg, (void *)&counter, sizeof(se_cmd_counter))) {
                retval = -EFAULT;
                goto out;
            }
            DBG_PRINT("counter.high = 0x%x\n", counter.high);
            DBG_PRINT("counter.low = 0x%x\n", counter.low);
            DBG_PRINT("se ioctl code=SE_IOC_READ_HW_CMD_COUNTER:\n");
            break;
        }
    default:  /* redundant, as cmd was checked against MAXNR */
        DBG_PRINT("se ioctl code not supported\n");
		retval = -ENOTTY;
        goto out;
	}
  out:
	up(&dev->sem);
	return retval;
}


/*
 * The "extended" operations -- only seek
 */

loff_t se_llseek(struct file *filp, loff_t off, int whence)
{
	return -EINVAL;
}



struct file_operations se_fops = {
	.owner =    THIS_MODULE,
	.llseek =   se_llseek,
	.read =     se_read,
	.write =    se_write,
	.ioctl =    se_ioctl,
	.open =     se_open,
	.release =  se_release,
};

/*
 * Finally, the module stuff
 */

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void se_cleanup_module(void)
{
	int i;
	dev_t devno = MKDEV(se_major, se_minor);

	DBG_PRINT(KERN_INFO "se clean module se_major = %d\n", se_major);

	//free_irq(SE_IRQ, NULL);
	/* Get rid of our char dev entries */
	if (se_devices) {
		for (i = 0; i < se_nr_devs; i++) {
			if(se_devices[i].CmdBuf) {
				kfree(se_devices[i].CmdBuf);
			}
			cdev_del(&se_devices[i].cdev);
		}
		kfree(se_devices);
	}

	//stop SE
	write_l(SE_GO | SE_CLR_WRITE_DATA, SE_REG(SE_CNTL));

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, se_nr_devs);
}


/*
 * Set up the char_dev structure for this device.
 */
static void se_setup_cdev(struct se_dev *dev, int index)
{
	int err, devno = MKDEV(se_major, se_minor + index);
    
	cdev_init(&dev->cdev, &se_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &se_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		DBG_PRINT(KERN_NOTICE "Error %d adding se%d", err, index);
}


int se_init_module(void)
{
	int result, i;
	dev_t dev = 0;

/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */

	if (se_major) {
		dev = MKDEV(se_major, se_minor);
		result = register_chrdev_region(dev, se_nr_devs, "se");
	} else {
		result = alloc_chrdev_region(&dev, se_minor, se_nr_devs,
				"se");
		se_major = MAJOR(dev);
	}
	if (result < 0) {
		DBG_PRINT(KERN_WARNING "se: can't get major %d\n", se_major);
		return result;
	}

	printk("se init module major number = %d\n", se_major);

        devfs_mk_cdev(MKDEV(se_major, se_minor), S_IFCHR|S_IRUSR|S_IWUSR, "se0");

        /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	se_devices = kmalloc(se_nr_devs * sizeof(struct se_dev), GFP_KERNEL);
	if (!se_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(se_devices, 0, se_nr_devs * sizeof(struct se_dev));

        /* Initialize each device. */
	for (i = 0; i < se_nr_devs; i++) {
            //initialize device structure
		//se_devices[i].= ;
		//se_devices[i].= ;
		init_MUTEX(&se_devices[i].sem);
		se_setup_cdev(&se_devices[i], i);
	}

        /* At this point call the init function for any friend device */
	dev = MKDEV(se_major, se_minor + se_nr_devs);

	return 0; /* succeed */

  fail:
	se_cleanup_module();
	return result;
}

module_init(se_init_module);
module_exit(se_cleanup_module);
