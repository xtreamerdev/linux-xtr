/*
 *  Driver for Realtek DVB-T rtl2830 demodulator
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <asm/pgtable.h>
#include "dvb_frontend.h"
#include "rtl2830_priv.h"
#include "rtl2830.h"
#include "rtl2830_api/foundation.h"
#include "rtl2830_api/demod_rtl2830.h"

//-------- MACROS ---------
static int debug;

#define dprintk(args...) \
    do { \
        if (debug) printk(KERN_DEBUG "rtl2830: " args); \
    } while (0)

#define MIN(a, b) ((a)< (b)? (a): (b))

#define DEMOD_UPDATE_INTERVAL 100
//-------- Dvb Frontend Functions
int  rtd2830_init(struct dvb_frontend* fe);
int  rtd2830_sleep(struct dvb_frontend* fe);
void rtd2830_release(struct dvb_frontend* fe);    
int  rtd2830_set_parameters(struct dvb_frontend* fe, struct dvb_frontend_parameters* param);
int  rtd2830_get_parameters(struct dvb_frontend* fe, struct dvb_frontend_parameters* param);
int  rtd2830_get_tune_settings(struct dvb_frontend* fe, struct dvb_frontend_tune_settings* fe_tune_settings);
int  rtd2830_read_status(struct dvb_frontend* fe, fe_status_t* status);
int  rtd2830_read_ber(struct dvb_frontend* fe, u32* ber);
int  rtd2830_read_signal_strength(struct dvb_frontend* fe, u16* strength);
int  rtd2830_read_snr(struct dvb_frontend* fe, u16* snr);
int  rtd2830_read_ucblocks(struct dvb_frontend* fe, u32* ucblocks);
int  rtd2830_user_cmd(struct dvb_frontend* fe, int cmd, int* p_arg_in, int n_arg_in, int* p_arg_out, int n_arg_out);
void rtd2830_channel_scan_mode_en(struct dvb_frontend* fe, unsigned char on_off);

//-------- User Command Functions 
int  rtd2830_ts_set_status(struct dvb_frontend* fe, int on_off);
int  rtd2830_pid_filter(struct dvb_frontend* fe, int* p_arg_in, int n_arg_in);
int  rtd2830_copy_from_usb(struct dvb_frontend* fe, void** pp_new_urb_buffer, u8* p_buffer, int len_buffer);

//-------- Internal Used Functions  ---------//        
#ifdef RTD2830_DEBUG
    int  rtd2830_get_writeable_size(struct rtd2830_ts_buffer_s* p_ts_ring_info);
    int  rtd2830_get_writeable_size_single(struct rtd2830_ts_buffer_s* p_ts_ring_info, int ring_size, int index_read);
#endif

#ifndef USE_USER_MEMORY
    int  rtd2830_mmap(struct dvb_frontend* fe, struct file* file, struct vm_area_struct* vma);
    int  rtd2830_get_size(struct dvb_frontend* fe);
    void rtd2830_mmap_reserved(u8* p_buffer, int len_buffer);
    void rtd2830_mmap_unreserved(u8* p_buffer, int len_buffer);
#else
    int  rtd2830_pre_setup(struct dvb_frontend* fe, unsigned char* p_phys, unsigned char* p_user_cached, int size);
    void rtd2830_post_setup(struct dvb_frontend* fe);        
#endif

//-------- Internal Used Arguments  ---------//        
struct dvb_frontend_ops rtd2830_ops = 
{
    .info = {
        .name                = "Realtek RTD2830 DVB-T",
        .type                = FE_OFDM,
        .frequency_min       = 174000000,
        .frequency_max       = 862000000,
        .frequency_stepsize  = 166667,
        .frequency_tolerance = 0,
        .caps                = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 |
                               FE_CAN_FEC_3_4 | FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 |
                               FE_CAN_FEC_AUTO |
                               FE_CAN_QPSK | FE_CAN_QAM_16 | FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
                               FE_CAN_TRANSMISSION_MODE_AUTO | FE_CAN_GUARD_INTERVAL_AUTO |
                               FE_CAN_HIERARCHY_AUTO | FE_CAN_RECOVER |
                               FE_CAN_MUTE_TS
    },

    .init =                     rtd2830_init,
    .release =                  rtd2830_release,

    .sleep =                    rtd2830_sleep,

    .set_frontend =             rtd2830_set_parameters,
    .get_frontend =             rtd2830_get_parameters,
    .get_tune_settings =        rtd2830_get_tune_settings,

    .read_status =              rtd2830_read_status,
    .read_ber =                 rtd2830_read_ber,
    .read_signal_strength =     rtd2830_read_signal_strength,
    .read_snr =                 rtd2830_read_snr,
    .read_ucblocks =            rtd2830_read_ucblocks,
    .user_cmd=                  rtd2830_user_cmd,
};

//-------- OSAL ---------
int rtl2830_AllocateMemory(void **ppBuffer, unsigned int ByteNum)
{
    *ppBuffer = kmalloc(ByteNum, GFP_KERNEL);
    return (*ppBuffer != NULL) ? MEMORY_ALLOCATING_SUCCESS : MEMORY_ALLOCATING_ERROR;
}

void rtl2830_FreeMemory(void *pBuffer)
{
    kfree(pBuffer);
}


void rtl2830_Wait(unsigned int WaitTimeMs)
{
    msleep(WaitTimeMs);
}


int rtl2830_SendReadCommand(
	unsigned char DeviceAddr,
	unsigned char *pReadingBytes,
	unsigned char ByteNum)
{
    printk("Warning : SendReadCommand : does not implement\n");
    return I2C_SUCCESS;
}

int rtl2830_SendWriteCommand(
	unsigned char DeviceAddr,
	const unsigned char *pWritingBytes,
	unsigned char ByteNum)
{    
    printk("Warning : SendWriteCommand : does not implement\n");
    return I2C_SUCCESS;
}


void rtl2830_UpdateFunction(DEMOD_MODULE *pDemod)
{    

#if 0
    static int count = 0;
    if (count++ >= 10){
        printk("rtl2830_UpdateFunction (10)\n");      
        count = 0;
    }
#endif
    
    if (pDemod->UpdateFunction2(pDemod)!=FUNCTION_SUCCESS){
        printk("rtl2830_UpdateFunction (2) fail\n");        
        return;
    } 
    
    if (pDemod->UpdateFunction3(pDemod)!=FUNCTION_SUCCESS){
        printk("rtl2830_UpdateFunction (3) fail\n");
        return;
    }        
        
    if (pDemod->UpdateFunction4(pDemod)!=FUNCTION_SUCCESS){
	    printk("rtl2830_UpdateFunction (4) fail\n");
	    return;
    }
    
    if (pDemod->UpdateFunction5(pDemod)!=FUNCTION_SUCCESS){
	    printk("rtl2830_UpdateFunction (5) fail\n");
	    return;
    }
}


static struct rtd2830_reg_addr rtd2830_reg_map[]= {
    /* RTD2830_RMAP_INDEX_BW_INDEX          */      { RTD2830_PAGE_0, 0x08, 1, 2    },
    /* RTD2830_RMAP_INDEX_SAMPLE_RATE_INDEX */      { RTD2830_PAGE_0, 0x08, 3, 3    },
    /* RTD2830_RMAP_INDEX_PFREQ1_0          */      { RTD2830_PAGE_0, 0x0D, 0, 1    },
    /* RTD2830_RMAP_INDEX_PD_DA8            */      { RTD2830_PAGE_0, 0x0D, 4, 4    },
    /* RTD2830_RMAP_INDEX_SOFT_RESET        */      { RTD2830_PAGE_1, 0x01, 2, 2    },
    /* RTD2830_RMAP_INDEX_LOOP_GAIN0_3      */      { RTD2830_PAGE_1, 0x04, 1, 4    },
    /* RTD2830_RMAP_INDEX_LOCK_TH           */      { RTD2830_PAGE_1, 0x10, 0, 1    },
    /* RTD2830_RMAP_INDEX_SPEC_INV          */      { RTD2830_PAGE_1, 0x15, 0, 0    },
    /* RTD2830_RMAP_INDEX_H_LPF_X           */      { RTD2830_PAGE_1, 0x1C, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_0           */      { RTD2830_PAGE_1, 0x1C, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_1           */      { RTD2830_PAGE_1, 0x1E, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_2           */      { RTD2830_PAGE_1, 0x20, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_3           */      { RTD2830_PAGE_1, 0x22, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_4           */      { RTD2830_PAGE_1, 0x24, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_5           */      { RTD2830_PAGE_1, 0x26, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_6           */      { RTD2830_PAGE_1, 0x28, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_7           */      { RTD2830_PAGE_1, 0x2A, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_8           */      { RTD2830_PAGE_1, 0x2C, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_9           */      { RTD2830_PAGE_1, 0x2E, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_10          */      { RTD2830_PAGE_1, 0x30, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_11          */      { RTD2830_PAGE_1, 0x32, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_12          */      { RTD2830_PAGE_1, 0x34, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_13          */      { RTD2830_PAGE_1, 0x36, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_14          */      { RTD2830_PAGE_1, 0x38, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_15          */      { RTD2830_PAGE_1, 0x3A, 0, 15   },
    /* RTD2830_RMAP_INDEX_H_LPF_16          */      { RTD2830_PAGE_1, 0x3C, 0, 15   },
    /* RTD2830_RMAP_INDEX_TRK_KC_P1         */      { RTD2830_PAGE_1, 0x73, 0, 2    },
    /* RTD2830_RMAP_INDEX_TRK_KC_P2         */      { RTD2830_PAGE_1, 0x73, 3, 5    },
    /* RTD2830_RMAP_INDEX_TRK_KC_I1         */      { RTD2830_PAGE_1, 0x74, 3, 5    },
    /* RTD2830_RMAP_INDEX_TRK_KC_I2         */      { RTD2830_PAGE_1, 0x75, 0, 2    },
    /* RTD2830_RMAP_INDEX_CKOUT_PWR         */      { RTD2830_PAGE_1, 0x7B, 6, 6    },
    /* RTD2830_RMAP_INDEX_BER_PASS_SCAL     */      { RTD2830_PAGE_1, 0x8C, 6, 11   },
    /* RTD2830_RMAP_INDEX_TR_WAIT_MIN_8K    */      { RTD2830_PAGE_1, 0x88, 2, 11   },
    /* RTD2830_RMAP_INDEX_RSD_BER_FAIL_VAL  */      { RTD2830_PAGE_1, 0x8F, 0, 15   },
    /* RTD2830_RMAP_INDEX_MGD_THD0          */      { RTD2830_PAGE_1, 0x95, 0, 7    },
    /* RTD2830_RMAP_INDEX_MGD_THD1          */      { RTD2830_PAGE_1, 0x96, 0, 7    },
    /* RTD2830_RMAP_INDEX_MGD_THD2          */      { RTD2830_PAGE_1, 0x97, 0, 7    },
    /* RTD2830_RMAP_INDEX_MGD_THD3          */      { RTD2830_PAGE_1, 0x98, 0, 7    },
    /* RTD2830_RMAP_INDEX_MGD_THD4          */      { RTD2830_PAGE_1, 0x99, 0, 7    },
    /* RTD2830_RMAP_INDEX_MGD_THD5          */      { RTD2830_PAGE_1, 0x9A, 0, 7    },
    /* RTD2830_RMAP_INDEX_MGD_THD6          */      { RTD2830_PAGE_1, 0x9B, 0, 7    },
    /* RTD2830_RMAP_INDEX_MGD_THD7          */      { RTD2830_PAGE_1, 0x9C, 0, 7    },
    /* RTD2830_RMAP_INDEX_ADD_WRITE         */      { RTD2830_PAGE_1, 0x9D, 0, 47   }, // @FIXME: not longer than 31
    /* RTD2830_RMAP_INDEX_CE_FFSM_BYPASS    */      { RTD2830_PAGE_2, 0xD5, 1, 1    },
    /* RTD2830_RMAP_INDEX_ALPHAIIR_N        */      { RTD2830_PAGE_2, 0xF1, 1, 2    },
    /* RTD2830_RMAP_INDEX_ALPHAIIR_DIF      */      { RTD2830_PAGE_2, 0xF1, 3, 7    },
    /* RTD2830_RMAP_INDEX_FSM_STAGE         */      { RTD2830_PAGE_3, 0x51, 3, 6    },
    /* RTD2831_RMAP_INDEX_USB_STAT          */      { RTD2831U_USB, USB_STAT, 0, 7    },
    /* RTD2831_RMAP_INDEX_USB_EPA_CTL       */      { RTD2831U_USB, USB_EPA_CTL, 0, 31    },
    /* RTD2831_RMAP_INDEX_USB_SYSCTL        */      { RTD2831U_USB, USB_SYSCTL, 0, 31    },
    /* RTD2831_RMAP_INDEX_USB_EPA_CFG       */      { RTD2831U_USB, USB_EPA_CFG, 0, 31   },
    /* RTD2831_RMAP_INDEX_USB_EPA_MAXPKT    */      { RTD2831U_USB, USB_EPA_MAXPKT, 0, 31   },
    /* RTD2831_RMAP_INDEX_SYS_GPD           */      { RTD2831U_SYS, GPD, 0, 7   },
    /* RTD2831_RMAP_INDEX_SYS_GPOE          */      { RTD2831U_SYS, GPOE, 0, 7   },
    /* RTD2831_RMAP_INDEX_SYS_GPO           */      { RTD2831U_SYS, GPO, 0, 7   },
    /* RTD2831_RMAP_INDEX_SYS_SYS_0         */      { RTD2831U_SYS, SYS_0, 0, 7   },
    /* RTD2830_RMAP_INDEX_SET_IF_FREQ       */      { RTD2830_PAGE_1, 0x19, 0, 20},
};                                          


static int rtd2830_reg_mask[32]= {
    0x00000001,
    0x00000003,
    0x00000007,
    0x0000000f,
    0x0000001f,
    0x0000003f,
    0x0000007f,
    0x000000ff,
    0x000001ff,
    0x000003ff,
    0x000007ff,
    0x00000fff,
    0x00001fff,
    0x00003fff,
    0x00007fff,
    0x0000ffff,
    0x0001ffff,
    0x0003ffff,
    0x0007ffff,
    0x000fffff,
    0x001fffff,
    0x003fffff,
    0x007fffff,
    0x00ffffff,
    0x01ffffff,
    0x03ffffff,
    0x07ffffff,
    0x0fffffff,
    0x1fffffff,
    0x3fffffff,
    0x7fffffff,
    0xffffffff
};



static int rtd2830_page_switch(
    struct rtd2830_state*       p_state,
    u8                          page_addr)
{
    u8                          buf[2]= { RTD2830_PAGE_REG, page_addr };
    struct i2c_msg msg;
            
    msg.addr  = p_state->config.demod_address,
    msg.flags = 0,
    msg.buf   = buf,
    msg.len   = 2;

    if (p_state->version != RTL2831U){                    
        if (1!= i2c_transfer(p_state->i2c, &msg, 1)){
            printk("%s write to reg 0x%x failed\n", __FUNCTION__, RTD2830_PAGE_REG);
            return 1;
        }
    }
    return 0;
}



static int rtd2830_read_byte(
    struct rtd2830_state*       p_state,
    int                         page_addr,
    int                         byte_addr,
    int*                        p_val)
{
    if (p_state->version == RTL2831U)
    {     
        u8 b0[3]= { page_addr,byte_addr&0xff,byte_addr>>8};
        u8 b1[] = { 0 };
        struct i2c_msg msg[] = {
            { .addr = p_state->config.demod_address,
              .flags = 0,
              .buf = b0,
              .len = 3 
            },
            { .addr = p_state->config.demod_address,
              .flags = I2C_M_RD,
              .buf = b1,
              .len = 1 
            }
        };
    
        if (2!= i2c_transfer(p_state->i2c, msg, 2)){
            printk("%s: readreg error reg=0x%x\n", __FUNCTION__, byte_addr);
            return 2;
        }
    
        *p_val= (int)b1[0];
        return 0;
    }
    else
    {
        u8 b0[]=                    { byte_addr};
        u8 b1[]=                    { 0 };
        struct i2c_msg              msg[] = {{ .addr = p_state->config.demod_address,
                                               .flags = 0,
                                               .buf = b0,
                                               .len = 1 },
                                             { .addr = p_state->config.demod_address,
                                               .flags = I2C_M_RD,
                                               .buf = b1,
                                               .len = 1 }};
    
        if (rtd2830_page_switch(p_state, page_addr))
            return 1;
        if (2!= i2c_transfer(p_state->i2c, msg, 2)){
            printk("%s: readreg error reg=0x%x\n", __FUNCTION__, byte_addr);
            return 2;
        }
    
        *p_val= (int)b1[0];
        return 0;
    }
}


static int rtd2830_write_byte(
    struct rtd2830_state*       p_state,
    int                         page_addr,
    int                         byte_addr,
    int                         val)
{
    if (p_state->version == RTL2831U)
    {        
        u8              buf[4] = { page_addr, byte_addr&0xff, byte_addr>>8, val };
        struct i2c_msg  msg = {
                        .addr = p_state->config.demod_address,
                        .flags = 0,
                        .buf = buf,
                        .len = 4 };
#if 0                                        
        printk("*******************************************\n");
        printk("rtd2830_write_byte()page_addr=%x\n",page_addr);
        printk("rtd2830_write_byte()byte_addr=%x\n",byte_addr);
        printk("rtd2830_write_byte()val=%x\n",val);
#endif        
        if (1!= i2c_transfer(p_state->i2c, &msg, 1)){
            printk("%s write to reg 0x%x failed\n", __FUNCTION__, byte_addr);
            return 2;
        }
        return 0;
    }
    else
    {
        u8                          buf[2]= { byte_addr, val };
        struct i2c_msg              msg= {
                                        .addr = p_state->config.demod_address,
                                        .flags = 0,
                                        .buf = buf,
                                        .len = 2 };
    
        if (rtd2830_page_switch(p_state, page_addr))
            return 1;
            
        if (1!= i2c_transfer(p_state->i2c, &msg, 1)){
            printk("%s write to reg 0x%x failed\n", __FUNCTION__, byte_addr);
            return 2;
        }
    }
    return 0;
}

static int rtd2830_read_multi_byte(
    struct rtd2830_state*       p_state,
    int                         page_addr,
    int                         byte_addr,
    int                         n_bytes,
    int*                        p_val)
{
    int                         i;
    int                         val;
    int                         nbit_shift;

    *p_val= 0;
    for (i= 0; i< n_bytes; i++)
    {
        val= 0;
        if (rtd2830_read_byte(p_state, page_addr, byte_addr+ i, &val)){
            printk("%s: error in reading (page_addr, byte_addr)= (0x%x 0x%x)\n", __FUNCTION__, page_addr, byte_addr+ i);
            return 1;
        }
        
        if (p_state->version == RTL2831U)
        {
            if(page_addr<RTD2831U_USB)	    
	            *p_val= (*p_val<< 8)+ val;	    
	        else{
                nbit_shift= i<<3 ;
                *p_val= *p_val+ (val<<nbit_shift);
	        }
        }
        else{
            *p_val= (*p_val<< 8)+ val;
        }
    }
    return 0;
}


static int
rtd2830_write_multi_byte(
    struct rtd2830_state*       p_state,
    int                         page_addr,
    int                         byte_addr,
    int                         n_bytes,
    int                         val)
{
    int                         i; 
    int                         nbit_shift;
    
#if 0    
    printk("rtd2830_write_multi_byte**************************************************\n");
    printk("rtd2830_write_multi_byte()page_addr=%x\n",page_addr);
    printk("rtd2830_write_multi_byte()byte_addr=%x\n",byte_addr);
    printk("rtd2830_write_multi_byte()n_bytes=%x\n",n_bytes);
    printk("rtd2830_write_multi_byte()val=%x\n",val);
#endif
    
    for (i= n_bytes- 1; i>= 0; i--)
    {
        if (p_state->version==RTL2831U)
        {
            if(page_addr<RTD2831U_USB)
                nbit_shift= (n_bytes- i- 1) << 3;
	        else
	            nbit_shift= i << 3;
        }
        else{
            nbit_shift= (n_bytes- i- 1) << 3;
        }

        if (rtd2830_write_byte(p_state, page_addr, byte_addr+ i, (val>> nbit_shift) & 0xff)){
            printk("%s: error in writing (page_addr, byte_addr)= (0x%x 0x%x)\n", __FUNCTION__, page_addr, byte_addr+ i);
            return 1;
        }
    }
    return 0;
}



static int rtd2830_read_register(
    struct rtd2830_state*       p_state,
    rtd2830_reg_map_index       reg_map_index,
    int*                        p_val)
{
    int page_addr   = rtd2830_reg_map[reg_map_index].page_addr;
    int reg_addr    = rtd2830_reg_map[reg_map_index].reg_addr;
    int bit_low     = rtd2830_reg_map[reg_map_index].bit_low;
    int bit_high    = rtd2830_reg_map[reg_map_index].bit_high;
    int n_byte_read = (bit_high>> 3)+ 1;

    *p_val= 0;
    if (rtd2830_read_multi_byte(p_state, page_addr, reg_addr, n_byte_read, p_val)){
        printk("%s: error\n", __FUNCTION__);
        return 1;
    }

    *p_val= ((*p_val>> bit_low) & rtd2830_reg_mask[bit_high- bit_low]);
    return 0;
}



static int rtd2830_write_register(
    struct rtd2830_state*       p_state,
    rtd2830_reg_map_index       reg_map_index,
    int                         val_write)
{
    int                         page_addr=      rtd2830_reg_map[reg_map_index].page_addr;
    int                         reg_addr=       rtd2830_reg_map[reg_map_index].reg_addr;
    int                         bit_low=        rtd2830_reg_map[reg_map_index].bit_low;
    int                         bit_high=       rtd2830_reg_map[reg_map_index].bit_high;
    int                         n_byte_write=   (bit_high>> 3)+ 1;
    int                         val_read= 0;
    int                         new_val_write;

    if (rtd2830_read_multi_byte(p_state, page_addr, reg_addr, n_byte_write, &val_read)){
        printk("%s: error in read\n", __FUNCTION__);
        return 1;
    }

    new_val_write= (val_read & (~(rtd2830_reg_mask[bit_high- bit_low]<< bit_low))) | (val_write<< bit_low);

    if (rtd2830_write_multi_byte(p_state, page_addr, reg_addr, n_byte_write, new_val_write)){
        printk("%s: error in write\n", __FUNCTION__);
        return 2;
    }
    return 0;
}




static int rtd2830_write_register_table(
    struct rtd2830_state*       p_state,
    rtd2830_reg_map_index       reg_map_index,
    const u8*                   p_table,
    int                         len_table)
{
    int                         page_addr=      rtd2830_reg_map[reg_map_index].page_addr;
    int                         reg_addr=       rtd2830_reg_map[reg_map_index].reg_addr;
    int                         i;

    for (i= 0; i< len_table; i++){
        if (rtd2830_write_byte(p_state, page_addr, reg_addr+ i, (int)(p_table[i]))){
            printk("%s: write fail: (reg, val)= (%x %x) \n", __FUNCTION__, reg_addr+ i, (int)(p_table[i]));
            return 1;
        }
    }
    return 0;
}


int rtd2830_soft_reset(struct rtd2830_state* p_state)
{
    DEMOD_MODULE* pRT2830 = (DEMOD_MODULE*) p_state->pRT2830;
    return (pRT2830->SoftwareReset(pRT2830)==FUNCTION_SUCCESS) ? 0 : 1;    
}


int rtd2830_frequency_change_reset(struct rtd2830_state* p_state)
{        

    DEMOD_MODULE* pRT2830 = (DEMOD_MODULE*) p_state->pRT2830;    
    
    // Reset Update Function 4 / 5
    if (pRT2830->ResetFunction4(pRT2830)!=FUNCTION_SUCCESS){
	    printk("rtl2830_ResetFunction (4) fail\n");
	    return -1;
    }
    
    if (pRT2830->ResetFunction5(pRT2830)!=FUNCTION_SUCCESS){
	    printk("rtl2830_ResetFunction (5) fail\n");
	    return -1;
    }
    
#if 1
    rtd2830_soft_reset(p_state);
#endif
    
    return 0;
}


int rtd2830_general_init(struct rtd2830_state* p_state)
{    
    DEMOD_MODULE* pRT2830 = (DEMOD_MODULE*) p_state->pRT2830;
        
    u8 op_mode[4][2]= {
        {OUT_IF_SERIAL,     OUT_FREQ_6MHZ},
        {OUT_IF_SERIAL,     OUT_FREQ_10MHZ},
        {OUT_IF_PARALLEL,   OUT_FREQ_6MHZ},
        {OUT_IF_PARALLEL,   OUT_FREQ_10MHZ},
    };
    
    printk("rtd2830_general_init : RTL2830_DRV_20070509\n");
    
    if (p_state->output_mode > RTL2830_MAX_OUTPUTMODE ){
        p_state->output_mode = RTL2830_6MHZ_PARALLEL_OUT;
    }    
    
    return (pRT2830->Initialize(
	            pRT2830,
	            DONGLE_APPLICATION, 
	            IF_FREQ_36125000HZ,
	            SPECTRUM_INVERSE,
	            0x3F,       // 0x3F for QT1010, 0x20 for others
	            0x04,
	            op_mode[p_state->output_mode][0],
	            op_mode[p_state->output_mode][1])) ? 0 : 1;                                           
}




static int rtd2830_set_bandwidth(
    struct rtd2830_state*       p_state,
    enum   fe_bandwidth         bandwidth)
{
    static const u8 If_Freq[3]={0x30,0,0};  //set IF frequency= 36MHz for Thomson_FE664X
    
    DEMOD_MODULE* pRT2830 = (DEMOD_MODULE*) p_state->pRT2830;
    
    switch(bandwidth){
    case BANDWIDTH_6_MHZ: bandwidth = BANDWIDTH_6MHZ; break;    
    case BANDWIDTH_7_MHZ: bandwidth = BANDWIDTH_7MHZ; break;    
	case BANDWIDTH_8_MHZ: bandwidth = BANDWIDTH_8MHZ; break;    	    
	default:              bandwidth = BANDWIDTH_8MHZ; break;    	    	    
    };

    if (pRT2830->SetBandwidthMode(pRT2830, bandwidth) == FUNCTION_ERROR)     	    
        return 1;
    
    if(p_state->ext_flag){
        printk("FE Set Bandwidth\n");                
        if (rtd2830_write_register_table(p_state, RTD2830_RMAP_INDEX_SET_IF_FREQ, If_Freq, 3)){
            printk("%s: RTD2830_RMAP_INDEX_ADD_WRITE fail\n", __FUNCTION__);
            return 1;
        }            
    }
    return 0;    
}





// Demod I2C functions

// Set demod register bytes.
//
// 1. Set demod page register with PageNo.
// 2. Set demod registers sequentially with pWritingBytes.
//    The start register address is RegStartAddr.
//    The writing data length is ByteNum bytes.
//
int
rtl2831_SetRegBytes(
	DEMOD_MODULE*        pDemod,
	unsigned char        PageNo,
	unsigned char        RegStartAddr,
	const unsigned char *pWritingBytes,
	unsigned char        ByteNum
	)
{ 	
    int i, ret;    
    
    for(i=0; i<ByteNum; i++)
    {
        ret = rtd2830_write_byte( 
                pDemod->pExtra,
                PageNo,
                RegStartAddr + i,                                
                pWritingBytes[i]);
                
        if (ret!=0)
            goto error_status_set_bytes_fail;
    }
    
    return REG_ACCESS_SUCCESS;          
                
//---            
error_status_set_bytes_fail:

    printk("rtl2831_SetRegBytes Fail\n");
    printk("rtl2831_SetRegBytes : Page    : %d\n",PageNo);
    printk("rtl2831_SetRegBytes : Address : %d\n",RegStartAddr);
    printk("rtl2831_SetRegBytes : ByteNum : %d\n",ByteNum);      
    printk("rtl2831_SetRegBytes : pWritingBytes : ");      
    for (i = 0; i < ByteNum; i++)
        printk("%02x ",pWritingBytes[i]);      
    printk("\n");      

    return REG_ACCESS_ERROR;
}


// Get demod register bytes.
//
// 1. Set demod page register with PageNo.
// 2. Set the start register address to demod for reading.
//    The start register address is RegStartAddr.
// 3. Get demod registers sequentially and store register values to pReadingBytes.
//    The reading data length is ByteNum bytes.
//
int
rtl2831_GetRegBytes(
	DEMOD_MODULE *pDemod,
	unsigned char PageNo,
	unsigned char RegStartAddr,
	unsigned char *pReadingBytes,
	unsigned char ByteNum
	)
{	       
    int i, ret, val;    
    
    for(i=0; i<ByteNum; i++)
    {
        ret = rtd2830_read_byte( 
                pDemod->pExtra,
                PageNo,
                RegStartAddr + i,                                
                &val);
                
        pReadingBytes[i] = (unsigned char) val;
        
        if (ret!=0)
            goto error_status_get_bytes_fail;
    }
    
    return REG_ACCESS_SUCCESS;          
                
//---            
error_status_get_bytes_fail:

    printk("rtl2831_GetRegBytes Fail\n");
    printk("rtl2831_GetRegBytes : Page    : %d\n",PageNo);
    printk("rtl2831_GetRegBytes : Address : %d\n",RegStartAddr);
    printk("rtl2831_GetRegBytes : ByteNum : %d\n",ByteNum);              
    printk("\n");      

    return REG_ACCESS_ERROR;
}




/*=======================================================
 * Func : rtd2830_attach 
 *
 * Desc : construct a dvb_frontend object for rtd2830
 *
 * Parm : config : handle of rtd2830_config
 *        i2c : i2c repeater
 *
 * Retn : handle of dvb_frontend
 *=======================================================*/
struct dvb_frontend* rtd2830_attach(
        const struct rtd2830_config*        config,
        struct i2c_adapter*                 i2c
        )
{
    struct rtd2830_state* state= NULL;           
    unsigned char version = 0;
    DEMOD_MODULE* pRT2830 = NULL;       
      
#ifndef USE_USER_MEMORY
    struct rtd2830_ts_buffer_s* p_ts_ring_info;
#endif  

    switch(config->version){
    case RTL2830 : 
        version = DEMOD_RTL2830;     
        printk("RTL2830 SubType : RTL2830\n");
        break;    
    case RTL2830B: 
        version = DEMOD_RTL2830B;      
        printk("RTL2830 SubType : RTL2830-B\n");
        break;
    case RTL2831U: 
        version = DEMOD_RTL2831U;      
        printk("RTL2830 SubType : RTL2831U\n");
        break;
    default:        
        printk("rtd2830_attach : Fail, Unknow RTL2830 Sub Type %d\n", version);
        return NULL;
    }    
    
    /* allocate memory for the internal state */
    state = kmalloc(sizeof(struct rtd2830_state), GFP_KERNEL);
    if (state == NULL) 
        goto error;
        
    memset(state,0,sizeof(*state));


    printk("rtd2830_attach : RTL2830_DRV_2007\n");
    
    // Init OSAL Functions
    AllocateMemory   = rtl2830_AllocateMemory;
    FreeMemory       = rtl2830_FreeMemory;
    Wait             = rtl2830_Wait;
    SendReadCommand  = rtl2830_SendReadCommand;
    SendWriteCommand = rtl2830_SendWriteCommand;        
    
    // Init OSAL Functions    
       
    
    if(ConstructRtl2830Module(&pRT2830, version, config->demod_address,DEMOD_UPDATE_INTERVAL)
        ==MODULE_CONSTRUCTING_ERROR)
    {
        printk("%s: construct RTL2830 Module Fail\n", __FUNCTION__);
        kfree(state);
        return NULL;
    }               
    //--- Overwrite Set/Get Function for USB Version    
    pRT2830->SetRegBytes = rtl2831_SetRegBytes;
    pRT2830->GetRegBytes = rtl2831_GetRegBytes;  
    pRT2830->pExtra = (void*) state;    
    state->pRT2830  = (void*)pRT2830;    

    /* setup the state */
    state->i2c = i2c;
    memcpy(&state->config, config, sizeof(struct rtd2830_config));
    memcpy(&state->ops, &rtd2830_ops, sizeof(struct dvb_frontend_ops));
    memset(&state->fe_param, 0, sizeof(struct dvb_frontend_parameters));

#ifndef USE_USER_MEMORY

    state->len_ts_mmap_region = RTD2830_TS_BUFFER_SIZE;
    state->p_ts_mmap_region= (u8*)__get_free_pages(GFP_ATOMIC, get_order(state->len_ts_mmap_region));
    
    if (!state->p_ts_mmap_region){
        printk("%s: __get_free_pages fail\n", __FUNCTION__);
        kfree(state);
        return NULL;
    }

    rtd2830_mmap_reserved(state->p_ts_mmap_region, state->len_ts_mmap_region);
    
    p_ts_ring_info= RTD2830_GET_RING_INFO(state->p_ts_mmap_region, state->len_ts_mmap_region);
    
    memset(p_ts_ring_info,0,sizeof(*p_ts_ring_info));

    p_ts_ring_info->p_lower      =
    p_ts_ring_info->pp_read[0]   =
    p_ts_ring_info->pp_read[1]   =
    p_ts_ring_info->pp_read[2]   =
    p_ts_ring_info->pp_read[3]   =
    p_ts_ring_info->p_write      = state->p_ts_mmap_region;
    p_ts_ring_info->p_upper      = p_ts_ring_info->p_lower+ RTD2830_TS_STREAM_BUFFER_SIZE;

    p_ts_ring_info->p_b_active[0]=
    p_ts_ring_info->p_b_active[1]=
    p_ts_ring_info->p_b_active[2]=
    p_ts_ring_info->p_b_active[3]= 0;

    //state->p_next_urb_buffer= p_ts_ring_info->p_lower + config->urb_num * config->urb_size;

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
    state->version      = config->version;
    state->output_mode  = config->output_mode;
    state->frontend.demodulator_priv = state;
    state->frontend.tuner_priv = NULL;
    state->update_interval_ms  = DEMOD_UPDATE_INTERVAL;   
    state->next_update_time = jiffies;
    return &state->frontend;

error:
    kfree(state);
    return NULL;
}



/*=======================================================
 * Func : rtd2830_i2c_repeat 
 *
 * Desc : i2c repeat function for rtd2830
 *
 * Parm : fe : handle of dvb_frontend 
 *
 * Retn : N/A
 *=======================================================*/
void rtd2830_i2c_repeat( struct dvb_frontend* fe)
{
    struct rtd2830_state* state= fe->demodulator_priv;    
    ((DEMOD_MODULE*) state->pRT2830)->ForwardTunerI2C((DEMOD_MODULE*) state->pRT2830);
}




///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////




/*=======================================================
 * Func : rtd2830_init 
 *
 * Desc : dvb frontend api - Init function 
 *
 * Parm : fe : handle of dvb_frontend
 *
 * Retn : 0 : success, others : fail
 *=======================================================*/ 
int rtd2830_init(struct dvb_frontend* fe)
{     
    int data;
    struct rtd2830_state*   p_state = fe->demodulator_priv;    
    static const u8         If_Freq[3]={0x30,0,0};  //set IF frequency= 36MHz for Thomson_FE664X    
    p_state->ext_flag = fe->ext_flag;    
    
    if (p_state->version == RTL2831U)
    {            
        printk("##########################3 %s: %d: init once\n", __FUNCTION__, __LINE__);
    
        //============================================
        //get info :HIGH_SPEED or FULL_SPEED
        //============================================
        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_USB_STAT, &data);
        printk("#####################################################TD2831_RMAP_INDEX_USB_STAT=%x\n",data);
        if(data&0x01)
            printk("HIGH SPEED\n");        
        else
            printk("FULL SPEED\n");        

        //GPIO Enable
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_SYS_GPD, 0x0a);	
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_SYS_GPOE, 0x15);

        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_SYS_GPO, &data);
        printk("#####################################################RTD2831_RMAP_INDEX_SYS_GPO=%x\n",data);      

        data |= 0x01;
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_SYS_GPO,data);
    
        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_SYS_GPO, &data);
        printk("#####################################################RTD2831_RMAP_INDEX_SYS_GPO=%x\n",data);            
        data |= 0x04;
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_SYS_GPO,data);
        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_SYS_GPO, &data);
        printk("#####################################################RTD2831_RMAP_INDEX_SYS_GPO=%x\n",data);            
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_SYS_GPO,data);

    
        //========================================
        //[1]open demod pll enbale
        //[2]open demod adc enbale
        //[3]change the add of internal demod module
        //========================================    
        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_SYS_SYS_0, &data);
        printk("#####################################################RTD2831_RMAP_INDEX_SYS_SYS_0=%x\n",data);            
    
        data |= 0xe0;
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_SYS_SYS_0,data);
    
        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_SYS_SYS_0, &data);
        printk("#####################################################RTD2831_RMAP_INDEX_SYS_SYS_0=%x\n",data);            
        //=====================================
        //reset epa fifo
        //=====================================

        data=0x200;
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_CTL,data);

        data=0;
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_CTL,data);

        //============================================
        //[1]FULL packer mode(for bulk)
        //[2]verification mode(will not used in future)
        //[3]DMA enable.
        //============================================
        //	length = 4;
        //	usb_register_read(USB_SYSCTL, data, &length);
        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_USB_SYSCTL, &data);
        printk("#####################################################RTD2831_RMAP_INDEX_USB_SYSCTL=%x\n",data);                  

        data &=0xffffff00;
        // Richard: don't enable DMA at the initialization stage
        //         move it to RTD2830_CMD_TS_START/RTD2830_CMD_TS_STOP        
        data |= 0x08;
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_USB_SYSCTL, data);

        data=0;
        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_USB_SYSCTL, &data);
        printk("#####################################################RTD2831_RMAP_INDEX_USB_SYSCTL=%x\n",data);                        
        
        // Richard: don't enable DMA at the initialization stage
        //         move it to RTD2830_CMD_TS_START/RTD2830_CMD_TS_STOP    
        if((data&0xff)!=0x08)     // Dean add, for extern 
        {
            printk("Open bulk FULL packet mode error!\n");
            return 1;
        }

        //==================================================
        //check epa config,
        //[9-8]:00, 1 transaction per microframe
        //[7]:1, epa enable
        //[6-5]:10, bulk mode
        //[4]:1, device to host
        //[3:0]:0001, endpoint number
        //==================================================
        //bit      9 8 7 6 5 4 3 2 1 0
        //value    0 0 1 1 0 1 0 0 0 1
        //       0x0d1
        //	length =4;
        //	usb_register_read(USB_EPA_CFG, data, &length);
    
        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_CFG, &data);
        printk("#####################################################RTD2831_RMAP_INDEX_USB_EPA_CFG=%x\n",data);                  
        if((data&0x0300)!=0x0000 || (data&0xff)!=0xd1)
        {
            printk("data=0x%x\n" , data);
            printk("Open bulk EPA config error!\n");
            return 1;
        }
    
        data = 0x0200;
        rtd2830_write_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_MAXPKT, data);

        data=0;
        rtd2830_read_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_MAXPKT, &data);
        printk("#####################################################RTD2831_RMAP_INDEX_USB_EPA_MAXPKT=%x\n",data);                  
        if((data&0xffff)!=0x0200)
        {
            printk("Open bulk EPA max packet size error!\n");
          	return 1;      	
        }
    }
    
    // enable spectrum inversion
    if (rtd2830_write_register(p_state, RTD2830_RMAP_INDEX_SPEC_INV, 1)){
        printk("%s: rtd2830_write_register RTD2830_RMAP_INDEX_SPEC_INV fail\n", __FUNCTION__);
        return 1;
    }
    
    if (rtd2830_general_init(p_state)){
        printk("%s: rtd2830_general_init fail\n", __FUNCTION__);
        return 1;        
    }
        
    if(p_state->ext_flag){
        printk("FE664X = 36MHz\n");                        
        if (rtd2830_write_register_table(p_state, RTD2830_RMAP_INDEX_SET_IF_FREQ, If_Freq, 3)){
            printk("%s: RTD2830_RMAP_INDEX_ADD_WRITE fail\n", __FUNCTION__);
            return 1;
        }            
    }                                                        
    rtd2830_pid_filter(fe, NULL, 0xFF);                          

    if (p_state->config.pll_init)
        p_state->config.pll_init(fe, NULL);
    else
        printk("%s: error in NULL p_state->config.pll_init\n", __FUNCTION__);
            
    return 0;
}




