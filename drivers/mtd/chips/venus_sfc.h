#ifndef	VENUS_SERIAL_FLASH_CONTROLLER
#define VENUS_SERIAL_FLASH_CONTROLLER



/*
	for HWSD, the serial flash address started from 0xbfcfffff,
	so for 16Mbit SST Serial Flash, the address ranges from 0xbfb0-0000 - 0xbfcf-ffff
*/
/* 2 MBytes */
#define FLASH_ADDR_END	0xbfd00000
//#define FLASH_SIZE		0x00200000
//#define FLASH_BASE		((volatile unsigned char *)(FLASH_ADDR_END-FLASH_SIZE))


#define MD_PP_DATA_SIZE_SHIFT	8	/* 256 bytes */
#define MD_PP_DATA_SIZE		(1 << MD_PP_DATA_SIZE_SHIFT)

#define SFC_OPCODE		((volatile unsigned int *)0xb801a800)
#define SFC_CTL			((volatile unsigned int *)0xb801a804)
#define SFC_SCK			((volatile unsigned int *)0xb801a808)
#define SFC_CE			((volatile unsigned int *)0xb801a80c)
#define SFC_WP			((volatile unsigned int *)0xb801a810)
#define SB2_WRAPPER_CTRL	((volatile unsigned int *)0xb801a018)
#define SB2_SFC_POS_LATCH	((volatile unsigned int *)0xb801a814)

#define MD_SMQ_CNTL		((volatile unsigned int *)0xb800b000)
#define MD_FDMA_DDR_SADDR	((volatile unsigned int *)0xb800b088)
#define MD_FDMA_FL_SADDR	((volatile unsigned int *)0xb800b08c)
#define MD_FDMA_CTRL2		((volatile unsigned int *)0xb800b090)
#define MD_FDMA_CTRL1		((volatile unsigned int *)0xb800b094)


#define RDID_MANUFACTURER_ID_MASK	0x000000FF
#define RDID_DEVICE_ID_1_MASK		0x0000FF00
#define RDID_DEVICE_ID_2_MASK		0x00FF0000
#define RDID_DEVICE_EID_1_MASK		0x000000FF
#define RDID_DEVICE_EID_2_MASK		0x0000FF00

#define RDID_MANUFACTURER_ID(id)	(id & RDID_MANUFACTURER_ID_MASK)
#define RDID_DEVICE_ID_1(id)		((id & RDID_DEVICE_ID_1_MASK) >> 8)
#define RDID_DEVICE_ID_2(id)		((id & RDID_DEVICE_ID_2_MASK) >> 16)
#define RDID_DEVICE_EID_1(id)		(id & RDID_DEVICE_EID_1_MASK)
#define RDID_DEVICE_EID_2(id)		((id & RDID_DEVICE_EID_2_MASK) >> 8)

#define MANUFACTURER_ID_SPANSION		0x01
#define MANUFACTURER_ID_STM			0x20
#define MANUFACTURER_ID_PMC			0x7f
#define MANUFACTURER_ID_SST			0xbf
#define MANUFACTURER_ID_MXIC			0xc2
#define MANUFACTURER_ID_EON			0x1c

#define VENUS_SFC_ATTR_NONE			0x00
#define VENUS_SFC_ATTR_SUPPORT_MD_PP		0x01

#define VENUS_SFC_SMALL_PAGE_WRITE_MASK	0x3

typedef struct venus_sfc_info {
	struct semaphore	venus_sfc_lock;
	u8			manufacturer_id;
	u8			device_id1;
	u8			device_id2;
	u8			attr;
	u32			erase_size;
	u32			erase_opcode;
	struct mtd_info 	*mtd_info;
}venus_sfc_info_t;

#endif
