#ifndef __USB_OSINTF_H
#define __USB_OSINTF_H


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <usb_vendor_req.h>

extern NDIS_STATUS usb_dvobj_init(_adapter * adapter);
extern void usb_dvobj_deinit(_adapter * adapter);
extern NDIS_STATUS usb_rx_init(_adapter * padapter);
extern NDIS_STATUS usb_rx_deinit(_adapter * padapter);
extern u8 usbvendorrequest(struct dvobj_priv *pdvobjpriv, RT_USB_BREQUEST brequest, RT_USB_WVALUE wvalue, u8 windex, PVOID data, u8 datalen, u8 isdirectionin);

#endif