/*=======================================================
 * Func : rtd2830_release 
 *
 * Desc : dvb frontend api - release function 
 *
 * Parm : fe : handle of dvb_frontend
 *
 * Retn : N/A
 *=======================================================*/ 
void rtd2830_release(struct dvb_frontend* fe)
{
    int i;	
    struct rtd2830_state* state = fe->demodulator_priv;
    
    state->config.pll_uninit(fe);
          
    printk("%s: Destruct RTL2830 Module\n", __FUNCTION__);
    DestructRtl2830Module((DEMOD_MODULE*)state->pRT2830);               
    
#ifndef USE_USER_MEMORY
    if (state->p_ts_mmap_region){
        printk("============== %s: p_urb_free =============\n", __FUNCTION__);
        state->config.p_urb_free(state->config.dibusb);
        printk("============== %s: free_pages =============\n", __FUNCTION__);
        rtd2830_mmap_unreserved(state->p_ts_mmap_region, state->len_ts_mmap_region);
        free_pages((long)state->p_ts_mmap_region, get_order(state->len_ts_mmap_region));
    }
#else    
    if (state->p_ts_ring_info != NULL) {
        printk("============== %s: set ring_info->p_b_active to 0xFF =============\n", __FUNCTION__);        
        for (i=0; i<4;i++){            
            state->p_ts_ring_info->p_b_active[i] = 0x100;
	}
    }    
#endif // USE_USER_MEMORY
	
    kfree(state);
}




