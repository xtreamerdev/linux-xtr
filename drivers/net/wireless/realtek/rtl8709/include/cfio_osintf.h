
#ifndef __CFIO_OSINTF_H
#define __CFIO_OSINTF_H


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

extern NDIS_STATUS cf_dvobj_init(_adapter * adapter);
extern void cf_dvobj_deinit(_adapter * adapter);


#endif

