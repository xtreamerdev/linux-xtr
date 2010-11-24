/*
   This is part of rtl8187 OpenSource driver - v 0.1
   Copyright (C) Andrea Merello 2005  <andreamrl@tiscali.it> 
   Released under the terms of GPL (General Public License)
   
   
   Parts of this driver are based on the rtl8180 driver skeleton 
   from Patric Schenke & Andres Salomon.

   Parts of this driver are based on the Intel Pro Wireless 2*00 GPL drivers.
   
   some ideas might be derived from David Young rtl8180 netbsd driver.
   
   Parts of the usb code are from the r8150.c driver in linux kernel
   
   Some ideas borrowed from the 8139too.c driver included in linux kernel.
   
   We (I?) want to thanks the Authors of those projecs and also the 
   Ndiswrapper's project Authors.
   
   A special big thanks goes also to Realtek corp. for their help in my 
   attempt to add RTL8187 and RTL8225 support, and to David Young also. 

	- Please note that this file is a modified version from rtl8180-sa2400 
	drv. So some other people have contributed to this project, and they are
	thanked in the rtl8180-sa2400 CHANGELOG.
*/

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

#define CONFIG_RTL8180_IO_MAP

#include "r8180_hw.h"
#include "r8187.h"
#include "r8180_rtl8225.h" /* RTL8225 Radio frontend */
#include "r8180_93cx6.h"   /* Card EEPROM */
#include "r8180_wx.h"

#include <linux/usb.h>
// FIXME: check if 2.6.7 is ok
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,7))
#define usb_kill_urb usb_unlink_urb
#endif

#ifdef CONFIG_RTL8180_PM
#include "r8180_pm.h"
#endif

#ifndef USB_VENDOR_ID_REALTEK
#define USB_VENDOR_ID_REALTEK		0x0bda
#endif
#ifndef USB_VENDOR_ID_NETGEAR
#define USB_VENDOR_ID_NETGEAR		0x0846
#endif

#define TXISR_SELECT(priority)	  ((priority == MANAGE_PRIORITY)?rtl8187_managetx_isr:\
	(priority == BEACON_PRIORITY)?rtl8187_beacontx_isr: \
	(priority == VO_PRIORITY)?rtl8187_votx_isr: \
	(priority == VI_PRIORITY)?rtl8187_vitx_isr:\
	(priority == BE_PRIORITY)?rtl8187_betx_isr:rtl8187_bktx_isr)

static u32 MAC_REG_TABLE[][3]={ 
	{0xf0, 0x32, 0000}, {0xf1, 0x32, 0000}, {0xf2, 0x00, 0000}, {0xf3, 0x00, 0000}, 
	{0xf4, 0x32, 0000}, {0xf5, 0x43, 0000}, {0xf6, 0x00, 0000}, {0xf7, 0x00, 0000},
	{0xf8, 0x46, 0000}, {0xf9, 0xa4, 0000}, {0xfa, 0x00, 0000}, {0xfb, 0x00, 0000},
	{0xfc, 0x96, 0000}, {0xfd, 0xa4, 0000}, {0xfe, 0x00, 0000}, {0xff, 0x00, 0000}, 

	{0x58, 0x4b, 0001}, {0x59, 0x00, 0001}, {0x5a, 0x4b, 0001}, {0x5b, 0x00, 0001},
	{0x60, 0x4b, 0001}, {0x61, 0x09, 0001}, {0x62, 0x4b, 0001}, {0x63, 0x09, 0001},
	{0xce, 0x0f, 0001}, {0xcf, 0x00, 0001}, {0xe0, 0xff, 0001}, {0xe1, 0x0f, 0001},
	{0xe2, 0x00, 0001}, {0xf0, 0x4e, 0001}, {0xf1, 0x01, 0001}, {0xf2, 0x02, 0001},
	{0xf3, 0x03, 0001}, {0xf4, 0x04, 0001}, {0xf5, 0x05, 0001}, {0xf6, 0x06, 0001},
	{0xf7, 0x07, 0001}, {0xf8, 0x08, 0001}, 

	{0x4e, 0x00, 0002}, {0x0c, 0x04, 0002}, {0x21, 0x61, 0002}, {0x22, 0x68, 0002}, 
	{0x23, 0x6f, 0002}, {0x24, 0x76, 0002}, {0x25, 0x7d, 0002}, {0x26, 0x84, 0002}, 
	{0x27, 0x8d, 0002}, {0x4d, 0x08, 0002}, {0x50, 0x05, 0002}, {0x51, 0xf5, 0002}, 
	{0x52, 0x04, 0002}, {0x53, 0xa0, 0002}, {0x54, 0x1f, 0002}, {0x55, 0x23, 0002}, 
	{0x56, 0x45, 0002}, {0x57, 0x67, 0002}, {0x58, 0x08, 0002}, {0x59, 0x08, 0002}, 
	{0x5a, 0x08, 0002}, {0x5b, 0x08, 0002}, {0x60, 0x08, 0002}, {0x61, 0x08, 0002}, 
	{0x62, 0x08, 0002}, {0x63, 0x08, 0002}, {0x64, 0xcf, 0002}, {0x72, 0x56, 0002}, 
	{0x73, 0x9a, 0002},

	{0x34, 0xf0, 0000}, {0x35, 0x0f, 0000}, {0x5b, 0x40, 0000}, {0x84, 0x88, 0000},
	{0x85, 0x24, 0000}, {0x88, 0x54, 0000}, {0x8b, 0xb8, 0000}, {0x8c, 0x07, 0000},
	{0x8d, 0x00, 0000}, {0x94, 0x1b, 0000}, {0x95, 0x12, 0000}, {0x96, 0x00, 0000},
	{0x97, 0x06, 0000}, {0x9d, 0x1a, 0000}, {0x9f, 0x10, 0000}, {0xb4, 0x22, 0000},
	{0xbe, 0x80, 0000}, {0xdb, 0x00, 0000}, {0xee, 0x00, 0000}, {0x91, 0x03, 0000},

	{0x4c, 0x03, 0002}, {0x9f, 0x00, 0003}, {0x8c, 0x01, 0000}, {0x8d, 0x10, 0000},
	{0x8e, 0x08, 0000}, {0x8f, 0x00, 0000}
};

static u8  ZEBRA2_AGC[]={
	0,
	0x5e,0x5e,0x5e,0x5e,0x5d,0x5b,0x59,0x57,0x55,0x53,0x51,0x4f,0x4d,0x4b,0x49,0x47,
	0x45,0x43,0x41,0x3f,0x3d,0x3b,0x39,0x37,0x35,0x33,0x31,0x2f,0x2d,0x2b,0x29,0x27,
	0x25,0x23,0x21,0x1f,0x1d,0x1b,0x19,0x17,0x15,0x13,0x11,0x0f,0x0d,0x0b,0x09,0x07,
	0x05,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x19,0x19,0x19,0x019,0x19,0x19,0x19,0x19,0x19,0x20,0x21,0x22,0x23,0x24,0x25,0x26,
	0x26,0x27,0x27,0x28,0x28,0x29,0x2a,0x2a,0x2a,0x2b,0x2b,0x2b,0x2c,0x2c,0x2c,0x2d,
	0x2d,0x2d,0x2d,0x2e,0x2e,0x2e,0x2e,0x2f,0x2f,0x2f,0x30,0x30,0x31,0x31,0x31,0x31,
	0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31
};

static u32 ZEBRA2_RF_RX_GAIN_TABLE[]={	
	0,
	0x0400,0x0401,0x0402,0x0403,0x0404,0x0405,0x0408,0x0409,
	0x040a,0x040b,0x0502,0x0503,0x0504,0x0505,0x0540,0x0541,
	0x0542,0x0543,0x0544,0x0545,0x0580,0x0581,0x0582,0x0583,
	0x0584,0x0585,0x0588,0x0589,0x058a,0x058b,0x0643,0x0644,
	0x0645,0x0680,0x0681,0x0682,0x0683,0x0684,0x0685,0x0688,
	0x0689,0x068a,0x068b,0x068c,0x0742,0x0743,0x0744,0x0745,
	0x0780,0x0781,0x0782,0x0783,0x0784,0x0785,0x0788,0x0789,
	0x078a,0x078b,0x078c,0x078d,0x0790,0x0791,0x0792,0x0793,
	0x0794,0x0795,0x0798,0x0799,0x079a,0x079b,0x079c,0x079d,  
	0x07a0,0x07a1,0x07a2,0x07a3,0x07a4,0x07a5,0x07a8,0x07a9,  
	0x03aa,0x03ab,0x03ac,0x03ad,0x03b0,0x03b1,0x03b2,0x03b3,
	0x03b4,0x03b5,0x03b8,0x03b9,0x03ba,0x03bb,0x03bb
};

//
// ED: Senao's customized request.
// To verify external LNA Rx performance, Added by Roger, 2007.03.30.
//
static u32 ZEBRA2_RF_RX_GAIN_LNA_TABLE[]={  
	0,
	0x0400,0x0401,0x0402,0x0403,0x0404,0x0405,0x0408,0x0409,
	0x040a,0x040b,0x0502,0x0503,0x0504,0x0505,0x0540,0x0541,
	0x0542,0x0543,0x0544,0x0545,0x0580,0x0581,0x0582,0x0583,
	0x0584,0x0585,0x0588,0x0589,0x058a,0x058b,0x0643,0x0644,
	0x0645,0x0680,0x0681,0x0682,0x0683,0x0684,0x0685,0x0688,
	0x0689,0x068a,0x068b,0x068c,0x0742,0x0743,0x0744,0x0745,
	0x0780,0x0781,0x0782,0x0783,0x0784,0x0785,0x0788,0x0789,
	0x078a,0x078b,0x078c,0x078d,0x0790,0x0791,0x0792,0x0793,
	0x0394,0x0395,0x0398,0x0399,0x039a,0x039b,0x039c,0x039d,  
	0x03a0,0x03a1,0x03a2,0x03a3,0x03a4,0x03a5,0x03a8,0x03a9, 
	0x03aa,0x03ab,0x03ac,0x03ad,0x03b0,0x03b1,0x03b2,0x03b3,
	0x03b4,0x03b5,0x03b8,0x03b9,0x03ba,0x03bb,0x03bb
};

// Use the new SD3 given param, by shien chang, 2006.07.14
static u8 OFDM_CONFIG[]={
	// 0x00
	0x10, 0x0d, 0x01, 0x00, 0x14, 0xfb, 0xfb, 0x60, 
	0x00, 0x60, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 
			
	// 0x10
	0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0xa8, 0x26, 
	0x32, 0x33, 0x07, 0xa5, 0x6f, 0x55, 0xc8, 0xb3, 
			
	// 0x20
	0x0a, 0xe1, 0x2C, 0x8a, 0x86, 0x83, 0x34, 0x0f, 
	0x4f, 0x24, 0x6f, 0xc2, 0x6b, 0x40, 0x80, 0x00, 
			
	// 0x30
	0xc0, 0xc1, 0x58, 0xf1, 0x00, 0xe4, 0x90, 0x3e, 
	0x6d, 0x3c, 0xfb, 0x07
};

		//
		// For LNA settings, added by Roger, 2007.04.02.
		// 1. OFDM 0x05 is "0xfb".
		// 2. OFDM 0x17 is "0x56".		
		// 3. OFDM 0x1e is "0xa8".
		// 4. OFDM 0x24 is "0x86".
		//		
		
		// 
		// 2007-06-01, by Bruce: SD3 DZ solved the CCA delay probelm.
		// According to the RTL8187B PHY&MAC-Related Register Revision 9.
		// 1. OFDM[0x23]  from 0x8a to 0x4a.
		//
static u8 OFDM_CONFIG_LNA[]={
	// 0x00
	0x10, 0x0d, 0x01, 0x00, 0x14, 0xfb, 0xfb, 0x60, 
	0x00, 0x60, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 
			
	// 0x10
	0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0xa8, 0x56, 
	0x32, 0x33, 0x07, 0xa5, 0x6f, 0x55, 0xa8, 0xb3, 
			
	// 0x20
	0x0a, 0xe1, 0x2C, 0x4a, 0x86, 0x83, 0x34, 0x0f, 
	0x4f, 0x24, 0x6f, 0xc2, 0x6b, 0x40, 0x80, 0x00, 
			
	// 0x30
	0xc0, 0xc1, 0x58, 0xf1, 0x00, 0xe4, 0x90, 0x3e, 
	0x6d, 0x3c, 0xfb, 0x07
};

static struct usb_device_id rtl8187_usb_id_tbl[] = {
	{USB_DEVICE(USB_VENDOR_ID_REALTEK, 0x8187)},
	{USB_DEVICE(USB_VENDOR_ID_REALTEK, 0x8189)},
//	{USB_DEVICE_VER(USB_VENDOR_ID_REALTEK, 0x8187,0x0200,0x0200)},
	{USB_DEVICE(USB_VENDOR_ID_NETGEAR, 0x6100)},
	{USB_DEVICE(USB_VENDOR_ID_NETGEAR, 0x6a00)},
	{USB_DEVICE(0x050d, 0x705e)},
	{}
};

static char* ifname = "wlan%d";
#if 0
static int hwseqnum = 0;
static int hwwep = 0;
#endif
static int channels = 0x7ff;//0x3fff;

MODULE_LICENSE("GPL");
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
MODULE_VERSION("V 1.1");
#endif
MODULE_DEVICE_TABLE(usb, rtl8187_usb_id_tbl);
MODULE_AUTHOR("Andrea Merello <andreamrl@tiscali.it>");
MODULE_DESCRIPTION("Linux driver for Realtek RTL8187 WiFi cards");

#if 0
MODULE_PARM(ifname,"s");
MODULE_PARM_DESC(devname," Net interface name, wlan%d=default");

MODULE_PARM(hwseqnum,"i");
MODULE_PARM_DESC(hwseqnum," Try to use hardware 802.11 header sequence numbers. Zero=default");

MODULE_PARM(hwwep,"i");
MODULE_PARM_DESC(hwwep," Try to use hardware WEP support. Still broken and not available on all cards");

MODULE_PARM(channels,"i");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)
module_param(ifname, charp, S_IRUGO|S_IWUSR );
//module_param(hwseqnum,int, S_IRUGO|S_IWUSR);
//module_param(hwwep,int, S_IRUGO|S_IWUSR);
module_param(channels,int, S_IRUGO|S_IWUSR);
#else
MODULE_PARM(ifname, "s");
//MODULE_PARM(hwseqnum,"i");
//MODULE_PARM(hwwep,"i");
MODULE_PARM(channels,"i");
#endif

MODULE_PARM_DESC(devname," Net interface name, wlan%d=default");
//MODULE_PARM_DESC(hwseqnum," Try to use hardware 802.11 header sequence numbers. Zero=default");
//MODULE_PARM_DESC(hwwep," Try to use hardware WEP support. Still broken and not available on all cards");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
static int __devinit rtl8187_usb_probe(struct usb_interface *intf,
			 const struct usb_device_id *id);
static void __devexit rtl8187_usb_disconnect(struct usb_interface *intf);
#else
static void *__devinit rtl8187_usb_probe(struct usb_device *udev,unsigned int ifnum,
			 const struct usb_device_id *id);
static void __devexit rtl8187_usb_disconnect(struct usb_device *udev, void *ptr);
#endif
		 

static struct usb_driver rtl8187_usb_driver = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 15)
	.owner		= THIS_MODULE,
#endif
	.name		= RTL8187_MODULE_NAME,	          /* Driver name   */
	.id_table	= rtl8187_usb_id_tbl,	          /* PCI_ID table  */
	.probe		= rtl8187_usb_probe,	          /* probe fn      */
	.disconnect	= rtl8187_usb_disconnect,	  /* remove fn     */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 5, 0)
#ifdef CONFIG_RTL8180_PM
	.suspend	= rtl8180_suspend,	          /* PM suspend fn */
	.resume		= rtl8180_resume,                 /* PM resume fn  */
#else
	.suspend	= NULL,			          /* PM suspend fn */
	.resume      	= NULL,			          /* PM resume fn  */
#endif
#endif
};

#ifdef JOHN_HWSEC

void CAM_mark_invalid(struct net_device *dev, u8 ucIndex)
{
	u32 ulContent=0;
	u32 ulCommand=0;
	u32 ulEncAlgo=CAM_AES;

	// keyid must be set in config field
	ulContent |= (ucIndex&3) | ((u16)(ulEncAlgo)<<2);

	ulContent |= BIT15;
	// polling bit, and No Write enable, and address
	ulCommand= CAM_CONTENT_COUNT*ucIndex;
	ulCommand= ulCommand | BIT31|BIT16;
	// write content 0 is equall to mark invalid

	write_nic_dword(dev, WCAMI, ulContent);  //delay_ms(40);
	//RT_TRACE(COMP_SEC, DBG_LOUD, ("CAM_mark_invalid(): WRITE A4: %x \n",ulContent));
	write_nic_dword(dev, RWCAM, ulCommand);  //delay_ms(40);
	//RT_TRACE(COMP_SEC, DBG_LOUD, ("CAM_mark_invalid(): WRITE A0: %x \n",ulCommand));
}

void CAM_empty_entry(struct net_device *dev, u8 ucIndex)
{
	u32 ulCommand=0;
	u32 ulContent=0;
	u8 i;	
	u32 ulEncAlgo=CAM_AES;

	for(i=0;i<6;i++)
	{

		// filled id in CAM config 2 byte
		if( i == 0)
		{
			ulContent |=(ucIndex & 0x03) | ((u16)(ulEncAlgo)<<2);
			ulContent |= BIT15;
							
		}
		else
		{
			ulContent = 0;			
		}
		// polling bit, and No Write enable, and address
		ulCommand= CAM_CONTENT_COUNT*ucIndex+i;
		ulCommand= ulCommand | BIT31|BIT16;
		// write content 0 is equall to mark invalid
		write_nic_dword(dev, WCAMI, ulContent);  //delay_ms(40);
		//RT_TRACE(COMP_SEC, DBG_LOUD, ("CAM_empty_entry(): WRITE A4: %x \n",ulContent));
		write_nic_dword(dev, RWCAM, ulCommand);  //delay_ms(40);
		//RT_TRACE(COMP_SEC, DBG_LOUD, ("CAM_empty_entry(): WRITE A0: %x \n",ulCommand));
	}
}

void CamResetAllEntry(struct net_device *dev)
{
	u8 ucIndex;
	
	//2004/02/11  In static WEP, OID_ADD_KEY or OID_ADD_WEP are set before STA associate to AP.
	// However, ResetKey is called on OID_802_11_INFRASTRUCTURE_MODE and MlmeAssociateRequest
	// In this condition, Cam can not be reset because upper layer will not set this static key again.
	//if(Adapter->EncAlgorithm == WEP_Encryption)
	//	return;
//debug
	//DbgPrint("========================================\n");
	//DbgPrint("				Call ResetAllEntry						\n");
	//DbgPrint("========================================\n\n");

	for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
		CAM_mark_invalid(dev, ucIndex);
	for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
		CAM_empty_entry(dev, ucIndex);

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
#endif  /*JOHN_HWSEC*/


void write_nic_byte_E(struct net_device *dev, int indx, u8 data)
{
	int status;	
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
			       indx|0xfe00, 0, &data, 1, HZ / 2);

	if (status < 0)
	{
		printk("reg %x, write_nic_byte_E TimeOut! status:%x\n", indx, status);
	}	
}

u8 read_nic_byte_E(struct net_device *dev, int indx)
{
	int status;
	u8 data;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       RTL8187_REQ_GET_REGS, RTL8187_REQT_READ,
			       indx|0xfe00, 0, &data, 1, HZ / 2);

        if (status < 0)
        {
                printk("reg %x, read_nic_byte_E TimeOut! status:%x\n", indx, status);
        }

	return data;
}

void write_nic_byte(struct net_device *dev, int indx, u8 data)
{
	int status;
	
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
			       (indx&0xff)|0xff00, (indx>>8)&0x03, &data, 1, HZ / 2);

        if (status < 0)
        {
                printk("reg %x, write_nic_byte TimeOut! status:%x\n", indx, status);
        }
}


void write_nic_word(struct net_device *dev, int indx, u16 data)
{
	
	int status;
	
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
			       (indx&0xff)|0xff00, (indx>>8)&0x03, &data, 2, HZ / 2);

        if (status < 0)
        {
                printk("reg %x, write_nic_word TimeOut! status:%x\n", indx, status);
        }
}


void write_nic_dword(struct net_device *dev, int indx, u32 data)
{

	int status;
	
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;

	if(indx == 0x7c && (indx>>8) ==0){
		u32 databyte = data>>8;
		u8 cmdbyte = data & 0x000000ff;
		status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
			       ((indx&0xff)|0xff00)+1, 0, &databyte, 3, HZ / 2);
		if(status)
			status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
			       (indx&0xff)|0xff00, 0, &cmdbyte, 1, HZ / 2);	
	}
	else {
		status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			       RTL8187_REQ_SET_REGS, RTL8187_REQT_WRITE,
			       (indx&0xff)|0xff00, (indx>>8)&0x03, &data, 4, HZ / 2);
	}

        if (status < 0)
        {
                printk("reg %x, write_nic_dword TimeOut! status:%x\n", indx, status);
        }
}
 
 
 
u8 read_nic_byte(struct net_device *dev, int indx)
{
	u8 data;
	int status;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       RTL8187_REQ_GET_REGS, RTL8187_REQT_READ,
			       (indx&0xff)|0xff00, (indx>>8)&0x03, &data, 1, HZ / 2);
	
        if (status < 0)
        {
                printk("reg %x, read_nic_byte TimeOut! status:%x\n", indx, status);
        }
		
	return data;
}


 
u16 read_nic_word(struct net_device *dev, int indx)
{
	u16 data;
	int status;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       RTL8187_REQ_GET_REGS, RTL8187_REQT_READ,
			       (indx&0xff)|0xff00, (indx>>8)&0x03, &data, 2, HZ / 2);
	
        if (status < 0)
        {
                printk("reg %x, read_nic_word TimeOut! status:%x\n", indx, status);
        }

	return data;
}


u32 read_nic_dword(struct net_device *dev, int indx)
{
	u32 data;
	int status;
	
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct usb_device *udev = priv->udev;
	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       RTL8187_REQ_GET_REGS, RTL8187_REQT_READ,
			       (indx&0xff)|0xff00, (indx>>8)&0x03, &data, 4, HZ / 2);
	
        if (status < 0)
        {
                printk("reg %x, read_nic_dword TimeOut! status:%x\n", indx, status);
        }

	return data;
}


u8 read_phy_cck(struct net_device *dev, u8 adr);
u8 read_phy_ofdm(struct net_device *dev, u8 adr);
/* this might still called in what was the PHY rtl8185/rtl8187 common code 
 * plans are to possibilty turn it again in one common code...
 */
inline void force_pci_posting(struct net_device *dev)
{
}


//irqreturn_t rtl8180_interrupt(int irq, void *netdev, struct pt_regs *regs);
//void set_nic_rxring(struct net_device *dev);
//void set_nic_txring(struct net_device *dev);
static struct net_device_stats *rtl8180_stats(struct net_device *dev);
void rtl8180_commit(struct net_device *dev);
void rtl8180_restart(struct net_device *dev);

/****************************************************************************
   -----------------------------PROCFS STUFF-------------------------
*****************************************************************************/

static struct proc_dir_entry *rtl8180_proc = NULL;

static int proc_get_stats_ap(char *page, char **start,
                          off_t offset, int count,
                          int *eof, void *data)
{
        struct net_device *dev = data;
        struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
        struct ieee80211_device *ieee = priv->ieee80211;
        struct ieee80211_network *target;

        int len = 0;

        list_for_each_entry(target, &ieee->network_list, list) {

                len += snprintf(page + len, count - len,
                "%s ", target->ssid);

		len += snprintf(page + len, count - len,
                "%d ", (int)((jiffies-target->last_scanned)/HZ));


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

static int proc_get_registers(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n;
			
	int max=0xff;
	
	/* This dump the current register page */
	len += snprintf(page + len, count - len,
                        "\n####################page 0##################\n ");

	for(n=0;n<=max;)
	{
		//printk( "\nD: %2x> ", n);
		len += snprintf(page + len, count - len,
			"\nD:  %2x > ",n);

		for(i=0;i<16 && n<=max;i++,n++)
		len += snprintf(page + len, count - len,
			"%2x ",read_nic_byte(dev,n));

		//	printk("%2x ",read_nic_byte(dev,n));
	}
	len += snprintf(page + len, count - len,"\n");
	
	len += snprintf(page + len, count - len,
                        "\n####################page 1##################\n ");
	for(n=0;n<=36;)
        {
                len += snprintf(page + len, count - len,
                        "\nD:  %2x > ",n);

                for(i=0;i<6 && n<=max;i++,n++)
                len += snprintf(page + len, count - len,
                        "%8x ",read_cam(dev, n));
        }
	len += snprintf(page + len, count - len,"\n");
#if 0
        for(n=0;n<=max;)
        {
                //printk( "\nD: %2x> ", n);
                len += snprintf(page + len, count - len,
                        "\nD:  %2x > ",n);

                for(i=0;i<16 && n<=max;i++,n++)
                len += snprintf(page + len, count - len,
                        "%2x ",read_nic_byte(dev,0x100|n));

                //      printk("%2x ",read_nic_byte(dev,n));
        }
	
	len += snprintf(page + len, count - len,
                        "\n####################page 2##################\n ");
        for(n=0;n<=max;)
        {
                //printk( "\nD: %2x> ", n);
                len += snprintf(page + len, count - len,
                        "\nD:  %2x > ",n);

                for(i=0;i<16 && n<=max;i++,n++)
                len += snprintf(page + len, count - len,
                        "%2x ",read_nic_byte(dev,0x200|n));

                //      printk("%2x ",read_nic_byte(dev,n));
        }
#endif		
	*eof = 1;
	return len;

}


static int proc_get_cck_reg(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n;
			
	int max = 0x5F;
	
	/* This dump the current register page */
	for(n=0;n<=max;)
	{
		//printk( "\nD: %2x> ", n);
		len += snprintf(page + len, count - len,
			"\nD:  %2x > ",n);

		for(i=0;i<16 && n<=max;i++,n++)
		len += snprintf(page + len, count - len,
			"%2x ",read_phy_cck(dev,n));

		//	printk("%2x ",read_nic_byte(dev,n));
	}
	len += snprintf(page + len, count - len,"\n");

		
	*eof = 1;
	return len;

}


static int proc_get_ofdm_reg(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n;
			
	//int max=0xff;
	int max = 0x40;
	
	/* This dump the current register page */
	for(n=0;n<=max;)
	{
		//printk( "\nD: %2x> ", n);
		len += snprintf(page + len, count - len,
			"\nD:  %2x > ",n);

		for(i=0;i<16 && n<=max;i++,n++)
		len += snprintf(page + len, count - len,
			"%2x ",read_phy_ofdm(dev,n));

		//	printk("%2x ",read_nic_byte(dev,n));
	}
	len += snprintf(page + len, count - len,"\n");


		
	*eof = 1;
	return len;

}


#if 0
static int proc_get_stats_hw(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"NIC int: %lu\n"
		"Total int: %lu\n",
		priv->stats.ints,
		priv->stats.shints);
			
	*eof = 1;
	return len;
}
#endif

static int proc_get_stats_tx(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
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
		"TX BEACON queue: %d\n"
		"TX MANAGE queue: %d\n"
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
		atomic_read(&(priv->tx_pending[BEACON_PRIORITY])),
		atomic_read(&(priv->tx_pending[MANAGE_PRIORITY])),
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
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"RX packets: %lu\n"
		"RX urb status error: %lu\n"
		"RX invalid urb error: %lu\n",
		priv->stats.rxok,
		priv->stats.rxstaterr,
		priv->stats.rxurberr);
			
	*eof = 1;
	return len;
}		


static struct iw_statistics *r8180_get_wireless_stats(struct net_device *dev)
{
       struct r8180_priv *priv = ieee80211_priv(dev);

       return &priv->wstats;
}

void rtl8180_proc_module_init(void)
{	
	DMESG("Initializing proc filesystem");
	rtl8180_proc=create_proc_entry(RTL8187_MODULE_NAME, S_IFDIR, proc_net);
}


void rtl8180_proc_module_remove(void)
{
	remove_proc_entry(RTL8187_MODULE_NAME, proc_net);
}


void rtl8180_proc_remove_one(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	if (priv->dir_dev) {
	//	remove_proc_entry("stats-hw", priv->dir_dev);
		remove_proc_entry("stats-tx", priv->dir_dev);
		remove_proc_entry("stats-rx", priv->dir_dev);
	//	remove_proc_entry("stats-ieee", priv->dir_dev);
		remove_proc_entry("stats-ap", priv->dir_dev);
		remove_proc_entry("registers", priv->dir_dev);
		remove_proc_entry("cck-registers",priv->dir_dev);
		remove_proc_entry("ofdm-registers",priv->dir_dev);
		remove_proc_entry(dev->name, rtl8180_proc);
		priv->dir_dev = NULL;
	}
}


void rtl8180_proc_init_one(struct net_device *dev)
{
	struct proc_dir_entry *e;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dir_dev = create_proc_entry(dev->name, 
					  S_IFDIR | S_IRUGO | S_IXUGO, 
					  rtl8180_proc);
	if (!priv->dir_dev) {
		DMESGE("Unable to initialize /proc/net/rtl8187/%s\n",
		      dev->name);
		return;
	}
	#if 0
	e = create_proc_read_entry("stats-hw", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_hw, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8187/%s/stats-hw\n",
		      dev->name);
	}
	#endif
	e = create_proc_read_entry("stats-rx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_rx, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8187/%s/stats-rx\n",
		      dev->name);
	}
	
	
	e = create_proc_read_entry("stats-tx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_tx, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8187/%s/stats-tx\n",
		      dev->name);
	}
	#if 0
	e = create_proc_read_entry("stats-ieee", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_ieee, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8187/%s/stats-ieee\n",
		      dev->name);
	}
	
	#endif
	e = create_proc_read_entry("stats-ap", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_ap, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8187/%s/stats-ap\n",
		      dev->name);
	}
	
	
	e = create_proc_read_entry("registers", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers, dev);
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8187/%s/registers\n",
		      dev->name);
	}

	e = create_proc_read_entry("cck-registers", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_cck_reg, dev);
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8187/%s/cck-registers\n",
		      dev->name);
	}
	
	e = create_proc_read_entry("ofdm-registers", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_ofdm_reg, dev);
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8187/%s/ofdm-registers\n",
		      dev->name);
	}

}
/****************************************************************************
   -----------------------------MISC STUFF-------------------------
*****************************************************************************/

