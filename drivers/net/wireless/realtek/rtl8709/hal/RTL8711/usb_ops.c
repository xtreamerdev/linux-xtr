/******************************************************************************
* usb_ops.c                                                                                                                                 *
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
#define _HCI_OPS_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>
#include <usb_ops.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_USB_HCI

	#error "CONFIG_USB_HCI shall be on!\n"

#endif




#ifdef PLATFORM_LINUX

#endif

#ifdef PLATFORM_WINDOWS



#include <rtl8711_spec.h>
#include <usb.h>
#include <usbdlib.h>
#include <usbioctl.h>
#include <usb_ops.h>
#include <recv_osdep.h>

#define USBD_HALTED(Status) ((ULONG)(Status) >> 30 == 3)

#endif

u8 usb_read8(struct intf_hdl *intf_hdl, u32 addr)
{
_func_enter_;
#ifdef PLATFORM_WINDOWS

	




#endif	
	_func_exit_;
	return 0;
		
}
u16 usb_read16(struct intf_hdl *intf_hdl, u32 addr)
{       
	_func_enter_;
#ifdef PLATFORM_WINDOWS

	




#endif	
	_func_exit_;
     return 0;
}
u32 usb_read32(struct intf_hdl *intf_hdl, u32 addr)
{
	_func_enter_;
#ifdef PLATFORM_WINDOWS

	




#endif	
 	 _func_exit_;
   return 0;	
		
}
void usb_write8(struct intf_hdl *pintfhdl, u32 addr, u8 val)
{
	_func_enter_;
#ifdef PLATFORM_WINDOWS

	




#endif	
	_func_exit_;
	
}
void usb_write16(struct intf_hdl *pintfhdl, u32 addr, u16 val)
{
	_func_enter_;
#ifdef PLATFORM_WINDOWS

	




#endif	
	_func_exit_;
	
}
void usb_write32(struct intf_hdl *pintfhdl, u32 addr, u32 val)
{
	_func_enter_;
#ifdef PLATFORM_WINDOWS

	




#endif	
	_func_exit_;
}

void usb_read_mem(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	_func_enter_;
	_func_exit_;
}

void usb_write_mem(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{
	_func_enter_;
	_func_exit_;
}

void usb_set_intf_option(u32 *poption)
{
	
	*poption = ((*poption) | _INTF_ASYNC_);
	_func_enter_;
	_func_exit_;
	
}

void usb_intf_hdl_init(u8 *priv)
{
	_func_enter_;
	_func_exit_;
	
	
}

void usb_intf_hdl_unload(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	
}
void usb_intf_hdl_open(u8 *priv)
{
	_func_enter_;
	_func_exit_;
	
	
}

void usb_intf_hdl_close(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	
	
}


void usb_set_intf_funs(struct intf_hdl *pintf_hdl)
{
	
	pintf_hdl->intf_hdl_init = &usb_intf_hdl_init;
	pintf_hdl->intf_hdl_unload = &usb_intf_hdl_unload;
	pintf_hdl->intf_hdl_open = &usb_intf_hdl_open;
	pintf_hdl->intf_hdl_close = &usb_intf_hdl_close;
	_func_enter_;
	_func_exit_;
}


void usb_set_io_maxpktsz(u32	*maxpktsz)
{
		
	*maxpktsz = 64;	
	_func_enter_;
	_func_exit_;
}	

/*

*/
uint usb_init_intf_priv(struct intf_priv *pintfpriv)
{
	


        
#ifdef PLATFORM_WINDOWS
	u8		NextDeviceStackSize;
	PURB	piorw_urb;
#endif
	struct dvobj_priv   *pdev = (struct dvobj_priv   *)pintfpriv->intf_dev;	
#ifdef PLATFORM_LINUX
	//struct usb_device *pudev=pNdisCEDvice->pusbdev;
	_adapter * padapter=pdev->padapter;
#endif
	pintfpriv->intf_status = _IOREADY;
	//pintfpriv->len = pintfpriv->done_len = 0;
	//pintfpriv->max_pktsz = 64;
       //pintfpriv->max_xmitsz =  512;
	//pintfpriv->max_recvsz =  512;
	_func_enter_;
	DEBUG_INFO(("\n +usb_init_intf_priv\n"));
#ifdef PLATFORM_WINDOWS	
       if(pdev->ishighspeed) pintfpriv->max_iosz =  128;
	else pintfpriv->max_iosz =  64;	
#endif
#ifdef PLATFORM_LINUX
//	if(pudev->speed==USB_SPEED_HIGH)
//		pintfpriv->max_iosz=128;
//	else
		pintfpriv->max_iosz=64;
	_init_timer(&pintfpriv->io_timer,padapter->pnetdev, io_irp_timeout_handler,pintfpriv);
#endif
	DEBUG_ERR(("\n usb_init_intf_priv:pintfpriv->max_iosz:%d\n",pintfpriv->max_iosz));
	pintfpriv->io_wsz = 0;
	pintfpriv->io_rsz = 0;	
	
      //_rwlock_init(&pintfpriv->rwlock);	
	//_init_listhead(&pintfpriv->curlist);
	//_spinlock_init(&pintfpriv->rwlock);
	
	pintfpriv->allocated_io_rwmem = _malloc(pintfpriv->max_iosz +4); 
	
    if (pintfpriv->allocated_io_rwmem == NULL){
	DEBUG_ERR(("\n usb_init_intf_priv:pintfpriv->allocated_io_rwmem == NULL\n"));
    	goto usb_init_intf_priv_fail;
	}

	pintfpriv->io_rwmem = pintfpriv->allocated_io_rwmem +  4\
	- ( (u32)(pintfpriv->allocated_io_rwmem) & 3);
	
#ifdef PLATFORM_WINDOWS
     
     
  //   pintfpriv->pUsbDevObj = pNdisCEDvice->pnextdevobj;
 //    pintfpriv->RegInterruptInPipeHandle =pNdisCEDvice->regin_pipehandle;
 //   pintfpriv->RegInterruptOutPipeHandle = pNdisCEDvice->regout_pipehandle;

     NextDeviceStackSize = (u8) pdev->nextdevstacksz;//pintfpriv->pUsbDevObj->StackSize + 1;	 

      piorw_urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(URB) ); 
      if(piorw_urb == NULL) 
	  goto usb_init_intf_priv_fail;
	  
     pintfpriv->piorw_urb = piorw_urb;

     pintfpriv->piorw_irp = IoAllocateIrp(NextDeviceStackSize , FALSE);	 
    
	 
