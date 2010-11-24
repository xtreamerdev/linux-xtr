
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <asm/pgtable.h>
#include <linux/i2c.h>
   
#include "demod_rtd2820.h"
#include "demod_rtd2820_data.h"

#define RTD2820_MSG_PREFIX()                printk("\n%s ",__FUNCTION__)//printk("\n%s:%d:%s : ",__FILE__,__LINE__,__FUNCTION__)

#define RTD2820_DBG_EN
//#define RTD2820_FLOW_DBG_EN               // un mark it if you want to enable flow debug

#ifdef  RTD2820_DBG_EN
  #define RTD2820_DBG(args...)              do { RTD2820_MSG_PREFIX(); printk(" : "args); } while(0)
#else
  #define RTD2820_DBG(args...)        
#endif

#ifdef  RTD2820_COMM_DBG_EN
  #define RTD2820_COMM_DBG(args...)         RTD2820_DBG(args)
#else
  #define RTD2820_COMM_DBG(args...)        
#endif

#ifdef  RTD2820_FLOW_DBG_EN
  #define RTD2820_FLOW_DBG(args...)         RTD2820_DBG(args)
#else
  #define RTD2820_FLOW_DBG(args...)      
#endif                                        

#ifdef  RTD2820_COMM_FLOW_DBG_EN
  #define RTD2820_COMM_FLOW_DBG(args...)    RTD2820_DBG(args)
#else
  #define RTD2820_COMM_FLOW_DBG(args...)    
#endif  

// non mask able message
#define RTD2820_WARNING(args...)            do { RTD2820_MSG_PREFIX(); printk(args); } while(0)


#define RTD2820_MALLOC(x)                   kmalloc(x,GFP_KERNEL)
#define RTD2820_FREE(x)                     kfree(x)
#define RTD2820_MSLEEP(x)                   msleep(x)

//----- Function Prototype  ------//
static int rtd2820_init                (rtd2820_handle_t* hRTD2820);
static int rtd2820_load_firmware       (rtd2820_handle_t* hRTD2820);

static int rtd2820_soft_reset          (rtd2820_handle_t* hRTD2820);

static int rtd2820_agc_reset           (rtd2820_handle_t* hRTD2820);

static int rtd2820_forward_i2c         (rtd2820_handle_t* hRTD2820);

static int rtd2820_read_signal_strength(rtd2820_handle_t* hRTD2820, unsigned short* strength);

static int rtd2820_read_system_version (rtd2820_handle_t* hRTD2820, unsigned char* p_version);

static int rtd2820_read_status         (rtd2820_handle_t* hRTD2820, unsigned char* p_status);

static int rtd2820_read_lock_status    (rtd2820_handle_t* hRTD2820, unsigned char* p_lock);

static int rtd2820_read_ber            (rtd2820_handle_t* hRTD2820, unsigned short* ber);

static int rtd2820_read_snr            (rtd2820_handle_t* hRTD2820, unsigned short* snr);

static int rtd2820_read_carrier_offset (rtd2820_handle_t* hRTD2820, int* p_carrier_offset);

static int rtd2820_read_if_offset      (rtd2820_handle_t* hRTD2820, int* p_if_offset);

static int rtd2820_set_if              (rtd2820_handle_t* hRTD2820, unsigned long if_freq);

static int rtd2820_set_spec_inv        (rtd2820_handle_t* hRTD2820, unsigned char on_off);

static int rtd2820_page_switch         (rtd2820_handle_t* hRTD2820, unsigned char page);

static int rtd2820_ro_strobe           (rtd2820_handle_t* hRTD2820);

static int rtd2820_read_byte           (rtd2820_handle_t*          hRTD2820,
                                        unsigned char              page, 
                                        unsigned char              offset, 
                                        unsigned char*             p_val
                                        );    

static int rtd2820_write_byte          (rtd2820_handle_t*          hRTD2820,
                                        unsigned char              page, 
                                        unsigned char              offset, 
                                        unsigned char              val
                                        );    
    
static int rtd2820_read_multi_byte     (rtd2820_handle_t*          hRTD2820,                                 
                                        unsigned char              page, 
                                        unsigned char              offset,
                                        int                        n_bytes,
                                        int*                       p_val
                                        );
                                
static int rtd2820_write_multi_byte    (rtd2820_handle_t*          hRTD2820,
                                        unsigned char              page, 
                                        unsigned char              offset,
                                        int                        n_bytes,
                                        int                        val
                                        );                                        
                                        
static int rtd2820_read_bits           (rtd2820_handle_t*           hRTD2820,
                                        int                         page,
                                        int                         offset,
                                        int                         bit_low,
                                        int                         bit_high,
                                        int*                        p_val
                                        );
                                        
static int rtd2820_write_bits          (rtd2820_handle_t*           hRTD2820,
                                        int                         page,
                                        int                         offset,
                                        int                         bit_low,
                                        int                         bit_high,
                                        int                         val_write
                                        );                                        
                                        
static int rtd2820_read_register       (rtd2820_handle_t*          hRTD2820,
                                         rtd2820_reg_t              reg,
                                         int*                       p_val
                                        );

static int rtd2820_write_register      (rtd2820_handle_t*          hRTD2820,
                                         rtd2820_reg_t              reg,
                                         int                        val_write
                                        );
                                
static int rtd2820_write_table         (rtd2820_handle_t*           hRTD2820,
                                         unsigned char*              addr, 
                                         unsigned char*              val,
                                         unsigned char               size, 
                                         unsigned char               page
                                        );
                                        
static int rtd2820_channel_scan        (rtd2820_handle_t*           hRTD2820);                                        
                                
                                
/*=======================================================
 * Func : ConstructRTD2820Module 
 *
 * Desc : Construct RTD2820 Module
 *
 * Parm : i2c_addr : i2c Addr
 *        user_private : user private data 
 *
 * Retn : N/A
 *=======================================================*/
