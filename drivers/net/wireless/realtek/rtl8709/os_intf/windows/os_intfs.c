

#define _OS_INTFS_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>
#include <hal_init.h>
#include <rtl871x_ioctl.h>
#ifdef CONFIG_SDIO_HCI
	#include <sdio_osintf.h>
	#include <initguid.h>

	DEFINE_GUID( GUID_SDBUS_INTERFACE_STANDARD, 0x6bb24d81L, 0xe924, 0x4825, 0xaf, 0x49, 0x3a, 0xcd, 0x33, 0xc1, 0xd8, 0x20 );
#elif CONFIG_USB_HCI
	#include <usb_osintf.h>
#elif CONFIG_CFIO_HCI
	#include <cfio_osintf.h>
#endif

#define INTERRUPT_PASSIVE 1




BOOLEAN			RegisterNDIS51;

NDIS_STATUS drv_initialize( 
    OUT PNDIS_STATUS		OpenErrorStatus,
    OUT PUINT				SelectedMediumIndex,
    IN  PNDIS_MEDIUM		MediumArray,
    IN  uint				MediumArraySize,
    IN  _nic_hdl		NdisAdapterHandle,
    IN  _nic_hdl		WrapperConfigurationContext
);





NDIS_STATUS drv_reset(
			OUT PBOOLEAN AddressingReset,
			IN _nic_hdl MiniportAdapterContext
); 

#ifdef NDIS51_MINIPORT

VOID drv_cancel_send_packets(
	IN  _nic_hdl		MiniportAdapterContext,
	IN  PVOID			CancelId
);

VOID drv_pnp_event_notify(
	IN  _nic_hdl				MiniportAdapterContext,
	IN  NDIS_DEVICE_PNP_EVENT	PnPEvent,
	IN  PVOID					InformationBuffer,
	IN  unsigned long			InformationBufferLength
);

VOID drv_shutdown(IN  _nic_hdl     MiniportAdapterContext);

#endif


u32 loadregistry(
IN  _adapter	*padapter, 
IN  _nic_hdl	WrapperConfigurationContext
);

BOOLEAN drv_checkforhang(IN _nic_hdl MiniportAdapterContext);

VOID drv_halt( IN _nic_hdl Context );	

NDIS_STRING		DefaultSSID = NDIS_STRING_CONST("ANY");
NDIS_STRING		DefaultKey = NDIS_STRING_CONST("");