/* this is only for debugging */
static void print_buffer(u32 *buffer, int len)
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

static short check_nic_enought_desc(struct net_device *dev, priority_t priority)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	//int used = atomic_read((priority == NORM_PRIORITY) ? 
	//	&priv->tx_np_pending : &priv->tx_lp_pending);
	int used = atomic_read(&priv->tx_pending[priority]); 
	
	return (used < MAX_TX_URB);
}

static void tx_timeout(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	//rtl8180_commit(dev);
	
	printk("@@@@ Transmit timeout at %ld, latency %ld\n", jiffies,
                        jiffies - dev->trans_start);

        printk("@@@@ netif_queue_stopped = %d\n", netif_queue_stopped(dev));

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	schedule_work(&priv->reset_wq);
#else
	schedule_task(&priv->reset_wq);
#endif
	//DMESG("TXTIMEOUT");
}


/* this is only for debug */
static void dump_eprom(struct net_device *dev)
{
	int i;
	for(i=0; i<63; i++)
		DMESG("EEPROM addr %x : %x", i, eprom_read(dev,i));
}

/* this is only for debug */
static void rtl8180_dump_reg(struct net_device *dev)
{
	int i;
	int n;
	int max=0xff;
	
	DMESG("Dumping NIC register map");	
	
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


static void rtl8180_irq_enable(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);	
	//priv->irq_enabled = 1;
/*
	write_nic_word(dev,INTA_MASK,INTA_RXOK | INTA_RXDESCERR | INTA_RXOVERFLOW |\ 
	INTA_TXOVERFLOW | INTA_HIPRIORITYDESCERR | INTA_HIPRIORITYDESCOK |\ 
	INTA_NORMPRIORITYDESCERR | INTA_NORMPRIORITYDESCOK |\
	INTA_LOWPRIORITYDESCERR | INTA_LOWPRIORITYDESCOK | INTA_TIMEOUT);
*/
	write_nic_word(dev,INTA_MASK, priv->irq_mask);
}


static void rtl8180_irq_disable(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);	

	if(priv->bSurpriseRemoved == _FALSE ) 
	        write_nic_word(dev,INTA_MASK,0);
	force_pci_posting(dev);
//	priv->irq_enabled = 0;
}


void rtl8180_set_mode(struct net_device *dev,int mode)
{
	u8 ecmd;
	ecmd=read_nic_byte(dev, EPROM_CMD);
	ecmd=ecmd &~ EPROM_CMD_OPERATING_MODE_MASK;
	ecmd=ecmd | (mode<<EPROM_CMD_OPERATING_MODE_SHIFT);
	ecmd=ecmd &~ (1<<EPROM_CS_SHIFT);
	ecmd=ecmd &~ (1<<EPROM_CK_SHIFT);
	write_nic_byte(dev, EPROM_CMD, ecmd);
}


void rtl8180_update_msr(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	u8 msr;
	
	//msr  = read_nic_byte(dev, MSR);
	//msr &= ~ MSR_LINK_MASK;
	msr = MSR_LINK_ENEDCA;
	
	/* do not change in link_state != WLAN_LINK_ASSOCIATED.
	 * msr must be updated if the state is ASSOCIATING. 
	 * this is intentional and make sense for ad-hoc and
	 * master (see the create BSS/IBSS func)
	 */
	if (priv->ieee80211->state == IEEE80211_LINKED){ 
			
		if (priv->ieee80211->iw_mode == IW_MODE_INFRA)
			msr |= (MSR_LINK_MANAGED<<MSR_LINK_SHIFT);
		else if (priv->ieee80211->iw_mode == IW_MODE_ADHOC)
			msr |= (MSR_LINK_ADHOC<<MSR_LINK_SHIFT);
		else if (priv->ieee80211->iw_mode == IW_MODE_MASTER)
			msr |= (MSR_LINK_MASTER<<MSR_LINK_SHIFT);
		
	}else
		msr |= (MSR_LINK_NONE<<MSR_LINK_SHIFT);
		
	write_nic_byte(dev, MSR, msr);
}

void rtl8180_set_chan(struct net_device *dev,short ch)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);

	priv->chan=ch;
	#if 0
	if(priv->ieee80211->iw_mode == IW_MODE_ADHOC || 
		priv->ieee80211->iw_mode == IW_MODE_MASTER){
	
			priv->ieee80211->link_state = WLAN_LINK_ASSOCIATED;	
			priv->ieee80211->master_chan = ch;
			rtl8180_update_beacon_ch(dev); 
		}
	#endif
	
	priv->rf_set_chan(dev,priv->chan);
	mdelay(10);	
}
void rtl8187_rx_isr(struct urb *rx_urb, struct pt_regs *regs);
void rtl8187_rx_manage_isr(struct urb *rx_urb, struct pt_regs *regs);


void rtl8187_rx_urbsubmit(struct net_device *dev, struct urb* rx_urb)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	int err;
	
//	u8 *rx;
	
	//DMESG("starting RX");
	/*rx = kmalloc(RX_URB_SIZE*sizeof(u8),GFP_ATOMIC);
	if(!rx){ 
		DMESGE("unable to allocate RX buffer");
		return;
	}*/

	usb_fill_bulk_urb(rx_urb,priv->udev,
		usb_rcvbulkpipe(priv->udev,(NIC_8187 == priv->card_8187)?0x81:0x83), 
                rx_urb->transfer_buffer,
		RX_URB_SIZE,
                rtl8187_rx_isr,
                dev);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	err = usb_submit_urb(rx_urb, GFP_ATOMIC);	
#else
	err = usb_submit_urb(rx_urb);	
#endif
	if(err && err != -EPERM){
		DMESGE("cannot submit RX command. URB_STATUS %x(%d)",rx_urb->status,rx_urb->status);
	}
}


void rtl8187_rx_manage_urbsubmit(struct net_device *dev, struct urb* rx_urb)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	int err;
#ifdef THOMAS_BEACON
	usb_fill_bulk_urb(rx_urb,priv->udev,
		usb_rcvbulkpipe(priv->udev,0x09), rx_urb->transfer_buffer,
			rx_urb->transfer_buffer_length,
			rtl8187_rx_manage_isr, dev);
#else
	usb_fill_bulk_urb(rx_urb,priv->udev,
		usb_rcvbulkpipe(priv->udev,0x09), rx_urb->transfer_buffer,
			RX_URB_SIZE, rtl8187_rx_manage_isr,dev);

#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	err = usb_submit_urb(rx_urb, GFP_ATOMIC);	
#else
	err = usb_submit_urb(rx_urb);	
#endif
	if(err && err != -EPERM){
		DMESGE("cannot submit RX command. URB_STATUS %x(%d)",rx_urb->status,rx_urb->status);
	}
}


#ifndef JACKSON_NEW_RX
void rtl8187_rx_initiate(struct net_device *dev)
{
	int i;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	if(!priv->rx_urb)
		DMESGE("Cannot intiate RX urb mechanism");
	for(i=0;i<MAX_RX_URB;i++) // RX_MAX_URB is 1 
		rtl8187_rx_urbsubmit(dev,priv->rx_urb[i]);
		
}
#else
void rtl8187_rx_initiate(struct net_device *dev)
{
	int i;
	unsigned long flags;
#ifndef THOMAS_PCLINUX
	u32 Tmpaddr;
	int alignment;
#endif
        struct urb *purb;
        struct sk_buff *pskb;

        struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);

        priv->tx_urb_index = 0;

        if ((!priv->rx_urb) || (!priv->pp_rxskb)) {

                DMESGE("Cannot intiate RX urb mechanism");
                return;

        }

        priv->rx_inx = 0;
#ifdef THOMAS_TASKLET
	atomic_set(&priv->irt_counter,0);
//	priv->irt_counter_head = 0;
//	priv->irt_counter_tail = 0;
#endif


        for(i = 0;i < MAX_RX_URB; i++){

                purb = priv->rx_urb[i] = usb_alloc_urb(0,GFP_KERNEL);

                if(!priv->rx_urb[i])
                        goto destroy;
#ifndef THOMAS_PCLINUX
		pskb = priv->pp_rxskb[i] =  dev_alloc_skb (RX_URB_SIZE_ALIGN);
#else
                pskb = priv->pp_rxskb[i] =  dev_alloc_skb (RX_URB_SIZE);
#endif

                if (pskb == NULL)
                        goto destroy;

#ifndef THOMAS_PCLINUX
		// 512 bytes alignment for DVR hw access bug, by thomas 09-03-2007
                Tmpaddr = (u32)pskb->data;
                alignment = Tmpaddr & 0x1ff;
                skb_reserve(pskb,(512-alignment));
                purb->transfer_buffer_length = RX_URB_SIZE_ALIGN - (512-alignment); // dev_alloc_skb reserve 16 bytes for header
#else
		purb->transfer_buffer_length = RX_URB_SIZE;
#endif
                purb->transfer_buffer = pskb->data;

        }

	spin_lock_irqsave(&priv->irq_lock,flags);//added by thomas

        for(i=0;i<MAX_RX_URB;i++)
                rtl8187_rx_urbsubmit(dev,priv->rx_urb[i]);

	spin_unlock_irqrestore(&priv->irq_lock,flags);//added by thomas
        
	return;

destroy:

        for(i = 0; i < MAX_RX_URB; i++) {

                purb = priv->rx_urb[i];

                if (purb)
                        usb_free_urb(purb);

                pskb = priv->pp_rxskb[i];

                if (pskb)
                        dev_kfree_skb_any(pskb);

        }

        return;
}
#endif


void rtl8187_rx_manage_initiate(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	if(!priv->rx_urb)
		DMESGE("Cannot intiate RX urb mechanism");

	rtl8187_rx_manage_urbsubmit(dev,priv->rx_urb[MAX_RX_URB]);
		
}


void rtl8187_set_rxconf(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u32 rxconf;
	
	rxconf=read_nic_dword(dev,RX_CONF);
	rxconf = rxconf &~ MAC_FILTER_MASK;
	rxconf = rxconf | (1<<ACCEPT_MNG_FRAME_SHIFT);
	rxconf = rxconf | (1<<ACCEPT_DATA_FRAME_SHIFT);
	rxconf = rxconf | (1<<ACCEPT_BCAST_FRAME_SHIFT);
	rxconf = rxconf | (1<<ACCEPT_MCAST_FRAME_SHIFT);
	//rxconf = rxconf | (1<<ACCEPT_CTL_FRAME_SHIFT);	

	if (dev->flags & IFF_PROMISC) DMESG ("NIC in promisc mode");
	
	if(priv->ieee80211->iw_mode == IW_MODE_MONITOR || \
	   dev->flags & IFF_PROMISC){
		rxconf = rxconf | (1<<ACCEPT_ALLMAC_FRAME_SHIFT);
	} /*else if(priv->ieee80211->iw_mode == IW_MODE_MASTER){
		rxconf = rxconf | (1<<ACCEPT_ALLMAC_FRAME_SHIFT);
		rxconf = rxconf | (1<<RX_CHECK_BSSID_SHIFT);
	}*/else{
		rxconf = rxconf | (1<<ACCEPT_NICMAC_FRAME_SHIFT);
		rxconf = rxconf | (1<<RX_CHECK_BSSID_SHIFT);
	}
	
	
	if(priv->ieee80211->iw_mode == IW_MODE_MONITOR){
		rxconf = rxconf | (1<<ACCEPT_ICVERR_FRAME_SHIFT);
		rxconf = rxconf | (1<<ACCEPT_PWR_FRAME_SHIFT);
	}
	
	if( priv->crcmon == 1 && priv->ieee80211->iw_mode == IW_MODE_MONITOR)
		rxconf = rxconf | (1<<ACCEPT_CRCERR_FRAME_SHIFT);
	
	
	rxconf = rxconf &~ RX_FIFO_THRESHOLD_MASK;
	rxconf = rxconf | (RX_FIFO_THRESHOLD_NONE<<RX_FIFO_THRESHOLD_SHIFT);
	rxconf = rxconf &~ MAX_RX_DMA_MASK;
	rxconf = rxconf | (MAX_RX_DMA_2048<<MAX_RX_DMA_SHIFT);

	rxconf = rxconf | (1<<RX_AUTORESETPHY_SHIFT);
	rxconf = rxconf | RCR_ONLYERLPKT;
	
//	rxconf = rxconf &~ RCR_CS_MASK;
//	rxconf = rxconf | (1<<RCR_CS_SHIFT);

	write_nic_dword(dev, RX_CONF, rxconf);	
	
	#ifdef DEBUG_RX
	DMESG("rxconf: %x %x",rxconf ,read_nic_dword(dev,RX_CONF));
	#endif
}

void rtl8180_rx_enable(struct net_device *dev)
{
	u8 cmd;

	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);

	

	if(NIC_8187 == priv->card_8187) {
		rtl8187_set_rxconf(dev);	
		cmd=read_nic_byte(dev,CMD);
		write_nic_byte(dev,CMD,cmd | (1<<CMD_RX_ENABLE_SHIFT));
		rtl8187_rx_initiate(dev);
	}
	else {
		rtl8187_rx_initiate(dev);
		rtl8187_rx_manage_initiate(dev);
		//write_nic_dword(dev, RCR, priv->ReceiveConfig);
	}
}


void rtl8180_tx_enable(struct net_device *dev)
{
	u8 cmd;
	u8 byte;
	u32 txconf;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	//test loopback
	//	priv->TransmitConfig |= (TX_LOOPBACK_BASEBAND<<TX_LOOPBACK_SHIFT);
	if(NIC_8187B == priv->card_8187){
		//write_nic_dword(dev, TCR, priv->TransmitConfig);	
		//byte = read_nic_byte(dev, MSR);
		//byte |= MSR_LINK_ENEDCA;
		//write_nic_byte(dev, MSR, byte);
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
}

#if 0
void rtl8180_beacon_tx_enable(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask &=~(1<<TX_DMA_STOP_BEACON_SHIFT);
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);	
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
}


void rtl8180_
_disable(struct net_device *dev) 
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_BEACON_SHIFT);
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
}

#endif


void rtl8180_rtx_disable(struct net_device *dev)
{
	u8 cmd;
	int i,index;
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	if(priv->bSurpriseRemoved == _FALSE) {
	        cmd=read_nic_byte(dev,CMD);
	        write_nic_byte(dev, CMD, cmd &~ \
		       ((1<<CMD_RX_ENABLE_SHIFT)|(1<<CMD_TX_ENABLE_SHIFT)));
	        force_pci_posting(dev);
	        mdelay(10);
	}

#ifndef JACKSON_NEW_RX

        if(priv->rx_urb){
                for(i=0;i<MAX_RX_URB+1;i++)
                usb_kill_urb(priv->rx_urb[i]);
        }
#endif

	/*while (read_nic_byte(dev,CMD) & (1<<CMD_RX_ENABLE_SHIFT))
	  udelay(10); 
	*/
	
//	if(!priv->rx_skb_complete)
//		dev_kfree_skb_any(priv->rx_skb);

#ifdef THOMAS_BEACON
	i=0;
	index= priv->rx_inx;
	if(priv->rx_urb){
		if(priv->rx_urb[MAX_RX_URB])
			usb_kill_urb(priv->rx_urb[MAX_RX_URB]);

		while(i<MAX_RX_URB){
			if(priv->rx_urb[index]){
				usb_kill_urb(priv->rx_urb[index]);
			}
			if( index == (MAX_RX_URB-1) )
				index=0;
			else
				index=index+1;
			i++;
		}
        }
#else
#ifdef JACKSON_NEW_RX
        if(priv->rx_urb){
               for(i=0;i<(MAX_RX_URB+1);i++) {
                   if (priv->rx_urb[i]) {
                                usb_kill_urb(priv->rx_urb[i]);
                                usb_free_urb(priv->rx_urb[i]);
                        }
                }
        }
#endif
#endif
}

#if 0
static int alloc_tx_beacon_desc_ring(struct net_device *dev, int count)
{

	int i;
	u32 *tmp;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	priv->txbeaconring = (u32*)pci_alloc_consistent(priv->pdev,
					  sizeof(u32)*8*count, 
					  &priv->txbeaconringdma);
	if (!priv->txbeaconring) return -1;
	for (tmp=priv->txbeaconring,i=0;i<count;i++){
		*tmp = *tmp &~ (1<<31); // descriptor empty, owned by the drv 
		/*
		*(tmp+2) = (u32)dma_tmp;
		*(tmp+3) = bufsize;
		*/
		if(i+1<count)
			*(tmp+4) = (u32)priv->txbeaconringdma+((i+1)*8*4);
		else
			*(tmp+4) = (u32)priv->txbeaconringdma;
		
		tmp=tmp+8;
	}

	return 0;
}
#endif

void rtl8180_reset(struct net_device *dev)
{
	
	struct r8180_priv *priv = ieee80211_priv(dev);
	u8 cr;
	int i;

	/* make sure the analog power is on before
	 * reset, otherwise reset may fail
	 */
	if(NIC_8187 == priv->card_8187) {
		rtl8180_set_anaparam(dev, RTL8225_ANAPARAM_ON);
		rtl8185_set_anaparam2(dev, RTL8225_ANAPARAM2_ON);
		rtl8180_irq_disable(dev);
		mdelay(200);
		write_nic_byte_E(dev,0x18,0x10);
		write_nic_byte_E(dev,0x18,0x11);
		write_nic_byte_E(dev,0x18,0x00);
		mdelay(200);
	}
	//for 87b surpriseremoved
	if(priv->bSurpriseRemoved == _FALSE) {
	
	cr=read_nic_byte(dev,CMD);
	cr = cr & 2;
	cr = cr | (1<<CMD_RST_SHIFT);
	write_nic_byte(dev,CMD,cr);
	
	force_pci_posting(dev);
	
	mdelay(200);
	
	if(read_nic_byte(dev,CMD) & (1<<CMD_RST_SHIFT)) 
		DMESGW("Card reset timeout!");
	else 
		DMESG("Card successfully reset");
	}
	
	if(NIC_8187 == priv->card_8187) {

		printk("This is RTL8187 Reset procedure\n");
		rtl8180_set_mode(dev,EPROM_CMD_LOAD);
		force_pci_posting(dev);
		mdelay(200);

		/* after the eeprom load cycle, make sure we have
		 * correct anaparams
		 */
		rtl8180_set_anaparam(dev, RTL8225_ANAPARAM_ON);
		rtl8185_set_anaparam2(dev, RTL8225_ANAPARAM2_ON);
	}
	else {
		printk("This is RTL8187B Reset procedure\n");
		
		//test pending bug, john 20070815 
                //initialize tx_pending
                for(i=0;i<0x10;i++)  atomic_set(&(priv->tx_pending[i]), 0);
	}

}

static inline u16 ieeerate2rtlrate(int rate)
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
inline u16 rtl8180_rate2rate(short rate)
{
	if (rate >11) return 0;
	return rtl_rate[rate]; 
}
		
void rtl8180_irq_rx_tasklet(struct r8180_priv *priv);

void rtl8187_rx_isr(struct urb *rx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)rx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);
	priv->rxurb_task = rx_urb;
	
	
//	DMESGW("David: Rx tasklet start!");
#ifndef JACKSON_NEW_RX
	tasklet_schedule(&priv->irq_rx_tasklet);
#else

#ifdef THOMAS_TASKLET
	atomic_inc( &priv->irt_counter );
	//if( likely(priv->irt_counter_head +1 != priv->irt_counter_tail) ){
	//	priv->irt_counter_head = (priv->irt_counter_head+1)&0xffff;
		tasklet_schedule(&priv->irq_rx_tasklet);
	//}
	//else
	//	DMESG("error: priv->irt_counter_head is going to pass through priv->irt_counter_tail\n");
#else
	rtl8180_irq_rx_tasklet(priv);
#endif

#endif
//	DMESGW("=David: Rx tasklet finish!");
}

void rtl8187_rx_manage_isr(struct urb *rx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)rx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);
	int status,cmd;
	struct sk_buff *skb;
	u32 *desc;
	int ret;

	//DMESG("rtl8187_rx_manage_isr");
	//DMESG("RX %d ",rx_urb->status);
	status = rx_urb->status;
	if(status == 0){
		
		desc = (u32*)(rx_urb->transfer_buffer);
		cmd = (desc[0] >> 30) & 0x03;
		
		if(cmd == 0x00) {//beacon interrupt
			//send beacon packet
			skb = ieee80211_get_beacon(priv->ieee80211);

			if(!skb){ 
				DMESG("not enought memory for allocating beacon");
				return;
			}
			//printk(KERN_WARNING "to send beacon packet through beacon endpoint!\n");
			ret = rtl8180_tx(dev, (u32*)skb->data, skb->len, BEACON_PRIORITY,
					0, ieeerate2rtlrate(priv->ieee80211->basic_rate));
			
			if( ret != 0 ){
				printk(KERN_ALERT "tx beacon packet error : %d !\n", ret);
			}	
			dev_kfree_skb_any(skb);
		} else {//0x01 Tx Close Descriptor
			//read tx close descriptor
#ifdef THOMAS_RATEADAPTIVE
			priv->CurrRetryCnt += (u16)(desc[0]&0xff);
#endif
		}

	}else{
		priv->stats.rxstaterr++;
		priv->ieee80211->stats.rx_errors++;
	}

//	if(status == 0 )
	//if(status != -ENOENT)
//		rtl8187_rx_manage_urbsubmit(dev, rx_urb);
//	else 
//		DMESG("Mangement RX process aborted due to explicit shutdown (%x)(%d)", status, status);
	if(status == 0 )
		rtl8187_rx_manage_urbsubmit(dev, rx_urb);
	else {
		switch(status) {
			case -EINVAL:
			case -EPIPE:
			case -ENODEV:
			case -ESHUTDOWN:
				priv->bSurpriseRemoved = _TRUE;
			case -ENOENT:
		DMESG("Mangement RX process aborted due to explicit shutdown (%x)(%d)", status, status);
				break;
			case -EINPROGRESS:
				panic("URB IS IN PROGRESS!/n");
				break;
			default:
				rtl8187_rx_manage_urbsubmit(dev, rx_urb);
				break;
		}
	}
}

#if 0
void rtl8180_tx_queues_stop(struct net_device *dev)
{
	//struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u8 dma_poll_mask = (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_HIPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_NORMPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_BEACON_SHIFT);

	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,dma_poll_mask);
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
}
#endif

void rtl8180_data_hard_stop(struct net_device *dev)
{
	//FIXME !!
	#if 0
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}


void rtl8180_data_hard_resume(struct net_device *dev)
{
	// FIXME !!
	#if 0
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask &= ~(1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}

unsigned int PRI2EP[4] = {0x06,0x07,0x05,0x04};
/* this function TX data frames when the ieee80211 stack requires this.
 * It checks also if we need to stop the ieee tx queue, eventually do it
 */
void rtl8180_hard_data_xmit(struct sk_buff *skb, struct net_device *dev, int rate)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	short morefrag = 0;	
	unsigned long flags;
	struct ieee80211_hdr *h = (struct ieee80211_hdr  *) skb->data;

	unsigned char ep;
	short ret;

	if (le16_to_cpu(h->frame_ctl) & IEEE80211_FCTL_MOREFRAGS)
		morefrag = 1;
//	DMESG("%x %x", h->frame_ctl, h->seq_ctl);
	/*
	* This function doesn't require lock because we make
	* sure it's called with the tx_lock already acquired.
	* this come from the kernel's hard_xmit callback (trought
	* the ieee stack, or from the try_wake_queue (again trought
	* the ieee stack.
	*/
	spin_lock_irqsave(&priv->tx_lock,flags);	
		
	//DMESG("TX");
	if(NIC_8187B == priv->card_8187){
		ep = PRI2EP[skb->priority];
	} else {
		ep = LOW_PRIORITY;
	}
	//if (!check_nic_enought_desc(dev, PRI2EP[skb->priority])){
	if (!check_nic_enought_desc(dev, ep)){
		DMESG("Error: no TX slot ");
		ieee80211_stop_queue(priv->ieee80211);
	}
	
	//printk(KERN_WARNING "data rate = %d\n",rate);
	//rtl8180_tx(dev, (u32*)skb->data, skb->len, PRI2EP[skb->priority], morefrag,
	ret = rtl8180_tx(dev, (u32*)skb->data, skb->len, ep, morefrag,
                ieeerate2rtlrate(rate));
	if(ret) DMESG("Error: rtl8180_tx failed in rtl8180_hard_data_xmit\n");//john

	priv->stats.txdatapkt++;
	
	//if (!check_nic_enought_desc(dev, PRI2EP[skb->priority])){

	if (!check_nic_enought_desc(dev, ep)){
		ieee80211_stop_queue(priv->ieee80211);
	}
		
	spin_unlock_irqrestore(&priv->tx_lock,flags);	
			
}

/* This is a rough attempt to TX a frame
 * This is called by the ieee 80211 stack to TX management frames.
 * If the ring is full packet are dropped (for data frame the queue
 * is stopped before this can happen).
 */
int rtl8180_hard_start_xmit(struct sk_buff *skb,struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	int ret;
	unsigned long flags;
#if 0	
	struct ieee80211_hdr_3addr *frag_packet = (struct ieee80211_hdr_3addr*)skb->data;
#define BEACON_PACKET(fc)  ((fc&0xf0) == 0x80)
#endif	
	spin_lock_irqsave(&priv->tx_lock,flags);

#if 0	
	ret = rtl8180_tx(dev, (u32*)skb->data, skb->len, BEACON_PACKET(frag_packet->frame_ctl)? 
			BEACON_PRIORITY:MANAGE_PRIORITY, 0, ieeerate2rtlrate(ieee->basic_rate));
#endif	
	ret = rtl8180_tx(dev, (u32*)skb->data, skb->len, MANAGE_PRIORITY, 0, ieeerate2rtlrate(ieee->basic_rate));
//	ret = rtl8180_tx(dev, (u32*)skb->data, skb->len, BE_PRIORITY, 0, ieeerate2rtlrate(ieee->basic_rate));
/*
	int i;
	for(i=0;i<skb->len;i++)
		printk("%x ", skb->data[i]);
	printk("--------------------\n");
*/
	priv->ieee80211->stats.tx_bytes+=skb->len;
	priv->ieee80211->stats.tx_packets++;
	
	spin_unlock_irqrestore(&priv->tx_lock,flags);	
	
//	dev_kfree_skb_any(skb);
	return ret;
}


#if 0
// longpre 144+48 shortpre 72+24
u16 rtl8180_len2duration(u32 len, short rate,short* ext)
{
	u16 duration;
	u16 drift;
	*ext=0;
	
	switch(rate){
	case 0://1mbps
		*ext=0;
		duration = ((len+4)<<4) /0x2;
		drift = ((len+4)<<4) % 0x2;
		if(drift ==0 ) break;
		duration++;
		break;
		
	case 1://2mbps
		*ext=0;
		duration = ((len+4)<<4) /0x4;
		drift = ((len+4)<<4) % 0x4;
		if(drift ==0 ) break;
		duration++;
		break;
		
	case 2: //5.5mbps
		*ext=0;
		duration = ((len+4)<<4) /0xb;
		drift = ((len+4)<<4) % 0xb;
		if(drift ==0 ) 
			break;
		duration++;
		break;
		
	default:
	case 3://11mbps				
		*ext=0;
		duration = ((len+4)<<4) /0x16;
		drift = ((len+4)<<4) % 0x16;
		if(drift ==0 ) 
			break;
		duration++;
		if(drift > 6) 
			break;
		*ext=1;
		break;
	}
	
	return duration;
}
#endif

void rtl8180_try_wake_queue(struct net_device *dev, int pri);

void rtl8187_lptx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)tx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(tx_urb->status == 0){
		dev->trans_start = jiffies;     //john
		priv->stats.txlpokint++;
#ifdef THOMAS_RATEADAPTIVE
		priv->stats.NumTxOkTotal++;
		priv->stats.NumTxOkBytesTotal += tx_urb->actual_length - 8;
#endif
	}
	else
		priv->stats.txlperr++;
	kfree(tx_urb->transfer_buffer);
	usb_free_urb(tx_urb);
	atomic_dec(&priv->tx_pending[LOW_PRIORITY]);
	rtl8180_try_wake_queue(dev,LOW_PRIORITY);
}

void rtl8187_nptx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)tx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(tx_urb->status == 0){
		dev->trans_start = jiffies;     //john
		priv->stats.txnpokint++;
#ifdef THOMAS_RATEADAPTIVE
		priv->stats.NumTxOkTotal++;
		priv->stats.NumTxOkBytesTotal += tx_urb->actual_length - 8;
#endif
	}
	else
		priv->stats.txnperr++;
	kfree(tx_urb->transfer_buffer);
	usb_free_urb(tx_urb);
	atomic_dec(&priv->tx_pending[NORM_PRIORITY]);
//	rtl8180_try_wake_queue(dev,NORM_PRIORITY);
}

void rtl8187_votx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)tx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(tx_urb->status == 0){
		dev->trans_start = jiffies;     //john
		priv->stats.txvookint++;
#ifdef THOMAS_RATEADAPTIVE
		priv->stats.NumTxOkTotal++;
		priv->stats.NumTxOkBytesTotal += tx_urb->actual_length - 28;
#endif
	}
	else
		priv->stats.txvoerr++;
	kfree(tx_urb->transfer_buffer);
	usb_free_urb(tx_urb);
	atomic_dec(&priv->tx_pending[VO_PRIORITY]);
	rtl8180_try_wake_queue(dev,VO_PRIORITY);
}

void rtl8187_vitx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)tx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(tx_urb->status == 0){
		dev->trans_start = jiffies;     //john
		priv->stats.txviokint++;
#ifdef THOMAS_RATEADAPTIVE
		priv->stats.NumTxOkTotal++;
		priv->stats.NumTxOkBytesTotal += tx_urb->actual_length - 28;
#endif
	}
	else
		priv->stats.txvierr++;
	kfree(tx_urb->transfer_buffer);
	usb_free_urb(tx_urb);
	atomic_dec(&priv->tx_pending[VI_PRIORITY]);
	rtl8180_try_wake_queue(dev,VI_PRIORITY);
}

