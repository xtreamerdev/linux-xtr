#ifndef __SCD_H__
#define __SCD_H__

#include <linux/device.h>
#include "scd_debug.h"
#include "scd_atr.h"
#include "scd_buff.h"

extern struct bus_type scd_bus_type;


//-------------------------------------------------------
// device
//-------------------------------------------------------
typedef struct 
{
    unsigned long   id;
    char*           name;
    struct device   dev;
}scd_device;



#define to_scd_device(x)  container_of(x, scd_device, dev) 

static inline void *scd_get_drvdata (scd_device *device)
{
    return dev_get_drvdata(&device->dev);
}
 
static inline void
scd_set_drvdata (scd_device *device, void *data)
{
    struct device *dev = &device->dev;
    dev_set_drvdata(dev, data);
}
 
extern int  register_scd_device(scd_device* device);
extern void unregister_scd_device(scd_device* device);

//-------------------------------------------------------
// driver
//-------------------------------------------------------
typedef struct {
    unsigned long   clk;        // clock in Hz
    unsigned long   etu;        // etu
    unsigned char   parity;     // parity : 0 for no parity, others with parity        
}scd_param;


typedef struct 
{
    char*                       name;
    
    struct device_driver        drv;    
    
    //--------- ops
    
    int     (*probe)            (scd_device*        dev);
    
    void    (*remove)           (scd_device*        dev);    
        
    int     (*enable)           (scd_device*        dev, 
                                 unsigned char      on_off);
                                         
    int     (*set_param)        (scd_device*        dev, 
                                 scd_param*         p_param);
                                 
    int     (*get_param)        (scd_device*        dev, 
                                 scd_param*         p_param);                                 

    //---------         
    int     (*activate)         (scd_device*        dev);

    int     (*deactivate)       (scd_device*        dev);

    int     (*reset)            (scd_device*        dev);
    
    int     (*get_atr)          (scd_device*        dev, 
                                 scd_atr*           p_atr);
    
    int     (*get_card_status)  (scd_device*        dev);
    
    int     (*poll_card_status) (scd_device*        dev);
    
    int     (*xmit)             (scd_device*        dev, 
                                 sc_buff*           p_data);    
    
    sc_buff* (*read)            (scd_device*        dev);   
    //--------- power management ------------
    int     (*suspend)          (scd_device* dev);
    
    int     (*resume)           (scd_device* dev);
    
}scd_driver;


#define to_scd_driver(x)  container_of(x, scd_driver, drv) 

extern int  register_scd_driver(scd_driver* driver);
extern void unregister_scd_driver(scd_driver* driver);

//-------------------Error Code
/* scd error code definition */
typedef enum {
    SC_SUCCESS = 0,
    SC_FAIL,
}SC_RET;

#endif  //__SCD_CORE_H__

