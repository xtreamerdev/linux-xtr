/*

	MIPS-16 Mode if CONFIG_MIPS16 is on

*/


#define _RTL8187_RELATED_FILES_C
#define _WLAN_MLME_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#include <rtl8192u_spec.h>
#include <rtl8192u_regdef.h>

#include <usb_ops.h>
#include <rtl8192u/rtl8192u_mlme.h>
#include <rtl8192u/rtl8192u_event.h>
#include <rtl8192u/rtl8192u_haldef.h>
#include <circ_buf.h>
#include <wlan_mlme.h>
#include <wlan_sme.h>
#include <rtl8192u/irp.h>
#include <wlan_task.h>
#include <wlan_util.h>
#include <wlan.h>
#include <wifi.h>
#include <hal_init.h>

#include <linux/wireless.h>
#include <ieee80211.h>
#include <rtl8192u/RTL8711_RFCfg.h>
#include <rtl871x_debug.h>


extern volatile unsigned char	vcsMode;
extern volatile unsigned char	vcsType;	

extern VOID HTConstructCapabilityElement(_adapter *padapter,PHT_CAPABILITY_ELE pCapELE);
extern u8 HTConstructInfoElement(_adapter *padapter, 	PHT_INFORMATION_ELE		pHTInfoEle);
extern VOID HTOnAssocRsp(_adapter *padapter, u8 * pframe, u32 irp_len);
extern void SetHalRATRTable819xUsb(PADAPTER		Adapter,u8	*posLegacyRate,u8			*pMcsRate);

void issue_beacon(_adapter *padapter);

extern void join_ibss(_adapter *padapter,u8 * pframe,u32 irp_len);

static unsigned int iv;
unsigned char beacon_cnt = 10;

#define SURVEY_TO	(400)
extern unsigned int  ratetbl2rateset(_adapter *padapter, unsigned char *);
static int collect_bss_info(struct rxirp *, NDIS_WLAN_BSSID_EX *);

void mgt_handlers(struct rxirp *rx_irp);
#define MAX_BCNLEN	(512>>2)

#ifdef CONFIG_RTL8192U
u1Byte MCS_FILTER_ALL[16] = {	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
									0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	};

u1Byte MCS_FILTER_1SS[16] = {	0xff, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
									0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	};


static u2Byte MCS_DATA_RATE[2][2][77] = 
	{	{	{13, 26, 39, 52, 78, 104, 117, 130, 26, 52, 78 ,104, 156, 208, 234, 260,
			39, 78, 117, 234, 312, 351, 390, 52, 104, 156, 208, 312, 416, 468, 520, 
			0, 78, 104, 130, 117, 156, 195, 104, 130, 130, 156, 182, 182, 208, 156, 195,
			195, 234, 273, 273, 312, 130, 156, 181, 156, 181, 208, 234, 208, 234, 260, 260, 
			286, 195, 234, 273, 234, 273, 312, 351, 312, 351, 390, 390, 429},			// Long GI, 20MHz
			{14, 29, 43, 58, 87, 116, 130, 144, 29, 58, 87, 116, 173, 231, 260, 289, 
			43, 87, 130, 173, 260, 347, 390, 433, 58, 116, 173, 231, 347, 462, 520, 578, 
			0, 87, 116, 144, 130, 173, 217, 116, 144, 144, 173, 202, 202, 231, 173, 217, 
			217, 260, 303, 303, 347, 144, 173, 202, 173, 202, 231, 260, 231, 260, 289, 289, 
			318, 217, 260, 303, 260, 303, 347, 390, 347, 390, 433, 433, 477}	},		// Short GI, 20MHz
		{	{27, 54, 81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540, 
			81, 162, 243, 324, 486, 648, 729, 810, 108, 216, 324, 432, 648, 864, 972, 1080, 
			12, 162, 216, 270, 243, 324, 405, 216, 270, 270, 324, 378, 378, 432, 324, 405, 
			405, 486, 567, 567, 648, 270, 324, 378, 324, 378, 432, 486, 432, 486, 540, 540, 
			594, 405, 486, 567, 486, 567, 648, 729, 648, 729, 810, 810, 891}, 	// Long GI, 40MHz
			{30, 60, 90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600, 
			90, 180, 270, 360, 540, 720, 810, 900, 120, 240, 360, 480, 720, 960, 1080, 1200, 
			13, 180, 240, 300, 270, 360, 450, 240, 300, 300, 360, 420, 420, 480, 360, 450, 
			450, 540, 630, 630, 720, 300, 360, 420, 360, 420, 480, 540, 480, 540, 600, 600, 
			660, 450, 540, 630, 540, 630, 720, 810, 720, 810, 900, 900, 990}	}	// Short GI, 40MHz
	};

#endif


/*


Caller must guarantee the calling context in atomic.



*/


static unsigned int OnAssocReq(struct rxirp *irp);
static unsigned int OnAssocRsp(struct rxirp *irp);
static unsigned int OnProbeReq(struct rxirp *irp);
static unsigned int OnProbeRsp(struct rxirp *irp);
static unsigned int DoReserved(struct rxirp *irp);
static unsigned int OnBeacon(struct rxirp *irp);
static unsigned int OnAtim(struct rxirp *irp);
static unsigned int OnDisassoc(struct rxirp *irp);
static unsigned int OnAuth(struct rxirp *irp);
static unsigned int OnDeAuth(struct rxirp *irp);
static unsigned int OnAction(struct rxirp *irp);



struct mlme_handler {
	unsigned int   num;
	char* str;
	unsigned int (*func)(struct rxirp *irp);
};

struct mlme_handler mlme_tbl[]={
	{WIFI_ASSOCREQ,		"OnAssocReq",	&OnAssocReq},
	{WIFI_ASSOCRSP,		"OnAssocRsp",	&OnAssocRsp},
	{WIFI_REASSOCREQ,	"OnReAssocReq",	&OnAssocReq},
	{WIFI_REASSOCRSP,	"OnReAssocRsp",	&OnAssocRsp},
	{WIFI_PROBEREQ,		"OnProbeReq",	&OnProbeReq},
	{WIFI_PROBERSP,		"OnProbeRsp",	&OnProbeRsp},

	/*----------------------------------------------------------
					below 2 are reserved
	-----------------------------------------------------------*/
	{0,					"DoReserved",	&DoReserved},
	{0,					"DoReserved",	&DoReserved},
	{WIFI_BEACON,		"OnBeacon",		&OnBeacon},
	{WIFI_ATIM,			"OnATIM",		&OnAtim},
	{WIFI_DISASSOC,		"OnDisassoc",		&OnDisassoc},
	{WIFI_AUTH,			"OnAuth",		&OnAuth},
	{WIFI_DEAUTH,		"OnDeAuth",		&OnDeAuth},
	{WIFI_ACTION,		"OnAction",		&OnAction},
};

#ifdef CONFIG_WMM

unsigned char WMM_IE[] = {0x00, 0x50, 0xf2, 0x02, 0x00, 0x01, 0x00};
unsigned char WMM_PARA_IE[] = {0x00, 0x50, 0xf2, 0x02, 0x01, 0x01};
unsigned char WMM_para_cnt;

#endif


u8 MgntQuery_MgntFrameTxRate(_adapter *padapter)
{
	u8 WirelessMode = padapter->_sys_mib.network_type;
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	u8 rate;
	
//	rate = pMgntInfo->CurrentBasicRate & 0x7f;

//	if(rate == 0){
		// 2005.01.26, by rcnjko.
		if(WirelessMode == WIRELESS_11A ||
			WirelessMode == WIRELESS_11N_5G||
			(WirelessMode == WIRELESS_11N_2G4&&!pHTInfo->bCurSuppCCK))
			rate = 0x0c;
		else
			rate = 0x02;
//	}

	/*
	// Data rate of ProbeReq is already decided. Annie, 2005-03-31
	if( pMgntInfo->bScanInProgress || (pMgntInfo->bDualModeScanStep!=0) )
	{
		if(pMgntInfo->dot11CurrentWirelessMode==WIRELESS_MODE_A)
			rate = 0x0c;
		else
			rate = 0x02;
	}
	*/

	return rate;
}

void append_information_element(u8 **dest, u8 *source, struct txirp *irp, u8 length)
{
	if(dest == NULL)
		return;
	if(source != NULL)
	{
		_memcpy(*dest, source, length);
	}	

	*dest += length;
	irp->tx_len += length; 
}


#ifdef CONFIG_RTL8192U

VOID
ActivateBAEntry(
	PADAPTER		Adapter,
	PBABODY		pBA,
	u16			Time
	)
{
	pBA->bValid = _TRUE;
	if(Time != 0)
		_set_timer(&pBA->Timer, Time);
}


VOID
DeActivateBAEntry(
	PADAPTER		Adapter,
	PBABODY		pBA
)
{
	u8 bool;
	pBA->bValid = _FALSE;
	_cancel_timer(&pBA->Timer, &bool);
}

VOID
SendADDBAReq(	_adapter *padapter, u8 *paddr, PBABODY pbabody)
{
	u8 *prspframe, *bssid;
       struct ieee80211_hdr *hdr;

	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	struct txirp *irp = alloc_txirp(padapter);
	
	DEBUG_INFO(("+SendADDBAReq\n"));

	if (irp == NULL)
	{
		printk("Can't alloc irp %d:%s\n", __LINE__, __FILE__);	
		return;
	}
	
	prspframe = get_txpframe(irp);
	hdr = (struct ieee80211_hdr *)prspframe;
	bssid = _sys_mib->cur_network.MacAddress;
	hdr->frame_ctl = 0;
	SetFrameSubType(prspframe, WIFI_ACTION);
	hdr->duration_id = 0;
	hdr->seq_ctl = 0;
	
	_memcpy((void *)hdr->addr1, paddr, ETH_ALEN);
	_memcpy((void *)hdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_memcpy((void *)hdr->addr3, bssid, ETH_ALEN);


	prspframe += sizeof (struct ieee80211_hdr_3addr);
	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);

	*(u8 *)prspframe = 3;// Category field (Block Ack)
	*(u8 *)(prspframe+1) = 0; // Action field  (ADDBA request)
  	append_information_element(&prspframe, NULL, irp, 2);
	
	// Dialog Token
	append_information_element(&prspframe, &(pbabody->DialogToken), irp, 1);

	// BA Parameter Set
	append_information_element(&prspframe, (pbabody->BaParamSet.charData), irp, 2);

	// BA Timeout Value
	append_information_element(&prspframe, (u8*)&(pbabody->BaTimeoutValue), irp, 2);
	// BA Start SeqCtrl
	append_information_element(&prspframe, (u8*)&(pbabody->BaStartSeqCtrl.ShortData), irp, 2);

	wlan_tx(padapter, irp);
	
	DEBUG_INFO(("-SendADDBAReq"));
	
}


//static unsigned int iv;
int SendADDBARsp(_adapter *padapter, u8  * paddr, PBABODY   pbabody, u16 status)
{
	u8 *prspframe, *bssid;
	struct ieee80211_hdr *hdr;
	unsigned short *fctrl;
	struct txirp *irp = alloc_txirp(padapter);
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	DEBUG_INFO(("SendADDBARsp\n"));
	  
	if (irp == NULL)
	{
		printk("Can't alloc irp %d:%s\n", __LINE__, __FILE__);	
		return 0;
	}
	prspframe = get_txpframe(irp);
	hdr = (struct ieee80211_hdr *)prspframe;
        bssid = _sys_mib->cur_network.MacAddress;
	fctrl = &(hdr->frame_ctl);
	*(fctrl) = 0;
	hdr->seq_ctl = 0;
	_memcpy((void *)hdr->addr1, paddr, ETH_ALEN);
	_memcpy((void *)hdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_memcpy((void *)hdr->addr3, bssid, ETH_ALEN);

	SetFrameSubType(prspframe, WIFI_ACTION);

	prspframe += sizeof (struct ieee80211_hdr_3addr);
	
	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);

	*(u8 *)prspframe = 3;// Category field (Block Ack)
	*(u8 *)(prspframe+1) = 1; // Action field  (ADDBA response)
       prspframe += 2;
	irp->tx_len +=2;	
	_memcpy(prspframe, &(pbabody->DialogToken), 1); // Dialog Token
	  prspframe += 1;
	irp->tx_len += 1; 
	_memcpy(prspframe, &status, 2); // status
	  prspframe += 2;
	irp->tx_len += 2; 

	// BA Parameter Set
	_memcpy(prspframe, &(pbabody->BaParamSet), 2); // BaParamSet
	  prspframe += 2;
	irp->tx_len += 2; 

	// BA Timeout Value
	_memcpy(prspframe, &(pbabody->BaTimeoutValue), 2); // BaTimeoutValue
	  prspframe += 2;
	irp->tx_len += 2;
 
	DEBUG_INFO(("wlan_tx: SendADDBARsp\n"));
	wlan_tx(padapter, irp);
	return 1; 
}

VOID
SendDELBA(
	IN	_adapter 				*padapter,
	IN	u8  					*paddr,
	IN 	PBABODY			pbabody,
	IN	TR_SELECT			TxRxSelect,
	IN	u16					ReasonCode
	)
{
	DELBA_PARAM_SET DelbaParamSet;
	u8 *prspframe, *bssid;
       struct ieee80211_hdr *hdr;
//	u8 DataRate = MgntQuery_MgntFrameTxRate(padapter);
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	struct txirp *irp = alloc_txirp(padapter);
	if (irp == NULL)
	{
		printk("Can't alloc irp %d:%s\n", __LINE__, __FILE__);	
		return;
	}
	prspframe = get_txpframe(irp);
	hdr = (struct ieee80211_hdr *)prspframe;
	bssid = _sys_mib->cur_network.MacAddress;
	hdr->frame_ctl = 0;
	SetFrameSubType(prspframe, WIFI_ACTION);
	hdr->duration_id = 0;
	hdr->seq_ctl = 0;
	
	_memcpy((void *)hdr->addr1, bssid, ETH_ALEN);
	_memcpy((void *)hdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_memcpy((void *)hdr->addr3, bssid, ETH_ALEN);

	prspframe += sizeof (struct ieee80211_hdr_3addr);
	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);

	*(u8 *)prspframe = 3;// Category field (Block Ack)
	*(u8 *)(prspframe+1) = 2; // Action field  (DELBA)
  	append_information_element(&prspframe, NULL, irp, 2);

	// DELBA Parameter Set
	DelbaParamSet.field.Initiator	= (TxRxSelect==TX_DIR)?1:0;
	DelbaParamSet.field.TID	= 0;//pBA->BaParamSet.field.TID;
	append_information_element(&prspframe, &(pbabody->DialogToken), irp, 2);

	// Reason Code
	append_information_element(&prspframe, (u8*)&(ReasonCode), irp, 2);

	wlan_tx(padapter, irp);
}

void
OnADDBAReq(
	_adapter *padapter,
	u8 *pframe
	)
{
	


	BABODY   babody;
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;	
	PNDIS_WLAN_BSSID_EX pcur_network = &(padapter->_sys_mib.cur_network);
	u8				*pDialogToken = (u8 *)(pframe + WLAN_HDR_A3_LEN + 2);
	PBA_PARAM_SET		pBaParamSet =  (PBA_PARAM_SET)(pframe+ WLAN_HDR_A3_LEN + 3);
	u16				*pBaTimeoutVal = ((u16 *)(pframe + WLAN_HDR_A3_LEN + 5));
	u8				*paddr = GetAddr2Ptr(pframe);
	u16				StatusCode;
	u32 res = 1;
	PSEQUENCE_CONTROL	pBaStartSeqCtrl = ((PSEQUENCE_CONTROL)(pframe +WLAN_HDR_A3_LEN + 7));

	//
	// Check Block Ack support.
	// Reject if there is no support of Block Ack.
	//
	if(pcur_network->BssQos.bdQoSMode == QOS_DISABLE ||
		pHTInfo->bCurrentHTSupport == _FALSE )
	{
		StatusCode = ADDBA_STATUS_REFUSED;
		DEBUG_INFO(("ADDBA_STATUS_REFUSED\n"));
		goto OnADDBAReq_Fail;
	}
	
	


	//
	// To Determine the ADDBA Req content
	// We can do much more check here, including BufferSize, AMSDU_Support, Policy, StartSeqCtrl...
	// I want to check StartSeqCtrl to make sure when we start aggregation!!!
	//
	if(pBaParamSet->field.BAPolicy == BA_POLICY_DELAYED)
	{
		StatusCode = ADDBA_STATUS_INVALID_PARAM;
		printk("ADDBA_STATUS_INVALID_PARAM\n");
		goto OnADDBAReq_Fail;
	}


	
	babody.DialogToken = *pDialogToken;
	babody.BaParamSet = *pBaParamSet;
	babody.BaTimeoutValue = *pBaTimeoutVal;
	babody.BaStartSeqCtrl = *pBaStartSeqCtrl;
	babody.BaParamSet.field.BufferSize = 32;	// At least, forced by SPEC
	
	
	res = SendADDBARsp(padapter, paddr, &babody, ADDBA_STATUS_SUCCESS);
	if(res == 0)
		DEBUG_ERR(("sendADDBARsp Fail \n"));

	// End of procedure.
	return;

OnADDBAReq_Fail:
	{
		
		babody.BaParamSet = *pBaParamSet;
		babody.BaTimeoutValue = *pBaTimeoutVal;
		babody.DialogToken = *pDialogToken;
		babody.BaParamSet.field.BAPolicy = BA_POLICY_IMMEDIATE;
              res = SendADDBARsp(padapter, paddr, &babody, StatusCode);
   	if(res == 0)
		DEBUG_ERR(("sendADDBARsp Fail \n"));

	}

}


VOID
OnADDBARsp(
	_adapter *padapter,
	u8 *pframe
	)
{
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;	
	PNDIS_WLAN_BSSID_EX pcur_network = &(padapter->_sys_mib.cur_network);
	PBABODY 			pPendingBA, pAdmittedBA;
	PTX_TS_RECORD		pTS;
	struct ieee80211_hdr *pieee80211_frame = (struct ieee80211_hdr *)pframe;
	pu1Byte				Addr = pieee80211_frame->addr2;
	pu1Byte				pDialogToken =  (pframe + sMacHdrLng + 2);
	pu2Byte				pStatusCode =  ((pu2Byte)(pframe + sMacHdrLng + 3));
	PBA_PARAM_SET		pBaParamSet = ((PBA_PARAM_SET)(pframe + sMacHdrLng + 5));
	pu2Byte				pBaTimeoutVal = ((pu2Byte)(pframe + sMacHdrLng + 7));
	u16					ReasonCode;

	DEBUG_INFO(("+OnADDBARsp\n"));

	//
	// Check the capability
	// Since we can always receive A-MPDU, we just check if it is under HT mode.
	//
	if(pcur_network->BssQos.bdQoSMode == QOS_DISABLE ||
		pHTInfo->bCurrentHTSupport == _FALSE ||
		pHTInfo->bCurrentAMPDUEnable == _FALSE )
	{
		ReasonCode = DELBA_REASON_UNKNOWN_BA;
		goto OnADDBARsp_Reject;
	}

#if 0	
	//
	// Search for related TS.
	// If there is no TS found, we wil reject ADDBA Rsp by sending DELBA frame.
	//
	if (!GetTs(
			padapter, 
			(PTS_COMMON_INFO*)(&pTS), 
			Addr, 
			(u1Byte)(pBaParamSet->field.TID), 
			TX_DIR,
			FALSE)	)
	{
		ReasonCode = DELBA_REASON_UNKNOWN_BA;
		goto OnADDBARsp_Reject;
	}
#endif	
	pTS = &(padapter->xmitpriv.TxTsRecord);//john

	pTS->bAddBaReqInProgress = _FALSE;
	pPendingBA = pTS->TxPendingBARecord;
	pAdmittedBA = pTS->TxAdmittedBARecord;


	//
	// Check if related BA is waiting for setup.
	// If not, reject by sending DELBA frame.
	//
	if((pAdmittedBA->bValid==_TRUE))
	{
		// Since BA is already setup, we ignore all other ADDBA Response.
		DEBUG_ERR(("OnADDBARsp(): Recv ADDBA Rsp. Drop because already admit it! \n"));
		return;
	}
	else if((pPendingBA->bValid == _FALSE) ||(*pDialogToken != pPendingBA->DialogToken))
	{
		DEBUG_ERR(("OnADDBARsp(): Recv ADDBA Rsp. BA invalid, DELBA! \n"));
		ReasonCode = DELBA_REASON_UNKNOWN_BA;
		goto OnADDBARsp_Reject;
	}
	else
	{
		DEBUG_ERR(("OnADDBARsp(): Recv ADDBA Rsp. BA is admitted! Status code:%X\n", *pStatusCode));
		DeActivateBAEntry(padapter, pPendingBA);
	}


	if(*pStatusCode == ADDBA_STATUS_SUCCESS)
	{
		//
		// Determine ADDBA Rsp content here.
		// We can compare the value of BA parameter set that Peer returned and Self sent.
		// If it is OK, then admitted. Or we can send DELBA to cancel BA mechanism.
		//
		if(pBaParamSet->field.BAPolicy == BA_POLICY_DELAYED)
		{
			// Since this is a kind of ADDBA failed, we delay next ADDBA process.
			pTS->bAddBaReqDelayed = _TRUE;
		
			DeActivateBAEntry(padapter, pAdmittedBA);
			ReasonCode = DELBA_REASON_END_BA;
			goto OnADDBARsp_Reject;
		}


		//
		// Admitted condition
		//
		pAdmittedBA->DialogToken = *pDialogToken;
		pAdmittedBA->BaTimeoutValue = *pBaTimeoutVal;
		pAdmittedBA->BaStartSeqCtrl = pPendingBA->BaStartSeqCtrl;
		pAdmittedBA->BaParamSet = *pBaParamSet;
		DeActivateBAEntry(padapter, pAdmittedBA);
		ActivateBAEntry(padapter, pAdmittedBA, *pBaTimeoutVal);
	}
	else
	{
		// Delay next ADDBA process.
		pTS->bAddBaReqDelayed = _TRUE;
	}
	
	DEBUG_INFO(("-OnADDBARsp\n"));

	// End of procedure
	return;

OnADDBARsp_Reject:
	{
		BABODY BA;
		BA.BaParamSet = *pBaParamSet;
		SendDELBA(padapter, Addr, &BA, TX_DIR, ReasonCode);
	}
	
}