rtd2820_handle_t* ConstructRTD2820Module(unsigned char i2c_addr, void* user_private)
{
    rtd2820_handle_t* hRTD2820 = RTD2820_MALLOC(sizeof(rtd2820_handle_t));
    
    RTD2820_FLOW_DBG();
    
    if (hRTD2820 == NULL){        
        RTD2820_WARNING("Allocate RTD2820 Memory Fail\n");
        goto err_status_construct_rtd2820_fail;
    }
        
    hRTD2820->i2c_addr          = i2c_addr;
    hRTD2820->status            = 0x00;    
    hRTD2820->IfFreq            = 6000000;      // default If Freq : 6 MHz
    hRTD2820->user_private      = user_private;
    hRTD2820->init              = rtd2820_init;
    hRTD2820->soft_reset        = rtd2820_soft_reset;
    hRTD2820->agc_reset         = rtd2820_agc_reset;
    hRTD2820->forward_i2c       = rtd2820_forward_i2c;    
    hRTD2820->get_system_version= rtd2820_read_system_version;
    hRTD2820->get_status        = rtd2820_read_status;
    hRTD2820->get_lock_status   = rtd2820_read_lock_status;
    hRTD2820->get_signal_strength = rtd2820_read_signal_strength;
    hRTD2820->get_snr           = rtd2820_read_snr;
    hRTD2820->get_ber           = rtd2820_read_ber;    
    hRTD2820->get_carrier_offset= rtd2820_read_carrier_offset;
    hRTD2820->get_if_offset     = rtd2820_read_if_offset;
    hRTD2820->set_if            = rtd2820_set_if;    
    hRTD2820->set_spec_inv      = rtd2820_set_spec_inv;
    hRTD2820->read_byte         = rtd2820_read_byte;
    hRTD2820->write_byte        = rtd2820_write_byte;
    hRTD2820->read_multi_byte   = rtd2820_read_multi_byte;
    hRTD2820->write_multi_byte  = rtd2820_write_multi_byte;
    hRTD2820->read_bits         = rtd2820_read_bits;
    hRTD2820->write_bits        = rtd2820_write_bits;    
    hRTD2820->read_register     = rtd2820_read_register;
    hRTD2820->write_register    = rtd2820_write_register;
    hRTD2820->write_table       = rtd2820_write_table;    
        
    hRTD2820->page_switch       = rtd2820_page_switch;
    hRTD2820->ro_strobe         = rtd2820_ro_strobe;
    hRTD2820->channel_scan      = rtd2820_channel_scan;
    
    // AUX
    hRTD2820->mse_ber           = 0;
    
    return hRTD2820;
//---        
err_status_construct_rtd2820_fail:
    return NULL;
}


/*=======================================================
 * Func : DestructRTD2820Module 
 *
 * Desc : Destruct RTD2820 Module
 *
 * Parm : pRTD2820 : handle of rtd2820
 *
 * Retn : N/A
 *=======================================================*/
void DestructRTD2820Module(rtd2820_handle_t* hRTD2820)
{    
    RTD2820_FLOW_DBG();
    
    if (hRTD2820 != NULL)
        RTD2820_FREE(hRTD2820);        
}


/*=======================================================
 * Func : rtd2820_general_init 
 *
 * Desc : soft reset rt2820 fsm
 *
 * Parm : hRTD2820 : handle of rtd2820
 *
 * Retn : N/A
 *=======================================================*/
static int rtd2820_init(rtd2820_handle_t* hRTD2820)
{    
    int val;
	if ((hRTD2820->status & RT2820_INIT_RDY))
	    return RT2820_FUNCTION_OK;

    //System Configuration	
#if 0    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x02, 0, 7, 0);                	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x03, 0, 7, 0);            	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x04, 0, 2, 2);            
#else
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x02, 0, 18, 0x020000);
#endif	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x06, 3, 6, 8);            
	
    hRTD2820->read_bits(hRTD2820, RTD2820_PAGE_0, 0x04, 0, 7, &val);            
    
    RTD2820_DBG("1111111111111msg[0]=0x%02x\n",val);

	//Analog Configuration, power down ADC2	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x33, 4, 6, 0);            

	//power saving by power-down 7-bit ADC (RSSI)		
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x35, 2, 2, 1);            	

	////turn off OOB LO	
#if 0    	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x44, 0, 7, 1);            	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x45, 0, 7, 0);            	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x46, 0, 7, 0);            	
#else
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x44, 0, 23,0x000001);            	
#endif	
	
	//MPEG clock Rate programming	
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_29, 0x32, 0, 7,0xDA/*0xFF*/);            	// 25 MHz * 6 / (0x0A + 0x0D + 2) = 6
	
	//CM setting (new)    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_22, 0x40, 0, 7, 0x80);            	

    hRTD2820->read_bits(hRTD2820, RTD2820_PAGE_0, 0x04, 0, 7, &val);                    
    RTD2820_DBG("22222222222msg[0]=0x%02x\n",val);

	////change IF polarity	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x08, 6, 6, 1);
	
	////change RF gain polarity	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x08, 2, 2, 1);
	
	//enable IF AGC (new)	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x08, 7, 7, 1);

	////disable RF AGC	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x08, 3, 3, 0);

	//AAGC source selection (new) 0: ATSC, 1:QAM	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x08, 0, 0, 0);	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x08, 4, 4, 0);

	//AAGC driving current selection (new) 0:8mA, 1:12mA	
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x16, 2, 3, 0);
    
	//OOB AAGC output enable (new)	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x09, 4, 4, 0);	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_0, 0x0A, 4, 4, 0);
	
    hRTD2820->read_bits(hRTD2820, RTD2820_PAGE_0, 0x04, 0, 7, &val);                    
    RTD2820_DBG("22222222222msg[0]=0x%02x\n",val);	

	////set IF/RF AAGC MAX and MIN	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x0A, 0, 7, 128);	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x0B, 0, 7, 127);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x0C, 0, 7, 128);	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x0D, 0, 7, 128);

	////set AAGC target level	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x03, 0, 7, 48);		
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x04, 5, 6, 0);		
	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x45, 0, 7, 67);		
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_22, 0x45, 0, 0, 0);	

	////field sync 1	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x8A, 0, 6, 76);		
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x7C, 0, 7, 36);	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x7D, 0, 4, 2);		

	////field sync 2
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x78, 4, 4, 1);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x78, 5, 5, 1);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x73, 2, 2, 1);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x77, 5, 5, 1);

	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x6e, 0, 7, 16);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x6f, 0, 7, 15);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x70, 0, 7, 1);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x71, 0, 7, 2);

	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x78, 0, 3, 2);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x73, 3, 4, 0);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x75, 0, 7, 49);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x76, 0, 1, 0);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x74, 0, 7, 1);	//(new)
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x73, 5, 7, 2);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x76, 2, 4, 0);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x76, 5, 5, 0);
	
    hRTD2820->read_bits(hRTD2820, RTD2820_PAGE_0, 0x04, 0, 7, &val);                    
    RTD2820_DBG("special_write msg[0]=0x%02x\n",val);

	rtd2820_write_table(hRTD2820, addr_array_1, data_array_1, sizeof(addr_array_1), RTD2820_PAGE_21);	
	rtd2820_write_table(hRTD2820, addr_array_2, data_array_2, sizeof(addr_array_2), RTD2820_PAGE_21);	
	
    hRTD2820->read_bits(hRTD2820, RTD2820_PAGE_0, 0x04, 0, 7, &val);                    
    RTD2820_DBG("msg[0]=0x%02x\n",val);

	//CR parameter	
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x47, 0, 7, 19);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x48, 0, 7, 228);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x49, 0, 7, 252);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x4A, 0, 4, 9);
		    			
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x4B, 0, 7, 143);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x4C, 0, 7, 1);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x4D, 0, 3, 11);	

	hRTD2820->set_if(hRTD2820, hRTD2820->IfFreq);

	////retry timing setting
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xD1, 0, 7, 20);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xD2, 0, 7, 0);	

	////EQ related (new)	
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x7A, 0, 7, 0xBB);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x7B, 0, 3, 0x0A);	
    	
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x81, 0, 7, 0x8D);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x82, 0, 7, 2);	    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x83, 0, 2, 0);	
    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xD5, 0, 7, 0xFE);	
            
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xD6, 0, 1, 3);	    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x83, 4, 4, 0);	

    hRTD2820->read_bits(hRTD2820, RTD2820_PAGE_0, 0x04, 0, 7, &val);                    
    RTD2820_DBG("before load fw msg[0]=0x%02x\n",val);

	//initial the FSM
	rtd2820_load_firmware(hRTD2820);
	
    hRTD2820->read_bits(hRTD2820, RTD2820_PAGE_0, 0x04, 0, 7, &val);                    
    RTD2820_DBG("before load fw msg[0]=0x%02x\n",val);

	//function change_retry_setting
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_20, 0x22, 0, 7, 0);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_20, 0x23, 0, 7, 0);	
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_20, 0x24, 0, 7, 0);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_20, 0x25, 0, 7, 5);		
		
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_20, 0x1C, 0, 7, 0x51);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_20, 0x1D, 3, 5, 7);			

