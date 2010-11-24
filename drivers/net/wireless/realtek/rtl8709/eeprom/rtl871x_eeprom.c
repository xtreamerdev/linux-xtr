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


#ifdef CONFIG_RTL8711
#include <rtl8711_bitdef.h>
#endif

#ifdef CONFIG_RTL8711
void up_clk(_adapter*	padapter,	 u16 *x)
{
_func_enter_;
	*x = *x | EESK;
	write8(padapter, CR9346, (u8)*x);
	udelay_os(CLOCK_RATE);

_func_exit_;
	
}

void down_clk(_adapter *	padapter, u16 *x	)
{
_func_enter_;
	*x = *x & ~EESK;
	write8(padapter, CR9346, (u8)*x);
	udelay_os(CLOCK_RATE);
_func_exit_;	
}

void shift_out_bits(_adapter * padapter, u16 data, u16 count)
{
	u16 x,mask;
_func_enter_;

	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
	mask = 0x01 << (count - 1);
	x = read8(padapter, CR9346);

	x &= ~(EEDO | EEDI);

	do
	{
		x &= ~EEDI;
		if(data & mask)
			x |= EEDI;
		if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
		}
		write8(padapter, CR9346, (u8)x);
		udelay_os(CLOCK_RATE);
		up_clk(padapter, &x);
		down_clk(padapter, &x);
		mask = mask >> 1;
	} while(mask);
	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
	x &= ~EEDI;
	write8(padapter, CR9346, (u8)x);
out:	
_func_exit_;		
}

u16 shift_in_bits (_adapter * padapter)
{
	u16 x,d=0,i;
_func_enter_;	
	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
	x = read8(padapter, CR9346);

	x &= ~( EEDO | EEDI);
	d = 0;

	for(i=0; i<16; i++)
	{
		d = d << 1;
		up_clk(padapter, &x);
	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
		x = read8(padapter, CR9346);

		x &= ~(EEDI);
		if(x & EEDO)
		d |= 1;

		down_clk(padapter, &x);
	}
out:	
_func_exit_;		

	return d;
}

void standby(_adapter *	padapter	)
{
	u8   x;
_func_enter_;	
	x = read8(padapter, CR9346);

	x &= ~(EECS | EESK);
	write8(padapter, CR9346,x);

	udelay_os(CLOCK_RATE);
	x |= EECS;
	write8(padapter, CR9346, x);
	udelay_os(CLOCK_RATE);
_func_exit_;		
}

u16 wait_eeprom_cmd_done(_adapter* padapter)
{
	u8 	x;
	u16	i,res=_FALSE;
_func_enter_;	
	standby(padapter );
	for (i=0; i<200; i++) 
	{
		x = read8(padapter, CR9346);
		if (x & EEDO)
		{
			res=_TRUE;
			goto exit;
		}
		udelay_os(CLOCK_RATE);
	}
exit:
_func_exit_;
	return res;
}

void eeprom_clean(_adapter * padapter)
{
	u16 x;
_func_enter_;		
	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
	x = read8(padapter, CR9346);
	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
	x &= ~(EECS | EEDI);
	write8(padapter, CR9346, (u8)x);
	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
	up_clk(padapter, &x);
		if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
	down_clk(padapter, &x);
out:	
_func_exit_;			
}

void eeprom_write16(_adapter * padapter, u16 reg, u16 data)
{
	u8 x;
_func_enter_;		
	
	x = read8(padapter, CR9346);

	x &= ~(EEDI | EEDO | EESK | EEM0);
	x |= EEM1 | EECS;
	write8(padapter, CR9346, x);

	shift_out_bits(padapter, EEPROM_EWEN_OPCODE, 5);
	
	if(padapter->EepromAddressSize==8)	//CF+ and SDIO
		shift_out_bits(padapter, 0, 6);
	else									//USB
		shift_out_bits(padapter, 0, 4);
	
	standby( padapter);

// Commented out by rcnjko, 2004.0
//	// Erase this particular word.  Write the erase opcode and register
//	// number in that order. The opcode is 3bits in length; reg is 6 bits long.
//	shift_out_bits(Adapter, EEPROM_ERASE_OPCODE, 3);
//	shift_out_bits(Adapter, reg, Adapter->EepromAddressSize);
//
//	if (wait_eeprom_cmd_done(Adapter ) == FALSE) 
//	{
//		return;
//	}


	standby(padapter );

	// write the new word to the EEPROM

	// send the write opcode the EEPORM
	shift_out_bits(padapter, EEPROM_WRITE_OPCODE, 3);

	// select which word in the EEPROM that we are writing to.
	shift_out_bits(padapter, reg, padapter->EepromAddressSize);

	// write the data to the selected EEPROM word.
	shift_out_bits(padapter, data, 16);

	if (wait_eeprom_cmd_done(padapter ) == _FALSE) 
	{

		goto exit;
	}

	standby(padapter );

	shift_out_bits(padapter, EEPROM_EWDS_OPCODE, 5);
	shift_out_bits(padapter, reg, 4);

	eeprom_clean(padapter );
exit:	
_func_exit_;	
	return;
}

