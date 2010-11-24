/* 
   This file contains wireless extension handlers.

   This is part of rtl8180 OpenSource driver.
   Copyright (C) Andrea Merello 2004-2005  <andreamrl@tiscali.it> 
   Released under the terms of GPL (General Public Licence)
   
   Parts of this driver are based on the GPL part 
   of the official realtek driver.
   
   Parts of this driver are based on the rtl8180 driver skeleton 
   from Patric Schenke & Andres Salomon.

   Parts of this driver are based on the Intel Pro Wireless 2100 GPL driver.
   
   We want to tanks the Authors of those projects and the Ndiswrapper 
   project Authors.
*/

#include <linux/string.h>
#include "r8187.h"
#include "r8180_hw.h"

#ifdef THOMAS_MP_IOCTL
#include "r8180_93cx6.h"   /* Card EEPROM */
#include "r8180_rtl8225.h"
#include "r8187_mp_ioctl_linux.h"
#endif

#define RATE_COUNT 4
static u32 rtl8180_rates[] = {1000000,2000000,5500000,11000000,
	6000000,9000000,12000000,18000000,24000000,36000000,48000000,54000000};
	

static int r8180_wx_get_freq(struct net_device *dev,
			     struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	return ieee80211_wx_get_freq(priv->ieee80211,a,wrqu,b);
}


#if 0

static int r8180_wx_set_beaconinterval(struct net_device *dev, struct iw_request_info *aa,
			  union iwreq_data *wrqu, char *b)
{
	int *parms = (int *)b;
	int bi = parms[0];
	
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	down(&priv->wx_sem);
	DMESG("setting beacon interval to %x",bi);
	
	priv->ieee80211->beacon_interval=bi;
	rtl8180_commit(dev);
	up(&priv->wx_sem);
		
	return 0;	
}


static int r8180_wx_set_forceassociate(struct net_device *dev, struct iw_request_info *aa,
			  union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv=ieee80211_priv(dev);	
	int *parms = (int *)extra;
	
	priv->ieee80211->force_associate = (parms[0] > 0);
	

	return 0;
}

#endif
static int r8180_wx_get_mode(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8180_priv *priv=ieee80211_priv(dev);	

	return ieee80211_wx_get_mode(priv->ieee80211,a,wrqu,b);
}



static int r8180_wx_get_rate(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	return ieee80211_wx_get_rate(priv->ieee80211,info,wrqu,extra);
}



static int r8180_wx_set_rate(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	int ret;
	struct r8180_priv *priv = ieee80211_priv(dev);	
	
	down(&priv->wx_sem);

	ret = ieee80211_wx_set_rate(priv->ieee80211,info,wrqu,extra);
	
	up(&priv->wx_sem);
	
	return ret;
}

#ifdef JOHN_IOCTL
u16 read_rtl8225(struct net_device *dev, u8 addr);
void write_rtl8225(struct net_device *dev, u8 adr, u16 data);
void rtl8187_write_phy(struct net_device *dev, u8 adr, u32 data);
u8 rtl8187_read_phy(struct net_device *dev,u8 adr, u32 data);

static int r8180_wx_read_regs(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	u8 addr;
	u16 data1;

	down(&priv->wx_sem);

	
	get_user(addr,(u8*)wrqu->data.pointer);
	data1 = read_rtl8225(dev, addr);
	wrqu->data.length = data1;	

	up(&priv->wx_sem);
	return 0;
	 
}

static int r8180_wx_write_regs(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
        struct r8180_priv *priv = ieee80211_priv(dev);
        u8 addr;

        down(&priv->wx_sem);
             
        get_user(addr, (u8*)wrqu->data.pointer);
	write_rtl8225(dev, addr, wrqu->data.length);

        up(&priv->wx_sem);
	return 0;

}


static int r8180_wx_read_bb(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
        struct r8180_priv *priv = ieee80211_priv(dev);
	u8 databb;
        down(&priv->wx_sem);

	databb = rtl8187_read_phy(dev, (u8)wrqu->data.length, 0x00000000);
	wrqu->data.length = databb;

	up(&priv->wx_sem);
	return 0;
}


static int r8180_wx_write_bb(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8180_priv *priv = ieee80211_priv(dev);
        u8 databb;

        down(&priv->wx_sem);

        get_user(databb, (u8*)wrqu->data.pointer);
        rtl8187_write_phy(dev, wrqu->data.length, databb);

        up(&priv->wx_sem);
        return 0;

}


static int r8180_wx_write_nicb(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8180_priv *priv = ieee80211_priv(dev);
        u32 addr;

        down(&priv->wx_sem);
	
        get_user(addr, (u32*)wrqu->data.pointer);
        write_nic_byte(dev, addr, wrqu->data.length);

        up(&priv->wx_sem);
        return 0;

}
static int r8180_wx_read_nicb(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8180_priv *priv = ieee80211_priv(dev);
        u32 addr;
        u16 data1;

        down(&priv->wx_sem);


        printk("@@@@ Transmit timeout at %ld, latency %ld\n", jiffies,
                        jiffies - dev->trans_start);

        printk("@@@@ netif_queue_stopped = %d\n", netif_queue_stopped(dev));

        get_user(addr,(u32*)wrqu->data.pointer);
        data1 = read_nic_byte(dev, addr);
        wrqu->data.length = data1;

        up(&priv->wx_sem);
        return 0;
}

static inline int is_same_network(struct ieee80211_network *src,
                                  struct ieee80211_network *dst)
{
        /* A network is only a duplicate if the channel, BSSID, ESSID
         * and the capability field (in particular IBSS and BSS) all match.  
         * We treat all <hidden> with the same BSSID and channel
         * as one network */
        return ((src->ssid_len == dst->ssid_len) &&
                //(src->channel == dst->channel) &&
                !memcmp(src->bssid, dst->bssid, ETH_ALEN) &&
                !memcmp(src->ssid, dst->ssid, src->ssid_len) &&
                ((src->capability & WLAN_CAPABILITY_IBSS) ==
                (dst->capability & WLAN_CAPABILITY_IBSS)) &&
                ((src->capability & WLAN_CAPABILITY_BSS) ==
                (dst->capability & WLAN_CAPABILITY_BSS)));
}

static int r8180_wx_get_ap_status(struct net_device *dev,
                               struct iw_request_info *info,
                               union iwreq_data *wrqu, char *extra)
{
        struct r8180_priv *priv = ieee80211_priv(dev);
        struct ieee80211_device *ieee = priv->ieee80211;
        struct ieee80211_network *target;
	int name_len;

        down(&priv->wx_sem);

	//count the length of input ssid
	for(name_len=0 ; ((char*)wrqu->data.pointer)[name_len]!='\0' ; name_len++);

	//search for the correspoding info which is received
        list_for_each_entry(target, &ieee->network_list, list) {
                if ( (target->ssid_len == name_len) &&
		     (strncmp(target->ssid, (char*)wrqu->data.pointer, name_len)==0)){
			if( ((jiffies-target->last_scanned)/HZ > 1) && (ieee->state == IEEE80211_LINKED) && (is_same_network(&ieee->current_network,target)) )
				wrqu->data.length = 999;
			else
#if 0				
				wrqu->data.length = target->SignalStrength;
#else
			{
				if(target->SignalStrength>100) 
				{
				    wrqu->data.length = 101;
				}
				else if(target->SignalStrength==0)
				{
				    wrqu->data.length = 0;
				}
				else
				{
					// EQ value = 50 * Log (X)
					int iStrength[]={0,50,65,74,80,85,89,92,95,98,100};
					int iIndex=(target->SignalStrength/10);
					int mod = target->SignalStrength%10;
					wrqu->data.length = iStrength[iIndex]+mod*(iStrength[iIndex+1]-iStrength[iIndex])/10;
				}
			} 
#endif			
			if(target->wpa_ie_len>0 || target->rsn_ie_len>0 ) //set flags=1 to indicate this ap is WPA
				wrqu->data.flags = 1;
			else wrqu->data.flags = 0;
		
	
			break;
                }
        }
	
#if 0	
	if (&target->list == &ieee->network_list){
		wrqu->data.flags = 3;
	}
#endif


        up(&priv->wx_sem);
        return 0;
}

#endif

#ifdef THOMAS_MP_IOCTL
static  int r8180_wx_mp_ioctl_hdl(struct net_device *dev, struct iw_request_info *info,
						union iwreq_data *wrqu, char *extra)
{
	int	ret;
	u16	len;
	struct mp_ioctl_handler	*phandler;
	struct mp_ioctl_param		*poidparam;
	u8	*pparmbuf = NULL, bset;
	struct r8180_priv	*priv = ieee80211_priv(dev);
	struct iw_point	*p = &wrqu->data;

