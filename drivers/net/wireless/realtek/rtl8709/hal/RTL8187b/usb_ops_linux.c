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

#include <rtl8187/wlan_mlme.h>

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
	_adapter *				adapter =(_adapter *) precvframe->u.hdr.adapter;
	struct	recv_priv			*precvpriv=&adapter->recvpriv;
	struct intf_hdl * pintfhdl=&adapter->pio_queue->intf;
	struct recv_stat *prxstat;
	int len;

	DEBUG_INFO(("\n +usb_read_port_complete\n"));
	precvframe->u.hdr.irp_pending=_FALSE;

	precvpriv->rx_pending_cnt --;
	DEBUG_INFO(("\n usb_read_port_complete:precvpriv->rx_pending_cnt=%d\n",precvpriv->rx_pending_cnt));
	if (precvpriv->rx_pending_cnt== 0) {
		DEBUG_ERR(("usb_read_port_complete: rx_pending_cnt== 0, set allrxreturnevt!\n"));
	}

	if(adapter->bSurpriseRemoved ||adapter->bDriverStopped) {
		DEBUG_ERR(("usb_read_port_complete:adapter->bSurpriseRemoved(%x) ||adapter->bDriverStopped(%x)\n",adapter->bSurpriseRemoved,adapter->bDriverStopped));
		//3 Put this recv_frame into recv free Queue

		goto exit;
	}
	if(purb->status==0){
		if((purb->actual_length > (MAX_RXSZ-RECVFRAME_HDR_ALIGN)) || (purb->actual_length < 16) ) {
				
			// Put this recv_frame into recv free Queue
			DEBUG_ERR(("\n usb_read_port_complete: (purb->actual_length(%d) > MAX_RXSZ) || (purb->actual_length < 16)\n",purb->actual_length ));
			//_spinunlock(&(dev->in_lock));
			usb_read_port(pintfhdl,0, 0, (unsigned char *)precvframe);

		} else {
			len = purb->actual_length;
			len -= sizeof (struct recv_stat);
			prxstat = (struct recv_stat *)(precvframe->u.hdr.rx_data+len);

			if(prxstat->frame_length > purb->actual_length ) {
				usb_read_port(pintfhdl,0, 0, (unsigned char *)precvframe);
				goto exit;
			}

			get_rx_power_87b(adapter, prxstat);

			recvframe_put(precvframe, prxstat->frame_length);						

			adapter->_sys_mib.NumRxOkTotal++;

			if(recv_entry((_nic_hdl)precvframe)!=_SUCCESS){
	 			DEBUG_ERR(("usb_read_port_complete: recv_entry(adapter)!=_SUCCESS\n"));
			}
		}
	}
	else{
		DEBUG_ERR(("\n usb_read_port_complete:purb->status(%d)!=0!\n",purb->status));
		switch(purb->status) {
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
	DEBUG_INFO(("\n -usb_read_port_complete\n"));
	return ;

}


void usb_read_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
#if 1
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
	//_spinlock(&(pdev->in_lock));
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
			usb_rcvbulkpipe(pusbd,0x83/*RX_ENDPOINT*/), 
               				purb->transfer_buffer,
                				(MAX_RXSZ-RECVFRAME_HDR_ALIGN),
                				usb_read_port_complete,
                				precvframe);   //context is recv_frame

		err = usb_submit_urb(purb, GFP_ATOMIC);	
		if(err && err != -EPERM){
			DEBUG_ERR(("cannot submit RX command(err=0x%.8x). URB_STATUS =0x%.8x",err,purb->status));
		}
	}
	else{
		DEBUG_ERR(("usb_read_port:precv_frame ==NULL\n"));
	}
	
	//_spinunlock(&(pdev->in_lock));
	DEBUG_INFO(("\n -usb_read_port \n"));
	_func_exit_;
#endif
}





void usb_read_port_cancel(_adapter *padapter){
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
			free_mgntframe(pxmitpriv, (struct mgnt_frame *)pxmitframe);
		else
			free_xmitframe(pxmitpriv,pxmitframe);	          
      	}
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


	
	if (pxmitframe->frame_tag==TAG_MGNTFRAME)
	{
		pxmitframe->bpending[0] = _TRUE;//must use spin lock
		pxmitframe->sz[0] = cnt;
		purb = pxmitframe->pxmit_urb[0];
		pxmitframe->ac_tag[0] = ac_tag;
	}
	else // xmit frame for data 
	{
       for(i=0; i<8; i++)
       {
		DEBUG_INFO(("\n pxmitframe->bpending[%d] = %d\n",i,pxmitframe->bpending[i]));
         if(pxmitframe->bpending[i] == _FALSE)
         {
			DEBUG_INFO(("\n pxmitframe->bpending[i] == _FALSE; i=%d\n",i));
            //_enter_critical(&pxmitpriv->lock, &irqL);	      
            pxmitframe->bpending[i]  = _TRUE;//must use spin lock
            //_exit_critical(&pxmitpriv->lock, &irqL);
            pxmitframe->sz[i] = cnt;
	     purb	= pxmitframe->pxmit_urb[i];
 	     pxmitframe->ac_tag[i] = ac_tag;
	     break;	 
         }
       }
	}
	
	if(purb==NULL)
	{
		DEBUG_ERR(("\n usb_write_port:purb==NULL\n"));
		DEBUG_ERR(("\n usb_write_port:i=%d \n",i));
		purb=pxmitframe->pxmit_urb[i]=usb_alloc_urb(0,GFP_KERNEL);
	}
	
       if (pusbd->speed==USB_SPEED_HIGH)
	{
		if(cnt> 0 && cnt&511 == 0)
			cnt=cnt+1;
	}
	else
	{
		if(cnt > 0 && cnt&63 == 0)
			cnt=cnt+1;
	}

	DEBUG_ERR(("usb_write_port: ac_tag=%d\n", ac_tag));
	

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

	usb_fill_bulk_urb(purb,pusbd,
				pipe_hdl, 
       			pxmitframe->mem_addr,
              		cnt,
              		usb_write_port_complete,
              		pxmitframe);   //context is xmit_frame

	status = usb_submit_urb(purb, GFP_ATOMIC);

	if (!status){
		//atomic_inc(&priv->tx_pending[priority]);
		ret= _SUCCESS;
	}else{
		//DEBUG_ERR(("Error TX URB %d, error %d",atomic_read(&priv->tx_pending[priority]),status));
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
				usb_write_scsi_complete,
				psb_entry);  

	urb_status = usb_submit_urb(piorw_urb, GFP_ATOMIC);

	
	if(urb_status )
		printk(" submit write_txcmd_scsi URB error!!!! urb_status = %d\n", urb_status);

//DbgPrint("-scsififo_write_test \n");

	_func_exit_;
	return _SUCCESS;// NtStatus;
}



