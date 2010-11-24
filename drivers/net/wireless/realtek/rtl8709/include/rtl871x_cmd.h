#ifndef __RTL871X_CMD_H_
#define __RTL871X_CMD_H_
#include <drv_conf.h>
#include <rtl8711_spec.h>
#include <wlan_bssdef.h>
#include <rtl871x_rf.h>
#include <rtl871x_event.h>
#include <rtl8187/rtl8187_event.h>
#ifndef CONFIG_RTL8711FW

	#include <osdep_service.h>
	#include <ieee80211.h> // <ieee80211/ieee80211.h>


	#define FREE_CMDOBJ_SZ	128
	
	#define MAX_CMDSZ	512
	#define MAX_RSPSZ	512
	#define MAX_EVTSZ	512

	struct cmd_obj {
		u16	cmdcode;
		u8	res;
		u8	*parmbuf;
		u32	cmdsz;
		u8	*rsp;
		u32	rspsz;
		//_sema 	cmd_sem;
		_list	list;
	};


	struct	cmd_priv	{
		_sema	cmd_queue_sema;
		_sema	cmd_done_sema;
		_sema	terminate_cmdthread_sema;		
		_queue	cmd_queue;
		u8	cmd_seq;
		u8	*cmd_buf;	//shall be non-paged, and 4 bytes aligned
		u8	*cmd_allocated_buf;
		u8	*rsp_buf;	//shall be non-paged, and 4 bytes aligned		
		u8	*rsp_allocated_buf;
		u32	cmd_issued_cnt;
		u32	cmd_done_cnt;
		u32	rsp_cnt;
		_adapter *padapter;
		
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
		_sema	cmd_proc_sema;
		_queue	cmd_processing_queue;
#endif

	};

struct event_frame_priv{
	_list 	list;
	u32 	c2h_res;
	u8 	*c2h_buf;
};


struct	evt_priv	{
	_sema	evt_notify;
	_sema	terminate_evtthread_sema;	

#ifdef CONFIG_H2CLBK
	_sema	lbkevt_done;
	u8 lbkevt_limit;
	u8 lbkevt_num;
	u8* cmdevt_parm;
#endif		

	u8	event_seq;
	u8	*evt_buf;	//shall be non-paged, and 4 bytes aligned		
	u8	*evt_allocated_buf;
	u32	evt_done_cnt;

	//john debug 0520
	u8 	*event_frame_buf;
	u8 	*event_frame_allocated_buf;
	_queue 	free_event_queue;
	_queue 	pending_event_queue;
};



#define init_h2fwcmd_w_parm_no_rsp(pcmd, pparm, code) \
do {\
	_init_listhead(&pcmd->list);\
	pcmd->cmdcode = code;\
	pcmd->parmbuf = (u8 *)(pparm);\
	pcmd->cmdsz = sizeof (*pparm);\
	pcmd->rsp = NULL;\
	pcmd->rspsz = 0;\
} while(0)



//extern  void init_sema(_sema	*sema, int init_val);	// to initialize the sema with init_val
//extern	void up_sema(_sema	*sema);
//extern  int down_sema(_sema *sema);

extern	u32	enqueue_cmd(struct cmd_priv *pcmdpriv, struct cmd_obj *obj);
extern	struct	cmd_obj	*dequeue_cmd(_queue *queue);

extern void free_cmd_obj(struct cmd_obj *pcmd);


extern thread_return cmd_thread(void *context);

extern u32 cmd_enqueue(_queue *cmdq,struct cmd_obj	*pcmd);

extern u32	init_cmd_priv (struct	cmd_priv *pcmdpriv);
extern void free_cmd_priv (struct	cmd_priv *pcmdpriv);

extern u32	init_evt_priv (struct	evt_priv *pevtpriv);
extern void free_evt_priv (struct	evt_priv *pevtpriv);
extern void cmd_clr_isr(struct	cmd_priv *pcmdpriv);
extern void evt_notify_isr(struct evt_priv *pevtpriv);



#else
	#include <ieee80211.h>
#endif

enum RFINTFS {
	SWSI,
	HWSI,
	HWPI,
};



/*
Caller Mode: Infra, Ad-HoC(C)

Notes: To enter USB suspend mode

Command Mode

*/
struct usb_suspend_parm {
	u32 rsvd;
	
	
};



/*
Caller Mode: Infra, Ad-HoC

Notes: To join a known BSS.

Command-Event Mode

*/


