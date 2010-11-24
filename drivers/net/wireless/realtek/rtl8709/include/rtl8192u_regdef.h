#ifndef __RTL8192U_REGDEF_H__
#define __RTL8192U_REGDEF_H__

#include <drv_conf.h>
typedef enum _RF90_RADIO_PATH{
	RF90_PATH_A = 0,			//Radio Path A
	RF90_PATH_B = 1,			//Radio Path B
	RF90_PATH_C = 2,			//Radio Path C
	RF90_PATH_D = 3,			//Radio Path D
	RF90_PATH_MAX				//Max RF number 90 support 
}RF90_RADIO_PATH_E, *PRF90_RADIO_PATH_E;




typedef enum _HW90_BLOCK{
	HW90_BLOCK_MAC = 0,
	HW90_BLOCK_PHY0 = 1,
	HW90_BLOCK_PHY1 = 2,
	HW90_BLOCK_RF = 3,
	HW90_BLOCK_MAXIMUM = 4, // Never use this
}HW90_BLOCK_E, *PHW90_BLOCK_E;

typedef enum _BaseBand_Config_Type{
	BaseBand_Config_PHY_REG = 0,			//Radio Path A
	BaseBand_Config_AGC_TAB = 1,			//Radio Path B
}BaseBand_Config_Type, *PBaseBand_Config_Type;

/*--------------------------Define -------------------------------------------*/
#define	MGN_1M				0x02
#define	MGN_2M				0x04
#define	MGN_5_5M			0x0b
#define	MGN_11M			0x16

#define	MGN_6M				0x0c
#define	MGN_9M				0x12
#define	MGN_12M			0x18
#define	MGN_18M			0x24
#define	MGN_24M			0x30
#define	MGN_36M			0x48
#define	MGN_48M			0x60
#define	MGN_54M			0x6c

#define	MGN_MCS0			0x80
#define	MGN_MCS1			0x81
#define	MGN_MCS2			0x82
#define	MGN_MCS3			0x83
#define	MGN_MCS4			0x84
#define	MGN_MCS5			0x85
#define	MGN_MCS6			0x86
#define	MGN_MCS7			0x87
#define	MGN_MCS8			0x88
#define	MGN_MCS9			0x89
#define	MGN_MCS10			0x8a
#define	MGN_MCS11			0x8b
#define	MGN_MCS12			0x8c
#define	MGN_MCS13			0x8d
#define	MGN_MCS14			0x8e
#define	MGN_MCS15			0x8f

#define DESC90_RATE1M					0x00
#define DESC90_RATE2M					0x01
#define DESC90_RATE5_5M				0x02
#define DESC90_RATE11M					0x03
#define DESC90_RATE6M					0x04
#define DESC90_RATE9M					0x05
#define DESC90_RATE12M					0x06
#define DESC90_RATE18M					0x07
#define DESC90_RATE24M					0x08
#define DESC90_RATE36M					0x09
#define DESC90_RATE48M					0x0a
#define DESC90_RATE54M					0x0b
#define DESC90_RATEMCS0				0x00
#define DESC90_RATEMCS1				0x01
#define DESC90_RATEMCS2				0x02
#define DESC90_RATEMCS3				0x03
#define DESC90_RATEMCS4				0x04
#define DESC90_RATEMCS5				0x05
#define DESC90_RATEMCS6				0x06
#define DESC90_RATEMCS7				0x07
#define DESC90_RATEMCS8				0x08
#define DESC90_RATEMCS9				0x09
#define DESC90_RATEMCS10				0x0a
#define DESC90_RATEMCS11				0x0b
#define DESC90_RATEMCS12				0x0c
#define DESC90_RATEMCS13				0x0d
#define DESC90_RATEMCS14				0x0e
#define DESC90_RATEMCS15				0x0f
#define DESC90_RATEMCS32				0x20


//============================================================================
//       819xUsb Regsiter offset definition
//============================================================================
#define Cmd9346CR_9356SEL	0x00000010

#define bXtalCap01                			0xc0000000
#define bXtalCap23                			0x3

#define	CR9346_CFGRW		0xC0		// Config register write

