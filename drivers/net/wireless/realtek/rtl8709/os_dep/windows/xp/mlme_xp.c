


#define _MLME_OSDEP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#ifdef PLATFORM_LINUX

#include <linux/compiler.h>
#include <linux/config.h>
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

#ifdef PLATFORM_WINDOWS

#include <mlme_osdep.h>

#endif

int	time_after(u32 now, u32 old)
{
_func_enter_;
	if(now > old){
_func_exit_;
		return 1;
}
	else{
_func_exit_;			
		return 0;
}
}


void sitesurvey_ctrl_handler (
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	)
{
	_adapter *adapter = (_adapter *)FunctionContext;
	_sitesurvey_ctrl_handler(adapter);
}
	
void join_timeout_handler (
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	)
       {
	_adapter *adapter = (_adapter *)FunctionContext;
	_join_timeout_handler(adapter);
}

void init_mlme_timer(_adapter *padapter)
{
	struct mlme_priv* pmlmepriv = &padapter->mlmepriv;

	_init_timer(&(pmlmepriv->assoc_timer),padapter->hndis_adapter, join_timeout_handler, (pmlmepriv->nic_hdl));
	_init_timer(&(pmlmepriv->sitesurveyctrl.sitesurvey_ctrl_timer), padapter->hndis_adapter, sitesurvey_ctrl_handler, (u8 *)(pmlmepriv->nic_hdl));
}

void os_indicate_connect(_adapter *adapter)
{
_func_enter_;	
		NdisMIndicateStatus(adapter->hndis_adapter, NDIS_STATUS_MEDIA_CONNECT, NULL, 0);
		NdisMIndicateStatusComplete(adapter->hndis_adapter);
_func_exit_;	
}


void os_indicate_disconnect( _adapter *adapter )
{
_func_enter_;	
	NdisMIndicateStatus(adapter->hndis_adapter, NDIS_STATUS_MEDIA_DISCONNECT, NULL, 0);
	NdisMIndicateStatusComplete(adapter->hndis_adapter);
_func_exit_;	
}

void report_sec_ie(_adapter *adapter,u8 authmode,u8 *sec_ie)
{
	
}