u16 eeprom_read16(_adapter * padapter, u16 reg) //ReadEEprom
{

	u16 x;
	u16 data=0;
_func_enter_;		

	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
	// select EEPROM, reset bits, set EECS
	x = read8(padapter, CR9346);

	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}

	x &= ~(EEDI | EEDO | EESK | EEM0);
	x |= EEM1 | EECS;
	write8(padapter, CR9346, (unsigned char)x);

	// write the read opcode and register number in that order
	// The opcode is 3bits in length, reg is 6 bits long
	shift_out_bits(padapter, EEPROM_READ_OPCODE, 3);
	shift_out_bits(padapter, reg, padapter->EepromAddressSize);

	// Now read the data (16 bits) in from the selected EEPROM word
	data = shift_in_bits(padapter);

	eeprom_clean(padapter);
out:	
_func_exit_;		
	return data;


}



//From even offset
void eeprom_read_sz(_adapter * padapter, u16 reg, u8* data, u32 sz) 
{

	u16 x, data16;
	u32 i;
_func_enter_;		
	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}
	// select EEPROM, reset bits, set EECS
	x = read8(padapter, CR9346);

	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}

	x &= ~(EEDI | EEDO | EESK | EEM0);
	x |= EEM1 | EECS;
	write8(padapter, CR9346, (unsigned char)x);

	// write the read opcode and register number in that order
	// The opcode is 3bits in length, reg is 6 bits long
	shift_out_bits(padapter, EEPROM_READ_OPCODE, 3);
	shift_out_bits(padapter, reg, padapter->EepromAddressSize);


	for(i=0; i<sz; i+=2)
	{
		data16 = shift_in_bits(padapter);
		data[i] = data16 & 0xff;
		data[i+1] = data16 >>8;		
	}

	eeprom_clean(padapter);
out:	
_func_exit_;		



}


//yt
//addr_off : address offset of the entry in eeprom (not the tuple number of eeprom (reg); that is addr_off !=reg)
u8 eeprom_read(_adapter * padapter, u32 addr_off, u8 sz, u8* rbuf)
{
	u8 quotient, remainder, addr_2align_odd;
	u16 reg, stmp , i=0, idx = 0;
_func_enter_;		
	reg = (u16)(addr_off >> 1);
	addr_2align_odd = (u8)(addr_off & 0x1);

	if(addr_2align_odd) //read that start at high part: e.g  1,3,5,7,9,...
	{
		stmp = eeprom_read16(padapter, reg);
		rbuf[idx++] = (u8) ((stmp>>8)&0xff); //return hogh-part of the short
		reg++; sz--;
	}
	
	quotient = sz >> 1;
	remainder = sz & 0x1;

	for( i=0 ; i < quotient; i++)
	{
		stmp = eeprom_read16(padapter, reg+i);
		rbuf[idx++] = (u8) (stmp&0xff);
		rbuf[idx++] = (u8) ((stmp>>8)&0xff);
	}
	
	reg = reg+i;
	if(remainder){ //end of read at lower part of short : 0,2,4,6,...
		stmp = eeprom_read16(padapter, reg);
		rbuf[idx] = (u8)(stmp & 0xff); 
	}
_func_exit_;		
	return _TRUE;
}

#ifdef burst_read_eeprom


VOID read_eeprom_content(_adapter *	padapter	)
{
//	PHAL_DATA_8711		pHalData=GetHalData8711(adapter);
	u16		eepromid,stmp=0;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;	
	u8 pdata[200];
_func_enter_;		
	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto exit;
	}

#ifdef CONFIG_SDIO_HCI
	eeprom_read_sz(padapter, 0,pdata, 0xb7);
#else
	eepromid = eeprom_read16(padapter, 0);
	if( eepromid != RTL8711_EEPROM_ID )
	{
		if(eepromid ==0xffff){
			DEBUG_ERR(("\nEEPROM ID  = %x is invalid\n", eepromid)); 
			peeprompriv->bautoload_fail_flag = _FALSE;
			peeprompriv->bempty=_TRUE;
		}
		else {   
			//try to access as 93c66/93c56
			padapter->registrypriv.hci=RTL8711_USB;
			padapter->EepromAddressSize=8;
			eepromid = eeprom_read16(padapter, 0);
			if((eepromid == RTL8711_EEPROM_ID )||(eepromid ==0xffff)){
				peeprompriv->bautoload_fail_flag = _FALSE;
				//DbgPrint("eeprom is 93c56 or 93c66");
				if(eepromid ==0xffff){
					peeprompriv->bempty=_TRUE;
				}else{
					peeprompriv->bempty=_FALSE;
				}				
			}
			else{
				peeprompriv->bautoload_fail_flag = _TRUE;
				// since EEPROM content is not correct, no necessary to read anymore.
				goto exit;
			}
		}	
	} else {
		DEBUG_ERR(("\npadapter->EepromAddressSize=%d\n",padapter->EepromAddressSize)); 
		peeprompriv->bautoload_fail_flag = _FALSE;
	}

	eeprom_read_sz(padapter, 0,pdata, 0x2b);
	eeprom_read_sz(padapter,(EEPROM_COUNTRY_STRING_OFFSET>>1),&(pdata[EEPROM_COUNTRY_STRING_OFFSET]), 8);
