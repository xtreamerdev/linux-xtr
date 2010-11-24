
#define  _RTL871X_IOCTL_LINUX_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <wlan_bssdef.h>
#include <rtl871x_debug.h>
#include "wifi.h"
#include <rtl871x_mlme.h>
#include <rtl871x_ioctl.h>
#include <rtl871x_ioctl_set.h>
#include <rtl871x_ioctl_query.h>
#ifdef CONFIG_MP_INCLUDED
#include <rtl871x_mp_ioctl.h>
#endif
#include <linux/wireless.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/semaphore.h>
#include <net/iw_handler.h>
#include <linux/if_arp.h>




#define MAX_CUSTOM_LEN 64
#define RATE_COUNT 4
#define RTL_IOCTL_WPA_SUPPLICANT		SIOCIWFIRSTPRIV+30
#define RSN_HEADER_LEN 4
#define RSN_SELECTOR_LEN 4
static const u8 RSN_CIPHER_SUITE_NONE[] = { 0x00, 0x0f, 0xac, 0 };
static const u8 RSN_CIPHER_SUITE_WEP40[] = { 0x00, 0x0f, 0xac, 1 };
static const u8 RSN_CIPHER_SUITE_TKIP[] = { 0x00, 0x0f, 0xac, 2 };
static const u8 RSN_CIPHER_SUITE_WRAP[] = { 0x00, 0x0f, 0xac, 3 };
static const u8 RSN_CIPHER_SUITE_CCMP[] = { 0x00, 0x0f, 0xac, 4 };
static const u8 RSN_CIPHER_SUITE_WEP104[] = { 0x00, 0x0f, 0xac, 5 };
#define WPA_CIPHER_NONE 0
#define WPA_CIPHER_WEP40 1
#define WPA_CIPHER_WEP104 2
#define WPA_CIPHER_TKIP 3
#define WPA_CIPHER_CCMP 4
#define AUTH_ALG_OPEN_SYSTEM			0x1
#define AUTH_ALG_SHARED_KEY			0x2
#define IEEE_PARAM_IEEE_802_1X			6
#define IEEE_PARAM_WPAX_SELECT			7


u32 rtl8180_rates[] = {1000000,2000000,5500000,11000000,
	6000000,9000000,12000000,18000000,24000000,36000000,48000000,54000000};

const long ieee80211_wlan_frequencies[] = {  
	2412, 2417, 2422, 2427, 
	2432, 2437, 2442, 2447, 
	2452, 2457, 2462, 2467, 
	2472, 2484  
};
const char * const iw_operation_mode[] = { "Auto",
					"Ad-Hoc",
					"Managed",
					"Master",
					"Repeater",
					"Secondary",
					"Monitor" };

 int r8711_wx_set_scan(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	u8 _status;
	
	_adapter *padapter = (_adapter *)netdev_priv(dev);
	
	
	int ret = 0;
	
	DEBUG_INFO(("r8711_wx_set_scan\n"));
	_down_sema(&padapter->mlmepriv.ioctl_sema);

	_func_enter_;
	
	if(padapter->bDriverStopped){
           printk("bDriverStopped=%d\n", padapter->bDriverStopped);
		ret= -1;
		goto exit;
	}
	
	if(!padapter->bup){
		ret = -1;
		goto exit;

	}
	_status = set_802_11_bssid_list_scan(padapter);
	
	if(_status == _FALSE)
		ret = -1;
exit:		
	_func_exit_;
	
	DEBUG_INFO(("-r8711_wx_set_scan\n"));
	return ret;
}






uint	is_cckrates_included(u8 *rate)
{	
		u32	i = 0;			

		while(rate[i]!=0)
		{		
			if  (  (((rate[i]) & 0x7f) == 2)	|| (((rate[i]) & 0x7f) == 4) ||		
			(((rate[i]) & 0x7f) == 11)  || (((rate[i]) & 0x7f) == 22) )		
			return _TRUE;	
			i++;
		}
		
		return _FALSE;
}

uint	is_cckratesonly_included(u8 *rate)
{
	u32 i = 0;


	while(rate[i]!=0)
	{
			if  (  (((rate[i]) & 0x7f) != 2) && (((rate[i]) & 0x7f) != 4) &&
				(((rate[i]) & 0x7f) != 11)  && (((rate[i]) & 0x7f) != 22) )

			return _FALSE;		

			i++;
	}
	
	return _TRUE;

}

sint get_sec_ie(u8 *in_ie,uint in_len,u8 *rsn_ie,u8 *rsn_len,u8 *wpa_ie,u8 *wpa_len)
{
	u8 authmode,match,sec_idx,i;
	u32 cnt;
//	u8 sec_ie[255],uncst_oui[4],bkup_ie[255];
	u8 wpa_oui[4]={0x0,0x50,0xf2,0x01};
//	uint 	ielength;

	
_func_enter_;
	
	
	//4 	Search required WPA or WPA2 IE and copy to sec_ie[ ]
	cnt=12;
	sec_idx=0;
	match=_FALSE;
	while(cnt<in_len)
	{
		authmode=in_ie[cnt];
		if((authmode==_WPA_IE_ID_)&&(_memcmp(&in_ie[cnt+2], &wpa_oui[0],4)==_TRUE))
		{
			DEBUG_INFO(("\n get_wpa_ie: sec_idx=%d in_ie[cnt+1]+2=%d\n",sec_idx,in_ie[cnt+1]+2));		
			_memcpy(wpa_ie, &in_ie[cnt],in_ie[cnt+1]+2);
			for(i=0;i<(in_ie[cnt+1]+2);i=i+8)
			{
				DEBUG_INFO(("\n %2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x\n",wpa_ie[i],wpa_ie[i+1],wpa_ie[i+2],wpa_ie[i+3],wpa_ie[i+4],wpa_ie[i+5],wpa_ie[i+6],wpa_ie[i+7]));
			}
			*wpa_len=in_ie[cnt+1]+2;
			cnt+=in_ie[cnt+1]+2;  //get next
		}
		else
		{
			if(authmode==_WPA2_IE_ID_)
			{
				DEBUG_INFO(("\n get_rsn_ie: sec_idx=%d in_ie[cnt+1]+2=%d\n",sec_idx,in_ie[cnt+1]+2));		
				_memcpy(rsn_ie, &in_ie[cnt],in_ie[cnt+1]+2);
				for(i=0;i<(in_ie[cnt+1]+2);i=i+8)
				{
					DEBUG_INFO(("\n %2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x\n",rsn_ie[i],rsn_ie[i+1],rsn_ie[i+2],rsn_ie[i+3],rsn_ie[i+4],rsn_ie[i+5],rsn_ie[i+6],rsn_ie[i+7]));
				}
				*rsn_len=in_ie[cnt+1]+2;
				cnt+=in_ie[cnt+1]+2;  //get next
			}
			else
			{
				DEBUG_INFO(("get_wpa_ie: not WPA nor WPA2\n"));	
				cnt+=in_ie[cnt+1]+2;   //get next
			}
		}
	}
	
_func_exit_;
	return (*rsn_len+*wpa_len);
}

