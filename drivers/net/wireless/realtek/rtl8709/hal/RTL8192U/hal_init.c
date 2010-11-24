/******************************************************************************
* hal_init.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/

#define _HAL_INIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include<rtl8711_byteorder.h>

#include <hal_init.h>

#include <usb_hal.h>

#include <rtl871x_debug.h>

#if defined(CONFIG_RTL8192U)
#include <rtl8192u_spec.h>
#include <rtl8192u_regdef.h>
#include <rtl8192u/rtl8192u_haldef.h>

#include <farray_r8192u.h>
#endif

#include <wlan_mlme.h>
#include <wlan_sme.h>
#include <wlan.h>

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
	{0x27, 0x8d, 0002}, {0x4d, 0x08, 0002}, {0x50, 0x30, 0002}, {0x51, 0xf5, 0002}, // change 
	{0x52, 0x04, 0002}, {0x53, 0xa0, 0002}, {0x54, 0x1f, 0002}, {0x55, 0x23, 0002}, 
	{0x56, 0x45, 0002}, {0x57, 0x67, 0002}, {0x58, 0x08, 0002}, {0x59, 0x08, 0002}, 
	{0x5a, 0x08, 0002}, {0x5b, 0x08, 0002}, {0x60, 0x08, 0002}, {0x61, 0x08, 0002}, 
	{0x62, 0x08, 0002}, {0x63, 0x08, 0002}, {0x64, 0xcf, 0002}, {0x72, 0x56, 0002}, 
	{0x73, 0x9a, 0002},

	{0x34, 0xf0, 0000}, {0x35, 0x0f, 0000}, {0x5b, 0x40, 0000}, {0x84, 0x88, 0000},
	{0x85, 0x24, 0000}, {0x88, 0x44, 0000}, {0x8b, 0xb8, 0000}, {0x8c, 0x07, 0000},// change
	{0x8d, 0x00, 0000}, {0x94, 0x1b, 0000}, {0x95, 0x12, 0000}, {0x96, 0x00, 0000},
	{0x97, 0x06, 0000}, {0x9d, 0x1a, 0000}, {0x9f, 0x10, 0000}, {0xb4, 0x22, 0000},
	{0xbe, 0x80, 0000}, {0xdb, 0x00, 0000}, {0xee, 0x00, 0000}, {0x91, 0x03, 0000},

	{0x4c, 0x00, 0002}, {0x9f, 0x00, 0003}, {0x8c, 0x01, 0000}, {0x8d, 0x10, 0000},
	{0x8e, 0x08, 0000}, {0x8f, 0x00, 0000}
};

#if 0
static u32 ZEBRA_RF_RX_GAIN_TABLE[]={	
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
static u32 ZEBRA_RF_RX_GAIN_LNA_TABLE[]={  
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

static u8  ZEBRA_AGC[]={
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
// Use the new SD3 given param, by shien chang, 2006.07.14
static u8 OFDM_CONFIG[]={
	// 0x00
	0x10, 0x0d, 0x01, 0x00, 0x14, 0xfb, 0xfb, 0x60, 
	0x00, 0x60, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 
			
	// 0x10
	0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0xa8, 0x26, 
	0x32, 0x33, 0x07, 0xa5, 0x6f, 0x55, 0xc8, 0xb3, 
			
	// 0x20
	0x0a, 0xe1, 0x2C, 0x4a, 0x86, 0x83, 0x34, 0x0f, 
	0x4f, 0x24, 0x6f, 0xc2, 0x6b, 0x40, 0x80, 0x00, 
			
	// 0x30
	0xc0, 0xc1, 0x58, 0xf1, 0x00, 0xe4, 0x90, 0x3e, 
	0x6d, 0x3c, 0xfb, 0xc7
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
#endif

u32 rtl8225_chan[] = {
	0,	//dummy channel 0
	0x085c, //1	 
	0x08dc, //2  
	0x095c, //3  
	0x09dc, //4  
	0x0a5c, //5  
	0x0adc, //6  
	0x0b5c, //7  
	0x0bdc, //8  
	0x0c5c, //9 
	0x0cdc, //10  
	0x0d5c, //11  
	0x0ddc, //12  
	0x0e5c, //13 
	//0x0f5c, //14
	0x0f72, // 14  
};

//-
static u8 rtl8225z2_tx_power_cck_ch14[]={
	0x36,0x35,0x2e,0x1b,0x00,0x00,0x00,0x00, // 0 dB
//	0x30,0x2f,0x29,0x18,0x00,0x00,0x00,0x00, // 1 dB, commented out by rcnjko, 2006.04.27.
	0x30,0x2f,0x29,0x15,0x00,0x00,0x00,0x00, // 1 dB, 0x15 -> 0x18, asked for by SD3 ED. 2006.04.27, by rcnjko.
	0x30,0x2f,0x29,0x15,0x00,0x00,0x00,0x00, // 061004, TODO: SD3 ED didn't determine it.
	0x30,0x2f,0x29,0x15,0x00,0x00,0x00,0x00, // 061004, TODO: SD3 ED didn't determine it.
};


//-
static u8 rtl8225z2_tx_power_cck[]={
	0x36,0x35,0x2e,0x25,0x1c,0x12,0x09,0x04, // 0 dB
	0x30,0x2f,0x29,0x21,0x19,0x10,0x08,0x03, // 1 dB
	0x2b,0x2a,0x25,0x1e,0x16,0x0e,0x07,0x03, // 061004, specified by ED.
	0x26,0x25,0x21,0x1b,0x14,0x0d,0x06,0x03, // 061004, specified by ED.
};

//2005.11.16,
u8 ZEBRA2_CCK_OFDM_GAIN_SETTING[]={
        0x00,0x01,0x02,0x03,0x04,0x05,
        0x06,0x07,0x08,0x09,0x0a,0x0b,
        0x0c,0x0d,0x0e,0x0f,0x10,0x11,
        0x12,0x13,0x14,0x15,0x16,0x17,
        0x18,0x19,0x1a,0x1b,0x1c,0x1d,
        0x1e,0x1f,0x20,0x21,0x22,0x23,
};

#ifdef CONFIG_RTL8192U

u4Byte Rtl8192UsbMACPHY_ArrayDTM[] = {
0x03c,0xffff0000,0x00001818,
0x340,0xffffffff,0x00000000,
0x344,0xffffffff,0x00000000,
0x348,0x0000ffff,0x00000000,
0x12c,0xffffffff,0x08003001,
0x318,0x00000fff,0x00000800,
};

u4Byte Rtl8192UsbMACPHY_Array_PGDTM[] = {
0x03c,0xffff0000,0x00000f0f,
0x340,0xffffffff,0x04080808,
0x344,0xffffffff,0x00000204,
0x348,0x0000ffff,0x00000000,
0x12c,0xffffffff,0x40020080,
0x318,0x00000fff,0x00000100,
};

u4Byte Rtl8192UsbAGCTAB_ArrayDTM[AGCTAB_ArrayLengthDTM] = {
0xc78,0x7d000001,
0xc78,0x7d010001,
0xc78,0x7d020001,
0xc78,0x7d030001,
0xc78,0x7c040001,
0xc78,0x7b050001,
0xc78,0x7a060001,
0xc78,0x79070001,
0xc78,0x78080001,
0xc78,0x77090001,
0xc78,0x760a0001,
0xc78,0x750b0001,
0xc78,0x740c0001,
0xc78,0x730d0001,
0xc78,0x720e0001,
0xc78,0x710f0001,
0xc78,0x70100001,
0xc78,0x6f110001,
0xc78,0x6e120001,
0xc78,0x6d130001,
0xc78,0x6c140001,
0xc78,0x6b150001,
0xc78,0x6a160001,
0xc78,0x69170001,
0xc78,0x68180001,
0xc78,0x67190001,
0xc78,0x661a0001,
0xc78,0x651b0001,
0xc78,0x641c0001,
0xc78,0x491d0001,
0xc78,0x481e0001,
0xc78,0x471f0001,
0xc78,0x46200001,
0xc78,0x45210001,
0xc78,0x44220001,
0xc78,0x43230001,
0xc78,0x28240001,
0xc78,0x27250001,
0xc78,0x26260001,
0xc78,0x25270001,
0xc78,0x24280001,
0xc78,0x23290001,
0xc78,0x222a0001,
0xc78,0x092b0001,
0xc78,0x082c0001,
0xc78,0x072d0001,
0xc78,0x062e0001,
0xc78,0x052f0001,
0xc78,0x04300001,
0xc78,0x03310001,
0xc78,0x02320001,
0xc78,0x01330001,
0xc78,0x00340001,
0xc78,0x00350001,
0xc78,0x00360001,
0xc78,0x00370001,
0xc78,0x00380001,
0xc78,0x00390001,
0xc78,0x003a0001,
0xc78,0x003b0001,
0xc78,0x003c0001,
0xc78,0x003d0001,
0xc78,0x003e0001,
0xc78,0x003f0001,
0xc78,0x7d400001,
0xc78,0x7d410001,
0xc78,0x7d420001,
0xc78,0x7d430001,
0xc78,0x7c440001,
0xc78,0x7b450001,
0xc78,0x7a460001,
0xc78,0x79470001,
0xc78,0x78480001,
0xc78,0x77490001,
0xc78,0x764a0001,
0xc78,0x754b0001,
0xc78,0x744c0001,
0xc78,0x734d0001,
0xc78,0x724e0001,
0xc78,0x714f0001,
0xc78,0x70500001,
0xc78,0x6f510001,
0xc78,0x6e520001,
0xc78,0x6d530001,
0xc78,0x6c540001,
0xc78,0x6b550001,
0xc78,0x6a560001,
0xc78,0x69570001,
0xc78,0x68580001,
0xc78,0x67590001,
0xc78,0x665a0001,
0xc78,0x655b0001,
0xc78,0x645c0001,
0xc78,0x495d0001,
0xc78,0x485e0001,
0xc78,0x475f0001,
0xc78,0x46600001,
0xc78,0x45610001,
0xc78,0x44620001,
0xc78,0x43630001,
0xc78,0x28640001,
0xc78,0x27650001,
0xc78,0x26660001,
0xc78,0x25670001,
0xc78,0x24680001,
0xc78,0x23690001,
0xc78,0x226a0001,
0xc78,0x096b0001,
0xc78,0x086c0001,
0xc78,0x076d0001,
0xc78,0x066e0001,
0xc78,0x056f0001,
0xc78,0x04700001,
0xc78,0x03710001,
0xc78,0x02720001,
0xc78,0x01730001,
0xc78,0x00740001,
0xc78,0x00750001,
0xc78,0x00760001,
0xc78,0x00770001,
0xc78,0x00780001,
0xc78,0x00790001,
0xc78,0x007a0001,
0xc78,0x007b0001,
0xc78,0x007c0001,
0xc78,0x007d0001,
0xc78,0x007e0001,
0xc78,0x007f0001,
0xc78,0x3000001e,
0xc78,0x3001001e,
0xc78,0x3002001e,
0xc78,0x3003001e,
0xc78,0x3004001e,
0xc78,0x3605001e,
0xc78,0x3a06001e,
0xc78,0x3c07001e,
0xc78,0x3c08001e,
0xc78,0x3e09001e,
0xc78,0x400a001e,
0xc78,0x400b001e,
0xc78,0x420c001e,
0xc78,0x420d001e,
0xc78,0x440e001e,
0xc78,0x440f001e,
0xc78,0x4410001e,
0xc78,0x4611001e,
0xc78,0x4612001e,
0xc78,0x4813001e,
0xc78,0x4814001e,
0xc78,0x4a15001e,
0xc78,0x4a16001e,
0xc78,0x4c17001e,
0xc78,0x4c18001e,
0xc78,0x4c19001e,
0xc78,0x4e1a001e,
0xc78,0x4e1b001e,
0xc78,0x4e1c001e,
0xc78,0x501d001e,
0xc78,0x501e001e,
0xc78,0x501f001e,
0xc78,0x5020001e,
0xc78,0x5221001e,
0xc78,0x5222001e,
0xc78,0x5423001e,
0xc78,0x5424001e,
0xc78,0x5625001e,
0xc78,0x5626001e,
0xc78,0x5627001e,
0xc78,0x5828001e,
0xc78,0x5829001e,
0xc78,0x582a001e,
0xc78,0x582b001e,
0xc78,0x5a2c001e,
0xc78,0x5a2d001e,
0xc78,0x5a2e001e,
0xc78,0x5c2f001e,
0xc78,0x5c30001e,
0xc78,0x5c31001e,
0xc78,0x5e32001e,
0xc78,0x5e33001e,
0xc78,0x5e34001e,
0xc78,0x6035001e,
0xc78,0x6036001e,
0xc78,0x6037001e,
0xc78,0x6238001e,
0xc78,0x6239001e,
0xc78,0x623a001e,
0xc78,0x643b001e,
0xc78,0x663c001e,
0xc78,0x683d001e,
0xc78,0x6c3e001e,
0xc78,0x6c3f001e,
};

u4Byte Rtl8192UsbPHY_REGArrayDTM[PHY_REGArrayLengthDTM] = {
0x800,0x00050060,
0x804,0x00000001,
0x808,0x0000fc00,
0x80c,0x0000001c,
0x810,0x80101088,
0x814,0x000908c0,
0x818,0x00000000,
0x81c,0x00000000,
0x820,0x00000004,
0x824,0x00690000,
0x828,0x00000004,
0x82c,0x00a90000,
0x830,0x00000004,
0x834,0x00690000,
0x838,0x00000004,
0x83c,0x00a90000,
0x840,0x00000000,
0x844,0x00000000,
0x848,0x00000000,
0x84c,0x00000000,
0x850,0x00000000,
0x854,0x00000000,
0x858,0xa965a965,
0x85c,0xa965a965,
0x860,0x001f0010,
0x864,0x001f0010,
0x868,0x001f0010,
0x86c,0x001f0010,
0x870,0x0f700f70,
0x874,0x0f700f70,
0x878,0x00000000,
0x87c,0x00000000,
0x880,0x5c385eb8,
0x884,0x6357060d,
0x888,0x0460c341,
0x88c,0x0000ff00,
0x890,0x00000000,
0x894,0xfffffffe,
0x898,0x40302010,
0x89c,0x00706050,
0x8b0,0x00000000,
0x8e0,0x00000000,
0x8e4,0x00000000,
0x900,0x00000000,
0x904,0x00000023,
0x908,0x00000000,
0x90c,0x35541551,
0xa00,0x00d047d8,
0xa04,0xa01f0008,
0xa08,0x80c88300,
0xa0c,0x2e62120f,
0xa10,0x95009b78,
0xa14,0x112e5008,
0xa18,0x00881117,
0xa1c,0x89140fa0,
0xa20,0x13130000,
0xa24,0x06090d10,
0xa28,0x00000103,
0xa2c,0x00000000,
0xc00,0x00000080,
0xc04,0x00005001,
0xc08,0x000000e4,
0xc0c,0x6c6c6c6c,
0xc10,0x08000000,
0xc14,0x40000100,
0xc18,0x08000000,
0xc1c,0x40000100,
0xc20,0x08000000,
0xc24,0x40000100,
0xc28,0x08000000,
0xc2c,0x40000100,
0xc30,0x8de96c44,
0xc34,0x154052cd,
0xc38,0x00010a10,
0xc3c,0x0a979764,
0xc40,0x1f7c423f,
0xc44,0x000100b7,
0xc48,0xec020000,
0xc4c,0x00000325,
0xc50,0xe9543424,
0xc54,0x433c0194,
0xc58,0xe9543424,
0xc5c,0x433c0194,
0xc60,0xe9543424,
0xc64,0x433c0194,
0xc68,0xe9543424,
0xc6c,0x433c0194,
0xc70,0x2c7f000d,
0xc74,0x018616db,
0xc78,0x0000001f,
0xc7c,0x00b91612,
0xc80,0x288000a2,
0xc84,0x00000000,
0xc88,0x40000100,
0xc8c,0x08200000,
0xc90,0x288000a2,
0xc94,0x00000000,
0xc98,0x40000100,
0xc9c,0x00000000,
0xca0,0x00492492,
0xca4,0x00000000,
0xca8,0x00000000,
0xcac,0x00000000,
0xcb0,0x00000000,
0xcb4,0x00000000,
0xcb8,0x00000000,
0xcbc,0x00492492,
0xcc0,0x00000000,
0xcc4,0x00000000,
0xcc8,0x00000000,
0xccc,0x00000000,
0xcd0,0x00000000,
0xcd4,0x00000000,
0xcd8,0x64b22427,
0xcdc,0x00766932,
0xce0,0x00222222,
0xd00,0x00000780,
0xd04,0x00000401,
0xd08,0x0000803f,
0xd0c,0x00000001,
0xd10,0xa0699999,
0xd14,0x99993c67,
0xd18,0x6a8f5b6b,
0xd1c,0x00000000,
0xd20,0x00000000,
0xd24,0x00000000,
0xd28,0x00000000,
0xd2c,0xcc979975,
0xd30,0x00000000,
0xd34,0x00000000,
0xd38,0x00000000,
0xd3c,0x00027293,
0xd40,0x00000000,
0xd44,0x00000000,
0xd48,0x00000000,
0xd50,0x6437140a,
0xd54,0x024dbd02,
0xd58,0x00000000,
0xd5c,0x04032064,
};

u4Byte Rtl8192UsbRadioA_ArrayDTM[radioA_ArrayLengthDTM] = {
0x019,0x00000c03,
0x000,0x000000bf,
0x001,0x000006e0,
0x002,0x0000004c,
0x003,0x000007f1,
0x004,0x00000975,
0x005,0x00000c58,
0x006,0x00000ae6,
0x007,0x000000ca,
0x008,0x00000e1c,
0x009,0x000007f0,
0x00a,0x000009d0,
0x00b,0x000001ba,
0x00c,0x00000440,
0x00e,0x00000020,
0x00f,0x00000990,
0x012,0x00000800,
0x014,0x000005ab,
0x015,0x00000fb0,
0x016,0x00000020,
0x017,0x00000597,
0x018,0x0000050a,
0x01a,0x00000418,
0x01b,0x00000f5e,
0x01c,0x00000008,
0x01d,0x00000607,
0x01e,0x000006cc,
0x01f,0x00000000,
0x020,0x00000096,
0x01f,0x00000001,
0x020,0x00000076,
0x01f,0x00000002,
0x020,0x00000056,
0x01f,0x00000003,
0x020,0x00000036,
0x01f,0x00000004,
0x020,0x00000016,
0x01f,0x00000005,
0x020,0x000001f6,
0x01f,0x00000006,
0x020,0x000001d6,
0x01f,0x00000007,
0x020,0x000001b6,
0x01f,0x00000008,
0x020,0x00000196,
0x01f,0x00000009,
0x020,0x00000176,
0x01f,0x0000000a,
0x020,0x000000f7,
0x01f,0x0000000b,
0x020,0x000000d7,
0x01f,0x0000000c,
0x020,0x000000b7,
0x01f,0x0000000d,
0x020,0x00000097,
0x01f,0x0000000e,
0x020,0x00000077,
0x01f,0x0000000f,
0x020,0x00000057,
0x01f,0x00000010,
0x020,0x00000037,
0x01f,0x00000011,
0x020,0x000000fb,
0x01f,0x00000012,
0x020,0x000000db,
0x01f,0x00000013,
0x020,0x000000bb,
0x01f,0x00000014,
0x020,0x000000ff,
0x01f,0x00000015,
0x020,0x000000e3,
0x01f,0x00000016,
0x020,0x000000c3,
0x01f,0x00000017,
0x020,0x000000a3,
0x01f,0x00000018,
0x020,0x00000083,
0x01f,0x00000019,
0x020,0x00000063,
0x01f,0x0000001a,
0x020,0x00000043,
0x01f,0x0000001b,
0x020,0x00000023,
0x01f,0x0000001c,
0x020,0x00000003,
0x01f,0x0000001d,
0x020,0x000001e3,
0x01f,0x0000001e,
0x020,0x000001c3,
0x01f,0x0000001f,
0x020,0x000001a3,
0x01f,0x00000020,
0x020,0x00000183,
0x01f,0x00000021,
0x020,0x00000163,
0x01f,0x00000022,
0x020,0x00000143,
0x01f,0x00000023,
0x020,0x00000123,
0x01f,0x00000024,
0x020,0x00000103,
0x023,0x00000203,
0x024,0x00000300,
0x00b,0x000001ba,
0x02c,0x000003d7,
0x02d,0x00000ff0,
0x000,0x00000037,
0x004,0x00000160,
0x007,0x00000080,
0x002,0x0000088d,
0x0fe,
0x0fe,
0x016,0x00000200,
0x016,0x00000380,
0x016,0x00000020,
0x016,0x000001a0,
0x01a,0x00000e00,
0x000,0x000000bf,
0x00d,0x0000001f,
0x001,0x00000ee0,
0x00d,0x0000009f,
0x002,0x0000004d,
0x004,0x00000975,
0x007,0x00000700,
};

u4Byte Rtl8192UsbRadioB_ArrayDTM[radioB_ArrayLengthDTM] = {
0x019,0x00000c03,
0x000,0x000000bf,
0x001,0x000006e0,
0x002,0x0000004c,
0x003,0x000007f1,
0x004,0x00000975,
0x005,0x00000c58,
0x006,0x00000ae6,
0x007,0x000000ca,
0x008,0x00000e1c,
0x00a,0x000009d0,
0x00b,0x000001ba,
0x00c,0x00000440,
0x00e,0x00000020,
0x015,0x00000fb0,
0x016,0x00000020,
0x017,0x00000597,
0x018,0x0000050a,
0x01a,0x00000418,
0x01b,0x00000f5e,
0x01d,0x00000607,
0x01e,0x000006cc,
0x00b,0x000001ba,
0x023,0x00000203,
0x024,0x00000300,
0x000,0x00000037,
0x004,0x00000160,
0x016,0x00000200,
0x016,0x00000380,
0x016,0x00000020,
0x016,0x000001a0,
0x01a,0x00000e00,
0x000,0x000000bf,
0x002,0x0000004d,
0x004,0x00000975,
0x007,0x00000700,
};

u4Byte Rtl8192UsbRadioC_ArrayDTM[radioC_ArrayLengthDTM] = {
0x019,0x00000c03,
0x000,0x000000bf,
0x001,0x000006e0,
0x002,0x0000004c,
0x003,0x000007f1,
0x004,0x00000975,
0x005,0x00000c58,
0x006,0x00000ae6,
0x007,0x000000ca,
0x008,0x00000e1c,
0x009,0x000007f0,
0x00a,0x000009d0,
0x00b,0x000001ba,
0x00c,0x00000440,
0x00e,0x00000020,
0x00f,0x00000990,
0x012,0x00000800,
0x014,0x000005ab,
0x015,0x00000fb0,
0x016,0x00000020,
0x017,0x00000597,
0x018,0x0000050a,
0x01a,0x00000418,
0x01b,0x00000f5e,
0x01c,0x00000008,
0x01d,0x00000607,
0x01e,0x000006cc,
0x01f,0x00000000,
0x020,0x00000096,
0x01f,0x00000001,
0x020,0x00000076,
0x01f,0x00000002,
0x020,0x00000056,
0x01f,0x00000003,
0x020,0x00000036,
0x01f,0x00000004,
0x020,0x00000016,
0x01f,0x00000005,
0x020,0x000001f6,
0x01f,0x00000006,
0x020,0x000001d6,
0x01f,0x00000007,
0x020,0x000001b6,
0x01f,0x00000008,
0x020,0x00000196,
0x01f,0x00000009,
0x020,0x00000176,
0x01f,0x0000000a,
0x020,0x000000f7,
0x01f,0x0000000b,
0x020,0x000000d7,
0x01f,0x0000000c,
0x020,0x000000b7,
0x01f,0x0000000d,
0x020,0x00000097,
0x01f,0x0000000e,
0x020,0x00000077,
0x01f,0x0000000f,
0x020,0x00000057,
0x01f,0x00000010,
0x020,0x00000037,
0x01f,0x00000011,
0x020,0x000000fb,
0x01f,0x00000012,
0x020,0x000000db,
0x01f,0x00000013,
0x020,0x000000bb,
0x01f,0x00000014,
0x020,0x000000ff,
0x01f,0x00000015,
0x020,0x000000e3,
0x01f,0x00000016,
0x020,0x000000c3,
0x01f,0x00000017,
0x020,0x000000a3,
0x01f,0x00000018,
0x020,0x00000083,
0x01f,0x00000019,
0x020,0x00000063,
0x01f,0x0000001a,
0x020,0x00000043,
0x01f,0x0000001b,
0x020,0x00000023,
0x01f,0x0000001c,
0x020,0x00000003,
0x01f,0x0000001d,
0x020,0x000001e3,
0x01f,0x0000001e,
0x020,0x000001c3,
0x01f,0x0000001f,
0x020,0x000001a3,
0x01f,0x00000020,
0x020,0x00000183,
0x01f,0x00000021,
0x020,0x00000163,
0x01f,0x00000022,
0x020,0x00000143,
0x01f,0x00000023,
0x020,0x00000123,
0x01f,0x00000024,
0x020,0x00000103,
0x023,0x00000203,
0x024,0x00000300,
0x00b,0x000001ba,
0x02c,0x000003d7,
0x02d,0x00000ff0,
0x000,0x00000037,
0x004,0x00000160,
0x007,0x00000080,
0x002,0x0000088d,
0x0fe,
0x0fe,
0x016,0x00000200,
0x016,0x00000380,
0x016,0x00000020,
0x016,0x000001a0,
0x01a,0x00000e00,
0x000,0x000000bf,
0x00d,0x0000001f,
0x001,0x00000ee0,
0x00d,0x0000009f,
0x002,0x0000004d,
0x004,0x00000975,
0x007,0x00000700,
};

u4Byte Rtl8192UsbRadioD_ArrayDTM[radioD_ArrayLengthDTM] = {
0x019,0x00000c03,
0x000,0x000000bf,
0x001,0x000006e0,
0x002,0x0000004c,
0x003,0x000007f1,
0x004,0x00000975,
0x005,0x00000c58,
0x006,0x00000ae6,
0x007,0x000000ca,
0x008,0x00000e1c,
0x00a,0x000009d0,
0x00b,0x000001ba,
0x00c,0x00000440,
0x00e,0x00000020,
0x015,0x00000fb0,
0x016,0x00000020,
0x017,0x00000597,
0x018,0x0000050a,
0x01a,0x00000418,
0x01b,0x00000f5e,
0x01d,0x00000607,
0x01e,0x000006cc,
0x00b,0x000001ba,
0x023,0x00000203,
0x024,0x00000300,
0x000,0x00000037,
0x004,0x00000160,
0x016,0x00000200,
0x016,0x00000380,
0x016,0x00000020,
0x016,0x000001a0,
0x01a,0x00000e00,
0x000,0x000000bf,
0x002,0x0000004d,
0x004,0x00000975,
0x007,0x00000700,
};


#endif

VOID Hal_SetHwReg819xUsb(IN PADAPTER    Adapter, IN u8 variable, IN u8 * val);
void SetTxPowerLevel819xUsb(_adapter *padapter, short ch);

typedef struct _TX_FW_BOOT_FRAME{
	TX_DESC_CMD_819x_USB	tx_fw_cmd;
	u8 						tx_fw_data[((u32)BootArrayLength&0xfffffffc)+4];

	_adapter * 				padapter;
	PURB 					ptxfw_urb;
}TX_FW_BOOT_FRAME;

typedef struct _TX_FW_MAIN_FRAME{
	TX_DESC_CMD_819x_USB	tx_fw_cmd;
	u8 						tx_fw_data[1024];

	_adapter * 				padapter;
	PURB 					ptxfw_urb;
	u32						step;
	u32						tx_fw_data_offset;
}TX_FW_MAIN_FRAME,*PTX_FW_MAIN_FRAME;

typedef struct _TX_FW_DATA_FRAME{
	TX_DESC_CMD_819x_USB	tx_fw_cmd;
	u8 						tx_fw_data[1024];

	_adapter * 				padapter;
	PURB 					ptxfw_urb;
	u32						step;
	u32						tx_fw_data_offset;
}TX_FW_DATA_FRAME;

typedef struct _TX_FW_FRAME{
	TX_DESC_CMD_819x_USB	tx_fw_cmd;
	u8 						tx_fw_data[1024];

	_adapter *				padapter;
	PURB					ptxfw_urb;
	u32 					step;
	u32 					tx_fw_data_offset;
}TX_FW_FRAME,*PTX_FW_FRAME;

TX_FW_BOOT_FRAME *TxfwBootFrame;
TX_FW_MAIN_FRAME *TxfwMainFrame;
TX_FW_DATA_FRAME *TxfwDataFrame;


typedef u32 AC_CODING;
#define AC0_BE	0		// ACI: 0x00	// Best Effort
#define AC1_BK	1		// ACI: 0x01	// Background
#define AC2_VI	2		// ACI: 0x10	// Video
#define AC3_VO	3		// ACI: 0x11	// Voice
#define AC_MAX	4		// Max: define total number; Should not to be used as a real enum.

typedef struct 	ChnlAccessSetting {
	u16 SIFS_Timer;
	u16 DIFS_Timer; 
	u16 SlotTimeTimer;
	u16 EIFS_Timer;
	u16 CWminIndex;
	u16 CWmaxIndex;
}*PCHANNEL_ACCESS_SETTING,CHANNEL_ACCESS_SETTING;


#ifdef CONFIG_EMBEDDED_FWIMG
pu1Byte	HAL_FwImageBuf[3] = 	{&Rtl819XFwBootArray[0], 
						 	  &Rtl819XFwMainArray[0],
						 	  &Rtl819XFwDataArray[0] };
u4Byte	HAL_FwImageLen[3] = 	{sizeof(Rtl819XFwBootArray), 
						 	  sizeof(Rtl819XFwMainArray),
						 	  sizeof(Rtl819XFwDataArray) };
pu1Byte	HAL_FwImageBufDTM[3] = 	{&Rtl819XFwBootArrayDTM[0], 
							 	  &Rtl819XFwMainArrayDTM[0],
							 	  &Rtl819XFwDataArrayDTM[0] };
u4Byte	HAL_FwImageLenDTM[3] = 	{sizeof(Rtl819XFwBootArrayDTM), 
							 	  sizeof(Rtl819XFwMainArrayDTM),
							 	  sizeof(Rtl819XFwDataArrayDTM) };
#endif


// Local static function prototypes
RT_STATUS Hal_CPUCheckMainCodeOKAndTurnOnCPU(IN PADAPTER Adapter);
RT_STATUS Hal_CPUCheckFirmwareReady(IN PADAPTER Adapter);
RT_STATUS Hal_fwSendDownloadCode(IN PADAPTER 	Adapter, IN u32 Step);
//RT_STATUS Hal_fwSendDownloadCode(IN PADAPTER Adapter, IN pu1Byte CodeVirtualAddress, IN u4Byte BufferLen);
RT_STATUS Hal_FWDownload(IN PADAPTER Adapter);

void usb_write_txfw_complete_boot(struct urb* purb, struct pt_regs *regs);
void usb_write_txfw_complete_main(struct urb* purb, struct pt_regs *regs);
void usb_write_txfw_complete_data(struct urb* purb, struct pt_regs *regs);
//void usb_write_txfw_complete_boot(struct urb* purb);
//void usb_write_txfw_complete_main(struct urb* purb);
//void usb_write_txfw_complete_data(struct urb* purb);

VOID TxCmd_AppendZeroAndEndianTransform(u1Byte* pDst,u1Byte* pSrc,u2Byte* pLength);


RT_STATUS
phy_RF8256_Config_ParaFile(
	IN	PADAPTER		Adapter
	);


u4Byte phy_RFSerialRead(_adapter * padapter, u8 eRFPath, u32 Offset);

u4Byte phy_CalculateBitShift(u4Byte BitMask);

void PHY_SetBBReg(_adapter *padapter,u32 RegAddr, u32 BitMask , u32 Data);

void PHY_SetRFReg(_adapter *padapter, u8 eRFPath, u32 RegAddr, u32 BitMask , u32 Data);

u4Byte PHY_QueryBBReg(_adapter *padapter, u32 RegAddr , u32 BitMask);


RT_STATUS
PHY_ConfigRFWithHeaderFile(
	IN	PADAPTER			Adapter,
	RF90_RADIO_PATH_E		eRFPath
);


u4Byte
PHY_QueryRFReg(
	IN	PADAPTER			Adapter,
	IN	u1Byte eRFPath,
	IN	u4Byte				RegAddr,
	IN	u4Byte				BitMask
	);


void Set_HW_ACPARAM_8192(
		_adapter *padapter,
		PAC_PARAM pAcParam, 
		PCHANNEL_ACCESS_SETTING ChnlAccessSetting);


void cam_mark_invalid(_adapter *padapter, u8 ucIndex)
{
	u32 content=0;
	u32 cmd=0;
	struct security_priv *psecpriv = &padapter->securitypriv;
	u32 algo =  psecpriv->dot11PrivacyAlgrthm;


	// keyid must be set in config field
	content |= (ucIndex&3) | ((u16)(algo)<<2);

	content |= BIT(15);
	// polling bit, and No Write enable, and address
	cmd= CAM_CONTENT_COUNT*ucIndex;
	cmd= cmd | BIT(31)|BIT(16);
	// write content 0 is equall to mark invalid

	write32(padapter,WCAMI, content); //msleep_os(40);
	DEBUG_INFO(("CAM_mark_invalid(): WRITE A4: %x \n",content));
	write32(padapter,RWCAM,cmd); //msleep_os(40);
	DEBUG_INFO(("CAM_mark_invalid(): WRITE A0: %x \n",cmd));
}


void cam_empty_entry(_adapter *padapter, u8 ucIndex)
{
	u32 cmd = 0;
	u32 content = 0;
	u8 i;	
	u32 algo = padapter->securitypriv.dot11PrivacyAlgrthm;

	for(i=0;i<8;i++)
	{

		// filled id in CAM config 2 byte
		if( i == 0)
		{
			content |=(ucIndex & 0x03) | ((u16)(algo)<<2);
			content |= BIT(15);
							
		}
		else
		{
			content = 0;			
		}
		// polling bit, and No Write enable, and address
		cmd= CAM_CONTENT_COUNT*ucIndex+i;
		cmd= cmd | BIT(31)|BIT(16);
		// write content 0 is equall to mark invalid
		write32(padapter,WCAMI, content); //msleep_os(40);
		DEBUG_INFO(("CAM_empty_entry(): WRITE A4: %x \n",content));
		write32(padapter,RWCAM,cmd); //msleep_os(40);
		DEBUG_INFO(("CAM_empty_entry(): WRITE A0: %x \n",cmd));
	}
}

void cam_reset(_adapter *padapter)
{
	write32(padapter, RWCAM, BIT(31)|BIT(30) ); 
}

void ActUpdateChannelAccessSetting_8192(_adapter *padapter,
		int WirelessMode)
{
	AC_CODING	eACI;
	AC_PARAM	AcParam;
	BOOLEAN		bFollowLegacySetting = _FALSE;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	PNDIS_WLAN_BSSID_EX	pcur_network = &_sys_mib->cur_network;
	PCHANNEL_ACCESS_SETTING pChnlAccessSetting = (PCHANNEL_ACCESS_SETTING)_malloc(sizeof(CHANNEL_ACCESS_SETTING));

	switch( WirelessMode )
	{
		case WIRELESS_11A:
		#if 0
			ChnlAccessSetting->SIFS_Timer = 0x22;
			ChnlAccessSetting->DIFS_Timer = 34; // 34 = 16 + 2*9. 2006.06.07, by rcnjko.
			ChnlAccessSetting->SlotTimeTimer = 9;
			ChnlAccessSetting->EIFS_Timer = 23;
		#endif
			pChnlAccessSetting->CWminIndex = 4;
			pChnlAccessSetting->CWmaxIndex = 10;
			break;

		case WIRELESS_11B:
		#if 0
			ChnlAccessSetting->SIFS_Timer = 0x22;
			ChnlAccessSetting->DIFS_Timer = 50; // 50 = 10 + 2*20. 2006.06.07, by rcnjko.
			ChnlAccessSetting->SlotTimeTimer = 20;
			ChnlAccessSetting->EIFS_Timer = 91;
		#endif
			pChnlAccessSetting->CWminIndex = 5;
			pChnlAccessSetting->CWmaxIndex = 10;
			break;

		case WIRELESS_11G:
		case WIRELESS_11BG:
		
		#if 0	
			ChnlAccessSetting->SIFS_Timer = 0x22; // Suggested by Jong, 2005.12.08.
			ChnlAccessSetting->SlotTimeTimer = 9; // 2006.06.07, by rcnjko.
			ChnlAccessSetting->DIFS_Timer = 28; // 28 = 10 + 2*9. 2006.06.07, by rcnjko.
			ChnlAccessSetting->EIFS_Timer = 0x5B; // Suggested by wcchu, it is the default value of EIFS register, 2005.12.08.
		#endif	

			// TODO: SlotTimeTimer  = 20?? check XP driver
			pChnlAccessSetting->SlotTimeTimer = 20;
			pChnlAccessSetting->CWminIndex = 4;
			pChnlAccessSetting->CWmaxIndex = 10;
			break;
		case WIRELESS_11N_2G4:
		case WIRELESS_11N_5G:
//			ChnlAccessSetting->SIFS_Timer = 0xf;
//			ChnlAccessSetting->DIFS_Timer = 20;
		//xiong add it
			pChnlAccessSetting->SlotTimeTimer = 20;

//			ChnlAccessSetting->EIFS_Timer = 91;
			pChnlAccessSetting->CWminIndex = 4;
			pChnlAccessSetting->CWmaxIndex = 10;
			break;
		default:
			DEBUG_ERR(("ActUpdateChannelAccessSetting_8192(): Wireless mode is not defined!\n"));
			break;


			
	}

	Hal_SetHwReg819xUsb(padapter, HW_VAR_SLOT_TIME, (u8*)&pChnlAccessSetting->SlotTimeTimer );

	// <RJ_TODO_NOW_8185B> Update ECWmin/ECWmax, AIFS, TXOP Limit of each AC to the value defined by SPEC.
	if(pcur_network->BssQos.bdQoSMode > QOS_DISABLE )
	{ // QoS mode.
		// Follow AC Parameters of the QBSS.
		DEBUG_INFO(("Not QoS Disable \n"));
		for(eACI = 0; eACI < AC_MAX; eACI++)
		{
			DEBUG_INFO(("AC: %d param \n", eACI ));
			Hal_SetHwReg819xUsb(padapter, HW_VAR_AC_PARAM, (u8 *)(&(pcur_network->BssQos.AcParameter[eACI])) );
		}
	}
	else
	{ // Legacy 802.11.
		DEBUG_INFO(("QoS Disable\n"));
		bFollowLegacySetting = _TRUE;
	}


// QosSetLegacyACParam
	if(bFollowLegacySetting)
	{
		//
		// Follow 802.11 seeting to AC parameter, all AC shall use the same parameter.
		// 2005.12.01, by rcnjko.
		//		
		AcParam.longData = 0;
		AcParam.f.AciAifsn.f.AIFSN = 2; // Follow 802.11 DIFS.
		AcParam.f.AciAifsn.f.ACM = 0;
		AcParam.f.Ecw.f.ECWmin = pChnlAccessSetting->CWminIndex; // Follow 802.11 CWmin.
		AcParam.f.Ecw.f.ECWmax = pChnlAccessSetting->CWmaxIndex; // Follow 802.11 CWmax.
		AcParam.f.TXOPLimit = 0;

		for(eACI = 0; eACI < AC_MAX; eACI++)
		{
			AcParam.f.AciAifsn.f.ACI = (u8)eACI;
			Hal_SetHwReg819xUsb(padapter, HW_VAR_AC_PARAM, (u8 *)&AcParam);
		}

		// TODO: AdHoc mode ?
#if 0
		// Use the CW defined in SPEC to prevent unfair Beacon distribution in AdHoc mode.
		// Only set this value to AC3_VO because the throughput shakes if other ACs set this value.
		// 2006.08.04, by shien chang.
		AcParam.f.AciAifsn.f.ACI = (u8)AC3_VO;		
		if( padapter->_sys_mib.mac_ctrl.opmode == WIFI_ADHOC_STATE) {
			AcParam.f.Ecw.f.ECWmin = 4; 
			AcParam.f.Ecw.f.ECWmax = 10;
		}
		Set_HW_ACPARAM(padapter,&AcParam,ChnlAccessSetting);
#endif		
	}
	_mfree((u8 *)pChnlAccessSetting, sizeof(CHANNEL_ACCESS_SETTING));
}

void config87b_asic(_adapter *padapter)//MacConfig_87BASIC_HardCode(struct net_device *dev)
{
	//============================================================================
	// MACREG.TXT
	//============================================================================
	int	nlines_read = 0;
	u32	reg_offset, reg_value, page_idx;
	int	i;

	nlines_read=(sizeof(MAC_REG_TABLE)/3)/4;

	for(i = 0; i < nlines_read; i++)
	{
		reg_offset=MAC_REG_TABLE[i][0]; 
		reg_value=MAC_REG_TABLE[i][1]; 
		page_idx=MAC_REG_TABLE[i][2]; 

		reg_offset|= (page_idx << 8);

		write8(padapter, reg_offset, (u8)reg_value); 
	}
	//============================================================================
}

void write_rtl8225(_adapter* padapter, u8 adr, u16 data)
{

	struct dvobj_priv *pdev = &padapter->dvobjpriv;
	struct usb_device *udev = pdev->pusbdev;
	int status = 0;

	data = (data << 4) | (u16)(adr & 0x0f);

	//
	// OUT a vendor request to ask 8051 do HW three write operation.
	//
	DEBUG_INFO(("before write rtl8225 , data = %x\n",data));
	status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
                               		   0x04, 0x40,
 						   0x0000, 0x1304, &data, 2, HZ / 2);

	if (status < 0)
        {
                DEBUG_ERR(("write_rtl8225 TimeOut! status:%x\n", status));
        }


}



VOID
Hal_SetBrateCfg(
	IN PADAPTER			Adapter,
	IN OCTET_STRING		*mBratesOS,
	OUT pu2Byte			pBrateCfg
)
{
	BOOLEAN			ret = _TRUE;
	u1Byte			i;
	u1Byte			brate;

	for(i=0;i<mBratesOS->Length;i++)
	{
		brate = mBratesOS->Octet[i] & 0x7f;	
		if( ret )
		{
			switch(brate)
			{
			case 0x02:	*pBrateCfg |= RRSR_1M;	break;
			case 0x04:	*pBrateCfg |= RRSR_2M;	break;
			case 0x0b:	*pBrateCfg |= RRSR_5_5M;	break;
			case 0x16:	*pBrateCfg |= RRSR_11M;	break;
			case 0x0c:	*pBrateCfg |= RRSR_6M;	break;
			case 0x12:	*pBrateCfg |= RRSR_9M;	break;
			case 0x18:	*pBrateCfg |= RRSR_12M;	break;
			case 0x24:	*pBrateCfg |= RRSR_18M;	break;
			case 0x30:	*pBrateCfg |= RRSR_24M;	break;
			case 0x48:	*pBrateCfg |= RRSR_36M;	break;
			case 0x60:	*pBrateCfg |= RRSR_48M;	break;
			case 0x6c:	*pBrateCfg |= RRSR_54M;	break;
			}
		}

	}
}


VOID
SetHwVarACMCTRL(
           IN  PVOID pAdapter,
           PAC_PARAM pAcParam
)
{
         PADAPTER Adapter = (PADAPTER)pAdapter;
     
         AC_CODING	eACI = pAcParam->f.AciAifsn.f.ACI;
         u1Byte		AcmCtrl = PlatformEFIORead1Byte( Adapter, ACMHWCONTROL );
	AcmCtrl = AcmCtrl | 0x1;

			if( pAcParam->f.AciAifsn.f.ACM)
			{ // ACM bit is 1.
				switch(eACI)
				{
				case AC0_BE:
					AcmCtrl |= AcmHw_BeqEn;
					break;

				case AC2_VI:
					AcmCtrl |= AcmHw_ViqEn;
					break;

				case AC3_VO:
					AcmCtrl |= AcmHw_VoqEn;
					break;

				default:
					DEBUG_ERR(("[HW_VAR_ACM_CTRL] ACM set failed: eACI is %d\n", eACI ) );
					break;
				}
			}
			else
			{ // ACM bit is 0.
				switch(eACI)
				{
				case AC0_BE:
					AcmCtrl &= (~AcmHw_BeqEn);
					break;

				case AC2_VI:
					AcmCtrl &= (~AcmHw_ViqEn);
					break;

				case AC3_VO:
					AcmCtrl &= (~AcmHw_BeqEn);
					break;

				default:
					break;
				}
			}

			DEBUG_ERR(("SetHwReg8190pci(): [HW_VAR_ACM_CTRL] Write 0x%X\n", AcmCtrl ) );
			PlatformEFIOWrite1Byte(Adapter, ACMHWCONTROL, AcmCtrl );
}

void SetBeaconRelatedRegisters819xUsb(_adapter *padapter, u16 atimwindow)
{

	u16 BcnTimeCfg = 0;
	u16 BcnCW = 6, BcnIFS = 0xf;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	// ATIM window
	//
	write16(padapter, ATIMWND, atimwindow);
	
	//
	// Beacon interval (in unit of TU).
	//
	write16(padapter, BCNITV, get_beacon_interval(&_sys_mib->cur_network));
	//
	// DrvErlyInt (in unit of TU). (Time to send interrupt to notify driver to change beacon content)
	//
	write16(padapter, DRVERLYINT, 1);

	//
	// BcnDMATIM(in unit of us). Indicates the time before TBTT to perform beacon queue DMA 
	//
	write16(padapter, BCNDMA, 1023);	
	//PlatformEFIOWrite2Byte(Adapter, BCN_DMATIME, 256); // HWSD suggest this value 2006.11.14

	//
	// Force beacon frame transmission even after receiving beacon frame from other ad hoc STA
	//
	write8(padapter, BCN_ERR_THRESH, 100); // Reference from WMAC code 2006.11.14

 	if (_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)		
	 {
			BcnTimeCfg |= (BcnCW<<8 );
			BcnTimeCfg |= BcnIFS<<0;
			write16(padapter, BCNTCFG, BcnTimeCfg);

	 }
	
			

}


void set_hwbasicrate(_adapter *padapter ,u16 rate)
{

	u32 regtemp; 
	u16 val =  rate;
	val = val & 0x15f;
	regtemp = read32(padapter, RRSR);
	regtemp &= 0xffff0000;
	regtemp &=  (~(RRSR_MCS4 | RRSR_MCS5 | RRSR_MCS6 | RRSR_MCS7));

	write32(padapter, RRSR, val |regtemp);
	write32(padapter, RATR0+4*7, val);



}


VOID Hal_SetHwReg819xUsb(IN PADAPTER	Adapter, IN u8 variable, IN u8 * val)
{
	struct eeprom_priv*	peeprompriv = &Adapter->eeprompriv;
	struct mib_info *_sys_mib = &(Adapter->_sys_mib);
	PNDIS_WLAN_BSSID_EX	pcur_network = &_sys_mib->cur_network;
	AC_CODING	eACI;
	// PlatformEFIOWrite1Byte(Adapter, CR9346, 0xc0);	// enable config register write

	switch(variable)
	{
	case HW_VAR_ETHER_ADDR:
			write32(Adapter, IDR0, ((u32*)(val))[0]);
			write16(Adapter, IDR4, ((u16*)(val+4))[0]);
			break;

	case HW_VAR_MULTICAST_REG:
			break;
	
	case HW_VAR_BASIC_RATE:
	{
			u16 val = 0 ,i =0;
			struct mib_info *pmibinfo = &(Adapter->_sys_mib);
			

			//update ARFR & BRSR
			for (i = 0; i< 12; i++)
			{
				if ((pmibinfo->mac_ctrl.basicrate[i]) != 0xff)
					val |= BIT((11 - i));
			}
			
			// 2007.01.16, by Emily
			// Select RRSR (in Legacy-OFDM and CCK)
			// For 8190, we select only 24M, 12M, 6M, 11M, 5.5M, 2M, and 1M from the Basic rate.
			// We do not use other rates.
			

			set_hwbasicrate(Adapter,val);

		}
		break;

	case HW_VAR_BSSID:
			PlatformEFIOWrite2Byte(Adapter, BSSIDR, (((pu2Byte)(val))[0]));
			PlatformEFIOWrite4Byte(Adapter, BSSIDR+2, (((pu4Byte)(val+2))[0]));
			break;

	case HW_VAR_MEDIA_STATUS:

		{
			
			u1Byte btMsr = MSR_NOLINK;			
			btMsr &= 0xfc;
			
			if(_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE){

				btMsr |= MSR_ADHOC;
				
			}
			else if(_sys_mib->mac_ctrl.opmode & WIFI_STATION_STATE){

				btMsr |= MSR_INFRA;
			}

			else 
				btMsr |= MSR_NOLINK;

			PlatformEFIOWrite1Byte(Adapter, MSR, btMsr);
		}
		break;
		
	#if 0	
	case HW_VAR_BEACON_INTERVAL:
		// Beacon interval (in unit of TU).
		PlatformEFIOWrite2Byte(Adapter, BCNITV, *((pu2Byte)val));
		break;
	#endif

	case HW_VAR_CONTENTION_WINDOW:
			break;

	case HW_VAR_RETRY_COUNT:
			break;

	case HW_VAR_SIFS:
		write8(Adapter, SIFS_OFDM, val[0]);
		write8(Adapter, SIFS_OFDM+1, val[0]);
		break;

	case HW_VAR_DIFS:
		break;

	case HW_VAR_EIFS:
		break;

	case HW_VAR_SLOT_TIME:
		{
			write8(Adapter, SLOT, val[0]);
			pcur_network->SlotTime = val[0];

			// Update AIFS according to curren Slot time. 2006.06.07, by rcnjko.
			if(pcur_network->BssQos.bdQoSMode > QOS_DISABLE )
			{
				for(eACI = 0; eACI < AC_MAX; eACI++)
				{
					Hal_SetHwReg819xUsb(Adapter, HW_VAR_AC_PARAM, (u8 *)(&(pcur_network->BssQos.AcParameter[eACI])) );
				}
			}
			else
			{
				u1Byte	u1bAIFS = aSifsTime + (2 * val[0]);
				
				write8(Adapter, EDCAPARA_VO, u1bAIFS);
				write8(Adapter, EDCAPARA_VI, u1bAIFS);
				write8(Adapter, EDCAPARA_BE, u1bAIFS);
				write8(Adapter, EDCAPARA_BK, u1bAIFS);
			}
		}
		break;

	case HW_VAR_ACK_PREAMBLE:	
			break;	

	case HW_VAR_CW_CONFIG:
		break;

	case HW_VAR_CW_VALUES:

		break;

	case HW_VAR_RATE_FALLBACK_CONTROL:
		break;

	case HW_VAR_COMMAND:
		PlatformEFIOWrite1Byte(Adapter, CMDR, *((pu1Byte)val));
		break;

	case HW_VAR_WPA_CONFIG:
		
		PlatformEFIOWrite1Byte(Adapter, SECR, *((pu1Byte)val));
		break;

	case HW_VAR_AC_PARAM:
		{
			PAC_PARAM	pAcParam = (PAC_PARAM)val;
			AC_CODING	eACI;
			u1Byte		u1bAIFS;
			u4Byte		u4bAcParam;


#if 0

PHAL_DATA_819xUSB	pHalData = GET_HAL_DATA(Adapter);

	pHalData->bCurrentTurboEDCA = FALSE;
	pHalData->bIsAnyNonBEPkts = FALSE;
#endif
			// Retrive paramters to udpate.
			eACI = pAcParam->f.AciAifsn.f.ACI; 
			u1bAIFS = pAcParam->f.AciAifsn.f.AIFSN * pcur_network->SlotTime + aSifsTime; 
			u4bAcParam = (	(((u4Byte)(pAcParam->f.TXOPLimit)) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
							(((u4Byte)(pAcParam->f.Ecw.f.ECWmax)) << AC_PARAM_ECW_MAX_OFFSET)	|  
							(((u4Byte)(pAcParam->f.Ecw.f.ECWmin)) << AC_PARAM_ECW_MIN_OFFSET)	|  
							(((u4Byte)u1bAIFS) << AC_PARAM_AIFS_OFFSET)	);

			switch(eACI)
			{
			case AC1_BK:
				PlatformEFIOWrite4Byte(Adapter, EDCAPARA_BK, u4bAcParam);
				break;

			case AC0_BE:
				PlatformEFIOWrite4Byte(Adapter, EDCAPARA_BE, u4bAcParam);
				break;

			case AC2_VI:
				PlatformEFIOWrite4Byte(Adapter, EDCAPARA_VI, u4bAcParam);
				break;

			case AC3_VO:
				PlatformEFIOWrite4Byte(Adapter, EDCAPARA_VO, u4bAcParam);
				break;

			default:
				DEBUG_ERR(("SetHwReg8192(): invalid ACI: %d !\n", eACI));
				break;
			}

			// Check ACM bit.
			// If it is set, immediately set ACM control bit to downgrading AC for passing WMM testplan. Annie, 2005-12-13.
			//xiong mask it temporarily
			SetHwVarACMCTRL(Adapter, pAcParam);
		}
		break;

	case HW_VAR_ACM_CTRL:
		{
	#if 	0 // SetHwVarACMCTRL();
			// TODO:  Modified this part and try to set acm control in only 1 IO processing!!
                      /*
			
			PACI_AIFSN	pAciAifsn = (PACI_AIFSN)val;
			AC_CODING	eACI = pAciAifsn->f.ACI;
			u1Byte		AcmCtrl = PlatformEFIORead1Byte( Adapter, AcmHwCtrl );
			AcmCtrl = AcmCtrl | ((Adapter->MgntInfo.pStaQos->AcmMethod == 2)?0x0:0x1);

			if( pAciAifsn->f.ACM )
			{ // ACM bit is 1.
				switch(eACI)
				{
				case AC0_BE:
					AcmCtrl |= AcmHw_BeqEn;
					break;

				case AC2_VI:
					AcmCtrl |= AcmHw_ViqEn;
					break;

				case AC3_VO:
					AcmCtrl |= AcmHw_VoqEn;
					break;

				default:
					RT_TRACE( COMP_QOS, DBG_LOUD, ("SetHwReg8185(): [HW_VAR_ACM_CTRL] ACM set failed: eACI is %d\n", eACI ) );
					break;
				}
			}
			else
			{ // ACM bit is 0.
				switch(eACI)
				{
				case AC0_BE:
					AcmCtrl &= (~AcmHw_BeqEn);
					break;

				case AC2_VI:
					AcmCtrl &= (~AcmHw_ViqEn);
					break;

				case AC3_VO:
					AcmCtrl &= (~AcmHw_BeqEn);
					break;

				default:
					break;
				}
			}

			RT_TRACE( COMP_QOS, DBG_LOUD, ("SetHwReg8190pci(): [HW_VAR_ACM_CTRL] Write 0x%X\n", AcmCtrl ) );
			PlatformEFIOWrite1Byte(Adapter, AcmHwCtrl, AcmCtrl );
*/
#if defined(TODO)

                           pHalData->pAciAifsn819XUsb =   (PACI_AIFSN)val;