#define Cmd9346CR			0xFE58	// 9346CR cmd register

// Page 8
#define bRFSI_RFENV			0x10

#define rGlobalCtrl			0x00

#define CR9346				0x0E

#define BSSIDR				0x02E	// BSSID Register

#define CMDR				0x037	// Command register
#define SIFS_CCK			0x03c
#define SIFS_OFDM			0x03e

#define TCR					0x040	// Transmit Configuration Register
#define RCR					0x044	// Receive Configuration Register
#define SLOT 					0x049
#define EIFS					0x04A
#define ACK_TIMEOUT			0x04C	// Ack Timeout Register

#define EDCAPARA_BE		0x050	// EDCA Parameter of AC BE
#define EDCAPARA_BK		0x054	// EDCA Parameter of AC BK
#define EDCAPARA_VO		0x058	// EDCA Parameter of AC VO
#define	EDCAPARA_VI			0x05C	// EDCA Parameter of AC VI

#define BCNTCFG				0x062

#define	BCNITV				0x070	// Beacon Interval (TU)
#define	ATIMWND				0x072	// ATIM Window Size (TU)
#define DRVERLYINT			0x074
#define BCNDMA				0x076	// Beacon DMA register (2 bytes), in unit of us.
#define	BCN_ERR_THRESH		0x078	// Beacon Error Threshold
#define AckTimeOutReg	0x79		// ACK timeout register, in unit of 4 us. 
  
#define	RWCAM				0x0A0	//IN 8190 Data Sheet is called CAMcmd
#define	WCAMI				0x0A4	// Software write CAM input content

#define	SECR				0x0B0	//Security Configuration Register

#define IMR					0xF4	// Interrupt Mask Register
#define ISR					0xF8	// Interrupt Status Register

#define	CPU_GEN				0x100	// CPU Reset Register

#define CPUINST				0x120	//CPU Instruction Register

#define ACMHWCONTROL		0x171	//	ACM Hardware Control Register

#define RQPN1				0x180	// Reserved Queue Page Number , Vo Vi, Be, Bk
#define RQPN2				0x184	// Reserved Queue Page Number, HCCA, Cmd, Mgnt, High
#define RQPN3				0x188	// Reserved Queue Page Number, Bcn, Public, 
#define	USB_RX_AGGR			0x1a8					// Set USB rx aggr control

#define RF_DATA				0x1d4	// FW will write RF data in the register.

#define MAR0				0x240
#define MAR4				0x244
	
#define	BW_OPMODE			0x300	// Bandwidth operation mode
#define	MSR					0x303	// Media Status register
#define	RETRY_LIMIT			0x304	// Retry Limit [15:8]-short, [7:0]-long
#define LBDLY				0x306
#define	RRSR				0x310	// Response Rate Set

#define	UFWP				0x318	// Update Firmware Polling Register
#define	RATR0				0x320	// Rate Adaptive Table register1

//----------------------------------------------------------------------------
//       8190 Response Rate Set Register	(offset 0x310, 4 byte)
//----------------------------------------------------------------------------

#define RRSR_RSC_OFFSET			21
#define RRSR_SHORT_OFFSET			23
#define RRSR_RSC_DUPLICATE			0x600000
#define RRSR_RSC_LOWSUBCHNL		0x400000
#define RRSR_RSC_UPSUBCHANL		0x200000
#define RRSR_SHORT					0x800000
#define RRSR_1M						BIT(0)
#define RRSR_2M						BIT(1) 
#define RRSR_5_5M					BIT(2) 
#define RRSR_11M					BIT(3) 
#define RRSR_6M						BIT(4) 
#define RRSR_9M						BIT(5) 
#define RRSR_12M					BIT(6) 
#define RRSR_18M					BIT(7) 
#define RRSR_24M					BIT(8) 
#define RRSR_36M					BIT(9) 
#define RRSR_48M					BIT(10) 
#define RRSR_54M					BIT(11)
#define RRSR_MCS0					BIT(12)
#define RRSR_MCS1					BIT(13)
#define RRSR_MCS2					BIT(14)
#define RRSR_MCS3					BIT(15)
#define RRSR_MCS4					BIT(16)
#define RRSR_MCS5					BIT(17)
#define RRSR_MCS6					BIT(18)
#define RRSR_MCS7					BIT(19)