MP_REG_ENTRY NICRegTable[] = 
{
//    Reg value name			  			  Required 	Type			         Offset in struct registry_priv			  	  							  Field size			Default Value		 			Min		       				Max

	{NDIS_STRING_CONST("Chip_Version"),     0,	 	  	NdisParameterInteger,	  RGTRY_OFT(chip_version),			  					  RGTRY_SZ(chip_version),			1/*ASIC*/,      			0/*FPGA*/ ,       			1/*ASIC*/},
	{NDIS_STRING_CONST("Rf_Intf_Sel"),     0,	 	  	NdisParameterInteger,	  RGTRY_OFT(rfintfs),			  					  		  RGTRY_SZ(rfintfs),				SWSI,      					SWSI ,       					HWPI},
	{NDIS_STRING_CONST("Loop_Back_Mode"),     0,	 	NdisParameterInteger,	  RGTRY_OFT(lbkmode),			  					  	  RGTRY_SZ(lbkmode),				RTL8711_AIR_TRX,      			RTL8711_AIR_TRX,       		RTL8711_BB_FW_LBK},
	{NDIS_STRING_CONST("HCI_Type"),     	0,	 		NdisParameterInteger,	  RGTRY_OFT(hci),			  					  	  RGTRY_SZ(hci),						RTL8711_SDIO,      			RTL8711_SDIO,       		RTL8711_USB},
	{NDIS_STRING_CONST("Channel"),		  0,	 	  	NdisParameterInteger,	  RGTRY_OFT(channel),			  						  RGTRY_SZ(channel),				1,      			 			1,       						161},
	{NDIS_STRING_CONST("WirelessMode"),	  0,		  	NdisParameterInteger,	  RGTRY_OFT(wireless_mode),	  							  RGTRY_SZ(wireless_mode),		WIRELESS_11BG,      	WIRELESS_INVALID,      WIRELESS_11A},
	{NDIS_STRING_CONST("VrtlCarrier"),	  	  0,		  	NdisParameterInteger,	  RGTRY_OFT(vrtl_carrier_sense),  							  RGTRY_SZ(vrtl_carrier_sense),		AUTO_VCS,      					DISABLE_VCS,      				AUTO_VCS},	
	{NDIS_STRING_CONST("VCSType"),	  	  0,		  	NdisParameterInteger,	  RGTRY_OFT(vcs_type),  							  RGTRY_SZ(vcs_type),		RTS_CTS,      					RTS_CTS,      				CTS_TO_SELF},	
	{NDIS_STRING_CONST("RTSThresh"),	  0,		  	NdisParameterInteger,	  RGTRY_OFT(rts_thresh),	  							  	  RGTRY_SZ(rts_thresh),			2347,      						0,      						2347},		
	{NDIS_STRING_CONST("FragThresh"),	  0,		  	NdisParameterInteger,	  RGTRY_OFT(frag_thresh),	  							  	  RGTRY_SZ(frag_thresh),			2346,      						256,      						2346},		
	{NDIS_STRING_CONST("Preamble"),	  	  0,		  	NdisParameterInteger,	  RGTRY_OFT(preamble),	  	  							  RGTRY_SZ(preamble),			PREAMBLE_LONG,      			PREAMBLE_LONG,      			PREAMBLE_SHORT},	
	{NDIS_STRING_CONST("ScanMode"),	         0,		  	NdisParameterInteger,	  RGTRY_OFT(scan_mode),	  	  							  RGTRY_SZ(scan_mode),			1/*active*/,      				0/*passive*/,      				1/*active*/},	
	{NDIS_STRING_CONST("AdhocTxPwr"),	  0,		  	NdisParameterInteger,	  RGTRY_OFT(adhoc_tx_pwr),	  							  RGTRY_SZ(adhoc_tx_pwr),			1,      						0,      						3},	
	{NDIS_STRING_CONST("SoftAP"),	  	  	0,		  	NdisParameterInteger,	  RGTRY_OFT(soft_ap),	  	  						  		  RGTRY_SZ(soft_ap),				0/*disable*/,      				0/*disable*/,      				1/*enable*/},	
	{NDIS_STRING_CONST("SmartPS"),	  	  	0,		  	NdisParameterInteger,	  RGTRY_OFT(smart_ps),	  	  						  	  RGTRY_SZ(smart_ps),			0/*disable*/,      				0/*disable*/,      				1/*enable*/},	
	{NDIS_STRING_CONST("PowerMgnt"),	  	  	0,		  	NdisParameterInteger,	  RGTRY_OFT(power_mgnt),	  	  							  RGTRY_SZ(power_mgnt),			PWR_CAM,      				PWR_CAM,      				PWR_VOIP },		
	{NDIS_STRING_CONST("RadioEnable"),	  0,		  	NdisParameterInteger,	  RGTRY_OFT(radio_enable),	  	  						  RGTRY_SZ(radio_enable),			1/*enable*/,      				0/*disable*/,      				1/*enable*/},	
	{NDIS_STRING_CONST("LongRetryLimit"),	  0,		  	NdisParameterInteger,	  RGTRY_OFT(long_retry_lmt),	  							  RGTRY_SZ(long_retry_lmt),		7,      						0,      						255},	
	{NDIS_STRING_CONST("ShortRetryLimit"),	  0,		  	NdisParameterInteger,	  RGTRY_OFT(short_retry_lmt),	  							  RGTRY_SZ(short_retry_lmt),		7,      						0,      						255},	
	{NDIS_STRING_CONST("BusyThresh"),	  0,		  	NdisParameterInteger,	  RGTRY_OFT(busy_thresh),	  							  	  RGTRY_SZ(busy_thresh),			40,      						1,      						500},	
	{NDIS_STRING_CONST("AckPolicy"),	 	  0,		  	NdisParameterInteger,	  RGTRY_OFT(ack_policy),	  							  	  RGTRY_SZ(ack_policy),			NORMAL_ACK,      				NORMAL_ACK,      				BLOCK_ACK},	
	{NDIS_STRING_CONST("MPMode"),	 	  	0,		  	NdisParameterInteger,	  RGTRY_OFT(mp_mode),	  							  	  RGTRY_SZ(mp_mode),			0/*normal*/,      				0/*normal*/,      				1/*MP*/},	
	{NDIS_STRING_CONST("SoftwareEncrypt"),	   	0,		  	NdisParameterInteger,	  RGTRY_OFT(software_encrypt),	  							  	  RGTRY_SZ(software_encrypt),			0/*disable*/,      				0/*disable*/,      				1/*enable*/},	
	{NDIS_STRING_CONST("SoftwareDecrypt"),	   	0,		  	NdisParameterInteger,	  RGTRY_OFT(software_decrypt),	  							  	  RGTRY_SZ(software_decrypt),			0/*disable*/,      				0/*disable*/,      				1/*enable*/},	

	//U-APSD
	{NDIS_STRING_CONST("WMMEnable"),	  	0,	  		NdisParameterInteger,	  RGTRY_OFT(wmm_enable),	  	  						  RGTRY_SZ(wmm_enable),			0/*disable*/,      				0/*disable*/,      				1/*enable*/},		
	{NDIS_STRING_CONST("UAPSDEnable"),	  	0,	  		NdisParameterInteger,	  RGTRY_OFT(uapsd_enable),	  	  						  RGTRY_SZ(uapsd_enable),			0/*disable*/,      				0/*disable*/,      				1/*enable*/},			
	{NDIS_STRING_CONST("UAPSDMaxSP"),	  	0,	  		NdisParameterInteger,	  RGTRY_OFT(uapsd_max_sp),	  	  						  RGTRY_SZ(uapsd_max_sp),		NO_LIMIT,      				NO_LIMIT,      				SIX_MSDU},		
	{NDIS_STRING_CONST("UAPSDACBKEnable"),	0,	  		NdisParameterInteger,	  RGTRY_OFT(uapsd_acbk_en),	  	  						  RGTRY_SZ(uapsd_acbk_en),		0/*disable*/,      				0/*disable*/,      				1/*enable*/},		
	{NDIS_STRING_CONST("UAPSDACBEEnable"),	0,	  		NdisParameterInteger,	  RGTRY_OFT(uapsd_acbe_en),	  	  						  RGTRY_SZ(uapsd_acbe_en),		0/*disable*/,      				0/*disable*/,      				1/*enable*/},		
	{NDIS_STRING_CONST("UAPSDACVIEnable"),	0,	  		NdisParameterInteger,	  RGTRY_OFT(uapsd_acvi_en),	  	  						  RGTRY_SZ(uapsd_acvi_en),		0/*disable*/,      				0/*disable*/,      				1/*enable*/},		
	{NDIS_STRING_CONST("UAPSDACVOEnable"),	0,	  		NdisParameterInteger,	  RGTRY_OFT(uapsd_acvo_en),	  	  						  RGTRY_SZ(uapsd_acvo_en),		0/*disable*/,      				0/*disable*/,      				1/*enable*/},		

	{NDIS_STRING_CONST("NetworkMode"),	  0,	 	  	NdisParameterInteger,	  RGTRY_OFT(network_mode),	  								  RGTRY_SZ(network_mode),	       Ndis802_11Infrastructure,   		Ndis802_11IBSS,    			Ndis802_11AutoUnknown},
	{NDIS_STRING_CONST("SSID"),			  0,		  	NdisParameterString,	  RGTRY_OFT(ssid),			  	  							  RGTRY_SZ(ssid),					(SIZE_T)&DefaultSSID,	0,							32},


};


