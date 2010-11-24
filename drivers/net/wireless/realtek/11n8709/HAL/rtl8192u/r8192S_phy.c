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
// ******************************************************************************
//
// Note:		Merge 92SE/SU PHY config as below
//			1. BB register R/W API
// 			2. RF register R/W API
// 			3. Initial BB/RF/MAC config by reading BB/MAC/RF txt.
// 			3. Power setting API
// 			4. Channel switch API
// 			5. Initial gain switch API.
// 			6. Other BB/MAC/RF API.
// 			
// History:
//	Data		Who		Remark	
//	08/08/2008  MHC    	1. Port from 9x series phycfg.c
//						2. Reorganize code arch and ad description.
//						3. Collect similar function.
//						4. Seperate extern/local API.
//	08/12/2008	MHC		We must merge or move USB PHY relative function later.
//	10/07/2008	MHC		Add IQ calibration for PHY.(Only 1T2R mode now!!!)
//	11/06/2008	MHC		Add TX Power index PG file to config in 0xExx register
//						area to map with EEPROM/EFUSE tx pwr index.
//	
// ******************************************************************************/
#include "r8192U.h"
#include "r8192U_dm.h"
#include "r8192S_rtl6052.h"

#ifdef RTL8192SU
#include "r8192S_hw.h"
#include "r8192S_phy.h"
#include "r8192S_phyreg.h"
#include "r8192SU_HWImg.h"
//#include "r8192S_FwImgDTM.h"
#else
#include "r8192U_hw.h"
#include "r819xU_phy.h"
#include "r819xU_phyreg.h"
#endif

#ifdef ENABLE_DOT11D
#include "dot11d.h"
#endif

//---------------------------Define Local Constant---------------------------*/
// Channel switch:The size of command tables for switch channel*/
#define MAX_PRECMD_CNT 16
#define MAX_RFDEPENDCMD_CNT 16
#define MAX_POSTCMD_CNT 16
#define MAX_DOZE_WAITING_TIMES_9x 64

//------------------------Define local variable------------------------------*/
// 2004-05-11
#ifndef RTL8192SU
static u32	RF_CHANNEL_TABLE_ZEBRA[]={
		0,
		0x085c,//2412 1	 
		0x08dc,//2417 2 
		0x095c,//2422 3 
		0x09dc,//2427 4 
		0x0a5c,//2432 5 
		0x0adc,//2437 6 
		0x0b5c,//2442 7
		0x0bdc,//2447 8
		0x0c5c,//2452 9
		0x0cdc,//2457 10
		0x0d5c,//2462 11
		0x0ddc,//2467 12
		0x0e5c,//2472 13
		//0x0f5c,//2484  
		0x0f72,//2484  //20040810
};
#endif

static	u32 
phy_CalculateBitShift(u32 BitMask);
static	RT_STATUS 
phy_ConfigMACWithHeaderFile(struct net_device* dev);
static void 
phy_InitBBRFRegisterDefinition(struct net_device* dev);
static	RT_STATUS 
phy_BB8192S_Config_ParaFile(struct net_device* dev);
static	RT_STATUS 
phy_ConfigBBWithHeaderFile(struct net_device* dev,u8 ConfigType);
static bool 
phy_SetRFPowerState8192SU(struct net_device* dev,RT_RF_POWER_STATE eRFPowerState);
void 	
SetBWModeCallback8192SUsbWorkItem(struct net_device *dev);
void 
SetBWModeCallback8192SUsbWorkItem(struct net_device *dev);
void 
SwChnlCallback8192SUsbWorkItem(struct net_device *dev );
static void 
phy_FinishSwChnlNow(struct net_device* dev,u8 channel);
static bool
phy_SwChnlStepByStep(
	struct net_device* dev,
	u8		channel,
	u8		*stage,
	u8		*step,
	u32		*delay
	);
static RT_STATUS 
phy_ConfigBBWithPgHeaderFile(struct net_device* dev,u8 ConfigType);
#ifdef RTL8192SE
static  u32 phy_FwRFSerialRead( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset); 
static u32 phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset);
static	void phy_FwRFSerialWrite( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset, u32 Data);
static void phy_RFSerialWrite( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset, u32 Data);
#endif
static long phy_TxPwrIdxToDbm( struct net_device* dev, WIRELESS_MODE   WirelessMode, u8 TxPwrIdx);
static u8 phy_DbmToTxPwrIdx( struct net_device* dev, WIRELESS_MODE WirelessMode, long PowerInDbm);
void phy_SetFwCmdIOCallback(struct net_device* dev);
		
//#if ((HAL_CODE_BASE == RTL8192_S) && (DEV_BUS_TYPE==USB_INTERFACE))
#ifdef RTL8192SU
//
// Description:
//	Base Band read by 4181 to make sure that operation could be done in unlimited cycle.
//
// Assumption:
//		-	Only use on RTL8192S USB interface.
//		-	PASSIVE LEVEL
//
// Created by Roger, 2008.09.06.
//
//use in phy only
u32 phy_QueryUsbBBReg(struct net_device* dev, u32	RegAddr)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u32	ReturnValue = 0xffffffff;
	u8	PollingCnt = 50;
	u8	BBWaitCounter = 0;
	

	//
	// <Roger_Notes> Due to PASSIVE_LEVEL, so we ONLY simply busy waiting for a while here.
	// We have to make sure that previous BB I/O has been done. 
	// 2008.08.20.
	//
	while(priv->bChangeBBInProgress)
	{
		BBWaitCounter ++;
		RT_TRACE(COMP_RF, "phy_QueryUsbBBReg(): Wait 1 ms (%d times)...\n", BBWaitCounter);
		msleep(1); // 1 ms

		// Wait too long, return false to avoid to be stuck here.
		if((BBWaitCounter > 100) )//||RT_USB_CANNOT_IO(Adapter))
		{
			RT_TRACE(COMP_RF, "phy_QueryUsbBBReg(): (%d) Wait too logn to query BB!!\n", BBWaitCounter);
			return ReturnValue;
		}
	}

	priv->bChangeBBInProgress = true;
	
	read_nic_dword(dev, RegAddr);	

	do
	{// Make sure that access could be done.
		if((read_nic_byte(dev, PHY_REG)&HST_RDBUSY) == 0)
			break;
	}while( --PollingCnt );

	if(PollingCnt == 0)
	{
		RT_TRACE(COMP_RF, "Fail!!!phy_QueryUsbBBReg(): RegAddr(%#x) = %#x\n", RegAddr, ReturnValue);
	}
	else
	{
		// Data FW read back.
		ReturnValue = read_nic_dword(dev, PHY_REG_DATA);	
		RT_TRACE(COMP_RF, "phy_QueryUsbBBReg(): RegAddr(%#x) = %#x, PollingCnt(%d)\n", RegAddr, ReturnValue, PollingCnt);
	}
	
	priv->bChangeBBInProgress = false;

	return ReturnValue;
}



//
// Description:
//	Base Band wrote by 4181 to make sure that operation could be done in unlimited cycle.
//
// Assumption:
//		-	Only use on RTL8192S USB interface.
//		-	PASSIVE LEVEL
//
// Created by Roger, 2008.09.06.
//
//use in phy only
void
phy_SetUsbBBReg(struct net_device* dev,u32	RegAddr,u32 Data)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	BBWaitCounter = 0;
	
	RT_TRACE(COMP_RF, "phy_SetUsbBBReg(): RegAddr(%#x) <= %#x\n", RegAddr, Data);

	//
	// <Roger_Notes> Due to PASSIVE_LEVEL, so we ONLY simply busy waiting for a while here.
	// We have to make sure that previous BB I/O has been done. 
	// 2008.08.20.
	//
	while(priv->bChangeBBInProgress)
	{
		BBWaitCounter ++;
		RT_TRACE(COMP_RF, "phy_SetUsbBBReg(): Wait 1 ms (%d times)...\n", BBWaitCounter);
		msleep(1); // 1 ms
	
		if((BBWaitCounter > 100))// || RT_USB_CANNOT_IO(Adapter))
		{
			RT_TRACE(COMP_RF, "phy_SetUsbBBReg(): (%d) Wait too logn to query BB!!\n", BBWaitCounter);
			return;
		}
	}

	priv->bChangeBBInProgress = true;
	//printk("**************%s: RegAddr:%x Data:%x\n", __FUNCTION__,RegAddr, Data);	
	write_nic_dword(dev, RegAddr, Data);
	
	priv->bChangeBBInProgress = false;
}



//
// Description:
//	RF read by 4181 to make sure that operation could be done in unlimited cycle.
//
// Assumption:
//		-	Only use on RTL8192S USB interface.
//		-	PASSIVE LEVEL
//		- 	RT_RF_OPERATE_SPINLOCK is acquired and keep on holding to the end.FIXLZM
//
// Created by Roger, 2008.09.06.
//
//use in phy only
u32 phy_QueryUsbRFReg(	struct net_device* dev, RF90_RADIO_PATH_E eRFPath,	u32	Offset)
{
	
	struct r8192_priv *priv = rtllib_priv(dev);
	//u32	value  = 0, ReturnValue = 0;
	u32	ReturnValue = 0;
	//u32 	tmplong,tmplong2;
	u8	PollingCnt = 50;
	u8	RFWaitCounter = 0;


	//
	// <Roger_Notes> Due to PASSIVE_LEVEL, so we ONLY simply busy waiting for a while here.
	// We have to make sure that previous RF I/O has been done. 
	// 2008.08.20.
	//
	while(priv->bChangeRFInProgress)
	{
		//PlatformReleaseSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
		//spin_lock_irqsave(&priv->rf_lock, flags);	//LZM,090318
		down(&priv->rf_sem);

		RFWaitCounter ++;
		RT_TRACE(COMP_RF, "phy_QueryUsbRFReg(): Wait 1 ms (%d times)...\n", RFWaitCounter);
		msleep(1); // 1 ms

		if((RFWaitCounter > 100)) //|| RT_USB_CANNOT_IO(Adapter))
		{
			//PlatformAcquireSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
			RT_TRACE(COMP_RF, "phy_QueryUsbRFReg(): (%d) Wait too logn to query BB!!\n", RFWaitCounter);
			return 0xffffffff;
		}
		else
		{
			//PlatformAcquireSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
		}
	}

	priv->bChangeRFInProgress = true;
	//PlatformReleaseSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);


	Offset &= 0x3f; //RF_Offset= 0x00~0x3F		
	
	write_nic_dword(dev, RF_BB_CMD_ADDR, 0xF0000002|
						(Offset<<8)|	//RF_Offset= 0x00~0x3F
						(eRFPath<<16)); 	//RF_Path = 0(A) or 1(B)
	
	do
	{// Make sure that access could be done.
		if(read_nic_dword(dev, RF_BB_CMD_ADDR) == 0)
			break;
	}while( --PollingCnt );

	// Data FW read back.
	ReturnValue = read_nic_dword(dev, RF_BB_CMD_DATA);
	
	//PlatformAcquireSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
	//spin_unlock_irqrestore(&priv->rf_lock, flags);   //LZM,090318
	up(&priv->rf_sem);
	priv->bChangeRFInProgress = false;
	
	RT_TRACE(COMP_RF, "phy_QueryUsbRFReg(): eRFPath(%d), Offset(%#x) = %#x\n", eRFPath, Offset, ReturnValue);
	
	return ReturnValue;

}


//
// Description:
//	RF wrote by 4181 to make sure that operation could be done in unlimited cycle.
//
// Assumption:
//		-	Only use on RTL8192S USB interface.
//		-	PASSIVE LEVEL
//		- 	RT_RF_OPERATE_SPINLOCK is acquired and keep on holding to the end.FIXLZM
//
// Created by Roger, 2008.09.06.
//
//use in phy only
void phy_SetUsbRFReg(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32	RegAddr,u32 Data)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	u8	PollingCnt = 50;
	u8	RFWaitCounter = 0;


	//
	// <Roger_Notes> Due to PASSIVE_LEVEL, so we ONLY simply busy waiting for a while here.
	// We have to make sure that previous BB I/O has been done. 
	// 2008.08.20.
	//
	while(priv->bChangeRFInProgress)
	{
		//PlatformReleaseSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
		//spin_lock_irqsave(&priv->rf_lock, flags);	//LZM,090318
		down(&priv->rf_sem);

		RFWaitCounter ++;
		RT_TRACE(COMP_RF, "phy_SetUsbRFReg(): Wait 1 ms (%d times)...\n", RFWaitCounter);
		msleep(1); // 1 ms
	
		if((RFWaitCounter > 100))// || RT_USB_CANNOT_IO(Adapter))
		{
			//PlatformAcquireSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
			RT_TRACE(COMP_RF, "phy_SetUsbRFReg(): (%d) Wait too logn to query BB!!\n", RFWaitCounter);
			return;
		}
		else
		{
			//PlatformAcquireSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
		}
	}

	priv->bChangeRFInProgress = true;
	//PlatformReleaseSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);

	
	RegAddr &= 0x3f; //RF_Offset= 0x00~0x3F
	
	write_nic_dword(dev, RF_BB_CMD_DATA, Data);	
	write_nic_dword(dev, RF_BB_CMD_ADDR, 0xF0000003|
					(RegAddr<<8)| //RF_Offset= 0x00~0x3F
					(eRFPath<<16));  //RF_Path = 0(A) or 1(B)
	
	do
	{// Make sure that access could be done.
		if(read_nic_dword(dev, RF_BB_CMD_ADDR) == 0)
				break;
	}while( --PollingCnt );		

	if(PollingCnt == 0)
	{		
		RT_TRACE(COMP_RF, "phy_SetUsbRFReg(): Set RegAddr(%#x) = %#x Fail!!!\n", RegAddr, Data);
	}

	//PlatformAcquireSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
	//spin_unlock_irqrestore(&priv->rf_lock, flags);   //LZM,090318
	up(&priv->rf_sem);
	priv->bChangeRFInProgress = false;

}

#endif
						
//---------------------Define local function prototype-----------------------*/


//----------------------------Function Body----------------------------------*/
//
// 1. BB register R/W API
//
//
// Function:	PHY_QueryBBReg
//
// OverView:	Read "sepcific bits" from BB register
//
// Input:
//			PADAPTER		Adapter,
//			u32			RegAddr,		//The target address to be readback
//			u32			BitMask		//The target bit position in the target address
//										//to be readback	
// Output:	None
// Return:		u32			Data			//The readback register value
// Note:		This function is equal to "GetRegSetting" in PHY programming guide
//
//use phy dm core 8225 8256 6052
//u32 PHY_QueryBBReg(struct net_device* dev,u32		RegAddr,	u32		BitMask)
u32 rtl8192_QueryBBReg(struct net_device* dev, u32 RegAddr, u32 BitMask)
{

  	u32	ReturnValue = 0, OriginalValue, BitShift;

#if (DISABLE_BB_RF == 1)
	return 0;
#endif

	RT_TRACE(COMP_RF, "--->PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x)\n", RegAddr, BitMask);

	//
	// <Roger_Notes> Due to 8051 operation cycle (limitation cycle: 6us) and 1-Byte access issue, we should use 
	// 4181 to access Base Band instead of 8051 on USB interface to make sure that access could be done in 
	// infinite cycle.
	// 2008.09.06.
	//
//#if ((HAL_CODE_BASE == RTL8192_S) && (DEV_BUS_TYPE==USB_INTERFACE))
#ifdef RTL8192SU
	if(IS_BB_REG_OFFSET_92S(RegAddr))
	{
		//if(RT_USB_CANNOT_IO(Adapter))	return	false;

		if((RegAddr & 0x03) != 0)
		{
			printk("%s: Not DWORD alignment!!\n", __FUNCTION__);
			return 0;
		}
	
	OriginalValue = phy_QueryUsbBBReg(dev, RegAddr);
	}
	else
#endif
	{
	OriginalValue = read_nic_dword(dev, RegAddr);
	}

	BitShift = phy_CalculateBitShift(BitMask);
	ReturnValue = (OriginalValue & BitMask) >> BitShift;

	//RTPRINT(FPHY, PHY_BBR, ("BBR MASK=0x%x Addr[0x%x]=0x%x\n", BitMask, RegAddr, OriginalValue));
	RT_TRACE(COMP_RF, "<---PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x), OriginalValue(%#x)\n", RegAddr, BitMask, OriginalValue);
	return (ReturnValue);
}

//
// Function:	PHY_SetBBReg
//
// OverView:	Write "Specific bits" to BB register (page 8~) 
//
// Input:
//			PADAPTER		Adapter,
//			u32			RegAddr,		//The target address to be modified
//			u32			BitMask		//The target bit position in the target address
//										//to be modified	
//			u32			Data			//The new register value in the target bit position
//										//of the target address			
//
// Output:	None
// Return:		None
// Note:		This function is equal to "PutRegSetting" in PHY programming guide
///
//use phy dm core 8225 8256
//void PHY_SetBBReg(struct net_device* dev,u32		RegAddr,	u32		BitMask,	u32		Data	)
void rtl8192_setBBreg(struct net_device* dev, u32 RegAddr, u32 BitMask, u32 Data)
{
	u32	OriginalValue, BitShift, NewValue;

#if (DISABLE_BB_RF == 1)
	return;
#endif

	RT_TRACE(COMP_RF, "--->PHY_SetBBReg(): RegAddr(%#x), BitMask(%#x), Data(%#x)\n", RegAddr, BitMask, Data);

	//
	// <Roger_Notes> Due to 8051 operation cycle (limitation cycle: 6us) and 1-Byte access issue, we should use 
	// 4181 to access Base Band instead of 8051 on USB interface to make sure that access could be done in 
	// infinite cycle.
	// 2008.09.06.
	//
//#if ((HAL_CODE_BASE == RTL8192_S) && (DEV_BUS_TYPE==USB_INTERFACE))
#ifdef RTL8192SU
	if(IS_BB_REG_OFFSET_92S(RegAddr))
	{
		if((RegAddr & 0x03) != 0)
		{
			printk("%s: Not DWORD alignment!!\n", __FUNCTION__);
			return;
		}
	
		if(BitMask!= bMaskDWord)
		{//if not "double word" write
			OriginalValue = phy_QueryUsbBBReg(dev, RegAddr);
			BitShift = phy_CalculateBitShift(BitMask);
            NewValue = (((OriginalValue) & (~BitMask))|(Data << BitShift));
			phy_SetUsbBBReg(dev, RegAddr, NewValue);	
		}else
			phy_SetUsbBBReg(dev, RegAddr, Data);
	}	
	else
#endif
	{
		if(BitMask!= bMaskDWord)
		{//if not "double word" write
			OriginalValue = read_nic_dword(dev, RegAddr);
			BitShift = phy_CalculateBitShift(BitMask);
			NewValue = (((OriginalValue) & (~BitMask)) | (Data << BitShift));
			write_nic_dword(dev, RegAddr, NewValue);	
		}else
			write_nic_dword(dev, RegAddr, Data);	
	}

	//RT_TRACE(COMP_RF, "<---PHY_SetBBReg(): RegAddr(%#x), BitMask(%#x), Data(%#x)\n", RegAddr, BitMask, Data);

	return;
}


//
// 2. RF register R/W API
//
//
// Function:	PHY_QueryRFReg
//
// OverView:	Query "Specific bits" to RF register (page 8~) 
//
// Input:
//			PADAPTER		Adapter,
//			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
//			u32			RegAddr,		//The target address to be read
//			u32			BitMask		//The target bit position in the target address
//										//to be read	
//
// Output:	None
// Return:		u32			Readback value
// Note:		This function is equal to "GetRFRegSetting" in PHY programming guide
///
//in dm 8256 and phy
//u32 PHY_QueryRFReg(struct net_device* dev,	RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask)
u32 rtl8192_phy_QueryRFReg(struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask)
{
	u32 Original_Value, Readback_Value, BitShift;//, flags;	
	struct r8192_priv *priv = rtllib_priv(dev);
	
#if (DISABLE_BB_RF == 1) 
	return 0;
#endif
	
	RT_TRACE(COMP_RF, "--->PHY_QueryRFReg(): RegAddr(%#x), eRFPath(%#x), BitMask(%#x)\n", RegAddr, eRFPath,BitMask);
	//printk("%s(): rf_pathmap=%x eRFPath=%x\n", __func__,priv->rf_pathmap, eRFPath);
	
	if (!((priv->rf_pathmap >> eRFPath) & 0x1))
	{
		printk("%s()EEEEEError: rfpath off! rf_pathmap=%x eRFPath=%x\n", __func__,priv->rf_pathmap, eRFPath);
		return 0;
	}
	
	if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))
	{
		printk("%s()EEEEEError: not legal rfpath! eRFPath=%x\n",__func__, eRFPath);
		return 0;
	}
	
	// 2008/01/17 MH We get and release spin lock when reading RF register. */
	//PlatformAcquireSpinLock(dev, RT_RF_OPERATE_SPINLOCK);FIXLZM
	//spin_lock_irqsave(&priv->rf_lock, flags);	//YJ,test,090113	
	down(&priv->rf_sem);
	//
	// <Roger_Notes> Due to 8051 operation cycle (limitation cycle: 6us) and 1-Byte access issue, we should use 
	// 4181 to access Base Band instead of 8051 on USB interface to make sure that access could be done in 
	// infinite cycle.
	// 2008.09.06.
	//
//#if (HAL_CODE_BASE == RTL8192_S && DEV_BUS_TYPE==USB_INTERFACE)
#ifdef RTL8192SU
	//if(RT_USB_CANNOT_IO(Adapter))	
	//{
	//	PlatformReleaseSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
	//	return false;
	//}
	Original_Value = phy_QueryUsbRFReg(dev, eRFPath, RegAddr);	
#else
	if (priv->Rf_Mode == RF_OP_By_FW)
	{	
		Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
	}
	else
	{	
		Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);	   	
	}
#endif
	
	BitShift =  phy_CalculateBitShift(BitMask);
	Readback_Value = (Original_Value & BitMask) >> BitShift;	
	//spin_unlock_irqrestore(&priv->rf_lock, flags);   //YJ,test,090113
	up(&priv->rf_sem);
	//PlatformReleaseSpinLock(dev, RT_RF_OPERATE_SPINLOCK);

	//RTPRINT(FPHY, PHY_RFR, ("RFR-%d MASK=0x%x Addr[0x%x]=0x%x\n", eRFPath, BitMask, RegAddr, Original_Value));
	
	return (Readback_Value);
}

//
// Function:	PHY_SetRFReg
//
// OverView:	Write "Specific bits" to RF register (page 8~) 
//
// Input:
//			PADAPTER		Adapter,
//			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
//			u32			RegAddr,		//The target address to be modified
//			u32			BitMask		//The target bit position in the target address
//										//to be modified	
//			u32			Data			//The new register Data in the target bit position
//										//of the target address			
//
// Output:	None
// Return:		None
// Note:		This function is equal to "PutRFRegSetting" in PHY programming guide
///
//use phy  8225 8256
//void PHY_SetRFReg(struct net_device* dev,RF90_RADIO_PATH_E eRFPath, u32	RegAddr,	u32 BitMask,u32	Data	)
void rtl8192_phy_SetRFReg(struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask, u32 Data)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	u32 Original_Value, BitShift, New_Value;//, flags;
#if (DISABLE_BB_RF == 1)
	return;
