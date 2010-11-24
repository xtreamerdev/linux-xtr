
#define _RECV_OSDEP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#include <wifi.h>
#include <rtl8711_regdef.h>
#include <recv_osdep.h>

#include <osdep_intf.h>

#include <usb_ops.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifdef PLATFORM_WINDOWS
#include "wlan_bssdef.h"
#include <ethernet.h>
#include <if_ether.h>
#include <ip.h>

#endif


//recv_osdep.c

sint	_init_recv_priv(struct recv_priv *precvpriv, _adapter *padapter)
{
	sint i;
	union recv_frame *precvframe;
	NDIS_STATUS	 status;
	sint	res=_SUCCESS;
_func_enter_;		
	memset((unsigned char *)precvpriv, 0, sizeof (struct  recv_priv));

	_spinlock_init(&precvpriv->lock);
	_init_sema(&precvpriv->recv_sema, 0);
	_init_sema(&precvpriv->terminate_recvthread_sema, 0);

	 NdisInitializeEvent(&precvpriv->recv_resource_evt);

	 precvpriv->counter=1;

	for(i=0; i<MAX_RX_NUMBLKS; i++)
		_init_queue(&precvpriv->blk_strms[i]);


	_init_queue(&precvpriv->free_recv_queue);
	_init_queue(&precvpriv->recv_pending_queue);	



	NdisAllocatePacketPoolEx(
		&status,
		&precvpriv->RxPktPoolHdl,
		NR_RECVFRAME,
		NR_RECVFRAME,					//NumberOfOverflowDescriptors 
		sizeof(PVOID) * 4);	//ProtocolReservedLength 

	if(status != NDIS_STATUS_SUCCESS) 
	{
		DEBUG_ERR(("XXX InitializeTxRxBuffer(): NdisAllocatePacketPoolEx() failed\n" ));
		precvpriv->RxPktPoolHdl = NULL;
	}

	NdisAllocateBufferPool(
		&status,
		&precvpriv->RxBufPoolHdl,
		NR_RECVFRAME);
	

	if(status != NDIS_STATUS_SUCCESS) 
	{
		DEBUG_ERR(("XXX InitializeTxRxBuffer(): NdisAllocatePacketPoolEx() failed\n" ));
		precvpriv->RxBufPoolHdl = NULL;
	}



	/*
	remember: 256 aligned
	
	Please allocate memory with the sz = (struct recv_frame) * NR_recvFRAME, 
	and initialize free_recv_frame below.
	Please also link_up all the recv_frames...
	*/

	precvpriv->pallocated_frame_buf = _malloc(NR_RECVFRAME * sizeof(union recv_frame) + RXFRAME_ALIGN_SZ);
	if(precvpriv->pallocated_frame_buf==NULL){
		res= _FAIL;
		goto exit;
	}
	memset(precvpriv->pallocated_frame_buf, 0, NR_RECVFRAME * sizeof(union recv_frame) + RXFRAME_ALIGN_SZ);

	precvpriv->precv_frame_buf = precvpriv->pallocated_frame_buf + RXFRAME_ALIGN_SZ -
							((uint) (precvpriv->pallocated_frame_buf) &(RXFRAME_ALIGN_SZ-1));


	precvframe = (union recv_frame*) precvpriv->precv_frame_buf;

#ifdef CONFIG_USB_HCI	//allocate URB memory

	precvpriv->rx_pending_cnt=1;

	_init_sema(&precvpriv->allrxreturnevt, 0);//NdisInitializeEvent(&precvpriv->allrxreturnevt);
	
	precvpriv->pallocated_urb_buf = _malloc(NR_RECVFRAME * sizeof(URB));
	if(precvpriv->pallocated_urb_buf==NULL){
		DEBUG_ERR(("_init_recv_priv:allocate pallocated_frame_buf  FAILED\n"));		
		return _FAIL;
	}
	precvpriv->pnxtallocated_urb_buf =precvpriv->pallocated_urb_buf ;
#endif

	for(i=0; i < NR_RECVFRAME ; i++)
	{
		_init_listhead(&(precvframe->u.list));

		list_insert_tail(&(precvframe->u.list), &(precvpriv->free_recv_queue.queue));

		NdisAllocatePacket(
				&status,
				&precvframe->u.hdr.pkt,
				precvpriv->RxPktPoolHdl);

		if(status != NDIS_STATUS_SUCCESS)
		{
			DEBUG_ERR(("\n NdisAllocatePacket Fail in _init_recv_priv \n\n"));
			precvframe->u.hdr.pkt = NULL;
		}
#ifdef CONFIG_USB_HCI	
		precvframe->u.hdr.irp_pending=_FALSE;
		precvframe->u.hdr.pirp=IoAllocateIrp(padapter->dvobjpriv.nextdevstacksz, FALSE);
	
		if( precvframe->u.hdr.pirp == NULL)	{
			DEBUG_ERR(("_init_recv_priv: IoAllocateIrp FAILED\n"));	
		//	return _FAIL;
		}
		precvframe->u.hdr.purb=(PURB)precvpriv->pnxtallocated_urb_buf; 
		precvpriv->pnxtallocated_urb_buf += sizeof(URB);
#endif		


            precvframe->u.hdr.adapter =padapter;
		precvframe++;
	}
	
	precvpriv->adapter = padapter;
	precvpriv->free_recvframe_cnt=NR_RECVFRAME;
exit:	
_func_exit_;
	return _TRUE;
	

}

