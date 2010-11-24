/******************************************************************************
* rtl871x_mlme.c                                                                                                                                 *
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
#define _RTL871X_MLME_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

/*
#ifdef PLATFORM_LINUX
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/smp_lock.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/circ_buf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#include <asm/semaphore.h>
#endif
*/



#include <recv_osdep.h>
#include <xmit_osdep.h>
#include <hal_init.h>
#include <mlme_osdep.h>
#include <sta_info.h>
#include <wifi.h>
#include <wlan_bssdef.h>

#ifdef CONFIG_RTL8192U
#include <wlan_mlme.h>
#endif /*CONFIG_RTL8192U*/

void generate_random_ibss(u8* pibss)
{
	u32	curtime = get_current_time();

_func_enter_;
	pibss[0] = 0x02;  //in ad-hoc mode bit1 must set to 1
	pibss[1] = 0x11;
	pibss[2] = 0x87;
	pibss[3] = (u8)(curtime & 0xff) ;//p[0];
	pibss[4] = (u8)((curtime>>8) & 0xff) ;//p[1];
	pibss[5] = (u8)((curtime>>16) & 0xff) ;//p[2];
_func_exit_;
	return;
}
uint get_NDIS_WLAN_BSSID_EX_sz (NDIS_WLAN_BSSID_EX *bss)
{
	uint t_len;
	
_func_enter_;	
	t_len = sizeof (ULONG) + sizeof (NDIS_802_11_MAC_ADDRESS) + 2 + 
			sizeof (NDIS_802_11_SSID) + sizeof (ULONG) + 
			sizeof (NDIS_802_11_RSSI) + sizeof (NDIS_802_11_NETWORK_TYPE) + 
			sizeof (NDIS_802_11_CONFIGURATION) +	
			sizeof (NDIS_802_11_NETWORK_INFRASTRUCTURE) +   
			sizeof (NDIS_802_11_RATES_EX)+ sizeof (ULONG) + bss->IELength;
	#ifdef CONFIG_RTL8192U

		// u8 for SLOTTIME
		t_len +=sizeof(BSS_HT)+sizeof(BSS_QOS)+sizeof(u8);
	
	#endif
_func_exit_;	
	return t_len;
	
}

u8 *get_capability_from_ie(u8 *ie)
{
	return (ie + 8 + 2);
}

u16 get_capability(NDIS_WLAN_BSSID_EX *bss)
{
	u16	val;
_func_enter_;	
	_memcpy((u8 *)&val, get_capability_from_ie(bss->IEs), 2); 
_func_exit_;		
	return val;
}

u8 *get_timestampe_from_ie(u8 *ie)
{
	return (ie + 0);	
}

u8 *get_beacon_interval_from_ie(u8 *ie)
{
	return (ie + 8);	
}

u16 get_beacon_interval(NDIS_WLAN_BSSID_EX *bss)
{
	unsigned short	val;
	_memcpy((unsigned char *)&val, get_beacon_interval_from_ie(bss->IEs), 2);
	return le16_to_cpu(val);	

}
#ifdef CONFIG_RTL8192U
VOID ResetBaEntry(void *context)
{
	PBABODY pBA = (PBABODY)context;
	pBA->bValid					= _FALSE;
	pBA->BaParamSet.shortData	= 0;
	pBA->BaTimeoutValue			= 0;
	pBA->DialogToken				= 0;
	pBA->BaStartSeqCtrl.ShortData	= 0;
}

VOID
ResetTxTsEntry(void *context)
{
	PTX_TS_RECORD pTS = (PTX_TS_RECORD)context;
	//ResetTsCommonInfo(&pTS->TsCommonInfo);
	pTS->TxCurSeq = 0;
	pTS->bAddBaReqInProgress = _FALSE;
	pTS->bAddBaReqDelayed = _FALSE;
	pTS->bUsingBa = _FALSE;
	ResetBaEntry(pTS->TxAdmittedBARecord); //For BA Originator
	ResetBaEntry(pTS->TxPendingBARecord);
}


VOID
TSInitialize(
	_adapter *Adapter
	)
{
	struct xmit_priv		*pxmitpriv = &Adapter->xmitpriv;
	PTX_TS_RECORD		pTxTS = &(pxmitpriv->TxTsRecord);
//	PRX_TS_RECORD		pRxTS = &(pxmitpriv->RxTsRecord);
//	PRX_REORDER_ENTRY	pRxReorderEntry = pxmitpriv->RxReorderEntry;
//	u8					count = 0;

printk("+TSInitialize\n");

	// Initialize Tx TS related info.
	_init_listhead(&pxmitpriv->Tx_TS_Admit_List);
	_init_listhead(&pxmitpriv->Tx_TS_Pending_List);
	_init_listhead(&pxmitpriv->Tx_TS_Unused_List);

	pTxTS->TxPendingBARecord = kmalloc(sizeof(BABODY), GFP_ATOMIC);
	if(pTxTS->TxPendingBARecord == NULL)
	{
		DEBUG_ERR(("malloc pTxTS->TxPendingBARecord failed"));
		return;
	}
	pTxTS->TxAdmittedBARecord = kmalloc(sizeof(BABODY), GFP_ATOMIC);
	if(pTxTS->TxAdmittedBARecord == NULL)
	{
		DEBUG_ERR(("malloc pTxTS->TxAdmittedBARecord failed"));
		return;
	}

	//for(count = 0; count < TOTAL_TS_NUM; count++)
	{
		//
		// The timers for the operation of Traffic Stream and Block Ack.
		// DLS related timer will be add here in the future!!
		//
#ifdef TODO		
		_init_timer(
			Adapter, 
			&pTxTS->TsCommonInfo.SetupTimer, 
			(RT_TIMER_CALL_BACK)TsSetupTimeOut	);

		_init_timer(
			Adapter, 
			&pTxTS->TsCommonInfo.InactTimer, 
			(RT_TIMER_CALL_BACK)TsInactTimeout	);
#endif
		//pTxTS->TsAddBaTimer.Context = pTxTS;
		_init_timer(
			&(pTxTS->TsAddBaTimer), 
			Adapter->pnetdev, 
			&TsInitAddBA, 
			Adapter);
		
		//pTxTS->TxPendingBARecord.Timer.Context = pTxTS;
		_init_timer(
			&(pTxTS->TxPendingBARecord->Timer),
			Adapter->pnetdev, 
			&BaSetupTimeOut,
			Adapter);

		//pTxTS->TxAdmittedBARecord.Timer.Context = pTxTS;
		_init_timer(
			&pTxTS->TxAdmittedBARecord->Timer,
			Adapter->pnetdev, 
			&TxBaInactTimeout,
			Adapter);
		
		ResetTxTsEntry(pTxTS);
		//RTInsertTailList(&pMgntInfo->Tx_TS_Unused_List, &pTxTS->TsCommonInfo.List);
		//pTxTS++;
	}
	
#ifdef TODO
	// Initialize Rx TS related info.
	_init_listhead(&pMgntInfo->Rx_TS_Admit_List);
	_init_listhead(&pMgntInfo->Rx_TS_Pending_List);
	_init_listhead(&pMgntInfo->Rx_TS_Unused_List);
	for(count = 0; count < TOTAL_TS_NUM; count++)
	{
		RTInitializeListHead(&pRxTS->RxPendingPktList);

		PlatformInitializeTimer(
			Adapter, 
			&pRxTS->TsCommonInfo.SetupTimer, 
			(RT_TIMER_CALL_BACK)TsSetupTimeOut	);

		PlatformInitializeTimer(
			Adapter, 
			&pRxTS->TsCommonInfo.InactTimer, 
			(RT_TIMER_CALL_BACK)TsInactTimeout	);

		pRxTS->RxAdmittedBARecord.Timer.Context = pRxTS;
		PlatformInitializeTimer(
			Adapter,
			&pRxTS->RxAdmittedBARecord.Timer,
			(RT_TIMER_CALL_BACK)RxBaInactTimeout);
	
		pRxTS->RxPktPendingTimer.Context = pRxTS;
		PlatformInitializeTimer(
			Adapter,
			&pRxTS->RxPktPendingTimer,
			(RT_TIMER_CALL_BACK)RxPktPendingTimeout);

		ResetRxTsEntry(pRxTS);
		RTInsertTailList(&pMgntInfo->Rx_TS_Unused_List, &pRxTS->TsCommonInfo.List);
		pRxTS++;
	}

	// Initialize unused Rx Reorder List.
	RTInitializeListHead(&pMgntInfo->RxReorder_Unused_List);
	for(count = 0; count < REORDER_ENTRY_NUM; count++)
	{
		RTInsertTailList(&pMgntInfo->RxReorder_Unused_List, &pRxReorderEntry->List);
		pRxReorderEntry++;
	}
#endif /*TODO*/	
printk("-TSInitialize");
}

void TSDeInitialize(_adapter *padapter)
{
	u8 					canceled;
	struct xmit_priv		*pxmitpriv = &padapter->xmitpriv;
	PTX_TS_RECORD		pTxTS = &(pxmitpriv->TxTsRecord);

	DeActivateBAEntry(padapter, (pTxTS->TxPendingBARecord));
	DeActivateBAEntry(padapter, (pTxTS->TxAdmittedBARecord));
	
	if(pTxTS->TxPendingBARecord)
	{
		kfree(pTxTS->TxPendingBARecord);
	}
	if(pTxTS->TxAdmittedBARecord == NULL)
	{
		kfree(pTxTS->TxAdmittedBARecord);
	}

	_cancel_timer(&(pTxTS->TsAddBaTimer), &canceled);

}

