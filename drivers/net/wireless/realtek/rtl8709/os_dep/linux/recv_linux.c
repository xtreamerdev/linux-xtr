/******************************************************************************
* recv_linux.c                                                                                                                                 *
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
#define _RECV_OSDEP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#include <wifi.h>
//#include <rtl8711_regdef.h>
#include <recv_osdep.h>

#include <osdep_intf.h>
#include <Ethernet.h>

#include <usb_ops.h>
#include <wlan.h>


#ifdef PLATFORM_WINDOWS

#error "PLATFORM_WINDOWS should not be defined!\n"

#endif

//recv_osdep.c

sint	_init_recv_priv(struct recv_priv *precvpriv, _adapter *padapter)
{
	sint i;
	union recv_frame *precvframe;
//	NDIS_STATUS	 status;
	sint	res=_SUCCESS;
_func_enter_;		
	memset((unsigned char *)precvpriv, 0, sizeof (struct  recv_priv));

	_spinlock_init(&precvpriv->lock);
	_init_sema(&precvpriv->recv_sema, 0);
	_init_sema(&precvpriv->terminate_recvthread_sema, 0);

	 precvpriv->counter=1;

	for(i=0; i<MAX_RX_NUMBLKS; i++)
		_init_queue(&precvpriv->blk_strms[i]);


	_init_queue(&precvpriv->free_recv_queue);
	_init_queue(&precvpriv->recv_pending_queue);	


	/*
	remember: 2048 aligned
	
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

	for(i=0; i < NR_RECVFRAME ; i++)
	{
		_init_listhead(&(precvframe->u.list));

		//list_insert_tail(&(precvframe->u.list), &(precvpriv->free_recv_queue.queue));
		precvframe->u.hdr.adapter =padapter;
#ifdef CONFIG_USB_HCI
		//precvframe->u.hdr.purb=usb_alloc_urb(0,GFP_ATOMIC);
		precvframe->u.hdr.purb=usb_alloc_urb(0,GFP_KERNEL);
#endif
		precvframe->u.hdr.pkt = alloc_skb(SZ_RECVFRAME, GFP_ATOMIC);
		precvframe->u.hdr.fragcnt=0;
		precvframe++;
	}
	precvpriv->RxPktPoolHdl = NULL;
	precvpriv->RxBufPoolHdl = NULL;
	precvpriv->adapter = padapter;
	
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
		// free SKBuffer
		if(precvframe->u.hdr.pkt !=NULL)
			dev_kfree_skb_any(precvframe->u.hdr.pkt);
#ifdef CONFIG_USB_HCI
		if(precvframe->u.hdr.purb !=NULL){
			usb_kill_urb(precvframe->u.hdr.purb);
			usb_free_urb(precvframe->u.hdr.purb);
		}
#endif
		precvframe++;	
	}
	if(precvpriv->RxBufPoolHdl != NULL)
		DEBUG_ERR(("\n precvpriv->RxBufPoolHdl != NULL , something rwrong!!! \n"));

	if(precvpriv->RxPktPoolHdl != NULL)
		DEBUG_ERR(("\n precvpriv->RxPktPoolHdl != NULL , something rwrong!!! \n"));

	if(precvpriv->pallocated_frame_buf)
		_mfree(precvpriv->pallocated_frame_buf, NR_RECVFRAME * sizeof(union recv_frame)+RXFRAME_ALIGN_SZ );

_func_exit_;
}

_pkt *get_os_resource(u8 *bResult)
{
	_pkt *pkt = NULL;
_func_enter_;
#ifdef CONFIG_USB_HCI

	//allocate skb buff for next usb_read_port 
	//if kernel has no space to allocate, reuse it
	pkt = alloc_skb(SZ_RECVFRAME, GFP_ATOMIC);

	if(pkt){
		*bResult = _SUCCESS;
	}
	else {
		DEBUG_INFO(("No Space for allocate skb buff \n"));
		*bResult = _FAIL;
	}
#endif
#ifdef CONFIG_SDIO_HCI
	*bResult = _SUCCESS;
#endif
_func_exit_;
	return pkt;
}

void free_os_resource(_pkt *pkt)
{
_func_enter_;
	dev_kfree_skb_any(pkt);
_func_exit_;
}


void handle_tkip_mic_err(_adapter *padapter,u8 bgroup)
{
_func_enter_;

_func_exit_;
}



#ifdef CONFIG_HCI_SDIO
u32 blk_read_rxpkt(_adapter *padapter,union recv_frame *precvframe){

	sint rxlen, rxlen_t, remainder;
	struct recv_stat *prxstat;
	struct recv_test *prxtest;//
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
	
       prxstat=(struct recv_stat *)phead;
	prxtest=(struct recv_test *)phead;
   

#ifdef CONFIG_BIG_ENDIAN
prxstat ->frame_length = prxtest->frame_length1+(prxtest->frame_length2<<8);
prxstat->ack_policy = prxtest->ack_policy;
prxstat->addr_hit = prxtest->addr_hit;
prxstat->agc = prxtest->agc;
prxstat->antenna = prxtest->antenna;
prxstat->cfo = prxtest->cfo;
prxstat->crc32 = prxtest->crc32;
prxstat->data = prxtest->data;
prxstat->decrypted = prxtest->decrypted;
prxstat->eosp = prxtest->eosp;
prxstat->fot = prxtest->fot1+(prxtest->fot2<<8);
prxstat->frag = prxtest->frag;
prxstat->icv = prxtest->icv;
prxstat->k_int = prxtest->k_int;
prxstat->mdata = prxtest->mdata;
prxstat->mgt = prxtest->mgt;
prxstat->no_mcsi = prxtest->no_mcsi1+ (prxtest->no_mcsi2<< 2);
prxstat->own = prxtest->own;
prxstat->pwdb = prxtest->pwdb;
prxstat->pwrmgt = prxtest->pwrmgt;
prxstat->qos = prxtest->qos;
prxstat->queue_size = prxtest->queue_size;
prxstat->retx = prxtest->retx;
prxstat->rssi = prxstat->rssi;
prxstat->rxdata = prxstat->rxdata;
prxstat->rxsnr = prxtest->rxsnr;
prxstat->seq = prxtest->seq1 +(prxtest->seq2<<8);
prxstat->splcp = prxtest->splcp;
prxstat->sq = prxtest->sq;
prxstat->tid = prxtest->tid;
prxstat->txmac_id = prxtest->txmac_id;
#endif
	   
       
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
              if( rxlen_t > (sint)(pend-ptail) )//?
              {
                   //diag_printf("blk_read_rxpkt-1, rxlen_t=%d, rxlen=%d, pend=%x, ptail=%x, end-tail=%d\n", rxlen_t, rxlen,pend, ptail, (pend-ptail));			               	
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


u32 byte_read_rxpkt(_adapter *padapter, union recv_frame *precvframe){
	
	sint rxlen, rxlen_t, remainder;
	struct recv_stat *prxstat;
	struct recv_test *prxtest;//
	u32 ret=_SUCCESS;
	u8 *phead=precvframe->u.hdr.rx_head;
	u8 *pdata=precvframe->u.hdr.rx_data;
	u8 *ptail=precvframe->u.hdr.rx_tail;
	u8 *pend=precvframe->u.hdr.rx_end;   
	
_func_enter_;	
       //read 16 bytes rx status
	read_port(padapter, Host_R_FIFO, 16, phead);  
       prxstat=(struct recv_stat *)phead;
	prxtest=(struct recv_test *)phead;
   

#ifdef CONFIG_BIG_ENDIAN
prxstat ->frame_length = prxtest->frame_length1+(prxtest->frame_length2<<8);
prxstat->ack_policy = prxtest->ack_policy;
prxstat->addr_hit = prxtest->addr_hit;
prxstat->agc = prxtest->agc;
prxstat->antenna = prxtest->antenna;
prxstat->cfo = prxtest->cfo;
prxstat->crc32 = prxtest->crc32;
prxstat->data = prxtest->data;
prxstat->decrypted = prxtest->decrypted;
prxstat->eosp = prxtest->eosp;
prxstat->fot = prxtest->fot1+(prxtest->fot2<<8);
prxstat->frag = prxtest->frag;
prxstat->icv = prxtest->icv;
prxstat->k_int = prxtest->k_int;
prxstat->mdata = prxtest->mdata;
prxstat->mgt = prxtest->mgt;
prxstat->no_mcsi = prxtest->no_mcsi1+ (prxtest->no_mcsi2<< 2);
prxstat->own = prxtest->own;
prxstat->pwdb = prxtest->pwdb;
prxstat->pwrmgt = prxtest->pwrmgt;
prxstat->qos = prxtest->qos;
prxstat->queue_size = prxtest->queue_size;
prxstat->retx = prxtest->retx;
prxstat->rssi = prxstat->rssi;
prxstat->rxdata = prxstat->rxdata;
prxstat->rxsnr = prxtest->rxsnr;
prxstat->seq = prxtest->seq1 +(prxtest->seq2<<8);
prxstat->splcp = prxtest->splcp;
prxstat->sq = prxtest->sq;
prxstat->tid = prxtest->tid;
prxstat->txmac_id = prxtest->txmac_id;
#endif


	//get frame length	from rx status
	rxlen = prxstat->frame_length;	
	//diag_printf("byte_read_rxpkt-0, rxlen=%d, pend=%x, ptail=%x, end-tail=%d\n", rxlen,pend, ptail, (pend-ptail));			 
	
		 
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
                   //diag_printf("byte_read_rxpkt-1, rxlen=%d, pend=%x, ptail=%x, end-tail=%d\n", rxlen,pend, ptail, (pend-ptail));			 
              }
	       else
		{
                  //diag_printf("byte_read_rxpkt-2, rxlen=%d, pend=%x, ptail=%x, end-tail=%d\n", rxlen,pend, ptail, (pend-ptail));			  			
		     read_port(padapter,  Host_R_FIFO, rxlen, ptail);
		    //update tail
		    ptail = recvframe_put(precvframe, rxlen);	
                  //diag_printf("byte_read_rxpkt-3, rxlen=%d, pend=%x, ptail=%x, end-tail=%d\n", rxlen,pend, ptail, (pend-ptail));			 
              			
		    rxlen = 0;	
	       }
	}

_func_exit_;	

	return ret;
	
}
#endif


s32 recv_entry( IN _nic_hdl	cnxt)	
{
#ifndef RX_FUNCTION
	_irqL irqL;
#endif /*RX_FUNCTION*/

	_adapter *padapter;
	struct recv_priv *precvpriv;
	union recv_frame *precvframe;       
	struct recv_stat *prxstat;
	u8 *phead, *pdata, *ptail,*pend;      
	_pkt *precv_ndispkt; 
	uint  bufferlen;
	_queue *pfree_recv_queue, *ppending_recv_queue;
	s32 ret=_SUCCESS;
	struct intf_hdl * pintfhdl;

