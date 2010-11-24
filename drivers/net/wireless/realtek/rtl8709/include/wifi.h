#ifndef _WIFI_H_
#define _WIFI_H_

#include <rtl8711_byteorder.h>

#ifdef BIT
//#error	"BIT define occurred earlier elsewhere!\n"
#undef BIT
#endif
#define BIT(x)	(1 << (x))


#define WLAN_ETHHDR_LEN		14
#define WLAN_ETHADDR_LEN	6
#define WLAN_IEEE_OUI_LEN	3
#define WLAN_ADDR_LEN		6
#define WLAN_CRC_LEN		4
#define WLAN_BSSID_LEN		6
#define WLAN_BSS_TS_LEN		8
#define WLAN_HDR_A3_LEN		24
#define WLAN_HDR_A4_LEN		30
#define WLAN_HDR_A3_QOS_LEN	26
#define WLAN_HDR_A4_QOS_LEN	32
#define WLAN_SSID_MAXLEN	32
#define WLAN_DATA_MAXLEN	2312

#define WLAN_A3_PN_OFFSET	24
#define WLAN_A4_PN_OFFSET	30

#define WLAN_MIN_ETHFRM_LEN	60
#define WLAN_MAX_ETHFRM_LEN	1514
#define WLAN_ETHHDR_LEN		14

#define P80211CAPTURE_VERSION	0x80211001

#ifdef GREEN_HILL
#pragma pack(1)
#endif

enum WIFI_FRAME_TYPE {
	WIFI_MGT_TYPE  =	(0),
	WIFI_CTRL_TYPE =	(BIT(2)),
	WIFI_DATA_TYPE =	(BIT(3)),
	WIFI_QOS_DATA_TYPE	= (BIT(7)|BIT(3)),	//!< QoS Data	
};

enum WIFI_FRAME_SUBTYPE {