void rtl8187_betx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)tx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(tx_urb->status == 0){
		dev->trans_start = jiffies;     //john
		priv->stats.txbeokint++;
#ifdef THOMAS_RATEADAPTIVE
		priv->stats.NumTxOkTotal++;
		priv->stats.NumTxOkBytesTotal += tx_urb->actual_length - 28;
#endif
	}
	else
		priv->stats.txbeerr++;
	kfree(tx_urb->transfer_buffer);
	usb_free_urb(tx_urb);
	atomic_dec(&priv->tx_pending[BE_PRIORITY]);
	rtl8180_try_wake_queue(dev, BE_PRIORITY);
}

void rtl8187_bktx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)tx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(tx_urb->status == 0){
		dev->trans_start = jiffies;     //john
		priv->stats.txbkokint++;
#ifdef THOMAS_RATEADAPTIVE
		priv->stats.NumTxOkTotal++;
		priv->stats.NumTxOkBytesTotal += tx_urb->actual_length - 28;
#endif
	}
	else
		priv->stats.txbkerr++;
	kfree(tx_urb->transfer_buffer);
	usb_free_urb(tx_urb);
	atomic_dec(&priv->tx_pending[BK_PRIORITY]);
	rtl8180_try_wake_queue(dev,BK_PRIORITY);
}
void rtl8187_beacontx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)tx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	if(tx_urb->status == 0){
		dev->trans_start = jiffies;     //john
		priv->stats.txbeaconokint++;
#ifdef THOMAS_RATEADAPTIVE
		priv->stats.NumTxOkTotal++;
		priv->stats.NumTxOkBytesTotal += tx_urb->actual_length - 28;
#endif
	}
	else
		priv->stats.txbeaconerr++;
	kfree(tx_urb->transfer_buffer);
	usb_free_urb(tx_urb);
	atomic_dec(&priv->tx_pending[BEACON_PRIORITY]);
	//rtl8180_try_wake_queue(dev,BEACON_PRIORITY);
}

void rtl8187_managetx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)tx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(tx_urb->status == 0){
		dev->trans_start = jiffies;     //john
		priv->stats.txmanageokint++;
#ifdef THOMAS_RATEADAPTIVE
		priv->stats.NumTxOkTotal++;
		priv->stats.NumTxOkBytesTotal += tx_urb->actual_length - 28;
#endif
	}
	else
		priv->stats.txmanageerr++;
	kfree(tx_urb->transfer_buffer);
	usb_free_urb(tx_urb);
	atomic_dec(&priv->tx_pending[MANAGE_PRIORITY]);
	//rtl8180_try_wake_queue(dev,MANAGE_PRIORITY);
}

void rtl8187_beacon_stop(struct net_device *dev)
{
	u8 msr, msrm, msr2;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(priv->bSurpriseRemoved == _FALSE ) {
	msr  = read_nic_byte(dev, MSR);
	msrm = msr & MSR_LINK_MASK;
	msr2 = msr & ~MSR_LINK_MASK;

		//if(NIC_8187B == priv->card_8187) {
		//if(priv->rx_urb[MAX_RX_URB])
			//usb_kill_urb(priv->rx_urb[MAX_RX_URB]);
		//}
	if ((msrm == (MSR_LINK_ADHOC<<MSR_LINK_SHIFT) ||
		(msrm == (MSR_LINK_MASTER<<MSR_LINK_SHIFT)))){
		write_nic_byte(dev, MSR, msr2 | MSR_LINK_NONE);
		write_nic_byte(dev, MSR, msr);	
	}
}
}


void rtl8187_net_update(struct net_device *dev)
{

	struct r8180_priv *priv = ieee80211_priv(dev);
	struct ieee80211_network *net;
	net = & priv->ieee80211->current_network;
	
	if((priv->ieee80211->state != IEEE80211_LINKED_SCANNING)&&(priv->ieee80211->scanning !=1)&&(priv->MPState!=1)){
		write_nic_dword(dev,BSSID,((u32*)net->bssid)[0]);
		write_nic_word(dev,BSSID+4,((u16*)net->bssid)[2]);
	}
	//for(i=0;i<ETH_ALEN;i++)
	//	write_nic_byte(dev,BSSID+i,net->bssid[i]);

		
//	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_word(dev, AtimWnd, 2);
	//write_nic_word(dev, AtimtrItv, 100);	
	write_nic_word(dev, AtimtrItv, 80);	
//	write_nic_word(dev, BEACON_INTERVAL, net->beacon_interval);
	write_nic_word(dev, BEACON_INTERVAL, 99);
	write_nic_word(dev, BcnIntTime, 0x3FF);
	write_nic_byte(dev, BcnInt, 1);
	
	rtl8180_update_msr(dev);
}

void rtl8187_beacon_tx(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	struct sk_buff *skb;
	int i = 0;
	u8 cr;

	//clear MSR first
	priv->ieee80211->state = IEEE80211_NOLINK;
	rtl8180_update_msr(dev);
	priv->ieee80211->state = IEEE80211_LINKED;
	rtl8187_net_update(dev);
	//priv->ieee80211->rate = 360;

	if(NIC_8187B == priv->card_8187) {
		ActSetWirelessMode8187(dev, (u8)(priv->ieee80211->mode));
		//Cause TSF timer of MAC reset to 0
		cr=read_nic_byte(dev,CMD);
		cr = (cr & (CR_RE|CR_TE)) | CR_ResetTSF;
		//cr = cr | (1<<CMD_RST_SHIFT);
		write_nic_byte(dev,CMD,cr);
		//mdelay(200);

		DMESG("Card successfully reset for ad-hoc");

		//write_nic_byte(dev,CMD, (read_nic_byte(dev,CMD)|CR_RE|CR_TE));

		//	rtl8187_rx_manage_initiate(dev);
	} else {
		skb = ieee80211_get_beacon(priv->ieee80211);
		if(!skb){ 
			DMESG("not enought memory for allocating beacon");
			return;
		}

#if 0	
		while(MAX_TX_URB!=atomic_read(&priv->tx_np_pending)){
			msleep_interruptible_rtl(HZ/2);
			if(i++ > 20){
				DMESG("get stuck to wait EP3 become ready");
				return ;
			}
		}
#endif
		write_nic_byte(dev, BQREQ, read_nic_byte(dev, BQREQ) | (1<<7));

		i=0;
		//while(!read_nic_byte(dev,BQREQ & (1<<7)))
		while( (read_nic_byte(dev, BQREQ) & (1<<7)) == 0 )
		{
			msleep_interruptible_rtl(HZ/2);
			if(i++ > 10){
				DMESGW("get stuck to wait HW beacon to be ready");
				return ;
			}
		}
		//tx 	
		rtl8180_tx(dev, (u32*)skb->data, skb->len, NORM_PRIORITY,
				0, ieeerate2rtlrate(priv->ieee80211->basic_rate));
		if(skb)
			dev_kfree_skb_any(skb);
	}
}
#if 0
void rtl8187_nptx_isr(struct urb *tx_urb, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device*)tx_urb->context;
	struct r8180_priv *priv = ieee80211_priv(dev);

	if(tx_urb->status == 0)
		priv->stats.txnpokint++;
	else
		priv->stats.txnperr++;
	kfree(tx_urb->transfer_buffer);
	usb_free_urb(tx_urb);
	atomic_dec(&priv->tx_np_pending);
	//rtl8180_try_wake_queue(dev,NORM_PRIORITY);
}
#endif
inline u8 rtl8180_IsWirelessBMode(u16 rate)
{
	if( ((rate <= 110) && (rate != 60) && (rate != 90)) || (rate == 220) )
		return 1;
	else return 0;
}

static u16 N_DBPSOfRate(u16 DataRate);

