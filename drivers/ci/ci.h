/*=============================================================
 * Copyright C        Realtek Semiconductor Corporation, 2004 * 
 * All rights reserved.                                       *
 *============================================================*/

/*======================= Description ========================*/ 
/**
 * @file
 * For RTD2880's CI APIs.
 * Generic functions for EN50221 CA interfaces.
 *
 * @Author Moya Yu
 * @date 2004/12/12
 * @version { 1.0 } 
 * @ingroup dr_ci
 *
 */

/*======================= CVS Headers =========================
  $Header: /cvsroot/dtv/linux-2.6/include/rtd2880/ci/ci.h,v 1.1 2005/02/17 12:08:07 ghyu Exp $
  
  $Log: ci.h,v $
  Revision 1.1  2005/02/17 12:08:07  ghyu
  +: For common interface.


 *=============================================================*/
#ifndef __CI_H__
#define __CI_H__ 
/*====================== Include files ========================*/ 

#include <linux/list.h>
#include <linux/dvb/ca.h>
#include "dvb_ringbuffer.h"
#include "dvbdev.h"


/*======================== Constant ===========================*/ 

//---- IO SPACE for EN50221 CAM 
#define CTRLIF_DATA          0       //!< DATA register address of EN50221
#define CTRLIF_COMMAND        1       //!< Command register address of EN50221
#define CTRLIF_STATUS        1       //!< Status register address of EN50221
#define CTRLIF_SIZE_LOW      2       //!< Size Low register address of EN50221
#define CTRLIF_SIZE_HIGH     3       //!< Size High register address of EN50221

#define CMDREG_HC        1            /* Host control */
#define CMDREG_SW        2            /* Size write */
#define CMDREG_SR        4            /* Size read */
#define CMDREG_RS        8            /* Reset interface */
#define CMDREG_FRIE   0x40            /* Enable FR interrupt */
#define CMDREG_DAIE   0x80            /* Enable DA interrupt */
#define IRQEN (CMDREG_DAIE)

#define STATUSREG_RE     1            /* read error */
#define STATUSREG_WE     2            /* write error */
#define STATUSREG_FR  0x40            /* module free */
#define STATUSREG_DA  0x80            /* data available */
#define STATUSREG_TXERR (STATUSREG_RE|STATUSREG_WE)    /* general transfer error */

//-----  Parameters
#define INIT_TIMEOUT_SECS               5
#define HOST_LINK_BUF_SIZE              0x200
#define RX_BUFFER_SIZE                  65535
#define MAX_RX_PACKETS_PER_ITERATION    10


#define DVB_CA_EN50221_POLL_CAM_PRESENT        1
#define DVB_CA_EN50221_POLL_CAM_CHANGED        2
#define DVB_CA_EN50221_POLL_CAM_READY          4

#define DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE      1
#define DVB_CA_EN50221_FLAG_IRQ_FR             2
#define DVB_CA_EN50221_FLAG_IRQ_DA             4


/*======================== Type Defines =======================*/

 
/* Information on a CA slot */
struct dvb_ca_slot 
{    
    int slot_state;                     // current state of the CAM 
    
    #define DVB_CA_SLOTSTATE_NONE           0 
    #define DVB_CA_SLOTSTATE_UNINITIALISED  1       
    #define DVB_CA_SLOTSTATE_RUNNING        2
    #define DVB_CA_SLOTSTATE_INVALID        3       
    #define DVB_CA_SLOTSTATE_WAITREADY      4
    #define DVB_CA_SLOTSTATE_VALIDATE       5
    #define DVB_CA_SLOTSTATE_WAITFR         6
    #define DVB_CA_SLOTSTATE_LINKINIT       7    
    
    atomic_t camchange_count;           // Number of CAMCHANGES that have occurred since last processing 
    
    int camchange_type;                 // Type of last CAMCHANGE 
    
    #define DVB_CA_EN50221_CAMCHANGE_REMOVED       0
    #define DVB_CA_EN50221_CAMCHANGE_INSERTED      1
    
    u32 config_base;                    // base address of CAM config 
    
    u8 config_option;                   // value to write into Config Control register 
    
    u8 da_irq_supported:1;              // if 1, the CAM supports DA IRQs 
    
    int link_buf_size;                  // size of the buffer to use when talking to the CAM
    
    struct rw_semaphore sem;            // semaphore for syncing access to slot structure 
    
    struct dvb_ringbuffer rx_buffer;    // buffer for incoming packets
    
    unsigned long timeout;              // timer used during various states of the slot 
};



/* Private CA-interface information */
struct dvb_ca_private 
{
    
    u32 flags;                          // Flags describing the interface (DVB_CA_FLAG_*) 
    
    unsigned int slot_count;            // number of slots supported by this CA interface
    
    struct dvb_ca_slot *slot_info;      // information on each slot
    
    wait_queue_head_t wait_queue;       // wait queues for read() and write() operations 
    
    pid_t thread_pid;                   // PID of the monitoring thread
    
    wait_queue_head_t thread_queue;     // Wait queue used when shutting thread down 
    
    int exit:1;                         // Flag indicating when thread should exit 
    
    int open:1;                         // Flag indicating if the CA device is open 
    
    int wakeup:1;                       // Flag indicating the thread should wake up now 
    
    unsigned long delay;                // Delay the main thread should use
    
    int next_read_slot;                 // Slot to start looking for data to read from in the next user-space read operation 
};



/** Structure describing a CA interface 
 * @ingroup dr_ci 
 */
struct dvb_ca_en50221 {

    /* the module owning this structure */
    struct module* owner;

    /* NOTE: the read_*, write_* and poll_slot_status functions must use locks as
     * they may be called from several threads at once */

    /* functions for accessing attribute memory on the CAM */
    int (*read_attribute_mem)(struct dvb_ca_en50221* ca, int slot, int address);
    int (*write_attribute_mem)(struct dvb_ca_en50221* ca, int slot, int address, u8 value);

    /* functions for accessing the control interface on the CAM */
    int (*read_cam_control)(struct dvb_ca_en50221* ca, int slot, u8 address);
    int (*write_cam_control)(struct dvb_ca_en50221* ca, int slot, u8 address, u8 value);

    /* Functions for controlling slots */
    int (*slot_reset)(struct dvb_ca_en50221* ca, int slot);
    int (*slot_shutdown)(struct dvb_ca_en50221* ca, int slot);
    int (*slot_ts_enable)(struct dvb_ca_en50221* ca, int slot);

    /*
    * Poll slot status.
    * Only necessary if DVB_CA_FLAG_EN50221_IRQ_CAMCHANGE is not set
    */
    int (*poll_slot_status)(struct dvb_ca_en50221* ca, int slot, int open);

    /* private data, used by caller */
    void* data;

    /* Opaque data used by the dvb_ca core. Do not modify! */
    void* private;
};

#endif      // __CI_H__