	// below is for mgt frame
	WIFI_ASSOCREQ       = (0 | WIFI_MGT_TYPE),
    WIFI_ASSOCRSP       = (BIT(4) | WIFI_MGT_TYPE),
    WIFI_REASSOCREQ     = (BIT(5) | WIFI_MGT_TYPE),
    WIFI_REASSOCRSP     = (BIT(5) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_PROBEREQ       = (BIT(6) | WIFI_MGT_TYPE),
    WIFI_PROBERSP       = (BIT(6) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_BEACON         = (BIT(7) | WIFI_MGT_TYPE),
    WIFI_ATIM           = (BIT(7) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_DISASSOC       = (BIT(7) | BIT(5) | WIFI_MGT_TYPE),
    WIFI_AUTH           = (BIT(7) | BIT(5) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_DEAUTH         = (BIT(7) | BIT(6) | WIFI_MGT_TYPE),
    WIFI_ACTION         = (BIT(7) | BIT(6) | BIT(4) | WIFI_MGT_TYPE),

    // below is for control frame
    WIFI_PSPOLL         = (BIT(7) | BIT(5) | WIFI_CTRL_TYPE),
    WIFI_RTS            = (BIT(7) | BIT(5) | BIT(4) | WIFI_CTRL_TYPE),
    WIFI_CTS            = (BIT(7) | BIT(6) | WIFI_CTRL_TYPE),
    WIFI_ACK            = (BIT(7) | BIT(6) | BIT(4) | WIFI_CTRL_TYPE),
    WIFI_CFEND          = (BIT(7) | BIT(6) | BIT(5) | WIFI_CTRL_TYPE),
    WIFI_CFEND_CFACK    = (BIT(7) | BIT(6) | BIT(5) | BIT(4) | WIFI_CTRL_TYPE),

    // below is for data frame
    WIFI_DATA           = (0 | WIFI_DATA_TYPE),
    WIFI_DATA_CFACK     = (BIT(4) | WIFI_DATA_TYPE),
    WIFI_DATA_CFPOLL    = (BIT(5) | WIFI_DATA_TYPE),
    WIFI_DATA_CFACKPOLL = (BIT(5) | BIT(4) | WIFI_DATA_TYPE),
    WIFI_DATA_NULL      = (BIT(6) | WIFI_DATA_TYPE),
    WIFI_CF_ACK         = (BIT(6) | BIT(4) | WIFI_DATA_TYPE),
    WIFI_CF_POLL        = (BIT(6) | BIT(5) | WIFI_DATA_TYPE),
    WIFI_CF_ACKPOLL     = (BIT(6) | BIT(5) | BIT(4) | WIFI_DATA_TYPE),
    WIFI_QOS_DATA       = (BIT(7) | WIFI_DATA_TYPE),
    WIFI_QOS_NULL       = (BIT(7) | BIT(6) | WIFI_DATA_TYPE),
};

enum WIFI_REASON_CODE	{
	_RSON_RESERVED_					= 0,
	_RSON_UNSPECIFIED_				= 1,
	_RSON_AUTH_NO_LONGER_VALID_		= 2,
	_RSON_DEAUTH_STA_LEAVING_		= 3,
	_RSON_INACTIVITY_				= 4,
	_RSON_UNABLE_HANDLE_			= 5,
	_RSON_CLS2_						= 6,
	_RSON_CLS3_						= 7,
	_RSON_DISAOC_STA_LEAVING_		= 8,
	_RSON_ASOC_NOT_AUTH_			= 9,

	// WPA reason
	_RSON_INVALID_IE_				= 13,
	_RSON_MIC_FAILURE_				= 14,
	_RSON_4WAY_HNDSHK_TIMEOUT_		= 15,
	_RSON_GROUP_KEY_UPDATE_TIMEOUT_	= 16,
	_RSON_DIFF_IE_					= 17,
	_RSON_MLTCST_CIPHER_NOT_VALID_	= 18,
	_RSON_UNICST_CIPHER_NOT_VALID_	= 19,
	_RSON_AKMP_NOT_VALID_			= 20,
	_RSON_UNSUPPORT_RSNE_VER_		= 21,
	_RSON_INVALID_RSNE_CAP_			= 22,
	_RSON_IEEE_802DOT1X_AUTH_FAIL_	= 23,

	//belowing are Realtek definition
	_RSON_PMK_NOT_AVAILABLE_		= 24,
};

enum WIFI_STATUS_CODE {
	_STATS_SUCCESSFUL_			= 0,
	_STATS_FAILURE_				= 1,
	_STATS_CAP_FAIL_			= 10,
	_STATS_NO_ASOC_				= 11,
	_STATS_OTHER_				= 12,
	_STATS_NO_SUPP_ALG_			= 13,
	_STATS_OUT_OF_AUTH_SEQ_		= 14,
	_STATS_CHALLENGE_FAIL_		= 15,
	_STATS_AUTH_TIMEOUT_		= 16,
	_STATS_UNABLE_HANDLE_STA_	= 17,
	_STATS_RATE_FAIL_			= 18,
};

enum WIFI_REG_DOMAIN {
	DOMAIN_FCC		= 1,
	DOMAIN_IC		= 2,
	DOMAIN_ETSI		= 3,
	DOMAIN_SPAIN	= 4,
	DOMAIN_FRANCE	= 5,
	DOMAIN_MKK		= 6,
	DOMAIN_ISRAEL	= 7,
	DOMAIN_MKK1		= 8,
	DOMAIN_MKK2		= 9,
	DOMAIN_MKK3		= 10,
	DOMAIN_MAX
};

#define _TO_DS_		BIT(8)
#define _FROM_DS_	BIT(9)
#define _MORE_FRAG_	BIT(10)
#define _RETRY_		BIT(11)
#define _PWRMGT_	BIT(12)
#define _MORE_DATA_	BIT(13)
#define _PRIVACY_	BIT(14)
#define _ORDER_		BIT(15)

#define SetToDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_TO_DS_); \
	} while(0)

#define GetToDs(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_TO_DS_)) != 0)

#define ClearToDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_TO_DS_)); \
	} while(0)

#define SetFrDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_FROM_DS_); \
	} while(0)

#define GetFrDs(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_FROM_DS_)) != 0)

#define ClearFrDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_FROM_DS_)); \
	} while(0)

