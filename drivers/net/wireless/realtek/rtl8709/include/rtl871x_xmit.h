#ifndef _RTL871X_XMIT_H_
#define _RTL871X_XMIT_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#if defined(CONFIG_RTL8192U)
	#include <rtl8192u_spec.h>
	#include <rtl8192u_regdef.h>
#elif defined(CONFIG_RTL8187)
	#include <rtl8187_spec.h>
#endif

#ifdef PLATFORM_WINDOWS

//#include <circ_buf.h>
#ifdef CONFIG_USB_HCI
#include <usb.h>
#include <usbdlib.h>
#include <usbioctl.h>
#endif


#define ETH_ALEN	6

#endif


#ifdef PLATFORM_LINUX
#if defined(CONFIG_RTL8187) || defined(CONFIG_RTL8192U)
#define HWXMIT_ENTRY    6
#endif

#endif


#include <xmit_osdep.h>

#ifdef PLATFORM_DVR
	#define ALIGNMENT 512
#else
	#define   ALIGNMENT 0
#endif


#define MAX_XMITBUF_SZ	(2048+ALIGNMENT)
#define MAX_MGNTBUF_SZ 	(512+ALIGNMENT)
#ifdef PLATFORM_OS_XP
	#define NR_XMITFRAME	64
#else
	#ifdef PLATFORM_DVR 
		#define NR_XMITFRAME   16	
	#else
		#define NR_XMITFRAME	56	
	#endif
#endif
#define TX_GUARD_BAND		5
#define MAX_NUMBLKS		(32)


#if 1
#define WEP_IV(pattrib_iv, dot11txpn, keyidx)\
do{\
	_memcpy(pattrib_iv, &dot11txpn, 3);\
	pattrib_iv[3] = ((keyidx & 0x3)<<6);\
	dot11txpn.val = (dot11txpn.val == 0xffffff) ? 0: (dot11txpn.val+1);\
}while(0);


#define TKIP_IV(pattrib_iv, dot11txpn)\
do{\
	pattrib_iv[0] = dot11txpn._byte_.TSC1;\
	pattrib_iv[1] = (dot11txpn._byte_.TSC1 | 0x20) & 0x7f;\
	pattrib_iv[2] = dot11txpn._byte_.TSC0;\
	pattrib_iv[3] = BIT(5);\
	_memcpy(&pattrib_iv[4], &dot11txpn._byte_.TSC2, 4);\
	dot11txpn.val = dot11txpn.val == 0xffffffffffffULL ? 0: (dot11txpn.val+1);\
}while(0);

#define AES_IV(pattrib_iv, dot11txpn)\
do{\
	pattrib_iv[0] = dot11txpn._byte_.TSC0;\
	pattrib_iv[1] = dot11txpn._byte_.TSC1;\
	pattrib_iv[2] = 0;\
	pattrib_iv[3] = BIT(5);\
	_memcpy(&pattrib_iv[4], &dot11txpn._byte_.TSC2, 4);\
	dot11txpn.val = dot11txpn.val == 0xffffffffffffULL ? 0: (dot11txpn.val+1);\
}while(0)



//	pxmitpriv->ts_queue.free_sz = ((value>>16)&0xF)<<2;
#define UPDATE_FFSZ(pxmitpriv, value)\
do{\
	pxmitpriv->bmc_txqueue.free_sz = ((value>>24)&0xF)<<2;\
	pxmitpriv->bk_txqueue.free_sz = ((value>>12)&0xF)<<2;\
	pxmitpriv->be_txqueue.free_sz = ((value>>8 )&0xF)<<2;\
	pxmitpriv->vi_txqueue.free_sz  = ((value>>4)&0xF)<<2;\
	pxmitpriv->vo_txqueue.free_sz = (value &0xF) <<2;\
} while(0)

//pxmitpriv->ts_txqueue.tail  = (value>>16)&0x7;
#define UPDATE_TXCMD_FWCLPTR(pxmitpriv, value)\
do{\
	pxmitpriv->vo_txqueue.tail = value&0x7;\
	pxmitpriv->vi_txqueue.tail  = (value>>4)&0x7;\
	pxmitpriv->be_txqueue.tail = (value>>8)&0x7;\
	pxmitpriv->bk_txqueue.tail = (value>>12)&0x7;\
	pxmitpriv->bmc_txqueue.tail = (value>>24) & 0x3;\
}while(0)	


#endif
struct txcmd_offset0 {
	uint	vcs:2;
	uint	pmpd:1;
	uint	rsvd0:6;
	uint	navadjust:7;
	uint	keyid:4;
	uint	polltoself:1;
	uint	spc:1;
	uint	eosp:1;
	uint	mdata:1;
	uint	tods:1;
	uint	notify:1;
	uint	rsvd1:3;
	uint	oui:1;
	uint	packet_type:1;
	uint	hwpc:1;
};