// slot time for 11g
#define SHORT_SLOT_TIME			9
#define NON_SHORT_SLOT_TIME		20


// Bandwidth Offset
#define HAL_PRIME_CHNL_OFFSET_DONT_CARE	0
#define HAL_PRIME_CHNL_OFFSET_LOWER		1
#define HAL_PRIME_CHNL_OFFSET_UPPER		2


//
// Operation mode value
//
#define HT_OPMODE_NO_PROTECT		0
#define HT_OPMODE_OPTIONAL		1
#define HT_OPMODE_40MHZ_PROTECT	2
#define HT_OPMODE_MIXED			3


//
// MIMO Power Save Setings
//
#define MIMO_PS_STATIC				0
#define MIMO_PS_DYNAMIC			1
#define MIMO_PS_NOLIMIT			3

//----------------------------------------------------------------------------
//       8190 (CMD) command register bits		(Offset 0x37, 8 bit)
//----------------------------------------------------------------------------
#define 	CR_RST					0x10
#define 	CR_RE					0x08
#define 	CR_TE					0x04
#define 	CR_MulRW				0x01

//----------------------------------------------------------------------------
//       8190 (RCR) Receive Configuration Register	(Offset 0x44~47, 32 bit)
//----------------------------------------------------------------------------
#define RCR_ONLYERLPKT	BIT(31)				// Early Receiving based on Packet Size.
#define RCR_ENCS2		BIT(30)				// Enable Carrier Sense Detection Method 2
#define RCR_ENCS1		BIT(29)				// Enable Carrier Sense Detection Method 1
#define RCR_ENMBID		BIT(27)				// Enable Multiple BssId.
#define RCR_ACKTXBW		(BIT(24)|BIT(25))	// TXBW Setting of ACK frames
#define RCR_CBSSID		BIT(23)				// Accept BSSID match packet
#define RCR_APWRMGT		BIT(22)				// Accept power management packet
#define	RCR_ADD3		BIT(21)				// Accept address 3 match packet
#define RCR_AMF			BIT(20)				// Accept management type frame
#define RCR_ACF			BIT(19)				// Accept control type frame
#define RCR_ADF			BIT(18)				// Accept data type frame
#define RCR_RXFTH		BIT(13)				// Rx FIFO Threshold
#define RCR_AICV		BIT(12)				// Accept ICV error packet

#define RCR_9356SEL		BIT(6)

#define	RCR_ACRC32		BIT(5)				// Accept CRC32 error packet 
#define	RCR_AB			BIT(3)				// Accept broadcast packet 
#define	RCR_AM			BIT(2)				// Accept multicast packet 
#define	RCR_APM			BIT(1)				// Accept physical match packet
#define	RCR_AAP			BIT(0)				// Accept all unicast packet 

#define RCR_MXDMA_OFFSET	8
#define RCR_FIFO_OFFSET		13
#define RCR_OnlyErlPkt		BIT(31)				// Rx Early mode is performed for packet size greater than 1536

//----------------------------------------------------------------------------
//       8187B EDCAParam bits 					(Offset 0x074-0075, 16bit)
//----------------------------------------------------------------------------
#define AC_PARAM_TXOP_LIMIT_OFFSET		16
#define AC_PARAM_ECW_MAX_OFFSET			12
#define AC_PARAM_ECW_MIN_OFFSET			8
#define AC_PARAM_AIFS_OFFSET			0

//----------------------------------------------------------------------------
//       8190 DrvErlyInt bits					(Offset 0x074-0075, 16bit)
//----------------------------------------------------------------------------

#define	MSR_NOLINK				0x00
#define	MSR_ADHOC				0x01
#define	MSR_INFRA				0x02
#define	MSR_AP					0x03

#define	MSR_ENEDCA


//----------------------------------------------------------------------------
//       8190 CPU General Register		(offset 0x100, 4 byte)
//----------------------------------------------------------------------------

