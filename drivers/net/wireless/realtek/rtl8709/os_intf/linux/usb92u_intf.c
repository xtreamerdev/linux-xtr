#define _HCI_INTF_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>
#include <hal_init.h>

#ifdef PLATFORM_WINDOWS
#include <usb.h>
#include <usbioctl.h>
#include <usbdlib.h>
#endif

#include <usb_vendor_req.h>
#include <usb_ops.h>
#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_USB_HCI

#error "CONFIG_USB_HCI shall be on!\n"

#endif

#include <wlan_mlme.h>
#include <wlan_sme.h>

#include <rtl8192u_spec.h>
#include <rtl8192u_regdef.h>

#define	RTL8711_VENQT_READ	0xc0
#define	RTL8711_VENQT_WRITE	0x40

u8 read_usb_rxcmd(_adapter * padapter);



/*! \brief Issue an USB Vendor Request for RTL8711. This function can be called under Passive Level only.
	@param bRequest			bRequest of Vendor Request
	@param wValue			wValue of Vendor Request
	@param wIndex			wIndex of Vendor Request
	@param Data				
	@param DataLength
	@param isDirectionIn	Indicates that if this Vendor Request's direction is Device->Host
	
*/
u8 usbvendorrequest(struct dvobj_priv *pdvobjpriv, RT_USB_BREQUEST brequest, RT_USB_WVALUE wvalue, u8 windex, PVOID data, u8 datalen, u8 isdirectionin){

	u8				ret=_TRUE;
#if 0
	int status;
	struct usb_device *udev = pdvobjpriv->pusbdev;//priv->udev;
	_func_enter_;
	

	if (isdirectionin == _TRUE) {
		//get data
		status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       brequest, RTL8711_VENQT_READ,
			       wvalue, windex, data, datalen, HZ / 2);
	} 
	else {	
		//set data
		status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       brequest, RTL8711_VENQT_WRITE,
			       wvalue, windex, data, datalen, HZ / 2);		
	}	

        if (status < 0)
        {
		ret=_FALSE;
             	DEBUG_ERR(("\n usbvendorrequest: fail!!!!\n"));
        }

exit:	
#endif
	_func_exit_;
	return ret;	

}






u8  read_usb_rxcmd_cancel(_adapter * padapter){
#if 0	
	s8			i;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	DEBUG_ERR(("\n ===> read_usb_hisr_cancel(pdev->ts_in_cnt=%d) \n",pdev->ts_in_cnt));

	pdev->ts_in_cnt--;//pdev->tsint_cnt--;  //decrease 1 forInitialize ++ 

	if (pdev->ts_in_cnt) {//if (pdev->tsint_cnt) {
		// Canceling Pending Irp
			if ( pdev->ts_in_pendding == _TRUE) {//if ( pdev->tsint_pendding[pdev->tsint_idx] == _TRUE) {
				DEBUG_ERR(("\n  read_usb_hisr_cancel:sb_kill_urb\n"));
				usb_kill_urb(pdev->ts_in_urb);
			}
		
	DEBUG_ERR(("\n  read_usb_hisr_cancel:cancel ts_in_pendding\n"));

//		while (_TRUE) {
//			if (NdisWaitEvent(&pdev->interrupt_retevt, 2000)) {
//				break;
//			}
//		}

		_down_sema(&(pdev->hisr_retevt));
	DEBUG_ERR(("\n  read_usb_hisr_cancel:down sema\n"));

	}
	DEBUG_ERR(("\n <=== read_usb_hisr_cancel \n"));

#endif
	return _SUCCESS;
}


/* Define element ID of command packet. */
typedef enum tag_Command_Packet_Directories
{    
    RX_TX_FEEDBACK = 0,
    RX_INTERRUPT_STATUS,
    TX_SET_CONFIG,
    BOTH_QUERY_CONFIG,    
    RX_TX_STATUS,
    RX_DBGINFO_FEEDBACK,
    RX_CMD_ELE_MAX
}CMPK_ELEMENT_E;
/*------------------------------Define structure----------------------------*/ 
/* Define different command packet structure. */
/* 1. RX side: TX feedback packet. */
typedef struct tag_Cmd_Pkt_Tx_Feedback
{
	// DWORD 0
	u8	Element_ID;			/* Command packet type. */
	u8	Length;				/* Command packet length. */
	/* 2007/07/05 MH Change tx feedback info field. */
	/*------TX Feedback Info Field */
	u8	TID:4;				/* */	
	u8	Fail_Reason:3;		/* */		
	u8	TOK:1;				/* Transmit ok. */		
	u8	Reserve1:4;			/* */
	u8	Pkt_Type:2;		/* */
	u8	Bandwidth:1;		/* */
	u8	Qos_Pkt:1;			/* */	
	
	// DWORD 1	
	u8	Reserve2;			/* */
	/*------TX Feedback Info Field */
	u8	Retry_Cnt;			/* */
	u16	Pkt_ID;				/* */
	
	// DWORD 3	
	u16	Seq_Num;			/* */
	u8	S_Rate;				/* Start rate. */
	u8	F_Rate;				/* Final rate. */
	
	// DWORD 4	
	u8	S_RTS_Rate;			/* */
	u8	F_RTS_Rate;			/* */
	u16	pkt_length;			/* */
	
	// DWORD 5	
	u32	Reserve3;			/* */	
}CMPK_TXFB_T;

// For USB
/* 2. RX side: Interrupt status packet. It includes Beacon State, 
	  Beacon Timer Interrupt and other useful informations in MAC ISR Reg. */
typedef struct tag_Cmd_Pkt_Interrupt_Status
{
	u8	Element_ID;			/* Command packet type. */
	u8	Length;				/* Command packet length. */
	u16	Reserve;
	u32	InterruptStatus;				/* Interrupt Status. */	
}CMPK_INTR_STA_T;

// For PCI
/* 2. RX side: Beacon state packet. */
typedef struct tag_Cmd_Pkt_Beacon_State
{
	u8	Element_ID;			/* Command packet type. */
	u8	Length;				/* Command packet length. */
	u16	BOK:1;				/* Beacon info. */	
	u16	Reserve:15;			/* Beacon info. */	
}CMPK_BCN_STA_T;

/* 3. TX side: Set configuration packet. */
typedef struct tag_Cmd_Pkt_Set_Configuration
{
	u8	Element_ID;			/* Command packet type. */
	u8	Length;				/* Command packet length. */
	u16	Reserve1;			/* */
	u8 	Cfg_Reserve1:3;
	u8	Cfg_Size:2;			/* Configuration info. */
	u8	Cfg_Type:2;			/* Configuration info. */
	u8	Cfg_Action:1;		/* Configuration info. */
	u8	Cfg_Reserve2;		/* Configuration info. */
	u8	Cfg_Page:4;			/* Configuration info. */
	u8	Cfg_Reserve3:4;		/* Configuration info. */
	u8	Cfg_Offset;			/* Configuration info. */
	u32	Value;				/* */
	u32	Mask;				/* */
}CMPK_SET_CFG_T;

/* 5. Multi packet feedback status. */
typedef struct tag_Tx_Stats_Feedback // PJ quick rxcmd 09042007
{
	// For endian transfer --> Driver will not the same as firmware structure.
	// DW 0	
	u16	Reserve1;			
	u8 	Length;				// Command packet length
	u8 	Element_ID;			// Command packet type
	
	// DW 1	
	u16	TxFail;				// Tx Fail count
	u16 	TxOK;				// Tx ok count
	
	// DW 2
	u16	TxMCOK;  			// tx multicast
	u16 	TxRetry;			// Tx Retry count	
	
	// DW 3
	u16  TxUCOK;				// tx unicast
	u16	TxBCOK;  			// tx broadcast	
	
	// DW 4
	u16	TxBCFail;			//
	u16	TxMCFail;			//	
	
	// DW 5
	u16	Reserve2;			//
	u16	TxUCFail;			//
	
	// DW 6-8
	u32	TxMCLength;	
	u32	TxBCLength;
	u32	TxUCLength;
	
	// DW 9
	u16	Reserve3_23;
	u8	Reserve3_1;
	u8	Rate;
}CMPK_TX_STATUS_T;



