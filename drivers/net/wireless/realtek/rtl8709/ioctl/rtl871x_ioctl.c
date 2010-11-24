/******************************************************************************
* rtl871x_ioctl.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/

#define  _RTL871X_IOCTL_C_
#include <drv_conf.h>
#include <rtl8711_spec.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <wlan_bssdef.h>
#include "wifi.h"
#include <rtl871x_ioctl.h>
#include <rtl871x_ioctl_set.h>
#include <rtl871x_ioctl_query.h>
#ifdef CONFIG_MP_INCLUDED
#include <rtl871x_mp_ioctl.h>
#endif


#define NIC8711_SUPPORTED_FILTERS	\
			NDIS_PACKET_TYPE_DIRECTED | \
			NDIS_PACKET_TYPE_MULTICAST | \
			NDIS_PACKET_TYPE_ALL_MULTICAST | \
			NDIS_PACKET_TYPE_BROADCAST | \
			NDIS_PACKET_TYPE_PROMISCUOUS

NDIS_OID NICSupportedOids[] =
{
    OID_GEN_SUPPORTED_LIST,			//Q
    OID_GEN_HARDWARE_STATUS,		//Q
    OID_GEN_MEDIA_SUPPORTED,		//Q
    OID_GEN_MEDIA_IN_USE,			//Q
    OID_GEN_MAXIMUM_LOOKAHEAD,		//Q
    OID_GEN_MAXIMUM_FRAME_SIZE,		//Q
    OID_GEN_LINK_SPEED,				//Q
    OID_GEN_TRANSMIT_BUFFER_SPACE,	//Q
    OID_GEN_RECEIVE_BUFFER_SPACE,	//Q
    OID_GEN_TRANSMIT_BLOCK_SIZE,		//Q
    OID_GEN_RECEIVE_BLOCK_SIZE,		//Q
    OID_GEN_VENDOR_ID,				//Q
    OID_GEN_VENDOR_DESCRIPTION,		//Q
    OID_GEN_VENDOR_DRIVER_VERSION,	//Q
    OID_GEN_CURRENT_PACKET_FILTER,		//S
    OID_GEN_CURRENT_LOOKAHEAD,		//Q	S
    OID_GEN_DRIVER_VERSION,			//Q
    OID_GEN_MAXIMUM_TOTAL_SIZE,		//Q
    OID_GEN_PROTOCOL_OPTIONS,			//S
    OID_GEN_MAC_OPTIONS,				//Q
    OID_GEN_MEDIA_CONNECT_STATUS,	//Q
    OID_GEN_MAXIMUM_SEND_PACKETS,	//Q
    OID_GEN_XMIT_OK,					//Q
    OID_GEN_RCV_OK,					//Q
    OID_GEN_XMIT_ERROR,				//Q
    OID_GEN_RCV_ERROR,				//Q
    OID_GEN_RCV_NO_BUFFER,			//Q
//    OID_GEN_RCV_CRC_ERROR,			//Optional
//    OID_GEN_TRANSMIT_QUEUE_LENGTH,	//Optional
    OID_GEN_PHYSICAL_MEDIUM,			//Q
//    OID_GEN_NETWORK_LAYER_ADDRESSES,	//Optional
    OID_802_3_PERMANENT_ADDRESS,	//Q
    OID_802_3_CURRENT_ADDRESS,		//Q
    OID_802_3_MULTICAST_LIST,				//S
    OID_802_3_MAXIMUM_LIST_SIZE,		//Q
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_802_3_XMIT_DEFERRED,
    OID_802_3_XMIT_MAX_COLLISIONS,
    OID_802_3_RCV_OVERRUN,
    OID_802_3_XMIT_UNDERRUN,
    OID_802_3_XMIT_HEARTBEAT_FAILURE,
    OID_802_3_XMIT_TIMES_CRS_LOST,
    OID_802_3_XMIT_LATE_COLLISIONS,


	//mandatory (M), recommended (R), or optional (O)
	//query (Q) - set (S) - indication (I) -2kMe - XP - WPA - WPA2
	OID_802_11_BSSID,	//Q-S- -M-M-M-M
	OID_802_11_SSID,	//Q-S- -M-M-M-M
	OID_802_11_NETWORK_TYPES_SUPPORTED,	//Q- - -R-R-M-M
	OID_802_11_NETWORK_TYPE_IN_USE,		//Q-S- -O-M-M-M
//Optional	OID_802_11_TX_POWER_LEVEL,		//Q-S- -O-O-O-O
	OID_802_11_RSSI,				//Q- -I-O-M-M-M
//Optional	OID_802_11_RSSI_TRIGGER,		//Q-S- -O-O-O-O
	OID_802_11_INFRASTRUCTURE_MODE,
//Optional	OID_802_11_FRAGMENTATION_THRESHOLD,
//Optional	OID_802_11_RTS_THRESHOLD,
//Optional	OID_802_11_NUMBER_OF_ANTENNAS,
//Optional	OID_802_11_RX_ANTENNA_SELECTED,
//Optional	OID_802_11_TX_ANTENNA_SELECTED,
	OID_802_11_SUPPORTED_RATES,
//Optional	OID_802_11_DESIRED_RATES,
	OID_802_11_CONFIGURATION,
	OID_802_11_STATISTICS,
	OID_802_11_ADD_WEP,
	OID_802_11_REMOVE_WEP,
	OID_802_11_WEP_STATUS,
	OID_802_11_DISASSOCIATE,
	OID_802_11_POWER_MODE,
	OID_802_11_BSSID_LIST,
	OID_802_11_AUTHENTICATION_MODE,
//Optional	OID_802_11_PRIVACY_FILTER,
	OID_802_11_BSSID_LIST_SCAN,
	OID_802_11_RELOAD_DEFAULTS,
	OID_802_11_ADD_KEY,
	OID_802_11_REMOVE_KEY,
	OID_802_11_ENCRYPTION_STATUS,
	OID_802_11_ASSOCIATION_INFORMATION,
	OID_802_11_TEST,
	OID_802_11_CAPABILITY,
	OID_802_11_PMKID
};


struct oid_obj_priv oid_segment_0[] =
{
	{1, &oid_null_function},  						//0x00
	{1, &oid_gen_supported_list_hdl}, 			//0x01
	{1, &oid_gen_hardware_status_hdl}, 			//0x02
	{1, &oid_gen_media_supported_hdl},			//0x03
	{1, &oid_gen_media_in_use_hdl}, 			//0x04
	{1, &oid_gen_maximum_lookahead_hdl}, 		//0x05
	{1, &oid_gen_maximum_frame_size_hdl}, 		//0x06
	{1, &oid_gen_link_speed_hdl}, 				//0x07
	{1, &oid_gen_transmit_buffer_space_hdl}, 	//0x08
	{1, &oid_gen_receive_buffer_space_hdl}, 		//0x09
	{1, &oid_gen_transmit_block_size_hdl}, 		//0x0A
	{1, &oid_gen_receive_block_size_hdl}, 		//0x0B
	{1, &oid_gen_vendor_id_hdl}, 				//0x0C
	{1, &oid_gen_vendor_description_hdl}, 		//0x0D
	{1, &oid_gen_current_packet_filter_hdl}, 		//0x0E
	{1, &oid_gen_current_lookahead_hdl}, 		//0x0F
	{1, &oid_gen_driver_version_hdl}, 			//0x10
	{1, &oid_gen_maximum_total_size_hdl},		//0x11
	{1, &oid_gen_protocol_options_hdl}, 			//0x12
	{1, &oid_gen_mac_options_hdl}, 				//0x13
	{1, &oid_gen_media_connect_status_hdl}, 	//0x14
	{1, &oid_gen_maximum_send_packets_hdl}, 	//0x15
	{1, &oid_gen_vendor_driver_version_hdl}		//0x16
};

struct oid_obj_priv oid_segment_1[] =
{
	{1, &oid_null_function},						//0X00
	{1, &oid_null_function},						//0X01
	{1, &oid_gen_physical_medium_hdl}			//0X02
};

struct oid_obj_priv oid_segment_2[] =
{
	{1, &oid_null_function},						//0x00
	{1, &oid_gen_xmit_ok_hdl},					//0x01
	{1, &oid_gen_rcv_ok_hdl},					//0x02
	{1, &oid_gen_xmit_error_hdl},				//0x03
	{1, &oid_gen_rcv_error_hdl},					//0x04
	{1, &oid_gen_rcv_no_buffer_hdl}				//0x05
};

struct oid_obj_priv oid_segment_3[] =
{
	{1, &oid_null_function},						//0x00
	{1, &oid_802_3_permanent_address_hdl},		//0x01
	{1, &oid_802_3_current_address_hdl},		//0x02
	{1, &oid_802_3_multicast_list_hdl},			//0x03
	{1, &oid_802_3_maximum_list_size_hdl},		//0x04
	{1, &oid_802_3_mac_options_hdl}			//0x05
};

struct oid_obj_priv oid_segment_4[] =
{
	{1, &oid_null_function},						//0x00
	{1, &oid_802_3_rcv_error_alignment_hdl},	//0x01
	{1, &oid_802_3_xmit_one_collision_hdl},		//0x02
	{1, &oid_802_3_xmit_more_collisions_hdl}	//0x03
};

struct oid_obj_priv oid_segment_5[] =
{
	{1, &oid_null_function},						//0x00
	{1, &oid_802_3_xmit_deferred_hdl},			//0x01
	{1, &oid_802_3_xmit_max_collisions_hdl},		//0x02
	{1, &oid_802_3_rcv_overrun_hdl},			//0x03
	{1, &oid_802_3_xmit_underrun_hdl},			//0x04
	{1, &oid_802_3_xmit_heartbeat_failure_hdl},	//0x05
	{1, &oid_802_3_xmit_times_crs_lost_hdl},		//0X06
	{1, &oid_802_3_xmit_late_collisions_hdl}		//0X07
};

struct oid_obj_priv oid_segment_6[] =
{
	{1, &oid_pnp_capabilities_hdl},				//0x00
	{1, &oid_pnp_set_power_hdl},				//0x01
	{1, &oid_pnp_query_power_hdl},				//0x02
	{1, &oid_pnp_add_wake_up_pattern_hdl},		//0x03
	{1, &oid_pnp_remove_wake_up_pattern_hdl},	//0x04
	{1, &oid_pnp_wake_up_pattern_list_hdl},		//0x05
	{1, &oid_pnp_enable_wake_up_hdl}			//0X06
};

struct oid_obj_priv oid_segment_7[] =
{
	{1, &oid_null_function},						//0x00
	{1, &oid_802_11_bssid_hdl},					//0x01
	{1, &oid_802_11_ssid_hdl},					//0x02
	{1, &oid_null_function},						//0x03
	{1, &oid_null_function},						//0x04
	{1, &oid_null_function},						//0x05
	{1, &oid_null_function},						//0x06
	{1, &oid_null_function},						//0x07
	{1, &oid_802_11_infrastructure_mode_hdl},	//0x08
	{1, &oid_null_function},						//0x09
	{1, &oid_null_function},						//0x0A
	{1, &oid_null_function},						//0x0B
	{1, &oid_null_function},						//0x0C
	{1, &oid_null_function},						//0x0D
	{1, &oid_null_function},						//0x0E
	{1, &oid_null_function},						//0x0F
	{1, &oid_null_function},						//0x10
	{1, &oid_null_function},						//0x11
	{1, &oid_null_function},						//0x12
	{1, &oid_802_11_add_wep_hdl},				//0x13
	{1, &oid_802_11_remove_wep_hdl},			//0x14
	{1, &oid_802_11_disassociate_hdl},			//0x15
	{1, &oid_null_function},						//0x16
	{1, &oid_null_function},						//0x17
	{1, &oid_802_11_authentication_mode_hdl},	//0x18
	{1, &oid_802_11_privacy_filter_hdl},			//0x19
	{1, &oid_802_11_bssid_list_scan_hdl},		//0x1A
	{1, &oid_802_11_encryption_status_hdl},		//0x1B
	{1, &oid_802_11_reload_defaults_hdl},		//0x1C
	{1, &oid_802_11_add_key_hdl},				//0x1D
	{1, &oid_802_11_remove_key_hdl},			//0x1E
	{1, &oid_802_11_association_information_hdl},//0x1F
	{1, &oid_802_11_test_hdl},					//0x20
	{1, &oid_802_11_media_stream_mode_hdl},	//0x21
	{1, &oid_802_11_capability_hdl},				//0x22
	{1, &oid_802_11_pmkid_hdl}					//0x23
};

struct oid_obj_priv oid_segment_8[] =
{
	{1, &oid_null_function},							//0x00
	{1, &oid_null_function},							//0x01
	{1, &oid_null_function},							//0x02
	{1, &oid_802_11_network_types_supported_hdl},	//0x03
	{1, &oid_802_11_network_type_in_use_hdl},		//0x04
	{1, &oid_802_11_tx_power_level_hdl},			//0x05
	{1, &oid_802_11_rssi_hdl},						//0x06
	{1, &oid_802_11_rssi_trigger_hdl},				//0x07
	{1, &oid_null_function},							//0x08
	{1, &oid_802_11_fragmentation_threshold_hdl},	//0x09
	{1, &oid_802_11_rts_threshold_hdl},				//0x0A
	{1, &oid_802_11_number_of_antennas_hdl},		//0x0B
	{1, &oid_802_11_rx_antenna_selected_hdl},		//0x0C
	{1, &oid_802_11_tx_antenna_selected_hdl},		//0x0D
	{1, &oid_802_11_supported_rates_hdl},			//0x0E
	{1, &oid_null_function},							//0x0F
	{1, &oid_802_11_desired_rates_hdl},				//0x10
	{1, &oid_802_11_configuration_hdl},				//0x11
	{1, &oid_null_function},							//0x12
	{1, &oid_null_function},							//0x13
	{1, &oid_null_function},							//0x14
	{1, &oid_null_function},							//0x15
	{1, &oid_802_11_power_mode_hdl},				//0X16		
	{1, &oid_802_11_bssid_list_hdl}					//0X17		
};

struct oid_obj_priv oid_segment_9[] =
{
	{1, &oid_null_function},							//0x00
	{1, &oid_null_function},							//0x01
	{1, &oid_null_function},							//0x02
	{1, &oid_null_function},							//0x03
	{1, &oid_null_function},							//0x04
	{1, &oid_null_function},							//0x05
	{1, &oid_null_function},							//0x06
	{1, &oid_null_function},							//0x07
	{1, &oid_null_function},							//0x08
	{1, &oid_null_function},							//0x09
	{1, &oid_null_function},							//0x0A
	{1, &oid_null_function},							//0x0B
	{1, &oid_null_function},							//0x0C
	{1, &oid_null_function},							//0x0D
	{1, &oid_null_function},							//0x0E
	{1, &oid_null_function},							//0x0F
	{1, &oid_null_function},							//0x10
	{1, &oid_null_function},							//0x11
	{1, &oid_802_11_statistics_hdl}					//0X12	
};


struct oid_funs_node oid_funs_array[] = 
{
	{0x00010100, 0x000101FF, oid_segment_0, sizeof(oid_segment_0), 0, 0},
	{0x00010200, 0x000102FF, oid_segment_1, sizeof(oid_segment_1), 0, 0},
	{0x00020100, 0x000201FF, oid_segment_2, sizeof(oid_segment_2), 0, 0},
	{0x01010100, 0x010101FF, oid_segment_3, sizeof(oid_segment_3), 0, 0},
	{0x01020100, 0x010201FF, oid_segment_4, sizeof(oid_segment_4), 0, 0},
	{0x01020200, 0x010202FF, oid_segment_5, sizeof(oid_segment_5), 0, 0},
	{0xFD010100, 0xFD0101FF, oid_segment_6, sizeof(oid_segment_6), 0, 0},
	{0x0D010100, 0x0D0101FF, oid_segment_7, sizeof(oid_segment_7), 0, 0},
	{0x0D010200, 0x0D0102FF, oid_segment_8, sizeof(oid_segment_8), 0, 0},
	{0x0D020200, 0x0D0202FF, oid_segment_9, sizeof(oid_segment_9), 0, 0}
};
//u8 DescString8711_SDIO[]="Realtek RTL8711 Wireless LAN SDIO NIC                                     ";
#ifdef CONFIG_SDIO_HCI
u8 desc_string8711[]="Realtek RTL8711 Wireless LAN SDIO NIC                                     ";
#elif CONFIG_USB_HCI
u8 desc_string8711[]="Realtek RTL8711 Wireless LAN USB NIC                                      ";
#elif CONFIG_CFIO_HCI
u8 desc_string8711[]="Realtek RTL8711 Wireless LAN CFIO NIC                                     ";
#endif



NDIS_STATUS
oid_null_function(	struct oid_par_priv* poid_par_priv)
{
	_func_enter_;
	_func_exit_;
	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS copy_oid_info(struct oid_par_priv* poid_par_priv, PVOID pinfo, u32 info_len)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	_func_enter_;
	if(info_len <= poid_par_priv->information_buf_len)
	{
		// Copy result into InformationBuffer
		if(info_len)
		{
			*poid_par_priv->bytes_rw = info_len;
			_memcpy(poid_par_priv->information_buf, pinfo, info_len);
		}
	}
	else
	{
		// Buffer too short			
		*poid_par_priv->bytes_needed = info_len;
		status = NDIS_STATUS_BUFFER_TOO_SHORT;
	}
	_func_exit_;
	return status;
}






NDIS_STATUS FillQueryInformation(IN PVOID src, IN u32 srcLength, OUT PVOID dest, IN u32 destLength, OUT u32 *BytesWritten, u32 *BytesNeeded){
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	_func_enter_;
	if (src == NULL || dest == NULL){
		_func_exit_;
		return NDIS_STATUS_NOT_SUPPORTED;
	}

	if (srcLength <= destLength) {
		
		if (destLength > 0){

			*BytesWritten = srcLength;
			_memcpy(dest, src, srcLength);

		} else {

			*BytesWritten = 0;
		}

	} else {

		// dest buffer is too short
		*BytesNeeded = srcLength;
		*BytesWritten = 0;
		_func_exit_;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}
	_func_exit_;
	return NDIS_STATUS_SUCCESS;
}



//1Query Functions for Segment 1


NDIS_STATUS
query_oid_gen_supported_list(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	//unsigned db = poid_par_priv->dbg;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	pInfo = (PVOID) NICSupportedOids;
	ulInfoLen = sizeof(NICSupportedOids);
	
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}





NDIS_STATUS
query_oid_gen_hardware_status(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u8			bInformationCopied = _FALSE;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	
	ulInfo = NdisHardwareStatusReady;
	DEBUG_INFO(("Query Information, OID_GEN_HARDWARE_STATUS: 0x%08X\n", ulInfo));

       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_media_supported(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = NdisMedium802_3;
	DEBUG_ERR(("\nQuery Information, OID_GEN_MEDIA_SUPPORTED: %08x\n", ulInfo));

       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_media_in_use(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = NdisMedium802_3;
	DEBUG_ERR( ("\nQuery Information, OID_GEN_MEDIA_IN_USE: %08x\n", ulInfo));

       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}



NDIS_STATUS
query_oid_gen_maximum_lookahead(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = NIC_MAX_PACKET_SIZE - NIC_HEADER_SIZE;
			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_maximum_frame_size(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = NIC_MAX_PACKET_SIZE - NIC_HEADER_SIZE;
			
	 if(Adapter->registrypriv.mp_mode == 1 )
       {
		ulInfo = (u32) (ulInfo + 16);
       }
	else
	{
		ulInfo = (u32) ulInfo;
	}
		
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	 _func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_link_speed_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);

	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = 250000;
#ifdef CONFIG_USB_HCI
	ulInfo = 540000;
#endif

       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_transmit_buffer_space_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;
	// The OID_GEN_TRANSMIT_BUFFER_SPACE OID specifies the amount of memory, in bytes, on the NIC that is available for 
	// buffering transmit data. A protocol can use this OID as a guide for sizing the amount of transmit data per send.
	ulInfo = NIC_MAX_PACKET_SIZE * 40;//DDK: * Adapter->NumTcb;
	//Status = FillQueryInformation((PVOID)&ulInfo, 4, InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	//return Status;

			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}






NDIS_STATUS
query_oid_gen_receive_buffer_space_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;


	// The OID_GEN_RECEIVE_BUFFER_SPACE OID specifies the amount of memory on the NIC that is available for buffering receive data.
	// A protocol driver can use this OID as a guide for advertising its receive window after it establishes sessions with remote nodes.
	ulInfo = NIC_MAX_PACKET_SIZE * 10;//DDK: * Adapter->CurrNumRfd;
	//Status = FillQueryInformation((PVOID)&ulInfo, 4, InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	//return Status;

			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_transmit_block_size(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = (u32) NIC_MAX_PACKET_SIZE;
			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_receive_block_size(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = (u32) NIC_MAX_PACKET_SIZE;
			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_vendor_id_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_adapter *			padapter = (_adapter*)(poid_par_priv->adapter_context);
	
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;


	// The OID_GEN_VENDOR_ID OID specifies a three-byte IEEE-registered vendor code, followed by a single byte 
	// that the vendor assigns to identify a particular NIC.
		
	//NdisMoveMemory(Adapter->CurrentAddress, pHalData->EEPROMMACAddress, 6);//marked by yt; 20061107
	
	_memcpy(&ulInfo, padapter->eeprompriv.mac_addr, 3);
	//Status = FillQueryInformation((PVOID)&ulInfo, 4, InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	//return Status;
			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}





NDIS_STATUS
query_oid_gen_vendor_description_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;


	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;
	// The OID_GEN_VENDOR_DESCRIPTION OID points to a null-terminated string describing the NIC.
	//Status = FillQueryInformation((PVOID)&VendorDesc, sizeof(VendorDesc), InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	//return Status;
	pInfo = (PVOID)desc_string8711;
	ulInfoLen = strlen((const s8*)desc_string8711)+1;
	DEBUG_ERR( ("\nQuery Information, OID_GEN_VENDOR_DESCRIPTION:%s\n",(char *)pInfo));


			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}

NDIS_STATUS
query_oid_gen_current_packet_filter(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	// The OID_GEN_CURRENT_PACKET_FILTER OID specifies the types of net packets for which a protocol receives indications 
	// from a NIC driver. This NIC driver reports its medium type as one for which the system provides a filter library. 

	// NDIS_PACKET_TYPE_ALL_MULTICAST 
	//		All multicast address packets, not just the ones enumerated in the multicast address list. 

	// NDIS_PACKET_TYPE_PROMISCUOUS 
	//		Specifies all packets. 

	ulInfo = Adapter->NdisPacketFilter;

	//DEBUG_ERR( ("Query Information, OID_GEN_CURRENT_PACKET_FILTER:%d \n", ulInfo));
	//Status = FillQueryInformation((PVOID)&ulInfo, 4, InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	//return Status;
			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}



NDIS_STATUS
query_oid_gen_current_lookahead(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = NIC_MAX_PACKET_SIZE - NIC_HEADER_SIZE;
			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_driver_version_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u16		usInfo = 0;
	u32			usInfoLen = sizeof(usInfo);
	PVOID				pInfo = (PVOID) &usInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	usInfo = (USHORT) ((NDIS_MAJOR_VERSION << 8) + NDIS_MINOR_VERSION) ;
	DEBUG_ERR( ("\nQuery Information, OID_GEN_DRIVER_VERSION:%08x\n",usInfo));  
			
       status = copy_oid_info(poid_par_priv, pInfo, usInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_maximum_total_size(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = (u32) NIC_MAX_PACKET_SIZE;
			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	   _func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_mac_options(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo =	NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
#if COALESCE_RECEIVED_PACKET
					NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | 
#endif
					NDIS_MAC_OPTION_NO_LOOPBACK;

			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




NDIS_STATUS
query_oid_gen_media_connect_status(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	struct mlme_priv* 	pmlmepriv = &Adapter->mlmepriv;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	ulInfo = (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)?  
				NdisMediaStateConnected : NdisMediaStateDisconnected;

	if(Adapter->registrypriv.mp_mode == 1 )
		ulInfo = NdisMediaStateConnected;


	//Status = FillQueryInformation((PVOID)&ulInfo, 4, InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}


NDIS_STATUS
query_oid_gen_maximum_send_packets(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;


	// For Deserialized Miniport Drivers
	// NDIS ignores any value returned by a deserialized driver in response to a query of OID_GEN_MAXIMUM_SEND_PACKETS. 
	// NDIS does not adjust the size of the array of packet descriptors that it supplies to a deserialized miniport's 
	// MiniportSendPackets function.

	// If the underlying driver has only a MiniportSend function, it should return one for this query. 
	// In this case, NDIS will always pass one packet to the array at PacketArray.

	ulInfo = 1;
	//Status = FillQueryInformation((PVOID)&ulInfo, 1, InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	DEBUG_ERR( ("\nQuery Information, OID_GEN_MAXIMUM_SEND_PACKETS:%d\n",ulInfo));                        
	//return Status;
			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}


NDIS_STATUS
query_oid_gen_vendor_driver_version(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	// The OID_GEN_VENDOR_DRIVEN_VERSION OID specifies the vendor-assigned version number of the NIC driver. 
	// The low-order half of the return value specifies the minor version; the high-order half specifies the major version. 
	ulInfo = NIC_VENDOR_DRIVER_VERSION;
	DEBUG_ERR( ("Query Information, OID_GEN_VENDOR_DRIVER_VERSION:%08x\n",ulInfo));
	//Status = FillQueryInformation((PVOID)&ulInfo, 4, InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	//return Status;
	
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}




//1Query Functions for Segment 2

NDIS_STATUS query_oid_gen_physical_medium(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	DEBUG_ERR(("Query OID_GEN_PHYSICAL_MEDIUM\n"));

       if(Adapter->registrypriv.mp_mode == 1 )
       {
		ulInfo = NdisPhysicalMediumUnspecified;
		DEBUG_ERR( ("Query Information, OID_GEN_PHYSICAL_MEDIUM:%d(NdisPhysicalMediumUnspecified)\n", ulInfo));
       }
	else
	{
		ulInfo = NdisPhysicalMediumWirelessLan;
		DEBUG_ERR( ("Query Information, OID_GEN_PHYSICAL_MEDIUM:%d(NdisPhysicalMediumWirelessLan)\n", ulInfo));
	}

			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}



//1Query Functions for Segment 3

NDIS_STATUS query_oid_gen_xmit_ok(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;

	// The OID_GEN_XMIT_OK OID specifies the number of frames that are transmitted without errors.
	if(poid_par_priv->information_buf_len >= 8) {

		ulInfoLen = sizeof(ul64Info);

	} else if(poid_par_priv->information_buf_len == 4) {

		ulInfoLen = 4;

	} else {

		return NDIS_STATUS_BUFFER_TOO_SHORT;

	}

	status = FillQueryInformation((PVOID)&Adapter->xmitpriv.tx_pkts, ulInfoLen, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;
	
}


NDIS_STATUS query_oid_gen_rcv_ok(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;



	_func_enter_;
	// The OID_GEN_RCV_OK OID specifies the number of frames that the NIC receives without errors and indicates to bound protocols.
	if(poid_par_priv->information_buf_len >= 8) {

		ulInfoLen = sizeof(ul64Info);

	} else if(poid_par_priv->information_buf_len == 4) {

		ulInfoLen = 4;

	} else {
		_func_exit_;
		return NDIS_STATUS_BUFFER_TOO_SHORT;

	}
	
	status = FillQueryInformation((PVOID)&Adapter->recvpriv.rx_pkts, ulInfoLen, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;
	
}

NDIS_STATUS query_oid_gen_xmit_error(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;


	// The OID_GEN_XMIT_ERROR OID specifies the number of frames that a NIC fails to transmit.
	if(poid_par_priv->information_buf_len>= 8) {

		ulInfoLen = sizeof(ul64Info);

	} else if(poid_par_priv->information_buf_len== 4)	{

		ulInfoLen = 4;

	} else {
		_func_exit_;
		return NDIS_STATUS_BUFFER_TOO_SHORT;

	}
	
	status = FillQueryInformation((PVOID)&Adapter->xmitpriv.tx_drop, ulInfoLen, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;
	
}


NDIS_STATUS query_oid_gen_rcv_error(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;





	// The OID_GEN_RCV_ERROR OID specifies the number of frames that a NIC receives but does not indicate to the protocols due to errors.
	if(poid_par_priv->information_buf_len>= 8) {

		ulInfoLen = sizeof(ul64Info);

	} else if(poid_par_priv->information_buf_len== 4)	{

		ulInfoLen = 4;

	} else {
		_func_exit_;
		return NDIS_STATUS_BUFFER_TOO_SHORT;

	}
	
	status = FillQueryInformation((PVOID)&Adapter->recvpriv.rx_drop, ulInfoLen, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;
	
}



NDIS_STATUS query_oid_gen_rcv_no_buffer(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	// The OID_GEN_RCV_ERROR OID specifies the number of frames that a NIC receives but does not indicate to the protocols due to errors.
	if(poid_par_priv->information_buf_len>= 8) {

		ulInfoLen = sizeof(ul64Info);

	} else if(poid_par_priv->information_buf_len== 4)	{

		ulInfoLen = 4;

	} else {
		_func_exit_;
		return NDIS_STATUS_BUFFER_TOO_SHORT;

	}
	
	//status = FillQueryInformation((PVOID)&Adapter->recvpriv.rx_no_buffer, ulInfoLen, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;
	
}




//1Query Functions for Segment 4

NDIS_STATUS query_oid_802_3_permanent_address(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);


	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;
	//RTDumpHex("Query Information, OID_802_3_PERMANENT_ADDRESS", Adapter->PermanentAddress, 6, ':');
	//Status = FillQueryInformation((PVOID)Adapter->PermanentAddress, ETH_LENGTH_OF_ADDRESS, InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	//return Status;

	//NdisMoveMemory(Adapter->PermanentAddress, SDIO_HWADDR, 6);
//	NdisMoveMemory(Adapter->eeprompriv.mac_addr, Adapter->eeprompriv.mac_addr, 6);
	
	pInfo = Adapter->eeprompriv.mac_addr;
	ulInfoLen = ETH_LENGTH_OF_ADDRESS;
	DEBUG_ERR(("Query Information, OID_802_3_PERMANENT_ADDRESS:%02x:%02x:%02x:%02x:%02x:%02x\n", 
		Adapter->eeprompriv.mac_addr[0], Adapter->eeprompriv.mac_addr[1], Adapter->eeprompriv.mac_addr[2], 
		Adapter->eeprompriv.mac_addr[3],Adapter->eeprompriv.mac_addr[4], Adapter->eeprompriv.mac_addr[5]));

			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}

NDIS_STATUS query_oid_802_3_current_address(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);


	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	//NdisMoveMemory(Adapter->CurrentAddress, pHalData->EEPROMMACAddress, 6);
	
	pInfo = Adapter->eeprompriv.mac_addr;
	ulInfoLen = ETH_LENGTH_OF_ADDRESS;
	//DEBUG_ERR( ("Query Information, OID_802_3_CURRENT_ADDRESS:%02x:%02x:%02x:%02x:%02x:%02x\n", Adapter->CurrentAddress[0], Adapter->CurrentAddress[1], Adapter->CurrentAddress[2], Adapter->CurrentAddress[3], Adapter->CurrentAddress[4], Adapter->CurrentAddress[5]));            

       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}
	
NDIS_STATUS query_oid_802_3_multicast_list(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);

	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;


	DEBUG_ERR( ("Query OID_802_3_MULTICAST_LIST:\n"));
	{
		u32 MCListSize = Adapter->MCAddrCount*6;
		if( poid_par_priv->information_buf_len < MCListSize )
		{
			*poid_par_priv->bytes_needed = MCListSize;
			*poid_par_priv->bytes_rw = 0;
			_func_exit_;
			return NDIS_STATUS_BUFFER_TOO_SHORT;
		}

		_memcpy( poid_par_priv->information_buf, Adapter->MCList, MCListSize );
		*poid_par_priv->bytes_rw = MCListSize;
	}
	_func_exit_;
	return (status);
	
}
		
NDIS_STATUS query_oid_802_3_maximum_list_size(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);

	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;


	// The maximum number of multicast addresses the NIC driver can manage. 
	ulInfo = MAX_MCAST_LIST_NUM;
	//Status = FillQueryInformation((PVOID)&ulInfo, 4, InformationBuffer, InformationBufferLength, BytesWritten, BytesNeeded);
	//return Status;

       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
}





//1Query Functions for Segment 5

NDIS_STATUS query_oid_802_3_rcv_error_alignment(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	if(poid_par_priv->information_buf_len>= 8) {

		ulInfoLen = sizeof(ul64Info);

	} else if(poid_par_priv->information_buf_len== 4)	{

		ulInfoLen = 4;

	} else {
		_func_exit_;
		return NDIS_STATUS_BUFFER_TOO_SHORT;

	}			
	
	status = FillQueryInformation((PVOID)&ul64Info, ulInfoLen, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;
	
}

NDIS_STATUS query_oid_802_3_xmit_one_collision(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	if(poid_par_priv->information_buf_len>= 8) {

		ulInfoLen = sizeof(ul64Info);

	} else if(poid_par_priv->information_buf_len== 4)	{

		ulInfoLen = 4;

	} else {
		_func_exit_;
		return NDIS_STATUS_BUFFER_TOO_SHORT;

	}			
	
	status = FillQueryInformation((PVOID)&ul64Info, ulInfoLen, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;
	
}
	
NDIS_STATUS query_oid_802_3_xmit_more_collisions(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	if(poid_par_priv->information_buf_len>= 8) {

		ulInfoLen = sizeof(ul64Info);

	} else if(poid_par_priv->information_buf_len== 4)	{

		ulInfoLen = 4;

	} else {
		_func_exit_;
		return NDIS_STATUS_BUFFER_TOO_SHORT;

	}			
	
	status = FillQueryInformation((PVOID)&ul64Info, ulInfoLen, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;
	
}




//1Query Functions for Segment 6
//1Query Functions for Segment 7

NDIS_STATUS
query_oid_pnp_query_power(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	
	ulInfo = (ULONG)(NdisDeviceStateD3);	
	
	status = FillQueryInformation((PVOID)&ulInfo, ulInfoLen, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;
	
}


//1Query Functions for Segment 8

NDIS_STATUS query_oid_802_11_bssid(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	// When queried, this OID requests that the miniport driver return the MAC address of the associated AP. 
	// If the device is operating in ad hoc mode, the miniport driver returns the IBSS MAC address. 
	// The miniport driver must return an error code of NDIS_STATUS_ADAPTER_NOT_READY if the device is neither operating in ad hoc mode 
	// nor associated with an AP.

	{
		struct	mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
		NDIS_WLAN_BSSID_EX  *pcur_bss = &pmlmepriv->cur_network.network;
		u8 zero_string[ETH_ALEN] = {0x0,0x0,0x0,0x0,0x0,0x0};
	
		//if(pmlmepriv->fw_state & _FW_LINKED)  //jackson 0827
		if  ( ((check_fwstate(pmlmepriv, _FW_LINKED)) == _TRUE) || 
			((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)) == _TRUE) ||
			((check_fwstate(pmlmepriv, WIFI_AP_STATE)) == _TRUE) )
		{
			status = FillQueryInformation((PVOID)&(pcur_bss->MacAddress), 
										sizeof(NDIS_802_11_MAC_ADDRESS), 
										poid_par_priv->information_buf, 
										poid_par_priv->information_buf_len, 
										poid_par_priv->bytes_rw, 
										poid_par_priv->bytes_needed);
			_func_exit_;
			return status;

		} else {

			DEBUG_ERR( ("Query OID_802_11_BSSID: NDIS_STATUS_ADAPTER_NOT_READY\n"));
			status = FillQueryInformation((PVOID)zero_string, 
										sizeof(NDIS_802_11_MAC_ADDRESS), 
										poid_par_priv->information_buf, 
										poid_par_priv->information_buf_len, 
										poid_par_priv->bytes_rw, 
										poid_par_priv->bytes_needed);
			_func_exit_;
			return NDIS_STATUS_ADAPTER_NOT_READY;
		}


	
	}


	
}

NDIS_STATUS query_oid_802_11_ssid(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	// When queried, this OID requests that the miniport driver return the SSID with which the device is associated. 
	// The miniport driver returns zero for the SsidLength member if the device is not associated with an SSID.

	{
		struct	mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
		NDIS_WLAN_BSSID_EX  *pcur_bss = &pmlmepriv->cur_network.network;

	

		if ( (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) ||
		      (check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))

		{
			status = FillQueryInformation((PVOID)&(pcur_bss->Ssid), 
										sizeof( pcur_bss->Ssid.SsidLength) + pcur_bss->Ssid.SsidLength, 
										poid_par_priv->information_buf, 
										poid_par_priv->information_buf_len, 
										poid_par_priv->bytes_rw, 
										poid_par_priv->bytes_needed);



		} else {
			NDIS_802_11_SSID zero_ssid;	
			pcur_bss->Ssid.SsidLength = 0;

			memset(&zero_ssid, 0, sizeof(NDIS_802_11_SSID));
			
			status = FillQueryInformation((PVOID)&zero_ssid,
										FIELD_OFFSET(NDIS_802_11_SSID, Ssid),
										poid_par_priv->information_buf, 
										poid_par_priv->information_buf_len, 
										poid_par_priv->bytes_rw, 
										poid_par_priv->bytes_needed);
		}
		_func_exit_;
		return NDIS_STATUS_SUCCESS;  //0827

	
	}

	
}

NDIS_STATUS query_oid_802_11_infrastructure_mode(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;


	_func_enter_;
	// When queried, this OID requests that the miniport driver return its current network mode setting. 
	// If queried without a prior setting of this OID, the driver must return NDIS_STATUS_ADAPTER_NOT_READY.


	{
		struct	mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
		NDIS_802_11_NETWORK_INFRASTRUCTURE network_mode;
			

		if (check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE)
		{
			network_mode = Ndis802_11Infrastructure ;	
		}
		else if  ((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
			       (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE))
			
		{
			network_mode = Ndis802_11IBSS;		
		}
		else
		{
			network_mode = Ndis802_11AutoUnknown ;
		}
			
		status = FillQueryInformation((PVOID)&network_mode, 
									sizeof(NDIS_802_11_NETWORK_INFRASTRUCTURE),
									poid_par_priv->information_buf, 
									poid_par_priv->information_buf_len, 
									poid_par_priv->bytes_rw, 
									poid_par_priv->bytes_needed);
		_func_exit_;
		return status;
	}
	


	
	
	
}

NDIS_STATUS query_oid_802_11_authentication_mode(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;
	// When queried, this OID requests that the miniport driver return its IEEE 802.11 authentication mode.
	{
		NDIS_802_11_AUTHENTICATION_MODE	authmode;
		authmode=Adapter->securitypriv.ndisauthtype;
		//MgntActQuery_802_11_AUTHENTICATION_MODE( Adapter, &authmode );
		ulInfo = authmode;
		DEBUG_ERR( ("Query OID_802_11_AUTHENTICATION_MODE: %d\n",authmode));
	}
	
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
	
}

NDIS_STATUS query_oid_802_11_encryption_status(struct oid_par_priv* poid_par_priv)
{
	
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;
// ----------MgntInfo----------------------	
//	PMGNT_INFO     		pMgntInfo;
//	pMgntInfo = &Adapter->MgntInfo;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	// When queried, this OID requests that the miniport driver return its current encryption mode. 
	// In response, the miniport driver can indicate which encryption mode is enabled or disabled, that the transmit key is absent, 
	// or that encryption is not supported.

	// The data passed in a query or set of this OID is the NDIS_802_11_ENCRYPTION_STATUS enumeration, 
	// which defines the following encryption status values:

	//! \todo review OID_802_11_ENCRYPTION_STATUS please.

	switch(Adapter->securitypriv.ndisencryptstatus) {
		case Ndis802_11Encryption1Enabled:
			if(Adapter->securitypriv.dot11DefKeylen[Adapter->securitypriv.dot11PrivacyKeyIndex]== 0){
				ulInfo = Ndis802_11Encryption1KeyAbsent;
				DEBUG_ERR( ("Query OID_802_11_ENCRYPTION_STATUS: Ndis802_11Encryption1KeyAbsent\n"));
			}
			else
				ulInfo = Adapter->securitypriv.ndisencryptstatus;
			break;
			
		case Ndis802_11Encryption2Enabled:
			ulInfo = Adapter->securitypriv.ndisencryptstatus;
			break;
			
		case Ndis802_11Encryption3Enabled:
				ulInfo = Adapter->securitypriv.ndisencryptstatus;
			break;

		default:
			ulInfo = Adapter->securitypriv.ndisencryptstatus;
			break;
	}
	DEBUG_ERR( ("Query OID_802_11_ENCRYPTION_STATUS: %d\n", Adapter->securitypriv.ndisencryptstatus));

	_func_exit_;
	return status;
}



NDIS_STATUS query_oid_802_11_association_information(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;

	{
		PNDIS_802_11_ASSOCIATION_INFORMATION pAssocInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION)&padapter->securitypriv.assoc_info;
		memset(&padapter->securitypriv.assoc_info, 0, sizeof(NDIS_802_11_ASSOCIATION_INFORMATION));
		query_802_11_association_information(padapter, pAssocInfo);

		pInfo = pAssocInfo;
		ulInfoLen = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION) +	\
										pAssocInfo->RequestIELength + 				\
										pAssocInfo->ResponseIELength;

		DEBUG_ERR( ("Query OID_802_11_ASSOCIATION_INFORMATION, length=%d\n", ulInfoLen ) );
	}

			
       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
	
}

NDIS_STATUS query_oid_802_11_capability(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;
	
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;


	_func_enter_;
	// This OID is used to query the miniport driver for its supported wireless network authentication and encryption capabilities.

	// Only miniport drivers that support WPA2 functionality can support this OID. When queried, the driver must indicate that it supports both WPA2:AES and WPA-PSK2:AES in order to be considered WPA2-capable by the operating system.

	// Drivers that only support WPA must return NDIS_STATUS_NOT_SUPPORTED when this OID is queried.

	// In response to a query of this OID, the miniport driver returns an NDIS_802_11_CAPABILITY structure, which is defined as follows:
	{
		
		//pInfo = &(pMgntInfo->SecurityInfo.szCapability[0]);
		pInfo = &(Adapter->securitypriv.szofcapability[0]);

		query_802_11_capability( Adapter, (u8*)pInfo, &ulInfoLen );
		
	//! \todo process OID_802_11_CAPABILITY.
		DEBUG_ERR( ("Query OID_802_11_CAPABILITY.\n"));
	}

       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
	
	
}
	
NDIS_STATUS query_oid_802_11_pmkid(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;
	

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;


	DEBUG_ERR( ("Query OID_802_11_PMKID.\n"));


	_func_exit_;
	return (status);
	
	
}



//1Query Functions for Segment 9

NDIS_STATUS query_oid_802_11_network_types_supported(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;
	struct registry_priv	*pregistrypriv = &Adapter->registrypriv;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	// When queried, the OID_802_11_NETWORK_TYPES_SUPPORTED OID requests that the miniport driver return an array of 
	// all the network types that the underlying NIC's physical layer (PHY) supports. 
	// If the NIC supports multiple 802.11 radios, the driver returns all applicable network types.
	DEBUG_ERR( ("Query OID_802_11_NETWORK_TYPES_SUPPORTED: \n"));
	{

		if (poid_par_priv->information_buf_len < (sizeof(NDIS_802_11_NETWORK_TYPE) * 3 + sizeof(u32))){
			*poid_par_priv->bytes_rw = 0;
			*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_NETWORK_TYPE) * 3 + sizeof(u32);
			return NDIS_STATUS_BUFFER_TOO_SHORT;

		} else {
			
			NDIS_802_11_NETWORK_TYPE_LIST	*networkType = (NDIS_802_11_NETWORK_TYPE_LIST *)poid_par_priv->information_buf;

			if(pregistrypriv->wireless_mode == WIRELESS_11B)
			{
				
				networkType->NumberOfItems = 1;
				networkType->NetworkType[0] = Ndis802_11DS;
			
			}
			else if (pregistrypriv->wireless_mode == WIRELESS_11BG)
			{
				
				networkType->NumberOfItems = 2;
				networkType->NetworkType[0] = Ndis802_11DS;
				networkType->NetworkType[1] = Ndis802_11OFDM24;
			
			}
			else if  (pregistrypriv->wireless_mode == WIRELESS_11G)
			{
				networkType->NumberOfItems = 1;
				networkType->NetworkType[0] = Ndis802_11OFDM24;
			}
			
			else
				networkType->NumberOfItems = 0;

			*poid_par_priv->bytes_rw = 	sizeof(NDIS_802_11_NETWORK_TYPE) * networkType->NumberOfItems 
							+ sizeof(u32);
			*poid_par_priv->bytes_needed = 0;
				
			_func_exit_;
			return NDIS_STATUS_SUCCESS;			



			 
		}

	}

	
	
}

NDIS_STATUS query_oid_802_11_network_type_in_use(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;
	// When this OID is queried, the miniport driver must return the network type that is currently in use. 
	// For example, if the device is associated with an 802.11g access point that is using OFDM 2.4, 
	// the miniport returns Ndis802_11OFDM24. 
	// If the network type changes to DSSS, the miniport returns Ndis802_11DS when the OID is queried.

	// The miniport driver returns NDIS_STATUS_INVALID_DATA if an invalid type is specified in the request.

	DEBUG_ERR( ("Query OID_802_11_NETWORK_TYPE_IN_USE: \n"));

	{
		
		NDIS_WLAN_BSSID_EX *pnetwork;
		struct mlme_priv* pmlmepriv = &Adapter->mlmepriv;

		if ( (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) ||
			(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))
			pnetwork = &pmlmepriv->cur_network.network;
		else
			pnetwork = &(Adapter->registrypriv.dev_network);
		
		ulInfo =pnetwork->NetworkTypeInUse;

	}
	status = FillQueryInformation((PVOID)&ulInfo, 4, poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
	_func_exit_;
	return status;

		
	
}
	
NDIS_STATUS query_oid_802_11_rssi(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;
	// When queried, the OID_802_11_RSSI OID requests that the miniport driver return the current value of 
	// the received signal strength indication (RSSI).
	
	// The RSSI is measured in dBm, and the data type is defined as follows:
	//		typedef LONG  NDIS_802_11_RSSI;

	// The normal range for the RSSI values is from -10 through -200dBm.

	// A query of this OID is only valid when the device is associated. 
	// The driver must return NDIS_STATUS_ADAPTER_NOT_READY if the device is not associated.

	// A query of this OID must not cause the device to send or receive packets in order to determine the RSSI value.

	// Translate to dBm
	{
		struct	mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
		NDIS_WLAN_BSSID_EX  *pcur_bss = &pmlmepriv->cur_network.network;


		pcur_bss->Rssi = -10;

		status = FillQueryInformation((PVOID)&pcur_bss->Rssi, 
									4, 
									poid_par_priv->information_buf, 
									poid_par_priv->information_buf_len, 
									poid_par_priv->bytes_rw, 
									poid_par_priv->bytes_needed);
		
		//DEBUG_ERR( ("Query OID_802_11_RSSI: %d\n", pcur_bss->Rssi));
		_func_exit_;
		return status;


	}



}
	
NDIS_STATUS query_oid_802_11_supported_rates(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	{
		NDIS_802_11_RATES_EX* prates;
		struct	mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
		NDIS_WLAN_BSSID_EX  *pcur_bss = &pmlmepriv->cur_network.network;

 		
		if ( (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) ||
                    (check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))
			prates = &pcur_bss->SupportedRates;
		else
			prates = &Adapter->registrypriv.dev_network.SupportedRates;
	
		status = FillQueryInformation(prates, 
									sizeof(NDIS_802_11_RATES), 
									poid_par_priv->information_buf, 
									poid_par_priv->information_buf_len, 
									poid_par_priv->bytes_rw, 
									poid_par_priv->bytes_needed);
		_func_exit_;
		return status;
	}



}
	
NDIS_STATUS query_oid_802_11_configuration(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	{
		NDIS_802_11_CONFIGURATION		config80211;
		NDIS_802_11_CONFIGURATION		*pnic_Config;

		struct	mlme_priv	*pmlmepriv = &Adapter->mlmepriv;

		DEBUG_ERR( ("Query OID_802_11_CONFIGURATION: \n"));

		if ( (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) ||
			(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))
			pnic_Config = &pmlmepriv->cur_network.network.Configuration;
		else
			pnic_Config = &Adapter->registrypriv.dev_network.Configuration;

		_memcpy(&config80211, pnic_Config, sizeof(NDIS_802_11_CONFIGURATION));
		
		
		if((check_fwstate(pmlmepriv, WIFI_STATION_STATE)==_TRUE) ||
       	    (check_fwstate(pmlmepriv, WIFI_AP_STATE)==_TRUE) )
		{
			config80211.DSConfig = 0;
			config80211.ATIMWindow = 0;
		}

		if(check_fwstate(pmlmepriv, _FW_LINKED)==_FALSE)
			config80211.BeaconPeriod = 0;

		status = FillQueryInformation((PVOID)&config80211, sizeof(NDIS_802_11_CONFIGURATION), poid_par_priv->information_buf, poid_par_priv->information_buf_len, poid_par_priv->bytes_rw, poid_par_priv->bytes_needed);
		_func_exit_;
		return status;

	}
	
}

NDIS_STATUS query_oid_802_11_power_mode(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	// When queried, this OID requests that the miniport driver return the current power mode of the NIC.
#ifdef CONFIG_SDIO_HCI

	status = FillQueryInformation((PVOID)&Adapter->dvobjpriv.regpwrsave, 
								4, 
								poid_par_priv->information_buf, 
								poid_par_priv->information_buf_len, 
								poid_par_priv->bytes_rw, 
								poid_par_priv->bytes_needed);
#endif
	_func_exit_;
	return status;
	

}
	
NDIS_STATUS query_oid_802_11_bssid_list(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;

	{

		u32		bufSizeRequired, bssid_ex_sz;
		PNDIS_802_11_BSSID_LIST_EX	pNdisBssListEx;
		PNDIS_WLAN_BSSID_EX		pNdisBssEx;
		
		_irqL	irqL;


		u32	no_report = 0;


		struct	mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
		_queue				*queue	= &(pmlmepriv->scanned_queue);
		_list					*plist, *phead;
		struct	wlan_network	*pnetwork = NULL;
		struct  	wlan_network	*oldest = NULL;

		DEBUG_ERR( (" Start of Query OID_802_11_BSSID_LIST.\n"));
		
		// step1. calculate if(InformationBufferLength < bufSizeRequired)
		//_init_listhead(&h2c.list);


		if (poid_par_priv->information_buf_len < sizeof(u32)) {

				// only ask for a rought estimate...
				
				*poid_par_priv->bytes_needed = 
				pmlmepriv->num_of_scanned * (sizeof(NDIS_WLAN_BSSID_EX) );
				*poid_par_priv->bytes_rw = 0;
				_func_exit_;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
		}
		
		bufSizeRequired = sizeof(u32);

		pNdisBssListEx = (PNDIS_802_11_BSSID_LIST_EX)poid_par_priv->information_buf;
		
		pNdisBssListEx->NumberOfItems = 0;

		pNdisBssEx = &(pNdisBssListEx->Bssid[0]);


		//this is a extremely special case... we shall lock scanned_queue.lock to 
		//avoid possible memory access violation...


		_enter_critical(&(pmlmepriv->scanned_queue.lock), &irqL);

		phead = get_list_head(queue);
		plist = get_next(phead);

		
		while(1)
		{
			if (end_of_queue_search(phead,plist)== _TRUE)
				break;

			pnetwork	= LIST_CONTAINOR(plist, struct wlan_network, list);

			pnetwork->network.Length = bssid_ex_sz = get_NDIS_WLAN_BSSID_EX_sz(&pnetwork->network);
		
			bufSizeRequired += bssid_ex_sz;
			
			plist = get_next(plist);
			
			if(poid_par_priv->information_buf_len < bufSizeRequired) {
				no_report = 1;
				DEBUG_INFO(("InformationBufferLength:%x, bufSizeRequired:%x\n",
					poid_par_priv->information_buf_len < bufSizeRequired));
				pNdisBssListEx->NumberOfItems = 0;
				continue;
			}
			
			//copy pnetwork->network to the specified memory....

			_memcpy((u8 *)pNdisBssEx, &(pnetwork->network), bssid_ex_sz);

			// convert channel to frequency
			pNdisBssEx->Configuration.DSConfig = (u32)ch2freq((u32)pnetwork->network.Configuration.DSConfig);
					
			pNdisBssEx = (NDIS_WLAN_BSSID_EX *)( (u32)pNdisBssEx + bssid_ex_sz);
			
			pNdisBssListEx->NumberOfItems++;
		}


		_exit_critical(&(pmlmepriv->scanned_queue.lock), &irqL);


		//RT_TRACE(COMP_SITE_SURVEY, DBG_LOUD, ("@@@@@ End of OID_802_11_BSSID_LIST--- bsscnt = %d ", pNdisBssListEx->NumberOfItems));
		DEBUG_ERR (("@@@@@ End of OID_802_11_BSSID_LIST--- bsscnt = %d ", pNdisBssListEx->NumberOfItems));
		phead = get_list_head(queue);
		plist = get_next(phead);
		

		if (no_report) {
			
			*poid_par_priv->bytes_needed = bufSizeRequired;
			
			*poid_par_priv->bytes_rw = 0;	
			_func_exit_;
			return NDIS_STATUS_BUFFER_TOO_SHORT;
		} 
		else {

			//DEBUG_ERR("Report BSS with ByteWritten = %d\n",bufSizeRequired);	

			*poid_par_priv->bytes_needed = *poid_par_priv->bytes_rw = bufSizeRequired;
			_func_exit_;	
			return NDIS_STATUS_SUCCESS;
		}

	}




}


//1Query Functions for Segment 7


/*
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	ulInfo = NdisMedium802_3;


       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);
*/

