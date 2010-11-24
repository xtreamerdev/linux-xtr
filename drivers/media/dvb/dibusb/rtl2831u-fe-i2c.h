#ifndef __RTL2831U_FE_I2C_H__
#define __RTL2831U_FE_I2C_H__

#include "rtd2830_cypress-fe-i2c.h" 

extern struct i2c_algorithm rtl2831u_algo;

int rtl2831u_i2c_xfer(struct i2c_adapter *adap,struct i2c_msg *msg,int num);
u32 rtl2831u_i2c_func(struct i2c_adapter *adapter);
int rtl2831u_tuner_quirk(struct usb_dibusb *dib);
int rtl2831u_streaming(void *dib,int onoff);


// macro
#define rtl2831u_urb_init       rtd2830_urb_init
#define rtl2831u_urb_reset      rtd2830_urb_reset   
#define rtl2831u_urb_exit       rtd2830_urb_exit


#ifdef RTL2831U_DBG_EN
    #define RTL2831U_DBG                  printk
#else
    #define RTL2831U_DBG               
#endif

#define RTL2831U_WARNING                  printk

#endif //__RTL2831U_FE_I2C_H__