/*
caller of _free_recv_priv have to make sure " NO ONE WILL ACCESS recv_priv "
=>>>>>>down(terminate_recvthread_sema) is true
*/
void _free_recv_priv (struct recv_priv *precvpriv)
{
	uint i;
	union recv_frame *precvframe;
_func_enter_;		

	precvframe = (union recv_frame*) precvpriv->precv_frame_buf;


	for(i=0; i < NR_RECVFRAME ; i++)
	{ 
		// free NdisFreeBuffer
		NdisFreePacket(precvframe->u.hdr.pkt);
		precvframe++;	
	}

	NdisFreeBufferPool(precvpriv->RxBufPoolHdl);
	precvpriv->RxBufPoolHdl = NULL;

	NdisFreePacketPool(precvpriv->RxPktPoolHdl);
	precvpriv->RxPktPoolHdl = NULL;

	if(precvpriv->pallocated_frame_buf)
		_mfree(precvpriv->pallocated_frame_buf, NR_RECVFRAME * sizeof(union recv_frame) + RXFRAME_ALIGN_SZ);

_func_exit_;
}


_pkt * get_os_resource(u8 *bResult)
{
_func_enter_;
	*bResult = _SUCCESS;
	return NULL;
_func_exit_;
}

void free_os_resource(_pkt *pkt)
{
_func_enter_;

_func_exit_;
}


//
//	Description:
//		Handle TKIP MIC error. 
//		For example, indicate the MIC error event to supplicant.
//
void handle_tkip_mic_err(_adapter *padapter,u8 bgroup)
{
	u32			cur_time,cnt=0;
	LARGE_INTEGER	sys_time;
	u32  diff_time ;
	PNDIS_802_11_AUTHENTICATION_EVENT	pauth_evt;
	u8	sz_ary[30];
	struct mlme_priv* pmlmepriv=&padapter->mlmepriv;
	struct security_priv *psecuritypriv=&padapter->securitypriv;

	// Initialize event to indicate up.
	memset(sz_ary, 0,sizeof(sz_ary)); //	NdisZeroMemory(szArray, sizeof(szArray));
	pauth_evt=(PNDIS_802_11_AUTHENTICATION_EVENT)&sz_ary[0];//pAuthEvent = (PAUTHENTICATION_EVENT)&szArray[0];
	pauth_evt->Status.StatusType=Ndis802_11StatusType_Authentication;
	pauth_evt->Request->Length = sizeof(NDIS_802_11_AUTHENTICATION_REQUEST);
	_memcpy(pauth_evt->Request->Bssid, (get_bssid(pmlmepriv)), ETH_ALEN);		//	NdisMoveMemory(pAuthEvent->Request.Bssid, pMgntInfo->Bssid, 6);
		
	
	NdisGetCurrentSystemTime(&sys_time);	
	cur_time=(u32)(sys_time.QuadPart/10);  // In micro-second.
	DEBUG_ERR(("\n handle_tkip_mic_err:cur_time=0x%x\n",cur_time));
	DEBUG_ERR(("\n handle_tkip_mic_err:psecuritypriv->last_mic_err_time=0x%x\n",psecuritypriv->last_mic_err_time));
	diff_time = cur_time -psecuritypriv->last_mic_err_time; // In micro-second.
	DEBUG_ERR(("\n handle_tkip_mic_err:diff_time=0x%x\n",diff_time));

	if((bgroup==_TRUE)&&(psecuritypriv->bcheck_grpkey ==_FALSE)){
		DEBUG_ERR((" handle_tkip_mic_err :bgroup=%d psecuritypriv->bcheck_grpkey=%d",bgroup,psecuritypriv->bcheck_grpkey));
		goto exit;
	}
			
	// Indication of MIC error to upper layer.
	pauth_evt->Request->Flags = NDIS_802_11_AUTH_REQUEST_PAIRWISE_ERROR;
	NdisMIndicateStatus(
				padapter->hndis_adapter,
 				NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
				(PVOID)sz_ary,
				sizeof(NDIS_802_11_STATUS_INDICATION) + sizeof(NDIS_802_11_AUTHENTICATION_REQUEST));
	NdisMIndicateStatusComplete(padapter->hndis_adapter);
	DEBUG_ERR((" ======indicate mic err======"));
	psecuritypriv->btkip_wait_report=_TRUE;
	if(diff_time >60000000) 
	{ // Fisrt MIC Error happend in 60 seconds.
		DEBUG_ERR(("\n handle_tkip_mic_err(): TKIP MIC error.\n"));
		// Update MIC error time.
		psecuritypriv->last_mic_err_time= cur_time;
	}
	else
	{ // Second MIC Error happend in 60 seconds.
		DEBUG_ERR(("\n handle_tkip_mic_err(): Second TKIP MIC error.\n"));
		// Reset MIC error time.
		psecuritypriv->last_mic_err_time= 0;
		psecuritypriv->btkip_countermeasure=_TRUE;
		psecuritypriv->btkip_countermeasure_time=cur_time;
		while(psecuritypriv->btkip_wait_report=_TRUE){
			msleep_os(10);
			cnt++;
			if(cnt>1000)
				break;
		}
		msleep_os(10);
		disassoc_cmd(padapter);
		indicate_disconnect(padapter);
		psecuritypriv->btkip_countermeasure=_TRUE;
		// Set the flag to disassocate AP after MIC failure report send to AP.
	//Sec->bToDisassocAfterMICFailureReportSend = TRUE;
		// Insert current BSSID to denied list.
	//ecInsertDenyBssidList(pSec, pMgntInfo->Bssid, CurrTime);
	}
exit:	
	return;
}