void HTInitializeHTInfo(_adapter *padapter)
{
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;

	//
	// These parameters will be reset when receiving deauthentication packet 
	//
	pHTInfo->SelfMimoPs = MIMO_PS_DYNAMIC;
	pHTInfo->bCurrentHTSupport = _FALSE;
	pHTInfo->bRegBW40MHz = _FALSE;

	pHTInfo->CurrentChannelBW = HT_CHANNEL_WIDTH_20 ;
	pHTInfo->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

	
	// 40MHz channel support
	pHTInfo->bCurBW40MHz = _FALSE;
	pHTInfo->bCurTxBW40MHz = _FALSE;

	// Short GI support
	pHTInfo->bCurShortGI20MHz = _FALSE;
	pHTInfo->bCurShortGI40MHz = _FALSE;
	pHTInfo->bForcedShortGI = _FALSE;

	// CCK rate support
	// This flag is set to TRUE to support CCK rate by default.
	// It will be affected by "pHTInfo->bRegSuppCCK" and AP capabilities only when associate to
	// 11N BSS.
	pHTInfo->bCurSuppCCK = _TRUE;

	// AMSDU related
	pHTInfo->bCurrent_AMSDU_Support = _TRUE;
	pHTInfo->nCurrent_AMSDU_MaxSize = pHTInfo->nAMSDU_MaxSize;

	// AMPUD related
	pHTInfo->CurrentMPDUDensity = pHTInfo->MPDU_Density;
	pHTInfo->CurrentAMPDUFactor = pHTInfo->AMPDU_Factor;

	//HT Data Rate
	pHTInfo->HTCurrentOperaRate = 0;
	pHTInfo->HTHighestOperaRate = 0 ;
	

	// Initialize all of the parameters related to 11n	
	memset((PVOID)(&(pHTInfo->SelfHTCap)),0, sizeof(pHTInfo->SelfHTCap));
	memset((PVOID)(&(pHTInfo->SelfHTInfo)), 0,sizeof(pHTInfo->SelfHTInfo));
	memset((PVOID)(&(pHTInfo->PeerHTCapBuf)), 0,sizeof(pHTInfo->PeerHTCapBuf));
	memset((PVOID)(&(pHTInfo->PeerHTInfoBuf)), 0,sizeof(pHTInfo->PeerHTInfoBuf));

	// For Bandwidth Switching
	
	pHTInfo->bSwBwInProgress = _FALSE;
	pHTInfo->bChnlOp_Scan = _FALSE;
	pHTInfo->bChnlOp_SwBw = _FALSE;

	// Realtek proprietary aggregation mode
	pHTInfo->bCurrentRT2RTAggregation = _FALSE;
	pHTInfo->bCurrentRT2RTLongSlotTime = _FALSE;
	memset(pHTInfo->dot11HTOperationalRateSet, 0 , 16);

#ifdef USB_RX_AGGREGATION_SUPPORT
	pHTInfo->UsbRxFwAggrEn = 1;
	pHTInfo->UsbRxFwAggrPageNum = 24;
	pHTInfo->UsbRxFwAggrPacketNum = 8;
	pHTInfo->UsbRxFwAggrTimeout = 16;
#endif
	
	
}


#endif





int	init_mlme_priv (_adapter *padapter)//(struct	mlme_priv *pmlmepriv)
{
	sint	i;
	u8	*pbuf;
	struct wlan_network	*pnetwork;
	struct	mlme_priv *pmlmepriv = &padapter->mlmepriv;
	sint	res=_SUCCESS;
_func_enter_;	

	DEBUG_ERR(("+init_mlme_priv\n"));
#if (defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U))
	_init_mibinfo(padapter);
#endif
	memset((u8 *)pmlmepriv, 0, sizeof(struct mlme_priv));
	pmlmepriv->nic_hdl = (u8 *)padapter;

	pmlmepriv->pscanned = NULL;
	pmlmepriv->fw_state = 0;
	pmlmepriv->cur_network.network.InfrastructureMode = Ndis802_11AutoUnknown;

#if 1
	pmlmepriv->pnow_assoc_network = NULL;
#endif

		
	_spinlock_init(&(pmlmepriv->lock));	//jackson 0813
	_init_queue(&(pmlmepriv->free_bss_pool));
	_init_queue(&(pmlmepriv->scanned_queue));

#if 1 //yt : 1221
	set_scanned_network_val(pmlmepriv, 0);

	memset(&pmlmepriv->assoc_ssid,0,sizeof(NDIS_802_11_SSID));
#endif

	pbuf = _malloc(MAX_BSS_CNT * (sizeof(struct wlan_network)));
	
	if (pbuf == NULL){
		res=_FAIL;
		goto exit;
	}
	pmlmepriv->free_bss_buf = pbuf;
		
	pnetwork = (struct wlan_network *)pbuf;
	
	for(i = 0; i < MAX_BSS_CNT; i++)
	{		
		_init_listhead(&(pnetwork->list));

		list_insert_tail(&(pnetwork->list), &(pmlmepriv->free_bss_pool.queue));

		pnetwork++;
	}

	pbuf = _malloc(sizeof(struct sitesurvey_ctrl));

	pmlmepriv->sitesurveyctrl.last_rx_pkts=0;
	pmlmepriv->sitesurveyctrl.last_tx_pkts=0;
	pmlmepriv->sitesurveyctrl.traffic_busy=_FALSE;
	
	init_mlme_timer(padapter);

	//john debug 0521
	_init_sema(&pmlmepriv->ioctl_sema, 1);

#ifdef ENABLE_MGNT_THREAD
	_init_sema(&pmlmepriv->mgnt_sema, 0);//mgnt_sema is used in mgnt_thread
	_init_queue(&pmlmepriv->mgnt_pending_queue);
#endif
	

exit:
	DEBUG_ERR(("-init_mlme_priv\n"));
_func_exit_;	
	return res;
}

void free_mlme_priv (struct mlme_priv *pmlmepriv)
{
_func_enter_;	
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	_free_mibinfo((_adapter *)pmlmepriv->nic_hdl);
#endif
	if (pmlmepriv->free_bss_buf)
		_mfree(pmlmepriv->free_bss_buf, MAX_BSS_CNT * sizeof(struct wlan_network));
_func_exit_;	
}

int	enqueue_network(_queue *queue, struct wlan_network *pnetwork)
{
	_irqL irqL;
_func_enter_;		
	if (pnetwork == NULL)
		goto exit;
	
	_enter_critical(&queue->lock, &irqL);

	list_insert_tail(&(pnetwork->list),&(queue->queue)); 

	_exit_critical(&queue->lock, &irqL);
exit:	
_func_exit_;		
	return _SUCCESS;
}



struct	wlan_network *dequeue_network(_queue *queue)
{
	_irqL irqL;

	struct wlan_network *pnetwork;
_func_enter_;		
	_enter_critical(&queue->lock, &irqL);

	if (_queue_empty(queue) == _TRUE)

		pnetwork = NULL;
	
	else
	{
		pnetwork = LIST_CONTAINOR(get_next(&(queue->queue)), struct wlan_network, list);
		
		list_delete(&(pnetwork->list));
	}
	
	_exit_critical(&queue->lock, &irqL);
_func_exit_;		
	return pnetwork;
}


struct	wlan_network *alloc_network(struct	mlme_priv *pmlmepriv )//(_queue	*free_queue)
{
	_irqL	irqL;
	struct	wlan_network	*pnetwork;
	_queue *free_queue = &(pmlmepriv->free_bss_pool);
	_list* plist = NULL;
	
_func_enter_;			
	_enter_critical(&free_queue->lock, &irqL);
	
	if ((_queue_empty(free_queue)) == _TRUE){
		pnetwork=NULL;
		goto exit;
	}
	plist = get_next(&(free_queue->queue));
	
	pnetwork = LIST_CONTAINOR(plist , struct wlan_network, list);
	
	list_delete(&(pnetwork->list));
	
	DEBUG_INFO(("_alloc_network : ptr = %p; \n ",plist));
	
	pnetwork->last_scanned = get_current_time();
	pmlmepriv->num_of_scanned ++;
	
	_exit_critical(&free_queue->lock, &irqL);
exit:	
_func_exit_;			
	return pnetwork;
}

void free_network(struct mlme_priv *pmlmepriv, struct	wlan_network *pnetwork )//(struct	wlan_network *pnetwork, _queue	*free_queue)
{
	_irqL irqL;
	_queue *free_queue = &(pmlmepriv->free_bss_pool);
_func_enter_;		
	if (pnetwork == NULL)
		goto exit;

	if (pnetwork->fixed == _TRUE)
		goto exit;

	_enter_critical(&free_queue->lock, &irqL);
	
	list_delete(&(pnetwork->list));

	list_insert_tail(&(pnetwork->list),&(free_queue->queue));
		
	pmlmepriv->num_of_scanned --;
	
	_exit_critical(&free_queue->lock, &irqL);
exit:		
_func_exit_;		
}


void free_network_nolock(struct mlme_priv *pmlmepriv, struct	wlan_network *pnetwork )//(struct	wlan_network *pnetwork, _queue	*free_queue)
{
	//_irqL irqL;
	_queue *free_queue = &(pmlmepriv->free_bss_pool);

_func_enter_;		
	if (pnetwork == NULL)
		goto exit;

	if (pnetwork->fixed == _TRUE)
		goto exit;

	//_enter_critical(&free_queue->lock, &irqL);
	
	list_delete(&(pnetwork->list));

	list_insert_tail(&(pnetwork->list),get_list_head(free_queue));
		
	pmlmepriv->num_of_scanned --;
	
	//_exit_critical(&free_queue->lock, &irqL);
exit:		
_func_exit_;		
}


void free_network_queue(_adapter* padapter)
{
	_irqL irqL;
	_list *phead, *plist;
	struct wlan_network *pnetwork;
	struct mlme_priv* pmlmepriv = &padapter->mlmepriv;
	_queue *scanned_queue = &pmlmepriv->scanned_queue;
	//_queue	*free_queue = &pmlmepriv->free_bss_pool;
	//u8 *mybssid = get_bssid(pmlmepriv);//myid(adapter);

_func_enter_;		
	
	_enter_critical(&scanned_queue->lock, &irqL);

	phead = get_list_head(scanned_queue);
	plist = get_next(phead);

	while (end_of_queue_search(phead, plist) == _FALSE)
	{

		pnetwork = LIST_CONTAINOR(plist, struct wlan_network, list);

		plist = get_next(plist);

		free_network(pmlmepriv,pnetwork);
		
	}

	_exit_critical(&scanned_queue->lock, &irqL);
_func_exit_;			
}



/*
	return the wlan_network with the matching addr

	Shall be calle under atomic context... to avoid possible racing condition...
*/
struct	wlan_network *find_network(_queue *scanned_queue, u8 *addr)
{
	_irqL irqL;
	_list	*phead, *plist;
	struct	wlan_network *pnetwork = NULL;
	u8 zero_addr[ETH_ALEN] = {0,0,0,0,0,0};
	
_func_enter_;	
	if(_memcmp(zero_addr, addr, ETH_ALEN)){
		pnetwork=NULL;
		goto exit;
		}
	_enter_critical(&scanned_queue->lock, &irqL);

	phead = get_list_head(scanned_queue);
	plist = get_next(phead);
	 
	while (plist != phead)
       {
                pnetwork = LIST_CONTAINOR(plist, struct wlan_network ,list);
                plist = get_next(plist);
                if ((_memcmp(addr, pnetwork->network.MacAddress, 6)) == _TRUE)
                        break;
        }