//
// ADDBA initiate. This can only be called by TX side.
//
VOID
TsInitAddBA(PVOID	FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	PTX_TS_RECORD pTxTs = &(padapter->xmitpriv.TxTsRecord);
	
	PBABODY pBA = pTxTs->TxPendingBARecord;

	DEBUG_INFO(("+TsInitAddBA\n"));

	if(pBA->bValid==_TRUE)
		return;
	
	// Set parameters to "Pending" variable set
	DeActivateBAEntry(padapter, pBA);
	
	pBA->DialogToken++;						// DialogToken: Only keep the latest dialog token
	pBA->BaParamSet.field.AMSDU_Support = 0;	// Do not support A-MSDU with A-MPDU now!!
	pBA->BaParamSet.field.BAPolicy = BA_POLICY_IMMEDIATE;	// Policy: Delayed or Immediate
	pBA->BaParamSet.field.TID = 0; //FIXME: pTS->TsCommonInfo.TSpec.f.TSInfo.field.ucTSID;	// TID
	// BufferSize: This need to be set according to A-MPDU vector
	pBA->BaParamSet.field.BufferSize = 32;		// BufferSize: This need to be set according to A-MPDU vector
	pBA->BaTimeoutValue = 0;					// Timeout value: Set 0 to disable Timer
	pBA->BaStartSeqCtrl.Field.SeqNum = (padapter->ieee80211_seq_ctl + 3) & 4095; 	// Block Ack will start after 3 packets later.

	ActivateBAEntry(padapter, pBA, 200);  //BA_SETUP_TIMEOUT == 200

//	SendADDBAReq(padapter, padapter->_sys_mib.cur_network.MacAddress, pBA);

	DEBUG_INFO(("-TsInitAddBA\n"));
}

VOID
BaSetupTimeOut(PVOID	FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	PTX_TS_RECORD pTxTs = &(padapter->xmitpriv.TxTsRecord);
printk("+BaSetupTimeOut\n");
	pTxTs->bAddBaReqInProgress = _FALSE;
	pTxTs->bAddBaReqDelayed = _TRUE;
	pTxTs->TxPendingBARecord->bValid = _FALSE;
printk("-BaSetupTimeOut\n");
}

BOOLEAN
RxTsDeleteBA(
	PADAPTER		Adapter,
	PRX_TS_RECORD	pRxTs
	)
{
	PBABODY pBa = &pRxTs->RxAdmittedBARecord;
	BOOLEAN	 bSendDELBA = _FALSE;

	if(pBa->bValid)
	{
		DeActivateBAEntry(Adapter, pBa);
		bSendDELBA = _TRUE;
	}

	return bSendDELBA;
}


BOOLEAN
TxTsDeleteBA(
	PADAPTER		Adapter,
	PTX_TS_RECORD	pTxTs
	)
{
	PBABODY		pAdmittedBa = pTxTs->TxAdmittedBARecord;
	PBABODY		pPendingBa = pTxTs->TxPendingBARecord;
	BOOLEAN			bSendDELBA = _FALSE;

	// Delete pending BA
	if(pPendingBa->bValid)
	{
		DeActivateBAEntry(Adapter, pPendingBa);
		bSendDELBA = _TRUE;
	}

	// Delete admitted BA
	if(pAdmittedBa->bValid)
	{
		DeActivateBAEntry(Adapter, pAdmittedBa);
		bSendDELBA = _TRUE;
	}

	return bSendDELBA;
}


VOID
TxBaInactTimeout(PVOID	FunctionContext)
{
	PADAPTER	Adapter = (PADAPTER)FunctionContext;
	PTX_TS_RECORD	pTxTs = &(Adapter->xmitpriv.TxTsRecord);
printk("+TxBaInactTimeout\n");
	TxTsDeleteBA(Adapter, pTxTs);
	SendDELBA(
		Adapter, 
		NULL,//pTxTs->TsCommonInfo.Addr, 
		pTxTs->TxAdmittedBARecord,
		TX_DIR, 
		DELBA_REASON_TIMEOUT	);
printk("-TxBaInactTimeout\n");
}

VOID
OnDELBA(_adapter *padapter, u8 *pframe)
{
	u8 canceled;
	struct ieee80211_hdr 	*pframe_hdr = (struct ieee80211_hdr *)pframe;
//	pu1Byte				Addr = pframe_hdr->addr2;
	PDELBA_PARAM_SET	pDelBaParamSet = ((PDELBA_PARAM_SET)(pframe_hdr + sMacHdrLng + 2));
//	pu2Byte				pReasonCode = ((pu2Byte)(pframe_hdr + sMacHdrLng + 4));
	struct mib_info 		*_sys_mib = &(padapter->_sys_mib);
	PNDIS_WLAN_BSSID_EX pcur_network = &_sys_mib->cur_network;

printk("+OnDELBA\n");

	if(pcur_network->BssQos.bdQoSMode > QOS_DISABLE ||
		padapter->HTInfo.bCurrentHTSupport == _FALSE )
		return;


	if(pDelBaParamSet->field.Initiator == 1)
	{
		PRX_TS_RECORD 	pRxTs = &(padapter->xmitpriv.RxTsRecord);
#if 0
		if( !GetTs(
				Adapter, 
				(PTS_COMMON_INFO*)&pRxTs, 
				Addr, 
				(u1Byte)pDelBaParamSet->field.TID, 
				RX_DIR,
				FALSE)	)
		{
			return;
		}
#endif		
		RxTsDeleteBA(padapter, pRxTs);
	}
	else
	{

		PTX_TS_RECORD	pTxTs = &(padapter->xmitpriv.TxTsRecord);
#if 0	
		if(!GetTs(
			Adapter, 
			(PTS_COMMON_INFO*)&pTxTs, 
			Addr, 
			(u1Byte)pDelBaParamSet->field.TID, 
			TX_DIR,
			FALSE)	)
		{
			return;
		}
#endif		
		pTxTs->bUsingBa = _FALSE;
		pTxTs->bAddBaReqInProgress = _FALSE;
		pTxTs->bAddBaReqDelayed = _FALSE;
		_cancel_timer(&pTxTs->TsAddBaTimer, &canceled);
		TxTsDeleteBA(padapter, pTxTs);
	}
	
printk("-OnDELBA\n");

}

#endif /*CONFIG_RTL8192U*/

static unsigned char *set_fixed_ie(unsigned char *pbuf, unsigned int len, unsigned char *source,
				unsigned int *frlen)
{
	_memcpy((void *)pbuf, (void *)source, len);
	*frlen = *frlen + len;
	return (pbuf + len);
}

extern thread_return mlme_thread(thread_context context)
{
	_adapter * padapter = (_adapter *)context;
	u8 state;
	struct  cmd_priv	*pcmdpriv=&padapter->cmdpriv;
	struct cmd_obj	*pcmd;
	u8 (*pcmd_func)(_adapter	*dev, u8	*pcmd);	//void (*pcmd_callback)(unsigned char *dev, struct cmd_obj	*pcmd);
	struct	event_node *node;
#if 1 
_func_enter_;

#ifdef PLATFORM_LINUX
	daemonize("%s", padapter->pnetdev->name);
	allow_signal(SIGTERM);
#endif

	DEBUG_INFO(("\n\n + mlme_thread \n\n"));
	
	while(1)
	{
next:	

		if ((_down_sema(&(pcmdpriv->cmd_proc_sema))) == _FAIL){
			break;
		}

		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE)){
			DEBUG_ERR(("mlme_thread:bDriverStopped or bSurpriseRemoved"));
			break;
		}

		state =0x0;
		if(padapter->_sys_mib.change_chan==_TRUE){
			state |=BIT(2);
		}
//		if (padapter->_sys_mib.evt_clear == _SUCCESS){
//			state |=BIT(0);
//		}
		if(padapter->_sys_mib.cmd_proc==_TRUE) {
			state |=BIT(1);
		}
		DEBUG_ERR(("mlme_thread: state = %d\n\n", state));

		if(state==0)
			goto next;
			
#if 1
		if(state & BIT(0)){
			
			//needs to check event
			volatile int	*caller_ff_tail;
			int	caller_ff_sz;
			volatile int *head = &(padapter->_sys_mib.c2hevent.head);	
			volatile int *tail = &(padapter->_sys_mib.c2hevent.tail);
			DEBUG_ERR(("\n dequeue_event!!!\n"));
			node = &(padapter->_sys_mib.c2hevent.nodes[*tail]);

			if (CIRC_CNT(*head, *tail, C2HEVENT_SZ) == 0) {
#ifdef CONFIG_POWERSAVING
				CLR_DATA_OPEN(ps_data_open, C2H_DATA_OPEN);
#endif
				goto step2;
			}

			caller_ff_tail = node->caller_ff_tail;
			caller_ff_sz = node->caller_ff_sz;

			if (caller_ff_tail)
				*caller_ff_tail = ((*caller_ff_tail) + 1) & (caller_ff_sz - 1);
	
			*tail = ((*tail) + 1) & (C2HEVENT_SZ - 1);

			if (CIRC_CNT(*head, *tail, C2HEVENT_SZ) == 0) {
#ifdef CONFIG_POWERSAVING
				CLR_DATA_OPEN(ps_data_open, C2H_DATA_OPEN);
#endif
				goto step2;
			}

			c2hclrdsr(padapter);	

		step2:		
			padapter->_sys_mib.evt_clear = _FAIL;
		}
#endif
		if(state & BIT(1)){
			//there is a command needs to processing
			DEBUG_ERR(("+mlme_thread: dequeue cmd  \n"));
			
			pcmd=dequeue_cmd(&pcmdpriv->cmd_processing_queue);

			while(pcmd !=NULL){

				DEBUG_ERR(("pcmd->cmdcode = %d\n", pcmd->cmdcode));

				pcmd_func = cmd_funcs[pcmd->cmdcode].funcs;

				if(pcmd_func==NULL){
					//error
					DEBUG_ERR(("\n pcmd_func==NULL \n"));
				}	
				else{
					pcmd->res=pcmd_func(padapter, pcmd->parmbuf);
				//	cmd_clr_isr(&padapter->cmdpriv);
					DEBUG_ERR(("\n pcmd_func(res=%d, cmdcode=0x%x) \n", pcmd->res, pcmd->cmdcode));
				}
				
				pcmd=dequeue_cmd(&pcmdpriv->cmd_processing_queue);
			}
			padapter->_sys_mib.cmd_proc=_FALSE;
			DEBUG_ERR(("-mlme_thread: dequeue cmd  \n"));

		}

		if(state & BIT(2))
		{
			DEBUG_ERR(("+mlme_thread: change chan (%d) \n",padapter->_sys_mib.cur_channel));

#ifdef CONFIG_RTL8192U
			rtl8256_rf_set_chan(padapter, padapter->_sys_mib.cur_channel);	
#elif defined(CONFIG_RTL8187)
			rtl8225z2_rf_set_chan(padapter, padapter->_sys_mib.cur_channel);	
#endif

			if( padapter->_sys_mib.mac_ctrl.opmode &WIFI_SITE_MONITOR)
			{	
				DEBUG_ERR(("\n WIFI_SITE_MONITOR(0x%x) \n",padapter->_sys_mib.mac_ctrl.opmode));
				if (padapter->_sys_mib.sitesurvey_res.passive_mode == _TRUE)
	    	   		issue_probereq(padapter);

				_set_timer(&padapter->_sys_mib.survey_timer, SURVEY_TO);
				DEBUG_INFO(("mlme_thread: _set_timer(&padapter->_sys_mib.survey_timer, SURVEY_TO)\n"));
			}
			
			DEBUG_ERR(("\n-mlme_thread change chan (%d) ok\n",padapter->_sys_mib.cur_channel));
			padapter->_sys_mib.change_chan=_FALSE;
		}
		
#ifdef PLATFORM_LINUX
		if (signal_pending (current))
		{
			flush_signals(current);
       	}
#endif       	

	}

	_up_sema(&padapter->terminate_mlmethread_sema);

_func_exit_;	
#endif

#ifdef PLATFORM_LINUX
	complete_and_exit (NULL, 0);
#endif	

#ifdef PLATFORM_OS_XP
	PsTerminateSystemThread(STATUS_SUCCESS);
#endif

}

void report_join_res(_adapter *padapter,int res)
{
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	struct joinbss_event *joinbss_done =&( _sys_mib->joinbss_done);
	struct event_node *join_res_event = &(_sys_mib->join_res_event);
	
	DEBUG_INFO(("report joinbss with res=%d\n", res));
	_memcpy((unsigned char *)(&(joinbss_done->network.network)), &(_sys_mib->cur_network), sizeof (NDIS_WLAN_BSSID_EX));
	joinbss_done->network.join_res = joinbss_done->network.aid = cpu_to_le32( res );

	join_res_event->node = (unsigned char *)(joinbss_done) ;

	DEBUG_INFO(("joinbss_done->network.join_res=%x res=%x, joinbss_done=%p &( _sys_mib->joinbss_done)=%p join_res_event->node=0x%p\n", 
		joinbss_done->network.join_res, res, joinbss_done, &( _sys_mib->joinbss_done), join_res_event->node));

	join_res_event->evt_code = _JoinBss_EVT_;
	join_res_event->evt_sz = sizeof (joinbss_done);
	join_res_event->caller_ff_tail =  NULL;
	join_res_event->caller_ff_sz = 0;
	event_queuing(padapter, join_res_event);
}

void report_add_sta_event(_adapter *padater, unsigned char* MacAddr)
{
	struct mib_info *_sys_mib = &(padater->_sys_mib);
	struct stassoc_event *add_sta_done = &(_sys_mib->add_sta_done);
	struct event_node *add_sta_event = &(_sys_mib->add_sta_event);

	DEBUG_INFO(("C2H: add sta[%x %x: %x: %x: %x: %x]\n", 
		MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]));

	_memcpy(add_sta_done->macaddr, MacAddr, ETH_ALEN);

	add_sta_event->node = (unsigned char *)(add_sta_done);
	add_sta_event->evt_code = _AddSTA_EVT_;
	add_sta_event->evt_sz = sizeof(struct stassoc_event);
	add_sta_event->caller_ff_tail =  NULL;
	add_sta_event->caller_ff_sz = 0;
	event_queuing(padater, add_sta_event);
}


void report_del_sta_event(_adapter *padater, unsigned char* MacAddr)
{
	struct mib_info *_sys_mib = &(padater->_sys_mib);
	struct event_node *del_sta_event = &(_sys_mib->del_sta_event);
	struct stadel_event *prsp = NULL;
	volatile int old;
	volatile int *head = &(_sys_mib->del_sta_q.head);
	volatile int *tail = &(_sys_mib->del_sta_q.tail);
	
	DEBUG_INFO(("C2H: DEL sta[%x: %x: %x: %x: %x: %x]\n", 
		MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]));
	
	if ((CIRC_SPACE(*head, *tail , DEL_STA_QUEUE_SZ)) == 0)
	{
		DEBUG_ERR(("empty: %x, %x\n", *head, *tail));
		return;
	}
	
	prsp= (struct stadel_event *)(&(_sys_mib->del_sta_q.del_station[*head]));
	
	old = *head;
	*head = (*head + 1) & (DEL_STA_QUEUE_SZ - 1);
	
	_memcpy(prsp->macaddr, MacAddr, ETH_ALEN);
	del_sta_event->node = (unsigned char *)(prsp);
	del_sta_event->evt_code = _DelSTA_EVT_;
	del_sta_event->evt_sz = sizeof(struct stadel_event);
	del_sta_event->caller_ff_tail =  &(_sys_mib->del_sta_q.tail);
	del_sta_event->caller_ff_sz = DEL_STA_QUEUE_SZ;
	if (event_queuing(padater, del_sta_event) == FAIL)
	{
		*head = old;
		DEBUG_ERR(("\nevent_queuing(padater, del_sta_event) == FAIL\n"));
	}
}

void report_BSSID_info(struct rxirp *irp)
{
	struct event_node node;
	NDIS_WLAN_BSSID_EX	*prsp = NULL;
	_adapter *padapter = irp->adapter;
	struct mib_info *pmibinfo = &(padapter->_sys_mib);
	volatile int old;
	volatile int *head = &(pmibinfo->networks.head);
	volatile int *tail = &(pmibinfo->networks.tail);
	
	
	DEBUG_INFO(("report_BSSID_info\n"));
	if ((CIRC_SPACE(*head, *tail , NETWORK_QUEUE_SZ)) == 0)
	{
		DEBUG_ERR(("No enough space in network_queue for probe_rsp head=%d tail=%d \n", *head, *tail));
		
		return;
	}

	prsp= (NDIS_WLAN_BSSID_EX *)(&(pmibinfo->networks.networks[*head]));

	// copy the necessary fields to prsp
	
	if ((collect_bss_info(irp, prsp)) == SUCCESS)
	{
		old = *head;
		*head = (*head + 1) & (NETWORK_QUEUE_SZ - 1);
		node.node = (unsigned char *)prsp;
		node.evt_code = _Survey_EVT_;
		node.evt_sz = sizeof (*prsp);
		node.caller_ff_tail =  &(pmibinfo->networks.tail);
		node.caller_ff_sz = NETWORK_QUEUE_SZ;

		if ((event_queuing(padapter,&node)) == FAIL)
		{
			DEBUG_ERR(("report_BSSID_info: event_queuing fail\n"));
			*head = old;
		}

	}
}




int setup_bcnframe(_adapter *padapter, unsigned char *pframe)
{
	unsigned int	sz, rate_len;
	u8 erp_info	 = 0;
	u8 broadcast_sta[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct ieee80211_hdr_3addr *hdr = (struct ieee80211_hdr_3addr *)pframe;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	struct eeprom_priv *peepriv = &(padapter->eeprompriv);
	NDIS_WLAN_BSSID_EX *cur = &(_sys_mib->cur_network);
		
#ifdef CONFIG_RTL8192U
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	
	u8 ht_info_len = 0;
#endif
		
	sz = sizeof (struct ieee80211_hdr_3addr);
	
	memset(hdr, 0, sz);
	
	SetFrameSubType(&(hdr->frame_ctl), WIFI_BEACON);

	_memcpy(hdr->addr1, broadcast_sta, ETH_ALEN);
	_memcpy(hdr->addr2, myid(peepriv), ETH_ALEN);
	_memcpy(hdr->addr3, _sys_mib->cur_network.MacAddress, ETH_ALEN);
	
	sz += 8;	//timestamp will be inserted by hardware

	pframe += sz;

	//beacon interval : 2bytes
	_memcpy(pframe, (unsigned char *)(get_beacon_interval_from_ie(cur->IEs)), 2); 
	sz += 2; pframe += 2;

	//capability info
	_memcpy(pframe, (unsigned char *)(get_capability_from_ie(cur->IEs)), 2);
		
	sz += 2; pframe += 2;
	
	//SSID
	pframe = set_ie(pframe, _SSID_IE_, le32_to_cpu(cur->Ssid.SsidLength), cur->Ssid.Ssid, &sz);
	
	// supported rates...
	rate_len = get_rateset_len(cur->SupportedRates);
	pframe = set_ie(pframe, _SUPPORTEDRATES_IE_, ((rate_len > 8)? 8: rate_len), cur->SupportedRates, &sz);

	// DS parameter set
	pframe = set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&(cur->Configuration.DSConfig), &sz);

	_sys_mib->mac_ctrl.timie_offset = (unsigned int)(_sys_mib->mac_ctrl.pbcn) + sz;
	
	if (_sys_mib->mac_ctrl.opmode & WIFI_AP_STATE)
	{
		// TIM	
		*pframe = _TIM_IE_;
		*(pframe + 1)	=  3 + 8;
		*(pframe + 2) 	=  (_sys_mib->dtim_period - 1);
		*(pframe + 3) 	=  (_sys_mib->mac_ctrl.dtim) = _sys_mib->dtim_period;
		memset(pframe + 4, 9, 0);
		sz	+= 13;
	}
//	printk("Atim windows = %d \n", cur->Configuration.ATIMWindow);
//	cur->Configuration.ATIMWindow = 0;
//	if (_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)
//		pframe = set_ie(pframe, _IBSS_PARA_IE_, 2, (unsigned char *)&(cur->Configuration.ATIMWindow), &sz);

	//ERP INFOMATION
	if ((_sys_mib->network_type == WIRELESS_11G) || (_sys_mib->network_type == WIRELESS_11BG))
	{
		
		switch (_sys_mib->vcs_mode)
		{
			case 1: 
				erp_info |= BIT(1);
				break;
				
			case 2: 
				if (_sys_mib->dot11ErpInfo.protection)
				{
					erp_info |= BIT(1);
				}
				break;
				
			case 0:
			default:
				break;	
		}
			
		if (_sys_mib->dot11ErpInfo.nonErpStaNum)
		{
			erp_info |= BIT(0);
		}

		printk("ERP:%x\n", erp_info);
		pframe = set_ie(pframe, _ERPINFO_IE_, 1, &erp_info, &sz);
	}
	
	// EXTERNDED SUPPORTED RATE
	if (rate_len > 8)
	{
		pframe = set_ie(pframe, _EXT_SUPPORTEDRATES_IE_, (rate_len - 8), (cur->SupportedRates + 8), &sz);
	}
	

	if (_sys_mib->network_type == WIRELESS_11N_2G4)
	{

		// Only Support QoS in 11n mode
		pframe = set_ie(pframe, _RSN_IE_1_, _WMM_IE_Length_, WMM_IE,  &sz);
	

		
		// Set up HTCap element
		memset(&pHTInfo->SelfHTCap, 0 , sizeof(HT_CAPABILITY_ELE));			
		HTConstructCapabilityElement(padapter, &pHTInfo->SelfHTCap);

		pframe = set_ie(pframe,_HTCapability_IE,sizeof(HT_CAPABILITY_ELE) , (u8 *)&pHTInfo->SelfHTCap, &sz);		

		//Set up HTInfo element
		memset(&pHTInfo->SelfHTInfo, 0 , sizeof(HT_INFORMATION_ELE));			
		ht_info_len = HTConstructInfoElement(padapter, &pHTInfo->SelfHTInfo);

		pframe = set_ie(pframe,_HTInfo_IE,ht_info_len , (u8 *)&pHTInfo->SelfHTInfo, &sz);		
		
		
	  }
	
	
	return sz;
	
}


