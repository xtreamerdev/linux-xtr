#ifndef	__RTL871X_RF_H_ 
#define __RTL871X_RF_H_


#include <drv_conf.h>
#include <rtl871x_cmd.h>

#define OFDM_PHY		1
#define MIXED_PHY		2
#define CCK_PHY		3

#define		NumRates	(13)
#define RTL8711_RF_MAX_SENS 6
#define RTL8711_RF_DEF_SENS 4


#define NUM_CHANNELS	15
#define NUM_REGULATORYS	21

//Country codes
#define USA							0x555320
#define EUROPE						0x1 //temp, should be provided later	
#define JAPAN						0x2 //temp, should be provided later	

struct	regulatory_class {
	u32	starting_freq;					//MHz, 
	u8	channel_set[NUM_CHANNELS];
	u8	channel_cck_power[NUM_CHANNELS];//dbm
	u8	channel_ofdm_power[NUM_CHANNELS];//dbm
	u8	txpower_limit;  				//dbm
	u8	channel_spacing;				//MHz
	u8	modem;
};

struct setphyinfo_parm;
extern void init_phyinfo(_adapter  *adapter, struct setphyinfo_parm* psetphyinfopara);
extern u8 writephyinfo_fw(_adapter *padapter, u32 addr);
extern u32 ch2freq(u32 ch);
extern u32 freq2ch(u32 freq);


#endif //_RTL8711_RF_H_
