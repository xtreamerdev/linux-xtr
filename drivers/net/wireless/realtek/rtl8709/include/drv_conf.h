
#ifndef __DRV_CONF_H__
#define __DRV_CONF_H__
#include "autoconf.h"

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#if defined(CONFIG_RTL8192U)

typedef enum _OT_RST{
	OPT_SYSTEM_RESET = 0,
	OPT_FIRMWARE_RESET = 1,
}FIRMWARE_OPERATION_STATE_E;

	#define BOOLEAN	u8


	#define PlatformEFIORead1Byte read8
	#define PlatformEFIORead2Byte read16
	#define PlatformEFIORead4Byte read32

	#define PlatformEFIOWrite1Byte write8
	#define PlatformEFIOWrite2Byte write16
	#define PlatformEFIOWrite4Byte write32
#endif

#endif // __DRV_CONF_H__

