#ifndef __RTL8192U_SPEC_H__
#define __RTL8192U_SPEC_H__

#include <drv_conf.h>

/*	Operational registers offsets in PCI (I/O) space. 
 *	RealTek names are used.
 */

#define IDR0 0x0000
#define IDR1 0x0001
#define IDR2 0x0002
#define IDR3 0x0003
#define IDR4 0x0004
#define IDR5 0x0005

#define TAG_XMITFRAME 0
#define TAG_MGNTFRAME 1

#define VO_QUEUE_INX	0
#define VI_QUEUE_INX	1
#define BE_QUEUE_INX	2
#define BK_QUEUE_INX	3
#define TS_QUEUE_INX	4
#define MGT_QUEUE_INX	5
#define BMC_QUEUE_INX	6
#define BCN_QUEUE_INX	7

#define TXCMD_QUEUE_INX 8

#define MGNT_QUEUE_INX		4
#define BEACON_QUEUE_INX 	5

#define RTL819X_TOTAL_RF_PATH	4

//	Register address
#define UNUSED_REGISTER	0x6C

#define PhyAddr 0x007C

#define BRSR	UNUSED_REGISTER

#define WPA_CONFIG 0xb0
#define TOTAL_CAM_ENTRY         32


//#define CCK_TXAGC 0x9D
#define OFDM_TXAGC 0x9E
#define	MCS_TXAGC				0x340	// MCS AGC
#define	CCK_TXAGC				0x348	// CCK AGC


#define DEFAULT_RATE_ADAPTIVE_TIMER_PERIOD	300

typedef enum _RT_STATUS{
	RT_STATUS_SUCCESS,
	RT_STATUS_FAILURE,
	RT_STATUS_PENDING,
	RT_STATUS_RESOURCE
}RT_STATUS,*PRT_STATUS;


typedef enum _HW_VARIABLES{
	HW_VAR_ETHER_ADDR,
	HW_VAR_MULTICAST_REG,		
	HW_VAR_BASIC_RATE,
	HW_VAR_BSSID,
	HW_VAR_MEDIA_STATUS,
	HW_VAR_SECURITY_CONF,
	HW_VAR_BEACON_INTERVAL,
	HW_VAR_ATIM_WINDOW,	
	HW_VAR_LISTEN_INTERVAL,
	HW_VAR_CS_COUNTER,
	HW_VAR_DEFAULTKEY0,
	HW_VAR_DEFAULTKEY1,
	HW_VAR_DEFAULTKEY2,
	HW_VAR_DEFAULTKEY3,
	HW_VAR_SIFS,
	HW_VAR_DIFS,
	HW_VAR_EIFS,
	HW_VAR_SLOT_TIME,
	HW_VAR_ACK_PREAMBLE,
	HW_VAR_CW_CONFIG,
	HW_VAR_CW_VALUES,
	HW_VAR_RATE_FALLBACK_CONTROL,
	HW_VAR_CONTENTION_WINDOW,
	HW_VAR_RETRY_COUNT,
	HW_VAR_TR_SWITCH,
	HW_VAR_COMMAND,			// For Command Register, Annie, 2006-04-07.
	HW_VAR_WPA_CONFIG,		//2004/08/23, kcwu, for 8187 Security config
	HW_VAR_AC_PARAM,			// For AC Parameters, 2005.12.01, by rcnjko.
	HW_VAR_ACM_CTRL,			// For ACM Control, Annie, 2005-12-13.
	HW_VAR_DIS_Req_Qsize,		// For DIS_Reg_Qsize, Joseph
	HW_VAR_CCX_CHNL_LOAD,		// For CCX 2 channel load request, 2006.05.04.
	HW_VAR_CCX_NOISE_HISTOGRAM,	// For CCX 2 noise histogram request, 2006.05.04.
	HW_VAR_CCX_CLM_NHM,			// For CCX 2 parallel channel load request and noise histogram request, 2006.05.12.
	HW_VAR_TxOPLimit,				// For turbo mode related settings, added by Roger, 2006.12.07
	HW_VAR_TURBO_MODE,			// For turbo mode related settings, added by Roger, 2006.12.15.
	HW_VAR_RF_STATE, 			// For change or query RF power state, 061214, rcnjko.
	HW_VAR_RF_OFF_BY_HW,		// For UI to query if external HW signal disable RF, 061229, rcnjko. 
	HW_VAR_BUS_SPEED, 		// In unit of bps. 2006.07.03, by rcnjko.

	HW_VAR_SET_DEV_POWER,	// Set to low power, added by LanHsin, 2007.

	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//1Attention Please!!!<11n or 8190 specific code should be put below this line>
	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
	HW_VAR_RCR,				//for RCR, David 2006,05,11
	HW_VAR_RATR_0,
	HW_VAR_RRSR,
	HW_VAR_CPU_RST,
	HW_VAR_CECHK_BSSID,
	HW_VAR_USB_RX_AGGR

}HW_VARIABLES;

typedef enum _VERSION_819xUsb{
	VERSION_819xUsb_A, //A-Cut
	VERSION_819xUsb_B, //B-Cut
	VERSION_819xUsb_C, //C-Cut
}VERSION_819xUsb, *PVERSION_819xUsb;

