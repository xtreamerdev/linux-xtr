
#ifndef __LG_G252D_H__
#define __LG_G252D_H__



// Definitions

// Constant
#define DIVIDER_MSB_DEFAULT		0x0
#define DIVIDER_LSB_DEFAULT		0X0
#define CONTROL_DEFAULT			0xb4
#define BAND_SWITCH_DEFAULT		0x2
#define AUXILIARY_DEFAULT		0X20

#define CTRL_VALUE_FOR_SETTING_AUX		0x98

// Mask
#define DIVIDER_MASK		0x7fff
#define P3_MASK				0x8
#define P3_SHIFT			3


/////////////////////////////////////////////////////

typedef struct G252D_MODULE_TAG    G252D_MODULE;
struct G252D_MODULE_TAG
{
    unsigned char       tuner_addr;
    struct i2c_adapter* i2c_adap;       
    unsigned long       TargetRF;   
    unsigned long       Bandwidth;     
    
    // Tuner extra data
	unsigned char       DividerMsb;
	unsigned char       DividerLsb;
	unsigned char       Control;
	unsigned char       BandSwitch;
	unsigned char       Auxiliary;
    
};

// for dibusb core
int lg_tdtm_g252d_init(
    void*                   hFE, 
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap
    );
    
    
int  lg_tdtm_g252d_set (void* hFE, void* hFEParam);
void lg_tdtm_g252d_uninit(struct dvb_frontend* fe);


#define G252D_DBG   printk


#endif  // __LG_G252D_H__

