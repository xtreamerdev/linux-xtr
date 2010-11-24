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

#define	RTL8711_VENQT_READ	0xc0
#define	RTL8711_VENQT_WRITE	0x40

u8 read_usb_hisr(_adapter * padapter);



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

	
	_func_exit_;
	return ret;	

}






u8  read_usb_hisr_cancel(_adapter * padapter){
	
//	s8			i;
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

	return _SUCCESS;
	return _SUCCESS;
}


/*! \brief USB TS In IRP Complete Routine.
	@param Context pointer of RT_TSB
*/
static void read_usb_hisr_complete(struct urb *purb, struct pt_regs *regs) {

//	u32		tmp32;
	int 		status;
	int		bytecnt;
	 _adapter * padapter=(_adapter *)purb->context;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	struct	evt_priv	*pevt_priv = &(padapter->evtpriv);
	struct	cmd_priv	*pcmd_priv = &(padapter->cmdpriv);
	
	_func_enter_;
	//_spinlock(&(pdev->in_lock));
	pdev->ts_in_pendding= _FALSE;
	pdev->ts_in_cnt--;
	if (pdev->ts_in_cnt == 0) {
		_up_sema(&(pdev->hisr_retevt));
		DEBUG_ERR(("pdvobjpriv->tsint_cnt == 0, set interrupt_retevt!\n"));
	}
	
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
		case -ESHUTDOWN:
			padapter->bDriverStopped=_TRUE;
			DEBUG_ERR(("\n read_usb_hisr_complete: urb shutting down with status:%d \n",purb->status));
			goto exit;
		default:
			DEBUG_ERR(("\n read_usb_hisr_complete: nonzero urb status received: %d\n",purb->status));
			DEBUG_ERR(("\n purb->actual_length=%d\n",purb->actual_length));
			goto resubmit;
	}	
	if(purb->actual_length>128){
		DEBUG_ERR(("TS In size>128!!!\n"));
		goto exit;
	}
	else{
		bytecnt=(USHORT)purb->actual_length;
		if(bytecnt<7){
			DEBUG_ERR(("bytecnt must not less than 7 (3 for hisr,4 for fwclsptr) \n"));
		}
	}
		//Copy the value to CEdevice->HisrValue
	padapter->IsrContent = pdev->ts_in_buf[2]<<16;
	padapter->IsrContent |= pdev->ts_in_buf[1]<<8;
	padapter->IsrContent |= pdev->ts_in_buf[0];

	if( (padapter->IsrContent & _C2HSET )) // C2HSET
	{
		DEBUG_INFO(("\n----------read_usb_hisr_complete: _C2HSET  "));
		padapter->IsrContent  ^= _C2HSET;
		register_evt_alive(padapter);
		evt_notify_isr(pevt_priv);
		
	}	
	if( (padapter->IsrContent  & _H2CCLR)) // H2CCLR
	{
		DEBUG_ERR(("\n----------read_usb_hisr_complete: _H2CCLR  "));

		padapter->IsrContent  ^= _H2CCLR;
		cmd_clr_isr(pcmd_priv);

	}
		
/*
		DEBUG_ERR(("padapter->IsrContent = 0x%.8x pdvobjpriv->tsint_buf = 0x00%.2x%.2x%.2x",
			padapter->IsrContent,pdev->tsint_buf[pdev->tsint_idx][2],pdev->tsint_buf[pdev->tsint_idx][1],pdev->tsint_buf[pdev->tsint_idx][0]));
*/



	//_spinunlock(&(pdev->in_lock));	
	read_usb_hisr(padapter);
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

u8 read_usb_hisr(_adapter * padapter){
	
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
	if(pdev->ts_in_urb==NULL){
		pdev->ts_in_urb=usb_alloc_urb(0,GFP_ATOMIC);
		if(pdev->ts_in_urb==NULL){
			retval= _FAIL;
			goto exit;
		}
	}
	
	
	
	usb_fill_int_urb(pdev->ts_in_urb,udev,usb_rcvintpipe(udev,0xc),
		&(pdev->ts_in_buf[0]),128,read_usb_hisr_complete,
		padapter,1);

	pdev->ts_in_pendding=_TRUE; 
	
	status=usb_submit_urb(pdev->ts_in_urb,GFP_ATOMIC);
	if(status){
		DEBUG_ERR(("\n read_usb_hisr:submit urb error(status=%d)!!!!!!!\n",status));

	}
	pdev->ts_in_cnt++;

	DEBUG_INFO(("\n read_usb_hisr:(pdev->ts_in_cnts=%d)\n",pdev->ts_in_cnt));
exit:
	//_spinunlock(&(pdev->in_lock));
	_func_exit_;
	return retval;

 }
extern NDIS_STATUS usb_dvobj_init(_adapter * padapter){
//	u8 val8;
	NDIS_STATUS	status=_SUCCESS;
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;
	struct usb_device *pusbd=pdvobjpriv->pusbdev;

//	PURB						urb = NULL;

	_func_enter_;
	pdvobjpriv->padapter=padapter;
	padapter->EepromAddressSize = 6;

	  if (pusbd->speed==USB_SPEED_HIGH){
                      padapter->max_txcmd_nr = 100;
			padapter->scsi_buffer_size = 408;
		}
	  else{
			padapter->max_txcmd_nr = 13;//USB1.1 only transfer 64 bytes in one packet

										//We use the first eight bytes for communication between driver and FW
										//so...60-8 = 52; 13 txcmd is equal to 56 bytes
			padapter->scsi_buffer_size = 60;
		}

	  	



	_func_exit_;
	return status;


}
extern void usb_dvobj_deinit(_adapter * padapter){
	
//	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;

	_func_enter_;


	_func_exit_;
}


NDIS_STATUS usb_rx_init(_adapter * padapter){
#if 0
	u8 val8;
	u32 	blocksz;
	union recv_frame *prxframe;
	NDIS_STATUS 			status;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	struct intf_hdl * pintfhdl=&padapter->pio_queue->intf;
	s32 *ptr;
	u8 i;
	struct recv_priv *precvpriv = &(padapter->recvpriv);

	_func_enter_;

	//issue Rx irp to receive data
	ptr=&padapter->bDriverStopped;
	ptr=&padapter->bSurpriseRemoved;
	
	padapter->recvpriv.rx_pending_cnt=0;
	prxframe = (union recv_frame*)precvpriv->precv_frame_buf;
	for( i=0 ; i<NR_RECVFRAME; i++) {
		DEBUG_INFO(("%d receive frame = %p \n",i,prxframe));
		usb_read_port(pintfhdl,0,0,(unsigned char *) prxframe);
		prxframe++;
	}
#if 0
	prxframe=alloc_recvframe(&padapter->recvpriv.free_recv_queue);
	padapter->recvpriv.rx_pending_cnt=1;
	usb_read_port(pintfhdl,0,0,(unsigned char *) prxframe);
#endif
	//issue tsint irp to receive device interrupt
	
	pdev->ts_in_cnt=1;
	val8=read_usb_hisr(padapter);
	if(val8==_FAIL)
		status=_FAIL;
exit:
#endif
	_func_exit_;
	return 0;// status;

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

