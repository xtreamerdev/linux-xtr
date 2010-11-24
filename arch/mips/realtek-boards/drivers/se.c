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

#include "se_driver.h"
#include "sb2_reg.h"
#include "dcu_reg.h"

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


/* This function services keyboard interrupts. It reads the relevant
 *  * information from the keyboard and then scheduales the bottom half
 *   * to run when the kernel considers it safe.
 *    */
irqreturn_t se_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{

    struct se_dev *dev = dev_id;
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
        if(tmp > dev->hw_counter.low )	//check if overflow happen
            dev->hw_counter.high++;

        DBG_PRINT("[se driver] SE_REG(SE_INST_CNT) = 0x%x\n", readl(SE_REG(SE_INST_CNT)));
        DBG_PRINT("hw_counter.high = 0x%x\n", dev->hw_counter.high);
        DBG_PRINT("hw_counter.low = 0x%x\n", dev->hw_counter.low);
		//enable interrupt
        write_l(SE_CLR_WRITE_DATA | INT_SYNC, SE_REG(SE_INT_ENABLE));
        return IRQ_HANDLED;
    }
    DBG_PRINT("not se interrupt = %x\n", int_status);
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
	//if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
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
        }
        
#if 0
		if(!dev->CmdBuf)
		{
            int result;
			uint32_t *pPhysAddr;

            DBG_PRINT("se initialization\n");

			dev->size = SE_COMMAND_ENTRIES*sizeof(uint32_t);

			dev->CmdBuf = kmalloc(dev->size, GFP_KERNEL);
			pPhysAddr = (uint32_t *)__pa(dev->CmdBuf);
			DBG_PRINT("CmdBuf virt addr = %x, phy addr = %x\n", (uint32_t)dev->CmdBuf, (uint32_t)pPhysAddr);
            dev->v_to_p_offset = (int32_t)((uint8_t *)dev->CmdBuf - (uint32_t)pPhysAddr);

            dev->wrptr = 0;
			dev->CmdBase = pPhysAddr;
			dev->CmdLimit = pPhysAddr + SE_COMMAND_ENTRIES;

  			write_l((uint32_t)dev->CmdBase, SE_REG(SE_CmdBase));
            write_l((uint32_t)dev->CmdLimit, SE_REG(SE_CmdLimit));
	      	write_l((uint32_t)dev->CmdBase, SE_REG(SE_CmdRdptr));
            write_l((uint32_t)dev->CmdBase, SE_REG(SE_CmdWrptr));
            write_l(0, SE_REG(SE_INST_CNT));

            result = request_irq(SE_IRQ, se_irq_handler, SA_INTERRUPT|SA_SHIRQ, "se", dev);
            if(result)
			{
                DBG_PRINT(KERN_INFO "se: can't get assigned irq%i\n", SE_IRQ);
			}
			//enable interrupt
            write_l(SE_SET_WRITE_DATA | INT_SYNC, SE_REG(SE_INT_ENABLE));

            //start SE
            write_l(/*readl(SE_REG(SE_CNTL)) |*/ SE_GO | SE_SET_WRITE_DATA, SE_REG(SE_CNTL));
		}
#endif
		up(&dev->sem);
	//}
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

/********************************************
 * Function :
 * Description :
 * Input :  struct se_dev : hold driver data
 *          nLen : number of bytes
 *          pdwBuf : command buffer
 * Output : 
 *******************************************/
