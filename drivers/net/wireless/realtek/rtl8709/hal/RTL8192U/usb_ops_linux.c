/******************************************************************************
* usb_ops_xp.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _HCI_OPS_OS_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>
#include <usb_ops.h>
#include <circ_buf.h>
#include <Ethernet.h>
#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_USB_HCI

	#error "CONFIG_USB_HCI shall be on!\n"

#endif




#ifdef PLATFORM_LINUX

#endif

#ifdef PLATFORM_WINDOWS



#include <rtl8711_spec.h>
#include <usb.h>
#include <usbdlib.h>
#include <usbioctl.h>
#include <usb_ops.h>
#include <recv_osdep.h>

#define USBD_HALTED(Status) ((ULONG)(Status) >> 30 == 3)

#endif

#include <recv_osdep.h>

#include <wlan_mlme.h>
/*--------------------------Define -------------------------------------------*/
#define	MGN_1M				0x02
#define	MGN_2M				0x04
#define	MGN_5_5M			0x0b
#define	MGN_11M			0x16

#define	MGN_6M				0x0c
#define	MGN_9M				0x12
#define	MGN_12M			0x18
#define	MGN_18M			0x24
#define	MGN_24M			0x30
#define	MGN_36M			0x48
#define	MGN_48M			0x60
#define	MGN_54M			0x6c

#define	MGN_MCS0			0x80
#define	MGN_MCS1			0x81
#define	MGN_MCS2			0x82
#define	MGN_MCS3			0x83
#define	MGN_MCS4			0x84
#define	MGN_MCS5			0x85
#define	MGN_MCS6			0x86
#define	MGN_MCS7			0x87
#define	MGN_MCS8			0x88
#define	MGN_MCS9			0x89
#define	MGN_MCS10			0x8a
#define	MGN_MCS11			0x8b
#define	MGN_MCS12			0x8c
#define	MGN_MCS13			0x8d
#define	MGN_MCS14			0x8e
#define	MGN_MCS15			0x8f

#define DESC90_RATE1M					0x00
#define DESC90_RATE2M					0x01
#define DESC90_RATE5_5M				0x02
#define DESC90_RATE11M					0x03
#define DESC90_RATE6M					0x04
#define DESC90_RATE9M					0x05
#define DESC90_RATE12M					0x06
#define DESC90_RATE18M					0x07
#define DESC90_RATE24M					0x08
#define DESC90_RATE36M					0x09
#define DESC90_RATE48M					0x0a
#define DESC90_RATE54M					0x0b
#define DESC90_RATEMCS0				0x00
#define DESC90_RATEMCS1				0x01
#define DESC90_RATEMCS2				0x02
#define DESC90_RATEMCS3				0x03
#define DESC90_RATEMCS4				0x04
#define DESC90_RATEMCS5				0x05
#define DESC90_RATEMCS6				0x06
#define DESC90_RATEMCS7				0x07
#define DESC90_RATEMCS8				0x08
#define DESC90_RATEMCS9				0x09
#define DESC90_RATEMCS10				0x0a
#define DESC90_RATEMCS11				0x0b
#define DESC90_RATEMCS12				0x0c
#define DESC90_RATEMCS13				0x0d
#define DESC90_RATEMCS14				0x0e
#define DESC90_RATEMCS15				0x0f
#define DESC90_RATEMCS32				0x20

u32 read_counter = 0;

typedef	enum _RX_DESC_FIELD{
	//DOWRD 0
	Length,
	CRC32,
	ICV,
	RxDrvInfoSize,
	Shift,
	PHYStatus,
	SWDec,
	//LastSeg,
	//FirstSeg,
	//EOR,
	//OWN,
	Reserved1,

	//DWORD 1
	Reserved2,

}RX_DESC_FIELD,*PRX_DESC_FIELD;


u32
GetRxStatusDesc819xUsb(
	IN struct recv_stat 	*pRxDesc,
	IN RX_DESC_FIELD	descField
	)
{
	u32	value=0;
	
	switch(descField)
	{
		// Dword 0
		case Length:
			value = pRxDesc->frame_length;
			break;

		case CRC32:
			value = pRxDesc->crc32;
			break;			
			
		case ICV:
			value = pRxDesc->icv;
			break;
			
		case RxDrvInfoSize:
			value = pRxDesc->RxDrvInfoSize;
			break;
			
		case Shift:
			value = pRxDesc->RxShift;
			break;
			
		case PHYStatus:
			value = pRxDesc->PHYStatus;
			break;
			
		case SWDec:
			value = pRxDesc->SWDec;
			break;
			
		case Reserved1:
			value = pRxDesc->Reserved1;
			break;

		default:
			break;
	}

	return value;
}




