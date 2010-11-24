/******************************************************************************
* os_intfs.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                      
*		drv_entry
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
#define _OS_INTFS_C_

#include <drv_conf.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#define DBG_DECLARE 7

#include <linux/module.h>
#include <linux/init.h>

#include <osdep_service.h>
#include <drv_types.h>
#include <xmit_osdep.h>
#include <recv_osdep.h>
#include <mlme_osdep.h>
#include <hal_init.h>
#include <rtl871x_ioctl.h>
#include <rtl871x_mlme.h>

#ifdef CONFIG_RTL8192U
#include <rtl8192u/rtl8192u_haldef.h>
#endif

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
#include <linux/proc_fs.h>
#endif

#ifdef CONFIG_SDIO_HCI
#include <sdio_osintf.h>
#endif

#ifdef CONFIG_USB_HCI
//#include <linux/usb.h>
#include <usb_osintf.h>
#endif

MODULE_DEVICE_TABLE(usb, rtl871x_usb_id_tbl);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rtl8711 sdio wireless lan driver");
MODULE_AUTHOR("...");

/* module param defaults */
int rfintfs = HWPI;
int lbkmode = RTL8711_AIR_TRX;
#ifdef CONFIG_RTL8711
int nic_version=RTL8711_NIC;
int chip_version =RTL8711_3rdCUT;
#ifdef CONFIG_SDIO_HCI
int hci = RTL8711_SDIO;
#endif
#ifdef CONFIG_USB_HCI
int hci = RTL8711_USB;
#endif
#endif
#ifdef CONFIG_RTL8187
int nic_version=RTL8187_NIC;
int hci = RTL8187_USB;
#endif

#ifdef CONFIG_RTL8192U
int nic_version=RTL8192U_NIC;
int hci=RTL8192U_USB;
#endif

int network_mode = Ndis802_11Infrastructure;//infra, ad-hoc, auto	  
//NDIS_802_11_SSID	ssid;
int channel = 1;//ad-hoc support requirement 
int wireless_mode = WIRELESS_11BG;
int vrtl_carrier_sense = AUTO_VCS;
int vcs_type = RTS_CTS;//*
int rts_thresh = 2347;//*
int frag_thresh = 2346;//*
int preamble = PREAMBLE_LONG;//long, short, auto
int scan_mode = 1;//active, passive
int adhoc_tx_pwr = 1;
int soft_ap = 0;
int smart_ps = 0;  
int power_mgnt = PWR_CAM;
int radio_enable = 1;
int long_retry_lmt = 7;
int short_retry_lmt = 7;
int busy_thresh = 40;
//int qos_enable = 0; //*
int ack_policy = NORMAL_ACK;
int mp_mode = 0;	
int software_encrypt =0;
int software_decrypt = 0;	  
 
int wmm_enable = 1;
int uapsd_enable = 0;	  
int uapsd_max_sp = NO_LIMIT;
int uapsd_acbk_en = 0;
int uapsd_acbe_en = 0;
int uapsd_acvi_en = 0;
int uapsd_acvo_en = 0;
	
#ifdef CONFIG_RTL8711
module_param(chip_version, int, 0644);
#endif
module_param(rfintfs, int, 0644);
module_param(lbkmode, int, 0644);
module_param(hci, int, 0644);





uint loadparam( _adapter *padapter,  _nic_hdl	pnetdev);

static int __init drv_entry(void);
static void __exit drv_halt(void);
#ifdef CONFIG_SDIO_HCI
static unsigned char drv_init(PSDFUNCTION psdfunc, PSDDEVICE psddev);
static void dev_remove(PSDFUNCTION psdfunc, PSDDEVICE psddev);
#endif
#ifdef CONFIG_USB_HCI
static int drv_init(struct usb_interface *pusb_intf,const struct usb_device_id *pdid);
static void dev_remove(struct usb_interface *pusb_intf);
#endif
static int netdev_open (struct net_device *pnetdev);
static int netdev_close (struct net_device *pnetdev);
#ifdef CONFIG_SDIO_HCI
/* devices we support, null terminated */
/* @TODO: change to your devices identification */
static SD_PNP_INFO Ids[] = {
    {.SDIO_ManufacturerID = 0x0000,    /* 871x SDIO card */
     .SDIO_ManufacturerCode = 0x024c,
     .SDIO_FunctionNo = 1,
     .SDIO_FunctionClass = 7},   
    {}
};
#endif


#ifdef CONFIG_USB_HCI
#if  defined(CONFIG_RTL8192U)
static struct usb_device_id rtl871x_usb_id_tbl[] ={
	{USB_DEVICE(0x0BDA,0x8192)},
	{USB_DEVICE(0x0BDA,0x8709)},
	{}
};
#elif defined(CONFIG_RTL8187)
static struct usb_device_id rtl871x_usb_id_tbl[] ={
	{USB_DEVICE(0x0BDA,0x8711)},
	{USB_DEVICE(0x0BDA,0x8187)},	
	{USB_DEVICE(0x0BDA,0x8189)},
	{}
};
#endif
#endif


typedef struct _driver_priv{
#ifdef CONFIG_SDIO_HCI
	SDFUNCTION sdiofunc;	
#endif
#ifdef CONFIG_USB_HCI
	struct usb_driver devfunc;
#endif
}drv_priv, *pdrv_priv;