_func_enter_;		

        DEBUG_INFO(("\n ==>recv_entry!!!!!!!!!!!! \n"));	
#ifdef CONFIG_SDIO_HCI
	padapter=(_adapter *)netdev_priv(cnxt);
	pintfhdl=&padapter->pio_queue->intf;
#endif
#ifdef	CONFIG_USB_HCI
	precvframe=(union recv_frame *)cnxt;
	padapter=precvframe->u.hdr.adapter;
	pintfhdl=&padapter->pio_queue->intf;
#endif

	precvpriv = &(padapter->recvpriv);
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
		ret=blk_read_rxpkt(padapter, precvframe);
	
	else
		ret=byte_read_rxpkt(padapter, precvframe);

	if((ret==_FAIL) || (ret==state_no_rx_data))
	{
		//diag_printf("recv_entry-1\, ret=%d\n", ret);	
		goto _recv_entry_drop; 

	}

#endif /*CONFIG_SDIO_HCI*/

	phead=precvframe->u.hdr.rx_head;
	pdata=precvframe->u.hdr.rx_data;
	ptail=precvframe->u.hdr.rx_tail;
	pend=precvframe->u.hdr.rx_end;  
	precv_ndispkt=precvframe->u.hdr.pkt;

	//remove 4 bytes FCS
	ptail = recvframe_pull_tail(precvframe, 4);	
	prxstat = get_recv_stat(precvframe);

	bufferlen = ptail - pdata;


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


