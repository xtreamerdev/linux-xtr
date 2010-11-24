/* 
	This is part of rtl8187 OpenSource driver
	Copyright (C) Andrea Merello 2004-2005  <andreamrl@tiscali.it> 
	Released under the terms of GPL (General Public Licence)
	
	Parts of this driver are based on the GPL part of the official realtek driver
	Parts of this driver are based on the rtl8180 driver skeleton from Patric Schenke & Andres Salomon
	Parts of this driver are based on the Intel Pro Wireless 2100 GPL driver
	
	We want to tanks the Authors of such projects and the Ndiswrapper project Authors.
*/

/*This files contains card eeprom (93c46 or 93c56) programming routines*/
/*memory is addressed by WORDS*/

#include "r8187.h"
#include "r8180_hw.h"

#define EPROM_DELAY 10

#define EPROM_ANAPARAM_ADDRLWORD 0xd 
#define EPROM_ANAPARAM_ADDRHWORD 0xe 

#define EPROM_RFCHIPID 0x6
#define EPROM_TXPW_BASE 0x05
#define EPROM_RFCHIPID_RTL8225U 5 
#define EPROM_RF_PARAM 0x4
#define EPROM_CONFIG2 0xc

#define EPROM_VERSION 0x1E
#define MAC_ADR 0x7

#define CIS 0x18 

#define EPROM_TXPW0 0x16 
#define EPROM_TXPW2 0x1b
#define EPROM_TXPW1 0x3d

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

#define eprom_read                  RTL8187_SUFFIX(eprom_read)
#define Read_EEprom87B              RTL8187_SUFFIX(Read_EEprom87B)

u32 eprom_read(struct net_device *dev,u32 addr); //reads a 16 bits word
u16 Read_EEprom87B(struct net_device *dev, u16 addr); // read 8187b eeprom