#define NIC_NUM_REG_PARAMS (((ULONG)sizeof (NICRegTable)) / ((ULONG)sizeof(MP_REG_ENTRY)))


NDIS_STATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
{
	NDIS_STATUS						Status;
	_nic_hdl						NdisWrapperHandle;
	NDIS_MINIPORT_CHARACTERISTICS	MPChar;

	_func_enter_;
	
	// Notify the NDIS wrapper about this driver, get a NDIS wrapper handle back
	NdisMInitializeWrapper(
		&NdisWrapperHandle,
		DriverObject,
		RegistryPath,
		NULL);

	// Fill in the Miniport characteristics structure with the version numbers 
	// and the entry points for driver-supplied MiniportXxx 
	NdisZeroMemory(&MPChar, sizeof(MPChar));

	MPChar.MajorNdisVersion				= NDIS_MAJOR_VERSION;

	MPChar.MinorNdisVersion				= NDIS_MINOR_VERSION;

	MPChar.CheckForHangHandler			= drv_checkforhang;//NULL;
	MPChar.HaltHandler					= drv_halt;
	MPChar.InitializeHandler			= drv_initialize;
	MPChar.ResetHandler					= drv_reset;
	MPChar.ReturnPacketHandler				= recv_returnpacket;
	MPChar.SendHandler					= xmit_entry;
	MPChar.QueryInformationHandler			= drv_query_info;//CEQueryInformation;
	MPChar.SetInformationHandler			= drv_set_info;//CESetInformation;

#ifdef NDIS51_MINIPORT

	MPChar.CancelSendPacketsHandler		= drv_cancel_send_packets;
	MPChar.PnPEventNotifyHandler			= drv_pnp_event_notify;
	MPChar.AdapterShutdownHandler			= drv_shutdown;

#endif

	Status = NdisMRegisterMiniport(
		NdisWrapperHandle,
		&MPChar,
		sizeof(NDIS_MINIPORT_CHARACTERISTICS));

	if(Status != NDIS_STATUS_SUCCESS )
	{
		MPChar.MinorNdisVersion         = 0;
		Status = NdisMRegisterMiniport(
			NdisWrapperHandle,
			&MPChar,
			sizeof(NDIS_MINIPORT_CHARACTERISTICS));

		DEBUG_ERR(("Register as NDIS 5.0 driver Failed\n"));
		RegisterNDIS51 = _FALSE;
	}
	else
		RegisterNDIS51 = _TRUE;

	_func_exit_;

	return Status;
}

#ifdef PLATFORM_OS_XP
u32 xp_start_drv_threads(_adapter *padapter);
#endif

static u32 start_drv_threads(_adapter *padapter) {

      u32 _status;

#ifdef PLATFORM_OS_XP

	_status=xp_start_drv_threads(padapter);

#endif


#ifdef PLATFORM_OS_CE

	_status=ce_start_drv_threads(padapter);

#endif

    return _status;


}

