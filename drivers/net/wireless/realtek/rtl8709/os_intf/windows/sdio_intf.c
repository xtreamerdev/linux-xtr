
#define _HCI_INTF_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>

#include <hal_init.h>


#ifndef CONFIG_SDIO_HCI

#error "CONFIG_SDIO_HCI shall be on!\n"

#endif



#define INTERRUPT_PASSIVE


void sd_sync_int_hdl ( PADAPTER padapter, u32 InterruptType );

static u8 test_pullup_resistor(_adapter *padapter)
{
	s8 i;
	u8 tmp = 0, status = _TRUE; 

	_func_enter_;
	// read CD bit and it should be 0
	attrib_read(padapter, 0x7, 1,&tmp);
	
	if(tmp & BIT(7)) {

		DEBUG_ERR(("Error (item15) => after pwr on , CD disable !=0 !!!\n"));

		status = _FALSE;
	}

	// write CD bit to 1
	tmp = tmp | BIT(7);

	attrib_write(padapter, 0x7, 1,&tmp);

	// write cccr_res bit to 1
	tmp = BIT(3);
	
	attrib_write(padapter, 0x6, 1,&tmp); // reset

	i = 10;

	while( i-- )
	{
		attrib_read(padapter, 0x6, 1,&tmp); // reset
		
		if(tmp & BIT(3))
		{
			DEBUG_ERR(("\n===test_pullup_resistor: polling RES to be 0 at %dth times=== \n", i));

			break;
		}
		
		//NdisMSleep(1000);
		msleep_os(1);
	}

	if (i < 0)	{ 	

		DEBUG_ERR(("\n===test_pullup_resistor: polling RES to be 0 fail === \n"));
		
		status = _FALSE;

	}	
	// read CD bit and it should be still 1
	
	attrib_read(padapter, 0x7, 1,&tmp);
	
	if(!(tmp & BIT(7))) {

		DEBUG_ERR(("Error (item15) => after reset , CD disable != 1 !!!\n"));	

		status = _FALSE;
	}
	_func_exit_;
	return status;
}


static u8 test_ldo_hsysclk(_adapter * padapter) //item45_46
{
	u8 tmp;
	u8 ret8=_TRUE;
	_func_enter_;
	tmp  =  read8(padapter, LDOOFF);

	if (tmp != 0) {
		
		DEBUG_ERR(("\n Error(item45) => LDO Init value should be 0\n"));
		ret8=_FALSE;
		goto exit;

	}
	
	tmp  =  read8(padapter, HSYSCLK);

	if (tmp != 0 ) {		
		DEBUG_ERR(("\n Error(item46) => HSYSCLK Init value should be 0\n"));
		ret8=_FALSE;
		goto exit;
	}
	
exit:	
	_func_exit_;
	return _TRUE;
}


#ifdef PLATFORM_OS_XP