#define 	CPU_CCK_LOOPBACK			0x00030000
#define 	CPU_GEN_SYSTEM_RESET		0x00000001
#define 	CPU_GEN_FIRMWARE_RESET	0x00000008
#define 	CPU_GEN_BOOT_RDY			0x00000010
#define 	CPU_GEN_FIRM_RDY			0x00000020
#define 	CPU_GEN_PUT_CODE_OK		0x00000080
#define 	CPU_GEN_BB_RST			0x00000100
#define 	CPU_GEN_PWR_STB_CPU		0x00000004
#define 	CPU_GEN_NO_LOOPBACK_MSK	0xFFF8FFFF		// Set bit18,17,16 to 0. Set bit19
#define	CPU_GEN_NO_LOOPBACK_SET	0x00080000		// Set BIT19 to 1


#define VOQ_ACM_EN				(0x01 << 7) //BIT7
#define VIQ_ACM_EN				(0x01 << 6) //BIT6
#define BEQ_ACM_EN				(0x01 << 5) //BIT5
#define ACM_HW_EN				(0x01 << 4) //BIT4
#define TXOPSEL					(0x01 << 3) //BIT3
#define VOQ_ACM_CTL				(0x01 << 2) //BIT2 // Set to 1 when AC_VO used time reaches or exceeds the admitted time
#define VIQ_ACM_CTL				(0x01 << 1) //BIT1 // Set to 1 when AC_VI used time reaches or exceeds the admitted time
#define BEQ_ACM_CTL				(0x01 << 0) //BIT0 // Set to 1 when AC_BE used time reaches or exceeds the admitted time

//----------------------------------------------------------------------------
//       8190 AcmHwCtrl bits 					(offset 0x171, 1 byte)
//----------------------------------------------------------------------------
#define	AcmHw_HwEn			BIT(0)
#define	AcmHw_BeqEn			BIT(1)
#define	AcmHw_ViqEn			BIT(2)
#define	AcmHw_VoqEn			BIT(3)
#define	AcmHw_BeqStatus		BIT(4)
#define	AcmHw_ViqStatus		BIT(5)
#define	AcmHw_VoqStatus		BIT(6)

//----------------------------------------------------------------------------
//       8190 BW_OPMODE bits				(Offset 0x300, 8bit)
//----------------------------------------------------------------------------
#define	BW_OPMODE_11J		BIT(0)
#define	BW_OPMODE_5G		BIT(1)
#define	BW_OPMODE_20MHZ	BIT(2)

//----------------------------------------------------------------------------
//       8190 Retry Limit					(Offset 0x304-0305, 16bit)
//----------------------------------------------------------------------------
#define	SRL_SHIFT	8
#define	LRL_SHIFT	0

//----------------------------------------------------------------------------
//       8190 Response Rate Set Register	(offset 0x310, 4 byte)
//----------------------------------------------------------------------------
#define RRSR_1M				BIT(0)
#define RRSR_2M				BIT(1) 
#define RRSR_5_5M			BIT(2) 
#define RRSR_11M				BIT(3) 
#define RRSR_6M				BIT(4) 
#define RRSR_9M				BIT(5) 
#define RRSR_12M				BIT(6) 
#define RRSR_18M				BIT(7) 
#define RRSR_24M				BIT(8) 
#define RRSR_36M				BIT(9) 
#define RRSR_48M				BIT(10)
#define RRSR_54M				BIT(11)




#define rTxAGC_Rate18_06			0xe00
#define rTxAGC_Rate54_24			0xe04
#define rTxAGC_CCK_Mcs32			0xe08
#define rTxAGC_Mcs03_Mcs00			0xe10
#define rTxAGC_Mcs07_Mcs04			0xe14
#define rTxAGC_Mcs11_Mcs08			0xe18
#define rTxAGC_Mcs15_Mcs12			0xe1c

#define bTxAGCRate18_06			0x7f7f7f7f
#define bTxAGCRate54_24			0x7f7f7f7f
#define bTxAGCRateMCS32		0x7f
#define bTxAGCRateCCK			0x7f00
#define bTxAGCRateMCS3_MCS0	0x7f7f7f7f
#define bTxAGCRateMCS7_MCS4	0x7f7f7f7f
#define bTxAGCRateMCS11_MCS8	0x7f7f7f7f
#define bTxAGCRateMCS15_MCS12	0x7f7f7f7f

