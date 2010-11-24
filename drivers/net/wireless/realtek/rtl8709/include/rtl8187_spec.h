	


#ifndef __RTL8187_SPEC_H__
#define __RTL8187_SPEC_H__

#ifdef CONFIG_RTL8187
#include <drv_conf.h>

#define Virtual2Physical(x)		(((int)x) & 0x1fffffff)
#define Physical2Virtual(x)		(((int)x) | 0x80000000)
#define Virtual2NonCache(x)		(((int)x) | 0x20000000)
#define Physical2NonCache(x)		(((int)x) | 0xa0000000)

#define rtl_inb(offset) ( )
#define rtl_inw(offset) ( )
#define rtl_inl(offset) ( )

#define rtl_outb(offset,val)	( )
#define rtl_outw(offset,val)	( )
#define rtl_outl(offset,val)	( )

#ifndef BIT
#define BIT(x)	( 1 << (x))
#endif

#define SUCCESS	0
#define FAIL	(-1)

#define VO_QUEUE_INX	0
#define VI_QUEUE_INX	1
#define BE_QUEUE_INX	2
#define BK_QUEUE_INX	3
#define TS_QUEUE_INX	4
#define MGT_QUEUE_INX	5
#define BMC_QUEUE_INX	6
#define BCN_QUEUE_INX	7

#if defined(CONFIG_RTL8187) || defined(CONFIG_RTL8192U)
#define MGNT_QUEUE_INX		4
#define BEACON_QUEUE_INX 	5
#endif

#ifdef CONFIG_RTL8192U
#define TXCMD_QUEUE_INX 6
#endif

#define HW_QUEUE_ENTRY 8

//frame_tag definition 
#define TAG_XMITFRAME 0
#define TAG_MGNTFRAME 1

#define _A_BAND		BIT(1)

//----------------------------------------------------------------------------
//       8187B AC_XX_PARAM bits 
//----------------------------------------------------------------------------
#define AC_PARAM_TXOP_LIMIT_OFFSET		16
#define AC_PARAM_ECW_MAX_OFFSET			12
#define AC_PARAM_ECW_MIN_OFFSET			8
#define AC_PARAM_AIFS_OFFSET			0

//----------------------------------------------------------------------------
//       8187B ACM_CONTROL bits						(Offset 0xBF, 1 Byte)
//----------------------------------------------------------------------------
#define VOQ_ACM_EN				(0x01 << 7) //BIT7
#define VIQ_ACM_EN				(0x01 << 6) //BIT6
#define BEQ_ACM_EN				(0x01 << 5) //BIT5
#define ACM_HW_EN				(0x01 << 4) //BIT4
#define TXOPSEL					(0x01 << 3) //BIT3
#define VOQ_ACM_CTL				(0x01 << 2) //BIT2 // Set to 1 when AC_VO used time reaches or exceeds the admitted time
#define VIQ_ACM_CTL				(0x01 << 1) //BIT1 // Set to 1 when AC_VI used time reaches or exceeds the admitted time
#define BEQ_ACM_CTL				(0x01 << 0) //BIT0 // Set to 1 when AC_BE used time reaches or exceeds the admitted time


/* 
 * Operational registers offsets in PCI (I/O) space. 
 * RealTek names are used.
 */

#define IDR0 0x0000
#define IDR1 0x0001
#define IDR2 0x0002
#define IDR3 0x0003
#define IDR4 0x0004
#define IDR5 0x0005

/* 0x0006 - 0x0007 - reserved */

#define MAR0 0x0008
#define MAR1 0x0009
#define MAR2 0x000A
#define MAR3 0x000B
#define MAR4 0x000C
#define MAR5 0x000D
#define MAR6 0x000E
#define MAR7 0x000F

/* 0x0010 - 0x0017 - reserved */

#define TSFTR 0x0018
#define TSFTR_END 0x001F

#define TLPDA 0x0020
#define TLPDA_END 0x0023
#define TNPDA 0x0024
#define TNPDA_END 0x0027
#define THPDA 0x0028
#define THPDA_END 0x002B

#define BRSR_8187 0x002C
#define BRSR_8187_END 0x002D
#define BRSR_8187B 0x0034
#define BRSR_8187B_END 0x0035