static inline u8 *translate_scan(  struct wlan_network *pnetwork,
				u8 *start, u8 *stop  )
{
	struct iw_event iwe;
	u16 cap;
	char custom[MAX_CUSTOM_LEN];
	char *p;
	u8 max_rate, rate;
	u32 i = 0;	

	/*  AP MAC address  */
	iwe.cmd = SIOCGIWAP;
	iwe.u.ap_addr.sa_family = ARPHRD_ETHER;

	_memcpy(iwe.u.ap_addr.sa_data, pnetwork->network.MacAddress, ETH_ALEN);
	start = iwe_stream_add_event(start, stop, &iwe, IW_EV_ADDR_LEN);

	/* Add the ESSID */
	iwe.cmd = SIOCGIWESSID;
	iwe.u.data.flags = 1;

	
	iwe.u.data.length = min((u16)pnetwork->network.Ssid.SsidLength, (u16)32);
	start = iwe_stream_add_point(start, stop, &iwe, pnetwork->network.Ssid.Ssid);
	

	/* Add the protocol name */
	iwe.cmd = SIOCGIWNAME;

	if ((is_cckratesonly_included((u8*)&pnetwork->network.SupportedRates)) == _TRUE)		
		snprintf(iwe.u.name, IFNAMSIZ, "IEEE 802.11b");
	else if ((is_cckrates_included((u8*)&pnetwork->network.SupportedRates)) == _TRUE)	
		snprintf(iwe.u.name, IFNAMSIZ, "IEEE 802.11bg");
	else
		snprintf(iwe.u.name, IFNAMSIZ, "IEEE 802.11g");

	start = iwe_stream_add_event(start, stop, &iwe, IW_EV_CHAR_LEN);



	  /* Add mode */
        iwe.cmd = SIOCGIWMODE;
	_memcpy((u8 *)&cap, get_capability_from_ie(pnetwork->network.IEs), 2);

	if(cap & (WLAN_CAPABILITY_IBSS |WLAN_CAPABILITY_BSS)){
		if (cap & WLAN_CAPABILITY_BSS)
			iwe.u.mode = IW_MODE_MASTER;
		else
			iwe.u.mode = IW_MODE_ADHOC;

		start = iwe_stream_add_event(start, stop, &iwe,
					     IW_EV_UINT_LEN);

	}

	/*
  	if(pnetwork->network.InfrastructureMode == Ndis802_11IBSS)
		iwe.u.mode = IW_MODE_ADHOC;
	else if(pnetwork->network.InfrastructureMode == Ndis802_11Infrastructure)
		iwe.u.mode = IW_MODE_MASTER;
	else if (pnetwork->network.InfrastructureMode == Ndis802_11AutoUnknown)
		iwe.u.mode = IW_MODE_AUTO;
            start = iwe_stream_add_event(start, stop, &iwe, IW_EV_UINT_LEN);
	*/

	 /* Add frequency/channel */
	iwe.cmd = SIOCGIWFREQ;

	iwe.u.freq.m = pnetwork->network.Configuration.DSConfig;
	iwe.u.freq.e = 0;
	iwe.u.freq.i = 0;
	start = iwe_stream_add_event(start, stop, &iwe, IW_EV_FREQ_LEN);


	/* Add encryption capability */
	iwe.cmd = SIOCGIWENCODE;
	if (cap & WLAN_CAPABILITY_PRIVACY)
		iwe.u.data.flags = IW_ENCODE_ENABLED | IW_ENCODE_NOKEY;
	else
		iwe.u.data.flags = IW_ENCODE_DISABLED;
	iwe.u.data.length = 0;
	start = iwe_stream_add_point(start, stop, &iwe, pnetwork->network.Ssid.Ssid);

	
	/*Add basic and extended rates */
	max_rate = 0;
	p = custom;
	p += snprintf(p, MAX_CUSTOM_LEN - (p - custom), " Rates (Mb/s): ");
	while(pnetwork->network.SupportedRates[i]!=0)
	{
		rate = pnetwork->network.SupportedRates[i]&0x7F; 
		if (rate > max_rate)
			max_rate = rate;
		p += snprintf(p, MAX_CUSTOM_LEN - (p - custom),
			      "%d%s ", rate >> 1, (rate & 1) ? ".5" : "");
		i++;
	}
	

	iwe.cmd = SIOCGIWRATE;
	iwe.u.bitrate.fixed = iwe.u.bitrate.disabled = 0;
	iwe.u.bitrate.value = max_rate * 500000;
	start = iwe_stream_add_event(start, stop, &iwe,
				     IW_EV_PARAM_LEN);
	
	{
		u8 buf[MAX_WPA_IE_LEN * 2 + 30];
		u8 wpa_ie[255],rsn_ie[255],wpa_len=0,rsn_len=0;
		u8 *p;
		sint out_len=0;
		out_len=get_sec_ie(pnetwork->network.IEs ,pnetwork->network.IELength,rsn_ie,&rsn_len,wpa_ie,&wpa_len);

		DEBUG_INFO(("\n r8711_wx_get_scan:ssid=%s",pnetwork->network.Ssid.Ssid));
		DEBUG_INFO(("\n r8711_wx_get_scan:wpa_len=%d rsn_len=%d\n",wpa_len,rsn_len));

		if(wpa_len>0){
			p=buf;
			p += sprintf(p, "wpa_ie=");
			for (i = 0; i < wpa_len; i++) {
				p += sprintf(p, "%02x", wpa_ie[i]);
			}
	
			memset(&iwe, 0, sizeof(iwe));
			iwe.cmd = IWEVCUSTOM;
			iwe.u.data.length = strlen(buf);

			start = iwe_stream_add_point(start, stop, &iwe,buf);
		}
		if(rsn_len>0){
			p = buf;
			p += sprintf(p, "rsn_ie=");
			for (i = 0; i < rsn_len; i++) {
				p += sprintf(p, "%02x", rsn_ie[i]);
			}
			memset(&iwe, 0, sizeof(iwe));
			iwe.cmd = IWEVCUSTOM;
			iwe.u.data.length = strlen(buf);

			start = iwe_stream_add_point(start, stop, &iwe,buf);
		}
	}


	iwe.cmd = IWEVCUSTOM;
	iwe.u.data.length = p - custom;
	if (iwe.u.data.length)
		start = iwe_stream_add_point(start, stop, &iwe, custom);

	/* Add quality statistics */
	iwe.cmd = IWEVQUAL;
	iwe.u.qual.level =(u8) pnetwork->network.Rssi;
	start = iwe_stream_add_event(start, stop, &iwe, IW_EV_QUAL_LEN);
	
	return start;
}


#define SCAN_ITEM_SIZE 256

static int r8711_wx_get_scan(struct net_device *dev,
								struct iw_request_info *a,
		     						union iwreq_data *wrqu,
		     						char* extra)
{
	_irqL	irqL;
	_list					*plist, *phead;
	_adapter *padapter = (_adapter *)netdev_priv(dev);
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);	
	_queue				*queue	= &(pmlmepriv->scanned_queue);	
	struct	wlan_network	*pnetwork = NULL;
	char *ev = extra;
	char *stop = ev + wrqu->data.length;
	u32 ret = 0;	
	u32 cnt=0;

	_down_sema(&padapter->mlmepriv.ioctl_sema);
	
	DEBUG_INFO(("+r8711_wx_get_scan\n"));
	DEBUG_INFO((" Start of Query SIOCGIWSCAN .\n"));
	_func_enter_;
	
	if(padapter->bDriverStopped){                
		ret= -1;
		goto exit;
	}

 	while((check_fwstate(pmlmepriv, (_FW_UNDER_SURVEY|_FW_UNDER_LINKING))) == _TRUE)
	{
		DEBUG_ERR(("r8711_wx_get_scan: while loop sleep 30ms\n"));
		
		sleep_schedulable(30);// 30 msec
		cnt++;
		if(cnt > 100)//3 sec
			break;
	}

	_enter_critical(&(pmlmepriv->scanned_queue.lock), &irqL);

	phead = get_list_head(queue);
	plist = get_next(phead);
       
	while(1)
	{
		if (end_of_queue_search(phead,plist)== _TRUE)
		{
			DEBUG_ERR(("r8711_wx_get_scan: break while loop(end_of_queue_search)\n"));
			break;
		}

		if (stop - ev < SCAN_ITEM_SIZE) 
		{
			ret = -E2BIG;
			DEBUG_ERR(("r8711_wx_get_scan: break while loop(stop - ev < SCAN_ITEM_SIZE)\n"));
		
			break;
		}

		pnetwork = LIST_CONTAINOR(plist, struct wlan_network, list);

		ev=translate_scan(pnetwork,ev,stop);

		plist = get_next(plist);
	}

	_exit_critical(&(pmlmepriv->scanned_queue.lock), &irqL);
	wrqu->data.length = ev-extra;
	wrqu->data.flags = 0;