static drv_priv drvpriv = {	
	#ifdef CONFIG_SDIO_HCI
	.sdiofunc.pName = "871x_sdio_drv",
	.sdiofunc.Version = CT_SDIO_STACK_VERSION_CODE,
	.sdiofunc.MaxDevices = 2,
	.sdiofunc.NumDevices = 0,
	.sdiofunc.pIds = Ids,
	.sdiofunc.pProbe = drv_init,
	.sdiofunc.pRemove = dev_remove,
	.sdiofunc.pSuspend = NULL,
      .sdiofunc.pResume  = NULL,
      .sdiofunc.pWake    = NULL,
      .sdiofunc.pContext = &drvpriv,
	#endif
	#ifdef CONFIG_USB_HCI
	.devfunc.name="871x_usb_drv",
	.devfunc.id_table=rtl871x_usb_id_tbl,
	.devfunc.probe=drv_init,
	.devfunc.disconnect=dev_remove,
	.devfunc.suspend=NULL,
	.devfunc.resume=NULL,
	#endif
};	

#if 0
void init_tx_fw_priv(_adapter* Adapter)
	// 	Allocate for 4 bytes alignment
	//
	{
	TX_FW_FRAME * ptxfwframe;

#define NR_TX_FW_FRAME 5
		int frame_index;

		_spinlock_init(&Adapter->tx_fw_lock);
		_spinlock_init(&Adapter->rf_write_lock);
		
		_init_queue(&Adapter->pending_tx_fw_cmd_queue);
		_init_queue(&Adapter->free_tx_fw_cmd_queue);

		

		DEBUG_INFO(("TX FW FRAME size: %d\n", sizeof(TX_FW_FRAME)));
		Adapter->tx_fw_cmd_list_buf = _malloc(NR_TX_FW_FRAME*sizeof(TX_FW_FRAME)+4);
		if (Adapter->tx_fw_cmd_list_buf  == NULL)
		{
			DEBUG_ERR(("Memory allocation failed.\n"));
		}

		Adapter->tx_fw_cmd_list_buf = Adapter->tx_fw_cmd_list_buf + 4 -((uint) (Adapter->tx_fw_cmd_list_buf ) &3);
		ptxfwframe = (TX_FW_FRAME *) Adapter->tx_fw_cmd_list_buf;
	
		for(frame_index=0; frame_index < NR_TX_FW_FRAME ; frame_index++)
		{
			_init_listhead(&(ptxfwframe->list));

			ptxfwframe->ptxfw_urb = usb_alloc_urb(0,GFP_KERNEL); 
			if(ptxfwframe->ptxfw_urb == NULL)
				DEBUG_ERR(("ptxfwframe->ptxfw_urb== NULL"));

			ptxfwframe->padapter = Adapter;
			list_insert_tail( &(ptxfwframe->list), &(Adapter->free_tx_fw_cmd_queue.queue ));

			ptxfwframe++;
		}
	}

#endif

struct net_device *init_netdev(void)	
{
	_adapter *padapter;
	struct net_device *pnetdev;

	pnetdev = alloc_etherdev(sizeof(_adapter));
	//pnetdev = alloc_netdev(sizeof(_adapter), "wlan%d", ether_setup);
	if (!pnetdev)
	   return NULL;

#ifdef LINUX_24                                 
	SET_MODULE_OWNER(pnetdev);
#endif

	ether_setup(pnetdev);
	
	padapter = netdev_priv(pnetdev);

// kovich add
//	init_tx_fw_priv(padapter);

	padapter->pnetdev = pnetdev;	
	
	//pnetdev->init = NULL;
	pnetdev->open = netdev_open;
	pnetdev->stop = netdev_close;	

	pnetdev->hard_start_xmit = xmit_entry;
	pnetdev->tx_timeout = NULL;
	pnetdev->watchdog_timeo = HZ; /* 1 second timeout */	
	//pnetdev->set_mac_address = NULL;	   
	//pnetdev->change_mtu = NULL;
	//pnetdev->set_multicast_list = NULL;
	//pnetdev->get_stats = NULL;
	//pnetdev->type=ARPHRD_ETHER;	
	pnetdev->do_ioctl=r8711_ioctl;
	//pnetdev->ethtool_ops = &orinoco_ethtool_ops;
	pnetdev->wireless_handlers = (struct iw_handler_def *)&r8711_handlers_def;  
#ifdef WIRELESS_SPY
	//priv->wireless_data.spy_data = &priv->spy_data;
	//pnetdev->wireless_data = &priv->wireless_data;
#endif
	
	if(dev_alloc_name(pnetdev,"wlan%d") < 0)
	{
		DEBUG_ERR(("dev_alloc_name, fail! \n"));		
	}

	netif_carrier_off(pnetdev);
	netif_stop_queue(pnetdev);
	
	return pnetdev;

}

