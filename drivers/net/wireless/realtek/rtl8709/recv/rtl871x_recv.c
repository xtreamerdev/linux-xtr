/******************************************************************************
* rtl871x_recv.c                                                                                                                                 *
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
#define _RTL871X_RECV_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <mlme_osdep.h>
#include <ip.h>
#include <if_ether.h>
#include <Ethernet.h>
#include <usb_ops.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#include <wifi.h>
#include <circ_buf.h>


u8 SNAP_ETH_TYPE_IPX[2] = {0x81, 0x37};

u8 SNAP_ETH_TYPE_APPLETALK_AARP[2] = {0x80, 0xf3};
u8 SNAP_ETH_TYPE_APPLETALK_DDP[2] = {0x80, 0x9b};
u8 SNAP_HDR_APPLETALK_DDP[3] = {0x08, 0x00, 0x07}; // Datagram Delivery Protocol

static u8 oui_8021h[] = {0x00, 0x00, 0xf8};
static u8 oui_rfc1042[]= {0x00,0x00,0x00};


#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
extern void mgtctrl_dispatchers(union recv_frame *precv_frame);
extern void mgnt_work_dispatchers(void *ptr);
#endif

struct rxirp * alloc_rxirp(_adapter *padapter)
{
	unsigned int flags;
	struct rxirp *prxirp = NULL;
	struct mib_info* _sys_mib = &(padapter->_sys_mib);
	_list *head = &(_sys_mib->free_rxirp_mgt);
	
	_enter_critical(&(_sys_mib->free_rxirp_mgt_lock), &flags);
	if( is_list_empty(head) == _TRUE){
		goto exit;
	}

	prxirp = LIST_CONTAINOR(head->next, struct rxirp, list);
	list_delete(&prxirp->list);

exit:	
	_exit_critical(&(_sys_mib->free_rxirp_mgt_lock), &flags);

	return prxirp;
}

static  void mgtctrl_frame(_adapter *padapter, union recv_frame *precvframe)
{
	
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
#ifdef ENABLE_MGNT_THREAD
	_irqL irqL;
	struct rxirp *prxirp;

	prxirp = alloc_rxirp(padapter);	
	if(prxirp == NULL)
		return;
	
	_memcpy((void*)prxirp->rxbuf_phyaddr, precvframe->u.hdr.rx_data, precvframe->u.hdr.len);
	prxirp->rx_len = precvframe->u.hdr.len;

	list_delete(&prxirp->list);
	_enter_critical(&(padapter->mlmepriv.mgnt_pending_queue.lock), &irqL);
	list_insert_tail(&prxirp->list, &(padapter->mlmepriv.mgnt_pending_queue.queue) );
	_exit_critical(&(padapter->mlmepriv.mgnt_pending_queue.lock), &irqL);
	
	_up_sema(&padapter->mlmepriv.mgnt_sema);	
#else
	mgtctrl_dispatchers(precvframe);
#endif /*ENABLE_MGNT_THREAD*/

#endif /*defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)*/
}



void	_init_sta_recv_priv(struct sta_recv_priv *psta_recvpriv)
{
	sint i;
_func_enter_;	
	memset((u8 *)psta_recvpriv, 0, sizeof (struct sta_recv_priv));

	_spinlock_init(&psta_recvpriv->lock);

	for(i=0; i<MAX_RX_NUMBLKS; i++)
		_init_queue(&psta_recvpriv->blk_strms[i]);

	_init_queue(&psta_recvpriv->defrag_q);
_func_exit_;

}


/*
pfree_recv_queue accesser: returnPkt(dispatch level), rx_done(ISR?),  =>  using crtitical sec ??

caller of alloc_recvframe have to call =====> init_recvframe(precvframe, _pkt *pkt);

*/
union recv_frame *alloc_recvframe (_queue *pfree_recv_queue)
{
	_irqL irqL;
	union recv_frame  *precvframe;
	_list	*plist, *phead;
	_adapter *padapter;
	struct recv_priv *precvpriv;
	
_func_enter_;
	_enter_critical(&pfree_recv_queue->lock, &irqL);
	//_spinlock(&pfree_recv_queue->lock);

	if(_queue_empty(pfree_recv_queue) == _TRUE)
	{
		precvframe = NULL;
	}
	else
	{
		phead = get_list_head(pfree_recv_queue);

		plist = get_next(phead);

		precvframe = LIST_CONTAINOR(plist, union recv_frame, u);

		list_delete(&precvframe->u.hdr.list);
		padapter=precvframe->u.hdr.adapter;
		if(padapter !=NULL){
			precvpriv=&padapter->recvpriv;
			if(pfree_recv_queue == &precvpriv->free_recv_queue)
				precvpriv->free_recvframe_cnt--;
		}
	}

	//_spinunlock(&pfree_recv_queue->lock);
	_exit_critical(&pfree_recv_queue->lock, &irqL);
_func_exit_;
	return precvframe;
	
}



void init_recvframe(union recv_frame *precvframe, struct recv_priv *precvpriv)
{
_func_enter_;
	
	//_init_listhead(&precvframe->u.list);

	/* Perry: This can be removed */
	_init_listhead(&precvframe->u.hdr.list);
	
	alloc_os_rxpkt(precvframe, precvpriv);

_func_exit_;	
}


//
// caller have to make sure that (the q of precvframe)'s spinlock is up
//
sint	free_recvframe(union recv_frame *precvframe, _queue *pfree_recv_queue)
{

	_irqL irqL;
	_adapter *padapter=precvframe->u.hdr.adapter;
	struct recv_priv *precvpriv;

_func_enter_;
	//_spinlock(&pfree_recv_queue->lock);
	 _enter_critical(&pfree_recv_queue->lock, &irqL);


	//_init_listhead(&(precvframe->u.hdr.list));
	list_delete(&(precvframe->u.hdr.list));
	
	
	list_insert_tail(&(precvframe->u.hdr.list), get_list_head(pfree_recv_queue));

		if(padapter !=NULL){
			precvpriv=&padapter->recvpriv;
			if(pfree_recv_queue == &precvpriv->free_recv_queue)
				precvpriv->free_recvframe_cnt++;
		}

	//_spinunlock(&pfree_recv_queue->lock);
	 _exit_critical(&pfree_recv_queue->lock, &irqL);

_func_exit_;	
	return _SUCCESS;
}



union recv_frame *dequeue_recvframe (_queue *queue)
{
	return alloc_recvframe(queue);	
}


sint	enqueue_recvframe(union recv_frame *precvframe, _queue *queue)
{
	return free_recvframe(precvframe, queue);
}


sint	enqueue_recvframe_usb(union recv_frame *precvframe, _queue *queue)
{
_func_enter_;

	list_delete(&(precvframe->u.hdr.list));
		
	list_insert_tail(&(precvframe->u.hdr.list), get_list_head(queue));

_func_exit_;	
	return _SUCCESS;
}


/*
caller : defrag ; recvframe_chk_defrag in recv_thread  (passive)
pframequeue: defrag_queue : will be accessed in recv_thread  (passive)

using spinlock to protect

*/

void free_recvframe_queue(_queue *pframequeue,  _queue *pfree_recv_queue)
{
	union	recv_frame 	*precvframe;
	_list	*plist, *phead;

_func_enter_;
	_spinlock(&pframequeue->lock);

	phead = get_list_head(pframequeue);
	plist = get_next(phead);

	while(end_of_queue_search(phead, plist) == _FALSE)
	{
		precvframe = LIST_CONTAINOR(plist, union recv_frame, u);

		plist = get_next(plist);
		
		//list_delete(&precvframe->u.hdr.list); // will do this in free_recvframe()
		
		free_recvframe(precvframe, pfree_recv_queue);
	}

	_spinunlock(&pframequeue->lock);
_func_exit_;
}


