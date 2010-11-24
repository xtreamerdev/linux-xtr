/*-------------------------------------------------------------------------------
	
	For type defines and data structure defines

--------------------------------------------------------------------------------*/


#ifndef __DRV_TYPES_H__
#define __DRV_TYPES_H__

#include <drv_conf.h>
#include <osdep_service.h>
#include <wlan_bssdef.h>



#ifdef PLATFORM_OS_WINCE
#include <Sdcardddk.h>

#define MAX_ACTIVE_REG_PATH 256

#endif

//john
#define NUM_PRE_AUTH_KEY 16
#define NUM_PMKID_CACHE NUM_PRE_AUTH_KEY

/*
* 	WPA2
*/
typedef struct _PMKID_CANDIDATE {
    NDIS_802_11_MAC_ADDRESS BSSID;
    ULONG Flags;
} PMKID_CANDIDATE, *PPMKID_CANDIDATE;

typedef struct _NDIS_802_11_PMKID_CANDIDATE_LIST
{
    ULONG Version;       // Version of the structure
    ULONG NumCandidates; // No. of pmkid candidates
    PMKID_CANDIDATE CandidateList[1];
} NDIS_802_11_PMKID_CANDIDATE_LIST, *PNDIS_802_11_PMKID_CANDIDATE_LIST;




enum _NIC_VERSION {
	
	RTL8711_NIC,
	RTL8712_NIC,
	RTL8187_NIC,
	RTL8192U_NIC,
	RTL8713_NIC,
	RTL8716_NIC
};


enum _HW_VERSION {

	RTL8711_FPGA,
	RTL8711_1stCUT,//A Cut (RTL8711_ASIC)
	RTL8711_2ndCUT,//B Cut
	RTL8711_3rdCUT,//C Cut
};


enum _LOOPBACK_TYPE {

 RTL8711_AIR_TRX = 0,
 RTL8711_MAC_LBK,
 RTL8711_BB_LBK,
 RTL8711_MAC_FW_LBK = 4,
 RTL8711_BB_FW_LBK = 8,

};


enum RTL8711_HCI_TYPE {

	RTL8711_SDIO,
	RTL8711_CFIO,
	RTL8711_USB,
	RTL8187_USB,
	RTL8192U_USB,
};

enum	_REG_PREAMBLE_MODE{
	PREAMBLE_LONG	= 1,
	PREAMBLE_AUTO	= 2,
	PREAMBLE_SHORT	= 3,
};


#define MAX_MCAST_LIST_NUM					32

//From TypeDef.h
#define NIC_HEADER_SIZE				14
#define NIC_MAX_PACKET_SIZE			1514

#define sCrcLng							4		// octets for crc32 (FCS, ICV)
#define sMacHdrLng						24		// octets in data header, no WEP
#define sQoSCtlLng						2

typedef struct _ADAPTER _adapter, ADAPTER,*PADAPTER;

#include <rtl871x_cmd.h>
#include <wlan_bssdef.h>
#include <rtl871x_xmit.h>
#include <rtl871x_recv.h>
#include <rtl8711_qos.h>
#include <rtl871x_security.h>
#include <rtl871x_pwrctrl.h>
#include <rtl871x_io.h>
#include <rtl871x_eeprom.h>
#include <sta_info.h>
#include <rtl871x_mlme.h>
#include <rtl871x_mp.h>
#include <rtl871x_debug.h>
#include <rtl871x_rf.h>
#include <rtl871x_event.h>

#ifdef CONFIG_RTL8192U

#include <wlan.h>
#include <wifi.h>

#define DUMP_92U(ptr,len) \
	do{\
		int i;\
		for(i=1;i<=len;i++){\
			printk("%02x ", ((u8*)ptr)[i-1]);\
			if(i%8==0) printk("  ");\
			if(i%16 ==0) printk("\n");\
		}\
		printk("\n");\
	}while(0); 

