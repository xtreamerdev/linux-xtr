/*
 * dvb-dibusb-fe-i2c.c is part of the driver for mobile USB Budget DVB-T devices
 * based on reference design made by DiBcom (http://www.dibcom.fr/)
 *
 * Copyright (C) 2004-5 Patrick Boettcher (patrick.boettcher@desy.de)
 *
 * see dvb-dibusb-core.c for more copyright details.
 *
 * This file contains functions for attaching, initializing of an appropriate
 * demodulator/frontend. I2C-stuff is also located here.
 *
 */
#include <linux/usb.h> 
#include <linux/delay.h> 
#include "dvb-dibusb.h"
#include "rtd2830_cypress-fe-i2c.h"
#include "rtl2831u-fe-i2c.h"            
#include "gl861-fe-i2c.h"               
#include "qt1010.h"
#include "thomson_fe664x.h"
#include "panasonic_env77h11d5.h"
#include "panasonic_env57h1xd5.h"
#include "microtune_mt2060.h"
#include "philips_td1316.h"
#include "thomson_ddt7680x.h"
#include "thomson_dpt78010.h"

#ifdef DVB_RTL2830_EN
#include "../frontends/rtl2830_priv.h"
#endif

#ifdef ATSC_RTD2820_EN
#include "../frontends/rtd2820.h"
#endif



///-------------- Internal Used Function Prototype ---------------------//
static int  dibusb_i2c_xfer          (struct i2c_adapter* adap,
                                      struct i2c_msg*     msg,
                                      int                 num);                                      
static u32  dibusb_i2c_func          (struct i2c_adapter *adapter);
static void dibusb_general_pll_uninit(struct dvb_frontend *fe);
static u8   dibusb_general_pll_addr  (struct dvb_frontend *fe);
static int  dibusb_general_pll_init  (struct dvb_frontend *fe, u8 pll_buf[5]);
static int  dibusb_general_pll_set   (struct dvb_frontend *fe, 
                                      struct dvb_frontend_parameters* params, 
                                      u8 pll_buf[5]);

///-------------- Internal Used Data ---------------------//
static struct i2c_algorithm dibusb_algo = 
{
	.name			= "DiBcom USB i2c algorithm",
	.id				= I2C_ALGO_BIT,
	.master_xfer	= dibusb_i2c_xfer,
	.functionality	= dibusb_i2c_func,
};


#ifdef DVB_RTL2830_EN

static struct rtd2830_config rtk_rtd2830_config = 
{
    .demod_address      = 0x10,
    .version            = RTL2830,
    .output_mode        = RTL2830_6MHZ_PARALLEL_OUT,
    .urb_num            = 7,
    .urb_size           = 4096,
    .pll_init          	= dibusb_general_pll_init,
    .pll_addr           = dibusb_general_pll_addr,
    .pll_set            = dibusb_general_pll_set,
    .pll_uninit         = dibusb_general_pll_uninit,
    .p_dibusb_streaming = rtd2830_streaming,
    .p_urb_alloc      	= rtd2830_urb_init,
    .p_urb_free       	= rtd2830_urb_exit,
    .p_urb_reset     	= rtd2830_urb_reset,
};

static struct rtd2830_config rtk_rtl2831u_config = 
{
    .demod_address      = 0x10,
    .version            = RTL2831U,
    .output_mode        = RTL2830_10MHZ_PARALLEL_OUT,
    .urb_num            = 7,
    .urb_size           = 4096,
    .pll_init          	= dibusb_general_pll_init,
    .pll_addr           = dibusb_general_pll_addr,
    .pll_set            = dibusb_general_pll_set,
    .pll_uninit         = dibusb_general_pll_uninit,
    .p_dibusb_streaming = rtl2831u_streaming,
    .p_urb_alloc      	= rtl2831u_urb_init,
    .p_urb_free       	= rtl2831u_urb_exit,
    .p_urb_reset     	= rtl2831u_urb_reset,
};

#endif