/*! \brief Stop all System Threads define by RTL8711 Driver. This function is just wait for all Threads' terminations. */
VOID stop_drv_threads (_adapter *padapter) {

	_up_sema(&padapter->cmdpriv.cmd_queue_sema);

	_up_sema(&padapter->cmdpriv.cmd_done_sema);
	_up_sema(&padapter->evtpriv.evt_notify);

	_down_sema(&padapter->cmdpriv.terminate_cmdthread_sema);
	_down_sema(&padapter->evtpriv.terminate_evtthread_sema);

	// Below is to termindate tx_thread...
	DEBUG_ERR(("\n stop_drv_threads:xmit_sema! \n"));
	_up_sema(&padapter->xmitpriv.xmit_sema);	
	_down_sema(&padapter->xmitpriv.terminate_xmitthread_sema);	
	DEBUG_ERR(("\n stop_drv_threads: xmit_thread can be terminated ! \n"));

	 //Below is to termindate rx_thread...
	 DEBUG_ERR(("\n drv_halt:recv_sema! \n"));
	_up_sema(&padapter->recvpriv.recv_sema);	
	_down_sema(&padapter->recvpriv.terminate_recvthread_sema);
	DEBUG_ERR(("\n stop_drv_threads:recv_thread can be terminated! \n"));

	_up_sema(&padapter->mlmepriv.mgnt_sema);
	DEBUG_ERR(("\n stop_drv_threads:recv_thread can be terminated! \n"));

}

static u8 init_drv_sw(_adapter *padapter){
	u8		ret8=_SUCCESS;
	struct	io_queue		*pio_queue = NULL;
	_func_enter_;

#ifdef CONFIG_USB_HCI
	if ( (alloc_io_queue(padapter)) == _FAIL)
	{
			DEBUG_ERR((" \n Can't init io_reqs\n"));
			ret8 = _FAIL;	
			goto exit;
	}
	_init_sema(&(padapter->usb_suspend_sema), 0);
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
	
	_init_xmit_priv(&padapter->xmitpriv, padapter);
		
	_init_recv_priv(&padapter->recvpriv, padapter);

	memset((unsigned char *)&padapter->securitypriv, 0, sizeof (struct security_priv));

	_init_sta_priv(&padapter->stapriv);

	init_bcmc_stainfo(padapter);

	init_pwrctrl_priv(padapter);

	//_init_qos_priv();

	memset((u8 *)&padapter->qospriv, 0, sizeof (struct qos_priv));

	DEBUG_INFO(("address of pio_queue:%x, intf_hdl:%x\n", 
		(uint)padapter->pio_queue, (uint)&(pio_queue->intf) ));


	DEBUG_INFO((" \n\n\n##########padapter->hndis_adapter:%x, address of adapter:%x##########\n\n\n", 
				padapter->hndis_adapter, &padapter));


	NdisMSetPeriodicTimer(&padapter->mlmepriv.sitesurveyctrl.sitesurvey_ctrl_timer, traffic_scan_period);
	
	_init_sema(&(padapter->pwrctrlpriv.pnp_pwr_mgnt_sema), 0);
		
#ifdef CONFIG_MP_INCLUDED
        mp8711init(padapter); 
#endif
exit:
	_func_exit_;	
	return ret8;
}

#define USBD_HALTED(Status) ((ULONG)(Status) >> 30 == 3)
static u8 deinit_drv_sw(_adapter *padapter){

	u8		ret8=_SUCCESS;
	u8	 bool;
	struct	io_queue		*pio_queue = NULL;
	_func_enter_;
	DEBUG_ERR(("==>deinit_drv_sw"));
	free_cmd_priv(&padapter->cmdpriv);
	
	free_evt_priv(&padapter->evtpriv);

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

	
	free_mlme_priv(&padapter->mlmepriv);
	
	free_io_queue(padapter);
	
	_free_xmit_priv(&padapter->xmitpriv);
	
	_free_sta_priv(&padapter->stapriv); // will free bcmc_stainfo here

	

	padapter->recvpriv.counter--;

      if( padapter->recvpriv.counter >0 )
      		NdisWaitEvent( &padapter->recvpriv.recv_resource_evt, 0);
		
	if(padapter->recvpriv.counter==0)
		_free_recv_priv(&padapter->recvpriv); 	
	else
		DEBUG_ERR(("failed to free_recv_priv!\n"));	

	DEBUG_ERR(("<==deinit_drv_sw"));

	_func_exit_;	
	return ret8;
}

