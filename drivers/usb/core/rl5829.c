#include <asm/io.h>
#include <rl5829_reg.h>
#include <ehci_reg.h>
#include <platform.h>
#include <venus.h>
#include <mars.h>

#define WRAP_BASE				0xb8013800
#define WRAP_VStatusOut1			(WRAP_BASE + 0x04)
#define WRAP_VStatusOut2			(WRAP_BASE + 0x24)
#define WRAP_VStatusOut_set_vstatus(data)	(0x000000FF&(data))
#define WRAP_Dummy				(WRAP_BASE + 0x14)
#define WRAP_VLoadM1_bit_nEnable		(0x00000004)
#define WRAP_VLoadM1_set_nEnable(data)		(0x00000004&((data)<<2))
#define WRAP_VLoadM2_bit_nEnable		(0x00000008)
#define WRAP_VLoadM2_set_nEnable(data)		(0x00000008&((data)<<3))
#define WRAP_VStatusOut_bit_Enable		(0x00000010)
#define WRAP_VStatusOut_set_Enable(data)	(0x00000010&((data)<<4))
//#define Max_port_num				0x1

#define INT8U u8
#define BOOLEAN	bool
#define WRITE_REG_INT32U(addr, value)	writel((value),(u32 *)(addr))
#define READ_REG_INT32U(addr)		readl((u32 *)(addr))

void check_boot_code_param(int *pparam, int len, char *ptr)
{
   int *param, value, i = 0, index = 0;

   param = pparam;

   if(ptr[0] != 0)
   {
	while((ptr[i] != 0) && (index < len))
	{
		if(ptr[i] == ',')
		{
			i++;
			index++;
			continue;
		}
		
		value = ptr[i] - '0';
		if((value >= 0) && (value <=9))
		{
			param[index] = value;
			i++;
			//printk("## cfyeh %s(%d) index %d value %d\n", __func__, __LINE__, index, param[index]);
			continue;
		}
		
		value = ptr[i] - 'a';
		if((value >= 0) && (value <=5))
		{
			param[index] = value + 10;
			i++;
			//printk("## cfyeh %s(%d) index %d value %d\n", __func__, __LINE__, index, param[index]);
			continue;
		}
		
		value = ptr[i] - 'A';
		if((value >= 0) && (value <=5))
		{
			param[index] = value + 10;
			i++;
			//printk("## cfyeh %s(%d) index %d value %d\n", __func__, __LINE__, index, param[index]);
			continue;
		}

		i++;
	}
   }
}

char USBPHY_SetReg(u8 addr, u8 data);
char USBPHY_SetReg_port(int port, u8 addr, u8 data);

void USBPHY_SetReg_Default_30(void)
{
   USBPHY_SetReg(USBPHY_30, USBPHY_30_set_xcvr_autok(0x1) |       // POR = 1
                            USBPHY_30_set_xcvr_SC(0x3) |          // POR = 3, PLL_C
                            USBPHY_30_set_xcvr_CP(0x1));          // POR = 1
}

void USBPHY_SetReg_Default_31(void)
{
   USBPHY_SetReg(USBPHY_31, USBPHY_31_set_xcvr_call_host(0x1) |   // POR = 1
                            USBPHY_31_set_xcvr_zeres_sel(0x0) |   // POR = 0
                            USBPHY_31_set_xcvr_zo_en(0x1) |       // POR = 1
                            USBPHY_31_set_xcvr_SD(0x1) |          // POR = 1
                            USBPHY_31_set_xcvr_SR(0x4));          // POR = 4, PLL_R
}

void USBPHY_SetReg_Default_32(int sen, int sh)
{
   USBPHY_SetReg(USBPHY_32, USBPHY_32_set_SEN((sen & 0xf)) |      // POR = 8
                            USBPHY_32_set_SH((sh & 0xf)));        // POR = 5, swing
}

void USBPHY_SetReg_Default_32_port(int port, int sen, int sh)
{
   USBPHY_SetReg_port(port, USBPHY_32, USBPHY_32_set_SEN((sen & 0xf)) |      // POR = 8
                            USBPHY_32_set_SH((sh & 0xf)));        // POR = 5, swing
}

void USBPHY_SetReg_Default_33(int dr)
{
   // for venus
   if(is_venus_cpu())
   {
   USBPHY_SetReg(USBPHY_33, USBPHY_33_set_HSXMPTPDEN(0x1) |       // POR = 1, hi-speed XCVR power down enable
                            USBPHY_33_set_xcvr_CAL(0x1) |         // POR = 1
                            USBPHY_33_set_xcvr_DB(0x4) |          // POR = 4
#ifdef CONFIG_REALTEK_VENUS_USB_1261
			    USBPHY_33_set_xcvr_DR(0x1)  	  // POR = 0, 1 for LS FIB
#else /* CONFIG_REALTEK_VENUS_USB_1261 */
//#ifdef CONFIG_REALTEK_VENUS_USB_1261_ECO
			    USBPHY_33_set_xcvr_DR(0x0)  	  // POR = 0
#endif /* CONFIG_REALTEK_VENUS_USB_1261 */
			    );
   }
   else if(is_neptune_cpu())// for neptune
   {
   USBPHY_SetReg(USBPHY_33, USBPHY_33_set_HSXMPTPDEN(0x1) |       // POR = 1, hi-speed XCVR power down enable
                            USBPHY_33_set_xcvr_CAL(0x1) |         // POR = 1
                            USBPHY_33_set_xcvr_DB(0x4) |          // POR = 4
			    USBPHY_33_set_xcvr_DR(0x7)  	  // POR = 0
			    );
   }
   else if(is_mars_cpu())// for mars
   {
   USBPHY_SetReg(USBPHY_33, USBPHY_33_set_HSXMPTPDEN(0x1) |       // POR = 1, hi-speed XCVR power down enable
                            USBPHY_33_set_xcvr_CAL(0x1) |         // POR = 1
                            USBPHY_33_set_xcvr_DB(0x4) |          // POR = 4
			    USBPHY_33_set_xcvr_DR(dr & 0x7)  	  // POR = 0
			    );
   }
}

