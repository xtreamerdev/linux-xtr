/*
 *  Driver for Realtek DVB-T rtl2820 demodulator
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <asm/pgtable.h>
#include "dvb_frontend.h"
#include "rtd2820.h"
#include "rtd2820_api/demod_rtd2820.h"

unsigned long gIfFreq = 6000000;  //unit: MHz * 1000
static int STATUS_TIMEOUT_ = 10;
u8 SpecInvOn=1; //spectrum inverion option

//-------- MACROS ---------
int debug;

#define dprintk(args...) \
    do { \
        if (debug) printk(KERN_DEBUG "rtl2820: " args); \
    } while (0)

#define MIN(a, b) ((a)< (b)? (a): (b))

//-------- Dvb Frontend Functions
int  rtd2820_init(struct dvb_frontend* fe);
int  rtd2820_sleep(struct dvb_frontend* fe);
void rtd2820_release(struct dvb_frontend* fe);    
int  rtd2820_set_parameters(struct dvb_frontend* fe, struct dvb_frontend_parameters* param);
int  rtd2820_get_parameters(struct dvb_frontend* fe, struct dvb_frontend_parameters* param);
int  rtd2820_get_tune_settings(struct dvb_frontend* fe, struct dvb_frontend_tune_settings* fe_tune_settings);
int  rtd2820_read_status(struct dvb_frontend* fe, fe_status_t* status);
int  rtd2820_read_ber(struct dvb_frontend* fe, u32* ber);
int  rtd2820_read_signal_strength(struct dvb_frontend* fe, u16* strength);
int  rtd2820_read_snr(struct dvb_frontend* fe, u16* snr);
int  rtd2820_read_ucblocks(struct dvb_frontend* fe, u32* ucblocks);
int  rtd2820_user_cmd(struct dvb_frontend* fe, int cmd, int* p_arg_in, int n_arg_in, int* p_arg_out, int n_arg_out);
void rtd2820_channel_scan_mode_en(struct dvb_frontend* fe, unsigned char on_off);

//-------- User Command Functions 
int  rtd2820_ts_set_status(struct dvb_frontend* fe, int on_off);
int  rtd2820_pid_filter(struct dvb_frontend* fe, int* p_arg_in, int n_arg_in);
int  rtd2820_copy_from_usb(struct dvb_frontend* fe, void** pp_new_urb_buffer, u8* p_buffer, int len_buffer);

//-------- Internal Used Functions  ---------// 
#ifdef RTD2820_DEBUG
    int  rtd2820_get_writeable_size(struct rtd2830_ts_buffer_s* p_ts_ring_info);
    int  rtd2820_get_writeable_size_single(struct rtd2830_ts_buffer_s* p_ts_ring_info, int ring_size, int index_read);
#endif

#ifndef USE_USER_MEMORY
    int  rtd2820_mmap(struct dvb_frontend* fe, struct file* file, struct vm_area_struct* vma);
    int  rtd2820_get_size(struct dvb_frontend* fe);
    void rtd2820_mmap_reserved(u8* p_buffer, int len_buffer);
    void rtd2820_mmap_unreserved(u8* p_buffer, int len_buffer);
#else
    int  rtd2820_pre_setup(struct dvb_frontend* fe, unsigned char* p_phys, unsigned char* p_user_cached, int size);
    void rtd2820_post_setup(struct dvb_frontend* fe);        
#endif


//-------- Internal Used Arguments  ---------//        
struct dvb_frontend_ops rtd2820_ops = 
{
    .info = {
        .name                = "Nextwave rtd2820 VSB/QAM frontend",
        .type                = FE_ATSC,
        .frequency_min       = 54000000,
        .frequency_max       = 860000000,
        .frequency_stepsize  = 166666,
        .frequency_tolerance = 0,
        .caps                = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 |
                               FE_CAN_FEC_3_4 | FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 |
                               FE_CAN_FEC_AUTO |
                               FE_CAN_8VSB | FE_CAN_QAM_64 | FE_CAN_QAM_256                
    },
     
    .init =                    rtd2820_init,
    .release =                 rtd2820_release,

    .sleep =                   rtd2820_sleep,

    .set_frontend =            rtd2820_set_parameters,
    .get_frontend =            rtd2820_get_parameters,
    .get_tune_settings =       rtd2820_get_tune_settings,

    .read_status =             rtd2820_read_status,
    .read_ber =                rtd2820_read_ber,
    .read_signal_strength =    rtd2820_read_signal_strength,
    .read_snr =                rtd2820_read_snr,
    .read_ucblocks =           rtd2820_read_ucblocks,
    .user_cmd=                 rtd2820_user_cmd,
};



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

int rtd2820_write_bits(
    rtd2820_handle_t*           hRTD2820,
    int                         page,
    int                         offset,
    int                         bit_low,
    int                         bit_high,
    int                         val
    )
{
    rtd2820_reg_t   reg = {page,offset,bit_low,bit_high}; 
    return hRTD2820->write_register(hRTD2820,reg, val);
}

int rtd2820_read_bits(
    rtd2820_handle_t*           hRTD2820,
    int                         page,
    int                         offset,
    int                         bit_low,
    int                         bit_high,
    int*                        p_val
    )
{
    rtd2820_reg_t reg = {page,offset,bit_low,bit_high}; 
    return hRTD2820->read_register(hRTD2820,reg,p_val);
}

/*=======================================================
 * Func : rtd2820_attach 
 *
 * Desc : construct a dvb_frontend object for rtd2820
 *
 * Parm : config : handle of rtd2820_config
 *        i2c : i2c repeater
 *
 * Retn : handle of dvb_frontend
 *=======================================================*/
