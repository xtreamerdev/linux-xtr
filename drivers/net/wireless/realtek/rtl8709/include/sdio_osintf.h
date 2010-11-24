#ifndef __SDIO_OSINTF_H
#define __SDIO_OSINTF_H


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#ifdef PLATFORM_OS_XP
extern NDIS_STATUS sd_dvobj_init(_adapter * adapter);
extern NTSTATUS sd_get_func_num(_adapter *padapter);
#endif

#ifdef PLATFORM_OS_LINUX
extern unsigned int sd_dvobj_init(_adapter * adapter);
#endif

extern void sd_dvobj_deinit(_adapter * adapter);
extern void sd_async_int_hdl(IN _adapter *padapter, IN u32 InterruptType);

extern void sd_sync_int_hdl (_adapter *padapter, u32 InterruptType);




#endif