u8 HwRateToMRate90(
	IN	BOOLEAN		bIsHT,	
	IN 	u8			rate	
	)
{
	u8	ret_rate = 0x02;

	if(!bIsHT)
	{
		switch(rate)
		{
		
			case DESC90_RATE1M:			ret_rate = MGN_1M;		break;
			case DESC90_RATE2M:			ret_rate = MGN_2M;		break;
			case DESC90_RATE5_5M:		ret_rate = MGN_5_5M;		break;
			case DESC90_RATE11M:		ret_rate = MGN_11M;		break;
			case DESC90_RATE6M:			ret_rate = MGN_6M;		break;
			case DESC90_RATE9M:			ret_rate = MGN_9M;		break;
			case DESC90_RATE12M:		ret_rate = MGN_12M;		break;
			case DESC90_RATE18M:		ret_rate = MGN_18M;		break;
			case DESC90_RATE24M:		ret_rate = MGN_24M;		break;
			case DESC90_RATE36M:		ret_rate = MGN_36M;		break;
			case DESC90_RATE48M:		ret_rate = MGN_48M;		break;
			case DESC90_RATE54M:		ret_rate = MGN_54M;		break;
		
			default:							
					DEBUG_ERR(("HwRateToMRate90(): Non supported Rate [%x], bIsHT = %d!!!\n", rate, bIsHT ));
					break;
	}

	}else
	{
		switch(rate)
		{
		

			case DESC90_RATEMCS0:	ret_rate = MGN_MCS0;		break;
			case DESC90_RATEMCS1:	ret_rate = MGN_MCS1;		break;
			case DESC90_RATEMCS2:	ret_rate = MGN_MCS2;		break;
			case DESC90_RATEMCS3:	ret_rate = MGN_MCS3;		break;
			case DESC90_RATEMCS4:	ret_rate = MGN_MCS4;		break;
			case DESC90_RATEMCS5:	ret_rate = MGN_MCS5;		break;
			case DESC90_RATEMCS6:	ret_rate = MGN_MCS6;		break;
			case DESC90_RATEMCS7:	ret_rate = MGN_MCS7;		break;
			case DESC90_RATEMCS8:	ret_rate = MGN_MCS8;		break;
			case DESC90_RATEMCS9:	ret_rate = MGN_MCS9;		break;
			case DESC90_RATEMCS10:	ret_rate = MGN_MCS10;	break;
			case DESC90_RATEMCS11:	ret_rate = MGN_MCS11;	break;
			case DESC90_RATEMCS12:	ret_rate = MGN_MCS12;	break;
			case DESC90_RATEMCS13:	ret_rate = MGN_MCS13;	break;
			case DESC90_RATEMCS14:	ret_rate = MGN_MCS14;	break;
			case DESC90_RATEMCS15:	ret_rate = MGN_MCS15;	break;
			case DESC90_RATEMCS32:	ret_rate = (0x80|0x20);	break;
			
			
			default:							
					DEBUG_ERR(("HwRateToMRate90(): Non supported Rate [%x], bIsHT = %d!!!\n",rate, bIsHT ));
				break;
		}

	}

	
	return ret_rate;
}

/**
* Function:	UpdateRxPktTimeStamp
* Overview:	Recored down the TSF time stamp when receiving a packet
* 
* Input:		
* 	PADAPTER	Adapter
*	PRT_RFD		pRfd,
* 			
* Output:		
*	PRT_RFD		pRfd
*				(pRfd->Status.TimeStampHigh is updated)
*				(pRfd->Status.TimeStampLow is updated)
* Return:     	
*		None
*/
VOID
UpdateRxPktTimeStamp8190(
	IN		_adapter 			*padapter,
	IN OUT	union recv_frame *precvframe
	)
{
	//PHAL_DATA_819xUSB	pHalData = GET_HAL_DATA(padapter);
	struct recv_priv *precvpriv = &(padapter->recvpriv);
	struct rx_pkt_attrib *pattrib = &(precvframe->u.hdr.attrib);
	
	
	if(pattrib->bIsAMPDU && !pattrib->bFirstMPDU)
	{
		pattrib->TimeStampHigh = precvpriv->LastRxDescTSFHigh;  //where is TimeStampHigh updated?? John
		pattrib->TimeStampLow = precvpriv->LastRxDescTSFLow;
	}
	else
	{
		precvpriv->LastRxDescTSFHigh = pattrib->TimeStampHigh;  //where is TimeStampHigh updated??
		precvpriv->LastRxDescTSFLow = pattrib->TimeStampLow;
	}
}


