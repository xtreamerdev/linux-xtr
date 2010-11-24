/******************************************************************************
* rtl871x_pwrctrl.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTL871X_PWRCTRL_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>

#if 0 //def CONFIG_PWRCTRL
static sint set_fw_pwrstate(_adapter *adapter, u8 state);
static _inline void lower_fw_pwrstate(_adapter *adapter);
static sint	 register_task_alive(_adapter *adapter, uint tag);

static void update_fw_pwrstate_bh(_adapter *padapter);
#endif







void	init_pwrctrl_priv(_adapter *adapter)
{

#ifdef PLATFORM_WINDOWS
	struct pwrctrl_priv *pwrctrlpriv = &adapter->pwrctrlpriv;

_func_enter_;	
	memset((unsigned char *)pwrctrlpriv, 0, sizeof(struct pwrctrl_priv));
	_spinlock_init(&(pwrctrlpriv->pnp_pwr_mgnt_lock));
	_init_sema(&(pwrctrlpriv->pnp_pwr_mgnt_sema),0);
	pwrctrlpriv->pnp_wwirp_pending=_FALSE;
	pwrctrlpriv->pnp_current_pwr_state=NdisDeviceStateD0;
	#ifdef CONFIG_USB_HCI
	NdisInitializeEvent( &(pwrctrlpriv->pnp_wwirp_complete_evt));
	pwrctrlpriv->pnp_wwirp=NULL;
	#endif
#ifdef CONFIG_PWRCTRL
	_init_pwrlock(&pwrctrlpriv->lock);

	pwrctrlpriv->cpwm = FW_PWR0;

	pwrctrlpriv->pwr_mode = PWR_CAM;

	pwrctrlpriv->smart_ps = 1;

	pwrctrlpriv->tog = 0x80;
#endif	
_func_exit_;

#endif
}



void ps_mode_init(_adapter * padapter, uint ps_mode, uint smart_ps)
{
#ifdef CONFIG_PWRCTRL
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;
_func_enter_;
	DEBUG_INFO(("========= Power Mode is :%d\n", ps_mode));

	pwrpriv->pwr_mode = ps_mode;
	pwrpriv->smart_ps = smart_ps;
	if(ps_mode > PS_MODE_WWLAN) {
		DEBUG_ERR(("ps_mode:%d error\n", ps_mode));
		goto exit;
	}
	
	/* TODO: Perry
		We should check whether the ps_mode(registry) equals to firmware definitions.
	*/
	if(!setpwrmode_cmd(padapter,  ps_mode)) {
		DEBUG_ERR(("Can't invoke FW to change its power settings.\n"));
	}
exit:	
_func_exit_;	
#endif	
}


u8 get_cpwm(_adapter *adapter)
{


#ifdef CONFIG_PWRCTRL
	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;
_func_enter_;
	//DEBUG_ERR(("get_cpwm: %x", pwrctrl->cpwm));
	return (pwrctrl->cpwm);
#else
	return FW_PWR0;
#endif	

_func_exit_;

}

/*
Caller:ISR handler...

This will be called when CPWM interrupt is up.

using to update cpwn of drv; and drv willl make a decision to up or down pwr level
*/
void cpwm_int_hdl(_adapter *adapter)
{
#ifdef CONFIG_PWRCTRL
	struct pwrctrl_priv *pwrctrl = &(adapter->pwrctrlpriv);
_func_enter_;
	_enter_pwrlock(&adapter->pwrctrlpriv.lock);
	chg_fw_pwr(adapter);
	_exit_pwrlock(&adapter->pwrctrlpriv.lock);
#endif
_func_exit_;

}



#ifdef CONFIG_PWRCTRL

//static 
sint	 register_task_alive(_adapter *padapter, uint tag)
{
	s32	res;
	struct pwrctrl_priv *pwrctrl = &padapter->pwrctrlpriv;
_func_enter_;
	if (!(pwrctrl->alives & tag)) {	
		pwrctrl->alives |= tag;
		res=_SUCCESS;
	}
	else 
		res= _FAIL;
_func_exit_;	
	return res;
}


