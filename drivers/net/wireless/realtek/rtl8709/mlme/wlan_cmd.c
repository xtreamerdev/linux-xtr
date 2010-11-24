/*

	MIPS-16 Mode if CONFIG_MIPS16 is on

*/


#define _RTL8187_RELATED_FILES_C
#define _WLAN_CMD_C_
#include <drv_conf.h>
#include <wlan_bssdef.h>
#include <drv_types.h>
#include <wlan_mlme.h>
#include <wlan_util.h>
#include <wlan_sme.h>

#include <rtl8192u_spec.h>
#include <rtl8192u_regdef.h>

#include <circ_buf.h>
#include <wifi.h>

extern void start_create_bss(_adapter *padapter);

void c2hclrdsr(_adapter *padapter)
{
	struct	event_node *node;
	volatile int    *caller_ff_tail;
	int     caller_ff_sz;
	volatile int *head = &(padapter->_sys_mib.c2hevent.head);
	volatile int *tail = &(padapter->_sys_mib.c2hevent.tail);
		
 	node = &(padapter->_sys_mib.c2hevent.nodes[*tail]);

	//first CIRC_CNT checks the queue is not empty
	if (CIRC_CNT(*head, *tail, C2HEVENT_SZ)) {
#ifdef CONFIG_POWERSAVING
            	CLR_DATA_OPEN(ps_data_open, C2H_DATA_OPEN);
#endif			
		*tail = ((*tail) + 1) & (C2HEVENT_SZ - 1);
       	}
	else{
		printk("c2hclrdsr: !!ERROR!! head = %d, tail = %d\n", *head, *tail);
            	goto exit;
	}

	caller_ff_tail = node->caller_ff_tail;
	caller_ff_sz = node->caller_ff_sz;
	
	 //caller_ff_tail is a pointer. Only _Survey_EVT_ and _DelSTA_EVT_ will get a non-zero value here.
        	if (caller_ff_tail) 
            *caller_ff_tail = ((*caller_ff_tail) + 1) & (caller_ff_sz - 1);

	//second CIRC_CNT has no effect in driver.
	//in FW, it make sure that the next event in event_queue will be posted to HW by start_hw_event_posting.
        	if (CIRC_CNT(*head, *tail, C2HEVENT_SZ) == 0) {
#ifdef CONFIG_POWERSAVING
                	CLR_DATA_OPEN(ps_data_open, C2H_DATA_OPEN);
#endif
              	DEBUG_INFO(("\n dequeue_event:no pending event(2)!!!\n"));
            	goto exit;
		}

exit:
	DEBUG_INFO(("\n dequeue_event:end!!!\n"));
	return;
}

struct event_frame_priv *alloc_evt_frame(struct evt_priv *pevtpriv, _queue *pqueue)
{			
	struct event_frame_priv *peventframe =  NULL;
	_list	*plist, *phead;
	_irqL irqL;	

	_func_enter_;	
	
	_enter_critical(&(pqueue->lock), &irqL);
	if(_queue_empty(pqueue) == _TRUE)
	{
		//DEBUG_ERR(("!!ERROR!! no event queue frame\n"));
		peventframe =  NULL;
	}
	else
	{

		phead = get_list_head(pqueue);

		plist = get_next(phead);

		peventframe = LIST_CONTAINOR(plist, struct event_frame_priv, list);

		list_delete(&(peventframe->list));

	}
	_exit_critical(&(pqueue->lock), &irqL);
_func_exit_;	
	return peventframe;

}


void enqueue_evt_frame(struct event_frame_priv *peventframe, struct evt_priv *pevtpriv)
{
	_irqL irqL;
	
	_enter_critical(&(pevtpriv->pending_event_queue.lock), &irqL);
	
	list_insert_tail(&peventframe->list, &(pevtpriv->pending_event_queue.queue) );

	_exit_critical(&(pevtpriv->pending_event_queue.lock), &irqL);
	
}

void free_evt_frame(struct event_frame_priv *peventframe, struct evt_priv *pevtpriv)
{
	_irqL irqL;
	
	//send this list to event_frame pool
	_enter_critical(&(pevtpriv->free_event_queue.lock), &irqL);
	
	list_insert_tail(&peventframe->list, &(pevtpriv->free_event_queue.queue) );

	_exit_critical(&(pevtpriv->free_event_queue.lock), &irqL);
	
}

void start_hw_event_posting(_adapter *padapter)
{
	struct	event_node *node;
	
//	volatile int *head = &(padapter->_sys_mib.c2hevent.head);
	volatile int *tail = &(padapter->_sys_mib.c2hevent.tail);
	//unsigned long	flags;

	struct event_frame_priv *peventframe;


#ifdef CONFIG_POWERSAVING
	local_irq_save(flags);
	ps_32K_isr_patch();
	local_irq_restore(flags);
#endif

	node = &(padapter->_sys_mib.c2hevent.nodes[*tail]);

	//assign event code, size...
	//printk("tail:%d, evt_code:%d, evt_sz:%d\n",
	//*tail, node->evt_code,node->evt_sz); 

	peventframe = alloc_evt_frame(&padapter->evtpriv, &padapter->evtpriv.free_event_queue);
	peventframe->c2h_res = (node->evt_code << 24) | ((node->evt_sz) << 8) |padapter->_sys_mib.c2hevent.seq++;
	peventframe->c2h_buf=node->node;
	enqueue_evt_frame(peventframe, &padapter->evtpriv);
	if(node->evt_code == 1)
		DEBUG_INFO(("######## enqueuing a surveydone_event\n"));

	
	evt_notify_isr(&padapter->evtpriv);
//	rtl_outl(C2HR, ((unsigned int)c2hbuf | 0x01));
//	rtl_inl(C2HR);


	if(node->evt_code== 2)
	{
		DEBUG_INFO(("start_hw_event_posting(2): joinbss_done->network.join_res=%x \n", padapter->_sys_mib.joinbss_done.network.join_res));
		DEBUG_INFO(("address of joinbss_done=%p \n", &(padapter->_sys_mib.joinbss_done) ));
		DEBUG_INFO(("address of padapter->_sys_mib.c2h_buf=%p\n", padapter->_sys_mib.c2h_buf ));
	}

//exit:
	DEBUG_INFO(("\n dequeue_event:end!!!\n"));

	return;
}


unsigned char joinbss_hdl(struct _ADAPTER *padapter, unsigned char *pbuf)
{
	u8 bool;
	unsigned int i;
//	NDIS_WLAN_BSSID_EX* pwlanbssidex = (NDIS_WLAN_BSSID_EX*)pbuf;
	struct mib_info *sys_mib = &padapter->_sys_mib;
	
#ifdef CONFIG_RTL8192U
	PBSS_QOS PBssQos = NULL;
#endif
	
	if (sys_mib->mac_ctrl.opmode & WIFI_ASOC_STATE)
	{
		//FIXME: Add papapter to issue_deauth()
		issue_deauth(padapter, sys_mib->cur_network.MacAddress, 3);	
		del_assoc_sta(padapter, sys_mib->cur_network.MacAddress);
		sys_mib->mac_ctrl.opmode ^= WIFI_ASOC_STATE;
	}

	while(1) {
		_cancel_timer(&sys_mib->disconnect_timer, &bool);
		if(bool == _TRUE)
			break;
	}

	for (i = 5; i < MAX_STASZ; i++)
	{
		if (sys_mib->sta_data[i].status == 1)
		{	
			free_stadata(&(sys_mib->sta_data[i]));
		}
	}
/*	{	int i;
		NDIS_WLAN_BSSID_EX *pwlan;
		printk("\n pbuf= \n");
		for(i=0;i< sizeof(NDIS_WLAN_BSSID_EX);i=i+8)
			printk("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",pbuf[i],pbuf[i+1],pbuf[i+2],pbuf[i+3],pbuf[i+4],pbuf[i+5],pbuf[i+6],pbuf[i+7]);
		pwlan=(NDIS_WLAN_BSSID_EX *)pbuf;
		printk("\n IEs= \n");
                for(i=0;i< 192;i=i+8)
                        printk("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",pwlan->IEs[i],pwlan->IEs[i+1],pwlan->IEs[i+2],pwlan->IEs[i+3],pwlan->IEs[i+4],pwlan->IEs[i+5],pwlan->IEs[i+6],pwlan->IEs[i+7]);
	} */	
	_memcpy((void *)&(sys_mib->cur_network), (void *)pbuf, sizeof(NDIS_WLAN_BSSID_EX)); 

#ifdef CONFIG_RTL8192U
	PBssQos = (PBSS_QOS)(& (sys_mib->cur_network.BssQos ));

	for(i = 0 ; i < 4 ; i ++)
		PBssQos->AcParameter[i].longData = sys_mib->edca_param[i];

#endif

/*	{       int i;
		NDIS_WLAN_BSSID_EX *pwlan;
		u8 *buf=(u8 *)&(sys_mib->cur_network);
                printk("\nsys_mib->cur_network = \n");
                for(i=0;i< sizeof(NDIS_WLAN_BSSID_EX);i=i+8)
                        printk("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",buf[i],buf[i+1],pbuf[i+2],buf[i+3],buf[i+4],buf[i+5],buf[i+6],buf[i+7]);
		 pwlan=(NDIS_WLAN_BSSID_EX *)buf;
                printk("\n IEs= \n");
                for(i=0;i< 192;i=i+8)
                        printk("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",pwlan->IEs[i],pwlan->IEs[i+1],pwlan->IEs[i+2],pwlan->IEs[i+3],pwlan->IEs[i+4],pwlan->IEs[i+5],pwlan->IEs[i+6],pwlan->IEs[i+7]);
        
} */  
	sys_mib->join_req_ongoing = 1;
	
	sys_mib->cur_channel = le32_to_cpu(sys_mib->cur_network.Configuration.DSConfig);
		
	start_clnt_join(padapter);

	cmd_clr_isr(&padapter->cmdpriv);

	return 	H2C_SUCCESS;
}