struct dvb_frontend* rtd2820_attach(
        const struct rtd2820_config*        config,
        struct i2c_adapter*                 i2c
        )
{
    struct rtd2820_state*   state    = NULL;        
    rtd2820_handle_t*       hRTD2820 = NULL;      
    
    
#ifndef USE_USER_MEMORY
    struct rtd2820_ts_buffer_s* p_ts_ring_info;
#endif  
      
    //------------------------            
    hRTD2820 = ConstructRTD2820Module(config->demod_address, (void*)i2c);    
    if (hRTD2820 == NULL){
        printk("%s: construct RTD2820 Module fail\n", __FUNCTION__);
        goto error_status_construct_rtd2820_module_fail;
    }    
    
    //------------------------        
    state = kmalloc(sizeof(struct rtd2820_state), GFP_KERNEL);
    if (state == NULL) {
        printk("%s: construct rtd2820 state fail\n", __FUNCTION__);
        goto error_status_construct_rtd2820_state_fail;        
    }
        
    memset(state,0,sizeof(*state));
    
    /* setup the state */   
    state->hRTD2820 = (void*) hRTD2820;       
    /* setup the state */
    state->i2c = i2c;
    memcpy(&state->config,config,sizeof(struct rtd2820_config));
    memcpy(&state->ops, &rtd2820_ops, sizeof(struct dvb_frontend_ops));
    memset(&state->fe_param, 0, sizeof(struct dvb_frontend_parameters));

#ifndef USE_USER_MEMORY

    state->len_ts_mmap_region = RTD2820_TS_BUFFER_SIZE;
    state->p_ts_mmap_region = (u8*)__get_free_pages(GFP_ATOMIC, get_order(state->len_ts_mmap_region));
    
    if (!state->p_ts_mmap_region){
        printk("%s: __get_free_pages fail\n", __FUNCTION__);
        goto error_status_get_free_page_fail;
    }

    rtd2820_mmap_reserved(state->p_ts_mmap_region, state->len_ts_mmap_region);
    
    p_ts_ring_info= RTD2820_GET_RING_INFO(state->p_ts_mmap_region, state->len_ts_mmap_region);
    
    memset(p_ts_ring_info,0,sizeof(*p_ts_ring_info));

    p_ts_ring_info->p_lower      =
    p_ts_ring_info->pp_read[0]   =
    p_ts_ring_info->pp_read[1]   =
    p_ts_ring_info->pp_read[2]   =
    p_ts_ring_info->pp_read[3]   =
    p_ts_ring_info->p_write      = state->p_ts_mmap_region;
    p_ts_ring_info->p_upper      = p_ts_ring_info->p_lower+ RTD2820_TS_STREAM_BUFFER_SIZE;

    p_ts_ring_info->p_b_active[0]=
    p_ts_ring_info->p_b_active[1]=
    p_ts_ring_info->p_b_active[2]=
    p_ts_ring_info->p_b_active[3]= 0;

    state->p_next_urb_buffer= p_ts_ring_info->p_lower + config->urb_num * config->urb_size;

    printk("============== %s: p_urb_alloc =============\n", __FUNCTION__);
    state->config.p_urb_alloc(state->config.dibusb,
                              p_ts_ring_info->p_lower,
                              config->urb_num, 
                              config->urb_size);
    
    printk("%s: (lower, upper, write, read(0-3), ring_size)=\n(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
           __FUNCTION__,
           (int)p_ts_ring_info->p_lower,
           (int)p_ts_ring_info->p_upper,
           (int)p_ts_ring_info->p_write,
           (int)p_ts_ring_info->pp_read[0],
           (int)p_ts_ring_info->pp_read[1],
           (int)p_ts_ring_info->pp_read[2],
           (int)p_ts_ring_info->pp_read[3],
           (int)p_ts_ring_info->p_upper - (int)p_ts_ring_info->p_lower
           );
    state->p_ts_ring_info = p_ts_ring_info; // restore ts_ring_info
           
#endif // USE_USER_MEMORY

    /* create dvb_frontend */
    state->frontend.ops = &state->ops;    
    state->frontend.demodulator_priv = state;
    state->frontend.tuner_priv = NULL;
#ifdef URB_PARK_EN       
    state->p_urb_park = NULL;
    state->len_urb_park = 0;
#endif    
    return &state->frontend;

//---
#ifndef USE_USER_MEMORY
error_status_get_free_page_fail: 
#endif    
    kfree(state);
    
error_status_construct_rtd2820_state_fail:       
    DestructRTD2820Module(hRTD2820);
    
error_status_construct_rtd2820_module_fail:        
    return NULL;
}



/*=======================================================
 * Func : rtd2820_i2c_repeat 
 *
 * Desc : i2c repeat function for rtd2820
 *
 * Parm : fe : handle of dvb_frontend 
 *
 * Retn : N/A
 *=======================================================*/
void rtd2820_i2c_repeat( struct dvb_frontend* fe)
{
    struct rtd2820_state* p_state = fe->demodulator_priv;
    rtd2820_handle_t* hRTD2820 = (rtd2820_handle_t*)p_state->hRTD2820;
    
    if (hRTD2820 != NULL)
	    hRTD2820->forward_i2c(hRTD2820);	
	else
	    printk("%s: no RTD2820 module exists\n",__FUNCTION__);	
}




///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////




/*=======================================================
 * Func : rtd2820_init 
 *
 * Desc : dvb frontend api - Init function 
 *
 * Parm : fe : handle of dvb_frontend
 *
 * Retn : 0 : success, others : fail
 *=======================================================*/ 
int rtd2820_init(struct dvb_frontend* fe)
{     
    struct rtd2820_state* p_state = fe->demodulator_priv;    
    rtd2820_handle_t* hRTD2820 = (rtd2820_handle_t*)p_state->hRTD2820;
    
    if (hRTD2820 == NULL){
        printk("%s: no RTD2820 module exists\n",__FUNCTION__);	
        return -1;
    }
        
    if (hRTD2820->init(hRTD2820)!= RT2820_FUNCTION_OK){
        printk("%s: init fail\n", __FUNCTION__);
        return 1;        
    }             

    if (hRTD2820->set_if(hRTD2820, RT2820_IF_FREQ_6M)!= RT2820_FUNCTION_OK){
        printk("%s: set if \n", __FUNCTION__);
        return 1;        
    }       
    
    if (hRTD2820->set_spec_inv(hRTD2820, SpecInvOn)!= RT2820_FUNCTION_OK){
        printk("%s: set spec inv\n", __FUNCTION__);
        return 1;        
    }           
      
    if (p_state->config.pll_init)
        p_state->config.pll_init(fe, NULL);
    else
        printk("%s: error in NULL p_state->config.pll_init\n", __FUNCTION__);
    
    if (hRTD2820->soft_reset(hRTD2820)!= RT2820_FUNCTION_OK){
        printk("%s: soft reset fail\n", __FUNCTION__);
        return 1;        
    }     
    
    return 0;    
}




/*=======================================================
 * Func : rtd2820_release 
 *
 * Desc : dvb frontend api - release function 
 *
 * Parm : fe : handle of dvb_frontend
 *
 * Retn : N/A
 *=======================================================*/ 
