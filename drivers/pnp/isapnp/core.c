/*
 *  ISA Plug & Play support
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Changelog:
 *  2000-01-01	Added quirks handling for buggy hardware
 *		Peter Denison <peterd@pnd-pc.demon.co.uk>
 *  2000-06-14	Added isapnp_probe_devs() and isapnp_activate_dev()
 *		Christoph Hellwig <hch@infradead.org>
 *  2001-06-03  Added release_region calls to correspond with
 *		request_region calls when a failure occurs.  Also
 *		added KERN_* constants to printk() calls.
 *  2001-11-07  Added isapnp_{,un}register_driver calls along the lines
 *              of the pci driver interface
 *              Kai Germaschewski <kai.germaschewski@gmx.de>
 *  2002-06-06  Made the use of dma channel 0 configurable
 *              Gerald Teschl <gerald.teschl@univie.ac.at>
 *  2002-10-06  Ported to PnP Layer - Adam Belay <ambx1@neo.rr.com>
 */

#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/isapnp.h>
#include <linux/pnp.h>

LIST_HEAD(isapnp_cards);
LIST_HEAD(isapnp_devices);

#if 0
#define ISAPNP_REGION_OK
#endif
#if 0
#define ISAPNP_DEBUG
#endif

int isapnp_disable;			/* Disable ISA PnP */
int isapnp_rdp;				/* Read Data Port */
int isapnp_reset = 1;			/* reset all PnP cards (deactivate) */
int isapnp_skip_pci_scan;		/* skip PCI resource scanning */
int isapnp_verbose = 1;			/* verbose mode */

MODULE_AUTHOR("Jaroslav Kysela <perex@suse.cz>");
MODULE_DESCRIPTION("Generic ISA Plug & Play support");
MODULE_PARM(isapnp_disable, "i");
MODULE_PARM_DESC(isapnp_disable, "ISA Plug & Play disable");
MODULE_PARM(isapnp_rdp, "i");
MODULE_PARM_DESC(isapnp_rdp, "ISA Plug & Play read data port");
MODULE_PARM(isapnp_reset, "i");
MODULE_PARM_DESC(isapnp_reset, "ISA Plug & Play reset all cards");
MODULE_PARM(isapnp_allow_dma0, "i");
MODULE_PARM(isapnp_skip_pci_scan, "i");
MODULE_PARM_DESC(isapnp_skip_pci_scan, "ISA Plug & Play skip PCI resource scanning");
MODULE_PARM(isapnp_verbose, "i");
MODULE_PARM_DESC(isapnp_verbose, "ISA Plug & Play verbose mode");
MODULE_LICENSE("GPL");

#define _PIDXR		0x279
#define _PNPWRP		0xa79

/* short tags */
#define _STAG_PNPVERNO		0x01
#define _STAG_LOGDEVID		0x02
#define _STAG_COMPATDEVID	0x03
#define _STAG_IRQ		0x04
#define _STAG_DMA		0x05
#define _STAG_STARTDEP		0x06
#define _STAG_ENDDEP		0x07
#define _STAG_IOPORT		0x08
#define _STAG_FIXEDIO		0x09
#define _STAG_VENDOR		0x0e
#define _STAG_END		0x0f
/* long tags */
#define _LTAG_MEMRANGE		0x81
#define _LTAG_ANSISTR		0x82
#define _LTAG_UNICODESTR	0x83
#define _LTAG_VENDOR		0x84
#define _LTAG_MEM32RANGE	0x85
#define _LTAG_FIXEDMEM32RANGE	0x86

static unsigned char isapnp_checksum_value;
static DECLARE_MUTEX(isapnp_cfg_mutex);
static int isapnp_detected;

/* some prototypes */

static int isapnp_config_prepare(struct pnp_dev *dev);

static inline void write_data(unsigned char x)
{
	outb(x, _PNPWRP);
}

static inline void write_address(unsigned char x)
{
	outb(x, _PIDXR);
	udelay(20);
}

static inline unsigned char read_data(void)
{
	unsigned char val = inb(isapnp_rdp);
	return val;
}

unsigned char isapnp_read_byte(unsigned char idx)
{
	write_address(idx);
	return read_data();
}

unsigned short isapnp_read_word(unsigned char idx)
{
	unsigned short val;

	val = isapnp_read_byte(idx);
	val = (val << 8) + isapnp_read_byte(idx+1);
	return val;
}

unsigned int isapnp_read_dword(unsigned char idx)
{
	unsigned int val;

	val = isapnp_read_byte(idx);
	val = (val << 8) + isapnp_read_byte(idx+1);
	val = (val << 8) + isapnp_read_byte(idx+2);
	val = (val << 8) + isapnp_read_byte(idx+3);
	return val;
}

