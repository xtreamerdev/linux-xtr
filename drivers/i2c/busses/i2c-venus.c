/* ------------------------------------------------------------------------- */
/* i2c-venus.c i2c-hw access for Realtek Venus DVR I2C                       */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2005 Chih-pin Wu <wucp@realtek.com.tw>

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
    Version 1.0 written by wucp@realtek.com.tw
    Version 2.0 modified by Frank Ting(frank.ting@realtek.com.tw)(2007/06/21)	*/
/* ------------------------------------------------------------------------- */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/wait.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
//#include <linux/i2c-algo-venus.h>
#include "../algos/i2c-algo-venus.h"
#include "i2c-venus.h"

#define VERSION "I2C 2.0 20070621 (neptune & venus)"
#define WAIT_TIME	4*HZ

#ifdef CONFIG_I2C_DEBUG_BUS
#define i2c_debug(fmt, args...)		printk(KERN_INFO fmt, ## args)
#else
#define i2c_debug(fmt, args...)
#endif

enum {
    STANDARD_MODE = 1,
    FAST_MODE = 2,
    HIGH_SPEED_MODE = 3,
};



static int i2c_speed = 1;

module_param(i2c_speed, int, S_IRUGO);

static DECLARE_WAIT_QUEUE_HEAD(venus_i2c_send_wait);
static int VENUS_I2C_TX_FIFO_DEPTH;
static int VENUS_I2C_RX_FIFO_DEPTH;
static int LOW_SPEED;



/*------------------------------------------------------------------
 * Func : i2c_venus_handler
 *
 * Desc : interrupt handler of venus i2c 
 *
 * Parm : this_irq  : i2c adapter 
 *        dev_id    :
 *        regs      :
 *         
 * Retn : IRQ_HANDLED / IRQ_NONE
 *------------------------------------------------------------------*/ 
static irqreturn_t 
i2c_venus_handler(
    int             this_irq, 
    void*           dev_id, 
    struct pt_regs* regs
    )
{
	struct i2c_adapter*         adapter   =(struct i2c_adapter *)dev_id;
	struct i2c_algo_venus_data* algo_data = adapter->algo_data;
	unsigned int regValue;

	if (readl(MIS_ISR) & 0x00000010)    // interrupt belongs to I2C
	{	
		
		i2c_debug("Venus I2C: Interrupt Handler Triggered.\n");
		
		regValue = readl(IC_INTR_STAT);

		if ((regValue & RX_FULL_BIT)||(regValue & STOP_DET_BIT)) 
		{ 
			unsigned int *cur_len = &(algo_data->cur_len);
			unsigned int len = algo_data->len;
			unsigned char *buf = algo_data->buf;
			int i,num=readl(IC_RXFLR);

			//if (!(*cur_len)) complete(&algo_data->complete);
			
			for(i = 0 ; i < num ; i++) {
				buf[*cur_len] = (unsigned char)(readl(IC_DATA_CMD) & 0xff);
				(*cur_len)++;
			}
			
			if (num && (len == *cur_len)) complete(&algo_data->complete);
			//if (num && (len == *cur_len)) wake_up_interruptible(&venus_i2c_send_wait);

			if (regValue & STOP_DET_BIT){
				readl(IC_CLR_STOP_DET);
			} 

			writel(0x10, MIS_ISR);
			
			return IRQ_HANDLED;
			
		} 
		else if(regValue & TX_ABRT_BIT) 
		{	

			dev_err(&adapter->dev, "\n\n %%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n TX_ABRT,(target = %x) \n %%%%%%%%%%%%%%%%%%%%%%%%\n\n",(unsigned long) (readl(IC_TAR)&0x3ff));     
        
			algo_data->tx_abort=1;
			complete(&algo_data->complete);
			
			readl(IC_CLR_TX_ABRT);
			writel(0x10, MIS_ISR);
			
			return IRQ_HANDLED;
			
		} 
		else 
		{

			dev_err(&adapter->dev, "unexpected interrupt(%x)\n", regValue);

			/* clear interrupt flag */
			readl(IC_CLR_INTR);
			readl(IC_CLR_RX_UNDER);
			readl(IC_CLR_TX_OVER);
			readl(IC_CLR_RD_REQ);
			readl(IC_CLR_RX_DONE);
			readl(IC_CLR_ACTIVITY);
			readl(IC_CLR_GEN_CALL);

			writel(0x10, MIS_ISR);
			
			wake_up_interruptible(&venus_i2c_send_wait);
			
			return IRQ_HANDLED;
		}
	}	
	else 
	    return(IRQ_NONE);
}