	//printk("+r871x_mp_ioctl_hdl\n");

	down(&priv->wx_sem);

	if( (!p->length) || (!p->pointer)){
		ret = -EINVAL;
		goto _r871x_mp_ioctl_hdl_exit;
	}
	
	pparmbuf = NULL;
	bset = (u8)(p->flags&0xFFFF);
	len = p->length;
	pparmbuf = (u8*)kmalloc(len, GFP_ATOMIC);
	if (pparmbuf == NULL){
		ret = -ENOMEM;
		goto _r871x_mp_ioctl_hdl_exit;
	}
	
	if(bset)//set info
	{
		if (copy_from_user(pparmbuf, p->pointer, len)) {
			kfree(pparmbuf);
			ret = -EFAULT;
			goto _r871x_mp_ioctl_hdl_exit;
		}		

		//_memcpy(pparmbuf, p->pointer, len);
	}
	else//query info
	{
		if (copy_from_user(pparmbuf, p->pointer, len)) {
			kfree(pparmbuf);
			ret = -EFAULT;
			goto _r871x_mp_ioctl_hdl_exit;
		}	
		
		//_memcpy(pparmbuf, p->pointer, len);
	}

	poidparam = (struct mp_ioctl_param *)pparmbuf;	
	
	//DMESG("r871x_mp_ioctl_hdl: subcode [%d], len[%d], buffer_len[%d]\r\n",poidparam->subcode, poidparam->len, len);

	if ( poidparam->subcode >= MAX_MP_IOCTL_SUBCODE)
	{
		DMESGW("no matching drvext subcodes\r\n");
		ret = -EINVAL;
		goto _r871x_mp_ioctl_hdl_exit;
	}


	//DMESG("+r871x_mp_ioctl_hdl(%d)\n",poidparam->subcode);

	phandler = mp_ioctl_hdl + poidparam->subcode;

	if((phandler->paramsize!=0)&&(poidparam->len != phandler->paramsize))
	{
		DMESGW("no matching drvext param size %d vs %d\r\n",poidparam->len , phandler->paramsize);
		ret = -EINVAL;		
		goto _r871x_mp_ioctl_hdl_exit;
	}

	ret = phandler->handler(dev, bset, poidparam->data);

	if(bset==0x00)//query info
	{
		//_memcpy(p->pointer, pparmbuf, len);
		if(copy_to_user(p->pointer, pparmbuf, len))
		{
			ret = -EFAULT;
		}		
	}
	
		
_r871x_mp_ioctl_hdl_exit:

	if(pparmbuf)
		kfree(pparmbuf);

	up(&priv->wx_sem);
	
	return ret;	

}

void ResetRxStatistics(struct r8180_priv	*priv)
{
	priv->stats.rxok = 0;
	priv->stats.rxstaterr= 0;
	priv->stats.rxurberr= 0;
}

void ResetTxStatistics(struct r8180_priv	*priv)
{
	priv->stats.txbkokint = 0;
	priv->stats.txbkerr = 0;
	priv->stats.txbkdrop = 0;
	priv->stats.txbeokint = 0;
	priv->stats.txbeerr = 0;
	priv->stats.txbedrop = 0;
	priv->stats.txviokint = 0;
	priv->stats.txvierr = 0;
	priv->stats.txvidrop = 0;
	priv->stats.txvookint = 0;
	priv->stats.txvoerr = 0;
	priv->stats.txvodrop = 0;
	priv->stats.txbeaconokint = 0;
	priv->stats.txbeaconerr = 0;
	priv->stats.txbeacondrop = 0;
	priv->stats.txmanageokint = 0;
	priv->stats.txmanageerr = 0;
	priv->stats.txmanagedrop = 0;
}


#define MP_DEFAULT_TX_ESSID "rtl8187b_mp_test"

int mp_ioctl_start_hdl(struct net_device *dev, unsigned char set, void*param)
{
	int ret = 0;
	int bstart;
	struct r8180_priv	*priv = ieee80211_priv(dev);

	//DMESG("+mp_ioctl_start_hdl\n");

	if(param==NULL)
		return -1;
	
	bstart = *(int*)param;
	if(set)
	{
		if(bstart == MP_START_MODE)
		{
			u8 Index, btMsr = 0;
			u32 ReceiveConfig = 0;
			union iwreq_data wrqu;
			struct ieee80211_device *ieee = priv->ieee80211;

			//init mp_start_test status
			priv->MPState = 1;

			ieee80211_softmac_stop_protocol(ieee);
		
			//memset(&ieee->current_network, 0 , sizeof(struct ieee80211_network));

			ieee->iw_mode = IW_MODE_ADHOC;
			for(Index=0; Index<6; Index++)
			{
				ieee->current_network.bssid[Index] = Index;
			}

			strcpy(ieee->current_network.ssid,MP_DEFAULT_TX_ESSID);
			ieee->current_network.ssid_len = strlen(MP_DEFAULT_TX_ESSID);
			ieee->ssid_set = 1;

			if(ieee->modulation & IEEE80211_CCK_MODULATION){
			
				ieee->current_network.rates_len = 4;
				
				ieee->current_network.rates[0] = IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_1MB;
				ieee->current_network.rates[1] = IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_2MB;
				ieee->current_network.rates[2] = IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_5MB;
				ieee->current_network.rates[3] = IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_11MB;
					
			}else
				ieee->current_network.rates_len = 0;
			
			if(ieee->modulation & IEEE80211_OFDM_MODULATION){
				ieee->current_network.rates_ex_len = 8;
				
				ieee->current_network.rates_ex[0] = IEEE80211_BASIC_RATE_MASK | IEEE80211_OFDM_RATE_6MB;
				ieee->current_network.rates_ex[1] = IEEE80211_BASIC_RATE_MASK | IEEE80211_OFDM_RATE_9MB;
				ieee->current_network.rates_ex[2] = IEEE80211_BASIC_RATE_MASK | IEEE80211_OFDM_RATE_12MB;
				ieee->current_network.rates_ex[3] = IEEE80211_BASIC_RATE_MASK | IEEE80211_OFDM_RATE_18MB;
				ieee->current_network.rates_ex[4] = IEEE80211_BASIC_RATE_MASK | IEEE80211_OFDM_RATE_24MB;
				ieee->current_network.rates_ex[5] = IEEE80211_BASIC_RATE_MASK | IEEE80211_OFDM_RATE_36MB;
				ieee->current_network.rates_ex[6] = IEEE80211_BASIC_RATE_MASK | IEEE80211_OFDM_RATE_48MB;
				ieee->current_network.rates_ex[7] = IEEE80211_BASIC_RATE_MASK | IEEE80211_OFDM_RATE_54MB;
				
				ieee->rate = 540;
			}else{
				ieee->current_network.rates_ex_len = 0;
				ieee->rate = 110;
			}

			// By default, WMM function will be disabled in IBSS mode
			ieee->current_network.QoS_Enable = 0;
		
			ieee->current_network.atim_window = 0;
			ieee->current_network.capability = WLAN_CAPABILITY_IBSS;
			if(ieee->short_slot)
				ieee->current_network.capability |= WLAN_CAPABILITY_SHORT_SLOT;

			ieee->state = IEEE80211_LINKED;
			wrqu.ap_addr.sa_family = ARPHRD_ETHER;
			memcpy(wrqu.ap_addr.sa_data, ieee->current_network.bssid, ETH_ALEN);
			wireless_send_event(ieee->dev, SIOCGIWAP, &wrqu, NULL);

			netif_carrier_on(dev);

			// Make sure we won't enable CTS-to-SELF, RTS/CTS, and fragmentation.
			ieee->fts = 3000;
			priv->rts = 3000;

			// Switch no link.
			//eee->state = IEEE80211_NOLINK;
			btMsr = MSR_LINK_ENEDCA;
			btMsr |= MSR_LINK_NONE << MSR_LINK_SHIFT;
			write_nic_byte(dev, MSR, btMsr);

			// Set up RCR.
			// 1. Accept control frame might cause Rx completed with USBD_STATUS_BABBLE_DETECTED.
			// 2. Accept CRC32 error might cause Rx completed with USBD_STATUS_BABBLE_DETECTED.
			// 2006.05.19, by rcnjko.
			//
			// I set ACF and ACRC32 and restrict RMS to 2000 bytes to prevent USB babbled. 
			// 2006.05.26, by rcnjko.
			ReceiveConfig = 0xB014F70F;
			ReceiveConfig = RCR_ONLYERLPKT | RCR_EnCS1|  
						RCR_AMF | RCR_ADF |  RCR_ACF | 
						RCR_RXFTH | // Rx FIFO Threshold, 7: No Rx threshold.
					 	RCR_AICV | RCR_ACRC32 |
						RCR_MXDMA | // MAX DMA Burst Size per Rx DMA Burst, 7: Unlimited.
						RCR_AB | RCR_AM | RCR_APM | RCR_AAP;

			// Turn on RCR_ACRC32 because packet recived in high power state will become CRC 
			// and we still need to check AGC of these CRC error packet to adjust TR switch. 
			// 2006.02.15, by rcnjko.
			ReceiveConfig |= RCR_ACRC32;

			write_nic_dword(dev, RCR, ReceiveConfig);

			write_nic_word(dev, RMS, 2000);

			//switch to B mode
			ActSetWirelessMode8187(dev, WIRELESS_MODE_B);

			//set date rate
			ieee->rate = 10;
			write_nic_word(dev, BRSR_8187B, BIT0);

			//set channel to 1
			ieee->current_network.channel = 1;
			ieee->set_chan(ieee->dev, ieee->current_network.channel);
			
			//Switch to Antenna A.
			write_nic_dword(dev, PhyAddr, 0x01009b90);
			write_nic_dword(dev, PhyAddr, 0x90a6);
			write_nic_byte(dev, 0xff9f, 0x3);

			//Set Preamble Long
			priv->ieee80211->current_network.capability &= ~WLAN_CAPABILITY_SHORT_PREAMBLE;

			ResetRxStatistics(priv);
			ResetTxStatistics(priv);
		}
	}
	else
	{
		return -1;
	}
	
	return ret;

}

