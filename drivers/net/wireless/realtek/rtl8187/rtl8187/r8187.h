/* 
   This is part of rtl8187 OpenSource driver.
   Copyright (C) Andrea Merello 2004-2005  <andreamrl@tiscali.it> 
   Released under the terms of GPL (General Public Licence)
   
   Parts of this driver are based on the GPL part of the 
   official realtek driver
   
   Parts of this driver are based on the rtl8180 driver skeleton 
   from Patric Schenke & Andres Salomon
   
   Parts of this driver are based on the Intel Pro Wireless 2100 GPL driver
   
   We want to tanks the Authors of those projects and the Ndiswrapper 
   project Authors.
*/

#ifndef R8180H
#define R8180H


#define RTL8187_MODULE_NAME "rtl8187"
#define DMESG(x,a...) printk(KERN_INFO RTL8187_MODULE_NAME ": " x "\n", ## a)
#define DMESGW(x,a...) printk(KERN_WARNING RTL8187_MODULE_NAME ": WW:" x "\n", ## a)
#define DMESGE(x,a...) printk(KERN_WARNING RTL8187_MODULE_NAME ": EE:" x "\n", ## a)

#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/config.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
//#include <linux/pci.h>
#include <linux/usb.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/rtnetlink.h>	//for rtnl_lock()
#include <linux/wireless.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>	// Necessary because we use the proc fs
#include <linux/if_arp.h>
#include <linux/random.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/semaphore.h>

#include "ieee80211.h"

#define RTL8187_SUFFIX(item)            rtl8187_##item

//added for HW security, john.0629
#define FALSE 0
#define TRUE 1
#define MAX_KEY_LEN     61
#define KEY_BUF_SIZE    5

#define BIT0	0x00000001
#define BIT1	0x00000002
#define BIT2	0x00000004
#define BIT3	0x00000008
#define BIT4	0x00000010
#define BIT5	0x00000020
#define BIT6	0x00000040
#define BIT7	0x00000080
#define BIT8	0x00000100
#define BIT9	0x00000200
#define BIT10	0x00000400
#define BIT11	0x00000800
#define BIT12	0x00001000
#define BIT13	0x00002000
#define BIT14	0x00004000
#define BIT15	0x00008000
#define BIT16	0x00010000
#define BIT17	0x00020000
#define BIT18	0x00040000
#define BIT19	0x00080000
#define BIT20	0x00100000
#define BIT21	0x00200000
#define BIT22	0x00400000
#define BIT23	0x00800000
#define BIT24	0x01000000
#define BIT25	0x02000000
#define BIT26	0x04000000
#define BIT27	0x08000000
#define BIT28	0x10000000
#define BIT29	0x20000000
#define BIT30	0x40000000
#define BIT31	0x80000000

//8187B Security
#define RWCAM                   0xA0                    // Software read/write CAM config
#define WCAMI                   0xA4                    // Software write CAM input content
#define RCAMO                   0xA8                    // Output value from CAM according to 0xa0 setting
#define DCAM                    0xAC                    // Debug CAM Interface
#define SECR                    0xB0                    // Security configuration register
#define AESMSK_FC           0xB2    // AES Mask register for frame control (0xB2~0xB3). Added by Annie, 2006-03-06.
#define AESMSK_SC		0x1FC	// AES Mask for Sequence Control (0x1FC~0X1FD). Added by Annie, 2006-03-06.
#define AESMSK_QC		0x1CE			// AES Mask register for QoS Control when computing AES MIC, default = 0x000F. (2 bytes)

#define AESMSK_FC_DEFAULT			0xC78F	// default value of AES MASK for Frame Control Field. (2 bytes)
#define AESMSK_SC_DEFAULT			0x000F	// default value of AES MASK for Sequence Control Field. (2 bytes)
#define AESMSK_QC_DEFAULT			0x000F	// default value of AES MASK for QoS Control Field. (2 bytes)


