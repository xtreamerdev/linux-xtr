#ifndef __CEC_H__
#define __CEC_H__

#include <linux/device.h>
#include "cec_debug.h"
#include "cm_buff.h"

extern struct bus_type cec_bus_type;

typedef struct 
{
    unsigned long   id;
    char*           name;
    struct device   dev;
}cec_device;



#define to_cec_device(x)  container_of(x, cec_device, dev) 

static inline void *cec_get_drvdata (cec_device *device)
{
    return dev_get_drvdata(&device->dev);
}
 
static inline void
cec_set_drvdata (cec_device *device, void *data)
{
    struct device *dev = &device->dev;
    dev_set_drvdata(dev, data);
}
 


extern int register_cec_device(cec_device* device);
extern void unregister_cec_device(cec_device* device);



typedef struct 
{
    char*                   name;
    struct device_driver    drv;
    
    int      (*probe)       (cec_device* dev);
    void     (*remove)      (cec_device* dev);
    int      (*xmit)        (cec_device* dev, cm_buff* cmg, unsigned char flags);
    cm_buff* (*read)        (cec_device* dev, unsigned char flags);
    int      (*enable)      (cec_device* dev, unsigned char on_off);
    
    int      (*set_logical_addr)(cec_device* dev, unsigned char log_addr);
            
    int      (*suspend)     (cec_device* dev);
    int      (*resume)      (cec_device* dev);
            
}cec_driver ;


#define to_cec_driver(x)  container_of(x, cec_driver, drv) 

extern int register_cec_driver(cec_driver* driver);
extern void unregister_cec_driver(cec_driver* driver);



#define CEC_WAKEUP_BY_SET_STREAM_PATH       0x01
#define CEC_WAKEUP_BY_PLAY_CMD              0x02
#define CEC_WAKEUP_BY_IMAGE_VIEW_ON         0x04

#define CEC_MSG_SET_STREAM_PATH             0x86
#define CEC_MSG_IMAGE_VIEW_ON               0x04
#define CEC_MSG_TEXT_VIEW_ON                0x0D
#define CEC_MSG_PLAY                        0x41


#endif  //__CEC_CORE_H__