int mp_ioctl_stop_hdl(struct net_device *dev, unsigned char set, void*param)
{
	int ret = 0;
	int bstop;
	struct r8180_priv	*priv = ieee80211_priv(dev);

	//DMESG("+mp_ioctl_stop_hdl\n");

	bstop = *(int*)param;
	if(param==NULL)
		return -1;

	if(set)
	{
		if(bstop == MP_STOP_MODE)
		{
			u8 btMsr = 0;
			struct ieee80211_device *ieee = priv->ieee80211;
		
			priv->MPState = 0;

			ieee->fts = DEFAULT_FRAG_THRESHOLD;
			priv->rts = 2346;

			// Switch no link.
			ieee->state = IEEE80211_NOLINK;
			btMsr = MSR_LINK_ENEDCA;
			btMsr |= MSR_LINK_NONE << MSR_LINK_SHIFT;
			write_nic_byte(dev, MSR, btMsr);

			write_nic_dword(dev, RCR, priv->ReceiveConfig);

			write_nic_word(dev, RMS, 0x0800);

			write_nic_word(dev, BRSR_8187B, 0xfff);

			ieee->iw_mode = IW_MODE_INFRA;

			ieee->ssid_set = 0;

			ieee80211_softmac_stop_protocol(ieee);

			//memset(&ieee->current_network, 0 , sizeof(struct ieee80211_network));

			ResetRxStatistics(priv);

			ResetTxStatistics(priv);

			ieee80211_softmac_start_protocol(ieee);

			if(!netif_queue_stopped(dev))
				netif_start_queue(dev);
			else
				netif_wake_queue(dev);
		}
	}
	else
	{
		return -1;
	}

	return ret;

}


int mp_ioctl_set_ch_modulation_hdl(struct net_device *dev, unsigned char set, void *param)
{
	u8	btWirelessMode = 0;
	struct r8180_priv	*priv = ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;	
	struct rfchannel_param*prfchannel = (struct rfchannel_param*)param;

	if(prfchannel==NULL)
		return -1;

	//printk("set ch=0x%x, modem=0x%x\n", prfchannel->ch, prfchannel->modem);

	if(set)
	{
		btWirelessMode = prfchannel->modem;

		if(priv->ieee80211->mode != btWirelessMode)
			ActSetWirelessMode8187(dev, btWirelessMode);

		ieee->current_network.channel = prfchannel->ch;
		ieee->set_chan(ieee->dev, ieee->current_network.channel);
	}
	else
	{
		return -1;
	}

	return 0;


}


int mp_ioctl_set_txpower_hdl(struct net_device *dev, unsigned char set, void *param)
{
	unsigned short pi;
	struct r8180_priv *priv = ieee80211_priv(dev);
	struct txpower_param*ptxpwr = (struct txpower_param*)param;

	//printk("+mp_ioctl_set_txpower_hdl\n");

	if(ptxpwr==NULL)
		return -1;

	pi = (unsigned short)ptxpwr->pwr_index;

	if(pi > 35)
		return -1;

	//printk("+mp_ioctl_set_txpower, index=0x%x\n", pi);

	if(set)
	{
		if(priv->ieee80211->mode == WIRELESS_MODE_B)
			priv->chtxpwr[priv->ieee80211->current_network.channel] = pi;
		else
			priv->chtxpwr_ofdm[priv->ieee80211->current_network.channel] = pi;

		rtl8225z2_SetTXPowerLevel(dev, priv->ieee80211->current_network.channel);
	}
	else
	{
		return -1;
	}

	return 0;
}


static u16 rtl_rate[] = {10,20,55,110,60,90,120,180,240,360,480,540};
int mp_ioctl_set_datarate_hdl(struct net_device *dev, unsigned char set, void *param)
{
	unsigned char idx;
	struct datarate_param *pdatarate = (struct datarate_param*)param;
	struct r8180_priv *priv = ieee80211_priv(dev);

	idx = (unsigned char)pdatarate->rate_index;

	//printk("+mp_ioctl_set_datarate_hdl, rate idx:%d\n",idx);

	if(set)
	{			
		priv->ieee80211->rate = rtl_rate[idx];
		write_nic_word(dev, BRSR_8187B, (1<<idx));
	}
	else
	{
		return -1;
	}

	return 0;
}


int mp_ioctl_read_reg_hdl(struct net_device *dev, unsigned char set, void*param)
{
	int ret = 0;
	struct rwreg_param*prwreg = (struct rwreg_param*)param;

	if(prwreg==NULL)
		return -1;

	if(set)
	{
		ret = -1;
	}
	else
	{
		switch(prwreg->width)
		{
			case 1:
		               prwreg->value=read_nic_byte(dev, prwreg->offset);
			       break;
			case 2:
		               prwreg->value=read_nic_word(dev, prwreg->offset);
		  	       break;
			case 4:
		               prwreg->value=read_nic_dword(dev, prwreg->offset);
		  	       break;
			default:
                		ret = -1;
			   	break;
		}
	}

	/*if(ret!=-1){
		if(prwreg->width==1){
			printk("+mp_ioctl_read_reg_hdl addr:0x%08x byte:%d value:0x%02x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
		else if(prwreg->width==2){
			printk("+mp_ioctl_read_reg_hdl addr:0x%08x byte:%d value:0x%04x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
		else if(prwreg->width==4){
			printk("+mp_ioctl_read_reg_hdl addr:0x%08x byte:%d value:0x%08x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
	}*/

	return ret;
}


int mp_ioctl_write_reg_hdl(struct net_device *dev, unsigned char set, void *param)
{
	int ret;
	struct rwreg_param*prwreg = (struct rwreg_param*)param;
	
	if(prwreg==NULL)		return -1;

	ret = 0;
	if(set)
	{
		   switch(prwreg->width)
      		   {
            		case 1:
				//printk("+mp_ioctl_write_reg_hdl - write8 \n");
				write_nic_byte(dev, prwreg->offset, (unsigned char)prwreg->value);	  	
			    	break;						
	     		case 2:
				//printk("+mp_ioctl_write_reg_hdl - write16 \n");
		    		write_nic_word(dev, prwreg->offset, (unsigned short)prwreg->value);	
		  	  	break;												
	     		case 4:
				//printk("+mp_ioctl_write_reg_hdl - write32 \n");
		   		write_nic_dword(dev, prwreg->offset, (unsigned int)prwreg->value);	  	
		  	   	break;
	     		default:
               	 	ret = -1;
			   	break;
        	   }        
	
	}
	else
	{
		ret = -1;
	}

	/*if(ret!=-1){
		if(prwreg->width==1){
			printk("+mp_ioctl_write_reg_hdl addr:0x%08x byte:%d value:0x%02x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
		else if(prwreg->width==2){
			printk("+mp_ioctl_write_reg_hdl addr:0x%08x byte:%d value:0x%04x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
		else if(prwreg->width==4){
			printk("+mp_ioctl_write_reg_hdl addr:0x%08x byte:%d value:0x%08x\n",prwreg->offset,prwreg->width,prwreg->value);
	       }
	}*/

	return ret;
}


