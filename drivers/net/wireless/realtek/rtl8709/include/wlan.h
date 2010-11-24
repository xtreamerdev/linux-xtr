
#ifndef _WLAN_H_
#define _WLAN_H_

#include <drv_types.h>
#include <sta_info.h>
#include <rtl8187/rtl8187_event.h>

#ifdef CONFIG_RTL8192U
#include <wifi.h>
#endif

//#include <rtl871x_rf.h>
//#include <rtl871x_mlme.h>
struct	rxirp	{
	_list			list;
	unsigned int	rxbuf_phyaddr;
	unsigned int	rx_len;
	unsigned int	tag;
	_adapter		*adapter;	
};

#define		CH_1		0
#define		CH_2		1
#define 		CH_3		2
#define		CH_4		3
#define		CH_5		4
#define		CH_6		5
#define		CH_7		6
#define		CH_8		7
#define		CH_9		8
#define		CH_10		9
#define		CH_11		10
#define 		CH_12		11
#define		CH_13		12
#define		CH_14		13
#define		CH_34		14
#define		CH_36		15
#define		CH_38		16
#define		CH_40		17
#define		CH_42		18
#define		CH_44		19
#define		CH_46		20
#define		CH_48		21
#define		CH_52		22
#define		CH_56		23
#define		CH_60		24
#define		CH_64		25

#define 	NumChannels	26

#define 	CAMSIZE		64


#define		_1M_RATE_	0
#define		_2M_RATE_	1
#define		_5M_RATE_	2
#define		_11M_RATE_	3
#define		_6M_RATE_	4
#define		_9M_RATE_	5
#define		_12M_RATE_	6
#define		_18M_RATE_	7
#define		_24M_RATE_	8
#define		_36M_RATE_	9
#define		_48M_RATE_	10
#define		_54M_RATE_	11


#define		EN_1MRATE		(1 << (_1M_RATE_))
#define		EN_2MRATE		(1 << (_2M_RATE_))
#define		EN_5MRATE		(1 << (_5M_RATE_))
#define		EN_11MRATE	(1 << (_11M_RATE_))
#define		EN_6MRATE		(1 << (_6M_RATE_))
#define		EN_9MRATE		(1 << (_9M_RATE_))
#define		EN_12MRATE	(1 << (_12M_RATE_))
#define		EN_18MRATE	(1 << (_18M_RATE_))
#define		EN_24MRATE	(1 << (_24M_RATE_))
#define		EN_36MRATE	(1 << (_36M_RATE_))
#define		EN_48MRATE	(1 << (_48M_RATE_))
#define		EN_54MRATE	(1 << (_54M_RATE_))

#define REAUTH_TO		(1000)
#define REASSOC_TO		(1000)
#define DISCONNECT_TO	(10000)
#define	ERP_TO			(100)
#define	RF_TO			(10)
#ifdef THERMAL_METHOD
#define PT_TO			(500)
#else
#define PT_TO			(50)
#endif

#define REAUTH_LIMIT	6
#define REASSOC_LIMIT	6

#define OFDM_PHY	1
#define MIXED_PHY	2
#define CCK_PHY		3

#define BAND_A		1
#define BAND_B		2
#define BAND_G		BAND_B
#define BAND_AG		3

#define APMODE		0x0C
#define STAMODE		0x08
#define ADHOCMODE	0x04
#define NOLINKMODE	0x00


#define WIFI_NULL_STATE			0x00000000
#define	WIFI_ASOC_STATE			0x00000001
#define WIFI_REASOC_STATE		0x00000002
#define	WIFI_SLEEP_STATE		0x00000004
#define	WIFI_STATION_STATE		0x00000008
#define	WIFI_AP_STATE			0x00000010
#define	WIFI_ADHOC_STATE		0x00000020
//marc@1013
#define	WIFI_ADHOC_LINK			WIFI_ASOC_STATE
#define	WIFI_ADHOC_BEACON		0x00000080

#define	WIFI_AUTH_NULL			0x00000100
#define	WIFI_AUTH_STATE1		0x00000200
#define	WIFI_AUTH_SUCCESS		0x00000400
#define	WIFI_SITE_MONITOR		0x00000800		//to indicate the station is under site surveying

