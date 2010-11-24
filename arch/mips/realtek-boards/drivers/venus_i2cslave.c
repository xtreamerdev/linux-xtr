/* ------------------------------------------------------------------------- */
/* i2cslave-venus.c i2cslave hw access for Realtek Venus DVR I2C                       */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2007 yunfeng han <yunfeng_han@realsil.com.cn>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    Version 1.0 written by yunfeng_han@realsil.com.cn						*/
/* ------------------------------------------------------------------------- */
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
#include "venus_i2cslave.h"

#define VERSION "I2CSLAVE 1.0 20071106 (neptune & venus)"
#define WAIT_TIME	4*HZ

#define CONFIG_I2C_DEBUG_BUS
#ifdef CONFIG_I2C_DEBUG_BUS
#define i2cslave_debug(fmt, args...)		printk(KERN_INFO fmt, ## args)
#else
#define i2cslave_debug(fmt, args...)
#endif

enum {
    STANDARD_MODE = 1,
    FAST_MODE = 2,
    HIGH_SPEED_MODE = 3,
};
#define MAX_REGISTER 32

static int index = 0;
char * pRegister;
char RecvDataBuf[16];

typedef struct __RegProperty{
	unsigned char RDFlag; //0  for rw, 1 only for Read, 2 only for write 
	unsigned char width;//0 1byte,1 2bytes, 2 4bytes, 3 8bytes
	char *pAddr;//Reg Start Address
}RegProperty;

RegProperty RegPro[MAX_REGISTER];

//static char RegOffset1[8]; //used for user to read data from master i2c
//static char RegOffset2[8]; //used for user to write data to master i2c
/*
static int Reg1Updated;
static int Reg2Updated;
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
*/
static int result  = 0;
static int i2cslave_speed = 1;
module_param(i2cslave_speed, int, S_IRUGO);

static dev_t dev_venus_i2cslave = 0;
static struct cdev *venus_i2cslave_cdev 	= NULL;

static struct platform_device *venus_i2cslave_devs;
/*static struct device_driver venus_i2cslave_driver = {
    .name       = "Venusi2cslave",
    .bus        = &platform_bus_type,
    .suspend    = venus_i2cslave_suspend,
    .resume     = venus_i2cslave_resume,
};
*/
static DECLARE_WAIT_QUEUE_HEAD(venus_i2cslave_send_wait);
//static int VENUS_i2cslave_TX_FIFO_DEPTH;
//static int VENUS_i2cslave_RX_FIFO_DEPTH;
//static int LOW_SPEED;


static int venus_i2cslave_suspend(struct device *dev, pm_message_t state, u32 level) {
	/*disable DW_apb_i2cslave */
	writel(0, IC_ENABLE);
	return 0;
}

static int venus_i2cslave_resume(struct device *dev, u32 level) {
	
	/*enable DW_apb_i2cslave */
	writel(0x1, IC_ENABLE);
	return 0;
}

int i2cslave_venus_open(struct inode *inode, struct file *filp) {

	return 0;	/* success */
}

static struct device_driver venus_i2cslave_driver = {
     .name       = "Venusi2cslave",
     .bus        = &platform_bus_type,
     .suspend    = venus_i2cslave_suspend,
     .resume     = venus_i2cslave_resume,
 };

