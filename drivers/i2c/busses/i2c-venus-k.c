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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.		         */
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
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
//#include <linux/i2c-algo-venus.h>
#include "../algos/i2c-algo-venus.h"
#include "i2c-venus.h"

////////////// Internal Used Macros ///////////////
enum {
    STANDARD_MODE            = 1,
    FAST_MODE                = 2,
    HIGH_SPEED_MODE          = 3,
};

#define VENUS_I2C_TIMEOUT    (1*HZ)

#define SW_BUFFER_DEPTH      256

#ifdef  CONFIG_I2C_DEBUG_BUS
    #define VENUS_I2C_DBG(args...)          printk(KERN_WARNING args)
    #define VENUS_I2C_EVENT_DBG(args...)    printk(KERN_WARNING args)
#else
    #define VENUS_I2C_DBG(args...)          
    #define VENUS_I2C_EVENT_DBG(args...)     
#endif

#ifdef VENUS_I2C_FLOW_DBG_EN
    #define VENUS_I2C_FLOW_DBG()                    VENUS_I2C_DBG("Venus I2C: %s\n",__FUNCTION__)       
#else
    #define VENUS_I2C_FLOW_DBG()                    
#endif 

#define VENUS_I2C_WARNING(args...)          printk(KERN_WARNING args)
#define VENUS_I2C_ERR(args...)              printk(KERN_ERR args)

////////////// Internal Used data ///////////////
static int stop_detect_count = 0;
static int rx_full_count     = 0;
static int i2c_speed         = 1;

module_param(i2c_speed, int, S_IRUGO);

static DECLARE_WAIT_QUEUE_HEAD(venus_i2c_send_wait);
static DECLARE_MUTEX(lock);
static int VENUS_I2C_TX_FIFO_DEPTH;
static int VENUS_I2C_RX_FIFO_DEPTH;
static int TX_STATUS;
static int LOW_SPEED;

static int RX_SW_FIFO[SW_BUFFER_DEPTH];
static unsigned char RX_SW_FIFO_DEPTH;


/* ----- local functions ----------------------------------------------	*/
static void i2c_venus_clear_all(void) {
	readl(IC_CLR_INTR);
	readl(IC_CLR_RX_UNDER);
	readl(IC_CLR_RX_OVER);
	readl(IC_CLR_TX_OVER);
	readl(IC_CLR_RD_REQ);
	readl(IC_CLR_TX_ABRT);
	readl(IC_CLR_RX_DONE);
	readl(IC_CLR_ACTIVITY);
	readl(IC_CLR_STOP_DET);
	readl(IC_CLR_START_DET);
	readl(IC_CLR_GEN_CALL);
}



#ifdef I2C_VENUS_RANDOM_READ_EN