sint recvframe_chkmic(_adapter *adapter,  union recv_frame *precvframe){

	sint	i,res=_SUCCESS;
	u32	datalen;
	u8	miccode[8];
	u8	bmic_err=_FALSE;
	u8	*pframe, *payload,*pframemic;
	u8	*mickey;
	struct	sta_info		*stainfo;
	struct	rx_pkt_attrib	*prxattrib=&precvframe->u.hdr.attrib;
	struct 	security_priv	*psecuritypriv=&adapter->securitypriv;
	struct 	recv_stat *prxstat=get_recv_stat(precvframe);

_func_enter_;	
	
	stainfo=get_stainfo(&adapter->stapriv ,&prxattrib->ta[0] );

	if(prxattrib->encrypt ==_TKIP_){
		DEBUG_INFO(("\n recvframe_chkmic:prxattrib->encrypt ==_TKIP_\n"));
		DEBUG_INFO(("\n recvframe_chkmic:da=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
			prxattrib->ra[0],prxattrib->ra[1],prxattrib->ra[2],prxattrib->ra[3],prxattrib->ra[4],prxattrib->ra[5]));

		//calculate mic code
		if(stainfo!= NULL){
			if(IS_MCAST(prxattrib->ra)){
				mickey=&psecuritypriv->dot118021XGrprxmickey.skey[0];
				DEBUG_INFO(("\n recvframe_chkmic: bcmc key \n"));
				if(psecuritypriv->binstallGrpkey==_FALSE){
					res=_FAIL;
					DEBUG_ERR(("\n recvframe_chkmic:didn't install group key!!!!!!!!!!\n"));
					goto exit;
				}
			}
			else{
				mickey=&stainfo->dot11tkiprxmickey.skey[0];
				DEBUG_ERR(("\n recvframe_chkmic: unicast key \n"));
			}
			datalen=precvframe->u.hdr.len-prxattrib->hdrlen-prxattrib->iv_len-prxattrib->icv_len-8;//icv_len included the mic code
			pframe=precvframe->u.hdr.rx_data;
			payload=pframe+prxattrib->hdrlen+prxattrib->iv_len;
			
			seccalctkipmic(&stainfo->dot11tkiprxmickey.skey[0],pframe,payload, datalen ,&miccode[0], (unsigned char)prxattrib->priority ); //care the length of the data
			pframemic=payload+datalen;
			for(i=0;i<8;i++){
				if(miccode[i] != *(pframemic+i)){
					DEBUG_ERR(("recvframe_chkmic:miccode[%d] != *(pframemic+%d) ",i,i));
					bmic_err=_TRUE;
				}
			}
			if(bmic_err==_TRUE){
				DEBUG_ERR(("\n *(pframemic-8)-*(pframemic-1)=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
					*(pframemic-8),*(pframemic-7),*(pframemic-6),*(pframemic-5),*(pframemic-4),*(pframemic-3),*(pframemic-2),*(pframemic-1)));
				DEBUG_ERR(("\n *(pframemic-16)-*(pframemic-9)=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
					*(pframemic-16),*(pframemic-15),*(pframemic-14),*(pframemic-13),*(pframemic-12),*(pframemic-11),*(pframemic-10),*(pframemic-9)));

				{
					uint i;
					DEBUG_ERR(("\n ======demp packet (len=%d)======\n",precvframe->u.hdr.len));
					for(i=0;i<precvframe->u.hdr.len;i=i+8){
						DEBUG_ERR(("0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x",
							*(precvframe->u.hdr.rx_data+i),*(precvframe->u.hdr.rx_data+i+1),
							*(precvframe->u.hdr.rx_data+i+2),*(precvframe->u.hdr.rx_data+i+3),
							*(precvframe->u.hdr.rx_data+i+4),*(precvframe->u.hdr.rx_data+i+5),
							*(precvframe->u.hdr.rx_data+i+6),*(precvframe->u.hdr.rx_data+i+7)));
					}
					//DEBUG_ERR(("\n ======demp packet end======\n",precvframe->u.hdr.len));
					DEBUG_ERR(("\n hrdlen=%d, \n",prxattrib->hdrlen));
				}

				DEBUG_ERR(("prxstat->icv=%d ",prxstat->icv));
				DEBUG_ERR(("ra=0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x psecuritypriv->binstallGrpkey=%d ",
					prxattrib->ra[0],prxattrib->ra[1],prxattrib->ra[2],
					prxattrib->ra[3],prxattrib->ra[4],prxattrib->ra[5],psecuritypriv->binstallGrpkey));

				if(prxstat->decrypted==_TRUE)
				{
					handle_tkip_mic_err(adapter,(u8)IS_MCAST(prxattrib->ra));
				}
				else
				{
					DEBUG_ERR((" mic error :prxstat->decrypted=%d ",prxstat->decrypted));
				}
				res=_FAIL;
			}
			else{
				//mic checked ok
				if((psecuritypriv->bcheck_grpkey ==_FALSE)&&(IS_MCAST(prxattrib->ra)==_TRUE)){
					psecuritypriv->bcheck_grpkey =_TRUE;
					DEBUG_ERR(("psecuritypriv->bcheck_grpkey =_TRUE"));
				}					
			}
			recvframe_pull_tail(precvframe, 8);	
		}
		else{
			DEBUG_ERR(("recvframe_chkmic: get_stainfo==NULL!!!\n"));
		}
	}
exit:	
_func_exit_;
	return res;
}

//perform new defrag method, thomas 1202-07
union recv_frame * recvframe_defrag_new(_adapter *adapter,_queue *defrag_q,union recv_frame* precv_frame)
{
	_list	 *plist, *phead;	
	u8	wlanhdr_offset;
	struct recv_frame_hdr *pfhdr,*pnfhdr;
	union recv_frame* prframe,*prtnframe;

_func_enter_;		

	if( _queue_empty(defrag_q) == _FALSE ){

		phead = get_list_head(defrag_q);
		plist = get_next(phead);
		prframe = LIST_CONTAINOR(plist, union recv_frame, u);

		pfhdr=&prframe->u.hdr;
		pnfhdr=&precv_frame->u.hdr;
	
		//3 check the fragment sequence  (2nd ~n fragment frame)
		if(pfhdr->fragcnt != pnfhdr->attrib.frag_num) {
			//if miss 1 of the fragment
			//free the whole queue
			DEBUG_ERR(("Fragment number is wrong!!!,fragcnt = %d, frag_num = %d\n",pfhdr->fragcnt, pnfhdr->attrib.frag_num));
			pfhdr->fragcnt = 0;
			return NULL;
		}
	
		//3 copy the 2nd~n fragment frame's payload to the first fragment
		
		//get the 2nd~last fragment frame's payload
		
		wlanhdr_offset=pnfhdr->attrib.hdrlen+pnfhdr->attrib.iv_len;

		recvframe_pull(precv_frame, wlanhdr_offset);
			
		//append  to first fragment frame's tail (if privacy frame, pull the ICV)
		recvframe_pull_tail(prframe,pfhdr->attrib.icv_len);

		//memcpy
		_memcpy(pfhdr->rx_tail, pnfhdr->rx_data, pnfhdr->len);
		
		recvframe_put(prframe, pnfhdr->len);
		
		pfhdr->attrib.icv_len=pnfhdr->attrib.icv_len;	

		DEBUG_INFO(("Performance %d defrag!!!!!\n",pfhdr->fragcnt));

		if( pnfhdr->attrib.mfrag == 0) {
			list_delete(&prframe->u.hdr.list);
			pfhdr->attrib.mfrag = 0;
			pfhdr->fragcnt = 0;
			prtnframe = prframe;
		}
		else {
			prtnframe = precv_frame;
			pfhdr->fragcnt++;
		}

_func_exit_;	
		return prtnframe;

	}
	else {
		//defrag_q is empty
_func_exit_;
		DEBUG_ERR(("Missing frist fragment. Defrag queue is empty!!!!\n"));
		return NULL;
	}

}  

//check if need to defrag, if needed queue the first frame to defrag_q, and copy 2~n frame's payload to first frame
union recv_frame * recvframe_chk_defrag_new(_adapter *padapter,union recv_frame* precv_frame)
{
	u8	ismfrag;
	u8	fragnum;
	u8   *psta_addr;
	struct recv_frame_hdr *pfhdr;
	struct sta_info * psta;
	struct	sta_priv *pstapriv ;
	_list	 *phead,*plist;
	union recv_frame* prtnframe = NULL,*pfreeframe = NULL;
	_queue	 *pdefrag_q;
	struct intf_hdl * pintfhdl=&padapter->pio_queue->intf;

_func_enter_;		
	pstapriv = &padapter->stapriv;

	pfhdr=&precv_frame->u.hdr;

	//need to define struct of wlan header frame ctrl
	ismfrag= pfhdr->attrib.mfrag;
	fragnum=pfhdr->attrib.frag_num;
	
	psta_addr=pfhdr->attrib.ta;
	psta=get_stainfo(pstapriv, psta_addr);
	if(psta==NULL)
		pdefrag_q=NULL;
	else	
		pdefrag_q=&psta->sta_recvpriv.defrag_q;

