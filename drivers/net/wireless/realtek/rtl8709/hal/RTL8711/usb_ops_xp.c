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
#include <circ_buf.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_USB_HCI

	#error "CONFIG_USB_HCI shall be on!\n"

#endif









#include <rtl8711_spec.h>
#include <usb.h>
#include <usbdlib.h>
#include <usbioctl.h>
#include <usb_ops.h>
#include <recv_osdep.h>

#define USBD_HALTED(Status) ((ULONG)(Status) >> 30 == 3)






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
NTSTATUS usb_read_port_complete(PDEVICE_OBJECT	pUsbDevObj, PIRP pIrp, PVOID context) {

	struct _URB_BULK_OR_INTERRUPT_TRANSFER	*pbulkurb;
	USBD_STATUS			usbdstatus;
	union recv_frame	*		pnxtframe,*precvframe = (union recv_frame *)context;
	
	_adapter *				adapter =(_adapter *) precvframe->u.hdr.adapter;
	struct	recv_priv			*precvpriv=&adapter->recvpriv;
	struct dvobj_priv   *dev = (struct dvobj_priv   *)&adapter->dvobjpriv;
	PURB					purb=precvframe->u.hdr.purb;
	struct intf_hdl * pintfhdl=&adapter->pio_queue->intf;
	struct recv_stat *prxstat; 
	_irqL irqL;
//	UCHAR				*DA;

//	_spinlock(&(dev->in_lock));
	usbdstatus = URB_STATUS(purb);

	_enter_critical(&precvpriv->lock, &irqL);
	precvframe->u.hdr.irp_pending=_FALSE;
	precvpriv->rx_pending_cnt --;
	_exit_critical(&precvpriv->lock, &irqL);
	
	if (precvpriv->rx_pending_cnt== 0) {
		DEBUG_ERR(("usb_read_port_complete: rx_pending_cnt== 0, set allrxreturnevt!\n"));
		_up_sema(&precvpriv->allrxreturnevt);	//NdisSetEvent(&precvpriv->allrxreturnevt);
	}

	if( pIrp->Cancel == _TRUE ) {
		DEBUG_ERR(("usb_read_port_complete: One IRP has been cancelled succesfully\n"));

		//3 Put this recv_frame  into recv free Queue
		
		//NdisInterlockedInsertTailList(&Adapter->RfdIdleQueue, &pRfd->List, &Adapter->rxLock);
		
		//NdisReleaseSpinLock(&CEdevice->Lock);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	if(adapter->bSurpriseRemoved) {

		//3 Put this recv_frame into recv free Queue

		//NdisInterlockedInsertTailList(&Adapter->RfdIdleQueue, &pRfd->List, &Adapter->rxLock);
		//NdisReleaseSpinLock(&CEdevice->Lock);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	switch(pIrp->IoStatus.Status) {

		case STATUS_SUCCESS:
			pbulkurb = &(precvframe->u.hdr.purb)->UrbBulkOrInterruptTransfer;
			if((pbulkurb->TransferBufferLength >(MAX_RXSZ-RECVFRAME_HDR_ALIGN)) || (pbulkurb->TransferBufferLength < 16) ) {
				
				// Put this recv_frame into recv free Queue
				DEBUG_ERR(("\n usb_read_port_complete: (pbulkurb->TransferBufferLength > MAX_RXSZ) || (pbulkurb->TransferBufferLength < 16)\n"));
//				_spinunlock(&(dev->in_lock));
				usb_read_port(pintfhdl,0, 0, (unsigned char *)precvframe);

				//NdisInterlockedInsertTailList(&Adapter->RfdIdleQueue, &pRfd->List, &Adapter->rxLock);
				//CEusbIssueAnIRPRecv(CEdevice);

			} else {	
				prxstat=(struct recv_stat *)precvframe->u.hdr.rx_head;
				recvframe_put(precvframe, prxstat->frame_length+16);
	   			recvframe_pull(precvframe, 16);
				if(recv_entry(precvframe)!=_SUCCESS){
		 			DEBUG_ERR(("usb_read_port_complete: recv_entry(adapter)!=NDIS_STATUS_SUCCESS\n"));
				}
#if 0
				if(adapter->bDriverStopped !=_TRUE){
				pnxtframe=alloc_recvframe(&precvpriv->free_recv_queue);
//				_spinunlock(&(dev->in_lock));
				usb_read_port(pintfhdl,0, 0, (unsigned char *)pnxtframe);
			}
#endif
			}
			break;

		default:



			if( !USBD_HALTED(usbdstatus) ) 	{
				//NdisMSetTimer(&CEdevice->IssueRxIrpTimer, 10);
				DEBUG_ERR(("\n usb_read_port_complete():USBD_HALTED(usbdstatus)=%x  (need to handle ) \n",USBD_HALTED(usbdstatus)));

			} else {

				DEBUG_ERR(("\n usb_read_port_complete(): USBD_HALTED(usbdstatus)=%x \n\n",USBD_HALTED(usbdstatus)) );

			}
//				_spinunlock(&(dev->in_lock));
			//	usb_read_port(pintfhdl,0, 0, (unsigned char *)precvframe);

			break;
	}

	return STATUS_MORE_PROCESSING_REQUIRED;
}