NDIS_STATUS query_oid_pnp_capabilities(struct oid_par_priv* poid_par_priv)
{

	NDIS_STATUS				status = NDIS_STATUS_SUCCESS;
	NDIS_PNP_CAPABILITIES     Power_Management_Capabilities;
	u32						ulInfoLen = sizeof(NDIS_PNP_CAPABILITIES);
	PVOID					pInfo = (PVOID) &Power_Management_Capabilities;

	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	// Not support WOL
        Power_Management_Capabilities.WakeUpCapabilities.MinMagicPacketWakeUp   = NdisDeviceStateUnspecified;
        Power_Management_Capabilities.WakeUpCapabilities.MinPatternWakeUp      	= NdisDeviceStateUnspecified;
        Power_Management_Capabilities.WakeUpCapabilities.MinLinkChangeWakeUp    = NdisDeviceStateUnspecified;
	

       status = copy_oid_info(poid_par_priv, pInfo, ulInfoLen);	
	_func_exit_;
	return (status);



}

//1Query Functions for Segment 10

NDIS_STATUS query_oid_802_11_statistics(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	ULONGLONG			ul64Info = 0;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;


	DEBUG_ERR( ("Query OID_802_11_STATISTICS: We don't support it.\n"));
	_func_exit_;
	return NDIS_STATUS_NOT_SUPPORTED;


}