VOID
query_rx_desc_status(
	IN		_adapter					*padapter,  		//Adapter
	IN 		union recv_frame			*precvframe  	//pRfd
	)
{
	struct rx_pkt_attrib *prx_attrib = &(precvframe->u.hdr.attrib);
	struct recv_stat *precv_desc = (struct recv_stat *)(precvframe->u.hdr.rx_head);  //FIXME: right or not
	struct recv_drv_info *precv_drv_info = (struct recv_drv_info *)(precvframe->u.hdr.rx_head + sizeof(struct recv_stat) 
					+ prx_attrib->RxBufShift );
	
	//
	//Get Rx Descriptor Information
	//	
	prx_attrib->Length = (u16)GetRxStatusDesc819xUsb(precv_desc, Length);
	prx_attrib->RxDrvInfoSize = (u8)GetRxStatusDesc819xUsb(precv_desc, RxDrvInfoSize);
	prx_attrib->RxBufShift = (u8)(GetRxStatusDesc819xUsb(precv_desc, Shift)&0x03);      
	prx_attrib->bICV = (u16)GetRxStatusDesc819xUsb(precv_desc, ICV);
	prx_attrib->bCRC = (u16)GetRxStatusDesc819xUsb(precv_desc, CRC32);
	prx_attrib->bHwError = (prx_attrib->bCRC|prx_attrib->bICV);
	prx_attrib->Decrypted = !GetRxStatusDesc819xUsb(precv_desc, SWDec);	//RTL8190 set this bit to indicate that Hw does not decrypt packet
	precv_desc->decrypted = prx_attrib->Decrypted;

	//
	//Get Driver Info
	//
	// TODO: Need to verify it on FGPA platform
	//Driver info are written to the RxBuffer following rx desc
	if (prx_attrib->RxDrvInfoSize != 0)
	{
		
		if(!prx_attrib->bHwError)
			prx_attrib->DataRate = HwRateToMRate90(precv_drv_info->RxHT, precv_drv_info->RxRate);
		else
			prx_attrib->DataRate = 0x02;	

		prx_attrib->bShortPreamble = precv_drv_info->SPLCP;

//		// TODO: it is debug only. It should be disabled in released driver. 2007.1.11 by Emily
//		UpdateReceivedRateHistogramStatistics8190(Adapter, pRfd);
		
		prx_attrib->bIsAMPDU = (precv_drv_info->PartAggr==1);
		prx_attrib->bFirstMPDU = (precv_drv_info->PartAggr==1) && (precv_drv_info->FirstAGGR==1);
		
//		// TODO: it is debug only. It should be disabled in released driver. 2007.1.12 by Joseph
//		UpdateRxAMPDUHistogramStatistics8190(Adapter, pRfd);

		prx_attrib->TimeStampLow = precv_drv_info->TSFL;
		// xiong mask it, 070514
		//pRfd->Status.TimeStampHigh = PlatformEFIORead4Byte(Adapter, TSFR+4);
		UpdateRxPktTimeStamp8190(padapter, precvframe);

		//
		// Rx A-MPDU
		//
		if(precv_drv_info->FirstAGGR==1 || precv_drv_info->PartAggr == 1)
			DEBUG_ERR( ("precv_drv_info->FirstAGGR = %d, precv_drv_info->PartAggr = %d\n", 
					precv_drv_info->FirstAGGR, precv_drv_info->PartAggr));
		
	}

	//
	// Get Total offset of MPDU Frame Body 
	//
	if((prx_attrib->RxBufShift + prx_attrib->RxDrvInfoSize) > 0)
		prx_attrib->bShift = 1;

	//added by vivi, for MP, 20080108
	prx_attrib->RxIs40MHzPacket = precv_drv_info->BW;	

}


/**
*
* The strarting address of wireless lan header will shift 1 or 2 or 3 or "more" bytes for the following reason :
* (1) QoS control : shift 2 bytes
* (2) Mesh Network : shift 1 or 3 bytes
* (3) RxDriverInfo occupies  the front parts of Rx Packets buffer
*
* It is because Lextra CPU used by 8186 or 865x series assert exception if the statrting address
* of IP header is not double word alignment.
* This features is supported in 818xb and 8190 only, but not 818x.
* 
* \param PRT_RFD	      pRfd     Pointer of Reeceive frame descriptor which is initialized according to 
*						   Rx Descriptor						   
* \return  unsigned int		   number of total shifted bytes
* 
*  Notes: 2006/9/28, created by  Emily
*/

u32
GetRxPacketShiftBytes819xUsb(
	union recv_frame *precvframe
)
{
	struct rx_pkt_attrib *pattrib = &(precvframe->u.hdr.attrib);

#ifdef USB_RX_AGGREGATION_SUPPORT
	if (pattrib->bIsRxAggrSubframe)
		return (sizeof(struct recv_stat) + pattrib->RxDrvInfoSize 
			+ pattrib->RxBufShift+8);
	else
#endif	
	return (sizeof(struct recv_stat) + pattrib->RxDrvInfoSize 
			+ pattrib->RxBufShift);

}


/*
========AMSDU Subframe=========
+----+-----+-----+-------+-------+
|   6    |   6    |   2     | 0-2304 |  0-3   |
+----+-----+-----+-------+-------+
|         |         |         |             |           |
|  DA   |  RA   |length|  MSDU   |padding|
|         |         |         |             |           |
+----+-----+-----+-------+-------+
============================
*/

//skb->data points AMSDU subframe