/*

Caller Mode: Infra, Ad-Hoc

Notes: To join the specified bss

Command Event Mode

*/

struct joinbss_parm	{
	NDIS_WLAN_BSSID_EX network;
};

/*
Caller Mode: Infra, Ad-HoC(C)

Notes: To disconnect the current associated BSS

Command Mode

*/
struct disconnect_parm {
	u32 rsvd;
	
	
};

/*
Caller Mode: AP, Ad-HoC(M)

Notes: To create a BSS

Command Mode
*/
struct createbss_parm {
	NDIS_WLAN_BSSID_EX network;
};

/*
Caller Mode: AP, Ad-HoC, Infra

Notes: To set the NIC mode of RTL8711

Command Mode

The definition of mode:

#define IW_MODE_AUTO	0	// Let the driver decides which AP to join
#define IW_MODE_ADHOC	1	// Single cell network (Ad-Hoc Clients)
#define IW_MODE_INFRA	2	// Multi cell network, roaming, ..
#define IW_MODE_MASTER	3	// Synchronisation master or Access Point
#define IW_MODE_REPEAT	4	// Wireless Repeater (forwarder)
#define IW_MODE_SECOND	5	// Secondary master/repeater (backup)
#define IW_MODE_MONITOR	6	// Passive monitor (listen only)

*/
struct	setopmode_parm {
	u8	mode;
	u8	rsvd[3];
};

/*
Caller Mode: AP, Ad-HoC, Infra

Notes: To ask RTL8711 performing site-survey

Command-Event Mode 
*/
struct sitesurvey_parm {
	sint passive_mode;	//active: 1, passive: 0 
	sint bsslimit;	// 1 ~ 48
	sint	ss_ssidlen;
	u8 	ss_ssid[IW_ESSID_MAX_SIZE + 1];
};

/*
Caller Mode: Any

Notes: To set the auth type of RTL8711. open/shared/802.1x

Command Mode
*/
struct setauth_parm {
	u8 mode;  //0: legacy open, 1: legacy shared 2: 802.1x
	u8 _1x;   //0: PSK, 1: TLS
	u8 rsvd[2];
};



/*

Caller Mode: Infra

a. algorithm: wep40, wep104, tkip & aes
b. keytype: grp key/unicast key
c. key contents

when shared key ==> keyid is the camid
when 802.1x ==> keyid [0:1] ==> grp key
when 802.1x ==> keyid > 2 ==> unicast key

*/

struct setkey_parm {
	u8	algorithm;	// encryption algorithm, could be none, wep40, TKIP, CCMP, wep104
	u8	keyid;		
	u8 	grpkey;		// 1: this is the grpkey for 802.1x. 0: this is the unicast key for 802.1x
	u8	key[16];	// this could be 40 or 104
};


/*
When in AP or Ad-Hoc mode, this is used to 
allocate an sw/hw entry for a newly associated sta.

Command

when shared key ==> algorithm/keyid 

*/
struct set_stakey_parm	{
	
	u8  	addr[ETH_ALEN];
	u8	algorithm;
	u8	key[16];
};

struct set_stakey_rsp
{
    u8   addr[ETH_ALEN];
    u8   keyid;
    u8    rsvd;
};


/*

Caller Ad-Hoc/AP

Command -Rsp(AID == CAMID) mode

This is to force fw to add an sta_data entry per driver's request.

FW will write an cam entry associated with it.

*/
struct set_assocsta_parm {
	u8  	addr[ETH_ALEN];
};


struct set_assocsta_rsp	{
	u8	cam_id;
	u8 rsvd[3];
};

/*
	Caller Ad-Hoc/AP
	
	Command mode
	
	This is to force fw to del an sta_data entry per driver's request
	
	FW will invalidate the cam entry associated with it.

*/

struct del_assocsta_parm {
	u8  	addr[ETH_ALEN];
};





/*
Caller Mode: AP/Ad-HoC(M)

Notes: To notify fw that given staid has changed its power state

Command Mode
*/
struct setstapwrstate_parm {
	u8	staid;
	u8	status;
	u8	hwaddr[6];
};

/*
Caller Mode: Any

Notes: To setup the basic rate of RTL8711

Command Mode

*/
struct	setbasicrate_parm {
	u8	basicrates[NumRates];
};

