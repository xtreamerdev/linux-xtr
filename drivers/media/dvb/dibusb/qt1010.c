/* DTV tuner - Quantek qt1010 
 *
 * This file is the Driver of Tuner Qunatek QT1010
 * 
 *      [      DIBUSB       ]   application layer
 *                |
 *      [      qt1010.c     ]    
 *
 */
#include <linux/input.h>
#include <linux/config.h>

#include "dvb_frontend.h"
#include "qt1010.h"

typedef struct{
    unsigned char               tuner_addr;
    struct i2c_adapter*         i2c_adap;
    struct Chan_info_Type       strChan;
    struct X_RegSave_Type       X_RegSave;
    struct S_RegSave_Type       S_RegSave;
    struct A_Reg22Save_Type     A_Reg22Save;        
}qt1010_priv_t;


///////////////////////////////////////////////////////////////////////////
// QT1010 API for Linux DVB System
///////////////////////////////////////////////////////////////////////////

int quantek_cofdm_qt1010_init(
    struct dvb_frontend*    fe, 
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap)
{
    qt1010_priv_t*  pPriv;
    pPriv = (qt1010_priv_t*) kmalloc(sizeof(qt1010_priv_t),GFP_KERNEL);
    
    printk("Construct Tuner QT1010 \n");            
    
    if (pPriv != NULL)
    {            
        pPriv->tuner_addr = tuner_addr;
        pPriv->i2c_adap   = i2c_adap;        
        fe->tuner_priv    = pPriv;    
        qt1010_init(fe);        
        return 0;
    }
    else{
        printk("Construct Tuner QT1010 Failed\n");            
        return 1;
    }            
}

void quantek_cofdm_qt1010_uninit(struct dvb_frontend* fe)
{
    printk("Destruct Tuner QT1010 \n");            
    kfree(fe->tuner_priv);
}


int quantek_cofdm_qt1010_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep,
    unsigned char                       pllbuf[4])
{
    qt1010_channel_change(fe, (fep->frequency)/1000);
    return 0;
}


///////////////////////////////////////////////////////////////////////////
// QT1010 library
///////////////////////////////////////////////////////////////////////////

int QT_RI2C(struct dvb_frontend* fe, unsigned char reg_addr)
{ 
    struct i2c_msg msg[2];
    unsigned char  buf[2]= { reg_addr, 0 };
    qt1010_priv_t* pPriv = (qt1010_priv_t*) fe->tuner_priv;
        
    msg[0].addr  = pPriv->tuner_addr;
    msg[0].flags = 0;
    msg[0].buf   = &buf[0];
    msg[0].len   = 1;
    
    msg[1].addr  = pPriv->tuner_addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf   = &buf[1];
    msg[1].len   = 1;       
    
    if (2!= i2c_transfer(pPriv->i2c_adap, msg, 2)){
        printk("%s: readreg error reg=0x%x\n", __FUNCTION__, reg_addr);
    }
    return buf[1];    
}



void QT_WI2C(struct dvb_frontend* fe, unsigned char reg_addr, unsigned char val)
{        
    unsigned char   buf[2];
    struct i2c_msg  msg;
    qt1010_priv_t*  pPriv = (qt1010_priv_t*) fe->tuner_priv;
    
    buf[0]    = reg_addr;
    buf[1]    = val;
    msg.addr  = pPriv->tuner_addr;
    msg.flags = 0;
    msg.buf   = buf;
    msg.len   = 2 ;    
    
    if (1!= i2c_transfer(pPriv->i2c_adap, &msg, 1))
        printk("%s write to reg 0x%x failed\n", __FUNCTION__, reg_addr);

}


