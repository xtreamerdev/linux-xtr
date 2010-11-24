/******************************************************************************
* rtl871x_io.c                                                                                                                                 *
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
/*

The purpose of rtl8711_io.c

a. provides the API 

b. provides the protocol engine

c. provides the software interface between caller and the hardware interface


Compiler Flag Option:

1. CONFIG_SDIO_HCI:
    a. USE_SYNC_IRP:  Only sync operations are provided.
    b. USE_ASYNC_IRP:Both sync/async operations are provided.

2. CONFIG_USB_HCI:
   a. USE_ASYNC_IRP: Both sync/async operations are provided.

3. CONFIG_CFIO_HCI:
   b. USE_SYNC_IRP: Only sync operations are provided.


Only sync read/write_mem operations are provided.

jackson@realtek.com.tw

*/



#define _RTL871X_IO_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtl8711_spec.h>
#include <rtl871x_io.h>


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

#ifdef  PLATFORM_WINDOWS

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



#ifdef USE_SYNC_IRP

static u8 sync_irp_read8(_adapter *adapter, u32 addr);
static u16 sync_irp_read16(_adapter *adapter, u32 addr);
static u32 sync_irp_read32(_adapter *adapter, u32 addr);
static void sync_irp_write8(_adapter *adapter, u32 addr, u8 val);
static void sync_irp_write16(_adapter *adapter, u32 addr, u16 val);
static void sync_irp_write32(_adapter *adapter, u32 addr, u32 val);
static void sync_irp_read_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);
static void sync_irp_write_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);


#else

static u8 async_irp_read8(_adapter *adapter, u32 addr);
static u16 async_irp_read16(_adapter *adapter, u32 addr);
static u32 async_irp_read32(_adapter *adapter, u32 addr);
static void async_irp_write8(_adapter *adapter, u32 addr, u8 val);
static void async_irp_write16(_adapter *adapter, u32 addr, u16 val);
static void async_irp_write32(_adapter *adapter, u32 addr, u32 val);
static void async_irp_read_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);
static void async_irp_write_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);



#endif

uint _init_intf_hdl(_adapter *padapter, struct intf_hdl *pintf_hdl)
{
	//_adapter *adapter = (_adapter *)dev;
	struct	intf_priv	*pintf_priv;
	void (*set_intf_option)(u32 *poption) = NULL;
	void (*set_intf_funs)(struct intf_hdl *pintf_hdl);
	void (*set_intf_ops)(struct _io_ops	*pops);
	uint (*init_intf_priv)(struct intf_priv *pintfpriv);
	_func_enter_;
	
#ifdef CONFIG_SDIO_HCI

	set_intf_option = &(sdio_set_intf_option);
	set_intf_funs = &(sdio_set_intf_funs);
	set_intf_ops = &sdio_set_intf_ops;
	init_intf_priv = &sdio_init_intf_priv;
	
#endif

#ifdef CONFIG_CFIO_HCI

       set_intf_option = &(cf_set_intf_option);
	set_intf_funs = &(cf_set_intf_funs);
	set_intf_ops = &cf_set_intf_ops;	
	init_intf_priv = &cf_init_intf_priv;
	
#endif	

#ifdef CONFIG_USB_HCI	

       set_intf_option = &(usb_set_intf_option);
	set_intf_funs = &(usb_set_intf_funs);
	set_intf_ops = &usb_set_intf_ops;
	init_intf_priv = &usb_init_intf_priv;
	
#endif

	pintf_priv = pintf_hdl->pintfpriv =(struct intf_priv *) _malloc(sizeof(struct intf_priv));
	
	if (pintf_priv == NULL){
		DEBUG_ERR(("\n _init_intf_hdl:pintf_priv == NULL\n"));	
		goto _init_intf_hdl_fail;
	}
		
	set_intf_option(&pintf_hdl->intf_option);
	set_intf_funs(pintf_hdl);
	set_intf_ops(&pintf_hdl->io_ops);

	pintf_priv->intf_dev = (u8 *)&(padapter->dvobjpriv);
	if ((init_intf_priv(pintf_priv)) == _FAIL){
		DEBUG_ERR(("\n _init_intf_hdl:(init_intf_priv(pintf_priv)) == _FAIL\n"));	
		goto _init_intf_hdl_fail;
	}
	_func_exit_;
	return _SUCCESS;

_init_intf_hdl_fail:
	if (pintf_priv) {
		_mfree((u8 *)pintf_priv, sizeof(struct intf_priv));
	}
	_func_exit_;
	return _FAIL;
	
}

void _unload_intf_hdl(struct intf_priv *pintfpriv)
{

	void (*unload_intf_priv)(struct intf_priv *pintfpriv);
	_func_enter_;	
#ifdef CONFIG_SDIO_HCI

	unload_intf_priv = &sdio_unload_intf_priv;
#endif
#ifdef CONFIG_CFIO_HCI
       unload_intf_priv = &cf_unload_intf_priv;
#endif
#ifdef CONFIG_USB_HCI	

      unload_intf_priv = &usb_unload_intf_priv;
#endif 
	

	
	unload_intf_priv(pintfpriv);

	if(pintfpriv){
		_mfree((u8 *)pintfpriv, sizeof(struct intf_priv));
	}
	_func_exit_;
}


uint	register_intf_hdl (u8 *dev, struct intf_hdl 	*pintfhdl)
{
	_adapter *adapter = (_adapter *)dev;
	pintfhdl->intf_option = 0;
	pintfhdl->cnt = 0;
	pintfhdl->adapter = dev;
	pintfhdl->intf_dev = (u8 *)&(adapter->dvobjpriv);
	//pintfhdl->intf_status = _IOREADY;
	pintfhdl->len = 0;
	pintfhdl->done_len = 0;
	pintfhdl->do_flush = _FALSE;
	_func_enter_;
	if (_init_intf_hdl(adapter, pintfhdl) == _FALSE)
		goto register_intf_hdl_fail;
	_func_exit_;	
	return _SUCCESS;

register_intf_hdl_fail:
	// shall release all the allocated resources here...
	
	if (pintfhdl)
		_mfree((u8 *)pintfhdl, (sizeof (struct intf_hdl)));
		
	_func_exit_;
	return _FALSE;
	
}


