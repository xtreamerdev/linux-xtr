/*
 * pSeries NUMA support
 *
 * Copyright (C) 2002 Anton Blanchard <anton@au.ibm.com>, IBM
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <linux/threads.h>
#include <linux/bootmem.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <asm/lmb.h>

#if 1
#define dbg(args...) udbg_printf(args)
#else
#define dbg(args...)
#endif

int numa_cpu_lookup_table[NR_CPUS] = { [ 0 ... (NR_CPUS - 1)] = -1};
int numa_memory_lookup_table[MAX_MEMORY >> MEMORY_INCREMENT_SHIFT] =
	{ [ 0 ... ((MAX_MEMORY >> MEMORY_INCREMENT_SHIFT) - 1)] = -1};
int numa_node_exists[MAX_NUMNODES];

struct pglist_data node_data[MAX_NUMNODES];
bootmem_data_t plat_node_bdata[MAX_NUMNODES];

static int __init parse_numa_properties(void)
{
	struct device_node *cpu;
	struct device_node *memory;
	int *cpu_associativity;
	int *memory_associativity;
	int depth;
	int max_domain = 0;

	cpu = find_type_devices("cpu");
	if (!cpu)
		return -1;

	memory = find_type_devices("memory");
	if (!memory)
		return -1;

	cpu_associativity = (int *)get_property(cpu, "ibm,associativity", NULL);
	if (!cpu_associativity)
		return -1;

	memory_associativity = (int *)get_property(memory, "ibm,associativity",
						   NULL);
	if (!memory_associativity)
		return -1;

	/* find common depth */
	if (cpu_associativity[0] < memory_associativity[0])
		depth = cpu_associativity[0];
	else
		depth = memory_associativity[0];

	for (cpu = find_type_devices("cpu"); cpu; cpu = cpu->next) {
		int *tmp;
		int cpu_nr, numa_domain;

		tmp = (int *)get_property(cpu, "reg", NULL);
		if (!tmp)
			continue;
		cpu_nr = *tmp;

		tmp = (int *)get_property(cpu, "ibm,associativity",
					  NULL);
		if (!tmp)
			continue;
		numa_domain = tmp[depth];

		/* FIXME */
		if (numa_domain == 0xffff) {
			dbg("cpu %d has no numa doman\n", cpu_nr);
			numa_domain = 0;
		}

		if (numa_domain >= MAX_NUMNODES)
			BUG();

		if (max_domain < numa_domain)
			max_domain = numa_domain;

		numa_cpu_lookup_table[cpu_nr] = numa_domain;

		dbg("cpu %d maps to domain %d\n", cpu_nr, numa_domain);
	}

	for (memory = find_type_devices("memory"); memory;
	     memory = memory->next) {
		int *tmp1, *tmp2;
		unsigned long i;
		unsigned long start = 0;
		unsigned long size = 0;
		int numa_domain;
		int ranges;

		tmp1 = (int *)get_property(memory, "reg", NULL);
		if (!tmp1)
			continue;

		ranges = memory->n_addrs;
new_range:

		i = prom_n_size_cells(memory);
		while (i--) {
			start = (start << 32) | *tmp1;
			tmp1++;
		}

		i = prom_n_size_cells(memory);
		while (i--) {
			size = (size << 32) | *tmp1;
			tmp1++;
		}

		start = _ALIGN_DOWN(start, MEMORY_INCREMENT);
		size = _ALIGN_UP(size, MEMORY_INCREMENT);

		if ((start + size) > MAX_MEMORY)
			BUG();

		tmp2 = (int *)get_property(memory, "ibm,associativity",
					   NULL);
		if (!tmp2)
			continue;
		numa_domain = tmp2[depth];

		/* FIXME */
		if (numa_domain == 0xffff) {
			dbg("cpu has no numa doman\n");
			numa_domain = 0;
		}

		if (numa_domain >= MAX_NUMNODES)
			BUG();

		if (max_domain < numa_domain)
			max_domain = numa_domain;

		numa_node_exists[numa_domain] = 1;

		for (i = start ; i < (start+size); i += MEMORY_INCREMENT)
			numa_memory_lookup_table[i >> MEMORY_INCREMENT_SHIFT] =
				numa_domain;

		dbg("memory region %lx to %lx maps to domain %d\n",
		    start, start+size, numa_domain);

		ranges--;
		if (ranges)
			goto new_range;
	}

	numnodes = max_domain + 1;

	return 0;
}

