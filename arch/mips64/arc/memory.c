/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by David S. Miller
 * Copyright (C) 1999, 2000, 2001 by Ralf Baechle
 * Copyright (C) 1999, 2000 by Silicon Graphics, Inc.
 *
 * PROM library functions for acquiring/using memory descriptors given to us
 * from the ARCS firmware.  This is only used when CONFIG_ARC_MEMORY is set
 * because on some machines like SGI IP27 the ARC memory configuration data
 * completly bogus and alternate easier to use mechanisms are available.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/swap.h>

#include <asm/sgialib.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/bootinfo.h>

#undef DEBUG

struct linux_mdesc * __init
ArcGetMemoryDescriptor(struct linux_mdesc *Current)
{
	return (struct linux_mdesc *) ARC_CALL1(get_mdesc, Current);
}

#ifdef DEBUG /* convenient for debugging */
static char *arcs_mtypes[8] = {
	"Exception Block",
	"ARCS Romvec Page",
	"Free/Contig RAM",
	"Generic Free RAM",
	"Bad Memory",
	"Standalone Program Pages",
	"ARCS Temp Storage Area",
	"ARCS Permanent Storage Area"
};

static char *arc_mtypes[8] = {
	"Exception Block",
	"SystemParameterBlock",
	"FreeMemory",
	"Bad Memory",
	"LoadedProgram",
	"FirmwareTemporary",
	"FirmwarePermanent",
	"FreeContiguous"
};
#define mtypes(a) (prom_flags & PROM_FLAG_ARCS) ? arcs_mtypes[a.arcs] : arc_mtypes[a.arc]
#endif

static struct prom_pmemblock pblocks[PROM_MAX_PMEMBLOCKS];

#define MEMTYPE_DONTUSE   0
#define MEMTYPE_PROM      1
#define MEMTYPE_FREE      2

static inline int memtype_classify_arcs (union linux_memtypes type)
{
	switch (type.arcs) {
	case arcs_fcontig:
	case arcs_free:
		return MEMTYPE_FREE;
	case arcs_atmp:
		return MEMTYPE_PROM;
	case arcs_eblock:
	case arcs_rvpage:
	case arcs_bmem:
	case arcs_prog:
	case arcs_aperm:
		return MEMTYPE_DONTUSE;
	default:
		BUG();
	}
	while(1);				/* Nuke warning.  */
}

static inline int memtype_classify_arc (union linux_memtypes type)
{
	switch (type.arc) {
	case arc_free:
	case arc_fcontig:
		return BOOT_MEM_RAM;
	case arc_atmp:
		return BOOT_MEM_ROM_DATA;
	case arc_eblock:
	case arc_rvpage:
	case arc_bmem:
	case arc_prog:
	case arc_aperm:
		return BOOT_MEM_RESERVED;
	default:
		BUG();
	}
	while(1);				/* Nuke warning.  */
}

static int __init prom_memtype_classify (union linux_memtypes type)
{
	if (prom_flags & PROM_FLAG_ARCS)	/* SGI is ``different'' ...  */
		return memtype_classify_arcs(type);

	return memtype_classify_arc(type);
}

void __init prom_meminit(void)
{
	struct linux_mdesc *p;

#ifdef DEBUG
	int i = 0;

	prom_printf("ARCS MEMORY DESCRIPTOR dump:\n");
	i=0;
	prom_printf ("i=%d\n", i);
	p = ArcGetMemoryDescriptor(PROM_NULL_MDESC);
	prom_printf ("i=%d\n", i);
	while(p) {
		prom_printf("[%d,%p]: base<%08lx> pages<%08lx> type<%s>\n",
			    i, p, p->base, p->pages, mtypes(p->type));
		p = ArcGetMemoryDescriptor(p);
		i++;
	}
#endif

	p = PROM_NULL_MDESC;
	while ((p = ArcGetMemoryDescriptor(p))) {
		unsigned long base, size;
		long type;

		base = p->base << PAGE_SHIFT;
		size = p->pages << PAGE_SHIFT;
		type = prom_memtype_classify(p->type);

		add_memory_region(base, size, type);
	}
}

void __init
prom_free_prom_memory (void)
{
	struct prom_pmemblock *p;
	unsigned long freed = 0;
	unsigned long addr;
	int i;

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
}