#endif
	
	RT_TRACE(COMP_RF, "--->PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
		RegAddr, BitMask, Data, eRFPath);

	if (!((priv->rf_pathmap >> eRFPath) & 0x1))
	{
		printk("EEEEEError: rfpath off! rf_pathmap=%x eRFPath=%x\n", priv->rf_pathmap, eRFPath);
		return ;
	}
	if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))
	{
		printk("EEEEEError: not legal rfpath! eRFPath=%x\n", eRFPath);
		return;
	}
	
	// 2008/01/17 MH We get and release spin lock when writing RF register. */
	//PlatformAcquireSpinLock(dev, RT_RF_OPERATE_SPINLOCK);
	//spin_lock_irqsave(&priv->rf_lock, flags);	//YJ,test,090113
	down(&priv->rf_sem);
	//
	// <Roger_Notes> Due to 8051 operation cycle (limitation cycle: 6us) and 1-Byte access issue, we should use 
	// 4181 to access Base Band instead of 8051 on USB interface to make sure that access could be done in 
	// infinite cycle.
	// 2008.09.06.
	//
//#if (HAL_CODE_BASE == RTL8192_S && DEV_BUS_TYPE==USB_INTERFACE)
#ifdef RTL8192SU
		//if(RT_USB_CANNOT_IO(Adapter))	
		//	{
		//	PlatformReleaseSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
		//	return;	
		//	}	

		if (BitMask != bRFRegOffsetMask) // RF data is 12 bits only
		{
			Original_Value = phy_QueryUsbRFReg(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value)&(~BitMask))|(Data<< BitShift));
			phy_SetUsbRFReg(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_SetUsbRFReg(dev, eRFPath, RegAddr, Data);
#else
	if (priv->Rf_Mode == RF_OP_By_FW)	
	{		
		//DbgPrint("eRFPath-%d Addr[%02x] = %08x\n", eRFPath, RegAddr, Data);
		if (BitMask != bRFRegOffsetMask) // RF data is 12 bits only
		{
			Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value) & (~BitMask)) | (Data<< BitShift));
		
			phy_FwRFSerialWrite(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_FwRFSerialWrite(dev, eRFPath, RegAddr, Data);		
	}
	else
	{		
		//DbgPrint("eRFPath-%d Addr[%02x] = %08x\n", eRFPath, RegAddr, Data);
		if (BitMask != bRFRegOffsetMask) // RF data is 12 bits only
		{
			Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value) & (~BitMask)) | (Data<< BitShift));
		
			phy_RFSerialWrite(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_RFSerialWrite(dev, eRFPath, RegAddr, Data);
	
	}
#endif	
	//PlatformReleaseSpinLock(dev, RT_RF_OPERATE_SPINLOCK);
	//spin_unlock_irqrestore(&priv->rf_lock, flags);   //YJ,test,090113
	up(&priv->rf_sem);
	//RTPRINT(FPHY, PHY_RFW, ("RFW-%d MASK=0x%x Addr[0x%x]=0x%x\n", eRFPath, BitMask, RegAddr, Data));
	RT_TRACE(COMP_RF, "<---PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
			RegAddr, BitMask, Data, eRFPath);
	
}

#ifdef RTL8192SE
// *-----------------------------------------------------------------------------
// * Function:	phy_FwRFSerialRead()
// *
// * Overview:	We support firmware to execute RF-R/W.
// *
// * Input:		NONE
// *
// * Output:		NONE
// *
// * Return:		NONE
// *
// * Revised History:
// *	When		Who		Remark
// *	01/21/2008	MHC		Create Version 0.  
// *
// *---------------------------------------------------------------------------*/
//use in phy only
static	u32
phy_FwRFSerialRead(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset	)
{
	u32		retValue = 0;		
	//u32		Data = 0;
	//u8		time = 0;
#if 0	
	//DbgPrint("FW RF CTRL\n\r");
	// 2007/11/02 MH Firmware RF Write control. By Francis' suggestion, we can 
	   not execute the scheme in the initial step. Otherwise, RF-R/W will waste
	   much time. This is only for site survey. */
	// 1. Read operation need not insert data. bit 0-11	
	//Data &= bMask12Bits;
	// 2. Write RF register address. Bit 12-19
	Data |= ((Offset&0xFF)<<12);
	// 3. Write RF path.  bit 20-21
	Data |= ((eRFPath&0x3)<<20);
	// 4. Set RF read indicator. bit 22=0
	//Data |= 0x00000;
	// 5. Trigger Fw to operate the command. bit 31
	Data |= 0x80000000;		
	// 6. We can not execute read operation if bit 31 is 1.
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		// If FW can not finish RF-R/W for more than ?? times. We must reset FW.
		if (time++ < 100)
		{
			//DbgPrint("FW not finish RF-R Time=%d\n\r", time);
			delay_us(10);
		}
		else
			break;
	}
	// 7. Execute read operation.		
	PlatformIOWrite4Byte(dev, QPNR, Data);		
	// 8. Check if firmawre send back RF content.
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		// If FW can not finish RF-R/W for more than ?? times. We must reset FW.
		if (time++ < 100)
		{
			//DbgPrint("FW not finish RF-W Time=%d\n\r", time);
			delay_us(10);
		}
		else
			return	(0);
	}
	retValue = PlatformIORead4Byte(dev, RF_DATA);
#endif	
	return	(retValue);

}	// phy_FwRFSerialRead */

// *-----------------------------------------------------------------------------
// * Function:	phy_FwRFSerialWrite()
// *
// * Overview:	We support firmware to execute RF-R/W.
// *
// * Input:		NONE
// *
// * Output:		NONE
// *
// * Return:		NONE
// *
// * Revised History:
// *	When		Who		Remark
// *	01/21/2008	MHC		Create Version 0.  
// *
// *---------------------------------------------------------------------------*/
//use in phy only
static	void
phy_FwRFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data	)
{
#if 0	
	u8	time = 0;
	DbgPrint("N FW RF CTRL RF-%d OF%02x DATA=%03x\n\r", eRFPath, Offset, Data);
	// 2007/11/02 MH Firmware RF Write control. By Francis' suggestion, we can 
	//  not execute the scheme in the initial step. Otherwise, RF-R/W will waste
	//   much time. This is only for site survey. 
	
	// 1. Set driver write bit and 12 bit data. bit 0-11	
	//Data &= bMask12Bits;	// Done by uper layer.
	// 2. Write RF register address. bit 12-19
	Data |= ((Offset&0xFF)<<12);
	// 3. Write RF path.  bit 20-21
	Data |= ((eRFPath&0x3)<<20);
	// 4. Set RF write indicator. bit 22=1
	Data |= 0x400000;
	// 5. Trigger Fw to operate the command. bit 31=1
	Data |= 0x80000000;
	
	// 6. Write operation. We can not write if bit 31 is 1.
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		// If FW can not finish RF-R/W for more than ?? times. We must reset FW.
		if (time++ < 100)
		{
			delay_us(10);
		}
		else
			break;
	}
	// 7. No matter check bit. We always force the write. Because FW will 
	//    not accept the command.		
	PlatformIOWrite4Byte(dev, QPNR, Data);
	// 2007/11/02 MH Acoording to test, we must delay 20us to wait firmware
	//   to finish RF write operation. 
	// 2008/01/17 MH We support delay in firmware side now. 
	//delay_us(20);
#endif		
}	// phy_FwRFSerialWrite */

//
// Function:	phy_RFSerialRead
//
// OverView:	Read regster from RF chips 
//
// Input:
//			PADAPTER		Adapter,
//			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
//			u32			Offset,		//The target address to be read			
//
// Output:	None
// Return:		u32			reback value
// Note:		Threre are three types of serial operations: 
//			1. Software serial write
//			2. Hardware LSSI-Low Speed Serial Interface 
//			3. Hardware HSSI-High speed
//			serial write. Driver need to implement (1) and (2).
//			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
///
//use in phy only
static u32 phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset)
{

	u32						retValue = 0;
	struct r8192_priv *priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;
	u8						RfPiEnable=0;


	//
	// Make sure RF register offset is correct 
	//
	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||	
		priv->rf_chip == RF_6052)
	{
		//analog to digital off, for protection
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0x0);// 0x88c[11:8]

		if(Offset>=31)
		{
			priv->RFReadPageCnt[2]++;//cosa add for debug
			priv->RfReg0Value[eRFPath] |= 0x140;

			// Switch to Reg_Mode2 for Reg31~45
			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			// Modified Offset
			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			priv->RFReadPageCnt[1]++;//cosa add for debug
			priv->RfReg0Value[eRFPath] |= 0x100;
			priv->RfReg0Value[eRFPath] &= (~0x40);

			// Switch to Reg_Mode1 for Reg16~30
			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			// Modified Offset
			NewOffset = Offset - 15;
		}
		else
		{
			priv->RFReadPageCnt[0]++;//cosa add for debug
			NewOffset = Offset;
	}	
	}	
	else
		NewOffset = Offset;

	//
	// Put desired read address to LSSI control register
	//
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, bLSSIReadAddress, NewOffset);

	//
	// Issue a posedge trigger
	//
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x0);	
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x1);	
	
	// TODO: we should not delay such a  long time. Ask help from SD3
	mdelay(1);

	retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	
	// Switch back to Reg_Mode0;
	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_0222D)
	{
		if (Offset >= 0x10)
		{
			priv->RfReg0Value[eRFPath] &= 0xebf;
		
			rtl8192_setBBreg(
				dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);
		}

		//analog to digital on
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0xf);// 0x88c[11:8]
	}
	
	return retValue;	
}



//
// Function:	phy_RFSerialWrite
//
// OverView:	Write data to RF register (page 8~) 
//
// Input:
//			PADAPTER		Adapter,
//			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
//			u32			Offset,		//The target address to be read			
//			u32			Data			//The new register Data in the target bit position
//										//of the target to be read			
//
// Output:	None
// Return:		None
// Note:		Threre are three types of serial operations: 
//			1. Software serial write
//			2. Hardware LSSI-Low Speed Serial Interface 
//			3. Hardware HSSI-High speed
//			serial write. Driver need to implement (1) and (2).
//			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
//
//  Note: 		  For RF8256 only
// 			 The total count of RTL8256(Zebra4) register is around 36 bit it only employs 
// 			 4-bit RF address. RTL8256 uses "register mode control bit" (Reg00[12], Reg00[10]) 
// 			 to access register address bigger than 0xf. See "Appendix-4 in PHY Configuration
// 			 programming guide" for more details. 
// 			 Thus, we define a sub-finction for RTL8526 register address conversion
// 		       ===========================================================
// 			 Register Mode		RegCTL[1]		RegCTL[0]		Note
// 								        (Reg00[12])		(Reg00[10])
// 		       ===========================================================
// 			 Reg_Mode0		0			x			Reg 0 ~15(0x0 ~ 0xf)
// 		       ------------------------------------------------------------------
// 			 Reg_Mode1		1			0			Reg 16 ~30(0x1 ~ 0xf)
// 		       ------------------------------------------------------------------
// 			 Reg_Mode2		1			1			Reg 31 ~ 45(0x1 ~ 0xf)
// 		       ------------------------------------------------------------------
//
////use in phy only
static void
phy_RFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data
	)
{
	u32					DataAndAddr = 0;
	struct r8192_priv 			*priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32					NewOffset;

	Offset &= 0x3f;

	// Shadow Update
	PHY_RFShadowWrite(dev, eRFPath, Offset, Data);	


	// Switch page for 8256 RF IC
	if( 	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_0222D)
	{
		//analog to digital off, for protection
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0x0);// 0x88c[11:8]
				
		if(Offset>=31)
		{
			priv->RFWritePageCnt[2]++;//cosa add for debug
			priv->RfReg0Value[eRFPath] |= 0x140;
			
			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			priv->RFWritePageCnt[1]++;//cosa add for debug
			priv->RfReg0Value[eRFPath] |= 0x100;
			priv->RfReg0Value[eRFPath] &= (~0x40);			
							

			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 15;
		}
		else
		{
			priv->RFWritePageCnt[0]++;//cosa add for debug
			NewOffset = Offset;
	}
	}
	else
		NewOffset = Offset;

	//
	// Put write addr in [5:0]  and write data in [31:16]
	//
	DataAndAddr = (Data<<16) | (NewOffset&0x3f);

	//
	// Write Operation
	//
	rtl8192_setBBreg(dev, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);


	if(Offset==0x0)
		priv->RfReg0Value[eRFPath] = Data;

	// Switch back to Reg_Mode0;
 	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_0222D)
	{
		if (Offset >= 0x10)
		{
			if(Offset != 0)
			{
				priv->RfReg0Value[eRFPath] &= 0xebf;
				rtl8192_setBBreg(
				dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);
			}
		}	
		//analog to digital on
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0xf);// 0x88c[11:8]	
	}
	
}
#else
//
// Function:	phy_RFSerialRead
//
// OverView:	Read regster from RF chips 
//
// Input:
//			PADAPTER		Adapter,
//			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
//			u4Byte			Offset,		//The target address to be read			
//
// Output:	None
// Return:		u4Byte			reback value
// Note:		Threre are three types of serial operations: 
//			1. Software serial write
//			2. Hardware LSSI-Low Speed Serial Interface 
//			3. Hardware HSSI-High speed
//			serial write. Driver need to implement (1) and (2).
//			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
//
#if 0
static	u32
phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset)
{

	u32						retValue = 0;
	struct r8192_priv *priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;
	//u32						value  = 0;
	u32 						tmplong,tmplong2;
	u32						RfPiEnable=0;
#if 0
	if(pHalData->RFChipID == RF_8225 && Offset > 0x24) //36 valid regs
		return	retValue;
	if(pHalData->RFChipID == RF_8256 && Offset > 0x2D) //45 valid regs
		return	retValue;
#endif
	//
	// Make sure RF register offset is correct 
	//
	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
	NewOffset = Offset;

	// For 92S LSSI Read RFLSSIRead
	// For RF A/B write 0x824/82c(does not work in the future) 
	// We must use 0x824 for RF A and B to execute read trigger
	tmplong = rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord);
	tmplong2 = rtl8192_QueryBBReg(dev, pPhyReg->rfHSSIPara2, bMaskDWord);
	tmplong2 = (tmplong2 & (~bLSSIReadAddress)) | (NewOffset<<23) | bLSSIReadEdge;	//T65 RF
	
	rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong&(~bLSSIReadEdge));	
	mdelay(1);
	
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, bMaskDWord, tmplong2);	
	mdelay(1);
	
	rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong|bLSSIReadEdge);	
	mdelay(1);

	if(eRFPath == RF90_PATH_A)
		RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT8);
	else if(eRFPath == RF90_PATH_B)
		RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XB_HSSIParameter1, BIT8);
	
	if(RfPiEnable)
	{	// Read from BBreg8b8, 12 bits for 8190, 20bits for T65 RF
		retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBackPi, bLSSIReadBackData);
	}
	else
	{	//Read from BBreg8a0, 12 bits for 8190, 20 bits for T65 RF
		retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	}
	//RTPRINT(FPHY, PHY_RFR, ("RFR-%d Addr[0x%x]=0x%x\n", eRFPath, pPhyReg->rfLSSIReadBack, retValue));
	
	return retValue;	
		
}
4


//
// Function:	phy_RFSerialWrite
//
// OverView:	Write data to RF register (page 8~) 
//
// Input:
//			PADAPTER		Adapter,
//			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
//			u4Byte			Offset,		//The target address to be read			
//			u4Byte			Data			//The new register Data in the target bit position
//										//of the target to be read			
//
// Output:	None
// Return:		None
// Note:		Threre are three types of serial operations: 
//			1. Software serial write
//			2. Hardware LSSI-Low Speed Serial Interface 
//			3. Hardware HSSI-High speed
//			serial write. Driver need to implement (1) and (2).
//			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
//
//  Note: 		  For RF8256 only
// 			 The total count of RTL8256(Zebra4) register is around 36 bit it only employs 
// 			 4-bit RF address. RTL8256 uses "register mode control bit" (Reg00[12], Reg00[10]) 
// 			 to access register address bigger than 0xf. See "Appendix-4 in PHY Configuration
// 			 programming guide" for more details. 
// 			 Thus, we define a sub-finction for RTL8526 register address conversion
// 		       ===========================================================
// 			 Register Mode		RegCTL[1]		RegCTL[0]		Note
//								        (Reg00[12])		(Reg00[10])
// 		       ===========================================================
// 			 Reg_Mode0		0			x			Reg 0 ~15(0x0 ~ 0xf)
// 		       ------------------------------------------------------------------
// 			 Reg_Mode1		1			0			Reg 16 ~30(0x1 ~ 0xf)
// 		       ------------------------------------------------------------------
// 			 Reg_Mode2		1			1			Reg 31 ~ 45(0x1 ~ 0xf)
// 		       ------------------------------------------------------------------
// 
// 	2008/09/02	MH	Add 92S RF definition
// 	
// 
// 
//
static	void
phy_RFSerialWrite(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset,u32	Data)
{
	u32						DataAndAddr = 0;
	struct r8192_priv *priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;
	
#if 0
	//<Roger_TODO> We should check valid regs for RF_6052 case.
	if(pHalData->RFChipID == RF_8225 && Offset > 0x24) //36 valid regs
		return;
	if(pHalData->RFChipID == RF_8256 && Offset > 0x2D) //45 valid regs
		return;
#endif

	Offset &= 0x3f;

	//
	// Shadow Update
	//
	PHY_RFShadowWrite(dev, eRFPath, Offset, Data);	

	//
	// Switch page for 8256 RF IC
	//
		NewOffset = Offset;

	//
	// Put write addr in [5:0]  and write data in [31:16]
	//
	//DataAndAddr = (Data<<16) | (NewOffset&0x3f);
	DataAndAddr = ((NewOffset<<20) | (Data&0x000fffff)) & 0x0fffffff;	// T65 RF

	//
	// Write Operation
	//
	rtl8192_setBBreg(dev, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);
	//RTPRINT(FPHY, PHY_RFW, ("RFW-%d Addr[0x%x]=0x%x\n", eRFPath, pPhyReg->rf3wireOffset, DataAndAddr));

}
#endif
#endif

//
// Function:	phy_CalculateBitShift
//
// OverView:	Get shifted position of the BitMask
//
// Input:
//			u32		BitMask,	
//
// Output:	none
// Return:		u32		Return the shift bit bit position of the mask
///
//use in phy only
static u32 phy_CalculateBitShift(u32 BitMask)
{
	u32 i;

	for(i=0; i<=31; i++)
	{
		if ( ((BitMask>>i) &  0x1 ) == 1)
			break;
	}

	return (i);
}


//
// 3. Initial MAC/BB/RF config by reading MAC/BB/RF txt.
//
// *-----------------------------------------------------------------------------
// * Function:    PHY_MACConfig8192S
// *
// * Overview:	Condig MAC by header file or parameter file.
// *
// * Input:       NONE
// *
// * Output:      NONE
// *
// * Return:      NONE
// *
// * Revised History:
// *  When		Who		Remark
// *  08/12/2008	MHC		Create Version 0.
// *
// *---------------------------------------------------------------------------*/
//adapter_start
extern bool PHY_MACConfig8192S(struct net_device* dev)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;

	//
	// Config MAC
	//
#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigMACWithHeaderFile(dev);
#else
	// Not make sure EEPROM, add later
	RT_TRACE(COMP_INIT, "Read MACREG.txt\n");
	//rtStatus = phy_ConfigMACWithParaFile(dev, RTL819X_PHY_MACREG);// lzm del it temp
#endif
	return (rtStatus == RT_STATUS_SUCCESS) ? true:false;

}

//adapter_start
extern	bool
PHY_BBConfig8192S(struct net_device* dev)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;

	u8 PathMap = 0, index = 0, rf_num = 0;
	struct r8192_priv 	*priv = rtllib_priv(dev);

	phy_InitBBRFRegisterDefinition(dev);

	//
	// Config BB and AGC
	//
	//switch( Adapter->MgntInfo.bRegHwParaFile )
	//{
	//	case 0:
	//		phy_BB8190_Config_HardCode(dev);
	//		break;

	//	case 1:
			rtStatus = phy_BB8192S_Config_ParaFile(dev);
	//		break;

	//	case 2:
			// Partial Modify. 
	//		phy_BB8190_Config_HardCode(dev);
	//		phy_BB8192S_Config_ParaFile(dev);
	//		break;

	//	default:
	//		phy_BB8190_Config_HardCode(dev);
	//		break;
	//}
	PathMap = (u8)(rtl8192_QueryBBReg(dev, rFPGA0_TxInfo, 0xf) |
				rtl8192_QueryBBReg(dev, rOFDM0_TRxPathEnable, 0xf));
	priv->rf_pathmap = PathMap;
	for(index = 0; index<4; index++)
	{
		if((PathMap>>index)&0x1)
			rf_num++;
	}

	if((priv->rf_type==RF_1T1R && rf_num!=1) ||
		(priv->rf_type==RF_1T2R && rf_num!=2) ||
		(priv->rf_type==RF_2T2R && rf_num!=2) ||
		(priv->rf_type==RF_2T2R_GREEN && rf_num!=2) ||
		(priv->rf_type==RF_2T4R && rf_num!=4))
	{
		RT_TRACE( COMP_INIT, "PHY_BBConfig8192S: RF_Type(%x) does not match RF_Num(%x)!!\n", priv->rf_type, rf_num);
	}
	return (rtStatus == RT_STATUS_SUCCESS) ? 1:0;
}

//adapter_start
extern	bool
PHY_RFConfig8192S(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	RT_STATUS    		rtStatus = RT_STATUS_SUCCESS;

	//Set priv->rf_chip = RF_8225 to do real PHY FPGA initilization

	//<Roger_EXP> We assign RF type here temporally. 2008.09.12.
	priv->rf_chip = RF_6052;

	//
	// RF config
	//
	switch(priv->rf_chip)
	{
	case RF_8225:
	case RF_6052:
		rtStatus = PHY_RF6052_Config(dev);
		break;
		
	case RF_8256:			
		//rtStatus = PHY_RF8256_Config(dev);
		break;
		
	case RF_8258:
		break;
		
	case RF_PSEUDO_11N:
		//rtStatus = PHY_RF8225_Config(dev);
		break;
        default:
            break;
	}

	return (rtStatus == RT_STATUS_SUCCESS) ? 1:0;
}


// Joseph test: new initialize order!!
// Test only!! This part need to be re-organized.
// Now it is just for 8256.
//use in phy only
#ifdef TO_DO_LIST
static RT_STATUS
phy_BB8190_Config_HardCode(struct net_device* dev)
{
	//RT_ASSERT(false, ("This function is not implement yet!! \n"));
	return RT_STATUS_SUCCESS;
}
#endif