int mp_ioctl_read_bbreg_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct bbreg_param *pbbreg = (struct bbreg_param*)param;

	if(pbbreg==NULL)
		return -1;

	if(set)
		return -1;

	//printk("+mp_ioctl_read_bbreg_hdl offset:0x%02x \n",pbbreg->offset);

	write_nic_dword(dev, PhyAddr, pbbreg->offset);
	pbbreg->value = read_nic_byte(dev, PhyDataR);

	return 0;
}

int mp_ioctl_write_bbreg_hdl(struct net_device *dev, unsigned char set, void *param)
{
	int ret = 0;
	struct bbreg_param*pbbreg = (struct bbreg_param*)param;

	if(pbbreg==NULL)
		return -1;

	if(!set)
		return -1;

	//printk("+mp_ioctl_write_bbreg_hdl value:0x%02x\n", pbbreg->value);

	write_nic_dword(dev, PhyAddr, pbbreg->value);

	return ret;
}

int mp_ioctl_read_rfreg_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct rwreg_param*prwreg = (struct rwreg_param*)param;

	if(prwreg==NULL)
		return -1;
	if(set)
		return -1;

	//printk("+mp_ioctl_read_rfreg_hdl offset:0x%02x \n",prwreg->offset);

	prwreg->value = read_rtl8225(dev, (unsigned char)prwreg->offset);


	return 0;
}

int mp_ioctl_write_rfreg_hdl(struct net_device *dev, unsigned char set, void *param)
{
	struct rwreg_param*prwreg = (struct rwreg_param*)param;

	//printk("+mp_ioctl_write_rfreg_hdl offset:0x%02x value:0x%02x\n",prwreg->offset,prwreg->value);

	if(prwreg==NULL)
		return -1;

	if(!set)
		return -1;

	write_rtl8225(dev, (unsigned char)prwreg->offset, (u16)prwreg->value);

	return 0;
}

int mp_ioctl_read16_eeprom_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct eeprom_rw_param *peeprom16 = (struct eeprom_rw_param *)param;

	//printk("+mp_ioctl_read16_eeprom_hdl \n");

	if(peeprom16==NULL)
		return -1;

	if(set){
		return -1;
	}
	else
	{
		peeprom16->value = Read_EEprom87B(dev, (peeprom16->offset) >> 1);
		//printk("+mp_ioctl_read16_eeprom_hdl , offset = %x, value = %x\n",peeprom16->offset, peeprom16->value);
	}

	return 0;
}

int mp_ioctl_write16_eeprom_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct eeprom_rw_param *peeprom16 = (struct eeprom_rw_param *)param;

	if(peeprom16==NULL)
		return -1;

	if(!set)
		return -1;

	return 0;
}

#if 0
typedef	union _ThreeWire{
	struct _ThreeWireStruc{
		u16		data:1;
		u16		clk:1;
		u16		enableB:1;
		u16		read_write:1;
		u16		resv1:12;
//		u16		resv2:14;
//		u16		ThreeWireEnable:1;
//		u16		resv3:1;
	}struc;
	u16		longData;
}ThreeWireReg;

void ZEBRA_RFSerialWrite(
	struct net_device *dev,
	u32		data2Write,
	u8		totalLength,
	u8		low2high
	)
{
	ThreeWireReg		twreg;
	int 				i;
	u16				oval,oval2,oval3;
	u32				mask;
	u16				UshortBuffer;

//	Suggested by StevenLin, 2005.07.01.
//	PlatformEFIOWrite4Byte(Adapter, PhyMuxPar, 0x3dc00000);	
//	WriteBB(Adapter, 0x0095);
//	WriteBB(Adapter, 0x0097);

	UshortBuffer=read_nic_word(dev, RFPinsOutput);
	//oval=(UshortBuffer|0x0008)&0xfffb;
	oval=UshortBuffer&0xfff3;
	
	oval2=read_nic_word(dev, RFPinsEnable);
	oval3=read_nic_word(dev, RFPinsSelect);

	// <RJ_NOTE> 3-wire should be controled by HW when we finish SW 3-wire programming. 2005.08.10, by rcnjko.
	oval3 &= 0xfff8;

	write_nic_word(dev, RFPinsEnable, (oval2|0x0007));	// Set To Output Enable
	write_nic_word(dev, RFPinsSelect, (oval3|0x0007));	// Set To SW Switch
	udelay(10); // 


	// Add this to avoid hardware and software 3-wire conflict.
	// 2005.03.01, by rcnjko.
	twreg.longData = 0;
	twreg.struc.enableB = 1;
	write_nic_word(dev, RFPinsOutput, (twreg.longData|oval)); // Set SI_EN (RFLE)
	udelay(2);
	twreg.struc.enableB = 0;
	write_nic_word(dev, RFPinsOutput, (twreg.longData|oval)); // Clear SI_EN (RFLE)
	udelay(10);

	mask = (low2high)?0x01:((unsigned int)0x01<<(totalLength-1));
	for(i=0; i<totalLength/2; i++)
	{
		twreg.struc.data = ((data2Write&mask)!=0) ? 1 : 0;
		write_nic_word(dev, RFPinsOutput, (twreg.longData|oval));
		twreg.struc.clk = 1;
		write_nic_word(dev, RFPinsOutput, (twreg.longData|oval));
		write_nic_word(dev, RFPinsOutput, (twreg.longData|oval));
		mask = (low2high)?(mask<<1):(mask>>1);
		twreg.struc.data = ((data2Write&mask)!=0) ? 1 : 0;
		write_nic_word(dev, RFPinsOutput, (twreg.longData|oval));
		write_nic_word(dev, RFPinsOutput, (twreg.longData|oval));
		twreg.struc.clk = 0;
		write_nic_word(dev, RFPinsOutput, (twreg.longData|oval));
		mask = (low2high)?(mask<<1):(mask>>1);
	}

	twreg.struc.enableB = 1;
	twreg.struc.clk = 0;
	twreg.struc.data = 0;
	write_nic_word(dev, RFPinsOutput, (twreg.longData|oval));
	udelay(10);

	write_nic_word(dev, RFPinsOutput, (oval|0x0004));
	write_nic_word(dev, RFPinsSelect, (oval3));// Set To SW Switch

	//Zebra_2 demoboard removed
	//PlatformEFIOWrite2Byte(Adapter, RFPinsEnable, 0x1fff);

//	Suggested by StevenLin, 2005.07.01. 
//	PlatformEFIOWrite4Byte(Adapter, PhyMuxPar, 0x3dc00002);
//	WriteBB(Adapter, 0x4095);
//	WriteBB(Adapter, 0x4097);
}

void RF_WriteReg(struct net_device *dev, u8 offset, u32 data)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	u32			data2Write;
	u8			len;
	u8			low2high;

	switch(priv->rf_chip)
	{
	case  RF_ZEBRA:
	case  RF_ZEBRA2:
// <RJ_DBG_8187B>
//#if defined(ZEBRA_WRITE_BY_8051) && PLATFORM==PLATFORM_WINDOWS_USB
//		if(	pHalData->VersionID >=  VERSION_8187_D )			// D-cut or latter.
//		{ // Perform 3-wire programming via FW, 8051.
//			// <RJ_NOTE> Since FW will NAK other IO request before its 3-wire programming complete,
//			// it can prevent race condition between more than one thread doing 3-wire programming. 2005.08.10, by rcnjko.
//			ZEBRA_RFSerialWriteBy8051(Adapter, offset, data);
//		}
//		else
//#endif // #if defined(ZEBRA_WRITE_BY_8051) && PLATFORM==PLATFORM_WINDOWS_USB
//
		{ // Perform 3-wire programming via SW.
			data2Write = (data << 4) | (u32)(offset&0x0f);
			len=16;
			low2high=FALSE;
			ZEBRA_RFSerialWrite(dev, data2Write, len, low2high);
		}
		break;

	default:
		break;
	}

}
#endif