int i2cslave_venus_ioctl(struct inode * inode, struct file * filp, 
			unsigned int cmd, unsigned long arg) {
	int err = 0;
	int retval = 0;
	unsigned char RegIndex;
	unsigned char RegRDFlag;
	unsigned char Regwidth;
	if (_IOC_TYPE(cmd) != VENUS_I2C_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VENUS_I2C_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) 
		return -EFAULT;

	if (!capable (CAP_SYS_ADMIN))
		return -EPERM;

	switch(cmd) {
		case VENUS_I2C_IOC_CONFIG:
			i2cslave_debug("Venus i2cslave:entering VENUS_I2C_IOC_CONFIG\n");
			//if(sizeof(arg)<3)
			//	return -EFAULT;
			RegIndex = *(unsigned char *)arg;
			if(RegIndex > MAX_REGISTER)
				return -EFAULT;
			
			RegRDFlag = *(char *)(arg+1);
			if(RegRDFlag > 2)
				return -EFAULT;
			
			Regwidth = *(char *)(arg+2);
			if(Regwidth > 3)
				return -EFAULT;
			
			RegPro[RegIndex].RDFlag = RegRDFlag;
			RegPro[RegIndex].width= Regwidth;
			break;
		case VENUS_I2C_IOC_READ:
			i2cslave_debug("Venus i2cslave:entering VENUS_I2C_IOC_READ\n");
			RegIndex = *(unsigned char *)arg;
			if(RegIndex > MAX_REGISTER)
				return -EFAULT;
			i2cslave_debug("Venus i2cslave:regindex %d\n",RegIndex);
			//check the RDFlag
			RegRDFlag = RegPro[RegIndex].RDFlag;
			if((RegRDFlag != 0) &&(RegRDFlag != 2))
				return  -EFAULT;
			Regwidth = 1 << RegPro[RegIndex].width;
			if(Regwidth > 8)return -EFAULT;
			if (copy_to_user((char *)(arg+1), 
				   	RegPro[RegIndex].pAddr,
				   	Regwidth))
				return -EFAULT;
				break;
		case VENUS_I2C_IOC_WRITE:
			i2cslave_debug("Venus i2cslave:entering VENUS_I2C_IOC_WRITE\n");
			RegIndex = *(unsigned char *)arg;
			if(RegIndex > MAX_REGISTER)
				return -EFAULT;
			i2cslave_debug("Venus i2cslave:regindex %d\n",RegIndex);
			//check the RDFlag
			RegRDFlag = RegPro[RegIndex].RDFlag;
			if((RegRDFlag != 0) &&(RegRDFlag != 1))
				return  -EFAULT;
			Regwidth = 1 << RegPro[RegIndex].width;
			if(Regwidth > 8)return -EFAULT;
			if (copy_from_user(RegPro[RegIndex].pAddr,
				   (char *)(arg+1), 
				   Regwidth))
			return -EFAULT;
				
			//	i2cslave_debug("Venus i2cslave:%llx\n",*((long long *)RegOffset2));
			
				break;
			
		default:
			retval = -ENOIOCTLCMD;
	}

	return retval;
}

static irqreturn_t i2cslave_venus_handler(int this_irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int regValue;
	unsigned int RegSubaddr;
	unsigned char RDFlag;
	unsigned char RegSize;
	unsigned int num;
	int i;
	unsigned long timeout = WAIT_TIME;
	//i2cslave_debug("Venus i2cslave:  1111111111111111111111.\n");
	if(readl(MIS_ISR) & 0x00000010) {	// interrupt belongs to i2cslave
		
		i2cslave_debug("Venus i2cslave: Interrupt Handler Triggered.\n");
	
		regValue = readl(IC_INTR_STAT);

		if (regValue & RD_REQ_BIT) { 
			i2cslave_debug("Venus i2cslave: Read Request Interrupt\n");
			num = readl(IC_RXFLR);
			if(num < 1){
				i2cslave_debug("Venus i2cslave: No SubRegAddr Received\n");
				readl(IC_CLR_RD_REQ);//clear RD_REQ_BIT interrupt
				return IRQ_HANDLED;
			}
			//read subadress of reg
			RegSubaddr = (unsigned char)(readl(IC_DATA_CMD) & 0xff);

			//if(RegSubaddr != 0x02)//0x02 stands for RegOffset2
			//	return IRQ_NONE;
			i2cslave_debug("Venus i2cslave:RegSubaddr %d\n", RegSubaddr);

			//check the RDFlag
			RDFlag = RegPro[RegSubaddr].RDFlag;
			if((RDFlag != 0)&&(RDFlag != 1)){ //write only
				i2cslave_debug("Venus i2cslave: RD REQ Flag error\n");
				readl(IC_CLR_RD_REQ);//clear RD_REQ_BIT interrupt
				return IRQ_HANDLED;
			}
			//find the Regsize of the RegSubaddr
			RegSize = 1 << RegPro[RegSubaddr].width;
			if(RegSize >8){
				i2cslave_debug("Venus i2cslave: RD REQ Regsize error\n");
				return IRQ_HANDLED;
			}
			for(i=0;i<RegSize;i++){
				while(!(readl(IC_STATUS) & 0x2)){ // wait while transmit fifo is full
				if (time_after(jiffies, timeout)){
					i2cslave_debug("Venus i2cslave:time out(FIFO full)\n");
					return IRQ_HANDLED;
				}
			//	udelay(100); // 10 ^ -4 seconds (enough for 1 byte to transmit)
				}

				writel(((unsigned int)(*(RegPro[RegSubaddr].pAddr+i)&0xff)), IC_DATA_CMD);
			}
			
			readl(IC_CLR_RD_REQ);//clear RD_REQ_BIT interrupt
			writel(0x10, MIS_ISR);
			return IRQ_HANDLED;
			
		} else if(regValue & RX_FULL_BIT) {	

			i2cslave_debug("Venus i2cslave: Receive FIFO Full Interrupt\n");
			//reinitialize index for next transfer
			index = 0;
			while(num=readl(IC_RXFLR)){
				i2cslave_debug("Venus i2cslave: RX NUM %d\n",num);
				for(i = 0 ; i < num ; i++) {
					//read to fifo
					RecvDataBuf[index++]= (unsigned char)(readl(IC_DATA_CMD) & 0xff);
				}
				udelay(100);
			}
			//write the data to the SW Register
			//Get the Subaddr and Reg width
			RegSubaddr = RecvDataBuf[0];
			//check the RDFlag
			RDFlag = RegPro[RegSubaddr].RDFlag;
			if((RDFlag != 0)&&(RDFlag != 2)){
				i2cslave_debug("Venus i2cslave: RX FULL Flag error\n");
				return IRQ_HANDLED;
			}
			//find the Regsize of the RegSubaddr
			RegSize = 1 << RegPro[RegSubaddr].width;
			if(RegSize >8){
				i2cslave_debug("Venus i2cslave: RX FULL Regsize error\n");
				return IRQ_HANDLED;
			}
			i2cslave_debug("Venus i2cslave: RegAddr %d RegSize %d\n",RegSubaddr,RegSize);
			
			for(i=0;i<index&&i<RegSize;i++)
				*(RegPro[RegSubaddr].pAddr+i)=	RecvDataBuf[i+1];
			
			
			writel(0x10, MIS_ISR);
			return IRQ_HANDLED;
			
		} 
	/*	else if(regValue & RX_DONE_BIT) {	
			i2cslave_debug("Venus i2cslave: Receive Done Interrupt\n");
			num=readl(IC_RXFLR);
			for(i = 0 ; i < num ; i++) {
				//read to fifo
				RecvDataBuf[index++]= (unsigned char)(readl(IC_DATA_CMD) & 0xff);
			}
			//write the data to the SW Register
			//Get the Subaddr and Reg width
			RegSubaddr = RecvDataBuf[0];
			//check the RDFlag
			RDFlag = RegPro[RegSubaddr].RDFlag;
			if(RDFlag == 1){
				readl(IC_CLR_RX_DONE);//clear RX_DONE_BIT interrupt
				return IRQ_HANDLED;
			}
			//find the Regsize of the RegSubaddr
			RegSize = 1 << RegPro[RegSubaddr].width;
			
			for(i=0;i<index&&i<RegSize;i++)
				*(RegPro[RegSubaddr].pAddr+i)=	RecvDataBuf[i+1];
			//reinitialize index for next transfer
			index = 0;
			
			readl(IC_CLR_RX_DONE);
			writel(0x10, MIS_ISR);
			return IRQ_HANDLED;
			
		}
*/
		else {
			i2cslave_debug("unexpected interrupt(%x)\n", regValue);
			/* clear interrupt flag */
			readl(IC_CLR_INTR);
			readl(IC_CLR_RX_UNDER);
			readl(IC_CLR_TX_OVER);
			readl(IC_CLR_RX_DONE);
			readl(IC_CLR_ACTIVITY);
			readl(IC_CLR_GEN_CALL);

			writel(0x10, MIS_ISR);
			//wake_up_interruptible(&venus_i2cslave_send_wait);
			
			return IRQ_HANDLED;
		}
	}
	else return(IRQ_NONE);
}


