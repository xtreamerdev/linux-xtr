/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 * Linux device driver for RTL8192U 
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/

#ifdef RTL8192SU
#include <linux/string.h>
#include "r8192U.h"
#include "r8192S_hw.h"
#else
#include <linux/string.h>
#include "r8192U.h"
#include "r8192U_hw.h"
#endif

#ifdef ENABLE_DOT11D
#include "dot11d.h"
#endif

#define RATE_COUNT 12
u32 rtl8180_rates[] = {1000000,2000000,5500000,11000000,
	6000000,9000000,12000000,18000000,24000000,36000000,48000000,54000000};
	

#ifndef ENETDOWN
#define ENETDOWN 1
#endif

#if defined(ENABLE_UNASSOCIATED_USB_SUSPEND) && !defined(ENABLE_UNASSOCIATED_USB_AUTOSUSPEND)
extern int rtl8192U_resume (struct usb_interface *intf);
#endif


static int r8192_wx_get_freq(struct net_device *dev,
			     struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	return rtllib_wx_get_freq(priv->rtllib,a,wrqu,b);
}


#if 0

static int r8192_wx_set_beaconinterval(struct net_device *dev, struct iw_request_info *aa,
			  union iwreq_data *wrqu, char *b)
{
	int *parms = (int *)b;
	int bi = parms[0];
	
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);
	DMESG("setting beacon interval to %x",bi);
	
	priv->rtllib->beacon_interval=bi;
	rtl8180_commit(dev);
	up(&priv->wx_sem);
		
	return 0;	
}


static int r8192_wx_set_forceassociate(struct net_device *dev, struct iw_request_info *aa,
			  union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv=rtllib_priv(dev);	
	int *parms = (int *)extra;
	
	priv->rtllib->force_associate = (parms[0] > 0);
	

	return 0;
}

#endif
static int r8192_wx_get_mode(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv=rtllib_priv(dev);	

	return rtllib_wx_get_mode(priv->rtllib,a,wrqu,b);
}



static int r8192_wx_get_rate(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	return rtllib_wx_get_rate(priv->rtllib,info,wrqu,extra);
}



static int r8192_wx_set_rate(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);	
	
	down(&priv->wx_sem);

	ret = rtllib_wx_set_rate(priv->rtllib,info,wrqu,extra);
	
	up(&priv->wx_sem);
	
	return ret;
}


static int r8192_wx_set_rts(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);	
	
	down(&priv->wx_sem);

	ret = rtllib_wx_set_rts(priv->rtllib,info,wrqu,extra);
	
	up(&priv->wx_sem);
	
	return ret;
}

static int r8192_wx_get_rts(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	return rtllib_wx_get_rts(priv->rtllib,info,wrqu,extra);
}

static int r8192_wx_set_power(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);	
	
	down(&priv->wx_sem);

	ret = rtllib_wx_set_power(priv->rtllib,info,wrqu,extra);
	
	up(&priv->wx_sem);
	
	return ret;
}

static int r8192_wx_get_power(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	return rtllib_wx_get_power(priv->rtllib,info,wrqu,extra);
}



//#ifdef RTK_DMP_PLATFORM
static int r8192_wx_get_ap_status(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8192_priv *priv = rtllib_priv(dev);
        struct rtllib_device *ieee = priv->rtllib;
        struct rtllib_network *target;
	struct rtllib_network *latest = NULL;
	int name_len;

        down(&priv->wx_sem);

	//count the length of input ssid
	for(name_len=0 ; ((char*)wrqu->data.pointer)[name_len]!='\0' ; name_len++);

	//search for the correspoding info which is received
        list_for_each_entry(target, &ieee->network_list, list) {
                if ( (target->ssid_len == name_len) &&
		     (strncmp(target->ssid, (char*)wrqu->data.pointer, name_len)==0)){
			if ((latest == NULL) ||(target->last_scanned > latest->last_scanned))
				latest = target;
#if 0
			//if( ((jiffies-target->last_scanned)/HZ > 1) && (ieee->state == RTLLIB_LINKED) && (is_same_network(&ieee->current_network,target)) )
			//	wrqu->data.length = 999;
			//else
				wrqu->data.length = target->SignalStrength;

			if(target->wpa_ie_len>0 || target->rsn_ie_len>0 ) {
				//set flags=1 to indicate this ap is WPA or WPA2
				wrqu->data.flags = 1;
			} else {
				wrqu->data.flags = 0;
			}
		break;
#endif
		}
        }

        if(latest != NULL)
        {
		wrqu->data.length = latest->SignalStrength;

		if(latest->wpa_ie_len>0 || latest->rsn_ie_len>0 ) {
			//set flags=1 to indicate this ap is WPA or WPA2
			wrqu->data.flags = 1;
		} else {
			wrqu->data.flags = 0;
                }
        }

        up(&priv->wx_sem);
        return 0;
}

extern short rtl8192_get_channel_map(struct net_device * dev);
static int r8192_wx_set_countrycode(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8192_priv *priv = rtllib_priv(dev);
	int	countrycode=0;

        down(&priv->wx_sem);

	countrycode = (int)wrqu->data.pointer;
	printk("\n======== Set Countrycode = %d !!!! ========\n",countrycode);
	priv->ChannelPlan = countrycode;
	rtl8192_get_channel_map(dev);

        up(&priv->wx_sem);
        return 0;
}
//#endif

static int r8192_wx_null(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	return 0;
}

static int r8192_wx_force_reset(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);

	printk("%s(): force reset ! extra is %d\n",__FUNCTION__, *extra);
	priv->force_reset = *extra;
	up(&priv->wx_sem);
	return 0;

}

#ifdef RTL8192SU
static int r8191su_wx_get_firm_version(struct net_device *dev,
		struct iw_request_info *info,
		struct iw_param *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u16 firmware_version;

	down(&priv->wx_sem);
	firmware_version = priv->pFirmware->FirmwareVersion;
	wrqu->value = firmware_version;
	wrqu->fixed = 1;

	up(&priv->wx_sem);
	return 0;
}
#endif

