


#ifndef __OSDEP_SERVICE_H_
#define __OSDEP_SERVICE_H_

#include <drv_conf.h>
#include <basic_types.h>

//john
#define SCSI_ENTRY_MEMORY_SIZE (MAX_TXCMD*4+8)


#define _SUCCESS	1
#define _FAIL		0

#define _TRUE		1
#define _FALSE		0


#ifdef PLATFORM_LINUX

#include <osdep_linux_service.h>

#endif

#ifdef PLATFORM_OS_XP

#include <osdep_xp_service.h>

#endif

#ifdef PLATFORM_OS_WINCE

#include <osdep_ce_service.h>

#endif


#endif