struct rtl8192u_hw_info{

	u32	rf_sw_config;
	u8	tsftr;
	u8	pifs;
	u8	eifs_timer;
	u8	sifs_timer;
	u8	slot_time_timer;
	u8	bssid;
	u16	brsr;
	u8	cr;
	u32	ac_txop_queued;	
	u32	tcr;
	u32	rcr;
	u32	timer_int1;
	u8	config0;
	u8	config1;
	u8	config2;
	u32	ama_pram;
	u8	msr;
	u8	config3;
	u8 	config4;
	u8	channel;
	u8	retry_data;
	u8	retry_rts;
	u8	acm_ctrl;
} ;


#ifdef USE_ONE_USB_ENDPOINT
	#define NUM_OF_PAGE_IN_FW_QUEUE_VI		0x0FF
	#define NUM_OF_PAGE_IN_FW_QUEUE_BK		0
	#define NUM_OF_PAGE_IN_FW_QUEUE_BE		0
	#define NUM_OF_PAGE_IN_FW_QUEUE_VO		0
	#define NUM_OF_PAGE_IN_FW_QUEUE_HCCA	0
	#define NUM_OF_PAGE_IN_FW_QUEUE_CMD		0
	#define NUM_OF_PAGE_IN_FW_QUEUE_MGNT	0
	#define NUM_OF_PAGE_IN_FW_QUEUE_HIGH	0
	#define NUM_OF_PAGE_IN_FW_QUEUE_BCN		0
	#define NUM_OF_PAGE_IN_FW_QUEUE_PUB		0
#else
	#define NUM_OF_PAGE_IN_FW_QUEUE_BK		0x020
	#define NUM_OF_PAGE_IN_FW_QUEUE_BE		0x020
	#define NUM_OF_PAGE_IN_FW_QUEUE_VI		0x040
	//#define NUM_OF_PAGE_IN_FW_QUEUE_BK		0x000
	//#define NUM_OF_PAGE_IN_FW_QUEUE_VI		0x0A0
	//#define NUM_OF_PAGE_IN_FW_QUEUE_VO		0x000
	#define NUM_OF_PAGE_IN_FW_QUEUE_VO		0x040
	#define NUM_OF_PAGE_IN_FW_QUEUE_HCCA	0
	#define NUM_OF_PAGE_IN_FW_QUEUE_CMD		0x4
	#define NUM_OF_PAGE_IN_FW_QUEUE_MGNT	0x20
	#define NUM_OF_PAGE_IN_FW_QUEUE_HIGH	0
	#define NUM_OF_PAGE_IN_FW_QUEUE_BCN		0x4
	#define NUM_OF_PAGE_IN_FW_QUEUE_PUB		0x18
#endif

#define APPLIED_RESERVED_QUEUE_IN_FW		0x80000000
#define RSVD_FW_QUEUE_PAGE_BK_SHIFT		0x00
#define RSVD_FW_QUEUE_PAGE_BE_SHIFT		0x08
#define RSVD_FW_QUEUE_PAGE_VI_SHIFT		0x10
#define RSVD_FW_QUEUE_PAGE_VO_SHIFT		0x18
#define RSVD_FW_QUEUE_PAGE_MGNT_SHIFT	0x10
#define RSVD_FW_QUEUE_PAGE_CMD_SHIFT	0x08
#define RSVD_FW_QUEUE_PAGE_BCN_SHIFT	0x00
#define RSVD_FW_QUEUE_PAGE_PUB_SHIFT	0x08

//=============================================================
// TCB buffer management related definition 
//=============================================================
#define MAX_FIRMWARE_INFORMATION_SIZE		32 //2006/04/30 by Emily forRTL8190

#define MAX_FRAGMENT_COUNT			8	// Max number of MPDU per MSDU
#define ENCRYPTION_MAX_OVERHEAD		128 // Follow 8185, 2005.04.14, by rcnjko.
#define MAX_802_11_HEADER_LENGTH	(40 + MAX_FIRMWARE_INFORMATION_SIZE)
#define USB_HWDESC_HEADER_LEN 		sizeof(TX_DESC_819x_USB)
#define TX_PACKET_SHIFT_BYTES 		(USB_HWDESC_HEADER_LEN + sizeof(TX_FWINFO_819xUSB))
#define MAX_TRANSMIT_BUFFER_SIZE 	((1600+(TX_PACKET_SHIFT_BYTES+MAX_802_11_HEADER_LENGTH+ENCRYPTION_MAX_OVERHEAD))*MAX_FRAGMENT_COUNT)

#define AC_VO_PARAM             0x00F0                    // AC_VO Parameters Record
#define AC_VI_PARAM             0x00F4                    // AC_VI Parameters Record
#define AC_BE_PARAM             0x00F8                    // AC_BE Parameters Record
#define AC_BK_PARAM             0x00FC                    // AC_BK Parameters Record  

#define CAM_CONTENT_COUNT       8
#define CFG_DEFAULT_KEY         BIT(5)
#define CFG_VALID               BIT(15)
#define CAM_CONFIG_USEDK                1
#define CAM_CONFIG_NO_USEDK             0



#endif

