#ifndef _RTL871X_RECV_H_
#define _RTL871X_RECV_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#ifdef PLATFORM_OS_XP
#define NR_RECVFRAME 64
#else
#ifdef CONFIG_RTL8192U



#if 1
		#define MAX_SUBFRAME_COUNT 8
		#define NR_RECVFRAME 12
		#define SZ_RECVFRAME 2000
#else		
		#define MAX_SUBFRAME_COUNT 64
		#define NR_RECVFRAME 8
		#define SZ_RECVFRAME 9100
//		#define SZ_RECVFRAME 4096
#endif

#else
	#define NR_RECVFRAME 32
	#define SZ_RECVFRAME 2048
#endif

#endif

#ifdef CONFIG_RTL8192U
#define RXFRAME_BLOCK_NO 36
#else
#define RXFRAME_BLOCK_NO 9
#endif

#define MAX_RXFRAME_CNT	512
#define MAX_RX_NUMBLKS		(32)
#define RXFRAME_ALIGN	8
#define RXFRAME_ALIGN_SZ	(1<<RXFRAME_ALIGN)
#define MAX_RXSZ	(RXFRAME_ALIGN_SZ*RXFRAME_BLOCK_NO)
#define RECVFRAME_HDR_ALIGN 128
#define state_no_rx_data -1
#define state_no_rxframe 0

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
#define NR_RXIRP	8
#endif

#define SNAP_SIZE sizeof(struct ieee80211_snap_hdr)

struct recv_test{
//#ifdef CONFIG_BIG_ENDIAN	//jackson

    // DWORD 1
	u16		frame_length1:8;

	u16		pwrmgt:1;
	u16		crc32:1;
	u16		icv:1;
	u16		decrypted:1;
	u16		frame_length2:4;	

	u16		qos:1;		
	u16		rxdata:4;
	u16		eosp:1;
	u16		mdata:1;
	u16		splcp:1;


	u16		own:1;
	u16		retx:1;
	u16		mgt:1;
	u16		data:1;
	u16		tid:4;




	// DWORD 2
	u16		antenna:1;
	u16		addr_hit:1;
	u16		txmac_id:6;


	u16		agc:8;
	u16		sq:8;
	u16		queue_size:8;



	// DWORD 3
	u16		cfo:6;
	u16		ack_policy:2;



	u16		pwdb:8;
	u16		fot1:8;
	u16		rssi:7;
	u16		fot2:1;

	// DWORD 4
	u16		seq1:8;
	u16		frag:4;
	u16		seq2:4;

	u16		no_mcsi1:2;

	u16		k_int:6;

	u16		rxsnr:6;
	u16		no_mcsi2:2;

//#endif

};

#ifdef CONFIG_RTL8711

struct	recv_stat {

#ifdef CONFIG_LITTLE_ENDIAN	//jackson
    // DWORD 1
	u16		frame_length:12;
	u16		decrypted:1;
	u16		icv:1;
	u16		crc32:1;
	u16		pwrmgt:1;
	u16		splcp:1;
	u16		mdata:1;
	u16		eosp:1;
	u16		rxdata:4;
	u16		qos:1;
	u16		tid:4;
	u16		data:1;
	u16		mgt:1;
	u16		retx:1;
	u16		own:1;
	// DWORD 2
	u16		txmac_id:6;
	u16		addr_hit:1;
	u16		antenna:1;
	u16		agc:8;
	u16		sq:8;
	u16		queue_size:8;
	// DWORD 3
	u16		ack_policy:2;
	u16		cfo:6;
	u16		pwdb:8;
	u16		fot:9;
	u16		rssi:7;
	// DWORD 4
	u16		seq:12;
	u16		frag:4;
	u16		k_int:6;
	u16		no_mcsi:4;
	u16		rxsnr:6;
		
#elif CONFIG_BIG_ENDIAN	//jackson



#endif		
		
};

#endif

#if defined(CONFIG_RTL8187)

struct recv_stat {

#ifdef _LINUX_BYTEORDER_LITTLE_ENDIAN_H