#if USE_WORKITEM
	                    if( PlatformIsWorkItemScheduled(&(pHalData->SetHwVarACMCTRLWorkItem)) == FALSE)
	                    {
		                 PlatformScheduleWorkItem( &(pHalData->SetHwVarACMCTRLWorkItem) );
	                    }
#else
	                      SetHwVarACMCTRL((PVOID)Adapter);
#endif
#endif


#endif
		}
		break;

	case HW_VAR_DIS_Req_Qsize:	// Added by Annie, 2006-03-27.

		break;

#ifdef RTL8192_CCX_RADIO_MEASUREMENT
	case HW_VAR_CCX_CHNL_LOAD:
		{
			PRT_RM_REQ_PARAM	pRmReqPara = (PRT_RM_REQ_PARAM)val;
			u1Byte	u1bCmd;

			DEBUG_INFO(("Set HW_VAR_CCX_CHNL_LOAD: bToClear(%d) Duration(%d)\n", pRmReqPara->bToClear, pRmReqPara->Duration));

			SetupClm(Adapter, pRmReqPara);

			// Start to measure.
			u1bCmd = PlatformEFIORead1Byte(Adapter, MCTRL);
			PlatformEFIOWrite1Byte(Adapter, MCTRL, (u1bCmd | MCTRL_CLS));
		}
		break;

	case HW_VAR_CCX_NOISE_HISTOGRAM:
		{
			PRT_RM_REQ_PARAM	pRmReqPara = (PRT_RM_REQ_PARAM)val;
			u1Byte	u1bCmd;

			DEBUG_INFO( ("Set HW_VAR_CCX_NOISE_HISTOGRAM: bToClear(%d) Duration(%d)\n", pRmReqPara->bToClear, pRmReqPara->Duration));
			
			SetupNhm(Adapter, pRmReqPara);

			// Start to measure.
			// Note that, we must set CCX_CMD_BLOCK_ENABLE, asked for by SD3 C.M.Lin, 2006.05.05.
			u1bCmd = PlatformEFIORead1Byte(Adapter, MCTRL);
			PlatformEFIOWrite1Byte(Adapter, MCTRL, (u1bCmd | MCTRL_NHS | MCTRL_BLOCK_ENABLE));
		}
		break;


	case HW_VAR_CCX_CLM_NHM:
		{
			PRT_RM_REQ_PARAM	pRmReqPara = (PRT_RM_REQ_PARAM)val;
			u1Byte	u1bCmd;
			
			DEBUG_INFO( ("Set HW_VAR_CCX_CLM_NHM: bToClear(%d) Duration(%d)\n", pRmReqPara->bToClear, pRmReqPara->Duration));

			SetupClm(Adapter, pRmReqPara);
			SetupNhm(Adapter, pRmReqPara);

			// Start to measure.
			// Note that, we must set CCX_CMD_BLOCK_ENABLE, asked for by SD3 C.M.Lin, 2006.05.05.
			u1bCmd = PlatformEFIORead1Byte(Adapter, MCTRL);
			PlatformEFIOWrite1Byte(Adapter, MCTRL, (u1bCmd | MCTRL_CLS | MCTRL_NHS | MCTRL_BLOCK_ENABLE));	
		}
		break;
