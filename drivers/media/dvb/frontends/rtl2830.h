/*
 *  Driver for Realtek DVB-T rtd2830 demodulator
 */

#ifndef __RTD2830_H__
#define __RTD2830_H__

#include <linux/dvb/frontend.h>
#include <linux/jiffies.h>

//--------------------------------------
#define RTD2830_CRYSTAL_288
#define RTD2830_DEBUG
//--------------------------------------


#define RTL2831U_URB_SIZE                (4096)
#define RTL2831U_URB_NUMBER              (7)

#define RTD2830_URB_SIZE                 (192)
#define RTD2830_URB_NUMBER               (8)

#define RTD2830_CHANNEL_SCAN_URB_SIZE    (512)
#define RTD2830_CHANNEL_SCAN_URB_NUMBER  (1)



//--- TS Buffer Size
#define RTD2830_TS_BUFFER_SIZE                  (256*1024)

#if ((128*1024)== RTD2830_TS_BUFFER_SIZE)
  #define RTD2830_TS_STREAM_BUFFER_SIZE           (120*1024)
#elif ((256*1024)== RTD2830_TS_BUFFER_SIZE)
  #define RTD2830_TS_STREAM_BUFFER_SIZE           (252*1024)
#elif ((512*1024)== RTD2830_TS_BUFFER_SIZE)
  #define RTD2830_TS_STREAM_BUFFER_SIZE           (504*1024)
#endif

enum{
    RTL2830,
    RTL2830B,
    RTL2831U,    
};

enum{
    RTL2830_6MHZ_SERIAL_OUT,
    RTL2830_10MHZ_SERIAL_OUT,
    RTL2830_6MHZ_PARALLEL_OUT,
    RTL2830_10MHZ_PARALLEL_OUT,
    RTL2830_MAX_OUTPUTMODE
};

struct rtd2830_config
{
    /* the demodulator's i2c address */
    u8          demod_address;
    u8          version;
    u8          output_mode;
        
    void*       dibusb;

    /* the tuner's i2c address */
    // u8          tuner_address;
    
    // @FIXME: Richard: don't know what the below fields mean
    /* frequencies in kHz */
    int         adc_clock;  // default: 20480
    int         if2;        // default: 36166

    // @FIXME: Richard: I guess this no_tuner is useless and could be removed
    /* set if no pll is connected to the secondary i2c bus */
    int         no_tuner;
    int         urb_num;
    int         urb_size;

    // Richard: @FIXME remove it. Don't know this removal is good or not
    /* Initialise the demodulator and PLL. Cannot be NULL */
    // int         (*demod_init)(struct dvb_frontend* fe);

    // Richard: @FIXME: add this. Don't know this add is good or not
	u8 (*pll_addr)(struct dvb_frontend *fe);
	int (*pll_init)(struct dvb_frontend *fe, u8 pll_buf[5]);
	int (*pll_set)(struct dvb_frontend *fe, struct dvb_frontend_parameters* params, u8 pll_buf[5]);
	void(*pll_uninit)(struct dvb_frontend *fe);
	int (*p_dibusb_streaming)(void*, int on_off);
	int (*p_urb_alloc)(void* dib, u8* p_buffer, int len_buffer, int urb_size);
    int (*p_urb_free)(void* dib);
    int (*p_urb_reset)(void* dib);
};



struct rtd2830_state 
{
    void*                           pRT2830;  
    u8                              version;    
    u8                              output_mode;    
    u8                              flag;
    
    int                             ext_flag;
    unsigned long                   update_interval_ms;    
    unsigned long                   next_update_time;
    struct i2c_adapter*             i2c;
    struct dvb_frontend             frontend;
    struct dvb_frontend_ops         ops;            

    /* configuration settings */    
    struct rtd2830_config           config;    
    
    /* ring buffer info */
    struct rtd2830_ts_buffer_s*     p_ts_ring_info;
    u8*                             p_ts_mmap_region;
    int                             len_ts_mmap_region;
    u8*                             p_next_urb_buffer;
    
    // current tuner parameter
    struct dvb_frontend_parameters  fe_param;
};



#define RTD2830_STRAMING_ON             0x01
#define RTD2830_CHANNEL_SCAN_MODE_EN    0x02
#define RTD2830_URB_RDY                 0x04



extern struct dvb_frontend*
rtd2830_attach(
    const struct rtd2830_config*                config,
    struct i2c_adapter*                         i2c);

extern int
rtd2830_write(
    struct dvb_frontend*                        fe,
    u8*                                         ibuf,
    int                                         ilen);
    
extern void rtd2830_i2c_repeat( struct dvb_frontend* fe);    
        
extern int 
rtd2830_GPIO_Ctrl(
    struct dvb_frontend*                        fe,
    unsigned char                               on_off);

#endif // #ifndef __RTD2830_H__