void __init do_init_bootmem(void)
{
	int nid;

	min_low_pfn = 0;
	max_low_pfn = lmb_end_of_DRAM() >> PAGE_SHIFT;

	/* XXX FIXME: support machines without associativity information */
	if (parse_numa_properties())
		BUG();

	for (nid = 0; nid < numnodes; nid++) {
		unsigned long start, end;
		unsigned long start_paddr, end_paddr;
		int i;
		unsigned long bootmem_paddr;
		unsigned long bootmap_pages;

		if (!numa_node_exists[nid])
			continue;

		/* Find start and end of this zone */
		start = 0;
		while (numa_memory_lookup_table[start] != nid)
			start++;

		end = (MAX_MEMORY >> MEMORY_INCREMENT_SHIFT) - 1;
		while (numa_memory_lookup_table[end] != nid)
			end--;
		end++;

		start_paddr = start << MEMORY_INCREMENT_SHIFT;
		end_paddr = end << MEMORY_INCREMENT_SHIFT;

		dbg("node %d\n", nid);
		dbg("start_paddr = %lx\n", start_paddr);
		dbg("end_paddr = %lx\n", end_paddr);

		NODE_DATA(nid)->bdata = &plat_node_bdata[nid];

		bootmap_pages = bootmem_bootmap_pages(end_paddr - start_paddr);
		dbg("bootmap_pages = %lx\n", bootmap_pages);

		bootmem_paddr = lmb_alloc_base(bootmap_pages << PAGE_SHIFT,
				PAGE_SIZE, end_paddr);
		dbg("bootmap_paddr = %lx\n", bootmem_paddr);

		init_bootmem_node(NODE_DATA(nid), bootmem_paddr >> PAGE_SHIFT,
				  start_paddr >> PAGE_SHIFT,
				  end_paddr >> PAGE_SHIFT);

		for (i = 0; i < lmb.memory.cnt; i++) {
			unsigned long physbase, size;
			unsigned long type = lmb.memory.region[i].type;

			if (type != LMB_MEMORY_AREA)
				continue;

			physbase = lmb.memory.region[i].physbase;
			size = lmb.memory.region[i].size;

			if (physbase < end_paddr &&
			    (physbase+size) > start_paddr) {
				/* overlaps */
				if (physbase < start_paddr) {
					size -= start_paddr - physbase;
					physbase = start_paddr;
				}

				if (size > end_paddr - start_paddr)
					size = end_paddr - start_paddr;

				dbg("free_bootmem %lx %lx\n", physbase, size);
				free_bootmem_node(NODE_DATA(nid), physbase,
						  size);
			}
		}

		for (i = 0; i < lmb.reserved.cnt; i++) {
			unsigned long physbase = lmb.reserved.region[i].physbase;
			unsigned long size = lmb.reserved.region[i].size;

			if (physbase < end_paddr &&
			    (physbase+size) > start_paddr) {
				/* overlaps */
				if (physbase < start_paddr) {
					size -= start_paddr - physbase;
					physbase = start_paddr;
				}

				if (size > end_paddr - start_paddr)
					size = end_paddr - start_paddr;

				dbg("reserve_bootmem %lx %lx\n", physbase,
				    size);
				reserve_bootmem_node(NODE_DATA(nid), physbase,
						     size);
			}
		}
	}
}

void __init paging_init(void)
{
	unsigned long zones_size[MAX_NR_ZONES];
	int i, nid;

	for (i = 1; i < MAX_NR_ZONES; i++)
		zones_size[i] = 0;

	for (nid = 0; nid < numnodes; nid++) {
		unsigned long start_pfn;
		unsigned long end_pfn;

		if (!numa_node_exists[nid])
			continue;

		start_pfn = plat_node_bdata[nid].node_boot_start >> PAGE_SHIFT;
		end_pfn = plat_node_bdata[nid].node_low_pfn;

		zones_size[ZONE_DMA] = end_pfn - start_pfn;
		dbg("free_area_init node %d %lx %lx\n", nid,
				zones_size[ZONE_DMA], start_pfn);
		free_area_init_node(nid, NODE_DATA(nid), NULL, zones_size,
				    start_pfn, NULL);
	}
}