#define bZebra1_ChannelNum        0xf80
#define rZebra1_Channel               		0x7

#define bMask12Bits               0xfff
#define bMaskDWord                0xffffffff
#define 	QPNR					0x1d0					// Queue Packet Number report per TID

#define bLSSIReadBackData         		0xfff
#define bLSSIReadAddress          		0x3f000000   //LSSI "Read" Address
#define bLSSIReadEdge             		0x80000000   //LSSI "Read" edge signal
#define BB_GLOBAL_RESET		0x020					// BasebandGlobal Reset Register
//----------------------------------------------------------------------------
//       8190 BB_GLOBAL_RESET	 bits			(Offset 0x020-0027, 8bit)
//----------------------------------------------------------------------------
#define	BB_GLOBAL_RESET_BIT	0x1


//page-8
#define bRFMOD                    			0x1
#define bJapanMode                		0x2
#define bCCKTxSC                  			0x30
#define bCCKEn                    			0x1000000
#define bOFDMEn                   			0x2000000
#define bOFDMRxADCPhase           		0x10000
#define bOFDMTxDACPhase           		0x40000
#define bXATxAGC                  			0x3f
#define bXBTxAGC                  			0xf00
#define bXCTxAGC                  			0xf000
#define bXDTxAGC                  			0xf0000
#define bPAStart                  			0xf0000000
#define bTRStart                  			0x00f00000
#define bRFStart                  			0x0000f000
#define bBBStart                  			0x000000f0
#define bBBCCKStart               		0x0000000f
#define bPAEnd                    			0xf          //Reg0x814
#define bTREnd                    			0x0f000000
#define bRFEnd                    			0x000f0000
#define bCCAMask                  			0x000000f0   //T2R
#define bR2RCCAMask               		0x00000f00
#define bHSSI_R2TDelay            		0xf8000000
#define bHSSI_T2RDelay            		0xf80000
#define bContTxHSSI               		0x400     //chane gain at continue Tx
#define bIGFromCCK                		0x200
#define bAGCAddress               		0x3f
#define bRxHPTx                   			0x7000
#define bRxHPT2R                  			0x38000
#define bRxHPCCKIni               		0xc0000
#define bAGCTxCode                		0xc00000
#define bAGCRxCode                		0x300000
#define b3WireDataLength          		0x800
#define b3WireAddressLength       		0x400
#define b3WireRFPowerDown         		0x1
//#define bHWSISelect               		0x8
#define b5GPAPEPolarity           		0x40000000
#define b2GPAPEPolarity           		0x80000000
#define bRFSW_TxDefaultAnt        		0x3
#define bRFSW_TxOptionAnt         		0x30
#define bRFSW_RxDefaultAnt        		0x300
#define bRFSW_RxOptionAnt         		0x3000
#define bRFSI_3WireData           		0x1
#define bRFSI_3WireClock          		0x2
#define bRFSI_3WireLoad           		0x4
#define bRFSI_3WireRW             		0x8
#define bRFSI_3Wire               			0xf  //3-wire total control
#define bRFSI_RFENV               		0x10
#define bRFSI_TRSW                		0x20
#define bRFSI_TRSWB               		0x40
#define bRFSI_ANTSW               		0x100
#define bRFSI_ANTSWB              		0x200
#define bRFSI_PAPE                			0x400
#define bRFSI_PAPE5G              		0x800 
#define bBandSelect               			0x1
#define bHTSIG2_GI                			0x80
#define bHTSIG2_Smoothing         		0x01
#define bHTSIG2_Sounding          		0x02
#define bHTSIG2_Aggreaton         		0x08
#define bHTSIG2_STBC              		0x30
#define bHTSIG2_AdvCoding         		0x40
#define bHTSIG2_NumOfHTLTF        	0x300
#define bHTSIG2_CRC8              		0x3fc
#define bHTSIG1_MCS               		0x7f
#define bHTSIG1_BandWidth         		0x80
#define bHTSIG1_HTLength          		0xffff
#define bLSIG_Rate                			0xf
#define bLSIG_Reserved            		0x10
#define bLSIG_Length              		0x1fffe
#define bLSIG_Parity              			0x20
#define bCCKRxPhase               		0x4
#define bLSSIReadAddress          		0x3f000000   //LSSI "Read" Address
#define bLSSIReadEdge             		0x80000000   //LSSI "Read" edge signal
#define bLSSIReadBackData         		0xfff
#define bLSSIReadOKFlag           		0x1000
#define bCCKSampleRate            		0x8       //0: 44MHz, 1:88MHz

