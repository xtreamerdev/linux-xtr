#define _XMIT_OSDEP_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <circ_buf.h>





#ifdef PLATFORM_WINDOWS

#include <if_ether.h>
#include <ip.h>
#include <rtl8711_byteorder.h>
#include <wifi.h>
#include <mlme_osdep.h>
#include <xmit_osdep.h>
#include <rtl8711_regdef.h>
#include <osdep_intf.h>
#endif


void _open_pktfile (_pkt *pktptr, struct pkt_file *pfile)
{
_func_enter_;
	pfile->pkt = pktptr;

	NdisQueryPacket(pktptr, NULL, NULL, &pfile->cur_buffer,&pfile->pkt_len);
	NdisQueryBufferSafe(pfile->cur_buffer, &pfile->buf_start, &pfile->buf_len,HighPagePriority);

	pfile->cur_addr = pfile->buf_start;
_func_exit_;
}


uint _pktfile_read (struct pkt_file *pfile, u8 *rmem, uint rlen)
{

	_buffer	*next_buffer;
	
	uint	len = 0, t_len= 0;
_func_enter_;
	while(1)
	{

		len =  remainder_len(pfile);
	
		len = (rlen > len)? len: rlen;

		if (len) {
			
			if (rmem)
				_memcpy(rmem, pfile->cur_addr, len);
		}
		
		t_len += len;

		rlen -= len;

		rmem += len;

		pfile->cur_addr += len;

		pfile->pkt_len -= len;

		if (rlen)
		{
			NdisGetNextBuffer(pfile->cur_buffer, &next_buffer);
			
			if (next_buffer == NULL)
				break;
		}
		else
			break;
		
		pfile->cur_buffer = next_buffer;
		
		NdisQueryBufferSafe(pfile->cur_buffer, &pfile->buf_start, &pfile->buf_len,HighPagePriority);
		pfile->cur_addr = pfile->buf_start;
		
	}
_func_exit_;	
	return t_len;

}

int init_scsibuf_entry(_adapter *padapter, struct SCSI_BUFFER_ENTRY *psb_entry)
{
	int i,j;
	
	for(i=0 ; i<SCSI_BUFFER_NUMBER ; i++, psb_entry++){
#if 0
		psb_entry->entry_memory = _malloc(SCSI_ENTRY_MEMORY_SIZE);
		if(!psb_entry->entry_memory)
			return _FAIL;
#endif		
		memset(psb_entry->entry_memory, 0, SCSI_ENTRY_MEMORY_SIZE);		

		psb_entry->purb = (PURB)ExAllocatePool(NonPagedPool, sizeof(URB) ); 
		if(psb_entry->purb == NULL) {
		  	DbgPrint("!!!!!Error!!!!!  piorw_urb allocate fail in init_scsibuf\n");
			return _FAIL;
		}
	
		psb_entry->pirp = IoAllocateIrp(padapter->dvobjpriv.nextdevstacksz, _FALSE);
		
		psb_entry->scsi_irp_pending = _FALSE;
		
		psb_entry->padapter = padapter;

	}


	return _SUCCESS;
}

int init_scsibuf(_adapter *padapter)
{
	int i;
	struct SCSI_BUFFER *psb = NULL;
	struct SCSI_BUFFER_ENTRY *psb_entry;


	padapter->pscsi_buf = (struct SCSI_BUFFER *)_malloc( sizeof(struct SCSI_BUFFER) );
	psb = padapter->pscsi_buf ;
	psb->head  = 0;
	psb->tail = 0;

	psb->pscsi_buffer_entry = (struct SCSI_BUFFER_ENTRY *)_malloc( (sizeof(struct SCSI_BUFFER_ENTRY) * SCSI_BUFFER_NUMBER) );
	if(psb->pscsi_buffer_entry == NULL) {
	  	DEBUG_ERR(("!!!!!Error!!!!!  psb_entry allocate fail in init_scsibuf\n"));
		return _FAIL;
	}
	
	init_scsibuf_entry(padapter, (struct SCSI_BUFFER_ENTRY *)psb->pscsi_buffer_entry);
		
	return _SUCCESS;
}