//1Set Functions for Segment 1

NDIS_STATUS
set_oid_gen_current_packet_filter(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	u32			PacketFilter;
	
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;
	if(poid_par_priv->information_buf_len != sizeof(u32)) {
		_func_exit_;
		return NDIS_STATUS_INVALID_LENGTH;
	}

	*poid_par_priv->bytes_rw = poid_par_priv->information_buf_len;
	PacketFilter = *(PULONG)poid_par_priv->information_buf;

	if ( (PacketFilter & NIC8711_SUPPORTED_FILTERS) == 0 ) {
		
		status = NDIS_STATUS_NOT_SUPPORTED;
		_func_exit_;
		return status;
	}

	if ( Adapter->NdisPacketFilter == PacketFilter ) {
		status = NDIS_STATUS_SUCCESS;
		_func_exit_;
		return status;
	}

	if (PacketFilter & (NDIS_PACKET_TYPE_ALL_MULTICAST | NDIS_PACKET_TYPE_PROMISCUOUS )) {
		u32	MCR[2];
		MCR[0] = 0xffffffff;
		MCR[1] = 0xffffffff;
		//3TO DO here
		//3Write this MCR to MAR0-7 Multicast Register 0-7
		/*! \todo OID_GEN_CURRENT_PACKET_FILTER : Write this MCR to MAR0-7 Multicast Register 0-7*/
		
	}

	Adapter->NdisPacketFilter = PacketFilter;
	_func_exit_;
	return (status);
	
}