/* Different command packet have dedicated message length and definition. */
#define		CMPK_RX_TX_FB_SIZE					sizeof(CMPK_TXFB_T)		//20
#define		CMPK_RX_BEACON_STATE_SIZE			sizeof(CMPK_BCN_STA_T)	//4
#define		CMPK_TX_SET_CONFIG_SIZE				sizeof(CMPK_SET_CFG_T)	//16
#define		CMPK_BOTH_QUERY_CONFIG_SIZE			sizeof(CMPK_SET_CFG_T)	//16
#define		CMPK_RX_TX_STS_SIZE					sizeof(CMPK_TX_STATUS_T)//



#define ISR_TxBcnOk					BIT(27)			// Transmit Beacon OK
#define ISR_TxBcnErr				BIT(26)			// Transmit Beacon Error
#define ISR_BcnTimerIntr			BIT(13)			// Beacon Timer Interrupt

/*-----------------------------------------------------------------------------
 * Function:    cmpk_Handle_Tx_Feedback()
 *
 * Overview:	The function is responsible for extract the message inside TX
 *				feedbck message from firmware. It will contain dedicated info in
 *				ws-06-0063-rtl8190-command-packet-specification. Please 
 *				refer to chapter "TX Feedback Element". We have to read 20 bytes
 *				in the command packet.
 *
 * Input:       PADAPTER	pAdapter
 *				UINT8 		*pMsg		-	Msg Ptr of the command packet.
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 *  When		Who		Remark
 *  01/16/2007	MHC		Create Version 0. 
 *	01/17/2007	MHC		We will not use CountTxStatistics function now. Because
 *						original TCB is released when driver send packet to NIC.
 *						So, we write a function to collect TX feedback info for
 *						TX statistics.
 *	07/11/2007	MHC		Get current operation rate from TX feedback packet.
 *
 *---------------------------------------------------------------------------*/

static	void	
cmpk_Handle_Tx_Feedback(
	_adapter		*pAdapter, 
	u8			*pMsg)
{
#if 0
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	CMPK_TXFB_T		rx_tx_fb;	/* */	
	
	pAdapter->TxStats.NumTxFeedBack++;
	
	/* 0. Display received message. */
	//cmpk_Display_Message(CMPK_RX_TX_FB_SIZE, pMsg);
	
	/* 1. Extract TX feedback info from RFD to temp structure buffer. */
	/* It seems that FW use big endian(MIPS) and DRV use little endian in 
	   windows OS. So we have to read the content byte by byte or transfer 
	   endian type before copy the message copy. */	
	/* 2007/07/05 MH Use pointer to transfer structure memory. */
	//memcpy((UINT8 *)&rx_tx_fb, pMsg, sizeof(CMPK_TXFB_T));
	PlatformMoveMemory((PVOID)&rx_tx_fb, pMsg, sizeof(CMPK_TXFB_T));
	/* 2. Use tx feedback info to count TX statistics. */
	cmpk_CountTxStatistic(pAdapter, &rx_tx_fb);
	/* 2007/01/17 MH Comment previous method for TX statistic function. */
	/* Collect info TX feedback packet to fill TCB. */	
	/* We can not know the packet length and transmit type: broadcast or uni
	   or multicast. */	
	//CountTxStatistics( pAdapter, &tcb );
#endif	
}	/* cmpk_Handle_Tx_Feedback */



/*-----------------------------------------------------------------------------
 * Function:    cmpk_Handle_Interrupt_Status()
 *
 * Overview:    The function is responsible for extract the message from 
 *				firmware. It will contain dedicated info in
 *				ws-07-0063-v06-rtl819x-command-packet-specification-070315.doc.
 * 				Please refer to chapter "Interrupt Status Element".
 *
 * Input:       UINT8 *pMsg		-	Message Pointer of the command packet.
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 *  When			Who			Remark
 *  05/21/2007	Guangan		Add this for rtl8192.  
 *
 *---------------------------------------------------------------------------*/
static	void	
cmpk_Handle_Interrupt_Status(
	_adapter 		*padapter, 
	u8 			*pMsg)
{
	CMPK_INTR_STA_T		rx_intr_status;	/* */	
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	DEBUG_INFO(("cmpk_Handle_Interrupt_Status"));
	rx_intr_status.Length = pMsg[1];
	if (rx_intr_status.Length != (sizeof(CMPK_INTR_STA_T) - 2))
	{
		DEBUG_ERR( ("cmpk_Handle_Interrupt_Status: wrong length!\n") );
		return;
	}

	// Statistics of beacon for ad-hoc mode.
	if (OPMODE & WIFI_ADHOC_STATE)
	{
		//2 maybe need endian transform?
		rx_intr_status.InterruptStatus = *((u32  *)(pMsg + 4));
#if 0
		if (rx_intr_status.InterruptStatus & ISR_TxBcnOk)
		{
			pMgntInfo->bIbssCoordinator = TRUE;
			Adapter->TxStats.NumTxBeaconOk++;	
		}
		else if (rx_intr_status.InterruptStatus & ISR_TxBcnErr)
		{
			pMgntInfo->bIbssCoordinator = FALSE;
			Adapter->TxStats.NumTxBeaconErr++;	
		}
#endif
		if (rx_intr_status.InterruptStatus & ISR_BcnTimerIntr)
		{
			
			issue_beacon(padapter);
			DEBUG_INFO(("==== Issue beacon ===\n"));

		}

	}
	
	 // Other informations in interrupt status we need?
	 
}	/* cmpk_Handle_Interrupt_Status */




/*-----------------------------------------------------------------------------
 * Function:    cmpk_Handle_Query_Config_Rx()
 *
 * Overview:    The function is responsible for extract the message from 
 *				firmware. It will contain dedicated info in
 *				ws-06-0063-rtl8190-command-packet-specification. Please 
 *				refer to chapter "Beacon State Element".
 *
 * Input:       UINT8 *pMsg		-	Message Pointer of the command packet.
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 *  When		Who		Remark
 *  01/19/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
static	void	
cmpk_Handle_Query_Config_Rx(
	_adapter 			*pAdapter, 
	u8 				*pMsg)
{
#if 0
	CMPK_QUERY_CFG_T	rx_query_cfg;	/* */	
	
	/* 0. Display received message. */
	//cmpk_Display_Message(CMPK_RX_BEACON_STATE_SIZE, pMsg);
	
	/* 1. Extract TX feedback info from RFD to temp structure buffer. */
	/* It seems that FW use big endian(MIPS) and DRV use little endian in 
	   windows OS. So we have to read the content byte by byte or transfer 
	   endian type before copy the message copy. */	
	//rx_query_cfg.Element_ID 	= pMsg[0];
	//rx_query_cfg.Length 		= pMsg[1];
	rx_query_cfg.Cfg_Action 	= (pMsg[4] & 0x80000000)>>31;
	rx_query_cfg.Cfg_Type 		= (pMsg[4] & 0x60) >> 5;
	rx_query_cfg.Cfg_Size 		= (pMsg[4] & 0x18) >> 3;
	rx_query_cfg.Cfg_Page 		= (pMsg[6] & 0x0F) >> 0;
	rx_query_cfg.Cfg_Offset 	= pMsg[7];
	rx_query_cfg.Value 			= (pMsg[8] << 24) | (pMsg[9] << 16) | 
								  (pMsg[10] << 8) | (pMsg[11] << 0);
	rx_query_cfg.Mask 			= (pMsg[12] << 24) | (pMsg[13] << 16) | 
								  (pMsg[14] << 8) | (pMsg[15] << 0);
#endif	
}	/* cmpk_Handle_Query_Config_Rx */


