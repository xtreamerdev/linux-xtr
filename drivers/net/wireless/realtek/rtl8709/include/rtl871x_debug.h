

#ifndef __RTL871X_DEBUG_H__
#define __RTL871X_DEBUG_H__

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


#define _drv_emerg_			0
#define _drv_alert_			1
#define _drv_crit_			2
#define _drv_err_			3
#define	_drv_warning_		4
#define _drv_notice_		5
#define _drv_info_			6
#define	_drv_debug_			7



#define 	_module_rtl871x_xmit_c_ 		BIT(0)
#define 	_module_xmit_osdep_c_ 		BIT(1)
#define 	_module_rtl871x_recv_c_ 		BIT(2)
#define 	_module_recv_osdep_c_ 		BIT(3)
#define 	_module_rtl871x_mlme_c_ 		BIT(4)
#define	_module_mlme_osdep_c_ 		BIT(5)
#define 	_module_rtl871x_sta_mgt_c_ 	BIT(6)
#define 	_module_rtl871x_cmd_c_ 			BIT(7)
#define	_module_cmd_osdep_c_ 	BIT(8)
#define 	_module_rtl871x_io_c_ 				BIT(9)
#define	_module_io_osdep_c_ 		BIT(10)
#define 	_module_os_intfs_c_			BIT(11)
#define 	_module_rtl871x_security_c_		BIT(12)
#define 	_module_rtl871x_eeprom_c_			BIT(13)
#define 	_module_hal_init_c_		BIT(14)
#define 	_module_hci_hal_init_c_		BIT(15)
#define 	_module_rtl871x_ioctl_c_		BIT(16)
#define 	_module_rtl871x_ioctl_set_c_		BIT(17)
#define 	_module_rtl871x_ioctl_query_c_	BIT(18)
#define 	_module_rtl871x_pwrctrl_c_			BIT(19)
#define 	_module_hci_intfs_c_			BIT(20)
#define 	_module_hci_ops_c_			BIT(21)
#define 	_module_osdep_service_c_			BIT(22)
#define 	_module_rtl871x_mp_ioctl_c_			BIT(23)
#define 	_module_hci_ops_os_c_			BIT(24)
#define 	_module_rtl871x_ioctl_os_c			BIT(25)
#define 	_module_rtl8187_files_c			BIT(26)
#define		_module_rtl8192u_files_c		BIT(27)

#if defined _RTL871X_XMIT_C_
	#define _MODULE_DEFINE_	_module_rtl871x_xmit_c_	//0
#elif defined _XMIT_OSDEP_C_
	#define _MODULE_DEFINE_	_module_xmit_osdep_c_	
#elif defined _RTL871X_RECV_C_
	#define _MODULE_DEFINE_	_module_rtl871x_recv_c_
#elif defined _RECV_OSDEP_C_
	#define _MODULE_DEFINE_	_module_recv_osdep_c_	
#elif defined _RTL871X_MLME_C_
	#define _MODULE_DEFINE_	_module_rtl871x_mlme_c_
#elif defined _MLME_OSDEP_C_
	#define _MODULE_DEFINE_	_module_mlme_osdep_c_	//5
#elif defined _RTL871X_STA_MGT_C_
	#define _MODULE_DEFINE_	_module_rtl871x_sta_mgt_c_
#elif defined _RTL871X_CMD_C_
	#define _MODULE_DEFINE_	_module_rtl871x_cmd_c_
#elif defined _CMD_OSDEP_C_
	#define _MODULE_DEFINE_	_module_cmd_osdep_c_
#elif defined _RTL871X_IO_C_
	#define _MODULE_DEFINE_	_module_rtl871x_io_c_
#elif defined _IO_OSDEP_C_
	#define _MODULE_DEFINE_	_module_io_osdep_c_	//10
#elif defined _OS_INTFS_C_
	#define	_MODULE_DEFINE_	_module_os_intfs_c_
#elif defined _RTL871X_SECURITY_C_
	#define	_MODULE_DEFINE_	_module_rtl871x_security_c_	
#elif defined _RTL871X_EEPROM_C_
	#define	_MODULE_DEFINE_	_module_rtl871x_eeprom_c_	
#elif defined _HAL_INIT_C_
	#define	_MODULE_DEFINE_	_module_hal_init_c_	
#elif defined _HCI_HAL_INIT_C_
	#define	_MODULE_DEFINE_	_module_hci_hal_init_c_	//15
