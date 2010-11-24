/* DTV tuner - TDTM-G252D 
 *
 * This file is a link layer between dibusb and mt2060 api 
 * 
 *      [      DIBUSB       ]   application layer
 *                |
 *      [ tuner_tdtmg252d.h]   link layer 
 *
 */


#include <linux/slab.h>
#include "dvb_frontend.h"
#include "tdtm_g252d.h"

//---------------------------------------------
//  Internal Used Function
int lg_tdtm_g252d_LatchRegBytes(
    G252D_MODULE*       pG252D_Module
    );
    
int lg_tdtm_g252d_SetRfFreqHz(
    G252D_MODULE*       pG252D_Module, 
    unsigned long       TargetRfFreqHz
	);
		
int lg_tdtm_g252d_SetBandwidthMode( 
    G252D_MODULE*       pG252D_Module, 
    int                 BandwidthMode
    );		
	
//---------------------------------------------


/***************************************************************************** 
 * Func : lg_tdtm_g252d_init 
 *
 * Desc : init LG tdtm g252d tuner
 *
 * Parm : hFE      : handle of dvb_frontend
 *        pllbuf   : not used  
 *
 * Retn : 0 for success, others for fail 
 *
 *****************************************************************************/ 
int lg_tdtm_g252d_init(
    void*                   hFE, 
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap
    )
{              
    struct dvb_frontend* fe = (struct dvb_frontend*) hFE;          
    G252D_MODULE* pG252D_Module = NULL;   
    
    if (fe->tuner_priv == NULL)
    {
        G252D_DBG("LG_TDTM_252D Module Init....\n");                
        
        pG252D_Module = kmalloc(sizeof(G252D_MODULE), GFP_KERNEL);
        if (pG252D_Module == NULL){
            G252D_DBG("[ERROR][FETAL] : allocate LG_TDTM_G252D module memory fail\n");
            goto error_status_allocate_extra_module_memory;
        }
                                        
        fe->tuner_priv = (void*) pG252D_Module; 
                
        pG252D_Module->tuner_addr = tuner_addr;
        pG252D_Module->i2c_adap   = i2c_adap;
        pG252D_Module->TargetRF   = 545000000;
        pG252D_Module->Bandwidth  = BANDWIDTH_7_MHZ;
        pG252D_Module->DividerMsb = DIVIDER_MSB_DEFAULT;
	    pG252D_Module->DividerLsb = DIVIDER_LSB_DEFAULT;
	    pG252D_Module->Control    = CONTROL_DEFAULT;
	    pG252D_Module->BandSwitch = BAND_SWITCH_DEFAULT;
	    pG252D_Module->Auxiliary  = AUXILIARY_DEFAULT;                                
        
        if (lg_tdtm_g252d_SetRfFreqHz(pG252D_Module, pG252D_Module->TargetRF )<0) {
            G252D_DBG("[ERROR][FETAL] : Set TDTMG252D RF Frequency fail \n");
            goto error_status_set_g252d_rf; 
        }                  
        
        if (lg_tdtm_g252d_SetBandwidthMode(pG252D_Module, pG252D_Module->Bandwidth )<0){
            G252D_DBG("[ERROR][FETAL] : Set TDTMG252D Bandwidth fail \n");
            goto error_status_set_g252d_bandwidth; 
        }        
    }                        	    	                                      
    return 0;
    
//---    
error_status_set_g252d_bandwidth:
error_status_set_g252d_rf:
    kfree(pG252D_Module);    
    
error_status_allocate_extra_module_memory:
    fe->tuner_priv = NULL;
    return 1;    
}



/***************************************************************************** 
 * Func : lg_tdtm_g252d_uninit 
 *
 * Desc : uninit lg_tdtm_g252d tuner
 *
 * Parm : fe      : handle of dvb_frontend
 *
 * Retn : N/A
 *
 *****************************************************************************/ 
void lg_tdtm_g252d_uninit(struct dvb_frontend* fe)
{     
    G252D_DBG("%s : %d : %s \n",__FILE__,__LINE__, __FUNCTION__);    
    kfree(fe->tuner_priv);
}


/*****************************************************************************
 * Func : lg_tdtm_g252d_set 
 *
 * Desc : set lg_tdtm_g252d tuner
 *
 * Parm : hFE      : handle of dvb_frontend
 *        hFEParam : handle of dvb frontend parameters
 *
 * Retn : 0 for success, others for fail 
 *
 *****************************************************************************/ 
int lg_tdtm_g252d_set(
    void*           hFE,
    void*           hFEParam)
{
    
    struct dvb_frontend* fe = (struct dvb_frontend*) hFE;            
    struct dvb_frontend_parameters* fep = (struct dvb_frontend_parameters*) hFEParam;    
    G252D_MODULE* pG252D_Module = (G252D_MODULE*) fe->tuner_priv;               
    
    G252D_DBG("%s : %d : %s \n",__FILE__,__LINE__, __FUNCTION__);      
    
    /* check data */            
    if (pG252D_Module == NULL){
        G252D_DBG("%s : %d : %s : No G252D Module Exists\n",__FILE__,__LINE__, __FUNCTION__);   
        goto error_status_null_module;
    }
    
    /* set RF */        
    if (lg_tdtm_g252d_SetRfFreqHz(pG252D_Module, fep->frequency)<0){
        G252D_DBG("[ERROR][FETAL] : Set TDTMG252D RF Frequency fail \n");
        goto error_status_set_g252d_rf; 
    }                  
    
    /* set Bandwidth */                
    if (lg_tdtm_g252d_SetBandwidthMode(pG252D_Module, fep->u.ofdm.bandwidth)<0){
        G252D_DBG("[ERROR][FETAL] : Set TDTMG252D Bandwidth fail \n");
        goto error_status_set_g252d_bandwidth; 
    }        
    
    return 0;        
    
//----    
error_status_set_g252d_bandwidth:        
error_status_set_g252d_rf:
error_status_null_module:
    return -1;            
}