/*------------------------------------------------------------------
 * Func : i2c_venus_read
 *
 * Desc : do i2c read operation
 *
 * Parm : adapter : i2c adapter 
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/ 
static int 
i2c_venus_read(
    struct i2c_adapter*     adapter
    ) 
{	    
	unsigned int    i = 0;
	struct i2c_algo_venus_data *algo_data = adapter->algo_data;
    unsigned int*   cur_len = &(algo_data->cur_len);
	unsigned int    len     = algo_data->len;
	unsigned long   timeout;

	i2c_debug("Venus I2C: i2c_venus_read(%d): IC_RAW_INTR_STAT = %08X\n", len, readl(IC_RAW_INTR_STAT));

	*cur_len = 0;

	writel(readl(IC_INTR_MASK) | RX_FULL_BIT, IC_INTR_MASK); // enable RX_FULL interrupt

	for(i = 0 ; i < len ; i+=VENUS_I2C_TX_FIFO_DEPTH) 
	{
		int j;
		timeout = WAIT_TIME + jiffies;
		while (readl(IC_TXFLR))
		{
			if (algo_data->tx_abort)
			{
				dev_err(&adapter->dev, "tx abort(device absent)\n");
				// disable RX_FULL interrupt
				writel(readl(IC_INTR_MASK) & (~RX_FULL_BIT), IC_INTR_MASK);
				writel(0x0, IC_ENABLE);		// clear the FIFO
				return(-EIO);
			}
			
			if (time_after(jiffies, timeout)){
				dev_err(&adapter->dev, "time out\n");
				return(-ETIMEDOUT);
			}
						
            udelay(100);
		}
		
		for (j=i; j < min(len, i+VENUS_I2C_TX_FIFO_DEPTH); j++) 
		    writel(0x100, IC_DATA_CMD); 
	}
		
	wait_for_completion(&algo_data->complete);
	
	// disable RX_FULL interrupt
	writel(readl(IC_INTR_MASK) & (~RX_FULL_BIT), IC_INTR_MASK);
	
	if (algo_data->tx_abort)
	{
		dev_err(&adapter->dev, "tx abort([%x, %x], target=%x)\n", *cur_len, len, readl(IC_TAR)&0x3ff);
		writel(0x0, IC_ENABLE);			// clear the FIFO
		return(-EIO);
		
	}
	else
	{
		if (*cur_len != len){
			dev_err(&adapter->dev, "fail to receive(length=[%x, %x], target=%x)\n", *cur_len, len, readl(IC_TAR)&0x3ff);
			writel(0x0, IC_ENABLE);
			return(-ETIMEDOUT);
		}
		//dev_err(&adapter->dev, "i2c OK=%x\n", ret);
		return(0);
	}
		
}



/*------------------------------------------------------------------
 * Func : i2c_venus_write
 *
 * Desc : do i2c write operation
 *
 * Parm : adapter : i2c adapter 
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/ 
static int 
i2c_venus_write(
    struct i2c_adapter *adapter
    )
{
	int i = 0;
	struct i2c_algo_venus_data *algo_data = adapter->algo_data;
	unsigned int    len     = algo_data->len;
	unsigned char*  buf     = algo_data->buf;
	unsigned long   timeout = WAIT_TIME;
	
	i2c_debug("Venus I2C: i2c_venus_write(%d): IC_RAW_INTR_STAT = %08X\n", len, readl(IC_RAW_INTR_STAT));

	timeout += jiffies;
	while(i < len) 
	{
		while(!(readl(IC_STATUS) & 0x2))
		{   // wait while queue is full
			if (time_after(jiffies, timeout))
			{
				dev_err(&adapter->dev, "time out(FIFO full)\n");
				return(-ETIMEDOUT);
			}
			udelay(100); // 10 ^ -4 seconds (enough for 1 byte to transmit)
		}

		writel(((unsigned int)(buf[i]&0xff)), IC_DATA_CMD);
		i++;
	}


	i2c_debug("Venus I2C: i2c_venus_write(): IC_RAW_INTR_STAT = %08X Completed \n", readl(IC_RAW_INTR_STAT));

	return 0;
}



/*------------------------------------------------------------------
 * Func : i2c_venus_xfer
 *
 * Desc : start i2c xfer (read/write)
 *
 * Parm : adapter : i2c adapter
 *        msgs    : i2c messages
 *        num     : nessage counter
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
static int 
i2c_venus_xfer(
    struct i2c_adapter*     adapter, 
    struct i2c_msg*         msgs, 
    int                     num
    )
{
	int err = 0;
	int i;
	struct i2c_algo_venus_data *algo_data = adapter->algo_data;
	unsigned long timeout = WAIT_TIME;
	unsigned short target_addr;	
	unsigned char  tx_count = 0;	
	unsigned char  rx_count = 0;	
		

	//down(&lock);

	// always assume doing i2c transactions to same device
	for (i=0; i < num; i++)
	{
		struct i2c_msg *p;

		p = & msgs[i];
		if (!p->len){
			dev_err(&adapter->dev, "no such protocol\n");
			return(-EINVAL);
		}
		
        if (p->flags & I2C_M_RD)	
            rx_count++;
        else
		    tx_count++;
	}
	
#ifndef	I2C_VENUS_WAIT_COMPLETE_AT_END

	timeout+=jiffies;
	
	while(!(readl(IC_STATUS) & 0x04) && (readl(IC_STATUS) & 0x01))
	{ 
	    // wait for i2c adapter idel
		if (time_after(jiffies, timeout))
		{
			dev_err(&adapter->dev, "time out(FIFO isn't empty\n");
			writel(0, IC_ENABLE);
			udelay(100); 
			break;
			//return(-ETIMEDOUT);
		}
		/* for debug 
		if ((readl(IC_STATUS) & 0x04)){
		    printk("TX FIFO Complete Empty but I2C Still Busy\n");
        }
        */
		udelay(1000); 
	}		
