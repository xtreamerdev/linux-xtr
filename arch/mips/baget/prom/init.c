/*
 * init.c: PROM library initialisation code.
 *
 * Copyright (C) 1998 Gleb Raiko & Vladimir Roganov 
 *
 * $Id: init.c,v 1.2 1999/08/17 22:18:38 ralf Exp $
 */
#include <linux/init.h>
#include <linux/config.h>
#include <asm/bootinfo.h>

char arcs_cmdline[CL_SIZE];

int __init prom_init(unsigned int mem_upper)
{
	mips_memory_upper = mem_upper;
	mips_machgroup  = MACH_GROUP_UNKNOWN;
	mips_machtype   = MACH_UNKNOWN;
	arcs_cmdline[0] = 0;
	return 0;
}

void __init prom_fixup_mem_map(unsigned long start, unsigned long end)
{
}

void prom_free_prom_memory (void)
{
}