void isapnp_write_byte(unsigned char idx, unsigned char val)
{
	write_address(idx);
	write_data(val);
}

void isapnp_write_word(unsigned char idx, unsigned short val)
{
	isapnp_write_byte(idx, val >> 8);
	isapnp_write_byte(idx+1, val);
}

void isapnp_write_dword(unsigned char idx, unsigned int val)
{
	isapnp_write_byte(idx, val >> 24);
	isapnp_write_byte(idx+1, val >> 16);
	isapnp_write_byte(idx+2, val >> 8);
	isapnp_write_byte(idx+3, val);
}

void *isapnp_alloc(long size)
{
	void *result;

	result = kmalloc(size, GFP_KERNEL);
	if (!result)
		return NULL;
	memset(result, 0, size);
	return result;
}

static void isapnp_key(void)
{
	unsigned char code = 0x6a, msb;
	int i;

	mdelay(1);
	write_address(0x00);
	write_address(0x00);

	write_address(code);

	for (i = 1; i < 32; i++) {
		msb = ((code & 0x01) ^ ((code & 0x02) >> 1)) << 7;
		code = (code >> 1) | msb;
		write_address(code);
	}
}

/* place all pnp cards in wait-for-key state */
static void isapnp_wait(void)
{
	isapnp_write_byte(0x02, 0x02);
}

void isapnp_wake(unsigned char csn)
{
	isapnp_write_byte(0x03, csn);
}

void isapnp_device(unsigned char logdev)
{
	isapnp_write_byte(0x07, logdev);
}

void isapnp_activate(unsigned char logdev)
{
	isapnp_device(logdev);
	isapnp_write_byte(ISAPNP_CFG_ACTIVATE, 1);
	udelay(250);
}

void isapnp_deactivate(unsigned char logdev)
{
	isapnp_device(logdev);
	isapnp_write_byte(ISAPNP_CFG_ACTIVATE, 0);
	udelay(500);
}

static void __init isapnp_peek(unsigned char *data, int bytes)
{
	int i, j;
	unsigned char d=0;

	for (i = 1; i <= bytes; i++) {
		for (j = 0; j < 20; j++) {
			d = isapnp_read_byte(0x05);
			if (d & 1)
				break;
			udelay(100);
		}
		if (!(d & 1)) {
			if (data != NULL)
				*data++ = 0xff;
			continue;
		}
		d = isapnp_read_byte(0x04);	/* PRESDI */
		isapnp_checksum_value += d;
		if (data != NULL)
			*data++ = d;
	}
}

#define RDP_STEP	32	/* minimum is 4 */

static int isapnp_next_rdp(void)
{
	int rdp = isapnp_rdp;
	while (rdp <= 0x3ff) {
		/*
		 *	We cannot use NE2000 probe spaces for ISAPnP or we
		 *	will lock up machines.
		 */
		if ((rdp < 0x280 || rdp >  0x380) && !check_region(rdp, 1)) 
		{
			isapnp_rdp = rdp;
			return 0;
		}
		rdp += RDP_STEP;
	}
	return -1;
}

/* Set read port address */
static inline void isapnp_set_rdp(void)
{
	isapnp_write_byte(0x00, isapnp_rdp >> 2);
	udelay(100);
}

/*
 *	Perform an isolation. The port selection code now tries to avoid
 *	"dangerous to read" ports.
 */

static int __init isapnp_isolate_rdp_select(void)
{
	isapnp_wait();
	isapnp_key();

	/* Control: reset CSN and conditionally everything else too */
	isapnp_write_byte(0x02, isapnp_reset ? 0x05 : 0x04);
	mdelay(2);

	isapnp_wait();
	isapnp_key();
	isapnp_wake(0x00);

	if (isapnp_next_rdp() < 0) {
		isapnp_wait();
		return -1;
	}

	isapnp_set_rdp();
	udelay(1000);
	write_address(0x01);
	udelay(1000);
	return 0;
}

/*
 *  Isolate (assign uniqued CSN) to all ISA PnP devices.
 */