#elif defined(CONFIG_RTL8187)

#if 1
#define DUMP_87B(ptr,len) \
	do{\
		int i;\
		for(i=1;i<=len;i++){\
			printk("%02x ", ((u8*)ptr)[i-1]);\
			if(i%8==0) printk("  ");\
			if(i%16 ==0) printk("\n");\
		}\
		printk("\n");\
	}while(0); 
#else
#define DUMP_87B(ptr,len) 
#endif

#include <rtl8187/wlan.h>
#endif


#define NIC_HEADER_SIZE				14	


#undef ON_VISTA
//added by Jackson

#ifndef ON_VISTA

//
// Bus driver versions
//

#define SDBUS_DRIVER_VERSION_1          0x100
#define SDBUS_DRIVER_VERSION_2          0x200

#define    SDP_FUNCTION_TYPE	4
#define    SDP_BUS_DRIVER_VERSION 5
#define    SDP_BUS_WIDTH 6
#define    SDP_BUS_CLOCK 7
#define    SDP_BUS_INTERFACE_CONTROL 8
#define    SDP_HOST_BLOCK_LENGTH 9
#define    SDP_FUNCTION_BLOCK_LENGTH 10
#define    SDP_FN0_BLOCK_LENGTH 11
#define    SDP_FUNCTION_INT_ENABLE 12



#endif

//for ioctl

#define MAKE_DRIVER_VERSION(_MainVer,_MinorVer)	((((u32)(_MainVer))<<16)+_MinorVer)

#define NIC_HEADER_SIZE				14			//!< can be moved to typedef.h
#define NIC_MAX_PACKET_SIZE			1514		//!< can be moved to typedef.h
#define NIC_MAX_SEND_PACKETS			10		// max number of send packets the MiniportSendPackets function can accept, can be moved to typedef.h
#define NIC_VENDOR_DRIVER_VERSION       MAKE_DRIVER_VERSION(0,001)	//!< can be moved to typedef.h
#define NIC_MAX_PACKET_SIZE			1514		//!< can be moved to typedef.h


struct registry_priv
{    
	u8	chip_version;
	u8	nic_version;
	u8	rfintfs;
	u8	lbkmode;
	u8	hci;
	u8	network_mode;	//infra, ad-hoc, auto	  
	NDIS_802_11_SSID	ssid;
	u8	channel;//ad-hoc support requirement 
	u8	wireless_mode;//A, B, G, auto
	u8	vrtl_carrier_sense;//Enable, Disable, Auto
	u8	vcs_type;//RTS/CTS, CTS-to-self
	u16	rts_thresh;
	u16  frag_thresh;	
	u8	preamble;//long, short, auto
	u8  scan_mode;//active, passive
	u8  adhoc_tx_pwr;
	u8      	     soft_ap;
	u8      	     smart_ps;  
	  u8                  power_mgnt;
	  u8                  radio_enable;
	  u8                  long_retry_lmt;
	  u8                  short_retry_lmt;
  	  u16                 busy_thresh;
    	  u8                  ack_policy;
	  u8		     mp_mode;	
	  u8 		     software_encrypt;
	  u8 		     software_decrypt;	  

	  //UAPSD
	  u8		     wmm_enable;
	  u8		     uapsd_enable;	  
	  u8		     uapsd_max_sp;
	  u8		     uapsd_acbk_en;
	  u8		     uapsd_acbe_en;
	  u8		     uapsd_acvi_en;
	  u8		     uapsd_acvo_en;	  

	  NDIS_WLAN_BSSID_EX    dev_network;
};


//For registry parameters
#define RGTRY_OFT(field) ((ULONG)FIELD_OFFSET(struct registry_priv,field))
#define RGTRY_SZ(field)   sizeof(((struct registry_priv*) 0)->field)
#define BSSID_OFT(field) ((ULONG)FIELD_OFFSET(NDIS_WLAN_BSSID_EX,field))
#define BSSID_SZ(field)   sizeof(((PNDIS_WLAN_BSSID_EX) 0)->field)