void usb_read_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{

	u8 *pdata;
	u16 size;
	PURB		purb;
	PIRP			pirp;
	PIO_STACK_LOCATION	nextStack;
	NTSTATUS			ntstatus;
	USBD_STATUS		usbdstatus;
	union recv_frame *precvframe=(union recv_frame *)rmem;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;
	struct dvobj_priv   *pdev = (struct dvobj_priv   *)pintfpriv->intf_dev;
//	PCE_USB_DEVICE	pdev=(PCE_USB_DEVICE)pintfpriv->intf_dev;
	_adapter *	adapter=(_adapter *)pdev->padapter;
	struct recv_priv *precvpriv=&adapter->recvpriv;
	_irqL irqL;

	_func_enter_;
	if(adapter->bDriverStopped || adapter->bSurpriseRemoved||adapter->pwrctrlpriv.pnp_bstop_trx) {
		DEBUG_ERR(("usb_read_port:( padapter->bDriverStopped ||padapter->bSurpriseRemoved ||adapter->pwrctrlpriv.pnp_bstop_trx)!!!\n"));
		return;
	}

//	_spinlock(&(pdev->in_lock));
	if(precvframe !=NULL){
		
		//init recv_frame
		init_recvframe(precvframe, precvpriv);     

		_enter_critical(&precvpriv->lock, &irqL);
		precvpriv->rx_pending_cnt++;
		precvframe->u.hdr.irp_pending = _TRUE;
		_exit_critical(&precvpriv->lock, &irqL);

	       pdata=precvframe->u.hdr.rx_data;

		 size	 = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
		 purb = precvframe->u.hdr.purb;	
		 UsbBuildInterruptOrBulkTransferRequest(
			purb,
			(USHORT)size,
			pdev->rx_pipehandle,
			pdata,
			NULL, 
			(MAX_RXSZ-RECVFRAME_HDR_ALIGN),
			USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK, 
			NULL
			);

	 
		pirp = precvframe->u.hdr.pirp;

		// call the class driver to perform the operation
		// and pass the URB to the USB driver stack
		nextStack = IoGetNextIrpStackLocation(pirp);
		nextStack->Parameters.Others.Argument1 = purb;
		nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
		nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

		IoSetCompletionRoutine(
			pirp,					// irp to use
			usb_read_port_complete,	// routine to call when irp is done
			precvframe,					// context to pass routine 
			TRUE,					// call on success
			TRUE,					// call on error
			TRUE);					// call on cancel

	//
	// The IoCallDriver routine  
	// sends an IRP to the driver associated with a specified device object.
	//
		ntstatus = IoCallDriver(pdev->pnextdevobj, pirp);
		usbdstatus = URB_STATUS(purb);

		if( ntstatus != STATUS_PENDING || USBD_HALTED(usbdstatus) ) {
			if( USBD_HALTED(usbdstatus) ) {

			DEBUG_ERR(("\n usb_read_port(): USBD_HALTED(usbdstatus=0x%.8x)=%.8x  \n\n",usbdstatus,USBD_HALTED(usbdstatus)) );
			}
			pdev->padapter->bSurpriseRemoved=_TRUE;
			DbgPrint("usb_read_port:pdev->padapter->bSurpriseRemoved=_TRUE");
		}

	}
	else{
		DEBUG_ERR(("usb_read_port:precv_frame ==NULL\n"));
	}
	

//	_spinunlock(&(pdev->in_lock));
	_func_exit_;
}





void usb_read_port_cancel(_adapter *padapter){

	union recv_frame *precvframe;
	sint i;
	struct dvobj_priv   *pdev = &padapter->dvobjpriv;
	struct recv_priv *precvpriv=&padapter->recvpriv;
	struct SCSI_BUFFER * psb = padapter->pscsi_buf;
	struct SCSI_BUFFER_ENTRY *psb_entry = psb->pscsi_buffer_entry;
	_irqL irqL;
	
	DbgPrint("\n ==>usb_read_port_cancel\n");

	_enter_critical(&precvpriv->lock, &irqL);
	precvpriv->rx_pending_cnt--; //decrease 1 for Initialize ++ 
	_exit_critical(&precvpriv->lock, &irqL);

	if (precvpriv->rx_pending_cnt) {
		// Canceling Pending Recv Irp
		precvframe = (union recv_frame *)precvpriv->precv_frame_buf;
		
		for( i = 0; i < NR_RECVFRAME; i++ )	{
			if (precvframe->u.hdr.irp_pending== _TRUE) {				
				IoCancelIrp(precvframe->u.hdr.pirp);		
			}
				precvframe++;
		}
	DbgPrint("\n usb_read_port_cancel:cancel irp_pending\n");

	//	while (_TRUE) {
	//		if (NdisWaitEvent(&precvpriv->allrxreturnevt, 2000)) {
	//			break;
	//		}
	//	}
		_down_sema(&precvpriv->allrxreturnevt);
		DbgPrint("\n usb_read_port_cancel:down sema\n");


	}
	

	for(i=0 ; i<SCSI_BUFFER_NUMBER; i++, psb_entry++)
	{
		if(psb_entry->scsi_irp_pending == _TRUE)
				IoCancelIrp(psb_entry->pirp);		
	}
	DbgPrint("\n usb_read_port_cancel:cancel scsi_irp_pending\n");

	DbgPrint("\n <==usb_read_port_cancel\n");
	
}




