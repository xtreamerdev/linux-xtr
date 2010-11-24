/******************************************************************************
* sdio_intf.c                                                                                                                                 *
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

#define _HCI_INTF_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>


#include <hal_init.h>
#include <linux/sdio/_sdio_defs.h>
#include <linux/sdio/sdio_lib.h>




#define INTERRUPT_PASSIVE 1


void sd_sync_int_hdl(void *pcontext);


extern unsigned int sd_dvobj_init(_adapter * padapter){
	
	
	//unsigned int	status;	
	#ifdef no_bus_drv
	u16 func_num;
	u16 rwflag,reg_addr,write_data;
	u32 arg;
	u8 *pwrite;
	#else
	SDCONFIG_FUNC_ENABLE_DISABLE_DATA fData;
	#endif
	SDIO_STATUS status = SDIO_STATUS_SUCCESS;
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;
	PSDDEVICE psddev = pdvobjpriv->psddev;
	struct registry_priv *registry_par = &(padapter->registrypriv);

	_func_enter_;

	//psddev->block_transfer_len=512;
	//psddev->rxpktcount = 0;
	padapter->EepromAddressSize = 8;


       DEBUG_ERR((" Stack Version: %d.%d \n", SDDEVICE_GET_VERSION_MAJOR(psddev),
	   	                                                         SDDEVICE_GET_VERSION_MINOR(psddev)));
       DEBUG_ERR((" Card Flags:   0x%X \n",SDDEVICE_GET_CARD_FLAGS(psddev)));
       //DEBUG_ERR((" Card RCA:     0x%X \n",SDDEVICE_GET_CARD_RCA(psddev)));
       DEBUG_ERR((" Oper Clock:   %d Hz \n",SDDEVICE_GET_OPER_CLOCK(psddev)));
       DEBUG_ERR((" Max Clock:    %d Hz \n",SDDEVICE_GET_MAX_CLOCK(psddev)));
       DEBUG_ERR((" Oper Blksz:  %d bytes \n",SDDEVICE_GET_OPER_BLOCK_LEN(psddev)));
       DEBUG_ERR((" Max  Blksz:     %d bytes\n",SDDEVICE_GET_MAX_BLOCK_LEN(psddev)));
       DEBUG_ERR((" Oper BlkCnts:    %d blocks per trans \n",SDDEVICE_GET_OPER_BLOCKS(psddev)));
       DEBUG_ERR((" Max  BlkCnts:       %d blocks per trans \n",SDDEVICE_GET_MAX_BLOCKS(psddev)));
       DEBUG_ERR((" Slot Voltage Mask:  0x%X \n",SDDEVICE_GET_SLOT_VOLTAGE_MASK(psddev)));

       if ((SDDEVICE_GET_CARD_FLAGS(psddev) & CARD_SDIO) &&
            (SDDEVICE_GET_SDIO_FUNCNO(psddev) != 0))
       {
            DEBUG_ERR((" SDIO Func:  %d\n", SDDEVICE_GET_SDIO_FUNCNO(psddev)));
            DEBUG_ERR((" CIS PTR:    0x%X \n", SDDEVICE_GET_SDIO_FUNC_CISPTR(psddev)));            
        }
	
	
	//4   1.enable the function  (I/O enable I/O ready)
	#ifdef no_bus_drv
	 func_num = 0;
	 rwflag = 1;
        reg_addr = 0x0002;
        write_data = 0x02;
	 *pwrite = write_data;

	status = SDLIB_IssueCMD52(func_num,reg_addr,pwrite,1,rwflag);

	#else
        fData.EnableFlags = SDCONFIG_ENABLE_FUNC;
        fData.TimeOut = 500;//ms
        status = SDLIB_IssueConfig(psddev,
                                                  SDCONFIG_FUNC_ENABLE_DISABLE,
                                                  &fData,
                                                  sizeof(fData));
	#endif
	
        if (!SDIO_SUCCESS((status))) {
			
            	DEBUG_ERR(("sd_dvobj_init: failed to enable function %x\n",
                                    status));
				
            	goto sd_dvobj_init_error;
        }

	#ifdef no_bus_drv 
	//4  2. set bus width	
	func_num = 0;
	rwflag = 1;
       reg_addr = 0x0007;
       write_data = 0x02;
	*pwrite = write_data;
       // arg = (rwFlag<<31) | (funcNum<<28) | (rawFlag<<27) | ((regAddr)<<9) | writeData ;
       
	status = SDLIB_IssueCMD52(func_num,reg_addr,pwrite,1,rwflag);

	  if (!SDIO_SUCCESS((status))) {
			
            	DEBUG_ERR(("sd_dvobj_init: failed to set bus width %x\n",
                                    status));
				
            	goto sd_dvobj_init_error;
      	 }
	#endif

        /* check if the card support multi-block transfers */
        if (!(SDDEVICE_GET_SDIOCARD_CAPS(psddev) & SDIO_CAPS_MULTI_BLOCK))
	{
            DEBUG_INFO(("Byte Count Mode only\n"));

            pdvobjpriv->block_mode = 0;	    
           
        }else {
             DEBUG_INFO(("Multi-Block Mode\n"));
             pdvobjpriv->block_mode = 1;

	      /* add specific card initialization or check here */
		//4  3.set the function block size  
 	      status =  SDLIB_SetFunctionBlockSize(psddev, 512);//set the blocklen for func no1.
 	      if (!SDIO_SUCCESS(status)) {
	 	
                   DEBUG_ERR(("sd_dvobj_init: block length set failed %x \n", status));
				   
                   goto sd_dvobj_init_error;
		   
             }
        }		
        	
	//force to byte count mode , because ENE host controller bugs
	pdvobjpriv->block_mode = 0;
	
