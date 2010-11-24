#ifndef __SCD_MARS_PRIV_H__
#define __SCD_MARS_PRIV_H__

#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/completion.h>
#include "../core/scd.h"
#include "../core/scd_atr.h"

#define ID_MARS_SCD(x)          (0x12830000 | (x & 0x3))
#define MARS_SCD_CHANNEL(id)    (id & 0x03)

#define MISC_IRQ        3
#define RX_RING_LENGTH  32
#define TX_RING_LENGTH  32


typedef enum {    
    IFD_FSM_UNKNOWN,
    IFD_FSM_DISABLE,        // function disabled
	IFD_FSM_IDEL,	        // function enabled but not reset the card yet
	IFD_FSM_RESET,          // reset the card and waiting for ATR
	IFD_FSM_ACTIVE,         // card activated
}IFD_FSM; 





typedef struct mars_scd_tag   mars_scd;

struct mars_scd_tag
{    
    unsigned char       id;         // channel id    	
	IFD_FSM             fsm;        // finate state machine of ifd
	
	// config
	unsigned char       clock_div;          // clock divider
	unsigned char       pre_clock_div;      // pre_clock divider
	unsigned long       baud_div1; 
	unsigned long       baud_div2;    
	unsigned char       parity;
			
	// atr              
	scd_atr             atr;               // current atr
	scd_atr_info        atr_info;          // current atr
	unsigned long       atr_timeout;
	
    struct completion   card_detect_completion;
    
    struct timer_list   timer;
	
	struct completion  m_card_status_change;    
	
	int (*enable)           (mars_scd* p_this, unsigned char on_off);    	
	int (*set_clock)        (mars_scd* p_this, unsigned long clock);
	int (*set_etu)          (mars_scd* p_this, unsigned long etu);
	int (*set_parity)       (mars_scd* p_this, unsigned char parity);
	
    int (*get_clock)        (mars_scd* p_this, unsigned long* p_clock);
	int (*get_etu)          (mars_scd* p_this, unsigned long* p_etu);
	int (*get_parity)       (mars_scd* p_this, unsigned char* p_parity);
	
	int (*activate)         (mars_scd* p_this);	        
	int (*deactivate)       (mars_scd* p_this);	        
	int (*reset)            (mars_scd* p_this);	        
    int (*get_atr)          (mars_scd* p_this, scd_atr* p_atr);    
    int (*get_card_status)  (mars_scd* p_this);
    int (*poll_card_status) (mars_scd* p_this);
    int (*xmit)             (mars_scd* p_this, sc_buff* p_data);    		    
    sc_buff* (*read)        (mars_scd* p_this);
};


int mars_scd_card_detect(mars_scd* p_this);

extern mars_scd* mars_scd_open(unsigned char id);
extern void mars_scd_close(mars_scd* p_this);

#endif //__SCD_MARS_PRIV_H__
