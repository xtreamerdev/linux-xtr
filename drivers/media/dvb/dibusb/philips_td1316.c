#include <linux/input.h>
#include <linux/config.h>
#include <linux/delay.h>
#include "dvb_frontend.h"
#include "philips_td1316.h"
#include "../frontends/rtl2830.h"


//----------Internal Used Data ------------------------//

typedef struct { 
    unsigned long               i_ref_f;
    unsigned char               i_en_ext_agc;
    unsigned char               i_ext_dis_curr;
    unsigned char               i_en_7M_saw;
    unsigned long               i_freq_rf;
    unsigned long               i_freq_if;
    int                         i_int_agc_top;
    int                         i_int_agc_atc;
    int                         i_cp;
    int                         i_freq_band;        
}TD1316_PARAM;

typedef struct {
    unsigned char               tuner_addr;
    struct i2c_adapter*         i2c_adap;
    TD1316_PARAM                param;    
}TD1316_MODULE;



//----------Internal Used Function ------------------------//
int Tuner_opt_I2C_tuner_philips_TD1316 ( 
    TD1316_MODULE*      pTD1316,    
    unsigned short*     o_freq_div,
    unsigned char*      o_ctrB1_1, 
    unsigned char*      o_ctrB1_0, 
    unsigned char*      o_ctrB2);
    
int Tuner_Write_philips_TD1316(
    TD1316_MODULE*      pTD1316,    
    unsigned short      i_freq_div, 
    unsigned char       i_ctrB1, 
    unsigned char       i_ctrB2);    
    
//----------------------------------------------------------//    




int philips_cofdm_td1316_init(
    struct dvb_frontend*    fe,
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap)
{
    TD1316_MODULE*  pTD1316;
    pTD1316 = (TD1316_MODULE*) kmalloc(sizeof(TD1316_MODULE),GFP_KERNEL);
    
    printk("Construct Tuner TD1316 \n");            
    
    if (pTD1316 != NULL)
    {      
        pTD1316->tuner_addr= tuner_addr;
        pTD1316->i2c_adap  = i2c_adap;
        //----
        pTD1316->param.i_freq_rf      = 593000000;  // RF Freq = 545 MHz
        pTD1316->param.i_freq_if      = 36125000;   // IF Freq = 36.125 MHz   
        pTD1316->param.i_cp           = 130;
        pTD1316->param.i_freq_band    = 2;          //Band =0:LOW 1:MID 2:HIGH
        pTD1316->param.i_ref_f        = 166670;     //50K / 62.5K/ 166.67K
        pTD1316->param.i_en_7M_saw    = 0;          //0:8M 1:7M
        pTD1316->param.i_en_ext_agc   = 0;          //0:internal 1:External
        pTD1316->param.i_int_agc_top  = 124;        //124 ,121 ,118,115,112,109
        pTD1316->param.i_int_agc_atc  = 0;          //0 :fast tunin   1:normal mode
        pTD1316->param.i_ext_dis_curr = 1;          //0:off internal detector 1:off internal current source
        //----                        
        fe->tuner_priv = pTD1316;
        return 0;
    }
    else{
        printk("Construct Tuner TD1316 Failed\n");            
        return 1;    
    }
}

void philips_cofdm_td1316_uninit(struct dvb_frontend* fe)
{
    printk("Destruct Tuner TD1316 \n");            
    kfree(fe->tuner_priv);    
}



