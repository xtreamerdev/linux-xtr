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
 * Jerry chuang <wlanfae@realtek.com>
******************************************************************************/
// ****************************************************************************
// 
// History:
// Data			Who		Remark
// 
// 09/25/2008	MHC		Create initial version.
// 11/05/2008 	MHC		Add API for tw power setting.
// 
// 	
// ***************************************************************************/
#include "r8192U.h"
#include "r8192S_rtl6052.h"

#ifdef RTL8192SU
#include "r8192S_hw.h"
#include "r8192S_phyreg.h"
#include "r8192S_phy.h"
#else
#include "r8192U_hw.h"
#include "r819xU_phyreg.h"
#include "r819xU_phy.h"
#endif


//---------------------------Define Local Constant---------------------------*/
// Define local structure for debug!!!!!
typedef struct RF_Shadow_Compare_Map {
	// Shadow register value
	u32		Value;
	// Compare or not flag
	u8		Compare;
	// Record If it had ever modified unpredicted
	u8		ErrorOrNot;
	// Recorver Flag
	u8		Recorver;
	//
	u8		Driver_Write;
}RF_SHADOW_T;
//---------------------------Define Local Constant---------------------------*/


//------------------------Define global variable-----------------------------*/
//------------------------Define global variable-----------------------------*/




//---------------------Define local function prototype-----------------------*/
void phy_RF6052_Config_HardCode(struct net_device* dev);

RT_STATUS phy_RF6052_Config_ParaFile(struct net_device* dev);
//---------------------Define local function prototype-----------------------*/

//------------------------Define function prototype--------------------------*/
extern void RF_ChangeTxPath(struct net_device* dev,  u16 DataRate);

//------------------------Define function prototype--------------------------*/

//------------------------Define local variable------------------------------*/
// 2008/11/20 MH For Debug only, RF
static	RF_SHADOW_T	RF_Shadow[RF6052_MAX_PATH][RF6052_MAX_REG];// = {{0}};//FIXLZM
//------------------------Define local variable------------------------------*/

//------------------------Define function prototype--------------------------*/
//-----------------------------------------------------------------------------
// Function:	RF_ChangeTxPath
//
// Overview:	For RL6052, we must change some RF settign for 1T or 2T.
//
// Input:		u16 DataRate		// 0x80-8f, 0x90-9f
//
// Output:      NONE
//
// Return:      NONE
//
// Revised History:
// When			Who		Remark
// 09/25/2008 	MHC		Create Version 0.
//						Firmwaer support the utility later.
//
//---------------------------------------------------------------------------*/
extern void RF_ChangeTxPath(struct net_device* dev,  u16 DataRate)
{
// We do not support gain table change inACUT now !!!! Delete later !!!
#if 0//(RTL92SE_FPGA_VERIFY == 0)
	static	u1Byte	RF_Path_Type = 2;	// 1 = 1T 2= 2T			
	static	u4Byte	tx_gain_tbl1[6] 
			= {0x17f50, 0x11f40, 0x0cf30, 0x08720, 0x04310, 0x00100};
	static	u4Byte	tx_gain_tbl2[6] 
			= {0x15ea0, 0x10e90, 0x0c680, 0x08250, 0x04040, 0x00030};
	u1Byte	i;
	
	if (RF_Path_Type == 2 && (DataRate&0xF) <= 0x7)
	{
		// Set TX SYNC power G2G3 loop filter
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G2, bMask20Bits, 0x0f000);
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G3, bMask20Bits, 0xeacf1);

		// Change TX AGC gain table
		for (i = 0; i < 6; i++)					
			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
						RF_TX_AGC, bMask20Bits, tx_gain_tbl1[i]);

		// Set PA to high value
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G2, bMask20Bits, 0x01e39);
	}
	else if (RF_Path_Type == 1 && (DataRate&0xF) >= 0x8)
	{
		// Set TX SYNC power G2G3 loop filter
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G2, bMask20Bits, 0x04440);
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G3, bMask20Bits, 0xea4f1);

		// Change TX AGC gain table
		for (i = 0; i < 6; i++)
			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
						RF_TX_AGC, bMask20Bits, tx_gain_tbl2[i]);

		// Set PA low gain
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G2, bMask20Bits, 0x01e19);
	}