static int i2c_venus_random_read(
    unsigned char*      p_w_buf, 
    unsigned int        w_len, 
    unsigned char*      p_r_buf, 
    unsigned int        r_len
    ) 
{
    int i = 0;
    int bufferLength = 0;
	int p_rx_full_count = 0;
	int p_stop_detect_count = 0;	
    
    RX_SW_FIFO_DEPTH = 0;
    
    VENUS_I2C_FLOW_DBG();                
    
    writel(readl(IC_INTR_MASK) | 0x4, IC_INTR_MASK); // enable RX_FULL interrupt

    rx_full_count = 0;
    stop_detect_count = 0;
    
    // send write command
	for(i = 0 ; i < w_len ; i++)
    {
		while(!(readl(IC_STATUS) & 0x2))    // wait while queue is full
			udelay(20); // 10 ^ -4 seconds (enough for 1 byte to transmit)
		
	    writel(((unsigned int)(*(p_w_buf+i))&0xff), IC_DATA_CMD);	    
	}		
	
	// send read command
	for(i = 0 ; i < r_len ; i++)
    {
		while(!(readl(IC_STATUS) & 0x2))    // wait while queue is full
			udelay(20); // 10 ^ -4 seconds (enough for 1 byte to transmit)
		
        writel(0x100, IC_DATA_CMD);
	}
	
	p_rx_full_count = rx_full_count;
    p_stop_detect_count = stop_detect_count;
    
    while(readl(IC_TXFLR))      // wait while queue is full
	    udelay(100);            // 10 ^ -4 seconds (enough for 1 byte to transmit)
		        
    if (wait_event_interruptible_timeout(venus_i2c_send_wait,  TX_STATUS != 0, VENUS_I2C_TIMEOUT) < 0) {
		writel(readl(IC_INTR_MASK) & (~0x4), IC_INTR_MASK);
		return -ERESTARTSYS;
	}        
	
    // disable RX_FULL interrupt
	writel(readl(IC_INTR_MASK) & (~0x4), IC_INTR_MASK);

	if (TX_STATUS != 1) {
        VENUS_I2C_WARNING("Venus I2C: i2c_venus_read() failed: TX_ABRT\n");
		return -EIO;
    }
		
    memcpy(p_r_buf, RX_SW_FIFO, RX_SW_FIFO_DEPTH);
    bufferLength = RX_SW_FIFO_DEPTH;	

	RX_SW_FIFO_DEPTH = 0;
    
	while (readl(IC_RXFLR)>0) {
		p_r_buf[bufferLength] = (unsigned char)(readl(IC_DATA_CMD) & 0xff);
		bufferLength++;
	}
	
	if (bufferLength != r_len) {			
		VENUS_I2C_WARNING("Venus I2C: i2c_venus_random_read() failed: %d/%d bytes data received.\n",bufferLength, r_len);                
		return -EIO;
	}
	
	if (stop_detect_count != 1) {				
        VENUS_I2C_DBG("Venus I2C: p_rx_full_count=%d,p_stop_detect_count = %d\n",p_rx_full_count,p_stop_detect_count);     
        VENUS_I2C_DBG("Venus I2C: rx_full_count=%d,stop_detect_count = %d\n",rx_full_count,stop_detect_count);                  
	    
		VENUS_I2C_WARNING("Venus I2C: i2c_venus_random_read() : multiple stop detected, please retry later.\n");                
		return -EIO;
	}

    return 0;
}
#endif