sint	_init_xmit_priv(struct xmit_priv *pxmitpriv, _adapter *padapter)
{

	//yitsen
	
	sint i;
	struct xmit_frame*	pxframe;
	sint	res=_SUCCESS;   
	
#ifdef PLATFORM_OS_XP
        
	s8	NextDeviceStackSize;

 #ifdef CONFIG_USB_HCI	   
 	struct dvobj_priv	*pusbpriv=&padapter->dvobjpriv;
	//CE_USB_DEVICE  *pNdisCEDvice = (CE_USB_DEVICE  *)&padapter->NdisCEDev;	
       NextDeviceStackSize = (s8) pusbpriv->nextdevstacksz;//pNdisCEDvice->pUsbDevObj->StackSize + 1;
 #endif
	
#endif
	

_func_enter_;   	
	memset((unsigned char *)pxmitpriv, 0, sizeof(struct xmit_priv));
	
	_spinlock_init(&pxmitpriv->lock);
	_init_sema(&pxmitpriv->xmit_sema, 0);
	_init_sema(&pxmitpriv->terminate_xmitthread_sema, 0);

	/* 
	Please insert all the queue initializaiton using _init_queue below
	*/

	pxmitpriv->adapter = padapter;
	
	for(i = 0 ; i < MAX_NUMBLKS; i++)
		_init_queue(&pxmitpriv->blk_strms[i]);
	
	_init_queue(&pxmitpriv->be_pending);
	_init_queue(&pxmitpriv->bk_pending);
	_init_queue(&pxmitpriv->vi_pending);
	_init_queue(&pxmitpriv->vo_pending);
	_init_queue(&pxmitpriv->bm_pending);

	_init_queue(&pxmitpriv->legacy_dz_queue);
	_init_queue(&pxmitpriv->apsd_queue);

	_init_queue(&pxmitpriv->free_xmit_queue);


	/*	
	Please allocate memory with the sz = (struct xmit_frame) * NR_XMITFRAME, 
	and initialize free_xmit_frame below.
	Please also apply  free_txobj to link_up all the xmit_frames...
	*/

	pxmitpriv->pallocated_frame_buf = _malloc(NR_XMITFRAME * sizeof(struct xmit_frame) + 4);
	
	if (pxmitpriv->pallocated_frame_buf  == NULL){
		res= _FAIL;
		goto exit;
	}
	pxmitpriv->pxmit_frame_buf = pxmitpriv->pallocated_frame_buf + 4 -
							((uint) (pxmitpriv->pallocated_frame_buf) &3);

	pxframe = (struct xmit_frame*) pxmitpriv->pxmit_frame_buf;


	for(i=0; i < NR_XMITFRAME ; i++)
	{
		_init_listhead(&(pxframe->list));

#ifdef PLATFORM_OS_XP

 #ifdef CONFIG_USB_HCI       
 	{
           int j;
           for(j=0; j<8; j++)
           {
              	 
              pxframe->pxmit_urb[j] = (PURB)ExAllocatePool(NonPagedPool, sizeof(URB)); 
              if(pxframe->pxmit_urb[j] == NULL) 
	                  return _FAIL;	    

              pxframe->pxmit_irp[j] = IoAllocateIrp(NextDeviceStackSize, FALSE);	  	
		pxframe->bpending[j] =_FALSE;
           }
           }

             
	      pxframe->padapter= padapter;
             
 #endif		

#endif
              pxframe->pkt = NULL;
		list_insert_tail(&(pxframe->list), &(pxmitpriv->free_xmit_queue.queue));

		pxframe++;
	}


	pxmitpriv->free_xmitframe_cnt = NR_XMITFRAME;

	/*
		init xmit hw_txqueue
	*/

	_init_hw_txqueue(&pxmitpriv->be_txqueue, BE_QUEUE_INX);
	_init_hw_txqueue(&pxmitpriv->bk_txqueue, BK_QUEUE_INX);
	_init_hw_txqueue(&pxmitpriv->vi_txqueue,VI_QUEUE_INX);
	_init_hw_txqueue(&pxmitpriv->vo_txqueue,VO_QUEUE_INX);
	_init_hw_txqueue(&pxmitpriv->bmc_txqueue,BMC_QUEUE_INX);

	pxmitpriv->frag_len = MAX_FRAG_THRESHOLD;
	init_scsibuf(padapter);

exit:	
_func_exit_;		
	return _SUCCESS;

}