#ifdef RD_BLK_MODE
	if ((registry_par->chip_version >= RTL8711_3rdCUT) && (psddev->block_mode==1) )
	{
		//#define  RD_BLK_MODE 1
		
#ifdef RD_BLK_MODE 
		//switch dbg port
		write32(padapter, 0x1d622008, 0x090080);
		//blk mode
		write8(padapter, 0x1d600032, 0xff);
		write8(padapter, 0x1d600035, 0x44);
#endif
	}

#endif		
		
#ifdef INTERRUPT_PASSIVE
         /* register for sync interrupt handle */		         
        SDDEVICE_SET_IRQ_HANDLER(psddev, sd_sync_int_hdl, (void *)padapter);
#endif

        //4  Int enable
	#ifdef no_bus_drv
	func_num = 0;
	rwflag = 1;
       reg_addr = 0x0004;
       write_data = 0x03;
	*pwrite = write_data;     
	status = SDLIB_IssueCMD52(func_num,reg_addr,pwrite,1,rwflag);

	#else
        status = SDLIB_IssueConfig(psddev, SDCONFIG_FUNC_UNMASK_IRQ, NULL, 0);
	#endif
	
        if (!SDIO_SUCCESS((status))) {
			
             DEBUG_ERR(("sd_dvobj_init: failed to enable interrupt %x\n",
                                    status));
			 
	     goto sd_dvobj_init_error;
        }

	
	_func_exit_;
	return _SUCCESS;
	