#if 0
	/* Perry:
		If we receive packet which more = 0, it means we can push driver to sleep mode.
		Also, we must make sure that 
	*/
	if (prxstat->mdata == 0)
		unregister_rx_alive(padapter);
#endif

       //todo: check frame length, get len by using macro
       if(precvframe->u.hdr.len != (prxstat->frame_length-4))
	{
		DEBUG_ERR(("recv_entry:precvframe->u.hdr.len != (prxstat->frame_length-4)%d, %d\n", precvframe->u.hdr.len, prxstat->frame_length));
		goto _recv_entry_drop;
	}
       //     goto _recv_entry_drop;	   
	//todo:check if ptail > pend 	

       if((prxstat->decrypted==1)&&(prxstat->icv==1))
       {
              DEBUG_ERR(("prxstat->decrypted =%d prxstat->icv=%d  Drop packet\n",prxstat->decrypted,prxstat->icv));
              if(padapter->securitypriv.dot11PrivacyAlgrthm!=_AES_){
                     //DbgPrint("prxstat->decrypted =%d prxstat->icv=%d  Drop packet",prxstat->decrypted,prxstat->icv);
                     //DbgPrint("prxstat->frag =%d mfrag=0x%.2x ",prxstat->frag,*(pdata+1));
                     ret = _FAIL;		 
                     goto _recv_entry_drop;
              }
       }

       //todo: check if  free_recv_queue is empty after dequeue one 
       //        if yes, enqueue back to  free_recv_queue after read hw rx fifo data, and reuse ndispacket
