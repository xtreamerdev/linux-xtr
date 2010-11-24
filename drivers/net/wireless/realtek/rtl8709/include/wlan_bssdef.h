

#ifndef __WLAN_BSSDEF_H__
#define __WLAN_BSSDEF_H__
#ifdef CONFIG_RTL8711
#include <rtl8711_spec.h>
#endif

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
//#include <rtl8187_spec.h>
//#include <drv_types.h>
#include <drv_conf.h>
#include <osdep_service.h>
#endif
#define MAX_IE_SZ	256


#ifdef PLATFORM_LINUX

#define NDIS_802_11_LENGTH_SSID         32
#define NDIS_802_11_LENGTH_RATES        8
#define NDIS_802_11_LENGTH_RATES_EX     16

typedef unsigned char   NDIS_802_11_MAC_ADDRESS[6];
typedef long    		NDIS_802_11_RSSI;           // in dBm
typedef unsigned char   NDIS_802_11_RATES[NDIS_802_11_LENGTH_RATES];        // Set of 8 data rates
typedef unsigned char   NDIS_802_11_RATES_EX[NDIS_802_11_LENGTH_RATES_EX];  // Set of 16 data rates


typedef  ULONG  NDIS_802_11_KEY_INDEX;
typedef unsigned long long NDIS_802_11_KEY_RSC;


typedef struct _NDIS_802_11_SSID
{
  ULONG  SsidLength;
  UCHAR  Ssid[32];
} NDIS_802_11_SSID, *PNDIS_802_11_SSID;

typedef enum _NDIS_802_11_NETWORK_TYPE
{
    Ndis802_11FH,
    Ndis802_11DS,
    Ndis802_11OFDM5,
    Ndis802_11OFDM24,
    Ndis802_11NetworkTypeMax    // not a real type, defined as an upper bound
} NDIS_802_11_NETWORK_TYPE, *PNDIS_802_11_NETWORK_TYPE;

typedef struct _NDIS_802_11_CONFIGURATION_FH
{
    ULONG           Length;             // Length of structure
    ULONG           HopPattern;         // As defined by 802.11, MSB set
    ULONG           HopSet;             // to one if non-802.11
    ULONG           DwellTime;          // units are Kusec
} NDIS_802_11_CONFIGURATION_FH, *PNDIS_802_11_CONFIGURATION_FH;
 

/*
	FW will only save the channel number in DSConfig.
	ODI Handler will convert the channel number to freq. number.	
*/
typedef struct _NDIS_802_11_CONFIGURATION
{
    ULONG           Length;             // Length of structure
    ULONG           BeaconPeriod;       // units are Kusec
    ULONG           ATIMWindow;         // units are Kusec
    ULONG           DSConfig;           // Frequency, units are kHz
    NDIS_802_11_CONFIGURATION_FH    FHConfig;
} NDIS_802_11_CONFIGURATION, *PNDIS_802_11_CONFIGURATION;



typedef enum _NDIS_802_11_NETWORK_INFRASTRUCTURE
{
    Ndis802_11IBSS,
    Ndis802_11Infrastructure,
    Ndis802_11AutoUnknown,
    Ndis802_11InfrastructureMax         // Not a real value, defined as upper bound
} NDIS_802_11_NETWORK_INFRASTRUCTURE, *PNDIS_802_11_NETWORK_INFRASTRUCTURE;





typedef struct _NDIS_802_11_FIXED_IEs
{
  UCHAR  Timestamp[8];
  USHORT  BeaconInterval;
  USHORT  Capabilities;
} NDIS_802_11_FIXED_IEs, *PNDIS_802_11_FIXED_IEs;



typedef struct _NDIS_802_11_VARIABLE_IEs
{
  UCHAR  ElementID;
  UCHAR  Length;
  UCHAR  data[1];
} NDIS_802_11_VARIABLE_IEs, *PNDIS_802_11_VARIABLE_IEs;