#ifdef PLATFORM_LINUX

#define NDIS_STRING_CONST(x)	x

enum _NDIS_PARAM_TYPE {

 NdisParameterInteger = 0,
 NdisParameterHexInteger,
 NdisParameterString,
 NdisParameterMultiString, 
 
};

#endif


typedef struct _MP_REG_ENTRY
{
#ifdef PLATFORM_OS_XP
	NDIS_STRING		RegName;	// variable name text
	BOOLEAN			bRequired;	// 1 -> required, 0 -> optional
#endif	
#ifdef PLATFORM_LINUX
       unsigned char *RegName;
       int			bRequired;
#endif

	u8			Type;		// NdisParameterInteger/NdisParameterHexInteger/NdisParameterStringle/NdisParameterMultiString
	uint			FieldOffset;	// offset to MP_ADAPTER field
	uint			FieldSize;	// size (in bytes) of the field
	
#ifdef UNDER_AMD64
	u64			Default;
#else
	u32			Default;		// default value to use
#endif

	u32			Min;			// minimum value allowed
	u32			Max;		// maximum value allowed
} MP_REG_ENTRY, *PMP_REG_ENTRY;



typedef struct _OCTET_STRING{
	u8      *Octet;
	u16      Length;
} OCTET_STRING, *POCTET_STRING;

typedef enum _FIRMWARE_STATUS{
	FW_STATUS_0_INIT = 0,
	FW_STATUS_1_MOVE_BOOT_CODE = 1,
	FW_STATUS_2_MOVE_MAIN_CODE = 2,
	FW_STATUS_3_TURNON_CPU = 3,
	FW_STATUS_4_MOVE_DATA_CODE = 4,
	FW_STATUS_5_READY = 5,
}FIRMWARE_STATUS;

struct hal_priv{
	u8 (*hal_bus_init)(_adapter * adapter);
	u8  (*hal_bus_deinit)(_adapter * adapter);
	FIRMWARE_STATUS	FirmwareStatus;
};

#ifdef CONFIG_USB_HCI

typedef enum _RT_BOOT_TYPE {
	BOOT_FROM_SRAM = 0,		//!< Boot from SRAM
	BOOT_FROM_NAND = 1,		//!< Boot from NAND flash
	BOOT_FROM_NOR = 2,		//!< Boot from NOR flash
	BOOT_FROM_UNKNOWN	//!< Boot from Unknown
} RT_BOOT_TYPE;

#endif


struct dvobj_priv {

	_adapter * padapter;

#ifdef CONFIG_SDIO_HCI

#ifdef PLATFORM_OS_LINUX

	PSDFUNCTION psdfunc; 
	PSDDEVICE psddev;
	
#endif	

#ifdef PLATFORM_OS_XP
	PDEVICE_OBJECT	pphysdevobj;//pPhysDevObj;
	PDEVICE_OBJECT	pfuncdevobj;//pFuncDevObj;
	PDEVICE_OBJECT	pnextdevobj;//pNextDevObj;
	SDBUS_INTERFACE_STANDARD	sdbusinft;//SdBusInterface;
	u8	nextdevstacksz;//unsigned char			 NextDeviceStackSize;
#endif

#ifdef PLATFORM_OS_WINCE

	SD_DEVICE_HANDLE hDevice;

#endif

	u8	func_number;//unsigned char			FunctionNumber;
	u32	block_transfer_len;//unsigned long			BLOCK_TRANSFER_LEN;
	u16		driver_version;
	u8  block_mode;
	u32	rxpktcount;//unsigned int		RXPktCount; // using in rx-polling-rdy-bit
	u8	regpwrsave;//unsigned char 	RegPowerSaveMode;
#endif	
#ifdef CONFIG_CFIO_HCI