#endif

	case HW_VAR_RCR:
		{
			PlatformEFIOWrite4Byte(Adapter, RCR,((pu4Byte)(val))[0]);
		}
		break;

	case HW_VAR_RATR_0:
		{
			/* 2007/11/15 MH Copy from 8190PCI. */
			u4Byte ratr_value;
			ratr_value = ((pu4Byte)(val))[0];
			if(peeprompriv->RF_Type == RF_1T2R)	// 1T2R, Spatial Stream 2 should be disabled
			{
				ratr_value &=~ (RATE_ALL_OFDM_2SS);
				//DbgPrint("HW_VAR_TATR_0 from 0x%x ==> 0x%x\n", ((pu4Byte)(val))[0], ratr_value);
			}
			//DbgPrint("set HW_VAR_TATR_0 = 0x%x\n", ratr_value);
			//cosa PlatformEFIOWrite4Byte(Adapter, RATR0, ((pu4Byte)(val))[0]);
			PlatformEFIOWrite4Byte(Adapter, RATR0, ratr_value);
			PlatformEFIOWrite1Byte(Adapter, UFWP, 1);
#if 0		// Disable old code.
			u1Byte index;
			u4Byte input_value;
			index = (u1Byte)((((pu4Byte)(val))[0]) >> 28);
			input_value = (((pu4Byte)(val))[0]) & 0x0fffffff;
			// TODO: Correct it. Emily 2007.01.11
			PlatformEFIOWrite4Byte(Adapter, RATR0+index*4, input_value);
#endif
		}
		break;

	case HW_VAR_CPU_RST:
		PlatformEFIOWrite4Byte(Adapter, CPU_GEN, ((pu4Byte)(val))[0]);
		break;


	case HW_VAR_RF_STATE:
		{
#if defined(TODO)
			RT_RF_POWER_STATE eStateToSet = *((RT_RF_POWER_STATE *)val);
			SetRFPowerState(Adapter, eStateToSet);
#endif
		}
		break;	
		
	case HW_VAR_CECHK_BSSID:
		{
			u32 regRCR = 0;

			u8 type = ((u8 *)(val))[0];
		
			regRCR = PlatformEFIORead4Byte(Adapter, RCR);

			if (type == _TRUE)
				regRCR |= (RCR_CBSSID);
			else
				regRCR &= (~RCR_CBSSID);
			DEBUG_INFO(("type = %d\n", type));
			PlatformEFIOWrite4Byte(Adapter, RCR, regRCR);
			
		}
		break;

#ifdef USB_RX_AGGREGATION_SUPPORT
	case HW_VAR_USB_RX_AGGR:
		{			
			u8 Setting = ((pu1Byte)(val))[0];
			u32 ulValue;

			if(Setting==0)
			{
				ulValue = 0;
			}
			else
			{
				
				PRT_HIGH_THROUGHPUT pHTInfo = &Adapter->HTInfo;
#if 0
				if (pHalData->bForcedUsbRxAggr)
				{
					ulValue = pHalData->ForcedUsbRxAggrInfo;
				}
				else
#endif					
				{
					// If usb rx firmware aggregation is enabled, when anyone of three threshold conditions is reached, 
					// firmware will send aggregated packet to driver.
					ulValue = (pHTInfo->UsbRxFwAggrEn<<24) | (pHTInfo->UsbRxFwAggrPageNum<<16) | 
							(pHTInfo->UsbRxFwAggrPacketNum<<8) | (pHTInfo->UsbRxFwAggrTimeout);
				}
				
			}
			write32(Adapter, USB_RX_AGGR, ulValue);
		}
		break;
#endif



		
	default:
		break;
	
	}	

	// PlatformEFIOWrite1Byte(Adapter, CR9346, 0x00);	// disable config register write
}	


VOID Hal_HwConfigureRTL819xUsb( IN	PADAPTER Adapter)
{
//	PHAL_DATA_819xUSB	pHalData = GetHalData819xUsb(Adapter);
	u8	regBwOpMode = 0;
	u32	regRATR = 0, regRRSR = 0;
	u8	regTmp = 0;


	//1 This part need to modified according to the rate set we filtered!!
	//
	// Set RRSR, RATR, and BW_OPMODE registers
	//
	DEBUG_ERR(("Hal_HwConfigureRTL819xUsb: network type: %d\n", Adapter->_sys_mib.network_type));
	switch(Adapter->_sys_mib.network_type)
	{
	case WIRELESS_11B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK;
		regRRSR = RATE_ALL_CCK;
		break;
	case WIRELESS_11A:
		regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_11BG:
	case WIRELESS_11G:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_AUTO:
	case WIRELESS_11N_2G4:
		// It support CCK rate by default.
		// CCK rate will be filtered out only when associated AP does not support it.
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_11N_5G:
		regBwOpMode = BW_OPMODE_5G;
		regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	}
	
	// 2007/02/07 Mark by Emily becasue we have not verify whether this register works	
	write8(Adapter, BW_OPMODE, regBwOpMode);
	//cosa PlatformEFIOWrite4Byte(Adapter, RATR0, regRATR);
	Hal_SetHwReg819xUsb(Adapter, HW_VAR_RATR_0, (pu1Byte)(&regRATR));
	
	regTmp = read8(Adapter, 0x313);
	regRRSR = ((regTmp) << 24) | (regRRSR & 0x00ffffff);
	write32(Adapter, RRSR, regRRSR);

	// Set Retry Limit here
	write16(Adapter, RETRY_LIMIT, 
			Adapter->registrypriv.short_retry_lmt << SRL_SHIFT | Adapter->registrypriv.short_retry_lmt << LRL_SHIFT);

	// Set Contention Window here		

	// Set Tx AGC

	// Set Tx Antenna including Feedback control
		
	// Set Auto Rate fallback control
				
}