static int r8192_wx_force_mic_error(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	
	down(&priv->wx_sem);

	printk("%s(): force mic error ! \n",__FUNCTION__);
	ieee->force_mic_error = true;
	up(&priv->wx_sem);
	return 0;

}


#define MAX_ADHOC_PEER_NUM 64 //YJ,modified,090304
typedef struct 
{
	unsigned char MacAddr[ETH_ALEN];
	unsigned char WirelessMode;
	unsigned char bCurTxBW40MHz;		
} adhoc_peer_entry_t, *p_adhoc_peer_entry_t;
typedef struct 
{
	adhoc_peer_entry_t Entry[MAX_ADHOC_PEER_NUM];
	unsigned char num;
} adhoc_peers_info_t, *p_adhoc_peers_info_t;
int r8192_wx_get_adhoc_peers(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct sta_info * psta = NULL;
	adhoc_peers_info_t adhoc_peers_info;
	p_adhoc_peers_info_t  padhoc_peers_info = &adhoc_peers_info; //(p_adhoc_peers_info_t)extra;
	p_adhoc_peer_entry_t padhoc_peer_entry = NULL;
	int k=0;

	memset(extra, 0, 2047);
	padhoc_peers_info->num = 0;

	down(&priv->wx_sem);

	for(k=0; k<PEER_MAX_ASSOC; k++)
	{
		psta = priv->rtllib->peer_assoc_list[k];
		if(NULL != psta)
		{
			padhoc_peer_entry = &padhoc_peers_info->Entry[padhoc_peers_info->num];
			memset(padhoc_peer_entry,0, sizeof(adhoc_peer_entry_t));
			memcpy(padhoc_peer_entry->MacAddr, psta->macaddr, ETH_ALEN);
			padhoc_peer_entry->WirelessMode = psta->wireless_mode;
			padhoc_peer_entry->bCurTxBW40MHz = psta->htinfo.bCurTxBW40MHz;
			padhoc_peers_info->num ++;
			printk("[%d] MacAddr:"MAC_FMT" \tWirelessMode:%d \tBW40MHz:%d \n", \
				k, MAC_ARG(padhoc_peer_entry->MacAddr), padhoc_peer_entry->WirelessMode, padhoc_peer_entry->bCurTxBW40MHz);
			sprintf(extra, "[%d] MacAddr:"MAC_FMT" \tWirelessMode:%d \tBW40MHz:%d \n",  \
				k, MAC_ARG(padhoc_peer_entry->MacAddr), padhoc_peer_entry->WirelessMode, padhoc_peer_entry->bCurTxBW40MHz);
		}
	}

	up(&priv->wx_sem);

	wrqu->data.length = strlen(extra);//sizeof(adhoc_peers_info_t);
	wrqu->data.flags = 0;
	return 0;

}

static int r8192_wx_set_rawtx(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;
	
	down(&priv->wx_sem);
	
	ret = rtllib_wx_set_rawtx(priv->rtllib, info, wrqu, extra);
	
	up(&priv->wx_sem);
	
	return ret;
	 
}

static int r8192_wx_set_crcmon(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int *parms = (int *)extra;
	int enable = (parms[0] > 0);
	short prev = priv->crcmon;

	down(&priv->wx_sem);
	
	if(enable) 
		priv->crcmon=1;
	else 
		priv->crcmon=0;

	DMESG("bad CRC in monitor mode are %s", 
	      priv->crcmon ? "accepted" : "rejected");

	if(prev != priv->crcmon && priv->up){
		//rtl8180_down(dev);
		//rtl8180_up(dev);
	}
	
	up(&priv->wx_sem);
	
	return 0;
}

// check ac/dc status with the help of user space application */
static int r8192_wx_adapter_power_status(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
#ifdef ENABLE_LPS
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	struct rtllib_device* ieee = priv->rtllib;
#endif
	down(&priv->wx_sem);

#ifdef ENABLE_LPS
	RT_TRACE(COMP_POWER, "%s(): %s\n",__FUNCTION__, (*extra ==  6)?"DC power":"AC power");
    // ieee->ps shall not be set under DC mode, otherwise it conflict 
    // with Leisure power save mode setting.
    //
	if(*extra || priv->force_lps) {
		priv->ps_force = false;
		pPSC->bLeisurePs = true;
	} else {
		priv->ps_force = true;
		pPSC->bLeisurePs = false;
		ieee->ps = *extra;	
	}

#endif
	up(&priv->wx_sem);
	return 0;

}


static int r8192se_wx_set_lps_awake_interval(struct net_device *dev,
        struct iw_request_info *info,
        union iwreq_data *wrqu, char *extra)
{
    struct r8192_priv *priv = rtllib_priv(dev);
    PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));

    down(&priv->wx_sem);

    printk("%s(): set lps awake interval ! extra is %d\n",__FUNCTION__, *extra);

    pPSC->RegMaxLPSAwakeIntvl = *extra;
    up(&priv->wx_sem);
    return 0;

}

static int r8192se_wx_set_force_lps(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);

	printk("%s(): force LPS ! extra is %d (1 is open 0 is close)\n",__FUNCTION__, *extra);
	priv->force_lps = *extra;
	up(&priv->wx_sem);
	return 0;

}

static int r8192_wx_set_mode(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv = rtllib_priv(dev);
#ifdef ENABLE_IPS	
	RT_RF_POWER_STATE	rtState;
#endif
	int ret;

#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND
	if (priv->is_suspended) {
		printk("%s-%d: autoresume...\n", __FUNCTION__, __LINE__);
#ifdef ENABLE_UNASSOCIATED_USB_AUTOSUSPEND
		usb_autopm_disable(priv->intf);
#else
		rtl8192U_resume(priv->intf);
#endif
	}
        priv->suspend_delay_cnt = 0;
#endif

	down(&priv->wx_sem);

#ifdef ENABLE_IPS	
	rtState = priv->rtllib->eRFPowerState;
	if(wrqu->mode == IW_MODE_ADHOC){
	
		if(priv->rtllib->PowerSaveControl.bInactivePs){ 
			if(rtState == eRfOff){
				if(priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS)
				{
					RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
					up(&priv->wx_sem);
					return -1;
				}
				else{
					printk("=========>%s(): IPSLeave\n",__FUNCTION__);
					down(&priv->rtllib->ips_sem);
					IPSLeave(dev);
					up(&priv->rtllib->ips_sem);
				}
			}
		}
	}
#endif
	
	ret = rtllib_wx_set_mode(priv->rtllib,a,wrqu,b);
	
	//rtl8192_set_rxconf(dev);  //YJ,del,090414, it may cause bit RCR_CBSSID set to 1 when mode changes from managed to ad-hoc.
	
	up(&priv->wx_sem);
	return ret;
}