unsigned char disconnect_hdl(_adapter *padapter, unsigned char *pbuf)
{
	u8 bool;
	
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	if (_sys_mib->mac_ctrl.opmode & WIFI_ASOC_STATE)
	{
		if(_sys_mib->mac_ctrl.opmode & WIFI_STATION_STATE) 
		{
			issue_deauth(padapter, _sys_mib->cur_network.MacAddress, 3);
			_sys_mib->mac_ctrl.opmode &= ~(WIFI_STATION_STATE | WIFI_ASOC_STATE);
		}
		else if (_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)
		{
			_sys_mib->mac_ctrl.opmode &= ~(WIFI_ADHOC_STATE | WIFI_ASOC_STATE | WIFI_ADHOC_BEACON);
		}
		del_assoc_sta(padapter, _sys_mib->cur_network.MacAddress);

#ifdef TODO
		write8(padapter, MSR, MSR_LINK_ENEDCA|NOLINKMODE);
#endif
		write8(padapter, MSR, NOLINKMODE);

	}
	
	while(1) {
		_cancel_timer(&_sys_mib->disconnect_timer, &bool);
		if(bool == _TRUE)
			break;
	}

	cmd_clr_isr(&padapter->cmdpriv);
	return 	H2C_SUCCESS;
}







unsigned char createbss_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

#ifdef CONFIG_RTL8192U

	PBSS_QOS PBssQos = NULL;

#endif
	u8 i  = 0;

	if (_sys_mib->mac_ctrl.opmode & WIFI_ASOC_STATE)
	{
		issue_deauth(padapter, _sys_mib->cur_network.MacAddress, 3);	
		del_assoc_sta(padapter, _sys_mib->cur_network.MacAddress);
		_sys_mib->mac_ctrl.opmode ^= WIFI_ASOC_STATE;
	}
	
	_memcpy((void *)&(_sys_mib->cur_network), (void *)pbuf, sizeof(NDIS_WLAN_BSSID_EX));  
		
#ifdef CONFIG_RTL8192U

	PBssQos = (PBSS_QOS)(& (_sys_mib->cur_network.BssQos ));

	for(i = 0 ; i < 4 ; i ++)
		PBssQos->AcParameter[i].longData = _sys_mib->edca_param[i];

#endif
		
	_sys_mib->cur_channel = le32_to_cpu(_sys_mib->cur_network.Configuration.DSConfig);
	printk("->cur channel: %x\n", _sys_mib->cur_channel);
	
	start_create_bss(padapter);
	cmd_clr_isr(&padapter->cmdpriv);
	return 	H2C_SUCCESS;
}


unsigned char setopmode_hdl(_adapter *padapter,unsigned char *pbuf)
{
	 cmd_clr_isr(&padapter->cmdpriv);
	return 	H2C_SUCCESS;
}


unsigned char sitesurvey_hdl(_adapter *padapter,unsigned char *pbuf)
{
	struct sitesurvey_parm *pparm = (struct sitesurvey_parm *)pbuf;

	DEBUG_INFO(("get sitesurvey cmd\n"));

	if (padapter->_sys_mib.sitesurvey_res.state == _TRUE){
		cmd_clr_isr(&padapter->cmdpriv);
		DEBUG_ERR(("sitesurvey_hdl: return H2C_REJECTED\n"));
		return H2C_REJECTED;
	}
#ifdef CONFIG_RTL8711
#if 1	
	flush_fw_fifo();
#endif

	/* PAUSE all tx except management . BIT10 BE, BIT13 MGT*/
#ifdef MGT_CRTL_QUEUE			
	rtl_outl(ACCTRL, (rtl_inl(ACCTRL) & 0xffff00ff) | 0xdf00);
#else
	rtl_outl(ACCTRL, (rtl_inl(ACCTRL) & 0xffff00ff) | 0xfb00);
#endif
#endif /*CONFIG_RTL8711*/

	padapter->_sys_mib.sitesurvey_res.state = _TRUE;
	padapter->_sys_mib.sitesurvey_res.bss_cnt = 0;
	
	if (le32_to_cpu(pparm->ss_ssidlen))
		_memcpy(padapter->_sys_mib.sitesurvey_res.ss_ssid, pparm->ss_ssid,le32_to_cpu(pparm->ss_ssidlen));
		
	padapter->_sys_mib.sitesurvey_res.ss_ssidlen = le32_to_cpu(pparm->ss_ssidlen);
	
	padapter->_sys_mib.sitesurvey_res.passive_mode = le32_to_cpu(pparm->passive_mode);
	
//	rtl_outl(0xbd200240, 0x0100888a); //8711 control
	
	DEBUG_INFO(("\ncall sitesurvey cmd\n"));
	site_survey(padapter);

	DEBUG_INFO(("-sitesurvey_hdl: return H2C_SUCCESS\n"));
	return 	H2C_SUCCESS;
}



//unsigned char key_id;

/*

struct setkey_parm {	
unsigned char	algorithm;	// encryption algorithm, could be none, wep40, TKIP, CCMP, wep104	
unsigned char	keyid;		// this is only valid for legendary wep, 0~3 for key id.
unsigned char grpkey;		//1: this is the grpkey for 802.1x. 0: this is the unicast key for 802.1x
unsigned char	key[16];		// this could be 40 or 104
};

This will be used to set up the default key and group key

*/
unsigned char setkey_hdl(_adapter *padapter, unsigned char *pbuf)
{	
	unsigned short	ctrl;
	struct setkey_parm 	*pparm = (struct setkey_parm *)pbuf;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	
	printk("setkey_hdl: keyid=%x  algorithm=%x\n", pparm->keyid, pparm->algorithm);	
	_sys_mib->mac_ctrl.defkeyid = pparm->keyid;

	ctrl = BIT(15) | ((pparm->algorithm) << 2) | pparm->keyid;	
	if((pparm->algorithm != 2)&&(pparm->algorithm!=4))	//WEP40 or WEP104
	{	
		setup_assoc_sta((pseudo_sta0 + (8 * pparm->keyid)), pparm->keyid, -1, ctrl, pparm->key, padapter);
	}
	else												//TKIP or AES(TKIP==2; AES==4)
	{
		setup_assoc_sta(broadcast_sta, 4, 4, ctrl, pparm->key, padapter);	
	}
	cmd_clr_isr(&padapter->cmdpriv);
	return 	H2C_SUCCESS;
	
}


unsigned char set_assocsta_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct set_assocsta_parm  *pparm = (struct set_assocsta_parm  *)pbuf;
	struct sta_data *psta;
	printk("set_assocsta_hdl\n");
	del_assoc_sta(padapter, pparm->addr);
	
	setup_assoc_sta(pparm->addr, -1, 0, (BIT(15) | BIT(5)), null_key, padapter);
	
	psta = search_assoc_sta(pparm->addr, padapter);
	if (psta)
	{	
//		prsp->aid = psta->info2->aid;
		printk("set assocsta successfully\n");
		cmd_clr_isr(&padapter->cmdpriv);
		return 	H2C_SUCCESS_RSP;
	}
	else
	{
		cmd_clr_isr(&padapter->cmdpriv);
		return 	H2C_SUCCESS_RSP;
	}
	
}


unsigned char setauth_hdl(_adapter *padapter,unsigned char *pbuf)
{
	struct setauth_parm	*pparm = (struct setauth_parm *)pbuf;
	struct mib_info *psys_mib = &padapter->_sys_mib;

	//marc@0925
	//setting for the security type
	//0: legacy open, 1: legacy shared 2: 802.1x
	psys_mib->mac_ctrl.security = pparm->mode;
	//0: PSK, 1: TLS
	//seems useless
	psys_mib->mac_ctrl._1x = pparm->_1x;
	
	//setting for the authenication mode
	switch (pparm->mode)
	{
		case 0: 
		case 2:
		//		padapter->securitypriv.dot11AuthAlgrthm=0;
			padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm = 0;
			break;
		case 1:
		//	padapter->securitypriv.dot11AuthAlgrthm=1;
			padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm = 1;
			break;
		default:
			printk("H2C: invalid auth mode\n");
	}
	 cmd_clr_isr(&padapter->cmdpriv);
	
	return 	H2C_SUCCESS;
}

#ifdef DISABLE_EVENT_THREAD

static u8 event_process(struct event_frame_priv *peventframe, _adapter *padapter)
{
	struct evt_priv *pevt_priv = &padapter->evtpriv;
	unsigned int    val,r_sz, ec;
	void (*event_callback)(_adapter	*dev, u8 *pbuf);
	u8 	*peventbuf = NULL;

_func_enter_;
	
	if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE)){
		DEBUG_ERR(("event_thread:bDriverStopped or bSurpriseRemoved\n"));
		return _FAIL;
	}


	val = peventframe->c2h_res;


	r_sz = (val >> 8) & 0xffff;
	ec = (val >> 24);
		
	// checking event sequence...		
	if ((val & 0xff) == pevt_priv->event_seq)
	{
		DEBUG_INFO(("Evetn Seq Error! %d vs %d\n", (val & 0xff), pevt_priv->event_seq));
		goto _abort_event_;
	}

	// checking if event code is valid
	if (ec >= MAX_C2HEVT)
	{
		DEBUG_ERR(("\nEvent Code(%d) mismatch!\n", (val >> 24)));
		goto _abort_event_;
	}

	// checking if event size match the event parm size
	if ((wlanevents[ec].parmsize != 0) && 
			(wlanevents[ec].parmsize != r_sz))
	{
			
		DEBUG_ERR(("\nEvent(%d) Parm Size mismatch (%d vs %d)!\n", 
		ec, wlanevents[ec].parmsize, r_sz));
		goto _abort_event_;	
			
	}

	pevt_priv->event_seq = (val & 0xff);
		

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	peventbuf = peventframe->c2h_buf;
#else
	read_mem(padapter, (c2haddr + 4), r_sz, peventbuf);	