#endif
	
	/* step.1 disable DW_apb_i2c */
	writel(0, IC_ENABLE);

	if(msgs[0].flags & I2C_M_TEN) 
	{   
	    // 10-bit slave address
		/* step.2 write IC_SAR register */
		writel(VENUS_MASTER_7BIT_ADDR & 0x7f, IC_SAR);

		/* step.3 write IC_CON register, 7-bit master */
		writel((0x69 | (i2c_speed << 1)), IC_CON);

		/* step.4 write IC_TAR register */
        target_addr = (0x3ff & msgs[0].addr);
		writel(target_addr, IC_TAR);
	}
	else 
	{
		/* step.2 wirte IC_SAR register */
		writel(VENUS_MASTER_7BIT_ADDR & 0x7f, IC_SAR);

		/* step.3 write IC_CON register, 7-bit master */
		writel((0x61 | (i2c_speed << 1)), IC_CON);

		/* step.4 write IC_TAR register */
		target_addr = (0x7f & msgs[0].addr);
		writel(target_addr, IC_TAR);
		
		if (algo_data->p_target_info[target_addr]==NULL) 
		{
		    venus_set_target_name(adapter, target_addr, "Unknown", sizeof("Unknown"));
        }		    
	}

	// step.5 - speed decision
	if (msgs[0].flags & I2C_LOW_SPEED) 
	{   
		writel(0x16e, IC_SS_SCL_HCNT);  // 33KHz
		writel(0x1ad, IC_SS_SCL_LCNT);
		LOW_SPEED = 1;
	}
	else 
	{   	    
		writel(0x7a, IC_SS_SCL_HCNT);  // 100KHz
		writel(0x8f, IC_SS_SCL_LCNT);
		LOW_SPEED = 0;
	}

	if(i2c_speed == HIGH_SPEED_MODE) {
		/* TODO: write IC_HS_MADDR */
	}

	i2c_debug("Venus I2C: IC_CON = %08X IC_TAR = %08X\n", readl(IC_CON), readl(IC_TAR));

	/* step.6 enable DW_apb_i2c */
	writel(0x1, IC_ENABLE);    
    
	for (i = 0; !err && i < num; i++) 
	{
		struct i2c_msg *p;
		
		p = &msgs[i];
		
		algo_data->len = p->len;
		algo_data->buf = p->buf;
		algo_data->tx_abort = 0;
		init_completion(&algo_data->complete);

		if (p->flags & I2C_M_RD)		    
			err = i2c_venus_read(adapter);        
		else
			err = i2c_venus_write(adapter);
    }

#ifdef	I2C_VENUS_WAIT_COMPLETE_AT_END

	timeout+=jiffies;
	
	while(!(readl(IC_STATUS) & 0x04) && (readl(IC_STATUS) & 0x01))
	{ 
	    // wait for i2c adapter idel
		if (time_after(jiffies, timeout))
		{
			dev_err(&adapter->dev, "time out(FIFO isn't empty\n");
			writel(0, IC_ENABLE);
			err = -ETIMEDOUT;	
			break;			
		}		
		udelay(1000); 
	}	
	
	// wait the last byte xfer complete 
	if (msgs[0].flags & I2C_LOW_SPEED)
	    udelay(4050);               // 9 bit * 0.3ms * 1.5 = 4.05 ms
	else
	    udelay(1350);               // 9 bit * 0.1ms * 1.5 = 1.35 ms
	
	if (algo_data->tx_abort) 
	{        
        dev_err(&adapter->dev, "tx abort, target=%x(%s)\n", readl(IC_TAR) & 0x3ff,
                (target_addr <= 0x7F) ? algo_data->p_target_info[target_addr]->name : "Unknown");        
        
	    writel(0x0, IC_ENABLE);			// clear the FIFO
	    err = -EIO;		
    }	