static u16 ComputeTxTime( 
	u16		FrameLength,
	u16		DataRate,
	u8		bManagementFrame,
	u8		bShortPreamble
)
{
	u16	FrameTime;
	u16	N_DBPS;
	u16	Ceiling;

	if( rtl8180_IsWirelessBMode(DataRate) )
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

static u16 N_DBPSOfRate(u16 DataRate)
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
// NOte!!!
// the rate filled in is the rtl_rate.
// while the priv->ieee80211->basic_rate,used in the following code is ieee80211 rate.
short rtl8180_tx(struct net_device *dev, u32* txbuf, int len, priority_t priority,
		 short morefrag, short rate)
{
	u32 *tx;
	//	u16 duration;
	//	short ext;
	int pend ;
	int status;
	struct urb *tx_urb;
	int urb_len;	
	struct r8180_priv *priv = ieee80211_priv(dev);
	struct ieee80211_hdr_3addr_QOS *frag_hdr = (struct ieee80211_hdr_3addr_QOS *)txbuf;
	struct ieee80211_device *ieee;//added for descriptor
	u8 dest[ETH_ALEN];

	_BOOL			bUseShortPreamble = _FALSE;
	_BOOL			bCTSEnable = _FALSE;
	_BOOL			bRTSEnable = _FALSE;
	//u16			RTSRate = 22;
	//u8			RetryLimit = 0;
	u16 			Duration = 0; 
	u16			RtsDur = 0;
	u16			ThisFrameTime = 0;	
	u16			TxDescDuration = 0;
#ifndef THOMAS_PCLINUX
	u32			Tmpaddr = 0;
	int			alignment = 0;
#endif
	u16 ethertype;
	u8 *pos;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)txbuf;
	u16 fc = le16_to_cpu(hdr->frame_ctl);
	size_t hdrlen = ieee80211_get_hdrlen(fc);

	ieee = priv->ieee80211;
	//	pend = atomic_read((priority == NORM_PRIORITY)? &priv->tx_np_pending : &priv->tx_lp_pending);
	pend = atomic_read(&priv->tx_pending[priority]);

	pos = (u8*)hdr + hdrlen;
	ethertype = (pos[6] << 8) | pos[7];
	if (ethertype == ETH_P_PAE)
	{
		rate = ieeerate2rtlrate(ieee->basic_rate);
	}

	//for test
	//int 			i;

	//}	int rate = ieeerate2rtlrate(priv->ieee80211->rate);


	/* we are locked here so the two atomic_read and inc are executed without interleaves */
	if( pend > MAX_TX_URB){
		if(NIC_8187 == priv->card_8187) {
			if(priority == NORM_PRIORITY)
				priv->stats.txnpdrop++;
			else
				priv->stats.txlpdrop++;

		} else {
			switch (priority) {
				case VO_PRIORITY:
					priv->stats.txvodrop++;
					break;
				case VI_PRIORITY:
					priv->stats.txvidrop++;
					break;
				case BE_PRIORITY:
					priv->stats.txbedrop++;
					break;
				case BEACON_PRIORITY:
					priv->stats.txbeacondrop++;
					break;
				case MANAGE_PRIORITY:
					priv->stats.txmanagedrop++;
					break;
				default://BK_PRIORITY
					priv->stats.txbkdrop++;
					break;	
			}
		}
		printk(KERN_INFO "tx_pending > MAX_TX_URB\n");
		return -1;
	}

	urb_len = len + ((NIC_8187 == priv->card_8187)?(4*3):(4*8));
	if(( 0 == (urb_len&63) )||( 0 == (urb_len&511) )) {
//	if((0 == urb_len%64)||(0 == urb_len%512)) {
		urb_len += 1;	  
	}

#ifndef THOMAS_PCLINUX
        // 512 bytes alignment for DVR hw access bug, by thomas 09-03-2007
        tx = kmalloc(urb_len+512, GFP_ATOMIC);
        if(!tx) return -ENOMEM;        
	Tmpaddr= (u32)tx;
        alignment = Tmpaddr & 0x1ff;
        if( alignment )
                Tmpaddr += 512 - alignment;
        tx = (u32*)Tmpaddr;
#else
	tx = kmalloc(urb_len, GFP_ATOMIC);
	if(!tx) return -ENOMEM;
#endif
	memset(tx, 0, sizeof(u32) * 8);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	tx_urb = usb_alloc_urb(0,GFP_ATOMIC);
#else
	tx_urb = usb_alloc_urb(0);
#endif

	if(!tx_urb){
		kfree(tx);
		return -ENOMEM;
	}

#define sCrcLng         4	
#define sAckCtsLng	112		// bits in ACK and CTS frames
	// Check multicast/broadcast
	if (ieee->iw_mode == IW_MODE_INFRA) {
		/* To DS: Addr1 = BSSID, Addr2 = SA,
		   Addr3 = DA */
		//memcpy(&dest, frag_hdr->addr3, ETH_ALEN);
		memcpy(&dest, frag_hdr->addr1, ETH_ALEN);
	} else if (ieee->iw_mode == IW_MODE_ADHOC) {
		/* not From/To DS: Addr1 = DA, Addr2 = SA,
		   Addr3 = BSSID */
		memcpy(&dest, frag_hdr->addr1, ETH_ALEN);
	}

	if (is_multicast_ether_addr(dest) ||
			is_broadcast_ether_addr(dest)) 
	{
		Duration = 0;
		RtsDur = 0;
		bRTSEnable = _FALSE;
		bCTSEnable = _FALSE;

		ThisFrameTime = ComputeTxTime(len + sCrcLng, rtl8180_rate2rate(rate), _FALSE, bUseShortPreamble);
		TxDescDuration = ThisFrameTime;
	} else {// Unicast packet
		//u8 AckRate;
		u16 AckTime;
		// Figure out ACK rate according to BSS basic rate and Tx rate, 2006.03.08 by rcnjko.
		//AckRate = ComputeAckRate( pMgntInfo->mBrates, (u1Byte)(pTcb->DataRate) );
		// Figure out ACK time according to the AckRate and assume long preamble is used on receiver, 2006.03.08, by rcnjko.
		//AckTime = ComputeTxTime( sAckCtsLng/8, AckRate, FALSE, FALSE);
		//For simplicity, just use the 1M basic rate
		AckTime = ComputeTxTime(14, 10,_FALSE, _FALSE);	// AckCTSLng = 14 use 1M bps send
		//AckTime = ComputeTxTime(14, 2,_FALSE, _FALSE);	// AckCTSLng = 14 use 1M bps send


		if ( ((len + sCrcLng) > priv->rts) && priv->rts )
		{ // RTS/CTS.
			u16 RtsTime, CtsTime;
			//u16 CtsRate;
			bRTSEnable = _TRUE;
			bCTSEnable = _FALSE;

			// Rate and time required for RTS.
			RtsTime = ComputeTxTime( sAckCtsLng/8,priv->ieee80211->basic_rate, _FALSE, _FALSE);
			// Rate and time required for CTS.
			CtsTime = ComputeTxTime(14, 10,_FALSE, _FALSE);	// AckCTSLng = 14 use 1M bps send

			// Figure out time required to transmit this frame.
			ThisFrameTime = ComputeTxTime(len + sCrcLng, 
					rtl8180_rate2rate(rate), 
					_FALSE, 
					bUseShortPreamble);

			// RTS-CTS-ThisFrame-ACK.
			RtsDur = CtsTime + ThisFrameTime + AckTime + 3*aSifsTime;

			TxDescDuration = RtsTime + RtsDur;
		}
		else {// Normal case.
			bCTSEnable = _FALSE;
			bRTSEnable = _FALSE;
			RtsDur = 0;

			ThisFrameTime = ComputeTxTime(len + sCrcLng, rtl8180_rate2rate(rate), _FALSE, bUseShortPreamble);
			TxDescDuration = ThisFrameTime + aSifsTime + AckTime;
		}

		if(!(frag_hdr->frame_ctl & IEEE80211_FCTL_MOREFRAGS)) { //no more fragment
			// ThisFrame-ACK.
			Duration = aSifsTime + AckTime;
		} else { // One or more fragments remained.
			u16 NextFragTime;
			NextFragTime = ComputeTxTime( len + sCrcLng, //pretend following packet length equal current packet
					rtl8180_rate2rate(rate), 
					_FALSE, 
					bUseShortPreamble );

			//ThisFrag-ACk-NextFrag-ACK.
			Duration = NextFragTime + 3*aSifsTime + 2*AckTime;
		}

	} // End of Unicast packet


	//fill the tx desriptor
	tx[0] |= len & 0xfff;

#ifdef JOHN_HWSEC
        if(frag_hdr->frame_ctl & IEEE80211_FCTL_WEP ){
                tx[0] &= 0xffff7fff;
		
                //group key may be different from pairwise key
		if( is_broadcast_ether_addr(frag_hdr->addr1))
		{
                        if(ieee->broadcast_key_type == KEY_TYPE_CCMP) tx[7] |= 0x2;//ccmp
                        else tx[7] |= 0x1;//wep and tkip
                }
                else {
                        if(ieee->pairwise_key_type == KEY_TYPE_CCMP) tx[7] |= 0x2;//CCMP
			else tx[7] |= 0x1;//WEP and TKIP
		}
        }
        else
#endif /*JOHN_HWSEC*/

	tx[0] |= (1<<15);

	if (priv->ieee80211->current_network.capability&WLAN_CAPABILITY_SHORT_PREAMBLE){
		if (priv->plcp_preamble_mode==1 && rate!=0) {	//  short mode now, not long!
			tx[0] |= (1<<16);	
		}			// enable short preamble mode.
	}

	if(morefrag) tx[0] |= (1<<17);
	//printk(KERN_WARNING "rtl_rate = %d\n", rate);
	tx[0] |= (rate << 24); //TX rate
	frag_hdr->duration_id = Duration;

	if(NIC_8187B == priv->card_8187) {
		if(bCTSEnable) {
			tx[0] |= (1<<18);
		}

		if(bRTSEnable) //rts enable
		{
			tx[0] |= ((ieeerate2rtlrate(priv->ieee80211->basic_rate))<<19);//RTS RATE
			tx[0] |= (1<<23);//rts enable
			tx[1] |= RtsDur;//RTS Duration
		}	
		tx[3] |= (TxDescDuration<<16); //DURATION
		if( (WLAN_FC_GET_TYPE(frag_hdr->frame_ctl)== IEEE80211_FTYPE_MGMT) && (WLAN_FC_GET_STYPE(frag_hdr->frame_ctl)==IEEE80211_STYPE_PROBE_RESP))
			tx[5] = (2<<8);
		else
			tx[5] |= (11<<8);//(priv->retry_data<<8); //retry lim ;	

		//frag_hdr->duration_id = Duration;
		memcpy(tx+8,txbuf,len);
	} else {
		if ( (len>priv->rts) && priv->rts && priority==LOW_PRIORITY){
			tx[0] |= (1<<23);	//enalbe RTS function
			tx[1] |= RtsDur; 	//Need to edit here!  ----hikaru
		}
		else {
			tx[1]=0;
		}
		tx[0] |= (ieeerate2rtlrate(priv->ieee80211->basic_rate) << 19); /* RTS RATE - should be basic rate */

		tx[2] = 3;  // CW min
		tx[2] |= (7<<4); //CW max
		tx[2] |= (11<<8);//(priv->retry_data<<8); //retry lim

		//	printk("%x\n%x\n",tx[0],tx[1]);

#ifdef DUMP_TX
		int i;
		printk("<Tx pkt>--rate %x---",rate);
		for (i = 0; i < (len + 3); i++)
			printk("%2x", ((u8*)tx)[i]);
		printk("---------------\n");
#endif
		memcpy(tx+3,txbuf,len);
	}

#ifdef JOHN_DUMP_TXDESC
                int i;
                printk("<Tx descriptor>--rate %x---",rate);
                for (i = 0; i < 8; i++)
                        printk("%8x ", tx[i]);
                printk("\n");
#endif
#ifdef JOHN_DUMP_TXPKT
                int j;
                printk("<Tx packet>--rate %x--urb_len in decimal %d",rate, urb_len);
                for (j = 32; j < (urb_len); j++){
                        if( ( (j-32)%24 )==0 ) printk("\n");
                        printk("%2x ", ((u8*)tx)[j]);
                }
                printk("\n---------------\n");
#endif

	if(NIC_8187 == priv->card_8187) {
		usb_fill_bulk_urb(tx_urb,priv->udev,
				usb_sndbulkpipe(priv->udev,priority), tx,
				urb_len, (priority == LOW_PRIORITY)?rtl8187_lptx_isr:rtl8187_nptx_isr, dev);

	} else {
		//printk(KERN_WARNING "Tx packet use by submit urb!\n");
		/* FIXME check what EP is for low/norm PRI */
		usb_fill_bulk_urb(tx_urb,priv->udev,
				usb_sndbulkpipe(priv->udev,priority), tx,
				urb_len, TXISR_SELECT(priority), dev);
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	status = usb_submit_urb(tx_urb, GFP_ATOMIC);
#else
	status = usb_submit_urb(tx_urb);
#endif

	if (!status){
		//atomic_inc((priority == NORM_PRIORITY)? &priv->tx_np_pending : &priv->tx_lp_pending);
		atomic_inc(&priv->tx_pending[priority]);
		dev->trans_start = jiffies; 		//john
		return 0;
	}else{
		DMESGE("Error TX URB %d, error %d",
				//atomic_read((priority == NORM_PRIORITY)? &priv->tx_np_pending : &priv->tx_lp_pending),
				atomic_read(&priv->tx_pending[priority]),
				status);
		return -1;
	}
}

 



short rtl8187_usb_initendpoints(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	int align;
        u32 oldaddr,newaddr;
	priv->rx_urb = (struct urb**) kmalloc (sizeof(struct urb*) * (MAX_RX_URB+1), GFP_KERNEL);

#ifndef JACKSON_NEW_RX
	int i;
	for(i=0;i<(MAX_RX_URB+1);i++){

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		priv->rx_urb[i] = usb_alloc_urb(0,GFP_KERNEL);
#else
		priv->rx_urb[i] = usb_alloc_urb(0);
#endif

		priv->rx_urb[i]->transfer_buffer = kmalloc(RX_URB_SIZE, GFP_KERNEL);
			
		priv->rx_urb[i]->transfer_buffer_length = RX_URB_SIZE;
	}
#endif       

        memset(priv->rx_urb, 0, sizeof(struct urb*) * MAX_RX_URB);
        priv->pp_rxskb = (struct sk_buff **)kmalloc(sizeof(struct sk_buff *) * MAX_RX_URB, GFP_KERNEL);
        if (priv->pp_rxskb == NULL)
                goto destroy;

        memset(priv->pp_rxskb, 0, sizeof(struct sk_buff*) * MAX_RX_URB);

#ifdef THOMAS_BEACON
        priv->rx_urb[MAX_RX_URB] = usb_alloc_urb(0, GFP_KERNEL);
        priv->oldaddr = kmalloc(16, GFP_KERNEL);
        oldaddr = (u32)priv->oldaddr;
        align = oldaddr&3;
        if(align != 0 ){
                newaddr = oldaddr + 4 - align;
                priv->rx_urb[MAX_RX_URB]->transfer_buffer_length = 16-4+align;
        }
        else{
                newaddr = oldaddr;
                priv->rx_urb[MAX_RX_URB]->transfer_buffer_length = 16;
        }
        priv->rx_urb[MAX_RX_URB]->transfer_buffer = (u32*)newaddr;
#endif
        goto _middle;


destroy:
        if (priv->pp_rxskb) {
                kfree(priv->pp_rxskb);
        }
        if (priv->rx_urb) {
                kfree(priv->rx_urb);
        }
        priv->pp_rxskb = NULL;
	priv->rx_urb = NULL;

        DMESGE("Endpoint Alloc Failure");
        return -ENOMEM;


_middle:

        printk("End of initendpoints\n");
        return 0;

}
#ifdef THOMAS_BEACON
void rtl8187_usb_deleteendpoints(struct net_device *dev)
{	
	int i;
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	if(priv->rx_urb){
		for(i=0;i<(MAX_RX_URB+1);i++){
			if(priv->rx_urb[i]){
				usb_kill_urb(priv->rx_urb[i]);
				usb_free_urb(priv->rx_urb[i]);
			}	
		}
		kfree(priv->rx_urb);
		priv->rx_urb = NULL;	
	}	
	if(priv->oldaddr){
		kfree(priv->oldaddr);
		priv->oldaddr = NULL;
	}	
        if (priv->pp_rxskb) {
                kfree(priv->pp_rxskb);
                priv->pp_rxskb = 0;
	}
}
#else
void rtl8187_usb_deleteendpoints(struct net_device *dev)
{
	int i;
	struct r8180_priv *priv = ieee80211_priv(dev);

#ifndef JACKSON_NEW_RX
	
	if(priv->rx_urb){
		for(i=0;i<(MAX_RX_URB+1);i++){
			usb_kill_urb(priv->rx_urb[i]);
			kfree(priv->rx_urb[i]->transfer_buffer);
			usb_free_urb(priv->rx_urb[i]);
		}
		kfree(priv->rx_urb);
		priv->rx_urb = NULL;
		
	}
#else
	if(priv->rx_urb){
                kfree(priv->rx_urb);
                priv->rx_urb = NULL;
        }
	if(priv->oldaddr){
		kfree(priv->oldaddr);
		priv->oldaddr = NULL;
	}	
        if (priv->pp_rxskb) {
                kfree(priv->pp_rxskb);
                priv->pp_rxskb = 0;

        }

#endif	
}
#endif

void rtl8187_set_rate(struct net_device *dev)
{
	int i;
	u16 word;
	int basic_rate,min_rr_rate,max_rr_rate;

//	struct r8180_priv *priv = ieee80211_priv(dev);
	
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


void rtl8187_link_change(struct net_device *dev)
{
//	int i;
	
	struct r8180_priv *priv = ieee80211_priv(dev);
	//write_nic_word(dev, BintrItv, net->beacon_interval);
	if(priv->bSurpriseRemoved == _FALSE) {
	rtl8187_net_update(dev);
	/*update timing params*/
	rtl8180_set_chan(dev, priv->chan);
	}

	//rtl8187_set_rxconf(dev);
}

void rtl8180_wmm_param_update(struct ieee80211_device *ieee)
{
	struct net_device *dev = ieee->dev;
	struct r8180_priv *priv = ieee80211_priv(dev);
	u8 *ac_param = (u8 *)(ieee->current_network.wmm_param);
	u8 mode = ieee->current_network.mode;
	AC_CODING	eACI;
	AC_PARAM	AcParam;
	PAC_PARAM	pAcParam;
	u8 i;

	//8187 need not to update wmm param, added by David, 2006.9.8
	if(NIC_8187 == priv->card_8187) {
		return; 
	}

	if(!ieee->current_network.QoS_Enable) 
	{
		//legacy ac_xx_param update
		
		AcParam.longData = 0;
		AcParam.f.AciAifsn.f.AIFSN = 2; // Follow 802.11 DIFS.
		AcParam.f.AciAifsn.f.ACM = 0;
		AcParam.f.Ecw.f.ECWmin = 3; // Follow 802.11 CWmin.
		AcParam.f.Ecw.f.ECWmax = 7; // Follow 802.11 CWmax.
		AcParam.f.TXOPLimit = 0;
		for(eACI = 0; eACI < AC_MAX; eACI++)
		{
			AcParam.f.AciAifsn.f.ACI = (u8)eACI;
			{
				u8		u1bAIFS;
				u32		u4bAcParam;


				pAcParam = (PAC_PARAM)(&AcParam);
				// Retrive paramters to udpate.
				u1bAIFS = pAcParam->f.AciAifsn.f.AIFSN *(((mode&IEEE_G) == IEEE_G)?9:20)  + aSifsTime; 
				u4bAcParam = (	(((u32)(pAcParam->f.TXOPLimit)) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
						(((u32)(pAcParam->f.Ecw.f.ECWmax)) << AC_PARAM_ECW_MAX_OFFSET)	|  
						(((u32)(pAcParam->f.Ecw.f.ECWmin)) << AC_PARAM_ECW_MIN_OFFSET)	|  
						(((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET));

				switch(eACI)
				{
					case AC1_BK:
						write_nic_dword(dev, AC_BK_PARAM, u4bAcParam);
						break;

					case AC0_BE:
						write_nic_dword(dev, AC_BE_PARAM, u4bAcParam);
						break;

					case AC2_VI:
						write_nic_dword(dev, AC_VI_PARAM, u4bAcParam);
						break;

					case AC3_VO:
						write_nic_dword(dev, AC_VO_PARAM, u4bAcParam);
						break;

					default:
						printk(KERN_WARNING "SetHwReg8185(): invalid ACI: %d !\n", eACI);
						break;
				}
			}
		}

		return;
	}
	//
	for(i = 0; i < AC_MAX; i++)
	{
		//AcParam.longData = 0;
		
		
		pAcParam = (AC_PARAM * )ac_param;
		{
			AC_CODING	eACI;
			u8		u1bAIFS;
			u32		u4bAcParam;

			// Retrive paramters to udpate.
			eACI = pAcParam->f.AciAifsn.f.ACI; 
			//Mode G/A: slotTimeTimer = 9; Mode B: 20
			u1bAIFS = pAcParam->f.AciAifsn.f.AIFSN * (((mode&IEEE_G) == IEEE_G)?9:20) + aSifsTime; 
			u4bAcParam = (	(((u32)(pAcParam->f.TXOPLimit)) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
					(((u32)(pAcParam->f.Ecw.f.ECWmax)) << AC_PARAM_ECW_MAX_OFFSET)	|  
					(((u32)(pAcParam->f.Ecw.f.ECWmin)) << AC_PARAM_ECW_MIN_OFFSET)	|  
					(((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET));

			switch(eACI)
			{
				case AC1_BK:
					write_nic_dword(dev, AC_BK_PARAM, u4bAcParam);
					//printk(KERN_WARNING "[%04x]:0x%08x\n",AC_BK_PARAM,read_nic_dword(dev, AC_BK_PARAM));
					break;

				case AC0_BE:
					write_nic_dword(dev, AC_BE_PARAM, u4bAcParam);
					//printk(KERN_WARNING "[%04x]:0x%08x\n",AC_BE_PARAM,read_nic_dword(dev, AC_BE_PARAM));
					break;

				case AC2_VI:
					write_nic_dword(dev, AC_VI_PARAM, u4bAcParam);
					//printk(KERN_WARNING "[%04x]:0x%08x\n",AC_VI_PARAM,read_nic_dword(dev, AC_VI_PARAM));
					break;

				case AC3_VO:
					write_nic_dword(dev, AC_VO_PARAM, u4bAcParam);
					//printk(KERN_WARNING "[%04x]:0x%08x\n",AC_VO_PARAM,read_nic_dword(dev, AC_VO_PARAM));
					break;

				default:
					printk(KERN_WARNING "SetHwReg8185(): invalid ACI: %d !\n", eACI);
					break;
			}
		}
		ac_param += (sizeof(AC_PARAM));
	}
}

void rtl8180_irq_rx_tasklet_new(struct r8180_priv *priv);
void rtl8180_irq_rx_tasklet(struct r8180_priv *priv);

#ifdef JOHN_DIG
void r8187_dig_thread(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u16 CCKFalseAlarm, OFDMFalseAlarm;
	u16 OfdmFA1, OfdmFA2;
	int InitialGainStep = 7; // The number of initial gain stages.
	int LowestGainStage = 4; // The capable lowest stage of performing dig workitem.
	u16	CCK_Up_Th, CCK_Lw_Th;

	if(priv->bSurpriseRemoved == _TRUE)
		return;

	set_current_state(TASK_INTERRUPTIBLE);	
	//printk("timer==>...\n");

	priv->FalseAlarmRegValue = read_nic_dword(dev, CCK_FALSE_ALARM);
	CCKFalseAlarm = (u16)(priv->FalseAlarmRegValue & 0x0000ffff);
	OFDMFalseAlarm = (u16)( (priv->FalseAlarmRegValue>>16) & 0x0000ffff);
	OfdmFA1 = 0x15;
//	OfdmFA2 = ((u16)(priv->RegDigOfdmFaUpTh)) << 8;
	OfdmFA2 = 0xC00;		
//	printk("@@ FalseAlarmRegValue = %8x\n", priv->FalseAlarmRegValue);

// The number of initial gain steps is different, by Bruce, 2007-04-13.		
	if(priv->InitialGain == 0)  //autoDIG
	{// Advised from SD3 DZ, by Bruce, 2007-06-05.
		priv->InitialGain = 4; // In 87B, m74dBm means State 4 (m82dBm)
	}
	if(priv->card_8187_Bversion != VERSION_8187B_B)
	{ // Advised from SD3 DZ, by Bruce, 2007-06-05.
		OfdmFA1 =  0x20;
	}
	InitialGainStep = 8;
	LowestGainStage = 1;

	if (OFDMFalseAlarm > OfdmFA1)
	{
//printk("@@ (OFDMFalseAlarm > OfdmFA1)\n");
		if (OFDMFalseAlarm > OfdmFA2)
		{
//printk("@@ (OFDMFalseAlarm > OfdmFA2)\n");
			priv->DIG_NumberFallbackVote++;
			if (priv->DIG_NumberFallbackVote >1)
			{
//printk("@@ (priv->DIG_NumberFallbackVote >1) \n");
                             // serious OFDM  False Alarm, need fallback
                             // By Bruce, 2007-03-29.
                             // if (priv->InitialGain < 7) // In 87B, m66dBm means State 7 (m74dBm)
				if (priv->InitialGain < InitialGainStep)
				{
//printk("@@ (priv->InitialGain < InitialGainStep) \n");
					priv->InitialGain = (priv->InitialGain + 1);
					UpdateInitialGain(dev); // 2005.01.06, by rcnjko.
				}
				priv->DIG_NumberFallbackVote = 0;
				priv->DIG_NumberUpgradeVote=0;
			}
		}
		else
		{
//printk("@@ else (OFDMFalseAlarm < OfdmFA2)\n");
			if (priv->DIG_NumberFallbackVote)
				priv->DIG_NumberFallbackVote--;
		}
		priv->DIG_NumberUpgradeVote=0;
	}
	else    //OFDM False Alarm < 0x15
	{
//printk("@@ else    //OFDM False Alarm < 0x15\n");
		if (priv->DIG_NumberFallbackVote)
			priv->DIG_NumberFallbackVote--;
		priv->DIG_NumberUpgradeVote++;

		if (priv->DIG_NumberUpgradeVote>9)
		{
//printk("@@ (priv->DIG_NumberUpgradeVote>9)\n");
			if (priv->InitialGain > LowestGainStage) // In 87B, m78dBm means State 4 (m864dBm)
			{
//printk("@@ (priv->InitialGain > LowestGainStage)\n");
				priv->InitialGain = (priv->InitialGain - 1);
				UpdateInitialGain(dev); // 2005.01.06, by rcnjko.
			}
			priv->DIG_NumberFallbackVote = 0;
			priv->DIG_NumberUpgradeVote=0;
		}
	}

	// By Bruce, 2007-03-29.
	// Dynamically update CCK Power Detection Threshold.
	CCK_Up_Th = priv->CCKUpperTh;
	CCK_Lw_Th = priv->CCKLowerTh;	
	CCKFalseAlarm = (u16)((priv->FalseAlarmRegValue & 0x0000ffff) >> 8); // We only care about the higher byte.

	if( priv->StageCCKTh < 3 && CCKFalseAlarm >= CCK_Up_Th)
	{
		priv->StageCCKTh ++;
		UpdateCCKThreshold(dev);
	}
	else if(priv->StageCCKTh > 0 && CCKFalseAlarm <= CCK_Lw_Th)
	{
		priv->StageCCKTh --;
		UpdateCCKThreshold(dev);
	}	

	schedule_delayed_work(&priv->dig_thread, HZ*2);
}


void r8187_start_dig_thread(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->have_dig_thread = 1;
	schedule_delayed_work(&priv->dig_thread, HZ*2);
}

void r8187_stop_dig_thread(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	if(priv->have_dig_thread){
		cancel_rearming_delayed_work(&priv->dig_thread);
		priv->have_dig_thread = 0;
	}
	else flush_scheduled_work();
}
#endif


#define RTL8187B_VER_REG	0xE1		// 8187B version register.
void
GetHardwareVersion8187( struct net_device	*dev )
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	u8			u1bTmp;

	//
	// RTL8187B version, 2006.09.21 by SD1 William.
	//

	u1bTmp = read_nic_byte(dev, RTL8187B_VER_REG);
	switch(u1bTmp)
	{
	case 0x00:
		priv->card_8187_Bversion= VERSION_8187B_B;
		DMESG("Hardware version is 8187B_B");
		break;

	case 0x01:
		priv->card_8187_Bversion = VERSION_8187B_D;
		DMESG("Hardware version is 8187B_D");
		break;

	case 0x02:
		priv->card_8187_Bversion = VERSION_8187B_E;
		DMESG("Hardware version is 8187B_E");
		break;

	default:
		priv->card_8187_Bversion = VERSION_8187B_B;
		DMESG("Hardware version is 8187B_B");
		break;
	}
}

void
ChangeHalDefSettingByIcVersion( struct net_device	*dev )
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	// Default 3-wire setting.
	priv->RegThreeWireMode = SW_THREE_WIRE;

	// Modified by Roger, 2007.02.06.
	// D-Cut or later.
	if(priv->card_8187_Bversion >= VERSION_8187B_D)
	{
		//
		// Default PA for 87B D-cut is SST. ID: 690. 060927, by rcnjko.
		//
		if(priv->RegPaModel == PA_DEFAULT)
		{
			priv->RegPaModel = PA_SST;
		}

		// 
		// To speed up RF Write process, sync with normal driver. by Bruce, 2007-09-06. 
		//
		priv->RegThreeWireMode = HW_THREE_WIRE_BY_8051;//87B (87 don't care)		
		
	}
	else if( priv->card_8187_Bversion == VERSION_8187B_B)
	{
		//
		// Default PA for 87B B-cut is SIGS. ID: 690. 060927, by rcnjko.
		//
		if(priv->RegPaModel == PA_DEFAULT)
		{
			priv->RegPaModel = PA_SIGE;
		}			
	}
	// Default D-cut .
	else
	{
		//
		// Default PA for 87B D-cut is SST. ID: 690. 060927, by rcnjko.
		//
		if(priv->RegPaModel == PA_DEFAULT)
		{
			priv->RegPaModel = PA_SST;
		}
	}
}


short rtl8180_init(struct net_device *dev)
{
		
	struct r8180_priv *priv = ieee80211_priv(dev);
	int i;
	u16 word;
	int ch,interface_select;
	//u16 version;
	//u8 hw_version;
	//u8 config3;
//	struct usb_device *udev;
//	u16 idProduct;
	u16 EEPROMID, EEPROMVID, EEPROMDID;
	//u16 usValue;
        u8 version_87b;
	
	//FIXME: these constants are placed in a bad pleace.

//	priv->txbuffsize = 1024;
//	priv->txringcount = 32;
//	priv->rxbuffersize = 1024;
//	priv->rxringcount = 32; 
//	priv->txbeaconcount = 3;
//	priv->rx_skb_complete = 1;
	//priv->txnp_pending.ispending=0; 
	/* ^^ the SKB does not containt a partial RXed
	 * packet (is empty)
	 */

	if(!channels){
		DMESG("No channels, aborting");
		return -1;
	}
	ch=channels;
	 // set channels 1..14 allowed in given locale
	for (i=1; i<=14; i++) {
		(priv->ieee80211->channel_map)[i] = (u8)(ch & 0x01);
		ch >>= 1;
	}
	//memcpy(priv->stats,0,sizeof(struct Stats));
	
	//priv->irq_enabled=0;
	priv->bSurpriseRemoved = _FALSE;
//	priv->stats.rxdmafail=0;
	priv->stats.txrdu=0;
//	priv->stats.rxrdu=0;
//	priv->stats.rxnolast=0;
//	priv->stats.rxnodata=0;
	//priv->stats.rxreset=0;
	//priv->stats.rxwrkaround=0;
//	priv->stats.rxnopointer=0;
	priv->stats.txbeerr=0;
	priv->stats.txbkerr=0;
	priv->stats.txvierr=0;
	priv->stats.txvoerr=0;
	priv->stats.txmanageerr=0;
	priv->stats.txbeaconerr=0;
	priv->stats.txresumed=0;
//	priv->stats.rxerr=0;
//	priv->stats.rxoverflow=0;
//	priv->stats.rxint=0;
	priv->stats.txbeokint=0;
	priv->stats.txbkokint=0;
	priv->stats.txviokint=0;
	priv->stats.txvookint=0;
	priv->stats.txmanageokint=0;
	priv->stats.txbeaconokint=0;
	/*priv->stats.txhpokint=0;
	priv->stats.txhperr=0;*/
	priv->stats.rxurberr=0;
	priv->stats.rxstaterr=0;
	priv->stats.txoverflow=0;
	priv->stats.rxok=0;
//	priv->stats.txbeaconerr=0;
//	priv->stats.txlperr=0;
//	priv->stats.txlpokint=0;

//john
        priv->stats.txnpdrop=0;
        priv->stats.txlpdrop =0;
        priv->stats.txbedrop =0;
        priv->stats.txbkdrop =0;
        priv->stats.txvidrop =0;
        priv->stats.txvodrop =0;
        priv->stats.txbeacondrop =0;
        priv->stats.txmanagedrop =0;
#ifdef JOHN_DIG
	priv->RegBModeGainStage = 1;
	priv->CCKUpperTh = 0x100;
	priv->CCKLowerTh = 0x20;
	priv->StageCCKTh = 0;
	priv->InitialGain = 0;
	priv->DIG_NumberFallbackVote = 0;
	priv->DIG_NumberUpgradeVote = 0;
	priv->StageCCKTh = 0;
	priv->FalseAlarmRegValue = 0;
#endif	
#ifdef THOMAS_RATEADAPTIVE
	priv->CurrRetryCnt = 0;
	priv->LastRetryCnt = 0;
	priv->LastTxokCnt = 0;
	priv->LastRxokCnt = 0;
	priv->LastTxOKBytes = 0;
	priv->LastTxThroughput = 0;
	priv->LastRetryRate = 0;
	priv->TryupingCountNoData = 0;
	priv->TryDownCountLowData = 0;
	priv->bTryuping = _FALSE;
	priv->LastFailTxRate = 0;
	priv->LastFailTxRateSS = 0;
	priv->FailTxRateCount = 0;
	priv->TryupingCount = 0;
	priv->stats.NumTxOkTotal = 0;
	priv->stats.RecvSignalPower = 0;
	priv->stats.NumTxOkBytesTotal = 0;
	priv->bRateAdaptive = _FALSE;
#endif
	priv->ieee80211->iw_mode = IW_MODE_INFRA;

//test pending bug, john 20070815
        for(i=0;i<0x10;i++)  atomic_set(&(priv->tx_pending[i]), 0);
	
	priv->retry_rts = DEFAULT_RETRY_RTS;
	priv->retry_data = DEFAULT_RETRY_DATA;
	priv->ieee80211->rate = 110; //11 mbps
	priv->ieee80211->short_slot = 1;
	priv->ieee80211->mode = IEEE_G;
	priv->promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	spin_lock_init(&priv->tx_lock);
	spin_lock_init(&priv->irq_lock);//added by thomas
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	INIT_WORK(&priv->reset_wq,(void(*)(void*)) rtl8180_restart,dev);
#else
	tq_init(&priv->reset_wq,(void(*)(void*)) rtl8180_restart,dev);
#endif
	sema_init(&priv->wx_sem,1);
#ifdef THOMAS_TASKLET
	tasklet_init(&priv->irq_rx_tasklet,
		     (void(*)(unsigned long))rtl8180_irq_rx_tasklet_new,
		     (unsigned long)priv);
#else
	tasklet_init(&priv->irq_rx_tasklet,
		     (void(*)(unsigned long))rtl8180_irq_rx_tasklet,
		     (unsigned long)priv);
#endif

#ifdef JOHN_DIG
	INIT_WORK(&priv->dig_thread, (void(*)(void*)) r8187_dig_thread, dev);
#endif
#ifdef THOMAS_RATEADAPTIVE
	INIT_WORK(&priv->RateAdaptiveWorkItem, (void(*)(void*)) RateAdaptiveWorkItemCallback, dev);
#endif

	//priv->ieee80211->func = 
	//	kmalloc(sizeof(struct ieee80211_helper_functions),GFP_KERNEL);
	//memset(priv->ieee80211->func, 0,
	  //     sizeof(struct ieee80211_helper_functions));
	priv->ieee80211->current_network.beacon_interval = DEFAULT_BEACONINTERVAL;	
	priv->ieee80211->iw_mode = IW_MODE_INFRA;
	priv->ieee80211->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE; /* |
		IEEE_SOFTMAC_BEACONS;*/ //|  //IEEE_SOFTMAC_SINGLE_QUEUE;
	
	priv->ieee80211->active_scan = 1;
	priv->ieee80211->rate = 110; //11 mbps
	priv->ieee80211->modulation = IEEE80211_CCK_MODULATION | IEEE80211_OFDM_MODULATION;
	priv->ieee80211->host_encrypt = 1;
	priv->ieee80211->host_decrypt = 1;
	priv->ieee80211->start_send_beacons = rtl8187_beacon_tx;
	priv->ieee80211->stop_send_beacons = rtl8187_beacon_stop;
	priv->ieee80211->softmac_hard_start_xmit = rtl8180_hard_start_xmit;
	priv->ieee80211->set_chan = rtl8180_set_chan;
	priv->ieee80211->link_change = rtl8187_link_change;
	priv->ieee80211->softmac_data_hard_start_xmit = rtl8180_hard_data_xmit;
	priv->ieee80211->data_hard_stop = rtl8180_data_hard_stop;
	priv->ieee80211->data_hard_resume = rtl8180_data_hard_resume;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	INIT_WORK(&priv->ieee80211->wmm_param_update_wq,\
                  (void(*)(void*)) rtl8180_wmm_param_update,\
                  priv->ieee80211);
#else
	tq_init(&priv->ieee80211->wmm_param_update_wq,\
                (void(*)(void*)) rtl8180_wmm_param_update,\
                priv->ieee80211);
#endif
	priv->ieee80211->init_wmmparam_flag = 0;
	//{test for softbeacon by David 2006.8.16
	//priv->ieee80211->start_send_beacons = NULL;
	//priv->ieee80211->stop_send_beacons = NULL;
	//}
	priv->ieee80211->fts = DEFAULT_FRAG_THRESHOLD;
	
	//priv->card_8185 = 2;
	priv->phy_ver = 2;
	priv->card_type = USB;
//{add for 87B	
	priv->ShortRetryLimit = 7;
	priv->LongRetryLimit = 7;
	priv->EarlyRxThreshold = 7;
	
	priv->TransmitConfig = 
		TCR_DurProcMode |	//for RTL8185B, duration setting by HW
		TCR_DISReqQsize |
                (TCR_MXDMA_2048<<TCR_MXDMA_SHIFT)|  // Max DMA Burst Size per Tx DMA Burst, 7: reservied.
		(priv->ShortRetryLimit<<TX_SRLRETRY_SHIFT)|	// Short retry limit
		(priv->LongRetryLimit<<TX_LRLRETRY_SHIFT) |	// Long retry limit
		(_FALSE? TCR_SWPLCPLEN : 0);	// FALSE: HW provies PLCP length and LENGEXT, TURE: SW proiveds them

	priv->ReceiveConfig	=
		RCR_AMF | RCR_ADF |		//accept management/data
//		RCR_ACF |			//accept control frame for SW AP needs PS-poll, 2005.07.07, by rcnjko.
		RCR_AB | RCR_AM | RCR_APM |	//accept BC/MC/UC
		//RCR_AICV | RCR_ACRC32 | 	//accept ICV/CRC error packet
		RCR_MXDMA | // Max DMA Burst Size per Rx DMA Burst, 7: unlimited.
		(priv->EarlyRxThreshold<<RX_FIFO_THRESHOLD_SHIFT) | // Rx FIFO Threshold, 7: No Rx threshold.
		(priv->EarlyRxThreshold == 7 ? RCR_ONLYERLPKT:0);

	priv->AcmControl = 0;
	
	priv->enable_gpio0 = 0;

	/*the eeprom type is stored in RCR register bit #6 */ 
	if (RCR_9356SEL & read_nic_dword(dev, RCR)){
		priv->epromtype=EPROM_93c56;
		DMESG("Reported EEPROM chip is a 93c56 (2Kbit)");
	}else{
		priv->epromtype=EPROM_93c46;
		DMESG("Reported EEPROM chip is a 93c46 (1Kbit)");
	}
	
	dev->get_stats = rtl8180_stats;

	// Get hardware version
	GetHardwareVersion8187(dev);

	// Change some setting according to IC version.
	ChangeHalDefSettingByIcVersion(dev);

	EEPROMID = Read_EEprom87B(dev, 0x00);
	DMESG("EEPROM ID %4x",EEPROMID);
	for(i=0 ; i<6; i+=2){
		u16 Tmp = Read_EEprom87B(dev, ( (EEPROM_MAC_ADDRESS+i) >> 1));
		dev->dev_addr[i] = Tmp &0xff;
		dev->dev_addr[i+1] = (Tmp &0xff00)>>8;
	}
	DMESG("Card MAC address is "MAC_FMT, MAC_ARG(dev->dev_addr));
	
	//write IDR0~IDR5
	for(i=0 ; i<6 ; i++)
		write_nic_byte(dev, IDR0+i, dev->dev_addr[i]);
	
	//priv->rf_chip = 0x0f & Read_EEprom87B(dev,EEPROM_RF_CHIP_ID>>1);
	priv->rf_chip = RF_ZEBRA2;
	
        EEPROMVID = Read_EEprom87B(dev, EEPROM_VID>>1);
        EEPROMDID = Read_EEprom87B(dev, EEPROM_PID>>1);
        DMESG("EEPROM VID:0x%4x ,EEPROM DID:0x%4x",EEPROMVID,EEPROMDID);

        if(EEPROMDID == 0x8981)
                EEPROMID = 0x8129;

	priv->eeprom_channelplan = (Read_EEprom87B(dev, EEPROM_CHANNEL_PLAN>>1))&0xff;
	//DMESG("EEPROM ChannelPlan is 0x%x",priv->eeprom_channelplan);
	
	interface_select = ((Read_EEprom87B(dev, EEPROM_INTERFACE_SELECT>>1))&0xC000)>>14;
	//DMESG("Interface Selection is 0x%x",interface_select);
	
	// Read Tx power base offset.
	word = (Read_EEprom87B(dev, EEPROM_TX_POWER_BASE_OFFSET>>1))&0xff;
	//DMESG("EEPROM_TX_POWER_BASE_OFFSET:0x%0.2x",word);
	priv->cck_txpwr_base = word & 0xf;
	priv->ofdm_txpwr_base = (word>>4) & 0xf;
	
	// Read Tx power index of each channel from EEPROM.
	// ch1- ch2.
	word = Read_EEprom87B(dev, (EEPROM_TX_POWER_LEVEL_1_2 >> 1));
	priv->chtxpwr[1]=word & 0xf;
	priv->chtxpwr_ofdm[1]=(word & 0xf0)>>4;
	priv->chtxpwr[2]= (word & 0xf00)>>8;
	priv->chtxpwr_ofdm[2]=(word& 0xf000)>>12;
	//DMESG("TXPOWER_1_2 %x %x %x %x",priv->chtxpwr[1],priv->chtxpwr_ofdm[1],priv->chtxpwr[2],priv->chtxpwr_ofdm[2]);

	// ch3- ch4.
	word = Read_EEprom87B(dev, (EEPROM_TX_POWER_LEVEL_3_4 >> 1));
	priv->chtxpwr[3]=word & 0xf;
	priv->chtxpwr_ofdm[3]=(word & 0xf0)>>4;
	priv->chtxpwr[4]= (word & 0xf00)>>8;
	priv->chtxpwr_ofdm[4]=(word& 0xf000)>>12;
	//DMESG("TXPOWER_3_4 %x %x %x %x",priv->chtxpwr[3],priv->chtxpwr_ofdm[3],priv->chtxpwr[4],priv->chtxpwr_ofdm[4]);
	
	// ch5- ch6.
	word = Read_EEprom87B(dev, (EEPROM_TX_POWER_LEVEL_5_6 >> 1));
	priv->chtxpwr[5]=word & 0xf;
	priv->chtxpwr_ofdm[5]=(word & 0xf0)>>4;
	priv->chtxpwr[6]= (word & 0xf00)>>8;
	priv->chtxpwr_ofdm[6]=(word& 0xf000)>>12;
	//DMESG("TXPOWER_5_6 %x %x %x %x",priv->chtxpwr[5],priv->chtxpwr_ofdm[5],priv->chtxpwr[6],priv->chtxpwr_ofdm[6]);

	// ch7- ch10.
	for(i=7 ; i<10 ; i+=2){
		word = Read_EEprom87B(dev, (EEPROM_TX_POWER_LEVEL_7_10 + i - 7) >> 1);
		priv->chtxpwr[i]=word & 0xf;
        	priv->chtxpwr_ofdm[i]=(word & 0xf0)>>4;
		priv->chtxpwr[i+1]= (word & 0xf00)>>8;
        	priv->chtxpwr_ofdm[i+1]=(word& 0xf000)>>12;
		//DMESG("TXPOWER_%d_%d %x %x %x %x",i,i+1,priv->chtxpwr[i],priv->chtxpwr_ofdm[i],priv->chtxpwr[i+1],priv->chtxpwr_ofdm[i+1]);
	}

	// ch11.
	word = Read_EEprom87B(dev, (EEPROM_TX_POWER_LEVEL_11 >> 1));
	priv->chtxpwr[11]=word & 0xf;
	priv->chtxpwr_ofdm[11]=(word & 0xf0)>>4;

	// ch12.
	word = Read_EEprom87B(dev, (EEPROM_TX_POWER_LEVEL_12 >> 1));
	priv->chtxpwr[12]=word & 0xf;
	priv->chtxpwr_ofdm[12]=(word & 0xf0)>>4;
	//DMESG("TXPOWER_11_12 %x %x %x %x",priv->chtxpwr[11],priv->chtxpwr_ofdm[11],priv->chtxpwr[12],priv->chtxpwr_ofdm[12]);

	// ch13- ch14.
	word = Read_EEprom87B(dev, (EEPROM_TX_POWER_LEVEL_13_14 >> 1));
	priv->chtxpwr[13]=word & 0xf;
	priv->chtxpwr_ofdm[13]=(word & 0xf0)>>4;
	priv->chtxpwr[14]=(word& 0xf00)>>8;
	priv->chtxpwr_ofdm[14]=(word& 0xf000)>>12;
	//DMESG("TXPOWER_13_14 %x %x %x %x",priv->chtxpwr[13],priv->chtxpwr_ofdm[13],priv->chtxpwr[14],priv->chtxpwr_ofdm[14]);

	/*usValue = Read_EEprom87B(dev, EEPROM_SW_REVD_OFFSET>>1);

	// Low Noise Amplifier. added by Roger, 2007.04.06.
	if( (usValue & EEPROM_LNA_MASK) == EEPROM_LNA_ENABLE )
	{
		priv->bLowNoiseAmplifier = _TRUE;
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("Support LNA\n"));
	}
	else
	{
		priv->bLowNoiseAmplifier = _FALSE;
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("NOT Support LNA\n"));
	}*/

        version_87b = read_nic_byte(dev, 0xE1);
        if(version_87b == 0x02)
                EEPROMID = 0x8129;
	
/*
	u16 EEPROMID = 0;
	EEPROMID = eprom_read(dev, 0x00);
	DMESG("EEPROM ID %0.4x",EEPROMID);
        dev->dev_addr[0]=eprom_read(dev,MAC_ADR) & 0xff;
        dev->dev_addr[1]=(eprom_read(dev,MAC_ADR) & 0xff00)>>8;
        dev->dev_addr[2]=eprom_read(dev,MAC_ADR+1) & 0xff;
        dev->dev_addr[3]=(eprom_read(dev,MAC_ADR+1) & 0xff00)>>8;
        dev->dev_addr[4]=eprom_read(dev,MAC_ADR+2) & 0xff;
        dev->dev_addr[5]=((eprom_read(dev,MAC_ADR+2) & 0xff00)>>8);

        DMESG("Card MAC address is "MAC_FMT, MAC_ARG(dev->dev_addr));

        for(i=1,j=0; i<6; i+=2,j++){

                word = eprom_read(dev,EPROM_TXPW0 + j);
                priv->chtxpwr[i]=word & 0xf;
                priv->chtxpwr_ofdm[i]=(word & 0xf0)>>4;
                priv->chtxpwr[i+1]=(word & 0xf00)>>8;
                priv->chtxpwr_ofdm[i+1]=(word & 0xf000)>>12;
        }

        for(i=1,j=0; i<4; i+=2,j++){

                word = eprom_read(dev,EPROM_TXPW1 + j);
                priv->chtxpwr[i+6]=word & 0xf;
                priv->chtxpwr_ofdm[i+6]=(word & 0xf0)>>4;
                priv->chtxpwr[i+6+1]=(word & 0xf00)>>8;
                priv->chtxpwr_ofdm[i+6+1]=(word & 0xf000)>>12;
        }

        for(i=1,j=0; i<4; i+=2,j++){

                word = eprom_read(dev,EPROM_TXPW2 + j);
                priv->chtxpwr[i+6+4]=word & 0xf;
                priv->chtxpwr_ofdm[i+6+4]=(word & 0xf0)>>4;
                priv->chtxpwr[i+6+4+1]=(word & 0xf00)>>8;
                priv->chtxpwr_ofdm[i+6+4+1]=(word & 0xf000)>>12;
        }

        priv->rf_chip = 0xff & eprom_read(dev,EPROM_RFCHIPID);

        word = eprom_read(dev,EPROM_TXPW_BASE);
        priv->cck_txpwr_base = word & 0xf;
        priv->ofdm_txpwr_base = (word>>4) & 0xf;
*/
	
//{put the card to detect different card here, mainly I/O processing
	//udev = priv->udev;
	//idProduct = le16_to_cpu(udev->descriptor.idProduct);	
	//switch (idProduct) {
// changed by thomas, use EEPROMID to identify 87B or 87L
	switch (EEPROMID) {
		//case 0x8189:
		case 0x8129:
			/* check RF frontend chipset */
			priv->card_8187 = NIC_8187B;
			priv->rf_init = rtl8225z2_rf_init;
			priv->rf_set_chan = rtl8225z2_rf_set_chan;
			priv->rf_set_sens = NULL;
			break;

		case 0x8187:
			//legacy 8187 NIC
			//{ delete break intentionly
			//break;
			//}

		default:
			/* check RF frontend chipset */
			priv->ieee80211->softmac_features |= IEEE_SOFTMAC_SINGLE_QUEUE;
			priv->card_8187 = NIC_8187;
			switch (priv->rf_chip) {
				case EPROM_RFCHIPID_RTL8225U:
					DMESG("Card reports RF frontend Realtek 8225");
					DMESGW("This driver has EXPERIMENTAL support for this chipset.");
					DMESGW("use it with care and at your own risk and");
					DMESGW("**PLEASE** REPORT SUCCESS/INSUCCESS TO Realtek");
					if(rtl8225_is_V_z2(dev)){
						priv->rf_init = rtl8225z2_rf_init;
						priv->rf_set_chan = rtl8225z2_rf_set_chan;
						priv->rf_set_sens = NULL;
						DMESG("This seems a new V2 radio");
					}else{
						priv->rf_init = rtl8225_rf_init;
						priv->rf_set_chan = rtl8225_rf_set_chan;
						priv->rf_set_sens = rtl8225_rf_set_sens;
						DMESG("This seems a legacy 1st version radio");
					}
					break;

				default:
					DMESGW("Unknown RF module %x",priv->rf_chip);
					DMESGW("Exiting...");
					return -1;
			}
			break;
	}

	priv->rf_close = rtl8225_rf_close;
	priv->max_sens = RTL8225_RF_MAX_SENS;
	priv->sens = RTL8225_RF_DEF_SENS;

	DMESG("PAPE from CONFIG2: %x",read_nic_byte(dev,CONFIG2)&0x7);
	
	if(rtl8187_usb_initendpoints(dev)!=0){ 
		DMESG("Endopoints initialization failed");
		return -ENOMEM;
	}
	
#ifdef DEBUG_EPROM
	dump_eprom(dev);
#endif 
	return 0;

}

void rtl8185_rf_pins_enable(struct net_device *dev)
{
/*	u16 tmp;
	tmp = read_nic_word(dev, RFPinsEnable);*/
	write_nic_word(dev, RFPinsEnable, 0x1ff7);// | tmp);
}


void rtl8185_set_anaparam2(struct net_device *dev, u32 a)
{
	u8 conf3;

	rtl8180_set_mode(dev, EPROM_CMD_CONFIG);

	conf3 = read_nic_byte(dev, CONFIG3);
	write_nic_byte(dev, CONFIG3, conf3 | (1<<CONFIG3_ANAPARAM_W_SHIFT));	

	write_nic_dword(dev, ANAPARAM2, a);

	conf3 = read_nic_byte(dev, CONFIG3);
	write_nic_byte(dev, CONFIG3, conf3 &~(1<<CONFIG3_ANAPARAM_W_SHIFT));

	rtl8180_set_mode(dev, EPROM_CMD_NORMAL);

}


void rtl8180_set_anaparam(struct net_device *dev, u32 a)
{
	u8 conf3;

	rtl8180_set_mode(dev, EPROM_CMD_CONFIG);

	conf3 = read_nic_byte(dev, CONFIG3);
	write_nic_byte(dev, CONFIG3, conf3 | (1<<CONFIG3_ANAPARAM_W_SHIFT));
	
	write_nic_dword(dev, ANAPARAM, a);

	conf3 = read_nic_byte(dev, CONFIG3);
	write_nic_byte(dev, CONFIG3, conf3 &~(1<<CONFIG3_ANAPARAM_W_SHIFT));

	rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
	
}


void rtl8185_tx_antenna(struct net_device *dev, u8 ant)
{
	write_nic_byte(dev, ANTSEL, ant); 
	force_pci_posting(dev);
	mdelay(1);
}	


void rtl8187_write_phy(struct net_device *dev, u8 adr, u32 data)
{
	//u8 phyr;
	u32 phyw;
//	int i;
	
	adr |= 0x80;
	 
	phyw= ((data<<8) | adr);
	
	
	
	// Note that, we must write 0xff7c after 0x7d-0x7f to write BB register. 
	write_nic_byte(dev, 0x7f, ((phyw & 0xff000000) >> 24));
	write_nic_byte(dev, 0x7e, ((phyw & 0x00ff0000) >> 16));
	write_nic_byte(dev, 0x7d, ((phyw & 0x0000ff00) >> 8));
	write_nic_byte(dev, 0x7c, ((phyw & 0x000000ff) ));

	//read_nic_dword(dev, PHY_ADR);
#if 0	
	for(i=0;i<10;i++){
		write_nic_dword(dev, PHY_ADR, 0xffffff7f & phyw);
		phyr = read_nic_byte(dev, PHY_READ);
		if(phyr == (data&0xff)) break;
			
	}
#endif
	/* this is ok to fail when we write AGC table. check for AGC table might be
	 * done by masking with 0x7f instead of 0xff
	 */
	//if(phyr != (data&0xff)) DMESGW("Phy write timeout %x %x %x", phyr, data, adr);
	mdelay(1);
}

u8 rtl8187_read_phy(struct net_device *dev,u8 adr, u32 data)
{
	u32 phyw;

	adr &= ~0x80;
	phyw= ((data<<8) | adr);
	
	
	// Note that, we must write 0xff7c after 0x7d-0x7f to write BB register. 
	write_nic_byte(dev, 0x7f, ((phyw & 0xff000000) >> 24));
	write_nic_byte(dev, 0x7e, ((phyw & 0x00ff0000) >> 16));
	write_nic_byte(dev, 0x7d, ((phyw & 0x0000ff00) >> 8));
	write_nic_byte(dev, 0x7c, ((phyw & 0x000000ff) ));
	
	return(read_nic_byte(dev,0x7e));

}

u8 read_phy_ofdm(struct net_device *dev, u8 adr)
{
	return(rtl8187_read_phy(dev, adr, 0x000000));
}

u8 read_phy_cck(struct net_device *dev, u8 adr)
{
	return(rtl8187_read_phy(dev, adr, 0x10000));
}

inline void write_phy_ofdm (struct net_device *dev, u8 adr, u32 data)
{
	data = data & 0xff;
	rtl8187_write_phy(dev, adr, data);
}

void write_phy_cck (struct net_device *dev, u8 adr, u32 data)
{
	data = data & 0xff;
	rtl8187_write_phy(dev, adr, data | 0x10000);
}

void
MacConfig_87BASIC_HardCode(struct net_device *dev)
{
	//============================================================================
	// MACREG.TXT
	//============================================================================
	int	nLinesRead = 0;
	u32	u4bRegOffset, u4bRegValue, u4bPageIndex;
	int	i;

	nLinesRead=(sizeof(MAC_REG_TABLE)/3)/4;

	for(i = 0; i < nLinesRead; i++)
	{
		u4bRegOffset=MAC_REG_TABLE[i][0]; 
		u4bRegValue=MAC_REG_TABLE[i][1]; 
		u4bPageIndex=MAC_REG_TABLE[i][2]; 

		u4bRegOffset|= (u4bPageIndex << 8);

		write_nic_byte(dev, u4bRegOffset, (u8)u4bRegValue); 
	}
	//============================================================================
}

static void MacConfig_87BASIC(struct net_device *dev)
{

	MacConfig_87BASIC_HardCode(dev);

	//============================================================================

	// Follow TID_AC_MAP of WMac.
	//PlatformEFIOWrite2Byte(dev, TID_AC_MAP, 0xfa50);
	write_nic_word(dev, TID_AC_MAP, 0xfa50);

	// Interrupt Migration, Jong suggested we use set 0x0000 first, 2005.12.14, by rcnjko.
	write_nic_word(dev, INT_MIG, 0x0000);

	// Prevent TPC to cause CRC error. Added by Annie, 2006-06-10.
	write_nic_dword(dev, 0x1F0, 0x00000000);
	write_nic_dword(dev, 0x1F4, 0x00000000);
	write_nic_byte(dev, 0x1F8, 0x00);

	// For WiFi 5.2.2.5 Atheros AP performance. Added by Annie, 2006-06-12.
	// PlatformIOWrite4Byte(dev, RFTiming, 0x0008e00f);
	// Asked for by SD3 CM Lin, 2006.06.27, by rcnjko.
	write_nic_dword(dev, RFTiming, 0x00004003);

}


//
// Description:
//		Initialize RFE and read Zebra2 version code.
//
//	2005-08-01, by Annie.
//
void
SetupRFEInitialTiming(struct net_device*  dev)
{

	// setup initial timing for RFE
	// Set VCO-PDN pin.
	write_nic_word(dev, RFPinsOutput, 0x0480);
	write_nic_word(dev, RFPinsSelect, 0x2488);
	write_nic_word(dev, RFPinsEnable, 0x1FFF);
	mdelay(100);
	//msleep(100);

	// Steven recommends: delay 1 sec for setting RF 1.8V. by Annie, 2005-04-28.
	mdelay(1000);
	//msleep(1000);

}


void ZEBRA_Config_87BASIC_HardCode(struct net_device* dev)
{
	u32			i;
	u32			addr,data;
	u32			u4bRegOffset, u4bRegValue;

	struct r8180_priv *priv = ieee80211_priv(dev);

	//=============================================================================
	// RADIOCFG.TXT
	//=============================================================================
	write_rtl8225(dev, 0x00, 0x00b7);			mdelay(1);
	write_rtl8225(dev, 0x01, 0x0ee0);			mdelay(1);
	write_rtl8225(dev, 0x02, 0x044d);			mdelay(1);
	write_rtl8225(dev, 0x03, 0x0441);			mdelay(1);
	write_rtl8225(dev, 0x04, 0x08c3);			mdelay(1);
	write_rtl8225(dev, 0x05, 0x0c72);			mdelay(1);
	write_rtl8225(dev, 0x06, 0x00e6);			mdelay(1);
	write_rtl8225(dev, 0x07, 0x082a);			mdelay(1);
	write_rtl8225(dev, 0x08, 0x003f);			mdelay(1);
	write_rtl8225(dev, 0x09, 0x0335);			mdelay(1);
	write_rtl8225(dev, 0x0a, 0x09d4);			mdelay(1);
	write_rtl8225(dev, 0x0b, 0x07bb);			mdelay(1);
	write_rtl8225(dev, 0x0c, 0x0850);			mdelay(1);
	write_rtl8225(dev, 0x0d, 0x0cdf);			mdelay(1);
	write_rtl8225(dev, 0x0e, 0x002b);			mdelay(1);
	write_rtl8225(dev, 0x0f, 0x0114);			mdelay(1);
	
	write_rtl8225(dev, 0x00, 0x01b7);			mdelay(1);


	for(i=1;i<=95;i++)
	{
		write_rtl8225(dev, 0x01, i);mdelay(1); 

		//
		// For external LNA Rx performance. 
		// Added by Roger, 2007.03.30.
		//
		if( !priv->bLowNoiseAmplifier){//Non LNA settings.
			write_rtl8225(dev, 0x02, ZEBRA2_RF_RX_GAIN_TABLE[i]); 
			mdelay(1);
		}
		else {//LNA settings.
			write_rtl8225(dev, 0x02, ZEBRA2_RF_RX_GAIN_LNA_TABLE[i]); 
			mdelay(1);
		}
		//DbgPrint("RF - 0x%x =  0x%x\n", i, ZEBRA_RF_RX_GAIN_TABLE[i]);
	}

	write_rtl8225(dev, 0x03, 0x0082);			mdelay(1); 	// write reg 18
	write_rtl8225(dev, 0x05, 0x0004);			mdelay(1);	// write reg 20
	write_rtl8225(dev, 0x00, 0x00b7);			mdelay(1);	// switch to reg0-reg15
	msleep(1000);	
	msleep(400);
	write_rtl8225(dev, 0x02, 0x0c4d);			mdelay(1);
	msleep(100);
	msleep(100);
	write_rtl8225(dev, 0x02, 0x044d);			mdelay(1); 	
	write_rtl8225(dev, 0x00, 0x02bf);			mdelay(1);	//0x002f disable 6us corner change,  06f--> enable
	
	//=============================================================================
	
	//=============================================================================
	// CCKCONF.TXT
	//=============================================================================
	/*
	u4bRegOffset=0x41;
	u4bRegValue=0xc8;
	
	//DbgPrint("\nCCK- 0x%x = 0x%x\n", u4bRegOffset, u4bRegValue);
	WriteBB(dev, (0x01000080 | (u4bRegOffset & 0x7f) | ((u4bRegValue & 0xff) << 8)));
	*/

	
	//=============================================================================

	//=============================================================================
	// Follow WMAC RTL8225_Config()
	//=============================================================================
//	//
//	// enable EEM0 and EEM1 in 9346CR
//	PlatformEFIOWrite1Byte(dev, CR9346, PlatformEFIORead1Byte(dev, CR9346)|0xc0);
//	// enable PARM_En in Config3
//	PlatformEFIOWrite1Byte(dev, CONFIG3, PlatformEFIORead1Byte(dev, CONFIG3)|0x40);	
//
//	PlatformEFIOWrite4Byte(dev, AnaParm2, ANAPARM2_ASIC_ON); //0x727f3f52
//	PlatformEFIOWrite4Byte(dev, AnaParm, ANAPARM_ASIC_ON); //0x45090658

	// power control
	write_nic_byte(dev, CCK_TXAGC, 0x03);
	write_nic_byte(dev, OFDM_TXAGC, 0x07);
	write_nic_byte(dev, ANTSEL, 0x03);

//	// disable PARM_En in Config3
//	PlatformEFIOWrite1Byte(dev, CONFIG3, PlatformEFIORead1Byte(dev, CONFIG3)&0xbf);
//	// disable EEM0 and EEM1 in 9346CR
//	PlatformEFIOWrite1Byte(dev, CR9346, PlatformEFIORead1Byte(dev, CR9346)&0x3f);
	//=============================================================================

	//=============================================================================
	// AGC.txt
	//=============================================================================
	//write_nic_dword( dev, PhyAddr, 0x00001280);	// Annie, 2006-05-05
	//write_phy_ofdm( dev, 0x00, 0x12);	// David, 2006-08-01
	//write_phy_ofdm( dev, 0x80, 0x12);	// David, 2006-08-09
	write_nic_dword(dev, PhyAddr, 0x00001280); //// Annie, 2006-05-05

	for (i=0; i<128; i++)
	{
		//DbgPrint("AGC - [%x+1] = 0x%x\n", i, ZEBRA_AGC[i+1]);
		
		data = ZEBRA2_AGC[i+1];
		data = data << 8;
		data = data | 0x0000008F;

		addr = i + 0x80; //enable writing AGC table
		addr = addr << 8;
		addr = addr | 0x0000008E;

		write_nic_dword(dev,PhyAddr, data); 	       
		write_nic_dword(dev,PhyAddr, addr); 	    
		write_nic_dword(dev,PhyAddr, 0x0000008E);
	}

	write_nic_dword(dev, PhyAddr, 0x00001080);	// Annie, 2006-05-05
	//write_phy_ofdm( dev, 0x00, 0x10);	// David, 2006-08-01
	//write_phy_ofdm( dev, 0x80, 0x10);	// David, 2006-08-09	

	//=============================================================================
	
	//=============================================================================
	// OFDMCONF.TXT
	//=============================================================================

	for(i=0; i<60; i++)
	{
		u4bRegOffset=i;

		//
		// For external LNA Rx performance. 
		// Added by Roger, 2007.03.30.
		//
		if( priv->bLowNoiseAmplifier)
		{// LNA, we use different OFDM configurations.
			u4bRegValue=OFDM_CONFIG_LNA[i];
		}
		else
		{// Non LNA, we use default OFDM configurations.			
			u4bRegValue=OFDM_CONFIG[i];
		}

		write_nic_dword(dev,PhyAddr,(0x00000080 | (u4bRegOffset & 0x7f) | ((u4bRegValue & 0xff) << 8)));
	//	write_phy_ofdm(dev,i,u4bRegValue);
	}


	//=============================================================================
}

void ZEBRA_Config_87BASIC(struct net_device *dev)
{
	ZEBRA_Config_87BASIC_HardCode(dev);
}

void PhyConfig8187(struct net_device *dev)
{
        struct r8180_priv *priv = ieee80211_priv(dev);
	u8			btConfig4;

	btConfig4 = read_nic_byte(dev, CONFIG4);
	priv->RFProgType = (btConfig4 & 0x03);

	switch(priv->rf_chip)
	{
	case RF_ZEBRA:
		break;

	case RF_ZEBRA2:
		ZEBRA_Config_87BASIC(dev);
		break;

	default:
		break;
	}

#ifdef JOHN_DIG
	if(priv->InitialGain == 0){
		priv->InitialGain = 4;
	}
	UpdateCCKThreshold(dev);
	UpdateInitialGain(dev);
#endif /*JOHN_DIG*/

	return ;
}

u8 GetSupportedWirelessMode8187(struct net_device* dev)
{
	u8 btSupportedWirelessMode;
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	btSupportedWirelessMode = 0;
	
	switch(priv->rf_chip)
	{
		case RF_ZEBRA:
		case RF_ZEBRA2:
			btSupportedWirelessMode = (WIRELESS_MODE_B | WIRELESS_MODE_G);
			break;
		default:
			btSupportedWirelessMode = WIRELESS_MODE_B;
			break;
	}
	return btSupportedWirelessMode;
}

#if 0
void Set_HW_ACPARAM(struct net_device *dev,
		PAC_PARAM pAcParam, 
		PCHANNEL_ACCESS_SETTING ChnlAccessSetting)
{
	AC_CODING		eACI;
	u8				u1bAIFS;
	u32				u4bAcParam;
	PACI_AIFSN		pAciAifsn	= (PACI_AIFSN)(&pAcParam->f.AciAifsn);
	struct r8180_priv	*priv		= ieee80211_priv(dev);
	u8				AcmCtrl		= priv->AcmControl;
	

	// Retrive paramters to udpate.
	eACI		= pAcParam->f.AciAifsn.f.ACI; 
	u1bAIFS		= pAcParam->f.AciAifsn.f.AIFSN * ChnlAccessSetting->SlotTimeTimer + aSifsTime -14; 
	u4bAcParam	= (	(((u32)(pAcParam->f.TXOPLimit)) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
					(((u32)(pAcParam->f.Ecw.f.ECWmax)) << AC_PARAM_ECW_MAX_OFFSET)	|  
					(((u32)(pAcParam->f.Ecw.f.ECWmin)) << AC_PARAM_ECW_MIN_OFFSET)	|  
					(((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET));

	switch(eACI)
	{
		case AC1_BK:
			write_nic_dword(dev, AC_BK_PARAM, u4bAcParam);
			break;

		case AC0_BE:
			write_nic_dword(dev, AC_BE_PARAM, u4bAcParam);
			break;

		case AC2_VI:
			write_nic_dword(dev, AC_VI_PARAM, u4bAcParam);
			break;

		case AC3_VO:
			write_nic_dword(dev, AC_VO_PARAM, u4bAcParam);
			break;

		default:
			printk(KERN_WARNING "SetHwReg8185(): invalid ACI: %d !\n", eACI);
			break;
	}

	// Cehck ACM bit.
	// If it is set, immediately set ACM control bit to downgrading AC for passing WMM testplan. Annie, 2005-12-13.

	if( pAciAifsn->f.ACM )
	{ // ACM bit is 1.
		switch(eACI)
		{
			case AC0_BE:
				AcmCtrl |= (BEQ_ACM_EN|BEQ_ACM_CTL|ACM_HW_EN);	// or 0x21
				break;

			case AC2_VI:
				AcmCtrl |= (VIQ_ACM_EN|VIQ_ACM_CTL|ACM_HW_EN);	// or 0x42
				break;

			case AC3_VO:
				AcmCtrl |= (VOQ_ACM_EN|VOQ_ACM_CTL|ACM_HW_EN);	// or 0x84
				break;

			default:
				printk(KERN_WARNING "SetHwReg8185(): [HW_VAR_ACM_CTRL] ACM set\
									failed: eACI is %d\n", eACI );
				break;
		}
	}
	else
	{ // ACM bit is 0.
		switch(eACI)
		{
			case AC0_BE:
				AcmCtrl &= ( (~BEQ_ACM_EN) & (~BEQ_ACM_CTL) & (~ACM_HW_EN) );	// and 0xDE
				break;

			case AC2_VI:
				AcmCtrl &= ( (~VIQ_ACM_EN) & (~VIQ_ACM_CTL) & (~ACM_HW_EN) );	// and 0xBD
				break;

			case AC3_VO:
				AcmCtrl &= ( (~VOQ_ACM_EN) & (~VOQ_ACM_CTL) & (~ACM_HW_EN) );	// and 0x7B
				break;

			default:
				break;
		}
	}

	priv->AcmControl = AcmCtrl;

	write_nic_byte(dev, ACM_CONTROL, AcmCtrl);

}

void ActUpdateChannelAccessSetting(struct net_device *dev,
		int WirelessMode, 
		PCHANNEL_ACCESS_SETTING ChnlAccessSetting)
{
	AC_CODING	eACI;
	AC_PARAM	AcParam;
#ifdef TODO	
	PSTA_QOS	pStaQos = Adapter->MgntInfo.pStaQos;
#endif	
	//_BOOL		bFollowLegacySetting = _FALSE;
	struct r8180_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;

	switch( WirelessMode )
	{
		case WIRELESS_MODE_A:
			ChnlAccessSetting->SIFS_Timer = 0x22;
			ChnlAccessSetting->DIFS_Timer = 20;
			ChnlAccessSetting->SlotTimeTimer = 9;
			ChnlAccessSetting->EIFS_Timer = 23;
			ChnlAccessSetting->CWminIndex = 4;
			ChnlAccessSetting->CWmaxIndex = 10;
			break;

		case WIRELESS_MODE_B:
			ChnlAccessSetting->SIFS_Timer = 0x22;
			ChnlAccessSetting->DIFS_Timer = 36;
			ChnlAccessSetting->SlotTimeTimer = 20;
			ChnlAccessSetting->EIFS_Timer = 91;
			ChnlAccessSetting->CWminIndex = 5;
			ChnlAccessSetting->CWmaxIndex = 10;
			break;

		case WIRELESS_MODE_G:
			ChnlAccessSetting->SIFS_Timer = 0x22;
			ChnlAccessSetting->DIFS_Timer = 20;
			ChnlAccessSetting->SlotTimeTimer = 20;
			ChnlAccessSetting->EIFS_Timer = 91;
#ifdef TODO
			switch (Adapter->NdisUsbDev.CWinMaxMin)
#else
			switch (2)				
#endif
			{
				case 0:// 0: [max:7 min:1 ]
					ChnlAccessSetting->CWminIndex = 1;
					ChnlAccessSetting->CWmaxIndex = 7;
					break;
				case 1:// 1: [max:7 min:2 ]
					ChnlAccessSetting->CWminIndex = 2;
					ChnlAccessSetting->CWmaxIndex = 7;
					break;
				case 2:// 2: [max:7 min:3 ]
					ChnlAccessSetting->CWminIndex = 3;
					ChnlAccessSetting->CWmaxIndex = 7;
					break;
				case 3:// 3: [max:9 min:1 ]
					ChnlAccessSetting->CWminIndex = 1;
					ChnlAccessSetting->CWmaxIndex = 9;
					break;
				case 4:// 4: [max:9 min:2 ]
					ChnlAccessSetting->CWminIndex = 2;
					ChnlAccessSetting->CWmaxIndex = 9;
					break;
				case 5:// 5: [max:9 min:3 ]
					ChnlAccessSetting->CWminIndex = 3;
					ChnlAccessSetting->CWmaxIndex = 9;
					break;
				case 6:// 6: [max:A min:5 ]
					ChnlAccessSetting->CWminIndex = 5;
					ChnlAccessSetting->CWmaxIndex = 10;
					break;
				case 7:// 7: [max:A min:4 ]
					ChnlAccessSetting->CWminIndex = 4;
					ChnlAccessSetting->CWmaxIndex = 10;
					break;

				default:
					ChnlAccessSetting->CWminIndex = 1;
					ChnlAccessSetting->CWmaxIndex = 7;
					break;
			}

			// Use the CW defined in SPEC to prevent unfair Beacon distribution in AdHoc mode.
			// 2006.02.16, by rcnjko
			if( priv->ieee80211->iw_mode == IW_MODE_ADHOC)
			{
				ChnlAccessSetting->CWminIndex = 5;
				ChnlAccessSetting->CWmaxIndex= 10;
			}

			break;
	}

	write_nic_byte(dev, SIFS, ChnlAccessSetting->SIFS_Timer);
	//update slot time related by david, 2006-7-21
	write_nic_byte(dev, SLOT, ChnlAccessSetting->SlotTimeTimer);	// Rewrited from directly use PlatformEFIOWrite1Byte(), by Annie, 2006-03-29.
#ifdef TODO
	if(pStaQos->CurrentQosMode > QOS_DISABLE)
	{
		for(eACI = 0; eACI < AC_MAX; eACI++)
		{
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, \
				(pu1Byte)(&(pStaQos->WMMParamEle.AcParam[eACI])) );
		}
	}
	else
#endif		
	{
		u8 u1bAIFS = aSifsTime + (2 * ChnlAccessSetting->SlotTimeTimer ) - 14;//PHY_DELAY is 14

		write_nic_byte(dev, AC_VO_PARAM, u1bAIFS);
		write_nic_byte(dev, AC_VI_PARAM, u1bAIFS);
		write_nic_byte(dev, AC_BE_PARAM, u1bAIFS);
		write_nic_byte(dev, AC_BK_PARAM, u1bAIFS);
	}

	write_nic_byte(dev, EIFS_8187B, ChnlAccessSetting->EIFS_Timer);
	write_nic_byte(dev, AckTimeOutReg, 0x5B); // <RJ_EXPR_QOS> Suggested by wcchu, it is the default value of EIFS register, 2005.12.08.
#ifdef TODO
	// <RJ_TODO_NOW_8185B> Update ECWmin/ECWmax, AIFS, TXOP Limit of each AC to the value defined by SPEC.
	if( pStaQos->CurrentQosMode > QOS_DISABLE )
	{ // QoS mode.
		if(pStaQos->QBssWirelessMode == WirelessMode)
		{
			// Follow AC Parameters of the QBSS.
			for(eACI = 0; eACI < AC_MAX; eACI++)
			{
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, (pu1Byte)(&(pStaQos->WMMParamEle.AcParam[eACI])) );
			}
		}
		else
		{
			// Follow Default WMM AC Parameters.
			bFollowLegacySetting = TRUE;
		}
	}
	else
	{ // Legacy 802.11.
		bFollowLegacySetting = TRUE;
	}

	if(bFollowLegacySetting)
#endif

	if(_TRUE)
	{
		//
		// Follow 802.11 seeting to AC parameter, all AC shall use the same parameter.
		// 2005.12.01, by rcnjko.
		//
		AcParam.longData = 0;
		AcParam.f.AciAifsn.f.AIFSN = 2; // Follow 802.11 DIFS.
		AcParam.f.AciAifsn.f.ACM = 0;
		AcParam.f.Ecw.f.ECWmin = ChnlAccessSetting->CWminIndex; // Follow 802.11 CWmin.
		AcParam.f.Ecw.f.ECWmax = ChnlAccessSetting->CWmaxIndex; // Follow 802.11 CWmax.
		AcParam.f.TXOPLimit = 0;
		//for(eACI = 0; eACI < AC_MAX; eACI++)
		for(eACI = 0; eACI < AC3_VO; eACI++)
		{
			AcParam.f.AciAifsn.f.ACI = (u8)eACI;
			{
				Set_HW_ACPARAM(dev,&AcParam,ChnlAccessSetting);
#if 0
				PAC_PARAM	pAcParam = (PAC_PARAM)(&AcParam);
				AC_CODING	eACI;
				u8		u1bAIFS;
				u32		u4bAcParam;

				// Retrive paramters to udpate.
				eACI = pAcParam->f.AciAifsn.f.ACI; 
				u1bAIFS = pAcParam->f.AciAifsn.f.AIFSN * ChnlAccessSetting->SlotTimeTimer + aSifsTime -14; 
				u4bAcParam = (	(((u32)(pAcParam->f.TXOPLimit)) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
						(((u32)(pAcParam->f.Ecw.f.ECWmax)) << AC_PARAM_ECW_MAX_OFFSET)	|  
						(((u32)(pAcParam->f.Ecw.f.ECWmin)) << AC_PARAM_ECW_MIN_OFFSET)	|  
						(((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET));

				switch(eACI)
				{
					case AC1_BK:
						write_nic_dword(dev, AC_BK_PARAM, u4bAcParam);
						break;

					case AC0_BE:
						write_nic_dword(dev, AC_BE_PARAM, u4bAcParam);
						break;

					case AC2_VI:
						write_nic_dword(dev, AC_VI_PARAM, u4bAcParam);
						break;

					case AC3_VO:
						write_nic_dword(dev, AC_VO_PARAM, u4bAcParam);
						break;

					default:
						printk(KERN_WARNING "SetHwReg8185(): invalid ACI: %d !\n", eACI);
						break;
				}

				// Cehck ACM bit.
				// If it is set, immediately set ACM control bit to downgrading AC for passing WMM testplan. Annie, 2005-12-13.
					PACI_AIFSN	pAciAifsn = (PACI_AIFSN)(&pAcParam->f.AciAifsn);
					AC_CODING	eACI = pAciAifsn->f.ACI;

                                        u8      AcmCtrl = priv->AcmControl;

					if( pAciAifsn->f.ACM )
					{ // ACM bit is 1.
						switch(eACI)
						{
							case AC0_BE:
								AcmCtrl |= (BEQ_ACM_EN|BEQ_ACM_CTL|ACM_HW_EN);	// or 0x21
								break;

							case AC2_VI:
								AcmCtrl |= (VIQ_ACM_EN|VIQ_ACM_CTL|ACM_HW_EN);	// or 0x42
								break;

							case AC3_VO:
								AcmCtrl |= (VOQ_ACM_EN|VOQ_ACM_CTL|ACM_HW_EN);	// or 0x84
								break;

							default:
								printk(KERN_WARNING "SetHwReg8185(): [HW_VAR_ACM_CTRL] ACM set\
										failed: eACI is %d\n", eACI );
								break;
						}
					}
					else
					{ // ACM bit is 0.
						switch(eACI)
						{
							case AC0_BE:
								AcmCtrl &= ( (~BEQ_ACM_EN) & (~BEQ_ACM_CTL) & (~ACM_HW_EN) );	// and 0xDE
								break;

							case AC2_VI:
								AcmCtrl &= ( (~VIQ_ACM_EN) & (~VIQ_ACM_CTL) & (~ACM_HW_EN) );	// and 0xBD
								break;

							case AC3_VO:
								AcmCtrl &= ( (~VOQ_ACM_EN) & (~VOQ_ACM_CTL) & (~ACM_HW_EN) );	// and 0x7B
								break;

							default:
								break;
						}
					}

					priv->AcmControl = AcmCtrl;

					write_nic_byte(dev, ACM_CONTROL, AcmCtrl);
#endif
			}
		}

		// Use the CW defined in SPEC to prevent unfair Beacon distribution in AdHoc mode.
		// Only set this value to AC3_VO because the throughput shakes if other ACs set this value.
		// 2006.08.04, by shien chang.
		AcParam.f.AciAifsn.f.ACI = (u8)AC3_VO;		
		if( ieee->iw_mode == IW_MODE_ADHOC ) {
			AcParam.f.Ecw.f.ECWmin = 4; 
			AcParam.f.Ecw.f.ECWmax = 10;
		}
		Set_HW_ACPARAM(dev,&AcParam,ChnlAccessSetting);
	}

}
#endif

void Set_HW_ACPARAM(struct net_device *dev,
		PAC_PARAM pAcParam, 
		PCHANNEL_ACCESS_SETTING ChnlAccessSetting)
{
	AC_CODING		eACI;
	u8				u1bAIFS;
	u32				u4bAcParam;
	PACI_AIFSN		pAciAifsn	= (PACI_AIFSN)(&pAcParam->f.AciAifsn);
	struct r8180_priv	*priv		= ieee80211_priv(dev);
	u8				AcmCtrl		= priv->AcmControl;
	

	// Retrive paramters to udpate.
	eACI		= pAcParam->f.AciAifsn.f.ACI; 
	u1bAIFS		= pAcParam->f.AciAifsn.f.AIFSN * ChnlAccessSetting->SlotTimeTimer + aSifsTime;
	u4bAcParam	= (	(((u32)(pAcParam->f.TXOPLimit)) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
					(((u32)(pAcParam->f.Ecw.f.ECWmax)) << AC_PARAM_ECW_MAX_OFFSET)	|  
					(((u32)(pAcParam->f.Ecw.f.ECWmin)) << AC_PARAM_ECW_MIN_OFFSET)	|  
					(((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET));

	switch(eACI)
	{
		case AC1_BK:
			write_nic_dword(dev, AC_BK_PARAM, u4bAcParam);
			break;

		case AC0_BE:
			write_nic_dword(dev, AC_BE_PARAM, u4bAcParam);
			break;

		case AC2_VI:
			write_nic_dword(dev, AC_VI_PARAM, u4bAcParam);
			break;

		case AC3_VO:
			write_nic_dword(dev, AC_VO_PARAM, u4bAcParam);
			break;

		default:
			printk(KERN_WARNING "SetHwReg8185(): invalid ACI: %d !\n", eACI);
			break;
	}

	// Cehck ACM bit.
	// If it is set, immediately set ACM control bit to downgrading AC for passing WMM testplan. Annie, 2005-12-13.

	if( pAciAifsn->f.ACM )
	{ // ACM bit is 1.
		switch(eACI)
		{
			case AC0_BE:
				AcmCtrl |= (BEQ_ACM_EN|BEQ_ACM_CTL);	// or 0x21
				break;

			case AC2_VI:
				AcmCtrl |= (VIQ_ACM_EN|VIQ_ACM_CTL);	// or 0x42
				break;

			case AC3_VO:
				AcmCtrl |= (VOQ_ACM_EN|VOQ_ACM_CTL);	// or 0x84
				break;

			default:
				printk(KERN_WARNING "SetHwReg8185(): [HW_VAR_ACM_CTRL] ACM set failed: eACI is %d\n", eACI );
				break;
		}
	}
	else
	{ // ACM bit is 0.
		switch(eACI)
		{
			case AC0_BE:
				AcmCtrl &= ( (~BEQ_ACM_EN) & (~BEQ_ACM_CTL) );	// and 0xDE
				break;

			case AC2_VI:
				AcmCtrl &= ( (~VIQ_ACM_EN) & (~VIQ_ACM_CTL) );	// and 0xBD
				break;

			case AC3_VO:
				AcmCtrl &= ( (~VOQ_ACM_EN) & (~VOQ_ACM_CTL) );	// and 0x7B
				break;

			default:
				break;
		}
	}

	priv->AcmControl = AcmCtrl;

	write_nic_byte(dev, ACM_CONTROL, AcmCtrl);

}


void ActUpdateChannelAccessSetting(
	struct net_device			*dev,
	WIRELESS_MODE			WirelessMode,
	PCHANNEL_ACCESS_SETTING	ChnlAccessSetting
	)
{
		AC_CODING	eACI;
		AC_PARAM	AcParam;
		//PSTA_QOS	pStaQos = Adapter->MgntInfo.pStaQos;
		_BOOL		bFollowLegacySetting = FALSE;

		//
		// <RJ_TODO_8185B> 
		// TODO: We still don't know how to set up these registers, just follow WMAC to 
		// verify 8185B FPAG.
		//
		// <RJ_TODO_8185B>
		// Jong said CWmin/CWmax register are not functional in 8185B, 
		// so we shall fill channel access realted register into AC parameter registers,
		// even in nQBss.
		//
		ChnlAccessSetting->SIFS_Timer = 0x22; // Suggested by Jong, 2005.12.08.
		ChnlAccessSetting->DIFS_Timer = 0x32;
		ChnlAccessSetting->SlotTimeTimer = 20;
		ChnlAccessSetting->EIFS_Timer = 0x5B; // Suggested by wcchu, it is the default value of EIFS register, 2005.12.08.
		ChnlAccessSetting->CWminIndex = 5;
		ChnlAccessSetting->CWmaxIndex = 10;

		write_nic_byte(dev, SIFS, ChnlAccessSetting->SIFS_Timer);
		write_nic_byte(dev, DIFS, ChnlAccessSetting->DIFS_Timer); // <RJ_TODO_8185B> We don't known if DIFS in 8185B still works. 
		write_nic_byte(dev, SLOT, ChnlAccessSetting->SlotTimeTimer);// Rewrited from directly use PlatformEFIOWrite1Byte(), by Annie, 2006-03-29.
		write_nic_byte(dev, EIFS_8187B, ChnlAccessSetting->EIFS_Timer);

		write_nic_byte(dev, AckTimeOutReg, 0x5B); // <RJ_EXPR_QOS> Suggested by wcchu, it is the default value of EIFS register, 2005.12.08.

#ifdef TODO
		// <RJ_TODO_NOW_8185B> Update ECWmin/ECWmax, AIFS, TXOP Limit of each AC to the value defined by SPEC.
		if( pStaQos->CurrentQosMode > QOS_DISABLE )
		{ // QoS mode.
			if(pStaQos->QBssWirelessMode == WirelessMode)
			{
				// Follow AC Parameters of the QBSS.
				for(eACI = 0; eACI < AC_MAX; eACI++)
				{
					Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, (pu1Byte)(&(pStaQos->WMMParamEle.AcParam[eACI])) );
				}
			}
			else
			{
				// Follow Default WMM AC Parameters.
				bFollowLegacySetting = TRUE;
			}
		}
		else
#endif
		{ // Legacy 802.11.
			bFollowLegacySetting = TRUE;

		}

		if(bFollowLegacySetting)
		{
			//
			// Follow 802.11 seeting to AC parameter, all AC shall use the same parameter.
			// 2005.12.01, by rcnjko.
			//
			AcParam.longData = 0;
			AcParam.f.AciAifsn.f.AIFSN = 2; // Follow 802.11 DIFS.
			AcParam.f.AciAifsn.f.ACM = 0;
			AcParam.f.Ecw.f.ECWmin = ChnlAccessSetting->CWminIndex; // Follow 802.11 CWmin.
			AcParam.f.Ecw.f.ECWmax = ChnlAccessSetting->CWmaxIndex; // Follow 802.11 CWmax.
			AcParam.f.TXOPLimit = 0;
			for(eACI = 0; eACI < AC_MAX; eACI++)
			{
				AcParam.f.AciAifsn.f.ACI = (u8)eACI;
				Set_HW_ACPARAM(dev,&AcParam,ChnlAccessSetting);
			}
		}
}


void ActSetWirelessMode8187(struct net_device* dev, u8	btWirelessMode)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	//PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u8			btSupportedWirelessMode = GetSupportedWirelessMode8187(dev);

	if( (btWirelessMode & btSupportedWirelessMode) == 0 )
	{ // Don't switch to unsupported wireless mode, 2006.02.15, by rcnjko.
		printk(KERN_WARNING "ActSetWirelessMode8187(): WirelessMode(%d) is not supported (%d)!\n", 
				 btWirelessMode, btSupportedWirelessMode);
		return;
	}

	// 1. Assign wireless mode to swtich if necessary.
	if( (btWirelessMode == WIRELESS_MODE_AUTO) || 
			(btWirelessMode & btSupportedWirelessMode) == 0 )
	{
		if((btSupportedWirelessMode & WIRELESS_MODE_A))
		{
			btWirelessMode = WIRELESS_MODE_A;
		}
		else if((btSupportedWirelessMode & WIRELESS_MODE_G))
		{
			btWirelessMode = WIRELESS_MODE_G;
		}
		else if((btSupportedWirelessMode & WIRELESS_MODE_B))
		{
			btWirelessMode = WIRELESS_MODE_B;
		}
		else
		{
			printk(KERN_WARNING "MptActSetWirelessMode8187(): No valid wireless mode supported, \
					btSupportedWirelessMode(%x)!!!\n", btSupportedWirelessMode);
			btWirelessMode = WIRELESS_MODE_B;
		}
	}

	// 2. Swtich band.
	switch(priv->rf_chip)
	{
		case RF_ZEBRA:
		case RF_ZEBRA2:
			{
				// Update current wireless mode if we swtich to specified band successfully.
				ieee->mode = (WIRELESS_MODE)btWirelessMode;
			}
			break;

		default:
			printk(KERN_WARNING "MptActSetWirelessMode8187(): unsupported RF: 0x%X !!!\n", priv->rf_chip);
			break;
	}

	// 4. Change related setting.
	if( ieee->mode == WIRELESS_MODE_A ){
		printk(KERN_WARNING "WIRELESS_MODE_A\n"); 
	}
	else if(ieee->mode == WIRELESS_MODE_B ){
		printk(KERN_WARNING "WIRELESS_MODE_B\n"); 
	}
	else if( ieee->mode == WIRELESS_MODE_G ){
		printk(KERN_WARNING "WIRELESS_MODE_G\n"); 
	}
	ActUpdateChannelAccessSetting(dev, ieee->mode, &priv->ChannelAccessSetting );
#ifdef TODO	
	pAdapter->MgntInfo.dot11CurrentWirelessMode = pHalData->CurrentWirelessMode;
	MgntSetRegdot11OperationalRateSet( pAdapter );
#endif

#ifdef JOHN_DIG
	if(ieee->mode == WIRELESS_MODE_B && priv->InitialGain > priv->RegBModeGainStage){
		priv->InitialGain = priv->RegBModeGainStage;
		UpdateInitialGain(dev);
	}
#endif /*JOHN_DIG*/

}


void
InitializeExtraRegsOn8185(struct net_device	*dev)
{
	//RTL8185_TODO: Determine Retrylimit, TxAGC, AutoRateFallback control.
	_BOOL		bUNIVERSAL_CONTROL_RL = FALSE; // Enable per-packet tx retry, 2005.03.31, by rcnjko.
	_BOOL		bUNIVERSAL_CONTROL_AGC = TRUE;
	_BOOL		bUNIVERSAL_CONTROL_ANT = TRUE;
	_BOOL		bAUTO_RATE_FALLBACK_CTL = TRUE;
	u8		val8;

	// Set up ACK rate.
	// Suggested by wcchu, 2005.08.25, by rcnjko.
	// 1. Initialize (MinRR, MaxRR) to (6,24) for A/G.
	// 2. MUST Set RR before BRSR.
	// 3. CCK must be basic rate.
	//if((ieee->mode == IEEE_G)||(ieee->mode == IEEE_A))
	{
		write_nic_word(dev, BRSR_8187B, 0x0fff);
	}
	//else
	//{
	//	write_nic_word(dev, BRSR_8187B, 0x000f);
	//}

	// Retry limit
	val8 = read_nic_byte(dev, CW_CONF);
	if(bUNIVERSAL_CONTROL_RL)
	{
		val8 &= (~CW_CONF_PERPACKET_RETRY_LIMIT); 
	}
	else
	{
		val8 |= CW_CONF_PERPACKET_RETRY_LIMIT; 
	}
	write_nic_byte(dev, CW_CONF, val8);

	// Tx AGC
	val8 = read_nic_byte(dev, TX_AGC_CTL);
	if(bUNIVERSAL_CONTROL_AGC)
	{
		val8 &= (~TX_AGC_CTL_PER_PACKET_TXAGC);
		write_nic_byte(dev, CCK_TXAGC, 128);
		write_nic_byte(dev, OFDM_TXAGC, 128);
	}
	else
	{
		val8 |= TX_AGC_CTL_PER_PACKET_TXAGC;
	}
	write_nic_byte(dev, TX_AGC_CTL, val8);

	// Tx Antenna including Feedback control
	val8 = read_nic_byte(dev, TX_AGC_CTL);

	if(bUNIVERSAL_CONTROL_ANT)
	{
		write_nic_byte(dev, ANTSEL, 0x00);
		val8 &= (~TXAGC_CTL_PER_PACKET_ANT_SEL);
	}
	else
	{
		val8 |= TXAGC_CTL_PER_PACKET_ANT_SEL;
	}
	write_nic_byte(dev, TX_AGC_CTL, val8);

	// Auto Rate fallback control
	val8 = read_nic_byte(dev, RATE_FALLBACK);
	if( bAUTO_RATE_FALLBACK_CTL )
	{
		val8 |= RATE_FALLBACK_CTL_ENABLE | RATE_FALLBACK_CTL_AUTO_STEP0;

		// <RJ_TODO_8187B> We shall set up the ARFR according to user's setting.
		write_nic_word(dev, ARFR, 0x0fff); // set 1M ~ 54M
	}
	else
	{
		val8 &= (~RATE_FALLBACK_CTL_ENABLE);
	}
	write_nic_byte(dev, RATE_FALLBACK, val8);

}


void rtl8180_adapter_start(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	u8			InitWirelessMode;
	u8			SupportedWirelessMode;
	_BOOL			bInvalidWirelessMode = _FALSE;	
	u8			TmpU1b;
	u32			TmpU4b;


	if(NIC_8187 == priv->card_8187) {
		//rtl8180_rtx_disable(dev);
		rtl8180_reset(dev);

		write_nic_byte(dev,0x85,0);
		write_nic_byte(dev,0x91,0);

		/* light blink! */
		write_nic_byte(dev,0x85,4);
		write_nic_byte(dev,0x91,1);
		write_nic_byte(dev,0x90,0);
		/*	
			write_nic_byte(dev, CR9346, 0xC0);
		//LED TYPE
		write_nic_byte(dev, CONFIG1,((read_nic_byte(dev, CONFIG1)&0x3f)|0x80));	//turn on bit 5:Clkrun_mode
		write_nic_byte(dev, CR9346, 0x0);	// disable config register write
		*/	
		priv->irq_mask = 0xffff;
		/*
		   priv->dma_poll_mask = 0;
		   priv->dma_poll_mask|= (1<<TX_DMA_STOP_BEACON_SHIFT);
		   */	
		//	rtl8180_beacon_tx_disable(dev);

		rtl8180_set_mode(dev, EPROM_CMD_CONFIG);
		write_nic_dword(dev, MAC0, ((u32*)dev->dev_addr)[0]);
		write_nic_word(dev, MAC4, ((u32*)dev->dev_addr)[1] & 0xffff );

		rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
		rtl8180_update_msr(dev);

		rtl8180_set_mode(dev, EPROM_CMD_CONFIG);

		write_nic_word(dev,0xf4,0xffff);
		write_nic_byte(dev,
				CONFIG1, (read_nic_byte(dev,CONFIG1) & 0x3f) | 0x80);	

		rtl8180_set_mode(dev,EPROM_CMD_NORMAL);

		write_nic_dword(dev,INT_TIMEOUT,0);	

#ifdef DEBUG_REGISTERS
		rtl8180_dump_reg(dev);	
#endif


		write_nic_byte(dev, WPA_CONFIG, 0);	

		write_nic_byte(dev, RATE_FALLBACK, 0x81);
		rtl8187_set_rate(dev);

		priv->rf_init(dev);	

		if(priv->rf_set_sens != NULL) {
			priv->rf_set_sens(dev,priv->sens);	
		}

		write_nic_word(dev,0x5e,1);
		//mdelay(1);
		write_nic_word(dev,0xfe,0x10);
		//	mdelay(1);
		write_nic_byte(dev, TALLY_SEL, 0x80);//Set NQ retry count
		write_nic_byte(dev, 0xff, 0x60);
		write_nic_word(dev,0x5e,0);

		rtl8180_irq_enable(dev);
	} else {
		u8 i;
		/**
		 *  IO part migrated from Windows Driver.
		 */
		//priv->irq_mask = 0xffff;
		// Enable Config3.PARAM_En.
		write_nic_byte(dev, CR9346, 0xC0);
		TmpU1b = read_nic_byte(dev, CONFIG3);		
		write_nic_byte(dev, CONFIG3, (TmpU1b| CONFIG3_PARM_En));
		// Turn on Analog power.
		// Asked for by William, otherwise, MAC 3-wire can't work, 2006.06.27, by rcnjko.
		write_nic_dword(dev, ANA_PARAM2, ANAPARM2_ASIC_ON);
		write_nic_dword(dev, ANA_PARAM, ANAPARM_ASIC_ON);
		write_nic_byte(dev, ANA_PARAM3, 0x00);

		// 070111: Asked for by William to reset PLL.
		/*{
			u8 Reg62;
			write_nic_byte(dev, 0xff61, 0x10);
			Reg62 = read_nic_byte(dev, 0xff62);
			write_nic_byte(dev, 0xff62, (Reg62 & (~(BIT5))) );
			write_nic_byte(dev, 0xff62, (Reg62 | (BIT5)) );
		}*/

		write_nic_byte(dev, CONFIG3, (TmpU1b&(~CONFIG3_PARM_En)));
		write_nic_byte(dev, CR9346, 0x00);

		//-----------------------------------------------------------------------------
		// Set up MAC registers. 
		//-----------------------------------------------------------------------------

		//
		// Init multicast register to recv All.
		//
		if(read_nic_byte_E(dev, 0xFE11) & 0x01)
			priv->usbhighspeed = _TRUE;
		else
			priv->usbhighspeed = _FALSE;
		
		//2Init multicast register to recv All		
		write_nic_dword(dev, 0x210, 0xffffffff);
		write_nic_dword(dev, 0x214, 0xffffffff);

		for(i=0 ; i<6 ; i++)
			TmpU1b= read_nic_byte(dev, IDR0+i);

		for(i=0 ; i<6 ; i++)
			write_nic_byte(dev, IDR0+i, dev->dev_addr[i]);		

		TmpU4b = read_nic_dword(dev, ANA_PARAM);
		TmpU4b = read_nic_dword(dev, ANA_PARAM2);

		//InitPsr = read_nic_byte(dev, PSR);

		// Save target channel
		priv->chan = 1; //set to default initial
		
		// Issue Tx/Rx reset
		write_nic_byte(dev,CMD,CR_RST);
		// wait after the port reset command
		//PlatformStallExecution(20); xp delay 20us ?
		//mdelay(200); //linux delay 200ms 
		udelay(20);

		write_nic_byte(dev, CR9346, 0xc0);	// enable config register write

		InitializeExtraRegsOn8185(dev);

		write_nic_dword(dev, TCR, priv->TransmitConfig);
		TmpU4b = read_nic_dword(dev,TCR);
		//DMESG("RTL8187 init TCR:%x\n",TmpU4b);

		write_nic_dword(dev, RCR, priv->ReceiveConfig);
		TmpU4b = read_nic_dword(dev,RCR);
		//DMESG("RTL8187 init RCR:%x\n",TmpU4b);
		 
		write_nic_byte(dev, MSR, read_nic_byte(dev,MSR) & 0xf3);	// default network type to 'No  Link'
		
		write_nic_byte(dev, ACM_CONTROL, priv->AcmControl);	

		write_nic_word(dev, BcnIntv, 100);
		write_nic_word(dev, AtimWnd, 2);

		write_nic_word(dev, FEMR, 0xFFFF);

		write_nic_byte(dev, CR9346, 0x0);	// disable config register write

		MacConfig_87BASIC(dev);

		// Override the RFSW_CTRL (MAC offset 0x272-0x273), 2006.06.07, by rcnjko.
		write_nic_word(dev, RFSW_CTRL, 0x569a);		

		//-----------------------------------------------------------------------------

		//-----------------------------------------------------------------------------
		// Set up security related. 070106, by rcnjko:
		// 1. Clear all H/W keys.
		// 2. Enable H/W encryption/decryption.
		//-----------------------------------------------------------------------------
		//CamResetAllEntry(Adapter);
		//Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);
#ifdef JOHN_TKIP
		CamResetAllEntry(dev);		
		EnableHWSecurityConfig8187(dev);
		write_nic_word(dev, AESMSK_FC, AESMSK_FC_DEFAULT );	mdelay(1);
		write_nic_word(dev, AESMSK_SC, AESMSK_SC_DEFAULT );	mdelay(1);

		// Write AESMSK_QC (Mask for Qos Control Field). Annie, 2005-12-14.
		write_nic_word(dev, AESMSK_QC, AESMSK_QC_DEFAULT );	mdelay(1);
#endif
		//-----------------------------------------------------------------------------

		//-----------------------------------------------------------------------------
		// Set up PHY related. 
		//-----------------------------------------------------------------------------
		// Enable Config3.PARAM_En to revise AnaaParm.
		write_nic_byte(dev, CR9346, 0xC0);
		//write_nic_byte(dev, CONFIG3, read_nic_byte(dev,CONFIG3)|CONFIG3_PARM_En);
		write_nic_byte(dev, CONFIG3, CONFIG3_PARM_En);
		write_nic_byte(dev, CR9346, 0x0);

		// Initialize RFE and read Zebra2 version code. Added by Annie, 2005-08-01.
		SetupRFEInitialTiming(dev);

		// PHY config.
		PhyConfig8187(dev);

		// We assume RegWirelessMode has already been initialized before, 
		// however, we has to validate the wireless mode here and provide a reasonble
		// initialized value if necessary. 2005.01.13, by rcnjko.
		SupportedWirelessMode = GetSupportedWirelessMode8187(dev);
		if((ieee->mode != WIRELESS_MODE_B) && 
				(ieee->mode != WIRELESS_MODE_G) &&
				(ieee->mode != WIRELESS_MODE_A) &&
				(ieee->mode != WIRELESS_MODE_AUTO)) 
		{ // It should be one of B, G, A, or AUTO. 
			bInvalidWirelessMode = _TRUE;
		}
		else
		{ // One of B, G, A, or AUTO.
			// Check if the wireless mode is supported by RF.
			if( (ieee->mode != WIRELESS_MODE_AUTO) &&
					(ieee->mode & SupportedWirelessMode) == 0 )
			{
				bInvalidWirelessMode = _TRUE;
			}
		}

		if(bInvalidWirelessMode || ieee->mode==WIRELESS_MODE_AUTO)
		{ // Auto or other invalid value.
			// Assigne a wireless mode to initialize.
			if((SupportedWirelessMode & WIRELESS_MODE_A))
			{
				InitWirelessMode = WIRELESS_MODE_A;
			}
			else if((SupportedWirelessMode & WIRELESS_MODE_G))
			{
				InitWirelessMode = WIRELESS_MODE_G;
			}
			else if((SupportedWirelessMode & WIRELESS_MODE_B))
			{
				InitWirelessMode = WIRELESS_MODE_B;
			}
			else
			{
				printk(KERN_WARNING 
						"InitializeAdapter8187(): No valid wireless mode supported, SupportedWirelessMode(%x)!!!\n", 
						SupportedWirelessMode);
				InitWirelessMode = WIRELESS_MODE_B;
			}

			// Initialize RegWirelessMode if it is not a valid one.
			if(bInvalidWirelessMode)
			{
				ieee->mode = (WIRELESS_MODE)InitWirelessMode;
			}
		}
		else
		{ // One of B, G, A.
			InitWirelessMode = ieee->mode; 
		}

		ActSetWirelessMode8187(dev, (u8)(InitWirelessMode));

		// Enable tx/rx		
		write_nic_byte(dev, CMD, (CR_RE|CR_TE));// Using HW_VAR_COMMAND instead of writing CMDR directly. Rewrited by Annie, 2006-04-07.

		//rtl8180_irq_enable(dev);
		//add this is for 8187B Rx stall problem
		write_nic_byte_E(dev, 0x41, 0xF4);
		write_nic_byte_E(dev, 0x40, 0x00);
		write_nic_byte_E(dev, 0x42, 0x00);
		write_nic_byte_E(dev, 0x42, 0x01);
		write_nic_byte_E(dev, 0x40, 0x0F);
		write_nic_byte_E(dev, 0x42, 0x00);
		write_nic_byte_E(dev, 0x42, 0x01);

		// 8187B demo board MAC and AFE power saving parameters from SD1 William, 2006.07.20.
		// Parameter revised by SD1 William, 2006.08.21: 
		//	373h -> 0x4F // Original: 0x0F 
		//	377h -> 0x59 // Original: 0x4F 
		// Victor 2006/8/21: 筿把计某单SD3 蔼放 and cable link test OKタΑ release,ぃ某びе
		// 2006/9/5, Victor & ED: it is OK to use.
		//if(pHalData->RegBoardType == BT_DEMO_BOARD)
		//{
			// AFE.
			//
			// Revise the content of Reg0x372, 0x374, 0x378 and 0x37a to fix unusual electronic current 
			// while CTS-To-Self occurs, by DZ's request.
			// Modified by Roger, 2007.06.22.
			//
			write_nic_byte(dev, 0x0DB, (read_nic_byte(dev, 0x0DB)|(BIT2)));
			write_nic_word(dev, 0x372, 0x59FA); // 0x4FFA-> 0x59FA.
			write_nic_word(dev, 0x374, 0x59D2); // 0x4FD2-> 0x59D2.
			write_nic_word(dev, 0x376, 0x59D2);
			write_nic_word(dev, 0x378, 0x19FA); // 0x0FFA-> 0x19FA.
			write_nic_word(dev, 0x37A, 0x19FA); // 0x0FFA-> 0x19FA.
			write_nic_word(dev, 0x37C, 0x00D0);

			write_nic_byte(dev, 0x061, 0x00);

			// MAC.
			write_nic_byte(dev, 0x180, 0x0F);
			write_nic_byte(dev, 0x183, 0x03);
			// 061218, lanhsin: for victorh request
			//write_nic_byte(dev, 0x0DA, 0x10);
		//}
	
		//
		// MAC: Change the content of reg0x95 from "0x12" to "0x32" to enhance throughput. 
		// Added by Roger, 2006.12.18.
		//
		write_nic_byte(dev, 0x095, 0x32);
			
		//
		// 061213, rcnjko: 
		// Set MAC.0x24D to 0x00 to prevent cts-to-self Tx/Rx stall symptom.
		// If we set MAC 0x24D to 0x08, OFDM and CCK will turn off 
		// if not in use, and there is a bug about this action when 
		// we try to send CCK CTS and OFDM data together.
		//
		//PlatformEFIOWrite1Byte(Adapter, 0x24D, 0x00);
		// 061218, lanhsin: for victorh request
		//write_nic_byte(dev, 0x24D, 0x08);

		//
		// 061215, rcnjko:
		// Follow SD3 RTL8185B_87B_MacPhy_Register.doc v0.4.
		//
		//write_nic_dword(dev, HSSI_PARA, 0x0600321B);
		//
		// 061226, rcnjko:
		// Babble found in HCT 2c_simultaneous test, server with 87B might 
		// receive a packet size about 2xxx bytes.
		// So, we restrict RMS to 2048 (0x800), while as IC default value is 0xC00.
		//
		//write_nic_word(dev, RMS, 0x0800);

		//*****20070321 Resolve HW page bug on system logo test
		//faketemp=read_nic_byte(dev, 0x50);
		//write IDR0~IDR5
		//int i;
		//for(i=0 ; i<6 ; i++)
		//	write_nic_byte(dev, IDR0+i, dev->dev_addr[i]);
	}
}

/* this configures registers for beacon tx and enables it via
 * rtl8180_beacon_tx_enable(). rtl8180_beacon_tx_disable() might
 * be used to stop beacon transmission
 */
#if 0
void rtl8180_start_tx_beacon(struct net_device *dev)
{
	int i;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u16 word;	
	DMESG("Enabling beacon TX");
	//write_nic_byte(dev, 0x42,0xe6);// TCR
	//rtl8180_init_beacon(dev);
	//set_nic_txring(dev);
//	rtl8180_prepare_beacon(dev);
	rtl8180_irq_disable(dev);
//	rtl8180_beacon_tx_enable(dev);
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	//write_nic_byte(dev,0x9d,0x20); //DMA Poll
	//write_nic_word(dev,0x7a,0);
	//write_nic_word(dev,0x7a,0x8000);

	
	word  = read_nic_word(dev, BcnItv);
	word &= ~BcnItv_BcnItv; // clear Bcn_Itv
	write_nic_word(dev, BcnItv, word);

	write_nic_word(dev, AtimWnd, 
		       read_nic_word(dev, AtimWnd) &~ AtimWnd_AtimWnd);
	
	word  = read_nic_word(dev, BintrItv);
	word &= ~BintrItv_BintrItv;
	
	//word |= priv->ieee80211->beacon_interval * 
	//	((priv->txbeaconcount > 1)?(priv->txbeaconcount-1):1);
	// FIXME:FIXME check if correct ^^ worked with 0x3e8;
	
	write_nic_word(dev, BintrItv, word);
	
	//write_nic_word(dev,0x2e,0xe002);
	//write_nic_dword(dev,0x30,0xb8c7832e);
	for(i=0; i<ETH_ALEN; i++)
		write_nic_byte(dev, BSSID+i, priv->ieee80211->beacon_cell_ssid[i]);
	
//	rtl8180_update_msr(dev);

	
	//write_nic_byte(dev,CONFIG4,3); /* !!!!!!!!!! */
	
	rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
	
	rtl8180_irq_enable(dev);
	
	/* VV !!!!!!!!!! VV*/
	/*
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,0x9d,0x00); 	
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
*/
}
#endif
/***************************************************************************
    -------------------------------NET STUFF---------------------------
***************************************************************************/
static struct net_device_stats *rtl8180_stats(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	return &priv->ieee80211->stats;
}


int _rtl8180_up(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	//int i;

	priv->up=1;
	
	//DMESG("Bringing up iface");
	rtl8180_adapter_start(dev);
	rtl8180_rx_enable(dev);
	rtl8180_tx_enable(dev);

	ieee80211_softmac_start_protocol(priv->ieee80211);
	ieee80211_reset_queue(priv->ieee80211);

	if(!netif_queue_stopped(dev))
		netif_start_queue(dev);
	else
		netif_wake_queue(dev);

#ifdef JOHN_DIG
	r8187_start_dig_thread(dev);
#endif
#ifdef THOMAS_RATEADAPTIVE
	r8187_start_RateAdaptive(dev);
#endif

	return 0;
}


int rtl8180_open(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);
	ret = rtl8180_up(dev);
	up(&priv->wx_sem);
	return ret;
	
}


int rtl8180_up(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	if (priv->up == 1) return -1;
	
	return _rtl8180_up(dev);
}


int rtl8180_close(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);

	ret = rtl8180_down(dev);
	
	up(&priv->wx_sem);
	
	return ret;

}

int rtl8180_down(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	if (priv->up == 0) return -1;
	
	priv->up=0;

/* FIXME */
	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);
	
#ifdef JOHN_DIG
	r8187_stop_dig_thread(dev);
#endif
#ifdef THOMAS_RATEADAPTIVE
	r8187_stop_RateAdaptive(dev);
#endif
	rtl8180_rtx_disable(dev);
	rtl8180_irq_disable(dev);

	ieee80211_softmac_stop_protocol(priv->ieee80211);

		
	return 0;
}


void rtl8180_commit(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
        int i;

        if (priv->up == 0) return ;

        //test pending bug, john 20070815 
        //initialize tx_pending
        for(i=0;i<0x10;i++)  atomic_set(&(priv->tx_pending[i]), 0);

	ieee80211_softmac_stop_protocol(priv->ieee80211);
	
	rtl8180_irq_disable(dev);
	rtl8180_rtx_disable(dev);
	_rtl8180_up(dev);
}

void rtl8180_restart(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	down(&priv->wx_sem);
	
	rtl8180_commit(dev);
	
	up(&priv->wx_sem);
}

static void r8180_set_multicast(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	short promisc;

	//down(&priv->wx_sem);
	
	/* FIXME FIXME */
	
	promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	
	if (promisc != priv->promisc)
	//	rtl8180_commit(dev);
	
	priv->promisc = promisc;
	
	//schedule_work(&priv->reset_wq);
	//up(&priv->wx_sem);
}


int r8180_set_mac_adr(struct net_device *dev, void *mac)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
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

#define IPW_IOCTL_WPA_SUPPLICANT		SIOCIWFIRSTPRIV+30

#define IPW_CMD_SET_WPA_PARAM			1
#define	IPW_CMD_SET_WPA_IE			2
#define IPW_CMD_SET_ENCRYPTION			3
#define IPW_CMD_MLME				4

#define IPW_PARAM_WPA_ENABLED			1
#define IPW_PARAM_TKIP_COUNTERMEASURES		2
#define IPW_PARAM_DROP_UNENCRYPTED		3
#define IPW_PARAM_PRIVACY_INVOKED		4
#define IPW_PARAM_AUTH_ALGS			5
#define IPW_PARAM_IEEE_802_1X			6

#define IPW_MLME_STA_DEAUTH			1
#define IPW_MLME_STA_DISASSOC			2

#define IPW_CRYPT_ERR_UNKNOWN_ALG		2
#define IPW_CRYPT_ERR_UNKNOWN_ADDR		3
#define IPW_CRYPT_ERR_CRYPT_INIT_FAILED		4
#define IPW_CRYPT_ERR_KEY_SET_FAILED		5
#define IPW_CRYPT_ERR_TX_KEY_SET_FAILED		6
#define IPW_CRYPT_ERR_CARD_CONF_FAILED		7

#define	IPW_CRYPT_ALG_NAME_LEN			16

struct ipw_param {
        u32 cmd;
        u8 sta_addr[ETH_ALEN];
        union {
                struct {
                        u8 name;
                        u32 value;
                } wpa_param;
                struct {
                        u32 len;
                        u8 reserved[32]; 
                        u8 data[0];
                } wpa_ie;
                struct{
                        u32 command;
                        u32 reason_code;
                } mlme;
                struct {
                        u8 alg[16];
                        u8 set_tx;
                        u32 err;
                        u8 idx;
                        u8 seq[8]; 
                        u16 key_len;
                        u8 key[0];
                } crypt;

        } u;
};

/* based on ipw2200 driver */
int rtl8180_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct iwreq *wrq = (struct iwreq *)rq;
	struct ieee80211_device *ieee = priv->ieee80211;
        struct ipw_param *ipw = (struct ipw_param *)wrq->u.data.pointer;
	int ret=-1;
	u32 key[4];
	u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	down(&priv->wx_sem);

	switch (cmd) {
	    case RTL_IOCTL_WPA_SUPPLICANT:
#ifdef JOHN_TKIP
		//the following code is specified for ipw driver in wpa_supplicant
   		//

		//set pairwise key
		if(  ipw->cmd == IPW_CMD_SET_ENCRYPTION ){
			if( ipw->u.crypt.set_tx && ((strcmp(ipw->u.crypt.alg, "TKIP")==0)  || (strcmp(ipw->u.crypt.alg, "CCMP")==0) ) )
			{
				//enable HW security of TKIP and CCMP
	                	write_nic_byte(dev, WPA_CONFIG, SCR_TxSecEnable | SCR_RxSecEnable );
				
				//copy key from wpa_supplicant ioctl info
				key[0] = ((u32*)wrq->u.data.pointer)[12];
				key[1] = ((u32*)wrq->u.data.pointer)[13];
				key[2] = ((u32*)wrq->u.data.pointer)[14];
				key[3] = ((u32*)wrq->u.data.pointer)[15];
				switch (ieee->pairwise_key_type){
					case KEY_TYPE_TKIP:
						setKey(	dev, 
							0,			//EntryNo
							ipw->u.crypt.idx,			//KeyIndex 
					     		KEY_TYPE_TKIP,		//KeyType
	  			    			(u8*)ieee->ap_mac_addr,	//MacAddr
							0,			//DefaultKey
						      	key);			//KeyContent
						break;

					case KEY_TYPE_CCMP:
						setKey(	dev, 
							0,			//EntryNo
							ipw->u.crypt.idx,			//KeyIndex 
					     		KEY_TYPE_CCMP,		//KeyType
	  			    			(u8*)ieee->ap_mac_addr,	//MacAddr
							0,			//DefaultKey
						      	key);			//KeyContent
						break;
					default: 
						printk("error on key_type: %d\n", ieee->pairwise_key_type); 
						break;
				}
			}


			//set group key
			if( ipw->u.crypt.set_tx==0 && ipw->u.crypt.key_len ){

				key[0] = ((u32*)wrq->u.data.pointer)[12];
				key[1] = ((u32*)wrq->u.data.pointer)[13];
				key[2] = ((u32*)wrq->u.data.pointer)[14];
				key[3] = ((u32*)wrq->u.data.pointer)[15];

				if( strcmp(ipw->u.crypt.alg, "TKIP") == 0 ){

					setKey(	dev, 
						4,		//EntryNo
						ipw->u.crypt.idx,//KeyIndex 
				     		KEY_TYPE_TKIP,	//KeyType
				            	broadcast_addr,	//MacAddr
						0,		//DefaultKey
					      	key);		//KeyContent
				}
				else if( strcmp(ipw->u.crypt.alg, "CCMP") == 0 ){

					setKey(	dev, 
						4,		//EntryNo
						ipw->u.crypt.idx,//KeyIndex 
				     		KEY_TYPE_CCMP,	//KeyType
				            	broadcast_addr,	//MacAddr
						0,		//DefaultKey
					      	key);		//KeyContent
				}
				else if( strcmp(ipw->u.crypt.alg, "NONE" ) ){
	                                //do nothing
	                     }
	                     else if( strcmp(ipw->u.crypt.alg, "WEP") == 0  ){//WEP

					if(ipw->u.crypt.key_len == 5)
					{
						setKey( dev,
							4,              //EntryNo
							ipw->u.crypt.idx,//KeyIndex 
							KEY_TYPE_WEP40,  //KeyType
							broadcast_addr, //MacAddr
							0,              //DefaultKey
							key);           //KeyContent
					}
					else if(ipw->u.crypt.key_len == 13)
					{
						setKey( dev,
							2,              //EntryNo
							ipw->u.crypt.idx,//KeyIndex 
							KEY_TYPE_WEP104,  //KeyType
							broadcast_addr, //MacAddr
							0,              //DefaultKey
							key);           //KeyContent
					}
	                     }
				else printk("undefine group key type: %8x\n", ((u32*)wrq->u.data.pointer)[3]);
		    	}

		}
#endif /*JOHN_TKIP*/


#ifdef JOHN_HWSEC_DEBUG
		//john's test 0711
		printk("@@ wrq->u pointer = ");
		for(i=0;i<wrq->u.data.length;i++){
			if(i%10==0) printk("\n");
			printk( "%8x|", ((u32*)wrq->u.data.pointer)[i] );
		}
		printk("\n");
#endif /*JOHN_HWSEC_DEBUG*/
		ret = ieee80211_wpa_supplicant_ioctl(priv->ieee80211, &wrq->u.data);
		break; 

	    default:
		ret = -EOPNOTSUPP;
		break;
	}

	up(&priv->wx_sem);

	return ret;
}


struct tx_desc {

#ifdef _LINUX_BYTEORDER_LITTLE_ENDIAN_H


//dword 0
unsigned int    tpktsize:12;
unsigned int    rsvd0:3;
unsigned int    no_encrypt:1;
unsigned int    splcp:1;
unsigned int    morefrag:1;
unsigned int    ctsen:1;
unsigned int    rtsrate:4;
unsigned int    rtsen:1;
unsigned int    txrate:4;
unsigned int	last:1;
unsigned int	first:1;
unsigned int	dmaok:1;
unsigned int	own:1;

//dword 1
unsigned short  rtsdur;
unsigned short  length:15;
unsigned short  l_ext:1;

//dword 2
unsigned int	bufaddr;


//dword 3
unsigned short	rxlen:12;
unsigned short  rsvd1:3;
unsigned short  miccal:1;
unsigned short	dur;

//dword 4
unsigned int	nextdescaddr;

//dword 5
unsigned char	rtsagc;
unsigned char	retrylimit;
unsigned short	rtdb:1;
unsigned short	noacm:1;
unsigned short	pifs:1;
unsigned short	rsvd2:4;
unsigned short	rtsratefallback:4;
unsigned short	ratefallback:5;

//dword 6
unsigned short	delaybound;
unsigned short	rsvd3:4;
unsigned short	agc:8;
unsigned short	antenna:1;
unsigned short	spc:2;
unsigned short	rsvd4:1;

//dword 7
unsigned char	len_adjust:2;
unsigned char	rsvd5:1;
unsigned char	tpcdesen:1;
unsigned char	tpcpolarity:2;
unsigned char	tpcen:1;
unsigned char	pten:1;

unsigned char	bckey:6;
unsigned char	enbckey:1;
unsigned char	enpmpd:1;
unsigned short	fragqsz;


#else

#error "please modify tx_desc to your own\n"

#endif


} __attribute__((packed));

struct rx_desc_rtl8187b {

#ifdef _LINUX_BYTEORDER_LITTLE_ENDIAN_H

//dword 0
unsigned int	rxlen:12;
unsigned int	icv:1;
unsigned int	crc32:1;
unsigned int	pwrmgt:1;
unsigned int	res:1;
unsigned int	bar:1;
unsigned int	pam:1;
unsigned int	mar:1;
unsigned int	qos:1;
unsigned int	rxrate:4;
unsigned int	trsw:1;
unsigned int	splcp:1;
unsigned int	fovf:1;
unsigned int	dmafail:1;
unsigned int	last:1;
unsigned int	first:1;
unsigned int	eor:1;
unsigned int	own:1;


//dword 1
unsigned int	tsftl;


//dword 2
unsigned int	tsfth;


//dword 3
unsigned char	sq;
unsigned char	rssi:7;
unsigned char	antenna:1;

unsigned char	agc;
unsigned char 	decrypted:1;
unsigned char	wakeup:1;
unsigned char	shift:1;
unsigned char	rsvd0:5;

//dword 4
unsigned int	num_mcsi:4;
unsigned int	snr_long2end:6;
unsigned int	cfo_bias:6;

unsigned int	pwdb_g12:8;
unsigned int	fot:8;




#else

#error "please modify tx_desc to your own\n"

#endif

}__attribute__((packed));



struct rx_desc_rtl8187 {

#ifdef _LINUX_BYTEORDER_LITTLE_ENDIAN_H

//dword 0
unsigned int    rxlen:12;
unsigned int    icv:1;
unsigned int    crc32:1;
unsigned int    pwrmgt:1;
unsigned int    res:1;
unsigned int    bar:1;
unsigned int    pam:1;
unsigned int    mar:1;
unsigned int    qos:1;
unsigned int    rxrate:4;
unsigned int    trsw:1;
unsigned int    splcp:1;
unsigned int    fovf:1;
unsigned int    dmafail:1;
unsigned int    last:1;
unsigned int    first:1;
unsigned int    eor:1;
unsigned int    own:1;

//dword 1
unsigned char   sq;
unsigned char   rssi:7;
unsigned char   antenna:1;

unsigned char   agc;
unsigned char   decrypted:1;
unsigned char   wakeup:1;
unsigned char   shift:1;
unsigned char   rsvd0:5;

//dword 2
unsigned int tsftl;

//dword 3
unsigned int tsfth;



#else

#error "please modify tx_desc to your own\n"

#endif


}__attribute__((packed));



union rx_desc {

struct	rx_desc_rtl8187b desc_87b;
struct	rx_desc_rtl8187	 desc_87;

}__attribute__((packed));

#ifndef JACKSON_NEW_RX

void rtl8180_irq_rx_tasklet(struct r8180_priv *priv)
{
	struct urb *rx_urb = priv->rxurb_task;
	struct net_device *dev = (struct net_device*)rx_urb->context;
	int status,len,flen;
	struct sk_buff *skb;
	union	rx_desc *desc;
	int i;

	//DMESG("rtl8187_rx_isr");

	struct ieee80211_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
		//	.mac_time = jiffies,
		.freq = IEEE80211_24GHZ_BAND,
	};

	//DMESG("RX %d ",rx_urb->status);
	status = rx_urb->status;
	if(status == 0)
	{
		if(NIC_8187B == priv->card_8187) 
		{
			stats.nic_type = NIC_8187B;
			len = rx_urb->actual_length;
			//	len = len - 4 - 15 - 1; /* CRC, DESC, SEPARATOR*/ 
			//len -= 4*4;/* 4 dword and 4 byte CRC */
			//len -= 5*4;/* 4 dword and 4 byte CRC */

			len -= sizeof (struct rx_desc_rtl8187b);

			desc = (union rx_desc *)(rx_urb->transfer_buffer + len);
			
			flen = desc->desc_87b.rxlen ;
			
			if( flen <= rx_urb->actual_length){

				stats.mac_time[0] = desc->desc_87b.tsftl;
				stats.mac_time[1] = desc->desc_87b.tsfth;

				stats.signal = desc->desc_87b.rssi;
				stats.noise = desc->desc_87b.sq;
				stats.rate = desc->desc_87b.rxrate;
				skb = dev_alloc_skb(flen-4);
				//skb_reserve(skb,2);
				if(skb){ 
					//shift start address according to shift bits.
					//memcpy(skb_put(skb,flen-4),
					//	rx_urb->transfer_buffer+((desc[3]&0x0400)?2:0),flen -4);
					memcpy(skb_put(skb,flen-4),
							rx_urb->transfer_buffer,flen -4);

#ifdef DUMP_RX
					int i;
					for(i=0;i<flen-4;i++)
						printk("%2x ",((u8*)(rx_urb->transfer_buffer))[i]);
					printk("------RATE %x:w---------------\n",stats.rate);

#endif
					priv->stats.rxok++;
					//	priv->rxskb = skb;
					//	priv->tempstats = &stats;

					if(!ieee80211_rx(priv->ieee80211, 
								skb, &stats))
						dev_kfree_skb_any(skb);
				}
			}else priv->stats.rxurberr++;
		} else {
			stats.nic_type = NIC_8187;
			len = rx_urb->actual_length;
			//	len = len - 4 - 15 - 1; /* CRC, DESC, SEPARATOR*/ 
			//len -= 4*4;/* 4 dword and 4 byte CRC */
			len -= sizeof (struct rx_desc_rtl8187);

			desc = (union rx_desc *)(rx_urb->transfer_buffer + len);

			flen = desc->desc_87.rxlen ;

			if( flen <= rx_urb->actual_length){
				stats.signal = desc->desc_87.rssi;
				stats.noise = desc->desc_87.sq;
				stats.rate = desc->desc_87.rxrate;
				stats.mac_time[0] = desc->desc_87.tsftl;
				stats.mac_time[1] = desc->desc_87.tsfth;
				skb = dev_alloc_skb(flen-4);
				//skb_reserve(skb,2);
				if(skb){ 
					memcpy(skb_put(skb,flen-4),
							rx_urb->transfer_buffer,flen -4);

#ifdef DUMP_RX
					int i;
					for(i=0;i<flen-4;i++)
						printk("%2x ",((u8*)(rx_urb->transfer_buffer))[i]);
					printk("------RATE %x:w---------------\n",stats.rate);

#endif
					priv->stats.rxok++;
					//	priv->rxskb = skb;
					//	priv->tempstats = &stats;

					if(!ieee80211_rx(priv->ieee80211, 
								skb, &stats))
						dev_kfree_skb_any(skb);
				}
			}else priv->stats.rxurberr++;
		}	
	}else{
		priv->stats.rxstaterr++;
		priv->ieee80211->stats.rx_errors++;

	}

	if(status != -ENOENT)
		rtl8187_rx_urbsubmit(dev,rx_urb);
	else 
		DMESG("RX process aborted due to explicit shutdown");
}



#else

#ifdef THOMAS_SKB

void rtl8180_irq_rx_tasklet(struct r8180_priv *priv)
{
	int status,len,flen;
	s8 RxPower;
	s8 RX_PWDB;
        u32 SignalStrength = 0;
        _BOOL bCckRate = _FALSE;
	int *prx_inx;
	struct net_device *dev;
        struct sk_buff *skb;	
        struct sk_buff *skb2;//skb for check out of memory	
        union   rx_desc *desc;
	struct urb *rx_urb;
	struct ieee80211_rx_stats stats;
#ifndef THOMAS_PCLINUX
        u32 Tmpaddr = 0;
        int alignment = 0;
#endif
	
	prx_inx = &priv->rx_inx;
	rx_urb = priv->rx_urb[*prx_inx]; //changed by jackson

	dev = (struct net_device*)rx_urb->context;

	stats.signal = 0;
	stats.noise = -98;
	stats.rate = 0;
	stats.freq = IEEE80211_24GHZ_BAND;

        //DMESG("RX %d ",rx_urb->status);

        skb = priv->pp_rxskb[*prx_inx];

        status = rx_urb->status;

#ifndef THOMAS_PCLINUX
	skb2 = dev_alloc_skb(RX_URB_SIZE_ALIGN);
#else
	skb2 = dev_alloc_skb(RX_URB_SIZE);
#endif

	if (skb2 == NULL){
        	printk(KERN_ALERT "%d No Skb For RX!\n", *prx_inx);
		//rx_urb->transfer_buffer = skb->data;
        	//priv->pp_rxskb[*prx_inx] = skb;
	} else {
        
	if(status == 0)
        {
                if(NIC_8187B == priv->card_8187)
                {
                        stats.nic_type = NIC_8187B;
                        
			len = rx_urb->actual_length;

                        len -= sizeof (struct rx_desc_rtl8187b);

                        desc = (union rx_desc *)(rx_urb->transfer_buffer + len);

                        flen = desc->desc_87b.rxlen ;

                        if( flen <= rx_urb->actual_length){

                                stats.mac_time[0] = desc->desc_87b.tsftl;
                                stats.mac_time[1] = desc->desc_87b.tsfth;

                                stats.signal = desc->desc_87b.rssi;
                                stats.noise = desc->desc_87b.sq;
                                stats.rate = desc->desc_87b.rxrate;
					RxPower = (s8)(desc->desc_87b.pwdb_g12);
                                if( ((stats.rate <= 22) && (stats.rate != 12) && (stats.rate != 18)) || (stats.rate == 44) )
                                        bCckRate= TRUE;

                                if(!bCckRate)
                                { // OFDM rate.
                                        if( RxPower < -106)
                                                SignalStrength = 0;
                                        else
                                                SignalStrength = RxPower + 106;
						RX_PWDB = RxPower/2 -42;
                                }
                                else
                                { // CCK rate.
                                        if(desc->desc_87b.agc> 174)
                                                SignalStrength = 0;
                                        else
                                                SignalStrength = 174 - desc->desc_87b.agc;
						RX_PWDB = ((int)(desc->desc_87b.agc)/2)*(-1) - 8;
                                }

                                if(SignalStrength > 140)
                                        SignalStrength = 140;
                                SignalStrength = (SignalStrength * 100) / 140;
                                
                                stats.signalstrength = (u8)SignalStrength;
#ifdef THOMAS_RATEADAPTIVE
				priv->stats.RecvSignalPower = (int)(priv->stats.RecvSignalPower*5 + RX_PWDB - 1)/6;
#endif

				skb_put(skb,flen-4);

                                priv->stats.rxok++;

                                if(!ieee80211_rx(priv->ieee80211,skb, &stats)) {
                                	dev_kfree_skb_any(skb);
                                }
                        }else {
				priv->stats.rxurberr++;
				if(priv->MPState==0)
					printk("URB Error  flen:%d actual_length:%d\n",  flen , rx_urb->actual_length);
                        	dev_kfree_skb_any(skb);
			}
                } else {
                        
			stats.nic_type = NIC_8187;
                        
			len = rx_urb->actual_length;
                        
			len -= sizeof (struct rx_desc_rtl8187);

                        desc = (union rx_desc *)(rx_urb->transfer_buffer + len);

                        flen = desc->desc_87.rxlen ;

                        if(flen <= rx_urb->actual_length){
                                stats.signal = desc->desc_87.rssi;
                                stats.noise = desc->desc_87.sq;
                                stats.rate = desc->desc_87.rxrate;
                                stats.mac_time[0] = desc->desc_87.tsftl;
                                stats.mac_time[1] = desc->desc_87.tsfth;


				skb_put(skb,flen-4);

                                priv->stats.rxok++;

                                if(!ieee80211_rx(priv->ieee80211,skb, &stats))
                                        dev_kfree_skb_any(skb);


                        }else {
			 	priv->stats.rxurberr++;
                                printk("URB Error  flen:%d actual_length:%d\n",  flen , rx_urb->actual_length);
                                dev_kfree_skb_any(skb);
			} 
                }
        }else{

                priv->stats.rxstaterr++;
                priv->ieee80211->stats.rx_errors++;
                dev_kfree_skb_any(skb);

        }	
#ifndef THOMAS_PCLINUX
        // 512 bytes alignment for DVR hw access bug, by thomas 09-03-2007
        Tmpaddr = (u32)skb2->data;
        alignment = Tmpaddr & 0x1ff;
        skb_reserve(skb2,(512 - alignment));
#endif
        rx_urb->transfer_buffer = skb2->data;

        priv->pp_rxskb[*prx_inx] = skb2;
        }
/*	
	if(status != -ENOENT ){
                rtl8187_rx_urbsubmit(dev,rx_urb);
	}
        else {
		priv->pp_rxskb[*prx_inx] = NULL;
		dev_kfree_skb_any(skb2);
                DMESG("RX process %2d aborted due to explicit shutdown (%x)(%d)", *prx_inx, status, status);
        }
*/
	if( status == 0)
                rtl8187_rx_urbsubmit(dev,rx_urb);
	else {
		switch(status) {
			case -EINVAL:
			case -EPIPE:			
			case -ENODEV:
			case -ESHUTDOWN:
				priv->bSurpriseRemoved = _TRUE;
			case -ENOENT:
				priv->pp_rxskb[*prx_inx] = NULL;
				dev_kfree_skb_any(skb2);
				DMESG("RX process %2d aborted due to explicit shutdown (%x)(%d)", *prx_inx, status, status);
				break;
			case -EINPROGRESS:
				panic("URB IS IN PROGRESS!/n");
				break;
			default:
				rtl8187_rx_urbsubmit(dev,rx_urb);
				break;
		}
	}

       if (*prx_inx == (MAX_RX_URB -1))
                *prx_inx = 0;
       else
                *prx_inx = *prx_inx + 1;
}

#else

void rtl8180_irq_rx_tasklet(struct r8180_priv *priv)
{
        int status,len,flen;
        
	int *prx_inx;

        struct sk_buff *skb;

        union   rx_desc *desc;
	
	//struct urb *rx_urb = priv->rxurb_task;
	struct urb *rx_urb;
	prx_inx = &priv->rx_inx;
	rx_urb = priv->rx_urb[*prx_inx]; //changed by jackson

	struct net_device *dev = (struct net_device*)rx_urb->context;

        struct ieee80211_rx_stats stats = {
                .signal = 0,
                .noise = -98,
                .rate = 0,
                //      .mac_time = jiffies,
                .freq = IEEE80211_24GHZ_BAND,
        };

        //DMESG("RX %d ",rx_urb->status);


        skb = priv->pp_rxskb[*prx_inx];

        status = rx_urb->status;
        
	if(status == 0)
        {
                if(NIC_8187B == priv->card_8187)
                {
                        stats.nic_type = NIC_8187B;
                        
			len = rx_urb->actual_length;

                        len -= sizeof (struct rx_desc_rtl8187b);

                        desc = (union rx_desc *)(rx_urb->transfer_buffer + len);

                        flen = desc->desc_87b.rxlen ;

                        if( flen <= rx_urb->actual_length){

                                stats.mac_time[0] = desc->desc_87b.tsftl;
                                stats.mac_time[1] = desc->desc_87b.tsfth;

                                stats.signal = desc->desc_87b.rssi;
                                stats.noise = desc->desc_87b.sq;
                                stats.rate = desc->desc_87b.rxrate;
                               
				skb_put(skb,flen-4);
 
                                priv->stats.rxok++;

                                if(!ieee80211_rx(priv->ieee80211,skb, &stats)) {
                                	dev_kfree_skb_any(skb);
                                }
                        }else {
				priv->stats.rxurberr++;
				printk("URB Error  flen:%d actual_length:%d\n",  flen , rx_urb->actual_length);
                        	dev_kfree_skb_any(skb);
			}
                } else {
                        
			stats.nic_type = NIC_8187;
                        
			len = rx_urb->actual_length;
                        
			len -= sizeof (struct rx_desc_rtl8187);

                        desc = (union rx_desc *)(rx_urb->transfer_buffer + len);

                        flen = desc->desc_87.rxlen ;

                        if(flen <= rx_urb->actual_length){
                                stats.signal = desc->desc_87.rssi;
                                stats.noise = desc->desc_87.sq;
                                stats.rate = desc->desc_87.rxrate;
                                stats.mac_time[0] = desc->desc_87.tsftl;
                                stats.mac_time[1] = desc->desc_87.tsfth;


				skb_put(skb,flen-4);

                                priv->stats.rxok++;

                                if(!ieee80211_rx(priv->ieee80211,skb, &stats))
                                        dev_kfree_skb_any(skb);


                        }else {
			 	priv->stats.rxurberr++;
                                printk("URB Error  flen:%d actual_length:%d\n",  flen , rx_urb->actual_length);
                                dev_kfree_skb_any(skb);
			} 
                }
        }else{

                priv->stats.rxstaterr++;
                priv->ieee80211->stats.rx_errors++;
                dev_kfree_skb_any(skb);

        }
	

	skb = dev_alloc_skb(RX_URB_SIZE);
        if (skb == NULL)
                panic("No Skb For RX!/n");

        rx_urb->transfer_buffer = skb->data;

        priv->pp_rxskb[*prx_inx] = skb;

        if(status == 0)
                rtl8187_rx_urbsubmit(dev,rx_urb);
        else {
                priv->pp_rxskb[*prx_inx] = NULL;
                dev_kfree_skb_any(skb);
                printk("RX process aborted due to explicit shutdown (%x) ", status);
        }

       if (*prx_inx == (MAX_RX_URB -1))
                *prx_inx = 0;
       else
                *prx_inx = *prx_inx + 1;

}
#endif

#endif

#ifdef THOMAS_TASKLET
void rtl8180_irq_rx_tasklet_new(struct r8180_priv *priv){
	unsigned long flags;
	while( atomic_read( &priv->irt_counter ) ){
	//while( priv->irt_counter_head != priv->irt_counter_tail){
		spin_lock_irqsave(&priv->irq_lock,flags);//added by thomas
		rtl8180_irq_rx_tasklet(priv);
		spin_unlock_irqrestore(&priv->irq_lock,flags);//added by thomas
		atomic_dec( &priv->irt_counter );
		//priv->irt_counter_tail =  (priv->irt_counter_tail+1)&0xffff;
	}
}
#endif
/****************************************************************************
     ---------------------------- USB_STUFF---------------------------
*****************************************************************************/

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
static int __devinit rtl8187_usb_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
#else
static void * __devinit rtl8187_usb_probe(struct usb_device *udev,
			                unsigned int ifnum,
			          const struct usb_device_id *id)
#endif
{
//	unsigned long ioaddr = 0;
	struct net_device *dev = NULL;
	struct r8180_priv *priv= NULL;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct usb_device *udev = interface_to_usbdev(intf);
#endif

	dev = alloc_ieee80211(sizeof(struct r8180_priv));
	
	SET_MODULE_OWNER(dev);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	usb_set_intfdata(intf, dev);	
	SET_NETDEV_DEV(dev, &intf->dev);
#endif
	priv = ieee80211_priv(dev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	priv->ieee80211 = netdev_priv(dev);
#else
	priv->ieee80211 = (struct net_device *)dev->priv;
#endif
	priv->udev=udev;
	
	dev->open = rtl8180_open;
	dev->stop = rtl8180_close;
	//dev->hard_start_xmit = rtl8180_8023_hard_start_xmit;
	dev->tx_timeout = tx_timeout;
	dev->wireless_handlers = &r8180_wx_handlers_def;
	dev->do_ioctl = rtl8180_ioctl;
	dev->set_multicast_list = r8180_set_multicast;
	dev->set_mac_address = r8180_set_mac_adr;
	dev->get_wireless_stats = r8180_get_wireless_stats;
	dev->type=ARPHRD_ETHER;
	dev->watchdog_timeo = HZ *3; //modified by john, 0805
	
	if (dev_alloc_name(dev, ifname) < 0){
                DMESG("Oops: devname already taken! Trying wlan%%d...\n");
		ifname = "wlan%d";
		dev_alloc_name(dev, ifname);
        }
	
//	dev->open=rtl8180_init;
	
	if(rtl8180_init(dev)!=0){ 
		DMESG("Initialization failed");
		goto fail;
	}
	
	netif_carrier_off(dev);
	netif_stop_queue(dev);
	
	register_netdev(dev);
	
	rtl8180_proc_init_one(dev);
	
	
	DMESG("Driver probe completed\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	return dev;
#else
	return 0;	
#endif

	
fail:
	free_ieee80211(dev);
		
	DMESG("wlan driver load failed\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	
	return NULL;
#else
	return -ENODEV;
#endif
	
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) 
static void __devexit rtl8187_usb_disconnect(struct usb_interface *intf)
#else 
static void __devexit rtl8187_usb_disconnect(struct usb_device *udev, void *ptr)
#endif
{
	struct r8180_priv *priv;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct net_device *dev = usb_get_intfdata(intf);
#else
	struct net_device *dev = (struct net_device *)ptr;
#endif
 	if(dev){
		
		unregister_netdev(dev);
		
		priv=ieee80211_priv(dev);
		
		rtl8180_proc_remove_one(dev);
		
		rtl8180_down(dev);
		priv->rf_close(dev);
		//rtl8180_rtx_disable(dev);
		rtl8187_usb_deleteendpoints(dev);
		rtl8180_irq_disable(dev);
		rtl8180_reset(dev);
		mdelay(10);

	}
//	pci_disable_device(pdev);
	free_ieee80211(dev);
	DMESG("wlan driver removed\n");
}


static int __init rtl8187_usb_module_init(void)
{
	printk(KERN_INFO "\nLinux kernel driver for RTL8187/RTL8187B \
based WLAN cards\n");
	printk(KERN_INFO "Copyright (c) 2004-2005, Andrea Merello\n");
	DMESG("Initializing module");
	DMESG("Wireless extensions version %d", WIRELESS_EXT);
	rtl8180_proc_module_init();
	return usb_register(&rtl8187_usb_driver);
}


static void __exit rtl8187_usb_module_exit(void)
{
	usb_deregister(&rtl8187_usb_driver);

	rtl8180_proc_module_remove();
	DMESG("Exiting");
}


void rtl8180_try_wake_queue(struct net_device *dev, int pri)
{
	unsigned long flags;
	short enough_desc;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	spin_lock_irqsave(&priv->tx_lock,flags);
	enough_desc = check_nic_enought_desc(dev,pri);
        spin_unlock_irqrestore(&priv->tx_lock,flags);	
	
	if(enough_desc)
		ieee80211_wake_queue(priv->ieee80211);
}

#ifdef JOHN_HWSEC
void EnableHWSecurityConfig8187(struct net_device *dev)
{
        u8 SECR_value = 0x0;
	SECR_value = SCR_TxSecEnable | SCR_RxSecEnable;
        {
                //write_nic_byte(dev, 0x24e, 0x01);
                write_nic_byte(dev, WPA_CONFIG,  SECR_value |  SCR_UseDK ); 
                //write_nic_byte(dev, 0x24e, 0x01);
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
	int i;
	usConfig |= BIT15 | (KeyType<<2) | (DefaultKey<<4) | KeyIndex;

	for(i=0 ; i<6 ; i++){
		TargetCommand  = i+6*EntryNo;
		TargetCommand |= BIT31|BIT16;

		if(i==0)//MAC|Config
		{
			TargetContent = (u32)(*(MacAddr+0)) << 16|
					(u32)(*(MacAddr+1)) << 24|
					(u32)usConfig;

			write_nic_dword(dev, WCAMI, TargetContent); 
			write_nic_dword(dev, RWCAM, TargetCommand);
		}
		else if(i==1)//MAC
		{
                        TargetContent = (u32)(*(MacAddr+2)) 	 |
                                        (u32)(*(MacAddr+3)) <<  8|
                                        (u32)(*(MacAddr+4)) << 16|
                                        (u32)(*(MacAddr+5)) << 24;
			write_nic_dword(dev, WCAMI, TargetContent); 
			write_nic_dword(dev, RWCAM, TargetCommand);
		}
		else //Key Material
		{
			write_nic_dword(dev, WCAMI, (u32)(*(KeyContent+i-2)) ); 
			write_nic_dword(dev, RWCAM, TargetCommand);
		}
	}

}
#endif

#ifdef JOHN_DIG
void UpdateCCKThreshold(struct net_device *dev )
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);

        // Update CCK Power Detection(0x41) value.
        switch(priv->StageCCKTh)
        {
        case 0:
                write_nic_dword(dev, PHY_ADR, 0x010088C1);mdelay(1);
                break;

        case 1:
                write_nic_dword(dev, PHY_ADR, 0x010098C1);mdelay(1);
                break;

        case 2:
                write_nic_dword(dev, PHY_ADR, 0x0100C8C1);mdelay(1);
                break;

        case 3:
                write_nic_dword(dev, PHY_ADR, 0x0100D8C1);mdelay(1);
                break;

        default:
                break;
	}
}

void UpdateInitialGain(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	//
	// Note: 
	//      Whenever we update this gain table, we should be careful about those who call it.
	//      Functions which call UpdateInitialGain as follows are important:
	//      (1)StaRateAdaptive87B
	//      (2)DIG_Zebra
	//      (3)ActSetWirelessMode8187 (when the wireless mode is "B" mode, we set the 
	//              OFDM[0x17] = 0x26 to improve the Rx sensitivity).
	//      By Bruce, 2007-06-01.
	//

	//
	// SD3 C.M. Lin Initial Gain Table, by Bruce, 2007-06-01.
	//

	//printk("@@@@ priv->InitialGain = %x\n", priv->InitialGain);

	switch (priv->InitialGain)
	{
		case 1: //m861dBm
//			write_phy_ofdm(dev, 0x17, 0x26);
//			write_phy_ofdm(dev, 0x24, 0x86);
//			write_phy_ofdm(dev, 0x5, 0xfa);
#if 1
			write_nic_dword(dev, PHY_ADR, 0x2697);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0x86a4);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xfa85);        mdelay(1);
#endif
			break;

		case 2: //m862dBm
//			write_phy_ofdm(dev, 0x17, 0x36);
//			write_phy_ofdm(dev, 0x24, 0x86);
//			write_phy_ofdm(dev, 0x5, 0xfa);
#if 1
			write_nic_dword(dev, PHY_ADR, 0x3697);        mdelay(1);// Revise 0x26 to 0x36, by Roger, 2007.05.03.
			write_nic_dword(dev, PHY_ADR, 0x86a4);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xfa85);        mdelay(1);
#endif
			break;
		
		case 3: //m863dBm
//			write_phy_ofdm(dev, 0x17, 0x36);
//			write_phy_ofdm(dev, 0x24, 0x86);
//			write_phy_ofdm(dev, 0x5, 0xfb);
#if 1
			write_nic_dword(dev, PHY_ADR, 0x3697);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0x86a4);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xfb85);        mdelay(1);
#endif
			break;

		case 4: //m864dBm
//			write_phy_ofdm(dev, 0x17, 0x46);
//			write_phy_ofdm(dev, 0x24, 0x86);
//			write_phy_ofdm(dev, 0x5, 0xfb);
#if 1
			write_nic_dword(dev, PHY_ADR, 0x4697);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0x86a4);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xfb85);        mdelay(1);
#endif
			break;
	
		case 5: //m82dBm
//			write_phy_ofdm(dev, 0x17, 0x46);
//			write_phy_ofdm(dev, 0x24, 0x96);
//			write_phy_ofdm(dev, 0x5, 0xfb);
#if 1
			write_nic_dword(dev, PHY_ADR, 0x4697);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0x96a4);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xfb85);        mdelay(1);
#endif
			break;

		case 6: //m78dBm
//			write_phy_ofdm(dev, 0x17, 0x56);
//			write_phy_ofdm(dev, 0x24, 0x96);
//			write_phy_ofdm(dev, 0x5, 0xfc);
#if 1
			write_nic_dword(dev, PHY_ADR, 0x5697);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0x96a4);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xfc85);        mdelay(1);
