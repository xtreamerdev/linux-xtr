/******************************************************************************
* sdio_halinit.c                                                                                                                                 *
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



#include <sdio_hal.h>





#ifndef CONFIG_SDIO_HCI

#error "CONFIG_SDIO_HCI shall be on!\n"

#endif

u8 sd_hal_bus_init(_adapter * padapter){
	u32			val32;
	u8			val8;

//	u16 			i = 0;
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
//	NDIS_STATUS ndisstatus=NDIS_STATUS_SUCCESS;
	
	_func_enter_;


	//************************************************************************************************************
	// Initialization  SDIO
	//************************************************************************************************************

	//4 	1.SDIO need to turn on the LDO

	val8  =  read8(padapter, LDOOFF);		

	val8 |= _LDOOFF;

	write8(padapter, LDOOFF, val8);

	//NdisMSleep(50000); //delay 50ms
	msleep_os(50);


	
	//4  2.Enable VCO (if Xtal mode. )  
	

	//4 3.Set the HSSEL in local(HCI)
	write8(padapter, HSYSCLK, _HSSEL);
	//NdisMSleep(10000); //delay 10ms
	msleep_os( 10);


#ifdef _test_list_
	if(chk_sysclk_init_val(padapter)==_FALSE)
	{
		DEBUG_ERR(("\n\n####### Error(item76) => chk_sysclk_init_val != 0 fail #######\n\n"));
	}
#endif

	val32 = read32(padapter, SYSCLKR);

	if (pregistrypriv->chip_version == RTL8711_FPGA)
	{
		write32(padapter, SYSCLKR, (val32 | _LXBUS0S | _HWMASK | _HSSEL));
	}
	else
	{
        	write32(padapter, SYSCLKR, (val32 | _HWMASK | _HSSEL));

		//determine the chip version 
		if( (val32&BIT(31)) == 1 )
		{
			pregistrypriv->chip_version = RTL8711_3rdCUT;
                        DbgPrint("RTL8711_3rdCUT\n");
		}
		else
		{
			val32 = read32(padapter, AFEBYPS_CFG);
			if((val32&BIT(0)) == 1)
			{
				pregistrypriv->chip_version = RTL8711_2ndCUT;
                                DbgPrint("RTL8711_2ndCUT\n");
			}
			else
			{
				pregistrypriv->chip_version = RTL8711_1stCUT;
                                DbgPrint("RTL8711_1stCUT\n");
			}			
		
		}		
	}

#ifdef CONFIG_SDIO_HCI//temp

	DEBUG_INFO(("Turn  _SCRST | _UARTRST | BIT(3) !"));

	val32 = read32(padapter, SYSCTRL);

	write32(padapter, SYSCTRL, (val32 |_SCRST | _UARTRST | BIT(3)));

	//NdisMSleep(200000); //delay 
	msleep_os(200);// 200 ms

	val32= read32(padapter, SYSCTRL);

	DEBUG_INFO(("^^^^^^^^^SYSCTRL : %x !\n", val32));
	
#endif


	_func_exit_;
	return _SUCCESS;

}
 u8 sd_hal_bus_deinit(_adapter * padapter){
       u8	val8;
       u32 	val32;
	   
	_func_enter_;

	//Hold CPU reset	
	val32 = read32(padapter, SYSCTRL);

	write32(padapter, SYSCTRL, 0);
	
	val32 = read32(padapter, SYSCTRL);

	DEBUG_INFO(("SYSCONFIG_SYSCTRL = 0x%x\n", val32));

	//Turn off LDO	
	val8 = read8(padapter,  LDOOFF);
	
	DEBUG_INFO(("Before writing LDOOFF: Read HCI LDOOFF =%x\n",val8));
	
	val8 &= 0xFE;
	
	write8(padapter, LDOOFF, val8);	
	
	msleep_os( 50);

	val8 = read8(padapter, LDOOFF);

	DEBUG_INFO(("After writing LDOOFF: Read HCI LDOOFF =%x\n", val8));

	_func_exit_;
	return _SUCCESS;
 }