struct txcmd_offset4 {
	
	uint	burst:1;
	uint	block:1;
	uint	mgt:1;
	uint	ctrl:1;
	uint	hweosp:1;
	uint	tid:4;
	uint	non_qos:1;
	uint	macid:6;
	uint	frame_len:11;
	uint	no_encrypt:1;
	uint	splcp:1;
	uint	interrupt:1;
	uint	more_frag:1;
	uint	own_fw:1;
	
};

struct	txcmd_offset8	{
	uint	vcrate:4;
	uint	txrate:4;
	uint	antenna:1;
	uint	tssi:7;
	uint	txagc:8;
	uint	retxstatus:5;
	uint	pwrmgt:1;
	uint	txok:1;
	uint	own_mac:1;
};


struct	txcmd_offsetc	{
	uint	msdutm:16;
	uint	seq:12;
	uint	frag:4;	
};

struct tx_cmd {
	struct txcmd_offset0 txcmd0;
	struct txcmd_offset4 txcmd4;
	struct txcmd_offset8 txcmd8;
	struct txcmd_offsetc txcmdc;
};

union txcmd {
	struct tx_cmd cmd;
	uint value[4];
	
};

struct	hw_xmit	{
	_lock xmit_lock;
	_list	pending;
	_queue *sta_queue;
	struct hw_txqueue *phwtxqueue;
	sint	txcmdcnt;	
};


#define WLANHDR_OFFSET	64

#ifdef CONFIG_RTL8711
#define TXDESC_OFFSET 0
#define TX_FWINFO_OFFSET 0
#endif

#if defined(CONFIG_RTL8187)
#define TXDESC_OFFSET 32
#define TX_FWINFO_OFFSET 0
#endif

#ifdef CONFIG_RTL8192U

#define TXDESC_OFFSET sizeof(TX_DESC_8192_USB) //24 bytes
#define TX_FWINFO_OFFSET sizeof(TX_FWINFO_8192USB) //8 bytes


typedef struct _TX_FWINFO_8192USB{
	//DWORD 0
	u8			TxRate:7;
	u8			CtsEnable:1;
	u8			RtsRate:7;
	u8			RtsEnable:1;
	u8			TxHT:1;
	u8			Short:1;						//Short PLCP for CCK, or short GI for 11n MCS
	u8			TxBandwidth:1;				// This is used for HT MCS rate only.
	u8			TxSubCarrier:2;				// This is used for legacy OFDM rate only.
	u8			STBC:2;
	u8			AllowAggregation:1;
	u8			RtsHT:1;						//Interpre RtsRate field as high throughput data rate
	u8			RtsShort:1;					//Short PLCP for CCK, or short GI for 11n MCS
	u8			RtsBandwidth:1;				// This is used for HT MCS rate only.
	u8			RtsSubcarrier:2;				// This is used for legacy OFDM rate only.
	u8			RtsSTBC:2;
	u8			EnableCPUDur:1;				//Enable firmware to recalculate and assign packet duration
	
	//DWORD 1
	u32			RxMF:2;
	u32			RxAMD:3;
	u32			Reserved1:3;
	u32			TxAGCOffset:4;
	u32			TxAGCSign:1;
	u32			Tx_INFO_RSVD:6;
	u32			PacketID:13;
}TX_FWINFO_8192USB, *PTX_FWINFO_8192USB;

typedef struct _TX_DESC_8192_USB {
	//DWORD 0
	u16	PktSize;
	u8	Offset;
	u8	Reserved0:3;
	u8	CmdInit:1;
	u8	LastSeg:1;
	u8	FirstSeg:1;
	u8	LINIP:1;
	u8	OWN:1;

	//DWORD 1
	u8	TxFWInfoSize;
	u8	RATid:3;
	u8	DISFB:1;
	u8	USERATE:1;
	u8	MOREFRAG:1;
	u8	NoEnc:1;
	u8	PIFS:1;
	u8	QueueSelect:5;
	u8	NoACM:1;
	u8	Reserved1:2;
	u8	SecCAMID:5;
	u8	SecDescAssign:1;
	u8	SecType:2;

	//DWORD 2
	u16	TxBufferSize;
	//u16	Reserved2;
	u8	ResvForPaddingLen:7;
	u8	Reserved3:1;
	u8	Reserved4;

	//DWORD 3
	//u32	TxBufferAddr;

	//DWORD 4
	//u32	NextDescAddress;

	//DWORD 5, 6, 7
	u32	Reserved5;
	u32	Reserved6;
	u32	Reserved7;
	
}TX_DESC_8192_USB, *PTX_DESC_8192_USB;