struct  iw_range_with_scan_capa
{
        //Informative stuff (to choose between different interface) */
        __u32           throughput;     // To give an idea... */
        // In theory this value should be the maximum benchmarked
        // TCP/IP throughput, because with most of these devices the
        // bit rate is meaningless (overhead an co) to estimate how
        // fast the connection will go and pick the fastest one.
        // I suggest people to play with Netperf or any benchmark...
        // 

        // NWID (or domain id) */
        __u32           min_nwid;       // Minimal NWID we are able to set */
        __u32           max_nwid;       // Maximal NWID we are able to set */

        // Old Frequency (backward compat - moved lower ) */
        __u16           old_num_channels;
        __u8            old_num_frequency;

        // Scan capabilities */
        __u8            scan_capa;       
};
static int rtl8180_wx_get_range(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct iw_range *range = (struct iw_range *)extra;
	struct iw_range_with_scan_capa* tmp = (struct iw_range_with_scan_capa*)range;
	struct r8192_priv *priv = rtllib_priv(dev);
	u16 val;
	int i;

	wrqu->data.length = sizeof(*range);
	memset(range, 0, sizeof(*range));

	// Let's try to keep this struct in the same order as in
	// linux/include/wireless.h
	// 
	
	// TODO: See what values we can set, and remove the ones we can't
	//  set, or fill them with some default data.
	// 

	// ~5 Mb/s real (802.11b) */
	range->throughput = 5 * 1000 * 1000;     

	// TODO: Not used in 802.11b?
//	range->min_nwid;	// Minimal NWID we are able to set */
	// TODO: Not used in 802.11b?
//	range->max_nwid;	// Maximal NWID we are able to set */
	
        // Old Frequency (backward compat - moved lower ) */
//	range->old_num_channels; 
//	range->old_num_frequency;
//	range->old_freq[6]; // Filler to keep "version" at the same offset */
	if(priv->rf_set_sens != NULL)
		range->sensitivity = priv->max_sens;	// signal level threshold range */
	
	range->max_qual.qual = 100;
	// TODO: Find real max RSSI and stick here */
	range->max_qual.level = 0;
	range->max_qual.noise = -98;
	range->max_qual.updated = 7; // Updated all three */

	range->avg_qual.qual = 92; // > 8% missed beacons is 'bad' */
	// TODO: Find real 'good' to 'bad' threshol value for RSSI */
	range->avg_qual.level = 20 + -98;
	range->avg_qual.noise = 0;
	range->avg_qual.updated = 7; // Updated all three */

	range->num_bitrates = RATE_COUNT;
	
	for (i = 0; i < RATE_COUNT && i < IW_MAX_BITRATES; i++) {
		range->bitrate[i] = rtl8180_rates[i];
	}
	
	range->min_frag = MIN_FRAG_THRESHOLD;
	range->max_frag = MAX_FRAG_THRESHOLD;
	
	range->min_pmp=0;
	range->max_pmp = 5000000;
	range->min_pmt = 0;
	range->max_pmt = 65535*1000;	
	range->pmp_flags = IW_POWER_PERIOD;
	range->pmt_flags = IW_POWER_TIMEOUT;
	range->pm_capa = IW_POWER_PERIOD | IW_POWER_TIMEOUT | IW_POWER_ALL_R;

	range->we_version_compiled = WIRELESS_EXT;
	range->we_version_source = 16;

//	range->retry_capa;	// What retry options are supported */
//	range->retry_flags;	// How to decode max/min retry limit */
//	range->r_time_flags;	// How to decode max/min retry life */
//	range->min_retry;	// Minimal number of retries */
//	range->max_retry;	// Maximal number of retries */
//	range->min_r_time;	// Minimal retry lifetime */
//	range->max_r_time;	// Maximal retry lifetime */


	for (i = 0, val = 0; i < 14; i++) {
		
		// Include only legal frequencies for some countries
#ifdef ENABLE_DOT11D
		if ((GET_DOT11D_INFO(priv->rtllib)->channel_map)[i+1]) {
#else
		if ((priv->rtllib->channel_map)[i+1]) {
#endif
		        range->freq[val].i = i + 1;
			range->freq[val].m = rtllib_wlan_frequencies[i] * 100000;
			range->freq[val].e = 1;
			val++;
		} else {
			// FIXME: do we need to set anything for channels
			// we don't use ?
		}
		
		if (val == IW_MAX_FREQUENCIES)
		break;
	}
	range->num_frequency = val;
        range->num_channels = val;
#if WIRELESS_EXT > 17
	range->enc_capa = IW_ENC_CAPA_WPA|IW_ENC_CAPA_WPA2|
			  IW_ENC_CAPA_CIPHER_TKIP|IW_ENC_CAPA_CIPHER_CCMP;
#endif
	tmp->scan_capa = 0x01;
	return 0;
}


static int r8192_wx_set_scan(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	RT_RF_POWER_STATE	rtState;
	int ret = 0;

#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND
	if (priv->is_suspended) {
		printk("%s-%d: usb_autoresume...\n", __FUNCTION__, __LINE__);
#ifdef ENABLE_UNASSOCIATED_USB_AUTOSUSPEND
		usb_autopm_disable(priv->intf);
#else
		rtl8192U_resume(priv->intf);
#endif
	}
        priv->suspend_delay_cnt = 0;
#endif

	if(priv->bHwRadioOff == true){
		printk("================>%s(): hwradio off\n",__FUNCTION__);
		return 0;
	}
	rtState = priv->rtllib->eRFPowerState;
	
	if(!priv->up) return -ENETDOWN;
	
	if (priv->rtllib->LinkDetectInfo.bBusyTraffic == true)
		return -EAGAIN;
#if WIRELESS_EXT > 17
	if (wrqu->data.flags & IW_SCAN_THIS_ESSID)
	{
		struct iw_scan_req* req = (struct iw_scan_req*)b;
		if (req->essid_len)
		{
			//printk("==**&*&*&**===>scan set ssid:%s\n", req->essid);
			ieee->current_network.ssid_len = req->essid_len;
			memcpy(ieee->current_network.ssid, req->essid, req->essid_len); 
			//printk("=====>network ssid:%s\n", ieee->current_network.ssid);
		}
	}
#endif	
	
	down(&priv->wx_sem);
#ifdef ENABLE_IPS
	priv->rtllib->actscanning = true;
	if(priv->rtllib->state != RTLLIB_LINKED){
		if(priv->rtllib->PowerSaveControl.bInactivePs){ 
			if(rtState == eRfOff){
				if(priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS)
				{
					RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
					up(&priv->wx_sem);
					return -1;
				}
				else{
					printk("=========>%s(): IPSLeave\n",__FUNCTION__);
					down(&priv->rtllib->ips_sem);
					IPSLeave(dev);
					up(&priv->rtllib->ips_sem);				}
			}
		}
		priv->rtllib->scanning = 0;
		if(priv->rtllib->LedControlHandler)
			priv->rtllib->LedControlHandler(dev, LED_CTL_SITE_SURVEY);
                if(priv->rtllib->eRFPowerState != eRfOff){
			priv->rtllib->sync_scan_hurryup = 0;
			rtllib_softmac_scan_syncro(priv->rtllib);
                }
		ret = 0;
	}
	else
#else	
	if(priv->rtllib->state != RTLLIB_LINKED){
		
		if(priv->rtllib->LedControlHandler != NULL)
			priv->rtllib->LedControlHandler(dev, LED_CTL_SITE_SURVEY);
		
                priv->rtllib->scanning = 0;
                priv->rtllib->sync_scan_hurryup = 0;
                rtllib_softmac_scan_syncro(priv->rtllib);
                ret = 0;
        }
	else
#endif
		ret = rtllib_wx_set_scan(priv->rtllib,a,wrqu,b);
	up(&priv->wx_sem);
	return ret;
}


static int r8192_wx_get_scan(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{

	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if(!priv->up) return -ENETDOWN;
			
	down(&priv->wx_sem);

	ret = rtllib_wx_get_scan(priv->rtllib,a,wrqu,b);
		
	up(&priv->wx_sem);
	
	return ret;
}

static int r8192_wx_set_essid(struct net_device *dev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *b)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret;

#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND
	if (priv->is_suspended) {
		printk("%s-%d: autoresume...\n", __FUNCTION__, __LINE__);
#ifdef ENABLE_UNASSOCIATED_USB_AUTOSUSPEND
		usb_autopm_disable(priv->intf);
#else
		rtl8192U_resume(priv->intf);
#endif
	}
        priv->suspend_delay_cnt = 0;
#endif
	
	down(&priv->wx_sem);
	
	ret = rtllib_wx_set_essid(priv->rtllib,a,wrqu,b);

	up(&priv->wx_sem);

	return ret;
}




static int r8192_wx_get_essid(struct net_device *dev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *b)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);
	
	ret = rtllib_wx_get_essid(priv->rtllib, a, wrqu, b);

	up(&priv->wx_sem);
	
	return ret;
}