#ifdef WDS
#define	WIFI_WDS			0x00001000
#define	WIFI_WDS_RX_BEACON	0x00002000		// already rx WDS AP beacon
#endif

#ifdef AUTO_CONFIG
#define	WIFI_AUTOCONF		0x00004000
#define	WIFI_AUTOCONF_IND	0x00008000
#endif

#ifdef MP_TEST
#define	WIFI_MP_STATE			 0x00010000
#define	WIFI_MP_CTX_BACKGROUND	0x00020000	// in continous tx background
#define	WIFI_MP_CTX_ST		0x00040000	// in continous tx with single-tone
#define	WIFI_MP_CTX_BACKGROUND_PENDING	 0x00080000	// pending in continous tx background due to out of skb
#define	WIFI_MP_CTX_CCK_HW	0x00100000	// in continous tx
#define	WIFI_MP_CTX_CCK_CS	0x00200000	// in continous tx with carrier suppression
#endif

#ifdef CONFIG_RTL8192U
#define	BA_POLICY_DELAYED		0
#define	BA_POLICY_IMMEDIATE	1

#define	ADDBA_STATUS_SUCCESS			0
#define	ADDBA_STATUS_REFUSED		37
#define	ADDBA_STATUS_INVALID_PARAM	38
#endif

//#define _NO_PRIVACY_                    0
#define _WEP_40_PRIVACY_                1
#define _TKIP_PRIVACY_                  2
#define _CCMP_PRIVACY_                  4
#define _WEP_104_PRIVACY_               5
#define _WEP_WPA_MIXED_PRIVACY_ 		6       // WEP + WPA
/*

[4-2]		key type	000: NA
				001: WEP-40
				010: TKIP without MIC
				011: TKIP with MIC
				100: AES  
				101: WEP-104


*/
//#include <wlan_bssdef.h>


//#define MAX_STASZ	(CAMSIZE)	//Four entries was reserved for Default Key, 
									//one is reserved for broadcast
#define MAX_STASZ	(64)

//marc
#define RXDATAIRPS_TRASH			32	
#define RXDATAIRPS_TRASH_TAG		4
#define RXDATAIRPS_TRASH_SIZE		1600

#define RXMGTIRPS_MGT	8			//don't change
#define RXMGTIRPS_TAG	0
#define RXMGTBUF_SZ	512			//don't change


#define TXMGTIRPS_MGT	8			
#define TXMGTIRPS_TAG	0
#define TXMGTBUF_SZ	512			


#define TXDATAIRPS_SMALL		32		//don't change
#define TXDATAIRPS_SMALL_TAG	1
#define TXDATAIRPS_SMALL_SZ	128		//don't change

#define TXDATAIRPS_MEDIUM		16		//don't change
#define TXDATAIRPS_MEDIUM_TAG	2
#define TXDATAIRPS_MEDIUM_SZ	512	//don't change

#define TXDATAIRPS_LARGE		2
#define TXDATAIRPS_LARGE_TAG	3
#define TXDATAIRPS_LARGE_SZ	1600


#define TXDATAIRPS_CLONE		32
#define TXDATAIRPS_CLONE_TAG	4

//#define RX1_SWCMDQSZ	128
#define RX1_SWCMDQSZ	32
#define RX0_SWCMDQSZ	32

#define LAZY_NumRates 	13
#define OPMODE	(_sys_mib->mac_ctrl.opmode)



enum Synchronization_Sta_State {
        STATE_Sta_Min				= 0,
        STATE_Sta_No_Bss			= 1,
        STATE_Sta_Bss				= 2,
        STATE_Sta_Ibss_Active		= 3,
        STATE_Sta_Ibss_Idle		= 4,
        STATE_Sta_Auth_Success	= 5,
        STATE_Sta_Roaming_Scan	= 6,
  #ifdef CONFIG_RTL8192U
        STATE_Sta_Join_Wait_Beacon= 7,
  #endif
};



struct cmd_ctrl 
{
volatile	int	head;
volatile	int mask_head;
volatile	int	tail;
volatile	int	mask_tail;	// make the size of cmd_ctrl to be the multiples of 8
volatile    unsigned int	baseaddr;
};