NDIS_STATUS
set_oid_gen_current_lookahead(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	DEBUG_ERR( ("Set OID_GEN_CURRENT_LOOKAHEAD: Ignore it.\n"));


	_func_exit_;
	return (status);
	
}


NDIS_STATUS
set_oid_gen_protocol_options(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	_func_enter_;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;


	DEBUG_ERR( ("Set OID_GEN_PROTOCOL_OPTIONS: Ignore it.\n"));
	status = NDIS_STATUS_SUCCESS;
	_func_exit_;
	return (status);
	
}


//1Set Functions for Segment 2
//1Set Functions for Segment 3


//1Set Functions for Segment 4

NDIS_STATUS
set_oid_802_3_multicast_list(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;


	if( (poid_par_priv->information_buf == 0) || (poid_par_priv->information_buf_len == 0) || (poid_par_priv->information_buf_len % 6 != 0))	{
		_func_exit_;
		return NDIS_STATUS_INVALID_LENGTH;
	}

	Set_802_3_MULTICAST_LIST(Adapter, (u8 *)poid_par_priv->information_buf, (u32)poid_par_priv->information_buf_len, FALSE);

	_func_exit_;
	return (status);
	
}



//1Set Functions for Segment 5
//1Set Functions for Segment 6
//1Set Functions for Segment 7