static int r8192_wx_set_freq(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);
	
	down(&priv->wx_sem);
	
	ret = rtllib_wx_set_freq(priv->rtllib, a, wrqu, b);
	
	up(&priv->wx_sem);
	return ret;
}

static int r8192_wx_get_name(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	return rtllib_wx_get_name(priv->rtllib, info, wrqu, extra);
}


static int r8192_wx_set_frag(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if (wrqu->frag.disabled)
		priv->rtllib->fts = DEFAULT_FRAG_THRESHOLD;
	else {
		if (wrqu->frag.value < MIN_FRAG_THRESHOLD ||
		    wrqu->frag.value > MAX_FRAG_THRESHOLD)
			return -EINVAL;
		
		priv->rtllib->fts = wrqu->frag.value & ~0x1;
	}

	return 0;
}


static int r8192_wx_get_frag(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	wrqu->frag.value = priv->rtllib->fts;
	wrqu->frag.fixed = 0;	/* no auto select */
	wrqu->frag.disabled = (wrqu->frag.value == DEFAULT_FRAG_THRESHOLD);

	return 0;
}


static int r8192_wx_set_wap(struct net_device *dev,
			 struct iw_request_info *info,
			 union iwreq_data *awrq,
			 char *extra)
{

	int ret;
	struct r8192_priv *priv = rtllib_priv(dev);
//        struct sockaddr *temp = (struct sockaddr *)awrq;
	down(&priv->wx_sem);
	
	ret = rtllib_wx_set_wap(priv->rtllib,info,awrq,extra);

	up(&priv->wx_sem);

	return ret;
	
}
	

static int r8192_wx_get_wap(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	return rtllib_wx_get_wap(priv->rtllib,info,wrqu,extra);
}


static int r8192_wx_get_enc(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *key)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	return rtllib_wx_get_encode(priv->rtllib, info, wrqu, key);
}

static int r8192_wx_set_enc(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *key)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	int ret;

	//u32 TargetContent;
	u32 hwkey[4]={0,0,0,0};
	u8 mask=0xff;
	u32 key_idx=0;
	//u8 broadcast_addr[6] ={	0xff,0xff,0xff,0xff,0xff,0xff}; 
	u8 zero_addr[4][6] ={	{0x00,0x00,0x00,0x00,0x00,0x00},
				{0x00,0x00,0x00,0x00,0x00,0x01}, 
				{0x00,0x00,0x00,0x00,0x00,0x02}, 
				{0x00,0x00,0x00,0x00,0x00,0x03} };
	int i;