#define bRegulator0Standby        		0x1
#define bRegulatorPLLStandby      		0x2
#define bRegulator1Standby        		0x4
#define bPLLPowerUp               		0x8
#define bDPLLPowerUp              		0x10
#define bDA10PowerUp              		0x20
#define bAD7PowerUp               		0x200
#define bDA6PowerUp               		0x2000
#define bXtalPowerUp              		0x4000
#define b40MDClkPowerUP           		0x8000
#define bDA6DebugMode             		0x20000
#define bDA6Swing                 			0x380000
#define bADClkPhase               		0x4000000
#define b80MClkDelay              		0x18000000
#define bAFEWatchDogEnable        		0x20000000
#define bXtalCap                			0x0f000000
#define bIntDifClkEnable          		0x400
#define bExtSigClkEnable         	 	0x800
#define bBandgapMbiasPowerUp      	0x10000
#define bAD11SHGain               		0xc0000
#define bAD11InputRange           		0x700000
#define bAD11OPCurrent            		0x3800000
#define bIPathLoopback            		0x4000000
#define bQPathLoopback            		0x8000000
#define bAFELoopback              		0x10000000
#define bDA10Swing                		0x7e0
#define bDA10Reverse              		0x800
#define bDAClkSource              		0x1000
#define bAD7InputRange            		0x6000
#define bAD7Gain                  			0x38000
#define bAD7OutputCMMode          		0x40000
#define bAD7InputCMMode           		0x380000
#define bAD7Current               			0xc00000
#define bRegulatorAdjust          		0x7000000
#define bAD11PowerUpAtTx          		0x1
#define bDA10PSAtTx               		0x10
#define bAD11PowerUpAtRx          		0x100
#define bDA10PSAtRx               		0x1000

#define bCCKRxAGCFormat           		0x200

#define bPSDFFTSamplepPoint       		0xc000
#define bPSDAverageNum            		0x3000
#define bIQPathControl            		0xc00
#define bPSDFreq                  			0x3ff
#define bPSDAntennaPath           		0x30
#define bPSDIQSwitch              		0x40
#define bPSDRxTrigger             		0x400000
#define bPSDTxTrigger             		0x80000000
#define bPSDSineToneScale        		0x7f000000
#define bPSDReport                			0xffff



//for PutRegsetting & GetRegSetting BitMask
#define bMaskByte0                0xff
#define bMaskByte1                0xff00
#define bMaskByte2                0xff0000
#define bMaskByte3                0xff000000
#define bMaskHWord                0xffff0000
#define bMaskLWord                0x0000ffff
#define bMaskDWord                0xffffffff


//page a
#define rCCK0_System              		0xa00
#define rCCK0_AFESetting          		0xa04
#define rCCK0_CCA                 			0xa08
#define rCCK0_RxAGC1              		0xa0c  //AGC default value, saturation level
#define rCCK0_RxAGC2              		0xa10  //AGC & DAGC
#define rCCK0_RxHP                		0xa14
#define rCCK0_DSPParameter1       	0xa18  //Timing recovery & Channel estimation threshold
#define rCCK0_DSPParameter2       	0xa1c  //SQ threshold
#define rCCK0_TxFilter1           		0xa20
#define rCCK0_TxFilter2           		0xa24
#define rCCK0_DebugPort           		0xa28  //debug port and Tx filter3
#define rCCK0_FalseAlarmReport    	0xa2c  //0xa2d
#define rCCK0_TRSSIReport         		0xa50
#define rCCK0_RxReport            		0xa54  //0xa57
#define rCCK0_FACounterLower      	0xa5c  //0xa5b
#define rCCK0_FACounterUpper      	0xa58  //0xa5c