int philips_cofdm_td1316_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep
    )
{                  
    TD1316_MODULE*  pTD1316 = (TD1316_MODULE*) fe->tuner_priv;
    TD1316_PARAM*   pParm   = &pTD1316->param;    
    unsigned long   freq_osc;
    unsigned short  o_freq_div; // freq divider
    unsigned char   o_ctrB1_1;  // control byte 1 type 1
    unsigned char   o_ctrB1_0;  // control byte 1 type 0
    unsigned char   o_ctrB2;    // control byte 2
            
    pParm->i_freq_rf = fep->frequency;        
    pParm->i_en_7M_saw = 0; 
                
    printk("%s : %d :%s : RF Freq = %u, Bandwidth = %d MHz",__FILE__,__LINE__,__FUNCTION__,(unsigned int)pParm->i_freq_rf,fep->u.ofdm.bandwidth+6);                
                
    freq_osc  = pParm->i_freq_rf + pParm->i_freq_if;    
                    
    pParm->i_cp = (((freq_osc>=87000000)  && (freq_osc<130000000)) || ((freq_osc>=200000000) && (freq_osc<290000000)) || ((freq_osc>=480000000) && (freq_osc<620000000))) ? 130 :            
                  (((freq_osc>=130000000) && (freq_osc<160000000)) || ((freq_osc>=290000000) && (freq_osc<420000000)) || ((freq_osc>=620000000) && (freq_osc<830000000))) ? 280 :
                  (((freq_osc>=160000000) && (freq_osc<200000000)) || ((freq_osc>=420000000) && (freq_osc<480000000))) ? 410 :
                   ((freq_osc>=830000000) && (freq_osc<900000000)) ? 600 : 90;
    
    pParm->i_freq_band = (pParm->i_freq_rf>=49000000  && pParm->i_freq_rf< 162000000 ) ? 0 : 
                         (pParm->i_freq_rf>=162000000 && pParm->i_freq_rf< 448000000 ) ? 1 : 2;
            
    //----------------------------------------------------------------
    // compute parameters : Tuner_opt_I2C_tuner_philips_TD1316
                                        
    Tuner_opt_I2C_tuner_philips_TD1316 ( 
            pTD1316 ,            
            &o_freq_div, 
            &o_ctrB1_1,
            &o_ctrB1_0,
            &o_ctrB2);
            
    //---------------------------------------
    // set tuner                     
	Tuner_Write_philips_TD1316(
	        pTD1316, 
	        o_freq_div, 
	        o_ctrB1_1, 	        
	        o_ctrB2);
	        
    return 0;	        
    /* disable speed up */
    
	Tuner_Write_philips_TD1316(
	        pTD1316, 
	        o_freq_div, 	        
	        o_ctrB1_0, 
	        o_ctrB2);	        
	            
    return 0;                                 
}	