#endif	
	
}	// RF_ChangeTxPath */


//-----------------------------------------------------------------------------
// Function:    PHY_RF6052SetBandwidth()
//
// Overview:    This function is called by SetBWModeCallback8190Pci() only
//
// Input:       PADAPTER				Adapter
//			WIRELESS_BANDWIDTH_E	Bandwidth	//20M or 40M
//
// Output:      NONE
//
// Return:      NONE
//
// Note:		For RF type 0222D
//---------------------------------------------------------------------------*/
void PHY_RF6052SetBandwidth(struct net_device* dev, HT_CHANNEL_WIDTH Bandwidth)	//20M or 40M
{	
	//u8				eRFPath;	
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	

	//if (priv->card_8192 == NIC_8192SE)
#ifdef RTL8192SU  //YJ,test,090113	
	{		
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:
				//rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, RF_CHNLBW, BIT10|BIT11, 0x01);

				priv->RfRegChnlVal[0] = ((priv->RfRegChnlVal[0] & 0xfffff3ff) | 0x0400);
				rtl8192_phy_SetRFReg(dev, RF90_PATH_A, RF_CHNLBW, bRFRegOffsetMask, priv->RfRegChnlVal[0]);

				break;
			case HT_CHANNEL_WIDTH_20_40:
				//rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, RF_CHNLBW, BIT10|BIT11, 0x00);
				priv->RfRegChnlVal[0] = ((priv->RfRegChnlVal[0] & 0xfffff3ff));
				rtl8192_phy_SetRFReg(dev, RF90_PATH_A, RF_CHNLBW, bRFRegOffsetMask, priv->RfRegChnlVal[0]);

				break;
			default:
				RT_TRACE(COMP_DBG, "PHY_SetRF6052Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth);
				break;			
		}
	}
//	else
#else
	{
	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:
					//PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)RF90_PATH_A, RF_CHNLBW, (BIT10|BIT11), 0x01);				
				break;
			case HT_CHANNEL_WIDTH_20_40:
					//PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)RF90_PATH_A, RF_CHNLBW, (BIT10|BIT11), 0x00);
				break;
			default:
					RT_TRACE(COMP_DBG, "PHY_SetRF8225Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth );
				break;
				
		}
	}
	}
#endif
}


//-----------------------------------------------------------------------------
// Function:	PHY_RF6052SetCckTxPower
//
// Overview:	
//
// Input:       NONE
//
// Output:      NONE
//
// Return:      NONE
//
// Revised History:
// When			Who		Remark
// 11/05/2008 	MHC		Simulate 8192series..
//
//---------------------------------------------------------------------------*/
extern void PHY_RF6052SetCckTxPower(struct net_device* dev, u8	powerlevel)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32				TxAGC=0;
	bool			TurboScanOff=false;

	if ((priv->EEPROMVersion >= 2) && (priv->EEPROMRegulatory != 0))
		TurboScanOff = true;

	if(priv->ieee80211->scanning == 1 || priv->ieee80211->be_scan_inprogress == true)
		TxAGC = 0x3f;
	else
		TxAGC = powerlevel;

	if(TurboScanOff)
		TxAGC = powerlevel;

	//cosa add for lenovo, to pass the safety spec, don't increase power index for different rates.
	if(priv->bIgnoreDiffRateTxPowerOffset)
		TxAGC = powerlevel;

	if(TxAGC > RF6052_MAX_TX_PWR)
		TxAGC = RF6052_MAX_TX_PWR;

	//printk("CCK PWR= %x\n", TxAGC);
	rtl8192_setBBreg(dev, rTxAGC_CCK_Mcs32, bTxAGCRateCCK, TxAGC);

}	/* PHY_RF6052SetCckTxPower */