struct	mib_mac_ctrl {
	unsigned int	opmode;
	
	unsigned char	basicrate[LAZY_NumRates];
	unsigned char	basicrate_inx;
#ifdef LOWEST_BASIC_RATE	
	unsigned char	lowest_basicrate_inx;
#endif	
	unsigned char	datarate[LAZY_NumRates];
	unsigned char	dtim;
	//unsigned short	beacon_itv;				// beacon interval, in units of ms
	unsigned int	bcn_size;	
	unsigned char	en_rateadaptive;
	unsigned int	timie_offset;
	unsigned int	qbssloadie_offset;
	unsigned char	*pbcn;
	unsigned int	fimr;
	unsigned short	sysimr;
	unsigned short	rtimr;
	unsigned char	mac[ETH_ALEN];
#if 1
	unsigned char	security;		//0: NONE, 1: Legacy shared key 2: 802.1x(KeyMapping)
	unsigned char	_1x;			//0: PSK, 1: TLS
	unsigned char	defkeytype;		//This bit will only be valid when security is 1. 1: wep40, 2:wep104
	unsigned char	defkeyid;		//This bit will only be valid when security is 1. defkeyid: 0 ~ 3
	unsigned char 	grpkeytype;		//This bit will only be valid when security is 2. It carries the GRP key Type.
	unsigned char	unicastkeytype;	//This bit will only be valid when security is 2. It carries the Unicast Key Type.
#endif
	
	//struct	cmd_ctrl	txcmds[TXQUEUES];
	//struct	cmd_ctrl	rxcmds[RXQUEUES];
		
};

struct	txirp	{
	_list	list;
	unsigned int	txcmd_offset4;
	unsigned int	txbuf_phyaddr;
	unsigned int	tx_len;
	unsigned int	tag;
	int	q_inx;
};

struct	txirp_mgt {
	struct	txirp		irps[TXMGTIRPS_MGT];
};

struct	txirp_small {
	struct	txirp	irps[TXDATAIRPS_SMALL];
};

struct	txirp_medium {
	struct	txirp	irps[TXDATAIRPS_MEDIUM];
};

struct	txirp_large {
	struct	txirp	irps[TXDATAIRPS_LARGE];
};

struct	txbufhead_pool {
	struct	txirp_mgt		mgt;
	struct	txirp_small	small;
	struct	txirp_medium	medium;
	struct	txirp_large	large;
};

struct	rxirp_mgt	{
	struct	rxirp		irps[RXMGTIRPS_MGT];	
};

struct	rxbufhead_pool	{
	struct	rxirp_mgt	mgt;
};

struct	mib_rf_ctrl {
	unsigned char	shortpreamble;
#if 0	
	unsigned char	tx_antset;
	unsigned char	rx_antset;
	unsigned short	fixed_txant;
	unsigned char	fixed_rxant;

	// TXAGC shall depend on Rate/Channel
	unsigned char	txagc[LAZY_NumRates];			// Driver shall re-initialize txagc table 
												// before channel switching	
	
	unsigned char	agcctrl;

	//unsigned int 	tssi_tgt[NumChannels];		// TSSI TGT alwasy calibrate on 54Mbps PHY Rate
	
	unsigned char	ss_ForceUp[LAZY_NumRates];
	
	unsigned char	ss_ULevel[LAZY_NumRates];
	
	unsigned char	ss_DLevel[LAZY_NumRates];
	
	unsigned char	count_judge[LAZY_NumRates];
#endif
		
};


struct erp_mib {
	int	shortSlot;		// short slot time flag
	int	protection;		// protection mechanism flag
	int	nonErpStaNum;	// none ERP client assoication num
	int	olbcDetected;	// OLBC detected
	int	longPreambleStaNum; // number of assocated STA using long preamble
#if 0
	int	olbcExpired;	// expired time of OLBC state
	int	ctsToSelf;		// CTStoSelf flag
#endif
};