static u32 start_drv_threads(_adapter *padapter) {

	u32 _status = _SUCCESS;

#ifdef LINUX_TX_TASKLET
	tasklet_init(&padapter->irq_tx_tasklet,
		     (void(*)(unsigned long))xmit_tasklet,
		     (unsigned long)padapter);
#else
	padapter->xmitThread = kernel_thread(xmit_thread, padapter, CLONE_FS|CLONE_FILES);
	if(padapter->xmitThread < 0)
		_status = _FAIL;
#endif


#ifdef LINUX_TASKLET
	tasklet_init(&padapter->irq_rx_tasklet,
		     (void(*)(unsigned long))recv_tasklet,
		     (unsigned long)padapter);
#else    

	padapter->recvThread = kernel_thread(recv_thread, padapter, CLONE_FS|CLONE_FILES);
	if(padapter->recvThread < 0)
		_status = _FAIL;
#endif

	padapter->cmdThread = kernel_thread(cmd_thread, padapter, CLONE_FS|CLONE_FILES);
	if(padapter->cmdThread < 0)
		_status = _FAIL;		

#ifndef DISABLE_EVENT_THREAD
	padapter->evtThread = kernel_thread(event_thread, padapter, CLONE_FS|CLONE_FILES);
	if(padapter->evtThread < 0)
		_status = _FAIL;
#endif /*DISABLE_EVENT_THREAD*/

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	_init_sema(&padapter->terminate_mlmethread_sema, 0);

	padapter->mlmeThread = kernel_thread(mlme_thread, padapter, CLONE_FS|CLONE_FILES);
	if(padapter->mlmeThread < 0)
		_status = _FAIL;		

#endif /*CONFIG_RTL8187*/

#ifdef ENABLE_MGNT_THREAD
	padapter->mgntThread = kernel_thread(mgnt_thread, padapter, CLONE_FS|CLONE_FILES);
	if(padapter->recvThread < 0)
		_status = _FAIL;
#endif

    return _status;

}
void stop_drv_threads (_adapter *padapter) {

        u8	 bool;
		
	_up_sema(&padapter->cmdpriv.cmd_queue_sema);

	_up_sema(&padapter->cmdpriv.cmd_done_sema);

	_up_sema(&padapter->evtpriv.evt_notify);

	_down_sema(&padapter->cmdpriv.terminate_cmdthread_sema);

#ifndef DISABLE_EVENT_THREAD
	_down_sema(&padapter->evtpriv.terminate_evtthread_sema);
#endif
	_set_timer(&padapter->mlmepriv.assoc_timer, 10000);
	while(1)
	{		
		_cancel_timer(&padapter->mlmepriv.assoc_timer, &bool);
		if (bool == _TRUE)
			break;
	}
	DEBUG_INFO(("\n drv_halt:cancel association timer complete! \n"));
	
	while(1)
	{		
		_cancel_timer(&padapter->mlmepriv.sitesurveyctrl.sitesurvey_ctrl_timer, &bool);
		if (bool == _TRUE)
			break;
	}
	DEBUG_INFO(("\n drv_halt:cancel sitesurvey timer complete! \n"));
#ifndef LINUX_TX_TASKLET
	// Below is to termindate tx_thread...
	_up_sema(&padapter->xmitpriv.xmit_sema);	
	_down_sema(&padapter->xmitpriv.terminate_xmitthread_sema);	
	DEBUG_INFO(("\n drv_halt: xmit_thread can be terminated ! \n"));
#else
	tasklet_kill(&padapter->irq_tx_tasklet);
#endif

#ifndef LINUX_TASKLET
	//Below is to termindate rx_thread...
	_up_sema(&padapter->recvpriv.recv_sema);	
	_down_sema(&padapter->recvpriv.terminate_recvthread_sema);
	DEBUG_INFO(("\n drv_halt:recv_thread can be terminated! \n"));
#else

	tasklet_kill(&padapter->irq_rx_tasklet);
#endif

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	_up_sema(&padapter->cmdpriv.cmd_proc_sema);	
	_down_sema(&padapter->terminate_mlmethread_sema);
	DEBUG_INFO(("\n drv_halt:mlme_thread can be terminated! \n"));
#endif /*CONFIG_RTL8187*/

}
uint loadparam( _adapter *padapter,  _nic_hdl	pnetdev)
{
       
	uint status = _SUCCESS;
	struct registry_priv  *registry_par = &padapter->registrypriv;

_func_enter_;
#ifdef CONFIG_RTL8711
	registry_par->chip_version = (u8)chip_version;
#endif
	registry_par->nic_version = (u8)nic_version;
	registry_par->rfintfs = (u8)rfintfs;
	registry_par->lbkmode = (u8)lbkmode;	
	registry_par->hci = (u8)hci;
	registry_par->network_mode  = (u8)network_mode;	

     	_memcpy(registry_par->ssid.Ssid, "ANY", 3);
	//registry_par->ssid.Ssid  = "ANY";
	registry_par->ssid.SsidLength = 32;
	
	registry_par->channel = (u8)channel;
	registry_par->wireless_mode = (u8)wireless_mode;
	registry_par->vrtl_carrier_sense = (u8)vrtl_carrier_sense ;
	registry_par->rts_thresh=(u16)rts_thresh;	
	registry_par->frag_thresh=(u16)frag_thresh;
	registry_par->preamble = (u8)preamble;
	registry_par->scan_mode = (u8)scan_mode;
	registry_par->adhoc_tx_pwr = (u8)adhoc_tx_pwr;
	registry_par->soft_ap=  (u8)soft_ap;
	registry_par->smart_ps =  (u8)smart_ps;  
	registry_par->power_mgnt = (u8)power_mgnt;
	registry_par->radio_enable = (u8)radio_enable;
	registry_par->long_retry_lmt = (u8)long_retry_lmt;
	registry_par->short_retry_lmt = (u8)short_retry_lmt;
  	registry_par->busy_thresh = (u16)busy_thresh;
  	//registry_par->qos_enable = (u8)qos_enable;
    	registry_par->ack_policy = (u8)ack_policy;
	registry_par->mp_mode = (u8)mp_mode;	
	registry_par->software_encrypt = (u8)software_encrypt;
	registry_par->software_decrypt = (u8)software_decrypt;	  

	 //UAPSD
	registry_par->wmm_enable = (u8)wmm_enable;
	registry_par->uapsd_enable = (u8)uapsd_enable;	  
	registry_par->uapsd_max_sp = (u8)uapsd_max_sp;
	registry_par->uapsd_acbk_en = (u8)uapsd_acbk_en;
	registry_par->uapsd_acbe_en = (u8)uapsd_acbe_en;
	registry_par->uapsd_acvi_en = (u8)uapsd_acvi_en;
	registry_par->uapsd_acvo_en = (u8)uapsd_acvo_en;


_func_exit_;

	return status;


}
static u8 init_drv_sw(_adapter *padapter){
	
	u8		ret8=_SUCCESS;
	struct	io_queue		*pio_queue = NULL;
	
	_func_enter_;

	DEBUG_ERR(("+init_drv_sw:\n"));

	//adapter init	
	padapter->bDriverStopped = _FALSE;
	padapter->bSurpriseRemoved = _FALSE;
	padapter->bup = _FALSE;
	
	_spinlock_init(&padapter->rf_write_lock);

#ifdef LINUX_TASKLET
	DEBUG_INFO(("init_drv_sw: rx_thread_complelte = FALSE\n"));
	padapter->rx_thread_complete = _FALSE;
#endif
	if ((init_cmd_priv(&padapter->cmdpriv)) == _FAIL)
	{
			DEBUG_ERR(("\n Can't init cmd_priv\n"));
			ret8=_FAIL;
			goto exit;
	}
	padapter->cmdpriv.padapter=padapter;

	
	if ((init_evt_priv(&padapter->evtpriv)) == _FAIL)
	{
			DEBUG_ERR(("\n Can't init evt_priv\n"));
			ret8=_FAIL;
			goto exit;
	}
	
	
	if (init_mlme_priv(padapter) == _FAIL)
	{
			DEBUG_ERR(("\n Can't init mlme_priv\n"));
			ret8=_FAIL;
			goto exit;
	}



	pio_queue = (struct io_queue *)padapter->pio_queue;
	

	if (_init_xmit_priv(&padapter->xmitpriv, padapter) == _FAIL)
	{
			DEBUG_ERR(("\n Can't init _xmit_priv\n"));
			ret8=_FAIL;
			goto exit;
	}

	if (_init_recv_priv(&padapter->recvpriv, padapter) == _FAIL)
	{
			DEBUG_ERR(("\n Can't init _recv_priv\n"));
			ret8=_FAIL;
			goto exit;
	}

	// Init HT related Info  for 8192
	#ifdef CONFIG_RTL8192U
	
		TSInitialize(padapter);
		HTInitializeHTInfo(padapter);

	#endif

	

	memset((unsigned char *)&padapter->securitypriv, 0, sizeof (struct security_priv));

	_init_sta_priv(&padapter->stapriv);

	init_bcmc_stainfo(padapter);

	init_pwrctrl_priv(padapter);

	memset((u8 *)&padapter->qospriv, 0, sizeof (struct qos_priv));

	DEBUG_INFO(("address of pio_queue:%x, intf_hdl:%x\n", 
		(uint)padapter->pio_queue, (uint)&(pio_queue->intf) ));


	//DEBUG_INFO((" \n\n\n##########padapter->hndis_adapter:%x, address of adapter:%x##########\n\n\n", 
	//			padapter->hndis_adapter, &padapter));


	//NdisMSetPeriodicTimer(&padapter->mlmepriv.sitesurveyctrl.sitesurvey_ctrl_timer, traffic_scan_period);

#ifdef CONFIG_MP_INCLUDED
        mp8711init(padapter); 
#endif
exit:
	_func_exit_;	
	return ret8;
}