#if 1

u32 blk_read_rxpkt(_adapter *padapter,union recv_frame *precvframe){

	sint rxlen, rxlen_t, remainder;
	struct recv_stat *prxstat;
	u32 pattern = 0xffffffff;
   	uint rx_state = 1;
	u32 ret=_SUCCESS;
	u8 *phead=precvframe->u.hdr.rx_head;
	u8 *pdata=precvframe->u.hdr.rx_data;
	u8 *ptail=precvframe->u.hdr.rx_tail;
	u8 *pend=precvframe->u.hdr.rx_end;   
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
	 struct dvobj_priv *pdevobj=&padapter->dvobjpriv;

	
_func_enter_;			   


	if( (padapter->bDriverStopped ==_TRUE) || (padapter->bSurpriseRemoved == _TRUE)){
		ret= _FAIL;
		goto exit;
	}


	read_port(padapter, Host_R_FIFO, 512, phead);
       prxstat=(struct recv_stat *)phead;//get_recvframe_data(precvframe);  
       
	 if(_memcmp(pdata,&pattern,4) == _TRUE){
		DEBUG_ERR(("####recv 4 bytes of 0xff pkt"));
		ret = state_no_rx_data;
 	     	goto exit;
	}


	rxlen = prxstat->frame_length;	
	
	//notes:update tail before update pdata
	ptail = recvframe_put(precvframe, 16);    
       pdata = recvframe_pull(precvframe, 16);  

	rxlen_t = (rxlen < 496) ? rxlen : 496;
	ptail = recvframe_put(precvframe, rxlen_t); 
	rxlen -= rxlen_t;   

	rxlen_t = (rxlen >> 9) << 9;
	remainder = rxlen - rxlen_t;
	if(remainder) rxlen_t += 512;

	while(rxlen > 0)
	{
              if( rxlen_t > (sint)(pend-ptail) )
              {
             	DEBUG_ERR(("!!!! Rx pkt's len too large  = %x ;will drop pkt!!!!", rxlen_t));
                   rxlen_t = pend-ptail;
                   read_port(padapter,  Host_R_FIFO, rxlen_t, ptail);	
			
                   //don't update tail;
                   rxlen -= rxlen_t;                    				   
		      ret=_FAIL;
              }
	       else
		{
		     read_port(padapter,  Host_R_FIFO, rxlen_t, ptail);
		    //update tail
		    ptail = recvframe_put(precvframe, rxlen);	
		    rxlen = 0;	
	       }
	}
_func_exit_;	
exit:
	return ret;
}


u32 byte_read_rxpkt(_adapter *padapter,union recv_frame *precvframe){
	
	sint rxlen, rxlen_t, remainder;
	struct recv_stat *prxstat;
	u32 ret=_SUCCESS;
	u8 *phead=precvframe->u.hdr.rx_head;
	u8 *pdata=precvframe->u.hdr.rx_data;
	u8 *ptail=precvframe->u.hdr.rx_tail;
	u8 *pend=precvframe->u.hdr.rx_end;   
_func_enter_;	
       //read 16 bytes rx status
	read_port(padapter, Host_R_FIFO, 16, phead);  
       prxstat=(struct recv_stat *)phead;
	


	//get frame length	from rx status
	rxlen = prxstat->frame_length;	
		 
	//notes:update tail before update pdata
	ptail = recvframe_put(precvframe, 16);
       pdata = recvframe_pull(precvframe, 16);	
		
	
	//start to rd rxdata
	rxlen_t = (rxlen < 496) ? rxlen : 496;
	read_port(padapter,  Host_R_FIFO, rxlen_t, ptail);

       //update tail
       ptail = recvframe_put(precvframe, rxlen_t);
	rxlen -= rxlen_t;

	while(rxlen > 0)
	{
              if( rxlen > (sint)(pend-ptail) )
              {
                   rxlen_t = pend-ptail;
                   read_port(padapter,  Host_R_FIFO, rxlen_t, ptail);	

                   //don't update tail;		   
                   rxlen -= rxlen_t;                    				   
		     ret=_FAIL;
              }
	       else
		{
		     read_port(padapter,  Host_R_FIFO, rxlen, ptail);
		    //update tail
		    ptail = recvframe_put(precvframe, rxlen);		
		    rxlen = 0;	
	       }
	}

_func_exit_;	
	return ret;
}