void USBPHY_SetReg_Default_33_port(int port, int dr)
{
   // for venus
   if(is_venus_cpu())
   {
   USBPHY_SetReg_port(port, USBPHY_33, USBPHY_33_set_HSXMPTPDEN(0x1) |       // POR = 1, hi-speed XCVR power down enable
                            USBPHY_33_set_xcvr_CAL(0x1) |         // POR = 1
                            USBPHY_33_set_xcvr_DB(0x4) |          // POR = 4
#ifdef CONFIG_REALTEK_VENUS_USB_1261
			    USBPHY_33_set_xcvr_DR(0x1)  	  // POR = 0, 1 for LS FIB
#else /* CONFIG_REALTEK_VENUS_USB_1261 */
//#ifdef CONFIG_REALTEK_VENUS_USB_1261_ECO
			    USBPHY_33_set_xcvr_DR(0x0)  	  // POR = 0
#endif /* CONFIG_REALTEK_VENUS_USB_1261 */
			    );
   }
   else if(is_neptune_cpu())// for neptune
   {
   USBPHY_SetReg_port(port, USBPHY_33, USBPHY_33_set_HSXMPTPDEN(0x1) |       // POR = 1, hi-speed XCVR power down enable
                            USBPHY_33_set_xcvr_CAL(0x1) |         // POR = 1
                            USBPHY_33_set_xcvr_DB(0x4) |          // POR = 4
			    USBPHY_33_set_xcvr_DR(0x7)  	  // POR = 0
			    );
   }
   else if(is_mars_cpu())// for mars
   {
   USBPHY_SetReg_port(port, USBPHY_33, USBPHY_33_set_HSXMPTPDEN(0x1) |       // POR = 1, hi-speed XCVR power down enable
                            USBPHY_33_set_xcvr_CAL(0x1) |         // POR = 1
                            USBPHY_33_set_xcvr_DB(0x4) |          // POR = 4
			    USBPHY_33_set_xcvr_DR(dr & 0x7)  	  // POR = 0
			    );
   }
}

#ifdef USB_MARS_HOST_LS_HACK
void USBPHY_SetReg_Default_33_LS(int port, int dr)
{
//printk("#######[cfyeh-debug] %s(%d) port 0x%x, dr 0x%x\n", __func__, __LINE__, port, dr);
   // for venus
   if(is_venus_cpu())
   {
   USBPHY_SetReg(USBPHY_33, USBPHY_33_set_HSXMPTPDEN(0x1) |       // POR = 1, hi-speed XCVR power down enable
                            USBPHY_33_set_xcvr_CAL(0x1) |         // POR = 1
                            USBPHY_33_set_xcvr_DB(0x4) |          // POR = 4
#ifdef CONFIG_REALTEK_VENUS_USB_1261
			    USBPHY_33_set_xcvr_DR(0x1)  	  // POR = 0, 1 for LS FIB
#else /* CONFIG_REALTEK_VENUS_USB_1261 */
//#ifdef CONFIG_REALTEK_VENUS_USB_1261_ECO
			    USBPHY_33_set_xcvr_DR(0x0)  	  // POR = 0
#endif /* CONFIG_REALTEK_VENUS_USB_1261 */
			    );
   }
   else if(is_neptune_cpu())// for neptune
   {
   USBPHY_SetReg(USBPHY_33, USBPHY_33_set_HSXMPTPDEN(0x1) |       // POR = 1, hi-speed XCVR power down enable
                            USBPHY_33_set_xcvr_CAL(0x1) |         // POR = 1
                            USBPHY_33_set_xcvr_DB(0x4) |          // POR = 4
			    USBPHY_33_set_xcvr_DR(0x7)  	  // POR = 0
			    );
   }
   else if(is_mars_cpu())// for mars
   {
   USBPHY_SetReg_port(port, USBPHY_33, USBPHY_33_set_HSXMPTPDEN(0x1) |       // POR = 1, hi-speed XCVR power down enable
                            USBPHY_33_set_xcvr_CAL(0x1) |         // POR = 1
                            USBPHY_33_set_xcvr_DB(0x4) |          // POR = 4
			    USBPHY_33_set_xcvr_DR(0x2)  	  // POR = 0
			    );
   }
}
#endif /* USB_MARS_HOST_LS_HACK */

void USBPHY_SetReg_Default_34(void)
{
   // for venus and neptune
   if((is_venus_cpu() || is_neptune_cpu()))
   {
   USBPHY_SetReg(USBPHY_34, USBPHY_34_set_TPA_EN(0x0) |    	  // POR = 0
                            USBPHY_34_set_TPB_EN(0x0) |    	  // POR = 0
                            USBPHY_34_set_xcvr_TS(0x0) |          // POR = 0
                            USBPHY_34_set_xcvr_SE(0x2));          // POR = 2
   }
   else if(is_mars_cpu())// for mars
   {
   USBPHY_SetReg(USBPHY_34, USBPHY_34_set_DBG_EN(0x0) |    	  // POR = 0
                            USBPHY_34_set_xcvr_TS(0x0));          // POR = 0
   }
}

void USBPHY_SetReg_Default_35(int src)
{
   // for venus and neptune
   if((is_venus_cpu() || is_neptune_cpu()))
   {
   USBPHY_SetReg(USBPHY_35, USBPHY_35_set_TPC_EN(0x0) |          // POR = 0
                            USBPHY_35_set_xcvr_SP(0x0) |          // POR = 0
                            USBPHY_35_set_xcvr_SRC((src & 0x7)) | // POR = 7,  slew rate control
                            USBPHY_35_set_HSTESTEN(0x0) |         // POR = 0
                            USBPHY_35_set_xcvr_nsqdly(0x1));      // POR = 1
   }
   else if(is_mars_cpu())// for mars
   {
   USBPHY_SetReg(USBPHY_35, USBPHY_35_set_SE0_LVL(0x1) |          // POR = 1
                            USBPHY_35_set_xcvr_SRC((src & 0x7)) | // POR = 7,  slew rate control
                            USBPHY_35_set_HSTESTEN(0x0) |         // POR = 0
                            USBPHY_35_set_xcvr_nsqdly(0x1));      // POR = 1
   }
}