/*
	Calling Context:

	Could be in Dispatch or Passive Level...
*/
//static
uint set_fw_pwr (_adapter *adapter, u8 state)
{

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

#if defined CONFIG_USB_HCI

	async_write8(adapter, HRPWM, state, NULL);


#elif defined CONFIG_SDIO_HCI

#ifdef USE_SYNC_IRP

	DEBUG_ERR(("set_fw_pwr =>>WT HRPWM to %x", state));
	write8(adapter, HRPWM, (swrpwm2hwrpwm(state) + pwrctrl->tog));

#else

	async_write8(adapter, HRPWM, swrpwm2hwrpwm(state), NULL);

#endif

#elif defined CONFIG_CFIO_HCI

	write8(adapter, HRPWM, swrpwm2hwrpwm(state) + pwrctrl->tog);

#endif

	pwrctrl->tog += 0x80;
	pwrctrl->cur_rpwm = state;


	//yt: have to do more check for this statement
	//pwrctrl->cpwm = state;
_func_exit_;	
	return _SUCCESS;
	
}





uint up_fw_pwr (_adapter *adapter, u8 state)
{
	uint a;
_func_enter_;	
	a = set_fw_pwr(adapter, state);
_func_exit_;
	return a;

}


/* This function must be protected */ 
static void down_fw_pwr (_adapter *adapter)
{
	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;
_func_enter_;	

	DEBUG_ERR((" from %d to %d; alives:%x\n", pwrctrl->cpwm, pwrctrl->tgt_rpwm, pwrctrl->alives));

	if(pwrctrl->alives) 
	{
		DEBUG_ERR(("=== alives != 0.. Error %x\n", pwrctrl->alives));
		return;
	}
	
	/* Perry: 
		Driver wanted to FW_PWR3 state must to FW_PWR2 first.
		In update_fw_pwrstate_bh, driver will change to FW_PWR3
	*/
	if(pwrctrl->cpwm <= FW_PWR1)
	{
		/* CPWM = P0 or P1 */
		if(pwrctrl->tgt_rpwm >= FW_PWR2)
		{ 
			/* TGT_RPWM = P2 or P3 */

			set_fw_pwr(adapter, FW_PWR2);
			/* Perry: 
				We must udpate CPWM here to avoid Other functions active power without notifing again.
				For example, 
					1. RegCmd		FW_STATE = P0			CPWM=P0
					2. UnRegCmd		FW_STATE = P0 -> P2 		CPWM=P0 and "issue RPWM=P2"
					3. RegTx			FW_STATE = P0			CPWM=P0
					4. Update_CPWM	FW_STATE = P2			CPWM=P2
				It's clear that xmit_thread will be executed in P2 state. This is incorrent!
			*/

			pwrctrl->cpwm = FW_PWR2;
		}	
	}
	else if (pwrctrl->cpwm < pwrctrl->tgt_rpwm)
	{
		/* CPWM = P2 or P3 */

		if((pwrctrl->tgt_rpwm >= FW_PWR3) && (adapter->ImrContent != _CPWM))
		{
			DEBUG_ERR((" tgt_pwr = P3 state & ImrContent != _CPWM \n"));
			// Perry: To P3 state, driver must make sure cpwm = P2 state
			if(hwcpwm2swcpwm(read8(adapter, HCPWM)) >= FW_PWR2)
			{ 
				//P2 || P3
				/* Perry: To fix 32K HISR &  HIMR */
				// Backup ImrContent
				
				pwrctrl->ImrContent = adapter->ImrContent;
				write32(adapter, HIMR, _CPWM );
				set_fw_pwr(adapter, pwrctrl->tgt_rpwm);
				pwrctrl->cpwm = pwrctrl->tgt_rpwm;
				
				adapter->ImrContent = _CPWM;
				DEBUG_ERR(("Change to P3 state and set the HIMR to :%x cpwm:%d\n", read32(adapter, HIMR), pwrctrl->cpwm));
				
			}
		} 
		else 
		{
			set_fw_pwr(adapter, pwrctrl->tgt_rpwm);
			 pwrctrl->cpwm = pwrctrl->tgt_rpwm;
		}
	}
_func_exit_;

}


