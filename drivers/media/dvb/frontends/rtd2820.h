
#ifndef __RTD2820_H__
#define __RTD2820_H__

#define URB_PARK_EN 
#define RTD2820_DEBUG

#define RTD2820_CHANNEL_SCAN_URB_SIZE    (512)
#define RTD2820_CHANNEL_SCAN_URB_NUMBER  (1)

struct rtd2820_config
{
    /* the demodulator's i2c address */
    u8          demod_address;
        
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


struct dvb_frontend* rtd2820_attach(const struct rtd2820_config* config, struct i2c_adapter* i2c);

struct rtd2820_state 
{
    void*                               hRTD2820;
    u8                                  flag;
    
    struct i2c_adapter*                 i2c;
    struct dvb_frontend                 frontend;
    struct dvb_frontend_ops             ops;     

    /* configuration settings */
    struct rtd2820_config               config;    
    
    /* ring buffer info */
    struct rtd2830_ts_buffer_s*         p_ts_ring_info;    
    u8*                                 p_ts_mmap_region;
    int                                 len_ts_mmap_region;
    u8*                                 p_next_urb_buffer;
#ifdef URB_PARK_EN
    u8*                                 p_urb_park;
    int                                 len_urb_park;
#endif    
    // current tuner parameter
    struct dvb_frontend_parameters      fe_param;
};



#define RTD2820_STRAMING_ON             0x01
#define RTD2820_CHANNEL_SCAN_MODE_EN    0x02
#define RTD2820_URB_RDY                 0x04


//-------- Public Functions
struct dvb_frontend* rtd2820_attach(const struct rtd2820_config* config, struct i2c_adapter* i2c);
void rtd2820_i2c_repeat( struct dvb_frontend* fe);

#endif // __RTD2820_H__
