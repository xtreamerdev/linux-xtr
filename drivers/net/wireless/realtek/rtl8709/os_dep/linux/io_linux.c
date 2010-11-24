/******************************************************************************
* io_linux.c                                                                                                                                 *
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
#define _IO_OSDEP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#include <osdep_intf.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif



#ifdef PLATFORM_LINUX

#include <linux/compiler.h>
//#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/smp_lock.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/usb.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
#include <linux/usb_ch9.h>
#else
#include <linux/usb/ch9.h>
#endif
#include <linux/circ_buf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#include <asm/semaphore.h>
//#include <drv_conf.h>
//#include <io.h>
//#include <rtl8711.h>
//#include <rtl8711_hw.h>

#endif

#ifdef PLATFORM_WINDOWS

#include <osdep_service.h>
#include <drv_types.h>
//#include <rtl8711_debug.h>
//#include <io.h>
#include <osdep_intf.h>

#endif


#ifdef CONFIG_SDIO_HCI
#include <sdio_ops.h>
#endif
#ifdef CONFIG_CFIO_HCI
#include <cf_ops.h>
#endif
#ifdef CONFIG_USB_HCI
#include <usb_ops.h>
#endif