void unregister_intf_hdl(struct intf_hdl *pintfhdl)
{
	_func_enter_;
	_unload_intf_hdl(pintfhdl->pintfpriv);
	//NdisZeroMemory((u8 *)pintfhdl, sizeof(struct intf_hdl));	
	memset((u8 *)pintfhdl, 0, sizeof(struct intf_hdl));	
	_func_exit_;
}

#ifdef USE_SYNC_IRP

static u8 sync_irp_read8(_adapter *adapter, u32 addr) 
{
	u8 r_val;
	struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	u8 (*_read8)(struct intf_hdl *pintfhdl, u32 addr);
	_func_enter_;
	_read8 = pintfhdl->io_ops._read8;
	
	r_val = _read8(pintfhdl, addr);
	_func_exit_;
	return r_val;
}

static u16 sync_irp_read16(_adapter *adapter, u32 addr)
{
	u16 r_val;
	struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl		*pintfhdl = &(pio_queue->intf);
	u16 	(*_read16)(struct intf_hdl *pintfhdl, u32 addr);
	_func_enter_;
	_read16 = pintfhdl->io_ops._read16;

	r_val = _read16(pintfhdl, addr);
	_func_exit_;
	return r_val;
}

static u32 sync_irp_read32(_adapter *adapter, u32 addr)
{
	u32 r_val;
	struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl		*pintfhdl = &(pio_queue->intf);
	u32 	(*_read32)(struct intf_hdl *pintfhdl, u32 addr);
	_func_enter_;
	_read32 = pintfhdl->io_ops._read32;
	
	r_val = _read32(pintfhdl, addr);
	_func_exit_;
	return r_val;	
}

static void sync_irp_write8(_adapter *adapter, u32 addr, u8 val)
{
	struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl		*pintfhdl = &(pio_queue->intf);
	void (*_write8)(struct intf_hdl *pintfhdl, u32 addr, u8 val);
	_func_enter_;
	_write8 = pintfhdl->io_ops._write8;
	
	_write8(pintfhdl, addr, val);
	_func_exit_;

}

static void sync_irp_write16(_adapter *adapter, u32 addr, u16 val)
{

	struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl		*pintfhdl = &(pio_queue->intf);
	void (*_write16)(struct intf_hdl *pintfhdl, u32 addr, u16 val);
	_func_enter_;
	_write16 = pintfhdl->io_ops._write16;
	
	_write16(pintfhdl, addr, val);
	_func_exit_;

	
}

static void sync_irp_write32(_adapter *adapter, u32 addr, u32 val)
{
	
	struct	io_queue  	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl		*pintfhdl = (struct intf_hdl*)(&(pio_queue->intf));
	void (*_write32)(struct intf_hdl *pintfhdl, u32 addr, u32 val);
	_func_enter_;
	_write32 = pintfhdl->io_ops._write32;
	
	_write32(pintfhdl, addr, val);	
	_func_exit_;

}

static void sync_irp_read_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	//u32	val;
	void (*_read_mem)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	_func_enter_;
	_read_mem = pintfhdl->io_ops._read_mem;
	
	_read_mem(pintfhdl, addr, cnt, pmem);
	_func_exit_;
}



static void sync_irp_write_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	//u32	val;
	void (*_write_mem)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	_func_enter_;
	_write_mem = pintfhdl->io_ops._write_mem;
	
	_write_mem(pintfhdl, addr, cnt, pmem);
	_func_exit_;
}

void attrib_read(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	//u32	val;
	struct io_queue * pio_queue = (struct io_queue *)adapter->pio_queue;
	struct intf_hdl	*pintfhdl = &(pio_queue->intf);
	//volatile u8* pio_mem = pintfhdl->pintfpriv->io_rwmem;
	void (*_attrib_read)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	_func_enter_;
	_attrib_read = pintfhdl->io_ops._attrib_read;
	
	_attrib_read(pintfhdl, addr, cnt, pmem);
	_func_exit_;
	return ;
}

void attrib_write(_adapter *adapter, u32 addr, u32 cnt, u8* pmem)
{
	//u32	val;
	struct io_queue * pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	//volatile u8* pio_mem = pintfhdl->pintfpriv->io_rwmem;
	void (*_attrib_write)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	_func_enter_;
	_attrib_write = pintfhdl->io_ops._attrib_write;
	
	_attrib_write(pintfhdl, addr, cnt, pmem);
	 _func_exit_;
	return ;
}



u8 read8(_adapter *adapter, u32 addr)
{
	_func_enter_;
	_func_exit_;
	return sync_irp_read8(adapter, addr);
}

u16 read16(_adapter *adapter, u32 addr)
{
	_func_enter_;
	_func_exit_;

	return sync_irp_read16(adapter, addr);
}
	
u32 read32(_adapter *adapter, u32 addr)
{
	_func_enter_;
	_func_exit_;
	return sync_irp_read32(adapter, addr);	
}

void write8(_adapter *adapter, u32 addr, u8 val)
{
	_func_enter_;
	sync_irp_write8(adapter, addr, val);
	_func_exit_;
}
void write16(_adapter *adapter, u32 addr, u16 val)
{
	_func_enter_;
	sync_irp_write16(adapter, addr, val);
	_func_exit_;
}
void write32(_adapter *adapter, u32 addr, u32 val)
{
      struct registry_priv *pregistrypriv = &adapter->registrypriv;
	  
	_func_enter_; 
	
    if (pregistrypriv->chip_version <= RTL8711_1stCUT)
    {
 #ifdef CONFIG_RTL8711  
        if( (val == HIMR) || (val == HISR))
        {
             sync_irp_write8(adapter, addr, (unsigned char)val);
	      sync_irp_write8(adapter, addr+1, (unsigned char)(val>>8));
	      sync_irp_write8(adapter, addr+2, (unsigned char)(val>>16));
             sync_irp_write8(adapter, addr+3, (unsigned char)(val>>24));			   
        }
	else	
#endif	  
	    sync_irp_write32(adapter, addr, val);

    }else   
    {
       sync_irp_write32(adapter, addr, val);
    }
	_func_exit_;
	
}
void read_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	_func_enter_;
	 sync_irp_read_mem(adapter, addr, cnt, pmem);
	 _func_exit_;
}