#define BSSID_87B 0x002E
#define BSSID_END 0x0033

/* 0x0034 - 0x0034 - reserved */

/* 0x0038 - 0x003B - reserved */

#define IMR 0x003C
#define IMR_END 0x003D

#define ISR 0x003E
#define ISR_END 0x003F

#define TCR 0x0040
#define TCR_END 0x0043

#define RCR 0x0044
#define RCR_END 0x0047

#define TimerInt 0x0048
#define TimerInt_END 0x004B

#define TBDA 0x004C
#define TBDA_END 0x004F

#define CR9346 0x0050

#define CONFIG0 0x0051
#define CONFIG1 0x0052
#define CONFIG2 0x0053

#define ANA_PARAM 0x0054
#define ANA_PARAM_END 0x0x0057

#define MSR 0x0058

#define CONFIG3 0x0059

#define CONFIG3_GNTSel     ((1<<7))
#define CONFIG3_PARM_En    ((1<<6))
#define CONFIG3_Magic      ((1<<5))
#define CONFIG3_CardB_En   ((1<<3))
#define CONFIG3_CLKRUN_En  ((1<<2))
#define CONFIG3_FuncRegEn  ((1<<1))
#define CONFIG3_FBtbEn     ((1<<0))

#define CONFIG4 0x005A

#define TESTR 0x005B

/* 0x005C - 0x005D - reserved */
#define TFPC_AC  0x005C
#define PSR 0x005E

#define SCR 0x005F

/* 0x0060 - 0x006F - reserved */
#define ANA_PARAM2 0x0060
#define ANA_PARAM2_END 0x0063

//----------------------------------------------------------------------------
//       818xB AnaParm & AnaParm2 Register
//----------------------------------------------------------------------------
/*
#ifdef RTL8185B_FPGA
#define ANAPARM_FPGA_ON    0xa0000b59
//#define ANAPARM_FPGA_OFF    
#define ANAPARM2_FPGA_ON  0x860dec11
//#define ANAPARM2_FPGA_OFF
#else //ASIC
*/
#define ANAPARM_ASIC_ON    0x45090658
//#define ANAPARM_ASIC_OFF
#define ANAPARM2_ASIC_ON  0x727f3f52
//#define ANAPARM2_ASIC_OFF
//#endif

#define BcnIntv 0x0070
#define BcnItv_END 0x0071

#define AtimWnd 0x0072
#define AtimWnd_END 0x0073

#define BcnDma 0x0074
#define BcnDma_END 0x0075

#define BcnInt 0xEA		//Beacon Interrupt Register, in unit of tu.

#define AtimtrItv 0x0076
#define AtimtrItv_END 0x0077

#define PhyDelay 0x0078

//#define CRCount 0x0079

#define AckTimeOutReg      0x79            // ACK timeout register, in unit of 4 us.
/* 0x007A - 0x007B - reserved */

#define PhyAddr 0x007C
#define PhyDataW 0x007D
#define PhyDataR 0x007E
#define RF_Ready 0x007F

#define PhyCFG 0x0080
#define PhyCFG_END 0x0083

/* following are for rtl8185 */
#define RFPinsOutput 0x80
#define RFPinsEnable 0x82
#define RF_TIMING 0x8c 
#define RFPinsSelect 0x84
#define ANAPARAM2 0x60
#define RF_PARA 0x88
#define RFPinsInput 0x86
#define GP_ENABLE 0x90
#define GPIO 0x91
#define HSSI_PARA	0x94			// HSS Parameter
#define SW_CONTROL_GPIO 0x400
#define CCK_TXAGC 0x9d
#define OFDM_TXAGC 0x9e
#define ANTSEL 0x9f
#define TXAGC_CTL_PER_PACKET_ANT_SEL 0x02
#define WPA_CONFIG 0xb0
#define TX_AGC_CTL 0x9c
#define TX_AGC_CTL_PER_PACKET_TXAGC	0x01
#define TX_AGC_CTL_PERPACKET_GAIN_SHIFT 0
#define TX_AGC_CTL_PERPACKET_ANTSEL_SHIFT 1
#define TX_AGC_CTL_FEEDBACK_ANT 2
#define RESP_RATE 0x34
#define SIFS 0xb4
#define DIFS 0xb5
#define EIFS_8187  0x35
#define EIFS_8187B 0x2D
#define SLOT 0xb6
#define CW_VAL 0xbd
#define CW_CONF 0xbc
#define CW_CONF_PERPACKET_RETRY_LIMIT 0x02
#define CW_CONF_PERPACKET_CW 0x01
#define CW_CONF_PERPACKET_RETRY_SHIFT 1
#define CW_CONF_PERPACKET_CW_SHIFT 0
#define MAX_RESP_RATE_SHIFT 4
#define MIN_RESP_RATE_SHIFT 0
#define RATE_FALLBACK 0xbe
#define RATE_FALLBACK_CTL_ENABLE  0x80
#define RATE_FALLBACK_CTL_AUTO_STEP0 0x00

