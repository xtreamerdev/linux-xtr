
#ifndef __RECV_OSDEP_H_
#define __RECV_OSDEP_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#ifdef CONFIG_RTL8187	
#define RXCMD_SZ 	16
#endif
#ifdef CONFIG_RTL8192U
#define RXCMD_SZ 	2048
#endif


extern sint _init_recv_priv(struct recv_priv *precvpriv, _adapter *padapter);
extern void _free_recv_priv (struct recv_priv *precvpriv);
extern sint update_rxstatus(_adapter *adapter, union recv_frame *precv_frame, struct recv_stat *rxstatus );
extern void handle_tkip_mic_err(_adapter *padapter,u8 bgroup);
extern union recv_frame * recvframe_chk_defrag(_adapter *adapter, union recv_frame* precv_frame);
extern union recv_frame * recvframe_defrag(_adapter *adapter, _queue *defrag_q);
extern void alloc_os_rxpkt(union recv_frame *precvframe ,struct recv_priv *precvpriv);
extern s32 recv_entry( IN _nic_hdl	cnxt);	
extern u8  recv_indicatepkt(_adapter *padapter, union recv_frame *precv_frame, _pkt *pkt);
extern void recv_returnpacket(IN _nic_hdl cnxt, IN _pkt *preturnedpkt);
extern _pkt * get_os_resource(u8 *bResult);
extern void free_os_resource(_pkt *pkt);

#endif //3