NDIS_STATUS drv_initialize(
    OUT PNDIS_STATUS		OpenErrorStatus,
    OUT PUINT				SelectedMediumIndex,
    IN  PNDIS_MEDIUM		MediumArray,
    IN  uint				MediumArraySize,
    IN  _nic_hdl		NdisAdapterHandle,
    IN  _nic_hdl		WrapperConfigurationContext
    )
{


	u32 	index;

	_adapter *padapter = NULL;

	u32 _status;
	
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;

	
	_func_enter_;
	DEBUG_ERR(("\n===>drv_initialize!!!\n"));
	do
	{
		for (index = 0; index < MediumArraySize; ++index){
			if (MediumArray[index] == NdisMedium802_3) {
				break;
			}
		}

		if (index == MediumArraySize){
			DEBUG_ERR(("medium 802.3 not found!\n"));
			status = NDIS_STATUS_UNSUPPORTED_MEDIA;
			break;
		}

		*SelectedMediumIndex = index;
		
		padapter = (_adapter *)_malloc(sizeof(_adapter));
		
		if (!padapter) {
			
			DEBUG_ERR(("Can't alloc adapter for driver\n"));
			status = NDIS_STATUS_FAILURE;	
			break;

		}	

		memset((u8 *)padapter, 0, sizeof(_adapter));

		padapter->hndis_adapter=NdisAdapterHandle;
		padapter->hndis_config=WrapperConfigurationContext;

		//
		// NdisMSetAttributes will associate our adapter handle with the wrapper's
		// adapter handle.  The wrapper will then always use our handle
		// when calling us.  We use a pointer to the usbdevice as the context.
		//
		// An NDIS-WDM miniport driver CANNOT be an intermediate driver - 
		// so it should NOT set NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER flag.
		//
		
		NdisMSetAttributesEx(	NdisAdapterHandle,
			(_nic_hdl)padapter,
			2,	//CheckForHangTimeInSeconds
		//	NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT |
			NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
			//NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER |
	//		NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND |
			NDIS_ATTRIBUTE_DESERIALIZE,
			(NDIS_INTERFACE_TYPE)100);


		memset((u8 *)&padapter->dvobjpriv, 0, sizeof(struct dvobj_priv));
		//register hci initialize function depend on register
#ifdef CONFIG_SDIO_HCI
	if ( (alloc_io_queue(padapter)) == _FAIL)
	{
			DEBUG_ERR((" \n Can't init io_reqs\n"));
			status = NDIS_STATUS_FAILURE;	
			goto error;
	}
#endif

#ifdef CONFIG_SDIO_HCI
			padapter->dvobjpriv.dvobj_init=&sd_dvobj_init;
			padapter->dvobjpriv.dvobj_deinit=&sd_dvobj_deinit;
#elif CONFIG_CFIO_HCI
			padapter->dvobjpriv.dvobj_init=&cf_dvobj_init;
			padapter->dvobjpriv.dvobj_deinit=&cf_dvobj_deinit;
#elif  CONFIG_USB_HCI
			padapter->dvobjpriv.dvobj_init=&usb_dvobj_init;
			padapter->dvobjpriv.dvobj_deinit=&usb_dvobj_deinit;
#endif


		_status= loadregistry(padapter, WrapperConfigurationContext);
		
		if (_status != _SUCCESS) {
			
			DEBUG_ERR(("Read Registry Parameter Failed!\n"));			
			status = NDIS_STATUS_FAILURE;	
			break;
		} 

		//initialize the dvobj_priv 
		//to do 
		if(padapter->dvobjpriv.dvobj_init ==NULL){
			DEBUG_ERR(("\n Initialize dvobjpriv.dvobj_init error!!!\n"));
			goto error;
		}else{
			status=padapter->dvobjpriv.dvobj_init(padapter);
			if (status != NDIS_STATUS_SUCCESS) {
				
				DEBUG_ERR(("\n initialize device object priv Failed!\n"));			
				goto error;
			} 
		}
	
		
		
		_status = init_drv_sw( padapter);
	
		if(_status ==_FAIL) {
			DEBUG_ERR(("Initialize driver software resource Failed!\n"));			
			status = NDIS_STATUS_FAILURE;	
			break;
					
		}
			
		
		_status = rtl8711_hal_init(padapter);
		
		if (_status ==_FAIL) {

			DEBUG_ERR(("\n rtl8711_hal_init(X): Can't init h/w!\n"));
			status = NDIS_STATUS_FAILURE;	
			goto error;
		} 

		init_registrypriv_dev_network(padapter);
		
		update_registrypriv_dev_network(padapter);
		
		_status=start_drv_threads(padapter);
		if(_status ==_FAIL) {
			DEBUG_ERR(("Initialize driver software resource Failed!\n"));			
			status = NDIS_STATUS_FAILURE;	
			break;

		}

		get_encrypt_decrypt_from_registrypriv(padapter);

		
	} while (_FALSE);


	if (padapter &&  (status != NDIS_STATUS_SUCCESS))
	{
		DEBUG_ERR(("\n drv_initialize(X): Can't init h/w!\n"));
		
		goto error;
	}


#ifdef CONFIG_USB_HCI
	padapter->dvobjpriv.rxirp_init=&usb_rx_init;
	padapter->dvobjpriv.rxirp_deinit=&usb_rx_deinit;
	if((padapter->dvobjpriv.rxirp_init ==NULL)||(padapter->dvobjpriv.rxirp_deinit ==NULL)){
		DEBUG_ERR(("Initialize dvobjpriv.rxirp_init or dvobjpriv.rxirp_deinit error!!!\n"));
		goto error;
	}else{
		status=padapter->dvobjpriv.rxirp_init(padapter);
	}
			
#endif
	
#ifdef CONFIG_PWRCTRL
	DEBUG_INFO(("\n ========= Initialize Power Mode. \n"));
	ps_mode_init(padapter, padapter->registrypriv.power_mgnt, padapter->registrypriv.smart_ps);
#endif	
	
	//john
//	init_scsibuf(padapter);		//move to xmit_init

_func_exit_;
	DEBUG_ERR(("\n<===drv_initialize!!!\n"));

	return status;

	
error:
	
	status = NDIS_STATUS_FAILURE;
	DbgPrint("driver initialize fail!!!!!!!");
	if(padapter->dvobjpriv.dvobj_deinit==NULL){
		DEBUG_ERR(("\n Initialize dvobjpriv.dvobj_deinit error!!!\n"));
	}else{
			padapter->dvobjpriv.dvobj_deinit(padapter);
	}
	deinit_drv_sw(padapter);
	_mfree((void *)padapter, sizeof (padapter));
	
	_func_exit_;
	
	return status;
}