void write_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	_func_enter_;
	sync_irp_write_mem(adapter, addr, cnt, pmem);
	_func_exit_;
}

void read_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	//u32	val;
	void (*_read_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	_func_enter_;	


	if( (adapter->bDriverStopped ==_TRUE) || (adapter->bSurpriseRemoved == _TRUE))
		return;
	

	_read_port = pintfhdl->io_ops._read_port;
	
	 _read_port(pintfhdl, addr, cnt, pmem);
	 _func_exit_;

}


void write_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	//u32	val;
	u32 (*_write_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	_func_enter_;	
	_write_port = pintfhdl->io_ops._write_port;
	
	_write_port(pintfhdl, addr, cnt, pmem);
	 _func_exit_;
	 
}

void write_scsi(_adapter *adapter, u32 cnt, u8 *pmem)
{
	
	u32 (*_write_scsi)(struct intf_hdl *pintfhdl, u32 cnt, u8 *pmem);
	
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	_func_enter_;	
	_write_scsi = pintfhdl->io_ops._write_scsi;
	
	_write_scsi(pintfhdl, cnt, pmem);
	_func_exit_;

}


//ioreq 
void ioreq_read8(_adapter *adapter, u32 addr, u8 *pval)
{      
        //u8	val;
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL) {
		
		goto exit;
		
	}
		
	pio_req->command = 	IO_RD8;
	pio_req->addr = addr;
	pio_req->pbuf = pval;
	
	sync_ioreq_enqueue(pio_req, pio_queue);	
exit:
	 _func_exit_;
}

void ioreq_read16(_adapter *adapter, u32 addr, u16 *pval)
{
        //u8	val;
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)

		goto exit;
	
		
	pio_req->command = 	IO_RD16;
	pio_req->addr = addr;
	pio_req->pbuf = (u8 *)pval;
	
	sync_ioreq_enqueue(pio_req, pio_queue);	
exit:	
	 _func_exit_;
}
	
void ioreq_read32(_adapter *adapter, u32 addr, u32 *pval)
{
        //u8	val;
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	pio_req->command = 	IO_RD32;
	pio_req->addr = addr;
	pio_req->pbuf = (u8 *)pval;
	
	sync_ioreq_enqueue(pio_req, pio_queue);	
exit:	
	 _func_exit_;
}

void ioreq_write8(_adapter *adapter, u32 addr, u8 val)
{
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	pio_req->command = 	IO_WR8;
	pio_req->addr = addr;
	pio_req->val = val;
	
	sync_ioreq_enqueue(pio_req, pio_queue);		
exit:	
	_func_exit_;
	
}
void ioreq_write16(_adapter *adapter, u32 addr, u16 val)
{
       struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	pio_req->command = 	IO_WR16;
	pio_req->addr = addr;
	pio_req->val = val;
	
	sync_ioreq_enqueue(pio_req, pio_queue);
exit:	
	_func_exit_;
}
void ioreq_write32(_adapter *adapter, u32 addr, u32 val)
{
      struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
      struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	pio_req->command = 	IO_WR32;
	pio_req->addr = addr;
	pio_req->val = val;
	
	sync_ioreq_enqueue(pio_req, pio_queue);
exit:
	_func_exit_;
}


/*
the caller must lock pintfpriv->rwlock 

*/
void sync_bus_io(struct io_queue *pio_q)
{
        //_irqL	irqL;
	
	void (*_sync_irp_protocol_write)(struct io_queue *pio_q);
	
	struct intf_hdl *pintf = &(pio_q->intf);
	
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	_func_enter_;	
	_sync_irp_protocol_write =pintf->io_ops._sync_irp_protocol_rw;

	if (pintfpriv->intf_status == _IOREADY)
	{
		if (pintf->do_flush == _TRUE)
		{	
			if ((_ioreq2rwmem(pio_q)) == _TRUE)
			{
				if ((pintfpriv->io_rsz) > 1)
					pintfpriv->intf_status = _IO_WAIT_RSP;
				else
					pintfpriv->intf_status = _IO_WAIT_COMPLETE;
					
				_sync_irp_protocol_write(pio_q);				

				pintf->len = 0;//?maybe there are some io_reqs in pending queue
				pintf->done_len =0;//?
				
			}
		}	
	}
	_func_exit_;

}


void sync_ioreq_enqueue(struct io_req *preq, struct io_queue *pio_q)
{
	
	//u32 i;
	
	_irqL	irqL;
	
	struct	intf_hdl	*pintf;
	struct intf_priv	*pintfpriv;
	
	// enqueue anyway...	
	_func_enter_;	
	
	pintf = &(pio_q->intf);
	pintfpriv = pintf->pintfpriv;
	
	//_enter_critical(&(pio_q->lock), &irqL);
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);
	
	list_insert_tail(&(preq->list), &(pio_q->pending));
	
	if (preq->command & _IO_BURST_)
	{	
		if (!((preq->command) & _IO_WRITE_))
		{
			pintf->len += 2*4;	//read will take 2 DW.
			pintf->done_len += 2*4 +(u16) (preq->command & _IOSZ_MASK_);
		}
		else
			pintf->len += 2*4 + (u16)(preq->command & _IOSZ_MASK_); //only allowed 4 bytes alignment
	}
	else
	{
		if (!((preq->command) & _IO_WRITE_))
		{
			pintf->len += 2*4;	//read will take 2 DW.
			pintf->done_len += 3*4 ;
		}
		else
			pintf->len += 3*4; //write will take 3 DW.
		
	}
	
	if (pintf->do_flush == _FALSE)
	{
		if (((pintf->len) >= (pintfpriv->max_iosz - 4)) ||
	  		((pintf->done_len) >= (pintfpriv->max_iosz - 4)))
	 		pintf->do_flush = _TRUE; 	
	  	
	}

	
	sync_bus_io(pio_q);
	 
       //_exit_critical(&(pio_q->lock), &irqL);
       _exit_hwio_critical(&pintfpriv->rwlock, &irqL);
	_func_exit_;

}