	//DWORD 0
	unsigned int	frame_length:12;
	unsigned int	icv:1;
	unsigned int	crc32:1;
	unsigned int	pwrmgt:1;
	unsigned int	res:1;
	unsigned int	bar:1;
	unsigned int	pam:1;
	unsigned int	mar:1;
	unsigned int	qos:1;
	unsigned int	rxrate:4;
	unsigned int	trsw:1;
	unsigned int	splcp:1;
	unsigned int	fovf:1;
	unsigned int	dmafail:1;
	unsigned int	last:1;
	unsigned int	first:1;
	unsigned int	eor:1;
	unsigned int	own:1;

	//DWORD 1
	unsigned int	tsftl;

	//DWORD 2
	unsigned int	tsfth;

	//DWORD 3
	unsigned char	sq;
	unsigned char	rssi:7;
	unsigned char	antenna:1;
	unsigned char	agc;
	unsigned char decrypted:1;
	unsigned char	wakeup:1;
	unsigned char	shift:1;
	unsigned char	rsvd0:5;

	//DWORD 4
	unsigned int	num_mcsi:4;
	unsigned int	snr_long2end:6;
	unsigned int	cfo_bias:6;
	unsigned int	pwdb_g12:8;
	unsigned int	fot:8;

#else

#error "please modify tx_desc to your own\n"

#endif

}__attribute__((packed));

#endif
#ifdef CONFIG_RTL8192U

struct recv_stat {

#ifdef _LINUX_BYTEORDER_LITTLE_ENDIAN_H

	//DOWRD 0
	unsigned int			frame_length:14;
	unsigned int			crc32:1;
	unsigned int			icv:1;
	unsigned int			RxDrvInfoSize:8;
	unsigned int			RxShift:2;
	unsigned int			PHYStatus:1;
	unsigned int			SWDec:1;
	unsigned int			Reserved1:4;

	//DWORD 1
	unsigned int			decrypted;//Reserved2
	
#else

#error "please modify tx_desc to your own\n"

#endif /*_LINUX_BYTEORDER_LITTLE_ENDIAN_H*/

}__attribute__((packed));


//
// Driver info are written to the RxBuffer following Rx status desc.
// The size of driver info is indicated by RxDrvInfoSize in Rx status desc,
// not sizeof(RX_DRIVER_INFO_819xUSB). Here we only define the fields 
// that we need to process by now.
//
//typedef struct _RX_DRIVER_INFO_819xUSB{
struct recv_drv_info{
	//DWORD 0
	unsigned int			Reserved1:12;
	unsigned int			PartAggr:1;
	unsigned int			FirstAGGR:1;
	unsigned int			Reserved2:2;

	unsigned int			RxRate:7;
	unsigned int 			RxHT:1;

	unsigned int			BW:1;
	unsigned int			SPLCP:1;
	unsigned int			Reserved3:2;
	unsigned int			PAM:1;	
	unsigned int			Mcast:1;	
	unsigned int			Bcast:1;	
	unsigned int			Reserved4:1;	

	//DWORD 1
	unsigned int			TSFL;	
	
}__attribute__((packed));


#endif /*CONFIG_RTL8192U*/

struct	stainfo_rxcache	{
	u16 	tid_rxseq[16];
	unsigned short 	nonqos_seq;
/*	
	unsigned short 	tid0_rxseq;
	unsigned short 	tid1_rxseq;
	unsigned short 	tid2_rxseq;
	unsigned short 	tid3_rxseq;
	unsigned short 	tid4_rxseq;
	unsigned short 	tid5_rxseq;
	unsigned short 	tid6_rxseq;
	unsigned short 	tid7_rxseq;
	unsigned short 	tid8_rxseq;
	unsigned short 	tid9_rxseq;
	unsigned short 	tid10_rxseq;
	unsigned short 	tid11_rxseq;
	unsigned short 	tid12_rxseq;
	unsigned short 	tid13_rxseq;
	unsigned short 	tid14_rxseq;
	unsigned short 	tid15_rxseq;
*/
};


struct rx_pkt_attrib	{

	u8	qos;

	u8	to_fr_ds;
	u8	frag_num;
	u16	seq_num;
	u8	pw_save;
	u8	mfrag;
	u8	mdata;	
	u8	privacy; 	//in frame_ctrl field
	u8	bdecrypted;
	int	hdrlen;		//the WLAN Header Len
	int	encrypt;		//when 0 indicate no encrypt. when non-zero, indicate the encrypt algorith
	int	iv_len;
	int	icv_len;
	int	priority;
	int	ack_policy;
	