#if (0)	//(new)
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_51, 0x7, 5, 7, 5);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_51, 0x8, 0, 1, 2);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_51, 0x5, 4, 7, 0xe);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_51, 0x6, 0, 1, 0);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_51, 0x6, 2, 7, 0xe);
#endif

	//EQ parameters    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xBC, 0, 0, 1);
        
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xBD, 0, 7, 0);
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xBE, 0, 7, 4);  
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xBF, 0, 7, 0);    	

    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xD3, 0, 7, 248);
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xD4, 0, 7, 255);    	

    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xCD, 0, 7, 246);
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xCE, 0, 7, 207);  
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xCF, 0, 7, 170);    	    
    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xD0, 0, 3, 0);
    	    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xB3, 0, 7, 160);
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xB4, 0, 7, 46);  
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xB5, 0, 7, 108);    	    
            
	
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xB6, 0, 3, 2);  
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_22, 0x01, 2, 3, 0);    	    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x8C, 6, 6, 1);
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x8C, 5, 5, 1);  
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xD7, 0, 2, 0);    	    
			
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0xD8, 0, 7, 16);      
		
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_22, 0x46, 0, 7, 0xFF);    	    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_22, 0x47, 0, 7, 0xFF);

	
    RTD2820_DBG("=====\n");
    hRTD2820->status |= RT2820_INIT_RDY;	    

	return RT2820_FUNCTION_OK;	
	
//---	
//error_status_writereg_fail:    
    //RTD2820_WARNING("Init Fail\n");
    //return RT2820_FUNCTION_FAIL;

}



