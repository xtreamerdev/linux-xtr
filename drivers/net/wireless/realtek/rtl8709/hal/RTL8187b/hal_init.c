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

#include <rtl8187/wlan_mlme.h>
#include <rtl8187/wlan_sme.h>
#include <rtl8187/wlan.h>

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

static void mac_config87b(_adapter * padapter)//MacConfig_87BASIC(struct net_device *dev)
{
	config87b_asic(padapter);
	//============================================================================

	// Follow TID_AC_MAP of WMac.
	//PlatformEFIOWrite2Byte(dev, TID_AC_MAP, 0xfa50);
	write16(padapter, TID_AC_MAP, 0xfa50);

	// Interrupt Migration, Jong suggested we use set 0x0000 first, 2005.12.14, by rcnjko.
	write16(padapter, INT_MIG, 0x0000);

	// Prevent TPC to cause CRC error. Added by Annie, 2006-06-10.
	write32(padapter, 0x1F0, 0x00000000);
	write32(padapter, 0x1F4, 0x00000000);
	write8(padapter, 0x1F8, 0x00);

	// For WiFi 5.2.2.5 Atheros AP performance. Added by Annie, 2006-06-12.
	// PlatformIOWrite4Byte(dev, RFTiming, 0x0008e00f);
	// Asked for by SD3 CM Lin, 2006.06.27, by rcnjko.
	write32(padapter, RFTiming, 0x00004001);

	// Asked for by WCChu and Victor, always set MAC.24E to 1 in 8xB D-cut or latter version. 060927, by rcnjko.
	if((padapter->dvobjpriv.ishighspeed)||(padapter->registrypriv.chip_version>VERSION_8187B_B))
		write8(padapter, 0x24E, 0x01);
}

