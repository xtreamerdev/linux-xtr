/******************************************************************************
* rtl871x_debug.c                                                                                                                                 *
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

#define _RTL871X_DEBUG_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>



#ifdef CONFIG_DEBUG_RTL8711

	u32 GlobalDebugLevel =_drv_err_	;

	u64 GlobalDebugComponents	= \
			_module_rtl871x_xmit_c_ |
			_module_xmit_osdep_c_ 	|
			_module_rtl871x_recv_c_ |
			_module_recv_osdep_c_ 	|
			_module_rtl871x_mlme_c_ |
			_module_mlme_osdep_c_ 	|
			_module_rtl871x_sta_mgt_c_ 		|
			_module_rtl871x_cmd_c_ 	|
			_module_cmd_osdep_c_ 	|
			_module_rtl871x_io_c_   | 
			_module_io_osdep_c_ 	|
			_module_os_intfs_c_|
			_module_rtl871x_security_c_|
			_module_rtl871x_eeprom_c_|
			_module_hal_init_c_|
			_module_hci_hal_init_c_|
			_module_rtl871x_ioctl_c_|
			_module_rtl871x_ioctl_set_c_|
			_module_rtl871x_ioctl_query_c_|
			_module_rtl871x_pwrctrl_c_|
			_module_hci_intfs_c_|
			_module_hci_ops_c_|
			_module_rtl871x_mp_ioctl_c_|
			_module_hci_ops_os_c_|
			_module_rtl871x_ioctl_os_c|
			_module_rtl8187_files_c;

									
#endif