//
//Added by Roger. 2006.12.20.
//
void MptDumpCck(
	struct net_device *dev
)
{
	u8 i;
	
	for (i=0x0; i<=0x1f; i++)
	{
		u8 readByte;
		write_nic_dword(dev, PhyAddr, 0x01000000);
		write_nic_byte(dev, PhyAddr, i);
		readByte = read_nic_byte(dev, PhyDataR);
		//RT_TRACE(COMP_MP, DBG_TRACE, ("CCK: 0x%x = 0x%x\n", i, readByte));
	}
	
	for (i=0x40; i<=0x49; i++)
	{
		u8 readByte;
		write_nic_dword(dev, PhyAddr, 0x01000000);
		write_nic_byte(dev, PhyAddr, i);
		readByte = read_nic_byte(dev, PhyDataR);
		//RT_TRACE(COMP_MP, DBG_TRACE, ("CCK: 0x%x = 0x%x\n", i, readByte));
	}

	for (i=0x50; i<=0x51; i++)
	{
		u8 readByte;
		write_nic_dword(dev, PhyAddr, 0x01000000);
		write_nic_byte(dev, PhyAddr, i);
		readByte = read_nic_byte(dev, PhyDataR);
		//RT_TRACE(COMP_MP, DBG_TRACE, ("CCK: 0x%x = 0x%x\n", i, readByte));
	}	
}

void MptStartCckContTx(struct net_device *dev, int bScrambleOn)
{
	u8		u1bReg;
	u8		CckTxAGC;
	u32		data;
	struct r8180_priv *priv = ieee80211_priv(dev);

	MptDumpCck(dev);// Added by Roger. 2006.12.20.

	CckTxAGC = read_nic_byte(dev, CCK_TXAGC);

	// 1. Set up TXOP to CCK mode(11b data rate) 0x034-0x037=0x00/0x01/0x02/0x03
	write_nic_dword(dev, PhyAddr, 0x010000b4);
	write_nic_dword(dev, PhyAddr, 0x010001b5);
	write_nic_dword(dev, PhyAddr, 0x010002b6);
	write_nic_dword(dev, PhyAddr, 0x010003b7);

	// 2. Set up CCK Continuous Tx rate.
	// CCK offset 0x12[5:4], 0=1M, 1=2M, 2=5.5M, 3=11M
	write_nic_dword(dev, PhyAddr, 0x01000012);
	u1bReg = read_nic_byte(dev, PhyDataR);

	u1bReg &= (~(BIT4|BIT5));
	switch(priv->ieee80211->rate)
	{
		case 10:// 1M
			u1bReg |= (0 << 4);
			break;

		case 20:// 2M
			u1bReg |= (1 << 4);
			break;

		case 55:// 5.5M
			u1bReg |= (2 << 4);
			break;

		case 110:// 11M
			u1bReg |= (3 << 4);
			break;
	}
	data = 0x01000092 | (((u32)u1bReg) << 8);
	write_nic_dword(dev, PhyAddr, data);

	// 3. Enable continuous TX
	// CCK offset 0x00=0x0a (default=0xc8)
	// Asked by Penuang: we shall set CCK.0 after CCK.12, otherwise, 
	// the data rate of CCK cont. Tx might be defualt value or that of last CCK cont. Tx.
	if(bScrambleOn)
	{
		write_nic_dword(dev, PhyAddr, 0x01000a80);  // BB set continuous tx with scramble (BIT3).
	}
	else
	{
		write_nic_dword(dev, PhyAddr, 0x01000280);  // BB set continuous tx with scramble (BIT3).
	}

	// 4. Disable Hardware HSSI
	// MAC offset 0x85=0x04 (default=0x24)
	u1bReg = read_nic_byte(dev, (RFPinsSelect+1));
	write_nic_byte(dev, (RFPinsSelect+1), (u1bReg & (~BIT5)));

	// 5. Set RF to TX mode
	switch(priv->rf_chip)
	{
	case RF_ZEBRA:
	case RF_ZEBRA2:
		// RF offset 0x04=0x407 (default=0x94d, RX mode)
		//
		// 2006.06.13, SD3 C.M.Lin:
		// We must write OFDM Tx AGC into RF manually for 87B OFDM cont. tx.
		//
		//RF_WriteReg(dev, 0x04, (0x400 | CckTxAGC));
		write_rtl8225(dev, 0x04, (0x400 | CckTxAGC));
		break;

	default:
		break;
	}

	// 6. Set SW decided T/R switch to TX path.
	u1bReg = read_nic_byte(dev, (RFIO_CTRL+2));
	u1bReg &=  0xf0;
	u1bReg |= (0xe);
	write_nic_byte(dev, (RFIO_CTRL+2), u1bReg);

	// 7. Turn on PA
	// MAC offset 0x235[7:4]=f (default=0, hardware decided)
	u1bReg = read_nic_byte(dev, (RFIO_CTRL+1));
	u1bReg &= 0x0f;
	u1bReg |= 0xf0;
	write_nic_byte(dev, (RFIO_CTRL+1), u1bReg);

	priv->bCckContTx = TRUE;
	priv->bOfdmContTx= FALSE;

}

void MptStopCckContTx(struct net_device *dev)
{
	u8	u1bReg;
	struct r8180_priv *priv = ieee80211_priv(dev);

	priv->bCckContTx = FALSE;
	priv->bOfdmContTx= FALSE;

	switch(priv->rf_chip)
	{
	case RF_ZEBRA:
	case RF_ZEBRA2:
		{
		// I do this before restore other settings. Added by Roger. 2006.12.20.
		// 5. Disable CCK continuous TX.
		// CCK offset 0x00=0xC8.
		write_nic_dword(dev, PhyAddr, 0x0100C880);

		// 1. Turn off PA.
		// MAC offset 0x235[7:4]=0.
		u1bReg = read_nic_byte(dev, (RFIO_CTRL+1));
		u1bReg &= 0x0f;
		write_nic_byte(dev, (RFIO_CTRL+1), u1bReg);

		// 2. Set SW decided T/R switch to HW controled.
		u1bReg = read_nic_byte(dev, (RFIO_CTRL+2));
		u1bReg &=  0xf0;
		write_nic_byte(dev, (RFIO_CTRL+2), u1bReg);

		// 3. Restore RF to Rx mode.
		//RF_WriteReg(dev, 0x04, 0x94d);
		write_rtl8225(dev, 0x04, 0x94d);

		// 4. Enable Hardware HSS. MAC offset 0x85=0x24.
		u1bReg = read_nic_byte(dev, (RFPinsSelect+1));
		write_nic_byte(dev, (RFPinsSelect+1), (u1bReg | BIT5));
		}
		break;

	default:
		break;
	}

}

void MptStartOfdmContTx(struct net_device *dev)
{
	u8		u1bReg,OfdmTxAGC;
	u32		data;
	struct r8180_priv *priv = ieee80211_priv(dev);

	OfdmTxAGC = read_nic_byte(dev, OFDM_TXAGC);

	// 1. Enable OFDM continuous TX offset=0x03[7] 1:enable, 0:disable
	write_nic_dword(dev, PhyAddr, 0x00000003);
	u1bReg = read_nic_byte(dev, PhyDataR);
	u1bReg |= BIT7;
	data = 0x00000083 | (((unsigned int)u1bReg) << 8); 
	write_nic_dword(dev, PhyAddr, data);

	// 2. Disable Hardware HSSI.
	// MAC offset 0x85=0x04 (default=0x24)
	u1bReg = read_nic_byte(dev, (RFPinsSelect+1));
	write_nic_byte(dev, (RFPinsSelect+1), (u1bReg & (~BIT5)));

	// 3. Set RF to TX mode and OFDM Tx power.
	// RF offset 0x04=0x407 (default=0x94d, RX mode).
	switch(priv->rf_chip)
	{
	case RF_ZEBRA:
	case RF_ZEBRA2:
		// RF offset 0x04=0x407 (default=0x94d, RX mode)
		//
		// 2006.06.13, SD3 C.M.Lin:
		// We must write OFDM Tx AGC into RF manually for 87B OFDM cont. tx.
		//
		//RF_WriteReg(dev, 0x04, (0x400 | OfdmTxAGC));
		write_rtl8225(dev, 0x04, (0x400 | OfdmTxAGC));
		break;

	default:
		break;
	}

	// 4. Set SW decided T/R switch to TX path.
	u1bReg = read_nic_byte(dev, (RFIO_CTRL+2));
	u1bReg &=  0xf0;
	u1bReg |= (0xe);
	write_nic_byte(dev, (RFIO_CTRL+2), u1bReg);

	// 5. Turn on PA.
	// MAC offset 0x235[7:4]=f (default=0, hardware decided)
	u1bReg = read_nic_byte(dev, (RFIO_CTRL+1));
	u1bReg &= 0x0f;
	u1bReg |= 0xf0;
	write_nic_byte(dev, (RFIO_CTRL+1), u1bReg);

	priv->bCckContTx = FALSE;
	priv->bOfdmContTx= TRUE;

}