void rtd2820_release(struct dvb_frontend* fe)
{		
    int i;	
    struct rtd2820_state* p_state = fe->demodulator_priv;
            
    p_state ->config.pll_uninit(fe);              
            
#ifndef USE_USER_MEMORY
    if (p_state->p_ts_mmap_region)
    {
        printk("============== %s: p_urb_free =============\n", __FUNCTION__);
        p_state ->config.p_urb_free(p_state ->config.dibusb);
        printk("============== %s: free_pages =============\n", __FUNCTION__);
        rtd2820_mmap_unreserved(p_state ->p_ts_mmap_region, p_state ->len_ts_mmap_region);
        free_pages((long)p_state ->p_ts_mmap_region, get_order(p_state ->len_ts_mmap_region));
    }
#else
    if (p_state->p_ts_ring_info != NULL) {
        printk("============== %s: set ring_info->p_b_active to 0xFF =============\n", __FUNCTION__);
        for (i=0; i<4;i++){
             p_state->p_ts_ring_info->p_b_active[i] = 0xFF;
        }
    }	    
#endif // USE_USER_MEMORY
    DestructRTD2820Module((rtd2820_handle_t*)p_state->hRTD2820);
    kfree(p_state);
}




/*=======================================================
 * Func : rtd2820_sleep 
 *
 * Desc : dvb frontend api - sleep function 
 *
 * Parm : fe : handle of dvb_frontend
 *
 * Retn : 0 success
 *=======================================================*/ 
int rtd2820_sleep(struct dvb_frontend* fe)
{
    printk("%s: Do Nothing\n",__FUNCTION__);
    return 0;
}


/*=======================================================
 * Func : rtd2820_set_parameters 
 *
 * Desc : dvb frontend api - set frontend parameter function 
 *
 * Parm : fe : handle of dvb_frontend
 *        param :  frontend parameter
 *
 * Retn : 0 success
 *=======================================================
 * Flow : 1) reset rtd2820 agc gain
 *        2) set pll
 *        3) software reset rtd2820
 *=======================================================*/ 
int rtd2820_set_parameters(
    struct dvb_frontend*            fe,
    struct dvb_frontend_parameters* param
    )
{            
    struct rtd2820_state*   p_state  = fe->demodulator_priv;        
    rtd2820_handle_t*       hRTD2820 = (rtd2820_handle_t*)p_state->hRTD2820;  	               
    struct dvb_ofdm_parameters*     p_ofdm_param = &param->u.ofdm;    
    
    fe_status_t status;
    
    if (p_state->fe_param.frequency == param->frequency &&
        p_state->fe_param.u.ofdm.bandwidth == p_ofdm_param->bandwidth)
    {
        rtd2820_read_status(fe,&status);
        if ((status & FE_HAS_LOCK)){        // signal lock already
            return 0;
        }        
    }            
    else {

        printk("(%u %u | %u %u)\n",
                p_state->fe_param.frequency, 
                param->frequency,
                p_state->fe_param.u.ofdm.bandwidth,
                p_ofdm_param->bandwidth
            );
    }   
          
    ///////////////////////////////////////////////////////////    
    hRTD2820->agc_reset(hRTD2820);	
	
	if (p_state->config.pll_set)
        p_state->config.pll_set(fe, param, NULL);
    else
        printk("%s: error in NULL p_state->config.pll_set\n", __FUNCTION__);        		
	
    msleep(10);
    
    hRTD2820->set_if(hRTD2820, gIfFreq);
		
    hRTD2820->soft_reset(hRTD2820);             // ATSC demodulator software reset	
       
    hRTD2820->channel_scan(hRTD2820);           // channel scan to lock signal        
    
    hRTD2820->soft_reset(hRTD2820);             // ATSC demodulator software reset	
 
    memcpy(&p_state->fe_param, (unsigned char*)param, sizeof(struct dvb_frontend_parameters));	      
    
    return 0;
}





/*=======================================================
 * Func : rtd2820_get_parameters 
 *
 * Desc : dvb frontend api - get frontend parameter function
 *
 * Parm : fe : handle of dvb_frontend
 *        param :  frontend parameter
 *
 * Retn : 0 success
 *=======================================================*/ 
int rtd2820_get_parameters(
    struct dvb_frontend*            fe,
    struct dvb_frontend_parameters* param
    )
{
    printk("%s: Do Nothing\n",__FUNCTION__);
    return 0;
}




/*=======================================================
 * Func : rtd2820_get_tune_settings 
 *
 * Desc : dvb frontend api - get tune setting function
 *
 * Parm : fe : handle of dvb_frontend
 *        fe_tune_settings :  
 *
 * Retn : 0 success
 *=======================================================*/ 
int rtd2820_get_tune_settings(
    struct dvb_frontend*                fe, 
    struct dvb_frontend_tune_settings*  fe_tune_settings
    )
{
    fe_tune_settings->min_delay_ms = 800;
    fe_tune_settings->step_size = 0;
    fe_tune_settings->max_drift = 0;
    return 0;
}




/*=======================================================
 * Func : rtd2820_read_status 
 *
 * Desc : dvb frontend api - read frontend status function
 *
 * Parm : fe : handle of dvb_frontend
 *        fe_status_t : frontend status
 *
 * Retn : 0 success
 *=======================================================*/ 
int rtd2820_read_status(
    struct dvb_frontend*    fe, 
    fe_status_t*            status
    )
{    
    static char read_counter = 0;
    struct rtd2820_state* p_state= fe->demodulator_priv;        
    rtd2820_handle_t* hRTD2820 = (rtd2820_handle_t*)p_state->hRTD2820;
    unsigned short snr = 0;
    unsigned short ber = 0;
	unsigned char sync = 0;
	int times = 0;		

	hRTD2820->get_lock_status(hRTD2820, &sync);
	if (sync) {
	    *status = (FE_HAS_CARRIER| FE_HAS_VITERBI| FE_HAS_LOCK| FE_HAS_SYNC| FE_HAS_SIGNAL);        
        sync = 32;
    }
    else
    {
    	*status = 0;	
    	while((sync != 2) && (sync != 16) && (sync != 32) && (times++ < STATUS_TIMEOUT_) )
    	{
    		hRTD2820->get_status(hRTD2820, &sync);
    		msleep(1);
    	}
        
        if (sync != 32)         
    	    read_counter=10;        
        else{
    	    if (sync == 32 ||sync == 16)
    		    *status = (FE_HAS_CARRIER| FE_HAS_VITERBI| FE_HAS_LOCK| FE_HAS_SYNC| FE_HAS_SIGNAL);
        }
    }
    
    if (read_counter++>=10)
    {
        hRTD2820->get_snr(hRTD2820,&snr);
        hRTD2820->get_ber(hRTD2820,&ber);        
        printk("%s: status=%d, snr=%d, ber=%d\n",__FUNCTION__,sync,snr,ber);     
        read_counter = 0;           
    }
		    
	return 0;    
}