#ifdef CONFIG_USB_HCI

#ifdef RX_FUNCTION

	recv_function(precvframe);

#else
	_enter_critical(&ppending_recv_queue->lock, &irqL);

	if(_queue_empty(ppending_recv_queue) == _TRUE) {
		//enqueue to recv_pending_queue
		enqueue_recvframe_usb(precvframe, ppending_recv_queue);
#ifdef LINUX_TASKLET
		tasklet_schedule(&padapter->irq_rx_tasklet);
#else
		_up_sema(&precvpriv->recv_sema);
#endif /*LINUX_TASKLET*/
	}
	else
	{
		//enqueue to recv_pending_queue
		enqueue_recvframe_usb(precvframe, ppending_recv_queue);
	}	
	_exit_critical(&ppending_recv_queue->lock, &irqL);
	 
#endif 
	precvpriv->rx_pkts++;
		
_func_exit_;		
	return ret;
#else
	if(_queue_empty(pfree_recv_queue) == _TRUE )
	{
		//check if precv_frame->u.hdr.pkt == NULL
		if(precv_ndispkt == NULL)
		{
			ret = _FAIL;
			DEBUG_ERR(("recv_indicatepkt():failed to allocate SKB_BUFF\n"));
			goto  _recv_entry_drop;
		}

		//todo:
		DEBUG_ERR(("\n _recv_entry_drop: if(_queue_empty(pfree_recv_queue) == _TRUE)\n"));
		goto  _recv_entry_drop;
	}
	else
	{
		//enqueue to recv_pending_queue
		enqueue_recvframe(precvframe, ppending_recv_queue);

                //todo: _up_sema in caller
		_up_sema(&precvpriv->recv_sema);

		precvpriv->rx_pkts++;
_func_exit_;
		return ret;
	}
#endif /*CONFIG_USB_HCI*/

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


