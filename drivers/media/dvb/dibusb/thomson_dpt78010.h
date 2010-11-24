#ifndef  __THOMSON_DPT78010_H__
#define  __THOMSON_DPT78010_H__

typedef struct {
    unsigned char               tuner_addr;
    unsigned char               ext_tuner_addr;
    struct i2c_adapter*         i2c_adap;
    unsigned long               dfreq_if;
    unsigned long               dfreq_step_size;
}DPT78010_MODULE;

///////////////////////////////////////////////////////////////////////////
// THOMSON API for Linux DVB System
///////////////////////////////////////////////////////////////////////////
   
int thomson_cofdm_DPT78010_init(
    struct dvb_frontend*    fe,
    unsigned char           tuner_addr,     
    unsigned char           ext_tuner_addr,     
    struct i2c_adapter*     i2c_adap
    );
        
void thomson_cofdm_DPT78010_uninit(struct dvb_frontend* fe);

int thomson_cofdm_DPT78010_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep
    );    
    
#endif //__THOMSON_DPT78010_H__
