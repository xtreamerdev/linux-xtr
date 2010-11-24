
#ifndef __MICROTUNE_MT2060_H__
#define __MICROTUNE_MT2060_H__


typedef struct MT2060_MODULE_TAG    MT2060_MODULE;
struct MT2060_MODULE_TAG
{
    unsigned char       tuner_addr;
    struct i2c_adapter* i2c_adap; 
    void*               hMT2060;    // handle of  MT2060
    unsigned long       f_in;       // Input (RF) Frequency
    unsigned long       f_IF1;      // IF Frequency
    unsigned long       f_out;      // output IF Frequency
    unsigned long       f_IFBW;     // output IF Frequency
    unsigned long       VGA_Gain;   // output IF Frequency         
   
};

// for dibusb core
int microtune_cofdm_mt2060_init(
    void*                   hFE, 
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap
    );
    
int microtune_cofdm_mt2060_set (void* hFE, void* hFEParam, unsigned char   pllbuf[4]);
void microtune_cofdm_mt2060_uninit(void* hFE);

// for MT2060 API                                
int mt2060_write_reg(void*         hUserData, 
                     unsigned char addr, 
                     unsigned char subAddress, 
                     unsigned char Data);

int mt2060_read_reg(void*          hUserData, 
                    unsigned char  addr, 
                    unsigned char  subAddress, 
                    unsigned char* pData);

#endif  // __MICROTUNE_MT2060_H__