#define CAM_CONTENT_COUNT       6
#define CFG_DEFAULT_KEY         BIT5
#define CFG_VALID               BIT15

//----------------------------------------------------------------------------
//       8187B WPA Config Register (offset 0xb0, 1 byte)
//----------------------------------------------------------------------------
#define SCR_UseDK                       0x01
#define SCR_TxSecEnable                 0x02
#define SCR_RxSecEnable                 0x04

//----------------------------------------------------------------------------
//       8187B CAM Config Setting (offset 0xb0, 1 byte)
//----------------------------------------------------------------------------
#define CAM_VALID                               0x8000
#define CAM_NOTVALID                    0x0000
#define CAM_USEDK                               0x0020


#define CAM_NONE                                0x0
#define CAM_WEP40                               0x01
#define CAM_TKIP                                0x02
#define CAM_AES                                 0x04
#define CAM_WEP104                              0x05


//#define CAM_SIZE                              16
#define TOTAL_CAM_ENTRY         16
#define CAM_ENTRY_LEN_IN_DW     6                                                                       // 6, unit: in u4byte. Added by Annie, 2006-05-25.
#define CAM_ENTRY_LEN_IN_BYTE   (CAM_ENTRY_LEN_IN_DW*sizeof(u4Byte))    // 24, unit: in u1byte. Added by Annie, 2006-05-25.

#define CAM_CONFIG_USEDK                1
#define CAM_CONFIG_NO_USEDK             0

#define CAM_WRITE                               0x00010000
#define CAM_READ                                0x00000000
#define CAM_POLLINIG                    0x80000000

//=================================================================
//=================================================================

#define EPROM_93c46 0
#define EPROM_93c56 1

#define DEFAULT_FRAG_THRESHOLD 2342U
#define MIN_FRAG_THRESHOLD     256U
#define DEFAULT_BEACONINTERVAL 0x64U
#define DEFAULT_BEACON_ESSID "Rtl8187"

#define DEFAULT_SSID ""
#define DEFAULT_RETRY_RTS 7
#define DEFAULT_RETRY_DATA 7
#define PRISM_HDR_SIZE 64

#define DEFAULT_RATE_ADAPTIVE_TIMER_PERIOD	75 //300ms

typedef enum _WIRELESS_MODE {
	WIRELESS_MODE_UNKNOWN = 0x00,
	WIRELESS_MODE_A = 0x01,
	WIRELESS_MODE_B = 0x02,
	WIRELESS_MODE_G = 0x04,
	WIRELESS_MODE_AUTO = 0x08,
} WIRELESS_MODE;

#define RTL_IOCTL_WPA_SUPPLICANT		SIOCIWFIRSTPRIV+30

typedef struct buffer
{
	struct buffer *next;
	u32 *buf;
	
} buffer;

typedef struct rtl_reg_debug{
        unsigned int  cmd;
        struct {
                unsigned char type;
                unsigned char addr;
                unsigned char page;
                unsigned char length;
        } head;
        unsigned char buf[0xff];
}rtl_reg_debug;

#if 0

typedef struct tx_pendingbuf
{
	struct ieee80211_txb *txb;
	short ispending;
	short descfrag;
} tx_pendigbuf;

#endif