// *-----------------------------------------------------------------------------
// * Function:    phy_SetBBtoDiffRFWithHeaderFile()
// *
// * Overview:    This function 
// *			
// *
// * Input:      	PADAPTER		Adapter
// *			u1Byte 			ConfigType     0 => PHY_CONFIG
// *
// * Output:      NONE
// *
// * Return:      RT_STATUS_SUCCESS: configuration file exist
// * When			Who		Remark
// * 2008/11/10	tynli
// * use in phy only		
// *---------------------------------------------------------------------------*/
static RT_STATUS
phy_SetBBtoDiffRFWithHeaderFile(struct net_device* dev, u8 ConfigType)
{
	int i;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32* 			Rtl819XPHY_REGArraytoXTXR_Table;
	u16				PHY_REGArraytoXTXRLen;
	
//#if (HAL_CODE_BASE != RTL8192_S)	

	if(priv->rf_type == RF_1T1R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T1R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T1RArrayLength;
	} 
	else if(priv->rf_type == RF_1T2R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T2R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T2RArrayLength;
	}
	//else if(priv->rf_type == RF_2T2R || priv->rf_type == RF_2T2R_GREEN)
	//{
	//	Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to2T2R_Array;
	//	PHY_REGArraytoXTXRLen = PHY_ChangeTo_2T2RArrayLength;
	//}
	else
	{
		return RT_STATUS_FAILURE;
	}

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArraytoXTXRLen;i=i+3)
		{
			if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xf9)
				udelay(1);
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArraytoXTXR_Table[i], Rtl819XPHY_REGArraytoXTXR_Table[i+1], Rtl819XPHY_REGArraytoXTXR_Table[i+2]);		
			//RT_TRACE(COMP_SEND,  
			//"The Rtl819XPHY_REGArraytoXTXR_Table[0] is %lx Rtl819XPHY_REGArraytoXTXR_Table[1] is %lx Rtl819XPHY_REGArraytoXTXR_Table[2] is %lx \n",
			//Rtl819XPHY_REGArraytoXTXR_Table[i],Rtl819XPHY_REGArraytoXTXR_Table[i+1], Rtl819XPHY_REGArraytoXTXR_Table[i+2]);
		}
	}
	else {
		RT_TRACE(COMP_SEND, "phy_SetBBtoDiffRFWithHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
//#endif	// #if (HAL_CODE_BASE != RTL8192_S)		
	return RT_STATUS_SUCCESS;
}


//use in phy only
static	RT_STATUS
phy_BB8192S_Config_ParaFile(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;	
	//u8				u2RegValue;
	//u16				u4RegValue;
	//char				szBBRegFile[] = RTL819X_PHY_REG;
	//char				szBBRegFile1T2R[] = RTL819X_PHY_REG_1T2R;
	//char				szBBRegPgFile[] = RTL819X_PHY_REG_PG;
	//char				szAGCTableFile[] = RTL819X_AGC_TAB;
	//char				szBBRegto1T1RFile[] = RTL819X_PHY_REG_to1T1R;
	//char				szBBRegto1T2RFile[] = RTL819X_PHY_REG_to1T2R;
	
	RT_TRACE(COMP_INIT, "==>phy_BB8192S_Config_ParaFile\n");

	//
	// 1. Read PHY_REG.TXT BB INIT!!
	// We will seperate as 1T1R/1T2R/1T2R_GREEN/2T2R
	//
#if RTL8190_Download_Firmware_From_Header
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_2T2R ||
	    priv->rf_type == RF_1T1R ||priv->rf_type == RF_2T2R_GREEN)
	{
		rtStatus = phy_ConfigBBWithHeaderFile(dev,BaseBand_Config_PHY_REG);
		if(priv->rf_type != RF_2T2R && priv->rf_type != RF_2T2R_GREEN)
		{//2008.11.10 Added by tynli. The default PHY_REG.txt we read is for 2T2R,
		  //so we should reconfig BB reg with the right PHY parameters.
			rtStatus = phy_SetBBtoDiffRFWithHeaderFile(dev,BaseBand_Config_PHY_REG);
		}
	}else
		rtStatus = RT_STATUS_FAILURE;
#else
	RT_TRACE(COMP_INIT, "RF_Type == %d\n", priv->rf_type);		
	// No matter what kind of RF we always read PHY_REG.txt. We must copy different 
	// type of parameter files to phy_reg.txt at first.	
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_2T2R ||
	    priv->rf_type == RF_1T1R ||priv->rf_type == RF_2T2R_GREEN)
	{
		rtStatus = phy_ConfigBBWithParaFile(dev, (char* )&szBBRegFile);
		if(priv->rf_type != RF_2T2R && priv->rf_type != RF_2T2R_GREEN)
		{//2008.11.10 Added by tynli. The default PHY_REG.txt we read is for 2T2R,
		  //so we should reconfig BB reg with the right PHY parameters.
			if(priv->rf_type == RF_1T1R)
				rtStatus = phy_SetBBtoDiffRFWithParaFile(dev, (char* )&szBBRegto1T1RFile);
			else if(priv->rf_type == RF_1T2R)
				rtStatus = phy_SetBBtoDiffRFWithParaFile(dev, (char* )&szBBRegto1T2RFile);
		}

	}else
		rtStatus = RT_STATUS_FAILURE;
#endif

	if(rtStatus != RT_STATUS_SUCCESS){
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():Write BB Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	//
	// 2. If EEPROM or EFUSE autoload OK, We must config by PHY_REG_PG.txt
	//
	if (priv->AutoloadFailFlag == false)
	{
		priv->pwrGroupCnt = 0;

#if	RTL8190_Download_Firmware_From_Header
		rtStatus = phy_ConfigBBWithPgHeaderFile(dev,BaseBand_Config_PHY_REG);
#else
		rtStatus = phy_ConfigBBWithPgParaFile(dev, (char* )&szBBRegPgFile);
#endif
	}
	if(rtStatus != RT_STATUS_SUCCESS){
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():BB_PG Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}
	
	//
	// 3. BB AGC table Initialization
	//
#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigBBWithHeaderFile(dev,BaseBand_Config_AGC_TAB);
#else
	RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile AGC_TAB.txt\n");
	rtStatus = phy_ConfigBBWithParaFile(Adapter, (char* )&szAGCTableFile);
#endif

	if(rtStatus != RT_STATUS_SUCCESS){
		printk( "phy_BB8192S_Config_ParaFile():AGC Table Fail\n");
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	// Check if the CCK HighPower is turned ON.
	// This is used to calculate PWDB.
	priv->bCckHighPower = (bool)(rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, 0x200));


phy_BB8190_Config_ParaFile_Fail:	
	return rtStatus;
}

// *-----------------------------------------------------------------------------
// * Function:    phy_ConfigMACWithHeaderFile()
// *
// * Overview:    This function read BB parameters from Header file we gen, and do register
// *			  Read/Write 
// *
// * Input:      	PADAPTER		Adapter
// *			char* 			pFileName			
// *
// * Output:      NONE
// *
// * Return:      RT_STATUS_SUCCESS: configuration file exist
// *			
// * Note: 		The format of MACPHY_REG.txt is different from PHY and RF. 
// *			[Register][Mask][Value]
// *---------------------------------------------------------------------------*/
//use in phy only
static	RT_STATUS
phy_ConfigMACWithHeaderFile(struct net_device* dev)
{
	u32					i = 0;
	u32					ArrayLength = 0;
	u32*					ptrArray;	
	//struct r8192_priv 	*priv = rtllib_priv(dev);
	
//#if (HAL_CODE_BASE != RTL8192_S)	
//	if(Adapter->bInHctTest)
//	{
//		RT_TRACE(COMP_INIT, DBG_LOUD, ("Rtl819XMACPHY_ArrayDTM\n"));
//		ArrayLength = MACPHY_ArrayLengthDTM;
//		ptrArray = Rtl819XMACPHY_ArrayDTM;
//	}
//	else if(pHalData->bTXPowerDataReadFromEEPORM)
//	{
//		RT_TRACE(COMP_INIT, DBG_LOUD, ("Rtl819XMACPHY_Array_PG\n"));
//		ArrayLength = MACPHY_Array_PGLength;
//		ptrArray = Rtl819XMACPHY_Array_PG;

//	}else
	{ //2008.11.06 Modified by tynli.
		RT_TRACE(COMP_INIT, "Read Rtl819XMACPHY_Array\n");
		ArrayLength = MAC_2T_ArrayLength;
		ptrArray = Rtl819XMAC_Array;	
	}
		
//	for(i = 0 ;i < ArrayLength;i=i+3){
//		RT_TRACE(COMP_SEND, DBG_LOUD, ("The Rtl819XMACPHY_Array[0] is %lx Rtl819XMACPHY_Array[1] is %lx Rtl819XMACPHY_Array[2] is %lx\n",ptrArray[i], ptrArray[i+1], ptrArray[i+2]));
//		if(ptrArray[i] == 0x318)
//		{
//			ptrArray[i+2] = 0x00000800;
//			//DbgPrint("ptrArray[i], ptrArray[i+1], ptrArray[i+2] = %x, %x, %x\n",
//			//	ptrArray[i], ptrArray[i+1], ptrArray[i+2]);
//		}
//		PHY_SetBBReg(Adapter, ptrArray[i], ptrArray[i+1], ptrArray[i+2]);
//	}
	for(i = 0 ;i < ArrayLength;i=i+2){ // Add by tynli for 2 column
		write_nic_byte(dev, ptrArray[i], (u8)ptrArray[i+1]);
	}
//#endif
	return RT_STATUS_SUCCESS;
}


//****************************************
// The following is for High Power PA
//****************************************
void phy_ConfigBBExternalPA(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u32 temp=0;

	if(!priv->ExternalPA)
	{
		return;
	}
	
	rtl8192_setBBreg(dev, 0xee8, BIT28, 1);
	temp = rtl8192_QueryBBReg(dev, 0x860, bMaskDWord);
	temp |= (BIT26|BIT21|BIT10|BIT5);
	rtl8192_setBBreg(dev, 0x860, bMaskDWord, temp);
	rtl8192_setBBreg(dev, 0x870, BIT10, 0);
	rtl8192_setBBreg(dev, 0xc80, bMaskDWord, 0x20000080);
	rtl8192_setBBreg(dev, 0xc88, bMaskDWord, 0x40000100);
}


// *-----------------------------------------------------------------------------
// * Function:    phy_ConfigBBWithHeaderFile()
// *
// * Overview:    This function read BB parameters from general file format, and do register
// *			  Read/Write 
// *
// * Input:      	PADAPTER		Adapter
// *			u8 			ConfigType     0 => PHY_CONFIG
// *										 1 =>AGC_TAB
// *
// * Output:      NONE
// *
// * Return:      RT_STATUS_SUCCESS: configuration file exist
// *			
// *---------------------------------------------------------------------------*/
//use in phy only
static	RT_STATUS
phy_ConfigBBWithHeaderFile(struct net_device* dev,u8 ConfigType)
{
	int 		i;
	//u8 		ArrayLength;	
	u32*	Rtl819XPHY_REGArray_Table;
	u32*	Rtl819XAGCTAB_Array_Table;
	u16		PHY_REGArrayLen, AGCTAB_ArrayLen;
	//struct r8192_priv *priv = rtllib_priv(dev);
//#if (HAL_CODE_BASE != RTL8192_S)	
	/*if(Adapter->bInHctTest)
	{	
	
		AGCTAB_ArrayLen = AGCTAB_ArrayLengthDTM;
		Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_ArrayDTM;

		if(pHalData->RF_Type == RF_2T4R)
		{
			PHY_REGArrayLen = PHY_REGArrayLengthDTM;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REGArrayDTM;
		}
		else if (pHalData->RF_Type == RF_1T2R)
		{
			PHY_REGArrayLen = PHY_REG_1T2RArrayLengthDTM;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_1T2RArrayDTM;
		}		
	
	}
	else
	*/
	//{
	//
	// 2008.11.06 Modified by tynli.
	//
	AGCTAB_ArrayLen = AGCTAB_ArrayLength;
	Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_Array;
	PHY_REGArrayLen = PHY_REG_2T2RArrayLength; // Default RF_type: 2T2R
	Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_Array;
	//}

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArrayLen;i=i+2)
		{
			if (Rtl819XPHY_REGArray_Table[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xf9)
				udelay(1);
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArray_Table[i], bMaskDWord, Rtl819XPHY_REGArray_Table[i+1]);		
			//RT_TRACE(COMP_SEND, "The Rtl819XPHY_REGArray_Table[0] is %lx Rtl819XPHY_REGArray[1] is %lx \n",Rtl819XPHY_REGArray_Table[i], Rtl819XPHY_REGArray_Table[i+1]);
		}
		// for External PA
		phy_ConfigBBExternalPA(dev);
	}
	else if(ConfigType == BaseBand_Config_AGC_TAB){
		for(i=0;i<AGCTAB_ArrayLen;i=i+2)
		{
			rtl8192_setBBreg(dev, Rtl819XAGCTAB_Array_Table[i], bMaskDWord, Rtl819XAGCTAB_Array_Table[i+1]);		
		}
	}
//#endif	// #if (HAL_CODE_BASE != RTL8192_S)		
	return RT_STATUS_SUCCESS;
}


void storePwrIndexDiffRateOffset(
	struct net_device* dev,
	u32		RegAddr,
	u32		BitMask,
	u32		Data
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	if(RegAddr == rTxAGC_Rate18_06)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][0] = Data;
		RT_TRACE(COMP_INIT,"MCSTxPowerLevelOriginalOffset[%d][0] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][0]);
	}
	if(RegAddr == rTxAGC_Rate54_24)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][1] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][1] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][1]);
	}
	if(RegAddr == rTxAGC_CCK_Mcs32)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][6] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][6] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][6]);
	}
	if(RegAddr == rTxAGC_Mcs03_Mcs00)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][2] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][2] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][2]);
	}
	if(RegAddr == rTxAGC_Mcs07_Mcs04)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][3] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][3] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][3]);
	}
	if(RegAddr == rTxAGC_Mcs11_Mcs08)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][4] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][4] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][4]);
	}
	if(RegAddr == rTxAGC_Mcs15_Mcs12)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][5] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][5] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][5]);
		priv->pwrGroupCnt++;
	}
}


// *-----------------------------------------------------------------------------
// * Function:	phy_ConfigBBWithPgHeaderFile
// *
// * Overview:	Config PHY_REG_PG array 
// *
// * Input:       NONE
// *
// * Output:      NONE
// *
// * Return:      NONE
// *
// * Revised History:
// * When			Who		Remark
// * 11/06/2008 	MHC		Add later!!!!!!.. Please modify for new files!!!!
// * 11/10/2008	tynli		Modify to mew files.
// //use in phy only
// *---------------------------------------------------------------------------*/
static RT_STATUS
phy_ConfigBBWithPgHeaderFile(struct net_device* dev,u8 ConfigType)
{
	int i;
	//u8 ArrayLength;	
	u32*	Rtl819XPHY_REGArray_Table_PG;
	u16	PHY_REGArrayPGLen;
	//struct r8192_priv *priv = rtllib_priv(dev);
//#if (HAL_CODE_BASE != RTL8192_S)	
	// Default: pHalData->RF_Type = RF_2T2R.
	
	PHY_REGArrayPGLen = PHY_REG_Array_PGLength;
	Rtl819XPHY_REGArray_Table_PG = Rtl819XPHY_REG_Array_PG;

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArrayPGLen;i=i+3)
		{
			if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xf9)
				udelay(1);
			storePwrIndexDiffRateOffset(dev, Rtl819XPHY_REGArray_Table_PG[i], 
				Rtl819XPHY_REGArray_Table_PG[i+1], 
				Rtl819XPHY_REGArray_Table_PG[i+2]);
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArray_Table_PG[i], Rtl819XPHY_REGArray_Table_PG[i+1], Rtl819XPHY_REGArray_Table_PG[i+2]);		
			//RT_TRACE(COMP_SEND, "The Rtl819XPHY_REGArray_Table_PG[0] is %lx Rtl819XPHY_REGArray_Table_PG[1] is %lx \n",
			//		Rtl819XPHY_REGArray_Table_PG[i], Rtl819XPHY_REGArray_Table_PG[i+1]);
		}
	}else{
		RT_TRACE(COMP_SEND, "phy_ConfigBBWithPgHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
	return RT_STATUS_SUCCESS;
	
}	// phy_ConfigBBWithPgHeaderFile */

//****************************************
// The following is for High Power PA
//****************************************
#define HighPowerRadioAArrayLen 22
//This is for High power PA
u32 Rtl8192S_HighPower_RadioA_Array[HighPowerRadioAArrayLen] = {
0x013,0x00029ea4,
0x013,0x00025e74,
0x013,0x00020ea4,
0x013,0x0001ced0,
0x013,0x00019f40,
0x013,0x00014e70,
0x013,0x000106a0,
0x013,0x0000c670,
0x013,0x000082a0,
0x013,0x00004270,
0x013,0x00000240,
};

u8 PHY_ConfigRFExternalPA(
	struct net_device* dev,
	RF90_RADIO_PATH_E		eRFPath
)
{
	u8	rtStatus = 0;
	struct r8192_priv *priv = rtllib_priv(dev);
	u16 i=0;

	if(!priv->ExternalPA)
	{
		return rtStatus;
	}
	
	//add for SU High Power PA
	for(i = 0;i<HighPowerRadioAArrayLen; i=i+2)
	{
		RT_TRACE(COMP_INIT, "External PA, write RF 0x%x=0x%x\n", Rtl8192S_HighPower_RadioA_Array[i], Rtl8192S_HighPower_RadioA_Array[i+1]);
		rtl8192_phy_SetRFReg(dev, eRFPath, Rtl8192S_HighPower_RadioA_Array[i], bRFRegOffsetMask, Rtl8192S_HighPower_RadioA_Array[i+1]);
	}

	return rtStatus;
}

//****************************************
// The following is for WNC High temprature issue
//****************************************
#define WNCRadioAArrayLen 22

u32 Rtl8192S_WNC_RadioA_Array[WNCRadioAArrayLen] = {
0x013, 0x29EF4, // 63~58
0x013, 0x25EC4, // 57~52
0x013, 0x21E94, // 51~46
0x013, 0x1CEC4, // 45~40
0x013, 0x18ED0, // 39~34, PA=01
0x013, 0x14EA0, // 33~28
0x013, 0x106D0, // 27~22, PA=00
0x013, 0x0C6A0, // 21~16
0x013, 0x082D0, // 15~10
0x013, 0x042A0, // 9~4 
0x013, 0x00280, // 3~1
};

u8 PHY_ConfigRFWNC(
	struct net_device* dev,
	RF90_RADIO_PATH_E		eRFPath
)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	u16 i=0;

	if(priv->CustomerID != RT_CID_819x_WNC_COREGA)
	{
		return rtStatus;
	}

	for(i = 0;i<WNCRadioAArrayLen; i=i+2)
	{
		RT_TRACE(COMP_INIT, "WNC gain, write RF 0x%x=0x%x\n", Rtl8192S_WNC_RadioA_Array[i], Rtl8192S_WNC_RadioA_Array[i+1]);
		rtl8192_phy_SetRFReg(dev, eRFPath, Rtl8192S_WNC_RadioA_Array[i], bRFRegOffsetMask, Rtl8192S_WNC_RadioA_Array[i+1]);
	}

	return rtStatus;
}

//****************************************
// The following is for PA Bias Current for 
// Inferiority IC
//****************************************
u8 PHY_ConfigRFPABiasCurrent(
	struct net_device* dev,
	RF90_RADIO_PATH_E		eRFPath
)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	u32		tmpval=0;

	// If inferiority IC, we have to increase the PA bias current
	if(priv->IC_Class != IC_INFERIORITY_A)
	{
		tmpval = rtl8192_phy_QueryRFReg(dev, eRFPath, RF_IPA, 0xf);
		rtl8192_phy_SetRFReg(dev, eRFPath, RF_IPA, 0xf, tmpval+1);
		RT_TRACE(COMP_INIT,"Inferiority IC, update PA bias current, set RF(0x%x[3:0]) = 0x%x\n", RF_IPA, tmpval);
	}
	
	return rtStatus;
}

// *-----------------------------------------------------------------------------
// * Function:    PHY_ConfigRFWithHeaderFile()
// *
// * Overview:    This function read RF parameters from general file format, and do RF 3-wire
// *
// * Input:      	PADAPTER			Adapter
// *			char* 				pFileName			
// *			RF90_RADIO_PATH_E	eRFPath
// *
// * Output:      NONE
// *
// * Return:      RT_STATUS_SUCCESS: configuration file exist
// *			
// * Note:		Delay may be required for RF configuration
// *---------------------------------------------------------------------------*/
//in 8256 phy_RF8256_Config_ParaFile only
//RT_STATUS PHY_ConfigRFWithHeaderFile(struct net_device* dev,RF90_RADIO_PATH_E eRFPath)
u8 rtl8192_phy_ConfigRFWithHeaderFile(struct net_device* dev, RF90_RADIO_PATH_E	eRFPath)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	int			i;
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	u32			*Rtl819XRadioA_Array_Table;
	u32			*Rtl819XRadioB_Array_Table;
	u16			RadioA_ArrayLen,RadioB_ArrayLen;

	RadioA_ArrayLen = RadioA_1T_ArrayLength;
	Rtl819XRadioA_Array_Table=Rtl819XRadioA_Array;

	//
	// <Roger_Notes> Using Green mode array table for RF_2T2R_GREEN
	// 2008.12.24.
	//
	if( priv->rf_type == RF_2T2R_GREEN )
	{
		Rtl819XRadioB_Array_Table = Rtl819XRadioB_GM_Array;
		RadioB_ArrayLen = RadioB_GM_ArrayLength;
	}
	else
	{		
		Rtl819XRadioB_Array_Table = Rtl819XRadioB_Array;
		RadioB_ArrayLen = RadioB_ArrayLength;	
	}
	
	rtStatus = RT_STATUS_SUCCESS;

	// When initialization, we want the delay function(mdelay(), delay_us() 
	// ==> actually we call PlatformStallExecution()) to do NdisStallExecution()
	// [busy wait] instead of NdisMSleep(). So we acquire RT_INITIAL_SPINLOCK 
	// to run at Dispatch level to achive it.	
	//cosa PlatformAcquireSpinLock(Adapter, RT_INITIAL_SPINLOCK);
	
	switch(eRFPath){
		case RF90_PATH_A:
			for(i = 0;i<RadioA_ArrayLen; i=i+2)
			{
				if(Rtl819XRadioA_Array_Table[i] == 0xfe)
					mdelay(50);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfd)
					mdelay(5);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfc)
					mdelay(1);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfb)
					udelay(50);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfa)
					udelay(5);
				else if (Rtl819XRadioA_Array_Table[i] == 0xf9)
					udelay(1);
				else
				{
					rtl8192_phy_SetRFReg(dev, eRFPath, Rtl819XRadioA_Array_Table[i], bRFRegOffsetMask, Rtl819XRadioA_Array_Table[i+1]);
				}
			}			
			//Add for WNC High temprature issue
			PHY_ConfigRFWNC(dev, eRFPath);
			//Add for High Power PA
			PHY_ConfigRFExternalPA(dev, eRFPath);
			//PA Bias current for inferiority IC
			PHY_ConfigRFPABiasCurrent(dev, eRFPath);
			break;
		case RF90_PATH_B:
			for(i = 0;i<RadioB_ArrayLen; i=i+2){
				if(Rtl819XRadioB_Array_Table[i] == 0xfe)
				{ // Deay specific ms. Only RF configuration require delay.												
#ifdef RTL8192SU
					mdelay(1000);
#else
					mdelay(50);
#endif
				}
				else if (Rtl819XRadioB_Array_Table[i] == 0xfd)
					mdelay(5);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfc)
					mdelay(1);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfb)
					udelay(50);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfa)
					udelay(5);
				else if (Rtl819XRadioB_Array_Table[i] == 0xf9)
					udelay(1);
				else
				{
					rtl8192_phy_SetRFReg(dev, eRFPath, Rtl819XRadioB_Array_Table[i], bRFRegOffsetMask, Rtl819XRadioB_Array_Table[i+1]);
				}
			}			
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		default:
			break;
	}

	return rtStatus;

}

