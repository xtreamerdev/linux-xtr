/******************************************************************************
* usb_halinit.c                                                                                                                                 *
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
#define _HCI_HAL_INIT_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <hal_init.h>


#include <usb_hal.h>
#include <usb_osintf.h>
#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_USB_HCI

#error "CONFIG_USB_HCI shall be on!\n"

#endif


u8 usb_hal_bus_init(_adapter * padapter){
	u32			val32;
	u8			tmp8,val8=_SUCCESS;

//	u16 			i = 0;
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
//	NDIS_STATUS ndisstatus=NDIS_STATUS_SUCCESS;
	
	_func_enter_;

	//************************************************************************************************************
	// Initialization for USB
	//************************************************************************************************************


	//SET_SYSCLK to 40M
	usbvendorrequest(&padapter->dvobjpriv,  RT_USB_GET_SYSCLK, 0, 0, &tmp8, 1, _TRUE);
	DEBUG_ERR(("usb_hal_bus_init() : System Clock = %d\n", tmp8));

	if (tmp8 > 0){
		DEBUG_ERR(("usb_hal_bus_init() : System Clock = is High Speed, no necessary to set again.tmp8=0x%x\n", tmp8));
	} else {
		
		DEBUG_ERR(("usb_hal_bus_init() : Set System Clock to 40MHz.\n"));
		usbvendorrequest(&padapter->dvobjpriv, RT_USB_SET_SYSCLK, RT_USB_SYSCLK_40MHZ, 0,  NULL, 0, _FALSE);
		// check the value
		msleep_os(100);
		usbvendorrequest(&padapter->dvobjpriv,  RT_USB_GET_SYSCLK, 0, 0, &tmp8, 1, _TRUE);
		DEBUG_ERR(("usb_hal_bus_init() : System Clock = %d\n", tmp8));
		if ( tmp8 != RT_USB_SYSCLK_40MHZ) {
			DEBUG_ERR(("usb_hal_bus_init(): System Clock is not matched, GET_SYSCLK = %d",tmp8));
			val8 = _FAIL;
			goto exit;
		}		
	}
	
	//4	Turn on LDO
	usbvendorrequest(&padapter->dvobjpriv, RT_USB_GET_REGISTER, RT_USB_LDO, 0, &tmp8, 1, _TRUE);
	DEBUG_ERR(("usb_hal_bus_init() : LDO = %d\n", tmp8));
	if (tmp8 == 0x01){
		DEBUG_ERR(("usb_hal_bus_init() : LDO is turned on. No necessary to Turn it on.\n"));
	} else {
		DEBUG_ERR(("usb_hal_bus_init() : Turn On LDO\n"));
		usbvendorrequest(&padapter->dvobjpriv, RT_USB_SET_REGISTER, RT_USB_LDO, RT_USB_LDO_ON,  NULL, 0, _FALSE);
		msleep_os(100);
		usbvendorrequest(&padapter->dvobjpriv, RT_USB_GET_REGISTER, RT_USB_LDO, 0, &tmp8, 1, _TRUE);
		if(tmp8 != 0x01){
			DEBUG_ERR(("usb_hal_bus_init() : LDO can't turn on.\n"));
			DEBUG_ERR(("usb_hal_bus_init() : LDO = %d\n", tmp8));
			val8 = _FAIL;
			goto exit;
		}
		DEBUG_ERR(("usb_hal_bus_init() : LDO = %d\n", tmp8));
	}

	DEBUG_ERR(("before read SYSCLKR!\n"));
	DEBUG_ERR(("bSupriseRemoved=%x! bDriverstopped=%x!\n",padapter->bSurpriseRemoved,padapter->bDriverStopped));
	val32 = read32(padapter, SYSCLKR);
	DEBUG_ERR(("^^^^^^^^^SYSCLKR : %.8x !\n", val32));
	if(padapter->bDriverStopped){
		DEBUG_ERR(("padapter->bDriverStopped=%d !\n", padapter->bDriverStopped));
		val8=_FAIL;
	}
	
	if  (pregistrypriv->chip_version == RTL8711_FPGA)
		write32(padapter, SYSCLKR, (val32 | _LXBUS0S | _HWMASK | _HSSEL));
	else
        	write32(padapter, SYSCLKR, (val32 | _HWMASK | _HSSEL));

	if (pregistrypriv->chip_version == RTL8711_FPGA)
	{		
		pregistrypriv->chip_version = RTL8711_FPGA;
	}
	else
	{        	
		//determine the chip version 
		if(val32&BIT(31))
		{
			pregistrypriv->chip_version = RTL8711_3rdCUT;
			DEBUG_ERR(("usb_hal_bus_init:RTL8711_3rdCUT\n"));
		}
		else
		{
			//try again
			val32 = read32(padapter, SYSCLKR);
			if(val32&BIT(31))
			{
				pregistrypriv->chip_version = RTL8711_3rdCUT;
				DEBUG_ERR(("usb_hal_bus_init(2nd time):RTL8711_3rdCUT\n"));
			}
			else{
				val32 = read32(padapter, AFEBYPS_CFG);
				if(val32&BIT(0))
				{
					pregistrypriv->chip_version = RTL8711_2ndCUT;
					DEBUG_ERR(("usb_hal_bus_init:RTL8711_2ndCUT\n"));
				}
				else
				{
					pregistrypriv->chip_version = RTL8711_1stCUT;
					DEBUG_ERR(("usb_hal_bus_init:RTL8711_1stCUT\n"));
				}			
			}
		}		
	}
	val32 = read32(padapter, SYSCLKR);
	DEBUG_ERR(("^^^^^^^^^SYSCLKR : %.8x !\n", val32));


	DEBUG_ERR(("Turn  _SCRST | _UARTRST | BIT(3) !"));

	val32 = read32(padapter, SYSCTRL);

	write32(padapter, SYSCTRL, (val32 |_SCRST | _UARTRST | BIT(3)));
	msleep_os(200); //delay 


	val32= read32(padapter, SYSCTRL);

	DEBUG_INFO(("^^^^^^^^^SYSCTRL : %x !\n", val32));
	


	
exit:
	_func_exit_;
	return val8;

}
 u8 usb_hal_bus_deinit(_adapter * padapter){

	u8			tmp8;
	_func_enter_;
	
	//4	Turn off LDO
	usbvendorrequest(&padapter->dvobjpriv, RT_USB_GET_REGISTER, RT_USB_LDO, 0, &tmp8, 1, _TRUE);
	DEBUG_ERR(("usb_hal_bus_deinit() : LDO = %d\n", tmp8));
	if (tmp8 == 0x00){
		DEBUG_ERR(("usb_hal_bus_deinit() : LDO is off. No necessary to Turn it off.\n"));
	} else {
		DEBUG_ERR(("usb_hal_bus_deinit() : LDO is on. Need to Turn it off.\n"));
		usbvendorrequest(&padapter->dvobjpriv, RT_USB_SET_REGISTER, RT_USB_LDO, RT_USB_LDO_OFF,  NULL, 0, _FALSE);
		msleep_os(100); 
		usbvendorrequest(&padapter->dvobjpriv, RT_USB_GET_REGISTER, RT_USB_LDO, 0, &tmp8, 1, _TRUE);
		DEBUG_ERR(("usb_hal_bus_deinit() : LDO = %d\n", tmp8));

	}

		
	_func_exit_;
	return _SUCCESS;
 }