/*



Length is the 4 bytes multiples of the sume of
	sizeof (NDIS_802_11_MAC_ADDRESS) + 2 + sizeof (NDIS_802_11_SSID) + sizeof (ULONG)
+   sizeof (NDIS_802_11_RSSI) + sizeof (NDIS_802_11_NETWORK_TYPE) + sizeof (NDIS_802_11_CONFIGURATION)
+   sizeof (NDIS_802_11_RATES_EX) + IELength

Except the IELength, all other fields are fixed length. Therefore, we can define a marco to present the
partial sum.

*/

#define MAX_IES_LEN	192
#define QOS_DISABLE			0
#define QOS_WMM			1
#define QOS_WMMSA			2
#define QOS_EDCA			4
#define QOS_HCCA			8
#define QOS_WMM_UAPSD		16   //WMM Power Save, 2006-06-14 Isaiah 

//
// ECWmin/ECWmax field.
// Ref: WMM spec 2.2.2: WME Parameter Element, p.13.
//
typedef	union _ECW{
	u8	charData;
	struct
	{
		u8	ECWmin:4;
		u8	ECWmax:4;
	}f;	// Field
}ECW, *PECW;


//
// ACI/AIFSN Field.
// Ref: WMM spec 2.2.2: WME Parameter Element, p.12.
//
typedef	union _ACI_AIFSN{
	u8	charData;
	
	struct
	{
		u8	AIFSN:4;
		u8	ACM:1;
		u8	ACI:2;
		u8	Reserved:1;
	}f;	// Field
}ACI_AIFSN, *PACI_AIFSN;

//
// AC Parameters Record Format.
// Ref: WMM spec 2.2.2: WME Parameter Element, p.12.
//
typedef	union _AC_PARAM{
	u32	longData;
	u8	charData[4];

	struct
	{
		ACI_AIFSN	AciAifsn;
		ECW		Ecw;
		u16		TXOPLimit;
	}f;	// Field
}AC_PARAM, *PAC_PARAM;


typedef	union _QOS_INFO_FIELD{
	u1Byte	charData;
	
	struct
	{
		u1Byte		ucParameterSetCount:4;
		u1Byte		ucReserved:4;	
	}WMM;
}QOS_INFO_FIELD, *PQOS_INFO_FIELD;

typedef struct _BSS_QOS{

	u32				bdQoSMode;

	QOS_INFO_FIELD			QosInfoField;
	AC_PARAM				AcParameter[4];

}BSS_QOS, *PBSS_QOS;

typedef struct _BSS_HT{

	BOOLEAN				bdSupportHT;

	// HT related elements
	u1Byte					bdHTCapBuf[32];
	u2Byte					bdHTCapLen;
	u1Byte					bdHTInfoBuf[32];
	u2Byte					bdHTInfoLen;

	BOOLEAN					bdRT2RTAggregation;
	BOOLEAN					bdRT2RTLongSlotTime;
}BSS_HT, *PBSS_HT;


typedef struct _NDIS_WLAN_BSSID_EX
{
  ULONG  Length;
  NDIS_802_11_MAC_ADDRESS  MacAddress;
  UCHAR  Reserved[2];
  NDIS_802_11_SSID  Ssid;
  ULONG  Privacy;
  NDIS_802_11_RSSI  Rssi;
  NDIS_802_11_NETWORK_TYPE  NetworkTypeInUse;
  NDIS_802_11_CONFIGURATION  Configuration;
  NDIS_802_11_NETWORK_INFRASTRUCTURE  InfrastructureMode;
  NDIS_802_11_RATES_EX  SupportedRates;

  // For HT related Info
  #ifdef CONFIG_RTL8192U
  BSS_HT						BssHT;
  BSS_QOS					BssQos;
  u8 			SlotTime;
  #endif

  ULONG  IELength;
  UCHAR  IEs[192];	//(timestamp, beacon interval, and capability information)
} NDIS_WLAN_BSSID_EX, *PNDIS_WLAN_BSSID_EX;


typedef struct _NDIS_802_11_BSSID_LIST_EX
{
  ULONG  NumberOfItems;
  NDIS_WLAN_BSSID_EX  Bssid[1];
} NDIS_802_11_BSSID_LIST_EX, *PNDIS_802_11_BSSID_LIST_EX;

