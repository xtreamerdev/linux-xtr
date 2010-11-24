#ifndef __DEMOD_RTD2820_H__
#define __DEMOD_RTD2820_H__
 
#define RT2820_REG_ACCESS_FAIL              (-1)
#define RT2820_FUNCTION_OK                  (0)
#define RT2820_FUNCTION_FAIL                (-2)


#define RT2820_INIT_RDY                     (0x01)
#define RT2820_SPEC_INV                     (0x10)

#define RT2820_IF_FREQ_6M                   6000000

#define RT2820_SPEC_INV_ON                  (0x01)
#define RT2820_SPEC_INV_OFF                 (0x00)

//---------------------
typedef struct rtd2820_reg_tag              rtd2820_reg_t;
typedef struct rtd2820_init_table_tag       rtd2820_init_table_t;
typedef struct rtd2820_handle_tag           rtd2820_handle_t;
//---------------------


struct rtd2820_reg_tag
{
    int                 page;
    int                 addr;
    int                 bit_low;
    int                 bit_high;        
};


struct rtd2820_init_table_tag
{
    rtd2820_reg_t       reg;
    unsigned int        val;
};


struct rtd2820_handle_tag
{   
    unsigned char               i2c_addr;
    unsigned char               status;    
    void*                       user_private;           
    unsigned long               IfFreq; 
    
    int (*init)                 (rtd2820_handle_t* hRTD2820);
    int (*soft_reset)           (rtd2820_handle_t* hRTD2820);
    int (*agc_reset)            (rtd2820_handle_t* hRTD2820);
    int (*forward_i2c)          (rtd2820_handle_t* hRTD2820);
    int (*get_system_version)   (rtd2820_handle_t* hRTD2820, unsigned char* p_version);
    int (*get_status)           (rtd2820_handle_t* hRTD2820, unsigned char* p_status);
    int (*get_lock_status)      (rtd2820_handle_t* hRTD2820, unsigned char* p_lock);
    int (*get_signal_strength)  (rtd2820_handle_t* hRTD2820, unsigned short* strength);
    int (*get_snr)              (rtd2820_handle_t* hRTD2820, unsigned short* snr);
    int (*get_ber)              (rtd2820_handle_t* hRTD2820, unsigned short* ber);
    int (*get_carrier_offset)   (rtd2820_handle_t* hRTD2820, int* p_carrier_offset);
    int (*get_if_offset)        (rtd2820_handle_t* hRTD2820, int* p_if_offset);
    int (*set_if)               (rtd2820_handle_t* hRTD2820, unsigned long if_freq);    
    int (*set_spec_inv)         (rtd2820_handle_t* hRTD2820, unsigned char on_off);    
    int (*channel_scan)         (rtd2820_handle_t* hRTD2820);
    
    int (*read_byte)            (rtd2820_handle_t*          hRTD2820, 
                                 unsigned char              page,         
                                 unsigned char              offset, 
                                 unsigned char*             p_val
                                );
    
    int (*write_byte)           (rtd2820_handle_t*          hRTD2820, 
                                 unsigned char              page,         
                                 unsigned char              offset, 
                                 unsigned char              val
                                 );
    
    int (*read_multi_byte)      (rtd2820_handle_t*          hRTD2820,                                 
                                 unsigned char              offset,
                                 unsigned char              page,                                      
                                 int                        n_bytes,
                                 int*                       p_val
                                );

    int (*write_multi_byte)     (rtd2820_handle_t*          hRTD2820,                                 
                                 unsigned char              page,     
                                 unsigned char              offset,
                                 int                        n_bytes,
                                 int                        val
                                );
                                
    int (*read_bits)           (rtd2820_handle_t*           hRTD2820,
                                int                         page,
                                int                         offset,
                                int                         bit_low,
                                int                         bit_high,
                                int*                        p_val
                                );
                                    
    int (*write_bits)          (rtd2820_handle_t*           hRTD2820,
                                int                         page,
                                int                         offset,
                                int                         bit_low,
                                int                         bit_high,
                                int                         val_write
                                );
                                    
    int (*read_register)        (rtd2820_handle_t*          hRTD2820,
                                 rtd2820_reg_t              reg,
                                 int*                       p_val
                                );

    int (*write_register)       (rtd2820_handle_t*          hRTD2820,
                                 rtd2820_reg_t              reg,
                                 int                        val_write
                                );    
                                
    int (*write_table)          (rtd2820_handle_t*          hRTD2820,
                                 unsigned char*             addr, 
                                 unsigned char*             val,
                                 unsigned char              size, 
                                 unsigned char              page
                                );
    
    int (*page_switch)         (rtd2820_handle_t* hRTD2820, unsigned char page );
                                
    int (*ro_strobe)           (rtd2820_handle_t* hRTD2820);
    
    unsigned long              mse_ber;    
};

//----- public function ------//
rtd2820_handle_t* ConstructRTD2820Module(unsigned char i2c_addr, void* user_private);
void              DestructRTD2820Module (rtd2820_handle_t* hRTD2820);




//Realtlek Digital Receiver Test Module

#define ID_RTD2820              0xa2

//Chuck add different definition for digital and analog mode on 20051028
#define MODE_ANALOG             0x0
#define MODE_DIGITAL            0x1

#define ANALOG_FREQ_IF          45750000
#define DIGITAL_FREQ_IF         44000000

#define ATSCFE_MAJOR            254
#define ATSCFE_DESC             "ATSC Frontend RTD2820" 
#define CABFE_MAJOR             199 
#define CABFE_DESC              "Cable Frontend RTD2820 sub" 
#define ANALOG_MAJOR            200 
#define ANALOG_DESC             "Analog Frontend RTD2820" 

#define SRC_TERRESTRIAL         0
#define SRC_CABLE               8

#define RTD2820_PAGE_0          0x00
#define RTD2820_PAGE_1          0x01
#define RTD2820_PAGE_2          0x02
#define RTD2820_PAGE_3          0x03
#define RTD2820_PAGE_4          0x04
#define RTD2820_PAGE_20         0x20
#define RTD2820_PAGE_21         0x21
#define RTD2820_PAGE_22         0x22
#define RTD2820_PAGE_29         0x29
#define RTD2820_PAGE_2F         0x2f
#define RTD2820_PAGE_51         0x51
#define RTD2820_PAGE_NONE       0xFF

//page 0 registers
#define REG_PAGE_CRTL           0x00
#define REG_VERSION             0x01
#define REG_CONFIG_1            0x02
#define REG_CONFIG_2            0x03
#define I2C_REPEATER_ON         0x20
#define REG_CONFIG_3            0x04
#define REG_CONFIG_4            0x05
#define REG_CONFIG_5            0x06
#define REG_CONFIG_6            0x07
#define REG_CONFIG_7            0x08
#define REG_CONFIG_8            0x09

//pgae 1 registers
#define REG_AAGC_CONF_1         0x03
#define REG_AAGC_CONF_6         0x08
#define REG_DDC_CONF_1          0x0f
#define REG_DDC_CONF_2          0x10
#define REG_DDC_CONF_3          0x11
#define SPEC_INV_ON             0x08
#define REG_CRL_CONF_3          0x53
#define REG_CRL_CONF_4          0x54
#define REG_TR_CONF_3           0x6b
#define REG_TR_CONF_4           0x6c
#define REG_TR_CONF_5           0x6d
#define REG_EQ_CONF_50          0xbc
#define EQ_LEAK_ON              0x01
#define REG_EQ_CONF_51          0xbd
#define REG_EQ_CONF_52          0xbe
#define REG_EQ_CONF_53          0xbf

#endif //__DEMOD_RTD2820_H__