//page-a
#define bCCKBBMode                		0x3
#define bCCKTxPowerSaving         		0x80
#define bCCKRxPowerSaving         		0x40
#define bCCKSideBand              		0x10
#define bCCKScramble              		0x8
#define bCCKAntDiversity    		      	0x8000
#define bCCKCarrierRecovery   	    	0x4000
#define bCCKTxRate           		     	0x3000
#define bCCKDCCancel             	 	0x0800
#define bCCKISICancel             		0x0400
#define bCCKMatchFilter           		0x0200
#define bCCKEqualizer             		0x0100
#define bCCKPreambleDetect       	 	0x800000
#define bCCKFastFalseCCA          		0x400000
#define bCCKChEstStart            		0x300000
#define bCCKCCACount              		0x080000
#define bCCKcs_lim                			0x070000
#define bCCKBistMode              		0x80000000
#define bCCKCCAMask             	  	0x40000000
#define bCCKTxDACPhase         	   	0x4
#define bCCKRxADCPhase         	   	0x20000000   //r_rx_clk
#define bCCKr_cp_mode0         	   	0x0100
#define bCCKTxDCOffset           	 	0xf0
#define bCCKRxDCOffset           	 	0xf
#define bCCKCCAMode              	 	0xc000
#define bCCKFalseCS_lim           		0x3f00
#define bCCKCS_ratio              		0xc00000
#define bCCKCorgBit_sel           		0x300000
#define bCCKPD_lim                			0x0f0000
#define bCCKNewCCA                		0x80000000
#define bCCKRxHPofIG              		0x8000
#define bCCKRxIG                  			0x7f00
#define bCCKLNAPolarity           		0x800000
#define bCCKRx1stGain             		0x7f0000
#define bCCKRFExtend              		0x20000000 //CCK Rx Iinital gain polarity
#define bCCKRxAGCSatLevel        	 	0x1f000000
#define bCCKRxAGCSatCount       	  	0xe0
#define bCCKRxRFSettle            		0x1f       //AGCsamp_dly
#define bCCKFixedRxAGC           	 	0x8000
//#define bCCKRxAGCFormat         	 	0x4000   //remove to HSSI register 0x824
#define bCCKAntennaPolarity      	 	0x2000
#define bCCKTxFilterType          		0x0c00
#define bCCKRxAGCReportType   	   	0x0300
#define bCCKRxDAGCEn              		0x80000000
#define bCCKRxDAGCPeriod        	  	0x20000000
#define bCCKRxDAGCSatLevel     	   	0x1f000000
#define bCCKTimingRecovery       	 	0x800000
#define bCCKTxC0                  			0x3f0000
#define bCCKTxC1                  			0x3f000000
#define bCCKTxC2                  			0x3f
#define bCCKTxC3                  			0x3f00
#define bCCKTxC4                  			0x3f0000
#define bCCKTxC5                  			0x3f000000
#define bCCKTxC6                  			0x3f
#define bCCKTxC7                  			0x3f00
#define bCCKDebugPort             		0xff0000
#define bCCKDACDebug              		0x0f000000
#define bCCKFalseAlarmEnable      		0x8000
#define bCCKFalseAlarmRead        		0x4000
#define bCCKTRSSI                 			0x7f
#define bCCKRxAGCReport           		0xfe
#define bCCKRxReport_AntSel       		0x80000000
#define bCCKRxReport_MFOff        		0x40000000
#define bCCKRxRxReport_SQLoss     	0x20000000
#define bCCKRxReport_Pktloss      		0x10000000
#define bCCKRxReport_Lockedbit    	0x08000000
#define bCCKRxReport_RateError    	0x04000000
#define bCCKRxReport_RxRate       		0x03000000
#define bCCKRxFACounterLower      	0xff
#define bCCKRxFACounterUpper      	0xff000000
#define bCCKRxHPAGCStart          		0xe000
#define bCCKRxHPAGCFinal          		0x1c00

#define bCCKRxFalseAlarmEnable    	0x8000
#define bCCKFACounterFreeze       		0x4000

