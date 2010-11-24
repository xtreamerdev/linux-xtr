#ifndef __RTL871X_PWRCTRL_H_
#define __RTL871X_PWRCTRL_H_

#include <drv_conf.h>
#include <osdep_service.h>		
#include <drv_types.h>

#define PS_MODE_ACTIVE		0
#define PS_MODE_MIN		1
#define PS_MODE_MAX        	2
#define PS_MODE_VOIP       	3
#define PS_MODE_UAPSD      	4
#define PS_MODE_UAPSD_WMM   5
#define PS_MODE_IBSS       	6
#define PS_MODE_WWLAN      	7

#define FW_PWR0	0	
#define FW_PWR1 	1
#define FW_PWR2 	2
#define FW_PWR3 	3


#define HW_PWR0	7	
#define HW_PWR1 	6
#define HW_PWR2 	2
#define HW_PWR3	0
#define HW_PWR4	8

#define FW_PWRMSK	0x7


#define XMIT_ALIVE	BIT(0)
#define RECV_ALIVE	BIT(1)
#define CMD_ALIVE	BIT(2)
#define EVT_ALIVE	BIT(3)


/*! \enum _PS_MODE
	\brief Power save mode configured. 
*/
typedef	enum _ps_mode {
	eActive,	// Active/Continuous access.
	eMaxPs,		// Max power save mode.
	eFastPs		// Fast power save mode.
} ps_mode;


	typedef _sema _pwrlock;




static void __inline _init_pwrlock(_pwrlock *plock)
{

#if defined PLATFORM_LINUX

	_init_sema(plock, 1);
        //#error "patch _init_pwrlock \n"

#elif defined PLATFORM_WINDOWS


	_init_sema(plock, 1);



#endif

}


static void __inline _enter_pwrlock(_pwrlock *plock)
{


#if defined __KERNEL__

	//#error "patch _enter_pwrlock \n"
       _down_sema(plock);

#elif defined PLATFORM_WINDOWS



	_down_sema(plock);



#endif


}


static void __inline _exit_pwrlock(_pwrlock *plock)
{

#if defined PLATFORM_LINUX

	//#error "patch _exit_pwrlock \n"
       _up_sema(plock);

#elif defined PLATFORM_WINDOWS



	_up_sema(plock);


#endif


}

/*
Will be called when reading from CPWM Register...
*/

static __inline u8 hwcpwm2swcpwm(u8 hwcpwm)
{

	u8 swcpwm;
	
	switch (hwcpwm & FW_PWRMSK)
	{
		case HW_PWR0:
			swcpwm = FW_PWR0;
			break;
		
		case HW_PWR1:
			swcpwm = FW_PWR1;
			break;

		case HW_PWR2:
			swcpwm = FW_PWR2;
			break;

		case HW_PWR3:
			swcpwm = FW_PWR3;
			break;

		default:
			swcpwm = FW_PWR0;
			break;
			
	}

	return swcpwm;

}



static __inline u8 swrpwm2hwrpwm(u8 swrpwm)
{
	u8 hwrpwm;
	
	switch (swrpwm)
	{
		case FW_PWR0:
			hwrpwm = HW_PWR0;
			break;
		
		case FW_PWR1:
			hwrpwm = HW_PWR1;
			break;

		case FW_PWR2:
			hwrpwm = HW_PWR2;
			break;

		case FW_PWR3:
			hwrpwm = HW_PWR3;
			break;

		default:
			hwrpwm = HW_PWR0;
			break;
			
	}

	return hwrpwm;

}


struct	pwrctrl_priv {
	_pwrlock	lock;
	volatile u8 cur_rpwm; // requested power state for fw
	volatile u8 cpwm; // fw current power state. updated when 1. read from HCPWM 2. driver lowers power level
	volatile u8 tog; // toggling
	volatile u8 tgt_rpwm; // wanted power state 
	uint pwr_mode;
	uint smart_ps;
	uint alives;
	uint ImrContent;	// used to store original imr.

	_sema	pnp_pwr_mgnt_sema;
	_lock	pnp_pwr_mgnt_lock;
	s32	pnp_current_pwr_state;
	u8	pnp_bstop_trx;
	u8	pnp_wwirp_pending;
#ifdef PLATFORM_WINDOWS
#ifdef CONFIG_USB_HCI
	PIRP pnp_wwirp;
	NDIS_EVENT pnp_wwirp_complete_evt;
#endif
#endif
};
extern uint update_tgt_rpwm(_adapter *adapter, u8 state);

extern uint set_fw_pwr(_adapter *adapter, u8 state);
extern void down_fw_pwr(_adapter *adapter);
extern sint register_task_alive(_adapter *adapter, uint tag);

extern void init_pwrctrl_priv(_adapter *adapter);
//extern int is_task_alive(_adapter *adapter, unsigned int tag);
//extern int set_fw_pwrstate(_adapter *adapter, unsigned char state);
//extern int set_fw_tgt_pwrstate(_adapter *adapter, unsigned char state);
extern u8 get_cpwm(_adapter *padapter);
extern void chg_fw_pwr(_adapter *padapter);
extern sint register_tx_alive(_adapter *padapter);
extern void unregister_tx_alive(_adapter *padapter);
extern sint register_rx_alive(_adapter *padapter);
extern void unregister_rx_alive(_adapter *padapter);
extern sint register_cmd_alive(_adapter *padapter);
extern void unregister_cmd_alive(_adapter *padapter);
extern sint register_evt_alive(_adapter *padapter);
extern void unregister_evt_alive(_adapter *padapter);
extern void cpwm_int_hdl(_adapter *padapter);
extern void ps_mode_init(_adapter * padapter, uint ps_mode, uint smart_ps);
#endif  //__RTL871X_PWRCTRL_H_