/*=======================================================
 * Func : rtd2820_read_ber( 
 *
 * Desc : dvb frontend api - read bit error rate function
 *
 * Parm : fe : handle of dvb_frontend
 *        ber : bit error rate
 *
 * Retn : 0 success
 *=======================================================*/
int rtd2820_read_ber(
    struct dvb_frontend*            fe, 
    u32*                            ber
    )
{
    unsigned short ber_rate;
    struct rtd2820_state* p_state= fe->demodulator_priv;        
    rtd2820_handle_t* hRTD2820 = (rtd2820_handle_t*)p_state->hRTD2820;
    
    //printk("%s: Do Nothing\n",__FUNCTION__);
    hRTD2820->get_ber(hRTD2820, &ber_rate);
    *ber = ber_rate;
        
    return 0;
}




/*=======================================================
 * Func : rtd2820_read_signal_strength( 
 *
 * Desc : dvb frontend api - read bit error rate function
 *
 * Parm : fe : handle of dvb_frontend
 *        strength : siganl strength
 *
 * Retn : 0 success
 *=======================================================*/
int rtd2820_read_signal_strength(
    struct dvb_frontend*        fe, 
    u16*                        strength
    )
{
    printk("%s: Do Nothing\n",__FUNCTION__);
    *strength = 0;
    return 0;
}




/*=======================================================
 * Func : rtd2820_read_snr( 
 *
 * Desc : dvb frontend api - read signal noise ratio function
 *
 * Parm : fe : handle of dvb_frontend
 *        snr : siganl noise ratio
 *
 * Retn : 0 success
 *=======================================================*/
int rtd2820_read_snr(
    struct dvb_frontend*        fe, 
    u16*                        snr
    )
{        
    struct rtd2820_state* p_state= fe->demodulator_priv;        
    rtd2820_handle_t* hRTD2820 = (rtd2820_handle_t*)p_state->hRTD2820;
           
    hRTD2820->get_snr(hRTD2820,snr);
    
    return 0;
}




/*=======================================================
 * Func : rtd2820_read_ucblocks( 
 *
 * Desc : dvb frontend api - ucblocks
 *
 * Parm : fe : handle of dvb_frontend
 *        ucblocks : 
 *
 * Retn : 0 success
 *=======================================================*/
int rtd2820_read_ucblocks(
    struct dvb_frontend*        fe, 
    u32*                        ucblocks
    )
{
    printk("%s: Do Nothing\n",__FUNCTION__);
    return 0;
}




/*=======================================================
 * Func : rtd2820_user_cmd( 
 *
 * Desc : dvb frontend api - user command function
 *
 * Parm : fe : handle of dvb_frontend
 *        cmd : command type
 *        p_arg_in : input arguments 
 *        n_arg_in : number of input arguments 
 *        p_arg_out : output arguments 
 *        n_arg_out : number of output arguments  
 *
 * Retn : 0 success
 *=======================================================*/
int rtd2820_user_cmd(
    struct dvb_frontend*        fe,
    int                         cmd,
    int*                        p_arg_in,
    int                         n_arg_in,
    int*                        p_arg_out,
    int                         n_arg_out
    )
{
    struct rtd2820_state* state = fe->demodulator_priv;

    switch (cmd)
    {
            
#ifdef USE_USER_MEMORY
    case RTD2830_CMD_PRE_SETUP:
        rtd2820_pre_setup(fe, (unsigned char*)p_arg_in[0], (unsigned char*)p_arg_in[1], (int)p_arg_in[2]);
        return 0;
        
    case RTD2830_CMD_POST_SETUP:
        rtd2820_post_setup(fe);
        return 0;        
#else
    case RTD2820_MMAP:
        return rtd2820_mmap(fe, (struct file*)p_arg_in[0], (struct vm_area_struct*)p_arg_in[1]);
        
    case RTD2830_CMD_GET_RING_SIZE:
        p_arg_out[0]= rtd2820_get_size(fe);
        return 0;        
#endif

    case RTD2830_CMD_TS_START:
        return rtd2820_ts_set_status(fe, 1);
        
    case RTD2830_CMD_TS_STOP:
        return rtd2820_ts_set_status(fe, 0);
        
    case RTD2830_CMD_TS_IS_START:
        p_arg_out[0] = ((state->flag & RTD2820_STRAMING_ON)) ? 1 : 0;
        return 0;
        
    case RTD2830_CMD_PID_FILTER:
        return rtd2820_pid_filter(fe, p_arg_in, n_arg_in);
        
    case RTD2830_CMD_CHANNEL_SCAN_MODE_EN:      
        rtd2820_channel_scan_mode_en(fe, p_arg_in[0]);        
        return 0;

    case RTD2830_CMD_URB_IN:
        return rtd2820_copy_from_usb(fe, (void**)p_arg_out[0], (u8*)p_arg_in[0], p_arg_in[1]);    
        
    default:
        printk("%s: unknow command\n", __FUNCTION__);
        break;
    }
        
    return 1;
}






//////////////////////////////////////////////////////////
// User Command Functions
//---------------------------------------------------------
// rtd2820_ts_set_status 
// rtd2820_copy_from_usb/ rtd2820_pid_filter/ rtd2820_mmap 
//////////////////////////////////////////////////////////


/*=======================================================
 * Func : rtd2820_ts_set_status 
 *
 * Desc : transport stream control
 *
 * Parm : fe   : handle of dvb frontend
 *        on_off : 0 : stop streaming, 1: start streaming 
 *
 * Retn : N/A
 *=======================================================*/