struct Dot1180211AuthEntry {
	unsigned int	dot11AuthAlgrthm;		// 802.11 auth, could be open, shared, auto
	unsigned int	dot11PrivacyAlgrthm;	// encryption algorithm, could be none, wep40, TKIP, CCMP, wep104
	unsigned int	dot11PrivacyKeyIndex;	// this is only valid for legendary wep, 0~3 for key id.
	unsigned int	dot11PrivacyKeyLen;		// this could be 40 or 104
#ifdef INCLUDE_WPA_PSK
	int				dot11EnablePSK;			// 0: disable, bit0: WPA, bit1: WPA2
	int				dot11WPACipher;			// bit0-wep64, bit1-tkip, bit2-wrap,bit3-ccmp, bit4-wep128
#ifdef RTL_WPA2
	int				dot11WPA2Cipher;			// bit0-wep64, bit1-tkip, bit2-wrap,bit3-ccmp, bit4-wep128
#endif
	unsigned char	dot11PassPhrase[65];	// passphrase
	unsigned long	dot11GKRekeyTime;		// group key rekey time, 0 - disable
#endif
	int	authModeRetry;
};

struct Dot11RsnIE {
	unsigned char rsnie[128];
	unsigned char rsnielen;
};

struct mib_info 
{
	struct 	sta_data	sta_data[MAX_STASZ];
	struct 	stainfo			stainfos[MAX_STASZ];
	struct 	stainfo2			stainfo2s[MAX_STASZ];
	struct	stainfo_rxcache	rxcache[MAX_STASZ];
	struct	stainfo_stats		stainfostats[MAX_STASZ];	
	struct	network_queue	networks;
	struct	mib_rf_ctrl		rf_ctrl;
	struct 	erp_mib 			dot11ErpInfo;
	struct 	Dot11RsnIE		dot11RsnIE;
	struct	Dot1180211AuthEntry 	dot1180211authentry;
	struct	c2hevent_queue 	c2hevent;
	struct	del_sta_queue		del_sta_q;
	struct	regulatory_class	class_sets[NUM_REGULATORYS];
	struct	regulatory_class	*cur_class;
	struct	rxbufhead_pool	rxbufheadpool;
	struct	txbufhead_pool	txbufheadpool;
	struct	mib_mac_ctrl		mac_ctrl;
	struct	event_node		survey_done_event;
	struct 	surveydone_event	survey_done;
	struct	event_node		join_res_event;
	struct	joinbss_event		joinbss_done;
	struct	event_node		add_sta_event;
	struct	stassoc_event		add_sta_done;
	struct	event_node		del_sta_event;
	struct	stadel_event		del_sta_done;
	struct	ss_res			sitesurvey_res;
	NDIS_WLAN_BSSID_EX	cur_network;
	_timer	survey_timer;
	_timer	reauth_timer;
	_timer	reassoc_timer;
	_timer	disconnect_timer;
#ifdef CONFIG_RTL8192U
	_timer	sendbeacon_timer;	
	u32 edca_param[4];
#endif

	_list	free_rxirp_mgt;
	_list	free_txirp_mgt;
	_list 	assoc_entry;	
	_list	hash_array[MAX_STASZ];
	
	_sema cam_sema;
	_lock free_rxirp_mgt_lock;
	_lock free_txirp_mgt_lock;

#ifdef CONFIG_RTL8192U
	_sema queue_event_sema;
#endif

	u8	evt_clear;
	
	u32	reauth_count;
	u32	reassoc_count;
	u32	auth_seq;
	u32	join_req_ongoing;
	u32	join_res;
	u32	bcnlimit;
	u32 h2crsp_addr[512>>2];
	u32	c2h_res;
	int 	authModeToggle;
	int	bcn_to_cnt;

	u16	aid;
	u16	ps_bcn_itv;	/* Current AP's beacon interval */
	
	unsigned char	cur_channel;
	unsigned char	dtim_period;
	u8	change_chan;	
	u8	cmd_proc;
	u8	cur_modem;
	u8	network_type;
	u8	vcs_mode;
	u8 	h2cseq;
	u8	old_channel;
	u8	old_modem;
	u8	sur_regulatory_inx;
	u8	ch_inx;
	u8	beacon_cnt;
	u8	*c2h_buf;
	u8	*pallocated_fw_rxirp_buf;
	u8	*pfw_rxirp_buf;
	u8	*pallocated_fw_txirp_buf;
	u8	*pfw_txirp_buf;