#define CCK_FALSE_ALARM 0xd0

/* 0x00D4 - 0x00D7 - reserved */

#define CONFIG5 0x00D8

#define TPPoll 0x00D9

/* 0x00DA - 0x00DB - reserved */

#define CWR 0x00DC
#define CWR_END 0x00DD

#define RetryCTR 0x00DE

/* 0x00DF - 0x00E3 - reserved */

#define RDSAR 0x00E4
#define RDSAR_END 0x00E7

/* 0x00E8 - 0x00EF - reserved */
#define ANA_PARAM3 0x00EE

#define FER 0x00F0
#define FER_END 0x00F3

#define FEMR	0x1D4	// Function Event Mask register (0xf4 ~ 0xf7)
//#define FEMR 0x00F4
#define FEMR_END 0x00F7

#define FPSR 0x00F8
#define FPSR_END 0x00FB

#define FFER 0x00FC
#define FFER_END 0x00FF

/*
 *  0x0000 - 0x00ff is selected to page 0 when PSEn bit (bit0, PSR) 
 *  is set to 2
 */
#define RFSW_CTRL                  0x272   // 0x272-0x273.
/* 0x00E8 - 0x00EF - reserved */
#define ANA_PARAM3 0x00EE

#define ARFR			0x1E0	// Auto Rate Fallback Register (0x1e0 ~ 0x1e2)
#define RMS			0x1EC	// Rx Max Pacetk Size (0x1ec[0:12])

/*
 *  0x0084 - 0x00D3 is selected to page 1 when PSEn bit (bit0, PSR) 
 *  is set to 1
 */
 
#define TCR_DurProcMode  ((1<<30))
#define TCR_DISReqQsize  ((1<<28))
#define TCR_HWVERID_MASK ((1<<27)|(1<<26)|(1<<25))
#define TCR_HWVERID_SHIFT 25
#define TCR_SWPLCPLEN     ((1<<24))
#define TCR_PLCP_LEN TCR_SAT // rtl8180
#define TCR_MXDMA_MASK   ((1<<23)|(1<<22)|(1<<21)) 
#define TCR_MXDMA_1024 6
#define TCR_MXDMA_2048 7
#define TCR_MXDMA_SHIFT  21
#define TCR_DISCW   ((1<<20))
#define TCR_ICV     ((1<<19))
#define TCR_LBK     ((1<<18)|(1<<17))
#define TCR_LBK1    ((1<<18))
#define TCR_LBK0    ((1<<17))
#define TCR_CRC     ((1<<16))
#define TCR_SRL_MASK   ((1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9)|(1<<8))
#define TCR_LRL_MASK   ((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7))
#define TCR_PROBE_NOTIMESTAMP_SHIFT 29 //rtl8185

#define RCR_ONLYERLPKT ((1<<31))
#define RCR_CS_SHIFT   29
#define RCR_CS_MASK    ((1<<30) | (1<<29))
#define RCR_ENMARP     ((1<<28))
#define RCR_CBSSID     ((1<<23))
#define RCR_APWRMGT    ((1<<22))
#define RCR_ADD3       ((1<<21))
#define RCR_AMF        ((1<<20))
#define RCR_ACF        ((1<<19))
#define RCR_ADF        ((1<<18))
#define RCR_RXFTH      ((1<<15)|(1<<14)|(1<<13))
#define RCR_RXFTH2     ((1<<15))
#define RCR_RXFTH1     ((1<<14))
#define RCR_RXFTH0     ((1<<13))
#define RCR_AICV       ((1<<12))
#define RCR_MXDMA      ((1<<10)|(1<< 9)|(1<< 8))
#define RCR_MXDMA2     ((1<<10))
#define RCR_MXDMA1     ((1<< 9))
#define RCR_MXDMA0     ((1<< 8))
#define RCR_9356SEL    ((1<< 6))
#define RCR_ACRC32     ((1<< 5))
#define RCR_AB         ((1<< 3))
#define RCR_AM         ((1<< 2))
#define RCR_APM        ((1<< 1))
#define RCR_AAP        ((1<< 0))

