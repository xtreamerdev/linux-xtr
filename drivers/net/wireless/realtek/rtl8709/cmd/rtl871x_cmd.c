/******************************************************************************
* rtl871x_cmd.c                                                                                                                                 *
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
#define _RTL871X_CMD_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <rtl8711_spec.h>
#include <cmd_osdep.h>
#include <mlme_osdep.h>
#include <rtl8711_byteorder.h>

#ifdef PLATFORM_LINUX

#include <linux/compiler.h>
//#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/smp_lock.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/usb.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
#include <linux/usb_ch9.h>
#else
#include <linux/usb/ch9.h>
#endif
#include <linux/circ_buf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#include <asm/semaphore.h>
#include <linux/rtnetlink.h>


#endif

#ifdef PLATFORM_WINDOWS

#include <wlan_bssdef.h>

#endif


/*
Caller and the cmd_thread can protect cmd_q by spin_lock.
No irqsave is necessary.
*/


//extern int cmd_thread (void *data);
//extern int cmd_enqueue(_queue *cmdq,struct cmd_obj	*pcmd);



static __inline _OS_STATUS res_to_status(sint res)
{


#if defined (PLATFORM_LINUX) || defined (PLATFORM_MPIXEL)
	return res;
#endif

#ifdef PLATFORM_WINDOWS

	if (res == _SUCCESS)
		return NDIS_STATUS_SUCCESS;
	else
		return NDIS_STATUS_FAILURE;

#endif	
	
}




u32	init_cmd_priv(struct cmd_priv *pcmdpriv)
{
	sint res=_SUCCESS;
	
_func_enter_;		
	_init_sema(&(pcmdpriv->cmd_queue_sema), 0);
	_init_sema(&(pcmdpriv->cmd_done_sema), 0);
	_init_sema(&(pcmdpriv->terminate_cmdthread_sema), 0);
	
	
	_init_queue(&(pcmdpriv->cmd_queue));
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	_init_sema(&(pcmdpriv->cmd_proc_sema), 0);
	_init_queue(&(pcmdpriv->cmd_processing_queue));
#endif
	
	//allocate DMA-able/Non-Page memory for cmd_buf and rsp_buf
	
	pcmdpriv->cmd_seq = 1;
	
	pcmdpriv->cmd_allocated_buf = _malloc(MAX_CMDSZ + 4);
	
	if (pcmdpriv->cmd_allocated_buf == NULL){
		res= _FAIL;
		goto exit;
		}
	pcmdpriv->cmd_buf = pcmdpriv->cmd_allocated_buf  +  4 - ( (uint)(pcmdpriv->cmd_allocated_buf) & 3);
		
	pcmdpriv->rsp_allocated_buf = _malloc(MAX_RSPSZ + 4);
	
	if (pcmdpriv->rsp_allocated_buf == NULL){
		res= _FAIL;
		goto exit;
		}
	pcmdpriv->rsp_buf = pcmdpriv->rsp_allocated_buf  +  4 - ( (uint)(pcmdpriv->rsp_allocated_buf) & 3);

	pcmdpriv->cmd_issued_cnt = pcmdpriv->cmd_done_cnt = pcmdpriv->rsp_cnt = 0;

exit:	
_func_exit_;	  	 
	return res;
	
}

u32	init_evt_priv (struct	evt_priv *pevtpriv)
{
	sint res=_SUCCESS;
	struct event_frame_priv *peventframe;
	int i;

_func_enter_;		
	_init_sema(&(pevtpriv->evt_notify), 0);
	_init_sema(&(pevtpriv->terminate_evtthread_sema), 0);

#ifdef CONFIG_H2CLBK
	_init_sema(&(pevtpriv->lbkevt_done), 0);
	pevtpriv->lbkevt_limit = 0;
	pevtpriv->lbkevt_num = 0;
	pevtpriv->cmdevt_parm = NULL;		
#endif		
	
	//allocate DMA-able/Non-Page memory for cmd_buf and rsp_buf
	
	pevtpriv->event_seq = 1;
	
	pevtpriv->evt_allocated_buf = _malloc(MAX_EVTSZ + 4);
	
	if (pevtpriv->evt_allocated_buf == NULL){
		res= _FAIL;
		goto exit;
		}
	pevtpriv->evt_buf = pevtpriv->evt_allocated_buf  +  4 - ((unsigned int)(pevtpriv->evt_allocated_buf) & 3);
	
	//initialize variables relative to event
	_init_queue(&pevtpriv->free_event_queue);
	_init_queue(&pevtpriv->pending_event_queue);
	
	pevtpriv->event_frame_allocated_buf = kmalloc(sizeof(struct event_frame_priv)*C2HEVENT_SZ + 4, GFP_KERNEL);
	if (pevtpriv->event_frame_allocated_buf == NULL)
	{
		DEBUG_ERR(("init_evt_priv: allocate eventframe fail\n"));
		res= _FAIL;
		goto exit;
	}

	DEBUG_INFO(("pevtpriv->event_frame_allocated_buf=%x\n", (u32)pevtpriv->event_frame_allocated_buf));

	memset(pevtpriv->event_frame_allocated_buf, 0, sizeof(struct event_frame_priv)*C2HEVENT_SZ + 4);
	pevtpriv->event_frame_buf = 	pevtpriv->event_frame_allocated_buf  
								+  4 - ((unsigned int)(pevtpriv->event_frame_allocated_buf) & 3);

	peventframe = (struct event_frame_priv*)pevtpriv->event_frame_buf;
	for(i=0 ; i<C2HEVENT_SZ ; i++)
	{
		_init_listhead(&peventframe->list);
		list_insert_tail(&(peventframe->list), &(pevtpriv->free_event_queue.queue));
		peventframe ++;
	}

	pevtpriv->evt_done_cnt = 0;	
		
exit:	
_func_exit_;	  	 
	return res;
}


void free_evt_priv (struct	evt_priv *pevtpriv)
{
_func_enter_;	
	if (pevtpriv->evt_allocated_buf)
		_mfree(pevtpriv->evt_allocated_buf, MAX_CMDSZ + 4);

	if(pevtpriv->event_frame_allocated_buf)
		_mfree(pevtpriv->event_frame_allocated_buf, sizeof(struct event_frame_priv)*C2HEVENT_SZ + 4);
_func_exit_;		
}	


void free_cmd_priv (struct	cmd_priv *pcmdpriv)
{
_func_enter_;
	if (pcmdpriv->cmd_allocated_buf)
		
		_mfree(pcmdpriv->cmd_allocated_buf, MAX_CMDSZ + 4);
	
	if (pcmdpriv->rsp_allocated_buf)
		_mfree(pcmdpriv->rsp_allocated_buf, MAX_RSPSZ + 4);
_func_exit_;	
}	


sint	_enqueue_cmd(_queue *queue, struct cmd_obj *obj)
{
_func_enter_;
	if (obj == NULL)
		goto exit;

	_spinlock(&(queue->lock));

	list_insert_tail(&(obj->list),&(queue->queue));

	_spinunlock(&(queue->lock));
exit:	
_func_exit_;
	return _SUCCESS;

}