static u8 free_drv_sw(_adapter *padapter)
{
	#ifdef CONFIG_SDIO_HCI
       PSDFUNCTION psdfunc = &drvpriv.sdiofunc;
	#endif
	struct net_device *pnetdev = (struct net_device*)padapter->pnetdev;		
	DEBUG_ERR(("+free_drv_sw\n"));
	
	padapter->recvpriv.counter--;

#if 0
	if( padapter->recvpriv.counter >0 )
      		NdisWaitEvent( &padapter->recvpriv.recv_resource_evt, 0);
#endif	
		
	if(padapter->recvpriv.counter==0)
	{
		_free_recv_priv(&padapter->recvpriv); 	
	}
	else
	{
		_free_recv_priv(&padapter->recvpriv);
		DEBUG_ERR(("SdioHalt():failed to free_recv_priv!\n"));	
	}

#ifdef CONFIG_RTL8192U
	//TSDeInitialize(padapter);
#endif

	if(padapter->dvobjpriv.dvobj_deinit==NULL){
		DEBUG_ERR(("Initialize hcipriv.hci_priv_init error!!!\n"));
	}else{
		padapter->dvobjpriv.dvobj_deinit(padapter);
	}	
	DEBUG_ERR(("free_drv_sw: free_cmd_priv\n"));
	
       free_cmd_priv(&padapter->cmdpriv);
	DEBUG_ERR(("free_drv_sw: free_evt_priv\n"));
	
	free_evt_priv(&padapter->evtpriv);
	DEBUG_ERR(("free_drv_sw: free_mlme_priv\n"));
	
	free_mlme_priv(&padapter->mlmepriv);

	DEBUG_ERR(("free_drv_sw: free_io_queue\n"));

	free_io_queue(padapter);
	DEBUG_ERR(("free_drv_sw: free_xmit_priv\n"));
	
	_free_xmit_priv(&padapter->xmitpriv);
	DEBUG_ERR(("free_drv_sw: free_sta_priv\n"));
	
	_free_sta_priv(&padapter->stapriv); // will free bcmc_stainfo here

	//_mfree((void *)padapter, sizeof (padapter));//******

	DEBUG_ERR(("free_drv_sw: unregister_netdev\n"));
	
	if(pnetdev)
	{	      	 		
		unregister_netdev(pnetdev);
		free_netdev(pnetdev);
	}

#ifdef CONFIG_SDIO_HCI
       psdfunc->pContext = NULL;
#endif

	DEBUG_ERR(("-free_drv_sw\n"));

	return _SUCCESS;
}


