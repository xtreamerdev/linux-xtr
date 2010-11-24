/******************************************************************************

  Copyright(c) 2003 - 2004 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.

  Contact Information:
  James P. Ketrenos <ipw2100-admin@linux.intel.com>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

******************************************************************************

  Few modifications for Realtek's Wi-Fi drivers by 
  Andrea Merello <andreamrl@tiscali.it>
  
  A special thanks goes to Realtek for their support ! 

******************************************************************************/

#include <linux/compiler.h>
//#include <linux/config.h>
#include <linux/errno.h>
#include <linux/if_arp.h>
#include <linux/in6.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include <linux/proc_fs.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/tcp.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/wireless.h>
#include <linux/etherdevice.h>
#include <asm/uaccess.h>
#include <linux/if_vlan.h>

#include "rtllib.h"

#ifdef RTK_DMP_PLATFORM
#include <linux/usb_setting.h> // for USB_USE_ALIGNMENT
#endif

/*


802.11 Data Frame


802.11 frame_contorl for data frames - 2 bytes
     ,-----------------------------------------------------------------------------------------.
bits | 0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  a  |  b  |  c  |  d  |  e   |
     |----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|------|
val  | 0  |  0  |  0  |  1  |  x  |  0  |  0  |  0  |  1  |  0  |  x  |  x  |  x  |  x  |  x   |
     |----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|------|
desc | ^-ver-^  |  ^type-^  |  ^-----subtype-----^  | to  |from |more |retry| pwr |more |wep   |
     |          |           | x=0 data,x=1 data+ack | DS  | DS  |frag |     | mgm |data |      |
     '-----------------------------------------------------------------------------------------'
		                                    /\
                                                    |
802.11 Data Frame                                   |
           ,--------- 'ctrl' expands to >-----------'
          |
      ,--'---,-------------------------------------------------------------.
Bytes |  2   |  2   |    6    |    6    |    6    |  2   | 0..2312 |   4  |
      |------|------|---------|---------|---------|------|---------|------|
Desc. | ctrl | dura |  DA/RA  |   TA    |    SA   | Sequ |  Frame  |  fcs |
      |      | tion | (BSSID) |         |         | ence |  data   |      |
      `--------------------------------------------------|         |------'
Total: 28 non-data bytes                                 `----.----'
                                                              |
       .- 'Frame data' expands to <---------------------------'
       |
       V
      ,---------------------------------------------------.
Bytes |  1   |  1   |    1    |    3     |  2   |  0-2304 |
      |------|------|---------|----------|------|---------|
Desc. | SNAP | SNAP | Control |Eth Tunnel| Type | IP      |
      | DSAP | SSAP |         |          |      | Packet  |
      | 0xAA | 0xAA |0x03 (UI)|0x00-00-F8|      |         |
      `-----------------------------------------|         |
Total: 8 non-data bytes                         `----.----'
                                                     |
       .- 'IP Packet' expands, if WEP enabled, to <--'
       |
       V
      ,-----------------------.
Bytes |  4  |   0-2296  |  4  |
      |-----|-----------|-----|
Desc. | IV  | Encrypted | ICV |
      |     | IP Packet |     |
      `-----------------------'
Total: 8 non-data bytes


802.3 Ethernet Data Frame

      ,-----------------------------------------.
Bytes |   6   |   6   |  2   |  Variable |   4  |
      |-------|-------|------|-----------|------|
Desc. | Dest. | Source| Type | IP Packet |  fcs |
      |  MAC  |  MAC  |      |           |      |
      `-----------------------------------------'
Total: 18 non-data bytes

In the event that fragmentation is required, the incoming payload is split into
N parts of size ieee->fts.  The first fragment contains the SNAP header and the
remaining packets are just data.

If encryption is enabled, each fragment payload size is reduced by enough space
to add the prefix and postfix (IV and ICV totalling 8 bytes in the case of WEP)
So if you have 1500 bytes of payload with ieee->fts set to 500 without
encryption it will take 3 frames.  With WEP it will take 4 frames as the
payload of each frame is reduced to 492 bytes.

* SKB visualization
*
*  ,- skb->data
* |
* |    ETHERNET HEADER        ,-<-- PAYLOAD
* |                           |     14 bytes from skb->data
* |  2 bytes for Type --> ,T. |     (sizeof ethhdr)
* |                       | | |
* |,-Dest.--. ,--Src.---. | | |
* |  6 bytes| | 6 bytes | | | |
* v         | |         | | | |
* 0         | v       1 | v | v           2
* 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
*     ^     | ^         | ^ |
*     |     | |         | | |
*     |     | |         | `T' <---- 2 bytes for Type
*     |     | |         |
*     |     | '---SNAP--' <-------- 6 bytes for SNAP
*     |     |
*     `-IV--' <-------------------- 4 bytes for IV (WEP)
*
*      SNAP HEADER
*
*/

static u8 P802_1H_OUI[P80211_OUI_LEN] = { 0x00, 0x00, 0xf8 };
static u8 RFC1042_OUI[P80211_OUI_LEN] = { 0x00, 0x00, 0x00 };

inline int rtllib_put_snap(u8 *data, u16 h_proto)
{
	struct rtllib_snap_hdr *snap;
	u8 *oui;

	snap = (struct rtllib_snap_hdr *)data;
	snap->dsap = 0xaa;
	snap->ssap = 0xaa;
	snap->ctrl = 0x03;

	if (h_proto == 0x8137 || h_proto == 0x80f3)
		oui = P802_1H_OUI;
	else
		oui = RFC1042_OUI;
	snap->oui[0] = oui[0];
	snap->oui[1] = oui[1];
	snap->oui[2] = oui[2];

	*(u16 *)(data + SNAP_SIZE) = htons(h_proto);

	return SNAP_SIZE + sizeof(u16);
}

#ifdef _RTL8192_EXT_PATCH_
int rtllib_encrypt_fragment(
	struct rtllib_device *ieee,
	struct sk_buff *frag,
	int hdr_len,
	u8 is_mesh)
#else
int rtllib_encrypt_fragment(
	struct rtllib_device *ieee,
	struct sk_buff *frag,
	int hdr_len)
#endif
{
	struct rtllib_crypt_data* crypt = NULL;//ieee->crypt[ieee->tx_keyidx];
	int res;
	
#ifdef _RTL8192_EXT_PATCH_
	if(is_mesh)
		crypt = ieee->cryptlist[0]->crypt[ieee->mesh_txkeyidx]; //AMY modify 090226
	else
		crypt = ieee->sta_crypt[ieee->tx_keyidx]; 		
#else
	crypt = ieee->crypt[ieee->tx_keyidx];
#endif

	if (!(crypt && crypt->ops))
	{
		printk("=========>%s(), crypt is null\n", __FUNCTION__);
		return -1;
	}
#ifdef CONFIG_RTLLIB_CRYPT_TKIP
	struct rtllib_hdr_1addr *header;

	if (ieee->tkip_countermeasures &&
	    crypt && crypt->ops && strcmp(crypt->ops->name, "TKIP") == 0) {
		header = (struct rtllib_hdr_1addr *) frag->data;
		if (net_ratelimit()) {
			printk(KERN_DEBUG "%s: TKIP countermeasures: dropped "
			       "TX packet to " MAC_FMT "\n",
			       ieee->dev->name, MAC_ARG(header->addr1));
		}
		return -1;
	}
#endif
	/* To encrypt, frame format is:
	 * IV (4 bytes), clear payload (including SNAP), ICV (4 bytes) */

	// PR: FIXME: Copied from hostap. Check fragmentation/MSDU/MPDU encryption.
	/* Host-based IEEE 802.11 fragmentation for TX is not yet supported, so
	 * call both MSDU and MPDU encryption functions from here. */
	atomic_inc(&crypt->refcnt);
	res = 0;
	if (crypt->ops->encrypt_msdu)
		res = crypt->ops->encrypt_msdu(frag, hdr_len, crypt->priv);
	if (res == 0 && crypt->ops->encrypt_mpdu)
		res = crypt->ops->encrypt_mpdu(frag, hdr_len, crypt->priv);

	atomic_dec(&crypt->refcnt);
	if (res < 0) {
		printk(KERN_INFO "%s: Encryption failed: len=%d.\n",
		       ieee->dev->name, frag->len);
		ieee->ieee_stats.tx_discards++;
		return -1;
	}

	return 0;
}


