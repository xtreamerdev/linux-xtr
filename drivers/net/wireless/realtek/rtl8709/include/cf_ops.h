

#ifndef __CFIO_OPS_H_
#define __CFIO_OPS_H_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>




extern u8 cf_read8(struct intf_hdl *intf_hdl, u32 addr);
extern u16 cf_read16(struct intf_hdl *intf_hdl, u32 addr);
extern u32 cf_read32(struct intf_hdl *intf_hdl, u32 addr);



extern void cf_set_intf_option(u32 *poption);

extern void cf_set_intf_funs(struct intf_hdl *pintf_hdl);

extern uint cf_init_intf_priv(struct intf_priv *pintfpriv);

extern void cf_unload_intf_priv(struct intf_priv *pintfpriv);

extern void cf_intf_hdl_init(u8 *priv);

extern void cf_intf_hdl_unload(u8 *priv);

extern void cf_intf_hdl_open(u8 *priv);

extern void cf_intf_hdl_close(u8 *priv);

extern void cf_set_intf_ops(struct _io_ops *pops);
	
//extern void cf_set_intf_callbacks(struct _io_callbacks *pcallbacks);

#endif
