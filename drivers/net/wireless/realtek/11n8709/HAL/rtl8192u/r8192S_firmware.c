/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/
// **************************************************************************************************
// * Procedure:    Init boot code/firmware code/data session
// *
// * Description: This routine will intialize firmware. If any error occurs during the initialization 
// * 		process, the routine shall terminate immediately and return fail.
// *		NIC driver should call NdisOpenFile only from MiniportInitialize.	
// *
// * Arguments:   The pointer of the adapter
//
// * Returns:
// *        NDIS_STATUS_FAILURE - the following initialization process should be terminated
// *        NDIS_STATUS_SUCCESS - if firmware initialization process success
// **************************************************************************************************/
//#include "rtllib.h"

#include "r8192U.h"
#include "r8192S_firmware.h"
#include <linux/unistd.h>


#include "r8192S_hw.h"
#include "r8192SU_HWImg.h"
//#include "r8192S_FwImgDTM.h"


#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
#include <linux/firmware.h>
#endif

#ifdef RTK_DMP_PLATFORM
#include <linux/usb_setting.h> // for USB_USE_ALIGNMENT
#endif

#define   byte(x,n)  ( (x >> (8 * n)) & 0xff  )


RT_STATUS
FirmwareEnableCPU(struct net_device *dev)
{

	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	u8		tmpU1b, CPUStatus = 0;
	u16		tmpU2b;	
	u32		iCheckTime = 200;	

	RT_TRACE(COMP_FIRMWARE, "-->FirmwareEnableCPU()\n" );

	// Enable CPU.
	tmpU1b = read_nic_byte(dev, SYS_CLKR);
	write_nic_byte(dev,  SYS_CLKR, (tmpU1b|SYS_CPU_CLKSEL)); //AFE source

	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);	
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|FEN_CPUEN));

	//Polling IMEM Ready after CPU has refilled.
	do
	{
		CPUStatus = read_nic_byte(dev, TCR);
		if(CPUStatus& IMEM_RDY)
		{
			RT_TRACE(COMP_FIRMWARE, "IMEM Ready after CPU has refilled.\n");	
			break;		
		}

		//usleep(100);
		udelay(50);
		udelay(50);
	}while(iCheckTime--);

	if(!(CPUStatus & IMEM_RDY))
		return RT_STATUS_FAILURE;

	RT_TRACE(COMP_FIRMWARE, "<--FirmwareEnableCPU(): rtStatus(%#x)\n", rtStatus);
	return rtStatus;			
}