#endif	

	if (peventbuf)
	{
		event_callback = wlanevents[ec].event_callback;
		event_callback(padapter, peventbuf);
 	}
	else 
		DEBUG_ERR(("event_process: !!ERROR!! peventbuf is NULL\n"));

_abort_event_:
	
	//clear C2HSET...
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

	if(ec ==1)
		DEBUG_ERR(("Call c2hclrdsr at event_thread1\n"));

	/*
		queue_event_semaphore has been down before the funtion was called. 
		if using DISABLE_EVENT_THREAD, we cannot call down sema twice.
	*/
	//_down_sema(&(padapter->_sys_mib.queue_event_sema));	
	c2hclrdsr(padapter);	
	//_up_sema(&(padapter->_sys_mib.queue_event_sema));	
#else
	write32(padapter,C2HR, (c2haddr));
		//sync_ioreq_flush(dev);
#endif	

	/* Perry:
		We have completed this event, unregister power level here.
	*/
	unregister_evt_alive(padapter);

	if(ec == 1)
	{
		 printk("\n^^^start_hw_event_posting :cmd_clr_isr^^^\n");
		 cmd_clr_isr(&padapter->cmdpriv);
	}

	free_evt_frame(peventframe, pevt_priv);

_func_exit_;	
	return _SUCCESS;
}
#endif /*DISABLE_EVENT_THREAD*/

int event_queuing (_adapter * padapter,struct event_node *evtnode)
{
	volatile int	*head = &(padapter->_sys_mib.c2hevent.head);
	volatile int	*tail= &(padapter->_sys_mib.c2hevent.tail);
	int	res = SUCCESS, hwen = 0;		
	struct event_node *node = NULL;

	struct event_frame_priv *peventframe = alloc_evt_frame(&padapter->evtpriv, &padapter->evtpriv.free_event_queue);
	if (peventframe == NULL)
	{
		DEBUG_ERR(("event_queuing: alloc eventframe fail\n"));
		return _FAIL;
	}

	if (CIRC_SPACE(*head, *tail, C2HEVENT_SZ))
	{
		_memcpy(&(padapter->_sys_mib.c2hevent.nodes[*head]), evtnode, sizeof (struct event_node));	
		node = &(padapter->_sys_mib.c2hevent.nodes[*head]);

		if (*head == *tail)
			hwen = 1;
		
		*head = (*head + 1) & (C2HEVENT_SZ - 1);
	}
	else
	{
		DEBUG_ERR(("event_queuing: CIRC_SPACE is all occupied\n"));
		res = FAIL;
	}
	

#ifdef CONFIG_POWERSAVING
	local_irq_save(flags);
	ps_32K_isr_patch();
	local_irq_restore(flags);
#endif

	peventframe->c2h_res = (evtnode->evt_code << 24) | ((evtnode->evt_sz) << 8) |padapter->_sys_mib.c2hevent.seq++;
	peventframe->c2h_buf=evtnode->node;

#ifdef DISABLE_EVENT_THREAD
	event_process(peventframe, padapter);
#else
	enqueue_evt_frame(peventframe, &padapter->evtpriv);
	if(evtnode->evt_code == 1)
		DEBUG_INFO(("######## enqueuing a surveydone_event\n"));
	
	evt_notify_isr(&padapter->evtpriv);
#endif /*DISABLE_EVENT_THREAD*/

	DEBUG_INFO(("\n dequeue_event:end!!!\n"));
	return res;	
}


unsigned char set_stakey_hdl(_adapter *padapter,unsigned char *pbuf){
	unsigned short ctrl;
	struct set_stakey_parm	*pparm = (struct set_stakey_parm *)pbuf;

	struct sta_data			*psta;
	u8 i = 0 ;

	DEBUG_INFO(("rcv set_stakey sta addr:\n"));

	for(i = 0 ; i < 6 ; i ++)
		DEBUG_INFO(("%x ", pparm->addr[i]));

	ctrl = BIT(15) | (BIT(6) | ((pparm->algorithm) << 2));

#ifdef CONFIG_SCSI_READ_FIFO
	unicast_enc = pparm->algorithm;
#endif

	//mprintk("%1.x\n", pparm->algorithm);

	if ((pparm->algorithm == 0x1) || (pparm->algorithm == 0x5))
	{
		printk("WEP");
		ctrl |= BIT(5);
	}

#if 1	
	del_assoc_sta(padapter,pparm->addr);
#endif
	
	setup_assoc_sta(pparm->addr, -1, 0, ctrl, pparm->key,padapter);
	psta = search_assoc_sta(pparm->addr, padapter);
	if(psta)
		printk("successfully add sta in set_stakey_hdl\n");
#if 0	
	psta = search_assoc_sta(pparm->addr);
	if (psta)
	{
		_memcpy(prsp->addr, pparm->addr, ETH_ALEN);
		prsp->keyid = psta->info2->aid;
		
		printk("return set_stakey: %x\n", prsp->keyid);
		return 	H2C_SUCCESS_RSP;	
	}
	else
	{
		printk("set_stakey failed\n");
		return H2C_REJECTED;
	}
#endif
cmd_clr_isr(&padapter->cmdpriv);
		return 	H2C_SUCCESS_RSP;	
}
unsigned char del_assocsta_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char setstapwrstate_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char setbasicrate_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}	
unsigned char getbasicrate_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char setdatarate_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char getdatarate_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char setphyinfo_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char getphyinfo_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char setphy_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}

unsigned char getphy_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char readBB_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char writeBB_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char readRF_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char writeRF_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char readRssi_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char readGain_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}
unsigned char setrfintfs_hdl(_adapter *padapter,unsigned char *pbuf){
cmd_clr_isr(&padapter->cmdpriv);
	return H2C_SUCCESS;
}

#if 0

#include <config.h>
#include <arch/byteorder.h>
#include <rtl8711_spec.h>
#include <types.h>
#include <console.h>
#include <system32.h>
#include <sched.h>
#include <wlan.h>
#include <rtl8711_cmd.h>
#include <wlan_task.h>
#include <wlan_util.h>
#include <wlan_sme.h>
#include <wlan_mlme.h>
#include <system32.h>
#include <circ_buf.h>
#include <string.h>
#include <wlan_mlme.h>
#include <RTL8711_RFCfg.h>

#ifdef CONFIG_POWERSAVING
#include <power_saving.h>
#endif
/*------------------------------------------------------------------------------
	
					Sequence Number Domain

-------------------------------------------------------------------------------*/



volatile unsigned char	chip_version = RTL8711_FPGA;
volatile unsigned char	hci_sel = RTL8711_SDIO;
volatile unsigned char	lbk_mode = RTL8711_AIR_TRX;
volatile unsigned char  rate_adaptive = 1;	/* for rateadaptive registry*/
volatile unsigned char	dig = 1;		/* 0:off 1:on */
volatile unsigned char	vcsMode = 0;		/* 0:off 1:on 2:auto */
volatile unsigned char	vcsType = 2;		/* 1:RTS/CTS 2:CTS to self */
volatile unsigned short	RTSThreadshold = 0x2a09;

volatile unsigned int	_h2cbuf_[H2CSZ] __in_4word__;
volatile unsigned int	_h2crsp_[H2CSZ] __in_4word__;

#ifdef CONFIG_SCSI_READ_FIFO
volatile unsigned int	c2hbuf[C2HSZ + 1] __in_4word__;
#else
volatile unsigned int	c2hbuf[C2HSZ] __in_4word__;
#endif

volatile unsigned int	*h2cbuf;
volatile unsigned int	*h2crsp;
//volatile unsigned int	*c2hbuf = ((unsigned int)_c2hbuf_ | 0xa0000000); //no need for write-through cache...

volatile unsigned int	h2crsp_addr;

#ifdef CONFIG_SCSI_READ_FIFO
volatile unsigned char unicast_enc = 0;
volatile unsigned char brcast_enc = 0;
#endif

u8 (*h2cfun)(u8 *pbuf);

//volatile u8  h2cseq;

struct cmdfile {
	volatile u8	seq;
	volatile u8 result;	
	volatile u16	cmdcode;
};

union cmdf {
	struct	cmdfile	cmd;
	unsigned int	value;
};

#define H2CPARM_BUF(pcmd)		(u8 *)((u32)(pcmd) + sizeof (union cmdf))
#define NEXT_CMDPTR(pcmd, parm_size)	(union cmdf *)((u32)(pcmd) + sizeof(struct cmdfile) + parm_size)


volatile union cmdf *pcmdfile;

extern struct  sta_data        stadata[MAX_STASZ];

//sunghau@0319: eliminate warning
extern int update_rate(unsigned char *, unsigned char *, unsigned int);


//sunghau@0320: eliminate warning
#ifdef CONFIG_H2CLBK
static int enqueue_ccode(u16 ccode)
{
	volatile int	*head = &sys_mib.h2cqueue.head;
	volatile int	*tail = &sys_mib.h2cqueue.tail;
	if (CIRC_SPACE(*head, *tail, H2CCMDQUEUE_SZ)) {
		sys_mib.h2cqueue.ccode[*head] = ccode;
		*head = (*head + 1) & (H2CCMDQUEUE_SZ - 1);
		return SUCCESS;
	};
		return FAIL;
}
#endif