//int		enqueue_cmd(_queue *queue, struct cmd_obj *obj)
u32		enqueue_cmd(struct cmd_priv *pcmdpriv, struct cmd_obj *obj)
{
	int	res;
_func_enter_;	
	if(pcmdpriv->padapter->eeprompriv.bautoload_fail_flag==_TRUE){
		DEBUG_ERR(("pcmdpriv->padapter->eeprompriv.bautoload_fail_flag=%x",pcmdpriv->padapter->eeprompriv.bautoload_fail_flag));
		return _FAIL;
	}
	res = _enqueue_cmd(&(pcmdpriv->cmd_queue),obj);

	_up_sema(&pcmdpriv->cmd_queue_sema);
	DEBUG_ERR(("\nenqueue_cmd: _up_sema(&pcmdpriv->cmd_queue_sema)\n"));
_func_exit_;		
	return res;
	
}


struct	cmd_obj	*_dequeue_cmd(_queue *queue)
{

	struct cmd_obj *obj;
_func_enter_;
	_spinlock(&(queue->lock));

	if (is_list_empty(&(queue->queue)))
		obj = NULL;
	else
	{
		obj = LIST_CONTAINOR(get_next(&(queue->queue)), struct cmd_obj, list);
		list_delete(&obj->list);
	}
	_spinunlock(&(queue->lock));
_func_exit_;	
	return obj;
}


struct	cmd_obj	*dequeue_cmd(_queue *queue)
{
	struct	cmd_obj	*pcmd;
_func_enter_;		
	pcmd = _dequeue_cmd(queue);
_func_exit_;			
	return pcmd;
}



void cmd_clr_isr(struct	cmd_priv *pcmdpriv)
{
_func_enter_;
	pcmdpriv->cmd_done_cnt++;
	DEBUG_ERR(("\n++++++++++++++++++ cmd_clr_isr \n"));
	_up_sema(&(pcmdpriv->cmd_done_sema));
_func_exit_;		
}

void evt_notify_isr(struct evt_priv *pevtpriv)
{
_func_enter_;
	pevtpriv->evt_done_cnt++;
		_up_sema(&(pevtpriv->evt_notify));
_func_exit_;	
}

void free_cmd_obj(struct cmd_obj *pcmd)
{
_func_enter_;
	if((pcmd->cmdcode!=_JoinBss_CMD_) &&(pcmd->cmdcode!= _CreateBss_CMD_))
		_mfree((unsigned char*) pcmd->parmbuf, pcmd->cmdsz);
	if(pcmd->rsp!=NULL)
		if(pcmd->rspsz!= 0)
			_mfree((unsigned char*) pcmd->rsp, pcmd->rspsz);

	_mfree((unsigned char*) pcmd, sizeof(struct cmd_obj));
_func_exit_;		
}
extern thread_return cmd_thread(thread_context context)
{
#ifdef CONFIG_RTL8711
	unsigned int	h2caddr=0x0;
#endif
	unsigned int 	val;
	struct cmd_obj	*pcmd;
	unsigned char 	*pcmdbuf, *prspbuf;

	void (*pcmd_callback)(_adapter	*dev, struct cmd_obj	*pcmd);//	void (*pcmd_callback)(unsigned char *dev, struct cmd_obj	*pcmd);

       _adapter * padapter = (_adapter *)context;
	struct	cmd_priv	*pcmdpriv = &(padapter->cmdpriv);
	
_func_enter_;

#ifdef PLATFORM_LINUX
	daemonize("%s", padapter->pnetdev->name);
	allow_signal(SIGTERM);
#endif
#ifdef CONFIG_RTL8711
	h2caddr = read32(padapter, H2CR);

	h2caddr &= 0xfffffff;
	DEBUG_INFO(("\n\n\n @@@@@@@ cmd_thread: H2C Addr=%08x @@@@@@@\n\n\n\n", h2caddr));
#endif	
	
	while(1)
	{
		pcmdbuf = pcmdpriv->cmd_buf;
		prspbuf = pcmdpriv->rsp_buf;
		
		if ((_down_sema(&(pcmdpriv->cmd_queue_sema))) == _FAIL)
			break;

		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE)){
			DEBUG_ERR(("\ncmd_thread:bDriverStopped or bSurpriseRemoved\n"));
			break;
		}
		if (register_cmd_alive(padapter) != _SUCCESS) {
			continue;
		}

		
		if(!(pcmd = dequeue_cmd(&(pcmdpriv->cmd_queue)))) {
			unregister_cmd_alive(padapter);
			continue;
		}

		DEBUG_ERR(("\n@@@@ cmd_thread: pcmd->cmdsz = _RND4!!\n"));

		 pcmd->cmdsz = _RND4( (pcmd->cmdsz));
	
		_memcpy(pcmdbuf, pcmd->parmbuf, pcmd->cmdsz);
			
		if (pcmd)
		{
		 	pcmdpriv->cmd_issued_cnt++;
#ifdef CONFIG_RTL8711			
		 	//fill memory
		 	write_mem(padapter, h2caddr + 4, pcmd->cmdsz, pcmdbuf);
		 	write32(padapter, h2caddr, (pcmd->cmdcode << 16 | pcmdpriv->cmd_seq));
			pcmdpriv->cmd_seq++;
		 	//fill H2C HOWN = 1
		 	write32(padapter,H2CR, (h2caddr | 0x01));
		 			 	
			if(pcmd->cmdcode == _SetUsbSuspend_CMD_)
			{
				_up_sema(&padapter->usb_suspend_sema);
				continue;
				
			}
#endif		 			 	
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
			DEBUG_ERR(("\n@@@@ cmd_thread: _enqueue_cmd(%d,0x%x) for mlme thread process\n",pcmd->cmdcode,pcmd->cmdcode));

			_enqueue_cmd(&(pcmdpriv->cmd_processing_queue), pcmd);
			padapter->_sys_mib.cmd_proc=_TRUE;
			_up_sema(&(pcmdpriv->cmd_proc_sema));
			DEBUG_ERR(("\n cmd_thread:_up_sema(&(pcmdpriv->cmd_proc_sema))\n"));
#endif
			//wait_H2C_Complete ...
		 	if ((_down_sema(&pcmdpriv->cmd_done_sema)) == _FAIL){
				DEBUG_ERR(("\n@@@@ cmd_thread: (_down_sema(&pcmdpriv->cmd_done_sema)) == _FAIL\n"));
		 		break;
			}
			DEBUG_ERR(("\n@@@@ cmd_thread:(_down_sema(&pcmdpriv->cmd_done_sema)\n"));
			if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
			{
				free_cmd_obj(pcmd);
				goto cmd_thread_free_resource;
			}
#ifdef CONFIG_RTL8711
			val = read32(padapter, h2caddr);
		 	val = (val >> 8) & 0xff;
#endif
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

			val=pcmd->res;
#endif
			pcmd->res =(unsigned char) val;			
			if (!((val == H2C_SUCCESS) || (val == H2C_SUCCESS_RSP))){
		 		DEBUG_ERR(("\nError in H2C cmdcode=%d, res=%d\n", pcmd->cmdcode, val));
			}

		 	DEBUG_ERR(("\n H2C cmdcode=%d, res=%d\n", pcmd->cmdcode, val));
			if (val == H2C_SUCCESS_RSP){
#ifdef CONFIG_RTL8711
		 		read_mem(padapter, h2caddr + H2C_RSP_OFFSET, pcmd->rspsz, prspbuf);
				_memcpy(pcmd->rsp, prspbuf, pcmd->rspsz);
#endif
			} 
			
			// invoke cmd->callback function
		
			pcmd_callback = cmd_callback[pcmd->cmdcode].callback;

			if(pcmd_callback==NULL)
				free_cmd_obj(pcmd);
			else
				pcmd_callback(padapter, pcmd);
		

			/* Perry:
				If cmd_queue is empty, we can lower the power level
			*/
			if(_queue_empty(&(pcmdpriv->cmd_queue)))
				unregister_cmd_alive(padapter);			
		
		}
		else
			DEBUG_ERR(("\nShall not be empty when dequeue cmd_queuu\n"));
			
	
