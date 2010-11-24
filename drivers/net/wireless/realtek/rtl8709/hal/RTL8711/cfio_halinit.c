/******************************************************************************
* cfio_halinit.c                                                                                                                                 *
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


#include <cfio_hal.h>




u8 cf_hal_bus_init(_adapter * padapter){
	u32			val32;
	u8			val8;

	struct registry_priv *pregistrypriv = &padapter->registrypriv;
	
	_func_enter_;


	_func_exit_;
	return _SUCCESS;

}
 u8 cf_hal_bus_deinit(_adapter * adapter){

	_func_enter_;
	
	_func_exit_;
	return _SUCCESS;
 }