static void chg_fw_pwr_bh(_adapter *adapter)
{

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;
	struct xmit_priv *pxmitpriv = &adapter->xmitpriv;
	struct cmd_priv *pcmdpriv = &adapter->cmdpriv;

_func_enter_;

	DEBUG_ERR(("===tgt=%d; cpwm=%d ;rpwm=%d; alives = %x; ", pwrctrl->tgt_rpwm, pwrctrl->cpwm, pwrctrl->cur_rpwm,pwrctrl->alives));

	if((pwrctrl->tgt_rpwm <= FW_PWR1) && (pwrctrl->cpwm <= FW_PWR1))
		_up_sema(&pxmitpriv->xmit_sema);

	if((pwrctrl->tgt_rpwm <= FW_PWR2) && (pwrctrl->cpwm <= FW_PWR2))
		_up_sema(&pcmdpriv->cmd_queue_sema);


	if (pwrctrl->cpwm == pwrctrl->tgt_rpwm)
	{
		DEBUG_ERR(("chg_fw_pwr_bh return due to cpwm==tgt_rpwm"));
		return;
	}
	
	else if (pwrctrl->cpwm < pwrctrl->tgt_rpwm) 
	{
		/* Perry: To P3 state, we must to P2 state first. */
		//set_fw_pwrstate(adapter, (pwrctrl->cpwm + 1));	
		down_fw_pwr(adapter);
	}
	else 
	{
		/* Perry : Fw allows driver to raise power level directly. */
		up_fw_pwr(adapter, pwrctrl->tgt_rpwm);
		//set_fw_pwrstate(adapter, (pwrctrl->cpwm - 1));
	}

_func_exit_;

}




/*
Calling Context:   ISR or Thread
*/
void chg_fw_pwr(_adapter *adapter)
{
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

	DEBUG_ERR((" *@*@chg_fw_pwr drv_cpwm:%x*@*@", adapter->pwrctrlpriv.cpwm));

	pwrctrl->cpwm = hwcpwm2swcpwm(read8(adapter, HCPWM));
	chg_fw_pwr_bh(adapter);

_func_exit_;

//4 To Modify the following; under callback, we can't lock sema or spinlock ??  (yt)
/* 	
#if defined CONFIG_USB_HCI
	// TODO: Perry: fix it
	async_read8(adapter, HCPWM, &chg_fw_pwr(_adapter *adapter, struct io_req *pio_req, unsigned char *cnxt));
	return ;
#elif defined CONFIG_SDIO_HCI



#ifdef USE_SYNC_IRP
	//DEBUG_ERR("update_fw_pwrstate: RD HCPWM: %x" , );
	pwrctrl->cpwm = hwcpwm2swcpwm(read8(adapter, HCPWM));
	chg_fw_pwr_bh(adapter);
#else
	// TODO: Perry: fix it
	async_read8(adapter, HCPWM, &chg_fw_pwr(_adapter *adapter, struct io_req *pio_req, unsigned char *cnxt));
	return;
#endif

#elif defined CONFIG_CFIO_HCI
	pwrctrl->cpwm = hwcpwm2swcpwm(read8(adapter, HCPWM));	
	chg_fw_pwr_bh(adapter);		
#endif
*/




#endif /*CONFIG_PWRCTRL*/
}



#if  0//defined (CONFIG_USB_HCI) || ( (defined (CONFIG_SDIO_HCI)) && (defined (USE_ASYNC_IRP)))
static void chg_fw_pwr (_adapter *adapter, struct io_req *pio_req, unsigned char *cnxt)
{
	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;
	pwrctrl->cpwm = hwcpwm2swcpwm((unsigned char )(pio_req->val));

	_enter_pwrlock(&pwrctrl->lock);
	chg_fw_pwr_bh(adapter);
	_exit_pwrlock(&pwrctrl->lock);

}
#endif



//static
uint update_tgt_rpwm(_adapter *adapter, u8 state)
{

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;
_func_enter_;
	
	if (pwrctrl->alives) {
		if (pwrctrl->tgt_rpwm > state)
			pwrctrl->tgt_rpwm = state;	
	}
	else
		pwrctrl->tgt_rpwm = state;

_func_exit_;

	return _SUCCESS;
}



