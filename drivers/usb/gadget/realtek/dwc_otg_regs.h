#ifndef __DWC_OTG_REGS_H__
#define __DWC_OTG_REGS_H__

#include <asm/byteorder.h>

#if defined(__LITTLE_ENDIAN)
#include "dwc_otg_regs_little.h"
#elif defined(__BIG_ENDIAN)
#include "dwc_otg_reg_big.h"
#else
#error "DWC OTG driver cannot get CPU endian info."
#endif

#endif // __DWC_OTG_REGS_H__
