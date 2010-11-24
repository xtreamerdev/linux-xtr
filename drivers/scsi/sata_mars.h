#define MARS_SATA_GAP       	4
#define	MARS_BANK_GAP			0x100
#define MARS_SATA0_BASE     	0xb8012b00
#define MARS_SATA1_BASE     	MARS_SATA0_BASE+MARS_BANK_GAP
#define SATA_PORT_COUNT     	2
#define MARS_SATA_DEV_ONLINE	3

/*===== Mars SATA register =========*/

/*Port0 register*/

#define     SATA0_CDR0		MARS_SATA0_BASE+0x00		/*0xb8012b00*/
#define     SATA0_CDR1      MARS_SATA0_BASE+0x04        /*0xb8012b04*/
#define     SATA0_CDR2      MARS_SATA0_BASE+0x08        /*0xb8012b08*/
#define     SATA0_CDR3      MARS_SATA0_BASE+0x0c        /*0xb8012b0c*/
#define     SATA0_CDR4      MARS_SATA0_BASE+0x10        /*0xb8012b10*/
#define     SATA0_CDR5      MARS_SATA0_BASE+0x14        /*0xb8012b14*/
#define     SATA0_CDR6      MARS_SATA0_BASE+0x18        /*0xb8012b18*/
#define     SATA0_CDR7      MARS_SATA0_BASE+0x1c        /*0xb8012b1c*/
#define     SATA0_CLR0      MARS_SATA0_BASE+0x20        /*0xb8012b20*/
#define     SATA0_SCR0      MARS_SATA0_BASE+0x24        /*0xb8012b24*/
#define     SATA0_SCR1      MARS_SATA0_BASE+0x28        /*0xb8012b28*/
#define     SATA0_SCR2      MARS_SATA0_BASE+0x2c        /*0xb8012b2c*/
#define     SATA0_SCR3      MARS_SATA0_BASE+0x30        /*0xb8012b30*/
#define     SATA0_SCR4      MARS_SATA0_BASE+0x34        /*0xb8012b34*/
#define     SATA0_SCR5      MARS_SATA0_BASE+0x38        /*0xb8012b38*/
#define     SATA0_DMACTRL   MARS_SATA0_BASE+0xd0        /*0xb8012bd0*/
#define     SATA0_PRDPTR    MARS_SATA0_BASE+0xd4        /*0xb8012bd4*/
#define     SATA0_PRDETY0   MARS_SATA0_BASE+0xd8        /*0xb8012bd8*/
#define     SATA0_PRDETY1   MARS_SATA0_BASE+0xdc        /*0xb8012bdc*/
#define     SATA0_ENABLE    MARS_SATA0_BASE+0xe0        /*0xb8012be0*/
#define     SATA0_DMAMISC   MARS_SATA0_BASE+0xe4        /*0xb8012be4*/
#define     SATA0_MDIOCTRL  MARS_SATA0_BASE+0xe8        /*0xb8012be8*/
#define     SATA0_DMACRC    MARS_SATA0_BASE+0xec        /*0xb8012bec*/
#define     SATA0_PINOUT    MARS_SATA0_BASE+0xf0        /*0xb8012bf0*/
#define     SATA0_TIMEOUT   MARS_SATA0_BASE+0xf4        /*0xb8012bf4*/
#define     SATA0_DBG       MARS_SATA0_BASE+0xf8        /*0xb8012bf8*/
#define     SATA0_DUMMY     MARS_SATA0_BASE+0xfc        /*0xb8012bfc*/

#define     SATA0_FPTAGR    MARS_SATA0_BASE+0x64        /*0xb8012b64*/
#define     SATA0_FPBOR     MARS_SATA0_BASE+0x68        /*0xb8012b68*/
#define     SATA0_FPTCR     MARS_SATA0_BASE+0x6c        /*0xb8012b6c*/
#define     SATA0_DMACR     MARS_SATA0_BASE+0x70        /*0xb8012b70*/
#define     SATA0_DBTSR     MARS_SATA0_BASE+0x74        /*0xb8012b74*/
#define     SATA0_INTPR     MARS_SATA0_BASE+0x78        /*0xb8012b78*/
#define     SATA0_INTMR     MARS_SATA0_BASE+0x7c        /*0xb8012b7c*/
#define     SATA0_ERRMR     MARS_SATA0_BASE+0x80        /*0xb8012b80*/
#define     SATA0_LLCR      MARS_SATA0_BASE+0x84        /*0xb8012b84*/