/*
Caller Mode: Any

Notes: To read the current basic rate

Command-Rsp Mode

*/
struct getbasicrate_parm {
	u32 rsvd;
	
};
struct getbasicrate_rsp {
	u8 basicrates[NumRates];
};


/*
Caller Mode: Any

Notes: To setup the data rate of RTL8711

Command Mode

*/
struct	setdatarate_parm {
	u8	mac_id;
	u8	datarates[NumRates];
};


/*
Caller Mode: Any

Notes: To read the current data rate

Command-Rsp Mode

*/
struct getdatarate_parm {
	u32 rsvd;
	
};
struct getdatarate_rsp {
	u8 datarates[NumRates];
};


/*
Caller Mode: Any
AP: AP can use the info for the contents of beacon frame
Infra: STA can use the info when sitesurveying
Ad-HoC(M): Like AP
Ad-HoC(C): Like STA


Notes: To set the phy capability of the NIC

Command Mode

*/


struct	setphyinfo_parm {
	struct regulatory_class class_sets[NUM_REGULATORYS];
	//u8	status;
};



struct	getphyinfo_parm {
	u32 rsvd;
};


struct	getphyinfo_rsp {
	struct regulatory_class class_sets[NUM_REGULATORYS];
	u8	status;
};


/*
Caller Mode: Any

Notes: To set the channel/modem/band
This command will be used when channel/modem/band is changed.

Command Mode

*/
struct	setphy_parm {
	u8	rfchannel;
	u8	modem;
};

/*
Caller Mode: Any

Notes: To get the current setting of channel/modem/band

Command-Rsp Mode



*/
struct	getphy_parm {
	u32 rsvd;

};
struct	getphy_rsp {
	u8	rfchannel;
	u8	modem;
};


struct readBB_parm
{
	u8	offset;
};
struct readBB_rsp
{
	u8	value;
};

struct writeBB_parm
{
	u8	offset;
	u8	value;
};

struct readRF_parm
{
	u8	offset;
};
struct readRF_rsp
{
	u32	value;
};

struct writeRF_parm
{
	u8	offset;
	u32	value;
};


struct setrfintfs_parm {
	u8	rfintfs;
};

struct getrfintfs_parm {
	u8	rfintfs;
};


/*
	Notes: This command is used for H2C/C2H loopback testing

	mac[0] == 0 
	==> CMD mode, return H2C_SUCCESS.
	The following condition must be ture under CMD mode
		mac[1] == mac[4], mac[2] == mac[3], mac[0]=mac[5]= 0;
		s0 == 0x1234, s1 == 0xabcd, w0 == 0x78563412, w1 == 0x5aa5def7;
		s2 == (b1 << 8 | b0);
	
	mac[0] == 1
	==> CMD_RSP mode, return H2C_SUCCESS_RSP
	
	The rsp layout shall be:
	rsp: 			parm:
		mac[0]  =   mac[5];
		mac[1]  =   mac[4];
		mac[2]  =   mac[3];
		mac[3]  =   mac[2];
		mac[4]  =   mac[1];
		mac[5]  =   mac[0];
		s0		=   s1;
		s1		=   swap16(s0);
		w0		=  	swap32(w1);
		b0		= 	b1
		s2		= 	s0 + s1
		b1		= 	b0
		w1		=	w0
		
	mac[0] == 	2
	==> CMD_EVENT mode, return 	H2C_SUCCESS
	The event layout shall be:
	event:			parm:
		mac[0]  =   mac[5];
		mac[1]  =   mac[4];
		mac[2]  =   event's sequence number, starting from 1 to parm's marc[3]
		mac[3]  =   mac[2];
		mac[4]  =   mac[1];
		mac[5]  =   mac[0];
		s0		=   swap16(s0) - event.mac[2];
		s1		=   s1 + event.mac[2];
		w0		=  	swap32(w0);
		b0		= 	b1
		s2		= 	s0 + event.mac[2]
		b1		= 	b0 
		w1		=	swap32(w1) - event.mac[2];	
	
		parm->mac[3] is the total event counts that host requested.
		
	
	event will be the same with the cmd's param.
		
*/

#ifdef CONFIG_H2CLBK

struct seth2clbk_parm {
	u8 mac[6];
	u16	s0;
	u16	s1;
	u32	w0;
	u8	b0;
	u16  s2;
	u8	b1;
	u32	w1;
};


struct geth2clbk_parm {
	u32 rsv;	

};