void USBPHY_SetReg_Default_35_port(int port, int src)
{
   // for venus and neptune
   if((is_venus_cpu() || is_neptune_cpu()))
   {
   USBPHY_SetReg_port(port, USBPHY_35, USBPHY_35_set_TPC_EN(0x0) |          // POR = 0
                            USBPHY_35_set_xcvr_SP(0x0) |          // POR = 0
                            USBPHY_35_set_xcvr_SRC((src & 0x7)) | // POR = 7,  slew rate control
                            USBPHY_35_set_HSTESTEN(0x0) |         // POR = 0
                            USBPHY_35_set_xcvr_nsqdly(0x1));      // POR = 1
   }
   else if(is_mars_cpu())// for mars
   {
   USBPHY_SetReg_port(port, USBPHY_35, USBPHY_35_set_SE0_LVL(0x1) |          // POR = 1
                            USBPHY_35_set_xcvr_SRC((src & 0x7)) | // POR = 7,  slew rate control
                            USBPHY_35_set_HSTESTEN(0x0) |         // POR = 0
                            USBPHY_35_set_xcvr_nsqdly(0x1));      // POR = 1
   }
}

void USBPHY_SetReg_Default_36(int senh)
{
   USBPHY_SetReg(USBPHY_36, USBPHY_36_set_xcvr_senh((senh & 0xf)) | // POR = 9,  sensitivity for host, for HS disconnect detect (EOP) 
                            USBPHY_36_set_xcvr_adjr(0x8));          // POR = 8
}

void USBPHY_SetReg_Default_36_port(int port, int senh)
{
   USBPHY_SetReg_port(port, USBPHY_36, USBPHY_36_set_xcvr_senh((senh & 0xf)) | // POR = 9,  sensitivity for host, for HS disconnect detect (EOP) 
                            USBPHY_36_set_xcvr_adjr(0x8));          // POR = 8
}

void USBPHY_SetReg_Default_37(void)
{
   USBPHY_SetReg(USBPHY_37, USBPHY_37_set_LDO(0x0) |              // POR = 0   
			    USBPHY_37_set_LDO_TN(0x3) |		  // POR = 3
			    USBPHY_37_set_PLL_TEST_EN(0x0));	  // POR = 0
}

void USBPHY_SetReg_Default_38(int slb_en)
{
   if(slb_en != 0)
   	 slb_en = 1;

   USBPHY_SetReg(USBPHY_38, USBPHY_38_set_dbnc_en(0x1) |          // POR = 1
                            USBPHY_38_set_discon_enable(0x1) |    // POR = 1
                            USBPHY_38_set_EN_ERR_UNDERRUN(0x1) |  // POR = 1
                            USBPHY_38_set_LATE_DLLEN(0x1) |       // POR = 1
                            USBPHY_38_set_INTG(0x1) |             // POR = 0
                            USBPHY_38_set_SOP_KK(0x1) |           // POR = 1
                            USBPHY_38_set_SLB_INNER(0x0) |        // POR = 0
                            USBPHY_38_set_SLB_EN((slb_en & 0x1)));        // POR = 0
}

void USBPHY_SetReg_Default_39(void)
{
	 USBPHY_SetReg(USBPHY_39, USBPHY_39_set_utmi_pos_out(0x1) |     // POR = 1
				  USBPHY_39_set_slb_rst(0x0) |		// POR = 0
				  USBPHY_39_set_slb_sel(0x0) |		// POR = 0
				  USBPHY_39_set_auto_sel(0x1) |		// POR = 1
				  USBPHY_39_set_tx_delay(0x1) |		// POR = 1
				  USBPHY_39_set_slb_fs(0x0));		// POR = 0
}

void USBPHY_SetReg_Default_3A(void)
{
	 USBPHY_SetReg(USBPHY_3A, USBPHY_3A_set_force_xcvrsel(0x0) |		// POR = 0
	                          USBPHY_3A_set_xcvsel_mode(0x0) |		// POR = 0
	                          USBPHY_3A_set_force_termsel(0x0) |		// POR = 0
	                          USBPHY_3A_set_termsel_mode(0x0) |		// POR = 0
	                          USBPHY_3A_set_force_opmode(0x0) |		// POR = 0
	                          USBPHY_3A_set_opmode_mode(0x0));		// POR = 0
}

#if defined (USB_MARS_HOST_TEST_MODE_JK) || defined (USB_MARS_OTG_TEST_MODE_JK)
void USBPHY_SetReg_Default_39_for_JK(int port)
{
	 USBPHY_SetReg_port(port, USBPHY_39, USBPHY_39_set_utmi_pos_out(0x1) |     // POR = 1
				  USBPHY_39_set_slb_rst(0x0) |		// POR = 0
				  USBPHY_39_set_slb_sel(0x0) |		// POR = 0
				  USBPHY_39_set_auto_sel(0x1) |		// POR = 1
				  USBPHY_39_set_tx_delay(0x3) |		// POR = 1
				  USBPHY_39_set_slb_fs(0x0));		// POR = 0
}

void USBPHY_SetReg_Default_3A_for_JK(int port)
{
	 USBPHY_SetReg_port(port, USBPHY_3A, USBPHY_3A_set_force_xcvrsel(0x1) |		// POR = 0
	                          USBPHY_3A_set_xcvsel_mode(0x0) |		// POR = 0
	                          USBPHY_3A_set_force_termsel(0x1) |		// POR = 0
	                          USBPHY_3A_set_termsel_mode(0x0) |		// POR = 0
	                          USBPHY_3A_set_force_opmode(0x1) |		// POR = 0
	                          USBPHY_3A_set_opmode_mode(0x2));		// POR = 0
}
#endif /* defined (USB_MARS_HOST_TEST_MODE_JK) || defined (USB_MARS_OTG_TEST_MODE_JK) */

void USBPHY_SetReg_Default_3B(void)
{
	 USBPHY_SetReg(USBPHY_3B, USBPHY_3B_set_force_serial(0x0) |		// POR = 0
	                          USBPHY_3B_set_serial_mode(0x0) |		// POR = 0
	                          USBPHY_3B_set_force_suspendm(0x0) |		// POR = 0
	                          USBPHY_3B_set_suspendm_mode(0x1) |		// POR = 1
	                          USBPHY_3B_set_force_txse0(0x0) |		// POR = 0
	                          USBPHY_3B_set_txse0_mode(0x0) |		// POR = 0
	                          USBPHY_3B_set_force_txenable_n(0x0) |		// POR = 0
	                          USBPHY_3B_set_txenable_n_mode(0x1));		// POR = 1
}

