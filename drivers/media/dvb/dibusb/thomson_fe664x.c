#include <linux/input.h>
#include <linux/config.h>
#include "dvb_frontend.h"
#include "thomson_fe664x.h"



int thomson_cofdm_fe664x_init(
    struct dvb_frontend*    fe,
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap)
{
    FE664X_MODULE*  pFE664X;
    pFE664X = (FE664X_MODULE*) kmalloc(sizeof(FE664X_MODULE),GFP_KERNEL);
    
    printk("Construct Tuner FE664X \n");            
    
    if (pFE664X != NULL){      
        pFE664X->tuner_addr = tuner_addr;
        pFE664X->i2c_adap  = i2c_adap;
        fe->tuner_priv = pFE664X;
        return 0;
    }
    else{
        printk("Construct Tuner FE664X Failed\n");            
        return 1;    
    }
}

void thomson_cofdm_fe664x_uninit(struct dvb_frontend* fe)
{
    printk("Destruct Tuner FE664X \n");            
    kfree(fe->tuner_priv);    
}


int thomson_cofdm_fe664x_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep,
    unsigned char                       pllbuf[6])
{
    FE664X_MODULE* pFE664X = (FE664X_MODULE*) fe->tuner_priv;

    struct i2c_msg msg = {
        .addr  = pFE664X->tuner_addr,
        .flags = 0xffff,
        .buf   = pllbuf,
        .len   = 6 
    };
 
    if (1!= i2c_transfer(pFE664X->i2c_adap, &msg, 1)){
        printk("%s write to reg failed\n", __FUNCTION__);
    }
    return 0;
}




int thomson_fe664x_eu_pll_set(
    struct dvb_frontend_parameters*     fep, 
    unsigned char                       pllbuf[6])
{
#if 0    
    u32 freq_osc,o_freq_div;
    	
    freq_osc=fep->frequency/1000000+36;
    
    o_freq_div=freq_osc*1000000/166666;
    
    pllbuf[0]=(unsigned char)(o_freq_div>>8);
    pllbuf[1]=(unsigned char)o_freq_div;
    
    
    //                 Control Byte
    //       - 185MHz      0xB7
    //186MHz - 465MHz      0xB4
    //466MHz - 734MHz      0xBC
    //735MHz - 834MHz      0xF4
    //835MHz -             0xFC
    
    if(freq_osc<=185)          
        pllbuf[2]=0xb7;
    else if(freq_osc<=465 )
        pllbuf[2]=0xb4;
    else if(freq_osc<=734 )
        pllbuf[2]=0xbC;
    else if(freq_osc<=834 )
        pllbuf[2]=0xf4;
    else
        pllbuf[2]=0xfc;
    //                 Bandswitch Byte
    //       - 185MHz      0x1
    //186MHz - 465MHz      0x2
    //466MHz -             0x8
    
    if(freq_osc<=185)          
        pllbuf[3]=0x1;
    else if(freq_osc<=465 )
        pllbuf[3]=0x2;
    else
        pllbuf[3]=0x8;
    
    pllbuf[4]=0xa0;	
    pllbuf[5]=fep->u.ofdm.bandwidth;
	
#else	

    u32 freq_osc,o_freq_div;
    	
    freq_osc=fep->frequency+36000000;
    
    o_freq_div=freq_osc/166666;
        
    pllbuf[0]=(unsigned char)(o_freq_div>>8);
    pllbuf[1]=(unsigned char)o_freq_div;
    
    
    //                 Control Byte
    //       - 185MHz      0xB7
    //186MHz - 465MHz      0xB4
    //466MHz - 734MHz      0xBC
    //735MHz - 834MHz      0xF4
    //835MHz -             0xFC
    
    if(freq_osc<185200000)          
        pllbuf[2]=0xb7;
    else if(freq_osc<465200000 )
        pllbuf[2]=0xb4;
    else if(freq_osc<735000000 )
        pllbuf[2]=0xbC;
    else if(freq_osc<835000000 )
        pllbuf[2]=0xf4;
    else
        pllbuf[2]=0xfc;
    //                 Bandswitch Byte
    //       - 185MHz      0x1
    //186MHz - 465MHz      0x2
    //466MHz -             0x8
    
    if(freq_osc<=185200000)          
        pllbuf[3]=0x1;
    else if(freq_osc<465200000 )
        pllbuf[3]=0x2;
    else
        pllbuf[3]=0x8;
    
    pllbuf[4]=0xa0;	
    pllbuf[5]=fep->u.ofdm.bandwidth;
	
#endif	

	return 0;
}	