#define get_tofr_ds(pframe)	((GetToDs(pframe) << 1) | GetFrDs(pframe))


#define SetMFrag(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_MORE_FRAG_); \
	} while(0)

#define GetMFrag(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_MORE_FRAG_)) != 0)

#define ClearMFrag(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_MORE_FRAG_)); \
	} while(0)

#define SetRetry(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_RETRY_); \
	} while(0)

#define GetRetry(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_RETRY_)) != 0)

#define ClearRetry(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_RETRY_)); \
	} while(0)

#define SetPwrMgt(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_PWRMGT_); \
	} while(0)

#define GetPwrMgt(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_PWRMGT_)) != 0)

#define ClearPwrMgt(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_PWRMGT_)); \
	} while(0)

#define SetMData(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_MORE_DATA_); \
	} while(0)

#define GetMData(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_MORE_DATA_)) != 0)

#define ClearMData(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_MORE_DATA_)); \
	} while(0)

#define SetPrivacy(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_PRIVACY_); \
	} while(0)

#define GetPrivacy(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_PRIVACY_)) != 0)

#define ClearPrivacy(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_PRIVACY_)); \
	} while(0)
	
	
#define SetOrder(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_ORDER_); \
	} while(0)

#define GetOrder(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_ORDER_)) != 0)

#define ClearOrder(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_ORDER_)); \
	} while(0)


#define GetFrameType(pbuf)	(le16_to_cpu(*(unsigned short *)(pbuf)) & (BIT(3) | BIT(2)))

#define SetFrameType(pbuf,type)	\
	do { 	\
		*(unsigned short *)(pbuf) &= __constant_cpu_to_le16(~(BIT(3) | BIT(2))); \
		*(unsigned short *)(pbuf) |= __constant_cpu_to_le16(type); \
	} while(0)

#define GetFrameSubType(pbuf)	(cpu_to_le16(*(unsigned short *)(pbuf)) & (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2)))

#define SetFrameSubType(pbuf,type) \
	do {    \
		*(unsigned short *)(pbuf) &= cpu_to_le16(~(BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2))); \
		*(unsigned short *)(pbuf) |= cpu_to_le16(type); \
	} while(0)

#define SetQC(pbuf,qc) \
	do {    \
		*(unsigned short *)(pbuf) = cpu_to_le16(qc); \
	} while(0)


#define GetSequence(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 22)) >> 4)

#define GetFragNum(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 22)) & 0x0f)

#define GetTupleCache(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 22)))

#define SetFragNum(pbuf, num) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 22) = \
			((*(unsigned short *)((unsigned int)(pbuf) + 22)) & le16_to_cpu(~(0x000f))) | \
			cpu_to_le16(0x0f & (num));     \
	} while(0)

#define SetSeqNum(pbuf, num) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 22) = \
			((*(unsigned short *)((unsigned int)(pbuf) + 22)) & le16_to_cpu((unsigned short)~0xfff0)) | \
			le16_to_cpu((unsigned short)(0xfff0 & (num << 4))); \
	} while(0)

#define SetDuration(pbuf, dur) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 2) |= cpu_to_le16(0xffff & (dur)); \
	} while(0)


#define SetPriority(pbuf, tid)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(tid & 0xf); \
	} while(0)

#define GetPriority(pbuf)	((le16_to_cpu(*(unsigned short *)(pbuf))) & 0xf)

#define SetAckpolicy(pbuf, ack)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16( (ack & 3) << 5); \
	} while(0)

