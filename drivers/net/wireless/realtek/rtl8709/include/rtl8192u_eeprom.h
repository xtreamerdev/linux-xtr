#ifndef __RTL8192U_EEPROM_H__
#define __RTL8192U_EEPROM_H__ 

#define CLOCK_RATE			50

#define CSR_EEPROM_CONTROL_REG		0xFE58
#define EEDO							0x01
#define EEDI							0x02
#define EECS							0x08
#define EESK							0x04
#define EEM0							0x40
#define EEM1							0x80

//----------------------------------------------------------------------------
//       8190 EEROM
//----------------------------------------------------------------------------

#define RTL8190_EEPROM_ID				0x8129
#define EEPROM_NODE_ADDRESS_BYTE_0	0x0C

#define EEPROM_TxPowerDiff				0x1F
#define EEPROM_ThermalMeter			0x20
#define EEPROM_PwDiff					0x21	//0x21
#define EEPROM_CrystalCap				0x22	//0x22
#define EEPROM_TxPwIndex_CCK			0x23	//0x23
#define EEPROM_TxPwIndex_OFDM_24G	0x24	//0x24~0x26
#define EEPROM_TxPwIndex_OFDM_5G		0x27	//0x27~0x3E
#define EEPROM_Default_TxPowerDiff		0x0
#define EEPROM_Default_ThermalMeter	0x7
#define EEPROM_Default_PwDiff			0x4
#define EEPROM_Default_CrystalCap		0x5
#define EEPROM_Default_TxPower			0x1010


#define EEPROM_ChannelPlan				0x7c	//0x7C
#define EEPROM_IC_VER					0x7d	//0x7D


//- EEPROM data locations
#define RTL8187B_EEPROM_ID			0x8129
#define RTL8187_EEPROM_ID			0x8187
#define EEPROM_VID						0x02
#define EEPROM_PID						0x04
#define EEPROM_CHANNEL_PLAN			0x06
#define EEPROM_INTERFACE_SELECT		0x09
#define EEPROM_TX_POWER_BASE_OFFSET 0x0A
#define EEPROM_RF_CHIP_ID			0x0C
#define EEPROM_MAC_ADDRESS			0x0C
#define EEPROM_CONFIG2				0x18
#define EEPROM_TX_POWER_LEVEL_12	0x14 // ch12, Byte offset 0x14
#define EEPROM_TX_POWER_LEVEL_1_6	0x2C // ch1 - ch6, Byte offset 0x2C - 0x31
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

// BIT[8-9] is for SW Antenna Diversity. Only the value EEPROM_SW_AD_ENABLE means enable, other values are diable.
#define EEPROM_SW_AD_MASK			0x0300
#define EEPROM_SW_AD_ENABLE			0x0100

// BIT[10-11] determine if Antenna 1 is the Default Antenna. Only the value EEPROM_DEF_ANT_1 means TRUE, other values are FALSE.
#define EEPROM_DEF_ANT_MASK			0x0C00
#define EEPROM_DEF_ANT_1			0x0400

// BIT[15] determine whether support LNA. EEPROM_LNA_ENABLE means support LNA. 2007.04.06, by Roger.
#define EEPROM_LNA_MASK			0x8000
#define EEPROM_LNA_ENABLE			0x8000


#endif