#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

static struct proc_dir_entry *rtl8180_proc = NULL;

static int proc_get_registers(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	_adapter *padapter = data;
		
	int len = 0;
	int i,n;
			
	int max=0xff;
	
	/* This dump the current register page */
	len += snprintf(page + len, count - len,
                        "\n####################page 0##################\n ");

	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len,
			"\nD:  %2x > ",n);

		for(i=0;i<16 && n<=max;i++,n++)
		len += snprintf(page + len, count - len,
			"%2x ",read8(padapter, n));
	}
	len += snprintf(page + len, count - len,"\n");
	len += snprintf(page + len, count - len,
                        "\n####################page 1##################\n ");

    for(n=0;n<=max;)
    {
            len += snprintf(page + len, count - len,
                    "\nD:  %2x > ",n);

            for(i=0;i<16 && n<=max;i++,n++)
            len += snprintf(page + len, count - len,
                    "%2x ", read8(padapter, 0x100|n));
    }
		
	len += snprintf(page + len, count - len,
                        "\n####################page 2##################\n ");
	
    for(n=0;n<=max;)
    {
            len += snprintf(page + len, count - len,
                    "\nD:  %2x > ",n);

            for(i=0;i<16 && n<=max;i++,n++)
            len += snprintf(page + len, count - len,
                    "%2x ", read8(padapter, 0x200|n));
    }

	len += snprintf(page + len, count - len,
                        "\n");

		
	*eof = 1;
	return len;

}

static int proc_get_rf1(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	_adapter *padapter = data;
		
	int len = 0;
	int i, j,x;
			
	int max=0xff;

	for(j=8, x=0; j<10 ; j++)
	{
		/* This dump the current register page */
		len += snprintf(page + len, count - len,
	                        "\n####################page %x##################\n", j);

		for(i=0 ; (i<<2)<max ; i++)
		{
			len += snprintf(page + len, count - len,
				"0x%3x 0x%08x \n", j<<8|(i<<2), read32(padapter, j<<8|(i<<2) ) );
		}

		len += snprintf(page + len, count - len,"\n");
	}

	len += snprintf(page + len, count - len, "\n");			
		
	*eof = 1;
	return len;

}

static int proc_get_rf2(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	_adapter *padapter = data;
		
	int len = 0;
	int i, j,x;
			
	int max=0xff;
	

	for(j=10, x=0; j<12 ; j++)
	{
		/* This dump the current register page */
		len += snprintf(page + len, count - len,
	                        "\n####################page %x##################\n", j);

		for(i=0 ; (i<<2)<max ; i++)
		{
			len += snprintf(page + len, count - len,
				"0x%3x 0x%08x \n", j<<8|(i<<2), read32(padapter, j<<8|(i<<2) ) );
		}

		len += snprintf(page + len, count - len,"\n");
	}

	len += snprintf(page + len, count - len, "\n");			
		
	*eof = 1;
	return len;

}

static int proc_get_rf3(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	_adapter *padapter = data;
		
	int len = 0;
	int i, j,x;
			
	int max=0xff;
	

	for(j=12, x=0; j<14 ; j++)
	{
		/* This dump the current register page */
		len += snprintf(page + len, count - len,
	                        "\n####################page %x##################\n", j);

		for(i=0 ; (i<<2)<max ; i++)
		{
			len += snprintf(page + len, count - len,
				"0x%3x 0x%08x \n", j<<8|(i<<2), read32(padapter, j<<8|(i<<2) ) );
		}

		len += snprintf(page + len, count - len,"\n");
	}

	len += snprintf(page + len, count - len, "\n");			
		
	*eof = 1;
	return len;

}

static int proc_get_rf4(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	_adapter *padapter = data;
		
	int len = 0;
	int i, j,x;
			
	int max=0xff;
	

	for(j=14, x=0; j<16 ; j++)
	{
		/* This dump the current register page */
		len += snprintf(page + len, count - len,
	                        "\n####################page %x##################\n", j);

		for(i=0 ; (i<<2)<max ; i++)
		{
			len += snprintf(page + len, count - len,
				"0x%3x 0x%08x \n", j<<8|(i<<2), read32(padapter, j<<8|(i<<2) ) );
		}

		len += snprintf(page + len, count - len,"\n");
	}

	len += snprintf(page + len, count - len, "\n");			
		
	*eof = 1;
	return len;

}