void qt1010_init(struct dvb_frontend* fe)
{
    // Richard: fix the parameter
    const int           flgLNA= LNA;
    const int           flgCLK= Crystal;
    
    int                 TimeOut = 0;
    int                 R_Data = 0;
    qt1010_priv_t*      pPriv = (qt1010_priv_t*) fe->tuner_priv;

    QT_WI2C(fe, 0x01,0x80);
    QT_WI2C(fe, 0x0D,0x84);
    
    if(flgLNA == LNA)
        QT_WI2C(fe, 0x0E,0xB7);
    else
        QT_WI2C(fe, 0x0E,0xB4);
        
    QT_WI2C(fe, 0x2A,0x23);
    QT_WI2C(fe, 0x2C,0xDC);
    QT_WI2C(fe, 0x25,0x40);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0x81);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x25);
        if(R_Data & 0x80) break;//If bit 7 is 1 of the R_Data must exit for loop.
    }
    pPriv->S_RegSave.S_Reg25 = QT_RI2C(fe, 0x25);

    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x2A,0x23);
    QT_WI2C(fe, 0x2B,0x70);
    QT_WI2C(fe, 0x26,0x08);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0x82);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x26);
        if(R_Data & 0x10) break;//If bit 4 is 1 of the R_Data must exit for loop.
    }
    pPriv->S_RegSave.S_Reg26 =  QT_RI2C(fe, 0x26);

    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x05,0x14);
    QT_WI2C(fe, 0x06,0x44);
    QT_WI2C(fe, 0x07,0x28);
    QT_WI2C(fe, 0x08,0x0B);
    if(flgCLK == Oscillator)
        QT_WI2C(fe, 0x11,(0xF9 & 0x0F));
    else
        QT_WI2C(fe, 0x11,0xF9);
    QT_WI2C(fe, 0x22,0x0D);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x06,0x40);
    QT_WI2C(fe, 0x16,0xF0);
    QT_WI2C(fe, 0x02,0x38);
    QT_WI2C(fe, 0x03,0x18);
    QT_WI2C(fe, 0x1F,0x20);
    QT_WI2C(fe, 0x20,0xE0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0x84);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x1F);
        if(R_Data & 0x40) break;//If bit 6 is 1 of the R_Data must exit for loop.
    }
    pPriv->S_RegSave.S_Reg1F = QT_RI2C(fe, 0x1F);
    pPriv->S_RegSave.S_Reg20 = QT_RI2C(fe, 0x20);

    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x03,0x19);
    QT_WI2C(fe, 0x02,0x3F);
    QT_WI2C(fe, 0x21,0x53);
    
    pPriv->S_RegSave.S_Reg21 = QT_RI2C(fe, 0x21);

    if(flgCLK == Oscillator)
        QT_WI2C(fe, 0x11,(0xFD & 0x0F));
    else
        QT_WI2C(fe, 0x11,0xFD);

    QT_WI2C(fe, 0x05,0x34);
    QT_WI2C(fe, 0x06,0x44);
    QT_WI2C(fe, 0x07,0x31);
    QT_WI2C(fe, 0x08,0x08);
    QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);
    
    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->A_Reg22Save.FirReg22 = QT_RI2C(fe, 0x22);

    QT_WI2C(fe, 0x07,0x32);
    QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->A_Reg22Save.SecReg22 = QT_RI2C(fe, 0x22);

    QT_WI2C(fe, 0x07,0x33);
    QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->A_Reg22Save.ThiReg22 = QT_RI2C(fe, 0x22);

    QT_WI2C(fe, 0x07,0x34);
    QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->A_Reg22Save.FouReg22 = QT_RI2C(fe, 0x22);

    QT_WI2C(fe, 0x07,0x35);
    QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->A_Reg22Save.FifReg22 = QT_RI2C(fe, 0x22);

    QT_WI2C(fe, 0x07,0x36);
    QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->A_Reg22Save.SixReg22 = QT_RI2C(fe, 0x22);

    QT_WI2C(fe, 0x07,0x37);
    QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->A_Reg22Save.SevReg22 = QT_RI2C(fe, 0x22);

    QT_WI2C(fe, 0x07,0x38);
    QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->A_Reg22Save.EigReg22 = QT_RI2C(fe, 0x22);

    QT_WI2C(fe, 0x07,0x39);
    QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->A_Reg22Save.NinReg22 = QT_RI2C(fe, 0x22);
}



int qt1010_channel_change(struct dvb_frontend* fe, unsigned long RF)
{
    qt1010_priv_t* pPriv = (qt1010_priv_t*) fe->tuner_priv;
    // Richard: fix the parameter
    // FIXME: Richard: checking the RF here 
    Ch_Chage(fe, RF, 36125, Normal, Crystal);
    if(pPriv->X_RegSave.X_Reg22 == 0xFF)
        Ch_Chage(fe, RF, 36125, SwitchBand, Crystal);
        
    if(pPriv->X_RegSave.X_Reg23 == 0xE0)
        Ch_Chage(fe, RF, 36125, Offset_E0, Crystal);
        
    if(pPriv->X_RegSave.X_Reg23 == 0xFF)
        Ch_Chage(fe, RF, 36125, Offset_FF, Crystal);
    return 0;
}