exit:		
	_up_sema(&padapter->mlmepriv.ioctl_sema);
	DEBUG_INFO(("-r8711_wx_get_scan\n"));	
	_func_exit_;	
	return ret ;
}



static int r8711_wx_get_name(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	_adapter *padapter = (_adapter *)netdev_priv(dev);
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	NDIS_WLAN_BSSID_EX  *pcur_bss = &pmlmepriv->cur_network.network;
	NDIS_802_11_RATES_EX* prates = NULL;
	DEBUG_INFO(("cmd_code=%x\n", info->cmd));
	_func_enter_;
	
 	
	if ( (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) ||
                    (check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))
			prates = &pcur_bss->SupportedRates;
		else
			prates = &padapter->registrypriv.dev_network.SupportedRates;
		
        
	if ((is_cckratesonly_included((u8*)prates)) == _TRUE)		
		snprintf(wrqu->name, IFNAMSIZ, "IEEE 802.11b");
	else if ((is_cckrates_included((u8*)prates)) == _TRUE)	
		snprintf(wrqu->name, IFNAMSIZ, "IEEE 802.11bg");
	else
		snprintf(wrqu->name, IFNAMSIZ, "IEEE 802.11g");

	_func_exit_;
	return 0;
}






static int r8711_wx_get_wap(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *extra)
{

	_adapter *padapter = (_adapter *)netdev_priv(dev);
	
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	NDIS_WLAN_BSSID_EX  *pcur_bss = &pmlmepriv->cur_network.network;	
	wrqu->ap_addr.sa_family = ARPHRD_ETHER;
	memset(wrqu->ap_addr.sa_data, 0, ETH_ALEN);
	DEBUG_INFO(("r8711_wx_get_wap\n"));

	_func_enter_;

	if  ( ((check_fwstate(pmlmepriv, _FW_LINKED)) == _TRUE) || 
			((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)) == _TRUE) ||
			((check_fwstate(pmlmepriv, WIFI_AP_STATE)) == _TRUE) )
		{

			_memcpy(wrqu->ap_addr.sa_data, pcur_bss->MacAddress, ETH_ALEN);
	}
	else
		 	memset(wrqu->ap_addr.sa_data, 0, ETH_ALEN);

	_func_exit_;
	return 0;
}



