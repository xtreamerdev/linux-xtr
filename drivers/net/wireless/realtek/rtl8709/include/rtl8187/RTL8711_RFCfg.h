
#ifndef _RTL8711_RFCFG_H_
#define _RTL8711_RFCFG_H_

#include <rtl8711_spec.h>



#ifndef _RTL8711_RFCFG_C_

extern void set_cpu_rate(unsigned rate);
extern void delay(unsigned int ms);
extern void rf_init(void);
extern void RTL8711RF_Cfg(unsigned char channel); 
extern void SelectChannel( _adapter *padapter,unsigned char channel);
extern void RTL8225_Config(void);
extern void OFDM_Config(void);
extern void RFSerialWrite(ULONG, UCHAR, UCHAR);
extern void bb_write(unsigned char, unsigned char);
extern unsigned char  bb_read(unsigned char);
extern void dump_phy_tables(UCHAR);
extern void SwChnl( _adapter *padapter,UCHAR channel);

extern ULONG RF_ReadReg(UCHAR offset);
extern void RF_WriteReg(UCHAR, UCHAR, ULONG);

#else


#endif



#endif // _RTL8711_RFCFG_H_