     u32                                          io_base_address;    
     u32                                          io_range;           
     u32                                          interrupt_level;
     NDIS_PHYSICAL_ADDRESS     	mem_phys_address;
     PVOID                                      port_offset;
 
     NDIS_MINIPORT_INTERRUPT  		interrupt; 
#endif
 
#ifdef CONFIG_USB_HCI
#ifdef PLATFORM_LINUX
	struct usb_device *pusbdev;
	PURB		ts_in_urb;
#endif	
	u8	ishighspeed;//link to USB2.0 or USB1.1	//	BOOLEAN			bUSBIsHighSpeed;	
#ifdef PLATFORM_WINDOWS
	//3 related device objects
	PDEVICE_OBJECT	pphysdevobj;//pPhysDevObj;
	PDEVICE_OBJECT	pfuncdevobj;//pFuncDevObj;
	PDEVICE_OBJECT	pnextdevobj;//pNextDevObj;

	u8	nextdevstacksz;//unsigned char			 NextDeviceStackSize;	//= (CHAR)CEdevice->pUsbDevObj->StackSize + 1; 

	//urb for control diescriptor request
	struct _URB_CONTROL_DESCRIPTOR_REQUEST	descriptor_urb;
	PUSB_CONFIGURATION_DESCRIPTOR				pconfig_descriptor;//UsbConfigurationDescriptor;
	u32	config_descriptor_len;//ULONG						UsbConfigurationDescriptorLength;

	u8	cpu_boot_type;//RT_BOOT_TYPE	CPUBootType;			//!< RTL8711 Boot Type, Driver must read it by Vendor Request at Initialization.

	//3Endpoint handles
	HANDLE			regin_pipehandle;//RegInterruptInPipeHandle;
	HANDLE			regout_pipehandle;//RegInterruptOutPipeHandle;
	HANDLE			be_pipehandle;//BEBulkOutPipeHandle;		//Tx AC FiFO
	HANDLE			bk_pipehandle;//BKBulkOutPipeHandle;
	HANDLE			vi_pipehandle;//VIBulkOutPipeHandle;
	HANDLE			vo_pipehandle;//VOBulkOutPipeHandle;
	HANDLE			bm_pipehandle;//BMBulkOutPipeHandle;		//Broad/Multicast
	HANDLE			ts_pipehandle;//TSBulkOutPipeHandle;	
	HANDLE			rx_pipehandle;//RxBulkInPipeHandle;
	HANDLE			interrupt_pipehandle;//TSInterruptInPipeHandle;	//Event report
	HANDLE			scsi_in_pipehandle;
	HANDLE			scsi_out_pipehandle;

	
	_lock	in_lock;
	PIRP		ts_in_irp;//PIRP		tsint_irp[2];
	URB		ts_in_urb;//URB		tsint_urb[2];
#endif
	_sema	tx_retevt;//NDIS_EVENT				txirp_retevt;//AllRegWriteReturnedEvent;
	u8		txirp_cnt;

	//4	TS Interrupt In Buffer
	_sema	hisr_retevt;//NDIS_EVENT				interrupt_retevt;//AllTSIrpReturnedEvent;
	u8		ts_in_cnt;//tsint_cnt;
	u8		ts_in_buf[128];//tsint_buf[2][128];
	u8		ts_in_pendding;//tsint_pendding[2];

#if defined(CONFIG_RTL8192U)
	struct rtl8192u_hw_info hw_info;
#elif defined(CONFIG_RTL8187)
	struct rtl87b_hw_info hw_info;
#endif

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	PURB	rxcmd_urb;
	u32		*rxcmd_buf;
#endif

#endif

	NDIS_STATUS (*dvobj_init)(_adapter * adapter);

	void  (*dvobj_deinit)(_adapter * adapter);

	NDIS_STATUS  (*rxirp_init)(_adapter * adapter);
	NDIS_STATUS  (*rxirp_deinit)(_adapter * adapter);
};

