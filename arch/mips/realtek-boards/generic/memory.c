/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
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
 *
 * PROM library functions for acquiring/using memory descriptors given to
 * us from the YAMON.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#ifdef	CONFIG_REALTEK_RECLAIM_BOOT_MEM
#include <linux/swap.h>
#endif

#include <asm/bootinfo.h>
#include <asm/page.h>

#include <prom.h>
#include <platform.h>

extern char *find_args(char *args, char *name);

#ifdef	CONFIG_REALTEK_RECLAIM_BOOT_MEM
#define	F_ADDR1	0x80010000
#define	F_ADDR2	0x80020000
#define	T_ADDR1	0x80090000
#define	T_ADDR2	0x800f0000
#define	T_ADDR3	0x80100000
#endif
static unsigned int debug_flag = 0;
unsigned int audio_addr = 0;
/*#define DEBUG*/

enum yamon_memtypes {
	yamon_dontuse,
	yamon_prom,
	yamon_free,
};
struct prom_pmemblock mdesc[PROM_MAX_PMEMBLOCKS];

#ifdef DEBUG
static char *mtypes[3] = {
	"Dont use memory",
	"YAMON PROM memory",
	"Free memmory",
};
#endif

/* References to section boundaries */
extern char _end;

#define PFN_ALIGN(x)    (((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)