	if((ismfrag==0)&&(fragnum==0)) {
		//isn't a fragment frame
		prtnframe = precv_frame;
		recvframe_chkmic(padapter,  prtnframe);
	}
	else if(pdefrag_q != NULL) {
		if(ismfrag==1){
			// 1~(n-1) fragment frame
			//copy payload to first pkt
			if(fragnum==0) 
			{	//the first fragment
				if(_queue_empty(pdefrag_q) == _FALSE) {
					//free current defrag_q
					DEBUG_ERR(("Another first fragment packet, free defrag_q \n"));
					phead = get_list_head(pdefrag_q);
					plist = get_next(phead);
					pfreeframe = LIST_CONTAINOR(plist, union recv_frame, u);
					list_delete(&pfreeframe->u.hdr.list);
					pfreeframe->u.hdr.fragcnt = 0;
					usb_read_port(pintfhdl, 0, 0, (unsigned char *)pfreeframe);
				}

				//Then enqueue the first fragment in the defrag_q
				phead = get_list_head(pdefrag_q);
				list_insert_tail(&pfhdr->list,phead);
				pfhdr->fragcnt ++;

				DEBUG_INFO(("Enqueu first fragment to defrag_q \n"));
				prtnframe = NULL;
			}
			else 
			{	//  2nd~(n-1) fragment
				prtnframe = recvframe_defrag_new(padapter,pdefrag_q,precv_frame);
				if( prtnframe == NULL ) {
					prtnframe = precv_frame;
				}
			}
		}  
		else if((ismfrag==0)&&(fragnum!=0)){
			//the last fragment frame
			prtnframe = recvframe_defrag_new(padapter,pdefrag_q,precv_frame);
			if( prtnframe == NULL ) {
				
				precv_frame->u.hdr.attrib.mfrag = 1;
				
				if(_queue_empty(pdefrag_q) == _FALSE) {
					//defrag queue is not empty, dequeue the first recv frame, and submit it
					DEBUG_INFO(("Frag_num is wrong, free defrag_q \n"));
					phead = get_list_head(pdefrag_q);
					plist = get_next(phead);
					pfreeframe = LIST_CONTAINOR(plist, union recv_frame, u);
					list_delete(&pfreeframe->u.hdr.list);
					usb_read_port(pintfhdl, 0, 0, (unsigned char *)pfreeframe);
					prtnframe = precv_frame;
				}
				else 
					prtnframe = precv_frame;

			}
			else {
				DEBUG_INFO(("Fragment complete \n"));
				recvframe_chkmic(padapter,  prtnframe);
				usb_read_port(pintfhdl, 0, 0, (unsigned char *)precv_frame);
			}
		}
	}
	else {
		prtnframe = precv_frame;
		DEBUG_ERR(("Can not  find this ta's defrag_queue\n"));
	}

_func_exit_;
	return prtnframe;

}

//perform defrag
union recv_frame * recvframe_defrag(_adapter *adapter,_queue *defrag_q)
{


	_list	 *plist, *phead;	
	u8	*data,wlanhdr_offset;
	u8   curfragnum;
	struct recv_frame_hdr *pfhdr,*pnfhdr;
	union recv_frame* prframe, *pnextrframe;
	_queue	*pfree_recv_queue;

_func_enter_;		
	curfragnum=0;
	pfree_recv_queue=&adapter->recvpriv.free_recv_queue;

	phead = get_list_head(defrag_q);			
	plist = get_next(phead);
	
	prframe = LIST_CONTAINOR(plist, union recv_frame, u);

	pfhdr=&prframe->u.hdr;
	list_delete(&(prframe->u.list));

	if(curfragnum!=pfhdr->attrib.frag_num) {
		//the first fragment number must be 0
		//free the whole queue
		 free_recvframe_queue(defrag_q, pfree_recv_queue);  
		return NULL;
	}
	curfragnum++;
	
	plist= get_list_head(defrag_q);

	plist = get_next(plist);

	data=get_recvframe_data(prframe);

	while(end_of_queue_search(phead, plist) == _FALSE){		
			
		pnextrframe = LIST_CONTAINOR(plist, union recv_frame , u);
		pnfhdr=&pnextrframe->u.hdr;
		//4 check the fragment sequence  (2ed ~n fragment frame)
		
		if(curfragnum!=pnfhdr->attrib.frag_num){
			//the fragment number must be increasing  (after decache) 
			//3 release the defrag_q & prframe
			free_recvframe(prframe, pfree_recv_queue);  
			free_recvframe_queue(defrag_q, pfree_recv_queue);  
			return NULL;
		}
		curfragnum++;

		//3 copy the 2nd~n fragment frame's payload to the first fragment
		
		//get the 2nd~last fragment frame's payload
		
		wlanhdr_offset=pnfhdr->attrib.hdrlen+pnfhdr->attrib.iv_len;

		recvframe_pull(pnextrframe, wlanhdr_offset);
			
		//append  to first fragment frame's tail (if privacy frame, pull the ICV)
		recvframe_pull_tail(prframe,pfhdr->attrib.icv_len);

		//memcpy
		_memcpy(pfhdr->rx_tail, pnfhdr->rx_data, pnfhdr->len);
		
		recvframe_put(prframe, pnfhdr->len);
		
		pfhdr->attrib.icv_len=pnfhdr->attrib.icv_len;
		plist = get_next(plist);
				
	};

	//free the defrag_q queue and return the prframe
	free_recvframe_queue(defrag_q, pfree_recv_queue);  
	DEBUG_INFO(("Performance defrag!!!!!\n"));
_func_exit_;	
	return prframe;

}  

//check if need to defrag, if needed queue the frame to defrag_q
union recv_frame * recvframe_chk_defrag(_adapter *padapter,union recv_frame* precv_frame)
{
	u8	ismfrag;
	u8	fragnum;
	u8   *psta_addr;
	struct recv_frame_hdr *pfhdr;
	struct sta_info * psta;
	struct	sta_priv *pstapriv ;
	_list	 *phead;
	union recv_frame* prtnframe = NULL;
	_queue	 *pfree_recv_queue, *pdefrag_q;

_func_enter_;		
	pstapriv = &padapter->stapriv;

	pfhdr=&precv_frame->u.hdr;
	
	pfree_recv_queue=&padapter->recvpriv.free_recv_queue;

	//need to define struct of wlan header frame ctrl
	ismfrag= pfhdr->attrib.mfrag;
	fragnum=pfhdr->attrib.frag_num;
	
	psta_addr=pfhdr->attrib.ta;
	psta=get_stainfo(pstapriv, psta_addr);
	if(psta==NULL)
		pdefrag_q=NULL;
	else	
		pdefrag_q=&psta->sta_recvpriv.defrag_q;

	if((ismfrag==0)&&(fragnum==0))
		//isn't a fragment frame
		prtnframe=precv_frame;
	
	if(ismfrag==1){
		// 0~(n-1) fragment frame
		//enqueue
		if(pdefrag_q != NULL){
		
			if(fragnum==0){
				//the first fragment
			
				//if(_queue_empty(&psta->sta_recvpriv.defrag_q) == _FALSE)
				if(_queue_empty(pdefrag_q) == _FALSE)
					//free current defrag_q
					free_recvframe_queue(pdefrag_q,pfree_recv_queue );
			}
			//Then enqueue the first fragment in the defrag_q
			//spinlock
			_spinlock(&pdefrag_q->lock);
			phead = get_list_head(pdefrag_q);
			list_insert_tail(&pfhdr->list,phead);
			//unlock
			_spinunlock(&pdefrag_q->lock);
			DEBUG_INFO(("Enqueuq: ismfrag = %d, fragnum= %d\n", ismfrag,fragnum));
			prtnframe=NULL;
		}
		else{
			//can't find this ta's defrag_queue, so free this recv_frame 
			free_recvframe(precv_frame,pfree_recv_queue);
			prtnframe=NULL;
			DEBUG_ERR(("Free because pdefrag_q ==NULL: ismfrag = %d, fragnum= %d\n", ismfrag,fragnum));
		}
	}  
	if((ismfrag==0)&&(fragnum!=0)){
		//the last fragment frame
		//enqueue the last fragment
		if(pdefrag_q != NULL){
			//spinlock
			_spinlock(&pdefrag_q->lock);
			phead = get_list_head(pdefrag_q);
			list_insert_tail(&pfhdr->list,phead);
			_spinunlock(&pdefrag_q->lock);
			//unlock
		
			//call recvframe_defrag to defrag
			DEBUG_INFO(("defrag: ismfrag = %d, fragnum= %d\n", ismfrag,fragnum));
			precv_frame=recvframe_defrag(padapter,pdefrag_q);
			prtnframe=precv_frame;
		
		}
		else{
			//can't find this ta's defrag_queue, so free this recv_frame 
			free_recvframe(precv_frame,pfree_recv_queue);
			prtnframe=NULL;
			DEBUG_ERR(("Free because pdefrag_q ==NULL: ismfrag = %d, fragnum= %d\n", ismfrag,fragnum));
		}
	}
	if((prtnframe!=NULL)&&(prtnframe->u.hdr.attrib.privacy)){
		//after defrag we must check tkip mic code 
		if(recvframe_chkmic(padapter,  prtnframe)==_FAIL){
			DEBUG_ERR(("\n    recvframe_chkmic(padapter,  prtnframe)==_FAIL \n"));
			free_recvframe(prtnframe,pfree_recv_queue);
			prtnframe=NULL;
		}
	}
_func_exit_;		
	return prtnframe;
}


//decrypt and set the ivlen,icvlen of the recv_frame
union recv_frame * decryptor(_adapter *padapter,union recv_frame * precv_frame)
{

	struct rx_pkt_attrib	*prxattrib = &precv_frame->u.hdr.attrib;
	struct security_priv	*psecuritypriv=&padapter->securitypriv;
	struct recv_stat *prxstat = get_recv_stat(precv_frame);
	union recv_frame * return_packet=precv_frame;
_func_enter_;

