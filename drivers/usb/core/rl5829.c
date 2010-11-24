#include <asm/io.h>
#include <rl5829_reg.h>
#include <ehci_reg.h>
#include <platform.h>

#define  WRAP_BASE                           0xb8013800
#define  WRAP_VStatusOut1                    (WRAP_BASE + 0x04)
#define  WRAP_VStatusOut2                    (WRAP_BASE + 0x24)
#define  WRAP_VStatusOut_set_vstatus(data)   (0x000000FF&(data))
#define  WRAP_Dummy                          (WRAP_BASE + 0x14)
#define  WRAP_VLoadM1_bit_nEnable             (0x00000004)
#define  WRAP_VLoadM1_set_nEnable(data)       (0x00000004&((data)<<2))
#define  WRAP_VLoadM2_bit_nEnable             (0x00000008)
#define  WRAP_VLoadM2_set_nEnable(data)       (0x00000008&((data)<<3))
#define  WRAP_VStatusOut_bit_Enable          (0x00000010)
#define  WRAP_VStatusOut_set_Enable(data)    (0x00000010&((data)<<4))

void check_boot_code_param(int *param0, int *param1, int *param2)
{
   int param[3], value, i = 0, index = 0;
   char *ptr = platform_info.usb_param;

   param[0] = *param0;
   param[1] = *param1;
   param[2] = *param2;

   if(ptr[0] != 0)
   {
	while((ptr[i] != 0) && (index < sizeof(param)))
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
			continue;
		}
		
		value = ptr[i] - 'a';
		if((value >= 0) && (value <=5))
		{
			param[index] = value + 10;
			i++;
			continue;
		}
		
		value = ptr[i] - 'A';
		if((value >= 0) && (value <=5))
		{
			param[index] = value + 10;
			i++;
			continue;
		}

		i++;
	}
   }

   *param0 = param[0];
   *param1 = param[1];
   *param2 = param[2];
 
}

char USBPHY_SetReg(u8 addr, u8 data);

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

void USBPHY_SetReg_Default_32(int sh)
{
   USBPHY_SetReg(USBPHY_32, USBPHY_32_set_SEN(0x8) |              // POR = 8
                            USBPHY_32_set_SH((sh & 0xf)));        // POR = 5, swing
}

void USBPHY_SetReg_Default_33(void)
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

void USBPHY_SetReg_Default_34(void)
{
   USBPHY_SetReg(USBPHY_34, USBPHY_34_set_TPA_EN(0x0) |      // POR = 0
                            USBPHY_34_set_TPB_EN(0x0) |      // POR = 0
                            USBPHY_34_set_xcvr_TS(0x0) |          // POR = 0
                            USBPHY_34_set_xcvr_SE(0x2));          // POR = 2
}

void USBPHY_SetReg_Default_35(int src)
{
   USBPHY_SetReg(USBPHY_35, USBPHY_35_set_TPC_EN(0x0) |          // POR = 0
                            USBPHY_35_set_xcvr_SP(0x0) |          // POR = 0
                            USBPHY_35_set_xcvr_SRC((src & 0x7)) | // POR = 7,  slew rate control
                            USBPHY_35_set_HSTESTEN(0x0) |         // POR = 0
                            USBPHY_35_set_xcvr_nsqdly(0x1));      // POR = 1
}