//-----------------------------------------------------------------------------
// Function:    PHY_CheckBBAndRFOK()
//
// Overview:    This function is write register and then readback to make sure whether
//			  BB[PHY0, PHY1], RF[Patha, path b, path c, path d] is Ok
//
// Input:      	PADAPTER			Adapter
//			HW90_BLOCK_E		CheckBlock
//			RF90_RADIO_PATH_E	eRFPath		// it is used only when CheckBlock is HW90_BLOCK_RF
//
// Output:      NONE
//
// Return:      RT_STATUS_SUCCESS: PHY is OK
//			
// Note:		This function may be removed in the ASIC
//---------------------------------------------------------------------------*/
//in 8256 phy_RF8256_Config_HardCode
//but we don't use it temp
RT_STATUS
PHY_CheckBBAndRFOK(
	struct net_device* dev,
	HW90_BLOCK_E		CheckBlock,
	RF90_RADIO_PATH_E	eRFPath
	)
{
	//struct r8192_priv *priv = rtllib_priv(dev);
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;
	u32				i, CheckTimes = 4,ulRegRead = 0;
	u32				WriteAddr[4];
	u32				WriteData[] = {0xfffff027, 0xaa55a02f, 0x00000027, 0x55aa502f};

	// Initialize register address offset to be checked
	WriteAddr[HW90_BLOCK_MAC] = 0x100;
	WriteAddr[HW90_BLOCK_PHY0] = 0x900;
	WriteAddr[HW90_BLOCK_PHY1] = 0x800;
	WriteAddr[HW90_BLOCK_RF] = 0x3;
	
	for(i=0 ; i < CheckTimes ; i++)
	{

		//
		// Write Data to register and readback
		//
		switch(CheckBlock)
		{
		case HW90_BLOCK_MAC:
			//RT_ASSERT(false, ("PHY_CheckBBRFOK(): Never Write 0x100 here!"));
			RT_TRACE(COMP_INIT, "PHY_CheckBBRFOK(): Never Write 0x100 here!\n");
			break;
			
		case HW90_BLOCK_PHY0:
		case HW90_BLOCK_PHY1:
			write_nic_dword(dev, WriteAddr[CheckBlock], WriteData[i]);
			ulRegRead = read_nic_dword(dev, WriteAddr[CheckBlock]);
			break;

		case HW90_BLOCK_RF:
			// When initialization, we want the delay function(mdelay(), delay_us() 
			// ==> actually we call PlatformStallExecution()) to do NdisStallExecution()
			// [busy wait] instead of NdisMSleep(). So we acquire RT_INITIAL_SPINLOCK 
			// to run at Dispatch level to achive it.	
			//cosa PlatformAcquireSpinLock(dev, RT_INITIAL_SPINLOCK);
			WriteData[i] &= 0xfff;
			rtl8192_phy_SetRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bRFRegOffsetMask, WriteData[i]);
			// TODO: we should not delay for such a long time. Ask SD3
			mdelay(10);
			ulRegRead = rtl8192_phy_QueryRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bMaskDWord);				
			mdelay(10);
			//cosa PlatformReleaseSpinLock(dev, RT_INITIAL_SPINLOCK);
			break;
			
		default:
			rtStatus = RT_STATUS_FAILURE;
			break;
		}


		//
		// Check whether readback data is correct
		//
		if(ulRegRead != WriteData[i])
		{
			//RT_TRACE(COMP_FPGA,  ("ulRegRead: %x, WriteData: %x \n", ulRegRead, WriteData[i]));
			RT_TRACE(COMP_ERR, "read back error(read:%x, write:%x)\n", ulRegRead, WriteData[i]);
			rtStatus = RT_STATUS_FAILURE;			
			break;
		}
	}

	return rtStatus;
}

//no use temp in windows driver
#ifdef TO_DO_LIST
void
PHY_SetRFPowerState8192SUsb(
	struct net_device* dev,
	RF_POWER_STATE	RFPowerState
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	bool			WaitShutDown = false;
	u32			DWordContent;
	//RF90_RADIO_PATH_E	eRFPath;
	u8				eRFPath;
	BB_REGISTER_DEFINITION_T	*pPhyReg;
	
	if(priv->SetRFPowerStateInProgress == true)
		return;
	
	priv->SetRFPowerStateInProgress = true;
	
	// TODO: Emily, 2006.11.21, we should rewrite this function

	if(RFPowerState==RF_SHUT_DOWN)
	{
		RFPowerState=RF_OFF;
		WaitShutDown=true;
	}
	
	
	priv->RFPowerState = RFPowerState;
	switch( priv->rf_chip )
	{
	case RF_8225:
	case RF_6052:
		switch( RFPowerState )
		{
		case RF_ON:
			break;
	
		case RF_SLEEP:
			break;
	
		case RF_OFF:
			break;
		}
		break;

	case RF_8256:
		switch( RFPowerState )
		{
		case RF_ON:
			break;
	
		case RF_SLEEP:
			break;
	
		case RF_OFF:
			for(eRFPath=(RF90_RADIO_PATH_E)RF90_PATH_A; eRFPath < RF90_PATH_MAX; eRFPath++)
			{
				if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))		
					continue;	
				
				pPhyReg = &priv->PHYRegDef[eRFPath];
				rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV, bRFSI_RFENV);
				rtl8192_setBBreg(dev, pPhyReg->rfintfo, bRFSI_RFENV, 0);
			}
			break;
		}
		break;
		
	case RF_8258:
		break;
	}// switch( priv->rf_chip )

	priv->SetRFPowerStateInProgress = false;
}
#endif

#ifdef RTL8192U
//no use temp in windows driver
void
PHY_UpdateInitialGain(
	struct net_device* dev
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	//unsigned char			*IGTable;
	//u8			DIG_CurrentInitialGain = 4;
	
	switch(priv->rf_chip)
	{
	case RF_8225:
		break;
	case RF_8256:
		break;
	case RF_8258:
		break;
	case RF_PSEUDO_11N:
		break;
	case RF_6052:
		break;
	default:
		RT_TRACE(COMP_DBG, "PHY_UpdateInitialGain(): unknown rf_chip: %#X\n", priv->rf_chip);
		break;
	}
}
#endif

//YJ,modified,090107
void PHY_GetHWRegOriginalValue(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	// read rx initial gain
	priv->DefaultInitialGain[0] = rtl8192_QueryBBReg(dev, rOFDM0_XAAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[1] = rtl8192_QueryBBReg(dev, rOFDM0_XBAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[2] = rtl8192_QueryBBReg(dev, rOFDM0_XCAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[3] = rtl8192_QueryBBReg(dev, rOFDM0_XDAGCCore1, bMaskByte0);
	RT_TRACE(COMP_INIT, "Default initial gain (c50=0x%x, c58=0x%x, c60=0x%x, c68=0x%x) \n", 
			priv->DefaultInitialGain[0], priv->DefaultInitialGain[1], 
			priv->DefaultInitialGain[2], priv->DefaultInitialGain[3]);

	// read framesync
	priv->framesync = rtl8192_QueryBBReg(dev, rOFDM0_RxDetector3, bMaskByte0);
	priv->framesyncC34 = rtl8192_QueryBBReg(dev, rOFDM0_RxDetector2, bMaskDWord);
	RT_TRACE(COMP_INIT, "Default framesync (0x%x) = 0x%x \n", 
				rOFDM0_RxDetector3, priv->framesync);
}
//YJ,modified,090107,end



//
//Function:	phy_InitBBRFRegisterDefinition
//
//OverView:	Initialize Register definition offset for Radio Path A/B/C/D
//
//Input:
//      		PADAPTER		Adapter,
//
//Output:	None
//Return:		None
//Note:		The initialization value is constant and it should never be changes
//
//use in phy only
static void phy_InitBBRFRegisterDefinition(	struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	// RF Interface Sowrtware Control
	priv->PHYRegDef[RF90_PATH_A].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 LSBs if read 32-bit from 0x870
	priv->PHYRegDef[RF90_PATH_B].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 MSBs if read 32-bit from 0x870 (16-bit for 0x872)
	priv->PHYRegDef[RF90_PATH_C].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 LSBs if read 32-bit from 0x874
	priv->PHYRegDef[RF90_PATH_D].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 MSBs if read 32-bit from 0x874 (16-bit for 0x876)

	// RF Interface Readback Value
	priv->PHYRegDef[RF90_PATH_A].rfintfi = rFPGA0_XAB_RFInterfaceRB; // 16 LSBs if read 32-bit from 0x8E0	
	priv->PHYRegDef[RF90_PATH_B].rfintfi = rFPGA0_XAB_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E0 (16-bit for 0x8E2)
	priv->PHYRegDef[RF90_PATH_C].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 LSBs if read 32-bit from 0x8E4
	priv->PHYRegDef[RF90_PATH_D].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E4 (16-bit for 0x8E6)

	// RF Interface Output (and Enable)
	priv->PHYRegDef[RF90_PATH_A].rfintfo = rFPGA0_XA_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x860
	priv->PHYRegDef[RF90_PATH_B].rfintfo = rFPGA0_XB_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x864
	priv->PHYRegDef[RF90_PATH_C].rfintfo = rFPGA0_XC_RFInterfaceOE;// 16 LSBs if read 32-bit from 0x868
	priv->PHYRegDef[RF90_PATH_D].rfintfo = rFPGA0_XD_RFInterfaceOE;// 16 LSBs if read 32-bit from 0x86C

	// RF Interface (Output and)  Enable
	priv->PHYRegDef[RF90_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x860 (16-bit for 0x862)
	priv->PHYRegDef[RF90_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x864 (16-bit for 0x866)
	priv->PHYRegDef[RF90_PATH_C].rfintfe = rFPGA0_XC_RFInterfaceOE;// 16 MSBs if read 32-bit from 0x86A (16-bit for 0x86A)
	priv->PHYRegDef[RF90_PATH_D].rfintfe = rFPGA0_XD_RFInterfaceOE;// 16 MSBs if read 32-bit from 0x86C (16-bit for 0x86E)

	//Addr of LSSI. Wirte RF register by driver
	priv->PHYRegDef[RF90_PATH_A].rf3wireOffset = rFPGA0_XA_LSSIParameter; //LSSI Parameter
	priv->PHYRegDef[RF90_PATH_B].rf3wireOffset = rFPGA0_XB_LSSIParameter;
	priv->PHYRegDef[RF90_PATH_C].rf3wireOffset = rFPGA0_XC_LSSIParameter;
	priv->PHYRegDef[RF90_PATH_D].rf3wireOffset = rFPGA0_XD_LSSIParameter;

	// RF parameter
	priv->PHYRegDef[RF90_PATH_A].rfLSSI_Select = rFPGA0_XAB_RFParameter;  //BB Band Select
	priv->PHYRegDef[RF90_PATH_B].rfLSSI_Select = rFPGA0_XAB_RFParameter;
	priv->PHYRegDef[RF90_PATH_C].rfLSSI_Select = rFPGA0_XCD_RFParameter;
	priv->PHYRegDef[RF90_PATH_D].rfLSSI_Select = rFPGA0_XCD_RFParameter;

	// Tx AGC Gain Stage (same for all path. Should we remove this?)
	priv->PHYRegDef[RF90_PATH_A].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	priv->PHYRegDef[RF90_PATH_B].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	priv->PHYRegDef[RF90_PATH_C].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	priv->PHYRegDef[RF90_PATH_D].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage

	// Tranceiver A~D HSSI Parameter-1
	priv->PHYRegDef[RF90_PATH_A].rfHSSIPara1 = rFPGA0_XA_HSSIParameter1;  //wire control parameter1
	priv->PHYRegDef[RF90_PATH_B].rfHSSIPara1 = rFPGA0_XB_HSSIParameter1;  //wire control parameter1
	priv->PHYRegDef[RF90_PATH_C].rfHSSIPara1 = rFPGA0_XC_HSSIParameter1;  //wire control parameter1
	priv->PHYRegDef[RF90_PATH_D].rfHSSIPara1 = rFPGA0_XD_HSSIParameter1;  //wire control parameter1

	// Tranceiver A~D HSSI Parameter-2
	priv->PHYRegDef[RF90_PATH_A].rfHSSIPara2 = rFPGA0_XA_HSSIParameter2;  //wire control parameter2
	priv->PHYRegDef[RF90_PATH_B].rfHSSIPara2 = rFPGA0_XB_HSSIParameter2;  //wire control parameter2
	priv->PHYRegDef[RF90_PATH_C].rfHSSIPara2 = rFPGA0_XC_HSSIParameter2;  //wire control parameter2
	priv->PHYRegDef[RF90_PATH_D].rfHSSIPara2 = rFPGA0_XD_HSSIParameter2;  //wire control parameter1

	// RF switch Control
	priv->PHYRegDef[RF90_PATH_A].rfSwitchControl = rFPGA0_XAB_SwitchControl; //TR/Ant switch control
	priv->PHYRegDef[RF90_PATH_B].rfSwitchControl = rFPGA0_XAB_SwitchControl;
	priv->PHYRegDef[RF90_PATH_C].rfSwitchControl = rFPGA0_XCD_SwitchControl;
	priv->PHYRegDef[RF90_PATH_D].rfSwitchControl = rFPGA0_XCD_SwitchControl;

	// AGC control 1 
	priv->PHYRegDef[RF90_PATH_A].rfAGCControl1 = rOFDM0_XAAGCCore1;
	priv->PHYRegDef[RF90_PATH_B].rfAGCControl1 = rOFDM0_XBAGCCore1;
	priv->PHYRegDef[RF90_PATH_C].rfAGCControl1 = rOFDM0_XCAGCCore1;
	priv->PHYRegDef[RF90_PATH_D].rfAGCControl1 = rOFDM0_XDAGCCore1;

	// AGC control 2 
	priv->PHYRegDef[RF90_PATH_A].rfAGCControl2 = rOFDM0_XAAGCCore2;
	priv->PHYRegDef[RF90_PATH_B].rfAGCControl2 = rOFDM0_XBAGCCore2;
	priv->PHYRegDef[RF90_PATH_C].rfAGCControl2 = rOFDM0_XCAGCCore2;
	priv->PHYRegDef[RF90_PATH_D].rfAGCControl2 = rOFDM0_XDAGCCore2;

	// RX AFE control 1 
	priv->PHYRegDef[RF90_PATH_A].rfRxIQImbalance = rOFDM0_XARxIQImbalance;
	priv->PHYRegDef[RF90_PATH_B].rfRxIQImbalance = rOFDM0_XBRxIQImbalance;
	priv->PHYRegDef[RF90_PATH_C].rfRxIQImbalance = rOFDM0_XCRxIQImbalance;
	priv->PHYRegDef[RF90_PATH_D].rfRxIQImbalance = rOFDM0_XDRxIQImbalance;	

	// RX AFE control 1  
	priv->PHYRegDef[RF90_PATH_A].rfRxAFE = rOFDM0_XARxAFE;
	priv->PHYRegDef[RF90_PATH_B].rfRxAFE = rOFDM0_XBRxAFE;
	priv->PHYRegDef[RF90_PATH_C].rfRxAFE = rOFDM0_XCRxAFE;
	priv->PHYRegDef[RF90_PATH_D].rfRxAFE = rOFDM0_XDRxAFE;	

	// Tx AFE control 1 
	priv->PHYRegDef[RF90_PATH_A].rfTxIQImbalance = rOFDM0_XATxIQImbalance;
	priv->PHYRegDef[RF90_PATH_B].rfTxIQImbalance = rOFDM0_XBTxIQImbalance;
	priv->PHYRegDef[RF90_PATH_C].rfTxIQImbalance = rOFDM0_XCTxIQImbalance;
	priv->PHYRegDef[RF90_PATH_D].rfTxIQImbalance = rOFDM0_XDTxIQImbalance;	

	// Tx AFE control 2 
	priv->PHYRegDef[RF90_PATH_A].rfTxAFE = rOFDM0_XATxAFE;
	priv->PHYRegDef[RF90_PATH_B].rfTxAFE = rOFDM0_XBTxAFE;
	priv->PHYRegDef[RF90_PATH_C].rfTxAFE = rOFDM0_XCTxAFE;
	priv->PHYRegDef[RF90_PATH_D].rfTxAFE = rOFDM0_XDTxAFE;	

	// Tranceiver LSSI Readback  SI mode 
	priv->PHYRegDef[RF90_PATH_A].rfLSSIReadBack = rFPGA0_XA_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_B].rfLSSIReadBack = rFPGA0_XB_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_C].rfLSSIReadBack = rFPGA0_XC_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_D].rfLSSIReadBack = rFPGA0_XD_LSSIReadBack;	

	// Tranceiver LSSI Readback PI mode 
	priv->PHYRegDef[RF90_PATH_A].rfLSSIReadBackPi = TransceiverA_HSPI_Readback;
	priv->PHYRegDef[RF90_PATH_B].rfLSSIReadBackPi = TransceiverB_HSPI_Readback;
	//pHalData->PHYRegDef[RF90_PATH_C].rfLSSIReadBackPi = rFPGA0_XC_LSSIReadBack;
	//pHalData->PHYRegDef[RF90_PATH_D].rfLSSIReadBackPi = rFPGA0_XD_LSSIReadBack;	

}


//
//	Description:  Change RF power state.
//
//	Assumption: This function must be executed in re-schdulable context, 
//		ie. PASSIVE_LEVEL.
//
//	050823, by rcnjko.
//not understand it seem's use in init
//SetHwReg8192SUsb--->HalFunc.SetHwRegHandler
bool PHY_SetRFPowerState(struct net_device* dev, RT_RF_POWER_STATE eRFPowerState)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	bool			bResult = false;

	RT_TRACE(COMP_RF, "---------> PHY_SetRFPowerState(): eRFPowerState(%d)\n", eRFPowerState);

	if(eRFPowerState == priv->rtllib->eRFPowerState)
	{
		RT_TRACE(COMP_RF, "<--------- PHY_SetRFPowerState(): discard the request for eRFPowerState(%d) is the same.\n", eRFPowerState);
		return bResult;
	}

	bResult = phy_SetRFPowerState8192SU(dev, eRFPowerState);
	
	RT_TRACE(COMP_RF, "<--------- PHY_SetRFPowerState(): bResult(%d)\n", bResult);

	return bResult;
}

//use in phy only
static bool phy_SetRFPowerState8192SU(struct net_device* dev,RT_RF_POWER_STATE eRFPowerState)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	bool			bResult = true;
	//u8		eRFPath;
	//u8		i, QueueID;
	u8 		u1bTmp;
	
	if(priv->SetRFPowerStateInProgress == true)
		return false;
	
	priv->SetRFPowerStateInProgress = true;

	switch(priv->rf_chip )
	{
		default:
		switch( eRFPowerState )
		{
			case eRfOn:
				write_nic_dword(dev, WFM5, FW_BB_RESET_ENABLE);
				write_nic_word(dev, CMDR, 0x37FC);
				write_nic_byte(dev, PHY_CCA, 0x3);
				write_nic_byte(dev, TXPAUSE, 0x00);
				write_nic_byte(dev, SPS1_CTRL, 0x64);
				break;
    	    
			// 
			// In current solution, RFSleep=RFOff in order to save power under 802.11 power save.
			// By Bruce, 2008-01-16.
			//
			case eRfSleep:
			case eRfOff:  
			  	if (priv->rtllib->eRFPowerState == eRfSleep || priv->rtllib->eRFPowerState == eRfOff)
						break;
				
#ifdef TO_DO_LIST
				// Make sure BusyQueue is empty befor turn off RFE pwoer.
				for(QueueID = 0, i = 0; QueueID < MAX_TX_QUEUE; )
				{
						ring = &priv->tx_ring[QueueID];
						if(skb_queue_len(&ring->queue) == 0)
					{
						QueueID++;
						continue;
					}
					else
					{
						RT_TRACE(COMP_POWER, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 before doze!\n", (i+1), QueueID);
						udelay(10);
						i++;
					}
					
					if(i >= MAX_DOZE_WAITING_TIMES_9x)
					{
							RT_TRACE(COMP_POWER, "\n\n\n %s(): eRfOff: %d times TcbBusyQueue[%d] != 0 !!!\n\n\n", __FUNCTION__,MAX_DOZE_WAITING_TIMES_9x, QueueID);
						break;
					}
				}			
#endif
				// 
				//RF Off/Sleep sequence. Designed/tested from SD4 Scott, SD1 Grent and Jonbon.
				// Added by Bruce, 2008-11-22.
				//
				//==================================================================
				// (0) Disable FW BB reset checking
				write_nic_dword(dev, WFM5, FW_BB_RESET_DISABLE);				

				// (1) Switching Power Supply Register : Disable LD12 & SW12 (for IT)
				u1bTmp = read_nic_byte(dev, LDOV12D_CTRL);
				u1bTmp |= BIT0;
				write_nic_byte(dev, LDOV12D_CTRL, u1bTmp);

				write_nic_byte(dev, SPS1_CTRL, 0x0);
				write_nic_byte(dev, TXPAUSE, 0xFF);

				// (2) MAC Tx/Rx enable, BB enable, CCK/OFDM enable
				write_nic_word(dev, CMDR, 0x77FC);
				write_nic_byte(dev, PHY_CCA, 0x0);
				udelay(100);

				write_nic_word(dev, CMDR, 0x37FC);
				udelay(10);

				write_nic_word(dev, CMDR, 0x77FC);
				udelay(10);

				// (3) Reset BB TRX blocks
				write_nic_word(dev, CMDR, 0x57FC);
				break;

			default:
				bResult = false;
				//RT_ASSERT(false, ("phy_SetRFPowerState8192SU(): unknow state to set: 0x%X!!!\n", eRFPowerState));
				break;
		} 
		break;
		
	}
	//priv->rtllib->eRFPowerState = eRFPowerState;
		
	if(bResult)
	{
		// Update current RF state variable.
		priv->rtllib->eRFPowerState = eRFPowerState;
		
		switch(priv->rf_chip )
		{
			case RF_8256:		
			switch(priv->rtllib->eRFPowerState)
			{
				case eRfOff:
					//
					//If Rf off reason is from IPS, Led should blink with no link, by Maddest 071015
					//
					if(priv->rtllib->RfOffReason==RF_CHANGE_BY_IPS )
					{
						priv->rtllib->LedControlHandler(dev,LED_CTL_NO_LINK); 
					}
					else
					{
						// Turn off LED if RF is not ON.
						priv->rtllib->LedControlHandler(dev, LED_CTL_POWER_OFF); 
					}
					break;
        		
				case eRfOn:
					// Turn on RF we are still linked, which might happen when 
					// we quickly turn off and on HW RF. 2006.05.12, by rcnjko.
					if(priv->rtllib->state == RTLLIB_LINKED)
					{
						priv->rtllib->LedControlHandler(dev, LED_CTL_LINK); 
					}
					else
					{
						// Turn off LED if RF is not ON.
						priv->rtllib->LedControlHandler(dev, LED_CTL_NO_LINK); 
					}
					break;
        		
				default:
					// do nothing.
					break;
			}// Switch RF state

				break;
		
			default:
				RT_TRACE(COMP_RF, "phy_SetRFPowerState8192SU(): Unknown RF type\n");
				break;
		}// Switch rf_chip
	}

	priv->SetRFPowerStateInProgress = false;

	return bResult;
}

//-----------------------------------------------------------------------------
// Function:    GetTxPowerLevel8190()
//
// Overview:    This function is export to "common" moudule
//
// Input:       PADAPTER		Adapter
//			psByte			Power Level
//
// Output:      NONE
//
// Return:      NONE
//
//---------------------------------------------------------------------------*/
 // no use temp
 void
PHY_GetTxPowerLevel8192S(
	struct net_device* dev,
	 long*    		powerlevel
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8			TxPwrLevel = 0;
	long			TxPwrDbm;
	//
	// Because the Tx power indexes are different, we report the maximum of them to 
	// meet the CCX TPC request. By Bruce, 2008-01-31.
	//

	// CCK
	TxPwrLevel = priv->CurrentCckTxPwrIdx;
	TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_B, TxPwrLevel);

	// Legacy OFDM
	TxPwrLevel = priv->CurrentOfdm24GTxPwrIdx + priv->LegacyHTTxPowerDiff;

	// Compare with Legacy OFDM Tx power.
	if(phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_G, TxPwrLevel);

	// HT OFDM
	TxPwrLevel = priv->CurrentOfdm24GTxPwrIdx;
	
	// Compare with HT OFDM Tx power.
	if(phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_N_24G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_N_24G, TxPwrLevel);

	*powerlevel = TxPwrDbm;
}