#endif
			break;

		case 7: //m74dBm
//			write_phy_ofdm(dev, 0x17, 0x56);
//			write_phy_ofdm(dev, 0x24, 0xa6);
//			write_phy_ofdm(dev, 0x5, 0xfc);
#if 1
			write_nic_dword(dev, PHY_ADR, 0x5697);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xa6a4);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xfc85);        mdelay(1);
#endif
			break;

		case 8: 
//			write_phy_ofdm(dev, 0x17, 0x66);
//			write_phy_ofdm(dev, 0x24, 0xb6);
//			write_phy_ofdm(dev, 0x5, 0xfc);
#if 1
			write_nic_dword(dev, PHY_ADR, 0x6697);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xb6a4);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xfc85);        mdelay(1);
#endif
			break;

		default: //MP
//			write_phy_ofdm(dev, 0x17, 0x26);
//			write_phy_ofdm(dev, 0x24, 0x86);
//			write_phy_ofdm(dev, 0x5, 0xfa);
#if 1
			write_nic_dword(dev, PHY_ADR, 0x2697);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0x86a4);        mdelay(1);
			write_nic_dword(dev, PHY_ADR, 0xfa85);        mdelay(1);
#endif
			break;

	}

}
#endif

#ifdef THOMAS_RATEADAPTIVE
void r8187_start_RateAdaptive(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->bRateAdaptive = _TRUE;
	schedule_delayed_work(&priv->RateAdaptiveWorkItem, DEFAULT_RATE_ADAPTIVE_TIMER_PERIOD);
}