/*
struct event_node	{
	unsigned char *node;
	unsigned char evt_code;
	unsigned short evt_sz;
	int	*caller_ff_tail;
	int	caller_ff_sz;
};
*/




static void inline flush_fw_fifo(void)
{
	unsigned int cnt = 0;
	
	local_irq_enable();

#ifdef MGT_CRTL_QUEUE
	while((rtl_inl(FREETX) & 0x00f00000) != 0x00900000 )	// MGT FF
#else
	while((rtl_inl(FREETX) & 0x00000F00) != 0x00000E00 )	// BE FF
#endif
	{
		cnt++;
		if (cnt == 100)
		{
			printk("break from survey loop\n");
			break;
		}
	}

	local_irq_disable();
}


// H2C is a dsr function
void h2csetdsr(_adapter* padapter, union cmdf* pcmdfile)
{
	u8	res;
	struct mlme_priv* pmlmepriv = &(padapter->mlmepriv);
	
//	pcmdfile = (union cmdf *)( (u32)h2cbuf | 0xa0000000 );
//	h2crsp_addr = (u32)( (u32)h2crsp | 0xa0000000) ;
	
	if ((pcmdfile->cmd.seq) == pmlmepriv->h2cseq)
	{
		printk("repeated cmd code = %x, cmd seq=%x, h2dseq=%x\n", 
			pcmdfile->cmd.cmdcode, pcmdfile->cmd.seq, pmlmepriv->h2cseq);

		pcmdfile->cmd.result = H2C_DUPLICATED;		
		goto _EndOfH2C_;
	}

	if ((le16_to_cpu(pcmdfile->cmd.cmdcode)) >=MAX_H2CCMD)
	{
		printk("oversized cmd code: %d\n", le16_to_cpu(pcmdfile->cmd.cmdcode));
		pcmdfile->cmd.result = H2C_DROPPED;		
		goto _EndOfH2C_;	
	}
				
#if 1		
	printk("cmd %d, seq:%d\n", 
	le16_to_cpu(pcmdfile->cmd.cmdcode), pcmdfile->cmd.seq);
#endif
		 
	pmlmepriv->h2cseq = pcmdfile->cmd.seq;
		
	h2cfun = wlancmds[le16_to_cpu(pcmdfile->cmd.cmdcode)].h2cfuns;
	res = h2cfun(H2CPARM_BUF(pcmdfile));

	printk("res = %d\n", res);
	pcmdfile->cmd.result = res;
		
_EndOfH2C_:
	//printk("CLR\n");
	
#ifdef CONFIG_RTL8711 	
	/* Perry: SiteSurvey command should be clear when complete */
	if(le16_to_cpu(pcmdfile->cmd.cmdcode) != _SiteSurvey_CMD_) {
		//*(u32 *)h2crsp_addr = 0xffffffff;
		rtl_outl(H2CR ,(u32)(h2cbuf));
		rtl_inl(H2CR);
	}
#endif	
}

#ifdef CONFIG_POWERSAVING
unsigned char setatim_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct setatim_parm *pparm = (struct setpwrmode_parm *)pbuf;

	if(pparm->op > 2) {
		return H2C_PARAMETERS_ERROR;
	}

	if(pparm->op) {
		/* DELETE */
		CLR_BITS(ps_atim_sta[0], le32_to_cpu(pparm->txid[0]));
		CLR_BITS(ps_atim_sta[1], le32_to_cpu(pparm->txid[1]));
	} else {
		/* ADD */
		atim_report = 1;	// Perry: Driver asks to add atim sta, after complete it request, fw must report.
		SET_BITS(ps_atim_sta[0], le32_to_cpu(pparm->txid[0]));
		SET_BITS(ps_atim_sta[1], le32_to_cpu(pparm->txid[1]));
	}

	return H2C_SUCCESS;
}

unsigned char setpwrmode_hdl(_adapter *padapter, unsigned char *pbuf)
{
	unsigned char ret = H2C_SUCCESS;
	struct setpwrmode_parm *pparm = (struct setpwrmode_parm *)pbuf;

	dprintk("H2C Cmd --> mode:%d app_itv:%d bcn_to:%d bcn_pass_cnt:%d\n", pparm->mode, pparm->app_itv, pparm->bcn_to, pparm->bcn_pass_cnt);

	if(!pparm->bcn_pass_cnt)
		ps_bcn_pass_cnt = 1;
	else
		ps_bcn_pass_cnt = pparm->bcn_pass_cnt;

	if(pparm->bcn_to) {
		// Write to TCDATA_BCN_TO, and it will be refresh next time.
		if(ps_bcn_to != pparm->bcn_to) {
			ps_bcn_to = pparm->bcn_to;
			rtl_outl(TCDATA_BCN_TO, ps_bcn_to << 5);
		}
	}

	if(pparm->app_itv) {
		if((pparm->mode == PS_MODE_IBSS) && (ps_app_itv != pparm->app_itv)) {
			// Write to TCDATA_APP, and it will be refresh next time.
			ps_app_itv = pparm->app_itv;
			rtl_outl(TCDATA_APP, ps_app_itv << 5);
		}
	}

	if(pparm->mode <= PS_MODE_WWLAN) {
		if(ps_mode != pparm->mode) {
			ps_set_mode(pparm->mode);
		}
	} else
		ret = H2C_PARAMETERS_ERROR;

	return ret;
}
#endif


#ifdef CONFIG_H2CLBK

unsigned char seth2clbk_hdl(_adapter *padapter, unsigned char *pbuf)
{
	u8 res;
	struct seth2clbk_parm *pparm = (struct seth2clbk_parm *)pbuf;
	struct geth2clbk_rsp   *prsp = NULL;

	//printk("Here is the loopback cmd test (%d)\n", (pparm->mac[0]));

	if ((pparm->mac[0]) == 0x00)
	{
		
	
		
		res = H2C_SUCCESS; 
		//this is the cmd mode...check the following conditons...
		/*
		The following condition must be ture under CMD mode		
		mac[1] == mac[4], mac[2] == mac[3], mac[0]=mac[5]= 0;		
		s0 == 0x1234, s1 == 0xabcd, w0 == 0x78563412, w1 == 0x5aa5def7;		
		s2 == (b1 << 8 | b0);
		*/

		if (pparm->mac[5] != 0)
			res = H2C_PARAMETERS_ERROR;

		if ((pparm->mac[1] != pparm->mac[4]) || 
			(pparm->mac[2] != pparm->mac[3]))
			res = H2C_PARAMETERS_ERROR;

		if (le16_to_cpu(pparm->s0) != 0x1234)
			res = H2C_PARAMETERS_ERROR;

		if (le16_to_cpu(pparm->s1) != 0xabcd)
			res = H2C_PARAMETERS_ERROR;

		if (le32_to_cpu(pparm->w0) != 0x78563412)
			res = H2C_PARAMETERS_ERROR;

		if (le32_to_cpu(pparm->w1) !=  0x5aa5def7)
			res = H2C_PARAMETERS_ERROR;

		if (le16_to_cpu(pparm->s2) !=  ((pparm->b1) << 8 | (pparm->b0)))
			res = H2C_PARAMETERS_ERROR;

		printk("CMD[%d]\n", pparm->mac[4]);
		
		return res;
	}
	else if ((pparm->mac[0]) == 0x01)
	{
	/*
			mac[0] == 1	==> CMD_RSP mode, 
			return H2C_SUCCESS_RSP		
			The rsp layout shall be:	
			rsp: 			parm:		
			mac[0]  =   mac[5];		
			mac[1]  =   mac[4];		
			mac[2]  =   mac[3];		
			mac[3]  =   mac[2];		
			mac[4]  =   mac[1];		
			mac[5]  =   mac[0];		
			s0		=   s1;		
			s1		=   swap16(s0);		
			w0		=  	swap32(w1);		
			b0		= 	b1		
			s2		= 	s0 + s1		
			b1		= 	b0		
			w1		=	w0
	*/

		prsp = (struct geth2clbk_rsp *)h2crsp_addr;

		prsp->mac[0]  =   pparm->mac[5];		
		prsp->mac[1]  =   pparm->mac[4];		
		prsp->mac[2]  =   pparm->mac[3];		
		prsp->mac[3]  =   pparm->mac[2];		
		prsp->mac[4]  =   pparm->mac[1];		
		prsp->mac[5]  =   pparm->mac[0];		
		prsp->s0	 =   cpu_to_le16(le16_to_cpu(pparm->s1));
		prsp->s1	 =   cpu_to_le16(swab16(le16_to_cpu(pparm->s0)));	
		prsp->w0 =   cpu_to_le32(swab32(le32_to_cpu(pparm->w1)));
		prsp->b0		= 	pparm->b1;
		prsp->s2		= 	cpu_to_le16(le16_to_cpu(pparm->s1) + le16_to_cpu(pparm->s0));		
		prsp->b1		= 	pparm->b0;	
		prsp->w1	= 	 cpu_to_le32((le32_to_cpu(pparm->w0)));

		//h2crsp_addr += sizeof(struct geth2clbk_rsp);

		printk("CMD_RSP[%d]\n", pparm->mac[4]);

		return H2C_SUCCESS_RSP;
	}
	else if ((pparm->mac[0]) == 0x02)
	{

		printk("cmd_evt mac3=%d\n", pparm->mac[3]);
		
		_memcpy(&sys_mib.lbkparm, pparm, sizeof (*pparm));

		if ((enqueue_ccode(_SetH2cLbk_CMD_)) == SUCCESS)
			return H2C_SUCCESS;
		else
			return H2C_CMD_OVERFLOW;
	}
	else
	{

		return H2C_PARAMETERS_ERROR;

	}
	
}
unsigned char geth2clbk_hdl(_adapter *padapter, unsigned char *pbuf)
{
	
	
	
}
#endif



