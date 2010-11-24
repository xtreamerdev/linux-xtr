#ifndef __RTD2830_PRIV_H__
#define __RTD2830_PRIV_H__

struct rtd2830_reg_addr{
    int                page_addr;
    int                reg_addr;
    int                bit_low;
    int                bit_high;
};

#define RTD2830_PAGE_REG            0x00

#define RTD2830_PAGE_0              0x00
#define RTD2830_PAGE_1              0x01
#define RTD2830_PAGE_2              0x02
#define RTD2830_PAGE_3              0x03    
#define RTD2830_PAGE_4              0x04    
#define RTD2831U_USB                0x05
#define RTD2831U_SYS                0x06



typedef enum{
    RTD2830_RMAP_INDEX_BW_INDEX                         = 0x00, // RTD2830_PAGE_0, 0x08, 1, 2
    RTD2830_RMAP_INDEX_SAMPLE_RATE_INDEX                      , // RTD2830_PAGE_0, 0x08, 3, 3
    RTD2830_RMAP_INDEX_PFREQ1_0                               , // RTD2830_PAGE_0, 0x0D, 0, 1
    RTD2830_RMAP_INDEX_PD_DA8                                 , // RTD2830_PAGE_0, 0x0D, 4, 4
    RTD2830_RMAP_INDEX_SOFT_RESET                             , // RTD2830_PAGE_1, 0x01, 2, 2
    RTD2830_RMAP_INDEX_LOOP_GAIN0_3                           , // RTD2830_PAGE_1, 0x04, 1, 4
    RTD2830_RMAP_INDEX_LOCK_TH                                , // RTD2830_PAGE_1, 0x10, 0, 1
    RTD2830_RMAP_INDEX_SPEC_INV                               , // RTD2830_PAGE_1, 0x15, 0, 0
    RTD2830_RMAP_INDEX_H_LPF_X                                , // RTD2830_PAGE_1, 0x1C, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_0                                , // RTD2830_PAGE_1, 0x1C, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_1                                , // RTD2830_PAGE_1, 0x1E, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_2                                , // RTD2830_PAGE_1, 0x20, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_3                                , // RTD2830_PAGE_1, 0x22, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_4                                , // RTD2830_PAGE_1, 0x24, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_5                                , // RTD2830_PAGE_1, 0x26, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_6                                , // RTD2830_PAGE_1, 0x28, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_7                                , // RTD2830_PAGE_1, 0x2A, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_8                                , // RTD2830_PAGE_1, 0x2C, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_9                                , // RTD2830_PAGE_1, 0x2E, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_10                               , // RTD2830_PAGE_1, 0x30, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_11                               , // RTD2830_PAGE_1, 0x32, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_12                               , // RTD2830_PAGE_1, 0x34, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_13                               , // RTD2830_PAGE_1, 0x36, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_14                               , // RTD2830_PAGE_1, 0x38, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_15                               , // RTD2830_PAGE_1, 0x3A, 0, 15
    RTD2830_RMAP_INDEX_H_LPF_16                               , // RTD2830_PAGE_1, 0x3C, 0, 15
    RTD2830_RMAP_INDEX_TRK_KC_P1                              , // RTD2830_PAGE_1, 0x73, 0, 2
    RTD2830_RMAP_INDEX_TRK_KC_P2                              , // RTD2830_PAGE_1, 0x73, 3, 5
    RTD2830_RMAP_INDEX_TRK_KC_I1                              , // RTD2830_PAGE_1, 0x74, 3, 5
    RTD2830_RMAP_INDEX_TRK_KC_I2                              , // RTD2830_PAGE_1, 0x75, 0, 2
    RTD2830_RMAP_INDEX_CKOUT_PWR                              , // RTD2830_PAGE_1, 0x7B, 6, 6
    RTD2830_RMAP_INDEX_BER_PASS_SCAL                          , // RTD2830_PAGE_1, 0x8C, 6, 11
    RTD2830_RMAP_INDEX_TR_WAIT_MIN_8K                         , // RTD2830_PAGE_1, 0x88, 2, 11
    RTD2830_RMAP_INDEX_RSD_BER_FAIL_VAL                       , // RTD2830_PAGE_1, 0x8F, 0, 15
    RTD2830_RMAP_INDEX_MGD_THD0                               , // RTD2830_PAGE_1, 0x95, 0, 7
    RTD2830_RMAP_INDEX_MGD_THD1                               , // RTD2830_PAGE_1, 0x96, 0, 7
    RTD2830_RMAP_INDEX_MGD_THD2                               , // RTD2830_PAGE_1, 0x97, 0, 7
    RTD2830_RMAP_INDEX_MGD_THD3                               , // RTD2830_PAGE_1, 0x98, 0, 7
    RTD2830_RMAP_INDEX_MGD_THD4                               , // RTD2830_PAGE_1, 0x99, 0, 7
    RTD2830_RMAP_INDEX_MGD_THD5                               , // RTD2830_PAGE_1, 0x9A, 0, 7
    RTD2830_RMAP_INDEX_MGD_THD6                               , // RTD2830_PAGE_1, 0x9B, 0, 7
    RTD2830_RMAP_INDEX_MGD_THD7                               , // RTD2830_PAGE_1, 0x9C, 0, 7
    RTD2830_RMAP_INDEX_ADD_WRITE                              , // RTD2830_PAGE_1, 0x9D, 0, 47 // @FIXME: not longer than 31
    RTD2830_RMAP_INDEX_CE_FFSM_BYPASS                         , // RTD2830_PAGE_2, 0xD5, 1, 1
    RTD2830_RMAP_INDEX_ALPHAIIR_N                             , // RTD2830_PAGE_2, 0xF1, 1, 2
    RTD2830_RMAP_INDEX_ALPHAIIR_DIF                           , // RTD2830_PAGE_2, 0xF1, 3, 7
    RTD2830_RMAP_INDEX_FSM_STAGE                              , // RTD2830_PAGE_3, 0x51, 3, 6

    RTD2831_RMAP_INDEX_USB_STAT                               ,//RTD2831U_USB, USB_STAT, 0, 7
    RTD2831_RMAP_INDEX_USB_EPA_CTL                              ,
    RTD2831_RMAP_INDEX_USB_SYSCTL                           ,
    RTD2831_RMAP_INDEX_USB_EPA_CFG                         ,
    RTD2831_RMAP_INDEX_USB_EPA_MAXPKT                   ,
    RTD2831_RMAP_INDEX_SYS_GPD,
    RTD2831_RMAP_INDEX_SYS_GPOE,
    RTD2831_RMAP_INDEX_SYS_GPO,
    RTD2831_RMAP_INDEX_SYS_SYS_0,    
    RTD2830_RMAP_INDEX_SET_IF_FREQ                              , // RTD2830_PAGE_1, 0x9D, 0, 47 // @FIXME: not
} rtd2830_reg_map_index;