struct geth2clbk_rsp {
	u8 	mac[6];
	u16	s0;
	u16	s1;
	u32	w0;
	u8	b0;
	u16  s2;
	u8	b1;
	u32	w1;
};

#endif

/*------------------- Below are used for RF/BB tunning ---------------------*/

struct	setantenna_parm {
	u8	tx_antset;		
	u8	rx_antset;
	u8	tx_antenna;		
	u8	rx_antenna;		
};

struct	enrateadaptive_parm {
	u32	en;
};

struct settxagctbl_parm {
	u32	txagc[MAX_RATES_LENGTH];
};

struct gettxagctbl_parm {
	u32 rsvd;
};

struct gettxagctbl_rsp {
	u32	txagc[MAX_RATES_LENGTH];
};

struct setagcctrl_parm {
	u32	agcctrl;		// 0: pure hw, 1: fw
};


struct setssup_parm	{
	u32	ss_ForceUp[MAX_RATES_LENGTH];
};


struct getssup_parm	{
	u32 rsvd;
};

struct getssup_rsp	{
	u8	ss_ForceUp[MAX_RATES_LENGTH];
};


struct setssdlevel_parm	{
	u8	ss_DLevel[MAX_RATES_LENGTH];
};

struct getssdlevel_parm	{
	u32 rsvd;
};

struct getssdlevel_rsp	{
	u8	ss_DLevel[MAX_RATES_LENGTH];
};

struct setssulevel_parm	{
	u8	ss_ULevel[MAX_RATES_LENGTH];
};

struct getssulevel_parm	{
	u32 rsvd;
};

struct getssulevel_rsp	{
	u8	ss_ULevel[MAX_RATES_LENGTH];
};


struct	setcountjudge_parm {
	u8	count_judge[MAX_RATES_LENGTH];
};

struct	getcountjudge_parm {
	u32 rsvd;
};

struct	getcountjudge_rsp {
	u8	count_judge[MAX_RATES_LENGTH];
};


#ifdef CONFIG_PWRCTRL
struct setpwrmode_parm  {
    u8 mode;     // 0: Active, 1: Min, 2: Max, 3: VOIP, 4: UAPSD, 5: UAPSD-WMM, 6: ADHOC, 7:WWLAN
    u8 app_itv;  // only valid for VOIP mode (ms).
    u8 bcn_to;   // beacon timeout (ms).
    u8 bcn_pass_cnt; // fw will only report one beacon information to driver when it receives bcn_pass_cnt beacons.
};

struct setatim_parm {
    u8 op;   // 0: add, 1:del
    u8 txid; // id of dest station.
};
#endif /*CONFIG_PWRCTRL*/

struct setratable_parm {
        u8 ss_ForceUp[NumRates];
        u8 ss_ULevel[NumRates];
        u8 ss_DLevel[NumRates];
        u8 count_judge[NumRates];
};

struct  getratable_parm {
                uint rsvd;
};
 
struct getratable_rsp {
        u8 ss_ForceUp[NumRates];
        u8 ss_ULevel[NumRates];
        u8 ss_DLevel[NumRates];
        u8 count_judge[NumRates];
};


#define H2CSZ	(512/(sizeof(uint)))
#define C2HSZ	(512/(sizeof(uint)))

#ifndef CONFIG_RTL8711FW


#else

struct cmdobj {
	uint	parmsize;
	u8 (*h2cfuns)(u8 *pbuf);	
};

extern u8 joinbss_hdl(_adapter *padapter, u8 *pbuf);	
extern u8 disconnect_hdl(_adapter *padapter, u8 *pbuf);
extern u8 createbss_hdl(_adapter *padapter, u8 *pbuf);
extern u8 setopmode_hdl(_adapter *padapter, u8 *pbuf);
extern u8 sitesurvey_hdl(_adapter *padapter, u8 *pbuf);	
extern u8 setauth_hdl(_adapter *padapter, u8 *pbuf);
extern u8 setkey_hdl(_adapter *padapter, u8 *pbuf);
extern u8 set_stakey_hdl(_adapter *padapter, u8 *pbuf);
extern u8 set_assocsta_hdl(_adapter *padapter, u8 *pbuf);
extern u8 del_assocsta_hdl(_adapter *padapter, u8 *pbuf);
extern u8 setstapwrstate_hdl(_adapter *padapter, u8 *pbuf);
extern u8 setbasicrate_hdl(_adapter *padapter, u8 *pbuf);	
extern u8 getbasicrate_hdl(_adapter *padapter, u8 *pbuf);
extern u8 setdatarate_hdl(_adapter *padapter, u8 *pbuf);
extern u8 getdatarate_hdl(_adapter *padapter, u8 *pbuf);
extern u8 setphyinfo_hdl(_adapter *padapter, u8 *pbuf);	
extern u8 getphyinfo_hdl(_adapter *padapter, u8 *pbuf);
extern u8 setphy_hdl(_adapter *padapter, u8 *pbuf);
extern u8 getphy_hdl(_adapter *padapter, u8 *pbuf);