#if 0
typedef struct _TX_DESC_CMD_819x_USB {
        //DWORD 0
	u16	Reserved0;
	u8	Reserved1;
	u8	Reserved2:3;
	u8	CmdInit:1;
	u8	LastSeg:1;
	u8	FirstSeg:1;
	u8	LINIP:1;
	u8	OWN:1;

        //DOWRD 1
	//u4Byte	Reserved3;
	u8	TxFWInfoSize;
	u8	Reserved3;
	u8	QueueSelect;
	u8	Reserved4;

        //DOWRD 2
	u16	TxBufferSize;
	u16	Reserved5;
	
       //DWORD 3,4,5
	//u4Byte	TxBufferAddr;
	//u4Byte	NextDescAddress;
	u32	Reserved6;
	u32	Reserved7;
	u32	Reserved8;
}TX_DESC_CMD_819x_USB, *PTX_DESC_CMD_819x_USB;
#endif



#endif /*CONFIG 8192 */






#if defined(CONFIG_RTL8187) || defined(CONFIG_RTL8192U)

#define NR_MGNTFRAME 8

struct	mgnt_frame	{
	_list	list;
	struct	pkt_attrib	attrib;
	_pkt 	*pkt;
	u8 		frame_tag;

#ifdef CONFIG_USB_HCI
	//insert urb, irp, and irpcnt info below...
	_adapter *	padapter;//u8 *dev;
	u8 *	mem_addr;      
	u32 	sz[8];	   
	PURB	pxmit_urb[8];
	
#ifdef PLATFORM_WINDOWS
	PIRP	pxmit_irp[8];
#endif

	u8 	bpending[8];
        sint 	ac_tag[8];
	sint 	last[8];
	uint 	irpcnt;         
	uint 	fragcnt;             	
#endif 

	uint	mem[(MAX_MGNTBUF_SZ >> 2)];

};

#endif /*CONFIG_RTL8187 CONFIG_RTL8192U*/


struct	xmit_frame	{
	_list	list;
	struct	pkt_attrib	attrib;
	_pkt *pkt;
	u8 		frame_tag;
	
#ifdef CONFIG_USB_HCI
	//insert urb, irp, and irpcnt info below...
       _adapter *	padapter;//u8 *dev;
       u8 *mem_addr;      
       u32 sz[8];	   
	PURB	pxmit_urb[8];
#ifdef PLATFORM_WINDOWS
	PIRP		pxmit_irp[8];
#endif
	u8 bpending[8];
	sint ac_tag[8];
	sint last[8];
       uint irpcnt;         
       uint fragcnt;             	
#endif

	uint	mem[(MAX_XMITBUF_SZ >> 2)];
	u8 	order_tag;  //interrupt flags control in usb_write_port
};


struct tx_servq {
	_list	tx_pending;
	_queue	sta_pending;
};




struct	sta_xmit_priv	{

	_lock	lock;
	sint	option;
	sint	apsd_setting;	//When bit mask is on, the associated edca queue supports APSD.


	struct tx_servq blk_q[MAX_NUMBLKS];
	struct tx_servq	be_q;			//priority == 0,3 
	struct tx_servq	bk_q;			//priority == 1,2
	struct tx_servq	vi_q;			//priority == 4,5
	struct tx_servq	vo_q;			//priority == 6,7
	_list 	legacy_dz;
	_list  apsd;

	uint	sta_tx_bytes;
	uint	sta_tx_pkts;
	uint	sta_tx_fail;

};


#define AC_BE_TAG	0
#define AC_BK_TAG	1
#define AC_VI_TAG	2
#define AC_VO_TAG	3
#define AC_TS_TAG	4
#define AC_BMC_TAG	5

#define BE_FF_SZ	0xE
#define BK_FF_SZ	0xE
#define VI_FF_SZ		0xE
#define VO_FF_SZ	0xE
#define TS_FF_SZ	0x4
#define BMC_FF_SZ	0xE



struct	hw_txqueue	{
	volatile sint	head;
	volatile sint	tail;
	volatile sint 	free_sz;	//in units of 64 bytes
	//volatile sint	budget_sz;
	volatile sint      free_cmdsz;
	volatile sint	 txsz[8];
	uint	ff_hwaddr;
	uint	cmd_hwaddr;
	sint	ac_tag;
};

/*





*/
#ifdef CONFIG_RTL8192U
#define TOTAL_TS_NUM 16