/*-----------------------------------------------------------------------------
 * Function:	cmpk_Handle_Tx_Status()
 *
 * Overview:	Firmware add a new tx feedback status to reduce rx command 
 *				packet buffer operation load.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	09/11/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
static	void	
cmpk_Handle_Tx_Status(
	_adapter		*pAdapter, 
	u8			*pMsg)
{	
#if 0
	CMPK_TX_STATUS_T	rx_tx_sts;	/* */
	
	PlatformMoveMemory((PVOID)&rx_tx_sts, pMsg, sizeof(CMPK_TX_STATUS_T));
	/* 2. Use tx feedback info to count TX statistics. */
	cmpk_CountTxStatus(pAdapter, &rx_tx_sts);
#endif
}	/* cmpk_Handle_Tx_Status */


/*-----------------------------------------------------------------------------
 * Function:	DCMD_Save_FW_Dbg_Info()
 *
 * Overview:	Handle dbg message from Firmware. We will sned the info to upper
 *				layer like pomelo or UI.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	08/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
extern s32 DCMD_Save_FW_Dbg_Info(	IN 	ULONG	Length, 
										IN	PVOID	pBuffer)
{
#if 0	
	if (Length > MAX_BUF_SIZE)
	{
		//DbgPrint("\r\nBuffer overflow");
		return NDIS_STATUS_SUCCESS;
	}
    
	dcmd_Buf_Idx = 0;
	/* Move input string to debug command buffer. */
	_memcpy(&dcmd_Return_Buffer[dcmd_Buf_Idx], pBuffer, Length);
	
	/* Increase buffer index. */
	dcmd_Buf_Idx += Length;
	/* Firmware return debug message. */
	dcmd_Finifh_Flag = 1;
#endif	

	return _SUCCESS;
}	/* DCMD_Save_FW_Dbg_Info */


/*! \brief USB TS In IRP Complete Routine.
	@param Context pointer of RT_TSB
*/
static void read_usb_rxcmd_complete(struct urb *purb, struct pt_regs *regs) {

	int status, offset_to_rxcmd, rxcmd_payload_length;
	u8 element_id, cmd_length=0;
	u8 *rxmcd_payload;		
	struct recv_stat *pdesc;
	 _adapter * padapter=(_adapter *)purb->context;
	
	_func_enter_;
	DEBUG_ERR(("\n+read_usb_rxcmd_complete!!\n"));
	if(padapter->bSurpriseRemoved||padapter->bDriverStopped) {

		DEBUG_ERR(("\nread_usb_hisr_complete: Surprised remove(%d) or DriverStopped(%d)!!!\n",padapter->bSurpriseRemoved,padapter->bDriverStopped));
		goto exit;
	}

	switch(purb->status) {
		case 0:
			//success		
			break;
		case -ECONNRESET:
		case -ENOENT:
		case -EPROTO:	
		case -ESHUTDOWN:
			padapter->bDriverStopped=_TRUE;
			DEBUG_ERR(("\n read_usb_hisr_complete: urb shutting down with status:%d \n",purb->status));
			goto exit;
		default:
			DEBUG_ERR(("\n read_usb_hisr_complete: nonzero urb status received: %d\n",purb->status));
			DEBUG_ERR(("\n purb->actual_length=%d\n",purb->actual_length));
			goto resubmit;
	}	
	if(purb->actual_length>16){
		DEBUG_ERR(("RXCMD size >16!!!\n"));
		goto exit;
	}
	else{
		pdesc = (struct recv_stat *)(purb->transfer_buffer);
	
		offset_to_rxcmd  = (sizeof(struct recv_stat) + pdesc->RxDrvInfoSize + (pdesc->RxShift &3) );
		
		rxmcd_payload = (u8*)(purb->transfer_buffer) + offset_to_rxcmd;
		rxcmd_payload_length = pdesc->frame_length;
		
		while (rxcmd_payload_length > 0)
		{
			/* 2007/01/17 MH We support aggregation of different cmd in the same packet. */
			element_id = rxmcd_payload[0];
			
			switch(element_id)
			{
				case	RX_TX_FEEDBACK:				
					cmpk_Handle_Tx_Feedback(padapter, rxmcd_payload);
					cmd_length = CMPK_RX_TX_FB_SIZE;				
					break;
					
				case 	RX_INTERRUPT_STATUS:
					cmpk_Handle_Interrupt_Status(padapter, rxmcd_payload);
					cmd_length = sizeof(CMPK_INTR_STA_T);	
					break;

				case	BOTH_QUERY_CONFIG:
					cmpk_Handle_Query_Config_Rx(padapter, rxmcd_payload);
					cmd_length = CMPK_BOTH_QUERY_CONFIG_SIZE;	
					break;
					
				case	RX_TX_STATUS:
					cmpk_Handle_Tx_Status(padapter, rxmcd_payload);
					cmd_length = CMPK_RX_TX_STS_SIZE;
					break;
					
				case	RX_DBGINFO_FEEDBACK:
					//DbgPrint("\n\r RX debug message from firmware");
					DCMD_Save_FW_Dbg_Info(rxcmd_payload_length, (PVOID)rxmcd_payload);
					break;

				default:
					DEBUG_ERR(("\n\r[CMPK]-->Unknow CMD Element\n"));
					break;
			}
			
			rxcmd_payload_length -= cmd_length;
			rxmcd_payload += cmd_length;		
		}	/* while (rxcmd_payload_length > 0) */
#if 0		
		if(cmd == 0x00) {//beacon interrupt
			//send beacon packet
			//printk("RXCMD: get beacon interrupt \n");
			issue_beacon(padapter);

		} else {//0x01 Tx Close Descriptor
			//read tx close descriptor
			padapter->_sys_mib.CurrRetryCnt += (u16)(desc[0]&0xff);
			DEBUG_ERR(("\n desc[0]=0x%.8x desc[1]=0x%.8x\n",desc[0],desc[1]));
		}
#endif

	}
	
			
	read_usb_rxcmd(padapter);
exit:
	_func_exit_;
	return ;
resubmit:
	
	status=usb_submit_urb(purb,GFP_ATOMIC);
	if(status){
		DEBUG_ERR(("\n read_usb_hisr_complete: can't resubmit intr status :0x%.8x  \n",status));
	}
	//_spinunlock(&(pdev->in_lock));
}

u8 read_usb_rxcmd(_adapter * padapter){
	
	int	status;
	u8 retval=_SUCCESS;

	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	struct usb_device *udev=pdev->pusbdev;
	_func_enter_;

	//_spinlock(&(pdev->in_lock));
	if(padapter->bDriverStopped || padapter->bSurpriseRemoved) {
		DEBUG_ERR(("( padapter->bDriverStopped ||padapter->bSurpriseRemoved )!!!\n"));
		retval= _FAIL;
		goto exit;
	}
	if(pdev->rxcmd_urb==NULL){
		pdev->rxcmd_urb=usb_alloc_urb(0,GFP_ATOMIC);
		if(pdev->rxcmd_urb==NULL){
			retval= _FAIL;
			goto exit;
		}
	}

	DEBUG_INFO(("Submit RX Cmd urb\n"));	
	usb_fill_bulk_urb(	pdev->rxcmd_urb,udev,
						usb_rcvbulkpipe(udev,0x09),
						pdev->rxcmd_urb->transfer_buffer,
						pdev->rxcmd_urb->transfer_buffer_length,
						(usb_complete_t)read_usb_rxcmd_complete,
						padapter);
	
	status=usb_submit_urb(pdev->rxcmd_urb,GFP_ATOMIC);
	if(status){
		DEBUG_ERR(("\n read_usb_rxcmd:submit urb error(status=%d)!!!!!!!\n",status));

	}

exit:
	//_spinunlock(&(pdev->in_lock));
	_func_exit_;

	return retval;

 }