	_exit_critical(&scanned_queue->lock, &irqL);
exit:		
_func_exit_;		
	return pnetwork;

}

sint if_up(_adapter *padapter)	{

	//yitsen
	sint res;
_func_enter_;		
	if( padapter->bDriverStopped || padapter->bSurpriseRemoved ||
		(check_fwstate(&padapter->mlmepriv, _FW_LINKED)== _FALSE)){
		res=_FALSE;
	}
	else
		res=  _TRUE;
	
_func_exit_;
	return res;
}

static int is_same_network(NDIS_WLAN_BSSID_EX *src, NDIS_WLAN_BSSID_EX *dst)
{

	 u16 s_cap, d_cap;
_func_enter_;	
	 if ( ((uint)dst) <= 0x7fffffff || 
	 	((uint)src) <= 0x7fffffff ||
	 	((uint)&s_cap) <= 0x7fffffff ||
	 	((uint)&d_cap) <= 0x7fffffff)
			{
			DEBUG_ERR(("\n@@@@ error address of dst\n"));
#ifdef PLATFORM_OS_XP			
			KeBugCheckEx(0x87110000, (ULONG_PTR)dst, (ULONG_PTR)src,(ULONG_PTR)&s_cap, (ULONG_PTR)&d_cap);
#endif
			return _FALSE;
			}

/*
	if ( ((unsigned int)dst) <= 0x7fffffff)
	{
		DEBUG_ERR("\n@@@@ error address of dst\n");
		KeBugCheck(0x87110000, dst, src, &s_cap, &d_cap);
		return _FALSE;
	}

	if ( ((unsigned int)src) <= 0x7fffffff)
	{
		DEBUG_ERR("\n@@@@ error address of src\n");
		KeBugCheck(0x87110001, dst, src, &s_cap, &d_cap);
		return _FALSE;
	}
*/
	_memcpy((u8 *)&s_cap, get_capability_from_ie(src->IEs), 2);
	_memcpy((u8 *)&d_cap, get_capability_from_ie(dst->IEs), 2);
_func_exit_;			
	return ((src->Ssid.SsidLength == dst->Ssid.SsidLength) &&
		(src->Configuration.DSConfig == dst->Configuration.DSConfig) &&
		( (_memcmp(src->MacAddress, dst->MacAddress, ETH_ALEN)) == _TRUE) &&
		( (_memcmp(src->Ssid.Ssid, dst->Ssid.Ssid, src->Ssid.SsidLength)) == _TRUE) &&
		((s_cap & WLAN_CAPABILITY_IBSS) == 
		(d_cap & WLAN_CAPABILITY_IBSS)) &&
		((s_cap & WLAN_CAPABILITY_BSS) == 
		(d_cap & WLAN_CAPABILITY_BSS)));
}

struct	wlan_network	* get_oldest_wlan_network(_queue *scanned_queue)
{
	_list	*plist, *phead;

	
	struct	wlan_network	*pwlan = NULL;
	struct	wlan_network	*oldest = NULL;
_func_enter_;		
	phead = get_list_head(scanned_queue);
	
	plist = get_next(phead);

	while(1)
	{
		
		if (end_of_queue_search(phead,plist)== _TRUE)
			break;
		
		pwlan= LIST_CONTAINOR(plist, struct wlan_network, list);

		if(pwlan->fixed!=_TRUE)
		{		
			if (oldest == NULL ||time_after(oldest->last_scanned, pwlan->last_scanned))
				oldest = pwlan;
		}
		
		plist = get_next(plist);
	}
_func_exit_;		
	return oldest;
	
}


static void update_network(NDIS_WLAN_BSSID_EX *dst, NDIS_WLAN_BSSID_EX *src)
{
_func_enter_;		
	_memcpy((u8 *)dst, (u8 *)src, get_NDIS_WLAN_BSSID_EX_sz(src));
_func_exit_;		
}

static void update_current_network(_adapter *adapter, NDIS_WLAN_BSSID_EX *pnetwork)
{
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
_func_enter_;		

	if ((unsigned long)(&(pmlmepriv->cur_network.network)) < 0x7ffffff)
	{
#ifdef PLATFORM_OS_XP		
				KeBugCheckEx(0x87111c1c, (ULONG_PTR)(&(pmlmepriv->cur_network.network)), 0, 0,0);
#endif
	}

	if(is_same_network(&(pmlmepriv->cur_network.network), pnetwork))
	{
		//DEBUG_ERR("Same Network\n");
		update_network(&(pmlmepriv->cur_network.network), pnetwork);
		update_protection(adapter, 
		(pmlmepriv->cur_network.network.IEs) + sizeof (NDIS_802_11_FIXED_IEs), 
		pmlmepriv->cur_network.network.IELength);
	}	
	
_func_exit_;			
}


/*

Caller must hold pmlmepriv->lock first.


*/
void update_scanned_network(_adapter *adapter, NDIS_WLAN_BSSID_EX *target)
{
	_list	*plist, *phead;
	
	ULONG	bssid_ex_sz;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	_queue	*queue	= &(pmlmepriv->scanned_queue);
	struct	wlan_network	*pnetwork = NULL;
	struct  wlan_network	*oldest = NULL;
_func_enter_;			

	
	phead = get_list_head(queue);
	
	plist = get_next(phead);
	
	
	while(1)
	{
		
		if (end_of_queue_search(phead,plist)== _TRUE)
			break;
		
		pnetwork	= LIST_CONTAINOR(plist, struct wlan_network, list);

		if ((unsigned long)(pnetwork) < 0x7ffffff)
		{
#ifdef PLATFORM_OS_XP	
				KeBugCheckEx(0x87111c1c, (ULONG_PTR)pnetwork, 0, 0,0);
#endif
		}

		if (is_same_network(&(pnetwork->network), target))
			break;
		
		if ((oldest == ((struct wlan_network *)0)) ||
		time_after(oldest->last_scanned, pnetwork->last_scanned))
			oldest = pnetwork;
		
		plist = get_next(plist);
		
	}
	
	
	/* If we didn't find a match, then get a new network slot to initialize
	 * with this beacon's information */
	if (end_of_queue_search(phead,plist)== _TRUE) {
		
		if (_queue_empty(&(pmlmepriv->free_bss_pool)) == _TRUE) {
			/* If there are no more slots, expire the oldest */
			//list_del_init(&oldest->list);
			pnetwork = oldest;
			_memcpy(&(pnetwork->network), target,  get_NDIS_WLAN_BSSID_EX_sz(target));
			pnetwork->last_scanned = get_current_time();

		} else {
			/* Otherwise just pull from the free list */
			
			pnetwork = alloc_network(pmlmepriv); // will update scan_time


			if(pnetwork==NULL){ 
				DEBUG_ERR(("\n\n\nsomething wrong here\n\n\n"));
				goto exit;
			}


			bssid_ex_sz = get_NDIS_WLAN_BSSID_EX_sz(target);
			target->Length = bssid_ex_sz;
			
			_memcpy(&(pnetwork->network), target, bssid_ex_sz );

			list_insert_tail(&(pnetwork->list),&(queue->queue)); 

			
		}	 
	} else {
		/* we have an entry and we are going to update it. But this entry may
		 * be already expired. In this case we do the same as we found a new 
		 * net and call the new_net handler
		 */
		update_network(&(pnetwork->network),target);

	}


exit:	
_func_exit_;			

}


void rtl8711_add_network(_adapter *adapter, NDIS_WLAN_BSSID_EX *pnetwork)
{
	_irqL irqL;
	struct	mlme_priv	*pmlmepriv = &(((_adapter *)adapter)->mlmepriv);
	_queue	*queue	= &(pmlmepriv->scanned_queue);

_func_enter_;			
	_enter_critical(&queue->lock, &irqL);
	
	update_current_network(adapter, pnetwork);
	
	update_scanned_network(adapter, pnetwork);

	_exit_critical(&queue->lock, &irqL);
_func_exit_;		
}

/* TODO: Perry : For Power Management */
void atimdone_event_callback(_adapter	*adapter , u8 *pbuf)
{

_func_enter_;		
	DEBUG_ERR(("receive atimdone_evet\n"));	
_func_exit_;			
	return;	
}


void survey_event_callback(_adapter	*adapter, u8 *pbuf)
{
	_irqL  irqL;
	u32 len;
	NDIS_WLAN_BSSID_EX *pnetwork;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);

_func_enter_;			

	pnetwork = (NDIS_WLAN_BSSID_EX *)pbuf;
	//double check

        //endian_convert
        pnetwork->Ssid.SsidLength = cpu_to_le32(pnetwork->Ssid.SsidLength);
	pnetwork->Privacy =cpu_to_le32( pnetwork->Privacy);
	pnetwork->Rssi = cpu_to_le32(pnetwork->Rssi);
	pnetwork->NetworkTypeInUse =cpu_to_le32(pnetwork->NetworkTypeInUse) ;
	
	pnetwork->Configuration.ATIMWindow = cpu_to_le32(pnetwork->Configuration.ATIMWindow);
	pnetwork->Configuration.BeaconPeriod = cpu_to_le32(pnetwork->Configuration.BeaconPeriod);
	pnetwork->Configuration.DSConfig =cpu_to_le32(pnetwork->Configuration.DSConfig);
	pnetwork->Configuration.FHConfig.DwellTime=cpu_to_le32(pnetwork->Configuration.FHConfig.DwellTime);

	pnetwork->Configuration.FHConfig.HopPattern=cpu_to_le32(pnetwork->Configuration.FHConfig.HopPattern);
	pnetwork->Configuration.FHConfig.HopSet=cpu_to_le32(pnetwork->Configuration.FHConfig.HopSet);
	pnetwork->Configuration.FHConfig.Length=cpu_to_le32(pnetwork->Configuration.FHConfig.Length);

	
	pnetwork->Configuration.Length = cpu_to_le32(pnetwork->Configuration.Length);
	pnetwork->InfrastructureMode = cpu_to_le32(pnetwork->InfrastructureMode );
	pnetwork->IELength = cpu_to_le32(pnetwork->IELength );
	// TODO: HT Info & Cab endian issue ?

	len = get_NDIS_WLAN_BSSID_EX_sz(pnetwork);
	if(len > (sizeof(NDIS_WLAN_BSSID_EX) + 192))
	{
		DEBUG_ERR(("\n ****survey_event_callback: return a wrong bss ***\n"));
		goto exit;
	}


	_enter_critical(&pmlmepriv->lock, &irqL);


	// update IBSS_network 's timestamp
	if ((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)) == _TRUE)
	{
		//DEBUG_ERR("survey_event_callback : WIFI_ADHOC_MASTER_STATE \n\n");
		if(_memcmp(&(pmlmepriv->cur_network.network.MacAddress), pnetwork->MacAddress, ETH_ALEN))
		{
			struct wlan_network* ibss_wlan = NULL;
			
			_memcpy(pmlmepriv->cur_network.network.IEs, pnetwork->IEs, 8);

			ibss_wlan = find_network(&pmlmepriv->scanned_queue,  pnetwork->MacAddress);
			if(!ibss_wlan)
			{
				_memcpy(ibss_wlan->network.IEs , pnetwork->IEs, 8);			
				goto exit;
			}
		}
	}

	// lock pmlmepriv->lock when you accessing network_q
	if ((check_fwstate(pmlmepriv, _FW_UNDER_LINKING)) == _FALSE){
		rtl8711_add_network(adapter, pnetwork);
	}

	_exit_critical(&pmlmepriv->lock, &irqL);