//
// powerbase0 for OFDM rates
// powerbase1 for HT MCS rates
//
void getPowerBase(
	struct net_device* dev,
	u8*		pPowerLevel,
	u8		Channel,
	u32*	OfdmBase,
	u32*	MCSBase,
	u8*		pFinalPowerIndex
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32			powerBase0, powerBase1;
	u32			Legacy_pwrdiff=0, HT20_pwrdiff=0;
	u8			i, powerlevel[4];

	for(i=0; i<2; i++)
		powerlevel[i] = pPowerLevel[i];
	// We only care about the path A for legacy.	
	if (priv->EEPROMVersion < 2)
		powerBase0 = powerlevel[0] + (priv->LegacyHTTxPowerDiff & 0xf); 
	else if (priv->EEPROMVersion >= 2)
	{
		Legacy_pwrdiff = priv->TxPwrLegacyHtDiff[RF90_PATH_A][Channel-1];
		// For legacy OFDM, tx pwr always > HT OFDM pwr. We do not care Path B
		// legacy OFDM pwr diff. NO BB register to notify HW.
		powerBase0 = powerlevel[0] + Legacy_pwrdiff; 
		//RTPRINT(FPHY, PHY_TXPWR, (" [LagacyToHT40 pwr diff = %d]\n", Legacy_pwrdiff));		
	}
	powerBase0 = (powerBase0<<24) | (powerBase0<<16) |(powerBase0<<8) |powerBase0;
	*OfdmBase = powerBase0;
	//printk(" [OFDM power base index = 0x%lx]\n", powerBase0);
	
	//MCS rates
	if(priv->EEPROMVersion >= 2)
	{	
		//Check HT20 to HT40 diff		
		if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
		{
			for(i=0; i<2; i++)	// rf-A, rf-B
			{	// HT 20<->40 pwr diff
				HT20_pwrdiff = priv->TxPwrHt20Diff[i][Channel-1];
				if (HT20_pwrdiff < 8)	// 0~+7
					powerlevel[i] += HT20_pwrdiff;	
				else				// index8-15=-8~-1
					powerlevel[i] -= (16-HT20_pwrdiff);
				//RTPRINT(FPHY, PHY_TXPWR, (" [HT20 to HT40 pwrdiff = %d]\n", HT20_pwrdiff));
			}
		}
	}
	powerBase1 = powerlevel[0];	//use index of rf-A
	powerBase1 = (powerBase1<<24) | (powerBase1<<16) |(powerBase1<<8) |powerBase1;
	*MCSBase = powerBase1;
	
	//printk(" [MCS power base index = 0x%x]\n", powerBase1);
	//RTPRINT(FPHY, PHY_TXPWR, (" [Legacy/HT power index= %lx/%lx]\n", powerBase0, powerBase1));

	//The following is for Antenna diff from Ant-B to Ant-A
	pFinalPowerIndex[0] = powerlevel[0];
	pFinalPowerIndex[1] = powerlevel[1];
	switch(priv->EEPROMRegulatory)
		{
		case 3:
			//The following is for calculation of the power diff for Ant-B to Ant-A.
			if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
			{
				pFinalPowerIndex[0] += priv->PwrGroupHT40[RF90_PATH_A][Channel-1];
				pFinalPowerIndex[1] += priv->PwrGroupHT40[RF90_PATH_B][Channel-1];
			}
			else
			{
				pFinalPowerIndex[0] += priv->PwrGroupHT20[RF90_PATH_A][Channel-1];
				pFinalPowerIndex[1] += priv->PwrGroupHT20[RF90_PATH_B][Channel-1];
			}
			break;
		default:
			break;
	}
	if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
	{
		RT_TRACE(COMP_RF, "40MHz finalPowerIndex (A / B) = 0x%x / 0x%x\n", pFinalPowerIndex[0], pFinalPowerIndex[1]);
	}
	else
	{
		RT_TRACE(COMP_RF, "20MHz finalPowerIndex (A / B) = 0x%x / 0x%x\n", pFinalPowerIndex[0], pFinalPowerIndex[1]);
	}
}
	
