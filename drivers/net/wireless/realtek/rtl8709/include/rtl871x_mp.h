#ifndef __RTL871X_MP_H_
#define __RTL871X_MP_H_

#define MPT_NOOP				0
#define MPT_READ_MAC_1BYTE		1
#define MPT_READ_MAC_2BYTE		2
#define MPT_READ_MAC_4BYTE		3
#define MPT_WRITE_MAC_1BYTE		4
#define MPT_WRITE_MAC_2BYTE		5
#define MPT_WRITE_MAC_4BYTE		6
#define MPT_READ_BB_CCK			7
#define MPT_WRITE_BB_CCK		8
#define MPT_READ_BB_OFDM		9
#define MPT_WRITE_BB_OFDM		10
#define MPT_READ_RF				11
#define MPT_WRITE_RF			12
#define MPT_READ_EEPROM_1BYTE	13
#define MPT_WRITE_EEPROM_1BYTE	14
#define MPT_READ_EEPROM_2BYTE	15
#define MPT_WRITE_EEPROM_2BYTE	16
#define MPT_SET_CSTHRESHOLD		21
#define MPT_SET_INITGAIN		22
#define MPT_SWITCH_BAND			23
#define MPT_SWITCH_CHANNEL		24
#define MPT_SET_DATARATE		25
#define MPT_SWITCH_ANTENNA		26
#define MPT_SET_TX_POWER		27
#define MPT_SET_CONT_TX			28
#define MPT_SET_SINGLE_CARRIER		29
#define MPT_SET_CARRIER_SUPPRESSION	30
#define MPT_GET_RATE_TABLE 			31


struct mp_wiparam
{
	u32	bcompleted;
	u32	act_type;
	u32		io_offset;
	u32		io_value;
};


typedef void(*wi_act_func)(PVOID padapter);

#ifdef PLATFORM_OS_XP
struct mp_wi_cntx
{
	u8 bmpdrv_unload;
	
	// Work Item 
	NDIS_WORK_ITEM mp_wi;
	NDIS_EVENT mp_wi_evt; 
	_lock mp_wi_lock; 
	u8 bmp_wi_progress; 
	wi_act_func curractfunc;
	// Variable needed in each implementation of CurrActFunc.
	struct mp_wiparam param;
	
} ;


struct mp_priv {
	
     NDIS_802_11_MAC_ADDRESS network_macaddr;
     struct wlan_network	mp_network;	 
     sint prev_fw_state;

     //Testing Flag
     uint mode;//0 for sd3, 1 for sd4
     
	
     //Tx Section
     u8 TID;
     u32 tx_pktcount;
  
     //Rx Section
     u32 rx_pktcount;//for sd3
     u32 rx_crcerrpktcount;//for sd3
     u32 rx_pktloss;//for sd4
     	 
     u32 rx_testcnt;
     u32 rx_testcnt1;	 
     u32 rx_testcnt2;
     u32 tx_testcnt;
     u32 tx_testcnt1;
     	 

     //RF/BB relative
     uint curr_ch;
     uint curr_modem;     
    
    //OID cmd handler
    struct mp_wiparam workparam;
    u8 act_in_progress;	
    struct  mp_wi_cntx wi_cntx;
    u8 	h2c_result;
    u8	h2c_seqnum;
    u16	h2c_cmdcode;	  
    u8	h2c_resp_parambuf[512];	
    _lock	h2c_lock;
    _lock	wkitm_lock;	
    uint	h2c_cmdcnt;
	

    struct recv_stat rxstat;
     	 
    
 };
#endif




extern void mp8711init(_adapter *padapter);
#endif //__RTL871X_MP_H_