static int __init isapnp_isolate(void)
{
	unsigned char checksum = 0x6a;
	unsigned char chksum = 0x00;
	unsigned char bit = 0x00;
	int data;
	int csn = 0;
	int i;
	int iteration = 1;

	isapnp_rdp = 0x213;
	if (isapnp_isolate_rdp_select() < 0)
		return -1;

	while (1) {
		for (i = 1; i <= 64; i++) {
			data = read_data() << 8;
			udelay(250);
			data = data | read_data();
			udelay(250);
			if (data == 0x55aa)
				bit = 0x01;
			checksum = ((((checksum ^ (checksum >> 1)) & 0x01) ^ bit) << 7) | (checksum >> 1);
			bit = 0x00;
		}
		for (i = 65; i <= 72; i++) {
			data = read_data() << 8;
			udelay(250);
			data = data | read_data();
			udelay(250);
			if (data == 0x55aa)
				chksum |= (1 << (i - 65));
		}
		if (checksum != 0x00 && checksum == chksum) {
			csn++;

			isapnp_write_byte(0x06, csn);
			udelay(250);
			iteration++;
			isapnp_wake(0x00);
			isapnp_set_rdp();
			udelay(1000);
			write_address(0x01);
			udelay(1000);
			goto __next;
		}
		if (iteration == 1) {
			isapnp_rdp += RDP_STEP;
			if (isapnp_isolate_rdp_select() < 0)
				return -1;
		} else if (iteration > 1) {
			break;
		}
	      __next:
		checksum = 0x6a;
		chksum = 0x00;
		bit = 0x00;
	}
	isapnp_wait();
	return csn;
}

/*
 *  Read one tag from stream.
 */

static int __init isapnp_read_tag(unsigned char *type, unsigned short *size)
{
	unsigned char tag, tmp[2];

	isapnp_peek(&tag, 1);
	if (tag == 0)				/* invalid tag */
		return -1;
	if (tag & 0x80) {	/* large item */
		*type = tag;
		isapnp_peek(tmp, 2);
		*size = (tmp[1] << 8) | tmp[0];
	} else {
		*type = (tag >> 3) & 0x0f;
		*size = tag & 0x07;
	}
#if 0
	printk(KERN_DEBUG "tag = 0x%x, type = 0x%x, size = %i\n", tag, *type, *size);
#endif
	if (type == 0)				/* wrong type */
		return -1;
	if (*type == 0xff && *size == 0xffff)	/* probably invalid data */
		return -1;
	return 0;
}

/*
 *  Skip specified number of bytes from stream.
 */

static void __init isapnp_skip_bytes(int count)
{
	isapnp_peek(NULL, count);
}

/*
 *  Parse EISA id.
 */

static void isapnp_parse_id(struct pnp_dev * dev, unsigned short vendor, unsigned short device)
{
	struct pnp_id * id = isapnp_alloc(sizeof(struct pnp_id));
	if (!id)
		return;
	if (!dev)
		return;
	sprintf(id->id, "%c%c%c%x%x%x%x",
			'A' + ((vendor >> 2) & 0x3f) - 1,
			'A' + (((vendor & 3) << 3) | ((vendor >> 13) & 7)) - 1,
			'A' + ((vendor >> 8) & 0x1f) - 1,
			(device >> 4) & 0x0f,
			device & 0x0f,
			(device >> 12) & 0x0f,
			(device >> 8) & 0x0f);
	pnp_add_id(id, dev);
}

/*
 *  Parse logical device tag.
 */

static struct pnp_dev * __init isapnp_parse_device(struct pnp_card *card, int size, int number)
{
	unsigned char tmp[6];
	struct pnp_dev *dev;

	isapnp_peek(tmp, size);
	dev = isapnp_alloc(sizeof(struct pnp_dev));
	if (!dev)
		return NULL;
	pnp_init_device(dev);
	dev->number = number;
	isapnp_parse_id(dev, (tmp[1] << 8) | tmp[0], (tmp[3] << 8) | tmp[2]);
	dev->regs = tmp[4];
	dev->card = card;
	if (size > 5)
		dev->regs |= tmp[5] << 8;
	return dev;
}


/*
 *  Add IRQ resource to resources list.
 */

static void __init isapnp_add_irq_resource(struct pnp_dev *dev,
                                               int depnum, int size)
{
	unsigned char tmp[3];
	struct pnp_irq *irq;

	isapnp_peek(tmp, size);
	irq = isapnp_alloc(sizeof(struct pnp_irq));
	if (!irq)
		return;
	irq->map = (tmp[1] << 8) | tmp[0];
	if (size > 2)
		irq->flags = tmp[2];
	else
		irq->flags = IORESOURCE_IRQ_HIGHEDGE;
	pnp_add_irq_resource(dev, depnum, irq);
	return;
}

/*
 *  Add DMA resource to resources list.
 */

static void __init isapnp_add_dma_resource(struct pnp_dev *dev,
                                    	       int depnum, int size)
{
	unsigned char tmp[2];
	struct pnp_dma *dma;

	isapnp_peek(tmp, size);
	dma = isapnp_alloc(sizeof(struct pnp_dma));
	if (!dma)
		return;
	dma->map = tmp[0];
	dma->flags = tmp[1];
	pnp_add_dma_resource(dev, depnum, dma);
	return;
}