void getTxPowerIndex(
	struct net_device* dev,
	u8			channel,
	u8*		cckPowerLevel,
	u8*		ofdmPowerLevel
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	index = (channel -1);

	// 1. CCK
	cckPowerLevel[0] = priv->RfTxPwrLevelCck[0][index];	//RF-A
	cckPowerLevel[1] = priv->RfTxPwrLevelCck[1][index];	//RF-B
		
	// 2. OFDM for 1T or 2T
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_1T1R)
	{
		// Read HT 40 OFDM TX power
		ofdmPowerLevel[0] = priv->RfTxPwrLevelOfdm1T[0][index];
		ofdmPowerLevel[1] = priv->RfTxPwrLevelOfdm1T[1][index];
	}
	else if (priv->rf_type == RF_2T2R)
	{
		// Read HT 40 OFDM TX power
		ofdmPowerLevel[0] = priv->RfTxPwrLevelOfdm2T[0][index];
		ofdmPowerLevel[1] = priv->RfTxPwrLevelOfdm2T[1][index];
	}
	//RTPRINT(FPHY, PHY_TXPWR, ("Channel-%d, set tx power index !!\n", channel));
}

void ccxPowerIndexCheck(
	struct net_device* dev,
	u8			channel,
	u8*		cckPowerLevel,
	u8*		ofdmPowerLevel
	)
			{
#if 0//cosa
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	PRT_CCX_INFO		pCcxInfo = GET_CCX_INFO(pMgntInfo);
		
	//
	// CCX 2 S31, AP control of client transmit power:
	// 1. We shall not exceed Cell Power Limit as possible as we can.
	// 2. Tolerance is +/- 5dB.
	// 3. 802.11h Power Contraint takes higher precedence over CCX Cell Power Limit.
	// 
	// TODO: 
	// 1. 802.11h power contraint 
	//
	// 071011, by rcnjko.
	//
	if(	pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE && 
		pMgntInfo->mAssoc &&
		pCcxInfo->bUpdateCcxPwr &&
		pCcxInfo->bWithCcxCellPwr &&
		channel == pMgntInfo->dot11CurrentChannelNumber)
	{
		u1Byte	CckCellPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_B, pCcxInfo->CcxCellPwr);
		u1Byte	LegacyOfdmCellPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_G, pCcxInfo->CcxCellPwr);
		u1Byte	OfdmCellPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_N_24G, pCcxInfo->CcxCellPwr);

		RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("CCX Cell Limit: %d dbm => CCK Tx power index : %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n", 
		pCcxInfo->CcxCellPwr, CckCellPwrIdx, LegacyOfdmCellPwrIdx, OfdmCellPwrIdx));
		RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("EEPROM channel(%d) => CCK Tx power index: %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n",
		channel, cckPowerLevel[0], ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff, ofdmPowerLevel[0])); 

		// CCK
		if(cckPowerLevel[0] > CckCellPwrIdx)
			cckPowerLevel[0] = CckCellPwrIdx;
		// Legacy OFDM, HT OFDM
		if(ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff > LegacyOfdmCellPwrIdx)
		{
			if((OfdmCellPwrIdx - pHalData->LegacyHTTxPowerDiff) > 0)
			{
				ofdmPowerLevel[0] = OfdmCellPwrIdx - pHalData->LegacyHTTxPowerDiff;
			}
			else
			{
				ofdmPowerLevel[0] = 0;
			}
		}

		RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("Altered CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		cckPowerLevel[0], ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff, ofdmPowerLevel[0]));
	}

	pHalData->CurrentCckTxPwrIdx = cckPowerLevel[0];
	pHalData->CurrentOfdm24GTxPwrIdx = ofdmPowerLevel[0];

	RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("PHY_SetTxPowerLevel8192S(): CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		cckPowerLevel[0], ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff, ofdmPowerLevel[0]));
#endif
}


//-----------------------------------------------------------------------------
// Function:    SetTxPowerLevel8190()
//
// Overview:    This function is export to "HalCommon" moudule
//			We must consider RF path later!!!!!!!
//
// Input:       PADAPTER		Adapter
//			u1Byte		channel
//
// Output:      NONE
//
// Return:      NONE
//	2008/11/04	MHC		We remove EEPROM_93C56.
//						We need to move CCX relative code to independet file.
//      2009/01/21	MHC		Support new EEPROM format from SD3 requirement.
//---------------------------------------------------------------------------*/
 void PHY_SetTxPowerLevel8192S(struct net_device* dev, u8	channel)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	cckPowerLevel[2], ofdmPowerLevel[2];	// [0]:RF-A, [1]:RF-B

	if(priv->bTXPowerDataReadFromEEPORM == false)
		return;

	//
	// Mainly we use RF-A Tx Power to write the Tx Power registers, but the RF-B Tx
	// Power must be calculated by the antenna diff.
	// So we have to rewrite Antenna gain offset register here.		
	// Please refer to BB register 0x80c
	// 1. For CCK.
	// 2. For OFDM 1T or 2T
	//
	getTxPowerIndex(dev, channel, &cckPowerLevel[0], &ofdmPowerLevel[0]);
	//printk("Channel-%d, cckPowerLevel (A / B) = 0x%x / 0x%x,   ofdmPowerLevel (A / B) = 0x%x / 0x%x\n", 
	//	channel, cckPowerLevel[0], cckPowerLevel[1], ofdmPowerLevel[0], ofdmPowerLevel[1]);

	ccxPowerIndexCheck(dev, channel, &cckPowerLevel[0], &ofdmPowerLevel[0]);
	
	switch(priv->rf_chip)
	{
		case RF_8225:
			//PHY_SetRF8225CckTxPower(dev, powerlevel);
			//PHY_SetRF8225OfdmTxPower(dev, powerlevelOFDM24G);
		break;

		case RF_8256:
#if 0
			PHY_SetRF8256CCKTxPower(dev, powerlevel);
			PHY_SetRF8256OFDMTxPower(dev, powerlevelOFDM24G);
#endif
			break;

		case RF_6052:
			PHY_RF6052SetCckTxPower(dev, cckPowerLevel[0]);
			PHY_RF6052SetOFDMTxPower(dev, &ofdmPowerLevel[0], channel);
			break;

		case RF_8258:
			break;
		default:
			break;
	}	

}

//
//	Description:
//		Update transmit power level of all channel supported.
//
//	TODO: 
//		A mode.
//	By Bruce, 2008-02-04.
//    no use temp
bool PHY_UpdateTxPowerDbm8192S(struct net_device* dev, long powerInDbm)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8				idx;
	u8				rf_path;

	// TODO: A mode Tx power.
	u8	CckTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, powerInDbm);
	u8	OfdmTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, powerInDbm);

	if(OfdmTxPwrIdx - priv->LegacyHTTxPowerDiff > 0)
		OfdmTxPwrIdx -= priv->LegacyHTTxPowerDiff;
	else
		OfdmTxPwrIdx = 0;

	for(idx = 0; idx < 14; idx++)
	{
		priv->TxPowerLevelCCK[idx] = CckTxPwrIdx;
		priv->TxPowerLevelCCK_A[idx] = CckTxPwrIdx;
		priv->TxPowerLevelCCK_C[idx] = CckTxPwrIdx;
		priv->TxPowerLevelOFDM24G[idx] = OfdmTxPwrIdx;
		priv->TxPowerLevelOFDM24G_A[idx] = OfdmTxPwrIdx;
		priv->TxPowerLevelOFDM24G_C[idx] = OfdmTxPwrIdx;
		
		for (rf_path = 0; rf_path < 2; rf_path++)
		{
			priv->RfTxPwrLevelCck[rf_path][idx] = CckTxPwrIdx;
			priv->RfTxPwrLevelOfdm1T[rf_path][idx] =  \
			priv->RfTxPwrLevelOfdm2T[rf_path][idx] = OfdmTxPwrIdx;
		}
	}

	PHY_SetTxPowerLevel8192S(dev, priv->chan);

	return true;	
}

//
//      Description:
//      	When beacon interval is changed, the values of the 
//      	hw registers should be modified.
//      By tynli, 2008.10.24.

//

extern void PHY_SetBeaconHwReg(	struct net_device* dev, u16 BeaconInterval)
{
	u32 NewBeaconNum;	

	NewBeaconNum = BeaconInterval *32 - 64;
	write_nic_dword(dev, WFM3+4, NewBeaconNum);
	write_nic_dword(dev, WFM3, 0xB026007C);
}

//
//	Description:
//		Map dBm into Tx power index according to 
//		current HW model, for example, RF and PA, and
//		current wireless mode.
//	By Bruce, 2008-01-29.
//    use in phy only
static u8 phy_DbmToTxPwrIdx(
	struct net_device* dev, 
	WIRELESS_MODE	WirelessMode,
	long			PowerInDbm
	)
{
	//struct r8192_priv *priv = rtllib_priv(dev);
	u8				TxPwrIdx = 0;
	long				Offset = 0;
	

	//
	// Tested by MP, we found that CCK Index 0 equals to -7dbm, OFDM legacy equals to 
	// 3dbm, and OFDM HT equals to 0dbm repectively.
	// Note:
	//	The mapping may be different by different NICs. Do not use this formula for what needs accurate result.  
	// By Bruce, 2008-01-29.
	// 
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;
		break;
	default:
		break;
	}

	if((PowerInDbm - Offset) > 0)
	{
		TxPwrIdx = (u8)((PowerInDbm - Offset) * 2);
	}
	else
	{
		TxPwrIdx = 0;
	}

	// Tx Power Index is too large.
	if(TxPwrIdx > MAX_TXPWR_IDX_NMODE_92S)
		TxPwrIdx = MAX_TXPWR_IDX_NMODE_92S;

	return TxPwrIdx;
}
//
//	Description:
//		Map Tx power index into dBm according to 
//		current HW model, for example, RF and PA, and
//		current wireless mode.
//	By Bruce, 2008-01-29.
//    use in phy only
static long phy_TxPwrIdxToDbm(
	struct net_device* dev,
	WIRELESS_MODE	WirelessMode,
	u8			TxPwrIdx
	)
{
	//struct r8192_priv *priv = rtllib_priv(dev);
	long				Offset = 0;
	long				PwrOutDbm = 0;
	
	//
	// Tested by MP, we found that CCK Index 0 equals to -7dbm, OFDM legacy equals to 
	// 3dbm, and OFDM HT equals to 0dbm repectively.
	// Note:
	//	The mapping may be different by different NICs. Do not use this formula for what needs accurate result.  
	// By Bruce, 2008-01-29.
	// 
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;
		break;
	default:
		break;
	}

	PwrOutDbm = TxPwrIdx / 2 + Offset; // Discard the decimal part.

	return PwrOutDbm;
}


extern void PHY_ScanOperationBackup8192S(struct net_device* dev, u8 Operation)
{
	struct r8192_priv *priv = rtllib_priv(dev);

#if(RTL8192S_DISABLE_FW_DM == 0)

	if(priv->up)
	{
		switch(Operation)
		{
			case SCAN_OPT_BACKUP:
				//
				// <Roger_Notes> We halt FW DIG and disable high ppower both two DMs here 
				// and resume both two DMs while scan complete. 
				// 2008.11.27.
				//
				priv->rtllib->SetFwCmdHandler(dev, FW_CMD_PAUSE_DM_BY_SCAN);
				break;

			case SCAN_OPT_RESTORE:
				//
				// <Roger_Notes> We resume DIG and enable high power both two DMs here and 
				// recover earlier DIG settings. 
				// 2008.11.27.
				//
				priv->rtllib->SetFwCmdHandler(dev, FW_CMD_RESUME_DM_BY_SCAN);
				break;

			default:
				RT_TRACE(COMP_SCAN,"Unknown Scan Backup Operation. \n");
				break;
		}
	}
#endif
}


//nouse temp
void PHY_InitialGain8192S(struct net_device* dev,u8 Operation	)
{

	//struct r8192_priv *priv = rtllib_priv(dev);
	//u32					BitMask;
	//u8					initial_gain;

#if 0	// For 8192s test disable
	if(!dev->bDriverStopped)
	{
		switch(Operation)
		{
			case IG_Backup:
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("IG_Backup, backup the initial gain.\n"));
				initial_gain = priv->DefaultInitialGain[0];
				BitMask = bMaskByte0;
				if(DM_DigTable.Dig_Algorithm == DIG_ALGO_BY_FALSE_ALARM)
					PHY_SetMacReg(dev, UFWP, bMaskByte1, 0x8);	// FW DIG OFF
				pMgntInfo->InitGain_Backup.XAAGCCore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XAAGCCore1, BitMask);
				pMgntInfo->InitGain_Backup.XBAGCCore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XBAGCCore1, BitMask);
				pMgntInfo->InitGain_Backup.XCAGCCore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XCAGCCore1, BitMask);
				pMgntInfo->InitGain_Backup.XDAGCCore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XDAGCCore1, BitMask);
				BitMask  = bMaskByte2;
				pMgntInfo->InitGain_Backup.CCA		= (u8)rtl8192_QueryBBReg(dev, rCCK0_CCA, BitMask);

			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xc50 is %x\n",pMgntInfo->InitGain_Backup.XAAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xc58 is %x\n",pMgntInfo->InitGain_Backup.XBAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xc60 is %x\n",pMgntInfo->InitGain_Backup.XCAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xc68 is %x\n",pMgntInfo->InitGain_Backup.XDAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xa0a is %x\n",pMgntInfo->InitGain_Backup.CCA));

			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Write scan initial gain = 0x%x \n", initial_gain));
				write_nic_byte(dev, rOFDM0_XAAGCCore1, initial_gain);
				write_nic_byte(dev, rOFDM0_XBAGCCore1, initial_gain);
				write_nic_byte(dev, rOFDM0_XCAGCCore1, initial_gain);
				write_nic_byte(dev, rOFDM0_XDAGCCore1, initial_gain);
				break;
			case IG_Restore:
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("IG_Restore, restore the initial gain.\n"));
				BitMask = 0x7f; //Bit0~ Bit6
				if(DM_DigTable.Dig_Algorithm == DIG_ALGO_BY_FALSE_ALARM)
					PHY_SetMacReg(dev, UFWP, bMaskByte1, 0x8);	// FW DIG OFF
			
				rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, BitMask, (u32)pMgntInfo->InitGain_Backup.XAAGCCore1);
				rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, BitMask, (u32)pMgntInfo->InitGain_Backup.XBAGCCore1);
				rtl8192_setBBreg(dev, rOFDM0_XCAGCCore1, BitMask, (u32)pMgntInfo->InitGain_Backup.XCAGCCore1);
				rtl8192_setBBreg(dev, rOFDM0_XDAGCCore1, BitMask, (u32)pMgntInfo->InitGain_Backup.XDAGCCore1);
				BitMask  = (BIT22|BIT23);
				rtl8192_setBBreg(dev, rCCK0_CCA, BitMask, (u32)pMgntInfo->InitGain_Backup.CCA);

			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xc50 is %x\n",pMgntInfo->InitGain_Backup.XAAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xc58 is %x\n",pMgntInfo->InitGain_Backup.XBAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xc60 is %x\n",pMgntInfo->InitGain_Backup.XCAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xc68 is %x\n",pMgntInfo->InitGain_Backup.XDAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xa0a is %x\n",pMgntInfo->InitGain_Backup.CCA));

				if(DM_DigTable.Dig_Algorithm == DIG_ALGO_BY_FALSE_ALARM)
					PHY_SetMacReg(dev, UFWP, bMaskByte1, 0x1);	// FW DIG ON
				break;
			default:
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Unknown IG Operation. \n"));
				break;
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Function:    SetBWModeCallback8190Pci()
//
// Overview:    Timer callback function for SetSetBWMode
//
// Input:       	PRT_TIMER		pTimer
//
// Output:      NONE
//
// Return:      NONE
//
// Note:		(1) We do not take j mode into consideration now
//			(2) Will two workitem of "switch channel" and "switch channel bandwidth" run
//			     concurrently?
//---------------------------------------------------------------------------*/
//    use in phy only (in win it's timer)
void PHY_SetBWModeCallback8192S(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	 			regBwOpMode;

	//return;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//u32				NowL, NowH;
	//u8Byte				BeginTime, EndTime; 
	u8				regRRSR_RSC;

	RT_TRACE(COMP_SWBW, "==>SetBWModeCallback8190Pci()  Switch to %s bandwidth\n", \
					priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz");

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SetBWModeInProgress= false;
		return;
	}
	
	if(!priv->up)
		return;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = read_nic_dword(dev, TSFR);
	//NowH = read_nic_dword(dev, TSFR+4);
	//BeginTime = ((u8Byte)NowH << 32) + NowL;
		
	//3//
	//3//<1>Set MAC register
	//3//
	regBwOpMode = read_nic_byte(dev, BW_OPMODE);
	regRRSR_RSC = read_nic_byte(dev, RRSR+2);
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			//if(priv->card_8192_version >= VERSION_8192S_BCUT)
			//	write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x58);

			regBwOpMode |= BW_OPMODE_20MHZ;
		       	// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:
			//if(priv->card_8192_version >= VERSION_8192S_BCUT)
			//	write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x18);

			regBwOpMode &= ~BW_OPMODE_20MHZ;
        		// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			regRRSR_RSC = (regRRSR_RSC&0x90) |(priv->nCur40MhzPrimeSC<<5);
			write_nic_byte(dev, RRSR+2, regRRSR_RSC);
			break;

		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8190Pci():\
						unknown Bandwidth: %#X\n",priv->CurrentChannelBW);
			break;
	}
	
	//3//
	//3//<2>Set PHY related register
	//3//
	switch(priv->CurrentChannelBW)
	{
		// 20 MHz channel*/
		case HT_CHANNEL_WIDTH_20:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x0);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x0);

			// Correct the tx power for CCK rate in 40M. Suggest by YN, 20071207
			// It is set in Tx descriptor for 8192x series
			//write_nic_dword(dev, rCCK0_TxFilter1, 0x1a1b0000);
			//write_nic_dword(dev, rCCK0_TxFilter2, 0x090e1317);
			//write_nic_dword(dev, rCCK0_DebugPort, 0x00000204);
			#if 0 //LZM 090219
			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000204);
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00300000, 3);
			#endif
			
			if (priv->card_8192_version >= VERSION_8192S_BCUT)
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x58);
			

			break;

		// 40 MHz channel*/
		case HT_CHANNEL_WIDTH_20_40:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x1);
			
			// Correct the tx power for CCK rate in 40M. Suggest by YN, 20071207
			//write_nic_dword(dev, rCCK0_TxFilter1, 0x35360000);
			//write_nic_dword(dev, rCCK0_TxFilter2, 0x121c252e);
			//write_nic_dword(dev, rCCK0_DebugPort, 0x00000409);
			#if 0 //LZM 090219
			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x35360000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x121c252e);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000409);
			#endif
			
			// Set Control channel to upper or lower. These settings are required only for 40MHz
			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));
			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);			
			
			//rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00300000, 3);
			if (priv->card_8192_version >= VERSION_8192S_BCUT)
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x18);
			
			break;
			
		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8190Pci(): unknown Bandwidth: %#X\n"\
						,priv->CurrentChannelBW);
			break;
			
	}
	//Skip over setting of J-mode in BB register here. Default value is "None J mode". Emily 20070315

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = read_nic_dword(dev, TSFR);
	//NowH = read_nic_dword(dev, TSFR+4);
	//EndTime = ((u8Byte)NowH << 32) + NowL;
	//RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWModeCallback8190Pci: time of SetBWMode = %I64d us!\n", (EndTime - BeginTime)));

	//3<3>Set RF related register
	switch( priv->rf_chip )
	{
		case RF_8225:		
			//PHY_SetRF8225Bandwidth(dev, priv->CurrentChannelBW);
			break;	
			
		case RF_8256:
			// Please implement this function in Hal8190PciPhy8256.c
			//PHY_SetRF8256Bandwidth(dev, priv->CurrentChannelBW);
			break;
			
		case RF_8258:
			// Please implement this function in Hal8190PciPhy8258.c
			// PHY_SetRF8258Bandwidth();
			break;

		case RF_PSEUDO_11N:
			// Do Nothing
			break;

		case RF_6052:
			PHY_RF6052SetBandwidth(dev, priv->CurrentChannelBW);
			break;
		default:
			printk("Unknown rf_chip: %d\n", priv->rf_chip);
			break;
	}

	priv->SetBWModeInProgress= false;

	RT_TRACE(COMP_SWBW, "<==SetBWModeCallback8190Pci() \n" );
}


//-----------------------------------------------------------------------------
// Function:   SetBWMode8190Pci()
//
// Overview:  This function is export to "HalCommon" moudule
//
// Input:       	PADAPTER			Adapter
//			HT_CHANNEL_WIDTH	Bandwidth	//20M or 40M
//
// Output:      NONE
//
// Return:      NONE
//
// Note:		We do not take j mode into consideration now
//---------------------------------------------------------------------------*/
//extern void PHY_SetBWMode8192S(	struct net_device* dev,
//	HT_CHANNEL_WIDTH	Bandwidth,	// 20M or 40M
//	HT_EXTCHNL_OFFSET	Offset		// Upper, Lower, or Don't care
void rtl8192_SetBWMode(struct net_device *dev, HT_CHANNEL_WIDTH	Bandwidth, HT_EXTCHNL_OFFSET Offset)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	HT_CHANNEL_WIDTH tmpBW = priv->CurrentChannelBW; 

	
	// Modified it for 20/40 mhz switch by guangan 070531

	//return;
	
	//if(priv->SwChnlInProgress)
//	if(pMgntInfo->bScanInProgress)
//	{
//		RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWMode8190Pci() %s Exit because bScanInProgress!\n", 
//					Bandwidth == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz"));
//		return;
//	}

//	if(priv->SetBWModeInProgress)
//	{
//		// Modified it for 20/40 mhz switch by guangan 070531
//		RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWMode8190Pci() %s cancel last timer because SetBWModeInProgress!\n", 
//					Bandwidth == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz"));
//		PlatformCancelTimer(dev, &priv->SetBWModeTimer);
//		//return;
//	}

	if(priv->SetBWModeInProgress)
		return;

	priv->SetBWModeInProgress= true;
	
	priv->CurrentChannelBW = Bandwidth;
	
	if(Offset==HT_EXTCHNL_OFFSET_LOWER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
	else if(Offset==HT_EXTCHNL_OFFSET_UPPER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
	else
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

#if 0
	if(!priv->bDriverStopped)
	{
#ifdef USE_WORKITEM	
		PlatformScheduleWorkItem(&(priv->SetBWModeWorkItem));//SetBWModeCallback8192SUsbWorkItem
#else
		PlatformSetTimer(dev, &(priv->SetBWModeTimer), 0);//PHY_SetBWModeCallback8192S
#endif		
	}
#endif
	if((priv->up) )// && !(RT_CANNOT_IO(Adapter) && Adapter->bInSetPower) )
	{
#ifdef RTL8192SE
	PHY_SetBWModeCallback8192S(dev);
#elif defined(RTL8192SU)		
	SetBWModeCallback8192SUsbWorkItem(dev);
#endif
	}
	else
	{
		RT_TRACE(COMP_SCAN, "PHY_SetBWMode8192S() SetBWModeInProgress false driver sleep or unload\n");	
		priv->SetBWModeInProgress= false;	
		priv->CurrentChannelBW = tmpBW;
	}
}

//    use in phy only (in win it's timer)
void PHY_SwChnlCallback8192S(struct net_device *dev)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	u32		delay;
	//bool			ret;
	
	RT_TRACE(COMP_CH, "==>SwChnlCallback8190Pci(), switch to channel %d\n", priv->chan);
	
	if(!priv->up)
		return;
	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=false;
		return; 								//return immediately if it is peudo-phy	
	}
	
	do{
		if(!priv->SwChnlInProgress)
			break;

		//if(!phy_SwChnlStepByStep(dev, priv->CurrentChannel, &priv->SwChnlStage, &priv->SwChnlStep, &delay))
		if(!phy_SwChnlStepByStep(dev, priv->chan, &priv->SwChnlStage, &priv->SwChnlStep, &delay))
		{
			if(delay>0)
			{
				mdelay(delay);
				//PlatformSetTimer(dev, &priv->SwChnlTimer, delay);
				//mod_timer(&priv->SwChnlTimer,  jiffies + MSECS(delay));
				//==>PHY_SwChnlCallback8192S(dev); for 92se
				//==>SwChnlCallback8192SUsb(dev) for 92su
			}
			else
			continue;
		}
		else
		{
			priv->SwChnlInProgress=false;
			break;
		}
	}while(true);
}