//
// Description:
//		Initialize RFE and read Zebra2 version code.
//
//	2005-08-01, by Annie.
//
void setup_rfe_initialtiming(_adapter *padapter)
{

	// setup initial timing for RFE
	// Set VCO-PDN pin.
	write16(padapter, RFPinsOutput, 0x0480);
	write16(padapter, RFPinsSelect, 0x2488);
	write16(padapter, RFPinsEnable, 0x1FFF);
	//mdelay(100);
	msleep_os(100);

	// Steven recommends: delay 1 sec for setting RF 1.8V. by Annie, 2005-04-28.
	//mdelay(1000);
	msleep_os(1000);

	//
	// TODO: Read Zebra version code if necessary.
	//
//	priv->rf_chip = RF_ZEBRA2;
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



void phyconfig8187(_adapter *padapter)
{
	u8			val8;
	u32			i;
	u32			addr,data;
	u32			reg_offset, reg_val;

	val8= read8(padapter, CONFIG4);
	//priv->RFProgType = (val8 & 0x03);

	DEBUG_ERR(("\n phyconfig8187= CONFIG4&0x03=%x\n",(val8 & 0x03)));


	//=============================================================================
	// RADIOCFG.TXT
	//=============================================================================
	write_rtl8225(padapter, 0x00, 0x00b7);			mdelay_os(1);
	write_rtl8225(padapter, 0x01, 0x0ee0);			mdelay_os(1);
	write_rtl8225(padapter, 0x02, 0x044d);			mdelay_os(1);
	write_rtl8225(padapter, 0x03, 0x0441);			mdelay_os(1);
	write_rtl8225(padapter, 0x04, 0x08c3);			mdelay_os(1);
	write_rtl8225(padapter, 0x05, 0x0c72);			mdelay_os(1);
	write_rtl8225(padapter, 0x06, 0x00e6);			mdelay_os(1);
	write_rtl8225(padapter, 0x07, 0x082a);			mdelay_os(1);
	write_rtl8225(padapter, 0x08, 0x003f);			mdelay_os(1);
	write_rtl8225(padapter, 0x09, 0x0335);			mdelay_os(1);
	write_rtl8225(padapter, 0x0a, 0x09d4);			mdelay_os(1);
	write_rtl8225(padapter, 0x0b, 0x07bb);			mdelay_os(1);
	write_rtl8225(padapter, 0x0c, 0x0850);			mdelay_os(1);
	write_rtl8225(padapter, 0x0d, 0x0cdf);			mdelay_os(1);
	write_rtl8225(padapter, 0x0e, 0x002b);			mdelay_os(1);
	write_rtl8225(padapter, 0x0f, 0x0114);			mdelay_os(1);
	
	write_rtl8225(padapter, 0x00, 0x01b7);			mdelay_os(1);


	for(i=1;i<=95;i++)
	{
		write_rtl8225(padapter, 0x01, i);	mdelay_os(1); 

		//
		// For external LNA Rx performance. 
		// Added by Roger, 2007.03.30.
		//
		if( !padapter->eeprompriv.blow_noise_amplifier){//Non LNA settings.
			write_rtl8225(padapter, 0x02, ZEBRA_RF_RX_GAIN_TABLE[i]); 
			mdelay_os(1);
		}
		else {//LNA settings.
			write_rtl8225(padapter, 0x02, ZEBRA_RF_RX_GAIN_LNA_TABLE[i]); 
			mdelay_os(1);
		}
		DEBUG_INFO(("RF - 0x%x =  0x%x\n", i, ZEBRA_RF_RX_GAIN_TABLE[i]));
	}

	write_rtl8225(padapter, 0x03, 0x0082);			mdelay_os(1); 	// write reg 18
	write_rtl8225(padapter, 0x05, 0x0004);			mdelay_os(1);	// write reg 20
	write_rtl8225(padapter, 0x00, 0x00b7);			mdelay_os(1);	// switch to reg0-reg15
	msleep_os(1000);	
	msleep_os(400);
	write_rtl8225(padapter, 0x02, 0x0c4d);			mdelay_os(1);
	msleep_os(100);
	msleep_os(100);
	write_rtl8225(padapter, 0x02, 0x044d);			mdelay_os(1); 	
	write_rtl8225(padapter, 0x00, 0x02bf);			mdelay_os(1);	//0x002f disable 6us corner change,  06f--> enable
	
	//=============================================================================
	
	//=============================================================================
	// CCKCONF.TXT
	//=============================================================================
	/*
	u4bRegOffset=0x41;
	u4bRegValue=0xc8;
	
	//DbgPrint("\nCCK- 0x%x = 0x%x\n", u4bRegOffset, u4bRegValue);
	WriteBB(dev, (0x01000080 | (u4bRegOffset & 0x7f) | ((u4bRegValue & 0xff) << 8)));
	*/

	
	//=============================================================================

	//=============================================================================
	// Follow WMAC RTL8225_Config()
	//=============================================================================
//	//
//	// enable EEM0 and EEM1 in 9346CR
//	PlatformEFIOWrite1Byte(dev, CR9346, PlatformEFIORead1Byte(dev, CR9346)|0xc0);
//	// enable PARM_En in Config3
//	PlatformEFIOWrite1Byte(dev, CONFIG3, PlatformEFIORead1Byte(dev, CONFIG3)|0x40);	
//
//	PlatformEFIOWrite4Byte(dev, AnaParm2, ANAPARM2_ASIC_ON); //0x727f3f52
//	PlatformEFIOWrite4Byte(dev, AnaParm, ANAPARM_ASIC_ON); //0x45090658

	// power control
	write8(padapter, CCK_TXAGC, 0x03);
	write8(padapter, OFDM_TXAGC, 0x07);
	write8(padapter, ANTSEL, 0x03);

//	// disable PARM_En in Config3
//	PlatformEFIOWrite1Byte(dev, CONFIG3, PlatformEFIORead1Byte(dev, CONFIG3)&0xbf);
//	// disable EEM0 and EEM1 in 9346CR
//	PlatformEFIOWrite1Byte(dev, CR9346, PlatformEFIORead1Byte(dev, CR9346)&0x3f);
	//=============================================================================

	//=============================================================================
	// AGC.txt
	//=============================================================================
	//write_nic_dword( dev, PhyAddr, 0x00001280);	// Annie, 2006-05-05
	//write_phy_ofdm( dev, 0x00, 0x12);	// David, 2006-08-01
	//write_phy_ofdm( dev, 0x80, 0x12);	// David, 2006-08-09
	write32(padapter, PhyAddr, 0x00001280); //// Annie, 2006-05-05

	for (i=0; i<128; i++)
	{
		DEBUG_INFO(("AGC - [%x+1] = 0x%x\n", i, ZEBRA_AGC[i+1]));
		
		data = ZEBRA_AGC[i+1];
		data = data << 8;
		data = data | 0x0000008F;

		addr = i + 0x80; //enable writing AGC table
		addr = addr << 8;
		addr = addr | 0x0000008E;

		write32(padapter,PhyAddr, data); 	       
		write32(padapter,PhyAddr, addr); 	    
		write32(padapter,PhyAddr, 0x0000008E);
	}

	write32(padapter, PhyAddr, 0x00001080);	// Annie, 2006-05-05
	//write_phy_ofdm( dev, 0x00, 0x10);	// David, 2006-08-01
	//write_phy_ofdm( dev, 0x80, 0x10);	// David, 2006-08-09	

	//=============================================================================
	
	//=============================================================================
	// OFDMCONF.TXT
	//=============================================================================

	for(i=0; i<60; i++)
	{
		reg_offset=i;

		//
		// For external LNA Rx performance. 
		// Added by Roger, 2007.03.30.
		//
		if( padapter->eeprompriv.blow_noise_amplifier)
		{// LNA, we use different OFDM configurations.
			reg_val=OFDM_CONFIG_LNA[i];
		}
		else
		{// Non LNA, we use default OFDM configurations.			
			reg_val=OFDM_CONFIG[i];
		}

		write32(padapter,PhyAddr,(0x00000080 | (reg_offset & 0x7f) | ((reg_val & 0xff) << 8)));
	}


	//=============================================================================


	DEBUG_ERR(("\n phyconfig8187 ok!!\n"));
	return ;
}
void init_extra_regs(_adapter *padapter){//InitializeExtraRegsOn8185(dev);
	u8	bUNIVERSAL_CONTROL_RL = _FALSE; // Enable per-packet tx retry, 2005.03.31, by rcnjko.
	u8	bUNIVERSAL_CONTROL_AGC = _TRUE;
	u8	bUNIVERSAL_CONTROL_ANT = _TRUE;
	u8	bAUTO_RATE_FALLBACK_CTL = _TRUE;
	u8	val8;

	// Set up ACK rate.
	// Suggested by wcchu, 2005.08.25, by rcnjko.
	// 1. Initialize (MinRR, MaxRR) to (6,24) for A/G.
	// 2. MUST Set RR before BRSR.
	// 3. CCK must be basic rate.
	//if((ieee->mode == IEEE_G)||(ieee->mode == IEEE_A))
	{
		write16(padapter, BRSR_8187B, 0x0fff);
	}
	//else
	//{
	//	write_nic_word(dev, BRSR_8187B, 0x000f);
	//}

	// Retry limit
			
	val8 =read8(padapter, CW_CONF); 
	if(bUNIVERSAL_CONTROL_RL)
	{
		val8 &= (~CW_CONF_PERPACKET_RETRY_LIMIT); 
	}
	else
	{
		val8 |= CW_CONF_PERPACKET_RETRY_LIMIT; 
	}
	write8(padapter, CW_CONF, val8);

	// Tx AGC
	val8=read8(padapter, TX_AGC_CTL);
	if(bUNIVERSAL_CONTROL_AGC)
	{
		val8 &= (~TX_AGC_CTL_PER_PACKET_TXAGC);
		write8(padapter, CCK_TXAGC, 128);
		write8(padapter, OFDM_TXAGC,128);
	}
	else
	{
		val8 |= TX_AGC_CTL_PER_PACKET_TXAGC;
	}
	write8(padapter, TX_AGC_CTL, val8);

	// Tx Antenna including Feedback control
	val8 =read8(padapter, TX_AGC_CTL);

	if(bUNIVERSAL_CONTROL_ANT)
	{
		write8(padapter, ANTSEL, 0x00);
		val8 &= (~TXAGC_CTL_PER_PACKET_ANT_SEL);
	}
	else
	{
		val8 |= TXAGC_CTL_PER_PACKET_ANT_SEL;
	}
	write8(padapter, TX_AGC_CTL, val8);

	// Auto Rate fallback control
	val8 = read8(padapter, RATE_FALLBACK);
	if( bAUTO_RATE_FALLBACK_CTL )
	{
		val8 |= RATE_FALLBACK_CTL_ENABLE | RATE_FALLBACK_CTL_AUTO_STEP0;

		// <RJ_TODO_8187B> We shall set up the ARFR according to user's setting.
		write16(padapter, ARFR, 0x0fff);
	}
	else
	{
		val8 &= (~RATE_FALLBACK_CTL_ENABLE);
	}
	write8(padapter, RATE_FALLBACK,val8);

	return;
}  //end initialize extra 8185

void cam_mark_invalid(_adapter *padapter, u8 ucIndex)
{
	u32 content=0;
	u32 cmd=0;
	u32 algo=_AES_;

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
	u32 cmd=0;
	u32 content=0;
	u8 i;	
	u32 algo=_AES_;

	for(i=0;i<6;i++)
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
	u8 idx;
	
	//2004/02/11  In static WEP, OID_ADD_KEY or OID_ADD_WEP are set before STA associate to AP.
	// However, ResetKey is called on OID_802_11_INFRASTRUCTURE_MODE and MlmeAssociateRequest
	// In this condition, Cam can not be reset because upper layer will not set this static key again.
	//if(Adapter->EncAlgorithm == WEP_Encryption)
	//	return;
//debug
	//DbgPrint("========================================\n");
	//DbgPrint("				Call ResetAllEntry						\n");
	//DbgPrint("========================================\n\n");

	for(idx=0;idx<TOTAL_CAM_ENTRY;idx++)
		cam_mark_invalid(padapter, idx);
	for(idx=0;idx<TOTAL_CAM_ENTRY;idx++)
		cam_empty_entry(padapter, idx);

}


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


void Set_HW_ACPARAM(
		_adapter *padapter,
		PAC_PARAM pAcParam, 
		PCHANNEL_ACCESS_SETTING ChnlAccessSetting)
{
	AC_CODING	eACI;
	u8			u1bAIFS;
	u32			u4bAcParam;
	PACI_AIFSN	pAciAifsn	= (PACI_AIFSN)(&pAcParam->f.AciAifsn);
	u8			AcmCtrl		= padapter->dvobjpriv.hw_info.acm_ctrl;

	// Retrive paramters to udpate.
	eACI		= pAcParam->f.AciAifsn.f.ACI; 
	u1bAIFS		= pAcParam->f.AciAifsn.f.AIFSN * ChnlAccessSetting->SlotTimeTimer + aSifsTime -14; 
	u4bAcParam	= (	(((u32)(pAcParam->f.TXOPLimit)) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
					(((u32)(pAcParam->f.Ecw.f.ECWmax)) << AC_PARAM_ECW_MAX_OFFSET)	|  
					(((u32)(pAcParam->f.Ecw.f.ECWmin)) << AC_PARAM_ECW_MIN_OFFSET)	|  
					(((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET));

	switch(eACI)
	{
		case AC1_BK:
			write32(padapter, AC_BK_PARAM, u4bAcParam);
			break;

		case AC0_BE:
			write32(padapter, AC_BE_PARAM, u4bAcParam);
			break;

		case AC2_VI:
			write32(padapter, AC_VI_PARAM, u4bAcParam);
			break;

		case AC3_VO:
			write32(padapter, AC_VO_PARAM, u4bAcParam);
			break;

		default:
			DEBUG_ERR((KERN_WARNING "SetHwReg8185(): invalid ACI: %d !\n", eACI));
			break;
	}

	// Cehck ACM bit.
	// If it is set, immediately set ACM control bit to downgrading AC for passing WMM testplan. Annie, 2005-12-13.

	if( pAciAifsn->f.ACM )
	{ // ACM bit is 1.
		switch(eACI)
		{
			case AC0_BE:
				AcmCtrl |= (BEQ_ACM_EN|BEQ_ACM_CTL|ACM_HW_EN);	// or 0x21
				break;

			case AC2_VI:
				AcmCtrl |= (VIQ_ACM_EN|VIQ_ACM_CTL|ACM_HW_EN);		// or 0x42
				break;

			case AC3_VO:
				AcmCtrl |= (VOQ_ACM_EN|VOQ_ACM_CTL|ACM_HW_EN);	// or 0x84
				break;

			default:
				DEBUG_ERR((KERN_WARNING "SetHwReg8185(): [HW_VAR_ACM_CTRL] ACM set failed: eACI is %d\n", eACI ));
				break;
		}
	}
	else
	{ // ACM bit is 0.
		switch(eACI)
		{
			case AC0_BE:
				AcmCtrl &= ( (~BEQ_ACM_EN) & (~BEQ_ACM_CTL) & (~ACM_HW_EN) );	// and 0xDE
				break;

			case AC2_VI:
				AcmCtrl &= ( (~VIQ_ACM_EN) & (~VIQ_ACM_CTL) & (~ACM_HW_EN) );		// and 0xBD
				break;

			case AC3_VO:
				AcmCtrl &= ( (~VOQ_ACM_EN) & (~VOQ_ACM_CTL) & (~ACM_HW_EN) );	// and 0x7B
				break;

			default:
				break;
		}
	}

	padapter->dvobjpriv.hw_info.acm_ctrl = AcmCtrl;

	write8(padapter, ACM_CONTROL, AcmCtrl);

}


void ActUpdateChannelAccessSetting(_adapter *padapter,
		int WirelessMode)
{
	AC_CODING	eACI;
	AC_PARAM	AcParam;
	PCHANNEL_ACCESS_SETTING ChnlAccessSetting = (PCHANNEL_ACCESS_SETTING)_malloc(sizeof(CHANNEL_ACCESS_SETTING));

	switch( WirelessMode )
	{
		case WIRELESS_11A:
			ChnlAccessSetting->SIFS_Timer = 0x22;
			ChnlAccessSetting->DIFS_Timer = 34; // 34 = 16 + 2*9. 2006.06.07, by rcnjko.
			ChnlAccessSetting->SlotTimeTimer = 9;
			ChnlAccessSetting->EIFS_Timer = 23;
			ChnlAccessSetting->CWminIndex = 4;
			ChnlAccessSetting->CWmaxIndex = 10;
			break;

		case WIRELESS_11B:
			ChnlAccessSetting->SIFS_Timer = 0x22;
			ChnlAccessSetting->DIFS_Timer = 50; // 50 = 10 + 2*20. 2006.06.07, by rcnjko.
			ChnlAccessSetting->SlotTimeTimer = 20;
			ChnlAccessSetting->EIFS_Timer = 91;
			ChnlAccessSetting->CWminIndex = 5;
			ChnlAccessSetting->CWmaxIndex = 10;
			break;

		case WIRELESS_11G:
		case WIRELESS_11BG:
			//
			// <RJ_TODO_8185B> 
			// TODO: We still don't know how to set up these registers, just follow WMAC to 
			// verify 8185B FPAG.
			//
			// <RJ_TODO_8185B>
			// Jong said CWmin/CWmax register are not functional in 8185B, 
			// so we shall fill channel access realted register into AC parameter registers,
			// even in nQBss.
			//
			ChnlAccessSetting->SIFS_Timer = 0x22; // Suggested by Jong, 2005.12.08.
			ChnlAccessSetting->SlotTimeTimer = 9; // 2006.06.07, by rcnjko.
			ChnlAccessSetting->DIFS_Timer = 28; // 28 = 10 + 2*9. 2006.06.07, by rcnjko.
			ChnlAccessSetting->EIFS_Timer = 0x5B; // Suggested by wcchu, it is the default value of EIFS register, 2005.12.08.
#ifdef TODO
			switch (Adapter->NdisUsbDev.CWinMaxMin)
#else
			switch (2)				
#endif
			{
				case 0:// 0: [max:7 min:1 ]
					ChnlAccessSetting->CWminIndex = 1;
					ChnlAccessSetting->CWmaxIndex = 7;
					break;
				case 1:// 1: [max:7 min:2 ]
					ChnlAccessSetting->CWminIndex = 2;
					ChnlAccessSetting->CWmaxIndex = 7;
					break;
				case 2:// 2: [max:7 min:3 ]
					ChnlAccessSetting->CWminIndex = 3;
					ChnlAccessSetting->CWmaxIndex = 7;
					break;
				case 3:// 3: [max:9 min:1 ]
					ChnlAccessSetting->CWminIndex = 1;
					ChnlAccessSetting->CWmaxIndex = 9;
					break;
				case 4:// 4: [max:9 min:2 ]
					ChnlAccessSetting->CWminIndex = 2;
					ChnlAccessSetting->CWmaxIndex = 9;
					break;
				case 5:// 5: [max:9 min:3 ]
					ChnlAccessSetting->CWminIndex = 3;
					ChnlAccessSetting->CWmaxIndex = 9;
					break;
				case 6:// 6: [max:A min:5 ]
					ChnlAccessSetting->CWminIndex = 5;
					ChnlAccessSetting->CWmaxIndex = 10;
					break;
				case 7:// 7: [max:A min:4 ]
					ChnlAccessSetting->CWminIndex = 4;
					ChnlAccessSetting->CWmaxIndex = 10;
					break;

				default:
					ChnlAccessSetting->CWminIndex = 1;
					ChnlAccessSetting->CWmaxIndex = 7;
					break;
			}

			break;
	}

	DEBUG_INFO(("sifs=%d difs=%d slottime=%d eifs = %d cwmin=%d cwmax=%d\n",ChnlAccessSetting->SIFS_Timer, 
		ChnlAccessSetting->DIFS_Timer,ChnlAccessSetting->SlotTimeTimer, ChnlAccessSetting->EIFS_Timer,
		ChnlAccessSetting->CWminIndex, ChnlAccessSetting->CWmaxIndex));
	write8(padapter, SIFS, ChnlAccessSetting->SIFS_Timer);
	//update slot time related by david, 2006-7-21
	write8(padapter, SLOT, ChnlAccessSetting->SlotTimeTimer);// Rewrited from directly use PlatformEFIOWrite1Byte(), by Annie, 2006-03-29.
#ifdef TODO
	if(pStaQos->CurrentQosMode > QOS_DISABLE)
	{
		for(eACI = 0; eACI < AC_MAX; eACI++)
		{
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, \
				(pu1Byte)(&(pStaQos->WMMParamEle.AcParam[eACI])) );
		}
	}
	else
#endif		
	{
		u8 u1bAIFS = aSifsTime + (2 * ChnlAccessSetting->SlotTimeTimer) - 14;//PHY_DELAY is 14

		write8(padapter, AC_VO_PARAM, u1bAIFS);
		write8(padapter, AC_VI_PARAM, u1bAIFS);
		write8(padapter, AC_BE_PARAM, u1bAIFS);
		write8(padapter, AC_BK_PARAM, u1bAIFS);
	}
//}

	write8(padapter, EIFS_8187B, ChnlAccessSetting->EIFS_Timer);
	write8(padapter, AckTimeOutReg, 0x5B); // <RJ_EXPR_QOS> Suggested by wcchu, it is the default value of EIFS register, 2005.12.08.
#ifdef TODO
	// <RJ_TODO_NOW_8185B> Update ECWmin/ECWmax, AIFS, TXOP Limit of each AC to the value defined by SPEC.
	if( pStaQos->CurrentQosMode > QOS_DISABLE )
	{ // QoS mode.
		if(pStaQos->QBssWirelessMode == WirelessMode)
		{
			// Follow AC Parameters of the QBSS.
			for(eACI = 0; eACI < AC_MAX; eACI++)
			{
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, (pu1Byte)(&(pStaQos->WMMParamEle.AcParam[eACI])) );
			}
		}
		else
		{
			// Follow Default WMM AC Parameters.
			bFollowLegacySetting = TRUE;
		}
	}
	else
	{ // Legacy 802.11.
		bFollowLegacySetting = TRUE;
	}

	if(bFollowLegacySetting)
#endif

	if(_TRUE)
	{
		//
		// Follow 802.11 seeting to AC parameter, all AC shall use the same parameter.
		// 2005.12.01, by rcnjko.
		//		
		AcParam.longData = 0;
		AcParam.f.AciAifsn.f.AIFSN = 2; // Follow 802.11 DIFS.
		AcParam.f.AciAifsn.f.ACM = 0;
		AcParam.f.Ecw.f.ECWmin = ChnlAccessSetting->CWminIndex; // Follow 802.11 CWmin.
		AcParam.f.Ecw.f.ECWmax = ChnlAccessSetting->CWmaxIndex; // Follow 802.11 CWmax.
		AcParam.f.TXOPLimit = 0;

		for(eACI = 0; eACI < AC3_VO; eACI++)
		{
			AcParam.f.AciAifsn.f.ACI = (u8)eACI;
			{
				Set_HW_ACPARAM(padapter,&AcParam,ChnlAccessSetting);
			}
		}

		// Use the CW defined in SPEC to prevent unfair Beacon distribution in AdHoc mode.
		// Only set this value to AC3_VO because the throughput shakes if other ACs set this value.
		// 2006.08.04, by shien chang.
		AcParam.f.AciAifsn.f.ACI = (u8)AC3_VO;		
		if( padapter->_sys_mib.mac_ctrl.opmode == WIFI_ADHOC_STATE) {
			AcParam.f.Ecw.f.ECWmin = 4; 
			AcParam.f.Ecw.f.ECWmax = 10;
		}
		Set_HW_ACPARAM(padapter,&AcParam,ChnlAccessSetting);
	}
}


void ActSetWirelessMode8187(_adapter *padapter, u8	btWirelessMode)//(struct net_device* dev, u8	btWirelessMode)
{
	//struct r8180_priv *priv = ieee80211_priv(dev);
	//struct ieee80211_device *ieee = priv->ieee80211;
	//PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);

	u8	btSupportedWirelessMode =WIRELESS_11BG;

	if( (btWirelessMode & btSupportedWirelessMode) == 0 )
	{ // Don't switch to unsupported wireless mode, 2006.02.15, by rcnjko.
		DEBUG_ERR((KERN_WARNING "ActSetWirelessMode8187(): WirelessMode(%d) is not supported (%d)!\n", 
				 btWirelessMode, btSupportedWirelessMode));
		return;
	}
#if 0
	// 1. Assign wireless mode to swtich if necessary.
	if( (btWirelessMode == WIRELESS_MODE_AUTO) || 
			(btWirelessMode & btSupportedWirelessMode) == 0 )
	{
		if((btSupportedWirelessMode & WIRELESS_11A))
		{
			btWirelessMode = WIRELESS_11A;
		}
		else if((btSupportedWirelessMode & WIRELESS_11G))
		{
			btWirelessMode = WIRELESS_11G;
		}
		else if((btSupportedWirelessMode & WIRELESS_11B))
		{
			btWirelessMode = WIRELESS_11B;
		}
		else
		{
			DEBUG_INFO((KERN_WARNING "MptActSetWirelessMode8187(): No valid wireless mode supported, \
					btSupportedWirelessMode(%x)!!!\n", btSupportedWirelessMode));
			btWirelessMode = WIRELESS_11B;
		}
	}
#endif
	// 2. Swtich band.

	//padapter->_sys_mib.cur_modem=btWirelessMode;

	
	//ActUpdateChannelAccessSetting(padapter,padapter->_sys_mib.cur_modem);
	ActUpdateChannelAccessSetting(padapter, btWirelessMode);


	if(btWirelessMode == WIRELESS_11B && padapter->_sys_mib.InitialGain > padapter->_sys_mib.RegBModeGainStage){
		padapter->_sys_mib.InitialGain = padapter->_sys_mib.RegBModeGainStage;
		UpdateInitialGain(padapter);
	}

}



void rtl8187_start(_adapter *padapter){

		u8 val8;
	//	u8 faketemp;		
		/**
		 *  IO part migrated from Windows Driver.
		 */
		//priv->irq_mask = 0xffff;
		// Enable Config3.PARAM_En.
		write8(padapter, CR9346, 0xC0);
		val8=read8(padapter, CONFIG3);
		write8(padapter, CONFIG3, (val8|CONFIG3_PARM_En));
		// Turn on Analog power.
		// Asked for by William, otherwise, MAC 3-wire can't work, 2006.06.27, by rcnjko.
		write32(padapter, ANA_PARAM2, ANAPARM2_ASIC_ON);
		write32(padapter, ANA_PARAM, ANAPARM_ASIC_ON);
		write8(padapter, ANA_PARAM3,  0x00);

		// 070111: Asked for by William to reset PLL.
		{
			u8 reg62;
			write8(padapter, 0xff61, 0x10);
			reg62=read8(padapter, 0xff62);
			write8(padapter, 0xff62,  (reg62 & (~(BIT(5)))));
			write8(padapter, 0xff62, (reg62 | (BIT(5))));
		}
		write8(padapter,CONFIG3, (val8&(~CONFIG3_PARM_En)));
		write8(padapter, CR9346, 0x00);

		//-----------------------------------------------------------------------------
		// Set up MAC registers. 
		//-----------------------------------------------------------------------------

		//
		// Init multicast register to recv All.
		//
		if((read8(padapter, 0xFE11)&0x01))
			padapter->dvobjpriv.ishighspeed =_TRUE;
		else
			padapter->dvobjpriv.ishighspeed=_FALSE;

		write32(padapter, 0x210, 0xffffffff);
		write32(padapter,  0x214, 0xffffffff);

		val8=read8(padapter, PSR);
		DEBUG_ERR(("\n Initial PSR=0x%x\n",val8));
		
		

		// Get hardware version
		val8= read8(padapter, RTL8187B_VER_REG);
		switch(val8)
		{
		case 0x00:
			padapter->registrypriv.chip_version=VERSION_8187B_B;
			DEBUG_ERR(("\nHardware version is 8187B_B\n"));
			break;

		case 0x01:
			padapter->registrypriv.chip_version=VERSION_8187B_D;
			DEBUG_ERR(("\nHardware version is 8187B_D\n"));
			break;

		case 0x02:
			padapter->registrypriv.chip_version= VERSION_8187B_E;
			DEBUG_ERR(("\nHardware version is 8187B_E\n"));
			break;

		default:
			padapter->registrypriv.chip_version= VERSION_8187B_B;
			DEBUG_ERR(("\nHardware version is 8187B_B\n"));
			break;
		}
		// Change some setting according to IC version.
		//ChangeHalDefSettingByIcVersion(Adapter); if linux driver define RegPaModel ?

		// Issue Tx/Rx reset
		write8(padapter, CMD, CR_RST);
		// wait after the port reset command
		//PlatformStallExecution(20); xp delay 20us ?
		//mdelay(200); //linux delay 200ms 
		udelay_os(20);

		write8(padapter, CR9346, 0xc0);// enable config register write

		init_extra_regs(padapter);
		
		
		padapter->dvobjpriv.hw_info.tcr=TCR_DurProcMode |	//for RTL8185B, duration setting by HW
		TCR_DISReqQsize |
                (TCR_MXDMA_2048<<TCR_MXDMA_SHIFT)|  // Max DMA Burst Size per Tx DMA Burst, 7: reservied.
		(7<<TX_SRLRETRY_SHIFT)|	// Short retry limit
		(7<<TX_LRLRETRY_SHIFT) ;	// Long retry limit

		
		write32(padapter, TCR, padapter->dvobjpriv.hw_info.tcr);  //TCR config    priv->TransmitConfig
		padapter->dvobjpriv.hw_info.tcr=read32(padapter, TCR);
		DEBUG_ERR(("\n RTL8187 init TCR:%x\n",padapter->dvobjpriv.hw_info.tcr));

		padapter->dvobjpriv.hw_info.rcr=	RCR_AMF | RCR_ADF |		//accept management/data
//		RCR_ACF |			//accept control frame for SW AP needs PS-poll, 2005.07.07, by rcnjko.
		RCR_AB | RCR_AM | RCR_APM |	//accept BC/MC/UC
		//RCR_AICV | RCR_ACRC32 | 	//accept ICV/CRC error packet
		RCR_MXDMA | // Max DMA Burst Size per Rx DMA Burst, 7: unlimited.
		(7<<RX_FIFO_THRESHOLD_SHIFT) | // Rx FIFO Threshold, 7: No Rx threshold.
		RCR_ONLYERLPKT;

		write32(padapter, RCR, padapter->dvobjpriv.hw_info.rcr);	//RCR  config priv->ReceiveConfig
		padapter->dvobjpriv.hw_info.rcr=read32(padapter, RCR);
		DEBUG_ERR(("\n RTL8187 init RCR:%x\n",padapter->dvobjpriv.hw_info.rcr));

		padapter->dvobjpriv.hw_info.msr=read8(padapter,MSR) & 0xf3 ;
		write8(padapter, MSR, padapter->dvobjpriv.hw_info.msr);  // default network type to 'No  Link'

		write8(padapter, ACM_CONTROL, 0x00);  //priv->AcmControl
	
		write16(padapter, BcnIntv,  100);
		write16(padapter, AtimWnd, 2);
		write16(padapter, FEMR, 0xFFFF);
		//LED TYPE
		//{
			//write_nic_byte(dev, CONFIG1,((read_nic_byte(dev, CONFIG1)&0x3f)|0x80));	//turn on bit 5:Clkrun_mode
		//}
		write8(padapter, CR9346, 0x0); // disable config register write

		mac_config87b(padapter);

		// Override the RFSW_CTRL (MAC offset 0x272-0x273), 2006.06.07, by rcnjko.
		write16(padapter, RFSW_CTRL, 0x569a);

		//-----------------------------------------------------------------------------

		//-----------------------------------------------------------------------------
		// Set up security related. 070106, by rcnjko:
		// 1. Clear all H/W keys.
		// 2. Enable H/W encryption/decryption.
		//-----------------------------------------------------------------------------
		//CamResetAllEntry(Adapter);
		//Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);

		cam_reset(padapter);	
		//hw security config
              write8(padapter, WPA_CONFIG, ( SCR_TxSecEnable | SCR_RxSecEnable |  SCR_UseDK) ); 

		write16(padapter, AESMSK_FC, AESMSK_FC_DEFAULT ); 
		mdelay_os(1);
		write16(padapter, AESMSK_SC, AESMSK_SC_DEFAULT );
		mdelay_os(1);

		// Write AESMSK_QC (Mask for Qos Control Field). Annie, 2005-12-14.
		write16(padapter, AESMSK_QC, AESMSK_QC_DEFAULT );	
		mdelay_os(1);
		
		//-----------------------------------------------------------------------------

		//-----------------------------------------------------------------------------
		// Set up PHY related. 
		//-----------------------------------------------------------------------------
		// Enable Config3.PARAM_En to revise AnaaParm.
		write8(padapter, CR9346,  0xC0);
		//write_nic_byte(dev, CONFIG3, read_nic_byte(dev,CONFIG3)|CONFIG3_PARM_En);
		write8(padapter, CONFIG3, CONFIG3_PARM_En);
		write8(padapter, CR9346, 0x0);

		// Initialize RFE and read Zebra2 version code. Added by Annie, 2005-08-01.
		setup_rfe_initialtiming(padapter);

		// PHY config.
		phyconfig8187(padapter);

		// We assume RegWirelessMode has already been initialized before, 
		// however, we has to validate the wireless mode here and provide a reasonble
		// initialized value if necessary. 2005.01.13, by rcnjko.


		//1 set wireless mode and set mac initialize value
		DEBUG_ERR(("\n padapter=%p\n",padapter));
		 ActSetWirelessMode8187(padapter, (u8)(WIRELESS_11G));
		DEBUG_ERR(("\n padapter=%p\n",padapter));

		// Enable tx/rx		
		write8(padapter, CMD, (CR_RE|CR_TE));// Using HW_VAR_COMMAND instead of writing CMDR directly. Rewrited by Annie, 2006-04-07.
		DEBUG_ERR(("\n padapter=%p\n",padapter));

	//3	rtl8187_rx_initiate(dev);   //will call later 
	//3	rtl8187_rx_manage_initiate(dev);   //move to later init
		//rtl8180_irq_enable(dev);
		//add this is for 8187B Rx stall problem
		DEBUG_ERR(("\n padapter=%p write FE41\n",padapter));
		write8(padapter, 0xfe41, 0xF4);//write_nic_byte_E(dev, 0x41, 0xF4);
		DEBUG_ERR(("\n padapter=%p write FE40\n",padapter));
		write8(padapter, 0xfe40, 0x00);
		DEBUG_ERR(("\n padapter=%p write FE42\n",padapter));
		write8(padapter, 0xfe42, 0x00);
		write8(padapter, 0xfe42, 0x01);
		write8(padapter, 0xfe40, 0x0F);
		write8(padapter, 0xfe42, 0x00);
		write8(padapter, 0xfe42, 0x01);

		// 8187B demo board MAC and AFE power saving parameters from SD1 William, 2006.07.20.
		// Parameter revised by SD1 William, 2006.08.21: 
		//	373h -> 0x4F // Original: 0x0F 
		//	377h -> 0x59 // Original: 0x4F 
		// Victor 2006/8/21: 筿把计某单SD3 蔼放 and cable link test OKタΑ release,ぃ某びе
		// 2006/9/5, Victor & ED: it is OK to use.
		//if(pHalData->RegBoardType == BT_DEMO_BOARD)
		//{

		
			// AFE.
			//
			// Revise the content of Reg0x372, 0x374, 0x378 and 0x37a to fix unusual electronic current 
			// while CTS-To-Self occurs, by DZ's request.
			// Modified by Roger, 2007.06.22.
			//
			write8(padapter, 0x0DB, (read8(padapter, 0x0DB)|BIT(2)));
			write16(padapter, 0x372, 0x59FA); // 0x4FFA-> 0x59FA.
			write16(padapter, 0x374, 0x59D2); // 0x4FD2-> 0x59D2.
			write16(padapter, 0x376, 0x59D2);
			write16(padapter, 0x378, 0x19FA); // 0x0FFA-> 0x19FA.
			write16(padapter, 0x37A, 0x19FA); // 0x0FFA-> 0x19FA.
			write16(padapter, 0x37C, 0x00D0);

			write8(padapter, 0x061, 0x00);

			// MAC.
			write8(padapter, 0x180, 0x0F);
			write8(padapter, 0x183, 0x03);
			// 061218, lanhsin: for victorh request
			write8(padapter, 0x0DA, 0x10);
		//}
	
		//
		// 061213, rcnjko: 
		// Set MAC.0x24D to 0x00 to prevent cts-to-self Tx/Rx stall symptom.
		// If we set MAC 0x24D to 0x08, OFDM and CCK will turn off 
		// if not in use, and there is a bug about this action when 
		// we try to send CCK CTS and OFDM data together.
		//
		//PlatformEFIOWrite1Byte(Adapter, 0x24D, 0x00);
		// 061218, lanhsin: for victorh request
		write8(padapter, 0x24D, 0x08);


		//
		// 061215, rcnjko:
		// Follow SD3 RTL8185B_87B_MacPhy_Register.doc v0.4.
		//
		write32(padapter, HSSI_PARA, 0x0600321B);
		//
		// 061226, rcnjko:
		// Babble found in HCT 2c_simultaneous test, server with 87B might 
		// receive a packet size about 2xxx bytes.
		// So, we restrict RMS to 2048 (0x800), while as IC default value is 0xC00.
		//
		write16(padapter, RMS, 0x0800);

		//*****20070321 Resolve HW page bug on system logo test
		val8=read8(padapter, 0x50);
		DEBUG_ERR(("\n reg 0x50  val8 =0x%x\n",val8));
	
}




/*! Initialize hardware  for RTL8187 Device. */
uint	 rtl8187_hal_init(_adapter *padapter) {

	uint status = _SUCCESS;
	
	_func_enter_;
	DEBUG_ERR(("\n +rtl8187_hal_init\n"));

	rtl8187_start(padapter);
	DEBUG_ERR(("\n -rtl8187_hal_init\n"));

	padapter->hw_init_completed=_TRUE;

	r8187_start_DynamicInitGain(padapter);
	r8187_start_RateAdaptive(padapter);

	_func_exit_;
	return status;
	
}

uint rtl8187_hal_deinit(_adapter *padapter)
{
	u8	bool;
	uint status = _SUCCESS;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
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
#ifdef CONFIG_RTL8192U

	while(1) {
		_cancel_timer(&_sys_mib->sendbeacon_timer, &bool);
		if(bool == _TRUE)
			break;
	}

#endif

	r8187_stop_DynamicInitGain(padapter);
	r8187_stop_RateAdaptive(padapter);

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



sint _init_mibinfo(_adapter* padapter)
{
	int i;
	sint res=_SUCCESS;
	unsigned int addr;
	struct mib_info* _sys_mib = &(padapter->_sys_mib);

	//init lock
	_spinlock_init(&_sys_mib->free_rxirp_mgt_lock);
	_spinlock_init(&_sys_mib->free_txirp_mgt_lock);
	 _init_sema(&_sys_mib->cam_sema,1);

	//init list
	_init_listhead(&(_sys_mib->free_rxirp_mgt));
	_init_listhead(&(_sys_mib->free_txirp_mgt));
	_init_listhead(&(_sys_mib->assoc_entry));

	//init timer
	_init_timer(&(_sys_mib->survey_timer), padapter->pnetdev, &survey_timer_hdl, padapter);
	_init_timer(&(_sys_mib->reauth_timer), padapter->pnetdev, &reauth_timer_hdl, padapter);
	_init_timer(&(_sys_mib->reassoc_timer), padapter->pnetdev, &reassoc_timer_hdl, padapter);
	_init_timer(&(_sys_mib->disconnect_timer), padapter->pnetdev, &disconnect_timer_hdl, padapter);


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
	_sys_mib->cur_channel = 6;

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


