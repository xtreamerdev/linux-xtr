/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * console.c: SGI arcs console code.
 *
 * Copyright (C) 1996 David S. Miller (dm@sgi.com)
 */
#include <linux/init.h>
#include <asm/sgialib.h>

void __init prom_putchar(char c)
{
	long cnt;
	char it = c;

	romvec->write(1, &it, 1, &cnt);
}

char __init prom_getchar(void)
{
	long cnt;
	char c;

	romvec->read(0, &c, 1, &cnt);
	return c;
}