void getTxPowerWriteValByRegulatory(
	struct net_device* dev,
	u8		Channel,
	u8		index,
	u32		powerBase0,
	u32		powerBase1,
	u32*		pOutWriteVal
	)
	{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//u16 RegOffset[6] = {0xe00, 0xe04, 0xe10, 0xe14, 0xe18, 0xe1c};
	u8	i, chnlGroup = 0, pwr_diff_limit[4];
	u32 	writeVal, customer_limit;
	
		//
		// Index 0 & 1= legacy OFDM, 2-5=HT_MCS rate
		//		
	switch(priv->EEPROMRegulatory)
	{
		case 0:	// Realtek better performance
				// increase power diff defined by Realtek for large power
			chnlGroup = 0;
			//RTPRINT(FPHY, PHY_TXPWR, ("MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n", 
			//	chnlGroup, index, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index]));
			writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index] + 
				((index<2)?powerBase0:powerBase1);
			//RTPRINT(FPHY, PHY_TXPWR, ("RTK better performance, writeVal = 0x%x\n", writeVal));
			break;
		case 1:	// Realtek regulatory
				// increase power diff defined by Realtek for regulatory
			if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
			{
			writeVal = ((index<2)?powerBase0:powerBase1);
				//RTPRINT(FPHY, PHY_TXPWR, ("Realtek regulatory, 40MHz, writeVal = 0x%x\n", writeVal));
			}
			else
			{
				if(priv->pwrGroupCnt == 1)
					chnlGroup = 0;
				if(priv->pwrGroupCnt >= 3)
				{
					if(Channel <= 3)
						chnlGroup = 0;
					else if(Channel >= 4 && Channel <= 8)
						chnlGroup = 1;
					else if(Channel > 8)
						chnlGroup = 2;
					if(priv->pwrGroupCnt == 4)
						chnlGroup++;
				}
				//RTPRINT(FPHY, PHY_TXPWR, ("MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n", 
				//chnlGroup, index, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index]));
				writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index] + 
						((index<2)?powerBase0:powerBase1);
				//RTPRINT(FPHY, PHY_TXPWR, ("Realtek regulatory, 20MHz, writeVal = 0x%x\n", writeVal));
			}
			break;
		case 2:	// Better regulatory
				// don't increase any power diff
			writeVal = ((index<2)?powerBase0:powerBase1);
			//RTPRINT(FPHY, PHY_TXPWR, ("Better regulatory, writeVal = 0x%x\n", writeVal));
			break;
		case 3:	// Customer defined power diff.
				// increase power diff defined by customer.
			chnlGroup = 0;
			//RTPRINT(FPHY, PHY_TXPWR, ("MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n", 
			//	chnlGroup, index, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index]));

			if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
			{
				//RTPRINT(FPHY, PHY_TXPWR, ("customer's limit, 40MHz = 0x%x\n", 
				//	priv->PwrGroupHT40[RF90_PATH_A][Channel-1]));
			}
			else
			{
				//RTPRINT(FPHY, PHY_TXPWR, ("customer's limit, 20MHz = 0x%x\n", 
				//	priv->PwrGroupHT20[RF90_PATH_A][Channel-1]));
			}
			for (i=0; i<4; i++)
			{
				pwr_diff_limit[i] = (u8)((priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index]&(0x7f<<(i*8)))>>(i*8));
				//RTPRINT(FPHY, PHY_TXPWR, ("pwr_diff_limit[%d] = 0x%x\n", i, pwr_diff_limit[i]));
				if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
				{
					if(pwr_diff_limit[i] > priv->PwrGroupHT40[RF90_PATH_A][Channel-1])
					{
						pwr_diff_limit[i] = priv->PwrGroupHT40[RF90_PATH_A][Channel-1];
						//RTPRINT(FPHY, PHY_TXPWR, ("Use customer's limit, 40MHz, pwr_diff_limit[%d] = 0x%x\n", i, pwr_diff_limit[i]));
					}
				}
				else
				{
					if(pwr_diff_limit[i] > priv->PwrGroupHT20[RF90_PATH_A][Channel-1])
					{
						pwr_diff_limit[i] = priv->PwrGroupHT20[RF90_PATH_A][Channel-1];
						//RTPRINT(FPHY, PHY_TXPWR, ("Use customer's limit, 20MHz, pwr_diff_limit[%d] = 0x%x\n", i, pwr_diff_limit[i]));
					}
				}
			}
			customer_limit = (pwr_diff_limit[3]<<24) | (pwr_diff_limit[2]<<16) |
							(pwr_diff_limit[1]<<8) | (pwr_diff_limit[0]);
			//RTPRINT(FPHY, PHY_TXPWR, ("Customer's limit = 0x%x\n", customer_limit));

			writeVal = customer_limit + ((index<2)?powerBase0:powerBase1);
			//RTPRINT(FPHY, PHY_TXPWR, ("Customer, writeVal = 0x%x\n", writeVal));
			break;
		default:
			chnlGroup = 0;
			writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index] + 
				((index<2)?powerBase0:powerBase1);
			//RTPRINT(FPHY, PHY_TXPWR, ("RTK better performance, writeVal = 0x%x\n", writeVal));
			break;
	}

	*pOutWriteVal = writeVal;

	//RTPRINT(FPHY, PHY_TXPWR, ("Reg 0x%x, chnlGroup = %d, Original=%x writeVal=%x\n", 
		//RegOffset[index], chnlGroup, pHalData->MCSTxPowerLevelOriginalOffset[chnlGroup][index], 
		//writeVal));
}