#if defined (USB_PHY_SETTING_NORMAL)
// for venus

void UTMI_VendorIF_Init(void){
	int port_num ;
	int Max_port_num = 0;

	// for venus and neptune
	if((is_venus_cpu() || is_neptune_cpu()))
		Max_port_num = 1;
	else if(is_mars_cpu())// for mars
		Max_port_num = 2;

	for(port_num=1; port_num <=Max_port_num; port_num++)
	{
		//polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
		// set VPort[3:0] 
		WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
				(~ EHCI_INSNREG05_bit_VPort)) |
				EHCI_INSNREG05_set_VPort(port_num));

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
		//VLoadM1 = 1, VControl1[3:0] = 0
		WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
				(~ EHCI_INSNREG05_bit_VLoadM) &
				(~ EHCI_INSNREG05_bit_VCtrl)) |
				EHCI_INSNREG05_set_VLoadM(0x1) |
				EHCI_INSNREG05_set_VCtrl(0x0));

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
		if (port_num == 1)
		{
			// clear vstatus_out1
			WRITE_REG_INT32U(WRAP_VStatusOut1, WRAP_VStatusOut_set_vstatus(0x0));
		}
		else
		{
			// clear vstatus_out2
			WRITE_REG_INT32U(WRAP_VStatusOut2, WRAP_VStatusOut_set_vstatus(0x0));
		}

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
	}
}

char USBPHY_SetReg(u8 addr, u8 data){
	int port_num ;
	int Max_port_num = 0;
	unsigned int reg_value = 0;

	// for venus and neptune
	if((is_venus_cpu() || is_neptune_cpu()))
		Max_port_num = 1;
	else if(is_mars_cpu())// for mars
		Max_port_num = 2;

	for(port_num=1; port_num <=Max_port_num; port_num++)
	{
		if (port_num == 1)
		{
			//write data to VStatusOut1 (data output to phy)
			WRITE_REG_INT32U(WRAP_VStatusOut1, WRAP_VStatusOut_set_vstatus(data));
		}
		else 
		{
			//write data to VStatusOut2 (data output to phy)
			WRITE_REG_INT32U(WRAP_VStatusOut2, WRAP_VStatusOut_set_vstatus(data));
		}

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

		// VCtrl = low nibble of addr, VLoadM = 1
		reg_value = (READ_REG_INT32U(EHCI_INSNREG05) &
				(~ EHCI_INSNREG05_bit_VLoadM) &
				(~ EHCI_INSNREG05_bit_VPort) &
				(~ EHCI_INSNREG05_bit_VCtrl)) |
				EHCI_INSNREG05_set_VPort(port_num) |
				EHCI_INSNREG05_set_VLoadM(0x1) |
				EHCI_INSNREG05_set_VCtrl((0x0f & addr));
		WRITE_REG_INT32U(EHCI_INSNREG05, reg_value);

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

		// VCtrl = low nibble of addr, VLoadM = 0
		reg_value &= ~ EHCI_INSNREG05_bit_VLoadM;
		WRITE_REG_INT32U(EHCI_INSNREG05, reg_value);

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

		// VCtrl = high nibble of addr, VLoadM = 1
		reg_value |= EHCI_INSNREG05_bit_VLoadM;
		reg_value &= ~ EHCI_INSNREG05_bit_VCtrl;
		reg_value |= EHCI_INSNREG05_set_VCtrl(((0xf0 & addr) >> 4));
		WRITE_REG_INT32U(EHCI_INSNREG05, reg_value);

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

		// VCtrl = high nibble of addr, VLoadM = 0
		reg_value &= ~ EHCI_INSNREG05_bit_VLoadM;
		WRITE_REG_INT32U(EHCI_INSNREG05, reg_value);

		// polling until VBusy == 0, ensure the data is writen
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
	}
	return 0;
}

char USBPHY_SetReg_port(int port, u8 addr, u8 data){
	int port_num ;
	int Max_port_num = 0;
	unsigned int reg_value = 0;

	// for venus and neptune
	if((is_venus_cpu() || is_neptune_cpu()))
		Max_port_num = 1;
	else if(is_mars_cpu())// for mars
		Max_port_num = 2;

	for(port_num=1; port_num <=Max_port_num; port_num++)
	{
		if(port != port_num)
			continue;
		if (port_num == 1)
		{
			//write data to VStatusOut1 (data output to phy)
			WRITE_REG_INT32U(WRAP_VStatusOut1, WRAP_VStatusOut_set_vstatus(data));
		}
		else 
		{
			//write data to VStatusOut2 (data output to phy)
			WRITE_REG_INT32U(WRAP_VStatusOut2, WRAP_VStatusOut_set_vstatus(data));
		}

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

		// VCtrl = low nibble of addr, VLoadM = 1
		reg_value = (READ_REG_INT32U(EHCI_INSNREG05) &
				(~ EHCI_INSNREG05_bit_VLoadM) &
				(~ EHCI_INSNREG05_bit_VPort) &
				(~ EHCI_INSNREG05_bit_VCtrl)) |
				EHCI_INSNREG05_set_VPort(port_num) |
				EHCI_INSNREG05_set_VLoadM(0x1) |
				EHCI_INSNREG05_set_VCtrl((0x0f & addr));
		WRITE_REG_INT32U(EHCI_INSNREG05, reg_value);

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

		// VCtrl = low nibble of addr, VLoadM = 0
		reg_value &= ~ EHCI_INSNREG05_bit_VLoadM;
		WRITE_REG_INT32U(EHCI_INSNREG05, reg_value);

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

		// VCtrl = high nibble of addr, VLoadM = 1
		reg_value |= EHCI_INSNREG05_bit_VLoadM;
		reg_value &= ~ EHCI_INSNREG05_bit_VCtrl;
		reg_value |= EHCI_INSNREG05_set_VCtrl(((0xf0 & addr) >> 4));
		WRITE_REG_INT32U(EHCI_INSNREG05, reg_value);

		// polling until VBusy == 0
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

		// VCtrl = high nibble of addr, VLoadM = 0
		reg_value &= ~ EHCI_INSNREG05_bit_VLoadM;
		WRITE_REG_INT32U(EHCI_INSNREG05, reg_value);

		// polling until VBusy == 0, ensure the data is writen
		while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
	}
	return 0;
}

