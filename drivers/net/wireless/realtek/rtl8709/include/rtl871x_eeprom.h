#ifndef __RTL871X_EEPROM_H__
#define __RTL871X_EEPROM_H__

#include <drv_conf.h>
#include <osdep_service.h>

#define	RTL8711_EEPROM_ID			0x8711
#define	EEPROM_MAX_SIZE			256
#define	CLOCK_RATE					50			//100us		

//- EEPROM opcodes
#define EEPROM_READ_OPCODE		06
#define EEPROM_WRITE_OPCODE		05
#define EEPROM_ERASE_OPCODE		07
#define EEPROM_EWEN_OPCODE		19      // Erase/write enable
#define EEPROM_EWDS_OPCODE		16      // Erase/write disable

//Country codes
#define USA							0x555320
#define EUROPE						0x1 //temp, should be provided later	
#define JAPAN						0x2 //temp, should be provided later

#ifdef CONFIG_SDIO_HCI
#define eeprom_cis0_sz	17
#define eeprom_cis1_sz	50
#endif

typedef struct _BB_REGISTER_DEFINITION{
	u4Byte rfintfs; 			// set software control: 
							//		0x870~0x877[8 bytes]
							
	u4Byte rfintfi; 			// readback data: 
							//		0x8e0~0x8e7[8 bytes]
							
	u4Byte rfintfo; 			// output data: 
							//		0x860~0x86f [16 bytes]
							
	u4Byte rfintfe; 			// output enable: 
							//		0x860~0x86f [16 bytes]
							
	u4Byte rf3wireOffset; 		// LSSI data:
							//		0x840~0x84f [16 bytes]
							
	u4Byte rfLSSI_Select; 		// BB Band Select: 
							//		0x878~0x87f [8 bytes]
							
	u4Byte rfTxGainStage;		// Tx gain stage: 
							//		0x80c~0x80f [4 bytes]
							
      u4Byte rfHSSIPara1; 		// wire parameter control1 : 
      							//		0x820~0x823,0x828~0x82b, 0x830~0x833, 0x838~0x83b [16 bytes]
      							
      u4Byte rfHSSIPara2; 		// wire parameter control2 : 
      							//		0x824~0x827,0x82c~0x82f, 0x834~0x837, 0x83c~0x83f [16 bytes]
      							
      u4Byte rfSwitchControl; 	//Tx Rx antenna control : 
      							//		0x858~0x85f [16 bytes]
      							
      u4Byte rfAGCControl1; 	//AGC parameter control1 : 
      							//		0xc50~0xc53,0xc58~0xc5b, 0xc60~0xc63, 0xc68~0xc6b [16 bytes] 
      							
      u4Byte rfAGCControl2; 	//AGC parameter control2 : 
      							//		0xc54~0xc57,0xc5c~0xc5f, 0xc64~0xc67, 0xc6c~0xc6f [16 bytes] 
      							
      u4Byte rfRxIQImbalance; 	//OFDM Rx IQ imbalance matrix : 
      							//		0xc14~0xc17,0xc1c~0xc1f, 0xc24~0xc27, 0xc2c~0xc2f [16 bytes]
      							
      u4Byte rfRxAFE;  			//Rx IQ DC ofset and Rx digital filter, Rx DC notch filter : 
      							//		0xc10~0xc13,0xc18~0xc1b, 0xc20~0xc23, 0xc28~0xc2b [16 bytes]
      							
      u4Byte rfTxIQImbalance; 	//OFDM Tx IQ imbalance matrix
      							//		0xc80~0xc83,0xc88~0xc8b, 0xc90~0xc93, 0xc98~0xc9b [16 bytes]
      							
      u4Byte rfTxAFE; 			//Tx IQ DC Offset and Tx DFIR type
      							//		0xc84~0xc87,0xc8c~0xc8f, 0xc94~0xc97, 0xc9c~0xc9f [16 bytes]
      							
      u4Byte rfLSSIReadBack; 	//LSSI RF readback data
      							//		0x8a0~0x8af [16 bytes]
}BB_REGISTER_DEFINITION_T, *PBB_REGISTER_DEFINITION_T;



typedef	enum _RF_TYPE_819xU{
	RF_TYPE_MIN,	// 0
	RF_8225=1,			// 1 11b/g RF for verification only
	RF_8256=2,			// 2 11b/g/n 
	RF_8258=3,			// 3 11a/b/g/n RF
	// TODO: We sholud remove this psudo PHY RF after we get new RF.
	RF_PSEUDO_11N=4,	// 3, It is a temporality RF. 
}RF_TYPE_819xU,*PRF_TYPE_819xU;




struct eeprom_priv 
{    
	u8		bautoload_fail_flag;
	u8		bempty;
	u8		sys_config;
	u8		mac_addr[6];	
	u8		config0;
	u16	channel_plan;
	u8		country_string[3];	
	u8		tx_power_b[15];
	u8		tx_power_g[15];
	u8		tx_power_a[201];
	#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	u16		eepromid;
	u16		vid;
	u16		did;
	u8	cck_txpwr_base;
	u8	ofdm_txpwr_base;
	u8	blow_noise_amplifier;
	#endif	
	#ifdef CONFIG_SDIO_HCI
	u8		sdio_setting;	
	u32		ocr;
	u8		cis0[eeprom_cis0_sz];
	u8		cis1[eeprom_cis1_sz];	
	#endif

	#ifdef CONFIG_RTL8192U
	u8 EEPROMThermalMeter;
	u8 EEPROMPwDiff;
 u8 RF_Type;
	RF_TYPE_819xU			RFChipID;
	u32 MCSTxPowerLevelOriginalOffset[2];
	u32 CCKTxPowerLevelOriginalOffset;
	u32	RfReg0Value[4];
	u32	RFWritePageCnt[3];				//debug only
	u32	RFReadPageCnt[3];				//debug only

	BOOLEAN				bTXPowerDataReadFromEEPORM;
	BB_REGISTER_DEFINITION_T	PHYRegDef[4];	//Radio A/B/C/D
	VERSION_819xUsb			VersionID;
    u8 CrystalCap;
	
	u16 TxPowerDiff;
	#endif

};


extern void eeprom_write16(_adapter *padapter, u16 reg, u16 data);
extern u16 eeprom_read16(_adapter *padapter, u16 reg);
extern void read_eeprom_content(_adapter *padapter);
extern void eeprom_read_sz(_adapter * padapter, u16 reg,u8* data, u32 sz); 

extern void read_eeprom_content_by_attrib(_adapter *	padapter	);

#endif  //__RTL871X_EEPROM_H__