// reason code: -1 for deauthenication, -2 for disassocation
unsigned int receive_disconnect(struct rxirp *irp, int reason)
{
	u8 bool;
	//unsigned long flags;
	unsigned char *pframe;
	struct ieee80211_hdr *hdr;
	struct sta_data *psta;
	_adapter *padapter = irp->adapter;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	
	pframe = get_rxpframe(irp);
	hdr = (struct ieee80211_hdr*)pframe;

#ifdef CHECK_ADDR
	struct eeprom_priv *peepriv = &(padapter->eeprompriv);
	if (memcmp(myid(peepriv), get_da(pframe), ETH_ALEN))
	{
		return SUCCESS;
	}
#endif
	
	//check whether BSSID is correct
	if (memcmp(hdr->addr2, _sys_mib->cur_network.MacAddress, ETH_ALEN))
	{
		return SUCCESS;
	}
	
	psta = search_assoc_sta(hdr->addr2, padapter);
	if (psta == NULL)
	{
		return SUCCESS;
	}
	
	if (_sys_mib->mac_ctrl.opmode & WIFI_STATION_STATE)
	{
		_sys_mib->mac_ctrl.opmode &= ~(WIFI_ASOC_STATE);

		if (_sys_mib->join_req_ongoing)
		{
			_sys_mib->join_req_ongoing = 0;	
			
			//local_irq_save(flags);

			while(1) {
				_cancel_timer(&_sys_mib->reauth_timer, &bool);
				if(bool == _TRUE)
					break;
			}

			while(1) {
				_cancel_timer(&_sys_mib->reassoc_timer, &bool);
				if(bool == _TRUE)
					break;
			}

			//local_irq_restore(flags);
			
			//join_event C2H event
			report_join_res(padapter, -2);
		}
		else
		{
			report_del_sta_event(padapter, hdr->addr2);
		}
	} 
	else if (_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)
	{
		_sys_mib->mac_ctrl.opmode &= ~(WIFI_ASOC_STATE | WIFI_ADHOC_BEACON);
		
		report_del_sta_event(padapter, hdr->addr2);
	}

#ifdef TODO
	write8(padapter, MSR, MSR_LINK_ENEDCA|NOLINKMODE);
#endif

	write8(padapter, MSR, NOLINKMODE);

	del_assoc_sta(padapter, hdr->addr2);

	while(1) {
		_cancel_timer(&_sys_mib->disconnect_timer, &bool);
		if(bool == _TRUE)
			break;
	}

	return SUCCESS;
}



void survey_timer_hdl (PVOID FunctionContext)
{
       _adapter * padapter= (_adapter *)FunctionContext;
	struct mib_info	*_sys_mib=&padapter->_sys_mib;

_func_enter_;		
	DEBUG_ERR(("\n^^^survey_timer_hdl ^^^\n"));
	_sys_mib->ch_inx++;

	if ((_sys_mib->class_sets[_sys_mib->sur_regulatory_inx].channel_set[_sys_mib->ch_inx]) == 0) {

		_sys_mib->sur_regulatory_inx++;
		_sys_mib->ch_inx = 0;
	}
	
	if (_sys_mib->sur_regulatory_inx >= NUM_REGULATORYS)
	{
		DEBUG_ERR(("survey_timer_hdl: sur_regulatory_inx >= NUM_REGULATORYS\n"));
		goto survey_end;
	}

	if ((padapter->_sys_mib.class_sets[_sys_mib->sur_regulatory_inx].channel_set[_sys_mib->ch_inx]) == 0)
	{
		DEBUG_ERR(("survey_timer_hdl: _sys_mib.class_sets[_sys_mib->sur_regulatory_inx].channel_set[_sys_mib->ch_inx]) == 0\n"));
		goto survey_end;
	}

	_sys_mib->cur_channel = _sys_mib->class_sets[_sys_mib->sur_regulatory_inx].channel_set[_sys_mib->ch_inx] ;
	_sys_mib->cur_modem = _sys_mib->class_sets[_sys_mib->sur_regulatory_inx].modem;

#if 0	
	rtl8225z2_rf_set_chan(padapter, padapter->_sys_mib.cur_channel);	


	if (_sys_mib->sitesurvey_res.passive_mode == _TRUE)
	        issue_probereq(padapter);

	_set_timer(&_sys_mib->survey_timer, SURVEY_TO);
#endif
	_sys_mib->change_chan=_TRUE;

	_up_sema(&(padapter->cmdpriv.cmd_proc_sema));
_func_exit_;	
	return;

survey_end:
//restore the old state...
// issue survey_end event to host...
	
	DEBUG_ERR(("\n^^^survey_timer_hdl :survey end^^^\n"));
	
	_sys_mib->cur_channel =_sys_mib->old_channel;
	_sys_mib->cur_modem =_sys_mib->old_modem;

	//may change channel
	//DEBUG_ERR(("\n^^^survey_timer_hdl :cmd_clr_isr^^^\n"));
	//printk("\n^^^survey_timer_hdl :cmd_clr_isr^^^\n");
	//cmd_clr_isr(&padapter->cmdpriv);
	
	_sys_mib->change_chan=_TRUE;
	_up_sema(&(padapter->cmdpriv.cmd_proc_sema));
	
	padapter->_sys_mib.survey_done.bss_cnt = 0;	
	padapter->_sys_mib.survey_done_event.node=(u8 *)(&padapter->_sys_mib.survey_done);
	padapter->_sys_mib.survey_done_event.evt_code = _SurveyDone_EVT_;
	padapter->_sys_mib.survey_done_event.evt_sz = sizeof (struct surveydone_event);
	padapter->_sys_mib.survey_done_event.caller_ff_tail =  NULL;
	padapter->_sys_mib.survey_done_event.caller_ff_sz = 0;

	event_queuing(padapter,&padapter->_sys_mib.survey_done_event);

	_sys_mib->sitesurvey_res.state = _FALSE;

	//john debug 0521
	_sys_mib->mac_ctrl.opmode &= (~WIFI_SITE_MONITOR);

_func_exit_;	

}

void reauth_timer_hdl(PVOID FunctionContext)
{
	//unsigned long flags;
	_adapter * padapter = (_adapter *)FunctionContext;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	//local_irq_save(flags);

	if (++_sys_mib->reauth_count > REAUTH_LIMIT)
	{
		DEBUG_ERR(("Client Auth time-out!\n"));
		//local_irq_restore(flags);
#if 0
		SelectChannel(padapter, _sys_mib->cur_channel);
		report_join_res(padapter, -1);
#else

		_sys_mib->change_chan=_TRUE;
		DEBUG_ERR(("\n^^^ reauth_timer_hdl  _up_sema(&(padapter->cmdpriv.cmd_proc_sema)) ^^^\n"));
		_up_sema(&(padapter->cmdpriv.cmd_proc_sema));
#endif
		return;
	}

	if (_sys_mib->mac_ctrl.opmode & WIFI_AUTH_SUCCESS)
	{
		//local_irq_restore(flags);
		return;
	}

	_sys_mib->auth_seq = 1;
	_sys_mib->mac_ctrl.opmode &= ~(WIFI_AUTH_STATE1);
	_sys_mib->mac_ctrl.opmode |= WIFI_AUTH_NULL;

	printk("auth timeout, sending auth req again\n");
	issue_auth(padapter, NULL, 0);

	_set_timer(&_sys_mib->reauth_timer, SURVEY_TO);

	//local_irq_restore(flags);
	return;
}

void reassoc_timer_hdl(PVOID FunctionContext)
{
	//unsigned long flags;
	_adapter * padapter = (_adapter *)FunctionContext;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	//local_irq_save(flags);

	if (++_sys_mib->reassoc_count > REASSOC_LIMIT)
	{
		DEBUG_ERR(("Client Assoc time-out!\n"));
		//local_irq_restore(flags);
		report_join_res(padapter, -2);
		return;
	}

	if (_sys_mib->mac_ctrl.opmode & WIFI_ASOC_STATE)
	{
		//local_irq_restore(flags);
		return;
	}

	DEBUG_ERR(("assoc timeout, sending assoc req again\n"));
	issue_assocreq(padapter);

	_set_timer(&(_sys_mib->reassoc_timer), REASSOC_TO);
	//local_irq_restore(flags);

}

void disconnect_timer_hdl(PVOID FunctionContext)
{
	//unsigned long		flags;
	//struct sta_data	*psta;
	//struct list_head	*node, *tmp;
	_adapter			*padapter	= (_adapter *)FunctionContext;
	struct mib_info	*_sys_mib	= &(padapter->_sys_mib);
	
	//local_irq_save(flags);

#if 0
	list_for_each_safe(node, tmp, (struct list_head*)&(_sys_mib->assoc_entry))
	{
		psta = list_entry(node, struct sta_data, assoc);
		
		if (psta->stats->rx_pkts == 0)
		{
			printk("disconnect timer\n");
			report_del_sta_event(padapter, psta->info2->addr);
			del_assoc_sta(padapter, psta->info2->addr);
		}
		else
		{
			psta->stats->rx_pkts = 0;
		}
	}
#endif
	
	_set_timer(&(_sys_mib->disconnect_timer), DISCONNECT_TO);
#ifdef CONFIG_ERP
	_sys_mib.erp_timer.time_out = ERP_TO;
#endif	
	//local_irq_restore(flags);

}

#ifdef CONFIG_RTL8192U
void sendbeacon_timer_hdl(PVOID FunctionContext)
{

	_adapter			*padapter	= (_adapter *)FunctionContext;
	struct mib_info	*_sys_mib	= &(padapter->_sys_mib);
	u16 beacon_interval = get_beacon_interval(&_sys_mib->cur_network);
	
	DEBUG_INFO(("===============sendbeacon_timer_hdl: intvl: %d \n", beacon_interval));

	
	if(_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)
	{
		issue_beacon(padapter);
		_set_timer(&padapter->_sys_mib.sendbeacon_timer, (beacon_interval*1024)/1000);
	}
}
#endif

void start_clnt_auth(_adapter* padapter)
{
	u8 bool;
	//unsigned long flags;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	
	//local_irq_save(flags);

	_sys_mib->mac_ctrl.opmode &= ~(WIFI_AUTH_SUCCESS | WIFI_AUTH_STATE1 | WIFI_ASOC_STATE);
	_sys_mib->mac_ctrl.opmode |= WIFI_AUTH_NULL;
	
	//the WAP_config tx encry may be turned off by the driver
	//rtl_outw(WPA_CONFIG, BIT(2) | BIT(1) | BIT(0));
			
	_sys_mib->reauth_count = 0;
	_sys_mib->reassoc_count = 0;
	_sys_mib->auth_seq = 1;

	while(1) {
		_cancel_timer(&_sys_mib->reauth_timer, &bool);
		if(bool == _TRUE)
			break;
	}

	while(1) {
		_cancel_timer(&_sys_mib->reassoc_timer, &bool);
		if(bool == _TRUE)
			break;
	}

#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv))
#endif

	DEBUG_INFO(("start_clnt_auth: _sys_mib->cur_channel=%d\n", _sys_mib->cur_channel));

	//SwChnl(_sys_mib->cur_channel);
	SelectChannel(padapter, _sys_mib->cur_channel);

	_set_timer(&_sys_mib->reauth_timer,REAUTH_TO);
	
	printk("start sending auth req\n");
	
	issue_auth(padapter, NULL, 0);

	//local_irq_restore(flags);
}


void start_clnt_join(_adapter* padapter)
{
//	unsigned long	flags;
	unsigned short val16;
	unsigned char *addr;
	unsigned short ctrl = 0;
	unsigned char	*key = NULL;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
//	u32 i = 0 ;
	padapter->_sys_mib.old_channel = _sys_mib->cur_channel;
	padapter->_sys_mib.old_modem   = _sys_mib->cur_modem;

	printk("start join bss\n");
	
	val16 = get_capability(&(_sys_mib->cur_network));
	if ((get_bsstype(val16)) == WIFI_AP_STATE)
	{
		_sys_mib->mac_ctrl.opmode = WIFI_STATION_STATE;
		
#ifdef UNIVERSAL_REPEATER
		if (IS_ROOT_INTERFACE(priv))
#endif
			
		//marc: to allocate the CAM_ID 4
		ctrl = BIT(15) | BIT(5);
		ctrl |= ((unsigned short) _sys_mib->mac_ctrl.defkeytype);
		key = null_key;
		// TODO: CAM?
		setup_assoc_sta(broadcast_sta, 4, -1, ctrl, key, padapter);
	
		//marc: to allocate the CAM_ID 5
		ctrl = BIT(15) | BIT(6);
		if (_sys_mib->mac_ctrl.security < 2)
		{
			ctrl |= BIT(5);
		}
		ctrl |= ((unsigned short) _sys_mib->mac_ctrl.defkeytype);
		key = null_key;
		addr = _sys_mib->cur_network.MacAddress; 
			
		setup_assoc_sta(addr, 5, 0, ctrl, key, padapter);
			
		start_clnt_auth(padapter);
		return;
	}
	else if ((get_bsstype(val16)) == WIFI_ADHOC_STATE)
	{
		printk("Ask for Ad-HoC Mode\n");
		_sys_mib->mac_ctrl.opmode = WIFI_ADHOC_STATE;
		//update_bss(&pmib->dot11StationConfigEntry, &pmib->dot11Bss);
		//pmib->dot11RFEntry.dot11channel = pmib->dot11Bss.channel;
		//marc: to allocate the CAM_ID 4
		ctrl = BIT(15) | BIT(6) | BIT(5);
		ctrl |= ((unsigned short) _sys_mib->mac_ctrl.defkeytype);
		key = null_key;
			
		setup_assoc_sta(broadcast_sta, 4, -1, ctrl, key, padapter);
	
		//marc: to allocate the CAM_ID 5
		ctrl = BIT(15);
		if (_sys_mib->mac_ctrl.security < 2)
		{
			ctrl |= BIT(5);
		}
	
		ctrl |= ((unsigned short) _sys_mib->mac_ctrl.defkeytype);
		key = null_key;
		addr = _sys_mib->cur_network.MacAddress; 
			
		setup_assoc_sta(addr, 5, -1, ctrl, key, padapter);
			
		SelectChannel(padapter, _sys_mib->cur_channel);
	#ifdef CONFIG_RTL8187
		join_bss(padapter);
	#endif
		_sys_mib->join_req_ongoing = 0;
			
		_sys_mib->mac_ctrl.opmode |= WIFI_ASOC_STATE;
	
		//local_irq_save(flags);

#ifdef CONFIG_RTL8187
		_sys_mib->join_res = STATE_Sta_Ibss_Active;
		
#elif defined(CONFIG_RTL8192U)

		//Wait for Beacon from Ad hoc master
		_sys_mib->join_res = STATE_Sta_Join_Wait_Beacon;
#endif

		_set_timer(&(_sys_mib->disconnect_timer), DISCONNECT_TO);
		
		//TODO: MAC do not handle the multicast frame, need a multicast CAM entry in the future
		//rtl_outw(WPA_CONFIG, BIT(2) | BIT(1) | BIT(0));
		
		//local_irq_restore(flags);

		report_join_res(padapter, 1);
		return;
	}
	else
	{
		DEBUG_ERR(("invalid cap:%x\n", val16));
		return;
	}
}

#ifdef CONFIG_RTL8192U

void join_ibss(_adapter *padapter,u8 * pframe,u32 irp_len)
{
	//unsigned long flags;
	u8	msr,val, slot_time_val;
	//u16 BcnCW = 6, BcnIFS = 0xf, BcnTimeCfg = 0;
	//u32 regTemp,regRCR, i = 0 ;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	unsigned char *assoc_bssid = _sys_mib->cur_network.MacAddress;
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	PNDIS_WLAN_BSSID_EX	pcur_network = &_sys_mib->cur_network;
	DEBUG_ERR(("JOIN_ibss !!!!\n"));	


	// 1th:
	// Clear MSR
	//
	
	if(_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE){

		msr = 0;
		msr &= 0xfc;
		write8(padapter, MSR, msr|NOLINKMODE);
	}

	// 2rd:
	// Set Basic Rate
	//
	//Being done in  update_supported_rate() , OnBeacon
	
	//	Hal_SetHwReg819xUsb(padapter,HW_VAR_BASIC_RATE,&val);

	printk("BSSID = %x %x %x %x %x %x\n",assoc_bssid[0],assoc_bssid[1],assoc_bssid[2],assoc_bssid[3],assoc_bssid[4],assoc_bssid[5]);


	// 3rd:
	// Set BSSID.
	//
	write16(padapter, BSSIDR,((u16 *)assoc_bssid)[0] );
	write32(padapter, BSSIDR+2 , ((u32 *)assoc_bssid+2)[0]);

	// 4th:
	// Set Beacon related registers
	//
	SetBeaconRelatedRegisters819xUsb(padapter, 2);

	//5th:
	//HW_VAR_MEDIA_STATUS: IBSS mode
	//
	Hal_SetHwReg819xUsb(padapter, HW_VAR_MEDIA_STATUS, &val);
	
	// TODO: check BSSID
	//6th:
	//HW_VAR_CECHK_BSSID : 
	//
#if 0 
	val = _TRUE;
	Hal_SetHwReg819xUsb(padapter, HW_VAR_CECHK_BSSID, &val);
#endif

	//7th:
	//Set HT related capabilities
	//
	if(1)//_sys_mib->cur_network.BssHT.bdSupportHT && pHTInfo->bEnableHT )
	{

		if(_sys_mib->network_type &( WIRELESS_11B ||WIRELESS_11G) )
			_sys_mib->network_type = WIRELESS_11N_2G4;

		else if(_sys_mib->network_type == WIRELESS_11A )
			_sys_mib->network_type = WIRELESS_11N_5G;

		pHTInfo->bCurrentHTSupport = _TRUE;
		DEBUG_INFO(("Join_ibss : current wireless mode : %d\n ",_sys_mib->network_type));

	}
	else
		pHTInfo->bCurrentHTSupport = _FALSE;


	if(pHTInfo->bCurrentHTSupport)
	{
		//Only support QoS  in 11n mode
		pcur_network->BssQos.bdQoSMode |= QOS_WMM;
		HTOnAssocRsp(padapter, pframe,irp_len);

	}
	else	
	{
		HTSetConnectBwMode(padapter,  HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);
		memset(pHTInfo->dot11HTOperationalRateSet, 0 , 16);
	}

	//8th:
	// QoS related value:
	//
	ActSetWirelessMode819xUsb(padapter, _sys_mib->network_type);




	// 9th:
	// Update RATR Table
	//
	SetHalRATRTable819xUsb(padapter, _sys_mib->mac_ctrl.datarate,
									pHTInfo->dot11HTOperationalRateSet);
	

	switch(_sys_mib->network_type)
	{
		case WIRELESS_11B:
			_sys_mib->rate = 110;
			printk(KERN_INFO"Using B rates\n");
			break;
		case WIRELESS_11G:
		case WIRELESS_11BG:
		case WIRELESS_11N_2G4:
		{

			
	//10th:
	// Set Short slot time
	//			
	//
	// Disable Short slot time. Added by Annie, 2005-11-17.
	// 
	// Ref: 802.11g/D8.2, sec. 7.3.1.4, p13. First line...
	// 	"For IBSS, the Short Slot Time subfield shall be set to 0."
	//
			if (_sys_mib->mac_ctrl.opmode  & WIFI_ADHOC_STATE)
			{
				slot_time_val = NON_SHORT_SLOT_TIME;
				Hal_SetHwReg819xUsb(padapter, HW_VAR_SLOT_TIME, &slot_time_val);
			}
			
			_sys_mib->rate = 540;
			
			DEBUG_ERR(("Using 54 M  rate\n"));
		}
			break;
	}


	if ((_sys_mib->mac_ctrl.opmode  & WIFI_ADHOC_STATE) ||
		(_sys_mib->mac_ctrl.opmode & WIFI_AP_STATE)) {

		_sys_mib->mac_ctrl.bcn_size = setup_bcnframe(padapter, _sys_mib->mac_ctrl.pbcn);
		
		DEBUG_ERR(("bcn size: %d\n", _sys_mib->mac_ctrl.bcn_size));
		DEBUG_ERR(("set send beacon timer \n"));
		_set_timer(&padapter->_sys_mib.sendbeacon_timer, 0);


#ifdef CONFIG_BEACON_CNT
		_sys_mib->bcnlimit  = CONFIG_BEACON_CNT;
#else
		_sys_mib->bcnlimit  = 0;
#endif	
				
	
	}

	
	
}


#endif



void start_create_bss(_adapter *padapter)
{
	unsigned short val16;
	//unsigned long flags;
	unsigned char	*addr, *dummy_ptr = NULL;
	unsigned short ctrl = 0;
	unsigned char	*key = NULL;
	u32 dummy_length = 0;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	
	val16 = get_capability(&(_sys_mib->cur_network));
	if ((get_bsstype(val16)) == WIFI_AP_STATE)
	{
	}
	else
	{
		printk("Ad-Hoc create bss\n");
		
		//marc: to allocate the CAM_ID 4
		ctrl = BIT(15) | BIT(6) | BIT(5);
		ctrl |= ((unsigned short) _sys_mib->mac_ctrl.defkeytype);
		key = null_key;
			
		setup_assoc_sta(broadcast_sta, 4, -1, ctrl, key, padapter);
	
		//marc: to allocate the CAM_ID 5
		ctrl = BIT(15);
		if (_sys_mib->mac_ctrl.security < 2)
		{
			ctrl |= BIT(5);
		}
	
		ctrl |= ((unsigned short) _sys_mib->mac_ctrl.defkeytype);
		key = null_key;
		addr = get_bssid(&(padapter->mlmepriv));
			
		setup_assoc_sta(addr, 5, -1, ctrl, key, padapter);
		
		//TODO: MAC do not handle the multicast frame, need a multicast CAM entry in the future
		//rtl_outw(WPA_CONFIG, BIT(2) | BIT(1) | BIT(0));
		
		//local_irq_save(flags);
		
		_sys_mib->mac_ctrl.opmode = WIFI_ADHOC_STATE;
	
		SelectChannel(padapter, _sys_mib->cur_channel);
#ifdef CONFIG_RTL8187
		join_bss(padapter);
#elif defined(CONFIG_RTL8192U)	
		DEBUG_ERR(("==== BEFORE JOIN_IBSS \n"));
		join_ibss(padapter, dummy_ptr, dummy_length);
#endif	
		_sys_mib->mac_ctrl.opmode |= WIFI_ASOC_STATE;
	
		_set_timer(&(_sys_mib->disconnect_timer), DISCONNECT_TO);

#ifdef CONFIG_ERP		
		_sys_mib->erp_timer.time_out = ERP_TO;
#endif
		
		//local_irq_restore(flags);
		
	}
}