INT8U USBPHY_GetReg(INT8U port, INT8U addr){
	if((USBPHY_30 == addr) | (USBPHY_31 == addr) | (USBPHY_32 == addr) |
	   (USBPHY_33 == addr) | (USBPHY_34 == addr) | (USBPHY_35 == addr) |
	   (USBPHY_36 == addr) | (USBPHY_37 == addr) | (USBPHY_38 == addr) |
	   (USBPHY_39 == addr) | (USBPHY_3A == addr) | (USBPHY_3B == addr) )
		return 0;

	// for venus and neptune
	if((is_venus_cpu() || is_neptune_cpu()))
	{
		// valid port number : 1
		if(1 != port)
			return 0;
	}
	else if(is_mars_cpu())// for mars
	{
		// valid port number : 1 & 2
		if((0 == port) || (3 <= port))
			return 0;
	}

	// polling until VBusy == 0
	while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

	// VCtrl = low nibble of addr, VLoadM = 1
	WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
			(~ EHCI_INSNREG05_bit_VLoadM) &
			(~ EHCI_INSNREG05_bit_VPort) &
			(~ EHCI_INSNREG05_bit_VCtrl)) |
			EHCI_INSNREG05_set_VPort(port) |
			EHCI_INSNREG05_set_VLoadM(0x1) |
			EHCI_INSNREG05_set_VCtrl(((0xf0 & addr) >> 4)));

	// polling until VBusy == 0
	while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

	// VCtrl = low nibble of addr, VLoadM = 0
	WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
			(~ EHCI_INSNREG05_bit_VLoadM)) |
			EHCI_INSNREG05_set_VLoadM(0x0));

	// polling until VBusy == 0
	while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

	// VCtrl = high nibble of addr, VLoadM = 1
	WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
			(~ EHCI_INSNREG05_bit_VLoadM) &
			(~ EHCI_INSNREG05_bit_VCtrl)) |
			EHCI_INSNREG05_set_VLoadM(0x1) |
			EHCI_INSNREG05_set_VCtrl((0x0f & addr)));

	// polling until VBusy == 0
	while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

	// VCtrl = high nibble of addr, VLoadM = 0
	WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
			(~ EHCI_INSNREG05_bit_VLoadM)) |
			EHCI_INSNREG05_set_VLoadM(0x0));

	// polling until VBusy == 0, ensure the data is writen
	while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

	return (INT8U)EHCI_INSNREG05_get_VStatus(READ_REG_INT32U(EHCI_INSNREG05));
}