static u32 init_sdio_priv_xp(_adapter * padapter) {

	u32	_status = _SUCCESS;

	u8 val8;
	u16	v16;
	u32 	blocksz;
	NDIS_STATUS			status;
	struct dvobj_priv *psddev=&padapter->dvobjpriv;

	_func_enter_;
	
	psddev->padapter = padapter;
	
	NdisMGetDeviceProperty(padapter->hndis_adapter,
			&(psddev->pphysdevobj),
			&(psddev->pfuncdevobj),
			&(psddev->pnextdevobj),
			NULL,
			NULL);
	
	psddev->sdbusinft.Size = sizeof(SDBUS_INTERFACE_STANDARD);
	psddev->sdbusinft.Version = SDBUS_INTERFACE_VERSION;

	status = SdBusOpenInterface (
			psddev->pphysdevobj,
			&psddev->sdbusinft,
			sizeof(SDBUS_INTERFACE_STANDARD),
			SDBUS_INTERFACE_VERSION);		

	DEBUG_INFO((" phcipriv->sdbusinft.Size = 0x%x\n",psddev->sdbusinft.Size));
	DEBUG_INFO((" phcipriv->sdbusinft.Version = 0x%x\n",psddev->sdbusinft.Version));
	DEBUG_INFO((" phcipriv->sdbusinft.Context = 0x%x\n",psddev->sdbusinft.Context));
		
	if (NT_SUCCESS(status)) 
	{
		SDBUS_INTERFACE_PARAMETERS interface_param = {0};
		interface_param.Size = sizeof(SDBUS_INTERFACE_PARAMETERS);
		interface_param.TargetObject =psddev->pnextdevobj; 
		interface_param.DeviceGeneratesInterrupts = _TRUE;

//#ifdef INTERRUPT_PASSIVE
		interface_param.CallbackAtDpcLevel = _FALSE;
		interface_param.CallbackRoutine = sd_sync_int_hdl;
//#endif

		interface_param.CallbackRoutineContext = (PVOID )padapter;
		_status = _FAIL;
		if (psddev->sdbusinft.InitializeInterface) 
		{
			status = 	psddev->sdbusinft.InitializeInterface
				(psddev->sdbusinft.Context, &interface_param);
		}
	}

	psddev->nextdevstacksz= (u8)psddev->pnextdevobj->StackSize + 1; 
	psddev->block_transfer_len=512;
	psddev->rxpktcount = 0;
	padapter->EepromAddressSize = 8;

	//4 Get Function Number 
	{//SDIOGetFuncNum(Adapter);
		PSDBUS_REQUEST_PACKET sdrp = NULL;
		UCHAR	*pLData;
		NTSTATUS Status ;				

		sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
		if(!sdrp){
			_status= FAIL;
			DEBUG_ERR(("\n  allocate sdrp failed \n"));
			goto exit ;
		}
		RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

		sdrp->RequestFunction = SDRF_GET_PROPERTY;
		sdrp->Parameters.GetSetProperty.Property = SDP_FUNCTION_NUMBER;
		sdrp->Parameters.GetSetProperty.Buffer = &psddev->func_number;//&sddev->FunctionNumber;
		sdrp->Parameters.GetSetProperty.Length = sizeof(psddev->func_number);//sizeof(sddev->FunctionNumber);
		Status =SdBusSubmitRequest (psddev->sdbusinft.Context,sdrp); //SdBusSubmitRequest (sddev->SdBusInterface.Context,sdrp);
				
		DEBUG_INFO(("SDdevice->FunctionNumber: %x\n ", psddev->func_number));	
							
		ExFreePool(sdrp);
	}
	if(!NT_SUCCESS(status)){ 
		_status=_FAIL;
		goto exit;
	}

	//4	 Get Bus Driver Version 
	{//SDIOGetFuncNum(Adapter);
		PSDBUS_REQUEST_PACKET sdrp = NULL;
		UCHAR	*pLData;
		NTSTATUS Status ;				

		sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
		if(!sdrp){
			_status= _FAIL;
			DEBUG_ERR(("\n  allocate sdrp failed \n"));
			goto exit ;
		}
		RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

		sdrp->RequestFunction = SDRF_GET_PROPERTY;
		sdrp->Parameters.GetSetProperty.Property = SDP_BUS_DRIVER_VERSION;
		sdrp->Parameters.GetSetProperty.Buffer =&psddev->driver_version; //&sddev->DriverVersion;
		sdrp->Parameters.GetSetProperty.Length = sizeof(psddev->driver_version);//sizeof(sddev->DriverVersion);
		Status = SdBusSubmitRequest (psddev->sdbusinft.Context,sdrp);//SdBusSubmitRequest (sddev->SdBusInterface.Context,sdrp);

		if (NT_SUCCESS(Status)) {	
			DEBUG_ERR(("pdvobjpriv->driver_version: %x\n ", psddev->driver_version));	
		}
		
		ExFreePool(sdrp);
	}

	//4 Check Host Block Size
	if (psddev->driver_version >= SDBUS_DRIVER_VERSION_2)
	{

		PSDBUS_REQUEST_PACKET sdrp = NULL;
		UCHAR	*pLData;
		NTSTATUS Status ;				

		sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
		if(!sdrp){
			_status= _FAIL;
			DEBUG_ERR(("\n  allocate sdrp failed \n"));
			goto exit ;
		}
		RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

		sdrp->RequestFunction = SDRF_GET_PROPERTY;
		sdrp->Parameters.GetSetProperty.Property = SDP_HOST_BLOCK_LENGTH;
		sdrp->Parameters.GetSetProperty.Buffer = &v16;
		sdrp->Parameters.GetSetProperty.Length = sizeof(v16);
		Status =SdBusSubmitRequest (psddev->sdbusinft.Context,sdrp); //SdBusSubmitRequest (sddev->SdBusInterface.Context,sdrp);

		if (NT_SUCCESS(Status)) {	
			DEBUG_ERR(("Host Block Length: %x\n ", v16));	
		}
		ExFreePool(sdrp);

	}


	//4 check Function Block Size
	if (psddev->driver_version>= SDBUS_DRIVER_VERSION_2)
	{

		PSDBUS_REQUEST_PACKET sdrp = NULL;
		UCHAR	*pLData;
		NTSTATUS Status ;				

		sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
		if(!sdrp){
			_status= _FAIL;
			DEBUG_ERR(("\n  allocate sdrp failed \n"));
			goto exit ;
		}
		RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

		sdrp->RequestFunction = SDRF_GET_PROPERTY;
		sdrp->Parameters.GetSetProperty.Property = SDP_FUNCTION_BLOCK_LENGTH;
		sdrp->Parameters.GetSetProperty.Buffer = &v16;
		sdrp->Parameters.GetSetProperty.Length = sizeof(v16);
		Status =SdBusSubmitRequest (psddev->sdbusinft.Context,sdrp);// SdBusSubmitRequest (sddev->SdBusInterface.Context,sdrp);

		if (NT_SUCCESS(Status)) {	
			DEBUG_ERR(("FUNCTION Block Length: %x\n ", v16));	
		}
		ExFreePool(sdrp);

	}


	//4 check Function Block Size
	if (psddev->driver_version >= SDBUS_DRIVER_VERSION_2)
	{

		PSDBUS_REQUEST_PACKET sdrp = NULL;
		UCHAR	*pLData;
		NTSTATUS Status ;				

		sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
		if(!sdrp){
			_status= _FAIL;
			DEBUG_ERR(("\n  allocate sdrp failed \n"));
			goto exit ;
		}
		RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

		sdrp->RequestFunction = SDRF_GET_PROPERTY;
		sdrp->Parameters.GetSetProperty.Property = SDP_FN0_BLOCK_LENGTH;
		sdrp->Parameters.GetSetProperty.Buffer = &v16;
		sdrp->Parameters.GetSetProperty.Length = sizeof(v16);
		Status =SdBusSubmitRequest (psddev->sdbusinft.Context,sdrp);// SdBusSubmitRequest (sddev->SdBusInterface.Context,sdrp);

		if (NT_SUCCESS(Status)) {	
			DEBUG_ERR(("FUNCTION 0 Block Length: %x\n ", v16));	
		}
		ExFreePool(sdrp);

	}


	//Set Host Block Size
	if (psddev->driver_version  >= SDBUS_DRIVER_VERSION_2)
	{

		PSDBUS_REQUEST_PACKET sdrp = NULL;
		UCHAR	*pLData;
		NTSTATUS Status ;				

		sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
		if(!sdrp){
			_status= _FAIL;
			DEBUG_ERR(("\n  allocate sdrp failed \n"));
			goto exit ;
		}
		RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

		v16 = 512;
		
		sdrp->RequestFunction = SDRF_SET_PROPERTY;
		sdrp->Parameters.GetSetProperty.Property = SDP_HOST_BLOCK_LENGTH;
		sdrp->Parameters.GetSetProperty.Buffer = &v16;
		sdrp->Parameters.GetSetProperty.Length = sizeof(v16);
		Status =SdBusSubmitRequest (psddev->sdbusinft.Context,sdrp);// SdBusSubmitRequest (sddev->SdBusInterface.Context,sdrp);

		if (NT_SUCCESS(Status)) {	
			DEBUG_ERR(("Set Host Block Length: %x\n ", v16));	
		}
		ExFreePool(sdrp);

	}	


	//4 check Host Block Size after setting
	if (psddev->driver_version  >= SDBUS_DRIVER_VERSION_2)
	{

		PSDBUS_REQUEST_PACKET sdrp = NULL;
		UCHAR	*pLData;
		NTSTATUS Status ;				

		sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
		if(!sdrp){
			_status= _FAIL;
			DEBUG_ERR(("\n  allocate sdrp failed \n"));
			goto exit ;
		}
		RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

		sdrp->RequestFunction = SDRF_GET_PROPERTY;
		sdrp->Parameters.GetSetProperty.Property = SDP_HOST_BLOCK_LENGTH;
		sdrp->Parameters.GetSetProperty.Buffer = &v16;
		sdrp->Parameters.GetSetProperty.Length = sizeof(v16);
		Status = SdBusSubmitRequest (psddev->sdbusinft.Context,sdrp);//SdBusSubmitRequest (sddev->SdBusInterface.Context,sdrp);

		if (NT_SUCCESS(Status)) {	
			DEBUG_ERR(("Host Block Length: %x\n ", v16));	
		}
		ExFreePool(sdrp);

	}


	//4 Set Function Block Size
	if (psddev->driver_version  >= SDBUS_DRIVER_VERSION_2)
	{

		PSDBUS_REQUEST_PACKET sdrp = NULL;
		UCHAR	*pLData;
		NTSTATUS Status ;				

		sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
		if(!sdrp){
			_status= _FAIL;
			DEBUG_ERR(("\n  allocate sdrp failed \n"));
			goto exit ;
		}
		RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

		v16 = 512;
		
		sdrp->RequestFunction = SDRF_SET_PROPERTY;
		sdrp->Parameters.GetSetProperty.Property = SDP_FUNCTION_BLOCK_LENGTH;
		sdrp->Parameters.GetSetProperty.Buffer = &v16;
		sdrp->Parameters.GetSetProperty.Length = sizeof(v16);
		Status = SdBusSubmitRequest (psddev->sdbusinft.Context,sdrp);//SdBusSubmitRequest (sddev->SdBusInterface.Context,sdrp);

		if (NT_SUCCESS(Status)) {	
			DEBUG_ERR(("Set Function Block Length: %x\n ", v16));	
		}
		ExFreePool(sdrp);

	}	


	//4 check Host Block Size after setting
	if (psddev->driver_version >= SDBUS_DRIVER_VERSION_2)
	{

		PSDBUS_REQUEST_PACKET sdrp = NULL;
		UCHAR	*pLData;
		NTSTATUS Status ;				

		sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
		if(!sdrp){
			_status= _FAIL;
			DEBUG_ERR(("\n  allocate sdrp failed \n"));
			goto exit ;
		}
		RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

		sdrp->RequestFunction = SDRF_GET_PROPERTY;
		sdrp->Parameters.GetSetProperty.Property = SDP_FUNCTION_BLOCK_LENGTH;
		sdrp->Parameters.GetSetProperty.Buffer = &v16;
		sdrp->Parameters.GetSetProperty.Length = sizeof(v16);
		Status = SdBusSubmitRequest (psddev->sdbusinft.Context,sdrp);//SdBusSubmitRequest (sddev->SdBusInterface.Context,sdrp);

		if (NT_SUCCESS(Status)) {	
			DEBUG_ERR(("Function Block Length: %x\n ", v16));	
		}
		ExFreePool(sdrp);

	}

	if (psddev->driver_version< SDBUS_DRIVER_VERSION_2) {
		psddev->block_mode= 0;
    	} else {
        psddev->block_mode= 1;
    	}
		
exit:
	return status;

}