typedef union _BA_PARAM_SET {
	u8 charData[2];
	u16 shortData;

	struct {
		u16 AMSDU_Support:1;
		u16 BAPolicy:1;
		u16 TID:4;
		u16 BufferSize:10;
	} field;
} BA_PARAM_SET, *PBA_PARAM_SET;


typedef union _SEQUENCE_CONTROL{
	u16 ShortData;
	struct
	{
		u16	FragNum:4;
		u16	SeqNum:12;
	}Field;
}SEQUENCE_CONTROL, *PSEQUENCE_CONTROL;


typedef struct _BABODY {
	_timer				Timer;
	u8					bValid;	
	u8					DialogToken;
	BA_PARAM_SET		BaParamSet;
	u16					BaTimeoutValue;
	SEQUENCE_CONTROL	BaStartSeqCtrl;
} BABODY, *PBABODY;


typedef struct _TX_TS_RECORD{
//	TS_COMMON_INFO		TsCommonInfo;
	u16					TxCurSeq;
	PBABODY 			TxPendingBARecord;  	// For BA Originator
	PBABODY			TxAdmittedBARecord;	// For BA Originator
	u8					bAddBaReqInProgress;
	u8					bAddBaReqDelayed;
	u8					bUsingBa;
	_timer				TsAddBaTimer;
} TX_TS_RECORD, *PTX_TS_RECORD;


typedef struct _RX_TS_RECORD {
//	TS_COMMON_INFO		TsCommonInfo;
	u2Byte				RxIndicateSeq;
	u2Byte				RxTimeoutIndicateSeq;
	_list					RxPendingPktList;
	_timer				RxPktPendingTimer;
	BABODY				RxAdmittedBARecord;	 // For BA Recepient
	u2Byte				RxLastSeqNum;
	u1Byte				RxLastFragNum;
//	QOS_DL_RECORD		DLRecord;
} RX_TS_RECORD, *PRX_TS_RECORD;

typedef struct _RX_REORDER_ENTRY
{
	_list				List;
	u2Byte			SeqNum;
	//PRT_RFD			pRfd;  //FIXME
} RX_REORDER_ENTRY, *PRX_REORDER_ENTRY;


#endif /*CONFIG_RTL8192U*/

struct	xmit_priv	{
	
	_lock	lock;

	_sema	xmit_sema;
	_sema	terminate_xmitthread_sema;
	
	_queue	blk_strms[MAX_NUMBLKS];
	_queue	be_pending;
	_queue	bk_pending;
	_queue	vi_pending;
	_queue	vo_pending;
	_queue	bm_pending;
	
	_queue	legacy_dz_queue;
	_queue	apsd_queue;
	
	u8 *pallocated_frame_buf;
	u8 *pxmit_frame_buf;
	uint free_xmitframe_cnt;

	uint mapping_addr;
	uint pkt_sz;	
	
	_queue	free_xmit_queue;
	
	struct	hw_txqueue	be_txqueue;
	struct	hw_txqueue	bk_txqueue;
	struct	hw_txqueue	vi_txqueue;
	struct	hw_txqueue	vo_txqueue;
	struct	hw_txqueue	bmc_txqueue;

	uint	frag_len;

	_adapter	*adapter;
	
	u8    vcs_setting;
	u8	vcs;
	u8	vcs_type;
	u16  rts_thresh;
	
	uint	tx_bytes;
	uint	tx_pkts;
	uint	tx_drop;
#ifdef LINUX_TX_TASKLET
	struct hw_xmit hwxmits[6];
#else
	struct hw_xmit *hwxmits;
#endif
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	struct	hw_txqueue	mgnt_txqueue;
	struct	hw_txqueue	beacon_txqueue;
	_queue	mgnt_pending;
	_queue	beacon_pending;
	_queue	free_mgnt_queue;

	_sema	mgnt_sema;
	
	u8 *	pallocated_mgnt_frame_buf;
	u8 *	pxmit_mgnt_frame_buf;
	uint	free_mgnt_frame_cnt;

	u16 	ieee_basic_rate;
	u16 	ieee_rate;	
#endif

#ifdef CONFIG_RTL8192U
	_list Tx_TS_Admit_List;
	_list Tx_TS_Pending_List;
	_list Tx_TS_Unused_List;
	TX_TS_RECORD TxTsRecord;
	RX_TS_RECORD RxTsRecord;
	RX_REORDER_ENTRY RxReorderEntry[128];
#endif
};






static __inline _queue *get_free_xmit_queue(struct	xmit_priv	*pxmitpriv)
{
	return &(pxmitpriv->free_xmit_queue);
}

