
#define _CFIO_INTF_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>
#include <hal_init.h>


#define NIC_RESOURCE_BUF_SIZE           (sizeof(NDIS_RESOURCE_LIST) + \
                                        (10*sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)))

NDIS_STATUS cf_dvobj_init(_adapter * padapter){

	u8					index;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR pres_desc;
	u8					res_buf[NIC_RESOURCE_BUF_SIZE];
	u8				bres_port = _FALSE, bres_interrupt = _FALSE, bres_memory = _FALSE;
	u32					errorcode = 0;
	u32					errorvalue = 0;
	uint					bufsize = NIC_RESOURCE_BUF_SIZE;
	PNDIS_RESOURCE_LIST	res_list = (PNDIS_RESOURCE_LIST)res_buf;
	NDIS_STATUS			status=NDIS_STATUS_SUCCESS;
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;
	

	_func_enter_;

	//Cfio find adapter
	NdisMQueryAdapterResources(
		&status, 
		padapter->hndis_config, //WrapperConfigurationContext, 
		res_list, 
		&bufsize);
	
	 if (status != NDIS_STATUS_SUCCESS)
        {
		errorcode = NDIS_ERROR_CODE_RESOURCE_CONFLICT;
		errorvalue = 0x101;
		goto exit;
        }

	 for (index=0; index < res_list->Count; index++)
        {
            pres_desc = &res_list->PartialDescriptors[index];

            switch(pres_desc->Type)
            {
                case CmResourceTypePort:
                    pdvobjpriv->io_base_address = NdisGetPhysicalAddressLow(pres_desc->u.Port.Start); 
                    pdvobjpriv->io_range = pres_desc->u.Port.Length;
                    bres_port = _TRUE;

                    DEBUG_INFO(("pdvobjpriv->io_base_address = 0x%x\n", pdvobjpriv->io_base_address));
                    DEBUG_INFO(("pdvobjpriv->io_range  = x%x\n", pdvobjpriv->io_range ));
                    break;

                case CmResourceTypeInterrupt:
                    pdvobjpriv->interrupt_level = pres_desc->u.Interrupt.Level;
                    bres_interrupt = _TRUE;
                    
                    DEBUG_INFO(("pdvobjpriv->interrupt_level  = x%x\n", pdvobjpriv->interrupt_level ));
                    break;

                case CmResourceTypeMemory:
                    // Our CSR memory space should be 0x1000, other memory is for 
                    // flash address, a boot ROM address, etc.
                    if (pres_desc->u.Memory.Length == 0x1000)
                    {
                    	pdvobjpriv->mem_phys_address = pres_desc->u.Memory.Start;
				bres_memory = _TRUE;
				DEBUG_INFO(("MemPhysAddress(Low) = 0x%0x\n", NdisGetPhysicalAddressLow(pdvobjpriv->mem_phys_address )));
                    	DEBUG_INFO(("MemPhysAddress(High) = 0x%0x\n", NdisGetPhysicalAddressHigh(pdvobjpriv->mem_phys_address)));
                    }
                    break;
            }
        } 
	 
        //
        // Map bus-relative IO range to system IO space
        //
        status= NdisMRegisterIoPortRange(
                     (PVOID *)&pdvobjpriv->port_offset,
                     padapter->hndis_adapter,
                     pdvobjpriv->io_base_address,
                     pdvobjpriv->io_range);
        if (status!= NDIS_STATUS_SUCCESS)
        {
            DEBUG_ERR(("\nNdisMRegisterioPortRange failed\n"));
    
            NdisWriteErrorLogEntry(
                padapter->hndis_adapter,
                NDIS_ERROR_CODE_BAD_IO_BASE_ADDRESS,
                0);
        
            goto exit;
        }

	// Register the interrupt
	status = NdisMRegisterInterrupt(
			&pdvobjpriv->interrupt, //&CEdevice->Interrupt,  
	             padapter->hndis_adapter,//MiniportAdapterHandle,
			pdvobjpriv->interrupt_level,		//CEdevice->InterruptLevel,
			pdvobjpriv->interrupt_level,		//CEdevice->InterruptLevel,
			_TRUE,       // RequestISR
			_TRUE,       // SharedInterrupt
			NdisInterruptLevelSensitive  );

	if(status != NDIS_STATUS_SUCCESS)
	{
		DEBUG_ERR(("\nNdisMRegisterInterrupt failed\n"));			
		DEBUG_ERR(("\nNdisstatus =  %x\n", status));			
	}

	
exit:
	_func_exit_;
	return status;


}

void cf_dvobj_deinit(_adapter * padapter){
	
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;

	_func_enter_;


	_func_exit_;
}