sd_dvobj_init_error:
	
	_func_exit_;
	return _FAIL;

}
extern void sd_dvobj_deinit(_adapter * padapter)
{
       unsigned char data;      
       SDIO_STATUS status;
//       SDCONFIG_BUS_MODE_DATA busSettings;
       SDCONFIG_FUNC_ENABLE_DISABLE_DATA fData;
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;
	PSDDEVICE psddev = pdvobjpriv->psddev;

_func_enter_;

  if (psddev != NULL) {
		
	 DEBUG_ERR(("+SDIO deinit\n"));	

    if (!(SDDEVICE_IS_CARD_REMOVED(psddev))) {

  	 /* Unregister the IRQ Handler */
        SDDEVICE_SET_IRQ_HANDLER(psddev, NULL, NULL);	
	 
        /* Disable the host controller interrupts */
      	 status = SDLIB_IssueConfig(psddev, SDCONFIG_FUNC_MASK_IRQ, NULL, 0);    
	 if (!SDIO_SUCCESS((status))) {
	 	 DEBUG_ERR(("sd_dvobj_init: failed to disable the host controller interrupts , %x\n",  status));	     
        }	
        

	memset(&fData, 0, sizeof(fData));
        /* Disable the card */
        fData.EnableFlags = SDCONFIG_DISABLE_FUNC;
        fData.TimeOut = 100;
        status = SDLIB_IssueConfig(psddev, SDCONFIG_FUNC_ENABLE_DISABLE, &fData, sizeof(fData));
        if (!SDIO_SUCCESS((status))) {
	 	 DEBUG_ERR(("sd_dvobj_init: failed to disable the card , %x\n",  status));	     
         }		
        
#if 1
        /* Perform a soft I/O reset */
        data = SDIO_IO_RESET;
        status = SDLIB_IssueCMD52(psddev, 0, SDIO_IO_ABORT_REG, &data, 1, 1);
	 if (!SDIO_SUCCESS((status))) {
	 	 DEBUG_ERR(("sd_dvobj_init: failed to perform a soft I/O reset , %x\n",  status));	     
         }	
#endif		

    }
     else {
        DEBUG_ERR( ("SDIO deinit: Card has been removed \n"));
    }	

#if 0
	 /* Remove the allocated current if any */
        status = SDLIB_IssueConfig(psddev, SDCONFIG_FUNC_FREE_SLOT_CURRENT, NULL, 0);   
	 if (!SDIO_SUCCESS((status))) {
	 	 DEBUG_ERR(("sd_dvobj_init: failed to remove the allocated current , %x\n",  status));	     
        }
#endif			 

  }

    DEBUG_ERR(("-SDIO deinit\n"));
	
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
//	s8 	 i=0;
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



#ifndef RD_BLK_MODE 
u8 sd_sync_rx(PADAPTER	Adapter)
{
	//struct dvobj_priv*	psddev = &Adapter->dvobjpriv;
	_nic_hdl pnetdev = (_nic_hdl)Adapter->pnetdev;	
	u8	i,			_Rdy = _FALSE;
	u32 polling_cnt=0;

	//DEBUG_ERR(("\n\n\nAdapter->bDriverStopped=%d, Adapter->bSurpriseRemoved=%d\n\n\n\n",Adapter->bDriverStopped, Adapter->bSurpriseRemoved));

#if 1
	if(Adapter->bDriverStopped || Adapter->bSurpriseRemoved)
	{
		DEBUG_ERR(("\n\n\n*************sd_sync_rx : NIC will be unloaded!!!*************\n\n\n\n"));
		return _TRUE;
	}

#endif
	while(1)
	{
		
		_Rdy= SDIO_RdHRFF0(Adapter);

		if(!_Rdy)
		{

			//RT_TRACE(COMP_RECV, DBG_TRACE, ("\n\n\n******* HCI RX FF Not Ready *******\n\n\n\n"));
			
			if(polling_cnt == 0)
				break;


			polling_cnt++;
			
			if(polling_cnt < 8)
			{
				usleep_os(25);
				continue;

			}
			else
			{
				break;
			}
			
		}

		
		polling_cnt = 1;
		
	      	for (i = 0; i < Adapter->dvobjpriv.rxpktcount; i++)
		{
	       	if(recv_entry(pnetdev))		  	
		  		DEBUG_ERR(("\n\n^^^^recv_entry Fail ^^^^n\n"));
		}		

              //NdisMSleep(10000);			  
	}

	return _TRUE;

}

#else

u8 sd_sync_rx(PADAPTER	Adapter)
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
#endif


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
	DEBUG_INFO(("\n ==>sd_int_isr\n"));
	IsrContent=read32(padapter, HISR);
	//write32(padapter, HIMR, 0);
       //ioreq_read32(padapter, HISR, &IsrContent);
       //ioreq_write32(padapter, HIMR, 0);
       //ioreq_flush(padapter, (struct io_queue*)padapter->pio_queue);	   
	   
       padapter->IsrContent = IsrContent;
	  
_func_exit_;
	DEBUG_INFO(("\n sd_int_isr <==\n"));
	return ((padapter->IsrContent & padapter->ImrContent)!=0) ;


	// TODO: Perry: check this.
	if (IsrContent)
		return _TRUE;
	else
		return _FALSE;
	
}