int rtd2820_ts_set_status(
    struct dvb_frontend*        fe,
    int                         on_off
    )
{
    unsigned long               urb_size;
    unsigned long               urb_num;    
    struct rtd2820_state*       state = fe->demodulator_priv;
    struct rtd2830_ts_buffer_s* p_ts_ring_info = state->p_ts_ring_info;
    
    if ((state->flag & RTD2820_CHANNEL_SCAN_MODE_EN)){
        urb_size = RTD2820_CHANNEL_SCAN_URB_SIZE;    
        urb_num  = RTD2820_CHANNEL_SCAN_URB_NUMBER;    
    }
    else{    
        urb_size = state->config.urb_size;    
        urb_num  = state->config.urb_num;    
    }

    if (( on_off &&  (state->flag & RTD2820_STRAMING_ON)) || 
        (!on_off && !(state->flag & RTD2820_STRAMING_ON))){
        return 0;
    }         
    
    if (0== on_off){        
        
        printk("%s : %d :    Stop Streaming\n", __FILE__,__LINE__);
                               
        state->config.p_dibusb_streaming(state->config.dibusb, 0);           
        
        state->flag &= ~RTD2820_STRAMING_ON;          // reset streaming on flag  
        
        p_ts_ring_info->pp_read[0] = p_ts_ring_info->p_lower;
        p_ts_ring_info->pp_read[1] = p_ts_ring_info->p_lower;
        p_ts_ring_info->pp_read[2] = p_ts_ring_info->p_lower;
        p_ts_ring_info->pp_read[3] = p_ts_ring_info->p_lower;
        p_ts_ring_info->p_write    = p_ts_ring_info->p_lower;
        
        memset(&state->fe_param, 0, sizeof(struct dvb_frontend_parameters));
                    
    }
    else{
    
        state->config.p_urb_reset(state->config.dibusb);
        
        p_ts_ring_info->pp_read[0] = p_ts_ring_info->p_lower;
        p_ts_ring_info->pp_read[1] = p_ts_ring_info->p_lower;
        p_ts_ring_info->pp_read[2] = p_ts_ring_info->p_lower;
        p_ts_ring_info->pp_read[3] = p_ts_ring_info->p_lower;
        p_ts_ring_info->p_write    = p_ts_ring_info->p_lower;
        
        state->p_next_urb_buffer   = state->p_ts_ring_info->p_lower + (urb_size * urb_num); // reset next urb buffer position
        
#ifdef URB_PARK_EN
        if (!(state->flag & RTD2820_CHANNEL_SCAN_MODE_EN) && state->p_urb_park != NULL)
            state->p_next_urb_buffer = state->p_ts_ring_info->p_lower;
#endif        
        
        printk("%s : %d :   ------------------------\n", __FILE__,__LINE__);
        printk("%s : %d :    Start Streaming        \n", __FILE__,__LINE__);
        printk("%s : %d :   ------------------------\n", __FILE__,__LINE__);        
        printk("%s : %d :    urb_size = %d          \n", __FILE__,__LINE__,(int)urb_size);        
        printk("%s : %d :    urb_num = %d           \n", __FILE__,__LINE__,(int)urb_num);        
        printk("%s : %d :    p_next_urb_buffer = %p \n", __FILE__,__LINE__,(unsigned char*)state->p_next_urb_buffer);          
        printk("%s : %d :   ------------------------\n", __FILE__,__LINE__);
                
        state->flag |= RTD2820_STRAMING_ON;
        state->config.p_dibusb_streaming(state->config.dibusb, 1);  // start streaming                        
    }
    
    return 0;
}




/*=======================================================
 * Func : rtd2820_copy_from_usb 
 *
 * Desc : this function will be called as urb complete
 *
 * Parm : fe   : handle of dvb frontend
 *        pp_new_urb_buffer : next buffer address of new urb
 *        p_buffer : urb data address
 *        len_buffer : length of buffer
 *
 * Retn : N/A
 *=======================================================*/
int rtd2820_copy_from_usb(
    struct dvb_frontend*        fe,
    void**                      pp_new_urb_buffer,
    u8*                         p_buffer,
    int                         len_buffer)
{
    struct rtd2820_state*       state = fe->demodulator_priv;
    struct rtd2830_ts_buffer_s* p_ts_ring_info = state->p_ts_ring_info;
    unsigned long               urb_size;
#ifdef RTD2820_DEBUG
    int                         writeable_size;    
#endif            
        
    urb_size = ((state->flag & RTD2820_CHANNEL_SCAN_MODE_EN)) ? RTD2820_CHANNEL_SCAN_URB_SIZE 
                                                              : state->config.urb_size;     
    
    //printk("----------------URB %d Complete---------------\n",i);
    
    if(!(state->flag & RTD2820_STRAMING_ON))
    {                        
        printk("%s: %d: skip\n", __FUNCTION__, __LINE__);
        return 0;
    }
    
#ifdef URB_PARK_EN    
                                              
    if (!(state->flag & RTD2820_CHANNEL_SCAN_MODE_EN) && state->p_urb_park!=NULL)
    {        
        if ((p_buffer < state->p_ts_ring_info->p_lower) || 
            (p_buffer > state->p_ts_ring_info->p_upper))
        {
            *pp_new_urb_buffer = state->p_next_urb_buffer;     // direct to the new buffer       
            state->p_next_urb_buffer += urb_size;            
            return 0;            
        }
    }
    
#endif

#ifdef RTD2820_DEBUG
  
    if ((int)p_buffer!= (int)*pp_new_urb_buffer)
        printk("00000000000000000000000000000000000000\n");

    if ((urb_size!= len_buffer) || (p_buffer!= p_ts_ring_info->p_write)){
        printk("11111111111111111111111111111111111111 0x%08x 0x%08x %d (%d)\n", 
                (int)p_ts_ring_info->p_write, 
                (int)p_buffer, 
                (int)len_buffer,
                (int)urb_size);
    }

    /*                
    // some checking for illegal/exception handle
    if(!(state->flag & RTD2820_STRAMING_ON)){
        printk("22222222222222222222222222222222222222\n");
    }
    */

    /*
    if ((p_ts_ring_info->p_write== p_ts_ring_info->p_lower) && (0x47!= *p_buffer)){
        printk("33333333333333333333333333333333333333\n");
    }
    */

    if (urb_size > (writeable_size= rtd2820_get_writeable_size(p_ts_ring_info)))
    {                        
        // D7 of active byte was used to notify buffer full status to AP       
        // user must clear it by it's self   
        int i;  
        for (i=0; i<4;i++){
            if (p_ts_ring_info->p_b_active[i])
                p_ts_ring_info->p_b_active[i] |= 0x80;                    
        }            
                
        printk("44444444444444444444444444444444444444 %d [%02x %02x %02x %02x]\n", 
                writeable_size,
                p_ts_ring_info->p_b_active[0],
                p_ts_ring_info->p_b_active[1],
                p_ts_ring_info->p_b_active[2],
                p_ts_ring_info->p_b_active[3]);
            
        printk("write, read[0..3]= (0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
                (int)p_ts_ring_info->p_write,
                (int)p_ts_ring_info->pp_read[0],
                (int)p_ts_ring_info->pp_read[1],
                (int)p_ts_ring_info->pp_read[2],
                (int)p_ts_ring_info->pp_read[3]);              
    }

#endif

    // just a workaround in case of urb returning not in the order of submission
    if (p_buffer> p_ts_ring_info->p_write)
    {
        while (p_buffer> p_ts_ring_info->p_write)
        {
            *((unsigned int*)p_ts_ring_info->p_write)= 0xffffffff;
            p_ts_ring_info->p_write += urb_size;
        }
    }
    else if (p_buffer< p_ts_ring_info->p_write)
    {
        while (p_ts_ring_info->p_upper > p_ts_ring_info->p_write)
        {
            *((unsigned int*)p_ts_ring_info->p_write)= 0xffffffff;
            p_ts_ring_info->p_write += urb_size;
        }
        
        p_ts_ring_info->p_write= p_ts_ring_info->p_lower;
        
        while (p_buffer> p_ts_ring_info->p_write)
        {
            *((unsigned int*)p_ts_ring_info->p_write)= 0xffffffff;
            p_ts_ring_info->p_write += urb_size;
        }
    }

    p_ts_ring_info->p_write += urb_size;

#ifdef RTD2820_DEBUG
    if (p_ts_ring_info->p_write> p_ts_ring_info->p_upper){
        printk("55555555555555555555555555555555555555\n");
    }
#endif

    // warp arround
    if (p_ts_ring_info->p_write== p_ts_ring_info->p_upper)
        p_ts_ring_info->p_write = p_ts_ring_info->p_lower;

    // send next URB buffer position
    *pp_new_urb_buffer= state->p_next_urb_buffer;
    
    // compute next urb buffer    
    state->p_next_urb_buffer += urb_size;

#ifdef RTD2820_DEBUG
    if (state->p_next_urb_buffer > p_ts_ring_info->p_upper){
        printk("66666666666666666666666666666666666666\n");
    }
#endif

    if (state->p_next_urb_buffer == p_ts_ring_info->p_upper){
        state->p_next_urb_buffer =  p_ts_ring_info->p_lower;
    }

    return 0;
}




