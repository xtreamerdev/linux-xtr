
#ifndef __RTL871X_MLME_H_
#define __RTL871X_MLME_H_

#include <drv_conf.h>
#include <osdep_service.h>
#ifdef CONFIG_RTL8711
#include <rtl8711_spec.h>
#endif
#include <drv_types.h>
#include <wlan_bssdef.h>
//#include <rtl8187/wlan.h>
#include <rtl871x_security.h>
#include <sta_info.h>

#ifndef CONFIG_RTL8711FW



#define	MAX_BSS_CNT	128
#define   MAX_JOIN_TIMEOUT    15000//	2000
//#define   MAX_JOIN_TIMEOUT	2500

#define 	WIFI_NULL_STATE		0x00000000
#define	WIFI_ASOC_STATE		0x00000001		// Under Linked state...
#define 	WIFI_REASOC_STATE	       0x00000002
#define	WIFI_SLEEP_STATE	       0x00000004
#define	WIFI_STATION_STATE	0x00000008
#define	WIFI_AP_STATE				0x00000010
#define	WIFI_ADHOC_STATE			0x00000020
#define   WIFI_ADHOC_MASTER_STATE 0x00000040
#if 0
#define	WIFI_AUTH_NULL		0x00000100
#define	WIFI_AUTH_STATE1	0x00000200
#define	WIFI_AUTH_SUCCESS	0x00000400
#endif
#define WIFI_UNDER_LINKING	0x00000100
#define WIFI_UNDER_CMD		0x00000200
#define	WIFI_SITE_MONITOR	0x00000800		//to indicate the station is under site surveying
#ifdef WDS
#define	WIFI_WDS			0x00001000
#define	WIFI_WDS_RX_BEACON	0x00002000		// already rx WDS AP beacon

#endif

#ifdef AUTO_CONFIG
#define	WIFI_AUTOCONF		0x00004000
#define	WIFI_AUTOCONF_IND	0x00008000
#endif

//#ifdef UNDER_MPTEST
#define	WIFI_MP_STATE			 0x00010000
#define	WIFI_MP_CTX_BACKGROUND	0x00020000	// in continous tx background
#define	WIFI_MP_CTX_ST		0x00040000	// in continous tx with single-tone
#define	WIFI_MP_CTX_BACKGROUND_PENDING	 0x00080000	// pending in continous tx background due to out of skb
#define	WIFI_MP_CTX_CCK_HW	0x00100000	// in continous tx
#define	WIFI_MP_CTX_CCK_CS	0x00200000	// in continous tx with carrier suppression
#define   WIFI_MP_LPBK_STATE	0x00400000

//#endif
#define _FW_UNDER_CMD		WIFI_UNDER_CMD
#define _FW_UNDER_LINKING	WIFI_UNDER_LINKING
#define _FW_LINKED			WIFI_ASOC_STATE
#define _FW_UNDER_SURVEY	WIFI_SITE_MONITOR



/*

there are several "locks" in mlme_priv,
since mlme_priv is a shared resource between many threads,
like ISR/Call-Back functions, the OID handlers, and even timer functions.


Each _queue has its own locks, already.
Other items are protected by mlme_priv.lock.

To avoid possible dead lock, any thread trying to modifiying mlme_priv
SHALL not lock up more than one locks at a time!



*/
#define join_bss_buf_len 256
#define traffic_threshold	10
#define	traffic_scan_period	500
struct sitesurvey_ctrl{
	uint	last_tx_pkts;
	uint	last_rx_pkts;
	sint		traffic_busy;
	_timer	sitesurvey_ctrl_timer;
};

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

struct	bssid_desc	{
	unsigned char 	addr[ETH_ALEN];
};

struct	ss_res	{
	int	state;
	int	bss_cnt;
	int	channel;
	int	ss_ssidlen;
	int	passive_mode;
	unsigned char 	ss_ssid[IW_ESSID_MAX_SIZE + 1];
	struct	bssid_desc	bssid[MAX_BSS_CNT];
	
};
#endif

struct	mlme_priv	{
	_lock	lock;
	sint	fw_state;	//shall we protect this variable? maybe not necessarily...
	//unsigned char bscanned;
	
	u8 to_join; //flag
	u8 *nic_hdl;
	_list		*pscanned;
	_queue	free_bss_pool;
	_queue	scanned_queue;
	u8	*free_bss_buf;
	unsigned long	num_of_scanned;
	NDIS_802_11_SSID	assoc_ssid;
	u8	assoc_bssid[6];
	struct	wlan_network	cur_network;
	struct	sitesurvey_ctrl sitesurveyctrl;
	uint	wireless_mode;
	_timer	assoc_timer;
	uint  assoc_by_bssid;

	//john debug 0521
	_sema ioctl_sema;
#ifdef ENABLE_MGNT_THREAD
	_sema mgnt_sema;//mgnt_sema is used in mgnt_thread
	_queue mgnt_pending_queue;
#endif

#if 1
	struct	wlan_network* pnow_assoc_network;
#endif
	
};

#ifdef CONFIG_RTL8192U
extern void TSInitialize(_adapter *Adapter);
extern void TSDeInitialize(_adapter *padapter);
#endif

extern void survey_event_callback(_adapter *adapter, u8 *pbuf);
extern void surveydone_event_callback(_adapter *adapter, u8 *pbuf);
extern void joinbss_event_callback(_adapter *adapter, u8 *pbuf);
extern void stassoc_event_callback(_adapter *adapter, u8 *pbuf);
extern void stadel_event_callback(_adapter *adapter, u8 *pbuf);
extern void atimdone_event_callback(_adapter *adapter, u8 *pbuf);
extern thread_return event_thread(void *context);