static int r8711_wx_get_mode(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	_adapter *padapter = (_adapter *)netdev_priv(dev);
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	DEBUG_INFO((" r8711_wx_get_mode \n"));

	_func_enter_;
	
	if (check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE)
	{
		wrqu->mode = IW_MODE_INFRA;
	}
	else if  ((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		       (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE))
		
	{
		wrqu->mode = IW_MODE_ADHOC;
	}
	else
	{
		wrqu->mode = IW_MODE_AUTO;
	}

	_func_exit_;
	return 0;
}


static int r8711_wx_get_essid(struct net_device *dev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *extra)
{
	u32 len,ret = 0;
	_adapter *padapter = (_adapter *)netdev_priv(dev);
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	NDIS_WLAN_BSSID_EX  *pcur_bss = &pmlmepriv->cur_network.network;

	DEBUG_INFO(("r8711_wx_get_essid\n"));

	_func_enter_;

		if ( (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) ||
		      (check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))

		{
			len = pcur_bss->Ssid.SsidLength;

			wrqu->essid.length = len;
			
			_memcpy(extra,pcur_bss->Ssid.Ssid,len);

			wrqu->essid.flags = 1;
		}
		else
		{
			ret = -1;
			goto exit;
		}

exit:
	_func_exit_;
	
	return ret;
}


static int r8711_wx_set_essid(struct net_device *dev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *extra)
{
	_adapter *padapter = (_adapter *)netdev_priv(dev);
	uint ret = 0,len;
	NDIS_802_11_SSID ndis_ssid;
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	_list	*phead;
	u8 *dst_ssid, *src_ssid;
	NDIS_802_11_AUTHENTICATION_MODE	authmode;
	_queue	*queue	= &(pmlmepriv->scanned_queue);
	struct	wlan_network	*pnetwork = NULL;

	_func_enter_;

	DEBUG_INFO(("r8711_wx_set_essid\n"));	
	
	if(!padapter->bup){
		ret = -1;
		goto exit;

	}

	if (wrqu->essid.length > IW_ESSID_MAX_SIZE){
		ret= -E2BIG;
		goto exit;
	}
	
	
	authmode = padapter->securitypriv.ndisauthtype;
	if (wrqu->essid.flags && wrqu->essid.length)
	{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
		len = ((wrqu->essid.length-1) < IW_ESSID_MAX_SIZE) ? (wrqu->essid.length-1) : IW_ESSID_MAX_SIZE;
#else
		len = (wrqu->essid.length < IW_ESSID_MAX_SIZE) ? wrqu->essid.length : IW_ESSID_MAX_SIZE;
#endif

		memset(&ndis_ssid, 0, sizeof(NDIS_802_11_SSID));

		ndis_ssid.SsidLength = len;

		_memcpy(ndis_ssid.Ssid,extra, len);
		
		
	        phead = get_list_head(queue);
                pmlmepriv->pscanned = get_next(phead);

		while (1)
		 {
			
			if ((end_of_queue_search(phead, pmlmepriv->pscanned)) == _TRUE)
			{
				if(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE)
				{
	            			set_802_11_ssid(padapter, &ndis_ssid);

		    			goto exit;                    
				}
				else
				{	

					ret = -EINVAL;
					goto exit;
				}
			
			}
	
			pnetwork = LIST_CONTAINOR(pmlmepriv->pscanned, struct wlan_network, list);

			pmlmepriv->pscanned = get_next(pmlmepriv->pscanned);

			dst_ssid = pnetwork->network.Ssid.Ssid;

			src_ssid = ndis_ssid.Ssid;

			if (((_memcmp(dst_ssid, src_ssid, ndis_ssid.SsidLength)) == _TRUE)&&
				(pnetwork->network.Ssid.SsidLength==ndis_ssid.SsidLength))
			{
				
				if(!set_802_11_infrastructure_mode(padapter,pnetwork->network.InfrastructureMode))
				{
					ret = -1;
					goto exit;
				}

				break;			
			}

		}

		set_802_11_authenticaion_mode( padapter, authmode );
		if(!set_802_11_ssid(padapter, &ndis_ssid))
		{
			ret = -1;
			goto exit;
		}
	
	}

			

exit:
	_func_exit_;
	
	return ret;
}





static int r8711_wx_get_range(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct iw_range *range = (struct iw_range *)extra;
//	_adapter *padapter = (_adapter *)netdev_priv(dev);

	u16 val;
	int i;
	
	_func_enter_;
	
	DEBUG_INFO(("r8711_wx_get_range. cmd_code=%x\n", info->cmd));

	wrqu->data.length = sizeof(*range);
	memset(range, 0, sizeof(*range));

	/* Let's try to keep this struct in the same order as in
	 * linux/include/wireless.h
	 */
	
	/* TODO: See what values we can set, and remove the ones we can't
	 * set, or fill them with some default data.
	 */

	/* ~5 Mb/s real (802.11b) */
	range->throughput = 5 * 1000 * 1000;     

	// TODO: Not used in 802.11b?
//	range->min_nwid;	/* Minimal NWID we are able to set */
	// TODO: Not used in 802.11b?
//	range->max_nwid;	/* Maximal NWID we are able to set */
	
        /* Old Frequency (backward compat - moved lower ) */
//	range->old_num_channels; 
//	range->old_num_frequency;
//	range->old_freq[6]; /* Filler to keep "version" at the same offset */


	// TODO: 8711 sensitivity ?
	/* signal level threshold range */


	
	range->max_qual.qual = 100;
	/* TODO: Find real max RSSI and stick here */
	range->max_qual.level = 0;
	range->max_qual.noise = -98;
	range->max_qual.updated = 7; /* Updated all three */

	range->avg_qual.qual = 92; /* > 8% missed beacons is 'bad' */
	/* TODO: Find real 'good' to 'bad' threshol value for RSSI */
	range->avg_qual.level = 20 + -98;
	range->avg_qual.noise = 0;
	range->avg_qual.updated = 7; /* Updated all three */

	range->num_bitrates = RATE_COUNT;
	
	for (i = 0; i < RATE_COUNT && i < IW_MAX_BITRATES; i++) {
		range->bitrate[i] = rtl8180_rates[i];
	}
	
	range->min_frag = MIN_FRAG_THRESHOLD;
	range->max_frag = MAX_FRAG_THRESHOLD;
	
	range->pm_capa = 0;

	range->we_version_compiled = WIRELESS_EXT;
	range->we_version_source = 16;

//	range->retry_capa;	/* What retry options are supported */
//	range->retry_flags;	/* How to decode max/min retry limit */
//	range->r_time_flags;	/* How to decode max/min retry life */
//	range->min_retry;	/* Minimal number of retries */
//	range->max_retry;	/* Maximal number of retries */
//	range->min_r_time;	/* Minimal retry lifetime */
//	range->max_r_time;	/* Maximal retry lifetime */

        range->num_channels = 14;

	for (i = 0, val = 0; i < 14; i++) {
		
		// Include only legal frequencies for some countries
		//if ((priv->challow)[i+1]) {
		        range->freq[val].i = i + 1;
			range->freq[val].m = ieee80211_wlan_frequencies[i] * 100000;
			range->freq[val].e = 1;
			val++;
		//} else {
			// FIXME: do we need to set anything for channels
			// we don't use ?
		//}
		
		if (val == IW_MAX_FREQUENCIES)
		break;
	}

	range->num_frequency = val;
	_func_exit_;
	return 0;
}




static int r8711_wx_set_mode(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	_adapter *padapter = (_adapter *)netdev_priv(dev);
	NDIS_802_11_NETWORK_INFRASTRUCTURE networkType ;
	int ret = 0;
	_func_enter_;

	switch(wrqu->mode)
		{
			case IW_MODE_AUTO:
				networkType = Ndis802_11AutoUnknown;
				break;
				
			case IW_MODE_ADHOC:
			case IW_MODE_MASTER:
				networkType = Ndis802_11IBSS;
				break;
				
			case IW_MODE_INFRA:
				networkType = Ndis802_11Infrastructure;
				break;
	
			default :
				ret = -EINVAL;;
				DEBUG_ERR(("\n Mode: %s is not supported  \n", iw_operation_mode[wrqu->mode]));
				goto exit;
		}
	
       if (set_802_11_infrastructure_mode(padapter, networkType) ==_FALSE){

		ret = -1;
		goto exit;

	   }
	
	
exit:
	_func_exit_;
	return ret;
}



static int r8711_wx_set_frag(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	_adapter *padapter = netdev_priv(dev);

	_func_enter_;
	
	if (wrqu->frag.disabled)
		padapter->xmitpriv.frag_len = MAX_FRAG_THRESHOLD;
	else {
		if (wrqu->frag.value < MIN_FRAG_THRESHOLD ||
		    wrqu->frag.value > MAX_FRAG_THRESHOLD)
			return -EINVAL;
		
		padapter->xmitpriv.frag_len = wrqu->frag.value & ~0x1;
	}
	_func_exit_;
	return 0;
}


static int r8711_wx_get_frag(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	_adapter *padapter = netdev_priv(dev);
	_func_enter_;
	wrqu->frag.value = padapter->xmitpriv.frag_len;
	wrqu->frag.fixed = 0;	/* no auto select */
	//wrqu->frag.disabled = (wrqu->frag.value == DEFAULT_FRAG_THRESHOLD);
	_func_exit_;
	return 0;
}



static int r8711_wx_set_wap(struct net_device *dev,
			 struct iw_request_info *info,
			 union iwreq_data *awrq,
			 char *extra)
{
	uint ret = 0;
	_adapter *padapter = netdev_priv(dev);
	struct sockaddr *temp = (struct sockaddr *)awrq;
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	_list	*phead;
	u8 *dst_bssid, *src_bssid;
	_queue	*queue	= &(pmlmepriv->scanned_queue);
	struct	wlan_network	*pnetwork = NULL;
	NDIS_802_11_AUTHENTICATION_MODE	authmode;


	_func_enter_;
	
	if(!padapter->bup){
		ret = -1;
		goto exit;

	}

	
	if (temp->sa_family != ARPHRD_ETHER){
		ret = -EINVAL;
		goto exit;
	}

		authmode = padapter->securitypriv.ndisauthtype;

                phead = get_list_head(queue);
                pmlmepriv->pscanned = get_next(phead);

		while (1)
		 {
			
			if ((end_of_queue_search(phead, pmlmepriv->pscanned)) == _TRUE)
			{
				ret = -EINVAL;
			goto exit;
			
			}
	
			pnetwork = LIST_CONTAINOR(pmlmepriv->pscanned, struct wlan_network, list);

			pmlmepriv->pscanned = get_next(pmlmepriv->pscanned);

			dst_bssid = pnetwork->network.MacAddress;

			src_bssid = temp->sa_data;

			if ((_memcmp(dst_bssid, src_bssid, ETH_ALEN)) == _TRUE)
			{
			
				if(!set_802_11_infrastructure_mode(padapter,pnetwork->network.InfrastructureMode))
				{
					ret = -1;
					goto exit;
				}

				break;			
			}

		}
	set_802_11_authenticaion_mode( padapter, authmode );
	if(!set_802_11_bssid(padapter, temp->sa_data))
	{
		ret = -1;
		goto exit;
		
	}
	
	
	
exit:
	_func_exit_;
	return ret;
	
}

static int r8711_wx_set_enc(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *keybuf)
{
	struct iw_point *erq = &(wrqu->encoding);
	_adapter *padapter = netdev_priv(dev);
	u32 keyindex_provided;
	u32 key,ret = 0;
	NDIS_802_11_WEP	*pwep;
	NDIS_802_11_AUTHENTICATION_MODE authmode;
	pwep=(NDIS_802_11_WEP *)_malloc(sizeof(NDIS_802_11_WEP)+13);
	memset(pwep, 0, sizeof(NDIS_802_11_WEP)+13);
	key = erq->flags & IW_ENCODE_INDEX;

	if (key) 
	{
		if (key > WEP_KEYS)
		{
			printk("r8711_wx_set_enc: key > WEP_KEYS\n");
			return -EINVAL;
		}
		key--;
		keyindex_provided = 1;
	} 
	else
	{
		keyindex_provided = 0;
		key = padapter->securitypriv.dot11PrivacyKeyIndex;
	}
	
	_func_enter_;
	

	if (erq->flags & IW_ENCODE_DISABLED) 
	{
		DEBUG_ERR( ("EncryptionDisabled\n"));
		padapter->securitypriv.ndisencryptstatus = Ndis802_11EncryptionDisabled;
		padapter->securitypriv.dot11PrivacyAlgrthm=_NO_PRIVACY_;
		padapter->securitypriv.dot118021XGrpPrivacy=_NO_PRIVACY_;
		padapter->securitypriv.dot11AuthAlgrthm= 0; //open system
  		authmode = Ndis802_11AuthModeOpen;
		padapter->securitypriv.ndisauthtype=authmode;
     		
		goto exit;
	}

	//set authentication mode
	
	if(erq->flags & IW_ENCODE_RESTRICTED){
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
		padapter->securitypriv.dot11AuthAlgrthm= 1; //shared system
		padapter->securitypriv.dot11PrivacyAlgrthm=_WEP40_;
		padapter->securitypriv.dot118021XGrpPrivacy=_WEP40_;
		
		authmode = Ndis802_11AuthModeShared;
		padapter->securitypriv.ndisauthtype=authmode;
	}
	else{
		padapter->securitypriv.ndisencryptstatus = Ndis802_11EncryptionDisabled;
		padapter->securitypriv.dot11AuthAlgrthm= 0; //open system
		padapter->securitypriv.dot11PrivacyAlgrthm=_NO_PRIVACY_;
		padapter->securitypriv.dot118021XGrpPrivacy=_NO_PRIVACY_;
  		authmode = Ndis802_11AuthModeOpen;
		padapter->securitypriv.ndisauthtype=authmode;
	}
	
	pwep->KeyIndex = key;
	if (erq->length > 0) 
	{
		pwep->KeyLength = erq->length <= 5 ? 5 : 13;
		if (pwep->KeyLength == 5)
					padapter->securitypriv.dot11PrivacyAlgrthm=_WEP40_;
		if (pwep->KeyLength == 13)
					padapter->securitypriv.dot11PrivacyAlgrthm=_WEP104_;

	}
	else 
	{
		if(keyindex_provided == 1)
		{
			padapter->securitypriv.dot11PrivacyKeyIndex = key;
			goto exit;

		}
	}
	_memcpy(pwep->KeyMaterial, keybuf,pwep->KeyLength);
	
	if(set_802_11_add_wep(padapter, pwep) == _FAIL)
	{
		ret = -EOPNOTSUPP ;
		goto exit;
	}

exit:
	_mfree((u8*)pwep, sizeof(NDIS_802_11_WEP)+13);
	_func_exit_;
	return ret;
}


static int r8711_wx_get_enc(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *keybuf)
{
	uint key, ret =0;
	_adapter *padapter = netdev_priv(dev);
	struct iw_point *erq = &(wrqu->encoding);

	_func_enter_;
	key = erq->flags & IW_ENCODE_INDEX;

	if (key) {
		if (key > WEP_KEYS)
			return -EINVAL;
		key--;
	} else
		key = padapter->securitypriv.dot11PrivacyKeyIndex;

	erq->flags = key + 1;

	if(padapter->securitypriv.ndisauthtype == Ndis802_11AuthModeOpen)
	      erq->flags |= IW_ENCODE_OPEN;
		
	
	switch(padapter->securitypriv.ndisencryptstatus) {

		case Ndis802_11EncryptionNotSupported:
		case Ndis802_11EncryptionDisabled:

		erq->length = 0;
		erq->flags |= IW_ENCODE_DISABLED;
	
		break;
		
		case Ndis802_11Encryption1Enabled:					
		//TODO: Enable WEP
		erq->flags |= IW_ENCODE_ENABLED;
		erq->flags |= IW_ENCODE_RESTRICTED;


		erq->length = padapter->securitypriv.dot11DefKeylen[key];

		_memcpy(keybuf,padapter->securitypriv.dot11DefKey[key].skey,padapter->securitypriv.dot11DefKeylen[key]);

		break;
	

		default:
		erq->length = 0;
		erq->flags |= IW_ENCODE_DISABLED;

		break;
	}
	_func_exit_;
	return ret;
	
}

static int r8711_wx_readreg(struct net_device *dev,
                            struct iw_request_info *info,
                            union iwreq_data *wrqu, char *keybuf)
{
     //   uint key, ret =0;
        _adapter *padapter = netdev_priv(dev);
     //   struct iw_point *erq = &(wrqu->encoding);

        u16 addr, leng;
        u32 data = 0;
		
	addr = wrqu->data.flags;
	leng = wrqu->data.length;
	printk("addr=%x, leng=%x\n", addr, leng);
	switch(leng)
	{
		case 8:
			data = read8(padapter, addr);
			printk("read8=%x\n", data);
			//put_user(data, (u32*)wrqu->data.pointer);
			break;
		case 16:
			data = read16(padapter, addr);			
			printk("read16=%x\n", data);
			break;
		case 32:
			data = read32(padapter, addr);			
			printk("read32=%x\n", data);
			break;
		default:
			break;			
	}
	//data = read32(padapter, addr);
	put_user(data, (u32*)wrqu->data.pointer);
        return 0;

}

#if 0
static int r8711_wx_write8(struct net_device *dev,
                            struct iw_request_info *info,
                            union iwreq_data *wrqu,char *keybuf)
{
        _adapter *padapter = netdev_priv(dev);

        u16 addr;
        u8 data;

	 addr = wrqu->data.flags;
        get_user(data, (u8*)wrqu->data.pointer);
        write8(padapter, addr, data);

	printk("-addr=%x, data=%x  read=%x\n", addr, data, read8(padapter, addr) );

        return 0;

}


static int r8711_wx_write16(struct net_device *dev,
                            struct iw_request_info *info,
                            union iwreq_data *wrqu,char *keybuf)
{
        _adapter *padapter = netdev_priv(dev);

        u16 addr;
        u16 data;

	 addr = wrqu->data.flags;
        get_user(data, (u16*)wrqu->data.pointer);
        write16(padapter, addr, data);

printk("-addr=%x, data=%x  read=%x\n", addr, data, read16(padapter, addr) );

        return 0;

}
#endif

static int r8711_wx_writereg(struct net_device *dev,
                            struct iw_request_info *info,
                            union iwreq_data *wrqu,char *keybuf)
{
        _adapter *padapter = netdev_priv(dev);

       u16 addr, leng;
       u32 data;
		
	addr = wrqu->data.flags;
	leng = wrqu->data.length;
       get_user(data, (u32*)wrqu->data.pointer);

	switch(leng)
	{
		case 8:
			write8(padapter, addr, data);
			break;
		case 16:
			write16(padapter, addr, data);	
			break;
		case 32:
			write32(padapter, addr, data);
			break;
		default:
			break;			
	}

printk("-addr=%x, data=%x  read=%x\n", addr, data, read32(padapter, addr) );

        return 0;

}
extern u4Byte PHY_QueryRFReg(PADAPTER Adapter, u1Byte eRFPath, u4Byte	RegAddr, u4Byte BitMask);


static int r8711_wx_readrf32_a(struct net_device *dev,
                            struct iw_request_info *info,
                            union iwreq_data *wrqu, char *keybuf)
{
     //   uint key, ret =0;
        _adapter *padapter = netdev_priv(dev);
     //   struct iw_point *erq = &(wrqu->encoding);

        u16 addr;
        u32 data1;

printk("r8711_wx_rearfd32_a\n");
		
	addr = wrqu->data.flags;
	data1 = PHY_QueryRFReg(padapter, RF90_PATH_A, addr, 0xffffffff);
	put_user(data1, (u32*)wrqu->data.pointer);

printk("addr = %x, data1 = %x\n", addr, data1);
	
       return 0;

}
static int r8711_wx_readrf32_b(struct net_device *dev,
                            struct iw_request_info *info,
                            union iwreq_data *wrqu, char *keybuf)
{
     //   uint key, ret =0;
        _adapter *padapter = netdev_priv(dev);
     //   struct iw_point *erq = &(wrqu->encoding);

        u16 addr;
        u32 data1;

printk("r8711_wx_rearfd32_b\n");
		
	addr = wrqu->data.flags;
	data1 = PHY_QueryRFReg(padapter, RF90_PATH_B, addr, 0xffffffff);
	put_user(data1, (u32*)wrqu->data.pointer);

printk("addr = %x, data1 = %x\n", addr, data1);

        return 0;

}

#if 0 
static int r8711_wx_writerf32(struct net_device *dev,
                            struct iw_request_info *info,
                            union iwreq_data *wrqu,char *keybuf)
{

        _adapter *padapter = netdev_priv(dev);

        u16 addr;
        u32 data;

	 addr = wrqu->data.flags;
        get_user(data, (u32*)wrqu->data.pointer);
        write32(padapter, addr, data);

printk("-addr=%x, data=%x  read=%x\n", addr, data, read32(padapter, addr) );

        return 0;

}
#endif


static int dummy(struct net_device *dev, struct iw_request_info *a,
		 union iwreq_data *wrqu,char *b)
{

	printk("cmd_code=%x\n", a->cmd);
	
	return -1;
}


static uint wpa_set_auth_algs(struct net_device *dev, u32 value)
{
	
	_adapter *padapter = netdev_priv(dev);
	int ret = 0;

	if (value & AUTH_ALG_SHARED_KEY) {
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
		padapter->securitypriv.ndisauthtype=Ndis802_11AuthModeShared;
		padapter->securitypriv.dot11AuthAlgrthm= 1;
	} else {
		//padapter->securitypriv.ndisencryptstatus = Ndis802_11EncryptionDisabled;
		if(padapter->securitypriv.ndisauthtype<4){
			padapter->securitypriv.ndisauthtype=Ndis802_11AuthModeOpen;
 			padapter->securitypriv.dot11AuthAlgrthm= 0;
		}
	}

	

	return ret;
}


static uint wpa_set_param(struct net_device *dev, u8 name, u32 value)
{
	uint ret=0;
//	u32 flags;
	_adapter *padapter = netdev_priv(dev);
	switch (name) {
	case IEEE_PARAM_WPA_ENABLED:
		padapter->securitypriv.dot11AuthAlgrthm= 2;  //802.1x
		//ret = ieee80211_wpa_enable(ieee, value);
		switch((value)&0xff){
			case 1 : //WPA
			padapter->securitypriv.ndisauthtype=Ndis802_11AuthModeWPAPSK; //WPA_PSK
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption2Enabled;
				break;
			case 2: //WPA2
			padapter->securitypriv.ndisauthtype=Ndis802_11AuthModeWPA2PSK; //WPA2_PSK
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
				break;
		}
		DEBUG_ERR(("\n wpa_set_param:padapter->securitypriv.ndisauthtype=%d\n",padapter->securitypriv.ndisauthtype));
		break;

	case IEEE_PARAM_TKIP_COUNTERMEASURES:
	//	ieee->tkip_countermeasures=value;
		break;

	case IEEE_PARAM_DROP_UNENCRYPTED: {
		/* HACK:
		 *
		 * wpa_supplicant calls set_wpa_enabled when the driver
		 * is loaded and unloaded, regardless of if WPA is being
		 * used.  No other calls are made which can be used to
		 * determine if encryption will be used or not prior to
		 * association being expected.  If encryption is not being
		 * used, drop_unencrypted is set to false, else true -- we
		 * can use this to determine if the CAP_PRIVACY_ON bit should
		 * be set.
		 */
	#if 0	 
		struct ieee80211_security sec = {
			.flags = SEC_ENABLED,
			.enabled = value,
		};
 		ieee->drop_unencrypted = value;
		/* We only change SEC_LEVEL for open mode. Others
		 * are set by ipw_wpa_set_encryption.
		 */
		if (!value) {
			sec.flags |= SEC_LEVEL;
			sec.level = SEC_LEVEL_0;
		}
		else {
			sec.flags |= SEC_LEVEL;
			sec.level = SEC_LEVEL_1;
		}
		if (ieee->set_security)
			ieee->set_security(ieee->dev, &sec);
#endif		
		break;

	}

	case IEEE_PARAM_PRIVACY_INVOKED:
	//	ieee->privacy_invoked=value;
		break;

	case IEEE_PARAM_AUTH_ALGS:
		ret = wpa_set_auth_algs(dev, value);
		break;

	case IEEE_PARAM_IEEE_802_1X:
//		ieee->ieee802_1x=value;
		break;
	case IEEE_PARAM_WPAX_SELECT:
		// added for WPA2 mixed mode
		//printk(KERN_WARNING "------------------------>wpax value = %x\n", value);
/*
		spin_lock_irqsave(&ieee->wpax_suitlist_lock,flags);
		ieee->wpax_type_set = 1;
		ieee->wpax_type_notify = value;
		spin_unlock_irqrestore(&ieee->wpax_suitlist_lock,flags);
*/
		break;

	default:
		ret = -EOPNOTSUPP;
	}

	return ret;
}


static uint wpa_mlme(struct net_device *dev, u32 command, u32 reason)
{
	
	uint ret = 0;
	_adapter *padapter = netdev_priv(dev);

	switch (command) {
	case IEEE_MLME_STA_DEAUTH:
		// silently ignore
		break;

	case IEEE_MLME_STA_DISASSOC:
		if(!set_802_11_disassociate( padapter))
			ret = -1;
		
	//	ieee80211_disassociate(ieee);
		break;

	default:
		ret = -EOPNOTSUPP;
	}

	return ret;
}



static uint wpa_set_encryption(struct net_device *dev,
				  struct ieee_param *param, u32 param_len)
{
	uint ret = 0;
	_adapter *padapter = netdev_priv(dev);
	struct mlme_priv 		*pmlmepriv = &padapter->mlmepriv;	
//	NDIS_802_11_AUTHENTICATION_MODE authmode;

	param->u.crypt.err = 0;
	param->u.crypt.alg[IEEE_CRYPT_ALG_NAME_LEN - 1] = '\0';

	if (param_len !=
	    (u32) ((u8 *) param->u.crypt.key - (u8 *) param) +
	    param->u.crypt.key_len) {
		ret =  -EINVAL;
		goto exit;
	}
	if (param->sta_addr[0] == 0xff && param->sta_addr[1] == 0xff &&
	    param->sta_addr[2] == 0xff && param->sta_addr[3] == 0xff &&
	    param->sta_addr[4] == 0xff && param->sta_addr[5] == 0xff) {
		if (param->u.crypt.idx >= WEP_KEYS)
			{
				ret = -EINVAL;
				goto exit;
			}
	
	} else {
		
		ret= -EINVAL;
		goto exit;
	}

	if(padapter->securitypriv.dot11AuthAlgrthm== 2) // 802_1x
	{
			struct sta_info * psta,*pbcmc_sta;
			struct sta_priv * pstapriv = &padapter->stapriv;

		if(check_fwstate(pmlmepriv, WIFI_STATION_STATE | WIFI_MP_STATE)) //sta mode
		{
			psta = get_stainfo(pstapriv, get_bssid(pmlmepriv));
			
			if(psta==NULL){
				DEBUG_ERR( ("Set OID_802_11_ADD_KEY: Obtain Sta_info fail \n"));
			}
			else
			{
				psta->ieee8021x_blocked = _FALSE;
				if((padapter->securitypriv.ndisencryptstatus == Ndis802_11Encryption2Enabled)||
					(padapter->securitypriv.ndisencryptstatus ==  Ndis802_11Encryption3Enabled))
					psta->dot118021XPrivacy =padapter->securitypriv.dot11PrivacyAlgrthm;		
				
				if(param->u.crypt.set_tx ==1){   //pairwise key
					_memcpy(psta->dot118021x_UncstKey.skey,  param->u.crypt.key,(param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));
					if(strcmp(param->u.crypt.alg, "TKIP") == 0){
					//set mic key
						DEBUG_ERR(("\nset key length :param->u.crypt.key_len=%d\n",param->u.crypt.key_len));
						_memcpy(psta->dot11tkiptxmickey.skey,&(param->u.crypt.key[16]),8);
						_memcpy(psta->dot11tkiprxmickey.skey,&(param->u.crypt.key[24]),8);
					}
					DEBUG_ERR(("\n param->u.crypt.key_len=%d\n",param->u.crypt.key_len));
					DEBUG_ERR(("\n ~~~~stastakey:unicastkey\n"));
					setstakey_cmd(padapter,(unsigned char *)psta,_TRUE);
				}
				else{ //group key
					_memcpy(padapter->securitypriv.dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key,(param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));
					DEBUG_ERR(("\n param->u.crypt.key_len=%d\n",param->u.crypt.key_len));
					DEBUG_ERR(("\n ~~~~stastakey:groupkey\n"));
					set_key(padapter,&padapter->securitypriv,param->u.crypt.idx);
				}
			}

			pbcmc_sta=get_bcmc_stainfo(padapter);
			if(pbcmc_sta==NULL){
				DEBUG_ERR( ("Set OID_802_11_ADD_KEY: bcmc stainfo is null \n"));
			}
			else
			{
				pbcmc_sta->ieee8021x_blocked = _FALSE;
				if((padapter->securitypriv.ndisencryptstatus == Ndis802_11Encryption2Enabled)||
					(padapter->securitypriv.ndisencryptstatus ==  Ndis802_11Encryption3Enabled))
						
					pbcmc_sta->dot118021XPrivacy =padapter->securitypriv.dot11PrivacyAlgrthm;
			}
		}

		else if(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)) //adhoc mode
		{					
		}
		
	}	

exit:
	_func_exit_;
	return ret;

	
}