extern volatile unsigned char rfintfs;

unsigned char setrfintfs_hdl(_adapter *padapter, unsigned char *pbuf)
{

	struct setrfintfs_parm	*pparm = (struct setrfintfs_parm *)pbuf;
	
	rfintfs = pparm->rfintfs;

//	printk("setting rfintfs:%x\n", rfintfs);
	if (rfintfs == SWSI) {
//	if (rfintfs == 0) {
		printk("Config RF Interface to SW-3Wire \n");
		rtl_outb(RFINTFS, rtl_inb(RFINTFS) |  0x0F );
		rtl_outb(0xbd200278, 0xd2);
	}
	else {
		printk("Config RF Interface to HW-3Wire \n");

		rtl_outb(RFINTFS, rtl_inb(RFINTFS) & 0xF0 );

		if (rfintfs == HWSI) {
//		if (rfintfs == 1) {
			printk("Config RF Interface to SI \n");
			rtl_outb(0xbd200278, 0xd2);

		} 
		else {
			printk("Config RF Interface to PI \n");
			rtl_outb(0xbd200278, 0xd0);
		}
	}


	return H2C_SUCCESS;
};


unsigned char getrfintfs_hdl(_adapter *padapter, unsigned char *pbuf)
{


	return H2C_SUCCESS;

}



/*
struct set_stakey_parm	{		
unsigned char addr[ETH_ALEN];	
unsigned char	algorithm;	
unsigned char	keyid;	
unsigned char	key[16];
};

This will be used to set up the unicast key


*/
unsigned char set_stakey_hdl(_adapter *padapter, unsigned char *pbuf)
{
	unsigned short ctrl;
	struct set_stakey_parm	*pparm = (struct set_stakey_parm *)pbuf;
	struct set_stakey_rsp	*prsp = (struct set_stakey_rsp *)h2crsp_addr;
	struct sta_data			*psta;
	//unsigned int	i;
	
	printk("rcv set_stakey\n");

	ctrl = BIT(15) | (BIT(6) | ((pparm->algorithm) << 2));

#ifdef CONFIG_SCSI_READ_FIFO
	unicast_enc = pparm->algorithm;
#endif

	//mprintk("%1.x\n", pparm->algorithm);

	if ((pparm->algorithm == 0x1) || (pparm->algorithm == 0x5))
	{
		printk("WEP");
		ctrl |= BIT(5);
	}

#if 1	
	del_assoc_sta(padapter, pparm->addr);
#endif
	
	setup_assoc_sta(pparm->addr, -1, 0, ctrl, pparm->key, padapter);
	
	psta = search_assoc_sta(pparm->addr);
	if (psta)
	{
		_memcpy(prsp->addr, pparm->addr, ETH_ALEN);
		prsp->keyid = psta->info2->aid;
		
		printk("return set_stakey: %x\n", prsp->keyid);
		return 	H2C_SUCCESS_RSP;	
	}
	else
	{
		printk("set_stakey failed\n");
		return H2C_REJECTED;
	}
	
}

unsigned char del_assocsta_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct del_assocsta_parm  *pparm = (struct del_assocsta_parm  *)pbuf;
	struct sta_data *psta = search_assoc_sta(pparm->addr);

#if 0
	if (psta == NULL)
		return H2C_PARAMETERS_ERROR;
#endif
	
	if (psta)
		free_stadata(psta);
	
	return 	H2C_SUCCESS;
}



unsigned char setstapwrstate_hdl(_adapter *padapter, unsigned char *pbuf)
{
	
	return 	H2C_SUCCESS;
}

unsigned char setbasicrate_hdl(_adapter *padapter, unsigned char *pbuf)
{
	unsigned long flags;
	unsigned char basic_rate[NumRates];
	struct setbasicrate_parm *parm = (struct setbasicrate_parm *)pbuf;
	
	local_irq_save(flags);
	
	memset(basic_rate, 0xff, NumRates);
	
	_memcpy(basic_rate, parm->basicrates, MAX_RATES_LENGTH);
	
	sys_mib.mac_ctrl.basicrate_inx = 
	update_rate(sys_mib.mac_ctrl.basicrate, basic_rate, 1);

	if ((chip_version == RTL8711_1stCUT) && (sys_mib.mac_ctrl.basicrate_inx == 8))
		sys_mib.mac_ctrl.basicrate_inx = 9;

#ifdef LOWEST_BASIC_RATE	
	sys_mib.mac_ctrl.lowest_basicrate_inx = 
	update_rate(sys_mib.mac_ctrl.basicrate, basic_rate, 0);
#endif
	
	local_irq_restore(flags);
	
	return 	H2C_SUCCESS;
}

unsigned char getbasicrate_hdl(_adapter *padapter, unsigned char *pbuf)
{
	
	return 	H2C_SUCCESS;
}

unsigned char setdatarate_hdl(_adapter *padapter, unsigned char *pbuf)
{
	unsigned char datarates[NumRates];

	struct	setdatarate_parm *parm = (struct setdatarate_parm *)pbuf;

	if (parm->mac_id > MAX_STASZ)
	{
		return H2C_PARAMETERS_ERROR;
	}
	
	memset(datarates, 0xff, NumRates);
	
	_memcpy(datarates, parm->datarates, MAX_RATES_LENGTH);
	
	
	if ((parm->mac_id) == MAX_STASZ)
	{
		_memcpy(sys_mib.mac_ctrl.datarate, datarates, MAX_RATES_LENGTH);
	}
	else
	{
		sys_mib.stainfos[parm->mac_id].txrate_inx = 
		update_rate(sys_mib.stainfos[parm->mac_id].txrate, datarates, 1);
	}
	
	return 	H2C_SUCCESS;
}

unsigned char getdatarate_hdl(_adapter *padapter, unsigned char *pbuf)
{
	
	return 	H2C_SUCCESS;
}


/*


struct	setphyinfo_parm 
{	struct regulatory_class class_sets[NUM_REGULATORYS];	
	unsigned char	status;
};

*/

unsigned char setphyinfo_hdl(_adapter *padapter, unsigned char *pbuf)
{
	int i;
	struct	setphyinfo_parm 	*parm = (struct	setphyinfo_parm *)pbuf;
	struct 	regulatory_class	*regulatory;

	_memcpy((unsigned char *)(sys_mib.class_sets), pbuf, sizeof (struct setphyinfo_parm)); 

	sys_mib.class_cnt = 0;

	for(i = 0; i < NUM_REGULATORYS; i++) {
		regulatory = &(parm->class_sets[i]);
		if (regulatory->starting_freq == 0)
			break;
		sys_mib.class_cnt++;
	}

	return 	H2C_SUCCESS;
}

unsigned char getphyinfo_hdl(_adapter *padapter, unsigned char *pbuf)
{
	
	struct getphyinfo_rsp  *prsp = (struct getphyinfo_rsp *)h2crsp_addr;
	_memcpy((unsigned char *)prsp, pbuf, sizeof (struct getphyinfo_rsp)); 
	//h2crsp_addr += sizeof(struct getphyinfo_rsp);

	return H2C_SUCCESS_RSP;
}


/*

struct	setphy_parm {	
	unsigned char	rfchannel;	
	unsigned char	modem;
};

*/
unsigned char setphy_hdl(_adapter *padapter, unsigned char *pbuf)
{
	
	unsigned long flags;
	unsigned char	*basic_rate;
	unsigned char	*data_rate;
	unsigned char cur_phy;
	
	struct	setphy_parm	*parm = (struct	setphy_parm *)pbuf;

	local_irq_save(flags);
	
	sys_mib.cur_channel = parm->rfchannel;
	sys_mib.cur_modem = parm->modem;

	cur_phy = get_phy();
	
	//jackson 1026	
	
	if ( cur_phy == CCK_PHY) {
		basic_rate = cck_basicrate;
		data_rate = cck_datarate;
	}
#if 0	
	else if ( cur_phy == OFDM_PHY) {
		basic_rate = ofdm_basicrate;
		data_rate = ofdm_datarate;
	}
#endif	
	else {  
		basic_rate = mixed_basicrate;
		data_rate = mixed_datarate;
	}
		
	sys_mib.mac_ctrl.basicrate_inx = 
	update_rate(sys_mib.mac_ctrl.basicrate, basic_rate, 1);

	if ((chip_version == RTL8711_1stCUT) && (sys_mib.mac_ctrl.basicrate_inx == 8))
		sys_mib.mac_ctrl.basicrate_inx = 9;	

#ifdef LOWEST_BASIC_RATE	
	sys_mib.mac_ctrl.lowest_basicrate_inx = 
	update_rate(sys_mib.mac_ctrl.basicrate, basic_rate, 0);
#endif

	update_rate(sys_mib.mac_ctrl.datarate, data_rate, 1);

	sys_mib.stainfos[0].txrate_inx = 
	update_rate(sys_mib.stainfos[0].txrate, basic_rate, 0);

	sys_mib.stainfos[1].txrate_inx = 
	update_rate(sys_mib.stainfos[1].txrate, basic_rate, 0);
	
	sys_mib.stainfos[2].txrate_inx = 
	update_rate(sys_mib.stainfos[2].txrate, basic_rate, 0);

	sys_mib.stainfos[3].txrate_inx = 
	update_rate(sys_mib.stainfos[3].txrate, basic_rate, 0);

	sys_mib.stainfos[4].txrate_inx = 
	update_rate(sys_mib.stainfos[4].txrate, basic_rate, 0);

	sys_mib.stainfos[5].txrate_inx = 
	update_rate(sys_mib.stainfos[5].txrate, data_rate, 1);
	
	SwChnl(sys_mib.cur_channel);
	
	local_irq_restore(flags);
	
	return 	H2C_SUCCESS;
}