//************************************
//Sdio Interrupt Deferred_Procedure_Call
//************************************
void sd_int_dpc( PADAPTER	padapter)
{
	
#ifndef CONFIG_POLLING_MODE
        _irqL 	irqL;	
	      unsigned long 	tmpL1=0, newtail, i;
	      struct 	xmit_priv *pxmitpriv= &(padapter->xmitpriv);
	      u32	txdonebits = _VOACINT | _VIACINT | _BEACINT| _BKACINT|_BMACINT;
#endif


	struct	evt_priv	*pevt_priv = &(padapter->evtpriv);
	struct	cmd_priv	*pcmd_priv = &(padapter->cmdpriv);
	
	uint tasks = (padapter->IsrContent & padapter->ImrContent);
	
_func_enter_;
	//DEBUG_INFO(("\n ==> sd_int_dpc \n"));
	if( (tasks & _C2HSET )) // C2HSET
	{
		DEBUG_ERR(("sd_int_dpc _C2HSET\n"));
		padapter->IsrContent  ^= _C2HSET;
		//_up_sema(&(pevt_priv->evt_notify));
		/* Perry:
			C2H interrupt tells driver that FW is staying at 40M.
		*/
		register_evt_alive(padapter);
		evt_notify_isr(pevt_priv);		
		
	}	
	
	if(tasks & _RXDONE){

		//DEBUG_ERR(("sd_int_dpc _RXDONE\n"));
		
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
		//DEBUG_ERR(("sd_int_dpc _CPWM\n"));
	
		padapter->IsrContent  ^= BIT(17);
		DEBUG_ERR(("SDIOINTDpc(): CPWM_INT\n"));
		//NdisSetEvent(&(adapter->CPWM_InterruptTest));
		//if(adapter->mppriv.cpwm_flag == 0)
		//          adapter->mppriv.cpwm_flag = 1;

#ifdef CONFIG_PWRCTRL
		// add cpwm handler here (Perry)
		fw_pwrstate_update_evt(padapter);
#endif
	}
	

	if( (tasks & _H2CCLR)) // H2CCLR
	{
		DEBUG_ERR(("sd_int_dpc _H2CCLR\n"));
	
		padapter->IsrContent  ^= _H2CCLR;
		cmd_clr_isr(pcmd_priv);
				
	}


	if(tasks & _RXERR){
		padapter->IsrContent  ^= _RXERR;
		//DEBUG_ERR(("sd_int_dpc _RXERR  "));
		
	}	

	
	if (tasks & _TXFFERR) {
/*		
		DEBUG_ERR((" sd_int_dpc _TXFFERR\n  "));
*/
		
		padapter->IsrContent ^= _TXFFERR;

	}


#ifndef CONFIG_POLLING_MODE

	if(tasks & txdonebits)
       {
	   	tmpL1 = read32(padapter, TXCMD_FWCLPTR);
		// DEBUG_ERR("TXCMD_FWCLPTR:%X\n", tmpL1);	
       }
	   
       if(tasks & _VOACINT ){
	   	
	   	padapter->IsrContent  ^=  _VOACINT ;
              
              			  
		newtail = tmpL1&0x7;
		i = pxmitpriv->vo_txqueue.tail;
		while( i != newtail)
		{
		     _enter_critical(&pxmitpriv->lock, &irqL);
                   pxmitpriv->vo_txqueue.free_sz += pxmitpriv->vo_txqueue.txsz[i];
                   _exit_critical(&pxmitpriv->lock, &irqL);
				   
                  if(i == (8-1))
                        i=0;
                   else				   
                       i++;
                  
		}
		              
		  pxmitpriv->vo_txqueue.tail = newtail;
			  
		
       }
	   
       if(tasks & _VIACINT ){
	   	
	   	padapter->IsrContent  ^= _VIACINT ;
            
              			  
		newtail = (tmpL1>>4)&0x7;
		i = pxmitpriv->vi_txqueue.tail;
		while( i != newtail)
		{
  		     _enter_critical(&pxmitpriv->lock, &irqL);
                   pxmitpriv->vi_txqueue.free_sz += pxmitpriv->vi_txqueue.txsz[i];
                   _exit_critical(&pxmitpriv->lock, &irqL);
				   
                  if(i == (8-1))
                        i=0;
                   else				   
                       i++;
                  
		}
		
              pxmitpriv->vi_txqueue.tail = newtail;
       }	   
	   
     
      if(tasks & _BEACINT ){
	   	
	   	padapter->IsrContent  ^=  _BEACINT ;
              
              			  
		newtail = (tmpL1>>8)&0x7;
		i = pxmitpriv->be_txqueue.tail;
		while( i != newtail)
		{  
		     _enter_critical(&pxmitpriv->lock, &irqL);                   		 
                   pxmitpriv->be_txqueue.free_sz += pxmitpriv->be_txqueue.txsz[i];                   				   
                   //DEBUG_ERR((" sd_int_dpc  Txdonebits:_BEACINT= free_sz:%d, txsz=%d, i=%d\n",pxmitpriv->be_txqueue.free_sz, pxmitpriv->be_txqueue.txsz[i], i));					   
                   _exit_critical(&pxmitpriv->lock, &irqL);

                   if(i == (8-1))
                        i=0;
                   else				   
                       i++;
                  
		}
		
              pxmitpriv->be_txqueue.tail = newtail;
       }	   
	
	 if(tasks & _BKACINT ){
	   	
	   	padapter->IsrContent  ^=  _BKACINT ;
              
              			  
		newtail = (tmpL1>>12)&0x7;
		i = pxmitpriv->bk_txqueue.tail;
		while( i != newtail)
		{
		     _enter_critical(&pxmitpriv->lock, &irqL);
                   pxmitpriv->bk_txqueue.free_sz += pxmitpriv->bk_txqueue.txsz[i];
                   _exit_critical(&pxmitpriv->lock, &irqL);

		    if(i == (8-1))
                        i=0;
                   else				   
                       i++;
                  
		}
		
              pxmitpriv->bk_txqueue.tail = newtail;
			  
		
       }	   

	if(tasks & _BMACINT ){
	   	
	   	padapter->IsrContent  ^=  _BMACINT ;
            
              			  
		newtail = ( tmpL1 >>24) & 0x3;
		i = pxmitpriv->bmc_txqueue.tail;
		while( i != newtail)
		{
		     _enter_critical(&pxmitpriv->lock, &irqL);
                   pxmitpriv->bmc_txqueue.free_sz += pxmitpriv->bmc_txqueue.txsz[i];
                   _exit_critical(&pxmitpriv->lock, &irqL);

				   
                 if(i == (4-1))
                        i=0;
                   else				   
                       i++;
                  	
		}
		
             pxmitpriv->bmc_txqueue.tail = newtail;
			  
		
       }	   
      
#if 0
       if(pxmitpriv->bk_txqueue.free_sz > 56 || 	pxmitpriv->bk_txqueue.free_sz <1 ||
	   pxmitpriv->be_txqueue.free_sz > 56 || 	pxmitpriv->be_txqueue.free_sz <1 ||
	   pxmitpriv->vi_txqueue.free_sz > 56 || 	pxmitpriv->vi_txqueue.free_sz <1 ||
	   pxmitpriv->vo_txqueue.free_sz > 56 || 	pxmitpriv->vo_txqueue.free_sz <1 )
       {

              DEBUG_ERR(("!!!bk, free_sz:%d\n", pxmitpriv->bk_txqueue.free_sz));	
		DEBUG_ERR(("!!!be, free_sz:%d\n", pxmitpriv->be_txqueue.free_sz));		
		DEBUG_ERR(("!!!vi, free_sz:%d\n", pxmitpriv->vi_txqueue.free_sz));		
		DEBUG_ERR(("!!!vo, free_sz:%d\n", pxmitpriv->vo_txqueue.free_sz));		

	  }
#endif

	if(tasks & txdonebits)
		_up_sema(&pxmitpriv->xmit_sema);	
		
#endif

_func_exit_;
	DEBUG_INFO(("\n  sd_int_dpc <==\n"));
	return;

}

void sd_sync_int_hdl(void *pcontext)
{
	
	_adapter *padapter = (_adapter *)pcontext;
	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;
	PSDDEVICE psddev = pdvobjpriv->psddev;

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

        if(sd_int_isr(padapter))
        {		
	      //write32(adapter, HIMR, 0);		
	      sd_int_dpc(padapter);        
		
	}
	else
	{
		DEBUG_ERR(("\n\n<=========== sd_sync_int_hdl(): not our INT\n\n"));	
	}



exit:	

_func_exit_;
	
	SDLIB_IssueConfig(psddev, SDCONFIG_FUNC_ACK_IRQ, NULL, 0);

	//write32(padapter, HIMR, padapter->ImrContent);

	return;
}