#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND
	if (priv->is_suspended) {
		printk("%s-%d: usb_autoresume...\n", __FUNCTION__, __LINE__);
#ifdef ENABLE_UNASSOCIATED_USB_AUTOSUSPEND
		usb_autopm_disable(priv->intf);
#else
		rtl8192U_resume(priv->intf);
#endif
	}
        priv->suspend_delay_cnt = 0;
#endif

       if(!priv->up) return -ENETDOWN;

        priv->rtllib->wx_set_enc = 1;
#ifdef ENABLE_IPS
        down(&priv->rtllib->ips_sem);
        IPSLeave(dev);
        up(&priv->rtllib->ips_sem);			
#endif
	down(&priv->wx_sem);
	
	RT_TRACE(COMP_SEC, "Setting SW wep key");
	ret = rtllib_wx_set_encode(priv->rtllib,info,wrqu,key);

	up(&priv->wx_sem);



	//sometimes, the length is zero while we do not type key value
	if (wrqu->encoding.flags & IW_ENCODE_DISABLED) {
		ieee->pairwise_key_type = ieee->group_key_type = KEY_TYPE_NA;
		CamResetAllEntry(dev);
		memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32);//added by amy for silent reset 090415 
		goto end_hw_sec;
	}
	if(wrqu->encoding.length!=0){

		for(i=0 ; i<4 ; i++){
			hwkey[i] |=  key[4*i+0]&mask;
			if(i==1&&(4*i+1)==wrqu->encoding.length) mask=0x00;
			if(i==3&&(4*i+1)==wrqu->encoding.length) mask=0x00;
			hwkey[i] |= (key[4*i+1]&mask)<<8;
			hwkey[i] |= (key[4*i+2]&mask)<<16;
			hwkey[i] |= (key[4*i+3]&mask)<<24;
		}

		#define CONF_WEP40  0x4
		#define CONF_WEP104 0x14

		switch(wrqu->encoding.flags & IW_ENCODE_INDEX){
			case 0: key_idx = ieee->tx_keyidx; break;
			case 1:	key_idx = 0; break;
			case 2:	key_idx = 1; break;
			case 3:	key_idx = 2; break;
			case 4:	key_idx	= 3; break;
			default: break;
		}

		if(wrqu->encoding.length==0x5){
				ieee->pairwise_key_type = KEY_TYPE_WEP40;
			EnableHWSecurityConfig8192(dev);

			setKey( dev,
				key_idx,                //EntryNo
				key_idx,                //KeyIndex 
				KEY_TYPE_WEP40,         //KeyType
				zero_addr[key_idx],
				0,                      //DefaultKey
				hwkey);                 //KeyContent

		}

		else if(wrqu->encoding.length==0xd){
				ieee->pairwise_key_type = KEY_TYPE_WEP104;
				EnableHWSecurityConfig8192(dev);

			setKey( dev,
				key_idx,                //EntryNo
				key_idx,                //KeyIndex 
				KEY_TYPE_WEP104,        //KeyType
				zero_addr[key_idx],
				0,                      //DefaultKey
				hwkey);                 //KeyContent
 
		}
		else printk("wrong type in WEP, not WEP40 and WEP104\n");

	}
end_hw_sec:
        priv->rtllib->wx_set_enc = 0;
	return ret;
}


static int r8192_wx_set_scan_type(struct net_device *dev, struct iw_request_info *aa, union
 iwreq_data *wrqu, char *p){
  
 	struct r8192_priv *priv = rtllib_priv(dev);
	int *parms=(int*)p;
	int mode=parms[0];
	
	priv->rtllib->active_scan = mode;
	
	return 1;
}



static int r8192_wx_set_retry(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int err = 0;
	
	down(&priv->wx_sem);
	
	if (wrqu->retry.flags & IW_RETRY_LIFETIME || 
	    wrqu->retry.disabled){
		err = -EINVAL;
		goto exit;
	}
	if (!(wrqu->retry.flags & IW_RETRY_LIMIT)){
		err = -EINVAL;
		goto exit;
	}

	if(wrqu->retry.value > R8180_MAX_RETRY){
		err= -EINVAL;
		goto exit;
	}
	if (wrqu->retry.flags & IW_RETRY_MAX) {
		priv->retry_rts = wrqu->retry.value;
		DMESG("Setting retry for RTS/CTS data to %d", wrqu->retry.value);
	
	}else {
		priv->retry_data = wrqu->retry.value;
		DMESG("Setting retry for non RTS/CTS data to %d", wrqu->retry.value);
	}
	
	// FIXME ! 
	// We might try to write directly the TX config register
	// or to restart just the (R)TX process.
	// I'm unsure if whole reset is really needed
	// 

 	rtl8192_commit(dev);
	//
	//if(priv->up){
	//	rtl8180_rtx_disable(dev);
	//	rtl8180_rx_enable(dev);
	//	rtl8180_tx_enable(dev);
	//		
	//}
	//
exit:
	up(&priv->wx_sem);
	
	return err;
}

static int r8192_wx_get_retry(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	

	wrqu->retry.disabled = 0; // can't be disabled */

	if ((wrqu->retry.flags & IW_RETRY_TYPE) == 
	    IW_RETRY_LIFETIME) 
		return -EINVAL;
	
	if (wrqu->retry.flags & IW_RETRY_MAX) {
		wrqu->retry.flags = IW_RETRY_LIMIT | IW_RETRY_MAX;
		wrqu->retry.value = priv->retry_rts;
	} else {
		wrqu->retry.flags = IW_RETRY_LIMIT | IW_RETRY_MIN;
		wrqu->retry.value = priv->retry_data;
	}
	//printk("returning %d",wrqu->retry.value);
	

	return 0;
}

static int r8192_wx_get_sens(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	if(priv->rf_set_sens == NULL) 
		return -1; // we have not this support for this radio */
	wrqu->sens.value = priv->sens;
	return 0;
}