/*
 *  Add port resource to resources list.
 */

static void __init isapnp_add_port_resource(struct pnp_dev *dev,
						int depnum, int size)
{
	unsigned char tmp[7];
	struct pnp_port *port;

	isapnp_peek(tmp, size);
	port = isapnp_alloc(sizeof(struct pnp_port));
	if (!port)
		return;
	port->min = (tmp[2] << 8) | tmp[1];
	port->max = (tmp[4] << 8) | tmp[3];
	port->align = tmp[5];
	port->size = tmp[6];
	port->flags = tmp[0] ? ISAPNP_PORT_FLAG_16BITADDR : 0;
	pnp_add_port_resource(dev,depnum,port);
	return;
}

/*
 *  Add fixed port resource to resources list.
 */

static void __init isapnp_add_fixed_port_resource(struct pnp_dev *dev,
						      int depnum, int size)
{
	unsigned char tmp[3];
	struct pnp_port *port;

	isapnp_peek(tmp, size);
	port = isapnp_alloc(sizeof(struct pnp_port));
	if (!port)
		return;
	port->min = port->max = (tmp[1] << 8) | tmp[0];
	port->size = tmp[2];
	port->align = 0;
	port->flags = ISAPNP_PORT_FLAG_FIXED;
	pnp_add_port_resource(dev,depnum,port);
	return;
}

/*
 *  Add memory resource to resources list.
 */

static void __init isapnp_add_mem_resource(struct pnp_dev *dev,
					       int depnum, int size)
{
	unsigned char tmp[9];
	struct pnp_mem *mem;

	isapnp_peek(tmp, size);
	mem = isapnp_alloc(sizeof(struct pnp_mem));
	if (!mem)
		return;
	mem->min = ((tmp[2] << 8) | tmp[1]) << 8;
	mem->max = ((tmp[4] << 8) | tmp[3]) << 8;
	mem->align = (tmp[6] << 8) | tmp[5];
	mem->size = ((tmp[8] << 8) | tmp[7]) << 8;
	mem->flags = tmp[0];
	pnp_add_mem_resource(dev,depnum,mem);
	return;
}

/*
 *  Add 32-bit memory resource to resources list.
 */

static void __init isapnp_add_mem32_resource(struct pnp_dev *dev,
						 int depnum, int size)
{
	unsigned char tmp[17];
	struct pnp_mem32 *mem32;

	isapnp_peek(tmp, size);
	mem32 = isapnp_alloc(sizeof(struct pnp_mem32));
	if (!mem32)
		return;
	memcpy(mem32->data, tmp, 17);
	pnp_add_mem32_resource(dev,depnum,mem32);
}

/*
 *  Add 32-bit fixed memory resource to resources list.
 */

static void __init isapnp_add_fixed_mem32_resource(struct pnp_dev *dev,
						       int depnum, int size)
{
	unsigned char tmp[17];
	struct pnp_mem32 *mem32;

	isapnp_peek(tmp, size);
	mem32 = isapnp_alloc(sizeof(struct pnp_mem32));
	if (!mem32)
		return;
	memcpy(mem32->data, tmp, 17);
	pnp_add_mem32_resource(dev,depnum,mem32);
}

/*
 *  Parse card name for ISA PnP device.
 */ 
 
static void __init 
isapnp_parse_name(char *name, unsigned int name_max, unsigned short *size)
{
	if (name[0] == '\0') {
		unsigned short size1 = *size >= name_max ? (name_max - 1) : *size;
		isapnp_peek(name, size1);
		name[size1] = '\0';
		*size -= size1;
		
		/* clean whitespace from end of string */
		while (size1 > 0  &&  name[--size1] == ' ')
			name[size1] = '\0';
	}	
}

/*
 *  Parse resource map for logical device.
 */