// Richard: @FIXME: ONLY for testing. Remove this some time later
#define RTD2830_RMAP_INDEX_PAGE                         (rtd2830_reg_map_index)(0x30)    // RTD2830_PAGE_0, 0x00, 0, 7
                                                        

#define DEMOD_I2C_ADDRESS               0x20
#define TUNER_I2C_ADDRESS_Thomson_if    0x8A

///////////////////////////////////////////USB resigter//////////////////////////////////////////////////////////////////////////
#define USB_BASE_ADDRESS	0x20 
//SIE Control Registers==================================================================
#define USB_SYSCTL			0x0000 		//USB System Control Register 
//0004h Reserved                    	
#define USB_IRQSTAT			0x0008		//SIE Interrupt Status 
#define USB_IRQEN			0x000C		//SIE Interrupt Enable 
#define USB_CTRL			0x0010		//USB Control Register 
#define USB_STAT			0x0014		//USB Status Register 
#define USB_DEVADDR			0x0018		//USB Device Address 
#define USB_TEST			0x001C		//USB Test Mode Register 
#define FRAME_NUMBER		0x0020		//Frame number 
						  //0x0024 		//Reserved                    	
#define USB_FIFO_ADDR		0x0028		//Address of SIE FIFO RAM 
#define USB_FIFO_CMD		0x002A		//SIE FIFO RAM Access Command 
						  //0x002C 		//Reserved                    	
#define USB_FIFO_DATA		0x0030		//SIE FIFO RAM Data Register
						  //0x0034 - 0x00F4 - Reserved 
//Endpoint Registers=====================================================================
#define EP0_SETUPA			0x00F8  	//Endpoint 0 Setup Packet Lower Byte Register 
#define EP0_SETUPB			0x00FC  	//Endpoint 0 Setup Packet Higher Byte Register 
						  //0x0100		//Reserved              	
#define USB_EP0_CFG			0x0104  	//Endpoint 0 Configure Register 
#define USB_EP0_CTL			0x0108  	//Endpoint 0 Control Register 
#define USB_EP0_STAT		0x010C  	//Endpoint 0 Status Register 
#define USB_EP0_IRQSTAT		0x0110  	//Endpoint 0 Interrupt Status Register 
#define USB_EP0_IRQEN		0x0114  	//Endpoint 0 Interrupt Enable Register 
#define USB_EP0_MAXPKT		0x0118  	//Endpoint 0 Max Packet Size Register 
						  //0x011C		//Reserved                  	
#define USB_EP0_BC			0x0120  	//Endpoint 0 FIFO Byte Counter Register 
						  //0x0124 - 0x0140 - Reserved          	