#endif

NDIS_STATUS sd_dvobj_init(_adapter * padapter){

       u32 status;
	 u8 val8;
	 int blocksz;
	 struct dvobj_priv *psddev=&padapter->dvobjpriv;
//	struct registry_priv *registry_par = &(padapter->registrypriv);

	psddev->padapter=padapter;
#ifdef PLATFORM_OS_XP

	status = init_sdio_priv_xp(padapter);

#endif

#ifdef PLATFORM_OS_WINCE

	status = init_sdio_priv_wince(padapter);

#endif
		
	if(status == _FAIL) 
		goto error;

	if(test_pullup_resistor(padapter) == _FALSE){ //item 15 //initialize		
			DEBUG_ERR(("\n\n####### Error(item 15) => test_pullup_resistor fail #######\n\n" ));
	}



		
		//4 Write Function 0 Block Size
		blocksz = psddev->block_transfer_len;
		val8 = (u8)(blocksz & 0xFF);
		attrib_write(padapter, 0x10, 1, &val8);
		val8 = (u8)((blocksz >> 8) & 0xFF);
		attrib_write(padapter, 0x11, 1, &val8);

		//4 Write Function 1 Block Size
		val8 = (u8)(blocksz & 0xFF);
		attrib_write(padapter, 0x110, 1, &val8);
		val8 = (u8)((blocksz >> 8) & 0xFF);
		attrib_write(padapter, 0x111, 1, &val8);

		if(test_ldo_hsysclk( padapter ) == _FALSE) 
		{
			DEBUG_ERR(("\n\n####### Error(item45_46) => test_ldo_hsysclk fail #######\n\n"));

			goto error;
		}

		_func_exit_;
		return status;
error:
	_func_exit_;
	return status;

}