uint sync_ioreq_flush(_adapter *adapter, struct io_queue *ioqueue)
{
        _irqL	irqL;
	 uint ret = _TRUE;
//    struct	io_queue *pio_queue = ioqueue;
       struct	io_queue *pio_q = (struct io_queue*)adapter->pio_queue;
	struct	intf_hdl	*pintf = &(pio_q->intf);	
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	_func_enter_;	
	
	//_enter_critical(&(pio_q->lock), &irqL);       
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);
	
	if ((is_list_empty(&(pio_q->pending))) == _TRUE)
	{
	      ret = _FALSE;
	}
	else
	{
	      pintf->do_flush = _TRUE;
	      sync_bus_io(pio_q);    
	}

       //_exit_critical(&(pio_q->lock), &irqL);
       _exit_hwio_critical(&pintfpriv->rwlock, &irqL);
	_func_exit_;
	return ret;
}

#else



/*
async_bus_io:
	This function can be called during any context (isr, process, bh...)
	shall use spin_lock_irqsave/raise to highest irq to avoid racing condition.
	//only asynchronous based operations will call ioreq_enqueue...

*/
void async_bus_io(struct io_queue *pio_q)
{
//	_irqL	irqL;
	
	void (*_async_irp_protocol_write)(struct io_queue *pio_q);
	
	struct intf_hdl *pintf = &(pio_q->intf);
	
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	_func_enter_;	
	
	_async_irp_protocol_write = pintf->io_ops._async_irp_protocol_write;

	if (pintfpriv->intf_status == _IOREADY)
	{
		if (pintf->do_flush == _TRUE)
		{	
			if ((_ioreq2rwmem(pio_q)) == _TRUE)
			{
				if ((pintfpriv->io_rsz) > 1)
					pintfpriv->intf_status = _IO_WAIT_RSP;
				else
					pintfpriv->intf_status = _IO_WAIT_COMPLETE;
					
				_async_irp_protocol_write(pio_q);

				pintf->len = 0;
				pintf->done_len =0;


				
			}
		}	
	}
	_func_exit_;
}


void ioreq_enqueue(struct io_req *preq, struct io_queue *pio_q)
{
	
//	u32 i;
	
	_irqL	irqL;
	
	struct	intf_hdl	*pintf;
	struct intf_priv	*pintfpriv;
	
	// enqueue anyway...	
	
	pintf = &(pio_q->intf);
	pintfpriv = pintf->pintfpriv;
	_func_enter_;	
	_enter_critical(&(pio_q->lock), &irqL);
	
	list_insert_tail(&(preq->list), &(pio_q->pending));
	
	if (preq->command & _IO_BURST_)
	{	
		if (!((preq->command) & _IO_WRITE_))
		{
			pintf->len += 2*4;	//read will take 2 DW.
			pintf->done_len += 2*4 + (u16)(preq->command & _IOSZ_MASK_);
		}
		else
			pintf->len += 2*4 + (u16)(preq->command & _IOSZ_MASK_); //only allowed 4 bytes alignment
	}
	else
	{
		if (!((preq->command) & _IO_WRITE_))
		{
			pintf->len += 2*4;	//read will take 2 DW.
			pintf->done_len += 3*4 ;
		}
		else
			pintf->len += 3*4; //write will take 3 DW.
		
	}
	
	if (pintf->do_flush == _FALSE)
	{
		if (((preq->command) & _IO_SYNC_) ||
	  			((pintf->len) >= (pintfpriv->max_iosz - 4)) ||
	  			((pintf->done_len) >= (pintfpriv->max_iosz - 4)))
	 		pintf->do_flush = _TRUE; 	
	  	
	}

	
	async_bus_io(pio_q);
	
	_exit_critical(&(pio_q->lock), &irqL);
	
	_func_exit_;
}


/*
*	async_irp_read8:
using async_irp to implement syncronous_io_access
*/
static u8 async_irp_read8(_adapter *adapter, u32 addr) 
{

	u8	val;
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL) {
		val = _FAIL;
		goto exit;
	}
	pio_req->command = 	IO_RD8;
	pio_req->addr = addr;
	
	ioreq_enqueue(pio_req, pio_queue);
	
	_down_sema(&pio_req->sema);
	
	val = (u8 )pio_req->val;	
	
	free_ioreq(pio_req, pio_queue);
	
exit:
	_func_exit_;
	return val;
	

}

/*
*	async_irp_read16:
using async_irp to implement syncronous_io_access
*/
static u16 async_irp_read16(_adapter *adapter, u32 addr)
{
	u16	val;
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)  {
		val = _FAIL;
		goto exit;
	}
		
	pio_req->command = 	IO_RD16;
	pio_req->addr = addr;
	
	ioreq_enqueue(pio_req, pio_queue);
	
	_down_sema(&pio_req->sema);
	
	val = (u16 )pio_req->val;
	
	free_ioreq(pio_req, pio_queue);	
exit:	
	_func_exit_;
	return val;
	
}

/*
*async_irp_read32:
using async_irp to implement syncronous_io_access
*/
static u32 async_irp_read32(_adapter *adapter, u32 addr)
{
	u32	val=0x0;
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if ((pio_req == NULL)||(adapter->bSurpriseRemoved)){
		DEBUG_ERR(("async_irp_read32 : pio_req =%p adapter->bSurpriseRemoved=0x%x",pio_req,adapter->bSurpriseRemoved ));
		goto exit;
	}
	pio_req->command = 	IO_RD32;
	pio_req->addr = addr;
	
	ioreq_enqueue(pio_req, pio_queue);
	
	_down_sema(&pio_req->sema);

	val = (u32 )pio_req->val;
	
	free_ioreq(pio_req, pio_queue);
exit:
	_func_exit_;
	return val;
	
}

