/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000, 2001 Keith M Wesolowski
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <asm/pci_channel.h>
#include <asm/ip32/mace.h>
#include <asm/ip32/crime.h>
#include <asm/ip32/ip32_ints.h>

#undef DEBUG_MACE_PCI

/*
 * Handle errors from the bridge.  This includes master and target aborts,
 * various command and address errors, and the interrupt test.  This gets
 * registered on the bridge error irq.  It's conceivable that some of these
 * conditions warrant a panic.  Anybody care to say which ones?
 */
static irqreturn_t macepci_error(int irq, void *dev, struct pt_regs *regs)
{
	u32 flags, error_addr;
	char space;

	flags = mace_read_32(MACEPCI_ERROR_FLAGS);
	error_addr = mace_read_32(MACEPCI_ERROR_ADDR);

	if (flags & MACEPCI_ERROR_MEMORY_ADDR)
		space = 'M';
	else if (flags & MACEPCI_ERROR_CONFIG_ADDR)
		space = 'C';
	else
		space = 'X';

	if (flags & MACEPCI_ERROR_MASTER_ABORT) {
		printk("MACEPCI: Master abort at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(flags & ~MACEPCI_ERROR_MASTER_ABORT,
			      MACEPCI_ERROR_FLAGS);
	}
	if (flags & MACEPCI_ERROR_TARGET_ABORT) {
		printk("MACEPCI: Target abort at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(flags & ~MACEPCI_ERROR_TARGET_ABORT,
			      MACEPCI_ERROR_FLAGS);
	}
	if (flags & MACEPCI_ERROR_DATA_PARITY_ERR) {
		printk("MACEPCI: Data parity error at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(flags & ~MACEPCI_ERROR_DATA_PARITY_ERR,
			      MACEPCI_ERROR_FLAGS);
	}
	if (flags & MACEPCI_ERROR_RETRY_ERR) {
		printk("MACEPCI: Retry error at 0x%08x (%c)\n", error_addr,
		       space);
		mace_write_32(flags & ~MACEPCI_ERROR_RETRY_ERR,
			      MACEPCI_ERROR_FLAGS);
	}
	if (flags & MACEPCI_ERROR_ILLEGAL_CMD) {
		printk("MACEPCI: Illegal command at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(flags & ~MACEPCI_ERROR_ILLEGAL_CMD,
			      MACEPCI_ERROR_FLAGS);
	}
	if (flags & MACEPCI_ERROR_SYSTEM_ERR) {
		printk("MACEPCI: System error at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(flags & ~MACEPCI_ERROR_SYSTEM_ERR,
			      MACEPCI_ERROR_FLAGS);
	}
	if (flags & MACEPCI_ERROR_PARITY_ERR) {
		printk("MACEPCI: Parity error at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(flags & ~MACEPCI_ERROR_PARITY_ERR,
			      MACEPCI_ERROR_FLAGS);
	}
	if (flags & MACEPCI_ERROR_OVERRUN) {
		printk("MACEPCI: Overrun error at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(flags & ~MACEPCI_ERROR_OVERRUN,
			      MACEPCI_ERROR_FLAGS);
	}
	if (flags & MACEPCI_ERROR_SIG_TABORT) {
		printk("MACEPCI: Signaled target abort (clearing)\n");
		mace_write_32(flags & ~MACEPCI_ERROR_SIG_TABORT,
			      MACEPCI_ERROR_FLAGS);
	}
	if (flags & MACEPCI_ERROR_INTERRUPT_TEST) {
		printk("MACEPCI: Interrupt test triggered (clearing)\n");
		mace_write_32(flags & ~MACEPCI_ERROR_INTERRUPT_TEST,
			      MACEPCI_ERROR_FLAGS);
	}

	return IRQ_HANDLED;
}


extern struct pci_ops mace_pci_ops;
#ifdef CONFIG_MIPS64
static struct resource mace_pci_mem_resource = {
	.name	= "SGI O2 PCI MEM",
	.start	= 0x280000000UL,
	.end	= 0x2FFFFFFFFUL,
	.flags	= IORESOURCE_MEM,
};
static struct resource mace_pci_io_resource = {
	.name	= "SGI O2 PCI IO",
	.start	= 0x00000000UL,
	.end	= 0xffffffffUL,
	.flags	= IORESOURCE_IO,
};
#define MACE_PCI_MEM_OFFSET 0x200000000
#else

static struct resource mace_pci_mem_resource = {
	.name	= "SGI O2 PCI MEM",
	.start	= MACEPCI_LOW_MEMORY,
	.end	= MACEPCI_LOW_MEMORY+0x2000000-1,
	.flags	= IORESOURCE_MEM,
};
static struct resource mace_pci_io_resource = {
	.name	= "SGI O2 PCI IO",
	.start	= 0x00000000,
	.end	= 0xFFFFFFFF,
	.flags	= IORESOURCE_IO,
};
#define MACE_PCI_MEM_OFFSET (MACEPCI_LOW_MEMORY-0x80000000UL)
#endif
static struct pci_controller mace_pci_controller = {
	.pci_ops	= &mace_pci_ops,
	.mem_resource	= &mace_pci_mem_resource,
	.io_resource	= &mace_pci_io_resource,
	.iommu		= 0,
	.mem_offset	= MACE_PCI_MEM_OFFSET,
	.io_offset	= 0x0,
};


static int __init mace_init(void)
{
	/* Clear any outstanding errors and enable interrupts */
	mace_write_32(0, MACEPCI_ERROR_ADDR);
	mace_write_32(0, MACEPCI_ERROR_FLAGS);
	mace_write_32(0xff008500, MACEPCI_CONTROL);
	/* FIXME: should be done in crime.c */
	crime_write(0, CRIME_HARD_INT);
	crime_write(0, CRIME_SOFT_INT);
	crime_write(0x000000000000ff00UL, CRIME_INT_STAT);

	if (request_irq(MACE_PCI_BRIDGE_IRQ, macepci_error, 0,
			"MACE PCI error", NULL))
		printk("PCI bridge can't get interrupt; can't happen.\n");
	else
		printk("PCI bridge interrupt registered\n");
	return 0;

}
subsys_initcall(mace_init);

extern int pci_probe_only;
void __init ip32_pci_setup(void)
{
	const int field = 2 * sizeof(unsigned long);
	PCIBIOS_MIN_IO = 0x1000;

 	printk("MACE PCI rev %d at 0x%0*lx\n", mace_read_32(MACEPCI_REV),
		field, (unsigned long) MACE_BASE + MACE_PCI);

	ioport_resource.end = mace_pci_io_resource.end;
	register_pci_controller(&mace_pci_controller);
}