#define TCR 0x0040
#define TCR_HWVERID_MASK ((1<<27)|(1<<26)|(1<<25))
#define TCR_HWVERID_SHIFT 25

#define CMD 0x37
#define CMD_RST_SHIFT 4
#define CMD_RESERVED_MASK ((1<<1) | (1<<5) | (1<<6) | (1<<7))
#define CMD_RX_ENABLE_SHIFT 3
#define CMD_TX_ENABLE_SHIFT 2

#define CR_ResetTSF ((1<< 5))
#define CR_RST      ((1<< 4))
#define CR_RE       ((1<< 3))
#define CR_TE       ((1<< 2))
#define CR_MulRW    ((1<< 0))

#define RX_CONF 0x44
#define MAC_FILTER_MASK ((1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<5) | \
(1<<12) | (1<<18) | (1<<19) | (1<<20) | (1<<21) | (1<<22) | (1<<23))
#define RX_CHECK_BSSID_SHIFT 23
#define ACCEPT_PWR_FRAME_SHIFT 22
#define ACCEPT_MNG_FRAME_SHIFT 20
#define ACCEPT_CTL_FRAME_SHIFT 19
#define ACCEPT_DATA_FRAME_SHIFT 18
#define ACCEPT_ICVERR_FRAME_SHIFT 12
#define ACCEPT_CRCERR_FRAME_SHIFT 5
#define ACCEPT_BCAST_FRAME_SHIFT 3
#define ACCEPT_MCAST_FRAME_SHIFT 2
#define ACCEPT_ALLMAC_FRAME_SHIFT 0
#define ACCEPT_NICMAC_FRAME_SHIFT 1
#define RX_FIFO_THRESHOLD_MASK ((1<<13) | (1<<14) | (1<<15))
#define RX_FIFO_THRESHOLD_SHIFT 13
#define RX_FIFO_THRESHOLD_128 3
#define RX_FIFO_THRESHOLD_256 4
#define RX_FIFO_THRESHOLD_512 5
#define RX_FIFO_THRESHOLD_1024 6
#define RX_FIFO_THRESHOLD_NONE 7
#define RX_AUTORESETPHY_SHIFT 28
#define EPROM_TYPE_SHIFT 6
/*
 *  0x0084 - 0x00D3 is selected to page 0 when PSEn bit (bit0, PSR) 
 *  is set to 0
 */

/* 0x0084 - 0x008F - reserved */

#define DK0 0x0090
#define DK0_END 0x009F
#define DK1 0x00A0
#define DK1_END 0x00AF
#define DK2 0x00B0
#define DK2_END 0x00BF
#define DK3 0x00C0
#define DK3_END 0x00CF

#define RFTiming	        0x008C
#define ACM_CONTROL             0x00BF      // ACM Control Registe
#define INT_MIG                 0x00E2      // Interrupt Migration (0xE2 ~ 0xE3)
#define TID_AC_MAP         	0x00E8      // TID to AC Mapping Register

#define AC_VO_PARAM             0x00F0                    // AC_VO Parameters Record
#define AC_VI_PARAM             0x00F4                    // AC_VI Parameters Record
#define AC_BE_PARAM             0x00F8                    // AC_BE Parameters Record
#define AC_BK_PARAM             0x00FC                    // AC_BK Parameters Record  

/* 0x00D0 - 0x00D3 - reserved */