bool
FirmwareCheckReady(struct net_device *dev,	u8 LoadFWStatus)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	rt_firmware	*pFirmware = priv->pFirmware;
	int			PollingCnt = 1000;
	u8	 	CPUStatus = 0;
	u32		tmpU4b;

	RT_TRACE(COMP_FIRMWARE, "--->FirmwareCheckReady(): LoadStaus(%d),", LoadFWStatus);

	pFirmware->FWStatus = (FIRMWARE_8192S_STATUS)LoadFWStatus;	
	if( LoadFWStatus == FW_STATUS_LOAD_IMEM)
	{				
		do
		{//Polling IMEM code done.
			CPUStatus = read_nic_byte(dev, TCR);
			if(CPUStatus& IMEM_CODE_DONE)
				break;	
			
			udelay(5);
		}while(PollingCnt--);		
		if(!(CPUStatus & IMEM_CHK_RPT) || PollingCnt <= 0)
		{
			RT_TRACE(COMP_ERR, "FW_STATUS_LOAD_IMEM FAIL CPU, Status=%x\r\n", CPUStatus);
			return false;			
		}
	}
	else if( LoadFWStatus == FW_STATUS_LOAD_EMEM)
	{//Check Put Code OK and Turn On CPU
		do
		{//Polling EMEM code done.
			CPUStatus = read_nic_byte(dev, TCR);
			if(CPUStatus& EMEM_CODE_DONE)
				break;
			
			udelay(5);
		}while(PollingCnt--);		
		if(!(CPUStatus & EMEM_CHK_RPT))
		{
			RT_TRACE(COMP_ERR, "FW_STATUS_LOAD_EMEM FAIL CPU, Status=%x\r\n", CPUStatus);
			return false;
		}

		// Turn On CPU
		rtStatus = FirmwareEnableCPU(dev);
		if(rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_ERR, "Enable CPU fail ! \n" );
			return false;
		}		
	}
	else if( LoadFWStatus == FW_STATUS_LOAD_DMEM)
	{
		do
		{//Polling DMEM code done
			CPUStatus = read_nic_byte(dev, TCR);
			if(CPUStatus& DMEM_CODE_DONE)
				break;		
			
			udelay(5);
		}while(PollingCnt--);
		
		if(!(CPUStatus & DMEM_CODE_DONE))
		{
			RT_TRACE(COMP_ERR, "Polling  DMEM code done fail ! CPUStatus(%#x)\n", CPUStatus);
			return false;
		}

		RT_TRACE(COMP_FIRMWARE, "DMEM code download success, CPUStatus(%#x)\n", CPUStatus);		
                
		PollingCnt = 1000; // Set polling cycle to 10000ms.
              
		do
		{//Polling Load Firmware ready
			CPUStatus = read_nic_byte(dev, TCR);
			if(CPUStatus & FWRDY)
				break;		
						
			udelay(100);
		}while(PollingCnt--);	
		
		RT_TRACE(COMP_FIRMWARE, "Polling Load Firmware ready, CPUStatus(%x)\n", CPUStatus);		

		if((CPUStatus & LOAD_FW_READY) != LOAD_FW_READY)
		{
			RT_TRACE(COMP_ERR, "Polling Load Firmware ready fail ! CPUStatus(%x)\n", CPUStatus);
			return false;
		}

	       //
              // <Roger_Notes> USB interface will update reserved followings parameters later!! 
              // 2008.08.28.
              //
#ifdef RTL8192SE
		//write_nic_dword(dev, RQPN, 0x10101010);
		//write_nic_byte(dev,  0xAB, 0x80);
#endif 

	       //
              // <Roger_Notes> If right here, we can set TCR/RCR to desired value 
              // and config MAC lookback mode to normal mode. 2008.08.28.
              //
              tmpU4b = read_nic_dword(dev,TCR);
		write_nic_dword(dev, TCR, (tmpU4b&(~TCR_ICV)));

		tmpU4b = read_nic_dword(dev, RCR);
		write_nic_dword(dev, RCR, 
			(tmpU4b|RCR_APPFCS|RCR_APP_ICV|RCR_APP_MIC));
		
		RT_TRACE(COMP_FIRMWARE, "FirmwareCheckReady(): Current RCR settings(%#x)\n", tmpU4b);
		
#if (RTL8192SU_FPGA_2MAC_VERIFICATION ||RTL8192SU_ASIC_VERIFICATION)
		priv->TransmitConfig = read_nic_dword(dev, TCR);
		RT_TRACE(COMP_FIRMWARE, "FirmwareCheckReady(): Current TCR settings(%x)\n", priv->TransmitConfig);		
		priv->FwRsvdTxPageCfg = read_nic_byte(dev, FW_RSVD_PG_CRTL);
#endif
		
		// Set to normal mode.
		write_nic_byte(dev, LBKMD_SEL, LBK_NORMAL);		

	}	

	RT_TRACE(COMP_FIRMWARE, "<---FirmwareCheckReady(): LoadFWStatus(%d), rtStatus(%x)\n", LoadFWStatus, rtStatus);
	return (rtStatus == RT_STATUS_SUCCESS) ? true:false;		
}


