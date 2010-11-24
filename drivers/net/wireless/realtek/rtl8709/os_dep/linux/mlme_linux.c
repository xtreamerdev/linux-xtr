/******************************************************************************
* mlme_linux.c                                                                                                                                 *
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


#define _MLME_OSDEP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <mlme_osdep.h>
#include <rtl871x_mlme.h>

#ifdef PLATFORM_LINUX

#include <linux/compiler.h>
//#include <linux/config.h>
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
#include <wlan_mlme.h>
#include <wlan_sme.h>
#endif

void sitesurvey_ctrl_handler (IN PVOID	FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;
	_sitesurvey_ctrl_handler(adapter);
        }

void join_timeout_handler (IN	PVOID	FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;
	_join_timeout_handler(adapter);
}

void init_mlme_timer(_adapter *padapter)
{
	struct	mlme_priv *pmlmepriv = &padapter->mlmepriv;

	_init_timer(&(pmlmepriv->assoc_timer), padapter->pnetdev, join_timeout_handler, (pmlmepriv->nic_hdl));
	_init_timer(&(pmlmepriv->sitesurveyctrl.sitesurvey_ctrl_timer), padapter->pnetdev, sitesurvey_ctrl_handler, (u8 *)(pmlmepriv->nic_hdl));
}

void os_indicate_connect(_adapter *adapter)
{

_func_enter_;	

	netif_carrier_on(adapter->pnetdev);

_func_exit_;	
}


void os_indicate_disconnect( _adapter *adapter )
{
	
_func_enter_;	
	netif_carrier_off(adapter->pnetdev);
	memset((unsigned char *)&adapter->securitypriv, 0, sizeof (struct security_priv));
 	
_func_exit_;	
}


void report_sec_ie(_adapter *adapter,u8 authmode,u8 *sec_ie)
{
		u8 *buff,*p,i;
		union iwreq_data wrqu;

_func_enter_;

	if(authmode==_WPA_IE_ID_){
		buff=kmalloc(IW_CUSTOM_MAX,GFP_ATOMIC);
		memset(buff,0,IW_CUSTOM_MAX);
		p=buff;
		p+=sprintf(p,"ASSOCINFO(ReqIEs=");
		for(i=0;i<sec_ie[1]+2;i++){
			p+=sprintf(p,"%02x",sec_ie[i]);
		}
		p+=sprintf(p,")");
		memset(&wrqu,0,sizeof(wrqu));
		wrqu.data.length=p-buff;
		wireless_send_event(adapter->pnetdev,IWEVCUSTOM,&wrqu,buff);
	}

_func_exit_;
}

