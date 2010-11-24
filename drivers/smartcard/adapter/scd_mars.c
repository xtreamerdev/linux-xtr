/* ------------------------------------------------------------------------- 
   scd_mars.c  scd driver for Realtek Neptune/Mars           
   ------------------------------------------------------------------------- 
    Copyright (C) 2008 Kevin Wang <kevin_wang@realtek.com.tw>

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
----------------------------------------------------------------------------
Update List :
----------------------------------------------------------------------------
    1.0     |   20090312    |   Kevin Wang  | 1) create phase
----------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/io.h>
#include "scd_mars_priv.h"
#include "scd_mars_reg.h"

MODULE_LICENSE("GPL");


/*------------------------------------------------------------------
 * Func : ops_probe
 *
 * Desc : probe a scd device
 *
 * Parm : dev
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/
static 
int ops_probe(scd_device* dev)
{    
    mars_scd* p_scd;
    
    if ((dev->id & 0xFFFFFFFC)!=0x12830000)
        return -ENODEV;                            
                
    p_scd = mars_scd_open((dev->id & 0x3));
    
    if (p_scd==NULL)
        return -ENOMEM;
   
    scd_set_drvdata(dev, p_scd);    // attach it to the scd_device
    return 0;     
}



/*------------------------------------------------------------------
 * Func : ops_remove
 *
 * Desc : this function will be invoked when a smart card device which 
 *        associated with this driver has been removed
 *
 * Parm : dev : smart card device to be removed
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void ops_remove(scd_device* dev)
{       
    mars_scd_close((mars_scd*) scd_get_drvdata(dev));
}



/*------------------------------------------------------------------
 * Func : ops_enable
 *
 * Desc : enable IFD
 *
 * Parm : dev    : ifd device
 *        on_off : 0      : disable IFD 
 *                 others : enable IFD
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/
static 
int ops_enable(scd_device* dev, unsigned char on_off)
{    
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);
    
    return p_this->enable(p_this, on_off);        
}



/*------------------------------------------------------------------
 * Func : ops_set_param
 *
 * Desc : set param of IFD
 *
 * Parm : dev
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/
static 
int ops_set_param(scd_device* dev, scd_param* p_param)
{    
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);
    int ret;
    ret  = p_this->set_clock(p_this, p_param->clk);
    ret |= p_this->set_etu(p_this, p_param->etu);
    ret |= p_this->set_parity(p_this, p_param->parity);
    return ret;
}



/*------------------------------------------------------------------
 * Func : ops_get_param
 *
 * Desc : get param of IFD
 *
 * Parm : dev
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/
static 
int ops_get_param(scd_device* dev, scd_param* p_param)
{    
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);
    int ret;    
    ret  = p_this->get_clock(p_this,  &p_param->clk);
    ret |= p_this->get_etu(p_this,    &p_param->etu);
    ret |= p_this->get_parity(p_this, &p_param->parity);  
    return ret;
}




/*------------------------------------------------------------------
 * Func : ops_activate
 *
 * Desc : activate ICC via reset procedure
 *
 * Parm : dev
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/        
static 
int ops_activate(scd_device* dev)
{            
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);    
    return p_this->activate(p_this);    
}




/*------------------------------------------------------------------
 * Func : ops_deactivate
 *
 * Desc : deactivate icc
 *
 * Parm : dev
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/        
static 
int ops_deactivate(scd_device* dev)
{            
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);    
    return p_this->deactivate(p_this);    
}



/*------------------------------------------------------------------
 * Func : ops_reset
 *
 * Desc : reset a scd device 
 *
 * Parm : dev
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/        
static 
int ops_reset(scd_device* dev)
{            
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);    
    return p_this->reset(p_this);    
}



/*------------------------------------------------------------------
 * Func : ops_get_atr
 *
 * Desc : get atr from a scd device 
 *
 * Parm : dev 
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/        
static 
int ops_get_atr(scd_device* dev, scd_atr* p_atr)
{
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);              
    return p_this->get_atr(p_this, p_atr);            
}


/*------------------------------------------------------------------
 * Func : ops_get_card_status
 *
 * Desc : get card status of IFD 
 *
 * Parm : dev 
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/        
static 
int ops_get_card_status (scd_device* dev)
{    
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);    
    return p_this->get_card_status(p_this);
}


/*------------------------------------------------------------------
 * Func : ops_reset
 *
 * Desc : reset a scd device 
 *
 * Parm : dev  :   
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/        
static 
int ops_poll_card_status(scd_device* dev)
{
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);    
    return p_this->poll_card_status(p_this);    
}

  
/*------------------------------------------------------------------
 * Func : ops_xmit
 *
 * Desc : xmit data via smart card bus
 *
 * Parm : dev  :   
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/        
static 
int ops_xmit(scd_device* dev, sc_buff* scb)
{   
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);    
    return p_this->xmit(p_this, scb);     
}                                 
              
              
  
/*------------------------------------------------------------------
 * Func : ops_read
 *
 * Desc : read data via smart card bus
 *
 * Parm : dev  :   
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/        
static 
sc_buff* ops_read(scd_device* dev)
{   
    mars_scd* p_this = (mars_scd*) scd_get_drvdata(dev);
    return p_this->read(p_this);
}                                 
                                                                  

static scd_device mars_scd_controller[2] = 
{
    {
        .id   = ID_MARS_SCD(0),
        .name = "mars_scd_0"
    },
    {
        .id   = ID_MARS_SCD(1),
        .name = "mars_scd_1"    
    },
};


static scd_driver mars_scd_driver = 
{    
    .name               = "mars_scd",
    .probe              = ops_probe,
    .remove             = ops_remove,        
    .suspend            = NULL,
    .resume             = NULL,
    .enable             = ops_enable,
    .set_param          = ops_set_param,
    .get_param          = ops_get_param,    
    .activate           = ops_activate,
    .deactivate         = ops_deactivate,
    .reset              = ops_reset,
    .get_atr            = ops_get_atr,
    .get_card_status    = ops_get_card_status,
    .poll_card_status   = ops_poll_card_status,
    .xmit               = ops_xmit,
    .read               = ops_read,
};




/*------------------------------------------------------------------
 * Func : mars_scd_module_init
 *
 * Desc : init mars smart card interface driver
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static int __init mars_scd_module_init(void)
{                        
    SC_INFO("mars scd module init\n");        
                                
    if (register_scd_driver(&mars_scd_driver)!=0)
        return -EFAULT;                                        

    register_scd_device(&mars_scd_controller[0]);          // register scd device
    register_scd_device(&mars_scd_controller[1]);          // register scd device
                            
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_module_exit
 *
 * Desc : uninit mars smart card interface driver
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static void __exit mars_scd_module_exit(void)
{            
    unregister_scd_device(&mars_scd_controller[0]);
    unregister_scd_device(&mars_scd_controller[1]);
    unregister_scd_driver(&mars_scd_driver);
}


module_init(mars_scd_module_init);
module_exit(mars_scd_module_exit);