#define bCCKTxPathSel             		0x10000000
#define bCCKDefaultRxPath         		0xc000000
#define bCCKOptionRxPath          		0x3000000


//page d
#define rOFDM1_LSTF               		0xd00
#define rOFDM1_TRxPathEnable      	0xd04
#define rOFDM1_CFO                		0xd08
#define rOFDM1_CSI1               		0xd10
#define rOFDM1_SBD                		0xd14
#define rOFDM1_CSI2               		0xd18
#define rOFDM1_CFOTracking        		0xd2c
#define rOFDM1_TRxMesaure1        	0xd34
#define rOFDM1_IntfDet            		0xd3c
#define rOFDM1_PseudoNoiseStateAB 0xd50
#define rOFDM1_PseudoNoiseStateCD 0xd54
#define rOFDM1_RxPseudoNoiseWgt   0xd58
#define rOFDM_PHYCounter1         		0xda0  //cca, parity fail
#define rOFDM_PHYCounter2         		0xda4  //rate illegal, crc8 fail
#define rOFDM_PHYCounter3         		0xda8  //MCS not support
#define rOFDM_ShortCFOAB          		0xdac
#define rOFDM_ShortCFOCD          		0xdb0
#define rOFDM_LongCFOAB           		0xdb4
#define rOFDM_LongCFOCD           		0xdb8
#define rOFDM_TailCFOAB           		0xdbc
#define rOFDM_TailCFOCD           		0xdc0
#define rOFDM_PWMeasure1          	0xdc4
#define rOFDM_PWMeasure2          	0xdc8
#define rOFDM_BWReport            		0xdcc
#define rOFDM_AGCReport           		0xdd0
#define rOFDM_RxSNR               		0xdd4
#define rOFDM_RxEVMCSI            		0xdd8
#define rOFDM_SIGReport           		0xddc



//----------------------------------------------------------------------------
//       8190 Rate Adaptive Table Register	(offset 0x320, 4 byte)
//----------------------------------------------------------------------------
//	CCK
#define	RATR_1M		0x00000001
#define	RATR_2M		0x00000002
#define	RATR_55M	0x00000004
#define	RATR_11M	0x00000008
//	OFDM
#define	RATR_6M		0x00000010
#define	RATR_9M		0x00000020
#define	RATR_12M	0x00000040
#define	RATR_18M	0x00000080
#define	RATR_24M	0x00000100
#define	RATR_36M	0x00000200
#define	RATR_48M	0x00000400
#define	RATR_54M	0x00000800
//	MCS 1 Spatial Stream	
#define	RATR_MCS0	0x00001000
#define	RATR_MCS1	0x00002000
#define	RATR_MCS2	0x00004000
#define	RATR_MCS3	0x00008000
#define	RATR_MCS4	0x00010000
#define	RATR_MCS5	0x00020000
#define	RATR_MCS6	0x00040000
#define	RATR_MCS7	0x00080000
//	MCS 2 Spatial Stream
#define	RATR_MCS8	0x00100000
#define	RATR_MCS9	0x00200000
#define	RATR_MCS10	0x00400000
#define	RATR_MCS11	0x00800000
#define	RATR_MCS12	0x01000000
#define	RATR_MCS13	0x02000000
#define	RATR_MCS14	0x04000000
#define	RATR_MCS15	0x08000000
//	ALL CCK Rate
#define RATE_ALL_CCK				RATR_1M|RATR_2M|RATR_55M|RATR_11M 
#define RATE_ALL_OFDM_AG			RATR_6M|RATR_9M|RATR_12M|RATR_18M|RATR_24M\
									|RATR_36M|RATR_48M|RATR_54M	
#define RATE_ALL_OFDM_1SS			RATR_MCS0|RATR_MCS1|RATR_MCS2|RATR_MCS3 | \
									RATR_MCS4|RATR_MCS5|RATR_MCS6	|RATR_MCS7	
#define RATE_ALL_OFDM_2SS			RATR_MCS8|RATR_MCS9	|RATR_MCS10|RATR_MCS11| \
									RATR_MCS12|RATR_MCS13|RATR_MCS14|RATR_MCS15
									
#endif