#endif
#ifdef PLATFORM_LINUX
	pintfpriv->piorw_urb=usb_alloc_urb(0,GFP_ATOMIC);
	if(pintfpriv->piorw_urb==NULL)
		DEBUG_ERR(("\npintfpriv->piorw_urb==NULL!!!\n"));
	_init_sema(&(pintfpriv->io_retevt),0);
#endif
     pintfpriv->io_irp_cnt=1;
     pintfpriv->bio_irp_pending=_FALSE;
	 
     	_init_sema(&(pintfpriv->io_retevt), 0);//NdisInitializeEvent(&pintfpriv->io_irp_return_evt);
		_func_exit_;
		return _SUCCESS;

usb_init_intf_priv_fail:

	if (pintfpriv->allocated_io_rwmem)
		_mfree((u8 *)(pintfpriv->allocated_io_rwmem), pintfpriv->max_iosz +4);

#ifdef PLATFORM_WINDOWS	
	if(piorw_urb)
		ExFreePool(piorw_urb);	
#endif
	DEBUG_INFO(("\n -usb_init_intf_priv(usb_init_intf_priv_fail)\n"));
	_func_exit_;
	return _FAIL;
		
}

void usb_unload_intf_priv(struct intf_priv *pintfpriv)
{
	_func_enter_;
	DEBUG_ERR(("\n+usb_unload_intf_priv\n"));
	_mfree((u8 *)(pintfpriv->allocated_io_rwmem), pintfpriv->max_iosz+4);
#ifdef PLATFORM_WINDOWS
	if(pintfpriv->piorw_urb)
		ExFreePool(pintfpriv->piorw_urb);	

	if(pintfpriv->piorw_irp)
		IoFreeIrp(pintfpriv->piorw_irp);
#endif		
#ifdef PLATFORM_LINUX
	DEBUG_ERR(("\npintfpriv->io_irp_cnt=%d\n",pintfpriv->io_irp_cnt));
	pintfpriv->io_irp_cnt--;
	if(pintfpriv->io_irp_cnt){
		if(pintfpriv->bio_irp_pending==_TRUE){
		DEBUG_ERR(("\nkill iorw_urb\n"));
		usb_kill_urb(pintfpriv->piorw_urb);
		}
		DEBUG_ERR(("\n wait io_retevt\n"));
		_down_sema(&(pintfpriv->io_retevt));
	}
	DEBUG_ERR(("\n cancel io_urb ok\n"));
#endif
	DEBUG_ERR(("\n-usb_unload_intf_priv\n"));
	_func_exit_;
}


void usb_set_intf_ops(struct _io_ops	*pops)
{
	_func_enter_;
	memset((u8 *)pops, 0, sizeof(struct _io_ops));
	
#ifdef USE_SYNC_IRP	
	pops->_read8 = &usb_read8;
	pops->_read16 = &usb_read16;
	pops->_read32 = &usb_read32;
	pops->_read_mem = &usb_read_mem;//yt;20061108
	
	pops->_write8 = &usb_write8;
	pops->_write16 = &usb_write16;
	pops->_write32 = &usb_write32;
	pops->_write_mem = &usb_write_mem; //yt;20061108

#endif	
	


#ifdef USE_ASYNC_IRP
       pops->_async_irp_protocol_read = &_async_protocol_read;
       pops->_async_irp_protocol_write = &_async_protocol_write;
       
       pops->_read_port = &usb_read_port;	
	pops->_write_port = &usb_write_port;	   
	pops->_write_scsi = &usb_write_scsi;	   


#endif
	_func_exit_;

}

#if 0	
void usb_set_intf_callbacks(struct _io_callbacks	*pcallbacks)
{
	
	memset((u8 *)pcallbacks, 0, sizeof(struct _io_callbacks));
	
	
}
#endif
#ifdef PLATFORM_WINDOWS
void io_irp_timeout_handler (
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	)
#endif
#ifdef PLATFORM_LINUX
void io_irp_timeout_handler (IN	PVOID FunctionContext)
#endif
{
       struct intf_priv *pintfpriv= ( struct intf_priv *)FunctionContext;


_func_enter_;		
	DEBUG_ERR(("^^^io_irp_timeout_handler ^^^\n"));
	
#ifdef PLATFORM_LINUX
	pintfpriv->bio_irp_timeout=_TRUE;
	usb_kill_urb(pintfpriv->piorw_urb);
#endif		
_func_exit_;	

}


