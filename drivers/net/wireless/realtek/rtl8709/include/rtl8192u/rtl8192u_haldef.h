#ifndef __RTL8192U_HALDEF_H__
#define __RTL8192U_HALDEF_H__

#include <rtl8192u_spec.h>
#include <hal_init.h>


typedef	enum _RT_RF_TYPE_DEFINITION
{
	RF_1T2R = 0,
	RF_2T4R,
	//RF_1T1R,
	//RF_3T3R,
	//RF_3T4R,
	//RF_4T4R,
	RF_819X_MAX_TYPE
}RT_RF_TYPE_DEF_E;
#define RTL819X_DEFAULT_RF_TYPE	RF_1T2R

#define RTL819X_FW_BOOT_IMG   	"rtl8192Usb\\boot.img"
#define RTL819X_FW_MAIN_IMG		"rtl8192Usb\\main.img"
#define RTL819X_FW_DATA_IMG		"rtl8192Usb\\data.img"
#define RTL819X_PHY_MACPHY_REG	"rtl8192Usb\\MACPHY_reg.txt"
#define RTL819X_PHY_MACPHY_REG_PG	"rtl8192Usb\\MACPHY_reg_PG.txt"
#define RTL819X_PHY_REG			"rtl8192Usb\\PHY_REG.txt"
#define RTL819X_PHY_REG_1T2R	"rtl8192Usb\\PHY_REG_1T2R.txt"
#define RTL819X_AGC_TAB			"rtl8192Usb\\AGC_TAB.txt"
#define RTL819X_PHY_RADIO_A	"rtl8192Usb\\radio_a.txt"
#define RTL819X_PHY_RADIO_B	"rtl8192Usb\\radio_b.txt"	
#define RTL819X_PHY_RADIO_C	"rtl8192Usb\\radio_c.txt"
#define RTL819X_PHY_RADIO_D	"rtl8192Usb\\radio_d.txt"

#ifdef  IMAGE_RX_AGG
#elif IMAGE_4_DEBUG
#elif defined(IMAGE_4_10)
#define BootArrayLength 344
#define MainArrayLength 41408
#define DataArrayLength 564
#define PHY_REGArrayLength 1
#define PHY_REG_1T2RArrayLength 296
#define RadioA_ArrayLength 246
#define RadioB_ArrayLength 78
#define RadioC_ArrayLength 1
#define RadioD_ArrayLength 1
#define MACPHY_ArrayLength 18
#define MACPHY_Array_PGLength 30
#define AGCTAB_ArrayLength 384
#elif defined(IMAGE_3_26)
#define BootArrayLength 344
#define MainArrayLength 38744
#define DataArrayLength 564
#define PHY_REGArrayLength 1
#define PHY_REG_1T2RArrayLength 296
#define RadioA_ArrayLength 246
#define RadioB_ArrayLength 78
#define RadioC_ArrayLength 1
#define RadioD_ArrayLength 1
#define MACPHY_ArrayLength 18
#define MACPHY_Array_PGLength 30
#define AGCTAB_ArrayLength 384
#else
#define BootArrayLength 344
#define MainArrayLength 47796
#define DataArrayLength 2848
#define PHY_REGArrayLength 296
#define PHY_REG_1T2RArrayLength 296
#define RadioA_ArrayLength 244
#define RadioB_ArrayLength 74
#define RadioC_ArrayLength 1
#define RadioD_ArrayLength 1
#define MACPHY_ArrayLength 18
#define MACPHY_Array_PGLength 18
#define AGCTAB_ArrayLength 384
#endif

#define BootArrayLengthDTM 344
#define MainArrayLengthDTM 50816
#define DataArrayLengthDTM 3256
#define PHY_REGArrayLengthDTM 278
#define radioA_ArrayLengthDTM 246
#define radioB_ArrayLengthDTM 72
#define radioC_ArrayLengthDTM 246
#define radioD_ArrayLengthDTM 72
#define MACPHY_ArrayLengthDTM 18
#define MACPHY_Array_PGLengthDTM 18
#define AGCTAB_ArrayLengthDTM 384