s32 recv_entry( IN _nic_hdl	cnxt){
	_irqL irqL;
	_adapter *padapter;
	struct recv_priv *precvpriv;
	struct	mlme_priv	*pmlmepriv ;
	 struct dvobj_priv *pdev;
	struct registry_priv *registry_par;
	union recv_frame *precvframe; 
	struct recv_stat *prxstat; 
	 u8 *phead, *pdata, *ptail,*pend;    
	 uint bufferlen;
	_queue *pfree_recv_queue, *ppending_recv_queue;
	u8 blk_mode = _FALSE;
	s32 ret=_SUCCESS;
	NDIS_STATUS	 status=NDIS_STATUS_SUCCESS;
	struct intf_hdl * pintfhdl;

_func_enter_;		

#ifdef CONFIG_SDIO_HCI
		padapter=(_adapter *)cnxt;   
#else  //for usb
	precvframe=(union recv_frame *)cnxt;
	padapter=precvframe->u.hdr.adapter;
	pintfhdl=&padapter->pio_queue->intf;
#endif

	pmlmepriv = &padapter->mlmepriv;
	precvpriv = &(padapter->recvpriv);
	pdev=&padapter->dvobjpriv;
	registry_par = &(padapter->registrypriv);
	pfree_recv_queue = &(precvpriv->free_recv_queue);
	ppending_recv_queue = &(precvpriv->recv_pending_queue);


#ifdef CONFIG_SDIO_HCI		
	
	//dequeue
	precvframe = alloc_recvframe(pfree_recv_queue);	

       if (precvframe == NULL)
       {    
       	DEBUG_ERR(("alloc_recvframe failed!!"));
       	goto _recv_entry_drop;      
       }
	   
	
	//init recv_frame
	init_recvframe(precvframe, precvpriv);     

	if ((registry_par->chip_version >= RTL8711_3rdCUT) || (registry_par->chip_version == RTL8711_FPGA))
	{
		if(pdev->block_mode==1) 
	{
		#ifdef RD_BLK_MODE 
		blk_mode = _TRUE;
		#endif
	}   
	}   

	if(blk_mode)
		ret=blk_read_rxpkt(padapter,precvframe);
	
	else
		ret=byte_read_rxpkt(padapter,precvframe);

	if((ret==_FAIL) || (ret==state_no_rx_data))
		goto _recv_entry_drop; 
	
#endif	

	phead=precvframe->u.hdr.rx_head;
	pdata=precvframe->u.hdr.rx_data;
	ptail=precvframe->u.hdr.rx_tail;
	pend=precvframe->u.hdr.rx_end;  
	prxstat=(struct recv_stat *)phead;
	
#ifdef CONFIG_MP_INCLUDED	
       if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE)// && 
	   	/*(check_fwstate(pmlmepriv, WIFI_MP_LPBK_STATE) != _TRUE)*/ )	   	
       {
           padapter->mppriv.rx_pktcount++;
		   
           if(prxstat->crc32)
		padapter->mppriv.rx_crcerrpktcount++;
		
           _memcpy(&padapter->mppriv.rxstat, prxstat, sizeof(struct recv_stat));
       }
	   
#endif	

       //remove 4 bytes FCS
       ptail = recvframe_pull_tail(precvframe, 4);	
       bufferlen = ptail -pdata;
	
       //todo: check frame length, get len by using macro
       if(precvframe->u.hdr.len != (prxstat->frame_length-4))
      {
                 DEBUG_ERR(("recv_entry:%d, %d\n", precvframe->u.hdr.len, prxstat->frame_length));
		   ret = _FAIL;		 
		   goto _recv_entry_drop;
	}

	if((prxstat->decrypted==1)&&(prxstat->icv==1))
	{
		DEBUG_ERR(("prxstat->icv=%d  Drop packet",prxstat->icv));
		if(padapter->securitypriv.dot11PrivacyAlgrthm!=_AES_){
			//	DbgPrint("prxstat->decrypted =%d prxstat->icv=%d  Drop packet",prxstat->decrypted,prxstat->icv);
			//	DbgPrint("prxstat->frag =%d mfrag=0x%.2x ",prxstat->frag,*(pdata+1));
			ret = _FAIL;		 
		   	goto _recv_entry_drop;
		}
	}

       //todo: check if  free_recv_queue is empty after dequeue one 
       //        if yes, enqueue back to  free_recv_queue after read hw rx fifo data, and reuse ndispacket
