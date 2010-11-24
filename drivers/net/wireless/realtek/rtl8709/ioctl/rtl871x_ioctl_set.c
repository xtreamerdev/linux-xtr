/******************************************************************************
* rtl871x_ioctl_set.c                                                                                                                                 *
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
#define _RTL871X_IOCTL_SET_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <wlan_bssdef.h>
#include <rtl8711_spec.h>
#include <rtl871x_ioctl_set.h>
#include <hal_init.h>
#ifdef CONFIG_USB_HCI
#include <usb_osintf.h>
#include <usb_ops.h>
#endif

#define IS_MAC_ADDRESS_BROADCAST(addr) \
( \
	( (addr[0] == 0xff) && (addr[1] == 0xff) && \
		(addr[2] == 0xff) && (addr[3] == 0xff) && \
		(addr[4] == 0xff) && (addr[5] == 0xff) )  ? _TRUE : _FALSE \
)

u8 validate_ssid(NDIS_802_11_SSID *ssid)
{

	u8	 i;
	u8	ret=_TRUE;
_func_enter_;
	if(ssid->SsidLength > 32)
	{
		DEBUG_ERR(("ssid length >32"));
		ret= _FALSE;
		goto exit;
	}


	for(i = 0; i < ssid->SsidLength; i++)
	{
		//wifi, printable ascii code must be supported
		if(!( (ssid->Ssid[i] >= 0x20) && (ssid->Ssid[i] <= 0x7e) )){
			DEBUG_ERR(("ssid has nonprintabl ascii "));
			ret= _FALSE;
			goto exit;
			}
	}
exit:	
_func_exit_;
	return ret;
}



// YT; 20061109
u8 do_join(_adapter * padapter)
{
	_list	*plist, *phead;
	u8* pibss = NULL;
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	_queue	*queue	= &(pmlmepriv->scanned_queue);
	u8 ret=_TRUE;
	phead = get_list_head(queue);
	plist = get_next(phead);

_func_enter_;
	DEBUG_INFO(("\n do_join: phead = %p; plist = %p \n\n\n", phead, plist));

	pmlmepriv->cur_network.join_res = -2;
		
	pmlmepriv->fw_state |= _FW_UNDER_LINKING;

	pmlmepriv->pscanned = plist;

	pmlmepriv->to_join = _TRUE;

	if(_queue_empty(queue)== _TRUE)
	{
		// submit site_survey_cmd
		if(pmlmepriv->fw_state & _FW_UNDER_LINKING)
	               pmlmepriv->fw_state ^= _FW_UNDER_LINKING;
            		
		if(pmlmepriv->sitesurveyctrl.traffic_busy==_FALSE)
			sitesurvey_cmd(padapter, &pmlmepriv->assoc_ssid);

		DEBUG_INFO(("\n *** site survey***\n."));
		ret=_FALSE;
		goto exit;
	}
	
	else 	
	{	
		if(select_and_join_from_scanned_queue(pmlmepriv)==_SUCCESS){
			DEBUG_INFO(("\n do_join ***set_timer: assoc_timer***\n."));
		     _set_timer(&pmlmepriv->assoc_timer, MAX_JOIN_TIMEOUT );
		}	 
		else	
		{

			if(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)==_TRUE)
			{
				// submit createbss_cmd
				//if(adapter->registrypriv.dev_network.InfrastructureMode != Ndis802_11IBSS){
				//	update_registrypriv_dev_network(adapter);
				//	RT_TRACE(COMP_SITE_SURVEY, DBG_LOUD,("***Error=> MgntActSet_802_11_SSID: WIFI_ADHOC_STATE but dev_network.InfrastructureMode != Ndis802_11IBSS*** \n "));
				//}

 				//pmlmepriv->lock has been acquired by caller...
				NDIS_WLAN_BSSID_EX    *pdev_network = &(padapter->registrypriv.dev_network);

				pmlmepriv->fw_state = WIFI_ADHOC_MASTER_STATE;
				
				pibss = padapter->registrypriv.dev_network.MacAddress;


				//init_registrypriv_dev_network(adapter);
				memset(&pdev_network->Ssid, 0, sizeof(NDIS_802_11_SSID));
				_memcpy(&pdev_network->Ssid, &pmlmepriv->assoc_ssid, sizeof(NDIS_802_11_SSID));
	
				update_registrypriv_dev_network(padapter);

				generate_random_ibss(pibss);
					
				if(createbss_cmd(padapter)!=_SUCCESS){
					DEBUG_ERR(("***Error=>do_goin: createbss_cmd status FAIL*** \n "));						
					return _FALSE;
				}

				DEBUG_INFO(("***Error=> select_and_join_from_scanned_queue FAIL under STA_Mode*** \n "));						

			}
			
			else
			{ // can't associate ; reset under-linking
			
				if(pmlmepriv->fw_state & _FW_UNDER_LINKING)
				    pmlmepriv->fw_state ^= _FW_UNDER_LINKING;

				if((check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE))
				{
					if(_memcmp(pmlmepriv->cur_network.network.Ssid.Ssid, pmlmepriv->assoc_ssid.Ssid, pmlmepriv->assoc_ssid.SsidLength))
					{ // for funk to do roaming
						DEBUG_ERR(("for funk to do roaming"));
						if(pmlmepriv->sitesurveyctrl.traffic_busy==_FALSE)
							sitesurvey_cmd(padapter, &pmlmepriv->assoc_ssid);
					}
				}				
			}

		}

	}
exit:
_func_exit_;	
	return ret;
}
#ifdef PLATFORM_WINDOWS
#ifdef CONFIG_USB_HCI
VOID
WaitWakeComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
	_adapter *padapter=(_adapter *)Context;

	DEBUG_ERR(("===> WaitWakeComplete()----------------------------\n"));

	DEBUG_ERR(("MinorFunction=%#X\n", MinorFunction));
	DEBUG_ERR(("Dev State=%#X, System State=%#X\n", PowerState.DeviceState, PowerState.SystemState));
	DEBUG_ERR(("IoStatus: Status=%#X, Information:%#X\n", IoStatus->Status, IoStatus->Information));

	_spinlock(&(padapter->pwrctrlpriv.pnp_pwr_mgnt_lock));
	padapter->pwrctrlpriv.pnp_wwirp_pending =_FALSE;
	padapter->pwrctrlpriv.pnp_wwirp =NULL;
	_spinunlock(&(padapter->pwrctrlpriv.pnp_pwr_mgnt_lock));

	NdisSetEvent( &(padapter->pwrctrlpriv.pnp_wwirp_complete_evt));	

	DEBUG_ERR(("<=== WaitWakeComplete()----------------------------\n"));
}

VOID
SendWaitWakeIrp(
	_adapter * padapter)
{
	POWER_STATE PowerState;  	
	NTSTATUS	Status;

	DEBUG_ERR(("===> SendWaitWakeIrp()\n"));
	_spinlock(&(padapter->pwrctrlpriv.pnp_pwr_mgnt_lock));
	
	if(padapter->pwrctrlpriv.pnp_wwirp_pending)
	{
		_spinunlock(&(padapter->pwrctrlpriv.pnp_pwr_mgnt_lock));
		DEBUG_ERR(("<=== SendWaitWakeIrp(): already submit on WW IRP down!\n"));
		return;
	}
	else
	{
		padapter->pwrctrlpriv.pnp_wwirp_pending=_TRUE;
		NdisResetEvent( &(padapter->pwrctrlpriv.pnp_wwirp_complete_evt));
		_spinunlock(&(padapter->pwrctrlpriv.pnp_pwr_mgnt_lock));
	}

	PowerState.SystemState = PowerSystemSleeping3;

	Status = PoRequestPowerIrp(
				padapter->dvobjpriv.pphysdevobj,//pDevice->pPhysDevObj,
				IRP_MN_WAIT_WAKE,
				PowerState, 
				(PREQUEST_POWER_COMPLETE)WaitWakeComplete, 
				padapter,//(PVOID)pDevice,
				&(padapter->pwrctrlpriv.pnp_wwirp));
 
	if(Status == STATUS_PENDING) 
	{ 
		DEBUG_ERR(("SendWaitWakeIrp(): STATUS_PENDING\n"));
	}
	else
	{
		DEBUG_ERR(("SendWaitWakeIrp(): Status(%#X)\n", Status));
	}

	DEBUG_ERR( ("<=== SendWaitWakeIrp()\n"));
}

#endif
#endif

extern void pnp_hw_init_thread(void *context)

{
	_adapter * padapter = (_adapter *)context;
	uint res;
_func_enter_;
	
#ifdef PLATFORM_LINUX
	daemonize("%s", padapter->pnetdev->name);
	allow_signal(SIGTERM);
#endif	
//	DbgPrint(" ==>pnp_hw_init_thread");
	padapter->pwrctrlpriv.pnp_bstop_trx=_FALSE;
//	DbgPrint(" pnp_hw_init_thread:padapter->pwrctrlpriv.pnp_bstop_trx=%d",padapter->pwrctrlpriv.pnp_bstop_trx);
	// USB: set up Bulk IN/OUT transfer.
	_spinlock(&(padapter->pwrctrlpriv.pnp_pwr_mgnt_lock));
	if(!padapter->pwrctrlpriv.pnp_wwirp_pending)
	{
		_spinunlock(&(padapter->pwrctrlpriv.pnp_pwr_mgnt_lock));
#ifdef PLATFORM_WINDOWS
		SendWaitWakeIrp(padapter);
#endif
	}
	else
	{
		_spinunlock(&(padapter->pwrctrlpriv.pnp_pwr_mgnt_lock));
	}

#if defined(CONFIG_RTL8711)
//	DbgPrint("pnp_hw_init_thread: before rtl8711_hal_init");	
	res=rtl8711_hal_init(padapter);
//	DbgPrint("pnp_hw_init_thread:rtl8711_hal_init=%d",res);
#elif defined(CONFIG_RTL8192U)
	res=rtl8192u_hal_init(padapter);
#elif defined(CONFIG_RTL8187)
	res=rtl8187_hal_init(padapter);
#endif

	if(res ==_FAIL){
		DEBUG_ERR(("\n========== pnp_hw_init_thread:init 8711 fail!!!!!!!!!!\n"));
	}
		
	if(padapter->dvobjpriv.rxirp_init !=NULL){	
		padapter->dvobjpriv.rxirp_init(padapter);
		DEBUG_ERR(("\n pnp_hw_init_thread:rx irp init!!\n"));
	}
	

	padapter->hw_init_completed=_TRUE;
		
		
#ifdef PLATFORM_LINUX
		if (signal_pending (current)) {
			flush_signals(current);
        	}
#endif       			
#ifdef 	 PLATFORM_WINDOWS
	NdisMSetInformationComplete(padapter->hndis_adapter, NDIS_STATUS_SUCCESS);
#endif				
	
_func_exit_;	

#ifdef PLATFORM_LINUX

	complete_and_exit (NULL, 0);

#endif	

#ifdef PLATFORM_OS_XP

	PsTerminateSystemThread(STATUS_SUCCESS);

#endif

}


u8 pnp_set_power_wakeup(_adapter* padapter){
#ifdef PLATFORM_WINDOWS
	NTSTATUS status;
	HANDLE hThread;
	OBJECT_ATTRIBUTES oa;
#endif
	u8 res=_SUCCESS;
_func_enter_;
	DEBUG_ERR(("\n==>pnp_set_power_wakeup!!!\n"));
	// HW: Configure device to active state.
		//4 pnp_hw_init_thread
#ifdef PLATFORM_OS_XP
	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &oa, NULL, NULL, (PKSTART_ROUTINE)pnp_hw_init_thread,padapter);
	
	if (!NT_SUCCESS(status))
	{
		DEBUG_ERR(("Create thread fail: cmd_thread .\n"));
		res=_FAIL;
		goto exit;
	}
#endif


#ifdef PLATFORM_OS_XP
exit:	
#endif
_func_exit_;
	DEBUG_ERR(("\n<==pnp_set_power_wakeup!!!\n"));

	return res;
}


extern void pnp_sleep_thread(void *context)

{
	_adapter * padapter = (_adapter *)context;
	_irqL irqL;
//	u8 tmp8,res=_SUCCESS;
	struct mlme_priv *pmlmepriv =&padapter->mlmepriv;
	struct pwrctrl_priv *pwrctrl=&padapter->pwrctrlpriv;
_func_enter_;
	
#ifdef PLATFORM_LINUX
	daemonize("%s", padapter->pnetdev->name);
	allow_signal(SIGTERM);
#endif	


	//disassociated (not to transfer)
	_enter_critical(&pmlmepriv->lock, &irqL );
	if(check_fwstate(pmlmepriv, _FW_LINKED)== _TRUE) 
	{
		indicate_disconnect(padapter); //will clr Linked_state; before this function, we must have chked whether  issue dis-assoc_cmd or not		
	}
	_exit_critical(&pmlmepriv->lock, &irqL );

	DEBUG_ERR(("\n  pnp_sleep_thread:fwstate:%d\n",pmlmepriv->fw_state));


#ifdef CONFIG_USB_HCI

//	setusbstandby_cmd(padapter);	
//	_down_sema(&padapter->usb_suspend_sema);
	pwrctrl->pnp_bstop_trx=_TRUE;
	//delay 1 second  then cancel IRP
	msleep_os(1000);

		//cancel irp
	//  1.rx irp
	//  2.int irp
	//  3.reg read/write irp
	if(padapter->dvobjpriv.rxirp_deinit !=NULL){	
		padapter->dvobjpriv.rxirp_deinit(padapter);
		DEBUG_ERR(("\n pnp_set_power_sleep:rx irp deinit!!\n"));
	}
#if 0
	//4	5.Turn off LDO
	usbvendorrequest(&padapter->dvobjpriv, RT_USB_GET_REGISTER, RT_USB_LDO, 0, &tmp8, 1, TRUE);
	DEBUG_ERR(("usb_hal_bus_deinit() : LDO = %d\n", tmp8));
	if (tmp8 == 0x00){
		DEBUG_ERR(("usb_hal_bus_deinit() : LDO is off. No necessary to Turn it off.\n"));

	} else {
		DEBUG_ERR(("usb_hal_bus_deinit() : LDO is on. Need to Turn it off.\n"));
		usbvendorrequest(&padapter->dvobjpriv, RT_USB_SET_REGISTER, RT_USB_LDO, RT_USB_LDO_OFF,  NULL, 0, FALSE);
		NdisMSleep(100);
		usbvendorrequest(&padapter->dvobjpriv, RT_USB_GET_REGISTER, RT_USB_LDO, 0, &tmp8, 1, TRUE);
		DEBUG_ERR(("usb_hal_bus_deinit() : LDO = %d\n", tmp8));

	}
#endif


	
#endif

#ifdef 	 PLATFORM_WINDOWS
	NdisMSetInformationComplete(padapter->hndis_adapter, NDIS_STATUS_SUCCESS);
#endif		
	padapter->hw_init_completed=_FALSE;
_func_exit_;	

#ifdef PLATFORM_OS_XP

	PsTerminateSystemThread(STATUS_SUCCESS);

#endif

}

u8 pnp_set_power_sleep(_adapter* padapter){
	
//	_irqL irqL;
#ifdef PLATFORM_OS_XP
	NTSTATUS status;
	HANDLE hThread;
	OBJECT_ATTRIBUTES oa;
#endif
	u8 res=_SUCCESS;
//	struct mlme_priv *pmlmepriv =&padapter->mlmepriv;
//	struct pwrctrl_priv *pwrctrl=&padapter->pwrctrlpriv;
_func_enter_;
	DEBUG_ERR(("\n==>pnp_set_power_sleep!!!\n"));


#ifdef PLATFORM_OS_XP
	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &oa, NULL, NULL, (PKSTART_ROUTINE)pnp_sleep_thread,padapter);
	
	if (!NT_SUCCESS(status))
	{
		DEBUG_ERR(("Create thread fail: cmd_thread .\n"));
		res=_FAIL;
	}
#endif

_func_exit_;
	DEBUG_ERR(("\n<==pnp_set_power_sleep!!!\n"));

	return res;
}

u8 set_802_11_bssid(_adapter* padapter, u8 *bssid){

	_list	*plist, *phead;
	_irqL irqL;
	
	u8 status=_TRUE;

	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);

	_queue	*queue	= &(pmlmepriv->scanned_queue);

//	struct sta_info *psta = NULL;
	
_func_enter_;
	
	_enter_critical(&pmlmepriv->lock, &irqL);


	phead = get_list_head(queue);
	plist = get_next(phead);
	DEBUG_INFO(("\n set_802_11_bssid: bssid= 0x%.2x  0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n", bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5]));



 	if   ( (check_fwstate(pmlmepriv, _FW_UNDER_SURVEY) == _TRUE) ||
		 (check_fwstate(pmlmepriv, _FW_UNDER_LINKING) == _TRUE))		
	{
		DEBUG_ERR(("\n\n Set BSSID is not allowed under surveying || adhoc master || under linking\n\n."));
		goto _Abort_Set_BSSID;
	}

	if  ( (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) )	
	{
		DEBUG_INFO(("\n _FW_LINKED ||WIFI_ADHOC_MASTER_STATE \n."));

		if((_memcmp(&pmlmepriv->cur_network.network.MacAddress, bssid, 6))==_TRUE) //if((_memcmp(&pmlmepriv->assoc_bssid.Ssid, ssid->Ssid, ssid->SsidLength)) 

		{		
			if((check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _FALSE))
				goto _Abort_Set_BSSID;
		}
		else
		{
			DEBUG_INFO(("\n Set BSSID not the same ssid \n."));
			DEBUG_INFO(("\n set_bssid = 0x%.2x  0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n", bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5]));
			DEBUG_INFO(("\n cur_bssid = 0x%.2x  0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n", 
				pmlmepriv->cur_network.network.MacAddress[0],pmlmepriv->cur_network.network.MacAddress[1],pmlmepriv->cur_network.network.MacAddress[2],
				pmlmepriv->cur_network.network.MacAddress[3],pmlmepriv->cur_network.network.MacAddress[4],pmlmepriv->cur_network.network.MacAddress[5]));


			if((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))
			{
				pmlmepriv->fw_state ^=WIFI_ADHOC_MASTER_STATE;	
				pmlmepriv->fw_state |=WIFI_ADHOC_STATE;
			}

			disassoc_cmd(padapter);

			if (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)
				indicate_disconnect(padapter);

			free_assoc_resources(padapter);
		}
	}

	_memcpy(&pmlmepriv->assoc_bssid, bssid, 6);
	pmlmepriv->assoc_by_bssid=_TRUE;
	status = do_join(padapter);

	_exit_critical(&pmlmepriv->lock, &irqL);
_func_exit_;
	return status;


_Abort_Set_BSSID:
	
	_exit_critical(&pmlmepriv->lock, &irqL);
	DEBUG_INFO(("\nset_802_11_bssid :_Abort_Set_BSSID\n"));
_func_exit_;
	return status;



}


u8 set_802_11_ssid(_adapter* padapter, NDIS_802_11_SSID *ssid) {

	_list	*plist, *phead;
	_irqL irqL;
#ifdef PLATFORM_WINDOWS
	LARGE_INTEGER	sys_time;
	u32  diff_time,cur_time ;
#endif
	u8 status=_TRUE;

	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);

	_queue	*queue	= &(pmlmepriv->scanned_queue);

//	struct sta_info *psta = NULL;
	
_func_enter_;
	
	_enter_critical(&pmlmepriv->lock, &irqL);
	DEBUG_ERR(("\n  set_802_11_ssid:fwstate:%d\n",pmlmepriv->fw_state));

	if(padapter->hw_init_completed==_FALSE){
		DEBUG_ERR(("\n===set_802_11_ssid:hw_init_completed==_FALSE=>exit!!!\n"));
		goto _Abort_Set_SSID;

	}
		

	phead = get_list_head(queue);
	plist = get_next(phead);
	DEBUG_INFO(("\n set_802_11_ssid: ssid= %s \n\n", ssid->Ssid));



 	if   ( (check_fwstate(pmlmepriv, _FW_UNDER_SURVEY) == _TRUE) ||
		 (check_fwstate(pmlmepriv, _FW_UNDER_LINKING) == _TRUE))		
	{
		DEBUG_ERR(("\n\n Set SSID is not allowed under surveying || adhoc master || under linking\n\n."));
		goto _Abort_Set_SSID;
	}


	if  ( (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) )	
	{
		DEBUG_INFO(("\n _FW_LINKED ||WIFI_ADHOC_MASTER_STATE \n."));

		if((_memcmp(&pmlmepriv->assoc_ssid.Ssid, ssid->Ssid, ssid->SsidLength)) 
			&& (pmlmepriv->assoc_ssid.SsidLength==ssid->SsidLength))
		{		
			if((check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _FALSE))
				goto _Abort_Set_SSID;
		}
		else
		{
			DEBUG_INFO(("\n Set SSID not the same ssid \n."));
			DEBUG_INFO(("\n set_ssid = %s len = %x\n.", ssid->Ssid, (unsigned int)ssid->SsidLength));
			DEBUG_INFO(("\n assoc_ssid = %s len = %x\n.", pmlmepriv->assoc_ssid.Ssid, (unsigned int)pmlmepriv->assoc_ssid.SsidLength));


			disassoc_cmd(padapter);

			if (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)
				indicate_disconnect(padapter);

			free_assoc_resources(padapter);

			if((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))
			{
				pmlmepriv->fw_state ^=WIFI_ADHOC_MASTER_STATE;	
				pmlmepriv->fw_state |=WIFI_ADHOC_STATE;
			}
		}
	}
#ifdef PLATFORM_WINDOWS
	if(padapter->securitypriv.btkip_countermeasure==_TRUE){
		DEBUG_ERR(("\n set_802_11_ssid:padapter->securitypriv.btkip_countermeasure==_TRUE\n"));
		NdisGetCurrentSystemTime(&sys_time);	
		cur_time=(u32)(sys_time.QuadPart/10);  // In micro-second.
		DEBUG_ERR(("\n set_802_11_ssid:cur_time=0x%x\n",cur_time));
		DEBUG_ERR(("\n set_802_11_ssid:psecuritypriv->last_mic_err_time=0x%x\n",padapter->securitypriv.btkip_countermeasure_time));
		diff_time = cur_time -padapter->securitypriv.btkip_countermeasure_time; // In micro-second.
		DEBUG_ERR(("\n set_802_11_ssid:diff_time=0x%x\n",diff_time));


		if(diff_time >60000000) 
		{ 
			DEBUG_ERR(("\n set_802_11_ssid(): countermeasure time >60s.\n"));
			padapter->securitypriv.btkip_countermeasure=_FALSE;
		// Update MIC error time.
			padapter->securitypriv.btkip_countermeasure_time=0;
		}
		else
		{ // can't join  in 60 seconds.
			DEBUG_ERR(("\n set_802_11_ssid(): countermeasure time <60s.\n"));
			goto _Abort_Set_SSID;
		}


	}
#endif
	if( !validate_ssid(ssid))
		goto _Abort_Set_SSID;

	_memcpy(&pmlmepriv->assoc_ssid, ssid, sizeof(NDIS_802_11_SSID));
	pmlmepriv->assoc_by_bssid=_FALSE;
	status = do_join(padapter);

	_exit_critical(&pmlmepriv->lock, &irqL);
_func_exit_;
	return status;


_Abort_Set_SSID:
	DEBUG_ERR((" _Abort_Set_SSID"));
	_exit_critical(&pmlmepriv->lock, &irqL);
_func_exit_;
	return status;
}

u8 set_802_11_infrastructure_mode(
	_adapter* padapter, 
	NDIS_802_11_NETWORK_INFRASTRUCTURE networktype)
{

	_irqL irqL;
	struct	mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	struct	wlan_network	*cur_network = &pmlmepriv->cur_network;

	NDIS_802_11_NETWORK_INFRASTRUCTURE* pold_state = &(cur_network->network.InfrastructureMode);
_func_enter_;
	DEBUG_ERR(("\n***set_802_11_infrastructure_mode old_state = %x ; new_state=%x  fw_state = %x \n", *pold_state, networktype, pmlmepriv->fw_state));

	_enter_critical(&(pmlmepriv->lock), &irqL);


	if(*pold_state != networktype)
	{
		DEBUG_INFO((" change mode ; "));

		if((check_fwstate(pmlmepriv, _FW_LINKED)== _TRUE) ||(*pold_state==Ndis802_11IBSS))
			disassoc_cmd(padapter);


		if((check_fwstate(pmlmepriv, _FW_LINKED)== _TRUE) ||
			(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)== _TRUE) )//if(*pold_state!=Ndis802_11AutoUnknown)
			free_assoc_resources(padapter);


		if((check_fwstate(pmlmepriv, _FW_LINKED)== _TRUE) || (*pold_state==Ndis802_11AutoUnknown))
			indicate_disconnect(padapter); //will clr Linked_state; before this function, we must have chked whether  issue dis-assoc_cmd or not


	
	
		*pold_state = networktype;
		
		// clear WIFI_STATION_STATE; WIFI_AP_STATE; WIFI_ADHOC_STATE; WIFI_ADHOC_MASTER_STATE
		pmlmepriv->fw_state &= 0xffffff87 ; 
		switch(networktype)
		{
			case Ndis802_11IBSS:
				pmlmepriv->fw_state |=WIFI_ADHOC_STATE;
				break;
				
			case Ndis802_11Infrastructure:
				pmlmepriv->fw_state |= WIFI_STATION_STATE;
				break;
				
			case Ndis802_11AutoUnknown:
			case Ndis802_11InfrastructureMax:
				break;
                        				
		}

		//SecClearAllKeys(adapter);

		//RT_TRACE(COMP_OID_SET, DBG_LOUD, (" fw_state:%x after changing mode\n.", pmlmepriv->fw_state ));

	}

	_exit_critical(&(pmlmepriv->lock), &irqL);

_func_exit_;
	return _TRUE;

}





u8 set_802_11_disassociate(_adapter*	padapter) {


	_irqL irqL;
	struct mlme_priv * pmlmepriv = &padapter->mlmepriv;

_func_enter_;

	_enter_critical(&pmlmepriv->lock, &irqL);

	if(check_fwstate(pmlmepriv, _FW_LINKED))
	{
		DEBUG_INFO(("\n *** MgntActSet_802_11_DISASSOCIATE: indicate_disconnect***\n\n."));
	
		indicate_disconnect(padapter);
		free_assoc_resources(padapter);
		disassoc_cmd(padapter);		
		
	}
	_exit_critical(&pmlmepriv->lock, &irqL);
_func_exit_;
	return _TRUE;

	
}




u8 set_802_11_bssid_list_scan(_adapter* padapter){
	
	_irqL	irqL;
	struct	mlme_priv		*pmlmepriv= &padapter->mlmepriv;
//	struct cmd_priv 			*pcmdpriv= &padapter->cmdpriv;
	u8	res=_TRUE;

	DEBUG_INFO(("+set_802_11_bssid_list_scan\n"));

_func_enter_;
	if (padapter == NULL) {
		res=_FALSE;
		goto exit;
	}
	if (padapter->hw_init_completed==_FALSE){
		DEBUG_ERR(("\n===set_802_11_bssid_list_scan:hw_init_completed==_FALSE===\n"));
		goto exit;
	}
	
	if (((check_fwstate(pmlmepriv, (_FW_UNDER_SURVEY|_FW_UNDER_LINKING))) == _TRUE) 
				||  pmlmepriv->sitesurveyctrl.traffic_busy==_TRUE)
	{
		// Scan or linking is in progress, do nothing.
		DEBUG_ERR(("\n\n ***set_802_11_bssid_list_scan fail since fw_state = %x***\n\n\n", pmlmepriv->fw_state));
		res = _TRUE;
#if 1 //for dbg
		if(check_fwstate(pmlmepriv, (_FW_UNDER_SURVEY|_FW_UNDER_LINKING))== _TRUE){
			DEBUG_ERR(("\n###_FW_UNDER_SURVEY|_FW_UNDER_LINKING\n\n"));
		}
		else{
			DEBUG_ERR(("\n###pmlmepriv->sitesurveyctrl.traffic_busy==_TRUE\n\n"));
		}
#endif

	} 
	else 
	{
		NDIS_802_11_SSID ssid;
		_enter_critical(&pmlmepriv->lock, &irqL);

		//NdisZeroMemory((unsigned char*)&ssid, sizeof(NDIS_802_11_SSID));
		memset((unsigned char*)&ssid, 0, sizeof(NDIS_802_11_SSID));
		res = sitesurvey_cmd(padapter, &ssid);
		_exit_critical(&pmlmepriv->lock, &irqL);

	}
exit:
_func_exit_;
	DEBUG_INFO(("-set_802_11_bssid_list_scan res=%d\n", res));
	return res;
}




u8 set_802_11_reload_defaults(_adapter * padapter, NDIS_802_11_RELOAD_DEFAULTS reloadDefaults){

_func_enter_;
	switch( reloadDefaults) {
		case Ndis802_11ReloadWEPKeys:
			DEBUG_INFO(("SetInfo OID_802_11_RELOAD_DEFAULTS : Ndis802_11ReloadWEPKeys\n"));
			break;
	}


	// SecClearAllKeys(Adapter);
	// 8711 CAM was not for En/Decrypt only
	// so, we can't clear all keys.
	// should we disable WPAcfg (ox0088) bit 1-2, instead of clear all CAM 
	//3TO DO...

	return _TRUE;
}


u8 set_802_11_powersavemode(_adapter*	padapter, ps_mode rtPsMode) {

	u8 ret=_SUCCESS;


	return ret;
}


u8 set_802_11_authenticaion_mode(_adapter* padapter, NDIS_802_11_AUTHENTICATION_MODE authmode) {

	u8 ret;
	struct security_priv *psecuritypriv=&(padapter->securitypriv);
	int res;
_func_enter_;
	psecuritypriv->ndisauthtype=authmode;
	DEBUG_ERR(("set_802_11_authenticaion_mode:psecuritypriv->ndisauthtype=%d",psecuritypriv->ndisauthtype));
	if(psecuritypriv->ndisauthtype>3)
		psecuritypriv->dot11AuthAlgrthm=2;
	res=set_auth(padapter,psecuritypriv);
	if(res==_SUCCESS)
		ret=_TRUE;
	else
		ret=_FALSE;
_func_exit_;
	return ret;

}

u8 set_802_11_add_wep(_adapter* padapter, NDIS_802_11_WEP *wep){

	u8		bdefaultkey;
	u8		btransmitkey;
	sint		keyid,res;
	struct security_priv* psecuritypriv=&(padapter->securitypriv);
	u8		ret=_SUCCESS;

_func_enter_;
	bdefaultkey=(wep->KeyIndex & 0x40000000) > 0 ? _FALSE : _TRUE;   //for ???
	btransmitkey= (wep->KeyIndex & 0x80000000) > 0 ? _TRUE  : _FALSE;	//for ???
	keyid=wep->KeyIndex & 0x3fffffff;

	DEBUG_INFO(("set_802_11_add_wep\n"));

	if(keyid>4){
		DEBUG_ERR(("MgntActSet_802_11_ADD_WEP:keyid>4=>fail\n"));
		ret=_FALSE;
		goto exit;
	}
	switch(wep->KeyLength){
		case 5:
			psecuritypriv->dot11PrivacyAlgrthm=_WEP40_;
			DEBUG_INFO(("MgntActSet_802_11_ADD_WEP:wep->KeyLength=5\n"));
			break;
		case 13:
			psecuritypriv->dot11PrivacyAlgrthm=_WEP104_;
			DEBUG_INFO(("MgntActSet_802_11_ADD_WEP:wep->KeyLength=13\n"));
			break;
		default:
			psecuritypriv->dot11PrivacyAlgrthm=_NO_PRIVACY_;
			DEBUG_INFO(("MgntActSet_802_11_ADD_WEP:wep->KeyLength!=5 or 13\n"));
			break;
	}
	DEBUG_INFO(("MgntActSet_802_11_ADD_WEP:befor memcpy, wep->KeyLength=%d wep->KeyIndex=%d  keyid =%d\n",wep->KeyLength,wep->KeyIndex,keyid));
	_memcpy(&(psecuritypriv->dot11DefKey[keyid].skey[0]),&(wep->KeyMaterial),wep->KeyLength);
	psecuritypriv->dot11DefKeylen[keyid]=wep->KeyLength;
	psecuritypriv->dot11PrivacyKeyIndex=keyid;
	DEBUG_INFO(("MgntActSet_802_11_ADD_WEP:security key material : %x %x %x %x %x %x %x %x %x %x %x %x %x \n",
		psecuritypriv->dot11DefKey[keyid].skey[0],psecuritypriv->dot11DefKey[keyid].skey[1],psecuritypriv->dot11DefKey[keyid].skey[2],
		psecuritypriv->dot11DefKey[keyid].skey[3],psecuritypriv->dot11DefKey[keyid].skey[4],psecuritypriv->dot11DefKey[keyid].skey[5],
		psecuritypriv->dot11DefKey[keyid].skey[6],psecuritypriv->dot11DefKey[keyid].skey[7],psecuritypriv->dot11DefKey[keyid].skey[8],
		psecuritypriv->dot11DefKey[keyid].skey[9],psecuritypriv->dot11DefKey[keyid].skey[10],psecuritypriv->dot11DefKey[keyid].skey[11],
		psecuritypriv->dot11DefKey[keyid].skey[12]));
	res=set_key(padapter,psecuritypriv,keyid);
	if(res==_FAIL)
		ret= _FALSE;
exit:
_func_exit_;
	return ret;
}

u8 set_802_11_remove_wep(_adapter* padapter, u32 keyindex){
	
	u8 ret=_SUCCESS;
_func_enter_;
	if (keyindex >= 0x80000000 || padapter == NULL){
		
		ret=_FALSE;
		goto exit;

	} else {
		int res;
		struct security_priv* psecuritypriv=&(padapter->securitypriv);
		if( keyindex < 4 ){
			//NdisZeroMemory(&psecuritypriv->dot11DefKey[keyindex],16);
			memset(&psecuritypriv->dot11DefKey[keyindex], 0, 16);
			res=set_key(padapter,psecuritypriv,keyindex);
			psecuritypriv->dot11DefKeylen[keyindex]=0;
			if(res==_FAIL)
				ret=_FAIL;
		}else{			
			ret=_FAIL;
		}
		
	}
exit:	
_func_exit_;
	return ret;
}

u8 set_802_11_add_key(_adapter* padapter, NDIS_802_11_KEY *key){

	uint			encryptionalgo;
	u8 * pbssid;
	struct sta_info *stainfo;
	u8	bgroup = _FALSE;
	u8	bgrouptkey = _FALSE;
	u8	ret=_SUCCESS;
_func_enter_;
	if (((key->KeyIndex & 0x80000000) == 0) && ((key->KeyIndex & 0x40000000) > 0)){

		// It is invalid to clear bit 31 and set bit 30. If the miniport driver encounters this combination, 
		// it must fail the request and return NDIS_STATUS_INVALID_DATA.
		DEBUG_ERR(("MgntActSet_802_11_ADD_KEY: ((key->KeyIndex & 0x80000000) == 0)[=%d] ",(int)(key->KeyIndex & 0x80000000) == 0));
		DEBUG_ERR(("MgntActSet_802_11_ADD_KEY:((key->KeyIndex & 0x40000000) > 0)[=%d]" , (int)(key->KeyIndex & 0x40000000) > 0));
		DEBUG_ERR(("MgntActSet_802_11_ADD_KEY: key->KeyIndex=%d \n" ,(int)key->KeyIndex));
		ret= _FAIL;
		goto exit;
	}

	if(key->KeyIndex & 0x40000000) { 
		// Pairwise key

		DEBUG_ERR(("OID_802_11_ADD_KEY: +++++ Pairwise key +++++\n"));
	
		pbssid=get_bssid(&padapter->mlmepriv);
		stainfo=get_stainfo(&padapter->stapriv,pbssid);

		if((stainfo!=NULL)&&(padapter->securitypriv.dot11AuthAlgrthm==2)){
			DEBUG_ERR(("OID_802_11_ADD_KEY:( stainfo!=NULL)&&(Adapter->securitypriv.dot11AuthAlgrthm==2)\n"));
			encryptionalgo=stainfo->dot118021XPrivacy;
		}
		else{
			DEBUG_ERR(("OID_802_11_ADD_KEY: stainfo==NULL)||(Adapter->securitypriv.dot11AuthAlgrthm!=2)\n"));
			encryptionalgo=padapter->securitypriv.dot11PrivacyAlgrthm;
		}

		DEBUG_ERR(("Set_802_11_ADD_KEY: (encryptionalgo ==%d)!\n",encryptionalgo ));
		DEBUG_ERR(("Set_802_11_ADD_KEY: (Adapter->securitypriv.dot11PrivacyAlgrthm ==%d)!\n",padapter->securitypriv.dot11PrivacyAlgrthm));
		DEBUG_ERR(("Set_802_11_ADD_KEY: (Adapter->securitypriv.dot11AuthAlgrthm ==%d)!\n",padapter->securitypriv.dot11AuthAlgrthm));

		if((stainfo!=NULL)){
			DEBUG_ERR(("Set_802_11_ADD_KEY: (stainfo->dot118021XPrivacy ==%d)!\n",stainfo->dot118021XPrivacy));
		}
		
		if(key->KeyIndex & 0x000000FF) {
			// The key index is specified in the lower 8 bits by values of zero to 255.
			// The key index should be set to zero for a Pairwise key, and the driver should fail with
			// NDIS_STATUS_INVALID_DATA if the lower 8 bits is not zero
			DEBUG_ERR((" key->KeyIndex & 0x000000FF.\n"));
			ret= _FAIL;
			goto exit;
		}

		// check BSSID
		if (IS_MAC_ADDRESS_BROADCAST(key->BSSID) == _TRUE){

			DEBUG_ERR(("MacAddr_isBcst(key->BSSID)\n"));
			ret= _FALSE;
			goto exit;
		}

		// Check key length for TKIP.
		//if(encryptionAlgorithm == RT_ENC_TKIP_ENCRYPTION && key->KeyLength != 32)
		if((encryptionalgo== _TKIP_)&& (key->KeyLength != 32)){
			DEBUG_ERR(("TKIP KeyLength:%d != 32\n", key->KeyLength));
			ret=_FAIL;
			goto exit;

		}

		// Check key length for AES.
		if((encryptionalgo== _AES_)&& (key->KeyLength != 16)) {
			// For our supplicant, EAPPkt9x.vxd, cannot differentiate TKIP and AES case.
			if(key->KeyLength == 32) {
				key->KeyLength = 16; 
			} else {
				ret= _FAIL;
				goto exit;
			}
		}

		// Check key length for WEP. For NDTEST, 2005.01.27, by rcnjko.
		if(	(encryptionalgo== _WEP40_|| encryptionalgo== _WEP104_) && (key->KeyLength != 5 || key->KeyLength != 13)) {
			DEBUG_ERR(("WEP KeyLength:%d != 5 or 13\n", key->KeyLength));
			ret=_FAIL;
			goto exit;
		}

		bgroup = _FALSE;

		// Check the pairwise key. Added by Annie, 2005-07-06.
		DEBUG_ERR(("------------------------------------------\n"));
		DEBUG_ERR(("[Pairwise Key set]\n"));
		DEBUG_ERR(("------------------------------------------\n"));
		DEBUG_ERR(("key index: 0x%8x(0x%8x)\n", key->KeyIndex,(key->KeyIndex&0x3)));
		DEBUG_ERR(("key Length: %d\n", key->KeyLength));
		DEBUG_ERR(("------------------------------------------\n"));
	
	} else {
		
		// Group key
		DEBUG_ERR(("OID_802_11_ADD_KEY: +++++ Group key +++++\n"));


		// when add wep key through add key and didn't assigned encryption type before
		if((padapter->securitypriv.ndisauthtype<=3)&&(padapter->securitypriv.dot118021XGrpPrivacy==0)){
			DEBUG_ERR(("keylen=%d( Adapter->securitypriv.dot11PrivacyAlgrthm=%x  )padapter->securitypriv.dot118021XGrpPrivacy(%x)\n", key->KeyLength,padapter->securitypriv.dot11PrivacyAlgrthm,padapter->securitypriv.dot118021XGrpPrivacy));

			switch(key->KeyLength)
			{
				case 5:
					padapter->securitypriv.dot11PrivacyAlgrthm=_WEP40_;
					DEBUG_ERR(("Adapter->securitypriv.dot11PrivacyAlgrthm= %d key->KeyLength=%d\n", padapter->securitypriv.dot11PrivacyAlgrthm,key->KeyLength));
					break;
				case 13:
					padapter->securitypriv.dot11PrivacyAlgrthm=_WEP104_;
					DEBUG_ERR(("Adapter->securitypriv.dot11PrivacyAlgrthm= %x key->KeyLength=%d\n", padapter->securitypriv.dot11PrivacyAlgrthm,key->KeyLength));
					break;
				default:
					padapter->securitypriv.dot11PrivacyAlgrthm=_NO_PRIVACY_;
					DEBUG_ERR(("Adapter->securitypriv.dot11PrivacyAlgrthm= %x key->KeyLength=%d\n", padapter->securitypriv.dot11PrivacyAlgrthm,key->KeyLength));
					break;
			}
			encryptionalgo=padapter->securitypriv.dot11PrivacyAlgrthm;
			DEBUG_ERR((" Adapter->securitypriv.dot11PrivacyAlgrthm=%x\n", padapter->securitypriv.dot11PrivacyAlgrthm));
			
		}
		else {
			encryptionalgo=padapter->securitypriv.dot118021XGrpPrivacy;
			DEBUG_ERR(("( Adapter->securitypriv.dot11PrivacyAlgrthm=%x  )encryptionalgo(%x)=padapter->securitypriv.dot118021XGrpPrivacy(%x)keylen=%d\n", padapter->securitypriv.dot11PrivacyAlgrthm,encryptionalgo,padapter->securitypriv.dot118021XGrpPrivacy,key->KeyLength));

		}
		if((check_fwstate(&padapter->mlmepriv, WIFI_ADHOC_STATE)==_TRUE) && (IS_MAC_ADDRESS_BROADCAST(key->BSSID) == _FALSE)) {
			DEBUG_ERR((" IBSS but BSSID is not Broadcast Address.\n"));
			ret= _FAIL;
			goto exit;
		}

		// Check key length for TKIP
		if((encryptionalgo== _TKIP_) && (key->KeyLength != 32)) {

			DEBUG_ERR((" TKIP GTK KeyLength:%d != 32\n", key->KeyLength));
			ret= _FAIL;
			goto exit;

		} else if(encryptionalgo== _AES_ && (key->KeyLength != 16 && key->KeyLength != 32) ) {
			
			// Check key length for AES
			// For NDTEST, we allow keylen=32 in this case. 2005.01.27, by rcnjko.
			DEBUG_ERR(("<=== SetInfo, OID_802_11_ADD_KEY: AES GTK KeyLength:%d != 16 or 32\n", key->KeyLength));
			ret= _FAIL;
			goto exit;
		}

		// Change the key length for EAPPkt9x.vxd. Added by Annie, 2005-11-03.
		if((encryptionalgo==  _AES_) && (key->KeyLength == 32) ) {
			key->KeyLength = 16; 
			DEBUG_ERR(("AES key length changed: %d\n", key->KeyLength) );
		}

		if(key->KeyIndex & 0x8000000) {
			bgrouptkey = _TRUE;
		}

		if((check_fwstate(&padapter->mlmepriv, WIFI_ADHOC_STATE)==_TRUE)&&(check_fwstate(&padapter->mlmepriv, _FW_LINKED)==_TRUE)) {
			bgrouptkey = _TRUE;
		}

		bgroup = _TRUE;

		DEBUG_ERR(("------------------------------------------\n") );
		DEBUG_ERR(("[Group Key set]\n") );
		DEBUG_ERR(("------------------------------------------\n")) ;
		DEBUG_ERR(("key index: 0x%8x(0x%8x)\n", key->KeyIndex,(key->KeyIndex&0x3)));
		DEBUG_ERR(("key Length: %d\n", key->KeyLength)) ;
		DEBUG_ERR(("------------------------------------------\n"));
	}

	// If WEP encryption algorithm, just call MgntActSet_802_11_ADD_WEP().
	if((padapter->securitypriv.dot11AuthAlgrthm !=2)&&(encryptionalgo== _WEP40_  || encryptionalgo== _WEP104_)) {



		u32 len = FIELD_OFFSET(NDIS_802_11_KEY, KeyMaterial) + key->KeyLength;
		NDIS_802_11_WEP *wep = &padapter->securitypriv.ndiswep;
		u8 ret;
		u32 keyindex;

		DEBUG_ERR(("OID_802_11_ADD_KEY: +++++ WEP key +++++\n"));

		wep->Length = len;
		keyindex=key->KeyIndex&0x7fffffff;
		wep->KeyIndex = keyindex ;
		wep->KeyLength = key->KeyLength;
		DEBUG_ERR(("OID_802_11_ADD_KEY:Before memcpy \n"));
//		RTDumpHex("Key Material", wep->KeyMaterial, (USHORT)wep->KeyLength, ':' );
		_memcpy(wep->KeyMaterial,key->KeyMaterial, key->KeyLength);
		//NdisMoveMemory(wep->KeyMaterial, key->KeyMaterial, key->KeyLength);
		_memcpy(&(padapter->securitypriv.dot11DefKey[keyindex].skey[0]), key->KeyMaterial, key->KeyLength);
		//NdisMoveMemory(&(Adapter->securitypriv.dot11DefKey[keyindex].skey[0]), key->KeyMaterial, key->KeyLength);
		padapter->securitypriv.dot11DefKeylen[keyindex]=key->KeyLength;
		padapter->securitypriv.dot11PrivacyKeyIndex=keyindex;
		ret = set_802_11_add_wep(padapter, wep);
		//ExFreePool(wep);
		goto exit;
	}

	if(key->KeyIndex & 0x20000000){
		// SetRSC
		DEBUG_ERR(("OID_802_11_ADD_KEY: +++++ SetRSC+++++\n"));
		if(bgroup == _TRUE){

			NDIS_802_11_KEY_RSC keysrc=key->KeyRSC & 0x00ffffffffffffULL;
			_memcpy(&padapter->securitypriv.dot11Grprxpn, &keysrc, 8 );
			//NdisMoveMemory(&Adapter->securitypriv.dot11Grprxpn, &keysrc, 8 );
		} else {
			NDIS_802_11_KEY_RSC keysrc=key->KeyRSC & 0x00ffffffffffffULL;
			
_memcpy(&padapter->securitypriv.dot11Grptxpn, &keysrc, 8);
			//NdisMoveMemory(&Adapter->securitypriv.dot11Grptxpn, &keysrc, 8 );
		}
			
	}

	// Indicate this key idx is used for TX
	// Save the key in KeyMaterial
	if(bgroup == _TRUE) {
		int res;
		// Group transmit key
		if(bgrouptkey == _TRUE) {
			//Adapter->MgntInfo.SecurityInfo.GroupTransmitKeyIdx = (UCHAR)key->KeyIndex;
			padapter->securitypriv.dot118021XGrpKeyid=(u8)key->KeyIndex;
		}
		
		
		memset(&padapter->securitypriv.dot118021XGrpKey[(u8)(key->KeyIndex & 0x03)], 0, 16);
		memset(&padapter->securitypriv.dot118021XGrptxmickey, 0, 16);
		memset(&padapter->securitypriv.dot118021XGrprxmickey, 0, 16);
		if((key->KeyIndex & 0x10000000)){
			_memcpy(&padapter->securitypriv.dot118021XGrptxmickey, key->KeyMaterial + 16, 8);
			_memcpy(&padapter->securitypriv.dot118021XGrprxmickey, key->KeyMaterial + 24, 8);
			DEBUG_ERR(("\n set_802_11_add_key:rx mic :0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
				padapter->securitypriv.dot118021XGrprxmickey.skey[0],padapter->securitypriv.dot118021XGrprxmickey.skey[1],
				padapter->securitypriv.dot118021XGrprxmickey.skey[2],padapter->securitypriv.dot118021XGrprxmickey.skey[3],
				padapter->securitypriv.dot118021XGrprxmickey.skey[4],padapter->securitypriv.dot118021XGrprxmickey.skey[5],
				padapter->securitypriv.dot118021XGrprxmickey.skey[6],padapter->securitypriv.dot118021XGrprxmickey.skey[7]));
			DEBUG_ERR(("\n set_802_11_add_key:set Group mic key!!!!!!!!\n"));

		} else {
			_memcpy(&padapter->securitypriv.dot118021XGrptxmickey, key->KeyMaterial + 24, 8);
			_memcpy(&padapter->securitypriv.dot118021XGrprxmickey, key->KeyMaterial + 16, 8);
			DEBUG_ERR(("\n set_802_11_add_key:rx mic :0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
				padapter->securitypriv.dot118021XGrprxmickey.skey[0],padapter->securitypriv.dot118021XGrprxmickey.skey[1],
				padapter->securitypriv.dot118021XGrprxmickey.skey[2],padapter->securitypriv.dot118021XGrprxmickey.skey[3],
				padapter->securitypriv.dot118021XGrprxmickey.skey[4],padapter->securitypriv.dot118021XGrprxmickey.skey[5],
				padapter->securitypriv.dot118021XGrprxmickey.skey[6],padapter->securitypriv.dot118021XGrprxmickey.skey[7]));
			DEBUG_ERR(("\n set_802_11_add_key:set Group mic key!!!!!!!!\n"));
		
		}
		// set group key by index
		_memcpy(&padapter->securitypriv.dot118021XGrpKey[(u8)(key->KeyIndex & 0x03)], key->KeyMaterial, key->KeyLength );
		key->KeyIndex=key->KeyIndex & 0x03;
		padapter->securitypriv.binstallGrpkey=_TRUE;
		padapter->securitypriv.bcheck_grpkey=_FALSE;
		DEBUG_ERR(("reset group key"));
		res=set_key(padapter,&padapter->securitypriv,key->KeyIndex);
			if(res==_FAIL)
				ret= _FAIL;
			goto exit;
			
	}else{
		u8 res;
		// Pairwise Key
		pbssid=get_bssid(&padapter->mlmepriv);
		stainfo=get_stainfo(&padapter->stapriv , pbssid );
		if(stainfo!=NULL){
			// clear keybuffer
			memset( &stainfo->dot118021x_UncstKey, 0, 16);


			_memcpy(&stainfo->dot118021x_UncstKey, key->KeyMaterial, 16);
	
		if(encryptionalgo== _TKIP_){

		// if TKIP, save the Receive/Transmit MIC key in KeyMaterial[128-255]
		if((key->KeyIndex & 0x10000000)){
			_memcpy(&stainfo->dot11tkiptxmickey, key->KeyMaterial + 16, 8);
			_memcpy(&stainfo->dot11tkiprxmickey, key->KeyMaterial + 24, 8);

		} else {
			_memcpy(&stainfo->dot11tkiptxmickey, key->KeyMaterial + 24, 8);
			_memcpy(&stainfo->dot11tkiprxmickey, key->KeyMaterial + 16, 8);

		}

	} else if(encryptionalgo == _AES_) {		
	
	}
	//Set key to CAM through H2C command
	if(bgrouptkey){
		res=setstakey_cmd(padapter, (unsigned char *)stainfo, _FALSE);
		DEBUG_ERR(("\n set_802_11_add_key:setstakey_cmd(group)\n"));
		}
	else{
		res=setstakey_cmd(padapter, (unsigned char *)stainfo, _TRUE);
		DEBUG_ERR(("\n set_802_11_add_key:setstakey_cmd(unicast)\n"));
	}
		if(res ==_FALSE)
			ret= _FAIL;
		}

	}

exit:
_func_exit_;
	return ret;
}

u8 set_802_11_remove_key(_adapter*	padapter, NDIS_802_11_REMOVE_KEY *key){
	
	uint				encryptionalgo;
	u8 * pbssid;
	struct sta_info *stainfo;
	u8	bgroup = (key->KeyIndex & 0x4000000) > 0 ? _FALSE: _TRUE;
	u8	keyIndex = (u8)key->KeyIndex & 0x03;
	u8	ret=_SUCCESS;
_func_enter_;
	if ((key->KeyIndex & 0xbffffffc) > 0) {
		ret=_FAIL;
		goto exit;
	}


	if (bgroup == _TRUE) {
		encryptionalgo= padapter->securitypriv.dot118021XGrpPrivacy;
		// clear group key by index
		//NdisZeroMemory(Adapter->MgntInfo.SecurityInfo.KeyBuf[keyIndex], MAX_WEP_KEY_LEN);
		//Adapter->MgntInfo.SecurityInfo.KeyLen[keyIndex] = 0;
		
		memset(&padapter->securitypriv.dot118021XGrpKey[keyIndex], 0, 16);
		

		//! \todo Send a H2C Command to Firmware for removing this Key in CAM Entry.
	
	} else {
		pbssid=get_bssid(&padapter->mlmepriv);
		stainfo=get_stainfo(&padapter->stapriv , pbssid );
		if(stainfo !=NULL){
			encryptionalgo=stainfo->dot118021XPrivacy;

		// clear key by BSSID
		memset(&stainfo->dot118021x_UncstKey, 0, 16);
		
		//! \todo Send a H2C Command to Firmware for disable this Key in CAM Entry.

		}
		else{
			ret= _FAIL;
			goto exit;
		}
	}

exit:
_func_exit_;

	return _TRUE;
}

u8 set_802_11_test(_adapter* padapter, NDIS_802_11_TEST *test){
	u8 ret=_TRUE;
_func_enter_;
#ifdef PLATFORM_WINDOWS
	switch(test->Type) {
		case 1:
			NdisMIndicateStatus(padapter->hndis_adapter, NDIS_STATUS_MEDIA_SPECIFIC_INDICATION, (PVOID)&test->AuthenticationEvent, test->Length - 8);
			NdisMIndicateStatusComplete(padapter->hndis_adapter);
			break;

		case 2:
			NdisMIndicateStatus(padapter->hndis_adapter, NDIS_STATUS_MEDIA_SPECIFIC_INDICATION, (PVOID)&test->RssiTrigger, sizeof(NDIS_802_11_RSSI));
			NdisMIndicateStatusComplete(padapter->hndis_adapter);
			break;

		default:
			ret=_FALSE;
			break;
	}
#endif
_func_exit_;
	return ret;
}

u8	set_802_11_pmkid(_adapter*	padapter, NDIS_802_11_PMKID *pmkid)
{
	u8	ret=_SUCCESS;

	return ret;
}

