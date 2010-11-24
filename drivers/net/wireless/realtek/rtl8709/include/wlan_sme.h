
#ifndef _WLAN_SME_H_
#define _WLAN_SME_H_


#ifndef _WLAN_SME_C_

extern unsigned char  cck_basicrate[];
extern unsigned char  ofdm_basicrate[];
extern unsigned char  mixed_basicrate[];
extern unsigned char  cck_datarate[];
extern unsigned char  ofdm_datarate[];
extern unsigned char  mixed_datarate[];
extern unsigned char  rate_tbl[];


extern unsigned int get_NDIS_WLAN_BSSID_EX_sz (NDIS_WLAN_BSSID_EX *bss);
extern void set_capability_to_ie(unsigned char *ie, unsigned short val);
extern void set_beacon_interval_to_ie(unsigned char *ie, unsigned short val);
extern unsigned int	get_rateset_len(unsigned char	*rateset);
extern void setup_rate_tbl(_adapter *padapter);
extern void rf_init(void);
extern void write_cam(_adapter *padapter,unsigned char entry, unsigned short ctrl, unsigned char *mac, unsigned char *key);
extern void read_cam(unsigned char entry, unsigned char *cnt);
extern void invalidate_cam(unsigned char entry);
extern void cam_config(void);

#ifdef CONFIG_RTL8187
extern void join_bss(_adapter *padapter);
#endif

extern void setup_assoc_sta(unsigned char *addr, int camid, int mode, unsigned short ctrl, unsigned char *key, _adapter* padapter);
extern int	 judge_network_type(unsigned char *rate, int ratelen, int channel);
extern void update_supported_rate(_adapter *padapter, unsigned char *da, unsigned char *ptn, unsigned int ptn_sz);
extern void set_slot_time(int slot_time);

extern unsigned char ratetbl_val_2wifirate(unsigned char rate);
#else

#ifdef CONFIG_RTL8187
void join_bss(_adapter *padapter);
#endif

#endif



#endif // _WLAN_SME_H_