void
WriteCmd(struct se_dev *dev, uint8_t *pBuf, int nLen)
{
    int ii;
    uint8_t *writeptr;
    uint8_t *pWptr;
    uint8_t *pWptrLimit;

    dev->sw_counter.low++;
	if(dev->sw_counter.low == 0)
    {
        dev->sw_counter.high++;
    }
    if( (dev->sw_counter.low & INST_CNT_MASK) == 0)
    {   //Insert a SYNC command and enable interrupt for updating the upper 48 bits counter in software.
        uint32_t dwValue = SYNC;
		//recursive call
        WriteCmd(dev, (uint8_t *)&dwValue, sizeof(uint32_t));      //recursive
    }

    DBG_PRINT("[queue size, wptr, rptr] =");
    DBG_PRINT("[0x%x, ", dev->size);
    DBG_PRINT("0x%x, ", readl(SE_REG(SE_CmdWrptr)));
    DBG_PRINT("0x%x]\n", readl(SE_REG(SE_CmdRdptr)));
	DBG_PRINT("SE_REG(SE_INST_CNT) %x\n", readl(SE_REG(SE_INST_CNT)));
	DBG_PRINT("SE_REG(SE_CmdFifoState) %x\n", readl(SE_REG(SE_CmdFifoState)));
	DBG_PRINT("SE_REG(SE_CNTL) %x\n", readl(SE_REG(SE_CNTL)));
	DBG_PRINT("SE_REG(SE_INT_STATUS) %x\n", readl(SE_REG(SE_INT_STATUS)));
    writeptr = (uint8_t *)readl(SE_REG(SE_CmdWrptr));
#ifndef SIMULATOR
    while(1)
    {
        uint8_t *readptr = (uint8_t *)readl(SE_REG(SE_CmdRdptr));
        if(readptr <= writeptr)
        {
            readptr += dev->size; 
        }
        if((writeptr+nLen) < readptr)
        {
            break;
        }
	    DBG_PRINT("while loop w=%x r=%x\n", (uint32_t)writeptr, (uint32_t)readptr);
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(1);
    }
#endif

    pWptrLimit = (uint8_t *)(dev->CmdBuf + dev->size);
	while(1)
    {   //handle the condition that command buffer wrapped around inside a 2-/3-word command
        //causes SE decording error
        //restore write pointer afater recusive call
        pWptr = dev->CmdBuf + dev->wrptr;
 	    //if((nLen > 4) && ((pWptr + sizeof(uint32_t)) == pWptrLimit))
 	    if((nLen > 4) && (pWptr + nLen) >= pWptrLimit)
        {
            uint32_t cmd_word = 0x00000071; //dummy command by writing 0 to SE_Input register

            //recursive call
            WriteCmd(dev, (uint8_t *)&cmd_word, sizeof(uint32_t));
        }
		else
        {
            break;
        }
    }

    DBG_PRINT("kernel land [");
    //Start writing command words to the ring buffer.
    for(ii=0; ii<nLen; ii+=sizeof(uint32_t))
    {
        DBG_PRINT("(%x-%x)-", (uint32_t)pWptr, *(uint32_t *)(pBuf + ii));


        *(uint32_t *)((uint32_t)pWptr | 0xA0000000) = endian_swap_32(*(uint32_t *)(pBuf + ii));

        pWptr += sizeof(uint32_t);
        if(pWptr >= pWptrLimit)
            pWptr = dev->CmdBuf;
    }
    DBG_PRINT("]\n");

    dev->wrptr += nLen;
    if(dev->wrptr >= dev->size)
        dev->wrptr -= dev->size;

    //convert to physical address
    pWptr -= dev->v_to_p_offset;
	write_l( SE_GO | SE_CLR_WRITE_DATA, SE_REG(SE_CNTL));
    write_l((uint32_t)pWptr, SE_REG(SE_CmdWrptr));    
    write_l( SE_GO | SE_SET_WRITE_DATA, SE_REG(SE_CNTL));

    return;
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
    WriteCmd(dev, (uint8_t *)data, count);

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
	int ii, retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
    
    //DBG_PRINT("se ioctl code = %d\n", cmd);
	switch(cmd) {
	case SE_IOC_DG_LOCK:
        {
            /* register related information to SB2 to do block to linear address translation */
            ioc_dg_lock_param param;

            if (copy_from_user((void *)&param, (const void __user *)arg, sizeof(ioc_dg_lock_param))) {
                retval = -EFAULT;
                goto out;
            }

#ifndef SIMULATOR
            while(1)
            {
				DBG_PRINT("SE_REG(SE_INST_CNT) %x\n", readl(SE_REG(SE_INST_CNT)));
				DBG_PRINT("dev->hw_counter.high = %x\n", dev->hw_counter.high);
				DBG_PRINT("dev->hw_counter.low = %x\n", (dev->hw_counter.low + (readl(SE_REG(SE_INST_CNT)) & 0xFFFF)));
				DBG_PRINT("param->sw_counter.high = %x\n", param.sw_counter.high);
				DBG_PRINT("param->sw_counter.low = %x\n", param.sw_counter.low);
		        if((dev->hw_counter.high > param.sw_counter.high) ||
		           ((dev->hw_counter.high == param.sw_counter.high)
                   && ((dev->hw_counter.low + (readl(SE_REG(SE_INST_CNT)) & INST_CNT_MASK)) >= param.sw_counter.low)))
                    break;
                //Wait for all the pending command associated to this surface is serviced.
                set_current_state(TASK_INTERRUPTIBLE);
                schedule_timeout(1);
            }
#endif
            for(ii=0; ii<6; ii++)
            {
                if((param.pic_width >> (ii+8)) == 1)
                {
                    break;
                }
            }
            //DBG_PRINT("param.pic_width = %d\n", ii);
	
            //enable SB2 linear address to block address conversion
            write_l( PrepareTileStartAddrWriteData(param.tile_start_addr) 
			        | PreparePicWidthWriteData(ii) 
			        | PreparePicIndexWriteData(param.pic_index)
                    , SB2_REG(SB2_TILE_ACCESS_SET));
         
            DBG_PRINT("se ioctl code=DG_LOCK\n");
		    break;
        }
    case SE_IOC_DG_UNLOCK:
        /* register related information to SB2 to do block to linear address translation */

        //reset SB2 linear address to block address conversion
        write_l( PrepareTileStartAddrWriteData(0) 
		        | PreparePicWidthWriteData(0) 
		        | PreparePicIndexWriteData(0)
                , SB2_REG(SB2_TILE_ACCESS_SET));

        DBG_PRINT("se ioctl code=DG_UNLOCK\n");
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
    case SE_IOC_READ_SW_CMD_COUNTER:
        {
            DBG_PRINT("se ioctl code=SE_IOC_READ_SW_CMD_COUNTER:\n");
            if (copy_to_user((void __user *)arg, (void *)&(dev->sw_counter), sizeof(se_cmd_counter))) {
                retval = -EFAULT;
                goto out;
            }
            break;
        }
    case SE_IOC_SET_DCU_INFO:
        {
            ioc_dcu_info info;

            DBG_PRINT("se ioctl code=SE_IOC_SET_DCU_INFO\n");
            if (copy_from_user((void *)&info, (const void __user *)arg, sizeof(ioc_dcu_info))) {
                retval = -EFAULT;
                DBG_PRINT("se ioctl code=SE_IOC_SET_DCU_INFO failed!!!!!!!!!!!!!!!1\n");
                goto out;
            }
            write_l( PreparePictSetWriteData(info.index, info.pitch, info.addr),
                             DCU_REG(DC_PICT_SET));
            write_l( DC_PICT_SET_OFFSET_pict_set_num(info.index)
                     | DC_PICT_SET_OFFSET_pict_set_offset_x(0)
                     | DC_PICT_SET_OFFSET_pict_set_offset_y(0),
                     DCU_REG(DC_PICT_SET_OFFSET));


            DBG_PRINT("se ioctl code=SE_IOC_SET_DCU_INFO %x\n", PreparePictSetWriteData(info.index, info.pitch, info.addr));
			DBG_PRINT("index=%x, pitch=%x, addr=%x\n", info.index, info.pitch, info.addr);
//#define TEST_VOUT
#ifdef TEST_VOUT
            write_l( (((info.addr) >> (11+((readl(DCU_REG(DC_SYS_MISC)) >> 16) & 0x3))) & 0x1FFF), ((volatile unsigned int *)0xB80050A0));
#endif
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