//- EEPROM data locations
#define RTL8187B_EEPROM_ID			0x8129
#define RTL8187_EEPROM_ID			0x8187
#define EEPROM_VID						0x02
#define EEPROM_PID						0x04
#define EEPROM_CHANNEL_PLAN			0x06
#define EEPROM_INTERFACE_SELECT		0x09
#define EEPROM_TX_POWER_BASE_OFFSET 0x0A
#define EEPROM_RF_CHIP_ID			0x0C
#define EEPROM_MAC_ADDRESS			0x0E // 0x0E - 0x13
#define EEPROM_CONFIG2				0x18
#define EEPROM_TX_POWER_LEVEL_12	0x14 // ch12, Byte offset 0x14
#define EEPROM_TX_POWER_LEVEL_1_2	0x2C // ch1 - ch2, Byte offset 0x2C - 0x2D
#define EEPROM_TX_POWER_LEVEL_3_4	0x2E // ch3 - ch4, Byte offset 0x2E - 0x2F
#define EEPROM_TX_POWER_LEVEL_5_6	0x30 // ch5 - ch6, Byte offset 0x30 - 0x31
#define EEPROM_TX_POWER_LEVEL_11	0x36 // ch11, Byte offset 0x36
#define EEPROM_TX_POWER_LEVEL_13_14	0x38 // ch13 - ch14, Byte offset 0x38 - 0x39
#define EEPROM_TX_POWER_LEVEL_7_10	0x7A // ch7 - ch10, Byte offset 0x7A - 0x7D

//
// 0x7E-0x7F is reserved for SW customization. 2006.04.21, by rcnjko.
//
// BIT[0-7] is for CustomerID where value 0x00 and 0xFF is reserved for Realtek.
#define EEPROM_SW_REVD_OFFSET		0x7E

#define EEPROM_CID_MASK				0x00FF
#define EEPROM_CID_RSVD0			0x00
#define EEPROM_CID_WHQL			0xFE
#define EEPROM_CID_RSVD1			0xFF
#define EEPROM_CID_ALPHA0			0x01
#define EEPROM_CID_SERCOMM_PS		0x02
#define EEPROM_CID_HW_LED			0x03
#define EEPROM_CID_TOSHIBA_KOREA	0x04	// Toshiba in Korea setting, added by Bruce, 2007-04-16.

// BIT[8-9] is for SW Antenna Diversity. Only the value EEPROM_SW_AD_ENABLE means enable, other values are diable.
#define EEPROM_SW_AD_MASK			0x0300
#define EEPROM_SW_AD_ENABLE			0x0100

// BIT[10-11] determine if Antenna 1 is the Default Antenna. Only the value EEPROM_DEF_ANT_1 means TRUE, other values are FALSE.
#define EEPROM_DEF_ANT_MASK			0x0C00
#define EEPROM_DEF_ANT_1			0x0400

// BIT[15] determine whether support LNA. EEPROM_LNA_ENABLE means support LNA. 2007.04.06, by Roger.
#define EEPROM_LNA_MASK			0x8000
#define EEPROM_LNA_ENABLE			0x8000


#define MAX_RX_DMA_MASK ((1<<8) | (1<<9) | (1<<10))
#define MAX_RX_DMA_2048 7
#define MAX_RX_DMA_1024	6
#define MAX_RX_DMA_SHIFT 10


#define TX_CONF 0x40
#define TX_CONF_HEADER_AUTOICREMENT_SHIFT 30
#define TX_LOOPBACK_SHIFT 17
#define TX_LOOPBACK_MAC 1
#define TX_LOOPBACK_BASEBAND 2
#define TX_LOOPBACK_NONE 0
#define TX_LOOPBACK_CONTINUE 3
#define TX_LOOPBACK_MASK ((1<<17)|(1<<18))
#define TX_LRLRETRY_SHIFT 0
#define R8180_MAX_RETRY 255
#define TX_SRLRETRY_SHIFT 8
#define TX_NOICV_SHIFT 19
#define TX_NOCRC_SHIFT 16


//#define CAM_SIZE                              16
#define TOTAL_CAM_ENTRY         16
#define CAM_ENTRY_LEN_IN_DW     6                                                                       // 6, unit: in u4byte. Added by Annie, 2006-05-25.
#define CAM_ENTRY_LEN_IN_BYTE   (CAM_ENTRY_LEN_IN_DW*sizeof(u4Byte))    // 24, unit: in u1byte. Added by Annie, 2006-05-25.