static int rtd2820_load_firmware(rtd2820_handle_t* hRTD2820)
{
	RTD2820_DBG("%s\n", __FUNCTION__);

    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_0, sizeof(addr_array_3), data_array_3_0[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_1, sizeof(addr_array_3), data_array_3_1[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_2, sizeof(addr_array_3), data_array_3_2[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_3, sizeof(addr_array_3), data_array_3_3[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_4, sizeof(addr_array_3), data_array_3_4[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_5, sizeof(addr_array_3), data_array_3_5[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_6, sizeof(addr_array_3), data_array_3_6[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_7, sizeof(addr_array_3), data_array_3_7[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_8, sizeof(addr_array_3), data_array_3_8[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_9, sizeof(addr_array_3), data_array_3_9[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_10, sizeof(addr_array_3), data_array_3_10[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_11, sizeof(addr_array_3), data_array_3_11[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_12, sizeof(addr_array_3), data_array_3_12[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_13, sizeof(addr_array_3), data_array_3_13[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_14, sizeof(addr_array_3), data_array_3_14[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_15, sizeof(addr_array_3), data_array_3_15[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_16, sizeof(addr_array_3), data_array_3_16[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_17, sizeof(addr_array_3), data_array_3_17[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_18, sizeof(addr_array_3), data_array_3_18[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_19, sizeof(addr_array_3), data_array_3_19[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_20, sizeof(addr_array_3), data_array_3_20[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_21, sizeof(addr_array_3), data_array_3_21[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_22, sizeof(addr_array_3), data_array_3_22[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_23, sizeof(addr_array_3), data_array_3_23[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_24, sizeof(addr_array_3), data_array_3_24[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_25, sizeof(addr_array_3), data_array_3_25[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_26, sizeof(addr_array_3), data_array_3_26[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_27, sizeof(addr_array_3), data_array_3_27[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_28, sizeof(addr_array_3), data_array_3_28[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_29, sizeof(addr_array_3), data_array_3_29[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_30, sizeof(addr_array_3), data_array_3_30[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_31, sizeof(addr_array_3), data_array_3_31[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_32, sizeof(addr_array_3), data_array_3_32[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_33, sizeof(addr_array_3), data_array_3_33[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_34, sizeof(addr_array_3), data_array_3_34[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_35, sizeof(addr_array_3), data_array_3_35[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_36, sizeof(addr_array_3), data_array_3_36[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_37, sizeof(addr_array_3), data_array_3_37[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_38, sizeof(addr_array_3), data_array_3_38[0]);
    rtd2820_write_table(hRTD2820, addr_array_3, data_array_3_39, sizeof(addr_array_3), data_array_3_39[0]);

	return 0;
};


/*=======================================================
 * Func : rtd2820_soft_reset 
 *
 * Desc : soft reset rt2820 fsm
 *
 * Parm : hRTD2820 : handle of rtd2820
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_FUNCTION_FAIL
 *=======================================================*/
static int rtd2820_soft_reset(rtd2820_handle_t* hRTD2820)
{       
    RTD2820_FLOW_DBG();
        
	if (hRTD2820->write_register(hRTD2820, rtd2820_reg[RTD2820_OPT_SOFT_RESET_ATSC],1)!= RT2820_FUNCTION_OK)
	    goto error_status_function_fail;  
	    
	//RTD2820_MSLEEP(10);
	RTD2820_MSLEEP(50);             // increasing soft reset time to 50ms
	
	if (hRTD2820->write_register(hRTD2820, rtd2820_reg[RTD2820_OPT_SOFT_RESET_ATSC],0)!=RT2820_FUNCTION_OK)	    
	    goto error_status_function_fail;  
	
	RTD2820_MSLEEP(50);             // increasing soft reset time to 50ms
	
	return RT2820_FUNCTION_OK;
//---    
error_status_function_fail:

    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL;   	
}






/*=======================================================
 * Func : rtd2820_agc_reset 
 *
 * Desc : reset agc (this function should be invoked before rf freq changed)
 *
 * Parm : hRTD2820 : handle of rtd2820
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_FUNCTION_FAIL
 *=======================================================*/
static int rtd2820_agc_reset(rtd2820_handle_t* hRTD2820)
{           
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x79, 6, 6, 0);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x6D, 6, 7, 3);
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x69, 1, 6, 0);

	//AAGC target value	
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x03, 0, 7, 48);    		
	return RT2820_FUNCTION_OK;
}



/*=======================================================
 * Func : rtd2820_forward_i2c 
 *
 * Desc : i2c repeat on
 *
 * Parm : hRTD2820 : handle of rtd2820
 *
 * Retn : N/A
 *=======================================================*/
static int rtd2820_forward_i2c(rtd2820_handle_t* hRTD2820)
{        
    RTD2820_FLOW_DBG();
    
    if (hRTD2820->write_register(hRTD2820, rtd2820_reg[RTD2820_OPT_I2C_RELAY], 1)!=RT2820_FUNCTION_OK)
        goto error_status_function_fail;  
    
    return RT2820_FUNCTION_OK;    
        
//---    
error_status_function_fail:

    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL;                		    
}



/*=======================================================
 * Func : rtd2820_page_switch 
 *
 * Desc : soft reset rt2820 fsm
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        page : page number
 *
 * Retn : N/A
 *=======================================================*/
static int rtd2820_page_switch(
    rtd2820_handle_t*           hRTD2820,
    unsigned char                          page
    )
{                          
    unsigned char buf[2]= { 0, page };
    struct i2c_msg msg = {
            .addr  = hRTD2820->i2c_addr,
            .flags = 0,
            .buf   = buf,
            .len   = 2         
        };    
    
    RTD2820_COMM_FLOW_DBG();              
        	
    if (1!= i2c_transfer((struct i2c_adapter*)hRTD2820->user_private, &msg, 1)){
        RTD2820_WARNING("write to reg 0x%x failed\n", 0);
        return RT2820_REG_ACCESS_FAIL;
    }                
                              
    return RT2820_FUNCTION_OK;    
}



#if 0
/*=======================================================
 * Func : rtd2820_read_snr 
 *
 * Desc : read snr
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        snr : 
 *
 * Retn : N/A
 *=======================================================*/
static int rtd2820_read_snr(
    rtd2820_handle_t*           hRTD2820,
    unsigned short*             snr
    )
{    
    int i;
	unsigned char lock;
	unsigned long quality;			
	long est_snr = 0;
    long db_map[11][2] = {
        {10, 20230426},
        {12, 12764536},
        {14, 8053877},
        {16, 5081653},
        {18, 3206306},
        {20, 2023043},
        {22, 1276454},
        {24, 805388},
        {26, 508165},
        {28, 320631},
        {30, 202304},
	};
	
    RTD2820_FLOW_DBG();    	    
            
    if (hRTD2820->get_lock_status(hRTD2820,&lock) != RT2820_FUNCTION_OK)
        goto error_status_function_fail;
    
    if (lock)
	{	    	    
        if (hRTD2820->ro_strobe(hRTD2820) != RT2820_FUNCTION_OK)
            goto error_status_function_fail;
        
        RTD2820_MSLEEP(5);
                
	    if (hRTD2820->read_register(hRTD2820, rtd2820_reg[RTD2820_RO_MSE_FIELD], (int*)&quality)!=RT2820_FUNCTION_OK)
	         goto error_status_function_fail;
	    	    
	    est_snr = db_map[10][0];        // maximum    
	    
	    for (i=0; i<11; i++)
	    {
	        if (quality > db_map[i][1])
	        {
	            if (i==0)
	                est_snr = db_map[0][0];
                else 
                {
                    est_snr = db_map[(i-1)][0]+ 
                          (((db_map[(i-1)][1]-quality)*(db_map[i][0]-db_map[(i-1)][0]))/(db_map[(i-1)][1]-db_map[i][1]));                        

                    //RTD2820_DBG2("quality = %u, est_snr=%u\n", quality,est_snr);                
                }	                                         
                break;                   
            }
        }        		
	}
	
	*snr = (unsigned short) est_snr;

    return RT2820_FUNCTION_OK;
    
//---    
error_status_function_fail:
    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL;     
}

#else

// Log approximation with base 10
//
// 1. Use logarithm value table to approximate log10() function.
//
// Note: The Value must be 1 ~ 100000.
//       The unit of Value is 1.
//       The unit of return value is 0.0001.
//
/*=======================================================
 * Func : Log10Apx 
 *
 * Desc : log 10 approcimation
 *
 * Parm : Value 
 *
 * Retn : N/A
 *=======================================================*/
long Log10Apx(
    unsigned long       Value
    )
{
	long Result;

	// Note: The unit of Log10Table elements is 0.0001.
	//       The value of index 0 elements is 0.
	//
	const unsigned int Log10Table[] =
	{
		0,
		0,		3010,	4771,	6021,	6990,	7782,	8451,	9031,	9542,	10000,
		10414,	10792,	11139,	11461,	11761,	12041,	12304,	12553,	12788,	13010,
		13222,	13424,	13617,	13802,	13979,	14150,	14314,	14472,	14624,	14771,
		14914,	15051,	15185,	15315,	15441,	15563,	15682,	15798,	15911,	16021,
		16128,	16232,	16335,	16435,	16532,	16628,	16721,	16812,	16902,	16990,
		17076,	17160,	17243,	17324,	17404,	17482,	17559,	17634,	17709,	17782,
		17853,	17924,	17993,	18062,	18129,	18195,	18261,	18325,	18388,	18451,
		18513,	18573,	18633,	18692,	18751,	18808,	18865,	18921,	18976,	19031,
		19085,	19138,	19191,	19243,	19294,	19345,	19395,	19445,	19494,	19542,
		19590,	19638,	19685,	19731,	19777,	19823,	19868,	19912,	19956,	20000,
	};



	// Use logarithm value table to approximate log10() function.
	//
	if(Value < 100)
		Result = Log10Table[Value];
	else if(Value < 1000)
		Result = Log10Table[Value / 10]   + 10000;
	else if(Value < 10000)
		Result = Log10Table[Value / 100]  + 20000;
	else
		Result = Log10Table[Value / 1000] + 30000;

	return Result;
}


/*=======================================================
 * Func : rtd2820_read_snr 
 *
 * Desc : read snr
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        snr : 
 *
 * Retn : N/A
 *=======================================================*/
static int rtd2820_read_snr(
    rtd2820_handle_t*           hRTD2820,
    unsigned short*             snr
    )
{
    unsigned char i;
    unsigned char buf[4];
    unsigned long Mse; 
    unsigned long snr_e, snr0, snr1;
        
    // set strobe
    if (hRTD2820->ro_strobe(hRTD2820) != RT2820_FUNCTION_OK)
        goto error_status_function_fail;
    
    // Read MSE
    for (i=0; i<4 ; i++)
        hRTD2820->read_byte(hRTD2820,RTD2820_PAGE_2F, i+0x2C, &buf[i]);    
                    
    Mse =(((unsigned long)buf[3]<<24)+((unsigned long)buf[2]<<16)+((unsigned long)buf[1]<<8)+(unsigned long)buf[0])>>16;
    Mse = Mse + (Mse>>1); 
    
    if (Mse==0)
        snr_e = 65535;    
    else{
        snr0 = (37305-Log10Apx(Mse))/1000;//>>10;
        snr_e = snr0;
    }
    
    for (i=0; i<4 ; i++)
        hRTD2820->read_byte(hRTD2820,RTD2820_PAGE_2F, i+0x02, &buf[i]);        
    
    Mse=(((unsigned long)buf[3]<<24)+((unsigned long)buf[2]<<16)+((unsigned long)buf[1]<<8)+(unsigned long)buf[0])>>16;
    
    if (Mse==0)
        snr_e = 99999999;    
    else{
        snr1=(37305-Log10Apx(Mse))/1000;
        snr_e = snr1 * 100 + snr_e;
    }
    *snr = (unsigned short) (snr_e/100);
    return RT2820_FUNCTION_OK;
//---    
error_status_function_fail:
    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL;     
}
#endif




/*=======================================================
 * Func : rtd2820_read_ber 
 *
 * Desc : read ber
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        ber : 
 *
 * Retn : N/A
 *=======================================================*/
static int rtd2820_read_ber(
    rtd2820_handle_t*           hRTD2820,
    unsigned short*             ber
    )
{                
    int i;
    unsigned long test_volume;
    unsigned long ber_cnt,pkt_cnt,corr_cnt,uncorr_cnt;
    unsigned char SerCnt[10];
    unsigned char tmp;
    
    RTD2820_FLOW_DBG();
    
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    hRTD2820->read_byte(hRTD2820, RTD2820_PAGE_29, 0x03, &tmp);
    
    if ((tmp & 0x01)) 
    {
        // ber tester does not enabled
        // start
        test_volume = 5;
        //% reset BER tester    
        hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_29, 0x03, 0, 5, (test_volume<<3)+0*(1<<2)+0*(1<<1)+1);
    
        //% enable BER tester and release the BER counter    
        hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_29, 0x03, 0, 2, 0);    
        RTD2820_MSLEEP(10);          
    }    
      
    // hold ber counters    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_29, 0x03, 1, 1, 1);    
    
    // read counters    
    hRTD2820->ro_strobe(hRTD2820);    
    
    for (i=0; i<10;i++){
        hRTD2820->read_byte(hRTD2820, RTD2820_PAGE_29, i+4, &SerCnt[i]);            
    }    
    
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_29, 0x03, 1, 1, 0); // release ber count
    
    ber_cnt=(((unsigned long)SerCnt[2]<<16)+((unsigned long)SerCnt[1]<<8)+(unsigned long)SerCnt[0]);
    //RTD2820_DBG("ber_cnt =%u\n",(unsigned long)ber_cnt);
    
    pkt_cnt=(((unsigned long)SerCnt[5]<<16)+((unsigned long)SerCnt[4]<<8)+(unsigned long)SerCnt[3]);    
    //RTD2820_DBG("pkt_cnt =%u\n",(unsigned long)pkt_cnt);
    
    corr_cnt=(((unsigned long)SerCnt[7]<<8)+(unsigned long)SerCnt[6]);
    //RTD2820_DBG("corr_cnt =%u\n",(unsigned long)corr_cnt);    
    
    uncorr_cnt=(((unsigned long)SerCnt[9]<<8)+(unsigned long)SerCnt[8]);    
    //RTD2820_DBG("uncorr_cnt =%u\n", (unsigned long)uncorr_cnt);
        
    if (pkt_cnt >= 16384)
    {
        // update ber count
        hRTD2820->mse_ber = (unsigned short) ((ber_cnt * 100) /pkt_cnt); 
        
        RTD2820_DBG("ber_cnt =%u\n",(unsigned long)ber_cnt);
        RTD2820_DBG("uncorr_cnt =%u\n", (unsigned long)uncorr_cnt);      
               
        test_volume = 5;
        //% reset BER tester    
        hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_29, 0x03, 0, 5, (test_volume<<3)+0*(1<<2)+0*(1<<1)+1);
    }
        
    //BER : uncorr_cnt / pkt_cnt    
    *ber = hRTD2820->mse_ber;    
    return RT2820_FUNCTION_OK;        
}




/*=======================================================
 * Func : rtd2820_read_carrier_offset 
 *
 * Desc : read ber
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        p_carrier_offset : carrier offset
 *
 * Retn : RT2820_FUNCTION_OK/RT2820_FUNCTION_FAIL
 *=======================================================*/
static int rtd2820_read_carrier_offset(
    rtd2820_handle_t*           hRTD2820, 
    int*                        p_carrier_offset
    )
{
    int ret;
    
    RTD2820_FLOW_DBG();    
    
    ret = hRTD2820->ro_strobe(hRTD2820);
    if (ret != RT2820_FUNCTION_OK)
        goto error_status_function_fail;
        
    RTD2820_MSLEEP(5);
        
    ret = hRTD2820->read_register(
            hRTD2820,
            rtd2820_reg[RTD2820_RO_CARRIER_OFFSET_D_Q],
            (int*)p_carrier_offset
            );            
				            
    if (ret != RT2820_FUNCTION_OK)
        goto error_status_function_fail;   
        
    if (*p_carrier_offset >= (1<<15))
        *p_carrier_offset -= (1<<16);        
    
    return RT2820_FUNCTION_OK;
    
//---    
error_status_function_fail:

    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL;    
}




/*=======================================================
 * Func : rtd2820_read_if_offset 
 *
 * Desc : read ber
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        p_if_offset : IF offset
 *
 * Retn : RT2820_FUNCTION_OK/RT2820_FUNCTION_FAIL
 *=======================================================*/
static int rtd2820_read_if_offset(
    rtd2820_handle_t*           hRTD2820, 
    int*                        p_if_offset
    )
{
    int ret;
    
    RTD2820_FLOW_DBG();    
    
    ret = hRTD2820->ro_strobe(hRTD2820);
    if (ret != RT2820_FUNCTION_OK)
        goto error_status_function_fail;
        
    RTD2820_MSLEEP(5);
        
    ret = hRTD2820->read_register(
            hRTD2820,
            rtd2820_reg[RTD2820_RO_IF_OFFSET],
            (int*)p_if_offset
            );            	
				            
    if (ret != RT2820_FUNCTION_OK)
        goto error_status_function_fail;   

    if (*p_if_offset >= (1<<8))
        *p_if_offset -= (1<<9);        
    
    return RT2820_FUNCTION_OK;
    
//---    
error_status_function_fail:

    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL;    
}





/*=======================================================
 * Func : rtd2820_set_if 
 *
 * Desc : set if freq
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        if_freq  : IF frequency (Hz)
 *
 * Retn : N/A
 *=======================================================*/
static int rtd2820_set_if(
    rtd2820_handle_t*           hRTD2820,
    unsigned long               if_freq
    )
{        
    unsigned long ddc_phase_inc;

	switch(if_freq){
	default:
	case 6000000:  ddc_phase_inc = 0x3d73d; break;
	case 5950000:  ddc_phase_inc = 0x3cf0b; break;
	case 5900000:  ddc_phase_inc = 0x3c6d9; break;
	case 6050000:  ddc_phase_inc = 0x3df6e; break;
	case 5850000:  ddc_phase_inc = 0x3bea8; break;
	case 5800000:  ddc_phase_inc = 0x3b676; break;
	case 5750000:  ddc_phase_inc = 0x3ae45; break;
    }    	

    RTD2820_DBG("ddc_phase_inc is %02x-%02x-%02x\n",
            (unsigned char)(ddc_phase_inc & 0xFF),
            (unsigned char)((ddc_phase_inc & 0xFF00)>>8),
            (unsigned char)((ddc_phase_inc & 0x70000) >> 16));	
            
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x0F, 0, 7, (ddc_phase_inc)& 0xFF);        
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x10, 0, 7, ((ddc_phase_inc)& 0xFF00)>>8);        
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x11, 0, 2, (ddc_phase_inc & 0x70000) >> 16);    
	
	//CR: cr_const_inc
	hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x53, 0, 7, 228);        
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x54, 0, 4, 6);   
    
	//use fixed tr_phase_inc on 20070321
	//TR: tr_phase_inc	
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_22, 0x41, 0, 7, 0xA9);        
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_22, 0x42, 0, 7, 0x8D);        
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_22, 0x43, 0, 7, 0x52);
    hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_22, 0x44, 0, 2, 2);
	
	hRTD2820->IfFreq = if_freq;
	
	return RT2820_FUNCTION_OK;
}



/*=======================================================
 * Func : rtd2820_set_spec_inv 
 *
 * Desc : set spectrum inverse
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        on_off   : 0 : no inverse , others : inverse
 *
 * Retn : N/A
 *=======================================================*/
static int rtd2820_set_spec_inv(
    rtd2820_handle_t*           hRTD2820,
    unsigned char               on_off
    )
{    
    RTD2820_FLOW_DBG();
    
    if (( on_off && !(hRTD2820->status & RT2820_SPEC_INV)) || 
        (!on_off &&  (hRTD2820->status & RT2820_SPEC_INV)))
    {        
    
        if (hRTD2820->write_register(hRTD2820, rtd2820_reg[RTD2820_SPEC_INV_ON], (on_off)? 1 : 0) != RT2820_FUNCTION_OK)
            goto error_status_function_fail;
        
        if (on_off)
            hRTD2820->status |= RT2820_SPEC_INV;
        else        
            hRTD2820->status &= ~RT2820_SPEC_INV;
    }
    
    return RT2820_FUNCTION_OK;
    
//---
error_status_function_fail:
    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL;    
}


/*=======================================================
 * Func : rtd2820_read_signal_strength 
 *
 * Desc : read signal strength
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        snr : 
 *
 * Retn : N/A
 *=======================================================*/
static int rtd2820_read_signal_strength(
    rtd2820_handle_t*           hRTD2820,
    unsigned short*                        strength
    )
{
    unsigned char lock;
    unsigned int AagcLvlMax = 12000;
    unsigned int AagcLvlMin = 4300;
    unsigned int level;
    rtd2820_reg_t strength_reg = {RTD2820_PAGE_2F, 0x30, 0, 13};
    
    RTD2820_FLOW_DBG();
    	    	
    if (hRTD2820->get_lock_status(hRTD2820,&lock)!= RT2820_FUNCTION_OK)
        goto error_status_function_fail;

    if (lock)
    {
        if (hRTD2820->ro_strobe(hRTD2820)!= RT2820_FUNCTION_OK)
            goto error_status_function_fail;
            
        RTD2820_MSLEEP(5);
        
        if (hRTD2820->read_register(hRTD2820, strength_reg, &level)!= RT2820_FUNCTION_OK)
            goto error_status_function_fail;
                     
        level = level - (level>=8192)*16384 + 8192;

        if (level >= AagcLvlMax)
            *strength = 10;
        else if (level <= AagcLvlMin)
            *strength = 100;
        else
            *strength = 100 - (level-AagcLvlMin)*90/(AagcLvlMax-AagcLvlMin);
    }
    else    
        *strength = 0;    

    return RT2820_FUNCTION_OK;
    
//---
error_status_function_fail:
    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL;
}



/*=======================================================
 * Func : rtd2820_ro_strobe 
 *
 * Desc : strobe read only register
 *
 * Parm : hRTD2820 : handle of rtd2820
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_FUNCTION_FAIL
 *=======================================================*/
static int rtd2820_ro_strobe(
    rtd2820_handle_t*           hRTD2820 
    )
{            
    RTD2820_FLOW_DBG();
    
    if (hRTD2820->write_register(hRTD2820, rtd2820_reg[RTD2820_RO_STROBE], 1)!= RT2820_FUNCTION_OK)
        goto error_status_function_fail;
            
	if (hRTD2820->write_register(hRTD2820, rtd2820_reg[RTD2820_RO_STROBE], 0)!= RT2820_FUNCTION_OK)
	    goto error_status_function_fail;
	    
    return RT2820_FUNCTION_OK;
    
//---
error_status_function_fail:
    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL;    
}	





/*=======================================================
 * Func : rtd2820_read_system_version 
 *
 * Desc : read system version number
 *
 * Parm : hRTD2820  : handle of rtd2820
 *        p_version : rtd2820 version
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_FUNCTION_FAIL
 *=======================================================*/
static int rtd2820_read_system_version(
    rtd2820_handle_t*           hRTD2820,
    unsigned char*              p_version
    )
{
    unsigned int version;    
    
    RTD2820_FLOW_DBG();
	
    if (hRTD2820->ro_strobe(hRTD2820)!=RT2820_FUNCTION_OK)        
        goto error_status_function_fail;    
        
	RTD2820_MSLEEP(5);
	
    if (hRTD2820->read_register(hRTD2820, rtd2820_reg[RTD2820_SYSTEM_VERSION], &version)!=RT2820_FUNCTION_OK)
        goto error_status_function_fail;
        
    *p_version = (unsigned char) version;            
                
	return RT2820_FUNCTION_OK;
	
//---
error_status_function_fail:
    RTD2820_WARNING("function fail\n");    
    return RT2820_FUNCTION_FAIL;   	
}




/*=======================================================
 * Func : rtd2820_read_status 
 *
 * Desc : read status
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        p_status : rtd2820 status
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_FUNCTION_FAIL
 *=======================================================*/
static int rtd2820_read_status(
    rtd2820_handle_t*           hRTD2820,
    unsigned char*              p_status
    )
{
    unsigned int  status;
    rtd2820_reg_t status_reg = {RTD2820_PAGE_2F, 0x09, 2, 7};
    
    RTD2820_FLOW_DBG();
	
    if (hRTD2820->ro_strobe(hRTD2820)!=RT2820_FUNCTION_OK)
        goto error_status_function_fail;
        
	RTD2820_MSLEEP(5);
	
    if (hRTD2820->read_register(hRTD2820, status_reg, &status)!=RT2820_FUNCTION_OK)
        goto error_status_function_fail;
        
    *p_status = (unsigned char) status;            
                
	return RT2820_FUNCTION_OK;
	
//---
error_status_function_fail:
    RTD2820_WARNING("function fail\n"); 
    return RT2820_FUNCTION_FAIL;   	
}



/*=======================================================
 * Func : rtd2820_read_lock_status 
 *
 * Desc : read lock status strength
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        p_lock  : lock status
 *
 * Retn : 0 success
 *=======================================================*/
static int rtd2820_read_lock_status(
    rtd2820_handle_t*           hRTD2820,
    unsigned char*              p_lock
    )
{   
    unsigned int lock;     
    rtd2820_reg_t lock_reg = {RTD2820_PAGE_2F, 0x09, 1, 1};
    
    RTD2820_FLOW_DBG();
	
    if (hRTD2820->ro_strobe(hRTD2820)!=RT2820_FUNCTION_OK)
        goto error_status_function_fail;
        
	RTD2820_MSLEEP(5);
	
    if (hRTD2820->read_register(hRTD2820, lock_reg, &lock))    
        goto error_status_function_fail; 

    *p_lock = (unsigned int) lock;
    
	return RT2820_FUNCTION_OK;
	
//---
error_status_function_fail:
    RTD2820_WARNING("function fail\n");
    return RT2820_FUNCTION_FAIL; 	
}


/*=======================================================
 * Func : rtd2820_channel_scan 
 *
 * Desc : do channel scan to lock siganl
 *
 * Parm : hRTD2820 : handle of rtd2820 
 *
 * Retn : 0 success
 *=======================================================*/
static int rtd2820_channel_scan(
    rtd2820_handle_t* hRTD2820
    )
{
    int             i;
    unsigned char   status;
	int             CrOffset;
	int             IfOffset;
	long            TotalCrOffset = 0;
	long            cr_sweep_now  = 0 ;
	long            cr_offset_sweep[7]={-50,-100,50,-150,-200,-250,0};	
	
    for (i=0;i<2;i++)
	{  
	    hRTD2820->get_status(hRTD2820, &status);		

		if (status == 32 || status == 16)
		{
		    hRTD2820->get_carrier_offset(hRTD2820, &CrOffset);    					
            			
            hRTD2820->get_if_offset(hRTD2820, &IfOffset);		

            //---
			if ((hRTD2820->status & RT2820_SPEC_INV))
				TotalCrOffset = 488*(CrOffset + IfOffset*(1<<7))/(1<<16) - cr_sweep_now;
			else
            	TotalCrOffset = 488*(CrOffset + IfOffset*(1<<7))/(1<<16) + cr_sweep_now;
	
			RTD2820_DBG("IfOffset CrOffset is %ld-%ld-%ld....\n",(long int)IfOffset, (long int)CrOffset,(long int) TotalCrOffset);	
	
			hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x69, 1, 6, 0x3f);

			//Set the channel_scan and tr_lock_th1 to enable normal receiving
			//after this setting, keep going to final state, then TS start sending
			hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x79, 6, 6, 1);

			break;
		}
		else
		{
			//shift the IF frequency to get locked	        
	        hRTD2820->set_if(hRTD2820, hRTD2820->IfFreq + (cr_offset_sweep[i]*1000));
	        
			cr_sweep_now = cr_offset_sweep[i];					
			hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x79, 6, 6, 1);					
            RTD2820_MSLEEP(100);				// add delay
			hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x79, 6, 6, 0);
			RTD2820_MSLEEP(50);				// add delay

			//allow play TV without scanning again on 20070319
			if (cr_offset_sweep[i] == 0)
			{
				hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x69, 1, 6, 0x3f);				
				hRTD2820->write_bits(hRTD2820, RTD2820_PAGE_21, 0x79, 6, 6, 1);
			}
		}
    }
    
    return RT2820_FUNCTION_OK;
}



////////////////////////////////////////////////////////////////////
// Communication Functions
//------------------------------------------------------------------
// rtd2820_read_byte / rtd2820_write_byte
// rtd2820_read_multi_byte / rtd2820_write_multi_byte
// rtd2820_read_register / rtd2820_write_register / rtd2820_write_table
////////////////////////////////////////////////////////////////////

/*=======================================================
 * Func : rtd2820_read_byte 
 *
 * Desc : write single byte
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        page_addr : page
 *        byte_addr : byte address
 *        p_val     : byte value
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_REG_ACCESS_FAIL
 *=======================================================*/
static int rtd2820_read_byte(
    rtd2820_handle_t*           hRTD2820,    
    unsigned char               page,    
    unsigned char               offset,
    unsigned char*              p_val
    )
{              
    unsigned char buf[2] = { offset, 0 };    
    struct i2c_msg msg[2]= {
        { 
            .addr  = hRTD2820->i2c_addr,
            .flags = 0,
            .buf   = &buf[0],
            .len   = 1 
        },
        { 
            .addr  = hRTD2820->i2c_addr,
            .flags = I2C_M_RD,
            .buf   = &buf[1],
            .len   = 1 
        }
    };                                                  
    
    RTD2820_COMM_FLOW_DBG();
    
    hRTD2820->page_switch(hRTD2820,page);
        
    if (2!= i2c_transfer((struct i2c_adapter*)hRTD2820->user_private, msg, 2)){
        RTD2820_WARNING("readreg error reg=0x%x\n", offset);
        return RT2820_REG_ACCESS_FAIL;
    }

    *p_val= buf[1];
        
    return RT2820_FUNCTION_OK;    
}



/*=======================================================
 * Func : rtd2820_write_byte 
 *
 * Desc : write single byte
 *
 * Parm : hRTD2820  : handle of rtd2820
 *        page_addr : page
 *        offset    : offset
 *        val       : write value
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_REG_ACCESS_FAIL
 *=======================================================*/
static int rtd2820_write_byte(
    rtd2820_handle_t*           hRTD2820,   
    unsigned char               page,     
    unsigned char               offset, 
    unsigned char               val
    )
{            
    unsigned char buf[2]= { offset, val };
    struct i2c_msg msg = {        
            .addr  = hRTD2820->i2c_addr,
            .flags = 0,
            .buf   = buf,
            .len   = 2         
    };    
        
    RTD2820_COMM_FLOW_DBG();            
    
    hRTD2820->page_switch(hRTD2820,page);
        		        
    if (1!= i2c_transfer((struct i2c_adapter*)hRTD2820->user_private, &msg, 1)){
        RTD2820_WARNING("write to reg 0x%x failed\n", offset);
        return RT2820_REG_ACCESS_FAIL;
    }            
                                  
    return RT2820_FUNCTION_OK;
}




/*=======================================================
 * Func : rtd2820_read_multi_byte 
 *
 * Desc : read multiple bytes
 *
 * Parm : hRTD2820  : handle of rtd2820
 *        offset    : offset address
 *        n_bytes   : number of bytes
 *        p_val     : value
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_REG_ACCESS_FAIL
 *=======================================================*/
static int rtd2820_read_multi_byte(
    rtd2820_handle_t*           hRTD2820,    
    unsigned char               page,    
    unsigned char               offset,
    int                         n_bytes,
    int*                        p_val
    )
{
    int i;
    int ret = RT2820_FUNCTION_OK;
    unsigned char val;        
    
    RTD2820_COMM_FLOW_DBG();
    
    *p_val= 0;
    
    for (i= 0; i< n_bytes; i++)
    {
        val= 0;                
        ret = hRTD2820->read_byte(hRTD2820, page, offset+ i, &val);
          
        if (ret != RT2820_FUNCTION_OK){
            RTD2820_WARNING("error in reading (byte_addr)= (0x%x) , (%d)\n", offset+ i,ret);
            break;
        }
                       
        *p_val += ((int) val) << (8*i);        
    }
    
    return ret;
}



/*=======================================================
 * Func : rtd2820_write_multi_byte 
 *
 * Desc : write multiple bytes
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        page_addr : page
 *        offset    : register offset
 *        n_bytes   : length
 *        val       : value
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_REG_ACCESS_FAIL
 *=======================================================*/
static int rtd2820_write_multi_byte(
    rtd2820_handle_t*           hRTD2820,    
    unsigned char               page,
    unsigned char               offset,
    int                         n_bytes,
    int                         val
    )
{
    int i; 
    int ret = RT2820_FUNCTION_OK;
    int nbit_shift;
    
    RTD2820_COMM_FLOW_DBG();  
         
    for (i= 0; i< n_bytes; i++)
    {
        nbit_shift = i<<3;
        ret = hRTD2820->write_byte(hRTD2820, page, offset+ i, (val>> nbit_shift) & 0xff);
        
        if (ret!=RT2820_FUNCTION_OK){    
            RTD2820_WARNING("error in write (byte_addr)= (0x%x) , (%d)\n", offset+ i,ret);         
            break;        
        }
    }           
    
    return ret;
}






/*=======================================================
 * Func : rtd2820_read_bits 
 *
 * Desc : read rtd2820 bits
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        page      : page
 *        offset    : register offset
 *        bit_low   : low bit
 *        bit_high  : high bit
 *        p_val     : read data buffer
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_REG_ACCESS_FAIL
 *=======================================================*/
static int rtd2820_read_bits(
    rtd2820_handle_t*           hRTD2820,
    int                         page,
    int                         offset,
    int                         bit_low,
    int                         bit_high,
    int*                        p_val
    )
{
    int ret;     
    int n_byte_read = (bit_high>> 3)+ 1;
    
    RTD2820_COMM_FLOW_DBG();

    *p_val= 0;
         
    ret = hRTD2820->read_multi_byte(hRTD2820, page, offset, n_byte_read, p_val);
    if (ret != RT2820_FUNCTION_OK)
        return ret;                                
    
    *p_val= ((*p_val>> bit_low) & rtd2820_reg_mask[bit_high- bit_low]);
    
    RTD2820_COMM_DBG("reg.addr = %d, length = %d, val=%08x \n",addr, n_byte_read, *p_val);            

    return ret;
}


/*=======================================================
 * Func : rtd2820_write_bits 
 *
 * Desc : write bits
 *
 * Parm : hRTD2820  : handle of rtd2820
 *        page      : page
 *        offset    : register offset
 *        bit_low   : low bit
 *        bit_high  : high bit
 *        val_write : register value
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_REG_ACCESS_FAIL
 *=======================================================*/
static int rtd2820_write_bits(
    rtd2820_handle_t*           hRTD2820,
    int                         page,
    int                         offset,
    int                         bit_low,
    int                         bit_high,
    int                         val_write
    )
{
    int ret;
    int n_byte_write = (bit_high>> 3)+ 1;
    int val_read = 0;
    int new_val_write;    
    int mask ;
    
    RTD2820_COMM_FLOW_DBG();
    
    ret = hRTD2820->read_multi_byte(hRTD2820, page, offset, n_byte_write, &val_read);
    
    if (ret != RT2820_FUNCTION_OK){
        RTD2820_WARNING("error in read (%d)\n",ret);
        return ret;
    }
    
    mask = (rtd2820_reg_mask[bit_high- bit_low]<< bit_low);  
    
    new_val_write= (val_read & ~mask) | ((val_write<< bit_low) & mask);

    ret = hRTD2820->write_multi_byte(hRTD2820, page, offset, n_byte_write, new_val_write);   
    
    if (ret != RT2820_FUNCTION_OK)
        RTD2820_WARNING("error in write (%d)\n",ret);            
    
    return ret;
}



/*=======================================================
 * Func : rtd2820_read_register 
 *
 * Desc : read rtd2820 register
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        reg      : rtd2820 reg 
 *        p_val    : register value
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_REG_ACCESS_FAIL
 *=======================================================*/
static int rtd2820_read_register(
    rtd2820_handle_t*           hRTD2820,
    rtd2820_reg_t               reg,
    int*                        p_val
    )
{      
    return hRTD2820->read_bits(
                hRTD2820,reg.page,
                reg.addr,
                reg.bit_low,
                reg.bit_high,
                p_val
                );    
}



/*=======================================================
 * Func : rtd2820_write_register 
 *
 * Desc : write rtd2820 register
 *
 * Parm : hRTD2820  : handle of rtd2820
 *        reg       : rtd2820 reg 
 *        val_write : register value
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_REG_ACCESS_FAIL
 *=======================================================*/
static int rtd2820_write_register(
    rtd2820_handle_t*       hRTD2820,
    rtd2820_reg_t           reg,
    int                     val_write
    )
{    
    return hRTD2820->write_bits(
                hRTD2820, 
                reg.page, 
                reg.addr,
                reg.bit_low, 
                reg.bit_high, 
                val_write
                );
}



/*=======================================================
 * Func : rtd2820_write_table 
 *
 * Desc : write rtd2820 register
 *
 * Parm : hRTD2820 : handle of rtd2820
 *        addr    : address array
 *        val     : value array
 *        size    : length of array
 *        page    : page
 *
 * Retn : RT2820_FUNCTION_OK / RT2820_REG_ACCESS_FAIL
 *=======================================================*/
static int rtd2820_write_table(
    rtd2820_handle_t*           hRTD2820,
    unsigned char*              addr, 
    unsigned char*              val,
    unsigned char               size, 
    unsigned char               page
    )
{	
	int i = 0;
	int start = 0;

    RTD2820_COMM_FLOW_DBG();
        
	//the first one is not for page setting	
    start = (addr[0]!= 0x00) ? 0 : 1;				
	
	for (i=start; i<size; i++)
	{                        
		if (hRTD2820->write_byte(hRTD2820,page,(unsigned int) addr[i], val[i])!= RT2820_FUNCTION_OK){		
		    RTD2820_WARNING("error in write \n"); 		    
		    return RT2820_REG_ACCESS_FAIL;
        }
	}

	return RT2820_FUNCTION_OK;
}