void USBPHY_SetReg_Default_36(int senh)
{
   USBPHY_SetReg(USBPHY_36, USBPHY_36_set_xcvr_senh((senh & 0xf)) | // POR = 9,  sensitivity for host, for HS disconnect detect (EOP) 
                            USBPHY_36_set_xcvr_adjr(0x8));          // POR = 8
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

#if 1
// for venus

void UTMI_VendorIF_Init(void){

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
   // VPort[3:0] = 0001, VLoadM = 1, VControl[3:0] = 0000
   writel((readl((u32 *) EHCI_INSNREG05) & 
   				(~ EHCI_INSNREG05_bit_VLoadM) &
   				(~ EHCI_INSNREG05_bit_VCtrl)) |
   				EHCI_INSNREG05_set_VPort(0x1) |
   				EHCI_INSNREG05_set_VLoadM(0x1) |
   				EHCI_INSNREG05_set_VCtrl(0x0),
   				(u32 *) EHCI_INSNREG05);
}

char USBPHY_SetReg(u8 addr, u8 data){

   // write data out first
   writel((u32)WRAP_VStatusOut_set_vstatus(data), (u32 *)WRAP_VStatusOut1);

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
   // VCtrl = low nibble of addr, VLoadM = 1
   writel((readl((u32 *) EHCI_INSNREG05) &
   				(~ EHCI_INSNREG05_bit_VLoadM) &
   				(~ EHCI_INSNREG05_bit_VCtrl)) |
   				EHCI_INSNREG05_set_VPort(0x1) |
   				EHCI_INSNREG05_set_VLoadM(0x1) |
   				EHCI_INSNREG05_set_VCtrl((0x0f & addr)),
   				(u32 *) EHCI_INSNREG05);
	 
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
   // VCtrl = low nibble of addr, VLoadM = 0
   writel((readl((u32 *) EHCI_INSNREG05) &
   				(~ EHCI_INSNREG05_bit_VLoadM) &
   				(~ EHCI_INSNREG05_bit_VCtrl)) |
   				EHCI_INSNREG05_set_VPort(0x1) |
   				EHCI_INSNREG05_set_VLoadM(0x0) |
   				EHCI_INSNREG05_set_VCtrl((0x0f & addr)),
   				(u32 *) EHCI_INSNREG05);
	 
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
   // VCtrl = high nibble of addr, VLoadM = 1
   writel((readl((u32 *) EHCI_INSNREG05) &
   				(~ EHCI_INSNREG05_bit_VLoadM) &
   				(~ EHCI_INSNREG05_bit_VCtrl)) |
   				EHCI_INSNREG05_set_VPort(0x1) |
   				EHCI_INSNREG05_set_VLoadM(0x1) |
   				EHCI_INSNREG05_set_VCtrl(((0xf0 & addr) >> 4)),
   				(u32 *) EHCI_INSNREG05);

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
   // VCtrl = high nibble of addr, VLoadM = 0
   writel((readl((u32 *) EHCI_INSNREG05) &
   				(~ EHCI_INSNREG05_bit_VLoadM) &
   				(~ EHCI_INSNREG05_bit_VCtrl)) |
   				EHCI_INSNREG05_set_VPort(0x1) |
   				EHCI_INSNREG05_set_VLoadM(0x0) |
   				EHCI_INSNREG05_set_VCtrl(((0xf0 & addr) >> 4)),
   				(u32 *) EHCI_INSNREG05);
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
}
	
u8 USBPHY_GetReg(u8 addr){
	
   if((USBPHY_30 == addr) | (USBPHY_31 == addr) | (USBPHY_32 == addr) |
      (USBPHY_33 == addr) | (USBPHY_34 == addr) | (USBPHY_35 == addr) |
      (USBPHY_36 == addr) | (USBPHY_38 == addr)){
      while(1){
         // invalid address access, these registers should not be read 
      }
   }
   
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
   // VCtrl = low nibble of addr, VLoadM = 1
   writel((readl((u32 *) EHCI_INSNREG05) &
   				(~ EHCI_INSNREG05_bit_VLoadM) &
   				(~ EHCI_INSNREG05_bit_VCtrl)) |
   				EHCI_INSNREG05_set_VPort(0x1) |
   				EHCI_INSNREG05_set_VLoadM(0x1) |
   				EHCI_INSNREG05_set_VCtrl(((0xf0 & addr) >> 4)),
   				(u32 *) EHCI_INSNREG05);
	 
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
   // VCtrl = low nibble of addr, VLoadM = 0
   writel((readl((u32 *) EHCI_INSNREG05) &
   				(~ EHCI_INSNREG05_bit_VLoadM) &
   				(~ EHCI_INSNREG05_bit_VCtrl)) |
   				EHCI_INSNREG05_set_VPort(0x1) |
   				EHCI_INSNREG05_set_VLoadM(0x0) |
   				EHCI_INSNREG05_set_VCtrl((0x0f & addr)),
   				(u32 *) EHCI_INSNREG05);
	 
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
   // VCtrl = high nibble of addr, VLoadM = 1
   writel((readl((u32 *) EHCI_INSNREG05) &
   				(~ EHCI_INSNREG05_bit_VLoadM) &
   				(~ EHCI_INSNREG05_bit_VCtrl)) |
   				EHCI_INSNREG05_set_VPort(0x1) |
   				EHCI_INSNREG05_set_VLoadM(0x1) |
   				EHCI_INSNREG05_set_VCtrl((0x0f & addr)),
   				(u32 *) EHCI_INSNREG05);
	 
   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));
   // VCtrl = high nibble of addr, VLoadM = 0
   writel((readl((u32 *) EHCI_INSNREG05) &
   				(~ EHCI_INSNREG05_bit_VLoadM) &
   				(~ EHCI_INSNREG05_bit_VCtrl)) |
   				EHCI_INSNREG05_set_VPort(0x1) |
   				EHCI_INSNREG05_set_VLoadM(0x0) |
   				EHCI_INSNREG05_set_VCtrl(((0xf0 & addr) >> 4)),
   				(u32 *) EHCI_INSNREG05);

   // polling until VBusy == 0
   while(EHCI_INSNREG05_get_VBusy(readl((u32 *) EHCI_INSNREG05)));

   return((u8)EHCI_INSNREG05_get_VStatus(readl((u32 *) EHCI_INSNREG05)));
}