static int r8192_wx_set_sens(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	
	struct r8192_priv *priv = rtllib_priv(dev);
	
	short err = 0;
	down(&priv->wx_sem);
	//DMESG("attempt to set sensivity to %ddb",wrqu->sens.value);
	if(priv->rf_set_sens == NULL) {
		err= -1; // we have not this support for this radio */
		goto exit;
	}
	if(priv->rf_set_sens(dev, wrqu->sens.value) == 0)
		priv->sens = wrqu->sens.value;
	else
		err= -EINVAL;

exit:
	up(&priv->wx_sem);
	
	return err;
}

#if (WIRELESS_EXT >= 18)
#if 0
static int r8192_wx_get_enc_ext(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	int ret = 0;
	ret = rtllib_wx_get_encode_ext(priv->rtllib, info, wrqu, extra);
	return ret;
}
#endif
//hw security need to reorganized.
static int r8192_wx_set_enc_ext(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra)
{
	int ret=0;
// Modified by Albert 2009/09/22
// If the wireless extension version is greater than 18,
// it means the kernel had supported.
// The older kernel version can support more functions by
// patching the wireless extension and the kernel version is still the same.

#if (WIRELESS_EXT >= 18)
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	//printk("===>%s()\n", __FUNCTION__);

#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND
	if (priv->is_suspended) {
		printk("%s-%d: autoresume...\n", __FUNCTION__, __LINE__);
#ifdef ENABLE_UNASSOCIATED_USB_AUTOSUSPEND
		usb_autopm_disable(priv->intf);
#else
		rtl8192U_resume(priv->intf);
#endif
	}
        priv->suspend_delay_cnt = 0;
#endif
	
	down(&priv->wx_sem);

        priv->rtllib->wx_set_enc = 1;
#ifdef ENABLE_IPS
        down(&priv->rtllib->ips_sem);
        IPSLeave(dev);
        up(&priv->rtllib->ips_sem);			
#endif

	ret = rtllib_wx_set_encode_ext(priv->rtllib, info, wrqu, extra);
	
	{
		u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
		u8 zero[6] = {0};
		u32 key[4] = {0};
		struct iw_encode_ext *ext = (struct iw_encode_ext *)extra;
		struct iw_point *encoding = &wrqu->encoding;
#if 0
		static u8 CAM_CONST_ADDR[4][6] = {
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x03}};
#endif
		u8 idx = 0, alg = 0, group = 0;
		if ((encoding->flags & IW_ENCODE_DISABLED) ||
		ext->alg == IW_ENCODE_ALG_NONE) //none is not allowed to use hwsec WB 2008.07.01 
		{
			ieee->pairwise_key_type = ieee->group_key_type = KEY_TYPE_NA;
			CamResetAllEntry(dev);
			goto end_hw_sec;
		}
		alg =  (ext->alg == IW_ENCODE_ALG_CCMP)?KEY_TYPE_CCMP:ext->alg; // as IW_ENCODE_ALG_CCMP is defined to be 3 and KEY_TYPE_CCMP is defined to 4;
		idx = encoding->flags & IW_ENCODE_INDEX;
		if (idx)
			idx --;
		group = ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY;

		if ((!group) || (IW_MODE_ADHOC == ieee->iw_mode) || (alg ==  KEY_TYPE_WEP40))
		{
			if ((ext->key_len == 13) && (alg == KEY_TYPE_WEP40) )
				alg = KEY_TYPE_WEP104;
			ieee->pairwise_key_type = alg;
			EnableHWSecurityConfig8192(dev);
		}
		memcpy((u8*)key, ext->key, 16); //we only get 16 bytes key.why? WB 2008.7.1
		
		if ((alg & KEY_TYPE_WEP40) && (ieee->auth_mode !=2) )
		{

			setKey( dev,
					idx,//EntryNo
					idx, //KeyIndex 
					alg,  //KeyType
					zero, //MacAddr
					0,              //DefaultKey
					key);           //KeyContent
		}
		else if (group)
		{
			ieee->group_key_type = alg;
			setKey( dev,
					idx,//EntryNo
					idx, //KeyIndex 
					alg,  //KeyType
					broadcast_addr, //MacAddr
					0,              //DefaultKey
					key);           //KeyContent
		}
		else //pairwise key
		{
			setKey( dev,
					4,//EntryNo
					idx, //KeyIndex 
					alg,  //KeyType
					(u8*)ieee->ap_mac_addr, //MacAddr
					0,              //DefaultKey
					key);           //KeyContent
		}


	}

end_hw_sec:
        priv->rtllib->wx_set_enc = 0;
	up(&priv->wx_sem);
#endif
	return ret;	

}
static int r8192_wx_set_auth(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *data, char *extra)
{
	int ret=0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	//printk("====>%s()\n", __FUNCTION__);
	struct r8192_priv *priv = rtllib_priv(dev);
	down(&priv->wx_sem);
	ret = rtllib_wx_set_auth(priv->rtllib, info, &(data->param), extra);
	up(&priv->wx_sem);
#endif
	return ret;
}

static int r8192_wx_set_mlme(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra)
{
	//printk("====>%s()\n", __FUNCTION__);

	int ret=0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct r8192_priv *priv = rtllib_priv(dev);
	down(&priv->wx_sem);
	ret = rtllib_wx_set_mlme(priv->rtllib, info, wrqu, extra);

	up(&priv->wx_sem);
#endif
	return ret;
}
#endif

/*
		for PMK cache
		
        struct iw_pmksa
        {
            __u32   cmd;
            struct sockaddr bssid;
            __u8    pmkid[IW_PMKID_LEN];   //IW_PMKID_LEN=16
        }
        There are the BSSID information in the bssid.sa_data array.
        If cmd is IW_PMKSA_FLUSH, it means the wpa_suppplicant wants to clear all the PMKID information.
        If cmd is IW_PMKSA_ADD, it means the wpa_supplicant wants to add a PMKID/BSSID to driver.
        If cmd is IW_PMKSA_REMOVE, it means the wpa_supplicant wants to remove a PMKID/BSSID from driver.
*/

