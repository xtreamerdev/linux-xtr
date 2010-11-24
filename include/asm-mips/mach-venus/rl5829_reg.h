#ifndef _RL5829_REG_H_
#define _RL5829_REG_H_

// porting by cfyeh : 2008/06/02
// just copy file

#define USBPHY_20                            0xC0
#define USBPHY_21                            0xC1
#define USBPHY_22                            0xC2
#define USBPHY_23                            0xC3
#define USBPHY_24                            0xC4
#define USBPHY_25                            0xC5
#define USBPHY_26                            0xC6
#define USBPHY_27                            0xC7

#define USBPHY_28                            0xD0
#define USBPHY_29                            0xD1
#define USBPHY_2A                            0xD2
#define USBPHY_2B                            0xD3


#define USBPHY_30                            0xE0
#define USBPHY_30_set_xcvr_autok(data)       (0x80&((data)<<7))
#define USBPHY_30_set_xcvr_SC(data)          (0x78&((data)<<3))
#define USBPHY_30_set_xcvr_CP(data)          (0x07&(data))

#define USBPHY_31                            0xE1
#define USBPHY_31_set_xcvr_call_host(data)   (0x80&((data)<<7))
#define USBPHY_31_set_xcvr_zeres_sel(data)   (0x40&((data)<<6))
#define USBPHY_31_set_xcvr_zo_en(data)       (0x20&((data)<<5))
#define USBPHY_31_set_xcvr_SD(data)          (0x18&((data)<<3))
#define USBPHY_31_set_xcvr_SR(data)          (0x07&(data))

#define USBPHY_32                            0xE2
#define USBPHY_32_set_SEN(data)              (0xF0&((data)<<4))
#define USBPHY_32_set_SH(data)               (0x0F&(data))

#define USBPHY_33                            0xE3
#define USBPHY_33_set_HSXMPTPDEN(data)       (0x80&((data)<<7))
#define USBPHY_33_set_xcvr_CAL(data)         (0x40&((data)<<6))
#define USBPHY_33_set_xcvr_DB(data)          (0x38&((data)<<3))
#define USBPHY_33_set_xcvr_DR(data)          (0x07&(data))

#define USBPHY_34                            0xE4
#define USBPHY_34_set_xcvr_TS(data)          (0x38&((data)<<3))
// venus, neptune
#define USBPHY_34_set_TPA_EN(data)           (0x80&((data)<<7))
#define USBPHY_34_set_TPB_EN(data)           (0x40&((data)<<6))
#define USBPHY_34_set_xcvr_SE(data)          (0x07&(data))
// mars
#define USBPHY_34_set_DBG_EN(data)           (0x80&((data)<<7))

#define USBPHY_35                            0xE5
#define USBPHY_35_set_xcvr_SRC(data)         (0x1C&((data)<<2))
#define USBPHY_35_set_HSTESTEN(data)         (0x02&((data)<<1))
#define USBPHY_35_set_xcvr_nsqdly(data)      (0x01&(data))
// venus, neptune
#define USBPHY_35_set_TPC_EN(data)           (0x80&((data)<<7))
#define USBPHY_35_set_xcvr_SP(data)          (0x60&((data)<<5))
#define USBPHY_35_set_HSTESTEN(data)         (0x02&((data)<<1))
// mars
#define USBPHY_35_set_SE0_LVL(data)          (0x80&((data)<<7))
#define USBPHY_35_set_Force_XTL_ON(data)     (0x02&((data)<<1))

#define USBPHY_36                            0xE6
#define USBPHY_36_set_xcvr_senh(data)        (0xF0&((data)<<4))
#define USBPHY_36_set_xcvr_adjr(data)        (0x0F&(data))

#define USBPHY_37                            0xE7
#define USBPHY_37_set_LDO(data)              (0x38&((data)<<3))
#define USBPHY_37_set_LDO_TN(data)           (0x06&((data)<<1))
#define USBPHY_37_set_PLL_TEST_EN(data)      (0x01&(data))

#define USBPHY_38                            0xF0
#define USBPHY_38_set_dbnc_en(data)          (0x80&((data)<<7))
#define USBPHY_38_set_discon_enable(data)    (0x40&((data)<<6))
#define USBPHY_38_set_EN_ERR_UNDERRUN(data)  (0x20&((data)<<5))
#define USBPHY_38_set_LATE_DLLEN(data)       (0x10&((data)<<4))
#define USBPHY_38_set_INTG(data)             (0x08&((data)<<3))
#define USBPHY_38_set_SOP_KK(data)           (0x04&((data)<<2))
#define USBPHY_38_set_SLB_INNER(data)        (0x02&((data)<<1))
#define USBPHY_38_set_SLB_EN(data)           (0x01&(data))


#define USBPHY_39                            0xF1
#define USBPHY_39_set_utmi_pos_out(data)     (0x80&((data)<<7))
#define USBPHY_39_set_slb_rst(data)					 (0x40&((data)<<6))
#define USBPHY_39_set_slb_sel(data)          (0x30&((data)<<4)) 							
#define USBPHY_39_set_auto_sel(data)         (0x08&((data)<<3))
#define USBPHY_39_set_tx_delay(data)         (0x06&((data)<<1))
#define USBPHY_39_set_slb_fs(data)           (0x01&(data))


#define USBPHY_3A                            0xF2
#define USBPHY_3A_set_force_xcvrsel(data)    (0x80&((data)<<7))
#define USBPHY_3A_set_xcvsel_mode(data)      (0x60&((data)<<5))
#define USBPHY_3A_set_force_termsel(data)    (0x10&((data)<<4))
#define USBPHY_3A_set_termsel_mode(data)     (0x08&((data)<<3))
#define USBPHY_3A_set_force_opmode(data)     (0x04&((data)<<2))
#define USBPHY_3A_set_opmode_mode(data)      (0x03&(data))


#define USBPHY_3B                            0xF3
#define USBPHY_3B_set_force_serial(data)     (0x80&((data)<<7))
#define USBPHY_3B_set_serial_mode(data)      (0x40&((data)<<6))
#define USBPHY_3B_set_force_suspendm(data)   (0x20&((data)<<5))
#define USBPHY_3B_set_suspendm_mode(data)    (0x10&((data)<<4))
#define USBPHY_3B_set_force_txse0(data)      (0x08&((data)<<3))
#define USBPHY_3B_set_txse0_mode(data)       (0x04&((data)<<2))
#define USBPHY_3B_set_force_txenable_n(data) (0x02&((data)<<1))
#define USBPHY_3B_set_txenable_n_mode(data)  (0x01&(data))


#endif