int Ch_Compute(struct dvb_frontend* fe, unsigned long RF, unsigned long IF, int flgOffset)
{
    unsigned long IF1 = 1232000000;
    unsigned long IF2 = 0;
    unsigned long LO2 = 920000000;
    unsigned long LO1 = 0;
    unsigned long LO3 = 0;
    unsigned long Div1 = 0;
    unsigned long Div2 =0;
    unsigned long Div3 = 0;
    unsigned long CheckTemp = 0;
    unsigned long NF = 0;
        
    int A1 = 0;
    int B1 = 0;
    int A2 = 0;
    int B2 = 0;
    int N3 = 0;
    int Band1 = 1600000000;
    int Band2 = 1856000000;
    int Band3 = 2080000000;    
    qt1010_priv_t* pPriv = (qt1010_priv_t*) fe->tuner_priv;

    
    RF *= 1000;
    IF *= 1000;
    if(RF < 60000000 || RF > 900000000) return 0; // Richard // return false;

    //Operation Register
    Div1 = (int)(500000 + (RF + IF1) / 4) / 1000000;
    Div1 *= 1000000;
    A1 = (int)(Div1 / 8000000);
    B1 = (int)((Div1 % 8000000) / 1000000); 
    LO1 = RF + IF1 ;
    IF1 = Div1 * 4 - RF;

    Div2 = (LO2 / 4000000) * 1000000;
    A2 = (int)(Div2 / 8000000);
    B2 = (int)((Div2 % 8000000)  / 1000000);
    IF2 = IF1 - LO2;

    LO3 = IF2 - IF;
    Div3 = LO3 / 4;
    N3 = (int)(LO3 / 4000000);
    NF = (int)((((Div3 - N3 * 1000000) * 256) + 500000) / 1000000);

    pPriv->strChan.Reg0B = N3;
    pPriv->strChan.Reg0C = 0xE1;
    
    if(NF > 255) 
        pPriv->strChan.Reg1A = NF % 256;
    else
        pPriv->strChan.Reg1A = (int)NF;

    pPriv->strChan.Reg1B = 0x00;
    pPriv->strChan.Reg1C = 137 + (int)(NF / 256) * 2;
    pPriv->strChan.B1 = B1;

    if(flgOffset == Offset_E0)
    {
        if(B1 > 0)
            Div1 = (Div1 / 1000000) + (8 - B1);
        else
            Div1 = (Div1 / 1000000) + 8;
    }
    else if(flgOffset == Offset_FF)
    {
        if(B1 > 0)
            Div1 = (Div1 / 1000000) - B1;
        else
            Div1 = (Div1 / 1000000) - 8;
    }
    else
    {
        if(B1 >= 5)
            Div1 = (Div1 / 1000000) + (8 - B1);
        else
            Div1 = (Div1 / 1000000) - B1;
    }

    A1 = (int)(Div1 / 8);
    B1 = (int)(Div1 % 8);

    pPriv->strChan.Reg07 = A1;
    pPriv->strChan.Reg08 = B1 + 8;

    IF1 = Div1 * 4 - (RF / 1000000);
    LO1 = RF + IF1 * 1000000;
    pPriv->strChan.LO1 = (int)LO1;
    LO2 = (int)(IF1 * 1000000 - IF2);
    Div2 = (int)(LO2 / 4000000);
    A2 = (int)(Div2 / 8);
    B2 = (int)(Div2 % 8);

    pPriv->strChan.Reg09 = A2;
    pPriv->strChan.Reg0A = B2 + 8;

    CheckTemp = (8 * A1 + B1) * 4000000  - RF - 
                (8 * A2 + B2) * 4000000 -
                (N3 * 1000000 + (NF * 1000000 / 256)) * 4;
    if(IF != CheckTemp)
        return 0; // Richard
        // return false;

    //Change Band Setting
    pPriv->strChan.CReg05 = 0x14;
    pPriv->strChan.CReg11 = 0xF9;
    pPriv->strChan.Reg05 = 0x10;
    if(pPriv->strChan.LO1 > Band1 && pPriv->strChan.LO1 <= Band2)
    {
        pPriv->strChan.CReg05 = 0x34;
        pPriv->strChan.CReg11 = 0xFD;
        pPriv->strChan.Reg05 = 0x30;
    }
    else if(pPriv->strChan.LO1 > Band2 && pPriv->strChan.LO1 <= Band3)
    {
        pPriv->strChan.CReg05 = 0x54;
        pPriv->strChan.CReg11 = 0xF9;
        pPriv->strChan.Reg05 = 0x50;
    }
    else if(pPriv->strChan.LO1 > Band3)
    {
        pPriv->strChan.CReg05 = 0x74;
        pPriv->strChan.CReg11 = 0xF9;
        pPriv->strChan.Reg05 = 0x70;
    }

    if(flgOffset == Offset_E0)
    {
        switch(pPriv->strChan.B1)
        {
        case 0:
            pPriv->strChan.Reg1F = 0x09;
            pPriv->strChan.Reg20 = 0x0A;
            break;
        case 1:
            pPriv->strChan.Reg1F = 0x0A;
            pPriv->strChan.Reg20 = 0x0A;
            break;
        case 2:
            pPriv->strChan.Reg1F = 0x0A;
            pPriv->strChan.Reg20 = 0x0B;
            break;
        case 3:
            pPriv->strChan.Reg1F = 0x0B;
            pPriv->strChan.Reg20 = 0x0C;
            break;
        case 4:
            pPriv->strChan.Reg1F = 0x0C;
            pPriv->strChan.Reg20 = 0x0C;
            break;
        case 5:
            pPriv->strChan.Reg1F = 0x0D;
            pPriv->strChan.Reg20 = 0x0D;
            break;
        case 6:
            pPriv->strChan.Reg1F = 0x0D;
            pPriv->strChan.Reg20 = 0x0D;
            break;
        case 7:
            pPriv->strChan.Reg1F = 0x0F;
            pPriv->strChan.Reg20 = 0x0E;
            break;
        default:
            pPriv->strChan.Reg1F = 0x00;
            pPriv->strChan.Reg20 = 0x00;
            break;

        }
    }
    else if(flgOffset == Offset_FF)
    {
        switch(pPriv->strChan.B1)
        {
        case 0:
            pPriv->strChan.Reg1F = 0x16;
            pPriv->strChan.Reg20 = 0x15;
            break;
        case 1:
            pPriv->strChan.Reg1F = 0x11;
            pPriv->strChan.Reg20 = 0x0F;
            break;
        case 2:
            pPriv->strChan.Reg1F = 0x11;
            pPriv->strChan.Reg20 = 0x10;
            break;
        case 3:
            pPriv->strChan.Reg1F = 0x12;
            pPriv->strChan.Reg20 = 0x11;
            break;
        case 4:
            pPriv->strChan.Reg1F = 0x13;
            pPriv->strChan.Reg20 = 0x12;
            break;
        case 5:
            pPriv->strChan.Reg1F = 0x14;
            pPriv->strChan.Reg20 = 0x13;
            break;
        case 6:
            pPriv->strChan.Reg1F = 0x15;
            pPriv->strChan.Reg20 = 0x13;
            break;
        case 7:
            pPriv->strChan.Reg1F = 0x16;
            pPriv->strChan.Reg20 = 0x14;
            break;
        default:
            pPriv->strChan.Reg1F = 0x00;
            pPriv->strChan.Reg20 = 0x00;
            break;

        }
    }
    else
    {
        switch(pPriv->strChan.B1)
        {
        case 0:
            pPriv->strChan.Reg1F = 0x10;
            pPriv->strChan.Reg20 = 0x0F;
            break;
        case 1:
            pPriv->strChan.Reg1F = 0x11;
            pPriv->strChan.Reg20 = 0x0F;
            break;
        case 2:
            pPriv->strChan.Reg1F = 0x11;
            pPriv->strChan.Reg20 = 0x10;
            break;
        case 3:
            pPriv->strChan.Reg1F = 0x12;
            pPriv->strChan.Reg20 = 0x11;
            break;
        case 4:
            pPriv->strChan.Reg1F = 0x13;
            pPriv->strChan.Reg20 = 0x12;
            break;
        case 5:
            pPriv->strChan.Reg1F = 0x0D;
            pPriv->strChan.Reg20 = 0x0D;
            break;
        case 6:
            pPriv->strChan.Reg1F = 0x0D;
            pPriv->strChan.Reg20 = 0x0D;
            break;
        case 7:
            pPriv->strChan.Reg1F = 0x0F;
            pPriv->strChan.Reg20 = 0x0E;
            break;
        default:
            pPriv->strChan.Reg1F = 0x00;
            pPriv->strChan.Reg20 = 0x00;
            break;

        }
    }

    pPriv->strChan.LO1 /= 1000000;
    switch(pPriv->strChan.LO1)
    {
    case 1568:
        if(pPriv->A_Reg22Save.FirReg22 < 0xD0)
        {
            return 0;
            // return false;
            break;
        }
        else
        {   
            pPriv->strChan.Reg22 = pPriv->A_Reg22Save.FirReg22;
            break;
        }
    case 1600:
        if(pPriv->A_Reg22Save.SecReg22 < 0xD0)
        {
            return 0;
            // return false;
            break;
        }
        else
        {   
            pPriv->strChan.Reg22 = pPriv->A_Reg22Save.SecReg22;
            break;
        }
    case 1632:
        if(pPriv->A_Reg22Save.ThiReg22 < 0xD0)
        {
            return 0;
            // return false;
            break;
        }
        else
        {   
            pPriv->strChan.Reg22 = pPriv->A_Reg22Save.ThiReg22;
            break;
        }
    case 1664:
        if(pPriv->A_Reg22Save.FouReg22 < 0xD0)
        {
            return 0;
            // return false;
            break;
        }
        else
        {   
            pPriv->strChan.Reg22 = pPriv->A_Reg22Save.FouReg22;
            break;
        }
    case 1696:
        if(pPriv->A_Reg22Save.FifReg22 < 0xD0)
        {
            return 0;
            // return false;
            break;
        }
        else
        {   
            pPriv->strChan.Reg22 = pPriv->A_Reg22Save.FifReg22;
            break;
        }
    case 1728:
        if(pPriv->A_Reg22Save.SixReg22 < 0xD0)
        {
            return 0;
            // return false;
            break;
        }
        else
        {   
            pPriv->strChan.Reg22 = pPriv->A_Reg22Save.SixReg22;
            break;
        }
    case 1760:
        if(pPriv->A_Reg22Save.SevReg22 < 0xD0)
        {
            return 0;
            // return false;
            break;
        }
        else
        {   
            pPriv->strChan.Reg22 = pPriv->A_Reg22Save.SevReg22;
            break;
        }
    case 1792:
        if(pPriv->A_Reg22Save.EigReg22 < 0xD0)
        {
            return 0;
            // return false;
            break;
        }
        else
        {   
            pPriv->strChan.Reg22 = pPriv->A_Reg22Save.EigReg22;
            break;
        }
    case 1824:
        if(pPriv->A_Reg22Save.NinReg22 < 0xD0)
        {
            return 0;
            // return false;
            break;
        }
        else
        {   
            pPriv->strChan.Reg22 = pPriv->A_Reg22Save.NinReg22;
            break;
        }
    default:
        pPriv->strChan.Reg22 = 0xD0;
        break;
    }
    return 1;
    // return true;
}