exit:	
_func_exit_;		
	return;
	
}



void surveydone_event_callback(_adapter	*adapter, u8 *pbuf)
{
	_irqL  irqL;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	
	DEBUG_INFO(("+surveydone_event_callback\n"));	
_func_enter_;			
	_enter_critical(&pmlmepriv->lock, &irqL);
	
	DEBUG_INFO(("surveydone_event_callback: fw_state:%x\n\n", pmlmepriv->fw_state));
	
	if  (pmlmepriv->fw_state & _FW_UNDER_SURVEY)
	{
		pmlmepriv->fw_state &= ~_FW_UNDER_SURVEY;
	}
	else {
	
		DEBUG_ERR(("nic status =%x, survey done event comes too late!\n", pmlmepriv->fw_state));
	
	}	
	
	DEBUG_ERR(("surveydone_event_callback(2): fw_state:%x\n\n", pmlmepriv->fw_state));
	if(pmlmepriv->to_join == _TRUE)
	{

	if((check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)==_TRUE) )
	{
		if(check_fwstate(pmlmepriv, _FW_LINKED)==_FALSE)
		{

		   if(select_and_join_from_scanned_queue(pmlmepriv)==_SUCCESS)
		   {
			DEBUG_ERR(("\n set_timer assoc_timer\n"));
		       _set_timer(&pmlmepriv->assoc_timer, MAX_JOIN_TIMEOUT );	 
                   }
		   else	
		   {
			NDIS_WLAN_BSSID_EX    *pdev_network = &(adapter->registrypriv.dev_network); 			
			u8 *pibss = adapter->registrypriv.dev_network.MacAddress;

			DEBUG_ERR(("switching to adhoc master\n"));
				
			memset(&pdev_network->Ssid, 0, sizeof(NDIS_802_11_SSID));
			_memcpy(&pdev_network->Ssid, &pmlmepriv->assoc_ssid, sizeof(NDIS_802_11_SSID));
	
			update_registrypriv_dev_network(adapter);
			generate_random_ibss(pibss);

                        pmlmepriv->fw_state = WIFI_ADHOC_MASTER_STATE;
			
			if(createbss_cmd(adapter)!=_SUCCESS)
			{
	                     DEBUG_ERR(("Error=>createbss_cmd status FAIL\n"));						
			}	

		   }
		}

	}
	}

	_exit_critical(&pmlmepriv->lock, &irqL);	

	DEBUG_INFO(("-surveydone_event_callback\n"));	
	
_func_exit_;	
}

void free_scanqueue(struct	mlme_priv *pmlmepriv)
{
	_irqL irqL;
	_queue *free_queue = &(pmlmepriv->free_bss_pool);
	_queue *scan_queue = &(pmlmepriv->scanned_queue);
	_list	*plist, *phead, *ptemp;
//	int counter=0;
	
_func_enter_;		
	
	_enter_critical(&free_queue->lock, &irqL);

	phead = get_list_head(scan_queue);
	plist = get_next(phead);

	while (plist != phead)
       {
		ptemp = get_next(plist);
		list_delete(plist);
		list_insert_tail(plist,&(free_queue->queue));
		plist =ptemp;
		pmlmepriv->num_of_scanned --;
        }
	
	_exit_critical(&free_queue->lock, &irqL);
_func_exit_;			
}
	


/*
*free_assoc_resources: the caller has to lock pmlmepriv->lock
*/
void free_assoc_resources(_adapter *adapter )
{
	_irqL irqL;
	struct wlan_network* pwlan = NULL;
     	struct	mlme_priv *pmlmepriv = &adapter->mlmepriv;
   	struct	sta_priv *pstapriv = &adapter->stapriv;
	struct 	wlan_network *tgt_network = &(pmlmepriv->cur_network);
_func_enter_;			
	pwlan = find_network(&pmlmepriv->scanned_queue, tgt_network->network.MacAddress);
	DEBUG_INFO(("tgt_network->network.MacAddress = %x %x %x %x %x %x ssid = %s\n", 
		tgt_network->network.MacAddress[0],tgt_network->network.MacAddress[1]
		,tgt_network->network.MacAddress[2],tgt_network->network.MacAddress[3],
		tgt_network->network.MacAddress[4],tgt_network->network.MacAddress[5], 
		tgt_network->network.Ssid.Ssid));

	if(check_fwstate( pmlmepriv, WIFI_STATION_STATE))
	{
		struct sta_info* psta;
		
		psta = get_stainfo(&adapter->stapriv, tgt_network->network.MacAddress);

		_enter_critical(&(pstapriv->sta_hash_lock), &irqL);
		free_stainfo(adapter,  psta);
		_exit_critical(&(pstapriv->sta_hash_lock), &irqL);
		
	}

	if(check_fwstate( pmlmepriv, WIFI_ADHOC_STATE) || 
		check_fwstate( pmlmepriv, WIFI_ADHOC_MASTER_STATE))
	{
		free_all_stainfo(adapter);
	}

	if(pwlan)		
		pwlan->fixed = _FALSE;
	else{
		DEBUG_ERR(("free_assoc_resources : pwlan== NULL \n\n"));
	}


	if((check_fwstate( pmlmepriv, WIFI_ADHOC_MASTER_STATE) && (adapter->stapriv.asoc_sta_count== 1))
		||check_fwstate( pmlmepriv, WIFI_STATION_STATE))
		free_network_nolock(pmlmepriv, pwlan); 



_func_exit_;	
}


/*
*indicate_connect: the caller has to lock pmlmepriv->lock
*/
void indicate_connect(_adapter *adapter)
{

	struct	mlme_priv *pmlmepriv = &adapter->mlmepriv;
_func_enter_;	
	DEBUG_INFO(("\n\n indicate_connect ======\n\n"));
 
	pmlmepriv->to_join = _FALSE;

	os_indicate_connect(adapter);

	if((pmlmepriv->fw_state & _FW_LINKED) == _FALSE)
		pmlmepriv->fw_state |= _FW_LINKED;

	DEBUG_INFO(("\n after indicate_connect, fw_state=%d\n\n", pmlmepriv->fw_state));
 
_func_exit_;	
}


/*
*indicate_connect: the caller has to lock pmlmepriv->lock
*/
void indicate_disconnect( _adapter *adapter )
{
	
	struct	mlme_priv *pmlmepriv = &adapter->mlmepriv;
_func_enter_;	
	
	DEBUG_ERR(("\n indicate_disconnect \n\n"));

	if((pmlmepriv->fw_state & _FW_LINKED))//if(!(pmlmepriv->fw_state & _FW_LINKED))
		pmlmepriv->fw_state ^= _FW_LINKED;
	
	os_indicate_disconnect(adapter);
 	
_func_exit_;	
}