void deinit_scsibuf(_adapter *padapter)
{
	int i;
	struct SCSI_BUFFER *psb = padapter->pscsi_buf;
	struct SCSI_BUFFER_ENTRY *psb_entry = psb->pscsi_buffer_entry;
	
	for(i=0 ; i<SCSI_BUFFER_NUMBER ; i++, psb_entry++){
//		if(psb_entry->entry_memory)
//			 _mfree(psb_entry->entry_memory,  SCSI_ENTRY_MEMORY_SIZE );
		if(psb_entry->purb)
			ExFreePool(psb_entry->purb);
		if(psb_entry->pirp)
			IoFreeIrp(psb_entry->pirp);
	}
	if(psb->pscsi_buffer_entry)
		_mfree((u8 *)psb->pscsi_buffer_entry, (sizeof(struct SCSI_BUFFER_ENTRY) * SCSI_BUFFER_NUMBER) );

	if(psb)
		_mfree((u8*)psb, sizeof(struct SCSI_BUFFER) );
	
}


void _free_xmit_priv (struct xmit_priv *pxmitpriv)
{
       int i;
      _adapter *padapter = pxmitpriv->adapter;
	struct xmit_frame*	pxmitframe = (struct xmit_frame*) pxmitpriv->pxmit_frame_buf;

 _func_enter_;   

 	deinit_scsibuf(padapter);
		for(i=0; i<NR_XMITFRAME; i++)
		{

			if (pxmitframe->pkt)
			{
				NdisMSendComplete(padapter->hndis_adapter, 
 	 	                     pxmitframe->pkt, NDIS_STATUS_SUCCESS);
       	          pxmitframe->pkt = NULL;
			}
			pxmitframe++;
		}		
	if(pxmitpriv->pallocated_frame_buf)
		_mfree(pxmitpriv->pallocated_frame_buf, NR_XMITFRAME * sizeof(struct xmit_frame) + 4);
_func_exit_;		
}


void set_qos(struct pkt_file *ppktfile, struct pkt_attrib *pattrib)
{
	NDIS_PACKET_8021Q_INFO VlanPriInfo;
	UINT32 UserPriority;
	UINT32 VlanID;
	VlanPriInfo.Value = 
	      NDIS_PER_PACKET_INFO_FROM_PACKET(pkt, Ieee8021QInfo);
	UserPriority = VlanPriInfo.TagHeader.UserPriority;
	VlanID = VlanPriInfo.TagHeader.VlanId;

	if(VlanPriInfo.Value==NULL)//((VlanID==0) && (UserPriority==0))
	{
		// get UserPriority from IP hdr
		if(pattrib->ether_type== 0x0800)
		{
			struct iphdr ip_hdr; 
			
			i = _pktfile_read (ppktfile, (u8*)&ip_hdr, sizeof(ip_hdr));

			UserPriority = (ntohs(ip_hdr.tos) >> 5) & 0x3 ;
		}
		else
		{
			// "When priority processing of data frames is supported, 
			//a STA's SME should send EAPOL-Key frames at the highest priority."
		
			if(pattrib->ether_type == 0x888e)
				UserPriority = 7;
		}
	}
	pattrib->priority = (UCHAR)UserPriority;

	// get wlan_hdr_len
	pattrib->hdrlen = 26;
	pattrib->subtype = WIFI_QOS_DATA_TYPE;

}


NDIS_STATUS xmit_entry(
IN _nic_hdl		cnxt,
IN NDIS_PACKET		*pkt,
IN UINT				flags
)

{
	//struct	pkt_file	pktfile;
		
	struct	xmit_frame	*pxmitframe = NULL;
	
	_adapter *padapter = (_adapter *)cnxt;
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);
	_queue	*pfree_xmit_queue = get_free_xmit_queue(&padapter->xmitpriv);
	struct SCSI_BUFFER * psb = padapter->pscsi_buf;
	sint p_flag= _FALSE;
	_irqL irqL;
	
