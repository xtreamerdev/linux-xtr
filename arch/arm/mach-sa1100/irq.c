/*
 * linux/arch/arm/mach-sa1100/irq.c
 *
 * Copyright (C) 1999-2001 Nicolas Pitre
 *
 * Generic IRQ handling for the SA11x0, GPIO 11-27 IRQ demultiplexing.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/ptrace.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>

#include "generic.h"


/*
 * SA1100 GPIO edge detection for IRQs:
 * IRQs are generated on Falling-Edge, Rising-Edge, or both.
 * This must be called *before* the appropriate IRQ is registered.
 * Use this instead of directly setting GRER/GFER.
 */
static int GPIO_IRQ_rising_edge;
static int GPIO_IRQ_falling_edge;
static int GPIO_IRQ_mask = (1 << 11) - 1;

static void sa1100_manual_rerun(unsigned int irq)
{
	struct pt_regs regs;
	memset(&regs, 0, sizeof(regs));
	irq_desc[irq].handle(irq, &irq_desc[irq], &regs);
}

/*
 * To get the GPIO number from an IRQ number
 */
#define GPIO_11_27_IRQ(i)	((i) - 21)
#define GPIO11_27_MASK(irq)	(1 << GPIO_11_27_IRQ(irq))

static int sa1100_gpio_type(unsigned int irq, unsigned int type)
{
	unsigned int mask;

	if (irq <= 10)
		mask = 1 << irq;
	else
		mask = GPIO11_27_MASK(irq);

	if (type == IRQT_PROBE) {
		if ((GPIO_IRQ_rising_edge | GPIO_IRQ_falling_edge) & mask)
			return 0;
		type = __IRQT_RISEDGE | __IRQT_FALEDGE;
	}

	if (type & __IRQT_RISEDGE) {
		GPIO_IRQ_rising_edge |= mask;
	} else
		GPIO_IRQ_rising_edge &= ~mask;
	if (type & __IRQT_FALEDGE) {
		GPIO_IRQ_falling_edge |= mask;
	} else
		GPIO_IRQ_falling_edge &= ~mask;

	GRER = GPIO_IRQ_rising_edge & GPIO_IRQ_mask;
	GFER = GPIO_IRQ_falling_edge & GPIO_IRQ_mask;

	return 0;
}

/*
 * GPIO IRQs must be acknoledged.  This is for IRQs from 0 to 10.
 */
static void sa1100_low_gpio_ack(unsigned int irq)
{
	GEDR = (1 << irq);
}

static void sa1100_low_gpio_mask(unsigned int irq)
{
	ICMR &= ~(1 << irq);
}

static void sa1100_low_gpio_unmask(unsigned int irq)
{
	ICMR |= 1 << irq;
}

static int sa1100_low_gpio_wake(unsigned int irq, unsigned int on)
{
	if (on)
		PWER |= 1 << irq;
	else
		PWER &= ~(1 << irq);
	return 0;
}

static struct irqchip sa1100_low_gpio_chip = {
	.ack		= sa1100_low_gpio_ack,
	.mask		= sa1100_low_gpio_mask,
	.unmask		= sa1100_low_gpio_unmask,
	.rerun		= sa1100_manual_rerun,
	.type		= sa1100_gpio_type,
	.wake		= sa1100_low_gpio_wake,
};

/*
 * IRQ11 (GPIO11 through 27) handler.  We enter here with the
 * irq_controller_lock held, and IRQs disabled.  Decode the IRQ
 * and call the handler.
 */
static void
sa1100_high_gpio_handler(unsigned int irq, struct irqdesc *desc,
			 struct pt_regs *regs)
{
	unsigned int mask;

	mask = GEDR & 0xfffff800;
	do {
		/*
		 * clear down all currently active IRQ sources.
		 * We will be processing them all.
		 */
		GEDR = mask;

		irq = IRQ_GPIO11;
		desc = irq_desc + irq;
		mask >>= 11;
		do {
			if (mask & 1)
				desc->handle(irq, desc, regs);
			mask >>= 1;
			irq++;
			desc++;
		} while (mask);

		mask = GEDR & 0xfffff800;
	} while (mask);
}