#endif
	
	//4 Read EEPROM ID to make sure autoload is success
	
	_memcpy(&eepromid, &(pdata[0]), 2);
	if( eepromid != RTL8711_EEPROM_ID )
	{
		DEBUG_ERR(("\nEEPROM ID  = %x is invalid\n", eepromid)); 
		peeprompriv->bautoload_fail_flag = _TRUE;
		// since EEPROM content is not correct, no necessary to read anymore.
		goto exit;

	} else {
		peeprompriv->bautoload_fail_flag = _FALSE;
	}


	//4 Read MAC Address
	_memcpy(&(peeprompriv->mac_addr[0]), &(pdata[EEPROM_MAC_ADDRESS_OFFSET]),6);
	
	DEBUG_INFO(("EEPROM MAC Address = %x-%x-%x-%x-%x-%x\n", 
				peeprompriv->mac_addr[0],	peeprompriv->mac_addr[1],
				peeprompriv->mac_addr[2],	peeprompriv->mac_addr[3],
				peeprompriv->mac_addr[4],	peeprompriv->mac_addr[5]));

	
	//4 Read System config
	peeprompriv->sys_config =pdata[EEPROM_SYSTEMCFG_OFFSET] ;
	
	//4 Read Channel plan
	peeprompriv->channel_plan =pdata[EEPROM_CHANNELPLAN_OFFSET] ;
	DEBUG_INFO( ("EEPROM Channel plan = %d\n", peeprompriv->channel_plan ));



	//4Read 11B Tx Power
	peeprompriv->tx_power_b[1] = pdata[EEPROM_TXPOWERB1_OFFSET];
	peeprompriv->tx_power_b[6] = pdata[EEPROM_TXPOWERB1_OFFSET+1];
	peeprompriv->tx_power_b[11] = pdata[EEPROM_TXPOWERB1_OFFSET+2];
	peeprompriv->tx_power_b[14] = pdata[EEPROM_TXPOWERB1_OFFSET+3];

	//4Read 11G Tx Power
	peeprompriv->tx_power_g[1] = pdata[EEPROM_TXPOWERG1_OFFSET];
	peeprompriv->tx_power_g[6] = pdata[EEPROM_TXPOWERG1_OFFSET+1];
	peeprompriv->tx_power_g[11] = pdata[EEPROM_TXPOWERG1_OFFSET+2];
	peeprompriv->tx_power_g[14] =pdata[EEPROM_TXPOWERG1_OFFSET+3];
	
	//4Read 11A Tx Power 
	peeprompriv->tx_power_a[34] = peeprompriv->tx_power_a[36] = pdata[EEPROM_TXPOWER36_OFFSET];
	peeprompriv->tx_power_a[38] = peeprompriv->tx_power_a[40] = pdata[EEPROM_TXPOWER36_OFFSET+1];
	peeprompriv->tx_power_a[42] = peeprompriv->tx_power_a[44] = pdata[EEPROM_TXPOWER36_OFFSET+2];
	peeprompriv->tx_power_a[46] = peeprompriv->tx_power_a[48] = pdata[EEPROM_TXPOWER36_OFFSET+3];
	peeprompriv->tx_power_a[50] = peeprompriv->tx_power_a[52] = pdata[EEPROM_TXPOWER36_OFFSET+4];
	peeprompriv->tx_power_a[54] = peeprompriv->tx_power_a[56] = pdata[EEPROM_TXPOWER36_OFFSET+5];
	peeprompriv->tx_power_a[58] = peeprompriv->tx_power_a[60] = pdata[EEPROM_TXPOWER36_OFFSET+6];

	peeprompriv->tx_power_a[62] = peeprompriv->tx_power_a[64] = pdata[EEPROM_TXPOWER36_OFFSET+6];
	peeprompriv->tx_power_a[100] = peeprompriv->tx_power_a[7] = pdata[EEPROM_TXPOWER36_OFFSET+8];
	peeprompriv->tx_power_a[104] = peeprompriv->tx_power_a[8] = pdata[EEPROM_TXPOWER36_OFFSET+9];
	peeprompriv->tx_power_a[108] = peeprompriv->tx_power_a[9] = pdata[EEPROM_TXPOWER36_OFFSET+10];	
	peeprompriv->tx_power_a[112] = peeprompriv->tx_power_a[11] = pdata[EEPROM_TXPOWER36_OFFSET+11];
	peeprompriv->tx_power_a[116] = peeprompriv->tx_power_a[12] = pdata[EEPROM_TXPOWER36_OFFSET+12];	
	peeprompriv->tx_power_a[120] = peeprompriv->tx_power_a[16] = pdata[EEPROM_TXPOWER36_OFFSET+13];
	peeprompriv->tx_power_a[124] = peeprompriv->tx_power_a[183] = pdata[EEPROM_TXPOWER36_OFFSET+14];	
	peeprompriv->tx_power_a[128] = peeprompriv->tx_power_a[184] = pdata[EEPROM_TXPOWER36_OFFSET+15];
	peeprompriv->tx_power_a[132] = peeprompriv->tx_power_a[185] = pdata[EEPROM_TXPOWER36_OFFSET+16];
	peeprompriv->tx_power_a[136] = peeprompriv->tx_power_a[187] = pdata[EEPROM_TXPOWER36_OFFSET+17];
	peeprompriv->tx_power_a[140] = peeprompriv->tx_power_a[188] = pdata[EEPROM_TXPOWER36_OFFSET+18];	

	peeprompriv->country_string[0] = pdata[EEPROM_COUNTRY_STRING_OFFSET];
	peeprompriv->country_string[1] = pdata[EEPROM_COUNTRY_STRING_OFFSET+1];
	peeprompriv->country_string[2] = pdata[EEPROM_COUNTRY_STRING_OFFSET+2];

	
	peeprompriv->tx_power_a[149] = peeprompriv->tx_power_a[189] = pdata[EEPROM_TXPOWER149_OFFSET];
	peeprompriv->tx_power_a[153] = peeprompriv->tx_power_a[192] = pdata[EEPROM_TXPOWER149_OFFSET+1];
	peeprompriv->tx_power_a[157] = peeprompriv->tx_power_a[196] = pdata[EEPROM_TXPOWER149_OFFSET+2];
	peeprompriv->tx_power_a[161] = pdata[EEPROM_TXPOWER149_OFFSET+3];
	peeprompriv->tx_power_a[165] = pdata[EEPROM_TXPOWER149_OFFSET+4];

