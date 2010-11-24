/*
 * MTD chip driver for SST SPI Serial Flash 
 * w/ Venus-DVR Serial Flash Controller Interface
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
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/cfi.h>
#include <linux/delay.h>
#include <linux/init.h>
#include "venus_sfc.h"
#define SST_MANUFACTURER_ID			0xbf
#define SPANSION_MANUFACTURER_ID	0x01
#define MXIC_MANUFACTURER_ID		0xc2
#define PMC_MANUFACTURER_ID			0x7f
#define STM_MANUFACTURER_ID			0x20

//#define DEV_DEBUG

enum SPI_FLASH_TYPE {
	SST_SPI,
	SPANSION_SPI,
	SPANSION_SPI_256K_SECTOR,
	MXIC_SPI_4K_SECTOR,
	MXIC_SPI_64K_SECTOR,
	PMC_SPI,
	STM_SPI,
};

extern struct map_info physmap_map;

static enum SPI_FLASH_TYPE flashType;
static int sst_size;
static volatile unsigned char *FLASH_BASE;
module_param(sst_size, int, S_IRUGO);

DECLARE_MUTEX(sst_lock);
static struct mtd_info descriptor;
static volatile unsigned char* dest = (unsigned char*)0xbfc00000; 
static const char *part_probes[] = { "cmdlinepart", "RedBoot", NULL };

static int sst_probe(void) {
	unsigned int tmp = 0;
	writel(0x0000009f, SFC_OPCODE);	/* JEDEC Read ID */
	writel(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */

	tmp = *(volatile unsigned int *)dest;

	if((tmp & 0xff) == SST_MANUFACTURER_ID && 
		((tmp >> 8) & 0xff) == 0x25) {
		if(((tmp >> 16) & 0xff) == 0x41) 
			printk(KERN_NOTICE "VenusSFC MTD: SST 25VF016B detected.\n");
		else if(((tmp >> 16) & 0xff) == 0x8d)
			printk(KERN_NOTICE "VenusSFC MTD: SST 25VF040B detected.\n");
		else 
			printk(KERN_NOTICE "VenusSFC MTD: SST unknown detected.\n");

		flashType = SST_SPI;
	}
	else if((tmp & 0xff) == SPANSION_MANUFACTURER_ID &&
		((tmp >> 8) & 0xff) == 0x02) {
		if(((tmp >> 16) & 0xff) == 0x18)
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL128A detected.\n");
		else if(((tmp >> 16) & 0xff) == 0x16)
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL064A detected.\n");
		else if(((tmp >> 16) & 0xff) == 0x15) 
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL032A detected.\n");
		else if(((tmp >> 16) & 0xff) == 0x14)
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL016A detected.\n");
		else if(((tmp >> 16) & 0xff) == 0x13)
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL008A detected.\n");
		else if(((tmp >> 16) & 0xff) == 0x12)
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION S25FL004A detected.\n");
		else
			printk(KERN_NOTICE "VenusSFC MTD: SPANSION unknown detected.\n");

		flashType = SPANSION_SPI;
	}
	else if((tmp & 0xff) == MXIC_MANUFACTURER_ID &&
		((tmp >> 8) & 0xff) == 0x20) {
		if(((tmp >> 16) & 0xff) == 0x13) {
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L4005 detected.\n");
			flashType = MXIC_SPI_4K_SECTOR;
		}
		else if(((tmp >> 16) & 0xff) == 0x14) {
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L8005 detected.\n");
			flashType = MXIC_SPI_4K_SECTOR;
		}
		else if(((tmp >> 16) & 0xff) == 0x15) {
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L1605 detected.\n");
			flashType = MXIC_SPI_64K_SECTOR;
		}
		else if(((tmp >> 16) & 0xff) == 0x16) {
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L3205 detected.\n");
			flashType = MXIC_SPI_64K_SECTOR;
		}
		else if(((tmp >> 16) & 0xff) == 0x17) {
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L6405 detected.\n");
			flashType = MXIC_SPI_64K_SECTOR;
		}
		else if(((tmp >> 16) & 0xff) == 0x18) {
			printk(KERN_NOTICE "VenusSFC MTD: MXIC MX25L12805 detected.\n");
			flashType = MXIC_SPI_64K_SECTOR;
		}
		else {
			printk(KERN_NOTICE "VenusSFC MTD: MXIC unknown detected.\n");
			flashType = MXIC_SPI_4K_SECTOR;
		}
	}
	else if((tmp & 0xff) == PMC_MANUFACTURER_ID &&
		((tmp >> 8) & 0xff) == 0x9d) {
		if(((tmp >> 16) & 0xff) == 0x7c)
			printk(KERN_NOTICE "VenusSFC MTD: PMC Pm25LV010 detected.\n");
		else if(((tmp >> 16) & 0xff) == 0x7d)
			printk(KERN_NOTICE "VenusSFC MTD: PMC Pm25LV020 detected.\n");
		else if(((tmp >> 16) & 0xff) == 0x7e)
			printk(KERN_NOTICE "VenusSFC MTD: PMC Pm25LV040 detected.\n");
		else
			printk(KERN_NOTICE "VenusSFC MTD: PMC unknown detected.\n");

		flashType = PMC_SPI;
	}
	else if((tmp & 0xff) == STM_MANUFACTURER_ID &&
		((tmp >> 8) & 0xff) == 0x20) {
		if(((tmp >> 16) & 0xff) == 0x18)
			printk(KERN_NOTICE "VenusSFC MTD: ST M25P64detected.\n");

		flashType = STM_SPI;
	}
	else {
		printk(KERN_WARNING "VenusSFC MTD: Unknown flash type.\n");
		printk(KERN_WARNING "Manufacturer's ID = %02X, Memory Type = %02X, Memory Capacity = %02X\n", tmp & 0xff, (tmp >> 8) & 0xff, (tmp >> 16) & 0xff);
		return -1;
	}
	return 0;
}