NTSTATUS usb_write_port_complete(
	PDEVICE_OBJECT	pUsbDevObj,
	PIRP				pIrp,
	PVOID			pTxContext
) 
{
			
	u32				       i, bIrpSuccess,ac_tag;
	u32 sz;
	NTSTATUS			status = STATUS_SUCCESS;
		   u8 *ptr;
	struct xmit_frame *	pxmitframe = (struct xmit_frame *) pTxContext;
	_adapter *padapter = pxmitframe->padapter;
	struct dvobj_priv *	pdev =	(struct dvobj_priv *)&padapter->dvobjpriv;	
	struct	io_queue *pio_queue = (struct io_queue *)padapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
       struct    xmit_priv	*pxmitpriv = &padapter->xmitpriv;		
	_func_enter_;

//	DEBUG_ERR(("\n >===usb_write_port_complete(usb_complete_cnt=%d)===<\n",usb_complete_cnt));

	//NdisAcquireSpinLock(&CEdevice->Lock);	
	pdev->txirp_cnt--;
	if(pdev->txirp_cnt==0){
		DEBUG_ERR(("usb_write_port_complete: txirp_cnt== 0, set allrxreturnevt!\n"));
//		DbgPrint("\n usb_write_port_cancel :IoCancelIrp\n");

		//NdisSetEvent(&pCEdevice->txirp_retevt);
		_up_sema(&(pdev->tx_retevt));

	}
		
	
	status = pIrp->IoStatus.Status;

	if( status == STATUS_SUCCESS ) 
		bIrpSuccess = _TRUE;	
	else	
		bIrpSuccess = _FALSE;
	ptr=(u8 *)&pxmitframe->mem;
	ptr=ptr+64;
	DEBUG_INFO(("\n usb_write_port_complete: TID=%d SQ#=%d  pxmitpriv->free_xmitframe_cnt=%d\n",*(ptr+49),(u16)*(ptr+34),pxmitpriv->free_xmitframe_cnt));
	
	// <NOTE> Decrease IrpPendingCount (0-byte IRP had also be ++).
	//CEdevice->TxVOIrpPendingCount--;

	if( pIrp->Cancel == _TRUE )
	{		
	    if(pxmitframe !=NULL)
	    {	       
	    			DEBUG_ERR(("\n usb_write_port_complete:pIrp->Cancel == _TRUE,(pxmitframe !=NULL\n"));
				free_xmitframe(pxmitpriv,pxmitframe);
	
	    }
		
	     //Driver is going to unload.
	     //if (CEdevice->TxVOIrpPendingCount == 0)
	     // 	NdisSetEvent(&CEdevice->AllTxVOIrpReturnedEvent);
		
	     //RT_TRACE(COMP_SEND, DBG_TRACE, ("<=== SendPacketUSBVOComplete(): TxVOIrpPendingCount: %x\n", CEdevice->TxVOIrpPendingCount));
	     //NdisReleaseSpinLock(&CEdevice->Lock);
	     _func_exit_;
	     return STATUS_MORE_PROCESSING_REQUIRED;
	}


	//
	// Send 0-byte here if necessary.
	//
	// <Note> 
	// 1. We MUST keep at most one IRP pending in each endpoint, otherwise USB host controler driver will hang.
	// Besides, even 0-byte IRP shall be count into #IRP sent down, so, we send 0-byte here instead of TxFillDescriptor8187().
	// 2. If we don't count 0-byte IRP into an #IRP sent down, Tx will stuck when we download files via BT and 
	// play online video on XP SP1 EHCU.
	// 2005.12.26, by rcnjko.
	//
	for(i=0; i< 8; i++)
	{
            if(pIrp == pxmitframe->pxmit_irp[i])
            {
		    pxmitframe->bpending[i] = _FALSE;//
		    ac_tag = pxmitframe->ac_tag[i];
                  sz = pxmitframe->sz[i];
		    break;		  
            }
	}
	
    	pxmitframe->fragcnt--;
      if(pxmitframe->fragcnt == 0)// if((pxmitframe->fragcnt == 0) && (pxmitframe->irpcnt == 8)){
     {
     //		DEBUG_ERR(("\n usb_write_port_complete:pxmitframe->fragcnt == 0\n"));
           free_xmitframe(pxmitpriv,pxmitframe);	          
      	}
		  
	_func_exit_;
	return STATUS_MORE_PROCESSING_REQUIRED;

}

