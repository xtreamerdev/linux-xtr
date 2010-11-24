#include <linux/input.h>
#include <linux/config.h>
#include "dvb_frontend.h"
#include "thomson_dpt78010.h"

#define DIGITAL_FREQ_IF     44000000
#define TUNER_STEP_SIZE		62500


/*=======================================================
 * Func : thomson_cofdm_dpt78010_init 
 *
 * Desc : init DDT7680 Module
 *
 * Parm : fe : handle of DVB Frontend
 *        tuner_addr : tuner i2c address
 *        i2c_adap   : i2c adapter
 *
 * Retn : 0 success / otehrs fail
 *=======================================================*/
int thomson_cofdm_dpt78010_init(
    struct dvb_frontend*    fe,
    unsigned char           tuner_addr, 
    unsigned char           ext_tuner_addr, 
    struct i2c_adapter*     i2c_adap
    )
{
    DPT78010_MODULE* hDPT78010;
    hDPT78010 = (DPT78010_MODULE*) kmalloc(sizeof(DPT78010_MODULE),GFP_KERNEL);
    
    printk("%s\n", __FUNCTION__);          
    
    if (hDPT78010 != NULL)
    {      
        hDPT78010->tuner_addr = tuner_addr;        
        hDPT78010->ext_tuner_addr = ext_tuner_addr;  
        hDPT78010->i2c_adap = i2c_adap;
        hDPT78010->dfreq_if = DIGITAL_FREQ_IF;
        hDPT78010->dfreq_step_size = TUNER_STEP_SIZE;
        fe->tuner_priv = hDPT78010;
        return 0;
    }
    else{
        printk("Construct Tuner dpt78010 Failed\n");            
        return 1;    
    }
}



/*=======================================================
 * Func : thomson_cofdm_dpt78010_uninit 
 *
 * Desc : uninit DDT7680 Module
 *
 * Parm : fe : handle of DVB Frontend 
 *         
 * Retn : N/A
 *=======================================================*/
void thomson_cofdm_dpt78010_uninit(struct dvb_frontend* fe)
{
    printk("%s\n", __FUNCTION__);       
    kfree(fe->tuner_priv);    
}





/*=======================================================
 * Func : thomson_cofdm_dpt78010_set 
 *
 * Desc : write DDT7680 Module
 *
 * Parm : fe : handle of DVB Frontend
 *        fep : tuner i2c address
 *         
 * Retn : N/A
 *=======================================================*/
int thomson_cofdm_dpt78010_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep
    )
{

    DPT78010_MODULE*  hDPT78010  = (DPT78010_MODULE*) fe->tuner_priv;
    struct i2c_msg msg;
    unsigned long freq = fep->frequency;
    u16 tunerfreq = 0;
    u8 data[6];    

    printk("%s\n", __FUNCTION__);

    //Chuck add another check to avoid unepexted error on 20060214
    if ( freq < 50000000 || freq > 869000000){
        printk("frequency is out of range!\n");
        return 1;
    }

	freq   += hDPT78010->dfreq_if;
    tunerfreq = (freq/hDPT78010->dfreq_step_size);

    data[0] = (tunerfreq >> 8) & 0x7F;
    data[1] = (tunerfreq & 0xFF);
    data[2] = 0x85;		//step size 62.5Khz
    data[3] = 0x30;	    //power on mode, power down mode is 0x0
    
    if ((freq >= 99000000) && (freq <= 144000000))
		data[3] |= 0x08;
    else if ((freq > 144000000) && (freq <= 168000000))
	{
		data[2] |= 0x20;
		data[3] |= 0x08;
	}
    else if ((freq > 168000000) && (freq <= 186000000))
	{
		data[2] |= 0x40;
		data[3] |= 0x08;
	}
    else if ((freq > 186000000) && (freq <= 196000000))
	{
		data[2] |= 0x60;
		data[3] |= 0x08;
	}
    else if ((freq > 196000000) && (freq <= 318000000))
		data[3] |= 0x4c;
    else if ((freq > 318000000) && (freq <= 388000000))
	{
		data[2] |= 0x20;
		data[3] |= 0x4c;
	}
    else if ((freq > 388000000) && (freq <= 444000000))
	{
		data[2] |= 0x40;
		data[3] |= 0x4c;
	}
    else if ((freq > 444000000) && (freq <= 472000000))
	{
		data[2] |= 0x60;
		data[3] |= 0x4c;
	}
    else if ((freq > 472000000) && (freq <= 512000000))
		data[3] |= 0x84;
    else if ((freq > 512000000) && (freq <= 746000000))
	{
		data[2] |= 0x20;
		data[3] |= 0x84;
	}
    else if ((freq > 746000000) && (freq <= 848000000))
	{
		data[2] |= 0x40;
		data[3] |= 0x84;
	}
    else if ((freq > 848000000) && (freq <= 904000000))
	{
		data[2] |= 0x60;
		data[3] |= 0x84;
	}
    else
    {
        printk("frequency is out of range!\n");
        data[2] |= 0x60;
		data[3] |= 0x84;
        return 1;
    }        

	data[4] = 0x52;		//digital mode, 116 dBuV take over point, high current
	data[5] = 0x90; 	//internal AGC
    printk("Tuner Bytes: %02X %02X %02X %02X %02X %02X %02X\n",
							hDPT78010->tuner_addr<<1,data[0],data[1],data[2], data[3], data[4], data[5]);
       
    msg.addr  = hDPT78010->tuner_addr;
    msg.flags = 0;
    msg.buf   = data;
    msg.len   = 6;
    if (1!= i2c_transfer(hDPT78010->i2c_adap, &msg, 1))
        printk("%s write to reg failed-\n", __FUNCTION__);    

    msleep(100);        
    
    data[4] = 0x72;		//digital mode, 116 dBuV take over point, low current            
    if (1!= i2c_transfer(hDPT78010->i2c_adap, &msg, 1))
        printk("%s write to reg failed--\n", __FUNCTION__);    
    
#if 1    
    printk("%s : Analog Gain Down\n",__FUNCTION__);    
    msg.addr  = 0x42;
	data[0] = 0x00;
	data[1] = 0x34;
	data[2] = 0xB0;
	data[3] = 0x24;
    msg.len = 4;	
	
    if (1!= i2c_transfer(hDPT78010->i2c_adap, &msg, 1))
        printk("%s write to reg failed---\n", __FUNCTION__);    	
#endif            
    return 0;
}