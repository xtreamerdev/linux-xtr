/*
 * i2c-algo-venus.c: i2c driver algorithms for Realtek Venus DVR
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * Copyright (C) 2005 Chih-pin Wu <wucp@realtek.com.tw>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include <linux/i2c.h>
#include <linux/proc_fs.h>
//#include <linux/i2c-algo-venus.h>
#include "i2c-algo-venus.h"


/*------------------------------------------------------------------
 * Func : venus_xfer
 *
 * Desc : 
 *
 * Parm : adapter : i2c adapter
 *
 * Retn : 
 *------------------------------------------------------------------*/ 
static 
int venus_xfer(
    struct i2c_adapter*     i2c_adap, 
    struct i2c_msg*         msgs, 
    int                     num
    )
{
	struct i2c_algo_venus_data *adap = i2c_adap->algo_data;
	int err = 0;

	err = adap->masterXfer(i2c_adap, msgs, num);

	return err;
}


/*------------------------------------------------------------------
 * Func : venus_func
 *
 * Desc : 
 *
 * Parm : adapter : i2c adapter
 *
 * Retn : 
 *------------------------------------------------------------------*/ 
static 
u32 venus_func(
    struct i2c_adapter*     adap
    )
{
	return I2C_FUNC_SMBUS_EMUL;
}


/*------------------------------------------------------------------
 * Func : venus_set_target_name
 *
 * Desc : Set a target name for a specified address
 *
 * Parm : adapter : i2c adapter
 *        p_arg   : venus i2c name parameter
 *
 * Retn : 
 *------------------------------------------------------------------*/ 
int venus_set_target_name(
    struct i2c_adapter*     adapter,     
    unsigned char           addr,
    const char*             name, 
    unsigned char           length 
    )
{
    struct i2c_algo_venus_data *pData = (struct i2c_algo_venus_data*) adapter->algo_data;
        
    if (addr > 0x7F)			    
        return -EFAULT;
                    
    if (pData->p_target_info[addr] != NULL) 
    {                    
        printk("[I2C] reset target name (%02x) : %s --> %s\n", 
            addr,
            pData->p_target_info[addr]->name,
            name);
    }
    else 
    {            
        pData->p_target_info[addr] = kmalloc(sizeof(target_info), GFP_KERNEL);
        memset(pData->p_target_info[addr], 0, sizeof(target_info));        
    }
            
    memcpy(pData->p_target_info[addr]->name, name, 
            (length < (MAX_NAME_LENGTH-1)) ? length : (MAX_NAME_LENGTH-1));    
    
    return 0;
}

#define I2C_SET_TARGET_NAME     0x790


/*------------------------------------------------------------------
 * Func : venus_ioctl
 *
 * Desc : ioctl for venus i2c
 *
 * Parm : adapter : i2c adapter
 *        cmd     : command
 *        arg     : arguments
 *
 * Retn : 
 *------------------------------------------------------------------*/  
static 
int venus_ioctl( 
    struct i2c_adapter*     adapter, 
    unsigned int            cmd, 
    unsigned long           arg
    )
{    
    venus_i2c_name_param    name_arg;
    
    switch (cmd)
    {
    case I2C_SET_TARGET_NAME:
    
		if (copy_from_user(&name_arg, (venus_i2c_name_param __user *) arg, sizeof(venus_i2c_name_param)))
			return -EFAULT;			
			                
        return venus_set_target_name(adapter, name_arg.addr, name_arg.name, sizeof(name_arg.name));
        
    }    
    
    return -EFAULT;
}	
	

static 
struct i2c_algorithm venus_algo = {
	.name			= "Venus algorithm",
#define I2C_ALGO_VENUS 0x1b0000         /* Venus algorithm, FIXME, move to i2c-id.h later */
	.id				= I2C_ALGO_VENUS,
	.master_xfer	= venus_xfer,
	.algo_control	= venus_ioctl,     /* ioctl */
	.functionality	= venus_func,
};



/*------------------------------------------------------------------
 * Func : venus_i2c_read_proc
 *
 * Desc : display proc information of venus i2c 
 *
 * Parm : page   : page alignmented output data buffer 
 *        start  : start position (start read)
 *        off    : offset
 *        count  : ????
 *        eof    : End of File ?
 *        data   : private data
 *
 * Retn : out data length
 *------------------------------------------------------------------*/   
int venus_i2c_read_proc(
    char*           page, 
    char**          start, 
    off_t           off,
    int             count, 
    int*            eof, 
    void*           data
    )
{
    struct i2c_adapter * adapter = (struct i2c_adapter*) data;    
    struct i2c_algo_venus_data *pData = (struct i2c_algo_venus_data*) adapter->algo_data;
    int i;
    int len = 0;
  
    len += sprintf(page, "%-8s %-8s %-8s %-8s %-8s %-31s\n",
                    "ADDR", "TX", "TXE", "RX", "RXE", "OWNER");
        
    len += sprintf(page+len, "---------------------------------------------------------\n");
    for (i=0; i < 128; i++) 
    {
        if (pData->p_target_info[i]!=NULL) 
        {            
            len += sprintf(page + len, "%-8x %-8u %-8u %-8u %-8u %-31s\n",
                    i, 
                    pData->p_target_info[i]->tx_count,
                    pData->p_target_info[i]->tx_err,
                    pData->p_target_info[i]->rx_count,
                    pData->p_target_info[i]->rx_err,
                    pData->p_target_info[i]->name);
        }
    }
  
    return len;

}
 
 
/*------------------------------------------------------------------
 * Func : i2c_venus_add_bus
 *
 * Desc : add an venus i2c adapter onto i2c bus
 *
 * Parm : adapter : i2c adapter 
 *
 * Retn : 
 *------------------------------------------------------------------*/  
int i2c_venus_add_bus(struct i2c_adapter *adap)
{        
	adap->id  |= venus_algo.id;
	adap->algo = &venus_algo;        
    create_proc_read_entry("venus_i2c", 0, proc_root_driver, venus_i2c_read_proc, adap);    
	return i2c_add_adapter(adap);
}



/*------------------------------------------------------------------
 * Func : i2c_venus_add_bus
 *
 * Desc : delete an venus i2c adapter from i2c bus
 *
 * Parm : adapter : i2c adapter 
 *
 * Retn : 
 *------------------------------------------------------------------*/
int i2c_venus_del_bus(struct i2c_adapter *adap)
{    
    remove_proc_entry("venus_i2c", proc_root_driver);
	return i2c_del_adapter(adap);
}



EXPORT_SYMBOL(i2c_venus_add_bus);
EXPORT_SYMBOL(i2c_venus_del_bus);

MODULE_AUTHOR("Chih-pin Wu <wucp@realtek.com.tw>");
MODULE_DESCRIPTION("I2C-Bus Venus algorithm");
MODULE_LICENSE("GPL");