static uint parse_wpa_ie(struct net_device *dev, u8 * ies)
{
	_adapter *padapter = netdev_priv(dev);
	u8 auth_mode;
	u8 length;
	u8 ret=_SUCCESS;
	u8 *pos=ies;
	u8 group_cipher[4];
	u8 pairwise_cipher[4];

	auth_mode=*pos;
	pos=pos+1;
	printk("parse_wpa_ie:auth_mode=0x%.2x",auth_mode);
	length=*pos;
       printk("parse_wpa_ie:length=0x%.2x",length);
	pos=pos+1;
	if(auth_mode==0xdd)
	{ //wpa
		pos=pos+4; //remove wpa tag
		//get group key cipher
		padapter->securitypriv.dot11AuthAlgrthm= 2;
		padapter->securitypriv.ndisauthtype=Ndis802_11AuthModeWPAPSK;
		

	}
	if(auth_mode==0x30)
	{ //wpa2
			padapter->securitypriv.dot11AuthAlgrthm= 2;
			padapter->securitypriv.ndisauthtype=Ndis802_11AuthModeWPA2PSK;
	}
	if(*pos ==1){
		pos=pos+2;
		_memcpy(group_cipher, pos,RSN_SELECTOR_LEN );
		printk("\n group_cipher=0x%.2x 0x%.2x 0x%.2x 0x%.2x \n",group_cipher[0],group_cipher[1],group_cipher[2],group_cipher[3]);
		
		switch(group_cipher[3])
		{
			case 0:
				padapter->securitypriv.dot118021XGrpPrivacy=_NO_PRIVACY_;
				padapter->securitypriv.ndisencryptstatus=Ndis802_11EncryptionDisabled;
				break;
			case 1:
				padapter->securitypriv.dot118021XGrpPrivacy=_WEP40_;
				padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
				break;
			case 2:
				padapter->securitypriv.dot118021XGrpPrivacy=_TKIP_;
				padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption2Enabled;
				break;
			case 4:
				padapter->securitypriv.dot118021XGrpPrivacy=_AES_;
				padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
				break;
			case 5:	
				padapter->securitypriv.dot118021XGrpPrivacy=_WEP104_;
				padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
				break;

		}
		pos=pos+4;
	}
	if(*pos ==1){
		pos=pos+2;
		_memcpy(pairwise_cipher, pos,RSN_SELECTOR_LEN );
		printk("\n pairwise_cipher=0x%.2x 0x%.2x 0x%.2x 0x%.2x \n",pairwise_cipher[0],pairwise_cipher[1],pairwise_cipher[2],pairwise_cipher[3]);
		switch(pairwise_cipher[3])
		{
			case 0:
				padapter->securitypriv.dot11PrivacyAlgrthm=_NO_PRIVACY_;
				padapter->securitypriv.ndisencryptstatus=Ndis802_11EncryptionDisabled;
				break;
			case 1:
				padapter->securitypriv.dot11PrivacyAlgrthm=_WEP40_;
				padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
				break;
			case 2:
				padapter->securitypriv.dot11PrivacyAlgrthm=_TKIP_;
				padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption2Enabled;
printk("\n (TKIP Pairwise)padapter->securitypriv.ndisencryptstatus=%d\n",padapter->securitypriv.ndisencryptstatus);
				break;
			case 4:
				padapter->securitypriv.dot11PrivacyAlgrthm=_AES_;
				padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
printk("\n(AES Pairwise) padapter->securitypriv.ndisencryptstatus=%d\n",padapter->securitypriv.ndisencryptstatus);
				break;
			case 5:	
				padapter->securitypriv.dot11PrivacyAlgrthm=_WEP104_;
				padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
				break;

		}
		pos=pos+4;
	}
	printk("\n padapter->securitypriv.ndisencryptstatus=%d\n",padapter->securitypriv.ndisencryptstatus);
	return ret;
}