void issue_probereq(_adapter *padapter)
{
	unsigned char *pframe;
	struct ieee80211_hdr *hdr;
	unsigned short *fctrl;
	unsigned char 	bssrate[NumRates];
	int		bssrate_len;
	unsigned char	*mac;

	struct txirp *irp = alloc_txirp(padapter);

	u8* debug_pframe;

	if (irp == NULL)
	{
		printk("Can't alloc irp %d:%s\n", __LINE__, __FILE__);
		return;
	}

	mac = myid(&(padapter->eeprompriv));

	pframe = get_txpframe(irp);
	debug_pframe = pframe;

	hdr = (struct ieee80211_hdr *)pframe;

	fctrl = &(hdr->frame_ctl);
	*(fctrl) = 0;

	_memcpy(hdr->addr1, broadcast_sta, ETH_ALEN);
	_memcpy(hdr->addr2, mac, ETH_ALEN);
	_memcpy(hdr->addr3, broadcast_sta, ETH_ALEN);

	hdr->seq_ctl = 0;
	SetFrameSubType(pframe, WIFI_PROBEREQ);
	
	pframe += sizeof (struct ieee80211_hdr_3addr);
	
	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);
	
	pframe = set_ie(pframe, _SSID_IE_, padapter->_sys_mib.sitesurvey_res.ss_ssidlen, padapter->_sys_mib.sitesurvey_res.ss_ssid, &(irp->tx_len));	
	
	get_bssrate_set(padapter, _SUPPORTEDRATES_IE_, bssrate, &bssrate_len);
	
	pframe = set_ie(pframe, _SUPPORTEDRATES_IE_ , bssrate_len , bssrate, &(irp->tx_len));

	get_bssrate_set(padapter, _EXT_SUPPORTEDRATES_IE_, bssrate, &bssrate_len);

	if (bssrate_len)
		pframe = set_ie(pframe, _EXT_SUPPORTEDRATES_IE_ , bssrate_len , bssrate, &(irp->tx_len));

	DEBUG_INFO(("issuing probe_req, tx_len=%d\n", irp->tx_len));
	wlan_tx(padapter, irp);

	DEBUG_INFO(("issuing probe_req end\n"));
}

static __inline unsigned char get_band(_adapter *padapter)
{
	if (padapter->_sys_mib.cur_channel > 14)
		return BAND_A;
	else
		return BAND_G;
}


void issue_probersp(_adapter *padapter, unsigned char *da, int set_privacy)
{
	unsigned char	*bssid,*pbuf;
	struct ieee80211_hdr *hdr;
	unsigned char	val8;
	unsigned int rate_len;
	unsigned short *fctrl;
	struct txirp *irp = alloc_txirp(padapter);
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	NDIS_WLAN_BSSID_EX *cur = &(_sys_mib->cur_network);

#ifdef CONFIG_RTL8192U

	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	
	u8 ht_info_len = 0;
#endif

	if (irp == NULL)
	{
		printk("Can't alloc tx irp %d:%s\n", __LINE__, __FILE__);
		return;
	}
	
	bssid = get_bssid(&(padapter->mlmepriv));
	pbuf = get_txpframe(irp);
	hdr = (struct ieee80211_hdr *)pbuf;
   	fctrl = &(hdr->frame_ctl);
   	*(fctrl) = 0;
	pbuf += sizeof (struct ieee80211_hdr_3addr);
   	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);

	pbuf += _TIMESTAMP_;
	irp->tx_len += _TIMESTAMP_;

	//beacon interval : 2bytes
	_memcpy(pbuf, (unsigned char *)(get_beacon_interval_from_ie(cur->IEs)), 2); 
	irp->tx_len += 2; pbuf += 2;

	//capability info
	_memcpy(pbuf, (unsigned char *)(get_capability_from_ie(cur->IEs)), 2);
	irp->tx_len += 2; pbuf += 2;
	
	pbuf = set_ie(pbuf, _SSID_IE_, le32_to_cpu(cur->Ssid.SsidLength), cur->Ssid.Ssid, &(irp->tx_len));

	// supported rates...
	rate_len = get_rateset_len(cur->SupportedRates);
	pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, ((rate_len > 8)? 8: rate_len), cur->SupportedRates, &(irp->tx_len));

	// DS parameter set
	pbuf = set_ie(pbuf, _DSSET_IE_, 1, (unsigned char *)&(cur->Configuration.DSConfig), &(irp->tx_len));

#if 0
	// TIM
	*pbuf = _TIM_IE_;
	*(pbuf + 1)	=  3 + 8;
	*(pbuf + 2) 	=  (sys_mib.dtim_period - 1);
	*(pbuf + 3) 	=  sys_mib.dtim_period;
	memset(pbuf + 4, 9, 0);
	irp->tx_len += 13;
#endif

	if (_sys_mib->mac_ctrl.opmode & WIFI_AP_STATE) {
		// TIM
		*pbuf = _TIM_IE_;
		*(pbuf + 1)	=  3 + 8;
		*(pbuf + 2) 	=  (_sys_mib->dtim_period - 1);
		*(pbuf + 3) 	=  _sys_mib->dtim_period;
		memset(pbuf + 4, 9, 0);
		irp->tx_len += 13;
		
		if (get_band(padapter) & BAND_G) {
		 	//ERP infomation.
			val8=0;
			if (_sys_mib->dot11ErpInfo.protection)
				val8 |= BIT(1);
			if (_sys_mib->dot11ErpInfo.nonErpStaNum)
				val8 |= BIT(0);
			pbuf = set_ie(pbuf, _ERPINFO_IE_ , 1 , &val8, &irp->tx_len);
		}
	}

	if (_sys_mib->dot11RsnIE.rsnielen && set_privacy)
	{
		_memcpy(pbuf, _sys_mib->dot11RsnIE.rsnie, _sys_mib->dot11RsnIE.rsnielen);
		pbuf += _sys_mib->dot11RsnIE.rsnielen;
		irp->tx_len += _sys_mib->dot11RsnIE.rsnielen;
	}

#if 0
	// Realtek proprietary IE
	if ((_sys_mib->mac_ctrl.opmode & WIFI_AP_STATE) && (priv->pmib->miscEntry.turbo_mode&0xff) != TURBO_OFF)
		pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, &irp->tx_len);

	if (priv->pmib->miscEntry.private_ie_len) {
		_memcpy(pbuf, pmib->miscEntry.private_ie, pmib->miscEntry.private_ie_len);
		pbuf += pmib->miscEntry.private_ie_len;
		txinsn.fr_len += pmib->miscEntry.private_ie_len;
	}
#endif

	if (rate_len > 8)
	{
		pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, (rate_len - 8), (cur->SupportedRates + 8), &(irp->tx_len));
	}


	if (_sys_mib->network_type == WIRELESS_11N_2G4)
	{

	// Only Support QoS in 11n mode
		pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_IE_Length_, WMM_IE,  &(irp->tx_len));
		
				
	// Set up HTCap element
		memset(&pHTInfo->SelfHTCap, 0 , sizeof(HT_CAPABILITY_ELE));			
		HTConstructCapabilityElement(padapter, &pHTInfo->SelfHTCap);

		pbuf = set_ie(pbuf,_HTCapability_IE,sizeof(HT_CAPABILITY_ELE) , (u8 *)&pHTInfo->SelfHTCap, &(irp->tx_len));		

	//Set up HTInfo element
		memset(&pHTInfo->SelfHTInfo, 0 , sizeof(HT_INFORMATION_ELE));			
		ht_info_len = HTConstructInfoElement(padapter, &pHTInfo->SelfHTInfo);

		pbuf = set_ie(pbuf,_HTInfo_IE,ht_info_len , (u8 *)&pHTInfo->SelfHTInfo, &(irp->tx_len));		
	  }
	
	
	

	SetFrameSubType(fctrl, WIFI_PROBERSP);

	_memcpy((void *)hdr->addr1, da, ETH_ALEN);
	_memcpy((void *)hdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_memcpy((void *)hdr->addr3, bssid, ETH_ALEN);
	
	wlan_tx(padapter, irp);

}


void issue_beacon(_adapter *padapter)
{
	unsigned char	*pbuf;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	struct txirp *irp = alloc_txirp(padapter);

	//printk("issue_beacon\n");
    	if (irp == NULL)
	{
		printk("Can't alloc tx irp %d:%s\n", __LINE__, __FILE__);
		return;
	}
	
	irp->tx_len = _sys_mib->mac_ctrl.bcn_size;
	pbuf = get_txpframe(irp);
	_memcpy(pbuf, _sys_mib->mac_ctrl.pbcn, irp->tx_len);
	
	wlan_tx(padapter, irp);

}


void issue_disassoc(_adapter *padapter, unsigned char *da, int reason)
{
	unsigned char	*bssid, *pbuf;
	unsigned short  val;
	struct ieee80211_hdr *hdr;
	unsigned short *fctrl;

	struct txirp *irp = alloc_txirp(padapter);

	if (irp == NULL)
	{
		printk("Can't alloc tx irp %d:%s\n", __LINE__, __FILE__);
		return;
	}

	pbuf = get_txpframe(irp);
  	hdr = (struct ieee80211_hdr *)pbuf;
   	fctrl = &(hdr->frame_ctl);
   	*(fctrl) = 0;

	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);
	pbuf += sizeof (struct ieee80211_hdr_3addr);
	bssid = get_bssid(&(padapter->mlmepriv));

	val = cpu_to_le16(reason);

	pbuf = set_fixed_ie(pbuf, _RSON_CODE_, (unsigned char *)&val, &(irp->tx_len));

	//SetFrameType(fctrl, WIFI_MGT_TYPE);
	SetFrameSubType(fctrl, WIFI_DISASSOC);

	_memcpy((void *)hdr->addr1, da, ETH_ALEN);
	_memcpy((void *)hdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_memcpy((void *)hdr->addr3, get_bssid(&(padapter->mlmepriv)), ETH_ALEN);

	wlan_tx(padapter, irp);


}


// if pstat == NULL, indiate we are station now...
void issue_auth(_adapter *padapter, struct sta_data *pstat, unsigned short status)
{
	unsigned char	*pframe;
	struct ieee80211_hdr *hdr;
	unsigned short *fctrl;
	unsigned short  val;
	int use_shared_key = 0;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	struct txirp *irp = alloc_txirp(padapter);

	if (irp == NULL)
	{
		printk("Can't alloc tx irp %d:%s\n", __LINE__, __FILE__);
		return;
	}

	pframe = get_txpframe(irp);
  	hdr = (struct ieee80211_hdr *)pframe;
   	fctrl = &(hdr->frame_ctl);
   	*(fctrl) = 0;
	hdr->seq_ctl = 0;
	if (pstat)	// for AP mode
	{
	
		_memcpy(hdr->addr1, get_sta_addr(pstat), ETH_ALEN);
		_memcpy(hdr->addr2, _sys_mib->cur_network.MacAddress, ETH_ALEN);
	}
	else
	{
		_memcpy(hdr->addr1, _sys_mib->cur_network.MacAddress, ETH_ALEN);
		_memcpy(hdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	}

	_memcpy(hdr->addr3, _sys_mib->cur_network.MacAddress, ETH_ALEN);
	
	SetFrameSubType(pframe,WIFI_AUTH);
	
   	pframe += sizeof (struct ieee80211_hdr_3addr);
   	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);

	// setting auth algm number
	/* In AP mode,	if auth is set to shared-key, use shared key
	 *		if auth is set to auto, use shared key if client use shared
	 *		otherwise set to open
	 * In client mode, if auth is set to shared-key or auto, and WEP is used,
	 *		use shared key algorithm
	 */
	val = 0;
	if (padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm > 0) 
	{
		if (pstat) 
		{
			if (padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm == 1) // shared key
				val = 1;
			else // auto mode, check client algorithm
			{ 
				if (pstat && (get_sta_authalgrthm(pstat))) //soft AP
					val = 1;
			}
		}
		else // client mode, use shared key if WEP is enabled
		{ 
			if (padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm == 1) 
			{ // shared-key ONLY
				//if (dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
				//	dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)
					val = 1;
			}
			else 
			{ // auto-auth
				if (padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm == _WEP_40_PRIVACY_ ||
					padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm == _WEP_104_PRIVACY_) 
				{
					if (_sys_mib->auth_seq == 1)
						_sys_mib->authModeToggle = (_sys_mib->authModeToggle ? 0 : 1);

					if (_sys_mib->authModeToggle)
						val = 1;
				}
			}
		}

		if (val) {
			val = cpu_to_le16(val);
			use_shared_key = 1;
		}
	}

	if (pstat && (status != _STATS_SUCCESSFUL_))
		val = cpu_to_le16(pstat->AuthAlgrthm);

	if ((_sys_mib->auth_seq == 3) && ((_sys_mib->mac_ctrl.opmode) & WIFI_AUTH_STATE1) && use_shared_key)
	{
		unsigned val32;
		val32 = cpu_to_le32((iv ++) | (_sys_mib->mac_ctrl.defkeyid << 30));
		pframe = set_fixed_ie(pframe, 4, (unsigned char *)&val32, &(irp->tx_len));
	}
	
	pframe = set_fixed_ie(pframe, _AUTH_ALGM_NUM_, (unsigned char *)&val, &(irp->tx_len));

	// setting transaction sequence number...
	if (pstat)
		val = cpu_to_le16(pstat->auth_seq);
	else
		val = cpu_to_le16(_sys_mib->auth_seq);

	pframe = set_fixed_ie(pframe, _AUTH_SEQ_NUM_, (unsigned char *)&val, &(irp->tx_len));

	// setting status code...
	val = cpu_to_le16(status);
	pframe = set_fixed_ie(pframe, _STATUS_CODE_, (unsigned char *)&val, &(irp->tx_len));

	// then checking to see if sending challenging text...
	if (pstat)
	{
		if ((pstat->auth_seq == 2) && (pstat->state & WIFI_AUTH_STATE1) && use_shared_key)
			pframe = set_ie(pframe, _CHLGETXT_IE_, 128, pstat->chg_txt, &(irp->tx_len));
	}
	else
	{	
		if ((_sys_mib->auth_seq == 3) && (_sys_mib->mac_ctrl.opmode & WIFI_AUTH_STATE1) && use_shared_key)
		{
			pframe = set_ie(pframe, _CHLGETXT_IE_, 128, chg_txt, &(irp->tx_len));
			SetPrivacy(fctrl);
			printk("sending a privacy pkt with auth_seq=%d\n", _sys_mib->auth_seq);
		}
	}

	wlan_tx(padapter, irp);

}



void issue_asocrsp(_adapter *padapter, unsigned short status, struct sta_data *pstat, int pkt_type)
{
	unsigned short val;
	struct ieee80211_hdr *hdr;
	unsigned char	*bssid,*pbuf;
	unsigned short *fctrl;
	struct txirp *irp = alloc_txirp(padapter);

	if (irp == NULL)
	{
		printk("Can't alloc tx irp %d:%s\n", __LINE__, __FILE__);
		return;
	}

	pbuf = get_txpframe(irp);
  	hdr = (struct ieee80211_hdr *)pbuf;
   	fctrl = &(hdr->frame_ctl);
   	*(fctrl) = 0;


	pbuf += sizeof (struct ieee80211_hdr_3addr);
   	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);

	bssid = get_bssid(&(padapter->mlmepriv));


	_memcpy((void *)hdr->addr1, get_sta_addr(pstat), ETH_ALEN);
	_memcpy((void *)hdr->addr2, bssid, ETH_ALEN);
	_memcpy((void *)hdr->addr3, bssid, ETH_ALEN);	


	val = cpu_to_le16(BIT(0));

	if (padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm)
		val |= cpu_to_le16(BIT(4));

	if (padapter->_sys_mib.rf_ctrl.shortpreamble)//SHORTPREAMBLE
		val |= cpu_to_le16(BIT(5));

	if (padapter->_sys_mib.dot11ErpInfo.shortSlot)
		val |= cpu_to_le16(BIT(10));

	pbuf = set_fixed_ie(pbuf, _CAPABILITY_ , (unsigned char *)&val, &(irp->tx_len));

	status = cpu_to_le16(status);
	
	pbuf = set_fixed_ie(pbuf , _STATUS_CODE_ , (unsigned char *)&status, &(irp->tx_len));

	val = cpu_to_le16(pstat->info2->aid | 0xC000);
	pbuf = set_fixed_ie(pbuf, _ASOC_ID_ , (unsigned char *)&val, &(irp->tx_len));

	if (pstat->bssratelen <= 8)
		pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, pstat->bssratelen, pstat->bssrateset, &(irp->tx_len));
	else {
		pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, 8, pstat->bssrateset, &(irp->tx_len));
		pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, pstat->bssratelen-8, pstat->bssrateset+8, &(irp->tx_len));
	}

	if ((pkt_type == WIFI_ASSOCRSP) || (pkt_type == WIFI_REASSOCRSP))
		SetFrameSubType(fctrl, pkt_type);
	else
		return;
		
	wlan_tx(padapter, irp);

}


void site_survey(_adapter *padapter)
{
	struct mib_info *_sys_mib = &padapter->_sys_mib;
	_sys_mib->old_channel=padapter->_sys_mib.cur_channel;
	_sys_mib->old_modem=padapter->_sys_mib.cur_modem;

	_sys_mib->sur_regulatory_inx =_sys_mib->ch_inx = 0;
	padapter->_sys_mib.cur_channel =padapter->_sys_mib.class_sets[_sys_mib->sur_regulatory_inx ].channel_set[_sys_mib->ch_inx];
	padapter->_sys_mib.cur_modem = padapter->_sys_mib.class_sets[padapter->_sys_mib.sur_regulatory_inx ].modem;
	DEBUG_INFO(("\n in  site_survey ::modem: %x\n", padapter->_sys_mib.class_sets[_sys_mib->sur_regulatory_inx ].modem));

	padapter->_sys_mib.mac_ctrl.opmode |=WIFI_SITE_MONITOR;	
	_sys_mib->sitesurvey_res.bss_cnt = 0;
#ifdef CONFIG_RTL8192U

	DEBUG_INFO(("site_survey: current channel: %d", padapter->_sys_mib.cur_channel));

	rtl8256_rf_set_chan(padapter, padapter->_sys_mib.cur_channel);	
	
#elif defined(CONFIG_RTL8187)

	rtl8225z2_rf_set_chan(padapter, padapter->_sys_mib.cur_channel);	

#endif

//	rtl8225z2_rf_set_chan(padapter, padapter->_sys_mib.cur_channel);

	//send issue_probereq frames...
	DEBUG_INFO(("\nsitesurvey\n"));
	if (_sys_mib->sitesurvey_res.passive_mode == _TRUE)
		issue_probereq(padapter);
	
	//padapter->_sys_mib.survey_timer.time_out = SURVEY_TO;
	_set_timer(&padapter->_sys_mib.survey_timer, SURVEY_TO);
	DEBUG_INFO(("\nset sitesurvey timer \n"));
	padapter->_sys_mib.mac_ctrl.opmode |=WIFI_SITE_MONITOR;
	return;
}


void issue_deauth(_adapter *padapter, unsigned char *da, int reason)
{
	unsigned char	*pframe;
	unsigned short  val;
	struct ieee80211_hdr *hdr;
    	unsigned short *fctrl;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
    
	struct txirp *irp = alloc_txirp(padapter);

	if (irp == NULL)
    	{
		printk("Can't alloc tx irp %d:%s\n", __LINE__, __FILE__);
        	return;
	}

	printk("issue_deauth\n");

	pframe = get_txpframe(irp);
  	hdr = (struct ieee80211_hdr *)pframe;
   	fctrl = &(hdr->frame_ctl);
   	*(fctrl) = 0;


	_memcpy(hdr->addr1, da, ETH_ALEN);
	_memcpy(hdr->addr2, (myid(&(padapter->eeprompriv))), ETH_ALEN);
	_memcpy(hdr->addr3, _sys_mib->cur_network.MacAddress, ETH_ALEN);
	//_memcpy(hdr->addr3, (get_bssid(&(padapter->mlmepriv))), ETH_ALEN);

	SetFrameSubType(pframe,WIFI_DEAUTH);
	
   	pframe += sizeof (struct ieee80211_hdr_3addr);
   	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);

	val = cpu_to_le16(reason);

	pframe = set_fixed_ie(pframe, _RSON_CODE_ , (unsigned char *)&val, &(irp->tx_len));
	
	wlan_tx(padapter, irp);

}