/*
*	async_irp_write8:
using async_irp to implement syncronous_io_access
*/
static void async_irp_write8(_adapter *adapter, u32 addr, u8 val)
{
	
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	pio_req->command = 	IO_WR8;
	pio_req->addr = addr;
	pio_req->val = val;
	
	ioreq_enqueue(pio_req, pio_queue);
	if(pio_req->addr ==CR9346){
		//DEBUG_ERR(("\n async_irp_write8:address==CR9346   flush io_queue\n"));
		ioreq_flush(adapter,pio_queue);
	}
	_down_sema(&pio_req->sema);
	
	free_ioreq(pio_req, pio_queue);
exit:	
	_func_exit_;
}


/*
*async_irp_write16:
using async_irp to implement syncronous_io_access
*/
static void async_irp_write16(_adapter *adapter, u32 addr, u16 val)
{
	
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	pio_req->command = 	IO_WR16;
	pio_req->addr = addr;
	pio_req->val = val;
	
	ioreq_enqueue(pio_req, pio_queue);
	
	_down_sema(&pio_req->sema);
	
	free_ioreq(pio_req, pio_queue);
exit:
	_func_exit_;
}


/*
*async_irp_write32:
using async_irp to implement syncronous_io_access
*/
static void async_irp_write32(_adapter *adapter, u32 addr, u32 val)
{
	
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if ((pio_req == NULL)||(adapter->bSurpriseRemoved)){
		DEBUG_ERR(("async_irp_write32 : pio_req =%p adapter->bSurpriseRemoved=0x%x",pio_req,adapter->bSurpriseRemoved ));
		goto exit;
	}	
	pio_req->command = 	IO_WR32;
	pio_req->addr = addr;
	pio_req->val = val;
	
	ioreq_enqueue(pio_req, pio_queue);

	_down_sema(&pio_req->sema);
	
	free_ioreq(pio_req, pio_queue);
exit:
	_func_exit_;
}

/*
*async_irp_read_mem:
using async_irp to implement syncronous_io_access
*/
static void async_irp_read_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	u32 r_cnt;
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);

	struct	intf_priv *pintfpriv = pintfhdl->pintfpriv;
	u16	maxlen ;

	struct io_req *pio_req = alloc_ioreq(pio_queue);
	maxlen=(u16)((MAX_PROT_SZ<pintfpriv->max_iosz)?MAX_PROT_SZ:(pintfpriv->max_iosz-12));

	cnt = _RND4(cnt);
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	do{
		if(cnt <=4) cnt=8;//burst mode must be > 4bytes;
		
		r_cnt = (cnt > maxlen) ? maxlen: cnt;
	
		//res = protocol_read(pintfpriv, addr, r_cnt, rmem);

		pio_req->command = 	IO_RD_BURST(r_cnt);
		pio_req->addr = addr;
		pio_req->pbuf = pmem;
	
		ioreq_enqueue(pio_req, pio_queue);
	
		cnt -= r_cnt;
		pmem += r_cnt;
		addr += r_cnt;

		_down_sema(&pio_req->sema);	

	}while(cnt > 0);

	free_ioreq(pio_req, pio_queue);	
exit:
	_func_exit_;
}


/*
*async_irp_write_mem:
using async_irp to implement syncronous_io_access
*/
static void async_irp_write_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	u32 w_cnt;
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);

	struct	intf_priv *pintfpriv = pintfhdl->pintfpriv;
	u16	maxlen ;
	struct io_req *pio_req = alloc_ioreq(pio_queue);

	maxlen=(u16)((MAX_PROT_SZ<pintfpriv->max_iosz)?MAX_PROT_SZ:(pintfpriv->max_iosz-12));
	DEBUG_INFO(("\n async_irp_write_mem:maxlen=%d\n",maxlen));
	cnt = _RND4(cnt);
	DEBUG_INFO(("\n async_irp_write_mem:cnt=%d\n",cnt));
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	do{
		if(adapter->bSurpriseRemoved){
			DEBUG_ERR(("async_irp_write_mem:adapter->bSurpriseRemoved"));
			break;
	
		}
		if(cnt <=4) cnt=8;//burst mode must be > 4bytes;
		
		w_cnt = (cnt > maxlen) ? maxlen: cnt;
		pio_req->command = 	IO_WR_BURST(w_cnt);
		pio_req->addr = addr;
		pio_req->pbuf = pmem;
	
		ioreq_enqueue(pio_req, pio_queue);
	
		cnt -= w_cnt;
		pmem += w_cnt;
		addr += w_cnt;

		_down_sema(&pio_req->sema);	

	}while(cnt > 0);

	free_ioreq(pio_req, pio_queue);	
exit:
	_func_exit_;
		
}


/*
/ read8() : this is a function that use async_irp to implement sync_io_access 
/	(by semaphore)
*/
u8 read8(_adapter *adapter, u32 addr)
{
	_func_enter_;	
	_func_exit_;
	if(adapter->bSurpriseRemoved){
		DEBUG_ERR(("read8: bSurpriseRemoved"));
			return 0xff;
	}
	return async_irp_read8(adapter, addr);
}

u16 read16(_adapter *adapter, u32 addr)
{
	_func_enter_;	
	_func_exit_;
	if(adapter->bSurpriseRemoved){
		DEBUG_ERR(("read16: bSurpriseRemoved"));
			return 0xffff;
	}
	return async_irp_read16(adapter, addr);
}
	
u32 read32(_adapter *adapter, u32 addr)
{
	_func_enter_;	
	if(adapter->bSurpriseRemoved){
		DEBUG_ERR(("read32: bSurpriseRemoved"));
			return 0xffffffff;
	}
	_func_exit_;
	return async_irp_read32(adapter, addr);	
}