#define RTL8190_MAX_FIRMWARE_CODE_SIZE	64000	//64k

extern u4Byte Rtl8192UsbMACPHY_Array_PG[];
extern u4Byte	Rtl8192UsbMACPHY_Array[];
extern u4Byte	Rtl8192UsbAGCTAB_Array[];
extern u4Byte	Rtl8192UsbPHY_REGArray[];
extern u4Byte	Rtl8192UsbPHY_REG_1T2RArray[];
extern u4Byte	Rtl8192UsbRadioA_Array[];
extern u4Byte	Rtl8192UsbRadioB_Array[];
extern u4Byte	Rtl8192UsbRadioC_Array[];
extern u4Byte	Rtl8192UsbRadioD_Array[];

extern u4Byte Rtl8192UsbMACPHY_ArrayDTM[];
extern u4Byte Rtl8192UsbAGCTAB_ArrayDTM [];
extern u4Byte Rtl8192UsbPHY_REGArrayDTM[];
extern u4Byte Rtl8192UsbRadioA_ArrayDTM[];
extern u4Byte Rtl8192UsbRadioB_ArrayDTM[];
extern u4Byte Rtl8192UsbRadioC_ArrayDTM[];
extern u4Byte Rtl8192UsbRadioD_ArrayDTM[];



#define Rtl819XFwBootArray		Rtl8192UsbFwBootArray
#define Rtl819XFwMainArray		Rtl8192UsbFwMainArray
#define Rtl819XFwDataArray		Rtl8192UsbFwDataArray
#define Rtl819XMACPHY_Array_PG	Rtl8192UsbMACPHY_Array_PG
#define Rtl819XMACPHY_Array		Rtl8192UsbMACPHY_Array
#define Rtl819XAGCTAB_Array		Rtl8192UsbAGCTAB_Array
#define Rtl819XPHY_REGArray		Rtl8192UsbPHY_REGArray
#define Rtl819XPHY_REG_1T2RArray	Rtl8192UsbPHY_REG_1T2RArray
#define Rtl819XRadioA_Array		Rtl8192UsbRadioA_Array
#define Rtl819XRadioB_Array		Rtl8192UsbRadioB_Array
#define Rtl819XRadioC_Array		Rtl8192UsbRadioC_Array
#define Rtl819XRadioD_Array		Rtl8192UsbRadioD_Array

#define Rtl819XFwBootArrayDTM	Rtl8192UsbFwBootArrayDTM
#define Rtl819XFwMainArrayDTM	Rtl8192UsbFwMainArrayDTM
#define Rtl819XFwDataArrayDTM	Rtl8192UsbFwDataArrayDTM	
#define Rtl819XMACPHY_ArrayDTM	Rtl8192UsbMACPHY_ArrayDTM
#define Rtl819XAGCTAB_ArrayDTM	Rtl8192UsbAGCTAB_ArrayDTM 
#define Rtl819XPHY_REGArrayDTM	Rtl8192UsbPHY_REGArrayDTM
#define Rtl819XRadioA_ArrayDTM	Rtl8192UsbRadioA_ArrayDTM
#define Rtl819XRadioB_ArrayDTM	Rtl8192UsbRadioB_ArrayDTM
#define Rtl819XRadioC_ArrayDTM	Rtl8192UsbRadioC_ArrayDTM
#define Rtl819XRadioD_ArrayDTM	Rtl8192UsbRadioD_ArrayDTM

#define bXtalCap	0x0f000000