#ifdef CONFIG_USB_HCI
	_enter_critical(&ppending_recv_queue->lock, &irqL);

	if(_queue_empty(ppending_recv_queue) == _TRUE) {
		//enqueue to recv_pending_queue
		enqueue_recvframe_usb(precvframe, ppending_recv_queue);
		_up_sema(&precvpriv->recv_sema);
	}
	else 
		//enqueue to recv_pending_queue
		enqueue_recvframe_usb(precvframe, ppending_recv_queue);
	
	_exit_critical(&ppending_recv_queue->lock, &irqL);
#else
	if(precvpriv->free_recvframe_cnt >1){
#ifdef  use_recv_func	
	  recv_func(padapter, precvframe);
#else
          //enqueue to recv_pending_queue
	   enqueue_recvframe(precvframe, ppending_recv_queue);
		  
          //todo: _up_sema in caller
	   _up_sema(&precvpriv->recv_sema);          
#endif	
	   precvpriv->rx_pkts++;       
	}
	else
		goto _recv_entry_drop;
#endif
_func_exit_;		
	   return ret;

	
_recv_entry_drop:

#ifdef	CONFIG_USB_HCI
	usb_read_port(pintfhdl, 0, 0, (unsigned char *)precvframe);
#else
	if(precvframe)//enqueue back to  free_recv_queue
		free_recvframe(precvframe, pfree_recv_queue);
#endif

	precvpriv->rx_drop++;	
#ifdef CONFIG_MP_INCLUDED	
	padapter->mppriv.rx_pktloss = precvpriv->rx_drop;
#endif
       DEBUG_ERR(("_recv_entry_drop\n"));
_func_exit_;	
	return ret;

}



#else


//#ifndef RD_BLK_MODE 
NDIS_STATUS recv_entry_byte_md( IN _nic_hdl	cnxt)	
{


	 struct recv_priv *precvpriv;
       _queue *pfree_recv_queue, *ppending_recv_queue;	
       union recv_frame *precvframe;       
       u8 *phead, *pdata, *ptail,*pend;      
	struct recv_stat *prxstat; 
	uint rxlen, bufferlen, rxlen_t;
	NDIS_STATUS	 status=NDIS_STATUS_SUCCESS;
	_adapter *padapter=(_adapter *)cnxt;       
	struct	mlme_priv	*pmlmepriv = &padapter->mlmepriv;
       u8 bdrop=_FALSE;		
       

_func_enter_;		

	precvpriv = &(padapter->recvpriv);


	pfree_recv_queue = &(precvpriv->free_recv_queue);
	ppending_recv_queue = &(precvpriv->recv_pending_queue);
	
	//dequeue
	precvframe = alloc_recvframe(pfree_recv_queue);
       if (precvframe == NULL)
       {    
       	DEBUG_INFO(("alloc_recvframe failed!!"));
       	goto _recv_entry_drop;      
       }
	   
	
	//init recv_frame
	init_recvframe(precvframe, precvpriv);     

	phead=precvframe->u.hdr.rx_head;
       pdata=precvframe->u.hdr.rx_data;
       ptail=precvframe->u.hdr.rx_tail;
       pend=precvframe->u.hdr.rx_end;  

	
       //read 16 bytes rx status
	read_port(padapter, Host_R_FIFO, 16, phead);  
       prxstat=(struct recv_stat *)get_recvframe_data(precvframe);
	
       //update_rxstatus(padapter, precvframe, prxstat );
#ifdef CONFIG_MP_INCLUDED	
       if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE)// && 
	   	/*(check_fwstate(pmlmepriv, WIFI_MP_LPBK_STATE) != _TRUE)*/ )	   	
       {
           padapter->mppriv.rx_pktcount++;
		   
           if(prxstat->crc32)
		padapter->mppriv.rx_crcerrpktcount++;

           _memcpy(&padapter->mppriv.rxstat, prxstat, sizeof(struct recv_stat));

           //DEBUG_ERR("mp_mode:recv_entry()\n");		   

           //goto _recv_entry_drop;	    
       }
	   
