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
    Version 2.0 modified by Frank Ting(frank.ting@realtek.com.tw)(2007/06/21)	
-------------------------------------------------------------------------     
    1.4     |   20081016    | Multiple I2C Support
-------------------------------------------------------------------------     
    1.5     |   20090423    | Add Suspen/Resume Feature    
-------------------------------------------------------------------------*/    
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
#include <linux/platform.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include "../algos/i2c-algo-venus-k.h"
#include "i2c-venus-priv.h"


static int i2c_speed = SPD_MODE_SS;
module_param(i2c_speed, int, S_IRUGO);

#define VENUS_I2C_IRQ	        3
#define VENUS_MASTER_7BIT_ADDR	0x24

static struct i2c_adapter venus_i2c_adapter[MAX_I2C_CNT]; 
static i2c_venus_algo i2c_venus_data[MAX_I2C_CNT];

////////////////////////////////////////////////////////////////////

#ifdef CONFIG_I2C_DEBUG_BUS
#define i2c_debug(fmt, args...)		printk(KERN_INFO fmt, ## args)
#else
#define i2c_debug(fmt, args...)
#endif

/*------------------------------------------------------------------
 * Func : i2c_venus_xfer
 *
 * Desc : start i2c xfer (read/write)
 *
 * Parm : p_msg : i2c messages 
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
void i2c_venus_dump_msg(const struct i2c_msg* p_msg)
{
    printk("msg->addr  = %02x\n",p_msg->addr);
    printk("msg->flags = %04x\n",p_msg->flags);
    printk("msg->len   = %d  \n",p_msg->len);
    printk("msg->buf   = %p  \n",p_msg->buf);    
}


#define IsReadMsg(x)        (x.flags & I2C_M_RD)
#define IsSameTarget(x,y)   ((x.addr == y.addr) && !((x.flags ^ y.flags) & (~I2C_M_RD)))

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
    void*                   dev_id, 
    struct i2c_msg*         msgs, 
    int                     num
    )
{
    venus_i2c* p_this = (venus_i2c*) dev_id;
	int ret = 0;
	int i;
	    
	for (i = 0; i < num; i++) 
	{			         
        ret = p_this->set_tar(p_this, msgs[i].addr, ADDR_MODE_7BITS);
        
        if (ret<0)
            goto err_occur;
        
        if (IsReadMsg(msgs[i]))            
        {       
            // Single Read     
            ret = p_this->read(p_this, NULL, 0, msgs[i].buf, msgs[i].len);
        }                            
        else
        {             
            if ((i < (num-1)) && IsReadMsg(msgs[i+1]) && IsSameTarget(msgs[i], msgs[i+1]))
            {
                // Random Read = Write + Read (same addr)                
                ret = p_this->read(p_this, msgs[i].buf, msgs[i].len, msgs[i+1].buf, msgs[i+1].len);                        
                i++;
            }
            else
            {   
                // Single Write
                ret = p_this->write(p_this, msgs[i].buf, msgs[i].len, (i==(num-1)) ? WAIT_STOP : NON_STOP);
            }            
        }
        
        if (ret < 0)        
            goto err_occur;                          
    }     

    return i;
    
////////////////////    
err_occur:        
    
    if (ret==-ETXABORT && (msgs[i].flags & I2C_M_NO_ABORT_MSG))    
        return -EACCES; 
        
    printk("-----------------------------------------\n");        
          
    switch(ret)
    {
    case -ECMDSPLIT:
        printk("[I2C%d] Xfer fail - MSG SPLIT (%d/%d)\n", p_this->id, i,num);
        break;                                            
                                        
    case -ETXABORT:             
            
        printk("[I2C%d] Xfer fail - TXABORT (%d/%d), Reason=%04x\n",p_this->id, i,num, p_this->get_tx_abort_reason(p_this));        
        break;                  
                                
    case -ETIMEOUT:              
        printk("[I2C%d] Xfer fail - TIMEOUT (%d/%d)\n", p_this->id, i,num);        
        break;                  
                                
    case -EILLEGALMSG:           
        printk("[I2C%d] Xfer fail - ILLEGAL MSG (%d/%d)\n",p_this->id, i,num);
        break;
    
    case -EADDROVERRANGE:    
        printk("[I2C%d] Xfer fail - ADDRESS OUT OF RANGE (%d/%d)\n",p_this->id, i,num);
        break;
        
    default:        
        printk("[I2C%d] Xfer fail - Unkonwn Return Value (%d/%d)\n", p_this->id, i,num);
        break;
    }    
    
    i2c_venus_dump_msg(&msgs[i]);
    
    printk("-----------------------------------------\n");        
        
    ret = -EACCES;        
    return ret;
}



/*------------------------------------------------------------------
 * Func : i2c_venus_set_speed
 *
 * Desc : set speed of venus i2c
 *
 * Parm : dev_id : i2c adapter
 *        KHz    : speed of i2c adapter 
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
static int 
i2c_venus_set_speed(
    void*                   dev_id, 
    int                     KHz
    )
{
    venus_i2c* p_this = (venus_i2c*) dev_id;        	
    
    if (p_this)
	    return p_this->set_spd(p_this, KHz);
	        
    return -1;	        
}

    

/*------------------------------------------------------------------
 * Func : i2c_venus_register_adapter
 *
 * Desc : register i2c_adapeter 
 *
 * Parm : p_adap      : i2c_adapter data structure
 *        p_algo_data : data of venus i2c algorithm
 *        p_phy       : pointer of i2c phy
 *         
 * Retn : 0 : success, others failed
 *------------------------------------------------------------------*/  
