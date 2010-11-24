#ifndef __STA_INFO_H_
#define __STA_INFO_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtl871x_xmit.h>
#include <rtl871x_recv.h>
#include <rtl871x_security.h>

#define NUM_STA 32
#define NUM_ACL 64
#define LAZY_NumRates2 13


//if mode ==0, then the sta is allowed once the addr is hit.
//if mode ==1, then the sta is rejected once the addr is non-hit.
struct wlan_acl_node {
        _list		        list;
        u8       addr[ETH_ALEN];
        u8       mode;
};

struct wlan_acl_pool {
        struct wlan_acl_node aclnode[NUM_ACL];
};

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

struct stainfo	{
unsigned char	txrate[LAZY_NumRates2];
unsigned char	c_ss_forceup;
unsigned char	c_ss_ulevel;
unsigned char	c_ss_dlevel;
unsigned char	c_retrycnt;
unsigned char	c_txfailcnt;		//sunghau@0405
unsigned char	st_test_rate_up;	//1212: by scott for rateadaptive test rate up
unsigned char	cnt_insert_desc;	//1212: by scott for rateadaptive test rate up to insert higher descriptor
unsigned char	txrate_inx;
unsigned char	attribute;		// bit0 is used to indicate if it is multicast
unsigned short	tx_swant;	// tx sw antenna
};


struct stainfo2	{
unsigned short 	tid0_seq;
unsigned short  tid1_seq;	
unsigned short  tid2_seq;	
unsigned short  tid3_seq;	
unsigned short  tid4_seq;	
unsigned short  tid5_seq;	
unsigned short  tid6_seq;
unsigned short  tid7_seq;	
unsigned short  tid8_seq;
unsigned short  tid9_seq;
unsigned short  tid10_seq;
unsigned short  tid11_seq;
unsigned short  tid12_seq;
unsigned short  tid13_seq;
unsigned short  tid14_seq;
unsigned short  tid15_seq;
unsigned short	nonqos_seq;
//unsigned char	rsvd0[2];
unsigned short	txretry_cnt;
unsigned char	addr[6];
//unsigned char	camid;
unsigned char	aid;
//unsigned char	rsvd1[5];
unsigned char	ra_result;	//ra_result: 2->up 1->down 0-> noset
unsigned int	txok_cnt;
};

struct	sta_data	{

	struct stainfo	*info;
	struct stainfo2 *info2;
	struct stainfo_rxcache *rxcache;
	struct stainfo_stats *stats;
	int status;
	unsigned char bssrateset[32];
	unsigned int	bssratelen;
	int _protected; //when set, this entry can't be removed/deleted
	unsigned long	jiffies;
	unsigned int	auth_seq;
	unsigned int	AuthAlgrthm; //802.11 auth, could be open, shared or auto
	unsigned int		state;
	_list	hash;
	_list	assoc;
	_list	sleep;
	unsigned char	chg_txt[128];
	unsigned int		pkt_cnt;
	
};
#endif /*CONFIG_RTL8187*/

struct	stainfo_stats	{

	uint	rx_pkts;
	uint	rx_bytes;
	uint	tx_pkts;
	uint	tx_bytes;

};

struct sta_info {

	_lock lock;
	_list list; //free_sta_queue
	_list hash_list; //sta_hash
	_list asoc_list; //20061114
	_list sleep_list;//sleep_q
	_list wakeup_list;//wakeup_q
	
	struct sta_xmit_priv sta_xmitpriv;
	struct sta_recv_priv sta_recvpriv;
	
	uint	state;
	uint aid;
	uint mac_id;
	uint qos_option;
	u8	hwaddr[ETH_ALEN];

	uint	ieee8021x_blocked;	//0: allowed, 1:blocked 
	uint	dot118021XPrivacy; //aes, tkip...
	union Keytype	dot11tkiptxmickey;
	union Keytype	dot11tkiprxmickey;
	union Keytype	dot118021x_UncstKey;	
	union pn48		dot11txpn;			// PN48 used for Unicast xmit.
	union pn48		dot11rxpn;			// PN48 used for Unicast recv.


	u8		bssrateset[32];
	uint		bssratelen;
	u16	rtk_capability;
	s32  rssi;
	s32	 signal_quality;


};



struct	sta_priv {
	
	u8 *pallocated_stainfo_buf;
	u8 *pstainfo_buf;
	_queue	free_sta_queue;
	
	_lock sta_hash_lock;
	_list   sta_hash[NUM_STA];
	int asoc_sta_count;
	_queue sleep_q;
	_queue wakeup_q;
	
};


static __inline u32 wifi_mac_hash(u8 *mac)
{
        u32 x;

        x = mac[0];
        x = (x << 2) ^ mac[1];
        x = (x << 2) ^ mac[2];
        x = (x << 2) ^ mac[3];
        x = (x << 2) ^ mac[4];
        x = (x << 2) ^ mac[5];

        x ^= x >> 8;
        x  = x & (NUM_STA - 1);
		
        return x;
}


extern u32	_init_sta_priv(struct sta_priv *pstapriv);
extern u32	_free_sta_priv(struct sta_priv *pstapriv);
extern struct sta_info *alloc_stainfo(struct	sta_priv *pstapriv, u8 *hwaddr);
extern u32	free_stainfo(_adapter *padapter , struct sta_info *psta);
extern void free_all_stainfo(_adapter *padapter);
extern struct sta_info *get_stainfo(struct sta_priv *pstapriv, u8 *hwaddr);
extern u32 init_bcmc_stainfo(_adapter* padapter);
extern struct sta_info* get_bcmc_stainfo(_adapter* padapter);
extern u8 access_ctrl(struct wlan_acl_pool* pacl_list, u8 * mac_addr);

#endif //_STA_INFO_H_