	if(prxattrib->encrypt>0)
	{
		if(prxstat->decrypted==0)
		{
			psecuritypriv->hw_decrypted=_FALSE;
			DEBUG_INFO(("prxstat->decrypted==0 psecuritypriv->hw_decrypted=_FALSE\n"));
			DEBUG_INFO(("perfrom software decryption! \n"));
			switch(prxattrib->encrypt){
				case _WEP40_:
				case _WEP104_:
					wep_decrypt(padapter, (u8 *)precv_frame);
					break;
				case _TKIP_:
					tkip_decrypt(padapter, (u8 *)precv_frame);
					break;
				case _AES_:
					aes_decrypt(padapter, (u8 * )precv_frame);
					break;
				default:
					break;
			}
		}
		else if(prxstat->decrypted==1)
		{
#ifdef CONFIG_RTL8711
			if((prxstat->icv==1)&&(prxattrib->encrypt!=_AES_)) //HW bug! Some AES packets are decrypted without error, but ICV is still set.
#else				
			if(prxstat->icv==1)
#endif
			{
				psecuritypriv->hw_decrypted=_FALSE;
				DEBUG_ERR(("decryptor: psecuritypriv->hw_decrypted=_FALSE\n"));
#ifdef CONFIG_SDIO_HCI
				free_recvframe(precv_frame,&padapter->recvpriv.free_recv_queue);
#endif
				return_packet=NULL;
			}
			else{
				psecuritypriv->hw_decrypted=_TRUE;
				DEBUG_ERR(("decryptor: psecuritypriv->hw_decrypted=_TRUE\n"));

			}
		}
	}
	
	//recvframe_chkmic(adapter, precv_frame);   //move to recvframme_defrag function
_func_exit_;		
	return return_packet;
}


//###set the security information in the recv_frame
union recv_frame * portctrl(_adapter *adapter,union recv_frame * precv_frame)
{
	u8   *psta_addr,*ptr;
	uint  auth_alg;	
	struct recv_frame_hdr *pfhdr;
	struct sta_info * psta;
	struct	sta_priv *pstapriv;
	struct recv_stat *prxstat;
	union recv_frame * prtnframe;
	u16	ether_type=0;
_func_enter_;			
	pstapriv = &adapter->stapriv;
	ptr=get_recvframe_data(precv_frame);
	pfhdr=&precv_frame->u.hdr;
	psta_addr=pfhdr->attrib.ta;
	psta=get_stainfo(pstapriv, psta_addr);
	
	auth_alg=adapter->securitypriv.dot11AuthAlgrthm;
	DEBUG_INFO(("########portctrl:adapter->securitypriv.dot11AuthAlgrthm= 0x%d\n",adapter->securitypriv.dot11AuthAlgrthm));
	if(auth_alg==2){
		if((psta!=NULL)&&( psta->ieee8021x_blocked)){
			//blocked
			//only accept EAPOL frame
			DEBUG_INFO(("########portctrl:psta->ieee8021x_blocked==1\n"));
		
			prtnframe=precv_frame;
		
			//get ether_type
			ptr=ptr+pfhdr->attrib.hdrlen+pfhdr->attrib.iv_len+LLC_HEADER_SIZE;
			_memcpy(&ether_type,ptr, 2);
			ether_type= ntohs((unsigned short )ether_type);

			if(ether_type == 0x888e){
				prtnframe=precv_frame;
			}
			else{  //free this frame
#ifdef CONFIG_SDIO_HCI
				free_recvframe(precv_frame, &adapter->recvpriv.free_recv_queue);
#endif
				prtnframe=NULL;
			}
		}
		else{ //allowed
			//check decryption status, and decrypt the frame if needed	
			DEBUG_INFO(("########portctrl:psta->ieee8021x_blocked==0\n"));
			DEBUG_INFO(("portctrl:precv_frame->u.hdr.attrib.privicy=%x\n",precv_frame->u.hdr.attrib.privacy));

			prxstat = get_recv_stat(precv_frame);

			if(prxstat->decrypted==0)
				DEBUG_INFO(("portctrl:prxstat->decrypted=%x\n",prxstat->decrypted));

			prtnframe=precv_frame;
			//check is the EAPOL frame or not (Rekey)
			if(ether_type == 0x888e){
				DEBUG_INFO(("########portctrl:ether_type == 0x888e\n"));
				//check Rekey
				prtnframe=precv_frame;
			}
			else{
				DEBUG_INFO(("########portctrl:ether_type != 0x888e\n"));
			}
		}
	}
	else
		prtnframe=precv_frame;
_func_exit_;	
		return prtnframe;
}


sint recv_decache(union recv_frame *precv_frame, u8 bretry, struct stainfo_rxcache *prxcache)
{
	sint tid = precv_frame->u.hdr.attrib.priority;

	u16 seq_ctrl = ( (precv_frame->u.hdr.attrib.seq_num&0xffff) << 4) | 
		(precv_frame->u.hdr.attrib.frag_num & 0xf);

_func_enter_;			
	if(bretry)
	{
		if(seq_ctrl == prxcache->tid_rxseq[tid])
			return _FAIL;
	}

	prxcache->tid_rxseq[tid] = seq_ctrl;
_func_exit_;		
	return _SUCCESS;

}


sint sta2sta_data_frame(
	_adapter *adapter, 
	union recv_frame *precv_frame, 
 	struct sta_info**psta
 )
{
	u8 *ptr = precv_frame->u.hdr.rx_data;
	sint ret = _SUCCESS;
	struct rx_pkt_attrib *pattrib = & precv_frame->u.hdr.attrib;
	struct	sta_priv 		*pstapriv = &adapter->stapriv;
//	struct	qos_priv   	*pqospriv= &adapter->qospriv;
//	struct	security_priv	*psecuritypriv = &adapter->securitypriv;
	struct	mlme_priv	*pmlmepriv = &adapter->mlmepriv;
	u8 *mybssid  = get_bssid(pmlmepriv);
	u8 *myhwaddr = myid(&adapter->eeprompriv);
	u8 * sta_addr = NULL;

	sint bmcast = IS_MCAST(pattrib->dst);
	
_func_enter_;		

 	if ((check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE)) 
	{

           // filter packets that SA is myself or multicast or broadcast
	    if (_memcmp(myhwaddr, pattrib->src, ETH_ALEN)){
		  DEBUG_ERR((" SA==myself \n"));
		  ret= _FAIL;
		  goto exit;
	    }		
        
	    if( (!_memcmp(myhwaddr, pattrib->dst, ETH_ALEN))	&& (!bmcast) ){
		  ret= _FAIL;
		  goto exit;
	    	}  
	    if( _memcmp(pattrib->bssid, "\x0\x0\x0\x0\x0\x0", ETH_ALEN) ||
		   _memcmp(mybssid, "\x0\x0\x0\x0\x0\x0", ETH_ALEN) ||
		   (!_memcmp(pattrib->bssid, mybssid, ETH_ALEN)) ) {
			ret= _FAIL;
			goto exit;
	    	}
	    sta_addr = pattrib->src;			
		
	}
	else if(check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE)
	{	
	      // For Station mode, sa and bssid should always be BSSID, and DA is my mac-address
		if(!_memcmp(pattrib->bssid, pattrib->src, ETH_ALEN) )
		{
			DEBUG_ERR(("bssid != TA under STATION_MODE; drop pkt\n"));
			ret= _FAIL;
			goto exit;
		}

	       sta_addr = pattrib->bssid;			

	 }
	 else if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
	 {
	      	if (bmcast)
		{
			// For AP mode, if DA == MCAST, then BSSID should be also MCAST
			if (!IS_MCAST(pattrib->bssid)){
					ret= _FAIL;
					goto exit;
			}
		}
		else // not mc-frame
		{
			// For AP mode, if DA is non-MCAST, then it must be BSSID, and bssid == BSSID
			if(!_memcmp(pattrib->bssid, pattrib->dst, ETH_ALEN)) {
				ret= _FAIL;
				goto exit;
			}

	       	sta_addr = pattrib->src;			
		
		}
		
	  }
	 else if(check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE)
	 {
              _memcpy(pattrib->dst, GetAddr1Ptr(ptr), ETH_ALEN);
	       _memcpy(pattrib->src, GetAddr2Ptr(ptr), ETH_ALEN);
	       _memcpy(pattrib->bssid, GetAddr3Ptr(ptr), ETH_ALEN);
  	       _memcpy(pattrib->ra, pattrib->dst, ETH_ALEN);
		_memcpy(pattrib->ta, pattrib->src, ETH_ALEN);

              sta_addr = mybssid;	 
	  }
	 else
	 {
	 	ret  = _FAIL;
	 }


	 
	if(bmcast)
		*psta = get_bcmc_stainfo(adapter);
	else
		*psta = get_stainfo(pstapriv, sta_addr); // get ap_info

	if (*psta == NULL) {
	       DEBUG_ERR(("can't get psta under sta2sta_data_frame ; drop pkt\n"));
#ifdef CONFIG_MP_INCLUDED		   
               if(check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE)
                    adapter->mppriv.rx_pktloss++;   
#endif			   
	       ret= _FAIL;
		   goto exit;
	}			
	 
exit:
_func_exit_;		
	return ret;

}


