#define _HCI_INTF_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>
#include <hal_init.h>


#include <usb.h>
#include <usbioctl.h>
#include <usbdlib.h>

#include <usb_vendor_req.h>
#include <usb_ops.h>
#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_USB_HCI

#error "CONFIG_USB_HCI shall be on!\n"

#endif

#define USBD_HALTED(Status) ((ULONG)(Status) >> 30 == 3)
u8 read_usb_hisr(_adapter * padapter);

// PASSIVE_LEVEL
//
NTSTATUS 
callusbd(
	struct dvobj_priv *pdvobjpriv, 
	PURB			purb)
{

	IO_STATUS_BLOCK			io_status_block;
	PIO_STACK_LOCATION		next_stack;
	LARGE_INTEGER			wait_time;
	KEVENT					kevent;
	PIRP						irp;
	USBD_STATUS			usbdstatus;
	NTSTATUS				nt_status = STATUS_SUCCESS;
	_adapter	*				padapter = pdvobjpriv->padapter;
	
	_func_enter_;

	if(padapter->bDriverStopped|| padapter->bSurpriseRemoved){
		DEBUG_ERR(("\n Driver stopped || SurpriseRemoved!\n"));
		goto	exit;
	}
		
	KeInitializeEvent(&kevent, NotificationEvent, FALSE);	//PASSIVE_LEVEL
	irp = IoBuildDeviceIoControlRequest(
			IOCTL_INTERNAL_USB_SUBMIT_URB,
			pdvobjpriv->pnextdevobj, 
			NULL, 
			0, 
			NULL, 
			0, 
			_TRUE, 
			&kevent, 
			&io_status_block);

	if (irp == NULL) 
	{	
		DEBUG_ERR(("\n memory allocate for irp failed!!\n"));
		nt_status=STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}

	next_stack = IoGetNextIrpStackLocation(irp);
	if(next_stack == NULL)
		DEBUG_ERR(("IoGetNextIrpStackLocation ==NULL "));

	next_stack->Parameters.Others.Argument1 = purb;
	//nextStack->Parameters.Others.Argument1 = pUrb;
	nt_status = IoCallDriver(pdvobjpriv->pnextdevobj, irp);
	//ntStatus = IoCallDriver(CEdevice->pUsbDevObj, irp);
	usbdstatus = URB_STATUS(purb);
	DEBUG_INFO(("After IoCallDriver()... URB STATUS: %X\n", usbdstatus));
	//RT_TRACE(COMP_DBG, DBG_TRACE, ("After IoCallDriver()... URB STATUS: %X\n", usbdstatus));

	if(nt_status == STATUS_PENDING) 
	{
		// Method 1
		wait_time.QuadPart = -10000 * 5000;
		nt_status = KeWaitForSingleObject(&kevent, Executive, KernelMode, _FALSE, &wait_time); //8150 code
		// Method 2
		//ntStatus = KeWaitForSingleObject(&Kevent, Executive, KernelMode, FALSE, NULL); //DDK sample
		
		usbdstatus = URB_STATUS(purb);
		DEBUG_ERR(("After KeWaitForSingleObject()...URB STATUS: %X\n", usbdstatus));
		//RT_TRACE(COMP_DBG, DBG_TRACE, ("After KeWaitForSingleObject()...URB STATUS: %X\n", usbdstatus));

		if(nt_status == STATUS_TIMEOUT)
		{
			IoCancelIrp(irp);
			nt_status = KeWaitForSingleObject(&kevent, Executive, KernelMode, _FALSE, NULL); //DDK sample
			usbdstatus = URB_STATUS(purb);
			DEBUG_ERR(("After IoCancelIrp()...URB STATUS: %X\n", usbdstatus));
			DEBUG_ERR(("After IoCancelIrp()...URB STATUS: %X\n", usbdstatus));
			usbdstatus = USBD_STATUS_SUCCESS;
		}
	} 

exit:	
	_func_exit_;
	return nt_status;
}


/*! \brief Wrap the pUrb to an IRP and send this IRP to Bus Driver. Then wait for this IRP completion.
	The Caller shall be at Passive Level.
*/
NTSTATUS  synccallusbd(struct dvobj_priv *pdvobjpriv, PURB purb) {


	KEVENT					kevent;
	PIRP						irp;
	IO_STATUS_BLOCK			iostatusblock;
	PIO_STACK_LOCATION		nextstack;
	USBD_STATUS			usbdstatus;
	LARGE_INTEGER			waittime;
	_adapter	*				padapter=pdvobjpriv->padapter;
	NTSTATUS				ntstatus = STATUS_SUCCESS;

	_func_enter_;

//	if(padapter->bDriverStopped) {
//		goto exit;
//	}
	
	KeInitializeEvent(&kevent, NotificationEvent, _FALSE);
	irp = IoBuildDeviceIoControlRequest(
			IOCTL_INTERNAL_USB_SUBMIT_URB,
			pdvobjpriv->pphysdevobj,//CEdevice->pUsbDevObj, 
			NULL, 
			0, 
			NULL, 
			0, 
			_TRUE, 
			&kevent, 
			&iostatusblock);

	if(NULL == irp) {
		DEBUG_ERR(("SyncCallUSBD: memory alloc for irp failed\n"));
		ntstatus=STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}
	nextstack = IoGetNextIrpStackLocation(irp);
	if(nextstack == NULL)	
		DEBUG_ERR(("IoGetNextIrpStackLocation\n"));

	nextstack->Parameters.Others.Argument1 = purb;
	
	// Issue an IRP for Sync IO.
	ntstatus = IoCallDriver(pdvobjpriv->pphysdevobj, irp);
	usbdstatus = URB_STATUS(purb);

	if(ntstatus == STATUS_PENDING) {
		// Method 1
		waittime.QuadPart = -10000 * 50000;
		ntstatus = KeWaitForSingleObject(&kevent, Executive, KernelMode, _FALSE, &waittime); //8150 code
		// Method 2
		//ntStatus = KeWaitForSingleObject(&Kevent, Executive, KernelMode, FALSE, NULL); //DDK sample
		
		usbdstatus = URB_STATUS(purb);

		if(ntstatus == STATUS_TIMEOUT) {
			//usbdevice->nIoStuckCnt++;
			//RT_TRACE( COMP_DBG, DBG_SERIOUS,("RTusbCallUSBD: TIMEOUT....5000ms, nIoStuckCnt: %d\n", usbdevice->nIoStuckCnt) );

			// Method 2
			IoCancelIrp(irp);
			ntstatus = KeWaitForSingleObject(&kevent, Executive, KernelMode, _FALSE, NULL); //DDK sample
			usbdstatus = URB_STATUS(purb);

			usbdstatus = USBD_STATUS_SUCCESS;
		}
	}
	
exit:	
	_func_exit_;
	return ntstatus;
}