static int i2c_venus_read(unsigned char *buf, unsigned int len) 
{
	int retval = 0;
	int i = 0;
	
    VENUS_I2C_FLOW_DBG();
    
	/* step.7 filling IC_DATA_CMD to generate a master-read command */
	if(len <= VENUS_I2C_RX_FIFO_DEPTH) 
	{
		for(i = 0 ; i < len ; i++)
		{
            while(!(readl(IC_STATUS) & 0x2))    // wait while queue is full
			    udelay(20); // 10 ^ -4 seconds (enough for 1 byte to transmit)
			    
			writel(0x100, IC_DATA_CMD);
        }
			
		//if (wait_event_interruptible(venus_i2c_send_wait, TX_STATUS != 0) != 0)
        if (wait_event_interruptible_timeout(venus_i2c_send_wait, TX_STATUS != 0, VENUS_I2C_TIMEOUT ) < 0)		
		{
		    VENUS_I2C_WARNING("Venus I2C: -ERESTARTSYS");
			return -ERESTARTSYS;
        }                  
        
		if(TX_STATUS == 1) 
		{   		    		    
			i = 0;
			
			while(1) 
			{
				*(buf+i) = (unsigned char)(readl(IC_DATA_CMD) & 0xff);
				i++;

				if(i == len)
					break;

				if(readl(IC_RXFLR) == 0) // if STOP_DET && no data to receive
					break;
			}	        

			if(i != len){
                VENUS_I2C_WARNING("Venus I2C: data length mismatch(%d/%d)\n",i,len);	                
				retval = -EIO;
            }
			else
				retval = 0;
			
			///--- DBG Message ---//					
			int foo;
			for(foo = 0 ; foo < len ; foo++){
				VENUS_I2C_DBG("Venus I2C: data(%d) = %08X\n", foo, *(buf+foo));
            }

			return retval;
		}
		else 
		{   		    
            VENUS_I2C_WARNING("Venus I2C: TX_ABORT_OCCUR ><");         				
			return -EINVAL;
		}        
	}
	else 
	{   
	    // access longer than FIFO depth
		int bufferLength = 0;
		RX_SW_FIFO_DEPTH = 0;
		int p_rx_full_count = 0;
		int p_stop_detect_count = 0;

		writel(readl(IC_INTR_MASK) | 0x4, IC_INTR_MASK); // enable RX_FULL interrupt

        rx_full_count = 0;
        stop_detect_count = 0;
        
		for(i = 0 ; i < len ; i++) 
		{		    					    
            while(!(readl(IC_STATUS) & 0x2))    // wait while queue is full
	            udelay(20); // 10 ^ -4 seconds (enough for 1 byte to transmit)
			    
			writel(0x100, IC_DATA_CMD); // send read request as many as possible (RX_FULL irq will cover)			
	    }		

		p_rx_full_count = rx_full_count;
		p_stop_detect_count = stop_detect_count;		  
		        
        if (wait_event_interruptible_timeout(venus_i2c_send_wait, TX_STATUS != 0, VENUS_I2C_TIMEOUT) < 0) {
			writel(readl(IC_INTR_MASK) & (~0x4), IC_INTR_MASK);
			return -ERESTARTSYS;
		}        
        
		// disable RX_FULL interrupt
		writel(readl(IC_INTR_MASK) & (~0x4), IC_INTR_MASK);

		if(TX_STATUS == 1) 
		{   		    
			for(i = 0 ; i < RX_SW_FIFO_DEPTH ; i++) {
				*(buf+bufferLength) = RX_SW_FIFO[i];
				bufferLength++;
			}

			RX_SW_FIFO_DEPTH = 0;			
            
			while (readl(IC_RXFLR)>0) {
				*(buf+bufferLength) = (unsigned char)(readl(IC_DATA_CMD) & 0xff);
				bufferLength++;
			}
			
			if (bufferLength != len) {				
				VENUS_I2C_WARNING("Venus I2C: i2c_venus_read() failed: %d/%d bytes data received.\n", len, bufferLength);                
				return -EIO;
			}
			
			if (stop_detect_count != 1) {				
                VENUS_I2C_DBG("Venus I2C: p_rx_full_count=%d,p_stop_detect_count = %d\n",p_rx_full_count,p_stop_detect_count);     
                VENUS_I2C_DBG("Venus I2C: rx_full_count=%d,stop_detect_count = %d\n",rx_full_count,stop_detect_count);                
			    
				VENUS_I2C_WARNING("Venus I2C: i2c_venus_read() : multiple stop detected, please retry later.\n");                
				return -EIO;
			}

            return 0;			
		}
		else { // TX_ABRT 
			VENUS_I2C_WARNING("Venus I2C: i2c_venus_read() failed: TX_ABRT><\n");
			return -EIO;
		}
	}
}



static int i2c_venus_write(unsigned char *buf, unsigned int len) 
{
	int i = 0;
		
    VENUS_I2C_FLOW_DBG();		
	
#if 0	
	for(int foo = 0; foo < len ; foo++){
		VENUS_I2C_DBG(KERN_WARNING "Venus I2C: write data(%d) = %08X\n", foo, *(buf+foo));
    }
#endif

	/* step.7 filling IC_DATA_CMD (greedy) */
	while(i < len) 
	{
		while(!(readl(IC_STATUS) & 0x2))    // wait while queue is full
			udelay(20); // 10 ^ -4 seconds (enough for 1 byte to transmit)
		
	    writel(((unsigned int)(*(buf+i))&0xff), IC_DATA_CMD);
	    i++;		
	}	
	
    if (wait_event_interruptible_timeout(venus_i2c_send_wait, TX_STATUS != 0,VENUS_I2C_TIMEOUT) < 0){        
        VENUS_I2C_WARNING("Venus I2C: -ERESTARTSYS");
        return -ERESTARTSYS;
    }
    
	if (TX_STATUS != 1) {   
		VENUS_I2C_WARNING("Venus I2C: i2c_venus_write() failed: TX_ABRT><\n");
		return -EIO;
	}
    
	return 0;
}