void rtllib_txb_free(struct rtllib_txb *txb) {
	//int i;
	if (unlikely(!txb))
		return;
#if 0
	for (i = 0; i < txb->nr_frags; i++)
		if (txb->fragments[i])
			dev_kfree_skb_any(txb->fragments[i]);
#endif
	kfree(txb);
}

struct rtllib_txb *rtllib_alloc_txb(int nr_frags, int txb_size,
					  int gfp_mask)
{
#ifdef USB_USE_ALIGNMENT
	u32 Tmpaddr=0;
	int alignment=0;
#endif
	struct rtllib_txb *txb;
	int i;
	txb = kmalloc(
		sizeof(struct rtllib_txb) + (sizeof(u8*) * nr_frags),
		gfp_mask);
	if (!txb)
		return NULL;

	memset(txb, 0, sizeof(struct rtllib_txb));
	txb->nr_frags = nr_frags;
	txb->frag_size = txb_size;

	for (i = 0; i < nr_frags; i++) {
#ifdef USB_USE_ALIGNMENT
		txb->fragments[i] = dev_alloc_skb(txb_size+USB_512B_ALIGNMENT_SIZE);
#else
		txb->fragments[i] = dev_alloc_skb(txb_size);
#endif
		if (unlikely(!txb->fragments[i])) {
			i--;
			break;
		}
#ifdef USB_USE_ALIGNMENT
		Tmpaddr = (u32)(txb->fragments[i]->data);
		alignment = Tmpaddr & 0x1ff;
		skb_reserve(txb->fragments[i],(USB_512B_ALIGNMENT_SIZE - alignment));
#endif
		memset(txb->fragments[i]->cb, 0, sizeof(txb->fragments[i]->cb));
	}
	if (unlikely(i != nr_frags)) {
		while (i >= 0)
			dev_kfree_skb_any(txb->fragments[i--]);
		kfree(txb);
		return NULL;
	}
	return txb;
}

// Classify the to-be send data packet
// Need to acquire the sent queue index.
int 
rtllib_classify(struct sk_buff *skb, u8 bIsAmsdu)
{
	struct ethhdr *eth;
	struct iphdr *ip;

	eth = (struct ethhdr *)skb->data;
	if (eth->h_proto != htons(ETH_P_IP))
		return 0;

#ifdef ENABLE_AMSDU
	if(bIsAmsdu)
		ip = (struct iphdr*)(skb->data + sizeof(struct ether_header) + AMSDU_SUBHEADER_LEN + SNAP_SIZE + sizeof(u16));
	else
		ip = (struct iphdr*)(skb->data + sizeof(struct ether_header));
#else
	RTLLIB_DEBUG_DATA(RTLLIB_DL_DATA, skb->data, skb->len);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22))
	ip = ip_hdr(skb);
#else
	ip = (struct iphdr*)(skb->data + sizeof(struct ether_header));
#endif
#endif
	switch (ip->tos & 0xfc) {
		case 0x20:
			return 2;
		case 0x40:
			return 1;
		case 0x60:
			return 3;
		case 0x80:
			return 4;
		case 0xa0:
			return 5;
		case 0xc0:
			return 6;
		case 0xe0:
			return 7;
		default:
			return 0;
	}
}

#define SN_LESS(a, b)		(((a-b)&0x800)!=0)
void rtllib_tx_query_agg_cap(struct rtllib_device* ieee, struct sk_buff* skb, cb_desc* tcb_desc)
{
	PRT_HIGH_THROUGHPUT	pHTInfo = ieee->pHTInfo;
	PTX_TS_RECORD			pTxTs = NULL;
	struct rtllib_hdr_1addr* hdr = (struct rtllib_hdr_1addr*)skb->data;

	if (!pHTInfo->bCurrentHTSupport||!pHTInfo->bEnableHT)
		return;
	if (!IsQoSDataFrame(skb->data))
		return;

	if (is_multicast_ether_addr(hdr->addr1) || is_broadcast_ether_addr(hdr->addr1))
		return;
	//check packet and mode later
#ifdef TO_DO_LIST
	if(pTcb->PacketLength >= 4096)
		return;
	// For RTL819X, if pairwisekey = wep/tkip, we don't aggrregation.
	if(!Adapter->HalFunc.GetNmodeSupportBySecCfgHandler(Adapter))
		return; 
#endif

	if(pHTInfo->IOTAction & HT_IOT_ACT_TX_NO_AGGREGATION)
		return;
	
	if(!ieee->GetNmodeSupportBySecCfg(ieee->dev))
	{
		return; 
	}
	if(pHTInfo->bCurrentAMPDUEnable)
	{
		if (!GetTs(ieee, (PTS_COMMON_INFO*)(&pTxTs), hdr->addr1, skb->priority, TX_DIR, true))
		{
			printk("===>can't get TS\n");
			return;
		}
		if (pTxTs->TxAdmittedBARecord.bValid == false)
		{
			//as some AP will refuse our action frame until key handshake has been finished. WB
			if (ieee->wpa_ie_len && (ieee->pairwise_key_type == KEY_TYPE_NA)) {
				;//printk("############### ieee->wpa_ie_len=%d ieee->pairwise_key_type=%d\n", ieee->wpa_ie_len,ieee->pairwise_key_type);
			} else if (tcb_desc->bdhcp == 1){
				;//dhcp my be very slow for some ap like linksys610N/350N
			} else if (!pTxTs->bDisable_AddBa){
				TsStartAddBaProcess(ieee, pTxTs);
			}
			goto FORCED_AGG_SETTING;
		}
		else if (pTxTs->bUsingBa == false)
		{
			if (SN_LESS(pTxTs->TxAdmittedBARecord.BaStartSeqCtrl.field.SeqNum, (pTxTs->TxCurSeq+1)%4096))
				pTxTs->bUsingBa = true;
			else
				goto FORCED_AGG_SETTING;
		}
#ifndef _RTL8192_EXT_PATCH_
		if (ieee->iw_mode == IW_MODE_INFRA)
#endif
		{
			tcb_desc->bAMPDUEnable = true;
			tcb_desc->ampdu_factor = pHTInfo->CurrentAMPDUFactor;
			tcb_desc->ampdu_density = pHTInfo->CurrentMPDUDensity;
		}
	}
FORCED_AGG_SETTING:
	switch(pHTInfo->ForcedAMPDUMode )
	{
		case HT_AGG_AUTO:
			break;

		case HT_AGG_FORCE_ENABLE:
			tcb_desc->bAMPDUEnable = true;
			tcb_desc->ampdu_density = pHTInfo->ForcedMPDUDensity;
			tcb_desc->ampdu_factor = pHTInfo->ForcedAMPDUFactor;
			break;

		case HT_AGG_FORCE_DISABLE:
			tcb_desc->bAMPDUEnable = false;
			tcb_desc->ampdu_density = 0;
			tcb_desc->ampdu_factor = 0;
			break;

	}	
		return;
}

