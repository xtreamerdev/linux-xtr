

#ifndef __XMIT_OSDEP_H_
#define __XMIT_OSDEP_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#define sCrcLng         4	
#define sAckCtsLng	112		// bits in ACK and CTS frames
#define aSifsTime	(_sys_mib->network_type == WIRELESS_11N_2G4) ? 16: 10

struct pkt_attrib	{
	u8	type;
	u8   subtype;
	u8	bswenc;
	u16	ether_type;
	//u8   m_data;
	int	pktlen;		//the original 802.3 pkt raw_data len (not include ether_hdr data)
	int	pkt_hdrlen;	//the original 802.3 pkt header len
	int	hdrlen;		//the WLAN Header Len
	//int	frag_len;	//
	int	nr_frags;
	int	last_txcmdsz;
	int	encrypt;	//when 0 indicate no encrypt. when non-zero, indicate the encrypt algorith
	unsigned char iv[8];
	int	iv_len;
	unsigned char icv[8];
	int	icv_len;
	int	priority;
	int	ack_policy;
	int	mac_id;
	int	vcs_mode;	//virtual carrier sense method
	//struct	hw_txqueue *hwqueue;
	u8 dst[ETH_ALEN];
	u8	src[ETH_ALEN];
	u8	ta[ETH_ALEN];
	u8 ra[ETH_ALEN];
#ifdef CONFIG_RTL8192U
	u8 bRTSUseShortPreamble;
	u8  bTxDisableRateFallBack;
	u8 bTxUseDriverAssingedRate;
	u8 RATRIndex;
	u8 bRTSSTBC;

	u8 bRTSUseShortGI;
	u8 bCTSEnable;
	u8 RTSSC;
	u8 bRTSBW;

	u8 bRTSEnable;
	u8 RTSRate;
	u8 bNeedAckReply;
	u8 bNeedCRCAppend;
#endif /*CONFIG_RTL8192U*/
	
};


#ifdef PLATFORM_WINDOWS


typedef	NDIS_PACKET	_pkt;
typedef NDIS_BUFFER	_buffer;


#define ETH_ALEN	6

struct pkt_file {
	_pkt *pkt;
	u32	pkt_len;	 //the remainder length of the open_file
	_buffer *cur_buffer;
	u8 *buf_start;
	u8 *cur_addr;
	u32 buf_len;
};

extern NDIS_STATUS xmit_entry(
IN _nic_hdl		cnxt,
IN NDIS_PACKET		*pkt,
IN UINT				flags
);


#endif


#ifdef PLATFORM_LINUX

struct xmit_priv;
struct sta_xmit_priv;
typedef unsigned char	_buffer;

struct pkt_file {
	_pkt *pkt;
	u32	pkt_len;	 //the remainder length of the open_file
	_buffer *cur_buffer;
	u8 *buf_start;
	u8 *cur_addr;
	u32 buf_len;
};



extern int xmit_entry(_pkt *pkt, _nic_hdl pnetdev);

#endif
extern void _open_pktfile (_pkt *pktptr, struct pkt_file *pfile);
extern uint _pktfile_read (struct pkt_file *pfile, u8 *rmem, uint rlen);
extern void set_qos(struct pkt_file *ppktfile, struct pkt_attrib *pattrib);

extern sint _init_xmit_priv(struct xmit_priv *pxmitpriv, _adapter *padapter);
extern void _free_xmit_priv (struct xmit_priv *pxmitpriv);
#endif //