#ifdef ATSC_RTD2820_EN

static struct rtd2820_config rtk_rtd2820_config = 
{
    .demod_address      = 0x12,        
    .urb_num            = 7,
    .urb_size           = 4096,
    .pll_init          	= dibusb_general_pll_init,
    .pll_addr           = dibusb_general_pll_addr,
    .pll_set            = dibusb_general_pll_set,
    .pll_uninit         = dibusb_general_pll_uninit,
    .p_dibusb_streaming = gl861_streaming,
    .p_urb_alloc      	= gl861_urb_init,
    .p_urb_free       	= gl861_urb_exit,
    .p_urb_reset     	= gl861_urb_reset,
};

#endif

///-------------- Internal Used Function Prototype ---------------------//


/*
 * I2C master xfer function
 */
static int dibusb_i2c_xfer(struct i2c_adapter *adap,struct i2c_msg *msg,int num)
{
	return num;
}



static u32 dibusb_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C;
}



static int dibusb_tuner_quirk(struct usb_dibusb *dib)
{   
	switch (dib->dibdev->dev_cl->id) 
	{
		case DIBUSB1_1:             /* some these device have the ENV77H11D5 and some the THOMSON CABLE */
		case DIBUSB1_1_AN2235: {    /* actually its this device, but in warm state they are indistinguishable */
		
			struct dibusb_tuner *t;
			u8 b[2] = { 0,0 } ,b2[1];
			struct i2c_msg msg[2] = 
			{
				{ .flags = 0, .buf = b, .len = 2 },
				{ .flags = I2C_M_RD, .buf = b2, .len = 1},
			};

			t = &dibusb_tuner[DIBUSB_TUNER_COFDM_PANASONIC_ENV77H11D5];

			msg[0].addr = msg[1].addr = t->pll_addr;

			if (dib->xfer_ops.tuner_pass_ctrl != NULL)
				dib->xfer_ops.tuner_pass_ctrl(dib->fe,1,t->pll_addr);
				
			dibusb_i2c_xfer(&dib->i2c_adap,msg,2);
			
			if (dib->xfer_ops.tuner_pass_ctrl != NULL)
				dib->xfer_ops.tuner_pass_ctrl(dib->fe,0,t->pll_addr);

			if (b2[0] == 0xfe)
				info("this device has the Thomson Cable onboard. Which is default.");
			else 
			{
				dib->tuner = t;
				info("this device has the Panasonic ENV77H11D5 onboard.");
			}
			
			break;
		}

#ifdef DVB_RTL2830_EN		

		case CLASS_RTL2831U:       
		    return rtl2831u_tuner_quirk(dib);
		    
		case CLASS_RTD2830:       
		    return rtd2830_tuner_quirk(dib);
#endif
		    
		default:
			break;
	}
	return 0;
}