#if (WIRELESS_EXT >= 18)
static int r8192_wx_set_pmkid(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *wrqu, char *extra)
{
	int i;
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	struct iw_pmksa*  pPMK = (struct iw_pmksa*)extra;
	int	intReturn = false;
	
	switch (pPMK->cmd)
	{
		case IW_PMKSA_ADD:	
			for (i = 0; i < NUM_PMKID_CACHE; i++)
			{
				if (memcmp(ieee->PMKIDList[i].Bssid, pPMK->bssid.sa_data, ETH_ALEN) == 0)
				{ 
					// BSSID is matched, the same AP => rewrite with new PMKID.
                    			memcpy(ieee->PMKIDList[i].PMKID, pPMK->pmkid, IW_PMKID_LEN);
					memcpy(ieee->PMKIDList[i].Bssid, pPMK->bssid.sa_data, ETH_ALEN);
					ieee->PMKIDList[i].bUsed = true;
					intReturn = true;
					goto __EXIT__;
				}	
			}
			
			// Find a new entry
			for (i = 0; i < NUM_PMKID_CACHE; i++)
			{
				if (ieee->PMKIDList[i].bUsed == false)
				{ 
					// BSSID is matched, the same AP => rewrite with new PMKID.
					memcpy(ieee->PMKIDList[i].PMKID, pPMK->pmkid, IW_PMKID_LEN);
					memcpy(ieee->PMKIDList[i].Bssid, pPMK->bssid.sa_data, ETH_ALEN);
					ieee->PMKIDList[i].bUsed = true;
					intReturn = true;
					goto __EXIT__;
				}	
			}
			break;
				
		case IW_PMKSA_REMOVE:
			for (i = 0; i < NUM_PMKID_CACHE; i++)
			{
				if (memcmp(ieee->PMKIDList[i].Bssid, pPMK->bssid.sa_data, ETH_ALEN) == true)
				{
					// BSSID is matched, the same AP => Remove this PMKID information and reset it. 
					memset(&ieee->PMKIDList[i], 0x00, sizeof(RT_PMKID_LIST));
					intReturn = true;
					break;
				}	
	        }
			break;
			
		case IW_PMKSA_FLUSH:
			memset(&ieee->PMKIDList[0], 0x00, (sizeof(RT_PMKID_LIST) * NUM_PMKID_CACHE));
			//psecuritypriv->PMKIDIndex = 0;
            intReturn = true;
			break;
			
		default:	
			break;
	}
	
__EXIT__:	
	return (intReturn);
	
}                                       
#endif

static int r8192_wx_set_gen_ie(struct net_device *dev,
                                        struct iw_request_info *info,
                                        union iwreq_data *data, char *extra)
{
	   //printk("====>%s(), len:%d\n", __FUNCTION__, data->length);
	int ret=0;
#if (WIRELESS_EXT >= 18)
        struct r8192_priv *priv = rtllib_priv(dev);
        down(&priv->wx_sem);
#if 1
        ret = rtllib_wx_set_gen_ie(priv->rtllib, extra, data->data.length);
#endif
        up(&priv->wx_sem);
	//printk("<======%s(), ret:%d\n", __FUNCTION__, ret);
#endif
        return ret;


}

static int dummy(struct net_device *dev, struct iw_request_info *a,
		 union iwreq_data *wrqu,char *b)
{
	return -1;
}


static iw_handler r8192_wx_handlers[] =
{
        NULL,                     /* SIOCSIWCOMMIT */
        r8192_wx_get_name,   	  /* SIOCGIWNAME */
        dummy,                    /* SIOCSIWNWID */
        dummy,                    /* SIOCGIWNWID */
        r8192_wx_set_freq,        /* SIOCSIWFREQ */
        r8192_wx_get_freq,        /* SIOCGIWFREQ */
        r8192_wx_set_mode,        /* SIOCSIWMODE */
        r8192_wx_get_mode,        /* SIOCGIWMODE */
        r8192_wx_set_sens,        /* SIOCSIWSENS */
        r8192_wx_get_sens,        /* SIOCGIWSENS */
        NULL,                     /* SIOCSIWRANGE */
        rtl8180_wx_get_range,	  /* SIOCGIWRANGE */
        NULL,                     /* SIOCSIWPRIV */
        NULL,                     /* SIOCGIWPRIV */
        NULL,                     /* SIOCSIWSTATS */
        NULL,                     /* SIOCGIWSTATS */
        dummy,                    /* SIOCSIWSPY */
        dummy,                    /* SIOCGIWSPY */
        NULL,                     /* SIOCGIWTHRSPY */
        NULL,                     /* SIOCWIWTHRSPY */
        r8192_wx_set_wap,      	  /* SIOCSIWAP */
        r8192_wx_get_wap,         /* SIOCGIWAP */
#if (WIRELESS_EXT >= 18)
        r8192_wx_set_mlme,                     /* MLME-- */
#else
	 NULL,
#endif
        dummy,                     /* SIOCGIWAPLIST -- depricated */
        r8192_wx_set_scan,        /* SIOCSIWSCAN */
        r8192_wx_get_scan,        /* SIOCGIWSCAN */
        r8192_wx_set_essid,       /* SIOCSIWESSID */
        r8192_wx_get_essid,       /* SIOCGIWESSID */
        dummy,                    /* SIOCSIWNICKN */
        dummy,                    /* SIOCGIWNICKN */
        NULL,                     /* -- hole -- */
        NULL,                     /* -- hole -- */
        r8192_wx_set_rate,        /* SIOCSIWRATE */
        r8192_wx_get_rate,        /* SIOCGIWRATE */
        r8192_wx_set_rts,                    /* SIOCSIWRTS */
        r8192_wx_get_rts,                    /* SIOCGIWRTS */
        r8192_wx_set_frag,        /* SIOCSIWFRAG */
        r8192_wx_get_frag,        /* SIOCGIWFRAG */
        dummy,                    /* SIOCSIWTXPOW */
        dummy,                    /* SIOCGIWTXPOW */
        r8192_wx_set_retry,       /* SIOCSIWRETRY */
        r8192_wx_get_retry,       /* SIOCGIWRETRY */
        r8192_wx_set_enc,         /* SIOCSIWENCODE */
        r8192_wx_get_enc,         /* SIOCGIWENCODE */
        r8192_wx_set_power,                    /* SIOCSIWPOWER */
        r8192_wx_get_power,                    /* SIOCGIWPOWER */
	NULL,			/*---hole---*/
	NULL, 			/*---hole---*/
	r8192_wx_set_gen_ie,//NULL, 			/* SIOCSIWGENIE */
	NULL, 			/* SIOCSIWGENIE */
	
#if (WIRELESS_EXT >= 18)
	r8192_wx_set_auth,//NULL, 			/* SIOCSIWAUTH */
	NULL,//r8192_wx_get_auth,//NULL, 			/* SIOCSIWAUTH */
	r8192_wx_set_enc_ext, 			/* SIOCSIWENCODEEXT */
	NULL,//r8192_wx_get_enc_ext,//NULL, 			/* SIOCSIWENCODEEXT */
	r8192_wx_set_pmkid, 			/* SIOCSIWPMKSA */
#else
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, 			 /*---hole---*/
#endif
	r8192_wx_set_pmkid, 			/* SIOCSIWPMKSA */
	NULL, 			 /*---hole---*/
	
}; 

