/*
 * MTD chip driver for SPI Serial Flash 
 * w/ Venus-DVR Serial Flash Controller Interface
 * w/ Neptune MD engine
 *
 * Copyright 2005 Chih-pin Wu <wucp@realtek.com.tw>
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/cfi.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>

#include <platform.h>
#include "venus_sfc.h"

extern struct map_info physmap_map;

static int venus_sfc_size;
static volatile unsigned char *FLASH_BASE;
module_param(venus_sfc_size, int, S_IRUGO);

static struct mtd_info descriptor;
static venus_sfc_info_t venus_sfc_info;
static long sb2_ctrl_value;
static const char *part_probes[] = { "cmdlinepart", "RedBoot", NULL };

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
typedef enum {
	INVALID 			= 0,
	READ_ACCESS 		= 0x10000000,
	READ_COMPLETED		= 0x01000000,
	WRITE_ACCESS 		= 0x20000000,
	WRITE_COMPLETED		= 0x02000000,
	ERASE_ACCESS		= 0x40000000,
	ERASE_COMPLETED		= 0x04000000,

	READ_READY		 	= 0x00000001,
	READ_STAGE_1 		= 0x00000002,
	READ_STAGE_2 		= 0x00000004,
	READ_STAGE_3 		= 0x00000008,
	WRITE_READY			= 0x00000100,
	WRITE_STAGE_1		= 0x00000200,
	WRITE_STAGE_2		= 0x00000400,
	WRITE_STAGE_3		= 0x00000800,
	WRITE_STAGE_4		= 0x00001000,
	WRITE_STAGE_5		= 0x00002000,
} PROGRESS_STATUS;

typedef struct _tagDescriptorTable {
	int from;
	int to;
	int length;
	PROGRESS_STATUS status;
} DESCRIPTOR_TABLE;

#define DBG_ENTRY_NUM 256

static int *dbg_counter = NULL;
static DESCRIPTOR_TABLE *dbg_table = NULL;

static void add_debug_entry(int from, int to, int length, PROGRESS_STATUS status) {
	if(dbg_counter && dbg_table) {
		if((*dbg_counter) < (DBG_ENTRY_NUM - 1))
			(*dbg_counter)++;
		else
			(*dbg_counter) = 0;

		dbg_table[*dbg_counter].from = from;
		dbg_table[*dbg_counter].to = to;
		dbg_table[*dbg_counter].length = length;
		dbg_table[*dbg_counter].status = status;
	}
}

static void change_status(PROGRESS_STATUS status) {
	if(dbg_counter && dbg_table)
		dbg_table[*dbg_counter].status |= status;
}
#endif

static void _before_serial_flash_access(void) {
	__asm__ __volatile__ ("sync;");

	sb2_ctrl_value = readl(SB2_WRAPPER_CTRL);

	writel((sb2_ctrl_value & (~0x03)) | 0x8, SB2_WRAPPER_CTRL);

	__asm__ __volatile__ ("sync;");

	writel((sb2_ctrl_value & (~0x03)) | 0xc, SB2_WRAPPER_CTRL);

	__asm__ __volatile__ ("sync;");
}

static void _after_serial_flash_access(void) {
	__asm__ __volatile__ ("sync;");

	writel((sb2_ctrl_value & (~0x04)) | 0x8, SB2_WRAPPER_CTRL);

	__asm__ __volatile__ ("sync;");

	writel(sb2_ctrl_value | 0x8, SB2_WRAPPER_CTRL);

	__asm__ __volatile__ ("sync;");
}

static void switch_to_read_mode(void) {
	/* restore opcode to read */
#if 1
	writel(0x00d2000b, SFC_OPCODE); /* High-Speed Read */
	writel(0x001c0019, SFC_CTL);    /* dataen = 1, adren = 1, dmycnt = 1 */
#else
	writel(0x00000003, SFC_OPCODE); /* Read */
	writel(0x00000018, SFC_CTL);    /* dataen = 1, adren = 1, dmycnt = 0 */
#endif
}