// Call after initialization
//extern void PHY_SwChnl8192S(struct net_device* dev,	u8 channel)
u8 rtl8192_phy_SwChnl(struct net_device* dev, u8 channel)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	//u8 			tmpchannel =channel;
	//bool			bResult = false;

        if(!priv->up)
		return false;
		
	if(priv->SwChnlInProgress)
		return false;

	if(priv->SetBWModeInProgress)
		return false;

	//--------------------------------------------
	switch(priv->rtllib->mode)
	{
	case WIRELESS_MODE_A:
	case WIRELESS_MODE_N_5G:
		if (channel<=14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_A but channel<=14");
			return false;
		}
		break;
		
	case WIRELESS_MODE_B:
		if (channel>14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_B but channel>14");
			return false;
		}
		break;
		
	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		if (channel>14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_G but channel>14");
			return false;
		}
		break;

	default:
			;//RT_TRACE(COMP_ERR, "Invalid WirelessMode(%#x)!!\n", priv->rtllib->mode);
		break;
	}
	//--------------------------------------------
	
	priv->SwChnlInProgress = true;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;

	priv->SwChnlStage=0;
	priv->SwChnlStep=0;

	if((priv->up))// && !(RT_CANNOT_IO(Adapter) && Adapter->bInSetPower))
	{
#ifdef RTL8192SE
	PHY_SwChnlCallback8192S(dev);
#elif defined(RTL8192SU)	
	SwChnlCallback8192SUsbWorkItem(dev);
#endif
#ifdef TO_DO_LIST
	if(bResult)
		{
			RT_TRACE(COMP_SCAN, "PHY_SwChnl8192S SwChnlInProgress true schdule workitem done\n");
		}
		else
		{
			RT_TRACE(COMP_SCAN, "PHY_SwChnl8192S SwChnlInProgress false schdule workitem error\n");		
			priv->SwChnlInProgress = false; 	
			priv->CurrentChannel = tmpchannel;			
		}
#endif
	}
	else
	{
		RT_TRACE(COMP_SCAN, "PHY_SwChnl8192S SwChnlInProgress false driver sleep or unload\n");	
		priv->SwChnlInProgress = false;		
		//priv->CurrentChannel = tmpchannel;
	}
        return true;	
}


//
// Description:
//	Switch channel synchronously. Called by SwChnlByDelayHandler.
//
// Implemented by Bruce, 2008-02-14.
// The following procedure is operted according to SwChanlCallback8190Pci().
// However, this procedure is performed synchronously  which should be running under
// passive level.
// 
//not understant it
void PHY_SwChnlPhy8192S(	// Only called during initialize
	struct net_device* dev,
	u8		channel
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	RT_TRACE(COMP_SCAN, "==>PHY_SwChnlPhy8192S(), switch to channel %d.\n", priv->chan);

#ifdef TO_DO_LIST
	// Cannot IO.
	if(RT_CANNOT_IO(dev))
		return;
#endif

	// Channel Switching is in progress.
	if(priv->SwChnlInProgress)
		return;
	
	//return immediately if it is peudo-phy
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=false;
		return;
	}
	
	priv->SwChnlInProgress = true;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;
	
	priv->SwChnlStage = 0;
	priv->SwChnlStep = 0;
	
	phy_FinishSwChnlNow(dev,channel);
	
	priv->SwChnlInProgress = false;
}

//    use in phy only 
static bool
phy_SetSwChnlCmdArray(
	SwChnlCmd*		CmdTable,
	u32			CmdTableIdx,
	u32			CmdTableSz,
	SwChnlCmdID		CmdID,
	u32			Para1,
	u32			Para2,
	u32			msDelay
	)
{
	SwChnlCmd* pCmd;

	if(CmdTable == NULL)
	{
		//RT_ASSERT(false, ("phy_SetSwChnlCmdArray(): CmdTable cannot be NULL.\n"));
		return false;
	}
	if(CmdTableIdx >= CmdTableSz)
	{
		//RT_ASSERT(false, 
			//	("phy_SetSwChnlCmdArray(): Access invalid index, please check size of the table, CmdTableIdx:%d, CmdTableSz:%d\n",
				//CmdTableIdx, CmdTableSz));
		return false;
	}

	pCmd = CmdTable + CmdTableIdx;
	pCmd->CmdID = CmdID;
	pCmd->Para1 = Para1;
	pCmd->Para2 = Para2;
	pCmd->msDelay = msDelay;

	return true;
}

//    use in phy only 
static bool
phy_SwChnlStepByStep(
	struct net_device* dev,
	u8		channel,
	u8		*stage,
	u8		*step,
	u32		*delay
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	//PCHANNEL_ACCESS_SETTING	pChnlAccessSetting;
	SwChnlCmd				PreCommonCmd[MAX_PRECMD_CNT];
	u32					PreCommonCmdCnt;
	SwChnlCmd				PostCommonCmd[MAX_POSTCMD_CNT];
	u32					PostCommonCmdCnt;
	SwChnlCmd				RfDependCmd[MAX_RFDEPENDCMD_CNT];
	u32					RfDependCmdCnt;
	SwChnlCmd				*CurrentCmd = NULL;	
	u8					eRFPath;	
	
	//RT_ASSERT((dev != NULL), ("Adapter should not be NULL\n"));
	//RT_ASSERT(IsLegalChannel(dev, channel), ("illegal channel: %d\n", channel));
	RT_TRACE(COMP_CH, "===========>%s(), channel:%d, stage:%d, step:%d\n", __FUNCTION__, channel, *stage, *step);		
	//RT_ASSERT((pHalData != NULL), ("pHalData should not be NULL\n"));
#ifdef ENABLE_DOT11D
	if (!IsLegalChannel(priv->rtllib, channel))
	{
		RT_TRACE(COMP_ERR, "=============>set to illegal channel:%d\n", channel);
		return true; //return true to tell upper caller function this channel setting is finished! Or it will in while loop.
	}		
#endif	

	//pChnlAccessSetting = &Adapter->MgntInfo.Info8185.ChannelAccessSetting;
	//RT_ASSERT((pChnlAccessSetting != NULL), ("pChnlAccessSetting should not be NULL\n"));
	
	//for(eRFPath = RF90_PATH_A; eRFPath <priv->NumTotalRFPath; eRFPath++)
	//for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	//{
		// <1> Fill up pre common command.
	PreCommonCmdCnt = 0;
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_SetTxPowerLevel, 0, 0, 0);
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_End, 0, 0, 0);
	
		// <2> Fill up post common command.
	PostCommonCmdCnt = 0;

	phy_SetSwChnlCmdArray(PostCommonCmd, PostCommonCmdCnt++, MAX_POSTCMD_CNT, 
				CmdID_End, 0, 0, 0);
	
		// <3> Fill up RF dependent command.
	RfDependCmdCnt = 0;
	switch( priv->rf_chip )
	{
		case RF_8225:		
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		//RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		// 2008/09/04 MH Change channel. 
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;	
		
	case RF_8256:
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		// TEST!! This is not the table for 8256!!
		//RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;
		
	case RF_6052:
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, RF_CHNLBW, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_End, 0, 0, 0);		
		break;

	case RF_8258:
		break;
		
	default:
		//RT_ASSERT(false, ("Unknown rf_chip: %d\n", priv->rf_chip));
		return false;
		break;
	}

	
	do{
		switch(*stage)
		{
		case 0:
			CurrentCmd=&PreCommonCmd[*step];
			break;
		case 1:
			CurrentCmd=&RfDependCmd[*step];
			break;
		case 2:
			CurrentCmd=&PostCommonCmd[*step];
			break;
		}
		
		if(CurrentCmd->CmdID==CmdID_End)
		{
			if((*stage)==2)
			{
				return true;
			}
			else
			{
				(*stage)++;
				(*step)=0;
				continue;
			}
		}
		
		switch(CurrentCmd->CmdID)
		{
		case CmdID_SetTxPowerLevel:
			//if(priv->card_8192_version > VERSION_8190_BD)
				PHY_SetTxPowerLevel8192S(dev,channel);
			break;
		case CmdID_WritePortUlong:
			write_nic_dword(dev, CurrentCmd->Para1, CurrentCmd->Para2);
			break;
		case CmdID_WritePortUshort:
			write_nic_word(dev, CurrentCmd->Para1, (u16)CurrentCmd->Para2);
			break;
		case CmdID_WritePortUchar:
			write_nic_byte(dev, CurrentCmd->Para1, (u8)CurrentCmd->Para2);
			break;
		case CmdID_RF_WriteReg:	// Only modify channel for the register now !!!!!
			for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
			{
#ifdef  RTL8192SU
			// For new T65 RF 0222d register 0x18 bit 0-9 = channel number.
				//rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, 0x1f, (CurrentCmd->Para2));
				priv->RfRegChnlVal[eRFPath] = ((priv->RfRegChnlVal[eRFPath] & 0xfffffc00) | CurrentCmd->Para2);
				rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, priv->RfRegChnlVal[eRFPath]);
#endif
			}
			break;
                default:
                        break;
		}
		
		break;
	}while(true);
	//cosa }/*for(Number of RF paths)*/

	(*delay)=CurrentCmd->msDelay;
	(*step)++;
	RT_TRACE(COMP_CH, "<===========%s(), channel:%d, stage:%d, step:%d\n", __FUNCTION__, channel, *stage, *step);		
	return false;
}

//called PHY_SwChnlPhy8192S, SwChnlCallback8192SUsbWorkItem
//    use in phy only 
static void
phy_FinishSwChnlNow(	// We should not call this function directly
	struct net_device* dev,
	u8		channel
		)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32			delay;
  
	while(!phy_SwChnlStepByStep(dev,channel,&priv->SwChnlStage,&priv->SwChnlStep,&delay))
	{
		//if(delay>0)    
		//	mdelay(delay); 
		if(!priv->up)
			break;
	}
}


/*-----------------------------------------------------------------------------
 * Function:	PHYCheckIsLegalRfPath8190Pci()
 *
 * Overview:	Check different RF type to execute legal judgement. If RF Path is illegal
 *			We will return false.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	11/15/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
 //called by rtl8192_phy_QueryRFReg, rtl8192_phy_SetRFReg, PHY_SetRFPowerState8192SUsb
//extern	bool	
//PHY_CheckIsLegalRfPath8192S(	
//	struct net_device* dev,
//	u32	eRFPath)
u8 rtl8192_phy_CheckIsLegalRFPath(struct net_device* dev, u32 eRFPath)
{
//	struct r8192_priv *priv = rtllib_priv(dev);
	bool				rtValue = true;

	// NOt check RF Path now.!
#if 0	
	if (priv->rf_type == RF_1T2R && eRFPath != RF90_PATH_A)
	{		
		rtValue = false;
	}
	if (priv->rf_type == RF_1T2R && eRFPath != RF90_PATH_A)
	{

	}
#endif
	return	rtValue;

}	// PHY_CheckIsLegalRfPath8192S */



//-----------------------------------------------------------------------------
// Function:	PHY_IQCalibrate8192S()
//
// Overview:	After all MAC/PHY/RF is configued. We must execute IQ calibration 
//			to improve RF EVM!!?
//
// Input:		IN	PADAPTER	pAdapter
//
// Output:		NONE
//
// Return:		NONE
//
// Revised History:
//	When		Who		Remark
//	10/07/2008	MHC		Create. Document from SD3 RFSI Jenyu.  
//
//---------------------------------------------------------------------------*/
 //called by InitializeAdapter8192SE
void	
PHY_IQCalibrate(	struct net_device* dev)
{
	//struct r8192_priv 	*priv = rtllib_priv(dev);
	u32				i, reg;
	u32				old_value;
	long				X, Y, TX0[4];
	u32				TXA[4];
	
	// 1. Check QFN68 or 64 92S (Read from EEPROM)

	//
	// 2. QFN 68	
	//
	// For 1T2R IQK only now !!!
	for (i = 0; i < 10; i++)
	{
		// IQK 
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05430);
		//PlatformStallExecution(5);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000800e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x80800000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe40, bMaskDWord, 0x02140148);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe44, bMaskDWord, 0x681604a2);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe4c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe60, bMaskDWord, 0x0214014d);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe64, bMaskDWord, 0x281608ba);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe6c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000001);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000001);
		udelay(2000);
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05433);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000000e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x0);		

	
		reg = rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord);

		// Readback IQK value and rewrite
		if (!(reg&(BIT27|BIT28|BIT30|BIT31)))
		{
			old_value = (rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord) & 0x3FF);

			// Calibrate init gain for A path for TX0
			X = (rtl8192_QueryBBReg(dev, 0xe94, bMaskDWord) & 0x03FF0000)>>16;
			TXA[RF90_PATH_A] = (X * old_value)/0x100;
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xFFFFFC00) | (u32)TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			udelay(5);

			// Calibrate init gain for C path for TX0
			Y = ( rtl8192_QueryBBReg(dev, 0xe9C, bMaskDWord) & 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xffc0ffff) |((u32) (TX0[RF90_PATH_C]&0x3F)<<16);			
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc94, bMaskDWord);		
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc94, bMaskDWord, reg);
			udelay(5);

			// Calibrate RX A and B for RX0
			reg = rtl8192_QueryBBReg(dev, 0xc14, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xea4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			Y = (rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			udelay(5);
			old_value = (rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord) & 0x3FF);
			
			// Calibrate init gain for A path for TX1 !!!!!!
			X = (rtl8192_QueryBBReg(dev, 0xeb4, bMaskDWord) & 0x03FF0000)>>16;
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			TXA[RF90_PATH_A] = (X * old_value) / 0x100;
			reg = (reg & 0xFFFFFC00) | TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			udelay(5);

			// Calibrate init gain for C path for TX1
			Y = (rtl8192_QueryBBReg(dev, 0xebc, bMaskDWord)& 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			reg = (reg & 0xffc0ffff) |( (TX0[RF90_PATH_C]&0x3F)<<16);
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc9c, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc9c, bMaskDWord, reg);
			udelay(5);

			// Calibrate RX A and B for RX1
			reg = rtl8192_QueryBBReg(dev, 0xc1c, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xec4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			
			Y = (rtl8192_QueryBBReg(dev, 0xecc, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			udelay(5);
			
			RT_TRACE(COMP_INIT, "PHY_IQCalibrate OK\n");
			break;
		}

	}
	
	
	//
	// 3. QFN64. Not enabled now !!! We must use different gain table for 1T2R.
	//


}

//-----------------------------------------------------------------------------
// Function:	PHY_IQCalibrateBcut()
//
// Overview:	After all MAC/PHY/RF is configued. We must execute IQ calibration 
//			to improve RF EVM!!?
//
// Input:		IN	PADAPTER	pAdapter
//
// Output:		NONE
//
// Return:		NONE
//
// Revised History:
//	When		Who		Remark
//	11/18/2008	MHC		Create. Document from SD3 RFSI Jenyu. 
//						92S B-cut QFN 68 pin IQ calibration procedure.doc
//
//---------------------------------------------------------------------------*/
extern void PHY_IQCalibrateBcut(struct net_device* dev)
{
	//struct r8192_priv 	*priv = rtllib_priv(dev);
	//PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	u32				i, reg;
	u32				old_value;
	long				X, Y, TX0[4];
	u32				TXA[4];	
	u32				calibrate_set[13] = {0};
	u32				load_value[13];
	u8				RfPiEnable=0;
	
	// 0. Check QFN68 or 64 92S (Read from EEPROM/EFUSE)

	//
	// 1. Save e70~ee0 register setting, and load calibration setting
	// 
	/*
	0xee0[31:0]=0x3fed92fb;   
	0xedc[31:0] =0x3fed92fb;   
	0xe70[31:0] =0x3fed92fb;   
	0xe74[31:0] =0x3fed92fb;   
	0xe78[31:0] =0x3fed92fb;   
	0xe7c[31:0]= 0x3fed92fb;   
	0xe80[31:0]= 0x3fed92fb;   
	0xe84[31:0]= 0x3fed92fb; 
	0xe88[31:0]= 0x3fed92fb;  
	0xe8c[31:0]= 0x3fed92fb;   
	0xed0[31:0]= 0x3fed92fb;   
	0xed4[31:0]= 0x3fed92fb;  
	0xed8[31:0]= 0x3fed92fb;
	*/
	calibrate_set [0] = 0xee0;
	calibrate_set [1] = 0xedc;
	calibrate_set [2] = 0xe70;
	calibrate_set [3] = 0xe74;
	calibrate_set [4] = 0xe78;
	calibrate_set [5] = 0xe7c;
	calibrate_set [6] = 0xe80;
	calibrate_set [7] = 0xe84;
	calibrate_set [8] = 0xe88;
	calibrate_set [9] = 0xe8c;
	calibrate_set [10] = 0xed0;
	calibrate_set [11] = 0xed4;
	calibrate_set [12] = 0xed8;
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Save e70~ee0 register setting\n"));	
	for (i = 0; i < 13; i++)
	{
		load_value[i] = rtl8192_QueryBBReg(dev, calibrate_set[i], bMaskDWord);
		rtl8192_setBBreg(dev, calibrate_set[i], bMaskDWord, 0x3fed92fb);		
		
	}
	
	RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT8);

	//
	// 2. QFN 68	
	//
	// For 1T2R IQK only now !!!
	for (i = 0; i < 10; i++)
	{
		RT_TRACE(COMP_INIT, "IQK -%d\n", i);	
		//BB switch to PI mode. If default is PI mode, ignoring 2 commands below.
		if (!RfPiEnable)	//if original is SI mode, then switch to PI mode.
		{
			//DbgPrint("IQK Switch to PI mode\n");
			rtl8192_setBBreg(dev, 0x820, bMaskDWord, 0x01000100);
			rtl8192_setBBreg(dev, 0x828, bMaskDWord, 0x01000100);
		}
	
		// IQK 
		// 2. IQ calibration & LO leakage calibration
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05430);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000800e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x80800000);
		udelay(5);
		//path-A IQ K and LO K gain setting
		rtl8192_setBBreg(dev, 0xe40, bMaskDWord, 0x02140102);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe44, bMaskDWord, 0x681604c2);
		udelay(5);
		//set LO calibration
		rtl8192_setBBreg(dev, 0xe4c, bMaskDWord, 0x000028d1);
		udelay(5);
		//path-B IQ K and LO K gain setting
		rtl8192_setBBreg(dev, 0xe60, bMaskDWord, 0x02140102);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe64, bMaskDWord, 0x28160d05);
		udelay(5);
		//K idac_I & IQ
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000000);
		udelay(5);

		// delay 2ms
		udelay(2000);

		//idac_Q setting
		rtl8192_setBBreg(dev, 0xe6c, bMaskDWord, 0x020028d1);
		udelay(5);
		//K idac_Q & IQ
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000000);

		// delay 2ms
		udelay(2000);

		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05433);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000000e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x0);		

		if (!RfPiEnable)	//if original is SI mode, then switch to PI mode.
		{
			//DbgPrint("IQK Switch back to SI mode\n");
			rtl8192_setBBreg(dev, 0x820, bMaskDWord, 0x01000000);
			rtl8192_setBBreg(dev, 0x828, bMaskDWord, 0x01000000);
		}

	
		reg = rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord);

		// 3.	check fail bit, and fill BB IQ matrix
		// Readback IQK value and rewrite
		if (!(reg&(BIT27|BIT28|BIT30|BIT31)))
		{			
			old_value = (rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord) & 0x3FF);

			// Calibrate init gain for A path for TX0
			X = (rtl8192_QueryBBReg(dev, 0xe94, bMaskDWord) & 0x03FF0000)>>16;
			TXA[RF90_PATH_A] = (X * old_value)/0x100;
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xFFFFFC00) | (u32)TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			udelay(5);

			// Calibrate init gain for C path for TX0
			Y = ( rtl8192_QueryBBReg(dev, 0xe9C, bMaskDWord) & 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xffc0ffff) |((u32) (TX0[RF90_PATH_C]&0x3F)<<16);			
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc94, bMaskDWord);		
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc94, bMaskDWord, reg);
			udelay(5);

			// Calibrate RX A and B for RX0
			reg = rtl8192_QueryBBReg(dev, 0xc14, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xea4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			Y = (rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			udelay(5);
			old_value = (rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord) & 0x3FF);
			
			// Calibrate init gain for A path for TX1 !!!!!!
			X = (rtl8192_QueryBBReg(dev, 0xeb4, bMaskDWord) & 0x03FF0000)>>16;
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			TXA[RF90_PATH_A] = (X * old_value) / 0x100;
			reg = (reg & 0xFFFFFC00) | TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			udelay(5);

			// Calibrate init gain for C path for TX1
			Y = (rtl8192_QueryBBReg(dev, 0xebc, bMaskDWord)& 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			reg = (reg & 0xffc0ffff) |( (TX0[RF90_PATH_C]&0x3F)<<16);
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc9c, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc9c, bMaskDWord, reg);
			udelay(5);

			// Calibrate RX A and B for RX1
			reg = rtl8192_QueryBBReg(dev, 0xc1c, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xec4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			
			Y = (rtl8192_QueryBBReg(dev, 0xecc, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			udelay(5);
			
			RT_TRACE(COMP_INIT, "PHY_IQCalibrate OK\n");
			break;
		}

	}
	
	//
	// 4. Reload e70~ee0 register setting.
	//
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reload e70~ee0 register setting.\n"));	
	for (i = 0; i < 13; i++)
		rtl8192_setBBreg(dev, calibrate_set[i], bMaskDWord, load_value[i]);
	
	
	//
	// 3. QFN64. Not enabled now !!! We must use different gain table for 1T2R.
	//
	


}	// PHY_IQCalibrateBcut


//
// Move from phycfg.c to gen.c to be code independent later
// 
//-------------------------Move to other DIR later----------------------------*/
//#if (DEV_BUS_TYPE == USB_INTERFACE)
#ifdef RTL8192SU

//    use in phy only (in win it's timer)
void SwChnlCallback8192SUsb(struct net_device *dev)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	u32			delay;
//	bool			ret;

	RT_TRACE(COMP_SCAN, "==>SwChnlCallback8190Pci(), switch to channel\
				%d\n", priv->chan);


	if(!priv->up)
		return;
	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=false;
		return; 								//return immediately if it is peudo-phy	
	}
	
	do{
		if(!priv->SwChnlInProgress)
			break;

		if(!phy_SwChnlStepByStep(dev, priv->chan, &priv->SwChnlStage, &priv->SwChnlStep, &delay))
		{
			if(delay>0)
			{
				//PlatformSetTimer(dev, &priv->SwChnlTimer, delay);
				
			}
			else
			continue;
		}
		else
		{
			priv->SwChnlInProgress=false;
		}
		break;
	}while(true);
}