void r8187_stop_RateAdaptive(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	if(priv->bRateAdaptive){
		cancel_rearming_delayed_work(&priv->RateAdaptiveWorkItem);
		priv->bRateAdaptive = _FALSE;
	}
	else flush_scheduled_work();
}

//
//	Description:
//		Callback of RateAdaptiveWorkItem.
//
void RateAdaptiveWorkItemCallback(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;

	if(priv->bSurpriseRemoved == _TRUE)
		return;

	if(ieee->ForcedDataRate == 0 && ieee->state == IEEE80211_LINKED) // Rate adaptive
	{
		//printk("RTL8187B Rate Adaptive Call back \n");
		StaRateAdaptive87B(dev);
	}
	schedule_delayed_work(&priv->RateAdaptiveWorkItem, DEFAULT_RATE_ADAPTIVE_TIMER_PERIOD);
}

//
//	Helper function to determine if specified data rate is 
//	CCK rate.
//	2005.01.25, by rcnjko.
//
_BOOL MgntIsCckRate(u8 rate)
{
	_BOOL bReturn = FALSE;

	if((rate <= 22) && (rate != 12) && (rate != 18))
	{
		bReturn = TRUE;
	}

	return bReturn;
}

//
// Return BOOLEAN: TxRate is included in SupportedRates or not.
// Added by Annie, 2006-05-16, in SGS, for Cisco B mode AP. (with ERPInfoIE in its beacon.)
//
_BOOL IncludedInSupportedRates(struct net_device *dev, u8 TxRate)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	u8	*pSupRateSet = ieee->SupportedRates;
	u8			Length = ieee->SupportedRates_length;
	u8			RateMask = 0x7F;
	u8			idx;
	_BOOL			Found = FALSE;
	u8			NaiveTxRate = TxRate&RateMask;

	for( idx=0; idx<Length; idx++ )
	{
		if( (pSupRateSet[idx] & RateMask) == NaiveTxRate )
		{
			Found = TRUE;
			break;
		}
	}

	return Found;
}