//
// Description:   This routine will intialize firmware. If any error occurs during the initialization 
//				process, the routine shall terminate immediately and return fail.
//
// Arguments:   The pointer of the adapter
//			   Code address (Virtual address, should fill descriptor with physical address)
//			   Code size
// Created by Roger, 2008.04.10.
// 
bool FirmwareDownloadCode(struct net_device *dev, u8 *code_virtual_address, u32 buffer_len)
{
	struct r8192_priv	*priv = rtllib_priv(dev);
	bool				rt_status = true;
	u16				frag_threshold = RTL8192S_FW_PKT_FRAG_SIZE;
	u16				frag_length, frag_offset = 0, ExtraDescOffset = 0;
	struct sk_buff	*skb;
	unsigned char		*seg_ptr;
	cb_desc			*tcb_desc;	
	u8				bLastIniPkt = 0;


	RT_TRACE(COMP_FIRMWARE, "--->FirmwareDownloadCode()\n" );	

	//MAX_TRANSMIT_BUFFER_SIZE
	if(buffer_len >= MAX_FIRMWARE_CODE_SIZE-USB_HWDESC_HEADER_LEN)
	{
		RT_TRACE(COMP_ERR, "Size over MAX_FIRMWARE_CODE_SIZE! \n");
		goto cmdsend_downloadcode_fail;
	}
	
	ExtraDescOffset = USB_HWDESC_HEADER_LEN;
	
	do {
		if((buffer_len-frag_offset) > frag_threshold)
		{
			frag_length = frag_threshold + ExtraDescOffset;
		}
		else
		{
			frag_length = (u16)(buffer_len - frag_offset + ExtraDescOffset);
			bLastIniPkt = 1;	
		}

		// Allocate skb buffer to contain firmware info and tx descriptor info. */
#ifdef USB_USE_ALIGNMENT
		u32 Tmpaddr;
		int alignment;
		skb  = dev_alloc_skb(frag_length + USB_512B_ALIGNMENT_SIZE);
#else
		skb  = dev_alloc_skb(frag_length);
#endif

		if (skb == NULL) {
			RT_TRACE(COMP_ERR, "dev_alloc_skb() failed! \n");
			goto cmdsend_downloadcode_fail;
		}

		skb->dev = dev;
		//memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
		
		tcb_desc = (cb_desc*)(skb->cb + MAX_DEV_ADDR_SIZE);
		tcb_desc->queue_index = TXCMD_QUEUE;
		tcb_desc->bCmdOrInit = DESC_PACKET_TYPE_INIT;
		tcb_desc->bLastIniPkt = bLastIniPkt;

#ifdef USB_USE_ALIGNMENT
		Tmpaddr = (u32)skb->data;
		alignment = Tmpaddr & 0x1ff;
		skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));
#endif

		skb_reserve(skb, ExtraDescOffset);

		seg_ptr = (u8 *)skb_put(skb, (u32)(frag_length-ExtraDescOffset));
		memcpy(seg_ptr, code_virtual_address+frag_offset, (u32)(frag_length-ExtraDescOffset));
		
		tcb_desc->txbuf_size= frag_length;

		if(!priv->rtllib->check_nic_enough_desc(dev,tcb_desc->queue_index)||
			(!skb_queue_empty(&priv->rtllib->skb_waitQ[tcb_desc->queue_index]))||\
			(priv->rtllib->queue_stop) ) 
		{
			RT_TRACE(COMP_FIRMWARE,"=====================================================> tx full!\n");
			skb_queue_tail(&priv->rtllib->skb_waitQ[tcb_desc->queue_index], skb);
		} 
		else 
		{
			priv->rtllib->softmac_hard_start_xmit(skb,dev);
		}
		
		frag_offset += (frag_length - ExtraDescOffset);

	}while(frag_offset < buffer_len);
	
	return rt_status ;


cmdsend_downloadcode_fail:	
	rt_status = false;
	RT_TRACE(COMP_ERR, "CmdSendDownloadCode fail !!\n");
	return rt_status;

}


//
// Description:   This routine is to update the RF types in FW header partially.
//
// Created by Roger, 2008.12.24.
//
u8 FirmwareHeaderMapRfType(struct net_device *dev)
{	
	struct r8192_priv 	*priv = rtllib_priv(dev);
	switch(priv->rf_type)
	{
		case RF_1T1R: 	return 0x11;
		case RF_1T2R: 	return 0x12;
		case RF_2T2R: 	return 0x22;
		case RF_2T2R_GREEN: 	return 0x92;
		default:
			RT_TRACE(COMP_INIT, "Unknown RF type(%x)\n",priv->rf_type);
			break;
	}
	return 0x22;
}