//
// Callback routine of the work item for switch channel.
//
//    use in phy only (in win it's work)
void SwChnlCallback8192SUsbWorkItem(struct net_device *dev )
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	RT_TRACE(COMP_TRACE, "==> SwChnlCallback8192SUsbWorkItem()\n");
#ifdef TO_DO_LIST	
	if(pAdapter->bInSetPower && RT_USB_CANNOT_IO(pAdapter))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("<== SwChnlCallback8192SUsbWorkItem() SwChnlInProgress false driver sleep or unload\n"));
	
		pHalData->SwChnlInProgress = false;		
		return;
	}
#endif
	phy_FinishSwChnlNow(dev, priv->chan);  
	priv->SwChnlInProgress = false;
	
	RT_TRACE(COMP_TRACE, "<== SwChnlCallback8192SUsbWorkItem()\n");
}


//-----------------------------------------------------------------------------
// Function:    SetBWModeCallback8192SUsb()
//
// Overview:    Timer callback function for SetSetBWMode
//
// Input:       	PRT_TIMER		pTimer
//
// Output:      NONE
//
// Return:      NONE
//
// Note:		(1) We do not take j mode into consideration now
//			(2) Will two workitem of "switch channel" and "switch channel bandwidth" run
//			     concurrently?
//---------------------------------------------------------------------------*/
//====>//rtl8192_SetBWMode
//    use in phy only (in win it's timer)
void SetBWModeCallback8192SUsb(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	 			regBwOpMode;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//u32				NowL, NowH;
	//u8Byte				BeginTime, EndTime; 
	u8				regRRSR_RSC;
	
	RT_TRACE(COMP_SCAN, "==>SetBWModeCallback8190Pci()  Switch to %s bandwidth\n", \
					priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz");

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SetBWModeInProgress= false;
		return;
	}
		
	if(!priv->up)
		return;
		
	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = read_nic_dword(dev, TSFR);
	//NowH = read_nic_dword(dev, TSFR+4);
	//BeginTime = ((u8Byte)NowH << 32) + NowL;
		
	//3<1>Set MAC register
	regBwOpMode = read_nic_byte(dev, BW_OPMODE);
	regRRSR_RSC = read_nic_byte(dev, RRSR+2);
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
		       // 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
        		// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			
			regRRSR_RSC = (regRRSR_RSC&0x90) |(priv->nCur40MhzPrimeSC<<5);
			write_nic_byte(dev, RRSR+2, regRRSR_RSC);
			break;

		default:
			RT_TRACE(COMP_DBG, "SetChannelBandwidth8190Pci():\
						unknown Bandwidth: %#X\n",priv->CurrentChannelBW);
			break;
	}
	
	//3 <2>Set PHY related register
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x0);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x0);
			#if 0 //LZM090219
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00300000, 3);

			// Correct the tx power for CCK rate in 20M. Suggest by YN, 20071207
			//write_nic_dword(dev, rCCK0_TxFilter1, 0x1a1b0000);
			//write_nic_dword(dev, rCCK0_TxFilter2, 0x090e1317);
			//write_nic_dword(dev, rCCK0_DebugPort, 0x00000204);
			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000204);
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00300000, 3);
			#endif
			
			if (priv->card_8192_version >= VERSION_8192S_BCUT)
				rtl8192_setBBreg(dev, rFPGA0_AnalogParameter2, 0xff, 0x58);

			break;
		case HT_CHANNEL_WIDTH_20_40:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));
			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);

			// Correct the tx power for CCK rate in 40M. Suggest by YN, 20071207
			//PHY_SetBBReg(Adapter, rCCK0_TxFilter1, bMaskDWord, 0x35360000);
			//PHY_SetBBReg(Adapter, rCCK0_TxFilter2, bMaskDWord, 0x121c252e);
			//PHY_SetBBReg(Adapter, rCCK0_DebugPort, bMaskDWord, 0x00000409);
			//PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter1, bADClkPhase, 0);

			if (priv->card_8192_version >= VERSION_8192S_BCUT)
				rtl8192_setBBreg(dev, rFPGA0_AnalogParameter2, 0xff, 0x18);

			break;
		default:
			RT_TRACE(COMP_DBG, "SetChannelBandwidth8190Pci(): unknown Bandwidth: %#X\n"\
						,priv->CurrentChannelBW);
			break;
			
	}
	//Skip over setting of J-mode in BB register here. Default value is "None J mode". Emily 20070315

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = read_nic_dword(dev, TSFR);
	//NowH = read_nic_dword(dev, TSFR+4);
	//EndTime = ((u8Byte)NowH << 32) + NowL;
	//RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWModeCallback8190Pci: time of SetBWMode = %I64d us!\n", (EndTime - BeginTime)));

#if 1
	//3<3>Set RF related register
	switch( priv->rf_chip )
	{
		case RF_8225:		
			PHY_SetRF8225Bandwidth(dev, priv->CurrentChannelBW);
			break;	
			
		case RF_8256:
			// Please implement this function in Hal8190PciPhy8256.c
			//PHY_SetRF8256Bandwidth(dev, priv->CurrentChannelBW);
			break;
			
		case RF_6052:
			PHY_RF6052SetBandwidth(dev, priv->CurrentChannelBW);
			break;		
			
		case RF_8258:
			// Please implement this function in Hal8190PciPhy8258.c
			// PHY_SetRF8258Bandwidth();
			break;

		case RF_PSEUDO_11N:
			// Do Nothing
			break;
			
		default:
			//RT_ASSERT(false, ("Unknown rf_chip: %d\n", priv->rf_chip));
			break;
	}
#endif
	priv->SetBWModeInProgress= false;

	RT_TRACE(COMP_SCAN, "<==SetBWMode8190Pci()" );
}

//
// Callback routine of the work item for set bandwidth mode.
//
//    use in phy only (in win it's work)
void SetBWModeCallback8192SUsbWorkItem(struct net_device *dev)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);
	u8	 			regBwOpMode;
	u8			regRRSR_RSC;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//u32				NowL, NowH;
	//u8Byte				BeginTime, EndTime; 
	
	RT_TRACE(COMP_SCAN, "==>SetBWModeCallback8192SUsbWorkItem()  Switch to %s bandwidth\n", \
					priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz");

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SetBWModeInProgress= false;
		return;
	}
		
	if(!priv->up)
		return;
		
	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = read_nic_dword(dev, TSFR);
	//NowH = read_nic_dword(dev, TSFR+4);
	//BeginTime = ((u8Byte)NowH << 32) + NowL;
		
	//3<1>Set MAC register
	regBwOpMode = read_nic_byte(dev, BW_OPMODE);
	regRRSR_RSC = read_nic_byte(dev, RRSR+2);
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
		       // 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
        		// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);

			// Joseph marked out for Netgear 3500 TKIP channel 7 issue.(Temporarily)
			regRRSR_RSC = (regRRSR_RSC&0x90) |(priv->nCur40MhzPrimeSC<<5);
			write_nic_byte(dev, RRSR+2, regRRSR_RSC);

			break;

		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8192SUsbWorkItem():\
						unknown Bandwidth: %#X\n",priv->CurrentChannelBW);
			break;
	}
	
	//3 <2>Set PHY related register
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x0);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x0);
			
			#if 0 //LZM 090219
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, bADClkPhase, 1);

			// Correct the tx power for CCK rate in 20M. Suggest by YN, 20071207
			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000204);
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00100000, 1);
			#endif
			
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter2, 0xff, 0x58);

			break;
		case HT_CHANNEL_WIDTH_20_40:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x1);
			#if 0 //LZM 090219
			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));	

			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, bADClkPhase, 0);

   			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);
			// Correct the tx power for CCK rate in 40M. Suggest by YN, 20071207
			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x35360000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x121c252e);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000409);
			#endif
			
			// Set Control channel to upper or lower. These settings are required only for 40MHz
			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));
			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);

			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter2, 0xff, 0x18);

			break;


		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8192SUsbWorkItem(): unknown Bandwidth: %#X\n"\
						,priv->CurrentChannelBW);
			break;
			
	}
	//Skip over setting of J-mode in BB register here. Default value is "None J mode". Emily 20070315

	//3<3>Set RF related register
	switch( priv->rf_chip )
	{
		case RF_8225:		
			PHY_SetRF8225Bandwidth(dev, priv->CurrentChannelBW);
			break;	
			
		case RF_8256:
			// Please implement this function in Hal8190PciPhy8256.c
			//PHY_SetRF8256Bandwidth(dev, priv->CurrentChannelBW);
			break;
			
		case RF_6052:
			PHY_RF6052SetBandwidth(dev, priv->CurrentChannelBW);
			break;	

		case RF_8258:
			// Please implement this function in Hal8190PciPhy8258.c
			// PHY_SetRF8258Bandwidth();
			break;

		case RF_PSEUDO_11N:
			// Do Nothing
			break;
			
		default:
			//RT_ASSERT(false, ("Unknown rf_chip: %d\n", priv->rf_chip));
			break;
	}

	priv->SetBWModeInProgress= false;

	RT_TRACE(COMP_SCAN, "<==SetBWModeCallback8192SUsbWorkItem()" );
}

//--------------------------Move to oter DIR later-------------------------------*/
#ifdef RTL8192SU
void InitialGain8192S(struct net_device *dev,	u8 Operation)
{
#ifdef TO_DO_LIST
	struct r8192_priv *priv = rtllib_priv(dev);
#endif

}
#endif

void InitialGain819xUsb(struct net_device *dev,	u8 Operation)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	priv->InitialGainOperateType = Operation;

	if(priv->up)
	{
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20) 
		queue_delayed_work(priv->priv_wq,&priv->initialgain_operate_wq,0);
	#else
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		schedule_task(&priv->initialgain_operate_wq);	
		#else
		queue_work(priv->priv_wq,&priv->initialgain_operate_wq);	
		#endif
	#endif	
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
extern void InitialGainOperateWorkItemCallBack(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
       struct r8192_priv *priv = container_of(dwork,struct r8192_priv,initialgain_operate_wq);
       struct net_device *dev = priv->rtllib->dev;
#else
extern void InitialGainOperateWorkItemCallBack(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
#endif
#define SCAN_RX_INITIAL_GAIN	0x17
#define POWER_DETECTION_TH	0x08
	u32	BitMask;
	u8	initial_gain;
	u8	Operation;
	
	Operation = priv->InitialGainOperateType;

	switch(Operation)
	{
		case IG_Backup:
			RT_TRACE(COMP_SCAN, "IG_Backup, backup the initial gain.\n");
			initial_gain = SCAN_RX_INITIAL_GAIN;//priv->DefaultInitialGain[0];//
			BitMask = bMaskByte0;
			if(dm_digtable.dig_algorithm == DIG_ALGO_BY_FALSE_ALARM)
				rtl8192_setBBreg(dev, UFWP, bMaskByte1, 0x8);	// FW DIG OFF
			priv->initgain_backup.xaagccore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XAAGCCore1, BitMask);
			priv->initgain_backup.xbagccore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XBAGCCore1, BitMask);
			priv->initgain_backup.xcagccore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XCAGCCore1, BitMask);
			priv->initgain_backup.xdagccore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XDAGCCore1, BitMask);
			BitMask  = bMaskByte2;
			priv->initgain_backup.cca		= (u8)rtl8192_QueryBBReg(dev, rCCK0_CCA, BitMask);

			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xc50 is %x\n",priv->initgain_backup.xaagccore1);
			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xc58 is %x\n",priv->initgain_backup.xbagccore1);
			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xc60 is %x\n",priv->initgain_backup.xcagccore1);
			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xc68 is %x\n",priv->initgain_backup.xdagccore1);
			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xa0a is %x\n",priv->initgain_backup.cca);

			RT_TRACE(COMP_SCAN, "Write scan initial gain = 0x%x \n", initial_gain);
			write_nic_byte(dev, rOFDM0_XAAGCCore1, initial_gain);
			write_nic_byte(dev, rOFDM0_XBAGCCore1, initial_gain);
			write_nic_byte(dev, rOFDM0_XCAGCCore1, initial_gain);
			write_nic_byte(dev, rOFDM0_XDAGCCore1, initial_gain);
			RT_TRACE(COMP_SCAN, "Write scan 0xa0a = 0x%x \n", POWER_DETECTION_TH);
			write_nic_byte(dev, 0xa0a, POWER_DETECTION_TH);
			break;
		case IG_Restore:
			RT_TRACE(COMP_SCAN, "IG_Restore, restore the initial gain.\n");
			BitMask = 0x7f; //Bit0~ Bit6
			if(dm_digtable.dig_algorithm == DIG_ALGO_BY_FALSE_ALARM)
				rtl8192_setBBreg(dev, UFWP, bMaskByte1, 0x8);	// FW DIG OFF
			
			rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, BitMask, (u32)priv->initgain_backup.xaagccore1);
			rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, BitMask, (u32)priv->initgain_backup.xbagccore1);
			rtl8192_setBBreg(dev, rOFDM0_XCAGCCore1, BitMask, (u32)priv->initgain_backup.xcagccore1);
			rtl8192_setBBreg(dev, rOFDM0_XDAGCCore1, BitMask, (u32)priv->initgain_backup.xdagccore1);
			BitMask  = bMaskByte2;
			rtl8192_setBBreg(dev, rCCK0_CCA, BitMask, (u32)priv->initgain_backup.cca);

			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xc50 is %x\n",priv->initgain_backup.xaagccore1);
			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xc58 is %x\n",priv->initgain_backup.xbagccore1);
			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xc60 is %x\n",priv->initgain_backup.xcagccore1);
			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xc68 is %x\n",priv->initgain_backup.xdagccore1);
			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xa0a is %x\n",priv->initgain_backup.cca);

			PHY_SetTxPowerLevel8192S(dev,priv->rtllib->current_network.channel); 

			if(dm_digtable.dig_algorithm == DIG_ALGO_BY_FALSE_ALARM)
				rtl8192_setBBreg(dev, UFWP, bMaskByte1, 0x1);	// FW DIG ON
			break;
		default:
			RT_TRACE(COMP_SCAN, "Unknown IG Operation. \n");
			break;
	}
}

#endif	// #if (DEV_BUS_TYPE == USB_INTERFACE)

//-----------------------------------------------------------------------------
//	Description:
//		Schedule workitem to send specific CMD IO to FW.The FW v.53 and later support 
//		non-polling base CMD scheduling.
//	Added by Roger, 2008.12.03.
//		
//-----------------------------------------------------------------------------
bool HalSetFwCmd8192S(struct net_device* dev, FW_CMD_IO_TYPE	FwCmdIO)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_HIGH_THROUGHPUT	pHTInfo = priv->rtllib->pHTInfo;
	rt_firmware		*pFirmware = priv->pFirmware;
	u32	FwParam = FW_CMD_IO_PARA_QUERY(dev);
	u16	FwCmdMap = FW_CMD_IO_QUERY(dev);
	bool	bPostProcessing = false;

	//
	//Comment out by Roger's suggestion. By Maddest 05132009.
	//
	
	//if(IS_HARDWARE_TYPE_8192SU(Adapter) && Adapter->bInHctTest)
	if(priv->bInHctTest)
		return true;

	RT_TRACE(COMP_CMD, "-->HalSetFwCmd8192S(): Set FW Cmd(%x), SetFwCmdInProgress(%d)\n", (u32)FwCmdIO, priv->SetFwCmdInProgress);

	do{
		if(pFirmware->FirmwareVersion >= 0x35)
		{
			//
			// <Roger_Notes> We re-map to combined FW CMD ones if firmware version 
			// is v.53 or later. 2009.05.08.
			//
			switch(FwCmdIO)
			{
				case FW_CMD_RA_REFRESH_N:
					FwCmdIO = FW_CMD_RA_REFRESH_N_COMB;
					break;
				case FW_CMD_RA_REFRESH_BG:
					FwCmdIO = FW_CMD_RA_REFRESH_BG_COMB;
					break;
				default:
					break;
			}
		}
		else
		{			
			if((FwCmdIO == FW_CMD_IQK_ENABLE) ||
				(FwCmdIO == FW_CMD_RA_REFRESH_N) ||
				(FwCmdIO == FW_CMD_RA_REFRESH_BG))
			{
				bPostProcessing = true;
				break;
			}
		}

		//
		// <Roger_Notes> We shall revise all FW Cmd IO into Reg0x364 DM map table in the future.
		// 2009.05.08.
		//
		switch(FwCmdIO)
		{
		
			case FW_CMD_RA_INIT:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] RA init!!\n");
				FwCmdMap |= FW_RA_INIT_CTL;
				FW_CMD_IO_SET(dev, FwCmdMap);	
				FW_CMD_IO_CLR(dev, FW_RA_INIT_CTL);	 // Clear control flag to sync with FW.
				break;

			case FW_CMD_DIG_DISABLE:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG disable!!\n");
				FwCmdMap &= ~FW_DIG_ENABLE_CTL;
				FW_CMD_IO_SET(dev, FwCmdMap);			
				break;
				
			case FW_CMD_DIG_ENABLE:
			case FW_CMD_DIG_RESUME:	
				if(!(priv->DMFlag & HAL_DM_DIG_DISABLE))
				{
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG enable or resume!!\n");
					FwCmdMap |= (FW_DIG_ENABLE_CTL|FW_SS_CTL);
					FW_CMD_IO_SET(dev, FwCmdMap);	
				}
				break;

			case FW_CMD_DIG_HALT:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG halt!!\n");
				FwCmdMap &= ~(FW_DIG_ENABLE_CTL|FW_SS_CTL);
				FW_CMD_IO_SET(dev, FwCmdMap);				
				break;

			case FW_CMD_TXPWR_TRACK_THERMAL:
				{
					u8	ThermalVal = 0;
					FwCmdMap |= FW_PWR_TRK_CTL;
					FwParam &= FW_PWR_TRK_PARAM_CLR; // Clear FW parameter in terms of thermal parts.
					ThermalVal = priv->ThermalValue;
					FwParam |= ((ThermalVal<<24) |(priv->ThermalMeter[0]<<16));
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set TxPwr tracking!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
					FW_CMD_PARA_SET(dev, FwParam);
					FW_CMD_IO_SET(dev, FwCmdMap);
					FW_CMD_IO_CLR(dev, FW_PWR_TRK_CTL);	 // Clear control flag to sync with FW.
				}
				break;
				
			//
			// <Roger_Notes> The following FW CMDs are only compatible to v.53 or later.
			// 2009.05.08.
			//
			case FW_CMD_RA_REFRESH_N_COMB:
				FwCmdMap |= FW_RA_N_CTL;
				FwCmdMap &= ~(FW_RA_BG_CTL |FW_RA_INIT_CTL);// Clear RA BG mode control.
				FwParam &= FW_RA_PARAM_CLR; // Clear FW parameter in terms of RA parts.
				if(!(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL))
					FwParam |= ((pHTInfo->IOTRaFunc)&0xf);
				FwParam |= ((pHTInfo->IOTPeer & 0xf) <<4);
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set RA/IOT Comb in n mode!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
				FW_CMD_PARA_SET(dev, FwParam);
				FW_CMD_IO_SET(dev, FwCmdMap);	
				FW_CMD_IO_CLR(dev, FW_RA_N_CTL); // Clear control flag to sync with FW.	
				break;	

			case FW_CMD_RA_REFRESH_BG_COMB:				
				FwCmdMap |= FW_RA_BG_CTL;
				FwCmdMap &= ~(FW_RA_N_CTL|FW_RA_INIT_CTL); // Clear RA n-mode control.
				FwParam &= FW_RA_PARAM_CLR; // Clear FW parameter in terms of RA parts.
				if(!(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL))
					FwParam |= ((pHTInfo->IOTRaFunc)&0xf);
				FwParam |= ((pHTInfo->IOTPeer & 0xf) <<4);
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set RA/IOT Comb in BG mode!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
				FW_CMD_PARA_SET(dev, FwParam);
				FW_CMD_IO_SET(dev, FwCmdMap);	
				FW_CMD_IO_CLR(dev, FW_RA_BG_CTL); // Clear control flag to sync with FW.
				break;

			case FW_CMD_IQK_ENABLE:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] IQK enable.\n");
				FwCmdMap |= FW_IQK_CTL;
				FW_CMD_IO_SET(dev, FwCmdMap);	
				FW_CMD_IO_CLR(dev, FW_IQK_CTL); // Clear control flag to sync with FW.
				break;

			//
			// <Roger_Notes> The followed FW Cmds needs post-processing later.
			// 2009.05.08.
			//
			case FW_CMD_RESUME_DM_BY_SCAN:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Resume DM after scan.\n");
				if(priv->DMFlag & (HAL_DM_DIG_DISABLE | HAL_DM_HIPWR_DISABLE))
				{
					// Do Nothing!!
					break;
				}

				if( !(pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_HIGH_POWER))	
				{
					FwCmdMap |= (FW_DIG_ENABLE_CTL|FW_HIGH_PWR_ENABLE_CTL|FW_SS_CTL);
				}
				else
				{
					RT_TRACE(COMP_CMD, "[FW CMD]  [New Version] Resume DIG only!!\n");
					FwCmdMap |= (FW_DIG_ENABLE_CTL|FW_SS_CTL);
					FwCmdMap &= ~FW_HIGH_PWR_ENABLE_CTL;				
				}
				FW_CMD_IO_SET(dev, FwCmdMap);		
				bPostProcessing = true;
				break;

			case FW_CMD_PAUSE_DM_BY_SCAN:
				if(priv->DMFlag & (HAL_DM_DIG_DISABLE | HAL_DM_HIPWR_DISABLE))
				{
					// Do Nothing!!
					break;
				}
				else
				{
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Pause DM before scan.\n");
					FwCmdMap &= ~(FW_DIG_ENABLE_CTL|FW_HIGH_PWR_ENABLE_CTL|FW_SS_CTL);
					FW_CMD_IO_SET(dev, FwCmdMap);		
					bPostProcessing = true;
				}
				break;

			case FW_CMD_HIGH_PWR_DISABLE:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version]  Set HighPwr disable!!\n");
				FwCmdMap &= ~FW_HIGH_PWR_ENABLE_CTL;
				FW_CMD_IO_SET(dev, FwCmdMap);		
				bPostProcessing = true;
				break;
				
			case FW_CMD_HIGH_PWR_ENABLE:	
				if(((pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_HIGH_POWER)==0) &&
					!(priv->DMFlag & HAL_DM_HIPWR_DISABLE))
				{
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set HighPwr enable!!\n");
					FwCmdMap |= (FW_HIGH_PWR_ENABLE_CTL|FW_SS_CTL);
					FW_CMD_IO_SET(dev, FwCmdMap);		
					bPostProcessing = true;
				}
				break;
				
			default:
				bPostProcessing = true; // Pass to original FW CMD processing callback routine.
				break;
		}
	}while(false);


	RT_TRACE(COMP_CMD, "[FW CMD] Current FwCmdMap(%#x)\n", priv->FwCmdIOMap);
	RT_TRACE(COMP_CMD, "[FW CMD] Current FwCmdIOParam(%#x)\n", priv->FwCmdIOParam);

	//
	// <Roger_Notes> We shall post processing these FW CMD if variable bPostProcessing is set.
	// 2009.05.08.
	//
	if(bPostProcessing && !priv->SetFwCmdInProgress)
	{
		if(!priv->up)
		{
			RT_TRACE(COMP_CMD, "SetFwCmdIOTimerCallback(): driver is going to unload\n");
			return false;
		}
		priv->SetFwCmdInProgress = true;
		priv->CurrentFwCmdIO = FwCmdIO; // Update current FW Cmd for callback use.
	}
	else
	{
		return false;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
	queue_work(priv->priv_wq, &priv->FwCmdIOWorkItem);
#else
	schedule_task(&priv->FwCmdIOWorkItem);
#endif
	RT_TRACE(COMP_CMD, "<--HalSetFwCmd8192S(): Set FW Cmd(%x)\n", FwCmdIO);
	//phy_SetFwCmdIOCallback(dev);

	return true;
}