static int 
__init i2c_venus_register_adapter(
    struct i2c_adapter*     p_adap,
    i2c_venus_algo*         p_algo_data,    
    venus_i2c*              p_phy
    ) 
{        
	p_adap->owner           = THIS_MODULE;
	p_adap->class           = I2C_CLASS_HWMON;
	p_adap->id		        = 0x00;	
	sprintf(p_adap->name, "%s I2C %d bus", MODLE_NAME, p_phy->id);	
	p_adap->algo_data       = p_algo_data;
		
	p_algo_data->dev_id     = (void*) p_phy;
	p_algo_data->masterXfer = i2c_venus_xfer;
	p_algo_data->set_speed  = i2c_venus_set_speed;
		
	return i2c_venus_add_bus(p_adap);	
}



//////////////////////////////////////////////////////////////////////////////////////////////
// Platform Device Interface
//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_PM

#define VENUS_I2C_BASE_ID	    0x12610000 
#define VENUS_I2C_MAJOR_ID(x)	(x & 0xFFFF0000)
#define VENUS_I2C_MINOR_ID(x)	(x & 0x0000FFFF)
#define VENUS_I2C_NAME          "Venus_I2C"

static struct platform_device* p_venus_i2c[MAX_I2C_CNT] = {NULL, NULL};


static int venus_i2c_probe(struct device *dev)
{
    struct platform_device	*pdev = to_platform_device(dev);
    return strncmp(pdev->name,VENUS_I2C_NAME, strlen(VENUS_I2C_NAME));
}


static int venus_i2c_remove(struct device *dev)
{
    // we don't need to do anything for it...
    return 0;
}
 
static void venus_i2c_shutdown(struct device *dev)
{
    // we don't need to do anything for it...    
}

static int venus_i2c_suspend(struct device *dev, pm_message_t state, u32 level)
{
	struct platform_device* pdev   = to_platform_device(dev);  
	i2c_venus_algo*         p_algo = (i2c_venus_algo*) venus_i2c_adapter[pdev->id].algo_data;
	venus_i2c*              p_this = (venus_i2c*) p_algo->dev_id;		
	
	if (p_this && level==SUSPEND_POWER_DOWN)
	    p_this->suspend(p_this);        	        
	    
    return 0;    
}


static int venus_i2c_resume(struct device *dev, u32 level)
{    
    struct platform_device* pdev   = to_platform_device(dev);  
	i2c_venus_algo*         p_algo = (i2c_venus_algo*) venus_i2c_adapter[pdev->id].algo_data;
	venus_i2c*              p_this = (venus_i2c*) p_algo->dev_id;		
	
	if (p_this && level==RESUME_POWER_ON)
	    p_this->resume(p_this);
	    
    return 0;    
}