#define rOFDM0_LSTF               		0xc00
#define rOFDM0_TRxPathEnable      	0xc04
#define rOFDM0_TRMuxPar           		0xc08
#define rOFDM0_TRSWIsolation      		0xc0c
#define rOFDM0_XARxAFE            		0xc10  //RxIQ DC offset, Rx digital filter, DC notch filter
#define rOFDM0_XARxIQImbalance    	0xc14  //RxIQ imblance matrix
#define rOFDM0_XBRxAFE            		0xc18
#define rOFDM0_XBRxIQImbalance    	0xc1c
#define rOFDM0_XCRxAFE            		0xc20
#define rOFDM0_XCRxIQImbalance    	0xc24
#define rOFDM0_XDRxAFE            		0xc28
#define rOFDM0_XDRxIQImbalance    	0xc2c
#define rOFDM0_RxDetector1        		0xc30  //PD,BW & SBD
#define rOFDM0_RxDetector2        		0xc34  //SBD & Fame Sync.
#define rOFDM0_RxDetector3        		0xc38  //Frame Sync.
#define rOFDM0_RxDetector4        		0xc3c  //PD, SBD, Frame Sync & Short-GI
#define rOFDM0_RxDSP              		0xc40  //Rx Sync Path
#define rOFDM0_CFOandDAGC         	0xc44  //CFO & DAGC
#define rOFDM0_CCADropThreshold   	0xc48 //CCA Drop threshold
#define rOFDM0_ECCAThreshold      	0xc4c // energy CCA
#define rOFDM0_XAAGCCore1         	0xc50
#define rOFDM0_XAAGCCore2         	0xc54
#define rOFDM0_XBAGCCore1         	0xc58
#define rOFDM0_XBAGCCore2         	0xc5c
#define rOFDM0_XCAGCCore1         	0xc60
#define rOFDM0_XCAGCCore2         	0xc64
#define rOFDM0_XDAGCCore1         	0xc68
#define rOFDM0_XDAGCCore2         	0xc6c
#define rOFDM0_AGCParameter1      	0xc70
#define rOFDM0_AGCParameter2      	0xc74
#define rOFDM0_AGCRSSITable       	0xc78
#define rOFDM0_HTSTFAGC           		0xc7c
#define rOFDM0_XATxIQImbalance   	0xc80
#define rOFDM0_XATxAFE            		0xc84
#define rOFDM0_XBTxIQImbalance    	0xc88
#define rOFDM0_XBTxAFE            		0xc8c
#define rOFDM0_XCTxIQImbalance    	0xc90
#define rOFDM0_XCTxAFE            		0xc94
#define rOFDM0_XDTxIQImbalance    	0xc98
#define rOFDM0_XDTxAFE            		0xc9c
#define rOFDM0_RxHPParameter      	0xce0
#define rOFDM0_TxPseudoNoiseWgt   	0xce4
#define rOFDM0_FrameSync          		0xcf0
#define rOFDM0_DFSReport          		0xcf4
#define rOFDM0_TxCoeff1           		0xca4
#define rOFDM0_TxCoeff2           		0xca8
#define rOFDM0_TxCoeff3           		0xcac
#define rOFDM0_TxCoeff4           		0xcb0
#define rOFDM0_TxCoeff5           		0xcb4
#define rOFDM0_TxCoeff6           		0xcb8


#define rFPGA0_RFMOD              		0x800  //RF mode & CCK TxSC
#define rFPGA0_TxInfo             		0x804
#define rFPGA0_PSDFunction        		0x808
#define rFPGA0_TxGainStage        		0x80c
#define rFPGA0_RFTiming1          		0x810
#define rFPGA0_RFTiming2          		0x814