/*=======================================================
 * Func : rtd2820_pid_filter 
 *
 * Desc : set pid filter
 *
 * Parm : fe   : handle of dvb frontend
 *        p_arg_in : PID Value
 *        n_arg_in : number of PIDs
 *
 * Retn : N/A
 *=======================================================*/
int rtd2820_pid_filter(
    struct dvb_frontend*        fe,
    int*                        p_arg_in,
    int                         n_arg_in
    )
{
    struct rtd2820_state*       p_state= fe->demodulator_priv;
    u8                          buf[64]; 

    struct i2c_msg msg = {
        .addr = 0xff, // given a invalid address
        .flags = 0xffff, // user define usb i2c command
        .buf = buf
    };           
    
#if 0 //def RTD2820_DEBUG
    int                         i;
#endif        
            
    buf[0]= 0x08;              // OPCODE
    buf[1]=(u8)n_arg_in;
    
    if (0xff== buf[1]){
		msg.len= 2;
	}
	else{
        memcpy(&buf[2], (u16*)p_arg_in, n_arg_in*sizeof(u16));
        msg.len= n_arg_in*sizeof(u16)+ 2;
        
#if 0 //def RTD2820_DEBUG
        printk("=========== %s %d set pid =============\n", __FILE__, __LINE__);
        for (i= 0; i< n_arg_in; i++)
            printk("0x%04x ", ((u16*)p_arg_in)[i]);
        printk("\n\n");
#endif
	}

    if (1!= i2c_transfer(p_state->i2c, &msg, 1)){
        printk("======= set pid filter to device fail =====\n");
        return 1;              
    }
    return 0;
}



#ifndef USE_USER_MEMORY

/*=======================================================
 * Func : rtd2820_mmap 
 *
 * Desc : memory map function for rtd2820
 *
 * Parm : fe   : handle of dvb frontend
 *        file : 
 *        vma  : 
 *
 * Retn : N/A
 *=======================================================*/
int rtd2820_mmap(
    struct dvb_frontend*        fe,
    struct file*                file,
    struct vm_area_struct*      vma)
{
    int ret = 0;
    struct rtd2820_state*       state = fe->demodulator_priv;
        
#ifdef CONFIG_REALTEK_VENUS
#else
    unsigned long               prot;
#endif

    if (vma->vm_end!= vma->vm_start + state->len_ts_mmap_region) {
		printk("%s %d\n", __FUNCTION__, __LINE__);
        ret = -EINVAL;
        goto bailout;
    }

    vma->vm_file = file;

#ifdef CONFIG_REALTEK_VENUS
    printk("========= define CONFIG_REALTEK_VENUS ========\n");
    vma->vm_page_prot = PAGE_KERNEL;
#else
    printk("========= undefine CONFIG_REALTEK_VENUS ========\n");
    prot = pgprot_val(vma->vm_page_prot);
    prot |= _PAGE_KERNEL;
    vma->vm_page_prot = __pgprot(prot);
#endif

    ret = remap_pfn_range(vma, vma->vm_start,
                          virt_to_phys(state->p_ts_mmap_region) >> PAGE_SHIFT,                          
                          vma->vm_end - vma->vm_start,
                          vma->vm_page_prot) ? -EAGAIN : 0;

    if (0== ret){ 
        state->p_ts_ring_info->offset_user_kernel= (int)vma->vm_start- (int)state->p_ts_mmap_region;
        printk("================ mmap successful ================\n");
    }
    
bailout:
    return ret;
}


/*=======================================================
 * Func : rtd2820_get_size 
 *
 * Desc : get size of mmap region 
 *
 * Parm : fe : handle of dvb frontend 
 *
 * Retn : size of mmap region 
 *=======================================================*/
int rtd2820_get_size(
    struct dvb_frontend*        fe
    )
{
    struct rtd2820_state* state = fe->demodulator_priv;
    return state->len_ts_mmap_region;
}


#else




/*=======================================================
 * Func : rtd2820_pre_setup 
 *
 * Desc : pre setup ring buffer
 *
 * Parm : fe : handle of dvb frontend 
 *        p_phys : physical address of buffer
 *        p_user_cached : cached user space address of buffer
 *        size : size of buffer
 *
 * Retn : 1 : success, 0 : fail
 *=======================================================*/
