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
#include <rtl8187_spec.h>

#define RTL8187_REQT_READ 0xc0
u16 eeprom_read16(_adapter * padapter, u16 reg) //ReadEEprom
{

	int status;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	struct usb_device *udev=pdev->pusbdev;
	u16 data=0;
_func_enter_;		

	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto out;
	}	
	status = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			       0x05, RTL8187_REQT_READ,
			       reg, 0x9346, &data, 2, HZ);
	
        if (status < 0)
        {
                DEBUG_INFO(("write_nic_byte TimeOut! status:%x\n", status));
        }

	
out:	
_func_exit_;		
	return data;


}


VOID read_eeprom_content(_adapter *	padapter	)
{
	u16		val16,stmp=0;
	int i;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;	
_func_enter_;		
	DEBUG_ERR(("+read_eeprom_content"));

	if(padapter->bSurpriseRemoved==_TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved==_TRUE"));
		goto exit;
	}

	peeprompriv->eepromid = eeprom_read16(padapter, 0);


	DEBUG_ERR(("EEPROM ID %4x",peeprompriv->eepromid));
	for(i=0 ; i<6; i+=2){
		u16 Tmp =eeprom_read16(padapter, ( (EEPROM_MAC_ADDRESS+i) >> 1));// Read_EEprom87B(dev, ( (EEPROM_MAC_ADDRESS+i) >> 1));
		peeprompriv->mac_addr[i]= Tmp &0xff;
		peeprompriv->mac_addr[i+1]=  (Tmp &0xff00)>>8;
	}
	DEBUG_ERR(("\nCard MAC address is \n"MAC_FMT,MAC_ARG(peeprompriv->mac_addr)));

	
       peeprompriv->vid= eeprom_read16(padapter, EEPROM_VID>>1);
	peeprompriv->did=eeprom_read16(padapter, EEPROM_PID>>1);
	DEBUG_ERR(("\nEEPROM VID:0x%.4x ,EEPROM DID:0x%.4x\n",peeprompriv->vid,peeprompriv->did));

        if(peeprompriv->did== 0x8981)
                peeprompriv->eepromid = 0x8129;
		
	peeprompriv->channel_plan=(eeprom_read16(padapter, EEPROM_CHANNEL_PLAN>>1))&0xff;
	
	stmp = (eeprom_read16(padapter, EEPROM_INTERFACE_SELECT>>1))>>14;
	DEBUG_ERR(("Interface Selection is 0x%x",stmp));


	// Read Tx power base offset.
	val16=(eeprom_read16(padapter, EEPROM_TX_POWER_BASE_OFFSET>>1))&0xff;// (Read_EEprom87B(dev, EEPROM_TX_POWER_BASE_OFFSET>>1))&0xff;
	//DMESG("EEPROM_TX_POWER_BASE_OFFSET:0x%0.2x",word);
	peeprompriv->cck_txpwr_base = val16 & 0xf;
	peeprompriv->ofdm_txpwr_base = (val16>>4) & 0xf;
	DEBUG_ERR(("peeprompriv->cck_txpwr_base=0x%x  peeprompriv->ofdm_txpwr_base=0x%x\n",peeprompriv->cck_txpwr_base,peeprompriv->ofdm_txpwr_base));
	
	// Read Tx power index of each channel from EEPROM.
	// ch1- ch2.
	val16=eeprom_read16(padapter,(EEPROM_TX_POWER_LEVEL_1_2 >> 1));
	peeprompriv->tx_power_b[1]=val16&0xf;
	peeprompriv->tx_power_g[1]=(val16&0xf0)>>4;
	peeprompriv->tx_power_b[2]=(val16&0xf00)>>8;
	peeprompriv->tx_power_g[2]=(val16&0xf000)>>12;
	DEBUG_ERR(("val16=0x%x peeprompriv->tx_power_b[1]=0x%x  peeprompriv->tx_power_g[1]=0x%x peeprompriv->tx_power_b[2]=0x%x peeprompriv->tx_power_g[2]=0x%x\n",
		val16,peeprompriv->tx_power_b[1],peeprompriv->tx_power_g[1],peeprompriv->tx_power_b[2],peeprompriv->tx_power_g[2]));


	// ch3- ch4.
	val16=eeprom_read16(padapter,(EEPROM_TX_POWER_LEVEL_3_4 >> 1));
	peeprompriv->tx_power_b[3]=val16&0xf;
	peeprompriv->tx_power_g[3]=(val16&0xf0)>>4;
	peeprompriv->tx_power_b[4]=(val16&0xf00)>>8;
	peeprompriv->tx_power_g[4]=(val16&0xf000)>>12;
	DEBUG_ERR(("val16=0x%x peeprompriv->tx_power_b[3]=0x%x  peeprompriv->tx_power_g[3]=0x%x peeprompriv->tx_power_b[4]=0x%x peeprompriv->tx_power_g[4]=0x%x\n",
		val16,peeprompriv->tx_power_b[3],peeprompriv->tx_power_g[3],peeprompriv->tx_power_b[4],peeprompriv->tx_power_g[4]));

	
	// ch5- ch6.
	val16=eeprom_read16(padapter,(EEPROM_TX_POWER_LEVEL_5_6 >> 1));
	peeprompriv->tx_power_b[5]=val16&0xf;
	peeprompriv->tx_power_g[5]=(val16&0xf0)>>4;
	peeprompriv->tx_power_b[6]=(val16&0xf00)>>8;
	peeprompriv->tx_power_g[6]=(val16&0xf000)>>12;
	DEBUG_ERR(("val16=0x%x peeprompriv->tx_power_b[5]=0x%x  peeprompriv->tx_power_g[5]=0x%x peeprompriv->tx_power_b[6]=0x%x peeprompriv->tx_power_g[6]=0x%x\n",
		val16,peeprompriv->tx_power_b[5],peeprompriv->tx_power_g[5],peeprompriv->tx_power_b[6],peeprompriv->tx_power_g[6]));


	// ch7- ch10.
	for(i=7 ; i<10 ; i+=2){
		val16=eeprom_read16(padapter, (EEPROM_TX_POWER_LEVEL_7_10 + i - 7) >> 1);
		peeprompriv->tx_power_b[i]=val16&0xf;
		peeprompriv->tx_power_g[i]=(val16&0xf0)>>4;
		peeprompriv->tx_power_b[i+1]=(val16&0xf00)>>8;
		peeprompriv->tx_power_g[i+1]=(val16&0xf000)>>12;
		DEBUG_ERR(("val16=0x%x peeprompriv->tx_power_b[%d]=0x%x  peeprompriv->tx_power_g[%d]=0x%x peeprompriv->tx_power_b[%d]=0x%x peeprompriv->tx_power_g[%d]=0x%x\n",
		val16,i,peeprompriv->tx_power_b[i],i,peeprompriv->tx_power_g[i],i+1,peeprompriv->tx_power_b[i+1],i+1,peeprompriv->tx_power_g[i+1]));
	}

	// ch11.
	val16=eeprom_read16(padapter, EEPROM_TX_POWER_LEVEL_11>> 1);
	peeprompriv->tx_power_b[11]=val16&0xf;
	peeprompriv->tx_power_g[11]=(val16&0xf0)>>4;
	DEBUG_ERR(("val16=0x%x peeprompriv->tx_power_b[11]=0x%x  peeprompriv->tx_power_g[11]=0x%x \n",
	val16,peeprompriv->tx_power_b[11],peeprompriv->tx_power_g[11]));

	// ch12.
	val16=eeprom_read16(padapter, EEPROM_TX_POWER_LEVEL_12>> 1);
	peeprompriv->tx_power_b[12]=val16&0xf;
	peeprompriv->tx_power_g[12]=(val16&0xf0)>>4;
	DEBUG_ERR(("val16=0x%x peeprompriv->tx_power_b[12]=0x%x  peeprompriv->tx_power_g[12]=0x%x \n",
	val16,peeprompriv->tx_power_b[12],peeprompriv->tx_power_g[12]));
	

	// ch13- ch14.
	val16=eeprom_read16(padapter,(EEPROM_TX_POWER_LEVEL_13_14 >> 1));
	peeprompriv->tx_power_b[13]=val16&0xf;
	peeprompriv->tx_power_g[13]=(val16&0xf0)>>4;
	peeprompriv->tx_power_b[14]=(val16&0xf00)>>8;
	peeprompriv->tx_power_g[14]=(val16&0xf000)>>12;
	DEBUG_ERR(("val16=0x%x peeprompriv->tx_power_b[13]=0x%x  peeprompriv->tx_power_g[13]=0x%x peeprompriv->tx_power_b[14]=0x%x peeprompriv->tx_power_g[14]=0x%x\n",
		val16,peeprompriv->tx_power_b[13],peeprompriv->tx_power_g[13],peeprompriv->tx_power_b[14],peeprompriv->tx_power_g[14]));

	val16=eeprom_read16(padapter,(EEPROM_SW_REVD_OFFSET >> 1));

	// Low Noise Amplifier. added by Roger, 2007.04.06.
	if( (val16 & EEPROM_LNA_MASK) == EEPROM_LNA_ENABLE )
	{
		peeprompriv->blow_noise_amplifier = _TRUE;
	}
	else
	{
		peeprompriv->blow_noise_amplifier = _FALSE;
	}
	
	DEBUG_ERR(("val16=0x%x peeprompriv->blow_noise_amplifier ]=0x%x \n",val16,peeprompriv->blow_noise_amplifier ));
	
 peeprompriv->country_string[0]=0x55;
 peeprompriv->country_string[1]=0x53;
 peeprompriv->country_string[2]=0x20;
	
exit:
	DEBUG_ERR(("-read_eeprom_content\n"));

_func_exit_;		
}



