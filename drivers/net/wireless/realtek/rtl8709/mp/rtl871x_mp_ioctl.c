/******************************************************************************
* rtl871x_mp_ioctl.c                                                                                                                                 *
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
#define _RTL871X_MP_IOCTL_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


//#include <stdlib.h>
//#include <stdio.h>

#include <rtl8711_regdef.h>
#include <rtl871x_mp_ioctl.h>
#include <mp_custom_oid.h>

#ifdef CONFIG_MP_INCLUDED
static	u8	mpdatarate[NumRates]={11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0xff};

	
NDIS_STATUS 
mp_query_info(
	PADAPTER		Adapter,
	NDIS_OID		Oid,
	u8			*InformationBuffer,
	u32			InformationBufferLength,
	u32 *			BytesWritten,
	u32 *			BytesNeeded
)
{
       NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	_irqL				oldirql;
	LIST_ENTRY		*pListEntry = NULL;
	pRW_Reg 			RegRWStruct;
	pEEPROM_RWParam 	EEPROMStruct;
	pBurst_RW_Reg		pBstRwReg;
//	pRW_ACCESS_PROTOCOL	pRWProtocol;
//	pSCSI_DATA			pSCSIdata;
	UCHAR	cnt,tmpLen;
	PVOID	pBuffer[10];
	pUSB_VendorReq		pUsbVdrReq;
	BOOLEAN				bRet;       
	u32 info, info_len;	
	
#if 1
	oldirql  = KeGetCurrentIrql();
	if( oldirql > PASSIVE_LEVEL) 
	{
	    KeLowerIrql(PASSIVE_LEVEL);
	}
#endif
  switch(Oid)
  {

	case OID_RT_PRO_READ_REGISTER:	
		DEBUG_ERR(("\n ===> Query OID_RT_PRO_READ_REGISTER.\n"));
		{
			RegRWStruct=(pRW_Reg)InformationBuffer;
			
			switch(RegRWStruct->width)
                     {
                          case 1:
					    RegRWStruct->value=read8(Adapter, RegRWStruct->offset);	  	
			 
					    break;						
			     case 2:
				  	    RegRWStruct->value=read16(Adapter, RegRWStruct->offset);
				 
				  	    break;												
			     case 4:
				  	     RegRWStruct->value=read32(Adapter, RegRWStruct->offset);	  	
				 
				  	    break;
			     default:
                                      Status = NDIS_STATUS_FAILURE;
					   break;
			}                      	
							
			
                     if( Status == NDIS_STATUS_FAILURE) Status = NDIS_STATUS_NOT_ACCEPTED;
			
		}		
		DEBUG_INFO(("\n <=== Query OID_RT_PRO_READ_REGISTER.\n"));
		break;
		
	case OID_RT_PRO_BURST_READ_REGISTER:
	DEBUG_ERR(("\n ===> Query OID_RT_PRO_BURST_READ_REGISTER.\n"));
		pBstRwReg = (pBurst_RW_Reg)InformationBuffer;
		read_mem(Adapter, pBstRwReg->offset, (ULONG)pBstRwReg->len, pBstRwReg->Data);
	DEBUG_INFO(("\n <=== Query OID_RT_PRO_BURST_READ_REGISTER.\n"));
		break;
	case OID_RT_PRO_READ16_EEPROM:
		DEBUG_ERR(("\n ===> Query OID_RT_PRO_READ16_EEPROM.\n"));
		{
			EEPROMStruct=(pEEPROM_RWParam)InformationBuffer;
			if((Adapter->EepromAddressSize==6)&&(EEPROMStruct->offset >128)){
				DEBUG_ERR(("\n Adapter->EepromAddressSize==6 EEPROMStruct->offset =%d(offset must not larger than 128)\n",EEPROMStruct->offset ));
			}
			else{
			EEPROMStruct->value = eeprom_read16(Adapter, (USHORT)(EEPROMStruct->offset >>1));
			DEBUG_ERR(("\n offset=%d EEPROMStruct->value=0x%4x\n",EEPROMStruct->offset ,EEPROMStruct->value));
			}
		}
		DEBUG_INFO(("\n <=== Query OID_RT_PRO_READ16_EEPROM.\n"));
		break;	
       case OID_RT_PRO_QUERY_TX_PACKET_SENT:
		if(InformationBufferLength == sizeof(ULONG))
		 {
			*(ULONG*)InformationBuffer = Adapter->mppriv.tx_pktcount;
		 }			 
		 else
		 {
			Status = NDIS_STATUS_NOT_ACCEPTED;
		 }			
		break;
	case OID_RT_PRO_QUERY_RX_PACKET_RECEIVED:
		if(InformationBufferLength == sizeof(ULONG))
		 {
			 *(ULONG*)InformationBuffer = Adapter->mppriv.rx_pktcount;
		 }			 
		 else
		 {
			 Status = NDIS_STATUS_NOT_ACCEPTED;
		 }			
		break;
	case OID_RT_PRO_QUERY_RX_PACKET_CRC32_ERROR:		
		if(InformationBufferLength == sizeof(ULONG))
		{
			*(ULONG*)InformationBuffer = Adapter->mppriv.rx_crcerrpktcount;
		 }			 
		 else
		 {
			Status = NDIS_STATUS_NOT_ACCEPTED;
		 }			
		break;					
	case OID_RT_PRO_H2C_QUERY_RESULT://

       case OID_RT_PRO_H2C_CMD_RSP_MODE://

	case OID_RT_QRY_POLL_WKITEM://

	case OID_RT_PRO_RW_ACCESS_PROTOCOL_TEST://

	case OID_RT_PRO_SCSI_TCPIPOFFLOAD_IN://		

	case OID_RT_PRO_USB_VENDOR_REQ://

        case OID_RT_PRO_USB_MAC_RX_FIFO_POLLING://

	case OID_RT_PRO_USB_MAC_RX_FIFO_READ://

       case OID_RT_RRO_RX_PKT_VIA_IOCTRL:
	   	break;


	case OID_RT_MH_VENDER_ID:
		DEBUG_ERR(("\n ===> Query OID_RT_MH_VENDER_ID.\n"));

		info_len = 4;
		info = REALTEK_VENDER_ID;

		if(info_len <= InformationBufferLength)
		{
			*BytesWritten = info_len;
			_memcpy(InformationBuffer, (u8*)info, info_len);	
			DEBUG_ERR(("\n ===> MH InformationBuffer = 0x%x.\n", *InformationBuffer));
			
		}
		else
		{
			*BytesNeeded = info_len;
			Status = NDIS_STATUS_BUFFER_TOO_SHORT;
		}
		
		
		
		break;


       case OID_RT_PRO8711_PKT_LOSS:
	   	if(InformationBufferLength == sizeof(unsigned int)*2)
	   	{
	   	  if(*(unsigned int*)InformationBuffer == 1)//init==1
	   	        Adapter->mppriv.rx_pktloss=0;
		
                *((unsigned int*)InformationBuffer+1) = Adapter->mppriv.rx_pktloss;
	   	}
		else
		  Status = NDIS_STATUS_INVALID_LENGTH; 	
	   	break;
	case OID_RT_PRO8711_WI_POLL:
	 {
	 	struct mp_wiparam *pwi_param=(struct mp_wiparam *)InformationBuffer;
	   	if(InformationBufferLength == sizeof(struct mp_wiparam ))
	   	{
	   	    if(Adapter->mppriv.workparam.bcompleted == _TRUE)
	   	    {		
	   	    	_memcpy(pwi_param, &Adapter->mppriv.workparam, sizeof(struct mp_wiparam));
                      Adapter->mppriv.act_in_progress = _FALSE;		   
			 //RT_TRACE(COMP_OID_SET, DBG_LOUD, ("rf:%x\n", pwiparam->IoValue));				  
					  
	   	    }
		    else
		    {		        
                       Status = NDIS_STATUS_NOT_ACCEPTED;				
		    }
	   	}
		else
		{
		    Status = NDIS_STATUS_INVALID_LENGTH;  
		}
		break;
	   }

	case OID_RT_RPO_ASYNC_RWIO_POLL:	
		break;
       case OID_RT_RD_ATTRIB_MEM:	   
	   	#ifdef CONFIG_SDIO_HCI
	   	 DEBUG_ERR(("===> Query OID_RT_RD_ATTRIB_MEM.\n"));		   	
		{		  
		  unsigned long *plmem = (unsigned long*)InformationBuffer+2;
		  attrib_read(Adapter, *((unsigned long*)InformationBuffer), *((unsigned long*)InformationBuffer+1), (unsigned char*)plmem);
              }
		#endif
	   	break;
	case OID_RT_POLL_RX_STATUS:	
		_memcpy(InformationBuffer, (unsigned char*)&Adapter->mppriv.rxstat, sizeof(struct recv_stat));
		break;

#ifdef CONFIG_PWRCTRL
	case OID_RT_PRO_QRY_PWRSTATE:
		if(InformationBufferLength >= 8) 		
		{
		   *BytesWritten = 8;
		   _memcpy(InformationBuffer, &(Adapter->pwrctrlpriv.pwr_mode), 8);		
		}
		else
		   Status = NDIS_STATUS_NOT_ACCEPTED;

		DEBUG_INFO( ("Query Information, OID_RT_PRO_QRY_PWRSTATE:%d smart_ps:%d\n", Adapter->pwrctrlpriv.pwr_mode, Adapter->pwrctrlpriv.smart_ps));
		break;	

		
#endif


	default:
		DEBUG_ERR(("\n ===> this oid querying is not suppoered now.\n"));
		break;
			
   }
#if 1
       if(KeGetCurrentIrql() == PASSIVE_LEVEL)
       {
           KeRaiseIrql(DISPATCH_LEVEL, &oldirql);
       }
#endif
	return Status;

}

//This function initializes the DUT to the MP test mode
int mp_start_test(_adapter *padapter)
{
     	_irqL irqL;
	int	res;
	struct sta_info *psta, *psta_old;
	unsigned char dst_macaddr[6];
	unsigned long length;
	unsigned int tmpvalue;
	
	struct wlan_network *pnetwork=NULL;
	struct mp_priv *pmppriv = &(padapter->mppriv);
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);	
	struct wlan_network *tgt_network = &(pmlmepriv->cur_network);	
	

       //note: maybe need to _enter_critical when change fw_state
	   
       //init mp_start_test status
       pmppriv->prev_fw_state = pmlmepriv->fw_state;
       pmlmepriv->fw_state = WIFI_MP_STATE;

	if(pmppriv->mode == 1)
	  set_fwstate(pmlmepriv, WIFI_MP_LPBK_STATE);//for sd4 Loopback testing   
	
	set_fwstate(pmlmepriv, _FW_UNDER_LINKING);
	
       //clear psta in the cur_network, if any
       psta_old = get_stainfo(&padapter->stapriv, tgt_network->network.MacAddress);
       if (psta_old)
	   	free_stainfo(padapter,  psta_old);
      

	//create new  a wlan_network for mp driver and replace the cur_network;
       pnetwork= (struct wlan_network *)_malloc(sizeof(struct wlan_network));       
	memset((unsigned char *)pnetwork, 0, sizeof (struct wlan_network));
       pnetwork->join_res = 1;//
       _memcpy(&(pnetwork->network.MacAddress), pmppriv->network_macaddr, ETH_ALEN);
		   
	pnetwork->network.InfrastructureMode = Ndis802_11IBSS;
	pnetwork->network.NetworkTypeInUse = Ndis802_11DS;
	
       pnetwork->network.IELength = 0;
	   
	pnetwork->network.Ssid.SsidLength = 10;
	_memcpy(pnetwork->network.Ssid.Ssid , (unsigned char*)"mp_george", pnetwork->network.Ssid.SsidLength);
		

       length = get_NDIS_WLAN_BSSID_EX_sz(&pnetwork->network);
       pnetwork->network.Length = (length /4 + ((length %4 != 0) ? 1 : 0))*4;//round up to multiple of 4 bytes.

	
       //create a new psta for mp driver in the new created wlan_network
	psta = alloc_stainfo(&padapter->stapriv, pnetwork->network.MacAddress);
	if (psta == NULL) {
		//DEBUG_ERR("Can't alloc sta_info\n");
		return _FAIL;
	}

	_enter_critical(&pmlmepriv->lock, &irqL);

	tgt_network->join_res = pnetwork->join_res;
	
	if (pnetwork->join_res > 0) {

		if ((pmlmepriv->fw_state) & _FW_UNDER_LINKING) {

			psta_old = get_stainfo(&padapter->stapriv, tgt_network->network.MacAddress);

			if (psta_old)
				free_stainfo(padapter, psta_old);
			
			 _memcpy(&tgt_network->network, &pnetwork->network,
				(get_NDIS_WLAN_BSSID_EX_sz(&pnetwork->network)));

			tgt_network->aid = psta->aid  = pnetwork->join_res;

			(pmlmepriv->fw_state) ^= _FW_UNDER_LINKING;
		}
		else {

			//DEBUG_ERR("err: fw_state:%x",pmlmepriv->fw_state);
			free_stainfo(padapter, psta);
			
			res = _FAIL;
			goto end_of_mp_start_test;
		}



#ifdef __WINDOWS__			

		NdisMIndicateStatus(padapter->NdisCEDev.hNdisAdapter, NDIS_STATUS_MEDIA_CONNECT, NULL, 0);
		NdisMIndicateStatusComplete(padapter->NdisCEDev.hNdisAdapter);
#endif



              
			  
              //Set to LINKED STATE for MP TRX Testing
		pmlmepriv->fw_state |= _FW_LINKED;		

              //NDIS_802_11_NETWORK_INFRASTRUCTURE networktype;
              if(padapter->eeprompriv.bautoload_fail_flag==_FALSE)
              res = setopmode_cmd(padapter, 5);//?
              	res=_SUCCESS;

	}
	else {

		free_stainfo(padapter, psta);
              res = _FAIL;

	}	
				
end_of_mp_start_test:				

	_exit_critical(&pmlmepriv->lock, &irqL);

	_mfree((unsigned char*)pnetwork, sizeof(struct wlan_network));

	return res;

}

//This function change the DUT from the MP test mode into normal mode
int mp_stop_test(_adapter *padapter)
{
  	struct sta_info *psta;
	unsigned char dst_macaddr[6];
	
	struct mp_priv *pmppriv = &(padapter->mppriv);
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);	
	struct wlan_network *tgt_network = &(pmlmepriv->cur_network);	
	

       //return to normal state (default:station mode)
       pmlmepriv->fw_state = WIFI_STATION_STATE;

       //Todo:check if  tgt_network->network.MacAddress == mp_priv->network_macaddr       
	
	//clear psta used in mp test mode.
       psta = get_stainfo(&padapter->stapriv, tgt_network->network.MacAddress);
       if (psta)
	   	//free_stainfo((unsigned char *)padapter, &padapter->stapriv.free_sta_queue, psta);
	   	free_stainfo(padapter, psta);

	//flush the cur_network   
	memset((unsigned char *)tgt_network, 0, sizeof (struct wlan_network));	
	
	return _SUCCESS;

}
int mp_start_joinbss(_adapter *padapter, NDIS_802_11_SSID *pssid)
{
     unsigned char res = _SUCCESS;
     struct mp_priv *pmppriv = &(padapter->mppriv);
     struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);	
     struct	wlan_network	*pnetwork = (struct	wlan_network	*)&pmppriv->mp_network;
	 
     if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE) && (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) )    
     	   pmlmepriv->fw_state ^= _FW_LINKED;         
     else
    	   return _FAIL;

     res = setassocsta_cmd(padapter, pmppriv->network_macaddr);
     pmlmepriv->fw_state |= _FW_UNDER_LINKING;

     
#if 0
     unsigned char *dst_ssid, *src_ssid;
     unsigned char res = _SUCCESS;
     struct mp_priv *pmppriv = &(padapter->mppriv);
     struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);	
     struct	wlan_network	*pnetwork = &mp_priv->mp_network;
	 
     if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE) && (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) )    
     	   pmlmepriv->fw_state ^= _FW_LINKED;         
     else
    	   return _FAIL;

     _memcpy(&padapter->MgntInfo.ssid, pssid, sizeof(NDIS_802_11_SSID));     	
     _memcpy(&pmlmepriv->assoc_ssid, pssid, sizeof(NDIS_802_11_SSID));	 
	  	
    pmlmepriv->cur_network.join_res = -2;
    pmlmepriv->fw_state |= _FW_UNDER_LINKING;

    dst_ssid = pnetwork->network.Ssid.Ssid;
    src_ssid = pmlmepriv->assoc_ssid.Ssid;
    if ((_memcmp(dst_ssid, src_ssid, pmlmepriv->assoc_ssid.SsidLength)) == _FALSE)
 	  return _FAIL;
    
     res =  joinbss_cmd((unsigned char  *)padapter, pnetwork);	 	

     NdisMSetTimer(&pmlmepriv->assoc_timer, MAX_JOIN_TIMEOUT );
	 
#endif
     

     return res;
	 
}


NDIS_STATUS 
mp_set_info(
	PADAPTER		Adapter,
	NDIS_OID		Oid,
	UCHAR			*InformationBuffer,
	ULONG			InformationBufferLength,
	PULONG			BytesRead,
	PULONG			BytesNeeded
)
{
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
//	PHAL_DATA_8711		pHalData =GetHalData8711(Adapter);
//	PMGNT_INFO      		pMgntInfo;
	_irqL				oldirql;
	
	pRW_Reg 			RegRWStruct;
	pEEPROM_RWParam 	EEPROMStruct;
	pBurst_RW_Reg		pBstRwReg;
	TX_CMD_Desc		*TxCmd_Info;
//	pH2C_cmd_Tuple 		pH2CCmd;
//	ULONG				H2C_C2H_TestMode;	
//	pUSB_VendorReq		pUsbVdrReq;

	LIST_ENTRY			*pListEntry = NULL;
	BOOLEAN				bRet;
	unsigned char 		*pScsiOutData;
	unsigned long			ScsiOutDataLen;   
	unsigned char			u8Tmp;
       unsigned short		u16Tmp;
	unsigned int			u32Tmp;   
	


#if 1
	oldirql  = KeGetCurrentIrql();
	if( oldirql > PASSIVE_LEVEL) {
	    KeLowerIrql(PASSIVE_LEVEL);
	}
#endif
	switch(Oid) {
		case OID_RT_PRO_WRITE_REGISTER:	
			DEBUG_ERR(("\n ===> Set OID_RT_PRO_WRITE_REGISTER.\n"));
			{
				RegRWStruct=(pRW_Reg)InformationBuffer;
                           switch(RegRWStruct->width)
                           {
                              case 1:
					    write8(Adapter, RegRWStruct->offset, (unsigned char)RegRWStruct->value);	  	
					    break;						
				  case 2:
				  	    write16(Adapter, RegRWStruct->offset, (unsigned short)RegRWStruct->value);	
				  	    break;												
				  case 4:
				  	    write32(Adapter, RegRWStruct->offset, (unsigned int)RegRWStruct->value);	  	
				  	    break;
				   default:
                                      Status = NDIS_STATUS_FAILURE;
					   break;
                           	}                       	
								

				//Henry: for maintain the correct HIMR status
				if(Status == NDIS_STATUS_SUCCESS && RegRWStruct->offset == HIMR && RegRWStruct->width == 4)
				{
					Adapter->ImrContent=RegRWStruct->value;
				}
				
				if( Status == NDIS_STATUS_FAILURE) Status = NDIS_STATUS_NOT_ACCEPTED;
			}
			DEBUG_ERR(("\n <=== Set OID_RT_PRO_WRITE_REGISTER.\n"));
			break;
			
		case OID_RT_PRO_BURST_WRITE_REGISTER:
			DEBUG_ERR(("\n ===> Set OID_RT_PRO_BURST_WRITE_REGISTER.\n"));
			pBstRwReg = (pBurst_RW_Reg)InformationBuffer;	
			write_mem(Adapter, pBstRwReg->offset, (ULONG)pBstRwReg->len, pBstRwReg->Data);			
			DEBUG_ERR(("\n <=== Set OID_RT_PRO_BURST_WRITE_REGISTER.\n"));
			break;
			
		case OID_RT_PRO_WRITE_TXCMD:
			DEBUG_ERR(("===> Set OID_RT_PRO_WRITE_TXCMD.\n"));
			TxCmd_Info=(TX_CMD_Desc*)InformationBuffer;

                     DEBUG_ERR(("WRITE_TXCMD:Addr=%.8X\n", TxCmd_Info->offset));
  		       DEBUG_ERR(("WRITE_TXCMD:1.)%.8X\n", (ULONG)TxCmd_Info->TxCMD.value[0]));
 		       DEBUG_ERR(("WRITE_TXCMD:2.)%.8X\n", (ULONG)TxCmd_Info->TxCMD.value[1]));
			DEBUG_ERR(("WRITE_TXCMD:3.)%.8X\n", (ULONG)TxCmd_Info->TxCMD.value[2]));   
			DEBUG_ERR(("WRITE_TXCMD:4.)%.8X\n", (ULONG)TxCmd_Info->TxCMD.value[3]));
 					 
			write32(Adapter, TxCmd_Info->offset + 0, (unsigned int)TxCmd_Info->TxCMD.value[0]);	  	
			write32(Adapter, TxCmd_Info->offset + 4, (unsigned int)TxCmd_Info->TxCMD.value[1]);	  	

			DEBUG_ERR(("<=== Set OID_RT_PRO_WRITE_TXCMD.\n"));
			
			break;
		case OID_RT_PRO_WRITE16_EEPROM:
			DEBUG_ERR(("===> Set OID_RT_PRO_WRITE16_EEPROM.\n"));
			EEPROMStruct=(pEEPROM_RWParam)InformationBuffer; 
			DEBUG_ERR(("\n Adapter->EepromAddressSize=%d\n",Adapter->EepromAddressSize));
			if((Adapter->registrypriv.hci==RTL8711_USB)&&(EEPROMStruct->offset>128)){
				DEBUG_ERR(("\n Adapter->EepromAddressSize=%d EEPROMStruct->offset=%d (when Adapter->EepromAddressSize=6, EEPROMStruct->offset must less than 128)\n",
					Adapter->EepromAddressSize,EEPROMStruct->offset));
			}
				
			eeprom_write16(Adapter, (USHORT)(EEPROMStruct->offset >>1), EEPROMStruct->value);
			DEBUG_ERR(("\n offset=%d value=0x%4x\n",EEPROMStruct->offset ,EEPROMStruct->value));

			break;

		case OID_RT_PRO_H2C_SET_COMMAND:
		case OID_RT_PRO_H2C_C2H_LBK_TEST:
		case OID_RT_PRO_H2C_CMD_MODE:
		case OID_RT_SET_READ_REG:
		case OID_RT_SET_WRITE_REG:
		case OID_RT_SET_READ16_EEPROM:
		case OID_RT_SET_WRITE16_EEPROM:
		case OID_RT_PRO_SCSI_ACCESS_TEST:  
		case OID_RT_PRO_USB_VENDOR_REQ:
		case OID_RT_PRO_SCSI_AUTO_TEST:
		case OID_RT_PRO_SCSI_TCPIPOFFLOAD_OUT:
		case OID_RT_PRO_SCSI_TCPIPOFFLOAD_IN:		
		case OID_RT_PRO_USB_MAC_AC_FIFO_WRITE:
			break;
              case OID_RT_PRO_START_TEST:
			DEBUG_ERR(("\n ===> Set OID_RT_PRO_START_TEST.\n"));
			{
                     	uint mode = *((UINT*)InformationBuffer);				  	
				Adapter->mppriv.mode = mode;//0 for sd3, 1 for sd4
				if(  mp_start_test(Adapter) == _FAIL ) 
					Status = NDIS_STATUS_NOT_ACCEPTED;  
				else 
					Status = NDIS_STATUS_SUCCESS;
              	}
			DEBUG_ERR(("\n <=== Set OID_RT_PRO_START_TEST.\n"));
                     break;					 
		case OID_RT_PRO_STOP_TEST:	
			DEBUG_ERR(("\n ===> Set OID_RT_PRO_STOP_TEST.\n"));
			if(  mp_stop_test(Adapter) == _FAIL ) 
				Status = NDIS_STATUS_NOT_ACCEPTED;  
			else 
				Status = NDIS_STATUS_SUCCESS;
			DEBUG_ERR(("\n <=== Set OID_RT_PRO_STOP_TEST.\n"));
			break;			
		case OID_RT_PRO8711_JOIN_BSS:
			if( (InformationBuffer == 0) || (InformationBufferLength == 0) ) {
				
				return NDIS_STATUS_INVALID_LENGTH;

			} else {	

				NDIS_802_11_SSID    	*ssid;								
				if (InformationBufferLength < sizeof(NDIS_802_11_SSID)){
					*BytesNeeded = sizeof(NDIS_802_11_SSID);
					*BytesRead = 0;
					return NDIS_STATUS_INVALID_LENGTH;
				}

				ssid = (NDIS_802_11_SSID *)InformationBuffer;
			
			       if( mp_start_joinbss(Adapter, ssid) == _FAIL )	Status = NDIS_STATUS_NOT_ACCEPTED;  
			       else Status = NDIS_STATUS_SUCCESS;

                            *BytesRead = sizeof(NDIS_802_11_SSID);					

			}
			break;
		case OID_RT_PRO_SET_CHANNEL_DIRECT_CALL:
			DEBUG_ERR(("OID_RT_PRO_SET_CHANNEL_DIRECT_CALL\n"));
			{
			    UINT Channel = *((UINT*)InformationBuffer);				
			    Adapter->mppriv.curr_ch = Channel;
			    if(setphy_cmd(Adapter, (unsigned char)Adapter->mppriv.curr_modem, (unsigned char)Channel) == _FAIL)
				 Status = NDIS_STATUS_NOT_ACCEPTED;  
			}		

			break;
              case OID_RT_PRO_SET_DATA_RATE_EX:
			 DEBUG_ERR(("===> OID_RT_PRO_SET_DATA_RATE_EX.\n"));				
			{
			   if( setdatarate_cmd(Adapter, InformationBuffer) !=_SUCCESS)
				   Status = NDIS_STATUS_NOT_ACCEPTED;	  
              	}
			  	
			 break;
              case OID_RT_PRO_SET_DATA_RATE:
			{				
		 	 
                       int i;	         
                       unsigned char datarates[NumRates];		         		         					   
			  unsigned long ratevalue = *((ULONG*)InformationBuffer);//4			  

                       DEBUG_ERR(("===> OID_RT_PRO_SET_DATA_RATE.\n"));				
			  
			  for(i=0; i<NumRates; i++)
			  {                          
			     if(ratevalue==mpdatarate[i])						  	
                          {
                             datarates[i] = mpdatarate[i];				 		 
                          }else{
                             datarates[i] = 0xff;                         
  			     }
 
                          DEBUG_ERR(("datarate_inx=%d\n",datarates[i]));						  
			  }
			  
                       if( setdatarate_cmd(Adapter, datarates) !=_SUCCESS)
				Status = NDIS_STATUS_NOT_ACCEPTED;	  

                       DEBUG_ERR(("<=== OID_RT_PRO_SET_DATA_RATE.\n"));					   
			  
			}		
	   	
	       break;	
               case OID_RT_PRO_SET_BASIC_RATE:
	   	 {				
		 	 
                       int i;	         
                       unsigned char datarates[NumRates];		         		         					   
			  unsigned long ratevalue = *((ULONG*)InformationBuffer);//4			  

                      DEBUG_ERR(("===> OID_RT_PRO_SET_BASIC_RATE.\n"));				
			  
			  for(i=0; i<NumRates; i++)
			  {                          
			     if(ratevalue==mpdatarate[i])						  	
                          {
                             datarates[i] = mpdatarate[i];				 		 
                          }else{
                             datarates[i] = 0xff;                         
  			     }
 
                          DEBUG_ERR(("basicrate_inx=%d\n",datarates[i]));						  
			  }
			  
                       if( setbasicrate_cmd(Adapter, datarates) !=_SUCCESS)
				Status = NDIS_STATUS_NOT_ACCEPTED;	  

                       DEBUG_ERR(("<=== OID_RT_PRO_SET_BASIC_RATE.\n"));					   
			  
		 }			   	
	        break;		  		   
	        case OID_RT_PRO_SET_MODULATION:
			  DEBUG_ERR(("OID_RT_PRO_SET_MODULATION\n"));
			  Adapter->mppriv.curr_modem = *((UINT*)InformationBuffer);
			break;
               case OID_RT_PRO_WRITE_BB_REG:
			 if(InformationBufferLength == (sizeof(unsigned long)*3))
                     {      
                         //RegOffsetValue	- The offset of BB register to write.
                         //bOFDM			- TRUE if the regsiter to access is for OFDM.
                         //RegDataValue	- The value to write. 
                          //RegOffsetValue = *((unsigned long*)InformationBuffer);
                          //bOFDM = *((unsigned long*)InformationBuffer+1);	   
	                   //RegDataValue =  *((unsigned long*)InformationBuffer+2);	
	                   if(!setbbreg_cmd(Adapter, 
					   	              *(unsigned char*)InformationBuffer, 
					   	              (unsigned char)(*((unsigned long*)InformationBuffer+2))))
			     {
				Status = NDIS_STATUS_NOT_ACCEPTED;
			      }
	                   
                      }else
                      {
                           Status = NDIS_STATUS_NOT_ACCEPTED;
			 }   	
		  	
			break;
		case OID_RT_PRO_RF_WRITE_REGISTRY:	
			if(InformationBufferLength == (sizeof(unsigned long)*3))
                     {      
                           //RegOffsetValue	- The offset of RF register to write.
	                    //RegDataWidth	- The data width of RF register to write.
	                    //RegDataValue	- The value to write. 
                           //RegOffsetValue = *((unsigned long*)InformationBuffer);
                           //RegDataWidth = *((unsigned long*)InformationBuffer+1);	   
                           //RegDataValue =  *((unsigned long*)InformationBuffer+2);	
	                   if(!setrfreg_cmd(Adapter, 
					   	              *(unsigned char*)InformationBuffer, 
					   	              (unsigned long)(*((unsigned long*)InformationBuffer+2))))
			     {
				Status = NDIS_STATUS_NOT_ACCEPTED;
			      }
	                   
                      }else
                      {
                           Status = NDIS_STATUS_NOT_ACCEPTED;
			 }   	
			break;	
              case OID_RT_PRO_READ_BB_REG:			  	
			if(InformationBufferLength == (sizeof(unsigned long)*3))
	              {
	                     if(Adapter->mppriv.act_in_progress == _TRUE)
	                     {
	                         Status = NDIS_STATUS_NOT_ACCEPTED;
	                     }
	                     else
	                     {
				       //init workparam
				       Adapter->mppriv.act_in_progress = _TRUE;
				       Adapter->mppriv.workparam.bcompleted = _FALSE;
	                            Adapter->mppriv.workparam.act_type= MPT_READ_BB_OFDM;
	                            Adapter->mppriv.workparam.io_offset= *(unsigned long*)InformationBuffer;		
					Adapter->mppriv.workparam.io_value= 0xcccccccc;
				       
					//RegOffsetValue	- The offset of BB register to read.
	                           //bOFDM			- TRUE if the regsiter to access is for OFDM.
	                           //RegDataValue	- The value to read. 
	                           //RegOffsetValue = *((unsigned long*)InformationBuffer);
	                           //bOFDM = *((unsigned long*)InformationBuffer+1);	   
		                    //RegDataValue =  *((unsigned long*)InformationBuffer+2);	   	 	                   
		                   if(!getbbreg_cmd(Adapter, 
						   	              *(unsigned char*)InformationBuffer, 
						   	               (unsigned char*)&Adapter->mppriv.workparam.io_value))
				     {
					   Status = NDIS_STATUS_NOT_ACCEPTED;
				      }
	                     }			      		   
		                   
	               }else
	               {
	                           Status = NDIS_STATUS_NOT_ACCEPTED;
			  }   			  	
		break;	
	       case OID_RT_PRO_RF_READ_REGISTRY:	
			if(InformationBufferLength == (sizeof(unsigned long)*3))
	              {
	                     if(Adapter->mppriv.act_in_progress == _TRUE)
	                     {
	                         Status = NDIS_STATUS_NOT_ACCEPTED;
	                     }
	                     else
	                     {
				       //init workparam
				       Adapter->mppriv.act_in_progress = _TRUE;
				       Adapter->mppriv.workparam.bcompleted= _FALSE;
	                            Adapter->mppriv.workparam.act_type = MPT_READ_RF;
	                            Adapter->mppriv.workparam.io_offset = *(unsigned long*)InformationBuffer;		
					Adapter->mppriv.workparam.io_value = 0xcccccccc;
				       
					//RegOffsetValue	- The offset of RF register to read.
		                     //RegDataWidth	- The data width of RF register to read.
		                     //RegDataValue	- The value to read. 
	                            //RegOffsetValue = *((unsigned long*)InformationBuffer);
	                            //RegDataWidth = *((unsigned long*)InformationBuffer+1);	   
	                            //RegDataValue =  *((unsigned long*)InformationBuffer+2);	   	 	                   
		                   if(!getrfreg_cmd(Adapter, 
						   	              *(unsigned char*)InformationBuffer, 
						   	               (unsigned char*)&Adapter->mppriv.workparam.io_value))
				     {
					   Status = NDIS_STATUS_NOT_ACCEPTED;
				      }
	                     }
				      		   
		                   
	               }else
	               {
	                           Status = NDIS_STATUS_NOT_ACCEPTED;
			  }   			  	
			break;	
              case OID_RT_PRO_RESET_TX_PACKET_SENT:
			  if(InformationBufferLength == sizeof(ULONG))
			 {
			    Adapter->mppriv.tx_pktcount = 0;
			 }			 
			 else
			 {
			    Status = NDIS_STATUS_NOT_ACCEPTED;
			 }			
			 break;
		case OID_RT_PRO_RESET_RX_PACKET_RECEIVED:
			if(InformationBufferLength == sizeof(ULONG))
			 {
			    Adapter->mppriv.rx_pktcount = 0;
			    Adapter->mppriv.rx_crcerrpktcount = 0;				
			 }			 
			 else
			 {
			    Status = NDIS_STATUS_NOT_ACCEPTED;
			 }			
			
			 break;		
              case OID_RT_WR_ATTRIB_MEM:
		#ifdef CONFIG_SDIO_HCI
			 {				
			  unsigned long *plmem = (unsigned long*)InformationBuffer+2;
			  attrib_write(Adapter, *(unsigned long*)InformationBuffer, *((unsigned long*)InformationBuffer+1), (unsigned char*)plmem);
              	}
		#endif
                     break;	
		case OID_RT_RPO_ASYNC_RWIO_TEST:
			break;
              case OID_RT_PRO_SET_RF_INTFS:	
			  DEBUG_ERR(("OID_RT_PRO_SET_RF_INTFS\n"));
			{			    
			    if(setrfintfs_cmd(Adapter, *(unsigned char*)InformationBuffer) == _FAIL)
				 Status = NDIS_STATUS_NOT_ACCEPTED;  
			}	
			break;
#ifdef CONFIG_PWRCTRL

		case OID_RT_PRO_SET_PWRSTATE:
			DEBUG_ERR(("***Set OID_RT_PRO_SET_PWRSTATE ***\n"));
			if( (InformationBuffer == 0) || (InformationBufferLength < 8) ) {
				*BytesRead = 0;
				*BytesNeeded = 8;
				return NDIS_STATUS_INVALID_LENGTH;
			} else {
				unsigned int pwr_mode = *(unsigned int *)(InformationBuffer);
				unsigned int smart_ps = *(unsigned int *)((int)InformationBuffer + 4);
				if(pwr_mode != Adapter->pwrctrlpriv.pwr_mode || smart_ps != Adapter->pwrctrlpriv.smart_ps) {
					ps_mode_init(Adapter, pwr_mode, smart_ps);
				}
			}
			*BytesRead = 8;
				
			return NDIS_STATUS_SUCCESS;
			
#endif

		case OID_RT_PRO_H2C_SET_RATE_TABLE:
		{
			struct setratable_parm*prate_table=(struct setratable_parm*)InformationBuffer;
			u8 res;
			if(InformationBufferLength < sizeof(struct setratable_parm)) {
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(struct setratable_parm);
				break;
			}
			res=setrttbl_cmd(Adapter, prate_table);
			if(res=_FAIL){
				Status= NDIS_STATUS_FAILURE;
			}	
			break;
		}
		case OID_RT_PRO_H2C_GET_RATE_TABLE:
		{
	#if 0		
			struct mp_wi_cntx *pmp_wi_cntx=&(Adapter->mppriv.wi_cntx);
			u8 res=_SUCCESS;
			DEBUG_INFO(("===> Set OID_RT_PRO_H2C_GET_RATE_TABLE.\n"));
			
			if(pmp_wi_cntx->bmp_wi_progress ==_TRUE){
				DEBUG_ERR(("\n mp workitem is progressing, not allow to set another workitem right now!!!\n"));				
				Status = NDIS_STATUS_NOT_ACCEPTED;
				break;
			}
			else{
				pmp_wi_cntx->bmp_wi_progress=_TRUE;
				pmp_wi_cntx->param.bcompleted=_FALSE;
				pmp_wi_cntx->param.act_type=MPT_GET_RATE_TABLE;
				pmp_wi_cntx->param.io_offset=0x0;
				pmp_wi_cntx->param.bytes_cnt=sizeof(struct getratable_rsp);
				pmp_wi_cntx->param.io_value=0xffffffff;

				res=getrttbl_cmd(Adapter,(struct getratable_rsp *)pmp_wi_cntx->param.data);
				if(res != _SUCCESS)
			     	{
				   Status = NDIS_STATUS_NOT_ACCEPTED;
			      	}
			}
			DEBUG_INFO(("\n <=== Set OID_RT_PRO_H2C_GET_RATE_TABLE.\n"));
	#endif
			break;
		}
		default:
			DEBUG_ERR(("\n ===>this oid setting is  not suppoered now.\n"));
			break;
			
	}
#if 1
	if(KeGetCurrentIrql() == PASSIVE_LEVEL) {
		KeRaiseIrql(DISPATCH_LEVEL, &oldirql);
	}
#endif	   
	return Status;

}

#endif