NDIS_STATUS
set_oid_pnp_set_power(struct oid_par_priv* poid_par_priv)
{
	NDIS_DEVICE_POWER_STATE new_pwr_state;
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8			ret = _FALSE;
	
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	if(poid_par_priv->information_buf_len < sizeof(NDIS_DEVICE_POWER_STATE))
	{
		*poid_par_priv->bytes_needed = sizeof(NDIS_DEVICE_POWER_STATE);
		_func_exit_;
		status= NDIS_STATUS_INVALID_LENGTH;
	} 
	else {
		new_pwr_state = *((NDIS_DEVICE_POWER_STATE*)poid_par_priv->information_buf);
		if(	new_pwr_state == NdisDeviceStateD0 && 
				Adapter->pwrctrlpriv.pnp_current_pwr_state > NdisDeviceStateD0)
			{ //4       Wake up.

				DEBUG_ERR(("set_oid_pnp_set_power: wake up from %d to D0\n", Adapter->pwrctrlpriv.pnp_current_pwr_state ));
				Adapter->pwrctrlpriv.pnp_current_pwr_state  = new_pwr_state;
				pnp_set_power_wakeup(Adapter);
				//4  DO SOMETHING
				
				status = NDIS_STATUS_PENDING;
			}
			else if(Adapter->pwrctrlpriv.pnp_current_pwr_state  < new_pwr_state)
			{ //4  Sleep.
				DEBUG_ERR(("Set set_oid_pnp_set_power: going to sleep: %d\n", new_pwr_state));
				Adapter->pwrctrlpriv.pnp_current_pwr_state  = new_pwr_state;
				ret=pnp_set_power_sleep(Adapter);
				//4 DO SOMETHING
				status = NDIS_STATUS_PENDING;				 
			}
			else 
			{ // Otherwise.
				DEBUG_ERR(("Set set_oid_pnp_set_power: Current: %d to New: %d\n", Adapter->pwrctrlpriv.pnp_current_pwr_state, new_pwr_state));

				status = NDIS_STATUS_SUCCESS;
			}

	}

	_func_exit_;
	return status;

	
}



//1Set Functions for Segment 8

NDIS_STATUS
set_oid_802_11_bssid(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;

	if(poid_par_priv->information_buf_len < 6)
	{
		*poid_par_priv->bytes_needed = 6;
		_func_exit_;
		return NDIS_STATUS_INVALID_LENGTH;
	} else 
	{
		//MgntActSet_802_11_BSSID( Adapter, (u8*)poid_par_priv->information_buf);
		*poid_par_priv->bytes_rw= 6;
	}
	_func_exit_;
	return NDIS_STATUS_SUCCESS;


	
}

NDIS_STATUS
set_oid_802_11_ssid(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8			ret = _FALSE;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	
	if( (poid_par_priv->information_buf == 0) || (poid_par_priv->information_buf_len == 0) ) {
		_func_exit_;
		return NDIS_STATUS_INVALID_LENGTH;

	} else {

		NDIS_802_11_SSID    	*ssid;
		u8 i=0;
		
		ret = _FALSE;
		
		
		if (poid_par_priv->information_buf_len < sizeof(NDIS_802_11_SSID)){
			*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_SSID);
			*poid_par_priv->bytes_rw = 0;
			_func_exit_;
			return NDIS_STATUS_INVALID_LENGTH;
		}

		ssid = (NDIS_802_11_SSID *)poid_par_priv->information_buf;

		ret = set_802_11_ssid(padapter, ssid);
	
		*poid_par_priv->bytes_rw = sizeof(NDIS_802_11_SSID);
		_func_exit_;
		return NDIS_STATUS_SUCCESS;
	}
	



}

NDIS_STATUS
set_oid_802_11_infrastructure_mode(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8				ret = _FALSE;
	struct mlme_priv 		*pmlmepriv = &padapter->mlmepriv;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	if( (poid_par_priv->information_buf == 0) || (poid_par_priv->information_buf_len < 4) )	{

		*poid_par_priv->bytes_needed = sizeof(u32);
		_func_exit_;
		return NDIS_STATUS_INVALID_LENGTH;

	} else {
		NDIS_802_11_NETWORK_INFRASTRUCTURE networkType = *(u32 *)poid_par_priv->information_buf;
		*poid_par_priv->bytes_rw = sizeof(u32);

		ret = set_802_11_infrastructure_mode(padapter, networkType);
		if (ret == _FALSE){
			_func_exit_;
			return NDIS_STATUS_INVALID_DATA;
		} else {
			_func_exit_;
			return NDIS_STATUS_SUCCESS;
		}
	}

}

NDIS_STATUS
set_oid_802_11_add_wep(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	BOOLEAN				ret = _FALSE;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;
	{
		NDIS_802_11_WEP	*wep;

		DEBUG_ERR(("SET OID_802_11_ADD_WEP: \n"));

		if (poid_par_priv->information_buf_len < sizeof(NDIS_802_11_WEP) || poid_par_priv->information_buf == NULL){
			*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_WEP);
			*poid_par_priv->bytes_rw = 0;
			_func_exit_;
			return NDIS_STATUS_INVALID_LENGTH;
		}
		
		wep = (NDIS_802_11_WEP	*)poid_par_priv->information_buf;
		ret =set_802_11_add_wep(Adapter, wep);

		if (ret == _FAIL){
			*poid_par_priv->bytes_rw = poid_par_priv->information_buf_len;
			_func_exit_;
			return NDIS_STATUS_INVALID_DATA;
		} else {
			
			*poid_par_priv->bytes_rw = poid_par_priv->information_buf_len;
			_func_exit_;
			return NDIS_STATUS_SUCCESS;
		}

	}


}

NDIS_STATUS
set_oid_802_11_remove_wep(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	BOOLEAN				ret = _FALSE;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	{
		u32			keyindex;

		if (poid_par_priv->information_buf_len < sizeof(u32) || poid_par_priv->information_buf == NULL){
			*poid_par_priv->bytes_needed = sizeof(u32);
			*poid_par_priv->bytes_rw = 0;
			_func_exit_;
			return NDIS_STATUS_INVALID_LENGTH;
		}

		keyindex = *(u32 *)poid_par_priv->information_buf;
		ret = set_802_11_remove_wep(padapter, keyindex);

		*poid_par_priv->bytes_rw = sizeof(u32);

		if (ret == _FALSE){
			_func_exit_;
			return NDIS_STATUS_INVALID_DATA;
		} else {
			_func_exit_;
			return NDIS_STATUS_SUCCESS;
		}
	}

}
	
NDIS_STATUS
set_oid_802_11_disassociate(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	BOOLEAN				ret = FALSE;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;

	ret = set_802_11_disassociate( Adapter);
	_func_exit_;
	return NDIS_STATUS_SUCCESS;


}
	
NDIS_STATUS	
set_oid_802_11_authentication_mode(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8			ret = _FALSE;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

{
	NDIS_802_11_AUTHENTICATION_MODE authmode;

	if( (poid_par_priv->information_buf == 0) || (poid_par_priv->information_buf_len < sizeof(NDIS_802_11_AUTHENTICATION_MODE )) ) {
		status = NDIS_STATUS_INVALID_LENGTH;
		*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_AUTHENTICATION_MODE );
		_func_exit_;
		return status;
	}
	
	authmode = *(NDIS_802_11_AUTHENTICATION_MODE*)poid_par_priv->information_buf;
	DEBUG_ERR(("~~~~SET OID_802_11_AUTHENTICATION_MODE: %d\n", authmode));

	padapter->securitypriv.ndisauthtype=authmode;

	switch(authmode) {
		case Ndis802_11AuthModeOpen:
		case Ndis802_11AuthModeWPANone:	
			padapter->securitypriv.dot11AuthAlgrthm= 0; //open system
			if((padapter->securitypriv.ndisauthtype >1)||(padapter->securitypriv.dot11PrivacyAlgrthm>1)){
				padapter->securitypriv.ndisauthtype=1;
				padapter->securitypriv.dot11PrivacyAlgrthm=1;
				padapter->securitypriv.dot118021XGrpPrivacy=0;
			}
			break;
		case Ndis802_11AuthModeShared:
			padapter->securitypriv.dot11AuthAlgrthm= 1; //shared system
			if((padapter->securitypriv.ndisauthtype >1)||(padapter->securitypriv.dot11PrivacyAlgrthm>1)){
				padapter->securitypriv.ndisauthtype=1;
				padapter->securitypriv.dot11PrivacyAlgrthm=1;
				padapter->securitypriv.dot118021XGrpPrivacy=0;
			}
			break;
		case Ndis802_11AuthModeAutoSwitch:
			padapter->securitypriv.dot11AuthAlgrthm= 1; //shared system
			if((padapter->securitypriv.ndisauthtype >1)||(padapter->securitypriv.dot11PrivacyAlgrthm>1)){
				padapter->securitypriv.ndisauthtype=1;
				padapter->securitypriv.dot11PrivacyAlgrthm=1;
				padapter->securitypriv.dot118021XGrpPrivacy=0;
			}
			break;
		case Ndis802_11AuthModeWPA:
		case Ndis802_11AuthModeWPAPSK:
			padapter->securitypriv.dot11AuthAlgrthm= 2;  //802.1x
			break;

		case Ndis802_11AuthModeWPA2:
		case Ndis802_11AuthModeWPA2PSK:
			padapter->securitypriv.dot11AuthAlgrthm= 2; //802.1x
			break;

		default:
			status = NDIS_STATUS_INVALID_DATA;
			break;
	}
	DEBUG_ERR(("~~~~padapter->securitypriv.dot11PrivacyAlgrthm: %d\n", padapter->securitypriv.dot11PrivacyAlgrthm));

		if(status == NDIS_STATUS_SUCCESS){
			//clear keys
			//Set auth, if not at the Ad Hoc mode
			if(authmode !=Ndis802_11AuthModeWPANone)
				set_802_11_authenticaion_mode( padapter, authmode );
		}


	}



	_func_exit_;
	return status;


}
		