#ifdef PLATFORM_LINUX
		if (signal_pending (current)) {
			flush_signals(current);
        	}
#endif       	
	
	}


cmd_thread_free_resource:
	
	// free all cmd_obj resources
	do{
		pcmd = dequeue_cmd(&(pcmdpriv->cmd_queue));
		if(pcmd==NULL)
			break;

		free_cmd_obj(pcmd);
	}while(1);


	_up_sema(&pcmdpriv->terminate_cmdthread_sema);

_func_exit_;	

#ifdef PLATFORM_LINUX
	
	complete_and_exit (NULL, 0);

#endif	

	

#ifdef PLATFORM_OS_XP

	PsTerminateSystemThread(STATUS_SUCCESS);

#endif

	
}


/*
C2HR Layout

Bit31 ~ Bit24: Event Code
Bit23 ~ Bit8: event length
Bit7 ~ Bit0: event_seq


*/
extern thread_return event_thread(thread_context	context)
{
#ifdef CONFIG_RTL8711
	unsigned int	c2haddr=0x0;
#endif
	unsigned int    val,r_sz, ec;
	void (*event_callback)(_adapter	*dev, u8 *pbuf);
	u8 	*peventbuf = NULL;
//	_adapter	*dev = (_adapter *)data;
	_adapter * padapter = (_adapter *)context;
	struct	evt_priv	*pevt_priv = &(padapter->evtpriv);

	struct event_frame_priv *peventframe;
_func_enter_;
	
#ifdef PLATFORM_LINUX
	//first, let's get H2C address...
	daemonize("%s", padapter->pnetdev->name);
	allow_signal(SIGTERM);
#endif	



	//first, let's get C2HR address...
#ifdef CONFIG_RTL8711	
	c2haddr = read32(padapter, C2HR);

	c2haddr &= 0xfffffff;
	DEBUG_INFO(("\n @@@@@@@ event_thread: C2HR Addr=%08x @@@@@@@\n", c2haddr));
#endif	

	while(1)
	{
		if ((_down_sema(&(pevt_priv->evt_notify))) == _FAIL)
			break;

		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE)){
			DEBUG_ERR(("event_thread:bDriverStopped or bSurpriseRemoved\n"));
			break;
		}

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

		peventframe = (struct event_frame_priv *)alloc_evt_frame(&padapter->evtpriv, &padapter->evtpriv.pending_event_queue);		
		if(peventframe == NULL)
			continue;
		val = peventframe->c2h_res;

#else
		// an event has reported by FW	
		val = read32(padapter, c2haddr);
#endif		

		r_sz = (val >> 8) & 0xffff;
		ec = (val >> 24);
		
		if(ec == 1)
			printk("event_thread: survey_done_callback ec=%x, r_sz=%x\n", ec, r_sz);
		
		// checking event sequence...		
		if ((val & 0xff) == pevt_priv->event_seq)
		{
			DEBUG_INFO(("Evetn Seq Error! %d vs %d\n", (val & 0xff), pevt_priv->event_seq));
			goto _abort_event_;
		}

		// checking if event code is valid
		if (ec >= MAX_C2HEVT)
		{
			DEBUG_ERR(("\nEvent Code(%d) mismatch!\n", (val >> 24)));
			goto _abort_event_;
		}

		// checking if event size match the event parm size

	
		if ((wlanevents[ec].parmsize != 0) && 
			(wlanevents[ec].parmsize != r_sz))
		{
			
			DEBUG_ERR(("\nEvent(%d) Parm Size mismatch (%d vs %d)!\n", 
			ec, wlanevents[ec].parmsize, r_sz));
			goto _abort_event_;	
			
		}

		pevt_priv->event_seq = (val & 0xff);
		
		//peventbuf = _malloc(r_sz);

		peventbuf = pevt_priv->evt_buf;

		if (peventbuf == NULL)
		{
			DEBUG_ERR(("\nCan't allocate memory buf for event code:%d, len:%d\n", (val >> 24), r_sz));
			goto _abort_event_;
		}

	#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
		peventbuf = peventframe->c2h_buf;
	#else
		read_mem(padapter, (c2haddr + 4), r_sz, peventbuf);	
	#endif	

		if (peventbuf)
		{
			event_callback = wlanevents[ec].event_callback;
			event_callback(padapter, peventbuf);
			//_mfree(peventbuf, r_sz);
		}

_abort_event_:
	
		//clear C2HSET...
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

		if(ec ==1)
			DEBUG_ERR(("Call c2hclrdsr at event_thread1\n"));
		
		_down_sema(&(padapter->_sys_mib.queue_event_sema));	
		c2hclrdsr(padapter);	
		_up_sema(&(padapter->_sys_mib.queue_event_sema));	
		
#else
		write32(padapter,C2HR, (c2haddr));
		//sync_ioreq_flush(dev);
#endif	
		peventbuf = NULL;

		if(ec ==1)
			DEBUG_ERR(("Call c2hclrdsr at event_thread2\n"));

		/* Perry:
			We have completed this event, unregister power level here.
		*/
		unregister_evt_alive(padapter);

		if(ec == 1)
		{
			 DEBUG_ERR(("\n^^^start_hw_event_posting :cmd_clr_isr^^^\n"));
			 cmd_clr_isr(&padapter->cmdpriv);
		}

		free_evt_frame(peventframe, &padapter->evtpriv);

#ifdef PLATFORM_LINUX
		if (signal_pending (current)) {
			flush_signals(current);
        	}
#endif       			

	}

	_up_sema(&pevt_priv->terminate_evtthread_sema);
_func_exit_;	

#ifdef PLATFORM_LINUX
	
	complete_and_exit (NULL, 0);

#endif	

#ifdef PLATFORM_OS_XP

	PsTerminateSystemThread(STATUS_SUCCESS);

#endif

}


#ifdef CONFIG_USB_HCI