extern NDIS_STATUS usb_dvobj_init(_adapter * padapter){
	NDIS_STATUS	status=_SUCCESS;
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;
	u8 hw_version;
	u32  val32,size;
	_func_enter_;
	pdvobjpriv->padapter=padapter;
	
	DEBUG_ERR(("\nusb_dvobj_init:read hw_version!!\n"));
	hw_version = 0;

#if 0
	hw_version = (read32(padapter, TCR) & TCR_HWVERID_MASK)>>TCR_HWVERID_SHIFT;

	switch (hw_version) {
		case VERSION_819xUsb_A:
			padapter->registrypriv.chip_version=VERSION_819xUsb_A;
			DEBUG_ERR(("\nusb_dvobj_init:padapter->registrypriv.chip_version=%d(VERSION_819xUsb_A)\n",padapter->registrypriv.chip_version));
			
		default:
			padapter->registrypriv.chip_version= VERSION_819xUsb_A;
			break;
	}
	  	
	DEBUG_ERR(("\nusb_dvobj_init:padapter->registrypriv.chip_version=%d\n",padapter->registrypriv.chip_version));
#endif

	/*Read EEprom size */ 
	DEBUG_ERR(("\nusb_dvobj_init:read eeprom Size!!\n"));

	val32 = PlatformEFIORead2Byte(padapter, Cmd9346CR);
	size = (val32 & Cmd9346CR_9356SEL) ? 8 : 6;
	padapter->EepromAddressSize = size;

	DEBUG_ERR(("EEPROM type is %s\n",size==8 ? "93C56" : "93C46"));
	
	DEBUG_ERR(("\nusb_dvobj_init:padapter->EepromAddressSize=%d\n",padapter->EepromAddressSize));
	
	read_eeprom_content(padapter);
	
	//write IDR0~IDR5
//	for(i=0 ; i<6 ; i++)
//		write8(padapter, IDR0+i, padapter->eeprompriv.mac_addr[i]);

	PlatformEFIOWrite4Byte(padapter, IDR0, ((pu4Byte)(padapter->eeprompriv.mac_addr))[0]);
	PlatformEFIOWrite2Byte(padapter, IDR4, ((pu2Byte)(padapter->eeprompriv.mac_addr+4))[0]);


//	for(i=0 ; i<6 ; i++)
	{
		
		DEBUG_ERR(("%8x ",PlatformEFIORead4Byte(padapter, IDR0) ) );
		DEBUG_ERR(("%4x ",PlatformEFIORead2Byte(padapter, IDR4) ));
	}
printk("\n");

#ifdef TODO
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
	INIT_WORK(&_sys_mib->DigWorkItem, (void(*)(void*)) DigWorkItemCallback, padapter);
	INIT_WORK(&_sys_mib->RateAdaptiveWorkItem, (void(*)(void*)) RateAdaptiveWorkItemCallback, padapter);
#else
	INIT_DELAYED_WORK(&_sys_mib->DigWorkItem, DigWorkItemCallback);
	INIT_DELAYED_WORK(&_sys_mib->RateAdaptiveWorkItem, RateAdaptiveWorkItemCallback);
#endif
#endif

	_func_exit_;
	return status;


}
extern void usb_dvobj_deinit(_adapter * padapter){
	
//	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;

	_func_enter_;


	_func_exit_;
}


NDIS_STATUS usb_rx_init(_adapter * padapter){

	u8 val8=_SUCCESS;
	union recv_frame *prxframe;
	NDIS_STATUS 			status=_SUCCESS;
	struct intf_hdl * pintfhdl=&padapter->pio_queue->intf;
	s32 *ptr;
	u8 i;
	struct recv_priv *precvpriv = &(padapter->recvpriv);

	_func_enter_;

	DEBUG_ERR(("+usb_rx_init\n"));

	//issue Rx irp to receive data
	ptr=&padapter->bDriverStopped;
	ptr=&padapter->bSurpriseRemoved;
	
	padapter->recvpriv.rx_pending_cnt=1;
	prxframe = (union recv_frame*)precvpriv->precv_frame_buf;
	for( i=0 ; i<NR_RECVFRAME; i++) {
		DEBUG_INFO(("%d receive frame = %p \n",i,prxframe));
		usb_read_port(pintfhdl,0,0,(unsigned char *) prxframe);
		prxframe++;
	}
	
#if 0	
	//issue rxcmd irp to receive rxcmd( beacon interrupt and tx close descriptor)
	pdev->rxcmd_urb = usb_alloc_urb(0, GFP_KERNEL);
	pdev->rxcmd_buf = kmalloc(16, GFP_KERNEL);

	oldaddr = (u32)pdev->rxcmd_buf;
	align = oldaddr&3;
	if(align != 0 ){
		newaddr = oldaddr + 4 - align;
		pdev->rxcmd_urb->transfer_buffer_length = 16-4+align;
	}
	else{
		newaddr = oldaddr;
		pdev->rxcmd_urb->transfer_buffer_length = 16;
	}
	pdev->rxcmd_urb->transfer_buffer = (u32*)newaddr;

	read_usb_rxcmd(padapter);
#endif
	
	if(val8==_FAIL)
		status=_FAIL;

	DEBUG_ERR(("-usb_rx_init\n"));

	_func_exit_;
	return status;

}

NDIS_STATUS usb_rx_deinit(_adapter * padapter){
#if 0
//	struct dvobj_priv * pdev=&padapter->dvobjpriv;
	DEBUG_ERR(("\n usb_rx_deinit \n"));
	usb_read_port_cancel(padapter);
	read_usb_hisr_cancel(padapter);
//	tsint_cancel(padapter);
//	free_tsin_buf(pdev);
#endif
	return _SUCCESS;
}



#if 0

void get_rx_power_87b(_adapter *padapter, struct recv_stat *desc_87b)
{
	s8 RxPower;
	s8 RX_PWDB;
	u16 Rate;
	u32 SignalStrength = 0;
	u8 bCckRate = _FALSE;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	
	Rate = desc_87b->rxrate;
	RxPower = (s8)(desc_87b->pwdb_g12);
	if( ((Rate <= 22) && (Rate != 12) && (Rate != 18)) || (Rate == 44) )
		bCckRate= _TRUE;

	if(!bCckRate)
	{ // OFDM rate.
		if( RxPower < -106)
			SignalStrength = 0;
		else
			SignalStrength = RxPower + 106;
			RX_PWDB = RxPower/2 -42;
		}
	else
	{ // CCK rate.
		if(desc_87b->agc> 174)
			SignalStrength = 0;
		else
			SignalStrength = 174 - desc_87b->agc;
		RX_PWDB = ((int)(desc_87b->agc)/2)*(-1) - 8;
	}

	if(SignalStrength > 140)
		SignalStrength = 140;
	SignalStrength = (SignalStrength * 100) / 140;
			
	_sys_mib->RecvSignalPower = ( _sys_mib->RecvSignalPower*5 + RX_PWDB-1)/6;

}


void UpdateCCKThreshold(_adapter *padapter)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	// Update CCK Power Detection(0x41) value.
	switch(_sys_mib->StageCCKTh)
	{
		case 0:
			write32(padapter, PhyAddr, 0x010088C1);		mdelay_os(1);
			break;

		case 1:
			write32(padapter, PhyAddr, 0x010098C1);		mdelay_os(1);
			break;

		case 2:
			write32(padapter, PhyAddr, 0x0100C8C1);		mdelay_os(1);
			break;

		case 3:
			write32(padapter, PhyAddr, 0x0100D8C1);		mdelay_os(1);
			break;

		default:
			break;
	}
}

