#ifndef __CEC_DEBUG_H__
#define __CEC_DEBUG_H__


//-- cec debug messages
//#define CONFIG_CEC_DBG
//#define CEC_TX_DBG
//#define CEC_RX_DBG

#ifdef CONFIG_CEC_DBG
#define cec_dbg			printk			
#else
#define cec_dbg(args...)	
#endif


#ifdef CEC_TX_DBG
#define cec_tx_dbg              printk
#else
#define cec_tx_dbg(args...)	
#endif



#ifdef CEC_RX_DBG
#define cec_rx_dbg              printk			
#else
#define cec_rx_dbg(args...)	
#endif



#define cec_info		printk	
#define cec_warning		printk	


#endif  //__CEC_DEBUG_H__



