#ifndef _RL5829_H_
#define _RL5829_H_

#if 1
// for venus

extern void UTMI_VendorIF_Init(void);
extern void USBPHY_SetReg_Default_30(void);
extern void USBPHY_SetReg_Default_31(void);
extern void USBPHY_SetReg_Default_32(int swing);
extern void USBPHY_SetReg_Default_33(void);
extern void USBPHY_SetReg_Default_34(void);
extern void USBPHY_SetReg_Default_35(int src);
extern void USBPHY_SetReg_Default_36(int senh);
extern void USBPHY_SetReg_Default_38(int slb_en);
extern char USBPHY_SetReg(u8, u8);
extern u8 USBPHY_GetReg(u8);
extern void USBPHY_Register_Setting(void);

#else
// for MARS FPGA
extern void UTMI_VendorIF_Init(void);
extern char USBPHY_SetReg(u8, u8);
extern u8 USBPHY_GetReg(u8, u8);
extern void USBPHY_Register_Setting(void);

#endif

#endif  //_RL5829_H_