/*
the caller of up_tgt_pwrstate must lock pwr_lock
*/
static uint  up_tgt_pwr (_adapter *adapter )
{
	u8  cpwm;
	u8 tgt_pwrstate = 0;
	uint status = _SUCCESS;
	struct pwrctrl_priv *pwrpriv = &adapter->pwrctrlpriv;
	sint	traffic_busy = adapter->mlmepriv.sitesurveyctrl.traffic_busy;

_func_enter_;

	if (pwrpriv->pwr_mode > PWR_CAM)
	{
		if(check_fwstate(&adapter->mlmepriv, _FW_UNDER_LINKING) || !check_fwstate(&adapter->mlmepriv, _FW_LINKED)) 
		{
			tgt_pwrstate = FW_PWR0; 
		} 
		else if(pwrpriv->smart_ps == _TRUE && traffic_busy) 
		{
			tgt_pwrstate = FW_PWR0;
		} 
		else
		{
			tgt_pwrstate = FW_PWR1;
			//*tgt_pwrstate = FW_PWR0; // Perry: for test
		}
	} 
	else 
	{
		tgt_pwrstate = FW_PWR0;
	}
	
	update_tgt_rpwm(adapter,tgt_pwrstate);
	//update_drv_cpwm(adapter);
	cpwm = get_cpwm(adapter);

	//DEBUG_ERR((" cpwm:%d ; tgt_rpwm:%d\n", cpwm, pwrpriv->tgt_rpwm));
	
	if (cpwm > pwrpriv->tgt_rpwm)
	{
		up_fw_pwr(adapter, pwrpriv->tgt_rpwm);
		status =  _FAIL;
	}

_func_exit_;

	return status;

}



/*
the caller of up_tgt_pwrstate must lock pwr_lock
*/

static void down_tgt_pwr (_adapter *adapter )
{
	u8  cpwm;
	u8 tgt_pwrstate = 0;
	struct pwrctrl_priv *pwrpriv = &adapter->pwrctrlpriv;
	sint	traffic_busy = adapter->mlmepriv.sitesurveyctrl.traffic_busy;

_func_enter_;

	if (pwrpriv->pwr_mode != PWR_CAM)
	{
		if(check_fwstate(&adapter->mlmepriv, _FW_UNDER_LINKING) || !check_fwstate(&adapter->mlmepriv, _FW_LINKED))
		{
			 tgt_pwrstate = FW_PWR0;
		} 
		else if ((pwrpriv->smart_ps == 1) && (traffic_busy == _TRUE)) 
		{
			 tgt_pwrstate = FW_PWR0;
		} 
		else 
		{			

#if defined CONFIG_SDIO_HCI || defined CONFIG_CFIO_HCI
			 tgt_pwrstate = FW_PWR3;
#elif defined CONFIG_USB_HCI
			 tgt_pwrstate = FW_PWR2;
#endif
		}
	}
	else
		 tgt_pwrstate = FW_PWR0;


	//update_drv_cpwm(adapter);
	cpwm = get_cpwm(adapter);

	if ( cpwm < tgt_pwrstate) 
	{
		update_tgt_rpwm(adapter, tgt_pwrstate);

		if (pwrpriv->alives == 0)
			down_fw_pwr(adapter);
		
	} else
	{
		//DbgPrint("cpwm(%d) > tgt_pwrstate(%d)\n", pwrctrl->cpwm, tgt_pwrstate);
	}

	DEBUG_ERR(("*@*@ down_tgt_pwr to %x*@*@", tgt_pwrstate));

_func_exit_;

}


#endif



/*
Caller: xmit_thread

Check if the fw_pwrstate is okay for xmit.
If not (cpwm is less than P1 state), then the sub-routine
will raise the cpwm to be greater than or equal to P1. 

Calling Context: Passive

Return Value:

_SUCCESS: xmit_thread can write fifo/txcmd afterwards.
_FAIL: xmit_thread can not do anything.
*/
sint register_tx_alive(_adapter *adapter)
{
	uint res = _SUCCESS;
	
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	register_task_alive(adapter, XMIT_ALIVE);

	res = up_tgt_pwr(adapter);

	_exit_pwrlock(&pwrctrl->lock);
	
_func_exit_;

#endif	/* CONFIG_PWRCTRL */

	return res;	

}