int dibusb_fe_init(struct usb_dibusb* dib)
{
	struct dib3000_config demod_cfg;
	int i;

	if (dib->init_state & DIBUSB_STATE_I2C) {
	    
		for (i = 0; i < sizeof(dib->dibdev->dev_cl->demod->i2c_addrs) / sizeof(unsigned char) &&
				dib->dibdev->dev_cl->demod->i2c_addrs[i] != 0; i++) 
        {
			demod_cfg.demod_address = dib->dibdev->dev_cl->demod->i2c_addrs[i];
			demod_cfg.pll_addr = dibusb_general_pll_addr;
			demod_cfg.pll_set  = dibusb_general_pll_set;
			demod_cfg.pll_init = dibusb_general_pll_init;

			deb_info("demod id: %d %d\n",dib->dibdev->dev_cl->demod->id,DTT200U_FE);

			switch ((int)dib->dibdev->dev_cl->demod->id) {

#ifdef DVB_RTL2830_EN

            case DEMOD_RTL2831U:
                rtk_rtl2831u_config.demod_address= dib->dibdev->dev_cl->demod->i2c_addrs[i];
                rtk_rtl2831u_config.urb_num = dib->dibdev->dev_cl->urb_count;
                rtk_rtl2831u_config.urb_size = dib->dibdev->dev_cl->urb_buffer_size;
				rtk_rtl2831u_config.dibusb= (void*)dib;
				dib->fe= rtd2830_attach(&rtk_rtl2831u_config, &dib->i2c_adap);
				break;			
						
			case DEMOD_RTD2830:
				rtk_rtd2830_config.demod_address = dib->dibdev->dev_cl->demod->i2c_addrs[i];
                rtk_rtd2830_config.urb_num       = dib->dibdev->dev_cl->urb_count;
                rtk_rtd2830_config.urb_size      = dib->dibdev->dev_cl->urb_buffer_size;
				rtk_rtd2830_config.dibusb        = (void*)dib;
				dib->fe= rtd2830_attach(&rtk_rtd2830_config, &dib->i2c_adap);
				break;			
#endif

#ifdef ATSC_RTD2820_EN
					
            case DEMOD_RTL2820:
				rtk_rtd2820_config.demod_address = dib->dibdev->dev_cl->demod->i2c_addrs[i];
                rtk_rtd2820_config.urb_num       = dib->dibdev->dev_cl->urb_count;
                rtk_rtd2820_config.urb_size      = dib->dibdev->dev_cl->urb_buffer_size;
				rtk_rtd2820_config.dibusb        = (void*)dib;
				dib->fe= rtd2820_attach(&rtk_rtd2820_config, &dib->i2c_adap);
				break;	
				
#endif										
		    }
		
			if (dib->fe != NULL) {
				info("found demodulator at i2c address 0x%x",dib->dibdev->dev_cl->demod->i2c_addrs[i]);
				break;
			}
		}	
		
		/* if a frontend was found */
		if (dib->fe != NULL) {
		    
			if (dib->fe->ops->sleep != NULL)
				dib->fe_sleep = dib->fe->ops->sleep;
				
			dib->fe->ops->sleep = dibusb_hw_sleep;

			if (dib->fe->ops->init != NULL )
				dib->fe_init = dib->fe->ops->init;
								
			dib->fe->ops->init = dibusb_hw_wakeup;

			/* setting the default tuner */
			dib->tuner = dib->dibdev->dev_cl->tuner;

			/* check which tuner is mounted on this device, in case this is unsure */
			dibusb_tuner_quirk(dib);
		}
	}
	
	if (dib->fe == NULL) {
		err("A frontend driver was not found for device '%s'.",dib->dibdev->name);
		return -ENODEV;
	}
	else {
        printk("Frontend registration .");
		if (dvb_register_frontend(&dib->adapter, dib->fe)) {
			err("Frontend registration failed.");
			if (dib->fe->ops->release)
				dib->fe->ops->release(dib->fe);
			dib->fe = NULL;
			return -ENODEV;
		}				
	}
	return 0;
}




int dibusb_fe_exit(struct usb_dibusb *dib)
{
	if (dib->fe != NULL)
		dvb_unregister_frontend(dib->fe);
	return 0;
}