/*
u8 setusbstandby_cmd(unsigned char  *adapter) 


*/
u8 setusbstandby_cmd(_adapter *padapter)
{
	struct cmd_obj*			ph2c;
	struct usb_suspend_parm*	psetusbsuspend;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;

	u8	res=_SUCCESS;
_func_enter_;	
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	
	psetusbsuspend = (struct usb_suspend_parm*)_malloc(sizeof(struct usb_suspend_parm)); 
	if(psetusbsuspend==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetusbsuspend, _SetUsbSuspend_CMD_);

	enqueue_cmd(pcmdpriv, ph2c);	
exit:	
_func_exit_;		
	return res;
}


#endif

/*
u8 setstandby_cmd(unsigned char  *adapter) 


*/
u8 setstandby_cmd(_adapter *padapter)
{
	struct cmd_obj*			ph2c;
	struct usb_suspend_parm*	psetusbsuspend;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;

	u8	res=_SUCCESS;
_func_enter_;	
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	
	psetusbsuspend = (struct usb_suspend_parm*)_malloc(sizeof(struct usb_suspend_parm)); 
	if(psetusbsuspend==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetusbsuspend, _SetUsbSuspend_CMD_);

	enqueue_cmd(pcmdpriv, ph2c);	
exit:	
_func_exit_;		
	return res;
}




/*
sitesurvey_cmd(~)
	### NOTE:#### (!!!!)
	MUST TAKE CARE THAT BEFORE CALLING THIS FUNC, YOU SHOULD HAVE LOCKED pmlmepriv->lock
*/
u8 sitesurvey_cmd(_adapter  *padapter, NDIS_802_11_SSID *pssid)
{
	struct cmd_obj*			ph2c;
	struct sitesurvey_parm*	psurveyPara;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	struct mlme_priv			*pmlmepriv = &padapter->mlmepriv;


_func_enter_;	
	
	DEBUG_INFO(("+sitesurvey_cmd \n"));

	
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c == NULL)
		return _FAIL;



	psurveyPara = (struct sitesurvey_parm*)_malloc(sizeof(struct sitesurvey_parm)); 

	if(psurveyPara == NULL) {
		
		_mfree((unsigned char *) ph2c, sizeof(struct	cmd_obj));

		return _FAIL;
	}

	free_network_queue(padapter);
	DEBUG_INFO(("\nflush  network queue\n\n"));

	init_h2fwcmd_w_parm_no_rsp(ph2c, psurveyPara, _SiteSurvey_CMD_);

	psurveyPara->bsslimit = cpu_to_le32(48);
	psurveyPara->passive_mode = cpu_to_le32(padapter->registrypriv.scan_mode);
	psurveyPara->ss_ssidlen= cpu_to_le32(0);// pssid->SsidLength;

	//if (pssid->SsidLength)
	//	_memcpy(psurveyPara->ss_ssid, pssid, pssid->SsidLength);
	//else
		memset(psurveyPara->ss_ssid, 0, IW_ESSID_MAX_SIZE + 1);
		
	set_fwstate(pmlmepriv, _FW_UNDER_SURVEY);

	enqueue_cmd(pcmdpriv, ph2c);	
	

	DEBUG_INFO(("-sitesurvey_cmd \n"));
_func_exit_;		
	return _SUCCESS;
}



u8 setdatarate_cmd(_adapter  *padapter, u8 *rateset)
{
       struct cmd_obj*			ph2c;
	struct setdatarate_parm*	pbsetdataratepara;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	u8	res=_SUCCESS;
	
_func_enter_;	
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
	}
	pbsetdataratepara = (struct setdatarate_parm*)_malloc(sizeof(struct setdatarate_parm)); 

	if(pbsetdataratepara==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, pbsetdataratepara, _SetDataRate_CMD_);

      pbsetdataratepara->mac_id = 5;
       _memcpy(pbsetdataratepara->datarates, rateset, NumRates);	   

	enqueue_cmd(pcmdpriv, ph2c);	
exit:
_func_exit_;		
	return res;

}

u8 setbasicrate_cmd(_adapter  *padapter, u8 *rateset)
{
       struct cmd_obj*			ph2c;
	struct setbasicrate_parm*	pssetbasicratepara;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	u8	res=_SUCCESS;
	
_func_enter_;	
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	pssetbasicratepara = (struct setbasicrate_parm*)_malloc(sizeof(struct setbasicrate_parm)); 

	if(pssetbasicratepara==NULL){
		_mfree((u8*) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, pssetbasicratepara, _SetBasicRate_CMD_);

       _memcpy(pssetbasicratepara->basicrates, rateset, NumRates);	   

	enqueue_cmd(pcmdpriv, ph2c);	
exit:	
_func_exit_;		
	return res;

}


/*
unsigned char setphy_cmd(unsigned char  *adapter) 

1.  be called only after update_registrypriv_dev_network( ~) or mp testing program
2.  for AdHoc/Ap mode or mp mode?

*/
u8 setphy_cmd(_adapter *padapter, u8 modem, u8 ch)
{
	struct cmd_obj*			ph2c;
	struct setphy_parm*		psetphypara;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
//	struct mlme_priv			*pmlmepriv = &padapter->mlmepriv;
//	struct registry_priv*		pregistry_priv = &padapter->registrypriv;
//	NDIS_WLAN_BSSID_EX		*dev_network = &padapter->registrypriv.dev_network;
	u8	res=_SUCCESS;
_func_enter_;	
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	psetphypara = (struct setphy_parm*)_malloc(sizeof(struct setphy_parm)); 

	if(psetphypara==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetphypara, _SetPhy_CMD_);

	DEBUG_INFO(("CH=%d, modem=%d", ch, modem));

	psetphypara->modem = modem;
	psetphypara->rfchannel = ch;

	enqueue_cmd(pcmdpriv, ph2c);	
exit:	
_func_exit_;		
	return res;
}

u8 setbbreg_cmd(_adapter*padapter, u8 offset, u8 val)
{
	
	struct cmd_obj*			ph2c;
	struct writeBB_parm*		pwritebbparm;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;	
	u8	res=_SUCCESS;
_func_enter_;
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	pwritebbparm = (struct writeBB_parm*)_malloc(sizeof(struct writeBB_parm)); 

	if(pwritebbparm==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, pwritebbparm, GEN_CMD_CODE(_SetBBReg));	

	pwritebbparm->offset = offset;
	pwritebbparm->value = val;

	enqueue_cmd(pcmdpriv, ph2c);	
exit:	
_func_exit_;	
	return res;
}

u8 getbbreg_cmd(_adapter  *padapter, u8 offset, u8 *pval)
{
	
	struct cmd_obj*			ph2c;
	struct readBB_parm*		prdbbparm;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	u8	res=_SUCCESS;
	
_func_enter_;
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res=_FAIL;
		goto exit;
		}
	prdbbparm = (struct readBB_parm*)_malloc(sizeof(struct readBB_parm)); 

	if(prdbbparm ==NULL){
		_mfree((unsigned char *) ph2c, sizeof(struct	cmd_obj));
		return _FAIL;
	}

	_init_listhead(&ph2c->list);
	ph2c->cmdcode =GEN_CMD_CODE(_GetBBReg);
	ph2c->parmbuf = (unsigned char *)prdbbparm;
	ph2c->cmdsz =  sizeof(struct readBB_parm);
	ph2c->rsp = pval;
	ph2c->rspsz = sizeof(struct readBB_rsp);
	
	prdbbparm ->offset = offset;
	
	enqueue_cmd(pcmdpriv, ph2c);	
exit:
_func_exit_;	
	return res;
}