/*! \brief Issue an USB Vendor Request for RTL8711. This function can be called under Passive Level only.
	@param bRequest			bRequest of Vendor Request
	@param wValue			wValue of Vendor Request
	@param wIndex			wIndex of Vendor Request
	@param Data				
	@param DataLength
	@param isDirectionIn	Indicates that if this Vendor Request's direction is Device->Host
	
*/
u8 usbvendorrequest(struct dvobj_priv *pdvobjpriv, RT_USB_BREQUEST brequest, RT_USB_WVALUE wvalue, u8 windex, PVOID data, u8 datalen, u8 isdirectionin){

	PURB			urb;
	u8				ret=_TRUE;
	NTSTATUS		ntstatus = STATUS_SUCCESS;
	ULONG			transferflags = 0;

	_func_enter_;
	
	urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST) );
	if(urb == NULL) {
		
		DEBUG_ERR(("CEUsbSetVendorReq(): Failed to allocate urb !!!\n"));
		ret=_FALSE;
		goto exit;
	}


	if (isdirectionin == _TRUE) {
		transferflags = USBD_TRANSFER_DIRECTION_IN;
	} else {
		transferflags= 0;
	}

	UsbBuildVendorRequest(
			urb, 		//Pointer to an URB that is to be formatted as a vendor or class request. 
			URB_FUNCTION_VENDOR_DEVICE,	//Indicates the URB is a vendor-defined request for a USB device. 
			sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),  //Specifies the length, in bytes, of the URB. 
			transferflags, 			//TransferFlags 
			0,			//ReservedBits 
			brequest,  	//Request 
			wvalue, 	//Value 
			windex,		//Index
			data,		//TransferBuffer 
			NULL,		//TransferBufferMDL 
			datalen,	//TransferBufferLength 
			NULL		//Link 
	);

	ntstatus = synccallusbd(pdvobjpriv,  urb);
	if(!NT_SUCCESS(ntstatus))
	{
		ExFreePool(urb);
		DEBUG_ERR((" CEusbVendorRequest() : SOMETHING WRONG\n") );
		ret = _FALSE;
		goto exit;
	}

	ExFreePool(urb);

exit:	
	_func_exit_;
	return ret;	

}

