#define _IO_OSDEP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>





#ifdef PLATFORM_LINUX

#include <linux/compiler.h>
#include <linux/config.h>
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
#include <linux/usb_ch9.h>
#include <linux/circ_buf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#include <asm/semaphore.h>
#include <io.h>
#include <rtl8711.h>
#include <rtl8711_hw.h>

#endif

#ifdef PLATFORM_WINDOWS

#include <osdep_intf.h>

#endif


#ifdef CONFIG_SDIO_HCI
#include <sdio_ops.h>
#elif CONFIG_CFIO_HCI
#include <cf_ops.h>
#elif CONFIG_USB_HCI
#include <usb_ops.h>
#endif