#endif	

	//get frame length	from rx status
	rxlen = prxstat->frame_length;	
		 
	//notes:update tail before update pdata
	ptail = recvframe_put(precvframe, 16);
       pdata = recvframe_pull(precvframe, 16);	      
		
	
	//start to rd rxdata
	rxlen_t = (rxlen < 496) ? rxlen : 496;
	read_port(padapter,  Host_R_FIFO, rxlen_t, ptail);

       //update tail
       ptail = recvframe_put(precvframe, rxlen_t);
	rxlen -= rxlen_t;

	while(rxlen > 0)
	{
              if( rxlen > (unsigned int)(pend-ptail) )
              {
                   rxlen_t = pend-ptail;
                   read_port(padapter,  Host_R_FIFO, rxlen_t, ptail);	

                   //don't update tail;		   
                   rxlen -= rxlen_t;                    				   
		     bdrop=_TRUE;
              }
	       else
		{
		     read_port(padapter,  Host_R_FIFO, rxlen, ptail);
		    //update tail
		    ptail = recvframe_put(precvframe, rxlen);		
		    rxlen = 0;	
	       }
	}

	/* Perry:
		If we receive packet which more = 0, it means we can push driver to sleep mode.
		Also, we must make sure that 
	*/
	if (prxstat->mdata == 0)
		unregister_rx_alive(padapter);

	if(bdrop==_TRUE)
	     goto _recv_entry_drop;

       //remove 4 bytes FCS
       ptail = recvframe_pull_tail(precvframe, 4);	
       bufferlen = ptail -pdata;
	
       //todo: check frame length, get len by using macro
       if(precvframe->u.hdr.len != (prxstat->frame_length-4))
      {
                 DEBUG_ERR(("recv_entry:%d, %d\n", precvframe->u.hdr.len, prxstat->frame_length));
		   goto _recv_entry_drop;
	}
       //     goto _recv_entry_drop;	   
	//todo:check if ptail > pend 	

      //if ((if_up((unsigned char*)padapter)) == _FALSE)
      //     goto _recv_entry_drop;

       //todo: check if  free_recv_queue is empty after dequeue one 
       //        if yes, enqueue back to  free_recv_queue after read hw rx fifo data, and reuse ndispacket

	if(precvpriv->free_recvframe_cnt >1){
          //enqueue to recv_pending_queue
	   enqueue_recvframe(precvframe, ppending_recv_queue);
		  
          //todo: _up_sema in caller
	   _up_sema(&precvpriv->recv_sema);          
	
	   precvpriv->rx_pkts++;       
	}
	else
		goto _recv_entry_drop;
_func_exit_;		
	   return ret;

	
_recv_entry_drop:


	if(precvframe)//enqueue back to  free_recv_queue
		free_recvframe(precvframe, pfree_recv_queue);

	precvpriv->rx_drop++;	
#ifdef CONFIG_MP_INCLUDED	
	padapter->mppriv.rx_pktloss = precvpriv->rx_drop;
#endif
       DEBUG_ERR(("_recv_entry_drop\n"));
_func_exit_;	
	return status;

}



//#else

