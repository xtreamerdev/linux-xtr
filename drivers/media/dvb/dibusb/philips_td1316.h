#ifndef  __PHILIPS_TD1316_H__
#define  __PHILIPS_TD1316_H__

///////////////////////////////////////////////////////////////////////////
// THOMSON API for Linux DVB System
///////////////////////////////////////////////////////////////////////////
int philips_cofdm_td1316_set(
    struct dvb_frontend*                fe,
    struct dvb_frontend_parameters*     fep);
    
int philips_cofdm_td1316_init(
    struct dvb_frontend*    fe,
    unsigned char           tuner_addr, 
    struct i2c_adapter*     i2c_adap);
        
void philips_cofdm_td1316_uninit(struct dvb_frontend* fe);

#endif //__PHILIPS_TD1316_H__