sint amsdu_wlanhdr_to_ethhdr (union recv_frame *precvframe,  _pkt *skb)
{
	//remove the wlanhdr and add the eth_hdr
	u8			srcaddr[6];
	u8			desaddr[6];
	u8			RemoveLLCFlag=_FALSE;
	u8			*pLLC, *pFrame;
	u8			NeedCheckTypes=1;
	u8			LLCHeader[1][5]={	// Remember to check NeedCheckTypes
										{0x00,0x00,0x00,0x81,0x37},
									};
	
	int i;
	
_func_enter_;	
		pLLC = skb->data;

		// Get Source Address and Destination Address
		_memcpy(desaddr, pLLC - ETHERNET_HEADER_SIZE, ETH_ALEN);
		_memcpy(srcaddr, pLLC - ETHERNET_HEADER_SIZE + ETHERNET_ADDRESS_LENGTH, ETH_ALEN);

		
		pFrame = pLLC -ETHERNET_HEADER_SIZE;
		if(	*(pLLC + 0) == 0xaa	&& 
			*(pLLC + 1) == 0xaa	&& 
			*(pLLC + 2) == 0x03
			)
		{	
			// check if need remove LLC header
		
			RemoveLLCFlag=_TRUE;
			for(i=0;i<NeedCheckTypes;i++)
			{
				if(_memcmp((pLLC + 3), LLCHeader[i], 5))
				{
					RemoveLLCFlag=_FALSE;
					break;
				}
			}
			if(RemoveLLCFlag)
				pFrame += 8;
		}
			
		_memcpy(pFrame	, desaddr, ETH_ALEN);
		_memcpy(pFrame+6, srcaddr, ETH_ALEN);

		//2 Insert length field if LLC is not removed
		if(!RemoveLLCFlag)
		{
			u16 len=skb->len;

			*(pFrame + 12)=(u8)(len>>8);
			*(pFrame + 13)=(u8)(len&0xff);
		}

		skb_push(skb, (skb->data - pFrame));
_func_exit_;	
	return 0;
}

#define sCrcLng							4		// octets for crc32 (FCS, ICV)
#define sMacHdrLng						24		// octets in data header, no WEP
#define sQoSCtlLng						2
//
//	There should be 128 bits to cover all of the MCS rates. However, since
//	8190 does not support too much rates, one integer is quite enough.
//




s8 process_indicate_amsdu(_adapter *padapter, union recv_frame *precvframe)
{
	u16			LLCOffset;
	u16			nRemain_Length;
	u16			nSubframe_Length;
	u16			SeqNum=0;
	u8			bIsAggregateFrame = _FALSE;
	u8			nPadding_Length = 0;
	u8 			res = _SUCCESS;
	
	struct rx_pkt_attrib *prxattrib = &(precvframe->u.hdr.attrib);

	SeqNum = (u16)prxattrib->seq_num;
	
	if( prxattrib->qos)
	{
		if(((struct ieee80211_qos_field*)(precvframe->u.hdr.rx_data+sMacHdrLng))->Reserved == 1)
			bIsAggregateFrame = _TRUE;
	}

	// Null packet, don't indicate it to upper layer
	if( prxattrib->Length <= prxattrib->hdrlen )
	{
		DEBUG_ERR(("prxattrib->Length <= prxattrib->hdrlen\n"));
		res =  _SUCCESS;
		goto error;
	}
	
	nRemain_Length = precvframe->u.hdr.rx_tail - precvframe->u.hdr.rx_data - prxattrib->hdrlen;
	LLCOffset = prxattrib->hdrlen;

	if(!bIsAggregateFrame)
	{
		return _FAIL;
	}
	else
	{	
		prxattrib->nTotalSubframe = 0;
		while(nRemain_Length > ETHERNET_HEADER_SIZE)
		{
			_pkt *pskb;
		
			nSubframe_Length = *((u16*)(precvframe->u.hdr.rx_data + prxattrib->hdrlen + 12));
			//==m==>change the length order
			nSubframe_Length = (nSubframe_Length>>8) + (nSubframe_Length<<8);

			if(nRemain_Length<(ETHERNET_HEADER_SIZE + nSubframe_Length))
			{
				DEBUG_ERR(("process_indicate_amsdu(): A-MSDU subframe parse error!! pRfd->nTotalSubframe : %d\n", prxattrib->nTotalSubframe));
				DEBUG_ERR(("process_indicate_amsdu(): A-MSDU subframe parse error!! Subframe Length: %d\n", nSubframe_Length));
				DEBUG_ERR(("nRemain_Length is %d and nSubframe_Length is : %d\n",nRemain_Length,nSubframe_Length));
				DEBUG_ERR(("The Packet SeqNum is %d\n",SeqNum));
				res = _SUCCESS;  //_SUCCESS means that this is an A-MSDU packet. do not regard it as b/g packet in the following code.
				goto error;
			}
		
			LLCOffset += ETHERNET_HEADER_SIZE;		

			//clone a skb and modify its data and tail pointer		
			pskb = skb_clone(precvframe->u.hdr.pkt, GFP_ATOMIC);
			if(pskb == NULL) 
				goto error;
			
			pskb->data = precvframe->u.hdr.rx_data + LLCOffset;
			pskb->tail = precvframe->u.hdr.rx_data + nSubframe_Length;
			pskb->len = nSubframe_Length;
			
			prxattrib->nTotalSubframe++;

			if(prxattrib->nTotalSubframe >= MAX_SUBFRAME_COUNT)
			{
				DEBUG_ERR(("process_indicate_amsdu(): Too many Subframes! Packets dropped!\n"));
				res = _SUCCESS;  //_SUCCESS means that this is an A-MSDU packet. do not regard it as b/g packet in the following code.
				goto error;
			}

			//calculate the padding space of amsdu subframe
			nRemain_Length -= (ETHERNET_HEADER_SIZE + nSubframe_Length);
			if(nRemain_Length != 0)
			{
				nPadding_Length = 4 - ((nSubframe_Length + ETHERNET_HEADER_SIZE) & 3);
				if(nPadding_Length == 4)
					nPadding_Length = 0;
				

				if(nRemain_Length < nPadding_Length)
				{
					DEBUG_ERR(	("process_indicate_amsdu(): A-MSDU subframe parse error!!! Remain Length: %d\n", nRemain_Length));
					res = _SUCCESS;  //_SUCCESS means that this is an A-MSDU packet. do not regard it as b/g packet in the following code.
					goto error;
				}
				
				nRemain_Length -= nPadding_Length;
				LLCOffset += nSubframe_Length + nPadding_Length;
			}			

			//indicate every cloned skbuff after being constructed
			amsdu_wlanhdr_to_ethhdr (precvframe, pskb);
			
			if ((padapter->bDriverStopped ==_FALSE)&&( padapter->bSurpriseRemoved==_FALSE))
			{
				
				//Indicate this cloned skbuff
				pskb->ip_summed = CHECKSUM_NONE;
				pskb->dev = padapter->pnetdev;
				pskb->protocol = eth_type_trans(pskb, padapter->pnetdev);
				
		        	netif_rx(pskb);  //indicate skbuff to OS
			}
			else
				free_os_resource(pskb);
		}
		
	}

error:	
	free_os_resource(precvframe->u.hdr.pkt);
	return res;

}