unsigned char getphy_hdl(_adapter *padapter, unsigned char *pbuf)
{

	struct getphy_rsp  *prsp = (struct getphy_rsp *)h2crsp_addr;

	prsp->rfchannel = sys_mib.cur_channel;
	prsp->modem = sys_mib.cur_modem;



	//h2crsp_addr += sizeof(struct getphy_rsp);
	return 	H2C_SUCCESS_RSP;
}


unsigned char readBB_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct readBB_parm *parm = (struct readBB_parm *)pbuf;
	struct readBB_rsp  *prsp = (struct readBB_rsp *)(h2crsp_addr);
	
	prsp->value = bb_read(parm->offset);
	
	printk("read BB: %x %x", parm->offset, prsp->value);
	
	return 	H2C_SUCCESS_RSP;
}

unsigned char writeBB_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct writeBB_parm *parm = (struct writeBB_parm *)pbuf;

	printk("write BB: %x %x", parm->offset, parm->value);
	bb_write(parm->offset, parm->value);
	
	return 	H2C_SUCCESS;
}

unsigned char readRF_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct readRF_parm *parm = (struct readRF_parm *)pbuf;
	struct readRF_rsp  *prsp = (struct readRF_rsp *)h2crsp_addr;
	
	prsp->value = cpu_to_le32(RF_ReadReg(parm->offset));
	
	printk("read RF: %x %x", parm->offset, prsp->value);
	
	return 	H2C_SUCCESS_RSP;
}

unsigned char writeRF_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct writeRF_parm *parm = (struct writeRF_parm *)pbuf;

	printk("write RF: %x %x", parm->offset, le32_to_cpu(parm->value));
	RF_WriteReg(parm->offset, 16, le32_to_cpu(parm->value));
	
	return 	H2C_SUCCESS;
}

void get_phy_table(unsigned char idx, unsigned char* pbuf)
{
	int	offset;
	
	bb_write(0x3e, idx << 4);

	for(offset = 0; offset < 64; offset++)
	{
		bb_write(0x3f, offset);
		pbuf[offset] = bb_read(0x60);
	}
	
}

unsigned char readRssi_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct readRSSI_parm *parm = (struct readRSSI_parm *)pbuf;
	struct readRSSI_rsp  *prsp = (struct readRSSI_rsp *)h2crsp_addr;
	
	get_phy_table((parm->idx + 3), prsp->value);
	
	return 	H2C_SUCCESS_RSP;
}

unsigned char readGain_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct readGAIN_parm *parm = (struct readGAIN_parm *)pbuf;
	struct readGAIN_rsp  *prsp = (struct readGAIN_rsp *)h2crsp_addr;
	
	get_phy_table((parm->idx + 1), prsp->value);
	
	return 	H2C_SUCCESS_RSP;	
}

//sunghau@0329
unsigned char setratable_hdl(_adapter *padapter, unsigned char *pbuf){
	int i;
	struct setratable_parm *parm = (struct setratable_parm *)pbuf;
	
	for(i=0;i<NumRates;i++){
		sys_mib.rf_ctrl.ss_ForceUp[i] = parm->ss_ForceUp[i];
		sys_mib.rf_ctrl.ss_ULevel[i] = parm->ss_ULevel[i];
		sys_mib.rf_ctrl.ss_DLevel[i] = parm->ss_DLevel[i];
		sys_mib.rf_ctrl.count_judge[i] = parm->count_judge[i];
	}

	
	return H2C_SUCCESS;
}

unsigned char getratable_hdl(_adapter *padapter, unsigned char *pbuf){
	int i;
	struct getratable_rsp *prsp = (struct getratable_rsp *)h2crsp_addr;

	for(i=0;i<NumRates;i++){
		prsp->ss_ForceUp[i] = sys_mib.rf_ctrl.ss_ForceUp[i];
		prsp->ss_ULevel[i] = sys_mib.rf_ctrl.ss_ULevel[i];
		prsp->ss_DLevel[i] = sys_mib.rf_ctrl.ss_DLevel[i];
		prsp->count_judge[i] = sys_mib.rf_ctrl.count_judge[i];
	}

	return H2C_SUCCESS_RSP;
}

unsigned char setusbsuspend_hdl(_adapter *padapter, unsigned char *pbuf){
	int v32;
	
	rtl_outb(0xbd200058, rtl_inb(0xbd200058) | BIT(5) );
	for(v32=0; v32 <10000; v32++) ;
	rtl_outb(0xbd621024, 0); 
	rtl_outb(0xbd621028, 0);
	rtl_outb(0xbd62110c, 0x0E);
	v32 = rtl_inl(0xbd621100);
	rtl_outl(0xbd620010, 0xC49);
	v32 &= 0xFFFFFFDF;
	//rtl_out(0xbd621100, v32 & CLEAR_BIT(4) & CLEAR_BIT(5) );
	rtl_outl(0xbd621100, v32);
	v32 &= 0xFFFFFFCF;
	//rtl_out(0xbd621100, v32 & CLEAR_BIT(4) & CLEAR_BIT(5) );
	rtl_outl(0xbd621100, v32);
	rtl_outl(0xbd620004, 0x11);

	rtl_outl(0xbd620004, 0x10);
	rtl_outl(0xbd620010, 0);

	return H2C_SUCCESS;
}

#if 0
 unsigned char AddAssocSta(unsigned char *pbuf)
{
	
	struct addassocsta_parm *pparm;
	unsigned int	addr;
	unsigned short	*seq;
	unsigned long	flags;
	
	pparm = (struct addassocsta_parm *)pbuf;

#ifdef CONFIG_H2C_CHECK
	
	if ((pparm->staid) >= MAXCAMINX)
		return H2C_PARAMETERS_ERROR;
		
#endif		
	
	local_irq_save(flags);
	
	addr = (pparm->staid) * CAMENTRYSZ;
	
	rtl_outl(CAM_WDATA, (*(u32 *)((u32)(pparm->stainfo) + 0)) | CAM_VALID);
	rtl_outl(CAM_RW, (CAM_WE) | addr | CAM_POLLING);
	
	rtl_outl(CAM_WDATA, *(u32 *)((u32)(pparm->stainfo) + 4));
	rtl_outl(CAM_RW, (CAM_WE) | (addr+4) | CAM_POLLING);
	
	rtl_outl(CAM_WDATA, *(u32 *)((u32)(pparm->stainfo) + 8));
	rtl_outl(CAM_RW, (CAM_WE) | (addr+8) | CAM_POLLING);
	
	rtl_outl(CAM_WDATA, *(u32 *)((u32)(pparm->stainfo) + 12));
	rtl_outl(CAM_RW, (CAM_WE) | (addr+12) | CAM_POLLING);
	
	rtl_outl(CAM_WDATA, *(u32 *)((u32)(pparm->stainfo) + 16));
	rtl_outl(CAM_RW, (CAM_WE) | (addr+16) | CAM_POLLING);
	
	rtl_outl(CAM_WDATA, *(u32 *)((u32)(pparm->stainfo) + 20));
	rtl_outl(CAM_RW, (CAM_WE) | (addr+20) | CAM_POLLING);
	
	
	seq = &(sys_mib.stainfo2s[pparm->staid].tid0_seq);
	
	for (addr = 0; addr <= 16 ; addr ++)
		*(seq + addr) = 0;
		
	sys_mib.stainfos[pparm->staid].attribute = 0;
	
	sys_mib.stainfo2s[pparm->staid].addr[0] = *(u8 *)(((u32)pparm + 4) + 2);
	sys_mib.stainfo2s[pparm->staid].addr[1] = *(u8 *)(((u32)pparm + 4) + 3);
	sys_mib.stainfo2s[pparm->staid].addr[2] = *(u8 *)(((u32)pparm + 4) + 4);
	sys_mib.stainfo2s[pparm->staid].addr[3] = *(u8 *)(((u32)pparm + 4) + 5);
	sys_mib.stainfo2s[pparm->staid].addr[4] = *(u8 *)(((u32)pparm + 4) + 6);
	sys_mib.stainfo2s[pparm->staid].addr[5] = *(u8 *)(((u32)pparm + 4) + 7);
	
	if ((sys_mib.stainfo2s[pparm->staid].addr[0]) & 0x01)
		sys_mib.stainfos[pparm->staid].attribute |= 0x01;
		
		
	local_irq_restore(flags);	
	
	return H2C_SUCCESS;
		
}

/*
struct	delassocsta_parm {
	unsigned char	staid;
	unsigned char	rsvd[3];
};
*/

unsigned char DelAssocSta(unsigned char *pbuf)
{

	struct delassocsta_parm *pparm;
	unsigned int	addr;
	pparm = (struct delassocsta_parm *)pbuf;

#ifdef CONFIG_H2C_CHECK
	
	if ((pparm->staid) >= MAXCAMINX)
		return H2C_PARAMETERS_ERROR;

#endif
	
	addr = (pparm->staid) * CAMENTRYSZ;

	rtl_outl(CAM_WDATA, 0);
	rtl_outl(CAM_RW, (CAM_WE) | addr | CAM_POLLING);	
	
	return H2C_SUCCESS;
	
}
	
/*
struct	setopmode_parm {
	unsigned char	mode;
	unsigned char	rsvd[3];
};
*/
	
