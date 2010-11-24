/******************************************************************************
* rtl871x_eeprom.c                                                                                                                                 *
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
#define _RTL871X_EEPROM_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#ifdef CONFIG_RTL8192U
#include <rtl8192u_spec.h>
#include <rtl8192u_eeprom.h>
#include <rtl8192u/rtl8192u_haldef.h>
#endif

#define RTL8187_REQT_READ 0xc0

VOID 
RaiseClock(
	PADAPTER	Adapter,
	u2Byte		*x
	)
{
	*x = *x | EESK;
	PlatformEFIOWrite1Byte(Adapter, CSR_EEPROM_CONTROL_REG, (u1Byte)(*x));
	udelay_os(CLOCK_RATE);
}

VOID 
LowerClock(
	PADAPTER	Adapter,
	u2Byte		*x
	)
{
	*x = *x & ~EESK;
	PlatformEFIOWrite1Byte(Adapter, CSR_EEPROM_CONTROL_REG, (u1Byte)(*x));
	udelay_os(CLOCK_RATE);
}

VOID 
ShiftOutBits(
	PADAPTER	Adapter,
	u2Byte		data,
	u2Byte		count
	)
{
	u2Byte x,mask;

	mask = 0x01 << (count - 1);
	x = PlatformEFIORead1Byte(Adapter, CSR_EEPROM_CONTROL_REG);

	x &= ~(EEDO | EEDI);

	do
	{
		x &= ~EEDI;
		if(data & mask)
			x |= EEDI;

		PlatformEFIOWrite1Byte(Adapter, CSR_EEPROM_CONTROL_REG, (u1Byte)x);
		udelay_os(CLOCK_RATE);
		RaiseClock(Adapter, &x);
		LowerClock(Adapter, &x);
		mask = mask >> 1;
	} while(mask);

	x &= ~EEDI;
	PlatformEFIOWrite1Byte(Adapter, CSR_EEPROM_CONTROL_REG, (u1Byte)x);
}

u2Byte ShiftInBits(PADAPTER	Adapter)
{
	u2Byte x,d,i;
	x = PlatformEFIORead1Byte(Adapter, CSR_EEPROM_CONTROL_REG);

	x &= ~( EEDO | EEDI);
	d = 0;

	for(i=0; i<16; i++)
	{
		d = d << 1;
		RaiseClock(Adapter, &x);

		x = PlatformEFIORead1Byte(Adapter, CSR_EEPROM_CONTROL_REG);

		x &= ~(EEDI);
		if(x & EEDO)
		d |= 1;

		LowerClock(Adapter, &x);
	}

	return d;
}

VOID 
EEpromCleanup(
	PADAPTER	Adapter
	)
{
	u2Byte x;
	x = PlatformEFIORead1Byte(Adapter, CSR_EEPROM_CONTROL_REG);

	x &= ~(EECS | EEDI);
	PlatformEFIOWrite1Byte(Adapter, CSR_EEPROM_CONTROL_REG, (u1Byte)x);

	RaiseClock(Adapter, &x);
	LowerClock(Adapter, &x);
	// Return to the normal state. Added by Bruce, 2007-03-06.
	x &= ~(EEM1 | EEM0);
	PlatformEFIOWrite1Byte(Adapter, CSR_EEPROM_CONTROL_REG, (u1Byte)x);
}


u16 eeprom_read16(_adapter * padapter, u16 reg) //ReadEEprom
{
	u16 data=0;
	u2Byte x;

_func_enter_;		

	// select EEPROM, reset bits, set EECS
	x = read8(padapter, CSR_EEPROM_CONTROL_REG);

	x &= ~(EEDI | EEDO | EESK | EEM0);
	x |= EEM1 | EECS;
	write8(padapter, CSR_EEPROM_CONTROL_REG, (u1Byte)x);

	// write the read opcode and register number in that order
	// The opcode is 3bits in length, reg is 6 bits long
	ShiftOutBits(padapter, EEPROM_READ_OPCODE, 3);
	ShiftOutBits(padapter, reg, padapter->EepromAddressSize);

	// Now read the data (16 bits) in from the selected EEPROM word
	data = ShiftInBits(padapter);

	EEpromCleanup(padapter);

_func_exit_;

	return data;
}

VOID read_eeprom_content(_adapter *	padapter	)
{
	u16		val16;
	int i;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;

_func_enter_;		
	DEBUG_ERR(("+read_eeprom_content"));

	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto exit;
	}

	// Read eeprom ID
	peeprompriv->eepromid = eeprom_read16(padapter, 0);
	if( peeprompriv->eepromid != RTL8190_EEPROM_ID )
		DEBUG_ERR(("EEPROM ID is invalid : %4x",peeprompriv->eepromid));

	for(i=0 ; i<6; i+=2){
		u16 Tmp =eeprom_read16(padapter, ( (EEPROM_MAC_ADDRESS+i) >> 1));// Read_EEprom87B(dev, ( (EEPROM_MAC_ADDRESS+i) >> 1));
		peeprompriv->mac_addr[i]= Tmp &0xff;
		peeprompriv->mac_addr[i+1]=  (Tmp &0xff00)>>8;
	}

	if(	peeprompriv->mac_addr[0]==0xff && 	peeprompriv->mac_addr[1]==0xff && 
		peeprompriv->mac_addr[2]==0xff && 	peeprompriv->mac_addr[3]==0xff && 
		peeprompriv->mac_addr[4]==0xff && 	peeprompriv->mac_addr[5]==0xff)
	{
		peeprompriv->mac_addr[0]= 0x00;
		peeprompriv->mac_addr[1]= 0xe0;
		peeprompriv->mac_addr[2]= 0x4c;
		peeprompriv->mac_addr[3]= 0x01;
		peeprompriv->mac_addr[4]= 0x14;
		peeprompriv->mac_addr[5]= 0x30;
	}
	
	DEBUG_ERR(("\nCard MAC address is \n"MAC_FMT,MAC_ARG(peeprompriv->mac_addr)));

	val16 = 	eeprom_read16(padapter, EEPROM_CHANNEL_PLAN>>1);
	peeprompriv->channel_plan = val16&0xff;

	peeprompriv->VersionID = (VERSION_819xUsb)((val16&0xff00)>>8);
	peeprompriv->bTXPowerDataReadFromEEPORM = _TRUE;
	peeprompriv->RF_Type = RTL819X_DEFAULT_RF_TYPE;	// default : 1T2R
	peeprompriv->RFChipID = RF_8256;

	DEBUG_ERR(("read_eeprom_content: RFChipID = RF_8256 \n"));

	DEBUG_ERR(("read_eeprom_content: RF Type = 1T 2R \n"));

	if(peeprompriv->VersionID == VERSION_819xUsb_A)
	{
		u16 EEPROMTxPowerLevelCCK;
		u16 EEPROMTxPowerLevelOFDM24G[3];
		
		EEPROMTxPowerLevelCCK = (eeprom_read16(padapter, (u2Byte) ((EEPROM_TxPwIndex_CCK)>>1) ) & 0xff00) >> 8;

		EEPROMTxPowerLevelOFDM24G[0] = eeprom_read16(padapter, (u2Byte) ((EEPROM_TxPwIndex_OFDM_24G)>>1) ) & 0x00ff;

		EEPROMTxPowerLevelOFDM24G[1] = (eeprom_read16(padapter, (u2Byte) ((EEPROM_TxPwIndex_OFDM_24G+1)>>1) ) & 0xff00) >> 8;

		EEPROMTxPowerLevelOFDM24G[2] = eeprom_read16(padapter, (u2Byte) ((EEPROM_TxPwIndex_OFDM_24G+2)>>1) ) & 0x00ff;

		for(i=0; i<14; i++)
		{
			if(i<=3)
				peeprompriv->tx_power_g[i] = EEPROMTxPowerLevelOFDM24G[0];
			else if(i>=4 && i<=9)
				peeprompriv->tx_power_g[i] = EEPROMTxPowerLevelOFDM24G[1];
			else
				peeprompriv->tx_power_g[i] = EEPROMTxPowerLevelOFDM24G[2];
			DEBUG_INFO(("OFDM 2.4G Tx Power Level, Index %d = 0x%02x\n", i, EEPROMTxPowerLevelOFDM24G[i]));
		}

		for(i=0; i<14; i++)
		{
			if(i<=3)
				peeprompriv->tx_power_b[i] = 
					EEPROMTxPowerLevelOFDM24G[0] + (EEPROMTxPowerLevelCCK - EEPROMTxPowerLevelOFDM24G[1]);
			else if(i>=4 && i<=9)
				peeprompriv->tx_power_b[i] = EEPROMTxPowerLevelCCK;
			else
				peeprompriv->tx_power_b[i] = 
					EEPROMTxPowerLevelOFDM24G[2] + (EEPROMTxPowerLevelCCK - EEPROMTxPowerLevelOFDM24G[1]);
			DEBUG_INFO(("CCK 2.4G Tx Power Level, Index %d = 0x%02x\n", i, EEPROMTxPowerLevelOFDM24G[i]));
		}
	}
	peeprompriv->EEPROMThermalMeter = (u1Byte)((eeprom_read16(padapter, (EEPROM_ThermalMeter>>1))) & 0x000f);

	peeprompriv->TxPowerDiff = (((eeprom_read16(padapter, (EEPROM_PwDiff>>1)))&0x0f00)>>8);
	peeprompriv->CrystalCap=(eeprom_read16(padapter,(EEPROM_CrystalCap>>1))&0x000f);
	
	 peeprompriv->country_string[0]=0x55;
	 peeprompriv->country_string[1]=0x53;
	 peeprompriv->country_string[2]=0x20;


exit:
	DEBUG_ERR(("-read_eeprom_content\n"));

_func_exit_;		
}