void USBPHY_Register_Setting(void){
	int param[0x10];
	int param1[0x10];
	int param2[0x10];
	int i=0;
	unsigned char *usb_param, *usb1_param, *usb2_param;

	param[0] = 0x5;		// reg 32 sh, POR = 5, 4bit;
	param[1] = 0x7;		// reg 35 src, POR = 7, 3bit;
	param[2] = 0xd;		// reg 36 senh, POR = 9, 4bit;
	param[3] = 0x8;		// reg 32 sen, POR = 8, 4bit;
	param[4] = 0x0;		// reg 33 dr, POR = 0, 3bit;

	// for venus and neptune
	if((is_venus_cpu() || is_neptune_cpu()))
		printk("[usb_param] setting for venus or neptune\n");
	else if(is_mars_cpu())// for mars
	{
		printk("[usb_param] setting for mars\n");

#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING
		// VENUS_SB2_CHIP_INFO: bit[31,16]
		// 0xa0 => A version
		// 0xb0 => B version
		if((inl(VENUS_SB2_CHIP_INFO) >> 16) == 0xa0)
		{
			outl(0x4000a081, VENUS_USB_HOST_USBIP_INPUT);
			outl(0x40002000, MARS_USB_HOST_USBIPINPUT_2PORT);
			printk("Force diconnect for MARS-A\n");
		}
		else
		{
			outl(0x4400a081, VENUS_USB_HOST_USBIP_INPUT);
			outl(0x44002000, MARS_USB_HOST_USBIPINPUT_2PORT);
			printk("Setting for MARS-B\n");
		}
#endif /* USB_MARS_EHCI_CONNECTION_STATE_POLLING */

		// set MARS_USB_HOST_VERSION[bit1](Nouse_done)=1
		// 1: Interrupt doesnt gatting sb1_usb_done signal
		// 0: interrupt gatting with sb1_usb_done signal
		outl(inl(MARS_USB_HOST_VERSION)| 1<<1, MARS_USB_HOST_VERSION);

#ifdef USB_MARS_HOST_OTG_SWITCH_BY_GPIO
	printk("[usb_param] set GPIO 8 as input pin for switch host and otg!\n");
	// set MARS_0_SYS_MUXPAD0[25:24] = 2'b01 for gpio8
	writel((readl((unsigned int *)MARS_0_SYS_MUXPAD0)& ~(3<<24)), (unsigned int *)MARS_0_SYS_MUXPAD0);
	writel((readl((unsigned int *)MARS_0_SYS_MUXPAD0)| (1<<24)), (unsigned int *)MARS_0_SYS_MUXPAD0);

	// set MARS_MIS_GP0DIR[8]=1'b0
	outl(inl(MARS_MIS_GP0DIR)& ~(1<<8), MARS_MIS_GP0DIR);

	// check MARS_MIS_GP0DATI gpio 8 : 1=otg device 0=host mode
	if((inl(MARS_MIS_GP0DATI) & (1<<8)) == (1<<8)) // otg device
	{
		if((inl(MARS_USB_HOST_OTG) & 1) != 1) // not set otg device yet
		{
			// set MARS_USB_HOST_OTGMUX=0x001e01ff for otg
			outl(0x001e01ff, MARS_USB_HOST_OTGMUX);

			// set MARS_USB_HOST_OTG[bit0](otg_enable)
			// 1: enable OTG, 0: disable OTG
			printk("#######[cfyeh-debug] %s(%d) set port two as otg device!\n", __func__, __LINE__);
			outl(0x1, MARS_USB_HOST_OTG); // enable otg
		}
	}
	else
	{
		if((inl(MARS_USB_HOST_OTG) & 1) == 1) // already set otg device
		{
			// set MARS_USB_HOST_OTG[bit0](otg_enable)
			// 1: enable OTG, 0: disable OTG
			printk("#######[cfyeh-debug] %s(%d) set port two as host mode!\n", __func__, __LINE__);
			outl(0x0, MARS_USB_HOST_OTG); // disable otg
		}
	}
#else
#ifdef USB_MARS_OTG_ENABLE_IN_PORT_TWO
		// set MARS_USB_HOST_OTGMUX=0x001e01ff for otg
		outl(0x001e01ff, MARS_USB_HOST_OTGMUX);

		// set MARS_USB_HOST_OTG[bit0](otg_enable)
		// 1: enable OTG, 0: disable OTG
		printk("[usb_param] set port two for otg!\n");
		outl(0x1, MARS_USB_HOST_OTG); // enable otg
#else
		printk("[usb_param] set port two for host!\n");
		outl(0x0, MARS_USB_HOST_OTG); // disable otg
#endif /* USB_MARS_OTG_ENABLE_IN_PORT_TWO */
#endif /* USB_MARS_HOST_OTG_SWITCH_BY_GPIO */
	}

	usb_param = parse_token(platform_info.system_parameters, "usb_param");
	if(!usb_param) {
		usb_param = platform_info.usb_param;
	}

	usb1_param = parse_token(platform_info.system_parameters, "usb1_param");
	if(!usb1_param) {
		usb1_param = platform_info.usb1_param;
	}

	usb2_param = parse_token(platform_info.system_parameters, "usb2_param");
	if(!usb2_param) {
		usb2_param = platform_info.usb2_param;
	}

	check_boot_code_param((int *)&param[0], 5, usb_param);
	if(is_mars_cpu())
	{
		for(i=0; i<10; i++)
		{
			param1[i]=param[i];
			param2[i]=param[i];
		}
		check_boot_code_param((int *)&param1[0], 5, usb1_param);
		check_boot_code_param((int *)&param2[0], 5, usb2_param);
	}

	UTMI_VendorIF_Init();

	USBPHY_SetReg_Default_30();
	USBPHY_SetReg_Default_31();
	if((is_venus_cpu() || is_neptune_cpu()))
	{
		USBPHY_SetReg_Default_32(param[3], param[0]);
		USBPHY_SetReg_Default_33(param[4]);
	}
	else if(is_mars_cpu())// for mars
	{
		USBPHY_SetReg_Default_32_port(1, param1[3], param1[0]);
		USBPHY_SetReg_Default_33_port(1, param1[4]);

		USBPHY_SetReg_Default_32_port(2, param2[3], param2[0]);
		USBPHY_SetReg_Default_33_port(2, param2[4]);
	}
	USBPHY_SetReg_Default_34();
	if((is_venus_cpu() || is_neptune_cpu()))
	{
		USBPHY_SetReg_Default_35(param[1]);
		USBPHY_SetReg_Default_36(param[2]);
	}
	else if(is_mars_cpu())// for mars
	{
		USBPHY_SetReg_Default_35_port(1, param1[1]);
		USBPHY_SetReg_Default_36_port(1, param1[2]);

		USBPHY_SetReg_Default_35_port(2, param2[1]);
		USBPHY_SetReg_Default_36_port(2, param2[2]);
	}

	// for mars
	if(is_mars_cpu())// for mars
		USBPHY_SetReg_Default_37();

	USBPHY_SetReg_Default_38(0);

	// for mars
	if(is_mars_cpu())// for mars
	{
		USBPHY_SetReg_Default_39();
		USBPHY_SetReg_Default_3A();
		USBPHY_SetReg_Default_3B();
	}

	if((is_venus_cpu() || is_neptune_cpu()))
	{
		printk("[usb_param] usbphy reg 32, set sh = 0x%x, get sh = 0x%x, 4bit\n", \
				param[0], USBPHY_GetReg(1, USBPHY_22) & 0xf);
		printk("[usb_param] usbphy reg 35, set src = 0x%x, get src = 0x%x, 3bit\n", \
				param[1], (USBPHY_GetReg(1, USBPHY_25)>>2) & 0x7);
		printk("[usb_param] usbphy reg 36, set senh = 0x%x, get senh = 0x%x, 4bit\n", \
				param[2], (USBPHY_GetReg(1, USBPHY_26)>>4) & 0xf);
		printk("[usb_param] usbphy reg 32, set sen = 0x%x, get sen = 0x%x, 4bit\n", \
				param[3], (USBPHY_GetReg(1, USBPHY_22)>>4) & 0xf);
		printk("[usb_param] usbphy reg 33, set dr = 0x%x, get dr = 0x%x, 3bit\n", \
				param[4], (USBPHY_GetReg(1, USBPHY_23)) & 0x7);
	}
	else if(is_mars_cpu())
	{
		printk("[usb1_param] usbphy reg 32, set sh = 0x%x, get sh = 0x%x, 4bit\n", \
				param1[0], USBPHY_GetReg(1, USBPHY_22) & 0xf);
		printk("[usb1_param] usbphy reg 35, set src = 0x%x, get src = 0x%x, 3bit\n", \
				param1[1], (USBPHY_GetReg(1, USBPHY_25)>>2) & 0x7);
		printk("[usb1_param] usbphy reg 36, set senh = 0x%x, get senh = 0x%x, 4bit\n", \
				param1[2], (USBPHY_GetReg(1, USBPHY_26)>>4) & 0xf);
		printk("[usb1_param] usbphy reg 32, set sen = 0x%x, get sen = 0x%x, 4bit\n", \
				param1[3], (USBPHY_GetReg(1, USBPHY_22)>>4) & 0xf);
		printk("[usb1_param] usbphy reg 33, set dr = 0x%x, get dr = 0x%x, 3bit\n", \
				param1[4], (USBPHY_GetReg(1, USBPHY_23)) & 0x7);

		printk("[usb2_param] usbphy reg 32, set sh = 0x%x, get sh = 0x%x, 4bit\n", \
				param2[0], USBPHY_GetReg(2, USBPHY_22) & 0xf);
		printk("[usb2_param] usbphy reg 35, set src = 0x%x, get src = 0x%x, 3bit\n", \
				param2[1], (USBPHY_GetReg(2, USBPHY_25)>>2) & 0x7);
		printk("[usb2_param] usbphy reg 36, set senh = 0x%x, get senh = 0x%x, 4bit\n", \
				param2[2], (USBPHY_GetReg(2, USBPHY_26)>>4) & 0xf);
		printk("[usb2_param] usbphy reg 32, set sen = 0x%x, get sen = 0x%x, 4bit\n", \
				param2[3], (USBPHY_GetReg(2, USBPHY_22)>>4) & 0xf);
		printk("[usb2_param] usbphy reg 33, set dr = 0x%x, get dr = 0x%x, 3bit\n", \
				param2[4], (USBPHY_GetReg(2, USBPHY_23)) & 0x7);
	}
}