#if 1
typedef enum _NDIS_802_11_AUTHENTICATION_MODE
{
    Ndis802_11AuthModeOpen,
    Ndis802_11AuthModeShared,
    Ndis802_11AuthModeAutoSwitch,
    Ndis802_11AuthModeWPA,
    Ndis802_11AuthModeWPAPSK,
    Ndis802_11AuthModeWPANone,
    Ndis802_11AuthModeMax               // Not a real mode, defined as upper bound
} NDIS_802_11_AUTHENTICATION_MODE, *PNDIS_802_11_AUTHENTICATION_MODE;

typedef enum _NDIS_802_11_WEP_STATUS
{
    Ndis802_11WEPEnabled,
    Ndis802_11Encryption1Enabled = Ndis802_11WEPEnabled,
    Ndis802_11WEPDisabled,
    Ndis802_11EncryptionDisabled = Ndis802_11WEPDisabled,
    Ndis802_11WEPKeyAbsent,
    Ndis802_11Encryption1KeyAbsent = Ndis802_11WEPKeyAbsent,
    Ndis802_11WEPNotSupported,
    Ndis802_11EncryptionNotSupported = Ndis802_11WEPNotSupported,
    Ndis802_11Encryption2Enabled,
    Ndis802_11Encryption2KeyAbsent,
    Ndis802_11Encryption3Enabled,
    Ndis802_11Encryption3KeyAbsent
} NDIS_802_11_WEP_STATUS, *PNDIS_802_11_WEP_STATUS,
  NDIS_802_11_ENCRYPTION_STATUS, *PNDIS_802_11_ENCRYPTION_STATUS;


#define NDIS_802_11_AI_REQFI_CAPABILITIES      1
#define NDIS_802_11_AI_REQFI_LISTENINTERVAL    2
#define NDIS_802_11_AI_REQFI_CURRENTAPADDRESS  4

#define NDIS_802_11_AI_RESFI_CAPABILITIES      1
#define NDIS_802_11_AI_RESFI_STATUSCODE        2
#define NDIS_802_11_AI_RESFI_ASSOCIATIONID     4

typedef struct _NDIS_802_11_AI_REQFI
{
    USHORT Capabilities;
    USHORT ListenInterval;
    NDIS_802_11_MAC_ADDRESS  CurrentAPAddress;
} NDIS_802_11_AI_REQFI, *PNDIS_802_11_AI_REQFI;

typedef struct _NDIS_802_11_AI_RESFI
{
    USHORT Capabilities;
    USHORT StatusCode;
    USHORT AssociationId;
} NDIS_802_11_AI_RESFI, *PNDIS_802_11_AI_RESFI;

typedef struct _NDIS_802_11_ASSOCIATION_INFORMATION
{
    ULONG                   Length;
    USHORT                  AvailableRequestFixedIEs;
    NDIS_802_11_AI_REQFI    RequestFixedIEs;
    ULONG                   RequestIELength;
    ULONG                   OffsetRequestIEs;
    USHORT                  AvailableResponseFixedIEs;
    NDIS_802_11_AI_RESFI    ResponseFixedIEs;
    ULONG                   ResponseIELength;
    ULONG                   OffsetResponseIEs;
} NDIS_802_11_ASSOCIATION_INFORMATION, *PNDIS_802_11_ASSOCIATION_INFORMATION;

typedef enum _NDIS_802_11_RELOAD_DEFAULTS
{
   Ndis802_11ReloadWEPKeys
} NDIS_802_11_RELOAD_DEFAULTS, *PNDIS_802_11_RELOAD_DEFAULTS;


// Key mapping keys require a BSSID
typedef struct _NDIS_802_11_KEY
{
    ULONG           Length;             // Length of this structure
    ULONG           KeyIndex;           
    ULONG           KeyLength;          // length of key in bytes
    NDIS_802_11_MAC_ADDRESS BSSID;
    NDIS_802_11_KEY_RSC KeyRSC;
    UCHAR           KeyMaterial[1];     // variable length depending on above field
} NDIS_802_11_KEY, *PNDIS_802_11_KEY;