typedef struct Stats
{
	unsigned long txrdu;
//	unsigned long rxrdu;
	//unsigned long rxnolast;
	//unsigned long rxnodata;
//	unsigned long rxreset;
//	unsigned long rxwrkaround;
//	unsigned long rxnopointer;
	unsigned long rxok;
	unsigned long rxurberr;
	unsigned long rxstaterr;
	unsigned long txnperr;
	unsigned long txnpdrop;
	unsigned long txresumed;
//	unsigned long rxerr;
//	unsigned long rxoverflow;
//	unsigned long rxint;
	unsigned long txnpokint;
//	unsigned long txhpokint;
//	unsigned long txhperr;
//	unsigned long ints;
//	unsigned long shints;
	unsigned long txoverflow;
//	unsigned long rxdmafail;
//	unsigned long txbeacon;
//	unsigned long txbeaconerr;
	unsigned long txlpokint;
	unsigned long txlpdrop;
	unsigned long txlperr;
	unsigned long txbeokint;
	unsigned long txbedrop;
	unsigned long txbeerr;
	unsigned long txbkokint;
	unsigned long txbkdrop;
	unsigned long txbkerr;
	unsigned long txviokint;
	unsigned long txvidrop;
	unsigned long txvierr;
	unsigned long txvookint;
	unsigned long txvodrop;
	unsigned long txvoerr;
	unsigned long txbeaconokint;
	unsigned long txbeacondrop;
	unsigned long txbeaconerr;
	unsigned long txmanageokint;
	unsigned long txmanagedrop;
	unsigned long txmanageerr;
	unsigned long txdatapkt;
#ifdef THOMAS_RATEADAPTIVE
	s32		RecvSignalPower;
	u64		NumTxOkTotal;
	u64		NumTxOkBytesTotal;
#endif
} Stats;

typedef struct 	ChnlAccessSetting {
	u16 SIFS_Timer;
	u16 DIFS_Timer; 
	u16 SlotTimeTimer;
	u16 EIFS_Timer;
	u16 CWminIndex;
	u16 CWmaxIndex;
}*PCHANNEL_ACCESS_SETTING,CHANNEL_ACCESS_SETTING;