unsigned int issue_assocreq(_adapter *padapter)
{
	unsigned char	*bssid, *pframe;
	unsigned short val;
	struct ieee80211_hdr *hdr;
	unsigned short *fctrl;
	unsigned char bssrate[LAZY_NumRates];	
	unsigned int	bssrate_len;
	unsigned int	i;
#ifdef CONFIG_WMM
	unsigned char	*p;
#endif
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	unsigned char OUI1[] = {0x00, 0x50, 0xf2,0x01};
	struct txirp *irp = alloc_txirp(padapter);

#ifdef CONFIG_RTL8192U
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	PHT_CAPABILITY_ELE 		pHTCap = NULL;
#endif
	
	
	bssid = _sys_mib->cur_network.MacAddress;

	if (irp == NULL)
	{
		printk("Can't alloc tx irp %d:%s\n", __LINE__, __FILE__);
       	return 0;
	}

	pframe = get_txpframe(irp);
	hdr = (struct ieee80211_hdr *)pframe;
   	fctrl = &(hdr->frame_ctl);
   	*(fctrl) = 0;
	hdr->seq_ctl = 0;
	SetFrameSubType(hdr, WIFI_ASSOCREQ);

	_memcpy((void *)hdr->addr1, bssid, ETH_ALEN);
	_memcpy((void *)hdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_memcpy((void *)hdr->addr3, bssid, ETH_ALEN);

	hdr->seq_ctl  = 0;
	pframe += sizeof (struct ieee80211_hdr_3addr);
   	irp->tx_len = sizeof (struct ieee80211_hdr_3addr);

	_memcpy(pframe, get_capability_from_ie((_sys_mib->cur_network.IEs)), 2);
	pframe += 2;
	
	val = cpu_to_le16(0xa);
	_memcpy(pframe ,(unsigned char *)&val, 2);
	pframe += 2;

	irp->tx_len += 4;

	//printk("SsidLength:%d\n", le32_to_cpu(_sys_mib.cur_network.Ssid.SsidLength));

	pframe = set_ie(pframe, _SSID_IE_,  le32_to_cpu(_sys_mib->cur_network.Ssid.SsidLength) ,
	 _sys_mib->cur_network.Ssid.Ssid, &(irp->tx_len));	
	
	get_bssrate_set(padapter, _SUPPORTEDRATES_IE_, bssrate, &bssrate_len);

	//printk("bssrate_len for supportedie:%d\n", bssrate_len);
	
	pframe = set_ie(pframe, _SUPPORTEDRATES_IE_ , bssrate_len , bssrate, &(irp->tx_len));

	get_bssrate_set(padapter, _EXT_SUPPORTEDRATES_IE_, bssrate, &bssrate_len);
	
	if (bssrate_len)
		pframe = set_ie(pframe, _EXT_SUPPORTEDRATES_IE_ , bssrate_len , bssrate, &(irp->tx_len));
	
	//printk("bssrate_len for ext supportedie:%d\n", bssrate_len);

	//for WPA/WPA2 to set the WPA/RSN IE
	if (padapter->_sys_mib.mac_ctrl.security == 2)//!!FIXME!!
	{
		for (i = sizeof(NDIS_802_11_FIXED_IEs); i < le32_to_cpu(_sys_mib->cur_network.IELength);)
		{
			switch (((PNDIS_802_11_VARIABLE_IEs)(_sys_mib->cur_network.IEs + i))->ElementID)
			{
				case 0x30:	//RSN IE
					pframe = set_ie(pframe, 0x30, (((PNDIS_802_11_VARIABLE_IEs)(_sys_mib->cur_network.IEs + i))->Length) , (_sys_mib->cur_network.IEs + i + 2), &(irp->tx_len));
					goto ctnd;
					
				case 0xDD:	//WPA IE
					if ((!memcmp((_sys_mib->cur_network.IEs + i + 2), OUI1, 4)))
					{
						pframe = set_ie(pframe, 0xDD, (((PNDIS_802_11_VARIABLE_IEs)(_sys_mib->cur_network.IEs + i))->Length) , (_sys_mib->cur_network.IEs + i + 2), &(irp->tx_len));
						goto ctnd;
					}
					else
					{
						break;
					}
					
				default:
					break;	
			}
			i += ((((PNDIS_802_11_VARIABLE_IEs)(_sys_mib->cur_network.IEs + i))->Length) + 2);
		}
	}
	
	DEBUG_ERR(("\n issue_assocreq:padapter->_sys_mib.mac_ctrl.security=%d\n",padapter->_sys_mib.mac_ctrl.security));
ctnd:
	
#ifdef CONFIG_WMM	

	if (1)
	{
		for (i = sizeof(NDIS_802_11_FIXED_IEs); i < le32_to_cpu(_sys_mib->cur_network.IELength);)
		{
			switch (((PNDIS_802_11_VARIABLE_IEs)(_sys_mib->cur_network.IEs + i))->ElementID)
			{
				case _RSN_IE_1_:		
					p = _sys_mib->cur_network.IEs + i;
					if ((!memcmp(p + 2, WMM_IE, 6)) || (!memcmp(p + 2, WMM_PARA_IE, 6)))
					{
						DEBUG_ERR(("get WMM IE and set it\n"));
						pframe = set_ie(pframe, _RSN_IE_1_, _WMM_IE_Length_, WMM_IE, &(irp->tx_len));
						goto cntd_ext_support_rate;
					}
					else
					{
						break;
					}
				
				default:
					break;
			}
		
			i += ((((PNDIS_802_11_VARIABLE_IEs)(_sys_mib->cur_network.IEs + i))->Length) + 2);
		}
	}
	
cntd_ext_support_rate:
	
#endif	


#ifdef CONFIG_RTL8192U
	// Include High Throuput capability && Realtek proprietary
	// TODO: support HT
	// ( AP support HT && we support HT && support WMM )
	if(_sys_mib->cur_network.BssHT.bdSupportHT && pHTInfo->bEnableHT )
	{
		printk("issue association request : support HT!!!!\n");
		pHTInfo->bCurrentHTSupport = _TRUE;
		pHTCap = (PHT_CAPABILITY_ELE )_malloc(sizeof(HT_CAPABILITY_ELE));

		if(pHTCap == NULL)
		{
			DEBUG_ERR(("alloc HTCap memory failed !"));
		}
		else
		{
		
			ActSetWirelessMode819xUsb(padapter, WIRELESS_11N_2G4);
			
			memset(pHTCap, 0 , sizeof(HT_CAPABILITY_ELE));			
			HTConstructCapabilityElement(padapter, pHTCap);

			pframe = set_ie(pframe,_HTCapability_IE,sizeof(HT_CAPABILITY_ELE) , (u8 *)pHTCap, &(irp->tx_len));		
		}
	}
#endif	


	wlan_tx(padapter, irp);

	return 1;
	
}


void start_clnt_assoc(_adapter *padapter)
{
	//unsigned long flags;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	// now auth has succedded...let's perform assoc
	//local_irq_save(flags);

	_sys_mib->mac_ctrl.opmode &= (~ (WIFI_AUTH_NULL | WIFI_AUTH_STATE1 | WIFI_ASOC_STATE));
	_sys_mib->mac_ctrl.opmode |= (WIFI_AUTH_SUCCESS);
	_sys_mib->join_res = STATE_Sta_Auth_Success;
	_sys_mib->reauth_count = 0;
	_sys_mib->reassoc_count = 0;

	//printk("start sending assoc req\n");

	if (issue_assocreq(padapter) == 0) {
		_set_timer(&(_sys_mib->reassoc_timer), REASSOC_TO);//ms

		//local_irq_restore(flags);
	}
	else {
		//local_irq_restore(flags);
	}
}


void mgtctrl_dispatchers(union recv_frame *precv_frame)
{
	unsigned int flags;
	struct rxirp *rx_irp;
	_adapter *padapter=precv_frame->u.hdr.adapter;
	struct mib_info* _sys_mib = &(padapter->_sys_mib);
	_list *head = &(_sys_mib->free_rxirp_mgt);

	_enter_critical(&(_sys_mib->free_rxirp_mgt_lock), &flags);
	if( is_list_empty(head) == _TRUE){
		DEBUG_ERR(("\n !!! mgt frame list is empty\n !!!"));
		_exit_critical(&(_sys_mib->free_rxirp_mgt_lock), &flags);
		goto ret;
	}

	rx_irp = LIST_CONTAINOR(head->next, struct rxirp, list);
	list_delete(head->next);
	
	_exit_critical(&(_sys_mib->free_rxirp_mgt_lock), &flags);
	
	_memcpy((void*)rx_irp->rxbuf_phyaddr, precv_frame->u.hdr.rx_data, precv_frame->u.hdr.len);
	rx_irp->rx_len = precv_frame->u.hdr.len;

	mgt_handlers(rx_irp);

	free_rxirp(rx_irp);
	
ret:
	return;
}


#ifdef ENABLE_MGNT_THREAD
struct rxirp *dequeue_rxirp(_queue *pqueue)
{
	_irqL irqL;
	struct rxirp *prxirp;
	_list *phead, *plist;
	
	_enter_critical(&pqueue->lock, &irqL);
	if(_queue_empty(pqueue) == _TRUE)
	{
		prxirp = NULL;
	}
	else
	{
		phead = get_list_head(pqueue);

		plist = get_next(phead);

		prxirp = LIST_CONTAINOR(plist, struct rxirp, list);

		list_delete(&prxirp->list);
	}
	_exit_critical(&pqueue->lock, &irqL);

	return prxirp;
}


extern thread_return mgnt_thread(thread_context context)
{	
	_adapter * padapter = (_adapter *)context;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct rxirp *prxirp;
_func_enter_;
	
	daemonize_thread(padapter);


	while(1)
	{
		if ((_down_sema(&(pmlmepriv->mgnt_sema))) == _FAIL)
			break;

		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{
			DEBUG_ERR(("mgnt_thread => bDriverStopped or bSurpriseRemoved \n"));
			break;
		}
		
		prxirp = dequeue_rxirp(&pmlmepriv->mgnt_pending_queue);
		if(prxirp == NULL)    //no recv frame need to process
		{
			DEBUG_ERR(("mgnt_thread: allocate prxirp from mgnt_pending_queue fail\n"));
			continue;
		}

		mgt_handlers(prxirp);

		free_rxirp(prxirp);
	}

	exit_thread(NULL, 0);

_func_exit_;

}
#endif /*ENABLE_MGNT_THREAD*/


#ifdef CONFIG_POWERSAVING
//static inline void issue_frame(unsigned char *da, unsigned char frame_type, unsigned char frame_subtype, unsigned char txirp_type, unsigned char pwr)
static void issue_frame(_adapter *padapter, unsigned char *da, unsigned char frame_type, unsigned char frame_subtype, unsigned char txirp_type, unsigned char pwr)
{
	unsigned char	*pframe;
	struct ieee80211_hdr *hdr;
	unsigned short *fctrl;
	struct txirp *irp;
	unsigned int crc32;
	
	if ((irp = alloc_txirp(txirp_type)) == NULL) {
		printk("!!!! Can't alloc tx irp %d:%s\n", __LINE__, __FILE__);
		return;
	}

	pframe = get_txpframe(irp);
  	hdr = (struct ieee80211_hdr *)pframe;
   	fctrl = &(hdr->frame_ctl);
   	*(fctrl) = 0;

	SetFrameSubType(fctrl, frame_subtype);

	if(pwr)
		SetPwrMgt(fctrl);

	if(frame_type == WIFI_CTRL_TYPE) {
		if(frame_subtype == WIFI_PSPOLL) {
			hdr->duration_id = cpu_to_le16(padapter->_sys_mib.aid);
			//dprintk("3 aid:%d\n", sys_mib.aid);
			_memcpy(hdr->addr1, (get_bssid(&(padapter->mlmepriv))), ETH_ALEN); 
			_memcpy(hdr->addr2, (myid(&(padapter->eeprompriv))), ETH_ALEN);
			/*	Perry:
				Because hardware can't calculate right checksum for short packet like PS-POLL,
				checksum must be computed by software.
				Also, packet length in TXCMD must be change (in wlan_tx function)
			*/
   			irp->tx_len = sizeof (struct ieee80211_hdr_3addr);
		} else {
			/* TODO: Only PS-Poll will call this function */
		}
	 } else {
 		_memcpy((void *)hdr->addr1, da, ETH_ALEN);
		_memcpy(hdr->addr2, (myid(&(padapter->eeprompriv))), ETH_ALEN); 
		_memcpy(hdr->addr3, (get_bssid(&(padapter->mlmepriv))), ETH_ALEN);
   		irp->tx_len = sizeof (struct ieee80211_hdr_3addr);
	 }
	wlan_tx(padapter, irp);
}


void issue_PS_POLL(_adapter *padapter, unsigned char pwr) 
{
	dprintk("pwr = %d\n", pwr);
	return issue_frame(padapter, get_bssid(&(padapter->mlmepriv)), WIFI_CTRL_TYPE, WIFI_PSPOLL, TXMGTIRPS_TAG, pwr);
	//issue_frame(get_bssid(&(padapter->mlmepriv)), WIFI_CTRL_TYPE, WIFI_PSPOLL, TXMGTIRPS_TAG, pwr);
	//dprintk("aid:%d\n", sys_mib.aid);
}


void issue_NULL_DATA(_adapter *padapter, unsigned char pwr)
{
	/* Use TXMGTIRPS_TAG instead of TXDATAIRPS_SMALL_TAG because fw use this to decide which queue it want */
	if(ps_mode != PS_MODE_IBSS)
		return issue_frame(padapter, get_bssid(&(padapter->mlmepriv)), WIFI_DATA_TYPE, WIFI_DATA_NULL, TXMGTIRPS_TAG, pwr);
		//return issue_frame(get_bssid(&(padapter->mlmepriv)), WIFI_DATA_TYPE, WIFI_DATA_NULL, TXDATAIRPS_SMALL_TAG, pwr);
	else
		return issue_frame(padapter, broadcast_sta, WIFI_DATA_TYPE, WIFI_DATA_NULL, TXMGTIRPS_TAG, pwr);
		//return issue_frame(broadcast_sta, WIFI_DATA_TYPE, WIFI_DATA_NULL, TXDATAIRPS_SMALL_TAG, pwr);
}


void issue_ATIM(_adapter *padapter, unsigned char *da)
{
	return issue_frame(padapter, da, WIFI_MGT_TYPE, WIFI_ATIM, TXMGTIRPS_TAG, 1 /* don't care?? */);
}

void issue_QNULL(_adapter *padapter, unsigned char pwr)
{
	/* Use TXMGTIRPS_TAG instead of TXDATAIRPS_SMALL_TAG because fw use this to decide which queue it want */
	//return issue_frame(get_bssid(&(padapter->mlmepriv)), WIFI_DATA_TYPE, WIFI_QOS_NULL, TXDATAIRPS_SMALL_TAG, pwr);
	return issue_frame(padapter, get_bssid(&(padapter->mlmepriv)), WIFI_DATA_TYPE, WIFI_QOS_NULL, TXMGTIRPS_TAG, pwr);
}
#endif /*CONFIG_POWERSAVING*/


static unsigned int OnAuth(struct rxirp *irp)
{
	unsigned int	privacy, seq, len, status, algthm, offset, go2asoc=0;
	//unsigned long	flags;
	unsigned char	*p;
	struct sta_data *pstadata;
	unsigned char *pframe = get_rxpframe(irp);
	_adapter *padapter = irp->adapter;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	
	if (!(_sys_mib->mac_ctrl.opmode & WIFI_STATION_STATE))
		return SUCCESS;

	if (memcmp(myid(&(padapter->eeprompriv)), get_da(pframe), ETH_ALEN))
		return SUCCESS;

	if (_sys_mib->mac_ctrl.opmode & WIFI_SITE_MONITOR)
		return SUCCESS;

	privacy = padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm;

	if (GetPrivacy(pframe))
		offset = 4;
	else
		offset = 0;

	algthm = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + offset));
	seq = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + offset + 2));
	status = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + offset + 4));

	if (status != 0)
	{
		printk("clnt auth fail, status: %d\n", status);
		goto authclnt_err_end;
	}

	if (seq == 2)
	{
		pstadata = search_assoc_sta(get_sa(pframe),padapter);
		if ((privacy == 1) || // legendary shared system
			((privacy == 2) && (_sys_mib->authModeToggle) && // auto and use shared-key currently
			 (padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm == _WEP_40_PRIVACY_ ||
			  padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm == _WEP_104_PRIVACY_)))
		{
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _AUTH_IE_OFFSET_, _CHLGETXT_IE_, (int *)&len,
				irp->rx_len - WLAN_HDR_A3_LEN - _AUTH_IE_OFFSET_);

			if (p == NULL) {
				DEBUG_ERR(("no challenge text?\n"));
				goto authclnt_fail;
			}

			_memcpy((void *)(pstadata->chg_txt), (void *)(p+2), len);
			//local_irq_save(flags);
			_sys_mib->auth_seq = 3;
			_sys_mib->mac_ctrl.opmode &= (~ WIFI_AUTH_NULL);
			_sys_mib->mac_ctrl.opmode |= (WIFI_AUTH_STATE1);
			//local_irq_restore(flags);
			issue_auth(padapter, NULL, 0);
			return SUCCESS;
		}
		else {// open system
			if( _sys_mib->mac_ctrl.opmode&WIFI_AUTH_SUCCESS)
				go2asoc = 0;
			else{	
				_sys_mib->mac_ctrl.opmode |= (WIFI_AUTH_SUCCESS);
				DEBUG_ERR(("\nOnAuth already received Auth resp\n"));
				go2asoc = 1;
			}
		}
	}
	else if (seq == 4)
	{
		if (privacy)
			go2asoc = 1;
		else
		{
			// this is illegal
			printk("no privacy but auth seq=4?\n");
			goto authclnt_fail;
		}
	}
	else
	{
		// this is also illegal
		printk("clnt auth failed due to illegal seq=%x\n", seq);
		goto authclnt_fail;
	}

	if (go2asoc)
	{
		printk("auth successful! going to assoc\n");
		start_clnt_assoc(padapter);
		return SUCCESS;
	}

authclnt_fail:

	if ((++_sys_mib->reauth_count) < REAUTH_LIMIT)
		return FAIL;

authclnt_err_end:

	if ((padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm == 2) &&
		((padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm == _WEP_40_PRIVACY_) ||
		 (padapter->_sys_mib.dot1180211authentry.dot11AuthAlgrthm == _WEP_104_PRIVACY_)) &&
		(_sys_mib->dot1180211authentry.authModeRetry == 0)) 
	{
		// auto-auth mode, retry another auth method
		_sys_mib->dot1180211authentry.authModeRetry++;

		start_clnt_auth(padapter);
		return SUCCESS;
	}
	else {
		_sys_mib->join_res = STATE_Sta_No_Bss;
		_sys_mib->join_req_ongoing = 0;

		return FAIL;
	}

	return SUCCESS;
}


static unsigned int OnAssocReq(struct rxirp *irp)
{
	return SUCCESS;
}


static unsigned int OnAssocRsp(struct rxirp *irp)
{
	//unsigned long flags;
	u8				bool;
	unsigned char	*pframe, *p;
	unsigned char	supportRate[32];
	int				supportRateNum;
	unsigned short	val;
	int				len,res;

	_adapter *				padapter		= irp->adapter;
	struct registry_priv *	pregistrypriv	= &(padapter->registrypriv);
	struct mib_info *		_sys_mib		= &(padapter->_sys_mib);

#ifdef CONFIG_RTL8192U
	PNDIS_WLAN_BSSID_EX	pcur_network = &_sys_mib->cur_network;
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	unsigned char *assoc_bssid = _sys_mib->cur_network.MacAddress;
	u8 slot_time_val = 0 ;
#endif

#ifdef LINUX_TASKLET
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	if(check_fwstate(pmlmepriv, _FW_LINKED)==_TRUE)
	{
		DEBUG_INFO(("OnBaasocRsp : return \n"));
		return SUCCESS;
	}
#endif

	DEBUG_INFO(("OnAssocRsp\n"));

	pframe = get_rxpframe(irp);

	if (!(_sys_mib->mac_ctrl.opmode & WIFI_STATION_STATE))
	{
		DEBUG_INFO(("OnAssocRsp: sys_mib->mac_ctrl.opmode != WIFI_STATION_STATE\n"));
		return SUCCESS;
	}

#ifdef CHECK_ADDR
	struct eeprom_priv *peepriv = &(padapter->eeprompriv);
	if (memcmp(myid(peepriv), get_da(pframe) , ETH_ALEN))
		return SUCCESS;
#endif
		
	if (_sys_mib->mac_ctrl.opmode & WIFI_SITE_MONITOR)
	{
		DEBUG_INFO(("OnAssocRsp: sys_mib->mac_ctrl.opmode = WIFI_SITE_MONITOR\n"));
		return SUCCESS;
	}

	if (_sys_mib->mac_ctrl.opmode & WIFI_ASOC_STATE)
	{
		DEBUG_INFO(("OnAssocRsp: sys_mib->mac_ctrl.opmode = WIFI_ASOC_STATE\n"));
		return SUCCESS;
	}

	// checking status
	_memcpy((unsigned char *)&val, (unsigned char *)((unsigned int)pframe + WLAN_HDR_A3_LEN + 2), 2);
	val = cpu_to_le16(val);

	if (val) {
		DEBUG_INFO(("assoc reject, status: %d\n", val));
		res = -1;
		goto assoc_rejected;
	}

	_memcpy(&val, (pframe + WLAN_HDR_A3_LEN + 4), 2);
	val = le16_to_cpu(val) & 0x3fff;	//only 14 bits are valid...
	
	_sys_mib->aid = val;

	// get rates
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
		irp->rx_len - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);

	if (p == NULL) {
		res = -1;
		goto assoc_rejected;
	}
	
	_memcpy(supportRate, p+2, len);
	
	supportRateNum = len;

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
		irp->rx_len - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
	
	if (p !=  NULL) {
		_memcpy(supportRate+supportRateNum, p+2, len);
		supportRateNum += len;
	}

	_sys_mib->network_type = judge_network_type(supportRate,supportRateNum,_sys_mib->cur_channel);
	
	if (_sys_mib->network_type == WIRELESS_INVALID) {
		printk("Invalid Wireless Network Type associated!\n");
		res = -1;
		goto assoc_rejected;
	}

	update_supported_rate(padapter, get_sa(pframe), supportRate, supportRateNum);

#ifdef CONFIG_RTL8192U
	if(pcur_network->BssHT.bdSupportHT)
	{
		if(_sys_mib->network_type &( WIRELESS_11B ||WIRELESS_11G) )
			_sys_mib->network_type = WIRELESS_11N_2G4;

		else if(_sys_mib->network_type == WIRELESS_11A )
			_sys_mib->network_type = WIRELESS_11N_5G;
	
	}
	DEBUG_ERR(("OnAssocRsp : current wireless mode : %d\n ",_sys_mib->network_type));
	
	if(pHTInfo->bCurrentHTSupport)
	{

		HTOnAssocRsp(padapter, pframe,irp->rx_len);
	}
	else 
	{
		memset(pHTInfo->dot11HTOperationalRateSet, 0 , 16);

		HTSetConnectBwMode(
						padapter, 
						HT_CHANNEL_WIDTH_20, 
						HT_EXTCHNL_OFFSET_NO_EXT);
	}

#endif


	// other capabilities
	_memcpy(&val, (pframe + WLAN_HDR_A3_LEN), 2);
	//sys_mib.cur_network.capability = val; //update the capability
	val = le16_to_cpu(val);

#if 0
	//Below is for to-do list...
	//checking for QBSSID
	if (val & BIT(9)) {

		//checking for parameters set...
		p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _EDCASETS_IE_, &len,
		irp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);

		if ((p != NULL) && (len == 18))  {
			//got an valid EDCA Parameter Sets...
			//_memcpy(sys_mib.cur_network.edca_parmsets, (p+2), 18);	
		}

		//checking for QBSS Load...

		p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _QBSSLOAD_IE_, &len,
		irp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);

		if ((p != NULL) && (len == 5))  {
			//got an valid EDCA Parameter Sets...
			_memcpy(sys_mib.cur_network.qbssload, (p+2), 5);	
		}

	}

#endif		