unsigned char SetOpMode(unsigned char *pbuf)
{
	struct	setopmode_parm *pparm;
	pparm = (struct setopmode_parm *)pbuf;

	rtl_outb(MSR, pparm->mode);
	
	return 	H2C_SUCCESS;
	
}

/*
struct	setbssid_parm {
	unsigned char	bssid[6];
	unsigned char	rsvd[2];
};

*/




 unsigned char SetBSSID(unsigned char *pbuf)
{

	struct setbssid_parm *pparm;
	pparm = (struct setbssid_parm *)pbuf;

#ifdef CONFIG_H2C_CHECK
	
	if ((pparm->bssid[0]) & 0x01)
		return H2C_PARAMETERS_ERROR;	

#endif
	
	sys_mib.mac_ctrl.bssid[0] = pparm->bssid[0];
	sys_mib.mac_ctrl.bssid[1] = pparm->bssid[1];
	sys_mib.mac_ctrl.bssid[2] = pparm->bssid[2];
	sys_mib.mac_ctrl.bssid[3] = pparm->bssid[3];
	sys_mib.mac_ctrl.bssid[4] = pparm->bssid[4];
	sys_mib.mac_ctrl.bssid[5] = pparm->bssid[5];
	
	rtl_outl(BSSIDR, cpu_to_le32(*(u32 *)(((u32)pparm->bssid) + 0)));
	rtl_outl(BSSIDR + 4, cpu_to_le32(*(u32 *)(((u32)pparm->bssid) + 4)));

	
	
	return 	H2C_SUCCESS;		
}


 unsigned char SetStaPwrState(unsigned char *pbuf)
{
	
	
	
	return 	H2C_SUCCESS;
}



/*
struct setdatarate_parm {
	unsigned char	staid;
	unsigned char rsvd[3];
	unsigned char	datarate[NumRates];
};
*/



#define GETSTATXRATE(staid) \
	(sys_mib.stainfos[staid].txrate[sys_mib.stainfos[staid].txrate_inx])
	
unsigned char SetDataRate(unsigned char *pbuf)
{
	struct setdatarate_parm *pparm;
	unsigned int staid;
	pparm = (struct setdatarate_parm *)pbuf;

	staid = (pparm->staid);

#ifdef CONFIG_H2C_CHECK

	if ((pparm->staid) >= MAXCAMINX)
		return H2C_PARAMETERS_ERROR;
		
#endif


	if (staid == MAX_STASZ)  {
		
		
	}
	else {

#if 0
	*(u32 *)((u32)(sys_mib.stainfos[staid].txrate) + 0) = (*(u32 *)((u32)(pparm->datarate) + 0)) << 4;
	*(u32 *)((u32)(sys_mib.stainfos[staid].txrate) + 4) = (*(u32 *)((u32)(pparm->datarate) + 4)) << 4;
	*(u32 *)((u32)(sys_mib.stainfos[staid].txrate) + 8) = (*(u32 *)((u32)(pparm->datarate) + 8)) << 4;
	*(u32 *)((u32)(sys_mib.stainfos[staid].txrate) + 12) = (*(u32 *)((u32)(pparm->datarate) + 12)) << 4;
#endif

		_memcpy(	(sys_mib.stainfos[staid].txrate), (pparm->datarate), 16);
	}
	
	sys_mib.stainfos[staid].txrate_inx = 0;
	
	while( (GETSTATXRATE(staid)) == 0xff )
		sys_mib.stainfos[staid].txrate_inx++;
		
	
	
	return H2C_SUCCESS;
}

/*
struct getdatarate_parm {
	unsigned char	staid;
	unsigned char	rsvd[3];
};

struct getdatarate_rsp {
	unsigned char datarate[NumRates];
};
*/
 unsigned char GetDataRate(unsigned char *pbuf)
{
	
	struct	getdatarate_parm 	*pparm;
	struct	getdatarate_rsp		*prsp;
	unsigned char	staid;
	
	pparm = (struct	getdatarate_parm *)pbuf;
	prsp  = (struct getdatarate_rsp *) h2crsp_addr;
	staid = pparm->staid;
	
#ifdef CONFIG_H2C_CHECK

	if ((pparm->staid) >= MAXCAMINX)
		return H2C_PARAMETERS_ERROR;
		
#endif	
	
	*(u32 *)((u32)(prsp->datarate) + 0) = (*(u32 *)((u32)(sys_mib.stainfos[staid].txrate) + 0)) >> 4;
	*(u32 *)((u32)(prsp->datarate) + 4) = (*(u32 *)((u32)(sys_mib.stainfos[staid].txrate) + 4)) >> 4;
	*(u32 *)((u32)(prsp->datarate) + 8) = (*(u32 *)((u32)(sys_mib.stainfos[staid].txrate) + 8)) >> 4;
	*(u32 *)((u32)(prsp->datarate) + 12) = (*(u32 *)((u32)(sys_mib.stainfos[staid].txrate) + 12)) >> 4;
	
	//h2crsp_addr += sizeof (struct getdatarate_rsp);
	
	return H2C_SUCCESS_RSP;	
	
}

/*
struct	setbasicrate_parm {
	unsigned char	basicrates[NumRates];
};
*/

 unsigned char SetBasicRate(unsigned char *pbuf)
{
	
	unsigned short	val = 0;
	unsigned int		i;
	
	_memcpy(sys_mib.mac_ctrl.basicrate, pbuf, 16);
	
	for(i = 0; i< NumRates; i++)
	{
		if ((sys_mib.mac_ctrl.basicrate[i]) != 0xff)
			val |= BIT((11 - i));
	}
	
	sys_mib.mac_ctrl.basicrate_inx = 0;
	
	while(1)
	{
		if(sys_mib.mac_ctrl.basicrate[sys_mib.mac_ctrl.basicrate_inx] != 0xff)
			break;
		
		sys_mib.mac_ctrl.basicrate_inx++;
		
	}
	
	if ((chip_version == RTL8711_1stCUT) && (sys_mib.mac_ctrl.basicrate_inx == 8))
		sys_mib.mac_ctrl.basicrate_inx = 9;	
	
	rtl_outw(BRSR , val);
	
	return H2C_SUCCESS;
	
	
}

/*
struct getbasicrate_parm {
	
	
};

struct getbasicrate_rsp {
	unsigned char basicrates[NumRates];
};
*/

 unsigned char GetBasicRate(unsigned char *pbuf)
{
	struct	getbasicrate_rsp		*prsp;
	
	prsp  = (struct getbasicrate_rsp *) h2crsp_addr;

	_memcpy(prsp->basicrates, sys_mib.mac_ctrl.basicrate, 16);
	
	//h2crsp_addr += sizeof (struct getbasicrate_rsp);
	
	return H2C_SUCCESS_RSP;
}


/*
struct setdtim_parm {
	unsigned char	count;
	unsigned char	rsvd[3];	
};

struct	getdtim_parm {

};

struct getdtim_rsp {
	unsigned char	count;
	unsigned char	rsvd[3];	
};

*/

 unsigned char SetDTIM(unsigned char *pbuf)
{
	struct setdtim_parm *pparm;
	pparm = (struct setdtim_parm *)pbuf;

	sys_mib.mac_ctrl.dtim = pparm->count;
	return H2C_SUCCESS;
	
}



 unsigned char GetDTIM(unsigned char *pbuf)
{
	*(u8 *)h2crsp_addr = sys_mib.mac_ctrl.dtim;
	//h2crsp_addr += sizeof (struct getdtim_rsp);
	return H2C_SUCCESS_RSP;
	
}


/*
struct setbintval_parm {
	unsigned int	count;	// in units of TU
};

struct	getbintval_parm {
	
	
};

struct getbintval_rsp {
	unsigned int	count;	// in units of TU
};

*/

 unsigned char SetBintVal(unsigned char *pbuf)
{
	//sys_mib.mac_ctrl.beacon_itv = *(u16 *)pbuf;
	return H2C_SUCCESS;	
}



 unsigned char GetBintVal(unsigned char *pbuf)
{
	*(u8 *)h2crsp_addr = sys_mib.mac_ctrl.beacon_itv;
	//h2crsp_addr += sizeof (struct getbintval_rsp);
	return H2C_SUCCESS_RSP;
}




/*
struct	setphymode_parm {
	unsigned char	band;
	unsigned char	channel;
	unsigned char	status;
	unsigned char	rsvd;
};

struct	getphymode_parm {


};

struct	getphymode_rsp {
	unsigned char	band;
	unsigned char	channel;
	unsigned char	rsvd[2];
};
*/


 unsigned char SetPhyMode(unsigned char *pbuf)
{
	struct setphymode_parm *pparm;
	pparm = (struct setphymode_parm *)pbuf;
	
	if (pparm->status)
	{
		sys_mib.mac_ctrl.rfband = pparm->band;
		sys_mib.mac_ctrl.rfchannel = pparm->channel;
		
	#ifdef CONFIG_RF
		
		RTL8711RF_Cfg(pparm->channel);
		
	#endif
	
		
	}
	return H2C_SUCCESS;
}



 unsigned char GetPhyMode(unsigned char *pbuf)
{
	struct	getphymode_rsp	*prsp;
	
	prsp  = (struct getphymode_rsp *) h2crsp_addr;
	
	prsp->band = sys_mib.mac_ctrl.rfband;
	prsp->channel = sys_mib.mac_ctrl.rfchannel;
	
	//h2crsp_addr += sizeof (struct getphymode_rsp);
	return H2C_SUCCESS_RSP;
	
}


/*

	{sizeof(struct	setassocstacnt_parm), &SetAssocStaCnt},
#define _SetAssocStaCnt_CMD_	28

*/

