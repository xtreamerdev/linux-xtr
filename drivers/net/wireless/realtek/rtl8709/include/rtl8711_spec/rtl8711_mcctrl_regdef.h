
#ifndef __RTL8711_MCCTRL_REGDEF_H__
#define __RTL8711_MCTRL_REGDEF_H__

#define	SMCR	(RTL8711_MCCTRL_ + 0x0000)	// System Memory Controller Register
#define	NACR	(RTL8711_MCCTRL_ + 0x0B04) 	// NAND flash controller register
#define NACAR	(RTL8711_MCCTRL_ + 0x0B08)	// NAND Flash Command Register
#define	NAADDR	(RTL8711_MCCTRL_ + 0x0B0C) 	// NAND flash address register
#define	NADC	(RTL8711_MCCTRL_ + 0x0B10) 	// NAND flash DMA controller register
#define	NADR	(RTL8711_MCCTRL_ + 0x0B14)	// NAND Flash Data Register
#define	NADSAF	(RTL8711_MCCTRL_ + 0x0B18) 	// NAND flash DMA start address in flash
#define	NADSAS	(RTL8711_MCCTRL_ + 0x0B1C) 	// NAND flash DMA start address in SRAM
#define NASR	(RTL8711_MCCTRL_ + 0x0B20) 	// NAND Flash Status Register
#define	NOCR	(RTL8711_MCCTRL_ + 0x0B24) 	// NOR Flash Controller Register
#define	NOCTR0	(RTL8711_MCCTRL_ + 0x0B28) 	// NOR Flash Controller Timing Register 0

#endif