#ifdef CONFIG_RTL8192U
#ifdef CONFIG_WMM

	if (1)
	{
		for (p = pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_; ;p += (len + 2))
		{
			p = get_ie(p, _RSN_IE_1_, &len,	irp->rx_len - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
			
		if (p != NULL) 
		{
			if (!memcmp(p+2, WMM_PARA_IE, 6)) 
			{
				WMM_para_cnt = ((*(p + 8)) & 0x0f);
				_memcpy(&(pcur_network->BssQos.QosInfoField),p+8, sizeof(QOS_INFO_FIELD));

				pcur_network->BssQos.bdQoSMode |= QOS_WMM;

				p += 10;  // start of EDCA parameters
				_memcpy(&(pcur_network->BssQos.AcParameter),p, sizeof(AC_PARAM)*4);
			}
		}
			
			if ((p == NULL) || (len == 0))
		        {
				break;
		        }
		}
	} //end for qos

#endif
#endif


	if (val & BIT(5)) {
#ifdef CONFIG_RTL8187
		// set preamble according to AP
		write16(padapter,BRSR, (read16(padapter,BRSR) | BIT(15)));
#endif
		//pstat->useShortPreamble = 1;
		_sys_mib->rf_ctrl.shortpreamble = 1;
	}
	else {
#ifdef CONFIG_RTL8187
		// set preamble according to AP
		write16(padapter,BRSR, (read16(padapter,BRSR) | BIT(15)));
#endif
		//pstat->useShortPreamble = 0;
		_sys_mib->rf_ctrl.shortpreamble = 0;
	}

	if (_sys_mib->network_type == WIRELESS_11G ||_sys_mib->network_type == WIRELESS_11N_2G4)
	{
		if (val & BIT(10)) {
			_sys_mib->dot11ErpInfo.shortSlot = 1;
#ifdef CONFIG_RTL8187
			set_slot_time(_sys_mib->dot11ErpInfo.shortSlot);
	
#elif defined(CONFIG_RTL8192U)

	slot_time_val = SHORT_SLOT_TIME;
	Hal_SetHwReg819xUsb(padapter, HW_VAR_SLOT_TIME, &slot_time_val);
	
#endif

	}
		
	else {
			_sys_mib->dot11ErpInfo.shortSlot = 0;

#ifdef CONFIG_RTL8187

			set_slot_time(_sys_mib->dot11ErpInfo.shortSlot);

#elif defined(CONFIG_RTL8192U)

			slot_time_val = NON_SHORT_SLOT_TIME;
			Hal_SetHwReg819xUsb(padapter, HW_VAR_SLOT_TIME, &slot_time_val);
	
#endif

	}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
			irp->rx_len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

		if (_sys_mib->vcs_mode == 2)
		{
			if (p && (*(p+2) & BIT(1)))	// use Protection
			{
				_sys_mib->dot11ErpInfo.protection = pregistrypriv->vcs_type;
				//printk("assocrsp: RTS\n");
			}
			else
			{
				_sys_mib->dot11ErpInfo.protection = 0;
			}
		}

		if (p && (*(p+2) & BIT(2)))	// use long preamble
			_sys_mib->dot11ErpInfo.longPreambleStaNum = 1;
		else
			_sys_mib->dot11ErpInfo.longPreambleStaNum = 0;
	}

	//local_irq_save(flags);

	while(1) {
		_cancel_timer(&_sys_mib->reauth_timer, &bool);
		if(bool == _TRUE)
			break;
	}

	while(1) {
		_cancel_timer(&_sys_mib->reassoc_timer, &bool);
		if(bool == _TRUE)
			break;
	}

	_sys_mib->mac_ctrl.opmode |= WIFI_ASOC_STATE;
	//rtl_outw(WPA_CONFIG, rtl_inw(WPA_CONFIG) & 0xfffe);
#ifdef CONFIG_RTL8187

	join_bss(padapter);

#elif defined(CONFIG_RTL8192U)

	switch(_sys_mib->network_type)
	{
			case WIRELESS_11B:
				_sys_mib->rate = 110;
				printk(KERN_INFO"Using 11M rates\n");
				break;
			case WIRELESS_11G:
			case WIRELESS_11BG:
			case WIRELESS_11N_2G4:
				_sys_mib->rate = 540;
				printk(KERN_INFO"Using 54M rates\n");
				break;
	}

	
	Hal_SetHwReg819xUsb(padapter, HW_VAR_BSSID, assoc_bssid);
	Hal_SetHwReg819xUsb(padapter, HW_VAR_MEDIA_STATUS, (u8 *)&val);

	ActSetWirelessMode819xUsb(padapter, _sys_mib->network_type);

	//write Beacon Interval 
	write16(padapter, BCNITV, *(u16 *)get_beacon_interval_from_ie(pcur_network->IEs));

	//  set CBSSID bit when link with any AP
//	regRCR = PlatformEFIORead4Byte(padapter, RCR);
//	regRCR |= (RCR_CBSSID);
//	PlatformEFIOWrite4Byte(padapter, RCR, regRCR);

	// SetRate Adaptive Table
	SetHalRATRTable819xUsb(padapter, _sys_mib->mac_ctrl.datarate,
									pHTInfo->dot11HTOperationalRateSet);
#endif
	
	_set_timer(&(_sys_mib->disconnect_timer), DISCONNECT_TO);
	
	_sys_mib->join_res = STATE_Sta_Bss;
	_sys_mib->join_req_ongoing = 0;	
	//local_irq_restore(flags);

	res = _sys_mib->aid;

	goto assoc_accepted;
	
assoc_rejected:

	_sys_mib->join_res = STATE_Sta_No_Bss;

	SelectChannel(padapter, _sys_mib->cur_channel);

	while(1) {
		_cancel_timer(&_sys_mib->reassoc_timer, &bool);
		if(bool == _TRUE)
			break;
	}
	
assoc_accepted:
	
	_sys_mib->join_req_ongoing = 0;

#ifdef UNIVERSAL_REPEATER
	disable_vxd_ap(GET_VXD_PRIV(priv));
#endif

	report_join_res(padapter, res);

	return SUCCESS;

}

static unsigned int OnProbeReq(struct rxirp *irp)
{
#if 0
	unsigned char	*pframe, *p;
	unsigned int	len;
	_adapter *padapter = irp->adapter;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	NDIS_WLAN_BSSID_EX *cur = &(_sys_mib->cur_network);
	
	pframe = get_rxpframe(irp);
	
	if (!((OPMODE & WIFI_ASOC_STATE) || (OPMODE & WIFI_AP_STATE)))
		return SUCCESS;

	DEBUG_INFO(("OnProbeReq \n"));
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_, _SSID_IE_, (int *)&len,
			irp->rx_len - WLAN_HDR_A3_LEN - _PROBEREQ_IE_OFFSET_);
	
	if (p != NULL)
	{
		if ((len != 0) && (memcmp((void *)(p+2), (void *)cur->Ssid.Ssid, le32_to_cpu(cur->Ssid.SsidLength))))
		{
			return SUCCESS;
		}
		
		issue_probersp(padapter, get_sa(pframe), _sys_mib->mac_ctrl.security);
	}
#endif
	return SUCCESS;

}


typedef struct _DOT11_WPA_MULTICAST_CIPHER{
        unsigned char	EventId;
        unsigned char	IsMoreEvent;
        unsigned char	MulticastCipher;
}DOT11_WPA_MULTICAST_CIPHER;

#ifdef CONFIG_RTL8192U
VOID
HalSetBrateCfg(
	 PADAPTER			Adapter,
	 u8		*mBratesOS,
        u16 *pBrateCfg
)
{
	
	
	u8			i, brate ;


	for(i=0; i<NumRates ;i++)
	{
		
		brate = ratetbl_val_2wifirate(mBratesOS[i]) ;	

		if ((brate != 0xff) && (brate != 0xfe))
		{
			brate &= 0x7f;
			
			switch(brate)
			{
			case 0x02:	*pBrateCfg |= RRSR_1M;	break;
			case 0x04:	*pBrateCfg |= RRSR_2M;	break;
			case 0x0b:	*pBrateCfg |= RRSR_5_5M;	break;
			case 0x16:	*pBrateCfg |= RRSR_11M;	break;
			case 0x0c:	*pBrateCfg |= RRSR_6M;	break;
			case 0x12:	*pBrateCfg |= RRSR_9M;	break;
			case 0x18:	*pBrateCfg |= RRSR_12M;	break;
			case 0x24:	*pBrateCfg |= RRSR_18M;	break;
			case 0x30:	*pBrateCfg |= RRSR_24M;	break;
			case 0x48:	*pBrateCfg |= RRSR_36M;	break;
			case 0x60:	*pBrateCfg |= RRSR_48M;	break;
			case 0x6c:	*pBrateCfg |= RRSR_54M;	break;
			}
		}

	}
}




/*
*	Description:
*		Set HalRATR for Rate Adaptive
*	/param 	Adapter	Pionter to Adapter entity
*	/param	index	RATR index
*	/param	pRateSet 	rate bitmap
*	
*	/return	void
*
*	NOTE: if we are in 8190, we write the rate bitmap into HW
*	11/15/2007	MHC		Merge SVN NDIS V0244 8190PCI function.
*/
VOID
SetHalRATRTable819xUsb(
	PADAPTER		Adapter,
	u8	*posLegacyRate,
	u8			*pMcsRate
)
{
	u32		ratr_value = 0;
	u32 		ratr_index = 0;
	u32		RateSet = 0;
	struct eeprom_priv*	peeprompriv = &Adapter->eeprompriv;
	PRT_HIGH_THROUGHPUT pHTInfo = &Adapter->HTInfo;

	

	HalSetBrateCfg(Adapter, posLegacyRate, (pu2Byte)&RateSet);
	RateSet |= (*((pu2Byte)(pMcsRate))) << 12;


	for(ratr_index = 0; ratr_index < 7; ratr_index++)
	{
		// Original rate set
		ratr_value = RateSet;

		//
		// Index0: MCS rate only.
		// Index1: Full rate set without 2ss rate, for static mimo ps
		// Index2: Filter out CCK rates, for STA which is not support CCK rates
		// Index3: Combine the condition in Index1 and Index2
		// Index4: Only legacy rates, for legacy STA
		// Index5: Only CCK rate, for 11b STA
		// Index6: Filter out all legacy rate.
		//

		if(ratr_index == RATR_INDEX_N_RATE)
		{
			// MCS rate only => for 11N mode.
			
			if(peeprompriv->RF_Type == RF_1T2R)	// 1T2R, Spatial Stream 2 should be disabled
			{
				ratr_value &= 0x000FF007;
				//DbgPrint("SetHalRATRTable8190Pci() = 0x0F81F007 ==> 0x%x", ratr_value);
			}
			else
			{
				ratr_value &= 0x0F81F007;
			}
		}
		else if(ratr_index == RATR_INDEX_N_NO_2SS)
		{
			// MCS rate without 2ss rates => for 11N Static MIMO PS
			ratr_value &= 0x0007F007;
		}
		else if(ratr_index == RATR_INDEX_G)
		{
			// Filter out all MCS rate, legacy rate only!! => for b/g mode
			//ratr_value &= 0x00000FF0;			
			ratr_value &= 0x00000FF7;			
		}
		else if(ratr_index == RATR_INDEX_B)
		{
			// Legacy CCK rate only. => for b mode.
			ratr_value &= 0x0000000F;
		}
		else if(ratr_index == RATR_INDEX_A)
		{
			// Legacy Ofdm rate only!! => for a mode
			ratr_value &= 0x00000FF0;
		}
		else
		{
			// Filter out all legacy reate
			// This is just for test!!
			ratr_value &=0x0F0F0000;
			ratr_value |=0x80000000;
		}

		//--------------------------------------------------
		//Set short GI bit according to BW & support Short GI
		//
		//--------------------------------------------------
		ratr_value &= 0x0FFFFFFF;
		if(pHTInfo->bCurTxBW40MHz && pHTInfo->bCurShortGI40MHz){
			ratr_value |= 0x80000000;
		}else if(!pHTInfo->bCurTxBW40MHz && pHTInfo->bCurShortGI20MHz){
			ratr_value |= 0x80000000;
		}	

		//3 Write into RATR
		PlatformEFIOWrite4Byte(Adapter, RATR0+ratr_index*4, ratr_value);
	}
	
	//3 UFWP
	PlatformEFIOWrite1Byte(Adapter, UFWP, 1);

}