void joinbss_event_callback(_adapter *adapter, u8 *pbuf)
{

	_irqL irqL,irqL2;
	int	res;
	u8 timer_cancelled;
	struct sta_info *ptarget_sta= NULL, *pcur_sta = NULL;
   	struct	sta_priv *pstapriv = &adapter->stapriv;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	struct wlan_network 	*pnetwork	= (struct wlan_network *)pbuf;
	struct wlan_network 	*cur_network = &(pmlmepriv->cur_network);
	struct wlan_network	*pcur_wlan = NULL, *ptarget_wlan = NULL;
	unsigned int 		the_same_macaddr = _FALSE;

        //endian_convert
	pnetwork->join_res = cpu_to_le32(pnetwork->join_res);
//	pnetwork->network_type = cpu_to_le32(pnetwork->network_type);
	

	pnetwork->network.Ssid.SsidLength = cpu_to_le32(pnetwork->network.Ssid.SsidLength);
	pnetwork->network.Privacy =cpu_to_le32( pnetwork->network.Privacy);
	pnetwork->network.Rssi =0;// cpu_to_le32(pnetwork->Rssi);
	pnetwork->network.NetworkTypeInUse =cpu_to_le32(pnetwork->network.NetworkTypeInUse) ;
	
	pnetwork->network.Configuration.ATIMWindow = cpu_to_le32(pnetwork->network.Configuration.ATIMWindow);
	pnetwork->network.Configuration.BeaconPeriod = cpu_to_le32(pnetwork->network.Configuration.BeaconPeriod);
	pnetwork->network.Configuration.DSConfig =cpu_to_le32(pnetwork->network.Configuration.DSConfig);
	pnetwork->network.Configuration.FHConfig.DwellTime=cpu_to_le32(pnetwork->network.Configuration.FHConfig.DwellTime);

	pnetwork->network.Configuration.FHConfig.HopPattern=cpu_to_le32(pnetwork->network.Configuration.FHConfig.HopPattern);
	pnetwork->network.Configuration.FHConfig.HopSet=cpu_to_le32(pnetwork->network.Configuration.FHConfig.HopSet);
	pnetwork->network.Configuration.FHConfig.Length=cpu_to_le32(pnetwork->network.Configuration.FHConfig.Length);

	
	pnetwork->network.Configuration.Length = cpu_to_le32(pnetwork->network.Configuration.Length);
//	pnetwork->network.InfrastructureMode = cpu_to_le32(pnetwork->network.InfrastructureMode );
	pnetwork->network.IELength = cpu_to_le32(pnetwork->network.IELength );

_func_enter_;	
	DEBUG_INFO(("joinbss event call back received with res=%d\n", pnetwork->join_res));
	get_encrypt_decrypt_from_registrypriv(adapter);


	if (pmlmepriv->assoc_ssid.SsidLength == 0){
		DEBUG_ERR(("@@@@@   joinbss event call back  for Any SSid\n"));		
	}
	else{
		DEBUG_ERR(("@@@@@   joinbss_event_callback for SSid:%s\n", pmlmepriv->assoc_ssid.Ssid));
	}

	the_same_macaddr = _memcmp(pnetwork->network.MacAddress, cur_network->network.MacAddress, ETH_ALEN);

	_enter_critical(&pmlmepriv->lock, &irqL);
	DEBUG_INFO(("\n joinbss_event_callback !! remove spinlock \n"));

	if (pnetwork->join_res > 0) {

		cur_network->join_res = pnetwork->join_res;

		if ((pmlmepriv->fw_state) & _FW_UNDER_LINKING) 
		{
			DEBUG_INFO(("pnetwork->network addr: %p\n", &pnetwork->network));
			pnetwork->network.Length = get_NDIS_WLAN_BSSID_EX_sz(&pnetwork->network);
			if(pnetwork->network.Length > (sizeof(NDIS_WLAN_BSSID_EX)+192))
			{
				DEBUG_ERR(("\n\n ***joinbss_evt_callback return a wrong bss ***\n\n"));
				goto ignore_joinbss_callback;
			}
				
			if((pmlmepriv->fw_state) & _FW_LINKED)
			{
				if(the_same_macaddr == _TRUE)
				{
					ptarget_wlan = find_network(&pmlmepriv->scanned_queue, cur_network->network.MacAddress);

					// update network in pscanned_q
					_memcpy(&(ptarget_wlan->network), &pnetwork->network, pnetwork->network.Length );
					ptarget_sta = get_stainfo(pstapriv, pnetwork->network.MacAddress);					
				}
				else
				{
					pcur_wlan = find_network(&pmlmepriv->scanned_queue, cur_network->network.MacAddress);
					pcur_wlan->fixed = _FALSE;
					ptarget_wlan = find_network(&pmlmepriv->scanned_queue, pnetwork->network.MacAddress);

					// update network in pscanned_q
					_memcpy(&(ptarget_wlan->network), &pnetwork->network, pnetwork->network.Length );
					ptarget_wlan->fixed = _TRUE;	
					
					pcur_sta = get_stainfo(pstapriv, cur_network->network.MacAddress);
					_enter_critical(&(pstapriv->sta_hash_lock), &irqL2);
					free_stainfo(adapter,  pcur_sta);
					_exit_critical(&(pstapriv->sta_hash_lock), &irqL2);

					ptarget_sta = alloc_stainfo(&adapter->stapriv, pnetwork->network.MacAddress);

				}

			}
			else
			{
				ptarget_wlan = find_network(&pmlmepriv->scanned_queue, pnetwork->network.MacAddress);
				if(ptarget_wlan)
				{
					DEBUG_INFO(("\n BSSID:%x:%x:%x:%x:%x:%x (fw_state=%d)\n", 
						pnetwork->network.MacAddress[0], pnetwork->network.MacAddress[1],
						pnetwork->network.MacAddress[2], pnetwork->network.MacAddress[3],
						pnetwork->network.MacAddress[4], pnetwork->network.MacAddress[5],
						pmlmepriv->fw_state));					
				}

				// update network in pscanned_q
				//_memcpy(&(ptarget_wlan->network), &pnetwork->network, pnetwork->network.Length );
				ptarget_wlan->fixed = _TRUE; //jackson1229
				if(check_fwstate( pmlmepriv, WIFI_STATION_STATE) == _TRUE)
				{ 

										
					ptarget_sta = alloc_stainfo(&adapter->stapriv, pnetwork->network.MacAddress);
					if ((ptarget_wlan == NULL)) {
						DEBUG_ERR(("#########Can't allocate  network when joinbss_event callback\n"));
						goto ignore_joinbss_callback;
					}

					
					if(adapter->securitypriv.dot11AuthAlgrthm== 2)
					{
						ptarget_sta->ieee8021x_blocked = _TRUE;
						ptarget_sta->dot118021XPrivacy =adapter->securitypriv.dot11PrivacyAlgrthm;
						adapter->securitypriv.binstallGrpkey=_FALSE;
						adapter->securitypriv.bgrpkey_handshake=_FALSE;
						DEBUG_ERR(("\nptarget_sta->ieee8021x_blocked = _TRUE  adapter->securitypriv.bgrpkey_handshake=_FALSE;\n"));
					}	
					//DEBUG_ERR(("after alloc_stainfo\n\n"));
				}

			}

//start: initial new architecture : Henry
/*			if(ptarget_sta){
				ptarget_sta->aid  = pnetwork->join_res;
				if(check_fwstate( pmlmepriv, WIFI_STATION_STATE) == _TRUE)
					ptarget_sta->mac_id=5;				
			}*/
//ending: initial new architecture : Henry
			DEBUG_INFO(("ptarget_sta addr: %p\n", ptarget_sta));
			if(ptarget_sta){				
				ptarget_sta->aid  = pnetwork->join_res;				
				ptarget_sta->qos_option = 1; //pstassoc->qos_option 			

				if(check_fwstate( pmlmepriv, WIFI_STATION_STATE) == _TRUE)
					ptarget_sta->mac_id=5;				
			}
			else
			{
				DEBUG_INFO(("ptarget_sta==NULL\n\n"));
			}

			if ((ptarget_wlan == NULL)) {
				DEBUG_ERR(("Can't allocate  network when joinbss_event callback\n"));
				goto ignore_joinbss_callback;
			}

			if(check_fwstate( pmlmepriv, WIFI_STATION_STATE) == _TRUE) {
				if(ptarget_sta == NULL) { //jackson1229
				DEBUG_ERR(("Can't allocate  stainfo when joinbss_event callback\n"));
				printk("Can't allocate  stainfo when joinbss_event callback\n");
				goto ignore_joinbss_callback;
				}
			}

			_memcpy(&cur_network->network, &pnetwork->network,pnetwork->network.Length);
			cur_network->aid = pnetwork->join_res;

			// update fw_state // will clr _FW_UNDER_LINKING here indirectly
			switch(pnetwork->network.InfrastructureMode)
			{
				case Ndis802_11Infrastructure:
					//if(STA_MODE)			
					adapter->mlmepriv.fw_state = WIFI_STATION_STATE;
					break;

				case Ndis802_11IBSS:		
					adapter->mlmepriv.fw_state = WIFI_ADHOC_STATE;
					break;

				default:
					DEBUG_ERR(("Invalid network_mode\n"));
					break;
			}


			DEBUG_INFO(("before indicate connect fw_state:%x",pmlmepriv->fw_state));
			DEBUG_INFO(("\n ==BSSID=0x%02x:0x%2x:0x%2x:0x%2x:0x%2x:0x%2x\n",
				pmlmepriv->cur_network.network.MacAddress[0],pmlmepriv->cur_network.network.MacAddress[1],
				pmlmepriv->cur_network.network.MacAddress[2],pmlmepriv->cur_network.network.MacAddress[3],
				pmlmepriv->cur_network.network.MacAddress[4],pmlmepriv->cur_network.network.MacAddress[5]));
			if(check_fwstate( pmlmepriv, WIFI_STATION_STATE) == _TRUE){
				DEBUG_ERR(("\n joinbss_event_callback:indicate_connect  \n"));
#if 1

				DEBUG_ERR(("\n joinbss_event_callback:adapter->securitypriv.dot11AuthAlgrthm = %d adapter->securitypriv.ndisauthtype=%d\n",
				adapter->securitypriv.dot11AuthAlgrthm,adapter->securitypriv.ndisauthtype));					
				if(adapter->securitypriv.dot11AuthAlgrthm== 2)
				{
					if(ptarget_sta!=NULL){
						u8 null_key[16]={0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};
						ptarget_sta->ieee8021x_blocked = _TRUE;
						ptarget_sta->dot118021XPrivacy =adapter->securitypriv.dot11PrivacyAlgrthm;
						_memcpy(&ptarget_sta->dot11tkiptxmickey.skey[0], null_key, 16);
		//				DbgPrint("\nreset dot11tkiptxmickey\n");
					}
					adapter->securitypriv.binstallGrpkey=_FALSE;
					adapter->securitypriv.bgrpkey_handshake=_FALSE;
			//		DbgPrint("\nadapter->securitypriv.bgrpkey_handshake=_FALSE(2);\n");
	
				}	

#endif
				indicate_connect(adapter);

			}
			// adhoc mode will indicate_connect when stassoc_event_callback
			
		}
		else
		{
			DEBUG_ERR(("joinbss_event_callback err: fw_state:%x",pmlmepriv->fw_state));	
			goto	ignore_joinbss_callback;
		}

		update_protection(adapter, 
		(cur_network->network.IEs) + sizeof (NDIS_802_11_FIXED_IEs), 
		(cur_network->network.IELength));

		// ToDo: Try to add Timer Os_Independent layer...
		DEBUG_INFO(("Cancle assoc_timer \n"));
		_cancel_timer(&pmlmepriv->assoc_timer, &timer_cancelled);
		//NdisMCancelTimer(&pmlmepriv->assoc_timer, &timer_cancelled);

	}
	else
	{
		res = select_and_join_from_scanned_queue(pmlmepriv);	
		DEBUG_ERR(("select_and_join_from_scanned_queue again! res:%d\n",res));	

		if (res != _SUCCESS){
			DEBUG_ERR(("Set Assoc_Timer = 1; can't find match ssid in scanned_q \n"));
			_set_timer(&pmlmepriv->assoc_timer, 1);

			//free_assoc_resources(adapter); //yt:20071018//modify

			//DEBUG_ERR(("^free_assoc_resources, fw_state=%x\n", pmlmepriv->fw_state));

			if((check_fwstate(pmlmepriv, _FW_UNDER_LINKING)) == _TRUE)
			{
			
	                	DEBUG_ERR(("fail! clear _FW_UNDER_LINKING ^^^fw_state=%x\n", pmlmepriv->fw_state));
				pmlmepriv->fw_state ^= _FW_UNDER_LINKING;
			}

		}
		
	}	
				
ignore_joinbss_callback:

	_exit_critical(&pmlmepriv->lock, &irqL);

_func_exit_;	
}