void rtl8180_proc_remove_one(_adapter *padapter)
{
	DEBUG_ERR(("+rtl8180_proc_remove_one\n"));	
	if (padapter->dir_dev) {
		remove_proc_entry("registers", padapter->dir_dev);
		remove_proc_entry("rf1", padapter->dir_dev);		
		remove_proc_entry("rf2", padapter->dir_dev);		
		remove_proc_entry("rf3", padapter->dir_dev);		
		remove_proc_entry("rf4", padapter->dir_dev);		
		remove_proc_entry("rtl8192u", rtl8180_proc);
		padapter->dir_dev = NULL;
	}
	DEBUG_ERR(("-rtl8180_proc_remove_one\n"));	
}



void rtl8180_proc_init_one(_adapter *padapter)
{
	struct proc_dir_entry *e;
	
	padapter->dir_dev = create_proc_entry("rtl8192u", 
					  S_IFDIR | S_IRUGO | S_IXUGO, 
					  rtl8180_proc);
	if (!padapter->dir_dev) {
		DEBUG_ERR(("Unable to initialize /proc/rtl8192u\n"));
		return;
	}
	
	e = create_proc_read_entry("registers", S_IFREG | S_IRUGO,
				   padapter->dir_dev, proc_get_registers, padapter);
	if (!e) {
		DEBUG_INFO(("Unable to initialize /proc/rtl8192/registers\n"));
	}

	e = create_proc_read_entry("rf1", S_IFREG | S_IRUGO,
				   padapter->dir_dev, proc_get_rf1, padapter);
	if (!e) {
		DEBUG_INFO(("Unable to initialize /proc/rtl8192/rf\n"));
	}

	e = create_proc_read_entry("rf2", S_IFREG | S_IRUGO,
				   padapter->dir_dev, proc_get_rf2, padapter);
	if (!e) {
		DEBUG_INFO(("Unable to initialize /proc/rtl8192/rf2\n"));
	}

	e = create_proc_read_entry("rf3", S_IFREG | S_IRUGO,
				   padapter->dir_dev, proc_get_rf3, padapter);
	if (!e) {
		DEBUG_INFO(("Unable to initialize /proc/rtl8192/rf2\n"));
	}

	e = create_proc_read_entry("rf4", S_IFREG | S_IRUGO,
				   padapter->dir_dev, proc_get_rf4, padapter);
	if (!e) {
		DEBUG_INFO(("Unable to initialize /proc/rtl8192/rf2\n"));
	}	
}

#endif /* CONFIG_RTL8187 */



/*
 * drv_init() - a device potentially for us
 *
 * notes: drv_init() is called when the bus driver has located a card for us to support.
 *        We accept the new device by returning TRUE.
*/
#ifdef CONFIG_SDIO_HCI
static unsigned char drv_init(PSDFUNCTION psdfunc, PSDDEVICE psddev)
#endif
#ifdef CONFIG_USB_HCI
static int drv_init(struct usb_interface *pusb_intf,const struct usb_device_id *pdid)
#endif
{
        u8	val8;
        uint status;	 
	 _adapter *padapter = NULL;
        struct dvobj_priv *pdvobjpriv;
	struct net_device *pnetdev;


	DEBUG_ERR(("+871x - drv_init\n"));


	//step 1.
	//padapter = (_adapter *)_malloc(sizeof(_adapter));

	pnetdev = init_netdev();
	if (!pnetdev)
		goto error;	
	
	padapter = netdev_priv(pnetdev);
	pdvobjpriv = &padapter->dvobjpriv;	
	pdvobjpriv->padapter = padapter;
	#ifdef CONFIG_SDIO_HCI	
	pdvobjpriv->psdfunc = psdfunc;
	pdvobjpriv->psddev = psddev;	
	psdfunc->pContext = padapter;
	#endif
	#ifdef CONFIG_USB_HCI
	padapter->dvobjpriv.pusbdev=interface_to_usbdev(pusb_intf);
	usb_set_intfdata(pusb_intf, pnetdev);	
	SET_NETDEV_DEV(pnetdev, &pusb_intf->dev);
	#endif
	
	//step 2. 
	if(alloc_io_queue(padapter) == _FAIL)
	{
	   DEBUG_ERR(("Can't init io_reqs\n"));
	   goto error;
	}

	//step 3.

	

        status = loadparam(padapter, pnetdev);	
        if (status != _SUCCESS) {
			
	    DEBUG_ERR(("Read Parameter Failed!\n"));			
	    goto error;		
	} 


	//step 4.
	//dvobj_init(padapter);
#ifdef CONFIG_SDIO_HCI
	padapter->dvobjpriv.dvobj_init=&sd_dvobj_init;
	padapter->dvobjpriv.dvobj_deinit=&sd_dvobj_deinit;
#endif
#ifdef CONFIG_CFIO_HCI
	padapter->dvobjpriv.dvobj_init=&cf_dvobj_init;
	padapter->dvobjpriv.dvobj_deinit=&cf_dvobj_deinit;
#endif
#ifdef  CONFIG_USB_HCI
	padapter->dvobjpriv.dvobj_init=&usb_dvobj_init;
	padapter->dvobjpriv.dvobj_deinit=&usb_dvobj_deinit;
#endif
	
	if(padapter->dvobjpriv.dvobj_init ==NULL){
		DEBUG_ERR(("\n Initialize dvobjpriv.dvobj_init error!!!\n"));
		goto error;
	}else{
		status=padapter->dvobjpriv.dvobj_init(padapter);
	}	
		
	if (status != _SUCCESS) {
			
		DEBUG_ERR(("\n initialize device object priv Failed!\n"));			
		goto error;
	} 
	

	//step 4.	
	val8=init_drv_sw( padapter);
	if(val8 ==_FAIL){
		DEBUG_ERR(("Initialize driver software resource Failed!\n"));			
		goto error;
	}

	DEBUG_INFO(("EEPROM MAC Address = %x-%x-%x-%x-%x-%x\n", 
				padapter->eeprompriv.mac_addr[0],	padapter->eeprompriv.mac_addr[1],
				padapter->eeprompriv.mac_addr[2],	padapter->eeprompriv.mac_addr[3],
				padapter->eeprompriv.mac_addr[4],	padapter->eeprompriv.mac_addr[5]));

	//step 7.
	_memcpy(pnetdev->dev_addr, padapter->eeprompriv.mac_addr, ETH_ALEN);

	//step 8.
	/* Tell the network stack we exist */
	if (register_netdev(pnetdev) != 0) {
		DEBUG_ERR(("register_netdev() failed\n"));
		goto error;
	}
	
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

	rtl8180_proc_init_one(padapter);
#endif /* CONFIG_RTL8187 */

			  
    DEBUG_ERR(("-drv_init - Adapter->bDriverStopped=%d, Adapter->bSurpriseRemoved=%d\n",padapter->bDriverStopped, padapter->bSurpriseRemoved));				
    DEBUG_ERR(("-871x_drv - drv_init, success!\n"));
    return 0;

error:	
      

      if(padapter->dvobjpriv.dvobj_deinit==NULL){
		DEBUG_ERR(("\n Initialize dvobjpriv.dvobj_deinit error!!!\n"));
	}else{
		padapter->dvobjpriv.dvobj_deinit(padapter);
	} 	  

      if(pnetdev)
      {	 
          unregister_netdev(pnetdev);
          free_netdev(pnetdev);
      }

      DEBUG_ERR(("-871x_sdio - drv_init, fail!\n"));
	  
      return -1;
	  

}