#define SCSI_BUFFER_NUMBER 4
#define MAX_TXCMD 100

struct SCSI_BUFFER_ENTRY{
	u8 entry_memory[SCSI_ENTRY_MEMORY_SIZE];
#ifdef CONFIG_USB_HCI
	PURB purb;
#endif
#ifdef PLATFORM_WINDOWS	
        PIRP pirp;
#endif
	u8 scsi_irp_pending;
	_adapter * padapter;
};

struct SCSI_BUFFER {
	int head;
	int tail;
	struct SCSI_BUFFER_ENTRY *pscsi_buffer_entry;
};

//-------------------- CONFIG_RTL8192U --------------

#ifdef CONFIG_RTL8192U

//
// Represent Channel Width in HT Capabilities
//
typedef enum _HT_CHANNEL_WIDTH{
	HT_CHANNEL_WIDTH_20 = 0,
	HT_CHANNEL_WIDTH_20_40 = 1,
}HT_CHANNEL_WIDTH, *PHT_CHANNEL_WIDTH;


typedef enum _HT_EXTCHNL_OFFSET{
	HT_EXTCHNL_OFFSET_NO_EXT = 0,
	HT_EXTCHNL_OFFSET_UPPER = 1,
	HT_EXTCHNL_OFFSET_NO_DEF = 2,
	HT_EXTCHNL_OFFSET_LOWER = 3,
}HT_EXTCHNL_OFFSET, *PHT_EXTCHNL_OFFSET;



struct _HT_CAPABILITY_ELE{

	//HT capability info
	u8	AdvCoding:1;
	u8	ChlWidth:1;
	u8	MimoPwrSave:2;
	u8	GreenField:1;
	u8	ShortGI20Mhz:1;
	u8	ShortGI40Mhz:1;
	u8	TxSTBC:1;	
	u8	RxSTBC:2;
	u8	DelayBA:1;
	u8	MaxAMSDUSize:1;
	u8	DssCCk:1;
	u8	PSMP:1;
	u8	Rsvd1:1;
	u8	LSigTxopProtect:1;

	//MAC HT parameters info
	u8	MaxRxAMPDUFactor:2;
	u8	MPDUDensity:3;
	u8	Rsvd2:3;

	//Supported MCS set
	u8	MCS[16];
			

	//Extended HT Capability Info
	u16	ExtHTCapInfo;

	//TXBF Capabilities
	u8	TxBFCap[4];

	//Antenna Selection Capabilities
	u8	ASCap;

}__attribute__ ((packed));
typedef	struct _HT_CAPABILITY_ELE HT_CAPABILITY_ELE, *PHT_CAPABILITY_ELE;


typedef struct _HT_INFORMATION_ELE{
	u8	ControlChl;

	u8	ExtChlOffset:2;
	u8	RecommemdedTxWidth:1;
	u8	RIFS:1;
	u8	PSMPAccessOnly:1;
	u8	SrvIntGranularity:3;

	u8	OptMode:2;
	u8	NonGFDevPresent:1;
	u8	Revd1:5;
	u8	Revd2:8;

	u8	Rsvd3:6;
	u8	DualBeacon:1;
	u8	DualCTSProtect:1;

	u8	SecondaryBeacon:1;
	u8	LSigTxopProtectFull:1;
	u8	PcoActive:1;
	u8	PcoPhase:1;
	u8	Rsvd4:4;

	u8	BasicMSC[16];
}HT_INFORMATION_ELE, *PHT_INFORMATION_ELE;