//
// The driver should never read RxCmd register. We have to set  
// RCR CMDHAT0 (bit6) to append Rx status before the Rx frame. 
//
// |<--------  pBulkUrb->TransferBufferLength  ------------>|
// +------------------+-------------------+------------+
//  | Rx status (16 bytes)  | Rx frame .....             | CRC(4 bytes) |
// +------------------+-------------------+------------+
// ^
// ^pRfd->Buffer.VirtualAddress
//
/*! \brief USB RX IRP Complete Routine.
	@param Context pointer of RT_RFD
*/

static void usb_read_port_complete(struct urb *purb, struct pt_regs *regs)
{
	union recv_frame	*precvframe = (union recv_frame *)purb->context;	
	_adapter *adapter = (_adapter *)precvframe->u.hdr.adapter;
	struct recv_priv *precvpriv = &adapter->recvpriv;
	struct intf_hdl * pintfhdl = &adapter->pio_queue->intf;
	struct recv_stat *prxstat;
	
	DEBUG_INFO(("usb_read_port_complete: actual length: %d\n", purb->actual_length));
	precvframe->u.hdr.irp_pending=_FALSE;
//==============DEBUG_MSG==================
//printk("usb_read_port_complete: dumpping transfer_buffer\n");
//DUMP_92U(purb->transfer_buffer, 80);
//========================================

	precvpriv->rx_pending_cnt --;
	DEBUG_INFO(("\n usb_read_port_complete:precvpriv->rx_pending_cnt=%d\n",precvpriv->rx_pending_cnt));
	if (precvpriv->rx_pending_cnt== 0) 
	{
		DEBUG_ERR(("usb_read_port_complete: rx_pending_cnt== 0, set allrxreturnevt!\n"));
	}

	if(adapter->bSurpriseRemoved ||adapter->bDriverStopped) 
	{
		DEBUG_ERR(("usb_read_port_complete:adapter->bSurpriseRemoved(%x) ||adapter->bDriverStopped(%x)\n",adapter->bSurpriseRemoved,adapter->bDriverStopped));
		//3 Put this recv_frame into recv free Queue

		goto exit;
	}
	
	if(purb->status==0)
	{
		//if((purb->actual_length > (MAX_RXSZ-RECVFRAME_HDR_ALIGN)) || (purb->actual_length < 16) ) 
		if( (purb->actual_length > SZ_RECVFRAME) || (purb->actual_length < 16) ) 
		{
				
			// Put this recv_frame into recv free Queue
			DEBUG_ERR(("\n usb_read_port_complete: (purb->actual_length(%d) > MAX_RXSZ) || (purb->actual_length < 16)\n",purb->actual_length ));
			usb_read_port(pintfhdl,0, 0, (unsigned char *)precvframe);

		} 
		else 
		{
			DEBUG_INFO(("\n usb_read_port_complete: normal urb.\n" ));
			//8192: rx descriptor is followed by packet payload
			prxstat = (struct recv_stat *)(precvframe->u.hdr.rx_head);

			if(prxstat->frame_length > purb->actual_length ) 
			{
				DEBUG_ERR(("frame_length=%d > actual_length=%d\n", prxstat->frame_length, purb->actual_length));
				usb_read_port(pintfhdl,0, 0, (unsigned char *)precvframe);
				goto exit;
			}

			//8192: not defined in 8192 yet
			//get_rx_power_87b(adapter, prxstat);
#ifndef USB_RX_AGGREGATION_SUPPORT
			query_rx_desc_status(adapter, precvframe);

			//FIXME: the format of 8192u is very different from 8187
			//the length needed modified

			//move the rx_data pointer, skip the descriptor and drvInfo
			//then, move the rx_tail pointer to the end of the buffer
			recvframe_put(precvframe, prxstat->frame_length + GetRxPacketShiftBytes819xUsb(precvframe));						
			recvframe_pull(precvframe, GetRxPacketShiftBytes819xUsb(precvframe));
			
			adapter->_sys_mib.NumRxOkTotal++;
#endif
			if(recv_entry((_nic_hdl)precvframe)!=_SUCCESS)
			{
	 			DEBUG_ERR(("usb_read_port_complete: recv_entry(adapter)!=_SUCCESS\n"));
			}
		}	
	}
	else
	{
		DEBUG_INFO(("\n usb_read_port_complete:purb->status(%d)!=0!\n",purb->status));
		DEBUG_INFO(("usb_read_port_complete: actual length: %d\n", purb->actual_length));
		switch(purb->status) 
		{
			case -EINVAL:
			case -EPIPE:
			case -EPROTO:
			case -ENODEV:
			case -ESHUTDOWN:
				adapter->bSurpriseRemoved = _TRUE;
			case -ENOENT:
				adapter->bDriverStopped = _TRUE;
				DEBUG_ERR(("\nusb_read_port:apapter->bDriverStopped-_TRUE\n"));
				//free skb
				if(precvframe->u.hdr.pkt !=NULL)
					dev_kfree_skb_any(precvframe->u.hdr.pkt);
				break;
			case -EINPROGRESS:
				panic("URB IS IN PROGRESS!/n");
				break;
			default:
				usb_read_port(pintfhdl,0, 0, (unsigned char *)precvframe);
				break;
		}
	}
exit:
	//DEBUG_ERR(("-usb_read_port_complete\n\n"));
	return ;

}