typedef struct _NDIS_802_11_REMOVE_KEY
{
    ULONG                   Length;        // Length of this structure
    ULONG                   KeyIndex;           
    NDIS_802_11_MAC_ADDRESS BSSID;      
} NDIS_802_11_REMOVE_KEY, *PNDIS_802_11_REMOVE_KEY;

typedef struct _NDIS_802_11_WEP
{
    ULONG     Length;        // Length of this structure
    ULONG     KeyIndex;      // 0 is the per-client key, 1-N are the global keys
    ULONG     KeyLength;     // length of key in bytes
    UCHAR     KeyMaterial[1];// variable length depending on above field
} NDIS_802_11_WEP, *PNDIS_802_11_WEP;

typedef struct _NDIS_802_11_AUTHENTICATION_REQUEST
{
    ULONG Length;            // Length of structure
    NDIS_802_11_MAC_ADDRESS Bssid;
    ULONG Flags;
} NDIS_802_11_AUTHENTICATION_REQUEST, *PNDIS_802_11_AUTHENTICATION_REQUEST;

typedef enum _NDIS_802_11_STATUS_TYPE
{
	Ndis802_11StatusType_Authentication,
	Ndis802_11StatusType_MediaStreamMode,
	Ndis802_11StatusType_PMKID_CandidateList,		
	Ndis802_11StatusTypeMax    // not a real type, defined as an upper bound
} NDIS_802_11_STATUS_TYPE, *PNDIS_802_11_STATUS_TYPE;

typedef struct _NDIS_802_11_STATUS_INDICATION
{
    NDIS_802_11_STATUS_TYPE StatusType;
} NDIS_802_11_STATUS_INDICATION, *PNDIS_802_11_STATUS_INDICATION;

// mask for authentication/integrity fields
#define NDIS_802_11_AUTH_REQUEST_AUTH_FIELDS        0x0f
#define NDIS_802_11_AUTH_REQUEST_REAUTH			0x01
#define NDIS_802_11_AUTH_REQUEST_KEYUPDATE		0x02
#define NDIS_802_11_AUTH_REQUEST_PAIRWISE_ERROR		0x06
#define NDIS_802_11_AUTH_REQUEST_GROUP_ERROR		0x0E

// MIC check time, 60 seconds.
#define MIC_CHECK_TIME	60000000

typedef struct _NDIS_802_11_AUTHENTICATION_EVENT
{
    NDIS_802_11_STATUS_INDICATION       Status;
    NDIS_802_11_AUTHENTICATION_REQUEST  Request[1];
} NDIS_802_11_AUTHENTICATION_EVENT, *PNDIS_802_11_AUTHENTICATION_EVENT;
        
typedef struct _NDIS_802_11_TEST
{
    ULONG Length;
    ULONG Type;
    union
    {
        NDIS_802_11_AUTHENTICATION_EVENT AuthenticationEvent;
        NDIS_802_11_RSSI RssiTrigger;
    }tt;
} NDIS_802_11_TEST, *PNDIS_802_11_TEST;



#endif


#endif



struct	wlan_network {
	_list	list;	
	int	network_type;	//refer to ieee80211.h for WIRELESS_11A/B/G
	int	fixed;			// set to fixed when not to be removed as site-surveying
	unsigned long	last_scanned;
	int	aid;			//will only be valid when a BSS is joinned.
	int	join_res;
	NDIS_WLAN_BSSID_EX	network; //must be the last item
	unsigned char  iebuf[MAX_IE_SZ];
};


struct	joinned_network {
	struct	wlan_network	network;
	
};

enum VRTL_CARRIER_SENSE
{
    DISABLE_VCS,	
    ENABLE_VCS,	
    AUTO_VCS
};

enum VCS_TYPE
{
    NONE_VCS,	
    RTS_CTS,
    CTS_TO_SELF 
};




#define PWR_CAM 0
#define PWR_MINPS 1
#define PWR_MAXPS 2
#define PWR_UAPSD 3
#define PWR_VOIP 4


enum UAPSD_MAX_SP
{
	NO_LIMIT,
       TWO_MSDU,
       FOUR_MSDU,
       SIX_MSDU
};








#endif //#ifndef WLAN_BSSDEF_H_