//
//	Description:
//		Get the Tx rate one degree up form the input rate in the supported rates.
//		Return the upgrade rate if it is successed, otherwise return the input rate.
//	By Bruce, 2007-06-05.
// 
u16 GetUpgradeTxRate(struct net_device *dev , u8 rate)
{
	u8			UpRate;

	// Upgrade 1 degree.
	switch(rate)
	{	
	case 108: // Up to 54Mbps.
		UpRate = 108;
		break;
	
	case 96: // Up to 54Mbps.
		UpRate = 108;
		break;

	case 72: // Up to 48Mbps.
		UpRate = 96;
		break;

	case 48: // Up to 36Mbps.
		UpRate = 72;
		break;

	case 36: // Up to 24Mbps.
		UpRate = 48;
		break;

	case 22: // Up to 18Mbps.
		UpRate = 36;
		break;

	case 11: // Up to 11Mbps.
		UpRate = 22;
		break;

	case 4: // Up to 5.5Mbps.
		UpRate = 11;
		break;

	case 2: // Up to 2Mbps.
		UpRate = 4;
		break;

	default:
		//RT_TRACE(COMP_RATE, DBG_WARNING, ("GetUpgradeTxRate(): Input Tx Rate(%d) is undefined!\n", rate));
		return rate;		
	}

	// Check if the rate is valid.
	if(IncludedInSupportedRates(dev, UpRate))
	{
		//RT_TRACE(COMP_RATE, DBG_TRACE, ("GetUpgradeTxRate(): GetUpgrade Tx rate(%d) from %d !\n", UpRate, pMgntInfo->CurrentOperaRate));
		return UpRate;
	}
	else
	{
		//RT_TRACE(COMP_RATE, DBG_WARNING, ("GetUpgradeTxRate(): Tx rate (%d) is not in supported rates\n", UpRate));		
		return rate;
	}
	return rate;
}