VOID drv_halt( IN _nic_hdl Context )
{
	u8	val8;
	u32 	val32;
	u8	 bool;
	_adapter	*padapter = (_adapter *)Context;
	
	_func_exit_;
	
	DbgPrint("\n===>drv_halt!!!\n");
	DEBUG_ERR(("\n===>drv_halt!!!\n"));
	if(padapter->bSurpriseRemoved == _TRUE){
		DEBUG_ERR(("padapter->bSurpriseRemoved "));
		DbgPrint("\ndrv_halt:padapter->bSurpriseRemoved \n");
		deinit_drv_sw(padapter);
	_mfree((void *)padapter, sizeof (padapter));
	goto exit;

	}
				
       padapter->bDriverStopped = _TRUE;
#ifdef CONFIG_USB_HCI
	if(padapter->dvobjpriv.rxirp_deinit !=NULL){	
		padapter->dvobjpriv.rxirp_deinit(padapter);
	}
#endif

	//4 Step1. Close HIMR
	write32(padapter, HIMR, 0);
	DEBUG_INFO(("\n drv_halt: Close HIMR \n" ));

	_up_sema(&padapter->cmdpriv.cmd_queue_sema);

	_up_sema(&padapter->cmdpriv.cmd_done_sema);

	_up_sema(&padapter->evtpriv.evt_notify);

	_down_sema(&padapter->cmdpriv.terminate_cmdthread_sema);
	_down_sema(&padapter->evtpriv.terminate_evtthread_sema);
	//4 Below is to termindate tx_thread...

	_up_sema(&padapter->xmitpriv.xmit_sema);
	
	_down_sema(&padapter->xmitpriv.terminate_xmitthread_sema);
	DEBUG_INFO(("\n drv_halt: xmit_thread can be terminated ! \n"));

	 //4 Below is to termindate rx_thread...

	_up_sema(&padapter->recvpriv.recv_sema);
	
	_down_sema(&padapter->recvpriv.terminate_recvthread_sema);
	DEBUG_INFO(("\n drv_halt:recv_thread can be terminated! \n"));

	rtl8711_hal_deinit(padapter);
	if(padapter->dvobjpriv.dvobj_deinit==NULL){
		DEBUG_ERR(("Initialize hcipriv.hci_priv_init error!!!\n"));
	}else{
			padapter->dvobjpriv.dvobj_deinit(padapter);
	}
	deinit_drv_sw(padapter);
//	deinit_scsibuf(padapter);
	_mfree((void *)padapter, sizeof (padapter));
exit:
	_func_exit_;
	DEBUG_ERR(("\n<===drv_halt!!!\n"));
	DbgPrint("\n<===drv_halt!!!\n");	
	return;

}

NDIS_STATUS
drv_reset(
OUT PBOOLEAN AddressingReset,
IN _nic_hdl MiniportAdapterContext
) 
{
	NDIS_STATUS			status	= NDIS_STATUS_SUCCESS;
	_adapter *padapter = (_adapter *)MiniportAdapterContext;
	//PCE_SDIO_DEVICE	sddev	= &padapter->NdisCEDev;

	_func_enter_;
	drv_halt(padapter);

	//2TO DO here


	_func_exit_;

	return status;
}

#define consider_bm_ff 1

#if consider_bm_ff
#define HWXMIT_ENTRY	5
#else
#define HWXMIT_ENTRY	4
#endif

#ifdef NDIS51_MINIPORT
VOID drv_cancel_send_packets (
	IN  _nic_hdl		MiniportAdapterContext,
	IN  PVOID			CancelId)
{
	_adapter *padapter = (_adapter *)MiniportAdapterContext;
	
	//PCE_SDIO_DEVICE	sddev = &padapter->NdisCEDev;

	_func_enter_;
	//2TO DO here


	_func_exit_;
	return;
}