static int __init isapnp_create_device(struct pnp_card *card,
					   unsigned short size)
{
	int number = 0, skip = 0, depnum = 0, dependent = 0, compat = 0;
	unsigned char type, tmp[17];
	struct pnp_dev *dev;
	if ((dev = isapnp_parse_device(card, size, number++)) == NULL)
		return 1;
	if (pnp_build_resource(dev, 0) == NULL)
		return 1;
	list_add_tail(&dev->card_list, &card->devices);
	while (1) {
		if (isapnp_read_tag(&type, &size)<0)
			return 1;
		if (skip && type != _STAG_LOGDEVID && type != _STAG_END)
			goto __skip;
		switch (type) {
		case _STAG_LOGDEVID:
			if (size >= 5 && size <= 6) {
				isapnp_config_prepare(dev);
				if ((dev = isapnp_parse_device(card, size, number++)) == NULL)
					return 1;
				pnp_build_resource(dev,0);
				list_add_tail(&dev->card_list, &card->devices);
				size = 0;
				skip = 0;
			} else {
				skip = 1;
			}
			dependent = 0;
			depnum = 0;
			compat = 0;
			break;
		case _STAG_COMPATDEVID:
			if (size == 4 && compat < DEVICE_COUNT_COMPATIBLE) {
				isapnp_peek(tmp, 4);
				isapnp_parse_id(dev,(tmp[1] << 8) | tmp[0], (tmp[3] << 8) | tmp[2]);
				compat++;
				size = 0;
			}
			break;
		case _STAG_IRQ:
			if (size < 2 || size > 3)
				goto __skip;
			isapnp_add_irq_resource(dev, depnum, size);
			size = 0;
			break;
		case _STAG_DMA:
			if (size != 2)
				goto __skip;
			isapnp_add_dma_resource(dev, depnum, size);
			size = 0;
			break;
		case _STAG_STARTDEP:
			if (size > 1)
				goto __skip;
			dependent = 0x100 | ISAPNP_RES_PRIORITY_ACCEPTABLE;
			if (size > 0) {
				isapnp_peek(tmp, size);
				dependent = 0x100 | tmp[0];
				size = 0;
			}
			pnp_build_resource(dev,dependent);
			depnum = pnp_get_max_depnum(dev);
			break;
		case _STAG_ENDDEP:
			if (size != 0)
				goto __skip;
			dependent = 0;
			depnum = 0;
			break;
		case _STAG_IOPORT:
			if (size != 7)
				goto __skip;
			isapnp_add_port_resource(dev, depnum, size);
			size = 0;
			break;
		case _STAG_FIXEDIO:
			if (size != 3)
				goto __skip;
			isapnp_add_fixed_port_resource(dev, depnum, size);
			size = 0;
			break;
		case _STAG_VENDOR:
			break;
		case _LTAG_MEMRANGE:
			if (size != 9)
				goto __skip;
			isapnp_add_mem_resource(dev, depnum, size);
			size = 0;
			break;
		case _LTAG_ANSISTR:
			isapnp_parse_name(dev->name, sizeof(dev->name), &size);
			break;
		case _LTAG_UNICODESTR:
			/* silently ignore */
			/* who use unicode for hardware identification? */
			break;
		case _LTAG_VENDOR:
			break;
		case _LTAG_MEM32RANGE:
			if (size != 17)
				goto __skip;
			isapnp_add_mem32_resource(dev, depnum, size);
			size = 0;
			break;
		case _LTAG_FIXEDMEM32RANGE:
			if (size != 17)
				goto __skip;
			isapnp_add_fixed_mem32_resource(dev, depnum, size);
			size = 0;
			break;
		case _STAG_END:
			if (size > 0)
				isapnp_skip_bytes(size);
			isapnp_config_prepare(dev);
			return 1;
		default:
			printk(KERN_ERR "isapnp: unexpected or unknown tag type 0x%x for logical device %i (device %i), ignored\n", type, dev->number, card->number);
		}
	      __skip:
	      	if (size > 0)
		      	isapnp_skip_bytes(size);
	}
	isapnp_config_prepare(dev);
	return 0;
}

/*
 *  Parse resource map for ISA PnP card.
 */
 
static void __init isapnp_parse_resource_map(struct pnp_card *card)
{
	unsigned char type, tmp[17];
	unsigned short size;

	while (1) {
		if (isapnp_read_tag(&type, &size)<0)
			return;
		switch (type) {
		case _STAG_PNPVERNO:
			if (size != 2)
				goto __skip;
			isapnp_peek(tmp, 2);
			card->pnpver = tmp[0];
			card->productver = tmp[1];
			size = 0;
			break;
		case _STAG_LOGDEVID:
			if (size >= 5 && size <= 6) {
				if (isapnp_create_device(card, size)==1)
					return;
				size = 0;
			}
			break;
		case _STAG_VENDOR:
			break;
		case _LTAG_ANSISTR:
			isapnp_parse_name(card->name, sizeof(card->name), &size);
			break;
		case _LTAG_UNICODESTR:
			/* silently ignore */
			/* who use unicode for hardware identification? */
			break;
		case _LTAG_VENDOR:
			break;
		case _STAG_END:
			if (size > 0)
				isapnp_skip_bytes(size);
			return;
		default:
			printk(KERN_ERR "isapnp: unexpected or unknown tag type 0x%x for device %i, ignored\n", type, card->number);
		}
	      __skip:
	      	if (size > 0)
		      	isapnp_skip_bytes(size);
	}
}

