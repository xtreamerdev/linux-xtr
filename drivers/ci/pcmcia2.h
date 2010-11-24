/*=============================================================
 * Copyright (c)      Realtek Semiconductor Corporation, 2005 		*
 * All rights reserved.                                       					*
 *============================================================*/

/*======================= Description ========================*/
/**
 * @file
 * This the PCMCIA driver interface(include Extended Channel) for CA.
 *
 * @author Frank Cheng
 * @date { 2005/9/20 }
 * @version { 1.0 }
 * @ingroup p_pc
 *
 */



/*====================== Include files ========================*/
#include <rtd_types.h>
#include <rtd_2880.h>
#include <rtd_errors.h>
#include <linux/interrupt.h>
#include <asm/irq.h>


/*======================== Constant ===========================*/


/*======================== Type Defines =======================*/
#define PCM_TEST_IRQ	6		//!< The current IRQ Number of PCMCIA interface
#define PCM_DEV_ID		0x5653   //!< Device ID for PCMCIA interface
#define SLOT0           1

/*==================== Function  Prototypes ==================*/
/**
 * Initial the PCMCIA interface of RTD2880.
 * This PCMCIA driver is bound with DVB-CI interrupt handler. It means
 * it only works with CAM module, not for other generic PCMCIA cards.
 *
 *
 * @return ErrCode
 * @retval R_ERR_SUCCESS if successful.
 * @retval R_ERR_FAILED if failed.
 * @retval R_ERR_IRQ_NOT_AVALIABLE if the irq requested is not avaliable.
 * @ingroup p_pc
 */
ErrCode _rtdpc_init(void);

/**
 * This function will reset the PCMCIA controller.
 * This PCMCIA driver is bound with DVB-CI interrupt handler. It means
 * it only works with CAM module, not for other generic PCMCIA cards.
 *
 * @return none
 * @ingroup p_pc
 */
void rtdpc_reset(void);

/*
 * This function will reset the PCMCIA interface.
 * This PCMCIA driver is bound with DVB-CI interrupt handler. It means
 * it only works with CAM module, not for other generic PCMCIA cards.
 *
 * @return none
 * @ingroup p_pc
 */
ErrCode rtdpc_Hardreset(void);

/**
 * Release the PCMCIA interface of RTD2880.
 * It will unregister the interrupt handler of the PCMCIA driver.
 *
 * @return ErrCode
 * @retval R_ERR_SUCCESS if successful.
 * @ingroup p_pc
 */
ErrCode rtdpc_release(void);

/**
 * Read data from PCMCIA's attribute memory of the specified slot.
 * This function will return one byte data from the attribute memory
 * at the address passed in.
 *
 * @param addr: [input] The address you want to read.
 * @param channel: [input] The channel of PCMCIA interface you want to read from,1:Extended Channel;0:data Channel.
 *
 * @return one byte data
 * @ingroup p_pc
 */
UINT8  __inline rtdpc_readAttrMem(UINT32 addr, UINT32 channel);

/**
 * Read data from PCMCIA's IO memory of the specified slot.
 * This function will return one byte data from the IO memory
 * at the address passed in.
 *
 * @param addr: [input] The address you want to read.
 * @param channel: [input] The channel of PCMCIA interface you want to read from,1:Extended Channel;0:data Channel.
 *
 * @return one byte data
 * @ingroup p_pc
 */
UINT8  __inline rtdpc_readIO(UINT32 addr, UINT32 channel);

/**
 * Write data to PCMCIA's attribute memory of the specified slot.
 * This function will return one byte data from the attribute memory
 * at the address passed in.
 *
 * @param addr: [input] The address you want to read.
 * @param data: [input] One byte data you want to write.
 * @param channel: [input] The channel of PCMCIA interface you want to write,1:Extended Channel;0:data Channel.
 *
 * @return ErrCode
 * @retval R_ERR_SUCCESS if successful.
 * @retval R_ERR_FAILED if failed.
 * @ingroup p_pc
 */
ErrCode __inline rtdpc_writeIO(UINT32 addr, UINT8 data, UINT32 channel);

/**
 * Write data to PCMCIA's IO memory of the Status register
 * specified in en50221's spec.
 *
 * @param addr: [input] The address you want to read.
 * @param data: [input] One byte data you want to write.
 * @param channel: [input] The channel of PCMCIA interface you want to write,1:Extended Channel;0:data Channel.
 *
 * @return ErrCode
 * @retval R_ERR_SUCCESS if successful.
 * @retval R_ERR_FAILED if failed.
 * @ingroup p_pc
 */
ErrCode __inline  rtdpc_writeIO_DRFR(UINT32 addr, UINT8 data, UINT32 channel);

/**
 * Write data to PCMCIA's attribute memory of the specified slot.
 * This function will return one byte data from the attribute memory
 * at the address passed in.
 *
 * @param addr: [input] The address you want to read.
 * @param data: [input] One byte data you want to write.
 * @param channel: [input] The channel of PCMCIA interface you want to write,1:Extended Channel;0:data Channel.
 *
 * @return ErrCode
 * @retval R_ERR_SUCCESS if successful.
 * @retval R_ERR_FAILED if failed.
 * @ingroup p_pc
 */
ErrCode __inline  rtdpc_writeAttrMem(UINT32 addr, UINT8 data, UINT32 channel);

/**
 * This function is for internal use only.
 *
 * @param addr: [input] The address you want to read.
 * @param data: [input] The data you want to write
 * @param mode: [input]  Write to attribute memory or IO memory.
 * @param channel: [input] The channel of PCMCIA interface you want to write,1:Extended Channel;0:data Channel.
 *
 * @return one byte data
 * @ingroup p_pc
 */
ErrCode rtdpc_writeMem(UINT32 addr, UINT8 data,
	UINT32 mode, UINT32 slot);

/**
 * This function is for PCMCIA's interrupt handler.
 *
 * @param irq: [input] The irq number.
 * @param dev_id: [input] The device id or data.
 * @param regs: [input]  The structure of registers.
 *
 * @return none
 * @ingroup p_pc
 */
irqreturn_t rtdpc_intHandler(INT32 irq, void *dev_id, struct pt_regs *regs);

/**
 * This is the Common Interface interrupt handler.
 *
 * @param irq: [input] The irq number.
 * @param dev_id: [input] The device id or data.
 * @param regs: [input]  The structure of registers.
 *
 * @return none
 * @ingroup p_pc
 */
void rtdpc_chiIntHandler(unsigned long dev_id);