#define GetAckpolicy(pbuf) (((le16_to_cpu(*(unsigned short *)pbuf)) >> 5) & 0x3)


#define GetAid(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 2)) & 0x3fff)

#define GetTid(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + (((GetToDs(pbuf)<<1)|GetFrDs(pbuf))==3?30:24))) & 0x000f)

#define GetAddr1Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 4))

#define GetAddr2Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 10))

#define GetAddr3Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 16))

#define GetAddr4Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 24))



static __inline int IS_MCAST(unsigned char *da)
{
	if ((*da) & 0x01)
		return _TRUE;
	else
		return _FALSE;
}


static __inline unsigned char * get_da(unsigned char *pframe)
{
	unsigned char 	*da;
	unsigned int	to_fr_ds	= (GetToDs(pframe) << 1) | GetFrDs(pframe);

	switch (to_fr_ds) {
		case 0x00:	// ToDs=0, FromDs=0
			da = GetAddr1Ptr(pframe);
			break;
		case 0x01:	// ToDs=0, FromDs=1
			da = GetAddr1Ptr(pframe);
			break;
		case 0x02:	// ToDs=1, FromDs=0
			da = GetAddr3Ptr(pframe);
			break;
		default:	// ToDs=1, FromDs=1
			da = GetAddr3Ptr(pframe);
			break;
	}

	return da;
}


static __inline unsigned char * get_sa(unsigned char *pframe)
{
	unsigned char 	*sa;
	unsigned int	to_fr_ds	= (GetToDs(pframe) << 1) | GetFrDs(pframe);

	switch (to_fr_ds) {
		case 0x00:	// ToDs=0, FromDs=0
			sa = GetAddr2Ptr(pframe);
			break;
		case 0x01:	// ToDs=0, FromDs=1
			sa = GetAddr3Ptr(pframe);
			break;
		case 0x02:	// ToDs=1, FromDs=0
			sa = GetAddr2Ptr(pframe);
			break;
		default:	// ToDs=1, FromDs=1
			sa = GetAddr4Ptr(pframe);
			break;
	}

	return sa;
}

static __inline unsigned char * get_hdr_bssid(unsigned char *pframe)
{
	unsigned char 	*sa;
	unsigned int	to_fr_ds	= (GetToDs(pframe) << 1) | GetFrDs(pframe);

	switch (to_fr_ds) {
		case 0x00:	// ToDs=0, FromDs=0
			sa = GetAddr3Ptr(pframe);
			break;
		case 0x01:	// ToDs=0, FromDs=1
			sa = GetAddr2Ptr(pframe);
			break;
		case 0x02:	// ToDs=1, FromDs=0
			sa = GetAddr1Ptr(pframe);
			break;
		default:	// ToDs=1, FromDs=1
			sa =NULL; //???????
			break;
	}

	return sa;
}







/*-----------------------------------------------------------------------------
			Below is for the security related definition
------------------------------------------------------------------------------*/
#define _RESERVED_FRAME_TYPE_	0
#define _SKB_FRAME_TYPE_		2
#define _PRE_ALLOCMEM_			1
#define _PRE_ALLOCHDR_			3
#define _PRE_ALLOCLLCHDR_		4
#define _PRE_ALLOCICVHDR_		5
#define _PRE_ALLOCMICHDR_		6

#define _SIFSTIME_				((priv->pmib->dot11BssType.net_work_type&WIRELESS_11A)?16:10)
#define _ACKCTSLNG_				14	//14 bytes long, including crclng
#define _CRCLNG_				4

#define _ASOCREQ_IE_OFFSET_		4	// excluding wlan_hdr
#define	_ASOCRSP_IE_OFFSET_		6
#define _REASOCREQ_IE_OFFSET_	10
#define _REASOCRSP_IE_OFFSET_	6
#define _PROBEREQ_IE_OFFSET_	0
#define	_PROBERSP_IE_OFFSET_	12
#define _AUTH_IE_OFFSET_		6
#define _DEAUTH_IE_OFFSET_		0
#define _BEACON_IE_OFFSET_		12