void ChkFwCmdIoDone(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u16 PollingCnt = 1000;		
	u32 tmpValue;
	
	do
	{// Make sure that CMD IO has be accepted by FW.	

		if(!priv->up)
		{
			RT_TRACE(COMP_CMD, "ChkFwCmdIoDone(): USB can NOT IO!!\n");
			return;
		}

		udelay(10);

		tmpValue = read_nic_dword(dev, WFM5);
		if(tmpValue == 0)
		{
			RT_TRACE(COMP_CMD, "[FW CMD] Set FW Cmd success!!\n");
			break;
		}			
		else
		{
			RT_TRACE(COMP_CMD, "[FW CMD] Polling FW Cmd PollingCnt(%d)!!\n", PollingCnt);
		}
	}while( --PollingCnt );

	if(PollingCnt == 0)
	{
		RT_TRACE(COMP_ERR, "[FW CMD] Set FW Cmd fail!!\n");
	}
}
// 	Callback routine of the timer callback for FW Cmd IO.
//
//	Description:
//		This routine will send specific CMD IO to FW and check whether it is done.
//
void phy_SetFwCmdIOCallback(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_HIGH_THROUGHPUT	pHTInfo = priv->rtllib->pHTInfo;
	rt_firmware		*pFirmware = priv->pFirmware;
	u32	input, CurrentAID = 0;;
	//static u32 ScanRegister;

	//if(RT_USB_CANNOT_IO(pAdapter))
	//{
	//	RT_TRACE(COMP_CMD, DBG_WARNING, ("SetFwCmdIOWorkItemCallback(): USB can NOT IO return\n"));
	//	pHalData->SetFwCmdInProgress = false;
	//	return;
	//}
		
	if(!priv->up)
	{
		RT_TRACE(COMP_CMD, "SetFwCmdIOTimerCallback(): driver is going to unload\n");
		return;
	}
	
	RT_TRACE(COMP_CMD, "--->SetFwCmdIOTimerCallback(): Cmd(%#x), SetFwCmdInProgress(%d)\n", priv->CurrentFwCmdIO, priv->SetFwCmdInProgress);

		//
	// <Roger_Notes> We re-map RA related CMD IO to combinational ones 
	// if FW version is v.52 or later. 2009.05.08.
		//
	if(pFirmware->FirmwareVersion >= 0x34)
	{
		switch(priv->CurrentFwCmdIO)
		{
			case FW_CMD_RA_REFRESH_N:
				priv->CurrentFwCmdIO = FW_CMD_RA_REFRESH_N_COMB;
			break;
			case FW_CMD_RA_REFRESH_BG:
				priv->CurrentFwCmdIO = FW_CMD_RA_REFRESH_BG_COMB;
			break;
			default:
			break;
		}
	}

	switch(priv->CurrentFwCmdIO)
	{
		case FW_CMD_RA_RESET:
			RT_TRACE(COMP_CMD,"[FW CMD] Set RA Reset!!\n");
			write_nic_dword(dev, WFM5, FW_RA_RESET);	
			ChkFwCmdIoDone(dev);	
			break;
			
		case FW_CMD_RA_ACTIVE:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA Active!!\n");
			write_nic_dword(dev, WFM5, FW_RA_ACTIVE); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_RA_REFRESH_N:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA n refresh!!\n");
			if(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL)
				input = FW_RA_REFRESH;
			else
				input = FW_RA_REFRESH | (pHTInfo->IOTRaFunc << 8);
			write_nic_dword(dev, WFM5, input); 
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_RA_ENABLE_RSSI_MASK); 
			ChkFwCmdIoDone(dev);	
			break;			
			
		case FW_CMD_RA_REFRESH_BG:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA BG refresh!!\n");
			write_nic_dword(dev, WFM5, FW_RA_REFRESH); 
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_RA_DISABLE_RSSI_MASK); 			
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_RA_REFRESH_N_COMB:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA n Combo refresh!!\n");
			if(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL)
				input = FW_RA_IOT_N_COMB;
			else
				input = FW_RA_IOT_N_COMB | (((pHTInfo->IOTRaFunc)&0x0f) << 8);
			input = input |((pHTInfo->IOTPeer & 0xf) <<12);
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA/IOT Comb in n mode!! input(%#x)\n", input);
			write_nic_dword(dev, WFM5, input); 			
			ChkFwCmdIoDone(dev);	
			break;		

		case FW_CMD_RA_REFRESH_BG_COMB:				
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA B/G Combo refresh!!\n");
			if(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL)
				input = FW_RA_IOT_BG_COMB;
			else
				input = FW_RA_IOT_BG_COMB | (((pHTInfo->IOTRaFunc)&0x0f) << 8);
			input = input |((pHTInfo->IOTPeer & 0xf) <<12);
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA/IOT Comb in B/G mode!! input(%#x)\n", input);
			write_nic_dword(dev, WFM5, input); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_IQK_ENABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] IQK Enable!!\n");
			write_nic_dword(dev, WFM5, FW_IQK_ENABLE); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_TXPWR_TRACK_ENABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] TxPwr tracking Enable!!\n");
			write_nic_dword(dev, WFM5, FW_TXPWR_TRACK_ENABLE); 
			ChkFwCmdIoDone(dev);
			break;

		case FW_CMD_TXPWR_TRACK_DISABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] TxPwr tracking Disable!!\n");
			write_nic_dword(dev, WFM5, FW_TXPWR_TRACK_DISABLE); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_PAUSE_DM_BY_SCAN:
			RT_TRACE(COMP_CMD,"[FW CMD] Pause DM by Scan!!\n");
			//Lower initial gain
			rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, bMaskByte0, 0x17);
			// CCA threshold
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x40);
			break;
		
		case FW_CMD_RESUME_DM_BY_SCAN:			
			RT_TRACE(COMP_CMD, "[FW CMD] Resume DM by Scan!!\n");
			// CCA threshold
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x83);
			PHY_SetTxPowerLevel8192S(dev, priv->chan);
			break;
	
		case FW_CMD_HIGH_PWR_DISABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] High Pwr Disable!!\n");
			if(priv->DMFlag & HAL_DM_HIPWR_DISABLE)
				break;
			//Lower initial gain
			rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, bMaskByte0, 0x17);
			// CCA threshold
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x40);
			break;
			
		case FW_CMD_HIGH_PWR_ENABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] High Pwr Enable!!\n");
			if(priv->DMFlag & HAL_DM_HIPWR_DISABLE)
				break;
			// CCA threshold
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x83);	
			break;

		case FW_CMD_LPS_ENTER:
			RT_TRACE(COMP_CMD, "[FW CMD] Enter LPS mode!!\n");
			CurrentAID = priv->rtllib->assoc_id;
			write_nic_dword(dev, WFM5, (FW_LPS_ENTER| ((CurrentAID|0xc000)<<8))    );
			ChkFwCmdIoDone(dev);	
			// FW set TXOP disable here, so disable EDCA turbo mode until driver leave LPS
			pHTInfo->IOTAction |=  HT_IOT_ACT_DISABLE_EDCA_TURBO;
			break;

		case FW_CMD_LPS_LEAVE:
			RT_TRACE(COMP_CMD, "[FW CMD] Leave LPS mode!!\n");
			write_nic_dword(dev, WFM5, FW_LPS_LEAVE );
			ChkFwCmdIoDone(dev);	
			pHTInfo->IOTAction &=  (~HT_IOT_ACT_DISABLE_EDCA_TURBO);
			break;
		
		default:			
			break;
	}
	
	priv->SetFwCmdInProgress = false;// Clear FW CMD operation flag.
	RT_TRACE(COMP_CMD, "<---SetFwCmdIOWorkItemCallback()\n");

}

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void SetFwCmdIOWorkItemCallback(struct work_struct * work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, FwCmdIOWorkItem);
        struct net_device *dev = priv->rtllib->dev;
#else
void SetFwCmdIOWorkItemCallback(struct net_device *dev)
{
        struct r8192_priv *priv = rtllib_priv(dev);
#endif

	if( !priv->up || priv->bResetInProgress){
		RT_TRACE(COMP_CMD, "SetFwCmdIOWorkItemCallback(): USB can NOT IO return\n");
		priv->SetFwCmdInProgress = false;
		return;
	}

	phy_SetFwCmdIOCallback(dev);
}

//**************************************************************
					//follwing is for Radio on/off
//**************************************************************
void
MlmeDisassociateRequest(
	struct net_device* dev,
	u8* 		asSta,
	u8		asRsn
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8 i;

	RemovePeerTS(priv->rtllib, asSta);
	
	SendDisassociation( priv->rtllib, 0, asRsn );
	
	if(memcpy(priv->rtllib->current_network.bssid,asSta,6) == 0)
	{
		//ShuChen TODO: change media status.
		//ShuChen TODO: What to do when disassociate.
		priv->rtllib->state = RTLLIB_NOLINK;
		//pMgntInfo->AsocTimestamp = 0;
		for(i=0;i<6;i++)  priv->rtllib->current_network.bssid[i] = 0x22;
//		pMgntInfo->mBrates.Length = 0;
//		dev->HalFunc.SetHwRegHandler( dev, HW_VAR_BASIC_RATE, (pu1Byte)(&pMgntInfo->mBrates) );
		priv->OpMode = RT_OP_MODE_NO_LINK;
		{
			RT_OP_MODE	OpMode = priv->OpMode;
			LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
			//u8 btMsr = read_nic_byte(dev, MSR);
			u8 	btMsr 	= MSR_NOLINK;

			btMsr &= 0xfc;

			switch(OpMode)
			{
			case RT_OP_MODE_INFRASTRUCTURE:
				btMsr |= MSR_LINK_MANAGED;
				//LedAction = LED_CTL_LINK;
				break;

			case RT_OP_MODE_IBSS:
				btMsr |= MSR_LINK_ADHOC;
				// led link set seperate
				break;

			case RT_OP_MODE_AP:
				btMsr |= MSR_LINK_MASTER;
				//LedAction = LED_CTL_LINK;
				break;

			default:
				btMsr |= MSR_LINK_NONE;
				break;
			}

			write_nic_byte(dev, MSR, btMsr);

			// LED control
			priv->rtllib->LedControlHandler(dev, LedAction);
		}
		rtllib_disassociate(priv->rtllib);

		write_nic_word(dev, BSSIDR, ((u16*)priv->rtllib->current_network.bssid)[0]);
		write_nic_dword(dev, BSSIDR+2, ((u32*)(priv->rtllib->current_network.bssid+2))[0]);
		
	}

}


void
MgntDisconnectIBSS(
	struct net_device* dev
) 
{
	struct r8192_priv *priv = rtllib_priv(dev);
	//RT_OP_MODE	OpMode;
	u8			i;
	bool	bFilterOutNonAssociatedBSSID = false;

	//RTLLIB_DEBUG(RTLLIB_DL_TRACE, "XXXXXXXXXX MgntDisconnect IBSS\n");

	priv->rtllib->state = RTLLIB_NOLINK;

//	PlatformZeroMemory( pMgntInfo->Bssid, 6 );
	for(i=0;i<6;i++)  priv->rtllib->current_network.bssid[i]= 0x55;
	
	priv->OpMode = RT_OP_MODE_NO_LINK;

	//Stop Beacon.
	write_nic_word(dev, BSSIDR, ((u16*)priv->rtllib->current_network.bssid)[0]);
	write_nic_dword(dev, BSSIDR+2, ((u32*)(priv->rtllib->current_network.bssid+2))[0]);	
	{
			RT_OP_MODE	OpMode = priv->OpMode;
			LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
			//u8	btMsr = read_nic_byte(dev, MSR);
			u8	btMsr = MSR_NOLINK;

			btMsr &= 0xfc;

			switch(OpMode)
			{
			case RT_OP_MODE_INFRASTRUCTURE:
				btMsr |= MSR_LINK_MANAGED;
				//LedAction = LED_CTL_LINK;
				break;

			case RT_OP_MODE_IBSS:
				btMsr |= MSR_LINK_ADHOC;
				// led link set seperate
				break;

			case RT_OP_MODE_AP:
				btMsr |= MSR_LINK_MASTER;
				//LedAction = LED_CTL_LINK;
				break;

			default:
				btMsr |= MSR_LINK_NONE;
				break;
			}

			write_nic_byte(dev, MSR, btMsr);

			// LED control
			priv->rtllib->LedControlHandler(dev, LedAction);
	}
	rtllib_stop_send_beacons(priv->rtllib);

#ifdef TO_DI_LIST //we use hardware beacon now
	// Stop SW Beacon.
	if(pMgntInfo->bEnableSwBeaconTimer)
	{
		// SwBeaconTimer will stop if pMgntInfo->mIbss==false, see SwBeaconCallback() for details.
//#if DEV_BUS_TYPE==USB_INTERFACE
		PlatformCancelTimer( Adapter, &pMgntInfo->SwBeaconTimer);
//#endif
	}
#endif

	// If disconnect, clear RCR CBSSID bit
	bFilterOutNonAssociatedBSSID = false;
	{
			u32 Type;
			Type = bFilterOutNonAssociatedBSSID;

			if (Type == true)
				priv->ReceiveConfig |= (RCR_CBSSID);
			else
				priv->ReceiveConfig &= (~RCR_CBSSID);

			write_nic_dword(dev, RCR,priv->ReceiveConfig);
			
	}
	//MgntIndicateMediaStatus( dev, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );
	notify_wx_assoc_event(priv->rtllib);

}

void
MgntDisconnectAP(
	struct net_device* dev,
	u8 asRsn
) 
{
	struct r8192_priv *priv = rtllib_priv(dev);
	bool bFilterOutNonAssociatedBSSID = false;

//
// Commented out by rcnjko, 2005.01.27: 
// I move SecClearAllKeys() to MgntActSet_802_11_DISASSOCIATE().
//
//	//2004/09/15, kcwu, the key should be cleared, or the new handshaking will not success
//	SecClearAllKeys(dev);

	// In WPA WPA2 need to Clear all key ... because new key will set after new handshaking.
#ifdef TO_DO
	if(   pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch ||
		(pMgntInfo->bAPSuportCCKM && pMgntInfo->bCCX8021xenable) )	// In CCKM mode will Clear key
	{
		SecClearAllKeys(dev);
		RT_TRACE(COMP_SEC, DBG_LOUD,("======>CCKM clear key..."))
	}
#endif

	// If disconnect, clear RCR CBSSID bit
	bFilterOutNonAssociatedBSSID = false;
	{
			u32 Type;
			Type = bFilterOutNonAssociatedBSSID;

			
			if (Type == true)
				priv->ReceiveConfig |= (RCR_CBSSID);
			else 
				priv->ReceiveConfig &= (~RCR_CBSSID);

			write_nic_dword(dev, RCR,priv->ReceiveConfig);	
	}

	// 2004.10.11, by rcnjko.
	//MlmeDisassociateRequest( dev, pMgntInfo->Bssid, disas_lv_ss );
	MlmeDisassociateRequest( dev, priv->rtllib->current_network.bssid, asRsn );

	priv->rtllib->state = RTLLIB_NOLINK;
	//pMgntInfo->AsocTimestamp = 0;
}

bool
MgntDisconnect(
	struct net_device* dev,
	u8 asRsn
) 
{
	struct r8192_priv *priv = rtllib_priv(dev);

	//
	// Schedule an workitem to wake up for ps mode, 070109, by rcnjko.
	//
#ifdef TO_DO
	if(pMgntInfo->mPss != eAwake)
	{
		// 
		// Using AwkaeTimer to prevent mismatch ps state.
		// In the timer the state will be changed according to the RF is being awoke or not. By Bruce, 2007-10-31. 
		//
		// PlatformScheduleWorkItem( &(pMgntInfo->AwakeWorkItem) );
		PlatformSetTimer( dev, &(pMgntInfo->AwakeTimer), 0 );
	}
#endif
	// Follow 8180 AP mode, 2005.05.30, by rcnjko.
#ifdef TO_DO
	if(pMgntInfo->mActingAsAp)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnect() ===> AP_DisassociateAllStation\n"));
		AP_DisassociateAllStation(dev, unspec_reason);
		return true;
	}	
#endif
	// Indication of disassociation event.
	//DrvIFIndicateDisassociation(dev, asRsn);

	// In adhoc mode, update beacon frame.
	if( priv->rtllib->state == RTLLIB_LINKED )
	{
		if( priv->rtllib->iw_mode == IW_MODE_ADHOC )
		{
			//RT_TRACE(COMP_MLME, "MgntDisconnect() ===> MgntDisconnectIBSS\n");
			MgntDisconnectIBSS(dev);
		}
		if( priv->rtllib->iw_mode == IW_MODE_INFRA )
		{
			// We clear key here instead of MgntDisconnectAP() because that  
			// MgntActSet_802_11_DISASSOCIATE() is an interface called by OS, 
			// e.g. OID_802_11_DISASSOCIATE in Windows while as MgntDisconnectAP() is 
			// used to handle disassociation related things to AP, e.g. send Disassoc
			// frame to AP.  2005.01.27, by rcnjko.
			//RTLLIB_DEBUG(RTLLIB_DL_TRACE,"MgntDisconnect() ===> MgntDisconnectAP\n");
			MgntDisconnectAP(dev, asRsn);
		}

		// Inidicate Disconnect, 2005.02.23, by rcnjko.
		//MgntIndicateMediaStatus( dev, RT_MEDIA_DISCONNECT, GENERAL_INDICATE); 
		notify_wx_assoc_event(priv->rtllib);
	}

	return true;
}

//
//
//	Description: 
//		Chang RF Power State.
//		Note that, only MgntActSet_RF_State() is allowed to set HW_VAR_RF_STATE.
//
//	Assumption:
//		PASSIVE LEVEL.
//	form MgntActSetParam.c
bool
MgntActSet_RF_State(
	struct net_device* dev,
	RT_RF_POWER_STATE	StateToSet,
	RT_RF_CHANGE_SOURCE ChangeSource
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	bool 			bActionAllowed = false; 
	bool 			bConnectBySSID = false;
	RT_RF_POWER_STATE	rtState;
	u16					RFWaitCounter = 0;
	unsigned long flag;
	RT_TRACE(COMP_POWER, "===>MgntActSet_RF_State(): StateToSet(%d)\n",StateToSet);
	printk("===>MgntActSet_RF_State(): StateToSet(%d)\n",StateToSet);

	//1//
	//1//<1>Prevent the race condition of RF state change. 
	//1//
	// Only one thread can change the RF state at one time, and others should wait to be executed. By Bruce, 2007-11-28.

	while(true)
	{
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		if(priv->RFChangeInProgress)
		{
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
			RT_TRACE(COMP_POWER, "MgntActSet_RF_State(): RF Change in progress! Wait to set..StateToSet(%d).\n", StateToSet);
			
			// Set RF after the previous action is done. 
			while(priv->RFChangeInProgress)
			{
				RFWaitCounter ++;
				RT_TRACE(COMP_POWER, "MgntActSet_RF_State(): Wait 1 ms (%d times)...\n", RFWaitCounter);
				udelay(1000); // 1 ms

				// Wait too long, return false to avoid to be stuck here.
				if(RFWaitCounter > 100)
				{
					RT_TRACE(COMP_ERR, "MgntActSet_RF_State(): Wait too logn to set RF\n");
					// TODO: Reset RF state?
					return false;
				}
			}
		}
		else
		{
			priv->RFChangeInProgress = true;
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
			break;
		}
	}

	rtState = priv->rtllib->eRFPowerState;

	switch(StateToSet) 
	{
	case eRfOn:
		//
		// Turn On RF no matter the IPS setting because we need to update the RF state to Ndis under Vista, or
		// the Windows does not allow the driver to perform site survey any more. By Bruce, 2007-10-02. 
		//

		priv->rtllib->RfOffReason &= (~ChangeSource);

		if(! priv->rtllib->RfOffReason)
		{
			priv->rtllib->RfOffReason = 0;
			bActionAllowed = true;

			
			if(rtState == eRfOff && ChangeSource >=RF_CHANGE_BY_HW )
			{
				bConnectBySSID = true;
			}
		}
		else{
			RT_TRACE(COMP_POWER, "MgntActSet_RF_State - eRfon reject pMgntInfo->RfOffReason= 0x%x, ChangeSource=0x%X\n", priv->rtllib->RfOffReason, ChangeSource);
			printk("MgntActSet_RF_State - eRfon reject pMgntInfo->RfOffReason= 0x%x, ChangeSource=0x%X\n", priv->rtllib->RfOffReason, ChangeSource);
                }

		break;

	case eRfOff:

			if (priv->rtllib->RfOffReason > RF_CHANGE_BY_IPS)
			{
				//
				// 060808, Annie: 
				// Disconnect to current BSS when radio off. Asked by QuanTa.
				//
				// Set all link status falg, by Bruce, 2007-06-26.
				//MgntActSet_802_11_DISASSOCIATE( dev, disas_lv_ss );			
				MgntDisconnect(dev, disas_lv_ss);
	
	
				// Clear content of bssDesc[] and bssDesc4Query[] to avoid reporting old bss to UI. 
				// 2007.05.28, by shien chang.
				
			}


		priv->rtllib->RfOffReason |= ChangeSource;
		bActionAllowed = true;
		break;

	case eRfSleep:
		priv->rtllib->RfOffReason |= ChangeSource;
		bActionAllowed = true;
		break;

	default:
		break;
	}

	if(bActionAllowed)
	{
		RT_TRACE(COMP_POWER, "MgntActSet_RF_State(): Action is allowed.... StateToSet(%d), RfOffReason(%#X)\n", StateToSet, priv->rtllib->RfOffReason);
				// Config HW to the specified mode.
		PHY_SetRFPowerState(dev, StateToSet);	
		// Turn on RF.
		if(StateToSet == eRfOn) 
		{				
			//dev->HalFunc.HalEnableRxHandler(dev);
			if(bConnectBySSID)
			{				
				//MgntActSet_802_11_SSID(dev, dev->MgntInfo.Ssid.Octet, dev->MgntInfo.Ssid.Length, true );
			}
		}
		// Turn off RF.
		else if(StateToSet == eRfOff)
		{		
			//dev->HalFunc.HalDisableRxHandler(dev);
		}		
	}
	else
	{
		RT_TRACE(COMP_POWER, "MgntActSet_RF_State(): Action is rejected.... StateToSet(%d), ChangeSource(%#X), RfOffReason(%#X)\n", StateToSet, ChangeSource, priv->rtllib->RfOffReason);
	}

	// Release RF spinlock
	spin_lock_irqsave(&priv->rf_ps_lock,flag);
	priv->RFChangeInProgress = false;
	spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
	
	RT_TRACE(COMP_POWER, "<===MgntActSet_RF_State()\n");
	return bActionAllowed;
}