static uint wpa_set_wpa_ie(struct net_device *dev,
			      struct ieee_param *param, u32 plen)
{
	u8 *buf;
	uint ret = 0;
	u32 left;
//	u32 pairwise_cipher = 0 ;
//	_adapter *padapter = netdev_priv(dev);
	 u8 *pos;

	if (param->u.wpa_ie.len > MAX_WPA_IE_LEN ||
	    (param->u.wpa_ie.len && param->u.wpa_ie.data == NULL))
		return -EINVAL;

	if (param->u.wpa_ie.len) {
	buf = _malloc(param->u.wpa_ie.len);
	if (buf == NULL){
		ret =  -ENOMEM;
		goto exit;
	}
	_memcpy(buf, param->u.wpa_ie.data, param->u.wpa_ie.len);
	{
		int i;
		printk("\n wpa_ie(length:%d):\n",param->u.wpa_ie.len);
		for(i=0;i<param->u.wpa_ie.len;i=i+8)
			printk("0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x \n",buf[i],buf[i+1],buf[i+2],buf[i+3],buf[i+4],buf[i+5],buf[i+6],buf[i+7]);
	}
		pos = buf;
		if(param->u.wpa_ie.len < RSN_HEADER_LEN){
			DEBUG_ERR(("Ie len too short 0x%x\n",(u32)param->u.wpa_ie.len ));
			ret  = -1;
			goto exit;

		}

		pos += RSN_HEADER_LEN;
		left  = param->u.wpa_ie.len- RSN_HEADER_LEN;
		if (left >= RSN_SELECTOR_LEN) {
			pos += RSN_SELECTOR_LEN;
			left -= RSN_SELECTOR_LEN;
		}
		else if (left > 0 ){
			DEBUG_ERR(("Ie length mismatch, %u too much \n",left ));
			ret =-1;
			goto exit;

			}
		parse_wpa_ie(dev, buf);

	}
 
	
exit:
	return ret;
}