	u8	dst[ETH_ALEN];
	u8	src[ETH_ALEN];
	u8	ta[ETH_ALEN];
	u8	ra[ETH_ALEN];
	u8	bssid[ETH_ALEN];
	
#ifdef CONFIG_RTL8192U
	u32				Length;
	u8				DataRate;			// In 0.5 Mbps
	u8				SignalQuality; // in 0-100 index. 
	s32				RecvSignalPower; // in dBm.
	s8				RxPower; // in dBm Translate from PWdB
	u8				SignalStrength; // in 0-100 index.
	u16				bHwError:1;
	u16				bCRC:1;
	u16				bICV:1;
	u16				bShortPreamble:1;
	u16				Antenna:1;	//for rtl8185
	u16				Decrypted:1;	//for rtl8185, rtl8187
	u16				Wakeup:1;	//for rtl8185
	u16				Reserved0:1;	//for rtl8185
	u8				AGC;
	u32				TimeStampLow;
	u32				TimeStampHigh;
	u8				bShift;
//	u8				bIsQosData;		// Added by Annie, 2005-12-22.
//	u8				UserPriority;

	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//1Attention Please!!!<11n or 8190 specific code should be put below this line>
	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!

	u8				RxDrvInfoSize;
	u8				RxBufShift;
	u8				bIsAMPDU;
	u8				bFirstMPDU;
	u8				bContainHTC;
	u8				RxIs40MHzPacket;
	u32				RxPWDBAll;
	u8				RxMIMOSignalStrength[4]; 	// in 0~100 index
	s8				RxMIMOSignalQuality[2];
	u8				bPacketMatchBSSID;	
	u8				bIsCCK;
	u8				bPacketToSelf;

	u16			nTotalFrag;
	u8			*SubframeArray[MAX_SUBFRAME_COUNT];
	_pkt 			cloned_skb[MAX_SUBFRAME_COUNT];	
	u16			SubframeLenArray[MAX_SUBFRAME_COUNT];
	u8			nTotalSubframe;
#ifdef USB_RX_AGGREGATION_SUPPORT
	u8 bIsRxAggrSubframe ; 
#endif
	
#endif

};




/*

	head  ----->

		data  ----->
	
			payload
	
		tail  ----->
	
	
	end   ----->

	len = (unsigned int )(tail - data);

*/


struct recv_frame_hdr{
_list	list;
_pkt	*pkt;




_adapter  *adapter;

#ifdef CONFIG_USB_HCI

PURB	purb;
#ifdef PLATFORM_WINDOWS
PIRP		pirp;
#endif
u8	irp_pending;

#endif

#ifdef CONFIG_SDIO_HCI

#ifdef USE_ASYNC_IRP

#endif

#endif

u8 fragcnt;

struct rx_pkt_attrib attrib;

uint  len;	//optional
u8 *rx_head;
u8 *rx_data;
u8 *rx_tail;
u8 *rx_end;
u8 buse_mem;
};



union recv_frame{

union{	
_list list;
struct recv_frame_hdr hdr;
uint mem[RECVFRAME_HDR_ALIGN>>2];
}u;

#ifndef PLATFORM_LINUX
uint mem[MAX_RXSZ>>2];
#endif

};


/*
accesser of recv_priv: recv_entry(dispatch / passive level); recv_thread(passive) ; returnpkt(dispatch)
; halt(passive) ;

using enter_critical section to protect
*/
struct recv_priv {

  	  _lock	lock;

	_sema	recv_sema;
	_sema	terminate_recvthread_sema;
	
	_queue	blk_strms[MAX_RX_NUMBLKS];    // keeping the block ack frame until return ack
	_queue	free_recv_queue;
	_queue	recv_pending_queue;
	

	u8 *pallocated_frame_buf;
	u8 *precv_frame_buf;    //2048 alignment
	
	uint free_recvframe_cnt;
	
	_nic_hdl  RxPktPoolHdl;
	_nic_hdl  RxBufPoolHdl;

	_adapter	*adapter;

