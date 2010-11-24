/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/tty.h>

#ifdef CONFIG_MTD
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#endif

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
//#include <generic.h>
#include <prom.h>
#include <venus.h>
#include <asm/dma.h>
#include <asm/time.h>
#include <asm/traps.h>

extern void mips_reboot_setup(void);
extern void mips_time_init(void);
extern void mips_timer_setup(struct irqaction *irq);
extern unsigned long mips_rtc_get_time(void);
extern int mips_rtc_set_time(unsigned long);

#ifdef CONFIG_KGDB
extern void kgdb_config(void);
#endif

//struct resource standard_io_resources[] = {
//	{ "dma1", 0x00, 0x1f, IORESOURCE_BUSY },
//	{ "timer", 0x40, 0x5f, IORESOURCE_BUSY },
//	{ "keyboard", 0x60, 0x6f, IORESOURCE_BUSY },
//	{ "dma page reg", 0x80, 0x8f, IORESOURCE_BUSY },
//	{ "dma2", 0xc0, 0xdf, IORESOURCE_BUSY },
//};

#ifdef CONFIG_MTD
static struct mtd_partition venus_mtd_partitions[] = {
//	{
//		.name =		"Reserved",
//		.offset = 	0x0,
//		.size =		0x10000,
//	},
	{
		.name =		"User FS",
		.offset = 	0x0,
		.size =		0x2f0000,
	},
	{
		.name =		"YAMON",
		.offset =	0x2f0000,
		.size =		0x70000,
		.mask_flags =	MTD_WRITEABLE
	},
	{
		.name =		"User FS 2",
		.offset =	0x360000,
		.size =		0xa0000,
	}

};

#define number_partitions	(sizeof(venus_mtd_partitions)/sizeof(struct mtd_partition))
#endif

const char *get_system_type(void)
{
	return "Realtek Venus";
}

#if 0
void __init reset_ethernet(void) {

	printk("Set Ethernet RCR to 0x0.\n");
	writel(0x0, VENUS_ETH_RCR); //Set RX mode to 0.

	printk("Reset Ethernet Phy.\n");
	writel(0x84008000, VENUS_ETH_MIIAR); //Reset ethernet Phy.
	mb();
	while((readl(VENUS_ETH_MIIAR)&0x10000000));

	printk("Reset Ethernet Mac.\n");
	writeb(0x01, VENUS_ETH_CR); //Reset ethernet Mac.
	mb();
	while((readb(VENUS_ETH_CR)&0x01));

	return;
}
#endif /* 0 */

void __init plat_setup(void)
{
//	unsigned int i;
	unsigned int flashsize, flashstart;
	char *flashsize_str;

	/* Request I/O space for devices used on the Malta board. */
//	for (i = 0; i < ARRAY_SIZE(standard_io_resources); i++)
//		request_resource(&ioport_resource, standard_io_resources+i);

	/*
	 * Enable DMA channel 4 (cascade channel) in the PIIX4 south bridge.
	 */
//	enable_dma(4);

#ifdef CONFIG_KGDB
	kgdb_config ();
#endif

#ifdef CONFIG_MTD
	/*
	 * Support for MTD on Malta. Use the generic physmap driver
	 */
	flashsize_str = prom_getenv("flashsize");
	if(!flashsize_str) {
		prom_printf ("flashsize not set in boot prom, set to default (4Mb)\n");
		flashsize = 0x400000;
	} else {
		prom_printf("prom_flashsize = %s\n", flashsize_str);
		flashsize = simple_strtol(flashsize_str, NULL, 0);
	}
	if(flashsize < 0x100000)
		flashstart = 0x1fc00000;
	else
		flashstart = 0x1fd00000 - flashsize;
// If use this command here, the setting of CONFIG_MTD_PHYSMAP_START, CONFIG_MTD_PHYSMAP_LEN, ... will be lost.
//	physmap_configure(0x1e000000, 0x400000, 1, NULL);
	physmap_configure(flashstart, flashsize, 1, NULL);
//	physmap_set_partitions(venus_mtd_partitions, number_partitions);
#endif

	mips_reboot_setup();

	board_time_init = mips_time_init;
	board_timer_setup = mips_timer_setup;

	/* Reset ethernet at init stage. */
	//reset_ethernet();

	return;
}

//early_initcall(malta_setup);