extern void rtllib_qurey_ShortPreambleMode(struct rtllib_device* ieee, cb_desc* tcb_desc)
{
	tcb_desc->bUseShortPreamble = false;
	if (tcb_desc->data_rate == 2)
	{//// 1M can only use Long Preamble. 11B spec
		return;
	}
	else if (ieee->current_network.capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
	{
		tcb_desc->bUseShortPreamble = true;
	}
	return;	
}

extern	void	
rtllib_query_HTCapShortGI(struct rtllib_device *ieee, cb_desc *tcb_desc)
{
	PRT_HIGH_THROUGHPUT		pHTInfo = ieee->pHTInfo;

	tcb_desc->bUseShortGI 		= false;

	if(!pHTInfo->bCurrentHTSupport||!pHTInfo->bEnableHT)
		return;

	if(pHTInfo->bForcedShortGI)
	{
		tcb_desc->bUseShortGI = true;
		return;
	}

	if((pHTInfo->bCurBW40MHz==true) && pHTInfo->bCurShortGI40MHz)
		tcb_desc->bUseShortGI = true;
	else if((pHTInfo->bCurBW40MHz==false) && pHTInfo->bCurShortGI20MHz)
		tcb_desc->bUseShortGI = true;
}

void rtllib_query_BandwidthMode(struct rtllib_device* ieee, cb_desc *tcb_desc)
{
	PRT_HIGH_THROUGHPUT	pHTInfo = ieee->pHTInfo;

	tcb_desc->bPacketBW = false;

	if(!pHTInfo->bCurrentHTSupport||!pHTInfo->bEnableHT)
		return;

	if(tcb_desc->bMulticast || tcb_desc->bBroadcast)
		return;

	if((tcb_desc->data_rate & 0x80)==0) // If using legacy rate, it shall use 20MHz channel.
		return;
	//BandWidthAutoSwitch is for auto switch to 20 or 40 in long distance
	if(pHTInfo->bCurBW40MHz && pHTInfo->bCurTxBW40MHz && !ieee->bandwidth_auto_switch.bforced_tx20Mhz)
		tcb_desc->bPacketBW = true;
	return;
}
//added by amy for adhoc 090403
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
extern void rtllib_ibss_query_HTCapShortGI(struct rtllib_device *ieee, cb_desc *tcb_desc,u8 is_peer_shortGI_40M,u8 is_peer_shortGI_20M)
{
	PRT_HIGH_THROUGHPUT		pHTInfo = ieee->pHTInfo;

	tcb_desc->bUseShortGI 		= false;

	if(!pHTInfo->bCurrentHTSupport||!pHTInfo->bEnableHT || (ieee->iw_mode != IW_MODE_ADHOC))
	{
	//	printk("%s: curHT=%d EnableHT=%d\n", __FUNCTION__,pHTInfo->bCurrentHTSupport, pHTInfo->bEnableHT);
		return;
	}

	if(pHTInfo->bForcedShortGI)
	{
	//	printk("========>bForcedShortGI\n");
		tcb_desc->bUseShortGI = true;
		return;
	}
	if((pHTInfo->bCurBW40MHz==true) && is_peer_shortGI_40M)
		tcb_desc->bUseShortGI = true;
	else if((pHTInfo->bCurBW40MHz==false) && is_peer_shortGI_20M)
		tcb_desc->bUseShortGI = true;
	//printk("====>tcb_desc->bUseShortGI is %d\n",tcb_desc->bUseShortGI);
}
void rtllib_ibss_query_BandwidthMode(struct rtllib_device* ieee, cb_desc *tcb_desc, u8 is_peer_40M)
{
	PRT_HIGH_THROUGHPUT	pHTInfo = ieee->pHTInfo;

	tcb_desc->bPacketBW = false;

	if(!pHTInfo->bCurrentHTSupport||!pHTInfo->bEnableHT || (ieee->iw_mode != IW_MODE_ADHOC))
	{
	//	printk("%s: curHT=%d EnableHT=%d\n", __FUNCTION__,pHTInfo->bCurrentHTSupport, pHTInfo->bEnableHT);
		return;
	}

	if(tcb_desc->bMulticast || tcb_desc->bBroadcast)
	{
	//	printk("%s: Multicast=%d Broadcast=%d\n", __FUNCTION__, tcb_desc->bMulticast, tcb_desc->bBroadcast);
		return;
	}

	if((tcb_desc->data_rate & 0x80)==0) // If using legacy rate, it shall use 20MHz channel.
	{
	//	printk("%s: data_rate=0x%x\n", __FUNCTION__,tcb_desc->data_rate);
		return;
	}
	//BandWidthAutoSwitch is for auto switch to 20 or 40 in long distance
	if(pHTInfo->bCurBW40MHz && is_peer_40M && !ieee->bandwidth_auto_switch.bforced_tx20Mhz)
		tcb_desc->bPacketBW = true;
   	//printk("====>tcb_desc->bPacketBW is %d\n",tcb_desc->bPacketBW);
	return;
}
#endif
//added by amy for adhoc 090403 end
void rtllib_query_protectionmode(struct rtllib_device* ieee, cb_desc* tcb_desc, struct sk_buff* skb)
{
	// Common Settings
	tcb_desc->bRTSSTBC			= false;
	tcb_desc->bRTSUseShortGI		= false; // Since protection frames are always sent by legacy rate, ShortGI will never be used.
	tcb_desc->bCTSEnable			= false; // Most of protection using RTS/CTS
	tcb_desc->RTSSC				= 0;		// 20MHz: Don't care;  40MHz: Duplicate.
	tcb_desc->bRTSBW			= false; // RTS frame bandwidth is always 20MHz
	
	if(tcb_desc->bBroadcast || tcb_desc->bMulticast)//only unicast frame will use rts/cts
		return;
	
	if (is_broadcast_ether_addr(skb->data+16))  //check addr3 as infrastructure add3 is DA.
		return;

	if (ieee->mode < IEEE_N_24G) //b, g mode
	{
			// (1) RTS_Threshold is compared to the MPDU, not MSDU.
			// (2) If there are more than one frag in  this MSDU, only the first frag uses protection frame.
			//		Other fragments are protected by previous fragment.
			//		So we only need to check the length of first fragment.
		if (skb->len > ieee->rts)
		{
			tcb_desc->bRTSEnable = true;
			tcb_desc->rts_rate = MGN_24M;
		}
		else if (ieee->current_network.buseprotection)
		{
			// Use CTS-to-SELF in protection mode.
			tcb_desc->bRTSEnable = true;
			tcb_desc->bCTSEnable = true;
			tcb_desc->rts_rate = MGN_24M;
		}
		//otherwise return;
		return;
	}
	else
	{// 11n High throughput case.
		PRT_HIGH_THROUGHPUT pHTInfo = ieee->pHTInfo;
		while (true)
		{
			//check IOT action
			if(pHTInfo->IOTAction & HT_IOT_ACT_FORCED_CTS2SELF)
			{
				tcb_desc->bCTSEnable	= true;
				tcb_desc->rts_rate  = 	MGN_24M;
#if defined(RTL8192SE) || defined(RTL8192SU) || defined RTL8192CE
				tcb_desc->bRTSEnable = false;
#else
				tcb_desc->bRTSEnable = true;
#endif
				break;
			}
			else if(pHTInfo->IOTAction & (HT_IOT_ACT_FORCED_RTS|HT_IOT_ACT_PURE_N_MODE))
			{
				tcb_desc->bRTSEnable = true;
				tcb_desc->rts_rate  = 	MGN_24M;
				break;
			}
			//check ERP protection
			if (ieee->current_network.buseprotection)
			{// CTS-to-SELF 
				tcb_desc->bRTSEnable = true;
				tcb_desc->bCTSEnable = true;
				tcb_desc->rts_rate = MGN_24M;
				break;
			}
			//check HT op mode
			if(pHTInfo->bCurrentHTSupport  && pHTInfo->bEnableHT)
			{
				u8 HTOpMode = pHTInfo->CurrentOpMode;
				if((pHTInfo->bCurBW40MHz && (HTOpMode == 2 || HTOpMode == 3)) ||
							(!pHTInfo->bCurBW40MHz && HTOpMode == 3) )
				{
					tcb_desc->rts_rate = MGN_24M; // Rate is 24Mbps.
					tcb_desc->bRTSEnable = true;
					break;
				}
			}
			//check rts
			if (skb->len > ieee->rts)
			{
				tcb_desc->rts_rate = MGN_24M; // Rate is 24Mbps.
				tcb_desc->bRTSEnable = true;
				break;
			}
			//to do list: check MIMO power save condition.
			//check AMPDU aggregation for TXOP
			if(tcb_desc->bAMPDUEnable)
			{
				tcb_desc->rts_rate = MGN_24M; // Rate is 24Mbps.
				// According to 8190 design, firmware sends CF-End only if RTS/CTS is enabled. However, it degrads
				// throughput around 10M, so we disable of this mechanism. 2007.08.03 by Emily
				tcb_desc->bRTSEnable = false;
				break;
			}
			// Totally no protection case!!
			goto NO_PROTECTION;
		}
		}
	// For test , CTS replace with RTS 
	if( 0 )
	{
		tcb_desc->bCTSEnable	= true;
		tcb_desc->rts_rate = MGN_24M;
		tcb_desc->bRTSEnable 	= true;
	}
	if (ieee->current_network.capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
		tcb_desc->bUseShortPreamble = true;
	if (ieee->iw_mode == IW_MODE_MASTER)
			goto NO_PROTECTION;
	return;
NO_PROTECTION:
	tcb_desc->bRTSEnable	= false;
	tcb_desc->bCTSEnable	= false;
	tcb_desc->rts_rate		= 0;
	tcb_desc->RTSSC		= 0;
	tcb_desc->bRTSBW		= false;
}


#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
void rtllib_txrate_selectmode(struct rtllib_device* ieee, cb_desc* tcb_desc,u8 ratr_index)
#else
void rtllib_txrate_selectmode(struct rtllib_device* ieee, cb_desc* tcb_desc)
#endif
{
#ifdef TO_DO_LIST	
	if(!IsDataFrame(pFrame))
	{
		pTcb->bTxDisableRateFallBack = true;
		pTcb->bTxUseDriverAssingedRate = true;
		pTcb->RATRIndex = 7;
		return;
	}

	if(pMgntInfo->ForcedDataRate!= 0)
	{
		pTcb->bTxDisableRateFallBack = true;
		pTcb->bTxUseDriverAssingedRate = true;
		return;
	}
#endif
	if(ieee->bTxDisableRateFallBack)
		tcb_desc->bTxDisableRateFallBack = true;

	if(ieee->bTxUseDriverAssingedRate)
		tcb_desc->bTxUseDriverAssingedRate = true;
	if(!tcb_desc->bTxDisableRateFallBack || !tcb_desc->bTxUseDriverAssingedRate)
	{
#ifdef _RTL8192_EXT_PATCH_
		if ((ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_ADHOC) 
			//|| ((ieee->iw_mode == IW_MODE_MESH) && (ieee->only_mesh == 0)))  //YJ,test,090611
			|| (ieee->iw_mode == IW_MODE_MESH)) 
#else			
		if (ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_ADHOC) 
#endif		
			tcb_desc->RATRIndex = 0;
	}
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	if(ieee->iw_mode == IW_MODE_ADHOC)
		tcb_desc->RATRIndex = ratr_index;
#endif

#ifdef _RTL8192_EXT_PATCH_
	if(ieee->bUseRAMask){
		tcb_desc->macId =0;
	//	printk("=============>%s():bUseRAMask is true,tcb_desc->macId is %d\n",__FUNCTION__,tcb_desc->macId);
	}
#endif
}

u16 rtllib_query_seqnum(struct rtllib_device*ieee, struct sk_buff* skb, u8* dst)
{
	u16 seqnum = 0;

	if (is_multicast_ether_addr(dst) || is_broadcast_ether_addr(dst))
		return 0;
	if (IsQoSDataFrame(skb->data)) //we deal qos data only
	{
		PTX_TS_RECORD pTS = NULL;
		if (!GetTs(ieee, (PTS_COMMON_INFO*)(&pTS), dst, skb->priority, TX_DIR, true))
		{
			return 0;
		}
		seqnum = pTS->TxCurSeq;
		pTS->TxCurSeq = (pTS->TxCurSeq+1)%4096;
		return seqnum;
	}
	return 0;
}



#ifdef ENABLE_AMSDU
#if 0
static void CB_DESC_DUMP(pcb_desc tcb, char* func)
{
	printk("\n%s",func);
	printk("\n-------------------CB DESC DUMP ><------------------------");
	printk("\npkt_size:\t %d", tcb->pkt_size);
	printk("\nqueue_index:\t %d", tcb->queue_index);
	printk("\nbMulticast:\t %d", tcb->bMulticast);
	printk("\nbBroadcast:\t %d", tcb->bBroadcast);
	printk("\nbPacketBw:\t %d", tcb->bPacketBW);
	printk("\nbRTSEnable:\t %d", tcb->bRTSEnable);
	printk("\nrts_rate:\t %d", tcb->rts_rate);
//	printk("\nbEncrypt:\t %d", tcb->bEncrypt);
//	printk("\nbHwSec:\t %d", tcb->bHwSec);
	printk("\nbUseShortGI:\t %d", tcb->bUseShortGI);
//	printk("\nbAMPDUEnable:\t %d", tcb->bAMPDUEnable);
//	printk("\nbDrvAggEnable:\t %d", tcb->drv_agg_enable);
	printk("\nbAMSDU:\t %d", tcb->bAMSDU);
	printk("\nFromAggrQ:\t %d", tcb->bFromAggrQ);
//	printk("\nrata_index:\t %d", tcb->rata_index);
//	printk("\ntxbuf_size:\t %d", tcb->txbuf_size);
	printk("\nRATRIndex:\t %d", tcb->RATRIndex);
	printk("\ndata_rate:\t %d", tcb->data_rate);
//	printk("\nampdu_factor:\t %d", tcb->ampdu_factor);
//	printk("\nampdu_density:\t %d", tcb->ampdu_density);
//	printk("\nDrvAggrNum:\t %d", tcb->DrvAggrNum);
	printk("\n-------------------CB DESC DUMP <>------------------------\n");
}
#endif
struct sk_buff *AMSDU_Aggregation(
	struct rtllib_device 	*ieee,
	struct sk_buff_head 		*pSendList
	)
{
	struct sk_buff *	pSkb;
	struct sk_buff * 	pAggrSkb;
	u8		i;
	u32		total_length = 0;
	u32		skb_len, num_skb;
	pcb_desc 	pcb;
	u8 		amsdu_shdr[AMSDU_SUBHEADER_LEN];
	u8		padding = 0;
	u8		*p = NULL, *q=NULL;
	u16		ether_type;

	//
	// Calculate the total length
	//
	num_skb = skb_queue_len(pSendList);
	if(num_skb == 0)
		return NULL;
	if(num_skb == 1)
	{
		pSkb = (struct sk_buff *)skb_dequeue(pSendList);
		memset(pSkb->cb, 0, sizeof(pSkb->cb));
		pcb = (pcb_desc)(pSkb->cb + MAX_DEV_ADDR_SIZE);
		pcb->bFromAggrQ = true;
		return pSkb;
	}

	total_length += sizeof(struct ethhdr);
	for(i=0; i<num_skb; i++)	
	{
		pSkb= (struct sk_buff *)skb_dequeue(pSendList);
		if(pSkb->len <= (ETH_ALEN*2))
		{
			dev_kfree_skb_any(pSkb);
			continue;
		}
		skb_len = pSkb->len - ETH_ALEN*2 + SNAP_SIZE + AMSDU_SUBHEADER_LEN;
		if(i < (num_skb-1))
		{
			skb_len += ((4-skb_len%4)==4)?0:(4-skb_len%4);
		}
		total_length += skb_len;
		skb_queue_tail(pSendList, pSkb);
	}
	
	//
	// Create A-MSDU
	//
	pAggrSkb = dev_alloc_skb(total_length);
	if(NULL == pAggrSkb)
	{
		skb_queue_purge(pSendList);
		printk("%s: Can not alloc skb!\n", __FUNCTION__);
		return NULL;
	}
	skb_put(pAggrSkb,total_length);
	pAggrSkb->priority = pSkb->priority;

	//
	// Fill AMSDU attibutes within cb
	//
	memset(pAggrSkb->cb, 0, sizeof(pAggrSkb->cb));
	pcb = (pcb_desc)(pAggrSkb->cb + MAX_DEV_ADDR_SIZE);
	pcb->bFromAggrQ = true;
	pcb->bAMSDU = true;

	//printk("======== In %s: num_skb=%d total_length=%d\n", __FUNCTION__,num_skb, total_length);
	//
	// Make A-MSDU
	//
	memset(amsdu_shdr, 0, AMSDU_SUBHEADER_LEN);
	p = pAggrSkb->data;
	for(i=0; i<num_skb; i++)	
	{
		q = p;
		pSkb= (struct sk_buff *)skb_dequeue(pSendList);
		ether_type = ntohs(((struct ethhdr *)pSkb->data)->h_proto);

		skb_len = pSkb->len - sizeof(struct ethhdr) + AMSDU_SUBHEADER_LEN + SNAP_SIZE + sizeof(u16);
		if(i < (num_skb-1))
		{
			padding = ((4-skb_len%4)==4)?0:(4-skb_len%4);
			skb_len += padding;
		}
		if(i == 0)
		{
			memcpy(p, pSkb->data, sizeof(struct ethhdr));
			p += sizeof(struct ethhdr);
		}
		//if(memcmp(pSkb->data, pAggrSkb->data, sizeof(struct ethhdr)))
		//	printk(""MAC_FMT"-"MAC_FMT"\n",MAC_ARG(pSkb->data), MAC_ARG(pAggrSkb->data));
		memcpy(amsdu_shdr, pSkb->data, (ETH_ALEN*2));
		skb_pull(pSkb, sizeof(struct ethhdr));
		*(u16*)(amsdu_shdr+ETH_ALEN*2) = ntohs(pSkb->len + SNAP_SIZE + sizeof(u16));
		memcpy(p, amsdu_shdr, AMSDU_SUBHEADER_LEN);
		p += AMSDU_SUBHEADER_LEN;

		rtllib_put_snap(p, ether_type);
		p += SNAP_SIZE + sizeof(u16);

		memcpy(p, pSkb->data, pSkb->len);
		p += pSkb->len;
		if(padding > 0)
		{
			memset(p, 0, padding);
			p += padding;
			padding = 0;
		}
		dev_kfree_skb_any(pSkb);
	}
	
	//printk("-------%d\n",pAggrSkb->len);
	return pAggrSkb;
}


/* NOTE:
	This function return a list of SKB which is proper to be aggregated. 
	If no proper SKB is found to do aggregation, SendList will only contain the input SKB.
*/
u8 AMSDU_GetAggregatibleList(
	struct rtllib_device *	ieee,
	struct sk_buff *		pCurSkb,
	struct sk_buff_head 		*pSendList,
	u8				queue_index
	)
{
	struct sk_buff 			*pSkb = NULL;
	u16				nMaxAMSDUSize = 0;
	u32				AggrSize = 0;
	u32				nAggrSkbNum = 0;
	u8 				padding = 0;
	struct sta_info			*psta = NULL;	
	u8 				*addr = (u8*)(pCurSkb->data);	
	struct sk_buff_head *header;
	struct sk_buff 	  *punlinkskb = NULL;	

	padding = ((4-pCurSkb->len%4)==4)?0:(4-pCurSkb->len%4);
	AggrSize = AMSDU_SUBHEADER_LEN + pCurSkb->len + padding;
	skb_queue_tail(pSendList, pCurSkb);
	nAggrSkbNum++;

	//
	// Get A-MSDU aggregation threshold.
	//
	if(ieee->iw_mode == IW_MODE_MASTER){	
		psta = GetStaInfo(ieee, addr);
		if(NULL != psta)
			nMaxAMSDUSize = psta->htinfo.AMSDU_MaxSize;
		else
			return 1;
	}else if(ieee->iw_mode == IW_MODE_ADHOC){
		psta = GetStaInfo(ieee, addr);
		if(NULL != psta)
			nMaxAMSDUSize = psta->htinfo.AMSDU_MaxSize;
		else
			return 1;
	}else{
		nMaxAMSDUSize = ieee->pHTInfo->nCurrent_AMSDU_MaxSize;
	}
	nMaxAMSDUSize = ((nMaxAMSDUSize)==0)?HT_AMSDU_SIZE_4K:HT_AMSDU_SIZE_8K;

	if(ieee->pHTInfo->ForcedAMSDUMode == HT_AGG_FORCE_ENABLE)
	{
		nMaxAMSDUSize = ieee->pHTInfo->ForcedAMSDUMaxSize;
	}
	
	// 
	// Build pSendList
	//
	header = (&ieee->skb_aggQ[queue_index]);
	pSkb = header->next;
	while(pSkb != (struct sk_buff*)header)
	{
		//
		// Get Aggregation List. Only those frames with the same RA can be aggregated.
		// For Infrastructure mode, RA is always the AP MAC address so the frame can just be aggregated 
		// without checking RA.
		// For AP mode and IBSS mode, RA is the same as DA. Checking RA is needed before aggregation.
		//
		if((ieee->iw_mode == IW_MODE_MASTER) ||(ieee->iw_mode == IW_MODE_ADHOC))
		{
			if(memcmp(pCurSkb->data, pSkb->data, ETH_ALEN) != 0) //DA
			{
				//printk(""MAC_FMT"-"MAC_FMT"\n",MAC_ARG(pCurSkb->data), MAC_ARG(pSkb->data));
				pSkb = pSkb->next;
				continue;				
			}
		}
		//
		// Limitation shall be checked:
		// (1) A-MSDU size limitation
		//
		if((AMSDU_SUBHEADER_LEN + pSkb->len + AggrSize < nMaxAMSDUSize) )
		{
			// Unlink skb
			punlinkskb = pSkb;
			pSkb = pSkb->next;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)
			skb_unlink(punlinkskb, header);	
#else
			/*
			 * __skb_unlink before linux2.6.14 does not use spinlock to protect list head.
			 * add spinlock function manually. john,2008/12/03
			 */
			{
				unsigned long flags;
				spin_lock_irqsave(&ieee->lock, flags);
				__skb_unlink(punlinkskb,header);
				spin_unlock_irqrestore(&ieee->lock, flags);
			}
#endif
			
			//Do aggregation
			padding = ((4-punlinkskb->len%4)==4)?0:(4-punlinkskb->len%4);
			AggrSize += AMSDU_SUBHEADER_LEN + punlinkskb->len + padding;
			//printk(""MAC_FMT": %d\n",MAC_ARG(punlinkskb->data),punlinkskb->len);
			skb_queue_tail(pSendList, punlinkskb);
			nAggrSkbNum++;
		}
		else
		{
			//Do not do aggregation because out of resources
			//printk("\nStop aggregation: ");
			if(!(AMSDU_SUBHEADER_LEN + pSkb->len + AggrSize < nMaxAMSDUSize))
				;//printk("[A-MSDU size limitation]");
		
			break; //To be sure First In First Out, 'break' should be marked, 
		}
	}
	return nAggrSkbNum;
}
#endif
static int wme_downgrade_ac(struct sk_buff *skb)
{
	switch (skb->priority) {
		case 6:
		case 7:
			skb->priority = 5; /* VO -> VI */
			return 0;
		case 4:
		case 5:
			skb->priority = 3; /* VI -> BE */
			return 0;
		case 0:
		case 3:
			skb->priority = 1; /* BE -> BK */
			return 0;
		default:
			return -1;
	}
}

int rtllib_xmit_inter(struct sk_buff *skb, struct net_device *dev)
{
	struct rtllib_device *ieee = (struct rtllib_device *)netdev_priv_rsl(dev);
	struct rtllib_txb *txb = NULL;
	struct rtllib_hdr_3addrqos *frag_hdr;
	int i, bytes_per_frag, nr_frags, bytes_last_frag, frag_size;
	unsigned long flags;
	struct net_device_stats *stats = &ieee->stats;
	int ether_type = 0, encrypt;
	int bytes, fc, qos_ctl = 0, hdr_len;
	struct sk_buff *skb_frag;
	struct rtllib_hdr_3addrqos header = { /* Ensure zero initialized */
		.duration_id = 0,
		.seq_ctl = 0,
		.qos_ctl = 0
	};
	u8 dest[ETH_ALEN], src[ETH_ALEN];
	int qos_actived = ieee->current_network.qos_data.active;
	struct rtllib_crypt_data* crypt;
	cb_desc *tcb_desc;
	u8 bIsMulticast = false;
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
	struct sta_info *p_sta = NULL;
#endif	
	u8 IsAmsdu = false;
#ifdef ENABLE_AMSDU	
	u8 queue_index = WME_AC_BE;
	cb_desc *tcb_desc_skb;
	u8 bIsSptAmsdu = false;
#endif	

	bool	bdhcp =false;
#ifndef _RTL8192_EXT_PATCH_
	//PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(ieee->PowerSaveControl));//added by amy for Leisure PS 090402
#endif
	spin_lock_irqsave(&ieee->lock, flags);

	/* If there is no driver handler to take the TXB, dont' bother
	 * creating it... */
	if ((!ieee->hard_start_xmit && !(ieee->softmac_features & IEEE_SOFTMAC_TX_QUEUE))||
	   ((!ieee->softmac_data_hard_start_xmit && (ieee->softmac_features & IEEE_SOFTMAC_TX_QUEUE)))) {
		printk(KERN_WARNING "%s: No xmit handler.\n",
		       ieee->dev->name);
		goto success;
	}
	

	if(likely(ieee->raw_tx == 0)){
		if (unlikely(skb->len < SNAP_SIZE + sizeof(u16))) {
			printk(KERN_WARNING "%s: skb too small (%d).\n",
			ieee->dev->name, skb->len);
			goto success;
		}

		/* Save source and destination addresses */
		memcpy(dest, skb->data, ETH_ALEN);
		memcpy(src, skb->data+ETH_ALEN, ETH_ALEN);
	
#ifdef ENABLE_AMSDU	
		if(ieee->iw_mode == IW_MODE_ADHOC)
		{
			p_sta = GetStaInfo(ieee, dest);
			if(p_sta)	{
				if(p_sta->htinfo.bEnableHT)
					bIsSptAmsdu = true;
			}
		}else if(ieee->iw_mode == IW_MODE_INFRA) {
			bIsSptAmsdu = true;
		}else
			bIsSptAmsdu = true;
		bIsSptAmsdu = (bIsSptAmsdu && ieee->pHTInfo->bCurrent_AMSDU_Support && qos_actived);
			
		//u8 *a = skb->data;
		//u8 *b = (u8*)skb->data + ETH_ALEN;
		//printk("\n&&&&&&&skb=%p len=%d dst:"MAC_FMT" src:"MAC_FMT"\n",skb,skb->len,MAC_ARG(a),MAC_ARG(b));
		tcb_desc_skb = (pcb_desc)(skb->cb + MAX_DEV_ADDR_SIZE);  //YJ,move,081104
		if(bIsSptAmsdu) {
			if(!tcb_desc_skb->bFromAggrQ)  //Normal MSDU
			{
				if(qos_actived)
				{
					queue_index = UP2AC(skb->priority);
				} else {
					queue_index = WME_AC_BE;
				}

				//printk("Normal MSDU,queue_idx=%d nic_enough=%d queue_len=%d\n", queue_index, ieee->check_nic_enough_desc(ieee->dev,queue_index), skb_queue_len(&ieee->skb_aggQ[queue_index]));
				if ((skb_queue_len(&ieee->skb_aggQ[queue_index]) != 0)||
#if defined RTL8192SE || defined RTL8192CE
				   (ieee->get_nic_desc_num(ieee->dev,queue_index)) > 1||
#else
				   (!ieee->check_nic_enough_desc(ieee->dev,queue_index))||
#endif
				   (ieee->queue_stop) ||
				   (ieee->amsdu_in_process)) //YJ,add,090409 
				{
					/* insert the skb packet to the Aggregation queue */
					//printk("!!!!!!!!!!%s(): intert to aggr queue\n", __FUNCTION__);
					skb_queue_tail(&ieee->skb_aggQ[queue_index], skb);
					spin_unlock_irqrestore(&ieee->lock, flags);
					return 0;
				}
			}
			else  //AMSDU
			{
				//printk("AMSDU!!!!!!!!!!!!!\n");
				if(tcb_desc_skb->bAMSDU)
					IsAmsdu = true;
				
				//YJ,add,090409
				ieee->amsdu_in_process = false;
			}
		}
#endif	
		memset(skb->cb, 0, sizeof(skb->cb));
		ether_type = ntohs(((struct ethhdr *)skb->data)->h_proto);

		// The following is for DHCP and ARP packet, we use cck1M to tx these packets and let LPS awake some time 
		// to prevent DHCP protocol fail
		if (skb->len > 282){//MINIMUM_DHCP_PACKET_SIZE) {
			if (ETH_P_IP == ether_type) {// IP header
				const struct iphdr *ip = (struct iphdr *)((u8 *)skb->data+14);
				if (IPPROTO_UDP == ip->protocol) {//FIXME windows is 11 but here UDP in linux kernel is 17.
					struct udphdr *udp = (struct udphdr *)((u8 *)ip + (ip->ihl << 2));
					//if(((ntohs(udp->source) == 68) && (ntohs(udp->dest) == 67)) ||
					 ///   ((ntohs(udp->source) == 67) && (ntohs(udp->dest) == 68))) {
					if(((((u8 *)udp)[1] == 68) && (((u8 *)udp)[3] == 67)) ||
					    ((((u8 *)udp)[1] == 67) && (((u8 *)udp)[3] == 68))) {
						// 68 : UDP BOOTP client
						// 67 : UDP BOOTP server
						printk("===>DHCP Protocol start tx DHCP pkt src port:%d, dest port:%d!!\n", ((u8 *)udp)[1],((u8 *)udp)[3]);
						// Use low rate to send DHCP packet.
						//if(pMgntInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)	
						//{
						//	tcb_desc->DataRate = MgntQuery_TxRateExcludeCCKRates(ieee);//0xc;//ofdm 6m
						//	tcb_desc->bTxDisableRateFallBack = false;
						//}
						//else
						//pTcb->DataRate = Adapter->MgntInfo.LowestBasicRate; 
						//RTPRINT(FDM, WA_IOT, ("DHCP TranslateHeader(), pTcb->DataRate = 0x%x\n", pTcb->DataRate));

						bdhcp = true;
#ifdef _RTL8192_EXT_PATCH_
						ieee->LPSDelayCnt = 100;//pPSC->LPSAwakeIntvl*2; //AMY,090701
#else
						ieee->LPSDelayCnt = 100;//pPSC->LPSAwakeIntvl*2;
#endif	
					}
				}
			}else if(ETH_P_ARP == ether_type){// IP ARP packet
				printk("=================>DHCP Protocol start tx ARP pkt!!\n");
				bdhcp = true;
				ieee->LPSDelayCnt = ieee->current_network.tim.tim_count;

				//if(pMgntInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)	
				//{
				//	tcb_desc->DataRate = MgntQuery_TxRateExcludeCCKRates(Adapter->MgntInfo.mBrates);//0xc;//ofdm 6m
				//	tcb_desc->bTxDisableRateFallBack = FALSE;
				//}
				//else
				//	tcb_desc->DataRate = Adapter->MgntInfo.LowestBasicRate; 
				//RTPRINT(FDM, WA_IOT, ("ARP TranslateHeader(), pTcb->DataRate = 0x%x\n", pTcb->DataRate));

			}
		}
		
		skb->priority = rtllib_classify(skb, IsAmsdu);
	
#ifdef _RTL8192_EXT_PATCH_
		crypt = ieee->sta_crypt[ieee->tx_keyidx];
#else
		crypt = ieee->crypt[ieee->tx_keyidx];
#endif	
		encrypt = !(ether_type == ETH_P_PAE && ieee->ieee802_1x) &&
			ieee->host_encrypt && crypt && crypt->ops;
	
		if (!encrypt && ieee->ieee802_1x &&
		ieee->drop_unencrypted && ether_type != ETH_P_PAE) {
			stats->tx_dropped++;
			goto success;
		}
	#ifdef CONFIG_RTLLIB_DEBUG
		if (crypt && !encrypt && ether_type == ETH_P_PAE) {
			struct eapol *eap = (struct eapol *)(skb->data +
				sizeof(struct ethhdr) - SNAP_SIZE - sizeof(u16));
			RTLLIB_DEBUG_EAP("TX: IEEE 802.11 EAPOL frame: %s\n",
				eap_get_type(eap->type));
		}
	#endif
	
		/* Advance the SKB to the start of the payload */
		skb_pull(skb, sizeof(struct ethhdr));

                /* Determine total amount of storage required for TXB packets */
#ifdef ENABLE_AMSDU	
		if(!IsAmsdu)
			bytes = skb->len + SNAP_SIZE + sizeof(u16);
		else
			bytes = skb->len;
#else
		bytes = skb->len + SNAP_SIZE + sizeof(u16);
#endif	

		if (encrypt)
			fc = RTLLIB_FTYPE_DATA | RTLLIB_FCTL_WEP;
		else 
			fc = RTLLIB_FTYPE_DATA; 
		
		//if(ieee->current_network.QoS_Enable) 
		if(qos_actived)
			fc |= RTLLIB_STYPE_QOS_DATA; 
		else
			fc |= RTLLIB_STYPE_DATA;
	
#ifdef _RTL8192_EXT_PATCH_
		if ((ieee->iw_mode == IW_MODE_INFRA) 
			//|| ((ieee->iw_mode == IW_MODE_MESH) && (ieee->only_mesh == 0)))  //YJ,test,090610
			|| (ieee->iw_mode == IW_MODE_MESH) ) 
#else
		if (ieee->iw_mode == IW_MODE_INFRA) 
#endif
		{
			fc |= RTLLIB_FCTL_TODS;
			/* To DS: Addr1 = BSSID, Addr2 = SA,
			Addr3 = DA */
			memcpy(&header.addr1, ieee->current_network.bssid, ETH_ALEN);
			memcpy(&header.addr2, &src, ETH_ALEN);
			if(IsAmsdu)
				memcpy(&header.addr3, ieee->current_network.bssid, ETH_ALEN);
			else
				memcpy(&header.addr3, &dest, ETH_ALEN);
		} else if (ieee->iw_mode == IW_MODE_ADHOC) {
			/* not From/To DS: Addr1 = DA, Addr2 = SA,
			Addr3 = BSSID */
			memcpy(&header.addr1, dest, ETH_ALEN);
			memcpy(&header.addr2, src, ETH_ALEN);
			memcpy(&header.addr3, ieee->current_network.bssid, ETH_ALEN);
		}

		bIsMulticast = is_broadcast_ether_addr(header.addr1) ||is_multicast_ether_addr(header.addr1);

                header.frame_ctl = cpu_to_le16(fc);

		/* Determine fragmentation size based on destination (multicast
		* and broadcast are not fragmented) */
		if (bIsMulticast) {
			frag_size = MAX_FRAG_THRESHOLD;
			qos_ctl |= QOS_CTL_NOTCONTAIN_ACK;
		}
		else {
#ifdef ENABLE_AMSDU	
			if(bIsSptAmsdu) {
				if(ieee->iw_mode == IW_MODE_ADHOC) {
					if(p_sta)
						frag_size = p_sta->htinfo.AMSDU_MaxSize;
					else
						frag_size = ieee->pHTInfo->nAMSDU_MaxSize;
				}
				else
					frag_size = ieee->pHTInfo->nAMSDU_MaxSize;
				qos_ctl = 0;
			}
			else
#endif	
			{
				frag_size = ieee->fts;//default:392
				qos_ctl = 0;
			}
		}
	
		if(qos_actived)
		{
			hdr_len = RTLLIB_3ADDR_LEN + 2;

                    /* in case we are a client verify acm is not set for this ac */
                    while (unlikely(ieee->wmm_acm & (0x01 << skb->priority))) {
                        printk("skb->priority = %x\n", skb->priority);
                        if (wme_downgrade_ac(skb)) {
                            break;
                        }
                        printk("converted skb->priority = %x\n", skb->priority);
                    }
                    qos_ctl |= skb->priority; //set in the rtllib_classify 	
#ifdef ENABLE_AMSDU	
			if(IsAmsdu)
			{
				qos_ctl |= QOS_CTL_AMSDU_PRESENT;
			}
                    header.qos_ctl = cpu_to_le16(qos_ctl);
#else	
                    header.qos_ctl = cpu_to_le16(qos_ctl & RTLLIB_QOS_TID);
#endif
		} else {
			hdr_len = RTLLIB_3ADDR_LEN;		
		}
		/* Determine amount of payload per fragment.  Regardless of if
		* this stack is providing the full 802.11 header, one will
		* eventually be affixed to this fragment -- so we must account for
		* it when determining the amount of payload space. */
		bytes_per_frag = frag_size - hdr_len;
		if (ieee->config &
		(CFG_RTLLIB_COMPUTE_FCS | CFG_RTLLIB_RESERVE_FCS))
			bytes_per_frag -= RTLLIB_FCS_LEN;
	
		/* Each fragment may need to have room for encryptiong pre/postfix */
		if (encrypt)
			bytes_per_frag -= crypt->ops->extra_prefix_len +
				crypt->ops->extra_postfix_len;
	
		/* Number of fragments is the total bytes_per_frag /
		* payload_per_fragment */
		nr_frags = bytes / bytes_per_frag;
		bytes_last_frag = bytes % bytes_per_frag;
		if (bytes_last_frag)
			nr_frags++;
		else
			bytes_last_frag = bytes_per_frag;
	
		/* When we allocate the TXB we allocate enough space for the reserve
		* and full fragment bytes (bytes_per_frag doesn't include prefix,
		* postfix, header, FCS, etc.) */
		txb = rtllib_alloc_txb(nr_frags, frag_size + ieee->tx_headroom, GFP_ATOMIC);
		if (unlikely(!txb)) {
			printk(KERN_WARNING "%s: Could not allocate TXB\n",
			ieee->dev->name);
			goto failed;
		}
		txb->encrypted = encrypt;
		txb->payload_size = bytes;

		//if (ieee->current_network.QoS_Enable) 
		if(qos_actived)
		{
			txb->queue_index = UP2AC(skb->priority);
		} else {
			txb->queue_index = WME_AC_BE;;
		}

		for (i = 0; i < nr_frags; i++) {
			skb_frag = txb->fragments[i];
			tcb_desc = (cb_desc *)(skb_frag->cb + MAX_DEV_ADDR_SIZE);
#ifdef _RTL8192_EXT_PATCH_
			tcb_desc->mesh_pkt = 0;//AMY added 090226
			if(ieee->iw_mode == IW_MODE_ADHOC)
				tcb_desc->badhoc = 1;
			else
				tcb_desc->badhoc = 0;
#endif
			if(qos_actived){
				skb_frag->priority = skb->priority;//UP2AC(skb->priority);	
				tcb_desc->queue_index =  UP2AC(skb->priority);
			} else {
				skb_frag->priority = WME_AC_BE;
				tcb_desc->queue_index = WME_AC_BE;
			}
			skb_reserve(skb_frag, ieee->tx_headroom);

			if (encrypt){
				if (ieee->hwsec_active)
					tcb_desc->bHwSec = 1;
				else
					tcb_desc->bHwSec = 0;
				skb_reserve(skb_frag, crypt->ops->extra_prefix_len);
			}
			else
			{
				tcb_desc->bHwSec = 0;
			}
			frag_hdr = (struct rtllib_hdr_3addrqos *)skb_put(skb_frag, hdr_len);
			memcpy(frag_hdr, &header, hdr_len);
	
			/* If this is not the last fragment, then add the MOREFRAGS
			* bit to the frame control */
			if (i != nr_frags - 1) {
				frag_hdr->frame_ctl = cpu_to_le16(
					fc | RTLLIB_FCTL_MOREFRAGS);
				bytes = bytes_per_frag;
		
			} else {
				/* The last fragment takes the remaining length */
				bytes = bytes_last_frag;
			}
			//if(ieee->current_network.QoS_Enable) 
			if((qos_actived) && (!bIsMulticast))
			{	
				// add 1 only indicate to corresponding seq number control 2006/7/12
				//frag_hdr->seq_ctl = cpu_to_le16(ieee->seq_ctrl[UP2AC(skb->priority)+1]<<4 | i);
				frag_hdr->seq_ctl = rtllib_query_seqnum(ieee, skb_frag, header.addr1); 
				frag_hdr->seq_ctl = cpu_to_le16(frag_hdr->seq_ctl<<4 | i);
			} else {
				frag_hdr->seq_ctl = cpu_to_le16(ieee->seq_ctrl[0]<<4 | i);
			}
			/* Put a SNAP header on the first fragment */
#ifdef ENABLE_AMSDU	
			if ((i == 0) && (!IsAmsdu)) 
#else
			if (i == 0) 
#endif	
			{
				rtllib_put_snap(
					skb_put(skb_frag, SNAP_SIZE + sizeof(u16)),
					ether_type);
				bytes -= SNAP_SIZE + sizeof(u16);
			}
	
			memcpy(skb_put(skb_frag, bytes), skb->data, bytes);
	
			/* Advance the SKB... */
			skb_pull(skb, bytes);
	
			/* Encryption routine will move the header forward in order
			* to insert the IV between the header and the payload */
			if (encrypt)
#ifdef _RTL8192_EXT_PATCH_
				rtllib_encrypt_fragment(ieee, skb_frag, hdr_len, 0);
#else
				rtllib_encrypt_fragment(ieee, skb_frag, hdr_len);
#endif
			if (ieee->config &
			(CFG_RTLLIB_COMPUTE_FCS | CFG_RTLLIB_RESERVE_FCS))
				skb_put(skb_frag, 4);
		}

		if((qos_actived) && (!bIsMulticast))
		{
		  if (ieee->seq_ctrl[UP2AC(skb->priority) + 1] == 0xFFF)
			ieee->seq_ctrl[UP2AC(skb->priority) + 1] = 0;
		  else
			ieee->seq_ctrl[UP2AC(skb->priority) + 1]++;
		} else {
  		  if (ieee->seq_ctrl[0] == 0xFFF)
			ieee->seq_ctrl[0] = 0;
		  else
			ieee->seq_ctrl[0]++;
		}
	}else{
		if (unlikely(skb->len < sizeof(struct rtllib_hdr_3addr))) {
			printk(KERN_WARNING "%s: skb too small (%d).\n",
			ieee->dev->name, skb->len);
			goto success;
		}
	
		txb = rtllib_alloc_txb(1, skb->len, GFP_ATOMIC);
		if(!txb){
			printk(KERN_WARNING "%s: Could not allocate TXB\n",
			ieee->dev->name);
			goto failed;
		}
		
		txb->encrypted = 0;
		txb->payload_size = skb->len;
		memcpy(skb_put(txb->fragments[0],skb->len), skb->data, skb->len);
	}	

 success:
//WB add to fill data tcb_desc here. only first fragment is considered, need to change, and you may remove to other place.
	if (txb)
	{
#if 1	
		cb_desc *tcb_desc = (cb_desc *)(txb->fragments[0]->cb + MAX_DEV_ADDR_SIZE);
		tcb_desc->bTxEnableFwCalcDur = 1;
		tcb_desc->priority = skb->priority;

                if(ether_type == ETH_P_PAE) {
			if(ieee->pHTInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)	
			{
				tcb_desc->data_rate = MgntQuery_TxRateExcludeCCKRates(ieee);//0xc;//ofdm 6m
				tcb_desc->bTxDisableRateFallBack = false;
			}else{
                        tcb_desc->data_rate = ieee->basic_rate;
                        tcb_desc->bTxDisableRateFallBack = 1;
			}
			
			printk("EAPOL TranslateHeader(), pTcb->DataRate = 0x%x\n", tcb_desc->data_rate);
			
                        tcb_desc->RATRIndex = 7;                        
                        tcb_desc->bTxUseDriverAssingedRate = 1;
                } else {
		if (is_multicast_ether_addr(header.addr1))
			tcb_desc->bMulticast = 1;
		if (is_broadcast_ether_addr(header.addr1))
			tcb_desc->bBroadcast = 1;
#if defined(RTL8192U) || defined(RTL8192SU) || defined(RTL8192SE)
		if ( tcb_desc->bMulticast ||  tcb_desc->bBroadcast){
			rtllib_txrate_selectmode(ieee, tcb_desc, 7);  
			tcb_desc->data_rate = ieee->basic_rate;
		}
		else
		{
			if(ieee->iw_mode == IW_MODE_ADHOC)
			{
				u8 is_peer_shortGI_40M = 0;
				u8 is_peer_shortGI_20M = 0;
				u8 is_peer_BW_40M = 0;
				p_sta = GetStaInfo(ieee, header.addr1);
				if(NULL == p_sta)
				{
					rtllib_txrate_selectmode(ieee, tcb_desc, 7);
					tcb_desc->data_rate = ieee->rate;
				}
				else
				{
					rtllib_txrate_selectmode(ieee, tcb_desc, p_sta->ratr_index);
					tcb_desc->data_rate = CURRENT_RATE(p_sta->wireless_mode, p_sta->CurDataRate, p_sta->htinfo.HTHighestOperaRate);
					is_peer_shortGI_40M = p_sta->htinfo.bCurShortGI40MHz;
					is_peer_shortGI_20M = p_sta->htinfo.bCurShortGI20MHz;
					is_peer_BW_40M = p_sta->htinfo.bCurTxBW40MHz;
				}
				rtllib_qurey_ShortPreambleMode(ieee, tcb_desc);
				rtllib_tx_query_agg_cap(ieee, txb->fragments[0], tcb_desc);
				rtllib_ibss_query_HTCapShortGI(ieee, tcb_desc,is_peer_shortGI_40M,is_peer_shortGI_20M); 
				rtllib_ibss_query_BandwidthMode(ieee, tcb_desc,is_peer_BW_40M);
				rtllib_query_protectionmode(ieee, tcb_desc, txb->fragments[0]);
				//CB_DESC_DUMP(tcb_desc, __FUNCTION__);
			}
			else {
				rtllib_txrate_selectmode(ieee, tcb_desc, 0); 
				tcb_desc->data_rate = CURRENT_RATE(ieee->mode, ieee->rate, ieee->HTCurrentOperaRate);
				if(bdhcp == true){
					// Use low rate to send DHCP packet.
					if(ieee->pHTInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom) {
						tcb_desc->data_rate = MGN_1M;//MgntQuery_TxRateExcludeCCKRates(ieee);//0xc;//ofdm 6m
						tcb_desc->bTxDisableRateFallBack = false;
					}else{
						tcb_desc->data_rate = MGN_1M;
						tcb_desc->bTxDisableRateFallBack = 1;
					}

					tcb_desc->RATRIndex = 7;
					tcb_desc->bTxUseDriverAssingedRate = 1;
					tcb_desc->bdhcp = 1;
				}
				rtllib_qurey_ShortPreambleMode(ieee, tcb_desc);
				rtllib_tx_query_agg_cap(ieee, txb->fragments[0], tcb_desc);
				rtllib_query_HTCapShortGI(ieee, tcb_desc); 
				rtllib_query_BandwidthMode(ieee, tcb_desc);
				rtllib_query_protectionmode(ieee, tcb_desc, txb->fragments[0]);
				
			}
		}
#else
		rtllib_txrate_selectmode(ieee, tcb_desc);
		if ( tcb_desc->bMulticast ||  tcb_desc->bBroadcast)
			tcb_desc->data_rate = ieee->basic_rate;
		else
			//tcb_desc->data_rate = CURRENT_RATE(ieee->current_network.mode, ieee->rate, ieee->HTCurrentOperaRate);
			tcb_desc->data_rate = CURRENT_RATE(ieee->mode, ieee->rate, ieee->HTCurrentOperaRate);

		if(bdhcp == true){
			// Use low rate to send DHCP packet.
			if(ieee->pHTInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)	
			{
				tcb_desc->data_rate = MgntQuery_TxRateExcludeCCKRates(ieee);//0xc;//ofdm 6m
				tcb_desc->bTxDisableRateFallBack = false;
			}else{
				tcb_desc->data_rate = MGN_1M;
                    tcb_desc->bTxDisableRateFallBack = 1;
			}

			//printk("DHCP TranslateHeader(), pTcb->DataRate = 0x%x\n", tcb_desc->data_rate);
			
                    	tcb_desc->RATRIndex = 7;
                    tcb_desc->bTxUseDriverAssingedRate = 1;
                    //tcb_desc->bTxEnableFwCalcDur = 1;
                }
		
		rtllib_qurey_ShortPreambleMode(ieee, tcb_desc);
		rtllib_tx_query_agg_cap(ieee, txb->fragments[0], tcb_desc);
		rtllib_query_HTCapShortGI(ieee, tcb_desc); 
		rtllib_query_BandwidthMode(ieee, tcb_desc);
		rtllib_query_protectionmode(ieee, tcb_desc, txb->fragments[0]);
#endif
                } 		
//		rtllib_query_seqnum(ieee, txb->fragments[0], header.addr1);
//		RTLLIB_DEBUG_DATA(RTLLIB_DL_DATA, txb->fragments[0]->data, txb->fragments[0]->len);
		//RTLLIB_DEBUG_DATA(RTLLIB_DL_DATA, tcb_desc, sizeof(cb_desc));
#endif
	}
	spin_unlock_irqrestore(&ieee->lock, flags);
	dev_kfree_skb_any(skb);
	if (txb) {
		if (ieee->softmac_features & IEEE_SOFTMAC_TX_QUEUE){
			rtllib_softmac_xmit(txb, ieee);
		}else{
			if ((*ieee->hard_start_xmit)(txb, dev) == 0) {
				stats->tx_packets++;
				stats->tx_bytes += txb->payload_size;
				return 0;
			}
			rtllib_txb_free(txb);
		}
	}

	return 0;

 failed:
	spin_unlock_irqrestore(&ieee->lock, flags);
	netif_stop_queue(dev);
	stats->tx_errors++;
	return 1;

}
int rtllib_xmit(struct sk_buff *skb, struct net_device *dev)
{
	memset(skb->cb, 0, sizeof(skb->cb));
	return rtllib_xmit_inter(skb, dev);
}


#ifndef BUILT_IN_RTLLIB
EXPORT_SYMBOL_RSL(rtllib_txb_free);
#ifdef ENABLE_AMSDU
EXPORT_SYMBOL_RSL(AMSDU_Aggregation);
EXPORT_SYMBOL_RSL(AMSDU_GetAggregatibleList);
#endif
#ifdef _RTL8192_EXT_PATCH_
EXPORT_SYMBOL_RSL(rtllib_query_seqnum);
EXPORT_SYMBOL_RSL(rtllib_alloc_txb);
EXPORT_SYMBOL_RSL(rtllib_encrypt_fragment);
#endif // _RTL8187_EXT_PATCH_
#endif