static int sst_init(venus_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x25:
		switch(sfc_info->device_id2) {
		case 0x41:
			printk(KERN_NOTICE "VenusSFC MTD: SST 25VF016B detected.\n");
			break;
		case 0x8d:
			printk(KERN_NOTICE "VenusSFC MTD: SST 25VF040B detected.\n");
			break;
		default:
			printk(KERN_NOTICE "VenusSFC MTD: SST unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		break;
	default:
		printk(KERN_NOTICE "VenusSFC MTD: SST unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	sfc_info->erase_size	= 0x1000;	//4KB
	sfc_info->erase_opcode	= 0x00000020;	//for 4KB erase.
	return 0;
}

static int spansion_init(venus_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x02:
		switch(sfc_info->device_id2) {
		case 0x12:
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL004A detected.\n");
			break;
		case 0x13:
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL008A detected.\n");
			break;
		case 0x14:
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL016A detected.\n");
			break;
		case 0x15:
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL032A detected.\n");
			break;
		case 0x16:
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL064A detected.\n");
			break;
		default:
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		sfc_info->erase_size	= 0x10000;	//64KB
		sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
		return 0;
	case 0x20:
		switch(sfc_info->device_id2) {
		case 0x18: // S25FL128P has two types of erase size, must identify via extended device id
			{
				unsigned int tmp2 = 0;
				u8 ext_id1;
				u8 ext_id2;

				writel(0x0000009f, SFC_OPCODE);	/* JEDEC Read ID */
				writel(0x00000013, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 3 */

				tmp2 = *(volatile unsigned int *)FLASH_BASE;

				ext_id1 = RDID_DEVICE_EID_1(tmp2);
				ext_id2 = RDID_DEVICE_EID_2(tmp2);
				if(ext_id1 == 0x03 && ext_id2 == 0x00) {
					printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL128P(256K sector) detected.\n");
					sfc_info->erase_size	= 0x40000;	//256KB
					sfc_info->erase_opcode	= 0x000000d8;	//for 256KB erase.	
				}
				else {
					printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL128P(64K sector) detected.\n");
					sfc_info->erase_size	= 0x10000;	//64KB
					sfc_info->erase_opcode	= 0x00000020;	//for 64KB erase.
				}
			}
			break;
		default:
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION unknown id2=0x%x detected.\n",
							sfc_info->device_id2);

			sfc_info->erase_size	= 0x10000;	//64KB
			sfc_info->erase_opcode	= 0x00000020;	//for 64KB erase.
			break;
		}

		sfc_info->attr		|= VENUS_SFC_ATTR_SUPPORT_MD_PP;
		return 0;
	default:
		printk(KERN_NOTICE "VenusSFC MTD: SPANSION unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	sfc_info->erase_size	= 0x10000;	//64KB
	sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
	return 0;
}

static int mxic_init(venus_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x20:
		switch(sfc_info->device_id2) {
		case 0x13:
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L4005 detected.\n");
			sfc_info->erase_size	= 0x1000;	//4KB
			sfc_info->erase_opcode	= 0x00000020;	//for 4KB erase.
			break;
		case 0x14:
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L8005 detected.\n");
			sfc_info->erase_size	= 0x1000;	//4KB
			sfc_info->erase_opcode	= 0x00000020;	//for 4KB erase.
			break;
		case 0x15:
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L1605 detected.\n");
			sfc_info->erase_size	= 0x10000;	//64KB
			sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
			break;
		case 0x16:
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L3205 detected.\n");
			sfc_info->erase_size	= 0x10000;	//64KB
			sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
			break;
		case 0x17:
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L6405 detected.\n");
			sfc_info->erase_size	= 0x10000;	//64KB
			sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
			break;
		case 0x18:
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L12805 detected.\n");
			sfc_info->erase_size	= 0x10000;	//64KB
			sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
			sfc_info->attr		|= VENUS_SFC_ATTR_SUPPORT_MD_PP;
			break;
		default:
			printk(KERN_NOTICE "VenusSFC MTD: MXIC unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			sfc_info->erase_size	= 0x1000;	//4KB
			sfc_info->erase_opcode	= 0x00000020;	//for 4KB erase.
			break;
		}
		return 0;
	default:
		printk(KERN_NOTICE "VenusSFC MTD: MXIC unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	sfc_info->erase_size	= 0x1000;	//4KB
	sfc_info->erase_opcode	= 0x00000020;	//for 4KB erase.
	return 0;
}

static int pmc_init(venus_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x9d:
		switch(sfc_info->device_id2) {
		case 0x7c:
			printk(KERN_NOTICE "VenusSFC MTD: PMC Pm25LV010 detected.\n");
			break;
		case 0x7d:
			printk(KERN_NOTICE "VenusSFC MTD: PMC Pm25LV020 detected.\n");
			break;
		case 0x7e:
			printk(KERN_NOTICE "VenusSFC MTD: PMC Pm25LV040 detected.\n");
			break;
		default:
			printk(KERN_NOTICE "VenusSFC MTD: PMC unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		break;
	default:
		printk(KERN_NOTICE "VenusSFC MTD: PMC unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	sfc_info->erase_size	= 0x1000;	//4KB
	sfc_info->erase_opcode	= 0x000000d7;	//for 4KB erase.
	return 0;
}

static int stm_init(venus_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
		case 0x20:
			switch(sfc_info->device_id2) {
				case 0x17:
					printk(KERN_NOTICE "VenusSFC MTD: ST M25P128 detected.\n");
					sfc_info->erase_size	= 0x10000;		//64KB
					sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
					break;
				case 0x18:
					printk(KERN_NOTICE "VenusSFC MTD: ST M25P128 detected.\n");
					sfc_info->erase_size	= 0x40000;		//256KB
					sfc_info->erase_opcode	= 0x000000d8;	//for 256KB erase.
					break;
				default:
					printk(KERN_NOTICE "VenusSFC MTD: ST unknown id2=0x%x detected.\n",	sfc_info->device_id2);
					sfc_info->erase_size	= 0x10000;		//64KB
					sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
					break;
			}
			break;
		default:
			printk(KERN_NOTICE "VenusSFC MTD: ST unknown id1=0x%x detected.\n",	sfc_info->device_id1);
			sfc_info->erase_size	= 0x10000;		//64KB
			sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
			break;
	}
	sfc_info->attr		|= VENUS_SFC_ATTR_SUPPORT_MD_PP;
	return 0;
}

static int eon_init(venus_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x20:
		switch(sfc_info->device_id2) {
		case 0x17:
			printk(KERN_NOTICE "VenusSFC MTD: EON EN25B64-100FIP detected.\n");
			sfc_info->erase_size	= 0x10000;	//64KB
			sfc_info->erase_opcode	= 0x000000d8;	//for 64KB erase.
			break;
		default:
			printk(KERN_NOTICE "VenusSFC MTD: EON unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			sfc_info->erase_size	= 0x1000;	//4KB
			sfc_info->erase_opcode	= 0x000000d8;	//for 4KB erase.
			break;
		}
		return 0;
	default:
		printk(KERN_NOTICE "VenusSFC MTD: EON unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	sfc_info->erase_size	= 0x1000;	//4KB
	sfc_info->erase_opcode	= 0x000000d8;	//for 4KB erase.
	return 0;
}

static int _sfc_read_bytes(struct mtd_info *mtd, loff_t from, size_t len,
						size_t *retlen, const u_char *buf) {
	size_t i = 0;
	size_t num = len >> 2;
	size_t remain = len & 0x3;

#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Venus SFC MTD: _sfc_read_bytes(), from = %08X, length = %08X\n", (int)from, (int)len);
#endif

	*retlen = 0;

	/* access in unit of 4 byte */
	for(i = 0 ; i < (num<<2); i+=4)
		*(unsigned int *)(buf+i) = *(volatile unsigned int *)(FLASH_BASE + from + i);

	while(remain > 0) {
		*(unsigned char *)(buf+i) = *(volatile unsigned char *)(FLASH_BASE + from + i);
		i++;
		remain--;
	}

	*retlen = i;

	return 0;
}

static int _sfc_read_md(struct mtd_info *mtd, loff_t from, size_t len,
					size_t *retlen, const u_char *buf) {
	volatile u8 *src = (volatile u8*)(FLASH_BASE + from);
	u8 *dest = (u8*)buf;
	u8 true_data_buf[MD_PP_DATA_SIZE*2];
	u8 *data_buf;
	u32 read_count = 0;
	u32 val;
	int i, ret;

#ifdef DEV_DEBUG
	printk(KERN_WARNING "Venus SFC MTD: %s() $ [%08X => %08X (virt: %08X) : %08X]\n", __func__, (int)src, (int)(virt_to_phys(buf)), buf, (int)len);
#endif

	if(unlikely((len <= VENUS_SFC_SMALL_PAGE_WRITE_MASK) || (len & VENUS_SFC_SMALL_PAGE_WRITE_MASK)))
		BUG();

	// workaround to avoid mantis issue #10822:
	// while writing less than 128 bytes and DDR address is 0x0 - 0x7f, 
	// change DDR address to any address larger than 0x7f for avoidance
	if(len < 128 && ((int)(true_data_buf) & 0x07ffffff) <= 0x7f)
		data_buf = &(true_data_buf[MD_PP_DATA_SIZE]);
	else
		data_buf = true_data_buf;

	while(len > 0) {
		size_t dma_length;

		if(len >= MD_PP_DATA_SIZE)
			dma_length = MD_PP_DATA_SIZE;
		else
			dma_length = len;

		//flush dirty cache lines
		dma_cache_sync(data_buf, dma_length, DMA_FROM_DEVICE);

		switch_to_read_mode();

		//setup MD DDR addr and flash addr
		writel(((unsigned int)data_buf), MD_FDMA_DDR_SADDR);
		writel(((unsigned int)src), MD_FDMA_FL_SADDR);

		//setup MD direction and move data length
		val = (0x0C000000 | dma_length);
		writel(val, MD_FDMA_CTRL2);		//for dma_length bytes.

		mb();

		//go 
		writel(0x03, MD_FDMA_CTRL1);

		/* wait for MD done its operation */
		while((ret = readl(MD_FDMA_CTRL1)) & 0x1);

		/* data moved by MD module is endian inverted 
		 * revert it after read from flash */
		for(i = 0 ; i < dma_length ; i += 4) {
			val = *((u32*)(data_buf + i));
			*((volatile u32*)(dest + i)) = 
				((val >> 24) |
				 ((val >> 8 ) & 0x0000ff00) |
				 ((val << 8 ) & 0x00ff0000) |
				 (val << 24));
		}

		src += dma_length;
		dest += dma_length;
		read_count += dma_length;
		len -= dma_length;
	}

	*retlen = read_count;

	return 0;
}

static int venus_sfc_read_pages(struct mtd_info *mtd, loff_t from, size_t len,
						size_t *retlen, u_char *buf) {
	venus_sfc_info_t *sfc_info;
	size_t read_len = 0;
	size_t rlen = 0;
	size_t length_to_read;

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	add_debug_entry(from, buf, len, READ_ACCESS);
#endif

	if((sfc_info = (venus_sfc_info_t*)mtd->priv) == NULL)
		return -EINVAL;

	if(down_interruptible(&sfc_info->venus_sfc_lock) == -EINTR)
		return -EINVAL;

	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

	*retlen = 0;

	switch_to_read_mode();

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(READ_READY);
#endif
	// 1st pass: read data 0-3 bytes 
	if(len > 0 && (from & VENUS_SFC_SMALL_PAGE_WRITE_MASK)) {
		if(len > VENUS_SFC_SMALL_PAGE_WRITE_MASK)
			length_to_read = (VENUS_SFC_SMALL_PAGE_WRITE_MASK + 1) - (from & VENUS_SFC_SMALL_PAGE_WRITE_MASK);
		else
			length_to_read = len;

		_sfc_read_bytes(mtd, from, length_to_read, &rlen, buf);

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_read() $ PASS #1 $ [%08X:%08X]\n", (int)from, (int)len);
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_read() $ PASS #1 $ %d bytes read\n", (int)rlen);
#endif

		from += rlen;
		read_len += rlen;
		len -= rlen;
		buf += rlen;
	}

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(READ_STAGE_1);
#endif
	// 2rd pass: using MD to read data which aligned in 4 bytes
	if(len > VENUS_SFC_SMALL_PAGE_WRITE_MASK) {
		length_to_read = len & (~VENUS_SFC_SMALL_PAGE_WRITE_MASK);

		_sfc_read_md(mtd, from, length_to_read, &rlen, buf);

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_read() $ PASS #2 $ [%08X:%08X]\n", (int)from, (int)len);
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_read() $ PASS #2 $ %d bytes read\n", (int)rlen);
#endif

		from += rlen;
		read_len += rlen;
		len -= rlen;
		buf += rlen;
	}

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(READ_STAGE_2);
#endif
	// 3rd pass: read rest data (0-3 bytes)
	if(len > 0) {
		length_to_read = len;

		_sfc_read_bytes(mtd, from, length_to_read, &rlen, buf);

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_read() $ PASS #3 $ [%08X:%08X]\n", (int)from, (int)len);
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_read() $ PASS #3 $ %d bytes read\n", (int)rlen);
#endif

		from += rlen;
		read_len += rlen;
		len -= rlen;
		buf += rlen;
	}

	*retlen = read_len;

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(READ_STAGE_3);
#endif
	/* Restore SB2 */
	_after_serial_flash_access();

	up(&sfc_info->venus_sfc_lock);

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(READ_COMPLETED);
#endif

	return 0;
};


static int venus_sfc_read(struct mtd_info *mtd, loff_t from, size_t len,
						size_t *retlen, u_char *buf) {
	venus_sfc_info_t *sfc_info;

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	add_debug_entry(from, buf, len, READ_ACCESS);
#endif

	if((sfc_info = (venus_sfc_info_t*)mtd->priv) == NULL)
		return -EINVAL;

	if(down_interruptible(&sfc_info->venus_sfc_lock) == -EINTR)
		return -EINVAL;

#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Venus SFC MTD: venus_sfc_read(), from = %08X, length = %08X\n", (int)from, (int)len);
#endif

	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

	*retlen = 0;

	switch_to_read_mode();

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(READ_READY);
#endif

	_sfc_read_bytes(mtd, from, len, retlen, buf);

	/* Restore SB2 */
	_after_serial_flash_access();

	up(&sfc_info->venus_sfc_lock);

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(READ_COMPLETED);
#endif

	return 0;
}

static int _sfc_write_bytes(struct mtd_info *mtd, loff_t to, size_t len,
					size_t *retlen, const u_char *buf) {
	u8 *src = (u8*)buf;
	volatile u8 *dest = (volatile u8*)(FLASH_BASE + to);
	u8 tmp;
	u32 written_count = 0;

	while(len--) {
		/* send write enable first */
		writel(0x00520006, SFC_OPCODE);	/* Write enable */
		writel(0x00180000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
		tmp = *(volatile unsigned char *)FLASH_BASE;

		writel(0x00000002, SFC_OPCODE);	/* Byte Programming */
		writel(0x00000018, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 0 */

		*dest++ = *src++;

		mb();

		/* using RDSR to make sure the operation is completed. */
		do {
			writel(0x00000005, SFC_OPCODE);	/* RDSR */
			writel(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */
		} while((*(volatile unsigned char *)FLASH_BASE) & 0x1);

		written_count++;
	}

	/* send write disable then */
	writel(0x00000004, SFC_OPCODE);	/* Write disable */
	writel(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	tmp = *(volatile unsigned char *)FLASH_BASE;

	*retlen = written_count;

	return 0;
}

/*
 * _sfc_write_small_pages()
 * using MD to write data from DDR to FLASH which size is within a single page
 *
 * pass-in address must be aligned on 4-bytes, and 
 * length must be multiples of 4-bytes
 */

static int _sfc_write_small_pages(struct mtd_info *mtd, loff_t to, size_t len,
					size_t *retlen, const u_char *buf) {
	u8 tmp;
	u8 *src = (u8*)buf;
	volatile u8 *dest = (volatile u8*)(FLASH_BASE + to);
	u32 written_count = 0;
	u8 true_data_buf[MD_PP_DATA_SIZE*2];
	u8 *data_buf;
	u32 val;
	int ret;
	int i;

	// workaround to avoid mantis issue #10822:
	// while writing less than 128 bytes and DDR address is 0x0 - 0x7f, 
	// change DDR address to any address larger than 0x7f for avoidance
	if(len < 128 && ((int)(true_data_buf) & 0x07ffffff) <= 0x7f)
		data_buf = &(true_data_buf[MD_PP_DATA_SIZE]);
	else
		data_buf = true_data_buf;


	// support write fewer than 256 bytes and size must be multiples of 4-bytes
	if(unlikely((len >= MD_PP_DATA_SIZE) || (len & VENUS_SFC_SMALL_PAGE_WRITE_MASK)))
		BUG();

	if((to & VENUS_SFC_SMALL_PAGE_WRITE_MASK)) // only support write onto 4-bytes aligned area
		BUG();

	while(len > 0) {
		for(i = 0; i < len ; i += 4) { /* endian convert */
			val = *((u32*)(src + i));
			*((u32*)(data_buf + i)) = 
						((val >> 24) |
						((val >> 8 ) & 0x0000ff00) |
						((val << 8 ) & 0x00ff0000) |
						(val << 24));
		}

		dma_cache_sync(data_buf, len, DMA_TO_DEVICE);

		//(write enable)
		writel(0x00520006, SFC_OPCODE);
		writel(0x00180000, SFC_CTL);
		tmp = *(volatile unsigned char *)FLASH_BASE;

		//issue write command
		writel(0x00000002, SFC_OPCODE);
		writel(0x00000018, SFC_CTL);

		//setup MD DDR addr and flash addr
		writel(((unsigned int)data_buf), MD_FDMA_DDR_SADDR);
		writel(((unsigned int)dest), MD_FDMA_FL_SADDR);

 		//setup MD direction and move data length
		val = (0x0E000000 | len);
		writel(val, MD_FDMA_CTRL2);		//for dma_length bytes.

		mb();

		//go 
		writel(0x03, MD_FDMA_CTRL1);

		/* wait for MD done its operation */
		while((ret = readl(MD_FDMA_CTRL1)) & 0x1);

		/* wait for flash controller done its operation */
		do {
			writel(0x00000005, SFC_OPCODE);
			writel(0x00000010, SFC_CTL);
		} while((*(volatile unsigned char *)FLASH_BASE) & 0x1);
    	    
		/* shift to next page to writing */
		src += len;
		dest += len;
		written_count += len;
		len -= len;
	} //end of page program
        
	/* send write disable then */
	writel(0x00000004, SFC_OPCODE);	/* Write disable */
	writel(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	tmp = *(volatile unsigned char *)FLASH_BASE;

	*retlen = written_count;

	return 0;
}

static int _sfc_write_pages(struct mtd_info *mtd, loff_t to, size_t len,
					size_t *retlen, const u_char *buf) {
	u8 tmp;
	u8 *src = (u8*)buf;
	volatile u8 *dest = (volatile u8*)(FLASH_BASE + to);
	u32 written_count = 0;
	u8 data_buf[MD_PP_DATA_SIZE];
	u32 val;
	int ret;
	int i;

	if(unlikely((len < MD_PP_DATA_SIZE))) {
		*retlen = 0;
		return 0;
	}

	if(unlikely(((u32)dest&(MD_PP_DATA_SIZE-1)))) {
		BUG();
	}

	while(len >= MD_PP_DATA_SIZE) {
		/* data moved by MD module is endian inverted.
					revert it before moved to flash */
		for(i = 0; i < MD_PP_DATA_SIZE; i += 4) {
			val = *((u32*)(src + i));
			*((u32*)(data_buf + i)) = 
						((val >> 24) |
						((val >> 8 ) & 0x0000ff00) |
						((val << 8 ) & 0x00ff0000) |
						(val << 24));
		}

		dma_cache_sync(data_buf, MD_PP_DATA_SIZE, DMA_TO_DEVICE);

		//(write enable)
		writel(0x00520006, SFC_OPCODE);
		writel(0x00180000, SFC_CTL);
		tmp = *(volatile unsigned char *)FLASH_BASE;

		//issue write command
		writel(0x00000002, SFC_OPCODE);
		writel(0x00000018, SFC_CTL);

		//setup MD DDR addr and flash addr
		writel(((unsigned int)data_buf), MD_FDMA_DDR_SADDR);
		writel(((unsigned int)dest), MD_FDMA_FL_SADDR);

        //setup MD direction and move data length
        val = (0x0E000000 | MD_PP_DATA_SIZE);
		writel(val, MD_FDMA_CTRL2);		//for dma_length bytes.

		mb();

		//go 
		writel(0x03, MD_FDMA_CTRL1);

		/* wait for MD done its operation */
		while((ret = readl(MD_FDMA_CTRL1)) & 0x1);

		/* wait for flash controller done its operation */
		do {
			writel(0x00000005, SFC_OPCODE);
			writel(0x00000010, SFC_CTL);
		} while((*(volatile unsigned char *)FLASH_BASE) & 0x1);
    	    
		/* shift to next page to writing */
		src += MD_PP_DATA_SIZE;
		dest += MD_PP_DATA_SIZE;
		written_count += MD_PP_DATA_SIZE;
		len -= MD_PP_DATA_SIZE;
	}//end of page program
        
	/* send write disable then */
	writel(0x00000004, SFC_OPCODE);	/* Write disable */
	writel(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	tmp = *(volatile unsigned char *)FLASH_BASE;

	*retlen = written_count;

	return 0;
}

/* venus_sfc_write_pages() supports MD(move data module) page programming(PP). */
static int venus_sfc_write_pages(struct mtd_info *mtd, loff_t to, size_t len,
						size_t *retlen, const u_char *buf) {
	venus_sfc_info_t *sfc_info;
	u8 *src = (u8*)buf;
	size_t written_len = 0;
	size_t wlen = 0;
	size_t length_to_write;
	int ret;

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	add_debug_entry(buf, to, len, WRITE_ACCESS);
#endif

#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write(), addr = %08X, size = %08X\n", (int)to, (int)len);
#endif

	if((sfc_info = (venus_sfc_info_t*)mtd->priv) == NULL) {
		return -EINVAL;
	}

	if(down_interruptible(&sfc_info->venus_sfc_lock) == -EINTR) {
		return -EINVAL;
	}

	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(WRITE_READY);
#endif

	// 1st pass: write data 0-3 bytes 
	// (to make destination address within 4-bytes alignment)
	if(len > 0 && (to & VENUS_SFC_SMALL_PAGE_WRITE_MASK)) {
		if(len > VENUS_SFC_SMALL_PAGE_WRITE_MASK)
			length_to_write = (VENUS_SFC_SMALL_PAGE_WRITE_MASK + 1) - (to & VENUS_SFC_SMALL_PAGE_WRITE_MASK);
		else
			length_to_write = len;

		ret = _sfc_write_bytes(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up(&sfc_info->venus_sfc_lock);
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #1 $ [%08X:%08X]\n", (int)to, (int)len);
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #1 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(WRITE_STAGE_1);
#endif
	// 2st pass: write data which aligned in 4 bytes but smaller than a single page (256 bytes)
	// (to make destination address within 256-bytes alignment)
	if(len > VENUS_SFC_SMALL_PAGE_WRITE_MASK && 
					(((to + MD_PP_DATA_SIZE - 1) & (~(MD_PP_DATA_SIZE - 1))) - to) > VENUS_SFC_SMALL_PAGE_WRITE_MASK) {
		length_to_write = ((to + MD_PP_DATA_SIZE - 1) & (~(MD_PP_DATA_SIZE - 1))) - to;

		if(length_to_write > len)
			length_to_write = len;

		if(length_to_write & VENUS_SFC_SMALL_PAGE_WRITE_MASK)
			length_to_write = length_to_write & (~VENUS_SFC_SMALL_PAGE_WRITE_MASK);

		ret = _sfc_write_small_pages(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up(&sfc_info->venus_sfc_lock);
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #2 $ [%08X:%08X]\n", (int)to, (int)len);
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #2 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(WRITE_STAGE_2);
#endif
	// 3rd pass: write data in page unit (256 bytes)
	if(len >= MD_PP_DATA_SIZE) {
		length_to_write = len / MD_PP_DATA_SIZE * MD_PP_DATA_SIZE;

		ret = _sfc_write_pages(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up(&sfc_info->venus_sfc_lock);
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #3 $ [%08X:%08X] (%d)\n", (int)to, (int)len, MD_PP_DATA_SIZE);
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #3 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(WRITE_STAGE_3);
#endif
	// 4th pass: write data which aligned in 4 bytes but smaller than a single page (256 bytes)
	if(len > 0 && (len & (~VENUS_SFC_SMALL_PAGE_WRITE_MASK))) {
		length_to_write = len & (~VENUS_SFC_SMALL_PAGE_WRITE_MASK);

		ret = _sfc_write_small_pages(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up(&sfc_info->venus_sfc_lock);
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #4 $ [%08X:%08X]\n", (int)to, (int)len);
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #4 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(WRITE_STAGE_4);
#endif
	// 5th pass: write data 0-3 bytes
	if(len > 0) {
		length_to_write = len;

		ret = _sfc_write_bytes(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up(&sfc_info->venus_sfc_lock);
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #5 $ [%08X:%08X]\n", (int)to, (int)len);
		printk(KERN_WARNING "Venus SFC MTD: venus_sfc_write() $ PASS #5 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(WRITE_STAGE_5);
#endif

	*retlen = written_len;

	/* restore opcode to read */
	switch_to_read_mode();

	/* Restore SB2 */
	_after_serial_flash_access();

	up(&sfc_info->venus_sfc_lock);

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(WRITE_COMPLETED);
#endif

	return 0;
}

static int venus_sfc_write(struct mtd_info *mtd, loff_t to, size_t len,
					size_t *retlen, const u_char *buf) {
	int retval;

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	add_debug_entry(buf, to, len, WRITE_ACCESS);
#endif

	if(down_interruptible(&((venus_sfc_info_t*)mtd->priv)->venus_sfc_lock) == -EINTR)
		return -EINVAL;

	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(WRITE_READY);
#endif

	retval = _sfc_write_bytes(mtd, to, len, retlen, buf);

	/* restore opcode to read */
	switch_to_read_mode();

	/* Restore SB2 */
	_after_serial_flash_access();

	up(&((venus_sfc_info_t*)mtd->priv)->venus_sfc_lock);

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(WRITE_COMPLETED);
#endif

	return retval;
}

static int venus_sfc_erase(struct mtd_info *mtd, struct erase_info *instr) {
	venus_sfc_info_t *sfc_info;
	unsigned int size;
	volatile unsigned char *addr;
	unsigned char tmp;

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	add_debug_entry(instr->addr, 0, instr->len, ERASE_ACCESS);
#endif

	if((sfc_info = (venus_sfc_info_t*)mtd->priv) == NULL) {
		return -EINVAL;
	}

	if(instr->addr + instr->len > mtd->size)
		return -EINVAL;

	if(down_interruptible(&sfc_info->venus_sfc_lock) == -EINTR)
		return -EINVAL;

#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Venus SFC MTD: venus_sfc_erase(), addr = %08X, size = %08X\n", instr->addr, instr->len);
#endif

	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

	addr = FLASH_BASE + (instr->addr);
	size = instr->len;

	for(size = instr->len ; size > 0 ; size -= mtd->erasesize) {

		/* prior to any write instructions, send write enable first */
		writel(0x00000006, SFC_OPCODE);	/* Write enable */
		writel(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
		tmp = *(volatile unsigned char *)FLASH_BASE;

		writel(sfc_info->erase_opcode, SFC_OPCODE);	/* Sector-Erase */
		writel(0x00000008, SFC_CTL);	/* adren = 1 */
		tmp = *addr;

		/* using RDSR to make sure the operation is completed. */
		while(1) {
			writel(0x00000005, SFC_OPCODE);	/* Read status register */
			writel(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */

			if((*(volatile unsigned char *)FLASH_BASE) & 0x1)
				msleep(100); // erasing 256KBytes might take up to 2 seconds!
			else
				break;
		}

		addr += mtd->erasesize;
	}

	/* send write disable then */
	writel(0x00000004, SFC_OPCODE);	/* Write disable */
	writel(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	tmp = *(volatile unsigned char *)FLASH_BASE;

	/* restore opcode to read */
	switch_to_read_mode();

	/* Restore SB2 */
	_after_serial_flash_access();

	up(&sfc_info->venus_sfc_lock);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	change_status(ERASE_COMPLETED);
#endif

	return 0;
}

static int venus_sfc_suspend(struct mtd_info *mtd)
{
	return 0;
}

static void venus_sfc_resume(struct mtd_info *mtd)
{
	return;
}

#if 0
static int venus_sfc_point(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char **mtdbuf)
{
	return 0;
}

static void venus_sfc_unpoint(struct mtd_info *mtd, u_char *addr, loff_t from, size_t len)
{
	return;
}
#endif

static int venus_sfc_probe(struct mtd_info *mtd_info) {
	venus_sfc_info_t *sfc_info;
	unsigned int tmp = 0;
	int ret = 0;

	if((sfc_info = (venus_sfc_info_t*)mtd_info->priv) == NULL) {
		return -ENODEV;
	}

	writel(0x0000009f, SFC_OPCODE);	/* JEDEC Read ID */
	writel(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */

	tmp = *(volatile unsigned int *)FLASH_BASE;

	sfc_info->manufacturer_id = RDID_MANUFACTURER_ID(tmp);
	sfc_info->device_id1 = RDID_DEVICE_ID_1(tmp);
	sfc_info->device_id2 = RDID_DEVICE_ID_2(tmp);

	switch(sfc_info->manufacturer_id) {
	case MANUFACTURER_ID_SST:
		ret = sst_init(sfc_info);
		break;
	case MANUFACTURER_ID_SPANSION:
		ret = spansion_init(sfc_info);
		break;
	case MANUFACTURER_ID_MXIC:
		ret = mxic_init(sfc_info);
		break;
	case MANUFACTURER_ID_PMC:
		ret = pmc_init(sfc_info);
		break;
	case MANUFACTURER_ID_STM:
		ret = stm_init(sfc_info);
		break;
	case MANUFACTURER_ID_EON:
		ret = eon_init(sfc_info);
		break;
	default:
		printk(KERN_WARNING "VenusSFC MTD: Unknown flash type.\n");
		printk(KERN_WARNING "Manufacturer's ID = %02X, Memory Type = %02X, Memory Capacity = %02X\n", tmp & 0xff, (tmp >> 8) & 0xff, (tmp >> 16) & 0xff);
		return -ENODEV;
		break;
	}

	mtd_info->erasesize = sfc_info->erase_size;

#if defined(CONFIG_MTD_VENUS_SFC_READ_MD) || defined(CONFIG_MTD_VENUS_SFC_WRITE_MD)
	if((sfc_info->attr & VENUS_SFC_ATTR_SUPPORT_MD_PP) && (is_neptune_cpu() || is_mars_cpu())) { // The feature is Neptune Only
		printk("VenusSFC MTD: Enable VenusSFC MD PP callback function.\n");

	#if defined(CONFIG_MTD_VENUS_SFC_READ_MD)
		mtd_info->read = venus_sfc_read_pages;
	#endif
	#if defined(CONFIG_MTD_VENUS_SFC_WRITE_MD)
		mtd_info->write = venus_sfc_write_pages;
	#endif
	} 
#endif

	return 0;
}


int __init venus_sfc_init(void)
{
	unsigned char tmp;
	int nr_parts = 0;
	struct mtd_partition *parts;

	printk("VenusSFC MTD init\n");

	// check if serial flash is accessible on Mars chip
	if (is_mars_cpu()) {
		tmp = (*(volatile unsigned int *)0xb8000304) & 0x3;
		if (tmp == 0x2) {	// serial flash is inaccessible in NAND boot 
			printk("serial flash inaccessible\n");
			return -ENODEV;
		}
	}
	// parsing parameters
	venus_sfc_size = physmap_map.size;
	FLASH_BASE = (unsigned char*)(physmap_map.phys + 0xa0000000);	// offset to kseg1

	// initialize hardware (??)
	_before_serial_flash_access();

	// use 40MHz to access, SPI Mode 3 (for ATMEL)
	// use 40MHz to access, SPI Mode 0 (for Non-ATMEL)
	// writel(0x01040004, SFC_SCK);

	// deselect cycle, post-cycle, pre-cycle setting
	// writel(0x000f0f0f, SFC_CE);
	writel(0x00090202, SFC_CE);

	// disable HW write protect
	writel(0x00000000, SFC_WP);

	// use slower speed to read JEDEC-ID - use 25MHz
	writel(0x01070007, SFC_SCK);

	// use rising edge to latch data because clock rate is low now
	if(is_neptune_cpu() || is_mars_cpu())
		writel(0x1, SB2_SFC_POS_LATCH);

	/* prepare mtd_info */
	memset(&descriptor, 0, sizeof(struct mtd_info));
	memset(&venus_sfc_info, 0, sizeof(venus_sfc_info_t));

	venus_sfc_info.mtd_info = &descriptor;
	init_MUTEX(&venus_sfc_info.venus_sfc_lock);

	descriptor.priv = &venus_sfc_info;
	descriptor.name = "VenusSFC";
	descriptor.size = venus_sfc_size;	/* 2MBytes */
	descriptor.flags = MTD_CLEAR_BITS | MTD_ERASEABLE;
	descriptor.erase = venus_sfc_erase;
	descriptor.read = venus_sfc_read;
	descriptor.suspend = venus_sfc_suspend;
	descriptor.write = venus_sfc_write;
	descriptor.resume = venus_sfc_resume;
	descriptor.owner = THIS_MODULE;
	descriptor.type = MTD_NORFLASH;
	descriptor.numeraseregions = 0;	// uni-erasesize

	if(venus_sfc_probe(&descriptor) != 0) {
		/* restore SB2 */
		_after_serial_flash_access();

		return -ENODEV;
	}

	// restore speed - use 40MHz to access
	if(is_mars_cpu()) // 168.75MHz / 4 ~= 42MHz
		writel(0x01030003, SFC_SCK);
	else // 200MHz / 5 ~= 40MHz
		writel(0x01040004, SFC_SCK);

	// use falling edge to latch data because clock rate is changed to high
	if(is_neptune_cpu() || is_mars_cpu())
		writel(0x0, SB2_SFC_POS_LATCH);

	/* EWSR: enable write status register */
	writel(0x00000050, SFC_OPCODE);
	writel(0x00000000, SFC_CTL);

	tmp = (unsigned char)*((volatile unsigned char*)FLASH_BASE);

	/* WRSR: write status register, no memory protection */
	writel(0x00000001, SFC_OPCODE);
	writel(0x00000010, SFC_CTL);
	*((volatile unsigned char*)FLASH_BASE)= 0x00;

	/*
	 * Partition selection stuff.
	 */

#ifdef CONFIG_MTD_PARTITIONS
	nr_parts = parse_mtd_partitions(&descriptor, part_probes, &parts, 0);
#endif

	if(nr_parts <= 0) {
		printk(KERN_NOTICE "Venus SFC: using single partition ");
		if(add_mtd_device(&descriptor)) {
			/* restore SB2 */
			_after_serial_flash_access();

			printk(KERN_WARNING "Venus SFC: (for SST/SPANSION/MXIC SPI-Flash) Failed to register new device\n");
			return -EAGAIN;
		}
	}
	else {
		printk(KERN_NOTICE "Venus SFC: using dynamic partition ");
		add_mtd_partitions(&descriptor, parts, nr_parts);
	}

    /* restore opcode to read */
	switch_to_read_mode();

	/* restore SB2 */
	_after_serial_flash_access();

#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	/* allocating debug information */
	if((dbg_counter = (int *) kmalloc(sizeof(int), GFP_KERNEL)))
		memset(dbg_counter, 0, sizeof(int));

	if((dbg_table = (DESCRIPTOR_TABLE *) kmalloc(sizeof(DESCRIPTOR_TABLE) * DBG_ENTRY_NUM, GFP_KERNEL)))
		memset(dbg_table, 0, sizeof(DESCRIPTOR_TABLE) * DBG_ENTRY_NUM);

	printk("DEBUG COUNTER = %08X  /  DEBUG TABLE = %08X\n", dbg_counter, dbg_table);
#endif

	printk("Venus SFC: (for SST/SPANSION/MXIC SPI Flash)\n");
	return 0;
}

static void __exit venus_sfc_exit(void)
{
#if defined(CONFIG_MTD_VENUS_SFC_DEBUG)
	if(dbg_counter)
		kfree(dbg_counter);
	if(dbg_table)
		kfree(dbg_table);
#endif

	/* put read opcode into control register */
	writel(0x00000003, SFC_OPCODE);
	writel(0x00000018, SFC_CTL);

	del_mtd_device(&descriptor);
}

module_init(venus_sfc_init);
module_exit(venus_sfc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chih-pin Wu <wucp@realtek.com.tw>");
MODULE_DESCRIPTION("MTD chip driver for Realtek Venus Serial Flash Controller");