#endif

    if (target_addr <= 0x7F) 
    {
        if (!err) {
            algo_data->p_target_info[target_addr]->tx_count += tx_count;  
            algo_data->p_target_info[target_addr]->rx_count += rx_count;  
        }
        else 
        {
            algo_data->p_target_info[target_addr]->tx_err += tx_count;                 
            algo_data->p_target_info[target_addr]->rx_err += rx_count;                 
        }
    }

    return (err) ? err : num;

}



/* ------------------------------------------------------------------------
 *  * Encapsulate the above functions in the correct operations structure.
 *   * This is only done when more than one hardware adapter is supported.
 *    */
static struct i2c_algo_venus_data i2c_venus_data = 
{
	.masterXfer	= i2c_venus_xfer,
};


static struct i2c_adapter venus_i2c_adapter = 
{
	.owner		= THIS_MODULE,
	.class		= I2C_CLASS_HWMON,
	.id		    = 0x00,
	.name		= "Venus I2C bus",
	.algo_data	= &i2c_venus_data,
};


/*------------------------------------------------------------------
 * Func : i2c_venus_module_init
 *
 * Desc : init venus i2c module
 *
 * Parm : N/A
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
static int 
__init i2c_venus_module_init(void) 
{
	if (i2c_speed > 3 || i2c_speed < 1)
		i2c_speed = 1;

	printk(KERN_INFO "%s\n", VERSION);

#ifdef  I2C_VENUS_WAIT_COMPLETE_AT_END	
	printk("i2c-venus: WAIT AT END : ON\n");
#else	
	printk("i2c-venus: WAIT AT END : OFF\n");
#endif	

	/* examing GPIO#5/GPIO#4 belongs to I2C */
	if((readl(MIS_PSELL) & 0x00000f00) != 0x00000500) 
	{
		printk(KERN_ERR "i2c-venus: pins are not defined as I2C\n");
		return -ENODEV;
	}

	if (request_irq(VENUS_I2C_IRQ, i2c_venus_handler, SA_INTERRUPT|SA_SHIRQ, "Venus I2C", &venus_i2c_adapter) < 0) 
	{
		printk(KERN_ERR "i2c-venus: Request irq%d failed\n", VENUS_I2C_IRQ);
		goto fail_req_irq;
	}

    /* register a physical venus i2c bus */    
    memset(&i2c_venus_data, 0, sizeof(i2c_venus_data));    
    i2c_venus_data.masterXfer = i2c_venus_xfer;
    
	if (i2c_venus_add_bus(&venus_i2c_adapter) < 0)
		goto fail;

	/* hardware initialization */
	VENUS_I2C_TX_FIFO_DEPTH = ((readl(IC_COMP_PARAM_1) & 0xff0000) >> 16) + 1;
	VENUS_I2C_RX_FIFO_DEPTH = ((readl(IC_COMP_PARAM_1) & 0xff00) >> 8) + 1;

	printk(KERN_INFO "i2c-venus: TX FIFO Depth = %d, RX FIFO Depth = %d\n", VENUS_I2C_TX_FIFO_DEPTH, VENUS_I2C_RX_FIFO_DEPTH);
	printk(KERN_INFO "i2c-venus: modify i2c patch + 1ms delay\n");
	/* enable I2C interrupt for TX_ABRT only */
	writel(TX_ABRT_BIT|STOP_DET_BIT, IC_INTR_MASK);

	/* set Threshhold Level */
	//writel(VENUS_I2C_RX_FIFO_DEPTH, IC_RX_TL);
						
	return 0;

fail:
	free_irq(VENUS_I2C_IRQ, &venus_i2c_adapter);

fail_req_irq:
	return -ENODEV;
}



/*------------------------------------------------------------------
 * Func : i2c_venus_module_exit
 *
 * Desc : exit venus i2c module
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/  
static void 
i2c_venus_module_exit(void)
{
	/* disable interrupt */
	writel(0, IC_INTR_MASK);

	free_irq(VENUS_I2C_IRQ, &venus_i2c_adapter);

	i2c_venus_del_bus(&venus_i2c_adapter);      // remove venus i2c bus
}



MODULE_AUTHOR("Chih-pin Wu <wucp@realtek.com.tw>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for Realtek Venus DVR");
MODULE_LICENSE("GPL");

module_init(i2c_venus_module_init);
module_exit(i2c_venus_module_exit);