NDIS_STATUS configureusbdevice(struct dvobj_priv *pdvobjpriv) {
	u32					siz;
	USBD_INTERFACE_LIST_ENTRY		usbdinterfacelist[2];
	NTSTATUS			ntstatus = STATUS_SUCCESS;
	PURB				urb=NULL,cfgurb=NULL;
	PUSB_INTERFACE_DESCRIPTOR		interfacedescriptor = NULL;
	PUSBD_INTERFACE_INFORMATION	intf = NULL;
	u8					tmpuchar = 0;
	u32					epcount = 0;

	_func_enter_;

	urb =(PURB) & pdvobjpriv->descriptor_urb;// (PURB)  &CEdevice->DescriptorUrb;
	// When USB_CONFIGURATION_DESCRIPTOR_TYPE is specified for DescriptorType
	// in a call to UsbBuildGetDescriptorRequest(),
	// all interface, endpoint, class-specific, and vendor-specific descriptors
	// for the configuration also are retrieved.
	// The caller must allocate a buffer large enough to hold all of this
	// information or the data is truncated without error.
	// Therefore the 'siz' set below is just a 'good guess', and we may have to retry
	siz = 0x1ff;
	while( 1 ) 
	{
		// <NOTE> We MUST free the memory allocated via ExAllocatePool() later, 
		// otherwise, driver verifier will detect a memory leak and cause KeBugCheck 0xC4.
		pdvobjpriv->config_descriptor_len=siz;//CEdevice->UsbConfigurationDescriptorLength = siz;
		pdvobjpriv->pconfig_descriptor = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, siz);
		//CEdevice->UsbConfigurationDescriptor		= (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, siz);

		if(pdvobjpriv->pconfig_descriptor==NULL){
		//if(CEdevice->UsbConfigurationDescriptor == NULL) {
			ntstatus = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		
		UsbBuildGetDescriptorRequest(urb,
								 (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
								 USB_CONFIGURATION_DESCRIPTOR_TYPE,
								 0,
								 0,
								 pdvobjpriv->pconfig_descriptor,//CEdevice->UsbConfigurationDescriptor,
								 NULL,
								 pdvobjpriv->config_descriptor_len,//CEdevice->UsbConfigurationDescriptorLength,
								 NULL);
		//ntStatus =  CEusbCallUSBD(CEdevice,  urb); 
		ntstatus=callusbd(pdvobjpriv, urb);
		if( !NT_SUCCESS(ntstatus) ) 
		{
			DEBUG_ERR(("XXX ConfigureDevice(): UsbBuildGetDescriptorRequest() FAILURE (%x)\n", ntstatus));
			break;
		} 
		DEBUG_INFO((" ConfigureDevice(): Configuration Descriptor len = 0x%x\n",
					urb->UrbControlDescriptorRequest.TransferBufferLength));

		//
		// if we got some data see if it was enough.
		// NOTE: we may get an error in URB because of buffer overrun
		//if( (urb->UrbControlDescriptorRequest.TransferBufferLength > 0) &&
		//	( CEdevice->UsbConfigurationDescriptor->wTotalLength > siz) ) 
		if( (urb->UrbControlDescriptorRequest.TransferBufferLength > 0) &&
				(pdvobjpriv->pconfig_descriptor->wTotalLength> siz) ) 
		{
			// Free UsbConfigurationDescriptor allocated by ExAllocatePool()
			ExFreePool(pdvobjpriv->pconfig_descriptor);//ExFreePool(CEdevice->UsbConfigurationDescriptor);
			pdvobjpriv->pconfig_descriptor = NULL;//CEdevice->UsbConfigurationDescriptor = NULL;

			// Try to allocate larger memory again.
			siz =pdvobjpriv->pconfig_descriptor->wTotalLength; //CEdevice->UsbConfigurationDescriptor->wTotalLength;
			continue;
		} 
		if(pdvobjpriv->pconfig_descriptor ==NULL)
			DEBUG_ERR(("CEdevice->UsbConfigurationDescriptor = NULL") );
		pdvobjpriv->pconfig_descriptor->bConfigurationValue = 1;
		//CEdevice->UsbConfigurationDescriptor->bConfigurationValue = 1;


		// Show Configuration Descriptor
		DEBUG_INFO(("=== RTL8711 Configuration Descriptor ===\n"));
		DEBUG_INFO(("bLength:	0x%X\n", pdvobjpriv->pconfig_descriptor->bLength));
		DEBUG_INFO(("bDescriptorType:	0x%X\n", pdvobjpriv->pconfig_descriptor->bDescriptorType));
		DEBUG_INFO(("wTotalLength:	0x%X\n", pdvobjpriv->pconfig_descriptor->wTotalLength));
		DEBUG_INFO(("bNumInterfaces:	0x%X\n", pdvobjpriv->pconfig_descriptor->bNumInterfaces));
		DEBUG_INFO(("bConfigurationValue:	0x%X\n", pdvobjpriv->pconfig_descriptor->bConfigurationValue));
		DEBUG_INFO(("iConfiguration:	0x%X\n", pdvobjpriv->pconfig_descriptor->iConfiguration));
		DEBUG_INFO(("bmAttributes:	0x%X\n", pdvobjpriv->pconfig_descriptor->bmAttributes));
		DEBUG_INFO(("MaxPower:	0x%X-h\n", pdvobjpriv->pconfig_descriptor->MaxPower));


		// Read 8711 CPU Boot Type, USB only. added by pollo.
		usbvendorrequest(pdvobjpriv, RT_USB_GET_REGISTER, RT_USB_BOOT_TYPE, 0, &tmpuchar, 1, _TRUE);
		DEBUG_INFO(("Boot Type is %d\n", tmpuchar));
		pdvobjpriv->cpu_boot_type=tmpuchar;
		//CEdevice->CPUBootType = TmpUchar;


		// Interrupt endpoint's MaxPacketSize must be less than 0x40 in Interface 0, Alternate Setting 0
		// So we Search Interface 0, Alternate Setting 1 first. modified by pollo.
		interfacedescriptor = USBD_ParseConfigurationDescriptorEx(pdvobjpriv->pconfig_descriptor,//CEdevice->UsbConfigurationDescriptor,
								  pdvobjpriv->pconfig_descriptor,//CEdevice->UsbConfigurationDescriptor, //search from start of config  descriptro
								  0,	  // interface number not a criteria; we only support one interface
								  1,   // not interested in alternate setting here either
								  -1,   // interface class not a criteria
								  -1,   // interface subclass not a criteria
								  -1    // interface protocol not a criteria
								  );
		if(interfacedescriptor == NULL) {
			DEBUG_ERR(("Can not find Interface 0, Alternate Setting 1. Try to search Interface 0, Alternate Setting 0.\n"));
			interfacedescriptor = USBD_ParseConfigurationDescriptorEx(pdvobjpriv->pconfig_descriptor,//CEdevice->UsbConfigurationDescriptor,
				pdvobjpriv->pconfig_descriptor,//CEdevice->UsbConfigurationDescriptor, //search from start of config  descriptro
				0,	  // interface number not a criteria; we only support one interface
				0,   // not interested in alternate setting here either
				-1,   // interface class not a criteria
				-1,   // interface subclass not a criteria
				-1    // interface protocol not a criteria
				);
			if(interfacedescriptor == NULL) {
				DEBUG_ERR(("Can not find Interface 0, Alternate Setting 0, return STATUS_INSUFFICIENT_RESOURCES.\n"));
				ntstatus=STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
		}

		usbdinterfacelist[0].InterfaceDescriptor = interfacedescriptor;
		usbdinterfacelist[0].Interface = NULL;
		usbdinterfacelist[1].InterfaceDescriptor = NULL;
		usbdinterfacelist[1].Interface = NULL;

		cfgurb = USBD_CreateConfigurationRequestEx( 
					pdvobjpriv->pconfig_descriptor,//CEdevice->UsbConfigurationDescriptor,
					usbdinterfacelist );

		if(cfgurb == NULL) {
			DEBUG_ERR(("XXX SelectConfiguration: USBD_CreateConfigurationRequestEx() failed\n") );
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		intf= usbdinterfacelist[0].Interface;

		DEBUG_INFO(("Interface->NumberOfPipes = %d\n", intf->NumberOfPipes));

		UsbBuildSelectConfigurationRequest(
			cfgurb,
			(USHORT)(GET_SELECT_CONFIGURATION_REQUEST_SIZE(1, intf->NumberOfPipes)),
			pdvobjpriv->pconfig_descriptor//CEdevice->UsbConfigurationDescriptor
			);

		ntstatus = callusbd(pdvobjpriv, cfgurb); 
		if( ntstatus != STATUS_SUCCESS ) {
			DEBUG_ERR(("XXX SelectConfiguration: failed to select a configuration\n" ));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		// save the handles of the AC_VO bulk-out pipe
		// control pipe is not included, so AC_VO bulk-out pipe is pipe[0], and so on.
		if( (UsbdPipeTypeBulk == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_OUT( intf->Pipes[epcount].EndpointAddress ) ) {
			pdvobjpriv->vo_pipehandle=intf->Pipes[epcount].PipeHandle;//CEdevice->VOBulkOutPipeHandle = Interface->Pipes[epCount].PipeHandle;
			if (intf->Pipes[epcount].MaximumPacketSize > 64){//if (Interface->Pipes[epCount].MaximumPacketSize > 64){
				pdvobjpriv->ishighspeed = _TRUE;//CEdevice->bUSBIsHighSpeed = _TRUE;
				DEBUG_ERR(("USB Speed : High Speed\n"));
			} else {
				pdvobjpriv->ishighspeed=_FALSE;//CEdevice->bUSBIsHighSpeed = FALSE;
				DEBUG_ERR(("USB Speed : Full Speed\n"));
			}
		} else {

			DEBUG_ERR((" XXX SelectConfiguration: pipe type of AC_VO bulk-out pipe(%d) is wrong.\n", epcount ));
			DEBUG_ERR(("Pipe Type = %d, Endpoint Address = %d\n", intf->Pipes[epcount].PipeType, intf->Pipes[epcount].EndpointAddress));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		epcount++;

		if( (UsbdPipeTypeBulk == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_OUT( intf->Pipes[epcount].EndpointAddress ) ) {
			pdvobjpriv->vi_pipehandle=intf->Pipes[epcount].PipeHandle;//CEdevice->VIBulkOutPipeHandle = Interface->Pipes[epCount].PipeHandle;
		} else	{
			DEBUG_ERR((" XXX SelectConfiguration: pipe type of AC_VI bulk-out pipe is wrong(%d).\n" , epcount));
			DEBUG_ERR(("Pipe Type = %d, Endpoint Address = %d\n", intf->Pipes[epcount].PipeType, intf->Pipes[epcount].EndpointAddress));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		epcount++;

		if( (UsbdPipeTypeBulk == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_OUT( intf->Pipes[epcount].EndpointAddress ) ) {
			pdvobjpriv->be_pipehandle=intf->Pipes[epcount].PipeHandle;//CEdevice->BEBulkOutPipeHandle = Interface->Pipes[epCount].PipeHandle;
		} else {
			DEBUG_ERR((" XXX SelectConfiguration: pipe type of AC_BE bulk-out pipe is wrong(%d)\n", epcount));
			DEBUG_ERR(("Pipe Type = %d, Endpoint Address = %d\n", intf->Pipes[epcount].PipeType, intf->Pipes[epcount].EndpointAddress));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		epcount++;

		if( (UsbdPipeTypeBulk == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_OUT( intf->Pipes[epcount].EndpointAddress ) ) {
			pdvobjpriv->bk_pipehandle=intf->Pipes[epcount].PipeHandle;//CEdevice->BKBulkOutPipeHandle = Interface->Pipes[epCount].PipeHandle;

		} else {
			DEBUG_ERR((" XXX SelectConfiguration: pipe type of AC_BK bulk-out pipe is wrong(%d)\n" , epcount));
			DEBUG_ERR(("Pipe Type = %d, Endpoint Address = %d\n", intf->Pipes[epcount].PipeType, intf->Pipes[epcount].EndpointAddress));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		epcount++;

		if( (UsbdPipeTypeBulk == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_OUT( intf->Pipes[epcount].EndpointAddress ) ) {
			pdvobjpriv->ts_pipehandle=intf->Pipes[epcount].PipeHandle;//CEdevice->TSBulkOutPipeHandle = Interface->Pipes[epCount].PipeHandle;
		} else {
			DEBUG_ERR((" XXX SelectConfiguration: pipe type of TS bulk-out pipe is wrong(%d)\n", epcount));
			DEBUG_ERR(("Pipe Type = %d, Endpoint Address = %d\n", intf->Pipes[epcount].PipeType, intf->Pipes[epcount].EndpointAddress));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		epcount++;

		if( (UsbdPipeTypeBulk == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_OUT( intf->Pipes[epcount].EndpointAddress ) ) {
			pdvobjpriv->bm_pipehandle= intf->Pipes[epcount].PipeHandle;
		} else {
			DEBUG_ERR((" XXX SelectConfiguration: pipe type of BM bulk-out pipe is wrong(%d)\n", epcount));
			DEBUG_ERR(("Pipe Type = %d, Endpoint Address = %d\n", intf->Pipes[epcount].PipeType, intf->Pipes[epcount].EndpointAddress));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}
		
		epcount++;

		if( (UsbdPipeTypeBulk == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_IN( intf->Pipes[epcount].EndpointAddress ) ) {
			pdvobjpriv->rx_pipehandle= intf->Pipes[epcount].PipeHandle;
		} else {
			DEBUG_ERR((" XXX SelectConfiguration: pipe type of Rx bulk-in pipe is wrong(%d)\n", epcount));
			DEBUG_ERR(("Pipe Type = %d, Endpoint Address = %d\n", intf->Pipes[epcount].PipeType, intf->Pipes[epcount].EndpointAddress));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		epcount++;

		if( (UsbdPipeTypeInterrupt == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_OUT( intf->Pipes[epcount].EndpointAddress ) ) 	{
			pdvobjpriv->regout_pipehandle= intf->Pipes[epcount].PipeHandle;
			DEBUG_INFO(("RegInterruptOutPipeHandle's MaximumPacketSize = %d\n", intf->Pipes[epcount].MaximumPacketSize));
		} else {
			DEBUG_ERR((" XXX SelectConfiguration: pipe type of reg. interrupt-out pipe is wrong\n", epcount));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		epcount++;

		if( (UsbdPipeTypeInterrupt == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_IN( intf->Pipes[epcount].EndpointAddress ) ) {
			pdvobjpriv->regin_pipehandle= intf->Pipes[epcount].PipeHandle;
			DEBUG_INFO(("RegInterruptInPipeHandle's MaximumPacketSize = %d\n", intf->Pipes[epcount].MaximumPacketSize));

		} else {
			DEBUG_ERR((" XXX SelectConfiguration: pipe type of reg. interrupt-in pipe is wrong(%d)\n", epcount));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		epcount++;

		if (pdvobjpriv->cpu_boot_type== BOOT_FROM_NAND) {
			// WLAN + MS has not Mass Storage Bulk In/Bulk Out pipes
			DEBUG_INFO(("WLAN + MS has not Mass Storage Bulk In/Bulk Out pipes.\n"));

		} else {

			if( (UsbdPipeTypeBulk == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_OUT( intf->Pipes[epcount].EndpointAddress ) ) 	{
				pdvobjpriv->scsi_out_pipehandle= intf->Pipes[epcount].PipeHandle;
				DEBUG_INFO(("Got Mass Storage Bulk Out pipe.\n"));
			} else {
				DEBUG_ERR((" XXX SelectConfiguration: pipe type of SCSI bulk-out pipe is wrong\n" ));
				ExFreePool(cfgurb);
				ntstatus=STATUS_UNSUCCESSFUL;
				break;
			}

			epcount++;

			if( (UsbdPipeTypeBulk == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_IN( intf->Pipes[epcount].EndpointAddress ) ) 	{
				pdvobjpriv->scsi_in_pipehandle= intf->Pipes[epcount].PipeHandle;
				DEBUG_INFO(("Got Mass Storage Bulk In pipe.\n"));
			} else {
				DEBUG_ERR((" XXX SelectConfiguration: pipe type of SCSI bulk-in pipe is wrong\n" ));
				ExFreePool(cfgurb);
				ntstatus=STATUS_UNSUCCESSFUL;
				break;
			}

			epcount++;
		}


		if( (UsbdPipeTypeInterrupt == intf->Pipes[epcount].PipeType) && USB_ENDPOINT_DIRECTION_IN( intf->Pipes[epcount].EndpointAddress ) ) {
			pdvobjpriv->interrupt_pipehandle= intf->Pipes[epcount].PipeHandle;
			DEBUG_INFO(("TSInterruptInPipeHandle's MaximumPacketSize = %d\n", intf->Pipes[epcount].MaximumPacketSize));

		} else {
			DEBUG_ERR((" XXX SelectConfiguration: pipe type of TS interrupt-in pipe is wrong\n" ));
			ExFreePool(cfgurb);
			ntstatus=STATUS_UNSUCCESSFUL;
			break;
		}

		if (cfgurb) {
			// don't call the MemFree since the buffer was
			//  alloced by USBD_CreateConfigurationRequest, not MemAlloc()
			ExFreePool(cfgurb);
		}

		break;
	}

	if (pdvobjpriv->pconfig_descriptor!= NULL)//if (CEdevice->UsbConfigurationDescriptor != NULL)
	{
		// Free UsbConfigurationDescriptor allocated by ExAllocatePool()
		ExFreePool(pdvobjpriv->pconfig_descriptor);
		pdvobjpriv->pconfig_descriptor= NULL;
	}

	
	_func_exit_;
	return ntstatus;	

}






u8  read_usb_hisr_cancel(_adapter * padapter){
	
	s8			i;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	DbgPrint("\n ===> read_usb_hisr_cancel \n");

	pdev->ts_in_cnt--;//pdev->tsint_cnt--;  //decrease 1 forInitialize ++ 

	if (pdev->ts_in_cnt) {//if (pdev->tsint_cnt) {
		// Canceling Pending Irp
			if ( pdev->ts_in_pendding == _TRUE) {//if ( pdev->tsint_pendding[pdev->tsint_idx] == _TRUE) {
				IoCancelIrp( pdev->ts_in_irp);//IoCancelIrp( pdev->tsint_irp[pdev->tsint_idx]);
			}
		
	DbgPrint("\n  read_usb_hisr_cancel:cancel ts_in_pendding\n");

//		while (_TRUE) {
//			if (NdisWaitEvent(&pdev->interrupt_retevt, 2000)) {
//				break;
//			}
//		}

		_down_sema(&(pdev->hisr_retevt));
	DbgPrint("\n  read_usb_hisr_cancel:down sema\n");

	}
	DbgPrint("\n <=== read_usb_hisr_cancel \n");

	return _SUCCESS;
}


/*! \brief USB TS In IRP Complete Routine.
	@param Context pointer of RT_TSB
*/
NTSTATUS read_usb_hisr_complete(PDEVICE_OBJECT	pdevobj/*pUsbDevObj*/, PIRP pirp, PVOID context) {

	struct _URB_BULK_OR_INTERRUPT_TRANSFER	*pinturb;
	u16						bytecnt;
	u32						tmp32;
	PURB					purb;
	USBD_STATUS			usbdstatus;
	NTSTATUS				ntstatus=STATUS_SUCCESS;
	_adapter * padapter=(_adapter *)context;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	 struct xmit_priv *pxmitpriv= &(padapter->xmitpriv);	
	struct	evt_priv	*pevt_priv = &(padapter->evtpriv);
	struct	cmd_priv	*pcmd_priv = &(padapter->cmdpriv);
	 
	_func_enter_;
	purb = &pdev->ts_in_urb;//purb = &pdev->tsint_urb[pdev->tsint_idx];//&pTSb->Urb;	   //changed
	pdev->ts_in_pendding= _FALSE;//pdev->tsint_pendding[pdev->tsint_idx]= _FALSE;
	pdev->ts_in_cnt--;//pdev->tsint_cnt--;
	if (pdev->ts_in_cnt == 0) {
		DEBUG_ERR(("pdvobjpriv->tsint_cnt == 0, set interrupt_retevt!\n"));
		_up_sema(&(pdev->hisr_retevt));//NdisSetEvent(&pdev->interrupt_retevt);
	}
	if( pirp->Cancel == _TRUE ) {
		DEBUG_ERR((" pirp->Cancel == _TRUE,IRP has been cancelled!!! \n") );
		ntstatus= STATUS_MORE_PROCESSING_REQUIRED;
		goto exit;
	}
	if(padapter->bSurpriseRemoved) {

		DEBUG_ERR((" Surprised remove!!!\n"));
		ntstatus=STATUS_MORE_PROCESSING_REQUIRED;
		goto exit;
	}

	switch(pirp->IoStatus.Status) {

		case STATUS_SUCCESS:
		//Copy the value to CEdevice->HisrValue
		pinturb =& pdev->ts_in_urb.UrbBulkOrInterruptTransfer;  //pinturb =& pdev->tsint_urb[pdev->tsint_idx].UrbBulkOrInterruptTransfer;    //changed
		if(pinturb->TransferBufferLength > 128) {
			DEBUG_ERR(("TS In size>128!!!\n"));
			ntstatus= STATUS_MORE_PROCESSING_REQUIRED;
			goto exit;
		}
		else{
			bytecnt=(USHORT)pinturb->TransferBufferLength;
			if(bytecnt<7){
				DEBUG_ERR(("bytecnt must not less than 7 (3 for hisr,4 for fwclsptr) \n"));
			}
		}	
		padapter->IsrContent = pdev->ts_in_buf[2]<<16;// pdev->tsint_buf[pdev->tsint_idx][2]<<16;
		padapter->IsrContent |= pdev->ts_in_buf[1]<<8;//pdev->tsint_buf[pdev->tsint_idx][1]<<8;
		padapter->IsrContent |= pdev->ts_in_buf[0];//pdev->tsint_buf[pdev->tsint_idx][0];


	if( (padapter->IsrContent & _C2HSET )) // C2HSET
	{
		DEBUG_INFO(("\n----------read_usb_hisr_complete: _C2HSET  "));
		padapter->IsrContent  ^= _C2HSET;
		register_evt_alive(padapter);
		evt_notify_isr(pevt_priv);
		
	}	
	if( (padapter->IsrContent  & _H2CCLR)) // H2CCLR
	{
		DEBUG_INFO(("\n----------read_usb_hisr_complete: _H2CCLR  "));

		padapter->IsrContent  ^= _H2CCLR;
		cmd_clr_isr(pcmd_priv);

	}
	if( (padapter->IsrContent  & _RXDONE)) // H2CCLR
	{
		DEBUG_INFO(("\n----------read_usb_hisr_complete: _RXDONE "));
	}


		ntstatus=STATUS_MORE_PROCESSING_REQUIRED;
			break;
		default:

			ntstatus= STATUS_MORE_PROCESSING_REQUIRED;
			goto exit;
		}


	
	if((padapter->bDriverStopped !=_TRUE)&&(padapter->bSurpriseRemoved!= _TRUE)){	
	read_usb_hisr(padapter);
	}

exit:	
	_func_exit_;
	return ntstatus;

	
}

u8 read_usb_hisr(_adapter * padapter){
	
	PIO_STACK_LOCATION	nextStack;
	NTSTATUS			ntstatus;
	USBD_STATUS		usbdstatus;
	u8 retval=_SUCCESS;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	_func_enter_;
	
	if(padapter->bDriverStopped || padapter->bSurpriseRemoved||padapter->pwrctrlpriv.pnp_bstop_trx) {
		DEBUG_ERR(("( padapter->bDriverStopped ||padapter->bSurpriseRemoved )!!!\n"));
		retval= _FAIL;
		goto exit;
	}
// Build our URB for USBD
	_spinlock(&(pdev->in_lock));
	UsbBuildInterruptOrBulkTransferRequest(
		&(pdev->ts_in_urb),//&(pdev->tsint_urb[pdev->tsint_idx]),//&(pTSb->Urb),                                                             //changed
		(USHORT)sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER ),			//(USHORT)size,
		pdev->interrupt_pipehandle,					//CEdevice->TSInterruptInPipeHandle,
		&(pdev->ts_in_buf[0]),		//&(pdev->tsint_buf[pdev->tsint_idx][0]),							
		NULL, 
		128,	//TS In size 128B
		USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK, 
		NULL
		);

	pdev->ts_in_pendding=_TRUE; //pdev->tsint_pendding[pdev->tsint_idx]=_TRUE; 
	if(pdev->ts_in_cnt==1){
		pdev->ts_in_cnt++;//pdev->tsint_cnt++;
	}
	else{
		// error
		DEBUG_ERR(("ERROR:read_usb_hisr:pdev->ts_in_cnt==%d ",pdev->ts_in_cnt));

	}
	// call the class driver to perform the operation
	// and pass the URB to the USB driver stack
	nextStack = IoGetNextIrpStackLocation(pdev->ts_in_irp);//nextStack = IoGetNextIrpStackLocation(pdev->tsint_irp[pdev->tsint_idx]);
	nextStack->Parameters.Others.Argument1 = &pdev->ts_in_urb;//nextStack->Parameters.Others.Argument1 = &pdev->tsint_urb[pdev->tsint_idx];//&pTSb->Urb;   //changed
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

	IoSetCompletionRoutine(
		pdev->ts_in_irp,//pdev->tsint_irp[pdev->tsint_idx],		//pIrp,					// irp to use
		read_usb_hisr_complete,//CEusbTSCompleteRecv,	// routine to call when irp is done
		padapter,				//pTSb,					// context to pass routine 
		_TRUE,					// call on success
		_TRUE,					// call on error
		_TRUE);					// call on cancel

	//
	// The IoCallDriver routine  
	// sends an IRP to the driver associated with a specified device object.
	//
	ntstatus = IoCallDriver(pdev->pnextdevobj, pdev->ts_in_irp);//ntstatus = IoCallDriver(pdev->pnextdevobj, pdev->tsint_irp[pdev->tsint_idx]);
	usbdstatus = URB_STATUS(&(pdev->ts_in_urb));//usbdstatus = URB_STATUS(&(pdev->tsint_urb[pdev->tsint_idx]));

	if( ntstatus != STATUS_PENDING || USBD_HALTED(usbdstatus) ) {
		DEBUG_ERR(("\n read_usb_hisr() : IoCallDriver failed!!! IRP STATUS: %X\n", ntstatus) );
		DEBUG_ERR(("\n read_usb_hisr() : IoCallDriver failed!!! USB STATUS: %X\n", usbdstatus) );

		if( USBD_HALTED(usbdstatus) ) {

		DEBUG_ERR(("\n read_usb_hisr() :  USB STATUS: %X\n", usbdstatus) );

		}
		retval=_FAIL;
	}
	_spinunlock(&(pdev->in_lock));
exit:
	_func_exit_;
	return retval;

 }
extern NDIS_STATUS usb_dvobj_init(_adapter * padapter){
	u8 val8;
	u32 	blocksz;
	NDIS_STATUS			status=NDIS_STATUS_SUCCESS;
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;
	PURB						urb = NULL;
	PUSB_DEVICE_DESCRIPTOR		deviceDescriptor = NULL;

	_func_enter_;
	pdvobjpriv->padapter=padapter;

	padapter->EepromAddressSize = 6;

	NdisMGetDeviceProperty(padapter->hndis_adapter,
			&(pdvobjpriv->pphysdevobj),
			&(pdvobjpriv->pfuncdevobj),
			&(pdvobjpriv->pnextdevobj),
			NULL,
			NULL);

	//NDIS_STATUS					ndisstatus = NDIS_STATUS_SUCCESS;
	//PCE_USB_DEVICE				CEdevice = &Adapter->NdisCEDev;
//	PURB						urb = NULL;
//	PUSB_DEVICE_DESCRIPTOR		deviceDescriptor = NULL;
	
	pdvobjpriv->nextdevstacksz=(CHAR)pdvobjpriv->pnextdevobj->StackSize + 1; 
	//CEdevice->NextDeviceStackSize = (CHAR)CEdevice->pUsbDevObj->StackSize + 1; 

	urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST) );
	if( urb != NULL)
	{
		deviceDescriptor =(PUSB_DEVICE_DESCRIPTOR)_malloc((u32)sizeof(USB_DEVICE_DESCRIPTOR));
			// PlatformAllocateMemory((PVOID*)&deviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR) );
		if(deviceDescriptor != NULL){
				
			UsbBuildGetDescriptorRequest(
				urb,
				(USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
				USB_DEVICE_DESCRIPTOR_TYPE,
				0,
				0,
				deviceDescriptor,
				NULL,
				sizeof(USB_DEVICE_DESCRIPTOR),
				NULL);
			
			status = callusbd(pdvobjpriv, urb); 
			if( NT_SUCCESS(status) ) 
			{
				// display device descriptor
				DEBUG_INFO(("Device Descriptor len = 0x%x\n", urb->UrbControlDescriptorRequest.TransferBufferLength));
				DEBUG_INFO(("RTL8711 Device Descriptor:\n"));
				DEBUG_INFO(("-------------------------\n"));
				DEBUG_INFO(("bLength: 0x%x\n", deviceDescriptor->bLength));
				DEBUG_INFO(("bDescriptorType: 0x%x\n", deviceDescriptor->bDescriptorType));
				DEBUG_INFO(("bcdUSB: 0x%x\n", deviceDescriptor->bcdUSB));
				DEBUG_INFO(("bDeviceClass: 0x%x\n", deviceDescriptor->bDeviceClass));
				DEBUG_INFO(("bDeviceSubClass: 0x%x\n", deviceDescriptor->bDeviceSubClass));
				DEBUG_INFO(("bDeviceProtocol: 0x%x\n", deviceDescriptor->bDeviceProtocol));
				DEBUG_INFO(("bMaxPacketSize0: 0x%x\n", deviceDescriptor->bMaxPacketSize0));
				DEBUG_INFO(("idVendor: 0x%x\n", deviceDescriptor->idVendor));
				DEBUG_INFO(("idProduct: 0x%x\n", deviceDescriptor->idProduct));
				DEBUG_INFO(("bcdDevice: 0x%x\n", deviceDescriptor->bcdDevice));
				DEBUG_INFO(("iManufacturer: 0x%x\n", deviceDescriptor->iManufacturer));
				DEBUG_INFO(("iProduct: 0x%x\n", deviceDescriptor->iProduct));
				DEBUG_INFO(("iSerialNumber: 0x%x\n", deviceDescriptor->iSerialNumber));
				DEBUG_INFO(("bNumConfigurations: 0x%x\n", deviceDescriptor->bNumConfigurations));

				//3We can get DeviceID and VendorID here, should we do some check here? 
//				device->pUsbInfo.UsbDeviceDescriptor = deviceDescriptor;
//				device->IdVendor = deviceDescriptor->idVendor;
//				device->IdDevice = deviceDescriptor->idProduct;
				
				ExFreePool(urb);
		}
		else // deviceDescriptor alloc fail
		{
			ExFreePool(urb);
			DEBUG_ERR(("XXX CEusbStartDevice(): deviceDrscriptor MemAlloc() failed\n") );
			status =STATUS_INSUFFICIENT_RESOURCES;
			goto exit;
		}
			}
	}
	else // ( urb ) alloc fail
	{
		DEBUG_ERR(("XXX CEusbStartDevice(): urb ExAllocatePool() failed\n") );
		status= STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}
	_mfree((u8 *)deviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
//	PlatformFreeMemory(deviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));

	if ( configureusbdevice(pdvobjpriv) != STATUS_SUCCESS )
	{
		status= STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}

	if(padapter->dvobjpriv.ishighspeed ==_TRUE)
		{
			padapter->max_txcmd_nr = 100;
			padapter->scsi_buffer_size = 408;
		}
		else
		{
			padapter->max_txcmd_nr = 13;//USB1.1 only transfer 64 bytes in one packet
										//We use the first eight bytes for communication between driver and FW
										//so...60-8 = 52; 13 txcmd is equal to 56 bytes
			padapter->scsi_buffer_size = 60;
		}
exit:

	_func_exit_;
	return status;


}
extern void usb_dvobj_deinit(_adapter * padapter){
	
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;

	_func_enter_;


	_func_exit_;
}

NDIS_STATUS usb_rx_init(_adapter * padapter){
	u8 val8,i;
	u32 	blocksz;
	union recv_frame *prxframe;
	NDIS_STATUS			status=NDIS_STATUS_SUCCESS;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	struct intf_hdl * pintfhdl=&padapter->pio_queue->intf;
	struct recv_priv *precvpriv = &(padapter->recvpriv);

	_func_enter_;
	DEBUG_ERR(("\n ===> usb_rx_init \n"));
	_spinlock_init(&(pdev->in_lock));
	//issue Rx irp to receive data
	padapter->recvpriv.rx_pending_cnt=1;
	prxframe = (union recv_frame*)precvpriv->precv_frame_buf;
	for(i=0;i<32;i++){
		//DbgPrint("%d receive frame = %p \n",i,prxframe);
		//prxframe=alloc_recvframe(&padapter->recvpriv.free_recv_queue);
		usb_read_port(pintfhdl,0,0,(unsigned char *) prxframe);
		prxframe++;
	}
	//issue tsint irp to receive device interrupt
	
	pdev->ts_in_irp=IoAllocateIrp(pdev->nextdevstacksz, _FALSE);//pdev->tsint_irp[0]=IoAllocateIrp(pdev->nextdevstacksz, _FALSE);
//	pdev->tsint_irp[1]=IoAllocateIrp(pdev->nextdevstacksz, _FALSE);
//	pdev->tsint_idx=0;
	if((pdev->ts_in_irp== NULL) ){//if((pdev->tsint_irp[0]== NULL) ||(pdev->tsint_irp[1]== NULL) ){
		DEBUG_ERR(("allocate ts irp failed \n"));
		status = NDIS_STATUS_FAILURE;
		goto exit;
	}
	//for tx irp
	pdev->txirp_cnt=1;
	_init_sema(&(pdev->tx_retevt), 0);//NdisInitializeEvent(&pdev->txirp_retevt);

	pdev->ts_in_cnt=1;//pdev->tsint_cnt=1;
	_init_sema(&(pdev->hisr_retevt), 0);//NdisInitializeEvent(&pdev->interrupt_retevt);

	val8=read_usb_hisr(padapter);
	if(val8==_FAIL){
		DEBUG_ERR(("\n  usb_rx_init: read_usb_hisr=_FAIL=> status=NDIS_STATUS_FAILURE\n"));

		status=NDIS_STATUS_FAILURE;
	}
exit:
	DEBUG_ERR(("\n <=== usb_rx_init \n"));

	_func_exit_;
	return status;

}

NDIS_STATUS usb_rx_deinit(_adapter * padapter){
	struct dvobj_priv * pdev=&padapter->dvobjpriv;
	DbgPrint("\n ===> usb_rx_deinit \n");
	DEBUG_ERR(("\n ===> usb_rx_deinit \n"));

	DbgPrint("\n usb_rx_deinit :before usb_read_port_cancel\n");
	usb_read_port_cancel(padapter);
	DbgPrint("\n usb_rx_deinit :after usb_read_port_cancel before read_usb_hisr_cancel\n");
	read_usb_hisr_cancel(padapter);
	DbgPrint("\n usb_rx_deinit :after read_usb_hisr_cancel \n");

	DEBUG_ERR(("\n <=== usb_rx_deinit \n"));
	DbgPrint("\n <===  usb_rx_deinit \n");
	return _SUCCESS;


}