static struct device_driver venus_i2c_drv = 
{
    .name     =	VENUS_I2C_NAME,
	.bus	  =	&platform_bus_type,	
    .probe    = venus_i2c_probe,
    .remove   = venus_i2c_remove,
    .shutdown = venus_i2c_shutdown,
    .suspend  = venus_i2c_suspend,
    .resume   = venus_i2c_resume,
};

#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// Device Attribute
//////////////////////////////////////////////////////////////////////////////////////////////




static int venus_i2c_show_speed(struct device *dev, struct device_attribute *attr, char* buf)
{    
    struct platform_device* pdev   = to_platform_device(dev);  
	i2c_venus_algo*         p_algo = (i2c_venus_algo*) venus_i2c_adapter[pdev->id].algo_data;
	venus_i2c*              p_this = (venus_i2c*) p_algo->dev_id;				
	    
	if (p_this)    
	    return sprintf(buf, "%d\n", p_this->spd);    
                    
    return 0;    
}



static int venus_i2c_store_speed(struct device *dev, struct device_attribute *attr, char* buf,  size_t count)
{    
    
    struct platform_device* pdev   = to_platform_device(dev);  
	i2c_venus_algo*         p_algo = (i2c_venus_algo*) venus_i2c_adapter[pdev->id].algo_data;
	venus_i2c*              p_this = (venus_i2c*) p_algo->dev_id;				
	int                     spd;
				
    if (p_this && sscanf(buf,"%d\n", &spd)==1)     
        p_algo->set_speed(p_this, spd);
        
    return count;    
}


DEVICE_ATTR(speed, S_IRUGO | S_IWUGO, venus_i2c_show_speed, venus_i2c_store_speed);



//////////////////////////////////////////////////////////////////////////////////////////////
// Module
//////////////////////////////////////////////////////////////////////////////////////////////



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
    int i;
    venus_i2c* p_this;
    
    memset(venus_i2c_adapter,0, sizeof(venus_i2c_adapter)); 
    memset(i2c_venus_data,0, sizeof(i2c_venus_data)); 
    
    for (i=0; i<MAX_I2C_CNT; i++)
    {                            
        p_this = create_venus_i2c_handle(i, VENUS_MASTER_7BIT_ADDR, 
                    ADDR_MODE_7BITS, SPD_MODE_SS, VENUS_I2C_IRQ);

	    if (p_this!=NULL)
	    {	        
            if (p_this->init(p_this)<0 || 
                i2c_venus_register_adapter(&venus_i2c_adapter[i], &i2c_venus_data[i], p_this)<0)
            {
                destroy_venus_i2c_handle(p_this);                         
                continue;
            }
            
            device_create_file(&venus_i2c_adapter[i].dev, &dev_attr_speed);            

#ifdef CONFIG_PM        
            p_venus_i2c[i] = platform_device_register_simple(VENUS_I2C_NAME, i, NULL, 0);                                    
#endif                    
        }        
    }
    
#ifdef CONFIG_PM        
    driver_register(&venus_i2c_drv);
#endif                        

    return 0;
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
__exit i2c_venus_module_exit(void)
{	
    int i;
    
    for (i=0; i<MAX_I2C_CNT; i++)
    {          
        i2c_venus_algo* p_algo = (i2c_venus_algo*) venus_i2c_adapter[i].algo_data;
    
        device_remove_file(&venus_i2c_adapter[i].dev, &dev_attr_speed);
    
	    i2c_venus_del_bus(&venus_i2c_adapter[i]);
	
        destroy_venus_i2c_handle((venus_i2c*)p_algo->dev_id);	
        
#ifdef CONFIG_PM            
        if (p_venus_i2c[i])
        {
            // we don't need to free the handle of p_venus_i2c
            // it will be released by platform subsystem automatically
            platform_device_unregister(p_venus_i2c[i]);        	    
        }            
#endif
    }     

#ifdef CONFIG_PM            
    driver_unregister(&venus_i2c_drv);    
#endif           
}



MODULE_AUTHOR("Kevin-Wang <kevin_wang@realtek.com.tw>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for Realtek Venus/Neptune DVR");
MODULE_LICENSE("GPL");

module_init(i2c_venus_module_init);
module_exit(i2c_venus_module_exit);