VOID
phy_InitBBRFRegisterDefinition(_adapter *padapter)
{
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;

	// RF Interface Sowrtware Control
	peeprompriv->PHYRegDef[RF90_PATH_A].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 LSBs if read 32-bit from 0x870
	peeprompriv->PHYRegDef[RF90_PATH_B].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 MSBs if read 32-bit from 0x870 (16-bit for 0x872)
	peeprompriv->PHYRegDef[RF90_PATH_C].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 LSBs if read 32-bit from 0x874
	peeprompriv->PHYRegDef[RF90_PATH_D].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 MSBs if read 32-bit from 0x874 (16-bit for 0x876)

	// RF Interface Readback Value
	peeprompriv->PHYRegDef[RF90_PATH_A].rfintfi = rFPGA0_XAB_RFInterfaceRB; // 16 LSBs if read 32-bit from 0x8E0	
	peeprompriv->PHYRegDef[RF90_PATH_B].rfintfi = rFPGA0_XAB_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E0 (16-bit for 0x8E2)
	peeprompriv->PHYRegDef[RF90_PATH_C].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 LSBs if read 32-bit from 0x8E4
	peeprompriv->PHYRegDef[RF90_PATH_D].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E4 (16-bit for 0x8E6)

	// RF Interface Output (and Enable)
	peeprompriv->PHYRegDef[RF90_PATH_A].rfintfo = rFPGA0_XA_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x860
	peeprompriv->PHYRegDef[RF90_PATH_B].rfintfo = rFPGA0_XB_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x864
	peeprompriv->PHYRegDef[RF90_PATH_C].rfintfo = rFPGA0_XC_RFInterfaceOE;// 16 LSBs if read 32-bit from 0x868
	peeprompriv->PHYRegDef[RF90_PATH_D].rfintfo = rFPGA0_XD_RFInterfaceOE;// 16 LSBs if read 32-bit from 0x86C

	// RF Interface (Output and)  Enable
	peeprompriv->PHYRegDef[RF90_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x860 (16-bit for 0x862)
	peeprompriv->PHYRegDef[RF90_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x864 (16-bit for 0x866)
	peeprompriv->PHYRegDef[RF90_PATH_C].rfintfe = rFPGA0_XC_RFInterfaceOE;// 16 MSBs if read 32-bit from 0x86A (16-bit for 0x86A)
	peeprompriv->PHYRegDef[RF90_PATH_D].rfintfe = rFPGA0_XD_RFInterfaceOE;// 16 MSBs if read 32-bit from 0x86C (16-bit for 0x86E)

	//Addr of LSSI. Wirte RF register by driver
	peeprompriv->PHYRegDef[RF90_PATH_A].rf3wireOffset = rFPGA0_XA_LSSIParameter; //LSSI Parameter
	peeprompriv->PHYRegDef[RF90_PATH_B].rf3wireOffset = rFPGA0_XB_LSSIParameter;
	peeprompriv->PHYRegDef[RF90_PATH_C].rf3wireOffset = rFPGA0_XC_LSSIParameter;
	peeprompriv->PHYRegDef[RF90_PATH_D].rf3wireOffset = rFPGA0_XD_LSSIParameter;

	// RF parameter
	peeprompriv->PHYRegDef[RF90_PATH_A].rfLSSI_Select = rFPGA0_XAB_RFParameter;  //BB Band Select
	peeprompriv->PHYRegDef[RF90_PATH_B].rfLSSI_Select = rFPGA0_XAB_RFParameter;
	peeprompriv->PHYRegDef[RF90_PATH_C].rfLSSI_Select = rFPGA0_XCD_RFParameter;
	peeprompriv->PHYRegDef[RF90_PATH_D].rfLSSI_Select = rFPGA0_XCD_RFParameter;

	// Tx AGC Gain Stage (same for all path. Should we remove this?)
	peeprompriv->PHYRegDef[RF90_PATH_A].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	peeprompriv->PHYRegDef[RF90_PATH_B].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	peeprompriv->PHYRegDef[RF90_PATH_C].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	peeprompriv->PHYRegDef[RF90_PATH_D].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage

	// Tranceiver A~D HSSI Parameter-1
	peeprompriv->PHYRegDef[RF90_PATH_A].rfHSSIPara1 = rFPGA0_XA_HSSIParameter1;  //wire control parameter1
	peeprompriv->PHYRegDef[RF90_PATH_B].rfHSSIPara1 = rFPGA0_XB_HSSIParameter1;  //wire control parameter1
	peeprompriv->PHYRegDef[RF90_PATH_C].rfHSSIPara1 = rFPGA0_XC_HSSIParameter1;  //wire control parameter1
	peeprompriv->PHYRegDef[RF90_PATH_D].rfHSSIPara1 = rFPGA0_XD_HSSIParameter1;  //wire control parameter1

	// Tranceiver A~D HSSI Parameter-2
	peeprompriv->PHYRegDef[RF90_PATH_A].rfHSSIPara2 = rFPGA0_XA_HSSIParameter2;  //wire control parameter2
	peeprompriv->PHYRegDef[RF90_PATH_B].rfHSSIPara2 = rFPGA0_XB_HSSIParameter2;  //wire control parameter2
	peeprompriv->PHYRegDef[RF90_PATH_C].rfHSSIPara2 = rFPGA0_XC_HSSIParameter2;  //wire control parameter2
	peeprompriv->PHYRegDef[RF90_PATH_D].rfHSSIPara2 = rFPGA0_XD_HSSIParameter2;  //wire control parameter1

	// RF switch Control
	peeprompriv->PHYRegDef[RF90_PATH_A].rfSwitchControl = rFPGA0_XAB_SwitchControl; //TR/Ant switch control
	peeprompriv->PHYRegDef[RF90_PATH_B].rfSwitchControl = rFPGA0_XAB_SwitchControl;
	peeprompriv->PHYRegDef[RF90_PATH_C].rfSwitchControl = rFPGA0_XCD_SwitchControl;
	peeprompriv->PHYRegDef[RF90_PATH_D].rfSwitchControl = rFPGA0_XCD_SwitchControl;

	// AGC control 1 
	peeprompriv->PHYRegDef[RF90_PATH_A].rfAGCControl1 = rOFDM0_XAAGCCore1;
	peeprompriv->PHYRegDef[RF90_PATH_B].rfAGCControl1 = rOFDM0_XBAGCCore1;
	peeprompriv->PHYRegDef[RF90_PATH_C].rfAGCControl1 = rOFDM0_XCAGCCore1;
	peeprompriv->PHYRegDef[RF90_PATH_D].rfAGCControl1 = rOFDM0_XDAGCCore1;

	// AGC control 2 
	peeprompriv->PHYRegDef[RF90_PATH_A].rfAGCControl2 = rOFDM0_XAAGCCore2;
	peeprompriv->PHYRegDef[RF90_PATH_B].rfAGCControl2 = rOFDM0_XBAGCCore2;
	peeprompriv->PHYRegDef[RF90_PATH_C].rfAGCControl2 = rOFDM0_XCAGCCore2;
	peeprompriv->PHYRegDef[RF90_PATH_D].rfAGCControl2 = rOFDM0_XDAGCCore2;

	// RX AFE control 1 
	peeprompriv->PHYRegDef[RF90_PATH_A].rfRxIQImbalance = rOFDM0_XARxIQImbalance;
	peeprompriv->PHYRegDef[RF90_PATH_B].rfRxIQImbalance = rOFDM0_XBRxIQImbalance;
	peeprompriv->PHYRegDef[RF90_PATH_C].rfRxIQImbalance = rOFDM0_XCRxIQImbalance;
	peeprompriv->PHYRegDef[RF90_PATH_D].rfRxIQImbalance = rOFDM0_XDRxIQImbalance;	

	// RX AFE control 1  
	peeprompriv->PHYRegDef[RF90_PATH_A].rfRxAFE = rOFDM0_XARxAFE;
	peeprompriv->PHYRegDef[RF90_PATH_B].rfRxAFE = rOFDM0_XBRxAFE;
	peeprompriv->PHYRegDef[RF90_PATH_C].rfRxAFE = rOFDM0_XCRxAFE;
	peeprompriv->PHYRegDef[RF90_PATH_D].rfRxAFE = rOFDM0_XDRxAFE;	

	// Tx AFE control 1 
	peeprompriv->PHYRegDef[RF90_PATH_A].rfTxIQImbalance = rOFDM0_XATxIQImbalance;
	peeprompriv->PHYRegDef[RF90_PATH_B].rfTxIQImbalance = rOFDM0_XBTxIQImbalance;
	peeprompriv->PHYRegDef[RF90_PATH_C].rfTxIQImbalance = rOFDM0_XCTxIQImbalance;
	peeprompriv->PHYRegDef[RF90_PATH_D].rfTxIQImbalance = rOFDM0_XDTxIQImbalance;	

	// Tx AFE control 2 
	peeprompriv->PHYRegDef[RF90_PATH_A].rfTxAFE = rOFDM0_XATxAFE;
	peeprompriv->PHYRegDef[RF90_PATH_B].rfTxAFE = rOFDM0_XBTxAFE;
	peeprompriv->PHYRegDef[RF90_PATH_C].rfTxAFE = rOFDM0_XCTxAFE;
	peeprompriv->PHYRegDef[RF90_PATH_D].rfTxAFE = rOFDM0_XDTxAFE;	

	// Tranceiver LSSI Readback
	peeprompriv->PHYRegDef[RF90_PATH_A].rfLSSIReadBack = rFPGA0_XA_LSSIReadBack;
	peeprompriv->PHYRegDef[RF90_PATH_B].rfLSSIReadBack = rFPGA0_XB_LSSIReadBack;
	peeprompriv->PHYRegDef[RF90_PATH_C].rfLSSIReadBack = rFPGA0_XC_LSSIReadBack;
	peeprompriv->PHYRegDef[RF90_PATH_D].rfLSSIReadBack = rFPGA0_XD_LSSIReadBack;	

}

RT_STATUS PHY_CheckBBAndRFOK(_adapter *padapter,HW90_BLOCK_E CheckBlock,RF90_RADIO_PATH_E eRFPath )
{
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;

	u4Byte				i, CheckTimes = 4,ulRegRead=0;

	u4Byte				WriteAddr[4];
	u4Byte				WriteData[] = {0xfffff027, 0xaa55a02f, 0x00000027, 0x55aa502f};

	// Initialize register address offset to be checked
	WriteAddr[HW90_BLOCK_MAC] = 0x100;
	WriteAddr[HW90_BLOCK_PHY0] = 0x900;
	WriteAddr[HW90_BLOCK_PHY1] = 0x800;
	WriteAddr[HW90_BLOCK_RF] = 0x3;
	
	for(i=0 ; i < CheckTimes ; i++)
	{

		//
		// Write Data to register and readback
		//
		switch(CheckBlock)
		{
		case HW90_BLOCK_MAC:
			DEBUG_ERR( ("PHY_CheckBBRFOK(): Never Write 0x100 here!"));
			break;
			
		case HW90_BLOCK_PHY0:
		case HW90_BLOCK_PHY1:
			write32(padapter, WriteAddr[CheckBlock], WriteData[i]);
			ulRegRead = read32(padapter, WriteAddr[CheckBlock]);
			break;

		case HW90_BLOCK_RF:
			WriteData[i] &= 0xfff;

			/*PHY_SetBBReg(Adapter,  pPhyReg->rfLSSIReadBack, bLSSIReadBackData, 0x000);
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Test BB READBACK REG: WriteData  %#x \n", 0x000));
			ulRegRead = PHY_QueryBBReg(Adapter, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Test BB READBACK REG: ReadData  %#x \n", ulRegRead));
                     */
			//DEBUG_ERR( ("PHY_RF8256_Config(): WriteData  %#x \n", WriteData[i]));
	
			PHY_SetRFReg(padapter, eRFPath, WriteAddr[HW90_BLOCK_RF], bMask12Bits, WriteData[i]);
			// TODO: we should not delay for such a long time. Ask SD3
			msleep_os(1);
			ulRegRead = PHY_QueryRFReg(padapter, eRFPath, WriteAddr[HW90_BLOCK_RF], bMask12Bits);				
			msleep_os(1);
			break;
			
		default:
			rtStatus = RT_STATUS_FAILURE;
			break;
		}


		//
		// Check whether readback data is correct
		//
		if(ulRegRead != WriteData[i])
		{
			DEBUG_ERR(("ulRegRead: %x, WriteData: %x \n", ulRegRead, WriteData[i]));
			rtStatus = RT_STATUS_FAILURE;			
			break;
		}
	}

	return rtStatus;
}



RT_STATUS PHY_ConfigBBWithHeaderFile(_adapter *padapter, u8 ConfigType)
{
	int i;

#if 0
	if(Adapter->bInHctTest)
	{	
		PHY_REGArrayLen = PHY_REGArrayLengthDTM;
		AGCTAB_ArrayLen = AGCTAB_ArrayLengthDTM;
		Rtl8190PHY_REGArray_Table = Rtl819XPHY_REGArrayDTM;
		Rtl8190AGCTAB_Array_Table = Rtl819XAGCTAB_ArrayDTM;
	}
	
#endif	


	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		DEBUG_INFO(("Start to dump the Rtl819XPHY_REG_1T2RArray regs\n"));
		//ArrayLength = sizeof(Rtl8190PHY_REGArray[]);
		for(i=0;i<PHY_REG_1T2RArrayLength;i=i+2)
		{
			//Rtl8190PHY_REGArray = Rtl8190PHY_REGArray+i;
			PHY_SetBBReg(padapter, Rtl819XPHY_REG_1T2RArray[i], bMaskDWord, Rtl819XPHY_REG_1T2RArray[i+1]);	
			DEBUG_INFO(("0x%03x,0x%08x,\n",Rtl819XPHY_REG_1T2RArray[i],read32(padapter, Rtl819XPHY_REG_1T2RArray[i])));

//			DEBUG_INFO(("Register(%lx): write %lx, read %lx,%lx \n",Rtl819XPHY_REG_1T2RArray[i], Rtl819XPHY_REG_1T2RArray[i+1],Rtl819XPHY_REG_1T2RArray[i],read32(padapter, Rtl819XPHY_REG_1T2RArray[i])));
		}
		DEBUG_INFO(("Finish to dump the Rtl819XPHY_REG_1T2RArray regs\n"));

	}else if(ConfigType == BaseBand_Config_AGC_TAB){
		DEBUG_INFO(("Start to dump the Rtl819XAGCTAB_Array regs\n"));
	
		//ArrayLength = sizeof(Rtl8190AGCTAB_Array[]);
		for(i=0;i<AGCTAB_ArrayLength;i=i+2)
		{
			//Rtl8190AGCTAB_Array = Rtl8190AGCTAB_Array+i;
			PHY_SetBBReg(padapter, Rtl819XAGCTAB_Array[i], bMaskDWord, Rtl819XAGCTAB_Array[i+1]);
			DEBUG_INFO(("0x%03x,0x%08x,\n",Rtl819XAGCTAB_Array[i],read32(padapter, Rtl819XAGCTAB_Array[i])));
		}

		DEBUG_INFO(("Finish to dump the Rtl819XAGCTAB_Array regs\n"));

	}
	
	return RT_STATUS_SUCCESS;
}






RT_STATUS
phy_BB819xUsb_Config_ParaFile(_adapter *padapter)
{

	RT_STATUS					rtStatus = RT_STATUS_SUCCESS;

	u1Byte						u1RegValue;
	u4Byte						u4RegValue;
	u1Byte						eCheckItem;

	//3//-----------------------------------------------------------------
	//3 //<1> Initialize Baseband
	//3//-----------------------------------------------------------------
	/*----Set BB Global Reset----*/
	u1RegValue = read8(padapter, BB_GLOBAL_RESET);
	write8(padapter, BB_GLOBAL_RESET, (u1RegValue|BB_GLOBAL_RESET_BIT) );


	/*----Set BB reset Active----*/
	u4RegValue = read32(padapter, CPU_GEN);
	write32(padapter, CPU_GEN, (u4RegValue)&(~CPU_GEN_BB_RST) );

	/*----Ckeck FPGAPHY0 and PHY1 board is OK----*/
	// TODO: this function should be removed on ASIC , Emily 2007.2.2
	for(eCheckItem=(HW90_BLOCK_E)HW90_BLOCK_PHY0; eCheckItem<=HW90_BLOCK_PHY1; eCheckItem++)
	{
		rtStatus  = PHY_CheckBBAndRFOK(padapter, (HW90_BLOCK_E)eCheckItem, (RF90_RADIO_PATH_E)0); //don't care RF path
		if(rtStatus != RT_STATUS_SUCCESS)
		{
			DEBUG_ERR(("%s(%d):Check PHY%d Fail!!\n", __FUNCTION__, __LINE__, eCheckItem));
			return rtStatus;
		}			
	}

	/*---- Set CCK and OFDM Block "OFF"----*/
	PHY_SetBBReg(padapter, rFPGA0_RFMOD, bCCKEn|bOFDMEn, 0x0);


	/*----BB Register Initilazation----*/
	//==m==>Set PHY REG From Header<==m==
//#if	RTL8190_Download_Firmware_From_Header
	rtStatus = PHY_ConfigBBWithHeaderFile(padapter,BaseBand_Config_PHY_REG);

	if(rtStatus != RT_STATUS_SUCCESS){
		DEBUG_ERR(("phy_RF8256_Config_ParaFile():Write BB Reg Fail!!"));
		goto phy_BB8190_Config_ParaFile_Fail;
	}
		

	/*----Set BB reset de-Active----*/
	u4RegValue = read32(padapter, CPU_GEN);
	write32(padapter, CPU_GEN, u4RegValue|CPU_GEN_BB_RST);
	

	/*----BB AGC table Initialization----*/
	//==m==>Set PHY REG From Header<==m==

//#if RTL8190_Download_Firmware_From_Header
	rtStatus = PHY_ConfigBBWithHeaderFile(padapter,BaseBand_Config_AGC_TAB);

	if(rtStatus != RT_STATUS_SUCCESS){
		DEBUG_ERR(("phy_RF8256_Config_ParaFile():Write BB AGC Table Fail!!"));
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	/*----Enable XSTAL ----*/
	// This shall be done before setting CrystalCap(0x880).
	write8(padapter, 0xfe5e, 0x00);


	// Antenna gain offset from B/C/D to A
	u4RegValue = (padapter->eeprompriv.TxPowerDiff & 0xff);
	PHY_SetBBReg(padapter, rFPGA0_TxGainStage, 	(bXBTxAGC|bXCTxAGC), u4RegValue);
    	
	// CrystalCap
	u4RegValue = (padapter->eeprompriv.CrystalCap & 0xf);	// crystal cap
	PHY_SetBBReg(padapter, rFPGA0_AnalogParameter1, bXtalCap, u4RegValue);

phy_BB8190_Config_ParaFile_Fail:	
	return rtStatus;
}






u8 PHY_BBConfig819xUsb(_adapter *padapter)
{
	u8	rtStatus = RT_STATUS_SUCCESS;

	phy_InitBBRFRegisterDefinition(padapter);

	//
	// Config BB and RF
	//

	// TODO: Adapter->MgntInfo.bRegHwParaFile 
	//?
	switch(1)//	switch( Adapter->MgntInfo.bRegHwParaFile )
	{
		case 0:
			DEBUG_ERR(("This function is not implement yet!! \n"));
			break;

		case 1:
			rtStatus = phy_BB819xUsb_Config_ParaFile(padapter);
			break;

		case 2:
			// Partial Modify. 
			DEBUG_ERR(("This function is not implement yet!! \n"));
			phy_BB819xUsb_Config_ParaFile(padapter);
			break;

		default:
			DEBUG_ERR(("This function is not implement yet!! \n"));
			break;
	}

	return rtStatus;
}




void CAM_program_entry(
	PADAPTER     		Adapter,
	u4Byte	 		iIndex, 
	u1Byte			*pucMacad,
	u1Byte			*pucKey128, 
	u2Byte 			usConfig
)
{
	u4Byte target_command=0;
	u4Byte target_content=0;
	u1Byte entry_i=0;

	DEBUG_ERR(("===> CAM_program_entry(): usConfig=0x%02X\n", usConfig ) );
	
	for(entry_i=0;entry_i<CAM_CONTENT_COUNT;entry_i++)
	{
		// polling bit, and write enable, and address
		//
		// 0xFFA0
		//		BIT[31]	Polling bit.
		//		BIT[16]	1=Write, 0=Read.
		//		BIT[6:0]	CAM_ADDRESS	//[AnnieNote] CAM: 6*16 = 96 DWs. Need 7 bits to  present it (64<=96<128, 128 is 2 order 7).
		//
		target_command= entry_i+CAM_CONTENT_COUNT*iIndex;
		target_command= target_command |BIT(31)|BIT(16);

		if(entry_i==0)
		{
			//first 32-bit is MAC address and CFG field
			target_content= (u4Byte)(*(pucMacad+0))<<16		// Byte 3
							|(u4Byte)(*(pucMacad+1))<<24	// Byte 2
							|(u4Byte)usConfig;				// Byte 1, 0

			//target_content=target_content|config;
			write32(Adapter, WCAMI, target_content);  //delay_ms(40);
			DEBUG_ERR(("CAM_program_entry(): WRITE %x: %x \n",WCAMI,target_content));
			write32(Adapter, RWCAM, target_command);  //delay_ms(40);
			DEBUG_ERR(("The Key ID is %d\n",iIndex));
			DEBUG_ERR(("CAM_program_entry(): WRITE %x: %x \n",RWCAM,target_command));
		}
		else if(entry_i==1)
		{
			//second 32-bit is MAC address
			target_content= (u4Byte)(*(pucMacad+5))<<24
				       |(u4Byte)(*(pucMacad+4))<<16
				       |(u4Byte)(*(pucMacad+3))<<8
				       |(u4Byte)(*(pucMacad+2));
			write32(Adapter, WCAMI, target_content);  //delay_ms(40);
			DEBUG_ERR(("CAM_program_entry(): WRITE A4: %x \n",target_content));
			write32(Adapter, RWCAM, target_command);  //delay_ms(40);
			DEBUG_ERR(("CAM_program_entry(): WRITE A0: %x \n",target_command));
		}
		else
		{
			// Key Material
			target_content= (u4Byte)(*(pucKey128+(entry_i*4-8)+3))<<24
					|(u4Byte)(*(pucKey128+(entry_i*4-8)+2))<<16
					|(u4Byte)(*(pucKey128+(entry_i*4-8)+1))<<8
					|(u4Byte)(*(pucKey128+(entry_i*4-8)+0));
			write32(Adapter, WCAMI, target_content);  //delay_ms(40);
			DEBUG_ERR(("CAM_program_entry(): WRITE A4: %x \n",target_content));
			write32(Adapter, RWCAM, target_command);  //delay_ms(40);
			DEBUG_ERR(("CAM_program_entry(): WRITE A0: %x \n",target_command));
		}
	}
	
  //do we need read-back immediately, or ...?
  //check later.
}


//----------------------------------------
//Function Definition for external usage 
//----------------------------------------
u1Byte CamAddOneEntry(
	PADAPTER     		padapter,
	u1Byte 			*pucMacAddr, 
	u4Byte 			ulKeyId, 
	u4Byte			ulEntryId,
	u4Byte 			ulEncAlg, 
	u4Byte 			ulUseDK, 
	u1Byte 			*pucKey
)
{

	//struct security_priv *pSec = &padapter->securitypriv;
	u1Byte retVal = 0;
	//u1Byte ucCamIndex = 0;
	u2Byte usConfig = 0;

	DEBUG_ERR(("===> CamAddOneEntry(): ulKeyId=%d, ulEncAlg=%d, ulUseDK=%d\n",
															ulKeyId, ulEncAlg, ulUseDK ) );

	if(ulKeyId==TOTAL_CAM_ENTRY)
	{
		DEBUG_ERR(("<=== CamAddOneEntry(): ulKeyId exceed!\n") );
		return retVal;
	}

	if(ulUseDK==1)
	{
		usConfig=usConfig|CFG_VALID|((u2Byte)(ulEncAlg)<<2);			
	}
	else	
	{
		usConfig=usConfig|CFG_VALID|((u2Byte)(ulEncAlg)<<2)|(u1Byte)(ulKeyId);
	}

	//Added by Mars we set the hal_code_base flag for the different architecture
	//between  818x and 8190 .The CAM content Entery won't record the address
	//in the default entry 0~3

	CAM_program_entry(padapter, ulEntryId,pucMacAddr,pucKey,usConfig);

	
	DEBUG_ERR(("<=== CamAddOneEntry()\n") );
	return 1;

}


int CamDeleteOneEntry(
		PADAPTER		padapter,
		u1Byte 			*pucMacAddr, 
		u4Byte 			ulKeyId
)
{

	u4Byte ulContent = 0;
	u4Byte ulCommand = 0;
						
	// polling bit, and No Write enable, and address
	ulCommand = ulKeyId*CAM_CONTENT_COUNT;
	ulCommand= ulCommand | BIT(31)|BIT(16);
	// write content 0 is equall to mark invalid
	write32(padapter, WCAMI, ulContent);  //delay_ms(40);
	DEBUG_ERR(("CAM_empty_entry(): WRITE A4: %x \n", ulContent));
	write32(padapter, RWCAM, ulCommand);  //delay_ms(40);
	DEBUG_ERR(("CAM_empty_entry(): WRITE A0: %x \n", ulCommand));			
	
 	return 0;
 	
}


void CamResetAllEntry(
	PADAPTER     	Adapter
)
{
	DEBUG_INFO(("CamResetAllEntry: \n"));
	//2004/02/11  In static WEP, OID_ADD_KEY or OID_ADD_WEP are set before STA associate to AP.
	// However, ResetKey is called on OID_802_11_INFRASTRUCTURE_MODE and MlmeAssociateRequest
	// In this condition, Cam can not be reset because upper layer will not set this static key again.
	//if(Adapter->EncAlgorithm == WEP_Encryption)
	//	return;

	write32(Adapter, RWCAM,( BIT(31)|BIT(30))); 
}


VOID
SetKey819xUsb(
	IN	_adapter			*padapter,
	IN	u32				KeyIndex,
	IN	u8*				pMacAddr,
	IN	u8				IsGroup,
	IN	u8				EncAlgo,
	IN	u8				IsWEPKey, //if OID = OID_802_11_WEP
	IN	u8				ClearAll
	)
{
	struct security_priv *	pSecInfo = &(padapter->securitypriv);
	pu1Byte				MacAddr = pMacAddr;
	
	//Addde by Mars Because of the architecture of 8190 we 
	//add a EntryId for tell from KeyId and EnteryId
	u4Byte EntryId = 0;
	BOOLEAN IsPairWise = _FALSE ;
	//BOOLEAN NeedAESSoftkEY = _FALSE;
	
	static u1Byte	CAM_CONST_ADDR[4][6] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x03}};
	static u1Byte	CAM_CONST_BROAD[] = 
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	if(ClearAll)
	{
		CamResetAllEntry(padapter);
	}
	else
	{
		if(EncAlgo == 0 || EncAlgo>5)
			EncAlgo = _TKIP_;
		
		if(IsWEPKey )//|| pSecInfo->UseDefaultKey)
		{
			MacAddr = CAM_CONST_ADDR[KeyIndex];
			EntryId = KeyIndex;
		}
		else
		{
			if(IsGroup)
			{
				MacAddr = CAM_CONST_BROAD;
				EntryId = KeyIndex;
			}
			else
			{
				//Added by Mars because of the pairwisekey id is 0
				//we use the EnteryId for the pairwise key in  the Entry 4
				//because the  architecture of 8190 the entry 0~3 won't
				//record the address so we can't put the key in default 0~3
				KeyIndex = 0;
				EntryId = 4;
				IsPairWise = _TRUE;
			}
		}

		if(pSecInfo->dot11DefKeylen[KeyIndex] ==0 )
		{ 
			// Clear Key
			CamDeleteOneEntry( padapter, pMacAddr, KeyIndex);
		}
		else
		{
			//DEBUG_ERR(("The TKIP insert KEY length is %d\n",pSecInfo->KeyLen[PAIRWISE_KEYIDX]));
			//DEBUG_ERR(("The TKIP insert KEY  is %x %x \n",pSecInfo->KeyBuf[0][0],pSecInfo->KeyBuf[0][1]));
			//DEBUG_ERR(("Pairwiase Key content :",pSecInfo->PairwiseKey, pSecInfo->KeyLen[PAIRWISE_KEYIDX]);
			if(IsPairWise){
				CamAddOneEntry(padapter, 
						MacAddr, 
						KeyIndex, 
						EntryId,
						EncAlgo, 
						CAM_CONFIG_NO_USEDK,
						pSecInfo->dot11DefKey[KeyIndex].skey);
			}else{
				CamAddOneEntry(padapter, 
						MacAddr, 
						KeyIndex, 
						EntryId,
						EncAlgo, 
						CAM_CONFIG_NO_USEDK,
						pSecInfo->dot11DefKey[EntryId].skey);			
			}

			#if 0
			//Set the AES Soft key in Ad hoc mode
			if(NeedAESSoftkEY){
				AESCCMP_BLOCK   blockKey;
				PlatformMoveMemory(blockKey.x, pSecInfo->KeyBuf[EntryId],16);
				AES_SetKey(blockKey.x,
					AESCCMP_BLK_SIZE*8,
					(u4Byte *)padapter->MgntInfo.SecurityInfo.AESKeyBuf[0]);     // run the key schedule
			}
			#endif
		}
	}
	
}