#elif defined (USB_MARS_PHY_SETTING_FOR_FPGA)
// for MARS FPGA

void UTMI_VendorIF_Init(void){

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
   // VPort[3:0] = 1
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VPort)) |
                                    EHCI_INSNREG05_set_VPort(0x1));

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
   // VLoadM1 = 1, VControl1[3:0] = 0
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VLoadM) &
                                     (~ EHCI_INSNREG05_bit_VCtrl)) |
                                    EHCI_INSNREG05_set_VLoadM(0x1) |
                                    EHCI_INSNREG05_set_VCtrl(0x0));
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

   // clear vstatus_out1
   WRITE_REG_INT32U(WRAP_VStatusOut1, WRAP_VStatusOut_set_vstatus(0x0));

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
   // VPort[3:0] = 2
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VPort)) |
                                    EHCI_INSNREG05_set_VPort(0x2));

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
   // VLoadM2 = 1, VControl2[3:0] = 0
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VLoadM) &
                                     (~ EHCI_INSNREG05_bit_VCtrl)) |
                                    EHCI_INSNREG05_set_VLoadM(0x1) |
                                    EHCI_INSNREG05_set_VCtrl(0x0));
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

   // clear vstatus_out2
   WRITE_REG_INT32U(WRAP_VStatusOut2, WRAP_VStatusOut_set_vstatus(0x0));

   // clear P1_VLOADM / P2_VLOADM, tri-state the VSTATUS[7:0]
   WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                 (~ WRAP_VLoadM1_bit_nEnable) &
                                 (~ WRAP_VLoadM2_bit_nEnable) &
                                 (~ WRAP_VStatusOut_bit_Enable)) |
                                 WRAP_VLoadM1_set_nEnable(0x1) |
                                 WRAP_VLoadM2_set_nEnable(0x1));
}

char USBPHY_SetReg(u8 addr, u8 data){

   char result;

   // set output enable of VSTATUS[7:0]
   WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                 (~ WRAP_VStatusOut_bit_Enable)) |
                                 WRAP_VStatusOut_set_Enable(0x1));

   // set P1_VLOADM, P2_VLOADM
   WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                 (~ WRAP_VLoadM1_bit_nEnable) &
                                 (~ WRAP_VLoadM2_bit_nEnable)) |
                                WRAP_VLoadM1_set_nEnable(0x0) |
                                WRAP_VLoadM2_set_nEnable(0x0));

   // write data to VStatusOut1, the data will be sent out as VSTATUS
   WRITE_REG_INT32U(WRAP_VStatusOut1, WRAP_VStatusOut_set_vstatus(data));

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
   // VPort[3:0] = port
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VPort)) |
                                    EHCI_INSNREG05_set_VPort(0x1));

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
   // VCtrl = low nibble of addr
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VCtrl)) |
                                    EHCI_INSNREG05_set_VCtrl((0x0f & addr)));
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

   // clear P1_VLOADM, P2_VLOADM
   WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                 (~ WRAP_VLoadM1_bit_nEnable) &
                                 (~ WRAP_VLoadM2_bit_nEnable)) |
                                WRAP_VLoadM1_set_nEnable(0x1) |
                                WRAP_VLoadM2_set_nEnable(0x1));

   // set P1_VLOADM, P2_VLOADM
   WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                 (~ WRAP_VLoadM1_bit_nEnable) &
                                 (~ WRAP_VLoadM2_bit_nEnable)) |
                                WRAP_VLoadM1_set_nEnable(0x0) |
                                WRAP_VLoadM2_set_nEnable(0x0));

   // VCtrl = high nibble of addr
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VCtrl)) |
                                    EHCI_INSNREG05_set_VCtrl(((0xf0 & addr) >> 4)));
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

   // clear P1_VLOADM, P2_VLOADM
   WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                 (~ WRAP_VLoadM1_bit_nEnable) &
                                 (~ WRAP_VLoadM2_bit_nEnable)) |
                                WRAP_VLoadM1_set_nEnable(0x1) |
                                WRAP_VLoadM2_set_nEnable(0x1));

   // set P1_VLOADM, P2_VLOADM
   WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                 (~ WRAP_VLoadM1_bit_nEnable) &
                                 (~ WRAP_VLoadM2_bit_nEnable)) |
                                WRAP_VLoadM1_set_nEnable(0x0) |
                                WRAP_VLoadM2_set_nEnable(0x0));

   // VControl1[3:0] = 0
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VCtrl)) |
                                    EHCI_INSNREG05_set_VCtrl(0x0));
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

   // clear output enable of VSTATUS[7:0]
   WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                 (~ WRAP_VStatusOut_bit_Enable)) |
                                 WRAP_VStatusOut_set_Enable(0x0));

   result = (data == (u8)EHCI_INSNREG05_get_VStatus(READ_REG_INT32U(EHCI_INSNREG05)));

   // clear VStatusOut1, the data will NOT be sent out as VSTATUS
   WRITE_REG_INT32U(WRAP_VStatusOut1, WRAP_VStatusOut_set_vstatus(0x0));

   // clear P1_VLOADM, P2_VLOADM
   WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                 (~ WRAP_VLoadM1_bit_nEnable) &
                                 (~ WRAP_VLoadM2_bit_nEnable)) |
                                WRAP_VLoadM1_set_nEnable(0x1) |
                                WRAP_VLoadM2_set_nEnable(0x1));

   return(result);
}