void sd_dvobj_deinit(_adapter * padapter){
	
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;

	_func_enter_;

#ifdef PLATFORM_OS_XP

	if (pdvobjpriv->sdbusinft.InterfaceDereference)//(sddev->SdBusInterface.InterfaceDereference) 
	{
		(pdvobjpriv->sdbusinft.InterfaceDereference)(pdvobjpriv->sdbusinft.Context);//(sddev->SdBusInterface.InterfaceDereference) (sddev->SdBusInterface.Context);
		memset(&pdvobjpriv->sdbusinft, 0, sizeof(SDBUS_INTERFACE_STANDARD));
	}

#endif

	_func_exit_;
}



//***********************************
//
//Rd HRFF0 Bit
//
//***********************************
u8
SDIO_RdHRFF0(
	PADAPTER			padapter
)

{
	u16	 Temp16=0; 
	s8 	 i=0;
	u8 status= _FALSE;
	struct dvobj_priv*psddev= &padapter->dvobjpriv;
_func_enter_;
	do{// polling ready bit
		//i++;			

		Temp16 = read16(padapter, HFFR);

		if ( Temp16 & _HFFR_HRFF0_READTY){
			_func_exit_;
				status = _TRUE;
		}
		psddev->rxpktcount= Temp16 >> 12;

		if (psddev->rxpktcount <= 1)
			psddev->rxpktcount = 1;

#if 0
		if (status == TRUE)
			psddev->rxpktcount ++;
#endif		
		//DEBUG_ERR("SDIO_RdHRFF0 %x", Temp16);
		
	}while(0);
_func_exit_;
	return status;
}