sint ap2sta_data_frame(
	_adapter *adapter, 
	union recv_frame *precv_frame, 
 	struct sta_info**psta )
{
	u8 *ptr = precv_frame->u.hdr.rx_data;
	struct rx_pkt_attrib *pattrib = & precv_frame->u.hdr.attrib;
	sint ret = _SUCCESS;
	struct	sta_priv 		*pstapriv = &adapter->stapriv;
//	struct	qos_priv   	*pqospriv= &adapter->qospriv;
//	struct	security_priv	*psecuritypriv = &adapter->securitypriv;
	struct	mlme_priv	*pmlmepriv = &adapter->mlmepriv;

	u8 *mybssid  = get_bssid(pmlmepriv);
	u8 *myhwaddr = myid(&adapter->eeprompriv);
	
	sint bmcast = IS_MCAST(pattrib->dst);

_func_enter_;	

	if ((check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE) && 
		(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) )
	{
	
	       // if NULL-frame
	      if ((GetFrameSubType(ptr)) == WIFI_DATA_NULL)
	      {
		   	DEBUG_INFO((" NULL frame \n"));	
			ret= _FAIL;
			goto exit;
	           
	       }
		   
              // filter packets that SA is myself or multicast or broadcast
	       if (_memcmp(myhwaddr, pattrib->src, ETH_ALEN)){
		     DEBUG_ERR((" SA==myself \n"));
			ret= _FAIL;
			goto exit;
		}	

		// da should be for me  
              if((!_memcmp(myhwaddr, pattrib->dst, ETH_ALEN))&& (!bmcast))
              {
                  DEBUG_INFO((" ap2sta_data_frame:  compare DA fail; DA= %x:%x:%x:%x:%x:%x \n",
					pattrib->dst[0],
					pattrib->dst[1],
					pattrib->dst[2],
					pattrib->dst[3],
					pattrib->dst[4],
					pattrib->dst[5]));
				   
				ret= _FAIL;
				goto exit;
              }
			  
		
		// check BSSID
		if( _memcmp(pattrib->bssid, "\x0\x0\x0\x0\x0\x0", ETH_ALEN) ||
		     _memcmp(mybssid, "\x0\x0\x0\x0\x0\x0", ETH_ALEN) ||
		     (!_memcmp(pattrib->bssid, mybssid, ETH_ALEN)) )
		{
                    DEBUG_INFO((" ap2sta_data_frame:  compare BSSID fail ; BSSID=%x:%x:%x:%x:%x:%x\n",
				pattrib->bssid[0],
				pattrib->bssid[1],
				pattrib->bssid[2],
				pattrib->bssid[3],
				pattrib->bssid[4],
				pattrib->bssid[5]));
				
			DEBUG_INFO(("mybssid= %x:%x:%x:%x:%x:%x\n", 
				mybssid[0],
				mybssid[1],
				mybssid[2],
				mybssid[3],
				mybssid[4],
				mybssid[5]));
                   
			ret= _FAIL;
			goto exit;
			}	

		if(bmcast)
			*psta = get_bcmc_stainfo(adapter);
		else
		       *psta = get_stainfo(pstapriv, pattrib->bssid); // get ap_info

		if (*psta == NULL) {
			DEBUG_ERR(("ap2sta: can't get psta under STATION_MODE ; drop pkt\n"));
			ret= _FAIL;
			goto exit;
		}

	}
       else if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE) && 
		     (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) )
	{      
	       _memcpy(pattrib->dst, GetAddr1Ptr(ptr), ETH_ALEN);
	       _memcpy(pattrib->src, GetAddr2Ptr(ptr), ETH_ALEN);
	       _memcpy(pattrib->bssid, GetAddr3Ptr(ptr), ETH_ALEN);
  	       _memcpy(pattrib->ra, pattrib->dst, ETH_ALEN);
		_memcpy(pattrib->ta, pattrib->src, ETH_ALEN);

		//
		_memcpy(pattrib->bssid,  mybssid, ETH_ALEN);
		  
		  
		   *psta = get_stainfo(pstapriv, pattrib->bssid); // get sta_info
		if (*psta == NULL) {
		       DEBUG_ERR(("can't get psta under MP_MODE ; drop pkt\n"));
			ret= _FAIL;
			goto exit;
		}

	
	}
	else
	{
		ret =  _FAIL;
	}
exit:	
_func_exit_;	
	return ret;

}




sint sta2ap_data_frame(
	_adapter *adapter, 
	union recv_frame *precv_frame, 
 	struct sta_info**psta )
{
//	u8 *ptr = precv_frame->u.hdr.rx_data;
	struct rx_pkt_attrib *pattrib = & precv_frame->u.hdr.attrib;
	struct	sta_priv 		*pstapriv = &adapter->stapriv;
//	struct	qos_priv   	*pqospriv= &adapter->qospriv;
//	struct	security_priv	*psecuritypriv = &adapter->securitypriv;
	struct	mlme_priv	*pmlmepriv = &adapter->mlmepriv;

	unsigned char *mybssid  = get_bssid(pmlmepriv);
//	unsigned char *myhwaddr = myid(&adapter->eeprompriv);
	sint ret=_SUCCESS;
	sint bmcast = IS_MCAST(pattrib->dst);

_func_enter_;


