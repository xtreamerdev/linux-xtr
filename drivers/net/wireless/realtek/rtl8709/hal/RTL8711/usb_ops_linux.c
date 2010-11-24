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

static void usb_read_port_complete(struct urb *purb, struct pt_regs *regs){
#if 1
	union recv_frame	*precvframe = (union recv_frame *)purb->context;	
	_adapter *				adapter =(_adapter *) precvframe->u.hdr.adapter;
	struct	recv_priv			*precvpriv=&adapter->recvpriv;
	struct intf_hdl * pintfhdl=&adapter->pio_queue->intf;
	struct recv_stat *prxstat; 

	//_spinlock(&(dev->in_lock));
	DEBUG_INFO(("\n +usb_read_port_complete\n"));
	precvframe->u.hdr.irp_pending=_FALSE;

	precvpriv->rx_pending_cnt --;
	DEBUG_INFO(("\n usb_read_port_complete:precvpriv->rx_pending_cnt=%d\n",precvpriv->rx_pending_cnt));
	if (precvpriv->rx_pending_cnt== 0) {
		DEBUG_ERR(("usb_read_port_complete: rx_pending_cnt== 0, set allrxreturnevt!\n"));
		//NdisSetEvent(&precvpriv->allrxreturnevt);
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
//			prxstat=(struct recv_stat *)precvframe->u.hdr.rx_head;
			prxstat=(struct recv_stat *)precvframe->u.hdr.rx_data;
			recvframe_put(precvframe, prxstat->frame_length+16);
   			recvframe_pull(precvframe, 16);
			if(recv_entry((_nic_hdl)precvframe)!=_SUCCESS){
	 			DEBUG_ERR(("usb_read_port_complete: recv_entry(adapter)!=_SUCCESS\n"));
			}
		}

	}
	else{
		DEBUG_ERR(("\n usb_read_port_complete:purb->status(%d)!=0!\n",purb->status));
		if(purb->status ==-ESHUTDOWN){
			DEBUG_ERR(("\nusb_read_port: ESHUTDOWN\n"));
			adapter->bDriverStopped=_TRUE;
			DEBUG_ERR(("\nusb_read_port:apapter->bDriverStopped-_TRUE\n"));
		}
		else{
			adapter->bSurpriseRemoved=_TRUE;
		}
		//free skb
		if(precvframe->u.hdr.pkt !=NULL)
			dev_kfree_skb_any(precvframe->u.hdr.pkt);

		//_spinunlock(&(dev->in_lock));
            //  usb_read_port(pintfhdl,0, 0, (unsigned char *)precvframe);

	}
exit:
	DEBUG_INFO(("\n -usb_read_port_complete\n"));
	return ;
#endif	
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
			usb_rcvbulkpipe(pusbd,0x7/*RX_ENDPOINT*/), 
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
#if 1			
	u32				       i, ac_tag;
	u32 sz;
	u8 *	ptr;
	struct xmit_frame *	pxmitframe = (struct xmit_frame *)purb->context;
	_adapter *padapter = pxmitframe->padapter;
//	struct dvobj_priv *	pCEdevice =	(struct dvobj_priv *)&padapter->dvobjpriv;	
//	struct	io_queue *pio_queue = (struct io_queue *)padapter->pio_queue;
//	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
       struct    xmit_priv	*pxmitpriv = &padapter->xmitpriv;		
	_func_enter_;
	DEBUG_INFO(("\n +usb_write_port_complete\n"));
       pxmitframe->irpcnt++;//
#ifndef JOHN1202	
	pxmitframe->fragcnt--;