void UpdateInitialGain(_adapter *padapter)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	
	//
	// Note: 
	//      Whenever we update this gain table, we should be careful about those who call it.
	//      Functions which call UpdateInitialGain as follows are important:
	//      (1)StaRateAdaptive87B
	//      (2)DIG_Zebra
	//      (3)ActSetWirelessMode8187 (when the wireless mode is "B" mode, we set the 
	//              OFDM[0x17] = 0x26 to improve the Rx sensitivity).
	//      By Bruce, 2007-06-01.
	//

	//
	// SD3 C.M. Lin Initial Gain Table, by Bruce, 2007-06-01.
	//

	//printk("@@@@ priv->InitialGain = %x\n", priv->InitialGain);

	switch (_sys_mib->InitialGain)
	{
		case 1: //m861dBm
			write32(padapter, PhyAddr, 0x2697);        mdelay_os(1);
			write32(padapter, PhyAddr, 0x86a4);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xfa85);        mdelay_os(1);
			break;

		case 2: //m862dBm
			write32(padapter, PhyAddr, 0x3697);        mdelay_os(1);// Revise 0x26 to 0x36, by Roger, 2007.05.03.
			write32(padapter, PhyAddr, 0x86a4);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xfa85);        mdelay_os(1);
			break;
		
		case 3: //m863dBm
			write32(padapter, PhyAddr, 0x3697);        mdelay_os(1);
			write32(padapter, PhyAddr, 0x86a4);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xfb85);        mdelay_os(1);
			break;

		case 4: //m864dBm
			write32(padapter, PhyAddr, 0x4697);        mdelay_os(1);
			write32(padapter, PhyAddr, 0x86a4);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xfb85);        mdelay_os(1);
			break;
	
		case 5: //m82dBm
			write32(padapter, PhyAddr, 0x4697);        mdelay_os(1);
			write32(padapter, PhyAddr, 0x96a4);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xfb85);        mdelay_os(1);
			break;

		case 6: //m78dBm
			write32(padapter, PhyAddr, 0x5697);        mdelay_os(1);
			write32(padapter, PhyAddr, 0x96a4);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xfc85);        mdelay_os(1);
			break;

		case 7: //m74dBm
			write32(padapter, PhyAddr, 0x5697);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xa6a4);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xfc85);        mdelay_os(1);
			break;

		case 8: 
			write32(padapter, PhyAddr, 0x6697);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xb6a4);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xfc85);        mdelay_os(1);
			break;

		default: //MP
			write32(padapter, PhyAddr, 0x2697);        mdelay_os(1);
			write32(padapter, PhyAddr, 0x86a4);        mdelay_os(1);
			write32(padapter, PhyAddr, 0xfa85);        mdelay_os(1);
			break;

	}

}

void DynamicInitGain(_adapter *padapter)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	u16	CCKFalseAlarm, OFDMFalseAlarm;
	u16	OfdmFA1, OfdmFA2;
	u16	CCK_Up_Th, CCK_Lw_Th;
	int	InitialGainStep = 7; // The number of initial gain stages.
	int	LowestGainStage = 4; // The capable lowest stage of performing dig workitem.
	
	set_current_state(TASK_INTERRUPTIBLE);	
#ifdef TODO
	_sys_mib->FalseAlarmRegValue = read32(padapter, CCK_FALSE_ALARM);
#endif
	
	CCKFalseAlarm = (u16)(_sys_mib->FalseAlarmRegValue & 0x0000ffff);
	OFDMFalseAlarm = (u16)( (_sys_mib->FalseAlarmRegValue>>16) & 0x0000ffff);
	OfdmFA1 = 0x15;
	OfdmFA2 = ((u16)(_sys_mib->RegDigOfdmFaUpTh)) << 8;
	//OfdmFA2 = 0xC00;

	DEBUG_INFO(("\n r8187_dig_thread:FalseAlarmRegValue = %8x \n", _sys_mib->FalseAlarmRegValue));

	// The number of initial gain steps is different, by Bruce, 2007-04-13.		
	if(_sys_mib->InitialGain == 0)  //autoDIG
	{// Advised from SD3 DZ, by Bruce, 2007-06-05.
		_sys_mib->InitialGain = 4; // In 87B, m74dBm means State 4 (m82dBm)
	}

#ifdef TODO
	if(padapter->registrypriv.chip_version != VERSION_8187B_B)
	{ // Advised from SD3 DZ, by Bruce, 2007-06-05.
		OfdmFA1 =  0x20;
	}
#endif

	InitialGainStep = 8;
	LowestGainStage = 1;

	if (OFDMFalseAlarm > OfdmFA1)
	{
		if (OFDMFalseAlarm > OfdmFA2)
		{
			_sys_mib->DIG_NumberFallbackVote++;
			if (_sys_mib->DIG_NumberFallbackVote >1)
			{
                             // serious OFDM  False Alarm, need fallback
                             // By Bruce, 2007-03-29.
                             // if (priv->InitialGain < 7) // In 87B, m66dBm means State 7 (m74dBm)
				if (_sys_mib->InitialGain < InitialGainStep)
				{
					_sys_mib->InitialGain = (_sys_mib->InitialGain + 1);
					UpdateInitialGain(padapter); // 2005.01.06, by rcnjko.
				}
				_sys_mib->DIG_NumberFallbackVote	= 0;
				_sys_mib->DIG_NumberUpgradeVote	= 0;
			}
		}
		else
		{
			if (_sys_mib->DIG_NumberFallbackVote)
				_sys_mib->DIG_NumberFallbackVote--;
		}
		_sys_mib->DIG_NumberUpgradeVote=0;
	}
	else    //OFDM False Alarm < 0x15
	{
		if (_sys_mib->DIG_NumberFallbackVote)
			_sys_mib->DIG_NumberFallbackVote--;
		_sys_mib->DIG_NumberUpgradeVote++;

		if (_sys_mib->DIG_NumberUpgradeVote>9)
		{
			if (_sys_mib->InitialGain > LowestGainStage) // In 87B, m78dBm means State 4 (m864dBm)
			{
				_sys_mib->InitialGain = (_sys_mib->InitialGain - 1);
				UpdateInitialGain(padapter); // 2005.01.06, by rcnjko.
			}
			_sys_mib->DIG_NumberFallbackVote	= 0;
			_sys_mib->DIG_NumberUpgradeVote	= 0;
		}
	}

	// By Bruce, 2007-03-29.
	// Dynamically update CCK Power Detection Threshold.
	CCK_Up_Th = _sys_mib->CCKUpperTh;
	CCK_Lw_Th = _sys_mib->CCKLowerTh;	
	CCKFalseAlarm = (u16)((_sys_mib->FalseAlarmRegValue & 0x0000ffff) >> 8); // We only care about the higher byte.

	if( _sys_mib->StageCCKTh < 3 && CCKFalseAlarm >= CCK_Up_Th)
	{
		_sys_mib->StageCCKTh ++;
		UpdateCCKThreshold(padapter);
	}
	else if(_sys_mib->StageCCKTh > 0 && CCKFalseAlarm <= CCK_Lw_Th)
	{
		_sys_mib->StageCCKTh --;
		UpdateCCKThreshold(padapter);
	}	
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
void DigWorkItemCallback(_adapter *padapter)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	DynamicInitGain(padapter);

	schedule_delayed_work(&_sys_mib->DigWorkItem, HZ*2);
}
#else
void DigWorkItemCallback(struct work_struct *work)
{
	struct mib_info *_sys_mib = container_of(work, struct mib_info, DigWorkItem.work);
	_adapter *padapter = _sys_mib->padapter;
	
	DynamicInitGain(padapter);

	schedule_delayed_work(&_sys_mib->DigWorkItem, HZ*2);
}
#endif

void r8187_start_DynamicInitGain(_adapter *padapter)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	_sys_mib->bDynamicInitGain = _TRUE;
	schedule_delayed_work(&_sys_mib->DigWorkItem, HZ*2);
}

void r8187_stop_DynamicInitGain(_adapter *padapter)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	if(_sys_mib->bDynamicInitGain){
		cancel_rearming_delayed_work(&_sys_mib->DigWorkItem);
		_sys_mib->bDynamicInitGain= _FALSE;
	}
	else flush_scheduled_work();
}


//
//	Helper function to determine if specified data rate is 
//	CCK rate.
//	2005.01.25, by rcnjko.
//
u8 MgntIsCckRate(u8 rate)
{
	u8 bReturn = _FALSE;

	if((rate <= 22) && (rate != 12) && (rate != 18))
	{
		bReturn = _TRUE;
	}

	return bReturn;
}

