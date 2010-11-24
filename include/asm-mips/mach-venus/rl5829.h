#ifndef _RL5829_H_
#define _RL5829_H_

// porting by cfyeh : 2008/06/02
// add some extern funcgions

extern void UTMI_VendorIF_Init(void);
extern void USBPHY_SetReg_Default_31(void);
extern void USBPHY_SetReg_Default_32(int sen, int swing);
extern void USBPHY_SetReg_Default_33_LS(int port, int dr);
extern void USBPHY_SetReg_Default_38(int slb_en);
extern void USBPHY_SetReg_Default_39(void);
extern void USBPHY_SetReg_Default_39_for_JK(int port);
extern void USBPHY_SetReg_Default_3A(void);
extern void USBPHY_SetReg_Default_3A_for_JK(int port);
extern char USBPHY_SetReg(u8, u8);
extern u8 USBPHY_GetReg(u8, u8);
extern void USBPHY_Register_Setting(void);

#endif  //_RL5829_H_