#ifdef CONFIG_SDIO_HCI

	// read sdio_setting
	peeprompriv->sdio_setting=pdata[EEPROM_SDIO_SETTING_OFFSET];
	
	// read sdio_ocr
	_memcpy(&peeprompriv->ocr, &(pdata[EEPROM_ORC_OFFSET]), 3);

	// read sdio_cis0 & sdio_cis1
	_memcpy(peeprompriv->cis0, &(pdata[EEPROM_CIS0_OFFSET]), eeprom_cis0_sz);
	
	_memcpy(peeprompriv->cis1, &(pdata[EEPROM_CIS1_OFFSET]), eeprom_cis1_sz);


#endif


	
	
exit:
_func_exit_;		
}

#else
VOID read_eeprom_content(_adapter *	padapter	)
{
//	PHAL_DATA_8711		pHalData=GetHalData8711(adapter);
	u16		eepromid,stmp=0;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;	
_func_enter_;		
	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto exit;
	}
	
	//4 Read EEPROM ID to make sure autoload is success
	eepromid = eeprom_read16(padapter, 0);
	if( eepromid != RTL8711_EEPROM_ID )
	{
		if(eepromid ==0xffff){
			DEBUG_ERR(("\nEEPROM ID  = %x is invalid\n", eepromid)); 
			peeprompriv->bautoload_fail_flag = _FALSE;
			peeprompriv->bempty=_TRUE;
		}
		else {   
			#ifdef CONFIG_USB_HCI
			//try to access as 93c66/93c56
			padapter->registrypriv.hci=RTL8711_USB;
			padapter->EepromAddressSize=8;
			eepromid = eeprom_read16(padapter, 0);
			if((eepromid == RTL8711_EEPROM_ID )||(eepromid ==0xffff)){
				peeprompriv->bautoload_fail_flag = _FALSE;
			//	DbgPrint("eeprom is 93c56 or 93c66");
				if(eepromid ==0xffff){
					peeprompriv->bempty=_TRUE;
				}else{
					peeprompriv->bempty=_FALSE;
				}				
			}
			else{
				peeprompriv->bautoload_fail_flag = _TRUE;
				// since EEPROM content is not correct, no necessary to read anymore.
				goto exit;

			}
			#else
			DEBUG_ERR(("\nEEPROM ID  = %x is invalid peeprompriv->bempty=_FALSE\n", eepromid)); 
			peeprompriv->bautoload_fail_flag = _TRUE;
			// since EEPROM content is not correct, no necessary to read anymore.
			goto exit;
			#endif
		}
	
	} else {
		DEBUG_ERR(("\npadapter->EepromAddressSize=%d\n",padapter->EepromAddressSize)); 
		peeprompriv->bautoload_fail_flag = _FALSE;
	}


	//4 Read MAC Address
	stmp = eeprom_read16(padapter, EEPROM_MAC_ADDRESS_OFFSET>>1);
	peeprompriv->mac_addr[0] = stmp>>8;
	
	stmp = eeprom_read16(padapter, (EEPROM_MAC_ADDRESS_OFFSET+1)>>1);
	peeprompriv->mac_addr[1] = stmp & 0xff;
	peeprompriv->mac_addr[2] = stmp>>8;
	
	stmp = eeprom_read16(padapter, (EEPROM_MAC_ADDRESS_OFFSET+3)>>1);
	peeprompriv->mac_addr[3] = stmp & 0xff;
	peeprompriv->mac_addr[4] = stmp>>8;

	stmp = eeprom_read16(padapter, (EEPROM_MAC_ADDRESS_OFFSET+5)>>1);
	peeprompriv->mac_addr[5] = stmp & 0xff;

	
	DEBUG_INFO(("EEPROM MAC Address = %x-%x-%x-%x-%x-%x\n", 
				peeprompriv->mac_addr[0],	peeprompriv->mac_addr[1],
				peeprompriv->mac_addr[2],	peeprompriv->mac_addr[3],
				peeprompriv->mac_addr[4],	peeprompriv->mac_addr[5]));

	
	//4 Read Channel plan
	peeprompriv->sys_config = (eeprom_read16(padapter, EEPROM_SYSTEMCFG_OFFSET>>1)) & 0xff;	

	//4 Read Channel plan
	peeprompriv->channel_plan =(eeprom_read16(padapter, EEPROM_CHANNELPLAN_OFFSET>>1)) & 0xff;	
	DEBUG_INFO( ("EEPROM Channel plan = %d\n", peeprompriv->channel_plan ));

	//4Read 11B Tx Power
	stmp = eeprom_read16(padapter, EEPROM_TXPOWERB1_OFFSET>>1);
	peeprompriv->tx_power_b[1] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWERB1_OFFSET+1)>>1);
	peeprompriv->tx_power_b[6] = stmp & 0xff;
	peeprompriv->tx_power_b[11] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWERB1_OFFSET+3)>>1);
	peeprompriv->tx_power_b[14] = stmp & 0xff;

	//4Read 11G Tx Power
	stmp = eeprom_read16(padapter, EEPROM_TXPOWERG1_OFFSET>>1);
	peeprompriv->tx_power_g[1] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWERG1_OFFSET+1)>>1);
	peeprompriv->tx_power_g[6] = stmp & 0xff;
	peeprompriv->tx_power_g[11] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWERG1_OFFSET+3)>>1);
	peeprompriv->tx_power_g[14] = stmp & 0xff;

	//4Read 11A Tx Power //
	stmp = eeprom_read16(padapter, EEPROM_TXPOWER36_OFFSET>>1);
	peeprompriv->tx_power_a[34] = peeprompriv->tx_power_a[36] = stmp>>8;
	stmp = eeprom_read16(padapter, (+1)>>1);
	peeprompriv->tx_power_a[38] = peeprompriv->tx_power_a[40] = stmp & 0xff;
	peeprompriv->tx_power_a[42] = peeprompriv->tx_power_a[44] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+3)>>1);
	peeprompriv->tx_power_a[46] = peeprompriv->tx_power_a[48] = stmp & 0xff;
	peeprompriv->tx_power_a[50] = peeprompriv->tx_power_a[52] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+5)>>1);
	peeprompriv->tx_power_a[54] = peeprompriv->tx_power_a[56] = stmp & 0xff;
	peeprompriv->tx_power_a[58] = peeprompriv->tx_power_a[60] = stmp>>8;

	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+7)>>1);
	peeprompriv->tx_power_a[62] = peeprompriv->tx_power_a[64] = stmp & 0xff;
	peeprompriv->tx_power_a[100] = peeprompriv->tx_power_a[7] = stmp>>8;	
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+9)>>1);
	peeprompriv->tx_power_a[104] = peeprompriv->tx_power_a[8] = stmp & 0xff;
	peeprompriv->tx_power_a[108] = peeprompriv->tx_power_a[9] = stmp>>8;		
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+11)>>1);
	peeprompriv->tx_power_a[112] = peeprompriv->tx_power_a[11] = stmp & 0xff;
	peeprompriv->tx_power_a[116] = peeprompriv->tx_power_a[12] = stmp>>8;		
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+13)>>1);
	peeprompriv->tx_power_a[120] = peeprompriv->tx_power_a[16] = stmp & 0xff;
	peeprompriv->tx_power_a[124] = peeprompriv->tx_power_a[183] = stmp>>8;		
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+15)>>1);
	peeprompriv->tx_power_a[128] = peeprompriv->tx_power_a[184] = stmp & 0xff;
	peeprompriv->tx_power_a[132] = peeprompriv->tx_power_a[185] = stmp>>8;		
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+17)>>1);
	peeprompriv->tx_power_a[136] = peeprompriv->tx_power_a[187] = stmp & 0xff;
	peeprompriv->tx_power_a[140] = peeprompriv->tx_power_a[188] = stmp>>8;		

	stmp = eeprom_read16(padapter, EEPROM_COUNTRY_STRING_OFFSET>>1);
	peeprompriv->country_string[0] = stmp & 0xff;
	peeprompriv->country_string[1] = stmp>>8;

	stmp = eeprom_read16(padapter, EEPROM_TXPOWER149_OFFSET>>1);
	peeprompriv->country_string[2] = stmp & 0xff;
	peeprompriv->tx_power_a[149] = peeprompriv->tx_power_a[189] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER149_OFFSET+1)>>1);
	peeprompriv->tx_power_a[153] = peeprompriv->tx_power_a[192] = stmp & 0xff;
	peeprompriv->tx_power_a[157] = peeprompriv->tx_power_a[196] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER149_OFFSET+3)>>1);
	peeprompriv->tx_power_a[161] = stmp & 0xff;
	peeprompriv->tx_power_a[165] = stmp>>8;