#ifdef CONFIG_H2CLBK
extern u8 seth2clbk_hdl(_adapter *padapter, u8 *pbuf);
extern u8 geth2clbk_hdl(_adapter *padapter, u8 *pbuf);
#endif

#endif  //CONFIG_RTL8711FW


#define GEN_CMD_CODE(cmd)	cmd ## _CMD_
enum rtl8711_h2c_cmd {
	GEN_CMD_CODE(_JoinBss) ,		/*0*/
	GEN_CMD_CODE(_DisConnect) ,
	GEN_CMD_CODE(_CreateBss) ,
	GEN_CMD_CODE(_SetOpMode) ,	
	GEN_CMD_CODE(_SiteSurvey) ,
	GEN_CMD_CODE(_SetAuth) ,
	GEN_CMD_CODE(_SetKey) ,
	GEN_CMD_CODE(_SetStaKey) ,
	GEN_CMD_CODE(_SetAssocSta) ,
	GEN_CMD_CODE(_DelAssocSta) ,
	GEN_CMD_CODE(_SetStaPwrState) ,	/* 10 */
	GEN_CMD_CODE(_SetBasicRate) ,
	GEN_CMD_CODE(_GetBasicRate) ,
	GEN_CMD_CODE(_SetDataRate) ,
	GEN_CMD_CODE(_GetDataRate) ,
	GEN_CMD_CODE(_SetPhyInfo) ,
	GEN_CMD_CODE(_GetPhyInfo) ,
	GEN_CMD_CODE(_SetPhy) ,
	GEN_CMD_CODE(_GetPhy) ,
	GEN_CMD_CODE(_GetBBReg) ,
	GEN_CMD_CODE(_SetBBReg) ,	/* 20 */
	GEN_CMD_CODE(_GetRFReg) ,
	GEN_CMD_CODE(_SetRFReg) ,
	GEN_CMD_CODE(_readRssi) ,
	GEN_CMD_CODE(_readGain) ,
	GEN_CMD_CODE(_SetRFIntFs) ,
	GEN_CMD_CODE(_GetRFIntFs) , /* 26 */
#ifdef CONFIG_PWRCTRL	
	GEN_CMD_CODE(_SetAtim) ,
	GEN_CMD_CODE(_SetPwrMode) ,	/* 28 */
#endif
	 GEN_CMD_CODE(_SetRaTable) ,     /* 29 */
        GEN_CMD_CODE(_GetRaTable) ,     /* 30 */


#ifdef CONFIG_H2CLBK	
	GEN_CMD_CODE(_SetH2cLbk) ,
	GEN_CMD_CODE(_GetH2cLbk) ,
#endif
	
	GEN_CMD_CODE(_SetUsbSuspend),
	
	MAX_H2CCMD
};

#ifdef CONFIG_RTL8711FW