//
// Description:   This routine is to update the private parts in FW header partially.
//
// Created by Roger, 2008.12.18.
//
void FirmwareHeaderPriveUpdate(struct net_device *dev, PRT_8192S_FIRMWARE_PRIV 	pFwPriv)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);

	// Update RF types for RATR settings.
	pFwPriv->rf_config = FirmwareHeaderMapRfType(dev);
	
	// Update USB endpoint number for RQPN settings.
	pFwPriv->usb_ep_num = priv->EEPROMUsbEndPointNumber; // endpoint number: 4, 6 and 11.
	RT_TRACE(COMP_INIT, "FirmwarePriveUpdate(): usb_ep_num(%#x)\n", pFwPriv->usb_ep_num);
}


FIRMWARE_8192S_STATUS
FirmwareGetNextStatus(FIRMWARE_8192S_STATUS FWCurrentStatus)
{
	FIRMWARE_8192S_STATUS	NextFWStatus = 0;
	
	switch(FWCurrentStatus)
	{
		case FW_STATUS_INIT:
			NextFWStatus = FW_STATUS_LOAD_IMEM;
			break;

		case FW_STATUS_LOAD_IMEM:
			NextFWStatus = FW_STATUS_LOAD_EMEM;
			break;
		
		case FW_STATUS_LOAD_EMEM:
			NextFWStatus = FW_STATUS_LOAD_DMEM;
			break;

		case FW_STATUS_LOAD_DMEM:
			NextFWStatus = FW_STATUS_READY;
			break;

		default:
			RT_TRACE(COMP_ERR,"Invalid FW Status(%#x)!!\n", FWCurrentStatus);			
			break;
	}	
	return	NextFWStatus;
}