//***********************************
//
//Rd HRFF0 Bit
//
//***********************************
/*
u8
SDIO_PollHRFF0Rdy(
	PADAPTER			padapter
)

{
	u16	Temp16=0; 
	s8 	i=0;
_func_enter_;
	do{

		Temp16 = read16(padapter, HFFR);

		if ( Temp16 & _HFFR_HRFF0_READTY){
			_func_exit_;
				return _TRUE;
		}
		
		i++;					
	}while(i <5);
_func_exit_;
	DEBUG_ERR( ("\n SDIO_PollHRFF0Rdy: Polling HFFR0_READY Bit FAIL !!!!\n"));
	return _FALSE;
}

*/


//#ifndef RD_BLK_MODE 
u8 sd_sync_rx_byte(PADAPTER	Adapter)
{
	struct dvobj_priv*	psddev = &Adapter->dvobjpriv;
	u8	i,			_Rdy = _FALSE;
	u32 polling_cnt=0;



	while(1)
	{


	if(Adapter->bDriverStopped || Adapter->bSurpriseRemoved)
	{
		DEBUG_ERR(("\n***sd_sync_rx : NIC will be unloaded!!!***\n"));
		return _TRUE;
	}

		_Rdy= SDIO_RdHRFF0(Adapter);

		if(!_Rdy)
		{
			DEBUG_ERR(("\n*** HCI RX FF Not Ready ***\n"));
				
			if(polling_cnt == 0)
				break;


			polling_cnt++;
			
			if(polling_cnt < 32)
			{
				usleep_os(1);
				continue;

			}
			else
			{
				break;
			}

			break;
		}		

              polling_cnt = 1;			  

             	for (i = 0; i < Adapter->dvobjpriv.rxpktcount; i++)
		{
	       	 if(recv_entry(Adapter)!=NDIS_STATUS_SUCCESS)
		  		DEBUG_ERR(("\n^^^^recv_entry Fail ^^^^\n"));           
		}		  
	}

	return _TRUE;

}