void stassoc_event_callback(_adapter *adapter, u8 *pbuf)
{
	_irqL irqL;
	
	struct sta_info *psta;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	struct stassoc_event *pstassoc	= (struct stassoc_event*)pbuf;

_func_enter_;	
	DEBUG_INFO(("stassoc_event_callback\n"));
	
	// to do: 
	if(access_ctrl(&adapter->acl_list, pstassoc->macaddr) == _FALSE)
		return;

	if((check_fwstate(pmlmepriv, _FW_UNDER_LINKING)) == _TRUE)
		return;

	psta = get_stainfo(&adapter->stapriv, pstassoc->macaddr);
	
	DEBUG_INFO(("stassoc_event_callback\n"));

	if( psta != NULL){ // the sta have been in sta_info_queue => do nothing //YT; 20061108 
		DEBUG_ERR(("Error: stassoc_event_callback: sta has been in sta_hash_queue \n"));
		goto exit; //(between drv has received this event before and  fw have not yet to set key to CAM_ENTRY)//YT; 20061108
	}

	psta = alloc_stainfo(&adapter->stapriv, pstassoc->macaddr);
	
	if (psta == NULL) {
		DEBUG_ERR(("Can't alloc sta_info when stassoc_event_callback\n"));
		goto exit;
	}

	//_memcpy(psta->hwaddr, pstassoc->macaddr, ETH_ALEN);
	
	// to do: init qos_option 
	psta->qos_option = 0; //pstassoc->qos_option 

	//adapter->stapriv.asoc_sta_count++; // move to alloc_stainfo(); protected by pstapriv->hash_lock

	// to do : init sta_info variable

	if(adapter->securitypriv.dot11AuthAlgrthm==2)
		psta->dot118021XPrivacy = adapter->securitypriv.dot11PrivacyAlgrthm;

	psta->ieee8021x_blocked = _FALSE;
	
	_enter_critical(&pmlmepriv->lock, &irqL);

	if ( (check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)==_TRUE ) || 
		(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)==_TRUE ) )
	{
		if(adapter->stapriv.asoc_sta_count== 2) { // a sta + bc/mc_stainfo (no Ibss_stainfo)
			indicate_connect(adapter);
		}
	}

	_exit_critical(&pmlmepriv->lock, &irqL);

	// submit SetStaKey_cmd to tell fw, fw will allocate an CAM entry for this sta
	setstakey_cmd(adapter, (unsigned char*)psta, _FALSE);
exit:
_func_exit_;	

}

void stadel_event_callback(_adapter *adapter, u8 *pbuf)
{
	_irqL irqL,irqL2;
	
	struct sta_info *psta;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	struct 	stadel_event *pstadel	= (struct stadel_event*)pbuf;
   	struct	sta_priv *pstapriv = &adapter->stapriv;
	
_func_enter_;	
	_enter_critical(&pmlmepriv->lock, &irqL2);

	if(pmlmepriv->fw_state& WIFI_STATION_STATE){
		free_assoc_resources(adapter);
		indicate_disconnect(adapter);
	}

	if ( (pmlmepriv->fw_state& WIFI_ADHOC_MASTER_STATE) || 
	      (pmlmepriv->fw_state& WIFI_ADHOC_STATE))	//if(pmlmepriv->fw_state& WIFI_ADHOC_STATE)
	{
		psta = get_stainfo(&adapter->stapriv, pstadel->macaddr);
		_enter_critical(&(pstapriv->sta_hash_lock), &irqL);
		free_stainfo(adapter,  psta);
		_exit_critical(&(pstapriv->sta_hash_lock), &irqL);
		
		if(adapter->stapriv.asoc_sta_count== 1) { // a sta + bc/mc_stainfo (no Ibss_stainfo)
			indicate_disconnect(adapter);
			if((pmlmepriv->fw_state & WIFI_ADHOC_STATE)){
				pmlmepriv->fw_state |= WIFI_ADHOC_MASTER_STATE;
				pmlmepriv->fw_state ^= WIFI_ADHOC_STATE;

			}
		}
	}
	_exit_critical(&pmlmepriv->lock, &irqL2);

	//disconnect_hdl_under_linked((unsigned char*)adapter, psta, _FALSE);
_func_exit_;	
}




void _sitesurvey_ctrl_handler (_adapter *adapter)
{
	struct	mlme_priv *pmlmepriv = &adapter->mlmepriv;
	struct	sitesurvey_ctrl	*psitesurveyctrl=&pmlmepriv->sitesurveyctrl;
	struct	registry_priv	*pregistrypriv=&adapter->registrypriv;
	uint current_tx_pkts;
	uint current_rx_pkts;
_func_enter_;		
	

	current_tx_pkts=(adapter->xmitpriv.tx_pkts)-(psitesurveyctrl->last_tx_pkts);
	current_rx_pkts=(adapter->recvpriv.rx_pkts)-(psitesurveyctrl->last_rx_pkts);

	psitesurveyctrl->last_tx_pkts=adapter->xmitpriv.tx_pkts;
	psitesurveyctrl->last_rx_pkts=adapter->recvpriv.rx_pkts;

	if( (current_tx_pkts>pregistrypriv->busy_thresh)||(current_rx_pkts>pregistrypriv->busy_thresh)) {//if( (current_tx_pkts>traffic_threshold)||(current_rx_pkts>traffic_threshold)) {
		psitesurveyctrl->traffic_busy= _TRUE;
	}
	
	else {
		
		psitesurveyctrl->traffic_busy= _FALSE;
	}
	
_func_exit_;	
}


void _join_timeout_handler (_adapter *adapter)
{
	_irqL irqL;
	struct	mlme_priv *pmlmepriv = &adapter->mlmepriv;

#if 0
	if (adapter->bDriverStopped == _TRUE){
		_up_sema(&pmlmepriv->assoc_terminate);
		return;
	}
#endif	
_func_enter_;		
	DEBUG_ERR(("\n^^^join_timeout_handler ^^^fw_state=%x\n", pmlmepriv->fw_state));
	if(adapter->bDriverStopped ||adapter->bSurpriseRemoved)
		return;
	_enter_critical(&pmlmepriv->lock, &irqL);


	if((check_fwstate(pmlmepriv, _FW_UNDER_LINKING)) == _TRUE)
	{
		
                DEBUG_ERR(("clear _FW_UNDER_LINKING ^^^fw_state=%x\n", pmlmepriv->fw_state));
		pmlmepriv->fw_state ^= _FW_UNDER_LINKING;
                printk("\n join_timeout_handler clear _FW_UNDER_LINKING ^^^fw_state=%x\n", pmlmepriv->fw_state);

	}

	if((check_fwstate(pmlmepriv, _FW_LINKED)) == _TRUE)
	{
		os_indicate_disconnect(adapter);
		pmlmepriv->fw_state ^= _FW_LINKED;
	}

	free_scanqueue(pmlmepriv);
	 
	_exit_critical(&pmlmepriv->lock, &irqL);
	
_func_exit_;	

}


/*
Calling context:
The caller of the sub-routine will be in critical section...

The caller must hold the following spinlock

pmlmepriv->lock


*/
int select_and_join_from_scanned_queue(struct mlme_priv	*pmlmepriv )
{

	_list	*phead;

	unsigned char *dst_ssid, *src_ssid;
	_adapter *adapter;
	
	_queue	*queue	= &(pmlmepriv->scanned_queue);
	struct	wlan_network	*pnetwork = NULL;

	phead = get_list_head(queue);
		
	adapter = (_adapter *)pmlmepriv->nic_hdl;

_func_enter_;	
	while (1) {
			
		if ((end_of_queue_search(phead, pmlmepriv->pscanned)) == _TRUE)
		{
			DEBUG_ERR(("(1)select_and_join_from_scanned_queue return _FAIL\n"));
			return _FAIL;	
		}

		pnetwork = LIST_CONTAINOR(pmlmepriv->pscanned, struct wlan_network, list);
		if(pnetwork==NULL){
			DEBUG_ERR(("(2)select_and_join_from_scanned_queue return _FAIL:(pnetwork==NULL)\n"));
			return _FAIL;	
		}

		
		pmlmepriv->pscanned = get_next(pmlmepriv->pscanned);

		if(pmlmepriv->assoc_by_bssid==_TRUE){
			dst_ssid=pnetwork->network.MacAddress;
			src_ssid=pmlmepriv->assoc_bssid;//pmlmepriv->assoc_bssid;
			if(_memcmp(dst_ssid,src_ssid,ETH_ALEN)==_TRUE){
				if((pmlmepriv->cur_network.network.InfrastructureMode==Ndis802_11AutoUnknown)||
					pmlmepriv->cur_network.network.InfrastructureMode == pnetwork->network.InfrastructureMode)
						goto ask_for_joinbss;
			}
		}
		else{
		if (pmlmepriv->assoc_ssid.SsidLength == 0)
			goto ask_for_joinbss;

		dst_ssid = pnetwork->network.Ssid.Ssid;

		src_ssid = pmlmepriv->assoc_ssid.Ssid;

		if (((_memcmp(dst_ssid, src_ssid, pmlmepriv->assoc_ssid.SsidLength)) == _TRUE)&&
			(pnetwork->network.Ssid.SsidLength==pmlmepriv->assoc_ssid.SsidLength))
		{
			DEBUG_ERR(("dst_ssid=%s, src_ssid=%s \n",dst_ssid, src_ssid));
			if((pmlmepriv->cur_network.network.InfrastructureMode==Ndis802_11AutoUnknown)||
				pmlmepriv->cur_network.network.InfrastructureMode == pnetwork->network.InfrastructureMode)
			{
				_memcpy(pmlmepriv->assoc_bssid,pnetwork->network.MacAddress, ETH_ALEN);
			goto ask_for_joinbss;
			}
		}
 	}
	}
_func_exit_;
return _FAIL;	


ask_for_joinbss:
_func_exit_;
	return joinbss_cmd(adapter, pnetwork);

}




