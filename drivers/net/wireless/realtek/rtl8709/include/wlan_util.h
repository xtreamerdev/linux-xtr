
#ifndef _WLAN_UTIL_H_
#define _WLAN_UTIL_H_

#include <rtl8711_spec.h>


#ifndef __ASSEMBLY__


#ifndef _WLAN_UTIL_C_


extern struct sta_data	*alloc_stadata(int id);
extern void *free_stadata(struct sta_data *psta);
extern struct sta_data* add_assoc_sta(unsigned char *addr, int id, int type, _adapter* padapter);
extern struct sta_data* search_assoc_sta(unsigned char *addr, _adapter* padapter);
extern void del_assoc_sta(_adapter *padapter, unsigned char *addr);
//extern void init_pktinfo(PACKET_INFO *pinfo);
extern void set_wlanhdr(unsigned char * pbuf, u8* da, u8 *sa, u8 *ra);
extern void llc_hdr_gen(unsigned char *pbuf);
//extern struct	sta_data *get_stainfo(unsigned char *hwaddr);
extern int get_bit_value_from_ieee_value(unsigned char val);
extern int get_rate_index_from_ieee_value(unsigned char val);
extern unsigned char get_rate_from_bit_value(int bit_val);
extern void get_bssrate_set(_adapter *padapter, int bssrate_ie, unsigned char *pbssrate, int *bssrate_len);



#else

struct sta_data	*alloc_stadata(int id);
void *free_stadata(struct sta_data *psta);
struct sta_data* add_assoc_sta(unsigned char *addr, int id, int type, _adapter* padapter);
struct sta_data* search_assoc_sta(unsigned char *addr, _adapter *padapter);
void del_assoc_sta(_adapter *padapter, unsigned char *addr);
void set_wlanhdr(unsigned char * pbuf, u8* da, u8 *sa, u8 *ra);
void llc_hdr_gen(unsigned char *pbuf);
//unsigned char * get_da(unsigned char *pframe);
struct	sta_data *get_stainfo(unsigned char *hwaddr);


#endif


#endif

#endif // _WLAN_UTIL_H_





