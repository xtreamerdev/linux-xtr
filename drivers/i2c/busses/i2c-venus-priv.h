#ifndef __I2C_VENUS_PRIV_H__
#define __I2C_VENUS_PRIV_H__

#include <linux/i2c.h>
#include <linux/spinlock.h>

////////////////////////////////////////////////////////////////////
#ifdef CONFIG_MARS_I2C_EN
#define MODLE_NAME             "MARS"
#define MAX_I2C_CNT             2
#else
#define MODLE_NAME             "NEPTUNE/VENUS"
#define MAX_I2C_CNT             1
#endif  

#define VERSION                "2.0a"

#define MINIMUM_DELAY_EN    
#define SPIN_LOCK_PROTECT_EN
#define FIFO_THRESHOLD         4
//#define I2C_PROFILEING_EN         
//#define I2C_TIMEOUT_INTERVAL   20    // (unit : jiffies = 10ms)
#define I2C_TIMEOUT_INTERVAL   100    // (unit : jiffies = 10ms)

#define EVENT_START_XFER       4
#define EVENT_STOP_XFER        5
#define EVENT_ENTER_ISR        6
#define EVENT_EXIT_ISR         7
#define EVENT_EXIT_TIMEOUT     8

////////////////////////////////////////////////////////////////////

typedef enum {
    SPD_MODE_LS = 33,
    SPD_MODE_SS = 100,
    SPD_MODE_FS = 400,
    SPD_MODE_HS = 1000   
}SPD_MODE;



typedef enum {
    ADDR_MODE_7BITS   = 7,
    ADDR_MODE_10BITS  = 10
}ADDR_MODE;

enum {    
    ECMDSPLIT   = 40,       // stop detected during transfer
    ETXABORT    = 41,
    ETIMEOUT    = 42,    
    EILLEGALMSG = 43,       // illegal message
    EADDROVERRANGE = 44,      // invalid Address
};


enum {    
    NON_STOP    = 0,       // stop detected during transfer
    WAIT_STOP   = 1,    
};

    
typedef struct {    
    unsigned char       mode;
        
    #define I2C_IDEL               0
    #define I2C_MASTER_READ        1
    #define I2C_MASTER_WRITE       2
    #define I2C_MASTER_RANDOM_READ 3
    
    unsigned char       flags;
                              
    unsigned char*      tx_buff;
    unsigned short      tx_buff_len;
    unsigned short      tx_len;        
    unsigned char*      rx_buff;
    unsigned short      rx_buff_len;
    unsigned short      rx_len;           
    unsigned long       timeout;            
    int                 ret;        // 0 : on going, >0 : success, <0 : err                        
    unsigned int        tx_abort_source;
    struct completion   complete;
    struct timer_list   timer;        
    
}venus_i2c_xfer;
    

typedef struct venus_i2c_t    venus_i2c;

struct venus_i2c_t
{    
    unsigned int        irq;        
    unsigned char       id;        
    unsigned int        spd;
    unsigned int        tick;
    unsigned short      sar;
    ADDR_MODE           sar_mode;        
    
    unsigned short      tar;
    ADDR_MODE           tar_mode;
    
    struct {
        unsigned long   scl_bit;
        unsigned long   sda_bit;
        struct{
            unsigned long   gpie;   
            unsigned long   gpdato;
            unsigned long   gpdati;
            unsigned long   gpdir;
        }reg;
    }gpio_map;
    
    unsigned char       rx_fifo_depth;
    unsigned char       tx_fifo_depth;
    
    unsigned long       time_stamp;
    
    venus_i2c_xfer      xfer;
    
    spinlock_t          lock;
    
    int (*init)         (venus_i2c* p_this);
    int (*uninit)       (venus_i2c* p_this);
    int (*set_spd)      (venus_i2c* p_this, int KHz);
    int (*set_tar)      (venus_i2c* p_this, unsigned short, ADDR_MODE mode);    
    int (*read)         (venus_i2c* p_this, unsigned char* tx_buf, unsigned short tx_buf_len, unsigned char *rx_buff, unsigned short rx_buf_len);
    int (*write)        (venus_i2c* p_this, unsigned char* tx_buf, unsigned short tx_buf_len, unsigned char wait_stop);    
    int (*dump)         (venus_i2c* p_this);        // for debug
    int (*suspend)      (venus_i2c* p_this);
    int (*resume)       (venus_i2c* p_this);
    unsigned int (*get_tx_abort_reason) (venus_i2c* p_this);
};


venus_i2c*  create_venus_i2c_handle     (unsigned char      id,
                                         unsigned short     sar, 
                                         ADDR_MODE          sar_mode, 
                                         SPD_MODE           spd,
                                         unsigned int       irq);
                                         
void        destroy_venus_i2c_handle    (venus_i2c*         hHandle);



#endif  // __I2C_VENUS_PRIV_H__