static irqreturn_t i2c_venus_handler(int this_irq, void *dev_id, struct pt_regs *regs) 
{
	unsigned int regValue;

    if (!(readl(MIS_ISR) & 0x00000010)) 
        return IRQ_NONE;
        
    regValue = readl(IC_INTR_STAT);
		
	VENUS_I2C_EVENT_DBG("Venus I2C: Interrupt Handler Triggered. IC_INTR_STAT = %08x\n",regValue);	

	if (regValue & 0x4 )
	{   	    
		VENUS_I2C_EVENT_DBG("Venus I2C: INT(RX_FULL).\n");
				
		while (readl(IC_RXFLR))
		{
			RX_SW_FIFO[RX_SW_FIFO_DEPTH] = (unsigned char)(readl(IC_DATA_CMD) & 0xff);
			RX_SW_FIFO_DEPTH++;
		}
				
		rx_full_count++;
		goto end_interrupt;		
	}
	else if (regValue & 0x200)          // STOP_DET
	{   		    
		TX_STATUS = 1;			
        VENUS_I2C_EVENT_DBG("Venus I2C: INT(STOP_DET).\n");               	        
        stop_detect_count++;	
        wake_up_interruptible(&venus_i2c_send_wait);        	    		
	}
	else if(regValue & 0x40)            // TX_ABRT
	{	
		TX_STATUS = 2;
        VENUS_I2C_EVENT_DBG("Venus I2C: INT(TX_ABRT).\n");
		wake_up_interruptible(&venus_i2c_send_wait);
	}

	/* clear interrupt flag */
	i2c_venus_clear_all();

	/* always disable TX_EMPTY */
	writel(readl(IC_INTR_MASK) & (~0x10), IC_INTR_MASK);

end_interrupt:
	writel(0x10, MIS_ISR);
	
	return IRQ_HANDLED;
}



static int i2c_venus_xfer(struct i2c_msg *msgs, int num) 
{
	int err = 0;
	int retval = 0;
	int i = 0;

	down(&lock);

	// always assume doing i2c transactions to same device

		/* step.1 disable DW_apb_i2c */
	writel(0, IC_ENABLE);

	if(msgs[0].flags & I2C_M_TEN) 
	{   // 10-bit slave address
		/* step.2 write IC_SAR register */
		writel(VENUS_MASTER_7BIT_ADDR & 0x7f, IC_SAR);

		/* step.3 write IC_CON register, 7-bit master */
		writel((0x69 | (i2c_speed << 1)), IC_CON);

		/* step.4 write IC_TAR register */
		writel((0x3ff & msgs[0].addr), IC_TAR);
	}
	else 
	{
		/* step.2 wirte IC_SAR register */
		writel(VENUS_MASTER_7BIT_ADDR & 0x7f, IC_SAR);

		/* step.3 write IC_CON register, 7-bit master */
		writel((0x61 | (i2c_speed << 1)), IC_CON);

		/* step.4 write IC_TAR register */
		writel((0x7f & msgs[0].addr), IC_TAR);
	}

	// step.5 - speed decision
	if(msgs[0].flags & I2C_LOW_SPEED)       // 33KHz
	{                                       
		writel(0x16e, IC_SS_SCL_HCNT);
		writel(0x1ad, IC_SS_SCL_LCNT);
		LOW_SPEED = 1;
	}
	else                                    // 100KHz
	{      
		writel(0x7a, IC_SS_SCL_HCNT);
		writel(0x8f, IC_SS_SCL_LCNT);
		LOW_SPEED = 0;
	}

	if(i2c_speed == HIGH_SPEED_MODE) {
		/* TODO: write IC_HS_MADDR */
	}

	VENUS_I2C_DBG("Venus I2C: IC_CON = %08X\n", readl(IC_CON));
	VENUS_I2C_DBG("Venus I2C: IC_TAR = %08X\n", readl(IC_TAR));
	
	writel(readl(IC_INTR_MASK) | 0x200, IC_INTR_MASK);          // enable STOP_DET interrupt

	/* step.6 enable DW_apb_i2c */
	writel(0x1, IC_ENABLE);

	for (i = 0; !err && i < num; i++) 
	{
		struct i2c_msg *p;
		p = &msgs[i];			    
	    
		if (!p->len)
			continue;        
      
        TX_STATUS = 0;                              
        
		if (!(p->flags & I2C_M_RD))
		{			    
#ifdef I2C_VENUS_RANDOM_READ_EN
		    if ((i < (num -1)))
		    {
		        if ((msgs[i+1].flags & I2C_M_RD)){              // random address read : W + R
		            err = i2c_venus_random_read(p->buf, p->len, msgs[i+1].buf, msgs[i+1].len );                          
                    i++;      
                }
                else
                    err = i2c_venus_write(p->buf, p->len);      // write        
            }
            else
#endif            
                err = i2c_venus_write(p->buf, p->len);          // write        		    		    		    			    
        }
		else		
			err = i2c_venus_read(p->buf, p->len);               // current address read		
		
		if (err) {
		    VENUS_I2C_WARNING("Venus I2C: Fail transaction\n");			
			retval = -EINVAL;
			break;
		}
	}		

	up(&lock);
	return retval;
}




