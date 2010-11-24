
#ifndef _R8187_MP_IOCTL_LINUX_H
#define _R8187_MP_IOCTL_LINUX_H


#define GEN_MP_IOCTL_SUBCODE(code) _MP_IOCTL_ ## code ## _CMD_

enum RTL871X_MP_IOCTL_SUBCODE{
	GEN_MP_IOCTL_SUBCODE(MP_START), 		/*0*/
	GEN_MP_IOCTL_SUBCODE(MP_STOP), 		/*1*/	
	GEN_MP_IOCTL_SUBCODE(SET_CHANNEL_MODULATION),
	GEN_MP_IOCTL_SUBCODE(SET_TXPOWER),
	GEN_MP_IOCTL_SUBCODE(SET_DATARATE),
	GEN_MP_IOCTL_SUBCODE(READ_REG),
	GEN_MP_IOCTL_SUBCODE(WRITE_REG),
	GEN_MP_IOCTL_SUBCODE(READ_BB_REG),	
	GEN_MP_IOCTL_SUBCODE(WRITE_BB_REG),	
	GEN_MP_IOCTL_SUBCODE(READ_RF_REG),
	GEN_MP_IOCTL_SUBCODE(WRITE_RF_REG),
	GEN_MP_IOCTL_SUBCODE(READ16_EEPROM),	
	GEN_MP_IOCTL_SUBCODE(WRITE16_EEPROM),
	GEN_MP_IOCTL_SUBCODE(SET_CONTINUOUS_TX),
	GEN_MP_IOCTL_SUBCODE(SET_SINGLE_CARRIER),
	GEN_MP_IOCTL_SUBCODE(GET_PACKETS_RX),
	
	MAX_MP_IOCTL_SUBCODE,
};


enum MP_MODE{
	MP_START_MODE,
       MP_STOP_MODE,
	MP_ERR_MODE
};
	

struct rwreg_param{
	unsigned int offset;
	unsigned int width;
	unsigned int value;
};

struct bbreg_param{
	unsigned int offset;
	unsigned int phymask;
	unsigned int value;
};

struct rfchannel_param{
	unsigned int	ch;
	unsigned int	modem;
};


struct txpower_param{
	unsigned int pwr_index;
};


struct datarate_param{
	unsigned int rate_index;
};


struct eeprom_rw_param{
	unsigned int offset;
	unsigned short value;
};


struct continuous_tx_param{
	unsigned int mode;
};

struct packets_rx_param{
	unsigned int rxok;
	unsigned int rxerr;
};

struct mp_ioctl_handler{
	unsigned int paramsize;
	int (*handler)(struct net_device *dev, unsigned char  set, void *param);	
};


extern int mp_ioctl_start_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_stop_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_set_ch_modulation_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_set_txpower_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_read_reg_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_write_reg_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_set_datarate_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_read_bbreg_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_write_bbreg_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_read_rfreg_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_write_rfreg_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_read16_eeprom_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_write16_eeprom_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_set_continuous_tx_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_set_single_carrier_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_get_packets_rx_hdl(struct net_device *dev, unsigned char set, void*param);


#define GEN_MP_IOCTL_HANDLER(sz, subcode) {sz, &mp_ioctl_ ## subcode ## _hdl},

struct mp_ioctl_handler mp_ioctl_hdl[] = {

       GEN_MP_IOCTL_HANDLER(sizeof(unsigned int), start)
	GEN_MP_IOCTL_HANDLER(sizeof(unsigned int), stop)

	GEN_MP_IOCTL_HANDLER(sizeof(struct rfchannel_param), set_ch_modulation)
	GEN_MP_IOCTL_HANDLER(sizeof(struct txpower_param), set_txpower)
	GEN_MP_IOCTL_HANDLER(sizeof(struct datarate_param), set_datarate)

	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), read_reg)
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), write_reg)
	
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), read_bbreg)
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), write_bbreg)
	
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), read_rfreg)
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), write_rfreg)

	GEN_MP_IOCTL_HANDLER(sizeof(struct eeprom_rw_param), read16_eeprom)
	GEN_MP_IOCTL_HANDLER(sizeof(struct eeprom_rw_param), write16_eeprom)

	GEN_MP_IOCTL_HANDLER(sizeof(struct continuous_tx_param), set_continuous_tx)
	GEN_MP_IOCTL_HANDLER(sizeof(struct continuous_tx_param), set_single_carrier)
	GEN_MP_IOCTL_HANDLER(sizeof(struct packets_rx_param), get_packets_rx)

};


struct mp_ioctl_param{
	unsigned int subcode;
	unsigned int len;
	unsigned char data[0];
};


	

#endif