//
//	Description:
//		Get the Tx rate one degree down form the input rate in the supported rates.
//		Return the degrade rate if it is successed, otherwise return the input rate.
//	By Bruce, 2007-06-05.
// 
u16 GetDegradeTxRate(struct net_device *dev , u8 rate)
{
	u8			DownRate;

	// Upgrade 1 degree.
	switch(rate)
	{			
	case 108: // Down to 48Mbps.
		DownRate = 96;
		break;

	case 96: // Down to 36Mbps.
		DownRate = 72;
		break;

	case 72: // Down to 24Mbps.
		DownRate = 48;
		break;

	case 48: // Down to 18Mbps.
		DownRate = 36;
		break;

	case 36: // Down to 11Mbps.
		DownRate = 22;
		break;

	case 22: // Down to 5.5Mbps.
		DownRate = 11;
		break;

	case 11: // Down to 2Mbps.
		DownRate = 4;
		break;

	case 4: // Down to 1Mbps.
		DownRate = 2;
		break;	

	case 2: // Down to 1Mbps.
		DownRate = 2;
		break;	
		
	default:
		//RT_TRACE(COMP_RATE, DBG_WARNING, ("GetDegradeTxRate(): Input Tx Rate(%d) is undefined!\n", rate));
		return rate;		
	}

	// Check if the rate is valid.
	if(IncludedInSupportedRates(dev, DownRate))
	{
		//RT_TRACE(COMP_RATE, DBG_TRACE, ("GetDegradeTxRate(): GetDegrade Tx rate(%d) from %d!\n", DownRate, pMgntInfo->CurrentOperaRate));
		return DownRate;
	}
	else
	{
		//RT_TRACE(COMP_RATE, DBG_WARNING, ("GetDegradeTxRate(): Tx rate (%d) is not in supported rates\n", DownRate));		
		return rate;
	}
	return rate;
}

void StaRateAdaptive87B(struct net_device *dev )
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	u64			CurrTxokCnt;
	u16			CurrRetryCnt;
	u16			CurrRetryRate;
	u8			CurrentOperaRate;
//	u16			i,idx;
	u64			CurrRxokCnt;
	_BOOL			bTryUp = FALSE;
	_BOOL			bTryDown = FALSE;
	u8			TryUpTh = 1;
	u8			TryDownTh = 2;
	u32			TxThroughput;
	s32			CurrSignalStrength;
	_BOOL			bUpdateInitialGain = FALSE;

	CurrRetryCnt	= priv->CurrRetryCnt;
	CurrTxokCnt	= priv->stats.NumTxOkTotal - priv->LastTxokCnt;
	CurrRxokCnt	= priv->stats.rxok- priv->LastRxokCnt;
	CurrSignalStrength = priv->stats.RecvSignalPower;
	TxThroughput = (u32)(priv->stats.NumTxOkBytesTotal - priv->LastTxOKBytes);
	priv->LastTxOKBytes = priv->stats.NumTxOkBytesTotal;
	CurrentOperaRate = (u8)((ieee->rate/10) * 2);

	//2 Compute retry ratio.
	if (CurrTxokCnt>0)
	{
		CurrRetryRate = (u16)((int)CurrRetryCnt*100/(int)CurrTxokCnt);
	}
	else
	{ // It may be serious retry. To distinguish serious retry or no packets modified by Bruce
		CurrRetryRate = (u16)((int)CurrRetryCnt*100/1);
	}

	//
	// For debug information.
	//
	//printk("\n(1) LastRetryRate: %d \n",(int)priv->LastRetryRate);
	//printk("(2) RetryCnt = %d  \n", (int)CurrRetryCnt);	
	//printk("(3) TxokCnt = %d \n", (int)CurrTxokCnt);
	//printk("(4) CurrRetryRate = %d \n", (int)CurrRetryRate);	
	//printk("(5) SignalStrength = %d \n",(int)priv->ieee80211->current_network.SignalStrength);
	//printk("(6) ReceiveSignalPower = %d \n",(int)CurrSignalStrength);

	priv->LastRetryCnt = priv->CurrRetryCnt;
	priv->LastTxokCnt	 = priv->stats.NumTxOkTotal;
	priv->LastRxokCnt = priv->stats.rxok;
	priv->CurrRetryCnt = 0;

	//2No Tx packets, return to init_rate or not?
	if (CurrRetryRate==0 && CurrTxokCnt == 0)
	{	
		//
		// 2007.04.09, by Roger.
		//After 4.5 seconds in this condition, we try to raise rate.
		//
		priv->TryupingCountNoData++;		
		
		if (priv->TryupingCountNoData>15)
		{
			priv->TryupingCountNoData = 0;
		 	//CurrentOperaRate = GetUpgradeTxRate(dev, CurrentOperaRate);
			// Reset Fail Record
			priv->LastFailTxRate = 0;
			priv->LastFailTxRateSS = -200;
			priv->FailTxRateCount = 0;
		}
		goto SetInitialGain;
	}
        else
	{
		priv->TryupingCountNoData=0; //Reset trying up times.
	}

	//
	// Restructure rate adaptive as the following main stages:
	// (1) Add retry threshold in 54M upgrading condition with signal strength.
	// (2) Add the mechanism to degrade to CCK rate according to signal strength 
	//		and retry rate.
	// (3) Remove all Initial Gain Updates over OFDM rate. To avoid the complicated 
	//		situation, Initial Gain Update is upon on DIG mechanism except CCK rate.
	// (4) Add the mehanism of trying to upgrade tx rate.
	// (5) Record the information of upping tx rate to avoid trying upping tx rate constantly.
	// By Bruce, 2007-06-05.
	//	
	//

	// 11Mbps or 36Mbps
	// Check more times in these rate(key rates).
	//
	if(CurrentOperaRate == 22 || CurrentOperaRate == 72)
	{ 
		TryUpTh += 9;
	}
	//
	// Let these rates down more difficult.
	//
	if(MgntIsCckRate(CurrentOperaRate) || CurrentOperaRate == 36)
	{
			TryDownTh += 1;
	}
	//1 Adjust Rate.
	if (priv->bTryuping == TRUE)
	{	
		//2 For Test Upgrading mechanism
		// Note:
		// 	Sometimes the throughput is upon on the capability bwtween the AP and NIC,
		// 	thus the low data rate does not improve the performance.
		// 	We randomly upgrade the data rate and check if the retry rate is improved.
		
		// Upgrading rate did not improve the retry rate, fallback to the original rate.
		if ( (CurrRetryRate > 25) && TxThroughput < priv->LastTxThroughput)
		{
			//Not necessary raising rate, fall back rate.
			bTryDown = TRUE;
		}
		else
		{
			priv->bTryuping = FALSE;
		}
	}	
	else if (CurrSignalStrength > -51 && (CurrRetryRate < 100))
	{
		//2For High Power
		//
		// Added by Roger, 2007.04.09.
		// Return to highest data rate, if signal strength is good enough.
		// SignalStrength threshold(-50dbm) is for RTL8186.
		// Revise SignalStrength threshold to -51dbm.	
		//
		// Also need to check retry rate for safety, by Bruce, 2007-06-05.
		if(CurrentOperaRate != 108)
		{
			bTryUp = TRUE;
			// Upgrade Tx Rate directly.
			priv->TryupingCount += TryUpTh;			
		}
	}
	else if(CurrTxokCnt< 100 && CurrRetryRate >= 600) 
	{
		//2 For Serious Retry
		//
		// Traffic is not busy but our Tx retry is serious. 
		//
		bTryDown = TRUE;
		// Let Rate Mechanism to degrade tx rate directly.
		priv->TryDownCountLowData += TryDownTh;
		//RT_TRACE(COMP_RATE, DBG_LOUD, ("RA: Tx Retry is serious. Degrade Tx Rate to %d directly...\n", pMgntInfo->CurrentOperaRate));	
	}
	else if ( CurrentOperaRate == 108 )
	{
		//2For 54Mbps
		// if ( (CurrRetryRate>38)&&(pHalData->LastRetryRate>35)) 
		if ( (CurrRetryRate>33)&&(priv->LastRetryRate>32))
		{
			//(30,25) for cable link threshold. (38,35) for air link.
			//Down to rate 48Mbps.
			bTryDown = TRUE;
		}
	}
	else if ( CurrentOperaRate == 96 )
	{
		//2For 48Mbps
		// if ( ((CurrRetryRate>73) && (pHalData->LastRetryRate>72)) && IncludedInSupportedRates(Adapter, 72) )
		if ( ((CurrRetryRate>48) && (priv->LastRetryRate>47)))
		{	
			//(73, 72) for temp used.
			//(25, 23) for cable link, (60,59) for air link.
			//CurrRetryRate plus 25 and 26 respectively for air link.
			//Down to rate 36Mbps.
			bTryDown = TRUE;
		}
		else if ( (CurrRetryRate<8) && (priv->LastRetryRate<8) ) //TO DO: need to consider (RSSI)
		{
			bTryUp = TRUE;
		}
	}
	else if ( CurrentOperaRate == 72 )
	{
		//2For 36Mbps
		//if ( (CurrRetryRate>97) && (pHalData->LastRetryRate>97)) 
		if ( (CurrRetryRate>55) && (priv->LastRetryRate>54)) 
		{	
			//(30,25) for cable link threshold respectively. (103,10) for air link respectively.
			//CurrRetryRate plus 65 and 69 respectively for air link threshold.
			//Down to rate 24Mbps.
			bTryDown = TRUE;
		}
		// else if ( (CurrRetryRate<20) &&  (pHalData->LastRetryRate<20) && IncludedInSupportedRates(Adapter, 96) )//&& (device->LastRetryRate<15) ) //TO DO: need to consider (RSSI)
		else if ( (CurrRetryRate<15) &&  (priv->LastRetryRate<16))//&& (device->LastRetryRate<15) ) //TO DO: need to consider (RSSI)
		{
			bTryUp = TRUE;
		}
	}
	else if ( CurrentOperaRate == 48 )
	{
		//2For 24Mbps
		// if ( ((CurrRetryRate>119) && (pHalData->LastRetryRate>119) && IncludedInSupportedRates(Adapter, 36)))
		if ( ((CurrRetryRate>63) && (priv->LastRetryRate>62)))
		{	
			//(15,15) for cable link threshold respectively. (119, 119) for air link threshold.
			//Plus 84 for air link threshold.
			//Down to rate 18Mbps.
			bTryDown = TRUE;
		}
  		// else if ( (CurrRetryRate<14) && (pHalData->LastRetryRate<15) && IncludedInSupportedRates(Adapter, 72)) //TO DO: need to consider (RSSI)
  		else if ( (CurrRetryRate<20) && (priv->LastRetryRate<21)) //TO DO: need to consider (RSSI)
		{	
			bTryUp = TRUE;	
		}
	}
	else if ( CurrentOperaRate == 36 )
	{
		//2For 18Mbps
		if ( ((CurrRetryRate>109) && (priv->LastRetryRate>109)))
		{
			//(99,99) for cable link, (109,109) for air link.
			//Down to rate 11Mbps.
			bTryDown = TRUE;
		}
		// else if ( (CurrRetryRate<15) && (pHalData->LastRetryRate<16) && IncludedInSupportedRates(Adapter, 48)) //TO DO: need to consider (RSSI)	
		else if ( (CurrRetryRate<25) && (priv->LastRetryRate<26)) //TO DO: need to consider (RSSI)
		{	
			bTryUp = TRUE;	
		}
	}
	else if ( CurrentOperaRate == 22 )
	{
		//2For 11Mbps
		// if (CurrRetryRate>299 && IncludedInSupportedRates(Adapter, 11))
		if (CurrRetryRate>95)
		{
			bTryDown = TRUE;
		}
		else if (CurrRetryRate<55)//&& (device->LastRetryRate<55) ) //TO DO: need to consider (RSSI)
		{
			bTryUp = TRUE;
		}
	}	
	else if ( CurrentOperaRate == 11 )
	{
		//2For 5.5Mbps
		// if (CurrRetryRate>159 && IncludedInSupportedRates(Adapter, 4) ) 
		if (CurrRetryRate>149) 
		{	
			bTryDown = TRUE;			
		}
		// else if ( (CurrRetryRate<30) && (pHalData->LastRetryRate<30) && IncludedInSupportedRates(Adapter, 22) )
		else if ( (CurrRetryRate<60) && (priv->LastRetryRate < 65))
		{
			bTryUp = TRUE;
		}		
	}
	else if ( CurrentOperaRate == 4 )
	{
		//2For 2 Mbps
		if((CurrRetryRate>99) && (priv->LastRetryRate>99))
		{
			bTryDown = TRUE;			
	}
		// else if ( (CurrRetryRate<50) && (pHalData->LastRetryRate<65) && IncludedInSupportedRates(Adapter, 11) )
		else if ( (CurrRetryRate < 65) && (priv->LastRetryRate < 70))
	{
			bTryUp = TRUE;
		}
	}
	else if ( CurrentOperaRate == 2 )
		{
		//2For 1 Mbps
		// if ( (CurrRetryRate<50) && (pHalData->LastRetryRate<65) && IncludedInSupportedRates(Adapter, 4))
		if ( (CurrRetryRate<70) && (priv->LastRetryRate<75))
			{
			bTryUp = TRUE;
		}
	}

	//1 Test Upgrading Tx Rate
	// Sometimes the cause of the low throughput (high retry rate) is the compatibility between the AP and NIC.
	// To test if the upper rate may cause lower retry rate, this mechanism randomly occurs to test upgrading tx rate.
	if(!bTryUp && !bTryDown && (priv->TryupingCount == 0) && (priv->TryDownCountLowData == 0)
		&& CurrentOperaRate != 108 && priv->FailTxRateCount < 2)
	{
		if(jiffies% (CurrRetryRate + 101) == 0)
		{
			bTryUp = TRUE;	
			priv->bTryuping = TRUE;
			//RT_TRACE(COMP_RATE, DBG_LOUD, ("StaRateAdaptive87B(): Randomly try upgrading...\n"));
		}
	}

	//1 Rate Mechanism
	if(bTryUp)
	{
		priv->TryupingCount++;
		priv->TryDownCountLowData = 0;
			
		//
		// Check more times if we need to upgrade indeed.
		// Because the largest value of pHalData->TryupingCount is 0xFFFF and 
		// the largest value of pHalData->FailTxRateCount is 0x14,
		// this condition will be satisfied at most every 2 min.
		//
		if((priv->TryupingCount > (TryUpTh + priv->FailTxRateCount * priv->FailTxRateCount)) ||
			(CurrSignalStrength > priv->LastFailTxRateSS) || priv->bTryuping)
		{
			priv->TryupingCount = 0;
			// 
			// When transfering from CCK to OFDM, DIG is an important issue.
			//
			if(CurrentOperaRate == 22)
				bUpdateInitialGain = TRUE;
			// (1)To avoid upgrade frequently to the fail tx rate, add the FailTxRateCount into the threshold.
			// (2)If the signal strength is increased, it may be able to upgrade.
			CurrentOperaRate = GetUpgradeTxRate(dev, CurrentOperaRate);
			//RT_TRACE(COMP_RATE, DBG_LOUD, ("StaRateAdaptive87B(): Upgrade Tx Rate to %d\n", pMgntInfo->CurrentOperaRate));
				
			// Update Fail Tx rate and count.
			if(priv->LastFailTxRate != CurrentOperaRate)
			{
				priv->LastFailTxRate = CurrentOperaRate;
				priv->FailTxRateCount = 0;
				priv->LastFailTxRateSS = -200; // Set lowest power.
			}
		}
	}
	else
	{	
		if(priv->TryupingCount > 0)
			priv->TryupingCount --;
	}
	
	if(bTryDown)
	{
		priv->TryDownCountLowData++;
		priv->TryupingCount = 0;
				
		
		//Check if Tx rate can be degraded or Test trying upgrading should fallback.
		if(priv->TryDownCountLowData > TryDownTh || priv->bTryuping)
		{
			priv->TryDownCountLowData = 0;
			priv->bTryuping = FALSE;
			// Update fail information.
			if(priv->LastFailTxRate == CurrentOperaRate)
			{
				priv->FailTxRateCount ++;
				// Record the Tx fail rate signal strength.
				if(CurrSignalStrength > priv->LastFailTxRateSS)
				{
					priv->LastFailTxRateSS = CurrSignalStrength;
				}
			}
			else
			{
				priv->LastFailTxRate = CurrentOperaRate;
				priv->FailTxRateCount = 1;
				priv->LastFailTxRateSS = CurrSignalStrength;
			}
			CurrentOperaRate = GetDegradeTxRate(dev, CurrentOperaRate);
			//
			// When it is CCK rate, it may need to update initial gain to receive lower power packets.
			//
			if(MgntIsCckRate(CurrentOperaRate))
			{
				bUpdateInitialGain = TRUE;
			}
			//RT_TRACE(COMP_RATE, DBG_LOUD, ("StaRateAdaptive87B(): Degrade Tx Rate to %d\n", pMgntInfo->CurrentOperaRate));
		}
	}
	else
	{
		if(priv->TryDownCountLowData > 0)
			priv->TryDownCountLowData --;
	}
			
	// Keep the Tx fail rate count to equal to 0x15 at most.
	// Reduce the fail count at least to 10 sec if tx rate is tending stable.
	if(priv->FailTxRateCount >= 0x15 || 
		(!bTryUp && !bTryDown && priv->TryDownCountLowData == 0 && priv->TryupingCount && priv->FailTxRateCount > 0x6))
	{
		priv->FailTxRateCount --;
	}

	//
	// We need update initial gain when we set tx rate "from OFDM to CCK" or
	// "from CCK to OFDM". 
	//	
SetInitialGain:
	if(bUpdateInitialGain)
	{
		if(MgntIsCckRate(CurrentOperaRate)) // CCK
		{
			if(priv->InitialGain > priv->RegBModeGainStage)
			{
				if(CurrSignalStrength < -85) // Low power, OFDM [0x17] = 26.
				{
					priv->InitialGain = priv->RegBModeGainStage;
				}
				else if(priv->InitialGain > priv->RegBModeGainStage + 1)
				{
					priv->InitialGain -= 2;
				}
				else
				{
					priv->InitialGain --;
				}
				//RT_TRACE(COMP_RATE | COMP_DIG, DBG_LOUD, ("StaRateAdaptive87B(): update init_gain to index %d for date rate %d\n",pHalData->InitialGain, pMgntInfo->CurrentOperaRate));			
				UpdateInitialGain(dev);
			}
		}
		else // OFDM
		{			
			if(priv->InitialGain < 4)
			{
				priv->InitialGain ++;
				//RT_TRACE(COMP_RATE | COMP_DIG, DBG_LOUD, ("StaRateAdaptive87B(): update init_gain to index %d for date rate %d\n",pHalData->InitialGain, pMgntInfo->CurrentOperaRate));			
				UpdateInitialGain(dev);
			}					
		}
	}

	//Record the related info
	ieee->rate = ((int)CurrentOperaRate/2)*10;
	priv->LastRetryRate = CurrRetryRate;
	priv->LastTxThroughput = TxThroughput;
}
#endif

/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/
module_init(rtl8187_usb_module_init);
module_exit(rtl8187_usb_module_exit);