#define rFPGA0_XA_HSSIParameter1  	0x820
#define rFPGA0_XA_HSSIParameter2  	0x824
#define rFPGA0_XB_HSSIParameter1  	0x828
#define rFPGA0_XB_HSSIParameter2  	0x82c
#define rFPGA0_XC_HSSIParameter1  	0x830
#define rFPGA0_XC_HSSIParameter2  	0x834
#define rFPGA0_XD_HSSIParameter1  	0x838
#define rFPGA0_XD_HSSIParameter2  	0x83c
#define rFPGA0_XA_LSSIParameter   	0x840
#define rFPGA0_XB_LSSIParameter   	0x844
#define rFPGA0_XC_LSSIParameter   	0x848
#define rFPGA0_XD_LSSIParameter   	0x84c
#define rFPGA0_RFWakeUpParameter  	0x850
#define rFPGA0_RFSleepUpParameter 	0x854
#define rFPGA0_XAB_SwitchControl  	0x858
#define rFPGA0_XCD_SwitchControl  	0x85c
#define rFPGA0_XA_RFInterfaceOE   	0x860
#define rFPGA0_XB_RFInterfaceOE   	0x864
#define rFPGA0_XC_RFInterfaceOE   	0x868
#define rFPGA0_XD_RFInterfaceOE   	0x86c
#define rFPGA0_XAB_RFInterfaceSW  	0x870
#define rFPGA0_XCD_RFInterfaceSW  	0x874
#define rFPGA0_XAB_RFParameter    	0x878
#define rFPGA0_XCD_RFParameter    	0x87c
#define rFPGA0_AnalogParameter1   	0x880
#define rFPGA0_AnalogParameter2   	0x884
#define rFPGA0_AnalogParameter3   	0x888
#define rFPGA0_AnalogParameter4   	0x88c
#define rFPGA0_XA_LSSIReadBack    	0x8a0
#define rFPGA0_XB_LSSIReadBack    	0x8a4
#define rFPGA0_XC_LSSIReadBack    	0x8a8
#define rFPGA0_XD_LSSIReadBack    	0x8ac
#define rFPGA0_PSDReport          		0x8b4
#define rFPGA0_XAB_RFInterfaceRB  	0x8e0
#define rFPGA0_XCD_RFInterfaceRB  	0x8e4





#define rFPGA0_XA_LSSIReadBack    	0x8a0
#define rFPGA0_XB_LSSIReadBack    	0x8a4
#define rFPGA0_XC_LSSIReadBack    	0x8a8
#define rFPGA0_XD_LSSIReadBack    	0x8ac

//page 9
#define rFPGA1_RFMOD              		0x900  //RF mode & OFDM TxSC
#define rFPGA1_TxBlock            		0x904
#define rFPGA1_DebugSelect        		0x908
#define rFPGA1_TxInfo             		0x90c




#define BK_QUEUE						0
#define BE_QUEUE						1
#define VI_QUEUE						2
#define VO_QUEUE						3
#define HCCA_QUEUE					4
#define TXCMD_QUEUE					5
#define MGNT_QUEUE					6
#define HIGH_QUEUE					7

//
// Queue Select Value in TxDesc
//
#define QSLT_BK					0x1
#define QSLT_BE					0x0
#define QSLT_VI					0x4
#define QSLT_VO					0x6
#define	QSLT_BEACON			0x10
#define	QSLT_HIGH				0x11
#define	QSLT_MGNT				0x12
#define	QSLT_CMD				0x13


typedef enum _FIRMWARE_INIT_STEP{
	FW_INIT_STEP0_BOOT = 0,
	FW_INIT_STEP1_MAIN = 1,
	FW_INIT_STEP2_DATA = 2,
}FIRMWARE_INIT_STEP_E;

typedef struct _RT_FIRMWARE{
	FIRMWARE_STATUS	FirmwareStatus;
	u2Byte				CmdPacketFragThresold;
//	FIRMWARE_SOURCE	eFWSource;
	u1Byte				FwBootImg[16000];
	u4Byte				FwBootImgLen;
	u1Byte				FwMainImg[64000];
	u4Byte				FwMainImgLen;
	u1Byte				FwDataImg[16000];
	u4Byte				FwDataImgLen;
	u1Byte				szFwTmpBuffer[64000];
}RT_FIRMWARE, *PRT_FIRMWARE;

