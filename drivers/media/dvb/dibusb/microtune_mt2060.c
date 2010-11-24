/* DTV tuner - microtune mt2060 
 *
 * This file is a link layer between dibusb and mt2060 api 
 * 
 *      [      DIBUSB       ]   application layer
 *                |
 *      [ microtune_mt2060.c]   link layer
 *                |   
 *      [     MT2060 API    ]   mt2060 api
 *
 */


#include <linux/slab.h>
#include "dvb_frontend.h"
#include "mt2060_api/mt_userdef.h"
#include "mt2060_api/mt2060.h"
#include "microtune_mt2060.h"




/***************************************************************************** 
 * Func : microtune_cofdm_mt2060_init 
 *
 * Desc : init mt2060 tuner
 *
 * Parm : hFE      : handle of dvb_frontend
 *        pllbuf   : not used  
 *
 * Retn : 0 for success, others for fail 
 *
 *****************************************************************************/ 
int microtune_cofdm_mt2060_init(
    void*                   hFE, 
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap
    )
{              
    struct dvb_frontend* fe     = (struct dvb_frontend*) hFE;      
    unsigned long   MTStatus    =  MT_OK;     
    MT2060_MODULE*  pMT2060_Module = NULL;   

    if (fe->tuner_priv == NULL)
    {
        printk("microtune_cofdm_mt2060_init....\n");                
        
        pMT2060_Module = kmalloc(sizeof(MT2060_MODULE), GFP_KERNEL);
        if (pMT2060_Module == NULL){
            printk("[ERROR][FETAL] : allocate mt2060 module memory fail\n");
            goto error_status_allocate_extra_module_memory;
        }
        
        fe->tuner_priv = (void*) pMT2060_Module; 
        
        pMT2060_Module->hMT2060         = NULL;
        pMT2060_Module->f_in            = 545000000;          // Default RF Freq
        pMT2060_Module->f_IF1           = 1220000000;         // 1st IF SAW : 1220 MHz
        pMT2060_Module->f_out           = 36125000;           // Ouput IF Frequency : 36.125 MHz
        pMT2060_Module->f_IFBW          = 6000000 + 750000;   // Bandwidth : 6MHz + 750KHz Guard Band        
        pMT2060_Module->VGA_Gain        = MT2060_VGAG_5DB;    
        pMT2060_Module->tuner_addr      = tuner_addr;
        pMT2060_Module->i2c_adap        = i2c_adap;
            	    	
        if ((MTStatus=MT2060_Open(tuner_addr,&(pMT2060_Module->hMT2060), fe))!=MT_OK){
            printk("[ERROR][FETAL] : construct mt2060 fail (%d)\n",(int)MTStatus);
            goto error_status_construct_mt2060;                    
        }         
                
        //perform tuner driver overrides here
    	MTStatus |= MT2060_SetParam(pMT2060_Module->hMT2060, MT2060_IF1_CENTER, pMT2060_Module->f_IF1);
    	if(!MT_NO_ERROR(MTStatus))
    	{
    		printk("[ERROR][FETAL]Initial:: SetParam Error!! \n");
    		goto error_status_set_mt2060_if1;       		
    	}
    	
    	MTStatus |= MT2060_SetGainRange(pMT2060_Module->hMT2060, pMT2060_Module->VGA_Gain);
    	if(!MT_NO_ERROR(MTStatus))
    	{
    		printk("[ERROR][FETAL]Initial::SetGainRange Fail!! \n");
    		goto error_status_set_mt2060_vga;   
    	}      	          
    	
    	MTStatus |= MT2060_ChangeFreq(
    	    pMT2060_Module->hMT2060,
		    pMT2060_Module->f_in,     /* RF input center frequency   */
		    pMT2060_Module->f_IF1,    /* desired 1st IF frequency    */
		    pMT2060_Module->f_out,    /* IF output center frequency  */
		    pMT2060_Module->f_IFBW);  /* IF output bandwidth + guard */        
		    
    	if(!MT_NO_ERROR(MTStatus))
    	{
    		printk("[ERROR][FETAL]Initial::MT2060_ChangeFreq Fail!! \n");
    		goto error_status_set_mt2060_frequency;   
    	}      	     	        		    
    }                        	    	                                      
    return 0;
    
//---    
error_status_set_mt2060_frequency:
error_status_set_mt2060_vga:
error_status_set_mt2060_if1:
    MT2060_Close(pMT2060_Module->hMT2060);
error_status_construct_mt2060:
    kfree(pMT2060_Module);    
error_status_allocate_extra_module_memory:
    return 1;    
}



/***************************************************************************** 
 * Func : microtune_cofdm_mt2060_uninit 
 *
 * Desc : uninit mt2060 tuner
 *
 * Parm : fe      : handle of dvb_frontend
 *
 * Retn : 0 for success, others for fail 
 *
 *****************************************************************************/ 