/*=======================================================
 * Func : rtd2830_sleep 
 *
 * Desc : dvb frontend api - sleep function 
 *
 * Parm : fe : handle of dvb_frontend
 *
 * Retn : 0 success
 *=======================================================*/ 
int rtd2830_sleep(struct dvb_frontend* fe)
{
    printk("%s: Do Nothing\n",__FUNCTION__);
    return 0;
}


/*=======================================================
 * Func : rtd2830_set_parameters 
 *
 * Desc : dvb frontend api - set frontend parameter function 
 *
 * Parm : fe : handle of dvb_frontend
 *        param :  frontend parameter
 *
 * Retn : 0 success
 *=======================================================*/ 
int rtd2830_set_parameters(
    struct dvb_frontend*            fe,
    struct dvb_frontend_parameters* param
    )
{
	struct rtd2830_state*           p_state= fe->demodulator_priv;
    struct dvb_ofdm_parameters*     p_ofdm_param = &param->u.ofdm;    
#if 0	
    if (0== memcmp(&p_state->fe_param, param, sizeof(struct dvb_frontend_parameters)))
        return 0;
#else        
    fe_status_t status;
    
    if (p_state->fe_param.frequency == param->frequency &&
        p_state->fe_param.u.ofdm.bandwidth == p_ofdm_param->bandwidth)
    {
        rtd2830_read_status(fe,&status);
        if ((status & FE_HAS_LOCK)){        // signal lock already
            return 0;
        }        
    }        
#endif
           
    memcpy(&p_state->fe_param, param, sizeof(struct dvb_frontend_parameters));
    
    printk("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    printk("%s, freq = %d, bandwidth = %d \n", __FUNCTION__, param->frequency, p_ofdm_param->bandwidth);
    printk("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");        
    
	rtd2830_set_bandwidth(p_state, p_ofdm_param->bandwidth);

    if (p_state->config.pll_set)
        p_state->config.pll_set(fe, param, NULL);
    else
        printk("%s: error in NULL p_state->config.pll_set\n", __FUNCTION__);

    rtd2830_frequency_change_reset(p_state);        
    
    return 0;
}





/*=======================================================
 * Func : rtd2830_get_parameters 
 *
 * Desc : dvb frontend api - get frontend parameter function
 *
 * Parm : fe : handle of dvb_frontend
 *        param :  frontend parameter
 *
 * Retn : 0 success
 *=======================================================*/ 
int rtd2830_get_parameters(
    struct dvb_frontend*            fe,
    struct dvb_frontend_parameters* param
    )
{
    printk("%s: Do Nothing\n",__FUNCTION__);
    return 0;
}




/*=======================================================
 * Func : rtd2830_get_tune_settings 
 *
 * Desc : dvb frontend api - get tune setting function
 *
 * Parm : fe : handle of dvb_frontend
 *        fe_tune_settings :  
 *
 * Retn : 0 success
 *=======================================================*/ 
int rtd2830_get_tune_settings(
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
 * Func : rtd2830_read_status 
 *
 * Desc : dvb frontend api - read frontend status function
 *
 * Parm : fe : handle of dvb_frontend
 *        fe_status_t : frontend status
 *
 * Retn : 0 success
 *=======================================================*/ 
int rtd2830_read_status(
    struct dvb_frontend*    fe, 
    fe_status_t*            status
    )
{    
    int rtd2830_fsm_stage= -1;
    unsigned long BerNum;
    unsigned long BerDen;
    unsigned long SnrNum;
    unsigned long SnrDen;
    struct rtd2830_state* state= fe->demodulator_priv;        
    static char read_counter = 0;
                    
    *status = 0;
    rtd2830_read_register(state, RTD2830_RMAP_INDEX_FSM_STAGE, &rtd2830_fsm_stage);
    
    if (0x0b== rtd2830_fsm_stage)
        *status |= (FE_HAS_CARRIER| FE_HAS_VITERBI| FE_HAS_LOCK| FE_HAS_SYNC| FE_HAS_SIGNAL);   
         
    if (((DEMOD_MODULE*) state->pRT2830)->GetBer((DEMOD_MODULE*) state->pRT2830, & BerNum, &BerDen)==FUNCTION_ERROR)
        printk("Get Ber Fail\n");
    else
    {                 
        if (BerNum >0 || (read_counter++)>=10)
        {
            if (((DEMOD_MODULE*) state->pRT2830)->GetSnr((DEMOD_MODULE*) state->pRT2830, & SnrNum, &SnrDen)==FUNCTION_ERROR){
                printk("Get SNR Fail\n");        
                read_counter = 0;
                return 0;
            }                
            SnrNum = (SnrNum * 10) / SnrDen;            
            printk("\nRTL283x: FSM = %02x, Ber Num = %u, SNR = %u.%u dB \n\n",rtd2830_fsm_stage, (unsigned int) BerNum, (unsigned int)SnrNum/10,(unsigned int)(SnrDen%10));
            read_counter = 0;
        }
    }    

    if (time_after(jiffies, state->next_update_time))        
    {
        rtl2830_UpdateFunction((DEMOD_MODULE*) state->pRT2830);
        state->next_update_time = jiffies + (state->update_interval_ms *HZ)/1000;     // next update interval should more than 100ms
    }

    return 0;
}




/*=======================================================
 * Func : rtd2830_read_ber( 
 *
 * Desc : dvb frontend api - read bit error rate function
 *
 * Parm : fe : handle of dvb_frontend
 *        ber : bit error rate
 *
 * Retn : 0 success
 *=======================================================*/
int rtd2830_read_ber(
    struct dvb_frontend*            fe, 
    u32*                            ber
    )
{
    struct rtd2830_state* state= fe->demodulator_priv;           
    DEMOD_MODULE* hRT2830 = (DEMOD_MODULE*) state->pRT2830;
    unsigned long BerNum;
    unsigned long BerDen;
    unsigned char Lock;        
    
    *ber = 100;   
    if (hRT2830->TestSignalLock(hRT2830, &Lock)==FUNCTION_SUCCESS)
    {
        if ((Lock) && (hRT2830->GetBer(hRT2830, & BerNum, &BerDen)==FUNCTION_SUCCESS)){
            *ber = (u32)((BerNum * 100) / BerDen);            
            if (*ber > 100) *ber = 100;
        }
    }
              
    return 0;
}




/*=======================================================
 * Func : rtd2830_read_signal_strength( 
 *
 * Desc : dvb frontend api - read bit error rate function
 *
 * Parm : fe : handle of dvb_frontend
 *        strength : siganl strength
 *
 * Retn : 0 success
 *=======================================================*/
int rtd2830_read_signal_strength(
    struct dvb_frontend*        fe, 
    u16*                        strength
    )
{
    printk("%s: Do Nothing\n",__FUNCTION__);
    *strength = 0;
    return 0;
}




/*=======================================================
 * Func : rtd2830_read_snr( 
 *
 * Desc : dvb frontend api - read signal noise ratio function
 *
 * Parm : fe : handle of dvb_frontend
 *        snr : siganl noise ratio
 *
 * Retn : 0 success
 *=======================================================*/
int rtd2830_read_snr(
    struct dvb_frontend*        fe, 
    u16*                        snr
    )
{
    struct rtd2830_state* state= fe->demodulator_priv;           
    DEMOD_MODULE* hRT2830 = (DEMOD_MODULE*) state->pRT2830;
    unsigned long SnrNum;
    unsigned long SnrDen;
    unsigned char Lock;        
    
    *snr = 0;   
    if ( hRT2830->TestSignalLock(hRT2830, &Lock)==FUNCTION_SUCCESS)
    {
        if ((Lock) && (hRT2830->GetSnr(hRT2830, & SnrNum, &SnrDen)==FUNCTION_SUCCESS)){
            *snr = (u16)(SnrNum / SnrDen);            
            if (*snr > 40) *snr = 40;
        }
    }
              
    return 0;
}




/*=======================================================
 * Func : rtd2830_read_ucblocks( 
 *
 * Desc : dvb frontend api - ucblocks
 *
 * Parm : fe : handle of dvb_frontend
 *        ucblocks : 
 *
 * Retn : 0 success
 *=======================================================*/
int rtd2830_read_ucblocks(
    struct dvb_frontend*        fe, 
    u32*                        ucblocks
    )
{
    printk("%s: Do Nothing\n",__FUNCTION__);
    return 0;
}




/*=======================================================
 * Func : rtd2830_user_cmd( 
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
int rtd2830_user_cmd(
    struct dvb_frontend*        fe,
    int                         cmd,
    int*                        p_arg_in,
    int                         n_arg_in,
    int*                        p_arg_out,
    int                         n_arg_out
    )
{
    struct rtd2830_state*       state = fe->demodulator_priv;

    switch (cmd)
    {
            
#ifdef USE_USER_MEMORY

    case RTD2830_CMD_PRE_SETUP:
        rtd2830_pre_setup(fe, (unsigned char*)p_arg_in[0], (unsigned char*)p_arg_in[1], (int)p_arg_in[2]);
        return 0;
        
    case RTD2830_CMD_POST_SETUP:
        rtd2830_post_setup(fe);
        return 0;
#else

    case RTD2830_MMAP:
        return rtd2830_mmap(fe, (struct file*)p_arg_in[0], (struct vm_area_struct*)p_arg_in[1]);
        
    case RTD2830_CMD_GET_RING_SIZE:
        p_arg_out[0]= rtd2830_get_size(fe);
        return 0;
#endif

    case RTD2830_CMD_TS_START:
        return rtd2830_ts_set_status(fe, 1);
        
    case RTD2830_CMD_TS_STOP:
        return rtd2830_ts_set_status(fe, 0);
        
    case RTD2830_CMD_TS_IS_START:
        p_arg_out[0] = ((state->flag & RTD2830_STRAMING_ON)) ? 1 : 0;
        return 0;
        
    case RTD2830_CMD_PID_FILTER:
        return rtd2830_pid_filter(fe, p_arg_in, n_arg_in);
        
    case RTD2830_CMD_CHANNEL_SCAN_MODE_EN:      
        rtd2830_channel_scan_mode_en(fe, p_arg_in[0]);                                
        return 0;

    case RTD2830_CMD_URB_IN:
        return rtd2830_copy_from_usb(fe, (void**)p_arg_out[0], (u8*)p_arg_in[0], p_arg_in[1]);
        
    default:
        printk("%s: unknow command\n", __FUNCTION__);
        break;
    }
        
    return 1;
}






//////////////////////////////////////////////////////////
// User Command Functions
//---------------------------------------------------------
// rtd2830_ts_set_status 
// rtd2830_copy_from_usb/ rtd2830_pid_filter/ rtd2830_mmap 
//////////////////////////////////////////////////////////


/*=======================================================
 * Func : rtd2830_ts_set_status 
 *
 * Desc : transport stream control
 *
 * Parm : fe   : handle of dvb frontend
 *        on_off : 0 : stop streaming, 1: start streaming 
 *
 * Retn : N/A
 *=======================================================*/
int rtd2830_ts_set_status(
    struct dvb_frontend*        fe,
    int                         on_off
    )
{
    unsigned long               urb_size;
    unsigned long               urb_num;    
    struct rtd2830_state*       state = fe->demodulator_priv;
    struct rtd2830_ts_buffer_s* p_ts_ring_info = state->p_ts_ring_info;
    
    if ((state->flag & RTD2830_CHANNEL_SCAN_MODE_EN)){
        urb_size = RTD2830_CHANNEL_SCAN_URB_SIZE;    
        urb_num  = RTD2830_CHANNEL_SCAN_URB_NUMBER;    
    }
    else{    
        urb_size = state->config.urb_size;    
        urb_num  = state->config.urb_num;    
    }

    if (( on_off &&  (state->flag & RTD2830_STRAMING_ON)) || 
        (!on_off && !(state->flag & RTD2830_STRAMING_ON))){
        return 0;
    }         
    
    if (0== on_off){        
        
        printk("%s : %d :    Stop Streaming\n", __FILE__,__LINE__);                               
        
        state->config.p_dibusb_streaming(state->config.dibusb, 0);

        state->flag &= ~RTD2830_STRAMING_ON;          // reset streaming on flag           
        
        p_ts_ring_info->pp_read[0] = p_ts_ring_info->p_lower;
        p_ts_ring_info->pp_read[1] = p_ts_ring_info->p_lower;
        p_ts_ring_info->pp_read[2] = p_ts_ring_info->p_lower;
        p_ts_ring_info->pp_read[3] = p_ts_ring_info->p_lower;
        p_ts_ring_info->p_write    = p_ts_ring_info->p_lower;
        /*
        state->p_next_urb_buffer   = p_ts_ring_info->p_lower + (urb_size * urb_num);        
        state->config.p_urb_reset(state->config.dibusb);                
        */
                
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
        
        printk("%s : %d :   ------------------------\n", __FILE__,__LINE__);
        printk("%s : %d :    Start Streaming        \n", __FILE__,__LINE__);
        printk("%s : %d :   ------------------------\n", __FILE__,__LINE__);        
        printk("%s : %d :    urb_size = %d          \n", __FILE__,__LINE__,urb_size);        
        printk("%s : %d :    urb_num = %d           \n", __FILE__,__LINE__,urb_num);        
        printk("%s : %d :    p_next_urb_buffer = %p \n", __FILE__,__LINE__,state->p_next_urb_buffer);          
        printk("%s : %d :   ------------------------\n", __FILE__,__LINE__);
                
        state->flag |= RTD2830_STRAMING_ON;                  
        state->config.p_dibusb_streaming(state->config.dibusb, 1);  // start streaming                        
    }
    
    return 0;
}




/*=======================================================
 * Func : rtd2830_copy_from_usb 
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
int rtd2830_copy_from_usb(
    struct dvb_frontend*        fe,
    void**                      pp_new_urb_buffer,
    u8*                         p_buffer,
    int                         len_buffer)
{
    struct rtd2830_state*       state = fe->demodulator_priv;
    struct rtd2830_ts_buffer_s* p_ts_ring_info = state->p_ts_ring_info;
    unsigned long               urb_size;
#ifdef RTD2830_DEBUG
    int                         writeable_size;    
#endif    
        
    urb_size = ((state->flag & RTD2830_CHANNEL_SCAN_MODE_EN)) ? RTD2830_CHANNEL_SCAN_URB_SIZE 
                                                              : state->config.urb_size;      
    
    //printk("----------------URB %d Complete---------------\n",i);
    
    if(!(state->flag & RTD2830_STRAMING_ON))
    {                        
        printk("%s: %d: skip\n", __FUNCTION__, __LINE__);             
        return 0;
    }

#ifdef RTD2830_DEBUG
  
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
    if(!(state->flag & RTD2830_STRAMING_ON)){
        printk("22222222222222222222222222222222222222\n");
    }
    */

    /*
    if ((p_ts_ring_info->p_write== p_ts_ring_info->p_lower) && (0x47!= *p_buffer)){
        printk("33333333333333333333333333333333333333\n");
    }
    */

    if (urb_size > (writeable_size= rtd2830_get_writeable_size(p_ts_ring_info)))
    {        
        // D7 of active byte was used to notify buffer full status to AP       
        // user must clear it by it's self     
        int i;
        for (i=0; i<4;i++){
            if (p_ts_ring_info->p_b_active[i])
                p_ts_ring_info->p_b_active[i] |= 0x80;                    
        }                       
                
        printk("44444444444444444444444444444444444444 %d [%d %d %d %d]\n", writeable_size,
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

#ifdef RTD2830_DEBUG
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

#ifdef RTD2830_DEBUG
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
 * Func : rtd2830_pid_filter 
 *
 * Desc : set pid filter
 *
 * Parm : fe   : handle of dvb frontend
 *        p_arg_in : PID Value
 *        n_arg_in : number of PIDs
 *
 * Retn : N/A
 *=======================================================*/
int rtd2830_pid_filter(
    struct dvb_frontend*        fe,
    int*                        p_arg_in,
    int                         n_arg_in
    )
{
    struct rtd2830_state*       p_state= fe->demodulator_priv;
    u8                          buf[64]; 

    struct i2c_msg msg = {
        .addr = 0xff, // given a invalid address
        .flags = 0xffff, // user define usb i2c command
        .buf = buf
    };           
    
#ifdef RTD2830_DEBUG
    int                         i;
#endif        
            
    buf[0]= 0x08;              // OPCODE
    buf[1]=(u8)n_arg_in;
    
    if (0xff== buf[1]){
		msg.len= 2;
	}
	else{
        memcpy(&buf[2], (u16*)p_arg_in, n_arg_in*sizeof(u16));
        msg.len= n_arg_in * sizeof(u16)+ 2;
        
#ifdef RTD2830_DEBUG
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
 * Func : rtd2830_mmap 
 *
 * Desc : memory map function for rtd2830
 *
 * Parm : fe   : handle of dvb frontend
 *        file : 
 *        vma  : 
 *
 * Retn : N/A
 *=======================================================*/
int rtd2830_mmap(
    struct dvb_frontend*        fe,
    struct file*                file,
    struct vm_area_struct*      vma)
{
    int ret = 0;
    struct rtd2830_state*       state = fe->demodulator_priv;
        
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
 * Func : rtd2830_get_size 
 *
 * Desc : get size of mmap region 
 *
 * Parm : fe : handle of dvb frontend 
 *
 * Retn : size of mmap region 
 *=======================================================*/
int rtd2830_get_size(
    struct dvb_frontend*        fe
    )
{
    struct rtd2830_state* state = fe->demodulator_priv;
    return state->len_ts_mmap_region;
}


#else




/*=======================================================
 * Func : rtd2830_pre_setup 
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
int rtd2830_pre_setup(
    struct dvb_frontend*        fe,
    unsigned char*              p_phys,
    unsigned char*              p_user_cached,
    int                         size
    )
{
    struct rtd2830_state*       state = fe->demodulator_priv;
    struct rtd2830_ts_buffer_s* p_ts_ring_info= NULL;
    int                         valid_buffer_size = size - sizeof(struct rtd2830_ts_buffer_s) - 188;
    int                         stream_buffer_size = size;

    state->len_ts_mmap_region = size;
    state->p_ts_mmap_region = phys_to_virt((long)p_phys);

    if(!state->p_ts_mmap_region){
        printk("%s: NULL memory specified\n", __FUNCTION__);
        return 0;
    }    

    if((state->flag &RTD2830_CHANNEL_SCAN_MODE_EN)){
        printk("%s : %d : Reset RTL283x to Pass Through Mode \n", __FUNCTION__,__LINE__);
        state->flag &= ~RTD2830_CHANNEL_SCAN_MODE_EN;       // reset to pass through mode
    }

    state->config.urb_size = (valid_buffer_size < (47*512))    ? 188  : 
                             (valid_buffer_size < (47*1024))   ? 512  : 
                             (valid_buffer_size < (94*1024))   ? 1024 : 
                             (valid_buffer_size < (376*1024))  ? 2048 : 
                             (valid_buffer_size < (1504*1024)) ? 4096 : 32768;
                          
    switch (state->config.urb_size){
    case 188:
        stream_buffer_size = ((int)(valid_buffer_size / 188)) *188;
        break;        
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
    
    state->config.p_urb_alloc(state->config.dibusb, p_ts_ring_info->p_lower, state->config.urb_num, state->config.urb_size);    

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
    state->flag |= RTD2830_URB_RDY;
    return 1;
}


/*=======================================================
 * Func : rtd2830_post_setup 
 *
 * Desc : post setup ring buffer
 *
 * Parm : fe : handle of dvb frontend 
 *
 * Retn : 1 : success, 0 : fail
 *=======================================================*/
void rtd2830_post_setup(
    struct dvb_frontend*            fe
    )
{				
    struct rtd2830_state* state = fe->demodulator_priv;    
    
    if (state->p_ts_mmap_region)
    {
        printk("============== %s: p_urb_free =============\n", __FUNCTION__);
        printk("=================== %d %d =================\n", state->config.urb_num, state->config.urb_size);
                
        if((state->flag & RTD2830_STRAMING_ON)){            
            rtd2830_ts_set_status(fe, 0);   // stop streaming            
        }
            
        state->config.p_urb_free(state->config.dibusb);
        printk("============== %s: free_pages =============\n", __FUNCTION__);
        state->p_ts_mmap_region = NULL;
    }
}

#endif




/*=======================================================
 * Func : rtd2830_channel_scan_mode_en
 *
 * Desc : enable channel scan mode
 *
 * Parm : fe : handle of dvb_frontend
 *        on_off : enable / disable
 *
 * Retn : N/A
 *=======================================================*/ 
void rtd2830_channel_scan_mode_en(
    struct dvb_frontend*            fe,
    unsigned char                   on_off
    )
{    
    struct rtd2830_state* state = fe->demodulator_priv;    
    unsigned char ts_on = state->flag & RTD2830_STRAMING_ON;
    unsigned char cs_on = state->flag & RTD2830_CHANNEL_SCAN_MODE_EN;    
    unsigned long urb_size = (on_off) ? 512 : state->config.urb_size;    
    unsigned long urb_num  = (on_off) ? 1   : state->config.urb_num;
                        
    if ((on_off && !cs_on) || (!on_off && cs_on))
    {
#if 1        
        if (on_off)                                  
            printk("%s : %d :    Set RTL283x to Channel Scan Mode\n", __FILE__,__LINE__);
        else
            printk("%s : %d :    Set RTL283x to Pass Through Mode\n", __FILE__,__LINE__);
#endif     

#if 1  
        if (ts_on)            
            rtd2830_ts_set_status(fe, 0);                       // stop streaming         
#endif
                    
        state->config.p_urb_free(state->config.dibusb);         // free urb               
        
        state->flag = (on_off) ? (state->flag |= RTD2830_CHANNEL_SCAN_MODE_EN): 
                                 (state->flag &=~RTD2830_CHANNEL_SCAN_MODE_EN);
                                         
        state->config.p_urb_alloc(state->config.dibusb,         // realloc urb with minimum size
                                  state->p_ts_ring_info->p_lower,
                                  urb_num,                     
                                  urb_size);           
#if 1           
        if (ts_on)
            rtd2830_ts_set_status(fe, 1);                       // restart streaming                   
#endif                    
    }
}        




//////////////////////////////////////////////////////////
// AUX Functions for Memory Map
//---------------------------------------------------------
// rtd2830_mmap_reserved / rtd2830_mmap_unreserved
// rtd2830_get_writeable_size_single / rtd2830_get_writeable_size
//////////////////////////////////////////////////////////



#ifndef USE_USER_MEMORY


/*=======================================================
 * Func : rtd2830_mmap_reserved 
 *
 * Desc : reserve memory for mmap
 *
 * Parm : p_buffer : buffer address
 *        len_buffer : buffer length
 *
 * Retn : N/A
 *=======================================================*/
void rtd2830_mmap_reserved(
    u8*                         p_buffer,
    int                         len_buffer
    )
{
    struct page*                p_page;

    for (p_page= virt_to_page(p_buffer); p_page< virt_to_page(p_buffer+ len_buffer); p_page++)
        SetPageReserved(p_page);
}




/*=======================================================
 * Func : rtd2830_mmap_unreserved 
 *
 * Desc : unreserve memory for mmap
 *
 * Parm : p_buffer : buffer address
 *        len_buffer : buffer length
 *
 * Retn : N/A
 *=======================================================*/
void rtd2830_mmap_unreserved(
    u8*                         p_buffer,
    int                         len_buffer
    )
{
    struct page*                p_page;

    for (p_page= virt_to_page(p_buffer); p_page< virt_to_page(p_buffer+ len_buffer); p_page++)
        ClearPageReserved(p_page);
}

#endif // ifndef USE_USER_MEMORY




#ifdef RTD2830_DEBUG

/*=======================================================
 * Func : rtd2830_get_writeable_size 
 *
 * Desc : compute write able size - for 4 read pointer
 *
 * Parm : p_ts_ring_info : transport stream ring buffer
 *
 * Retn : N/A
 *=======================================================*/
int rtd2830_get_writeable_size(
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
                            rtd2830_get_writeable_size_single(p_ts_ring_info, ring_size, i));
    }
    return writeable_size;
}



/*=======================================================
 * Func : rtd2830_get_writeable_size_single 
 *
 * Desc : compute write able size - for single read pointer
 *
 * Parm : p_ts_ring_info : transport stream ring buffer
 *
 * Retn : N/A
 *=======================================================*/
int rtd2830_get_writeable_size_single(
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

MODULE_DESCRIPTION("Realtek RTD2830 DVB-T Demodulator driver");
MODULE_AUTHOR("Richard Kuo");
MODULE_LICENSE("GPL");
EXPORT_SYMBOL(rtd2830_attach);
EXPORT_SYMBOL(rtd2830_i2c_repeat);