void setAntennaDiff(
	struct net_device* dev,
	u8*		pFinalPowerIndex
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	s8	ant_pwr_diff=0;
	u32	u4RegValue=0;

	if (priv->rf_type== RF_2T2R)
	{
		ant_pwr_diff = pFinalPowerIndex[1] - pFinalPowerIndex[0];
		
		// range is from 7~-8, index = 0x0~0xf
		if(ant_pwr_diff > 7)
			ant_pwr_diff = 7;
		if(ant_pwr_diff < -8)
			ant_pwr_diff = -8;
		//RTPRINT(FPHY, PHY_TXPWR, ("Antenna Diff from RF-B to RF-A = %d (0x%x)\n", 
		//	ant_pwr_diff, ant_pwr_diff&0xf));
		ant_pwr_diff &= 0xf;
	}
	// Antenna TX power difference
	priv->AntennaTxPwDiff[2] = 0;// RF-D, don't care
	priv->AntennaTxPwDiff[1] = 0;// RF-C, don't care
	priv->AntennaTxPwDiff[0] = (u8)(ant_pwr_diff);		// RF-B
	//RTPRINT(FPHY, PHY_TXPWR, ("Antenna Diff from RF-B to RF-A = %d (0x%x)\n", 
		//ant_pwr_diff, pHalData->AntennaTxPwDiff[0]));

	u4RegValue = (priv->AntennaTxPwDiff[2]<<8 | 
				priv->AntennaTxPwDiff[1]<<4 | 
				priv->AntennaTxPwDiff[0]	);

	rtl8192_setBBreg(dev, rFPGA0_TxGainStage, 
		(bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);

	//RTPRINT(FPHY, PHY_TXPWR, ("Write BCD-Diff(0x%x) = 0x%lx\n", 
	//	rFPGA0_TxGainStage, u4RegValue));
}

void writeOFDMPowerReg(
	struct net_device* dev,
	u8		index,
	u32		Value
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u16 RegOffset[6] = {0xe00, 0xe04, 0xe10, 0xe14, 0xe18, 0xe1c};
	u8 i, rfa_pwr[4];
	u8 rfa_lower_bound = 0, rfa_upper_bound = 0, rf_pwr_diff = 0;
	u32 writeVal=Value;
	//
	// If path A and Path B coexist, we must limit Path A tx power.
	// Protect Path B pwr over or under flow. We need to calculate upper and
	// lower bound of path A tx power.
	//
	if (priv->rf_type == RF_2T2R)
	{			
		rf_pwr_diff = priv->AntennaTxPwDiff[0];
		//RTPRINT(FPHY, PHY_TXPWR, ("2T2R RF-B to RF-A PWR DIFF=%d\n", rf_pwr_diff));

		if (rf_pwr_diff >= 8)		// Diff=-8~-1
		{	// Prevent underflow!!
			rfa_lower_bound = 0x10-rf_pwr_diff;
			//printk("rfa_lower_bound= %d\n", rfa_lower_bound);
		}
		else if (rf_pwr_diff >= 0)	// Diff = 0-7
		{
			rfa_upper_bound = RF6052_MAX_TX_PWR-rf_pwr_diff;
			//printk("rfa_upper_bound= %d\n", rfa_upper_bound);
		}			
	}

	for (i=0; i<4; i++)
	{
		rfa_pwr[i] = (u8)((writeVal & (0x7f<<(i*8)))>>(i*8));
		if (rfa_pwr[i]  > RF6052_MAX_TX_PWR)
			rfa_pwr[i]  = RF6052_MAX_TX_PWR;

		// 
		// If path A and Path B coexist, we must limit Path A tx power.
		// Protect Path B pwr over or under flow. We need to calculate upper and
		// lower bound of path A tx power.
		//
		if (priv->rf_type== RF_2T2R)
		{
			if (rf_pwr_diff >= 8)		// Diff=-8~-1
			{	// Prevent underflow!!
				if (rfa_pwr[i] <rfa_lower_bound)
				{
					//printk("Underflow");
					rfa_pwr[i] = rfa_lower_bound;
				}
			}
			else if (rf_pwr_diff >= 1)	// Diff = 0-7
			{	// Prevent overflow
				if (rfa_pwr[i] > rfa_upper_bound)
				{
					//printk("Overflow");
					rfa_pwr[i] = rfa_upper_bound;
				}
			}
			//printk("rfa_pwr[%d]=%x\n", i, rfa_pwr[i]);
		}

	}

	writeVal = (rfa_pwr[3]<<24) | (rfa_pwr[2]<<16) |(rfa_pwr[1]<<8) |rfa_pwr[0];

	rtl8192_setBBreg(dev, RegOffset[index], 0x7f7f7f7f, writeVal);
	//printk("Set 0x%x = %08x\n",RegOffset[index], writeVal);
}

	
//-----------------------------------------------------------------------------
// Function:	PHY_RF6052SetOFDMTxPower
		//
// Overview:	For legacy and HY OFDM, we must read EEPROM TX power index for 
//			different channel and read original value in TX power register area from
//			0xe00. We increase offset and original value to be correct tx pwr.
		//		
// Input:       NONE
		//
// Output:      NONE
		// 
// Return:      NONE
//
// Revised History:
// When			Who		Remark
// 11/05/2008 	MHC		Simulate 8192 series method.
//01/06/2009	MHC		1. Prevent Path B tx power overflow or underflow dure to
//						A/B pwr difference or legacy/HT pwr diff.
//						2. We concern with path B legacy/HT OFDM difference.
// 01/22/2009	MHC		Support new EPRO format from SD3.
//---------------------------------------------------------------------------*/
extern void PHY_RF6052SetOFDMTxPower(struct net_device* dev, u8* pPowerLevel, u8 Channel)
{
	u32 writeVal, powerBase0, powerBase1;
	u8 index = 0;
	u8 finalPowerIndex[4];

	getPowerBase(dev, pPowerLevel, Channel, &powerBase0, &powerBase1, &finalPowerIndex[0]);
	setAntennaDiff(dev, &finalPowerIndex[0]	);
	
	for(index=0; index<6; index++)
		{
		getTxPowerWriteValByRegulatory(dev, Channel, index, 
			powerBase0, powerBase1, &writeVal);

		writeOFDMPowerReg(dev, index, writeVal);
	}
}	/* PHY_RF6052SetOFDMTxPower */


RT_STATUS PHY_RF6052_Config(struct net_device* dev)
{
	struct r8192_priv 			*priv = ieee80211_priv(dev);
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;	
	//RF90_RADIO_PATH_E		eRFPath;
	//BB_REGISTER_DEFINITION_T	*pPhyReg; 
	//u32						OrgStoreRFIntSW[RF90_PATH_D+1];
	
	//
	// Initialize general global value
	//
	// TODO: Extend RF_PATH_C and RF_PATH_D in the future
	if(priv->rf_type == RF_1T1R)
		priv->NumTotalRFPath = 1;
	else
		priv->NumTotalRFPath = 2;

	//
	// Config BB and RF
	//
//	switch( priv->bRegHwParaFile )
//	{
//		case 0:
//			phy_RF6052_Config_HardCode(dev);
//			break;

//		case 1:
			rtStatus = phy_RF6052_Config_ParaFile(dev);
//			break;

//		case 2:
			// Partial Modify. 
//			phy_RF6052_Config_HardCode(dev);
//			phy_RF6052_Config_ParaFile(dev);
//			break;

//		default:
//			phy_RF6052_Config_HardCode(dev);
//			break;
//	}
	return rtStatus;
		
}

void phy_RF6052_Config_HardCode(struct net_device* dev)
{
	
	// Set Default Bandwidth to 20M
	//Adapter->HalFunc	.SetBWModeHandler(Adapter, HT_CHANNEL_WIDTH_20);

	// TODO: Set Default Channel to channel one for RTL8225
	
}

RT_STATUS phy_RF6052_Config_ParaFile(struct net_device* dev)
{
	u32			u4RegValue = 0;
	//static s1Byte		szRadioAFile[] = RTL819X_PHY_RADIO_A;
	//static s1Byte		szRadioBFile[] = RTL819X_PHY_RADIO_B;
	//static s1Byte		szRadioBGMFile[] = RTL819X_PHY_RADIO_B_GM;
	u8			eRFPath;
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg;	
	//u8			eCheckItem;
	

	//3//-----------------------------------------------------------------
	//3// <2> Initialize RF
	//3//-----------------------------------------------------------------
	//for(eRFPath = RF90_PATH_A; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{

		pPhyReg = &priv->PHYRegDef[eRFPath];
		
		//----Store original RFENV control type----*/		
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			u4RegValue = rtl8192_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			u4RegValue = rtl8192_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16);
			break;
		}

		//----Set RF_ENV enable----*/		
		rtl8192_setBBreg(dev, pPhyReg->rfintfe, bRFSI_RFENV<<16, 0x1);
		
		//----Set RF_ENV output high----*/
		rtl8192_setBBreg(dev, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);

		// Set bit number of Address and Data for RF register */
		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0); 	// Set 1 to 4 bits for 8255
		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0);	// Set 0 to 12  bits for 8255


		//----Initialize RF fom connfiguration file----*/
		switch(eRFPath)
		{
		case RF90_PATH_A:
#if	RTL8190_Download_Firmware_From_Header
			rtStatus= rtl8192_phy_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath);