VOID drv_pnp_event_notify(
	IN  _nic_hdl				MiniportAdapterContext,
	IN  NDIS_DEVICE_PNP_EVENT	PnPEvent,
	IN  PVOID					InformationBuffer,
	IN  unsigned long					InformationBufferLength)
{
	_irqL irqL;
	_adapter			*padapter = (_adapter *)MiniportAdapterContext;
	struct	mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	//PCE_SDIO_DEVICE	sddev = &padapter->NdisCEDev;

	_func_enter_;

	switch(PnPEvent) {
		
		case NdisDevicePnPEventSurpriseRemoved:
			DbgPrint("drv_pnp_event_notify:NdisDevicePnPEventSurpriseRemoved");
				padapter->bSurpriseRemoved = _TRUE;
				stop_drv_threads(padapter);
		
			DEBUG_INFO(("drv_pnp_event_notify: NdisDevicePnPEventSurpriseRemoved.\n"));
			
			// indicate-disconnect if necssary (free all assoc-resources)
			// dis-assoc from assoc_sta (optional)
			_enter_critical(&pmlmepriv->lock, &irqL );
			if(check_fwstate(pmlmepriv, _FW_LINKED)== _TRUE) 
			{
				indicate_disconnect(padapter); //will clr Linked_state; before this function, we must have chked whether  issue dis-assoc_cmd or not		
			}
			_exit_critical(&pmlmepriv->lock, &irqL );

			//3 todo:  wait until fw has process dis-assoc cmd

			//return rx-pkts
	

			if((padapter->dvobjpriv.rxirp_deinit !=NULL)){

				padapter->dvobjpriv.rxirp_deinit(padapter);
			}
				

			break;

		case NdisDevicePnPEventPowerProfileChanged:
			
			switch(*(u32 *)InformationBuffer)
			{
				case NdisPowerProfileBattery: 
					DEBUG_INFO(("MPPnPEventNotify: NdisPowerProfileBattery.\n"));
					break;
					
				case NdisPowerProfileAcOnLine: 
					DEBUG_INFO(("MPPnPEventNotify: NdisPowerProfileAcOnLine.\n"));
					break;

			}
			break;
			
		default:
			DEBUG_INFO(("MPPnPEventNotify: unknown PnP event %x \n", PnPEvent));
			break;         
    }

	_func_exit_;

	return;
}

VOID drv_shutdown(IN  _nic_hdl     MiniportAdapterContext)
{
	PADAPTER			Adapter = (PADAPTER)MiniportAdapterContext;
	//PCE_SDIO_DEVICE	sddev = &Adapter->NdisCEDev;

	_func_enter_;
	
	drv_halt(Adapter);
	_func_exit_;
	
	return;
}
#endif


BOOLEAN drv_checkforhang(IN _nic_hdl MiniportAdapterContext) {

	_adapter *padapter = (_adapter *)MiniportAdapterContext;
	
	//PCE_SDIO_DEVICE		psdiodevice = &padapter->NdisCEDev;

	_func_enter_;

	if(padapter->bSurpriseRemoved == _TRUE || padapter->bDriverStopped == _TRUE)
	{
		DbgPrint("#*#*#*#indicate hard failure #*#*#*#");
		DEBUG_ERR(("#*#*#*#indicate hard failure #*#*#*#"));
		NdisMIndicateStatus(padapter->hndis_adapter, NDIS_STATUS_HARD_ERRORS, NULL, 0);
		NdisMIndicateStatusComplete(padapter->hndis_adapter);
	}
	_func_exit_;
	
	return _FALSE;
}


static NDIS_STATUS
CopyFromUnicodeToOS( 
u8	*pointer,
UNICODE_STRING	*uniStr,
u16			copyLen)
{
	uint		s;
	OCTET_STRING	*os = (OCTET_STRING *)pointer;
	for(s=0; s<copyLen; s++)
	{
		os->Octet[s] = (u8)uniStr->Buffer[s];
	}
	os->Length = copyLen;
	return NDIS_STATUS_SUCCESS;

}

static NDIS_STATUS
CopyFromUnicodeToNDIS_802_11_SSID(u8 *pointer,UNICODE_STRING *uniStr, u16 copyLen)
{
	uint		s;
	NDIS_802_11_SSID	*NdisSsid = (NDIS_802_11_SSID *)pointer;
	
	for(s=0; s<copyLen; s++)
	{
		NdisSsid->Ssid[s] = (u8)uniStr->Buffer[s];
	}
	NdisSsid->SsidLength = copyLen;
	
	return NDIS_STATUS_SUCCESS;
}



static NDIS_STATUS
CopyFromUnicodeToString( 
						u8			*pointer,
						UNICODE_STRING	*uniStr,
						u16			copyLen)
{
	uint s;
	for(s=0; s<copyLen; s++)
	{
		pointer[s] = (u8)uniStr->Buffer[s];
	}
	return NDIS_STATUS_SUCCESS;
}


#ifdef PLATFORM_OS_XP