NDIS_STATUS
set_oid_802_11_bssid_list_scan(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8				ret = _FALSE;
	_func_enter_;

	DEBUG_INFO(("+set_oid_802_11_bssid_list_scan\n"));
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	ret = set_802_11_bssid_list_scan(Adapter);
	_func_exit_;
	DEBUG_INFO(("-set_oid_802_11_bssid_list_scan\n"));
	return NDIS_STATUS_SUCCESS;
	


}
		
NDIS_STATUS
set_oid_802_11_encryption_status(struct oid_par_priv* poid_par_priv)
{
	struct sta_info *pbcmc_sta;
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8			ret = _FALSE;
	struct mlme_priv 		*pmlmepriv = &padapter->mlmepriv;	
	struct sta_priv * pstapriv = &padapter->stapriv;
	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;

	{
		
	u8 * pbssid;

	if( (poid_par_priv->information_buf == 0) || (poid_par_priv->information_buf_len < sizeof(NDIS_802_11_WEP_STATUS)) ) {

		*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_WEP_STATUS);
		_func_exit_;
		return NDIS_STATUS_INVALID_LENGTH;

	}

	padapter->securitypriv.ndisencryptstatus=*(NDIS_802_11_ENCRYPTION_STATUS *)poid_par_priv->information_buf;
	DEBUG_ERR(("~~~~~SET OID_802_11_ENCRYPTION_STATUS: %d\n", *(PNDIS_802_11_ENCRYPTION_STATUS)poid_par_priv->information_buf));



	switch(padapter->securitypriv.ndisencryptstatus) {

		case Ndis802_11EncryptionNotSupported:
		case Ndis802_11EncryptionDisabled:
		DEBUG_ERR( ("\n\tNdis802_11EncryptionNotSupport or Ndis802_11EncryptionDisabled\n"));
		padapter->securitypriv.dot11PrivacyAlgrthm=_NO_PRIVACY_;
		padapter->securitypriv.dot118021XGrpPrivacy=_NO_PRIVACY_;
		
		break;
		
		case Ndis802_11Encryption1Enabled:					
		//TODO: Enable WEP
		DEBUG_ERR(("\n\tOID_802_11_ENCRYPTION_STATUS:Ndis802_11Encryption1Enabled\n"));
		padapter->securitypriv.dot11PrivacyAlgrthm=_WEP40_;
		padapter->securitypriv.dot118021XGrpPrivacy=_WEP40_;
		
		break;
		case Ndis802_11Encryption2Enabled:
		//TODO: Enable WPA
		DEBUG_ERR(("\n\tOID_802_11_ENCRYPTION_STATUS:Ndis802_11Encryption2Enabled\n"));
		padapter->securitypriv.dot11PrivacyAlgrthm=_TKIP_;
		padapter->securitypriv.dot118021XGrpPrivacy=_TKIP_;
		
		break;

		case Ndis802_11Encryption3Enabled:
		DEBUG_ERR(("\n\tOID_802_11_ENCRYPTION_STATUS:Ndis802_11Encryption3Enabled\n"));
		padapter->securitypriv.dot11PrivacyAlgrthm=_AES_;
		padapter->securitypriv.dot118021XGrpPrivacy=_AES_;
		
		break;

		default:
		DEBUG_ERR(("\n\tdefault\n"));
		padapter->securitypriv.dot11PrivacyAlgrthm=_NO_PRIVACY_;
		padapter->securitypriv.dot118021XGrpPrivacy=_NO_PRIVACY_;

		break;
	}

	if(check_fwstate(pmlmepriv, WIFI_STATION_STATE | WIFI_MP_STATE)) //sta mode
	{
		pbcmc_sta = get_bcmc_stainfo(padapter);
		if(pbcmc_sta==NULL){
			DEBUG_ERR( ("set_oid_802_11_encryption_status: no bcmc stainfo \n"));
		}
		else
		{
			if((padapter->securitypriv.ndisencryptstatus == Ndis802_11Encryption2Enabled)||
					(padapter->securitypriv.ndisencryptstatus ==  Ndis802_11Encryption3Enabled))
				pbcmc_sta->ieee8021x_blocked = _TRUE;
					
		}
	}


	*poid_par_priv->bytes_rw = poid_par_priv->information_buf_len;
	_func_exit_;
	return NDIS_STATUS_SUCCESS;
	
	}

	_func_exit_;
	return status;
}
	
		
NDIS_STATUS
set_oid_802_11_reload_defaults(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8			ret = _FALSE;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	if( (poid_par_priv->information_buf == 0) || (poid_par_priv->information_buf_len < sizeof(NDIS_802_11_RELOAD_DEFAULTS)) ) {

		*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_RELOAD_DEFAULTS);
		_func_exit_;
		return NDIS_STATUS_INVALID_LENGTH;

	} else {

		NDIS_802_11_RELOAD_DEFAULTS NdisReloadDefault = *(NDIS_802_11_RELOAD_DEFAULTS *)poid_par_priv->information_buf;

		ret =set_802_11_reload_defaults(padapter, NdisReloadDefault);
		
//		*poid_par_priv->bytes_rw = sizeof(RT_802_11_RELOAD_DEFAULTS);
		
		if (ret == _FALSE){
			_func_exit_;
			return NDIS_STATUS_INVALID_DATA;
		} else {
			_func_exit_;
			return NDIS_STATUS_SUCCESS;
		}
	}	


}
			
void PlatformRequestPreAuthentication(_adapter * padapter)
{
	u8 authmode,securitytype,cnt,match,remove_cnt;
	u8 sec_ie[255],uncst_oui[4],bkup_ie[255];
	u8 wpa_oui[4]={0x0,0x50,0xf2,0x01};
	uint 	ielength;
	int iEntry,i, j;
	struct	security_priv	*psecuritypriv=&padapter->securitypriv;
	struct mlme_priv 		*pmlmepriv = &padapter->mlmepriv;	

	uint	ndissecuritytype=psecuritypriv->ndisencryptstatus;
	uint 	ndisauthmode=psecuritypriv->ndisauthtype;
	u8	uArray[sizeof(NDIS_802_11_STATUS_INDICATION) + \
						(NUM_PRE_AUTH_KEY+1)*sizeof(NDIS_802_11_AUTHENTICATION_REQUEST) +1 ];
	u8 *	pucCurPtr;
	PNDIS_802_11_PMKID_CANDIDATE_LIST 	pPMKID;
	PPMKID_CANDIDATE					pCandidate;
	int				iNumBssid=0;
	u8 * currentbssid = get_bssid(pmlmepriv);

	//
	// Condition Check
	//
	if(ndisauthmode!=Ndis802_11AuthModeWPA2)
	{
		return;
	}

	//
	// Initialize the buffer for status indication.
	//
	memset(uArray, 0, sizeof uArray );

	pucCurPtr = &uArray[0];
	*((PNDIS_802_11_STATUS_TYPE)pucCurPtr) = 2;//Ndis802_11StatusType_PMKID_CandidateList;
	pucCurPtr += sizeof(NDIS_802_11_STATUS_TYPE);

	//------------------------------------------------------------------
	// (1) Include associated current BSSID into list
	//------------------------------------------------------------------
	// Fill Version and NumCandidates field in NDIS_802_11_PMKID_CANDIDATE_LIST.
	pPMKID = (PNDIS_802_11_PMKID_CANDIDATE_LIST)pucCurPtr;
	pPMKID->Version = 1;	//OID Ducument: This value should be 1
	pPMKID->NumCandidates = 0;	// [AnnieNote] Why Zero????????? 2006-05-08.
	pucCurPtr = (u8 *)pPMKID->CandidateList;
	
	// The current associated BSSID should always be in the StatusBuffer.		
	pCandidate = (PPMKID_CANDIDATE)pucCurPtr;
	pCandidate->Flags = 1;//NDIS_802_11_PMKID_CANDIDATE_PREAUTH_ENABLED;
	
	_memcpy( pCandidate->BSSID, currentbssid, 6 );
	iNumBssid++;	
	pucCurPtr += sizeof(PMKID_CANDIDATE);		// pointer to next PMKID_CANDIDATE.


	//------------------------------------------------------------------
	// (2) Include other BSSID for roaming candidate list (with the same SSID)
	//------------------------------------------------------------------	

	// Add the bssid onto bssid list
	pCandidate = (PPMKID_CANDIDATE)pucCurPtr;
	pCandidate->Flags = 0x0;

	_memcpy( pCandidate->BSSID, currentbssid, 6 );
	iNumBssid++;
	pucCurPtr += sizeof(PMKID_CANDIDATE);

	pPMKID->NumCandidates = iNumBssid;

	//------------------------------------------------------------------
	// (3) Indicate to upper layer.
	//------------------------------------------------------------------
	{
		
		UINT	UsedSize =	sizeof(NDIS_802_11_STATUS_INDICATION)	+	\
							sizeof(NDIS_802_11_PMKID_CANDIDATE_LIST)	+	\
							(iNumBssid-1)*sizeof(PMKID_CANDIDATE);
		NdisMIndicateStatus(padapter->hndis_adapter, 
						NDIS_STATUS_MEDIA_SPECIFIC_INDICATION, 
						(PVOID)uArray,
						UsedSize	);
		NdisMIndicateStatusComplete( padapter->hndis_adapter );
	}
}

NDIS_STATUS
set_oid_802_11_add_key(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8			ret = _FALSE;
	struct mlme_priv 		*pmlmepriv = &padapter->mlmepriv;	

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;


	{
		NDIS_802_11_KEY		*key;

		if ( poid_par_priv->information_buf == NULL)//if (InformationBufferLength < sizeof(NDIS_802_11_KEY) || InformationBuffer == NULL)
		{
			*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_KEY);
			*poid_par_priv->bytes_rw = 0;
			DEBUG_ERR(("OID_802_11_ADD_KEY:NDIS_STATUS_INVALID_LENGTH\n"));
			DEBUG_ERR(("OID_802_11_ADD_KEY:InformationBufferLength=%d sizeof(NDIS_802_11_KEY)=%d sizeof(NDIS_802_11_WEP)=%d\n",
											poid_par_priv->information_buf_len, sizeof(NDIS_802_11_KEY),sizeof(NDIS_802_11_WEP)));
			_func_exit_;
			return NDIS_STATUS_INVALID_LENGTH;
		}

		key = (NDIS_802_11_KEY *)poid_par_priv->information_buf;
		ret = set_802_11_add_key(padapter, key);
		
		if(padapter->securitypriv.dot11AuthAlgrthm== 2) // 802_1x
		{
			struct sta_info * psta,*pbcmc_sta;
			struct sta_priv * pstapriv = &padapter->stapriv;

			if(check_fwstate(pmlmepriv, WIFI_STATION_STATE | WIFI_MP_STATE)) //sta mode
			{
				psta = get_stainfo(pstapriv, get_bssid(pmlmepriv));
				
				if(psta==NULL){
					DEBUG_ERR( ("Set OID_802_11_ADD_KEY: Obtain Sta_info fail \n"));
				}
				else
				{
					psta->ieee8021x_blocked = _FALSE;
					DEBUG_ERR(("\n set_oid_802_11_add_key:ieee8021x_blocked= _FALSE\n"));
					if((padapter->securitypriv.ndisencryptstatus == Ndis802_11Encryption2Enabled)||
						(padapter->securitypriv.ndisencryptstatus ==  Ndis802_11Encryption3Enabled))
						psta->dot118021XPrivacy =padapter->securitypriv.dot11PrivacyAlgrthm;						
					
				}

				pbcmc_sta=get_bcmc_stainfo(padapter);
				if(pbcmc_sta==NULL){
					DEBUG_ERR( ("Set OID_802_11_ADD_KEY: bcmc stainfo is null \n"));
				}
				else
				{
					pbcmc_sta->ieee8021x_blocked = _FALSE;
					DEBUG_ERR(("\n set_oid_802_11_add_key:ieee8021x_blocked= _FALSE\n"));
					if((padapter->securitypriv.ndisencryptstatus == Ndis802_11Encryption2Enabled)||
						(padapter->securitypriv.ndisencryptstatus ==  Ndis802_11Encryption3Enabled))
						pbcmc_sta->dot118021XPrivacy =padapter->securitypriv.dot11PrivacyAlgrthm;						
					
				}
			}

			else if(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)) //adhoc mode
			{					}
			
		}				

		*poid_par_priv->bytes_rw = poid_par_priv->information_buf_len;

		PlatformRequestPreAuthentication(padapter);		

		if (ret == _FALSE){
			_func_exit_;
			return NDIS_STATUS_INVALID_DATA;
		} else {
			_func_exit_;
			return NDIS_STATUS_SUCCESS;
		}
	}


	
}
			
