
#ifndef _WLAN_MLME_H_
#define _WLAN_MLME_H_

#ifndef __ASSEMBLY__

#define 	TOTAL_TXBA_NUM	16
#define	TOTAL_RXBA_NUM	16

#define	BA_SETUP_TIMEOUT	200
#define	BA_INACT_TIMEOUT	60000

#define	BA_POLICY_DELAYED		0
#define	BA_POLICY_IMMEDIATE	1

#define	ADDBA_STATUS_SUCCESS			0
#define	ADDBA_STATUS_REFUSED		37
#define	ADDBA_STATUS_INVALID_PARAM	38

#define	DELBA_REASON_QSTA_LEAVING	36
#define	DELBA_REASON_END_BA			37
#define	DELBA_REASON_UNKNOWN_BA	38
#define	DELBA_REASON_TIMEOUT			39


// This define the Tx/Rx directions
typedef enum _TR_SELECT {
	TX_DIR = 0, 
	RX_DIR = 1,
} TR_SELECT, *PTR_SELECT;

typedef union _DELBA_PARAM_SET {
	u1Byte charData[2];
	u2Byte shortData;

	struct {
		u2Byte Reserved:11;
		u2Byte Initiator:1;
		u2Byte TID:4;
	} field;
} DELBA_PARAM_SET, *PDELBA_PARAM_SET;

#ifndef _WLAN_MLME_C_

#include <linux/version.h>

extern void site_survey(_adapter *padapter);
extern void survey_timer_hdl(PVOID FunctionContext);
extern void reauth_timer_hdl(PVOID FunctionContext);
extern void reassoc_timer_hdl(PVOID FunctionContext);
extern void disconnect_timer_hdl(PVOID FunctionContext);
extern void mgt_init(void);
extern void issue_probereq(_adapter *padapter);
extern void issue_deauth(_adapter *padapter, unsigned char *da, int reason);
extern void issue_disassoc(_adapter *padapter, unsigned char *da, int reason);
extern void issue_auth(_adapter* padapter, struct sta_data *pstat, unsigned short status);
extern void issue_asocrsp(_adapter *padapter, unsigned short status, struct sta_data *pstat, int pkt_type);
extern void issue_probersp(_adapter *padapter, unsigned char *da, int set_privacy);
extern unsigned int issue_assocreq(_adapter *padapter);
extern void start_clnt_join(_adapter *padapter);
extern int setup_bcnframe(_adapter *padapter, unsigned char *pframe);
extern void issue_beacon(_adapter *padapter);
//extern void ActSetWirelessMode8187(_adapter *padapter, u8 btWirelessMode);

extern void get_rx_power_87b(_adapter *padapter, struct recv_stat *desc_87b);

#ifdef CONFIG_RTL8192U
extern void TsInitAddBA(PVOID	FunctionContext);
extern void BaSetupTimeOut(PVOID	FunctionContext);
extern void TxBaInactTimeout(PVOID	FunctionContext);
extern void DeActivateBAEntry(PADAPTER Adapter, PBABODY pBA);
extern void sendbeacon_timer_hdl(PVOID FunctionContext);
#endif /*CONFIG_RTL8192U*/

#ifdef TODO
extern void UpdateInitialGain(_adapter *padapter);

extern void r8187_start_DynamicInitGain(_adapter *padapter);
extern void r8187_stop_DynamicInitGain(_adapter *padapter);
extern void r8187_start_RateAdaptive(_adapter *padapter);
extern void r8187_stop_RateAdaptive(_adapter *padapter);


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
extern void DigWorkItemCallback(_adapter *padapter);
extern void RateAdaptiveWorkItemCallback(_adapter *padapter);
#else
extern void DigWorkItemCallback(struct work_struct *work);
extern void RateAdaptiveWorkItemCallback(struct work_struct *work);
#endif

#endif


#else

void survey_timer_hdl(PVOID FunctionContext);
void reauth_timer_hdl(PVOID FunctionContext);
void reassoc_timer_hdl(PVOID FunctionContext);
void disconnect_timer_hdl(PVOID FunctionContext);
void erp_timer_hdl(unsigned char *pbuf);
void rf_timer_hdl(unsigned char *pbuf);

void issue_probereq(_adapter *padapter);
void issue_deauth(_adapter *padapter, unsigned char *da, int reason);
void issue_disassoc(_adapter *padapter, unsigned char *da, int reason);
void issue_auth(_adapter* padapter, struct sta_data *pstat, unsigned short status);
void issue_asocrsp(_adapter *padapter, unsigned short status, struct sta_data *pstat, int pkt_type);
void issue_probersp(_adapter *padapter, unsigned char *da, int set_privacy);
unsigned int issue_assocreq(_adapter *padapter);

#endif


extern void HTSetConnectBwMode(_adapter *padapter, HT_CHANNEL_WIDTH Bandwidth, HT_EXTCHNL_OFFSET Offset);
extern void ActSetWirelessMode819xUsb(_adapter *padapter, u8 btWirelessMode);
extern void Hal_SetHwReg819xUsb(IN PADAPTER    Adapter, IN u8 variable, IN u8 * val);
extern void PHY_SetRFReg(_adapter *padapter, u8 eRFPath, u32 RegAddr, u32 BitMask , u32 Data);
extern void PHY_SetBBReg(_adapter *padapter,u32 RegAddr, u32 BitMask , u32 Data);
extern void SetHalRATRTable819xUsb(PADAPTER		Adapter, u8	*posLegacyRate,u8			*pMcsRate);
extern void SetBeaconRelatedRegisters819xUsb(_adapter *padapter, u16 atimwindow);


#endif

#endif // _WLAN_MGT_H_