	if (check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
	{ 

		if (bmcast)
		{
			// For AP mode, if DA == MCAST, then BSSID should be also MCAST
			//bssid = mc-addr => psta=NULL
			if (!IS_MCAST(pattrib->bssid)){
				ret= _FAIL;
				goto exit;
			}
			*psta = get_bcmc_stainfo(adapter);
			
		}
		else // not mc-frame

		{
		
			// For AP mode, if DA is non-MCAST, then it must be BSSID, and bssid == BSSID
			if( (!_memcmp(mybssid, pattrib->dst, ETH_ALEN)) || 
				(!_memcmp(pattrib->bssid, mybssid, ETH_ALEN)) ){
					ret= _FAIL;
					goto exit;
				}

			*psta = get_stainfo(pstapriv, pattrib->src);

			if (*psta == NULL)
			{
				DEBUG_ERR(("can't get psta under AP_MODE; drop pkt\n"));
				ret= _FAIL;
				goto exit;
			}
			
		}

	}
exit:
_func_exit_;	
	return ret;

}

sint validate_recv_ctrl_frame(_adapter *adapter, union recv_frame *precv_frame)
{
	
	mgtctrl_frame(adapter, precv_frame);
	return _FAIL;  //_FAIL means this frame is not data frame
}

sint validate_recv_mgnt_frame(_adapter *adapter, union recv_frame *precv_frame)
{
	mgtctrl_frame(adapter, precv_frame);	
	return _FAIL;  //_FAIL means this frame is not data frame
}

sint validate_recv_data_frame(_adapter *adapter, union recv_frame *precv_frame)
{
	int res;	
	u8 bretry;
	u8 *psa, *pda, *pbssid;
	u8 *ptr = precv_frame->u.hdr.rx_data;
	struct	sta_info	*psta = NULL;
	struct	rx_pkt_attrib	*pattrib = & precv_frame->u.hdr.attrib;
	//struct	sta_priv 		*pstapriv = &adapter->stapriv;
	struct	security_priv	*psecuritypriv = &adapter->securitypriv;
	sint	ret=_SUCCESS;

_func_enter_;	
	bretry = GetRetry(ptr);
	pda = get_da(ptr);
	psa = get_sa(ptr);
	pbssid = get_hdr_bssid(ptr);

	if(pbssid == NULL){
		ret= _FAIL;
		goto exit;
	}

	_memcpy(pattrib->dst, pda, ETH_ALEN);
	_memcpy(pattrib->src, psa, ETH_ALEN);
	_memcpy(pattrib->bssid, pbssid, ETH_ALEN);

	switch(pattrib->to_fr_ds)
	{
		case 0:
			_memcpy(pattrib->ra, pda, ETH_ALEN);
			_memcpy(pattrib->ta, psa, ETH_ALEN);
			res= sta2sta_data_frame(adapter, precv_frame, &psta);
			break;

		case 1:	
			_memcpy(pattrib->ra, pda, ETH_ALEN);
			_memcpy(pattrib->ta, pbssid, ETH_ALEN);
			res= ap2sta_data_frame(adapter, precv_frame, &psta);
			break;

		case 2:
			_memcpy(pattrib->ra, pbssid, ETH_ALEN);
			_memcpy(pattrib->ta, psa, ETH_ALEN);
			res= sta2ap_data_frame(adapter, precv_frame, &psta);
			break;
		
		case 3:	
			_memcpy(pattrib->ra, GetAddr1Ptr(ptr), ETH_ALEN);
			_memcpy(pattrib->ta, GetAddr2Ptr(ptr), ETH_ALEN);
			res=_FAIL;
			DEBUG_ERR((" case 3\n"));
			break;		
		default:
			res=_FAIL;			
			break;			

	}

	if(res==_FAIL){
		DEBUG_INFO((" after to_fr_ds_chk; res = fail \n"));
		ret= res;
		goto exit;
	}
#if 0
	if(check_fwstate(pmlmepriv, WIFI_STATION_STATE)){
		psta = get_stainfo(pstapriv, pattrib->bssid); // get sta_info
		DEBUG_INFO(("\n pattrib->bssid=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
		pattrib->bssid[0],pattrib->bssid[1],pattrib->bssid[2],pattrib->bssid[3],pattrib->bssid[4],pattrib->bssid[5]));
	}
#endif
	if(psta==NULL){
		DEBUG_INFO((" after to_fr_ds_chk; psta==NULL \n"));
		ret= _FAIL;
		goto exit;
	}

#ifndef CONFIG_RTL8192U
	//update RSSI & Signal Quality ; 20061102 yitsen
	psta->rssi = prxstat->rssi;
	psta->signal_quality = prxstat->sq;
	
	if(pcur_bss!=NULL){
		pcur_bss->Rssi = prxstat->rssi;
	}
#endif

	// parsing QC field
	if(pattrib->qos == 1)
	{
		pattrib->priority = GetPriority((ptr + 24));
		pattrib->ack_policy =GetAckpolicy((ptr + 24));
		pattrib->hdrlen = pattrib->to_fr_ds==3 ? 32 : 26;
	}
	else
		pattrib->hdrlen = pattrib->to_fr_ds==3 ? 30 : 24;

#ifdef CONFIG_RTL8192U	
	if(pattrib->bContainHTC)
		pattrib->hdrlen += 4;  //HTC length
#endif

	// decache

	if(recv_decache(precv_frame, bretry, &psta->sta_recvpriv.rxcache) == _FAIL)
	{
		DEBUG_INFO(("decache : drop pkt\n"));
		ret= _FAIL;
		goto exit;
	}

	if(pattrib->privacy){
		DEBUG_INFO(("validate_recv_data_frame:pattrib->privicy=%x\n",pattrib->privacy));
		DEBUG_INFO(("\n ^^^^^^^^^^^IS_MCAST(pattrib->ra(0x%02x))=%d^^^^^^^^^^^^^^^\n",pattrib->ra[0],IS_MCAST(pattrib->ra)));
		GET_ENCRY_ALGO(psecuritypriv, psta, pattrib->encrypt,IS_MCAST(pattrib->ra));
		SET_ICE_IV_LEN(pattrib->iv_len, pattrib->icv_len, pattrib->encrypt);
	}
	else
	{
		pattrib->encrypt = 0;
		pattrib->iv_len = pattrib->icv_len = 0;
	}
		
exit:
_func_exit_;
	return ret;

}


sint validate_recv_frame(_adapter *adapter, union recv_frame *precv_frame)
{
	//shall check frame subtype, to / from ds, da, bssid
	//then call check if rx seq/frag. duplicated.

	u8 type;
	u8 subtype;
	sint retval = _SUCCESS;
	
	struct rx_pkt_attrib *pattrib = &precv_frame->u.hdr.attrib;
	
	u8 *ptr = precv_frame->u.hdr.rx_data;
	u8  ver =(unsigned char) (*ptr)&0x3 ;

_func_enter_;
	//add version chk

	if(ver!=0){
		printk("validate_recv_frame: ver!=0\n");
		retval= _FAIL;
		goto exit;
	}
	type =  GetFrameType(ptr);
	subtype = GetFrameSubType(ptr); //bit(7)~bit(2)

	pattrib->to_fr_ds = get_tofr_ds(ptr);
	if(pattrib->to_fr_ds == 3) //WDS packet is not acceted if we do not enable soft AP.  Soft AP is not available now.
	{
		retval = _FAIL;
		goto exit;
	}
	 
	pattrib->frag_num = GetFragNum(ptr);
	pattrib->seq_num = GetSequence(ptr);

	pattrib->pw_save = GetPwrMgt(ptr);
	pattrib->mfrag = GetMFrag(ptr);
	pattrib->mdata = GetMData(ptr);	
	pattrib->privacy =  GetPrivacy(ptr);

	switch(type)
	{
		case WIFI_MGT_TYPE: //mgnt
			retval=validate_recv_mgnt_frame(adapter, precv_frame);
			break;

		case WIFI_CTRL_TYPE://ctrl
			retval=validate_recv_ctrl_frame(adapter, precv_frame);
			break;

		case WIFI_DATA_TYPE: //data

			pattrib->qos = (subtype & BIT(7))? 1:0;
#ifdef CONFIG_RTL8192U
			//FIXME::
			//supporting HT is another information needed record  (bCurrentHTSupport)
			//
			if(pattrib->qos && 1)//FIXME: replace 1 with the managemnt info field of support_HT 
				pattrib->bContainHTC = GetOrder(ptr);
			else
				pattrib->bContainHTC = _FALSE;
#endif	
			retval=validate_recv_data_frame(adapter, precv_frame);

			if (retval==_FAIL)
				DEBUG_INFO(("\n  validate_recv_data_frame fail\n"));
			break;

		default:
			printk("validate_recv_frame\n");
			retval=_FAIL;
			break;
			
	}
exit:

_func_exit_;	
	return retval;

}


sint wlanhdr_to_ethhdr ( union recv_frame *precvframe)
{
	//remove the wlanhdr and add the eth_hdr

	sint rmv_len;
	u16 eth_type;
	u8	bsnaphdr;
	u8	*psnap_type;
	struct ieee80211_snap_hdr	*psnap;
	
	sint ret=_SUCCESS;
	_adapter	*adapter =precvframe->u.hdr.adapter;
	struct	mlme_priv	*pmlmepriv = &adapter->mlmepriv;

	u8* ptr = get_recvframe_data(precvframe) ; // point to frame_ctrl field
	struct rx_pkt_attrib *pattrib = & precvframe->u.hdr.attrib;
        struct _vlan *pvlan = NULL;

_func_enter_;	
	psnap=(struct ieee80211_snap_hdr	*)(ptr+pattrib->hdrlen + pattrib->iv_len);
	psnap_type=ptr+pattrib->hdrlen + pattrib->iv_len+SNAP_SIZE;
	 if (psnap->dsap==0xaa && psnap->ssap==0xaa && psnap->ctrl==0x03) {
		 if ( _memcmp(psnap->oui, oui_rfc1042, WLAN_IEEE_OUI_LEN)) {
   			bsnaphdr=_TRUE;//wlan_pkt_format = WLAN_PKT_FORMAT_SNAP_RFC1042;
  		}
  		else if (_memcmp(psnap->oui, SNAP_HDR_APPLETALK_DDP, WLAN_IEEE_OUI_LEN) &&
     			_memcmp(psnap_type, SNAP_ETH_TYPE_APPLETALK_DDP, 2) )
    			bsnaphdr=_TRUE;	//wlan_pkt_format = WLAN_PKT_FORMAT_APPLETALK;
  		else if (_memcmp( psnap->oui, oui_8021h, WLAN_IEEE_OUI_LEN))
   			bsnaphdr=_TRUE;	//wlan_pkt_format = WLAN_PKT_FORMAT_SNAP_TUNNEL;
  		else {
  			 DEBUG_ERR(("drop pkt due to invalid frame format!\n"));
   			ret= _FAIL;
			goto exit;
 		 }

 	}

	 else
  		bsnaphdr=_FALSE;//wlan_pkt_format = WLAN_PKT_FORMAT_OTHERS;

	rmv_len = pattrib->hdrlen + pattrib->iv_len +(bsnaphdr?SNAP_SIZE:0);
	DEBUG_INFO(("\n===pattrib->hdrlen: %x,  pattrib->iv_len:%x ===\n\n", pattrib->hdrlen,  pattrib->iv_len));
	
	if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE))	   	
       {
	   ptr += rmv_len ;	
          *ptr = 0x87;
	   *(ptr+1) = 0x11;
	   
	    //back to original pointer	 
	    ptr -= rmv_len;          
        } 
	
	ptr += rmv_len ;

	_memcpy(&eth_type, ptr, 2);
	eth_type= ntohs((unsigned short )eth_type); //pattrib->ether_type

	ptr +=2;
	
	if(pattrib->encrypt){
		recvframe_pull_tail(precvframe, pattrib->icv_len);	
	}

	
	if(eth_type == 0x8100) //vlan
	{
		pvlan = (struct _vlan *) ptr;
		
		//eth_type = get_vlan_encap_proto(pvlan);
                //eth_type = pvlan->h_vlan_encapsulated_proto;//?
		rmv_len += 4;
		ptr+=4;
	}


	if(eth_type==0x0800)//ip 
	{
		struct iphdr*  piphdr = (struct iphdr*) ptr;
	//	__u8 tos = (unsigned char)(pattrib->priority & 0xff);

	//	piphdr->tos = tos;

		if (piphdr->protocol == 0x06)
		{

			DEBUG_INFO(("@@@===recv tcp len:%d @@@===\n", precvframe->u.hdr.len));
		}
	}
	else if(eth_type==0x8711)// append rx status for mp test packets
	{
	       //ptr -= 16;
	       //_memcpy(ptr, get_rxmem(precvframe), 16);		 
	}
	else
	{
#ifdef PLATFORM_OS_XP
		NDIS_PACKET_8021Q_INFO VlanPriInfo;
		UINT32 UserPriority = precvframe->u.hdr.attrib.priority;
		UINT32 VlanID = (pvlan!=NULL ? get_vlan_id(pvlan) : 0 );

		VlanPriInfo.Value =          // Get current value.
      			NDIS_PER_PACKET_INFO_FROM_PACKET(precvframe->u.hdr.pkt, Ieee8021QInfo); 

		VlanPriInfo.TagHeader.UserPriority = UserPriority;
		VlanPriInfo.TagHeader.VlanId =  VlanID ;

		VlanPriInfo.TagHeader.CanonicalFormatId = 0; // Should be zero.
		VlanPriInfo.TagHeader.Reserved = 0; // Should be zero.
		NDIS_PER_PACKET_INFO_FROM_PACKET(precvframe->u.hdr.pkt, Ieee8021QInfo) = VlanPriInfo.Value;	
#endif		
	}

       if(eth_type==0x8711)// append rx status for mp test packets
      	{
              ptr = recvframe_pull(precvframe, (rmv_len-sizeof(struct ethhdr)+2)-16);       
	       _memcpy(ptr, get_rxmem(precvframe), 16);		
             ptr+=16;			  
      	}
       else
	ptr = recvframe_pull(precvframe, (rmv_len-sizeof(struct ethhdr)+2));


	_memcpy(ptr, pattrib->dst, ETH_ALEN);
	_memcpy(ptr+ETH_ALEN, pattrib->src, ETH_ALEN);

	eth_type = htons((unsigned short)eth_type) ;
	_memcpy(ptr+12, &eth_type, 2);
