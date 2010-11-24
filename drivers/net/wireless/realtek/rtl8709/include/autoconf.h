/* Automatically generated C config: don't edit
 */
#define	AUTOCONF_INCLUDED
#define	RTL8711_MODULE_NAME	"rtl8187_usb"


//turn on/off debug message
#undef	CONFIG_DEBUG_RTL8711 


//device interface
#undef	CONFIG_SDIO_HCI
#undef	CONFIG_CFIO_HCI 
#undef	CONFIG_LM_HCI
#define	CONFIG_USB_HCI			1

//atenna selection
//#define CONFIG_1T1R


//chip type
#define	CONFIG_RTL8192U
#undef	CONFIG_RTL8187
#undef	CONFIG_RTL8711
#undef	CONFIG_RTL8712
#undef	CONFIG_RTL8716


//io
#define	USE_SYNC_IRP 1
#undef	USE_ASYNC_IRP 
#define	CONFIG_LITTLE_ENDIAN		1
#undef	CONFIG_BIG_ENDIAN

//#define	CONFIG_PWRCTRL	1
//#define	CONFIG_H2CLBK		1
#define	CONFIG_HWPKT_END

#ifdef linux
	#define	PLATFORM_LINUX		1
	#define	PLATFORM_OS_LINUX	1
	#undef	PLATFORM_WINDOWS 
	#undef	PLATFORM_OS_XP 
	#undef	PLATFORM_OS_CE 
#else
	#define	PLATFORM_WINDOWS
	#define	PLATFORM_OS_XP
	#undef	PLATFORM_OS_CE
#endif

#define	PLATFORM_DVR	1

#define	CONFIG_WMM		1
#undef	CONFIG_MP_INCLUDED
#define	CONFIG_POLLING_MODE
//#define	dlfw_blk_wo_chk
//#define	burst_read_eeprom

#define	LINUX_TASKLET
#define	DISABLE_EVENT_THREAD
//#define	RX_FUNCTION 
#if defined(RX_FUNCTION) || defined(LINUX_TASKLET)
#define	ENABLE_MGNT_THREAD
#endif

//FW image version
#undef	IMAGE_3_26
#undef	IMAGE_4_10
#define	IMAGE_4_DEBUG


#define	CONFIG_EMBEDDED_FWIMG
#undef	CONFIG_SUPPORT_40MHZ

