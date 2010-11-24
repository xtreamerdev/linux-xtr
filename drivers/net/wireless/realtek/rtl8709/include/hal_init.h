

#ifndef __HAL_INIT_H__
#define __HAL_INIT_H__


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>



#define FIRMWARE_DOWNLOAD_ADDRESS	0x1FC00000
struct fw_priv{
	u32	regulatory_class;
	u32	wireless_band;
	u32	myid;
	u32	rfintfs;
	u32	chip_version;
	u32	hci_sel;
	u32	lbk_mode;
	u32	tcp_tx_frame_len;
	u32	tcp_rx_frame_len;
	u32  agc_cmdarray;       //Perry: for RF testing
	u32  ofdm_cmdarray;      //Perry: for RF testing
	u32  radio_cmdarray; //Perry: for RF testing
	u32  qos;	
	u32  rate_adaptive;
       u32  dig;
	u32	vcsType;
	u32	vcsMode;
	u32	RTSThreadshold;

#ifdef CONFIG_RTL8192U
	FIRMWARE_STATUS	FirmwareStatus;
#endif

};

struct fw_hdr {  //total size 28 bytes
	u16	signature;	// 2 bytes
	u16	version; 	// 2 bytes
	u32	fw_priv_sz;	// 4 bytes
	struct fw_priv fwpriv;	//12 bytes
	u32	imgsz; 			// (fw size)
	u32	rsvd;
};

#if defined(CONFIG_RTL8711)
extern void rtl8711_hal_bus_register(_adapter *padapter);
extern uint rtl8711_hal_init(_adapter *padapter);
extern uint rtl8711_hal_deinit(_adapter *padapter) ;
#elif defined(CONFIG_RTL8192U)
extern void rtl8192u_hal_bus_register(_adapter *padapter);
extern uint rtl8192u_hal_init(_adapter *padapter);
extern uint rtl8192u_hal_deinit(_adapter *padapter) ;
extern void rtl8256_rf_set_chan(_adapter *padapter, short ch);
extern sint _init_mibinfo(_adapter *padapter);
extern sint _free_mibinfo(_adapter *padapter);

u4Byte PHY_QueryRFReg(IN PADAPTER Adapter, IN u1Byte eRFPath, IN u4Byte RegAddr, IN u4Byte BitMask);

u8 MapHwQueueToFirmwareQueue(u8 QueueID);

RT_STATUS Hal_FWDownload(IN PADAPTER Adapter);

#elif defined(CONFIG_RTL8187)
extern void rtl8187_hal_bus_register(_adapter *padapter);
extern uint rtl8187_hal_init(_adapter *padapter);
extern uint rtl8187_hal_deinit(_adapter *padapter) ;
extern void rtl8225z2_rf_set_chan(_adapter *padapter, short ch);
extern sint _init_mibinfo(_adapter *padapter);
extern sint _free_mibinfo(_adapter *padapter);
#endif

#endif //__HAL_INIT_H__