uint usb_write_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{
       u32 i;
       _irqL irqL;	
	   u8 *ptr;
	PIO_STACK_LOCATION	nextStack;
	USBD_STATUS		usbdstatus;
	HANDLE				PipeHandle;	
	PIRP					pirp = NULL;
	PURB				purb = NULL;
	u32			ac_tag = addr;
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	_adapter *padapter = (_adapter *)pintfhdl->adapter;
	struct dvobj_priv   *pNdisCEDvice = (struct dvobj_priv   *)&padapter->dvobjpriv;	
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct xmit_frame *pxmitframe = (struct xmit_frame *)wmem;
	_func_enter_;

#if 1    
	  if(padapter->pwrctrlpriv.pnp_bstop_trx==_TRUE){
		DEBUG_ERR(("\npadapter->pwrctrlpriv.pnp_bstop_trx==_TRUE\n"));
		_func_exit_;
		return _FAIL;
	  }
#endif	

//	DEBUG_ERR(("\n <===usb_write_port(usb_write_cnt=%d)===> \n",usb_write_cnt));
	pNdisCEDvice->txirp_cnt++;

       for(i=0; i<8; i++)
       {
         if(pxmitframe->bpending[i] == _FALSE)
         {
#ifdef JOHN1202
            _enter_critical(&pxmitpriv->lock, &irqL);	      
            pxmitframe->bpending[i]  = _TRUE;//must use spin lock
            _exit_critical(&pxmitpriv->lock, &irqL);
#else
            pxmitframe->bpending[i]  = _TRUE;//must use spin lock
#endif

            pxmitframe->sz[i] = cnt;
	     purb	= pxmitframe->pxmit_urb[i];
            pirp	= pxmitframe->pxmit_irp[i];
 	     pxmitframe->ac_tag[i] = ac_tag;
	     break;	 
         }
       }

       if (pNdisCEDvice->ishighspeed)
	{
		if(cnt> 0 && cnt%512 == 0)
			cnt=cnt+1;

	}
	else
	{
		if(cnt > 0 && cnt%64 == 0)
			cnt=cnt+1;
	}

	//if we wirte txcmd buffer into scsififio, the txfifo can not receive more than 8 payload
	if (cnt <= 500) cnt = 500;


       //check if pirp==NULL || purb==NULL       
      
	switch(ac_tag)
	{	    
	     case VO_QUEUE_INX:
		 	PipeHandle= pNdisCEDvice->vo_pipehandle; 
			break;
	     case VI_QUEUE_INX:
		 	PipeHandle= pNdisCEDvice->vi_pipehandle; 
			break;
	     case BE_QUEUE_INX:
		 	PipeHandle= pNdisCEDvice->be_pipehandle; 
			break;
	     case BK_QUEUE_INX:
		 	PipeHandle= pNdisCEDvice->bk_pipehandle;  
			break;
	     case TS_QUEUE_INX:
			PipeHandle= pNdisCEDvice->ts_pipehandle; 
	     case BMC_QUEUE_INX:
		 	PipeHandle=pNdisCEDvice->bm_pipehandle; 
			break;		
	     default:
			return _FAIL;
	}

	// Build our URB for USBD
	UsbBuildInterruptOrBulkTransferRequest(
				purb,
				sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
				PipeHandle,
				pxmitframe->mem_addr, 
				NULL, 
				cnt, 
				0, 
				NULL);
	
	//
	// call the calss driver to perform the operation
	// pass the URB to the USB driver stack
	//
	nextStack = IoGetNextIrpStackLocation(pirp);
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->Parameters.Others.Argument1 = purb;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	//Set Completion Routine
	IoSetCompletionRoutine(pirp,					// irp to use
				               usb_write_port_complete,	// callback routine
				               pxmitframe,				// context 
				               TRUE,					// call on success
				               TRUE,					// call on error
				               TRUE);					// call on cancel
	ptr=(u8 *)&pxmitframe->mem;
	ptr=ptr+64;
	DEBUG_INFO(("\n usb_write_port: TID=%d SQ#=%d  pxmitpriv->free_xmitframe_cnt=%d \n",*(ptr+49),(u16)*(ptr+34),pxmitpriv->free_xmitframe_cnt));
	// 
	// Call IoCallDriver to send the irp to the usb bus driver
	//
	ndisStatus = IoCallDriver(pNdisCEDvice->pnextdevobj, pirp);
	usbdstatus = URB_STATUS(purb);

	//
	// The usb bus driver should always return STATUS_PENDING when bulk out irp async
	//
	if ( (ndisStatus != STATUS_PENDING) || USBD_HALTED(usbdstatus) )
	{
		DEBUG_ERR(("\n usb_write_port(): ndisStatus(%x) != STATUS_PENDING!\n\n",ndisStatus) );
		padapter->bSurpriseRemoved=_TRUE;
//		DbgPrint("usb_write_port:padapter->bSurpriseRemoved=_TRUE");
		if( USBD_HALTED(usbdstatus) )
		{
			DEBUG_ERR(("\n usb_write_port(): USBD_HALTED(usbdstatus)=%x set bDriverStopped TRUE!\n\n",USBD_HALTED(usbdstatus)) );
		}
		_func_exit_;
		//return _FAIL;
	} 	

       //

	
	_func_exit_;
   return _SUCCESS;
   
}