sint set_auth(_adapter * adapter,struct security_priv *psecuritypriv)
{
	struct	cmd_obj* pcmd;
	struct 	setauth_parm *psetauthparm;
	struct	cmd_priv	*pcmdpriv=&(adapter->cmdpriv);
	sint		res=_SUCCESS;
_func_enter_;	
	pcmd = (struct	cmd_obj*)_malloc(sizeof(struct	cmd_obj));
	if(pcmd==NULL){
		res= _FAIL;  //try again
		goto exit;
	}
	psetauthparm=(struct setauth_parm*)_malloc(sizeof(struct setauth_parm));
	if(psetauthparm==NULL){
		_mfree((unsigned char *)pcmd, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	memset(psetauthparm, 0, sizeof(struct setauth_parm));
	psetauthparm->mode=(unsigned char)psecuritypriv->dot11AuthAlgrthm;
	
	pcmd->cmdcode = _SetAuth_CMD_;
	pcmd->parmbuf = (unsigned char *)psetauthparm;   
	pcmd->cmdsz =  (sizeof(struct setauth_parm));  
	pcmd->rsp = NULL;
	pcmd->rspsz = 0;


	_init_listhead(&pcmd->list);

	//_init_sema(&(pcmd->cmd_sem), 0);

	enqueue_cmd(pcmdpriv, pcmd);

	DEBUG_INFO(("### set auth : %d, and engueue the h2ccmd###\n",psetauthparm->mode));
exit:
_func_exit_;
	return _SUCCESS;

}


sint set_key(_adapter * adapter,struct security_priv *psecuritypriv, sint keyid)
{
	u8 keylen;
	struct	cmd_obj* pcmd;
	struct 	setkey_parm *psetkeyparm;
	struct	cmd_priv	*pcmdpriv=&(adapter->cmdpriv);
	sint res=_SUCCESS;
	
_func_enter_;
	
	pcmd = (struct	cmd_obj*)_malloc(sizeof(struct	cmd_obj));
	if(pcmd==NULL){
		res= _FAIL;  //try again
		goto exit;
	}
	psetkeyparm=(struct setkey_parm*)_malloc(sizeof(struct setkey_parm));
	if(psetkeyparm==NULL){
		_mfree((unsigned char *)pcmd, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	if(psecuritypriv->dot11AuthAlgrthm ==2){		
		psetkeyparm->algorithm=(unsigned char)psecuritypriv->dot118021XGrpPrivacy;	
		DEBUG_ERR(("\n set_key: psetkeyparm->algorithm=(unsigned char)psecuritypriv->dot118021XGrpPrivacy=%d \n",psetkeyparm->algorithm));
	}	
	else{
		psetkeyparm->algorithm=(u8)psecuritypriv->dot11PrivacyAlgrthm;
		DEBUG_ERR(("\n set_key: psetkeyparm->algorithm=(u8)psecuritypriv->dot11PrivacyAlgrthm=%d \n",psetkeyparm->algorithm));

	}
	psetkeyparm->keyid=(u8)keyid;
	DEBUG_ERR(("\n set_key: psetkeyparm->algorithm=%d psetkeyparm->keyid=(u8)keyid=%d \n",psetkeyparm->algorithm,keyid));

	switch(psetkeyparm->algorithm){
		
		case _WEP40_:
			keylen=5;
			_memcpy(&(psetkeyparm->key[0]), &(psecuritypriv->dot11DefKey[keyid].skey[0]), keylen);
			break;
		case _WEP104_:
			keylen=13;
			_memcpy(&(psetkeyparm->key[0]), &(psecuritypriv->dot11DefKey[keyid].skey[0]), keylen);
			break;
		case _TKIP_:
			keylen=16;			
			_memcpy(&psetkeyparm->key, &psecuritypriv->dot118021XGrpKey[keyid], keylen);
			psetkeyparm->grpkey=1;
			break;
		case _AES_:
			keylen=16;			
			_memcpy(&psetkeyparm->key, &psecuritypriv->dot118021XGrpKey[keyid], keylen);
			psetkeyparm->grpkey=1;
			break;
		default:
			DEBUG_ERR(("\n set_key:psecuritypriv->dot11PrivacyAlgrthm = %x (must be 1 or 2 or 4 or 5)\n",psecuritypriv->dot11PrivacyAlgrthm));
			res= _FAIL;
			goto exit;
	}
	


	
	pcmd->cmdcode = _SetKey_CMD_;
	pcmd->parmbuf = (u8 *)psetkeyparm;   
	pcmd->cmdsz =  (sizeof(struct setkey_parm));  
	pcmd->rsp = NULL;
	pcmd->rspsz = 0;


	_init_listhead(&pcmd->list);

	//_init_sema(&(pcmd->cmd_sem), 0);

	enqueue_cmd(pcmdpriv, pcmd);

exit:
_func_exit_;
	return _SUCCESS;

}




//adjust IEs for joinbss_cmd in WMM
int restruct_wmm_ie(_adapter *adapter, u8 *in_ie, u8 *out_ie, uint in_len, uint initial_out_len)
{
	unsigned	int ielength=0;
	unsigned int i, j;

	i = 12; //after the fixed IE
	while(i<in_len)
	{
		ielength = initial_out_len;		
		
		if(in_ie[i] == 0xDD && in_ie[i+2] == 0x00 && in_ie[i+3] == 0x50  && in_ie[i+4] == 0xF2 && in_ie[i+5] == 0x02 && i+5 < in_len) //WMM element ID and OUI
		{

			//Append WMM IE to the last index of out_ie

			for(j=i; j< i+(in_ie[i+1]+2); j++)
			{
				out_ie[ielength] = in_ie[j];				
				ielength++;
			}
			out_ie[initial_out_len+8] = 0x00; //force the QoS Info Field to be zero
	
			break;
		}

		i+=(in_ie[i+1]+2); // to the next IE element
	}
	

	return ielength;
}


//
// Ported from 8185: IsInPreAuthKeyList(). (Renamed from SecIsInPreAuthKeyList(), 2006-10-13.)
// Added by Annie, 2006-05-07.
//
// Search by BSSID,
// Return Value:
//		-1 		:if there is no pre-auth key in the  table
//		>=0		:if there is pre-auth key, and   return the entry id
//
int SecIsInPMKIDList(_adapter *Adapter, u8 *bssid)
{

	//john
//	struct mlme_priv			*pmlmepriv = &Adapter->mlmepriv;
	struct security_priv *psecuritypriv=&Adapter->securitypriv;

	u32			i=0;

	do
	{
		if( /*psecuritypriv->PMKIDList[i].bUsed &&*/
			_memcmp(psecuritypriv->PMKIDList[i].Bssid, bssid, ETH_ALEN)==_TRUE)
		{
			break;
		}
		else
		{	
			i++;
			//continue;
		}
	}while(i<NUM_PMKID_CACHE);

	if( i == NUM_PMKID_CACHE )
	{ // Could not find.
		i = -1;
	}
	else
	{ // There is one Pre-Authentication Key for the specific BSSID.
	}

	return i;
}


sint restruct_sec_ie(_adapter *adapter,u8 *in_ie,u8 *out_ie,uint in_len)
{
	u8 authmode = 0 ,securitytype,cnt,match,remove_cnt;
	u8 sec_ie[255],uncst_oui[4],bkup_ie[255];
	u8 wpa_oui[4]={0x0,0x50,0xf2,0x01};
	uint 	ielength;
	int iEntry;
	struct	security_priv	*psecuritypriv=&adapter->securitypriv;
	uint	ndissecuritytype=psecuritypriv->ndisencryptstatus;
	uint 	ndisauthmode=psecuritypriv->ndisauthtype;
	struct mlme_priv	*pmlmepriv = &adapter->mlmepriv;
	
_func_enter_;
	if((ndisauthmode==Ndis802_11AuthModeWPA)||(ndisauthmode==Ndis802_11AuthModeWPAPSK))
	{
		authmode=_WPA_IE_ID_;
		uncst_oui[0]=0x0;
		uncst_oui[1]=0x50;
		uncst_oui[2]=0xf2;
	}
	if((ndisauthmode==Ndis802_11AuthModeWPA2)||(ndisauthmode==Ndis802_11AuthModeWPA2PSK))
	{	
		authmode=_WPA2_IE_ID_;
		uncst_oui[0]=0x0;
		uncst_oui[1]=0x0f;
		uncst_oui[2]=0xac;
	}
	switch(ndissecuritytype){
	case Ndis802_11Encryption1Enabled:
	case Ndis802_11Encryption1KeyAbsent:
		securitytype=_WEP40_;
		uncst_oui[3]=0x1;
		break;
	case Ndis802_11Encryption2Enabled:
	case Ndis802_11Encryption2KeyAbsent:
		securitytype=_TKIP_;
		uncst_oui[3]=0x2;
		break;
	case Ndis802_11Encryption3Enabled:
	case Ndis802_11Encryption3KeyAbsent: 	
		securitytype=_AES_;
			uncst_oui[3]=0x4;
		break;
	default:
		securitytype=_NO_PRIVACY_;
		break;
	}
	//4 	Search required WPA or WPA2 IE and copy to sec_ie[ ]
	cnt=12;
	match=_FALSE;
	while(cnt<in_len){
		if(in_ie[cnt]==authmode){
			if((authmode==_WPA_IE_ID_)&&(_memcmp(&in_ie[cnt+2], &wpa_oui[0],4)==_TRUE)){
				_memcpy(&sec_ie[0],&in_ie[cnt],in_ie[cnt+1]+2);
				match=_TRUE;
				DEBUG_ERR(("\n Get WPA IE from %d in_len=%d \n",cnt,in_len));
				break;
			}
			if(authmode==_WPA2_IE_ID_){
				_memcpy(&sec_ie[0],&in_ie[cnt],in_ie[cnt+1]+2);
				match=_TRUE;
				DEBUG_ERR(("\n Get WPA2 IE from %d in_len=%d \n",cnt,in_len));
				break;
			}	
			if(((authmode==_WPA_IE_ID_)&&(_memcmp(&in_ie[cnt+2], &wpa_oui[0],4)==_TRUE))||(authmode==_WPA2_IE_ID_)){
				_memcpy(&bkup_ie[0], &in_ie[cnt],in_ie[cnt+1]+2);
				DEBUG_ERR(("\n cnt =%d in_len=%d \n",cnt,in_len));
			}
			cnt+=in_ie[cnt+1]+2;  //get next
		}
		else
			cnt+=in_ie[cnt+1]+2;   //get next
	}
	//restruct WPA IE or WPA2 IE in sec_ie[ ]
	if(match==_TRUE){
		if(sec_ie[0]==_WPA_IE_ID_){
		// parsing SSN IE to select required encryption algorithm, and set the bc/mc encryption algorithm
			while(_TRUE){
				if(_memcmp(&sec_ie[2],&wpa_oui[0],4) !=_TRUE){   //wpa_tag
					DEBUG_ERR(("\n SSN IE but doesn't has wpa_oui tag! \n"));
			match=_FALSE;
					break;
				}
				if((sec_ie[6]!=0x01)||(sec_ie[7]!= 0x0)){ //IE Ver error
					DEBUG_ERR(("\n SSN IE :IE version error (%.2x %.2x != 01 00 )! \n",sec_ie[6],sec_ie[7]));
			match=_FALSE;
					break;
				}
				if(_memcmp(&sec_ie[8],&wpa_oui[0],3) ==_TRUE){ //get bc/mc encryption type
					switch(sec_ie[11]){
					case 0x0: //none
						psecuritypriv->dot118021XGrpPrivacy=_NO_PRIVACY_;
						break;
					case 0x1: //WEP_40
						psecuritypriv->dot118021XGrpPrivacy=_WEP40_;
						break;
					case 0x2: //TKIP
						psecuritypriv->dot118021XGrpPrivacy=_TKIP_;
						break;
					case 0x3: //AESCCMP
					case 0x4: 
						psecuritypriv->dot118021XGrpPrivacy=_AES_;
						break;
					case 0x5: //WEP_104	
						psecuritypriv->dot118021XGrpPrivacy=_WEP104_;
						break;
			}
			}
			else{
					DEBUG_ERR(("\n SSN IE :Multicast error (%.2x %.2x %.2x %.2x != 00 50 F2 xx )! \n",
						sec_ie[8],sec_ie[9],sec_ie[10],sec_ie[11]));
					match =_FALSE;
					break;
			}
				if(sec_ie[12]==0x01){ //check the unicast encryption type
					if(_memcmp(&sec_ie[14],&uncst_oui[0],4) !=_TRUE){
						DEBUG_ERR(("\n SSN IE :Unicast error (%.2x %.2x %.2x %.2x != 00 50 F2 %.2x )! \n",
							sec_ie[14],sec_ie[15],sec_ie[16],sec_ie[17],uncst_oui[3]));
						match =_FALSE;
						break;
					} //else the uncst_oui is match
			}
				else{ //select the uncst_oui and remove the other uncst_oui
					cnt=sec_ie[12];
					remove_cnt=(cnt-1)*4;
					sec_ie[12]=0x01;
					_memcpy(&sec_ie[14],&uncst_oui[0],4);
					//remove the other unicast suit
					_memcpy(&sec_ie[18],&sec_ie[18+remove_cnt],(sec_ie[1]-18+2-remove_cnt));
					sec_ie[1]=sec_ie[1]-remove_cnt;
				}
				break;
		}
	}

	if(authmode==_WPA2_IE_ID_){
		// parsing RSN IE to select required encryption algorithm, and set the bc/mc encryption algorithm
			while(_TRUE){
				if((sec_ie[2]!=0x01)||(sec_ie[3]!= 0x0)){ //IE Ver error
					DEBUG_ERR(("\n RSN IE :IE version error (%.2x %.2x != 01 00 )! \n",sec_ie[2],sec_ie[3]));
			match=_FALSE;
					break;
				}
				if(_memcmp(&sec_ie[4],&uncst_oui[0],3) ==_TRUE){ //get bc/mc encryption type
					switch(sec_ie[7]){

					case 0x1: //WEP_40
						psecuritypriv->dot118021XGrpPrivacy=_WEP40_;
						break;
					case 0x2: //TKIP
						psecuritypriv->dot118021XGrpPrivacy=_TKIP_;
						break;
					case 0x4: //AESWRAP
						psecuritypriv->dot118021XGrpPrivacy=_AES_;
						break;
					case 0x5: //WEP_104	
						psecuritypriv->dot118021XGrpPrivacy=_WEP104_;
						break;
					default: //none
						psecuritypriv->dot118021XGrpPrivacy=_NO_PRIVACY_;
						break;	
			}
			}
			else{
					DEBUG_ERR(("\n RSN IE :Multicast error (%.2x %.2x %.2x %.2x != 00 50 F2 xx )! \n",
						sec_ie[4],sec_ie[5],sec_ie[6],sec_ie[7]));
			match=_FALSE;
					break;
				}
				if(sec_ie[8]==0x01){ //check the unicast encryption type
					if(_memcmp(&sec_ie[10],&uncst_oui[0],4) !=_TRUE){
						DEBUG_ERR(("\n SSN IE :Unicast error (%.2x %.2x %.2x %.2x != 00 50 F2 xx )! \n",
							sec_ie[10],sec_ie[11],sec_ie[12],sec_ie[13]));
						match =_FALSE;
						break;
					} //else the uncst_oui is match
			}
				else{ //select the uncst_oui and remove the other uncst_oui
					cnt=sec_ie[8];
					remove_cnt=(cnt-1)*4;
					sec_ie[8]=0x01;
				_memcpy( &sec_ie[10] , &uncst_oui[0],4 );				
					//remove the other unicast suit
					_memcpy(&sec_ie[14],&sec_ie[14+remove_cnt],(sec_ie[1]-14+2-remove_cnt));
					sec_ie[1]=sec_ie[1]-remove_cnt;
			}
				break;
			}
		}

	}

	
	if((authmode==_WPA_IE_ID_)||(authmode==_WPA2_IE_ID_)){
		//copy fixed ie 
		_memcpy(out_ie ,in_ie,12);
		ielength=12;
		//copy RSN or SSN		
		if(match ==_TRUE){		
			_memcpy(&out_ie[ielength],&sec_ie[0],sec_ie[1]+2);
		ielength+=sec_ie[1]+2;
			if(authmode==_WPA2_IE_ID_){
				//the Pre-Authentication bit should be zero, john
				out_ie[ielength-1]= 0;
				out_ie[ielength-2]= 0;
			}
		}
	}
	else{
		_memcpy(&out_ie[0],&in_ie[0],in_len);
		ielength=in_len;

	}

	iEntry = SecIsInPMKIDList(adapter, pmlmepriv->assoc_bssid);
	if(iEntry<0) {
		return ielength;
		}

	else{
		if(authmode == _WPA2_IE_ID_){
			out_ie[ielength]=1;
			ielength++;
			out_ie[ielength]=0;	//PMKID count = 0x0100
			ielength++;
			_memcpy(	&out_ie[ielength], &psecuritypriv->PMKIDList[iEntry].PMKID, 16);
		
		ielength+=16;
		out_ie[13]+=18;//PMKID length = 2+16

		}
	}
	report_sec_ie(adapter, authmode, sec_ie);

_func_exit_;
	return ielength;
}




void init_registrypriv_dev_network(	_adapter* adapter)
{
	struct registry_priv* pregistrypriv = &adapter->registrypriv;
	struct eeprom_priv* peepriv = &adapter->eeprompriv;
	NDIS_WLAN_BSSID_EX    *pdev_network = &pregistrypriv->dev_network;
	u8 *myhwaddr = myid(peepriv);
_func_enter_;
	_memcpy(pdev_network->MacAddress, myhwaddr, ETH_ALEN);

	_memcpy(&pdev_network->Ssid, &pregistrypriv->ssid, sizeof(NDIS_802_11_SSID));

	
	pdev_network->Configuration.Length=sizeof(NDIS_802_11_CONFIGURATION);
	pdev_network->Configuration.BeaconPeriod = 100;
	//pdev_network->Configuration.FHConfig.Length = 0;
	pdev_network->Configuration.FHConfig.Length = 0;
	pdev_network->Configuration.FHConfig.HopPattern = 0;
	pdev_network->Configuration.FHConfig.HopSet = 0;
	pdev_network->Configuration.FHConfig.DwellTime = 0;
_func_exit_;	
	
}


void update_registrypriv_dev_network(_adapter* adapter) 
{
	struct registry_priv* pregistrypriv = &adapter->registrypriv;
	
	NDIS_WLAN_BSSID_EX    *pdev_network = &pregistrypriv->dev_network;

	struct	security_priv*	psecuritypriv = &adapter->securitypriv;

	struct	wlan_network	*cur_network = &adapter->mlmepriv.cur_network;

	struct	xmit_priv	*pxmitpriv = &adapter->xmitpriv;

_func_enter_;

	pxmitpriv->vcs_setting = pregistrypriv->vrtl_carrier_sense;
	pxmitpriv->vcs = pregistrypriv->vcs_type;
	pxmitpriv->vcs_type = pregistrypriv->vcs_type;
	pxmitpriv->rts_thresh = pregistrypriv->rts_thresh;

	pxmitpriv->frag_len = pregistrypriv->frag_thresh;

	adapter->qospriv.qos_option = pregistrypriv->wmm_enable;

	pdev_network->Privacy = psecuritypriv->dot11PrivacyAlgrthm > 0 ? 1 : 0 ; // adhoc no 802.1x

	pdev_network->Rssi = 0;

	switch(pregistrypriv->wireless_mode)
	{
		case WIRELESS_11B:
			pdev_network->NetworkTypeInUse = Ndis802_11DS;
			break;	
		case WIRELESS_11G:
		case WIRELESS_11BG:
			pdev_network->NetworkTypeInUse = Ndis802_11OFDM24;
			break;
		case WIRELESS_11A:
			pdev_network->NetworkTypeInUse = Ndis802_11OFDM5;
			break;
		default :
			// TODO
			break;
	}

	
	pdev_network->Configuration.DSConfig = pregistrypriv->channel;
	DEBUG_ERR(("\n update_registrypriv_dev_network:(pregistrypriv->channel=%d)pdev_network->Configuration.DSConfig=0x%8x\n",pregistrypriv->channel,pdev_network->Configuration.DSConfig));
	pdev_network->InfrastructureMode = cur_network->network.InfrastructureMode ;

	if(cur_network->network.InfrastructureMode == Ndis802_11IBSS)
			pdev_network->Configuration.ATIMWindow = 3;


	// 1. Supported rates
	// 2. IE

	//set_supported_rate(pdev_network->SupportedRates, pregistrypriv->wireless_mode) ; // will be called in generate_ie
	generate_ie(pregistrypriv);

	pdev_network->Length = get_NDIS_WLAN_BSSID_EX_sz(pdev_network);
_func_exit_;	
}

void get_encrypt_decrypt_from_registrypriv(	_adapter* adapter)
{
#if 0
	u16	wpaconfig=0;
	
	struct registry_priv* pregistrypriv = &adapter->registrypriv;
	struct security_priv* psecuritypriv= &adapter->securitypriv;
_func_enter_;
	psecuritypriv->sw_encrypt=pregistrypriv->software_encrypt;
	psecuritypriv->sw_decrypt=pregistrypriv->software_decrypt;
	psecuritypriv->binstallGrpkey=_FAIL;
	//read wpa_config register value
	 wpaconfig= read16(adapter,SECR);	
	
	if(psecuritypriv->sw_encrypt)
		wpaconfig=wpaconfig & 0xfd;
	if(psecuritypriv->sw_decrypt)
		wpaconfig=wpaconfig & 0xfb;

	//write back to WPA_CONFIG
	write16(adapter, WPA_CONFIG, wpaconfig);
_func_exit_;	
#endif
}