void USBPHY_Register_Setting(void){
   int param[3];

   param[0] = 0x5;		// reg 32 sh, POR = 5, 4bit;
   param[1] = 0x7;		// reg 35 src, POR = 7, 3bit;
   param[2] = 0xd;		// reg 36 senh, POR = 9, 4bit;

   check_boot_code_param(&param[0], &param[1], &param[2]);
  
   UTMI_VendorIF_Init();

   USBPHY_SetReg_Default_30();

   USBPHY_SetReg_Default_31();

   USBPHY_SetReg_Default_32(param[0]);

   USBPHY_SetReg_Default_33();

   USBPHY_SetReg_Default_34();

   USBPHY_SetReg_Default_35(param[1]);

   USBPHY_SetReg_Default_36(param[2]);

   USBPHY_SetReg_Default_38(0);

   printk("[usb_param] usbphy reg 32, set sh = 0x%x, get sh = 0x%x, 4bit\n", \
		   param[0], USBPHY_GetReg(USBPHY_22) & 0xf);
   printk("[usb_param] usbphy reg 35, set src = 0x%x, get src = 0x%x, 3bit\n", \
		   param[1], (USBPHY_GetReg(USBPHY_25)>>2) & 0x7);
   printk("[usb_param] usbphy reg 36, set senh = 0x%x, get senh = 0x%x, 4bit\n", \
		   param[2], (USBPHY_GetReg(USBPHY_26)>>4) & 0xf);
}

EXPORT_SYMBOL (USBPHY_SetReg_Default_30);
EXPORT_SYMBOL (USBPHY_SetReg_Default_31);
EXPORT_SYMBOL (USBPHY_SetReg_Default_32);
EXPORT_SYMBOL (USBPHY_SetReg_Default_33);
EXPORT_SYMBOL (USBPHY_SetReg_Default_34);
EXPORT_SYMBOL (USBPHY_SetReg_Default_35);
EXPORT_SYMBOL (USBPHY_SetReg_Default_36);
EXPORT_SYMBOL (USBPHY_SetReg_Default_38);

#else
// for MARS FPGA
#define WRITE_REG_INT32U(addr, value)	writel((value),(u32 *)(addr))
#define READ_REG_INT32U(addr)		readl((u32 *)(addr))

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

   u8 tmp;
   int param[3];

   param[0] = 0x5;		// reg 32 sh, POR = 5, 4bit;
   param[1] = 0x7;		// reg 35 src, POR = 7, 3bit;
   param[2] = 0xb;		// reg 36 senh, POR = 9, 4bit;

   check_boot_code_param(&param[0], &param[1], &param[2]);
  
   UTMI_VendorIF_Init();

   USBPHY_SetReg_Default_30();

   USBPHY_SetReg_Default_31();

   USBPHY_SetReg_Default_32(param[0]);

   tmp = USBPHY_GetReg(1, USBPHY_21);

   USBPHY_SetReg_Default_33();

   USBPHY_SetReg_Default_34();

   USBPHY_SetReg_Default_35(param[1]);

   USBPHY_SetReg_Default_36(param[2]);

//   USBPHY_SetReg_Default_38(0);

   USBPHY_SetReg(USBPHY_38, USBPHY_38_set_dbnc_en(0x1) |          // POR = 1
                            USBPHY_38_set_discon_enable(0x1) |    // POR = 1
                            USBPHY_38_set_EN_ERR_UNDERRUN(0x1) |  // POR = 1
                            USBPHY_38_set_LATE_DLLEN(0x1) |       // POR = 1
                            USBPHY_38_set_INTG(0x0) |             // POR = 0
                            USBPHY_38_set_SOP_KK(0x1) |           // POR = 1
                            USBPHY_38_set_SLB_INNER(0x0) |        // POR = 0
                            USBPHY_38_set_SLB_EN(0x0));           // POR = 0

   tmp = USBPHY_GetReg(1, USBPHY_20);
   tmp = USBPHY_GetReg(1, USBPHY_21);
   tmp = USBPHY_GetReg(1, USBPHY_22);
   tmp = USBPHY_GetReg(1, USBPHY_23);
   tmp = USBPHY_GetReg(1, USBPHY_24);
   tmp = USBPHY_GetReg(1, USBPHY_25);
   tmp = USBPHY_GetReg(1, USBPHY_26);
   tmp = USBPHY_GetReg(1, USBPHY_28);
}

#endif

EXPORT_SYMBOL (UTMI_VendorIF_Init);
EXPORT_SYMBOL (USBPHY_SetReg);
EXPORT_SYMBOL (USBPHY_GetReg);
EXPORT_SYMBOL (USBPHY_Register_Setting);