void usb_read_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	int err;
	u8 *pdata;
	PURB		purb;
	union recv_frame *precvframe=(union recv_frame *)rmem;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;
	struct dvobj_priv   *pdev = (struct dvobj_priv   *)pintfpriv->intf_dev;
	_adapter *	adapter=(_adapter *)pdev->padapter;
	struct recv_priv *precvpriv=&adapter->recvpriv;
	struct usb_device *pusbd=pdev->pusbdev;

	_func_enter_;
	DEBUG_INFO(("\n +usb_read_port \n"));
	if(precvframe !=NULL){
		
		//init recv_frameadapter->bSurpriseRemoved
		init_recvframe(precvframe, precvpriv);     //already allocate skb_buf in init_recvframe

		precvpriv->rx_pending_cnt++;
		DEBUG_INFO(("\n usb_read_port:precvpriv->rx_pending_cnt=%d\n",precvpriv->rx_pending_cnt));
		precvframe->u.hdr.irp_pending = _TRUE;
	       pdata=precvframe->u.hdr.rx_data;

		purb = precvframe->u.hdr.purb;	
		if(purb ==NULL){
			purb=usb_alloc_urb(0,GFP_ATOMIC);
			DEBUG_ERR(("\n usb_read_port: urb reallocate!!\n"));
		}
		 usb_fill_bulk_urb(purb,pusbd,
			usb_rcvbulkpipe(pusbd, 0x83),   //0x83/*RX_ENDPOINT*/), 
               				purb->transfer_buffer,
                				(SZ_RECVFRAME),
                				(usb_complete_t) usb_read_port_complete,
                				precvframe);   //context is recv_frame

		err = usb_submit_urb(purb, GFP_ATOMIC);	
		if(err && err != -EPERM){
			DEBUG_ERR(("cannot submit RX command(err=%d). URB_STATUS =%d\n", err, purb->status));
		}
	}
	else{
		DEBUG_ERR(("usb_read_port:precv_frame ==NULL\n"));
	}
	
	//_spinunlock(&(pdev->in_lock));
	DEBUG_INFO(("\n -usb_read_port \n"));
	_func_exit_;
}





void usb_read_port_cancel(_adapter *padapter)
{
#if 0
	union recv_frame *precvframe;
	sint i;
	struct dvobj_priv   *pdev = &padapter->dvobjpriv;
	struct recv_priv *precvpriv=&padapter->recvpriv;
	
	precvpriv->rx_pending_cnt--; //decrease 1 for Initialize ++ 
	if (precvpriv->rx_pending_cnt) {
		// Canceling Pending Recv Irp
		precvframe = (union recv_frame *)precvpriv->precv_frame_buf;

		for( i = 0; i < NR_RECVFRAME; i++ )	{
			if (precvframe->u.hdr.irp_pending== _TRUE) {				
				IoCancelIrp(precvframe->u.hdr.pirp);		
			}
				precvframe++;
		}

		while (_TRUE) {
			if (NdisWaitEvent(&precvpriv->allrxreturnevt, 2000)) {
				break;
			}
		}

	}
#endif
}


