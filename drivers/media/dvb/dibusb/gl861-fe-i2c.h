#ifndef __GL861_FE_I2C_H__
#define __GL861_FE_I2C_H__

extern struct i2c_algorithm gl861_algo;

#define GL861_I2C_REG_BYTE_WRITE            1
#define GL861_I2C_REG_BYTE_READ             2
#define GL861_I2C_REG_SHORT_BYTE_WRITE      3
#define GL861_VIDEO_STREAM                  5 

int gl861_i2c_xfer(struct i2c_adapter* adap, struct i2c_msg* msg, int num); 
u32 gl861_i2c_func(struct i2c_adapter *adapter);
int gl861_streaming(void *dib,int onoff);

// I2C Address Definition
#define GL861_I2C_ADDR              0x00

// Register Definition
#define GL861_PIDFILTER_SET12_HB    0x80
#define GL861_PIDFILTER_SET12_LB    0x90
#define GL861_PIDFILTER_SET34_HB    0xa0
#define GL861_PIDFILTER_SET34_LB    0xb0

#define GL861_PIDFILTER_SET1_MASK   0x38
#define GL861_PIDFILTER_SET2_MASK   0x35
#define GL861_PIDFILTER_SET3_MASK   0x39
#define GL861_PIDFILTER_SET4_MASK   0x36

#define GL861_DTV_TMOD_CTL          0x33 
#define GL861_DTV_MASK_EN           0x37

#ifdef GL861_DBG_EN
    #define GL861_DBG               printk
#else
    #define GL861_DBG                
#endif

#define GL861_WARNING               printk

// macro
#define gl861_urb_init       rtd2830_urb_init
#define gl861_urb_reset      rtd2830_urb_reset   
#define gl861_urb_exit       rtd2830_urb_exit

#endif  //__GL861_FE_I2C_H__