u8 setrfreg_cmd(_adapter  *padapter, u8 offset, u32 val)
{
	
	struct cmd_obj*			ph2c;
	struct writeRF_parm*		pwriterfparm;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;	
	u8	res=_SUCCESS;
_func_enter_;
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;	
		goto exit;
		}
	pwriterfparm = (struct writeRF_parm*)_malloc(sizeof(struct writeRF_parm)); 

	if(pwriterfparm==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, pwriterfparm, GEN_CMD_CODE(_SetRFReg));	

	pwriterfparm->offset = offset;
	pwriterfparm->value = val;

	enqueue_cmd(pcmdpriv, ph2c);	
exit:
_func_exit_;	
	return res;
}
u8 getrfreg_cmd(_adapter  *padapter, u8 offset, u8 *pval)
{
	
	struct cmd_obj*			ph2c;
	struct readRF_parm*		prdrfparm;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;	
	u8	res=_SUCCESS;
_func_enter_;
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	prdrfparm = (struct readRF_parm*)_malloc(sizeof(struct readRF_parm)); 

	if(prdrfparm ==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	_init_listhead(&ph2c->list);
	ph2c->cmdcode =GEN_CMD_CODE(_GetRFReg);
	ph2c->parmbuf = (unsigned char *)prdrfparm;
	ph2c->cmdsz =  sizeof(struct readRF_parm);
	ph2c->rsp = pval;
	ph2c->rspsz = sizeof(struct readRF_rsp);
	
	prdrfparm ->offset = offset;
	
	enqueue_cmd(pcmdpriv, ph2c);	
exit:
_func_exit_;	
	return res;
}

void getbbrfreg_cmdrsp_callback(_adapter*	padapter,  struct cmd_obj *pcmd)
{
       //NDIS_STATUS  status;
 _func_enter_;  
	
	
	//free_cmd_obj(pcmd);
	_mfree((unsigned char*) pcmd->parmbuf, pcmd->cmdsz);
	_mfree((unsigned char*) pcmd, sizeof(struct cmd_obj));
	
#ifdef CONFIG_MP_INCLUDED	
	padapter->mppriv.workparam.bcompleted= _TRUE;
#endif	
_func_exit_;		
}

u8 createbss_cmd(_adapter  *padapter)
{
	struct cmd_obj*			pcmd;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	struct mlme_priv			*pmlmepriv = &padapter->mlmepriv;
	NDIS_WLAN_BSSID_EX		*pdev_network = &padapter->registrypriv.dev_network;
	u8	res=_SUCCESS;
_func_enter_;
	if (pmlmepriv->assoc_ssid.SsidLength == 0){
		DEBUG_INFO((" createbss for Any SSid:%s\n",pmlmepriv->assoc_ssid.Ssid));		
	}
	else{
		DEBUG_INFO((" createbss for SSid:%s\n", pmlmepriv->assoc_ssid.Ssid));
	}
		
	pcmd = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(pcmd==NULL){
		res= _FAIL;
		goto exit;
		}

	_init_listhead(&pcmd->list);
	pcmd->cmdcode = _CreateBss_CMD_;
	pcmd->parmbuf = (unsigned char *)pdev_network;
	pcmd->cmdsz =  (get_NDIS_WLAN_BSSID_EX_sz(pdev_network));
	pcmd->rsp = NULL;
	pcmd->rspsz = 0;
	
	

	enqueue_cmd(pcmdpriv, pcmd);	

exit:
_func_exit_;	
	return res;
}


u8 joinbss_cmd(_adapter  *padapter, struct wlan_network* pnetwork)
{
	u8 *auth,res=_SUCCESS;
	uint t_len=0;
	u32 i = 0 ;
	NDIS_WLAN_BSSID_EX		*psecnetwork;
	struct cmd_obj*			pcmd;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	struct mlme_priv			*pmlmepriv = &padapter->mlmepriv;
	struct qos_priv   			*pqospriv= &padapter->qospriv;
	struct security_priv *psecuritypriv=&padapter->securitypriv;
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
//	u8  *address=&pnetwork->network.IEs[0];

_func_enter_;

	if (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)  
		DEBUG_ERR(("JoinBSS_CMD : check fw FW LINKED \n"));

	if (pmlmepriv->assoc_ssid.SsidLength == 0){
		DEBUG_INFO(("@@@@@   Join cmd  for Any SSid\n"));		
	}
	else{
		DEBUG_ERR(("@@@@@    Join cmd for SSid:%s\n", pmlmepriv->assoc_ssid.Ssid));
	}

	pcmd = (struct	cmd_obj*)_malloc(sizeof(struct	cmd_obj));
	if(pcmd==NULL){
		res=_FAIL;
		DEBUG_INFO(("\n joinbss_cmd :memory allocate for cmd_obj fail!!!\n"));
		goto exit;
	}
	t_len = sizeof (ULONG) + sizeof (NDIS_802_11_MAC_ADDRESS) + 2 + 
			sizeof (NDIS_802_11_SSID) + sizeof (ULONG) + 
			sizeof (NDIS_802_11_RSSI) + sizeof (NDIS_802_11_NETWORK_TYPE) + 
			sizeof (NDIS_802_11_CONFIGURATION) +	
			sizeof (NDIS_802_11_NETWORK_INFRASTRUCTURE) +   
			sizeof (NDIS_802_11_RATES_EX)+ sizeof (ULONG) + 192;

// add HT INFO & Cab length

	#ifdef CONFIG_RTL8192U

	// u8 for SLOTTIME
	t_len +=sizeof(BSS_HT)+sizeof(BSS_QOS)+sizeof(u8);

	#endif

	psecnetwork=(NDIS_WLAN_BSSID_EX *)&psecuritypriv->sec_bss;

	if(psecnetwork==NULL){
		if(pcmd !=NULL)
			_mfree((unsigned char *)pcmd, sizeof(struct	cmd_obj));
		res=_FAIL;
		DEBUG_ERR(("\n joinbss_cmd :psecnetwork==NULL!!!\n"));
		goto exit;
	}

	_memcpy(psecnetwork, &pnetwork->network,t_len);
	auth=&psecuritypriv->authenticator_ie[0];
	psecuritypriv->authenticator_ie[0]=(unsigned char)psecnetwork->IELength;
      _memcpy(&psecuritypriv->authenticator_ie[1],&psecnetwork->IEs[12],psecnetwork->IELength-12);
	psecnetwork->IELength=restruct_sec_ie(padapter,&pnetwork->network.IEs[0],&psecnetwork->IEs[0],pnetwork->network.IELength);

	DEBUG_INFO(("\njoinbss_cmd: restruct_sec_ie=> psecnetwork->IELength=%d\n", psecnetwork->IELength));
	if(pregistrypriv->wmm_enable)	
	{		
	
		u32 tmp_len;
		
		tmp_len = restruct_wmm_ie(padapter,&pnetwork->network.IEs[0],&psecnetwork->IEs[0],pnetwork->network.IELength, psecnetwork->IELength);	

		if (psecnetwork->IELength != tmp_len)		
		{
			psecnetwork->IELength = tmp_len;
			pqospriv->qos_option = 1; //There is WMM IE in this corresp. beacon
		}
		else 
		{
			pqospriv->qos_option = 0;//There is no WMM IE in this corresp. beacon
		}
	}	
	else
	{
		pqospriv->qos_option = 0;
	}	

		
	psecuritypriv->supplicant_ie[0]=(u8)psecnetwork->IELength;
	_memcpy(&psecuritypriv->supplicant_ie[1],&psecnetwork->IEs[0],psecnetwork->IELength);
        pcmd->cmdsz = (get_NDIS_WLAN_BSSID_EX_sz(psecnetwork));//t_len;//get_NDIS_WLAN_BSSID_EX_sz(&pnetwork->network);//(get_NDIS_WLAN_BSSID_EX_sz(psecnetwork));

//endian conversion
       psecnetwork->Length = cpu_to_le32(psecnetwork->Length );
	psecnetwork->Ssid.SsidLength= cpu_to_le32(psecnetwork->Ssid.SsidLength);
	psecnetwork->Configuration.DSConfig = cpu_to_le32(psecnetwork->Configuration.DSConfig);
	psecnetwork->IELength = cpu_to_le32(psecnetwork->IELength);
        psecnetwork->InfrastructureMode = cpu_to_le32(psecnetwork->InfrastructureMode);

	_init_listhead(&pcmd->list);
	pcmd->cmdcode = _JoinBss_CMD_;
	pcmd->parmbuf = (unsigned char *)psecnetwork;
	pcmd->rsp = NULL;
	pcmd->rspsz = 0;
	DEBUG_INFO(("joinbss cmd :STA : ~~"));
	for(i = 0 ; i < 6 ; i ++)
		DEBUG_INFO(("%x ", psecnetwork->MacAddress[i]));
	enqueue_cmd(pcmdpriv, pcmd);

exit:
_func_exit_;
	return res;

}

u8 disassoc_cmd(_adapter*padapter) // for sta_mode
{
	struct	cmd_obj*	pdisconnect_cmd;
	struct	disconnect_parm* pdisconnect;
	struct 	mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct	cmd_priv   *pcmdpriv = &padapter->cmdpriv;
	u8	res=_SUCCESS;

_func_enter_;

	DEBUG_INFO(("disassoc_cmd. \n"));

	if ((check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)  
		||(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)== _TRUE))
	{
		pdisconnect_cmd = (struct	cmd_obj*)_malloc(sizeof(struct	cmd_obj));
		if(pdisconnect_cmd == NULL){
			res=_FAIL;
			goto exit;
			}
		pdisconnect = (struct	disconnect_parm*)_malloc(sizeof(struct	disconnect_parm));
		if(pdisconnect == NULL) {
			_mfree((u8 *)pdisconnect_cmd, sizeof(struct cmd_obj));
			res= _FAIL;
			goto exit;
		}
		
		init_h2fwcmd_w_parm_no_rsp(pdisconnect_cmd, pdisconnect, _DisConnect_CMD_);

		enqueue_cmd(pcmdpriv, pdisconnect_cmd);
	}
	
exit:
_func_exit_;	
	return res;

}