#else
			rtStatus = PHY_ConfigRFWithParaFile(Adapter, (char* )&szRadioAFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
			break;
		case RF90_PATH_B:
#if	RTL8190_Download_Firmware_From_Header
			rtStatus= rtl8192_phy_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath);
#else
			if(priv->rf_type == RF_2T2R_GREEN)
				rtStatus = PHY_ConfigRFWithParaFile(Adapter, (ps1Byte)&szRadioBGMFile, (RF90_RADIO_PATH_E)eRFPath);
			else
				rtStatus = PHY_ConfigRFWithParaFile(Adapter, (char* )&szRadioBFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		}

		//----Restore RFENV control type----*/;
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV, u4RegValue);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16, u4RegValue);
			break;
		}

		if(rtStatus != RT_STATUS_SUCCESS){
			printk("phy_RF6052_Config_ParaFile():Radio[%d] Fail!!", eRFPath);
			goto phy_RF6052_Config_ParaFile_Fail;
		}

	}

	RT_TRACE(COMP_INIT, "<---phy_RF6052_Config_ParaFile()\n");
	return rtStatus;
	
phy_RF6052_Config_ParaFile_Fail:	
	return rtStatus;
}


//
// ==> RF shadow Operation API Code Section!!!
//
//-----------------------------------------------------------------------------
// Function:	PHY_RFShadowRead
//				PHY_RFShadowWrite
//				PHY_RFShadowCompare
//				PHY_RFShadowRecorver
//				PHY_RFShadowCompareAll
//				PHY_RFShadowRecorverAll
//				PHY_RFShadowCompareFlagSet
//				PHY_RFShadowRecorverFlagSet
//
// Overview:	When we set RF register, we must write shadow at first.
//			When we are running, we must compare shadow abd locate error addr.
//			Decide to recorver or not.
//
// Input:       NONE
//
// Output:      NONE
//
// Return:      NONE
//
// Revised History:
// When			Who		Remark
// 11/20/2008 	MHC		Create Version 0.
//
//---------------------------------------------------------------------------*/
extern u32 PHY_RFShadowRead(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset)
{
	return	RF_Shadow[eRFPath][Offset].Value;
	
}	/* PHY_RFShadowRead */


