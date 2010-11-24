#ifndef  __THOMSON_FE664X_H__
#define  __THOMSON_FE664X_H__

typedef struct {
    unsigned char               tuner_addr;
    struct i2c_adapter*         i2c_adap;
}FE664X_MODULE;

///////////////////////////////////////////////////////////////////////////
// THOMSON API for Linux DVB System
///////////////////////////////////////////////////////////////////////////
int thomson_cofdm_fe664x_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep,
    unsigned char                       pllbuf[6]);
    
int thomson_fe664x_eu_pll_set(
    struct dvb_frontend_parameters*     fep, 
    unsigned char                       pllbuf[6]);
    
int thomson_cofdm_fe664x_init(
    struct dvb_frontend*    fe,
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap);
        
void thomson_cofdm_fe664x_uninit(struct dvb_frontend* fe);

#endif //__THOMSON_FE664X_H__