struct file_operations venus_i2cslave_fops = {
	.owner =    THIS_MODULE,
	.open  =    i2cslave_venus_open,
	//.read  =    i2cslave_venus_read,
	//.write  =   i2cslave_venus_write,
	.ioctl =    i2cslave_venus_ioctl,
};


static int __init i2cslave_venus_module_init(void) 
{
	int i;
	if(i2cslave_speed > 3 || i2cslave_speed < 1)
		i2cslave_speed = 1;
	printk(KERN_INFO "%s\n", VERSION);

	/* examing GPIO#5/GPIO#4 belongs to i2cslave */
	if((readl(MIS_PSELL) & 0x00000f00) != 0x00000500) {
		printk(KERN_ERR "i2cslave-venus: pins are not defined as i2cslave\n");
		return -ENODEV;
	}
	/* MKDEV */
	dev_venus_i2cslave = MKDEV(VENUS_I2C_MAJOR, VENUS_I2C_MINOR);

	/* Request Device Number */
	result = register_chrdev_region(dev_venus_i2cslave, VENUS_I2C_DEVICE_NUM, "Venus_i2cslave");
	if(result < 0) {
		printk(KERN_WARNING "i2cslave-venus: can't register device number.\n");
		goto fail_alloc_dev;
	}

	venus_i2cslave_devs = platform_device_register_simple("Venus_i2cslave", -1, NULL, 0);
	if(driver_register(&venus_i2cslave_driver) != 0)
		goto fail_device_register;


 
	//Request Virtual I2C Register memory
	//MAX_REGISTER*8 byte 
	//maxmium 8 byte for each register
	//user can use ioctl code VENUS_I2C_IOC_CONFIG to config which one to read or which one to write
	if((pRegister=kmalloc(MAX_REGISTER*8*sizeof(char),GFP_KERNEL))==NULL){
		printk(KERN_ERR "i2cslave-venus: kmalloc failed\n");
		goto fail_alloc_memory;
	}
	
	
	if (request_irq(VENUS_I2C_IRQ, i2cslave_venus_handler, SA_INTERRUPT|SA_SHIRQ, "Venus_i2cslave", i2cslave_venus_handler) < 0) {
		printk(KERN_ERR "i2cslave-venus: Request irq%d failed\n", VENUS_I2C_IRQ);
		goto fail_req_irq;
	}

	//VENUS_i2cslave_TX_FIFO_DEPTH = ((readl(IC_COMP_PARAM_1) & 0xff0000) >> 16) + 1;
	//VENUS_i2cslave_RX_FIFO_DEPTH = ((readl(IC_COMP_PARAM_1) & 0xff00) >> 8) + 1;
	//printk(KERN_INFO "i2cslave-venus: TX FIFO Depth = %d, RX FIFO Depth = %d\n", VENUS_i2cslave_TX_FIFO_DEPTH, VENUS_i2cslave_RX_FIFO_DEPTH);

	/* hardware initialization */
	/*disable DW_apb_i2cslave to config IC_SAR*/
	writel(0x0, IC_ENABLE);
	writel(VENUS_MASTER_7BIT_ADDR & 0x7f, IC_SAR);
	
	/*write IC_CON register, 7-bit slave mode 
	matser mode disabled*/
	writel((0x20 | (i2cslave_speed << 1)), IC_CON);

	// 33KHz speed decision
	writel(0x16e, IC_SS_SCL_HCNT);
	writel(0x1ad, IC_SS_SCL_LCNT);
	
	//set receive fifo threshold level 2 entrys 
	writel(0x01,IC_RX_TL);
	
	/*enable DW_apb_i2cslave */
	writel(0x1, IC_ENABLE);
	
	/* enable i2cslave interrupt */
	writel(RD_REQ_BIT|RX_FULL_BIT|RX_DONE_BIT, IC_INTR_MASK);

	
	/* Char Device Registration */
	/* Expose Register MIS_VFDO write interface */
	venus_i2cslave_cdev = cdev_alloc();
	venus_i2cslave_cdev->ops = &venus_i2cslave_fops;
	if(cdev_add(venus_i2cslave_cdev, MKDEV(VENUS_I2C_MAJOR, VENUS_I2C_MINOR), 1)) {
		printk(KERN_ERR "i2cslave-venus: can't add character device for i2cslave\n");
		result = -ENOMEM;
		goto fail_cdev_alloc;
	}
	/* use devfs to create device file! */
	devfs_mk_cdev(MKDEV(VENUS_I2C_MAJOR, VENUS_I2C_MINOR), S_IFCHR|S_IRUSR|S_IWUSR, VENUS_I2C_DEVICE_FILE);

	memset(RecvDataBuf,0,sizeof(RecvDataBuf));
	memset(RegPro,0,sizeof(RegPro));
	for(i=0;i<MAX_REGISTER;i++)
		RegPro[i].pAddr = pRegister + 8*i;
	
	return 0;	
	/* succeed ! */
	
fail_cdev_alloc:
	free_irq(VENUS_I2C_IRQ, i2cslave_venus_handler);
fail_req_irq:
	kfree(pRegister);
fail_alloc_memory:
	platform_device_unregister(venus_i2cslave_devs);
	driver_unregister(&venus_i2cslave_driver);
fail_device_register:
	unregister_chrdev_region(dev_venus_i2cslave, VENUS_I2C_DEVICE_NUM);
fail_alloc_dev:
	return result;
}

static void i2cslave_venus_module_exit(void)
{
	/* disable interrupt */
	writel(0, IC_INTR_MASK);
	/* remove device file by devfs */
	devfs_remove(VENUS_I2C_DEVICE_FILE);

	/* Release Character Device Driver */
	cdev_del(venus_i2cslave_cdev);

	/* Free IRQ Handler */
	free_irq(VENUS_I2C_IRQ, i2cslave_venus_handler);

	/* device driver removal */
	platform_device_unregister(venus_i2cslave_devs);
	driver_unregister(&venus_i2cslave_driver);
	
	/* Return Device Numbers */
	unregister_chrdev_region(dev_venus_i2cslave, VENUS_I2C_DEVICE_NUM);
}

MODULE_AUTHOR("yunfeng han <yunfeng_han@realsil.com.cn>");
MODULE_DESCRIPTION("i2cslave driver for Realtek Venus DVR");
MODULE_LICENSE("GPL");

module_init(i2cslave_venus_module_init);
module_exit(i2cslave_venus_module_exit);