int dibusb_i2c_init(struct usb_dibusb *dib)
{
	int ret = 0;
	dib->adapter.priv = dib;

	strncpy(dib->i2c_adap.name,dib->dibdev->name,I2C_NAME_SIZE);
	
#ifdef I2C_ADAP_CLASS_TV_DIGITAL
	dib->i2c_adap.class = I2C_ADAP_CLASS_TV_DIGITAL;
#else
	dib->i2c_adap.class = I2C_CLASS_TV_DIGITAL;
#endif



    switch (dib->dibdev->dev_cl->id)
    {        
    case CLASS_RTL2831U:        
        dib->i2c_adap.algo		= &rtl2831u_algo;
	    dib->i2c_adap.algo_data = NULL;
	    dib->i2c_adap.id		= I2C_ALGO_BIT;
	    break;
	    
    case CLASS_RTD2830:   
        dib->i2c_adap.algo		= &rtd2830_algo;
	    dib->i2c_adap.algo_data = NULL;
	    dib->i2c_adap.id		= I2C_ALGO_BIT;
	    break;     
	    
    case CLASS_RTL2830_GL861:	    
    case CLASS_RTL2820_GL861:	    
        dib->i2c_adap.algo		= &gl861_algo;
	    dib->i2c_adap.algo_data = NULL;
	    dib->i2c_adap.id		= I2C_ALGO_BIT;	    
	    break;             
    
    default:
	    dib->i2c_adap.algo		= &dibusb_algo;
	    dib->i2c_adap.algo_data = NULL;
	    dib->i2c_adap.id		= I2C_ALGO_BIT;
	    break;
	}
	
	i2c_set_adapdata(&dib->i2c_adap, dib);
	
	printk("i2c alg = %s\n",dib->i2c_adap.name);

	if ((ret = i2c_add_adapter(&dib->i2c_adap)) < 0)
		err("could not add i2c adapter");

	dib->init_state |= DIBUSB_STATE_I2C;    

	return ret;
}



int dibusb_i2c_exit(struct usb_dibusb *dib)
{        
	if (dib->init_state & DIBUSB_STATE_I2C)
		i2c_del_adapter(&dib->i2c_adap);
	dib->init_state &= ~DIBUSB_STATE_I2C;
	return 0;
}



static u8 dibusb_general_pll_addr(struct dvb_frontend *fe)
{
	struct usb_dibusb* dib = (struct usb_dibusb*) fe->dvb->priv;
	return dib->tuner->pll_addr;
}



static int dibusb_pll_i2c_helper(struct usb_dibusb *dib, u8 pll_buf[5], u8 buf[4])
{
	if (pll_buf == NULL) {
		struct i2c_msg msg = {
			.addr = dib->tuner->pll_addr,
			.flags = 0,
			.buf = buf,
			.len = sizeof(buf)
		};
		if (i2c_transfer (&dib->i2c_adap, &msg, 1) != 1)
			return -EIO;
		msleep(1);
	} else {
		pll_buf[0] = dib->tuner->pll_addr << 1;
		memcpy(&pll_buf[1],buf,4);
	}

	return 0;
}


static int dibusb_general_pll_init(struct dvb_frontend *fe, u8 pll_buf[5])
{
    int ret=0;
    u8  buf[4];
	struct usb_dibusb* dib = (struct usb_dibusb*) fe->dvb->priv;
    
	switch(dib->tuner->id) 
	{    
    //----------	    	
    case DIBUSB_TUNER_COFDM_QT1010:        
        printk("TUNER : QT1010\n");    
        ret = quantek_cofdm_qt1010_init(fe, dib->tuner->pll_addr, &dib->i2c_adap );        
        break;

    case DIBUSB_TUNER_COFDM_THOMSON_FE664:        
        printk("TUNER : FE664X\n");    
        ret = thomson_cofdm_fe664x_init(fe, dib->tuner->pll_addr, &dib->i2c_adap);    
        break;        
             
    case DIBUSB_TUNER_COFDM_MICROTUNE_MT2060:
        printk("TUNER : MT2060\n");
        ret = microtune_cofdm_mt2060_init(fe, dib->tuner->pll_addr, &dib->i2c_adap);       
        break;
    
    case DIBUSB_TUNER_COFDM_PHILIPS_TD1316:
        printk("TUNER : TD1316\n");
        return philips_cofdm_td1316_init(fe, dib->tuner->pll_addr, &dib->i2c_adap);       
        break;        
#if 0                
    case DIBUSB_TUNER_COFDM_LG_TDTMG252D:
        printk("TUNER : TDTMG252D\n");
        return lg_tdtm_g252d_init(fe, dib->tuner->pll_addr, &dib->i2c_adap);       
        break;        
#endif 

    case DIBUSB_TUNER_COFDM_THOMSON_DDT7680X:        
        printk("TUNER : DDT7680X\n");    
        return thomson_cofdm_ddt7680x_init(fe, dib->tuner->pll_addr, &dib->i2c_adap);    
        break;        
        
    case DIBUSB_TUNER_COFDM_THOMSON_DPT78010:        
        printk("TUNER : DPT78010\n");    
        return thomson_cofdm_dpt78010_init(fe, dib->tuner->pll_addr,dib->tuner->ext_pll_addr, &dib->i2c_adap);    
        break;                
        
    default:
        printk("Unknown Tuner %d\n",dib->tuner->id);
        ret = 1;
        break;
	}

    return ret;
}