void MptStopOfdmContTx(struct net_device *dev)
{
	u8		u1bReg;
	u32		data;
	struct r8180_priv *priv = ieee80211_priv(dev);

	priv->bCckContTx = FALSE;
	priv->bOfdmContTx= FALSE;

	// 5. Disable continuous TX.
	// Disable OFDM continuous TX offset=0x03[7] 1:enable, 0:disable.
	// <RJ_NOTE> I disable cont. Tx state before restore other setting to prevent
	// these setting won't take effect in cont. Tx state. Besides, my experiment
	// shows if we disable cont. tx at last step, we might encounter Tx stuck for 
	// several secondes. 2006.06.13, by rcnjko.
	write_nic_dword(dev, PhyAddr, 0x00000003);
	u1bReg = read_nic_byte(dev, PhyDataR);
	u1bReg &= (~BIT7);
	data = 0x00000083 | (((unsigned int)u1bReg) << 8);
	write_nic_dword(dev, PhyAddr, data);

	// 1. Turn off PA .
	// MAC offset 0x235[7:4]=0.
	u1bReg = read_nic_byte(dev, (RFIO_CTRL+1));
	u1bReg &= 0x0f;
	write_nic_byte(dev, (RFIO_CTRL+1), u1bReg);

	// 2. Set SW decided T/R switch to HW controled.
	// MAC offset 0x236[3:0]=0.
	u1bReg = read_nic_byte(dev, (RFIO_CTRL+2));
	u1bReg &=  0xf0;
	write_nic_byte(dev, (RFIO_CTRL+2), u1bReg);

	// 3. Restore RF to Rx mode.
	//RF_WriteReg(dev, 0x04, 0x94d);
	write_rtl8225(dev, 0x04, 0x94d);

	// 4. Enable Hardware HSSI.
	// MAC offset 0x85=0x24.
	u1bReg = read_nic_byte(dev, (RFPinsSelect+1));
	write_nic_byte(dev, (RFPinsSelect+1), (u1bReg | BIT5));

}

int mp_ioctl_set_continuous_tx_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct continuous_tx_param *pcontinuoustx = (struct continuous_tx_param *)param;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(pcontinuoustx==NULL)
		return -1;

	if(set)
	{
		if(pcontinuoustx->mode)
		{// Start Continuous Tx.
			switch(priv->ieee80211->mode)
			{
				case WIRELESS_MODE_A:
				case WIRELESS_MODE_G:
					{ // Start OFDM Continuous Tx.
						MptStartOfdmContTx(dev);
					}
					break;

				case WIRELESS_MODE_B:
					{ // Start CCK Continuous Tx with scramble on.
						MptStartCckContTx(dev, TRUE);
					}
					break;

				default:
					priv->bCckContTx = FALSE;
					priv->bOfdmContTx = FALSE;
					break;
			}
		}
		else
		{// Stop Continuous Tx.
			if(priv->bCckContTx == TRUE && priv->bOfdmContTx == FALSE)
			{ // Stop CCK Continuous Tx.
				MptStopCckContTx(dev);
			}
			else if(priv->bCckContTx == FALSE && priv->bOfdmContTx == TRUE)
			{ // Stop OFDM Continuous Tx.
				MptStopOfdmContTx(dev);
			}
			else if(priv->bCckContTx == FALSE && priv->bOfdmContTx == FALSE)
			{ // We've already stopped Continuous Tx.
			}
		}
	}
	else
	{
		return -1;
	}

	return 0;
}

int mp_ioctl_set_single_carrier_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct continuous_tx_param *pcontinuoustx = (struct continuous_tx_param *)param;
	struct r8180_priv *priv = ieee80211_priv(dev);
	unsigned char currentchannel = priv->ieee80211->current_network.channel;

	if(pcontinuoustx==NULL)
		return -1;

	if(set)
	{
		if(pcontinuoustx->mode)
		{// Start Single Carrier.
			if(priv->ieee80211->mode == WIRELESS_MODE_B)
			{// Start Single Carrier for B mode.
				switch(priv->rf_chip)
				{
				// Start Single Carrier for ZEBRA in B mode.
				case RF_ZEBRA:
				case RF_ZEBRA2:
					write_rtl8225(dev, 0x9, 0x134); mdelay(1);
					//RF_WriteReg(dev, 0x9, 0x134); mdelay(1);

					// Set Tx power level to max. 2004.11.17, by rcnjko.
					priv->btMpCckTxPower = priv->chtxpwr[currentchannel];
					priv->btMpOfdmTxPower= priv->chtxpwr_ofdm[currentchannel];
					priv->chtxpwr[currentchannel] = 11;
					priv->chtxpwr_ofdm[currentchannel] = 12;

					write_nic_dword(dev, PhyAddr, 0x010000c4);
					write_nic_dword(dev, PhyAddr, 0x010000c5);
					write_nic_dword(dev, PhyAddr, 0x010000c6);
					write_nic_dword(dev, PhyAddr, 0x010000c7);
					write_nic_dword(dev, PhyAddr, 0x010000c8);
					write_nic_dword(dev, PhyAddr, 0x010000c9);
					write_nic_dword(dev, PhyAddr, 0x010000ca);
					write_nic_dword(dev, PhyAddr, 0x010000cb);
					write_nic_dword(dev, PhyAddr, 0x010007cc);
					break;
				default:
					break;
				}
			}
		}
		else
		{// Stop Single Carrier.
			if(priv->ieee80211->mode == WIRELESS_MODE_B)
			{// Stop Single Carrier for B mode.

				write_nic_dword(dev, PhyAddr, 0x0100C880);
				write_nic_dword(dev, PhyAddr, 0x0100D093);
				write_nic_dword(dev, PhyAddr, 0x010088c1);

				write_nic_dword(dev, PhyAddr, 0x010026c4);
				write_nic_dword(dev, PhyAddr, 0x010025c5);
				write_nic_dword(dev, PhyAddr, 0x010021c6);
				write_nic_dword(dev, PhyAddr, 0x01001bc7);
				write_nic_dword(dev, PhyAddr, 0x010014c8);
				write_nic_dword(dev, PhyAddr, 0x01000dc9);
				write_nic_dword(dev, PhyAddr, 0x010006ca);
				write_nic_dword(dev, PhyAddr, 0x010003cb);
				write_nic_dword(dev, PhyAddr, 0x010000cc);

				// Restore original power level setting, 2004.11.17, by rcnjko.
				priv->chtxpwr[currentchannel] = priv->btMpCckTxPower;
				priv->chtxpwr_ofdm[currentchannel] = priv->btMpOfdmTxPower;
				priv->rf_set_chan(dev, currentchannel);

				switch(priv->rf_chip)
				{// Stop Single Carrier for Zebra in B mode.
				case RF_ZEBRA:
					write_rtl8225(dev, 0x9, 0x334); mdelay(1);
					write_rtl8225(dev, 0x4, 0x85e); mdelay(1);
					//RF_WriteReg(dev, 0x9, 0x334); mdelay(1);
					//RF_WriteReg(dev, 0x4, 0x85e); mdelay(1);
					break;

				case RF_ZEBRA2:
					write_rtl8225(dev, 0x9, 0x334); mdelay(1);
					write_rtl8225(dev, 0x4, 0x9cd); mdelay(1);
					//RF_WriteReg(dev, 0x9, 0x334); mdelay(1);
					//RF_WriteReg(dev, 0x4, 0x9cd); mdelay(1);
					break;

				default:
					break;
				}
			}
		}
	}
	else
	{
		return -1;
	}

	return 0;
}

int mp_ioctl_get_packets_rx_hdl(struct net_device *dev, unsigned char set, void*param)
{
	int ret = 0;
	struct packets_rx_param *ppacketsrx = (struct packets_rx_param *)param;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(ppacketsrx == NULL)
		return -1;

	if(set)
	{
		ret = -1;
	}
	else
	{
		ppacketsrx->rxok = priv->stats.rxok;
		ppacketsrx->rxerr = priv->stats.rxstaterr+priv->stats.rxurberr;
	}

	return ret;
}