	uint	counter; //record the number that up-layer will return to drv; only when counter==0 can we  release recv_priv 
#ifdef PLATFORM_WINDOWS
	NDIS_EVENT 	recv_resource_evt ;
#endif	
	uint	rx_bytes;
	uint	rx_pkts;
	uint	rx_drop;

#ifdef CONFIG_USB_HCI	
	u8 *pallocated_urb_buf;
	u8 *pnxtallocated_urb_buf;
	_sema		allrxreturnevt; //NDIS_EVENT		allrxreturnevt;
	u8  rx_pending_cnt;
#endif

#ifdef CONFIG_RTL8192U
	/*Last RxDesc TSF value*/
	u32	LastRxDescTSFHigh;
	u32	LastRxDescTSFLow;
#endif


};




struct sta_recv_priv {
    
    _lock	lock;
	sint	option;	
	
	_queue	blk_strms[MAX_RX_NUMBLKS];
	_queue defrag_q;	 //keeping the fragment frame until defrag
	
	struct	stainfo_rxcache rxcache;  
	
	uint	sta_rx_bytes;
	uint	sta_rx_pkts;
	uint	sta_rx_fail;


};



//rtl8711_recv.c
extern void _init_sta_recv_priv(struct sta_recv_priv *psta_recvpriv);
extern union recv_frame *alloc_recvframe (_queue *pfree_recv_queue);  //get a free recv_frame from pfree_recv_queue
extern int	free_recvframe(union recv_frame *precvframe, _queue *pfree_recv_queue);  
extern union recv_frame *dequeue_recvframe (_queue *queue);
extern int	enqueue_recvframe(union recv_frame *precvframe, _queue *queue);
extern int enqueue_recvframe_usb(union recv_frame *precvframe, _queue *queue);

extern void free_recvframe_queue(_queue *pframequeue,  _queue *pfree_recv_queue);  
extern void init_recvframe(union recv_frame *precvframe ,struct recv_priv *precvpriv);

extern int recv_decache(union recv_frame *precv_frame, u8 bretry, struct stainfo_rxcache *prxcache);
extern int wlanhdr_to_ethhdr ( union recv_frame *precvframe);
#ifdef LINUX_TASKLET
extern void recv_tasklet(_adapter * padapter );
extern thread_return recv_thread(void *context);

#else
extern thread_return recv_thread(void *context);
#endif

#ifdef RX_FUNCTION
extern void  recv_function(union recv_frame *prframe);
#endif

//added by jackson

static __inline u8 *get_rxmem(union recv_frame *precvframe)
{
	//always return rx_head...
	if(precvframe==NULL)
		return NULL;

	return precvframe->u.hdr.rx_head;
}

static __inline u8 *get_rx_status(union recv_frame *precvframe)
{
	
	return get_rxmem(precvframe);
	
}

static __inline u8 *get_recvframe_data(union recv_frame *precvframe)
{
	
	//alwasy return rx_data	
	if(precvframe==NULL)
		return NULL;

	return precvframe->u.hdr.rx_data;
	
}

static __inline u8 *recvframe_push(union recv_frame *precvframe, sint sz)
{	
	// append data before rx_data (by yitsen)

	/* add data to the start of recv_frame
 *
 *      This function extends the used data area of the recv_frame at the buffer
 *      start. rx_data must be still larger than rx_head, after pushing.
 */
 
	if(precvframe==NULL)
		return NULL;


	precvframe->u.hdr.rx_data -= sz ;
	if( precvframe->u.hdr.rx_data < precvframe->u.hdr.rx_head )
	{
		precvframe->u.hdr.rx_data += sz ;
		//DEBUG_ERR("\n Error :recvframe_pull: hdr.rx_Head > rx_data \n ");
		return NULL;
	}

	precvframe->u.hdr.len +=sz;

	return precvframe->u.hdr.rx_data;
	
}


static __inline u8 *recvframe_pull(union recv_frame *precvframe, sint sz)
{
	// rx_data += sz; move rx_data sz bytes  hereafter

	//used for extract sz bytes from rx_data, update rx_data and return the updated rx_data to the caller


	if(precvframe==NULL)
		return NULL;

	
	precvframe->u.hdr.rx_data += sz;

	if(precvframe->u.hdr.rx_data > precvframe->u.hdr.rx_tail)
	{
		precvframe->u.hdr.rx_data -= sz;
		return NULL;
	}

	precvframe->u.hdr.len -=sz;
	
	return precvframe->u.hdr.rx_data;
	
}