int rtd2820_pre_setup(
    struct dvb_frontend*        fe,
    unsigned char*              p_phys,
    unsigned char*              p_user_cached,
    int                         size
    )
{
    struct rtd2820_state*       state = fe->demodulator_priv;
    struct rtd2830_ts_buffer_s* p_ts_ring_info= NULL;
    int                         valid_buffer_size = size - sizeof(struct rtd2830_ts_buffer_s) - 188;
    int                         stream_buffer_size = size;

    state->len_ts_mmap_region = size;
    state->p_ts_mmap_region = phys_to_virt((long)p_phys);

    if (!state->p_ts_mmap_region){
        printk("%s: NULL memory specified\n", __FUNCTION__);
        return 0;
    }    

    if((state->flag & RTD2820_CHANNEL_SCAN_MODE_EN)){
        printk("%s : %d : Reset RTL2820 to Pass Through Mode \n", __FUNCTION__,__LINE__);
        state->flag &= ~RTD2820_CHANNEL_SCAN_MODE_EN;       // reset to pass through mode
    }

    state->config.urb_size = (valid_buffer_size < (47*1024))   ? 512  : 
                             (valid_buffer_size < (94*1024))   ? 1024 : 
                             (valid_buffer_size < (376*1024))  ? 2048 : 
                             (valid_buffer_size < (1504*1024)) ? 4096 : 32768;
                          
    switch (state->config.urb_size){    
    case 512:
        stream_buffer_size = (((int)((valid_buffer_size>>9) / 47)) * 47) << 9;
        break;                
    case 1024:
        stream_buffer_size = (((int)((valid_buffer_size>>10) / 47)) * 47) << 10;
        break;        
    case 2048:              // where 2048 was the limit of DIO
        stream_buffer_size = (((int)((valid_buffer_size>>10) / 94)) * 94) << 10;
        break;
    case 4096:
        stream_buffer_size = (((int)((valid_buffer_size>>10) / 376)) * 376) << 10;
        break;
    case 32768:        
        stream_buffer_size = (((int)((valid_buffer_size>>10) / 1504)) * 1504) << 10;                                
        break;            
    }
            
    state->config.urb_num = (int) (stream_buffer_size / state->config.urb_size);
    
    if (state->config.urb_num > 4) {        
        state->config.urb_num = 4;
    }
    
    printk(" ---------------------------------------\n");
    printk("| buffer size = %d\n", size);
    printk("| stream buffer size = %d\n", stream_buffer_size);
    printk("| urb_size = %d\n", state->config.urb_size);
    printk("| urb_num  = %d\n", state->config.urb_num);    
    printk(" ---------------------------------------\n");

    if (state->config.urb_num ==0){
        printk(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> FETAL ERROR : urb_num = 0 \n");                                
        return 0;
    }      
        
    p_ts_ring_info = RTD2830_GET_RING_INFO(state->p_ts_mmap_region, size);
        
    memset(p_ts_ring_info,0,sizeof(*p_ts_ring_info));
    
    p_ts_ring_info->p_lower    = 
    p_ts_ring_info->pp_read[0] = 
    p_ts_ring_info->pp_read[1] = 
    p_ts_ring_info->pp_read[2] =
    p_ts_ring_info->pp_read[3] = 
    p_ts_ring_info->p_write    = state->p_ts_mmap_region;       
    p_ts_ring_info->p_upper    = p_ts_ring_info->p_lower+ stream_buffer_size;

    p_ts_ring_info->p_b_active[0]= 
    p_ts_ring_info->p_b_active[1]=
    p_ts_ring_info->p_b_active[2]= 
    p_ts_ring_info->p_b_active[3]= 0;

    p_ts_ring_info->offset_user_kernel= (int)p_user_cached- (int)state->p_ts_mmap_region;

    //state->p_next_urb_buffer = p_ts_ring_info->p_lower + state->config.urb_num * state->config.urb_size;

    printk("============== %s: 0x%08x 0x%08x\n", 
            __FUNCTION__, 
            (int)p_ts_ring_info->p_lower,
            *((int*)p_ts_ring_info->p_lower)
            );
        
    printk("============== %s: p_urb_alloc =============\n", __FUNCTION__);
    
#ifdef URB_PARK_EN  
       
    if (state->p_urb_park != NULL)
        kfree(state->p_urb_park);
        
    state->len_urb_park = state->config.urb_num * state->config.urb_size;
    state->p_urb_park = kmalloc(state->len_urb_park, GFP_KERNEL);
    
    if (state->p_urb_park!=NULL)
        state->config.p_urb_alloc(state->config.dibusb, state->p_urb_park, state->config.urb_num, state->config.urb_size);    
    else
    {
        state->len_urb_park = 0;
        state->config.p_urb_alloc(state->config.dibusb, p_ts_ring_info->p_lower, state->config.urb_num, state->config.urb_size);
    }
        
#else 
        
    state->config.p_urb_alloc(state->config.dibusb, p_ts_ring_info->p_lower, state->config.urb_num, state->config.urb_size);    
    
#endif    
    

    printk("============== %s: 0x%08x 0x%08x\n", 
            __FUNCTION__, 
            (int)p_ts_ring_info->p_lower,
            *((int*)p_ts_ring_info->p_lower)
            );
    
    printk("%s: (lower, upper, write, read(0-3), ring_size)=\n(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
           __FUNCTION__,
           (int)p_ts_ring_info->p_lower,
           (int)p_ts_ring_info->p_upper,
           (int)p_ts_ring_info->p_write,
           (int)p_ts_ring_info->pp_read[0],
           (int)p_ts_ring_info->pp_read[1],
           (int)p_ts_ring_info->pp_read[2],
           (int)p_ts_ring_info->pp_read[3],
           (int)p_ts_ring_info->p_upper - (int)p_ts_ring_info->p_lower);
           
    printk("%s (user_cached, kernel_virt, diff1, diff2)= (0x%08x 0x%08x %d %d)\n",
            __FUNCTION__,
            (int)p_user_cached, 
            (int)state->p_ts_mmap_region,
            (int)p_user_cached - (int)state->p_ts_mmap_region,
            p_ts_ring_info->offset_user_kernel
            );
        
    state->p_ts_ring_info = p_ts_ring_info;          // restore address ot p_ts_ring_info  
    state->flag |= RTD2820_URB_RDY;
    return 1;
}


/*=======================================================
 * Func : rtd2820_post_setup 
 *
 * Desc : post setup ring buffer
 *
 * Parm : fe : handle of dvb frontend 
 *
 * Retn : 1 : success, 0 : fail
 *=======================================================*/
void rtd2820_post_setup(
    struct dvb_frontend*            fe
    )
{				
    struct rtd2820_state* state = fe->demodulator_priv;    
    
    if (state->p_ts_mmap_region)
    {
        printk("============== %s: p_urb_free =============\n", __FUNCTION__);
        printk("=================== %d %d =================\n", state->config.urb_num, state->config.urb_size);
                
        if((state->flag & RTD2820_STRAMING_ON)){            
            rtd2820_ts_set_status(fe, 0);   // stop streaming            
        }
            
        state->config.p_urb_free(state->config.dibusb);
        printk("============== %s: free_pages =============\n", __FUNCTION__);
        state->p_ts_mmap_region = NULL;
        
#ifdef URB_PARK_EN        
        if (state->p_urb_park!=NULL){
            kfree(state->p_urb_park);
            state->p_urb_park = NULL;
            state->len_urb_park = 0;
        }
#endif
        
    }
}

#endif




/*=======================================================
 * Func : rtd2820_channel_scan_mode_en
 *
 * Desc : enable channel scan mode
 *
 * Parm : fe : handle of dvb_frontend
 *        on_off : enable / disable
 *
 * Retn : N/A
 *=======================================================*/ 
void rtd2820_channel_scan_mode_en(
    struct dvb_frontend*            fe,
    unsigned char                   on_off
    )
{    
    struct rtd2820_state* state = fe->demodulator_priv;    
    unsigned char *p_buffer = state->p_ts_ring_info->p_lower;
    unsigned char ts_on = state->flag & RTD2820_STRAMING_ON;
    unsigned char cs_on = state->flag & RTD2820_CHANNEL_SCAN_MODE_EN;    
    unsigned long urb_size = (on_off) ? 512 : state->config.urb_size;    
    unsigned long urb_num  = (on_off) ? 1   : state->config.urb_num;
    
        
    if ((on_off && !cs_on) || (!on_off && cs_on))
    {
#if 1        
        if (on_off)                                  
            printk("%s : %d :    Set RTL282x to Channel Scan Mode\n", __FILE__,__LINE__);
        else
            printk("%s : %d :    Set RTL282x to Pass Through Mode\n", __FILE__,__LINE__);
#endif        

        if (ts_on)            
            rtd2820_ts_set_status(fe, 0);                       // stop streaming         
                    
        state->config.p_urb_free(state->config.dibusb);         // free urb               
        
        state->flag = (on_off) ? (state->flag |= RTD2820_CHANNEL_SCAN_MODE_EN) : 
                                 (state->flag &=~RTD2820_CHANNEL_SCAN_MODE_EN);
                                         
#ifdef URB_PARK_EN                   
        if (!on_off && state->p_urb_park!=NULL)
            p_buffer = state->p_urb_park;                       // redirect urb to the urb_park
#endif                                  
        state->config.p_urb_alloc(state->config.dibusb,         // realloc urb with minimum size
                                  p_buffer,
                                  urb_num,                     
                                  urb_size);        
                
        if (ts_on)
            rtd2820_ts_set_status(fe, 1);                       // restart streaming                   
                    
    }        
}    




//////////////////////////////////////////////////////////
// AUX Functions for Memory Map
//---------------------------------------------------------
// rtd2820_mmap_reserved / rtd2820_mmap_unreserved
// rtd2820_get_writeable_size_single / rtd2820_get_writeable_size
//////////////////////////////////////////////////////////



#ifndef USE_USER_MEMORY


/*=======================================================
 * Func : rtd2820_mmap_reserved 
 *
 * Desc : reserve memory for mmap
 *
 * Parm : p_buffer : buffer address
 *        len_buffer : buffer length
 *
 * Retn : N/A
 *=======================================================*/
void rtd2820_mmap_reserved(
    u8*                         p_buffer,
    int                         len_buffer
    )
{
    struct page*                p_page;

    for (p_page= virt_to_page(p_buffer); p_page< virt_to_page(p_buffer+ len_buffer); p_page++)
        SetPageReserved(p_page);
}




/*=======================================================
 * Func : rtd2820_mmap_unreserved 
 *
 * Desc : unreserve memory for mmap
 *
 * Parm : p_buffer : buffer address
 *        len_buffer : buffer length
 *
 * Retn : N/A
 *=======================================================*/
void rtd2820_mmap_unreserved(
    u8*                         p_buffer,
    int                         len_buffer
    )
{
    struct page*                p_page;

    for (p_page= virt_to_page(p_buffer); p_page< virt_to_page(p_buffer+ len_buffer); p_page++)
        ClearPageReserved(p_page);
}

#endif // ifndef USE_USER_MEMORY




#ifdef RTD2820_DEBUG

/*=======================================================
 * Func : rtd2820_get_writeable_size 
 *
 * Desc : compute write able size - for 4 read pointer
 *
 * Parm : p_ts_ring_info : transport stream ring buffer
 *
 * Retn : N/A
 *=======================================================*/
int rtd2820_get_writeable_size(
    struct rtd2830_ts_buffer_s* p_ts_ring_info
    )
{
    int i;
    int writeable_size;
    int ring_size;
    
    ring_size= writeable_size= (int)(p_ts_ring_info->p_upper- p_ts_ring_info->p_lower);
    for (i= 0; i< 4; i++){
        if (!p_ts_ring_info->p_b_active[i])
            continue;
        writeable_size= MIN(writeable_size,
                            rtd2820_get_writeable_size_single(p_ts_ring_info, ring_size, i));
    }
    return writeable_size;
}



/*=======================================================
 * Func : rtd2820_get_writeable_size_single 
 *
 * Desc : compute write able size - for single read pointer
 *
 * Parm : p_ts_ring_info : transport stream ring buffer
 *
 * Retn : N/A
 *=======================================================*/
int rtd2820_get_writeable_size_single(
    struct rtd2830_ts_buffer_s* p_ts_ring_info,
    int                         ring_size,
    int                         index_read
    )
{
    int                         size_writeable;
    
    if (p_ts_ring_info->p_write>= p_ts_ring_info->pp_read[index_read])
        size_writeable= ring_size- (p_ts_ring_info->p_write- p_ts_ring_info->pp_read[index_read])- 1;
    else
        size_writeable= p_ts_ring_info->pp_read[index_read]- p_ts_ring_info->p_write- 1;
    
    return size_writeable;
}

#endif






module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Turn on/off frontend debugging (default:off).");

MODULE_DESCRIPTION("Realtek RTD28s0 ATSC Demodulator driver");
MODULE_AUTHOR("Kevin Wang");
MODULE_LICENSE("GPL");
EXPORT_SYMBOL(rtd2820_attach);
EXPORT_SYMBOL(rtd2820_i2c_repeat);