/* ------------------------------------------------------------------------
 * Encapsulate the above functions in the correct operations structure.
 * This is only done when more than one hardware adapter is supported.
 */
static struct i2c_algo_venus_data i2c_venus_data = {
	.masterXfer		= i2c_venus_xfer,
};

static struct i2c_adapter venus_i2c_adapter = 
{
	.owner		= THIS_MODULE,
	.class		= I2C_CLASS_HWMON,
	.id			= 0x00,
	.name		= "Venus I2C bus",
	.algo_data	= &i2c_venus_data,
};




static int __init i2c_venus_module_init(void) 
{
	if(i2c_speed > 3 || i2c_speed < 1)
		i2c_speed = 1;

	VENUS_I2C_DBG("i2c-venus: Realtek Venus DVR I2C Driver\n");

	/* examing GPIO#5/GPIO#4 belongs to I2C */
	if((readl(MIS_PSELL) & 0x00000f00) != 0x00000500) 
	{
		VENUS_I2C_ERR("i2c-venus: pins are not defined as I2C\n");
		return -ENODEV;
	}

	if (request_irq(VENUS_I2C_IRQ, i2c_venus_handler, SA_INTERRUPT|SA_SHIRQ, "Venus I2C", i2c_venus_handler) < 0) 
	{
		VENUS_I2C_ERR("i2c-venus: Request irq%d failed\n", VENUS_I2C_IRQ);
		goto fail_req_irq;
	}

	if (i2c_venus_add_bus(&venus_i2c_adapter) < 0)
		goto fail;

	/* hardware initialization */
	VENUS_I2C_TX_FIFO_DEPTH = ((readl(IC_COMP_PARAM_1) & 0xff0000) >> 16) + 1;
	VENUS_I2C_RX_FIFO_DEPTH = ((readl(IC_COMP_PARAM_1) & 0xff00) >> 8) + 1;

	VENUS_I2C_DBG("i2c-venus: TX FIFO Depth = %d, RX FIFO Depth = %d\n", 
					VENUS_I2C_TX_FIFO_DEPTH,
					VENUS_I2C_RX_FIFO_DEPTH);

	/* enable I2C interrupt for TX_ABRT only */
	writel(0x40, IC_INTR_MASK);

	/* set Threshhold Level */
	writel(0x04, IC_RX_TL);
	
	return 0;

fail:
	free_irq(VENUS_I2C_IRQ, i2c_venus_handler);

fail_req_irq:
	return -ENODEV;
}




static void i2c_venus_module_exit(void)
{
	/* disable interrupt */
	writel(0, IC_INTR_MASK);

	free_irq(VENUS_I2C_IRQ, i2c_venus_handler);

	i2c_venus_del_bus(&venus_i2c_adapter);
}




MODULE_AUTHOR("Chih-pin Wu <wucp@realtek.com.tw>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for Realtek Venus DVR");
MODULE_LICENSE("GPL");

module_init(i2c_venus_module_init);
module_exit(i2c_venus_module_exit);