void microtune_cofdm_mt2060_uninit(void* hFE)
{
    struct dvb_frontend* fe = (struct dvb_frontend*) hFE;     
    MT2060_MODULE*  pMT2060_Module = (MT2060_MODULE*) fe->tuner_priv;
    
    printk("microtune_cofdm_mt2060_uninit....\n");      
    MT2060_Close(pMT2060_Module->hMT2060);
    kfree(fe->tuner_priv);
}


/*****************************************************************************
 * Func : microtune_cofdm_mt2060_set 
 *
 * Desc : set mt2060 tuner
 *
 * Parm : hFE      : handle of dvb_frontend
 *        hFEParam : handle of dvb frontend parameters
 *        pllbuf   : not used 
 *
 * Retn : 0 for success, others for fail 
 *
 *****************************************************************************/ 
int microtune_cofdm_mt2060_set(
    void*           hFE,
    void*           hFEParam,
    unsigned char   pllbuf[4])
{
    struct dvb_frontend* fe = (struct dvb_frontend*) hFE;            
    struct dvb_frontend_parameters* fep = (struct dvb_frontend_parameters*) hFEParam;
    unsigned long MTStatus = MT_OK;    
    MT2060_MODULE*  pMT2060_Module = (MT2060_MODULE*) fe->tuner_priv;   
    
    if (pMT2060_Module == NULL){
        printk("[ERROR][FETAL] microtune_cofdm_mt2060_set:: NULL Module!!\n");
        goto error_status_null_module;
    }
    
    printk("microtune_cofdm_mt2060_set....\n");
    
    MTStatus |= MT2060_ChangeFreq(
            pMT2060_Module->hMT2060,
		    fep->frequency,             /* RF input center frequency   */
		    pMT2060_Module->f_IF1,      /* desired 1st IF frequency    */
		    pMT2060_Module->f_out,      /* IF output center frequency  */
		    pMT2060_Module->f_IFBW);    /* IF output bandwidth + guard */
    
	if(MT_NO_ERROR(MTStatus)){
	    pMT2060_Module->f_in = fep->frequency;              
        return 0; 
    }        
    else        
        return 1;        
        
error_status_null_module:
    return 1;        
}


/***************************************************************************** 
 * Func : mt2060_write_reg 
 *
 * Desc : write mt2060 tuner register
 *        This function will be called by mt2060 api 
 *        to access registers
 * 
 * Parm : hUserData: handle of dvb_frontend
 *        addr      : address of mt2060 tuner
 *        subAddress: target register adress
 *        Data      : data to be written
 *
 * Retn : 0 for success, others for fail 
 *
 *****************************************************************************/ 
int mt2060_write_reg(
        void*           hUserData, 
        unsigned char   addr, 
        unsigned char   subAddress, 
        unsigned char   Data)
{           
    
    struct dvb_frontend* fe = (struct dvb_frontend*) hUserData;    
    MT2060_MODULE*       pMT2060_Module = (MT2060_MODULE*) fe->tuner_priv;   
    unsigned char        buff[2] = {subAddress,Data};        
    struct i2c_msg       msg;
    
        
    msg.addr  = addr;       // I2C Address                
    msg.flags = 0;          // Write Command
    msg.buf   = buff;
    msg.len   = 2;                    
                                                    
    if (1!= i2c_transfer(pMT2060_Module->i2c_adap, &msg, 1)){
        printk("%s write to reg 0x%x failed\n", __FUNCTION__, buff[0]);
        return MT_COMM_ERR;
    }    
    return MT_OK;
}



/***************************************************************************** 
 * Func : mt2060_read_reg 
 *
 * Desc : read mt2060 tuner register
 *        This function will be called by mt2060 api 
 *        to access registers
 * 
 * Parm : hUserData : handle of dvb_frontend
 *        addr      : address of mt2060 tuner
 *        subAddress: target register adress
 *        pData     : address of buffer 
 *
 * Retn : 0 for success, others for fail 
 *
 *****************************************************************************/
int mt2060_read_reg(
        void*          hUserData, 
        unsigned char  addr, 
        unsigned char  subAddress, 
        unsigned char* pData)
{     

    struct dvb_frontend* fe  = (struct dvb_frontend*) hUserData;   
    MT2060_MODULE*       pMT2060_Module = (MT2060_MODULE*) fe->tuner_priv;      
    struct i2c_msg       msg[2];
    unsigned char        buff[2]={subAddress,0};

    msg[0].addr  = addr;    // I2C Address                
    msg[0].flags = 0;       // WRITE
    msg[0].buf   = &buff[0];
    msg[0].len   = 1;
    
    msg[1].addr  = addr;    // I2C Address                
    msg[1].flags = I2C_M_RD;// READ
    msg[1].buf   = &buff[1];
    msg[1].len   = 1;
  
    if (2!= i2c_transfer(pMT2060_Module->i2c_adap, &msg[0], 2)){
        printk("%s: readreg error reg=0x%x\n", __FUNCTION__, subAddress);
        return MT_COMM_ERR;
    }
    
    *pData = buff[1];
    return MT_OK;
}