extern void PHY_RFShadowWrite(
	struct net_device		* dev,
	u32	eRFPath,
	u32					Offset,
	u32					Data)
{
	//RF_Shadow[eRFPath][Offset].Value = (Data & bMask20Bits);
	RF_Shadow[eRFPath][Offset].Value = (Data & bRFRegOffsetMask);
	RF_Shadow[eRFPath][Offset].Driver_Write = true;
	
}	/* PHY_RFShadowWrite */


extern void PHY_RFShadowCompare(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset)
{
	u32	reg;
	
	// Check if we need to check the register
	if (RF_Shadow[eRFPath][Offset].Compare == true)
	{
		reg = rtl8192_phy_QueryRFReg(dev, eRFPath, Offset, bRFRegOffsetMask);
		// Compare shadow and real rf register for 20bits!!
		if (RF_Shadow[eRFPath][Offset].Value != reg)
		{
			// Locate error position.
			RF_Shadow[eRFPath][Offset].ErrorOrNot = true;
			RT_TRACE(COMP_INIT, "PHY_RFShadowCompare RF-%d Addr%02xErr = %05x", eRFPath, Offset, reg);
		}
	}

}	/* PHY_RFShadowCompare */

extern void PHY_RFShadowRecorver(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset)
{
	// Check if the address is error
	if (RF_Shadow[eRFPath][Offset].ErrorOrNot == true)
	{
		// Check if we need to recorver the register.
		if (RF_Shadow[eRFPath][Offset].Recorver == true)
		{
			rtl8192_phy_SetRFReg(dev, eRFPath, Offset, bRFRegOffsetMask, RF_Shadow[eRFPath][Offset].Value);
			RT_TRACE(COMP_INIT, "PHY_RFShadowRecorver RF-%d Addr%02x=%05x", 
			eRFPath, Offset, RF_Shadow[eRFPath][Offset].Value);
		}
	}
	
}	/* PHY_RFShadowRecorver */