#define CAM_CONFIG_USEDK                1
#define CAM_CONFIG_NO_USEDK             0

#define CAM_WRITE                               0x00010000
#define CAM_READ                                0x00000000
#define CAM_POLLINIG                    0x80000000

//----------------------------------------------------------------------------
//       8187B WPA Config Register (offset 0xb0, 1 byte)
//----------------------------------------------------------------------------
#define SCR_UseDK                       0x01
#define SCR_TxSecEnable                 0x02
#define SCR_RxSecEnable                 0x04

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
#define CFG_DEFAULT_KEY         BIT(5)
#define CFG_VALID               BIT(15)


/*
 * Bitmasks for specific register functions. 
 * Names are derived from the register name and function name.
 *
 * <REGISTER>_<FUNCTION>[<bit>]
 *
 * this leads to some awkward names...
 */


#define CONFIG0_WEP104     ((1<<6))
#define CONFIG0_LEDGPO_En  ((1<<4))
#define CONFIG0_Aux_Status ((1<<3))
#define CONFIG0_GL         ((1<<1)|(1<<0))
#define CONFIG0_GL1        ((1<<1))
#define CONFIG0_GL0        ((1<<0))

#define CONFIG1_LEDS       ((1<<7)|(1<<6))
#define CONFIG1_LEDS1      ((1<<7))
#define CONFIG1_LEDS0      ((1<<6))
#define CONFIG1_LWACT      ((1<<4))
#define CONFIG1_MEMMAP     ((1<<3))
#define CONFIG1_IOMAP      ((1<<2))
#define CONFIG1_VPD        ((1<<1))
#define CONFIG1_PMEn       ((1<<0))

#define CONFIG2_LCK        ((1<<7))
#define CONFIG2_ANT        ((1<<6))
#define CONFIG2_DPS        ((1<<3))
#define CONFIG2_PAPE_sign  ((1<<2))
#define CONFIG2_PAPE_time  ((1<<1)|(1<<0))
#define CONFIG2_PAPE_time1 ((1<<1))
#define CONFIG2_PAPE_time0 ((1<<0))

#define CONFIG3_GNTSel     ((1<<7))
#define CONFIG3_PARM_En    ((1<<6))
#define CONFIG3_Magic      ((1<<5))
#define CONFIG3_CardB_En   ((1<<3))
#define CONFIG3_CLKRUN_En  ((1<<2))
#define CONFIG3_FuncRegEn  ((1<<1))
#define CONFIG3_FBtbEn     ((1<<0))

#define CONFIG4_VCOPDN     ((1<<7))
#define CONFIG4_PWROFF     ((1<<6))
#define CONFIG4_PWRMGT     ((1<<5))
#define CONFIG4_LWPME      ((1<<4))
#define CONFIG4_LWPTN      ((1<<2))
#define CONFIG4_RFTYPE     ((1<<1)|(1<<0))
#define CONFIG4_RFTYPE1    ((1<<1))
#define CONFIG4_RFTYPE0    ((1<<0))

#define CONFIG5_TX_FIFO_OK ((1<<7))
#define CONFIG5_RX_FIFO_OK ((1<<6))
#define CONFIG5_CALON      ((1<<5))
#define CONFIG5_EACPI      ((1<<2))
#define CONFIG5_LANWake    ((1<<1))
#define CONFIG5_PME_STS    ((1<<0))

#define MSR_LINK_MASK      ((1<<2)|(1<<3))
#define MSR_LINK_MANAGED   2
#define MSR_LINK_NONE      0
#define MSR_LINK_SHIFT     2
#define MSR_LINK_ADHOC     1
#define MSR_LINK_MASTER    3
#define MSR_LINK_ENEDCA	(1<<4)

#define DEFAULT_RATE_ADAPTIVE_TIMER_PERIOD	300

typedef enum _VERSION_8187{
	VERSION_8187_B,
	VERSION_8187_D,
	VERSION_8187B_B,
	VERSION_8187B_D,
	VERSION_8187B_E,
} VERSION_8187,*PVERSION_8187;

#define RTL8187B_VER_REG	0xE1		// 8187B version register.

struct rtl87b_hw_info{

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
#include <basic_types.h>
#endif
#endif // __RTL8187_SPEC_H__