#ifdef CONFIG_SDIO_HCI

	// read sdio_setting
 	eeprom_read( padapter, EEPROM_SDIO_SETTING_OFFSET, 1, &peeprompriv->sdio_setting);
	stmp =eeprom_read16(padapter, (EEPROM_SDIO_SETTING_OFFSET)>>1);
	
	// read sdio_ocr
 	eeprom_read( padapter, EEPROM_ORC_OFFSET, 3, (unsigned char*)(&peeprompriv->ocr));

	// read sdio_cis0 & sdio_cis1
 	eeprom_read( padapter, EEPROM_CIS0_OFFSET, eeprom_cis0_sz, peeprompriv->cis0);

 	eeprom_read( padapter, EEPROM_CIS1_OFFSET, eeprom_cis1_sz, peeprompriv->cis1);


#endif


	
	
exit:
_func_exit_;		
}

#endif


VOID read_eeprom_content_by_attrib(_adapter *	padapter	)
{
#ifdef CONFIG_SDIO_HCI
	u16		eepromid,stmp=0;
	u8 tmp8,*ptr;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;	
_func_enter_;		

	//4 Read EEPROM ID to make sure autoload is success
	 attrib_read(padapter, 0x1134, 2, (u8 *)&eepromid );
	ptr=&peeprompriv->bautoload_fail_flag;

	//eepromid =eeprom_read16(padapter, 0);
	if( eepromid != RTL8711_EEPROM_ID )
	{
		DEBUG_ERR(("\nEEPROM ID  = %x is invalid\n", eepromid)); 
		peeprompriv->bautoload_fail_flag = _TRUE;
		// since EEPROM content is not correct, no necessary to read anymore.
		goto exit;

	} else {
		peeprompriv->bautoload_fail_flag = _FALSE;
	}


	//4 Read MAC Address
	attrib_read(padapter, 0x1137,6,&(peeprompriv->mac_addr[0]));
	
	DEBUG_ERR(("EEPROM MAC Address = %x-%x-%x-%x-%x-%x\n", 
				peeprompriv->mac_addr[0],	peeprompriv->mac_addr[1],
				peeprompriv->mac_addr[2],	peeprompriv->mac_addr[3],
				peeprompriv->mac_addr[4],	peeprompriv->mac_addr[5]));

	
	//4 Read Channel plan
	attrib_read(padapter, 0x1136,1,&(peeprompriv->sys_config));

	//peeprompriv->sys_config = (eeprom_read16(padapter, EEPROM_SYSTEMCFG_OFFSET>>1)) & 0xff;	

	//4 Read Channel plan
	attrib_read(padapter, 0x1143, 1,(u8 *)&(peeprompriv->channel_plan));
	//peeprompriv->channel_plan =(eeprom_read16(padapter, EEPROM_CHANNELPLAN_OFFSET>>1)) & 0xff;	
	DEBUG_INFO( ("EEPROM Channel plan = %d\n", peeprompriv->channel_plan ));

	//4Read 11B Tx Power
#if 1
	attrib_read(padapter, 0x1145,1,&(peeprompriv->tx_power_b[1]));
	attrib_read(padapter, 0x1146,1,&(peeprompriv->tx_power_b[6]));
	attrib_read(padapter, 0x1147,1,&(peeprompriv->tx_power_b[11]));
	attrib_read(padapter, 0x1148,1,&(peeprompriv->tx_power_b[14]));

#else
	stmp = eeprom_read16(padapter, EEPROM_TXPOWERB1_OFFSET>>1);
	peeprompriv->tx_power_b[1] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWERB1_OFFSET+1)>>1);
	peeprompriv->tx_power_b[6] = stmp & 0xff;
	peeprompriv->tx_power_b[11] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWERB1_OFFSET+3)>>1);
	peeprompriv->tx_power_b[14] = stmp & 0xff;
