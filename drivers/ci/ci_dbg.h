#ifndef __CI_DBG_H__
#define __CI_DBG_H__

//#define CI_DBG_EN   
//#define CI_FUNCTION_TRACE_EN    
#define CI_RX_DBG_EN
#define CI_TX_DBG_EN

///////////////////////////////////////////////////////////////
#define ci_info(fmt, args...)             printk("[CI] INFO, " fmt, ## args)
#define ci_warning(fmt, args...)          printk("[CI] WARNING, " fmt, ## args)

#ifdef CI_DBG_EN
#define ci_dbg(fmt, args...)              printk("[CI] DBG, " fmt, ## args)
#else
#define ci_dbg(fmt, args...)
#endif

#ifdef CI_FUNCTION_TRACE_EN
#define ci_function_trace()               printk("[CI] FUNC, %s", __FUNCTION__)
#else
#define ci_function_trace()
#endif

/////////////////////////////////////////////////////////////

#ifdef CI_RX_DBG_EN
#define ci_rx_info(fmt, args...)          printk("[CI] RX INFO, " fmt, ## args)
#else
#define ci_rx_info(fmt, args...)
#endif

#define ci_rx_warning(fmt, args...)       printk("[CI] RX WARNING, " fmt, ## args)

/////////////////////////////////////////////////////////////

#ifdef CI_TX_DBG_EN
#define ci_tx_info(fmt, args...)          printk("[CI] TX INFO, " fmt, ## args)
#else
#define ci_tx_info(fmt, args...)
#endif

#define ci_tx_warning(fmt, args...)       printk("[CI] TX WARNING, " fmt, ## args)


#endif  //__CI_DBG_H__