#endif
	ptr=(u8 *)&pxmitframe->mem;
	ptr=ptr+64;
	DEBUG_INFO(("\n usb_write_port_complete: TID=%d SQ#=%d  pxmitpriv->free_xmitframe_cnt=%d\n",*(ptr+49),(u16)*(ptr+34),pxmitpriv->free_xmitframe_cnt));
	

	if(padapter->bSurpriseRemoved||padapter->bDriverStopped) {

		//3 Put this recv_frame into recv free Queue

		//NdisInterlockedInsertTailList(&Adapter->RfdIdleQueue, &pRfd->List, &Adapter->rxLock);
		//NdisReleaseSpinLock(&CEdevice->Lock);
		goto out;
	}

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

	if(pxmitframe->fragcnt == 0)// if((pxmitframe->fragcnt == 0) && (pxmitframe->irpcnt == 8)){
	{
     //		DEBUG_ERR(("\n usb_write_port_complete:pxmitframe->fragcnt == 0\n"));
		free_xmitframe(pxmitpriv,pxmitframe);	          
      	}
out:
	DEBUG_INFO(("\n -usb_write_port_complete\n"));	  
	_func_exit_;
	return ;
#endif
}


uint usb_write_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{
 	u32 i;
	u8 *ptr;
	int status;
	uint				pipe_hdl;	
	PURB				purb = NULL;
	u32			ac_tag = addr;
	uint 			ret;
	_adapter *padapter = (_adapter *)pintfhdl->adapter;
	struct dvobj_priv   *pdev = (struct dvobj_priv   *)&padapter->dvobjpriv;	
	struct xmit_frame *pxmitframe = (struct xmit_frame *)wmem;
	struct usb_device *pusbd=pdev->pusbdev;
	_func_enter_;
	DEBUG_INFO(("\n +usb_write_port\n"));

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
	if(purb==NULL){
		DEBUG_ERR(("\n usb_write_port:purb==NULL\n"));
		DEBUG_ERR(("\n usb_write_port:i=%d \n",i));
		purb=pxmitframe->pxmit_urb[i]=usb_alloc_urb(0,GFP_KERNEL);
		printk("\n usb_write_port:purb==NULL\n");
	}
       if (pusbd->speed==USB_SPEED_HIGH)
	{
		if(cnt> 0 && cnt%512 == 0)
			cnt=cnt+1;
	}
	else
	{
		if(cnt > 0 && cnt%64 == 0)
			cnt=cnt+1;
	}

	//john
	if(cnt<500)	cnt = 500;

       //check if pirp==NULL || purb==NULL       
      
	switch(ac_tag)
	{	    
	     case VO_QUEUE_INX:
		 	pipe_hdl=usb_sndbulkpipe(pusbd,0x1/*VO_ENDPOINT*/);
			break;
	     case VI_QUEUE_INX:
		 	pipe_hdl=usb_sndbulkpipe(pusbd,0x2/*VI_ENDPOINT*/);
			break;
	     case BE_QUEUE_INX:
		 	pipe_hdl=usb_sndbulkpipe(pusbd,0x3/*BE_ENDPOINT*/);
			break;
	     case BK_QUEUE_INX:
		 	pipe_hdl=usb_sndbulkpipe(pusbd,0x4/*BK_ENDPOINT*/);	    
			break;
	     case TS_QUEUE_INX:
			pipe_hdl=usb_sndbulkpipe(pusbd,0x5/*TS_ENDPOINT*/);
			break;	
	     case BMC_QUEUE_INX:
			pipe_hdl=usb_sndbulkpipe(pusbd,0x6/*BM_ENDPOINT*/);
			break;	
	     default:
			//RT_TRACE(COMP_SEND, DBG_LOUD, ("xmit_bulkout(): Queue index is invalid.\n"));
			return _FAIL;
	}

	usb_fill_bulk_urb(purb,pusbd,
				pipe_hdl, 
       			pxmitframe->mem_addr,
              		cnt,
              		usb_write_port_complete,
              		pxmitframe);   //context is xmit_frame

	ptr=(u8 *)&pxmitframe->mem;
	ptr=ptr+64;

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
		DEBUG_ERR(("\nSurprise removed\n"));
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
	struct dvobj_priv   *pdev=(struct dvobj_priv   *)&padapter->dvobjpriv;      
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


static void usbAsnyIntInComplete(struct urb *purb, struct pt_regs *regs)
{

	u32 r_cnt;
	_irqL irqL;
	_list	*phead, *plist;
	u8	bmatch;
	struct io_req	*pio_req;
	
//	NTSTATUS status = STATUS_SUCCESS;
	
	struct io_queue *pio_q = (struct io_queue *)purb->context;
	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	
	_adapter *padapter = (_adapter *)pintf->adapter;

	//PURB	piorw_urb = pintfpriv->piorw_urb;

	u32	*pcmd = (u32 *)(pintfpriv->io_rwmem) + 1;
	u32 false_val=0xffffffff;
	_func_enter_;
	DEBUG_INFO(("\n+usbAsnyIntInComplete!\n"));
	pintfpriv->io_irp_cnt--;
	DEBUG_INFO(("\nio_read_callback:pintfpriv->io_irp_cnt=%d\n",pintfpriv->io_irp_cnt));
	if(pintfpriv->io_irp_cnt==0){
		DEBUG_ERR(("\nio_read_callback:pintfpriv->io_irp_cnt==0\n"));
		_up_sema(&pintfpriv->io_retevt);
	}
	pintfpriv->bio_irp_pending=_FALSE;
	if(pintfpriv->bio_irp_timeout==_TRUE){
//		int 	status;
//		pintfpriv->bio_irp_timeout=_FALSE;
//		pintfpriv->bio_irp_pending=_TRUE;
//		status=usb_submit_urb(purb,GFP_KERNEL);
//		if(status){
//			DEBUG_ERR(("\n usbAsnyIntInComplete: resubmit urb error(%d)!!!!!!!\n",status));
//		}
//		_set_timer(&(pintfpriv->io_timer), 1000);
//		return;
		DEBUG_ERR(("\nreg_in:io_irp_timeout\n"));
	}
	else{

		_cancel_timer(&(pintfpriv->io_timer),&pintfpriv->bio_timer_cancel);
				
	}
	if (pintfpriv->intf_status != _IO_WAIT_COMPLETE){
		DEBUG_ERR(("\n=====usbAsnyIntInComplete:pintfpriv->intf_status != _IO_WAIT_COMPLETE"));
		DEBUG_ERR(("\n=====usbAsnyIntInComplete(bus address 11-8= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n (bytecount 7-4= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n",
		pintfpriv->io_rwmem[11],pintfpriv->io_rwmem[10],pintfpriv->io_rwmem[9],pintfpriv->io_rwmem[8],
		pintfpriv->io_rwmem[7],pintfpriv->io_rwmem[6],pintfpriv->io_rwmem[5],pintfpriv->io_rwmem[4]));

		return ;
		}
	phead = &(pio_q->processing);

	_enter_critical(&(pio_q->lock), &irqL);
	
	
	pintfpriv->intf_status = _IOREADY;
	DEBUG_INFO(("\n=====usbAsnyIntInComplete:pintfpriv->intf_status  = _IOREADY"));
		DEBUG_INFO(("\n=====usbAsnyIntInComplete(bus address 11-8= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n (bytecount 7-4= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n",
		pintfpriv->io_rwmem[11],pintfpriv->io_rwmem[10],pintfpriv->io_rwmem[9],pintfpriv->io_rwmem[8],
		pintfpriv->io_rwmem[7],pintfpriv->io_rwmem[6],pintfpriv->io_rwmem[5],pintfpriv->io_rwmem[4]));

	r_cnt=*(u32 *)(pintfpriv->io_rwmem);
	DEBUG_INFO(("\n ***r_cnt=%d***  \n",r_cnt));
	while(1) 
	{
		if ((is_list_empty(phead)) == _TRUE)
			break;
		if(r_cnt<=0){
			DEBUG_INFO(("\n ***r_cnt=0***  \n"));
			break;
		}
		
		//free all irp in processing list...
		plist = get_next(phead);

		list_delete(plist);

		pio_req = LIST_CONTAINOR(plist, struct io_req, list);

		if(_memcmp((pcmd+1), &(pio_req->addr), 4)){//*(pcmd+1)==pio_req->addr){//
				bmatch=_TRUE;
				DEBUG_INFO(("\n ****(same)******usbAsnyIntInComplete: pcmd=0x%.8x pio_req->addr=0x%.8x*********** \n",*(pcmd+1),pio_req->addr));
			}else{
				bmatch=_FALSE;
				DEBUG_ERR(("\n *****(not same)*****usbAsnyIntInComplete: pcmd=0x%.8x pio_req->addr=0x%.8x*********** \n",*(pcmd+1),pio_req->addr));
				DEBUG_ERR(("\n *****(not same)*****usbAsnyIntInComplete: pcmd(command)=0x%.8x *********** \n",*(pcmd)));
			}

		if (!(pio_req->command & _IO_WRITE_)) {

			pcmd += 2;
			// copy the contents to the read_buf...
			if (pio_req->command & _IO_BURST_)
			{
				_memcpy(pio_req->pbuf, (u8 *)pcmd,  (pio_req->command & _IOSZ_MASK_));
				pcmd += ((pio_req->command & _IOSZ_MASK_) >> 2);
			
			}
			else
			{
				if(bmatch==_TRUE){
				_memcpy((u8 *)&(pio_req->val), (u8 *)pcmd, 4);
				}
				else{
					_memcpy((u8 *)&(pio_req->val), (u8 *)&false_val, 4);
				}
				pcmd++;
			}
			r_cnt--;
		}
			

		if (pio_req->command & _IO_SYNC_){
			DEBUG_INFO(("\n+usbAsnyIntInComplete:_up_sema\n"));
			_up_sema(&pio_req->sema);
		}
		else
		{
                     if(pio_req->_async_io_callback !=NULL)
			          pio_req->_async_io_callback(padapter, pio_req, pio_req->cnxt);
			list_insert_tail(plist, &(pio_q->free_ioreqs));
		}
		
	}
#if 1	
		if (pintf->do_flush == _TRUE) {
		
			if ((is_list_empty(&(pio_q->pending))) == _TRUE)
				pintf->do_flush = _FALSE;	
			else
				async_bus_io(pio_q);

		}
#endif	
	_exit_critical(&(pio_q->lock), &irqL);
//	usb_free_urb(purb);
DEBUG_INFO(("\n-usbAsnyIntInComplete!\n"));
	_func_exit_;
	return ;
	
}

/*
Below will only be called under call-back.
And the calling context will enter the critical
section first.
*/

void _async_protocol_read(struct io_queue *pio_q)
{

	//USBD_STATUS			usbdstatus;
	//PIO_STACK_LOCATION		nextStack;

	u8	val8;
	int 	status;
	struct intf_hdl *pintf = &(pio_q->intf);	
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	_adapter *padapter = (_adapter *)(pintf->adapter);
	struct dvobj_priv   *pdev=&padapter->dvobjpriv;
	struct urb* piorw_urb=NULL;// = pintfpriv->piorw_urb;
	struct usb_device *pusbd=pdev->pusbdev;
//	NTSTATUS	NtStatus = STATUS_SUCCESS;

	_func_enter_;
	DEBUG_INFO(("\n+_async_protocol_read!\n"));
		DEBUG_INFO(("\n=====_async_protocol_read(bus address 11-8= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n (bytecount 7-4= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n",
		pintfpriv->io_rwmem[11],pintfpriv->io_rwmem[10],pintfpriv->io_rwmem[9],pintfpriv->io_rwmem[8],
		pintfpriv->io_rwmem[7],pintfpriv->io_rwmem[6],pintfpriv->io_rwmem[5],pintfpriv->io_rwmem[4]));
      	pdev = (struct dvobj_priv   *)pintfpriv->intf_dev;      
	
//	piorw_urb=usb_alloc_urb(0,GFP_ATOMIC);
	pintfpriv->io_irp_cnt++;
	DEBUG_INFO(("\nio_rwad:pintfpriv->ioirp_cnt=%d\n",pintfpriv->io_irp_cnt));
	pintfpriv->bio_irp_pending=_TRUE;
	piorw_urb=pintfpriv->piorw_urb;
	if(!(piorw_urb)){
		DEBUG_ERR(("\n+_async_protocol_read:usb_alloc_urb fail!!!\n"));
		val8= _FAIL;
		goto exit;
	}
	usb_fill_int_urb(piorw_urb,pusbd,usb_rcvintpipe(pusbd,0x9/*REG_IN_ENDPOINT*/),
		(void *)pintfpriv->io_rwmem,pintfpriv->max_iosz,usbAsnyIntInComplete,
		pio_q,1);
pintfpriv->bio_irp_timeout=_FALSE;
	status=usb_submit_urb(piorw_urb,GFP_KERNEL);
	if(status){
		DEBUG_ERR(("\n _async_protocol_read:submit urb error!!!!!!!\n"));
	}
	_set_timer(&(pintfpriv->io_timer), 1000);

	DEBUG_INFO(("\n _async_protocol_read:submit urb ok!!!!!!!\n"));

exit:	
	DEBUG_INFO(("\n-_async_protocol_read!\n"));
	_func_exit_;
	return;// NtStatus;





}

static void usbAsnyIntOutComplete(struct urb *purb, struct pt_regs *regs)
{
	
	_irqL irqL;
	_list	*head, *plist;

	struct io_req	*pio_req;
	
//	NTSTATUS status = STATUS_SUCCESS;
	
	struct io_queue *pio_q = (struct io_queue *)purb->context;
	struct intf_hdl *pintf = &(pio_q->intf);
	
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	
	_adapter *padapter = (_adapter *)pintf->adapter;


	_func_enter_;
	pintfpriv->io_irp_cnt--;
	DEBUG_INFO(("\nio_write_callback:pintfpriv->io_irp_cnt=%d\n",pintfpriv->io_irp_cnt));
	if(pintfpriv->io_irp_cnt==0){
		DEBUG_ERR(("\nio_write_callback:pintfpriv->io_irp_cnt==0\n"));
		_up_sema(&pintfpriv->io_retevt);
	}
	pintfpriv->bio_irp_pending=_FALSE;
	if(pintfpriv->bio_irp_timeout==_TRUE){
		
//		int 	status;
//		pintfpriv->bio_irp_timeout=_FALSE;
//		pintfpriv->bio_irp_pending=_TRUE;
//		status=usb_submit_urb(purb,GFP_KERNEL);
//		if(status){
//			DEBUG_ERR(("\n usbAsnyIntOutComplete: resubmit urb error(%d)!!!!!!!\n",status));
//		}
//		_set_timer(&(pintfpriv->io_timer), 1000);
		DEBUG_ERR(("\nreg_out:io_irp_timeout\n"));
//		return;
	}
	else{
		_cancel_timer(&(pintfpriv->io_timer),&pintfpriv->bio_timer_cancel);
				
	}
	
	head = &(pio_q->processing);
	_enter_critical(&(pio_q->lock), &irqL);
	DEBUG_INFO(("+usbAsnyIntOutComplete()"));

		DEBUG_INFO(("\n=====usbAsnyIntOutComplete(bus address 11-8= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n (bytecount 7-4= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n",
		pintfpriv->io_rwmem[11],pintfpriv->io_rwmem[10],pintfpriv->io_rwmem[9],pintfpriv->io_rwmem[8],
		pintfpriv->io_rwmem[7],pintfpriv->io_rwmem[6],pintfpriv->io_rwmem[5],pintfpriv->io_rwmem[4]));
						

	if (pintfpriv->intf_status == _IO_WAIT_RSP) {
		// issue read irp to read response...

		pintfpriv->intf_status = _IO_WAIT_COMPLETE;
		DEBUG_INFO(("\n=====usbAsnyIntOutComplete:pintfpriv->intf_status = _IO_WAIT_COMPLETE"));
		_async_protocol_read(pio_q);

	}
	else if (pintfpriv->intf_status == _IO_WAIT_COMPLETE) {

		pintfpriv->intf_status = _IOREADY;
		DEBUG_INFO(("\n=====usbAsnyIntOutComplete:pintfpriv->intf_status = _IOREADY"));

		//free all irp in processing list...

		while(is_list_empty(head) != _TRUE)
		{
			plist = get_next(head);

			list_delete(plist);

			pio_req = LIST_CONTAINOR(plist, struct io_req, list);



			if (pio_req->command & _IO_SYNC_)
				_up_sema(&pio_req->sema);
			else
			{
                   	  	if(pio_req->_async_io_callback !=NULL)
			          pio_req->_async_io_callback(padapter, pio_req, pio_req->cnxt);
				list_insert_tail(plist, &(pio_q->free_ioreqs));
	 	        }
		
		}
		
		
#if 1		
		if (pintf->do_flush == _TRUE) {

			if ((is_list_empty(&(pio_q->pending))) == _TRUE)
				pintf->do_flush = _FALSE;	
			else
				async_bus_io(pio_q);
		}
#endif		

	}
	else {

		DEBUG_ERR(("State Machine is not allowed to be here\n"));

	}
		
	_exit_critical(&(pio_q->lock), &irqL);
//	usb_free_urb(purb);
	DEBUG_INFO(("-usbAsnyIntOutComplete()"));
	_func_exit_;
	return ;

}


void _async_protocol_write(struct io_queue *pio_q)
{

	struct dvobj_priv   *pdev;
	struct usb_device *pusbd;
	u8	val8;
	int 	status;

	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;

	PURB	piorw_urb=NULL;// = &pintfpriv->piorw_urb;
	u32 io_wrlen = pintfpriv->io_wsz << 2;

	_func_enter_;
	DEBUG_INFO(("\n _async_protocol_write:io_wrlen=%d pintfpriv->io_wsz=%d\n",io_wrlen,pintfpriv->io_wsz));
		
		DEBUG_INFO(("\n=====_async_protocol_write(bus address 11-8= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n (bytecount 7-4= 0x%.2x 0x%.2x 0x%.2x 0x%.2x)\n",
		pintfpriv->io_rwmem[11],pintfpriv->io_rwmem[10],pintfpriv->io_rwmem[9],pintfpriv->io_rwmem[8],
		pintfpriv->io_rwmem[7],pintfpriv->io_rwmem[6],pintfpriv->io_rwmem[5],pintfpriv->io_rwmem[4]));

      pdev = (struct dvobj_priv   *)pintfpriv->intf_dev;      
	pusbd=pdev->pusbdev;
	piorw_urb=pintfpriv->piorw_urb;	
	pintfpriv->io_irp_cnt++;
	DEBUG_INFO(("\n io_write:pintfpriv->io_irp_cnt=%d\n",pintfpriv->io_irp_cnt));
	pintfpriv->bio_irp_pending=_TRUE;
	if(piorw_urb==NULL){
		DEBUG_ERR(("\npiorw_urv==NULL\n"));
		piorw_urb=usb_alloc_urb(0,GFP_ATOMIC);
		pintfpriv->piorw_urb=piorw_urb;
		if(!(piorw_urb)){
			val8= _FAIL;
			goto exit;
		}
	}
	DEBUG_INFO(("\n _async_protocol_write(usb_fill_int_urb):io_wrlen=%d \n",io_wrlen));
	usb_fill_int_urb(piorw_urb,pusbd,usb_sndintpipe(pusbd,0x8/*REG_OUT_ENDPOINT*/),
		(void *)pintfpriv->io_rwmem,io_wrlen,usbAsnyIntOutComplete,
		pio_q,1);
	pintfpriv->bio_irp_timeout=_FALSE;
	status=usb_submit_urb(piorw_urb,GFP_KERNEL);
	if(status){
		DEBUG_ERR(("\n _async_protocol_write:submit urb error!!!!!!!\n"));
	}
	_set_timer(&(pintfpriv->io_timer), 1000);
exit:	
	_func_exit_;
	return;// NtStatus;

}

void async_rd_io_callback(_adapter *padapter, struct io_req *pio_req, u8 *cnxt)
{

	 u8 width = (u8)((pio_req->command & (_IO_BYTE_|_IO_HW_ |_IO_WORD_)) >> 10);
	  _func_enter_;
       _memcpy((u8 *)pio_req->pbuf, (u8 *)&(pio_req->val), width);

//	NdisMQueryInformationComplete(padapter->hndis_adapter, 
//		                                          NDIS_STATUS_SUCCESS );
	_func_exit_;
       return;
}