NTSTATUS write_scsi_complete(
	PDEVICE_OBJECT	pUsbDevObj,
	PIRP				pIrp,
	PVOID			pTxContext
) 
{
		
	u32				       i, ac_tag;
	u32 sz;
	NTSTATUS			status = STATUS_SUCCESS;
	u8 *ptr;
	USBD_STATUS			usbdstatus;
	_irqL	irqL2;
	
	struct SCSI_BUFFER_ENTRY * psb_entry = (struct SCSI_BUFFER_ENTRY *)pTxContext;
	_adapter *padapter = psb_entry->padapter;
	struct SCSI_BUFFER *psb = padapter->pscsi_buf;
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);

	_func_enter_;
	
	status = pIrp->IoStatus.Status;

	DEBUG_INFO(("write_txcmd_scsififo_callback: circ_space = %d", CIRC_SPACE( psb->head,psb->tail,  SCSI_BUFFER_NUMBER)));

	psb_entry->scsi_irp_pending = _FALSE;

	usbdstatus = URB_STATUS(psb_entry->purb);

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


	DEBUG_INFO(("\n usb_read_port_complete!!!\n"));

	if( pIrp->Cancel == _TRUE ) {

		DEBUG_ERR(("usb_read_port_complete: One IRP has been cancelled succesfully\n"));
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	if(padapter->bSurpriseRemoved) {
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	switch(pIrp->IoStatus.Status) {

		case STATUS_SUCCESS:


			break;

		default:
			padapter->bSurpriseRemoved=_TRUE;
			DbgPrint("write_txcmd_scsififo_callback:padapter->bSurpriseRemoved=_TRUE");
			if( !USBD_HALTED(usbdstatus) ) 	{
				DEBUG_ERR(("\n usb_read_port_complete():USBD_HALTED(usbdstatus)=%x  (need to handle ) \n",USBD_HALTED(usbdstatus)));

			} else {

				DEBUG_ERR(("\n usb_read_port_complete(): USBD_HALTED(usbdstatus)=%x \n\n",USBD_HALTED(usbdstatus)) );

			}
			break;
	}
	
 	_func_exit_;
	return STATUS_MORE_PROCESSING_REQUIRED;

}


uint usb_write_scsi(struct intf_hdl *pintfhdl, u32 cnt, u8 *wmem)
{
	USBD_STATUS			usbdstatus;
	PIO_STACK_LOCATION		nextStack;
	struct dvobj_priv   *pdev;
	_adapter *padapter = (_adapter *)pintfhdl->adapter;
	struct SCSI_BUFFER *psb = padapter->pscsi_buf;
	struct SCSI_BUFFER_ENTRY* psb_entry = LIST_CONTAINOR(wmem, struct SCSI_BUFFER_ENTRY , entry_memory);
	PURB	piorw_urb = psb_entry->purb;
	PIRP		piorw_irp  = psb_entry->pirp;
	NTSTATUS 	NtStatus = STATUS_SUCCESS;
	int i,temp;
	
	_func_enter_;
	
       pdev = (struct dvobj_priv   *)&padapter->dvobjpriv;      
	_func_enter_;


	if(padapter->bSurpriseRemoved || padapter->bDriverStopped) return 0;
	
	psb_entry->scsi_irp_pending=_TRUE;

	// Build our URB for USBD
	UsbBuildInterruptOrBulkTransferRequest(
				piorw_urb,
				sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
				pdev->scsi_out_pipehandle,//pintfpriv->RegInterruptOutPipeHandle,
				wmem,
				NULL, 
				cnt, 
				0, 
				NULL);  

	//
	// call the calss driver to perform the operation
	// pass the URB to the USB driver stack
	//
	nextStack = IoGetNextIrpStackLocation(piorw_irp);
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->Parameters.Others.Argument1 = (PURB)piorw_urb;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	IoSetCompletionRoutine(
				piorw_irp,				// irp to use
				write_scsi_complete,		// routine to call when irp is done
				psb_entry,				// context to pass routine
				TRUE,					// call on success
				TRUE,					// call on error
				TRUE);					// call on cancel

	// 
	// Call IoCallDriver to send the irp to the usb port
	//
	NtStatus	= IoCallDriver(pdev->pnextdevobj, piorw_irp);//IoCallDriver(pintfpriv->pUsbDevObj, piorw_irp);
	usbdstatus = URB_STATUS(piorw_urb);

	//
	// The USB driver should always return STATUS_PENDING when
	// it receives a write irp
	//
	if ((NtStatus != STATUS_PENDING) || USBD_HALTED(usbdstatus) ) {

		if( USBD_HALTED(usbdstatus) ) {

			DEBUG_ERR(("_async_protocol_write():USBD_HALTED(usbdstatus)=%X!\n",USBD_HALTED(usbdstatus)) );
		}
		padapter->bSurpriseRemoved = _TRUE;
		DbgPrint("write_txcmd_scsififo:padapter->bSurpriseRemoved = _TRUE");
		_func_exit_;
		return _FAIL;//STATUS_UNSUCCESSFUL;
	}
	
	_func_exit_;
   return _SUCCESS;
   
}


void usb_write_port_cancel(_adapter *padapter){

	sint i,j;
	struct dvobj_priv   *pdev = &padapter->dvobjpriv;
	struct xmit_priv *pxmitpriv=&padapter->xmitpriv;
	struct xmit_frame *pxmitframe;
		DbgPrint("\n ===> usb_write_port_cancel \n");
	DbgPrint("\n usb_write_port_cancel :pdev->txirp_cnt=%d\n",pdev->txirp_cnt);
	pdev->txirp_cnt--; //decrease 1 for Initialize ++ 
	if (pdev->txirp_cnt) {
		// Canceling Pending Recv Irp
		pxmitframe= (struct xmit_frame *)pxmitpriv->pxmit_frame_buf;
		
		for( i = 0; i < NR_XMITFRAME; i++ )	{
			for(j=0;j<8;j++){
				if (pxmitframe->bpending[j]==_TRUE){			
						IoCancelIrp(pxmitframe->pxmit_irp[j]);		
						DbgPrint("\n usb_write_port_cancel :IoCancelIrp\n");

				}
			}
			pxmitframe++;
		}
		DbgPrint("\n usb_write_port_cancel :cancel pxmit_irp\n");

//		while (_TRUE) {
//			if (NdisWaitEvent(&pdev->txirp_retevt, 2000)) {
//				break;
//			}
//		}
		_down_sema(&(pdev->tx_retevt));
		DbgPrint("\n usb_write_port_cancel :down sema\n");

	}
			DbgPrint("\n <=== usb_write_port_cancel \n");

}



void usb_cancel_io_irp(_adapter *padapter){

	s8			i;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	
	struct io_queue *pio_q = padapter->pio_queue;
	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	
	pintfpriv->io_irp_cnt--;

	if(pintfpriv->io_irp_cnt){
		if(pintfpriv->bio_irp_pending ==_TRUE){
				IoCancelIrp( pintfpriv->piorw_irp);
		}

	//	while (_TRUE) {
	//		if (NdisWaitEvent(&pintfpriv->io_irp_return_evt, 2000)) {
	//			break;
	//		}
	//	}
		_down_sema(&(pintfpriv->io_retevt));

	}

	
	return;

}

// Below is added by jackson



NTSTATUS usbAsynIntInComplete(PDEVICE_OBJECT pUsbDevObj, PIRP pioread_irp, PVOID pusb_cnxt)
{

	u32 i,j,r_cnt;
	_irqL irqL;
	_list	*phead, *plist;
	struct io_req	*pio_req;
	
	NTSTATUS status = STATUS_SUCCESS;
	
	struct io_queue *pio_q = (struct io_queue *) pusb_cnxt;
	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	
	_adapter *padapter = (_adapter *)pintf->adapter;

	PURB	piorw_urb = pintfpriv->piorw_urb;
	u32	*pcmd = (u32 *)(pintfpriv->io_rwmem) + 1;
	u8	bsuccess,bmatch;
	u32 false_val=0xffffffff;
	_func_enter_;
	pintfpriv->io_irp_cnt --;
	if(pintfpriv->io_irp_cnt ==0){
		//NdisSetEvent(&pintfpriv->io_irp_return_evt);
		_up_sema(&(pintfpriv->io_retevt));
	}
	pintfpriv->bio_irp_pending=_FALSE;
	if (pintfpriv->intf_status != _IO_WAIT_COMPLETE){

		return STATUS_MORE_PROCESSING_REQUIRED;
		}
	phead = &(pio_q->processing);

	_enter_critical(&(pio_q->lock), &irqL);
	
	
	pintfpriv->intf_status = _IOREADY;

	switch(pioread_irp->IoStatus.Status) {
		case STATUS_SUCCESS:
			bsuccess=_TRUE;
			break;
		default:
			bsuccess=_FALSE;
			padapter->bSurpriseRemoved=_TRUE;
			DbgPrint("usbAsynIntInComplete:pioread_irp->IoStatus.Status !=STATUS_SUCCESS==>padapter->bSurpriseRemoved=_TRUE ");
			DEBUG_ERR(("\n usbAsnyIntInComplete:pioread_irp->IoStatus.Status !=STATUS_SUCCESS\n"));
			break;
	}

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

		if (!(pio_req->command & _IO_WRITE_)) {

			if(_memcmp((pcmd+1), &(pio_req->addr), 4)){//*(pcmd+1)==pio_req->addr){//
				bmatch=_TRUE;
				DEBUG_INFO(("\n ****(same)******usbAsnyIntInComplete: pcmd=0x%.8x pio_req->addr=0x%.8x*********** \n",*(pcmd+1),pio_req->addr));
			}else{
				bmatch=_FALSE;
				DEBUG_ERR(("\n *****(not same)*****usbAsnyIntInComplete: pcmd=0x%.8x pio_req->addr=0x%.8x*********** \n",*(pcmd+1),pio_req->addr));
				DEBUG_ERR(("\n *****(not same)*****usbAsnyIntInComplete: pcmd(command)=0x%.8x *********** \n",*(pcmd)));
				for(j=0x0;j<0x80;j=j+4){
					DEBUG_ERR(("\n *****(not same)*****pintfpriv->io_r_mem(%d-%d)=0x%.8x *********** \n",j,j+4,*(u32 *)(pintfpriv->io_rwmem+j)));
				}
					
			}
			
			pcmd += 2;
			// copy the contents to the read_buf...
			if (pio_req->command & _IO_BURST_)
			{
				_memcpy(pio_req->pbuf, (u8 *)pcmd,  (pio_req->command & _IOSZ_MASK_));
				pcmd += ((pio_req->command & _IOSZ_MASK_) >> 2);
			
			}
			else
			{
				if((bsuccess==_TRUE)&&(bmatch==_TRUE)){
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

			_up_sema(&pio_req->sema);
			if(padapter->bSurpriseRemoved){
				DEBUG_ERR(("usbAsynIntInComplete :_up_sema(&pio_req->sema) "));
			}
		}
		else
		{

			if(pio_req->_async_io_callback !=NULL){								
				_exit_critical(&(pio_q->lock), &irqL);
			          pio_req->_async_io_callback(padapter, pio_req, pio_req->cnxt);
				  _enter_critical(&(pio_q->lock), &irqL);
			}
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

	_func_exit_;
	return STATUS_MORE_PROCESSING_REQUIRED;
	
}

/*
Below will only be called under call-back.
And the calling context will enter the critical
section first.
*/

void _async_protocol_read(struct io_queue *pio_q)
{

	USBD_STATUS			usbdstatus;
	PIO_STACK_LOCATION		nextStack;
	struct dvobj_priv   *pNdisCEDvice;

	struct intf_hdl *pintf = &(pio_q->intf);	
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	_adapter *padapter = (_adapter *)(pintf->adapter);

	PURB	piorw_urb = pintfpriv->piorw_urb;
	PIRP		piorw_irp  = pintfpriv->piorw_irp; 

	NTSTATUS	NtStatus = STATUS_SUCCESS;

#ifdef NDIS51_MINIPORT
	IoReuseIrp(piorw_irp, STATUS_SUCCESS);
#endif

	_func_enter_;
      pNdisCEDvice = (struct dvobj_priv   *)pintfpriv->intf_dev;      
    //  pintfpriv->RegInterruptInPipeHandle = pNdisCEDvice->regin_pipehandle;
#if 1    
	  if(padapter->pwrctrlpriv.pnp_bstop_trx==_TRUE){
		DEBUG_ERR(("\npadapter->pwrctrlpriv.pnp_bstop_trx==_TRUE\n"));
		_func_exit_;
		return;
	  }
#endif	

	pintfpriv->io_irp_cnt++;
	pintfpriv->bio_irp_pending=_TRUE;
	// Build our URB for USBD
	UsbBuildInterruptOrBulkTransferRequest(
				piorw_urb,
				sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
				pNdisCEDvice->regin_pipehandle,//pintfpriv->RegInterruptInPipeHandle,
				(PVOID)pintfpriv->io_rwmem,
				NULL, 
				pintfpriv->max_iosz, 
				USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,  
				NULL);       

	//
	// call the calss driver to perform the operation
	// pass the URB to the USB driver stack
	//
	nextStack = IoGetNextIrpStackLocation(piorw_irp);
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->Parameters.Others.Argument1 = (PURB)piorw_urb;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	IoSetCompletionRoutine(
				piorw_irp,				// irp to use
				usbAsynIntInComplete,		// routine to call when irp is done
				pio_q,				// context to pass routine
				TRUE,					// call on success
				TRUE,					// call on error
				TRUE);					// call on cancel
	
	// 
	// Call IoCallDriver to send the irp to the usb port
	//
	NtStatus	= IoCallDriver(pNdisCEDvice->pnextdevobj, piorw_irp);//IoCallDriver(pintfpriv->pUsbDevObj, piorw_irp);
	usbdstatus	= URB_STATUS(piorw_urb);

	//
	// The USB driver should always return STATUS_PENDING when
	// it receives a write irp
	//
	if ( NtStatus != STATUS_PENDING || 	USBD_HALTED(usbdstatus) ) {
	//	DEBUG_ERR(("XXX _async_protocol_read: IoCallDriver failed!!! IRP STATUS: %X\n", NtStatus) );

		if( USBD_HALTED(usbdstatus) ) {

			DEBUG_ERR(("\n _async_protocol_read(): USBD_HALTED(usbdstatus)=%x ==> set bDriverStopped TRUE!\n",USBD_HALTED(usbdstatus)) );
		}
		_func_exit_;
		return;// STATUS_UNSUCCESSFUL;
	} 
	_func_exit_;
	return;// NtStatus;






}


NTSTATUS usbAsynIntOutComplete(PDEVICE_OBJECT	pUsbDevObj, PIRP piowrite_irp, PVOID pusb_cnxt)
{
	
	_irqL irqL;
	_list	*head, *plist;//,*pnext;

	struct io_req	*pio_req;
	
	NTSTATUS status = STATUS_SUCCESS;
	
	struct io_queue *pio_q = (struct io_queue *) pusb_cnxt;
	struct intf_hdl *pintf = &(pio_q->intf);
	
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	
	_adapter *padapter = (_adapter *)pintf->adapter;


	head = &(pio_q->processing);
	_func_enter_;
	_enter_critical(&(pio_q->lock), &irqL);
	pintfpriv->io_irp_cnt--;
	if(pintfpriv->io_irp_cnt ==0){
		//NdisSetEvent(&pintfpriv->io_irp_return_evt);
		_up_sema(&(pintfpriv->io_retevt));
	}	
	pintfpriv->bio_irp_pending=_FALSE;
	switch(piowrite_irp->IoStatus.Status) {
		case STATUS_SUCCESS:
			break;
		default:
			padapter->bSurpriseRemoved=_TRUE;
			DbgPrint("usbAsynIntOutComplete:pioread_irp->IoStatus.Status !=STATUS_SUCCESS==>padapter->bSurpriseRemoved=_TRUE");
			DEBUG_ERR(("\n usbAsynIntOutComplete:pioread_irp->IoStatus.Status !=STATUS_SUCCESS\n"));
			break;
	}				

	if (pintfpriv->intf_status == _IO_WAIT_RSP) {
		// issue read irp to read response...

		pintfpriv->intf_status = _IO_WAIT_COMPLETE;
		DEBUG_INFO(("\n=====usbAsnyIntOutComplete:pintfpriv->intf_status = _IO_WAIT_COMPLETE"));
		
		_async_protocol_read(pio_q);

	}
	else if (pintfpriv->intf_status == _IO_WAIT_COMPLETE) {

		pintfpriv->intf_status = _IOREADY;

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
		
#if 0		
			if (pio_req->command & _IO_SYNC_)
				_up_sema(&pio_req->sema);
			else
				list_insert_tail(plist, &(pio_q->free_ioreqs));
#endif
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
	_func_exit_;
	return STATUS_MORE_PROCESSING_REQUIRED;

}




void _async_protocol_write(struct io_queue *pio_q)
{

	USBD_STATUS			usbdstatus;
	PIO_STACK_LOCATION		nextStack;
	struct dvobj_priv   *pdev;

	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;

	PURB	piorw_urb = pintfpriv->piorw_urb;
	PIRP		piorw_irp  = pintfpriv->piorw_irp; 
	u32 io_wrlen = pintfpriv->io_wsz << 2;

	NTSTATUS				NtStatus = STATUS_SUCCESS;


#ifdef NDIS51_MINIPORT
	IoReuseIrp(piorw_irp, STATUS_SUCCESS);
#endif

      pdev = (struct dvobj_priv   *)pintfpriv->intf_dev;      
	_func_enter_;
#if 1
	if(pdev->padapter->pwrctrlpriv.pnp_bstop_trx==_TRUE){
		DEBUG_ERR(("\npadapter->pwrctrlpriv.pnp_bstop_trx==_TRUE\n"));
		_func_exit_;
		return;
	  }
#endif	
	pintfpriv->io_irp_cnt++;
	pintfpriv->bio_irp_pending=_TRUE;
	// Build our URB for USBD
	UsbBuildInterruptOrBulkTransferRequest(
				piorw_urb,
				sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
				pdev->regout_pipehandle,//pintfpriv->RegInterruptOutPipeHandle,
				(PVOID)pintfpriv->io_rwmem,
				NULL, 
				io_wrlen, 
				0, 
				NULL);  

	//
	// call the calss driver to perform the operation
	// pass the URB to the USB driver stack
	//
	nextStack = IoGetNextIrpStackLocation(piorw_irp);
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->Parameters.Others.Argument1 = (PURB)piorw_urb;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	IoSetCompletionRoutine(
				piorw_irp,				// irp to use
				usbAsynIntOutComplete,		// routine to call when irp is done
				pio_q,				// context to pass routine
				TRUE,					// call on success
				TRUE,					// call on error
				TRUE);					// call on cancel
	
	// 
	// Call IoCallDriver to send the irp to the usb port
	//
	NtStatus	= IoCallDriver(pdev->pnextdevobj, piorw_irp);//IoCallDriver(pintfpriv->pUsbDevObj, piorw_irp);
	usbdstatus	= URB_STATUS(piorw_urb);

	//
	// The USB driver should always return STATUS_PENDING when
	// it receives a write irp
	//
	if ((NtStatus != STATUS_PENDING) || USBD_HALTED(usbdstatus) ) {

		if( USBD_HALTED(usbdstatus) ) {

			DEBUG_ERR(("_async_protocol_write():USBD_HALTED(usbdstatus)=%X!\n",USBD_HALTED(usbdstatus)) );
		}
		_func_exit_;
		return;//STATUS_UNSUCCESSFUL;
	}
	_func_exit_;
	return;// NtStatus;
	

}
void async_rd_io_callback(_adapter *padapter, struct io_req *pio_req, u8 *cnxt)
{

	 u8 width = (u8)((pio_req->command & (_IO_BYTE_|_IO_HW_ |_IO_WORD_)) >> 10);
	  _func_enter_;
       _memcpy((u8 *)pio_req->pbuf, (u8 *)&(pio_req->val), width);

	NdisMQueryInformationComplete(padapter->hndis_adapter, 
		                                          NDIS_STATUS_SUCCESS );
	_func_exit_;
       return;
}