u8 setopmode_cmd(_adapter  *padapter, NDIS_802_11_NETWORK_INFRASTRUCTURE networktype)
{
	struct	cmd_obj*	ph2c;
	struct	setopmode_parm* psetop;

	struct	cmd_priv   *pcmdpriv= &padapter->cmdpriv;
	u8	res=_SUCCESS;
_func_enter_;
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));			
	if(ph2c==NULL){
		res= _FALSE;
		goto exit;
		}
	psetop = (struct setopmode_parm*)_malloc(sizeof(struct setopmode_parm)); 

	if(psetop==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res=_FALSE;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetop, _SetOpMode_CMD_);
	psetop->mode = networktype;

	enqueue_cmd(pcmdpriv, ph2c);
exit:
_func_exit_;	
	return res;

}


u8 setstakey_cmd(_adapter *padapter, u8 *psta, u8 unicast_key)
{
	struct cmd_obj*			ph2c;
	struct set_stakey_parm	*psetstakey_para;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	struct set_stakey_rsp		*psetstakey_rsp = NULL;
	
	struct mlme_priv			*pmlmepriv = &padapter->mlmepriv;
	struct security_priv 		*psecuritypriv = &padapter->securitypriv;
	struct sta_info* 			sta = (struct sta_info* )psta;
	u8	i=0 , res=_SUCCESS;
_func_enter_;

	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if ( ph2c == NULL){
		DEBUG_ERR(("setstakey_cmd : alloc command obj failed\n"));
		res= _FAIL;
		goto exit;
	}
	psetstakey_para = (struct set_stakey_parm*)_malloc(sizeof(struct set_stakey_parm));

	if(psetstakey_para==NULL){
		DEBUG_ERR(("setstakey_cmd : alloc sta_para failed\n"));
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res=_FAIL;
		goto exit;
	}

	psetstakey_rsp = (struct set_stakey_rsp*)_malloc(sizeof(struct set_stakey_rsp)); 
	if(psetstakey_rsp == NULL){

		DEBUG_ERR(("setstakey_cmd : alloc stakey_rsp failed\n"));

		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		_mfree((u8 *) psetstakey_para, sizeof(struct set_stakey_parm));
		res=_FAIL;
		goto exit;
	}

	_memcpy(psetstakey_rsp->addr, sta->hwaddr,ETH_ALEN);

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetstakey_para, _SetStaKey_CMD_);
	ph2c->rsp = (u8 *) psetstakey_rsp;
	ph2c->rspsz = sizeof(struct set_stakey_rsp);


	_memcpy(psetstakey_para->addr, sta->hwaddr,ETH_ALEN);
	
	DEBUG_ERR(("set key : sta addr: \n"));
	for(i = 0 ; i < 6 ; i++)
		DEBUG_ERR(("%x ", sta->hwaddr[i]));

	if(check_fwstate(pmlmepriv, WIFI_STATION_STATE))
		psetstakey_para->algorithm =(unsigned char) psecuritypriv->dot11PrivacyAlgrthm;
	else
		GET_ENCRY_ALGO(psecuritypriv, sta, psetstakey_para->algorithm, _FALSE);


	if(unicast_key == _TRUE)
		_memcpy(&psetstakey_para->key, &sta->dot118021x_UncstKey, 16);
	else
		_memcpy(&psetstakey_para->key, &psecuritypriv->dot118021XGrpKey[psecuritypriv->dot118021XGrpKeyid].skey, 16);

	enqueue_cmd(pcmdpriv, ph2c);	

