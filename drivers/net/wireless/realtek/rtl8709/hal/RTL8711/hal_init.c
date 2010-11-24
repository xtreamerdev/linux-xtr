/******************************************************************************
* hal_init.c                                                                                                                                 *
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

#define _HAL_INIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include<rtl8711_byteorder.h>

#include <hal_init.h>


#ifdef CONFIG_SDIO_HCI
			#include <sdio_hal.h>
#elif CONFIG_USB_HCI
			#include <usb_hal.h>
#elif CONFIG_CFIO_HCI
			#include <cfio_hal.h>
#endif	

#ifdef PLATFORM_OS_LINUX
#include <linux/firmware.h>
#endif

	
#ifdef CONFIG_EMBEDDED_FWIMG
#include <farray.h>
#endif


u32 rtl8711_open_fw(_adapter * padapter, void **pphfwfile_hdl, u8 **ppmappedfw)
{
	u32 len;
	
#ifdef  CONFIG_EMBEDDED_FWIMG	

	*ppmappedfw = f_array;
       len = sizeof(f_array);	
	   
#else

#ifdef PLATFORM_OS_XP
	NDIS_PHYSICAL_ADDRESS	ndisphysaddr;	
	NDIS_STATUS			ndisstatus;
	_nic_hdl				*phfilehandle;		//FW file handle
	
	phfilehandle = (_nic_hdl *)(pphfwfile_hdl);
	NdisInitializeString(&padapter->fw_img, "rtl8711fw.bin");	
	if( (padapter->fw_img.Buffer != NULL) && (padapter->fw_img.Length > 0))
	{
		// Open the file : rtl8711fw.bin
		ndisphysaddr.LowPart = ndisphysaddr.HighPart = -1;		
		NdisOpenFile(&ndisstatus,
					phfilehandle,
					&len,
					&padapter->fw_img,
					ndisphysaddr);		
		if(ndisstatus == NDIS_STATUS_SUCCESS){	
			
			NdisMapFile(&ndisstatus, ppmappedfw, *phfilehandle);// Map the FW file into memory.	
			
			if(ndisstatus != NDIS_STATUS_SUCCESS){		
				len = 0;			
				NdisCloseFile(*phfilehandle);// Close the file.
			}	
		}
		else	{
			len = 0;		
			NdisFreeString(padapter->fw_img);// Release the NDIS_STRING allocated via NdisInitializeString().		
		}
		
	}
	else	{
		len = 0;
	}
	
#endif

#ifdef PLATFORM_OS_LINUX
	int rc;		
	struct firmware **praw = (struct firmware **)(pphfwfile_hdl);	
#ifdef CONFIG_SDIO_HCI		
	struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)(&padapter->dvobjpriv);
	PSDDEVICE psddev = pdvobjpriv->psddev;
	rc = request_firmware(praw, "rtl8711fw.bin", SD_GET_OS_DEVICE(psddev));
#endif
#ifdef CONFIG_USB_HCI
	struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)(&padapter->dvobjpriv);
	struct usb_device *pusbdev=pdvobjpriv->pusbdev;
	rc = request_firmware((const struct firmware **)praw, "rtl8711fw.bin", &pusbdev->dev);
	DEBUG_ERR(("request_firmware: Reason 0x%.8x\n", rc));
#endif
       if (rc < 0) {
		DEBUG_ERR(("request_firmware failed: Reason 0x%.8x\n", rc));
		len = 0;
	}else{
	       *ppmappedfw = (u8 *)((*praw)->data);
       	len = (*praw)->size;
		
	}
#endif

#endif

 	return len;

}


void rtl8711_close_fw(_adapter *padapter, void *phfwfile_hdl)
{

#ifdef  CONFIG_EMBEDDED_FWIMG	
	   
#else
#ifdef PLATFORM_OS_XP
		_nic_hdl   hfilehandle = (_nic_hdl)phfwfile_hdl;
		if(hfilehandle)
		{
			NdisUnmapFile(hfilehandle);		
			NdisCloseFile(hfilehandle);// Close the file.				
			NdisFreeString(padapter->fw_img);// Release the NDIS_STRING allocated via NdisInitializeString().
		}
#endif

#ifdef PLATFORM_OS_LINUX
	    struct firmware *praw = (struct firmware *)phfwfile_hdl;
	    if(praw)	   
           	release_firmware(praw);
#endif
#endif

}

u8 rtl8711_dl_fw(_adapter * padapter){


	
	u32			i,fwhdrsz;
	u32 			tmpbusaddress;
	struct fw_hdr		fwhdr;	
	u32			ulfilelength;	//FW file size	
	void			*phfwfile_hdl = NULL;
	u8*			pmappedfw = NULL;		
	u8			ret8=_SUCCESS;
	struct registry_priv* pregistrypriv = &padapter->registrypriv;

_func_enter_;

	ulfilelength = rtl8711_open_fw(padapter, &phfwfile_hdl, &pmappedfw);
	if(padapter->bSurpriseRemoved){
		//DbgPrint("rtl8711_dl_fw: padapter->bSurpriseRemoved =%d",padapter->bSurpriseRemoved);
		ret8=_FAIL;
		goto exit;
	}
	if(pmappedfw && (ulfilelength>0))
	{		
	   	              fwhdr.signature = le16_to_cpu(*(u16 *)pmappedfw);
				fwhdr.version= le16_to_cpu(*(u16 *)(pmappedfw+2));
				fwhdr.fw_priv_sz = le32_to_cpu(*(u32 *)(pmappedfw+4));
				fwhdr.fwpriv=*(struct fw_priv *)(pmappedfw+8);
				fwhdr.imgsz = le32_to_cpu(*(u32 *)(pmappedfw+8+fwhdr.fw_priv_sz));
				fwhdrsz=16+fwhdr.fw_priv_sz;

				DEBUG_INFO(("signature=%04x, version=%04x, FWhdr.Fw_priv.regulatory_class = %x	FW_priv_sz = %d fwsz=%d\n", 
											fwhdr.signature, fwhdr.version, fwhdr.fwpriv.regulatory_class ,fwhdr.fw_priv_sz, fwhdr.imgsz));
			
				if (fwhdr.signature != 0x8711) {			
					DEBUG_ERR(("\nSignature does not match (Signature %d != 8711)! Issue complaints for fw coder\n", fwhdr.signature));
					ret8=_FAIL;
					goto exit;
				}

					             
				if (fwhdr.imgsz != (ulfilelength- fwhdrsz)) {				
					DEBUG_ERR(("\nFW Image size does not match! Issue complaints for fw coder \n	FWhdr.Imgsz  = %d! ulFileLength- fwhdrsz = %d!\n",
						fwhdr.imgsz,ulfilelength-fwhdrsz));
					ret8=_FAIL;
					goto exit;
				}
				//firmware check ok

				DEBUG_INFO(("Downloading RTL8711 firmware major(%d)/minor(%d) version...\n", fwhdr.version >>8, fwhdr.version & 0xff));

			       //Download FirmWare
				tmpbusaddress = FIRMWARE_DOWNLOAD_ADDRESS;
		#ifdef dlfw_blk_wo_chk
			write_mem(padapter, tmpbusaddress,fwhdr.imgsz, (pmappedfw+fwhdrsz));
		#else
{		u32	tmpwr32,tmprd32; //for compare usage
				for(i=0; i<fwhdr.imgsz; i=i+4) {				
					
					tmpwr32= le32_to_cpu(*(u32*)(pmappedfw+fwhdrsz+i));	
					
					write32(padapter, tmpbusaddress+i, tmpwr32);
					tmprd32 = read32(padapter, tmpbusaddress+i);

					if(tmpwr32 != tmprd32) {
					
						DEBUG_ERR(("\nRTL8711 Firmware Write(%x) and Read(%x) check error, Address = 0x%X\n",tmpwr32,tmprd32, tmpbusaddress + i));
						ret8=_FAIL;
						goto exit;
					}											
				
				}									
}								
		#endif
				//Write MAC address to SRAM			
				if(padapter->eeprompriv.bempty == _TRUE){
				//	DbgPrint("\n padapter->eeprompriv.bempty == _TRUE , not write mac address to fw!!\n");
				}
				else{
					//DbgPrint("\n padapter->eeprompriv.bempty != _TRUE , write mac address to fw!!\n");
		              	tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.myid) & 0x1FFFFFFC;
					for(i=0; i<6; i++)
					{
					
						write8(padapter, tmpbusaddress, padapter->eeprompriv.mac_addr[i]);					
						//val = read8(padapter, tmpbusaddress);					
						tmpbusaddress++;
					}
				}
                            				
				//Write PHY info to SRAM				
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.regulatory_class) & 0x1FFFFFFC;
				writephyinfo_fw(padapter, tmpbusaddress);
				
				//Configure Wireless Mode... 				
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.wireless_band) & 0x1FFFFFFF;
				write8(padapter, tmpbusaddress, pregistrypriv->wireless_mode);
				
				//Configure RF Interface Select...
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.rfintfs) & 0x1FFFFFFF;
				write8(padapter, tmpbusaddress, pregistrypriv->rfintfs);

				//Configure chip_version...
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.chip_version) & 0x1FFFFFFF;
				write8(padapter, tmpbusaddress, pregistrypriv->chip_version);				

				//Configure HCI_Type...
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.hci_sel) & 0x1FFFFFFF;
				write8(padapter, tmpbusaddress, pregistrypriv->hci);

				//Configure lbk_mode...
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.lbk_mode) & 0x1FFFFFFF;
				write8(padapter, tmpbusaddress, pregistrypriv->lbkmode);

		
				//Configure QoS...
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.qos) & 0x1FFFFFFF;
				write8(padapter, tmpbusaddress, pregistrypriv->wmm_enable);	


				//Configure DIG
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.dig)& 0x1FFFFFFF;
				if(pregistrypriv->mp_mode == 1)
				{					
					write8(padapter, tmpbusaddress, 0);//dig = 0;	
				}
				else
				{
					write8(padapter, tmpbusaddress, 1);//dig = 1;
				}
				//Configure vcsType
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.vcsType)& 0x1FFFFFFF;
				write8(padapter, tmpbusaddress, pregistrypriv->vcs_type);
								
				//Configure vcsMode
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.vcsMode)& 0x1FFFFFFF;
				write8(padapter, tmpbusaddress, pregistrypriv->vrtl_carrier_sense);
				
				//Configure RTSThreadshold
				tmpbusaddress = le32_to_cpu(fwhdr.fwpriv.RTSThreadshold)& 0x1FFFFFFF;
				write16(padapter, tmpbusaddress, pregistrypriv->rts_thresh);
				


		
	}
	else
	{
		ret8 = _FAIL;
	}

exit:

	rtl8711_close_fw(padapter, phfwfile_hdl);
	
_func_exit_;

	return ret8;
	
}	




/*! Initialize hardware and Download Firmware for RTL8711 Device. */
uint	 rtl8711_hal_init(_adapter *padapter) {

	u32			val32,count;
	u8			val8;

	uint status = _SUCCESS;;
	
	_func_enter_;

#ifdef CONFIG_SDIO_HCI
			padapter->halpriv.hal_bus_init=&sd_hal_bus_init;
			padapter->halpriv.hal_bus_deinit=&sd_hal_bus_deinit;
#endif
#ifdef CONFIG_CFIO_HCI
			padapter->halpriv.hal_bus_init=&cf_hal_bus_init;
			padapter->halpriv.hal_bus_deinit=&cf_hal_bus_deinit;
#endif
#ifdef CONFIG_USB_HCI
			padapter->halpriv.hal_bus_init=&usb_hal_bus_init;
			padapter->halpriv.hal_bus_deinit=&usb_hal_bus_deinit;
#endif

		if(padapter->halpriv.hal_bus_init ==NULL){
			DEBUG_ERR(("\nInitialize halpriv.hal_bus_init error!!!\n"));
			goto exit;
		}else{
			val8=padapter->halpriv.hal_bus_init(padapter);
		}
	if(val8==_FAIL){
		DEBUG_ERR(("\n rtl8711_hal_init: hal_bus_init fail\n"));		
		status= _FAIL;
		goto exit;
	}
#if 0
	read_eeprom_content_by_attrib(padapter);
#else
	read_eeprom_content(padapter);
	if(padapter->eeprompriv.bautoload_fail_flag==_TRUE){
		//DbgPrint("padapter->eeprompriv.bautoload_fail_flag==_TRUE");
		padapter->registrypriv.mp_mode=1;
	}
#endif
	
	
	val32 = read32(padapter, SYSCTRL);	
	if(padapter->bSurpriseRemoved){
		DEBUG_ERR(("\n rtl8711_hal_init : after read SYSCTRL :padapter->bSurpriseRemoved \n"));
		status= _FAIL;
		goto exit;
	}
	val32 |= _HCIRXEN;
	write32(padapter, SYSCTRL, val32);
	if(padapter->bSurpriseRemoved){
		DEBUG_ERR(("\nrtl8711_hal_init : after write SYSCTRL :padapter->bSurpriseRemoved \n"));
		status= _FAIL;
		goto exit;
	}

#ifdef CONFIG_USB_HCI

	val32 = (_C2HSET|_H2CCLR|_CPWM   );
	DEBUG_ERR(("\n~~~~~val32 = (_C2HSET|_H2CCLR|_CPWM   );~~~~~\n"));


#else	//CONFIG_SDIO_HCI

	#ifndef CONFIG_POLLING_MODE
       val32 = (_C2HSET|_H2CCLR | _RXDONE | _VOACINT |_VIACINT |_BEACINT|_BMACINT | _BKACINT | _CPWM  ); 
	#else
       val32 = (_C2HSET|_H2CCLR|_RXDONE |_CPWM   ); 
	#endif
#endif
	padapter->ImrContent = val32;
	write32(padapter, HIMR, val32);
	if(padapter->bSurpriseRemoved){
		DEBUG_ERR(("\nrtl8711_hal_init : after write HIMR : padapter->bSurpriseRemoved \n"));
		status= _FAIL;
		goto exit;
	}

	// Reading H2C, and it shall be zero
	val32 = read32(padapter, H2CR);
	if(padapter->bSurpriseRemoved){
		DEBUG_ERR(("\nrtl8711_hal_init : after read H2CR :padapter->bSurpriseRemoved \n"));
		status= _FAIL;
		goto exit;
	}
	if ( val32 )
	{
		
		DEBUG_ERR(("\nH2CR (0x%x) after POR \n",val32));
		write32(padapter, H2CR, val32);
		val32 = read32(padapter, H2CR);
		if ( val32 ){
			DEBUG_ERR(("\n(2)H2CR (0x%x) after POR \n",val32));
		status= _FAIL;
		goto exit;
	}
	}

	val8=rtl8711_dl_fw( padapter);
	if(val8==_FAIL){
		//DbgPrint("download fw fail!!!");
		status= _FAIL;
		goto exit;

	}

	// Release CPU reset
	val32 = read32(padapter,SYSCTRL);
	if(padapter->bSurpriseRemoved){
		DEBUG_ERR(("\nrtl8711_hal_init : after read SYSCTRL(before release CPU) :padapter->bSurpriseRemoved \n"));
		status= _FAIL;
		goto exit;
	}
	val32 |= _CPURST;
	
	write32(padapter, SYSCTRL, val32);
	if(padapter->bSurpriseRemoved){
		DEBUG_ERR(("\nrtl8711_hal_init : after write SYSCTRL(for release CPU) :padapter->bSurpriseRemoved \n"));
		status= _FAIL;
		goto exit;
	}
	val32= read32(padapter,SYSCTRL);
	if(padapter->bSurpriseRemoved){
		DEBUG_ERR(("\nrtl8711_hal_init : after read SYSCTRL(after release CPU) :padapter->bSurpriseRemoved \n"));
		status= _FAIL;
		goto exit;
	}
	DEBUG_INFO((" SYSCTRL after write = 0x%x\n", val32));

	
	// Polling H2C. Firmware boot up completed once H2CR is not zero
	count = 0;
	do
	{
		val32 = read32(padapter, H2CR);

		if(padapter->bSurpriseRemoved){
			DEBUG_ERR(("\nrtl8711_hal_init : after read H2CR(polling after download fw) :padapter->bSurpriseRemoved=%x \n",padapter->bSurpriseRemoved));
			status= _FAIL;
			goto exit;
		}
		if ( val32)
		{
		
			val32= val32 & 0x1FFFFFFF;
		
			DEBUG_ERR(("\nFirmware download OK! H2CSRAMAddress=0x%x\n",val32));			
			break;
		}
		//NdisMSleep(10000); // 
		//sleep_schedulable(10);//waiting 10ms
		msleep_os(10);//10ms
		count++;
	} while (count < 2000);
	
	if (count >= 2000)
	{
		
		DEBUG_ERR( ("\nFirmware download TIMEOUT (1sec) fail: H2CR is zero, H2CR=0x%x\n", val32));
		status= _FAIL;
		goto exit;
	}

	//Read C2HR
	val32= read32(padapter, C2HR);
	if(padapter->bSurpriseRemoved){
		DEBUG_ERR(("\nrtl8711_hal_init : after read C2HR :padapter->bSurpriseRemoved \n"));
		status= _FAIL;
		goto exit;
	}
	if ( val32 )
	{
		val32 = val32 & 0x1FFFFFFC;
		
		DEBUG_ERR(("\nC2HSRAMAddress=0x%x\n",val32));
	}		
	else
	{	
		
		DEBUG_ERR(("\nC2HR is zero !!! \n\n"));
		status= _FAIL;
	}

	padapter->hw_init_completed=_TRUE;

exit:

	_func_exit_;
	return status;
	
}


/*! Initialize hardware and Download Firmware for RTL8711 Device. */
uint	 rtl8711_hal_deinit(_adapter *padapter) {

	u32			val32;
	u8			val8;

//	u16 			i = 0;
//	struct registry_priv *pregistrypriv = &padapter->registrypriv;
	uint		res=_SUCCESS;
	
	_func_enter_;

	//4 Hold CPU reset
	
	val32 = read32(padapter, SYSCTRL);

	write32(padapter, SYSCTRL, 0);
	
	val32 = read32(padapter, SYSCTRL);

	DEBUG_INFO(("SYSCONFIG_SYSCTRL = 0x%x\n", val32));

	if(padapter->halpriv.hal_bus_deinit ==NULL){
		DEBUG_ERR(("\nInitialize halpriv.hal_bus_deinit error!!!\n"));
		goto exit;
	}else{
		val8=padapter->halpriv.hal_bus_deinit(padapter);
	}


exit:

	_func_exit_;
	return res;
}


