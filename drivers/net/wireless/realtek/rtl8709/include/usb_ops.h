#ifndef __USB_OPS_H_
#define __USB_OPS_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>


extern u8 usb_read8(struct intf_hdl *intf_hdl, u32 addr);
extern  uint usb_write_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem);

extern void usb_set_intf_option(u32 *poption);

extern void usb_set_intf_funs(struct intf_hdl *pintf_hdl);

extern uint usb_init_intf_priv(struct intf_priv *pintfpriv);

extern void usb_unload_intf_priv(struct intf_priv *pintfpriv);

extern void usb_intf_hdl_init(u8 *priv);

extern void usb_intf_hdl_unload(u8 *priv);

extern void usb_intf_hdl_open(u8 *priv);

extern void usb_intf_hdl_close(u8 *priv);

extern void usb_set_intf_ops(struct _io_ops *pops);
	
//extern void usb_set_intf_callbacks(struct _io_callbacks *pcallbacks);

void async_rd_io_callback(_adapter *padapter, struct io_req *pio_req, u8 *cnxt);

extern VOID query_rx_desc_status(_adapter	*padapter, union recv_frame *precvframe);
extern u32 GetRxPacketShiftBytes819xUsb(union recv_frame *precvframe);
extern s8 process_indicate_amsdu(	_adapter *padapter, union recv_frame *precvframe);


extern void usb_read_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem);
extern void usb_read_port_cancel(_adapter *padapter);
extern void _async_protocol_read(struct io_queue *pio_q);
extern void _async_protocol_write(struct io_queue *pio_q);

void usb_cancel_io_irp(_adapter *padapter);
uint usb_write_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem);
uint usb_write_scsi(struct intf_hdl *pintfhdl, u32 cnt, u8 *wmem);

void usb_write_port_cancel(_adapter *padapter);
#ifdef PLATFORM_WINDOWS
void io_irp_timeout_handler (	IN	PVOID	SystemSpecific1,IN	PVOID	FunctionContext,IN	PVOID	SystemSpecific2,IN	PVOID	SystemSpecific3);
#endif
#ifdef PLATFORM_LINUX
void io_irp_timeout_handler (IN	PVOID FunctionContext);
#endif
#endif