#define     SATA0_PHYCR     MARS_SATA0_BASE+0x88        /*0xb8012b88*/
#define     SATA0_PHYSR     MARS_SATA0_BASE+0x8c        /*0xb8012b8c*/

#define     SATA0_RXBISTPD  MARS_SATA0_BASE+0x90        /*0xb8012b90*/
#define     SATA0_RXBISTD1  MARS_SATA0_BASE+0x94        /*0xb8012b94*/
#define     SATA0_RXBISTD2  MARS_SATA0_BASE+0x98        /*0xb8012b98*/
#define     SATA0_TXBISTPD  MARS_SATA0_BASE+0x9c        /*0xb8012b9c*/
#define     SATA0_TXBISTD1  MARS_SATA0_BASE+0xa0        /*0xb8012ba0*/
#define     SATA0_TXBISTD2  MARS_SATA0_BASE+0xa4        /*0xb8012ba4*/
#define     SATA0_BISTCR    MARS_SATA0_BASE+0xa8        /*0xb8012ba8*/
#define     SATA0_BISTFCTR  MARS_SATA0_BASE+0xac        /*0xb8012bac*/
#define     SATA0_BISTSR    MARS_SATA0_BASE+0xb0        /*0xb8012bb0*/
#define     SATA0_BISTDECR  MARS_SATA0_BASE+0xb4        /*0xb8012bb4*/

/*=======================================*/
/*/Port1 register*/

#define     SATA1_CDR0      MARS_SATA1_BASE+0x00         /*0xb8012c00*/
#define     SATA1_CDR1      MARS_SATA1_BASE+0x04         /*0xb8012c04*/
#define     SATA1_CDR2      MARS_SATA1_BASE+0x08         /*0xb8012c08*/
#define     SATA1_CDR3      MARS_SATA1_BASE+0x0c         /*0xb8012c0c*/
#define     SATA1_CDR4      MARS_SATA1_BASE+0x10         /*0xb8012c10*/
#define     SATA1_CDR5      MARS_SATA1_BASE+0x14         /*0xb8012c14*/
#define     SATA1_CDR6      MARS_SATA1_BASE+0x18         /*0xb8012c18*/
#define     SATA1_CDR7      MARS_SATA1_BASE+0x1c         /*0xb8012c1c*/
#define     SATA1_CLR0      MARS_SATA1_BASE+0x20         /*0xb8012c20*/
#define     SATA1_SCR0      MARS_SATA1_BASE+0x24         /*0xb8012c24*/
#define     SATA1_SCR1      MARS_SATA1_BASE+0x28         /*0xb8012c28*/
#define     SATA1_SCR2      MARS_SATA1_BASE+0x2c         /*0xb8012c2c*/
#define     SATA1_SCR3      MARS_SATA1_BASE+0x30         /*0xb8012c30*/
#define     SATA1_SCR4      MARS_SATA1_BASE+0x34         /*0xb8012c34*/
#define     SATA1_SCR5      MARS_SATA1_BASE+0x38         /*0xb8012c38*/
#define     SATA1_DMACTRL   MARS_SATA1_BASE+0xd0         /*0xb8012cd0*/
#define     SATA1_PRDPTR    MARS_SATA1_BASE+0xd4         /*0xb8012cd4*/
#define     SATA1_PRDETY0   MARS_SATA1_BASE+0xd8         /*0xb8012cd8*/
#define     SATA1_PRDETY1   MARS_SATA1_BASE+0xdc         /*0xb8012cdc*/
#define     SATA1_ENABLE    MARS_SATA1_BASE+0xe0         /*0xb8012ce0*/
#define     SATA1_DMAMISC   MARS_SATA1_BASE+0xe4         /*0xb8012ce4*/
#define     SATA1_MDIOCTRL  MARS_SATA1_BASE+0xe8         /*0xb8012ce8*/
#define     SATA1_DMACRC    MARS_SATA1_BASE+0xec         /*0xb8012cec*/
#define     SATA1_PINOUT    MARS_SATA1_BASE+0xf0         /*0xb8012cf0*/
#define     SATA1_TIMEOUT   MARS_SATA1_BASE+0xf4         /*0xb8012cf4*/
#define     SATA1_DBG       MARS_SATA1_BASE+0xf8         /*0xb8012cf8*/
#define     SATA1_DUMMY     MARS_SATA1_BASE+0xfc         /*0xb8012cfc*/