exit:
_func_exit_;	
	return res;

}
u8 setrfintfs_cmd(_adapter  *padapter, u8 mode)
{	
	struct cmd_obj*			ph2c;
	struct setrfintfs_parm*		psetrfintfsparm;	
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	u8	res=_SUCCESS;
_func_enter_;	

	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	psetrfintfsparm = (struct setrfintfs_parm*)_malloc(sizeof(struct setrfintfs_parm)); 

	if(psetrfintfsparm==NULL){
		_mfree((unsigned char *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetrfintfsparm, GEN_CMD_CODE(_SetRFIntFs));

	psetrfintfsparm->rfintfs = mode;

	enqueue_cmd(pcmdpriv, ph2c);	
exit:
_func_exit_;	
	return res;

}

u8 setrttbl_cmd(_adapter  *padapter, struct setratable_parm *prate_table)
{
	struct cmd_obj*			ph2c;
	struct setratable_parm *	psetrttblparm;	
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	u8	res=_SUCCESS;
_func_enter_;	

	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	psetrttblparm = (struct setratable_parm*)_malloc(sizeof(struct setratable_parm)); 

	if(psetrttblparm==NULL){
		_mfree((unsigned char *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetrttblparm, GEN_CMD_CODE(_SetRaTable));

	_memcpy(psetrttblparm,prate_table,sizeof(struct setratable_parm));

	enqueue_cmd(pcmdpriv, ph2c);	
exit:
_func_exit_;	
	return res;

}

u8 getrttbl_cmd(_adapter  *padapter, struct getratable_rsp *pval)
{
	struct cmd_obj*			ph2c;
	struct getratable_parm *	pgetrttblparm;	
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	u8	res=_SUCCESS;
_func_enter_;	

	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	pgetrttblparm = (struct getratable_parm*)_malloc(sizeof(struct getratable_parm)); 

	if(pgetrttblparm==NULL){
		_mfree((unsigned char *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

//	init_h2fwcmd_w_parm_no_rsp(ph2c, psetrttblparm, GEN_CMD_CODE(_SetRaTable));

	_init_listhead(&ph2c->list);
	ph2c->cmdcode =GEN_CMD_CODE(_GetRaTable);
	ph2c->parmbuf = (unsigned char *)pgetrttblparm;
	ph2c->cmdsz =  sizeof(struct getratable_parm);
	ph2c->rsp = (u8*)pval;
	ph2c->rspsz = sizeof(struct getratable_rsp);
	
	pgetrttblparm ->rsvd = 0x0;
	
	enqueue_cmd(pcmdpriv, ph2c);	
exit:
_func_exit_;	
	return res;

}

u8 setassocsta_cmd(_adapter  *padapter, u8 *mac_addr)
{
	struct cmd_obj*			ph2c;
	struct set_assocsta_parm	*psetassocsta_para;
	struct cmd_priv 			*pcmdpriv=&padapter->cmdpriv;
	struct set_stakey_rsp		*psetassocsta_rsp = NULL;
	u8	res=_SUCCESS;
_func_enter_;	

	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	psetassocsta_para = (struct set_assocsta_parm*)_malloc(sizeof(struct set_assocsta_parm));

	if(psetassocsta_para==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		res=_FAIL;
		goto exit;
	}

	psetassocsta_rsp = (struct set_stakey_rsp*)_malloc(sizeof(struct set_assocsta_rsp)); 
	if(psetassocsta_rsp==NULL){
		_mfree((u8 *) ph2c, sizeof(struct	cmd_obj));
		_mfree((u8 *) psetassocsta_para, sizeof(struct set_assocsta_parm));
		return _FAIL;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetassocsta_para, _SetAssocSta_CMD_);
	ph2c->rsp = (u8 *) psetassocsta_rsp;
	ph2c->rspsz = sizeof(struct set_assocsta_rsp);

	_memcpy(psetassocsta_para->addr, mac_addr,ETH_ALEN);
	
	enqueue_cmd(pcmdpriv, ph2c);	

exit:
_func_exit_;	
	return res;

 }

void survey_cmd_callback(_adapter*	padapter ,  struct cmd_obj *pcmd)
{

	struct 	mlme_priv *pmlmepriv = &padapter->mlmepriv;
_func_enter_;	

	if((pcmd->res != H2C_SUCCESS)){

		clr_fwstate(pmlmepriv, _FW_UNDER_SURVEY);
		DEBUG_ERR(("\nsurvey_cmd_callback : clr _FW_UNDER_SURVEY "));		
		DEBUG_ERR(("\n ********Error: MgntActSet_802_11_BSSID_LIST_SCAN Fail ************\n\n."));
	} 

	// free cmd
	free_cmd_obj(pcmd);

	//john debug 0521
	_up_sema(&pmlmepriv->ioctl_sema);
	
_func_exit_;	
}
void disassoc_cmd_callback(_adapter*	padapter,  struct cmd_obj *pcmd)
{
	_irqL	irqL;
	struct 	mlme_priv *pmlmepriv = &padapter->mlmepriv;
_func_enter_;	
	if((pcmd->res != H2C_SUCCESS)){

		_enter_critical(&pmlmepriv->lock, &irqL);
		set_fwstate(pmlmepriv, _FW_LINKED);
		_exit_critical(&pmlmepriv->lock, &irqL);
				
		DEBUG_ERR(("\n ***Error: disconnect_cmd_callback Fail ***\n."));

		goto exit;
	}

	// free cmd
	free_cmd_obj(pcmd);
exit:	
_func_exit_;	
}


void joinbss_cmd_callback(_adapter*	padapter,  struct cmd_obj *pcmd)
{

	struct 	mlme_priv *pmlmepriv = &padapter->mlmepriv;

_func_enter_;	
	if((pcmd->res != H2C_SUCCESS))
	{				
		DEBUG_ERR(("\n joinbss_cmd_callback ********Error:select_and_join_from_scanned_queue Wait Sema  Fail ************\n\n."));
		_set_timer(&pmlmepriv->assoc_timer, 1);
		//NdisMSetTimer(&pmlmepriv->assoc_timer, 1 );
	}
	
	free_cmd_obj(pcmd);
_func_exit_;	
}

void createbss_cmd_callback(_adapter*	padapter, struct cmd_obj *pcmd)
{	
	_irqL irqL;

#ifdef PLATFORM_WINDOWS

	BOOLEAN	 timer_cancelled;
#else
	u8 timer_cancelled;	
#endif

	struct sta_info *psta = NULL;
	struct wlan_network *pwlan = NULL;	
		
	struct 	mlme_priv *pmlmepriv = &padapter->mlmepriv;	
	NDIS_WLAN_BSSID_EX *pnetwork = (NDIS_WLAN_BSSID_EX *) pcmd->parmbuf; // pcmd->parmbuf == dev_network
	struct wlan_network *tgt_network = &(pmlmepriv->cur_network);

_func_enter_;	

	if((pcmd->res != H2C_SUCCESS))
	{	
		DEBUG_ERR(("\n ********Error: createbss_cmd_callback  Fail ************\n\n."));
		_set_timer(&pmlmepriv->assoc_timer, 1 );
		//NdisMSetTimer(&pmlmepriv->assoc_timer, 1 );
	}

	//NdisMCancelTimer(&pmlmepriv->assoc_timer, &timer_cancelled);
	_cancel_timer(&pmlmepriv->assoc_timer, &timer_cancelled);

	_enter_critical(&pmlmepriv->lock, &irqL);

	
	{
		//rtl8711_add_network(adapter, pnetwork);
		//update_scanned_network((unsigned char *)adapter, pnetwork);

		pwlan = alloc_network(pmlmepriv);

		if ( pwlan == NULL)
		{
			pwlan = get_oldest_wlan_network(&pmlmepriv->scanned_queue);
			if( pwlan == NULL)
			{
				DEBUG_ERR(("\n Error:  can't get pwlan in joinbss_event_callback \n"));
				goto createbss_cmd_fail;
			}
			pwlan->last_scanned = get_current_time();			
		}	
				
		pnetwork->Length = get_NDIS_WLAN_BSSID_EX_sz(pnetwork);
		_memcpy(&(pwlan->network), pnetwork, pnetwork->Length );
		pwlan->fixed = _TRUE;

		list_insert_tail(&(pwlan->list),&pmlmepriv->scanned_queue.queue);

		// copy pdev_network information to 	pmlmepriv->cur_network
		_memcpy(&tgt_network->network, pnetwork,
				(get_NDIS_WLAN_BSSID_EX_sz(pnetwork)));

		// reset DSConfig
		tgt_network->network.Configuration.DSConfig = (u32) ch2freq(pnetwork->Configuration.DSConfig);

		if(pmlmepriv->fw_state & _FW_UNDER_LINKING)
		    pmlmepriv->fw_state ^= _FW_UNDER_LINKING;

		
		if((pmlmepriv->fw_state) & WIFI_AP_STATE)
		{
			psta = alloc_stainfo(&padapter->stapriv, pnetwork->MacAddress);

			if (psta == NULL) { // for AP Mode & Adhoc Master Mode
				DEBUG_ERR(("\nCan't alloc sta_info when createbss_cmd_callback\n"));
				goto createbss_cmd_fail ;
			}
			indicate_connect( padapter);
		}
		else {

			//indicate_disconnect(dev);
		}
		

		// we will set _FW_LINKED when there is one more sat to join us (stassoc_event_callback)
			
	}

createbss_cmd_fail:
	
	_exit_critical(&pmlmepriv->lock, &irqL);


	free_cmd_obj(pcmd);
_func_exit_;	

}



void setstaKey_cmdrsp_callback(_adapter*	padapter ,  struct cmd_obj *pcmd)
{
	
	struct sta_priv * pstapriv = &padapter->stapriv;
	struct set_stakey_rsp* psetstakey_rsp = (struct set_stakey_rsp*) (pcmd->rsp);
	struct sta_info*	psta = get_stainfo(pstapriv, psetstakey_rsp->addr);
	u32 i = 0 ;

_func_enter_;	
	DEBUG_ERR(("setstakey_cmd rsp callback : sta addr: \n"));
	for(i = 0 ; i < 6 ; i++)
		DEBUG_ERR(("%x ", psetstakey_rsp->addr[i]));

	if(psta==NULL)
	{
		DEBUG_ERR(("\nERROR: setstaKey_cmdrsp_callback => can't get sta_info \n\n"));
		goto exit;
	}
	psta->aid = psta->mac_id = psetstakey_rsp->keyid;
	
	free_cmd_obj(pcmd);
exit:	
_func_exit_;	
}
void setassocsta_cmdrsp_callback(_adapter*	padapter,  struct cmd_obj *pcmd)
{
	_irqL	irqL;
	struct sta_priv * pstapriv = &padapter->stapriv;
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;	
	struct set_assocsta_parm* passocsta_parm = (struct set_assocsta_parm*)(pcmd->parmbuf);
	struct set_assocsta_rsp* passocsta_rsp = (struct set_assocsta_rsp*) (pcmd->rsp);		
	struct sta_info*	psta = get_stainfo(pstapriv, passocsta_parm->addr);

_func_enter_;	
	
	if(psta==NULL)
	{
		DEBUG_ERR(("\nERROR: setassocsta_cmdrsp_callbac => can't get sta_info \n\n"));
		goto exit;
	}
	psta->aid = psta->mac_id = passocsta_rsp->cam_id;


	_enter_critical(&(pmlmepriv->lock), &irqL);

       if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE) && (check_fwstate(pmlmepriv, _FW_UNDER_LINKING) == _TRUE))           	
     	   pmlmepriv->fw_state ^= _FW_UNDER_LINKING;         

       set_fwstate(pmlmepriv, _FW_LINKED);       	   
	_exit_critical(&(pmlmepriv->lock), &irqL);
	  
	free_cmd_obj(pcmd);
exit:	
_func_exit_;	  
}

void getrttbl_cmd_cmdrsp_callback(_adapter*	padapter,  struct cmd_obj *pcmd)
{
_func_enter_;		  
	free_cmd_obj(pcmd);
#ifdef CONFIG_MP_INCLUDED
	padapter->mppriv.workparam.bcompleted=_TRUE;
#endif
//exit:	
_func_exit_;	  
}
#ifdef CONFIG_PWRCTRL

u8  setatim_cmd(_adapter* adapter, u8 add, u8 txid)
{
	
	struct	cmd_obj*	ph2c;
	struct	setatim_parm* psetatim;
	struct	cmd_priv   *pcmdpriv= &( adapter->cmdpriv);
	u8	res=_SUCCESS;
_func_enter_;
	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		goto exit;
		res= _FAIL;
	}
	psetatim = (struct setatim_parm*)_malloc(sizeof(struct setatim_parm)); 

	if(psetatim == NULL){
		_mfree((unsigned char *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	//NdisZeroMemory();
	psetatim->op = add;
	psetatim->txid = txid;


	init_h2fwcmd_w_parm_no_rsp(ph2c, psetatim, _SetAtim_CMD_);

	enqueue_cmd(pcmdpriv, ph2c);
exit:
_func_exit_;
	return res;	
}


u8 setpwrmode_cmd(_adapter* adapter, u32 ps_mode)
{
	struct	cmd_obj*	ph2c;
	struct	setpwrmode_parm* psetpwr;
	struct	cmd_priv   *pcmdpriv= &( adapter->cmdpriv);
	u8 res=_SUCCESS;
_func_enter_;
	DEBUG_ERR(("setpwrmode_cmd mode  = %x", ps_mode));

	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
		}
	psetpwr = (struct setpwrmode_parm*)_malloc(sizeof(struct setpwrmode_parm)); 

	if(psetpwr == NULL){
		_mfree((unsigned char *) ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	//NdisZeroMemory();
	memset(psetpwr, 0, sizeof(struct setpwrmode_parm));

	psetpwr->mode = (unsigned char)ps_mode;
	psetpwr->bcn_pass_cnt = 0;

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetpwr, _SetPwrMode_CMD_);

	enqueue_cmd(pcmdpriv, ph2c);
exit:
_func_exit_;
	return res;	
}
#endif