#endif
	//4Read 11G Tx Power

#if 1
	attrib_read(padapter, 0x1149,1,&(peeprompriv->tx_power_g[1] ));
	attrib_read(padapter, 0x114a,1,&(peeprompriv->tx_power_g[6] ));
	attrib_read(padapter, 0x114b,1,&(peeprompriv->tx_power_g[11] ));
	attrib_read(padapter, 0x114c,1,&(peeprompriv->tx_power_g[14] ));

#else
	stmp = eeprom_read16(padapter, EEPROM_TXPOWERG1_OFFSET>>1);
	peeprompriv->tx_power_g[1] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWERG1_OFFSET+1)>>1);
	peeprompriv->tx_power_g[6] = stmp & 0xff;
	peeprompriv->tx_power_g[11] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWERG1_OFFSET+3)>>1);
	peeprompriv->tx_power_g[14] = stmp & 0xff;
#endif
	//4Read 11A Tx Power //

#if 1
	attrib_read(padapter, 0x114d,1,&(peeprompriv->tx_power_a[34] ));
	peeprompriv->tx_power_a[36] = peeprompriv->tx_power_a[34] ;
	attrib_read(padapter, 0x114e,1,&(peeprompriv->tx_power_a[38] ));
	peeprompriv->tx_power_a[40] = peeprompriv->tx_power_a[38] ;
	attrib_read(padapter, 0x114f,1,&(peeprompriv->tx_power_a[42] ));
	peeprompriv->tx_power_a[44] = peeprompriv->tx_power_a[42]; 
	attrib_read(padapter, 0x1150,1,&(peeprompriv->tx_power_a[46] ));
	peeprompriv->tx_power_a[48] = peeprompriv->tx_power_a[46]; 
	attrib_read(padapter, 0x1151,1,&(peeprompriv->tx_power_a[50] ));
	peeprompriv->tx_power_a[52] = peeprompriv->tx_power_a[50];
	attrib_read(padapter, 0x1152,1,&(peeprompriv->tx_power_a[54] ));
	peeprompriv->tx_power_a[56] = peeprompriv->tx_power_a[54]; 
	attrib_read(padapter, 0x1153,1,&(peeprompriv->tx_power_a[58] ));
	peeprompriv->tx_power_a[60] = peeprompriv->tx_power_a[58]; 
	attrib_read(padapter, 0x1154,1,&(peeprompriv->tx_power_a[62] ));
	peeprompriv->tx_power_a[64] = peeprompriv->tx_power_a[62]; 
	attrib_read(padapter, 0x1155,1,&(peeprompriv->tx_power_a[100] ));
	peeprompriv->tx_power_a[7] = peeprompriv->tx_power_a[100]; 
	attrib_read(padapter, 0x1156,1,&(peeprompriv->tx_power_a[104] ));
	peeprompriv->tx_power_a[8] = peeprompriv->tx_power_a[104]; 
	attrib_read(padapter, 0x1157,1,&(peeprompriv->tx_power_a[108] ));
	peeprompriv->tx_power_a[9] = peeprompriv->tx_power_a[108]; 
	attrib_read(padapter, 0x1158,1,&(peeprompriv->tx_power_a[112] ));
	peeprompriv->tx_power_a[11] = peeprompriv->tx_power_a[112]; 
	attrib_read(padapter, 0x1159,1,&(peeprompriv->tx_power_a[116] ));
	peeprompriv->tx_power_a[12] = peeprompriv->tx_power_a[116]; 
	attrib_read(padapter, 0x115a,1,&(peeprompriv->tx_power_a[120] ));
	peeprompriv->tx_power_a[16] = peeprompriv->tx_power_a[120]; 
	attrib_read(padapter, 0x115b,1,&(peeprompriv->tx_power_a[124] ));
	peeprompriv->tx_power_a[183] = peeprompriv->tx_power_a[124]; 
	attrib_read(padapter, 0x115c,1,&(peeprompriv->tx_power_a[128] ));
	peeprompriv->tx_power_a[184] = peeprompriv->tx_power_a[128]; 
	attrib_read(padapter, 0x115d,1,&(peeprompriv->tx_power_a[132] ));
	peeprompriv->tx_power_a[185] = peeprompriv->tx_power_a[132]; 
	attrib_read(padapter, 0x115e,1,&(peeprompriv->tx_power_a[136] ));
	peeprompriv->tx_power_a[187] = peeprompriv->tx_power_a[136]; 
	attrib_read(padapter, 0x115f,1,&(peeprompriv->tx_power_a[140] ));
	peeprompriv->tx_power_a[188] = peeprompriv->tx_power_a[140]; 