u32 loadregistry_xp(
IN  _adapter			*padapter,
IN  _nic_hdl			WrapperConfigurationContext
)
{
	_nic_hdl		ConfigurationHandle;
	PMP_REG_ENTRY	pRegEntry;
	u8	*			pointer;
	u8	*			NetworkAddress;
	uint			i;
	uint			Length;
	PNDIS_CONFIGURATION_PARAMETER	ReturnedValue;
	u32	_status=_SUCCESS;
	NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	struct registry_priv 	*registry_par = &padapter->registrypriv;


#ifdef UNDER_AMD64
	u64		value;
#else
	uint	value;
#endif

	_func_enter_;


	NdisOpenConfiguration( &Status, &ConfigurationHandle, WrapperConfigurationContext);
	
	if(Status != NDIS_STATUS_SUCCESS)
	{
		DEBUG_ERR(("<== CEReadRegParameters, Status=%x\n", Status));
		_status=_FAIL;
		goto exit;
	}

	for(i = 0, pRegEntry = NICRegTable; i < NIC_NUM_REG_PARAMS; i++, pRegEntry++)
	{
		pointer = (PUCHAR) registry_par + pRegEntry->FieldOffset;

		//DEBUG_ERR(("%s",&pRegEntry->RegName));

		// Get the configuration value for a specific parameter.  Under NT the
		// parameters are all read in as DWORDs.
		NdisReadConfiguration(
			&Status,
			&ReturnedValue,
			ConfigurationHandle,
			&pRegEntry->RegName,
			(enum _NDIS_PARAMETER_TYPE)pRegEntry->Type);

		// If the parameter was present, then check its value for validity.
		if (Status == NDIS_STATUS_SUCCESS)
		{
			if( (pRegEntry->Type == NdisParameterInteger)||
				(pRegEntry->Type == NdisParameterHexInteger) 	)
			{
				// Check that param value is not too small or too large
				if(ReturnedValue->ParameterData.IntegerData < pRegEntry->Min ||
					ReturnedValue->ParameterData.IntegerData > pRegEntry->Max)
				{
					value = pRegEntry->Default;
				}
				else
				{
					value = ReturnedValue->ParameterData.IntegerData;
				}

			}
			else if( pRegEntry->Type == NdisParameterString )
			{
				u16	copyLen;
				copyLen = 
				( (ReturnedValue->ParameterData.StringData.Length/2)>(u16)(pRegEntry->Max)) ? (u16)(pRegEntry->Max) : (u16)(ReturnedValue->ParameterData.StringData.Length/2);
				
				if( pRegEntry->FieldOffset == RGTRY_OFT(ssid))
				{
					CopyFromUnicodeToNDIS_802_11_SSID( pointer, &ReturnedValue->ParameterData.StringData, copyLen);
				}
				else
				{
					CopyFromUnicodeToOS( pointer,
						&ReturnedValue->ParameterData.StringData,
						copyLen);
				}
			}
		}
		else if(pRegEntry->bRequired)
		{
			DEBUG_ERR((" -- failed\n"));
			DEBUG_ERR(("Read parameter FAIL!"));

			_status = _FAIL;
			break;
		}
		else
		{
			if( (pRegEntry->Type == NdisParameterInteger) ||
				(pRegEntry->Type == NdisParameterHexInteger) )
			{
				value = pRegEntry->Default;
			}
			else
			{
				if( pRegEntry->FieldOffset == RGTRY_OFT(ssid))
				{
					UNICODE_STRING	*ptemp_ssid = (UNICODE_STRING*)pRegEntry->Default;
					
					CopyFromUnicodeToNDIS_802_11_SSID(pointer, ptemp_ssid, ptemp_ssid->Length/2);					
					
				}
				else	
				{
					CopyFromUnicodeToOS( pointer,
						(NDIS_STRING *)(UINT_PTR)pRegEntry->Default,
						((NDIS_STRING *)(UINT_PTR)pRegEntry->Default)->Length/2 );
				}
			}
			_status = _SUCCESS;
		}

		switch(pRegEntry->FieldSize)
		{
		case 1:
			*((u8 *) pointer) = (u8) value;
			break;
		case 2:
			*((u16 *) pointer) = (u16) value;
			break;
		case 4:
			*((u32 *) pointer) = (u32) value;
			break;
		case 8:
			*((u64 *) pointer) = (u64) value;
			break;
		default:
			DEBUG_ERR(("Bogus field size %d\n", pRegEntry->FieldSize));
			break;
		}
	}

	// Close the registry
	NdisCloseConfiguration(ConfigurationHandle);

	//For MP driver
	if(registry_par->mp_mode == 1)
	{
		registry_par->vrtl_carrier_sense = DISABLE_VCS;
		registry_par->vcs_type = NONE_VCS;
	}

	//

#ifdef WINDOWS_VIRTUAL
	// set MAC Address
	padapter->PermanentAddress[0] = 0x02;
	padapter->PermanentAddress[1] = 0x50;
	padapter->PermanentAddress[2] = 0xF2;
	padapter->PermanentAddress[3] = 0x00;
	padapter->PermanentAddress[4] = 0x00;
	padapter->PermanentAddress[5] = 0x10;

	padapter->CurrentAddress[0] = 0x02;
	padapter->CurrentAddress[1] = 0x50;
	padapter->CurrentAddress[2] = 0xF2;
	padapter->CurrentAddress[3] = 0x00;
	padapter->CurrentAddress[4] = 0x00;
	padapter->CurrentAddress[5] = 0x10;
#endif

exit:
	_func_exit_;
	
	return _status;
}

#endif

u32 loadregistry(
IN  _adapter			*padapter,
IN  _nic_hdl			WrapperConfigurationContext
)
{


	u32 _status = _SUCCESS;

#ifdef PLATFORM_OS_XP

	_status = loadregistry_xp(padapter, WrapperConfigurationContext);

#endif


#ifdef PLATFORM_OS_WINCE

	_status = loadregistry_wince(padapter, WrapperConfigurationContext);

#endif


	return _status;


}


NDIS_STATUS drv_query_oid(
	IN	_nic_hdl		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
	)
{

	
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;

return Status;
}

NDIS_STATUS
drv_set_oid(
	IN	_nic_hdl		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	unsigned long		InformationBufferLength,
	OUT	unsigned long*	BytesRead,
	OUT	unsigned long*	BytesNeeded
	)
{

	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

return Status;
}