VOID
PHY_SetRF8256Bandwidth(
	_adapter *padapter, 
		HT_CHANNEL_WIDTH		Bandwidth)	//20M or 40M
{

	//RF90_RADIO_PATH_E		eRFPath;
	u1Byte	eRFPath;
	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;
	
// TODO: remove these definition to Hal8190PciPhyReg.h
#define	rRTL8256RxMixerPole		0xb
#define 	bZebraRxMixerPole		0x6
#define 	rRTL8256TxBBOPBias        0x9
#define 	bRTL8256TxBBOPBias       0x400
#define 	rRTL8256TxBBBW             19
#define 	bRTL8256TxBBBW            	0x18
// TODO: //Remove it!



	//for(eRFPath = RF90_PATH_A; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
	for(eRFPath = 0; eRFPath <RF90_PATH_MAX; eRFPath++)
	{
		if(peeprompriv->RF_Type == RF_1T2R)
			{
				if(eRFPath == RF90_PATH_C || eRFPath == RF90_PATH_D)
					continue;
			}
			else if(peeprompriv->RF_Type == RF_2T4R)

				continue;
			
		
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:
		
				if(peeprompriv->VersionID == VERSION_819xUsb_A ||peeprompriv->VersionID == VERSION_819xUsb_B)// 8256 D-cut, E-cut, xiong: consider it later!
				{	
					PHY_SetRFReg(padapter, (RF90_RADIO_PATH_E)eRFPath, 0x0b, bMask12Bits, 0x100); //phy para:1ba
					PHY_SetRFReg(padapter, (RF90_RADIO_PATH_E)eRFPath, 0x2c, bMask12Bits, 0x3d7);
					PHY_SetRFReg(padapter, (RF90_RADIO_PATH_E)eRFPath, 0x0e, bMask12Bits, 0x021);
					
				}
				else
				{
					DEBUG_ERR(("PHY_SetRF8256Bandwidth(): unknown hardware version\n"));
				}

				break;
			case HT_CHANNEL_WIDTH_20_40:

				if(peeprompriv->VersionID == VERSION_819xUsb_A ||peeprompriv->VersionID == VERSION_819xUsb_B)// 8256 D-cut, E-cut, xiong: consider it later!
				{	
					PHY_SetRFReg(padapter, (RF90_RADIO_PATH_E)eRFPath, 0x0b, bMask12Bits, 0x300); //phy para:3ba
					PHY_SetRFReg(padapter, (RF90_RADIO_PATH_E)eRFPath, 0x2c, bMask12Bits, 0x3df);
					PHY_SetRFReg(padapter, (RF90_RADIO_PATH_E)eRFPath, 0x0e, bMask12Bits, 0x0a1);
				}
				else
				{
					DEBUG_ERR(("PHY_SetRF8256Bandwidth(): unknown hardware version\n"));
				}


				break;
			default:
				DEBUG_ERR(("PHY_SetRF8256Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth ));
				break;
				
		}
	}

}




/*-----------------------------------------------------------------------------
 * Function:    SetBWModeCallback819xUsb()
 *
 * Overview:    Timer callback function for SetSetBWMode
 *
 * Input:       	PRT_TIMER		pTimer
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Note:		(1) We do not take j mode into consideration now
 *			(2) Will two workitem of "switch channel" and "switch channel bandwidth" run
 *			     concurrently?
 *---------------------------------------------------------------------------*/
VOID
SetBWModeCallback819xUsb(
	_adapter *padapter
)
{

	struct eeprom_priv*	peeprompriv = &padapter->eeprompriv;
 	
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	u1Byte	 			regBwOpMode;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//u4Byte				NowL, NowH;
	//u8Byte				BeginTime, EndTime; 
	
	
	if(peeprompriv->RFChipID == RF_PSEUDO_11N)
	{
	//	pHalData->SetBWModeInProgress= _FALSE;
		return;
	}
		
	if(padapter->bDriverStopped)
		return;
		
	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = PlatformEFIORead4Byte(Adapter, TSFR);
	//NowH = PlatformEFIORead4Byte(Adapter, TSFR+4);
	//BeginTime = ((u8Byte)NowH << 32) + NowL;
		
	//3<1>Set MAC register
	regBwOpMode = PlatformEFIORead1Byte(padapter, BW_OPMODE);
	
	switch(pHTInfo->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
		       // 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			PlatformEFIOWrite1Byte(padapter, BW_OPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
        		// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			PlatformEFIOWrite1Byte(padapter, BW_OPMODE, regBwOpMode);
			break;

		default:
			DEBUG_ERR(("SetBWModeCallback819xUsb():\
						unknown Bandwidth: %d\n",pHTInfo->CurrentChannelBW));
			break;
	}
	
	//3 <2>Set PHY related register
	switch(pHTInfo->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			PHY_SetBBReg(padapter, rFPGA0_RFMOD, bRFMOD, 0x0);
			PHY_SetBBReg(padapter, rFPGA1_RFMOD, bRFMOD, 0x0);
			PHY_SetBBReg(padapter, rFPGA0_AnalogParameter1, bADClkPhase, 1);
			PHY_SetBBReg(padapter, rOFDM0_RxDetector1, bMaskByte0, 0x44); //suggest by jerry, by emily

			// Correct the tx power for CCK rate in 20M. Suggest by YN, 20071207
			PlatformEFIOWrite4Byte(padapter, rCCK0_TxFilter1, 0x1a1b0000);
			PlatformEFIOWrite4Byte(padapter, rCCK0_TxFilter2, 0x090e1317);
			PlatformEFIOWrite4Byte(padapter, rCCK0_DebugPort, 0x00000204);

			break;
		case HT_CHANNEL_WIDTH_20_40:
			PHY_SetBBReg(padapter, rFPGA0_RFMOD, bRFMOD, 0x1);
			PHY_SetBBReg(padapter, rFPGA1_RFMOD, bRFMOD, 0x1);
			PHY_SetBBReg(padapter, rCCK0_System, bCCKSideBand, (pHTInfo->nCur40MhzPrimeSC>>1));
			PHY_SetBBReg(padapter, rFPGA0_AnalogParameter1, bADClkPhase, 0);
			PHY_SetBBReg(padapter, rOFDM1_LSTF, 0xC00, pHTInfo->nCur40MhzPrimeSC);
			PHY_SetBBReg(padapter, rOFDM0_RxDetector1, bMaskByte0, 0x42);//suggest by jerry, by emily

			// Correct the tx power for CCK rate in 40M. Suggest by YN, 20071207
			PlatformEFIOWrite4Byte(padapter, rCCK0_TxFilter1, 0x35360000);
			PlatformEFIOWrite4Byte(padapter, rCCK0_TxFilter2, 0x121c252e);
			PlatformEFIOWrite4Byte(padapter, rCCK0_DebugPort, 0x00000409);

			break;
		default:
			DEBUG_ERR(("SetBWModeCallback819xUsb(): unknown Bandwidth: %#X\n"\
						,pHTInfo->CurrentChannelBW));
			break;
			
	}
	//Skip over setting of J-mode in BB register here. Default value is "None J mode". Emily 20070315

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = PlatformEFIORead4Byte(Adapter, TSFR);
	//NowH = PlatformEFIORead4Byte(Adapter, TSFR+4);
	//EndTime = ((u8Byte)NowH << 32) + NowL;
	//RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWModeCallback8190Pci: time of SetBWMode = %I64d us!\n", (EndTime - BeginTime)));

#if 1
	//3<3>Set RF related register
	switch(peeprompriv->RFChipID)
	{
		case RF_8225:		
			
			break;	
			
		case RF_8256:
			// Please implement this function in Hal8190PciPhy8256.c
			PHY_SetRF8256Bandwidth(padapter, pHTInfo->CurrentChannelBW);
			break;
			
		case RF_8258:
			// Please implement this function in Hal8190PciPhy8258.c
			// PHY_SetRF8258Bandwidth();
			break;

		case RF_PSEUDO_11N:
			// Do Nothing
			break;
			
		default:
			DEBUG_ERR(("Unknown RFChipID: %d\n", peeprompriv->RFChipID));
			break;
	}
#endif


	
}




VOID
SetBWMode819xUsb(
	_adapter *padapter,
		HT_CHANNEL_WIDTH	Bandwidth,	// 20M or 40M
		HT_EXTCHNL_OFFSET	Offset		// Upper, Lower, or Don't care
)
{




	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	
	


	
	pHTInfo->CurrentChannelBW = Bandwidth;

	if(Offset==HT_EXTCHNL_OFFSET_LOWER)
		pHTInfo->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
	else if(Offset==HT_EXTCHNL_OFFSET_UPPER)
		pHTInfo->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
	else
		pHTInfo->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

	if(!padapter->bDriverStopped)
	{
		//PlatformSetTimer(Adapter, &(pHalData->SetBWModeTimer), 0);
		
	
		SetBWModeCallback819xUsb(padapter);
	}

}




u2Byte
HTMcsToDataRate(
	PADAPTER		Adapter,
	u1Byte			nMcsRate
	)
{
	PRT_HIGH_THROUGHPUT	pHTInfo = &Adapter->HTInfo;
	
	u1Byte	is40MHz = (pHTInfo->bCurBW40MHz)?1:0;
	u1Byte	isShortGI = (pHTInfo->bCurBW40MHz)?
						((pHTInfo->bCurShortGI40MHz)?1:0):
						((pHTInfo->bCurShortGI20MHz)?1:0);
	return MCS_DATA_RATE[is40MHz][isShortGI][(nMcsRate&0x7f)];
}



/*
*	Description:
*		This function will get the highest speed rate in input MCS set.
*
*	/param 	Adapter			Pionter to Adapter entity
*			pMCSRateSet		Pointer to MCS rate bitmap
*			pMCSFilter		Pointer to MCS rate filter
*	
*	/return	Highest MCS rate included in pMCSRateSet and filtered by pMCSFilter.
*
*/
u1Byte
HTGetHighestMCSRate(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pMCSRateSet,
	IN	pu1Byte			pMCSFilter
	)
{
	u1Byte		i, j;
	u1Byte		bitMap;
	u1Byte		mcsRate = 0;
	u1Byte		availableMcsRate[16];

	for(i=0; i<16; i++)
		availableMcsRate[i] = pMCSRateSet[i] & pMCSFilter[i];

	for(i = 0; i < 16; i++)
	{
		if(availableMcsRate[i] != 0)
			break;
	}
	if(i == 16)
		return 0;

	for(i = 0; i < 16; i++)
	{
		if(availableMcsRate[i] != 0)
		{
			bitMap = availableMcsRate[i];
			for(j = 0; j < 8; j++)
			{
				if((bitMap%2) != 0)
				{
					if(HTMcsToDataRate(Adapter, (8*i+j)) > HTMcsToDataRate(Adapter, mcsRate))
						mcsRate = (8*i+j);
				}
				bitMap = bitMap>>1;
			}
		}
	}
	return (mcsRate|0x80);
}
	



/*
**
**1.Filter our operation rate set with AP's rate set 
**2.shall reference channel bandwidth, STBC, Antenna number
**3.generate rate adative table for firmware
**David 20060906
**
** \pHTSupportedCap: the connected STA's supported rate Capability element
*/
BOOLEAN
HTFilterMCSRate(
	_adapter *padapter,
	u8	*pSupportMCS,
	u8	*pOperateMCS
)
{
	
	UINT i=0;
	PRT_HIGH_THROUGHPUT	pHTInfo = &padapter->HTInfo;
	struct mib_info* _sys_mib = &(padapter->_sys_mib);
	// filter out operational rate set not supported by AP, the lenth of it is 16
	for(i=0;i<=15;i++){
		pOperateMCS[i] = pHTInfo->dot11HTOperationalRateSet[i]&pSupportMCS[i];
	}

	// TODO: adjust our operational rate set  according to our channel bandwidth, STBC and Antenna number

	// TODO: fill suggested rate adaptive rate index and give firmware info using Tx command packet
	// we also shall suggested the first start rate set according to our singal strength
	if( (_sys_mib->network_type != WIRELESS_11N_2G4) && (_sys_mib->network_type != WIRELESS_11N_5G))
	{
		for(i=0;i<=15;i++)
		{

		    pOperateMCS[i] = 0;
		}
	}
	else
	{
		pOperateMCS[0] &=0xFF;//RATE_ADPT_1SS_MASK;	//support MCS 0~7
		pOperateMCS[1] &=0xF0;//RATE_ADPT_2SS_MASK;
		pOperateMCS[3] &=0x01; //RATE_ADPT_MCS32_MASK;

	}
	//
	// For RTL819X, we support only MCS0~15.
	// And also, we do not know how to use MCS32 now.
	//
	for(i=2; i<=15; i++)
		pOperateMCS[i] = 0;
	
	return _TRUE;
}


//
// This function set bandwidth mode in protocol layer.
//
void
HTSetConnectBwMode(_adapter *padapter,HT_CHANNEL_WIDTH		Bandwidth,HT_EXTCHNL_OFFSET		Offset)
{
	
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	struct mib_info* _sys_mib = &(padapter->_sys_mib);
	

	// To reduce dummy operation
//	if((pHTInfo->bCurBW40MHz==_FALSE && Bandwidth==HT_CHANNEL_WIDTH_20) ||
//	   (pHTInfo->bCurBW40MHz==_TRUE && Bandwidth==HT_CHANNEL_WIDTH_20_40 && Offset==pHTInfo->CurSTAExtChnlOffset))
//		return;
#if 0
	PlatformAcquireSpinLock(Adapter, RT_BW_SPINLOCK);
	if(pHTInfo->bSwBwInProgress)
	{
		PlatformReleaseSpinLock(Adapter, RT_BW_SPINLOCK);
		return;
	}


	{
		pHTInfo->bCurBW40MHz = _FALSE;
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_NO_EXT;
	}

	pHTInfo->bSwBwInProgress = _TRUE;
#endif
	// TODO: 2007.7.13 by Emily Wait 2000ms  in order to garantee that switching
	//   bandwidth is executed after scan is finished. It is a temporal solution
	//   because software should ganrantee the last operation of switching bandwidth
	//   is executed properlly.

#ifdef CONFIG_SUPPORT_40MHZ
	if(Bandwidth == HT_CHANNEL_WIDTH_20_40)
	{
		if(Offset==HT_EXTCHNL_OFFSET_UPPER|| Offset==HT_EXTCHNL_OFFSET_LOWER)
		{
			
			printk("Set Bandwidth: 40MHz mode\n");
			pHTInfo->bCurBW40MHz = _TRUE;
			pHTInfo->CurSTAExtChnlOffset = Offset;
		
		}	
		else
		{
			printk("Set Bandwidth: 40MHz mode, no offset => 20 MHz\n");
			pHTInfo->bCurBW40MHz = _FALSE;
			pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_NO_EXT;
			
		}
	}
	else
#endif
	{
		printk("Set Bandwidth: 20MHz mode\n");
		pHTInfo->bCurBW40MHz = _FALSE;
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_NO_EXT;
	}
	pHTInfo->bSwBwInProgress = _TRUE;


	msleep_os(200);

	if(pHTInfo->bCurBW40MHz)
	{
/*		if(pHTInfo->CurSTAPrimaryChnlOffset==HAL_PRIME_CHNL_OFFSET_LOWER)
			pMgntInfo->dot11CurrentChannelNumber+=2;
		else
			pMgntInfo->dot11CurrentChannelNumber-=2;

		Adapter->HalFunc.SwChnlByTimerHandler(Adapter, pMgntInfo->dot11CurrentChannelNumber);
*/
		if(pHTInfo->CurSTAExtChnlOffset==HT_EXTCHNL_OFFSET_UPPER)

			SelectChannel(padapter, _sys_mib->cur_channel+2);
//			padapter->HalFunc.SwChnlByTimerHandler(Adapter, pMgntInfo->dot11CurrentChannelNumber+2);
		else
			SelectChannel(padapter, _sys_mib->cur_channel-2);
//			padapter->HalFunc.SwChnlByTimerHandler(Adapter, pMgntInfo->dot11CurrentChannelNumber-2);
			
		
		SetBWMode819xUsb(padapter, HT_CHANNEL_WIDTH_20_40,pHTInfo->CurSTAExtChnlOffset);
	}
	else
	{
		SelectChannel(padapter, _sys_mib->cur_channel);
		SetBWMode819xUsb(padapter, HT_CHANNEL_WIDTH_20,HT_EXTCHNL_OFFSET_NO_EXT);
	}


	#if 0
	
	PlatformReleaseSpinLock(Adapter, RT_BW_SPINLOCK);
	#endif
}







VOID
HTOnAssocRsp(
	_adapter *padapter,
	u8 * pframe,
	u32 irp_len
)
{


	PRT_HIGH_THROUGHPUT	pHTInfo = &padapter->HTInfo;
	PHT_CAPABILITY_ELE		pPeerHTCap;
	PHT_INFORMATION_ELE		pPeerHTInfo;
	u8 i = 0 ,*ptr = NULL;
	u32 ie_len = 0;
	u2Byte					nMaxAMSDUSize;
	pu1Byte					pMcsFilter;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	//static u1Byte				EWC11NHTCap[] = {0x00, 0x90, 0x4c, 0x33};		// For 11n EWC definition, 2007.07.17, by Emily
	//static u1Byte				EWC11NHTInfo[] = {0x00, 0x90, 0x4c, 0x34};	// For 11n EWC definition, 2007.07.17, by Emily
	
	if( pHTInfo->bCurrentHTSupport == _FALSE )
	{
		DEBUG_ERR(("<=== HTOnAssocRsp(): HT_DISABLE\n") );
		return;
	}
	
	memset(pHTInfo->PeerHTCapBuf,0, sizeof(pHTInfo->PeerHTCapBuf));
	memset(pHTInfo->PeerHTInfoBuf, 0, sizeof(pHTInfo->PeerHTInfoBuf));
	

	pPeerHTCap = (PHT_CAPABILITY_ELE)(pHTInfo->PeerHTCapBuf);

	pPeerHTInfo = (PHT_INFORMATION_ELE)(pHTInfo->PeerHTInfoBuf);
	
	// Ad-Hoc Master mode
	if((pframe == NULL) || (irp_len ==0))
	{

		if(_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)
		{
		
			pPeerHTCap->ChlWidth = HT_CHANNEL_WIDTH_20; // Operating in 20 MHz
			pPeerHTInfo->ExtChlOffset = HT_EXTCHNL_OFFSET_NO_EXT; // No SubChannel
			pPeerHTInfo->RecommemdedTxWidth = 0; //Operating in 20 MHz
			pPeerHTCap->ShortGI20Mhz = 1;
			pPeerHTCap->DssCCk =1;
			pPeerHTCap->MaxAMSDUSize = 0; // AMSDU size = 3839
			pPeerHTCap->MaxRxAMPDUFactor = 2;
			pPeerHTCap->MPDUDensity = 0 ;
			pPeerHTCap->MimoPwrSave = MIMO_PS_DYNAMIC;
			pPeerHTInfo->OptMode = HT_OPMODE_NO_PROTECT;

			for(i = 0 ; i < 15 ; i ++)
				pPeerHTCap->MCS[i] = 0xff;
		}
		else
		{
			DEBUG_ERR(("pframe == NULL or irp_len = 0 \n "));
			return;

		}
	}
	else
	{
		// Get HT CAP and copy into buffer

		ptr = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _HTCapability_IE, &ie_len,
			irp_len - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
	
		ie_len = ie_len> sizeof(pHTInfo->PeerHTCapBuf)?\
			sizeof(pHTInfo->PeerHTCapBuf):ie_len;	//prevent from overflow

		if(ie_len > 0)
			_memcpy(pHTInfo->PeerHTCapBuf, ptr+2, ie_len);

		// Get HT Info and copy into buffer
		ptr = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _HTInfo_IE, &ie_len,
			irp_len - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);

		ie_len = ie_len> sizeof(pHTInfo->PeerHTInfoBuf)?\
			sizeof(pHTInfo->PeerHTInfoBuf):ie_len;	//prevent from overflow
		
		if(ie_len > 0)
			_memcpy(pHTInfo->PeerHTInfoBuf, ptr+2, ie_len);
	}
	////////////////////////////////////////////////////////
	// Configurations:
	////////////////////////////////////////////////////////

	//
	// Config Supported Channel Width setting
	//
	HTSetConnectBwMode(
					padapter, 
					(HT_CHANNEL_WIDTH)(pPeerHTCap->ChlWidth), 
					(HT_EXTCHNL_OFFSET)(pPeerHTInfo->ExtChlOffset));

	if(pHTInfo->bCurBW40MHz == _TRUE)
		pHTInfo->bCurTxBW40MHz = ((pPeerHTInfo->RecommemdedTxWidth == 1)?_TRUE:_FALSE);

	//
	// Update short GI/ long GI setting
	//
	// TODO:
	pHTInfo->bCurShortGI20MHz= 
		(pPeerHTCap->ShortGI20Mhz==1)?_TRUE:_FALSE;

	pHTInfo->bCurShortGI40MHz= _FALSE;
#ifdef CONFIG_SUPPORT_40MHZ
	pHTInfo->bCurShortGI40MHz= 
		(pPeerHTCap->ShortGI40Mhz==1)?_TRUE:_FALSE;
#endif

	//
	// Config TX STBC setting
	//
	// TODO:

	//
	// Config DSSS/CCK  mode in 40MHz mode
	//
	// TODO:
	pHTInfo->bCurSuppCCK = 
		((pHTInfo->bRegSuppCCK)?((pPeerHTCap->DssCCk==1)?_TRUE:_FALSE):_FALSE);


	//
	// Config and configure A-MSDU setting
	//
	pHTInfo->bCurrent_AMSDU_Support = 1;//pHTInfo->bAMSDU_Support;

	nMaxAMSDUSize = (pPeerHTCap->MaxAMSDUSize==0)?3839:7935;

	if(pHTInfo->nAMSDU_MaxSize > nMaxAMSDUSize )
		pHTInfo->nCurrent_AMSDU_MaxSize = nMaxAMSDUSize;
	else
		pHTInfo->nCurrent_AMSDU_MaxSize = pHTInfo->nAMSDU_MaxSize;


	//
	// Config A-MPDU setting
	//
	pHTInfo->bCurrentAMPDUEnable = 1;//pHTInfo->bAMPDUEnable;

	// <1> Decide AMPDU Factor
		
	// By Emily
		// Decide AMPDU Factor according to protocol handshake
	
//	if(pHTInfo->AMPDU_Factor > pPeerHTCap->MaxRxAMPDUFactor)
			pHTInfo->CurrentAMPDUFactor = pPeerHTCap->MaxRxAMPDUFactor;
//		else
//			pHTInfo->CurrentAMPDUFactor = pHTInfo->AMPDU_Factor;

	// <2> Set AMPDU Density
//	if(pHTInfo->MPDU_Density > pPeerHTCap->MPDUDensity)
		pHTInfo->CurrentMPDUDensity = pPeerHTCap->MPDUDensity;
//	else
//		pHTInfo->CurrentMPDUDensity = pHTInfo->MPDU_Density;

	// Rx Reorder Setting
	pHTInfo->bCurRxReorderEnable = pHTInfo->bRegRxReorderEnable;

	//
	// Filter out unsupported HT rate for this AP
	// Update RATR table
	// This is only for 8190 ,8192 or later product which using firmware to handle rate adaptive mechanism.
	//

	
	HTFilterMCSRate(padapter, pPeerHTCap->MCS, pHTInfo->dot11HTOperationalRateSet);

// 
	//
	// Config MIMO Power Save setting
	//
	pHTInfo->PeerMimoPs = pPeerHTCap->MimoPwrSave;
	if(pHTInfo->PeerMimoPs == MIMO_PS_STATIC)
		pMcsFilter = MCS_FILTER_1SS;
	else
		pMcsFilter = MCS_FILTER_ALL;
	pHTInfo->HTHighestOperaRate = 0x87;
					//HTGetHighestMCSRate(	padapter, 
					//					pHTInfo->dot11HTOperationalRateSet,
					//					pMcsFilter	);
	pHTInfo->HTCurrentOperaRate = pHTInfo->HTHighestOperaRate;

	//
	// Config current operation mode.
	//
	pHTInfo->CurrentOpMode = pPeerHTInfo->OptMode;
}	

VOID 
HTInitializeBssDesc(IN PBSS_HT	 pBssHT)
{


	pBssHT->bdSupportHT = _FALSE;
	memset(pBssHT->bdHTCapBuf, 0,sizeof(pBssHT->bdHTCapBuf));
	pBssHT->bdHTCapLen = 0;
	memset(pBssHT->bdHTInfoBuf, 0,sizeof(pBssHT->bdHTInfoBuf));
	pBssHT->bdHTInfoLen = 0;



	pBssHT->bdRT2RTAggregation = _FALSE;
	pBssHT->bdRT2RTLongSlotTime = _FALSE;
}

/*
  * Get HT related information from beacon and save it in BssDesc
  *
  * (1) Parse HTCap, and HTInfo, and record whether it is 11n AP
  * (2) If peer is HT, but not WMM, call QosSetLegacyWMMParamWithHT()
  * (3) Check whether peer is Realtek AP (for Realtek proprietary aggregation mode).
  
*/

void HTGetValueFromBeaconOrProbeRsp(
	_adapter *padapter ,	
	u8 *pframe,
	NDIS_WLAN_BSSID_EX *prsp,
 u32 rxlen
)

{
	u8 *pie = NULL;
	u32 size = 0 , ie_len = 0 ;

	// TODO: Endian issue not solved ? 		
	//2Note:  
	//   Mark for IOT testing using  Linksys WRT350N, This AP does not contain WMM IE  when 
	//   it is configured at pure-N mode.
	//	if(bssDesc->BssQos.bdQoSMode & QOS_WMM)
	//	

	HTInitializeBssDesc (&prsp->BssHT);

	//2<1> Parse HTCap, and HTInfo
	// Get HT Capability IE: (1) Get IEEE Draft N IE or (2) Get EWC IE
	

	pie = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _HTCapability_IE, &ie_len,
		rxlen - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);

	if(pie && ie_len != 0)
	{
		size = ie_len > sizeof(prsp->BssHT.bdHTCapBuf)? sizeof(prsp->BssHT.bdHTCapBuf): ie_len;
		_memcpy(prsp->BssHT.bdHTCapBuf, pie+2, size);
		prsp->BssHT.bdHTCapLen = size;
	}
	

	// Get HT Information IE: (1) Get IEEE Draft N IE or (2) Get EWC IE
	pie = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _HTInfo_IE, &ie_len,
		rxlen - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);

	if(pie && ie_len != 0)
	{
		size = ie_len > sizeof(prsp->BssHT.bdHTInfoBuf)? sizeof(prsp->BssHT.bdHTInfoBuf): ie_len;
		_memcpy(prsp->BssHT.bdHTInfoBuf, pie+2, size);
		prsp->BssHT.bdHTInfoLen= size;

	}

	//2<2>If peer is HT, but not WMM, call QosSetLegacyWMMParamWithHT()
	if(prsp->BssHT.bdHTCapLen != 0)
	{
		prsp->BssHT.bdSupportHT = _TRUE;

		DEBUG_INFO(("GetValuefromBeacon or ProbeResp: AP supportHT !"));		

		// TODO: If peer is HT, but not WMM ?
		#if 0
		if(bssDesc->BssQos.bdQoSMode == QOS_DISABLE)
			QosSetLegacyWMMParamWithHT(Adapter, bssDesc);	
		#endif
	}
	else
	{

		prsp->BssHT.bdSupportHT = _FALSE;
	}

	
	
}

#endif

/*

We must make 2 records
One is for prsp, another is for ss_res

*/

static int collect_bss_info(struct rxirp *rxirp, NDIS_WLAN_BSSID_EX *prsp)
{
	int				i, index, len;
	unsigned char		*addr, *p, *pframe, *sa, *IEs; //, pw_dbm;
	unsigned short		val16;

        _adapter *		padapter=rxirp->adapter;
	unsigned char 		OUI1[] = {0x00, 0x50, 0xf2,0x01};

	//DOT11_WPA_MULTICAST_CIPHER wpaMulticastCipher;

#ifdef RTL_WPA2_CLIENT
	DOT11_WPA2_MULTICAST_CIPHER wpa2MulticastCipher;
	unsigned char OUI2[] = {0x00, 0x0f, 0xac};
#endif

#if 0
	if ( sitesurvey_res.bss_cnt >= MAX_BSS_CNT)
		return FAIL;
#endif

	pframe = get_rxpframe(rxirp);
	addr = GetAddr3Ptr(pframe);
	sa = GetAddr2Ptr(pframe);

	memset(prsp, 0, sizeof (NDIS_WLAN_BSSID_EX));

	prsp->NetworkTypeInUse = cpu_to_le32(Ndis802_11DS);
	
	IEs = prsp->IEs + (8 + 2 + 2);		// 8: TimeStamp, 2: Beacon Interval 2:Capability

	// checking SSID
	p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _SSID_IE_, &len,
		rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);

	// if scan specific SSID
	if (padapter->_sys_mib.sitesurvey_res.ss_ssidlen > 0)
	{
		if (!p)
		{
			DEBUG_ERR(("colletc_bss_info: !!ERROR!! get_ie == NULL\n"));
			return FAIL;
		}
			
		if ((padapter->_sys_mib.sitesurvey_res.ss_ssidlen != len) || (memcmp(padapter->_sys_mib.sitesurvey_res.ss_ssid, p+2, len)))
		{
			DEBUG_ERR(("colletc_bss_info: !!ERROR!! ss_ssidlen is wrong\n"));		
			return FAIL;
		}
	}
	
	prsp->Ssid.SsidLength = 0;
	
	if (p)
	{
		if (*(p + 1))
			_memcpy(prsp->Ssid.Ssid, (p + 2), *(p + 1));
			prsp->Ssid.SsidLength = cpu_to_le32(*(p + 1));	
	}
	
	memset(prsp->SupportedRates, 0, 16);
	
	//checking rate info...
	i = 0;
	p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
		rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
	if (p != NULL) {
		
		_memcpy(prsp->SupportedRates, (p + 2), len);
		i = len;
	#if 0	
		for(i=0; i<len; i++) {
			if (p[2+i] & 0x80)
				basicrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
			supportrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
		}
	#endif 	
	}

	p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
		rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
	if (p != NULL) {
		
		_memcpy(prsp->SupportedRates + i, (p + 2), len);
	#if 0	
		for(i=0; i<len; i++) {
			if (p[2+i] & 0x80)
				basicrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
			supportrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
		}
	#endif 	
	}	

	// Checking for DSConfig
	p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
	
	prsp->Configuration.DSConfig =  0;
	prsp->Configuration.Length = 0;
	
	if (p)
		prsp->Configuration.DSConfig = cpu_to_le32(*(p + 2));

#if 0
	prsp->network_type = judge_network_type(supported_rateset, rateset_len, prsp->channel);

	if (prsp->network_type == WIRELESS_INVALID)
		return FAIL;
#endif

		
	/*
	 * okay, recording this bss...
	 */
	//index = priv->site_survey.count;
	index = padapter->_sys_mib.sitesurvey_res.bss_cnt;
	padapter->_sys_mib.sitesurvey_res.bss_cnt++;

	//_memcpy(sitesurvey_res.bssid[index].addr, addr, ETH_ALEN);
	//_memcpy(prsp->MacAddress, addr, ETH_ALEN);


	//below is the fixed length IEs...
	_memcpy(get_timestampe_from_ie(prsp->IEs), (pframe + sizeof (struct ieee80211_hdr_3addr) ), 8);
	_memcpy(get_beacon_interval_from_ie(prsp->IEs),  (pframe + sizeof (struct ieee80211_hdr_3addr) + 8 ), 2);
	_memcpy(get_capability_from_ie(prsp->IEs),  (pframe + sizeof (struct ieee80211_hdr_3addr) + 8 + 2), 2);

	_memcpy(&prsp->Configuration.BeaconPeriod, get_beacon_interval_from_ie(prsp->IEs), 2);

	val16 = get_capability(prsp);
	
	if (val16 & BIT(0))
	{
		prsp->InfrastructureMode = cpu_to_le32(Ndis802_11Infrastructure);
		_memcpy(prsp->MacAddress, sa, ETH_ALEN);
	}
	else
	{
		prsp->InfrastructureMode = cpu_to_le32(Ndis802_11IBSS);
		_memcpy(prsp->MacAddress, addr, ETH_ALEN);
	}
	
	if (val16 & BIT(4))
		prsp->Privacy = 1;
	else
		prsp->Privacy = 0;

	prsp->Configuration.ATIMWindow = 0;	
	
	if (prsp->InfrastructureMode == Ndis802_11IBSS)
	{
		// Checking for IBSS Parameter Set...
		p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _IBSS_PARA_IE_, &len,
		rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);	
		
		if (p)
			_memcpy(&(prsp->Configuration.ATIMWindow), (p+2), 2);	
	}
	
	//Then collect the variable length IE
#if 0	

	#define _SSID_IE_                      	0	// Shall copy to Ssid
	#define _SUPPORTEDRATES_IE_     		1	// Shall copy to Supportedrates
	#define _DSSET_IE_                  	3	// Shall copy to NDIS_802_11_CONFIGURATION
	#define _TIM_IE_                        5	// Shall copy to IEs
	#define _IBSS_PARA_IE_                  6	// Shall copy to NDIS_802_11_CONFIGURATION
	#define _QBSSLOAD_IE_                   11	// Shall copy to IEs
	#define _EDCASETS_IE_                   12	// Shall copy to IEs
	#define _TS_IE_                         13	
	#define _TCLASS_IE_                     14
	#define _SCHEDULE_IE_                   15	 
	#define _TSDELAY_IE_                    43	
	#define _TCLASSPROCESSING_IE_   		44	
	#define _QOS_CAPABILITY_IE_     		46	// Shall copy to IEs
	#define _CHLGETXT_IE_                   16	
	#define _RSN_IE_2_                      48	// Shall copy to IEs
	#define _RSN_IE_1_                      221
	#define _ERPINFO_IE_                    42	// Shall copy to IEs
	#define _EXT_SUPPORTEDRATES_IE_ 		50	// Shall copy to Supportedrates
	

