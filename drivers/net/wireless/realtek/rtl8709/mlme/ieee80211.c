/******************************************************************************
* ieee80211.c                                                                                                                                 *
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
#define _IEEE80211_C

#include <drv_types.h>
//#include <ieee80211.h>
#include <wifi.h>
#include <osdep_service.h>
#include <wlan_bssdef.h>
//#include <TypeDef.h>


//-----------------------------------------------------------
// for adhoc-master to generate ie and provide supported-rate to fw 
//-----------------------------------------------------------

u8 	WIFI_CCKRATES[] = 
{(IEEE80211_CCK_RATE_1MB | IEEE80211_BASIC_RATE_MASK),
 (IEEE80211_CCK_RATE_2MB | IEEE80211_BASIC_RATE_MASK),
 (IEEE80211_CCK_RATE_5MB | IEEE80211_BASIC_RATE_MASK),
 (IEEE80211_CCK_RATE_11MB | IEEE80211_BASIC_RATE_MASK)};

u8 	WIFI_OFDMRATES[] = 
{(IEEE80211_OFDM_RATE_6MB),
 (IEEE80211_OFDM_RATE_9MB),
 (IEEE80211_OFDM_RATE_12MB),
 (IEEE80211_OFDM_RATE_18MB),
 (IEEE80211_OFDM_RATE_24MB),
 IEEE80211_OFDM_RATE_36MB,
 IEEE80211_OFDM_RATE_48MB,
 IEEE80211_OFDM_RATE_54MB};


#ifdef CONFIG_RTL8192U
VOID
HTConstructCapabilityElement(
_adapter *padapter, 	
PHT_CAPABILITY_ELE pCapELE
)
{	
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	
	
	//HT capability info
	pCapELE->AdvCoding 		= 0; // This feature is not supported now!!
	// TODO: BW 40 MHz ?

	#ifdef CONFIG_SUPPORT_40MHZ
	pCapELE->ChlWidth 		= 1; //(pHTInfo->bRegBW40MHz?1:0);
#else
	//Dont Support 40 MHz
	pCapELE->ChlWidth 		= 0; //(pHTInfo->bRegBW40MHz?1:0);
#endif

	pCapELE->MimoPwrSave 	= pHTInfo->PeerMimoPs;
	pCapELE->GreenField		= 0; // This feature is not supported now!!
	pCapELE->ShortGI20Mhz	= 1; // We can receive Short GI!!
#ifdef CONFIG_SUPPORT_40MHZ
	pCapELE->ShortGI40Mhz	= 1; // We can receive Short GI!!
#else
	//Dont Support 40 MHz
	pCapELE->ShortGI40Mhz	= 0; // We can receive Short GI!!
#endif
	
	//DbgPrint("TX HT cap/info ele BW=%d SG20=%d SG40=%d\n\r", 
		//pCapELE->ChlWidth, pCapELE->ShortGI20Mhz, pCapELE->ShortGI40Mhz);
	pCapELE->TxSTBC 		= 1;
	pCapELE->RxSTBC 		= 0;
	pCapELE->DelayBA		= 0;	// Do not support now!!
	// TODO: MaxAMSDU size? 
	pCapELE->MaxAMSDUSize	= 0; //(MAX_RECEIVE_BUFFER_SIZE>=7935)?1:0;
	pCapELE->DssCCk 		= ((pHTInfo->bCurBW40MHz)?(pHTInfo->bCurSuppCCK?1:0):0);
	pCapELE->PSMP			= 0; // Do not support now!!
	pCapELE->LSigTxopProtect	= 0; // Do not support now!!


	//MAC HT parameters info
        // TODO: Nedd to take care of this part
	pCapELE->MaxRxAMPDUFactor 	= 3; // 2 is for 32 K and 3 is 64K
	pCapELE->MPDUDensity 		= 0;//no density

		
	//Supported MCS set
	_memcpy(pCapELE->MCS, pHTInfo->dot11HTOperationalRateSet, 16);
	
//	pCapELE->MCS[0] = (u1Byte)(HT_SUPPORTED_MCS_1SS_BITMAP);			// 1ss : MCS 0~7
//	pCapELE->MCS[1] = (u1Byte)(HT_SUPPORTED_MCS_2SS_BITMAP>>8);		// 2ss : MCS 8~15
//	pCapELE->MCS[2] = 0x0;												// 3ss 8190 does not support 3ss
//	pCapELE->MCS[3] = 0x0;												// 4ss 8190 does not support 4ss
//	pCapELE->MCS[4] = 0x1;												// MCS32

	//Extended HT Capability Info
	memset(&pCapELE->ExtHTCapInfo, 0,2);


	//TXBF Capabilities
	memset(pCapELE->TxBFCap,0, 4);

	//Antenna Selection Capabilities
	pCapELE->ASCap = 0;
	
	
}



u8 
HTConstructInfoElement(
	_adapter *padapter, 	
	PHT_INFORMATION_ELE		pHTInfoEle
)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	PRT_HIGH_THROUGHPUT	pHTInfo = &padapter->HTInfo;
	u8 length = 0;

	if(_sys_mib->mac_ctrl.opmode  & WIFI_ADHOC_STATE)
	{
		pHTInfoEle->ControlChl 			= _sys_mib->cur_channel;
		pHTInfoEle->ExtChlOffset 			= ((pHTInfo->bCurBW40MHz==_FALSE)?HT_EXTCHNL_OFFSET_NO_EXT:
											((_sys_mib->cur_channel<=6)?
												HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER));
		pHTInfoEle->RecommemdedTxWidth	= pHTInfo->bCurBW40MHz;
		pHTInfoEle->RIFS 					= 0;
		pHTInfoEle->PSMPAccessOnly		= 0;
		pHTInfoEle->SrvIntGranularity		= 0;
		pHTInfoEle->OptMode				= pHTInfo->CurrentOpMode;
		pHTInfoEle->NonGFDevPresent		= 0;
		pHTInfoEle->DualBeacon			= 0;
		pHTInfoEle->SecondaryBeacon		= 0;
		pHTInfoEle->LSigTxopProtectFull		= 0;
		pHTInfoEle->PcoActive				= 0;
		pHTInfoEle->PcoPhase				= 0;

		memset(pHTInfoEle->BasicMSC, 0, 16);


		length = 22;

	}
	else
	{
		//STA should not generate High Throughput Information Element
		length = 0;
	}	
	return length ; 

}	




#endif



//extern __inline 
// set_ie will update frame length
u8 *set_ie
(
	u8 *pbuf, 
	sint index, 
	uint len,
	u8 *source, 
	uint *frlen //frame length
)
{
_func_enter_;
	*pbuf = (u8)index;

	*(pbuf + 1) = (u8)len;

	if (len > 0)
		_memcpy((void *)(pbuf + 2), (void *)source, len);
	
	*frlen = *frlen + (len + 2);
	
	return (pbuf + len + 2);
_func_exit_;	
}



/*----------------------------------------------------------------------------
index: the information element id index, limit is the limit for search
-----------------------------------------------------------------------------*/
u8 *get_ie(u8 *pbuf, sint index, sint *len, sint limit)
{
	sint tmp,i;
	u8 *p;
_func_enter_;
	if (limit < 1){
		_func_exit_;	
		return NULL;
	}

	p = pbuf;
	i = 0;
	*len = 0;
	while(1)
	{
		if (*p == index)
		{
			*len = *(p + 1);
			return (p);
		}
		else
		{
			tmp = *(p + 1);
			p += (tmp + 2);
			i += (tmp + 2);
		}
		if (i >= limit)
			break;
	}
_func_exit_;		
	return NULL;
}