NDIS_STATUS
set_oid_802_11_remove_key(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8			ret = _FALSE;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	{
		NDIS_802_11_REMOVE_KEY	*key = NULL;

		if (poid_par_priv->information_buf_len < sizeof(PNDIS_802_11_REMOVE_KEY) || poid_par_priv->information_buf == NULL){
			*poid_par_priv->bytes_needed = sizeof(PNDIS_802_11_REMOVE_KEY);
			*poid_par_priv->bytes_rw = 0;
			_func_exit_;
			return NDIS_STATUS_INVALID_LENGTH;
		}


		key = (NDIS_802_11_REMOVE_KEY *)poid_par_priv->information_buf;
		
		ret = set_802_11_remove_key(padapter, key);
		*poid_par_priv->bytes_rw = poid_par_priv->information_buf_len;

		if (ret == _FALSE){
			_func_exit_;
			return NDIS_STATUS_INVALID_DATA;
		} else {
			_func_exit_;
			return NDIS_STATUS_SUCCESS;
		}
	}

}
			
NDIS_STATUS
set_oid_802_11_test(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			padapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8				ret = _FALSE;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	{
		NDIS_802_11_TEST *test = NULL;

		DEBUG_ERR( ("Set OID_802_11_TEST.\n"));


		if (poid_par_priv->information_buf_len < sizeof(NDIS_802_11_TEST) || poid_par_priv->information_buf == NULL){
			*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_TEST);
			*poid_par_priv->bytes_rw = 0;
			_func_exit_;
			return NDIS_STATUS_INVALID_LENGTH;
		}

		test = (NDIS_802_11_TEST *)poid_par_priv->information_buf;
		
		ret = set_802_11_test(padapter, test);

		*poid_par_priv->bytes_rw = poid_par_priv->information_buf_len;

		if (ret == _FALSE){
			_func_exit_;
			return NDIS_STATUS_INVALID_DATA;
		} else {
			_func_exit_;
			return NDIS_STATUS_SUCCESS;
		}
	}


}
			
NDIS_STATUS
set_oid_802_11_pmkid(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8 			i,j,blInserted = _FALSE;
	u8				ret = _FALSE;
	struct mlme_priv			*pmlmepriv = &Adapter->mlmepriv;
	struct security_priv *psecuritypriv=&Adapter->securitypriv;
	PBSSIDInfo pBssidInfo;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	{
		
		NDIS_802_11_PMKID *pmkid = NULL;

		if( (poid_par_priv->information_buf == 0) || (poid_par_priv->information_buf_len < 4) ){
			*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_PMKID);
			*poid_par_priv->bytes_rw = 0;
			_func_exit_;
			return NDIS_STATUS_INVALID_LENGTH;
		}

		pmkid = (NDIS_802_11_PMKID *)poid_par_priv->information_buf;
		pBssidInfo = (PBSSIDInfo)(pmkid->BSSIDInfo);	//handle PMKID

	for( i=0; i<pmkid->BSSIDInfoCount; i++ )
	{
		if( i >= NUM_PMKID_CACHE )
		{
			break;
		}

		blInserted = _FALSE;
		
		//overwrite PMKID
		for(j=0 ; j<NUM_PMKID_CACHE; j++)
		{
			if(_memcmp(psecuritypriv->PMKIDList[j].Bssid, pBssidInfo->BSSID, ETH_ALEN)==_TRUE )
			{ // BSSID is matched, the same AP => rewrite with new PMKID.
				_memcpy(psecuritypriv->PMKIDList[j].PMKID, pBssidInfo->PMKID, sizeof(pBssidInfo->PMKID));
				psecuritypriv->PMKIDIndex = j+1;
				blInserted = _TRUE;
				break;
			}	
		}

		if(blInserted){
			break;
		}
		else{
			// Find a new entry
			_memcpy(psecuritypriv->PMKIDList[psecuritypriv->PMKIDIndex].Bssid, pBssidInfo->BSSID, ETH_ALEN);
			_memcpy(psecuritypriv->PMKIDList[psecuritypriv->PMKIDIndex].PMKID, pBssidInfo->PMKID, 16);
			psecuritypriv->PMKIDIndex ++ ;
			if(psecuritypriv->PMKIDIndex==16)
				psecuritypriv->PMKIDIndex =0;
		}

		pBssidInfo ++;	// pointer to next BSSID_INFO.		}
	}

		ret = set_802_11_pmkid(Adapter, pmkid);

		*poid_par_priv->bytes_rw = poid_par_priv->information_buf_len;

		if (ret == FALSE){
			_func_exit_;
			return NDIS_STATUS_INVALID_DATA;
		} else {
			_func_exit_;
			return NDIS_STATUS_SUCCESS;
		}
	}

}



//1Set Functions for Segment 9

NDIS_STATUS
set_oid_802_11_network_type_in_use(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8			ret = _FALSE;
	struct mlme_priv 		*pmlmepriv = &Adapter->mlmepriv;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;

	{
	
		NDIS_WLAN_BSSID_EX *pnetwork;
		u8* pnetwork_in_use = NULL;

		DEBUG_ERR( ("Set OID_802_11_NETWORK_TYPE_IN_USE:%d \n", poid_par_priv->information_buf));

		if( (poid_par_priv->information_buf == 0) || (poid_par_priv->information_buf_len < 4) )	{
				*poid_par_priv->bytes_needed = sizeof(u32);
				_func_exit_;
				return NDIS_STATUS_INVALID_LENGTH;
		}
		
		
		if ( (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) ||
			(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))
		{
			pnetwork = &pmlmepriv->cur_network.network;
		}
		
		else
		{
			pnetwork = &(Adapter->registrypriv.dev_network);
			pnetwork_in_use = &Adapter->registrypriv.wireless_mode;
		}


		//HCT12.0 NDTest
		if ((*(PULONG)poid_par_priv->information_buf == Ndis802_11OFDM5)||
			(*(PULONG)poid_par_priv->information_buf == Ndis802_11OFDM24) || 
			(*(PULONG)poid_par_priv->information_buf == Ndis802_11DS) )	{

			//Adapter->NdisCEDev.BeSetNetworkTypeByNDTest = (NDIS_802_11_NETWORK_TYPE)(*(PULONG)InformationBuffer);
			pnetwork->NetworkTypeInUse = (NDIS_802_11_NETWORK_TYPE)(*(PULONG)poid_par_priv->information_buf);
			if(pnetwork_in_use)
			{
				switch(*(PULONG)poid_par_priv->information_buf)
				{
					case Ndis802_11OFDM5:
						*pnetwork_in_use = WIRELESS_11A;
						break;

					case Ndis802_11OFDM24:
						*pnetwork_in_use = WIRELESS_11BG;
						break;

					case Ndis802_11DS:
						*pnetwork_in_use = WIRELESS_11B;
						break;
				}
			}
			
		} else {

			pnetwork->NetworkTypeInUse = (NDIS_802_11_NETWORK_TYPE)0;
			status = NDIS_STATUS_INVALID_DATA;
		}

		*poid_par_priv->bytes_rw =  sizeof(NDIS_802_11_NETWORK_TYPE);
		_func_exit_;
		return status;
	}



	
}

NDIS_STATUS
set_oid_802_11_configuration(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	BOOLEAN				ret = FALSE;
	struct mlme_priv 		*pmlmepriv = &Adapter->mlmepriv;
	struct registry_priv	*pregistrypriv = &Adapter->registrypriv;
	NDIS_WLAN_BSSID_EX    *pdev_network = &pregistrypriv->dev_network;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;

	_func_enter_;

	if( (poid_par_priv->information_buf == 0) || 
		(poid_par_priv->information_buf_len < sizeof(NDIS_802_11_CONFIGURATION)) )
	{
		*poid_par_priv->bytes_rw = 0;
		*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_CONFIGURATION);
		_func_exit_;
		return NDIS_STATUS_INVALID_LENGTH;
	}
	else
	{	
		NDIS_WLAN_BSSID_EX *pnetwork=NULL ;
		u8 * pchannel = NULL;
		NDIS_802_11_CONFIGURATION *Config80211 = (NDIS_802_11_CONFIGURATION *)poid_par_priv->information_buf;

		if(check_fwstate(pmlmepriv, _FW_LINKED))
			pnetwork = &pmlmepriv->cur_network.network;
		else
		{
			//return NDIS_STATUS_INVALID_LENGTH;
			//pnetwork = &(Adapter->registrypriv.dev_network);
			//pchannel = &Adapter->registrypriv.channel;
		}

		if(pnetwork)
			_memcpy(&pnetwork->Configuration, &Config80211, Config80211->Length);

		if(pchannel)
			*pchannel = (u8) freq2ch(Config80211->DSConfig);

		
		// double check
		if(!((check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)==_TRUE) ||
			(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)==_TRUE) ))

		{
			if(pnetwork)
			{
				if(pnetwork->Configuration.DSConfig != 0||
				pnetwork->Configuration.ATIMWindow != 0 )
				{
					pnetwork->Configuration.DSConfig = 0 ;
					pnetwork->Configuration.ATIMWindow = 0 ;
					DEBUG_ERR( ("*** Error? Not AdhocMode but DSConfig != 0 || ATIMWindow != 0***\n\n"));
				}
			}
		}
		
		//Update channel for using Funk Software in ad hoc master mode
		pdev_network->Configuration.DSConfig = freq2ch(Config80211->DSConfig/1000);
		pregistrypriv->channel = (u8)pdev_network->Configuration.DSConfig;

		
	}

	*poid_par_priv->bytes_rw = sizeof(NDIS_802_11_CONFIGURATION);
	_func_exit_;
	return NDIS_STATUS_SUCCESS;

	
}

NDIS_STATUS
set_oid_802_11_power_mode(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	u32			ulInfo = 0;
	u32			ulInfoLen = sizeof(ulInfo);
	PVOID				pInfo = (PVOID) &ulInfo;
	PADAPTER			Adapter = (PADAPTER)(poid_par_priv->adapter_context);
	u8			ret = _FALSE;

	*poid_par_priv->bytes_rw = 0;
	*poid_par_priv->bytes_needed = 0;
	_func_enter_;
	{
		NDIS_802_11_POWER_MODE ndisPsModeToSet;
		ps_mode rtPsModeToSet;

		if( (poid_par_priv->information_buf == NULL) || 	(poid_par_priv->information_buf_len < sizeof(NDIS_802_11_POWER_MODE)) ) {
			*poid_par_priv->bytes_needed = sizeof(NDIS_802_11_AUTHENTICATION_MODE );
			_func_exit_;
			return NDIS_STATUS_INVALID_LENGTH;
		}

		ndisPsModeToSet = *((NDIS_802_11_POWER_MODE*)poid_par_priv->information_buf);
		DEBUG_ERR( ("Set OID_802_11_POWER_MODE: 0x%X\n", ndisPsModeToSet));
	}

	_func_exit_;
	return status;

	
}





//1Set Functions for Segment 10




//1OID Handler for Segment 1


NDIS_STATUS
oid_gen_hardware_status_hdl(struct oid_par_priv* poid_par_priv)
{	

	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_hardware_status(poid_par_priv);	
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
	
}







NDIS_STATUS
oid_gen_supported_list_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_supported_list(poid_par_priv);	
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}


#define query_oid_hdl query_ ## __FUNCTION__



NDIS_STATUS
oid_gen_media_supported_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_media_supported(poid_par_priv);		
			//status = query_oid_hdl(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}


NDIS_STATUS
oid_gen_media_in_use_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_media_in_use(poid_par_priv);					
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}


NDIS_STATUS
oid_gen_maximum_lookahead_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_maximum_lookahead(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}



NDIS_STATUS
oid_gen_maximum_frame_size_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_maximum_frame_size(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}



NDIS_STATUS
oid_gen_link_speed_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_link_speed_hdl(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}




NDIS_STATUS
oid_gen_transmit_buffer_space_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_transmit_buffer_space_hdl(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}





NDIS_STATUS
oid_gen_receive_buffer_space_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_receive_buffer_space_hdl(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}



NDIS_STATUS
oid_gen_transmit_block_size_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_transmit_block_size(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}


NDIS_STATUS
oid_gen_receive_block_size_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_receive_block_size(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}



NDIS_STATUS
oid_gen_vendor_id_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_vendor_id_hdl(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}




NDIS_STATUS
oid_gen_vendor_description_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_vendor_description_hdl(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}				
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_gen_current_packet_filter_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_current_packet_filter(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_gen_current_packet_filter(poid_par_priv);			
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}



NDIS_STATUS
oid_gen_current_lookahead_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_current_lookahead(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_gen_current_lookahead(poid_par_priv);		
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}




NDIS_STATUS
oid_gen_driver_version_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_driver_version_hdl(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}



NDIS_STATUS
oid_gen_maximum_total_size_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_maximum_total_size(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_gen_protocol_options_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			set_oid_gen_protocol_options(poid_par_priv);
			break;
	}			
	_func_exit_;
	return status;
}



NDIS_STATUS
oid_gen_mac_options_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_mac_options(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}




NDIS_STATUS
oid_gen_media_connect_status_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_media_connect_status(poid_par_priv);		
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_gen_maximum_send_packets_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_maximum_send_packets(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}