_func_enter_;	
	
	if ((if_up(padapter)) == _FALSE)
	{
		DEBUG_ERR(("xmit_entry: if_up fail \n" ));
		_func_exit_;
		return NDIS_STATUS_HARD_ERRORS;
		//return NDIS_STATUS_FAILURE;
	}
		
	pxmitframe = alloc_xmitframe(pxmitpriv);

	if (pxmitframe == NULL)
	{
#if 0
		struct sta_info *pstainfo = get_stainfo(&padapter->stapriv, padapter->mlmepriv.cur_network.network.MacAddress);
		DEBUG_ERR(("pxmitframe == NULL \n"));

		if (pstainfo == NULL)
			DEBUG_ERR(("Weired Bug \n"));
#endif
		goto _xmit_entry_drop;
	}	

	#ifndef CONFIG_USB_HCI
       if( pxmitpriv->free_xmitframe_cnt <(NR_XMITFRAME-TX_GUARD_BAND))       
	{
            	p_flag = _TRUE;
		pxmitframe->pkt = pkt;
		if(pxmitframe->pkt==NULL)
			DEBUG_ERR(("\n ##############ERROR###########\n"));
       }
	else  
	#endif	
	{
	     p_flag = _FALSE;
	     pxmitframe->pkt = NULL;
	 }     	   


	if ((update_attrib(padapter, pkt, &pxmitframe->attrib)) == _FAIL)
	{
		DEBUG_ERR(("xmit_entry:drop xmit pkt for update fail\n"));
		pxmitframe->pkt = NULL;
		goto _xmit_entry_drop;
	}	

	xmitframe_coalesce(padapter, pkt, pxmitframe);
	
	_enter_critical(&pxmitpriv->lock, &irqL);
	if ((txframes_pending_nonbcmc(padapter)) == _FALSE){

		if ((xmit_classifier_direct(padapter, pxmitframe)) == _FAIL)		
		{
			DEBUG_ERR(("drop xmit pkt for classifier fail\n"));
			pxmitframe->pkt = NULL;
				_exit_critical(&pxmitpriv->lock, &irqL);	
			goto _xmit_entry_drop;
		}

		if(CIRC_SPACE(  psb->head, psb->tail, SCSI_BUFFER_NUMBER))
		{
			_up_sema(&pxmitpriv->xmit_sema);
		}

	}
	else{

		if ((xmit_classifier_direct(padapter, pxmitframe)) == _FAIL)		
		{
			DEBUG_ERR(("drop xmit pkt for classifier fail\n"));
			pxmitframe->pkt = NULL;
			_exit_critical(&pxmitpriv->lock, &irqL);	
			goto _xmit_entry_drop;			
		}

	}
	_exit_critical(&pxmitpriv->lock, &irqL);	

	pxmitpriv->tx_pkts++;
	
	if( p_flag == _TRUE){	
		_func_exit_;
		return NDIS_STATUS_PENDING;
	}
	else{
		_func_exit_;
		return NDIS_STATUS_SUCCESS;
	}
	
	
	

	
_xmit_entry_drop:


	if (pxmitframe)
	{
		free_xmitframe(pxmitpriv,pxmitframe);
	}
	pxmitpriv->tx_drop++;

_func_exit_;
	return NDIS_STATUS_SUCCESS;

}


 void check_free_tx_packet (struct xmit_priv *pxmitpriv,struct xmit_frame *pxmitframe)
 {
	_queue *pfree_xmit_queue = &pxmitpriv->free_xmit_queue;
		
	_adapter *padapter=pxmitpriv->adapter;

	_pkt		*pndis_pkt = NULL;

_func_enter_;	
	if (pxmitframe->pkt) 
	{
        	pndis_pkt  = pxmitframe->pkt;              
		pxmitframe->pkt = NULL;
	}	
	
       if(pndis_pkt)
           NdisMSendComplete(padapter->hndis_adapter, pndis_pkt, NDIS_STATUS_SUCCESS);

_func_exit_;	 
}

	