//extern __inline 
void set_supported_rate(u8* SupportedRates, uint mode) 
{
_func_enter_;
	memset(SupportedRates, 0, NDIS_802_11_LENGTH_RATES_EX);
	
	switch (mode)
	{
		case WIRELESS_11B:
			_memcpy(SupportedRates, WIFI_CCKRATES, IEEE80211_CCK_RATE_LEN);
			break;
		
		case WIRELESS_11G:
		case WIRELESS_11A:	
			_memcpy(SupportedRates, WIFI_OFDMRATES, IEEE80211_NUM_OFDM_RATESLEN);
			break;
		
		case WIRELESS_11BG:
			_memcpy(SupportedRates, WIFI_CCKRATES, IEEE80211_CCK_RATE_LEN);
			_memcpy(SupportedRates + IEEE80211_CCK_RATE_LEN, WIFI_OFDMRATES, IEEE80211_NUM_OFDM_RATESLEN);
			break;
	
	}
_func_exit_;	
}


//extern __inline 
uint	get_rateset_len(u8	*rateset)
{
	uint i = 0;
_func_enter_;	
	while(1)
	{
		if ((rateset[i]) == 0)
			break;
			
		if (i > 12)
			break;
			
		i++;			
	}
_func_exit_;		
	return i;
}

//extern __inline
uint generate_ie(struct registry_priv *pregistrypriv)
{
	uint 		sz = 0, rateLen;

	NDIS_WLAN_BSSID_EX*	pdev_network = &pregistrypriv->dev_network;
	u8*	ie = pdev_network->IEs;
_func_enter_;		
	//timestamp will be inserted by hardware
	sz += 8;	
	ie += sz;
	
	//beacon interval : 2bytes
	*(u16*)ie = (u16)pdev_network->Configuration.BeaconPeriod;//BCN_INTERVAL;
	sz += 2; 
	ie += 2;
	
	//capability info
	*(u16*)ie = 0;
	
	*(u16*)ie |= cap_IBSS;

	if(pregistrypriv->preamble == PREAMBLE_SHORT)
		*(u16*)ie |= cap_ShortPremble;
	
	if (pdev_network->Privacy)
		*(u16*)ie |= cap_Privacy;
	
	sz += 2;
	ie += 2;
	
	//SSID
	ie = set_ie(ie, _SSID_IE_, pdev_network->Ssid.SsidLength, pdev_network->Ssid.Ssid, &sz);
	
	//supported rates
	set_supported_rate(pdev_network->SupportedRates, pregistrypriv->wireless_mode) ;
	
	rateLen = get_rateset_len(pdev_network->SupportedRates);

	if (rateLen > 8)
	{
		ie = set_ie(ie, _SUPPORTEDRATES_IE_, 8, pdev_network->SupportedRates, &sz);
		ie = set_ie(ie, _EXT_SUPPORTEDRATES_IE_, (rateLen - 8), (pdev_network->SupportedRates + 8), &sz);
	}
	else
	{
		ie = set_ie(ie, _SUPPORTEDRATES_IE_, rateLen, pdev_network->SupportedRates, &sz);
	}

	//DS parameter set
	ie = set_ie(ie, _DSSET_IE_, 1, (u8 *)&(pdev_network->Configuration.DSConfig), &sz);


	//IBSS Parameter Set
	
	ie = set_ie(ie, _IBSS_PARA_IE_, 2, (u8 *)&(pdev_network->Configuration.ATIMWindow), &sz);

	pdev_network->IELength =  sz; //update IELength by yt; 2061107

_func_exit_;			
	return _SUCCESS;
	
}




