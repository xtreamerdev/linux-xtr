/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999, 2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * ########################################################################
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
 * ########################################################################
 *
 * Interrupt exception dispatch code.
 *
 */
#include <linux/config.h>

#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>

/* A lot of complication here is taken away because:
 *
 * 1) We handle one interrupt and return, sitting in a loop and moving across
 *    all the pending IRQ bits in the cause register is _NOT_ the answer, the
 *    common case is one pending IRQ so optimize in that direction.
 *
 * 2) We need not check against bits in the status register IRQ mask, that
 *    would make this routine slow as hell.
 *
 * 3) Linux only thinks in terms of all IRQs on or all IRQs off, nothing in
 *    between like BSD spl() brain-damage.
 *
 * Furthermore, the IRQs on the MIPS board look basically (barring software
 * IRQs which we don't use at all and all external interrupt sources are
 * combined together on hardware interrupt 0 (MIPS IRQ 2)) like:
 *
 *	MIPS IRQ	Source
 *      --------        ------
 *             0	Software (ignored)
 *             1        Software (ignored)
 *             2        Combined hardware interrupt (hw0)
 *             3        Hardware (ignored)
 *             4        Hardware (ignored)
 *             5        Hardware (ignored)
 *             6        Hardware (ignored)
 *             7        R4k timer (what we use)
 *
 * Note: On the SEAD board thing are a little bit different.
 *       Here IRQ 2 (hw0) is wired to the UART0 and IRQ 3 (hw1) is wired
 *       wired to UART1.
 *
 * We handle the IRQ according to _our_ priority which is:
 *
 * Highest ----     R4k Timer
 * Lowest  ----     Combined hardware interrupt
 *
 * then we just return, if multiple IRQs are pending then we will just take
 * another exception, big deal.
 */


// Reference galileo-boards/ev96100/int-handler.S

	.text
	.set	noreorder
	.set	noat
	.align	5
	NESTED(mipsIRQ, PT_SIZE, sp)
#ifdef CONFIG_REALTEK_USE_SHADOW_REGISTERS
	MY_SAVE_ALL
#elif  CONFIG_REALTEK_USE_FAST_INTERRUPT
	mfc0	k0, CP0_CAUSE		# get irq bits
	mfc0	k1, CP0_STATUS		# get irq mask
	and	k0, k1
	andi	k0, k0, CAUSEF_IP2	# check if it's USB interrupt
	beq	k0, zero, 1f
//	li	k0, 0x61;		/*cy test*/
//	li	k1, 0xb801b200
//	sw	k0, (k1)
	FAST_SAVE_ALL
	CLI
	li	a0, 0x4
	move    a1, sp
	jal	venus_hw0_irqdispatch
	nop
	j	fast_ret_from_irq
	nop
1:
	SAVE_ALL
#elif  CONFIG_REALTEK_USE_DMEM
	DMEM_SAVE_ALL
#else
	SAVE_ALL
#endif
	CLI
	.set	at

	mfc0	s0, CP0_CAUSE		# get irq bits
	mfc0	s1, CP0_STATUS		# get irq mask
	and	s0, s1
	andi	s0, 0xff00

	/* First we check for r4k counter/timer IRQ. */
#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
	andi	a0, s0, CAUSEF_IP6
#else
	andi	a0, s0, CAUSEF_IP7
#endif
	beq	a0, zero, 1f
//	nop

	/* Wheee, a timer interrupt. */
	move	a0, sp
	jal	mips_timer_interrupt
	nop
	j	ret_from_irq
	nop

1:
	beqz	s0, 3f			# spurious interrupt
	sra	a0, s0, 8
	move    a1, sp
	jal	venus_hw0_irqdispatch
	nop
	j	ret_from_irq
	nop

3:
	j	spurious_interrupt
	nop
	END(mipsIRQ)