typedef struct _TX_DESC_CMD_819x_USB {
        //DWORD 0
	u2Byte	Reserved0;
	u1Byte	Reserved1;
	u1Byte	Reserved2:3;
	u1Byte	CmdInit:1;
	u1Byte	LastSeg:1;
	u1Byte	FirstSeg:1;
	u1Byte	LINIP:1;
	u1Byte	OWN:1;

        //DOWRD 1
	//u4Byte	Reserved3;
	u1Byte	TxFWInfoSize;
	u1Byte	Reserved3;
	u1Byte	QueueSelect;
	u1Byte	Reserved4;

        //DOWRD 2
	u2Byte	TxBufferSize;
	u2Byte	Reserved5;
	
       //DWORD 3,4,5
	//u4Byte	TxBufferAddr;
	//u4Byte	NextDescAddress;
	u4Byte	Reserved6;
	u4Byte	Reserved7;
	u4Byte	Reserved8;
}TX_DESC_CMD_819x_USB, *PTX_DESC_CMD_819x_USB;

typedef enum _DESC_PACKET_TYPE{
	DESC_PACKET_TYPE_INIT = 0,
	DESC_PACKET_TYPE_NORMAL = 1
}DESC_PACKET_TYPE;

typedef struct _TX_DESC_819x_USB {
	//DWORD 0
	u2Byte	PktSize;
	u1Byte	Offset;
	u1Byte	Reserved0:3;
	u1Byte	CmdInit:1;
	u1Byte	LastSeg:1;
	u1Byte	FirstSeg:1;
	u1Byte	LINIP:1;
	u1Byte	OWN:1;

	//DWORD 1
	u1Byte	TxFWInfoSize;
	u1Byte	RATid:3;
	u1Byte	DISFB:1;
	u1Byte	USERATE:1;
	u1Byte	MOREFRAG:1;
	u1Byte	NoEnc:1;
	u1Byte	PIFS:1;
	u1Byte	QueueSelect:5;
	u1Byte	NoACM:1;
	u1Byte	Reserved1:2;
	u1Byte	SecCAMID:5;
	u1Byte	SecDescAssign:1;
	u1Byte	SecType:2;

	//DWORD 2
	u2Byte	TxBufferSize;
	//u2Byte	Reserved2;
	u1Byte	ResvForPaddingLen:7;
	u1Byte	Reserved3:1;
	u1Byte	Reserved4;

	//DWORD 3
	//u4Byte	TxBufferAddr;

	//DWORD 4
	//u4Byte	NextDescAddress;

	//DWORD 5, 6, 7
	u4Byte	Reserved5;
	u4Byte	Reserved6;
	u4Byte	Reserved7;
	
}TX_DESC_819x_USB, *PTX_DESC_819x_USB;

typedef struct _TX_FWINFO_819xUSB{
	//DWORD 0
	u1Byte			TxRate:7;
	u1Byte			CtsEnable:1;
	u1Byte			RtsRate:7;
	u1Byte			RtsEnable:1;
	u1Byte			TxHT:1;
	u1Byte			Short:1;						//Short PLCP for CCK, or short GI for 11n MCS
	u1Byte			TxBandwidth:1;				// This is used for HT MCS rate only.
	u1Byte			TxSubCarrier:2;				// This is used for legacy OFDM rate only.
	u1Byte			STBC:2;
	u1Byte			AllowAggregation:1;
	u1Byte			RtsHT:1;						//Interpre RtsRate field as high throughput data rate
	u1Byte			RtsShort:1;					//Short PLCP for CCK, or short GI for 11n MCS
	u1Byte			RtsBandwidth:1;				// This is used for HT MCS rate only.
	u1Byte			RtsSubcarrier:2;				// This is used for legacy OFDM rate only.
	u1Byte			RtsSTBC:2;
	u1Byte			EnableCPUDur:1;				//Enable firmware to recalculate and assign packet duration
	
	//DWORD 1
	u4Byte			RxMF:2;
	u4Byte			RxAMD:3;
	u4Byte			Reserved1:3;
	u4Byte			TxAGCOffset:4;
	u4Byte			TxAGCSign:1;
	u4Byte			Tx_INFO_RSVD:6;
	u4Byte			PacketID:13;
}TX_FWINFO_819xUSB, *PTX_FWINFO_819xUSB;

#endif