bool FirmwareDownload92S(struct net_device *dev)
{	
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool				rtStatus = true;
	const char 		*pFwImageFileName[1] = {"RTL8192SU/rtl8192sfw.bin"};
	u8				*pucMappedFile = NULL;
	u32				ulFileLength, ulInitStep = 0;	
	u8				FwHdrSize = RT_8192S_FIRMWARE_HDR_SIZE;	
	rt_firmware		*pFirmware = priv->pFirmware;
	u8				FwStatus = FW_STATUS_INIT;
	PRT_8192S_FIRMWARE_HDR		pFwHdr = NULL;
	PRT_8192S_FIRMWARE_PRIV		pFwPriv = NULL;
	int 				rc;
	const struct firmware 	*fw_entry;
	u32				file_length = 0;
	
	pFirmware->FWStatus = FW_STATUS_INIT;

	RT_TRACE(COMP_FIRMWARE, " --->FirmwareDownload92S()\n");

	//3//
	//3 //<1> Open Image file, and map file to contineous memory if open file success.
	//3  //        or read image file from array. Default load from BIN file
	//3//
#if defined(USE_FW_SOURCE_HEADER_FILE) || defined(RTK_DMP_PLATFORM)
	printk("Using FW_SOURCE_HEADER_FILE\n");
	priv->firmware_source = FW_SOURCE_HEADER_FILE;
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	priv->firmware_source = FW_SOURCE_HEADER_FILE;
#else
	priv->firmware_source = FW_SOURCE_IMG_FILE;// We should decided by Reg.
#endif
#endif

	switch( priv->firmware_source )
	{
		case FW_SOURCE_IMG_FILE:		
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)		
			if(pFirmware->szFwTmpBufferLen == 0)
			{

				rc = request_firmware(&fw_entry, pFwImageFileName[ulInitStep],&priv->udev->dev);//===>1
				if(rc < 0 ) {
					RT_TRACE(COMP_ERR, "request firmware fail!\n");
					goto DownloadFirmware_Fail;
				} 

				if(fw_entry->size > sizeof(pFirmware->szFwTmpBuffer)) 
				{
					RT_TRACE(COMP_ERR, "img file size exceed the container buffer fail!\n");
					release_firmware(fw_entry);
					goto DownloadFirmware_Fail;
				}

				memcpy(pFirmware->szFwTmpBuffer,fw_entry->data,fw_entry->size);
				pFirmware->szFwTmpBufferLen = fw_entry->size;
				release_firmware(fw_entry);

				pucMappedFile = pFirmware->szFwTmpBuffer;
				file_length = pFirmware->szFwTmpBufferLen;

				//Retrieve FW header.
				pFirmware->pFwHeader = (PRT_8192S_FIRMWARE_HDR) pucMappedFile;
				pFwHdr = pFirmware->pFwHeader;
				RT_TRACE(COMP_FIRMWARE,"signature:%x, version:%x, size:%x, imemsize:%x, sram size:%x\n", \
						pFwHdr->Signature, pFwHdr->Version, pFwHdr->DMEMSize, \
						pFwHdr->IMG_IMEM_SIZE, pFwHdr->IMG_SRAM_SIZE);
				pFirmware->FirmwareVersion =  byte(pFwHdr->Version ,0);
				if ((pFwHdr->IMG_IMEM_SIZE==0) || (pFwHdr->IMG_IMEM_SIZE > sizeof(pFirmware->FwIMEM)))
				{
					RT_TRACE(COMP_ERR, "%s: memory for data image is less than IMEM required\n",\
							__FUNCTION__);
					goto DownloadFirmware_Fail;
				} else {
					pucMappedFile+=FwHdrSize;

					//Retrieve IMEM image.
					memcpy(pFirmware->FwIMEM, pucMappedFile, pFwHdr->IMG_IMEM_SIZE);
					pFirmware->FwIMEMLen = pFwHdr->IMG_IMEM_SIZE;
				}

				if (pFwHdr->IMG_SRAM_SIZE > sizeof(pFirmware->FwEMEM))
				{
					RT_TRACE(COMP_ERR, "%s: memory for data image is less than EMEM required\n",\
							__FUNCTION__);
					goto DownloadFirmware_Fail;
				} else {
					pucMappedFile += pFirmware->FwIMEMLen;

					// Retriecve EMEM image */
					memcpy(pFirmware->FwEMEM, pucMappedFile, pFwHdr->IMG_SRAM_SIZE);//===>6 
					pFirmware->FwEMEMLen = pFwHdr->IMG_SRAM_SIZE;
				}


			}
#endif
			break;	

		case FW_SOURCE_HEADER_FILE:	

#define Rtl819XFwImageArray Rtl8192SUFwImgArray
			//2008.11.10 Add by tynli.
			pucMappedFile = Rtl819XFwImageArray;
			ulFileLength = ImgArrayLength;

			RT_TRACE(COMP_INIT,"Fw download from header.\n");
			// Retrieve FW header*/
			pFirmware->pFwHeader = (PRT_8192S_FIRMWARE_HDR) pucMappedFile;
			pFwHdr = pFirmware->pFwHeader;
			pFwPriv = (PRT_8192S_FIRMWARE_PRIV)&pFwHdr->FWPriv;
			RT_TRACE(COMP_FIRMWARE,"signature:%x, version:%x, size:%x, imemsize:%x, sram size:%x\n", \
					pFwHdr->Signature, pFwHdr->Version, pFwHdr->DMEMSize, \
					pFwHdr->IMG_IMEM_SIZE, pFwHdr->IMG_SRAM_SIZE);
			pFirmware->FirmwareVersion =  byte(pFwHdr->Version ,0);

			if ((pFwHdr->IMG_IMEM_SIZE==0) || (pFwHdr->IMG_IMEM_SIZE > sizeof(pFirmware->FwIMEM)))
			{
				printk("FirmwareDownload92S(): memory for data image is less than IMEM required\n");	
				goto DownloadFirmware_Fail;
			} else {				
				pucMappedFile+=FwHdrSize;
				//Retrieve IMEM image.
				memcpy(pFirmware->FwIMEM, pucMappedFile, pFwHdr->IMG_IMEM_SIZE);
				pFirmware->FwIMEMLen = pFwHdr->IMG_IMEM_SIZE;				
			}

			if (pFwHdr->IMG_SRAM_SIZE > sizeof(pFirmware->FwEMEM))
			{
				printk(" FirmwareDownload92S(): memory for data image is less than EMEM required\n");					
				goto DownloadFirmware_Fail;
			} else {	
				pucMappedFile+= pFirmware->FwIMEMLen;

				//Retriecve EMEM image.
				memcpy(pFirmware->FwEMEM, pucMappedFile, pFwHdr->IMG_SRAM_SIZE);
				pFirmware->FwEMEMLen = pFwHdr->IMG_SRAM_SIZE;		
			}

			break;
		default:
			break;
	}			

	FwStatus = FirmwareGetNextStatus(pFirmware->FWStatus);
	while(FwStatus!= FW_STATUS_READY)
	{
		// Image buffer redirection.
		switch(FwStatus)
		{
			case FW_STATUS_LOAD_IMEM:				
				pucMappedFile = pFirmware->FwIMEM;
				ulFileLength = pFirmware->FwIMEMLen;					
				break;

			case FW_STATUS_LOAD_EMEM:				
				pucMappedFile = pFirmware->FwEMEM;
				ulFileLength = pFirmware->FwEMEMLen;					
				break;

			case FW_STATUS_LOAD_DMEM:			
				// <Roger_Notes> Partial update the content of header private. 2008.12.18 */
				pFwHdr = pFirmware->pFwHeader;
				pFwPriv = (PRT_8192S_FIRMWARE_PRIV)&pFwHdr->FWPriv;
				FirmwareHeaderPriveUpdate(dev, pFwPriv);
				pucMappedFile = (u8*)(pFirmware->pFwHeader)+RT_8192S_FIRMWARE_HDR_EXCLUDE_PRI_SIZE;
				ulFileLength = FwHdrSize-RT_8192S_FIRMWARE_HDR_EXCLUDE_PRI_SIZE;
				break;
					
			default:
				RT_TRACE(COMP_ERR, "Unexpected Download step!!\n");
				goto DownloadFirmware_Fail;
				break;
		}		
		
		//3//
		//3// <2> Download image file 
	//3	//
		rtStatus = FirmwareDownloadCode(dev, pucMappedFile, ulFileLength);	
		
		if(rtStatus != true)
		{
			RT_TRACE(COMP_ERR, "FirmwareDownloadCode() fail ! \n" );
			goto DownloadFirmware_Fail;
		}	
		
		//3//
		//3// <3> Check whether load FW process is ready 
	//3	// 		
		rtStatus = FirmwareCheckReady(dev, FwStatus);

		if(rtStatus != true)
		{
			RT_TRACE(COMP_ERR, "FirmwareDownloadCode() fail ! \n");
			goto DownloadFirmware_Fail;
		}
		
		FwStatus = FirmwareGetNextStatus(pFirmware->FWStatus);
	}

	RT_TRACE(COMP_FIRMWARE, "Firmware Download Success!!\n");	
	return rtStatus;	
	
	DownloadFirmware_Fail:	
	RT_TRACE(COMP_INIT, "Firmware Download Fail!! Dump related configurations.\n");
	RT_TRACE(COMP_INIT, "Dump CMDR(0x40): %x\n", read_nic_dword(dev, CMDR));
	RT_TRACE(COMP_ERR, "Firmware Download Fail!!%x\n",read_nic_word(dev, TCR));	
	RT_TRACE(COMP_INIT, "Dump CPUINST(0x318): %x\n", read_nic_word(dev, CPUINST));
	RT_TRACE(COMP_INIT, "Dump CPUCAUSE(0x31C): %x\n", read_nic_word(dev, CPUCAUSE));
	RT_TRACE(COMP_INIT, "Dump SYS_CLKR(0x08): %x\n", read_nic_word(dev, SYS_CLKR));
	
	rtStatus = false;
	return rtStatus;	
}