//while(_Rdy || (SdioDevice->RXPktCount > 0))
//_up_sema(&precvpriv->recv_sema);
NDIS_STATUS recv_entry_blk_md( IN _nic_hdl	cnxt)	
{


	 struct recv_priv *precvpriv;
       _queue *pfree_recv_queue, *ppending_recv_queue;	
       union recv_frame *precvframe = NULL;       
       u8 *phead, *pdata, *ptail,*pend;      
	struct recv_stat *prxstat; 
	uint rxlen, bufferlen, rxlen_t, remainder;
	NDIS_STATUS	 status=NDIS_STATUS_SUCCESS;
	_adapter *padapter=(_adapter *)cnxt;       
	struct	mlme_priv	*pmlmepriv = &padapter->mlmepriv;
       u8 bdrop=_FALSE;		
    	u32 pattern = 0xffffffff;
   	uint rx_state = 1;


_func_enter_;		

	precvpriv = &(padapter->recvpriv);

	pfree_recv_queue = &(precvpriv->free_recv_queue);
	ppending_recv_queue = &(precvpriv->recv_pending_queue);
	
	//dequeue
	precvframe = alloc_recvframe(pfree_recv_queue);
       if (precvframe == NULL)
       {    
       	DEBUG_INFO(("alloc_recvframe failed!!"));
       	goto _recv_entry_drop;      
       }
	   	
	//init recv_frame
	init_recvframe(precvframe, precvpriv);     

	phead=precvframe->u.hdr.rx_head;
       pdata=precvframe->u.hdr.rx_data;
       ptail=precvframe->u.hdr.rx_tail;
       pend=precvframe->u.hdr.rx_end;  

	
	 read_port(padapter, Host_R_FIFO, 512, phead);
       prxstat=(struct recv_stat *)phead;//get_recvframe_data(precvframe);
       
	 if(_memcmp(pdata,&pattern,4) == _TRUE){
	 	rx_state = state_no_rx_data;
		DEBUG_ERR(("~~~recv 4 bytes of 0xff pkt"));
		/*if(_memcmp(pdata+4,&pattern,4) == _FALSE)
		{
			write32(padapter, SCRAM0, 0);
			DEBUG_ERR(("#############however the4~7 bytes is not 0xff"));
		}*/
 	     	goto _recv_entry_drop;
	}


	rxlen = prxstat->frame_length;	
	
	//notes:update tail before update pdata
	ptail = recvframe_put(precvframe, 16);
       pdata = recvframe_pull(precvframe, 16);

	rxlen_t = (rxlen < 496) ? rxlen : 496;
	ptail = recvframe_put(precvframe, rxlen_t);
	rxlen -= rxlen_t;   

	rxlen_t = (rxlen >> 9) << 9;
	remainder = rxlen - rxlen_t;
	if(remainder) rxlen_t += 512;

	while(rxlen > 0)
	{
              if( rxlen_t > (unsigned int)(pend-ptail) )
              {
             	DEBUG_ERR(("!!!! Rx pkt's len too large  = %x ;will drop pkt!!!!", rxlen_t));
                   rxlen_t = pend-ptail;
                   read_port(padapter,  Host_R_FIFO, rxlen_t, ptail);	
			
                   //don't update tail;
                   rxlen -= rxlen_t;                    				   
		      bdrop=_TRUE;
              }
	       else
		{
		     read_port(padapter,  Host_R_FIFO, rxlen_t, ptail);
		    //update tail
		    ptail = recvframe_put(precvframe, rxlen);	
		    rxlen = 0;	
	       }
	}

	/* Perry:
		If we receive packet which more = 0, it means we can push driver to sleep mode.
		Also, we must make sure that 
	*/
	if (prxstat->mdata == 0)
		unregister_rx_alive(padapter);

	if(bdrop==_TRUE)
	     goto _recv_entry_drop;

       //remove 4 bytes FCS
       ptail = recvframe_pull_tail(precvframe, 4);	
       bufferlen = ptail -pdata;
	
       //todo: check frame length, get len by using macro
       if(precvframe->u.hdr.len != (prxstat->frame_length-4))
      {
            DEBUG_ERR(("recv_entry:%d, %d\n", precvframe->u.hdr.len, prxstat->frame_length));
		goto _recv_entry_drop;
	}
       //     goto _recv_entry_drop;
	//todo:check if ptail > pend 	

      //if ((if_up((unsigned char*)padapter)) == _FALSE)
      //     goto _recv_entry_drop;

       //todo: check if  free_recv_queue is empty after dequeue one 
       //        if yes, enqueue back to  free_recv_queue after read hw rx fifo data, and reuse ndispacket

	if(precvpriv->free_recvframe_cnt >1){
          //enqueue to recv_pending_queue
	   enqueue_recvframe(precvframe, ppending_recv_queue);
		  
          //todo: _up_sema in caller
	   _up_sema(&precvpriv->recv_sema);          
	
	   precvpriv->rx_pkts++;       	
	}
	else
		goto _recv_entry_drop;
_func_exit_;
	   return status;

	
_recv_entry_drop:


	if(precvframe)//enqueue back to  free_recv_queue
		free_recvframe(precvframe, pfree_recv_queue);

	precvpriv->rx_drop++;	
#ifdef CONFIG_MP_INCLUDED	
	padapter->mppriv.rx_pktloss = precvpriv->rx_drop;
#endif
       DEBUG_ERR(("_recv_entry_drop\n"));
_func_exit_;	
	return status;

}


NDIS_STATUS recv_entry( IN _nic_hdl	cnxt)
{
	_adapter *padapter=(_adapter *)cnxt;       
	 struct dvobj_priv *psddev=&padapter->dvobjpriv;
	struct registry_priv *registry_par = &(padapter->registrypriv);
	u8 blk_mode = _FALSE;
	if ((registry_par->chip_version >= RTL8711_3rdCUT) && (psddev->block_mode==1) )
	{
		#ifdef RD_BLK_MODE 
		blk_mode = _TRUE;
		#endif
	}


	if(blk_mode)
		return recv_entry_blk_md(cnxt)	;
	else
		return recv_entry_byte_md(cnxt)	;

}

#endif