static int sst_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	size_t i = 0;
	size_t num = len >> 2;
	size_t remain = len & 0x3;

	if(down_interruptible(&sst_lock) == -EINTR)
		return -EINVAL;

#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Venus SST MTD: sst_read(), from = %08X, length = %08X\n", (int)from, (int)len);
#endif

	*retlen = 0;

#if 1
	writel(0x00d2000b, SFC_OPCODE);	/* High-Speed Read */
	writel(0x001c0019, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 1 */
#else
	writel(0x00000003, SFC_OPCODE);	/* Read */
	writel(0x00000018, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 0 */
#endif

	/* access in unit of 4 byte */
	for(i = 0 ; i < (num<<2); i+=4)
		*(unsigned int *)(buf+i) = *(volatile unsigned int *)(FLASH_BASE + from + i);

	while(remain > 0) {
		*(unsigned char *)(buf+i) = *(volatile unsigned char *)(FLASH_BASE + from + i);
		i++;
		remain--;
	}

	*retlen = i;

	up(&sst_lock);
	return 0;
}

static int sst_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	unsigned char tmp;

	size_t i = 0;

	*retlen = 0;

	if(down_interruptible(&sst_lock) == -EINTR)
		return -EINVAL;

#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Venus SST MTD: sst_write(), to = %08X, length = %08X\n", (int)to, (int)len);
#endif

	while(len--) {
		/* send write enable first */
		writel(0x00000006, SFC_OPCODE);	/* Write enable */
		writel(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
		tmp = *(volatile unsigned char *)(0xbfc00000);

		writel(0x00000002, SFC_OPCODE);	/* Byte Programming */
		writel(0x00000018, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 0 */

		*(volatile unsigned char *)(FLASH_BASE + to + i) = *(buf+i);
		i++;

		/* using RDSR to make sure the operation is completed. */
		do {
			writel(0x00000005, SFC_OPCODE);	/* RDSR */
			writel(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */
		} while((tmp = *dest) & 0x1);
	}

	/* send write disable then */
	writel(0x00000004, SFC_OPCODE);	/* Write disable */
	writel(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	tmp = *(volatile unsigned char *)(0xbfc00000);

	*retlen = i;
	up(&sst_lock);
	return 0;
}

static int sst_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	unsigned int size;
	volatile unsigned char *addr;
	unsigned char tmp;
	int tmp_integer;

	if(instr->addr + instr->len > mtd->size)
		return -EINVAL;

	if(down_interruptible(&sst_lock) == -EINTR)
		return -EINVAL;

#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Venus SST MTD: sst_erase(), addr = %08X, size = %08X\n", instr->addr, instr->len);
#endif

	addr = FLASH_BASE + (instr->addr);
	size = instr->len;

	/* prior to any write instructions, send write enable first */
	writel(0x00000006, SFC_OPCODE);	/* Write enable */
	writel(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	tmp = *(volatile unsigned char *)(0xbfc00000);

	/* erase instruction */
	if(flashType == SST_SPI)
		writel(0x00000020, SFC_OPCODE);	/* 4KBytes Sector-Erase */
	else if(flashType == SPANSION_SPI)
		writel(0x000000d8, SFC_OPCODE);	/* 64KBytes Sector-Erase */
	else if(flashType == MXIC_SPI_4K_SECTOR)
		writel(0x00000020, SFC_OPCODE);	/* 4KBytes Sector-Erase */
	else if(flashType == MXIC_SPI_64K_SECTOR)
		writel(0x00000020, SFC_OPCODE);	/* 64KBytes Sector-Erase */
	else if(flashType == PMC_SPI)
		writel(0x000000d7, SFC_OPCODE); /* 4KBytes Sector-Erase */
	else if(flashType == STM_SPI)
		writel(0x000000d8, SFC_OPCODE); /* 64KBytes Sector Erase */

	writel(0x00000008, SFC_CTL);	/* adren = 1 */

	for(size = instr->len ; size > 0 ; size -= mtd->erasesize) {
		tmp_integer = *(addr);
		addr += mtd->erasesize;
		/* using RDSR to make sure the operation is completed. */
		do {
			writel(0x00000005, SFC_OPCODE);	/* Read status register */
			writel(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */
		} while((tmp = *dest) & 0x1);
	}

	/* send write disable then */
	writel(0x00000004, SFC_OPCODE);	/* Write disable */
	writel(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	tmp = *(volatile unsigned char *)(0xbfc00000);

	up(&sst_lock);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

static int sst_suspend(struct mtd_info *mtd)
{
	return 0;
}

static void sst_resume(struct mtd_info *mtd)
{
	return;
}

#if 0
static int sst_point(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char **mtdbuf)
{
	return 0;
}

static void sst_unpoint(struct mtd_info *mtd, u_char *addr, loff_t from, size_t len)
{
	return;
}
#endif

int __init sst_spi_flash_init(void)
{
	unsigned char tmp;
	int nr_parts = 0;
	struct mtd_partition *parts;

	// parsing parameters
	sst_size = physmap_map.size;
	FLASH_BASE = physmap_map.phys + 0xa0000000;	// offset to kseg1

	// initialize hardware (??)
	*(volatile unsigned int *)(0xb801a018) = 0x0000000c;

	// use 40MHz to access, SPI Mode 3 (for ATMEL)
	// use 40MHz to access, SPI Mode 0 (for Non-ATMEL)
	// writel(0x01040004, SFC_SCK);

	// deselect cycle, post-cycle, pre-cycle setting
	// writel(0x000f0f0f, SFC_CE);
	writel(0x00090101, SFC_CE);

	// disable HW write protect
	writel(0x00000000, SFC_WP);

	// use slower speed to read JEDEC-ID - use 25MHz
	writel(0x01070007, SFC_SCK);

	if(sst_probe() != 0)
		return -ENODEV;

	// restore speed - use 40MHz to access
	writel(0x01040004, SFC_SCK);

	/* EWSR: enable write status register */
	writel(0x00000050, SFC_OPCODE);
	writel(0x00000000, SFC_CTL);
	tmp = *dest;

	/* WRSR: write status register, no memory protection */
	writel(0x00000001, SFC_OPCODE);
	writel(0x00000010, SFC_CTL);
	*dest = 0x00;

	/* prepare mtd_info */
	memset(&descriptor, 0, sizeof(struct mtd_info));

	descriptor.priv = NULL;	// no private space used
	descriptor.name = "VenusSFC";
	descriptor.size = sst_size;	/* 2MBytes */
	descriptor.flags = MTD_CLEAR_BITS | MTD_ERASEABLE;
	descriptor.erase = sst_erase;
	descriptor.read = sst_read;
	descriptor.suspend = sst_suspend;
	descriptor.write = sst_write;
	descriptor.resume = sst_resume;
	descriptor.owner = THIS_MODULE;
	descriptor.type = MTD_NORFLASH;

	if(flashType == SST_SPI)
		descriptor.erasesize = 0x1000;	// 4K
	else if(flashType == SPANSION_SPI)
		descriptor.erasesize = 0x10000;	// 64K
	else if(flashType == SPANSION_SPI_256K_SECTOR)
		descriptor.erasesize = 0x40000; // 256K
	else if(flashType == MXIC_SPI_4K_SECTOR)
		descriptor.erasesize = 0x1000; // 4K
	else if(flashType == MXIC_SPI_64K_SECTOR)
		descriptor.erasesize = 0x10000; // 64K
	else if(flashType == PMC_SPI)
		descriptor.erasesize = 0x1000; // 4K
	else if(flashType == STM_SPI)
		descriptor.erasesize = 0x10000; // 64K

	descriptor.numeraseregions = 0;	// uni-erasesize

	/*
	 * Partition selection stuff.
	 */

#ifdef CONFIG_MTD_PARTITIONS
	nr_parts = parse_mtd_partitions(&descriptor, part_probes, &parts, 0);
#endif

	if(nr_parts <= 0) {
		printk(KERN_NOTICE "Venus SFC: using single partition ");
		if(add_mtd_device(&descriptor)) {
			printk(KERN_WARNING "Venus SFC: (for SST/SPANSION/MXIC SPI-Flash) Failed to register new device\n");
			return -EAGAIN;
		}
	}
	else {
		printk(KERN_NOTICE "Venus SFC: using dynamic partition ");
		add_mtd_partitions(&descriptor, parts, nr_parts);
	}

	printk("Venus SFC: (for SST/SPANSION/MXIC SPI Flash)\n");
	return 0;
}

static void __exit sst_spi_flash_exit(void)
{
	/* put read opcode into control register */
	writel(0x00000003, SFC_OPCODE);
	writel(0x00000018, SFC_CTL);

	del_mtd_device(&descriptor);
}

module_init(sst_spi_flash_init);
module_exit(sst_spi_flash_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chih-pin Wu <wucp@realtek.com.tw>");
MODULE_DESCRIPTION("MTD chip driver for Realtek Venus Serial Flash Controller");
