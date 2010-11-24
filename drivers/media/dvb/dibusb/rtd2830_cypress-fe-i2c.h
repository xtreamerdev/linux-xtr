#ifndef __RTD2830_CYPRESS_FE_I2C_H__
#define __RTD2830_CYPRESS_FE_I2C_H__

extern struct i2c_algorithm rtd2830_algo;

int rtd2830_i2c_xfer(struct i2c_adapter *adap,struct i2c_msg *msg,int num);

int rtd2830_i2c_msg(
    struct usb_dibusb*      dib, 
    u8                      addr,
    u8*                     wbuf, 
    u16                     wlen, 
    u8*                     rbuf, 
    u16                     rlen);
    
int rtd2830_user_cmd(
    struct usb_dibusb*      dib, 
    u8*                     sndbuf, 
    u8                      snd_len);    
    
u32 rtd2830_i2c_func(struct i2c_adapter *adapter);

int rtd2830_tuner_quirk(struct usb_dibusb *dib);

int rtd2830_streaming(void *dib,int onoff);

int rtd2830_urb_init(
    void*               dib1,
    u8*                 p_buffer,
    int                 urb_number,
    int                 urb_size
    );
    
int rtd2830_urb_reset(void* dib1);

int rtd2830_urb_exit(void* dib1);


#ifdef RT2830_CYPRESS_DBG_EN
    #define RT2830_CYPRESS_DBG               printk
#else
    #define RT2830_CYPRESS_DBG               
#endif

#define RT2830_CYPRESS_WARNING               printk

    
#endif //__RTD2830_CYPRESS_FE_I2C_H__