#define     SATA1_FPTAGR    MARS_SATA1_BASE+0x64         /*0xb8012c64*/
#define     SATA1_FPBOR     MARS_SATA1_BASE+0x68         /*0xb8012c68*/
#define     SATA1_FPTCR     MARS_SATA1_BASE+0x6c         /*0xb8012c6c*/
#define     SATA1_DMACR     MARS_SATA1_BASE+0x70         /*0xb8012c70*/
#define     SATA1_DBTSR     MARS_SATA1_BASE+0x74         /*0xb8012c74*/
#define     SATA1_INTPR     MARS_SATA1_BASE+0x78         /*0xb8012c78*/
#define     SATA1_INTMR     MARS_SATA1_BASE+0x7c         /*0xb8012c7c*/
#define     SATA1_ERRMR     MARS_SATA1_BASE+0x80         /*0xb8012c80*/
#define     SATA1_LLCR      MARS_SATA1_BASE+0x84         /*0xb8012c84*/

#define     SATA1_PHYCR     MARS_SATA1_BASE+0x88         /*0xb8012c88*/
#define     SATA1_PHYSR     MARS_SATA1_BASE+0x8c         /*0xb8012c8c*/

#define     SATA1_RXBISTPD  MARS_SATA1_BASE+0x90         /*0xb8012c90*/
#define     SATA1_RXBISTD1  MARS_SATA1_BASE+0x94         /*0xb8012c94*/
#define     SATA1_RXBISTD2  MARS_SATA1_BASE+0x98         /*0xb8012c98*/
#define     SATA1_TXBISTPD  MARS_SATA1_BASE+0x9c         /*0xb8012c9c*/
#define     SATA1_TXBISTD1  MARS_SATA1_BASE+0xa0         /*0xb8012ca0*/
#define     SATA1_TXBISTD2  MARS_SATA1_BASE+0xa4         /*0xb8012ca4*/
#define     SATA1_BISTCR    MARS_SATA1_BASE+0xa8         /*0xb8012ca8*/
#define     SATA1_BISTFCTR  MARS_SATA1_BASE+0xac         /*0xb8012cac*/
#define     SATA1_BISTSR    MARS_SATA1_BASE+0xb0         /*0xb8012cb0*/
#define     SATA1_BISTDECR  MARS_SATA1_BASE+0xb4         /*0xb8012cb4*/

/*==========================================*/
/*===== CRT releative register =============*/
/*==========================================*/
#define		CRT_SOFT_RESET1 	0xB8000000
#define 	CRT_CLOCK_ENABLE1	0xB800000C

#define 	SATA_PORT0_CLK_EN	0x04
#define 	SATA_PORT1_CLK_EN	0x08
#define 	SATA_RST_MAC0		0x04
#define 	SATA_RST_MAC1		0x08
#define 	SATA_RST_PHY0		0x10
#define 	SATA_RST_PHY1		0x20

/*==========================================*/
/*===== SB1 releative register =============*/
/*==========================================*/
#define 	SB1_CTRL			0xB801C200
#define 	SB1_PRIORITY1		0xB801C204
#define 	SB1_PRIORITY2		0xB801C208
#define 	SB1_PRIORITY3		0xB801C20C
#define 	SB1_PRIORITY4		0xB801C210
#define 	SB1_PRIORITY5		0xB801C214
#define 	SB1_MASK			0xB801C218