/*
 *  Compute ISA PnP checksum for first eight bytes.
 */

static unsigned char __init isapnp_checksum(unsigned char *data)
{
	int i, j;
	unsigned char checksum = 0x6a, bit, b;
	
	for (i = 0; i < 8; i++) {
		b = data[i];
		for (j = 0; j < 8; j++) {
			bit = 0;
			if (b & (1 << j))
				bit = 1;
			checksum = ((((checksum ^ (checksum >> 1)) & 0x01) ^ bit) << 7) | (checksum >> 1);
		}
	}
	return checksum;
}

/*
 *  Parse EISA id for ISA PnP card.
 */

static void isapnp_parse_card_id(struct pnp_card * card, unsigned short vendor, unsigned short device)
{
	struct pnp_id * id = isapnp_alloc(sizeof(struct pnp_id));
	if (!id)
		return;
	sprintf(id->id, "%c%c%c%x%x%x%x",
			'A' + ((vendor >> 2) & 0x3f) - 1,
			'A' + (((vendor & 3) << 3) | ((vendor >> 13) & 7)) - 1,
			'A' + ((vendor >> 8) & 0x1f) - 1,
			(device >> 4) & 0x0f,
			device & 0x0f,
			(device >> 12) & 0x0f,
			(device >> 8) & 0x0f);
	list_add_tail(&id->id_list,&card->ids);
}

/*
 *  Build device list for all present ISA PnP devices.
 */

static int __init isapnp_build_device_list(void)
{
	int csn;
	unsigned char header[9], checksum;
	struct pnp_card *card;

	isapnp_wait();
	isapnp_key();
	for (csn = 1; csn <= 10; csn++) {
		isapnp_wake(csn);
		isapnp_peek(header, 9);
		checksum = isapnp_checksum(header);
#if 0
		printk(KERN_DEBUG "vendor: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			header[0], header[1], header[2], header[3],
			header[4], header[5], header[6], header[7], header[8]);
		printk(KERN_DEBUG "checksum = 0x%x\n", checksum);
#endif
		/* Don't be strict on the checksum, here !
                   e.g. 'SCM SwapBox Plug and Play' has header[8]==0 (should be: b7)*/
		if (header[8] == 0)
			;
		else if (checksum == 0x00 || checksum != header[8])	/* not valid CSN */
			continue;
		if ((card = isapnp_alloc(sizeof(struct pci_bus))) == NULL)
			continue;

		card->number = csn;
		INIT_LIST_HEAD(&card->devices);
		INIT_LIST_HEAD(&card->ids);
		isapnp_parse_card_id(card, (header[1] << 8) | header[0], (header[3] << 8) | header[2]);
		card->serial = (header[7] << 24) | (header[6] << 16) | (header[5] << 8) | header[4];
		isapnp_checksum_value = 0x00;
		isapnp_parse_resource_map(card);
		if (isapnp_checksum_value != 0x00)
			printk(KERN_ERR "isapnp: checksum for device %i is not valid (0x%x)\n", csn, isapnp_checksum_value);
		card->checksum = isapnp_checksum_value;

		list_add_tail(&card->node, &isapnp_cards);
	}
	return 0;
}

/*
 *  Basic configuration routines.
 */

int isapnp_present(void)
{
	return !list_empty(&isapnp_devices);
}

int isapnp_cfg_begin(int csn, int logdev)
{
	if (csn < 1 || csn > 10 || logdev > 10)
		return -EINVAL;
	MOD_INC_USE_COUNT;
	down(&isapnp_cfg_mutex);
	isapnp_wait();
	isapnp_key();
	isapnp_wake(csn);
#if 0
	/* to avoid malfunction when the isapnptools package is used */
	/* we must set RDP to our value again */
	/* it is possible to set RDP only in the isolation phase */
	/*   Jens Thoms Toerring <Jens.Toerring@physik.fu-berlin.de> */
	isapnp_write_byte(0x02, 0x04);	/* clear CSN of card */
	mdelay(2);			/* is this necessary? */
	isapnp_wake(csn);		/* bring card into sleep state */
	isapnp_wake(0);			/* bring card into isolation state */
	isapnp_set_rdp();		/* reset the RDP port */
	udelay(1000);			/* delay 1000us */
	isapnp_write_byte(0x06, csn);	/* reset CSN to previous value */
	udelay(250);			/* is this necessary? */
#endif
	if (logdev >= 0)
		isapnp_device(logdev);
	return 0;
}

int isapnp_cfg_end(void)
{
	isapnp_wait();
	up(&isapnp_cfg_mutex);
	MOD_DEC_USE_COUNT;
	return 0;
}