exit:	
_func_exit_;	
	return ret;

}


u8 process_packet(_adapter * padapter, union recv_frame *prframe, _pkt *pkt)
{
	u8 is_amsdu = _SUCCESS , retval = _SUCCESS ;
	
		retval=validate_recv_frame(padapter,prframe);
		if(retval !=_SUCCESS)
		{ 
			//free this recv_frame
			DEBUG_INFO(("validate_recv_frame: drop pkt \n"));
			retval = _FAIL;
			goto exit;

		}
	
		prframe=decryptor(padapter, prframe);
		if(prframe==NULL)
		{
//			goto submit_urb;
			retval = _FAIL;
			goto exit;
		}
		prframe=recvframe_chk_defrag_new(padapter, prframe);
		if(prframe==NULL) 
		{
			free_os_resource(pkt);
			
//			goto dequeue_next;
			retval = _SUCCESS;
			goto exit;
		}
		else if(prframe->u.hdr.attrib.mfrag == 1)
		{
		//	goto submit_urb;
			retval = _FAIL;
			goto exit;
		}
		prframe=portctrl(padapter,prframe);
		if(prframe==NULL)	
		{
		//	goto submit_urb;
			retval = _FAIL;
			goto exit;

		}
#ifdef CONFIG_RTL8192U
		is_amsdu = process_indicate_amsdu(padapter, prframe);
		if (is_amsdu == _SUCCESS) 
		{
		//	goto dequeue_next;
			retval = _SUCCESS;
			goto exit;

		}
#endif

		retval=wlanhdr_to_ethhdr (prframe);

		if(retval != _SUCCESS)
		{
		//	goto submit_urb;
			retval = _FAIL;
			goto exit;
		}
		if ((padapter->bDriverStopped ==_FALSE)&&( padapter->bSurpriseRemoved==_FALSE))
		{
			//Indicate this recv_frame
			retval = recv_indicatepkt(padapter, prframe, pkt);
		}
		else
			free_os_resource(pkt);
exit:
	return retval; 

}



#ifdef RX_FUNCTION
extern void  recv_function(union recv_frame *prframe)
{
	sint retval;
	u8 bGetresource;
	_pkt *pkt = NULL;
	_adapter * padapter = (_adapter *)prframe->u.hdr.adapter;
	union recv_frame *orig_prframe;
#ifndef CONFIG_USB_HCI
	_queue	*pfree_recv_queue=&padapter->recvpriv.free_recv_queue;
#endif
	struct intf_hdl	*pintfhdl=&padapter->pio_queue->intf;
	
#ifdef CONFIG_RTL8192U
	s8 is_amsdu = 0;
#endif
	

		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{
			DEBUG_ERR(("recv thread => bDriverStopped or bSurpriseRemoved \n"));
			goto exit;
		}



		if(prframe==NULL)    //no recv frame need to process
			goto exit;

		pkt = get_os_resource(&bGetresource);

		if( bGetresource == _FAIL )
		{

			usb_read_port(pintfhdl,0, 0, (unsigned char *)prframe);
			goto exit;
		}

		orig_prframe = prframe;

		//4 check the frame crtl field and decache
		retval=validate_recv_frame(padapter,prframe);
		if(retval !=_SUCCESS)
		{ 
			//free this recv_frame
			DEBUG_INFO(("validate_recv_frame: drop pkt \n"));
			goto submit_urb;
		}
	
		prframe=decryptor(padapter, prframe);
		if(prframe==NULL)
			goto submit_urb;
	
		prframe=recvframe_chk_defrag_new(padapter, prframe);
		if(prframe==NULL) 
		{
			free_os_resource(pkt);
			goto exit;
		}
		else if(prframe->u.hdr.attrib.mfrag == 1)
			goto submit_urb;
	
		prframe=portctrl(padapter,prframe);
		if(prframe==NULL)	
			goto submit_urb;

#ifdef CONFIG_RTL8192U
		is_amsdu = process_indicate_amsdu(padapter, prframe);
		if (is_amsdu == _SUCCESS) 
			goto exit;
#endif

		retval=wlanhdr_to_ethhdr (prframe);

		if(retval != _SUCCESS)
			goto submit_urb;

		if ((padapter->bDriverStopped ==_FALSE)&&( padapter->bSurpriseRemoved==_FALSE))
		{
			//Indicate this recv_frame
			recv_indicatepkt(padapter, prframe, pkt);
		}
		else
			free_os_resource(pkt);
		
		goto exit;

submit_urb:

		free_os_resource(pkt);

#ifdef CONFIG_USB_HCI
		usb_read_port(pintfhdl,0, 0, (unsigned char *)orig_prframe);
#else
		free_recvframe(prframe, pfree_recv_queue);
#endif

exit:
	return ; 
	
}
#endif /*RX_FUNCTION*/