void write8(_adapter *adapter, u32 addr, u8 val)
{
	_func_enter_;	
	if(adapter->bSurpriseRemoved){
		DEBUG_ERR(("write8: bSurpriseRemoved"));
			return;
	}
	async_irp_write8(adapter, addr, val);
	_func_exit_;
}
void write16(_adapter *adapter, u32 addr, u16 val)
{
	_func_enter_;	
	if(adapter->bSurpriseRemoved){
		DEBUG_ERR(("write16: bSurpriseRemoved"));
			return;
	}
	async_irp_write16(adapter, addr, val);
	_func_exit_;
}
void write32(_adapter *adapter, u32 addr, u32 val)
{
       struct registry_priv *pregistrypriv = &adapter->registrypriv;
	_func_enter_;	
   	if(adapter->bSurpriseRemoved){
		DEBUG_ERR(("write32: bSurpriseRemoved"));
			return;
	}
    if (pregistrypriv->chip_version <= RTL8711_1stCUT)
    { 
        if( (val == HIMR) || (val == HISR))
        {
             async_irp_write8(adapter, addr, (unsigned char)val);
	      async_irp_write8(adapter, addr+1, (unsigned char)(val>>8));
	      async_irp_write8(adapter, addr+2, (unsigned char)(val>>16));
             async_irp_write8(adapter, addr+3, (unsigned char)(val>>24));			   

        }
        else		      
	   async_irp_write32(adapter, addr, val);		
     }else     
     {
	   async_irp_write32(adapter, addr, val);
     }

	_func_exit_;
}

// only implement sync-based read_mem
void read_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	 async_irp_read_mem(adapter, addr, cnt, pmem);
	 _func_exit_;
}
// only implement sync-based write_mem
void write_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	_func_enter_;	
	if(adapter->bSurpriseRemoved){
		DEBUG_ERR(("write32: bSurpriseRemoved"));
			return;
	}
	async_irp_write_mem(adapter, addr, cnt, pmem);
	_func_exit_;
}

void read_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	void (*_read_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	_func_enter_;	
	_read_port = pintfhdl->io_ops._read_port;
	
	 _read_port(pintfhdl, addr, cnt, pmem);
	_func_exit_;

}


void write_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	
	u32 (*_write_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
	
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	_func_enter_;	
	_write_port = pintfhdl->io_ops._write_port;
	
	_write_port(pintfhdl, addr, cnt, pmem);
	_func_exit_;

}

void write_scsi(_adapter *adapter, u32 cnt, u8 *pmem)
{
	
	u32 (*_write_scsi)(struct intf_hdl *pintfhdl, u32 cnt, u8 *pmem);
	
	struct	io_queue *pio_queue = (struct io_queue *)adapter->pio_queue;
	struct	intf_hdl	*pintfhdl = &(pio_queue->intf);
	_func_enter_;	
	_write_scsi = pintfhdl->io_ops._write_scsi;
	
	_write_scsi(pintfhdl, cnt, pmem);
	_func_exit_;

}

uint async_read8(_adapter *adapter, u32 addr, u8 *pbuff,
	void (*_async_io_callback)(_adapter *padapter, struct io_req *pio_req, u8 *cnxt), u8 *cnxt) 
{

	u8	val;
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL) {
		val =  _FAIL;
		goto exit;
	}
	pio_req->command = 	IO_RD8_ASYNC;
	pio_req->addr = addr;
	pio_req->pbuf = pbuff;
	pio_req->cnxt = cnxt;
	pio_req->_async_io_callback = _async_io_callback;
	
	ioreq_enqueue(pio_req, pio_queue);
	val = _SUCCESS;
exit:
	_func_exit_;
	return val;
}

uint async_read16(_adapter *adapter, u32 addr, u8 *pbuff,
	void (*_async_io_callback)(_adapter *padapter, struct io_req *pio_req, u8 *cnxt), u8 *cnxt)
{
	u16	val;
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL) {
		val =  _FAIL;
		goto exit;
	}
		
	pio_req->command = 	IO_RD16_ASYNC;
	pio_req->addr = addr;
	pio_req->pbuf = pbuff;
	pio_req->cnxt = cnxt;
	pio_req->_async_io_callback = _async_io_callback;
	
	ioreq_enqueue(pio_req, pio_queue);
	val = _SUCCESS;
exit:
	_func_exit_;
	return val;

	
}


uint async_read32(_adapter *adapter, u32 addr, u8 *pbuff,
	void (*_async_io_callback)(_adapter *padapter, struct io_req *pio_req, u8 *cnxt), u8 *cnxt)
{
	uint	val;
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL){
		val =  _FAIL;
		goto exit;
	}
		
	pio_req->command = 	IO_RD32_ASYNC;
	pio_req->addr = addr;
	pio_req->pbuf = pbuff;
	pio_req->cnxt = cnxt;
	pio_req->_async_io_callback = _async_io_callback;
	
	ioreq_enqueue(pio_req, pio_queue);
	val = _SUCCESS;
exit:
	_func_exit_;
	return val;

}


void async_write8(_adapter *adapter, u32 addr, u8 val,
	void (*_async_io_callback)(_adapter *padapter, struct io_req *pio_req, u8 *cnxt), u8 *cnxt)
{
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	pio_req->command = 	IO_WR8_ASYNC;
	pio_req->addr = addr;
	pio_req->val = val;
	pio_req->cnxt = cnxt;
	pio_req->_async_io_callback = _async_io_callback;
	
	ioreq_enqueue(pio_req, pio_queue);
exit:
	_func_exit_;
}



void async_write16(_adapter *adapter, u32 addr, u16 val,
	void (*_async_io_callback)(_adapter *padapter, struct io_req *pio_req, u8 *cnxt), u8 *cnxt)
{
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	_func_enter_;	
	if (pio_req == NULL)
		goto exit;
		
	pio_req->command = 	IO_WR16_ASYNC;
	pio_req->addr = addr;
	pio_req->val = val;
	pio_req->cnxt = cnxt;
	pio_req->_async_io_callback = _async_io_callback;
	
	ioreq_enqueue(pio_req, pio_queue);
exit:
	_func_exit_;
}