//
//Note : SecClearAllKeys() need to Used before SecInit();
//          Because Clear CAM Key will check SecInfo Key Buffer Length 
//		If Length = 0, CAM entey will "NO" Clear ....

VOID
SecClearAllKeys(_adapter *padapter)
{
	//u8 i = 0;
	struct security_priv *psecpriv = &padapter->securitypriv;

	memset(psecpriv->dot118021XGrptxmickey.skey, 0, 16);
	memset(psecpriv->dot118021XGrprxmickey.skey, 0, 16);
	//pSec->DefaultTransmitKeyIdx = 0;
	SetKey819xUsb(padapter, 0, 0, 0, 0, 0, _TRUE);

#if 0 
	//
	// Clear keys used for RSNA IBSS.
	//
	for(i = 0; i < MAX_NUM_PER_STA_KEY; i++)
	{
		PlatformZeroMemory( &(pSec->PerStaDefKeyTable[i]), sizeof(PER_STA_DEFAULT_KEY_ENTRY) ); 
		PlatformZeroMemory( &(pSec->MAPKEYTable[i]), sizeof(PER_STA_MPAKEY_ENTRY) ); 
	}
#endif

}


VOID
SecSetSwEncryptionDecryption(
	PADAPTER			Adapter,
	BOOLEAN				bSWTxEncrypt,
	BOOLEAN				bSWRxDecrypt
	)
{
#if 0
	struct security_priv *pSec = &Adapter->securitypriv;
	// Tx Sw Encryption.
	if ( (pSec->RegSWTxEncryptFlag == EncryptionDecryptionMechanism_Auto) ||
		(pSec->RegSWTxEncryptFlag >= EncryptionDecryptionMechanism_Undefined) )
	{
		pSec->SWTxEncryptFlag = bSWTxEncrypt;
	}
	else if ( pSec->RegSWTxEncryptFlag == EncryptionDecryptionMechanism_Hw )
	{
		pSec->SWTxEncryptFlag = FALSE;
	}
	else if ( pSec->RegSWTxEncryptFlag == EncryptionDecryptionMechanism_Sw )
	{
		pSec->SWTxEncryptFlag = TRUE;
	}

	// Rx Sw Decryption.
	if ( (pSec->RegSWRxDecryptFlag == EncryptionDecryptionMechanism_Auto) ||
		(pSec->RegSWRxDecryptFlag == EncryptionDecryptionMechanism_Undefined) )
	{
		pSec->SWRxDecryptFlag = bSWRxDecrypt;
	}
	else if ( pSec->RegSWRxDecryptFlag == EncryptionDecryptionMechanism_Hw )
	{
		pSec->SWRxDecryptFlag = FALSE;
	}
	else if ( pSec->RegSWRxDecryptFlag == EncryptionDecryptionMechanism_Sw)
	{
		pSec->SWRxDecryptFlag = TRUE;
	}

	// If the decryption mechanism differs between driver determined and registry determined.
	// Print an warning message.
	if (pSec->RegSWRxDecryptFlag != bSWRxDecrypt)
	{
		RT_TRACE(COMP_SEC, DBG_WARNING,
			("SecSetSwEncryptionDecryption(): Warning! User and driver determined decryption mechanism mismatch.\n"));
		RT_TRACE(COMP_SEC, DBG_WARNING, 
			("SecSetSwEncryptionDecryption(): RegSWRxDecryptFlag = %d, bSWRxDecrypt = %d\n",
			pSec->RegSWRxDecryptFlag, bSWRxDecrypt));
	}
#endif
}


//
//Note : SecInit() need to Used after SecClearAllKeys();
//          Because Clear CAM Key will check SecInfo Key Buffer Length 
//		If Length = 0, CAM entey will "NO" Clear ....

VOID
SecInit(
	PADAPTER	padapter
	)
{
	struct security_priv *pSec = &padapter->securitypriv;

//	s1Byte	i = 0;
	
	pSec->dot11PrivacyAlgrthm = _AES_;
	SecSetSwEncryptionDecryption(padapter, _FALSE, _FALSE);
#if 0
	pSec->dot11AuthAlgrthm= RT_802_11AuthModeWPA2;
	pSec->EncryptionStatus = RT802_11Encryption3KeyAbsent;
	pSec->EncryptionStatus = RT802_11EncryptionDisabled;
	pSec->TxIV = 0;
	pSec->GroupRxIV = 0;

	// 2007.08.08 We must initialize this value, otherwise, software decryption
	// without initilization of AES mode cause Assertion in Vista. By Lanhsin
	pSec->AESCCMPMicLen = 8;
	
	PlatformZeroMemory(pSec->RSNIEBuf, MAXRSNIELEN);
	pSec->RSNIE.Length = 0;
	FillOctetString(pSec->RSNIE, pSec->RSNIEBuf, pSec->RSNIE.Length);
	pSec->SecLvl = RT_SEC_LVL_WPA2;

	//2004/09/07, kcwu, initialize key buffer
	for(i = 0;i<KEY_BUF_SIZE; i++){
		PlatformZeroMemory(pSec->AESKeyBuf[i], MAX_KEY_LEN);
		PlatformZeroMemory(pSec->KeyBuf[i], MAX_KEY_LEN);
		pSec->KeyLen[i] = 0;
	}

	//kcwu_todo: Clear key buffer

	//Point Pairwise key to appropriate index
	pSec->PairwiseKey = pSec->KeyBuf[PAIRWISE_KEYIDX];
	pSec->UseDefaultKey = FALSE;

	// WPA2 Pre-Authentication related. Added by Annie, 2006-05-07.
	pSec->EnablePreAuthentication = pSec->RegEnablePreAuth;		// Added by Annie, 2005-02-15.
	PlatformZeroMemory(pSec->PMKIDList, sizeof(RT_PMKID_LIST)*NUM_PMKID_CACHE);
	for( i=0; i<NUM_PMKID_CACHE; i++ )
	{
		FillOctetString( pSec->PMKIDList[i].Ssid, pSec->PMKIDList[i].SsidBuf, 0 );
	}

	PlatformZeroMemory(pSec->szCapability, 256);

	init_crc32();

	// Initialize TKIP MIC error related, 2004.10.06, by rcnjko.
	pSec->LastPairewiseTKIPMICErrorTime = 0;	
	pSec->bToDisassocAfterMICFailureReportSend = FALSE;
	for(i = 0; i < MAX_DENY_BSSID_LIST_CNT; i++)
	{
		pSec->DenyBssidList[i].bUsed = FALSE;
	}
#endif
	
}


void GetTxPowerOriginalOffset(_adapter *padapter)
{
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;

	peeprompriv->MCSTxPowerLevelOriginalOffset[0] =
		PlatformEFIORead4Byte(padapter, MCS_TXAGC);
	peeprompriv->MCSTxPowerLevelOriginalOffset[1] =
		PlatformEFIORead4Byte(padapter, (MCS_TXAGC+4));
	peeprompriv->CCKTxPowerLevelOriginalOffset =
		PlatformEFIORead4Byte(padapter, CCK_TXAGC);
}


RT_STATUS
PHY_ConfigMACWithHeaderFile(
	IN	PADAPTER		Adapter
)
{
	u4Byte	i;
	u4Byte	ArrayLength = 0;
	pu4Byte	ptrArray;


	//RT_TRACE(COMP_SEND, DBG_LOUD, ("The MAC PHY Array is %d \n",sizeof(Rtl8190MACPHY_Array)));
#if 0
	if(Adapter->bInHctTest)
	{
		DEBUG_INFO(("Rtl819XMACPHY_ArrayDTM\n"));
		ArrayLength = MACPHY_ArrayLengthDTM;
		ptrArray = Rtl819XMACPHY_ArrayDTM;
	}
	else if(pHalData->bTXPowerDataReadFromEEPORM)
	{
#endif
		DEBUG_INFO(("Rtl819XMACPHY_Array_PG\n"));
		ArrayLength = MACPHY_Array_PGLength;
		ptrArray = Rtl819XMACPHY_Array_PG;
#if 0

	}else
	{
		DEBUG_INFO( ("Read Rtl819XMACPHY_Array\n"));
		ArrayLength = MACPHY_ArrayLength;
		ptrArray = Rtl819XMACPHY_Array;	
	}
#endif
	
	for(i = 0 ;i < ArrayLength;i=i+3){
//		DEBUG_INFO(("Write %lx,%lx,%lx\n",	(long unsigned int)ptrArray[i], (long unsigned int)ptrArray[i+1],(long unsigned int)ptrArray[i+2]));
		PHY_SetBBReg(Adapter, ptrArray[i], ptrArray[i+1], ptrArray[i+2]); 
//		DEBUG_INFO(("Read %lx\n",	read32(Adapter, (long unsigned int)ptrArray[i])));
		
	}
	return RT_STATUS_SUCCESS;
}


VOID
ActSetWirelessMode819xUsb(_adapter *padapter, u8 btWirelessMode)
{
	
	
	struct mib_info* _sys_mib = &(padapter->_sys_mib);
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	pHTInfo->bEnableHT = _TRUE;
	
	if(btWirelessMode !=_sys_mib->network_type )
	{
		DEBUG_ERR(("WARNING: ActSetWirelessMode819xUsb :btWirelessMode !=_sys_mib->network_type  "));
		_sys_mib->network_type = btWirelessMode;
	}
	
	// TODO: this function doesn't work well at this time, we shoud wait for FPGA
	ActUpdateChannelAccessSetting_8192( padapter, btWirelessMode );

	#if 0
	if(pAdapter->MgntInfo.dot11CurrentWirelessMode==WIRELESS_MODE_N_24G ||
		pAdapter->MgntInfo.dot11CurrentWirelessMode==WIRELESS_MODE_N_5G)
		pAdapter->MgntInfo.pHTInfo->bEnableHT = TRUE;
	else
		pAdapter->MgntInfo.pHTInfo->bEnableHT = FALSE;
	#endif
	DEBUG_ERR(("========CurrentWirelessMode = %x\n",_sys_mib->network_type));

	//Refresh Data Rate
	if(_sys_mib->network_type == WIRELESS_11N_2G4 ||_sys_mib->network_type == WIRELESS_11N_5G)
	{
		pHTInfo->dot11HTOperationalRateSet[0] = 0xFF;	//support MCS 0~7
		pHTInfo->dot11HTOperationalRateSet[1] = 0xFF;	//support MCS 8~15
		//pHTInfo->dot11HTOperationalRateSet[4] = 0x01;	//support MCS 32

	}
	
		
	else
	{
		// pHTInfo->bEnableHT = _FALSE;
		memset(pHTInfo->dot11HTOperationalRateSet,0, 16);
	}

}


void rtl8192u_start(_adapter *padapter){

	u8 val8 = 0;
	u32 val32 = 0;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;
	struct mib_info* _sys_mib = &(padapter->_sys_mib);

	// ASIC power on sequence	
	DEBUG_INFO(("\nASIC power on sequence\n"));
	write8(padapter,0xfe5f,0x80);
	msleep_os(50 );

	write8(padapter, 0xfe5f, 0xf0);
	write8(padapter, 0xfe5d, 0x00);
	write8(padapter, 0xfe5e, 0x80);
	write8(padapter, 0x17, 0x37);
	msleep_os(10 );


	//2 Config CPUReset Register

	DEBUG_INFO(("\nFirmware Reset Or Not\n"));
	//3 Firmware Reset Or Not
	{
		u32 cpugen;
		cpugen = read32(padapter, CPU_GEN);

//		if(padapter->halpriv.FirmwareStatus == FW_STATUS_0_INIT)
//		{
			DEBUG_INFO(("rtl8192u_start: FW_STATUS_0_INIT\n"));		
			//called from MPInitialized. do nothing
			cpugen |= CPU_GEN_SYSTEM_RESET;
//		}
//		else if(padapter->halpriv.FirmwareStatus == FW_STATUS_5_READY)
//		{
//			DEBUG_INFO(("rtl8192u_start: FW_STATUS_5_READY\n"));				
//			cpugen |= CPU_GEN_FIRMWARE_RESET;	// Called from MPReset
//		}
//		else
//		{
//			DEBUG_INFO(("rtl8192u_start: undefined firmware state\n"));
//		}
		
		write32(padapter, CPU_GEN, cpugen);
	}

#if 1
	//	2 Config BB
	// 	Write here
	//		Rtl819XAGCTAB_Array
	//		Rtl819XPHY_REG_1T2RArray
	val8  = PHY_BBConfig819xUsb(padapter);
	if(val8 != RT_STATUS_SUCCESS)
	{
		DEBUG_ERR(("BB Config failed\n"));
		return;
	}
#endif

	//3Loopback mode or not
	//	Note!We should not merge these two CPU_GEN register writings
	//	because setting of System_Reset bit reset MAC to default transmission mode.
	val32 = read32(padapter, CPU_GEN);	
	DEBUG_INFO(  (" Loopback mode = No Loopback! \n") );
	val32 = ((val32 & CPU_GEN_NO_LOOPBACK_MSK) | CPU_GEN_NO_LOOPBACK_SET);
#if 0// For Loopback mode
	val32 |= CPU_CCK_LOOPBACK;
#endif
	write32(padapter, CPU_GEN, val32);

	//	After reset cpu, we sholud wait for a second, otherwise, it may fail to write registers.
	udelay_os(500);

	//	add for new bitfile: usb suspend reset pin set 1
	write8(padapter, 0xfe5f, (read8(padapter, 0xfe5f)|0x20));
	//msleep_os(1);

	//	Set Hardware
	Hal_HwConfigureRTL819xUsb(padapter);

	//=======================================================
	//	Common Setting for all of the FPGA platform. (part 1)
	//=======================================================
	//	If there is changes, please make sure it applies to all of the FPGA version
	//	Turn on Tx/Rx
	//	PlatformEFIOWrite1Byte(Adapter, CMDR, CR_RST);
	write8(padapter, CMDR, CR_RE|CR_TE);

	// ACM related setting.
	write8(padapter, ACMHWCONTROL, 0);

	DEBUG_INFO(  (" Hal_SetHwReg819xUsb HW_VAR_ETHER_ADDR\n") );
	// We must set address, otherwise, IDR0 will not contain MAC addr.
	Hal_SetHwReg819xUsb( padapter, HW_VAR_ETHER_ADDR, padapter->eeprompriv.mac_addr);

	//=======================================================
	//	Seperated Setting applied for  different platform
	//=======================================================
	// Set RCR
	padapter->dvobjpriv.hw_info.rcr=	RCR_AMF |RCR_ADF |//accept management/data
								// RCR_AAP | 	
								//guangan200710
								//RCR_ACF |						//accept control frame for SW AP needs PS-poll, 2005.07.07, by rcnjko.
								RCR_AB | RCR_AM | RCR_APM |		//accept BC/MC/UC
								RCR_AICV |// RCR_ACRC32 | 		//accept ICV/CRC error packet
								((u4Byte)7<<RCR_MXDMA_OFFSET) | // Max DMA Burst Size per Rx DMA Burst, 7: unlimited.
								(7<<RCR_FIFO_OFFSET) | // Rx FIFO Threshold, 7: No Rx threshold.
								(RCR_OnlyErlPkt);

	
	write32(padapter, RCR, padapter->dvobjpriv.hw_info.rcr);

	val32=read32(padapter, RCR);
	DEBUG_INFO(("\n RTL8192U init RCR:%x != REG(%X)\n",padapter->dvobjpriv.hw_info.rcr, val32));


	//2 Initialize Number of Reserved Pages in Firmware Queue
	write32(padapter, RQPN1,	NUM_OF_PAGE_IN_FW_QUEUE_BK << RSVD_FW_QUEUE_PAGE_BK_SHIFT |\
						NUM_OF_PAGE_IN_FW_QUEUE_BE << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
						NUM_OF_PAGE_IN_FW_QUEUE_VI << RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
						NUM_OF_PAGE_IN_FW_QUEUE_VO <<RSVD_FW_QUEUE_PAGE_VO_SHIFT);												
	write32(padapter, RQPN2, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT |\
						NUM_OF_PAGE_IN_FW_QUEUE_CMD << RSVD_FW_QUEUE_PAGE_CMD_SHIFT);
	write32(padapter, RQPN3, APPLIED_RESERVED_QUEUE_IN_FW| \
						NUM_OF_PAGE_IN_FW_QUEUE_BCN<<RSVD_FW_QUEUE_PAGE_BCN_SHIFT
	//						| NUM_OF_PAGE_IN_FW_QUEUE_PUB<<RSVD_FW_QUEUE_PAGE_PUB_SHIFT
						);

	//Set Response Rate Setting Register
	write32(padapter, RATR0+4*7, (RATE_ALL_OFDM_AG | RATE_ALL_CCK));

	//2Set AckTimeout
	// TODO: (it value is only for FPGA version). need to be changed!!2006.12.18, by Emily
	write8(padapter, ACK_TIMEOUT, 0x30);
	
	// Default : WIRELESS_11BG mode 
	// Update Channel setting
	ActSetWirelessMode819xUsb(padapter,_sys_mib->network_type);


	//3 Security related
	{
		SecClearAllKeys(padapter);  
		CamResetAllEntry(padapter);
		SecInit(padapter);
		write8(padapter, SECR, 0xf);  
	}


	//=======================================================
	//	Common Setting for all of the FPGA platform. (part 2)
	//=======================================================
	//	Beacon related
	write16(padapter, ATIMWND, 2);
	write16(padapter, BCNITV, 100);

#ifdef USB_RX_AGGREGATION_SUPPORT
	//3 For usb rx firmware aggregation control
//	if(Adapter->ResetProgress == RESET_TYPE_NORESET)	
	{

		// TODO: turn on or turn off Rx Aggregation function according to current traffic 
		// Always set Rx Aggregation to Disable if connected AP is Realtek AP, by Joseph
		// Alway set Rx Aggregation to Disable if current platform is Win2K USB 1.1, by Emily
		// set Rx Aggregation to Disable if using AES encryption/decryption temporarily, by Guangan
		
		//Set Rx Aggregation to ON by Default
		Hal_SetHwReg819xUsb(padapter, HW_VAR_USB_RX_AGGR, 1);		
	}
#endif


	//	Write Rtl819XMACPHY_Array_PG here
	PHY_ConfigMACWithHeaderFile(padapter);

	//msleep_os(100);
	if(peeprompriv->VersionID == VERSION_819xUsb_A)
	{

		DEBUG_ERR(("Set TX Power , current channel : %d\n", _sys_mib->cur_channel));
		// cosa add for tx power level initialization.
		GetTxPowerOriginalOffset(padapter);
		SetTxPowerLevel819xUsb(padapter, _sys_mib->cur_channel );
	}
	mdelay_os(1);

	//=======================================================
	//	Firmware download
	//=======================================================
	// We must download firmware before all of the hardware related setting are done. Emily
#if 1
	if(Hal_FWDownload(padapter) != RT_STATUS_SUCCESS)
	{
		DEBUG_ERR(("Download Firmware failed\n"));
	}
#endif

#if 1
	DEBUG_INFO(("PHY_RF8256_Config\n"));

	//	Write here
	//		Rtl819XRadioA_Array
	//		Rtl819XRadioB_Array
	phy_RF8256_Config_ParaFile(padapter);
#endif

#if 1
	/*---- Set CCK and OFDM Block "ON"----*/
	DEBUG_INFO(("Set CCK and OFDM Block ON\n"));
	PHY_SetBBReg(padapter, rFPGA0_RFMOD, bCCKEn, 0x1);
	PHY_SetBBReg(padapter, rFPGA0_RFMOD, bOFDMEn, 0x1);
#endif

	// For packet detection threshold, jordan
	write8(padapter, rOFDM0_RxDetector1,0x4f);
	write8(padapter, rOFDM0_XATxAFE+3,0xf0);

//	MgntActSet_RF_State(padapter, eRfOff, pMgntInfo->RfOffReason);
	DEBUG_INFO(("RTL819X_TOTAL_RF_PATH\n"));

	write8(padapter, 0x87, 0x0);

//	msleep_os(10);

	return;
}

RT_STATUS
phy_RF8256_Config_ParaFile(
	IN	PADAPTER		Adapter
	)
{
	u4Byte					u4RegValue=0;
	u1Byte					eRFPath;
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;

	BB_REGISTER_DEFINITION_T	*pPhyReg;


	u4Byte	RF3_Final_Value = 0;
	u1Byte	RetryTimes = 5;

	//3//-----------------------------------------------------------------
	//3// <2> Initialize RF
	//3//-----------------------------------------------------------------
	for(eRFPath = (RF90_RADIO_PATH_E)RF90_PATH_A;eRFPath<RTL819X_TOTAL_RF_PATH; eRFPath++)
	{
		if(eRFPath==RF90_PATH_C||eRFPath==RF90_PATH_D)
			continue;

		pPhyReg=&(Adapter->eeprompriv.PHYRegDef[eRFPath]);

		// Joseph test for shorten RF config
		//pHalData->RfReg0Value[eRFPath] =  PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, rGlobalCtrl, bMaskDWord);
		
		/*----Store original RFENV control type----*/
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			u4RegValue = PHY_QueryBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			u4RegValue = PHY_QueryBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV<<16);
			break;
		}

		/*----Set RF_ENV enable----*/		
		PHY_SetBBReg(Adapter, pPhyReg->rfintfe, bRFSI_RFENV<<16, 0x1);
		
		/*----Set RF_ENV output high----*/
		PHY_SetBBReg(Adapter, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);

		/* Set bit number of Address and Data for RF register */
		PHY_SetBBReg(Adapter, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0); 	// Set 0 to 4 bits for Z-serial and set 1 to 6 bits for 8258
		PHY_SetBBReg(Adapter, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0);	// Set 0 to 12 bits for Z-serial and 8258, and set 1 to 14 bits for ???

