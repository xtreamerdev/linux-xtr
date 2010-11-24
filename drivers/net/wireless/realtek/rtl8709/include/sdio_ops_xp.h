#ifndef _SDIO_OPS_XP_H_
#define _SDIO_OPS_XP_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>


#ifdef PLATFORM_OS_XP


extern u8 sdbus_cmd52r_xp(struct intf_priv *pintfpriv, u32 addr);


extern void sdbus_cmd52w_xp(struct intf_priv *pintfpriv, u32 addr,u8 val8);

extern uint sdbus_read_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt);

uint sdbus_read_blocks_to_membuf_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);

extern uint sdbus_read_bytes_to_membuf_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);

extern uint sdbus_write_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt);

extern uint sdbus_write_blocks_from_membuf_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);

extern uint sdbus_write_bytes_from_membuf_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);

#endif

#endif