// get beacon frame address
#define GETBCNFRADDR		((u32)(sys_mib.mac_ctrl.pbcn))
#define QBSSLOADIEOFFSET	((u32)(sys_mib.mac_ctrl.qbssloadie_offset))

 unsigned char SetAssocStaCnt(unsigned char *pbuf)
{
	struct	setassocstacnt_parm *pparm;
	pparm = (struct setassocstacnt_parm *)pbuf;
	
	
	*(unsigned short *)(GETBCNFRADDR + QBSSLOADIEOFFSET + 
											2) = pparm->assocsta_cnt;
	
	return H2C_SUCCESS;
	
}


/*

	tx_antset	=	0: SW Antenna 
	tx_antset 	= 	1: SW Fixed Antenna  
	tx_antset 	> 	1: HW Antenna

	rx_antset	= 	0: SW Fixed Antenna
	rx_antset	> 	0: HW Antenna

TX Antenna Flow:

	if (tx_antset == 0)
	{
		1. configure mac/bb to use tx_antenna as set in the TXCMD
		2. select antenna based on the previously received antenna of the packet,
		   as recorded in sta_info.tx_swant		   
	}
	else if (tx_antset == 1)
	{
		1. configure mac/bb to use tx_antenna as set in the TXCMD	
		2. select antenna from rf_ctrl.fixed_txant;
	}
	else
	{
		1. configure mac/bb NOT to use tx_antenna as set in the TXCMD		
		
	}


RX Antenna Flow:

	if (rx_antset == 0
	{
		1. configure mac/bb to use rx_antenna as set in the MAC Registers		
		2. select antenna from rf_ctrl.fixed_rxant;	
	}
	else
	{
		1. configure mac/bb NOT to use rx_antenna as configured in the MAC registers.
	}

*/

/*
struct	setantenna_parm {
	unsigned char	tx_antset;		
	unsigned char	rx_antset;		//antset=0: SW Antenna antset > 0: HW Antenna
	unsigned char	tx_antenna;		// tx_antenna = 0 or 1
	unsigned char	rx_antenna;		// tx_antenna = 0 or 1
};
*/


 unsigned char SetAntenna(unsigned char *pbuf)
{
	struct setantenna_parm *pparm;
	pparm = (struct setantenna_parm *)pbuf;
	
	
	sys_mib.rf_ctrl.tx_antset = pparm->tx_antset;
	sys_mib.rf_ctrl.rx_antset = pparm->rx_antset;
	sys_mib.rf_ctrl.fixed_txant = (pparm->tx_antenna) << 8;
	sys_mib.rf_ctrl.fixed_rxant = pparm->rx_antenna;
	
	
	return H2C_SUCCESS;
}


 unsigned char EnRateAdaptive(unsigned char *pbuf)
{
	sys_mib.mac_ctrl.en_rateadaptive = *(u32 *)pbuf;
	return H2C_SUCCESS;
}

 unsigned char	SetTxAgcTbl(unsigned char *pbuf)
{
	int i;
	struct settxagctbl_parm *pparm;
	pparm = (struct settxagctbl_parm *)pbuf;
	
	for( i = 0; i < NumRates; i++)
		sys_mib.rf_ctrl.txagc[i] = (pparm->txagc[i]) << 16;	

	return H2C_SUCCESS;	
	
}

 unsigned char	GetTxAgcTbl(unsigned char *pbuf)
{
	int	i;
	struct gettxagctbl_rsp *prsp;
	
	prsp  = (struct gettxagctbl_rsp *) h2crsp_addr;
	
	for (i = 0; i < NumRates ; i++)
		prsp->txagc[i] = sys_mib.rf_ctrl.txagc[i];
	
	//h2crsp_addr += sizeof (struct gettxagctbl_rsp);
	
	return H2C_SUCCESS_RSP;
}

 unsigned char	SetAgcCtrl(unsigned char *pbuf) 
{
	
	struct  setagcctrl_parm *pparm;
	pparm = (struct setagcctrl_parm *)pbuf;
	
	sys_mib.rf_ctrl.agcctrl = pparm->agcctrl;
	
	return H2C_SUCCESS;	
	

}

 unsigned char SetSsUp(unsigned char *pbuf)
{
	unsigned int i;
	struct setssup_parm *pparm;
	
	pparm = (struct setssup_parm *)pbuf;
	
	for (i = 0; i < NumRates; i++)
		sys_mib.rf_ctrl.ss_ForceUp[i] = pparm->ss_ForceUp[i];	

	return 	H2C_SUCCESS;
	
}


 unsigned char GetSsUp(unsigned char *pbuf)
{
	int	i;
	struct getssup_rsp *prsp;
	
	prsp  = (struct getssup_rsp *) h2crsp_addr;
	
	for (i = 0; i < NumRates ; i++)
		prsp->ss_ForceUp[i] = sys_mib.rf_ctrl.ss_ForceUp[i];
	
	//h2crsp_addr += sizeof (struct getssup_rsp);
	
	return H2C_SUCCESS_RSP;
	
}

 unsigned char SetSsULevel(unsigned char *pbuf)
{
	
	unsigned int i;
	struct setssulevel_parm *pparm;
	
	pparm = (struct setssulevel_parm *)pbuf;
	
	for (i = 0; i < NumRates; i++)
		sys_mib.rf_ctrl.ss_ULevel[i] = pparm->ss_ULevel[i];	

	return 	H2C_SUCCESS;
		
}

 unsigned char GetSsULevel(unsigned char *pbuf)
{
	int	i;
	struct getssulevel_rsp *prsp;
	
	prsp  = (struct getssulevel_rsp *) h2crsp_addr;
	
	for (i = 0; i < NumRates ; i++)
		prsp->ss_ULevel[i] = sys_mib.rf_ctrl.ss_ULevel[i];
	
	//h2crsp_addr += sizeof (struct getssulevel_rsp);
	
	return H2C_SUCCESS_RSP;
	
}

 unsigned char SetSsDLevel(unsigned char *pbuf)
{
	
	unsigned int i;
	struct setssdlevel_parm *pparm;
	
	pparm = (struct setssdlevel_parm *)pbuf;
	
	for (i = 0; i < NumRates; i++)
		sys_mib.rf_ctrl.ss_DLevel[i] = pparm->ss_DLevel[i];	

	return 	H2C_SUCCESS;
		
}

 unsigned char GetSsDLevel(unsigned char *pbuf)
{
	int	i;
	struct getssdlevel_rsp *prsp;
	
	prsp  = (struct getssdlevel_rsp *) h2crsp_addr;
	
	for (i = 0; i < NumRates ; i++)
		prsp->ss_DLevel[i] = sys_mib.rf_ctrl.ss_DLevel[i];
	
	//h2crsp_addr += sizeof (struct getssdlevel_rsp);
	
	return H2C_SUCCESS_RSP;
	
}

 unsigned char SetCountJudge(unsigned char *pbuf)
{
	
	unsigned int i;
	struct setcountjudge_parm *pparm;
	
	pparm = (struct setcountjudge_parm *)pbuf;
	
	for (i = 0; i < NumRates; i++)
		sys_mib.rf_ctrl.count_judge[i] = pparm->count_judge[i];	

	return 	H2C_SUCCESS;	
	
	
}


 unsigned char GetCountJudge(unsigned char *pbuf)
{
	int	i;
	struct getcountjudge_rsp *prsp;
	
	prsp  = (struct getcountjudge_rsp *) h2crsp_addr;
	
	for (i = 0; i < NumRates ; i++)
		prsp->count_judge[i] = sys_mib.rf_ctrl.count_judge[i];
	
	//h2crsp_addr += sizeof (struct getcountjudge_rsp);
	
	return H2C_SUCCESS_RSP;
	
}

 unsigned char	SetChannel(unsigned char	*pbuf)
{
	
	struct setchannel_parm *pparm;
	pparm = (struct setchannel_parm *)pbuf;
	
	sys_mib.mac_ctrl.rfchannel = pparm->channel;
	
	#ifdef CONFIG_RF
	
		SwChnl(pparm->channel);
	
	#endif
	
	return H2C_SUCCESS;
}


/*
struct	setbcnframe_parm {
	unsigned char 	timie_offset;
	unsigned char		qbssloadie_offset;
	unsigned short	bcn_size;
	unsigned int		bcnframe[(512-4-4-4)/4];	//4: cmdfile, 4:timeie/qbsslod/bcn_size, 4: 0xffffffff
};
*/

 unsigned char SetBcnFrame(unsigned char *pbuf)
{
	struct	setbcnframe_parm	*pparm;
	
	pparm = (struct	setbcnframe_parm *)pbuf;
	
	
	sys_mib.mac_ctrl.timie_offset = (pparm->timie_offset) + (u32)&sys_mib;
	sys_mib.mac_ctrl.qbssloadie_offset = (pparm->qbssloadie_offset) + (u32)&sys_mib;
	
	_memcpy(sys_mib.mac_ctrl.pbcn, pparm->bcnframe, pparm->bcn_size);

	sys_mib.mac_ctrl.bcn_size = (((pparm->bcn_size) << DMALEN_SHT) | DMA_READ | FIXED_TADDR | DMAOWN);
	
	//Setup DMA channel 0
	
	rtl_outl(CH0SRC, Virtual2Physical((u32)sys_mib.mac_ctrl.pbcn));
	rtl_outl(CH0DST, Virtual2Physical((u32)BCNFFWP));
	
	

	// Only turn on BCN Interrupt Interval Mask after Host driver SetBcnFrame.
	setfimr(BCNINT);
		
	
	return H2C_SUCCESS;
	 
}
#endif
#endif