/*
 * Like GPIO0 to 10, GPIO11-27 IRQs need to be handled specially.
 * In addition, the IRQs are all collected up into one bit in the
 * interrupt controller registers.
 */
static void sa1100_high_gpio_ack(unsigned int irq)
{
	unsigned int mask = GPIO11_27_MASK(irq);

	GEDR = mask;
}

static void sa1100_high_gpio_mask(unsigned int irq)
{
	unsigned int mask = GPIO11_27_MASK(irq);

	GPIO_IRQ_mask &= ~mask;

	GRER &= ~mask;
	GFER &= ~mask;
}

static void sa1100_high_gpio_unmask(unsigned int irq)
{
	unsigned int mask = GPIO11_27_MASK(irq);

	GPIO_IRQ_mask |= mask;

	GRER = GPIO_IRQ_rising_edge & GPIO_IRQ_mask;
	GFER = GPIO_IRQ_falling_edge & GPIO_IRQ_mask;
}

static int sa1100_high_gpio_wake(unsigned int irq, unsigned int on)
{
	if (on)
		PWER |= GPIO11_27_MASK(irq);
	else
		PWER &= ~GPIO11_27_MASK(irq);
	return 0;
}

static struct irqchip sa1100_high_gpio_chip = {
	.ack		= sa1100_high_gpio_ack,
	.mask		= sa1100_high_gpio_mask,
	.unmask		= sa1100_high_gpio_unmask,
	.rerun		= sa1100_manual_rerun,
	.type		= sa1100_gpio_type,
	.wake		= sa1100_high_gpio_wake,
};

/*
 * We don't need to ACK IRQs on the SA1100 unless they're GPIOs
 * this is for internal IRQs i.e. from 11 to 31.
 */
static void sa1100_mask_irq(unsigned int irq)
{
	ICMR &= ~(1 << irq);
}

static void sa1100_unmask_irq(unsigned int irq)
{
	ICMR |= (1 << irq);
}

static struct irqchip sa1100_normal_chip = {
	.ack		= sa1100_mask_irq,
	.mask		= sa1100_mask_irq,
	.unmask		= sa1100_unmask_irq,
	/* rerun should never be called */
};

static struct resource irq_resource = {
	.name	= "irqs",
	.start	= 0x90050000,
	.end	= 0x9005ffff,
};

void __init sa1100_init_irq(void)
{
	unsigned int irq;

	request_resource(&iomem_resource, &irq_resource);

	/* disable all IRQs */
	ICMR = 0;

	/* all IRQs are IRQ, not FIQ */
	ICLR = 0;

	/* clear all GPIO edge detects */
	GFER = 0;
	GRER = 0;
	GEDR = -1;

	/*
	 * Whatever the doc says, this has to be set for the wait-on-irq
	 * instruction to work... on a SA1100 rev 9 at least.
	 */
	ICCR = 1;

	for (irq = 0; irq <= 10; irq++) {
		set_irq_chip(irq, &sa1100_low_gpio_chip);
		set_irq_handler(irq, do_edge_IRQ);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	}

	for (irq = 12; irq <= 31; irq++) {
		set_irq_chip(irq, &sa1100_normal_chip);
		set_irq_handler(irq, do_level_IRQ);
		set_irq_flags(irq, IRQF_VALID);
	}

	for (irq = 32; irq <= 48; irq++) {
		set_irq_chip(irq, &sa1100_high_gpio_chip);
		set_irq_handler(irq, do_edge_IRQ);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	}

	/*
	 * Install handler for GPIO 11-27 edge detect interrupts
	 */
	set_irq_chip(IRQ_GPIO11_27, &sa1100_normal_chip);
	set_irq_chained_handler(IRQ_GPIO11_27, sa1100_high_gpio_handler);

	/*
	 * We generally don't want the LCD IRQ being
	 * enabled as soon as we request it.
	 */
	set_irq_flags(IRQ_LCD, IRQF_VALID/* | IRQF_NOAUTOEN*/);
}
