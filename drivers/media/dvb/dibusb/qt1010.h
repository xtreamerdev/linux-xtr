
#ifndef __QT1010_H__
#define __QT1010_H__

typedef struct X_RegSave_Type
{
    unsigned int        X_Reg05;
    unsigned int        X_Reg22;
    unsigned int        X_Reg23;
}X_RegSave_Type;

typedef struct Chan_info_Type
{
    int                 Reg05;
    int                 Reg07;
    int                 Reg08;
    int                 Reg09;
    int                 Reg0A;
    int                 Reg0B;
    int                 Reg0C;
    int                 Reg1A;
    int                 Reg1B;
    int                 Reg1C;
    int                 Reg22;
    int                 Reg1F;
    int                 Reg20;
    int                 CReg05;
    int                 CReg11;
    int                 Reg23;
    int                 Reg12;
    int                 B1;
    int                 LO1;
}Chan_info_Type;

typedef struct S_RegSave_Type
{
    unsigned int S_Reg1F;
    unsigned int S_Reg20;
    unsigned int S_Reg21;
    unsigned int S_Reg25;
    unsigned int S_Reg26;
}S_RegSave_Type;

typedef struct A_Reg22Save_Type
{
    unsigned int FirReg22;
    unsigned int SecReg22;
    unsigned int ThiReg22;
    unsigned int FouReg22;
    unsigned int FifReg22;
    unsigned int SixReg22;
    unsigned int SevReg22;
    unsigned int EigReg22;
    unsigned int NinReg22;
}A_Reg22Save_Type;



#define LNA             1
#define NonLNA          0

#define Crystal         0
#define Oscillator      1

#define Normal          0x00
#define SwitchBand      0x01
#define Offset_E0       0x02
#define Offset_FF       0x03


void qt1010_init(struct dvb_frontend* fe);
int  QT_RI2C (struct dvb_frontend* fe, unsigned char reg_addr);
void QT_WI2C (struct dvb_frontend* fe, unsigned char reg_addr, unsigned char val);
int  Ch_Chage(struct dvb_frontend* fe, 
              unsigned long        RF,
              unsigned long        IF,
              int                  flgState,
              int                  flgCLK);
int Ch_Compute(struct dvb_frontend* fe,unsigned long RF, unsigned long IF, int flgOffset);                     
int qt1010_channel_change(struct dvb_frontend* fe, unsigned long RF);



///////////////////////////////////////////////////////////////////////////
// QT1010 API for Linux DVB System
///////////////////////////////////////////////////////////////////////////
int quantek_cofdm_qt1010_init(
     struct dvb_frontend*       fe,
     unsigned char              tuner_addr, 
     struct i2c_adapter*        i2c_adap);
    
int quantek_cofdm_qt1010_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep,
    unsigned char                       pllbuf[4]);
    
void quantek_cofdm_qt1010_uninit(struct dvb_frontend* fe);    
   
#endif //__QT1010_H__   
    
