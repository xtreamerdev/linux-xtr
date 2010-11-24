
#ifndef _WLAN_TASK_H_
#define _WLAN_TASK_H_

#ifndef __ASSEMBLY__

#include <wlan.h>
#if 0
static __inline__ void setfimr(unsigned int bitmask)
{
	sys_mib.mac_ctrl.fimr |= bitmask;

	rtl_outl(FIMR, sys_mib.mac_ctrl.fimr);

}

static __inline__ void clrfimr(unsigned int bitmask)
{
	sys_mib.mac_ctrl.fimr &= (~bitmask);

	rtl_outl(FIMR, sys_mib.mac_ctrl.fimr);

}

static __inline__ void setsysimr(unsigned int bitmask)
{
	sys_mib.mac_ctrl.sysimr |= bitmask;

	rtl_outw(SYSIMR, sys_mib.mac_ctrl.sysimr);

}

static __inline__ void clsysimr(unsigned int bitmask)
{
	sys_mib.mac_ctrl.sysimr &= (~bitmask);

	rtl_outw(SYSIMR, sys_mib.mac_ctrl.sysimr);

}


static __inline__ void setrtimr(unsigned int bitmask)
{
	
	sys_mib.mac_ctrl.rtimr |= bitmask;

	rtl_outw(RTIMR, sys_mib.mac_ctrl.rtimr);
	
}

static __inline__ void clrtimr(unsigned int bitmask)
{
	
	sys_mib.mac_ctrl.rtimr &= (~bitmask);

	rtl_outw(RTIMR, sys_mib.mac_ctrl.rtimr);
	
}
#endif

#ifndef _WLAN_TASK_C_

extern int wlan_tx (_adapter *padapter, struct txirp *irp);

//sunghau@0319: the parameter is never used
//extern void txhw_fire (int);
extern void txhw_fire (void);

extern void wlan_swinit(void);

extern void wlan_hwinit(void);

extern void wlan_lbkinit(void);

#endif

#endif

#endif // _WLAN_TASK_H_