typedef struct r8180_priv
{
	struct usb_device *udev;
	short epromtype;
	u8    eeprom_channelplan;
	_BOOL usbhighspeed;
	_BOOL bLowNoiseAmplifier;
	int irq;
	struct ieee80211_device *ieee80211;
	
	short card_8187; /* O: rtl8180, 1:rtl8185 V B/C, 2:rtl8185 V D */
	short card_8187_Bversion; /* if TCR reports card V B/C this discriminates */
	short phy_ver; /* meaningful for rtl8225 1:A 2:B 3:C */
	short enable_gpio0;
	enum card_type {PCI,MINIPCI,CARDBUS,USB/*rtl8187*/}card_type;
	short hw_plcp_len;
	short plcp_preamble_mode;
	u8 RegPaModel;
	u8 RegThreeWireMode;
		
	spinlock_t irq_lock;
//	spinlock_t irq_th_lock;
	spinlock_t tx_lock;
	
	u16 irq_mask;
//	short irq_enabled;
	struct net_device *dev;
	short chan;
	short sens;
	short max_sens;
	u8 chtxpwr[15]; //channels from 1 to 14, 0 not used
	u8 chtxpwr_ofdm[15]; //channels from 1 to 14, 0 not used
	u8 cck_txpwr_base;
	u8 ofdm_txpwr_base;
	u8 challow[15]; //channels from 1 to 14, 0 not used
	short up;
	_BOOL bSurpriseRemoved;
	short crcmon; //if 1 allow bad crc frame reception in monitor mode
//	short prism_hdr;
	
//	struct timer_list scan_timer;
	/*short scanpending;
	short stopscan;*/
//	spinlock_t scan_lock;
//	u8 active_probe;
	//u8 active_scan_num;
	struct semaphore wx_sem;
//	short hw_wep;
		
//	short digphy;
//	short antb;
//	short diversity;
//	u8 cs_treshold;
//	short rcr_csense;
	short rf_chip;
//	u32 key0[4];
	short (*rf_set_sens)(struct net_device *dev,short sens);
	void (*rf_set_chan)(struct net_device *dev,short ch);
	void (*rf_close)(struct net_device *dev);
	void (*rf_init)(struct net_device *dev);
	//short rate;
	short promisc;	
	/*stats*/
	struct Stats stats;
	struct iw_statistics wstats;
	struct proc_dir_entry *dir_dev;
	
	/*RX stuff*/
//	u32 *rxring;
//	u32 *rxringtail;
//	dma_addr_t rxringdma;
	struct urb **rx_urb;
#ifdef THOMAS_BEACON
	u32 *oldaddr;
#endif
#ifdef THOMAS_TASKLET
	atomic_t irt_counter;//count for irq_rx_tasklet
	//u16 irt_counter_head;
	//u16 irt_counter_tail;
#endif
#ifdef JACKSON_NEW_RX
        struct sk_buff **pp_rxskb;
        int     rx_inx;
#endif

#ifdef JOHN_DIG
	u8 RegBModeGainStage;
	u8 RegDigOfdmFaUpTh;
	u8 InitialGain;
	u8 DIG_NumberFallbackVote;
	u8 DIG_NumberUpgradeVote;
	u32 FalseAlarmRegValue;

	u8 StageCCKTh;
	u16 CCKUpperTh;
	u16 CCKLowerTh;

	struct work_struct dig_thread;
	u8 have_dig_thread;
#endif /*JOHN_DIG*/
	short  tx_urb_index;
	
	//struct buffer *rxbuffer;
	//struct buffer *rxbufferhead;
	//int rxringcount;
	//u16 rxbuffersize;
	
	//struct sk_buff *rx_skb; 

	//short rx_skb_complete;

	//u32 rx_prevlen;
	//atomic_t tx_lp_pending;
	//atomic_t tx_np_pending;
	atomic_t tx_pending[0x10];//UART_PRIORITY+1

#if 0	
	/*TX stuff*/
	u32 *txlpring;
	u32 *txhpring;
	u32 *txnpring;
	dma_addr_t txlpringdma;
	dma_addr_t txhpringdma;
	dma_addr_t txnpringdma;
	u32 *txlpringtail;
	u32 *txhpringtail;
	u32 *txnpringtail;
	u32 *txlpringhead;
	u32 *txhpringhead;
	u32 *txnpringhead;
	struct buffer *txlpbufs;
	struct buffer *txhpbufs;
	struct buffer *txnpbufs;
	struct buffer *txlpbufstail;
	struct buffer *txhpbufstail;
	struct buffer *txnpbufstail;
	int txringcount;
	int txbuffsize;

	//struct tx_pendingbuf txnp_pending;
	struct tasklet_struct irq_tx_tasklet;
#endif
	struct tasklet_struct irq_rx_tasklet;
	struct urb *rxurb_task;
//	u8 dma_poll_mask;
	//short tx_suspend;
	
	/* adhoc/master mode stuff */
#if 0
	u32 *txbeacontail;
	dma_addr_t txbeaconringdma;
	u32 *txbeaconring;
	int txbeaconcount;
#endif
//	struct ieee_tx_beacon *beacon_buf;
	//char *master_essid;
//	dma_addr_t beacondmabuf;
	//u16 master_beaconinterval;
//	u32 master_beaconsize;
	//u16 beacon_interval;

#ifdef THOMAS_RATEADAPTIVE
	u16	CurrRetryCnt;
	u16	LastRetryCnt;
	u64	LastTxokCnt;
	u64	LastRxokCnt;
	u64	LastTxOKBytes;
	u32	LastTxThroughput;
	u16	LastRetryRate;
	u8	TryupingCountNoData;
	u8	TryDownCountLowData;
	_BOOL	bTryuping;
	u8	LastFailTxRate;
	s32	LastFailTxRateSS;
	u8	FailTxRateCount;
	u16	TryupingCount;

	struct work_struct RateAdaptiveWorkItem;
	_BOOL bRateAdaptive;
#endif

#ifdef THOMAS_MP_IOCTL
	u32	MPState; // 1:set driver to MP state.  0 : Normal driver
	u8	TestItem;
	u8	TestStop;
	u8	btMpCckTxPower;
	u8	btMpOfdmTxPower;
	_BOOL	bCckContTx;
	_BOOL	bOfdmContTx;
#endif

	//2 Tx Related variables
	u16	ShortRetryLimit;
	u16	LongRetryLimit;
	u32	TransmitConfig;
	u8	RegCWinMin;		// For turbo mode CW adaptive. Added by Annie, 2005-10-27.

	//2 Rx Related variables
	u16	EarlyRxThreshold;
	u32	ReceiveConfig;
	u8	AcmControl;

	u8	RFProgType;
	
	u8 retry_data;
	u8 retry_rts;
	u16 rts;

	struct 	ChnlAccessSetting  ChannelAccessSetting;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
	struct work_struct reset_wq;
#else
	struct tq_struct reset_wq;
#endif

	
}r8180_priv;