#endif
	
	p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _TIM_IE_, &len,
		rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
	if (p != NULL)
	{
		//printk("recording TIM with len =%d\n", len);
		_memcpy(IEs, p , (len + 2));
		IEs += (len + 2);
	}
	
#if 0
	//checking for QBSSID
	if (val16 & BIT(9)) 
	{

		//checking for parameters set...
		p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _EDCASETS_IE_, &len,
		pfrinfo->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);

		if ((p != NULL) && (len == 18))  {
			//got an valid EDCA Parameter Sets...
			//_memcpy(prsp->edca_parmsets, (p+2), 18);
			//printk("recording EDCA IE with len =%d\n", len);
			_memcpy(IEs, p, (len + 2));
			IEs += (len + 2);
				
		}

		//checking for QBSS Load...

		p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _QBSSLOAD_IE_, &len,
		pfrinfo->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);

		if ((p != NULL) && (len == 5))  {
			//got an valid EDCA Parameter Sets...
			//_memcpy(prsp->qbssload, (p+2), 5);
			//printk("recording QBSSLoad IE with len =%d\n", len);
			_memcpy(IEs, p, (len + 2));
			IEs += (len + 2);	
		}

	}
#endif

	if (val16 & BIT(4))
	{
		p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _RSN_IE_2_, &len,
		rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
		
		if (p) 
		{
			//printk("recording WAP2 IE with len =%d\n", len);
			_memcpy(IEs, p, (len + 2));
			IEs += (len + 2);
		}
		
		for (p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; ;p += (len + 2))
		{
			p = get_ie(p, _RSN_IE_1_, &len,	rxirp->rx_len - sizeof(struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
		
			if ((p) && (!memcmp(p+2, OUI1, 4)))
			{
				//printk("recording WPA IE with len =%d\n", len);
				_memcpy(IEs, p, (len + 2));
				IEs += (len + 2);
				break;
			}
			
			if ((p == NULL) || (len == 0))
			{
				break;
			}
		}
	}
	
	p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
	rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
	
	if (p) {
		//printk("recording ERPINFO IE with len =%d\n", len);
		_memcpy(IEs, p, (len + 2));
		IEs += (len + 2);
	}

#ifdef CONFIG_RTL8192U
#ifdef CONFIG_WMM
	
	if (1)
	{
		for (p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; ;p += (len + 2))
		{

			p = get_ie(p, _RSN_IE_1_, &len, rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
	
			if (p) 
			{
				if ((!memcmp(p+2, WMM_IE, 6)) || (!memcmp(p+2, WMM_PARA_IE, 6)))
				{
					DEBUG_INFO(("got WMM IE in beacon/probe rsp\n"));
					_memcpy(IEs, p, (len + 2));
					IEs += (len + 2);
					_memcpy(&(prsp->BssQos.QosInfoField),p+8, sizeof(QOS_INFO_FIELD));

					prsp->BssQos.bdQoSMode |= QOS_WMM;

					p += 10;  // start of EDCA parameters

					_memcpy(&(prsp->BssQos.AcParameter),p, sizeof(AC_PARAM)*4);
					break;
				}
			}

			if ((p == NULL) || (len == 0))
			{
				break;
			}

		}
		
	}

#endif
#endif
	
	// Get HT related Info
	HTGetValueFromBeaconOrProbeRsp(padapter, pframe, prsp,rxirp->rx_len);


	//marc: to get the reserved47 IE to identify whether it is broadcom AP
	p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _RESERVED47_, &len,
	rxirp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
	
	if (p) {
		//printk("recording RESERVED47 IE with len =%d\n", len);
		_memcpy(IEs, p, (len + 2));
		IEs += (len + 2);
	}

	prsp->IELength = cpu_to_le32(IEs - prsp->IEs);
	
	if (le32_to_cpu(prsp->IELength) > MAX_IES_LEN)
	{
		DEBUG_ERR(("colletc_bss_info: !!ERROR!! IELength = %d\n", le32_to_cpu(prsp->IELength)));
		return FAIL;
	}
	
	return SUCCESS;
}


static unsigned int OnProbeRsp(struct rxirp *rx_irp)
{
	unsigned char	*pframe;
	_adapter *padapter = rx_irp->adapter;
	struct eeprom_priv *peepriv = &(padapter->eeprompriv);
#ifdef LINUX_TASKLET
        struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
        if(check_fwstate(pmlmepriv, _FW_LINKED)==_TRUE)
        {
                return SUCCESS;
        }
#endif

	pframe = get_rxpframe(rx_irp);

	if (!memcmp(GetAddr1Ptr(pframe), myid(peepriv), ETH_ALEN))
	{
		_down_sema(&(padapter->_sys_mib.queue_event_sema));	
	
		report_BSSID_info(rx_irp);
		
		_up_sema(&(padapter->_sys_mib.queue_event_sema));	
	}

	return SUCCESS;
}

static unsigned int OnBeacon(struct rxirp *irp)
{
	unsigned char *pframe, *p = NULL;
	struct ieee80211_hdr *hdr;
	struct sta_data *psta;
	unsigned char supportRate[32];
	int	len;//, i;
	_adapter *padapter = irp->adapter;
	struct registry_priv *pregistrypriv = &(padapter->registrypriv);
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	u8 beacon_cnt = padapter->_sys_mib.beacon_cnt;
	
#ifdef CONFIG_ERP	
	int legacy = 1;
#endif

	int	supportRateNum = 0;
	
#ifdef CONFIG_POWERSAVING
	unsigned char tim = 0, dtim = 0;
	int offset;
	unsigned short bcn_itv;

	pframe = get_rxpframe(irp);
	hdr = (struct ieee80211_hdr*)pframe;

	if(!memcmp(GetAddr3Ptr(pframe), get_bssid(pmlmepriv), ETH_ALEN)) { /* BCN's BSSID == myBSSID */
		_sys_mib->bcn_to_cnt = 0;
		if(ps_mode != PS_MODE_ACTIVE) {
			/*	-- disable_BCN_TO_counter() --
				1. Stop BCN_TO_counter
				2. Clear BCN_TO counter INT
			*/
			rtl_outb(TCCNR, rtl_inb(TCCNR) & (~TCCNR_BCN_TO_EN));
			rtl_outb(TCIR, (rtl_inb(TCIR) & 0x0f) | TCIR_BCN_TO_INT);


			/* TODO: This should be moved to proper place. should we sawp byte order? */
			bcn_itv = cpu_to_le16(*((unsigned short *)(pframe + sizeof (struct ieee80211_hdr_3addr) + 8)));
			if(_sys_mib->ps_bcn_itv != bcn_itv) {
				if( bcn_itv > 500 ) {	//modified by scott for bcn overlap issue
					bcn_itv = 1024;
				}
				_sys_mib->ps_bcn_itv = bcn_itv;
				//rtl_outl(BCNITV, bcn_itv);
			}
			//dprintk("BCNITV:%d\n", bcn_itv);

			p = get_ie(pframe + sizeof (struct ieee80211_hdr_3addr) + _BEACON_IE_OFFSET_, _TIM_IE_, &len,
				irp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
			if(p != NULL) {

				/* TODO: This should be moved to proper place. */
				if((ps_mode == PS_MODE_MAX) && (_sys_mib->ps_dtim_cnt != p[3]) && (p[2] == 0)) {
					/* Perry:
					   We are in MAX power saving mode now. 
					   1. If DTIM period is not equal to current bss, 
					   2. If current DTIM count equals to 0.
					   update it and refresh BCN registers.
					*/
					_sys_mib->ps_dtim_cnt = p[3];
					rtl_outl(BCNITV, ps_bcn_itv * ps_dtim_cnt);
					rtl_outl(BINTRITV, ps_bintritv * ps_dtim_cnt);			
				}


				if(p[4] & BIT(0)) {
					dtim = 1;
				}
				offset = (p[4] & 0xfe) >> 1;	// Get the offset
				if(len > 3) {
					if(((offset << 3) <= _sys_mib->aid) && (_sys_mib->aid <= ((offset + len - 3) << 3))) {
						/* aid/8 must larger than offset  */
						offset = (_sys_mib->aid / 8) - offset;
						sprintk("In parsing TIM, offset: %d, TIM: %x", offset, p[5]);
						if(p[5 + offset] & BIT((_sys_mib->aid & 0x7)))
							tim = 1;
					}
				}
			}
			dprintk("bcn_itv:%d ps_bcn:%d dtim_per:%d dtim:%d tim:%d aid:%d seq:%d\n", bcn_itv, ps_bcn_itv, ps_dtim_cnt, dtim, tim, _sys_mib->aid, (__cpu_to_le16(hdr->seq_ctl) & 0xfff0) >> 4);
			SIG_DEBUG(SIG_PS_ID(21), 3, bcn_itv, dtim, tim);
#if 0
			if(bcn_itv == 100) {
				int i, j;
				printk("%x %x %x %x\n", 
					irp->swcmd.rxcmd_offset0,
					irp->swcmd.rxcmd_offset4,
					irp->swcmd.rxcmd_offset8,
					irp->swcmd.rxcmd_offsetc
				);	
#if 1
				dprintk("@@@@ seq:%d bssid:%x:%x:%x:%x:%x:%x\n", (__cpu_to_le16(hdr->seq_ctl) & 0xfff0) >> 4, 
					hdr->addr3[0],
					hdr->addr3[1],
					hdr->addr3[2],
					hdr->addr3[3],
					hdr->addr3[4],
					hdr->addr3[5]
				);
#endif
				for(i = 0 ; i < 64/ 4; i++) {
					for(j = 0; j < 4; j++) {
						printk("%x ", pframe[((i*4) + j)]);
					}
					printk("\n");
				}
			}
#endif 
			CLR_DATA_OPEN(ps_data_open, BCN_DATA_OPEN);
			ps_OnBeacon[ps_mode](dtim, tim);
		}
	}
#endif
	
	if (OPMODE & WIFI_AP_STATE)
		return SUCCESS;
	
	//printk("\non beacon\n");
	
	pframe = get_rxpframe(irp);
	hdr = (struct ieee80211_hdr*)pframe;

	if (OPMODE & WIFI_SITE_MONITOR)
	{
		_down_sema(&(padapter->_sys_mib.queue_event_sema));	

		report_BSSID_info(irp);

		_up_sema(&(padapter->_sys_mib.queue_event_sema));	

		return SUCCESS;
	}
	
	// to listen to the AP that is not been associated with 8711
	if (memcmp(hdr->addr3,  _sys_mib->cur_network.MacAddress, ETH_ALEN))
	{
#ifdef CONFIG_ERP		
		if ((OPMODE & WIFI_ADHOC_STATE) && 
		((_sys_mib->network_type == WIRELESS_11G) || (_sys_mib->network_type == WIRELESS_11BG)))
		{
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
				irp->rx_len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p) 
			{
				len = (len > 8)? 8: len;
				_memcpy(supportRate, p+2, len);
				supportRateNum += len;
			}

			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
				irp->rx_len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p) 
			{
				len = (len > 8)? 8: len;
				_memcpy(supportRate+supportRateNum, p+2, len);
				supportRateNum += len;
			}

			for (i = 0; i < supportRateNum; i++) 
			{
				if (!is_CCK_rate(supportRate[i]&0x7f)) 
				{
					legacy = 0;
					break;
				}
			}

			// look for ERP IE and check non ERP present
			if (legacy == 0) 
			{
				p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
					irp->rx_len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
				if (p && (*(p+2) & BIT(0)))
					legacy = 1;
			}

			if (legacy) 
			{
				//update beacon ERP IE
				_sys_mib->dot11ErpInfo.olbcDetected |= 1;
			}		
			
			if (pregistrypriv->vcs_mode == 2)
			{
				if (legacy)
				{
					_sys_mib->dot11ErpInfo.protection = pregistrypriv->vcs_type;
				}
				else
				{
					_sys_mib->dot11ErpInfo.protection = 0;
				}
			}		
		}
#endif		
		return SUCCESS;
	} 



	if (_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)
	{
		if(_sys_mib->join_res == STATE_Sta_Join_Wait_Beacon)
		{	
		
			join_ibss(padapter,pframe,  irp->rx_len);
			_sys_mib->join_res = STATE_Sta_Ibss_Active;
		}

	}
	
	psta = search_assoc_sta(hdr->addr2,padapter);
	if (psta == NULL) 
	{
		if (_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)
		{
			report_add_sta_event(padapter, hdr->addr2);
		}	
	}
	else
	{
		psta->stats->rx_pkts++;
				
		if (_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)
		{
			// get rates
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
				irp->rx_len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);			
			if (p != NULL)
			{
				_memcpy(supportRate, p+2, len);
				supportRateNum = len;
			}
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
				irp->rx_len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p != NULL) 
			{
				_memcpy(supportRate+supportRateNum, p+2, len);
				supportRateNum += len;
			}
			
			update_supported_rate(padapter, get_sa(pframe), supportRate, supportRateNum);
			
#ifdef CONFIG_ERP			
			if ((_sys_mib.network_type == WIRELESS_11G) || (_sys_mib.network_type == WIRELESS_11BG))
			{
				for (i = 0; i < supportRateNum; i++) 
				{
					if (!is_CCK_rate(supportRate[i] & 0x7f)) 
					{
						legacy = 0;
						break;
					}
				}

				// look for ERP IE and check non ERP present
				if (legacy == 0) 
				{
					p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
						irp->rx_len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
					if (p && (*(p+2) & BIT(0)))
						legacy = 1;
				}

				if (legacy) 
				{
					//update beacon ERP IE
					_sys_mib->dot11ErpInfo.olbcDetected |= 2;
					_sys_mib->dot11ErpInfo.nonErpStaNum = 1;
					//printk("n");
				}
				
				if (pregistrypriv->vcs_mode == 2)
				{
					if (legacy)
					{
						_sys_mib->dot11ErpInfo.protection = pregistrypriv->vcs_type;
					}
					else
					{
						_sys_mib->dot11ErpInfo.protection = 0;
					}	
				}
				
			}			
#endif
		}
		else
		{
			beacon_cnt--;
			
#ifdef CONFIG_WMM			
			if (beacon_cnt == 0)
			{				
				for (p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; ;p += (len + 2))
				{
					p = get_ie(p, _RSN_IE_1_, &len,	irp->rx_len - sizeof (struct ieee80211_hdr_3addr) - _BEACON_IE_OFFSET_);
	
					if (p) 
					{
						if (!memcmp(p+2, WMM_PARA_IE, 6)) 
						{
							if (WMM_para_cnt == ((*(p + 8)) & 0x0f))
							{
								goto __TX_VCS__;
							}
	
							WMM_para_cnt = ((*(p + 8)) & 0x0f);
							p += 10;  // start of EDCA parameters
							printk("update WMM_PARA_IE IE in beacon\n");
						} // end of get WMM PARA IE
					}
					
					if ((p == NULL) || (len == 0))
					{
						break;
					}
				}	
			} //end qos

__TX_VCS__:
			//TX_VCS: to record the ERP in the beacon of the assicated AP
#endif
			if ((beacon_cnt == 0) && (_sys_mib->vcs_mode== 2) &&
				((_sys_mib->network_type == WIRELESS_11G) || (_sys_mib->network_type == WIRELESS_11BG)))			
			{
				p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
					irp->rx_len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
					
				if (p && (*(p+2) & BIT(1)))
				{
					_sys_mib->dot11ErpInfo.protection = pregistrypriv->vcs_type;
					//rtl_outl(BK_TB_RT_PARAM, 0x0);
					DEBUG_INFO(("beacon: RTS on\n"));
				}
				else
				{
					_sys_mib->dot11ErpInfo.protection = 0;
					//rtl_outl(BK_TB_RT_PARAM, 0x80101015);
					//rtl_outl(BK_TB_RT_PARAM, 0x00100015);
					DEBUG_INFO(("beacon: RTS off\n"));
				}
			}
		
			if (beacon_cnt == 0)
			{
				beacon_cnt = 10;
			}
		
		}	
	}

	padapter->_sys_mib.beacon_cnt = beacon_cnt;

	return SUCCESS;

}

#ifdef CONFIG_POWERSAVING
extern volatile unsigned short dsr_pending;
#endif

static unsigned int OnAtim(struct rxirp *irp)
{
#ifdef CONFIG_POWERSAVING
    /* OnATIM is executed under interrupt disable region. */
	dprintk("ps_trx_atim:%d\n", ps_trx_atim);
	ps_trx_atim++;
	last_rpwm = PS_STATE_P1;    // simulate driver issues this RPWM
	dsr_pending |= RPWMDSR;
	SCRATCH_DEBUG(7);
	ps_80211_open = 1;
#endif
	return SUCCESS;
	

}


static unsigned int OnDisassoc(struct rxirp *irp)
{

	DEBUG_INFO(("OnDisassoc\n"));
	
	receive_disconnect(irp, -2);
	
#ifdef	CONFIG_POWERSAVING	
	ps_set_mode(PS_MODE_ACTIVE);
#endif	

	return SUCCESS;
}


static unsigned int OnDeAuth(struct rxirp *irp)
{
	DEBUG_INFO(("OnDeAuth\n"));
    
	receive_disconnect(irp, -1);

	return SUCCESS;
}

static unsigned int OnAction(struct rxirp *irp)
{
	u8 * pframe = NULL;
	u16 actselect ;
	_adapter *padapter = irp->adapter;
	pframe = get_rxpframe(irp);
	actselect =  *(u16 *)(pframe + WLAN_HDR_A3_LEN);
	DEBUG_INFO(("On Action frame\n"));
	switch(actselect)
	{
		case 0x0003://BAReq
//			printk("BAReq \n");
			OnADDBAReq(padapter, pframe);
			DEBUG_ERR(("OnADDBAReq \n"));
			break;
		case 0x0103://BARsp
			OnADDBARsp(padapter, pframe);
			DEBUG_INFO(("BARsp \n"));
			break;
		case 0x0203://DELBA
			OnDELBA(padapter, pframe);
			DEBUG_INFO(("DELBA\n"));
			DEBUG_INFO(("OnDELBA , not supported yet\n"));
			break;

		case 0x0107: //MIMO Power Save
//			OnMimoPs(Adapter, &mmpdu);
//			printk("MIMO\n");
		DEBUG_ERR(("OnMimoPs , not supported yet\n"));
			break;

		default:
			DEBUG_INFO(("Action : default: %x \n", actselect));
			break;

	}
	return SUCCESS;
}



static unsigned int DoReserved(struct rxirp *irp)
{
	return SUCCESS;
}

unsigned char getrfintfs_hdl(_adapter *padapter,unsigned char *pbuf)
{
	return SUCCESS;
}

unsigned char setratable_hdl(_adapter *padapter,unsigned char *pbuf)
{
	return SUCCESS;
}

unsigned char getratable_hdl(_adapter *padapter,unsigned char *pbuf)
{
	return SUCCESS;
}

unsigned char setusbsuspend_hdl(_adapter *padapter,unsigned char *pbuf)
{
	return SUCCESS;
}

void mgt_handlers(struct rxirp *rx_irp)
{
	unsigned int index;
	unsigned char *sa;
	struct mlme_handler *ptable;
	struct sta_data *psta = NULL;
	unsigned char *pframe = get_rxpframe(rx_irp);
	_adapter *padapter = rx_irp->adapter;
	u32 *popmode = &padapter->_sys_mib.mac_ctrl.opmode;
	u8 bc_addr[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

	if (GetFrameType(pframe) != WIFI_MGT_TYPE)
	{
	//	printk("type: %x\n", GetFrameType(pframe));	
		return;
	}
	
	if (memcmp(GetAddr1Ptr(pframe), myid(&padapter->eeprompriv), ETH_ALEN) &&
		memcmp(GetAddr1Ptr(pframe), bc_addr, ETH_ALEN)) 
	{
		return;
	}

	ptable = mlme_tbl;

	index = GetFrameSubType(pframe) >> 4;

	if(index == 5)
		DEBUG_INFO(("@@@@ mgt_handlers:got WIFI_PROBERSP index=%x\n", index));
	
	if (index >= ((sizeof (mlme_tbl))/(sizeof (struct mlme_handler))))
	{
		DEBUG_INFO(("Currently we do not support reserved sub-fr-type=%d\n", index));
		DEBUG_INFO(("frame len: %x\n", rx_irp->rx_len));
		DEBUG_INFO(("len in status: %x\n", *(((unsigned int*)pframe) - 4)));
		return;
	}
	ptable += index;

	sa = get_sa(pframe);

	psta = search_assoc_sta(sa, padapter);

	// only check last cache seq number for management frame
	if (psta != NULL) {
		if (GetRetry(pframe)) {
			if (GetTupleCache(pframe) == psta->rxcache->nonqos_seq) {
				//sys_mib.ext_stats.rx_decache++;
				DEBUG_INFO(("drop due to decache!\n"));
				return;
			}
		}
		psta->rxcache->nonqos_seq = GetTupleCache(pframe);
	}

	// log rx statistics...
#ifdef WDS
	if (psta && (psta->state & WIFI_WDS) && (ptable->num == WIFI_BEACON)) {
		psta->wds_rx_pkts++;
		psta->wds_rx_bytes += rxirp->pktlen;
	}
	else
#endif
	if (psta != NULL)
	{
		// If AP mode and rx is a beacon, do not count in statistics. david
		if (!((*(popmode) & WIFI_AP_STATE) && (ptable->num == WIFI_BEACON)))
		{
			#if 0
			psta->rx_pkts++;
			psta->rx_bytes += rxlen;
			//update_sta_rssi(priv, psta, pfrinfo);
			update_rxpkt(psta);
			update_rxbytes(psta, rx_irp->rx_len);
			#endif
		}
	}

	ptable->func(rx_irp);

/* it's never used now, comment by SungHau */
/*
#ifdef UNIVERSAL_REPEATER
	if (pfrinfo->is_br_mgnt) {
		pfrinfo->is_br_mgnt = 0;
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv))) {
			mgt_handler(GET_VXD_PRIV(priv), pfrinfo);
			return;
		}
	}
#endif
*/
	return;
}