#else

	stmp = eeprom_read16(padapter, EEPROM_TXPOWER36_OFFSET>>1);
	peeprompriv->tx_power_a[34] = peeprompriv->tx_power_a[36] = stmp>>8;
	stmp = eeprom_read16(padapter, (+1)>>1);
	peeprompriv->tx_power_a[38] = peeprompriv->tx_power_a[40] = stmp & 0xff;
	peeprompriv->tx_power_a[42] = peeprompriv->tx_power_a[44] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+3)>>1);
	peeprompriv->tx_power_a[46] = peeprompriv->tx_power_a[48] = stmp & 0xff;
	peeprompriv->tx_power_a[50] = peeprompriv->tx_power_a[52] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+5)>>1);
	peeprompriv->tx_power_a[54] = peeprompriv->tx_power_a[56] = stmp & 0xff;
	peeprompriv->tx_power_a[58] = peeprompriv->tx_power_a[60] = stmp>>8;

	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+7)>>1);
	peeprompriv->tx_power_a[62] = peeprompriv->tx_power_a[64] = stmp & 0xff;
	peeprompriv->tx_power_a[100] = peeprompriv->tx_power_a[7] = stmp>>8;	
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+9)>>1);
	peeprompriv->tx_power_a[104] = peeprompriv->tx_power_a[8] = stmp & 0xff;
	peeprompriv->tx_power_a[108] = peeprompriv->tx_power_a[9] = stmp>>8;		
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+11)>>1);
	peeprompriv->tx_power_a[112] = peeprompriv->tx_power_a[11] = stmp & 0xff;
	peeprompriv->tx_power_a[116] = peeprompriv->tx_power_a[12] = stmp>>8;		
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+13)>>1);
	peeprompriv->tx_power_a[120] = peeprompriv->tx_power_a[16] = stmp & 0xff;
	peeprompriv->tx_power_a[124] = peeprompriv->tx_power_a[183] = stmp>>8;		
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+15)>>1);
	peeprompriv->tx_power_a[128] = peeprompriv->tx_power_a[184] = stmp & 0xff;
	peeprompriv->tx_power_a[132] = peeprompriv->tx_power_a[185] = stmp>>8;		
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER36_OFFSET+17)>>1);
	peeprompriv->tx_power_a[136] = peeprompriv->tx_power_a[187] = stmp & 0xff;
	peeprompriv->tx_power_a[140] = peeprompriv->tx_power_a[188] = stmp>>8;		