//#else

u8 sd_sync_rx_blk(PADAPTER	Adapter)
{
	struct dvobj_priv*	psddev = &Adapter->dvobjpriv;

	if(Adapter->bDriverStopped || Adapter->bSurpriseRemoved)
	{
		DEBUG_ERR(("\n***sd_sync_rx : NIC will be unloaded!!!***\n"));
		return _TRUE;
	}

	while(1)
	{		
	      if(recv_entry(Adapter) == state_no_rx_data)
		break;
	}

	return _TRUE;
}
//#endif


u8 sd_sync_rx(PADAPTER	padapter)
{
	 struct dvobj_priv *psddev=&padapter->dvobjpriv;
	struct registry_priv *registry_par = &(padapter->registrypriv);
	u8 blk_mode = _FALSE;



	if ((registry_par->chip_version >= RTL8711_3rdCUT) || (registry_par->chip_version == RTL8711_FPGA))
	{
		if(psddev->block_mode==1)
	{

		#ifdef RD_BLK_MODE 
		blk_mode = _TRUE;
		#endif
	}
	}


	if(blk_mode)
		return sd_sync_rx_blk(padapter)	;
	else
		return sd_sync_rx_byte(padapter)	;

	
}

void rxdone_hdl (_adapter *padapter)
{
_func_enter_;
	//4  Read Data
	sd_sync_rx(padapter);
_func_exit_;
	
}


//************************************
//Sdio Interrupt ISR
//************************************
u8  sd_int_isr (IN PADAPTER	padapter)
{
	u32		IsrContent;
	//u8		tmp;

	/* TODO Perry: We should update IsrContent as follows:
		Adapter->IsrContent &= pHalData->IntrMask;
		So, ISR doesn't need to call unnecessary functions while IntrMask is off.
	*/

_func_enter_;
#if 1
	padapter->IsrContent = IsrContent = read32(padapter, HISR);
#else	
       sync_ioreq_read32(padapter, HISR, &IsrContent);
       sync_ioreq_write32(padapter, HIMR, 0);
       sync_ioreq_flush(padapter, (struct io_queue*)padapter->pio_queue);	 
#endif

  
	   
       padapter->IsrContent = IsrContent;	  
_func_exit_;
	return ((padapter->IsrContent & padapter->ImrContent)!=0) ;
#if 0
	// TODO: Perry: check this.
	if (IsrContent)
		return _TRUE;
	else
		return _FALSE;
#endif	
}