//           	PHY_SetRFReg(Adapter, eRFPath, 0x0, bMask12Bits, 0xbf);

        	PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E) eRFPath, 0x0, bMask12Bits, 0xbf);


		/*----Check RF block (for FPGA platform only)----*/
		// TODO: this function should be removed on ASIC , Emily 2007.2.2
		rtStatus  = PHY_CheckBBAndRFOK(Adapter, HW90_BLOCK_RF, (RF90_RADIO_PATH_E)eRFPath); 
		if(rtStatus!= RT_STATUS_SUCCESS)
		{
			DEBUG_ERR(("PHY_RF8256_Config():Check BBAndRFOK for Radio[%d] Fail!!\n", eRFPath));
			goto phy_RF8256_Config_ParaFile_Fail;
		}

		RF3_Final_Value = 0;
		RetryTimes = 5;
		
		/*----Initialize RF fom connfiguration file----*/
		switch(eRFPath)
		{
		case RF90_PATH_A:
			while(RF3_Final_Value!=0x7f1 && RetryTimes!=0)
			{
#ifdef	CONFIG_EMBEDDED_FWIMG
				rtStatus= PHY_ConfigRFWithHeaderFile(Adapter,(RF90_RADIO_PATH_E)eRFPath);
#else
				rtStatus = PHY_ConfigRFWithParaFile(Adapter, (ps1Byte)&szRadioAFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
				RF3_Final_Value = PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x3, bMask12Bits);
				DEBUG_INFO( ("RF %d 0 register final value: %x\n", eRFPath,RF3_Final_Value));
				RetryTimes--;
			}	
			break;
		case RF90_PATH_B:
			while(RF3_Final_Value!=0x7f1 && RetryTimes!=0)
			{
#ifdef	CONFIG_EMBEDDED_FWIMG
				rtStatus= PHY_ConfigRFWithHeaderFile(Adapter,(RF90_RADIO_PATH_E)eRFPath);
#else
				rtStatus = PHY_ConfigRFWithParaFile(Adapter, (ps1Byte)&szRadioBFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
				RF3_Final_Value = PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x3, bMask12Bits);
				DEBUG_INFO( ("RF %d 0 register final value: %x\n", eRFPath, RF3_Final_Value));
				RetryTimes--;
			}
			break;
		case RF90_PATH_C:
			while(RF3_Final_Value!=0x7f1 && RetryTimes!=0)
			{
#ifdef	CONFIG_EMBEDDED_FWIMG
				rtStatus= PHY_ConfigRFWithHeaderFile(Adapter,(RF90_RADIO_PATH_E)eRFPath);
#else
				rtStatus = PHY_ConfigRFWithParaFile(Adapter, (ps1Byte)&szRadioCFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
				RF3_Final_Value = PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x3, bMask12Bits);
				DEBUG_INFO( ("RF %d 0 register final value: %x\n", eRFPath, RF3_Final_Value));
				RetryTimes--;
			}				
			break;
		case RF90_PATH_D:
			while(RF3_Final_Value!=0x7f1 && RetryTimes!=0)
			{
#ifdef	CONFIG_EMBEDDED_FWIMG
				rtStatus= PHY_ConfigRFWithHeaderFile(Adapter,(RF90_RADIO_PATH_E)eRFPath);
#else
				rtStatus = PHY_ConfigRFWithParaFile(Adapter, (ps1Byte)&szRadioDFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
				RF3_Final_Value = PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x3, bMask12Bits);
				DEBUG_INFO(("RF %d 0 register final value: %x\n", eRFPath, RF3_Final_Value));
				RetryTimes--;
			}				
			break;
		}

		/*----Restore RFENV control type----*/;
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			PHY_SetBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV, u4RegValue);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			PHY_SetBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV<<16, u4RegValue);
			break;
		}
	
		if(rtStatus != RT_STATUS_SUCCESS){
			DEBUG_ERR(("phy_RF8256_Config_ParaFile():Radio[%d] Fail!!", eRFPath));
			goto phy_RF8256_Config_ParaFile_Fail;
		}
			
	}

	DEBUG_INFO(("PHY Initialization Success\n") );
	return rtStatus;
	
phy_RF8256_Config_ParaFile_Fail:	
	return rtStatus;
}

/**
* Function:	PHY_QueryRFReg
*
* OverView:	Query "Specific bits" to RF register (page 8~) 
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u4Byte			RegAddr,		//The target address to be read
*			u4Byte			BitMask		//The target bit position in the target address
*										//to be read	
*
* Output:	None
* Return:		u4Byte			Readback value
* Note:		This function is equal to "GetRFRegSetting" in PHY programming guide
*/
u4Byte
PHY_QueryRFReg(
	IN	PADAPTER			Adapter,
	IN	u1Byte eRFPath,
	IN	u4Byte				RegAddr,
	IN	u4Byte				BitMask
	)
{
	u4Byte	Original_Value, Readback_Value, BitShift;
	
	if(eRFPath == RF90_PATH_A || eRFPath == RF90_PATH_B)
			{
		Original_Value = phy_RFSerialRead(Adapter, eRFPath, RegAddr);
   		BitShift =  phy_CalculateBitShift(BitMask);
	   	Readback_Value = (Original_Value & BitMask) >> BitShift;

		return (Readback_Value);
	}
	else
	{
		return 0;
	}
}


/*-----------------------------------------------------------------------------
 * Function:    PHY_ConfigRFWithHeaderFile()
 *
 * Overview:    This function read RF parameters from general file format, and do RF 3-wire
 *
 * Input:      	PADAPTER			Adapter
 *			ps1Byte 				pFileName			
 *			RF90_RADIO_PATH_E	eRFPath
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note:		Delay may be required for RF configuration
 *---------------------------------------------------------------------------*/
RT_STATUS
PHY_ConfigRFWithHeaderFile(
	IN	PADAPTER			Adapter,
	RF90_RADIO_PATH_E		eRFPath
)
{
	int				i;
	RT_STATUS			rtStatus;

	rtStatus = RT_STATUS_SUCCESS;
	switch(eRFPath){
		case RF90_PATH_A:
			for(i = 0;i<RadioA_ArrayLength; i=i+2){
				
				if(Rtl819XRadioA_Array[i] == 0xfe){
						mdelay_os(100);
						continue;
				}
				PHY_SetRFReg(Adapter, eRFPath, Rtl819XRadioA_Array[i], bMask12Bits, Rtl819XRadioA_Array[i+1]);
				mdelay_os(1);
#if 0//cosa

						if(Rtl819XRadioA_Array[i] == 0x19 ||  Rtl819XRadioA_Array[i] == 0x00)
						{
							u4Byte tmpReg, nRetryCnt = 10;
						
							do
							{
								tmpReg = PHY_QueryRFReg(Adapter, eRFPath, Rtl819XRadioA_Array[i], bMask12Bits);
								DEBUG_INFO( ("RF reg %x, WriteValue: %x, ReadValue: %x\n", 
										Rtl819XRadioA_Array[i], Rtl819XRadioA_Array[i+1], tmpReg));

								if( (tmpReg&0xff8)!=(Rtl819XRadioA_Array[i+1]&0xff8) )
									PHY_SetRFReg(Adapter, eRFPath, Rtl819XRadioA_Array[i], bMask12Bits, Rtl819XRadioA_Array[i+1]);
								else{
									rtStatus = RT_STATUS_FAILURE;
									break;
								}	
								nRetryCnt--;
								//mdelay_os(1);
							}while(nRetryCnt > 0);
						}
#endif
				
			}			
			break;
		case RF90_PATH_B:
			for(i = 0;i<RadioB_ArrayLength; i=i+2){
				
				if(Rtl819XRadioB_Array[i] == 0xfe){
						mdelay_os(100);
						continue;
				}
				PHY_SetRFReg(Adapter, eRFPath, Rtl819XRadioB_Array[i], bMask12Bits, Rtl819XRadioB_Array[i+1]);
				mdelay_os(1);
#if 0
						if(Rtl819XRadioB_Array[i] == 0x19 ||  Rtl819XRadioB_Array[i] == 0x00)
						{
							u4Byte tmpReg, nRetryCnt = 10;
						
							do
							{
								tmpReg = PHY_QueryRFReg(Adapter, eRFPath, Rtl819XRadioB_Array[i], bMask12Bits);
								DEBUG_INFO( ("RF reg %x, WriteValue: %x, ReadValue: %x\n", 
										Rtl819XRadioB_Array[i], Rtl819XRadioB_Array[i+1], tmpReg));

								if( (tmpReg&0xff8)!=(Rtl819XRadioB_Array[i+1]&0xff8) )
									PHY_SetRFReg(Adapter, eRFPath, Rtl819XRadioB_Array[i], bMask12Bits, Rtl819XRadioB_Array[i+1]);
								else{
									rtStatus = RT_STATUS_FAILURE;
									break;
								}	
								nRetryCnt--;
				mdelay_os(1);
							}while(nRetryCnt > 0);
						}
#endif
			}			
			break;
		default:
			break;
	}
	
	return RT_STATUS_SUCCESS;;

}

//-----------------------------------------------------------------------------
// Procedure:    Init boot code/firmware code/data session
//
// Description:   This routine will intialize firmware. If any error occurs during the initialization 
//				process, the routine shall terminate immediately and return fail.
//				
//				/**NOTES**/A NIC driver should call NdisOpenFile only from MiniportInitialize.	

// Arguments:   The pointer of the adapter
//        
// Returns:
//        NDIS_STATUS_FAILURE - the following initialization process should be terminated
//        NDIS_STATUS_SUCCESS - if firmware initialization process success
//-----------------------------------------------------------------------------
RT_STATUS Hal_FWDownload(IN PADAPTER Adapter)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;

	FIRMWARE_STATUS		*pfwstatus = &(Adapter->halpriv.FirmwareStatus);

	ULONG					ulInitStep = 0;
	FIRMWARE_OPERATION_STATE_E 	eRstOpt = OPT_SYSTEM_RESET;
	FIRMWARE_INIT_STEP_E 	eStartingState=FW_INIT_STEP0_BOOT;

	DEBUG_INFO((" PlatformInitFirmware()==>\n") );

//	if (*pfwstatus == FW_STATUS_0_INIT )
//	{
		// it is called by MPReset(Distatch Level)
		eRstOpt = OPT_SYSTEM_RESET;
		eStartingState = FW_INIT_STEP0_BOOT;
		// TODO: system reset
		
//	}else if(*pfwstatus == FW_STATUS_5_READY) 
//	{
		// it is called by MPInitialize(Passive Level)
//		eRstOpt = OPT_FIRMWARE_RESET;
//		eStartingState = FW_INIT_STEP2_DATA;
//	}else
//		DEBUG_ERR(("PlatformInitFirmware: undefined firmware state\n"));




	//------------------------------------------------------------------
	//Download boot, main, and data image for System reset. 
	//Download data image for firmware reset
	//------------------------------------------------------------------

	for(ulInitStep = eStartingState; ulInitStep <=FW_INIT_STEP2_DATA; ulInitStep++)
	{
		//3//
		//3 //<1> Open Image file, and map file to contineous memory if open file success.
		//3  //        or read image file from array. Default load from IMG file
		//3//
#ifdef CONFIG_EMBEDDED_FWIMG
			DEBUG_INFO(("%s(%d): Getting the %d_th firmware image...\n",__FUNCTION__,__LINE__, ulInitStep));
//			pucMappedFile = FwImageBuf[ulInitStep];
//			ulFileLength = szFwImageLen[ulInitStep];
#endif
		
		//3//
		//3// <2> Download image file 
		//3// 
		DEBUG_INFO(("%s(%d): Start to download firmware\n",__FUNCTION__,__LINE__));
		rtStatus = Hal_fwSendDownloadCode(Adapter, ulInitStep);
	

		if(rtStatus != RT_STATUS_SUCCESS)
		{
			DEBUG_INFO(("fwSendDownloadCode() fail ! \n") );
			goto N5CDownloadFirmware_Fail;
		}
		
		switch(ulInitStep)
		{
			case FW_INIT_STEP0_BOOT:
				// Download boot
				// initialize command descriptor.  will set polling bit when firmware code is also configured
				*pfwstatus = FW_STATUS_1_MOVE_BOOT_CODE;		
				break;

			case FW_INIT_STEP1_MAIN:
				//Download firmware code. Wait until Boot Ready and Turn on CPU
				*pfwstatus = FW_STATUS_2_MOVE_MAIN_CODE;

				//Check Put Code OK and Turn On CPU
				rtStatus = Hal_CPUCheckMainCodeOKAndTurnOnCPU(Adapter);
				if(rtStatus != RT_STATUS_SUCCESS)	
				{
					DEBUG_INFO(("CPUCheckMainCodeOKAndTurnOnCPU() fail ! \n") );
					goto N5CDownloadFirmware_Fail;
				}
				
				DEBUG_ERR(( "\n Exception Register (reg0xF3) = 0x%x\n", read8(Adapter,0xF3)));

#if 0
				{
					int i;
					u4Byte	reg_value;
					for(i=0; i<20; i++)
					{
						msleep_os(20);
						reg_value = PlatformEFIORead4Byte(Adapter, 0x120);
						DEBUG_ERR(( "\n FwCPUCnt(reg0x120) = 0x%x", reg_value));
						msleep_os(20);
					}
				}
#endif
				
				*pfwstatus = FW_STATUS_3_TURNON_CPU;
				break;

			case FW_INIT_STEP2_DATA:
				//Download initial data code
				*pfwstatus = FW_STATUS_4_MOVE_DATA_CODE;
				mdelay_os(1);
		
				rtStatus = Hal_CPUCheckFirmwareReady(Adapter);
				if(rtStatus != RT_STATUS_SUCCESS)				
				{
					u1Byte	i;
					u4Byte	reg_value;
					
								for(i=0; i<20; i++)
					{
						udelay_os(50);
						reg_value = PlatformEFIORead4Byte(Adapter, 0x120);
						DEBUG_ERR(( "\n FwCPUCnt(reg0x120) = 0x%x", reg_value));
						udelay_os(50);
					}
					if(rtStatus != RT_STATUS_SUCCESS)				
					{
						DEBUG_INFO(("CPUCheckFirmwareReady() fail ! \n") );
					goto N5CDownloadFirmware_Fail;
				}
				}
				
				//wait until data code is initialized ready.
				*pfwstatus = FW_STATUS_5_READY;
				break;
				
		}
				
	}

	DEBUG_INFO(("Firmware Download Success\n") );
	DEBUG_INFO((" <== PlatformDownloadFirmware()\n") );

	//mdelay_os(100);


	return rtStatus;	
	
N5CDownloadFirmware_Fail:	
	//RT_ASSERT(FALSE, ("Firmware Download Fail\n"));
	DEBUG_INFO(("Firmware Download Fail\n") );
	DEBUG_INFO((" <== PlatformInitFirmware()\n") );

	_mfree((u8*)TxfwBootFrame,0);
	_mfree((u8*)TxfwMainFrame,0);
	_mfree((u8*)TxfwDataFrame,0);
	
	rtStatus = RT_STATUS_FAILURE;
	return rtStatus;

}


//-----------------------------------------------------------------------------
// Procedure:    Check whether main code is download OK. If OK, turn on CPU
//
// Description:   CPU register locates in different page against general register. 
//			    Switch to CPU register in the begin and switch back before return				
//				
//				
// Arguments:   The pointer of the adapter
//        
// Returns:
//        NDIS_STATUS_FAILURE - the following initialization process should be terminated
//        NDIS_STATUS_SUCCESS - if firmware initialization process success
//-----------------------------------------------------------------------------

RT_STATUS Hal_CPUCheckMainCodeOKAndTurnOnCPU(IN PADAPTER Adapter)
{

	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	int			iCheckPutCodeOKTime = 120000000, iCheckBootOkTime = 120000000;
	u4Byte	 	ulCPUStatus = 0;
	
	//switch to register page for CPU
	//PlatformAcquireSpinLock(Adapter, RT_CPU_SPINLOCK);

	//1(1)Check whether put code OK
	DEBUG_INFO(("Enter main code ok check loop\n"));;
	do
	{
		ulCPUStatus = PlatformEFIORead4Byte(Adapter, CPU_GEN);

		if(ulCPUStatus&CPU_GEN_PUT_CODE_OK)
			break;
		
	}while(iCheckPutCodeOKTime--);
	DEBUG_INFO(("Exit main code ok check loop\n"));;


	if(!(ulCPUStatus&CPU_GEN_PUT_CODE_OK))
		goto CPUCheckMainCodeOKAndTurnOnCPU_Fail;

	//1(2)Turn On CPU
	ulCPUStatus = PlatformEFIORead4Byte(Adapter, CPU_GEN);
	PlatformEFIOWrite1Byte(Adapter, CPU_GEN, (u1Byte) ((ulCPUStatus|CPU_GEN_PWR_STB_CPU)&0xff)    );
	mdelay_os(1);

	//1(3)Check whether CPU boot OK
	do
	{
		ulCPUStatus = PlatformEFIORead4Byte(Adapter, CPU_GEN);

		if(ulCPUStatus&CPU_GEN_BOOT_RDY)
			break;
		
	}while(iCheckBootOkTime--);

	if(!(ulCPUStatus&CPU_GEN_BOOT_RDY))
		goto CPUCheckMainCodeOKAndTurnOnCPU_Fail;

	//PlatformReleaseSpinLock(Adapter, RT_CPU_SPINLOCK);	
	return rtStatus;


CPUCheckMainCodeOKAndTurnOnCPU_Fail:	
	//PlatformReleaseSpinLock(Adapter, RT_CPU_SPINLOCK);
	if(iCheckPutCodeOKTime == 0)
	{
		DEBUG_INFO(("Put Code Failed!! \n") );
	}
	else
	{
		DEBUG_INFO( ("CPU not ready!! \n") );
	}
	rtStatus = RT_STATUS_FAILURE;
	return rtStatus;
	
}


RT_STATUS Hal_CPUCheckFirmwareReady(IN PADAPTER Adapter)
{

	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	int			iCheckTime = 120000000;
	u4Byte	 	ulCPUStatus = 0;

	//PlatformAcquireSpinLock(Adapter, RT_CPU_SPINLOCK);
	DEBUG_INFO(("+Hal_CPUCheckFirmwareReady\n"));
	
	//1Check Firmware Ready
	do
	{
		ulCPUStatus = PlatformEFIORead4Byte(Adapter, CPU_GEN);

		if(ulCPUStatus&CPU_GEN_FIRM_RDY)
			break;
		
	}while(iCheckTime--);

	if(!(ulCPUStatus&CPU_GEN_FIRM_RDY))
		goto CPUCheckFirmwareReady_Fail;

	DEBUG_INFO(("-Hal_CPUCheckFirmwareReady\n"));

	//mdelay_os(1000);

	//PlatformReleaseSpinLock(Adapter, RT_CPU_SPINLOCK);	
	return rtStatus;
	
CPUCheckFirmwareReady_Fail:

	DEBUG_INFO(("+Hal_CPUCheckFirmwareReady failed.\n"));

	//switch to register page for CPU
	//PlatformReleaseSpinLock(Adapter, RT_CPU_SPINLOCK);
	rtStatus = RT_STATUS_FAILURE;
	return rtStatus;

}

void TxFwFillCmdDesc8192Usb( u8 			QueueIndex,
						 u8 *		addr,
						 u32 		BufferLen,
						 u8 			bLastInitPacket,
						 u32 		DescPacketType)
{
	PTX_DESC_CMD_819x_USB pDesc = NULL;
	DEBUG_INFO(("+TxFwFillCmdDesc8192Usb\n"));

 
//	if(padapter->bDriverStopped || padapter->bSurpriseRemoved)
//	{
//		goto txfilldesc_error;
//	}
 
	if(addr == NULL)
	{
		DEBUG_ERR(("TxFillCmdDesc8192Usb: Addr should not be NULL !!!\n"));
		goto txfilldesc_error;
	}
 
	pDesc = (PTX_DESC_CMD_819x_USB)addr;
	memset(pDesc, 0 ,  sizeof(TX_DESC_CMD_819x_USB));
 
	pDesc->LINIP = bLastInitPacket; 
	pDesc->FirstSeg = 1;//bFirstSeg;
	pDesc->LastSeg = 1;//bLastSeg;
 
	pDesc->CmdInit = ((DescPacketType == DESC_PACKET_TYPE_INIT)? DESC_PACKET_TYPE_INIT: DESC_PACKET_TYPE_NORMAL) ;
	if (DescPacketType == DESC_PACKET_TYPE_NORMAL)
	{
		pDesc->TxFWInfoSize = (u8)(BufferLen - TXDESC_OFFSET);
		pDesc->QueueSelect = MapHwQueueToFirmwareQueue(QueueIndex);
	}
	else
	{
		//DEBUG_INFO(("TxFillCmdDesc8192Usb: apply buffer length as %x\n",BufferLen));
	
		pDesc->TxBufferSize= (u32)(BufferLen - TXDESC_OFFSET);
	}
 
 
	pDesc->OWN = 1;
 // TODO: cmd frame or cmd queue
	 //write_port(padapter, QueueIndex, BufferLen, (unsigned char*)pxmitframe);

 	DEBUG_INFO(("-TxFwFillCmdDesc8192Usb\n"));
	return;
  
// TODO: cmd frame free , deal with  related issue
txfilldesc_error:
	DEBUG_ERR(("TxFillCmdDesc8192Usb failed \n"));
 
 	return;
}

u8 MapHwQueueToFirmwareQueue(u8 QueueID)
{
	u8 QueueSelect = 0x0; //defualt set to
 
	switch(QueueID)                     
	{
		case VO_QUEUE_INX:
			QueueSelect  = QSLT_VO;
			break;
		case VI_QUEUE_INX:
			QueueSelect  = QSLT_VI;
			break;
		case BE_QUEUE_INX:
			QueueSelect  = QSLT_BE;
			break;
		case BK_QUEUE_INX:
			QueueSelect  = QSLT_BK;
			break;
 		case MGNT_QUEUE_INX: //mgnt_queue_inx
			QueueSelect  = QSLT_MGNT;
			break; 
   		case BEACON_QUEUE_INX: //beacon_queue_inx
			QueueSelect  = QSLT_BEACON;
			break; 
		case TXCMD_QUEUE_INX:
			QueueSelect  = QSLT_CMD;
			break;
		default:
			DEBUG_ERR(("Impossible Queue Selection: %d \n", QueueID));
			break;
	}

	return QueueSelect;
}

VOID TxCmd_AppendZeroAndEndianTransform(u1Byte* pDst,u1Byte* pSrc,u2Byte* pLength)
{

	u2Byte	ulAppendBytes = 0, i;
	u2Byte	ulLength = *pLength;

//test only
	DEBUG_INFO(("+TxCmd_AppendZeroAndEndianTransform for %p-%p len(%x)\n",pDst,pSrc,*pLength));

	memset(pDst, 0xcc, 12);

	pDst += USB_HWDESC_HEADER_LEN;
	ulLength -= USB_HWDESC_HEADER_LEN;

	for( i=0 ; i<ulLength ; i+=4)
	{
		if((i+3) < ulLength)	pDst[i+0] = pSrc[i+3];
		if((i+2) < ulLength)	pDst[i+1] = pSrc[i+2];
		if((i+1) < ulLength)	pDst[i+2] = pSrc[i+1];
		if((i+0) < ulLength)	pDst[i+3] = pSrc[i+0];

	}
	
	//1(2) Append Zero
	if(  ((*pLength) % 4)  >0)
	{
		ulAppendBytes = 4-((*pLength) % 4);

		for(i=0 ; i<ulAppendBytes; i++)
			pDst[  4*((*pLength)/4)  + i ] = 0x0;

		*pLength += ulAppendBytes;	
	}
	DEBUG_INFO(("-TxCmd_AppendZeroAndEndianTransform get length as %x\n",*pLength ));
}