extern void PHY_RFShadowCompareAll(struct net_device * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			PHY_RFShadowCompare(dev, (RF90_RADIO_PATH_E)eRFPath, Offset);
		}
	}
	
}	/* PHY_RFShadowCompareAll */


extern void PHY_RFShadowRecorverAll(struct net_device * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			PHY_RFShadowRecorver(dev, (RF90_RADIO_PATH_E)eRFPath, Offset);
		}
	}
	
}	/* PHY_RFShadowRecorverAll */


extern void PHY_RFShadowCompareFlagSet(
	struct net_device 		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset,
	u8					Type)
{
	// Set True or False!!!
	RF_Shadow[eRFPath][Offset].Compare = Type;
		
}	/* PHY_RFShadowCompareFlagSet */


extern void PHY_RFShadowRecorverFlagSet(
	struct net_device 		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset,
	u8					Type)
{
	// Set True or False!!!
	RF_Shadow[eRFPath][Offset].Recorver= Type;
		
}	/* PHY_RFShadowRecorverFlagSet */


extern void PHY_RFShadowCompareFlagSetAll(struct net_device  * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			// 2008/11/20 MH For S3S4 test, we only check reg 26/27 now!!!!
			if (Offset != 0x26 && Offset != 0x27)
				PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, FALSE);
			else
				PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, TRUE);
		}
	}
		
}	/* PHY_RFShadowCompareFlagSetAll */


extern void PHY_RFShadowRecorverFlagSetAll(struct net_device  * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			// 2008/11/20 MH For S3S4 test, we only check reg 26/27 now!!!!
			if (Offset != 0x26 && Offset != 0x27)
				PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, FALSE);
			else
				PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, TRUE);
		}
	}
		
}	/* PHY_RFShadowCompareFlagSetAll */



extern void PHY_RFShadowRefresh(struct net_device  * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			RF_Shadow[eRFPath][Offset].Value = 0;
			RF_Shadow[eRFPath][Offset].Compare = false;
			RF_Shadow[eRFPath][Offset].Recorver  = false;
			RF_Shadow[eRFPath][Offset].ErrorOrNot = false;
			RF_Shadow[eRFPath][Offset].Driver_Write = false;
		}
	}
	
}	/* PHY_RFShadowRead */

// End of HalRf6052.c */