#define 	SB1_MASK_SATA0		0x08
#define 	SB1_MASK_SATA1		0x10

/*=================================*/
/*===== SATA PHY register =========*/
/*=================================*/
#define     SATA_PHY_PCR        0x00
#define     SATA_PHY_RCR0       0x01
#define     SATA_PHY_RCR1       0x02
#define     SATA_PHY_RCR2       0x03
#define     SATA_PHY_RTCR       0x04
#define     SATA_PHY_TPCR       0x05
#define     SATA_PHY_TCR0       0x06
#define     SATA_PHY_TCR1       0x07
#define     SATA_PHY_SDCR       0x08
#define     SATA_PHY_IMR        0x09
#define     SATA_PHY_BPCR       0x0a
#define     SATA_PHY_MainCfg0   0x10
#define     SATA_PHY_MainCfg1   0x11
#define     SATA_PHY_BistCfg0   0x12
#define     SATA_PHY_BistCfg1   0x13
#define     SATA_PHY_PattCfg0   0x14
#define     SATA_PHY_PattCfg1   0x15
#define     SATA_PHY_PrimCfg1   0x16
#define     SATA_PHY_ErrStatus0 0x17
#define     SATA_PHY_ErrStatus1 0x18
#define     SATA_PHY_ErrStatus2 0x19
#define     SATA_PHY_ErrStatus3 0x1a
#define     SATA_PHY_ErrStatus4 0x1b


/* SATA MAC CONTROL */
#define     SCR2_VOL 		0x0310	//300:no speed restrictions; 310: limit on G1; 320: limit on G2
#define		SCR2_ACT_COMMU	0x0001
#define     TIMEOUT_VOL 	0x0000
#define     PINOUT_VOL 		0x020f
#define     ENABLE_VOL 		0x0001

#define     SCR1_ERR_I 		(1<<0)
#define     SCR1_ERR_M 		(1<<1)
#define     SCR1_ERR_T 		(1<<8)
#define     SCR1_ERR_C 		(1<<9)
#define     SCR1_ERR_P 		(1<<10)
#define     SCR1_ERR_E 		(1<<11)
#define     SCR1_DIAG_N		(1<<16)
#define     SCR1_DIAG_I 	(1<<17)
#define     SCR1_DIAG_W 	(1<<18)
#define     SCR1_DIAG_B 	(1<<19)
#define     SCR1_DIAG_D 	(1<<20)
#define     SCR1_DIAG_C 	(1<<21)
#define     SCR1_DIAG_H 	(1<<22)
#define     SCR1_DIAG_S 	(1<<23)
#define     SCR1_DIAG_T 	(1<<24)
#define     SCR1_DIAG_F 	(1<<25)
#define     SCR1_DIAG_X 	(1<<26)
#define     SCR1_DIAG_A 	(1<<27)

/* MDIO CONTROL */
#ifdef HEAT_DOWN12p5
	#define	BPCR_VAL 		0x6900
#endif
#ifdef HEAT_DOWN25
	#define	BPCR_VAL 		0x6d00
#endif

	#define	TCR0_VAL 		0x623C
	#define	RCR0_VAL 		0x7C65

#ifdef EN_SSC
	#define	SDCR_VAL 		0x0018
#endif

	#define	PCR_VAL			0x3020
	#define	RCR1_VAL 		0x12DE
	#define	RCR2_VAL 		0xC2B8
	#define	TCR1_VAL 		0x2900

#define	RCR1_SATA_EN 		0x0040

/*==================================================
 local function list
===================================================*/
void mars_ata_dma_setup(struct ata_queued_cmd *qc);
void mars_ata_dma_start(struct ata_queued_cmd *qc);
u8 mars_ata_dma_status(struct ata_port *ap);
void mars_ata_dma_stop(struct ata_queued_cmd *qc);
void mars_ata_dma_irq_clear(struct ata_port *ap);

#ifdef CONFIG_MARS_PM
int generic_sata_suspend(struct device *dev, u32 state);
int generic_sata_resume(struct device *dev);
#endif

/*==================================================
 externl function list
====================================================*/