void recv_indicatepkt(_adapter *padapter, union recv_frame *precv_frame, _pkt *pkt)
{
	_irqL	irqL;
	struct recv_priv *precvpriv;
	_queue	*pfree_recv_queue;		
	unsigned char *pdata;      
	unsigned int bufferlen;
	_pkt *precv_ndispkt;
	struct iphdr *piphdr;
	struct ethhdr *pethhdr;
	u8 bfreed=_FALSE;
	NDIS_STATUS	 status=NDIS_STATUS_SUCCESS;
	_buffer *pndisbuffer=NULL;
	_pkt *pndispkt_free=NULL;	
_func_enter_;
	precvpriv = &(padapter->recvpriv);	
	pfree_recv_queue = &(precvpriv->free_recv_queue);	
	pdata=get_recvframe_data(precv_frame);	
	bufferlen=precv_frame->u.hdr.len;
	precv_ndispkt=precv_frame->u.hdr.pkt;	

        //check if precv_frame->u.hdr.pkt == NULL
	if(precv_ndispkt == NULL)
	{
		status=NDIS_STATUS_RESOURCES ;
		DEBUG_ERR(("recv_indicatepkt():failed to allocate NdisPacket!\n"));
		goto _recv_indicatepkt_drop;
	}

	NDIS_SET_PACKET_HEADER_SIZE(precv_ndispkt, NIC_HEADER_SIZE);
	    
	NdisAllocateBuffer(&status,
	                           &pndisbuffer,
	                           precvpriv->RxBufPoolHdl,
	                           (void*)pdata,
	                           bufferlen);

	if(status != NDIS_STATUS_SUCCESS)
	{
	    DEBUG_ERR(("recv_indicatepkt():failed to allocate NdisBuffer!\n"));
	    goto _recv_indicatepkt_drop;
	}

	NdisChainBufferAtBack(precv_ndispkt, pndisbuffer);                         
	
	pethhdr = (struct ethhdr *)pdata;
	
	piphdr = (struct iphdr *)(pdata + 14);
	if(precvpriv->rx_pending_cnt<5) {
		NDIS_SET_PACKET_STATUS(precv_ndispkt, NDIS_STATUS_RESOURCES);	
		bfreed=_TRUE;
	}
	else {
		NDIS_SET_PACKET_STATUS(precv_ndispkt, NDIS_STATUS_SUCCESS);
	}

	_enter_critical(&precvpriv->lock, &irqL);
	precvpriv->counter++;
	_exit_critical(&precvpriv->lock, &irqL);
    
	NdisMIndicateReceivePacket(padapter->hndis_adapter, &precv_ndispkt, 1); 
	if(bfreed==_TRUE)
		recv_returnpacket(padapter, precv_ndispkt);
_func_exit_;		
        return;		

_recv_indicatepkt_drop:

#ifdef CONFIG_USB_HCI
        usb_read_port(pintfhdl,0,0,(unsigned char *)precv_frame);
#else
        //todo: enqueue back to free_recv_queue 
        if(precv_frame)
                free_recvframe(precv_frame, pfree_recv_queue);
#endif
	 
	precvpriv->rx_drop++;		
_func_exit_;
}

void recv_returnpacket(IN _nic_hdl cnxt, IN _pkt *preturnedpkt)
{
	_irqL irqL;

       struct recv_priv *precvpriv;
       _queue	*pfree_recv_queue;
       union recv_frame *precv_frame_returned;	
	_adapter *padapter= (_adapter *)cnxt;
	_buffer *pndisbuffer=NULL;
	struct intf_hdl * pintfhdl=&padapter->pio_queue->intf;
_func_enter_;       
	precvpriv = &(padapter->recvpriv);	
	pfree_recv_queue = &(precvpriv->free_recv_queue);

	//get recv_frame		
	precv_frame_returned = pkt_to_recvframe(preturnedpkt);	
		
	do{
		NdisUnchainBufferAtFront(preturnedpkt, &pndisbuffer);
		if(pndisbuffer==NULL)
			break;
		NdisFreeBuffer(pndisbuffer);
	}while(_TRUE);

	//NdisFreePacket(preturnedpkt);

#ifdef CONFIG_USB_HCI
	//after packet return, submit urb
	usb_read_port(pintfhdl,0, 0, (unsigned char *)precv_frame_returned);
#else
	//todo: enqueue back to free_recv_queue	 	
       free_recvframe(precv_frame_returned, pfree_recv_queue);
#endif

	 _enter_critical(&precvpriv->lock, &irqL);
               
		precvpriv->counter--;
	
		if(precvpriv->counter==0)			
		{
			NdisSetEvent(&precvpriv->recv_resource_evt);
		}

	 _exit_critical(&precvpriv->lock, &irqL);
_func_exit_;

}

void alloc_os_rxpkt(union recv_frame *precvframe ,struct recv_priv *precvpriv)
{
	NDIS_STATUS		status;
_func_enter_;
	if(precvframe->u.hdr.pkt == NULL)
	{
		DEBUG_ERR(("Error: pkt==NULL\n\n"));
		NdisAllocatePacket(
				&status,
				&precvframe->u.hdr.pkt,
				&precvpriv->RxPktPoolHdl);

		if(status != NDIS_STATUS_SUCCESS)
		{
			DEBUG_ERR(("Error: pkt==NULL\n\n"));
			precvframe->u.hdr.pkt = NULL;
		}
	}
	
	precvframe->u.hdr.len=0;
	
	precvframe->u.hdr.rx_head = precvframe->u.hdr.rx_data = precvframe->u.hdr.rx_tail 
				= (u8*)precvframe +  RECVFRAME_HDR_ALIGN;

	precvframe->u.hdr.rx_end = (u8*)precvframe + MAX_RXSZ; 
		
_func_exit_;
}

