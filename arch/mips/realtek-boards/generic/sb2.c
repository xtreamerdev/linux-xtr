

#include <linux/interrupt.h>
#include <asm/io.h>
#include <venus.h>

extern void show_registers(struct pt_regs *regs);

irqreturn_t sb2_intr(int irq, void *dev_id, struct pt_regs *regs)
{
	if(!(inl(VENUS_SB2_INV_INTSTAT) & 0x2))
		return IRQ_NONE;
	// prevent the error of prefetch...
	// TEMPORARILY: SE on Mars has problems, and therefore we temporarily disable the checking of this memory region
	if (inl(VENUS_SB2_INV_ADDR) > 0x8001000 && ((inl(VENUS_SB2_INV_ADDR)&0xfffff000) != 0x1800c000)) {
		prom_printf("You have accessed an invalid hardware address 0X%X\n", inl(VENUS_SB2_INV_ADDR));
		show_registers(regs);
	}
	outl(0xE, VENUS_SB2_INV_INTSTAT);

	return IRQ_HANDLED;
}

static struct irqaction sb2_action = {
	.handler        = sb2_intr,
	.flags		= SA_INTERRUPT | SA_SHIRQ,
	.name           = "SB2",
};


void __init mips_sb2_setup(void)
{
	outl(0x3, VENUS_SB2_INV_INTEN);
	outl(0xE, VENUS_SB2_INV_INTSTAT);
	setup_irq(VENUS_INT_SB2, &sb2_action);
}