#endif


static int r8180_wx_set_rawtx(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);
	
	ret = ieee80211_wx_set_rawtx(priv->ieee80211, info, wrqu, extra);
	
	up(&priv->wx_sem);
	
	return ret;
	 
}

static int r8180_wx_set_crcmon(struct net_device *dev, 
			       struct iw_request_info *info, 
			       union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
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
		rtl8180_down(dev);
		rtl8180_up(dev);
	}
	
	up(&priv->wx_sem);
	
	return 0;
}

static int r8180_wx_set_mode(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);
	
	ret = ieee80211_wx_set_mode(priv->ieee80211,a,wrqu,b);
	
	rtl8187_set_rxconf(dev);
	
	up(&priv->wx_sem);
	return ret;
}


static int rtl8180_wx_get_range(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct iw_range *range = (struct iw_range *)extra;
	struct r8180_priv *priv = ieee80211_priv(dev);
	u16 val;
	int i;

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
	if(priv->rf_set_sens != NULL)
		range->sensitivity = priv->max_sens;	/* signal level threshold range */
	
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
		if ((priv->challow)[i+1]) {
		        range->freq[val].i = i + 1;
			range->freq[val].m = ieee80211_wlan_frequencies[i] * 100000;
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
	return 0;
}


static int r8180_wx_set_scan(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	int ret;
	
	if(!priv->up) return -1;
	
	down(&priv->wx_sem);
	
	ret = ieee80211_wx_set_scan(priv->ieee80211,a,wrqu,b);
	
	up(&priv->wx_sem);
	return ret;
}


static int r8180_wx_get_scan(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{

	int ret;
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	if(!priv->up) return -1;
			
	down(&priv->wx_sem);

	ret = ieee80211_wx_get_scan(priv->ieee80211,a,wrqu,b);
		
	up(&priv->wx_sem);
	
	return ret;
}

static int r8180_wx_set_essid(struct net_device *dev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *b)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
//	struct ieee80211_device *ieee = priv->ieee80211;	
//	int len;
	int ret;
		
	down(&priv->wx_sem);
	
	ret = ieee80211_wx_set_essid(priv->ieee80211,a,wrqu,b);

	up(&priv->wx_sem);

	return ret;
}




static int r8180_wx_get_essid(struct net_device *dev, 
			      struct iw_request_info *a,
			      union iwreq_data *wrqu, char *b)
{
	int ret;
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	down(&priv->wx_sem);
	
	ret = ieee80211_wx_get_essid(priv->ieee80211, a, wrqu, b);

	up(&priv->wx_sem);
	
	return ret;
}


static int r8180_wx_set_freq(struct net_device *dev, struct iw_request_info *a,
			     union iwreq_data *wrqu, char *b)
{
	int ret;
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	down(&priv->wx_sem);
	
	ret = ieee80211_wx_set_freq(priv->ieee80211, a, wrqu, b);
	
	up(&priv->wx_sem);
	return ret;
}

static int r8180_wx_get_name(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	return ieee80211_wx_get_name(priv->ieee80211, info, wrqu, extra);
}


static int r8180_wx_set_frag(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	if (wrqu->frag.disabled)
		priv->ieee80211->fts = DEFAULT_FRAG_THRESHOLD;
	else {
		if (wrqu->frag.value < MIN_FRAG_THRESHOLD ||
		    wrqu->frag.value > MAX_FRAG_THRESHOLD)
			return -EINVAL;
		
		priv->ieee80211->fts = wrqu->frag.value & ~0x1;
	}

	return 0;
}


static int r8180_wx_get_frag(struct net_device *dev, 
			     struct iw_request_info *info, 
			     union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	wrqu->frag.value = priv->ieee80211->fts;
	wrqu->frag.fixed = 0;	/* no auto select */
	wrqu->frag.disabled = (wrqu->frag.value == DEFAULT_FRAG_THRESHOLD);

	return 0;
}


static int r8180_wx_set_wap(struct net_device *dev,
			 struct iw_request_info *info,
			 union iwreq_data *awrq,
			 char *extra)
{

	int ret;
	struct r8180_priv *priv = ieee80211_priv(dev);
//        struct ieee80211_device *ieee = priv->ieee80211;
//        struct sockaddr *temp = (struct sockaddr *)awrq;
//        u32 ori;
//        u32 TargetContent;
	
	down(&priv->wx_sem);
	
	ret = ieee80211_wx_set_wap(priv->ieee80211,info,awrq,extra);

	up(&priv->wx_sem);

	return ret;
	
}
	

static int r8180_wx_get_wap(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	return ieee80211_wx_get_wap(priv->ieee80211,info,wrqu,extra);
}


static int r8180_wx_get_enc(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *key)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	return ieee80211_wx_get_encode(priv->ieee80211, info, wrqu, key);
}

static int r8180_wx_set_enc(struct net_device *dev, 
			    struct iw_request_info *info, 
			    union iwreq_data *wrqu, char *key)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	int ret;
#ifdef 	JOHN_HWSEC
//	u32 TargetContent;
//	u32 clearkey[4] = {0,0,0,0};
	u32 hwkey[4]={0,0,0,0};
	u8 mask=0xff;
	u32 key_idx=0;
	u8 broadcast_addr[6] ={	0xff,0xff,0xff,0xff,0xff,0xff}; 
	u8 zero_addr[4][6] ={	{0x00,0x00,0x00,0x00,0x00,0x00},
				{0x00,0x00,0x00,0x00,0x00,0x01}, 
				{0x00,0x00,0x00,0x00,0x00,0x02}, 
				{0x00,0x00,0x00,0x00,0x00,0x03} };
	int i;

#endif

	down(&priv->wx_sem);
	
	DMESG("Setting SW wep key");
	ret = ieee80211_wx_set_encode(priv->ieee80211,info,wrqu,key);

	up(&priv->wx_sem);

#ifdef	JOHN_HWSEC

	//sometimes, the length is zero while we do not type key value
	if(wrqu->encoding.length!=0){

		for(i=0 ; i<4 ; i++){
			hwkey[i] |=  key[4*i+0]&mask;
		//	if(i==1&&(4*i+1)==wrqu->encoding.length) mask=0x00;
		//	if(i==3&&(4*i+1)==wrqu->encoding.length) mask=0x00;
			hwkey[i] |= (key[4*i+1]&mask)<<8;
			hwkey[i] |= (key[4*i+2]&mask)<<16;
			hwkey[i] |= (key[4*i+3]&mask)<<24;
		}

		if(wrqu->encoding.flags!=0)
			key_idx = wrqu->encoding.flags-1;
		
		write_nic_byte(dev, WPA_CONFIG, 7);

		if(wrqu->encoding.length == 0x5) //WEP40
		{
			ieee->broadcast_key_type	= KEY_TYPE_WEP40;
			ieee->pairwise_key_type	= KEY_TYPE_WEP40;
				
			setKey( dev,
				key_idx,                //EntryNo
				key_idx,                //KeyIndex 
				KEY_TYPE_WEP40,         //KeyType
				zero_addr[key_idx],
				0,                      //DefaultKey
				hwkey);                 //KeyContent

			if(key_idx == 0)
			{
				setKey( dev,
					4,                      //EntryNo
					key_idx,                      //KeyIndex 
					KEY_TYPE_WEP40,        //KeyType
					broadcast_addr,         //addr
					0,                      //DefaultKey
					hwkey);                 //KeyContent
			}
		}

		else if(wrqu->encoding.length == 0xd)//WEP104
		{
			ieee->broadcast_key_type	= KEY_TYPE_WEP104;
			ieee->pairwise_key_type	= KEY_TYPE_WEP104;
		
			setKey( dev,
				key_idx,                //EntryNo
				key_idx,                //KeyIndex 
				KEY_TYPE_WEP104,        //KeyType
				zero_addr[key_idx],
				0,                      //DefaultKey
				hwkey);                 //KeyContent
 
			if(key_idx == 0){

		
				setKey( dev,
					4,                      //EntryNo
					key_idx,                      //KeyIndex 
					KEY_TYPE_WEP104,        //KeyType
					broadcast_addr,         //addr
					0,                      //DefaultKey
					hwkey);                 //KeyContent
			}
		}
		else 
		{
                        setKey( dev,
                                key_idx,                //EntryNo
                                key_idx,                //KeyIndex 
                                KEY_TYPE_WEP104,        //KeyType
                                zero_addr[key_idx],
                                1,                      //DefaultKey
                                hwkey);                 //KeyContent

			printk("wrong type in WEP, not WEP40 and WEP104; clear the keys in CAM\n");
		}

	}

	//consider the setting different key index situation
	//wrqu->encoding.flags = 801 means that we set key with index "1"
	if(wrqu->encoding.length==0 && (wrqu->encoding.flags >>8) == 0x8 ){
		
		write_nic_byte(dev, WPA_CONFIG, 7);

		//copy wpa config from default key(key0~key3) to broadcast key(key5)
		//
		key_idx = (wrqu->encoding.flags & 0xf)-1 ;
		write_cam(dev, (4*6),   0xffff0000|read_cam(dev, key_idx*6) );
		write_cam(dev, (4*6)+1, 0xffffffff);
		write_cam(dev, (4*6)+2, read_cam(dev, (key_idx*6)+2) );
		write_cam(dev, (4*6)+3, read_cam(dev, (key_idx*6)+3) );
		write_cam(dev, (4*6)+4, read_cam(dev, (key_idx*6)+4) );
		write_cam(dev, (4*6)+5, read_cam(dev, (key_idx*6)+5) );
	}