static int isapnp_config_prepare(struct pnp_dev *dev)
{
	int idx;
	if (dev == NULL)
		return -EINVAL;
	if (dev->active || dev->ro)
		return -EBUSY;
	for (idx = 0; idx < DEVICE_COUNT_IRQ; idx++) {
		dev->irq_resource[idx].name = NULL;
		dev->irq_resource[idx].start = -1;
		dev->irq_resource[idx].end = -1;
		dev->irq_resource[idx].flags = 0;
	}
	for (idx = 0; idx < DEVICE_COUNT_DMA; idx++) {
		dev->dma_resource[idx].name = NULL;
		dev->dma_resource[idx].start = -1;
		dev->dma_resource[idx].end = -1;
		dev->dma_resource[idx].flags = 0;
	}
	for (idx = 0; idx < DEVICE_COUNT_RESOURCE; idx++) {
		dev->resource[idx].name = NULL;
		dev->resource[idx].start = 0;
		dev->resource[idx].end = 0;
		dev->resource[idx].flags = 0;
	}
	return 0;
}

void isapnp_resource_change(struct resource *resource,
			    unsigned long start,
			    unsigned long size)
{
	if (resource == NULL)
		return;
	resource->flags &= ~IORESOURCE_AUTO;
	resource->start = start;
	resource->end = start + size - 1;
}

/*
 *  Inititialization.
 */


EXPORT_SYMBOL(isapnp_cards);
EXPORT_SYMBOL(isapnp_devices);
EXPORT_SYMBOL(isapnp_present);
EXPORT_SYMBOL(isapnp_cfg_begin);
EXPORT_SYMBOL(isapnp_cfg_end);
EXPORT_SYMBOL(isapnp_read_byte);
EXPORT_SYMBOL(isapnp_read_word);
EXPORT_SYMBOL(isapnp_read_dword);
EXPORT_SYMBOL(isapnp_write_byte);
EXPORT_SYMBOL(isapnp_write_word);
EXPORT_SYMBOL(isapnp_write_dword);
EXPORT_SYMBOL(isapnp_wake);
EXPORT_SYMBOL(isapnp_device);
EXPORT_SYMBOL(isapnp_resource_change);

static int isapnp_get_resources(struct pnp_dev *dev)
{
	/* We don't need to do anything but this, the rest is taken care of */
	if ((dev->resource[0].start == 0) &&
	    (dev->irq_resource[0].start == -1) &&
	    (dev->dma_resource[0].start == -1))
		dev->active = 0;
	else
		dev->active = 1;
	return 0;
}

static int isapnp_set_resources(struct pnp_dev *dev, struct pnp_cfg *cfg, char flags)
{
	int tmp;
      	isapnp_cfg_begin(dev->card->number, dev->number);
	dev->active = 1;
	dev->irq_resource[0] = cfg->request.irq_resource[0];
	dev->irq_resource[1] = cfg->request.irq_resource[1];
	dev->dma_resource[0] = cfg->request.dma_resource[0];
	dev->dma_resource[1] = cfg->request.dma_resource[1];
	for (tmp = 0; tmp < 12; tmp++) {
		dev->resource[tmp] = cfg->request.resource[tmp];
	}
	for (tmp = 0; tmp < 8 && dev->resource[tmp].flags; tmp++)
		isapnp_write_word(ISAPNP_CFG_PORT+(tmp<<1), dev->resource[tmp].start);
	for (tmp = 0; tmp < 2 && dev->irq_resource[tmp].flags; tmp++) {
		int irq = dev->irq_resource[tmp].start;
		if (irq == 2)
			irq = 9;
		isapnp_write_byte(ISAPNP_CFG_IRQ+(tmp<<1), irq);
	}
	for (tmp = 0; tmp < 2 && dev->dma_resource[tmp].flags; tmp++)
		isapnp_write_byte(ISAPNP_CFG_DMA+tmp, dev->dma_resource[tmp].start);
	for (tmp = 0; tmp < 4 && dev->resource[tmp+8].flags; tmp++)
		isapnp_write_word(ISAPNP_CFG_MEM+(tmp<<2), (dev->resource[tmp + 8].start >> 8) & 0xffff);
	isapnp_activate(dev->number);
	isapnp_cfg_end();
	return 0;
}

static int isapnp_disable_resources(struct pnp_dev *dev)
{
	if (!dev || !dev->active)
		return -EINVAL;
      	isapnp_cfg_begin(dev->card->number, dev->number);
	isapnp_deactivate(dev->number);
	dev->active = 0;
	isapnp_cfg_end();
	return 0;
}