uint wpa_supplicant_ioctl(struct net_device *dev, struct iw_point *p)
{
	struct ieee_param *param;
	uint ret=0;

	//down(&ieee->wx_sem);
	//IEEE_DEBUG_INFO("wpa_supplicant: len=%d\n", p->length);

	if (p->length < sizeof(struct ieee_param) || !p->pointer){
		ret = -EINVAL;
		goto out;
	}
	
	param = (struct ieee_param *)_malloc(p->length);
	if (param == NULL){
		ret = -ENOMEM;
		goto out;
	}
	if (copy_from_user(param, p->pointer, p->length)) {
		kfree(param);
		ret = -EFAULT;
		goto out;
	}

	switch (param->cmd) {

	case IEEE_CMD_SET_WPA_PARAM:
		ret = wpa_set_param(dev, param->u.wpa_param.name,
					param->u.wpa_param.value);
		break;

	case IEEE_CMD_SET_WPA_IE:
		ret = wpa_set_wpa_ie(dev, param, p->length);
		break;

	case IEEE_CMD_SET_ENCRYPTION:
		ret = wpa_set_encryption(dev, param, p->length);
		break;

	case IEEE_CMD_MLME:
		ret = wpa_mlme(dev, param->u.mlme.command,
				   param->u.mlme.reason_code);
		break;

	default:
		printk("Unknown WPA supplicant request: %d\n",param->cmd);
		ret = -EOPNOTSUPP;
		break;
	}

	if (ret == 0 && copy_to_user(p->pointer, param, p->length))
		ret = -EFAULT;

	_mfree((u8 *)param,sizeof(struct ieee_param));
out:
//	up(&ieee->wx_sem);
	
	return ret;
}