typedef struct _RT_HIGH_THROUGHPUT{


	u8				bEnableHT;
	u8				bCurrentHTSupport;

	u8				bRegBW40MHz;				// Tx 40MHz channel capablity
	u8				bCurBW40MHz;				// Tx 40MHz channel capability

	u8				bRegShortGI40MHz;			// Tx Short GI for 40Mhz
	u8				bCurShortGI40MHz;			// Tx Short GI for 40MHz

	u8				bRegShortGI20MHz;			// Tx Short GI for 20MHz
	u8				bCurShortGI20MHz;			// Tx Short GI for 20MHz

	u8				bRegSuppCCK;				// Tx CCK rate capability
	u8				bCurSuppCCK;				// Tx CCK rate capability

	// 802.11n spec version for "peer"
	//HT_SPEC_VER			ePeerHTSpecVer;
	

	// HT related information for "Self"
	HT_CAPABILITY_ELE	SelfHTCap;					// This is HT cap element sent to peer STA, which also indicate HT Rx capabilities.
	HT_INFORMATION_ELE	SelfHTInfo;					// This is HT info element sent to peer STA, which also indicate HT Rx capabilities.

	// HT related information for "Peer"
	u8				PeerHTCapBuf[32];
	u8				PeerHTInfoBuf[32];


	// A-MSDU related
	u8				bAMSDU_Support;			// This indicates Tx A-MSDU capability
	u16				nAMSDU_MaxSize;			// This indicates Tx A-MSDU capability
	u8				bCurrent_AMSDU_Support;	// This indicates Tx A-MSDU capability
	u16				nCurrent_AMSDU_MaxSize;	// This indicates Tx A-MSDU capability
	

	// AMPDU  related <2006.08.10 Emily>
	u8				bAMPDUEnable;				// This indicate Tx A-MPDU capability
	u8				bCurrentAMPDUEnable;		// This indicate Tx A-MPDU capability		
	u8				AMPDU_Factor;				// This indicate Tx A-MPDU capability
	u8				CurrentAMPDUFactor;		// This indicate Tx A-MPDU capability
	u8				MPDU_Density;				// This indicate Tx A-MPDU capability
	u8				CurrentMPDUDensity;			// This indicate Tx A-MPDU capability

	// Forced A-MPDU enable
	//HT_AGGRE_MODE_E	ForcedAMPDUMode;
	u8				ForcedAMPDUFactor;
	u8				ForcedMPDUDensity;

	// Forced A-MSDU enable
	//HT_AGGRE_MODE_E	ForcedAMSDUMode;
	u16				ForcedAMSDUMaxSize;

	u8				bForcedShortGI;

	u8				CurrentOpMode;

	// MIMO PS related
	u8				SelfMimoPs;
	u8				PeerMimoPs;
	
	// 40MHz Channel Offset settings.
	HT_EXTCHNL_OFFSET	CurSTAExtChnlOffset;
	u8				bCurTxBW40MHz;	// If we use 40 MHz to Tx
	u8				PeerBandwidth;

	// For Bandwidth Switching
	u8				bSwBwInProgress;
	u8				bChnlOp_Scan;
	u8				bChnlOp_SwBw;
	u8				SwBwStep;
	//RT_TIMER			SwBwTimer;

	// For Realtek proprietary A-MPDU factor for aggregation
	u8				bRegRT2RTAggregation;
	u8				bCurrentRT2RTAggregation;
	u8				bCurrentRT2RTLongSlotTime;
	u8				szRT2RTAggBuffer[10];

	// Rx Reorder control
	u8				bRegRxReorderEnable;
	u8				bCurRxReorderEnable;
	u8				RxReorderWinSize;
	u8				RxReorderPendingTime;
	u16				RxReorderDropCounter;
	
	u8				HTCurrentOperaRate;
	u8				HTHighestOperaRate;
	HT_CHANNEL_WIDTH	CurrentChannelBW;
	u8					nCur40MhzPrimeSC;	// Control channel sub-carrier

	u8		           dot11HTOperationalRateSet[16];	
#ifdef USB_RX_AGGREGATION_SUPPORT
	u8				UsbRxFwAggrEn;
	u8				UsbRxFwAggrPageNum;
	u8				UsbRxFwAggrPacketNum;
	u8				UsbRxFwAggrTimeout;
#endif
}RT_HIGH_THROUGHPUT, *PRT_HIGH_THROUGHPUT;