static void usb_write_port_complete(struct urb *purb, struct pt_regs *regs)
{
	u32				       i, ac_tag;
	u32 sz;
	struct xmit_frame *	pxmitframe = (struct xmit_frame *)purb->context;
	_adapter *padapter = pxmitframe->padapter;
       struct    xmit_priv	*pxmitpriv = &padapter->xmitpriv;		

	_func_enter_;
	DEBUG_INFO(("\n +usb_write_port_complete\n"));
	pxmitframe->irpcnt++;

	pxmitframe->fragcnt--;

	if(padapter->bSurpriseRemoved||padapter->bDriverStopped) {

		//3 Put this recv_frame into recv free Queue

		//NdisInterlockedInsertTailList(&Adapter->RfdIdleQueue, &pRfd->List, &Adapter->rxLock);
		//NdisReleaseSpinLock(&CEdevice->Lock);
		goto out;
	}

	if(purb->status==0) {
		padapter->_sys_mib.NumTxOkTotal++;
		padapter->_sys_mib.NumTxOkBytesTotal += purb->actual_length -28;
	}
	DEBUG_INFO(("WRITE port COMPLETE: TAG: %d, frag cnt: %d\n", pxmitframe->frame_tag,pxmitframe->fragcnt));
	
	if (pxmitframe->frame_tag==TAG_MGNTFRAME)
	{
		pxmitframe->bpending[0] = _FALSE;//must use spin lock
		ac_tag = pxmitframe->ac_tag[0];
		sz = pxmitframe->sz[0];
	}
	else
	{
		for(i=0; i< 8; i++)
		{
         		if(purb == pxmitframe->pxmit_urb[i])
        		{
				pxmitframe->bpending[i] = _FALSE;//
				ac_tag = pxmitframe->ac_tag[i];
        			sz = pxmitframe->sz[i];
				break;		  
        		}
		}
	}
	if(pxmitframe->fragcnt == 0)// if((pxmitframe->fragcnt == 0) && (pxmitframe->irpcnt == 8)){
	{
		if(pxmitframe->frame_tag == TAG_MGNTFRAME)
		{
			DEBUG_INFO(("wirte_port_callback: MGNT frame\n"));	
			free_mgntframe(pxmitpriv, (struct mgnt_frame *)pxmitframe);
		}	
		else
			free_xmitframe(pxmitpriv,pxmitframe);	          
      	}
	else
	{
		DEBUG_INFO(("write port callback : pxmitrame->fragcnt ! =0 ~~~~~~~~~`\n"));
	}

	if(purb->status==0) {
		padapter->_sys_mib.NumTxOkTotal++;
		padapter->_sys_mib.NumTxOkBytesTotal += purb->actual_length -28;
	}
	else
		printk("write port complete : status != 0 \n");
out:
	DEBUG_INFO(("\n -usb_write_port_complete\n"));	  
	_func_exit_;
	return ;
}


uint usb_write_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{

	uint 			ret=_SUCCESS;
 	u32 i=0;
	int status;
	uint				pipe_hdl;	
	PURB				purb = NULL;
	u32			ac_tag = addr;
	_adapter *padapter = (_adapter *)pintfhdl->adapter;
	struct dvobj_priv   *pdev = (struct dvobj_priv   *)&padapter->dvobjpriv;	
	//struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct xmit_frame *pxmitframe = (struct xmit_frame *)wmem;
	struct usb_device *pusbd=pdev->pusbdev;

	_func_enter_;
	DEBUG_INFO(("\n +usb_write_port\n"));

	pxmitframe->bpending[0] = _TRUE;
	pxmitframe->sz[0] = cnt;
	purb = pxmitframe->pxmit_urb[0];
	pxmitframe->ac_tag[0] = ac_tag;
	
	if(purb==NULL)
	{
		DEBUG_ERR(("\n usb_write_port:purb==NULL\n"));
		DEBUG_ERR(("\n usb_write_port:i=%d \n",i));
		purb=pxmitframe->pxmit_urb[i]=usb_alloc_urb(0,GFP_KERNEL);
	}
	
       if (pusbd->speed==USB_SPEED_HIGH)
	{
		if(cnt> 0 && (cnt&511) == 0)
			cnt=cnt+1;

	}
	else
	{
		if(cnt > 0 && (cnt&63) == 0)
			cnt=cnt+1;
	}


	DEBUG_INFO(("usb_write_port: ac_tag=%d\n", ac_tag));
	

	switch(ac_tag)
	{	    
	     case VO_QUEUE_INX:
		 	pipe_hdl=usb_sndbulkpipe(pusbd,0x4/*VO_ENDPOINT*/);
			break;
	     case VI_QUEUE_INX:
		 	pipe_hdl=usb_sndbulkpipe(pusbd,0x5/*VI_ENDPOINT*/);
			break;
	     case BE_QUEUE_INX:
		 	pipe_hdl=usb_sndbulkpipe(pusbd,0x6/*BE_ENDPOINT*/);
			break;
	     case BK_QUEUE_INX:
		 	pipe_hdl=usb_sndbulkpipe(pusbd,0x7/*BK_ENDPOINT*/);	    
			break;

		 case MGNT_QUEUE_INX: //mgnt_queue_inx
			pipe_hdl=usb_sndbulkpipe(pusbd,0xc/*MGNT_ENDPOINT*/); 	
			break;	
			
		 case BEACON_QUEUE_INX: //beacon_queue_inx
		    pipe_hdl=usb_sndbulkpipe(pusbd,0xa/*BEACON_ENDPOINT*/);   
			break;	
				
	     default:
			return _FAIL;
	}

	DEBUG_INFO(("%s(%d): tx_addr %p\n",__FUNCTION__,__LINE__,pxmitframe->mem_addr));

	usb_fill_bulk_urb(	purb,
					pusbd,
					pipe_hdl, 
					pxmitframe->mem_addr,
					cnt,
					(usb_complete_t)usb_write_port_complete,
					pxmitframe);   //context is xmit_frame

	if((pxmitframe->order_tag & 7 ) == 0)
		purb->transfer_flags &= (~URB_NO_INTERRUPT);
	else 
		purb->transfer_flags |= URB_NO_INTERRUPT;

	DEBUG_INFO((" addr : %p , 4byte %x \n", pxmitframe->mem_addr+cnt, *(u32 *)(pxmitframe->mem_addr+cnt-4)  ));

	status = usb_submit_urb(purb, GFP_ATOMIC);

	if (!status){
		//atomic_inc(&priv->tx_pending[priority]);
		ret= _SUCCESS;
	}else{
		//DEBUG_ERR(("Error TX URB %d, error %d",atomic_read(&priv->tx_pending[priority]),status));
		DEBUG_ERR(("usb_write_port: submit urb fail, status=0x%x %d", status, status));
		ret= _FAIL;
	}

	DEBUG_INFO(("\n -usb_write_port\n"));
	_func_exit_;
   return ret;

}



