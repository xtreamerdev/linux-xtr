/******************************************************************************
* rtl871x_mp.c                                                                                                                                 *
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
#define _RTL871X_MP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#ifdef CONFIG_MP_INCLUDED

VOID
mp_wi_callback(
	IN NDIS_WORK_ITEM*	pwk_item,
	IN PVOID			cntx
	)
{
       _adapter* padapter =(_adapter *)cntx;
	struct mp_priv *pmppriv=&padapter->mppriv;
	struct mp_wi_cntx	*pmp_wi_cntx=&pmppriv->wi_cntx;
	
	// Execute specified action.
	if(pmp_wi_cntx->curractfunc != NULL)
	{
		LARGE_INTEGER	cur_time;
		ULONGLONG start_time, end_time;				
	       NdisGetCurrentSystemTime(&cur_time);	// driver version
	       start_time=cur_time.QuadPart/10; // The return value is in microsecond	
	
		pmp_wi_cntx->curractfunc(padapter);
		
              NdisGetCurrentSystemTime(&cur_time);	// driver version
	       end_time=cur_time.QuadPart/10; // The return value is in microsecond	

		DEBUG_INFO(("WorkItemActType: %d, time spent: %I64d us\n",
			pmp_wi_cntx->param.act_type, (end_time-start_time)));

	}

	NdisAcquireSpinLock( &(pmp_wi_cntx->mp_wi_lock) );
	pmp_wi_cntx->bmp_wi_progress= _FALSE;
	NdisReleaseSpinLock( &(pmp_wi_cntx->mp_wi_lock) );

	if(pmp_wi_cntx->bmpdrv_unload)
	{
		NdisSetEvent(&(pmp_wi_cntx->mp_wi_evt));
	}

	

}

sint  init_mp_priv (struct mp_priv  *pmppriv)	
{
      
      struct wlan_network *pnetwork = (struct wlan_network *)&pmppriv->mp_network;

      pmppriv->mode = 1;
	  
      pmppriv->network_macaddr[0]=0x00;
      pmppriv->network_macaddr[1]=0xE0;
      pmppriv->network_macaddr[2]=0x4C;
      pmppriv->network_macaddr[3]=0x87;
      pmppriv->network_macaddr[4]=0x66;
      pmppriv->network_macaddr[5]=0x55;
	  
      pnetwork->network.MacAddress[0] =0x00;
      pnetwork->network.MacAddress[1] =0xE0;
      pnetwork->network.MacAddress[2] =0x4C;
      pnetwork->network.MacAddress[3] =0x87;
      pnetwork->network.MacAddress[4] =0x66;
      pnetwork->network.MacAddress[5] =0x55;

      pnetwork->network.Ssid.SsidLength = 8;
      _memcpy(pnetwork->network.Ssid.Ssid, "mp_8711", pnetwork->network.Ssid.SsidLength);      	  
	  
	  
      pmppriv->tx_pktcount = 0;
      pmppriv->rx_pktcount = 0;
      pmppriv->rx_crcerrpktcount = 0;
      
      pmppriv->rx_testcnt = 0;
      pmppriv->rx_testcnt1 = 0;
      pmppriv->rx_testcnt2 = 0;
	  
      pmppriv->tx_testcnt = 0;
      pmppriv->tx_testcnt1 = 0;

	  
      
	  
      pmppriv->curr_ch = 6;
      pmppriv->curr_modem = MIXED_PHY;
	  

      return _SUCCESS;
}

void mp8711init(_adapter *padapter)
{

	struct mp_priv	*pmppriv=&padapter->mppriv;
	// MP8711 WorkItem   
	struct mp_wi_cntx *pmp_wi_cntx = &(pmppriv->wi_cntx);
	pmp_wi_cntx->bmpdrv_unload =_FALSE;
	pmp_wi_cntx->bmp_wi_progress=_FALSE;
	pmp_wi_cntx->curractfunc=NULL;
	NdisInitializeEvent(&(pmp_wi_cntx->mp_wi_evt));	
	NdisAllocateSpinLock(&(pmp_wi_cntx->mp_wi_lock));	
	NdisInitializeWorkItem(&(pmp_wi_cntx->mp_wi), 
		                          mp_wi_callback,// MP8711WorkItemCallback, 
 	                                 padapter);

       // H2C/C2H 
   //    padapter->nH2cCmdCnt = 0 ;
   //    NdisAllocateSpinLock(&(pAdapter->MpH2cSpinLock));
//	NdisInitializeEvent(&(pAdapter->CMDCommEvent));
						
	//New Arch.
       init_mp_priv(&padapter->mppriv);
	
						
 	                                 
}
#endif