/*******************************************************************************
*   Name:Tuner_opt_I2C_tuner_philips_TD1316
*   Purpose:obtain settings of Philips tuner TD1316         
*   Remarks:i_xxx is for input parameters
            o_xxx is for output/return parameters
*   Inputs:
*   Outputs:
*******************************************************************************/
int Tuner_opt_I2C_tuner_philips_TD1316 ( 
    TD1316_MODULE*      pTD1316,    
    unsigned short*     o_freq_div,
    unsigned char*      o_ctrB1_1, 
    unsigned char*      o_ctrB1_0, 
    unsigned char*      o_ctrB2
    )
{
    TD1316_PARAM*   pParm = &pTD1316->param;        
    
    // get program divider byte 1 & 2 (PLL reference divider bits) and control byte 1 (D/A=1)                    
    switch(pParm->i_ref_f){
    case 50000 : *o_ctrB1_1 = 0xCB;  break;     //R2=0; R1=1; R0=1;                
    case 62500 : *o_ctrB1_1 = 0xC8;  break;     //R2=0; R1=0; R0=0;           
    case 166670: *o_ctrB1_1 = 0xCA;  break;     //R2=0; R1=1; R0=0;           
    default:
        printk("illegal i_ref_f = %u Hz, reset to default (166670Hz) !\n", (unsigned int) pParm->i_ref_f);
        pParm->i_ref_f = 166670;        
        *o_ctrB1_1 = 0xCA;  
    }
    
    *o_freq_div= (int) ( pParm->i_freq_rf + pParm->i_freq_if + (pParm->i_ref_f >>1) )/pParm->i_ref_f;
                
    // get control byte 1 (D/A=0)
    if (!pParm->i_en_ext_agc)
    {        
        switch(pParm->i_int_agc_top){
        case 124:   *o_ctrB1_0 = 0x80;      break;  //AL2=0; AL1=0; AL0=0;            
        case 121:   *o_ctrB1_0 = 0x81;      break;  //AL2=0; AL1=0; AL0=1;                           
        case 118:   *o_ctrB1_0 = 0x82;      break;  //AL2=0; AL1=1; AL0=0;                           
        case 115:   *o_ctrB1_0 = 0x83;      break;  //AL2=0; AL1=1; AL0=1;                           
        case 112:   *o_ctrB1_0 = 0x84;      break;  //AL2=1; AL1=0; AL0=0;            
        case 109:   *o_ctrB1_0 = 0x85;      break;  //AL2=1; AL1=0; AL0=1;                                       
        default:
            printk("illegal i_int_agc_top = %d , reset to default (124) !\n", pParm->i_int_agc_top);
            pParm->i_int_agc_top = 124;
            *o_ctrB1_0 = 0x80;                        
            break;
        }        
    }
    else{
        if(pParm->i_ext_dis_curr)	// disable internal current source        
            *o_ctrB1_0 = 0x86;             //AL2=1; AL1=1; AL0=0;    
        else		
            *o_ctrB1_0 = 0x87;             //AL2=1; AL1=1; AL0=1;                                 
    }
    
    // get control byte 2
    // PLL charge pump current settings        
    switch(pParm->i_cp){
    case 40:  *o_ctrB2 = 0x00;    break;     // CP2=0; CP1=0; CP0=0;        
    case 60:  *o_ctrB2 = 0x20;    break;     // CP2=0; CP1=0; CP0=1;        
    case 90:  *o_ctrB2 = 0x40;    break;     // CP2=0; CP1=1; CP0=0;        
    case 130: *o_ctrB2 = 0x60;    break;     // CP2=0; CP1=1; CP0=1;        
    case 190: *o_ctrB2 = 0x80;    break;     // CP2=1; CP1=0; CP0=0;        
    case 280: *o_ctrB2 = 0xA0;    break;     // CP2=1; CP1=0; CP0=1;        
    case 410: *o_ctrB2 = 0xC0;    break;     // CP2=1; CP1=1; CP0=0;                
    case 600: *o_ctrB2 = 0xE0;    break;     // CP2=1; CP1=1; CP0=1;        
    default:
        printk("illegal i_cp = %d , reset to default (40) !\n", pParm->i_cp);
        pParm->i_cp = 40;
        *o_ctrB2 = 0x00;                                
    }
        
    if (!pParm->i_en_7M_saw)
        *o_ctrB2 |= 0x08;       // SP4 = 1    
    
    switch(pParm->i_freq_band){
    case 0:   *o_ctrB2 |= 0x01;   break;    //   SP3=0; SP2=0; SP1=1;        
    case 1:   *o_ctrB2 |= 0x02;   break;    //   SP3=0; SP2=1; SP1=0;                
    case 2:   *o_ctrB2 |= 0x04;   break;    //   SP3=1; SP2=0; SP1=0;                       
    default:  
        printk("illegal i_freq_band = %d , reset to default (1) !\n", pParm->i_freq_band);
        pParm->i_freq_band = 1;
        *o_ctrB2 |= 0x02;          
        break;
    }        
    return 0;
} 



/*******************************************************************************
*   Name: write Philips tuner TD1316
*   Purpose:write Philips tuner TD1316        
*   Remarks:
*   Inputs:
*   Outputs:
*******************************************************************************/
int Tuner_Write_philips_TD1316(
    TD1316_MODULE*      pTD1316,    
    unsigned short      i_freq_div, 
    unsigned char       i_ctrB1, 
    unsigned char       i_ctrB2)
{
    struct i2c_msg msg;
	unsigned char I2Cdata[4];
    
	I2Cdata[0]= i_freq_div>>8;
	I2Cdata[1]= (unsigned char)(i_freq_div & 0xFF);
	I2Cdata[2]= i_ctrB1;
	I2Cdata[3]= i_ctrB2;
    
    printk("Tuner Addr = %02x\n",pTD1316->tuner_addr);
	printk("I2Cdata[0-3]=%02x %02x %02x %02x\n",I2Cdata[0],I2Cdata[1],I2Cdata[2],I2Cdata[3]);
	msg.addr  = pTD1316->tuner_addr;
	msg.flags = 0;
	msg.buf   = I2Cdata;
	msg.len   = 4;
  
    if (1!= i2c_transfer(pTD1316->i2c_adap, &msg, 1)){
        printk("%s write to reg failed\n", __FUNCTION__);
    }
    
	return 1;
}