static __inline u16 ieeerate2rtlrate(int rate)
{
	switch(rate){
	case 10:		// 1M , b
	return 0;
	case 20:	//	2M   , b
	return 1;
	case 55://  5.5 M  , b
	return 2;
	case 110:// 11 M  ,  b
	return 3;
	case 60:// 6 M ,  g
	return 4;
	case 90:// 9M ,   g
	return 5;
	case 120:// 12 M , g
	return 6;
	case 180:// 18 M , g
	return 7;
	case 240:// 24 M ,  g
	return 8;
	case 360:// 36 M , g
	return 9;
	case 480:// 48 M , g
	return 10;
	case 540:// 54 M ,  g
	return 11;
// 802.11n mode 

	case MGN_MCS0:	
		return DESC90_RATEMCS0;	
	case MGN_MCS1:	
		return DESC90_RATEMCS1;	
	case MGN_MCS2:	
		return DESC90_RATEMCS2;
	case MGN_MCS3:	
		 return DESC90_RATEMCS3;
	case MGN_MCS4:
		return  DESC90_RATEMCS4;	
	case MGN_MCS5:	
		return  DESC90_RATEMCS5;	
	case MGN_MCS6:	
		return  DESC90_RATEMCS6;	
	case MGN_MCS7: 
		return  DESC90_RATEMCS7;	
	case MGN_MCS8:	
		return  DESC90_RATEMCS8;	
	case MGN_MCS9:	
		return  DESC90_RATEMCS9;	
	case MGN_MCS10:	
		return DESC90_RATEMCS10;	
	case MGN_MCS11:	
		return DESC90_RATEMCS11;	
	case MGN_MCS12:	
		return DESC90_RATEMCS12;	
	case MGN_MCS13:	
		return DESC90_RATEMCS13;	
	case MGN_MCS14:	
		return DESC90_RATEMCS14;	
	case MGN_MCS15:	
		return DESC90_RATEMCS15;	
	case (0x80|0x20): 
		return DESC90_RATEMCS32; 

	default:
	return 3;
	
	}
}

static __inline void init_hwxmits(struct hw_xmit *phwxmit, sint entry)
{
        sint i;
//_func_enter_;
        for(i = 0; i < entry; i++, phwxmit++)
        {
                _spinlock_init(&phwxmit->xmit_lock);
                _init_listhead(&phwxmit->pending);
                phwxmit->txcmdcnt = 0;
        }
//_func_exit_;
}
extern u8* get_xmit_payload_start_addr(struct xmit_frame *pxmitframe);
extern uint remainder_len(struct pkt_file *pfile);
extern void _init_sta_xmit_priv(struct sta_xmit_priv *psta_xmitpriv);
extern sint _init_hw_txqueue(struct hw_txqueue* phw_txqueue, u8 ac_tag);
extern sint update_attrib(_adapter *padapter, _pkt *pkt, struct pkt_attrib *pattrib);
extern sint xmitframe_coalesce(_adapter *padapter, _pkt *pkt, struct xmit_frame *pxmitframe);

extern sint txframes_pending_nonbcmc(_adapter * padapter);
extern void update_protection(_adapter *padapter, u8 *ie, uint ie_len);
extern struct xmit_frame *alloc_xmitframe(struct xmit_priv *pxmitpriv);//(_queue *pfree_xmit_queue)
extern struct mgnt_frame *alloc_mgnt_xmitframe(struct xmit_priv *pxmitpriv);
extern sint make_wlanhdr (_adapter *padapter, unsigned char *hdr, struct pkt_attrib *pattrib);
extern sint rtl8711_put_snap(u8 *data, u16 h_proto);
extern sint	free_xmitframe(struct xmit_priv *pxmitpriv, struct xmit_frame *pxmitframe);//(struct xmit_frame *pxmitframe, _queue *pfree_xmit_queue)
extern sint	free_mgntframe(struct xmit_priv *pxmitpriv, struct mgnt_frame *pmgntframe);//(struct xmit_frame *pxmitframe, _queue *pfree_xmit_queue)
extern void free_xmitframe_queue(struct xmit_priv *pxmitpriv, _queue *pframequeue );
extern sint xmit_classifier(_adapter *padapter, struct xmit_frame *pxmitframe);
extern sint xmit_classifier_direct(_adapter *padapter, struct xmit_frame *pxmitframe);
extern void check_free_tx_packet (struct xmit_priv *pxmitpriv,struct xmit_frame *pxmitframe);

extern thread_return xmit_thread(void *context);
#ifdef LINUX_TX_TASKLET

extern void xmit_tasklet(_adapter * padapter);
#endif

#endif	//_RTL871X_XMIT_H_