int Ch_Chage(struct dvb_frontend*        fe,
                    unsigned long        RF,
                    unsigned long        IF,
                    int                  flgState,
                    int                  flgCLK)
{
    int R_Data = 0;
    int TimeOut = 0;
    qt1010_priv_t* pPriv = (qt1010_priv_t*) fe->tuner_priv;

    if(!(Ch_Compute(fe,RF,IF,flgState)))
        return 1;

    QT_WI2C(fe, 0x01,0x80);
    QT_WI2C(fe, 0x02,0x3F);

    if(flgState == SwitchBand)
    {
        pPriv->strChan.CReg05 = pPriv->X_RegSave.X_Reg05 + 0x20;
        QT_WI2C(fe, 0x05,pPriv->strChan.CReg05);
    }
    else
    {
        QT_WI2C(fe, 0x05,pPriv->strChan.CReg05);
    }

    QT_WI2C(fe, 0x06,0x44);
    QT_WI2C(fe, 0x07,pPriv->strChan.Reg07);
    QT_WI2C(fe, 0x08,pPriv->strChan.Reg08);
    QT_WI2C(fe, 0x09,pPriv->strChan.Reg09);
    QT_WI2C(fe, 0x0A,pPriv->strChan.Reg0A);
    QT_WI2C(fe, 0x0B,pPriv->strChan.Reg0B);
    QT_WI2C(fe, 0x0C,pPriv->strChan.Reg0C);
    QT_WI2C(fe, 0x1A,pPriv->strChan.Reg1A);
    QT_WI2C(fe, 0x1B,pPriv->strChan.Reg1B);
    QT_WI2C(fe, 0x1C,pPriv->strChan.Reg1C);

    if(flgState == SwitchBand && pPriv->X_RegSave.X_Reg05 == 0x14)
        if(flgCLK == Oscillator)
            QT_WI2C(fe, 0x11,(0xFD & 0x0F));
        else
            QT_WI2C(fe, 0x11,0xFD);
    else if(flgState == SwitchBand && pPriv->X_RegSave.X_Reg05 != 0x14)
        if(flgCLK == Oscillator)
            QT_WI2C(fe, 0x11,(0xF9 & 0x0F));
        else
            QT_WI2C(fe, 0x11,0xF9);
    else
        if(flgCLK == Oscillator)
            QT_WI2C(fe, 0x11,(pPriv->strChan.CReg11 & 0x0F));
        else
            QT_WI2C(fe, 0x11,pPriv->strChan.CReg11);

    QT_WI2C(fe, 0x12,0x91);

    if(pPriv->strChan.CReg05 == 0x34)
    {
        if (pPriv->strChan.Reg22 < 0xF0)
            pPriv->strChan.Reg22 = 0xD0;
        else if(pPriv->strChan.Reg22 > 0xFA)
            pPriv->strChan.Reg22 = 0xDA;
        else
            pPriv->strChan.Reg22 -= 32;
        QT_WI2C(fe, 0x22,pPriv->strChan.Reg22);
    }
    else
        QT_WI2C(fe, 0x22,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xD0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x22);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    pPriv->X_RegSave.X_Reg05 = QT_RI2C(fe, 0x05);
    pPriv->X_RegSave.X_Reg22 = QT_RI2C(fe, 0x22);
    QT_WI2C(fe, 0x23,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xE0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x23);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }
    pPriv->X_RegSave.X_Reg23 = QT_RI2C(fe, 0x23);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x24,0xD0);
    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x1E,0xF0);

    for(TimeOut = 0;TimeOut <= 30;TimeOut ++)//If overtake 30ms must timeout.
    {
        R_Data = QT_RI2C(fe, 0x24);
        if(R_Data & 0x20) break;//If bit 5 is 1 of the R_Data must exit for loop.
    }

    QT_WI2C(fe, 0x1E,0x00);
    QT_WI2C(fe, 0x14,0x7F);
    QT_WI2C(fe, 0x15,0x7F);

    if(flgState == SwitchBand)
    {
        pPriv->strChan.Reg05 = pPriv->X_RegSave.X_Reg05 - 0x04;
        QT_WI2C(fe, 0x05,pPriv->strChan.Reg05);
    }
    else
    {
        QT_WI2C(fe, 0x05,pPriv->strChan.Reg05);
    }

    QT_WI2C(fe, 0x06,0x00);
    QT_WI2C(fe, 0x15,0x1F);
    QT_WI2C(fe, 0x16,0xFF);
    QT_WI2C(fe, 0x18,0xFF);
    QT_WI2C(fe, 0x1F,(pPriv->S_RegSave.S_Reg1F + pPriv->strChan.Reg1F));
    QT_WI2C(fe, 0x20,(pPriv->S_RegSave.S_Reg20 + pPriv->strChan.Reg20));
    QT_WI2C(fe, 0x21,(pPriv->S_RegSave.S_Reg21 + 0x00));
    QT_WI2C(fe, 0x25,(pPriv->S_RegSave.S_Reg25 + 0x00));
    QT_WI2C(fe, 0x26,(pPriv->S_RegSave.S_Reg26 + 0x00));
    QT_WI2C(fe, 0x00,0x81);
    QT_WI2C(fe, 0x02,0x00);
    QT_WI2C(fe, 0x01,0x00);
    // Richard: add this return
    return 0;
}