/* 
 * the following rule need to be follwing,
 * Odd : get (world access), 
 * even : set (root access) 
 * */
static const struct iw_priv_args r8192_private_args[] = { 
	
	{
		SIOCIWFIRSTPRIV + 0x0, 
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "badcrc" 
	}, 
	
	{
		SIOCIWFIRSTPRIV + 0x1,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "activescan"
	
	},
	{
		SIOCIWFIRSTPRIV + 0x2, 
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "rawtx" 
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x3,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "forcereset"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x4,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "force_mic_error"

	}

#ifdef RTL8192SU        
	,
	{
		SIOCIWFIRSTPRIV + 0x5,
		IW_PRIV_TYPE_NONE, IW_PRIV_TYPE_INT|IW_PRIV_SIZE_FIXED|1,
		"firm_ver"
	}
#endif

	,
	{
		SIOCIWFIRSTPRIV + 0x7,
		0, IW_PRIV_TYPE_CHAR|2047, "adhoc_peer_list"
	}

//#ifdef RTK_DMP_PLATFORM
        ,
        {
                SIOCIWFIRSTPRIV + 0x9,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "apinfo"
        }
        ,
        {
                SIOCIWFIRSTPRIV + 0xa,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "countrycode"
        }
//#endif
	,
	{
		SIOCIWFIRSTPRIV + 0xb,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED|1, IW_PRIV_TYPE_NONE, 
		"set_power"
	}
        ,
	{
		SIOCIWFIRSTPRIV + 0xc,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED|1, IW_PRIV_TYPE_NONE, 
		"lps_interv"
	}
        ,
	{
		SIOCIWFIRSTPRIV + 0xd,
		IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED|1, IW_PRIV_TYPE_NONE, 
		"lps_force"
	}
};


static iw_handler r8192_private_handler[] = {
//	r8192_wx_set_monitor,  /* SIOCIWFIRSTPRIV */
	r8192_wx_set_crcmon,   /*SIOCIWSECONDPRIV*/
//	r8192_wx_set_forceassociate,
//	r8192_wx_set_beaconinterval,
//	r8192_wx_set_monitor_type,
	r8192_wx_set_scan_type,
	r8192_wx_set_rawtx,
	r8192_wx_force_reset,
    (iw_handler)r8192_wx_force_mic_error,
#ifdef RTL8192SU        
    (iw_handler)r8191su_wx_get_firm_version,
#else
	r8192_wx_null,
#endif
	r8192_wx_null,
	r8192_wx_get_adhoc_peers,

//#ifdef RTK_DMP_PLATFORM
	r8192_wx_null,
        r8192_wx_get_ap_status, /* offset must be 9 for DMP platform*/
	r8192_wx_set_countrycode, //Set Channel depend on the country code
//#endif
    (iw_handler)r8192_wx_adapter_power_status,	    
    (iw_handler)r8192se_wx_set_lps_awake_interval,
    (iw_handler)r8192se_wx_set_force_lps,
};

//#if WIRELESS_EXT >= 17	
struct iw_statistics *r8192_get_wireless_stats(struct net_device *dev)
{
       struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* ieee = priv->rtllib;
	struct iw_statistics* wstats = &priv->wstats;
	int tmp_level = 0;
	int tmp_qual = 0;
	int tmp_noise = 0;
	if(ieee->state < RTLLIB_LINKED)
	{
		wstats->qual.qual = 0;
		wstats->qual.level = 0;
		wstats->qual.noise = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)) 
		wstats->qual.updated = IW_QUAL_ALL_UPDATED | IW_QUAL_DBM;
#else
		wstats->qual.updated = 0x0f;
#endif
		return wstats;
	}
	
       tmp_level = (&ieee->current_network)->stats.rssi;
	tmp_qual = (&ieee->current_network)->stats.signal;
	tmp_noise = (&ieee->current_network)->stats.noise;			
	//printk("level:%d, qual:%d, noise:%d\n", tmp_level, tmp_qual, tmp_noise);

	wstats->qual.level = tmp_level;
	wstats->qual.qual = tmp_qual;
	wstats->qual.noise = tmp_noise;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))
	wstats->qual.updated = IW_QUAL_ALL_UPDATED| IW_QUAL_DBM;
#else
        wstats->qual.updated = 0x0f;
#endif
	return wstats;
}
//#endif


struct iw_handler_def  r8192_wx_handlers_def={
	.standard = r8192_wx_handlers,
	.num_standard = sizeof(r8192_wx_handlers) / sizeof(iw_handler),
	.private = r8192_private_handler,
	.num_private = sizeof(r8192_private_handler) / sizeof(iw_handler),
 	.num_private_args = sizeof(r8192_private_args) / sizeof(struct iw_priv_args),
#if WIRELESS_EXT >= 17	
	.get_wireless_stats = r8192_get_wireless_stats,
#endif
	.private_args = (struct iw_priv_args *)r8192_private_args,	
};