#ifdef _WLAN_CMD_C_
#define GEN_FW_CMD_HANDLER(size, cmd)	{size, &##cmd##_hdl},
struct cmdobj	wlancmds[] = {
	GEN_FW_CMD_HANDLER(sizeof (struct joinbss_parm), joinbss)
	GEN_FW_CMD_HANDLER(sizeof (struct disconnect_parm), disconnect)
	GEN_FW_CMD_HANDLER(sizeof (struct createbss_parm), createbss)
	GEN_FW_CMD_HANDLER(sizeof (struct setopmode_parm), setopmode)
	GEN_FW_CMD_HANDLER(sizeof (struct sitesurvey_parm), sitesurvey)
	GEN_FW_CMD_HANDLER(sizeof (struct setauth_parm), setauth)
	GEN_FW_CMD_HANDLER(sizeof (struct setkey_parm), setkey)
	GEN_FW_CMD_HANDLER(sizeof (struct set_stakey_parm), set_stakey)
	GEN_FW_CMD_HANDLER(sizeof (struct set_assocsta_parm), set_assocsta)
	GEN_FW_CMD_HANDLER(sizeof (struct del_assocsta_parm), del_assocsta)
	GEN_FW_CMD_HANDLER(sizeof (struct setstapwrstate_parm), setstapwrstate)
	GEN_FW_CMD_HANDLER(sizeof (struct setbasicrate_parm), setbasicrate)
	GEN_FW_CMD_HANDLER(sizeof (struct getbasicrate_parm), getbasicrate)
	GEN_FW_CMD_HANDLER(sizeof (struct setdatarate_parm), setdatarate)
	GEN_FW_CMD_HANDLER(sizeof (struct getdatarate_parm), getdatarate)
	GEN_FW_CMD_HANDLER(sizeof (struct setphyinfo_parm), setphyinfo)
	GEN_FW_CMD_HANDLER(sizeof (struct getphyinfo_parm), getphyinfo)
	GEN_FW_CMD_HANDLER(sizeof (struct setphy_parm), setphy)
	GEN_FW_CMD_HANDLER(sizeof (struct getphy_parm), getphy)

#ifdef CONFIG_H2CLBK	
	GEN_FW_CMD_HANDLER(sizeof (struct seth2clbk_parm), seth2clbk)
	GEN_FW_CMD_HANDLER(sizeof (struct geth2clbk_rsp), geth2clbk)
#endif
};
#else
	extern struct cmdobj wlancmds[];
	extern void write_cam(u8 entry, u16 ctrl, u8 *mac, u8 *key);
	extern void read_cam(u8 entry, u8 *cnt);
#endif
#endif //CONFIG_RTL8711FW

/*

Result: 
0x00: success
0x01: sucess, and check Response.
0x02: cmd ignored due to duplicated sequcne number
0x03: cmd dropped due to invalid cmd code
0x04: reserved.

*/

#define H2C_RSP_OFFSET				512

#define H2C_SUCCESS				0x00
#define H2C_SUCCESS_RSP			0x01
#define H2C_DUPLICATED				0x02
#define H2C_DROPPED				0x03
#define H2C_PARAMETERS_ERROR		0x04
#define H2C_REJECTED				0x05
#define H2C_CMD_OVERFLOW			0x06
#define H2C_RESERVED				0x07

extern u8 setassocsta_cmd(_adapter  *padapter, u8 *mac_addr);
extern u8 setusbstandby_cmd(_adapter *padapter);
extern u8 setstandby_cmd(_adapter *padapter);
extern u8 sitesurvey_cmd(_adapter  *padapter, NDIS_802_11_SSID *pssid);
extern u8 createbss_cmd(_adapter  *padapter);
extern u8 setphy_cmd(_adapter  *padapter, u8 modem, u8 ch);
extern u8 setstakey_cmd(_adapter  *padapter, u8 *psta, u8 unicast_key);
extern u8 joinbss_cmd(_adapter  *padapter, struct wlan_network* pnetwork);
extern u8 disassoc_cmd(_adapter  *padapter);
extern u8 setopmode_cmd(_adapter  *padapter, NDIS_802_11_NETWORK_INFRASTRUCTURE networktype);
extern u8 setdatarate_cmd(_adapter  *padapter, u8 *rateset);
extern u8 setbasicrate_cmd(_adapter  *padapter, u8 *rateset);
extern u8 setbbreg_cmd(_adapter * padapter, u8 offset, u8 val);
extern u8 setrfreg_cmd(_adapter * padapter, u8 offset, u32 val);
extern u8 getbbreg_cmd(_adapter * padapter, u8 offset, u8 * pval);
extern u8 getrfreg_cmd(_adapter * padapter, u8 offset, u8 * pval);
extern u8 setrfintfs_cmd(_adapter  *padapter, u8 mode);
extern u8 setrttbl_cmd(_adapter  *padapter, struct setratable_parm *prate_table);
extern u8 getrttbl_cmd(_adapter  *padapter, struct getratable_rsp *pval);

extern void survey_cmd_callback(_adapter  *padapter, struct cmd_obj *pcmd);
extern void disassoc_cmd_callback(_adapter  *padapter, struct cmd_obj *pcmd);
extern void joinbss_cmd_callback(_adapter  *padapter, struct cmd_obj *pcmd);	
extern void createbss_cmd_callback(_adapter  *padapter, struct cmd_obj *pcmd);
extern void getbbrfreg_cmdrsp_callback(_adapter  *padapter, struct cmd_obj *pcmd);