/***************************************************************************** 
 * Func : lg_tdtm_g252d_LatchRegBytes 
 *
 * Desc : write tdtm_g252d tuner register
 *        This function will be called by tdtm_g252d api 
 *        to access registers
 * 
 * Parm : TUNER_MODULE: handle of tuner_module 
 *
 * Retn : REG_ACCESS_SUCCESS / REG_ACCESS_ERROR for fail 
 *
 *****************************************************************************/ 
int lg_tdtm_g252d_SetRfFreqHz(
    G252D_MODULE*   pG252D_Module, 
    unsigned long   TargetRfFreqHz
    )
{	
	unsigned long Divider;
	unsigned char Value;

	Divider = (3 * TargetRfFreqHz + 108500000 + 250000) / 500000;
	Divider &= DIVIDER_MASK;
	pG252D_Module->DividerMsb = (unsigned char)((Divider >> 8) & 0xFF);
	pG252D_Module->DividerLsb = (unsigned char)(Divider & 0xFF);


	// Set Control and BandSwitch according to frequency band.
	//
	if(TargetRfFreqHz >= 734000000)
	{
		pG252D_Module->Control = 0xf4;
		Value = 0x04;
	}
	else if(TargetRfFreqHz >= 450000000)
	{
		pG252D_Module->Control = 0xbc;
		Value = 0x04;
	}
	else
	{
		pG252D_Module->Control = 0xb4;
		Value = 0x02;
	}

	pG252D_Module->BandSwitch = Value | (pG252D_Module->BandSwitch & P3_MASK);


	// Latch tuner registers.
	//
	if(lg_tdtm_g252d_LatchRegBytes(pG252D_Module)<0)
		goto error_status_set_register_bytes;


	// Set ActualRfFreqHz with actual RF frequnecy.
	//
	// Note: Origin actual RF frequency equation:
	//       Actual_RF_freq_MHz = (Divider * 1MHz - 217 MHz) / 6
	//
	pG252D_Module->TargetRF = (Divider * 500000 - 108500000) / 3;	

	return 0;

error_status_set_register_bytes:
	return -1;
}






/***************************************************************************** 
 * Func : lg_tdtm_g252d_LatchRegBytes 
 *
 * Desc : write tdtm_g252d tuner register
 *        This function will be called by tdtm_g252d api 
 *        to access registers
 * 
 * Parm : TUNER_MODULE: handle of tuner_module 
 *
 * Retn : 0 / -1 for fail 
 *
 *****************************************************************************/ 
int lg_tdtm_g252d_SetBandwidthMode( 
    G252D_MODULE*       pG252D_Module, 
    int                 BandwidthMode
    )
{	
	unsigned char P3Value;

	// Set and latch P3 according to bandwidth mode.
	//
	switch(BandwidthMode)
	{
		case BANDWIDTH_7_MHZ:	P3Value = 0;	break;
		case BANDWIDTH_8_MHZ:	P3Value = 1;	break;
		default:				P3Value = 0;	break;
	}

	pG252D_Module->BandSwitch = (pG252D_Module->BandSwitch & ~P3_MASK) | (P3Value << P3_SHIFT);

	if(lg_tdtm_g252d_LatchRegBytes(pG252D_Module)<0)
		goto error_status_set_register_bytes;

	return 0;
error_status_set_register_bytes:
	return -1;
}






/***************************************************************************** 
 * Func : lg_tdtm_g252d_LatchRegBytes 
 *
 * Desc : write tdtm_g252d tuner register
 *        This function will be called by tdtm_g252d api 
 *        to access registers
 * 
 * Parm : TUNER_MODULE: handle of tuner_module 
 *
 * Retn : 0 : success / others : fail
 *
 *****************************************************************************/ 
int lg_tdtm_g252d_LatchRegBytes(G252D_MODULE* pG252D_Module)
{                           
	unsigned char I2Cdata[4];
    struct i2c_msg msg;	
		
	I2Cdata[0] = pG252D_Module->DividerMsb;
	I2Cdata[1] = pG252D_Module->DividerLsb;
	I2Cdata[2] = CTRL_VALUE_FOR_SETTING_AUX;
	I2Cdata[3] = pG252D_Module->Auxiliary;

    msg.addr   = pG252D_Module->tuner_addr;
    msg.flags  = 0;
    msg.buf    = I2Cdata;
    msg.len    = 4;        
		  
    if (1!= i2c_transfer(pG252D_Module->i2c_adap, &msg, 1)){
        G252D_DBG("%s : %d : %s \n : write register fail\n",__FILE__,__LINE__, __FUNCTION__);                      
        goto erros_status_access_reg_fail;
    }   
    
	// Latch tuner control and band switch bytes.	    
	I2Cdata[0] = pG252D_Module->Control;
	I2Cdata[1] = pG252D_Module->BandSwitch;
    msg.len    = 2;    

    if (1!= i2c_transfer(pG252D_Module->i2c_adap, &msg, 1)){
        G252D_DBG("%s : %d : %s \n : write register fail\n",__FILE__,__LINE__, __FUNCTION__);                      
        goto erros_status_access_reg_fail;
    }    		
    
    return 0;
//---
erros_status_access_reg_fail:    
    return -1;
}