/*
 * dev_remove() - our device is being removed
*/
#ifdef CONFIG_SDIO_HCI
static void dev_remove(PSDFUNCTION psdfunc, PSDDEVICE psddev)
#endif
#ifdef CONFIG_USB_HCI
static void dev_remove(struct usb_interface *pusb_intf)
#endif	
{
#ifdef CONFIG_SDIO_HCI
	struct net_device *pnetdev;	
	_adapter *padapter = (_adapter*)psdfunc->pContext;
#endif
#ifdef CONFIG_USB_HCI
	struct net_device *pnetdev=usb_get_intfdata(pusb_intf);	
	_adapter *padapter = (_adapter*)netdev_priv(pnetdev);
#endif

_func_exit_;

	if(padapter)	
	{
		DEBUG_INFO(("+dev_remove()\n"));
		pnetdev= (struct net_device*)padapter->pnetdev;	
		padapter->bDriverStopped = _TRUE;
		padapter->bSurpriseRemoved = _TRUE;	 
     
		if(pnetdev)   
		{
			netif_carrier_off(pnetdev);
			netif_stop_queue(pnetdev);
		}
		//
		netdev_close(pnetdev);
		free_drv_sw(padapter);

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
		rtl8180_proc_remove_one(padapter);
#endif 

	}
	DEBUG_INFO(("-dev_remove()\n"));

_func_exit_;
	
	return;
}

static int netdev_open(struct net_device *pnetdev)
{
	uint status;	
	_adapter *padapter = (_adapter *)netdev_priv(pnetdev);

	DEBUG_ERR(("+871x_drv - drv_open\n"));

	if(padapter->bup == _FALSE)
	{
		padapter->bDriverStopped = _FALSE;
		padapter->bSurpriseRemoved = _FALSE;	
		memset((unsigned char *)&padapter->securitypriv, 0, sizeof (struct security_priv));

		//	Step x1.
		// 		Hal Initialization
#if defined(CONFIG_RTL8711)
		status = rtl8711_hal_init(padapter);
#elif defined(CONFIG_RTL8192U)
		DEBUG_ERR(("\n call rtl8192u_hal_init\n"));
		status = rtl8192u_hal_init(padapter);
#elif defined(CONFIG_RTL8187)
		DEBUG_ERR(("\n call rtl8187_hal_init\n"));
		status = rtl8187_hal_init(padapter);
#else
	#error
#endif


#ifdef CONFIG_USB_HCI
		padapter->dvobjpriv.rxirp_init=&usb_rx_init;
		padapter->dvobjpriv.rxirp_deinit=&usb_rx_deinit;
		if((padapter->dvobjpriv.rxirp_init==NULL)||(padapter->dvobjpriv.rxirp_deinit==NULL))
		{
			DEBUG_ERR(("\n Initialize rx_init or rx_deinit error!!!\n"));
		}else{
			padapter->dvobjpriv.rxirp_init(padapter);
		}
#endif

		if (status ==_FAIL) {
			DEBUG_ERR(("\n rtl8x_hal_init(X): Can't init h/w!\n"));
			goto netdev_open_error;
		}

		//step 6.
		init_registrypriv_dev_network(padapter);
		update_registrypriv_dev_network(padapter);

		DEBUG_ERR(("@@@@ netdev_open: start_drv_threads\n"));
	
		//step x2.
		status=start_drv_threads(padapter);
		if(status ==_FAIL) {
			DEBUG_ERR(("start_drv_threads failed!\n"));			
			goto netdev_open_error;
		}

		DEBUG_ERR(("@@@@ netdev_open: get_encrypt_decrypt_from_registrypriv\n"));
		//step 8.
		//get_encrypt_decrypt_from_registrypriv(padapter);

		_memcpy(pnetdev->dev_addr, padapter->eeprompriv.mac_addr, ETH_ALEN);

		//netif_carrier_on(pnetdev);//call this func when joinbss_event_callback return success      
		if(!netif_queue_stopped(pnetdev))
			netif_start_queue(pnetdev);
		else
			netif_wake_queue(pnetdev);

		padapter->bDriverStopped = _FALSE;
		padapter->bSurpriseRemoved = _FALSE;	 
		padapter->bup = _TRUE;
	}		
			
	DEBUG_ERR(("-871x_sdio - drv_open\n"));		
	return 0;
	
netdev_open_error:

	padapter->bup = _FALSE;
	netif_carrier_off(pnetdev);
	netif_stop_queue(pnetdev);
	DEBUG_ERR(("-871x_sdio - drv_open, fail!\n"));
	return 1;
	
}