#elif defined _RTL871X_IOCTL_C_
	#define	_MODULE_DEFINE_	_module_rtl871x_ioctl_c_	
#elif defined _RTL871X_IOCTL_SET_C_
	#define	_MODULE_DEFINE_	_module_rtl871x_ioctl_set_c_	
#elif defined _RTL871X_IOCTL_QUERY_C_
	#define	_MODULE_DEFINE_	_module_rtl871x_ioctl_query_c_		
#elif defined _RTL871X_PWRCTRL_C_
	#define	_MODULE_DEFINE_	_module_rtl871x_pwrctrl_c_			
#elif defined _HCI_INTF_C_
	#define	_MODULE_DEFINE_	_module_hci_intfs_c_	//20
#elif defined _HCI_OPS_C_
	#define	_MODULE_DEFINE_	_module_hci_ops_c_	
#elif defined _OSDEP_SERVICE_C_
	#define	_MODULE_DEFINE_	_module_osdep_service_c_		
#elif defined _RTL871X_MP_IOCTL_C_
	#define	_MODULE_DEFINE_	_module_rtl871x_mp_ioctl_c_		
#elif defined _HCI_OPS_OS_C_
	#define	_MODULE_DEFINE_	_module_hci_ops_os_c_
#elif defined _RTL871X_IOCTL_LINUX_C_
	#define	_MODULE_DEFINE_	_module_rtl871x_ioctl_os_c	//25		
#elif defined _RTL8187_RELATED_FILES_C
	#define	_MODULE_DEFINE_	_module_rtl8187_files_c			
#elif defined _RTL8192U_RELATED_FILES_C
	#define _MODULE_DEFINE_ _module_rtl8192u_files_c
#else	
	#undef	_MODULE_DEFINE_	
#endif		
	
	
	
#define RT_TRACE(_Comp, _Level, Fmt) {}

#define DEBUG_ERR(_x_) 	{}

#define DEBUG_INFO(_x_) 	{}
	
#define _func_enter_ {}
	
#define _func_exit_ {}


#ifdef CONFIG_DEBUG_RTL8711

#ifndef _RTL8711_DEBUG_C_

	extern u32 GlobalDebugLevel;
	extern	u64 GlobalDebugComponents;

#endif

	#if defined PLATFORM_WINDOWS

		#define _dbgdump	DbgPrint

	#elif defined PLATFORM_LINUX

		#define _dbgdump	printk

	#else
	
		#undef	_dbgdump

	#endif

	#if	defined (_dbgdump) && defined (_MODULE_DEFINE_)
	
		#undef RT_TRACE
		#define RT_TRACE(_Comp, _Level, Fmt)\
		do { \										\
			if(((_Comp) & GlobalDebugComponents) && (_Level <= GlobalDebugLevel))	\
			{																	\
				_dbgdump("%s", RTL8711_MODULE_NAME);												\
				_dbgdump Fmt;														\
			}\
		while(0)
	
		#undef DEBUG_ERR
		
		#define DEBUG_ERR(_x_) \
		do {\
			if ((_MODULE_DEFINE_ & GlobalDebugComponents) && (GlobalDebugLevel >= _drv_err_)) {\
				_dbgdump _x_;\
			}\
		} while(0)
	
		#undef DEBUG_INFO
		#define DEBUG_INFO(_x_) \
		do {\
			if ((_MODULE_DEFINE_ & GlobalDebugComponents) && (GlobalDebugLevel >= _drv_info_)) {\
				_dbgdump _x_;\
			}\
		} while(0)
	
	#endif
	
	#if	defined (_dbgdump)
	
		#undef  _func_enter_
		#define _func_enter_ \
		do {	\
			if (GlobalDebugLevel >= _drv_debug_) \
			{																	\
				_dbgdump("\n %s : %s enters at %d\n", RTL8711_MODULE_NAME, __FUNCTION__, __LINE__);\
			}		\
		} while(0)
	
		#undef  _func_exit_
		#define _func_exit_ \
		do {	\
			if (GlobalDebugLevel >= _drv_debug_) \
			{																	\
				_dbgdump("\n %s : %s exits at %d\n", RTL8711_MODULE_NAME, __FUNCTION__, __LINE__); \
			}	\
		} while(0)
	
	#endif

#endif

#endif	//__RTL8711_DEBUG_H__