//
// Return BOOLEAN: TxRate is included in SupportedRates or not.
// Added by Annie, 2006-05-16, in SGS, for Cisco B mode AP. (with ERPInfoIE in its beacon.)
//
u8 IncludedInSupportedRates(_adapter *padapter, u8 TxRate)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	u8	idx;
	u8	RateMask = 0x7F;
	u8	Found = _FALSE;
	u8	NaiveTxRate = TxRate&RateMask;
	u8	*pSupRateSet = _sys_mib->cur_network.SupportedRates;
	u8	Length = get_rateset_len(pSupRateSet);

	for( idx=0; idx<Length; idx++ )
	{
		if( (pSupRateSet[idx] & RateMask) == NaiveTxRate )
		{
			Found = _TRUE;
			break;
		}
	}

	return Found;
}

//
//	Description:
//		Get the Tx rate one degree up form the input rate in the supported rates.
//		Return the upgrade rate if it is successed, otherwise return the input rate.
//	By Bruce, 2007-06-05.
// 
u16 GetUpgradeTxRate(_adapter *padapter , u8 rate)
{
	u8			UpRate;

	// Upgrade 1 degree.
	switch(rate)
	{	
	case 108: // Up to 54Mbps.
		UpRate = 108;
		break;
	
	case 96: // Up to 54Mbps.
		UpRate = 108;
		break;

	case 72: // Up to 48Mbps.
		UpRate = 96;
		break;

	case 48: // Up to 36Mbps.
		UpRate = 72;
		break;

	case 36: // Up to 24Mbps.
		UpRate = 48;
		break;

	case 22: // Up to 18Mbps.
		UpRate = 36;
		break;

	case 11: // Up to 11Mbps.
		UpRate = 22;
		break;

	case 4: // Up to 5.5Mbps.
		UpRate = 11;
		break;

	case 2: // Up to 2Mbps.
		UpRate = 4;
		break;

	default:
		DEBUG_ERR(("GetUpgradeTxRate(): Input Tx Rate(%d) is undefined!\n", rate));
		return rate;		
	}

	// Check if the rate is valid.
	if(IncludedInSupportedRates(padapter, UpRate))
	{
		DEBUG_INFO(("GetUpgradeTxRate(): GetUpgrade Tx rate(%d) from %d !\n", UpRate, (padapter->_sys_mib.rate/10) * 2));
		return UpRate;
	}
	else
	{
		DEBUG_ERR(("GetUpgradeTxRate(): Tx rate (%d) is not in supported rates\n", UpRate));		
		return rate;
	}
	return rate;
}

//
//	Description:
//		Get the Tx rate one degree down form the input rate in the supported rates.
//		Return the degrade rate if it is successed, otherwise return the input rate.
//	By Bruce, 2007-06-05.
// 
u16 GetDegradeTxRate(_adapter *padapter , u8 rate)
{
	u8			DownRate;

	// Upgrade 1 degree.
	switch(rate)
	{			
	case 108: // Down to 48Mbps.
		DownRate = 96;
		break;

	case 96: // Down to 36Mbps.
		DownRate = 72;
		break;

	case 72: // Down to 24Mbps.
		DownRate = 48;
		break;

	case 48: // Down to 18Mbps.
		DownRate = 36;
		break;

	case 36: // Down to 11Mbps.
		DownRate = 22;
		break;

	case 22: // Down to 5.5Mbps.
		DownRate = 11;
		break;

	case 11: // Down to 2Mbps.
		DownRate = 4;
		break;

	case 4: // Down to 1Mbps.
		DownRate = 2;
		break;	

	case 2: // Down to 1Mbps.
		DownRate = 2;
		break;	
		
	default:
		DEBUG_ERR(("GetDegradeTxRate(): Input Tx Rate(%d) is undefined!\n", rate));
		return rate;		
	}

	// Check if the rate is valid.
	if(IncludedInSupportedRates(padapter, DownRate))
	{
		DEBUG_INFO(("GetDegradeTxRate(): GetDegrade Tx rate(%d) from %d!\n", DownRate, (padapter->_sys_mib.rate/10) * 2));
		return DownRate;
	}
	else
	{
		DEBUG_ERR(("GetDegradeTxRate(): Tx rate (%d) is not in supported rates\n", DownRate));		
		return rate;
	}
	return rate;
}