#endif /*JOHN_HWSEC*/
	return ret;
}


static int r8180_wx_set_scan_type(struct net_device *dev, struct iw_request_info *aa, union
 iwreq_data *wrqu, char *p){
  
 	struct r8180_priv *priv = ieee80211_priv(dev);
	int *parms=(int*)p;
	int mode=parms[0];
	
	priv->ieee80211->active_scan = mode;
	
	return 1;
}



static int r8180_wx_set_retry(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
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
	
	/* FIXME ! 
	 * We might try to write directly the TX config register
	 * or to restart just the (R)TX process.
	 * I'm unsure if whole reset is really needed
	 */

 	rtl8180_commit(dev);
	/*
	if(priv->up){
		rtl8180_rtx_disable(dev);
		rtl8180_rx_enable(dev);
		rtl8180_tx_enable(dev);
			
	}
	*/
exit:
	up(&priv->wx_sem);
	
	return err;
}

static int r8180_wx_get_retry(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	

	wrqu->retry.disabled = 0; /* can't be disabled */

	if ((wrqu->retry.flags & IW_RETRY_TYPE) == 
	    IW_RETRY_LIFETIME) 
		return -EINVAL;
	
	if (wrqu->retry.flags & IW_RETRY_MAX) {
		wrqu->retry.flags = IW_RETRY_LIMIT & IW_RETRY_MAX;
		wrqu->retry.value = priv->retry_rts;
	} else {
		wrqu->retry.flags = IW_RETRY_LIMIT & IW_RETRY_MIN;
		wrqu->retry.value = priv->retry_data;
	}
	//DMESG("returning %d",wrqu->retry.value);
	

	return 0;
}

static int r8180_wx_get_sens(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	if(priv->rf_set_sens == NULL) 
		return -1; /* we have not this support for this radio */
	wrqu->sens.value = priv->sens;
	return 0;
}


static int r8180_wx_set_sens(struct net_device *dev, 
				struct iw_request_info *info, 
				union iwreq_data *wrqu, char *extra)
{
	
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	short err = 0;
	down(&priv->wx_sem);
	//DMESG("attempt to set sensivity to %ddb",wrqu->sens.value);
	if(priv->rf_set_sens == NULL) {
		err= -1; /* we have not this support for this radio */
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


static int dummy(struct net_device *dev, struct iw_request_info *a,
		 union iwreq_data *wrqu,char *b)
{
	return -1;
}


static iw_handler r8180_wx_handlers[] =
{
        NULL,                     /* SIOCSIWCOMMIT */
        r8180_wx_get_name,   	  /* SIOCGIWNAME */
        dummy,                    /* SIOCSIWNWID */
        dummy,                    /* SIOCGIWNWID */
        r8180_wx_set_freq,        /* SIOCSIWFREQ */
        r8180_wx_get_freq,        /* SIOCGIWFREQ */
        r8180_wx_set_mode,        /* SIOCSIWMODE */
        r8180_wx_get_mode,        /* SIOCGIWMODE */
        r8180_wx_set_sens,        /* SIOCSIWSENS */
        r8180_wx_get_sens,        /* SIOCGIWSENS */
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
        r8180_wx_set_wap,      	  /* SIOCSIWAP */
        r8180_wx_get_wap,         /* SIOCGIWAP */
        NULL,                     /* -- hole -- */
        dummy,                    /* SIOCGIWAPLIST -- depricated */
        r8180_wx_set_scan,        /* SIOCSIWSCAN */
        r8180_wx_get_scan,        /* SIOCGIWSCAN */
        r8180_wx_set_essid,       /* SIOCSIWESSID */
        r8180_wx_get_essid,       /* SIOCGIWESSID */
        dummy,                    /* SIOCSIWNICKN */
        dummy,                    /* SIOCGIWNICKN */
        NULL,                     /* -- hole -- */
        NULL,                     /* -- hole -- */
        r8180_wx_set_rate,        /* SIOCSIWRATE */
        r8180_wx_get_rate,        /* SIOCGIWRATE */
        dummy,                    /* SIOCSIWRTS */
        dummy,                    /* SIOCGIWRTS */
        r8180_wx_set_frag,        /* SIOCSIWFRAG */
        r8180_wx_get_frag,        /* SIOCGIWFRAG */
        dummy,                    /* SIOCSIWTXPOW */
        dummy,                    /* SIOCGIWTXPOW */
        r8180_wx_set_retry,       /* SIOCSIWRETRY */
        r8180_wx_get_retry,       /* SIOCGIWRETRY */
        r8180_wx_set_enc,         /* SIOCSIWENCODE */
        r8180_wx_get_enc,         /* SIOCGIWENCODE */
        dummy,                    /* SIOCSIWPOWER */
        dummy,                    /* SIOCGIWPOWER */
}; 


static const struct iw_priv_args r8180_private_args[] = { 
	
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
#ifdef JOHN_IOCTL	
	,
	{
		SIOCIWFIRSTPRIV + 0x3,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "readRF"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x4,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "writeRF"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x5,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "readBB"
	}
	,
	{
		SIOCIWFIRSTPRIV + 0x6,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "writeBB"
	}
        ,
        {
                SIOCIWFIRSTPRIV + 0x7,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "readnicb"
        }
        ,
        {
                SIOCIWFIRSTPRIV + 0x8,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "writenicb"
        }
        ,
        {
                SIOCIWFIRSTPRIV + 0x9,
                IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "apinfo"
        }
#endif
#ifdef THOMAS_MP_IOCTL
	,
	{
		SIOCIWFIRSTPRIV + 0xa, 0, 0, "mp_ioctl"
	}
#endif
};


static iw_handler r8180_private_handler[] = {
//	r8180_wx_set_monitor,  /* SIOCIWFIRSTPRIV */
	r8180_wx_set_crcmon,   /*SIOCIWSECONDPRIV*/
//	r8180_wx_set_forceassociate,
//	r8180_wx_set_beaconinterval,
//	r8180_wx_set_monitor_type,
	r8180_wx_set_scan_type,
	r8180_wx_set_rawtx,
#ifdef JOHN_IOCTL
	r8180_wx_read_regs,
	r8180_wx_write_regs,
	r8180_wx_read_bb,
	r8180_wx_write_bb,
        r8180_wx_read_nicb,
        r8180_wx_write_nicb,
	r8180_wx_get_ap_status,
#endif
#ifdef THOMAS_MP_IOCTL
	r8180_wx_mp_ioctl_hdl,
#endif
};

#if WIRELESS_EXT >= 17	
static struct iw_statistics *r8180_get_wireless_stats(struct net_device *dev)
{
       struct r8180_priv *priv = ieee80211_priv(dev);

       return &priv->wstats;
}
#endif


struct iw_handler_def  r8180_wx_handlers_def={
	.standard = r8180_wx_handlers,
	.num_standard = sizeof(r8180_wx_handlers) / sizeof(iw_handler),
	.private = r8180_private_handler,
	.num_private = sizeof(r8180_private_handler) / sizeof(iw_handler),
 	.num_private_args = sizeof(r8180_private_args) / sizeof(struct iw_priv_args),
#if WIRELESS_EXT >= 17	
	.get_wireless_stats = r8180_get_wireless_stats,
#endif
	.private_args = (struct iw_priv_args *)r8180_private_args,	
};