//************************************
//Sdio Interrupt Deferred_Procedure_Call
//************************************
void sd_int_dpc( PADAPTER	padapter)
{
	
	
	
	struct	evt_priv	*pevt_priv = &(padapter->evtpriv);
	struct	cmd_priv	*pcmd_priv = &(padapter->cmdpriv);
	struct 	xmit_priv *pxmitpriv= &(padapter->xmitpriv);
	
	struct	io_queue  *pio_queue = (struct io_queue *)padapter->pio_queue;
	struct 	intf_priv	*pintfpriv = pio_queue->intf.pintfpriv;

	u32	txdonebits = _VOACINT | _VIACINT | _BEACINT| _BKACINT|_BMACINT;

	uint tasks = (padapter->IsrContent & padapter->ImrContent);
_func_enter_;

	if( (tasks & _C2HSET )) // C2HSET
	{
		//DEBUG_ERR("sd_int_dpc _C2HSET  ");
		padapter->IsrContent  ^= _C2HSET;
		//_up_sema(&(pevt_priv->evt_notify));
		/* Perry:
			C2H interrupt tells driver that FW is staying at 40M.
		*/
		register_evt_alive(padapter);
		evt_notify_isr(pevt_priv);
		
		//NdisSetEvent(&(Adapter->C2HSET_InterruptTest));
	}	
	
	if(tasks & _RXDONE){

		//DEBUG_ERR("sd_int_dpc _RXDONE  ");
		
		padapter->IsrContent  ^= _RXDONE;				
		/* Perry : 
			Rx interrupt informs driver that FW is staying at 40M clk.
		*/
		register_rx_alive(padapter);
		rxdone_hdl(padapter);
	
	}
	
	/*	Perry:
		NOTE:
		RxDone and C2H event must be handled before CPWM handler.
	*/
       if(tasks & _CPWM) // CPWM_INT
	{
		//DEBUG_ERR("sd_int_dpc _CPWM  ");
	
		padapter->IsrContent  ^= BIT(17);
		DEBUG_ERR(("SDIOINTDpc(): CPWM_INT\n"));
		//NdisSetEvent(&(adapter->CPWM_InterruptTest));
		//if(adapter->mppriv.cpwm_flag == 0)
		//          adapter->mppriv.cpwm_flag = 1;

#ifdef CONFIG_PWRCTRL
		// add cpwm handler here (Perry)
		cpwm_int_hdl(padapter);
#endif
	}
	

	if( (tasks & _H2CCLR)) // H2CCLR
	{
		//DEBUG_ERR("sd_int_dpc _H2CCLR  ");
	
		padapter->IsrContent  ^= _H2CCLR;
		cmd_clr_isr(pcmd_priv);
		
		//NdisSetEvent(&(Adapter->H2CCLR_InterruptTest));
	}


	if(tasks & _RXERR){
		padapter->IsrContent  ^= _RXERR;
		//DEBUG_ERR(("sd_int_dpc _RXERR  "));
		
	}	

	
	if (tasks & _TXFFERR) {

		
		padapter->IsrContent ^= _TXFFERR;

	}



	
_func_exit_;
	return;

}

//SdioSynDriverCallback
void sd_sync_int_hdl ( PADAPTER padapter, u32 InterruptType )
{
	struct dvobj_priv*	psddev =&padapter->dvobjpriv;
	NDIS_STATUS		ndis_status =NDIS_STATUS_SUCCESS;

#ifdef CONFIG_PWRCTRL	
	u8 	cpwm;
#endif

_func_enter_;
	if( (padapter->bDriverStopped ==_TRUE) || (padapter->bSurpriseRemoved == _TRUE)){
		goto exit;
	}

#ifdef CONFIG_PWRCTRL
	/* Perry:
		Fix 32K HISR & HIMR problems.
	*/
	if(padapter->pwrctrlpriv.cpwm >= FW_PWR3) {
		cpwm = hwcpwm2swcpwm(read8(padapter, HCPWM));
		if(cpwm >= FW_PWR2) {
			DEBUG_ERR(("==== Error, This should not happen. fw_cpwm:%d himr:%x cpwm:%x\n", cpwm, read32(padapter, HIMR), read8(padapter, HCPWM)));
			/* Interrupt arrives but power level = P2 or P3 */
			padapter->pwrctrlpriv.tgt_rpwm = FW_PWR1;
			write8(padapter, HRPWM, swrpwm2hwrpwm(FW_PWR1));
			/* Perry:
				Because we didn't read HISR, return from here will cause OS invokes this ISR again.
				This works like polling method.
			*/
			if (psddev->sdbusinft.AcknowledgeInterrupt){
				(psddev->sdbusinft.AcknowledgeInterrupt)
					(psddev->sdbusinft.Context);
			}			
			goto exit;
		} else {
			/*Restore masks. */
			DEBUG_ERR(("restore HIMR to %x\n", padapter->pwrctrlpriv.ImrContent));
			padapter->ImrContent = padapter->pwrctrlpriv.ImrContent;
			write32(padapter, HIMR, padapter->ImrContent);
		}
	}
#endif

	if(sd_int_isr(padapter)){
		
		//write32(adapter, HIMR, 0);
		
		sd_int_dpc(padapter);		
	}
	else
	{
		DEBUG_ERR(("\n\n<=========== sd_sync_int_hdl(): not our INT\n\n"));	
	}

#ifdef PLATFORM_OS_XP

	if (psddev->sdbusinft.AcknowledgeInterrupt){
		 (psddev->sdbusinft.AcknowledgeInterrupt)
			(psddev->sdbusinft.Context);
	}
	
#endif
#if 0
	write32(padapter, HIMR, padapter->ImrContent);
#endif
exit:
_func_exit_;
	return ;
}