#define _SSID_IE_				0
#define _SUPPORTEDRATES_IE_	1
#define _DSSET_IE_				3
#define _TIM_IE_					5
#define _IBSS_PARA_IE_			6
#define _QBSSLOAD_IE_			11
#define _EDCASETS_IE_			12
#define _TS_IE_					13
#define _TCLASS_IE_				14
#define _SCHEDULE_IE_			15
#define _TSDELAY_IE_			43
#define _TCLASSPROCESSING_IE_	44
#define _QOS_CAPABILITY_IE_		46

#define _CHLGETXT_IE_			16
#define _RSN_IE_2_				48
#define _RSN_IE_1_				221
#define _ERPINFO_IE_			42
#define _EXT_SUPPORTEDRATES_IE_	50
#define   _HTCapability_IE  45
#define  _HTInfo_IE  61

#define	_RESERVED47_			47

/* ---------------------------------------------------------------------------
					Below is the fixed elements...
-----------------------------------------------------------------------------*/
#define _AUTH_ALGM_NUM_			2
#define _AUTH_SEQ_NUM_			2
#define _BEACON_ITERVAL_		2
#define _CAPABILITY_			2
#define _CURRENT_APADDR_		6
#define _LISTEN_INTERVAL_		2
#define _RSON_CODE_				2
#define _ASOC_ID_				2
#define _STATUS_CODE_			2
#define _TIMESTAMP_				8

#define AUTH_ODD_TO				0
#define AUTH_EVEN_TO			1

#define WLAN_ETHCONV_ENCAP		1
#define WLAN_ETHCONV_RFC1042	2
#define WLAN_ETHCONV_8021h		3

#define cap_ESS BIT(0)
#define cap_IBSS BIT(1)
#define cap_CFPollable BIT(2)
#define cap_CFRequest BIT(3)
#define cap_Privacy BIT(4)
#define cap_ShortPremble BIT(5)

/*-----------------------------------------------------------------------------
				Below is the definition for 802.11i / 802.1x
------------------------------------------------------------------------------*/
#define _IEEE8021X_MGT_			1		// WPA
#define _IEEE8021X_PSK_			2		// WPA with pre-shared key
/*
#define _NO_PRIVACY_			0
#define _WEP_40_PRIVACY_		1
#define _TKIP_PRIVACY_			2
#define _WRAP_PRIVACY_			3
#define _CCMP_PRIVACY_			4
#define _WEP_104_PRIVACY_		5
#define _WEP_WPA_MIXED_PRIVACY_ 6	// WEP + WPA
*/
//#ifdef PLATFORM_WINDOWS 
typedef struct _NDIS_802_11_AUTHENTICATION_ENCRYPTION
				{
					NDIS_802_11_AUTHENTICATION_MODE AuthModeSupported;
					NDIS_802_11_ENCRYPTION_STATUS EncryptStatusSupported;
				} NDIS_802_11_AUTHENTICATION_ENCRYPTION, *PNDIS_802_11_AUTHENTICATION_ENCRYPTION;
			 	typedef struct _NDIS_802_11_CAPABILITY {
					ULONG  Length;
					ULONG  Version;
					ULONG  NoOfPMKIDs;
					ULONG  NoOfAuthEncryptPairsSupported;
					NDIS_802_11_AUTHENTICATION_ENCRYPTION AuthenticationEncryptionSupported[1];
			 	} NDIS_802_11_CAPABILITY, *PNDIS_802_11_CAPABILITY;


/*-----------------------------------------------------------------------------
				Below is the definition for WMM
------------------------------------------------------------------------------*/
#define _WMM_IE_Length_				7  // for WMM STA
#define _WMM_Para_Element_Length_	24
				
#endif // _WIFI_H_

