#ifndef  __THOMSON_DDT7680X_H__
#define  __THOMSON_DDT7680X_H__

typedef struct {
    unsigned char               tuner_addr;
    struct i2c_adapter*         i2c_adap;
    unsigned long               dfreq_if;
    unsigned long               dfreq_step_size;
}DDT7680X_MODULE;

///////////////////////////////////////////////////////////////////////////
// THOMSON API for Linux DVB System
///////////////////////////////////////////////////////////////////////////
   
int thomson_cofdm_ddt7680x_init(
    struct dvb_frontend*    fe,
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap
    );
        
void thomson_cofdm_ddt7680x_uninit(struct dvb_frontend* fe);

int thomson_cofdm_ddt7680x_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep
    );    
    
#endif //__THOMSON_DDT7680X_H__
