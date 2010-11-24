#ifndef __I2C_VENUS_H_
#define __I2C_VENUS_H_

#define MIS_PSELL			((volatile unsigned int *)0xb801b004)
#define	MIS_ISR				((volatile unsigned int *)0xb801b00c)
#define IC_CON				((volatile unsigned int *)0xb801b300)
#define IC_TAR				((volatile unsigned int *)0xb801b304)
#define IC_SAR				((volatile unsigned int *)0xb801b308)
#define IC_HS_MADDR			((volatile unsigned int *)0xb801b30c)
#define IC_DATA_CMD			((volatile unsigned int *)0xb801b310)
#define IC_SS_SCL_HCNT		((volatile unsigned int *)0xb801b314)
#define IC_SS_SCL_LCNT		((volatile unsigned int *)0xb801b318)
#define IC_FS_SCL_HCNT		((volatile unsigned int *)0xb801b31c)
#define IC_FS_SCL_LCNT		((volatile unsigned int *)0xb801b320)
#define IC_INTR_STAT		((volatile unsigned int *)0xb801b32c)
#define IC_INTR_MASK		((volatile unsigned int *)0xb801b330)
#define IC_RAW_INTR_STAT	((volatile unsigned int *)0xb801b334)
#define IC_RX_TL			((volatile unsigned int *)0xb801b338)
#define IC_TX_TL			((volatile unsigned int *)0xb801b33c)
#define IC_CLR_INTR			((volatile unsigned int *)0xb801b340)
#define IC_CLR_RX_UNDER		((volatile unsigned int *)0xb801b344)
#define IC_CLR_RX_OVER		((volatile unsigned int *)0xb801b348)
#define IC_CLR_TX_OVER		((volatile unsigned int *)0xb801b34c)
#define IC_CLR_RD_REQ		((volatile unsigned int *)0xb801b350)
#define IC_CLR_TX_ABRT		((volatile unsigned int *)0xb801b354)
#define IC_CLR_RX_DONE		((volatile unsigned int *)0xb801b358)
#define IC_CLR_ACTIVITY		((volatile unsigned int *)0xb801b35c)
#define IC_CLR_STOP_DET		((volatile unsigned int *)0xb801b360)
#define IC_CLR_START_DET	((volatile unsigned int *)0xb801b364)
#define IC_CLR_GEN_CALL		((volatile unsigned int *)0xb801b368)
#define IC_ENABLE			((volatile unsigned int *)0xb801b36c)
#define IC_STATUS			((volatile unsigned int *)0xb801b370)
#define IC_TXFLR			((volatile unsigned int *)0xb801b374)
#define IC_RXFLR			((volatile unsigned int *)0xb801b378)
#define IC_TX_ABRT_SOURCE	((volatile unsigned int *)0xb801b380)
#define IC_DMA_CR			((volatile unsigned int *)0xb801b388)
#define IC_DMA_TDLR			((volatile unsigned int *)0xb801b38c)
#define IC_DMA_RDLR			((volatile unsigned int *)0xb801b390)
#define IC_COMP_PARAM_1		((volatile unsigned int *)0xb801b3f4)
#define IC_COMP_VERSION		((volatile unsigned int *)0xb801b3f8)
#define IC_COMP_TYPE		((volatile unsigned int *)0xb801b3fc)


#define VENUS_I2C_IRQ	3
#define VENUS_MASTER_7BIT_ADDR	0x24


#define GEN_CALL_BIT		0x800
#define START_DET_BIT		0x400
#define STOP_DET_BIT		0x200
#define ACTIVITY_BIT		0x100
#define RX_DONE_BIT		0x080
#define TX_ABRT_BIT		0x040
#define RD_REQ_BIT		0x020
#define TX_EMPTY_BIT		0x010
#define TX_OVER_BIT		0x008
#define RX_FULL_BIT		0x004
#define RX_OVER_BIT		0x002
#define RX_UNDER_BIT		0x001

#endif