int r8711_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
//	_adapter *padapter = netdev_priv(dev);
	struct iwreq *wrq = (struct iwreq *)rq;
//	uint i;
	int ret=-1;

//	down(&priv->wx_sem);

	switch (cmd) {
	    case RTL_IOCTL_WPA_SUPPLICANT:
	#if 0
		printk("@@ wrq->u pointer = ");
		for(i=0;i<wrq->u.data.length;i++){
			if(i%10==0) printk("\n");
			printk( "%8x|", ((u32*)wrq->u.data.pointer)[i] );
		}
		printk("\n");
	#endif
		ret = wpa_supplicant_ioctl(dev, &wrq->u.data);
		break; 

	    default:
		ret = -EOPNOTSUPP;
		break;
	}

//	up(&priv->wx_sem);
	
	return ret;
}

static iw_handler r8711_handlers[] =
{
        NULL,                     /* SIOCSIWCOMMIT */
        r8711_wx_get_name,   	  /* SIOCGIWNAME */
        dummy,                    /* SIOCSIWNWID */
        dummy,                    /* SIOCGIWNWID */
        dummy,//        r8711_wx_set_freq,        /* SIOCSIWFREQ */
        dummy,//        r8711_wx_get_freq,        /* SIOCGIWFREQ */
        r8711_wx_set_mode,        /* SIOCSIWMODE */
        r8711_wx_get_mode,        /* SIOCGIWMODE */
        dummy,//        r8711_wx_set_sens,        /* SIOCSIWSENS */
        dummy,//        r8711_wx_get_sens,        /* SIOCGIWSENS */
        NULL,                     /* SIOCSIWRANGE */
        r8711_wx_get_range,	  /* SIOCGIWRANGE */
        NULL,                     /* SIOCSIWPRIV */
        NULL,                     /* SIOCGIWPRIV */
        NULL,                     /* SIOCSIWSTATS */
        NULL,                     /* SIOCGIWSTATS */
        dummy,                    /* SIOCSIWSPY */
        dummy,                    /* SIOCGIWSPY */
        NULL,                     /* SIOCGIWTHRSPY */
        NULL,                     /* SIOCWIWTHRSPY */
       r8711_wx_set_wap,      	  /* SIOCSIWAP */
        r8711_wx_get_wap,         /* SIOCGIWAP */
        NULL,                     /* -- hole -- */
        dummy,                     /* SIOCGIWAPLIST -- depricated */
        r8711_wx_set_scan,        /* SIOCSIWSCAN */
        r8711_wx_get_scan,        /* SIOCGIWSCAN */
        r8711_wx_set_essid,       /* SIOCSIWESSID */
        r8711_wx_get_essid,       /* SIOCGIWESSID */
        dummy,                    /* SIOCSIWNICKN */
        dummy,                    /* SIOCGIWNICKN */
        NULL,                     /* -- hole -- */
        NULL,                     /* -- hole -- */
        dummy,//       r8711_wx_set_rate,        /* SIOCSIWRATE */
        dummy,//       r8711_wx_get_rate,        /* SIOCGIWRATE */
        dummy,                    /* SIOCSIWRTS */
        dummy,                    /* SIOCGIWRTS */
         r8711_wx_set_frag,        /* SIOCSIWFRAG */
       r8711_wx_get_frag,        /* SIOCGIWFRAG */
        dummy,                    /* SIOCSIWTXPOW */
        dummy,                    /* SIOCGIWTXPOW */
        dummy,//        r8711_wx_set_retry,       /* SIOCSIWRETRY */
        dummy,//        r8711_wx_get_retry,       /* SIOCGIWRETRY */
       r8711_wx_set_enc,         /* SIOCSIWENCODE */
        r8711_wx_get_enc,         /* SIOCGIWENCODE */
        dummy,                    /* SIOCSIWPOWER */
        dummy,                    /* SIOCGIWPOWER */
}; 

static const struct iw_priv_args r8711_private_args[] = {
	{
                SIOCIWFIRSTPRIV + 0x0,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "readreg"
        },
        
        {
                SIOCIWFIRSTPRIV + 0x1,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "writereg"
        },
	{
                SIOCIWFIRSTPRIV + 0x2,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "readrf32a"
        },

        {
                SIOCIWFIRSTPRIV + 0x3,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "readrf32b"
        }

};

static iw_handler r8711_private_handler[] = {
	r8711_wx_readreg,  			/* 0 */
	r8711_wx_writereg, 			/* 1 */
	r8711_wx_readrf32_a,		/* 2 */
	r8711_wx_readrf32_b		/* 3 */
};


struct iw_handler_def  r8711_handlers_def={
	.standard = r8711_handlers,
	.num_standard = sizeof(r8711_handlers) / sizeof(iw_handler),
	.private = r8711_private_handler,
	.num_private = sizeof(r8711_private_handler) / sizeof(iw_handler),
 	.num_private_args = sizeof(r8711_private_args) / sizeof(struct iw_priv_args),
#if WIRELESS_EXT >= 17	
//	.get_wireless_stats = r8711_get_wireless_stats,
#endif
	.private_args = (struct iw_priv_args *)r8711_private_args,	
};