extern void _sitesurvey_ctrl_handler(_adapter *adapter);
extern void _join_timeout_handler (_adapter *adapter);

extern void free_network_queue(_adapter *adapter);
extern int	init_mlme_priv(_adapter *adapter);// (struct	mlme_priv *pmlmepriv);

extern void free_mlme_priv (struct mlme_priv *pmlmepriv);
extern sint if_up(_adapter *padapter);

extern sint select_and_join_from_scanned_queue(struct mlme_priv	*pmlmepriv );
extern sint set_key(_adapter *adapter,struct security_priv *psecuritypriv,sint keyid);
extern sint set_auth(_adapter *adapter,struct security_priv *psecuritypriv);
static __inline 	sint check_fwstate(struct mlme_priv *pmlmepriv, sint state)
{
	
	if (pmlmepriv->fw_state & state)
		return _TRUE;
	else
		return _FALSE;
}

static u8 __inline *get_bssid(struct mlme_priv *pmlmepriv)
{	//if sta_mode:pmlmepriv->cur_network.network.MacAddress=> bssid
	// if adhoc_mode:pmlmepriv->cur_network.network.MacAddress=> ibss mac address
	return pmlmepriv->cur_network.network.MacAddress; 
}



/*
	No Limit on the calling context, 
	therefore set it to be the critical section...

### NOTE:#### (!!!!)
MUST TAKE CARE THAT BEFORE CALLING THIS FUNC, YOU SHOULD HAVE LOCKED pmlmepriv->lock

*/
static void	__inline 	set_fwstate(struct mlme_priv *pmlmepriv, sint state)
{
	
	pmlmepriv->fw_state = (pmlmepriv->fw_state | state);
	
}

/*
	No Limit on the calling context, 
	therefore set it to be the critical section...

*/
static void	__inline 	clr_fwstate(struct mlme_priv *pmlmepriv, sint state)
{
	_irqL	irqL;
	
	_enter_critical(&(pmlmepriv->lock), &irqL);
	
	if ((check_fwstate(pmlmepriv, state)) == _TRUE)
		pmlmepriv->fw_state = (pmlmepriv->fw_state ^ state);
	
	_exit_critical(&(pmlmepriv->lock), &irqL);


}


static  __inline  sint get_fwstate(struct mlme_priv *pmlmepriv)
{

	return pmlmepriv->fw_state;

}

static __inline void up_scanned_network(struct	mlme_priv	*pmlmepriv)
{
	
	_irqL	irqL;
	
	_enter_critical(&(pmlmepriv->lock), &irqL);
	
	pmlmepriv->num_of_scanned ++;
	
	_exit_critical(&(pmlmepriv->lock), &irqL);
	
}

static __inline void down_scanned_network(struct mlme_priv	*pmlmepriv)
{
	
	_irqL	irqL;
	
	_enter_critical(&(pmlmepriv->lock), &irqL);
	
	pmlmepriv->num_of_scanned --;
	
	_exit_critical(&(pmlmepriv->lock), &irqL);
	
	
}

static __inline void set_scanned_network_val(struct mlme_priv	*pmlmepriv, sint val)
{
	
	_irqL	irqL;
	
	_enter_critical(&(pmlmepriv->lock), &irqL);
	
	pmlmepriv->num_of_scanned = val;
	
	_exit_critical(&(pmlmepriv->lock), &irqL);
	
	
}


#endif

extern u16 get_capability(NDIS_WLAN_BSSID_EX *bss);
extern uint get_NDIS_WLAN_BSSID_EX_sz (NDIS_WLAN_BSSID_EX *bss);
extern void update_scanned_network(_adapter*adapter, NDIS_WLAN_BSSID_EX *target);
extern void disconnect_hdl_under_linked(_adapter* adapter, struct sta_info *psta, u8 free_assoc);
extern void generate_random_ibss(u8* pibss);
extern struct wlan_network *alloc_network(struct mlme_priv * pmlmepriv);
extern struct	wlan_network *find_network(_queue *scanned_queue, u8 *addr);
extern struct	wlan_network	* get_oldest_wlan_network(_queue *scanned_queue);

extern void free_assoc_resources(_adapter* adapter);
extern void indicate_disconnect(_adapter *adapter);
extern void indicate_connect(_adapter *adapter);

extern int restruct_sec_ie(_adapter *adapter,u8 *in_ie,u8 *out_ie,uint in_len);
extern int restruct_wmm_ie(_adapter *adapter, u8 *in_ie, u8 *out_ie, uint in_len, uint initial_out_len);
extern void init_registrypriv_dev_network(	_adapter* adapter);

extern void update_registrypriv_dev_network(	_adapter* adapter) ;
extern thread_return  mlme_thread(thread_context);
extern void get_encrypt_decrypt_from_registrypriv(	_adapter* adapter);

extern u8 *get_capability_from_ie(unsigned char *ie);
extern u16 get_capability(NDIS_WLAN_BSSID_EX *bss);
extern u8 *get_timestampe_from_ie(unsigned char *ie);
extern u8 *get_beacon_interval_from_ie(unsigned char *ie);
extern u16 get_beacon_interval(NDIS_WLAN_BSSID_EX *bss);
extern void HTInitializeHTInfo(_adapter *padapter);

#endif //__RTL871X_MLME_H_