#endif

#if 1
	attrib_read(padapter, 0x1160,3,&(peeprompriv->country_string[0] ));
#else
	stmp = eeprom_read16(padapter, EEPROM_COUNTRY_STRING_OFFSET>>1);
	peeprompriv->country_string[0] = stmp & 0xff;
	peeprompriv->country_string[1] = stmp>>8;

	stmp = eeprom_read16(padapter, EEPROM_TXPOWER149_OFFSET>>1);
	peeprompriv->country_string[2] = stmp & 0xff;
#endif

#if 1
	attrib_read(padapter, 0x1163,1,&(peeprompriv->tx_power_a[149] ));
	peeprompriv->tx_power_a[189] = peeprompriv->tx_power_a[149]; 
	attrib_read(padapter, 0x1164,1,&(peeprompriv->tx_power_a[153] ));
	peeprompriv->tx_power_a[192] = peeprompriv->tx_power_a[153]; 
	attrib_read(padapter, 0x1165,1,&(peeprompriv->tx_power_a[157] ));
	peeprompriv->tx_power_a[196] = peeprompriv->tx_power_a[157]; 
	attrib_read(padapter, 0x1166,1,&(peeprompriv->tx_power_a[161] ));
	attrib_read(padapter, 0x1167,1,&(peeprompriv->tx_power_a[165] ));
	
#else
	peeprompriv->tx_power_a[149] = peeprompriv->tx_power_a[189] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER149_OFFSET+1)>>1);
	peeprompriv->tx_power_a[153] = peeprompriv->tx_power_a[192] = stmp & 0xff;
	peeprompriv->tx_power_a[157] = peeprompriv->tx_power_a[196] = stmp>>8;
	stmp = eeprom_read16(padapter, (EEPROM_TXPOWER149_OFFSET+3)>>1);
	peeprompriv->tx_power_a[161] = stmp & 0xff;
	peeprompriv->tx_power_a[165] = stmp>>8;
#endif

#ifdef CONFIG_SDIO_HCI

	// read sdio_setting
 	eeprom_read( padapter, EEPROM_SDIO_SETTING_OFFSET, 1, &peeprompriv->sdio_setting);
	stmp =eeprom_read16(padapter, (EEPROM_SDIO_SETTING_OFFSET)>>1);
	
	// read sdio_ocr
 	eeprom_read( padapter, EEPROM_ORC_OFFSET, 3, (unsigned char*)(&peeprompriv->ocr));

	// read sdio_cis0 & sdio_cis1
 	eeprom_read( padapter, EEPROM_CIS0_OFFSET, eeprom_cis0_sz, peeprompriv->cis0);

 	eeprom_read( padapter, EEPROM_CIS1_OFFSET, eeprom_cis1_sz, peeprompriv->cis1);


#endif




	
	
exit:
_func_exit_;		
#endif
}
#endif