void StaRateAdaptive87B(_adapter *padapter )
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	u64			CurrTxokCnt;
	u16			CurrRetryCnt;
	u16			CurrRetryRate;
	u8			CurrentOperaRate;
	u64			CurrRxokCnt;
	u8			bTryUp = _FALSE;
	u8			bTryDown = _FALSE;
	u8			TryUpTh = 1;
	u8			TryDownTh = 2;
	u32			TxThroughput;
	s32			CurrSignalStrength;
	u8			bUpdateInitialGain = _FALSE;

	CurrRetryCnt	= _sys_mib->CurrRetryCnt;
	CurrTxokCnt	= _sys_mib->NumTxOkTotal - _sys_mib->LastTxokCnt;
	CurrRxokCnt	= _sys_mib->NumRxOkTotal - _sys_mib->LastRxokCnt;
	CurrSignalStrength = _sys_mib->RecvSignalPower;
	TxThroughput = (u32)(_sys_mib->NumTxOkBytesTotal - _sys_mib->LastTxOKBytes);
	_sys_mib->LastTxOKBytes = _sys_mib->NumTxOkBytesTotal;
	CurrentOperaRate = (u8)((_sys_mib->rate/10) * 2);
	DEBUG_INFO(("CurrentOperaRate = %d \n", CurrentOperaRate));

	//2 Compute retry ratio.
	if (CurrTxokCnt>0)
	{
		CurrRetryRate = (u16)((int)CurrRetryCnt*100/(int)CurrTxokCnt);
	}
	else
	{ // It may be serious retry. To distinguish serious retry or no packets modified by Bruce
		CurrRetryRate = (u16)((int)CurrRetryCnt*100/1);
	}

	//
	// For debug information.
	//
	DEBUG_INFO(("\n(1) LastRetryRate: %d \n",(int)_sys_mib->LastRetryRate));
	DEBUG_INFO(("(2) RetryCnt = %d  \n", (int)CurrRetryCnt));
	DEBUG_INFO(("(3) TxokCnt = %d \n", (int)CurrTxokCnt));
	DEBUG_INFO(("(4) CurrRetryRate = %d \n", (int)CurrRetryRate));
	DEBUG_INFO(("(5) SignalStrength = %d \n",(int)_sys_mib->RecvSignalPower));
	DEBUG_INFO(("(6) ReceiveSignalPower = %d \n",(int)CurrSignalStrength));

	_sys_mib->LastRetryCnt	= _sys_mib->CurrRetryCnt;
	_sys_mib->LastTxokCnt	= _sys_mib->NumTxOkTotal;
	_sys_mib->LastRxokCnt	= _sys_mib->NumRxOkTotal;
	_sys_mib->CurrRetryCnt	= 0;

	//2No Tx packets, return to init_rate or not?
	if (CurrRetryRate==0 && CurrTxokCnt == 0)
	{	
		//
		// 2007.04.09, by Roger.
		//After 4.5 seconds in this condition, we try to raise rate.
		//
		_sys_mib->TryupingCountNoData++;		
		
		if (_sys_mib->TryupingCountNoData>15)
		{
			_sys_mib->TryupingCountNoData = 0;
		 	CurrentOperaRate = GetUpgradeTxRate(padapter, CurrentOperaRate);
			// Reset Fail Record
			_sys_mib->LastFailTxRate = 0;
			_sys_mib->LastFailTxRateSS = -200;
			_sys_mib->FailTxRateCount = 0;
		}
		goto SetInitialGain;
	}
        else
	{
		_sys_mib->TryupingCountNoData=0; //Reset trying up times.
	}

	//
	// Restructure rate adaptive as the following main stages:
	// (1) Add retry threshold in 54M upgrading condition with signal strength.
	// (2) Add the mechanism to degrade to CCK rate according to signal strength 
	//		and retry rate.
	// (3) Remove all Initial Gain Updates over OFDM rate. To avoid the complicated 
	//		situation, Initial Gain Update is upon on DIG mechanism except CCK rate.
	// (4) Add the mehanism of trying to upgrade tx rate.
	// (5) Record the information of upping tx rate to avoid trying upping tx rate constantly.
	// By Bruce, 2007-06-05.
	//	
	//

	// 11Mbps or 36Mbps
	// Check more times in these rate(key rates).
	//
	if(CurrentOperaRate == 22 || CurrentOperaRate == 72)
	{ 
		TryUpTh += 9;
	}
	//
	// Let these rates down more difficult.
	//
	if(MgntIsCckRate(CurrentOperaRate) || CurrentOperaRate == 36)
	{
			TryDownTh += 1;
	}
	//1 Adjust Rate.
	if (_sys_mib->bTryuping == _TRUE)
	{	
		//2 For Test Upgrading mechanism
		// Note:
		// 	Sometimes the throughput is upon on the capability bwtween the AP and NIC,
		// 	thus the low data rate does not improve the performance.
		// 	We randomly upgrade the data rate and check if the retry rate is improved.
		
		// Upgrading rate did not improve the retry rate, fallback to the original rate.
		if ( (CurrRetryRate > 25) && TxThroughput < _sys_mib->LastTxThroughput )
		{
			//Not necessary raising rate, fall back rate.
			bTryDown = _TRUE;
		}
		else
		{
			_sys_mib->bTryuping = _FALSE;
		}
	}	
	else if (CurrSignalStrength > -51 && (CurrRetryRate < 100))
	{
		//2For High Power
		//
		// Added by Roger, 2007.04.09.
		// Return to highest data rate, if signal strength is good enough.
		// SignalStrength threshold(-50dbm) is for RTL8186.
		// Revise SignalStrength threshold to -51dbm.	
		//
		// Also need to check retry rate for safety, by Bruce, 2007-06-05.
		if(CurrentOperaRate != 108)
		{
			bTryUp = _TRUE;
			// Upgrade Tx Rate directly.
			_sys_mib->TryupingCount += TryUpTh;			
		}
	}
	else if(CurrTxokCnt< 100 && CurrRetryRate >= 600) 
	{
		//2 For Serious Retry
		//
		// Traffic is not busy but our Tx retry is serious. 
		//
		bTryDown = _TRUE;
		// Let Rate Mechanism to degrade tx rate directly.
		_sys_mib->TryDownCountLowData += TryDownTh;
		DEBUG_INFO(("RA: Tx Retry is serious. Degrade Tx Rate to %d directly...\n", CurrentOperaRate));
	}
	else if ( CurrentOperaRate == 108 )
	{
		//2For 54Mbps
		// if ( (CurrRetryRate>38)&&(pHalData->LastRetryRate>35)) 
		if ( (CurrRetryRate>33)&&(_sys_mib->LastRetryRate>32))
		{
			//(30,25) for cable link threshold. (38,35) for air link.
			//Down to rate 48Mbps.
			bTryDown = _TRUE;
		}
	}
	else if ( CurrentOperaRate == 96 )
	{
		//2For 48Mbps
		// if ( ((CurrRetryRate>73) && (pHalData->LastRetryRate>72)) && IncludedInSupportedRates(Adapter, 72) )
		if ( ((CurrRetryRate>48) && (_sys_mib->LastRetryRate>47)))
		{	
			//(73, 72) for temp used.
			//(25, 23) for cable link, (60,59) for air link.
			//CurrRetryRate plus 25 and 26 respectively for air link.
			//Down to rate 36Mbps.
			bTryDown = _TRUE;
		}
		else if ( (CurrRetryRate<8) && (_sys_mib->LastRetryRate<8) ) //TO DO: need to consider (RSSI)
		{
			bTryUp = _TRUE;
		}
	}
	else if ( CurrentOperaRate == 72 )
	{
		//2For 36Mbps
		//if ( (CurrRetryRate>97) && (pHalData->LastRetryRate>97)) 
		if ( (CurrRetryRate>55) && (_sys_mib->LastRetryRate>54)) 
		{	
			//(30,25) for cable link threshold respectively. (103,10) for air link respectively.
			//CurrRetryRate plus 65 and 69 respectively for air link threshold.
			//Down to rate 24Mbps.
			bTryDown = _TRUE;
		}
		// else if ( (CurrRetryRate<20) &&  (pHalData->LastRetryRate<20) && IncludedInSupportedRates(Adapter, 96) )//&& (device->LastRetryRate<15) ) //TO DO: need to consider (RSSI)
		else if ( (CurrRetryRate<15) &&  (_sys_mib->LastRetryRate<16))//&& (device->LastRetryRate<15) ) //TO DO: need to consider (RSSI)
		{
			bTryUp = _TRUE;
		}
	}
	else if ( CurrentOperaRate == 48 )
	{
		//2For 24Mbps
		// if ( ((CurrRetryRate>119) && (pHalData->LastRetryRate>119) && IncludedInSupportedRates(Adapter, 36)))
		if ( ((CurrRetryRate>63) && (_sys_mib->LastRetryRate>62)))
		{	
			//(15,15) for cable link threshold respectively. (119, 119) for air link threshold.
			//Plus 84 for air link threshold.
			//Down to rate 18Mbps.
			bTryDown = _TRUE;
		}
  		// else if ( (CurrRetryRate<14) && (pHalData->LastRetryRate<15) && IncludedInSupportedRates(Adapter, 72)) //TO DO: need to consider (RSSI)
  		else if ( (CurrRetryRate<20) && (_sys_mib->LastRetryRate<21)) //TO DO: need to consider (RSSI)
		{	
			bTryUp = _TRUE;	
		}
	}
	else if ( CurrentOperaRate == 36 )
	{
		//2For 18Mbps
		if ( ((CurrRetryRate>109) && (_sys_mib->LastRetryRate>109)))
		{
			//(99,99) for cable link, (109,109) for air link.
			//Down to rate 11Mbps.
			bTryDown = _TRUE;
		}
		// else if ( (CurrRetryRate<15) && (pHalData->LastRetryRate<16) && IncludedInSupportedRates(Adapter, 48)) //TO DO: need to consider (RSSI)	
		else if ( (CurrRetryRate<25) && (_sys_mib->LastRetryRate<26)) //TO DO: need to consider (RSSI)
		{	
			bTryUp = _TRUE;	
		}
	}
	else if ( CurrentOperaRate == 22 )
	{
		//2For 11Mbps
		// if (CurrRetryRate>299 && IncludedInSupportedRates(Adapter, 11))
		if (CurrRetryRate>95)
		{
			bTryDown = _TRUE;
		}
		else if (CurrRetryRate<55)//&& (device->LastRetryRate<55) ) //TO DO: need to consider (RSSI)
		{
			bTryUp = _TRUE;
		}
	}	
	else if ( CurrentOperaRate == 11 )
	{
		//2For 5.5Mbps
		// if (CurrRetryRate>159 && IncludedInSupportedRates(Adapter, 4) ) 
		if (CurrRetryRate>149) 
		{	
			bTryDown = _TRUE;			
		}
		// else if ( (CurrRetryRate<30) && (pHalData->LastRetryRate<30) && IncludedInSupportedRates(Adapter, 22) )
		else if ( (CurrRetryRate<60) && (_sys_mib->LastRetryRate < 65))
		{
			bTryUp = _TRUE;
		}		
	}
	else if ( CurrentOperaRate == 4 )
	{
		//2For 2 Mbps
		if((CurrRetryRate>99) && (_sys_mib->LastRetryRate>99))
		{
			bTryDown = _TRUE;			
	}
		// else if ( (CurrRetryRate<50) && (pHalData->LastRetryRate<65) && IncludedInSupportedRates(Adapter, 11) )
		else if ( (CurrRetryRate < 65) && (_sys_mib->LastRetryRate < 70))
	{
			bTryUp = _TRUE;
		}
	}
	else if ( CurrentOperaRate == 2 )
		{
		//2For 1 Mbps
		// if ( (CurrRetryRate<50) && (pHalData->LastRetryRate<65) && IncludedInSupportedRates(Adapter, 4))
		if ( (CurrRetryRate<70) && (_sys_mib->LastRetryRate<75))
			{
			bTryUp = _TRUE;
		}
	}

	//1 Test Upgrading Tx Rate
	// Sometimes the cause of the low throughput (high retry rate) is the compatibility between the AP and NIC.
	// To test if the upper rate may cause lower retry rate, this mechanism randomly occurs to test upgrading tx rate.
	if(!bTryUp && !bTryDown && (_sys_mib->TryupingCount == 0) && (_sys_mib->TryDownCountLowData == 0)
		&& CurrentOperaRate != 108 && _sys_mib->FailTxRateCount < 2)
	{
		if(jiffies% (CurrRetryRate + 101) == 0)
		{
			bTryUp = _TRUE;	
			_sys_mib->bTryuping = _TRUE;
			DEBUG_INFO(("StaRateAdaptive87B(): Randomly try upgrading...\n"));
		}
	}

	//1 Rate Mechanism
	if(bTryUp)
	{
		_sys_mib->TryupingCount++;
		_sys_mib->TryDownCountLowData = 0;
			
		//
		// Check more times if we need to upgrade indeed.
		// Because the largest value of pHalData->TryupingCount is 0xFFFF and 
		// the largest value of pHalData->FailTxRateCount is 0x14,
		// this condition will be satisfied at most every 2 min.
		//
		if((_sys_mib->TryupingCount > (TryUpTh + _sys_mib->FailTxRateCount * _sys_mib->FailTxRateCount)) ||
			(CurrSignalStrength > _sys_mib->LastFailTxRateSS) || _sys_mib->bTryuping)
		{
			_sys_mib->TryupingCount = 0;
			// 
			// When transfering from CCK to OFDM, DIG is an important issue.
			//
			if(CurrentOperaRate == 22)
				bUpdateInitialGain = _TRUE;
			// (1)To avoid upgrade frequently to the fail tx rate, add the FailTxRateCount into the threshold.
			// (2)If the signal strength is increased, it may be able to upgrade.
			CurrentOperaRate = GetUpgradeTxRate(padapter, CurrentOperaRate);
			DEBUG_INFO(("StaRateAdaptive87B(): Upgrade Tx Rate to %d\n", CurrentOperaRate));
				
			// Update Fail Tx rate and count.
			if(_sys_mib->LastFailTxRate != CurrentOperaRate)
			{
				_sys_mib->LastFailTxRate = CurrentOperaRate;
				_sys_mib->FailTxRateCount = 0;
				_sys_mib->LastFailTxRateSS = -200; // Set lowest power.
			}
		}
	}
	else
	{	
		if(_sys_mib->TryupingCount > 0)
			_sys_mib->TryupingCount --;
	}
	
	if(bTryDown)
	{
		_sys_mib->TryDownCountLowData++;
		_sys_mib->TryupingCount = 0;
				
		
		//Check if Tx rate can be degraded or Test trying upgrading should fallback.
		if(_sys_mib->TryDownCountLowData > TryDownTh || _sys_mib->bTryuping)
		{
			_sys_mib->TryDownCountLowData = 0;
			_sys_mib->bTryuping = _FALSE;
			// Update fail information.
			if(_sys_mib->LastFailTxRate == CurrentOperaRate)
			{
				_sys_mib->FailTxRateCount ++;
				// Record the Tx fail rate signal strength.
				if(CurrSignalStrength > _sys_mib->LastFailTxRateSS)
				{
					_sys_mib->LastFailTxRateSS = CurrSignalStrength;
				}
			}
			else
			{
				_sys_mib->LastFailTxRate = CurrentOperaRate;
				_sys_mib->FailTxRateCount = 1;
				_sys_mib->LastFailTxRateSS = CurrSignalStrength;
			}
			CurrentOperaRate = GetDegradeTxRate(padapter, CurrentOperaRate);
			//
			// When it is CCK rate, it may need to update initial gain to receive lower power packets.
			//
			if(MgntIsCckRate(CurrentOperaRate))
			{
				bUpdateInitialGain = _TRUE;
			}
			DEBUG_INFO(("StaRateAdaptive87B(): Degrade Tx Rate to %d\n", CurrentOperaRate));
		}
	}
	else
	{
		if(_sys_mib->TryDownCountLowData > 0)
			_sys_mib->TryDownCountLowData --;
	}
			
	// Keep the Tx fail rate count to equal to 0x15 at most.
	// Reduce the fail count at least to 10 sec if tx rate is tending stable.
	if(_sys_mib->FailTxRateCount >= 0x15 || 
		(!bTryUp && !bTryDown && _sys_mib->TryDownCountLowData == 0 && _sys_mib->TryupingCount && _sys_mib->FailTxRateCount > 0x6))
	{
		_sys_mib->FailTxRateCount --;
	}

	//
	// We need update initial gain when we set tx rate "from OFDM to CCK" or
	// "from CCK to OFDM". 
	//	