RT_STATUS Hal_fwSendDownloadCode(IN PADAPTER 	Adapter, IN u32 Step)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	u2Byte					Length;
	u2Byte					SubmitLength=0;
	u2Byte					FragThreshold = 1024;
	BOOLEAN					bLastInitPacket = _FALSE;
	int error_number;

	DEBUG_INFO(("+Hal_fwSendDownloadCode\n"));
	
printk("Hal_fwSendDownloadCode step=%d\n", Step);

	switch(Step)
	{
		case 0:
			TxfwBootFrame = (TX_FW_BOOT_FRAME*)_malloc(sizeof(TX_FW_BOOT_FRAME));
			if (TxfwBootFrame==0)
			{
				DEBUG_ERR(("Allocated TxfwBootFrame failed\n"));
			}
			else
			{
				DEBUG_ERR(("Allocate TxfwBootFrame at %p\n", TxfwBootFrame));
				memset(TxfwBootFrame,0,sizeof(TX_FW_BOOT_FRAME));
			}
			
			TxfwBootFrame->padapter=Adapter;
			
			Length = HAL_FwImageLen[Step] + USB_HWDESC_HEADER_LEN;
			TxCmd_AppendZeroAndEndianTransform((u8*)&TxfwBootFrame->tx_fw_cmd,HAL_FwImageBuf[Step],&Length);

			TxFwFillCmdDesc8192Usb(TXCMD_QUEUE, (u8*)&TxfwBootFrame->tx_fw_cmd,Length,_TRUE,DESC_PACKET_TYPE_INIT);
			TxfwBootFrame->ptxfw_urb = usb_alloc_urb(0,GFP_KERNEL);

			{
				int k;
				for( k = 0; k <32; k++)
					DEBUG_ERR(("%x ", *(((u8*)&TxfwBootFrame->tx_fw_cmd)+k)));
				DEBUG_ERR(("\n"));
				DEBUG_ERR(("Total applied length %x\n",Length));
			}
			
			usb_fill_bulk_urb(	TxfwBootFrame->ptxfw_urb,
							Adapter->dvobjpriv.pusbdev,
							usb_sndbulkpipe(Adapter->dvobjpriv.pusbdev,0x5), 
							&TxfwBootFrame->tx_fw_cmd,
							Length,
							usb_write_txfw_complete_boot,
							TxfwBootFrame);
			error_number=usb_submit_urb(TxfwBootFrame->ptxfw_urb, GFP_ATOMIC);

			
			if( error_number!=0 )
			{
				DEBUG_ERR(("Cannot submit urb for TxfwBootFrame: errno %d ",error_number ));
				goto CmdSendDownloadCode_Fail;
			}
			
			break;

		case 1:
			TxfwMainFrame = (TX_FW_MAIN_FRAME *)_malloc(sizeof(TX_FW_MAIN_FRAME));
			if (TxfwMainFrame==0)
			{
				DEBUG_ERR(("Allocated TxfwMainFrame failed\n"));
			}
			else
			{
				DEBUG_ERR(("Allocate TxfwMainFrame at %p\n", TxfwMainFrame));
				memset(TxfwMainFrame,0,sizeof(TX_FW_MAIN_FRAME));
			}
			
			TxfwMainFrame->padapter=Adapter;
			TxfwMainFrame->tx_fw_data_offset = 0;
			TxfwMainFrame->step = 1;
//			while(HAL_FwImageLen[Step]>TxfwMainFrame->tx_fw_data_offset)
			{
				if(HAL_FwImageLen[Step] >= TxfwMainFrame->tx_fw_data_offset +FragThreshold)
				{
					SubmitLength = FragThreshold;
				}
				else
				{
					bLastInitPacket = _TRUE;
					SubmitLength = HAL_FwImageLen[Step] -TxfwMainFrame->tx_fw_data_offset;
				}
			
				Length = SubmitLength+USB_HWDESC_HEADER_LEN;
				TxCmd_AppendZeroAndEndianTransform((u8*)&TxfwMainFrame->tx_fw_cmd,
												HAL_FwImageBuf[Step]+TxfwMainFrame->tx_fw_data_offset,
												&Length);
				TxFwFillCmdDesc8192Usb(
									TXCMD_QUEUE,
									(u8*)& TxfwMainFrame->tx_fw_cmd,
									Length,bLastInitPacket,
									DESC_PACKET_TYPE_INIT);

				TxfwMainFrame->ptxfw_urb = usb_alloc_urb(0,GFP_KERNEL);

				usb_fill_bulk_urb(	TxfwMainFrame->ptxfw_urb,
							Adapter->dvobjpriv.pusbdev,
							usb_sndbulkpipe(Adapter->dvobjpriv.pusbdev,0x5), 
							&TxfwMainFrame->tx_fw_cmd,
							Length ,
							usb_write_txfw_complete_main,
							TxfwMainFrame);
				DEBUG_ERR(("After fill main bulk urb\n"));
			
			
				error_number=usb_submit_urb(TxfwMainFrame->ptxfw_urb, GFP_ATOMIC);			
				if( error_number!=0 )
				{
					DEBUG_ERR(("Cannot submit urb for TxfwBootFrame: errno %d ",error_number ));
					goto CmdSendDownloadCode_Fail;
				}

				TxfwMainFrame->tx_fw_data_offset +=SubmitLength;
			}

//			TxfwMainFrame->step = 0;
//			goto CmdSendDownloadCode_Fail;

			break;
		case 2:
			TxfwDataFrame = (TX_FW_DATA_FRAME *)_malloc(sizeof(TX_FW_DATA_FRAME));
			if (TxfwDataFrame==0)
			{
				DEBUG_ERR(("Allocate TxfwDataFrame failed\n"));
			}
			else
			{
				DEBUG_ERR(("Allocate TxfwDataFrame at %p\n", TxfwDataFrame));
				memset(TxfwDataFrame,0,sizeof(TX_FW_DATA_FRAME));
			}
				
			TxfwDataFrame->padapter=Adapter;
			TxfwDataFrame->tx_fw_data_offset = 0;
			TxfwDataFrame->step = 2;
//			while(HAL_FwImageLen[Step]>TxfwMainFrame->tx_fw_data_offset)
			{
				if(HAL_FwImageLen[Step] >= TxfwDataFrame->tx_fw_data_offset +FragThreshold)
				{
					SubmitLength = FragThreshold;
				}
				else
				{
					bLastInitPacket = _TRUE;
					SubmitLength = HAL_FwImageLen[Step] -TxfwDataFrame->tx_fw_data_offset;
				}
				Length = SubmitLength+USB_HWDESC_HEADER_LEN;
				TxCmd_AppendZeroAndEndianTransform((u8*)&TxfwDataFrame->tx_fw_cmd,
												HAL_FwImageBuf[Step]+TxfwDataFrame->tx_fw_data_offset,
												&Length);
				TxFwFillCmdDesc8192Usb(
									TXCMD_QUEUE,
									(u8*)& TxfwDataFrame->tx_fw_cmd,
									Length,bLastInitPacket,
									DESC_PACKET_TYPE_INIT);
			
				TxfwDataFrame->ptxfw_urb = usb_alloc_urb(0,GFP_KERNEL);
			
				usb_fill_bulk_urb(	TxfwDataFrame->ptxfw_urb,
							Adapter->dvobjpriv.pusbdev,
							usb_sndbulkpipe(Adapter->dvobjpriv.pusbdev,0x5), 
							&TxfwDataFrame->tx_fw_cmd,
							Length ,
							usb_write_txfw_complete_data,
							TxfwDataFrame);
				DEBUG_ERR(("After fill data bulk urb\n"));
			
				error_number=usb_submit_urb(TxfwDataFrame->ptxfw_urb, GFP_ATOMIC);			
			if( error_number!=0 )
			{
				DEBUG_ERR(("Cannot submit urb for TxfwBootFrame: errno %d ",error_number ));
				goto CmdSendDownloadCode_Fail;
			}

				TxfwDataFrame->tx_fw_data_offset +=SubmitLength;
			}

			break;
		default:
				goto CmdSendDownloadCode_Fail;
			}

	DEBUG_INFO(("-Hal_fwSendDownloadCode\n"));

	return rtStatus;

CmdSendDownloadCode_Fail:	
	rtStatus = RT_STATUS_FAILURE;
	DEBUG_ERR(("CmdSendDownloadCode fail !!\n"));
	return rtStatus;
}

void usb_write_txfw_complete_boot(struct urb* purb, struct pt_regs *regs )
//void usb_write_txfw_complete_boot(struct urb* purb)
{
	usb_free_urb(purb);
	_mfree((u8*)TxfwBootFrame,0);

	return;
}


void usb_write_txfw_complete_main(struct urb* purb, struct pt_regs *regs )
//void usb_write_txfw_complete_main(struct urb* purb)
{

	_adapter*		Adapter 			=	TxfwMainFrame->padapter;
	u2Byte			FragThreshold 	= 	1024;
	BOOLEAN			bLastInitPacket 	= 	_FALSE;
	u2Byte			SubmitLength		=	0;
	u2Byte			Length 			= 0;
	u32	Step = TxfwMainFrame->step;
	int error_number;

//	usb_free_urb(purb);

	if(TxfwMainFrame->tx_fw_cmd.LINIP ==_TRUE)
	{
		usb_free_urb(purb);
		_mfree((u8 *)TxfwMainFrame,0);
		return;
	}

	if(HAL_FwImageLen[Step] > TxfwMainFrame->tx_fw_data_offset)
	{
		if(HAL_FwImageLen[Step] >= TxfwMainFrame->tx_fw_data_offset +FragThreshold)
		{
			SubmitLength = FragThreshold;
		}
		else
		{
			bLastInitPacket = _TRUE;
			SubmitLength = HAL_FwImageLen[Step]-TxfwMainFrame->tx_fw_data_offset;
			DEBUG_ERR(("Buffer length:%x TxfwMainFrame offset %x", (u32)HAL_FwImageBuf[Step],TxfwMainFrame->tx_fw_data_offset  ));
		}
	
		Length = SubmitLength+USB_HWDESC_HEADER_LEN;
		
		TxCmd_AppendZeroAndEndianTransform((u8*)&TxfwMainFrame->tx_fw_cmd,
										HAL_FwImageBuf[Step]+TxfwMainFrame->tx_fw_data_offset,
										&Length);
		TxFwFillCmdDesc8192Usb(TXCMD_QUEUE,
							(u8*)& TxfwMainFrame->tx_fw_cmd,
							Length,bLastInitPacket,
							DESC_PACKET_TYPE_INIT);
	
		//	TxfwMainFrame->ptxfw_urb = usb_alloc_urb(0,GFP_ATOMIC);

		DEBUG_INFO(("Before fill main_c bulk urb\n"));
		usb_fill_bulk_urb(	TxfwMainFrame->ptxfw_urb,
						Adapter->dvobjpriv.pusbdev,
						usb_sndbulkpipe(Adapter->dvobjpriv.pusbdev,0x5), 
						&TxfwMainFrame->tx_fw_cmd,
						Length,
						usb_write_txfw_complete_main,
						TxfwMainFrame);
		DEBUG_INFO(("After fill main_c bulk urb\n"));
			
			
		error_number=usb_submit_urb(TxfwMainFrame->ptxfw_urb, GFP_ATOMIC);			
		if( error_number!=0 )
		{
			DEBUG_ERR(("Cannot submit urb for TxfwBootFrame: errno %d ",error_number ));
			return;
		}
	
		TxfwMainFrame->tx_fw_data_offset +=SubmitLength;
	}
}

void usb_write_txfw_complete_data(struct urb* purb, struct pt_regs *regs )
//void usb_write_txfw_complete_data(struct urb* purb)
{

	_adapter*		Adapter 			=	TxfwDataFrame->padapter;
	u2Byte			FragThreshold 	= 	2048;
	BOOLEAN			bLastInitPacket 	= 	_FALSE;
	u2Byte			SubmitLength		=	0;
	u2Byte			Length 			= 0;
	int error_number;

	u32	Step = TxfwDataFrame->step;

	DEBUG_ERR(("+usb_write_txfw_complete_data with status %d\n",purb->status));

	if(TxfwDataFrame->tx_fw_cmd.LINIP ==_TRUE)
	{
		usb_free_urb(purb);
		_mfree((u8*)TxfwDataFrame,0);
		return;
	}

	{
		if(HAL_FwImageLen[Step] > TxfwDataFrame->tx_fw_data_offset)
		{
			
			if(HAL_FwImageLen[Step] >= TxfwDataFrame->tx_fw_data_offset +FragThreshold)
			{
				SubmitLength = FragThreshold;
			}
			else
			{
				bLastInitPacket = _TRUE;
				SubmitLength = HAL_FwImageLen[Step]-TxfwDataFrame->tx_fw_data_offset;
				DEBUG_ERR(("Buffer length:%x TxfwMainFrame offset %x",(u32) HAL_FwImageBuf[Step],TxfwDataFrame->tx_fw_data_offset  ));
			}
		
			Length = SubmitLength+USB_HWDESC_HEADER_LEN;
			
			TxCmd_AppendZeroAndEndianTransform((u8*)&TxfwDataFrame->tx_fw_cmd,
											HAL_FwImageBuf[Step]+TxfwDataFrame->tx_fw_data_offset,
											&Length);
			TxFwFillCmdDesc8192Usb(TXCMD_QUEUE,
								(u8*)& TxfwDataFrame->tx_fw_cmd,
								Length,bLastInitPacket,
								DESC_PACKET_TYPE_INIT);
		
			//	TxfwDataFrame->ptxfw_urb = usb_alloc_urb(0,GFP_ATOMIC);
		
			usb_fill_bulk_urb(	TxfwDataFrame->ptxfw_urb,
							Adapter->dvobjpriv.pusbdev,
							usb_sndbulkpipe(Adapter->dvobjpriv.pusbdev,0x5), 
							&TxfwDataFrame->tx_fw_cmd,
							Length ,
							usb_write_txfw_complete_data,
							TxfwDataFrame);
			DEBUG_ERR(("After fill data_c bulk urb\n"));
		
			error_number=usb_submit_urb(TxfwDataFrame->ptxfw_urb, GFP_ATOMIC);			
			if( error_number!=0 )
			{
				DEBUG_ERR(("Cannot submit urb for TxfwBootFrame: errno %d ",error_number ));
				return;
			}
			
			TxfwDataFrame->tx_fw_data_offset +=SubmitLength;
		}

	}

	DEBUG_ERR(("-usb_write_txfw_complete_data\n"));
}


#if 0
RT_STATUS Hal_TxCmdSendPacket(	PADAPTER				Adapter,
								PRT_TCB					pTcb,
								PRT_TX_LOCAL_BUFFER 	pBuf,
								u4Byte					BufferLen,
								u4Byte					PacketType,
								BOOLEAN					bLastInitPacket)
{
	s2Byte		i;
	u1Byte		QueueID;
	u2Byte		firstDesc,curDesc = 0;
	u2Byte		FragIndex=0, FragBufferIndex=0;

	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	
	CmdInitTCB(Adapter, pTcb, pBuf, BufferLen);

	
	if(CmdCheckFragment(Adapter, pTcb, pBuf))
		CmdFragmentTCB(Adapter, pTcb);
	else
		pTcb->FragLength[0] = (u2Byte)pTcb->BufferList[0].Length;

	QueueID=pTcb->SpecifiedQueueID;

	if(PlatformIsTxQueueAvailable(Adapter, QueueID, pTcb->BufferCount) &&
		RTIsListEmpty(&Adapter->TcbWaitQueue[QueueID]))
	{
		pTcb->nDescUsed=0;

		for(i=0 ; i<pTcb->BufferCount ; i++)
		{
			Adapter->HalFunc.TxFillCmdDescHandler(
				Adapter,											
				pTcb,
				QueueID,												//QueueIndex 
				curDesc,												//index
				FragBufferIndex==0,									//bFirstSeg
				FragBufferIndex==(pTcb->FragBufCount[FragIndex]-1),		//bLastSeg
				pTcb->BufferList[i].VirtualAddress,						//VirtualAddress
				pTcb->BufferList[i].PhysicalAddressLow,					//PhyAddressLow
				pTcb->BufferList[i].Length,								//BufferLen
				i!=0,												//bSetOwnBit
				(i==(pTcb->BufferCount-1)) && bLastInitPacket,			//bLastInitPacket
				PacketType,											//DescPacketType
				pTcb->FragLength[FragIndex]							//PktLen
				);

			if(FragBufferIndex==(pTcb->FragBufCount[FragIndex]-1))
			{ // Last segment of the fragment.
				pTcb->nFragSent++;
			}

			FragBufferIndex++;
			if(FragBufferIndex==pTcb->FragBufCount[FragIndex])
			{
				FragIndex++;
				FragBufferIndex=0;
			}

			pTcb->nDescUsed++;
		}

	}
	else
	{
		pTcb->bLastInitPacket = bLastInitPacket;
		RTInsertTailList(&Adapter->TcbWaitQueue[pTcb->SpecifiedQueueID], &pTcb->List);
	}

	return rtStatus;
}
#endif

/*! Initialize hardware  for RTL8187 Device. */
uint	 rtl8192u_hal_init(_adapter *padapter) {

	uint status = _SUCCESS;
	
	_func_enter_;
	DEBUG_ERR(("\n +rtl8192_hal_init\n"));

	rtl8192u_start(padapter);
	DEBUG_ERR(("\n -rtl8192_hal_init\n"));

	padapter->hw_init_completed=_TRUE;

#ifdef TODO
	r8187_start_DynamicInitGain(padapter);
	r8187_start_RateAdaptive(padapter);
#endif

	_func_exit_;
	return status;
	
}

uint rtl8192u_hal_deinit(_adapter *padapter)
{
	u8	bool;
	uint status = _SUCCESS;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	u8 OpMode;
	u8 valu8;
	u8 eRFPath;

	u32 valu32;
	
	_func_enter_;
	DEBUG_ERR(("\n +rtl8187_hal_deinit\n"));

	//cancel mlme timer
	while(1) {
		_cancel_timer(&_sys_mib->survey_timer, &bool);
		if(bool == _TRUE)
			break;
	}

	while(1) {
		_cancel_timer(&_sys_mib->reauth_timer, &bool);
		if(bool == _TRUE)
			break;
	}

	while(1) {
		_cancel_timer(&_sys_mib->reassoc_timer, &bool);
		if(bool == _TRUE)
			break;
	}

	while(1) {
		_cancel_timer(&_sys_mib->disconnect_timer, &bool);
		if(bool == _TRUE)
			break;
	}
	while(1) {
		_cancel_timer(&_sys_mib->sendbeacon_timer, &bool);
		if(bool == _TRUE)
			break;
	}
	

	

#ifdef TODO
	r8187_stop_DynamicInitGain(padapter);
	r8187_stop_RateAdaptive(padapter);
#endif

	OpMode=MSR_NOLINK;
	Hal_SetHwReg819xUsb(padapter,HW_VAR_MEDIA_STATUS,&OpMode);
	valu8=0x0;
	Hal_SetHwReg819xUsb(padapter,HW_VAR_COMMAND,&valu8);

	msleep_os(1);

	msleep_os(20);


	for(eRFPath=(RF90_RADIO_PATH_E)RF90_PATH_A; eRFPath < RF90_PATH_MAX; eRFPath++)
	{
		if(eRFPath == RF90_PATH_A || eRFPath == RF90_PATH_B)
		{
			PHY_SetBBReg(padapter,padapter->eeprompriv.PHYRegDef[eRFPath].rfintfs, bRFSI_RFENV, bRFSI_RFENV);
			PHY_SetBBReg(padapter, padapter->eeprompriv.PHYRegDef[eRFPath].rfintfo, bRFSI_RFENV, 0);
		}
	}

	
	valu32=read32(padapter,CPU_GEN)|CPU_GEN_SYSTEM_RESET;
	write32(padapter,CPU_GEN,valu32);
	
	write8(padapter,0xfe5e,0x80);
	write8(padapter,0xfe5e,0x80);

	DEBUG_ERR(("\n -rtl8187_hal_deinit\n"));
	_func_exit_;
	return status;
}


void rtl8225z2_SetTXPowerLevel(_adapter *padapter, short ch)
{
	int i;
	u32 value;
	u8 *cck_power_table;
	s8 max_cck_power_level;
	s8 min_cck_power_level;
	s8 max_ofdm_power_level;
	s8 min_ofdm_power_level;	
	s8 cck_power_level = (s8)padapter->eeprompriv.tx_power_b[ch];
	s8 ofdm_power_level = (s8)padapter->eeprompriv.tx_power_g[ch];
		
	max_cck_power_level = 17;
	min_cck_power_level = 2;
	max_ofdm_power_level = 25; //  12 -> 25
	min_ofdm_power_level = 10;
	
	/* CCK power setting */
	if(cck_power_level > (max_cck_power_level - min_cck_power_level))
		cck_power_level = max_cck_power_level;
	else
		cck_power_level += min_cck_power_level;
	
	cck_power_level += padapter->eeprompriv.cck_txpwr_base;
	
	if(cck_power_level > 35)
		cck_power_level = 35;
	if(cck_power_level <0)
		cck_power_level = 0;
		
	if(ch == 14) 
		cck_power_table = rtl8225z2_tx_power_cck_ch14;
	else 
		cck_power_table = rtl8225z2_tx_power_cck;
	
	if(cck_power_level<=5){
	}
	else if(cck_power_level<=11){
		cck_power_table += 8;
	}
	else if(cck_power_level<=17){
		cck_power_table += (8*2);
	}
	else{
		cck_power_table += (8*3);
	}
	for(i=0;i<8;i++){
		value = cck_power_table[i] << 8;
		write32(padapter,PhyAddr, ((0x010000c4+i) | value ));
	}

	//2005.11.17,
	write8(padapter, CCK_TXAGC, (ZEBRA2_CCK_OFDM_GAIN_SETTING[cck_power_level]*2));

	/* OFDM power setting */
//  Old:
//	if(ofdm_power_level > max_ofdm_power_level)
//		ofdm_power_level = 35;
//	ofdm_power_level += min_ofdm_power_level;
//  Latest:
	if(ofdm_power_level > (max_ofdm_power_level - min_ofdm_power_level))
		ofdm_power_level = max_ofdm_power_level;
	else
		ofdm_power_level += min_ofdm_power_level;
	
	ofdm_power_level += padapter->eeprompriv.ofdm_txpwr_base;
		
	if(ofdm_power_level > 35)
		ofdm_power_level = 35;
	if(ofdm_power_level < 0 )
		ofdm_power_level = 0;

	//2005.11.17,
	write8(padapter, OFDM_TXAGC, (ZEBRA2_CCK_OFDM_GAIN_SETTING[ofdm_power_level]*2));
	if(ofdm_power_level <= 11){
		write32(padapter, PhyAddr,0x00005C87);
		write32(padapter, PhyAddr,0x00005C89);
	}
	else if(ofdm_power_level <= 17){
		write32(padapter, PhyAddr,0x00005487);
		write32(padapter, PhyAddr,0x00005489);
	}
	else {
		write32(padapter, PhyAddr,0x00005087);
		write32(padapter, PhyAddr,0x00005089);
	}

}