static void dibusb_general_pll_uninit(struct dvb_frontend *fe)
{    
	struct usb_dibusb* dib = (struct usb_dibusb*) fe->dvb->priv;
    
	switch(dib->tuner->id){         
    case DIBUSB_TUNER_COFDM_QT1010:                  
        quantek_cofdm_qt1010_uninit(fe);                
        return;
        
    case DIBUSB_TUNER_COFDM_THOMSON_FE664:                  
        thomson_cofdm_fe664x_uninit(fe);    
        return;
             
    case DIBUSB_TUNER_COFDM_MICROTUNE_MT2060:        
        microtune_cofdm_mt2060_uninit(fe);        
        return;
    
    case DIBUSB_TUNER_COFDM_PHILIPS_TD1316:        
        philips_cofdm_td1316_uninit(fe);       
        break;   
#if 0    
    case DIBUSB_TUNER_COFDM_LG_TDTMG252D: 
        lg_tdtm_g252d_uninit(fe, dib->tuner->pll_addr, &dib->i2c_adap);       
        break;            
#endif        

    case DIBUSB_TUNER_COFDM_THOMSON_DDT7680X:
        thomson_cofdm_ddt7680x_uninit(fe);    
        return;
    
    case DIBUSB_TUNER_COFDM_THOMSON_DPT78010:
        thomson_cofdm_dpt78010_uninit(fe);    
        return;    
    
    default:
        printk("Unknown Tuner %d\n",dib->tuner->id);
        return;
	}	
}



static int dibusb_general_pll_set(
    struct dvb_frontend             *fe,
	struct dvb_frontend_parameters  *fep, 
	u8                               pll_buf[5])
{
	struct usb_dibusb* dib = (struct usb_dibusb*) fe->dvb->priv;		
	
	u8 buf[6];
	int ret=0;

	switch (dib->tuner->id) 
	{	
	//----
    case DIBUSB_TUNER_COFDM_QT1010:    
        return quantek_cofdm_qt1010_set(fe, fep, buf);
            
    //---            
    case DIBUSB_TUNER_COFDM_THOMSON_FE664:    
        ret = thomson_fe664x_eu_pll_set(fep,buf);
        if (ret)
	        return ret;
        return thomson_cofdm_fe664x_set(fe, fep, buf);	  
        
    //---
    case DIBUSB_TUNER_COFDM_MICROTUNE_MT2060:            
        return microtune_cofdm_mt2060_set(fe,fep,buf);    
    
    //---    
    case DIBUSB_TUNER_COFDM_PHILIPS_TD1316:          
        return philips_cofdm_td1316_set(fe, fep);                 
    
    //---        
#if 0      
    case DIBUSB_TUNER_COFDM_LG_TDTMG252D:         
        return lg_tdtm_g252d_set(fe, fep);
#endif         
    //---
    case DIBUSB_TUNER_COFDM_THOMSON_DDT7680X:            
        return thomson_cofdm_ddt7680x_set(fe, fep);
        
    //---
    case DIBUSB_TUNER_COFDM_THOMSON_DPT78010:            
        return thomson_cofdm_dpt78010_set(fe, fep);
                
    //---        
	default:
		warn("no pll programming routine found for tuner %d.\n",dib->tuner->id);
		ret = -ENODEV;
		break;
	}

    return ret;
}