static __inline u8 *recvframe_put(union recv_frame *precvframe, sint sz)
{
	// rx_tai += sz; move rx_tail sz bytes  hereafter

	//used for append sz bytes from ptr to rx_tail, update rx_tail and return the updated rx_tail to the caller
	//after putting, rx_tail must be still larger than rx_end. 
 	unsigned char * prev_rx_tail;

	if(precvframe==NULL)
		return NULL;

	prev_rx_tail = precvframe->u.hdr.rx_tail;
	
	precvframe->u.hdr.rx_tail += sz;
	
	if(precvframe->u.hdr.rx_tail > precvframe->u.hdr.rx_end)
	{
		precvframe->u.hdr.rx_tail -= sz;
		return NULL;
	}

	precvframe->u.hdr.len +=sz;

	return precvframe->u.hdr.rx_tail;

}



static __inline u8 *recvframe_pull_tail(union recv_frame *precvframe, sint sz)
{
	// rmv data from rx_tail (by yitsen)
	
	//used for extract sz bytes from rx_end, update rx_end and return the updated rx_end to the caller
	//after pulling, rx_end must be still larger than rx_data.

	if(precvframe==NULL)
		return NULL;

	precvframe->u.hdr.rx_tail -= sz;

	if(precvframe->u.hdr.rx_tail < precvframe->u.hdr.rx_data)
	{
		precvframe->u.hdr.rx_tail += sz;
		return NULL;
	}

	precvframe->u.hdr.len -=sz;

	return precvframe->u.hdr.rx_tail;

}



static __inline _buffer * get_rxbuf_desc(union recv_frame *precvframe)
{
	_buffer * buf_desc;
	
	if(precvframe==NULL)
		return NULL;
#ifdef PLATFORM_WINDOWS	
	NdisQueryPacket(precvframe->u.hdr.pkt, NULL, NULL, &buf_desc, NULL);
#endif

	return buf_desc;
}


static __inline union recv_frame *rxmem_to_recvframe(u8 *rxmem)
{
	//due to the design of 2048 bytes alignment of recv_frame, we can reference the union recv_frame 
	//from any given member of recv_frame.
	// rxmem indicates the any member/address in recv_frame
	
	return (union recv_frame*)(((uint)rxmem>>RXFRAME_ALIGN) <<RXFRAME_ALIGN) ;
	
}

static __inline union recv_frame *pkt_to_recvframe(_pkt *pkt)
{
	
	u8 * buf_star;
	union recv_frame * precv_frame;
#ifdef PLATFORM_WINDOWS
	_buffer * buf_desc;
	uint len;

	NdisQueryPacket(pkt, NULL, NULL, &buf_desc, &len);
	NdisQueryBufferSafe(buf_desc, &buf_star, &len, HighPagePriority);
#endif
	precv_frame = rxmem_to_recvframe((unsigned char*)buf_star);

	return precv_frame;
}

static __inline u8 *pkt_to_recvmem(_pkt *pkt)
{
	// return the rx_head
	
	union recv_frame * precv_frame = pkt_to_recvframe(pkt);

	return 	precv_frame->u.hdr.rx_head;

}

static __inline u8 *pkt_to_recvdata(_pkt *pkt)
{
	// return the rx_data

	union recv_frame * precv_frame =pkt_to_recvframe(pkt);

	return 	precv_frame->u.hdr.rx_data;
	
}


static __inline sint get_recvframe_len(union recv_frame *precvframe)
{
	return precvframe->u.hdr.len;
}

static __inline struct  recv_stat *get_recv_stat(union recv_frame *precv_frame)
{
#if defined(CONFIG_RTL8187)
	return (struct recv_stat *)(precv_frame->u.hdr.rx_tail+4); //add 4 is necessary after FCS is removed.
#elif defined(CONFIG_RTL8192U)
	return (struct recv_stat *)(precv_frame->u.hdr.rx_head);
#else
	return (struct recv_stat *)(precv_frame->u.hdr.rx_head);
#endif
}

#endif