// for rtl8187
// now mirging to rtl8187B
/*
typedef enum{ 
	LOW_PRIORITY = 0x02,
	NORM_PRIORITY 
	} priority_t;
*/
//for rtl8187B
typedef enum{
	BULK_PRIORITY = 0x01,
	//RSVD0,
	//RSVD1,
	LOW_PRIORITY,
	NORM_PRIORITY, 
	VO_PRIORITY,
	VI_PRIORITY, //0x05
	BE_PRIORITY,
	BK_PRIORITY,
	RSVD2,
	RSVD3,
	BEACON_PRIORITY, //0x0A
	HIGH_PRIORITY,
	MANAGE_PRIORITY,
	RSVD4,
	RSVD5,
	UART_PRIORITY //0x0F
} priority_t;

typedef enum{
	NIC_8187 = 1,
	NIC_8187B
	} nic_t;


typedef u32 AC_CODING;
#define AC0_BE	0		// ACI: 0x00	// Best Effort
#define AC1_BK	1		// ACI: 0x01	// Background
#define AC2_VI	2		// ACI: 0x10	// Video
#define AC3_VO	3		// ACI: 0x11	// Voice
#define AC_MAX	4		// Max: define total number; Should not to be used as a real enum.

//
// ECWmin/ECWmax field.
// Ref: WMM spec 2.2.2: WME Parameter Element, p.13.
//
typedef	union _ECW{
	u8	charData;
	struct
	{
		u8	ECWmin:4;
		u8	ECWmax:4;
	}f;	// Field
}ECW, *PECW;

//
// ACI/AIFSN Field.
// Ref: WMM spec 2.2.2: WME Parameter Element, p.12.
//
typedef	union _ACI_AIFSN{
	u8	charData;
	
	struct
	{
		u8	AIFSN:4;
		u8	ACM:1;
		u8	ACI:2;
		u8	Reserved:1;
	}f;	// Field
}ACI_AIFSN, *PACI_AIFSN;

//
// AC Parameters Record Format.
// Ref: WMM spec 2.2.2: WME Parameter Element, p.12.
//
typedef	union _AC_PARAM{
	u32	longData;
	u8	charData[4];

	struct
	{
		ACI_AIFSN	AciAifsn;
		ECW		Ecw;
		u16		TXOPLimit;
	}f;	// Field
}AC_PARAM, *PAC_PARAM;

#ifdef JOHN_HWSEC
struct ssid_thread {
        struct net_device *dev;
        u8 name[IW_ESSID_MAX_SIZE + 1];
};
#endif

short rtl8180_tx(struct net_device *dev,u32* skbuf, int len,priority_t priority,short morefrag,short rate);
#ifdef JOHN_TKIP

#define read_cam                    RTL8187_SUFFIX(read_cam)
#define write_cam                   RTL8187_SUFFIX(write_cam)
#define CamResetAllEntry            RTL8187_SUFFIX(CamResetAllEntry)
#define CAM_empty_entry             RTL8187_SUFFIX(CAM_empty_entry)
#define CAM_mark_invalid            RTL8187_SUFFIX(CAM_mark_invalid)

u32 read_cam(struct net_device *dev, u8 addr);
void write_cam(struct net_device *dev, u8 addr, u32 data);
void CAM_empty_entry(struct net_device *dev, u8 ucIndex);
void CAM_mark_invalid(struct net_device *dev, u8 ucIndex);
void CamResetAllEntry(struct net_device *dev);
#endif