SetInitialGain:
	if(bUpdateInitialGain)
	{
		if(MgntIsCckRate(CurrentOperaRate)) // CCK
		{
			if(_sys_mib->InitialGain > _sys_mib->RegBModeGainStage)
			{
				if(CurrSignalStrength < -85) // Low power, OFDM [0x17] = 26.
				{
					_sys_mib->InitialGain = _sys_mib->RegBModeGainStage;
				}
				else if(_sys_mib->InitialGain > _sys_mib->RegBModeGainStage + 1)
				{
					_sys_mib->InitialGain -= 2;
				}
				else
				{
					_sys_mib->InitialGain --;
				}
				DEBUG_INFO(("StaRateAdaptive87B(): update init_gain to index %d for date rate %d\n",_sys_mib->InitialGain, CurrentOperaRate));
				UpdateInitialGain(padapter);
			}
		}
		else // OFDM
		{			
			if(_sys_mib->InitialGain < 4)
			{
				_sys_mib->InitialGain ++;
				DEBUG_INFO(("StaRateAdaptive87B(): update init_gain to index %d for date rate %d\n",_sys_mib->InitialGain, CurrentOperaRate));			
				UpdateInitialGain(padapter);
			}					
		}
	}

	//Record the related info
	_sys_mib->rate = ((int)CurrentOperaRate/2) *10;
	_sys_mib->LastRetryRate = CurrRetryRate;
	_sys_mib->LastTxThroughput = TxThroughput;

}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
void RateAdaptiveWorkItemCallback(_adapter *padapter)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	
	if( (_sys_mib->ForcedDataRate == 0) && (check_fwstate(&padapter->mlmepriv, _FW_LINKED)== _TRUE) ) // Rate adaptive
	{
		DEBUG_INFO(("RTL8187B Rate Adaptive Call back \n"));
		StaRateAdaptive87B(padapter);
	}
	schedule_delayed_work(&_sys_mib->RateAdaptiveWorkItem, DEFAULT_RATE_ADAPTIVE_TIMER_PERIOD);
}
#else
void RateAdaptiveWorkItemCallback(struct work_struct *work)
{
	struct mib_info *_sys_mib = container_of(work, struct mib_info, RateAdaptiveWorkItem.work);
	_adapter *padapter = _sys_mib->padapter;

	if( (_sys_mib->ForcedDataRate == 0) && (check_fwstate(&padapter->mlmepriv, _FW_LINKED)== _TRUE) ) // Rate adaptive
	{
		DEBUG_INFO(("RTL8187B Rate Adaptive Call back \n"));
		StaRateAdaptive87B(padapter);
	}
	schedule_delayed_work(&_sys_mib->RateAdaptiveWorkItem, DEFAULT_RATE_ADAPTIVE_TIMER_PERIOD);
}
#endif

void r8187_start_RateAdaptive(_adapter *padapter)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	_sys_mib->bRateAdaptive = _TRUE;
	schedule_delayed_work(&_sys_mib->RateAdaptiveWorkItem, DEFAULT_RATE_ADAPTIVE_TIMER_PERIOD);
}

void r8187_stop_RateAdaptive(_adapter *padapter)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	if(_sys_mib->bRateAdaptive){
		cancel_rearming_delayed_work(&_sys_mib->RateAdaptiveWorkItem);
		_sys_mib->bRateAdaptive = _FALSE;
	}
	else flush_scheduled_work();
}

#endif


