#ifndef __SDIO_OPS_H_
#define __SDIO_OPS_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>

#ifdef PLATFORM_LINUX

#endif



#ifdef PLATFORM_WINDOWS
struct async_context
{
	PMDL pmdl;
	PSDBUS_REQUEST_PACKET sdrp;
	unsigned char* r_buf;
	unsigned char* padapter;
};
#endif



extern void sdio_set_intf_option(u32 *poption);

extern void sdio_set_intf_funs(struct intf_hdl *pintf_hdl);

extern uint sdio_init_intf_priv(struct intf_priv *pintfpriv);

extern void sdio_unload_intf_priv(struct intf_priv *pintfpriv);

extern void sdio_intf_hdl_init(u8 *priv);

extern void sdio_intf_hdl_unload(u8 *priv);

extern void sdio_intf_hdl_open(u8 *priv);

extern void sdio_intf_hdl_close(u8 *priv);

extern void sdio_set_intf_ops(struct _io_ops *pops);
	
//extern void sdio_set_intf_callbacks(struct _io_callbacks *pcallbacks);



#endif