NDIS_STATUS
oid_gen_vendor_driver_version_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_vendor_driver_version(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}



//1OID Handler for Segment 2

NDIS_STATUS
oid_gen_physical_medium_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
#ifndef WINDOWS_VIRTUAL		
	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_physical_medium(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			

#endif
	_func_exit_;
	return status;
}


//1OID Handler for Segment 3

NDIS_STATUS
oid_gen_xmit_ok_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_xmit_ok(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_gen_rcv_ok_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_rcv_ok(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_gen_xmit_error_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_xmit_error(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_gen_rcv_error_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_rcv_error(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_gen_rcv_no_buffer_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_gen_rcv_no_buffer(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}





//1OID Handler for Segment 4

NDIS_STATUS
oid_802_3_permanent_address_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_3_permanent_address(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_current_address_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_3_current_address(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_multicast_list_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_3_multicast_list(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_3_multicast_list(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_maximum_list_size_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_3_maximum_list_size(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_mac_options_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}



//1OID Handler for Segment 5

NDIS_STATUS
oid_802_3_rcv_error_alignment_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_3_rcv_error_alignment(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_xmit_one_collision_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_3_xmit_one_collision(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_xmit_more_collisions_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_3_xmit_more_collisions(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}


//1OID Handler for Segment 6

NDIS_STATUS
oid_802_3_xmit_deferred_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "oid_802_3_xmit_deferred Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_xmit_max_collisions_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_rcv_overrun_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_xmit_underrun_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");	
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_xmit_heartbeat_failure_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_xmit_times_crs_lost_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_3_xmit_late_collisions_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}



//1OID Handler for Segment 7

NDIS_STATUS
oid_pnp_capabilities_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_pnp_capabilities(poid_par_priv);	
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_pnp_set_power_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status=set_oid_pnp_set_power(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_pnp_query_power_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			query_oid_pnp_query_power(poid_par_priv);
			status = NDIS_STATUS_SUCCESS;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_pnp_add_wake_up_pattern_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_pnp_remove_wake_up_pattern_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_pnp_wake_up_pattern_list_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_pnp_enable_wake_up_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}



//1OID Handler for Segment 8

NDIS_STATUS
oid_802_11_bssid_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_bssid(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_ssid_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			query_oid_802_11_ssid(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			set_oid_802_11_ssid(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_infrastructure_mode_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_infrastructure_mode(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_infrastructure_mode(poid_par_priv);			
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_add_wep_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_add_wep(poid_par_priv);			
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_remove_wep_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_remove_wep(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_disassociate_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_disassociate(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_authentication_mode_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_authentication_mode(poid_par_priv);			
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_authentication_mode(poid_par_priv);			
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_privacy_filter_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "oid_802_11_privacy_filter Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_bssid_list_scan_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_bssid_list_scan(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_encryption_status_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_encryption_status(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_encryption_status(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_reload_defaults_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_reload_defaults(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_add_key_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_add_key(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}				
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_remove_key_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_remove_key(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_association_information_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_association_information(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS	
oid_802_11_test_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_test(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}				
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_media_stream_mode_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_capability_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_capability(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}


NDIS_STATUS
oid_802_11_pmkid_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status =	query_oid_802_11_pmkid(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_pmkid(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}




//1OID Handler for Segment 9

NDIS_STATUS
oid_802_11_network_types_supported_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_network_types_supported(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_network_type_in_use_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_network_type_in_use(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_network_type_in_use(poid_par_priv);	
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_tx_power_level_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_rssi_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			query_oid_802_11_rssi(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_rssi_trigger_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_fragmentation_threshold_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_rts_threshold_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_number_of_antennas_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_rx_antenna_selected_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_tx_antenna_selected_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_supported_rates_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_supported_rates(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;}

NDIS_STATUS
oid_802_11_desired_rates_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
/*	
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			DEBUG_OID(poid_par_priv->dbg, "Query operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
*/
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_configuration_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_configuration(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_configuration(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_power_mode_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_power_mode(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			status = set_oid_802_11_power_mode(poid_par_priv);
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}

NDIS_STATUS
oid_802_11_bssid_list_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_bssid_list(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}






//1OID Handler for Segment 10

NDIS_STATUS
oid_802_11_statistics_hdl(struct oid_par_priv* poid_par_priv)
{
	NDIS_STATUS	 status = NDIS_STATUS_SUCCESS;	
	_func_enter_;
	switch(poid_par_priv->type_of_oid)
	{
		case QUERY_OID:
			DEBUG_OID(poid_par_priv->dbg, "Query\n");
			status = query_oid_802_11_statistics(poid_par_priv);
			break;
		case SET_OID:
			DEBUG_OID(poid_par_priv->dbg, "Set\n");
			DEBUG_OID(poid_par_priv->dbg, "Set operation is not supported!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		default:
			DEBUG_OID(poid_par_priv->dbg, "Error OID type ... choose set or query!\n");
			status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}			
	_func_exit_;
	return status;
}





/*! \brief NDIS will pass OIDs to this function. this function must process these OIDs. */
NDIS_STATUS drv_query_info(
	IN	_nic_hdl		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
	)
{
	struct oid_par_priv 	oid_par;
	
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	PADAPTER			Adapter = (PADAPTER)MiniportAdapterContext;
	u8			offset;
	u32 i;

	*BytesWritten = 0;
	*BytesNeeded = 0;

	oid_par.adapter_context = MiniportAdapterContext;
	oid_par.oid = Oid;
	oid_par.information_buf = InformationBuffer;
	oid_par.information_buf_len = InformationBufferLength;
	oid_par.bytes_rw = BytesWritten;
	oid_par.bytes_needed = BytesNeeded;
	oid_par.type_of_oid = QUERY_OID;	
	oid_par.dbg = 0;

	_func_enter_;
	if(Adapter->bSurpriseRemoved){
		DEBUG_ERR(("drv_query_info:Adapter->bSurpriseRemoved"));
		goto exit;
	}
			
	for(i = 0; i < sizeof(oid_funs_array)/sizeof(struct oid_funs_node) ; i++)
	{	
	
		//If this segment of OID
		if(Oid >= oid_funs_array[i].oid_start && Oid <= oid_funs_array[i].oid_end)
		{			
		
			offset = (u8)(Oid - oid_funs_array[i].oid_start);

			//If offset is inside the boundary of this corresponding array
			if(offset < oid_funs_array[i].array_sz/sizeof(struct oid_obj_priv))
			{
				oid_par.dbg = oid_funs_array[i].node_array[offset].dbg; //Whether to print debug message
				Status = oid_funs_array[i].node_array[offset].oidfuns(&oid_par);	//OID handler function call
				oid_funs_array[i].query_counter++; // Count query hit numbers for each segment 
				break;			
			}			
			else
			{
				DEBUG_ERR(("Non-Executable OID\n"));
				break;
			}
			
		}
		
	}


	switch(Oid & 0xFFFFFF00)			
	{
	
#ifdef CONFIG_MP_INCLUDED

		case OID_MP_SEG1:			
		case OID_MP_SEG2:					
		case OID_MP_SEG3:			
		case OID_MP_SEG4:	
			Status=mp_query_info(Adapter,//MPPerformQueryInformation(Adapter,
							                                Oid,
							                               (u8*)InformationBuffer,
							                               (u32)InformationBufferLength,
							                               BytesWritten,
	  						                               BytesNeeded);
			
			*BytesWritten = InformationBufferLength;			
			break;		

#endif



	}
exit:
	_func_exit_;
	return(Status);
}




/*! \brief Compute CRC32 */
u32 ComputeCrc(u8 *Buffer, u32	Length) {
    u32	Crc, Carry;
    u32	i, j;
    u8	CurByte;

    Crc = 0xffffffff;
	_func_enter_;
    for (i = 0; i < Length; i++)
    {
        CurByte = Buffer[i];
        for (j = 0; j < 8; j++)
        {
            Carry = ((Crc & 0x80000000) ? 1 : 0) ^ (CurByte & 0x01);
            Crc <<= 1;
            CurByte >>= 1;
            if (Carry)
            {
                Crc = (Crc ^ 0x04c11db6) | Carry;
            }
        }
    }
    Crc = Crc & 0x0ff000000;                    // 95.05.04     Sid
    _func_exit_;
    return Crc;
}

/*! \brief ?? */
VOID GetMulticastBit(IN u8 Address[6], OUT u8 * Byte, OUT u8 *Value){
    u32 	Crc;
    u32 	BitNumber;

    Crc = ComputeCrc(Address, 6);
	_func_enter_;
    // The bit number is now in the 6 most significant bits of CRC.
    BitNumber = (u32)((Crc >> 26) & 0x3f);
    *Byte = (u8)(BitNumber / 8);
    *Value = (u8)((u8)1 << (BitNumber % 8));
	_func_exit_;
}

/*! \brief Get the Multicast List maintained by Driver. */
VOID Get_802_3_MULTICAST_LIST(PADAPTER Adapter, OUT u8 *buffer, OUT u32 *length){
	_func_enter_;
	if (Adapter->MCAddrCount == 0){
		*length = 0;
	} else {
		*length = Adapter->MCAddrCount * 6;
		_memcpy(buffer, Adapter->MCList, *length);
	}
	_func_exit_;
	return;	

}

/*! \brief Set the Multicast List specified by NDIS. */
VOID Set_802_3_MULTICAST_LIST(PADAPTER Adapter, u8 *MCListbuf, u32 MCListlen, BOOLEAN bAcceptAllMulticast) {
// ----------MgntInfo----------------------
//	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u8			MCReg[8];
	u32			i;
	u8 			Byte;
	u8			Bit;
	_func_enter_;
	Adapter->MCAddrCount = MCListlen/6;

	_memcpy( Adapter->MCList, MCListbuf, MCListlen );
	for( i = 0 ; i < 8; i++ ){ MCReg[i] = 0; }

	for( i = 0 ; i < Adapter->MCAddrCount; i++ ){
		if( i< MAX_MCAST_LIST_NUM ){
			GetMulticastBit( Adapter->MCList[i], &Byte, &Bit );
			MCReg[Byte] |= Bit;
		}
	}
// ----------MgntInfo----------------------
/*
	if( pMgntInfo->mActingAsAp || bAcceptAllMulticast ){
		_MAR[0] = 0xffffffff;
		_MAR[1] = 0xffffffff;
	} else {
		_MAR[0] = 0;
		_MAR[1] = 0;

		for (i = 0 ; i < 4; i++)	{
			_MAR[0] = _MAR[0] + ((MCReg[i])<<(8*i));
		}
		
		for (i = 4; i < 8; i++)	{
			_MAR[1] = _MAR[1] + ((MCReg[i])<<(8*(i-4)));
		}
	}
*/
	//3TO DO here...
	//3Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MULTICAST_REG, (pUCHAR)(&MCR[0]) );
	/*! \todo Write MAR into 8711's Multicast Registers. */
	_func_exit_;
}


NDIS_STATUS
drv_set_info(
	IN	_nic_hdl		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	u32		InformationBufferLength,
	OUT	u32*	BytesRead,
	OUT	u32*	BytesNeeded
	)
{
	struct oid_par_priv		oid_par;
	u32 						i;

	
	PADAPTER				Adapter = (PADAPTER)MiniportAdapterContext;
	u8 			offset = (u8)(Oid & 0xFF);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;


	*BytesRead = 0;

	oid_par.adapter_context = MiniportAdapterContext;
	oid_par.oid = Oid;
	oid_par.information_buf = InformationBuffer;
	oid_par.information_buf_len = InformationBufferLength;
	oid_par.bytes_rw = BytesRead;
	oid_par.bytes_needed = BytesNeeded;
	oid_par.type_of_oid = SET_OID;		
	oid_par.dbg = 0;


	_func_enter_;
	if(Adapter->bSurpriseRemoved){
		DEBUG_ERR(("drv_set_info:Adapter->bSurpriseRemoved"));
		goto exit;
	}
		
	for(i = 0; i < sizeof(oid_funs_array)/sizeof(struct oid_funs_node) ; i++)
	{	
	
		//If this segment of OID
		if(Oid >= oid_funs_array[i].oid_start && Oid <= oid_funs_array[i].oid_end)
		{			
		
			offset = (u8)(Oid - oid_funs_array[i].oid_start);

			//If offset is inside the boundary of this corresponding array
			if(offset < oid_funs_array[i].array_sz/sizeof(struct oid_obj_priv))
			{
				oid_par.dbg = oid_funs_array[i].node_array[offset].dbg; //Whether to print debug message
				Status = oid_funs_array[i].node_array[offset].oidfuns(&oid_par);	//OID handler function call
				oid_funs_array[i].set_counter++; // Count set hit numbers for each segment 
				break;			
			}	
			else
			{
				DEBUG_ERR(("Non-Executable OID\n"));
				break;
			}
			
			break;
		}
		
	}




	switch(Oid & 0xFFFFFF00)	
	{

		
#ifdef CONFIG_MP_INCLUDED	

		case OID_MP_SEG1:			
		case OID_MP_SEG2:					
		case OID_MP_SEG3:			
		case OID_MP_SEG4:	
		
			Status=mp_set_info(Adapter,//MPPerformSetInformation(Adapter,
						                           Oid,
						                           (u8*)InformationBuffer,
						                           (u32)InformationBufferLength,
						                           BytesRead,
						                           BytesNeeded
						                           );
			break;			

#endif

	


			
	}
exit:
	_func_exit_;
	return Status;
}