/*
Caller: cmd_thread

Check if the fw_pwrstate is okay for issuing cmd.
If not (cpwm should be is less than P2 state), then the sub-routine
will raise the cpwm to be greater than or equal to P2. 

Calling Context: Passive

Return Value:

_SUCCESS: cmd_thread can issue cmds to firmware afterwards.
_FAIL: cmd_thread can not do anything.
*/
sint register_cmd_alive(_adapter *adapter)
{
	uint res = _SUCCESS;
	
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	register_task_alive(adapter, CMD_ALIVE);

	/* Perry:
		We should stay at FW_P0 or FW_P1 because some H2C commands might issue frames.
	*/
	
	res = up_tgt_pwr(adapter);

	_exit_pwrlock(&pwrctrl->lock);
_func_exit_;
#endif

	return res;
}


/*
Caller: rx_isr

Calling Context: Dispatch/ISR

Return Value:

*/
sint register_rx_alive(_adapter *adapter)
{

#ifdef CONFIG_PWRCTRL

	u8 tgt_pwrstate, cpwm;

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	// Perry: we should change CPWM here.
	if(pwrctrl->cpwm > FW_PWR1)
		pwrctrl->cpwm = FW_PWR1;
	
	register_task_alive(adapter, RECV_ALIVE);

	DEBUG_ERR((" cpwm:%d\n", pwrctrl->cpwm));

	_exit_pwrlock(&pwrctrl->lock);
_func_exit_;
	
#endif /*CONFIG_PWRCTRL*/

	return _SUCCESS;
}


/*
Caller: evt_isr or evt_thread

Calling Context: Dispatch/ISR or Passive

Return Value:
*/
sint register_evt_alive(_adapter *adapter)
{

#ifdef CONFIG_PWRCTRL

	u8 tgt_pwrstate, cpwm;

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	// Perry: we should change CPWM here.
	if(pwrctrl->cpwm > FW_PWR2)
		pwrctrl->cpwm = FW_PWR2;
	register_task_alive(adapter, EVT_ALIVE);

	DEBUG_ERR(("cpwm:%d\n", pwrctrl->cpwm));

	_exit_pwrlock(&pwrctrl->lock);
_func_exit_;

#endif /*CONFIG_PWRCTRL*/

	return _SUCCESS;
}

/*
Caller: ISR

If ISR's txdone,
No more pkts for TX,
Then driver shall call this fun. to power down firmware again.
*/

void unregister_tx_alive(_adapter *adapter)
{
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	if (pwrctrl->alives & XMIT_ALIVE) 
	{		
		pwrctrl->alives ^= XMIT_ALIVE;

		DEBUG_ERR(("cpwm:%d ; alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));
		down_tgt_pwr(adapter );
	}
	
	_exit_pwrlock(&pwrctrl->lock);
_func_exit_;

#endif /*CONFIG_PWRCTRL*/
}

/*
Caller: ISR

If ISR's txdone,
No more pkts for TX,
Then driver shall call this fun. to power down firmware again.
*/

void unregister_cmd_alive(_adapter *adapter)
{
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	if (pwrctrl->alives & CMD_ALIVE)
	{
		pwrctrl->alives ^= CMD_ALIVE;

	 	DEBUG_ERR(("cpwm:%d ; alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));
		down_tgt_pwr(adapter );
	}	
	_exit_pwrlock(&pwrctrl->lock);
_func_exit_;

#endif /*CONFIG_PWRCTRL*/
}


/*

Caller: ISR

*/

void unregister_rx_alive(_adapter *adapter)
{
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	if (pwrctrl->alives & RECV_ALIVE) 
	{
		pwrctrl->alives ^= RECV_ALIVE;

		DEBUG_ERR(("cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));
		down_tgt_pwr(adapter );
	}
	
	_exit_pwrlock(&pwrctrl->lock);
_func_exit_;

#endif
}


void unregister_evt_alive(_adapter *adapter)
{
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &adapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	if (pwrctrl->alives & EVT_ALIVE)
	{
		pwrctrl->alives ^= EVT_ALIVE;

		DEBUG_ERR(("cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));
		down_tgt_pwr(adapter );			
	}
	
	_exit_pwrlock(&pwrctrl->lock);
_func_exit_;

#endif /*CONFIG_PWRCTRL*/
}



