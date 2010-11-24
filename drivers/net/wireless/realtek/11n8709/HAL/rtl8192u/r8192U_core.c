/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 * Linux device driver for RTL8192U 
 *
 * Based on the r8187 driver, which is:
 * Copyright 2004-2005 Andrea Merello <andreamrl@tiscali.it>, et al.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
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
 * Jerry chuang <wlanfae@realtek.com>
******************************************************************************/
#ifndef CONFIG_FORCE_HARD_FLOAT
double __floatsidf (int i) { return i; }
unsigned int __fixunsdfsi (double d) { return d; }
double __adddf3(double a, double b) { return a+b; }
double __addsf3(float a, float b) { return a+b; }
double __subdf3(double a, double b) { return a-b; }
double __extendsfdf2(float a) {return a;}
#endif

#undef LOOP_TEST
#undef DUMP_RX
#undef DUMP_TX
#undef DEBUG_TX_DESC2
#undef RX_DONT_PASS_UL
#undef DEBUG_EPROM
#undef DEBUG_RX_VERBOSE
#undef DUMMY_RX
#undef DEBUG_ZERO_RX
#undef DEBUG_RX_SKB
#undef DEBUG_TX_FRAG
#undef DEBUG_RX_FRAG
#undef DEBUG_TX_FILLDESC
#undef DEBUG_TX
#undef DEBUG_IRQ
#undef DEBUG_RX
#undef DEBUG_RXALLOC
#undef DEBUG_REGISTERS
#undef DEBUG_RING
#undef DEBUG_IRQ_TASKLET
#undef DEBUG_TX_ALLOC
#undef DEBUG_TX_DESC

#define CONFIG_RTL8192_IO_MAP

#ifdef RTL8192SU
#include <asm/uaccess.h>
#include "r8192U.h"
//#include "r8190_rtl8256.h" // RTL8225 Radio frontend */
#include "r8180_93cx6.h"   // Card EEPROM */
#include "r8192U_wx.h"

#include "r8192S_rtl8225.h" 
#include "r8192S_hw.h"
#include "r8192S_phy.h" 
#include "r8192S_phyreg.h"
#include "r8192S_Efuse.h"

#include "r819xU_cmdpkt.h"
#include "r8192U_dm.h"
//#include "r8192xU_phyreg.h"
#include <linux/usb.h>
// FIXME: check if 2.6.7 is ok
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,7))
#define usb_kill_urb usb_unlink_urb
#endif

#ifdef CONFIG_RTL8192_PM
#include "r8192U_pm.h"
#endif

#ifdef ENABLE_DOT11D
#include "dot11d.h"
#endif

#else

#include <asm/uaccess.h>
#include <linux/crc32.h>
#include "r8192U_hw.h"
#include "r8192U.h"
#include "r8190_rtl8256.h" // RTL8225 Radio frontend */
#include "r8180_93cx6.h"   // Card EEPROM */
#include "r8192U_wx.h"
#include "r819xU_phy.h" //added by WB 4.30.2008
#include "r819xU_phyreg.h"
#include "r819xU_cmdpkt.h"
#include "r8192U_dm.h"
//#include "r8192xU_phyreg.h"
#include <linux/usb.h>
// FIXME: check if 2.6.7 is ok
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,7))
#define usb_kill_urb usb_unlink_urb
#endif

#ifdef CONFIG_RTL8192_PM
#include "r8192U_pm.h"
#endif

#ifdef ENABLE_DOT11D
#include "dot11d.h"
#endif

#endif

#ifdef RTK_DMP_PLATFORM
#include <linux/usb_setting.h> // for USB_USE_ALIGNMENT
#endif

#ifdef RTL8192SU
u32 rt_global_debug_component = \
//				COMP_TRACE	|
//    				COMP_DBG	|
//				COMP_INIT    	|
//				COMP_RECV	|
//				COMP_SEND	|
//				COMP_IO		|
				COMP_POWER	|
//				COMP_EPROM   	|
				COMP_SWBW	|
				COMP_POWER_TRACKING |
				COMP_TURBO	|
				COMP_QOS	|
//				COMP_RATE	|
//				COMP_RM		|
//				COMP_DIG	|
//				COMP_EFUSE	|
//				COMP_CH		|
//				COMP_TXAGC	|
                              	COMP_HIPWR	|
//                             	COMP_HALDM	|
				COMP_SEC	|
//				COMP_LED	|
//				COMP_RF		|
//				COMP_RXDESC	|
				COMP_FIRMWARE	|
				COMP_HT		|
				COMP_AMSDU	|
				COMP_SCAN	|
//				COMP_CMD	|
				COMP_DOWN	|
				COMP_RESET	|
				//COMP_MLME	|
				COMP_ERR; //always open err flags on
#else
//set here to open your trace code. //WB
u32 rt_global_debug_component = \
			//	COMP_INIT    	|
//				COMP_DBG	|
			//	COMP_EPROM   	|
//				COMP_PHY	|
			//	COMP_RF		|
//				COMP_FIRMWARE	|
//				COMP_CH		|
			//	COMP_POWER_TRACKING |
//				COMP_RATE	|
			//	COMP_TXAGC	|
		//		COMP_TRACE	|
				COMP_DOWN	|
		//		COMP_RECV	|
                //              COMP_SWBW	|
				COMP_SEC	|
	//			COMP_RESET	|
		//		COMP_SEND	|
			//	COMP_EVENTS	|
				COMP_ERR ; //always open err flags on
#endif

#define TOTAL_CAM_ENTRY 32
#define CAM_CONTENT_COUNT 8

static struct usb_device_id rtl8192_usb_id_tbl[] = {
#ifdef RTL8192SU
	//92SU
	// Realtek */
	{USB_DEVICE(0x0bda, 0x8171)},
	{USB_DEVICE(0x0bda, 0x8172)},
	{USB_DEVICE(0x0bda, 0x8173)},
	{USB_DEVICE(0x0bda, 0x8174)},
	{USB_DEVICE(0x0bda, 0x8712)},
	{USB_DEVICE(0x0bda, 0x8713)},
	// Corega */
	{USB_DEVICE(0x07aa, 0x0047)},
	// Dlink */
	{USB_DEVICE(0x07d1, 0x3303)},
	{USB_DEVICE(0x07d1, 0x3302)},
	{USB_DEVICE(0x07d1, 0x3300)},
	// EnGenius */
	{USB_DEVICE(0x1740, 0x9603)},
	{USB_DEVICE(0x1740, 0x9605)},
	// Belkin */
	{USB_DEVICE(0x050d, 0x815F)},
	// Guillemot */
	{USB_DEVICE(0x06f8, 0xe031)},
	// Edimax */
	{USB_DEVICE(0x7392, 0x7612)},
	// Sitecom */	
	{USB_DEVICE(0x0DF6, 0x0045)},
	// Hawking */
	{USB_DEVICE(0x0E66, 0x0015)},
	{USB_DEVICE(0x0E66, 0x0016)},
#else	
	// Realtek */
	{USB_DEVICE(0x0bda, 0x8192)},
	{USB_DEVICE(0x0bda, 0x8709)},
	// Corega */
	{USB_DEVICE(0x07aa, 0x0043)},
	// Belkin */
	{USB_DEVICE(0x050d, 0x805E)},
	// Sitecom */
	{USB_DEVICE(0x0df6, 0x0031)},
	// EnGenius */
	{USB_DEVICE(0x1740, 0x9201)},
	// Dlink */
	{USB_DEVICE(0x2001, 0x3301)},
	// Zinwell */
	{USB_DEVICE(0x5a57, 0x0290)},
	// LG */
	{USB_DEVICE(0x043E, 0x7A01)},
#endif	
	{}
};

MODULE_LICENSE("GPL");
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
MODULE_VERSION("V 1.1");
#endif
MODULE_DEVICE_TABLE(usb, rtl8192_usb_id_tbl);
MODULE_DESCRIPTION("Linux driver for Realtek RTL8192 USB WiFi cards");

static char* ifname = "wlan%d";
#if 0
static int hwseqnum = 0;
static int hwwep = 0;
#endif
static int hwwep = 1;  //default use hw. set 0 to use software security
static int channels = 0x3fff;



#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)
module_param(ifname, charp, S_IRUGO|S_IWUSR );
//module_param(hwseqnum,int, S_IRUGO|S_IWUSR);
module_param(hwwep,int, S_IRUGO|S_IWUSR);
module_param(channels,int, S_IRUGO|S_IWUSR);
#else
MODULE_PARM(ifname, "s");
//MODULE_PARM(hwseqnum,"i");
MODULE_PARM(hwwep,"i");
MODULE_PARM(channels,"i");
#endif

MODULE_PARM_DESC(ifname," Net interface name, wlan%d=default");
//MODULE_PARM_DESC(hwseqnum," Try to use hardware 802.11 header sequence numbers. Zero=default");
MODULE_PARM_DESC(hwwep," Try to use hardware security support. ");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
static int __devinit rtl8192_usb_probe(struct usb_interface *intf,
			 const struct usb_device_id *id);
static void __devexit rtl8192_usb_disconnect(struct usb_interface *intf);
#else
static void *__devinit rtl8192_usb_probe(struct usb_device *udev,unsigned int ifnum,
			 const struct usb_device_id *id);
static void __devexit rtl8192_usb_disconnect(struct usb_device *udev, void *ptr);
#endif
		 

static struct usb_driver rtl8192_usb_driver = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 15)
	.owner		= THIS_MODULE,
#endif
	.name		= RTL819xU_MODULE_NAME,	          // Driver name   */
	.id_table	= rtl8192_usb_id_tbl,	          // PCI_ID table  */
	.probe		= rtl8192_usb_probe,	          // probe fn      */
	.disconnect	= rtl8192_usb_disconnect,	  // remove fn     */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 5, 0)
#ifdef CONFIG_RTL8192_PM
	.suspend	= rtl8192U_suspend,	          // PM suspend fn */
	.resume		= rtl8192U_resume,                // PM resume fn  */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
	.reset_resume   = rtl8192U_resume,                // PM reset resume fn  */
#endif
#else
	.suspend	= NULL,			          // PM suspend fn */
	.resume      	= NULL,			          // PM resume fn  */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
	.reset_resume   = NULL,                           // PM reset resume fn  */
#endif
#endif
#endif
};


#ifdef RTL8192SU
static void 	rtl8192SU_read_eeprom_info(struct net_device *dev);
short 	rtl8192SU_tx(struct net_device *dev, struct sk_buff* skb);
void 	rtl8192SU_rx_nomal(struct sk_buff* skb);
void 	rtl8192SU_rx_cmd(struct sk_buff *skb);
bool 	rtl8192SU_adapter_start(struct net_device *dev);
short	rtl8192SU_tx_cmd(struct net_device *dev, struct sk_buff *skb);
void 	rtl8192SU_link_change(struct net_device *dev);
void 	InitialGain8192S(struct net_device *dev,u8 Operation);
void 	rtl8192SU_query_rxdesc_status(struct sk_buff *skb, struct ieee80211_rx_stats *stats, bool bIsRxAggrSubframe);
void	rtl8192SU_HalUsbRxAggr8192SUsb(struct net_device *dev, bool Value);

struct rtl819x_ops rtl8192su_ops = {
	.nic_type = NIC_8192SU,
	.rtl819x_read_eeprom_info = rtl8192SU_read_eeprom_info,
	.rtl819x_tx = rtl8192SU_tx,
	.rtl819x_tx_cmd = rtl8192SU_tx_cmd,
	.rtl819x_rx_nomal = rtl8192SU_rx_nomal,
	.rtl819x_rx_cmd = rtl8192SU_rx_cmd,
	.rtl819x_adapter_start = rtl8192SU_adapter_start,
	.rtl819x_link_change = rtl8192SU_link_change,
	.rtl819x_initial_gain = InitialGain8192S,
	.rtl819x_query_rxdesc_status = rtl8192SU_query_rxdesc_status,
};
#else
static void 	rtl8192_read_eeprom_info(struct net_device *dev);
short	rtl8192_tx(struct net_device *dev, struct sk_buff* skb);
void 	rtl8192_rx_nomal(struct sk_buff* skb);
void 	rtl8192_rx_cmd(struct sk_buff *skb);
bool 	rtl8192_adapter_start(struct net_device *dev);
short	rtl819xU_tx_cmd(struct net_device *dev, struct sk_buff *skb);
void 	rtl8192_link_change(struct net_device *dev);
void 	InitialGain819xUsb(struct net_device *dev,u8 Operation);
void 	query_rxdesc_status(struct sk_buff *skb, struct ieee80211_rx_stats *stats, bool bIsRxAggrSubframe);

struct rtl819x_ops rtl8192u_ops = {
	.nic_type = NIC_8192U,
	.rtl819x_read_eeprom_info = rtl8192_read_eeprom_info,
	.rtl819x_tx = rtl8192_tx,
	.rtl819x_tx_cmd = rtl819xU_tx_cmd,
	.rtl819x_rx_nomal = rtl8192_rx_nomal,
	.rtl819x_rx_cmd = rtl8192_rx_cmd,
	.rtl819x_adapter_start = rtl8192_adapter_start,
	.rtl819x_link_change = rtl8192_link_change,
	.rtl819x_initial_gain = InitialGain819xUsb,
	.rtl819x_query_rxdesc_status = query_rxdesc_status,
};
#endif

#ifdef ENABLE_DOT11D

typedef struct _CHANNEL_LIST
{
	u8	Channel[32];
	u8	Len;
}CHANNEL_LIST, *PCHANNEL_LIST;

static CHANNEL_LIST ChannelPlan[] = {
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,149,153,157,161,165},24},  		//FCC
	{{1,2,3,4,5,6,7,8,9,10,11},11},                    				//IC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64},21},  	//ETSI
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},    //Spain. Change to ETSI. 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},  	//France. Change to ETSI.
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},	//MKK					//MKK
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},//MKK1
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},	//Israel.
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},			// For 11a , TELEC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64}, 21},    //MIC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14},14},					//For Global Domain. 1-11:active scan, 12-14 passive scan. //+YJ, 080626
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},					//WORLD_WIDE_13
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64}, 21},					//TELEC_NETGEAR
};

static void rtl819x_set_channel_map(u8 channel_plan, struct r8192_priv* priv)
{
	int i, max_chan=-1, min_chan=-1;
	struct ieee80211_device* ieee = priv->ieee80211;

	ieee->bGlobalDomain = false;
	//acturally 8225 & 8256 rf chip only support B,G,24N mode
	if ((priv->rf_chip == RF_8225) || (priv->rf_chip == RF_8256) || (priv->rf_chip == RF_6052))
	{
		min_chan = 1;
		max_chan = 14;
	}
	else
	{
		RT_TRACE(COMP_ERR, "unknown rf chip, can't set channel map in function:%s()\n", __FUNCTION__);
	}
	if (ChannelPlan[channel_plan].Len != 0){
		// Clear old channel map
		memset(GET_DOT11D_INFO(ieee)->channel_map, 0, sizeof(GET_DOT11D_INFO(ieee)->channel_map));
		// Set new channel map
		for (i=0;i<ChannelPlan[channel_plan].Len;i++) 
		{
			if (ChannelPlan[channel_plan].Channel[i] < min_chan || ChannelPlan[channel_plan].Channel[i] > max_chan)
				break; 	
			GET_DOT11D_INFO(ieee)->channel_map[ChannelPlan[channel_plan].Channel[i]] = 1;
		}
	}

	switch (channel_plan)
	{
		case COUNTRY_CODE_GLOBAL_DOMAIN:
			{
				//this flag enabled to follow 11d country IE setting, 
				//otherwise, it shall follow global domain setting
				ieee->bGlobalDomain = true;
				// 1~11 active scan
				// 12~14 passive scan
				for (i=12; i<=14; i++) {
					GET_DOT11D_INFO(ieee)->channel_map[i] = 2;
				}
				ieee->IbssStartChnl= 10;
				ieee->ibss_maxjoin_chal = 11;
				break;
			}

		case COUNTRY_CODE_WORLD_WIDE_13:
			{
				//this flag enabled to follow 11d country IE setting, 
				//otherwise, it shall follow global domain setting
				// 1~11 active scan
				// 12~13 passive scan
				printk("world wide 13\n");
				for (i=12; i<=13; i++) {
					GET_DOT11D_INFO(ieee)->channel_map[i] = 2;
				}
				ieee->IbssStartChnl= 10;
				ieee->ibss_maxjoin_chal = 11;
				break;
			}

		default:
			ieee->IbssStartChnl = 1;
			ieee->ibss_maxjoin_chal = 14;
			break;
	}
	return;
}
#endif

#define eqMacAddr(a,b) ( ((a)[0]==(b)[0] && (a)[1]==(b)[1] && (a)[2]==(b)[2] && (a)[3]==(b)[3] && (a)[4]==(b)[4] && (a)[5]==(b)[5]) ? 1:0 )

#ifdef RTL8192SU
#define		rx_hal_is_cck_rate(_pDesc)\
			((_pDesc->RxMCS  == DESC92S_RATE1M ||\
			_pDesc->RxMCS == DESC92S_RATE2M ||\
			_pDesc->RxMCS == DESC92S_RATE5_5M ||\
			_pDesc->RxMCS == DESC92S_RATE11M) &&\
			!_pDesc->RxHT)

#define 	tx_hal_is_cck_rate(_DataRate)\
			( _DataRate == MGN_1M ||\
			 _DataRate == MGN_2M ||\
			 _DataRate == MGN_5_5M ||\
			 _DataRate == MGN_11M ) 

#else
#define 	rx_hal_is_cck_rate(_pdrvinfo)\
			((_pdrvinfo->RxRate == DESC90_RATE1M ||\
			_pdrvinfo->RxRate == DESC90_RATE2M ||\
			_pdrvinfo->RxRate == DESC90_RATE5_5M ||\
			_pdrvinfo->RxRate == DESC90_RATE11M) &&\
			!_pdrvinfo->RxHT)
#endif

 

void CamResetAllEntry(struct net_device *dev)
{
#if 1
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32 ulcommand = 0;
        //2004/02/11  In static WEP, OID_ADD_KEY or OID_ADD_WEP are set before STA associate to AP.
        // However, ResetKey is called on OID_802_11_INFRASTRUCTURE_MODE and MlmeAssociateRequest
        // In this condition, Cam can not be reset because upper layer will not set this static key again.
        //if(Adapter->EncAlgorithm == WEP_Encryption)
        //      return;
//debug
        //DbgPrint("========================================\n");
        //DbgPrint("                            Call ResetAllEntry                                              \n");
        //DbgPrint("========================================\n\n");
	if(priv->bSurpriseRemoved)
		return;
	ulcommand |= BIT31|BIT30;
	write_nic_dword(dev, RWCAM, ulcommand); 
#else
        for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
                CAM_mark_invalid(dev, ucIndex);
        for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
                CAM_empty_entry(dev, ucIndex);
#endif

}


void write_cam(struct net_device *dev, u8 addr, u32 data)
{
        write_nic_dword(dev, WCAMI, data);
        write_nic_dword(dev, RWCAM, BIT31|BIT16|(addr&0xff) );
}

u32 read_cam(struct net_device *dev, u8 addr)
{
        write_nic_dword(dev, RWCAM, 0x80000000|(addr&0xff) );
        return read_nic_dword(dev, 0xa8);
}

void write_nic_byte_E(struct net_device *dev, int indx, u8 data)
{
	int status;	
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
			       indx|0xfe00, 0, &data, 1, HZ / 2);

	if (status < 0)
	{
		printk("write_nic_byte_E TimeOut! status:%d\n", status);
	}	
}

u8 read_nic_byte_E(struct net_device *dev, int indx)
{
	int status;
	u8 data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       RTL8187_REQ_GET_REGS, RTL8187_REQT_READ,
			       indx|0xfe00, 0, &data, 1, HZ / 2);

        if (status < 0)
        {
                printk("read_nic_byte_E TimeOut! status:%d\n", status);
        }

	return data;
}
//as 92U has extend page from 4 to 16, so modify functions below.
void write_nic_byte(struct net_device *dev, int indx, u8 data)
{
	int status;
	
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
#ifdef RTL8192SU
			       indx, 0, &data, 1, HZ / 2);
#else
			       (indx&0xff)|0xff00, (indx>>8)&0x0f, &data, 1, HZ / 2);
#endif

        if (status < 0)
        {
                printk("write_nic_byte TimeOut! status:%d\n", status);
        }


}


void write_nic_word(struct net_device *dev, int indx, u16 data)
{
	
	int status;
	
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
#ifdef RTL8192SU
			       indx, 0, &data, 2, HZ / 2);
#else
			       (indx&0xff)|0xff00, (indx>>8)&0x0f, &data, 2, HZ / 2);
#endif

        if (status < 0)
        {
                printk("write_nic_word TimeOut! status:%d\n", status);
        }

}


void write_nic_dword(struct net_device *dev, int indx, u32 data)
{

	int status;
	
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
#ifdef RTL8192SU
			       indx, 0, &data, 4, HZ / 2);
#else
			       (indx&0xff)|0xff00, (indx>>8)&0x0f, &data, 4, HZ / 2);
#endif


        if (status < 0)
        {
                printk("write_nic_dword TimeOut! status:%d\n", status);
        }

}
 
 
 
u8 read_nic_byte(struct net_device *dev, int indx)
{
	u8 data;
	int status;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       RTL8187_REQ_GET_REGS, RTL8187_REQT_READ,
#ifdef RTL8192SU
			       indx, 0, &data, 1, HZ / 2);
#else
			       (indx&0xff)|0xff00, (indx>>8)&0x0f, &data, 1, HZ / 2);
#endif
	
        if (status < 0)
        {
                printk("read_nic_byte TimeOut! status:%d\n", status);
        }

	return data;
}


 
u16 read_nic_word(struct net_device *dev, int indx)
{
	u16 data;
	int status;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       RTL8187_REQ_GET_REGS, RTL8187_REQT_READ,
#ifdef RTL8192SU
			       indx, 0, &data, 2, HZ / 2);
#else
			       (indx&0xff)|0xff00, (indx>>8)&0x0f, &data, 2, HZ / 2);
#endif
	
        if (status < 0)
        {
                printk("read_nic_word TimeOut! status:%d\n", status);
        }


	return data;
}

u16 read_nic_word_E(struct net_device *dev, int indx)
{
	u16 data;
	int status;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       RTL8187_REQ_GET_REGS, RTL8187_REQT_READ,
			       indx|0xfe00, 0, &data, 2, HZ / 2);
	
        if (status < 0)
        {
                printk("read_nic_word TimeOut! status:%d\n", status);
        }


	return data;
}

u32 read_nic_dword(struct net_device *dev, int indx)
{
	u32 data;
	int status;
//	int result;
	
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       RTL8187_REQ_GET_REGS, RTL8187_REQT_READ,
#ifdef RTL8192SU
			       indx, 0, &data, 4, HZ / 2);
#else
			       (indx&0xff)|0xff00, (indx>>8)&0x0f, &data, 4, HZ / 2);
#endif
//	if(0 != result) {
//	  printk(KERN_WARNING "read size of data = %d\, date = %d\n", result, data);
//	}
	
        if (status < 0)
        {
                printk("read_nic_dword TimeOut! status:%d\n", status);
		if(status == -ENODEV) {
			priv->usb_error = true;
		}
        }



	return data;
}


//u8 read_phy_cck(struct net_device *dev, u8 adr);
//u8 read_phy_ofdm(struct net_device *dev, u8 adr);
//this might still called in what was the PHY rtl8185/rtl8192 common code 
//plans are to possibilty turn it again in one common code...
// 
inline void force_pci_posting(struct net_device *dev)
{
}


static struct net_device_stats *rtl8192_stats(struct net_device *dev);
void rtl8192_commit(struct net_device *dev);
//void rtl8192_restart(struct net_device *dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_restart(struct work_struct *work);
//void rtl8192_rq_tx_ack(struct work_struct *work);
#else 
 void rtl8192_restart(struct net_device *dev);
// //void rtl8192_rq_tx_ack(struct net_device *dev);
 #endif

void watch_dog_timer_callback(unsigned long data);

/****************************************************************************
   -----------------------------PROCFS STUFF-------------------------
*****************************************************************************/

static struct proc_dir_entry *rtl8192_proc = NULL;



static int proc_get_stats_ap(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	struct ieee80211_network *target;
	
	int len = 0;

        list_for_each_entry(target, &ieee->network_list, list) {

		len += snprintf(page + len, count - len,
                "%s ", target->ssid);

		if(target->wpa_ie_len>0 || target->rsn_ie_len>0){
	                len += snprintf(page + len, count - len,
        	        "WPA\n");
		}
		else{
                        len += snprintf(page + len, count - len,
                        "non_WPA\n");
                }
		 
        }
	
	*eof = 1;
	return len;
}

#ifdef RTL8192SU
static int proc_get_registers_0(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n,page0,page1,page2;
			
	int max=0xff;
	page0 = 0x000;
	page1 = 0x100;
	page2 = 0x800;
	
	// This dump the current register page */
	if(!IS_BB_REG_OFFSET_92S(page0)){
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len,
					"\nD:  %2x > ",n);
			for(i=0;i<16 && n<=max;i++,n++)
				len += snprintf(page + len, count - len,
						"%2.2x ",read_nic_byte(dev,(page0|n)));
		}
	}else{
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
			for(i=0;i<4 && n<=max;n+=4,i++)
				len += snprintf(page + len, count - len,
						"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
		}
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
	
}

static int proc_get_registers_1(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x100;

	// This dump the current register page */
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len,
					"\nD:  %2x > ",n);
			for(i=0;i<16 && n<=max;i++,n++)
				len += snprintf(page + len, count - len,
						"%2.2x ",read_nic_byte(dev,(page0|n)));
		}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
static int proc_get_registers_2(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x200;

	// This dump the current register page */
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len,
					"\nD:  %2x > ",n);
			for(i=0;i<16 && n<=max;i++,n++)
				len += snprintf(page + len, count - len,
						"%2.2x ",read_nic_byte(dev,(page0|n)));
		}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
static int proc_get_registers_8(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x800;

	// This dump the current register page */
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
			for(i=0;i<4 && n<=max;n+=4,i++)
				len += snprintf(page + len, count - len,
						"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
		}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

	}
static int proc_get_registers_9(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x900;

	// This dump the current register page */
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
			for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
						"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
		}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_registers_a(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xa00;

	// This dump the current register page */
				len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
			for(i=0;i<4 && n<=max;n+=4,i++)
				len += snprintf(page + len, count - len,
						"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
		}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_registers_b(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xb00;

	// This dump the current register page */
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
			for(i=0;i<4 && n<=max;n+=4,i++)
				len += snprintf(page + len, count - len,
						"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
		}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
	}
static int proc_get_registers_c(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xc00;

	// This dump the current register page */
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
			for(i=0;i<4 && n<=max;n+=4,i++)
				len += snprintf(page + len, count - len,
						"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
		}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_registers_d(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xd00;

	// This dump the current register page */
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
			for(i=0;i<4 && n<=max;n+=4,i++)
				len += snprintf(page + len, count - len,
						"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
		}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_registers_e(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xe00;

	// This dump the current register page */
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
			for(i=0;i<4 && n<=max;n+=4,i++)
				len += snprintf(page + len, count - len,
						"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
		}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
#endif
#ifdef RTL8192U
static int proc_get_registers_0(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	int len = 0;
	int i,n,page0;
	int max=0xff;
	
	page0 = 0x000;
	// This dump the current register page */
	len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));

	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<16 && n<=max;i++,n++)
			len += snprintf(page + len, count - len, "%2x ",read_nic_byte(dev,page0|n));
	}

	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
static int proc_get_registers_1(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	int len = 0;
	int i,n,page0;
	int max=0xff;
	
	page0 = 0x100;
	// This dump the current register page */
	len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));

	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<16 && n<=max;i++,n++)
			len += snprintf(page + len, count - len, "%2x ",read_nic_byte(dev,page0|n));
	}

	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
static int proc_get_registers_2(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	int len = 0;
	int i,n,page0;
	int max=0xff;
	
	page0 = 0x200;
	// This dump the current register page */
	len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));

	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<16 && n<=max;i++,n++)
			len += snprintf(page + len, count - len, "%2x ",read_nic_byte(dev,page0|n));
	}

	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
static int proc_get_registers_3(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	int len = 0;
	int i,n,page0;
	int max=0xff;
	
	page0 = 0x300;
	// This dump the current register page */
	len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));

	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<16 && n<=max;i++,n++)
			len += snprintf(page + len, count - len, "%2x ",read_nic_byte(dev,page0|n));
	}

	len += snprintf(page + len, count - len,"\n");

	*eof = 1;
	return len;

}
static int proc_get_registers_4(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	int len = 0;
	int i,n,page0;
	int max=0xff;
	
	page0 = 0x400;
	// This dump the current register page */
	len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));

	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<16 && n<=max;i++,n++)
			len += snprintf(page + len, count - len, "%2x ",read_nic_byte(dev,page0|n));
	}

	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
#endif

static int proc_get_stats_tx(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"TX VI priority ok int: %lu\n"
		"TX VI priority error int: %lu\n"
		"TX VO priority ok int: %lu\n"
		"TX VO priority error int: %lu\n"
		"TX BE priority ok int: %lu\n"
		"TX BE priority error int: %lu\n"
		"TX BK priority ok int: %lu\n"
		"TX BK priority error int: %lu\n"
		"TX MANAGE priority ok int: %lu\n"
		"TX MANAGE priority error int: %lu\n"
		"TX BEACON priority ok int: %lu\n"
		"TX BEACON priority error int: %lu\n"
//		"TX high priority ok int: %lu\n"
//		"TX high priority failed error int: %lu\n"
		"TX queue resume: %lu\n"
		"TX queue stopped?: %d\n"
		"TX fifo overflow: %lu\n"
//		"TX beacon: %lu\n"
		"TX VI queue: %d\n"
		"TX VO queue: %d\n"
		"TX BE queue: %d\n"
		"TX BK queue: %d\n"
//		"TX HW queue: %d\n"
		"TX VI dropped: %lu\n"
		"TX VO dropped: %lu\n"
		"TX BE dropped: %lu\n"
		"TX BK dropped: %lu\n"
		"TX total data packets %lu\n",		
//		"TX beacon aborted: %lu\n",
		priv->stats.txviokint,
		priv->stats.txvierr,
		priv->stats.txvookint,
		priv->stats.txvoerr,
		priv->stats.txbeokint,
		priv->stats.txbeerr,
		priv->stats.txbkokint,
		priv->stats.txbkerr,
		priv->stats.txmanageokint,
		priv->stats.txmanageerr,
		priv->stats.txbeaconokint,
		priv->stats.txbeaconerr,
//		priv->stats.txhpokint,
//		priv->stats.txhperr,
		priv->stats.txresumed,
		netif_queue_stopped(dev),
		priv->stats.txoverflow,
//		priv->stats.txbeacon,
		atomic_read(&(priv->tx_pending[VI_PRIORITY])),
		atomic_read(&(priv->tx_pending[VO_PRIORITY])),
		atomic_read(&(priv->tx_pending[BE_PRIORITY])),
		atomic_read(&(priv->tx_pending[BK_PRIORITY])),
//		read_nic_byte(dev, TXFIFOCOUNT),
		priv->stats.txvidrop,
		priv->stats.txvodrop,
		priv->stats.txbedrop,
		priv->stats.txbkdrop,
		priv->stats.txdatapkt
//		priv->stats.txbeaconerr
		);
			
	*eof = 1;
	return len;
}		



static int proc_get_stats_rx(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"RX packets: %lu\n"
		"RX urb status error: %lu\n"
		"RX invalid urb error: %lu\n",
		priv->stats.rxoktotal,
		priv->stats.rxstaterr,
		priv->stats.rxurberr);
			
	*eof = 1;
	return len;
}		
#if 0
#if WIRELESS_EXT >= 12 && WIRELESS_EXT < 17

static struct iw_statistics *r8192_get_wireless_stats(struct net_device *dev)
{
       struct r8192_priv *priv = ieee80211_priv(dev);

       return &priv->wstats;
}
#endif
#endif
void rtl8192_proc_module_init(void)
{	
	RT_TRACE(COMP_INIT, "Initializing proc filesystem");
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
	rtl8192_proc=create_proc_entry(RTL819xU_MODULE_NAME, S_IFDIR, proc_net);
#else
	rtl8192_proc=create_proc_entry(RTL819xU_MODULE_NAME, S_IFDIR, init_net.proc_net);
#endif
}


void rtl8192_proc_module_remove(void)
{
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
	remove_proc_entry(RTL819xU_MODULE_NAME, proc_net);
#else
	remove_proc_entry(RTL819xU_MODULE_NAME, init_net.proc_net);
#endif
}


void rtl8192_proc_remove_one(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);


	if (priv->dir_dev) {
		remove_proc_entry("stats-tx", priv->dir_dev);
		remove_proc_entry("stats-rx", priv->dir_dev);
	//	remove_proc_entry("stats-ieee", priv->dir_dev);
		remove_proc_entry("stats-ap", priv->dir_dev);
		remove_proc_entry("registers-0", priv->dir_dev);
		remove_proc_entry("registers-1", priv->dir_dev);
		remove_proc_entry("registers-2", priv->dir_dev);
#ifdef RTL8192U
		remove_proc_entry("registers-3", priv->dir_dev);
		remove_proc_entry("registers-4", priv->dir_dev);
#endif
#ifdef RTL8192SU
		remove_proc_entry("registers-8", priv->dir_dev);
		remove_proc_entry("registers-9", priv->dir_dev);
		remove_proc_entry("registers-a", priv->dir_dev);
		remove_proc_entry("registers-b", priv->dir_dev);
		remove_proc_entry("registers-c", priv->dir_dev);
		remove_proc_entry("registers-d", priv->dir_dev);
		remove_proc_entry("registers-e", priv->dir_dev);
#endif		
		remove_proc_entry(dev->name, rtl8192_proc);
		priv->dir_dev = NULL;
	}
}


void rtl8192_proc_init_one(struct net_device *dev)
{
	struct proc_dir_entry *e;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dir_dev = create_proc_entry(dev->name, 
					  S_IFDIR | S_IRUGO | S_IXUGO, 
					  rtl8192_proc);
	if (!priv->dir_dev) {
		RT_TRACE(COMP_ERR, "Unable to initialize /proc/net/rtl8192/%s\n",
		      dev->name);
		return;
	}
	e = create_proc_read_entry("stats-rx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_rx, dev);
				   
	if (!e) {
		RT_TRACE(COMP_ERR,"Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-rx\n",
		      dev->name);
	}
	
	
	e = create_proc_read_entry("stats-tx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_tx, dev);
				   
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-tx\n",
		      dev->name);
	}
	#if 0
	e = create_proc_read_entry("stats-ieee", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_ieee, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-ieee\n",
		      dev->name);
	}
	
	#endif
	
	e = create_proc_read_entry("stats-ap", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_ap, dev);
				   
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-ap\n",
		      dev->name);
	}
	
	e = create_proc_read_entry("registers-0", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_0, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-0\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-1", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_1, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-1\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-2", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_2, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-2\n",
		      dev->name);
	}
#ifdef RTL8192U
	e = create_proc_read_entry("registers-3", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_3, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-3\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-4", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_4, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-4\n",
		      dev->name);
	}
#endif
#ifdef RTL8192SU
	e = create_proc_read_entry("registers-8", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_8, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-8\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-9", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_9, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-9\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-a", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_a, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-a\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-b", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_b, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-b\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-c", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_c, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-c\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-d", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_d, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-d\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-e", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_e, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-e\n",
		      dev->name);
	}
#endif
}
/****************************************************************************
   -----------------------------MISC STUFF-------------------------
*****************************************************************************/

// this is only for debugging */
void print_buffer(u32 *buffer, int len)
{
	int i;
	u8 *buf =(u8*)buffer;
	
	printk("ASCII BUFFER DUMP (len: %x):\n",len);
	
	for(i=0;i<len;i++)
		printk("%c",buf[i]);
		
	printk("\nBINARY BUFFER DUMP (len: %x):\n",len);
	
	for(i=0;i<len;i++)
		printk("%x",buf[i]);

	printk("\n");
}

//short check_nic_enough_desc(struct net_device *dev, priority_t priority)
short check_nic_enough_desc(struct net_device *dev,int queue_index)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int used = atomic_read(&priv->tx_pending[queue_index]); 
	
	return (used < MAX_TX_URB);
}

void tx_timeout(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//rtl8192_commit(dev);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	schedule_work(&priv->reset_wq);
#else
	schedule_task(&priv->reset_wq);
#endif
	//DMESG("TXTIMEOUT");
}


// this is only for debug */
void dump_eprom(struct net_device *dev)
{
	int i;
	for(i=0; i<63; i++)
		RT_TRACE(COMP_EPROM, "EEPROM addr %x : %x", i, eprom_read(dev,i));
}

// this is only for debug */
void rtl8192_dump_reg(struct net_device *dev)
{
	int i;
	int n;
	int max=0x1ff;
	
	RT_TRACE(COMP_PHY, "Dumping NIC register map");	
	
	for(n=0;n<=max;)
	{
		printk( "\nD: %2x> ", n);
		for(i=0;i<16 && n<=max;i++,n++)
			printk("%2x ",read_nic_byte(dev,n));
	}
	printk("\n");
}

/****************************************************************************
      ------------------------------HW STUFF---------------------------
*****************************************************************************/

#if 0
void rtl8192_irq_enable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);	
	//priv->irq_enabled = 1;

	//write_nic_word(dev,INTA_MASK,INTA_RXOK | INTA_RXDESCERR | INTA_RXOVERFLOW | 
	//INTA_TXOVERFLOW | INTA_HIPRIORITYDESCERR | INTA_HIPRIORITYDESCOK | 
	//INTA_NORMPRIORITYDESCERR | INTA_NORMPRIORITYDESCOK |
	//INTA_LOWPRIORITYDESCERR | INTA_LOWPRIORITYDESCOK | INTA_TIMEOUT);

	write_nic_word(dev,INTA_MASK, priv->irq_mask);
}


void rtl8192_irq_disable(struct net_device *dev)
{
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);	

	write_nic_word(dev,INTA_MASK,0);
	force_pci_posting(dev);
//	priv->irq_enabled = 0;
}
#endif

void rtl8192_set_mode(struct net_device *dev,int mode)
{
	u8 ecmd;
	ecmd=read_nic_byte(dev, EPROM_CMD);
	ecmd=ecmd &~ EPROM_CMD_OPERATING_MODE_MASK;
	ecmd=ecmd | (mode<<EPROM_CMD_OPERATING_MODE_SHIFT);
	ecmd=ecmd &~ (1<<EPROM_CS_SHIFT);
	ecmd=ecmd &~ (1<<EPROM_CK_SHIFT);
	write_nic_byte(dev, EPROM_CMD, ecmd);
}


void rtl8192_update_msr(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
	u8 msr;
	
	msr  = read_nic_byte(dev, MSR);
	msr &= ~ MSR_LINK_MASK;
	
	// do not change in link_state != WLAN_LINK_ASSOCIATED.
	// msr must be updated if the state is ASSOCIATING. 
	// this is intentional and make sense for ad-hoc and
	// master (see the create BSS/IBSS func)
	// 
	if (priv->ieee80211->state == IEEE80211_LINKED){ 
			
		if (priv->ieee80211->iw_mode == IW_MODE_INFRA){
			msr |= (MSR_LINK_MANAGED<<MSR_LINK_SHIFT);
			LedAction = LED_CTL_LINK;
		}
		else if (priv->ieee80211->iw_mode == IW_MODE_ADHOC)
			msr |= (MSR_LINK_ADHOC<<MSR_LINK_SHIFT);
		else if (priv->ieee80211->iw_mode == IW_MODE_MASTER)
			msr |= (MSR_LINK_MASTER<<MSR_LINK_SHIFT);
		
	}else
		msr |= (MSR_LINK_NONE<<MSR_LINK_SHIFT);
		
	write_nic_byte(dev, MSR, msr);
	
	if(priv->ieee80211->LedControlHandler != NULL)
		priv->ieee80211->LedControlHandler(dev, LedAction);
}

void rtl8192_set_chan(struct net_device *dev,short ch)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
//	u32 tx;
	RT_TRACE(COMP_CH, "=====>%s()====ch:%d\n", __FUNCTION__, ch);	
	//printk("=====>%s()====ch:%d\n", __FUNCTION__, ch);	
	priv->chan=ch;
	#if 0
	if(priv->ieee80211->iw_mode == IW_MODE_ADHOC || 
		priv->ieee80211->iw_mode == IW_MODE_MASTER){
	
			priv->ieee80211->link_state = WLAN_LINK_ASSOCIATED;	
			priv->ieee80211->master_chan = ch;
			rtl8192_update_beacon_ch(dev); 
		}
	#endif
	
	// this hack should avoid frame TX during channel setting*/


//	tx = read_nic_dword(dev,TX_CONF);
//	tx &= ~TX_LOOPBACK_MASK;

#ifndef LOOP_TEST	
//	write_nic_dword(dev,TX_CONF, tx |( TX_LOOPBACK_MAC<<TX_LOOPBACK_SHIFT));

	//need to implement rf set channel here WB

	if (priv->rf_set_chan)
	priv->rf_set_chan(dev,priv->chan);
	mdelay(10);
//	write_nic_dword(dev,TX_CONF,tx | (TX_LOOPBACK_NONE<<TX_LOOPBACK_SHIFT));
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static void rtl8192_rx_isr(struct urb *urb, struct pt_regs *regs);
#else
static void rtl8192_rx_isr(struct urb *urb);
#endif
//static void rtl8192_rx_isr(struct urb *rx_urb);

u32 get_rxpacket_shiftbytes_819xusb(struct ieee80211_rx_stats *pstats)
{

#ifdef USB_RX_AGGREGATION_SUPPORT
	if (pstats->bisrxaggrsubframe)
		return (sizeof(rx_desc_819x_usb) + pstats->RxDrvInfoSize
			+ pstats->RxBufShift + 8);
	else
#endif	
		return (sizeof(rx_desc_819x_usb) + pstats->RxDrvInfoSize 
				+ pstats->RxBufShift);

}
static int rtl8192_rx_initiate(struct net_device*dev)
{
        struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
        struct urb *entry;
        struct sk_buff *skb;
        struct rtl8192_rx_info *info;
	int err;
#ifdef USB_USE_ALIGNMENT
	u32 Tmpaddr=0;
        int alignment=0;
#endif
	// nomal packet rx procedure */
        while (skb_queue_len(&priv->rx_queue) < MAX_RX_URB) {
#ifdef USB_USE_ALIGNMENT
                skb = __dev_alloc_skb(RX_URB_SIZE+USB_512B_ALIGNMENT_SIZE, GFP_KERNEL);
                if (!skb)
                        break;
                Tmpaddr = (u32)skb->data;
                alignment = Tmpaddr & 0x1ff;
                skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#else
                skb = __dev_alloc_skb(RX_URB_SIZE, GFP_KERNEL);
                if (!skb)
                        break;
#endif
                skb->dev = dev;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) 
	        entry = usb_alloc_urb(0, GFP_KERNEL);
#else
	        entry = usb_alloc_urb(0);
#endif
                if (!entry) {
                        kfree_skb(skb);
                        break;
                }
//		printk("nomal packet IN request!\n");
                usb_fill_bulk_urb(entry, priv->udev,
                                  usb_rcvbulkpipe(priv->udev, 3), skb->tail,
                                  RX_URB_SIZE, rtl8192_rx_isr, skb);
                info = (struct rtl8192_rx_info *) skb->cb;
                info->urb = entry;
                info->dev = dev;
		info->out_pipe = 3; //denote rx normal packet queue
                skb_queue_tail(&priv->rx_queue, skb);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		err = usb_submit_urb(entry, GFP_KERNEL);
#else
		err = usb_submit_urb(entry);
#endif
		if(err && err != EPERM)
			printk("can not submit rxurb, err is %x,URB status is %x\n",err,entry->status);
		else {
			priv->IrpPendingCount++;
		}
        }
#ifndef RTL8192SU
	// command packet rx procedure */
        while (skb_queue_len(&priv->rx_queue) < MAX_RX_URB + 3) {
//		printk("command packet IN request!\n");
#ifdef USB_USE_ALIGNMENT
                skb = __dev_alloc_skb(RX_URB_SIZE+USB_512B_ALIGNMENT_SIZE ,GFP_KERNEL);
                if (!skb)
                        break;
                Tmpaddr = (u32)skb->data;
                alignment = Tmpaddr & 0x1ff;
                skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#else
                skb = __dev_alloc_skb(RX_URB_SIZE ,GFP_KERNEL);
                if (!skb)
                        break;
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
                entry = usb_alloc_urb(0, GFP_KERNEL);
#else
                entry = usb_alloc_urb(0);
#endif
                if (!entry) {
                        kfree_skb(skb);
                        break;
                }
                usb_fill_bulk_urb(entry, priv->udev,
                                  usb_rcvbulkpipe(priv->udev, 9), skb->tail,
                                  RX_URB_SIZE, rtl8192_rx_isr, skb);
                info = (struct rtl8192_rx_info *) skb->cb;
                info->urb = entry;
                info->dev = dev;
		   info->out_pipe = 9; //denote rx cmd packet queue
                skb_queue_tail(&priv->rx_queue, skb);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		usb_submit_urb(entry, GFP_KERNEL);
#else
		usb_submit_urb(entry);
#endif
        }
#endif
        return 0;
}

#if 0
void rtl8192_set_rxconf(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u32 rxconf;
	
	rxconf=read_nic_dword(dev,RCR);
	rxconf = rxconf &~ MAC_FILTER_MASK;
	
	rxconf = rxconf | RCR_AMF;   //accept management
	rxconf = rxconf | RCR_ADF;   //accept data
	rxconf = rxconf | RCR_ACF;   //accept control frame for SW AP needs PS-poll
	rxconf = rxconf | RCR_AB;    //accept broadcast 
	rxconf = rxconf | RCR_AM;    //accept multicast

	if (dev->flags & IFF_PROMISC) {DMESG ("NIC in promisc mode");}
	
	if(priv->ieee80211->iw_mode == IW_MODE_MONITOR || \
	   dev->flags & IFF_PROMISC){
		rxconf = rxconf | RCR_AAP;
	} /*else if(priv->ieee80211->iw_mode == IW_MODE_MASTER){
		rxconf = rxconf | (1<<ACCEPT_ALLMAC_FRAME_SHIFT);
		rxconf = rxconf | (1<<RX_CHECK_BSSID_SHIFT);
	}*/else if (priv->ieee80211->iw_mode == IW_MODE_INFRA || priv->ieee80211->iw_mode == IW_MODE_ADHOC){
		rxconf = rxconf | RCR_APM;  //accept unicast
		if (priv->ieee80211->state == IEEE80211_LINKED)
			rxconf = rxconf | RCR_CBSSID;
	}
	
	if(priv->ieee80211->iw_mode == IW_MODE_MONITOR){
		rxconf = rxconf | RCR_AICV;
		rxconf = rxconf | RCR_APWRMGT;
		if(priv->crcmon == 1)
			rxconf = rxconf | RCR_ACRC32;
	}
	
	rxconf = rxconf &~ RX_FIFO_THRESHOLD_MASK;
	rxconf = rxconf | (priv->EarlyRxThreshold<<RX_FIFO_THRESHOLD_SHIFT); // Rx FIFO Threshold, 7: No Rx threshold.
	rxconf = rxconf &~ MAX_RX_DMA_MASK;
	rxconf = rxconf | ((u32)7<<RCR_MXDMA_OFFSET);    // Max DMA Burst Size per Rx DMA Burst, 7: unlimited.
	rxconf = rxconf | (priv->EarlyRxThreshold == 7 ? RCR_ONLYERLPKT:0);  
	
//	rxconf = rxconf | (1<<RX_AUTORESETPHY_SHIFT);
//	rxconf = rxconf &~ RCR_CS_MASK;
//	rxconf = rxconf | (1<<RCR_CS_SHIFT);

	write_nic_dword(dev, RCR, rxconf);	
	
	#ifdef DEBUG_RX
	DMESG("rxconf: %x %x",rxconf ,read_nic_dword(dev,RCR));
	#endif
}
#endif

//wait to be removed
void rtl8192_rx_enable(struct net_device *dev)
{
	//u8 cmd;

	//struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	rtl8192_rx_initiate(dev);	
	
//	rtl8192_set_rxconf(dev);	
#if 0
	if(NIC_8187 == priv->card_8187) {
		cmd=read_nic_byte(dev,CMD);
		write_nic_byte(dev,CMD,cmd | (1<<CMD_RX_ENABLE_SHIFT));
	}
	else {
		//write_nic_dword(dev, RX_CONF, priv->ReceiveConfig);
	}
#endif
}


void rtl8192_tx_enable(struct net_device *dev)
{
#if 0
	u8 cmd;
	u8 byte;
	u32 txconf;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	//test loopback
	//	priv->TransmitConfig |= (TX_LOOPBACK_BASEBAND<<TX_LOOPBACK_SHIFT);
	if(NIC_8187B == priv->card_8187){
		write_nic_dword(dev, TX_CONF, priv->TransmitConfig);	
		byte = read_nic_byte(dev, MSR);
		byte |= MSR_LINK_ENEDCA;
		write_nic_byte(dev, MSR, byte);
	} else {
		byte = read_nic_byte(dev,CW_CONF);
		byte &= ~(1<<CW_CONF_PERPACKET_CW_SHIFT);
		byte &= ~(1<<CW_CONF_PERPACKET_RETRY_SHIFT);
		write_nic_byte(dev, CW_CONF, byte);

		byte = read_nic_byte(dev, TX_AGC_CTL);
		byte &= ~(1<<TX_AGC_CTL_PERPACKET_GAIN_SHIFT);
		byte &= ~(1<<TX_AGC_CTL_PERPACKET_ANTSEL_SHIFT);
		byte &= ~(1<<TX_AGC_CTL_FEEDBACK_ANT);
		write_nic_byte(dev, TX_AGC_CTL, byte);

		txconf= read_nic_dword(dev,TX_CONF);


		txconf = txconf &~ TX_LOOPBACK_MASK;

#ifndef LOOP_TEST
		txconf = txconf | (TX_LOOPBACK_NONE<<TX_LOOPBACK_SHIFT);
#else
		txconf = txconf | (TX_LOOPBACK_BASEBAND<<TX_LOOPBACK_SHIFT);
#endif
		txconf = txconf &~ TCR_SRL_MASK;
		txconf = txconf &~ TCR_LRL_MASK;

		txconf = txconf | (priv->retry_data<<TX_LRLRETRY_SHIFT); // long
		txconf = txconf | (priv->retry_rts<<TX_SRLRETRY_SHIFT); // short

		txconf = txconf &~ (1<<TX_NOCRC_SHIFT);

		txconf = txconf &~ TCR_MXDMA_MASK;
		txconf = txconf | (TCR_MXDMA_2048<<TCR_MXDMA_SHIFT);

		txconf = txconf | TCR_DISReqQsize;
		txconf = txconf | TCR_DISCW;
		txconf = txconf &~ TCR_SWPLCPLEN;

		txconf=txconf | (1<<TX_NOICV_SHIFT);

		write_nic_dword(dev,TX_CONF,txconf);

#ifdef DEBUG_TX
		DMESG("txconf: %x %x",txconf,read_nic_dword(dev,TX_CONF));
#endif

		cmd=read_nic_byte(dev,CMD);
		write_nic_byte(dev,CMD,cmd | (1<<CMD_TX_ENABLE_SHIFT));		
	}
#endif
}

#if 0
void rtl8192_beacon_tx_enable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask &=~(1<<TX_DMA_STOP_BEACON_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);	
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
}


void rtl8192_
_disable(struct net_device *dev) 
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_BEACON_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
}

#endif


void rtl8192_rtx_disable(struct net_device *dev)
{
	u8 cmd;
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct sk_buff *skb;
	struct rtl8192_rx_info *info;
	struct rtl8192_tx_info *tx_info;

	if(priv->bSurpriseRemoved == false){
		cmd=read_nic_byte(dev,CMDR);
		write_nic_byte(dev, CMDR, cmd &~ \
			(CR_TE|CR_RE));
	}
	force_pci_posting(dev);
	mdelay(10);

	while ((skb = __skb_dequeue(&priv->rx_queue))) {
		info = (struct rtl8192_rx_info *) skb->cb;
		if (info->urb){
			usb_kill_urb(info->urb);
		}
		dev_kfree_skb_any(skb);
	}

	if (skb_queue_len(&priv->rx_skb_queue)) {
		printk(KERN_WARNING "rx_skb_queue not empty\n");
	}

	skb_queue_purge(&priv->rx_skb_queue);
	
	while ((skb = skb_dequeue(&priv->tx_queue))) {
		tx_info = (struct rtl8192_tx_info *) skb->cb;
		if (tx_info->urb)
			usb_kill_urb(tx_info->urb);

		if (!tx_info->zero)
			usb_kill_urb(tx_info->zero);

		dev_kfree_skb_any(skb);
	}
	
	if (skb_queue_len(&priv->tx_skb_queue)) {
		printk(KERN_WARNING "tx_skb_queue not empty\n");
	}

	skb_queue_purge(&priv->tx_skb_queue);
	
	return;
}


int alloc_tx_beacon_desc_ring(struct net_device *dev, int count)
{
	#if 0
	int i;
	u32 *tmp;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	priv->txbeaconring = (u32*)pci_alloc_consistent(priv->pdev,
					  sizeof(u32)*8*count, 
					  &priv->txbeaconringdma);
	if (!priv->txbeaconring) return -1;
	for (tmp=priv->txbeaconring,i=0;i<count;i++){
		*tmp = *tmp &~ (1<<31); // descriptor empty, owned by the drv 
		
		//(tmp+2) = (u32)dma_tmp;
		//(tmp+3) = bufsize;
		
		if(i+1<count)
			*(tmp+4) = (u32)priv->txbeaconringdma+((i+1)*8*4);
		else
			*(tmp+4) = (u32)priv->txbeaconringdma;
		
		tmp=tmp+8;
	}
	#endif
	return 0;
}

#if 0
void rtl8192_reset(struct net_device *dev)
{
	
	//struct r8192_priv *priv = ieee80211_priv(dev);
	//u8 cr;


	// make sure the analog power is on before
	// reset, otherwise reset may fail
	// 
#if 0
	if(NIC_8187 == priv->card_8187) {
		rtl8192_set_anaparam(dev, RTL8225_ANAPARAM_ON);
		rtl8185_set_anaparam2(dev, RTL8225_ANAPARAM2_ON);
		rtl8192_irq_disable(dev);
		mdelay(200);
		write_nic_byte_E(dev,0x18,0x10);
		write_nic_byte_E(dev,0x18,0x11);
		write_nic_byte_E(dev,0x18,0x00);
		mdelay(200);
	}
#endif
	printk("=====>reset?\n");
#if 0
	cr=read_nic_byte(dev,CMD);
	cr = cr & 2;
	cr = cr | (1<<CMD_RST_SHIFT);
	write_nic_byte(dev,CMD,cr);
	
	force_pci_posting(dev);
	
	mdelay(200);
	
	if(read_nic_byte(dev,CMD) & (1<<CMD_RST_SHIFT)) 
		RT_TRACE(COMP_ERR, "Card reset timeout!\n");
	else 
		RT_TRACE(COMP_DOWN, "Card successfully reset\n");
#endif
#if 0	
	if(NIC_8187 == priv->card_8187) {

		printk("This is RTL8187 Reset procedure\n");
		rtl8192_set_mode(dev,EPROM_CMD_LOAD);
		force_pci_posting(dev);
		mdelay(200);

		// after the eeprom load cycle, make sure we have
		// correct anaparams
		// 
		rtl8192_set_anaparam(dev, RTL8225_ANAPARAM_ON);
		rtl8185_set_anaparam2(dev, RTL8225_ANAPARAM2_ON);
	}
	else
#endif
		printk("This is RTL8187B Reset procedure\n");

}
#endif
inline u16 ieeerate2rtlrate(int rate)
{
	switch(rate){
	case 10:	
	return 0;
	case 20:
	return 1;
	case 55:
	return 2;
	case 110:
	return 3;
	case 60:
	return 4;
	case 90:
	return 5;
	case 120:
	return 6;
	case 180:
	return 7;
	case 240:
	return 8;
	case 360:
	return 9;
	case 480:
	return 10;
	case 540:
	return 11;
	default:
	return 3;
	
	}
}
static u16 rtl_rate[] = {10,20,55,110,60,90,120,180,240,360,480,540};
inline u16 rtl8192_rate2rate(short rate)
{
	if (rate >11) return 0;
	return rtl_rate[rate]; 
}
		

// The protype of rx_isr has changed since one verion of Linux Kernel */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static void rtl8192_rx_isr(struct urb *urb, struct pt_regs *regs)
#else
static void rtl8192_rx_isr(struct urb *urb)
#endif
{
	struct sk_buff *skb = (struct sk_buff *) urb->context;
	struct sk_buff *skb2;
	struct rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev = info->dev;
	struct r8192_priv *priv = ieee80211_priv(dev);
	int out_pipe = info->out_pipe;
	int err;
	static u8 check_rx_err_cnt = 0;

	if(out_pipe == 3)
		priv->IrpPendingCount--;
#if 1
	if (unlikely(urb->status)) {
		switch(urb->status) {
			case -EINVAL:
			case -EPIPE:			
			case -ENODEV:
			case -ESHUTDOWN:
				priv->bSurpriseRemoved = true;
				printk("%s():rx status err %d\n",__FUNCTION__,urb->status);
			case -ENOENT:
				break;
			case -EPROTO:
				check_rx_err_cnt++;
				if(check_rx_err_cnt >= MAX_RX_URB)
					priv->bSurpriseRemoved = true;
				printk("%s():rx status err %d, it is EPROTO error !!!!! %d\n",__FUNCTION__,urb->status,check_rx_err_cnt);
				break;
			case -EINPROGRESS:
				printk("ERROR: URB IS IN PROGRESS!/n");
				break;
			default:
				break;
		}
		if((urb->status != -EPROTO) || (check_rx_err_cnt >= MAX_RX_URB)){
			info->urb = NULL;
			priv->stats.rxstaterr++;
			priv->ieee80211->stats.rx_errors++;
			usb_free_urb(urb);
			return;
		}
	}
	else
		check_rx_err_cnt = 0;
#else
        if (unlikely(urb->status)) {
		switch(urb->status) {
			case -EINVAL:
			case -EPIPE:			
			case -ENODEV:
			//case -EPROTO:
			case -ESHUTDOWN:
				priv->bSurpriseRemoved = true;
				printk("%s():rx status err %d\n",__FUNCTION__,urb->status);
			case -ENOENT:
				break;
			case -EINPROGRESS:
				printk("ERROR: URB IS IN PROGRESS!/n");
				break;
			default:
				break;
		}
                info->urb = NULL;
                priv->stats.rxstaterr++;
                priv->ieee80211->stats.rx_errors++;
                usb_free_urb(urb);

                return;
        }
#endif
	if(!priv->up){
		usb_free_urb(urb);
		return;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)
        skb_unlink(skb, &priv->rx_queue);
#else
	//  __skb_unlink before linux2.6.14 does not use spinlock to protect list head.
	//  add spinlock function manually. john,2008/12/03
	{
		unsigned long flags;
		spin_lock_irqsave(&(priv->rx_queue.lock), flags);
		__skb_unlink(skb,&priv->rx_queue);
		spin_unlock_irqrestore(&(priv->rx_queue.lock), flags);
	}
#endif


#ifdef USB_USE_ALIGNMENT
	u32 Tmpaddr=0;
        int alignment=0;
        skb2 = dev_alloc_skb(RX_URB_SIZE+USB_512B_ALIGNMENT_SIZE);
#else
        skb2 = dev_alloc_skb(RX_URB_SIZE);
#endif
        if (unlikely(!skb2)) {
		printk("%s():can,t alloc skb\n",__FUNCTION__);
        } else {
#if 1
		if(urb->status == -EPROTO)
		{
			printk("EPROTO error , drop receive packet !!!\n");
			dev_kfree_skb_any(skb);
		}
		else {
			skb_put(skb, urb->actual_length);

			skb_queue_tail(&priv->rx_skb_queue, skb);
			//tasklet_schedule(&priv->irq_rx_tasklet);
			tasklet_hi_schedule(&priv->irq_rx_tasklet);
		}
#else
	        skb_put(skb, urb->actual_length);
	
		skb_queue_tail(&priv->rx_skb_queue, skb);
		//tasklet_schedule(&priv->irq_rx_tasklet);
		tasklet_hi_schedule(&priv->irq_rx_tasklet);
#endif
		skb = skb2;

	        skb->dev = dev;
	
#ifdef USB_USE_ALIGNMENT
	        Tmpaddr = (u32)skb->data;
	        alignment = Tmpaddr & 0x1ff;
		skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif
        }
	
	usb_fill_bulk_urb(urb, priv->udev,
			usb_rcvbulkpipe(priv->udev, out_pipe), skb->tail,
			RX_URB_SIZE, rtl8192_rx_isr, skb);

        info = (struct rtl8192_rx_info *) skb->cb;
        info->urb = urb;
        info->dev = dev;
	info->out_pipe = out_pipe;

        urb->transfer_buffer = skb->tail;
        urb->context = skb;
        skb_queue_tail(&priv->rx_queue, skb);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
        err = usb_submit_urb(urb, GFP_ATOMIC);
#else
        err = usb_submit_urb(urb);
#endif
	if(err && err != EPERM)
		printk("can not submit rxurb, err is %x,URB status is %x\n",err,urb->status);	
	else {
		if(out_pipe == 3)
			priv->IrpPendingCount++;
	}

#if 1 // wakeup net_device stoppped by ieee80211_xmit() due to "Could not allocate TXB"! 
	if ( netif_queue_stopped(dev) ) {
		netif_wake_queue(dev);	
		printk("%s-%d: netif_wake_queue(dev)\n",__FUNCTION__, __LINE__);
	}	
#endif
}

u32	
rtl819xusb_rx_command_packet(
	struct net_device *dev,
	struct ieee80211_rx_stats *pstats
	)
{	
	u32	status;

	//RT_TRACE(COMP_RECV, DBG_TRACE, ("---> RxCommandPacketHandle819xUsb()\n"));

	status = cmpk_message_handle_rx(dev, pstats);
	if (status) 
	{
		DMESG("rxcommandpackethandle819xusb: It is a command packet\n");
	}
	else
	{
		//RT_TRACE(COMP_RECV, DBG_TRACE, ("RxCommandPacketHandle819xUsb: It is not a command packet\n"));
	}
	
	//RT_TRACE(COMP_RECV, DBG_TRACE, ("<--- RxCommandPacketHandle819xUsb()\n"));
	return status;
}

#if 0
void rtl8192_tx_queues_stop(struct net_device *dev)
{
	//struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u8 dma_poll_mask = (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_HIPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_NORMPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_BEACON_SHIFT);

	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
}
#endif

void rtl8192_data_hard_stop(struct net_device *dev)
{
	//FIXME !!
	#if 0
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}


void rtl8192_data_hard_resume(struct net_device *dev)
{
	// FIXME !!
	#if 0
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask &= ~(1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}

//  this function TX data frames when the ieee80211 stack requires this.
//  It checks also if we need to stop the ieee tx queue, eventually do it
// 
void rtl8192_hard_data_xmit(struct sk_buff *skb, struct net_device *dev, int rate)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	int ret;
	unsigned long flags;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	u8 queue_index = tcb_desc->queue_index;
	
	// shall not be referred by command packet */
	assert(queue_index != TXCMD_QUEUE);

	spin_lock_irqsave(&priv->tx_lock,flags);

	//memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
//	tcb_desc->RATRIndex = 7;
//	tcb_desc->bTxDisableRateFallBack = 1;
//	tcb_desc->bTxUseDriverAssingedRate = 1;
	tcb_desc->bTxEnableFwCalcDur = 1;
	skb_push(skb, priv->ieee80211->tx_headroom);
	ret = priv->ops->rtl819x_tx(dev, skb);

	//priv->ieee80211->stats.tx_bytes+=(skb->len - priv->ieee80211->tx_headroom);
	//priv->ieee80211->stats.tx_packets++;

	spin_unlock_irqrestore(&priv->tx_lock,flags);

//	return ret;
	return;
}

// This is a rough attempt to TX a frame
// This is called by the ieee 80211 stack to TX management frames.
// If the ring is full packet are dropped (for data frame the queue
// is stopped before this can happen).
// 
int rtl8192_hard_start_xmit(struct sk_buff *skb,struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	int ret;
	unsigned long flags;
        cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
        u8 queue_index = tcb_desc->queue_index;


	spin_lock_irqsave(&priv->tx_lock,flags);
	
	//memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
	if(queue_index == TXCMD_QUEUE) {
		skb_push(skb, USB_HWDESC_HEADER_LEN);
		priv->ops->rtl819x_tx_cmd(dev, skb);
		ret = 1;
	        spin_unlock_irqrestore(&priv->tx_lock,flags);	
		return ret;
	} else {
		skb_push(skb, priv->ieee80211->tx_headroom);
		ret = priv->ops->rtl819x_tx(dev, skb);
	}
	
	spin_unlock_irqrestore(&priv->tx_lock,flags);	
	
	return ret;
}


void rtl8192_try_wake_queue(struct net_device *dev, int pri);

#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
u16 DrvAggr_PaddingAdd(struct net_device *dev, struct sk_buff *skb)
{
	u16     PaddingNum =  256 - ((skb->len + TX_PACKET_DRVAGGR_SUBFRAME_SHIFT_BYTES) % 256);
	return  (PaddingNum&0xff);
}

u8 MRateToHwRate8190Pci(u8 rate);
u8 QueryIsShort(u8 TxHT, u8 TxRate, cb_desc *tcb_desc);
u8 MapHwQueueToFirmwareQueue(u8 QueueID);
struct sk_buff *DrvAggr_Aggregation(struct net_device *dev, struct ieee80211_drv_agg_txb *pSendList)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	struct ieee80211_device *ieee = netdev_priv(dev);
#else
	struct ieee80211_device *ieee = (struct ieee80211_device *)dev->priv;
#endif	
	struct r8192_priv *priv = ieee80211_priv(dev);
	cb_desc 	*tcb_desc = NULL;
	u8 		i;
	u32		TotalLength;
	struct sk_buff	*skb;
	struct sk_buff  *agg_skb;
	tx_desc_819x_usb_aggr_subframe *tx_agg_desc = NULL;	
	tx_fwinfo_819x_usb	       *tx_fwinfo = NULL;

	// 
	// Local variable initialization.
	//
	// first skb initialization */
	skb = pSendList->tx_agg_frames[0];
	TotalLength = skb->len;

	// Get the total aggregation length including the padding space and 
	// sub frame header.
	// 
	for(i = 1; i < pSendList->nr_drv_agg_frames; i++) {
		TotalLength += DrvAggr_PaddingAdd(dev, skb);
		skb = pSendList->tx_agg_frames[i];
		TotalLength += (skb->len + TX_PACKET_DRVAGGR_SUBFRAME_SHIFT_BYTES); 
	}

	// allocate skb to contain the aggregated packets */
#ifdef USB_USE_ALIGNMENT
	u32 Tmpaddr=0;
        int alignment=0;
	agg_skb = dev_alloc_skb(TotalLength + ieee->tx_headroom + 512);
        if (!agg_skb) {
        	printk("%s-%d: dev_alloc_skb() failed\n", __FUNCTION__, __LINE__);
        	return NULL;
        }
        Tmpaddr = (u32)agg_skb->data;
        alignment = Tmpaddr & 0x1ff;
        memset(agg_skb->data, 0, agg_skb->len);
	skb_reserve(agg_skb, ieee->tx_headroom + (USB_512B_ALIGNMENT_SIZE - alignment));
#else
	agg_skb = dev_alloc_skb(TotalLength + ieee->tx_headroom); 
        if (!agg_skb) {
        	printk("%s-%d: dev_alloc_skb() failed\n", __FUNCTION__, __LINE__);
        	return NULL;
        }
	memset(agg_skb->data, 0, agg_skb->len);
	skb_reserve(agg_skb, ieee->tx_headroom);
#endif

//	RT_DEBUG_DATA(COMP_SEND, skb->cb, sizeof(skb->cb));
	// reserve info for first subframe Tx descriptor to be set in the tx function */
	skb = pSendList->tx_agg_frames[0];
	tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	tcb_desc->drv_agg_enable = 1;
	tcb_desc->pkt_size = skb->len;
 	tcb_desc->DrvAggrNum = pSendList->nr_drv_agg_frames;
	//printk("DrvAggNum = %d\n", tcb_desc->DrvAggrNum);
//	RT_DEBUG_DATA(COMP_SEND, skb->cb, sizeof(skb->cb));
//	printk("========>skb->data ======> \n");
//	RT_DEBUG_DATA(COMP_SEND, skb->data, skb->len);
	memcpy(agg_skb->cb, skb->cb, sizeof(skb->cb));
	memcpy(skb_put(agg_skb,skb->len),skb->data,skb->len);

	for(i = 1; i < pSendList->nr_drv_agg_frames; i++) {
		// push the next sub frame to be 256 byte aline */
		skb_put(agg_skb,DrvAggr_PaddingAdd(dev,skb));

		// Subframe drv Tx descriptor and firmware info setting */
		skb = pSendList->tx_agg_frames[i];
		tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);	
		tx_agg_desc = (tx_desc_819x_usb_aggr_subframe *)agg_skb->tail;
		tx_fwinfo = (tx_fwinfo_819x_usb *)(agg_skb->tail + sizeof(tx_desc_819x_usb_aggr_subframe));

		memset(tx_fwinfo,0,sizeof(tx_fwinfo_819x_usb));
		// DWORD 0 */
		tx_fwinfo->TxHT = (tcb_desc->data_rate&0x80)?1:0;
		tx_fwinfo->TxRate = MRateToHwRate8190Pci(tcb_desc->data_rate);
		tx_fwinfo->EnableCPUDur = tcb_desc->bTxEnableFwCalcDur;     
		tx_fwinfo->Short = QueryIsShort(tx_fwinfo->TxHT, tx_fwinfo->TxRate, tcb_desc);
		if(tcb_desc->bAMPDUEnable) {//AMPDU enabled
			tx_fwinfo->AllowAggregation = 1;
			// DWORD 1 */
			tx_fwinfo->RxMF = tcb_desc->ampdu_factor;
			tx_fwinfo->RxAMD = tcb_desc->ampdu_density&0x07;//ampdudensity 
		} else {
			tx_fwinfo->AllowAggregation = 0;
			// DWORD 1 */
			tx_fwinfo->RxMF = 0;
			tx_fwinfo->RxAMD = 0;
		}

		// Protection mode related */
		tx_fwinfo->RtsEnable = (tcb_desc->bRTSEnable)?1:0;
		tx_fwinfo->CtsEnable = (tcb_desc->bCTSEnable)?1:0;
		tx_fwinfo->RtsSTBC = (tcb_desc->bRTSSTBC)?1:0;
		tx_fwinfo->RtsHT = (tcb_desc->rts_rate&0x80)?1:0;
		tx_fwinfo->RtsRate =  MRateToHwRate8190Pci((u8)tcb_desc->rts_rate);
		tx_fwinfo->RtsSubcarrier = (tx_fwinfo->RtsHT==0)?(tcb_desc->RTSSC):0;
		tx_fwinfo->RtsBandwidth = (tx_fwinfo->RtsHT==1)?((tcb_desc->bRTSBW)?1:0):0;
		tx_fwinfo->RtsShort = (tx_fwinfo->RtsHT==0)?(tcb_desc->bRTSUseShortPreamble?1:0):\
				      (tcb_desc->bRTSUseShortGI?1:0);

		// Set Bandwidth and sub-channel settings. */
		if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
		{
			if(tcb_desc->bPacketBW) {
				tx_fwinfo->TxBandwidth = 1;
				tx_fwinfo->TxSubCarrier = 0;    //By SD3's Jerry suggestion, use duplicated mode
			} else {
				tx_fwinfo->TxBandwidth = 0;
				tx_fwinfo->TxSubCarrier = priv->nCur40MhzPrimeSC;
			}
		} else {
			tx_fwinfo->TxBandwidth = 0;
			tx_fwinfo->TxSubCarrier = 0;
		}

		// Fill Tx descriptor */
		memset(tx_agg_desc, 0, sizeof(tx_desc_819x_usb_aggr_subframe));
		// DWORD 0 */
		//tx_agg_desc->LINIP = 0;
		//tx_agg_desc->CmdInit = 1; 
		tx_agg_desc->Offset =  sizeof(tx_fwinfo_819x_usb) + 8;
		// already raw data, need not to substract header length */
		tx_agg_desc->PktSize = skb->len & 0xffff;

		//DWORD 1*/
		tx_agg_desc->SecCAMID= 0;
		tx_agg_desc->RATid = tcb_desc->RATRIndex;
#if 0
		// Fill security related */
		if( pTcb->bEncrypt && !Adapter->MgntInfo.SecurityInfo.SWTxEncryptFlag)
		{
			EncAlg = SecGetEncryptionOverhead(
					Adapter,
					&EncryptionMPDUHeadOverhead,
					&EncryptionMPDUTailOverhead,
					NULL,
					NULL,
					FALSE,
					FALSE);
			//2004/07/22, kcwu, EncryptionMPDUHeadOverhead has been added in previous code
			//MPDUOverhead = EncryptionMPDUHeadOverhead + EncryptionMPDUTailOverhead;
			MPDUOverhead = EncryptionMPDUTailOverhead;
			tx_agg_desc->NoEnc = 0;
			RT_TRACE(COMP_SEC, DBG_LOUD, ("******We in the loop SecCAMID is %d SecDescAssign is %d The Sec is %d********\n",tx_agg_desc->SecCAMID,tx_agg_desc->SecDescAssign,EncAlg));
			//CamDumpAll(Adapter);
		}
		else
#endif
		{
			//MPDUOverhead = 0;
			tx_agg_desc->NoEnc = 1;
		}
#if 0
		switch(EncAlg){
			case NO_Encryption:
				tx_agg_desc->SecType = 0x0;
				break;
			case WEP40_Encryption:
			case WEP104_Encryption:
				tx_agg_desc->SecType = 0x1;
				break;
			case TKIP_Encryption:
				tx_agg_desc->SecType = 0x2;
				break;
			case AESCCMP_Encryption:
				tx_agg_desc->SecType = 0x3;
				break;
			default:
				tx_agg_desc->SecType = 0x0;
				break;
		}
#else
		tx_agg_desc->SecType = 0x0;
#endif

		if (tcb_desc->bHwSec) {
			switch (priv->ieee80211->pairwise_key_type)
			{
				case KEY_TYPE_WEP40:
				case KEY_TYPE_WEP104:
					tx_agg_desc->SecType = 0x1;
					tx_agg_desc->NoEnc = 0;
					break;
				case KEY_TYPE_TKIP:
					tx_agg_desc->SecType = 0x2;
					tx_agg_desc->NoEnc = 0;
					break;
				case KEY_TYPE_CCMP:
					tx_agg_desc->SecType = 0x3;
					tx_agg_desc->NoEnc = 0;
					break;
				case KEY_TYPE_NA:
					tx_agg_desc->SecType = 0x0;
					tx_agg_desc->NoEnc = 1;
					break;
			}
		}

		tx_agg_desc->QueueSelect = MapHwQueueToFirmwareQueue(tcb_desc->queue_index);
		tx_agg_desc->TxFWInfoSize =  sizeof(tx_fwinfo_819x_usb);

		tx_agg_desc->DISFB = tcb_desc->bTxDisableRateFallBack;
		tx_agg_desc->USERATE = tcb_desc->bTxUseDriverAssingedRate;

		tx_agg_desc->OWN = 1;

		//DWORD 2
		// According windows driver, it seems that there no need to fill this field */
		//tx_agg_desc->TxBufferSize= (u32)(skb->len - USB_HWDESC_HEADER_LEN);

		// to fill next packet */
		skb_put(agg_skb,TX_PACKET_DRVAGGR_SUBFRAME_SHIFT_BYTES);
		memcpy(skb_put(agg_skb,skb->len),skb->data,skb->len);
	}

	for(i = 0; i < pSendList->nr_drv_agg_frames; i++) {
		dev_kfree_skb_any(pSendList->tx_agg_frames[i]);
	}

	return agg_skb;
}

// NOTE:
//	This function return a list of PTCB which is proper to be aggregate with the input TCB. 
//	If no proper TCB is found to do aggregation, SendList will only contain the input TCB.
//
u8 DrvAggr_GetAggregatibleList(struct net_device *dev, struct sk_buff *skb, 
		struct ieee80211_drv_agg_txb *pSendList)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	struct ieee80211_device *ieee = netdev_priv(dev);
#else
	struct ieee80211_device *ieee = (struct ieee80211_device *)dev->priv;
#endif	
	PRT_HIGH_THROUGHPUT	pHTInfo = ieee->pHTInfo;
	u16		nMaxAggrNum = pHTInfo->UsbTxAggrNum;
	cb_desc 	*tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	u8		QueueID = tcb_desc->queue_index;

	do {
		pSendList->tx_agg_frames[pSendList->nr_drv_agg_frames++] = skb;
		if(pSendList->nr_drv_agg_frames >= nMaxAggrNum) {
			break;
		}

	} while((skb = skb_dequeue(&ieee->skb_drv_aggQ[QueueID])));

	RT_TRACE(COMP_AMSDU, "DrvAggr_GetAggregatibleList, nAggrTcbNum = %d \n", pSendList->nr_drv_agg_frames);
	return pSendList->nr_drv_agg_frames;
}
#endif

#ifdef WIFI_TEST
u8 wmm_queue_select(struct r8192_priv *priv, u8 queue_index, struct sk_buff_head queue[])
{
	
	int i, j, tmp, inx[4], acirp_cnt[4], orig_queue_index = queue_index;
	
	// 0->bk, 1->be, 2->vi, 3->vo.
	inx[0] = BK_QUEUE; acirp_cnt[0] = atomic_read(&priv->tx_pending[BK_QUEUE]);
	inx[1] = BE_QUEUE; acirp_cnt[1] = atomic_read(&priv->tx_pending[BE_QUEUE]);
	inx[2] = VI_QUEUE; acirp_cnt[2] = atomic_read(&priv->tx_pending[VI_QUEUE]);
	inx[3] = VO_QUEUE; acirp_cnt[3] = atomic_read(&priv->tx_pending[VO_QUEUE]);

	// After sorting, inx[0] will be the AC_QUEUE with least tx_pending URBs.
	for ( i=0; i<4; i++)
	{
		for ( j=i+1; j<4; j++)
		{
			if( acirp_cnt[j] <= acirp_cnt[i] )
			{
				tmp = acirp_cnt[i];
				acirp_cnt[i] = acirp_cnt[j];
				acirp_cnt[j] = tmp;
				
				tmp = inx[i];
				inx[i] = inx[j];
				inx[j] = tmp;
			}
		}
	}

#if 0 // debug
	for (i = 0; i < 3; i++) {
		printk("%d-%d  < ", inx[i], acirp_cnt[i]);
	}
	printk("%d-%d \n", inx[3], acirp_cnt[3]);
#endif
	
	for (i = 0; i < 4; i++) 
	{
		if ( (skb_queue_len(&queue[inx[i]]) > 0) ) {
			queue_index = inx[i];
#if 0 // debug			
			printk(" ==> %d\n", queue_index);
#endif			
			break;			
		}
	}
#if 0 // debug
	if (queue_index != orig_queue_index) {
		//printk("\nselect index (%d => %d)\n\n", orig_queue_index, queue_index);
		printk("q(%d=>%d)\n", orig_queue_index, queue_index);
	}	
#endif		
	return queue_index;
}
#endif


void handle_tx_skb(struct sk_buff *skb);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static void rtl8192_tx_isr(struct urb *tx_urb, struct pt_regs *reg)
#else
static void rtl8192_tx_isr(struct urb *tx_urb)
#endif
{
	struct sk_buff *skb = (struct sk_buff*)tx_urb->context;
	struct net_device *dev = skb->dev;
	struct r8192_priv *priv = ieee80211_priv(dev);
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);

	usb_free_urb(tx_urb);
#ifdef ENABLE_TX_ISR_TASKLET
	// move "atomic_dec(&priv->tx_pending[tcb_desc->queue_index])" to tasklet to reserve this tx_urb
#else
	atomic_dec(&priv->tx_pending[tcb_desc->queue_index]);
#endif

	if(tcb_desc->queue_index != TXCMD_QUEUE) {
		if(tx_urb->status == 0) {
		//	dev->trans_start = jiffies;
			// As act as station mode, destion shall be  unicast address.
			priv->ieee80211->stats.tx_bytes+=(skb->len - priv->ieee80211->tx_headroom);
			priv->ieee80211->stats.tx_packets++;
			priv->stats.txoktotal++;
			priv->ieee80211->LinkDetectInfo.NumTxOkInPeriod++;
			//lzm test
			//priv->stats.txbytesunicast += (skb->len - priv->ieee80211->tx_headroom);
		} else {
			priv->ieee80211->stats.tx_errors++;
			//priv->stats.txmanageerr++;
			// TODO */
		}
	}

	if( !priv->up)
		return;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)
	skb_unlink(skb, &priv->tx_queue);
#else
	skb_unlink(skb);
#endif

#ifdef ENABLE_TX_ISR_TASKLET
	skb_queue_tail(&priv->tx_skb_queue, skb);
	tasklet_schedule(&priv->irq_tx_tasklet);
#else
	handle_tx_skb(skb);
#endif	
}

void rtl8192_irq_tx_tasklet(struct r8192_priv *priv)
{
        struct sk_buff *skb;
	cb_desc *tcb_desc = NULL;
	skb = skb_dequeue(&priv->tx_skb_queue);
	
	if ( NULL != skb) {
		tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
		atomic_dec(&priv->tx_pending[tcb_desc->queue_index]);
        	handle_tx_skb(skb);
        }

	if ( (skb_queue_len(&priv->tx_skb_queue) != 0) )
	{
			tasklet_schedule(&priv->irq_tx_tasklet);
	}

}


void handle_tx_skb(struct sk_buff *skb)
{
	struct net_device *dev = skb->dev;
	struct r8192_priv *priv = ieee80211_priv(dev);
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	u8  queue_index = tcb_desc->queue_index;

	if(skb != NULL) {
		dev_kfree_skb_any(skb);
	}

	{
		//
		// Handle HW Beacon: 
		// We had transfer our beacon frame to host controler at this moment.
		//
#if 0
		if(tcb_desc->tx_queue == BEACON_QUEUE)
		{
			priv->bSendingBeacon = FALSE;
		}
#endif
		//
		// Caution:
		// Handling the wait queue of command packets.
		// For Tx command packets, we must not do TCB fragment because it is not handled right now.
		// We must cut the packets to match the size of TX_CMD_PKT before we send it.
		//
		if (queue_index == MGNT_QUEUE){
	        	if (priv->ieee80211->ack_tx_to_ieee){
		            	if (rtl8192_is_tx_queue_empty(dev)){
		                	priv->ieee80211->ack_tx_to_ieee = 0;
		                	ieee80211_ps_tx_ack(priv->ieee80211, 1);
		            	}
	        	}
	    	} 

		// Handle MPDU in wait queue. */
		if(queue_index != BEACON_QUEUE) {

			// Don't send data frame during scanning.*/
			if((skb_queue_len(&priv->ieee80211->skb_waitQ[queue_index]) != 0)&&\
					(!(priv->ieee80211->queue_stop))) {
#ifdef WIFI_TEST // kenny: queue select
				if (queue_index <= VO_QUEUE)
					queue_index = wmm_queue_select(priv, queue_index, priv->ieee80211->skb_waitQ);
#endif
				if(NULL != (skb = skb_dequeue(&(priv->ieee80211->skb_waitQ[queue_index]))))
				{
					if(queue_index >= TXCMD_QUEUE)
						priv->ieee80211->softmac_hard_start_xmit(skb, dev);  
					else
						priv->ieee80211->softmac_data_hard_start_xmit(skb, dev, priv->ieee80211->rate);  
				}
				return; //modified by david to avoid further processing AMSDU 
			}
#ifdef ENABLE_AMSDU
			else if((skb_queue_len(&priv->ieee80211->skb_aggQ[queue_index]) > 0)&&\
					(!(priv->ieee80211->queue_stop))){

				struct sk_buff_head pSendList;
				u8 dst[ETH_ALEN];
				cb_desc *tcb_desc = NULL;
				int qos_actived = priv->ieee80211->current_network.qos_data.active;//YJ,add,081103
				struct sta_info *psta = NULL;
				u8 bIsSptAmsdu = false;
				
#ifdef WIFI_TEST // kenny: queue select
				if (queue_index <= VO_QUEUE)
					queue_index = wmm_queue_select(priv, queue_index, priv->ieee80211->skb_aggQ);
#endif
				//printk("In %s: AggrQ len %d QIndex %d\n",__FUNCTION__, skb_queue_len(&priv->ieee80211->skb_aggQ[queue_index]), queue_index);
				//YJ,add,090409
				priv->ieee80211->amsdu_in_process = true;

				skb = skb_dequeue(&(priv->ieee80211->skb_aggQ[queue_index]));
				if(skb == NULL)
				{
					printk("In %s:Skb is NULL\n",__FUNCTION__);
					return;
				}
				//printk("!!!!!dequeue skb len %d &&&skb=%p\n",skb->len,skb);

				tcb_desc = (pcb_desc)(skb->cb + MAX_DEV_ADDR_SIZE);
				if(tcb_desc->bFromAggrQ)
				{
					dev->hard_start_xmit(skb, dev);
					return;
				}
					
				memcpy(dst, skb->data, ETH_ALEN);
				if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				{
					psta = GetStaInfo(priv->ieee80211, dst);
					if(psta) {
						if(psta->htinfo.bEnableHT)
							bIsSptAmsdu = true;
					}
				}
				else if(priv->ieee80211->iw_mode == IW_MODE_INFRA)
					bIsSptAmsdu = true;
				else
					bIsSptAmsdu = true;
				
				bIsSptAmsdu = bIsSptAmsdu && priv->ieee80211->pHTInfo->bCurrent_AMSDU_Support && qos_actived;

				//printk("@@@qos_actived=%d dst:"MAC_FMT" AMSDU_spt=%d\n", qos_actived, MAC_ARG(dst), priv->ieee80211->pHTInfo->bCurrent_AMSDU_Support);
				if(qos_actived && 	!is_broadcast_ether_addr(dst) && 
								!is_multicast_ether_addr(dst) &&
								bIsSptAmsdu)
				{
					skb_queue_head_init(&pSendList);
					if(AMSDU_GetAggregatibleList(priv->ieee80211, skb, &pSendList,queue_index))
					{
#if 1				
						struct sk_buff * pAggrSkb = AMSDU_Aggregation(priv->ieee80211, &pSendList);
						if(NULL != pAggrSkb)
							dev->hard_start_xmit(pAggrSkb, dev);
#endif
					}
				}
				else
				{
					memset(skb->cb,0,sizeof(skb->cb));
					tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
					tcb_desc->bFromAggrQ = true;
					dev->hard_start_xmit(skb, dev);
				}
			}
#endif
#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
			else if ((skb_queue_len(&priv->ieee80211->skb_drv_aggQ[queue_index])!= 0)&&\
 				(!(priv->ieee80211->queue_stop))) {

#ifdef WIFI_TEST // kenny: queue select
				if (queue_index <= VO_QUEUE)
					queue_index = wmm_queue_select(priv, queue_index, priv->ieee80211->skb_drv_aggQ);
#endif
				// Tx Driver Aggregation process 
				// The driver will aggregation the packets according to the following stets  
				// 1. check whether there's tx irq available, for it's a completion return 
				//    function, it should contain enough tx irq;
				// 2. check pakcet type;
				// 3. intialize sendlist, check whether the to-be send packet no greater than 1 
				// 4. aggregation the packets, and fill firmware info and tx desc to it, etc.
				// 5. check whehter the packet could be sent, otherwise just insert to wait head 
				// 
				skb = skb_dequeue(&priv->ieee80211->skb_drv_aggQ[queue_index]);
				if(skb == NULL)
				{
					printk("In %s:Skb is NULL\n",__FUNCTION__);
					return;
				}
				if(!check_nic_enough_desc(dev, queue_index)) {
					skb_queue_head(&(priv->ieee80211->skb_drv_aggQ[queue_index]), skb);
					return;
				}

				{
					//TODO*/

					//u8* pHeader = skb->data;
					
					//if(IsMgntQosData(pHeader) || 
				        //    IsMgntQData_Ack(pHeader) || 
					//    IsMgntQData_Poll(pHeader) || 
					//    IsMgntQData_Poll_Ack(pHeader) 
					//  ) 
					
					{
						struct ieee80211_drv_agg_txb SendList;

						memset(&SendList, 0, sizeof(struct ieee80211_drv_agg_txb));
						if(DrvAggr_GetAggregatibleList(dev, skb, &SendList) > 1) {
							skb = DrvAggr_Aggregation(dev, &SendList);
						if (skb == NULL)
							return;
#if 0
						printk("=============>to send aggregated packet!\n");
						RT_DEBUG_DATA(COMP_SEND, skb->cb, sizeof(skb->cb));
						printk("\n=================skb->len = %d\n", skb->len);	
						RT_DEBUG_DATA(COMP_SEND, skb->data, skb->len);
#endif
						}
					}
					priv->ieee80211->softmac_hard_start_xmit(skb, dev);
				}
			}
#endif
		}
	}

}

void rtl8192_beacon_stop(struct net_device *dev)
{
	u8 msr, msrm, msr2;
	struct r8192_priv *priv = ieee80211_priv(dev);

	msr  = read_nic_byte(dev, MSR);
	msrm = msr & MSR_LINK_MASK;
	msr2 = msr & ~MSR_LINK_MASK;

	if(NIC_8192U == priv->card_8192) {
		usb_kill_urb(priv->rx_urb[MAX_RX_URB]);
	}
	if ((msrm == (MSR_LINK_ADHOC<<MSR_LINK_SHIFT) ||
		(msrm == (MSR_LINK_MASTER<<MSR_LINK_SHIFT)))){
		write_nic_byte(dev, MSR, msr2 | MSR_LINK_NONE);
		write_nic_byte(dev, MSR, msr);	
	}
}

void rtl8192_config_rate(struct net_device* dev, u16* rate_config)
{
	 struct r8192_priv *priv = ieee80211_priv(dev);
	 struct ieee80211_network *net;
	 u8 i=0, basic_rate = 0;
	 net = & priv->ieee80211->current_network;
	 
	 for (i=0; i<net->rates_len; i++)
	 {
		 basic_rate = net->rates[i]&0x7f;
		 switch(basic_rate)
		 {
			 case MGN_1M:	*rate_config |= RRSR_1M;	break;
			 case MGN_2M:	*rate_config |= RRSR_2M;	break;
			 case MGN_5_5M:	*rate_config |= RRSR_5_5M;	break;
			 case MGN_11M:	*rate_config |= RRSR_11M;	break;
			 case MGN_6M:	*rate_config |= RRSR_6M;	break;
			 case MGN_9M:	*rate_config |= RRSR_9M;	break;
			 case MGN_12M:	*rate_config |= RRSR_12M;	break;
			 case MGN_18M:	*rate_config |= RRSR_18M;	break;
			 case MGN_24M:	*rate_config |= RRSR_24M;	break;
			 case MGN_36M:	*rate_config |= RRSR_36M;	break;
			 case MGN_48M:	*rate_config |= RRSR_48M;	break;
			 case MGN_54M:	*rate_config |= RRSR_54M;	break;
		 }
	 }
	 for (i=0; i<net->rates_ex_len; i++)
	 {
		 basic_rate = net->rates_ex[i]&0x7f;
		 switch(basic_rate)
		 {
			 case MGN_1M:	*rate_config |= RRSR_1M;	break;
			 case MGN_2M:	*rate_config |= RRSR_2M;	break;
			 case MGN_5_5M:	*rate_config |= RRSR_5_5M;	break;
			 case MGN_11M:	*rate_config |= RRSR_11M;	break;
			 case MGN_6M:	*rate_config |= RRSR_6M;	break;
			 case MGN_9M:	*rate_config |= RRSR_9M;	break;
			 case MGN_12M:	*rate_config |= RRSR_12M;	break;
			 case MGN_18M:	*rate_config |= RRSR_18M;	break;
			 case MGN_24M:	*rate_config |= RRSR_24M;	break;
			 case MGN_36M:	*rate_config |= RRSR_36M;	break;
			 case MGN_48M:	*rate_config |= RRSR_48M;	break;
			 case MGN_54M:	*rate_config |= RRSR_54M;	break;
		 }
	 }
}
			 
	 
#define SHORT_SLOT_TIME 9
#define NON_SHORT_SLOT_TIME 20

void rtl8192_update_cap(struct net_device* dev, u16 cap)
{
	//u32 tmp = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_network *net = &priv->ieee80211->current_network;
	priv->short_preamble = cap & WLAN_CAPABILITY_SHORT_PREAMBLE;

	//LZM MOD 090303 HW_VAR_ACK_PREAMBLE
#ifdef RTL8192SU
	{
		u8 tmp = 0;
		//tmp = ((priv->nCur40MhzPrimeSC) << 5);
		if (priv->short_preamble)
			tmp |= 0x80;
		write_nic_byte(dev, RRSR+2, tmp);
	}
#else
	{
	u32 tmp = 0;
	tmp = priv->basic_rate;
	if (priv->short_preamble)
		tmp |= BRSR_AckShortPmb;
	write_nic_dword(dev, RRSR, tmp);
	}
#endif

	if (net->mode & (IEEE_G|IEEE_N_24G))
	{
		u8 slot_time = 0;
		if ((cap & WLAN_CAPABILITY_SHORT_SLOT)&&(!priv->ieee80211->pHTInfo->bCurrentRT2RTLongSlotTime))
		{//short slot time
			slot_time = SHORT_SLOT_TIME;
		}
		else //long slot time
			slot_time = NON_SHORT_SLOT_TIME;
		priv->slot_time = slot_time;
		write_nic_byte(dev, SLOT_TIME, slot_time);
	}

}
void rtl8192_net_update(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_network *net;
	u16 BcnTimeCfg = 0, BcnCW = 6, BcnIFS = 0xf;
	u16 rate_config = 0;
	net = & priv->ieee80211->current_network;
	
	rtl8192_config_rate(dev, &rate_config);	
	priv->basic_rate = rate_config &= 0x15f;

	write_nic_dword(dev,BSSIDR,((u32*)net->bssid)[0]);
	write_nic_word(dev,BSSIDR+4,((u16*)net->bssid)[2]);
	//for(i=0;i<ETH_ALEN;i++)
	//	write_nic_byte(dev,BSSID+i,net->bssid[i]);

	rtl8192_update_msr(dev);
//	rtl8192_update_cap(dev, net->capability);
	if (priv->ieee80211->iw_mode == IW_MODE_ADHOC)
	{
	write_nic_word(dev, ATIMWND, 2);
	write_nic_word(dev, BCN_DMATIME, 1023);	
	write_nic_word(dev, BCN_INTERVAL, net->beacon_interval);
//	write_nic_word(dev, BcnIntTime, 100);
	write_nic_word(dev, BCN_DRV_EARLY_INT, 1);
	write_nic_byte(dev, BCN_ERR_THRESH, 100);
		BcnTimeCfg |= (BcnCW<<BCN_TCFG_CW_SHIFT);
	// TODO: BcnIFS may required to be changed on ASIC
	 	BcnTimeCfg |= BcnIFS<<BCN_TCFG_IFS;

	write_nic_word(dev, BCN_TCFG, BcnTimeCfg);
	}

	

}

//temporary hw beacon is not used any more.
//open it when necessary
#if 1
void rtl819xusb_beacon_tx(struct net_device *dev,u16  tx_rate)
{

#if 0
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct sk_buff *skb;
	int i = 0;
	//u8 cr;

	rtl8192_net_update(dev);

	skb = ieee80211_get_beacon(priv->ieee80211);
		if(!skb){ 
			DMESG("not enought memory for allocating beacon");
			return;
		}


		write_nic_byte(dev, BQREQ, read_nic_byte(dev, BQREQ) | (1<<7));

		i=0;
		//while(!read_nic_byte(dev,BQREQ & (1<<7)))
		while( (read_nic_byte(dev, BQREQ) & (1<<7)) == 0 )
		{
			msleep_interruptible(HZ/2);
			if(i++ > 10){
				DMESGW("get stuck to wait HW beacon to be ready");
				return ;
			}
		}
	skb->cb[0] = NORM_PRIORITY;
	skb->cb[1] = 0; //morefragment = 0
	skb->cb[2] = ieeerate2rtlrate(tx_rate);

	rtl8192_tx(dev,skb);

#endif
}
#endif
inline u8 rtl8192_IsWirelessBMode(u16 rate)
{
	if( ((rate <= 110) && (rate != 60) && (rate != 90)) || (rate == 220) )
		return 1;
	else return 0;
}

u16 N_DBPSOfRate(u16 DataRate);

u16 ComputeTxTime( 
	u16		FrameLength,
	u16		DataRate,
	u8		bManagementFrame,
	u8		bShortPreamble
)
{
	u16	FrameTime;
	u16	N_DBPS;
	u16	Ceiling;

	if( rtl8192_IsWirelessBMode(DataRate) )
	{
		if( bManagementFrame || !bShortPreamble || DataRate == 10 )
		{	// long preamble
			FrameTime = (u16)(144+48+(FrameLength*8/(DataRate/10)));		
		}
		else
		{	// Short preamble
			FrameTime = (u16)(72+24+(FrameLength*8/(DataRate/10)));
		}
		if( ( FrameLength*8 % (DataRate/10) ) != 0 ) //Get the Ceilling
				FrameTime ++;
	} else {	//802.11g DSSS-OFDM PLCP length field calculation.
		N_DBPS = N_DBPSOfRate(DataRate);
		Ceiling = (16 + 8*FrameLength + 6) / N_DBPS 
				+ (((16 + 8*FrameLength + 6) % N_DBPS) ? 1 : 0);
		FrameTime = (u16)(16 + 4 + 4*Ceiling + 6);
	}
	return FrameTime;
}

u16 N_DBPSOfRate(u16 DataRate)
{
	 u16 N_DBPS = 24;
	 
	 switch(DataRate)
	 {
	 case 60:
	  N_DBPS = 24;
	  break;
	 
	 case 90:
	  N_DBPS = 36;
	  break;
	 
	 case 120:
	  N_DBPS = 48;
	  break;
	 
	 case 180:
	  N_DBPS = 72;
	  break;
	 
	 case 240:
	  N_DBPS = 96;
	  break;
	 
	 case 360:
	  N_DBPS = 144;
	  break;
	 
	 case 480:
	  N_DBPS = 192;
	  break;
	 
	 case 540:
	  N_DBPS = 216;
	  break;
	 
	 default:
	  break;
	 }
	 
	 return N_DBPS;
}

void rtl819xU_cmd_isr(struct urb *tx_cmd_urb, struct pt_regs *regs)
{
#if 0
	struct net_device *dev = (struct net_device*)tx_cmd_urb->context;
	struct r8192_priv *priv = ieee80211_priv(dev);
	int		   last_init_packet = 0;
	u8		   *ptr_cmd_buf;
	u16		    cmd_buf_len;

	if(tx_cmd_urb->status != 0) {
		priv->pFirmware.firmware_seg_index = 0; //only begin transter, should it can be set to 1
	}
	
	// Free the urb and the corresponding buf for common Tx cmd packet, or
	// last segment of each firmware img.
	// 
	if((priv->pFirmware.firmware_seg_index == 0) ||(priv->pFirmware.firmware_seg_index == priv->pFirmware.firmware_seg_maxnum)) {
		priv->pFirmware.firmware_seg_index = 0;//only begin transter, should it can be set to 1
	} else {
		// prepare for last transfer */
		// update some infomation for */
		// last segment of the firmware img need indicate to device */
		priv->pFirmware.firmware_seg_index++;
		if(priv->pFirmware.firmware_seg_index == priv->pFirmware.firmware_seg_maxnum) {
			last_init_packet = 1;
		}
		
		cmd_buf_len = priv->pFirmware.firmware_seg_container[priv->pFirmware.firmware_seg_index-1].seg_size;
		ptr_cmd_buf = priv->pFfirmware.firmware_seg_container[priv->pFfirmware.firmware_seg_index-1].seg_ptr;
		rtl819xU_tx_cmd(dev, ptr_cmd_buf, cmd_buf_len, last_init_packet, DESC_PACKET_TYPE_INIT);
	}

	kfree(tx_cmd_urb->transfer_buffer);
#endif
	usb_free_urb(tx_cmd_urb);
}

unsigned int txqueue2outpipe(struct r8192_priv* priv,unsigned int tx_queue) {

	if(tx_queue >= 9)
	{
		RT_TRACE(COMP_ERR,"%s():Unknown queue ID!!!\n",__FUNCTION__);
		return 0x04;
	}
	return priv->txqueue_to_outpipemap[tx_queue];
}

#ifdef RTL8192SU
short rtl8192SU_tx_cmd(struct net_device *dev, struct sk_buff *skb)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int			status;
	struct urb		*tx_urb;
	unsigned int 		idx_pipe;
	tx_desc_cmd_819x_usb *pdesc = (tx_desc_cmd_819x_usb *)skb->data; 
	struct rtl8192_tx_info *tx_info = (struct rtl8192_tx_info *)skb->cb;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	u8 queue_index = tcb_desc->queue_index;
	u32			PktSize = 0;
	
	//printk("\n %s::::::::::::::::::::::queue_index = %d\n",__FUNCTION__, queue_index);
	atomic_inc(&priv->tx_pending[queue_index]);
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
	tx_urb = usb_alloc_urb(0);
#endif
	if(!tx_urb){
		dev_kfree_skb(skb);
		return -ENOMEM;
	}
	
	memset(pdesc, 0, USB_HWDESC_HEADER_LEN);
	
	// Tx descriptor ought to be set according to the skb->cb */
	pdesc->LINIP = tcb_desc->bLastIniPkt;
	PktSize = (u16)(skb->len - USB_HWDESC_HEADER_LEN); 
	pdesc->PktSize = PktSize;
	//printk("PKTSize = %d %x\n",pdesc->PktSize,pdesc->PktSize);
	//----------------------------------------------------------------------------
	// Fill up USB_OUT_CONTEXT.
	//----------------------------------------------------------------------------
	// Get index to out pipe from specified QueueID.
	idx_pipe = txqueue2outpipe(priv,queue_index);
	//printk("=============>%s queue_index:%d, outpipe:%d\n", __func__,queue_index,priv->RtOutPipes[idx_pipe]);

#ifdef JOHN_DUMP_TXDESC
	int i;
	printk("Len = %d\n", skb->len);
	for (i = 0; i < 8; i++)
		printk("%2.2x ", *((u8*)skb->data+i));
	printk("\n");
#endif

	skb->dev = dev;
	tx_info->urb = tx_urb;

	usb_fill_bulk_urb(tx_urb,
	                            priv->udev, 
	                            usb_sndbulkpipe(priv->udev,priv->RtOutPipes[idx_pipe]),
	                            skb->data, 
	                            skb->len, 
	                            rtl8192_tx_isr, 
	                            skb);

	skb_queue_tail(&priv->tx_queue, skb);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	status = usb_submit_urb(tx_urb, GFP_ATOMIC);
#else
	status = usb_submit_urb(tx_urb);
#endif

	if (!status){
		return 0;
	}else{
		printk("Error TX CMD URB, error %d",
				status);
		return -1;
	}
}
#endif
#ifdef RTL8192U
short rtl819xU_tx_cmd(struct net_device *dev, struct sk_buff *skb)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//u8			*tx;
	int			status;
	struct urb		*tx_urb;
	//int			urb_buf_len;
#ifndef USE_ONE_PIPE
	unsigned int 		idx_pipe;
#endif	
        u8 OutPipe;
	tx_desc_cmd_819x_usb *pdesc = (tx_desc_cmd_819x_usb *)skb->data; 
	struct rtl8192_tx_info *tx_info = (struct rtl8192_tx_info *)skb->cb;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	u8 queue_index = tcb_desc->queue_index;
	
	//printk("\n %s::queue_index = %d\n",__FUNCTION__, queue_index);
	atomic_inc(&priv->tx_pending[queue_index]);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
	tx_urb = usb_alloc_urb(0);
#endif
	if(!tx_urb){
		dev_kfree_skb(skb);
		return -ENOMEM;
	}
	
	memset(pdesc, 0, USB_HWDESC_HEADER_LEN);
	// Tx descriptor ought to be set according to the skb->cb */
	pdesc->FirstSeg = 1;//bFirstSeg;
	pdesc->LastSeg = 1;//bLastSeg;
	pdesc->CmdInit = tcb_desc->bCmdOrInit;
	pdesc->TxBufferSize = tcb_desc->txbuf_size;
	pdesc->OWN = 1;
	pdesc->LINIP = tcb_desc->bLastIniPkt;

	//----------------------------------------------------------------------------
	// Fill up USB_OUT_CONTEXT.
	//----------------------------------------------------------------------------
	// Get index to out pipe from specified QueueID.
#ifndef USE_ONE_PIPE
	idx_pipe = txqueue2outpipe(priv,queue_index);
        OutPipe = priv->RtOutPipes[idx_pipe];
#else
	OutPipe = 0x04;
#endif
#ifdef JOHN_DUMP_TXDESC
	int i;
	printk("<Tx descriptor>--rate %x---",rate);
	for (i = 0; i < 8; i++)
		printk("%8x ", tx[i]);
	printk("\n");
#endif

	skb->dev = dev;
	tx_info->urb = tx_urb;

	usb_fill_bulk_urb(tx_urb,priv->udev, usb_sndbulkpipe(priv->udev,OutPipe), \
			skb->data, skb->len, rtl8192_tx_isr, skb);

	skb_queue_tail(&priv->tx_queue, skb);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	status = usb_submit_urb(tx_urb, GFP_ATOMIC);
#else
	status = usb_submit_urb(tx_urb);
#endif

	if (!status){
		return 0;
	}else{
		DMESGE("Error TX CMD URB, error %d",
				status);
		return -1;
	}
}
#endif

//
// Mapping Software/Hardware descriptor queue id to "Queue Select Field"
// in TxFwInfo data structure
// 2006.10.30 by Emily
// // \param QUEUEID       Software Queue       
//
u8 MapHwQueueToFirmwareQueue(u8 QueueID)
{
	u8 QueueSelect = 0x0;       //defualt set to 

	switch(QueueID) {
		case BE_QUEUE:
			QueueSelect = QSLT_BE;  //or QSelect = pTcb->priority;
			break;

		case BK_QUEUE:
			QueueSelect = QSLT_BK;  //or QSelect = pTcb->priority;
			break;

		case VO_QUEUE:
			QueueSelect = QSLT_VO;  //or QSelect = pTcb->priority;
			break;

		case VI_QUEUE:
			QueueSelect = QSLT_VI;  //or QSelect = pTcb->priority;  
			break;
		case MGNT_QUEUE:
			QueueSelect = QSLT_MGNT;
			break;

		case BEACON_QUEUE:
			QueueSelect = QSLT_BEACON;
			break;

			// TODO: 2006.10.30 mark other queue selection until we verify it is OK
			// TODO: Remove Assertions 
//#if (RTL819X_FPGA_VER & RTL819X_FPGA_GUANGAN_070502)
		case TXCMD_QUEUE:
			QueueSelect = QSLT_CMD;
			break;
//#endif
		case HIGH_QUEUE:
			QueueSelect = QSLT_HIGH;
			break;

		default:
			RT_TRACE(COMP_ERR, "TransmitTCB(): Impossible Queue Selection: %d \n", QueueID);
			break;
	}
	return QueueSelect;
}

#ifdef RTL8192SU
u8 MRateToHwRate8190Pci(u8 rate)
{
	u8	ret = DESC92S_RATE1M;
		
	switch(rate)
	{
		// CCK and OFDM non-HT rates
	case MGN_1M:		ret = DESC92S_RATE1M;	break;
	case MGN_2M:		ret = DESC92S_RATE2M;	break;
	case MGN_5_5M:		ret = DESC92S_RATE5_5M;	break;
	case MGN_11M:		ret = DESC92S_RATE11M;	break;
	case MGN_6M:		ret = DESC92S_RATE6M;	break;
	case MGN_9M:		ret = DESC92S_RATE9M;	break;
	case MGN_12M:		ret = DESC92S_RATE12M;	break;
	case MGN_18M:		ret = DESC92S_RATE18M;	break;
	case MGN_24M:		ret = DESC92S_RATE24M;	break;
	case MGN_36M:		ret = DESC92S_RATE36M;	break;
	case MGN_48M:		ret = DESC92S_RATE48M;	break;
	case MGN_54M:		ret = DESC92S_RATE54M;	break;

		// HT rates since here
	case MGN_MCS0:		ret = DESC92S_RATEMCS0;	break;
	case MGN_MCS1:		ret = DESC92S_RATEMCS1;	break;
	case MGN_MCS2:		ret = DESC92S_RATEMCS2;	break;
	case MGN_MCS3:		ret = DESC92S_RATEMCS3;	break;
	case MGN_MCS4:		ret = DESC92S_RATEMCS4;	break;
	case MGN_MCS5:		ret = DESC92S_RATEMCS5;	break;
	case MGN_MCS6:		ret = DESC92S_RATEMCS6;	break;
	case MGN_MCS7:		ret = DESC92S_RATEMCS7;	break;
	case MGN_MCS8:		ret = DESC92S_RATEMCS8;	break;
	case MGN_MCS9:		ret = DESC92S_RATEMCS9;	break;
	case MGN_MCS10:	ret = DESC92S_RATEMCS10;	break;
	case MGN_MCS11:	ret = DESC92S_RATEMCS11;	break;
	case MGN_MCS12:	ret = DESC92S_RATEMCS12;	break;
	case MGN_MCS13:	ret = DESC92S_RATEMCS13;	break;
	case MGN_MCS14:	ret = DESC92S_RATEMCS14;	break;
	case MGN_MCS15:	ret = DESC92S_RATEMCS15;	break;

	// Set the highest SG rate
	case MGN_MCS0_SG:	
	case MGN_MCS1_SG:
	case MGN_MCS2_SG:	
	case MGN_MCS3_SG:	
	case MGN_MCS4_SG:	
	case MGN_MCS5_SG:
	case MGN_MCS6_SG:	
	case MGN_MCS7_SG:	
	case MGN_MCS8_SG:	
	case MGN_MCS9_SG:
	case MGN_MCS10_SG:
	case MGN_MCS11_SG:	
	case MGN_MCS12_SG:	
	case MGN_MCS13_SG:	
	case MGN_MCS14_SG:
	case MGN_MCS15_SG:	
	{
		ret = DESC92S_RATEMCS15_SG;
		break;
	}		

	default:		break;
	}
	return ret;
}
#endif
#ifdef RTL8192U
u8 MRateToHwRate8190Pci(u8 rate)
{
	u8  ret = DESC90_RATE1M;

	switch(rate) {
		case MGN_1M:    ret = DESC90_RATE1M;    break;
		case MGN_2M:    ret = DESC90_RATE2M;    break;
		case MGN_5_5M:  ret = DESC90_RATE5_5M;  break;
		case MGN_11M:   ret = DESC90_RATE11M;   break;
		case MGN_6M:    ret = DESC90_RATE6M;    break;
		case MGN_9M:    ret = DESC90_RATE9M;    break;
		case MGN_12M:   ret = DESC90_RATE12M;   break;
		case MGN_18M:   ret = DESC90_RATE18M;   break;
		case MGN_24M:   ret = DESC90_RATE24M;   break;
		case MGN_36M:   ret = DESC90_RATE36M;   break;
		case MGN_48M:   ret = DESC90_RATE48M;   break;
		case MGN_54M:   ret = DESC90_RATE54M;   break;

		// HT rate since here
		case MGN_MCS0:  ret = DESC90_RATEMCS0;  break;
		case MGN_MCS1:  ret = DESC90_RATEMCS1;  break;
		case MGN_MCS2:  ret = DESC90_RATEMCS2;  break;
		case MGN_MCS3:  ret = DESC90_RATEMCS3;  break;
		case MGN_MCS4:  ret = DESC90_RATEMCS4;  break;
		case MGN_MCS5:  ret = DESC90_RATEMCS5;  break;
		case MGN_MCS6:  ret = DESC90_RATEMCS6;  break;
		case MGN_MCS7:  ret = DESC90_RATEMCS7;  break;
		case MGN_MCS8:  ret = DESC90_RATEMCS8;  break;
		case MGN_MCS9:  ret = DESC90_RATEMCS9;  break;
		case MGN_MCS10: ret = DESC90_RATEMCS10; break;
		case MGN_MCS11: ret = DESC90_RATEMCS11; break;
		case MGN_MCS12: ret = DESC90_RATEMCS12; break;
		case MGN_MCS13: ret = DESC90_RATEMCS13; break;
		case MGN_MCS14: ret = DESC90_RATEMCS14; break;
		case MGN_MCS15: ret = DESC90_RATEMCS15; break;
		case (0x80|0x20): ret = DESC90_RATEMCS32; break;

		default:       break;
	}
	return ret;
}
#endif

u8 QueryIsShort(u8 TxHT, u8 TxRate, cb_desc *tcb_desc)
{
	u8   tmp_Short;

	tmp_Short = (TxHT==1)?((tcb_desc->bUseShortGI)?1:0):((tcb_desc->bUseShortPreamble)?1:0);

	if(TxHT==1 && TxRate != DESC90_RATEMCS15)
		tmp_Short = 0;

	return tmp_Short;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static void tx_zero_isr(struct urb *tx_urb, struct pt_regs *reg)
#else
static void tx_zero_isr(struct urb *tx_urb)
#endif
{
	usb_free_urb(tx_urb);
	return;
}


#ifdef RTL8192SU
 
// The tx procedure is just as following,  skb->cb will contain all the following
// information: * priority, morefrag, rate, &dev.
// 
 //	<Note> Buffer format for 8192S Usb bulk out:
//
//  --------------------------------------------------
//  | 8192S Usb Tx Desc | 802_11_MAC_header |    data          |
//  --------------------------------------------------
//  |  32 bytes           	  |       24 bytes             |0-2318 bytes|
//  --------------------------------------------------
//  |<------------ BufferLen ------------------------->|

short rtl8192SU_tx(struct net_device *dev, struct sk_buff* skb)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct rtl8192_tx_info *tx_info = (struct rtl8192_tx_info *)skb->cb;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	tx_desc_819x_usb *tx_desc = (tx_desc_819x_usb *)skb->data;
	//tx_fwinfo_819x_usb *tx_fwinfo = (tx_fwinfo_819x_usb *)(skb->data + USB_HWDESC_HEADER_LEN);//92su del
	struct usb_device *udev = priv->udev;
	int pend;
	int status;
	struct urb *tx_urb = NULL, *tx_urb_zero = NULL;
	//int urb_len;		
	unsigned int idx_pipe;
	u16		MPDUOverhead = 0;
 	//RT_DEBUG_DATA(COMP_SEND, tcb_desc, sizeof(cb_desc));
 	
 	u16 fc, type = 0;
	struct ieee80211_hdr_1addr * header = NULL;
	bool multi_addr = false, broad_addr = false, uni_addr = false;
	u8* pad_addr = NULL;

	//header = (struct ieee80211_hdr_1addr *)(((u8*)skb->data + USB_HWDESC_HEADER_LEN));
	header = (struct ieee80211_hdr_1addr *)(((u8*)skb->data));
    	fc = header->frame_ctl;
   	type = WLAN_FC_GET_TYPE(fc);
	
	pad_addr = header->addr1;
	if(is_multicast_ether_addr(pad_addr))
		multi_addr = true;
	else if(is_broadcast_ether_addr(pad_addr))
		broad_addr = true;
	else
		uni_addr = true;

	//lzm test
	if(uni_addr)
		priv->stats.txbytesunicast += (skb->len);
	else if(multi_addr)
		;
	else
		;
	
#if 0
	// Added by Annie for filling Len_Adjust field. 2005-12-14. */
        RT_ENC_ALG  EncAlg = NO_Encryption; 
#endif


	pend = atomic_read(&priv->tx_pending[tcb_desc->queue_index]);
	// we are locked here so the two atomic_read and inc are executed 
	// without interleaves  * !!! For debug purpose 	  
	if( pend > MAX_TX_URB){
		// tcb_desc->queue_index = WME_AC_BK, WME_AC_BE, WME_AC_VI, WME_AC_VO
		printk("To discard skb packet! queue_index=%d, pend=%d\n", tcb_desc->queue_index, pend);
		dev_kfree_skb_any(skb);
		return -1;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
	tx_urb = usb_alloc_urb(0);
#endif
	if(!tx_urb){
		dev_kfree_skb_any(skb);
		return -ENOMEM;
	}

	memset(tx_desc, 0, sizeof(tx_desc_819x_usb));

 
#if RTL8192SU_FPGA_UNSPECIFIED_NETWORK	
		if(IsQoSDataFrame(skb->data))		
		{
			tcb_desc->bAMPDUEnable = TRUE;	
			tx_desc->NonQos = 0;			
		}
		else
			tcb_desc->bAMPDUEnable = FALSE;

		tcb_desc->bPacketBW = TRUE;
		priv->CurrentChannelBW = HT_CHANNEL_WIDTH_20_40;			
#endif

#if (RTL8192SU_FPGA_2MAC_VERIFICATION||RTL8192SU_ASIC_VERIFICATION)	
		tx_desc->NonQos = (IsQoSDataFrame(skb->data)==TRUE)? 0:1;
#endif

	// Fill Tx descriptor */
	//memset(tx_fwinfo,0,sizeof(tx_fwinfo_819x_usb));
	
	// This part can just fill to the first descriptor of the frame.
	// DWORD 0 */
	tx_desc->TxHT = (tcb_desc->data_rate&0x80)?1:0;

#ifdef RTL8192SU_DISABLE_CCK_RATE
		if(tx_hal_is_cck_rate(tcb_desc->data_rate))
			tcb_desc->data_rate = MGN_6M;
#endif

	tx_desc->TxRate = MRateToHwRate8190Pci(tcb_desc->data_rate);
	//tx_desc->EnableCPUDur = tcb_desc->bTxEnableFwCalcDur;     
	tx_desc->TxShort = QueryIsShort(tx_desc->TxHT, tx_desc->TxRate, tcb_desc);

	
	// Aggregation related		
	if(tcb_desc->bAMPDUEnable) {//AMPDU enabled
		tx_desc->AllowAggregation = 1;
		// DWORD 1 */
		//tx_fwinfo->RxMF = tcb_desc->ampdu_factor;
		//tx_fwinfo->RxAMD = tcb_desc->ampdu_density&0x07;//ampdudensity 
	} else {
		tx_desc->AllowAggregation = 0;
		// DWORD 1 */
		//tx_fwinfo->RxMF = 0;
		//tx_fwinfo->RxAMD = 0;
	}

	//
	// <Roger_Notes> For AMPDU case, we must insert SSN into TX_DESC,
	// FW according as this SSN to do necessary packet retry.
	// 2008.06.06.
	//
	{
		u8	*pSeq;
		u16	Temp;				
		//pSeq = (u8 *)(VirtualAddress+USB_HWDESC_HEADER_LEN + FRAME_OFFSET_SEQUENCE);
		pSeq = (u8 *)(skb->data+USB_HWDESC_HEADER_LEN + 22);
		Temp = pSeq[0];
		Temp <<= 12;			
		Temp |= (*(u16 *)pSeq)>>4;
		tx_desc->Seq = Temp;	
	}
		
	// Protection mode related */
	tx_desc->RTSEn = (tcb_desc->bRTSEnable)?1:0;
	tx_desc->CTS2Self = (tcb_desc->bCTSEnable)?1:0;
	tx_desc->RTSSTBC = (tcb_desc->bRTSSTBC)?1:0;
	tx_desc->RTSHT = (tcb_desc->rts_rate&0x80)?1:0;
	tx_desc->RTSRate =  MRateToHwRate8190Pci((u8)tcb_desc->rts_rate);
	tx_desc->RTSSubcarrier = (tx_desc->RTSHT==0)?(tcb_desc->RTSSC):0;
	tx_desc->RTSBW = (tx_desc->RTSHT==1)?((tcb_desc->bRTSBW)?1:0):0;
	tx_desc->RTSShort = (tx_desc->RTSHT==0)?(tcb_desc->bRTSUseShortPreamble?1:0):\
				(tcb_desc->bRTSUseShortGI?1:0);
	//LZM 090219 
	tx_desc->DisRTSFB = 0;
	tx_desc->RTSRateFBLmt = 0xf;

	// <Roger_EXP> 2008.09.22. We disable RTS rate fallback temporarily.
	//tx_desc->DisRTSFB = 0x01;

	// Set Bandwidth and sub-channel settings. */
	if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
	{
		if(tcb_desc->bPacketBW) {
			tx_desc->TxBandwidth = 1;
			tx_desc->TxSubCarrier = 0;    //By SD3's Jerry suggestion, use duplicated mode
		} else {
			tx_desc->TxBandwidth = 0;
			tx_desc->TxSubCarrier = priv->nCur40MhzPrimeSC;
		}
	} else {
		tx_desc->TxBandwidth = 0;
		tx_desc->TxSubCarrier = 0;
	}

#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
	if (tcb_desc->drv_agg_enable)
	{
		//tx_desc->Tx_INFO_RSVD = (tcb_desc->DrvAggrNum & 0x1f) << 1; //92su del
	}
#endif

	//memset(tx_desc, 0, sizeof(tx_desc_819x_usb));
	// DWORD 0 */
        tx_desc->LINIP = 0;
        //tx_desc->CmdInit = 1; //92su del
        tx_desc->Offset =  USB_HWDESC_HEADER_LEN;

#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
	if (tcb_desc->drv_agg_enable) {
		tx_desc->PktSize = tcb_desc->pkt_size;//FIXLZM
	} else
#endif
	{
		tx_desc->PktSize = (skb->len - USB_HWDESC_HEADER_LEN) & 0xffff;
	}

	//DWORD 1*/
	//tx_desc->SecCAMID= 0;//92su del
	tx_desc->RaBRSRID= tcb_desc->RATRIndex;
//#ifdef RTL8192S_PREPARE_FOR_NORMAL_RELEASE
#if 0//LZM 090219
	tx_desc->RaBRSRID= 1;
#endif

#if 0
	// Fill security related */
	if( pTcb->bEncrypt && !Adapter->MgntInfo.SecurityInfo.SWTxEncryptFlag)
	{
		EncAlg = SecGetEncryptionOverhead(
				Adapter,
				&EncryptionMPDUHeadOverhead,
				&EncryptionMPDUTailOverhead,
				NULL,
				NULL,
				FALSE,
				FALSE);
		//2004/07/22, kcwu, EncryptionMPDUHeadOverhead has been added in previous code
		//MPDUOverhead = EncryptionMPDUHeadOverhead + EncryptionMPDUTailOverhead;
		MPDUOverhead = EncryptionMPDUTailOverhead;
		tx_desc->NoEnc = 0;
		RT_TRACE(COMP_SEC, DBG_LOUD, ("******We in the loop SecCAMID is %d SecDescAssign is %d The Sec is %d********\n",tx_desc->SecCAMID,tx_desc->SecDescAssign,EncAlg));
		//CamDumpAll(Adapter);
	}
	else
#endif
	{
		MPDUOverhead = 0;
		//tx_desc->NoEnc = 1;//92su del
	}
#if 0
	switch(EncAlg){
		case NO_Encryption:
			tx_desc->SecType = 0x0;
			break;
		case WEP40_Encryption:
		case WEP104_Encryption:
			tx_desc->SecType = 0x1;
			break;
		case TKIP_Encryption:
			tx_desc->SecType = 0x2;
			break;
		case AESCCMP_Encryption:
			tx_desc->SecType = 0x3;
			break;
		default:
			tx_desc->SecType = 0x0;
			break;
	}
#else
	tx_desc->SecType = 0x0;
#endif
		if (tcb_desc->bHwSec)
			{
				switch (priv->ieee80211->pairwise_key_type)
				{
					case KEY_TYPE_WEP40:
					case KEY_TYPE_WEP104:
						 tx_desc->SecType = 0x1;
						 //tx_desc->NoEnc = 0;//92su del
						 break;
					case KEY_TYPE_TKIP:
						 tx_desc->SecType = 0x2;
						 //tx_desc->NoEnc = 0;//92su del
						 break;
					case KEY_TYPE_CCMP:
						 tx_desc->SecType = 0x3;
						 //tx_desc->NoEnc = 0;//92su del
						 break;
					case KEY_TYPE_NA:
						 tx_desc->SecType = 0x0;
						 //tx_desc->NoEnc = 1;//92su del
						 break;
					default:
						 tx_desc->SecType = 0x0;
						 //tx_desc->NoEnc = 1;//92su del
						 break;
				}
			}

	//tx_desc->TxFWInfoSize =  sizeof(tx_fwinfo_819x_usb);//92su del

	
	tx_desc->USERATE = tcb_desc->bTxUseDriverAssingedRate;
	tx_desc->DISFB = tcb_desc->bTxDisableRateFallBack;
	tx_desc->DataRateFBLmt = 0x1F;// Alwasy enable all rate fallback range

#ifdef WIFI_TEST
	u8 orig_queue_index = tcb_desc->queue_index;
	static u8 vi2be_queue = 0;
	if (priv->ep_num == 4) {
		switch(tcb_desc->queue_index) {
			case BE_QUEUE:
				if  ( ( atomic_read(&priv->tx_pending[BK_QUEUE]) != 0 )
					&& ( atomic_read(&priv->tx_pending[VI_QUEUE]) == 0 )
					&& ( atomic_read(&priv->tx_pending[VO_QUEUE]) == 0 )
				     ) {
					tcb_desc->queue_index = VI_QUEUE; // move to high queue (VO+VI)
				}
				break;
			case VI_QUEUE:
				if ( (atomic_read(&priv->tx_pending[VO_QUEUE]) != 0 ) 
					|| vi2be_queue // To avoid out of order for VI traffic stream
				    ) {
					vi2be_queue = 1;
					tcb_desc->queue_index = BE_QUEUE; // move to low queue (BE+BK)
				}

				if (skb_queue_len(&priv->ieee80211->skb_waitQ[VI_QUEUE]) == 0) {
					vi2be_queue = 0;
				}
				break;
			default:
				break;					
		}
	}
#endif

	tx_desc->QueueSelect = MapHwQueueToFirmwareQueue(tcb_desc->queue_index);


        // Fill fields that are required to be initialized in all of the descriptors */
        //DWORD 0
#if 0	
        tx_desc->FirstSeg = (tcb_desc->bFirstSeg)? 1:0;
        tx_desc->LastSeg = (tcb_desc->bLastSeg)?1:0;
#else 
        tx_desc->FirstSeg = 1;
        tx_desc->LastSeg = 1;
#endif
        tx_desc->OWN = 1;

#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE	
	if (tcb_desc->drv_agg_enable) {
		tx_desc->TxBufferSize = tcb_desc->pkt_size + sizeof(tx_fwinfo_819x_usb);
	} else
#endif
	{
		//DWORD 2
		//tx_desc->TxBufferSize = (u32)(skb->len - USB_HWDESC_HEADER_LEN);
		tx_desc->TxBufferSize = (u32)(skb->len);//92su mod FIXLZM
	}

#if 0
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(1)TxFillDescriptor8192SUsb(): DataRate(%#x)\n", pTcb->DataRate));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(2)TxFillDescriptor8192SUsb(): bTxUseDriverAssingedRate(%#x)\n", pTcb->bTxUseDriverAssingedRate));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(3)TxFillDescriptor8192SUsb(): bAMPDUEnable(%d)\n", pTcb->bAMPDUEnable));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(4)TxFillDescriptor8192SUsb(): bRTSEnable(%d)\n", pTcb->bRTSEnable));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(5)TxFillDescriptor8192SUsb(): RTSRate(%#x)\n", pTcb->RTSRate));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(6)TxFillDescriptor8192SUsb(): bCTSEnable(%d)\n", pTcb->bCTSEnable));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(7)TxFillDescriptor8192SUsb(): bUseShortGI(%d)\n", pTcb->bUseShortGI));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(8)TxFillDescriptor8192SUsb(): bPacketBW(%d)\n", pTcb->bPacketBW));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(9)TxFillDescriptor8192SUsb(): CurrentChannelBW(%d)\n", pHalData->CurrentChannelBW));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(10)TxFillDescriptor8192SUsb(): bTxDisableRateFallBack(%d)\n", pTcb->bTxDisableRateFallBack));
	RT_TRACE(COMP_FPGA, DBG_LOUD, ("(11)TxFillDescriptor8192SUsb(): RATRIndex(%d)\n", pTcb->RATRIndex));
#endif

	// Get index to out pipe from specified QueueID */
	idx_pipe = txqueue2outpipe(priv,tcb_desc->queue_index);
	//printk("=============>%s queue_index:%d, outpipe:%d\n", __func__,tcb_desc->queue_index,priv->RtOutPipes[idx_pipe]);

	skb->dev = dev;
	tx_info->urb = tx_urb;

	//RT_DEBUG_DATA(COMP_SEND,tx_fwinfo,sizeof(tx_fwinfo_819x_usb));
	//RT_DEBUG_DATA(COMP_SEND,tx_desc,sizeof(tx_desc_819x_usb));

	// To submit bulk urb */
	usb_fill_bulk_urb(tx_urb,
				    udev, 
				    usb_sndbulkpipe(udev,priv->RtOutPipes[idx_pipe]), 
				    skb->data,
				    skb->len, rtl8192_tx_isr, skb);
#ifdef WIFI_TEST
	tcb_desc->queue_index = orig_queue_index;
#endif

        if(type == IEEE80211_FTYPE_DATA) {
            if(priv->ieee80211->LedControlHandler != NULL)
                priv->ieee80211->LedControlHandler(dev, LED_CTL_TX);
        }
	
	skb_queue_tail(&priv->tx_queue, skb);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	status = usb_submit_urb(tx_urb, GFP_ATOMIC);
#else
	status = usb_submit_urb(tx_urb);
#endif
	if (!status){
//we need to send 0 byte packet whenever 512N bytes/64N(HIGN SPEED/NORMAL SPEED) bytes packet has been transmitted. Otherwise, it will be halt to wait for another packet. WB. 2008.08.27
		bool bSend0Byte = false;
		u8 zero = 0;
		if(udev->speed == USB_SPEED_HIGH)
		{
			if (skb->len > 0 && skb->len % 512 == 0)
				bSend0Byte = true;
		}
		else
		{
			if (skb->len > 0 && skb->len % 64 == 0)
				bSend0Byte = true;
		} 
		if (bSend0Byte)
		{
#if 1
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
			tx_urb_zero = usb_alloc_urb(0,GFP_ATOMIC);
#else
			tx_urb_zero = usb_alloc_urb(0);
#endif
			if(!tx_urb_zero){
				RT_TRACE(COMP_ERR, "can't alloc urb for zero byte\n");
				return -ENOMEM;
			}

			tx_info->zero = tx_urb_zero;

			usb_fill_bulk_urb(tx_urb_zero,udev,
					usb_sndbulkpipe(udev,priv->RtOutPipes[idx_pipe]), &zero,
					0, tx_zero_isr, dev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
			status = usb_submit_urb(tx_urb_zero, GFP_ATOMIC);
#else
			status = usb_submit_urb(tx_urb_zero);
#endif
			if (status){
			RT_TRACE(COMP_ERR, "Error TX URB for zero byte %d, error %d", atomic_read(&priv->tx_pending[tcb_desc->queue_index]), status);
			return -1;
			}
#endif
		}
		dev->trans_start = jiffies;
		atomic_inc(&priv->tx_pending[tcb_desc->queue_index]);
		return 0;
	}else{
		RT_TRACE(COMP_ERR, "Error TX URB %d, error %d", atomic_read(&priv->tx_pending[tcb_desc->queue_index]),
				status);
		return -1;
	}
}
#endif

#ifdef RTL8192U
// 
// The tx procedure is just as following, 
// skb->cb will contain all the following information,
// priority, morefrag, rate, &dev.
// 
short rtl8192_tx(struct net_device *dev, struct sk_buff* skb)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct rtl8192_tx_info *tx_info = (struct rtl8192_tx_info *)skb->cb;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	tx_desc_819x_usb *tx_desc = (tx_desc_819x_usb *)skb->data;
	tx_fwinfo_819x_usb *tx_fwinfo = (tx_fwinfo_819x_usb *)(skb->data + USB_HWDESC_HEADER_LEN);
	struct usb_device *udev = priv->udev;
	int pend;
	int status;
	struct urb *tx_urb = NULL, *tx_urb_zero = NULL;
	//int urb_len;		
#ifndef USE_ONE_PIPE
	unsigned int idx_pipe;
#endif	
        u8 OutPipe;

	{
	bool multi_addr = false, broad_addr = false, uni_addr = false;
	u8* pad_addr = NULL;
	struct ieee80211_hdr_1addr * header = NULL;

	header = (struct ieee80211_hdr_1addr *)(((u8*)skb->data) + sizeof(tx_fwinfo_819x_usb));

	pad_addr = header->addr1;
	if(is_multicast_ether_addr(pad_addr))
		multi_addr = true;
	else if(is_broadcast_ether_addr(pad_addr))
		broad_addr = true;
	else
		uni_addr = true;

	//lzm test
	if(uni_addr)
		priv->stats.txbytesunicast += (skb->len) - sizeof(tx_fwinfo_819x_usb);
	else if(multi_addr)
		;
	else
		;
	}
//	RT_DEBUG_DATA(COMP_SEND, tcb_desc, sizeof(cb_desc));
#if 0
	// Added by Annie for filling Len_Adjust field. 2005-12-14. */
        RT_ENC_ALG  EncAlg = NO_Encryption; 
#endif
//	printk("=============> %s\n", __FUNCTION__);
	pend = atomic_read(&priv->tx_pending[tcb_desc->queue_index]);
	//  we are locked here so the two atomic_read and inc are executed 
	//  without interleaves 
	//  !!! For debug purpose 	 
	// 
	if( pend > MAX_TX_URB){
#if 0
		switch (tcb_desc->queue_index) {
			case VO_PRIORITY:
				priv->stats.txvodrop++;
				break;
			case VI_PRIORITY:
				priv->stats.txvidrop++;
				break;
			case BE_PRIORITY:
				priv->stats.txbedrop++;
				break;
			default://BK_PRIORITY
				priv->stats.txbkdrop++;
				break;	
		}
#endif	
		printk("To discard skb packet!\n");
		dev_kfree_skb_any(skb);
		return -1;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
	tx_urb = usb_alloc_urb(0);
#endif
	if(!tx_urb){
		dev_kfree_skb_any(skb);
		return -ENOMEM;
	}

	// Fill Tx firmware info */
	memset(tx_fwinfo,0,sizeof(tx_fwinfo_819x_usb));
	// DWORD 0 */
	tx_fwinfo->TxHT = (tcb_desc->data_rate&0x80)?1:0;
	tx_fwinfo->TxRate = MRateToHwRate8190Pci(tcb_desc->data_rate);
	tx_fwinfo->EnableCPUDur = tcb_desc->bTxEnableFwCalcDur;     
	tx_fwinfo->Short = QueryIsShort(tx_fwinfo->TxHT, tx_fwinfo->TxRate, tcb_desc);
	if(tcb_desc->bAMPDUEnable) {//AMPDU enabled
		tx_fwinfo->AllowAggregation = 1;
		// DWORD 1 */
		tx_fwinfo->RxMF = tcb_desc->ampdu_factor;
		tx_fwinfo->RxAMD = tcb_desc->ampdu_density&0x07;//ampdudensity 
	} else {
		tx_fwinfo->AllowAggregation = 0;
		// DWORD 1 */
		tx_fwinfo->RxMF = 0;
		tx_fwinfo->RxAMD = 0;
	}

	// Protection mode related */
	tx_fwinfo->RtsEnable = (tcb_desc->bRTSEnable)?1:0;
	tx_fwinfo->CtsEnable = (tcb_desc->bCTSEnable)?1:0;
	tx_fwinfo->RtsSTBC = (tcb_desc->bRTSSTBC)?1:0;
	tx_fwinfo->RtsHT = (tcb_desc->rts_rate&0x80)?1:0;
	tx_fwinfo->RtsRate =  MRateToHwRate8190Pci((u8)tcb_desc->rts_rate);
	tx_fwinfo->RtsSubcarrier = (tx_fwinfo->RtsHT==0)?(tcb_desc->RTSSC):0;
	tx_fwinfo->RtsBandwidth = (tx_fwinfo->RtsHT==1)?((tcb_desc->bRTSBW)?1:0):0;
	tx_fwinfo->RtsShort = (tx_fwinfo->RtsHT==0)?(tcb_desc->bRTSUseShortPreamble?1:0):\
				(tcb_desc->bRTSUseShortGI?1:0);

	// Set Bandwidth and sub-channel settings. */
	if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
	{
		if(tcb_desc->bPacketBW) {
			tx_fwinfo->TxBandwidth = 1;
			tx_fwinfo->TxSubCarrier = 0;    //By SD3's Jerry suggestion, use duplicated mode
		} else {
			tx_fwinfo->TxBandwidth = 0;
			tx_fwinfo->TxSubCarrier = priv->nCur40MhzPrimeSC;
		}
	} else {
		tx_fwinfo->TxBandwidth = 0;
		tx_fwinfo->TxSubCarrier = 0;
	}

#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
	if (tcb_desc->drv_agg_enable)
	{
		tx_fwinfo->Tx_INFO_RSVD = (tcb_desc->DrvAggrNum & 0x1f) << 1;
	}
#endif
	// Fill Tx descriptor */
	memset(tx_desc, 0, sizeof(tx_desc_819x_usb));
	// DWORD 0 */
        tx_desc->LINIP = 0;
        tx_desc->CmdInit = 1; 
        tx_desc->Offset =  sizeof(tx_fwinfo_819x_usb) + 8;

#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
	if (tcb_desc->drv_agg_enable) {
		tx_desc->PktSize = tcb_desc->pkt_size;
	} else
#endif
	{
		tx_desc->PktSize = (skb->len - TX_PACKET_SHIFT_BYTES) & 0xffff;
	}

	//DWORD 1*/
	tx_desc->SecCAMID= 0;
	tx_desc->RATid = tcb_desc->RATRIndex;
#if 0
	// Fill security related */
	if( pTcb->bEncrypt && !Adapter->MgntInfo.SecurityInfo.SWTxEncryptFlag)
	{
		EncAlg = SecGetEncryptionOverhead(
				Adapter,
				&EncryptionMPDUHeadOverhead,
				&EncryptionMPDUTailOverhead,
				NULL,
				NULL,
				FALSE,
				FALSE);
		//2004/07/22, kcwu, EncryptionMPDUHeadOverhead has been added in previous code
		//MPDUOverhead = EncryptionMPDUHeadOverhead + EncryptionMPDUTailOverhead;
		MPDUOverhead = EncryptionMPDUTailOverhead;
		tx_desc->NoEnc = 0;
		RT_TRACE(COMP_SEC, DBG_LOUD, ("******We in the loop SecCAMID is %d SecDescAssign is %d The Sec is %d********\n",tx_desc->SecCAMID,tx_desc->SecDescAssign,EncAlg));
		//CamDumpAll(Adapter);
	}
	else
#endif
	{
		//MPDUOverhead = 0;
		tx_desc->NoEnc = 1;
	}
#if 0
	switch(EncAlg){
		case NO_Encryption:
			tx_desc->SecType = 0x0;
			break;
		case WEP40_Encryption:
		case WEP104_Encryption:
			tx_desc->SecType = 0x1;
			break;
		case TKIP_Encryption:
			tx_desc->SecType = 0x2;
			break;
		case AESCCMP_Encryption:
			tx_desc->SecType = 0x3;
			break;
		default:
			tx_desc->SecType = 0x0;
			break;
	}
#else
	tx_desc->SecType = 0x0;
#endif
		if (tcb_desc->bHwSec)
			{
				switch (priv->ieee80211->pairwise_key_type)
				{
					case KEY_TYPE_WEP40:
					case KEY_TYPE_WEP104:
						 tx_desc->SecType = 0x1;
						 tx_desc->NoEnc = 0;
						 break;
					case KEY_TYPE_TKIP:
						 tx_desc->SecType = 0x2;
						 tx_desc->NoEnc = 0;
						 break;
					case KEY_TYPE_CCMP:
						 tx_desc->SecType = 0x3;
						 tx_desc->NoEnc = 0;
						 break;
					case KEY_TYPE_NA:
						 tx_desc->SecType = 0x0;
						 tx_desc->NoEnc = 1;
						 break;
				}
			}

	tx_desc->QueueSelect = MapHwQueueToFirmwareQueue(tcb_desc->queue_index);
	tx_desc->TxFWInfoSize =  sizeof(tx_fwinfo_819x_usb);

	tx_desc->DISFB = tcb_desc->bTxDisableRateFallBack;
	tx_desc->USERATE = tcb_desc->bTxUseDriverAssingedRate;

        // Fill fields that are required to be initialized in all of the descriptors */
        //DWORD 0
#if 0	
        tx_desc->FirstSeg = (tcb_desc->bFirstSeg)? 1:0;
        tx_desc->LastSeg = (tcb_desc->bLastSeg)?1:0;
#else 
        tx_desc->FirstSeg = 1;
        tx_desc->LastSeg = 1;
#endif
        tx_desc->OWN = 1;

#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE	
	if (tcb_desc->drv_agg_enable) {
		tx_desc->TxBufferSize = tcb_desc->pkt_size + sizeof(tx_fwinfo_819x_usb);
	} else
#endif
	{
		//DWORD 2
		tx_desc->TxBufferSize = (u32)(skb->len - USB_HWDESC_HEADER_LEN);
	}
	// Get index to out pipe from specified QueueID */
#ifndef USE_ONE_PIPE
	idx_pipe = txqueue2outpipe(priv,tcb_desc->queue_index);
        OutPipe = priv->RtOutPipes[idx_pipe];
#else
	OutPipe = 0x5;
#endif

	skb->dev = dev;
	tx_info->urb = tx_urb;

	//RT_DEBUG_DATA(COMP_SEND,tx_fwinfo,sizeof(tx_fwinfo_819x_usb));
	//RT_DEBUG_DATA(COMP_SEND,tx_desc,sizeof(tx_desc_819x_usb));

	// To submit bulk urb */
	usb_fill_bulk_urb(tx_urb,udev,
			usb_sndbulkpipe(udev,OutPipe), skb->data,
			skb->len, rtl8192_tx_isr, skb);

	skb_queue_tail(&priv->tx_queue, skb);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	status = usb_submit_urb(tx_urb, GFP_ATOMIC);
#else
	status = usb_submit_urb(tx_urb);
#endif
	if (!status){
//we need to send 0 byte packet whenever 512N bytes/64N(HIGN SPEED/NORMAL SPEED) bytes packet has been transmitted. Otherwise, it will be halt to wait for another packet. WB. 2008.08.27
		bool bSend0Byte = false;
		u8 zero = 0;
		if(udev->speed == USB_SPEED_HIGH)
		{
			if (skb->len > 0 && skb->len % 512 == 0)
				bSend0Byte = true;
		}
		else
		{
			if (skb->len > 0 && skb->len % 64 == 0)
				bSend0Byte = true;
		} 
		if (bSend0Byte)
		{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
			tx_urb_zero = usb_alloc_urb(0,GFP_ATOMIC);
#else
			tx_urb_zero = usb_alloc_urb(0);
#endif
			if(!tx_urb_zero){
				RT_TRACE(COMP_ERR, "can't alloc urb for zero byte\n");
				return -ENOMEM;
			}

			tx_info->zero = tx_urb_zero;

			usb_fill_bulk_urb(tx_urb_zero,udev,
					usb_sndbulkpipe(udev,OutPipe), &zero,
					0, tx_zero_isr, dev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
			status = usb_submit_urb(tx_urb_zero, GFP_ATOMIC);
#else
			status = usb_submit_urb(tx_urb_zero);
#endif
			if (status){
			RT_TRACE(COMP_ERR, "Error TX URB for zero byte %d, error %d", atomic_read(&priv->tx_pending[tcb_desc->queue_index]), status);
			return -1;
			}
		}
		dev->trans_start = jiffies;
		atomic_inc(&priv->tx_pending[tcb_desc->queue_index]);
		return 0;
	}else{
		RT_TRACE(COMP_ERR, "Error TX URB %d, error %d", atomic_read(&priv->tx_pending[tcb_desc->queue_index]),
				status);
		return -1;
	}
}
#endif

#if 0
void rtl8192_set_rate(struct net_device *dev)
{
	int i;
	u16 word;
	int basic_rate,min_rr_rate,max_rr_rate;

//	struct r8192_priv *priv = ieee80211_priv(dev);
	
	//if (ieee80211_is_54g(priv->ieee80211->current_network) && 
//		priv->ieee80211->state == IEEE80211_LINKED){
	basic_rate = ieeerate2rtlrate(240);
	min_rr_rate = ieeerate2rtlrate(60);
	max_rr_rate = ieeerate2rtlrate(240);
	
//	
//	}else{
//		basic_rate = ieeerate2rtlrate(20);
//		min_rr_rate = ieeerate2rtlrate(10);
//		max_rr_rate = ieeerate2rtlrate(110);
//	}

	write_nic_byte(dev, RESP_RATE,
			max_rr_rate<<MAX_RESP_RATE_SHIFT| min_rr_rate<<MIN_RESP_RATE_SHIFT);

	//word  = read_nic_word(dev, BRSR);
	word  = read_nic_word(dev, BRSR_8187);
	word &= ~BRSR_MBR_8185;
		

	for(i=0;i<=basic_rate;i++)
		word |= (1<<i);

	//write_nic_word(dev, BRSR, word);
	write_nic_word(dev, BRSR_8187, word);
	//DMESG("RR:%x BRSR: %x", read_nic_byte(dev,RESP_RATE), read_nic_word(dev,BRSR));
}
#endif


#ifdef RTL8192SU 
void rtl8192SU_net_update(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	struct ieee80211_network *net = &priv->ieee80211->current_network;
	//u16 BcnTimeCfg = 0, BcnCW = 6, BcnIFS = 0xf;
	u16 rate_config = 0;
	u32 regTmp = 0;
	u8 rateIndex = 0;
	u8	retrylimit = 0x30;
	u16 cap = net->capability;

	priv->short_preamble = cap & WLAN_CAPABILITY_SHORT_PREAMBLE;

//HW_VAR_BASIC_RATE
	//update Basic rate: RR, BRSR
	rtl8192_config_rate(dev, &rate_config);	//HalSetBrateCfg
	
#ifdef RTL8192SU_DISABLE_CCK_RATE 
	priv->basic_rate = rate_config  = rate_config & 0x150; // Disable CCK 11M, 5.5M, 2M, and 1M rates.
#else
	priv->basic_rate = rate_config  = rate_config & 0x15f;
#endif

	// Set RRSR rate table.
	write_nic_byte(dev, RRSR, rate_config&0xff);
	write_nic_byte(dev, RRSR+1, (rate_config>>8)&0xff);

	// Set RTS initial rate
	while(rate_config > 0x1)
	{
		rate_config = (rate_config>> 1);
		rateIndex++;
	}
	write_nic_byte(dev, INIRTSMCS_SEL, rateIndex);	
//HW_VAR_BASIC_RATE

	//set ack preample
	regTmp = (priv->nCur40MhzPrimeSC) << 5;
	if (priv->short_preamble)
		regTmp |= 0x80;
	write_nic_byte(dev, RRSR+2, regTmp);

	write_nic_dword(dev,BSSIDR,((u32*)net->bssid)[0]);
	write_nic_word(dev,BSSIDR+4,((u16*)net->bssid)[2]);

	write_nic_word(dev, BCN_INTERVAL, net->beacon_interval);
	//2008.10.24 added by tynli for beacon changed.
	PHY_SetBeaconHwReg( dev, net->beacon_interval);
	
	rtl8192_update_cap(dev, cap);
	
	if (ieee->iw_mode == IW_MODE_ADHOC){
		retrylimit = 7;
		//we should enable ibss interrupt here, but disable it temporarily
		if (0){
			priv->irq_mask |= (IMR_BcnInt | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
			//rtl8192_irq_disable(dev);
			//rtl8192_irq_enable(dev);
		}
	}
	else{		
		if (0){
			priv->irq_mask &= ~(IMR_BcnInt | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
			//rtl8192_irq_disable(dev);
			//rtl8192_irq_enable(dev);
		}
	}	
		
	priv->ShortRetryLimit = priv->LongRetryLimit = retrylimit;
			
	write_nic_word(dev, 	RETRY_LIMIT, 
				retrylimit << RETRY_LIMIT_SHORT_SHIFT | \
				retrylimit << RETRY_LIMIT_LONG_SHIFT);	
}

void rtl8192SU_update_ratr_table(struct net_device* dev)
{
		struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	u8* pMcsRate = ieee->dot11HTOperationalRateSet;
	//struct ieee80211_network *net = &ieee->current_network;
	u32 ratr_value = 0;

	u8 rate_index = 0;
	int WirelessMode = ieee->mode;
	u8 MimoPs = ieee->pHTInfo->PeerMimoPs;

	u8 bNMode = 0;

	rtl8192_config_rate(dev, (u16*)(&ratr_value));
	ratr_value |= (*(u16*)(pMcsRate)) << 12;

	//switch (ieee->mode)
	switch (WirelessMode)
	{
		case IEEE_A:
			ratr_value &= 0x00000FF0;
			break;
		case IEEE_B:
			ratr_value &= 0x0000000D;
			break;
		case IEEE_G:
			ratr_value &= 0x00000FF5;
			break;
		case IEEE_N_24G:
		case IEEE_N_5G:
		{
			bNMode = 1;
			
			if (MimoPs == 0) //MIMO_PS_STATIC
					{
				ratr_value &= 0x0007F005;
			}
			else
			{	// MCS rate only => for 11N mode.
				u32	ratr_mask;
			
				// 1T2R or 1T1R, Spatial Stream 2 should be disabled
				if (	priv->rf_type == RF_1T2R ||
					priv->rf_type == RF_1T1R ||
					(ieee->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_TX_2SS) )
						ratr_mask = 0x000ff005;
					else
						ratr_mask = 0x0f0ff005;

				if((ieee->pHTInfo->bCurTxBW40MHz) &&
				    !(ieee->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_TX_40_MHZ))
					ratr_mask |= 0x00000010; // Set 6MBps

				// Select rates for rate adaptive mechanism.
					ratr_value &= ratr_mask;
					}
			}
			break;
		default:
			if(0)
			{
				if(priv->rf_type == RF_1T2R)	// 1T2R, Spatial Stream 2 should be disabled
				{
				ratr_value &= 0x000ff0f5;				
				}
				else
				{
				ratr_value &= 0x0f0ff0f5;
				}
			}
			//printk("====>%s(), mode is not correct:%x\n", __FUNCTION__, ieee->mode);
			break; 
	}

#ifdef RTL8192SU_DISABLE_CCK_RATE
	ratr_value &= 0x0FFFFFF0;
#else
	ratr_value &= 0x0FFFFFFF;
#endif
	
	// Get MAX MCS available.
	if (   (bNMode && ((ieee->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_SHORT_GI)==0)) &&
		((ieee->pHTInfo->bCurBW40MHz && ieee->pHTInfo->bCurShortGI40MHz) ||
	        (!ieee->pHTInfo->bCurBW40MHz && ieee->pHTInfo->bCurShortGI20MHz)))
	{
		u8 shortGI_rate = 0;
		u32 tmp_ratr_value = 0;
		ratr_value |= 0x10000000;//???
		tmp_ratr_value = (ratr_value>>12);
		for(shortGI_rate=15; shortGI_rate>0; shortGI_rate--)
		{
			if((1<<shortGI_rate) & tmp_ratr_value)
				break;
		}	
		shortGI_rate = (shortGI_rate<<12)|(shortGI_rate<<8)|(shortGI_rate<<4)|(shortGI_rate);
		write_nic_byte(dev, SG_RATE, shortGI_rate);
		//printk("==>SG_RATE:%x\n", read_nic_byte(dev, SG_RATE));
	}
	write_nic_dword(dev, ARFR0+rate_index*4, ratr_value);
	printk("=============>ARFR0+rate_index*4:%#x\n", ratr_value);

	//2 UFWP
	if (ratr_value & 0xfffff000){
		//printk("===>set to N mode\n");
		HalSetFwCmd8192S(dev, FW_CMD_RA_REFRESH_N);
	}
	else	{
		//printk("===>set to B/G mode\n");
		HalSetFwCmd8192S(dev, FW_CMD_RA_REFRESH_BG);
	}
}

void rtl8192SU_link_change(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	u32 reg = 0;

	if(priv->bSurpriseRemoved)
		return;

	printk("=====>%s 1\n", __func__);
	reg = read_nic_dword(dev,RCR);

	if (ieee->state == IEEE80211_LINKED)
	{

		rtl8192SU_net_update(dev);
		rtl8192SU_update_ratr_table(dev);
		ieee->SetFwCmdHandler(dev, FW_CMD_HIGH_PWR_ENABLE);
		priv->ReceiveConfig = reg |= RCR_CBSSID;

	}else{
		priv->ReceiveConfig = reg &= ~RCR_CBSSID;

	}

	write_nic_dword(dev, RCR, reg);
	rtl8192_update_msr(dev);
	
	printk("<=====%s 2\n", __func__);
}
#endif
#ifdef RTL8192U
extern void rtl8192_update_ratr_table(struct net_device* dev);
void rtl8192_link_change(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;

	if(priv->bSurpriseRemoved)
		return;

	//write_nic_word(dev, BCN_INTR_ITV, net->beacon_interval);
	if (ieee->state == IEEE80211_LINKED)
	{
		rtl8192_net_update(dev);
		if(ieee->iw_mode == IW_MODE_INFRA)
			rtl8192_update_ratr_table(dev);
		//add this as in pure N mode, wep encryption will use software way, but there is no chance to set this as wep will not set group key in wext. WB.2008.07.08
		if ((KEY_TYPE_WEP40 == ieee->pairwise_key_type) || (KEY_TYPE_WEP104 == ieee->pairwise_key_type))
		EnableHWSecurityConfig8192(dev);
	}
	//update timing params*/
//	RT_TRACE(COMP_CH, "========>%s(), chan:%d\n", __FUNCTION__, priv->chan);
//	rtl8192_set_chan(dev, priv->chan);
	 if (ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_ADHOC)
        {
                if (priv->ieee80211->state == IEEE80211_LINKED)
                        priv->ReceiveConfig |= RCR_CBSSID;
                else
                        priv->ReceiveConfig &= ~RCR_CBSSID;
                write_nic_dword(dev, RCR, priv->ReceiveConfig);
        }

//	rtl8192_set_rxconf(dev);
}
#endif

static struct ieee80211_qos_parameters def_qos_parameters = {
        {3,3,3,3},// cw_min */
        {7,7,7,7},// cw_max */
        {2,2,2,2},// aifs */ 
        {0,0,0,0},// flags */ 
        {0,0,0,0} // tx_op_limit */
};


#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192_update_beacon(struct work_struct * work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, update_beacon_wq.work);
        struct net_device *dev = priv->ieee80211->dev;
#else
void rtl8192_update_beacon(struct net_device *dev)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
#endif
 	struct ieee80211_device* ieee = priv->ieee80211;
	struct ieee80211_network* net = &ieee->current_network;

	if (ieee->pHTInfo->bCurrentHTSupport)
		HTUpdateSelfAndPeerSetting(ieee, net);
	ieee->pHTInfo->bCurrentRT2RTLongSlotTime = net->bssht.bdRT2RTLongSlotTime;
	// Joseph test for turbo mode with AP
	ieee->pHTInfo->RT2RT_HT_Mode = net->bssht.RT2RT_HT_Mode;
	rtl8192_update_cap(dev, net->capability);
}
//
// background support to run QoS activate functionality
//
int WDCAPARA_ADD[] = {EDCAPARA_BE,EDCAPARA_BK,EDCAPARA_VI,EDCAPARA_VO};
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192_qos_activate(struct work_struct * work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, qos_activate);
        struct net_device *dev = priv->ieee80211->dev;
#else
void rtl8192_qos_activate(struct net_device *dev)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
#endif
        struct ieee80211_qos_parameters *qos_parameters = &priv->ieee80211->current_network.qos_data.parameters;
        u8 mode = priv->ieee80211->current_network.mode;
        //u32 size = sizeof(struct ieee80211_qos_parameters);
	u8  u1bAIFS;
	u32 u4bAcParam;
        int i;
	u8 AcmCtrl = priv->AcmControl;

        if (priv == NULL)
                return;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	down(&priv->mutex);
#else
       mutex_lock(&priv->mutex);
#endif
        if(priv->ieee80211->state != IEEE80211_LINKED)
		goto success;
	RT_TRACE(COMP_QOS,"qos active process with associate response received\n");
	// It better set slot time at first */
	// For we just support b/g mode at present, let the slot time at 9/20 selection */	
	// update the ac parameter to related registers */
	for(i = 0; i <  QOS_QUEUE_NUM; i++) {
		//Mode G/A: slotTimeTimer = 9; Mode B: 20
		u1bAIFS = qos_parameters->aifs[i] * ((mode&(IEEE_G|IEEE_N_24G)) ?9:20) + aSifsTime; 
		u4bAcParam = ((((u32)(qos_parameters->tx_op_limit[i]))<< AC_PARAM_TXOP_LIMIT_OFFSET)|
				(((u32)(qos_parameters->cw_max[i]))<< AC_PARAM_ECW_MAX_OFFSET)|
				(((u32)(qos_parameters->cw_min[i]))<< AC_PARAM_ECW_MIN_OFFSET)|
				((u32)u1bAIFS << AC_PARAM_AIFS_OFFSET));

		write_nic_dword(dev, WDCAPARA_ADD[i], u4bAcParam);
#ifdef WIFI_TEST
		printk("##### %d AcParam = %x \n",i,u4bAcParam);
#endif
		//write_nic_dword(dev, WDCAPARA_ADD[i], 0x005e4322);
	
		AcmCtrl = priv->AcmControl | 0x01;

		if( qos_parameters->flag[i] )
		{ // ACM bit is 1.
			switch(i)
			{
				case AC0_BE:
					AcmCtrl |= AcmHw_BeqEn;
					break;

				case AC2_VI:
					AcmCtrl |= AcmHw_ViqEn;
					break;

				case AC3_VO:
					AcmCtrl |= AcmHw_VoqEn;
					break;

				default:
					//RT_TRACE( COMP_QOS, DBG_LOUD, ("SetHwReg819xUsb(): [HW_VAR_ACM_CTRL] ACM set failed: eACI is %ld\n", eACI ) );
					break;
			}
		}
		else
		{ // ACM bit is 0.
			switch(i)
			{
				case AC0_BE:
					AcmCtrl &= (~AcmHw_BeqEn);
					break;

				case AC2_VI:
					AcmCtrl &= (~AcmHw_ViqEn);
					break;

				case AC3_VO:
					AcmCtrl &= (~AcmHw_BeqEn);
					break;

				default:
					break;
			}
		}

		write_nic_byte(dev, AcmHwCtrl, AcmCtrl );
	}

success:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	up(&priv->mutex);
#else
       mutex_unlock(&priv->mutex);
#endif
}

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl819x_set_mcast_register(struct work_struct * work)
{
    struct r8192_priv *priv = container_of(work, struct r8192_priv, mcast_wq);
#ifndef RTL8192SU
    struct net_device *dev = priv->ieee80211->dev;
#endif
#else
void rtl819x_set_mcast_register(struct net_device *dev)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
#endif
    if (priv == NULL)
        return;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
    down(&priv->mutex);
#else
    mutex_lock(&priv->mutex);
#endif
#ifndef RTL8192SU
    write_nic_dword(dev,MAR0,priv->mc_filter[0]);
    write_nic_dword(dev,MAR4,priv->mc_filter[1]);
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
    up(&priv->mutex);
#else
    mutex_unlock(&priv->mutex);
#endif
}


static int rtl8192_qos_handle_probe_response(struct r8192_priv *priv,
		int active_network,
		struct ieee80211_network *network)
{
	int ret = 0;
	u32 size = sizeof(struct ieee80211_qos_parameters);

	if(priv->ieee80211->state !=IEEE80211_LINKED)
                return ret;

        if ((priv->ieee80211->iw_mode != IW_MODE_INFRA))
                return ret;

	if (network->flags & NETWORK_HAS_QOS_MASK) {
		if (active_network &&
				(network->flags & NETWORK_HAS_QOS_PARAMETERS))
			network->qos_data.active = network->qos_data.supported;

		if ((network->qos_data.active == 1) && (active_network == 1) &&
				(network->flags & NETWORK_HAS_QOS_PARAMETERS) &&
				(network->qos_data.old_param_count !=
				 network->qos_data.param_count)) {
			network->qos_data.old_param_count =
				network->qos_data.param_count;
            priv->ieee80211->wmm_acm = network->qos_data.wmm_acm;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
			queue_work(priv->priv_wq, &priv->qos_activate);
#else
			schedule_task(&priv->qos_activate);
#endif
			RT_TRACE (COMP_QOS, "QoS parameters change call "
					"qos_activate\n");
		}
	} else {
		memcpy(&priv->ieee80211->current_network.qos_data.parameters,\
		       &def_qos_parameters, size);

		if ((network->qos_data.active == 1) && (active_network == 1)) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
			queue_work(priv->priv_wq, &priv->qos_activate);
#else
			schedule_task(&priv->qos_activate);
#endif
			RT_TRACE(COMP_QOS, "QoS was disabled call qos_activate \n");
		}
		network->qos_data.active = 0;
		network->qos_data.supported = 0;
	}

	return 0;
}

// handle manage frame frame beacon and probe response */
static int rtl8192_handle_beacon(struct net_device * dev,
                              struct ieee80211_beacon * beacon,
                              struct ieee80211_network * network)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	rtl8192_qos_handle_probe_response(priv,1,network);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	queue_delayed_work(priv->priv_wq, &priv->update_beacon_wq, 0);
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	schedule_task(&priv->update_beacon_wq);
#else
	queue_work(priv->priv_wq, &priv->update_beacon_wq);
#endif

#endif
	return 0;

}

//
//handling the beaconing responses. if we get different QoS setting
//off the network from the associated setting, adjust the QoS
//setting
//
static int rtl8192_qos_association_resp(struct r8192_priv *priv,
                                    struct ieee80211_network *network)
{
        int ret = 0;
        unsigned long flags;
        u32 size = sizeof(struct ieee80211_qos_parameters);
        int set_qos_param = 0;

        if ((priv == NULL) || (network == NULL))
                return ret;

	if(priv->ieee80211->state !=IEEE80211_LINKED)
                return ret;

        if ((priv->ieee80211->iw_mode != IW_MODE_INFRA))
                return ret;

        spin_lock_irqsave(&priv->ieee80211->lock, flags);
	if(network->flags & NETWORK_HAS_QOS_PARAMETERS) {
		memcpy(&priv->ieee80211->current_network.qos_data.parameters,\
			 &network->qos_data.parameters,\
			sizeof(struct ieee80211_qos_parameters));
		priv->ieee80211->current_network.qos_data.active = 1;
                priv->ieee80211->wmm_acm = network->qos_data.wmm_acm;
#if 0
		if((priv->ieee80211->current_network.qos_data.param_count != \
					network->qos_data.param_count))
#endif
		 {
                        set_qos_param = 1;
			// update qos parameter for current network */
			priv->ieee80211->current_network.qos_data.old_param_count = \
				 priv->ieee80211->current_network.qos_data.param_count;
			priv->ieee80211->current_network.qos_data.param_count = \
			     	 network->qos_data.param_count;
		}
        } else {
		memcpy(&priv->ieee80211->current_network.qos_data.parameters,\
		       &def_qos_parameters, size);
		priv->ieee80211->current_network.qos_data.active = 0;
		priv->ieee80211->current_network.qos_data.supported = 0;
                set_qos_param = 1;
        }

        spin_unlock_irqrestore(&priv->ieee80211->lock, flags);

	RT_TRACE(COMP_QOS, "%s: network->flags = %d,%d\n",__FUNCTION__,network->flags ,priv->ieee80211->current_network.qos_data.active);
        if (set_qos_param == 1){
            dm_init_edca_turbo(priv->ieee80211->dev);//YJ,add,090320, when roaming to another ap, bcurrent_turbo_EDCA should be resetted so that we can update edca tubo.
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
		queue_work(priv->priv_wq, &priv->qos_activate);
#else
		schedule_task(&priv->qos_activate);
#endif
        }


        return ret;
}


static int rtl8192_handle_assoc_response(struct net_device *dev,
                                     struct ieee80211_assoc_response_frame *resp,
                                     struct ieee80211_network *network)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
        rtl8192_qos_association_resp(priv, network);
        return 0;
}


void rtl8192_update_ratr_table(struct net_device* dev)
	//	POCTET_STRING	posLegacyRate,
	//	u8*			pMcsRate)
	//	PRT_WLAN_STA	pEntry)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	u8* pMcsRate = ieee->dot11HTOperationalRateSet;
	//struct ieee80211_network *net = &ieee->current_network;
	u32 ratr_value = 0;
	u8 rate_index = 0;
	rtl8192_config_rate(dev, (u16*)(&ratr_value));
	ratr_value |= (*(u16*)(pMcsRate)) << 12;
//	switch (net->mode)
	switch (ieee->mode)
	{
		case IEEE_A:
			ratr_value &= 0x00000FF0;
			break;
		case IEEE_B:
			ratr_value &= 0x0000000F;
			break;
		case IEEE_G:
			ratr_value &= 0x00000FF7;
			break;
		case IEEE_N_24G:
		case IEEE_N_5G:
			if (ieee->pHTInfo->PeerMimoPs == 0) //MIMO_PS_STATIC
				ratr_value &= 0x0007F007;
			else{
				if (priv->rf_type == RF_1T2R)
					ratr_value &= 0x000FF007;
				else
					ratr_value &= 0x0F81F007;
			}
			break;
		default:
			break; 
	}
	ratr_value &= 0x0FFFFFFF;
	if(ieee->pHTInfo->bCurTxBW40MHz && ieee->pHTInfo->bCurShortGI40MHz){
		ratr_value |= 0x80000000;
	}else if(!ieee->pHTInfo->bCurTxBW40MHz && ieee->pHTInfo->bCurShortGI20MHz){
		ratr_value |= 0x80000000;
	}	
	write_nic_dword(dev, RATR0+rate_index*4, ratr_value);
	write_nic_byte(dev, UFWP, 1);
}
//added by amy for adhoc 090402
#ifdef RTL8192U
bool rtl8192_check_ht_cap(struct net_device* dev, struct sta_info *sta, struct ieee80211_network* net)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	PHT_CAPABILITY_ELE	pHTCapIE = NULL;
	PHT_INFORMATION_ELE	 pPeerHTInfo = NULL;
	u8 ExtChlOffset=0;
	u8	*pMcsFilter = NULL;
	u16	nMaxAMSDUSize = 0;
	static u8	EWC11NHTCap[] = {0x00, 0x90, 0x4c, 0x33};		// For 11n EWC definition, 2007.07.17, by Emily
	static u8 	EWC11NHTInfo[] = {0x00, 0x90, 0x4c, 0x34};	// For 11n EWC definition, 2007.07.17, by Emily
	if((ieee->mode != WIRELESS_MODE_N_24G) && (ieee->mode != WIRELESS_MODE_N_5G))
	{
		printk("%s():i am G mode ,do not need to check Cap IE\n",__FUNCTION__);
		if(net->mode == IEEE_N_5G)
			sta->wireless_mode = IEEE_A;
		else if(net->mode == IEEE_N_24G)
		{
			if(net->rates_ex_len > 0)
				sta->wireless_mode = IEEE_G;
			else
				sta->wireless_mode = IEEE_B;
		}else
			sta->wireless_mode = net->mode;
		return false;
	}
	//2 Reject PEER MP to connect in N mode
	// If we do not support CCK rate in N mode, reject the B mode 
	if((ieee->mode ==WIRELESS_MODE_N_24G) 
		&& ieee->pHTInfo->bRegSuppCCK== false)
	{
		if(net->mode == IEEE_B){
			sta->wireless_mode = net->mode;
			printk("%s(): peer is B MODE return\n", __FUNCTION__);
			return false;
		}
	}
	//2 Set PEER MP Wireless Mode
	if(net->bssht.bdHTCapLen  != 0)
	{
		sta->htinfo.bEnableHT = true;
		sta->htinfo.bCurRxReorderEnable = ieee->pHTInfo->bRegRxReorderEnable;
		if(net->mode == IEEE_A)
			sta->wireless_mode = IEEE_N_5G;
		else
			sta->wireless_mode = IEEE_N_24G;
	}
	else
	{
		printk("%s(): have no HTCap IE, mode is %d\n",__FUNCTION__,net->mode);
		sta->wireless_mode = net->mode;
		sta->htinfo.bEnableHT = false;
		return true;
	}

	if(!memcmp(net->bssht.bdHTCapBuf ,EWC11NHTCap, sizeof(EWC11NHTCap)))
		pHTCapIE = (PHT_CAPABILITY_ELE)(&(net->bssht.bdHTCapBuf[4]));
	else
		pHTCapIE = (PHT_CAPABILITY_ELE)(net->bssht.bdHTCapBuf);

	if(!memcmp(net->bssht.bdHTInfoBuf, EWC11NHTInfo, sizeof(EWC11NHTInfo)))
		pPeerHTInfo = (PHT_INFORMATION_ELE)(&net->bssht.bdHTInfoBuf[4]);
	else		
		pPeerHTInfo = (PHT_INFORMATION_ELE)(net->bssht.bdHTInfoBuf);
	
	ExtChlOffset=((ieee->pHTInfo->bRegBW40MHz == false)?HT_EXTCHNL_OFFSET_NO_EXT:
					(ieee->current_network.channel<=6)?
					HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);
	printk("******** STA wireless mode %d\n", sta->wireless_mode);
		
	//1 Get HT properties of the STA
	// Get CCK rate support property
	if(ieee->pHTInfo->bRegSuppCCK)
		sta->htinfo.bSupportCck = (pHTCapIE->DssCCk==1)?true:false;
	else
	{
		if(pHTCapIE->DssCCk==1)
			return false;
	}

	// Get Mimo power save settings
	sta->htinfo.MimoPs= pHTCapIE->MimoPwrSave;

	printk("******** PEER MP MimoPs %d\n", sta->htinfo.MimoPs);
	// Get operation bandwidth
	if(ieee->pHTInfo->bRegBW40MHz)
		sta->htinfo.bBw40MHz= (pHTCapIE->ChlWidth==1)?true:false;
	else
		sta->htinfo.bBw40MHz = false;

	if((pPeerHTInfo->ExtChlOffset) != ExtChlOffset)
		sta->htinfo.bBw40MHz = false;
	
	printk("******** PEER MP bCurBW40M %d\n", sta->htinfo.bBw40MHz);
	if(ieee->pHTInfo->bRegBW40MHz == true)
		sta->htinfo.bCurTxBW40MHz = sta->htinfo.bBw40MHz;

	printk("******** PEER MP bCurTxBW40MHz %d\n", sta->htinfo.bCurTxBW40MHz);
	sta->htinfo.bCurShortGI20MHz= 
		((ieee->pHTInfo->bRegShortGI20MHz)?((pHTCapIE->ShortGI20Mhz==1)?true:false):false);
	sta->htinfo.bCurShortGI40MHz= 
		((ieee->pHTInfo->bRegShortGI40MHz)?((pHTCapIE->ShortGI40Mhz==1)?true:false):false);
	
	printk("******** PEER MP bCurShortGI20MHz %d, bCurShortGI40MHz %d\n",sta->htinfo.bCurShortGI20MHz,sta->htinfo.bCurShortGI40MHz);
	// Get A-MSDU property
	nMaxAMSDUSize = (pHTCapIE->MaxAMSDUSize==0)?3839:7935;
	if(ieee->pHTInfo->nAMSDU_MaxSize >= nMaxAMSDUSize)	
		sta->htinfo.AMSDU_MaxSize = nMaxAMSDUSize;
	else
		sta->htinfo.AMSDU_MaxSize = ieee->pHTInfo->nAMSDU_MaxSize;

	printk("****************AMSDU_MaxSize=%d\n",sta->htinfo.AMSDU_MaxSize);
	//sta->htinfo.bSupportGreenField = pHtCap->GreenField;
		
	// Get A-MPDU Factor
	// Decide AMPDU Factor according to protocol handshake
	if(ieee->pHTInfo->AMPDU_Factor >= pHTCapIE->MaxRxAMPDUFactor)
		sta->htinfo.AMPDU_Factor = pHTCapIE->MaxRxAMPDUFactor;
	else
		sta->htinfo.AMPDU_Factor = ieee->pHTInfo->AMPDU_Factor;

	// Get AMPDU Density
	if(ieee->pHTInfo->MPDU_Density >= pHTCapIE->MPDUDensity)
		sta->htinfo.MPDU_Density = pHTCapIE->MPDUDensity;
	else
		sta->htinfo.MPDU_Density = ieee->pHTInfo->MPDU_Density;

	// Filter MCS rate set that we support.
	HTFilterMCSRate(ieee, pHTCapIE->MCS, sta->htinfo.McsRateSet);
	//{
	//	int k=0;
	//	printk("***********Peer MCS rate\n");
	//	for(k=0;k<16;k++)
	//		printk("%x ",ext_entry->htinfo.McsRateSet[k]);
	//}
	// This is for static mimo power save
	if(sta->htinfo.MimoPs == 0)  //MIMO_PS_STATIC
		pMcsFilter = MCS_FILTER_1SS;
	else
		pMcsFilter = MCS_FILTER_ALL;

	sta->htinfo.HTHighestOperaRate = HTGetHighestMCSRate(ieee, sta->htinfo.McsRateSet, pMcsFilter);
	printk("******** PEER MP HTHighestOperaRate %x\n",sta->htinfo.HTHighestOperaRate);
	//printf("AP_CheckHTCap(): STA has HT cap, AMSDUSize = %d, AMPDUFactor = %d, MPDU Density = %d\n", 
	//				sta->htinfo.AMSDU_MaxSize, 
	//				sta->htinfo.AMPDU_Factor,
	//				sta->htinfo.MPDU_Density);

	return true;
	
}

void UpdateHalRATRTableIndex(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	u8		bitmap = 0;
	int		i;

	for(i = 0;i < PEER_MAX_ASSOC; i++)
	{
		if(NULL != ieee->peer_assoc_list[i])
		{
			bitmap |= BIT0 << ieee->peer_assoc_list[i]->ratr_index;
		}
	}

	priv->RATRTableBitmap = bitmap;
	return;
}
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192_update_peer_ratr_table_wq(struct work_struct * work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
	struct ieee80211_device *ieee = container_of(dwork, struct ieee80211_device, update_assoc_sta_info_wq);
        struct net_device *dev = ieee->dev;
        struct r8192_priv *priv = ieee80211_priv(dev);
#else
void rtl8192_update_peer_ratr_table_wq(struct net_device *dev)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
#endif
	int idx = 0;
	for(idx=0; idx<PEER_MAX_ASSOC; idx++)
	{	
		if(NULL != priv->ieee80211->peer_assoc_list[idx])
		{
			u8 * addr = priv->ieee80211->peer_assoc_list[idx]->macaddr;
			printk("%s: STA:%x:%x:%x:%x:%x:%x\n",__FUNCTION__,addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
			rtl8192_update_peer_ratr_table(dev,priv->ieee80211->peer_assoc_list[idx]->htinfo.McsRateSet,priv->ieee80211->peer_assoc_list[idx]);
			//HalUsbRxAggr819xUsb(dev);

		}
	}
	UpdateHalRATRTableIndex(dev);
}
u8 GetFreeRATRIndex819xUsb (struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 bitmap = priv->RATRTableBitmap;
	u8 ratr_index = 0;
	
	for(	;ratr_index<7; ratr_index++)
	{
		if((bitmap & BIT0) == 0)
		{
			priv->RATRTableBitmap |= BIT0<<ratr_index;
			return ratr_index;
		}
		bitmap = bitmap >>1;
	}
	if(ratr_index != 7)
		return ratr_index;
	else
		return 8;
}

void rtl8192_update_peer_ratr_table(struct net_device* dev,u8* pMcsRate,struct sta_info* pEntry)
{
	u32		ratr_value = 0;
	u8		ratr_index = 0;
	u8		MimoPs;
	u32		RateSet = 0;
	u32		currentRate = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	//PRT_HIGH_THROUGHPUT	pHTInfo = priv->ieee80211->pHTInfo;
	WIRELESS_MODE			WirelessMode;
	if(pEntry->ratr_index != 8)
		ratr_index = pEntry->ratr_index;
	else
	{
		ratr_index = GetFreeRATRIndex819xUsb(dev);
		pEntry->ratr_index = ratr_index;
	}
	if((ratr_index == 7) || (ratr_index == 8))
	{
		printk("Ratrtable are full\n");
		return;
	}
	//currentRate = read_nic_dword(dev, RATR0, ratr_value);
	currentRate = priv->ieee80211->currentRate;
	MimoPs = pEntry->htinfo.MimoPs;
	WirelessMode = pEntry->wireless_mode;
	printk("%s:WirelessMode is %d,pMcsRate is %x\n",__FUNCTION__,WirelessMode,*pMcsRate);
	RateSet = 0x00000FFF;  // all cck and ofdm rates
	RateSet |= ( (*((u16*)(pMcsRate))) )<< 12;
	ratr_value = RateSet;
//	switch (net->mode)
	switch (WirelessMode)
	{
		case IEEE_A:
			ratr_value &= 0x00000FF0;
			break;
		case IEEE_B:
			ratr_value &= 0x0000000F;
			break;
		case IEEE_G:
			ratr_value &= 0x00000FF7;
			break;
		case IEEE_N_24G:
		case IEEE_N_5G:
			if (MimoPs == MIMO_PS_STATIC)
				ratr_value &= 0x0007F007;
			else{
				if (priv->rf_type == RF_1T2R)
					ratr_value &= 0x000FF007;
				else
					ratr_value &= 0x0F81F007;
			}
			break;
		default:
			if(priv->rf_type == RF_1T2R)	// 1T2R, Spatial Stream 2 should be disabled
			{
				ratr_value &= 0x000FF007;
				//printk("SetHalRATRTable8190Pci() = 0x0F81F007 ==> 0x%x", ratr_value);
			}
			else
				ratr_value &= 0x0F81F007;
			break; 
	}
	ratr_value &= 0x0FFFFFFF;
	if(pEntry->htinfo.bCurTxBW40MHz && priv->ieee80211->pHTInfo->bCurShortGI40MHz && pEntry->htinfo.bCurShortGI40MHz){
		ratr_value |= 0x80000000;
	}else if(!pEntry->htinfo.bCurTxBW40MHz && priv->ieee80211->pHTInfo->bCurShortGI20MHz
			&& pEntry->htinfo.bCurShortGI20MHz && ((WirelessMode == IEEE_N_24G) || (WirelessMode == IEEE_N_5G))){
		ratr_value |= 0x80000000;
	}	
	printk("%s: ratr_index=%d ratr_value=0x%x\n",__FUNCTION__, ratr_index, ratr_value);
	pEntry->ratr_index = ratr_index;
	write_nic_dword(dev, RATR0+ratr_index*4, ratr_value);
	write_nic_byte(dev, UFWP, 1);
}
#endif
//added by amy for adhoc 090402 end

bool is_ap_in_wep_tkip(struct net_device*dev)
{
        static u8 ccmp_ie[4] = {0x00,0x50,0xf2,0x04};
        static u8 ccmp_rsn_ie[4] = {0x00, 0x0f, 0xac, 0x04};
        struct r8192_priv* priv = ieee80211_priv(dev);
        struct ieee80211_device* ieee = priv->ieee80211;
        int wpa_ie_len= ieee->wpa_ie_len;
        struct ieee80211_crypt_data* crypt;
        int encrypt;

        crypt = ieee->crypt[ieee->tx_keyidx];
        encrypt = (ieee->current_network.capability & WLAN_CAPABILITY_PRIVACY) ||\
                  (ieee->host_encrypt && crypt && crypt->ops && \
                   (0 == strcmp(crypt->ops->name,"WEP")));

        // simply judge  */
        if(encrypt && (wpa_ie_len == 0)) {
                // wep encryption, no N mode setting */
                return true;
        } else if((wpa_ie_len != 0)) {
                // parse pairwise key type */
                if (((ieee->wpa_ie[0] == 0xdd) && (!memcmp(&(ieee->wpa_ie[14]),ccmp_ie,4))) || ((ieee->wpa_ie[0] == 0x30) && (!memcmp(&ieee->wpa_ie[10],ccmp_rsn_ie, 4))))
                        return false;
                else
                        return true;
        } else {
                //RT_TRACE(COMP_ERR,"In %s The GroupEncAlgorithm is [4]\n",__FUNCTION__ );
                return false;
        }

return false;
}

static u8 ccmp_ie[4] = {0x00,0x50,0xf2,0x04};
static u8 ccmp_rsn_ie[4] = {0x00, 0x0f, 0xac, 0x04};
bool GetNmodeSupportBySecCfg8192(struct net_device*dev)
{
#if 1
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	struct ieee80211_network * network = &ieee->current_network;
        int wpa_ie_len= ieee->wpa_ie_len;
        struct ieee80211_crypt_data* crypt;
        int encrypt;

#ifdef RTL8192SU
#if	(RTL8192SU_FPGA_2MAC_VERIFICATION||RTL8192SU_ASIC_VERIFICATION)
	return TRUE;
#endif
#endif
                             
        crypt = ieee->crypt[ieee->tx_keyidx];
	//we use connecting AP's capability instead of only security config on our driver to distinguish whether it should use N mode or G mode
        encrypt = (network->capability & WLAN_CAPABILITY_PRIVACY) || (ieee->host_encrypt && crypt && crypt->ops && (0 == strcmp(crypt->ops->name,"WEP")));

	// simply judge  */
	if(encrypt && (wpa_ie_len == 0)) {
		// wep encryption, no N mode setting */
		return false;
//	} else if((wpa_ie_len != 0)&&(memcmp(&(ieee->wpa_ie[14]),ccmp_ie,4))) {
	} else if((wpa_ie_len != 0)) {
		// parse pairwise key type */
		//if((pairwisekey = WEP40)||(pairwisekey = WEP104)||(pairwisekey = TKIP)) 
		if (((ieee->wpa_ie[0] == 0xdd) && (!memcmp(&(ieee->wpa_ie[14]),ccmp_ie,4))) || ((ieee->wpa_ie[0] == 0x30) && (!memcmp(&ieee->wpa_ie[10],ccmp_rsn_ie, 4))))
			return true;
		else
			return false;
	} else {
		return true;
	}
	
#if 0
        //In here we discuss with SD4 David. He think we still can send TKIP in broadcast group key in MCS rate.
        //We can't force in G mode if Pairwie key is AES and group key is TKIP
        if((pSecInfo->GroupEncAlgorithm == WEP104_Encryption) || (pSecInfo->GroupEncAlgorithm == WEP40_Encryption)  ||
           (pSecInfo->PairwiseEncAlgorithm == WEP104_Encryption) ||
           (pSecInfo->PairwiseEncAlgorithm == WEP40_Encryption) || (pSecInfo->PairwiseEncAlgorithm == TKIP_Encryption))
        {
                return  false;
        }
        else
                return true;
#endif
	return true;
#endif
}

bool GetHalfNmodeSupportByAPs819xUsb(struct net_device* dev)
{
	bool			Reval;
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	
// 	Added by Roger, 2008.08.29.	
#ifdef RTL8192SU
	return false;
#endif

	if(ieee->bHalfWirelessN24GMode == true)
		Reval = true;
	else
		Reval =  false;

	return Reval;
}

void rtl8192_refresh_supportrate(struct r8192_priv* priv)
{
	struct ieee80211_device* ieee = priv->ieee80211;
	//we donot consider set support rate for ABG mode, only HT MCS rate is set here.
	if (ieee->mode == WIRELESS_MODE_N_24G || ieee->mode == WIRELESS_MODE_N_5G)
	{
		memcpy(ieee->Regdot11HTOperationalRateSet, ieee->RegHTSuppRateSet, 16);
#ifdef RTL8192SU
		if(priv->rf_type == RF_1T1R) {
			ieee->Regdot11HTOperationalRateSet[1] = 0;
		}

		if(priv->ieee80211->b1SSSupport == true) {
			ieee->Regdot11HTOperationalRateSet[1] = 0;
		}
#endif
		//RT_DEBUG_DATA(COMP_INIT, ieee->RegHTSuppRateSet, 16);
		//RT_DEBUG_DATA(COMP_INIT, ieee->Regdot11HTOperationalRateSet, 16);
	}
	else
		memset(ieee->Regdot11HTOperationalRateSet, 0, 16);
	return;
}

u8 rtl8192_getSupportedWireleeMode(struct net_device*dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 ret = 0;
	switch(priv->rf_chip)
	{
		case RF_8225:
		case RF_8256:
		case RF_PSEUDO_11N:
		case RF_6052:
			ret = (WIRELESS_MODE_N_24G|WIRELESS_MODE_G|WIRELESS_MODE_B);
			break;
		case RF_8258:
			ret = (WIRELESS_MODE_A|WIRELESS_MODE_N_5G);
			break;
		default:
			ret = WIRELESS_MODE_B;
			break;
	}
	return ret;
}
void rtl8192_SetWirelessMode(struct net_device* dev, u8 wireless_mode)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 bSupportMode = rtl8192_getSupportedWireleeMode(dev);

#if 1  
	if ((wireless_mode == WIRELESS_MODE_AUTO) || ((wireless_mode&bSupportMode)==0))
	{
		if(bSupportMode & WIRELESS_MODE_N_24G)
		{
			wireless_mode = WIRELESS_MODE_N_24G;
		}
		else if(bSupportMode & WIRELESS_MODE_N_5G)
		{
			wireless_mode = WIRELESS_MODE_N_5G;
		}
		else if((bSupportMode & WIRELESS_MODE_A))
		{
			wireless_mode = WIRELESS_MODE_A;
		}
		else if((bSupportMode & WIRELESS_MODE_G))
		{
			wireless_mode = WIRELESS_MODE_G;
		}
		else if((bSupportMode & WIRELESS_MODE_B))
		{
			wireless_mode = WIRELESS_MODE_B;
		}
		else{
			RT_TRACE(COMP_ERR, "%s(), No valid wireless mode supported, SupportedWirelessMode(%x)!!!\n", __FUNCTION__,bSupportMode);
			wireless_mode = WIRELESS_MODE_B;
		}
	}
#ifdef TO_DO_LIST //// TODO: this function doesn't work well at this time, we shoud wait for FPGA
	ActUpdateChannelAccessSetting( pAdapter, pHalData->CurrentWirelessMode, &pAdapter->MgntInfo.Info8185.ChannelAccessSetting );
#endif
#ifdef RTL8192SU 
	//LZM 090306 usb crash here, mark it temp
	//write_nic_word(dev, SIFS_OFDM, 0x0e0e);
#endif
	priv->ieee80211->mode = wireless_mode;
	
	if ((wireless_mode == WIRELESS_MODE_N_24G) ||  (wireless_mode == WIRELESS_MODE_N_5G))
		priv->ieee80211->pHTInfo->bEnableHT = 1;	
	else
		priv->ieee80211->pHTInfo->bEnableHT = 0;
	RT_TRACE(COMP_INIT, "Current Wireless Mode is %x\n", wireless_mode);
	rtl8192_refresh_supportrate(priv);
#endif	

}


short rtl8192_is_tx_queue_empty(struct net_device *dev)
{
	int i=0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	//struct ieee80211_device* ieee = priv->ieee80211;
	for (i=0; i<=MGNT_QUEUE; i++)
	{
		if ((i== TXCMD_QUEUE) || (i == HCCA_QUEUE) )
			continue;
		if (atomic_read(&priv->tx_pending[i]))
		{
			printk("===>tx queue is not empty:%d, %d\n", i, atomic_read(&priv->tx_pending[i]));
			return 0;
		}
	}
	return 1;
}
#if 0	
void rtl8192_rq_tx_ack(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	priv->ieee80211->ack_tx_to_ieee = 1;
}
#endif
void rtl8192_hw_sleep_down(struct net_device *dev)
{
	RT_TRACE(COMP_POWER, "%s()============>come to sleep down\n", __FUNCTION__);
#ifdef TODO	
//	MgntActSet_RF_State(dev, eRfSleep, RF_CHANGE_BY_PS);
#endif
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_hw_sleep_wq (struct work_struct *work)
{
//      struct r8180_priv *priv = container_of(work, struct r8180_priv, watch_dog_wq);
//      struct ieee80211_device * ieee = (struct ieee80211_device*)
//                                             container_of(work, struct ieee80211_device, watch_dog_wq);
        struct delayed_work *dwork = container_of(work,struct delayed_work,work);
        struct ieee80211_device *ieee = container_of(dwork,struct ieee80211_device,hw_sleep_wq);
        struct net_device *dev = ieee->dev;
#else
void rtl8192_hw_sleep_wq(struct net_device* dev)
{
#endif
	//printk("=========>%s()\n", __FUNCTION__);
        rtl8192_hw_sleep_down(dev);
}
//	printk("dev is %d\n",dev);	
//	printk("&*&(^*(&(&=========>%s()\n", __FUNCTION__);
void rtl8192_hw_wakeup(struct net_device* dev)
{
//	u32 flags = 0;

//	spin_lock_irqsave(&priv->ps_lock,flags);
	RT_TRACE(COMP_POWER, "%s()============>come to wake up\n", __FUNCTION__);
#ifdef TODO	
//	MgntActSet_RF_State(dev, eRfSleep, RF_CHANGE_BY_PS);
#endif
	//FIXME: will we send package stored while nic is sleep?
//	spin_unlock_irqrestore(&priv->ps_lock,flags);
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_hw_wakeup_wq (struct work_struct *work)
{
//	struct r8180_priv *priv = container_of(work, struct r8180_priv, watch_dog_wq);
//	struct ieee80211_device * ieee = (struct ieee80211_device*)
//	                                       container_of(work, struct ieee80211_device, watch_dog_wq);
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
	struct ieee80211_device *ieee = container_of(dwork,struct ieee80211_device,hw_wakeup_wq);  
	struct net_device *dev = ieee->dev;
#else
void rtl8192_hw_wakeup_wq(struct net_device* dev)
{
#endif
	rtl8192_hw_wakeup(dev);

}

#define MIN_SLEEP_TIME 50
#define MAX_SLEEP_TIME 10000
void rtl8192_hw_to_sleep(struct net_device *dev, u32 th, u32 tl)
{

	struct r8192_priv *priv = ieee80211_priv(dev);

	u32 rb = jiffies;
	unsigned long flags;
	
	spin_lock_irqsave(&priv->ps_lock,flags);
	
	// Writing HW register with 0 equals to disable
	// the timer, that is not really what we want
	// 
	tl -= MSECS(4+16+7);
	 
	//if(tl == 0) tl = 1;
	
	// FIXME HACK FIXME HACK */
//	force_pci_posting(dev);
	//mdelay(1);
	
//	rb = read_nic_dword(dev, TSFTR);
	
	// If the interval in witch we are requested to sleep is too
	// short then give up and remain awake
	// 
	if(((tl>=rb)&& (tl-rb) <= MSECS(MIN_SLEEP_TIME))
		||((rb>tl)&& (rb-tl) < MSECS(MIN_SLEEP_TIME))) {
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		printk("too short to sleep\n");
		return;
	}	
		
//	write_nic_dword(dev, TimerInt, tl);
//	rb = read_nic_dword(dev, TSFTR);
	{
		u32 tmp = (tl>rb)?(tl-rb):(rb-tl);
	//	if (tl<rb)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		schedule_task(&priv->ieee80211->hw_wakeup_wq);
#else
		queue_delayed_work(priv->ieee80211->wq, &priv->ieee80211->hw_wakeup_wq, tmp); //as tl may be less than rb
#endif
	}
	// if we suspect the TimerInt is gone beyond tl 
	// while setting it, then give up
	// 
#if 1
	if(((tl > rb) && ((tl-rb) > MSECS(MAX_SLEEP_TIME)))||
		((tl < rb) && ((rb-tl) > MSECS(MAX_SLEEP_TIME)))) {
		printk("========>too long to sleep:%x, %x, %lx\n", tl, rb,  MSECS(MAX_SLEEP_TIME));
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		return;
	}
#endif	
//	if(priv->rf_sleep)
//		priv->rf_sleep(dev);
	
	//printk("<=========%s()\n", __FUNCTION__);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	schedule_task(&priv->ieee80211->hw_sleep_wq);
#else
	queue_delayed_work(priv->ieee80211->wq, (void *)&priv->ieee80211->hw_sleep_wq,0);
#endif
	spin_unlock_irqrestore(&priv->ps_lock,flags);	
}
//init priv variables here. only non_zero value should be initialized here.
static void rtl8192_init_priv_variable(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 i;
	priv->card_8192 = NIC_8192U;
	priv->card_8192_version = 0;
	priv->chan = 1; //set to channel 1
	priv->RegChannelPlan = 0xf;
	//priv->ieee80211->mode = WIRELESS_MODE_G; //SET AUTO 
	priv->ieee80211->mode = WIRELESS_MODE_AUTO; //SET AUTO 
	priv->ieee80211->iw_mode = IW_MODE_INFRA;
	priv->ieee80211->ieee_up=0;
	priv->retry_rts = DEFAULT_RETRY_RTS;
	priv->retry_data = DEFAULT_RETRY_DATA;
	priv->ieee80211->rts = DEFAULT_RTS_THRESHOLD;
	priv->ieee80211->rate = 110; //11 mbps
	priv->ieee80211->short_slot = 1;
	priv->promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	priv->CckPwEnl = 6;
	//for silent reset
	priv->IrpPendingCount = 1;
	priv->ResetProgress = RESET_TYPE_NORESET;
	priv->bForcedSilentReset = 0;
	priv->bDisableNormalResetCheck = false;
	priv->force_reset = false;
	priv->bSurpriseRemoved = false;
	//added by amy for adhoc 090402
#ifdef RTL8192U
	for(i = 0; i<PEER_MAX_ASSOC; i++)
		priv->ieee80211->peer_assoc_list[i]=NULL;
	priv->RATRTableBitmap = 0;
#endif
	//added by amy for adhoc 090402 end
	priv->ieee80211->currentRate = 0xffffffff;//added by amy 090401 for adhoc
	//added by amy for power save
	priv->bHwRadioOff = false;
	priv->RegRfOff = 0;
	priv->ieee80211->RfOffReason = 0;
	priv->RFChangeInProgress = false;
	priv->bHwRfOffAction = 0;
	priv->SetRFPowerStateInProgress = false;
	priv->ieee80211->PowerSaveControl.bInactivePs = true;
	priv->ieee80211->PowerSaveControl.bIPSModeBackup = false;

	//YJ,add,090409
	priv->ieee80211->amsdu_in_process = 0;

	priv->ieee80211->FwRWRF = 0; 	//we don't use FW read/write RF until stable firmware is available.
	priv->ieee80211->current_network.beacon_interval = DEFAULT_BEACONINTERVAL;	
	priv->ieee80211->iw_mode = IW_MODE_INFRA;
	priv->ieee80211->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE |
		IEEE_SOFTMAC_BEACONS;//added by amy 080604 //|  //IEEE_SOFTMAC_SINGLE_QUEUE;
	
	priv->ieee80211->active_scan = 1;
	priv->ieee80211->modulation = IEEE80211_CCK_MODULATION | IEEE80211_OFDM_MODULATION;
	priv->ieee80211->host_encrypt = 1;
	priv->ieee80211->host_decrypt = 1;
	priv->ieee80211->start_send_beacons = NULL;//rtl819xusb_beacon_tx;//-by amy 080604
	priv->ieee80211->stop_send_beacons = NULL;//rtl8192_beacon_stop;//-by amy 080604
	priv->ieee80211->softmac_hard_start_xmit = rtl8192_hard_start_xmit;
	priv->ieee80211->set_chan = rtl8192_set_chan;
	priv->ieee80211->link_change = priv->ops->rtl819x_link_change;
	priv->ieee80211->softmac_data_hard_start_xmit = rtl8192_hard_data_xmit;
	priv->ieee80211->data_hard_stop = rtl8192_data_hard_stop;
	priv->ieee80211->data_hard_resume = rtl8192_data_hard_resume;
	priv->ieee80211->fts = DEFAULT_FRAG_THRESHOLD;
	priv->ieee80211->check_nic_enough_desc = check_nic_enough_desc;	
	priv->ieee80211->tx_headroom = TX_PACKET_SHIFT_BYTES;
	priv->ieee80211->qos_support = 1;

	priv->ieee80211->PowerSaveControl.bLeisurePs = false;//added by amy for Leisure PS 090402
	priv->ieee80211->PowerSaveControl.bFwCtrlLPS = false;//added by amy for Leisure PS 090402

	//added by WB
//	priv->ieee80211->SwChnlByTimerHandler = rtl8192_phy_SwChnl;
	priv->ieee80211->SetBWModeHandler = rtl8192_SetBWMode;
	priv->ieee80211->handle_assoc_response = rtl8192_handle_assoc_response;
	priv->ieee80211->handle_beacon = rtl8192_handle_beacon;
	//for LPS
	priv->ieee80211->sta_wake_up = rtl8192_hw_wakeup;
//	priv->ieee80211->ps_request_tx_ack = rtl8192_rq_tx_ack;
	priv->ieee80211->enter_sleep_state = rtl8192_hw_to_sleep;
	priv->ieee80211->ps_is_queue_empty = rtl8192_is_tx_queue_empty;
	//added by david
	priv->ieee80211->GetNmodeSupportBySecCfg = GetNmodeSupportBySecCfg8192;
	priv->ieee80211->GetHalfNmodeSupportByAPsHandler = GetHalfNmodeSupportByAPs819xUsb;
	priv->ieee80211->SetWirelessMode = rtl8192_SetWirelessMode;
	//added by amy
	priv->ieee80211->InitialGainHandler = priv->ops->rtl819x_initial_gain;
#ifdef RTL8192U
	priv->ieee80211->check_ht_cap = rtl8192_check_ht_cap;//added by amy for adhoc 090402
	priv->ieee80211->Adhoc_InitRateAdaptive = Adhoc_InitRateAdaptive;//added by amy for adhoc 090403
#endif
	priv->card_type = USB;

	priv->ieee80211->MaxMssDensity = 0;//added by amy 090330
	priv->ieee80211->MinSpaceCfg = 0;//added by amy 090330
	
	//added by lzm for LED 
#ifdef RTL8192SU
	priv->ieee80211->SetHwRegHandler = SetHwReg8192SU;
	priv->ieee80211->LedControlHandler = LedControl8192SUsb;
	priv->ieee80211->is_ap_in_wep_tkip = is_ap_in_wep_tkip;
#ifdef USB_RX_AGGREGATION_SUPPORT
	priv->ieee80211->HalUsbRxAggrHandler = rtl8192SU_HalUsbRxAggr8192SUsb;
#endif
#ifdef ENABLE_IPS
	priv->ieee80211->ieee80211_ips_leave = NULL;//ieee80211_ips_leave;//added by amy for IPS 090331
#endif
#ifdef ENABLE_LPS
	priv->ieee80211->LeisurePSLeave = NULL;//LeisurePSLeave;
#endif
#else
#ifdef USB_AGGR_BULKIN
	priv->bRegUsbAggrBulkinEnable = 0;
#endif
	priv->ieee80211->SetHwRegHandler = SetHwReg819xUsb;
	priv->ieee80211->LedControlHandler = NULL;
	priv->ieee80211->is_ap_in_wep_tkip = NULL;
#ifdef ENABLE_IPS
	priv->ieee80211->ieee80211_ips_leave = NULL;//ieee80211_ips_leave;//added by amy for IPS 090331
#endif
#ifdef ENABLE_LPS
	priv->ieee80211->LeisurePSLeave = NULL;//LeisurePSLeave;
#endif
#endif

#ifdef USB_RX_AGGREGATION_SUPPORT
	priv->bForcedUsbRxAggr = false;
#endif

#ifdef RTL8192SU
//1 RTL8192SU/
	priv->ieee80211->current_network.beacon_interval = DEFAULT_BEACONINTERVAL;
	priv->ieee80211->SetFwCmdHandler = HalSetFwCmd8192S;
	priv->ieee80211->ScanOperationBackupHandler = PHY_ScanOperationBackup8192S;
	priv->bRFSiOrPi = 0;//o=si,1=pi;
	//lzm add 
	priv->bInHctTest = false;
	
	priv->MidHighPwrTHR_L1 = 0x3B;
	priv->MidHighPwrTHR_L2 = 0x40;
	
	if(priv->bInHctTest)
  	{
		priv->ShortRetryLimit = HAL_RETRY_LIMIT_AP_ADHOC;
		priv->LongRetryLimit = HAL_RETRY_LIMIT_AP_ADHOC;
  	}
	else
	{
		priv->ShortRetryLimit = HAL_RETRY_LIMIT_INFRA;
		priv->LongRetryLimit = HAL_RETRY_LIMIT_INFRA;
	}

	priv->SetFwCmdInProgress = false; //is set FW CMD in Progress? 92S only
	priv->CurrentFwCmdIO = 0;

	priv->EarlyRxThreshold = 7;
	priv->pwrGroupCnt = 0;
	priv->enable_gpio0 = 0;

#if ((RTL8192SU_FPGA_2MAC_VERIFICATION != 1) && (RTL8192SU_ASIC_VERIFICATION !=1))
	priv->TransmitConfig	=		
				((u32)7<<TCR_MXDMA_OFFSET) |	// Max DMA Burst Size per Tx DMA Burst, 7: reservied.
				(priv->ShortRetryLimit<<TCR_SRL_OFFSET) |	// Short retry limit
				(priv->LongRetryLimit<<TCR_LRL_OFFSET) |	// Long retry limit
				(false ? TCR_SAT : 0);	// FALSE: HW provies PLCP length and LENGEXT, TURE: SW proiveds them
#endif
	if(priv->bInHctTest)							
		priv->ReceiveConfig	=	//priv->CSMethod |
								RCR_AMF | RCR_ADF |	//RCR_AAP | 	//accept management/data
									RCR_ACF |RCR_APPFCS|						//accept control frame for SW AP needs PS-poll, 2005.07.07, by rcnjko.
								RCR_AB | RCR_AM | RCR_APM |		//accept BC/MC/UC
								RCR_AICV | RCR_ACRC32 | 		//accept ICV/CRC error packet
								RCR_APP_PHYST_STAFF | RCR_APP_PHYST_RXFF |	// Accept PHY status
								((u32)7<<RCR_MXDMA_OFFSET) | // Max DMA Burst Size per Rx DMA Burst, 7: unlimited.
								(priv->EarlyRxThreshold<<RCR_FIFO_OFFSET) | // Rx FIFO Threshold, 7: No Rx threshold.
								(priv->EarlyRxThreshold == 7 ? RCR_OnlyErlPkt:0);
	else
		priv->ReceiveConfig	=	//priv->CSMethod |
									RCR_AMF | RCR_ADF | RCR_AB | 
									RCR_AM | RCR_APM |RCR_AAP |RCR_ADD3|RCR_APP_ICV|
								RCR_APP_PHYST_STAFF | RCR_APP_PHYST_RXFF |	// Accept PHY status
									RCR_APP_MIC | RCR_APPFCS;

	// <Roger_EXP> 2008.06.16.
	priv->IntrMask 		= 	(u16)(IMR_ROK | IMR_VODOK | IMR_VIDOK | IMR_BEDOK | IMR_BKDOK |		\
								IMR_HCCADOK | IMR_MGNTDOK | IMR_COMDOK | IMR_HIGHDOK | 					\
								IMR_BDOK | IMR_RXCMDOK | /*IMR_TIMEOUT0 |*/ IMR_RDU | IMR_RXFOVW	|			\
								IMR_TXFOVW | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);

//1 End

#endif
#ifdef RTL8192U

#ifdef TO_DO_LIST
	if(priv->bInHctTest)
  	{
		priv->ShortRetryLimit = 7;
		priv->LongRetryLimit = 7;
  	}
#endif
	{
		priv->ShortRetryLimit = 0x30;
		priv->LongRetryLimit = 0x30;
	}
	priv->EarlyRxThreshold = 7;
	priv->enable_gpio0 = 0;
	priv->TransmitConfig = 
	//	TCR_DurProcMode |	//for RTL8185B, duration setting by HW
	//?	TCR_DISReqQsize |
                (TCR_MXDMA_2048<<TCR_MXDMA_OFFSET)|  // Max DMA Burst Size per Tx DMA Burst, 7: reservied.
		(priv->ShortRetryLimit<<TCR_SRL_OFFSET)|	// Short retry limit
		(priv->LongRetryLimit<<TCR_LRL_OFFSET) |	// Long retry limit
		(false ? TCR_SAT: 0);	// FALSE: HW provies PLCP length and LENGEXT, TURE: SW proiveds them
#ifdef TO_DO_LIST
	if(priv->bInHctTest)
		priv->ReceiveConfig	=
						RCR_AMF | RCR_ADF |	//RCR_AAP | 	//accept management/data
						//guangan200710
						RCR_ACF |	//accept control frame for SW AP needs PS-poll, 2005.07.07, by rcnjko.
						RCR_AB | RCR_AM | RCR_APM |		//accept BC/MC/UC
						RCR_AICV | RCR_ACRC32 | 		//accept ICV/CRC error packet
						((u32)7<<RCR_MXDMA_OFFSET) | // Max DMA Burst Size per Rx DMA Burst, 7: unlimited.
						(priv->EarlyRxThreshold<<RCR_FIFO_OFFSET) | // Rx FIFO Threshold, 7: No Rx threshold.
						(priv->EarlyRxThreshold == 7 ? RCR_OnlyErlPkt:0);
	else
			
#endif
	priv->ReceiveConfig	=
		RCR_AMF | RCR_ADF |		//accept management/data
		//RCR_ACF |			//accept control frame for SW AP needs PS-poll, 2005.07.07, by rcnjko.
		RCR_AB | RCR_AM | RCR_APM |	//accept BC/MC/UC
		RCR_AICV |/*RCR_ACRC32 |*/ 	//accept ICV/CRC error packet
		((u32)7<<RCR_MXDMA_OFFSET)| // Max DMA Burst Size per Rx DMA Burst, 7: unlimited.
		(priv->EarlyRxThreshold<<RX_FIFO_THRESHOLD_SHIFT) | // Rx FIFO Threshold, 7: No Rx threshold.
		(priv->EarlyRxThreshold == 7 ? RCR_ONLYERLPKT:0);
#endif

	priv->AcmControl = 0;	
#ifdef RTK_DMP_PLATFORM
	priv->pFirmware = (rt_firmware*)dvr_malloc(sizeof(rt_firmware));
#else
	priv->pFirmware = (rt_firmware*)vmalloc(sizeof(rt_firmware));
#endif
	if (priv->pFirmware)
		memset(priv->pFirmware, 0, sizeof(rt_firmware));
	else
		printk("rt_firmware alloc failed!\n");

	// rx related queue */
        skb_queue_head_init(&priv->rx_queue);
	skb_queue_head_init(&priv->rx_skb_queue);

	// Tx related queue */
	skb_queue_head_init(&priv->tx_queue);
	skb_queue_head_init(&priv->tx_skb_queue);
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->ieee80211->skb_waitQ [i]);
	}
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->ieee80211->skb_aggQ [i]);
	}
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->ieee80211->skb_drv_aggQ [i]);
	}
	priv->rf_set_chan = rtl8192_phy_SwChnl;	
}	

//init lock here
static void rtl8192_init_priv_lock(struct r8192_priv* priv)
{
	spin_lock_init(&priv->tx_lock);
	spin_lock_init(&priv->irq_lock);//added by thomas
	//spin_lock_init(&priv->rf_lock);//use rf_sem, or will crash in some OS.
	sema_init(&priv->wx_sem,1);
	sema_init(&priv->rf_sem,1);
	spin_lock_init(&priv->ps_lock);
	spin_lock_init(&priv->rf_ps_lock);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	sema_init(&priv->mutex, 1);
#else
	mutex_init(&priv->mutex);
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
extern  void    rtl819x_watchdog_wqcallback(struct work_struct *work);
#else
extern  void    rtl819x_watchdog_wqcallback(struct net_device *dev);
#endif

void rtl8192_irq_rx_tasklet(struct r8192_priv *priv);
void rtl8192_irq_tx_tasklet(struct r8192_priv *priv);
//init tasklet and wait_queue here. only 2.6 above kernel is considered
#define DRV_NAME "wlan0"
static void rtl8192_init_priv_task(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);	

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
#ifdef PF_SYNCTHREAD
	priv->priv_wq = create_workqueue(DRV_NAME,0);
#else	
	priv->priv_wq = create_workqueue(DRV_NAME);
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	INIT_WORK(&priv->reset_wq, rtl8192_restart);
#ifdef RTL8192U
	INIT_DELAYED_WORK(&priv->ieee80211->update_assoc_sta_info_wq,(void*)rtl8192_update_peer_ratr_table_wq);
#endif
	//INIT_DELAYED_WORK(&priv->watch_dog_wq, hal_dm_watchdog);
	INIT_DELAYED_WORK(&priv->watch_dog_wq, rtl819x_watchdog_wqcallback);
#ifdef RTL8192U
	INIT_DELAYED_WORK(&priv->txpower_tracking_wq,  dm_txpower_trackingcallback);
#endif
//	INIT_DELAYED_WORK(&priv->gpio_change_rf_wq,  dm_gpio_change_rf_callback);
	INIT_DELAYED_WORK(&priv->rfpath_check_wq,  dm_rf_pathcheck_workitemcallback);
	INIT_DELAYED_WORK(&priv->update_beacon_wq, rtl8192_update_beacon);
	INIT_DELAYED_WORK(&priv->initialgain_operate_wq, InitialGainOperateWorkItemCallBack);
	//INIT_WORK(&priv->SwChnlWorkItem,  rtl8192_SwChnl_WorkItem);
	//INIT_WORK(&priv->SetBWModeWorkItem,  rtl8192_SetBWModeWorkItem);
	INIT_WORK(&priv->qos_activate, rtl8192_qos_activate);
        INIT_WORK(&priv->mcast_wq, rtl819x_set_mcast_register);
	INIT_DELAYED_WORK(&priv->ieee80211->hw_wakeup_wq,(void*) rtl8192_hw_wakeup_wq);
	INIT_DELAYED_WORK(&priv->ieee80211->hw_sleep_wq,(void*) rtl8192_hw_sleep_wq);
#ifdef RTL8192SU
	INIT_WORK(&priv->FwCmdIOWorkItem, SetFwCmdIOWorkItemCallback);
#endif

#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	tq_init(&priv->reset_wq, (void*)rtl8192_restart, dev);
#ifdef RTL8192U
	tq_init(&priv->ieee80211->update_assoc_sta_info_wq,(void*)rtl8192_update_peer_ratr_table_wq, dev);
#endif
	tq_init(&priv->watch_dog_wq, (void*)rtl819x_watchdog_wqcallback, dev);
#ifdef RTL8192U
	tq_init(&priv->txpower_tracking_wq, (void*)dm_txpower_trackingcallback, dev);
#endif
	tq_init(&priv->rfpath_check_wq, (void*)dm_rf_pathcheck_workitemcallback, dev);
	tq_init(&priv->update_beacon_wq, (void*)rtl8192_update_beacon, dev);
	//tq_init(&priv->SwChnlWorkItem, (void*) rtl8192_SwChnl_WorkItem, dev);
	//tq_init(&priv->SetBWModeWorkItem, (void*)rtl8192_SetBWModeWorkItem, dev);
	tq_init(&priv->qos_activate, (void *)rtl8192_qos_activate, dev);
        tq_init(&priv->mcast_wq, (void*)rtl819x_set_mcast_register, dev);
	tq_init(&priv->ieee80211->hw_wakeup_wq,(void*) rtl8192_hw_wakeup_wq, dev);
	tq_init(&priv->ieee80211->hw_sleep_wq,(void*) rtl8192_hw_sleep_wq, dev);

#ifdef RTL8192SU
	tq_init(&priv->FwCmdIOWorkItem, (void *)SetFwCmdIOWorkItemCallback,dev);
#endif

#else
	INIT_WORK(&priv->reset_wq,(void(*)(void*)) rtl8192_restart,dev);
#ifdef RTL8192U
	INIT_WORK(&priv->ieee80211->update_assoc_sta_info_wq,(void*)rtl8192_update_peer_ratr_table_wq, dev);
#endif
	//INIT_WORK(&priv->watch_dog_wq, (void(*)(void*)) hal_dm_watchdog,dev);
	INIT_WORK(&priv->watch_dog_wq, (void(*)(void*)) rtl819x_watchdog_wqcallback,dev);
#ifdef RTL8192U
	INIT_WORK(&priv->txpower_tracking_wq, (void(*)(void*)) dm_txpower_trackingcallback,dev);
#endif
//	INIT_WORK(&priv->gpio_change_rf_wq, (void(*)(void*)) dm_gpio_change_rf_callback,dev);
	INIT_WORK(&priv->rfpath_check_wq, (void(*)(void*)) dm_rf_pathcheck_workitemcallback,dev);
	INIT_WORK(&priv->update_beacon_wq, (void(*)(void*))rtl8192_update_beacon,dev);
	INIT_WORK(&priv->initialgain_operate_wq, (void(*)(void*))InitialGainOperateWorkItemCallBack,dev);
	//INIT_WORK(&priv->SwChnlWorkItem, (void(*)(void*)) rtl8192_SwChnl_WorkItem, dev);
	//INIT_WORK(&priv->SetBWModeWorkItem, (void(*)(void*)) rtl8192_SetBWModeWorkItem, dev);
	INIT_WORK(&priv->qos_activate, (void(*)(void *))rtl8192_qos_activate, dev);
        INIT_WORK(&priv->mcast_wq, (void*)rtl819x_set_mcast_register, dev);
	INIT_WORK(&priv->ieee80211->hw_wakeup_wq,(void*) rtl8192_hw_wakeup_wq, dev);
	INIT_WORK(&priv->ieee80211->hw_sleep_wq,(void*) rtl8192_hw_sleep_wq, dev);

#ifdef RTL8192SU
	INIT_WORK(&priv->FwCmdIOWorkItem, (void(*)(void *))SetFwCmdIOWorkItemCallback,dev);
#endif
	
#endif
#endif

	tasklet_init(&priv->irq_rx_tasklet,
	     (void(*)(unsigned long))rtl8192_irq_rx_tasklet,
	     (unsigned long)priv);

	tasklet_init(&priv->irq_tx_tasklet,
	     (void(*)(unsigned long))rtl8192_irq_tx_tasklet,
	     (unsigned long)priv);

}

static void rtl8192_get_eeprom_size(struct net_device* dev)
{
	u16 curCR = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	RT_TRACE(COMP_EPROM, "===========>%s()\n", __FUNCTION__);	
	curCR = read_nic_word_E(dev,EPROM_CMD);
	RT_TRACE(COMP_EPROM, "read from Reg EPROM_CMD(%x):%x\n", EPROM_CMD, curCR);
	//whether need I consider BIT5?	
	priv->epromtype = (curCR & Cmd9346CR_9356SEL) ? EPROM_93c56 : EPROM_93c46;
	RT_TRACE(COMP_EPROM, "<===========%s(), epromtype:%d\n", __FUNCTION__, priv->epromtype);	
}

//used to swap endian. as ntohl & htonl are not neccessary to swap endian, so use this instead.
static inline u16 endian_swap(u16* data)
{
	u16 tmp = *data;
	*data = (tmp >> 8) | (tmp << 8);
	return *data;	
}

#ifdef RTL8192SU
u8 rtl8192SU_UsbOptionToEndPointNumber(u8 UsbOption)
{
	u8	nEndPoint = 0;
	switch(UsbOption)
	{
		case 0:
			nEndPoint = 6;
			break;
		case 1:
			nEndPoint = 11;
			break;
		case 2:
			nEndPoint = 4;
			break;
		default:
			RT_TRACE(COMP_INIT, "UsbOptionToEndPointNumber(): Invalid UsbOption(%#x)\n", UsbOption);
			break;
	}
	return nEndPoint;
}

u8 rtl8192SU_BoardTypeToRFtype(struct net_device* dev,  u8 Boardtype)
{
	u8	RFtype = RF_1T2R;
	
	switch(Boardtype)
	{
		case 0:
			RFtype = RF_1T1R;
			break;
		case 1:
			RFtype = RF_1T2R;
			break;
		case 2:
			RFtype = RF_2T2R;
			break;
		case 3:
			RFtype = RF_2T2R_GREEN;
			break;
		default:			
			break;
	}
	
	return RFtype;
}

//
//	Description:
//		Config HW adapter information into initial value.
//
//	Assumption:
//		1. After Auto load fail(i.e, check CR9346 fail)
//
//	Created by Roger, 2008.10.21.
//
void 
rtl8192SU_ConfigAdapterInfo8192SForAutoLoadFail(struct net_device* dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	//u16			i,usValue;		
	//u8 sMacAddr[6] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
	u8		rf_path, index;	// For EEPROM/EFUSE After V0.6_1117	
	int	i;

	RT_TRACE(COMP_INIT, "====> ConfigAdapterInfo8192SForAutoLoadFail\n");

	write_nic_byte(dev, SYS_ISO_CTRL+1, 0xE8); // Isolation signals from Loader
	//PlatformStallExecution(10000);
	mdelay(10);
	write_nic_byte(dev, PMC_FSM, 0x02); // Enable Loader Data Keep

	//RT_ASSERT(priv->AutoloadFailFlag==TRUE, ("ReadAdapterInfo8192SEEPROM(): AutoloadFailFlag !=TRUE\n"));
		
	// Initialize IC Version && Channel Plan	
	priv->eeprom_vid = 0;
	priv->eeprom_pid = 0;
	priv->card_8192_version = 0;	
	priv->eeprom_ChannelPlan = 0;
	priv->eeprom_CustomerID = 0;
	priv->eeprom_SubCustomerID = 0;
	priv->bIgnoreDiffRateTxPowerOffset = false;

	RT_TRACE(COMP_INIT, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_INIT, "EEPROM PID = 0x%4x\n", priv->eeprom_pid);
	RT_TRACE(COMP_INIT, "EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);
	RT_TRACE(COMP_INIT, "EEPROM SubCustomer ID: 0x%2x\n", priv->eeprom_SubCustomerID);
	RT_TRACE(COMP_INIT, "EEPROM ChannelPlan = 0x%4x\n", priv->eeprom_ChannelPlan);
	RT_TRACE(COMP_INIT, "IgnoreDiffRateTxPowerOffset = %d\n", priv->bIgnoreDiffRateTxPowerOffset);

	priv->EEPROMUsbOption = EEPROM_USB_Default_OPTIONAL_FUNC;			
	RT_TRACE(COMP_INIT, "USB Option = %#x\n", priv->EEPROMUsbOption);		

	for(i=0; i<5; i++)
		priv->EEPROMUsbPhyParam[i] = EEPROM_USB_Default_PHY_PARAM;			
	
	//RT_PRINT_DATA(COMP_INIT|COMP_EFUSE, DBG_LOUD, ("EFUSE USB PHY Param: \n"), priv->EEPROMUsbPhyParam, 5);
	
	{
	//<Roger_Notes> In this case, we random assigh MAC address here. 2008.10.15.
		static u8 sMacAddr[6] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
		u8	i;

        	//sMacAddr[5] = (u8)GetRandomNumber(1, 254);	
		
		for(i = 0; i < 6; i++)
			dev->dev_addr[i] = sMacAddr[i];	
	}

	//NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);
	write_nic_dword(dev, IDR0, ((u32*)dev->dev_addr)[0]);
	write_nic_word(dev, IDR4, ((u16*)(dev->dev_addr + 4))[0]);

	RT_TRACE(COMP_INIT, "ReadAdapterInfo8192SEFuse(), Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
			dev->dev_addr[0], dev->dev_addr[1], 
			dev->dev_addr[2], dev->dev_addr[3], 
			dev->dev_addr[4], dev->dev_addr[5]); 
	
	priv->EEPROMBoardType = EEPROM_Default_BoardType;	
	priv->rf_type = RF_1T2R; //RF_2T2R
	priv->EEPROMTxPowerDiff = EEPROM_Default_PwDiff;
	priv->EEPROMThermalMeter = EEPROM_Default_ThermalMeter;
	priv->BluetoothCoexist = EEPROM_Default_BlueToothCoexist;
	priv->EEPROMTSSI_A = EEPROM_Default_TSSI;
	priv->EEPROMTSSI_B = EEPROM_Default_TSSI;
			
#ifdef EEPROM_OLD_FORMAT_SUPPORT 
	for(i=0; i<6; i++)
		{
		priv->EEPROMHT2T_TxPwr[i] = EEPROM_Default_HT2T_TxPwr;
		}			

		for(i=0; i<14; i++)
		{
		priv->EEPROMTxPowerLevelCCK24G[i] = (u8)(EEPROM_Default_TxPower & 0xff);
			priv->EEPROMTxPowerLevelOFDM24G[i] = (u8)(EEPROM_Default_TxPower & 0xff);
		}	
	
	//
	// Update HAL variables.
	//
	memcpy( priv->TxPowerLevelOFDM24G, priv->EEPROMTxPowerLevelOFDM24G, 14);		
	memcpy( priv->TxPowerLevelCCK, priv->EEPROMTxPowerLevelCCK24G, 14);
	//RT_PRINT_DATA(COMP_INIT|COMP_EFUSE, DBG_LOUD, ("HAL CCK 2.4G TxPwr: \n"), priv->TxPowerLevelCCK, 14);
	//RT_PRINT_DATA(COMP_INIT|COMP_EFUSE, DBG_LOUD, ("HAL OFDM 2.4G TxPwr: \n"), priv->TxPowerLevelOFDM24G, 14);
#else

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			// Read CCK RF A & B Tx power 
			priv->RfCckChnlAreaTxPwr[rf_path][i] = 
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][i] = 
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][i] = 
			(u8)(EEPROM_Default_TxPower & 0xff);
		}
	}	
			
	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		// Assign dedicated channel tx power
		for(i=0; i<14; i++)	// channel 1~3 use the same Tx Power Level.
		{
			if (i < 3)			// Cjanel 1-3
				index = 0;
			else if (i < 8)		// Channel 4-8
				index = 1;
			else				// Channel 9-14
				index = 2;

			// Record A & B CCK /OFDM - 1T/2T Channel area tx power
			priv->RfTxPwrLevelCck[rf_path][i]  = priv->RfCckChnlAreaTxPwr[rf_path][index];
			priv->RfTxPwrLevelOfdm1T[rf_path][i]  = priv->RfOfdmChnlAreaTxPwr1T[rf_path][index];
			priv->RfTxPwrLevelOfdm2T[rf_path][i]  = priv->RfOfdmChnlAreaTxPwr2T[rf_path][index];
		}
	}						

	// 2009/02/09 Cosa Support new EEPROM format from SD3 requirement for verification.
	for(i=0; i<14; i++)
	{			
		// Set HT 20 <->40MHZ tx power diff	
		priv->TxPwrHt20Diff[RF90_PATH_A][i] = DEFAULT_HT20_TXPWR_DIFF;
		priv->TxPwrHt20Diff[RF90_PATH_B][i] = DEFAULT_HT20_TXPWR_DIFF;

		priv->TxPwrLegacyHtDiff[0][i] = EEPROM_Default_LegacyHTTxPowerDiff;
		priv->TxPwrLegacyHtDiff[1][i] = EEPROM_Default_LegacyHTTxPowerDiff;
	}

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			// Read Power diff limit.
			priv->EEPROMPwrGroup[rf_path][i] = 0;
		}
	}	

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		// Fill Pwr group
		for(i=0; i<14; i++)
		{
			if (i < 3)			// Cjanel 1-3
				index = 0;
			else if (i < 8)		// Channel 4-8
				index = 1;
			else				// Channel 9-13
				index = 2;
			priv->PwrGroupHT20[rf_path][i] = (priv->EEPROMPwrGroup[rf_path][index]&0xf);
			priv->PwrGroupHT40[rf_path][i] = ((priv->EEPROMPwrGroup[rf_path][index]&0xf0)>>4);
		}
	}
	priv->EEPROMRegulatory = 0;
	
#endif

	//
	// Update remained HAL variables.
	//
	priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;
	priv->LegacyHTTxPowerDiff = priv->EEPROMTxPowerDiff;
	//cosa priv->TxPowerDiff = priv->EEPROMTxPowerDiff;
	priv->ThermalMeter[0] = (priv->EEPROMThermalMeter&0x1f);// ThermalMeter, [4:0]	
	priv->LedStrategy = SW_LED_MODE0;

	init_rate_adaptive(dev);
	
	RT_TRACE(COMP_INIT, "<==== ConfigAdapterInfo8192SForAutoLoadFail\n");

}

#if 0
static void rtl8192SU_ReadAdapterInfo8192SEEPROM(struct net_device* dev)
{
	u16 				EEPROMId = 0;
	u8 				bLoad_From_EEPOM = false;
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u16 				tmpValue = 0;	
	u8				tmpBuffer[30];
	int i;
	
	RT_TRACE(COMP_EPROM, "===========>%s()\n", __FUNCTION__);

	
	write_nic_byte(dev, SYS_ISO_CTRL+1, 0xE8); // Isolation signals from Loader
	udelay(10000);
	write_nic_byte(dev, PMC_FSM, 0x02); // Enable Loader Data Keep

	
	EEPROMId = eprom_read(dev, 0); //first read EEPROM ID out;
	RT_TRACE(COMP_EPROM, "EEPROM ID is 0x%x\n", EEPROMId);	
	
#if 1 // support 8712/8713
	if ( (EEPROMId != RTL8190_EEPROM_ID) && (EEPROMId != 0x8712) && (EEPROMId != 0x8713) )
#else
	if (EEPROMId != RTL8190_EEPROM_ID)
#endif	
	{
		priv->AutoloadFailFlag = true;
		RT_TRACE(COMP_ERR, "EEPROM ID is invalid(is 0x%x(should be 0x%x)\n", EEPROMId, RTL8190_EEPROM_ID);
	}
	else
	{
		priv->AutoloadFailFlag = false;
		bLoad_From_EEPOM = true;
	}

	if (bLoad_From_EEPOM)
	{
		tmpValue = eprom_read(dev, (EEPROM_VID>>1));
		priv->eeprom_vid = endian_swap(&tmpValue);
		priv->eeprom_pid = eprom_read(dev, (EEPROM_PID>>1));
		
		// Version ID, Channel plan
		tmpValue = eprom_read(dev, (EEPROM_Version>>1));
		//pHalData->card_8192_version = (VERSION_8192S)((usValue&0x00ff));
		priv->eeprom_ChannelPlan =(tmpValue&0xff00)>>8;
		priv->bTXPowerDataReadFromEEPORM = true;

		// Customer ID, 0x00 and 0xff are reserved for Realtek.
		tmpValue = eprom_read(dev, (u16)(EEPROM_CustomID>>1)) ;	
		priv->eeprom_CustomerID = (u8)( tmpValue & 0xff);	
		priv->eeprom_SubCustomerID = (u8)((tmpValue & 0xff00)>>8);
	}
	else
	{
		priv->eeprom_vid = 0;
		priv->eeprom_pid = 0;
		//priv->card_8192_version = VERSION_8192SU_A;	
		priv->eeprom_ChannelPlan = 0;
		priv->eeprom_CustomerID = 0;
		priv->eeprom_SubCustomerID = 0;
	}
	RT_TRACE(COMP_EPROM, "vid:0x%4x, pid:0x%4x, CustomID:0x%2x, ChanPlan:0x%x\n", priv->eeprom_vid, priv->eeprom_pid, priv->eeprom_CustomerID, priv->eeprom_ChannelPlan);
	//set channelplan from eeprom
	priv->ChannelPlan = priv->eeprom_ChannelPlan;// FIXLZM

	RT_TRACE(COMP_INIT, "EEPROMId = 0x%4x\n", EEPROMId);
	RT_TRACE(COMP_INIT, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_INIT, "EEPROM PID = 0x%4x\n", priv->eeprom_pid);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("EEPROM Version ID: 0x%2x\n", pHalData->VersionID));
	RT_TRACE(COMP_INIT, "EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);
	RT_TRACE(COMP_INIT, "EEPROM SubCustomer ID: 0x%2x\n", priv->eeprom_SubCustomerID);
	RT_TRACE(COMP_INIT, "EEPROM ChannelPlan = 0x%4x\n", priv->eeprom_ChannelPlan);

	// Read USB optional function.
	if(bLoad_From_EEPOM)
	{
		tmpValue = eprom_read(dev, (EEPROM_USB_OPTIONAL>>1));
		priv->EEPROMUsbOption = (u8)(tmpValue&0xff);
	}
	else
	{
		priv->EEPROMUsbOption = EEPROM_USB_Default_OPTIONAL_FUNC;			
	}
		
	RT_TRACE(COMP_INIT, "USB Option = %#x\n", priv->EEPROMUsbOption);		
	
	
	if (bLoad_From_EEPOM)
	{
		int i;
		for (i=0; i<6; i+=2)
		{
			u16 tmp = 0;
			tmp = eprom_read(dev, (u16)((EEPROM_NODE_ADDRESS_BYTE_0 + i)>>1));
			*(u16*)(&dev->dev_addr[i]) = tmp;
		}
	}	
	else
	{	
		//<Roger_Notes> In this case, we random assigh MAC address here. 2008.10.15.
		static u8 sMacAddr[6] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
		u8	i;

		//sMacAddr[5] = (u8)GetRandomNumber(1, 254);	

		for(i = 0; i < 6; i++)
			dev->dev_addr[i] = sMacAddr[i];
			
		//memcpy(dev->dev_addr, sMacAddr, 6);
		//should I set IDR0 here?
	}
	write_nic_dword(dev, IDR0, ((u32*)dev->dev_addr)[0]);
	write_nic_word(dev, IDR4, ((u16*)(dev->dev_addr + 4))[0]);
	RT_TRACE(COMP_EPROM, "MAC addr:"MAC_FMT"\n", MAC_ARG(dev->dev_addr));

	priv->rf_type = RTL819X_DEFAULT_RF_TYPE; //default 1T2R
#if (defined (RTL8192SU_FPGA_2MAC_VERIFICATION)||defined (RTL8192SU_ASIC_VERIFICATION))
	priv->rf_chip = RF_6052;
	priv->rf_type = RTL819X_DEFAULT_RF_TYPE;
	//priv->card_8192_version = VERSION_8192SU_A; //Over write for temporally experiment. 2008.10.16. By Roger.
#else
	priv->rf_chip = RF_8256;
#endif	

	{
#if 0
		if(bLoad_From_EEPOM)
		{
			tempval = (ReadEEprom(Adapter, (EEPROM_RFInd_PowerDiff>>1))) & 0xff;
			if (tempval&0x80)	//RF-indication, bit[7]
				pHalData->RF_Type = RF_1T2R;
			else
				pHalData->RF_Type = RF_2T4R;
		}
#endif
		
		priv->EEPROMTxPowerDiff = EEPROM_Default_TxPowerDiff;	
		RT_TRACE(COMP_INIT, "TxPowerDiff = %#x\n", priv->EEPROMTxPowerDiff);
		
		
		//
		// Read antenna tx power offset of B/C/D to A  from EEPROM
		// and read ThermalMeter from EEPROM
		//
		if(bLoad_From_EEPOM)
		{
			tmpValue = eprom_read(dev, (EEPROM_PwDiff>>1));
			priv->EEPROMPwDiff = tmpValue&0x00ff;
			priv->EEPROMThermalMeter = (tmpValue&0xff00)>>8;			
		}
		else
		{
			priv->EEPROMPwDiff = EEPROM_Default_PwDiff;
			priv->EEPROMThermalMeter = EEPROM_Default_ThermalMeter;
		}
		RT_TRACE(COMP_INIT, "PwDiff = %#x\n", priv->EEPROMPwDiff);
		RT_TRACE(COMP_INIT, "ThermalMeter = %#x\n", priv->EEPROMThermalMeter);
		
		priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;
		
		
		// Read CrystalCap from EEPROM
		if(bLoad_From_EEPOM)
		{
			priv->EEPROMCrystalCap =(u8) (((eprom_read(dev, (EEPROM_CrystalCap>>1)))&0xf000)>>12);
		}
		else
		{
			priv->EEPROMCrystalCap = EEPROM_Default_CrystalCap;
		}
		RT_TRACE(COMP_INIT, "CrystalCap = %#x\n", priv->EEPROMCrystalCap);
			
		
		//if(pHalData->EEPROM_Def_Ver == 0)	// old eeprom definition
		{	

			//
			// Get Tx Power Base.//===>
			//
			if(bLoad_From_EEPOM)
			{
				priv->EEPROMTxPwrBase =(u8) ((eprom_read(dev, (EEPROM_TxPowerBase>>1)))&0xff);
			}
			else
			{
				priv->EEPROMTxPwrBase = EEPROM_Default_TxPowerBase;	
			}
	
			RT_TRACE(COMP_INIT, "TxPwrBase = %#x\n", priv->EEPROMTxPwrBase);

			//
			// Get CustomerID(Boad Type)
			// i.e., 0x0: RTL8188SU, 0x1: RTL8191SU, 0x2: RTL8192SU, 0x3: RTL8191GU.
			// Others: Reserved. Default is 0x2: RTL8192SU.
			//
			if(bLoad_From_EEPOM)
			{
				tmpValue = eprom_read(dev, (u16) (EEPROM_BoardType>>1));
				priv->EEPROMBoardType = (u8)(tmpValue&0xff);
			}
			else
			{
				priv->EEPROMBoardType = EEPROM_Default_BoardType;	
			}
		
			RT_TRACE(COMP_INIT, "BoardType = %#x\n", priv->EEPROMBoardType);

#ifdef EEPROM_OLD_FORMAT_SUPPORT
		
			//
			// Buffer TxPwIdx(i.e., from offset 0x58~0x75, total 30Bytes)
			//
			if(bLoad_From_EEPOM)
			{
				for(i = 0; i < 30; i += 2)
				{
					tmpValue = eprom_read(dev, (u16) ((EEPROM_TxPowerBase+i)>>1));
					*((u16 *)(&tmpBuffer[i])) = tmpValue;					
				}
			}

			//
			// Update CCK, OFDM Tx Power Index from above buffer.
			//
			if(bLoad_From_EEPOM)
			{
				for(i=0; i<14; i++)
				{
					priv->EEPROMTxPowerLevelCCK24G[i] = (u8)tmpBuffer[i+1];
					priv->EEPROMTxPowerLevelOFDM24G[i] = (u8)tmpBuffer[i+15];
				}
				
			}
			else	
			{
				for(i=0; i<14; i++)
				{
					priv->EEPROMTxPowerLevelCCK24G[i] = (u8)(EEPROM_Default_TxPower & 0xff);
					priv->EEPROMTxPowerLevelOFDM24G[i] = (u8)(EEPROM_Default_TxPower & 0xff);
				}
			}

			for(i=0; i<14; i++)
			{
				RT_TRACE(COMP_INIT, "CCK 2.4G Tx Power Level, Index %d = 0x%02x\n", i, priv->EEPROMTxPowerLevelCCK24G[i]);
				RT_TRACE(COMP_INIT, "OFDM 2.4G Tx Power Level, Index %d = 0x%02x\n", i, priv->EEPROMTxPowerLevelOFDM24G[i]);
			}
#else
			// Please add code in the section!!!!
			// And merge tx power difference section.
#endif

			//
			// Get TSSI value for each path.
			//
			if(bLoad_From_EEPOM)
			{
				tmpValue = eprom_read(dev, (u16) ((EEPROM_TSSI_A)>>1));
				priv->EEPROMTSSI_A = (u8)((tmpValue&0xff00)>>8);
			}
			else
			{ // Default setting for Empty EEPROM
				priv->EEPROMTSSI_A = EEPROM_Default_TSSI;		
			}

			if(bLoad_From_EEPOM)
			{
				tmpValue = eprom_read(dev, (u16) ((EEPROM_TSSI_B)>>1));
				priv->EEPROMTSSI_B = (u8)(tmpValue&0xff);
				priv->EEPROMTxPwrTkMode = (u8)((tmpValue&0xff00)>>8);
			}
			else
			{ // Default setting for Empty EEPROM
				priv->EEPROMTSSI_B = EEPROM_Default_TSSI;	
				priv->EEPROMTxPwrTkMode = EEPROM_Default_TxPwrTkMode;
			}

			RT_TRACE(COMP_INIT, "TSSI_A = %#x, TSSI_B = %#x\n", priv->EEPROMTSSI_A, priv->EEPROMTSSI_B);	
			RT_TRACE(COMP_INIT, "TxPwrTkMod = %#x\n", priv->EEPROMTxPwrTkMode);

#ifdef EEPROM_OLD_FORMAT_SUPPORT
			//
			// Get HT 2T Path A and B Power Index.
			//
			if(bLoad_From_EEPOM)
			{
				for(i = 0; i < 6; i += 2)
				{
					tmpValue = eprom_read(dev, (u16) ((EEPROM_HT2T_CH1_A+i)>>1));
					*((u16*)(&priv->EEPROMHT2T_TxPwr[i])) = tmpValue;
				}		
			}
			else
			{ // Default setting for Empty EEPROM				
				for(i=0; i<6; i++)
				{
					priv->EEPROMHT2T_TxPwr[i] = EEPROM_Default_HT2T_TxPwr;
				}
			}

			for(i=0; i<6; i++)
			{
				RT_TRACE(COMP_INIT, "EEPROMHT2T_TxPwr, Index %d = 0x%02x\n", i, priv->EEPROMHT2T_TxPwr[i]);					
			}
#else

#endif	
		}

#ifdef EEPROM_OLD_FORMAT_SUPPORT 
		//
		// Update HAL variables.
		//
		for(i=0; i<14; i++)
		{			
			priv->TxPowerLevelOFDM24G[i] = priv->EEPROMTxPowerLevelOFDM24G[i];
			priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK24G[i];
		}
#else

#endif		
		priv->TxPowerDiff = priv->EEPROMPwDiff;
		// Antenna B gain offset to antenna A, bit0~3
		priv->AntennaTxPwDiff[0] = (priv->EEPROMTxPowerDiff & 0xf);
		// Antenna C gain offset to antenna A, bit4~7
		priv->AntennaTxPwDiff[1] = ((priv->EEPROMTxPowerDiff & 0xf0)>>4);
		// CrystalCap, bit12~15
		priv->CrystalCap = priv->EEPROMCrystalCap;
		// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
		// 92U does not enable TX power tracking.
		priv->ThermalMeter[0] = priv->EEPROMThermalMeter;
	}
	
	priv->LedStrategy = SW_LED_MODE0;
	
	if(priv->rf_type == RF_1T2R)
	{		
		RT_TRACE(COMP_EPROM, "\n1T2R config\n");
	}
	else
	{
		RT_TRACE(COMP_EPROM, "\n2T4R config\n");
	}
	
	// 2008/01/16 MH We can only know RF type in the function. So we have to init
	// DIG RATR table again.
	init_rate_adaptive(dev);
	//we need init DIG RATR table here again.

	RT_TRACE(COMP_EPROM, "<===========%s()\n", __FUNCTION__);
	return;	
}

//
//	Description:
//		1. Read HW adapter information by E-Fuse.
//		2. Refered from SD1 Richard.
//
//	Assumption:
//		1. Boot from E-Fuse and CR9346 regiser has verified.
//		2. PASSIVE_LEVEL (USB interface)
//
//	Created by Roger, 2008.10.21.
//
void
rtl8192SU_ReadAdapterInfo8192SEFuse(struct net_device* dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u16			i,usValue;
	u16			EEPROMId;
	u8			readbyte;	
	u8			OFDMTxPwr[14];
	u8			CCKTxPwr[14];
	u8			HT2T_TxPwr[6];
	u8			UsbPhyParam[5];
	u8			hwinfo[HWSET_MAX_SIZE_92S];
	    

	RT_TRACE(COMP_INIT, "====> ReadAdapterInfo8192SEFuse\n");

	//
	// <Roger_Notes> We set Isolation signals from Loader and reset EEPROM after system resuming 
	// from suspend mode.
	// 2008.10.21. 
	//
	write_nic_byte(dev, SYS_ISO_CTRL+1, 0xE8); // Isolation signals from Loader
	//PlatformStallExecution(10000);
	mdelay(10);
	write_nic_byte(dev, SYS_FUNC_EN+1, 0x40); 
	write_nic_byte(dev, SYS_FUNC_EN+1, 0x50); 
	
	readbyte = read_nic_byte(dev, EFUSE_TEST+3);
	write_nic_byte(dev, EFUSE_TEST+3, (readbyte | 0x80));
	write_nic_byte(dev, EFUSE_TEST+3, 0x72); 
	write_nic_byte(dev, EFUSE_CLK, 0x03);

	//
	// Dump EFUSe at init time for later use
	//
	// Read EFUSE real map to shadow!!
	EFUSE_ShadowMapUpdate(dev);
	
	memcpy(hwinfo, (void*)&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE_92S);		
	//RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("MAP \n"), hwinfo, HWSET_MAX_SIZE_92S);	

	//
	// <Roger_Notes> Event though CR9346 regiser can verify whether Autoload is success or not, but we still
	// double check ID codes for 92S here(e.g., due to HW GPIO polling fail issue).
	// 2008.10.21.
	//	
	ReadEFuse(dev, 0, 2, (unsigned char*) &EEPROMId);	
	
	if( EEPROMId != RTL8190_EEPROM_ID )
	{
		RT_TRACE(COMP_INIT, "EEPROM ID(%#x) is invalid!!\n", EEPROMId); 
		priv->AutoloadFailFlag=true;
	}
	else
	{
		priv->AutoloadFailFlag=false;
	}
	
       // Read IC Version && Channel Plan
	if(!priv->AutoloadFailFlag)
	{

        	// VID, PID	 
	    	ReadEFuse(dev, EEPROM_VID, 2, (unsigned char*) &priv->eeprom_vid);	
		ReadEFuse(dev, EEPROM_PID, 2, (unsigned char*) &priv->eeprom_pid);

		// Version ID, Channel plan
		ReadEFuse(dev, EEPROM_Version, 2, (unsigned char*) &usValue);		
		//pHalData->VersionID = (VERSION_8192S)(usValue&0x00ff);
		priv->eeprom_ChannelPlan = (usValue&0xff00>>8);
		priv->bTXPowerDataReadFromEEPORM = true;

		// Customer ID, 0x00 and 0xff are reserved for Realtek.
		ReadEFuse(dev, EEPROM_CustomID, 2, (unsigned char*) &usValue);		
		priv->eeprom_CustomerID = (u8)( usValue & 0xff);	
		priv->eeprom_SubCustomerID = (u8)((usValue & 0xff00)>>8);
	}
	else
	{
		priv->eeprom_vid = 0;
		priv->eeprom_pid = 0;
		priv->eeprom_ChannelPlan = 0;
		priv->eeprom_CustomerID = 0;
		priv->eeprom_SubCustomerID = 0;
	}

	RT_TRACE(COMP_INIT, "EEPROM Id = 0x%4x\n", EEPROMId);
	RT_TRACE(COMP_INIT, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_INIT, "EEPROM PID = 0x%4x\n", priv->eeprom_pid);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("EEPROM Version ID: 0x%2x\n", pHalData->VersionID));
	RT_TRACE(COMP_INIT, "EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);
	RT_TRACE(COMP_INIT, "EEPROM SubCustomer ID: 0x%2x\n", priv->eeprom_SubCustomerID);
	RT_TRACE(COMP_INIT, "EEPROM ChannelPlan = 0x%4x\n", priv->eeprom_ChannelPlan);


	// Read USB optional function.
	if(!priv->AutoloadFailFlag)
	{
		ReadEFuse(dev, EEPROM_USB_OPTIONAL, 1, (unsigned char*) &priv->EEPROMUsbOption);
	}
	else
	{
		priv->EEPROMUsbOption = EEPROM_USB_Default_OPTIONAL_FUNC;			
	}
	
	RT_TRACE(COMP_INIT, "USB Option = %#x\n", priv->EEPROMUsbOption);		


	// Read USB PHY parameters.
	if(!priv->AutoloadFailFlag)
	{
		ReadEFuse(dev, EEPROM_USB_PHY_PARA1, 5, (unsigned char*)UsbPhyParam);
		for(i=0; i<5; i++)
		{
			priv->EEPROMUsbPhyParam[i] = UsbPhyParam[i];
			RT_TRACE(COMP_INIT, "USB Param = index(%d) = %#x\n", i, priv->EEPROMUsbPhyParam[i]);		
		}
	}
	else
	{
		for(i=0; i<5; i++)
		{
			priv->EEPROMUsbPhyParam[i] = EEPROM_USB_Default_PHY_PARAM;		
			RT_TRACE(COMP_INIT, "USB Param = index(%d) = %#x\n", i, priv->EEPROMUsbPhyParam[i]);			
		}
	}	
	
	
       //Read Permanent MAC address
	if(!priv->AutoloadFailFlag)
	{
		u8			macaddr[6] = {0x00, 0xe1, 0x86, 0x4c, 0x92, 0x00};
	
		ReadEFuse(dev, EEPROM_NODE_ADDRESS_BYTE_0, 6, (unsigned char*)macaddr);
		for(i=0; i<6; i++)
			dev->dev_addr[i] = macaddr[i];	
	}
	else
	{//Auto load fail

		//<Roger_Notes> In this case, we random assigh MAC address here. 2008.10.15.
		static u8 sMacAddr[6] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
		u8	i;

		//if(!Adapter->bInHctTest)
                    //sMacAddr[5] = (u8)GetRandomNumber(1, 254);	
		
		for(i = 0; i < 6; i++)
			dev->dev_addr[i] = sMacAddr[i];			
	}	

	//NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);
	write_nic_dword(dev, IDR0, ((u32*)dev->dev_addr)[0]);
	write_nic_word(dev, IDR4, ((u16*)(dev->dev_addr + 4))[0]);

	RT_TRACE(COMP_INIT, "ReadAdapterInfo8192SEFuse(), Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
			dev->dev_addr[0], dev->dev_addr[1], 
			dev->dev_addr[2], dev->dev_addr[3], 
			dev->dev_addr[4], dev->dev_addr[5]); 	

	// 2007/11/15 MH For RTL8192USB we assign as 1T2R now.
	priv->rf_type = RTL819X_DEFAULT_RF_TYPE;	// default : 1T2R

#if (defined (RTL8192SU_FPGA_2MAC_VERIFICATION)||defined (RTL8192SU_ASIC_VERIFICATION))
	priv->rf_chip = RF_6052;
	priv->rf_type = RTL819X_DEFAULT_RF_TYPE;	
#else
	priv->rf_chip = RF_8256;
#endif		

	{
		//
		// Read antenna tx power offset of B/C/D to A  from EEPROM
		// and read ThermalMeter from EEPROM
		//
		if(!priv->AutoloadFailFlag)
		{
			ReadEFuse(dev, EEPROM_PwDiff, 2, (unsigned char*) &usValue);
			priv->EEPROMPwDiff = usValue&0x00ff;
			priv->EEPROMThermalMeter = (usValue&0xff00)>>8;
		}
		else
		{
			priv->EEPROMPwDiff = EEPROM_Default_PwDiff;
			priv->EEPROMThermalMeter = EEPROM_Default_ThermalMeter;
		}
		
		RT_TRACE(COMP_INIT, "PwDiff = %#x\n", priv->EEPROMPwDiff);		
		RT_TRACE(COMP_INIT, "ThermalMeter = %#x\n", priv->EEPROMThermalMeter);

		priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;

		//
		// Read Tx Power gain offset of legacy OFDM to HT rate.
		// Read CrystalCap from EEPROM
		//
		if(!priv->AutoloadFailFlag)
		{
			ReadEFuse(dev, EEPROM_CrystalCap, 1, (unsigned char*) &usValue);
			priv->EEPROMCrystalCap = (u8)((usValue&0xf0)>>4);
		}
		else
		{	
			priv->EEPROMCrystalCap = EEPROM_Default_CrystalCap;
		}
		
		RT_TRACE(COMP_INIT, "CrystalCap = %#x\n", priv->EEPROMCrystalCap);
			
		priv->EEPROMTxPowerDiff = EEPROM_Default_TxPowerDiff;
		RT_TRACE(COMP_INIT, "TxPowerDiff = %d\n", priv->EEPROMTxPowerDiff);
		

		//
		// Get Tx Power Base.
		//
		if(!priv->AutoloadFailFlag)
		{
			ReadEFuse(dev, EEPROM_TxPowerBase, 1, (unsigned char*) &priv->EEPROMTxPwrBase );
		}
		else
		{
			priv->EEPROMTxPwrBase = EEPROM_Default_TxPowerBase;	
		}
		
		RT_TRACE(COMP_INIT, "TxPwrBase = %#x\n", priv->EEPROMTxPwrBase);

		//
		// Get CustomerID(Boad Type)
		// i.e., 0x0: RTL8188SU, 0x1: RTL8191SU, 0x2: RTL8192SU, 0x3: RTL8191GU.
		// Others: Reserved. Default is 0x2: RTL8192SU.
		//
		if(!priv->AutoloadFailFlag)
		{
			ReadEFuse(dev, EEPROM_BoardType, 1, (unsigned char*) &priv->EEPROMBoardType );
		}
		else
		{
			priv->EEPROMBoardType = EEPROM_Default_BoardType;	
		}
		
		RT_TRACE(COMP_INIT, "BoardType = %#x\n", priv->EEPROMBoardType);

		//if(pHalData->EEPROM_Def_Ver == 0)
		{
#ifdef EEPROM_OLD_FORMAT_SUPPORT
			//
			// Get CCK Tx Power Index.
			//
			if(!priv->AutoloadFailFlag)
			{
				ReadEFuse(dev, EEPROM_TxPwIndex_CCK_24G, 14, (unsigned char*)CCKTxPwr);
				for(i=0; i<14; i++)
				{
					RT_TRACE(COMP_INIT, "CCK 2.4G Tx Power Level, Index %d = 0x%02x\n", i, CCKTxPwr[i]);
					priv->EEPROMTxPowerLevelCCK24G[i] = CCKTxPwr[i];
				}
			}
			else
			{ // Default setting for Empty EEPROM
				for(i=0; i<14; i++)
					priv->EEPROMTxPowerLevelCCK24G[i] = (u8)(EEPROM_Default_TxPower & 0xff);
			}

			//
			// Get OFDM Tx Power Index.
			//
			if(!priv->AutoloadFailFlag)
			{
				ReadEFuse(dev, EEPROM_TxPwIndex_OFDM_24G, 14, (unsigned char*)OFDMTxPwr);
				for(i=0; i<14; i++)
				{
					RT_TRACE(COMP_INIT, "OFDM 2.4G Tx Power Level, Index %d = 0x%02x\n", i, OFDMTxPwr[i]);
					priv->EEPROMTxPowerLevelOFDM24G[i] = OFDMTxPwr[i];
				}
			}
			else
			{ // Default setting for Empty EEPROM
				usValue = 0x10;
				for(i=0; i<14; i++)
					priv->EEPROMTxPowerLevelOFDM24G[i] = (u8)usValue;
			}		
#else
			// Please add code in the section!!!!
			// And merge tx power difference section.
#endif

			//
			// Get TSSI value for each path.
			//
			if(!priv->AutoloadFailFlag)
			{
				ReadEFuse(dev, EEPROM_TSSI_A, 2, (unsigned char*)&usValue);
				priv->EEPROMTSSI_A = (u8)(usValue&0xff);
				priv->EEPROMTSSI_B = (u8)((usValue&0xff00)>>8);
			}
			else
			{ // Default setting for Empty EEPROM
				priv->EEPROMTSSI_A = EEPROM_Default_TSSI;
				priv->EEPROMTSSI_B = EEPROM_Default_TSSI;
			}
			
			RT_TRACE(COMP_INIT, "TSSI_A = %#x, TSSI_B = %#x\n", 
					priv->EEPROMTSSI_A, priv->EEPROMTSSI_B);

			//
			// Get Tx Power tracking mode.
			//
			if(!priv->AutoloadFailFlag)
			{		
				ReadEFuse(dev, EEPROM_TxPwTkMode, 1, (unsigned char*)&priv->EEPROMTxPwrTkMode);
			}
			else
			{ // Default setting for Empty EEPROM
				priv->EEPROMTxPwrTkMode = EEPROM_Default_TxPwrTkMode;
			}

			RT_TRACE(COMP_INIT, "TxPwrTkMod = %#x\n", priv->EEPROMTxPwrTkMode);


			// TODO: The following HT 2T Path A and B Power Index should be updated.!! Added by Roger, 2008.20.23.
			
			//
			// Get HT 2T Path A and B Power Index.
			//
			if(!priv->AutoloadFailFlag)
			{
				ReadEFuse(dev, EEPROM_HT2T_CH1_A, 6, (unsigned char*)HT2T_TxPwr);
				for(i=0; i<6; i++)
				{
					priv->EEPROMHT2T_TxPwr[i] = HT2T_TxPwr[i];
				}					
			}
			else
			{ // Default setting for Empty EEPROM				
				for(i=0; i<6; i++)
				{
					priv->EEPROMHT2T_TxPwr[i] = EEPROM_Default_HT2T_TxPwr;
				}
			}			

			for(i=0; i<6; i++)
			{
				RT_TRACE(COMP_INIT, "EEPROMHT2T_TxPwr, Index %d = 0x%02x\n", 
						i, priv->EEPROMHT2T_TxPwr[i]);					
			}
		}
		
#ifdef EEPROM_OLD_FORMAT_SUPPORT 
		//
		// Update HAL variables.
		//
		for(i=0; i<14; i++)
		{			
			priv->TxPowerLevelOFDM24G[i] = priv->EEPROMTxPowerLevelOFDM24G[i];
			priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK24G[i];
		}		
#else

#endif
		priv->TxPowerDiff = priv->EEPROMPwDiff;
		// Antenna B gain offset to antenna A, bit0~3
		priv->AntennaTxPwDiff[0] = (priv->EEPROMTxPowerDiff & 0xf);
		// Antenna C gain offset to antenna A, bit4~7
		priv->AntennaTxPwDiff[1] = ((priv->EEPROMTxPowerDiff & 0xf0)>>4);
		// CrystalCap, bit12~15
		priv->CrystalCap = priv->EEPROMCrystalCap;
		// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
		// 92U does not enable TX power tracking.
		priv->ThermalMeter[0] = priv->EEPROMThermalMeter;
	}
	
	priv->LedStrategy = SW_LED_MODE0;

	init_rate_adaptive(dev);

	RT_TRACE(COMP_INIT, "<==== ReadAdapterInfo8192SEFuse\n");

}
#endif

//
//	Description:
//		Read HW adapter information by E-Fuse or EEPROM according CR9346 reported.
//
//	Assumption:
//		1. CR9346 regiser has verified.
//		2. PASSIVE_LEVEL (USB interface)
//
//	Created by Roger, 2008.10.21.
//
void
rtl8192SU_ReadAdapterInfo8192SUsb(struct net_device* dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u16			i,usValue;
	u8			tmpU1b, tempval;	    
	u16			EEPROMId;
	u8			hwinfo[HWSET_MAX_SIZE_92S];
	u8			rf_path, index;	// For EEPROM/EFUSE After V0.6_1117
	    

	RT_TRACE(COMP_INIT, "====> ReadAdapterInfo8192SUsb\n");
	
	//
	// <Roger_Note> The following operation are prevent Efuse leakage by turn on 2.5V.
	// 2008.11.25.
	//
	tmpU1b = read_nic_byte(dev, EFUSE_TEST+3);
	write_nic_byte(dev, EFUSE_TEST+3, tmpU1b|0x80);
	//PlatformStallExecution(1000);
	mdelay(1);
	write_nic_byte(dev, EFUSE_TEST+3, (tmpU1b&(~BIT7)));
	
	// Retrieve Chip version.
	priv->card_8192_version = (VERSION_8192S)((read_nic_dword(dev, PMC_FSM)>>16)&0xF);
	RT_TRACE(COMP_INIT, "Chip Version ID: 0x%2x\n", priv->card_8192_version);

	switch(priv->card_8192_version)
	{
		case 0:
			RT_TRACE(COMP_INIT, "Chip Version ID: VERSION_8192S_ACUT.\n");
			break;
		case 1:
			RT_TRACE(COMP_INIT, "Chip Version ID: VERSION_8192S_BCUT.\n");
			break;
		case 2:
			RT_TRACE(COMP_INIT, "Chip Version ID: VERSION_8192S_CCUT.\n");
			break;
		case 3:
			RT_TRACE(COMP_INIT, "Chip Version ID: VERSION_8192S_DCUT.\n");
			break;
		default:
			RT_TRACE(COMP_INIT, "Unknown Chip Version!!\n");
			priv->card_8192_version = VERSION_8192S_BCUT;
			break;
	}	
	
	//if (IS_BOOT_FROM_EEPROM(Adapter))
	if(priv->EepromOrEfuse)
	{	// Read frin EEPROM
		write_nic_byte(dev, SYS_ISO_CTRL+1, 0xE8); // Isolation signals from Loader
		//PlatformStallExecution(10000);
		mdelay(10);
		write_nic_byte(dev, PMC_FSM, 0x02); // Enable Loader Data Keep

		// Read all Content from EEPROM or EFUSE.
		for(i = 0; i < HWSET_MAX_SIZE_92S; i += 2)
		{
			usValue = eprom_read(dev, (u16) (i>>1));
			*((u16*)(&hwinfo[i])) = usValue;					
		}
	}
	else if (!(priv->EepromOrEfuse))
	{	// Read from EFUSE	

		//
		// <Roger_Notes> We set Isolation signals from Loader and reset EEPROM after system resuming 
		// from suspend mode.
		// 2008.10.21. 
		//
		//PlatformEFIOWrite1Byte(Adapter, SYS_ISO_CTRL+1, 0xE8); // Isolation signals from Loader
		//PlatformStallExecution(10000);
		//PlatformEFIOWrite1Byte(Adapter, SYS_FUNC_EN+1, 0x40); 
		//PlatformEFIOWrite1Byte(Adapter, SYS_FUNC_EN+1, 0x50); 
		
		//tmpU1b = PlatformEFIORead1Byte(Adapter, EFUSE_TEST+3);
		//PlatformEFIOWrite1Byte(Adapter, EFUSE_TEST+3, (tmpU1b | 0x80));
		//PlatformEFIOWrite1Byte(Adapter, EFUSE_TEST+3, 0x72); 
		//PlatformEFIOWrite1Byte(Adapter, EFUSE_CLK, 0x03);

		// Read EFUSE real map to shadow.
		EFUSE_ShadowMapUpdate(dev);
		memcpy(hwinfo, &priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE_92S);		
	}
	else
	{
		RT_TRACE(COMP_INIT, "ReadAdapterInfo8192SUsb(): Invalid boot type!!\n");
	}

	//
	// <Roger_Notes> The following are EFUSE/EEPROM independent operations!!
	//
	//RT_PRINT_DATA(COMP_EFUSE, DBG_LOUD, ("MAP: \n"), hwinfo, HWSET_MAX_SIZE_92S);	

	//
	// <Roger_Notes> Event though CR9346 regiser can verify whether Autoload is success or not, but we still
	// double check ID codes for 92S here(e.g., due to HW GPIO polling fail issue).
	// 2008.10.21.
	//		
	EEPROMId = *((u16 *)&hwinfo[0]);
	
#if 1 // support 8712/8713
	if( (EEPROMId != RTL8190_EEPROM_ID) && (EEPROMId != 0x8712) && (EEPROMId != 0x8713) )
#else
	if( EEPROMId != RTL8190_EEPROM_ID )
#endif
	{
		RT_TRACE(COMP_INIT, "ID(%#x) is invalid!!\n", EEPROMId); 
		priv->bTXPowerDataReadFromEEPORM = FALSE;
		priv->AutoloadFailFlag=TRUE;
	}
	else
	{	
		priv->AutoloadFailFlag=FALSE;
#if RTL8192SU_USE_PARAM_TXPWR
		priv->bTXPowerDataReadFromEEPORM = FALSE;
#else
		priv->bTXPowerDataReadFromEEPORM = TRUE;
#endif	

	}

	if(priv->AutoloadFailFlag == TRUE)
	{
		rtl8192SU_ConfigAdapterInfo8192SForAutoLoadFail(dev);
		return ;
	}

        // VID, PID	 	
	priv->eeprom_vid = *(u16 *)&hwinfo[EEPROM_VID];
	priv->eeprom_pid = *(u16 *)&hwinfo[EEPROM_PID];
	priv->bIgnoreDiffRateTxPowerOffset = false;	//cosa for test

	if( (priv->eeprom_vid == 0x07D1) && (priv->eeprom_pid == 0x3300))
		priv->bDMInitialGainEnable = TRUE;

	// EEPROM Version ID, Channel plan
	priv->EEPROMVersion = *(u8 *)&hwinfo[EEPROM_Version];
	priv->eeprom_ChannelPlan = *(u8 *)&hwinfo[EEPROM_ChannelPlan];

	// Customer ID, 0x00 and 0xff are reserved for Realtek.			
	priv->eeprom_CustomerID = *(u8 *)&hwinfo[EEPROM_CustomID];
	priv->eeprom_SubCustomerID = *(u8 *)&hwinfo[EEPROM_SubCustomID];

	// [For 90/92U/ USB single chip, read EEPROM to decied TxRx] 2009.03.31, by Bohn	
	priv->EEPROMOptional = *(u8 *)&hwinfo[EEPROM_Optional]; 

	// [For 90/92U/ USB single chip, read EEPROM to decied TxRx] 2009.03.31, by Bohn	
	priv->bForcedShowRxRate = FALSE;
	if(priv->ShowRateMode == 0)
	{
		if((priv->EEPROMOptional & BIT1) == 0x02/*0000_0000*/)
			priv->bForcedShowRxRate = TRUE;
	}
	else if(priv->ShowRateMode == 2)
	{
		priv->bForcedShowRxRate = TRUE;
	}

	priv->ExternalPA = ((priv->EEPROMOptional & BIT2)>>2);

	RT_TRACE(COMP_INIT, "EEPROM Id = 0x%4x\n", EEPROMId);
	RT_TRACE(COMP_INIT, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_INIT, "EEPROM PID = 0x%4x\n", priv->eeprom_pid);
	RT_TRACE(COMP_INIT, "EEPROM Version ID: 0x%2x\n", priv->EEPROMVersion);
	RT_TRACE(COMP_INIT, "EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);
	RT_TRACE(COMP_INIT, "EEPROM SubCustomer ID: 0x%2x\n", priv->eeprom_SubCustomerID);
	RT_TRACE(COMP_INIT, "EEPROM ChannelPlan = 0x%4x\n", priv->eeprom_ChannelPlan);
	RT_TRACE(COMP_INIT, "bIgnoreDiffRateTxPowerOffset = %d\n", priv->bIgnoreDiffRateTxPowerOffset);
	// [For 90/92U/ USB single chip, read EEPROM to decied TxRx] 2009.03.31, by Bohn	
	RT_TRACE(COMP_INIT, "EEPROMOptional = %x\n", priv->EEPROMOptional);

	//
	//  Decide CustomerID according to VID/DID or EEPROM
	//
	switch(priv->eeprom_CustomerID)
	{
		case EEPROM_CID_ALPHA:
			priv->CustomerID = RT_CID_819x_ALPHA;
			break;
			
		case EEPROM_CID_CAMEO:
			priv->CustomerID = RT_CID_819x_CAMEO;
			break;			
			
		case EEPROM_CID_SITECOM:
			priv->CustomerID = RT_CID_819x_Sitecom;
			break;	
			
		case EEPROM_CID_COREGA:
			priv->CustomerID = RT_CID_COREGA;						
			break;			
	
		case EEPROM_CID_Senao:
			priv->CustomerID = RT_CID_819x_Senao;
			break;

		case EEPROM_CID_EDIMAX_BELKIN:
			priv->CustomerID = RT_CID_819x_Edimax_Belkin;
			break;

		case EEPROM_CID_SERCOMM_BELKIN:
			priv->CustomerID = RT_CID_819x_Sercomm_Belkin;
			break;
			
		case EEPROM_CID_WHQL:
			priv->bInHctTest = TRUE;
#ifdef TO_DO_LIST
			pMgntInfo->bSupportTurboMode = FALSE;
			pMgntInfo->bAutoTurboBy8186 = FALSE;

			pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
			pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
			pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
			pMgntInfo->keepAliveLevel = 0;

			Adapter->bUnloadDriverwhenS3S4 = FALSE;
#endif
			break;
			
		case EEPROM_CID_NetCore:
			priv->CustomerID = RT_CID_819x_Netcore;
			break;

		case EEPROM_CID_CAMEO1:
			priv->CustomerID = RT_CID_819x_CAMEO1;
			break;
			
		default:
			priv->CustomerID = RT_CID_DEFAULT;
			break;
			
	}
        printk("CustomerID = 0x%4x\n", priv->CustomerID);

	//
	// Led mode
	// 
	switch(priv->CustomerID)
	{
		case RT_CID_DEFAULT:
		case RT_CID_819x_ALPHA:
		case RT_CID_819x_CAMEO:
			priv->LedStrategy = SW_LED_MODE1;
			priv->bRegUseLed = TRUE;			
			break;	

		case RT_CID_819x_Sitecom:
			priv->LedStrategy = SW_LED_MODE2;
			priv->bRegUseLed = TRUE;		
			break;	

		case RT_CID_COREGA:
		case RT_CID_819x_Senao:
			priv->LedStrategy = SW_LED_MODE3;
			priv->bRegUseLed = TRUE;		
			break;			

		case RT_CID_819x_Edimax_Belkin:
			priv->LedStrategy = SW_LED_MODE4;
			priv->bRegUseLed = TRUE;		
			break;					

		case RT_CID_819x_Sercomm_Belkin:
			priv->LedStrategy = SW_LED_MODE5;
			priv->bRegUseLed = TRUE;		
			break;

		default:
			priv->LedStrategy = SW_LED_MODE0;
			break;			
	}



	// Read USB optional function.
	priv->EEPROMUsbOption = *(u8 *)&hwinfo[EEPROM_USB_OPTIONAL];
	
	priv->EEPROMUsbEndPointNumber = rtl8192SU_UsbOptionToEndPointNumber((priv->EEPROMUsbOption&EEPROM_EP_NUMBER)>>3);

	RT_TRACE(COMP_INIT, "USB Option = %#x\n", priv->EEPROMUsbOption);
	RT_TRACE(COMP_INIT, "EndPoint Number = %#x\n", priv->EEPROMUsbEndPointNumber);

	// Read USB PHY parameters.
	for(i=0; i<5; i++)		
		priv->EEPROMUsbPhyParam[i] = *(u8 *)&hwinfo[EEPROM_USB_PHY_PARA1+i];		
	//RT_PRINT_DATA(COMP_EFUSE, DBG_LOUD, ("USB PHY Param: \n"), pHalData->EEPROMUsbPhyParam, 5);

	
       //Read Permanent MAC address
       	//if(!priv->AutoloadFailFlag)
	//{
	//	{
	//		for(i=0; i<6; i++)
	//			dev->dev_addr[i] =  *(u8 *)&hwinfo[EEPROM_NODE_ADDRESS_BYTE_0+i];	
	//	}		
	//}
	//else
	{
	for(i=0; i<6; i++)
		dev->dev_addr[i] =  *(u8 *)&hwinfo[EEPROM_NODE_ADDRESS_BYTE_0+i];	
	}

	//NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);
	write_nic_dword(dev, IDR0, ((u32*)dev->dev_addr)[0]);
	write_nic_word(dev, IDR4, ((u16*)(dev->dev_addr + 4))[0]);

	RT_TRACE(COMP_INIT, "ReadAdapterInfo8192SEFuse(), Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
			dev->dev_addr[0], dev->dev_addr[1], 
			dev->dev_addr[2], dev->dev_addr[3], 
			dev->dev_addr[4], dev->dev_addr[5]); 
	
	//
	// Get CustomerID(Boad Type)
	// i.e., 0x0: RTL8188SU, 0x1: RTL8191SU, 0x2: RTL8192SU, 0x3: RTL8191GU.
	// Others: Reserved. Default is 0x2: RTL8192SU.
	//
	//if(!priv->AutoloadFailFlag)
	//{			
		priv->EEPROMBoardType = *(u8 *)&hwinfo[EEPROM_BoardType];	
		priv->rf_type = rtl8192SU_BoardTypeToRFtype(dev, priv->EEPROMBoardType);
	//}
	//else
	//{
	//	priv->EEPROMBoardType = EEPROM_Default_BoardType;	
	//	priv->rf_type = RF_1T2R;
	//}		
	
#if (RTL8192SU_FPGA_2MAC_VERIFICATION||RTL8192SU_ASIC_VERIFICATION)
	priv->rf_chip = RF_6052;
#else
	priv->rf_chip = RF_8256;
#endif

	priv->rf_chip = RF_6052;//lzm test
	RT_TRACE(COMP_INIT, "BoardType = 0x%2x\n", priv->EEPROMBoardType);
	RT_TRACE(COMP_INIT, "RF_Type = 0x%2x\n", priv->rf_type);
	printk("BoardType = 0x%2x\n", priv->EEPROMBoardType);
	printk("RF_Type = 0x%2x\n", priv->rf_type);

	//
	// Read antenna tx power offset of B/C/D to A  from EEPROM
	// and read ThermalMeter from EEPROM
	//
	priv->EEPROMTxPowerDiff = *(u8 *)&hwinfo[EEPROM_PwDiff];
	priv->EEPROMThermalMeter = *(u8 *)&hwinfo[EEPROM_ThermalMeter];
	RT_TRACE(COMP_INIT, "PwDiff = %#x\n", priv->EEPROMTxPowerDiff);		
	RT_TRACE(COMP_INIT, "ThermalMeter = %#x\n", priv->EEPROMThermalMeter);
	
	// Bluetooth coexistance
	tempval = *(u8 *)&hwinfo[EEPROM_BLUETOOTH_COEXIST];
	priv->EEPROMBluetoothCoexist = ((tempval&0x2)>>1);	// 92se, 0x78[1]
	priv->BluetoothCoexist = priv->EEPROMBluetoothCoexist;
	RT_TRACE(COMP_INIT, "BlueTooth Coexistance = %#x\n", priv->BluetoothCoexist);
	
	//
	// Get TSSI value for each path.
	//
	priv->EEPROMTSSI_A = *(u8 *)&hwinfo[EEPROM_TSSI_A];
	priv->EEPROMTSSI_B = *(u8 *)&hwinfo[EEPROM_TSSI_B];
			
	RT_TRACE(COMP_INIT, "TSSI_A = %#x, TSSI_B = %#x\n", priv->EEPROMTSSI_A, priv->EEPROMTSSI_B);

	//
	// Buffer TxPwIdx(i.e., from offset 0x55~0x66, total 18Bytes)
	// Update CCK, OFDM (1T/2T)Tx Power Index from above buffer.
	//
	
	//
	// Get Tx Power Level by Channel
	//
	// Read Tx power of Channel 1 ~ 14 from EFUSE.
	// 92S suupport RF A & B
	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			// Read CCK RF A & B Tx power 
			priv->RfCckChnlAreaTxPwr[rf_path][i] = 
			hwinfo[EEPROM_TxPwIndex+rf_path*3+i];

			// Read OFDM RF A & B Tx power for 1T
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][i] = 
			hwinfo[EEPROM_TxPwIndex+6+rf_path*3+i];

			// Read OFDM RF A & B Tx power for 2T
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][i] = 
			hwinfo[EEPROM_TxPwIndex+12+rf_path*3+i];
		}
	}

	//
	// Update Tx Power HAL variables.
	//
	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			RT_TRACE(COMP_EPROM, "RF-%d CCK CHan_Area-%d = 0x%x\n",  rf_path, i,priv->RfCckChnlAreaTxPwr[rf_path][i]);
			RT_TRACE(COMP_EPROM, "RF-%d OFDM-1T CHan_Area-%d = 0x%x\n",  rf_path, i,priv->RfOfdmChnlAreaTxPwr1T[rf_path][i]);
			RT_TRACE(COMP_EPROM, "RF-%d OFDM-2T CHan_Area-%d = 0x%x\n",  rf_path, i,priv->RfOfdmChnlAreaTxPwr2T[rf_path][i]);
		}
	    
		// Assign dedicated channel tx power
		for(i=0; i<14; i++)	// channel 1~3 use the same Tx Power Level.
		{
			if (i < 3)			// Cjanel 1-3
				index = 0;
			else if (i < 8)		// Channel 4-8
				index = 1;
			else				// Channel 9-14
				index = 2;

			// Record A & B CCK /OFDM - 1T/2T Channel area tx power
			priv->RfTxPwrLevelCck[rf_path][i]  = 
			priv->RfCckChnlAreaTxPwr[rf_path][index];
			priv->RfTxPwrLevelOfdm1T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][index];
			priv->RfTxPwrLevelOfdm2T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][index];

			if (rf_path == 0)
			{
				priv->TxPowerLevelOFDM24G[i] = priv->RfTxPwrLevelOfdm1T[rf_path][i] ;
				priv->TxPowerLevelCCK[i] = priv->RfTxPwrLevelCck[rf_path][i];					
			}
		}

		for(i=0; i<14; i++)
		{
			RT_TRACE(COMP_EPROM,"Rf-%d TxPwr CH-%d CCK OFDM_1T OFDM_2T= 0x%x/0x%x/0x%x\n", 
			rf_path, i, priv->RfTxPwrLevelCck[rf_path][i], priv->RfTxPwrLevelOfdm1T[rf_path][i] ,priv->RfTxPwrLevelOfdm2T[rf_path][i]);
		}
	}

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			// Read Power diff limit.
			priv->EEPROMPwrGroup[rf_path][i] = hwinfo[EEPROM_TxPWRGroup+rf_path*3+i];
		}
	}

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		// Fill Pwr group
		for(i=0; i<14; i++)
		{
			if (i < 3)			// Cjanel 1-3
				index = 0;
			else if (i < 8)		// Channel 4-8
				index = 1;
			else				// Channel 9-13
				index = 2;
			priv->PwrGroupHT20[rf_path][i] = (priv->EEPROMPwrGroup[rf_path][index]&0xf);
			priv->PwrGroupHT40[rf_path][i] = ((priv->EEPROMPwrGroup[rf_path][index]&0xf0)>>4);
			RT_TRACE(COMP_INIT, "RF-%d PwrGroupHT20[%d] = 0x%x\n", rf_path, i, priv->PwrGroupHT20[rf_path][i]);
			RT_TRACE(COMP_INIT, "RF-%d PwrGroupHT40[%d] = 0x%x\n", rf_path, i, priv->PwrGroupHT40[rf_path][i]);
		}
	}	
	
	//
	// 2009/02/09 Cosa add for new EEPROM format
	//
	for(i=0; i<14; i++)	// channel 1~3 use the same Tx Power Level.
	{
		// Read tx power difference between HT OFDM 20/40 MHZ
		if (i < 3)			// Cjanel 1-3
			index = 0;
		else if (i < 8)		// Channel 4-8
			index = 1;
		else				// Channel 9-14
			index = 2;
		
		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_HT20_DIFF+index])&0xff;
		priv->TxPwrHt20Diff[RF90_PATH_A][i] = (tempval&0xF);
		priv->TxPwrHt20Diff[RF90_PATH_B][i] = ((tempval>>4)&0xF);

		// Read OFDM<->HT tx power diff
		if (i < 3)			// Cjanel 1-3
			tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_OFDM_DIFF])&0xff;
		else if (i < 8)		// Channel 4-8
			tempval = (*(u8 *)&hwinfo[EEPROM_PwDiff])&0xff;
		else				// Channel 9-14
			tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_OFDM_DIFF+1])&0xff;

		//cosa tempval = (*(u1Byte *)&hwinfo[EEPROM_TX_PWR_OFDM_DIFF+index])&0xff;
		priv->TxPwrLegacyHtDiff[RF90_PATH_A][i] = (tempval&0xF);
		priv->TxPwrLegacyHtDiff[RF90_PATH_B][i] = ((tempval>>4)&0xF);
	}

	priv->EEPROMRegulatory = 0;
	if(priv->EEPROMVersion >= 2)
	{
		if(priv->EEPROMVersion >= 4)
			priv->EEPROMRegulatory = (hwinfo[EEPROM_Regulatory]&0x7);	//bit0~2
		else
			priv->EEPROMRegulatory = (hwinfo[EEPROM_Regulatory]&0x1);	//bit0
	}

	//Temp code for VID 0x07D1
	if( (priv->eeprom_vid== 0x07D1) && 
	     ((priv->eeprom_pid == 0x3302) ||(priv->eeprom_pid == 0x3300)))
		priv->EEPROMRegulatory = 2;

	RT_TRACE(COMP_INIT, "EEPROMRegulatory = 0x%x\n", priv->EEPROMRegulatory);

	for(i=0; i<14; i++)
		RT_TRACE(COMP_INIT, "RF-A Ht20 to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrHt20Diff[RF90_PATH_A][i]);
	for(i=0; i<14; i++)
		RT_TRACE(COMP_INIT,  "RF-A Legacy to Ht40 Diff[%d] = 0x%x\n", i, priv->TxPwrLegacyHtDiff[RF90_PATH_A][i]);
	for(i=0; i<14; i++)
		RT_TRACE(COMP_INIT,  "RF-B Ht20 to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrHt20Diff[RF90_PATH_B][i]);
	for(i=0; i<14; i++)
		RT_TRACE(COMP_INIT,  "RF-B Legacy to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrLegacyHtDiff[RF90_PATH_B][i]);

	//
	// Update remained HAL variables.
	//
	priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;
	priv->LegacyHTTxPowerDiff = priv->TxPwrLegacyHtDiff[RF90_PATH_A][0];
	//cosa pHalData->TxPowerDiff = pHalData->EEPROMTxPowerDiff;		
	priv->ThermalMeter[0] = (priv->EEPROMThermalMeter&0x1f);// ThermalMeter, [4:0]	

	// <Roger_Notes> We overwrite LED strategy if CPU utilization is considerration. 2009.06.04.
	//if(priv->bRegUsbInTokenRev)
	//priv->LedStrategy = SW_LED_MODE0;

	init_rate_adaptive(dev);

	RT_TRACE(COMP_INIT, "<==== ReadAdapterInfo8192SUsb\n");

	//return RT_STATUS_SUCCESS;
}

country_code_type_t HalMapChannelPlan8192S(struct net_device *dev, u16 HalChannelPlan)
{
	country_code_type_t	rtChannelDomain;

	switch(HalChannelPlan)
	{	
	case EEPROM_CHANNEL_PLAN_GLOBAL_DOMAIN:
		rtChannelDomain = COUNTRY_CODE_GLOBAL_DOMAIN;
		break;
	
	default:
		rtChannelDomain = (country_code_type_t)HalChannelPlan;
		break;
	}
	return 	rtChannelDomain;
}


//
//	Description:
//		Read HW adapter information by E-Fuse or EEPROM according CR9346 reported.
//
//	Assumption:
//		1. CR9346 regiser has verified.
//		2. PASSIVE_LEVEL (USB interface)
//
//	Created by Roger, 2008.10.21.
//
static void rtl8192SU_read_eeprom_info(struct net_device *dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u8			tmpU1b;	    

	RT_TRACE(COMP_INIT, "====> ReadAdapterInfo8192SUsb\n");

	// Retrieve Chip version.
	priv->card_8192_version = (VERSION_8192S)((read_nic_dword(dev, PMC_FSM)>>16)&0xF);
	RT_TRACE(COMP_INIT, "Chip Version ID: 0x%2x\n", priv->card_8192_version);
	
	tmpU1b = read_nic_byte(dev, EPROM_CMD);//CR9346	

	// To check system boot selection.
	if (tmpU1b & CmdEERPOMSEL)
	{
		RT_TRACE(COMP_INIT, "Boot from EEPROM\n");
		priv->EepromOrEfuse = TRUE;
	}
	else 
	{
		RT_TRACE(COMP_INIT, "Boot from EFUSE\n");
		priv->EepromOrEfuse = FALSE;
	}	

	// To check autoload success or not.
	if (tmpU1b & CmdEEPROM_En)
	{
		RT_TRACE(COMP_INIT, "Autoload OK!!\n"); 
		priv->AutoloadFailFlag=FALSE;		
		rtl8192SU_ReadAdapterInfo8192SUsb(dev);//eeprom or e-fuse
	}	
	else
	{ // Auto load fail.		
		RT_TRACE(COMP_INIT, "AutoLoad Fail reported from CR9346!!\n"); 
		priv->AutoloadFailFlag=TRUE;		
		rtl8192SU_ConfigAdapterInfo8192SForAutoLoadFail(dev);		

		//if (IS_BOOT_FROM_EFUSE(Adapter))
		if(!priv->EepromOrEfuse)
		{			
			RT_TRACE(COMP_INIT, "Update shadow map for EFuse future use!!\n"); 
			EFUSE_ShadowMapUpdate(dev);
		}
	}	

	if((priv->RegChannelPlan >= COUNTRY_CODE_MAX) || (priv->eeprom_ChannelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK))
	{
		priv->ChannelPlan = HalMapChannelPlan8192S(dev, (priv->eeprom_ChannelPlan & (~(EEPROM_CHANNEL_PLAN_BY_HW_MASK))));
		priv->bChnlPlanFromHW = (priv->eeprom_ChannelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK) ? TRUE : FALSE; // User cannot change  channel plan.
	}
	else
	{
		priv->ChannelPlan = (country_code_type_t)priv->RegChannelPlan;
	}
	
	switch(priv->ChannelPlan)
	{
		case COUNTRY_CODE_GLOBAL_DOMAIN:
		{
			PRT_DOT11D_INFO	pDot11dInfo = GET_DOT11D_INFO(priv->ieee80211);
	
			pDot11dInfo->bEnabled = TRUE;
		}
		RT_TRACE(COMP_INIT,"rtl8192SU_read_eeprom_info(): Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n");
			break;
		default:
			break;
	}
			
	RT_TRACE(COMP_INIT, "RegChannelPlan(%d) EEPROMChannelPlan(%d)", priv->RegChannelPlan, priv->eeprom_ChannelPlan);
	RT_TRACE(COMP_INIT, "ChannelPlan = %d\n" , priv->ChannelPlan);

	RT_TRACE(COMP_INIT, "<==== ReadAdapterInfo8192SUsb\n");

	//return RT_STATUS_SUCCESS;
}
#endif
#ifdef RTL8192U
static void rtl8192_read_eeprom_info(struct net_device* dev)
{
	u16 wEPROM_ID = 0;
	u8 bMac_Tmp_Addr[6] = {0x00, 0xe0, 0x4c, 0x00, 0x00, 0x02};
	u8 bLoad_From_EEPOM = false;
	struct r8192_priv *priv = ieee80211_priv(dev);
	u16 tmpValue = 0;

	RT_TRACE(COMP_EPROM, "===========>%s()\n", __FUNCTION__);	
	wEPROM_ID = eprom_read(dev, 0); //first read EEPROM ID out;
	RT_TRACE(COMP_EPROM, "EEPROM ID is 0x%x\n", wEPROM_ID);
	
	if (wEPROM_ID != RTL8190_EEPROM_ID)
	{
		RT_TRACE(COMP_ERR, "EEPROM ID is invalid(is 0x%x(should be 0x%x)\n", wEPROM_ID, RTL8190_EEPROM_ID);
	}
	else
		bLoad_From_EEPOM = true;

	if (bLoad_From_EEPOM)
	{
		tmpValue = eprom_read(dev, (EEPROM_VID>>1));
		priv->eeprom_vid = endian_swap(&tmpValue);
		priv->eeprom_pid = eprom_read(dev, (EEPROM_PID>>1));
		tmpValue = eprom_read(dev, (EEPROM_ChannelPlan>>1));
		priv->eeprom_ChannelPlan =((tmpValue&0xff00)>>8);
		priv->btxpowerdata_readfromEEPORM = true;
		priv->eeprom_CustomerID = eprom_read(dev, (EEPROM_Customer_ID>>1)) >>8;
	}
	else
	{
		priv->eeprom_vid = 0;
		priv->eeprom_pid = 0;
		priv->card_8192_version = VERSION_819xU_B;
		priv->eeprom_ChannelPlan = 0;
		priv->eeprom_CustomerID = 0;
	}
	RT_TRACE(COMP_EPROM, "vid:0x%4x, pid:0x%4x, CustomID:0x%2x, ChanPlan:0x%x\n", priv->eeprom_vid, priv->eeprom_pid, priv->eeprom_CustomerID, priv->eeprom_ChannelPlan);
	//set channelplan from eeprom
	if(priv->RegChannelPlan == 0xf)
	{
		priv->ChannelPlan = priv->eeprom_ChannelPlan;
	}
	else
	{
		priv->ChannelPlan = priv->RegChannelPlan;
	}

	if (bLoad_From_EEPOM)
	{
		int i;
		for (i=0; i<6; i+=2)
		{
			u16 tmp = 0;
			tmp = eprom_read(dev, (u16)((EEPROM_NODE_ADDRESS_BYTE_0 + i)>>1));
			*(u16*)(&dev->dev_addr[i]) = tmp;
		}
	}	
	else
	{
		memcpy(dev->dev_addr, bMac_Tmp_Addr, 6);
		//should I set IDR0 here?
	}
	RT_TRACE(COMP_EPROM, "MAC addr:"MAC_FMT"\n", MAC_ARG(dev->dev_addr));
	priv->rf_type = RTL819X_DEFAULT_RF_TYPE; //default 1T2R
	priv->rf_chip = RF_8256;
	
	if (priv->card_8192_version == (u8)VERSION_819xU_A)
	{
		//read Tx power gain offset of legacy OFDM to HT rate
		if (bLoad_From_EEPOM)
			priv->EEPROMTxPowerDiff = (eprom_read(dev, (EEPROM_TxPowerDiff>>1))&0xff00) >> 8;
		else
			priv->EEPROMTxPowerDiff = EEPROM_Default_TxPower;
		RT_TRACE(COMP_EPROM, "TxPowerDiff:%d\n", priv->EEPROMTxPowerDiff);

		//2 Read Tx Power Track enable/disable
		if (bLoad_From_EEPOM)
			tmpValue = eprom_read(dev,(EEPROM_TxPwrTrack>>1)) & 0x3;
		else
			tmpValue = 0;
		RT_TRACE(COMP_POWER_TRACKING, "EEPROM_TxPwrTrack = 0x%x, ", tmpValue);
		if(tmpValue == 1) //[1:0] = 01b
			priv->EEPROMTxPowerTrackEnable = TRUE;
		else
			priv->EEPROMTxPowerTrackEnable = FALSE;
		RT_TRACE(COMP_POWER_TRACKING, "Tx Power Track enable = %d\n", priv->EEPROMTxPowerTrackEnable);
		
		//read ThermalMeter from EEPROM
		if (bLoad_From_EEPOM)
			priv->EEPROMThermalMeter = (u8)(eprom_read(dev, (EEPROM_ThermalMeter>>1))&0x00ff);
		else
			priv->EEPROMThermalMeter = EEPROM_Default_ThermalMeter;
		RT_TRACE(COMP_EPROM, "ThermalMeter:%d\n", priv->EEPROMThermalMeter);

		//vivi, for tx power track
		priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;

		//read antenna tx power offset of B/C/D to A from EEPROM
		if (bLoad_From_EEPOM)
			priv->EEPROMPwDiff = (eprom_read(dev, (EEPROM_PwDiff>>1))&0x0f00)>>8;
		else
			priv->EEPROMPwDiff = EEPROM_Default_PwDiff;
		RT_TRACE(COMP_EPROM, "TxPwDiff:%d\n", priv->EEPROMPwDiff);

		// Read CrystalCap from EEPROM
		if (bLoad_From_EEPOM)
			priv->EEPROMCrystalCap = (eprom_read(dev, (EEPROM_CrystalCap>>1))&0x0f);
		else
			priv->EEPROMCrystalCap = EEPROM_Default_CrystalCap;
		RT_TRACE(COMP_EPROM, "CrystalCap = %d\n", priv->EEPROMCrystalCap);

		//get per-channel Tx power level
		if (bLoad_From_EEPOM)
			priv->EEPROM_Def_Ver = (eprom_read(dev, (EEPROM_TxPwIndex_Ver>>1))&0xff00)>>8;
		else
			priv->EEPROM_Def_Ver = 1;
		RT_TRACE(COMP_EPROM, "EEPROM_DEF_VER:%d\n", priv->EEPROM_Def_Ver);

		if (priv->EEPROM_Def_Ver == 0) //old eeprom definition
		{
			int i;
			if (bLoad_From_EEPOM)
				priv->EEPROMTxPowerLevelCCK = (eprom_read(dev, (EEPROM_TxPwIndex_CCK>>1))&0xff) >> 8; 
			else
				priv->EEPROMTxPowerLevelCCK = 0x10;
			RT_TRACE(COMP_EPROM, "CCK Tx Power Levl: 0x%02x\n", priv->EEPROMTxPowerLevelCCK);
			for (i=0; i<3; i++)
			{
				if (bLoad_From_EEPOM)
				{
					tmpValue = eprom_read(dev, (EEPROM_TxPwIndex_OFDM_24G+i)>>1);
					if (((EEPROM_TxPwIndex_OFDM_24G+i) % 2) == 0)
						tmpValue = tmpValue & 0x00ff;
					else
						tmpValue = (tmpValue & 0xff00) >> 8;
				}
				else
					tmpValue = 0x10;
				priv->EEPROMTxPowerLevelOFDM24G[i] = (u8) tmpValue;
				RT_TRACE(COMP_EPROM, "OFDM 2.4G Tx Power Level, Index %d = 0x%02x\n", i, priv->EEPROMTxPowerLevelCCK);
			}
		}//end if EEPROM_DEF_VER == 0
		else if (priv->EEPROM_Def_Ver == 1)
		{
			if (bLoad_From_EEPOM)
			{
				tmpValue = eprom_read(dev, (EEPROM_TxPwIndex_CCK_V1>>1));
				tmpValue = (tmpValue & 0xff00) >> 8;
			}
			else
				tmpValue = 0x10;
			priv->EEPROMTxPowerLevelCCK_V1[0] = (u8)tmpValue;

			if (bLoad_From_EEPOM)
				tmpValue = eprom_read(dev, (EEPROM_TxPwIndex_CCK_V1 + 2)>>1);
			else
				tmpValue = 0x1010;
			*((u16*)(&priv->EEPROMTxPowerLevelCCK_V1[1])) = tmpValue;
			if (bLoad_From_EEPOM)
				tmpValue = eprom_read(dev, (EEPROM_TxPwIndex_OFDM_24G_V1>>1));
			else
				tmpValue = 0x1010;
			*((u16*)(&priv->EEPROMTxPowerLevelOFDM24G[0])) = tmpValue;
			if (bLoad_From_EEPOM)
				tmpValue = eprom_read(dev, (EEPROM_TxPwIndex_OFDM_24G_V1+2)>>1);
			else
				tmpValue = 0x10;
			priv->EEPROMTxPowerLevelOFDM24G[2] = (u8)tmpValue;
		}//endif EEPROM_Def_Ver == 1

		//update HAL variables
		//
		{
			int i;
			for (i=0; i<14; i++)
			{
				if (i<=3)
					priv->TxPowerLevelOFDM24G[i] = priv->EEPROMTxPowerLevelOFDM24G[0];
				else if (i>=4 && i<=9)
					priv->TxPowerLevelOFDM24G[i] = priv->EEPROMTxPowerLevelOFDM24G[1];
				else 
					priv->TxPowerLevelOFDM24G[i] = priv->EEPROMTxPowerLevelOFDM24G[2];
			}
			
			for (i=0; i<14; i++)
			{
				if (priv->EEPROM_Def_Ver == 0)
				{
					if (i<=3)
						priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelOFDM24G[0] + (priv->EEPROMTxPowerLevelCCK - priv->EEPROMTxPowerLevelOFDM24G[1]);
					else if (i>=4 && i<=9)
						priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK;
					else
						priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelOFDM24G[2] + (priv->EEPROMTxPowerLevelCCK - priv->EEPROMTxPowerLevelOFDM24G[1]);
				}
				else if (priv->EEPROM_Def_Ver == 1)
				{
					if (i<=3)
						priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK_V1[0];
					else if (i>=4 && i<=9)
						priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK_V1[1];
					else 
						priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK_V1[2];
				}
			}
		}//end update HAL variables
		priv->TxPowerDiff = priv->EEPROMPwDiff;
// Antenna B gain offset to antenna A, bit0~3
		priv->AntennaTxPwDiff[0] = (priv->EEPROMTxPowerDiff & 0xf);
		// Antenna C gain offset to antenna A, bit4~7
		priv->AntennaTxPwDiff[1] = ((priv->EEPROMTxPowerDiff & 0xf0)>>4);
		// CrystalCap, bit12~15
		priv->CrystalCap = priv->EEPROMCrystalCap;
#if 0//cosa removed for we don't have pg thermal meter value.
		// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
		// 92U does not enable TX power tracking.
		priv->ThermalMeter[0] = priv->EEPROMThermalMeter;
#endif
	}//end if VersionID == VERSION_819xU_A


	//
	//  Decide CustomerID according to VID/DID or EEPROM
	//
	switch(priv->eeprom_CustomerID)
	{
		case EEPROM_CID_RUNTOP:
			priv->CustomerID = RT_CID_819x_RUNTOP;
			break;
			
		case EEPROM_CID_DLINK:
			priv->CustomerID = RT_CID_DLINK;
			break;
			
		case EEPROM_CID_WHQL:
			priv->bInHctTest = TRUE;
#ifdef TO_DO_LIST
			pMgntInfo->bSupportTurboMode = FALSE;
			pMgntInfo->bAutoTurboBy8186 = FALSE;

			pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
			pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
			pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
			pMgntInfo->keepAliveLevel = 0;
#endif
			break;
			
		case EEPROM_CID_NetCore:
			priv->CustomerID = RT_CID_819x_Netcore;
			break;

		default:
			priv->CustomerID = RT_CID_DEFAULT;
			break;
			
	}
	if( priv->eeprom_vid == 0xaa07 &&  priv->eeprom_pid == 0x4300 )
	{
		priv->CustomerID =  RT_CID_COREGA;
	}

	switch(priv->CustomerID)
	{
		case RT_CID_819x_RUNTOP:
			priv->LedStrategy = SW_LED_MODE2;
			break;
			
 		case RT_CID_DLINK:
			priv->LedStrategy = SW_LED_MODE4;
			break;

		default:
			priv->LedStrategy = SW_LED_MODE0;
			break;
			
	}


	if(priv->rf_type == RF_1T2R)
	{		
		RT_TRACE(COMP_EPROM, "\n1T2R config\n");
	}
	else
	{
		RT_TRACE(COMP_EPROM, "\n2T4R config\n");
	}
	
	// 2008/01/16 MH We can only know RF type in the function. So we have to init
	// DIG RATR table again.
	init_rate_adaptive(dev);
	//we need init DIG RATR table here again.

	RT_TRACE(COMP_EPROM, "<===========%s()\n", __FUNCTION__);
	return;	
}


void HalInitUsbRxAggr819xUsb(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	PRT_HIGH_THROUGHPUT	pHTInfo = priv->ieee80211->pHTInfo;
	bool		bUsbRxAggr = FALSE, bUsbAggrBulkin = FALSE;
	u8		IC_Cut;

	if(udev->speed != USB_SPEED_HIGH)
	{
		// Alway set Rx Aggregation to Disable if current platform is Win2K USB 1.1, by Emily
		bUsbRxAggr = FALSE;
		bUsbAggrBulkin = FALSE;
	}
	else
	{
		IC_Cut = read_nic_byte(dev, IC_VERRSION);
#ifdef USB_RX_AGGREGATION_SUPPORT
		bUsbRxAggr = pHTInfo->UsbRxFwAggrEn;
#endif
#if USB_AGGR_BULKIN
		bUsbAggrBulkin = priv->bRegUsbAggrBulkinEnable;
#endif

		if(IC_Cut >= IC_VersionCut_D)
		{
			if(bUsbAggrBulkin)
				bUsbRxAggr = FALSE;
		}
		else
		{
			bUsbAggrBulkin = FALSE;
		}
	}

	if(bUsbRxAggr && bUsbAggrBulkin)
		printk("HalInitUsbRxAggr819xUsb(): 2 USB Aggr method cannot be turned on at the same time!!\n");

	if(bUsbAggrBulkin)
		priv->CurUsbAggrMode = 2;
	else if(bUsbRxAggr)
		priv->CurUsbAggrMode = 1;
	else 
		priv->CurUsbAggrMode = 0;

	// Initial value setting.
	priv->CurReg0xfe38 = read_nic_byte_E(dev, 0x38);
	priv->CurUsbAggrSetting = read_nic_dword(dev, USB_RX_AGGR);
#ifdef USB_RX_AGGREGATION_SUPPORT
	priv->bCurrentRxAggrEnable = FALSE;
#endif
#if USB_AGGR_BULKIN
	priv->bCurUsbAggrBulkinEnable = FALSE;
#endif
	
	RT_TRACE(COMP_RECV, "HalInitUsbRxAggr819xUsb():  Set RxAggregation Mode: %d\n", priv->CurUsbAggrMode);
}


void HalUsbRxAggr819xUsb(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_HIGH_THROUGHPUT	pHTInfo = priv->ieee80211->pHTInfo;

	// Turn On Rx Aggregation by default.
	bool		Value = TRUE;
	u8		tmp = 0;

	// Always set Rx Aggregation to Disable if connected AP is Realtek AP, by Joseph
	if(pHTInfo->bCurrentRT2RTAggregation)
		Value = FALSE;

	// set Rx Aggregation to Disable if using AES encryption/decryption temporarily, by Guangan
	// Re-enable by Emily. 2008.06.30
	//if ((pSecInfo->PairwiseEncAlgorithm == AESCCMP_Encryption) || (pSecInfo->GroupEncAlgorithm == AESCCMP_Encryption))
	//	Value = FALSE;

	// The RX aggregation may be enabled/disabled dynamically according current traffic stream. 
	//Enable Rx aggregation if downlink traffic is busier than uplink traffic. by Guangan

	if(priv->CurUsbAggrMode == 1)
	{
#ifdef USB_RX_AGGREGATION_SUPPORT
		if(priv->bCurrentRxAggrEnable != Value)
		{
			priv->bCurrentRxAggrEnable = Value;
			priv->bCurDmRxAggrOn = Value;
			tmp = ((Value)?1:0);
			priv->ieee80211->SetHwRegHandler(dev, HW_VAR_USB_RX_AGGR, &tmp);
		}
#endif
	}
	else if(priv->CurUsbAggrMode == 2)
	{
#if USB_AGGR_BULKIN
		if(priv->bCurUsbAggrBulkinEnable != Value)
		{
			priv->bCurUsbAggrBulkinEnable = Value;
			tmp = ((Value)?2:0);
			priv->ieee80211->SetHwRegHandler(dev, HW_VAR_USB_RX_AGGR, &tmp);
		}
#endif	
	}

	RT_TRACE(COMP_RECV,  "HalUsbRxAggr819xUsb() :  Set RxAggregation to %d\n", tmp);
}
#endif

short rtl8192_get_channel_map(struct net_device * dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
#ifdef ENABLE_DOT11D
	if(priv->ChannelPlan > COUNTRY_CODE_MAX){
		printk("rtl8180_init:Error channel plan! Set to default.\n");
		priv->ChannelPlan= COUNTRY_CODE_FCC;
	}
	RT_TRACE(COMP_INIT, "Channel plan is %d\n",priv->ChannelPlan);
		
	Dot11d_Init(priv->ieee80211);
	rtl819x_set_channel_map(priv->ChannelPlan, priv);
#else
	int ch,i;
	//Set Default Channel Plan
	if(!channels){
		DMESG("No channels, aborting");
		return -1;
	}
	ch=channels;
	priv->ChannelPlan= 0;//hikaru
	 // set channels 1..14 allowed in given locale
	for (i=1; i<=14; i++) {
		(priv->ieee80211->channel_map)[i] = (u8)(ch & 0x01);
		ch >>= 1;
	}
#endif
	return 0;
}

short rtl8192_init(struct net_device *dev)
{
		
	struct r8192_priv *priv = ieee80211_priv(dev);

	rtl8192_init_priv_variable(dev);
	rtl8192_init_priv_lock(priv);
	rtl8192_init_priv_task(dev);
	rtl8192_get_eeprom_size(dev);
	priv->ops->rtl819x_read_eeprom_info(dev);
	rtl8192_get_channel_map(dev);
	init_hal_dm(dev);
#ifdef RTL8192SU
        InitSwLeds(dev);
#endif
	init_timer(&priv->watch_dog_timer);
	priv->watch_dog_timer.data = (unsigned long)dev;
	priv->watch_dog_timer.function = watch_dog_timer_callback;
	
	//rtl8192_adapter_start(dev);
#ifdef DEBUG_EPROM
	dump_eprom(dev);
#endif 
	return 0;
}

// *****************************************************************************
//function:  This function actually only set RRSR, RATR and BW_OPMODE registers 
//	     not to do all the hw config as its name says
//   input:  net_device dev
//  output:  none
//  return:  none
//  notice:  This part need to modified according to the rate set we filtered
// ****************************************************************************/
void rtl8192_hwconfig(struct net_device* dev)
{
	u32 regRATR = 0, regRRSR = 0;
	u8 regBwOpMode = 0, regTmp = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);

// Set RRSR, RATR, and BW_OPMODE registers
	//
	switch(priv->ieee80211->mode)
	{
	case WIRELESS_MODE_B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK;
		regRRSR = RATE_ALL_CCK;
		break;
	case WIRELESS_MODE_A:
		regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_G:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_AUTO:
		if (priv->bInHctTest)
		{
		    regBwOpMode = BW_OPMODE_20MHZ;
		    regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		    regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		}
		else
		{
		    regBwOpMode = BW_OPMODE_20MHZ;
		    regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		    regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		}
		break;
	case WIRELESS_MODE_N_24G:
		// It support CCK rate by default.
		// CCK rate will be filtered out only when associated AP does not support it.
		regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_N_5G:
		regBwOpMode = BW_OPMODE_5G;
		regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	}

	write_nic_byte(dev, BW_OPMODE, regBwOpMode);
	{
		u32 ratr_value = 0;
		ratr_value = regRATR;
		if (priv->rf_type == RF_1T2R)
		{
			ratr_value &= ~(RATE_ALL_OFDM_2SS);
		}
		write_nic_dword(dev, RATR0, ratr_value);
		write_nic_byte(dev, UFWP, 1);
	}	
	regTmp = read_nic_byte(dev, 0x313);
	regRRSR = ((regTmp) << 24) | (regRRSR & 0x00ffffff);
	write_nic_dword(dev, RRSR, regRRSR);

	//
	// Set Retry Limit here
	//
	write_nic_word(dev, RETRY_LIMIT, 
			priv->ShortRetryLimit << RETRY_LIMIT_SHORT_SHIFT | \
			priv->LongRetryLimit << RETRY_LIMIT_LONG_SHIFT);
	// Set Contention Window here		

	// Set Tx AGC

	// Set Tx Antenna including Feedback control
		
	// Set Auto Rate fallback control
	
	
}

#ifdef RTL8192SU
//added by lzm 090402
void SetHwReg8192SU(struct net_device *dev,u8 variable,u8* val)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	switch(variable)
	{
		case HW_VAR_AMPDU_MIN_SPACE:
		{
			u8	MinSpacingToSet;
			u8	SecMinSpace;

			MinSpacingToSet = *((u8*)val);
			if(MinSpacingToSet <= 7)
			{
				if((priv->ieee80211->current_network.capability & WLAN_CAPABILITY_PRIVACY) == 0)  //NO_Encryption
					SecMinSpace = 0;
				//else if(priv->ieee80211->is_ap_in_wep_tkip(dev))//FIXLZM
				//	SecMinSpace = 6;
				else
					SecMinSpace = 6;
				
				if(MinSpacingToSet < SecMinSpace)
					MinSpacingToSet = SecMinSpace;
				priv->ieee80211->MinSpaceCfg = ((priv->ieee80211->MinSpaceCfg&0xf8) |MinSpacingToSet);
				RT_TRACE(COMP_MLME, "Set HW_VAR_AMPDU_MIN_SPACE: %#x\n", priv->ieee80211->MinSpaceCfg);
				write_nic_byte(dev, AMPDU_MIN_SPACE, priv->ieee80211->MinSpaceCfg);	
			}
		}		
		break;	
		case HW_VAR_SHORTGI_DENSITY:
		{
			u8	DensityToSet;
		
			DensityToSet = *((u8*)val);		
			DensityToSet &= 0x1f;
			priv->ieee80211->MinSpaceCfg &= 0x07; // Clear bit[7:3]
			RT_TRACE(COMP_MLME, "HW_VAR_SHORTGI_DENSITY: %#x, DensityToSet: %#x\n", priv->ieee80211->MinSpaceCfg, DensityToSet);
			priv->ieee80211->MinSpaceCfg|= (DensityToSet<<3);
			RT_TRACE(COMP_MLME, "Set HW_VAR_SHORTGI_DENSITY: %#x\n", priv->ieee80211->MinSpaceCfg);
			write_nic_byte(dev, AMPDU_MIN_SPACE, priv->ieee80211->MinSpaceCfg);
			break;		
		}
		case HW_VAR_AMPDU_FACTOR:
		{
			u8	FactorToSet;
			u8	RegToSet;
			u8	FactorLevel[18] = {2, 4, 4, 7, 7, 13, 13, 13, 2, 7, 7, 13, 13, 15, 15, 15, 15, 0};
			u8	index = 0;
		
			FactorToSet = *((u8*)val);
			if(FactorToSet <= 3)
			{
				FactorToSet = (1<<(FactorToSet + 2));
				if(FactorToSet>0xf)
					FactorToSet = 0xf;

				for(index=0; index<17; index++)
				{
					if(FactorLevel[index] > FactorToSet)
						FactorLevel[index] = FactorToSet;
				}

				for(index=0; index<8; index++)
				{
					RegToSet = ((FactorLevel[index*2]) | (FactorLevel[index*2+1]<<4));
					write_nic_byte(dev, AGGLEN_LMT_L+index, RegToSet);
				}
				RegToSet = ((FactorLevel[16]) | (FactorLevel[17]<<4));
				write_nic_byte(dev, AGGLEN_LMT_H, RegToSet);

				RT_TRACE(COMP_MLME, "Set HW_VAR_AMPDU_FACTOR: %#x\n", FactorToSet);
			}
		}
		break;
		default:
			break;
	}
}

//
//	Description:
//		Initial HW relted registers.
//
//	Assumption:
//		Config RTL8192S USB MAC, we should config MAC before download FW.
//
//	2008.09.03, Added by Roger.
//
static void rtl8192SU_MacConfigBeforeFwDownloadASIC(struct net_device *dev)
{
	u8				tmpU1b;// i;
	u8				PollingCnt = 20;
	
	RT_TRACE(COMP_INIT, "--->MacConfigBeforeFwDownloadASIC()\n");
	
	//2MAC Initialization for power on sequence, Revised by Roger. 2008.09.03.

	//
	//<Roger_Notes> Set control path switch to HW control and reset Digital Core,  CPU Core and 
	// MAC I/O to solve FW download fail when system from resume sate.
	// 2008.11.04.
	//
       tmpU1b = read_nic_byte(dev, SYS_CLKR+1);
       if(tmpU1b & 0x80)
	{
       	tmpU1b &= 0x3f;
              write_nic_byte(dev, SYS_CLKR+1, tmpU1b);
       }
	// Clear FW RPWM for FW control LPS. by tynli. 2009.02.23
	write_nic_byte_E(dev, RPWM, 0x0);
	
       tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
       tmpU1b &= 0x73;
       write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b);
       udelay(1000);

	//Revised POS, suggested by SD1 Alex, 2008.09.27.
	write_nic_byte(dev, SPS0_CTRL+1, 0x53);
	write_nic_byte(dev, SPS0_CTRL, 0x57);

	//Enable AFE Macro Block's Bandgap adn Enable AFE Macro Block's Mbias
	tmpU1b = read_nic_byte(dev, AFE_MISC);	
	write_nic_byte(dev, AFE_MISC, (tmpU1b|AFE_BGEN)); //Bandgap
	write_nic_byte(dev, AFE_MISC, (tmpU1b|AFE_BGEN|AFE_MBEN)); //Mbios

	//Enable PLL Power (LDOA15V)
	tmpU1b = read_nic_byte(dev, LDOA15_CTRL);	
	write_nic_byte(dev, LDOA15_CTRL, (tmpU1b|LDA15_EN));

	//Enable LDOV12D block
	tmpU1b = read_nic_byte(dev, LDOV12D_CTRL);	
	write_nic_byte(dev, LDOV12D_CTRL, (tmpU1b|LDV12_EN));	
	
	tmpU1b = read_nic_byte(dev, SYS_ISO_CTRL+1);	
	write_nic_byte(dev, SYS_ISO_CTRL+1, (tmpU1b|0x08));

	//Engineer Packet CP test Enable
	tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);	
	write_nic_byte(dev, SYS_FUNC_EN+1, (tmpU1b|0x20));

	//Support 64k IMEM, suggested by SD1 Alex.
	tmpU1b = read_nic_byte(dev, SYS_ISO_CTRL+1);	
	write_nic_byte(dev, SYS_ISO_CTRL+1, (tmpU1b& 0x68));

	//Enable AFE clock
	tmpU1b = read_nic_byte(dev, AFE_XTAL_CTRL+1);	
	write_nic_byte(dev, AFE_XTAL_CTRL+1, (tmpU1b& 0xfb));

	//Enable AFE PLL Macro Block
	tmpU1b = read_nic_byte(dev, AFE_PLL_CTRL);	
	write_nic_byte(dev, AFE_PLL_CTRL, (tmpU1b|0x11));

	//Attatch AFE PLL to MACTOP/BB/PCIe Digital
	tmpU1b = read_nic_byte(dev, SYS_ISO_CTRL);	
	write_nic_byte(dev, SYS_ISO_CTRL, (tmpU1b&0xEE));

	// Switch to 40M clock
	write_nic_byte(dev, SYS_CLKR, 0x00);
	
	//CPU Clock and 80M Clock SSC Disable to overcome FW download fail timing issue.
	tmpU1b = read_nic_byte(dev, SYS_CLKR);	
	write_nic_byte(dev, SYS_CLKR, (tmpU1b|0xa0));

	//Enable MAC clock
	tmpU1b = read_nic_byte(dev, SYS_CLKR+1);	
	write_nic_byte(dev, SYS_CLKR+1, (tmpU1b|0x18));

	//Revised POS, suggested by SD1 Alex, 2008.09.27.
	write_nic_byte(dev, PMC_FSM, 0x02);
	
	//Enable Core digital and enable IOREG R/W
	tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);	
	write_nic_byte(dev, SYS_FUNC_EN+1, (tmpU1b|0x08));

	//Enable REG_EN
	tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);	
	write_nic_byte(dev, SYS_FUNC_EN+1, (tmpU1b|0x80));

	//Switch the control path to FW
	tmpU1b = read_nic_byte(dev, SYS_CLKR+1);	
	write_nic_byte(dev, SYS_CLKR+1, (tmpU1b|0x80)& 0xBF);
	
	write_nic_byte(dev, CMDR, 0xFC);
	write_nic_byte(dev, CMDR+1, 0x37);

	//Fix the RX FIFO issue(usb error), 970410
	tmpU1b = read_nic_byte_E(dev, 0x5c);	
	write_nic_byte_E(dev, 0x5c, (tmpU1b|BIT7));
		
	 //For power save, used this in the bit file after 970621
	tmpU1b = read_nic_byte(dev, SYS_CLKR);	
	write_nic_byte(dev, SYS_CLKR, tmpU1b&(~SYS_CPU_CLKSEL));	
	
	// Revised for 8051 ROM code wrong operation. Added by Roger. 2008.10.16. 
	write_nic_byte_E(dev, 0x1c, 0x80);

	//
	// <Roger_EXP> To make sure that TxDMA can ready to download FW.
	// We should reset TxDMA if IMEM RPT was not ready.
	// Suggested by SD1 Alex. 2008.10.23.
	//
	do
	{
		tmpU1b = read_nic_byte(dev, TCR);
		if((tmpU1b & TXDMA_INIT_VALUE) == TXDMA_INIT_VALUE)
			break;	
		//PlatformStallExecution(5);	
		udelay(5);
	}while(PollingCnt--);	// Delay 1ms
	
	if(PollingCnt <= 0 )
	{
		RT_TRACE(COMP_INIT, "MacConfigBeforeFwDownloadASIC(): Polling TXDMA_INIT_VALUE timeout!! Current TCR(%#x)\n", tmpU1b);
		tmpU1b = read_nic_byte(dev, CMDR);			
		write_nic_byte(dev, CMDR, tmpU1b&(~TXDMA_EN));
		udelay(2);
		write_nic_byte(dev, CMDR, tmpU1b|TXDMA_EN);// Reset TxDMA
	}
	
	
	RT_TRACE(COMP_INIT, "<---MacConfigBeforeFwDownloadASIC()\n");
}

#ifdef USB_RX_AGGREGATION_SUPPORT
void rtl8192SU_HalUsbRxAggr8192SUsb(struct net_device *dev, bool Value)
{
	struct r8192_priv *priv = ieee80211_priv((struct net_device *)dev);
	PRT_HIGH_THROUGHPUT	pHTInfo = priv->ieee80211->pHTInfo;;

	//
	// <Roger_Notes> We decrease Rx page aggregated threshold in B/G mode.
	// 2008.10.29
	//
	if((priv->ieee80211->mode & WIRELESS_MODE_B) || (priv->ieee80211->mode & WIRELESS_MODE_G))
	{// Overwrite current settings to disable Rx Aggregation.
		Value = false;
	}

	// Alway set Rx Aggregation to Disable if current platform is Win2K USB 1.1, by Emily
	//if(PLATFORM_LIMITED_RX_BUF_SIZE(Adapter))
	//	Value = FALSE;

	// Always set Rx Aggregation to Disable if connected AP is Realtek AP, by Joseph
	//if(pHTInfo->bCurrentRT2RTAggregation)
	//	Value = FALSE;

	// The RX aggregation may be enabled/disabled dynamically according current traffic stream. 
	//Enable Rx aggregation if downlink traffic is busier than uplink traffic. by Guangan
	if(priv->bCurrentRxAggrEnable != Value)
	{
		priv->bCurrentRxAggrEnable = Value;
		//Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_USB_RX_AGGR, (pu1Byte)&pHalData->bCurrentRxAggrEnable);
		{			
			//u8 	Setting = ((pu1Byte)(val))[0];
			u8 	Setting = priv->bCurrentRxAggrEnable;
			u32 	ulValue;

			if(Setting==0)
			{
				//
				// <Roger_Notes> Reduce aggregated page threshold to 0x01 and set minimal threshold 0x0a. 
				// i.e., disable Rx aggregation.
				//
				ulValue = 0x0001000a;				

				//Clear Rx aggregation control and discard dummy bytes padding. Added by Roger, 2009.04.10.
				//priv->RxDMACtrl &= ~RXDMA_AGG_EN;
				printk("rtl8192SU_HalUsbRxAggr8192SUsb: Disable RxAggregation -----\n");
			}
			else
			{
				//PRT_HIGH_THROUGHPUT	pHTInfo = priv->ieee80211->pHTInfo;
				//HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);

				if (priv->bForcedUsbRxAggr)
				{// Using forced settings.					
					ulValue = priv->ForcedUsbRxAggrInfo;
				}
				else
				{// Using default settings.
					
					ulValue = (pHTInfo->UsbRxFwAggrEn<<24) | (pHTInfo->UsbRxFwAggrPageNum<<16) | 
							(pHTInfo->UsbRxFwAggrPacketNum<<8) | (pHTInfo->UsbRxFwAggrTimeout);
				}

				//Set Rx aggregation control and accept dummy bytes padding. Added by Roger, 2009.04.10.
				priv->RxDMACtrl |= RXDMA_AGG_EN;
				printk("rtl8192SU_HalUsbRxAggr8192SUsb: Enable RxAggregation +++++\n");
			}	

			write_nic_byte(dev, RXDMA_AGG_PG_TH, (u8)((ulValue&0xff0000)>>16));
			write_nic_byte_E(dev, USB_RX_AGG_TIMEOUT, (u8)(ulValue&0xff));
			//write_nic_byte(dev, RXDMA, priv->RxDMACtrl); // Set Rx DMA control.

			priv->LastUsbRxAggrInfoSetting = ulValue;
			
			RT_TRACE(COMP_RECV, "Set HW_VAR_USB_RX_AGGR: ulValue(%x)\n", ulValue);
		}
		RT_TRACE(COMP_RECV, "HalUsbRxAggr8192SUsb() :  Set RxAggregation to %s\n", Value?"ON":"OFF");
	}
}

u8 rtl8192SU_MapRxPageSizeToIdx(u16 RxPktSize	)
{
	switch(RxPktSize)
	{
		case 64:		return 0; break;
		case 128	:	return 1; break;
		case 256:		return 2; break;
		case 512:		return 3; break;
		case 1024:	return 4; break;		
		default:		return 0;	// We use 64bytes in defult.
	}	
}
#endif


//
//	Description:
//		Initial HW relted registers.
//
//	Assumption:
//		1. This function is only invoked at driver intialization once.
//		2. PASSIVE LEVEL.
//
//	2008.06.10, Added by Roger.
//
static void rtl8192SU_MacConfigAfterFwDownload(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv((struct net_device *)dev);
	//PRT_HIGH_THROUGHPUT	pHTInfo = priv->ieee80211->pHTInfo;
	u8	tmpU1b, RxPageCfg;


	RT_TRACE(COMP_INIT, "--->MacConfigAfterFwDownload()\n");

	// Enable Tx/Rx
	tmpU1b =(u8)( BBRSTn|BB_GLB_RSTn|SCHEDULE_EN|MACRXEN|MACTXEN|DDMA_EN|
			FW2HW_EN|RXDMA_EN|TXDMA_EN|HCI_RXDMA_EN|HCI_TXDMA_EN);
	//Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_COMMAND, &tmpU1b );
	write_nic_byte(dev, CMDR, tmpU1b);
	
	// Loopback mode or not	
	priv->LoopbackMode = RTL8192SU_NO_LOOPBACK; // Set no loopback as default.	
	if(priv->LoopbackMode == RTL8192SU_NO_LOOPBACK)	
		tmpU1b = LBK_NORMAL;			
	else if (priv->LoopbackMode == RTL8192SU_MAC_LOOPBACK )
		tmpU1b = LBK_MAC_DLB;		
	else
		RT_TRACE(COMP_INIT, "Serious error: wrong loopback mode setting\n");	

	//Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_LBK_MODE, &tmpU1b);	
	write_nic_byte(dev, LBKMD_SEL, tmpU1b);
	
	// Set RCR
	write_nic_dword(dev, RCR, priv->ReceiveConfig);
	RT_TRACE(COMP_INIT, "MacConfigAfterFwDownload(): Current RCR settings(%#x)\n", priv->ReceiveConfig);
	

	// Set RQPN
	//
	// <Roger_Notes> 2008.08.18.
	// 6 endpoints:
	// (1) Page number on CMDQ is 0x03.
	// (2) Page number on BCNQ, HQ and MGTQ is 0.
	// (3) Page number on BKQ, BEQ, VIQ and VOQ are 0x07.
	// (4) Page number on PUBQ is 0xdd
	//
	// 11 endpoints:
	// (1) Page number on CMDQ is 0x00.
	// (2) Page number on BCNQ is 0x02, HQ and MGTQ are 0x03.
	// (3) Page number on BKQ, BEQ, VIQ and VOQ are 0x07.
	// (4) Page number on PUBQ is 0xd8
	//	

#ifdef USB_RX_AGGREGATION_SUPPORT
	priv->bCurrentRxAggrEnable = false;
	// Size of Tx/Rx packet buffer.			
	tmpU1b = read_nic_byte(dev, PBP);
	RxPageCfg = rtl8192SU_MapRxPageSizeToIdx(priv->ieee80211->pHTInfo->UsbRxPageSize);

	write_nic_byte(dev, PBP, tmpU1b|RxPageCfg); // Set page size of Rx packet buffer to 128 bytes.
	priv->RxDMACtrl = read_nic_byte(dev, RXDMA);
	priv->RxDMACtrl |= RXDMA_AGG_EN;
	write_nic_byte(dev, RXDMA, priv->RxDMACtrl); // Set Rx DMA control.
		
#ifndef WIFI_TEST
	rtl8192SU_HalUsbRxAggr8192SUsb(dev, true);	
#else
	rtl8192SU_HalUsbRxAggr8192SUsb(dev, false);	
#endif

#endif
	
	// Fix the RX FIFO issue(USB error), Rivesed by Roger, 2008-06-14
	tmpU1b = read_nic_byte_E(dev, 0x5C);
	write_nic_byte_E(dev, 0x5C, tmpU1b|BIT7);		

	//
	// Revise USB PHY to solve the issue of Rx payload error, Rivesed by Roger,  2008-04-10
	// Suggest by SD1 Alex.
	//
	// <Roger_Notes> The following operation are ONLY for USB PHY test chip. 
	// 2008.10.07.
	//
#if RTL8192SU_USB_PHY_TEST
	write_nic_byte_E(dev, 0x41,0xf4);	
	write_nic_byte_E(dev, 0x40,0x00);	
	write_nic_byte_E(dev, 0x42,0x00);
	write_nic_byte_E(dev, 0x42,0x01);
	write_nic_byte_E(dev, 0x40,0x0f);
	write_nic_byte_E(dev, 0x42,0x00);	
	write_nic_byte_E(dev, 0x42,0x01);
#endif

	// For EFUSE init configuration.
	//if (IS_BOOT_FROM_EFUSE(Adapter))	// We may R/W EFUSE in EFUSE mode
	if (priv->bBootFromEfuse)
	{	
		u8	tempval;		
		
		tempval = read_nic_byte(dev, SYS_ISO_CTRL+1); 
		tempval &= 0xFE;
		write_nic_byte(dev, SYS_ISO_CTRL+1, tempval); 

		// Change Program timing
		write_nic_byte(dev, EFUSE_CTRL+3, 0x72); 
		//printk("!!!!!!!!!!!!!!!!!!!!!%s: write 0x33 with 0x72\n",__FUNCTION__);
		RT_TRACE(COMP_INIT, "EFUSE CONFIG OK\n");
	}

		
	RT_TRACE(COMP_INIT, "<---MacConfigAfterFwDownload()\n");
}

void rtl8192SU_HwConfigureRTL8192SUsb(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	u8			regBwOpMode = 0;
	u32			regRATR = 0, regRRSR = 0;
	u8			regTmp = 0;
	u32 			i = 0;

	//1 This part need to modified according to the rate set we filtered!!
	//
	// Set RRSR, RATR, and BW_OPMODE registers
	//
	switch(priv->ieee80211->mode)
	{
	case WIRELESS_MODE_B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK;
		regRRSR = RATE_ALL_CCK;
		break;
	case WIRELESS_MODE_A:
		regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_G:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_AUTO:
		if (priv->bInHctTest)
		{
		    regBwOpMode = BW_OPMODE_20MHZ;
		    regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		    regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		}
		else
		{
		    regBwOpMode = BW_OPMODE_20MHZ;
		    regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		    regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		}
		break;
	case WIRELESS_MODE_N_24G:
		// It support CCK rate by default.
		// CCK rate will be filtered out only when associated AP does not support it.
		regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_N_5G:
		regBwOpMode = BW_OPMODE_5G;
		regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	}
	
	//
	// <Roger_Notes> We disable CCK response rate until FIB CCK rate IC's back.
	// 2008.09.23.
	//
	regTmp = read_nic_byte(dev, INIRTSMCS_SEL);
#ifdef RTL8192SU_DISABLE_CCK_RATE
	regRRSR = ((regRRSR & 0x000ffff0)<<8) | regTmp;
#else
	regRRSR = ((regRRSR & 0x000fffff)<<8) | regTmp;
#endif

	//
	// Update SIFS timing.
	//
	//priv->SifsTime = 0x0e0e0a0a;
	//Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_SIFS,  (pu1Byte)&pHalData->SifsTime);
	{	u8 val[4] = {0x0e, 0x0e, 0x0a, 0x0a};
		// SIFS for CCK Data ACK
		write_nic_byte(dev, SIFS_CCK, val[0]);
		// SIFS for CCK consecutive tx like CTS data!
		write_nic_byte(dev, SIFS_CCK+1, val[1]);
		
		// SIFS for OFDM Data ACK
		write_nic_byte(dev, SIFS_OFDM, val[2]);
		// SIFS for OFDM consecutive tx like CTS data!
		write_nic_byte(dev, SIFS_OFDM+1, val[3]);
	}

	write_nic_dword(dev, INIRTSMCS_SEL, regRRSR);	
	write_nic_byte(dev, BW_OPMODE, regBwOpMode);
	
	//
	// Suggested by SD1 Alex, 2008-06-14.
	//
	//PlatformEFIOWrite1Byte(Adapter, TXOP_STALL_CTRL, 0x80);//NAV to protect all TXOP.	
	
	//
	// Set Data Auto Rate Fallback Retry Count register.
	//
	write_nic_dword(dev, DARFRC, 0x02010000);
	write_nic_dword(dev, DARFRC+4, 0x06050403);
	write_nic_dword(dev, RARFRC, 0x02010000);
	write_nic_dword(dev, RARFRC+4, 0x06050403);

	// Set Data Auto Rate Fallback Reg. Added by Roger, 2008.09.22.
	for (i = 0; i < 8; i++)
#ifdef RTL8192SU_DISABLE_CCK_RATE 
		write_nic_dword(dev, ARFR0+i*4, 0x1f0ff0f0);
#else
		write_nic_dword(dev, ARFR0+i*4, 0x1f0ffff0);
#endif
		
	//
	// Aggregation length limit. Revised by Roger. 2008.09.22.
	//
	write_nic_byte(dev, AGGLEN_LMT_H, 0x0f);	// Set AMPDU length to 12Kbytes for ShortGI case.
	write_nic_dword(dev, AGGLEN_LMT_L, 0xddd77442); // Long GI
	write_nic_dword(dev, AGGLEN_LMT_L+4, 0xfffdd772);

	// Set NAV protection length
	write_nic_word(dev, NAV_PROT_LEN, 0x0080);	

	// Set TXOP stall control for several queue/HI/BCN/MGT/
	write_nic_byte(dev, TXOP_STALL_CTRL, 0x00); // NAV Protect next packet.
	
	// Set MSDU lifetime.
	write_nic_byte(dev, MLT, 0x8f);
	
	// Set CCK/OFDM SIFS
	write_nic_word(dev, SIFS_CCK, 0x0a0a); // CCK SIFS shall always be 10us.
	write_nic_word(dev, SIFS_OFDM, 0x1010);
	
	write_nic_byte(dev, ACK_TIMEOUT, 0x40);
	
	// CF-END Threshold
	write_nic_byte(dev, CFEND_TH, 0xFF);	

	//
	// For Min Spacing configuration.
	//
	switch(priv->rf_type)
	{
		case RF_1T2R:
		case RF_1T1R:
			RT_TRACE(COMP_INIT, "Initializeadapter: RF_Type%s\n", (priv->rf_type==RF_1T1R? "(1T1R)":"(1T2R)"));
			priv->ieee80211->MinSpaceCfg = (MAX_MSS_DENSITY_1T<<3);	
			break;
		case RF_2T2R:
		case RF_2T2R_GREEN:
			RT_TRACE(COMP_INIT, "Initializeadapter:RF_Type(2T2R)\n");
			priv->ieee80211->MinSpaceCfg = (MAX_MSS_DENSITY_2T<<3);			
			break;
	}
	write_nic_byte(dev, AMPDU_MIN_SPACE, priv->ieee80211->MinSpaceCfg );
}

#endif

#ifdef RTL8192SU
//	Description:	Initial HW relted registers.
//
//	Assumption:	This function is only invoked at driver intialization once.
//
//	2008.06.10, Added by Roger.
bool rtl8192SU_adapter_start(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	rt_firmware		*pFirmware = priv->pFirmware;
	//u32 					dwRegRead = 0;
	//bool 					init_status = true;	
	//u32					ulRegRead;
	bool             				rtStatus = true;
	//u8			PipeIndex,eRFPath;
	u8				tmpU1b;
	u8 fw_download_times = 1;


	RT_TRACE(COMP_INIT, "--->InitializeAdapter8192SUsb()\n");
	
	//pHalData->bGPIOChangeRF = FALSE;


	//
	// <Roger_Notes> 2008.06.15.
	//
	// Initialization Steps on RTL8192SU:
	// a. MAC initialization prior to sending down firmware code.
	// b. Download firmware code step by step(i.e., IMEM, EMEM, DMEM).
	// c. MAC configuration after firmware has been download successfully.
	// d. Initialize BB related configurations.
	// e. Initialize RF related configurations.
	// f.  Start to BulkIn transfer. 
	// 	

	//
	//a. MAC initialization prior to send down firmware code.
	//
start:
	rtl8192SU_MacConfigBeforeFwDownloadASIC(dev);

	//
	//b. Download firmware code step by step(i.e., IMEM, EMEM, DMEM).
	//
	rtStatus = FirmwareDownload92S(dev);
	if(rtStatus != true)
	{
		if(fw_download_times == 1){
			RT_TRACE(COMP_INIT, "InitializeAdapter8192SUsb(): Download Firmware failed once, Download again!!\n");
			fw_download_times = fw_download_times + 1;
			goto start;
		}else{
			RT_TRACE(COMP_INIT, "InitializeAdapter8192SUsb(): Download Firmware failed twice, end!!\n");
		goto end;
	}
	}
	//
	//c. MAC configuration after firmware has been download successfully.
	//
	rtl8192SU_MacConfigAfterFwDownload(dev);	

#if (RTL8192S_DISABLE_FW_DM == 1)
	write_nic_dword(dev, WFM5, FW_DM_DISABLE); 
#endif

	// <Roger_Notes> Retrieve default FW Cmd IO map. 2009.05.08.
	priv->FwCmdIOMap = 	read_nic_word(dev, LBUS_MON_ADDR);
	priv->FwCmdIOParam = read_nic_dword(dev, LBUS_ADDR_MASK);

	//priv->bLbusEnable = TRUE;
	if(priv->RegRfOff == TRUE)
		priv->ieee80211->eRFPowerState = eRfOff;

	// Save target channel
	// <Roger_Notes> Current Channel will be updated again later.
	//priv->CurrentChannel = Channel;
	rtStatus = PHY_MACConfig8192S(dev);//===>ok

#ifdef TCP_CSUM_OFFLOAD_RX
	// Note: PHY_MACConfig8192S() configures RCR, too.
	printk("%s-%d: RCR = %x\n", __FUNCTION__, __LINE__, read_nic_dword(dev, RCR));
	priv->ReceiveConfig |= RCR_RX_TCPOFDL_EN;
	write_nic_dword(dev, RCR, (read_nic_dword(dev, RCR) | RCR_RX_TCPOFDL_EN));
	printk("%s-%d: RCR = %x\n", __FUNCTION__, __LINE__, read_nic_dword(dev, RCR));
#endif
	
	if(rtStatus != true)
	{
		RT_TRACE(COMP_INIT, "InitializeAdapter8192SUsb(): Fail to configure MAC!!\n");
		goto end;
	}
	if (1){
		int i;
		for (i=0; i<4; i++)
			write_nic_dword(dev,WDCAPARA_ADD[i], 0x5e4322);
		write_nic_byte(dev,AcmHwCtrl, 0x01);
	}


	//
	//d. Initialize BB related configurations.
	//

	rtStatus = PHY_BBConfig8192S(dev);//===>ok
	if(rtStatus != true)
	{
		RT_TRACE(COMP_INIT, "InitializeAdapter8192SUsb(): Fail to configure BB!!\n");
		goto end;
	}

	rtl8192_setBBreg(dev, rFPGA0_AnalogParameter2, 0xff, 0x58);//===>ok

	//
	// e. Initialize RF related configurations.
	//
	// 2007/11/02 MH Before initalizing RF. We can not use FW to do RF-R/W.
	priv->Rf_Mode = RF_OP_By_SW_3wire;	

	write_nic_byte(dev, AFE_XTAL_CTRL+1, 0xDB);

	// <Roger_Notes> The following IOs are configured for each RF modules.
	// Enable RF module and reset RF and SDM module. 2008.11.17.
	if(priv->card_8192_version == VERSION_8192S_ACUT)
		write_nic_byte(dev, SPS1_CTRL+3, (u8)(RF_EN|RF_RSTB|RF_SDMRSTB)); // Fix A-Cut bug.
	else
		write_nic_byte(dev, RF_CTRL, (u8)(RF_EN|RF_RSTB|RF_SDMRSTB));
	
	rtStatus = PHY_RFConfig8192S(dev);//===>ok
	if(rtStatus != true)
	{
		RT_TRACE(COMP_INIT, "InitializeAdapter8192SUsb(): Fail to configure RF!!\n");
		goto end;
	}	

	//
	// Joseph Note: Keep RfRegChnlVal for later use.
	//
	priv->RfRegChnlVal[0] = rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)0, RF_CHNLBW, bRFRegOffsetMask);
	priv->RfRegChnlVal[1] = rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)1, RF_CHNLBW, bRFRegOffsetMask);
	
	// Set CCK and OFDM Block "ON"
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bCCKEn, 0x1);
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, 0x1);

	//
	// Turn off Radio B while RF type is 1T1R by SD3 Wilsion's request. 
	// Revised by Roger, 2008.12.18.
	//
	if(priv->rf_type == RF_1T1R)
	{
		// This is needed for PHY_REG after 20081219
		rtl8192_setBBreg(dev, rFPGA0_RFMOD, 0xff000000, 0x03);
		// This is needed for PHY_REG before 20081219
		//PHY_SetBBReg(Adapter, rOFDM0_TRxPathEnable, bMaskByte0, 0x11);
	}

#if (RTL8192SU_DISABLE_IQK==0)
		// For 1T2R IQK only currently.
		if (priv->card_8192_version == VERSION_8192S_BCUT)
		{	
			PHY_IQCalibrateBcut(dev);
		}
		else if (priv->card_8192_version == VERSION_8192S_ACUT)
		{
			PHY_IQCalibrate(dev);	
		}
#endif


	//3//
	//3 //Set Hardware
	//3//
	rtl8192SU_HwConfigureRTL8192SUsb(dev);//==>ok

	//
	// <Roger_Notes> We set MAC address here if autoload was failed before, 
	// otherwise IDR0 will NOT contain any value.
	//
	write_nic_dword(dev, IDR0, ((u32*)dev->dev_addr)[0]);
	write_nic_word(dev, IDR4, ((u16*)(dev->dev_addr + 4))[0]);
	if(!priv->bInHctTest)
	{
		if(priv->ResetProgress == RESET_TYPE_NORESET)
		{
			//RT_TRACE(COMP_MLME, DBG_LOUD, ("Initializeadapter8192SUsb():RegWirelessMode(%#x) \n", Adapter->RegWirelessMode));
			//Adapter->HalFunc.SetWirelessModeHandler(Adapter, Adapter->RegWirelessMode);
			rtl8192_SetWirelessMode(dev, priv->ieee80211->mode);//===>ok
	        }
	}
	else
	{
		priv->ieee80211->mode = WIRELESS_MODE_G;
	 	rtl8192_SetWirelessMode(dev, WIRELESS_MODE_G);
	}

	//Security related.
	//-----------------------------------------------------------------------------
	// Set up security related. 070106, by rcnjko:
	// 1. Clear all H/W keys.
	// 2. Enable H/W encryption/decryption.
	//-----------------------------------------------------------------------------
	//CamResetAllEntry(Adapter);
	//Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);

	//SecClearAllKeys(Adapter);	
	CamResetAllEntry(dev);
	//SecInit(Adapter);  
	{
		u8 SECR_value = 0x0;
		SECR_value |= SCR_TxEncEnable;
		SECR_value |= SCR_RxDecEnable;
		SECR_value |= SCR_NoSKMC;
		write_nic_byte(dev, SECR, SECR_value);
	}
		 
#if 0

	if(pHalData->VersionID == VERSION_8192SU_A)
	{
		// cosa add for tx power level initialization.
		GetTxPowerOriginalOffset(Adapter);
		SetTxPowerLevel819xUsb(Adapter, Channel);
	}
#endif
	

#if 1

	//PHY_UpdateInitialGain(dev);
	
	if(priv->RegRfOff == true)
	{ // User disable RF via registry.
		u8 eRFPath = 0;
		
		RT_TRACE((COMP_INIT|COMP_RF), "InitializeAdapter8192SUsb(): Turn off RF for RegRfOff ----------\n");
		MgntActSet_RF_State(dev, eRfOff, RF_CHANGE_BY_SW);
		// Those action will be discard in MgntActSet_RF_State because off the same state
		for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
	}
	else if(priv->ieee80211->RfOffReason > RF_CHANGE_BY_PS)
	{ // H/W or S/W RF OFF before sleep.
		RT_TRACE((COMP_INIT|COMP_RF), "InitializeAdapter8192SUsb(): Turn off RF for RfOffReason(%d) ----------\n", priv->ieee80211->RfOffReason);
		MgntActSet_RF_State(dev, eRfOff, priv->ieee80211->RfOffReason);
	}
	else
	{
		priv->ieee80211->eRFPowerState = eRfOn;
		priv->ieee80211->RfOffReason = 0; 
#ifdef TO_DO_LIST//FIXLZM
		if(priv->bInSetPower)
			PlatformUsbEnableInPipes(Adapter);
#endif
		RT_TRACE((COMP_INIT|COMP_RF), "InitializeAdapter8192SUsb(): RF is on ----------\n");
	}
	
#endif


//
// f. Start to BulkIn transfer.
//
#ifdef TO_DO_LIST	

#ifndef UNDER_VISTA
	{
		u8	i;
		PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);

		for(PipeIndex=0; PipeIndex < MAX_RX_QUEUE; PipeIndex++)
		{
			if (PipeIndex == 0)
			{
				for(i=0; i<32; i++)			
				HalUsbInMpdu(Adapter, PipeIndex);
			}
			else
			{
				//HalUsbInMpdu(Adapter, PipeIndex);
				//HalUsbInMpdu(Adapter, PipeIndex);
				//HalUsbInMpdu(Adapter, PipeIndex);
			}
		}	
		PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
	}
#else
		// Joseph add to 819X code base for Vista USB platform.
		// This part may need to be add to Hal819xU code base. too.
	        PlatformUsbEnableInPipes(Adapter);
#endif

	RT_TRACE(COMP_INIT, "HighestOperaRate = %x\n", Adapter->MgntInfo.HighestOperaRate);

	PlatformStartWorkItem( &(pHalData->RtUsbCheckForHangWorkItem) );
	
	//
	// <Roger_EXP> The following  configurations are for ASIC verification temporally.
	// 2008.07.10.
	//

#endif

	rtl8192_rx_enable(dev);

	//
	// Read EEPROM TX power index and PHY_REG_PG.txt to capture correct
	// TX power index for different rate set.
	//
	if(priv->card_8192_version >= VERSION_8192S_ACUT)
	{
		// Get original hw reg values
		PHY_GetHWRegOriginalValue(dev);

		// Write correct tx power index//FIXLZM
		PHY_SetTxPowerLevel8192S(dev, priv->chan);
	}

	// EEPROM R/W workaround
	tmpU1b = read_nic_byte(dev, MAC_PINMUX_CFG);
	write_nic_byte(dev, MAC_PINMUX_CFG, tmpU1b&(~GPIOMUX_EN));
	
//
	// <Roger_Notes> The following FW CMDs are for DM initialization.
	//
		write_nic_dword(dev, WFM5, FW_IQK_ENABLE);
		ChkFwCmdIoDone(dev);


	//
	// <Roger_Notes> We enable high power mechanism after NIC initialized. 
	// 2008.11.27.
	//
	if(pFirmware->FirmwareVersion >= 0x35)
	{// Fw v.53 and later.
		priv->ieee80211->SetFwCmdHandler(dev,FW_CMD_RA_INIT);
	}
	else if(pFirmware->FirmwareVersion >= 0x34)
	{// Fw v.52 and later.
		write_nic_dword(dev, WFM5, FW_RA_INIT); 
		ChkFwCmdIoDone(dev);
	}
	else
	{// Compatible earlier FW version.
		write_nic_dword(dev, WFM5, FW_RA_RESET); 
		ChkFwCmdIoDone(dev);
		write_nic_dword(dev, WFM5, FW_RA_ACTIVE); 
		ChkFwCmdIoDone(dev);
		write_nic_dword(dev, WFM5, FW_RA_REFRESH); 
		ChkFwCmdIoDone(dev);
	}

	// Disable RF-B when 1T condition.
	if(priv->CustomerID == RT_CID_819x_CAMEO1)
	{
		write_nic_dword(dev, WFM5, FW_TXANT_SWITCH_DISABLE); 
		ChkFwCmdIoDone(dev);	
	}

// <Roger_Notes> We return status here for temporal FPGA verification. 2008.05.12.
//
#if RTL8192SU_FPGA_UNSPECIFIED_NETWORK
	rtStatus = true;
        goto end;
#endif

// The following IO was for FPGA verification purpose. Added by Roger, 2008.09.11.
#if 0
	// 2008/08/19 MH From SD1 Jong, we must write some register for true PHY/MAC FPGA.
	write_nic_byte(dev, rOFDM0_XAAGCCore1, 0x30);
	write_nic_byte(dev, rOFDM0_XBAGCCore1, 0x30);
	write_nic_byte(dev, rOFDM0_RxDetector1, 0x42);
	write_nic_dword(Adapter, rTxAGC_Mcs15_Mcs12, 0x06060606);
#endif

end:
return rtStatus;
}	

#endif

#ifdef RTL8192U
//added by Thomas 090711
void SetHwReg819xUsb(struct net_device *dev,u8 variable,u8* val)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	switch(variable)
	{
#if defined(USB_RX_AGGREGATION_SUPPORT) || defined(USB_AGGR_BULKIN)
		case HW_VAR_USB_RX_AGGR:
		{
			u8 Setting;
			u32 ulValue = 0;
			u8 usValue = priv->CurReg0xfe38;

			Setting = *((u8*)val);
			printk("SetHwReg819xUsb(): HW_VAR_USB_RX_AGGR, Setting = %d\n",Setting);
			if(priv->bForcedUsbRxAggr)
				return;

			if(Setting > 2)
				return;

			if(Setting==0)
			{
				ulValue = 0;
				usValue |= 0x20;
			}
			else if(Setting == 1)
			{
				PRT_HIGH_THROUGHPUT	pHTInfo = priv->ieee80211->pHTInfo;
#ifdef USB_RX_AGGREGATION_SUPPORT
				ulValue = (pHTInfo->UsbRxFwAggrEn<<24) | (pHTInfo->UsbRxFwAggrPageNum<<16) | 
						(pHTInfo->UsbRxFwAggrPacketNum<<8) | (pHTInfo->UsbRxFwAggrTimeout);
#endif
				usValue |= 0x20;
			}
			else if(Setting == 2)
			{
				ulValue = 0x029c2802;
				usValue &= 0xdf;
			}

			if(ulValue != priv->CurUsbAggrSetting)
			{
				priv->CurUsbAggrSetting = ulValue;
				write_nic_dword(dev, USB_RX_AGGR, ulValue);
			}

			if(usValue != priv->CurReg0xfe38)
			{
				priv->CurReg0xfe38 = usValue;
				write_nic_byte_E(dev, 0x38, usValue);
			}
		}		
		break;
#endif
		default:
			break;
	}
}


//InitializeAdapter and PhyCfg
bool rtl8192_adapter_start(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32 dwRegRead = 0;
	bool init_status = true;
	bool download_again = true;
	u8	tmpvalue;

	RT_TRACE(COMP_INIT, "====>%s()\n", __FUNCTION__);
	priv->Rf_Mode = RF_OP_By_SW_3wire;

	//for ASIC power on sequence
	write_nic_byte_E(dev, 0x5f, 0x80);
	mdelay(50);
	write_nic_byte_E(dev, 0x5f, 0xf0);
	write_nic_byte_E(dev, 0x5d, 0x00);
	write_nic_byte_E(dev, 0x5e, 0x80);
	write_nic_byte(dev, 0x17, 0x37);
	mdelay(10);

AGAIN:
	// For any kind of InitializeAdapter process, we shall use system now!!
	priv->pFirmware->firmware_status = FW_STATUS_0_INIT;

	//config CPUReset Register
	//Firmware Reset or not?
	dwRegRead = read_nic_dword(dev, CPU_GEN);
	if (priv->pFirmware->firmware_status == FW_STATUS_0_INIT)
	{//called from MPInitialized. do nothing
		dwRegRead |= CPU_GEN_SYSTEM_RESET;
	}
	else if (priv->pFirmware->firmware_status == FW_STATUS_5_READY)
	{
		dwRegRead |= CPU_GEN_FIRMWARE_RESET;	// Called from MPReset
	}
	else
		RT_TRACE(COMP_ERR, "ERROR in %s(): undefined firmware state(%d)\n", __FUNCTION__,   priv->pFirmware->firmware_status);

	write_nic_dword(dev, CPU_GEN, dwRegRead);

	//config BB.
	rtl8192_BBConfig(dev);

#if 1
	//Loopback mode or not
	priv->LoopbackMode = RTL819xU_NO_LOOPBACK;
	//2006.12.13 by emily. Note!We should not merge these two CPU_GEN register writings 
	//	because setting of System_Reset bit reset MAC to default transmission mode.
	dwRegRead = read_nic_dword(dev, CPU_GEN);
	if (priv->LoopbackMode == RTL819xU_NO_LOOPBACK)
		dwRegRead = ((dwRegRead & CPU_GEN_NO_LOOPBACK_MSK) | CPU_GEN_NO_LOOPBACK_SET);
	else if (priv->LoopbackMode == RTL819xU_MAC_LOOPBACK)
		dwRegRead |= CPU_CCK_LOOPBACK;
	else
		RT_TRACE(COMP_ERR, "Serious error in %s(): wrong loopback mode setting(%d)\n", __FUNCTION__,  priv->LoopbackMode);
	
	write_nic_dword(dev, CPU_GEN, dwRegRead);

	// 2006.11.29. After reset cpu, we sholud wait for a second, otherwise, it may fail to write registers. Emily
	udelay(500);

	//xiong add for new bitfile:usb suspend reset pin set to 1. //do we need?
	write_nic_byte_E(dev, 0x5f, (read_nic_byte_E(dev, 0x5f)|0x20));

	//Set Hardware
	rtl8192_hwconfig(dev);

	//turn on Tx/Rx
	write_nic_byte(dev, CMDR, CR_RE|CR_TE);
	
	// ACM related setting.
	write_nic_byte(dev, AcmHwCtrl, priv->AcmControl);

	//set IDR0 here
	write_nic_dword(dev, MAC0, ((u32*)dev->dev_addr)[0]);
	write_nic_word(dev, MAC4, ((u16*)(dev->dev_addr + 4))[0]);

	//set RCR
	write_nic_dword(dev, RCR, priv->ReceiveConfig);

	//Initialize Number of Reserved Pages in Firmware Queue
	write_nic_dword(dev, RQPN1,  NUM_OF_PAGE_IN_FW_QUEUE_BK << RSVD_FW_QUEUE_PAGE_BK_SHIFT |\
						NUM_OF_PAGE_IN_FW_QUEUE_BE << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
						NUM_OF_PAGE_IN_FW_QUEUE_VI << RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
						NUM_OF_PAGE_IN_FW_QUEUE_VO <<RSVD_FW_QUEUE_PAGE_VO_SHIFT);												
	write_nic_dword(dev, RQPN2, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT |\
						NUM_OF_PAGE_IN_FW_QUEUE_CMD << RSVD_FW_QUEUE_PAGE_CMD_SHIFT);
	write_nic_dword(dev, RQPN3, APPLIED_RESERVED_QUEUE_IN_FW| \
						NUM_OF_PAGE_IN_FW_QUEUE_BCN<<RSVD_FW_QUEUE_PAGE_BCN_SHIFT
//						| NUM_OF_PAGE_IN_FW_QUEUE_PUB<<RSVD_FW_QUEUE_PAGE_PUB_SHIFT
						);
#if 1 // Limit RTS/CTS rates to 24M
        write_nic_dword(dev, RATR0+4*7, (RATR_6M|RATR_9M|RATR_12M|RATR_18M|RATR_24M) | (RATE_ALL_CCK));
#else
        write_nic_dword(dev, RATR0+4*7, (RATE_ALL_OFDM_AG | RATE_ALL_CCK));
#endif

	//Set AckTimeout
	// TODO: (it value is only for FPGA version). need to be changed!!2006.12.18, by Emily
	write_nic_byte(dev, ACK_TIMEOUT, 0x30);

//	RT_TRACE(COMP_INIT, "%s():priv->ResetProgress is %d\n", __FUNCTION__,priv->ResetProgress);
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	rtl8192_SetWirelessMode(dev, priv->ieee80211->mode);
	if(priv->ResetProgress == RESET_TYPE_NORESET){
	CamResetAllEntry(dev);
	{
		u8 SECR_value = 0x0;
		SECR_value |= SCR_TxEncEnable;
		SECR_value |= SCR_RxDecEnable;
		SECR_value |= SCR_NoSKMC;
		write_nic_byte(dev, SECR, SECR_value);
	}
	}
	
	//Beacon related
	write_nic_word(dev, ATIMWND, 2);
	write_nic_word(dev, BCN_INTERVAL, 100);

	{
#define DEFAULT_EDCA 0x005e4332
		int i;
		for (i=0; i<QOS_QUEUE_NUM; i++)
		write_nic_dword(dev, WDCAPARA_ADD[i], DEFAULT_EDCA);
	}
#if defined(USB_RX_AGGREGATION_SUPPORT) || defined(USB_AGGR_BULKIN)
	//3 For usb rx firmware aggregation control
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
		HalInitUsbRxAggr819xUsb(dev);
		HalUsbRxAggr819xUsb(dev);
	}
	else
	{
		// Restore original setting
		write_nic_dword(dev, USB_RX_AGGR, priv->CurUsbAggrSetting);
		write_nic_byte_E(dev, 0x38, priv->CurReg0xfe38);
	}
#endif

	// Fix "CrazyRacing" Online game issue.
	write_nic_byte_E(dev, 0x1a, (read_nic_byte_E(dev, 0x1a)&0xef));

	rtl8192_phy_configmac(dev);
	
	tmpvalue = read_nic_byte(dev, IC_VERRSION);
	priv->IC_Cut = tmpvalue;
	printk("IC_Cut = %x\n",priv->IC_Cut);

	rtl8192_phy_getTxPower(dev);
	rtl8192_phy_setTxPower(dev, priv->chan);

	udelay(1000);

	priv->usb_error = false;
	//Firmware download 
	init_status = init_firmware(dev);
	if(!init_status)
	{
		RT_TRACE(COMP_ERR,"ERR!!! %s(): Firmware download is failed\n", __FUNCTION__);
		if (download_again)
		{
			download_again = false;
			goto AGAIN;
		} 
		return init_status;
	}
	RT_TRACE(COMP_INIT, "%s():after firmware download\n", __FUNCTION__);

	//config RF.
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
		// If driver is in the status of baseband config failure , driver skips RF initialization and RF is 
		// in turned off state. Driver should check whether Rx stuck and do silent reset. And
		// if driver is in baseband config failure status, driver should initialize RF in the following 
		// silent reset procedure Vivi, 2008.04.29
		rtl8192_phy_RFConfig(dev);
		RT_TRACE(COMP_INIT, "%s():after phy RF config\n", __FUNCTION__);
	}

	if(priv->ieee80211->FwRWRF && 
		priv->EEPROMTxPowerTrackEnable && 
		priv->IC_Cut < IC_VersionCut_D &&
			(priv->EEPROMThermalMeter>= VALID_TSSI_MIN &&
			priv->EEPROMThermalMeter <= VALID_TSSI_MAX) )
		// We can force firmware to do RF-R/W
		priv->Rf_Mode = RF_OP_By_FW;
	else
		priv->Rf_Mode = RF_OP_By_SW_3wire;

	rtl8192_phy_updateInitGain(dev);

	//--set CCK and OFDM Block "ON"--*/
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bCCKEn, 0x1);
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, 0x1);

#ifdef TO_DO_LIST
if(Adapter->ResetProgress == RESET_TYPE_NORESET)
	{
		if(pMgntInfo->RegRfOff == TRUE)
		{ // User disable RF via registry.
			RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter819xUsb(): Turn off RF for RegRfOff ----------\n"));
			MgntActSet_RF_State(Adapter, eRfOff, RF_CHANGE_BY_SW);
			// Those action will be discard in MgntActSet_RF_State because off the same state
			for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
		}
		else if(pMgntInfo->RfOffReason > RF_CHANGE_BY_PS)
		{ // H/W or S/W RF OFF before sleep.
			RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter819xUsb(): Turn off RF for RfOffReason(%d) ----------\n", pMgntInfo->RfOffReason));
			MgntActSet_RF_State(Adapter, eRfOff, pMgntInfo->RfOffReason);
		}
		else
		{
			pHalData->eRFPowerState = eRfOn;
			pMgntInfo->RfOffReason = 0; 
			RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter819xUsb(): RF is on ----------\n"));
		}
	}
	else
	{
		if(pHalData->eRFPowerState == eRfOff)
		{
			MgntActSet_RF_State(Adapter, eRfOff, pMgntInfo->RfOffReason);
			// Those action will be discard in MgntActSet_RF_State because off the same state
			for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
		}	
	}
#endif	
	
	rtl8192_rx_enable(dev);

	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{

#if 1	// C version TXPowerTracking mechanism is more stable than D!
		//priv->bDcut = TRUE; // --> dm_InitializeTXPowerTracking_TSSI()
		priv->bDcut = FALSE; // --> dm_InitializeTXPowerTracking_ThermalMeter()
		RT_TRACE(COMP_POWER_TRACKING, "C-cut\n");
#else 
		//if D or C cut
		u8 tmpvalue = read_nic_byte(dev, 0x301);
		if(tmpvalue ==0x03)
		{
			priv->bDcut = TRUE;
			RT_TRACE(COMP_POWER_TRACKING, "D-cut\n");
		}
		else
		{	
			priv->bDcut = FALSE;
			RT_TRACE(COMP_POWER_TRACKING, "C-cut\n");
		}
#endif		
		dm_initialize_txpower_tracking(dev);

		//if(priv->bDcut == TRUE)
		if(priv->IC_Cut >= IC_VersionCut_D)
		{
			u32 i, TempCCk;
			u32 tmpRegA= rtl8192_QueryBBReg(dev,rOFDM0_XATxIQImbalance,bMaskDWord);
			//u32 tmpRegC = rtl8192_QueryBBReg(dev,rOFDM0_XCTxIQImbalance,bMaskDWord);
			for(i = 0; i<TxBBGainTableLength; i++)
			{
				if(tmpRegA == priv->txbbgain_table[i].txbbgain_value)
				{
					priv->rfa_txpowertrackingindex= (u8)i;
					priv->rfa_txpowertrackingindex_real= (u8)i;
					priv->rfa_txpowertracking_default= priv->rfa_txpowertrackingindex;
					break;
				}
			}

			TempCCk = rtl8192_QueryBBReg(dev, rCCK0_TxFilter1, bMaskByte2);				

			for(i=0 ; i<CCKTxBBGainTableLength ; i++)
			{
				
				if(TempCCk == priv->cck_txbbgain_table[i].ccktxbb_valuearray[0])
				{
					priv->cck_present_attentuation_20Mdefault=(u8) i;
					break;
				}
			}
			priv->cck_present_attentuation_40Mdefault= 0;
			priv->cck_present_attentuation_difference= 0;
			priv->cck_present_attentuation = priv->cck_present_attentuation_20Mdefault;
		
	//		pMgntInfo->bTXPowerTracking = FALSE;//TEMPLY DISABLE
		}
	}
	write_nic_byte(dev, 0x87, 0x0);
			
	
#endif	
	return init_status;
}

#endif
// this configures registers for beacon tx and enables it via
// rtl8192_beacon_tx_enable(). rtl8192_beacon_tx_disable() might
// be used to stop beacon transmission
//
#if 0
void rtl8192_start_tx_beacon(struct net_device *dev)
{
	int i;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u16 word;	
	DMESG("Enabling beacon TX");
	//write_nic_byte(dev, TX_CONF,0xe6);// TX_CONF
	//rtl8192_init_beacon(dev);
	//set_nic_txring(dev);
//	rtl8192_prepare_beacon(dev);
	rtl8192_irq_disable(dev);
//	rtl8192_beacon_tx_enable(dev);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	//write_nic_byte(dev,0x9d,0x20); //DMA Poll
	//write_nic_word(dev,0x7a,0);
	//write_nic_word(dev,0x7a,0x8000);

	
	word  = read_nic_word(dev, BcnItv);
	word &= ~BcnItv_BcnItv; // clear Bcn_Itv
	write_nic_word(dev, BcnItv, word);

	write_nic_word(dev, AtimWnd, 
		       read_nic_word(dev, AtimWnd) &~ AtimWnd_AtimWnd);
	
	word  = read_nic_word(dev, BCN_INTR_ITV);
	word &= ~BCN_INTR_ITV_MASK;
	
	//word |= priv->ieee80211->beacon_interval * 
	//	((priv->txbeaconcount > 1)?(priv->txbeaconcount-1):1);
	// FIXME:FIXME check if correct ^^ worked with 0x3e8;
	
	write_nic_word(dev, BCN_INTR_ITV, word);
	
	//write_nic_word(dev,0x2e,0xe002);
	//write_nic_dword(dev,0x30,0xb8c7832e);
	for(i=0; i<ETH_ALEN; i++)
		write_nic_byte(dev, BSSID+i, priv->ieee80211->beacon_cell_ssid[i]);
	
//	rtl8192_update_msr(dev);

	
	//write_nic_byte(dev,CONFIG4,3); // !!!!!!!!!! */
	
	rtl8192_set_mode(dev, EPROM_CMD_NORMAL);
	
	rtl8192_irq_enable(dev);
	
	// VV !!!!!!!!!! VV*/
	
	//rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	//write_nic_byte(dev,0x9d,0x00); 	
	//rtl8192_set_mode(dev,EPROM_CMD_NORMAL);

}
#endif
/***************************************************************************
    -------------------------------NET STUFF---------------------------
***************************************************************************/

static struct net_device_stats *rtl8192_stats(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	return &priv->ieee80211->stats;
}

bool
HalTxCheckStuck819xUsb(
	struct net_device *dev
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u16 		RegTxCounter = read_nic_word(dev, 0x128);
	bool		bStuck = FALSE;
	RT_TRACE(COMP_RESET,"%s():RegTxCounter is %d,TxCounter is %d\n",__FUNCTION__,RegTxCounter,priv->TxCounter);
	if(priv->TxCounter==RegTxCounter)
		bStuck = TRUE;

	priv->TxCounter = RegTxCounter;

	return bStuck;
}

//
//<Assumption: RT_TX_SPINLOCK is acquired.>
//First added: 2006.11.19 by emily
//
RESET_TYPE
TxCheckStuck(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8			QueueID;
//	PRT_TCB			pTcb;
//	u8			ResetThreshold;
	bool			bCheckFwTxCnt = false;
	//unsigned long flags;
	
	//
	// Decide Stuch threshold according to current power save mode
	//

//     RT_TRACE(COMP_RESET, " ==> TxCheckStuck()\n");
//	     PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
//	     spin_lock_irqsave(&priv->ieee80211->lock,flags);
	     for (QueueID = 0; QueueID<=BEACON_QUEUE;QueueID ++)
	     {
	     		if(QueueID == TXCMD_QUEUE)
		         continue;
#if 1
#ifdef USB_TX_DRIVER_AGGREGATION_ENABLE
			if((skb_queue_len(&priv->ieee80211->skb_waitQ[QueueID]) == 0) && (skb_queue_len(&priv->ieee80211->skb_aggQ[QueueID]) == 0) && (skb_queue_len(&priv->ieee80211->skb_drv_aggQ[QueueID]) == 0))
#else
		     	if((skb_queue_len(&priv->ieee80211->skb_waitQ[QueueID]) == 0)  && (skb_queue_len(&priv->ieee80211->skb_aggQ[QueueID]) == 0))
#endif			 	
			 	continue;
#endif

	             bCheckFwTxCnt = true;
	     }
//	     PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
//	spin_unlock_irqrestore(&priv->ieee80211->lock,flags);
//	RT_TRACE(COMP_RESET,"bCheckFwTxCnt is %d\n",bCheckFwTxCnt); 
#if 1 
	if(bCheckFwTxCnt)
	{
		if(HalTxCheckStuck819xUsb(dev))
		{
			RT_TRACE(COMP_RESET, "TxCheckStuck(): Fw indicates no Tx condition! \n");
			return RESET_TYPE_SILENT;
		}
	}
#endif
	return RESET_TYPE_NORESET;
}

bool
HalRxCheckStuck819xUsb(struct net_device *dev)
{
	u16 	RegRxCounter = read_nic_word(dev, 0x130);
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool bStuck = FALSE;
//#ifdef RTL8192SU
	
//#else
	static u8	rx_chk_cnt = 0;
	RT_TRACE(COMP_RESET,"%s(): RegRxCounter is %d,RxCounter is %d\n",__FUNCTION__,RegRxCounter,priv->RxCounter);
	// If rssi is small, we should check rx for long time because of bad rx.
	// or maybe it will continuous silent reset every 2 seconds.
	rx_chk_cnt++;
	if(priv->undecorated_smoothed_pwdb >= (RateAdaptiveTH_High+5))
	{
		rx_chk_cnt = 0;	//high rssi, check rx stuck right now.
	}
	else if(priv->undecorated_smoothed_pwdb < (RateAdaptiveTH_High+5) &&
		((priv->CurrentChannelBW!=HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb>=RateAdaptiveTH_Low_40M) ||
		(priv->CurrentChannelBW==HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb>=RateAdaptiveTH_Low_20M)) )
	{
		if(rx_chk_cnt < 2)
		{
			return bStuck;
		}
		else
		{
			rx_chk_cnt = 0;
		}
	}
	else if(((priv->CurrentChannelBW!=HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb<RateAdaptiveTH_Low_40M) ||
		(priv->CurrentChannelBW==HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb<RateAdaptiveTH_Low_20M)) &&
		priv->undecorated_smoothed_pwdb >= VeryLowRSSI)
	{
		if(rx_chk_cnt < 4)
		{
			//printk("RSSI < %d && RSSI >= %d, no check this time \n", RateAdaptiveTH_Low, VeryLowRSSI);
			return bStuck;
		}
		else
		{
			rx_chk_cnt = 0;
			//DbgPrint("RSSI < %d && RSSI >= %d, check this time \n", RateAdaptiveTH_Low, VeryLowRSSI);
		}
	}
	else
	{
		if(rx_chk_cnt < 8)
		{
			//printk("RSSI <= %d, no check this time \n", VeryLowRSSI);
			return bStuck;
		}
		else
		{
			rx_chk_cnt = 0;
			//DbgPrint("RSSI <= %d, check this time \n", VeryLowRSSI);
		}
	}
//#endif

	if(priv->RxCounter==RegRxCounter)
		bStuck = TRUE;

	priv->RxCounter = RegRxCounter;

	return bStuck;
}

RESET_TYPE
RxCheckStuck(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev); 
	//int                     i;
	bool        bRxCheck = FALSE;

//       RT_TRACE(COMP_RESET," ==> RxCheckStuck()\n");
	//PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	
	 if(priv->IrpPendingCount > 1)
	 	bRxCheck = TRUE;
       //PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

//       RT_TRACE(COMP_RESET,"bRxCheck is %d \n",bRxCheck);
	if(bRxCheck)
	{
		if(HalRxCheckStuck819xUsb(dev))
		{
			RT_TRACE(COMP_RESET, "RxStuck Condition\n");
			return RESET_TYPE_SILENT;
		}
	}
	return RESET_TYPE_NORESET;
}


//
//	This function is called by Checkforhang to check whether we should ask OS to reset driver
//	
//	\param pAdapter	The adapter context for this miniport
//
//	Note:NIC with USB interface sholud not call this function because we cannot scan descriptor
//	to judge whether there is tx stuck.
//	Note: This function may be required to be rewrite for Vista OS.
//	<<<Assumption: Tx spinlock has been acquired >>>
//	
//	8185 and 8185b does not implement this function. This is added by Emily at 2006.11.24
//
RESET_TYPE
rtl819x_ifcheck_resetornot(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	RESET_TYPE	TxResetType = RESET_TYPE_NORESET;
	RESET_TYPE	RxResetType = RESET_TYPE_NORESET;
	RT_RF_POWER_STATE 	rfState;
	
#ifdef RTL8192SU	
#if (RTL8192SU_FPGA_2MAC_VERIFICATION ||RTL8192SU_ASIC_VERIFICATION)
	return RESET_TYPE_NORESET;
#endif  
#endif

	rfState = priv->ieee80211->eRFPowerState;
	
	TxResetType = TxCheckStuck(dev);
#if 1 
	if( (rfState != eRfOff) && 
		//ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FW_DOWNLOAD_FAILURE)) &&*/
		(priv->ieee80211->iw_mode != IW_MODE_ADHOC))
	{
		// If driver is in the status of firmware download failure , driver skips RF initialization and RF is 
		// in turned off state. Driver should check whether Rx stuck and do silent reset. And
		// if driver is in firmware download failure status, driver should initialize RF in the following 
		// silent reset procedure Emily, 2008.01.21

		// Driver should not check RX stuck in IBSS mode because it is required to
		// set Check BSSID in order to send beacon, however, if check BSSID is 
		// set, STA cannot hear any packet a all. Emily, 2008.04.12
		RxResetType = RxCheckStuck(dev);
	}
#endif
	if(TxResetType==RESET_TYPE_NORMAL || RxResetType==RESET_TYPE_NORMAL)
		return RESET_TYPE_NORMAL;
	else if(TxResetType==RESET_TYPE_SILENT || RxResetType==RESET_TYPE_SILENT){
		RT_TRACE(COMP_RESET,"%s():silent reset\n",__FUNCTION__);
		return RESET_TYPE_SILENT;
	}
	else
		return RESET_TYPE_NORESET;

}

void rtl8192_cancel_deferred_work(struct r8192_priv* priv);
int _rtl8192_up(struct net_device *dev);
int rtl8192_close(struct net_device *dev);



void 
CamRestoreAllEntry(	struct net_device *dev)
{
	u8 EntryId = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8*	MacAddr = priv->ieee80211->current_network.bssid;
	
	static u8	CAM_CONST_ADDR[4][6] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x03}};
	static u8	CAM_CONST_BROAD[] = 
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	RT_TRACE(COMP_SEC, "CamRestoreAllEntry: \n");

		
	if ((priv->ieee80211->pairwise_key_type == KEY_TYPE_WEP40)||
	    (priv->ieee80211->pairwise_key_type == KEY_TYPE_WEP104))
	{
		
		for(EntryId=0; EntryId<4; EntryId++)
		{
			{
				MacAddr = CAM_CONST_ADDR[EntryId];
				setKey(dev, 
						EntryId ,
						EntryId,
						priv->ieee80211->pairwise_key_type,
						MacAddr, 
						0,
						NULL);			
			}	
		}	
		
	}
	else if(priv->ieee80211->pairwise_key_type == KEY_TYPE_TKIP)
	{
		
		{
			if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type, 
						(u8*)dev->dev_addr, 
						0,
						NULL);		
			else
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type, 
						MacAddr, 
						0,
						NULL);			
		}
	}
	else if(priv->ieee80211->pairwise_key_type == KEY_TYPE_CCMP)
	{

		{
			if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type,
						(u8*)dev->dev_addr, 
						0,
						NULL);		
			else
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type,
						MacAddr, 
						0,
						NULL);				
		}
	}



	if(priv->ieee80211->group_key_type == KEY_TYPE_TKIP)
	{
		MacAddr = CAM_CONST_BROAD;
		for(EntryId=1 ; EntryId<4 ; EntryId++)
		{
			{
				setKey(dev, 
						EntryId, 
						EntryId,
						priv->ieee80211->group_key_type,
						MacAddr, 
						0,
						NULL);			
			}	
		}
		if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						0,
						0,
						priv->ieee80211->group_key_type,
						CAM_CONST_ADDR[0], 
						0,
						NULL);		
	}
	else if(priv->ieee80211->group_key_type == KEY_TYPE_CCMP)
	{
		MacAddr = CAM_CONST_BROAD;
		for(EntryId=1; EntryId<4 ; EntryId++)
		{
			{	
				setKey(dev, 
						EntryId , 
						EntryId,
						priv->ieee80211->group_key_type,
						MacAddr, 
						0,
						NULL);			
			}	
		}

		if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						0 , 
						0,
						priv->ieee80211->group_key_type,
						CAM_CONST_ADDR[0], 
						0,
						NULL);
	}
}
//////////////////////////////////////////////////////////////
// This function is used to fix Tx/Rx stop bug temporarily.
// This function will do "system reset" to NIC when Tx or Rx is stuck.
// The method checking Tx/Rx stuck of this function is supported by FW,
// which reports Tx and Rx counter to register 0x128 and 0x130.
//////////////////////////////////////////////////////////////
void
rtl819x_ifsilentreset(struct net_device *dev)
{
	//OCTET_STRING asocpdu;
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8	reset_times = 0;
	int reset_status = 0;
	struct ieee80211_device *ieee = priv->ieee80211;
	

	// 2007.07.20. If we need to check CCK stop, please uncomment this line.
	//bStuck = Adapter->HalFunc.CheckHWStopHandler(Adapter);
		
	if(priv->ResetProgress==RESET_TYPE_NORESET)
	{
RESET_START:		

		RT_TRACE(COMP_RESET,"=========>Reset progress!! \n");
		
		// Set the variable for reset.
		priv->bResetInProgress = true;
		priv->ResetProgress = RESET_TYPE_SILENT;
//		rtl8192_close(dev);
#if 1	
		down(&priv->wx_sem);	
		if(priv->up == 0)
		{
			RT_TRACE(COMP_ERR,"%s():the driver is not up! return\n",__FUNCTION__);
			up(&priv->wx_sem);
			return ;
		}
		priv->up = 0;
		RT_TRACE(COMP_RESET,"%s():======>start to down the driver\n",__FUNCTION__);
//		if(!netif_queue_stopped(dev))
//			netif_stop_queue(dev);
		
		rtl8192_rtx_disable(dev);
		rtl8192_cancel_deferred_work(priv);
		deinit_hal_dm(dev);
		del_timer_sync(&priv->watch_dog_timer);	
	
		ieee->sync_scan_hurryup = 1;
		if(ieee->state == IEEE80211_LINKED)
		{
			down(&ieee->wx_sem);
			printk("ieee->state is IEEE80211_LINKED\n");
			ieee80211_stop_send_beacons(priv->ieee80211);
			del_timer_sync(&ieee->associate_timer);
			#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
			cancel_delayed_work(&ieee->associate_retry_wq);	
			#endif	
			ieee80211_stop_scan(ieee);
			netif_carrier_off(dev);
			up(&ieee->wx_sem);
		}
		else{
			printk("ieee->state is NOT LINKED\n");
			ieee80211_softmac_stop_protocol(priv->ieee80211,true);			}
		up(&priv->wx_sem);
		RT_TRACE(COMP_RESET,"%s():<==========down process is finished\n",__FUNCTION__);	
	//rtl8192_irq_disable(dev);
		RT_TRACE(COMP_RESET,"%s():===========>start to up the driver\n",__FUNCTION__);
		reset_status = _rtl8192_up(dev);
		
		RT_TRACE(COMP_RESET,"%s():<===========up process is finished\n",__FUNCTION__);
		if(reset_status == -EAGAIN)
		{
			if(reset_times < 3)
			{
				reset_times++;
				goto RESET_START;
			}
			else
			{
				RT_TRACE(COMP_ERR," ERR!!! %s():  Reset Failed!!\n", __FUNCTION__);
			}
		}
#endif
		ieee->is_silent_reset = 1;
#if 1 
		EnableHWSecurityConfig8192(dev);
#if 1 
		if(ieee->state == IEEE80211_LINKED && ieee->iw_mode == IW_MODE_INFRA)
		{
			ieee->set_chan(ieee->dev, ieee->current_network.channel);
		
#if 1 
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)		
			queue_work(ieee->wq, &ieee->associate_complete_wq);
#else
			schedule_task(&ieee->associate_complete_wq);
#endif
#endif
	
		}
		else if(ieee->state == IEEE80211_LINKED && ieee->iw_mode == IW_MODE_ADHOC)
		{
			ieee->set_chan(ieee->dev, ieee->current_network.channel);
			ieee->link_change(ieee->dev);
	
		//	notify_wx_assoc_event(ieee);
	
			ieee80211_start_send_beacons(ieee);
	
			if (ieee->data_hard_resume)
				ieee->data_hard_resume(ieee->dev);
			netif_carrier_on(ieee->dev);
		}
#endif
		
		CamRestoreAllEntry(dev);

		priv->ResetProgress = RESET_TYPE_NORESET;
		priv->reset_count++;

		priv->bForcedSilentReset =false;
		priv->bResetInProgress = false;

		// For test --> force write UFWP.
		write_nic_byte(dev, UFWP, 1);	
		RT_TRACE(COMP_RESET, "Reset finished!! ====>[%d]\n", priv->reset_count);
#endif
	}
}

void CAM_read_entry(
	struct net_device *dev,
	u32	 		iIndex 
)
{
 	u32 target_command=0;
	 u32 target_content=0;
	 u8 entry_i=0;
	 u32 ulStatus;
	s32 i=100;
//	printk("=======>start read CAM\n"); 
 	for(entry_i=0;entry_i<CAM_CONTENT_COUNT;entry_i++)
 	{
   	// polling bit, and No Write enable, and address
		target_command= entry_i+CAM_CONTENT_COUNT*iIndex;
		target_command= target_command | BIT31;

	//Check polling bit is clear
//	mdelay(1);
#if 1
		while((i--)>=0)
		{
			ulStatus = read_nic_dword(dev, RWCAM);
			if(ulStatus & BIT31){
				continue;
			}
			else{
				break;
			}
		}
#endif
  		write_nic_dword(dev, RWCAM, target_command);
   	 	RT_TRACE(COMP_SEC,"CAM_read_entry(): WRITE A0: %x \n",target_command);
   	 //	printk("CAM_read_entry(): WRITE A0: %lx \n",target_command);
  	 	target_content = read_nic_dword(dev, RCAMO);
  	 	RT_TRACE(COMP_SEC, "CAM_read_entry(): WRITE A8: %x \n",target_content);
  	 //	printk("CAM_read_entry(): WRITE A8: %lx \n",target_content);
 	}
	printk("\n");
}

void rtl819x_update_rxcounts(
	struct r8192_priv *priv,
	u32* TotalRxBcnNum,
	u32* TotalRxDataNum
)	
{
	u16 			SlotIndex;
	u8			i;

	*TotalRxBcnNum = 0;
	*TotalRxDataNum = 0;
	
	SlotIndex = (priv->ieee80211->LinkDetectInfo.SlotIndex++)%(priv->ieee80211->LinkDetectInfo.SlotNum);
	priv->ieee80211->LinkDetectInfo.RxBcnNum[SlotIndex] = priv->ieee80211->LinkDetectInfo.NumRecvBcnInPeriod;
	priv->ieee80211->LinkDetectInfo.RxDataNum[SlotIndex] = priv->ieee80211->LinkDetectInfo.NumRecvDataInPeriod;
	for( i=0; i<priv->ieee80211->LinkDetectInfo.SlotNum; i++ ){	
		*TotalRxBcnNum += priv->ieee80211->LinkDetectInfo.RxBcnNum[i];
		*TotalRxDataNum += priv->ieee80211->LinkDetectInfo.RxDataNum[i];
	}
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
extern	void	rtl819x_watchdog_wqcallback(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
       struct r8192_priv *priv = container_of(dwork,struct r8192_priv,watch_dog_wq);
       struct net_device *dev = priv->ieee80211->dev;
#else
extern	void	rtl819x_watchdog_wqcallback(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
#endif
	struct ieee80211_device* ieee = priv->ieee80211;
	RESET_TYPE	ResetType = RESET_TYPE_NORESET;
      	static u8	check_reset_cnt=0;
	static u8	tx_idle_cnt = 30;
	bool bBusyTraffic = false;
	
	if(!priv->up)
		return;
	
	{//to get busy traffic condition
		if(ieee->state == IEEE80211_LINKED)
		{
			//windows mod 666 to 100.
			//if(	ieee->LinkDetectInfo.NumRxOkInPeriod> 666 ||
			//	ieee->LinkDetectInfo.NumTxOkInPeriod> 666 ) {
			if(	ieee->LinkDetectInfo.NumRxOkInPeriod> 100 ||
				ieee->LinkDetectInfo.NumTxOkInPeriod> 100 ) {
				bBusyTraffic = true;
			}
			//xmit null data packet for preventing disconnection
			if(ieee->LinkDetectInfo.NumTxOkInPeriod == 0){
				tx_idle_cnt--;
				if(tx_idle_cnt == 0){
					tx_idle_cnt = 30;
					//printk("xmit null data packet for preventing disconnect by AP\n");
					ieee80211_sta_ps_send_null_frame(ieee, 0);
				}
			}
			else {
				tx_idle_cnt = 30;
			}
			
			ieee->LinkDetectInfo.NumRxOkInPeriod = 0;
			ieee->LinkDetectInfo.NumTxOkInPeriod = 0;
			ieee->LinkDetectInfo.bBusyTraffic = bBusyTraffic;
		}
	}
	//added by amy for AP roaming
	{
		if(priv->ieee80211->state == IEEE80211_LINKED && priv->ieee80211->iw_mode == IW_MODE_INFRA)
		{
			u32	TotalRxBcnNum = 0;
			u32	TotalRxDataNum = 0;	

			rtl819x_update_rxcounts(priv, &TotalRxBcnNum, &TotalRxDataNum);
			if((TotalRxBcnNum+TotalRxDataNum) == 0)
			{
#ifdef RTK_DMP_PLATFORM
				// add by cfyeh : 2008/12/03 +++
				kobject_hotplug(&priv->ieee80211->dev->class_dev.kobj, KOBJ_LINKDOWN);
				// add by cfyeh : 2008/12/03 ---
#endif
				#ifdef TODO
				if(rfState == eRfOff)
					RT_TRACE(COMP_ERR,"========>%s()\n",__FUNCTION__);
				#endif
				printk("===>%s(): AP is power off,connect another one\n",__FUNCTION__);
			//	Dot11d_Reset(dev);
				priv->ieee80211->state = IEEE80211_ASSOCIATING;
				notify_wx_assoc_event(priv->ieee80211);
				RemovePeerTS(priv->ieee80211,priv->ieee80211->current_network.bssid);
				ieee->is_roaming = true;
				ieee->LinkDetectInfo.bBusyTraffic = false;
				priv->ieee80211->link_change(dev);
				
				if(ieee->LedControlHandler != NULL)
					ieee->LedControlHandler(ieee->dev, LED_CTL_START_TO_LINK); 

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
                                queue_delayed_work(ieee->wq, &ieee->associate_procedure_wq, 0);
#else
                                schedule_task(&priv->ieee80211->associate_procedure_wq);
#endif

			}
		}
		priv->ieee80211->LinkDetectInfo.NumRecvBcnInPeriod=0;
		priv->ieee80211->LinkDetectInfo.NumRecvDataInPeriod=0;	
	}

	hal_dm_watchdog(dev);

#if 1
	//check if reset the driver
	if(!ieee->is_roaming)
	{
    		ResetType = rtl819x_ifcheck_resetornot(dev);
		if(ResetType==RESET_TYPE_SILENT)
			check_reset_cnt++;
	}
#else
	//check if reset the driver
	if(check_reset_cnt++ >= 3 && !ieee->is_roaming)
	{
    		ResetType = rtl819x_ifcheck_resetornot(dev);
		check_reset_cnt = 3;
		//DbgPrint("Start to check silent reset\n");
	}
#endif
	//	RT_TRACE(COMP_RESET,"%s():priv->force_reset is %d,priv->ResetProgress is %d, priv->bForcedSilentReset is %d,priv->bDisableNormalResetCheck is %d,ResetType is %d\n",__FUNCTION__,priv->force_reset,priv->ResetProgress,priv->bForcedSilentReset,priv->bDisableNormalResetCheck,ResetType);
#if 1 
	if( (priv->force_reset) || (priv->ResetProgress==RESET_TYPE_NORESET &&
		(priv->bForcedSilentReset ||
		(!priv->bDisableNormalResetCheck && ResetType==RESET_TYPE_SILENT && check_reset_cnt == 3)))) // This is control by OID set in Pomelo
	{
		RT_TRACE(COMP_RESET,"%s():priv->force_reset is %d,priv->ResetProgress is %d, priv->bForcedSilentReset is %d,priv->bDisableNormalResetCheck is %d,ResetType is %d\n",__FUNCTION__,priv->force_reset,priv->ResetProgress,priv->bForcedSilentReset,priv->bDisableNormalResetCheck,ResetType);
		rtl819x_ifsilentreset(dev);
		check_reset_cnt = 0;
	}
#endif
	priv->force_reset = false;
	priv->bForcedSilentReset = false;
	priv->bResetInProgress = false;
#ifdef RTL8192U
	IbssAgeFunction(ieee);
#endif	
	RT_TRACE(COMP_TRACE, " <==RtUsbCheckForHangWorkItemCallback()\n");
	
}

void watch_dog_timer_callback(unsigned long data)
{
	struct r8192_priv *priv = ieee80211_priv((struct net_device *) data);
	//printk("===============>watch_dog  timer\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	queue_delayed_work(priv->priv_wq,&priv->watch_dog_wq, 0);
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	schedule_task(&priv->watch_dog_wq);
#else
	queue_work(priv->priv_wq,&priv->watch_dog_wq);
#endif
#endif
	mod_timer(&priv->watch_dog_timer, jiffies + MSECS(IEEE80211_WATCH_DOG_TIME));
#if 0
	priv->watch_dog_timer.expires = jiffies + MSECS(IEEE80211_WATCH_DOG_TIME);
	add_timer(&priv->watch_dog_timer);
#endif
}
int _rtl8192_up(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//int i;
	int init_status = 0;
	priv->up=1;
	priv->ieee80211->ieee_up=1;
	RT_TRACE(COMP_INIT, "Bringing up iface");
	init_status = priv->ops->rtl819x_adapter_start(dev);
	if(!init_status)
	{
		RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n", __FUNCTION__);
		priv->up=priv->ieee80211->ieee_up = 0;
		return -EAGAIN;
	}
	RT_TRACE(COMP_INIT, "start adapter finished\n");
	//rtl8192_rx_enable(dev);
	//rtl8192_tx_enable(dev);
	if(priv->ieee80211->state != IEEE80211_LINKED)
	ieee80211_softmac_start_protocol(priv->ieee80211);
	ieee80211_reset_queue(priv->ieee80211);
	watch_dog_timer_callback((unsigned long) dev);
	if(!netif_queue_stopped(dev))
		netif_start_queue(dev);
	else
		netif_wake_queue(dev);

	
	//  Make sure that drop_unencrypted is initialized as "0"
	//  No packets will be sent in non-security mode if we had set drop_unencrypted.
	//  ex, After kill wpa_supplicant process, make the driver up again.
	//  drop_unencrypted remains as "1", which is set by wpa_supplicant. 2008/12/04.john
	// 
	priv->ieee80211->drop_unencrypted = 0;

	return 0;
}


int rtl8192_open(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int ret;
	down(&priv->wx_sem);
	ret = rtl8192_up(dev);
	up(&priv->wx_sem);
	return ret;
	
}


int rtl8192_up(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	if (priv->up == 1) return -1;
	
	return _rtl8192_up(dev);
}


int rtl8192_close(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);

	ret = rtl8192_down(dev);
	
	up(&priv->wx_sem);
	
	return ret;

}

int rtl8192_down(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int i;

	if (priv->up == 0) return -1;
	
	priv->up=0;
	priv->ieee80211->ieee_up = 0;
	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
// FIXME */
	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);
	
        //YJ,add,090323
	priv->ieee80211->wpa_ie_len = 0;
	if(priv->ieee80211->wpa_ie)
		kfree(priv->ieee80211->wpa_ie);
	priv->ieee80211->wpa_ie = NULL;

	//for wps
	priv->ieee80211->wps_ie_len = 0;
	if(priv->ieee80211->wps_ie)
		kfree(priv->ieee80211->wps_ie);
	priv->ieee80211->wps_ie = NULL;

	
        CamResetAllEntry(dev);
                
	rtl8192_rtx_disable(dev);
	//rtl8192_irq_disable(dev);

        // Tx related queue release */
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->ieee80211->skb_waitQ [i]);
        }
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->ieee80211->skb_aggQ [i]);
        }

        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->ieee80211->skb_drv_aggQ [i]);
        }

        //as cancel_delayed_work will del work->timer, so if work is not definedas struct delayed_work, it will corrupt
//	flush_scheduled_work();
	rtl8192_cancel_deferred_work(priv);
	deinit_hal_dm(dev);
	del_timer_sync(&priv->watch_dog_timer);	

	
	ieee80211_softmac_stop_protocol(priv->ieee80211,true);
	memset(&priv->ieee80211->current_network, 0 , offsetof(struct ieee80211_network, list));

#ifdef RTL8192SU
	{//

		printk("rtl8192_down(): disable hw\n");

		write_nic_byte(dev, RF_CTRL, 0x00);

		// Turn off BB	
		//write8(padapter, CR+1, 0x07);
		mdelay(5);

		// Turn off MAC	
		//write8(padapter, SYS_CLKR+1, 0x78); // Switch Control Path
		write_nic_byte(dev, SYS_CLKR+1, 0x38); // Switch Control Path	
		write_nic_byte(dev, SYS_FUNC_EN+1, 0x70); // Reset MACTOP, IOREG, 4181
		write_nic_byte(dev, PMC_FSM, 0x06);  // Enable Loader Data Keep
		write_nic_byte(dev, SYS_ISO_CTRL, 0xF9); // Isolation signals from CORE, PLL
		write_nic_byte(dev, SYS_ISO_CTRL+1, 0xe8); // Enable EFUSE 1.2V(LDO) 
		//write8(padapter, SYS_ISO_CTRL+1, 0x69); // Isolation signals from Loader
		write_nic_byte(dev, AFE_PLL_CTRL, 0x00); // Disable AFE PLL.
		write_nic_byte(dev, LDOA15_CTRL, 0x54);  // Disable A15V
		write_nic_byte(dev, SYS_FUNC_EN+1, 0x50); // Disable E-Fuse 1.2V

		//write8(padapter, SPS1_CTRL, 0x64); // Disable LD12 & SW12 (for IT)
		write_nic_byte(dev, LDOV12D_CTRL, 0x24); // Disable LDO12(for CE)
	
		write_nic_byte(dev, AFE_MISC, 0x30); // Disable AFE BG&MB


	
		// <Roger_Notes> The following  options are alternative.
		// Disable 1.6V LDO or  1.8V Switch. 2008.09.26.

		// Option for Disable 1.6V LDO.	
		write_nic_byte(dev, SPS0_CTRL, 0x56); // Disable 1.6V LDO
		write_nic_byte(dev, SPS0_CTRL+1, 0x43);  // Set SW PFM 

		// Option for Disable 1.8V Switch.	
		//write8(padapter, SPS0_CTRL, 0x55); // Disable SW 1.8V
	}
#endif
	
	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

		return 0;
}


void rtl8192_commit(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int reset_status = 0;
	//u8 reset_times = 0;
	if (priv->up == 0) return ;
	priv->up = 0;

	rtl8192_cancel_deferred_work(priv);
	del_timer_sync(&priv->watch_dog_timer);	
	//cancel_delayed_work(&priv->SwChnlWorkItem);

	ieee80211_softmac_stop_protocol(priv->ieee80211,true);
	
	//rtl8192_irq_disable(dev);
	rtl8192_rtx_disable(dev);
	reset_status = _rtl8192_up(dev);
	
}


//void rtl8192_restart(struct net_device *dev)
//{
//	struct r8192_priv *priv = ieee80211_priv(dev);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_restart(struct work_struct *work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, reset_wq);
        struct net_device *dev = priv->ieee80211->dev;
#else
void rtl8192_restart(struct net_device *dev)
{

        struct r8192_priv *priv = ieee80211_priv(dev);
#endif

	down(&priv->wx_sem);
	
	rtl8192_commit(dev);
	
	up(&priv->wx_sem);
}

static void r8192_set_multicast(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int promisc;
#ifndef RTL8192SU        
        int allmulti;
#endif        
        //down(&priv->wx_sem);
        promisc = (dev->flags & IFF_PROMISC) ? 1:0;
        if (promisc != priv->promisc) {
            // TODO */
        }
        priv->promisc = promisc;
#ifndef RTL8192SU        
        // multicast hardware setting */
        allmulti = (dev->flags & IFF_ALLMULTI) ?1:0;
        if(allmulti||(dev->mc_count > 32)) {
            // write_nic_dword(dev,MAR0,0xffffffff);
            // write_nic_dword(dev,MAR4,0xffffffff); 
            priv->mc_filter[0] = 0xffffffff;
            priv->mc_filter[1] = 0xffffffff;
        } else {
            int i;
            u32 mc_filter[2];
            u32 bit_nr;
            struct dev_mc_list *mclist = dev->mc_list;

            mc_filter[1] = mc_filter[0] = 0;
            for (i = 0; i < dev->mc_count; i++) {
                if (!mclist)
                    break;
                bit_nr = ether_crc(ETH_ALEN, mclist->dmi_addr) >> 26;

                bit_nr &= 0x3F;
                mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
                mclist = mclist->next;
            }

            priv->mc_filter[0] = mc_filter[0];
            priv->mc_filter[1] = mc_filter[1];
            //
            // write_nic_dword(dev,MAR0,mc_filter[0]);
            // write_nic_dword(dev,MAR4,mc_filter[1]);
            // 
        }
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
        queue_work(priv->priv_wq,&priv->mcast_wq);
#else
        schedule_task(&priv->mcast_wq);
#endif
#endif        
        //up(&priv->wx_sem);
}


int r8192_set_mac_adr(struct net_device *dev, void *mac)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct sockaddr *addr = mac;
	
	down(&priv->wx_sem);
	
	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
		
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	schedule_work(&priv->reset_wq);
#else
	schedule_task(&priv->reset_wq);
#endif	
	up(&priv->wx_sem);
	
	return 0;
}

// based on ipw2200 driver */
int rtl8192_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct iwreq *wrq = (struct iwreq *)rq;
	int ret=-1;
	struct ieee80211_device *ieee = priv->ieee80211;
	u32 key[4];
	u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	u8 zero_addr[6] = {0};
	struct iw_point *p = &wrq->u.data;
	struct ieee_param *ipw = NULL;//(struct ieee_param *)wrq->u.data.pointer;

	down(&priv->wx_sem);

#ifdef CONFIG_MP_TOOL

	if(cmd == SIOCIWFIRSTPRIV+15)
	{
		ret = r8192u_mp_ioctl_handle(dev, NULL, &(wrq->u), (char *) &(wrq->u));
		goto out;
	}

#endif

     if (p->length < sizeof(struct ieee_param) || !p->pointer){
             ret = -EINVAL;
             goto out;
	}

     ipw = (struct ieee_param *)kmalloc(p->length, GFP_KERNEL);
     if (ipw == NULL){
             ret = -ENOMEM;
             goto out;
     }
     if (copy_from_user(ipw, p->pointer, p->length)) {
		kfree(ipw);
            ret = -EFAULT;
            goto out;
	}		

	switch (cmd) {
	    case RTL_IOCTL_WPA_SUPPLICANT:
	//parse here for HW security	
			if (ipw->cmd == IEEE_CMD_SET_ENCRYPTION)
			{
				if (ipw->u.crypt.set_tx)
				{
					if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
						ieee->pairwise_key_type = KEY_TYPE_CCMP;
					else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
						ieee->pairwise_key_type = KEY_TYPE_TKIP;
					else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
					{
						if (ipw->u.crypt.key_len == 13)
							ieee->pairwise_key_type = KEY_TYPE_WEP104;
						else if (ipw->u.crypt.key_len == 5)
							ieee->pairwise_key_type = KEY_TYPE_WEP40;
					}
					else
						ieee->pairwise_key_type = KEY_TYPE_NA;

					if (ieee->pairwise_key_type)
					{
				//	FIXME:these two lines below just to fix ipw interface bug, that is, it will never set mode down to driver. So treat it as ADHOC mode, if no association procedure. WB. 2009.02.04 
						if (memcmp(ieee->ap_mac_addr, zero_addr, 6) == 0)
							ieee->iw_mode = IW_MODE_ADHOC;
						memcpy((u8*)key, ipw->u.crypt.key, 16);
						EnableHWSecurityConfig8192(dev);
					//we fill both index entry and 4th entry for pairwise key as in IPW interface, adhoc will only get here, so we need index entry for its default key serching!
					//added by WB.
						setKey(dev, 4, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key);
						if (ieee->iw_mode == IW_MODE_ADHOC)
						setKey(dev, ipw->u.crypt.idx, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key);
					}
				}
				else //if (ipw->u.crypt.idx) //group key use idx > 0
				{
					memcpy((u8*)key, ipw->u.crypt.key, 16);
					if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
						ieee->group_key_type= KEY_TYPE_CCMP;
					else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
						ieee->group_key_type = KEY_TYPE_TKIP;
					else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
					{
						if (ipw->u.crypt.key_len == 13)
							ieee->group_key_type = KEY_TYPE_WEP104;
						else if (ipw->u.crypt.key_len == 5)
							ieee->group_key_type = KEY_TYPE_WEP40;
					}
					else
						ieee->group_key_type = KEY_TYPE_NA;

					if (ieee->group_key_type)
					{
							setKey(	dev, 
								ipw->u.crypt.idx,
								ipw->u.crypt.idx,		//KeyIndex 
						     		ieee->group_key_type,	//KeyType
						            	broadcast_addr,	//MacAddr
								0,		//DefaultKey
							      	key);		//KeyContent
					}
				}
			}
#ifdef JOHN_HWSEC_DEBUG
		//john's test 0711
		printk("@@ wrq->u pointer = ");
		for(i=0;i<wrq->u.data.length;i++){
			if(i%10==0) printk("\n");
			printk( "%8x|", ((u32*)wrq->u.data.pointer)[i] );
		}
		printk("\n");
#endif //JOHN_HWSEC_DEBUG*/
		ret = ieee80211_wpa_supplicant_ioctl(priv->ieee80211, &wrq->u.data);
		break; 

	    default:
		ret = -EOPNOTSUPP;
		break;
	}
	kfree(ipw);
        ipw = NULL;
out:
	up(&priv->wx_sem);
	return ret;
}

#ifdef RTL8192SU
u8 rtl8192SU_HwRateToMRate(bool bIsHT, u8 rate,bool bFirstAMPDU)
{

	u8	ret_rate = 0x02;

	if( bFirstAMPDU )
	{
	if(!bIsHT)
	{
		switch(rate)
		{
		
			case DESC92S_RATE1M:		ret_rate = MGN_1M;		break;
			case DESC92S_RATE2M:		ret_rate = MGN_2M;		break;
			case DESC92S_RATE5_5M:		ret_rate = MGN_5_5M;		break;
			case DESC92S_RATE11M:		ret_rate = MGN_11M;		break;
			case DESC92S_RATE6M:		ret_rate = MGN_6M;		break;
			case DESC92S_RATE9M:		ret_rate = MGN_9M;		break;
			case DESC92S_RATE12M:		ret_rate = MGN_12M;		break;
			case DESC92S_RATE18M:		ret_rate = MGN_18M;		break;
			case DESC92S_RATE24M:		ret_rate = MGN_24M;		break;
			case DESC92S_RATE36M:		ret_rate = MGN_36M;		break;
			case DESC92S_RATE48M:		ret_rate = MGN_48M;		break;
			case DESC92S_RATE54M:		ret_rate = MGN_54M;		break;
		
			default:							
				RT_TRACE(COMP_RECV, "HwRateToMRate90(): Non supported Rate [%x], bIsHT = %d!!!\n", rate, bIsHT);
					break;
	}
		}
		else
	{
		switch(rate)
		{
		
			case DESC92S_RATEMCS0:	ret_rate = MGN_MCS0;		break;
			case DESC92S_RATEMCS1:	ret_rate = MGN_MCS1;		break;
			case DESC92S_RATEMCS2:	ret_rate = MGN_MCS2;		break;
			case DESC92S_RATEMCS3:	ret_rate = MGN_MCS3;		break;
			case DESC92S_RATEMCS4:	ret_rate = MGN_MCS4;		break;
			case DESC92S_RATEMCS5:	ret_rate = MGN_MCS5;		break;
			case DESC92S_RATEMCS6:	ret_rate = MGN_MCS6;		break;
			case DESC92S_RATEMCS7:	ret_rate = MGN_MCS7;		break;
			case DESC92S_RATEMCS8:	ret_rate = MGN_MCS8;		break;
			case DESC92S_RATEMCS9:	ret_rate = MGN_MCS9;		break;
			case DESC92S_RATEMCS10:	ret_rate = MGN_MCS10;	break;
			case DESC92S_RATEMCS11:	ret_rate = MGN_MCS11;	break;
			case DESC92S_RATEMCS12:	ret_rate = MGN_MCS12;	break;
			case DESC92S_RATEMCS13:	ret_rate = MGN_MCS13;	break;
			case DESC92S_RATEMCS14:	ret_rate = MGN_MCS14;	break;
			case DESC92S_RATEMCS15:	ret_rate = MGN_MCS15;	break;
			case DESC92S_RATEMCS32:	ret_rate = (0x80|0x20);	break;
			
			default:							
					RT_TRACE(COMP_RECV, "HwRateToMRate92S(): Non supported Rate [%x], bIsHT = %d!!!\n",rate, bIsHT );
				break;
		}

	}	
	}
	else
	{
		switch(rate)
		{
		
			case DESC92S_RATE1M:	ret_rate = MGN_1M;		break;
			case DESC92S_RATE2M:	ret_rate = MGN_2M;		break;
			case DESC92S_RATE5_5M:	ret_rate = MGN_5_5M;		break;
			case DESC92S_RATE11M:	ret_rate = MGN_11M;		break;
			case DESC92S_RATE6M:	ret_rate = MGN_6M;		break;
			case DESC92S_RATE9M:	ret_rate = MGN_9M;		break;
			case DESC92S_RATE12M:	ret_rate = MGN_12M;		break;
			case DESC92S_RATE18M:	ret_rate = MGN_18M;		break;
			case DESC92S_RATE24M:	ret_rate = MGN_24M;		break;
			case DESC92S_RATE36M:	ret_rate = MGN_36M;		break;
			case DESC92S_RATE48M:	ret_rate = MGN_48M;		break;
			case DESC92S_RATE54M:	ret_rate = MGN_54M;		break;
			case DESC92S_RATEMCS0:	ret_rate = MGN_MCS0;		break;
			case DESC92S_RATEMCS1:	ret_rate = MGN_MCS1;		break;
			case DESC92S_RATEMCS2:	ret_rate = MGN_MCS2;		break;
			case DESC92S_RATEMCS3:	ret_rate = MGN_MCS3;		break;
			case DESC92S_RATEMCS4:	ret_rate = MGN_MCS4;		break;
			case DESC92S_RATEMCS5:	ret_rate = MGN_MCS5;		break;
			case DESC92S_RATEMCS6:	ret_rate = MGN_MCS6;		break;
			case DESC92S_RATEMCS7:	ret_rate = MGN_MCS7;		break;
			case DESC92S_RATEMCS8:	ret_rate = MGN_MCS8;		break;
			case DESC92S_RATEMCS9:	ret_rate = MGN_MCS9;		break;
			case DESC92S_RATEMCS10:	ret_rate = MGN_MCS10;	break;
			case DESC92S_RATEMCS11:	ret_rate = MGN_MCS11;	break;
			case DESC92S_RATEMCS12:	ret_rate = MGN_MCS12;	break;
			case DESC92S_RATEMCS13:	ret_rate = MGN_MCS13;	break;
			case DESC92S_RATEMCS14:	ret_rate = MGN_MCS14;	break;
			case DESC92S_RATEMCS15:	ret_rate = MGN_MCS15;	break;
			case DESC92S_RATEMCS32:	ret_rate = (0x80|0x20);	break;			
			
			default:							
				RT_TRACE(COMP_RECV, "HwRateToMRate92S(): Non supported Rate [%x], bIsHT = %d!!!\n",rate, bIsHT );
				break;
			}	
	}
	return ret_rate;
}
#endif

u8 HwRateToMRate90(bool bIsHT, u8 rate)
{
	u8  ret_rate = 0xff;

	if(!bIsHT) {
		switch(rate) {
			case DESC90_RATE1M:   ret_rate = MGN_1M;         break;
			case DESC90_RATE2M:   ret_rate = MGN_2M;         break;
			case DESC90_RATE5_5M: ret_rate = MGN_5_5M;       break;
			case DESC90_RATE11M:  ret_rate = MGN_11M;        break;
			case DESC90_RATE6M:   ret_rate = MGN_6M;         break;
			case DESC90_RATE9M:   ret_rate = MGN_9M;         break;
			case DESC90_RATE12M:  ret_rate = MGN_12M;        break;
			case DESC90_RATE18M:  ret_rate = MGN_18M;        break;
			case DESC90_RATE24M:  ret_rate = MGN_24M;        break;
			case DESC90_RATE36M:  ret_rate = MGN_36M;        break;
			case DESC90_RATE48M:  ret_rate = MGN_48M;        break;
			case DESC90_RATE54M:  ret_rate = MGN_54M;        break;

			default:
				ret_rate = 0xff;
				RT_TRACE(COMP_RECV, "HwRateToMRate90(): Non supported Rate [%x], bIsHT = %d!!!\n", rate, bIsHT);
				break;
		}

	} else {
		switch(rate) {
			case DESC90_RATEMCS0:   ret_rate = MGN_MCS0;    break;
			case DESC90_RATEMCS1:   ret_rate = MGN_MCS1;    break;
			case DESC90_RATEMCS2:   ret_rate = MGN_MCS2;    break;
			case DESC90_RATEMCS3:   ret_rate = MGN_MCS3;    break;
			case DESC90_RATEMCS4:   ret_rate = MGN_MCS4;    break;
			case DESC90_RATEMCS5:   ret_rate = MGN_MCS5;    break;
			case DESC90_RATEMCS6:   ret_rate = MGN_MCS6;    break;
			case DESC90_RATEMCS7:   ret_rate = MGN_MCS7;    break;
			case DESC90_RATEMCS8:   ret_rate = MGN_MCS8;    break;
			case DESC90_RATEMCS9:   ret_rate = MGN_MCS9;    break;
			case DESC90_RATEMCS10:  ret_rate = MGN_MCS10;   break;
			case DESC90_RATEMCS11:  ret_rate = MGN_MCS11;   break;
			case DESC90_RATEMCS12:  ret_rate = MGN_MCS12;   break;
			case DESC90_RATEMCS13:  ret_rate = MGN_MCS13;   break;
			case DESC90_RATEMCS14:  ret_rate = MGN_MCS14;   break;
			case DESC90_RATEMCS15:  ret_rate = MGN_MCS15;   break;
			case DESC90_RATEMCS32:  ret_rate = (0x80|0x20); break;

			default:
				ret_rate = 0xff;
				RT_TRACE(COMP_RECV, "HwRateToMRate90(): Non supported Rate [%x], bIsHT = %d!!!\n",rate, bIsHT);
				break;
		}
	}

	return ret_rate;
}

//
// Function:     UpdateRxPktTimeStamp
// Overview:     Recored down the TSF time stamp when receiving a packet
// 
// Input:                
//       PADAPTER        Adapter
//       PRT_RFD         pRfd,
//                       
// Output:               
//       PRT_RFD         pRfd
//                               (pRfd->Status.TimeStampHigh is updated)
//                               (pRfd->Status.TimeStampLow is updated)
// Return:       
//               None
//
void UpdateRxPktTimeStamp8190 (struct net_device *dev, struct ieee80211_rx_stats *stats)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	if(stats->bIsAMPDU && !stats->bFirstMPDU) {
		stats->mac_time[0] = priv->LastRxDescTSFLow;
		stats->mac_time[1] = priv->LastRxDescTSFHigh;
	} else {
		priv->LastRxDescTSFLow = stats->mac_time[0];
		priv->LastRxDescTSFHigh = stats->mac_time[1];
	}
}

//by amy 080606

long rtl819x_translate_todbm(u8 signal_strength_index	)// 0-100 index.
{
	long	signal_power; // in dBm.

	// Translate to dBm (x=0.5y-95).
	signal_power = (long)((signal_strength_index + 1) >> 1); 
	signal_power -= 95; 

	return signal_power;
}


//    2008/01/22 MH We can not delcare RSSI/EVM total value of sliding window to
//    be a local static. Otherwise, it may increase when we return from S3/S4. The
//    value will be kept in memory or disk. We must delcare the value in adapter
//    and it will be reinitialized when return from S3/S4. 
void rtl8192_process_phyinfo(struct r8192_priv * priv,u8* buffer, struct ieee80211_rx_stats * pprevious_stats, struct ieee80211_rx_stats * pcurrent_stats)
{
	bool bcheck = false;
	u8	rfpath;
	u32	nspatial_stream, tmp_val;
	static u32 slide_rssi_index=0, slide_rssi_statistics=0;	
	static u32 slide_evm_index=0, slide_evm_statistics=0;
	static u32 last_rssi=0, last_evm=0;

	static u32 slide_beacon_adc_pwdb_index=0, slide_beacon_adc_pwdb_statistics=0;
	static u32 last_beacon_adc_pwdb=0;

	struct ieee80211_hdr_3addr *hdr;
	u16 sc ;
	unsigned int frag,seq;
	hdr = (struct ieee80211_hdr_3addr *)buffer;
	sc = le16_to_cpu(hdr->seq_ctl);
	frag = WLAN_GET_SEQ_FRAG(sc);
	seq = WLAN_GET_SEQ_SEQ(sc);
	//cosa add 04292008 to record the sequence number
	pcurrent_stats->Seq_Num = seq;
	//
	// Check whether we should take the previous packet into accounting
	//
	if(!pprevious_stats->bIsAMPDU)
	{
		// if previous packet is not aggregated packet
		bcheck = true;
	}else 
	{
	#if 0
		// if previous packet is aggregated packet, and current packet
		//  (1) is not AMPDU
		//  (2) is the first packet of one AMPDU
		// that means the previous packet is the last one aggregated packet
		if( !pcurrent_stats->bIsAMPDU || pcurrent_stats->bFirstMPDU)
			bcheck = true;
	#endif
	}

	//
	// If the previous packet does not match the criteria, neglect it
	//
	if(!pprevious_stats->bPacketMatchBSSID)
	{
		if(!pprevious_stats->bToSelfBA)
			return;
	}

	if(!bcheck)
		return;

	//rtl8190_process_cck_rxpathsel(priv,pprevious_stats);//only rtl8190 supported

	//
	// Check RSSI
	//
	if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA)
	{
		if(slide_rssi_statistics++ >= PHY_RSSI_SLID_WIN_MAX)
		{
			slide_rssi_statistics = PHY_RSSI_SLID_WIN_MAX;
			last_rssi = priv->stats.slide_signal_strength[slide_rssi_index];
			priv->stats.slide_rssi_total -= last_rssi;
		}
		priv->stats.slide_rssi_total += pprevious_stats->SignalStrength;
		
		priv->stats.slide_signal_strength[slide_rssi_index++] = pprevious_stats->SignalStrength;
		if(slide_rssi_index >= PHY_RSSI_SLID_WIN_MAX)
			slide_rssi_index = 0;
		
		// <1> Showed on UI for user, in dbm
		tmp_val = priv->stats.slide_rssi_total/slide_rssi_statistics;
		priv->stats.signal_strength = rtl819x_translate_todbm((u8)tmp_val);
		pcurrent_stats->rssi = priv->stats.signal_strength;
	}

	priv->stats.num_process_phyinfo++;
	
	// <2> Showed on UI for engineering
	// hardware does not provide rssi information for each rf path in CCK
	if(!pprevious_stats->bIsCCK && (pprevious_stats->bPacketToSelf || pprevious_stats->bToSelfBA))
	{
		for (rfpath = RF90_PATH_A; rfpath < priv->NumTotalRFPath; rfpath++)
		{
                     if (!rtl8192_phy_CheckIsLegalRFPath(priv->ieee80211->dev, rfpath))		
			         continue;
	
			//Fixed by Jacken 2008-03-20
			if(priv->stats.rx_rssi_percentage[rfpath] == 0)
			{
				priv->stats.rx_rssi_percentage[rfpath] = pprevious_stats->RxMIMOSignalStrength[rfpath];
				//DbgPrint("MIMO RSSI initialize \n");
			}	
			if(pprevious_stats->RxMIMOSignalStrength[rfpath]  > priv->stats.rx_rssi_percentage[rfpath])
			{
				priv->stats.rx_rssi_percentage[rfpath] = 
					( (priv->stats.rx_rssi_percentage[rfpath]*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxMIMOSignalStrength[rfpath])) /(Rx_Smooth_Factor);
				priv->stats.rx_rssi_percentage[rfpath] = priv->stats.rx_rssi_percentage[rfpath]  + 1;
			}
			else
			{
				priv->stats.rx_rssi_percentage[rfpath] = 
					( (priv->stats.rx_rssi_percentage[rfpath]*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxMIMOSignalStrength[rfpath])) /(Rx_Smooth_Factor);
			}	
			RT_TRACE(COMP_DBG,"priv->stats.rx_rssi_percentage[rfPath]  = %d \n" ,priv->stats.rx_rssi_percentage[rfpath] );
		}		
	}
	
	
	//
	// Check PWDB.
	//
	RT_TRACE(COMP_RXDESC, "Smooth %s PWDB = %d\n", 
				pprevious_stats->bIsCCK? "CCK": "OFDM",
				pprevious_stats->RxPWDBAll);

	if(pprevious_stats->bPacketBeacon)
	{
// record the beacon pwdb to the sliding window. */
		if(slide_beacon_adc_pwdb_statistics++ >= PHY_Beacon_RSSI_SLID_WIN_MAX)
		{
			slide_beacon_adc_pwdb_statistics = PHY_Beacon_RSSI_SLID_WIN_MAX;	
			last_beacon_adc_pwdb = priv->stats.Slide_Beacon_pwdb[slide_beacon_adc_pwdb_index];
			priv->stats.Slide_Beacon_Total -= last_beacon_adc_pwdb;
			//DbgPrint("slide_beacon_adc_pwdb_index = %d, last_beacon_adc_pwdb = %d, Adapter->RxStats.Slide_Beacon_Total = %d\n", 
			//	slide_beacon_adc_pwdb_index, last_beacon_adc_pwdb, Adapter->RxStats.Slide_Beacon_Total);
		}
		priv->stats.Slide_Beacon_Total += pprevious_stats->RxPWDBAll;
		priv->stats.Slide_Beacon_pwdb[slide_beacon_adc_pwdb_index] = pprevious_stats->RxPWDBAll;
		//DbgPrint("slide_beacon_adc_pwdb_index = %d, pPreviousRfd->Status.RxPWDBAll = %d\n", slide_beacon_adc_pwdb_index, pPreviousRfd->Status.RxPWDBAll);
		slide_beacon_adc_pwdb_index++;
		if(slide_beacon_adc_pwdb_index >= PHY_Beacon_RSSI_SLID_WIN_MAX)
			slide_beacon_adc_pwdb_index = 0;
		pprevious_stats->RxPWDBAll = priv->stats.Slide_Beacon_Total/slide_beacon_adc_pwdb_statistics;
		if(pprevious_stats->RxPWDBAll >= 3)
			pprevious_stats->RxPWDBAll -= 3;	
	}

	RT_TRACE(COMP_RXDESC, "Smooth %s PWDB = %d\n", 
				pprevious_stats->bIsCCK? "CCK": "OFDM",
				pprevious_stats->RxPWDBAll);
	
	
	if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA)
	{
		if(priv->undecorated_smoothed_pwdb < 0)	// initialize
		{
			priv->undecorated_smoothed_pwdb = pprevious_stats->RxPWDBAll;
			//DbgPrint("First pwdb initialize \n");
		}
#if 1
		if(pprevious_stats->RxPWDBAll > (u32)priv->undecorated_smoothed_pwdb)
		{
			priv->undecorated_smoothed_pwdb =	
					( ((priv->undecorated_smoothed_pwdb)*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxPWDBAll)) /(Rx_Smooth_Factor);
			priv->undecorated_smoothed_pwdb = priv->undecorated_smoothed_pwdb + 1;
		}
		else
		{
			priv->undecorated_smoothed_pwdb =	
					( ((priv->undecorated_smoothed_pwdb)*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxPWDBAll)) /(Rx_Smooth_Factor);
		}
#else
		//Fixed by Jacken 2008-03-20
		if(pPreviousRfd->Status.RxPWDBAll > (u32)pHalData->UndecoratedSmoothedPWDB)
		{
			pHalData->UndecoratedSmoothedPWDB = 	
					( ((pHalData->UndecoratedSmoothedPWDB)* 5) + (pPreviousRfd->Status.RxPWDBAll)) / 6;
			pHalData->UndecoratedSmoothedPWDB = pHalData->UndecoratedSmoothedPWDB + 1;
		}
		else
		{
			pHalData->UndecoratedSmoothedPWDB = 	
					( ((pHalData->UndecoratedSmoothedPWDB)* 5) + (pPreviousRfd->Status.RxPWDBAll)) / 6;
		}		
#endif

	}

	//
	// Check EVM
	//	
	// record the general EVM to the sliding window. */
	if(pprevious_stats->SignalQuality == 0)
	{
	}
	else
	{
		if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA){
			if(slide_evm_statistics++ >= PHY_RSSI_SLID_WIN_MAX){
				slide_evm_statistics = PHY_RSSI_SLID_WIN_MAX;
				last_evm = priv->stats.slide_evm[slide_evm_index];
				priv->stats.slide_evm_total -= last_evm;
			}
	
			priv->stats.slide_evm_total += pprevious_stats->SignalQuality;
	
			priv->stats.slide_evm[slide_evm_index++] = pprevious_stats->SignalQuality;
			if(slide_evm_index >= PHY_RSSI_SLID_WIN_MAX)
				slide_evm_index = 0;
	
			// <1> Showed on UI for user, in percentage.
			tmp_val = priv->stats.slide_evm_total/slide_evm_statistics;
			priv->stats.signal_quality = tmp_val;
			//cosa add 10/11/2007, Showed on UI for user in Windows Vista, for Link quality.
			priv->stats.last_signal_strength_inpercent = tmp_val;
		}

		// <2> Showed on UI for engineering
		if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA)
		{
			for(nspatial_stream = 0; nspatial_stream<2 ; nspatial_stream++) // 2 spatial stream
			{
				if(pprevious_stats->RxMIMOSignalQuality[nspatial_stream] != -1)
				{
					if(priv->stats.rx_evm_percentage[nspatial_stream] == 0)	// initialize
					{
						priv->stats.rx_evm_percentage[nspatial_stream] = pprevious_stats->RxMIMOSignalQuality[nspatial_stream];
					}
					priv->stats.rx_evm_percentage[nspatial_stream] = 
						( (priv->stats.rx_evm_percentage[nspatial_stream]* (Rx_Smooth_Factor-1)) + 
						(pprevious_stats->RxMIMOSignalQuality[nspatial_stream]* 1)) / (Rx_Smooth_Factor);
				}
			}
		}
	}
	
	
}

//-----------------------------------------------------------------------------
// Function:	rtl819x_query_rxpwrpercentage()
//
// Overview:	
//
// Input:		char		antpower
//
// Output:		NONE
//
// Return:		0-100 percentage
//
// Revised History:
//	When		Who		Remark
//	05/26/2008	amy		Create Version 0 porting from windows code.  
//
//---------------------------------------------------------------------------*/
static u8 rtl819x_query_rxpwrpercentage(
	char		antpower
	)
{
	if ((antpower <= -100) || (antpower >= 20))
	{
		return	0;
	}
	else if (antpower >= 0)
	{
		return	100;
	}
	else
	{
		return	(100+antpower);
	}
	
}	/* QueryRxPwrPercentage */

static u8 
rtl819x_evm_dbtopercentage(
    char value
    )
{
    char ret_val;
    
    ret_val = value;
    
    if(ret_val >= 0)
        ret_val = 0;
    if(ret_val <= -33)
        ret_val = -33;
    ret_val = 0 - ret_val;
    ret_val*=3;
	if(ret_val == 99)
		ret_val = 100;
    return(ret_val);
}
//
//	Description:
// 	We want good-looking for signal strength/quality
//	2007/7/19 01:09, by cosa.
//
long
rtl819x_signal_scale_mapping(
	long currsig
	)
{
	long retsig;

	// Step 1. Scale mapping.
	if(currsig >= 81 && currsig <= 100)
	{
		retsig = 100; 
	}
	if(currsig >= 61 && currsig <= 80)
	{
		// mapping to 90~95
		retsig = 90 + ((currsig - 60) / 4);
	}
	else if(currsig >= 41 && currsig <= 60)
	{
		// mapping to 78~88
		retsig = 78 + ((currsig - 40) / 2);
	}
	else if(currsig >= 31 && currsig <= 40)
	{
		// mapping to 67~76
		retsig = 66 + (currsig - 30);
	}
	else if(currsig >= 21 && currsig <= 30)
	{
		// mapping to 55~64
		retsig = 54 + (currsig - 20);
	}
	else if(currsig >= 5 && currsig <= 20)
	{
		// mapping to 42~52
		retsig = 42 + (((currsig - 5) * 2) / 3);
	}
	else if(currsig == 4)
	{
		retsig = 15;
	}
	else if(currsig == 3)
	{
		retsig = 13;
	}
	else if(currsig == 2)
	{
		retsig = 11;
	}
	else if(currsig == 1)
	{
		retsig = 9;
	}
	else
	{
		retsig = currsig;
	}
	
	return retsig;
}

#ifdef RTL8192SU
//-----------------------------------------------------------------------------
// Function:	QueryRxPhyStatus8192S()
//
// Overview:	
//
// Input:		NONE
//
// Output:		NONE
//
// Return:		NONE
//
// Revised History:
//	When		Who		Remark
//	06/01/2007	MHC		Create Version 0.  
//	06/05/2007	MHC		Accordign to HW's new data sheet, we add CCK and OFDM
//						descriptor definition.
//	07/04/2007	MHC		According to Jerry and Bryant's document. We read 
//						ir_isolation and ext_lna for RF's init value and use
//						to compensate RSSI after receiving packets.
//	09/10/2008	MHC		Modify name and PHY status field for 92SE.
//	09/19/2008	MHC		Add CCK/OFDM SS/SQ for 92S series. 
//
//---------------------------------------------------------------------------*/
static void rtl8192SU_query_rxphystatus(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pstats,
	rx_desc_819x_usb	*pDesc,
	rx_drvinfo_819x_usb  * pdrvinfo,
	struct ieee80211_rx_stats * precord_stats,
	bool bpacket_match_bssid,
	bool bpacket_toself,
	bool bPacketBeacon,
	bool bToSelfBA
	)
{	
	//PRT_RFD_STATUS		pRtRfdStatus = &(pRfd->Status);	
	//PHY_STS_CCK_8192S_T	*pCck_buf;	
	phy_sts_cck_819xusb_t	*	pcck_buf;
	phy_ofdm_rx_status_rxsc_sgien_exintfflag* prxsc;
	//u8				*prxpkt;
	//u8				i, max_spatial_stream, tmp_rxsnr, tmp_rxevm, rxsc_sgien_exflg;
	u8				i, max_spatial_stream, rxsc_sgien_exflg;
	char				rx_pwr[4], rx_pwr_all=0;
	//long				rx_avg_pwr = 0;
	//char				rx_snrX, rx_evmX;
	u8				evm, pwdb_all;
	u32				RSSI, total_rssi=0;//, total_evm=0;
//	long				signal_strength_index = 0;
	u8				is_cck_rate=0;
	u8				rf_rx_num = 0;

	

	priv->stats.numqry_phystatus++;
	
	is_cck_rate = rx_hal_is_cck_rate(pDesc);

	// Record it for next packet processing
	memset(precord_stats, 0, sizeof(struct ieee80211_rx_stats));
	pstats->bPacketMatchBSSID = precord_stats->bPacketMatchBSSID = bpacket_match_bssid;
	pstats->bPacketToSelf = precord_stats->bPacketToSelf = bpacket_toself;
	pstats->bIsCCK = precord_stats->bIsCCK = is_cck_rate;//RX_HAL_IS_CCK_RATE(pDrvInfo);
	pstats->bPacketBeacon = precord_stats->bPacketBeacon = bPacketBeacon;
	pstats->bToSelfBA = precord_stats->bToSelfBA = bToSelfBA;

#ifndef RTL8192SU	
	phy_sts_ofdm_819xusb_t*	pofdm_buf = NULL;
	prxpkt = (u8*)pdrvinfo;	
	
	// Move pointer to the 16th bytes. Phy status start address. */	
	prxpkt += sizeof(rx_drvinfo_819x_usb);	
	
	// Initial the cck and ofdm buffer pointer */
	pcck_buf = (phy_sts_cck_819xusb_t *)prxpkt;
	pofdm_buf = (phy_sts_ofdm_819xusb_t *)prxpkt;			
#endif

	pstats->RxMIMOSignalQuality[0] = -1;
	pstats->RxMIMOSignalQuality[1] = -1;
	precord_stats->RxMIMOSignalQuality[0] = -1;
	precord_stats->RxMIMOSignalQuality[1] = -1;
			
	if(is_cck_rate)
	{
		u8 report;//, tmp_pwdb;
		//char cck_adc_pwdb[4];
	
		// CCK Driver info Structure is not the same as OFDM packet.
		pcck_buf = (phy_sts_cck_819xusb_t *)pdrvinfo;	
	
		// 
		// (1)Hardware does not provide RSSI for CCK
		//

		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//

		priv->stats.numqry_phystatusCCK++;

		if(!priv->bCckHighPower)
		{
			report = pcck_buf->cck_agc_rpt & 0xc0;
			report = report>>6;
			switch(report)
			{
				//Fixed by Jacken from Bryant 2008-03-20
				//Original value is -38 , -26 , -14 , -2
				//Fixed value is -35 , -23 , -11 , 6
				case 0x3:
					rx_pwr_all = -35 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x2:
					rx_pwr_all = -23 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x1:
					rx_pwr_all = -11 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x0:
					rx_pwr_all = 8 - (pcck_buf->cck_agc_rpt & 0x3e);//6->8
					break;
			}
		}
		else
		{
			report = pdrvinfo->cfosho[0] & 0x60;
			report = report>>5;
			switch(report)
			{
				case 0x3:
					rx_pwr_all = -35 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x2:
					rx_pwr_all = -23 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x1:
					rx_pwr_all = -11 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x0:
					rx_pwr_all = -8 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;//6->-8
					break;
			}
		}

		pwdb_all = rtl819x_query_rxpwrpercentage(rx_pwr_all);//check it
		pstats->RxPWDBAll = precord_stats->RxPWDBAll = pwdb_all;
		//pstats->RecvSignalPower = pwdb_all;
		pstats->RecvSignalPower = rx_pwr_all;

		//
		// (3) Get Signal Quality (EVM)
		//
	//if(bpacket_match_bssid)
	{
			u8	sq;

			if(pstats->RxPWDBAll > 40)
			{
				sq = 100;
			}else
			{
				sq = pcck_buf->sq_rpt;
				
				if(pcck_buf->sq_rpt > 64)
					sq = 0;
				else if (pcck_buf->sq_rpt < 20)
					sq = 100;
				else
					sq = ((64-sq) * 100) / 44;
			}
			pstats->SignalQuality = precord_stats->SignalQuality = sq;
			pstats->RxMIMOSignalQuality[0] = precord_stats->RxMIMOSignalQuality[0] = sq;
			pstats->RxMIMOSignalQuality[1] = precord_stats->RxMIMOSignalQuality[1] = -1;
		}
	}
	else
	{
		priv->stats.numqry_phystatusHT++;

		// 2008/09/19 MH For 92S debug, RX RF path always enable!!
		priv->brfpath_rxenable[0] = priv->brfpath_rxenable[1] = TRUE;

		// 
		// (1)Get RSSI for HT rate
		//
		//for(i=RF90_PATH_A; i<priv->NumTotalRFPath; i++)
		for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
		{
			// 2008/01/30 MH we will judge RF RX path now.
			if (priv->brfpath_rxenable[i])
				rf_rx_num++;
			//else
			//	continue;

		//if (!rtl8192_phy_CheckIsLegalRFPath(priv->ieee80211->dev, i))		
		//		continue;

			//Fixed by Jacken from Bryant 2008-03-20
			//Original value is 106
			//rx_pwr[i] = ((pofdm_buf->trsw_gain_X[i]&0x3F)*2) - 106;
			rx_pwr[i] = ((pdrvinfo->gain_trsw[i]&0x3F)*2) - 110;

			// Translate DBM to percentage. */
			RSSI = rtl819x_query_rxpwrpercentage(rx_pwr[i]);	//check ok
			total_rssi += RSSI;
			RT_TRACE(COMP_RF, "RF-%d RXPWR=%x RSSI=%d\n", i, rx_pwr[i], RSSI);
			
			//Get Rx snr value in DB
			//tmp_rxsnr =	pofdm_buf->rxsnr_X[i];
			//rx_snrX = (char)(tmp_rxsnr);
			//rx_snrX /= 2;
			//priv->stats.rxSNRdB[i] = (long)rx_snrX;
			priv->stats.rxSNRdB[i] = (long)(pdrvinfo->rxsnr[i]/2);
			
			// Translate DBM to percentage. */
			//RSSI = rtl819x_query_rxpwrpercentage(rx_pwr[i]);	
			//total_rssi += RSSI;

			// Record Signal Strength for next packet */
			//if(bpacket_match_bssid)
			{
				pstats->RxMIMOSignalStrength[i] =(u8) RSSI;
				precord_stats->RxMIMOSignalStrength[i] =(u8) RSSI;
			}
		}
		
		
		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		//Fixed by Jacken from Bryant 2008-03-20
		//Original value is 106
		//rx_pwr_all = (((pofdm_buf->pwdb_all ) >> 1 )& 0x7f) -106;
		rx_pwr_all = (((pdrvinfo->pwdb_all ) >> 1 )& 0x7f) -106;
		pwdb_all = rtl819x_query_rxpwrpercentage(rx_pwr_all);	

		pstats->RxPWDBAll = precord_stats->RxPWDBAll = pwdb_all;
		pstats->RxPower = precord_stats->RxPower =  rx_pwr_all;
		pstats->RecvSignalPower = rx_pwr_all;
		
		//
		// (3)EVM of HT rate
		//
		//if(pdrvinfo->RxHT && pdrvinfo->RxRate>=DESC90_RATEMCS8 &&
		 //	pdrvinfo->RxRate<=DESC90_RATEMCS15)
		 if(pDesc->RxHT && pDesc->RxMCS>=DESC92S_RATEMCS8 &&
		 	pDesc->RxMCS<=DESC92S_RATEMCS15)
			max_spatial_stream = 2; //both spatial stream make sense
		else
			max_spatial_stream = 1; //only spatial stream 1 makes sense

		for(i=0; i<max_spatial_stream; i++)
		{
			//tmp_rxevm =	pofdm_buf->rxevm_X[i];
			//rx_evmX = (char)(tmp_rxevm);
			
			// Do not use shift operation like "rx_evmX >>= 1" because the compilor of free build environment
			// fill most significant bit to "zero" when doing shifting operation which may change a negative 
			// value to positive one, then the dbm value (which is supposed to be negative)  is not correct anymore.
			//rx_evmX /= 2;	//dbm

			//evm = rtl819x_evm_dbtopercentage(rx_evmX);
			evm = rtl819x_evm_dbtopercentage( (pdrvinfo->rxevm[i] /*/ 2*/));	//dbm
			RT_TRACE(COMP_RF, "RXRATE=%x RXEVM=%x EVM=%s%d\n", pDesc->RxMCS, pdrvinfo->rxevm[i], "%", evm);
#if 0			
			EVM = SignalScaleMapping(EVM);//make it good looking, from 0~100//=====>from here
#endif

			//if(bpacket_match_bssid)
			{
				if(i==0) // Fill value in RFD, Get the first spatial stream only
					pstats->SignalQuality = precord_stats->SignalQuality = (u8)(evm & 0xff);
				pstats->RxMIMOSignalQuality[i] = precord_stats->RxMIMOSignalQuality[i] = (u8)(evm & 0xff);
			}
		}

		
		// record rx statistics for debug */
		//rxsc_sgien_exflg = pofdm_buf->rxsc_sgien_exflg;
		prxsc =	(phy_ofdm_rx_status_rxsc_sgien_exintfflag *)&rxsc_sgien_exflg;
		//if(pdrvinfo->BW)	//40M channel
		if(pDesc->BW)	//40M channel
			priv->stats.received_bwtype[1+pdrvinfo->rxsc]++;
		else				//20M channel
			priv->stats.received_bwtype[0]++;
	}

	//UI BSS List signal strength(in percentage), make it good looking, from 0~100.
	//It is assigned to the BSS List in GetValueFromBeaconOrProbeRsp().
	if(is_cck_rate)
	{
		pstats->SignalStrength = precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping((long)pwdb_all));//PWDB_ALL;//check ok
		
	}
	else
	{
		//pRfd->Status.SignalStrength = pRecordRfd->Status.SignalStrength = (u8)(SignalScaleMapping(total_rssi/=RF90_PATH_MAX));//(u8)(total_rssi/=RF90_PATH_MAX);
		// We can judge RX path number now.
		if (rf_rx_num != 0)
			pstats->SignalStrength = precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping((long)(total_rssi/=rf_rx_num)));		
	}
}/* QueryRxPhyStatus8192S */
#endif
#ifdef RTL8192U
static void rtl8192_query_rxphystatus(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pstats,
	rx_drvinfo_819x_usb  * pdrvinfo,
	struct ieee80211_rx_stats * precord_stats,
	bool bpacket_match_bssid,
	bool bpacket_toself,
	bool bPacketBeacon,
	bool bToSelfBA
	)
{	
	//PRT_RFD_STATUS		pRtRfdStatus = &(pRfd->Status);	
	phy_sts_ofdm_819xusb_t*	pofdm_buf;
	phy_sts_cck_819xusb_t	*	pcck_buf;
	phy_ofdm_rx_status_rxsc_sgien_exintfflag* prxsc;
	u8				*prxpkt;
	u8				i, max_spatial_stream, tmp_rxsnr, tmp_rxevm, rxsc_sgien_exflg;
	char				rx_pwr[4], rx_pwr_all=0;
	//long				rx_avg_pwr = 0;
	char				rx_snrX, rx_evmX;
	u8				evm, pwdb_all;
	u32				RSSI, total_rssi=0;//, total_evm=0;
//	long				signal_strength_index = 0;
	u8				is_cck_rate=0;
	u8				rf_rx_num = 0;
	

	priv->stats.numqry_phystatus++;
	
	is_cck_rate = rx_hal_is_cck_rate(pdrvinfo);

	// Record it for next packet processing
	memset(precord_stats, 0, sizeof(struct ieee80211_rx_stats));
	pstats->bPacketMatchBSSID = precord_stats->bPacketMatchBSSID = bpacket_match_bssid;
	pstats->bPacketToSelf = precord_stats->bPacketToSelf = bpacket_toself;
	pstats->bIsCCK = precord_stats->bIsCCK = is_cck_rate;//RX_HAL_IS_CCK_RATE(pDrvInfo);
	pstats->bPacketBeacon = precord_stats->bPacketBeacon = bPacketBeacon;
	pstats->bToSelfBA = precord_stats->bToSelfBA = bToSelfBA;
	
	prxpkt = (u8*)pdrvinfo;	
	
	// Move pointer to the 16th bytes. Phy status start address. */	
	prxpkt += sizeof(rx_drvinfo_819x_usb);	
	
	// Initial the cck and ofdm buffer pointer */
	pcck_buf = (phy_sts_cck_819xusb_t *)prxpkt;
	pofdm_buf = (phy_sts_ofdm_819xusb_t *)prxpkt;			
			
	pstats->RxMIMOSignalQuality[0] = -1;
	pstats->RxMIMOSignalQuality[1] = -1;
	precord_stats->RxMIMOSignalQuality[0] = -1;
	precord_stats->RxMIMOSignalQuality[1] = -1;
			
	if(is_cck_rate)
	{
		// 
		// (1)Hardware does not provide RSSI for CCK
		//

		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		u8 report;//, cck_agc_rpt; 

		priv->stats.numqry_phystatusCCK++;

		if(!priv->bCckHighPower)
		{
			report = pcck_buf->cck_agc_rpt & 0xc0;
			report = report>>6;
			switch(report)
			{
				//Fixed by Jacken from Bryant 2008-03-20
				//Original value is -38 , -26 , -14 , -2
				//Fixed value is -35 , -23 , -11 , 6
				case 0x3:
					rx_pwr_all = -35 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x2:
					rx_pwr_all = -23 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x1:
					rx_pwr_all = -11 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x0:
					rx_pwr_all = 6 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
			}
		}
		else
		{
			report = pcck_buf->cck_agc_rpt & 0x60;
			report = report>>5;
			switch(report)
			{
				case 0x3:
					rx_pwr_all = -35 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x2:
					rx_pwr_all = -23 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x1:
					rx_pwr_all = -11 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x0:
					rx_pwr_all = 6 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
			}
		}

		pwdb_all = rtl819x_query_rxpwrpercentage(rx_pwr_all);
		pstats->RxPWDBAll = precord_stats->RxPWDBAll = pwdb_all;
		pstats->RecvSignalPower = pwdb_all;

		//
		// (3) Get Signal Quality (EVM)
		//
		//if(bpacket_match_bssid)
		{
			u8	sq;

			if(pstats->RxPWDBAll > 40)
			{
				sq = 100;
			}else
			{
				sq = pcck_buf->sq_rpt;
				
				if(pcck_buf->sq_rpt > 64)
					sq = 0;
				else if (pcck_buf->sq_rpt < 20)
					sq = 100;
				else
					sq = ((64-sq) * 100) / 44;
			}
			pstats->SignalQuality = precord_stats->SignalQuality = sq;
			pstats->RxMIMOSignalQuality[0] = precord_stats->RxMIMOSignalQuality[0] = sq;
			pstats->RxMIMOSignalQuality[1] = precord_stats->RxMIMOSignalQuality[1] = -1;
		}
	}
	else
	{
		priv->stats.numqry_phystatusHT++;
		// 
		// (1)Get RSSI for HT rate
		//
		for(i=RF90_PATH_A; i<priv->NumTotalRFPath; i++)
		{
			// 2008/01/30 MH we will judge RF RX path now.
			if (priv->brfpath_rxenable[i])
				rf_rx_num++;
			else
				continue;

		if (!rtl8192_phy_CheckIsLegalRFPath(priv->ieee80211->dev, i))		
				continue;

			//Fixed by Jacken from Bryant 2008-03-20
			//Original value is 106
			rx_pwr[i] = ((pofdm_buf->trsw_gain_X[i]&0x3F)*2) - 106;

			//Get Rx snr value in DB
			tmp_rxsnr =	pofdm_buf->rxsnr_X[i];
			rx_snrX = (char)(tmp_rxsnr);
			//rx_snrX >>= 1;;
			rx_snrX /= 2;
			priv->stats.rxSNRdB[i] = (long)rx_snrX;
			
			// Translate DBM to percentage. */
			RSSI = rtl819x_query_rxpwrpercentage(rx_pwr[i]);	
			total_rssi += RSSI;

			// Record Signal Strength for next packet */
			//if(bpacket_match_bssid)
			{
				pstats->RxMIMOSignalStrength[i] =(u8) RSSI;
				precord_stats->RxMIMOSignalStrength[i] =(u8) RSSI;
			}
		}
		
		
		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		//Fixed by Jacken from Bryant 2008-03-20
		//Original value is 106
		rx_pwr_all = (((pofdm_buf->pwdb_all ) >> 1 )& 0x7f) -106;
		pwdb_all = rtl819x_query_rxpwrpercentage(rx_pwr_all);	

		pstats->RxPWDBAll = precord_stats->RxPWDBAll = pwdb_all;
		pstats->RxPower = precord_stats->RxPower =  rx_pwr_all;
		
		//
		// (3)EVM of HT rate
		//
		if(pdrvinfo->RxHT && pdrvinfo->RxRate>=DESC90_RATEMCS8 &&
		 	pdrvinfo->RxRate<=DESC90_RATEMCS15)
			max_spatial_stream = 2; //both spatial stream make sense
		else
			max_spatial_stream = 1; //only spatial stream 1 makes sense

		for(i=0; i<max_spatial_stream; i++)
		{
			tmp_rxevm =	pofdm_buf->rxevm_X[i];
			rx_evmX = (char)(tmp_rxevm);
			
			// Do not use shift operation like "rx_evmX >>= 1" because the compilor of free build environment
			// fill most significant bit to "zero" when doing shifting operation which may change a negative 
			// value to positive one, then the dbm value (which is supposed to be negative)  is not correct anymore.
			rx_evmX /= 2;	//dbm

			evm = rtl819x_evm_dbtopercentage(rx_evmX);
#if 0			
			EVM = SignalScaleMapping(EVM);//make it good looking, from 0~100
#endif
			//if(bpacket_match_bssid)
			{
				if(i==0) // Fill value in RFD, Get the first spatial stream only
					pstats->SignalQuality = precord_stats->SignalQuality = (u8)(evm & 0xff);
				pstats->RxMIMOSignalQuality[i] = precord_stats->RxMIMOSignalQuality[i] = (u8)(evm & 0xff);
			}
		}

		
		// record rx statistics for debug */
		rxsc_sgien_exflg = pofdm_buf->rxsc_sgien_exflg;
		prxsc =	(phy_ofdm_rx_status_rxsc_sgien_exintfflag *)&rxsc_sgien_exflg;
		if(pdrvinfo->BW)	//40M channel
			priv->stats.received_bwtype[1+prxsc->rxsc]++;
		else				//20M channel
			priv->stats.received_bwtype[0]++;
	}

	//UI BSS List signal strength(in percentage), make it good looking, from 0~100.
	//It is assigned to the BSS List in GetValueFromBeaconOrProbeRsp().
	if(is_cck_rate)
	{
		pstats->SignalStrength = precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping((long)pwdb_all));//PWDB_ALL;
		
	}
	else
	{
		//pRfd->Status.SignalStrength = pRecordRfd->Status.SignalStrength = (u8)(SignalScaleMapping(total_rssi/=RF90_PATH_MAX));//(u8)(total_rssi/=RF90_PATH_MAX);
		// We can judge RX path number now.
		if (rf_rx_num != 0)
			pstats->SignalStrength = precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping((long)(total_rssi/=rf_rx_num)));		
	}
}	/* QueryRxPhyStatus8190Pci */
#endif

void
rtl8192_record_rxdesc_forlateruse(
	struct ieee80211_rx_stats *	psrc_stats,
	struct ieee80211_rx_stats *	ptarget_stats
)
{
	ptarget_stats->bIsAMPDU = psrc_stats->bIsAMPDU;
	ptarget_stats->bFirstMPDU = psrc_stats->bFirstMPDU;
	ptarget_stats->Seq_Num = psrc_stats->Seq_Num;
}

#ifdef RTL8192SU
static void rtl8192SU_query_rxphystatus(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pstats,
	rx_desc_819x_usb	*pDesc,
	rx_drvinfo_819x_usb  * pdrvinfo,
	struct ieee80211_rx_stats * precord_stats,
	bool bpacket_match_bssid,
	bool bpacket_toself,
	bool bPacketBeacon,
	bool bToSelfBA
	);
void rtl8192SU_TranslateRxSignalStuff(struct sk_buff *skb, 
				   struct ieee80211_rx_stats * pstats,
				   rx_desc_819x_usb	*pDesc,
                                   rx_drvinfo_819x_usb  *pdrvinfo)
{
	// TODO: We must only check packet for current MAC address. Not finish
	rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev=info->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	bool bpacket_match_bssid, bpacket_toself;
	bool bPacketBeacon=FALSE, bToSelfBA=FALSE;
	static struct ieee80211_rx_stats  previous_stats;
	struct ieee80211_hdr_3addr *hdr;//by amy
       u16 fc,type;

	// Get Signal Quality for only RX data queue (but not command queue)

	u8* tmp_buf;
	//u16 tmp_buf_len = 0;
	u8  *praddr;
	
	// Get MAC frame start address. */		
	tmp_buf = (u8*)skb->data;// + get_rxpacket_shiftbytes_819xusb(pstats);

	hdr = (struct ieee80211_hdr_3addr *)tmp_buf;
	fc = le16_to_cpu(hdr->frame_ctl);
	type = WLAN_FC_GET_TYPE(fc);	
	praddr = hdr->addr1;

	// Check if the received packet is acceptabe. */
	bpacket_match_bssid = ((IEEE80211_FTYPE_CTL != type) &&
                                			(eqMacAddr(priv->ieee80211->current_network.bssid,  (fc & IEEE80211_FCTL_TODS)? hdr->addr1 : (fc & IEEE80211_FCTL_FROMDS )? hdr->addr2 : hdr->addr3))
                                				 && (!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV));
	bpacket_toself =  bpacket_match_bssid & (eqMacAddr(praddr, priv->ieee80211->dev->dev_addr));

#if 1//cosa
		if(WLAN_FC_GET_FRAMETYPE(fc)== IEEE80211_STYPE_BEACON)
		{
			bPacketBeacon = true;
			//DbgPrint("Beacon 2, MatchBSSID = %d, ToSelf = %d \n", bPacketMatchBSSID, bPacketToSelf);
		}
		if(WLAN_FC_GET_FRAMETYPE(fc) == IEEE80211_STYPE_BLOCKACK)
		{
			if((eqMacAddr(praddr,dev->dev_addr)))
				bToSelfBA = true;
				//DbgPrint("BlockAck, MatchBSSID = %d, ToSelf = %d \n", bPacketMatchBSSID, bPacketToSelf);
		}

#endif	

				 
	if(bpacket_match_bssid)
	{
		priv->stats.numpacket_matchbssid++;
	}
	if(bpacket_toself){
		priv->stats.numpacket_toself++;
	}
	//
	// Process PHY information for previous packet (RSSI/PWDB/EVM)
	//
	// Because phy information is contained in the last packet of AMPDU only, so driver 
	// should process phy information of previous packet	
	rtl8192_process_phyinfo(priv, tmp_buf, &previous_stats, pstats);
	rtl8192SU_query_rxphystatus(priv, pstats, pDesc, pdrvinfo, &previous_stats, bpacket_match_bssid,bpacket_toself,bPacketBeacon,bToSelfBA);
	rtl8192_record_rxdesc_forlateruse(pstats, &previous_stats);

}
#endif
#ifdef RTL8192U
void TranslateRxSignalStuff819xUsb(struct sk_buff *skb, 
				   struct ieee80211_rx_stats * pstats,
                                   rx_drvinfo_819x_usb  *pdrvinfo)
{
	// TODO: We must only check packet for current MAC address. Not finish
	rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev=info->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	bool bpacket_match_bssid, bpacket_toself;
	bool bPacketBeacon=FALSE, bToSelfBA=FALSE;
	static struct ieee80211_rx_stats  previous_stats;
	struct ieee80211_hdr_3addr *hdr;//by amy
       u16 fc,type;
		
	// Get Signal Quality for only RX data queue (but not command queue)
		
	u8* tmp_buf;
	//u16 tmp_buf_len = 0;
	u8  *praddr;
	
	// Get MAC frame start address. */		
	tmp_buf = (u8*)skb->data;// + get_rxpacket_shiftbytes_819xusb(pstats);

	hdr = (struct ieee80211_hdr_3addr *)tmp_buf;
	fc = le16_to_cpu(hdr->frame_ctl);
	type = WLAN_FC_GET_TYPE(fc);	
	praddr = hdr->addr1;

	// Check if the received packet is acceptabe. */
	bpacket_match_bssid = ((IEEE80211_FTYPE_CTL != type) &&
                                			(eqMacAddr(priv->ieee80211->current_network.bssid,  (fc & IEEE80211_FCTL_TODS)? hdr->addr1 : (fc & IEEE80211_FCTL_FROMDS )? hdr->addr2 : hdr->addr3))
                                				 && (!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV));
	bpacket_toself =  bpacket_match_bssid & (eqMacAddr(praddr, priv->ieee80211->dev->dev_addr));

#if 1//cosa
	if(WLAN_FC_GET_FRAMETYPE(fc)== IEEE80211_STYPE_BEACON)
	{
		bPacketBeacon = true;
		//printk("Beacon 2, MatchBSSID = %d, ToSelf = %d \n", bPacketMatchBSSID, bPacketToSelf);
	}
	if(WLAN_FC_GET_FRAMETYPE(fc) == IEEE80211_STYPE_BLOCKACK)
	{
		if((eqMacAddr(praddr,dev->dev_addr)))
			bToSelfBA = true;
		//printk("BlockAck, MatchBSSID = %d, ToSelf = %d \n", bPacketMatchBSSID, bPacketToSelf);
	}
#endif	

	if(bpacket_match_bssid)
	{
		priv->stats.numpacket_matchbssid++;
	}
	if(bpacket_toself){
		priv->stats.numpacket_toself++;
	}
	//
	// Process PHY information for previous packet (RSSI/PWDB/EVM)
	//
	// Because phy information is contained in the last packet of AMPDU only, so driver 
	// should process phy information of previous packet	
	rtl8192_process_phyinfo(priv, tmp_buf, &previous_stats, pstats);
	rtl8192_query_rxphystatus(priv, pstats, pdrvinfo, &previous_stats, bpacket_match_bssid,bpacket_toself,bPacketBeacon,bToSelfBA);
	rtl8192_record_rxdesc_forlateruse(pstats, &previous_stats);
		
}
#endif

//
//Function:	UpdateReceivedRateHistogramStatistics
//Overview:	Recored down the received data rate
//
//Input:		
//	struct net_device *dev
//      struct ieee80211_rx_stats *stats
//			
//Output:		
//      
//      		(priv->stats.ReceivedRateHistogram[] is updated)
//Return:     	
//      	None
//
void
UpdateReceivedRateHistogramStatistics8190(
	struct net_device *dev,
	struct ieee80211_rx_stats *stats
	)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
    	u32 rcvType=1;   //0: Total, 1:OK, 2:CRC, 3:ICV
    	u32 rateIndex;
    	u32 preamble_guardinterval;  //1: short preamble/GI, 0: long preamble/GI
    
				
    	if(stats->bCRC)
       	rcvType = 2;
   	else if(stats->bICV)
       	rcvType = 3;
    
   	if(stats->bShortPreamble)
       	preamble_guardinterval = 1;// short
    	else
       	preamble_guardinterval = 0;// long

	switch(stats->rate)
	{
		//
		// CCK rate
		//
		case MGN_1M:    rateIndex = 0;  break;
		case MGN_2M:    rateIndex = 1;  break;
		case MGN_5_5M:  rateIndex = 2;  break;
		case MGN_11M:   rateIndex = 3;  break;
		//
		// Legacy OFDM rate
		//
		case MGN_6M:    rateIndex = 4;  break;
		case MGN_9M:    rateIndex = 5;  break;
		case MGN_12M:   rateIndex = 6;  break;
		case MGN_18M:   rateIndex = 7;  break;
		case MGN_24M:   rateIndex = 8;  break;
		case MGN_36M:   rateIndex = 9;  break;
		case MGN_48M:   rateIndex = 10; break;
		case MGN_54M:   rateIndex = 11; break;
		//
		// 11n High throughput rate
		//
		case MGN_MCS0:  rateIndex = 12; break;
		case MGN_MCS1:  rateIndex = 13; break;
		case MGN_MCS2:  rateIndex = 14; break;
		case MGN_MCS3:  rateIndex = 15; break;
		case MGN_MCS4:  rateIndex = 16; break;
		case MGN_MCS5:  rateIndex = 17; break;
		case MGN_MCS6:  rateIndex = 18; break;
		case MGN_MCS7:  rateIndex = 19; break;
		case MGN_MCS8:  rateIndex = 20; break;
		case MGN_MCS9:  rateIndex = 21; break;
		case MGN_MCS10: rateIndex = 22; break;
		case MGN_MCS11: rateIndex = 23; break;
		case MGN_MCS12: rateIndex = 24; break;
		case MGN_MCS13: rateIndex = 25; break;
		case MGN_MCS14: rateIndex = 26; break;
		case MGN_MCS15: rateIndex = 27; break;
		default:        rateIndex = 28; break;
	}
    priv->stats.received_preamble_GI[preamble_guardinterval][rateIndex]++;
    priv->stats.received_rate_histogram[0][rateIndex]++; //total
    priv->stats.received_rate_histogram[rcvType][rateIndex]++;
}

#ifdef RTL8192SU
void rtl8192SU_query_rxdesc_status(struct sk_buff *skb, struct ieee80211_rx_stats *stats, bool bIsRxAggrSubframe)
{
	rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev=info->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	rx_drvinfo_819x_usb  *driver_info = NULL;
	rx_desc_819x_usb *desc = (rx_desc_819x_usb *)skb->data;

	if(0)
	{
		int m = 0;
		printk("========================");
		for(m=0; m<skb->len; m++){
			if((m%32) == 0)
				printk("\n");
			printk("%2x ",((u8*)skb->data)[m]);	
		}
		printk("\n========================\n");

	}


	//
	//Get Rx Descriptor Raw Information
	//	
	stats->Length = desc->Length ;
	stats->RxDrvInfoSize = desc->RxDrvInfoSize*RX_DRV_INFO_SIZE_UNIT;
	stats->RxBufShift = (desc->Shift)&0x03;     
	stats->bICV = desc->ICV;
	stats->bCRC = desc->CRC32;
	stats->bHwError = stats->bCRC|stats->bICV;
	stats->Decrypted = !desc->SWDec;//RTL8190 set this bit to indicate that Hw does not decrypt packet
	stats->bIsAMPDU = (desc->AMSDU==1);
	stats->bFirstMPDU = (desc->PAGGR==1) && (desc->FAGGR==1);
	stats->bShortPreamble = desc->SPLCP;
	stats->RxIs40MHzPacket = (desc->BW==1);	
	stats->TimeStampLow = desc->TSFL;	

	if((desc->FAGGR==1) || (desc->PAGGR==1))
	{// Rx A-MPDU
		RT_TRACE(COMP_RXDESC, "FirstAGGR = %d, PartAggr = %d\n", desc->FAGGR, desc->PAGGR);
	}	

#ifdef TCP_CSUM_OFFLOAD_RX
	//printk(" (%d, %d) \n", desc->TCPChkValID, desc->TCPChkRpt);
	if (desc->TCPChkValID && desc->TCPChkRpt) {
		stats->tcp_csum_valid = 1;
	} else
		stats->tcp_csum_valid = 0;
#endif

//YJ,test,090310
//if(stats->bHwError)
//{
//	if(stats->bICV)
//		printk("%s: Receive ICV error!!!!!!!!!!!!!!!!!!!!!!\n", __FUNCTION__);
//	if(stats->bCRC)
//		printk("%s: Receive CRC error!!!!!!!!!!!!!!!!!!!!!!\n", __FUNCTION__);
//}

	if(IS_UNDER_11N_AES_MODE(priv->ieee80211))
	{
		// Always received ICV error packets in AES mode.
		// This fixed HW later MIC write bug.
		if(stats->bICV && !stats->bCRC)
		{
			stats->bICV = FALSE;
			stats->bHwError = FALSE;
		}
	}

	// Transform HwRate to MRate
	if(!stats->bHwError)
		//stats->DataRate = HwRateToMRate(
		//	(BOOLEAN)GET_RX_DESC_RXHT(pDesc), 
		//	(u1Byte)GET_RX_DESC_RXMCS(pDesc), 
		//	(BOOLEAN)GET_RX_DESC_PAGGR(pDesc));
		stats->rate = rtl8192SU_HwRateToMRate(desc->RxHT, desc->RxMCS, desc->PAGGR);
	else
		stats->rate = MGN_1M;	

	//
	// Collect Rx rate/AMPDU/TSFL
	//
	//UpdateRxdRateHistogramStatistics8192S(Adapter, pRfd);
	//UpdateRxAMPDUHistogramStatistics8192S(Adapter, pRfd);	
	//UpdateRxPktTimeStamp8192S(Adapter, pRfd);
	UpdateReceivedRateHistogramStatistics8190(dev, stats);
	//UpdateRxAMPDUHistogramStatistics8192S(dev, stats);	//FIXLZM	
	UpdateRxPktTimeStamp8190(dev, stats);

	//
	// Get PHY Status and RSVD parts.
	// <Roger_Notes> It only appears on last aggregated packet. 
	//
	if (desc->PHYStatus)
	{
		//driver_info = (rx_drvinfo_819x_usb *)(skb->data + RX_DESC_SIZE + stats->RxBufShift);
		driver_info = (rx_drvinfo_819x_usb *)(skb->data + sizeof(rx_desc_819x_usb) + \
				stats->RxBufShift);
		if(0)
		{
			int m = 0;
			printk("========================\n");
			printk("RX_DESC_SIZE:%d, RxBufShift:%d, RxDrvInfoSize:%d\n", 
					RX_DESC_SIZE, stats->RxBufShift, stats->RxDrvInfoSize);
			for(m=0; m<32; m++){
			       printk("%2x ",((u8*)driver_info)[m]);	
			}
			printk("\n========================\n");
		
		}

	}

	//YJ,add,090107
	skb_pull(skb, sizeof(rx_desc_819x_usb));
	//YJ,add,090107,end

	//
	// Get Total offset of MPDU Frame Body 
	//
	if((stats->RxBufShift + stats->RxDrvInfoSize) > 0)
	{
		stats->bShift = 1;	
		//YJ,add,090107
		skb_pull(skb, stats->RxBufShift + stats->RxDrvInfoSize);
		//YJ,add,090107,end
	}

	//
	// Get PHY Status and RSVD parts.
	// <Roger_Notes> It only appears on last aggregated packet. 
	//
	if (desc->PHYStatus)
	{
		rtl8192SU_TranslateRxSignalStuff(skb, stats, desc, driver_info);		
	}
}
#endif
#ifdef RTL8192U
void query_rxdesc_status(struct sk_buff *skb, struct ieee80211_rx_stats *stats, bool bIsRxAggrSubframe)
{
	rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev=info->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	//rx_desc_819x_usb *desc = (rx_desc_819x_usb *)skb->data;
	rx_drvinfo_819x_usb  *driver_info = NULL;

	//
	//Get Rx Descriptor Information
	//      
#ifdef USB_RX_AGGREGATION_SUPPORT
	if (bIsRxAggrSubframe)
	{
		rx_desc_819x_usb_aggr_subframe *desc = (rx_desc_819x_usb_aggr_subframe *)skb->data;
		stats->Length = desc->Length ;
		stats->RxDrvInfoSize = desc->RxDrvInfoSize;
		stats->RxBufShift = 0; //RxBufShift = 2 in RxDesc, but usb didn't shift bytes in fact.
		stats->bICV = desc->ICV;
		stats->bCRC = desc->CRC32;
		stats->bHwError = stats->bCRC|stats->bICV;
		stats->Decrypted = !desc->SWDec;//RTL8190 set this bit to indicate that Hw does not decrypt packet
	} else
#endif
	{
		rx_desc_819x_usb *desc = (rx_desc_819x_usb *)skb->data;

		stats->Length = desc->Length;
		stats->RxDrvInfoSize = desc->RxDrvInfoSize;
		stats->RxBufShift = 0;//desc->Shift&0x03;
		stats->bICV = desc->ICV;
		stats->bCRC = desc->CRC32;
		stats->bHwError = stats->bCRC|stats->bICV;
		//RTL8190 set this bit to indicate that Hw does not decrypt packet
		stats->Decrypted = !desc->SWDec;
	}

	if((priv->ieee80211->pHTInfo->bCurrentHTSupport == true) && (priv->ieee80211->pairwise_key_type == KEY_TYPE_CCMP))
	{
		stats->bHwError = false;
	}
	else
	{
		stats->bHwError = stats->bCRC|stats->bICV;
	}

	if(stats->Length < 24 || stats->Length > MAX_8192U_RX_SIZE)
		stats->bHwError |= 1;
	//
	//Get Driver Info
	//
	// TODO: Need to verify it on FGPA platform
	//Driver info are written to the RxBuffer following rx desc
	if (stats->RxDrvInfoSize != 0) {
		driver_info = (rx_drvinfo_819x_usb *)(skb->data + sizeof(rx_desc_819x_usb) + \
				stats->RxBufShift);
		// unit: 0.5M */
		// TODO 
		if(!stats->bHwError){
			u8	ret_rate;
			ret_rate = HwRateToMRate90(driver_info->RxHT, driver_info->RxRate);
			if(ret_rate == 0xff)
			{
				// Abnormal Case: Receive CRC OK packet with Rx descriptor indicating non supported rate.
				// Special Error Handling here, 2008.05.16, by Emily
				
				stats->bHwError = 1;
				stats->rate = MGN_1M;	//Set 1M rate by default
			}else
			{
				stats->rate = ret_rate;
			}
		}
		else
			stats->rate = 0x02;

		stats->bShortPreamble = driver_info->SPLCP;

		
		UpdateReceivedRateHistogramStatistics8190(dev, stats);
		 
		stats->bIsAMPDU = (driver_info->PartAggr==1);
		stats->bFirstMPDU = (driver_info->PartAggr==1) && (driver_info->FirstAGGR==1);
#if 0
		// TODO: it is debug only. It should be disabled in released driver. 2007.1.12 by Joseph
		UpdateRxAMPDUHistogramStatistics8190(Adapter, pRfd);
#endif
		stats->TimeStampLow = driver_info->TSFL;
		// xiong mask it, 070514
		//pRfd->Status.TimeStampHigh = PlatformEFIORead4Byte(Adapter, TSFR+4);
		// stats->TimeStampHigh = read_nic_dword(dev,  TSFR+4);

		UpdateRxPktTimeStamp8190(dev, stats);

		//
		// Rx A-MPDU
		//
		if(driver_info->FirstAGGR==1 || driver_info->PartAggr == 1)
			RT_TRACE(COMP_RXDESC, "driver_info->FirstAGGR = %d, driver_info->PartAggr = %d\n",
					driver_info->FirstAGGR, driver_info->PartAggr);

	}

	skb_pull(skb,sizeof(rx_desc_819x_usb));
	//
	// Get Total offset of MPDU Frame Body 
	//
	if((stats->RxBufShift + stats->RxDrvInfoSize) > 0) {
		stats->bShift = 1;
		skb_pull(skb,stats->RxBufShift + stats->RxDrvInfoSize);
	}

#ifdef USB_RX_AGGREGATION_SUPPORT
	// for the rx aggregated sub frame, the redundant space truelly contained in the packet */
	if(bIsRxAggrSubframe) {
		skb_pull(skb, 8);
	}
#endif
	// for debug 2008.5.29 */
#if 0
	{
		int i;
		printk("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		for(i = 0; i < skb->len; i++) {
			if(i % 10 == 0) printk("\n");
			printk("%02x ", skb->data[i]);
		} 
		printk("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	}
#endif

	//added by vivi, for MP, 20080108
	stats->RxIs40MHzPacket = driver_info->BW;
	if(stats->RxDrvInfoSize != 0)
		TranslateRxSignalStuff819xUsb(skb, stats, driver_info);

}
#endif

#ifdef RTL8192SU
#if 0
//-----------------------------------------------------------------------------
// Function:	UpdateRxAMPDUHistogramStatistics8192S
//
// Overview:	Recored down the received A-MPDU aggregation size and pkt number
//
// Input:       Adapter
//
// Output:      Adapter
//				(Adapter->RxStats.RxAMPDUSizeHistogram[] is updated)
//				(Adapter->RxStats.RxAMPDUNumHistogram[] is updated)
//
// Return:      NONE
//
// Revised History:
// When			Who		Remark
// 09/18/2008 	MHC		Create Version 0.
//
//---------------------------------------------------------------------------*/
static	void
UpdateRxAMPDUHistogramStatistics8192S(
	struct net_device *dev,
	struct ieee80211_rx_stats *stats
	)
{
	//HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(Adapter);
	u8 	size_index;
	u8	num_index;
	u16	update_size = 0;
	u8	update_num = 0;
		
	if(stats->bIsAMPDU)
	{
		if(stats->bFirstMPDU)
		{
			if(stats->nRxAMPDU_Size!=0 && stats->nRxAMPDU_AggrNum!=0)
			{
				update_size = stats->nRxAMPDU_Size;
				update_num = stats->nRxAMPDU_AggrNum;
			}
			stats->nRxAMPDU_Size = stats->Length;
			stats->nRxAMPDU_AggrNum = 1;
		}
		else
		{
			stats->nRxAMPDU_Size += stats->Length;
			stats->nRxAMPDU_AggrNum++;
		}
	}
	else
	{
		if(stats->nRxAMPDU_Size!=0 && stats->nRxAMPDU_AggrNum!=0)
		{
			update_size = stats->nRxAMPDU_Size;
			update_num = stats->nRxAMPDU_AggrNum;
		}
		stats->nRxAMPDU_Size = 0;
		stats->nRxAMPDU_AggrNum = 0;		
	}

	if(update_size!=0 && update_num!= 0)
	{
		if(update_size < 4096)
			size_index = 0;
		else if(update_size < 8192)
			size_index = 1;
		else if(update_size < 16384)
			size_index = 2;
		else if(update_size < 32768)
			size_index = 3;
		else if(update_size < 65536)
			size_index = 4;
		else
		{
			RT_TRACE(COMP_RXDESC,  
			("UpdateRxAMPDUHistogramStatistics8192S(): A-MPDU too large\n");
		}

		Adapter->RxStats.RxAMPDUSizeHistogram[size_index]++;

		if(update_num < 5)
			num_index = 0;
		else if(update_num < 10)
			num_index = 1;
		else if(update_num < 20)
			num_index = 2;
		else if(update_num < 40)
			num_index = 3;
		else
			num_index = 4;

		Adapter->RxStats.RxAMPDUNumHistogram[num_index]++;
	}
}	// UpdateRxAMPDUHistogramStatistics8192S
#endif

//
// Description:
// 	The strarting address of wireless lan header will shift 1 or 2 or 3 or "more" bytes for the following reason :
// 	(1) QoS control : shift 2 bytes
// 	(2) Mesh Network : shift 1 or 3 bytes
// 	(3) RxDriverInfo occupies  the front parts of Rx Packets buffer(shift units is in 8Bytes)
//
//  	It is because Lextra CPU used by 8186 or 865x series assert exception if the statrting address
//	of IP header is not double word alignment.
//	This features is supported in 818xb and 8190 only, but not 818x.
// 
//	parameter: PRT_RFD, Pointer of Reeceive frame descriptor which is initialized according to 
//					     Rx Descriptor						   
//	return value: unsigned int,  number of total shifted bytes
// 
//	Notes: 2008/06/28, created by Roger
//
u32 GetRxPacketShiftBytes8192SU(struct ieee80211_rx_stats  *Status, bool bIsRxAggrSubframe)
{
	return (sizeof(rx_desc_819x_usb) + Status->RxDrvInfoSize + Status->RxBufShift);
}

#if 1
void rtl8192SU_rx(struct sk_buff* skb)
{
	rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev=info->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
		//      .mac_time = jiffies,
		.freq = IEEE80211_24GHZ_BAND,
	};
	u32 rx_pkt_len = 0;
	struct ieee80211_hdr_1addr *ieee80211_hdr = NULL;
	bool unicast_packet = false;


	// 20 is for ps-poll */
	if((skb->len >=(20 + sizeof(rx_desc_819x_usb))) && (skb->len < RX_URB_SIZE)) {

		// first packet should not contain Rx aggregation header */
		rtl8192SU_query_rxdesc_status(skb, &stats, false);
		// TODO */

		if(stats.bHwError) {
			dev_kfree_skb_any(skb);
			return;
		}
		
		priv->stats.rxoktotal++;  //YJ,test,090108

		// Process the MPDU recevied */
		skb_trim(skb, skb->len - 4/*sCrcLng*/);//FIXLZM
		
		rx_pkt_len = skb->len;
		ieee80211_hdr = (struct ieee80211_hdr_1addr *)skb->data;
		unicast_packet = false;
		if(is_broadcast_ether_addr(ieee80211_hdr->addr1)) {
			//TODO
		}else if(is_multicast_ether_addr(ieee80211_hdr->addr1)){
			//TODO
		}else {
			// unicast packet */
			unicast_packet = true;
		}

		{
			bool bLedBlinking=TRUE;
    		 	u16 fc=0, type=0;
			fc = le16_to_cpu(ieee80211_hdr->frame_ctl);
			type = WLAN_FC_GET_TYPE(fc);
			if(type == IEEE80211_FTYPE_MGMT)
			{
		  		bLedBlinking = false;
			}
			
                	if(bLedBlinking)
                   	 	if(priv->ieee80211->LedControlHandler != NULL)
                        		priv->ieee80211->LedControlHandler(dev, LED_CTL_RX);
		}

		if(!stats.Decrypted) {
			skb->cb[0] = 1;
		} else {
			skb->cb[0] = 0;
		}

		if(!ieee80211_rx(priv->ieee80211,skb, &stats)) {
			dev_kfree_skb_any(skb);
		} else {
			//priv->stats.rxoktotal++;  //YJ,test,090108
			if(unicast_packet) {
				priv->stats.rxbytesunicast += rx_pkt_len;
			}
		}

	} 
	else 
	{	
		priv->stats.rxurberr++;
		printk("actual_length:%d\n", skb->len);
		dev_kfree_skb_any(skb);
	}


}

void rtl8192SU_rx_nomal(struct sk_buff* skb)
{

#ifdef USB_RX_AGGREGATION_SUPPORT

	int rxdesc_sz, total_len, pkt_len;
	u16 pkt_cnt, drvinfo_sz, shift_sz, pkt_offset, tmp_len;
	u8 *pbuf;
	struct sk_buff *pskb_clone = NULL;
	rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev = info->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	rx_desc_819x_usb *desc;

	rxdesc_sz = sizeof(rx_desc_819x_usb);
	total_len = skb->len;

	pbuf = (u8*)skb->data;
	desc = (rx_desc_819x_usb *)pbuf;
	
	pkt_cnt = desc->UsbAggPktNum;
	pkt_len = desc->Length;
	
	if((total_len < (20 + rxdesc_sz)) || (total_len > RX_URB_SIZE))
	{
		priv->stats.rxurberr++;
		
		printk("actual_length:%d\n", total_len);
		
		goto exit_rtl8192SU_rx_nomal;
	}

	if((pkt_cnt<1) || (pkt_len<=0))
	{
		priv->stats.rxurberr++;
		
		goto exit_rtl8192SU_rx_nomal;
	}

	if(pkt_cnt>1){
		do{
			desc = (rx_desc_819x_usb *)pbuf;
			pkt_len = desc->Length;
			
			drvinfo_sz = desc->RxDrvInfoSize;
			drvinfo_sz = drvinfo_sz<<3;//*8

			//shift_sz = (desc->Shift)&0x03;
			shift_sz = 0;

			//printk("rtl8192SU_rx_nomal(): pktLen = %d, drvinfo_sz = %d, desc_sz = %d, shift_sz=%d\n",pkt_len,drvinfo_sz,rxdesc_sz, shift_sz);
			
			if(pkt_len<=0)				
				break;
			tmp_len = (pkt_len + drvinfo_sz + rxdesc_sz + shift_sz);
#if 0	
			pskb_clone = skb_clone(skb, GFP_ATOMIC);
			if(!pskb_clone)
				break;
			
			pskb_clone->len = 0;
			pskb_clone->data = pskb_clone->tail = pbuf;
			
			skb_put(pskb_clone, tmp_len);
#else
			pskb_clone = dev_alloc_skb(tmp_len);
			if(pskb_clone==NULL){
				printk("rlt8192SU_rx_nomal:can not allocate memory\n");
				goto exit_rtl8192SU_rx_nomal;	
			}
			pskb_clone->dev = skb->dev;
			memcpy(pskb_clone->cb, skb->cb,sizeof(struct rtl8192_rx_info));
			memcpy(pskb_clone->data,pbuf,tmp_len);
			skb_put(pskb_clone,tmp_len);
#endif		

			pkt_offset = (((tmp_len >> 7) + ((tmp_len & 127) ? 1: 0)) << 7);//default rx_page_sz=128
			           
			rtl8192SU_rx(pskb_clone);
			total_len -= pkt_offset;
			pbuf += pkt_offset;
			pkt_cnt--;		

		}while((total_len>0) && (pkt_cnt>0));

exit_rtl8192SU_rx_nomal:

		dev_kfree_skb_any(skb);
	}
	else{
		desc = (rx_desc_819x_usb *)pbuf;
		pkt_len = desc->Length;
			
		drvinfo_sz = desc->RxDrvInfoSize;
		drvinfo_sz = drvinfo_sz<<3;//*8

		//shift_sz = (desc->Shift)&0x03;
		shift_sz = 0;
		tmp_len = (pkt_len + drvinfo_sz + rxdesc_sz + shift_sz);
		skb_trim(skb, tmp_len);
		rtl8192SU_rx(skb);
	}

#else

	rtl8192SU_rx(skb);

#endif

}
#else
void rtl8192SU_rx_nomal(struct sk_buff* skb)
{
	rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev=info->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
		//      .mac_time = jiffies,
		.freq = IEEE80211_24GHZ_BAND,
	};
	u32 rx_pkt_len = 0;
	struct ieee80211_hdr_1addr *ieee80211_hdr = NULL;
	bool unicast_packet = false;

#ifdef USB_RX_AGGREGATION_SUPPORT
	u8 *pbuf;
	int *prxdesc0;
	struct sk_buff *agg_skb = NULL;
	u32  TotalLength = 0;//Total packet length for all aggregated packets.
	u32  PacketLength = 0;// Per-packet length include size of RxDesc , DrvInfoSize and shift
	u32	PacketOffset = 0;
	u8	TotalAggPkt = 0;

	TotalLength = skb->len;
	pbuf = (u8*)skb->data;
	prxdesc0 = (int*)pbuf;
	TotalAggPkt = (le32_to_cpu((*(prxdesc0+2)))>>16)&0x3ffff;
#endif
	
	//printk("**********skb->len = %d\n", skb->len);
	// 20 is for ps-poll */
	if((skb->len >=(20 + sizeof(rx_desc_819x_usb))) && (skb->len < RX_URB_SIZE))
	{
		rtl8192SU_query_rxdesc_status(skb, &stats, false);

#ifdef USB_RX_AGGREGATION_SUPPORT
		if(TotalAggPkt > 1)
		{
			PacketLength = stats->Length + GetRxPacketShiftBytes8192SU(&stats, false);
			
			agg_skb = skb;
			skb = dev_alloc_skb(stats->Length);
			if (skb == NULL) {
				dev_kfree_skb_any(agg_skb);
				printk("%s-%d(): dev_alloc_skb() == NULL\n", __FUNCTION__, __LINE__);
				return;
			}
			skb->dev = agg_skb->dev;
			memcpy(skb_put(skb,stats->Length),agg_skb->data,stats->Length);
		}
#endif	
		priv->stats.rxoktotal++;  //YJ,test,090108

		// Process the MPDU recevied */
		skb_trim(skb, skb->len - 4/*sCrcLng*/);//FIXLZM
		
		rx_pkt_len = skb->len;
		ieee80211_hdr = (struct ieee80211_hdr_1addr *)skb->data;
		unicast_packet = false;
		if(is_broadcast_ether_addr(ieee80211_hdr->addr1)) {
			//TODO
		}else if(is_multicast_ether_addr(ieee80211_hdr->addr1)){
			//TODO
		}else {
			// unicast packet */
			unicast_packet = true;
		}

		{
			bool bLedBlinking=TRUE;
			u16 fc=0, type=0;
			fc = le16_to_cpu(ieee80211_hdr->frame_ctl);
			type = WLAN_FC_GET_TYPE(fc);
			if(type == IEEE80211_FTYPE_MGMT)
			{
			  	bLedBlinking = false;
			}
			if(bLedBlinking)
				if(priv->ieee80211->LedControlHandler != NULL)
					priv->ieee80211->LedControlHandler(dev, LED_CTL_RX);
		}

		if(!ieee80211_rx(priv->ieee80211,skb, &stats)) {
			dev_kfree_skb_any(skb);
		} else {
			if(unicast_packet) {
				priv->stats.rxbytesunicast += rx_pkt_len;
			}
		}

		//up is firs pkt, follow is next and next

#ifdef USB_RX_AGGREGATION_SUPPORT
		//
		// The following operations are for processing Rx aggregated packets.
		//
		if(TotalAggPkt > 1){
			//Page size must align to multiple of 128-Bytes.
			PacketOffset = (((PacketLength >> 7) + ((PacketLength & 127) ? 1: 0)) << 7);// RxPageSize is 128bytes as default.
			TotalLength -= PacketOffset;
			TotalAggPkt--;

			while ( (TotalAggPkt>0) && (TotalLength >0))
			{// More aggregated packets need to process.

				skb_pull(agg_skb, PacketOffset - GetRxPacketShiftBytes8192SU(&stats, false));

				//
				// Process the MPDU recevied. 
				//
				memset(&stats, 0, sizeof(struct ieee80211_rx_stats));
				stats.signal = 0;
				stats.noise = -98;
				stats.rate = 0;
				stats.freq = IEEE80211_24GHZ_BAND;
				
				rtl8192SU_query_rxdesc_status(agg_skb, &stats, true);
				PacketLength = stats.Length +  GetRxPacketShiftBytes8192SU(&stats, true);

				if(PacketLength > TotalLength) {
					break;
				}

				//Page size must align to multiple of 128-Bytes.
				PacketOffset = (((PacketLength >> 7) + ((PacketLength & 127) ? 1: 0)) << 7);// RxPageSize is 128bytes as default.
					
				// Process the MPDU recevied */
				skb = dev_alloc_skb(stats.Length);
				if (skb == NULL) {
					dev_kfree_skb_any(agg_skb);
					printk("%s-%d(): dev_alloc_skb() == NULL\n", __FUNCTION__, __LINE__);
					return;
				}
				skb->dev = agg_skb->dev;
				memcpy(skb_put(skb,stats.Length),agg_skb->data, stats.Length);
				skb_trim(skb, skb->len - 4/*sCrcLng*/);

				rx_pkt_len = skb->len;
				ieee80211_hdr = (struct ieee80211_hdr_1addr *)skb->data;
				unicast_packet = false;
				if(is_broadcast_ether_addr(ieee80211_hdr->addr1)) {
					//TODO
				}else if(is_multicast_ether_addr(ieee80211_hdr->addr1)){
					//TODO
				}else {
					// unicast packet */
					unicast_packet = true;
				}
				if(!ieee80211_rx(priv->ieee80211,skb, &stats)) {
					dev_kfree_skb_any(skb);
				} else {
					priv->stats.rxoktotal++;
					if(unicast_packet) {
						priv->stats.rxbytesunicast += rx_pkt_len;
					}
				}

				TotalLength -= PacketOffset;
				TotalAggPkt--;
			}

			dev_kfree_skb(agg_skb);
		}
#endif
	} 
	else 
	{	
		priv->stats.rxurberr++;
		printk("actual_length:%d\n", skb->len);
		dev_kfree_skb_any(skb);
	}

}
#endif
#endif

#ifdef RTL8192U
u32 GetRxPacketShiftBytes819xUsb(struct ieee80211_rx_stats  *Status, bool bIsRxAggrSubframe)
{
#ifdef USB_RX_AGGREGATION_SUPPORT
	if (bIsRxAggrSubframe)
		return (sizeof(rx_desc_819x_usb) + Status->RxDrvInfoSize 
			+ Status->RxBufShift + 8);
	else
#endif	
		return (sizeof(rx_desc_819x_usb) + Status->RxDrvInfoSize 
				+ Status->RxBufShift);
}

void rtl8192_rx_nomal(struct sk_buff* skb)
{
	rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev=info->dev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
		//      .mac_time = jiffies,
		.freq = IEEE80211_24GHZ_BAND,
	};
	u32 rx_pkt_len = 0;
	struct ieee80211_hdr_1addr *ieee80211_hdr = NULL;
	bool unicast_packet = false;
#ifdef USB_RX_AGGREGATION_SUPPORT	
	struct sk_buff *agg_skb = NULL;
	u32  TotalLength = 0;
	u32  TempDWord = 0;
	u32  PacketLength = 0;
	u32  PacketOccupiedLendth = 0;
	u8   TempByte = 0;
	u32  PacketShiftBytes = 0;
	rx_desc_819x_usb_aggr_subframe *RxDescr = NULL;
	u8  PaddingBytes = 0;
	//add just for testing
	u8   testing;
	
#endif		

	// 20 is for ps-poll */
	if((skb->len >=(20 + sizeof(rx_desc_819x_usb))) && (skb->len < RX_URB_SIZE)) {
#ifdef USB_RX_AGGREGATION_SUPPORT
		TempByte = *(skb->data + sizeof(rx_desc_819x_usb));
#endif
		// first packet should not contain Rx aggregation header */
		query_rxdesc_status(skb, &stats, false);
		// TODO */
                if(stats.bHwError) {
                    dev_kfree_skb_any(skb);
                    return;
                }

		// hardware related info */
#ifdef USB_RX_AGGREGATION_SUPPORT
		if (TempByte & BIT0) {
			agg_skb = skb;
			//TotalLength = agg_skb->len - 4; //sCrcLng*/
			TotalLength = stats.Length - 4; //sCrcLng*/
			//RT_TRACE(COMP_RECV, "%s:first aggregated packet!Length=%d\n",__FUNCTION__,TotalLength);
			// though the head pointer has passed this position  */
			TempDWord = *(u32 *)(agg_skb->data - 4);
			PacketLength = (u16)(TempDWord & 0x3FFF); //sCrcLng*/
			skb = dev_alloc_skb(PacketLength);
			if (skb == NULL) {
				dev_kfree_skb_any(agg_skb);
				printk("%s-%d(): dev_alloc_skb() == NULL\n", __FUNCTION__, __LINE__);
				return;
			}
			skb->dev = agg_skb->dev;
			memcpy(skb_put(skb,PacketLength),agg_skb->data,PacketLength);
			PacketShiftBytes = GetRxPacketShiftBytes819xUsb(&stats, false);	
		} 
#endif	
		// Process the MPDU recevied */
		skb_trim(skb, skb->len - 4/*sCrcLng*/);
		
		rx_pkt_len = skb->len;
		ieee80211_hdr = (struct ieee80211_hdr_1addr *)skb->data;
		unicast_packet = false;
		if(is_broadcast_ether_addr(ieee80211_hdr->addr1)) {
			//TODO
		}else if(is_multicast_ether_addr(ieee80211_hdr->addr1)){
			//TODO
		}else {
			// unicast packet */
			unicast_packet = true;
		}

                if(!stats.Decrypted) {
                    skb->cb[0] = 1;
                } else {
                    skb->cb[0] = 0;
                }
		if(!ieee80211_rx(priv->ieee80211,skb, &stats)) {
			dev_kfree_skb_any(skb);
		} else {
			priv->stats.rxoktotal++;
			if(unicast_packet) {
				priv->stats.rxbytesunicast += rx_pkt_len;
			}
		}
#ifdef USB_RX_AGGREGATION_SUPPORT
		testing = 1;
		// (PipeIndex == 0) && (TempByte & BIT0) => TotalLength > 0.
		if (TotalLength > 0) {
			PacketOccupiedLendth = PacketLength + (PacketShiftBytes + 8);
			if ((PacketOccupiedLendth & 0xFF) != 0)
				PacketOccupiedLendth = (PacketOccupiedLendth & 0xFFFFFF00) + 256;
			PacketOccupiedLendth -= 8;
			TempDWord = PacketOccupiedLendth - PacketShiftBytes; /*- PacketLength */
			if (agg_skb->len > TempDWord)
				skb_pull(agg_skb, TempDWord);
			else 
				agg_skb->len = 0;

			while (agg_skb->len>=GetRxPacketShiftBytes819xUsb(&stats, true)) {
				u8 tmpCRC = 0, tmpICV = 0;
				//RT_TRACE(COMP_RECV,"%s:aggred pkt,total_len = %d\n",__FUNCTION__,agg_skb->len);
				RxDescr = (rx_desc_819x_usb_aggr_subframe *)(agg_skb->data);
				tmpCRC = RxDescr->CRC32;
				tmpICV = RxDescr->ICV;
				memcpy(agg_skb->data, &agg_skb->data[44], 2);
				RxDescr->CRC32 = tmpCRC;
				RxDescr->ICV = tmpICV;

				memset(&stats, 0, sizeof(struct ieee80211_rx_stats));
				stats.signal = 0;
				stats.noise = -98;
				stats.rate = 0;
				stats.freq = IEEE80211_24GHZ_BAND;
				query_rxdesc_status(agg_skb, &stats, true);
				PacketLength = stats.Length;

				if(PacketLength > agg_skb->len) {
					break;
				}
				// Process the MPDU recevied */
				skb = dev_alloc_skb(PacketLength);
				if (skb == NULL) {
					printk("%s-%d(): dev_alloc_skb() == NULL\n", __FUNCTION__, __LINE__);
					break;
				}
				skb->dev = agg_skb->dev;
				memcpy(skb_put(skb,PacketLength),agg_skb->data, PacketLength);
				skb_trim(skb, skb->len - 4/*sCrcLng*/);

				rx_pkt_len = skb->len;
				ieee80211_hdr = (struct ieee80211_hdr_1addr *)skb->data;
				unicast_packet = false;
				if(is_broadcast_ether_addr(ieee80211_hdr->addr1)) {
					//TODO
				}else if(is_multicast_ether_addr(ieee80211_hdr->addr1)){
					//TODO
				}else {
					// unicast packet */
					unicast_packet = true;
				}
                                if(!stats.Decrypted) {
                                    skb->cb[0] = 1;
                                    printk("need software to assit decription!\n");
                                } else {
                                    skb->cb[0] = 0;
                                }
				if(!ieee80211_rx(priv->ieee80211,skb, &stats)) {
					dev_kfree_skb_any(skb);
				} else {
					priv->stats.rxoktotal++;
					if(unicast_packet) {
						priv->stats.rxbytesunicast += rx_pkt_len;
					}
				}
				// should trim the packet which has been copied to target skb */
				skb_pull(agg_skb, PacketLength);
				PacketShiftBytes = GetRxPacketShiftBytes819xUsb(&stats, true);
				PacketOccupiedLendth = PacketLength + PacketShiftBytes;
				if ((PacketOccupiedLendth & 0xFF) != 0) {
					PaddingBytes = 256 - (PacketOccupiedLendth & 0xFF);
					if (agg_skb->len > PaddingBytes)
						skb_pull(agg_skb, PaddingBytes);
					else
						agg_skb->len = 0;	
				}
			}
			dev_kfree_skb(agg_skb);
		}
#endif
	} else {
		priv->stats.rxurberr++;
		printk("actual_length:%d\n", skb->len);
		dev_kfree_skb_any(skb);
	}

}

#endif

void
rtl819xusb_process_received_packet(
	struct net_device *dev,
	struct ieee80211_rx_stats *pstats	
	)
{
//	bool bfreerfd=false, bqueued=false;
	u8* 	frame;
	u16     frame_len=0;
	struct r8192_priv *priv = ieee80211_priv(dev);
//	u8			index = 0;
//	u8			TID = 0;
	//u16			seqnum = 0;
	//PRX_TS_RECORD	pts = NULL;

	// Get shifted bytes of Starting address of 802.11 header. 2006.09.28, by Emily
	//porting by amy 080508
	pstats->virtual_address += get_rxpacket_shiftbytes_819xusb(pstats);
	frame = pstats->virtual_address;
	frame_len = pstats->packetlength;
#ifdef TODO	// by amy about HCT
	if(!Adapter->bInHctTest)
		CountRxErrStatistics(Adapter, pRfd);
#endif
	{
	#ifdef ENABLE_PS  //by amy for adding ps function in future
		RT_RF_POWER_STATE rtState;
		// When RF is off, we should not count the packet for hw/sw synchronize
		// reason, ie. there may be a duration while sw switch is changed and hw
		// switch is being changed. 2006.12.04, by shien chang.
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (u8* )(&rtState));
		if (rtState == eRfOff)
		{
			return;
		}
	#endif
	priv->stats.rxframgment++;

	}
#ifdef TODO
	RmMonitorSignalStrength(Adapter, pRfd);
#endif
	// 2007/01/16 MH Add RX command packet handle here. */
	// 2007/03/01 MH We have to release RFD and return if rx pkt is cmd pkt. */
	if (rtl819xusb_rx_command_packet(dev, pstats))
	{
		return;
	}

#ifdef SW_CRC_CHECK
	SwCrcCheck();
#endif

	
}

void query_rx_cmdpkt_desc_status(struct sk_buff *skb, struct ieee80211_rx_stats *stats)
{
//	rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
//	struct net_device *dev=info->dev;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	rx_desc_819x_usb *desc = (rx_desc_819x_usb *)skb->data;
//	rx_drvinfo_819x_usb  *driver_info;

	//
	//Get Rx Descriptor Information
	// 
	stats->virtual_address = (u8*)skb->data;
	stats->Length = desc->Length;
	stats->RxDrvInfoSize = 0;
	stats->RxBufShift = 0;
	stats->packetlength = stats->Length-scrclng;
	stats->fraglength = stats->packetlength;
	stats->fragoffset = 0;
	stats->ntotalfrag = 1;
}

#ifdef RTL8192SU
void rtl8192SU_rx_cmd(struct sk_buff *skb)
{
	struct rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev = info->dev;

	// TODO */
	struct ieee80211_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
		//      .mac_time = jiffies,
		.freq = IEEE80211_24GHZ_BAND,
	};

	//
	// Check buffer length to determine if this is a valid MPDU.
	//
	if( (skb->len >= sizeof(rx_desc_819x_usb)) && (skb->len <= RX_URB_SIZE) )//&&
		//(pHalData->SwChnlInProgress == FALSE))
	{
		//
		// Collection information in Rx descriptor.
		//
#if 0
		pRxDesc = pContext->Buffer;

		pRfd->Buffer.VirtualAddress = pContext->Buffer; // 061109, rcnjko, for multi-platform consideration..

		pRtRfdStatus->Length = (u2Byte)GET_RX_DESC_PKT_LEN(pRxDesc);	
		pRtRfdStatus->RxDrvInfoSize = 0;
		pRtRfdStatus->RxBufShift = 0;
		
		pRfd->PacketLength	= pRfd->Status.Length - sCrcLng;
		pRfd->FragLength	= pRfd->PacketLength;
		pRfd->FragOffset	= 0;
		pRfd->nTotalFrag	= 1;
		pRfd->queue_id = PipeIndex;
#endif
		query_rx_cmdpkt_desc_status(skb,&stats);
		// this is to be done by amy 080508     prfd->queue_id = 1;
			
		//
		// Process the MPDU recevied. 
		//
		rtl819xusb_process_received_packet(dev,&stats);

		dev_kfree_skb_any(skb);
	}
	else
	{
		//RTInsertTailListWithCnt(&pAdapter->RfdIdleQueue, &pRfd->List, &pAdapter->NumIdleRfd);
		//RT_ASSERT(pAdapter->NumIdleRfd <= pAdapter->NumRfd, ("HalUsbInCommandComplete8192SUsb(): Adapter->NumIdleRfd(%d)\n", pAdapter->NumIdleRfd));
		//RT_TRACE(COMP_RECV, DBG_LOUD, ("HalUsbInCommandComplete8192SUsb(): NOT enough Resources!! BufLenUsed(%d), NumIdleRfd(%d)\n", 
			//pContext->BufLenUsed, pAdapter->NumIdleRfd));
	}

	//
	// Reuse USB_IN_CONTEXT since we had finished processing the 
	// buffer in USB_IN_CONTEXT.
	//
	//HalUsbReturnInContext(pAdapter, pContext);

	//
	// Issue another bulk IN transfer.
	//
	//HalUsbInMpdu(pAdapter, PipeIndex);

	RT_TRACE(COMP_RECV, "<--- HalUsbInCommandComplete8192SUsb()\n");	

}
#endif
#ifdef RTL8192U
void rtl8192_rx_cmd(struct sk_buff *skb)
{
	struct rtl8192_rx_info *info = (struct rtl8192_rx_info *)skb->cb;
	struct net_device *dev = info->dev;
	//int ret;
//	struct urb *rx_urb = info->urb;
	// TODO */
	struct ieee80211_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
		//      .mac_time = jiffies,
		.freq = IEEE80211_24GHZ_BAND,
	};

	if((skb->len >=(20 + sizeof(rx_desc_819x_usb))) && (skb->len < RX_URB_SIZE))
	{

		query_rx_cmdpkt_desc_status(skb,&stats);
		// this is to be done by amy 080508     prfd->queue_id = 1;


		//
		//  Process the command packet received. 
		//

		rtl819xusb_process_received_packet(dev,&stats);

		dev_kfree_skb_any(skb);
	}
	else
		;

	
#if 0
	desc = (u32*)(skb->data);
	cmd = (desc[0] >> 30) & 0x03;

	if(cmd == 0x00) {//beacon interrupt
		//send beacon packet
		skb = ieee80211_get_beacon(priv->ieee80211);

		if(!skb){ 
			DMESG("not enought memory for allocating beacon");
			return;
		}
		skb->cb[0] = BEACON_PRIORITY;
		skb->cb[1] = 0;
		skb->cb[2] = ieeerate2rtlrate(priv->ieee80211->basic_rate);
		ret = rtl8192_tx(dev, skb);

		if( ret != 0 ){
			printk(KERN_ALERT "tx beacon packet error : %d !\n", ret);
		}	
		dev_kfree_skb_any(skb);
	} else {//0x00
		//{ log the device information
		// At present, It is not implemented just now.
		//}
	}
#endif
}
#endif

void rtl8192_irq_rx_tasklet(struct r8192_priv *priv)
{
        struct sk_buff *skb;
	struct rtl8192_rx_info *info; 
	
        while (NULL != (skb = skb_dequeue(&priv->rx_skb_queue))) {
		info = (struct rtl8192_rx_info *)skb->cb;
                switch (info->out_pipe) {
                    // Nomal packet pipe */
			case 3:
				//RT_TRACE(COMP_RECV, "normal in-pipe index(%d)\n",info->out_pipe);
				priv->ops->rtl819x_rx_nomal(skb);
				break;

				// Command packet pipe */
			case 9:
				RT_TRACE(COMP_RECV, "command in-pipe index(%d)\n",\
						info->out_pipe);
				priv->ops->rtl819x_rx_cmd(skb);
				break;

			default: // should never get here! */
				RT_TRACE(COMP_ERR, "Unknown in-pipe index(%d)\n",\
						info->out_pipe);
				dev_kfree_skb(skb);
				break;

		}
        }
}



/****************************************************************************
     ---------------------------- USB_STUFF---------------------------
*****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
//LZM Merge from windows HalUsbSetQueuePipeMapping8192SUsb 090319
static void HalUsbSetQueuePipeMapping8192SUsb(struct usb_interface *intf, struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
#ifndef USE_ONE_PIPE
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	u8 i = 0;
#endif	

	priv->ep_in_num = 0;
	priv->ep_out_num = 0;
	memset(priv->RtOutPipes,0,16);
	memset(priv->RtInPipes,0,16);

#ifndef USE_ONE_PIPE
	iface_desc = intf->cur_altsetting;
	priv->ep_num = iface_desc->desc.bNumEndpoints;

	for (i = 0; i < priv->ep_num; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
		if (usb_endpoint_is_bulk_in(endpoint)) {
			priv->RtInPipes[priv->ep_in_num] = usb_endpoint_num(endpoint);
			priv->ep_in_num ++;
			//printk("in_endpoint_idx = %d\n", usb_endpoint_num(endpoint));
		} else if (usb_endpoint_is_bulk_out(endpoint)) {
			priv->RtOutPipes[priv->ep_out_num] = usb_endpoint_num(endpoint);
			priv->ep_out_num ++;
			//printk("out_endpoint_idx = %d\n", usb_endpoint_num(endpoint));
		}
#else
		if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) && 
		     ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)) {
			// we found a bulk in endpoint */
			priv->RtInPipes[priv->ep_in_num] = (endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
			priv->ep_in_num ++;
		} else if (((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT) && 
		     ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)) {
			// We found bulk out endpoint */
			priv->RtOutPipes[priv->ep_out_num] = endpoint->bEndpointAddress;
			priv->ep_out_num ++;
			}
#endif
	}
	{
		memset(priv->txqueue_to_outpipemap,0,9);
		if (priv->ep_num == 6) {
			// BK, BE, VI, VO, HCCA, TXCMD, MGNT, HIGH, BEACON
			u8 queuetopipe[] = {3, 2, 1, 0, 4, 4, 4, 4, 4};

			memcpy(priv->txqueue_to_outpipemap,queuetopipe,9);
		} else if (priv->ep_num == 4) {
			// BK, BE, VI, VO, HCCA, TXCMD, MGNT, HIGH, BEACON
			u8 queuetopipe[] = {1, 1, 0, 0, 2, 2, 2, 2, 2};

			memcpy(priv->txqueue_to_outpipemap,queuetopipe,9);
		} else if (priv->ep_num > 9) {
			// BK, BE, VI, VO, HCCA, TXCMD, MGNT, HIGH, BEACON
			u8 queuetopipe[] = {3, 2, 1, 0, 4, 8, 7, 6, 5};

			memcpy(priv->txqueue_to_outpipemap,queuetopipe,9);
		} else {//use sigle pipe 
			// BK, BE, VI, VO, HCCA, TXCMD, MGNT, HIGH, BEACON
			u8 queuetopipe[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
			memcpy(priv->txqueue_to_outpipemap,queuetopipe,9);
		}
	}
	printk("==>ep_num:%d, in_ep_num:%d, out_ep_num:%d\n", priv->ep_num, priv->ep_in_num, priv->ep_out_num);

	printk("==>RtInPipes:");
	for(i=0; i < priv->ep_in_num; i++)
		printk("%d  ", priv->RtInPipes[i]);
	printk("\n");

	printk("==>RtOutPipes:");
	for(i=0; i < priv->ep_out_num; i++)
		printk("%d  ", priv->RtOutPipes[i]);
	printk("\n");

	printk("==>txqueue_to_outpipemap for BK, BE, VI, VO, HCCA, TXCMD, MGNT, HIGH, BEACON:\n");
	for(i=0; i < 9; i++)
		printk("%d  ", priv->txqueue_to_outpipemap[i]);
	printk("\n");
#else
	{
		memset(priv->txqueue_to_outpipemap,0,9);
		memset(priv->RtOutPipes,4,16);//all use endpoint 4 for out
	}
#endif

	return;
}
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
static int __devinit rtl8192_usb_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
#else
static void * __devinit rtl8192_usb_probe(struct usb_device *udev,
			                unsigned int ifnum,
			          const struct usb_device_id *id)
#endif
{
//	unsigned long ioaddr = 0;
	struct net_device *dev = NULL;
	struct r8192_priv *priv= NULL;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct usb_device *udev = interface_to_usbdev(intf);
#endif
        RT_TRACE(COMP_INIT, "Oops: i'm coming\n");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
// In this probe function, O.S. will provide the usb interface pointer to driver.
// We have to increase the reference count of the usb device structure by using the usb_get_dev function
	usb_get_dev(udev);
#endif
	dev = alloc_ieee80211(sizeof(struct r8192_priv));
        if (!dev)
                goto fail;
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	SET_MODULE_OWNER(dev);
#endif

	priv = ieee80211_priv(dev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	priv->ieee80211 = netdev_priv(dev);
#else
	priv->ieee80211 = (struct net_device *)dev->priv;
#endif
	priv->udev=udev;
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	HalUsbSetQueuePipeMapping8192SUsb(intf, dev);
#else//use one pipe
	{
		memset(priv->txqueue_to_outpipemap,0,9);
		memset(priv->RtOutPipes,4,16);//all use endpoint 4 for out
	}
#endif
	
#ifdef RTL8192SU
	//printk("===============>NIC 8192SU\n");
	priv->ops = &rtl8192su_ops;
#else
	//printk("===============>NIC 8192U\n");
	priv->ops = &rtl8192u_ops;
#endif

	dev->open = rtl8192_open;
	dev->stop = rtl8192_close;
	//dev->hard_start_xmit = rtl8192_8023_hard_start_xmit;
	dev->tx_timeout = tx_timeout;
	//dev->wireless_handlers = &r8192_wx_handlers_def;
	dev->do_ioctl = rtl8192_ioctl;
	dev->set_multicast_list = r8192_set_multicast;
	dev->set_mac_address = r8192_set_mac_adr;
	dev->get_stats = rtl8192_stats;
	
         //DMESG("Oops: i'm coming\n");
#if WIRELESS_EXT >= 12
#if WIRELESS_EXT < 17
        dev->get_wireless_stats = r8192_get_wireless_stats;
#endif
        dev->wireless_handlers = (struct iw_handler_def *) &r8192_wx_handlers_def;
#endif
	dev->type=ARPHRD_ETHER;

	dev->watchdog_timeo = HZ*3;	//modified by john, 0805

	if (dev_alloc_name(dev, ifname) < 0){
                RT_TRACE(COMP_INIT, "Oops: devname already taken! Trying wlan%%d...\n");
		ifname = "wlan%d";
		dev_alloc_name(dev, ifname);
        }
	
	RT_TRACE(COMP_INIT, "Driver probe completed1\n");

	if(rtl8192_init(dev)!=0){ 
		RT_TRACE(COMP_ERR, "Initialization failed");
		goto fail;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	usb_set_intfdata(intf, dev);	
	SET_NETDEV_DEV(dev, &intf->dev);
#endif	
	netif_carrier_off(dev);
	netif_stop_queue(dev);
	
	if(register_netdev(dev) != 0){
		RT_TRACE(COMP_INIT, "register_netdev() failed\n");
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		usb_set_intfdata(intf, NULL);
#endif
		goto fail;
	}
	RT_TRACE(COMP_INIT, "dev name=======> %s\n",dev->name);
	rtl8192_proc_init_one(dev);
	
	
	RT_TRACE(COMP_INIT, "Driver probe completed\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	return dev;
#else
	return 0;	
#endif

	
fail:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	usb_put_dev(udev);
#endif
	if(dev){
		//unregister_netdev(dev);
		free_ieee80211(dev);
	}
		
	RT_TRACE(COMP_ERR, "wlan driver load failed\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	
	return NULL;
#else
	return -ENODEV;
#endif
	
}

//detach all the work and timer structure declared or inititialize in r8192U_init function.
void rtl8192_cancel_deferred_work(struct r8192_priv* priv)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
	cancel_work_sync(&priv->reset_wq);
	cancel_work_sync(&priv->qos_activate);
        cancel_work_sync(&priv->mcast_wq);
	cancel_delayed_work(&priv->watch_dog_wq);
	cancel_delayed_work(&priv->update_beacon_wq);
	cancel_delayed_work(&priv->ieee80211->hw_wakeup_wq);
	cancel_delayed_work(&priv->ieee80211->hw_sleep_wq);
#ifdef RTL8192U
	cancel_delayed_work(&priv->ieee80211->update_assoc_sta_info_wq);
#endif
	//cancel_work_sync(&priv->SetBWModeWorkItem);
	//cancel_work_sync(&priv->SwChnlWorkItem);
#ifdef RTL8192SU
	cancel_work_sync(&priv->FwCmdIOWorkItem);
#endif
#else
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	cancel_delayed_work(&priv->reset_wq);
	cancel_delayed_work(&priv->qos_activate);
        cancel_delayed_work(&priv->mcast_wq);
	cancel_delayed_work(&priv->watch_dog_wq);
	cancel_delayed_work(&priv->update_beacon_wq);
	cancel_delayed_work(&priv->ieee80211->hw_wakeup_wq);
	cancel_delayed_work(&priv->ieee80211->hw_sleep_wq);
#ifdef RTL8192U
	cancel_delayed_work(&priv->ieee80211->update_assoc_sta_info_wq);
#endif

	//cancel_delayed_work(&priv->SetBWModeWorkItem);
	//cancel_delayed_work(&priv->SwChnlWorkItem);	
#ifdef RTL8192SU
	cancel_delayed_work(&priv->FwCmdIOWorkItem);
#endif
#endif
#endif

}


#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) 
static void __devexit rtl8192_usb_disconnect(struct usb_interface *intf)
#else 
static void __devexit rtl8192_usb_disconnect(struct usb_device *udev, void *ptr)
#endif
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct net_device *dev = usb_get_intfdata(intf);
#else
	struct net_device *dev = (struct net_device *)ptr;
#endif
	
	struct r8192_priv *priv = ieee80211_priv(dev);
 	if(dev){
		
		unregister_netdev(dev);
		
		RT_TRACE(COMP_DOWN, "=============>wlan driver to be removed\n");
		rtl8192_proc_remove_one(dev);
		
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
		usb_put_dev(interface_to_usbdev(intf));
#endif
		//rtl8192_down(dev);
		if (priv->pFirmware)
		{
#ifdef RTK_DMP_PLATFORM
			dvr_free(priv->pFirmware);
#else
			vfree(priv->pFirmware);
#endif
			priv->pFirmware = NULL;
		}
	//	priv->rf_close(dev);
//		rtl8192_SetRFPowerState(dev, eRfOff);

#ifdef RTL8192SU
		DeInitSwLeds(dev);
#endif
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
		destroy_workqueue(priv->priv_wq);
#endif
		//rtl8192_irq_disable(dev);
		//rtl8192_reset(dev);
		mdelay(10);

		free_ieee80211(dev);
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	if(interface_to_usbdev(intf)->state != USB_STATE_NOTATTACHED)
		usb_reset_device(interface_to_usbdev(intf));
	usb_set_intfdata(intf, NULL);
#endif

	RT_TRACE(COMP_DOWN, "wlan driver removed\n");
}

#ifdef BUILT_IN_IEEE80211
/* fun with the built-in ieee80211 stack... */
extern int ieee80211_init(void);
extern int ieee80211_exit(void);
extern int ieee80211_crypto_init(void);
extern void ieee80211_crypto_deinit(void);
extern int ieee80211_crypto_tkip_init(void);
extern void ieee80211_crypto_tkip_exit(void);
extern int ieee80211_crypto_ccmp_init(void);
extern void ieee80211_crypto_ccmp_exit(void);
extern int ieee80211_crypto_wep_init(void);
extern void ieee80211_crypto_wep_exit(void);
#endif

#ifdef BUILT_IN_CRYPTO
int arc4_init(void);
void arc4_exit(void);
int michael_mic_init(void);
void michael_mic_exit(void);
int aes_init(void);
void aes_fini(void);
#endif

static int __init rtl8192_usb_module_init(void)
{
        int ret;

#ifdef BUILT_IN_CRYPTO
        ret = arc4_init();
        if (ret) {
                printk(KERN_ERR "arc4_init() failed %d\n", ret);
                return ret;
        }


        ret = michael_mic_init();
        if (ret) {
                printk(KERN_ERR "michael_mic_init() failed %d\n", ret);
                return ret;
        }
        
        ret = aes_init();
        if (ret) {
                printk(KERN_ERR "aes_init() failed %d\n", ret);
                return ret;
        }
#endif

#ifdef BUILT_IN_IEEE80211
        ret = ieee80211_init();
        if (ret) {
                printk(KERN_ERR "ieee80211_init() failed %d\n", ret);
                return ret;
        }
        ret = ieee80211_crypto_init();
        if (ret) {
                printk(KERN_ERR "ieee80211_crypto_init() failed %d\n", ret);
                return ret;
        }
        ret = ieee80211_crypto_tkip_init();
        if (ret) {
                printk(KERN_ERR "ieee80211_crypto_tkip_init() failed %d\n", ret);
                return ret;
        }
        ret = ieee80211_crypto_ccmp_init();
        if (ret) {
                printk(KERN_ERR "ieee80211_crypto_ccmp_init() failed %d\n", ret);
                return ret;
        }
        ret = ieee80211_crypto_wep_init();
        if (ret) {
                printk(KERN_ERR "ieee80211_crypto_wep_init() failed %d\n", ret);
                return ret;
        }
#endif
	printk(KERN_INFO "\nLinux kernel driver for RTL8192 based WLAN cards\n");
	printk(KERN_INFO "Copyright (c) 2007-2008, Realsil Wlan\n");
	RT_TRACE(COMP_INIT, "Initializing module");
	RT_TRACE(COMP_INIT, "Wireless extensions version %d", WIRELESS_EXT);
	rtl8192_proc_module_init();
	return usb_register(&rtl8192_usb_driver);
}


static void __exit rtl8192_usb_module_exit(void)
{
	RT_TRACE(COMP_DOWN, "Exiting");
	usb_deregister(&rtl8192_usb_driver);
	rtl8192_proc_module_remove();

#ifdef BUILT_IN_IEEE80211
        ieee80211_crypto_tkip_exit();
        ieee80211_crypto_ccmp_exit();
        ieee80211_crypto_wep_exit();
        ieee80211_crypto_deinit();
        ieee80211_exit();
#endif	

#ifdef BUILT_IN_CRYPTO
        arc4_exit();
        michael_mic_exit();
        aes_fini();
#endif

}


void rtl8192_try_wake_queue(struct net_device *dev, int pri)
{
	unsigned long flags;
	short enough_desc;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	spin_lock_irqsave(&priv->tx_lock,flags);
	enough_desc = check_nic_enough_desc(dev,pri);
        spin_unlock_irqrestore(&priv->tx_lock,flags);	
	
	if(enough_desc)
		ieee80211_wake_queue(priv->ieee80211);
}

#if 0
void DisableHWSecurityConfig8192SUsb(struct net_device *dev)
{
	u8 SECR_value = 0x0;
	write_nic_byte(dev, SECR,  SECR_value);//SECR_value |  SCR_UseDK ); 
}
#endif

void EnableHWSecurityConfig8192(struct net_device *dev)
{
        u8 SECR_value = 0x0;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	 struct ieee80211_device* ieee = priv->ieee80211; 

	SECR_value = SCR_TxEncEnable | SCR_RxDecEnable;
#if 1
	if (((KEY_TYPE_WEP40 == ieee->pairwise_key_type) || (KEY_TYPE_WEP104 == ieee->pairwise_key_type)) && (priv->ieee80211->auth_mode != 2))
	{
		SECR_value |= SCR_RxUseDK;
		SECR_value |= SCR_TxUseDK;
	}
	else if ((ieee->iw_mode == IW_MODE_ADHOC) && (ieee->pairwise_key_type & (KEY_TYPE_CCMP | KEY_TYPE_TKIP)))
	{
		SECR_value |= SCR_RxUseDK;
		SECR_value |= SCR_TxUseDK;
	}
#endif
        //add HWSec active enable here.
//default using hwsec. when peer AP is in N mode only and pairwise_key_type is none_aes(which HT_IOT_ACT_PURE_N_MODE indicates it), use software security. when peer AP is in b,g,n mode mixed and pairwise_key_type is none_aes, use g mode hw security. WB on 2008.7.4

	ieee->hwsec_active = 1;
	
	if ((ieee->pHTInfo->IOTAction&HT_IOT_ACT_PURE_N_MODE) || !hwwep)//!ieee->hwsec_support) //add hwsec_support flag to totol control hw_sec on/off
	{
		ieee->hwsec_active = 0;
		SECR_value &= ~SCR_RxDecEnable;
	}
	
	RT_TRACE(COMP_SEC,"%s:, hwsec:%d, pairwise_key:%d, SECR_value:%x\n", __FUNCTION__, \
			ieee->hwsec_active, ieee->pairwise_key_type, SECR_value);	
	{
                write_nic_byte(dev, SECR,  SECR_value);//SECR_value |  SCR_UseDK ); 
        }
}


void setKey(	struct net_device *dev, 
		u8 EntryNo,
		u8 KeyIndex, 
		u16 KeyType, 
		u8 *MacAddr, 
		u8 DefaultKey, 
		u32 *KeyContent )
{
	u32 TargetCommand = 0;
	u32 TargetContent = 0;
	u16 usConfig = 0;
	u8 i;
	if (EntryNo >= TOTAL_CAM_ENTRY)
		RT_TRACE(COMP_ERR, "cam entry exceeds in setKey()\n");

	RT_TRACE(COMP_SEC, "====>to setKey(), dev:%p, EntryNo:%d, KeyIndex:%d, KeyType:%d, MacAddr"MAC_FMT"\n", dev,EntryNo, KeyIndex, KeyType, MAC_ARG(MacAddr));
	
	if (DefaultKey)
		usConfig |= BIT15 | (KeyType<<2);
	else
		usConfig |= BIT15 | (KeyType<<2) | KeyIndex;
//	usConfig |= BIT15 | (KeyType<<2) | (DefaultKey<<5) | KeyIndex;


	for(i=0 ; i<CAM_CONTENT_COUNT; i++){
		TargetCommand  = i+CAM_CONTENT_COUNT*EntryNo;
		TargetCommand |= BIT31|BIT16;

		if(i==0){//MAC|Config
			TargetContent = (u32)(*(MacAddr+0)) << 16|
					(u32)(*(MacAddr+1)) << 24|
					(u32)usConfig;

			write_nic_dword(dev, WCAMI, TargetContent); 
			write_nic_dword(dev, RWCAM, TargetCommand);
	//		printk("setkey cam =%8x\n", read_cam(dev, i+6*EntryNo));
		}
		else if(i==1){//MAC
                        TargetContent = (u32)(*(MacAddr+2)) 	 |
                                        (u32)(*(MacAddr+3)) <<  8|
                                        (u32)(*(MacAddr+4)) << 16|
                                        (u32)(*(MacAddr+5)) << 24;
			write_nic_dword(dev, WCAMI, TargetContent); 
			write_nic_dword(dev, RWCAM, TargetCommand);
		}
		else {
			//Key Material
			if(KeyContent !=NULL){
			write_nic_dword(dev, WCAMI, (u32)(*(KeyContent+i-2)) ); 
			write_nic_dword(dev, RWCAM, TargetCommand);
		}
	}
	}

}

/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/
module_init(rtl8192_usb_module_init);
module_exit(rtl8192_usb_module_exit);