inline unsigned int reverseInt(unsigned int value)
{
	unsigned int b0 = value & 0x000000ff;
	unsigned int b1 = (value & 0x0000ff00) >> 8;
	unsigned int b2 = (value & 0x00ff0000) >> 16;
	unsigned int b3 = (value & 0xff000000) >> 24;

	return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

struct prom_pmemblock * __init prom_getmdesc(void)
{
	char *memsize_str;
	unsigned int memsize;
	char tmp_cmdline[COMMAND_LINE_SIZE];
	char *tmp_str;

	strlcpy(tmp_cmdline, arcs_cmdline, COMMAND_LINE_SIZE);
	tmp_str = find_args(tmp_cmdline, "debug_ros");
	if (tmp_str) {
		sscanf(tmp_str, "%x", &audio_addr);
		debug_flag = 1;
	}

	if (audio_addr == 0) {
		audio_addr = *((unsigned int *)0xa00000d8);
		audio_addr = reverseInt(audio_addr);
	}
	if ((audio_addr < 0x81000000) || (audio_addr >= 0x82000000)) {
		printk("**************************************************************\n");
		printk("You didn't specify audio image address, use default instead...\n");
		printk("**************************************************************\n");
		audio_addr = 0;
	}

	memsize_str = prom_getenv("memsize");
	if (!memsize_str) {
		prom_printf("memsize not set in boot prom, set to default (32Mb)\n");
		memsize = 0x02000000;
	} else {
#ifdef DEBUG
		prom_printf("prom_memsize = %s\n", memsize_str);
#endif
		memsize = simple_strtol(memsize_str, NULL, 0);
		if (memsize < 0x02000000) {
			panic("memsize cannot be less then 32Mb.\n");
		}
	}

	memset(mdesc, 0, sizeof(mdesc));

#ifdef CONFIG_REALTEK_MARS_RESERVE_LAST_48MB
	if (memsize == 0x10000000)
		memsize -= 0x03000000;
#endif

	// The last page is used to store the DRAM calibration parameter; 
	// Jacky says that this page can be included in his RTOS memory region.
//	mdesc[4].type = yamon_dontuse;
//	mdesc[4].base = memsize - 4096;
//	mdesc[4].size = 4096;

#if CONFIG_REALTEK_RTOS_MEMORY_SIZE
	// If memsize is over 32Mb
	if(memsize > 0x02000000) {
		mdesc[4].type = yamon_free;
		mdesc[4].base = 0x02000000;
		mdesc[4].size = memsize - 0x02000000;
	}

	// This memory region is used by RTOS.
	mdesc[3].type = yamon_dontuse;
//	mdesc[3].base = mdesc[4].base - CONFIG_REALTEK_RTOS_MEMORY_SIZE;
	if (audio_addr != 0) {
		audio_addr = audio_addr-0x80000000;
		mdesc[3].base = audio_addr;
		mdesc[3].size = 0x02000000 - audio_addr;
	} else {
		mdesc[3].base = 0x02000000 - CONFIG_REALTEK_RTOS_MEMORY_SIZE;
		mdesc[3].size = CONFIG_REALTEK_RTOS_MEMORY_SIZE;
	}
	printk("audio addr: %x \n", audio_addr);

	// Kernel image is stored in 0x100000 - CPHYSADDR(PFN_ALIGN(&_end)).
	mdesc[2].type = yamon_free;
	mdesc[2].base = CPHYSADDR(PFN_ALIGN(&_end));
	mdesc[2].size = mdesc[3].base - CPHYSADDR(PFN_ALIGN(&_end));
#else
	// Kernel image is stored in 0x100000 - CPHYSADDR(PFN_ALIGN(&_end)).
	mdesc[2].type = yamon_free;
	mdesc[2].base = CPHYSADDR(PFN_ALIGN(&_end));
	mdesc[2].size = memsize - CPHYSADDR(PFN_ALIGN(&_end));
#endif

	mdesc[1].type = yamon_dontuse;
	mdesc[1].base = 0x100000;
	mdesc[1].size = CPHYSADDR(PFN_ALIGN(&_end))-0x100000;

	mdesc[0].type = yamon_free;
	mdesc[0].base = 0x0;
	mdesc[0].size = 0x100000;

	return &mdesc[0];
}

static int __init prom_memtype_classify (unsigned int type)
{
	switch (type) {
	case yamon_free:
		return BOOT_MEM_RAM;
	case yamon_prom:
		return BOOT_MEM_ROM_DATA;
	default:
		return BOOT_MEM_RESERVED;
	}
}

void __init prom_meminit(void)
{
	struct prom_pmemblock *p;

#ifdef DEBUG
	prom_printf("YAMON MEMORY DESCRIPTOR dump:\n");
	p = prom_getmdesc();
	while (p->size) {
		int i = 0;
		prom_printf("[%d,%p]: base<%08lx> size<%08lx> type<%s>\n",
			    i, p, p->base, p->size, mtypes[p->type]);
		p++;
		i++;
	}
#endif
	p = prom_getmdesc();

	while (p->size) {
		long type;
		unsigned long base, size;

		type = prom_memtype_classify (p->type);
		base = p->base;
		size = p->size;

		add_memory_region(base, size, type);
                p++;
	}
}

unsigned long __init prom_free_prom_memory(void)
{
	unsigned long freed = 0;
	unsigned long addr;
	int i;
#ifdef	CONFIG_REALTEK_RECLAIM_BOOT_MEM
	unsigned long dest;
	struct page *page;
	int count;
#endif

	for (i = 0; i < boot_mem_map.nr_map; i++) {
		if (boot_mem_map.map[i].type != BOOT_MEM_ROM_DATA)
			continue;

		addr = boot_mem_map.map[i].addr;
		while (addr < boot_mem_map.map[i].addr
			      + boot_mem_map.map[i].size) {
			ClearPageReserved(virt_to_page(__va(addr)));
			set_page_count(virt_to_page(__va(addr)), 1);
			free_page((unsigned long)__va(addr));
			addr += PAGE_SIZE;
			freed += PAGE_SIZE;
		}
	}
	printk("Freeing prom memory: %ldkb freed\n", freed >> 10);
#ifdef	CONFIG_REALTEK_RECLAIM_BOOT_MEM
	if (!is_mars_cpu()) {
		// venus or neptune
		addr = F_ADDR1;
		if (debug_flag)
			dest = T_ADDR1;
		else
			dest = T_ADDR2;
	} else {
		// mars
		addr = F_ADDR2;
		if (debug_flag)
			dest = T_ADDR1;
		else
			dest = T_ADDR3;
	}
	printk("Reclaim bootloader memory from %x to %x\n", addr, dest);
	count = 0;
	while (addr < dest) {
		page = virt_to_page(addr);
/*
		printk("mem_map: %x, page: %x, size: %d \n", (int)mem_map, (int)page, sizeof(struct page));
		if (PageReserved(page) != 1)
			BUG();
		if (page->_count.counter != -1)
			BUG();
*/
		count++;

		__ClearPageReserved(page);
		set_page_count(page, 1);
		__free_page(page);

		addr += 0x1000; // 4KB
	}
	totalram_pages += count;
#endif
	return freed;
}