#endif /*CONFIG_RTL8192U*/

struct _ADAPTER{

#ifdef PLATFORM_WINDOWS
		//3 NDIS handles
	_nic_hdl		hndis_adapter;//hNdisAdapter;			//NDISMiniportAdapterHandle
	_nic_hdl		hndis_config;//hNdisConfiguration;
	NDIS_STRING fw_img;

	
#endif
	struct SCSI_BUFFER *pscsi_buf;
	int max_txcmd_nr;
	int scsi_buffer_size;

	struct dvobj_priv dvobjpriv;
	u8				hw_init_completed;
	u32	asic_ver;
	u32	io_baseaddr;
	
#ifdef PLATFORM_LINUX
	_nic_hdl pnetdev;
	int bup;
#ifdef LINUX_TASKLET
	struct tasklet_struct irq_rx_tasklet;//use tasklet to replace thread
	int rx_thread_complete;
#endif
#ifdef LINUX_TX_TASKLET
	struct tasklet_struct irq_tx_tasklet;//use tasklet to replace thread
#endif
#endif
	
	struct	mlme_priv	mlmepriv;
	struct	cmd_priv	cmdpriv;
	struct	evt_priv	evtpriv;
	struct	io_queue	*pio_queue;
	struct	xmit_priv	xmitpriv;
	struct	recv_priv	recvpriv;
	struct	sta_priv	stapriv;
	struct	security_priv	securitypriv;
	struct	qos_priv	qospriv;
	struct	registry_priv	registrypriv;
	struct	wlan_acl_pool	acl_list;
	struct	pwrctrl_priv	pwrctrlpriv;
	struct 	eeprom_priv eeprompriv;
	struct	hal_priv	halpriv;
		
#ifdef CONFIG_MP_INCLUDED

       struct mp_priv  mppriv;

#endif


#ifdef PLATFORM_OS_WINCE

	SD_DEVICE_HANDLE hDevice;     
	u8            ActivePath[MAX_ACTIVE_REG_PATH];

#endif

	s32	bDriverStopped; 
	s32	bSurpriseRemoved;

	u32	IsrContent;
	u32	ImrContent;

// just for async_test
	u32 async_cnt;
	

	u32	NdisPacketFilter;

	u8	EepromAddressSize;
	u8	MCList[MAX_MCAST_LIST_NUM][6];
	u32	MCAddrCount;					

	u64			DriverUpTime;
	u64			LastScanCompleteTime;


	_thread_hdl_	cmdThread;
	_thread_hdl_	evtThread;
	_thread_hdl_	xmitThread;
	_thread_hdl_	recvThread;

#ifdef ENABLE_MGNT_THREAD
	_thread_hdl_ 	mgntThread;
#endif


#if (defined(CONFIG_RTL8187) ||defined(CONFIG_RTL8192U))
	_thread_hdl_	mlmeThread;
	_sema terminate_mlmethread_sema;
	struct mib_info _sys_mib;
	struct proc_dir_entry *dir_dev;
	u16 		ieee80211_seq_ctl;
#endif

#ifdef CONFIG_RTL8192U
	_lock tx_fw_lock;
	_lock rf_write_lock;

	RT_HIGH_THROUGHPUT	HTInfo;
	_queue	pending_tx_fw_cmd_queue;
	_queue	free_tx_fw_cmd_queue;
	u8 *	tx_fw_cmd_list_buf;
#endif

#ifdef CONFIG_USB_HCI 
	_sema	usb_suspend_sema;
#endif
		   
};	
  
static __inline u8 *myid(struct eeprom_priv *peepriv)
{
	return (peepriv->mac_addr);
}


#endif //__DRV_TYPES_H__

