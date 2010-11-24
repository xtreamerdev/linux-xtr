#include <linux/input.h>
#include <linux/config.h>
#include "dvb_frontend.h"
#include "thomson_ddt7680x.h"

#define DIGITAL_FREQ_IF     44000000
#define TUNER_STEP_SIZE		200000


/*=======================================================
 * Func : thomson_cofdm_ddt7680x_init 
 *
 * Desc : init DDT7680 Module
 *
 * Parm : fe : handle of DVB Frontend
 *        tuner_addr : tuner i2c address
 *        i2c_adap   : i2c adapter
 *
 * Retn : 0 success / otehrs fail
 *=======================================================*/
int thomson_cofdm_ddt7680x_init(
    struct dvb_frontend*    fe,
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap
    )
{
    DDT7680X_MODULE* hDDT7680X;
    hDDT7680X = (DDT7680X_MODULE*) kmalloc(sizeof(DDT7680X_MODULE),GFP_KERNEL);
    
    printk("%s\n", __FUNCTION__);          
    
    if (hDDT7680X != NULL)
    {      
        hDDT7680X->tuner_addr = tuner_addr;
        hDDT7680X->i2c_adap = i2c_adap;
        hDDT7680X->dfreq_if = DIGITAL_FREQ_IF;
        hDDT7680X->dfreq_step_size = TUNER_STEP_SIZE;
        fe->tuner_priv = hDDT7680X;
        return 0;
    }
    else{
        printk("Construct Tuner ddt7680x Failed\n");            
        return 1;    
    }
}



/*=======================================================
 * Func : thomson_cofdm_ddt7680x_uninit 
 *
 * Desc : uninit DDT7680 Module
 *
 * Parm : fe : handle of DVB Frontend 
 *         
 * Retn : N/A
 *=======================================================*/
void thomson_cofdm_ddt7680x_uninit(struct dvb_frontend* fe)
{
    printk("%s\n", __FUNCTION__);       
    kfree(fe->tuner_priv);    
}





/*=======================================================
 * Func : thomson_cofdm_ddt7680x_set 
 *
 * Desc : write DDT7680 Module
 *
 * Parm : fe : handle of DVB Frontend
 *        fep : tuner i2c address
 *         
 * Retn : N/A
 *=======================================================*/
int thomson_cofdm_ddt7680x_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep
    )
{

    DDT7680X_MODULE*  hDDT7680X  = (DDT7680X_MODULE*) fe->tuner_priv;
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

	freq   += hDDT7680X->dfreq_if;
    tunerfreq = (freq/hDDT7680X->dfreq_step_size);

    data[0] = (tunerfreq >> 8) & 0x7F;
    data[1] = (tunerfreq & 0xFF);
    data[2] = 0xab;		//step size 200Khz, current 330 uA
    data[3] = ((freq >= 101000000) && (freq <= 191000000)) ? 0x39 :
              ((freq >= 197000000) && (freq <= 461000000)) ? 0x7c :
              ((freq >= 467000000) && (freq <= 905000000)) ? 0xb5 : 0xb5;    

	data[4] = 0x10;		//IAGC = 10uA, normal operation
	data[5] = 0x90; 	//AGC enabled, AFC voltage
    printk("Tuner Bytes: %02X %02X %02X %02X %02X %02X %02X\n",
							hDDT7680X->tuner_addr<<1,data[0],data[1],data[2], data[3], data[4], data[5]);
       
    msg.addr  = hDDT7680X->tuner_addr;
    msg.flags = 0;
    msg.buf   = data;
    msg.len   = 6;
    if (1!= i2c_transfer(hDDT7680X->i2c_adap, &msg, 1))
        printk("%s write to reg failed\n", __FUNCTION__);    

    return 0;
}