u8 USBPHY_GetReg(u8 port, u8 addr){

   u8 result;

   // valid port number : 1 & 2
   while((0 == port) || (3 <= port));

   if((USBPHY_30 == addr) | (USBPHY_31 == addr) | (USBPHY_32 == addr) |
      (USBPHY_33 == addr) | (USBPHY_34 == addr) | (USBPHY_35 == addr) |
      (USBPHY_36 == addr) | (USBPHY_38 == addr)){
      while(1){
         // invalid address access, these registers should not be read
      }
   }

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
   // VPort[3:0] = port
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VPort)) |
                                    EHCI_INSNREG05_set_VPort(port));

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
   // VCtrl = low nibble of addr
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VCtrl)) |
                                    EHCI_INSNREG05_set_VCtrl((0x0f & addr)));
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

   if (1 == port){
      // set P1_VLOADM
      WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                    (~ WRAP_VLoadM1_bit_nEnable)) |
                                   WRAP_VLoadM1_set_nEnable(0x0));
   } else {
      // set P2_VLOADM
      WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                    (~ WRAP_VLoadM2_bit_nEnable)) |
                                   WRAP_VLoadM2_set_nEnable(0x0));
   }

   // VCtrl = high nibble of addr
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VCtrl)) |
                                    EHCI_INSNREG05_set_VCtrl(((0xf0 & addr) >> 4)));
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

   if (1 == port){
      // clear P1_VLOADM
      WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                    (~ WRAP_VLoadM1_bit_nEnable)) |
                                   WRAP_VLoadM1_set_nEnable(0x1));

      // set P1_VLOADM
      WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                    (~ WRAP_VLoadM1_bit_nEnable)) |
                                   WRAP_VLoadM1_set_nEnable(0x0));
   } else {
      // clear P2_VLOADM
      WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                    (~ WRAP_VLoadM1_bit_nEnable) &
                                    (~ WRAP_VLoadM2_bit_nEnable)) |
                                   WRAP_VLoadM1_set_nEnable(0x1) |
                                   WRAP_VLoadM2_set_nEnable(0x1));

      // set P2_VLOADM
      WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                    (~ WRAP_VLoadM2_bit_nEnable)) |
                                   WRAP_VLoadM2_set_nEnable(0x0));
   }

#if 0
   // VLoadM = 0
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VLoadM)) |
                                    EHCI_INSNREG05_set_VLoadM(0x0));
   // polling until VBusy == 0, ensure the data is writen
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));
#endif

   // VLoadM = 1, VControl1[3:0] = 0
   WRITE_REG_INT32U(EHCI_INSNREG05, (READ_REG_INT32U(EHCI_INSNREG05) &
                                     (~ EHCI_INSNREG05_bit_VLoadM) &
                                     (~ EHCI_INSNREG05_bit_VCtrl)) |
                                    EHCI_INSNREG05_set_VLoadM(0x1) |
                                    EHCI_INSNREG05_set_VCtrl(0x0));
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(READ_REG_INT32U(EHCI_INSNREG05)));

   result = (u8)EHCI_INSNREG05_get_VStatus(READ_REG_INT32U(EHCI_INSNREG05));

   if (1 == port){
      // clear P1_VLOADM
      WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                    (~ WRAP_VLoadM1_bit_nEnable)) |
                                   WRAP_VLoadM1_set_nEnable(0x1));
   } else {
      // clear P2_VLOADM
      WRITE_REG_INT32U(WRAP_Dummy, (READ_REG_INT32U(WRAP_Dummy) &
                                    (~ WRAP_VLoadM2_bit_nEnable)) |
                                   WRAP_VLoadM2_set_nEnable(0x1));
   }

   return(result);
}

void USBPHY_Register_Setting(void){
   int param[0x10];

   param[0] = 0x5;		// reg 32 sh, POR = 5, 4bit;
   param[1] = 0x7;		// reg 35 src, POR = 7, 3bit;
   param[2] = 0xb;		// reg 36 senh, POR = 9, 4bit;
   param[3] = 0x8;		// reg 32 sen, POR = 8, 4bit;

   printk("[usb_param] setting for mars fpga\n");

   check_boot_code_param((int *)&param[0], 4, platform_info.usb_param);
  
   UTMI_VendorIF_Init();

   USBPHY_SetReg_Default_30();

   USBPHY_SetReg_Default_31();

   USBPHY_SetReg_Default_32(param[3], param[0]);

   USBPHY_SetReg_Default_33();

   USBPHY_SetReg_Default_34();

   USBPHY_SetReg_Default_35(param[1]);

   USBPHY_SetReg_Default_36(param[2]);

   USBPHY_SetReg_Default_38(0);

   printk("[usb_param] usbphy reg 32, set sh = 0x%x, get sh = 0x%x, 4bit\n", \
		   param[0], USBPHY_GetReg(1, USBPHY_22) & 0xf);
   printk("[usb_param] usbphy reg 35, set src = 0x%x, get src = 0x%x, 3bit\n", \
		   param[1], (USBPHY_GetReg(1, USBPHY_25)>>2) & 0x7);
   printk("[usb_param] usbphy reg 36, set senh = 0x%x, get senh = 0x%x, 4bit\n", \
		   param[2], (USBPHY_GetReg(1, USBPHY_26)>>4) & 0xf);
   printk("[usb_param] usbphy reg 32, set sen = 0x%x, get sen = 0x%x, 4bit\n", \
		   param[3], (USBPHY_GetReg(1, USBPHY_22)>>4) & 0xf);
}

#endif

EXPORT_SYMBOL (USBPHY_SetReg_Default_30);
EXPORT_SYMBOL (USBPHY_SetReg_Default_31);
EXPORT_SYMBOL (USBPHY_SetReg_Default_32);
EXPORT_SYMBOL (USBPHY_SetReg_Default_33);
EXPORT_SYMBOL (USBPHY_SetReg_Default_34);
EXPORT_SYMBOL (USBPHY_SetReg_Default_35);
EXPORT_SYMBOL (USBPHY_SetReg_Default_36);
EXPORT_SYMBOL (USBPHY_SetReg_Default_38);

EXPORT_SYMBOL (UTMI_VendorIF_Init);
EXPORT_SYMBOL (USBPHY_SetReg);
EXPORT_SYMBOL (USBPHY_GetReg);
EXPORT_SYMBOL (USBPHY_Register_Setting);