	//TX rate
	int	rate;       /* current rate */
	int	basic_rate;

	//DIG parameter
	u8	bDynamicInitGain;
	u8	RegBModeGainStage;
	u8	RegDigOfdmFaUpTh;
	u8	InitialGain;
	u8	DIG_NumberFallbackVote;
	u8	DIG_NumberUpgradeVote;
	u8	StageCCKTh;
	u16	CCKUpperTh;
	u16	CCKLowerTh;
	u32	FalseAlarmRegValue;

	//Rate adaptive parameter
	u8	bRateAdaptive;
	u8	ForcedDataRate;
	u8	TryupingCountNoData;
	u8	TryDownCountLowData;
	u8	bTryuping;
	u8	LastFailTxRate;
	u8	FailTxRateCount;
	u16	CurrRetryCnt;
	u16	LastRetryCnt;
	u16	LastRetryRate;
	u16	TryupingCount;
	u32	LastTxThroughput;
	s32	LastFailTxRateSS;
	s32	RecvSignalPower;
	u64	LastTxokCnt;
	u64	LastRxokCnt;
	u64	LastTxOKBytes;
	u64	NumTxOkTotal;
	u64	NumTxOkBytesTotal;
	u64	NumRxOkTotal;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
	struct work_struct DigWorkItem;
	struct work_struct RateAdaptiveWorkItem;
#else
	struct delayed_work DigWorkItem;
	struct delayed_work RateAdaptiveWorkItem;
	_adapter *padapter;
#endif
};

static __inline__ int is_CCK_rate(unsigned char rate)
{
	if ((rate == 2) || (rate == 4) || (rate == 11) || (rate == 22))
		return _TRUE;
	else
		return _FALSE;
}

static __inline int get_bsstype(unsigned short capability)
{

	if (capability & BIT(0))
		return WIFI_AP_STATE;
	else if (capability & BIT(1))
		return WIFI_ADHOC_STATE;
	else
		return 0;	
	
	
}

static __inline u8 get_phy(struct mib_info *pmibinfo)
{
	return (pmibinfo->cur_modem);
}

#ifndef _WLAN_TASK_C_
//extern struct mib_info sys_mib;
//extern struct Dot1180211AuthEntry	dot1180211AuthEntry;
//extern struct Dot11RsnIE dot11RsnIE; 
//extern struct erp_mib 	dot11ErpInfo;
	
//extern struct  sta_data        stadata[MAX_STASZ];
	
//extern volatile unsigned char	myid_var[] ;
extern unsigned char	assoc_bssid[] ;
extern unsigned char 	broadcast_sta[] ;

extern unsigned char assoc_sta0[];

extern unsigned char assoc_stax[];

extern unsigned char pseudo_sta0[];
extern unsigned char pseudo_sta1[];
extern unsigned char pseudo_sta2[];
extern unsigned char pseudo_sta3[];

extern unsigned char	wep40_df_key0[]; 
extern unsigned char	wep40_df_key1[];
extern unsigned char	wep40_df_key2[];
extern unsigned char	wep40_df_key3[];

extern unsigned char	wep104_df_key0[]; 
extern unsigned char	wep104_df_key1[];
extern unsigned char	wep104_df_key2[];
extern unsigned char	wep104_df_key3[];

extern unsigned char	grp_key0[];
extern unsigned char grp_key1[];

extern unsigned char assoc_bssid_key[];

extern unsigned char assoc_sta0_key[];

extern unsigned char assoc_stax_key[];

extern unsigned char null_key[];

extern unsigned char chg_txt[128];

#else
	struct mib_info sys_mib;