struct pnp_protocol isapnp_protocol = {
	name:	"ISA Plug and Play",
	get:	isapnp_get_resources,
	set:	isapnp_set_resources,
	disable:isapnp_disable_resources,
};

static inline int isapnp_init_device_tree(void)
{

	struct pnp_card *card;

	isapnp_for_each_card(card) {
		struct list_head *devlist;
		for (devlist = card->devices.next; devlist != &card->devices; devlist = devlist->next) {
			struct pnp_dev *dev = card_to_pnp_dev(devlist);
			snprintf(dev->dev.name, sizeof(dev->dev.name), "%s", dev->name);
			dev->card = card;
			dev->protocol = &isapnp_protocol;
			pnp_add_device(dev);
		}
	}

	return 0;
}

int __init isapnp_init(void)
{
	int cards;
	struct pnp_card *card;

	if (isapnp_disable) {
		isapnp_detected = 0;
		printk(KERN_INFO "isapnp: ISA Plug & Play support disabled\n");
		return 0;
	}
#ifdef ISAPNP_REGION_OK
	if (!request_region(_PIDXR, 1, "isapnp index")) {
		printk(KERN_ERR "isapnp: Index Register 0x%x already used\n", _PIDXR);
		return -EBUSY;
	}
#endif
	if (!request_region(_PNPWRP, 1, "isapnp write")) {
		printk(KERN_ERR "isapnp: Write Data Register 0x%x already used\n", _PNPWRP);
#ifdef ISAPNP_REGION_OK
		release_region(_PIDXR, 1);
#endif
		return -EBUSY;
	}

	if(pnp_protocol_register(&isapnp_protocol)<0)
		return -EBUSY;

	/*
	 *	Print a message. The existing ISAPnP code is hanging machines
	 *	so let the user know where.
	 */
	 
	printk(KERN_INFO "isapnp: Scanning for PnP cards...\n");
	if (isapnp_rdp >= 0x203 && isapnp_rdp <= 0x3ff) {
		isapnp_rdp |= 3;
		if (!request_region(isapnp_rdp, 1, "isapnp read")) {
			printk(KERN_ERR "isapnp: Read Data Register 0x%x already used\n", isapnp_rdp);
#ifdef ISAPNP_REGION_OK
			release_region(_PIDXR, 1);
#endif
			release_region(_PNPWRP, 1);
			return -EBUSY;
		}
		isapnp_set_rdp();
	}
	isapnp_detected = 1;
	if (isapnp_rdp < 0x203 || isapnp_rdp > 0x3ff) {
		cards = isapnp_isolate();
		if (cards < 0 || 
		    (isapnp_rdp < 0x203 || isapnp_rdp > 0x3ff)) {
#ifdef ISAPNP_REGION_OK
			release_region(_PIDXR, 1);
#endif
			release_region(_PNPWRP, 1);
			isapnp_detected = 0;
			printk(KERN_INFO "isapnp: No Plug & Play device found\n");
			return 0;
		}
		request_region(isapnp_rdp, 1, "isapnp read");
	}
	isapnp_build_device_list();
	cards = 0;

	isapnp_for_each_card(card) {
		cards++;
		if (isapnp_verbose) {
			struct list_head *devlist;
			printk(KERN_INFO "isapnp: Card '%s'\n", card->name[0]?card->name:"Unknown");
			if (isapnp_verbose < 2)
				continue;
			for (devlist = card->devices.next; devlist != &card->devices; devlist = devlist->next) {
				struct pci_dev *dev = pci_dev_b(devlist);
				printk(KERN_INFO "isapnp:   Device '%s'\n", dev->name[0]?card->name:"Unknown");
			}
		}
	}
	if (cards) {
		printk(KERN_INFO "isapnp: %i Plug & Play card%s detected total\n", cards, cards>1?"s":"");
	} else {
		printk(KERN_INFO "isapnp: No Plug & Play card found\n");
	}

	isapnp_init_device_tree();
	isapnp_proc_init();
	return 0;
}

subsys_initcall(isapnp_init);

/* format is: noisapnp */

static int __init isapnp_setup_disable(char *str)
{
	isapnp_disable = 1;
	return 1;
}

__setup("noisapnp", isapnp_setup_disable);

/* format is: isapnp=rdp,reset,skip_pci_scan,verbose */

static int __init isapnp_setup_isapnp(char *str)
{
	(void)((get_option(&str,&isapnp_rdp) == 2) &&
	       (get_option(&str,&isapnp_reset) == 2) &&
	       (get_option(&str,&isapnp_skip_pci_scan) == 2) &&
	       (get_option(&str,&isapnp_verbose) == 2));
	return 1;
}

__setup("isapnp=", isapnp_setup_isapnp);