void async_write32(_adapter *adapter, u32 addr, u32 val,
	void (*_async_io_callback)(_adapter *padapter, struct io_req *pio_req, u8 *cnxt), u8 *cnxt)
{
	struct io_queue	*pio_queue = (struct io_queue *)adapter->pio_queue;
	struct io_req *pio_req = alloc_ioreq(pio_queue);
	struct registry_priv *pregistrypriv = &adapter->registrypriv;
	
	_func_enter_;	
	if (pio_req == NULL){
		DEBUG_ERR(("\n==========!!!!!!!!!!!!!!!!!!!! async_write32: pio_req == NULL \n"));
		goto exit;
	}
  if (pregistrypriv->chip_version <= RTL8711_1stCUT)
  {			
        if( (val == HIMR) || (val == HISR))
        {
             async_write8(adapter, addr, (u8)val, _async_io_callback,cnxt);
	      async_write8(adapter, addr+1, (u8)(val>>8), _async_io_callback,cnxt);
	      async_write8(adapter, addr+2, (u8)(val>>16), _async_io_callback,cnxt);
             async_write8(adapter, addr+3, (u8)(val>>24), _async_io_callback,cnxt);			    

        }else
        {
		
	pio_req->command = 	IO_WR32_ASYNC;
	pio_req->addr = addr;
	pio_req->val = val;
	pio_req->cnxt = cnxt;
	pio_req->_async_io_callback = _async_io_callback;
	
	ioreq_enqueue(pio_req, pio_queue);
	
        }
    }
     else
     {
          pio_req->command = 	IO_WR32_ASYNC;
	   pio_req->addr = addr;
	   pio_req->val = val;
	   pio_req->cnxt = cnxt;
	   pio_req->_async_io_callback = _async_io_callback;
	
	   ioreq_enqueue(pio_req, pio_queue);	

	}  
	 
exit:
	_func_exit_;	
	return;
}
uint ioreq_flush(_adapter *adapter, struct io_queue *ioqueue)
{
       _irqL	irqL;
//    struct	io_queue *pio_queue = ioqueue;
       struct	io_queue *pio_queue = (struct io_queue*)adapter->pio_queue;
	struct	intf_hdl	*pintf = &(pio_queue->intf);	
	_func_enter_;	
	_enter_critical(&(pio_queue->lock), &irqL);
	
	if( pintf->do_flush == _FALSE)
	        pintf->do_flush = _TRUE;
       async_bus_io(pio_queue);        
                

       _exit_critical(&(pio_queue->lock), &irqL);
	_func_exit_;
	return _TRUE;
}
	

#endif

/*

Must use critical section to protect alloc_ioreq and free_ioreq,
due to the irq level possibilities of the callers.
This is to guarantee the atomic context of the service.

*/
struct io_req *alloc_ioreq(struct io_queue *pio_q)
{
	_irqL	irqL;
	_list	*phead = &pio_q->free_ioreqs;
	struct io_req *preq = NULL;
	_func_enter_;	
	_enter_critical(&pio_q->lock, &irqL);
	 
	if (is_list_empty(phead) == _FALSE)
	{
		preq = LIST_CONTAINOR(get_next(phead), struct io_req, list);
		list_delete(&preq->list);

		memset((u8 *)preq, 0, sizeof(struct io_req));
		_init_listhead(&preq->list);
#ifdef 		USE_ASYNC_IRP
		_init_sema(&preq->sema, 0);
#endif
	}

	_exit_critical(&pio_q->lock, &irqL);
	_func_exit_;
	return preq;
}

/*

	must use critical section to protect alloc_ioreq and free_ioreq,
	due to the irq level possibilities of the callers.
	This is to guarantee the atomic context of the service.

*/
uint free_ioreq(struct io_req *preq, struct io_queue *pio_q)
{
	_irqL	irqL;
	_list	*phead = &pio_q->free_ioreqs;
	//struct io_req *preq = NULL;
	_func_enter_;	
	_enter_critical(&pio_q->lock, &irqL);

	list_insert_tail(&(preq->list), phead);
		 
	_exit_critical(&pio_q->lock, &irqL);
	_func_exit_;
	return _SUCCESS;
}

void query_ioreq_sz(struct io_req *pio_req, u32 *w_sz, u32 *r_sz)
{
	*w_sz = 2;
	*r_sz = 0;
	_func_enter_;	
	if (pio_req->command & _IO_WRITE_)
	{
		if (pio_req->command & _IO_BURST_)
			*w_sz = (*w_sz) + (((pio_req->command) & _IOSZ_MASK_) >> 2);
		else
			*w_sz = (*w_sz) + 1;
	}
	else
	{
	#if 0	
		if ((*r_sz) == 0)
			*r_sz = 1;
	#endif
		
		*r_sz = 2;
		
		if (pio_req->command & _IO_BURST_)
		{
			*r_sz = (*r_sz) + (((pio_req->command) & _IOSZ_MASK_) >> 2);			
		}
		else
		{
			*r_sz = (*r_sz) + 1;
		}
	}
	_func_exit_;
}


/*



*/