/*
NTSTATUS write_txcmd_scsififo_callback(
	PDEVICE_OBJECT	pUsbDevObj,
	PIRP				pIrp,
	PVOID			pTxContext
) 
*/
static void usb_write_scsi_complete(struct urb *purb, struct pt_regs *regs)
{
		
//	u32				       i, ac_tag;
//	u32 sz;
//	u8 *ptr;
	
	struct SCSI_BUFFER_ENTRY * psb_entry = (struct SCSI_BUFFER_ENTRY *)purb->context;
	_adapter *padapter = psb_entry->padapter;
	struct SCSI_BUFFER *psb = padapter->pscsi_buf;
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);



//john
//DbgPrint("write_txcmd_scsififo_callback  psbhead = %d\n", psb->head);
	_func_enter_;	
	if(padapter->bSurpriseRemoved) {
		DEBUG_ERR(("\nsurprise removed!\n"));
		goto exit;
	}



	DEBUG_INFO(("write_txcmd_scsififo_callback: circ_space = %d", CIRC_SPACE( psb->head,psb->tail,  SCSI_BUFFER_NUMBER)));

	psb_entry->scsi_irp_pending = _FALSE;


	memset(psb_entry->entry_memory, 0, 8);

	if((psb->tail+1)  == SCSI_BUFFER_NUMBER)
		psb->tail = 0;
	else 
		psb->tail ++;

	if(CIRC_CNT(  psb->head, psb->tail, SCSI_BUFFER_NUMBER)==0)
	{
		DEBUG_ERR(("write_txcmd_scsififo_callback: up_sema"));
		_up_sema(&pxmitpriv->xmit_sema);
	}
	


exit:
	
 	_func_exit_;
	return ;

}



uint usb_write_scsi(struct intf_hdl *pintfhdl,u32 cnt, u8 *wmem)
{
	_adapter *padapter = (_adapter *)pintfhdl->adapter;
//	struct SCSI_BUFFER *psb = padapter->pscsi_buf;
	struct SCSI_BUFFER_ENTRY* psb_entry = LIST_CONTAINOR(wmem, struct SCSI_BUFFER_ENTRY , entry_memory);
	PURB	piorw_urb  = psb_entry->purb;
//	int i,temp;
	struct dvobj_priv   *pdev = (struct dvobj_priv   *)&padapter->dvobjpriv;      
	struct usb_device *pusbd=pdev->pusbdev;
	int urb_status ;
	
	_func_enter_;

	if(padapter->bSurpriseRemoved || padapter->bDriverStopped) return 0;
	
	psb_entry->scsi_irp_pending=_TRUE;


	// Build our URB for USBD
	usb_fill_bulk_urb(
				piorw_urb,
				pusbd,
				usb_sndbulkpipe(pusbd, 0xa),
				wmem,
				cnt,
				(usb_complete_t)usb_write_scsi_complete,
				psb_entry);  

	urb_status = usb_submit_urb(piorw_urb, GFP_ATOMIC);

	
	if(urb_status )
		DEBUG_INFO((" submit write_txcmd_scsi URB error!!!! urb_status = %d\n", urb_status));

//DbgPrint("-scsififo_write_test \n");

	_func_exit_;
	return _SUCCESS;// NtStatus;
}