void rtl8225z2_rf_set_chan(_adapter *padapter, short ch)
{

	rtl8225z2_SetTXPowerLevel(padapter, ch);

	write_rtl8225(padapter, 0x7, rtl8225_chan[ch]);

	mdelay_os(10);

#if 0
	{
		write16(padapter,AC_VO_PARAM,0x731c);
		write16(padapter, AC_VI_PARAM,0x731c);
		write16(padapter, AC_BE_PARAM,0x731c);
		write16(padapter, AC_BK_PARAM,0x731c);
	}
#endif

}
u4Byte phy_CalculateBitShift(u4Byte BitMask)
{
	u4Byte i;

	for(i=0; i<=31; i++)
	{
		if ( ((BitMask>>i) &  0x1 ) == 1)
			break;
	}

	return (i);
}



void PHY_SetBBReg(_adapter *padapter,u32 RegAddr, u32 BitMask , u32 Data)
{
	u4Byte	OriginalValue, BitShift, NewValue;
		
	if(BitMask!= 0xffffffff)
	{//if not "double word" write
		OriginalValue = read32(padapter, RegAddr);
		BitShift = phy_CalculateBitShift(BitMask);
            	NewValue = (((OriginalValue) & (~BitMask)) | (Data << BitShift));
		write32(padapter, RegAddr, NewValue);	
	}else
		write32(padapter, RegAddr, Data);	
	
	return;
}




void PHY_SetRF8256CCKTxPower(_adapter *padapter, u1Byte	powerlevel)
{
	u4Byte				TxAGC=0;
	

	TxAGC = powerlevel;
	PHY_SetBBReg(padapter, rTxAGC_CCK_Mcs32, bTxAGCRateCCK, TxAGC);
}



void  PHY_SetRF8256OFDMTxPower(_adapter *padapter, u1Byte	powerlevel)
{

	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;
	
	//modified by vivi, 20080109
	u4Byte OFDM6_OFDM9_MCS0_MCS8, OFDM12_MCS1_MCS9, OFDM18_MCS2_MCS10;
	u4Byte OFDM24_MCS3_MCS11, OFDM36_MCS4_MCS12, OFDM48_MCS5_MCS13;
	u4Byte OFDM54_MCS6_MCS14, MCS7_MCS15;
	u4Byte byte0, byte1, byte2, byte3, writeVal;
	
	OFDM6_OFDM9_MCS0_MCS8 = (peeprompriv->MCSTxPowerLevelOriginalOffset[0]&0xff);
	OFDM12_MCS1_MCS9 = ((peeprompriv->MCSTxPowerLevelOriginalOffset[0]&0xff00)>>8);
	OFDM18_MCS2_MCS10 = ((peeprompriv->MCSTxPowerLevelOriginalOffset[0]&0xff0000)>>16);
	OFDM24_MCS3_MCS11 = ((peeprompriv->MCSTxPowerLevelOriginalOffset[0]&0xff000000)>>24);
	OFDM36_MCS4_MCS12 = (peeprompriv->MCSTxPowerLevelOriginalOffset[1]&0xff);
	OFDM48_MCS5_MCS13 = ((peeprompriv->MCSTxPowerLevelOriginalOffset[1]&0xff00)>>8);
	OFDM54_MCS6_MCS14 = ((peeprompriv->MCSTxPowerLevelOriginalOffset[1]&0xff0000)>>16);
			MCS7_MCS15 = ((peeprompriv->MCSTxPowerLevelOriginalOffset[1]&0xff000000)>>24);

	byte0 = powerlevel+OFDM6_OFDM9_MCS0_MCS8+peeprompriv->TxPowerDiff;//OFDM6M
	byte1 = powerlevel+OFDM6_OFDM9_MCS0_MCS8+peeprompriv->TxPowerDiff;//OFDM9M
	byte2 = powerlevel+OFDM12_MCS1_MCS9+peeprompriv->TxPowerDiff;//OFDM12M
	byte3 = powerlevel+OFDM18_MCS2_MCS10+peeprompriv->TxPowerDiff;//OFDM18M
	writeVal = (byte3<<24|byte2<<16|byte1<<8|byte0);
	PHY_SetBBReg(padapter, rTxAGC_Rate18_06, bTxAGCRate18_06, writeVal);

	byte0 = powerlevel+OFDM24_MCS3_MCS11+peeprompriv->TxPowerDiff;//OFDM24M
	byte1 = powerlevel+OFDM36_MCS4_MCS12+peeprompriv->TxPowerDiff;//OFDM36M
	byte2 = powerlevel+OFDM48_MCS5_MCS13+peeprompriv->TxPowerDiff;//OFDM48M
	byte3 = powerlevel+OFDM54_MCS6_MCS14+peeprompriv->TxPowerDiff;//OFDM54M
	writeVal = (byte3<<24|byte2<<16|byte1<<8|byte0);
	PHY_SetBBReg(padapter, rTxAGC_Rate54_24, bTxAGCRate54_24, writeVal);

	byte0 = powerlevel+OFDM6_OFDM9_MCS0_MCS8;//MCS0
	byte1 = powerlevel+OFDM12_MCS1_MCS9;//MCS1
	byte2 = powerlevel+OFDM18_MCS2_MCS10;//MCS2
	byte3 = powerlevel+OFDM24_MCS3_MCS11;//MCS3
	writeVal = (byte3<<24|byte2<<16|byte1<<8|byte0);
	PHY_SetBBReg(padapter, rTxAGC_Mcs03_Mcs00, bTxAGCRateMCS3_MCS0, writeVal);

	byte0 = powerlevel+OFDM36_MCS4_MCS12;//MCS4
	byte1 = powerlevel+OFDM48_MCS5_MCS13;//MCS5
	byte2 = powerlevel+OFDM54_MCS6_MCS14;//MCS6
	byte3 = powerlevel+MCS7_MCS15;//MCS7
	writeVal = (byte3<<24|byte2<<16|byte1<<8|byte0);
	PHY_SetBBReg(padapter, rTxAGC_Mcs07_Mcs04, bTxAGCRateMCS7_MCS4, writeVal);

	byte0 = powerlevel+OFDM6_OFDM9_MCS0_MCS8;//MCS8
	byte1 = powerlevel+OFDM12_MCS1_MCS9;//MCS9
	byte2 = powerlevel+OFDM18_MCS2_MCS10;//MCS10
	byte3 = powerlevel+OFDM24_MCS3_MCS11;//MCS11
	writeVal = (byte3<<24|byte2<<16|byte1<<8|byte0);
	PHY_SetBBReg(padapter, rTxAGC_Mcs11_Mcs08, bTxAGCRateMCS11_MCS8, writeVal);

	byte0 = powerlevel+OFDM36_MCS4_MCS12;//MCS12
	byte1 = powerlevel+OFDM48_MCS5_MCS13;//MCS13
	byte2 = powerlevel+OFDM54_MCS6_MCS14;//MCS14
	byte3 = powerlevel+MCS7_MCS15;//MCS15
	writeVal = (byte3<<24|byte2<<16|byte1<<8|byte0);
	PHY_SetBBReg(padapter, rTxAGC_Mcs15_Mcs12, bTxAGCRateMCS15_MCS12, writeVal);
}






void SetTxPowerLevel819xUsb(_adapter *padapter, short ch)
{
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;
 	
	// RF 8256	
	
	switch(peeprompriv->RFChipID)
	{
	case RF_8225:
			DEBUG_ERR(("ERROR: SetTxPowerLevel819xUsb : RF_8225 not suport!"));
		break;

	case RF_8256:
		PHY_SetRF8256CCKTxPower(padapter, peeprompriv->tx_power_b[ch-1]);
		PHY_SetRF8256OFDMTxPower(padapter, peeprompriv->tx_power_g[ch-1]);
		break;

	case RF_TYPE_MIN:
	case RF_PSEUDO_11N:
 	case RF_8258:
			DEBUG_ERR(("ERROR: SetTxPowerLevel819xUsb : RFChipID not suport!"));
		break;
	}	

		
}


u4Byte PHY_QueryBBReg(_adapter *padapter, u32 RegAddr , u32 BitMask)
{

  	u4Byte	ReturnValue = 0, OriginalValue, BitShift;

	OriginalValue = read32(padapter, RegAddr);
	BitShift = phy_CalculateBitShift(BitMask);
	ReturnValue = (OriginalValue & BitMask) >> BitShift;

	return (ReturnValue);
}



void phy_RFSerialWrite(_adapter * padapter, u8 eRFPath, u32 Offset, u32 Data)
{


         u4Byte						DataAndAddr = 0;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;
	BB_REGISTER_DEFINITION_T	*pPhyReg = &peeprompriv->PHYRegDef[eRFPath];
	u4Byte					NewOffset;

	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
	
	{		

		//_spinlock(&padapter->rf_write_lock);

		if(Offset>=31)
		{
			peeprompriv->RFWritePageCnt[2]++;//cosa add for debug
			peeprompriv->RfReg0Value[eRFPath] |= 0x140;
			
			PHY_SetBBReg(
				padapter, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(peeprompriv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			peeprompriv->RFWritePageCnt[1]++;//cosa add for debug
			peeprompriv->RfReg0Value[eRFPath] |= 0x100;
			peeprompriv->RfReg0Value[eRFPath] &= (~0x40);			
							

			PHY_SetBBReg(
				padapter, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(peeprompriv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 15;
		}
		else
		{
			peeprompriv->RFWritePageCnt[0]++;//cosa add for debug
			NewOffset = Offset;
		}
	}


	//
	// Put write addr in [5:0]  and write data in [31:16]
	//
	DataAndAddr = (Data<<16) | (NewOffset&0x3f);

	//
	// Write Operation
	//
	PHY_SetBBReg(padapter, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);


	if(Offset==0x0)
		peeprompriv->RfReg0Value[eRFPath] = Data;

	// Switch back to Reg_Mode0;
 
	{
		if(Offset != 0)
		{
		peeprompriv->RfReg0Value[eRFPath] &= 0xebf;
		PHY_SetBBReg(
			padapter, 
			pPhyReg->rf3wireOffset, 
			bMaskDWord, 
			(peeprompriv->RfReg0Value[eRFPath] << 16)	);
		}


		//_spinunlock(&padapter->rf_write_lock);
	}

}

u4Byte phy_RFSerialRead(_adapter * padapter, u8 eRFPath, u32 Offset)
{

         u4Byte						retValue = 0;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;
	BB_REGISTER_DEFINITION_T	*pPhyReg = &peeprompriv->PHYRegDef[eRFPath];
	u4Byte						NewOffset;


	DEBUG_INFO(("Try to read RF register offset %x\n", Offset));

        PHY_SetBBReg(padapter, pPhyReg->rfLSSIReadBack, bLSSIReadBackData,0x000);
	//
	// Make sure RF register offset is correct 
	//
	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
	{
		if(Offset>=31)
		{
			peeprompriv->RFReadPageCnt[2]++;//cosa add for debug
			peeprompriv->RfReg0Value[eRFPath] |= 0x140;

			// Switch to Reg_Mode2 for Reg31~45
			PHY_SetBBReg(
				padapter, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(peeprompriv->RfReg0Value[eRFPath] << 16)	);

			// Modified Offset
			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			peeprompriv->RFReadPageCnt[1]++;//cosa add for debug
			peeprompriv->RfReg0Value[eRFPath] |= 0x100;
			peeprompriv->RfReg0Value[eRFPath] &= (~0x40);

			// Switch to Reg_Mode1 for Reg16~30
			PHY_SetBBReg(
				padapter, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(peeprompriv->RfReg0Value[eRFPath] << 16)	);

			// Modified Offset
			NewOffset = Offset - 15;
		}
		else
		{
			peeprompriv->RFReadPageCnt[0]++;//cosa add for debug
			NewOffset = Offset;
		}
		
	}
	//
	// Put desired read address to LSSI control register
	//
	PHY_SetBBReg(padapter, pPhyReg->rfHSSIPara2, bLSSIReadAddress, NewOffset);

	//
	// Issue a posedge trigger
	//
	PHY_SetBBReg(padapter, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x0);	
	PHY_SetBBReg(padapter, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x1);	
	
	
	// TODO: we should not delay such a  long time. Ask help from SD3
	mdelay_os(1);

	retValue = PHY_QueryBBReg(padapter, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);

	
	// Switch back to Reg_Mode0;
	
	{
		peeprompriv->RfReg0Value[eRFPath] &= 0xebf;
	
		PHY_SetBBReg(
			padapter, 
			pPhyReg->rf3wireOffset, 
			bMaskDWord, 
			(peeprompriv->RfReg0Value[eRFPath] << 16)	);
	}
	
	return retValue;	

}




void PHY_SetRFReg(_adapter *padapter, u8 eRFPath, u32 RegAddr, u32 BitMask , u32 Data)
{
	u4Byte Original_Value, BitShift, New_Value;

	if(eRFPath==RF90_PATH_C||eRFPath==RF90_PATH_D)
		return;

	{
	     	if (BitMask != bMask12Bits) // RF data is 12 bits only
   	        {
			Original_Value = phy_RFSerialRead(padapter, eRFPath, RegAddr);
	      		BitShift =  phy_CalculateBitShift(BitMask);
	      		New_Value = ( ( (Original_Value) & (~BitMask) )| (Data<< BitShift));
			phy_RFSerialWrite(padapter, eRFPath, RegAddr, New_Value);
			
	        }
		else
	        
			phy_RFSerialWrite(padapter, eRFPath, RegAddr, Data);
	}

}




void rtl8256_rf_set_chan(_adapter *padapter, short ch)
{


	u1Byte eRFPath;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;

	DEBUG_INFO(("Site surver for channal 1\n"));

	if(peeprompriv->VersionID == VERSION_819xUsb_A)
	{
		for(eRFPath = 0; eRFPath <RF90_PATH_MAX; eRFPath++)
		{	


			if(peeprompriv->RF_Type == RF_1T2R)
			{
				if(eRFPath == RF90_PATH_C || eRFPath == RF90_PATH_D)
					continue;
			}
			else if(peeprompriv->RF_Type == RF_2T4R)

				continue;

			
#if 1
			SetTxPowerLevel819xUsb(padapter, ch);
#endif
							
		}
	}
	for(eRFPath = 0; eRFPath <RF90_PATH_MAX; eRFPath++)
	{	

		if(peeprompriv->RF_Type == RF_1T2R)
		{
			if(eRFPath == RF90_PATH_C || eRFPath == RF90_PATH_D)
				continue;
		}
		else if(peeprompriv->RF_Type == RF_2T4R)
			continue;

		PHY_SetRFReg(padapter, eRFPath, rZebra1_Channel, bMask12Bits, (ch<<7));
//		PHY_SetRFReg(padapter, eRFPath, rZebra1_Channel, bZebra1_ChannelNum, (ch<<0x7));
		
	}
	
//	write_rtl8225(padapter, 0x7, rtl8225_chan[ch]);

	mdelay_os(10);


}







sint _init_mibinfo(_adapter* padapter)
{
	int i;
	sint res=_SUCCESS;
	unsigned int addr;
	struct mib_info* _sys_mib = &(padapter->_sys_mib);

	DEBUG_INFO(("+_init_mibinfo\n"));

	//init lock
	_spinlock_init(&_sys_mib->free_rxirp_mgt_lock);
	_spinlock_init(&_sys_mib->free_txirp_mgt_lock);
	 _init_sema(&_sys_mib->cam_sema,1);

	_init_sema(&_sys_mib->queue_event_sema, 1);

	//init list
	_init_listhead(&(_sys_mib->free_rxirp_mgt));
	_init_listhead(&(_sys_mib->free_txirp_mgt));
	_init_listhead(&(_sys_mib->assoc_entry));

	//init timer
	_init_timer(&(_sys_mib->survey_timer), padapter->pnetdev, &survey_timer_hdl, padapter);
	_init_timer(&(_sys_mib->reauth_timer), padapter->pnetdev, &reauth_timer_hdl, padapter);
	_init_timer(&(_sys_mib->reassoc_timer), padapter->pnetdev, &reassoc_timer_hdl, padapter);
	_init_timer(&(_sys_mib->disconnect_timer), padapter->pnetdev, &disconnect_timer_hdl, padapter);

#ifdef CONFIG_RTL8192U
	_init_timer(&(_sys_mib->sendbeacon_timer), padapter->pnetdev, &sendbeacon_timer_hdl, padapter);	

//	init EDCA Parameter
	_sys_mib->edca_param[0] = 0x0000A403;
	_sys_mib->edca_param[1] = 0x0000A427;
	_sys_mib->edca_param[2] = 0x005E4342;
	_sys_mib->edca_param[3] = 0x002F3262;

#endif

	_sys_mib->pallocated_fw_rxirp_buf = _malloc((RXMGTIRPS_MGT * RXMGTBUF_SZ) +4);
	if (_sys_mib->pallocated_fw_rxirp_buf  == NULL)
	{
		res=_FAIL;
		goto exit;
	}
	_sys_mib->pfw_rxirp_buf = _sys_mib->pallocated_fw_rxirp_buf +4 - ((uint)(_sys_mib->pallocated_fw_rxirp_buf)&3);
	addr =(unsigned int) _sys_mib->pfw_rxirp_buf;

	for(i = 0 ; i < RXMGTIRPS_MGT; i++, addr += RXMGTBUF_SZ)	//MGT Buf will take away 4KB
	{
		_init_listhead(&(_sys_mib->rxbufheadpool.mgt.irps[i].list));
		_sys_mib->rxbufheadpool.mgt.irps[i].tag = RXMGTIRPS_TAG;
		_sys_mib->rxbufheadpool.mgt.irps[i].rxbuf_phyaddr = addr;
		list_insert_tail(&(_sys_mib->rxbufheadpool.mgt.irps[i].list), &_sys_mib->free_rxirp_mgt);
		_sys_mib->rxbufheadpool.mgt.irps[i].adapter = padapter;
	}

	_sys_mib->pallocated_fw_txirp_buf = _malloc((TXMGTIRPS_MGT * TXMGTBUF_SZ) +4);
	if (_sys_mib->pallocated_fw_txirp_buf  == NULL)
	{
		res=_FAIL;
		goto exit;
	}
	_sys_mib->pfw_txirp_buf = _sys_mib->pallocated_fw_txirp_buf +4 - ((uint)(_sys_mib->pallocated_fw_txirp_buf)&3);
	addr =(unsigned int) _sys_mib->pfw_txirp_buf;
		
	for(i = 0 ; i < TXMGTIRPS_MGT; i++, addr += TXMGTBUF_SZ)	//MGT Buf will take away 4KB
	{
		_init_listhead(&(_sys_mib->txbufheadpool.mgt.irps[i].list));
		_sys_mib->txbufheadpool.mgt.irps[i].tag = TXMGTIRPS_TAG;
		_sys_mib->txbufheadpool.mgt.irps[i].txbuf_phyaddr = addr;
		list_insert_tail(&(_sys_mib->txbufheadpool.mgt.irps[i].list), &(_sys_mib->free_txirp_mgt));
	}

	// Below is for CAM management
	for(i = 0; i < MAX_STASZ; i++)
	{
		_init_listhead(&(_sys_mib->hash_array[i]));	
		//sys_mib.cam_array[i].sta = NULL;
	}

	for(i = 0; i < MAX_STASZ; i++)
	{
		_sys_mib->stainfo2s[i].aid = i;
	}
	
	for(i = 0; i < MAX_STASZ; i++)
        {
                _sys_mib->sta_data[i].status = -1;
                _sys_mib->sta_data[i]._protected = 0;
                _sys_mib->sta_data[i].info = &(_sys_mib->stainfos[i]);//!!sys_mib.stainfo is not initialized yet.
                _sys_mib->sta_data[i].info2 = &(_sys_mib->stainfo2s[i]);
                _sys_mib->sta_data[i].rxcache = &(_sys_mib->rxcache[i]);
                _sys_mib->sta_data[i].stats = &(_sys_mib->stainfostats[i]);

                _init_listhead (&(_sys_mib->sta_data[i].hash));
                _init_listhead (&(_sys_mib->sta_data[i].assoc));
                _init_listhead (&(_sys_mib->sta_data[i].sleep));

        }

	_sys_mib->cur_class = (struct regulatory_class *)_malloc(sizeof(struct regulatory_class));

	_sys_mib->mac_ctrl.opmode = WIFI_STATION_STATE;
	_sys_mib->mac_ctrl.pbcn = _malloc(512);
	
	_sys_mib->beacon_cnt = 10;

	_sys_mib->networks.head = 0;
	_sys_mib->networks.tail = 0;
	_sys_mib->network_type = WIRELESS_11BG;

	_sys_mib->cur_modem = MIXED_PHY;
	_sys_mib->cur_channel = 1;

	init_phyinfo(padapter, (struct setphyinfo_parm*)_sys_mib->class_sets);

	setup_rate_tbl(padapter);

	//Init DIG parameter
	_sys_mib->RegBModeGainStage = 1;
	_sys_mib->CCKUpperTh = 0x100;
	_sys_mib->CCKLowerTh = 0x20;
	_sys_mib->StageCCKTh = 0;
	_sys_mib->InitialGain = 0;
	_sys_mib->DIG_NumberFallbackVote = 0;
	_sys_mib->DIG_NumberUpgradeVote = 0;
	_sys_mib->FalseAlarmRegValue = 0;
	_sys_mib->RegDigOfdmFaUpTh = 0xC0;

	//Init RateAdaptive parameter
	_sys_mib->CurrRetryCnt = 0;
	_sys_mib->LastRetryCnt = 0;
	_sys_mib->LastTxokCnt = 0;
	_sys_mib->LastRxokCnt = 0;
	_sys_mib->LastTxOKBytes = 0;
	_sys_mib->LastTxThroughput = 0;
	_sys_mib->LastRetryRate = 0;
	_sys_mib->TryupingCountNoData = 0;
	_sys_mib->TryDownCountLowData = 0;
	_sys_mib->bTryuping = _FALSE;
	_sys_mib->LastFailTxRate = 0;
	_sys_mib->LastFailTxRateSS = 0;
	_sys_mib->FailTxRateCount = 0;
	_sys_mib->TryupingCount = 0;
	_sys_mib->NumTxOkTotal = 0;	
	_sys_mib->NumTxOkBytesTotal = 0;
	_sys_mib->NumRxOkTotal = 0;
	_sys_mib->RecvSignalPower = 0;
	_sys_mib->ForcedDataRate = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21))
	_sys_mib->padapter = padapter;
#endif

	DEBUG_INFO(("-_init_mibinfo\n"));

exit:	
	return res;
}

sint _free_mibinfo(_adapter* padapter)
{
	sint res=_SUCCESS;
	struct mib_info* _sys_mib = &(padapter->_sys_mib);
	if(_sys_mib->pallocated_fw_rxirp_buf)
		_mfree(_sys_mib->pallocated_fw_rxirp_buf, (RXMGTIRPS_MGT * RXMGTBUF_SZ) +4);

	if(_sys_mib->pallocated_fw_txirp_buf)
		_mfree(_sys_mib->pallocated_fw_txirp_buf, (TXMGTIRPS_MGT * TXMGTBUF_SZ) +4);

	if(_sys_mib->cur_class)
		_mfree((u8*)_sys_mib->cur_class, sizeof(struct regulatory_class));

	if(_sys_mib->mac_ctrl.pbcn)
		_mfree(_sys_mib->mac_ctrl.pbcn, 512);

	return res;
}