static u32 _fillin_iocmd(struct io_req *pio_req, volatile u32 **ppcmdbuf)
{
	u32 w_cnt = 2;
	u32 cmd;
	volatile u32 *pcmdbuf = *ppcmdbuf;
	u8 * pu8cmdbuf = (u8*)pcmdbuf;
	_func_enter_;	
	cmd = ((pio_req->command) &  _IO_CMDMASK_);
	
	if (pio_req->command & _IO_BURST_)
		cmd |= ((pio_req->command) & _IOSZ_MASK_);
	
	*pcmdbuf = cmd;
	
	pcmdbuf++;
	
	*pcmdbuf = pio_req->addr;

	pcmdbuf++;	

	if (pio_req->command & _IO_WRITE_)
	{
		if (pio_req->command & _IO_BURST_)
		{
			pu8cmdbuf = (u8*) pcmdbuf;
			_memcpy(pu8cmdbuf, pio_req->pbuf, ((pio_req->command) & _IOSZ_MASK_));
			
			pcmdbuf += (((pio_req->command) & _IOSZ_MASK_) >> 2);
			
			w_cnt += (((pio_req->command) & _IOSZ_MASK_) >> 2);
		}
		else
		{
			*pcmdbuf = pio_req->val;
			pcmdbuf ++;
			w_cnt ++;
		}
	}
	else
	{
#if 0		
		if (*r_cnt == 0)
			*r_cnt = 1;
		
		*r_cnt = (*r_cnt) + 2;
		
		if (pio_req->command & _IO_BURST_)
		{
			*r_cnt += (((pio_req->command) & _IOSZ_MASK_) >> 2);			
		}
		else
		{
			*r_cnt = (*r_cnt) + 1;
		}
#endif				
		
	}
	
	*ppcmdbuf = pcmdbuf;
	_func_exit_;
	return w_cnt;
}

u32 _ioreq2rwmem(struct io_queue *pio_q)
{

	_list	*phead, *plist;

	struct io_req	*pio_req;
	
	struct intf_hdl *pintf = &(pio_q->intf);
	
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	volatile u32 *pnum_reqs = (volatile u32 *)pintfpriv->io_rwmem;
	volatile u32 *pcmd = (volatile u32 *)pintfpriv->io_rwmem;

	u32 r_sz = 0, all_r_sz = 1,  w_sz = 0, all_w_sz = 1;
      _func_enter_;
	phead = &(pio_q->pending);

	plist = get_next(phead);

	memset((void*)pintfpriv->io_rwmem, 0, pintfpriv->max_iosz);
	
	pcmd++;
	while (1)
	{
		if ((is_list_empty(phead)) == _TRUE)
			break;
		
		pio_req = LIST_CONTAINOR(plist, struct io_req, list);
		
		query_ioreq_sz(pio_req, &w_sz, &r_sz);

		if ((all_r_sz + r_sz) > (pintfpriv->max_iosz >> 2))
			break;
		if ((all_w_sz + w_sz) > (pintfpriv->max_iosz >> 2))
			break;

		list_delete(&(pio_req->list));
		
		 _fillin_iocmd(pio_req, &pcmd);	

		list_insert_tail(&(pio_req->list),&(pio_q->processing));

		plist = get_next(phead);

		all_r_sz += r_sz;
		all_w_sz += w_sz;
		
		*pnum_reqs = *pnum_reqs + 1;
	}

	pintfpriv->io_rsz = all_r_sz;
	pintfpriv->io_wsz = all_w_sz;
	
	if ((all_r_sz > 1 ) || (all_w_sz > 1)) {
		_func_exit_;
		return _TRUE;
	}
	else {
		_func_exit_;
		return _FALSE;
		}
}

uint	alloc_io_queue(_adapter *adapter)
{

	u32	i;
	struct	io_queue	*pio_queue; 
	struct	io_req	*pio_req;
	
	pio_queue = (struct io_queue *)_malloc (sizeof (struct io_queue));
	_func_enter_;
	if (pio_queue == NULL){
		DEBUG_ERR(("\n  alloc_io_queue:pio_queue == NULL !!!!\n"));
		goto alloc_io_queue_fail;	
	}
	_init_listhead(&pio_queue->free_ioreqs);
	_init_listhead(&pio_queue->processing);
	_init_listhead(&pio_queue->pending);
		
	_spinlock_init(&pio_queue->lock);

	pio_queue->pallocated_free_ioreqs_buf = (u8 *)_malloc(NUM_IOREQ *(sizeof (struct io_req)) + 4);

	memset(pio_queue->pallocated_free_ioreqs_buf, 0, (NUM_IOREQ *(sizeof (struct io_req)) + 4));
	
	if ((pio_queue->pallocated_free_ioreqs_buf) == NULL){
		DEBUG_ERR(("\n  alloc_io_queue:(pio_queue->pallocated_free_ioreqs_buf) == NULL!!!!\n"));
		goto alloc_io_queue_fail;
	}
	pio_queue->free_ioreqs_buf = pio_queue->pallocated_free_ioreqs_buf + 4 
			- ((u32 )(pio_queue->pallocated_free_ioreqs_buf)  & 3);
	
	pio_req = (struct io_req *)(pio_queue->free_ioreqs_buf);


	for(i = 0; i < 	NUM_IOREQ; i++)
	{
		_init_listhead(&pio_req->list);
	#ifdef USE_ASYNC_IRP	
		_init_sema(&pio_req->sema, 0);
	#endif
		list_insert_tail(&pio_req->list, &pio_queue->free_ioreqs);
		pio_req++;	
	}		
	
	if ((register_intf_hdl((u8 *)adapter, &(pio_queue->intf))) == _FAIL){
		DEBUG_ERR(("\n  alloc_io_queue:register_intf_hdl == _FAIL !!!!\n"));
		goto alloc_io_queue_fail;
	}
	adapter->pio_queue = pio_queue;
	_func_exit_;
	return _SUCCESS;
	
alloc_io_queue_fail:

	if (pio_queue) {
		if (pio_queue->pallocated_free_ioreqs_buf)
			_mfree(pio_queue->pallocated_free_ioreqs_buf, NUM_IOREQ *(sizeof (struct io_req)) + 4);	
	}

	adapter->pio_queue = NULL;
	_func_exit_;
	return _FAIL;	
	
}


void free_io_queue(_adapter *adapter)
{
	struct	io_queue *pio_queue = (struct io_queue *)(adapter->pio_queue);
	_func_enter_;
	if (pio_queue)
	{
		_mfree(pio_queue->pallocated_free_ioreqs_buf, NUM_IOREQ *(sizeof (struct io_req)) + 4);	
		adapter->pio_queue = NULL;
		unregister_intf_hdl(&pio_queue->intf);
	}
	_func_exit_;
}