void recv_indicatepkt(_adapter *padapter, union recv_frame *precv_frame, _pkt *pkt)
{
	struct intf_hdl * pintfhdl;
	struct recv_priv *precvpriv;
	_queue	*pfree_recv_queue;		
	unsigned char *pdata;      
	unsigned int bufferlen;
	_pkt *skb;

	NDIS_STATUS	 status=0;

_func_enter_;
	pintfhdl=&(padapter->pio_queue->intf);
	precvpriv = &(padapter->recvpriv);	
	pfree_recv_queue = &(precvpriv->free_recv_queue);	
	pdata=get_recvframe_data(precv_frame);	
	bufferlen=precv_frame->u.hdr.len;
	skb=precv_frame->u.hdr.pkt;	

        //check if precv_frame->u.hdr.pkt == NULL
	if(skb == NULL)
	{
            status=1 ;
            DEBUG_ERR(("recv_indicatepkt():skb==NULL something wrong!!!!\n"));		   
	    goto _recv_indicatepkt_drop;
	}
	DEBUG_INFO(("recv_indicatepkt():skb != NULL !!!\n"));		
	DEBUG_INFO(("\n recv_indicatepkt():precv_frame->u.hdr.rx_head=%p  precv_frame->hdr.rx_data=%p ",precv_frame->u.hdr.rx_head,precv_frame->u.hdr.rx_data));
	DEBUG_INFO(("precv_frame->hdr.rx_tail=%p precv_frame->u.hdr.rx_end=%p precv_frame->hdr.len=%d \n",precv_frame->u.hdr.rx_tail,precv_frame->u.hdr.rx_end,precv_frame->u.hdr.len));
	skb->head=precv_frame->u.hdr.rx_head;
	skb->data=precv_frame->u.hdr.rx_data;
	skb->tail=precv_frame->u.hdr.rx_tail;
	skb->end=precv_frame->u.hdr.rx_end;
	skb->len=precv_frame->u.hdr.len;
	DEBUG_INFO(("\n skb->head=%p skb->data=%p skb->tail=%p skb->end=%p skb->len=%d\n",skb->head,skb->data,skb->tail,skb->end,skb->len));
	skb->ip_summed = CHECKSUM_NONE;
	skb->dev=padapter->pnetdev;
	skb->protocol= eth_type_trans(skb, padapter->pnetdev);
	if(skb!= NULL)
	        netif_rx(skb);
	else
		DEBUG_ERR(("\n recv_indicatepkt:doesn't call netif_rx(skb), beacuse skb==NULL!!!!!!!!!!\n"));

	//padapter->last_rx = jiffies;

	DEBUG_INFO(("\n recv_indicatepkt :after netif_rx!!!!\n"));
	//precv_frame->u.hdr.pkt=NULL;

	//assign pre allocated skb to recv frame and resubmit
	precv_frame->u.hdr.pkt = pkt;
#ifdef CONFIG_USB_HCI
	usb_read_port(pintfhdl,0,0,(unsigned char *)precv_frame);
#else//SDIO
	free_recvframe(precv_frame, pfree_recv_queue);
#endif

_func_exit_;		
        return;		

_recv_indicatepkt_drop:

#ifdef CONFIG_USB_HCI
	usb_read_port(pintfhdl,0,0,(unsigned char *)precv_frame);
#else //SDIO
	//todo: enqueue back to free_recv_queue	
	if(precv_frame)
		free_recvframe(precv_frame, pfree_recv_queue);
#endif
	 
 	 precvpriv->rx_drop++;	
_func_exit_;
}

void alloc_os_rxpkt(union recv_frame *precvframe ,struct recv_priv *precvpriv)
{
	if(precvframe->u.hdr.pkt == NULL)
	{
		precvframe->u.hdr.pkt=alloc_skb(SZ_RECVFRAME,GFP_ATOMIC);
		if(precvframe->u.hdr.pkt ==NULL)
		{
			DEBUG_ERR(("\n allocate skb_buff fail!!!!!!!!!!!!! \n"));
		}
	}
	
	precvframe->u.hdr.len=0;
		
	if(precvframe->u.hdr.pkt !=NULL){

		precvframe->u.hdr.rx_head=precvframe->u.hdr.pkt->head;
		precvframe->u.hdr.rx_data=precvframe->u.hdr.pkt->data;
		precvframe->u.hdr.rx_tail=precvframe->u.hdr.pkt->tail;
		precvframe->u.hdr.rx_end=precvframe->u.hdr.pkt->end;
#ifdef CONFIG_USB_HCI
		precvframe->u.hdr.purb->transfer_buffer=precvframe->u.hdr.rx_data;
#endif
	}
	else{
		precvframe->u.hdr.rx_head=precvframe->u.hdr.rx_data=precvframe->u.hdr.rx_tail=precvframe->u.hdr.rx_end=NULL;
		DEBUG_ERR(("/n precvframe->hdr.pkt == NULL !!!!!!!!!!/n"));
	}
		
}
