

#ifndef __OSDEP_INTF_H_
#define __OSDEP_INTF_H_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#ifdef PLATFORM_LINUX

#ifdef CONFIG_USB_HCI
#include <linux/usb.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
#include <linux/usb_ch9.h>
#else
#include <linux/usb/ch9.h>
#endif
#endif

#endif

#ifdef PLATFORM_OS_XP

#ifdef CONFIG_SDIO_HCI

#include <ntddsd.h>

#elif CONFIG_USB_HCI

#include <usb.h>
#include <usbioctl.h>
#include <usbdlib.h>

#endif

#endif


#define RND4(x)	(((x >> 2) + (((x & 3) == 0) ?  0: 1)) << 2)


#if 0
#define NUM_IOREQ		256


// IO COMMAND TYPE
#define _IOSZ_MASK_		(0x7F)
#define _IO_WRITE_		BIT(7)
#define _IO_FIXED_		BIT(8)
#define _IO_BURST_		BIT(9)
#define _IO_BYTE_		BIT(10)
#define _IO_HW_			BIT(11)
#define _IO_WORD_		BIT(12)
#define _IO_SYNC_		BIT(13)
#define _IO_CMDMASK_	(0x1FFF)


/* 
	When it's prompt, caller shall free io_req
	Otherwise, io_handler will free io_req
*/



// IO STATUS TYPE
#define _IO_ERR_		BIT(2)
#define _IO_SUCCESS_	BIT(1)
#define _IO_DONE_		BIT(0)



#define IO_RD32			(_IO_SYNC_ | _IO_WORD_)
#define IO_RD16			(_IO_SYNC_ | _IO_HW_)
#define IO_RD8			(_IO_SYNC_ | _IO_BYTE_)

#define IO_WSYN32		(_IO_WRITE_ | _IO_SYNC_ | _IO_WORD_)
#define IO_WSYN16		(_IO_WRITE_ | _IO_SYNC_ | _IO_HW_)
#define IO_WSYN8		(_IO_WRITE_ | _IO_SYNC_ | _IO_BYTE_)

#define IO_WR32			(_IO_WRITE_ | _IO_WORD_)
#define IO_WR16			(_IO_WRITE_ | _IO_HW_)
#define IO_WR8			(_IO_WRITE_ | _IO_BYTE_)

#define IO_WSYNC_BURST(x)	(_IO_WRITE_ | _IO_SYNC_ | _IO_BURST_ | ( (x) & _IOSZ_MASK_))
#define IO_WR_BURST(x)		(_IO_WRITE_ | _IO_BURST_ |  ( (x) & _IOSZ_MASK_))
#define IO_RD_BURST(x)		(_IO_SYNC_ | _IO_BURST_ | ( (x) & _IOSZ_MASK_))



#ifdef CONFIG_USB_HCI

#define USB_IOREADY			0
#define USB_WAIT_COMPLETE	1
#define USB_WAIT_RSP		2


#endif

#endif



struct intf_priv {
	
	u8 *intf_dev;
	u32	max_iosz; 	//USB2.0: 128, USB1.1: 64, SDIO:64
	u32	max_xmitsz; //USB2.0: unlimited, SDIO:512
	u32	max_recvsz; //USB2.0: unlimited, SDIO:512

	volatile u8 *io_rwmem;
	volatile u8 *allocated_io_rwmem;
	u32	io_wsz; //unit: 4bytes
	u32	io_rsz;//unit: 4bytes
	u8 intf_status;
	
	
	void (*_bus_io)(u8 *priv);	

/*
Under Sync. IRP (SDIO/CFIO)
A protection mechanism is necessary for the io_rwmem(read/write protocol)

Under Async. IRP (SDIO/USB)
The protection mechanism is through the pending queue.

*/

#ifdef USE_SYNC_IRP
	
	_rwlock rwlock;
	
#endif
	
#ifdef PLATFORM_LINUX
	
	#ifdef CONFIG_USB_HCI
	
	// when in USB, IO is through interrupt in/out endpoints
	struct usb_device 	*udev;
	PURB	piorw_urb;
	u8 io_irp_cnt;
	u8 bio_irp_pending;
	_sema io_retevt;//NDIS_EVENT	io_irp_return_evt;
	_timer	io_timer;
	u8 bio_irp_timeout;
	u8 bio_timer_cancel;
	#endif

#endif


#ifdef PLATFORM_OS_XP

	#ifdef CONFIG_SDIO_HCI
		// below is for io_rwmem...	
		PMDL pmdl;
		PSDBUS_REQUEST_PACKET  sdrp;

			PSDBUS_REQUEST_PACKET  recv_sdrp;

			PSDBUS_REQUEST_PACKET  xmit_sdrp;


		#ifdef USE_ASYNC_IRP
			PIRP		piorw_irp;
		#endif


	#endif





	#ifdef CONFIG_USB_HCI


		PURB	piorw_urb;
		PIRP		piorw_irp;
		u8 io_irp_cnt;
		u8 bio_irp_pending;
		_sema io_retevt;//NDIS_EVENT	io_irp_return_evt;

	
	#endif


#endif


};	


struct intf_hdl;
#ifdef PLATFORM_LINUX
int r8711_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
#endif

#endif	//_OSDEP_INTF_H_

