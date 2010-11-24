/******************************************************************************
* xmit_linux.c                                                                                                                                 *
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
#define _XMIT_OSDEP_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


#include <if_ether.h>
#include <ip.h>
#include <wifi.h>
#include <mlme_osdep.h>
#include <xmit_osdep.h>
#include <osdep_intf.h>
#include <circ_buf.h>
#include <rtl871x_xmit.h>
#ifdef CONFIG_RTL8711
#include <rtl8711_byteorder.h>
#include <rtl8711_regdef.h>
#endif

void _open_pktfile (_pkt *pktptr, struct pkt_file *pfile)
{
_func_enter_;

	pfile->pkt = pktptr;
	pfile->cur_addr = pfile->buf_start = pktptr->data;
	pfile->pkt_len = pfile->buf_len = pktptr->len;

	pfile->cur_buffer = pfile->buf_start ;
	
_func_exit_;
}

uint _pktfile_read (struct pkt_file *pfile, u8 *rmem, uint rlen)
{
	
	uint	len = 0;
	
_func_enter_;

       len =  remainder_len(pfile);
      	len = (rlen > len)? len: rlen;

       if(rmem)
		skb_copy_bits(pfile->pkt, pfile->buf_len-pfile->pkt_len, rmem, len);

       pfile->cur_addr += len;
       pfile->pkt_len -= len;
	   
_func_exit_;	       		

	return len;       	 
	
}


int init_scsibuf_entry(_adapter *padapter, struct SCSI_BUFFER_ENTRY *psb_entry)
{
	int i;
	
	for(i=0 ; i<SCSI_BUFFER_NUMBER ; i++, psb_entry++)
	{
		memset(psb_entry->entry_memory, 0, SCSI_ENTRY_MEMORY_SIZE);		

		psb_entry->purb = usb_alloc_urb(0,GFP_KERNEL); 
		if(psb_entry->purb == NULL) 
		{
		  	DEBUG_ERR(("!!!!!Error!!!!!  piorw_urb allocate fail in init_scsibuf\n"));
			return _FAIL;
		}
	
		psb_entry->scsi_irp_pending = _FALSE;
		psb_entry->padapter = padapter;
	}
	return _SUCCESS;
}


int init_scsibuf(_adapter *padapter)
{
	struct SCSI_BUFFER *psb = NULL;

	padapter->pscsi_buf = (struct SCSI_BUFFER *)_malloc( sizeof(struct SCSI_BUFFER) );
	psb = padapter->pscsi_buf ;
	psb->head  = 0;
	psb->tail = 0;

	psb->pscsi_buffer_entry = (struct SCSI_BUFFER_ENTRY *)_malloc( (sizeof(struct SCSI_BUFFER_ENTRY) * SCSI_BUFFER_NUMBER) );
	if(psb->pscsi_buffer_entry == NULL) 
	{
	  	DEBUG_ERR(("!!!!!Error!!!!!  psb_entry allocate fail in init_scsibuf\n"));
		return _FAIL;
	}
	
	init_scsibuf_entry(padapter, (struct SCSI_BUFFER_ENTRY *)psb->pscsi_buffer_entry);
		
	return _SUCCESS;
}



sint	_init_xmit_priv(struct xmit_priv *pxmitpriv, _adapter *padapter)
{
	sint i;
	struct xmit_frame*	pxframe;
	sint	res=_SUCCESS;   
#ifdef LINUX_TX_TASKLET
	struct hw_xmit *hwxmits = pxmitpriv->hwxmits;
#endif
	
#ifdef CONFIG_USB_HCI	   
	sint j;
#endif

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	struct mgnt_frame* pmgntframe;
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

#ifdef CONFIG_USB_HCI       

		pxframe->order_tag = i;

		for(j=0; j<8; j++)
		{
			pxframe->pxmit_urb[j] = usb_alloc_urb(0,GFP_KERNEL); 
			if(pxframe->pxmit_urb[j] == NULL) {
				DEBUG_ERR(("_init_xmit_priv:(iet:%d)pxframe->pxmit_urb[%d] == NULL",i,j));
				//return _FAIL;	    
			}
			pxframe->bpending[j]=_FALSE;
		}
             
		pxframe->padapter= padapter;
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

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

	_init_queue(&pxmitpriv->mgnt_pending);
	_init_queue(&pxmitpriv->beacon_pending);
	_init_queue(&pxmitpriv->free_mgnt_queue);
		
	_init_hw_txqueue(&pxmitpriv->mgnt_txqueue, MGNT_QUEUE_INX);
	_init_hw_txqueue(&pxmitpriv->beacon_txqueue, BEACON_QUEUE_INX);

	//FIXME::
	pxmitpriv->ieee_rate = 540;//54Mbps
	pxmitpriv->ieee_basic_rate = 10;// 1Mbps
	pxmitpriv->free_mgnt_frame_cnt = NR_MGNTFRAME; 
		
	_init_sema(&pxmitpriv->mgnt_sema, 0);
		
	pxmitpriv->pallocated_mgnt_frame_buf = _malloc(NR_MGNTFRAME * sizeof(struct mgnt_frame) + 4);
	if (pxmitpriv->pallocated_mgnt_frame_buf  == NULL){
		res= _FAIL;
		goto exit;
	}
		
	pxmitpriv->pxmit_mgnt_frame_buf = pxmitpriv->pallocated_mgnt_frame_buf + 4 -
							((uint) (pxmitpriv->pallocated_mgnt_frame_buf) &3);

	pmgntframe = (struct mgnt_frame*)pxmitpriv->pxmit_mgnt_frame_buf;
	
	for(i=0; i < NR_MGNTFRAME ; i++)
	{
		_init_listhead(&(pmgntframe->list));

		for(j=0 ; j<8 ; j++)
		{
			pmgntframe->pxmit_urb[j] = usb_alloc_urb(0,GFP_KERNEL); 
			if(pmgntframe->pxmit_urb[j] == NULL) 
			{
				DEBUG_ERR(("_init_xmit_priv:(iet:%d)pmgntframe->pxmit_urb == NULL",i));
				//return _FAIL; 	
			}
			pmgntframe->bpending[j]=_FALSE;
		}
		 
		pmgntframe->padapter= padapter;
		pmgntframe->pkt = NULL;
		pmgntframe->frame_tag = TAG_MGNTFRAME;
		list_insert_tail(&(pmgntframe->list), &(pxmitpriv->free_mgnt_queue.queue));
		pmgntframe++;
	}
	
		pxmitpriv->adapter = padapter;
	
#endif

	pxmitpriv->frag_len = MAX_FRAG_THRESHOLD;
	DEBUG_ERR(("\n _init_xmit_priv:pxmitpriv->frag_len=%d (%d)\n",pxmitpriv->frag_len,MAX_FRAG_THRESHOLD));

#ifdef LINUX_TX_TASKLET
        hwxmits[0] .phwtxqueue = &pxmitpriv->vo_txqueue;
        hwxmits[0] .sta_queue = &pxmitpriv->vo_pending;

        pxmitpriv->vi_txqueue.head = 0;
        hwxmits[1] .phwtxqueue = &pxmitpriv->vi_txqueue;
        hwxmits[1] .sta_queue = &pxmitpriv->vi_pending;

        pxmitpriv->be_txqueue.head = 0;
        hwxmits[2] .phwtxqueue = &pxmitpriv->be_txqueue;
        hwxmits[2] .sta_queue = &pxmitpriv->be_pending;

        pxmitpriv->bk_txqueue.head = 0;
        hwxmits[3] .phwtxqueue = &pxmitpriv->bk_txqueue;
        hwxmits[3] .sta_queue = &pxmitpriv->bk_pending;

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

        hwxmits[4] .sta_queue = &pxmitpriv->mgnt_pending;

        hwxmits[5] .sta_queue = &pxmitpriv->beacon_pending;

#endif
     init_hwxmits(hwxmits, HWXMIT_ENTRY);

#endif


#ifdef CONFIG_RTL8711
	init_scsibuf(padapter);
#endif

exit:	
_func_exit_;		
	return _SUCCESS;

}


void deinit_scsibuf(_adapter *padapter)
{
	int i;
	struct SCSI_BUFFER *psb = padapter->pscsi_buf;
	struct SCSI_BUFFER_ENTRY *psb_entry = psb->pscsi_buffer_entry;
	
	for(i=0 ; i<SCSI_BUFFER_NUMBER ; i++, psb_entry++)
	{
		if(psb_entry->purb)
			usb_free_urb(psb_entry->purb);
	}
	if(psb->pscsi_buffer_entry)
		_mfree((u8 *)psb->pscsi_buffer_entry, (sizeof(struct SCSI_BUFFER_ENTRY) * SCSI_BUFFER_NUMBER) );

	if(psb)
		_mfree((u8*)psb, sizeof(struct SCSI_BUFFER) );
	
}

void _free_xmit_priv (struct xmit_priv *pxmitpriv)
{
       int i;
#ifdef CONFIG_RTL8711
      _adapter *padapter = pxmitpriv->adapter;
#endif
	struct xmit_frame *pxmitframe = (struct xmit_frame*) pxmitpriv->pxmit_frame_buf;
	struct mgnt_frame *pmgntframe = (struct mgnt_frame*) pxmitpriv->pxmit_mgnt_frame_buf;

 _func_enter_;   	    

#ifdef CONFIG_RTL8711 
  	deinit_scsibuf(padapter);
#endif

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	for(i=0; i<NR_MGNTFRAME; i++)
	{
		if (pmgntframe->pkt)
		{
			pxmitframe->pkt = NULL;
		}
		pxmitframe++;
	}

	if(pxmitpriv->pallocated_frame_buf)
		_mfree(pxmitpriv->pallocated_mgnt_frame_buf, NR_MGNTFRAME * sizeof(struct mgnt_frame) + 4);
#endif

	for(i=0; i<NR_XMITFRAME; i++)
	{
		if (pxmitframe->pkt)
		{
			pxmitframe->pkt = NULL;
		}
		pxmitframe++;
	}		
	if(pxmitpriv->pallocated_frame_buf)
		_mfree(pxmitpriv->pallocated_frame_buf, NR_XMITFRAME * sizeof(struct xmit_frame) + 4);
#ifdef LINUX_TX_TASKLET
	_mfree((u8 *)pxmitpriv->hwxmits, sizeof(struct hw_xmit)*6);
#endif
_func_exit_;		
}

void set_qos(struct pkt_file *ppktfile, struct pkt_attrib *pattrib)
{

	u32 UserPriority = 0 ;
	u32 i;	
	
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
		
	pattrib->priority = UserPriority;

	// get wlan_hdr_len
	pattrib->hdrlen = 26;
	pattrib->subtype = WIFI_QOS_DATA_TYPE;



}


int xmit_entry(_pkt *pkt, _nic_hdl pnetdev)
{
	struct	xmit_frame	*pxmitframe = NULL;	
	_adapter *padapter = (_adapter *)pnetdev->priv;	
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);
//	_queue	*pfree_xmit_queue = get_free_xmit_queue(&padapter->xmitpriv);
//	sint p_flag= _FALSE;

#ifdef CONFIG_RTL8711
	struct SCSI_BUFFER *psb=padapter->pscsi_buf;
#endif

	int ret=0;
	_irqL irqL;

_func_enter_;	
	

	if ((if_up(padapter)) == _FALSE)
	{
		//DEBUG_ERR(("xmit_entry: if_up fail \n" ));
		_func_exit_;
		ret = 1;
		goto _xmit_entry_drop;		
	}
		
	pxmitframe = alloc_xmitframe(pxmitpriv);

	if (pxmitframe == NULL)
	{

		//struct sta_info *pstainfo = get_stainfo(&padapter->stapriv, padapter->mlmepriv.cur_network.network.MacAddress);
		DEBUG_ERR(("pxmitframe == NULL \n"));
		if (!netif_queue_stopped(pnetdev))
		       netif_stop_queue(pnetdev);		
		
		ret = 1;	
		goto _xmit_entry_drop;
	}	

	DEBUG_INFO(("%s(%d) pxmitframe=%p\n",__FUNCTION__,__LINE__,pxmitframe));
      
	pxmitframe->pkt = NULL;	  	   

	if ((update_attrib(padapter, pkt, &pxmitframe->attrib)) == _FAIL)
	{
		DEBUG_INFO(("drop xmit pkt for update fail\n"));
		pxmitframe->pkt = NULL;
		ret = 1;
		goto _xmit_entry_drop;
	}	

	DEBUG_INFO(("drop xmit pkt for update success\n"));

// TODO: AMSDU coalesce issue , do it here?
//#ifdef CONFIG_RTL8192U

//	pxmitframe->pkt = pkt;

//#else

	xmitframe_coalesce(padapter, pkt, pxmitframe);
//#endif


	_enter_critical(&pxmitpriv->lock, &irqL);
	if ((txframes_pending_nonbcmc(padapter)) == _FALSE)
	{
		if ((xmit_classifier_direct(padapter, pxmitframe)) == _FAIL)		
		{
			DEBUG_INFO(("xmit entry classifier failed, drop xmit pkt\n"));

			pxmitframe->pkt = NULL;
			goto _xmit_entry_drop;
		}

#ifdef CONFIG_RTL8711
		if(CIRC_SPACE(  psb->head, psb->tail, SCSI_BUFFER_NUMBER))
		{
			_up_sema(&pxmitpriv->xmit_sema);
		}
#endif 

#ifdef LINUX_TX_TASKLET
                tasklet_schedule(&padapter->irq_tx_tasklet);


#else
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
		DEBUG_INFO(("xmit entry :up_sema \n"));
		_up_sema(&pxmitpriv->xmit_sema);
#endif
#endif
	}
	else
	{
		if ((xmit_classifier_direct(padapter, pxmitframe)) == _FAIL)		
		{
			DEBUG_ERR(("xmit entry classifier failed, drop xmit pkt\n"));

			pxmitframe->pkt = NULL;
			goto _xmit_entry_drop;			
		}
#ifdef LINUX_TX_TASKLET
                tasklet_schedule(&padapter->irq_tx_tasklet);
#endif
	}
	_exit_critical(&pxmitpriv->lock, &irqL);	
	
	pxmitpriv->tx_pkts++;
	
	_func_exit_;

// If RTL8192U , free skb must be done after AMSDU, that is , we should keep the packet 
#if 0
#ifndef CONFIG_RTL8192U
	dev_kfree_skb(pkt);
#endif
#endif
	dev_kfree_skb(pkt);

	return ret;	
	
_xmit_entry_drop:
	

	if (pxmitframe)
	{
		free_xmitframe(pxmitpriv,pxmitframe);
	}

	pxmitpriv->tx_drop++;
	
_func_exit_;        
	return ret;

}


 void check_free_tx_packet (struct xmit_priv *pxmitpriv,struct xmit_frame *pxmitframe)
 {
	_adapter *	padapter = pxmitpriv->adapter;

_func_enter_;	

	if (pxmitframe->pkt) 
	{
		pxmitframe->pkt = NULL;
	}	
	

        if (netif_queue_stopped(padapter->pnetdev))		     
                 netif_wake_queue(padapter->pnetdev);	


_func_exit_;

}