//	struct  sta_data        stadata[MAX_STASZ];
//	struct Dot1180211AuthEntry	dot1180211AuthEntry;
//	struct Dot11RsnIE dot11RsnIE; 
//	struct erp_mib 	dot11ErpInfo;
	
	volatile unsigned char myid_var[] = {0x00, 0xe0, 0x4c, 0x87, 0x15, 0x51};
	unsigned char	assoc_bssid[] = {0x00, 0xe0, 0x4c, 0x87, 0x86, 0x11};
	unsigned char broadcast_sta[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
/*--------------------------------------------------------------------------*/

unsigned char assoc_sta0[] = {0x00, 0xe0, 0x4c, 0x87, 0x11, 0x00};

unsigned char assoc_stax[] = {0x00, 0xe0, 0x4c, 0x87, 0x11, 0x01};

unsigned char pseudo_sta0[] = {0x00, 0xe0, 0x4c, 0x00, 0x00, 0x00};
unsigned char pseudo_sta1[] = {0x00, 0xe0, 0x4c, 0x00, 0x00, 0x01};
unsigned char pseudo_sta2[] = {0x00, 0xe0, 0x4c, 0x00, 0x00, 0x02};
unsigned char pseudo_sta3[] = {0x00, 0xe0, 0x4c, 0x00, 0x00, 0x03};

//unsigned char broadcast_sta[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


unsigned char	wep40_df_key0[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
unsigned char	wep40_df_key1[] = {0x5a, 0xa5, 0x87, 0x39, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char	wep40_df_key2[] = {0xff, 0x00, 0x55, 0x74, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char	wep40_df_key3[] = {0x74, 0x91, 0xbe, 0xc3, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

unsigned char	wep104_df_key0[]= {0x00, 0x11, 0x22, 0x33, 0x44, 0x5a, 0xa5, 0x87, 0x39, 0x5f, 0x97, 0x23, 0xe2, 0x00, 0x00, 0x00}; 
unsigned char	wep104_df_key1[]= {0x5a, 0xa5, 0x87, 0x39, 0x5f, 0x00, 0x11, 0x22, 0x33, 0x44, 0x00, 0x55, 0x74, 0x00, 0x00, 0x00};
unsigned char	wep104_df_key2[]= {0xff, 0x00, 0x55, 0x74, 0x28, 0x74, 0x91, 0xbe, 0xc3, 0x6d, 0x87, 0x39, 0x5f, 0x00, 0x00, 0x00};
unsigned char	wep104_df_key3[]= {0x74, 0x91, 0xbe, 0xc3, 0x6d, 0x39, 0x5f, 0x00, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00};


unsigned char	grp_key0[] =      {0x73, 0xe6, 0x5a, 0x74, 0xa5, 0x74, 0x91, 0xbe, 0xfa, 0x6d, 0x87, 0xa1, 0x5f, 0x00, 0x00, 0x00};
unsigned char 	grp_key1[] = 	  {0xbe, 0x8d, 0x73, 0xc3, 0x6d, 0x63, 0x5f, 0x39, 0x23, 0x22, 0x37, 0x44, 0x8c, 0x00, 0x00, 0x00};

unsigned char assoc_bssid_key[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x5a, 0xa5, 0x87, 0x39, 0x5f, 0x74, 0x91, 0xbe, 0xc3, 0x6d, 0x23};

unsigned char assoc_sta0_key[]  = {0x5a, 0x37, 0x46, 0x7f, 0xe1, 0x28, 0x90, 0x87, 0x39, 0x3d, 0x2c, 0xbd, 0xeb, 0x3c, 0xd6, 0x62};

unsigned char assoc_stax_key[]  = {0x5a, 0x37, 0x46, 0x7f, 0xe1, 0x28, 0x90, 0x87, 0x39, 0x3d, 0x2c, 0xbd, 0xeb, 0x3c, 0xd6, 0x62};


unsigned char null_key[] = 		  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	
unsigned char chg_txt[128];
	
#endif
static __inline unsigned char *get_txpframe(struct txirp *pirp)
{
        return (unsigned char *)(pirp->txbuf_phyaddr);
}

static __inline__ unsigned char *get_rxpframe(struct rxirp *pirp)
{
	return (unsigned char *)( pirp->rxbuf_phyaddr);
}


static __inline unsigned char *get_sta_addr(struct  sta_data *psta)
{
	return (psta->info2->addr);	
}

static __inline unsigned char *get_my_addr(struct mib_info *_sys_mib)
{
	return (_sys_mib->mac_ctrl.mac);	
}


static __inline unsigned int	get_sta_authalgrthm(struct  sta_data *psta)
{
	return psta->AuthAlgrthm;
}


#endif /*_WLAN_H_*/




