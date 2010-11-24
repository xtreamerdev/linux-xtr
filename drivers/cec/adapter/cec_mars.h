#ifndef __MARS_CEC_H__
#define __MARS_CEC_H__

#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/completion.h>
#include "../core/cec.h"
#include "../core/cm_buff.h"


#define ID_MARS_CEC_CONTROLLER     0x1283

#define MISC_IRQ        3
#define RX_RING_LENGTH  8
#define TX_RING_LENGTH  8

typedef struct {
    unsigned char       enable : 1;
    unsigned char       state  : 7;    
    cm_buff*            cmb;    
    unsigned long       timeout;    
    struct work_struct  work;        
}mars_cec_xmit;


typedef struct {
    unsigned char       enable : 1;
    unsigned char       state  : 7;
    cm_buff*            cmb;
    struct completion   complete;
    struct work_struct  work;
}mars_cec_rcv;


enum 
{
    IDEL,
    XMIT,        
    RCV    
};

enum 
{    
    RCV_OK,
    RCV_FAIL,    
};

typedef struct 
{
    struct {
        unsigned char   init   : 1;
        unsigned char   enable : 1;   
    }status;
    
    mars_cec_xmit   xmit;    
    mars_cec_rcv    rcv;
    cm_buff_head    tx_queue;    
    cm_buff_head    rx_queue;    
    cm_buff_head    rx_free_queue;                        
    spinlock_t      lock;    

}mars_cec;

#endif //__MARS_CEC_H__