static int netdev_close(struct net_device *pnetdev)
{
	_adapter *padapter = (_adapter *)netdev_priv(pnetdev);
		
	DEBUG_INFO(("+871x_drv - drv_close\n"));

	if(padapter->bup == _TRUE)
	{
		padapter->bDriverStopped = _TRUE;
		padapter->bSurpriseRemoved = _TRUE;	 
		padapter->bup = _FALSE;
        
		netif_stop_queue(pnetdev);
		if(padapter->bSurpriseRemoved!=_TRUE){
#ifdef CONFIG_RTL8711
			//Step1. Close HIMR
			write32(padapter, HIMR, 0);	
			DEBUG_INFO(("\n drv_halt: Close HIMR \n" ));	 
#endif	
		}

		//step 2.
		stop_drv_threads(padapter);		

		DEBUG_INFO(("drv_close - hal_bus_deinit\n"));
		//step 3.
		if(padapter->halpriv.hal_bus_deinit ==NULL){
			DEBUG_ERR(("\n netdev_close - halpriv.hal_bus_deinit not defined!!!\n"));
		}
		else {
			padapter->halpriv.hal_bus_deinit(padapter);
		}

#ifdef CONFIG_USB_HCI
		DEBUG_INFO(("drv_close - rxirp_deinit\n"));

		if(padapter->dvobjpriv.rxirp_deinit ==NULL){
			DEBUG_ERR(("\n netdev_close - padapter->dvobjpriv.rxirp_deinit not defined!!!\n"));
		}
		else{
			padapter->dvobjpriv.rxirp_deinit(padapter);
		}
#endif

#if defined(CONFIG_RTL8192U)
		DEBUG_INFO(("drv_close - rtl8192u_hal_deinit\n"));

		rtl8192u_hal_deinit(padapter);
#elif defined(CONFIG_RTL8187)
		rtl8187_hal_deinit(padapter);
#endif

		DEBUG_INFO(("drv_close - indicate_disconnect\n"));
		indicate_disconnect(padapter);

		DEBUG_INFO(("drv_close - free_network_queue\n"));
		free_network_queue(padapter);
	}
	else{
		DEBUG_ERR(("\n drv_close:bup==FALSE!\n"));
	}

	DEBUG_INFO(("-871x_drv - drv_close\n"));
	return 0;
	
}
	
static int __init drv_entry(void)
{
#ifdef CONFIG_SDIO_HCI	
    SDIO_STATUS status;


    DEBUG_ERR(("+871x_drv- drv_entry\n"));


    /* register with bus driver core */
    status = SDIO_RegisterFunction(&drvpriv.sdiofunc);	


    DEBUG_ERR(("-871x_drv - drv_entry, status=%d\n", status));	

    
    return SDIOErrorToOSError(status);
#endif
#ifdef CONFIG_USB_HCI
	return usb_register(&drvpriv.devfunc);
#endif
	
}

static void __exit drv_halt(void)
{
#ifdef CONFIG_SDIO_HCI
    SDIO_STATUS status;
    struct dvobj_priv *pdvobjpriv;
    PSDFUNCTION psdfunc = &drvpriv.sdiofunc;
    _adapter *padapter = (_adapter *)psdfunc->pContext;
	
    
    DEBUG_ERR(("+871x_drv - drv_halt\n"));	
    	
    if(padapter)
    {
              pdvobjpriv = &padapter->dvobjpriv;
			  
              if(psdfunc == pdvobjpriv->psdfunc)
		     dev_remove(psdfunc, pdvobjpriv->psddev);	

     }
	
    /* unregister, this will call dev_remove() for each device */
    status = SDIO_UnregisterFunction(&drvpriv.sdiofunc);        
   
    DEBUG_ERR(("-871x_drv - drv_halt, status=%d\n", status));	
#endif
#ifdef CONFIG_USB_HCI
	usb_deregister(&drvpriv.devfunc);
#endif	
}

module_init(drv_entry);
module_exit(drv_halt);

/*
init (driver module)-> drv_entry
probe (sd device)-> drv_init(dev_init)
open (net_device) ->netdev_open
close (net_device) ->netdev_close
remove (sd device) ->dev_remove
exit (driver module)-> drv_halt
*/