#define USB_EPA_CFG			0x0144  	//Endpoint A Configure Register 
#define USB_EPA_CTL			0x0148  	//Endpoint A Control Register 
#define USB_EPA_STAT		0x014C  	//Endpoint A Status Register 
#define USB_EPA_IRQSTAT		0x0150  	//Endpoint A Interrupt Status Register 
#define USB_EPA_IRQEN		0x0154  	//Endpoint A Interrupt Enable Register 
#define USB_EPA_MAXPKT		0x0158  	//Endpoint A Max Packet Size Register 
						  //0x015C		//Reserved                  	
#define USB_EPA_FIFO_CFG	0x0160  	//Endpoint A FIFO Configure Register 
						  //0x0164 每 0x0F00 - Reserved
//Debug Registers=========================================================================
#define USB_PHYTSTDIS		0x0F04  	//PHY Test Disable 
#define USB_TOUT_VAL		0x0F08  	//USB Time-Out Time 
#define USB_VDRCTRL			0x0F10  	//UTMI Vendor Signal Control Register 
#define USB_VSTAIN			0x0F14  	//UTMI Vendor Signal Status In Register 
#define USB_VLOADM			0x0F18  	//UTMI Load Vendor Signal Status In Register 
#define USB_VSTAOUT			0x0F1C  	//UTMI Vendor Signal Status Out Register 
						  //0x0F20 每 0x0F7F - Reserved                           	
#define USB_UTMI_TST		0x0F80  	//UTMI Test register 
#define USB_UTMI_STATUS		0x0F84  	//UTMI Status Register 
#define USB_TSTCTL			0x0F88  	//Test Control Register 
#define USB_TSTCTL2			0x0F8C  	//Test Control Register 2 
#define USB_PID_FORCE		0x0F90  	//Force PID 
#define USB_PKTERR_CNT		0x0F94  	//Packet Error Counter 
#define USB_RXERR_CNT		0x0F98  	//RX Error Counter 
#define USB_MEM_BIST		0x0F9C  	//MEM BIST Test Register 
#define USB_SLBBIST			0x0FA0  	//Self-Loop-Back BIST register 
#define USB_CNTTEST			0x0FA4  	//Counter Test 
						  //0x0FA8 每 0x0FBF - Reserved                         	
#define USB_PHYTST			0x0FC0  	//USB PHY Test Register 
						  //0x0FC4 每 0x0FEF - Reserved                         	
#define USB_DBGIDX			0x0FF0  	//Select Individual Block Debug Signal 
#define USB_DBGMUX			0x0FF4  	//Debug Signal Module Mux Register 
						  //0x0FF8 - 0x0FFF - Reserved                   
//int usb_register_read(unsigned short offset, unsigned char *data, unsigned short *bytelength);
//int usb_register_write(unsigned short offset, unsigned char *data, unsigned short *bytelength);
//int report_all_usb_registers();
//int report_all_sys_registers();

///////////////////////////////////////////SYS resigter//////////////////////////////////////////////////////////////////////////
#define SYS_BASE_ADDRESS	0x30 //0x3000
//demod control registers===============================================================================
#define SYS_0				0x0000			//include DEMOD_CTL, GPO, GPI, GPOE***************	
#define DEMOD_CTL 			0x0000			//Control register for DVB-T demodulator 
//GPIO registers========================================================================================
#define GPO 				0x0001			//Output value of general purpose I/O 
#define GPI 				0x0002			//Input value of general purpose I/O 
#define GPOE 				0x0003			//Output enable of general purpose I/O 
#define SYS_1				0x0004			//include GPD, SYSINTE, SYSINTS, GP_CFG0**********
#define GPD 				0x0004			//Direction control for general purpose I/O 
#define SYSINTE 			0x0005			//System Interrupt Enable Register 
#define SYSINTS 			0x0006			//System Interrupt Status Register 
#define GP_CFG0 			0x0007			//PAD Configuration for GPIO0-GPIO3 
#define SYS_2				0x0008			//include GP_CFG1 and 3 reserved bytes************	
#define GP_CFG1 			0x0008			//PAD Configuration for GPIO4 
					      //0x0009 - 0x001F - - Reserved
//IrDA registers========================================================================================
#define IRRC_PSR 			0x0020			//IR protocol selection register 
#define IRRC_PER 			0x0024			//IR protocol extension register 
#define IRRC_SF 			0x0028			//IR sampling frequency 
#define IRRC_DPIR 			0x002C			//IR data package interval register 
#define IRRC_CR 			0x0030			//IR control register 
#define IRRC_RP 			0x0034			//IR read port 
#define IRRC_SR 			0x0038			//IR status register 
					      //0x003C - 0x003F - - Reserved
//I2C master registers==================================================================================
#define I2CCR 				0x0040			//I2C clock register 
#define I2CMCR 				0x0044			//I2C master control register 
#define I2CMSTR 			0x0048			//I2C master SCL timing register 
#define I2CMSR 				0x004C			//I2C master status register 
#define I2CMFR 				0x0050			//I2C master FIFO register 
                                                  
#endif // __RTD2830_PRIV_H__