extern thread_return recv_thread(thread_context context)
{
//	sint retval;
	u8 bGetresource, ret = _SUCCESS;
	_pkt *pkt = NULL;
	_adapter * padapter = (_adapter *)context;
	struct recv_priv	*precvpriv = &padapter->recvpriv;
	union recv_frame *prframe, *orig_prframe;
#ifdef LINUX_TASKLET

	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
#endif

#ifndef CONFIG_USB_HCI
	_queue	*pfree_recv_queue=&padapter->recvpriv.free_recv_queue;
#endif
	_queue	*precv_pending_queue=&padapter->recvpriv.recv_pending_queue;
	struct intf_hdl	*pintfhdl=&padapter->pio_queue->intf;
	
#ifdef CONFIG_RTL8192U
//	s8 is_amsdu = 0;

#ifdef USB_RX_AGGREGATION_SUPPORT
	u32 pkt_len = 0 , temp32 = 0  , total_len = 0 ;
	u8  drvinfo_sz = 0 , tempbyte  = 0 ; 
	struct recv_stat *prxstat = NULL;
	_pkt *pold_skb = NULL;
	u8 *pframe_head = NULL, *pcurrent_pkt = NULL;
	struct rx_pkt_attrib *prx_attrib = NULL;
	PURB ptempurb;
#endif


#endif


	daemonize_thread(padapter);

	while(1)
	{
		
		if ((_down_sema(&(precvpriv->recv_sema))) == _FAIL)
			break;
dequeue_next:
		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{
			DEBUG_ERR(("recv thread => bDriverStopped or bSurpriseRemoved \n"));
			break;
		}
#ifdef LINUX_TASKLET		
		if(check_fwstate(pmlmepriv,_FW_LINKED)==_TRUE)
		{
			DEBUG_INFO(("recv_thread: FW_LINKED bread\n"));
			break;
		}
#endif
		prframe = dequeue_recvframe(precv_pending_queue);
				   
		if(prframe==NULL)    //no recv frame need to process
			continue;

		pkt = get_os_resource(&bGetresource);

		if( bGetresource == _FAIL )
		{
			usb_read_port(pintfhdl,0, 0, (unsigned char *)prframe);
			goto dequeue_next;
		}

		orig_prframe = prframe;


#ifdef USB_RX_AGGREGATION_SUPPORT
		pframe_head = prframe->u.hdr.rx_head;
		tempbyte = *(u8 *)(prframe->u.hdr.rx_head+sizeof(struct recv_stat));
		prxstat = get_recv_stat(prframe);

		total_len = prxstat->frame_length;
		drvinfo_sz =prxstat->RxDrvInfoSize; 

		DEBUG_ERR(("pframe head addr: %p, length : %d\n", pframe_head, total_len));
		if(tempbyte & BIT(0))
		{
			DEBUG_ERR(("recv_thread: This is a RX AGGREGATION packet, total length: %d\n", total_len));
			temp32 = *(u32 *)(prframe->u.hdr.rx_head+sizeof(struct recv_stat)+drvinfo_sz-4);
						
			prxstat->frame_length= (u16 )(temp32 & 0x3FFF) ;
			
		}
		
		
		pold_skb = prframe->u.hdr.pkt;

		while(total_len >= sizeof(struct recv_stat) + drvinfo_sz)
		{

			prframe->u.hdr.pkt = skb_clone(pold_skb, GFP_ATOMIC);
			
			if(prframe->u.hdr.pkt == NULL) 
			{
				DEBUG_ERR(("recv_thread: alloc skb failed\n"));
				free_os_resource(pkt);	
				goto usb_submit;

			}
			memset((u8 *)prframe, 0 , sizeof(union recv_frame));
			pcurrent_pkt = (u8 *)(pframe_head+pkt_len);

			if(pcurrent_pkt == pframe_head) // first packet
			{
				pkt_len +=8; //Add 8 shift bytes in RxPacketBuffer
				prx_attrib->bIsRxAggrSubframe = _FALSE;
			}
			else
			{
			
				tempbyte = (*(pcurrent_pkt + 1)) & 0xc0; 

				//Copy frame length from offset 44 to offset0
				_memcpy(pcurrent_pkt, (pcurrent_pkt+44), 2);
				*(pcurrent_pkt+1) |=  tempbyte;

				//Transfer Receive Packet Buffer Layout format to Rx Status Desc
				_memcpy((pcurrent_pkt+2), (pcurrent_pkt+3), 1);
				_memcpy((pcurrent_pkt+3), (pcurrent_pkt+4), 1);
			}
			
			prframe->u.hdr.pkt->tail = prframe->u.hdr.pkt->data = pcurrent_pkt;
			
			prframe->u.hdr.rx_tail= prframe->u.hdr.rx_data=prframe->u.hdr.rx_head=pcurrent_pkt;
 			
			//prframe->u.hdr.rx_end=prframe->u.hdr.pkt->end;
			prxstat = get_recv_stat(prframe);
			prx_attrib = &(prframe->u.hdr.attrib);
			prx_attrib->bIsRxAggrSubframe =_TRUE;

			
			
			query_rx_desc_status(padapter, prframe);
			pkt_len = prxstat->frame_length; 
			pkt_len += GetRxPacketShiftBytes819xUsb(prframe);
			// 256 bytes alignment 
			if( (pkt_len & 0xFF)!=0) 
				pkt_len = (pkt_len & 0xFFFFFF00) + 256;

			if(pcurrent_pkt == pframe_head) // first packet
				pkt_len -= 8; // first pakcet doesn't contain 8 bytes shift
			
				
			if( total_len>= pkt_len)
				total_len -= pkt_len;
			else
				total_len = 0 ; 
			
			recvframe_put(prframe, prxstat->frame_length + GetRxPacketShiftBytes819xUsb(prframe));						
			recvframe_pull(prframe, GetRxPacketShiftBytes819xUsb(prframe));
			recvframe_pull_tail(prframe, 4);	// remove FCS 
				

			//FIXME: the format of 8192u is very different from 8187
			//the length needed modified

			//move the rx_data pointer, skip the descriptor and drvInfo
			//then, move the rx_tail pointer to the end of the buffer
		
		
			
			if(prframe->u.hdr.len != (prxstat->frame_length-sCrcLng))
			{
				DEBUG_ERR(("recv_entry:precvframe->u.hdr.len != (prxstat->frame_length-4)%d, %d\n", precvframe->u.hdr.len, prxstat->frame_length));
				continue;
			}
#endif
		
			   ret = process_packet(padapter, prframe, pkt);
		
#if 	0
		//4 check the frame crtl field and decache
		retval=validate_recv_frame(padapter,prframe);
		if(retval !=_SUCCESS)
		{ 
			goto submit_urb;
		}
	
		prframe=decryptor(padapter, prframe);
		if(prframe==NULL)
			goto submit_urb;
	
		prframe=recvframe_chk_defrag_new(padapter, prframe);
		if(prframe==NULL) 
		{
			free_os_resource(pkt);
			goto dequeue_next;
		}
		else if(prframe->u.hdr.attrib.mfrag == 1)
			goto submit_urb;
	
		prframe=portctrl(padapter,prframe);
		if(prframe==NULL)	
			goto submit_urb;

#ifdef CONFIG_RTL8192U
		is_amsdu = process_indicate_amsdu(padapter, prframe);
		if (is_amsdu == _SUCCESS) 
		{
			prframe->u.hdr.pkt = pkt;
			usb_read_port(pintfhdl, 0, 0, (u8*)orig_prframe);
			goto dequeue_next;
		}
#endif

		retval=wlanhdr_to_ethhdr (prframe);

		if(retval != _SUCCESS)
			goto submit_urb;

		if ((padapter->bDriverStopped ==_FALSE)&&( padapter->bSurpriseRemoved==_FALSE))
		{
			//Indicate this recv_frame
			recv_indicatepkt(padapter, prframe, pkt);
		}
		else
			free_os_resource(pkt);


		
		goto dequeue_next;

#endif
	
#ifdef USB_RX_AGGREGATION_SUPPORT
		
		
		
		}// end of while

		prframe->u.hdr.pkt = pkt;
		free_os_resource(pold_skb);
#endif

		
		if(ret != _SUCCESS)
			free_os_resource(pkt);			
usb_submit:
	
#ifdef CONFIG_USB_HCI
		usb_read_port(pintfhdl,0, 0, (unsigned char *)orig_prframe);
#else
		free_recvframe(prframe, pfree_recv_queue);
#endif

		goto dequeue_next;
		
		flush_signals_thread();
	}
#ifndef LINUX_TASKLET	
	_up_sema(&precvpriv->terminate_recvthread_sema);

	exit_thread(NULL, 0);
#else
	padapter->rx_thread_complete = _TRUE;
	exit_thread(NULL, 0);
#endif	
}


#if defined (LINUX_TASKLET) && (PLATFORM_LINUX)
void recv_tasklet(_adapter * padapter )
{
	sint retval;
	_pkt *skb;

	union recv_frame *prframe, *orig_prframe;
	_queue	*precv_pending_queue=&padapter->recvpriv.recv_pending_queue;
	struct intf_hdl * pintfhdl=&padapter->pio_queue->intf;

	while(1)
	{
dequeue_next:
		if (padapter->bDriverStopped == _TRUE)
			break;

		//dequeue from recv_pending_queue
		//prframe=alloc_recvframe(precv_pending_queue );
	       prframe = dequeue_recvframe(precv_pending_queue);

		if(prframe==NULL)    //no recv frame need to process
			break;

		//allocate skb buff for next usb_read_port 
		//if kernel has no space to allocate, reuse it
		skb =  alloc_skb(SZ_RECVFRAME, GFP_ATOMIC);
		
		if( skb != NULL ) 
		{
			//use to restore recv frame if dropped
			orig_prframe = prframe;
			
			//4 check the frame crtl field and decache
			retval=validate_recv_frame(padapter,prframe);
			if(retval !=_SUCCESS)
			{
				DEBUG_INFO(("validate_recv_frame: drop pkt \n"));
				goto submit_urb;
			}
	
			prframe=decryptor(padapter, prframe);
			if(prframe==NULL)
				goto submit_urb;

			prframe=recvframe_chk_defrag_new(padapter, prframe);
			if(prframe==NULL) {
				dev_kfree_skb_any(skb);
				goto dequeue_next;
			}
			else if(prframe->u.hdr.attrib.mfrag == 1)
				goto submit_urb;
	
			prframe=portctrl(padapter,prframe);
			if(prframe==NULL)	
				goto submit_urb;

			retval=wlanhdr_to_ethhdr (prframe);
			if(retval != _SUCCESS)
				goto submit_urb;

			//Indicate this recv_frame
			recv_indicatepkt(padapter,prframe,skb);
		}
		else
		{
			DEBUG_ERR(("No Space for allocate skb buff \n"));
			usb_read_port(pintfhdl,0, 0, (unsigned char *)prframe);
		}

		goto dequeue_next;

submit_urb:
		dev_kfree_skb_any(skb);
		usb_read_port(pintfhdl,0, 0, (unsigned char *)orig_prframe);
	}
}
#endif