extern void setstaKey_cmdrsp_callback(_adapter  *padapter,  struct cmd_obj *pcmd);
extern void setassocsta_cmdrsp_callback(_adapter  *padapter,  struct cmd_obj *pcmd);
extern void getrttbl_cmdrsp_callback(_adapter  *padapter,  struct cmd_obj *pcmd);

#ifdef CONFIG_PWRCTRL
extern u8 setpwrmode_cmd(_adapter* adapter, u32 ps_mode);
extern u8  setatim_cmd(_adapter* adapter, u8 add, u8 txid);

#endif

struct _cmd_callback {
	u32	cmd_code;
	void (*callback)(_adapter  *padapter, struct cmd_obj *cmd);
};

#ifdef _RTL871X_CMD_C_

struct _cmd_callback 	cmd_callback[] = {
	{_JoinBss_CMD_, &joinbss_cmd_callback},//select_and_join_from_scanned_queue	0
	{_DisConnect_CMD_, &disassoc_cmd_callback}, //MgntActSet_802_11_DISASSOCIATE
	{_CreateBss_CMD_, &createbss_cmd_callback},
	{_SetOpMode_CMD_,NULL},
	{_SiteSurvey_CMD_, &survey_cmd_callback}, //MgntActSet_802_11_BSSID_LIST_SCAN
	{_SetAuth_CMD_,NULL},
	{_SetKey_CMD_,NULL},
	{_SetStaKey_CMD_,&setstaKey_cmdrsp_callback},
	{_SetAssocSta_CMD_, &setassocsta_cmdrsp_callback},
	{_DelAssocSta_CMD_,NULL},
	{_SetStaPwrState_CMD_,NULL},	//10
	{_SetBasicRate_CMD_,NULL},
	{_GetBasicRate_CMD_,NULL},
	{_SetDataRate_CMD_,NULL},
	{_GetDataRate_CMD_,NULL},
	{_SetPhyInfo_CMD_,NULL},
	{_GetPhyInfo_CMD_,NULL},
	{_SetPhy_CMD_,NULL},
	{_GetPhy_CMD_,NULL},	
	{GEN_CMD_CODE(_GetBBReg),&getbbrfreg_cmdrsp_callback},
	{GEN_CMD_CODE(_SetBBReg),NULL},	//20
	{GEN_CMD_CODE(_GetRFReg),&getbbrfreg_cmdrsp_callback},
	{GEN_CMD_CODE(_SetRFReg),NULL},
	{GEN_CMD_CODE(_readRssi),NULL},
	{GEN_CMD_CODE(_readGain),NULL},	
	{GEN_CMD_CODE(_SetRFIntFs),NULL},
	{GEN_CMD_CODE(_GetRFIntFs),NULL},
#ifdef CONFIG_PWRCTRL
	{_SetAtim_CMD_, NULL},
	{_SetPwrMode_CMD_, NULL},	//28
#endif
	{_SetRaTable_CMD_, NULL},
	{_GetRaTable_CMD_, NULL},
	{_SetUsbSuspend_CMD_, NULL}
};

/*
	GEN_CMD_CODE(_JoinBss) ,
	GEN_CMD_CODE(_DisConnect) ,
	GEN_CMD_CODE(_CreateBss) ,
	GEN_CMD_CODE(_SetOpMode) ,	
	GEN_CMD_CODE(_SiteSurvey) ,
	GEN_CMD_CODE(_SetAuth) ,
	GEN_CMD_CODE(_SetKey) ,
	GEN_CMD_CODE(_SetStaKey) ,
	GEN_CMD_CODE(_SetAssocSta) ,
	GEN_CMD_CODE(_DelAssocSta) ,
	GEN_CMD_CODE(_SetStaPwrState) ,
	GEN_CMD_CODE(_SetBasicRate) ,
	GEN_CMD_CODE(_GetBasicRate) ,
	GEN_CMD_CODE(_SetDataRate) ,
	GEN_CMD_CODE(_GetDataRate) ,
	GEN_CMD_CODE(_SetPhyInfo) ,
	GEN_CMD_CODE(_GetPhyInfo) ,
	GEN_CMD_CODE(_SetPhy) ,
	GEN_CMD_CODE(_GetPhy) ,
*/	
#else

extern struct _cmd_callback 	cmd_callback[];

#endif

#endif // _CMD_H_