#define read_nic_byte               RTL8187_SUFFIX(read_nic_byte)
#define read_nic_byte_E             RTL8187_SUFFIX(read_nic_byte_E)
#define read_nic_dword              RTL8187_SUFFIX(read_nic_dword)
#define read_nic_word               RTL8187_SUFFIX(read_nic_word)
#define write_nic_byte              RTL8187_SUFFIX(write_nic_byte)
#define write_nic_byte_E            RTL8187_SUFFIX(write_nic_byte_E)
#define write_nic_word              RTL8187_SUFFIX(write_nic_word)
#define write_nic_dword             RTL8187_SUFFIX(write_nic_dword)
#define force_pci_posting           RTL8187_SUFFIX(force_pci_posting)


u8 read_nic_byte(struct net_device *dev, int x);
u8 read_nic_byte_E(struct net_device *dev, int x);
u32 read_nic_dword(struct net_device *dev, int x);
u16 read_nic_word(struct net_device *dev, int x) ;
void write_nic_byte(struct net_device *dev, int x,u8 y);
void write_nic_byte_E(struct net_device *dev, int x,u8 y);
void write_nic_word(struct net_device *dev, int x,u16 y);
void write_nic_dword(struct net_device *dev, int x,u32 y);
void force_pci_posting(struct net_device *dev);

void rtl8180_rtx_disable(struct net_device *);
void rtl8180_rx_enable(struct net_device *);
void rtl8180_tx_enable(struct net_device *);

void rtl8180_disassociate(struct net_device *dev);
//void fix_rx_fifo(struct net_device *dev);
void rtl8185_set_rf_pins_enable(struct net_device *dev,u32 a);

void rtl8180_set_anaparam(struct net_device *dev,u32 a);
void rtl8185_set_anaparam2(struct net_device *dev,u32 a);
void rtl8180_update_msr(struct net_device *dev);
int rtl8180_down(struct net_device *dev);
int rtl8180_up(struct net_device *dev);
void rtl8180_commit(struct net_device *dev);
void rtl8180_set_chan(struct net_device *dev,short ch);
void write_phy(struct net_device *dev, u8 adr, u8 data);
void write_phy_cck(struct net_device *dev, u8 adr, u32 data);
void write_phy_ofdm(struct net_device *dev, u8 adr, u32 data);
void rtl8185_tx_antenna(struct net_device *dev, u8 ant);
void rtl8187_set_rxconf(struct net_device *dev);

#ifdef JOHN_TKIP
#define setKey              RTL8187_SUFFIX(setKey)
void EnableHWSecurityConfig8187(struct net_device *dev);
void setKey(struct net_device *dev, u8 EntryNo, u8 KeyIndex, u16 KeyType, u8 *MacAddr, u8 DefaultKey, u32 *KeyContent );
#endif/*JOHN_TKIP*/ 

#ifdef JOHN_DIG
void dig_thread(struct net_device *dev);
void UpdateInitialGain(struct net_device *dev);
void UpdateCCKThreshold(struct net_device *dev);
#endif /*JOHN_DIG*/

#ifdef THOMAS_RATEADAPTIVE
void r8187_start_RateAdaptive(struct net_device *dev);
void r8187_stop_RateAdaptive(struct net_device *dev);
void RateAdaptiveWorkItemCallback(struct net_device *dev);
_BOOL MgntIsCckRate(u8 rate);
_BOOL IncludedInSupportedRates(struct net_device *dev, u8 TxRate);
u16 GetUpgradeTxRate(struct net_device *dev , u8 rate);
u16 GetDegradeTxRate(struct net_device *dev , u8 rate);
void StaRateAdaptive87B(struct net_device *dev );
#endif

void ActSetWirelessMode8187(struct net_device* dev, u8	btWirelessMode);

#endif
