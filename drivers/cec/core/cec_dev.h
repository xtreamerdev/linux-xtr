
#ifndef __CEC_DEV_H__
#define __CEC_DEV_H__

#include <linux/cdev.h>
#include "cm_buff.h"
#include "cec.h"

enum {
    CEC_ENABLE,
    CEC_SET_LOGICAL_ADDRESS,
    CEC_SET_POWER_STATUS,   
    CEC_SEND_MESSAGE,       
    CEC_RCV_MESSAGE,
};

typedef struct {       
    unsigned char*      buf;
    unsigned char       len;    
}cec_msg;


typedef struct{        
    struct cdev     cdev;
    cec_device*     device;
}cec_dev;


extern int  create_cec_dev_node(cec_device* dev);
extern void remove_cec_dev_node(cec_device* dev);


#endif  //__CEC_DEV_H__

