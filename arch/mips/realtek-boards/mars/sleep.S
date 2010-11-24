/*
 * Mars Assembler Sleep/WakeUp Management Routines
 *
 * Copyright (c) 2006 Colin <colin@realtek.com.tw>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2008-12-30:	Colin	First version.
 */

//#include <linux/linkage.h>
//#include <asm/assembler.h>
//#include <asm/hardware.h>
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <mars.h>
#include <linux/serial_reg.h>

#define __SLEEP		.section	".sleep.text","ax"
#define __SLEEPDATA	.section	".sleep.data","aw"


#define CACHE_OP( code, type )			( ((code) << 2) | (type) )
#define ICACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 0)
#define ICACHE_ADDR_FILL			CACHE_OP(0x5, 0)
#define DCACHE_ADDR_FETCH			CACHE_OP(0x7, 1)
#define DCACHE_ADDR_WRITEBACK			CACHE_OP(0x6, 1)
#define DCACHE_ADDR_INVALIDATE			CACHE_OP(0x4, 1)

	.text
	.set noreorder
	__SLEEP
	
/*
 * mars_cpu_suspend()
 *
 * Make mars enter sleep mode
 *
 * Parameters:
 *   a0 stores board_id
 *   a1 stores 12V/5V GPIO parameter. If it is -1, it means that there is no value for 12V/5V GPIO parameter. When the bit 28 of a1 is 1, it represents that 12V/5V will be on when 12V/5V GPIO is high. Bits 0-27 store the GPIO number of 12V/5V, and the value of it will locate between 0 and 35.
 *   a2 stores options:
 *       bit0: 0 represents normal resuming procedure. 1 means we want to use "reboot" to replace normal resuming procedure.
 *   a3 stores HW info:
 *       bit0-3: CPU type. 0 represents Venus, 1 represents Neptune, and 2 represents Mars.
 *       bit4: Package type. 0 represents QFP, and 1 represents BGA.
 *   Parameter5, which means 124(sp), stores the IRRP value of IR power key:
 *   Parameter6, which means 128(sp), stores the IRRP value of IR eject key:
 *   Parameter7, which means 132(sp), stores the GPIO pin num of GPIO power key:
 *   Parameter8, which means 136(sp), stores the GPIO pin num of GPIO eject key:
 *   Parameter9, which means 140(sp), stores the IR type:
 *
 * These registers are used for these purposes in the whole function:
 *   s0 is used to save base address
 *   s3 is used to save HW info
 *   s4 is used to save options
 *   s5 is used to save 12V/5V GPIO parameter
 *   s6 is used to save return value
 *   s7 is used to save board_id
 * 
 * Return 2 to represent that it is woken up by RTC alarm, 1 to represent that cdrom should be ejected, and 0 to represent that power on key is pressed.
 */
NESTED(mars_cpu_suspend, 0, sp)
	addiu	sp, sp, -108
	sw	s0, 0(sp)
	sw	s1, 4(sp)
	sw	s2, 8(sp)
	sw	s3, 12(sp)
	sw	s4, 16(sp)
	sw	s5, 20(sp)
	sw	s6, 24(sp)
	sw	s7, 28(sp)
	sw	ra, 32(sp)

	move	s7, a0		/* board_id */
	move	s5, a1		/* 12V/5V GPIO pin number */
	move	s4, a2		/* options */
	move	s3, a3		/* HW info */


/*********************************************************************************/

	/* Turn off ETH Phy */
	li	t1, 0x84000800
	sw	t1, 0xb801605c

	/* Turn off HDMI_TMDS */
	lw	t1, 0xb800d040
	li	t2, ~0xf0
	and	t1, t2, t1
	sw	t1, 0xb800d040

	/* Turn off HDMI_PLL */
	lw	t1, 0xb800d044
	li	t2, ~0x40
	and	t1, t2, t1
	sw	t1, 0xb800d044

	/* Mars has no Audio Codec */

	/* Mars has no ADC */

	/* Turn off DAC */
	lw	t1, 0xb8018200
	li	t2, ~0x00700000
	and	t1, t2, t1
	sw	t1, 0xb8018200
	lw	t1, 0xb8018204
	and	t1, t2, t1
	sw	t1, 0xb8018204

	/* Turn off SATA PHY0 */
//	li	t1, 0x129e0201
//	sw	t1, 0xb8012be8
	/* Turn off SATA PHY1 */
//	li	t1, 0x129e0201
//	sw	t1, 0xb8012ce8
	/* Turn off SATA FUNC */
	lw	t1, 0xb8000340
	li	t2, ~0x00000003
	and	t1, t2, t1
	sw	t1, 0xb8000340

	/* Turn off USB Phy */
	lw	t1, 0xb8013054
	li	t2, 0x80
	or	t1, t2, t1
	sw	t1, 0xb8013054

	lui	s0, 0xb800

	li	t2, 0x4
	/* disable ACPU PLL */
	li	t1, 5
	sw	t1, 0x10c(s0)

	/* disable VCPU PLL */
	li	t1, 5
	sw	t1, 0x114(s0)

	/* disable SD PLL */
	li	t1, 5
	sw	t1, 0x124(s0)

	/* disable HD PLL */
	li	t1, 5
	sw	t1, 0x134(s0)

	/* disable Audio PLL */
	li	t1, 5
	sw	t1, 0x13c(s0)

	jal	long_sleep_delay
	nop

/* For DRAM non-selfrefresh mode, putting code and data in cache is also necessary. If no doing this, system will halt on "turn on 27M OSC". */
	/* move code to I-cache */
	la	t1, _esleeptext
	addiu	t1, 16
	la	t2, _sleeptext
mars_load1:
.set mips3
	cache	ICACHE_ADDR_FILL, 0(t2)
.set mips0
	sltu	t0, t2, t1
	bnez	t0, mars_load1
	addiu	t2, 16

	/* move data to D-cache */
	la	t1, _esleepdata
	addiu	t1, 16
	la	t2, _sleepdata
mars_load2:
.set mips3
	cache	DCACHE_ADDR_FETCH, 0(t2)
.set mips0
	sltu	t0, t2, t1
	bnez	t0, mars_load2
	addiu	t2, 16

	/* move stack to D-cache */
	li	t1, 256
	move	t2, sp
mars_load3:
.set mips3
	cache	DCACHE_ADDR_FETCH, 0(t2)
.set mips0
	addiu	t1, -1
	bnez	t1, mars_load3
	addiu	t2, -16


mars_start:
	/* DDR goes into self-refresh */
	li	t1, 0x08
	sw	t1, 0x880c(s0)
	jal	sleep_delay
	nop

	/* disable DDR PLL */
	li	t1, 5
	sw	t1, 0x160(s0)
	jal long_sleep_delay
	nop

	/* Turn off DDR PAD */
	li	t2, 0x2000

	lw	t1, 0xb80088d0
	sw	t1, 36(sp)
	or	t1, t2, t1
	sw	t1, 0xb80088d0

	lw	t1, 0xb80088d4
	sw	t1, 40(sp)
	or	t1, t2, t1
	sw	t1, 0xb80088d4

	lw	t1, 0xb80088dc
	sw	t1, 44(sp)
	or	t1, t2, t1
	sw	t1, 0xb80088dc

	lw	t1, 0xb80088e0
	sw	t1, 48(sp)
	or	t1, t2, t1
	sw	t1, 0xb80088e0

	lw	t1, 0xb80088e4
	sw	t1, 52(sp)
	or	t1, t2, t1
	sw	t1, 0xb80088e4

	lw	t1, 0xb8008904
	sw	t1, 56(sp)
	or	t1, t2, t1
	sw	t1, 0xb8008904

	lw	t1, 0xb800891c
	sw	t1, 60(sp)
	or	t1, t2, t1
	sw	t1, 0xb800891c

	lw	t1, 0xb8008920
	sw	t1, 64(sp)
	or	t1, t2, t1
	sw	t1, 0xb8008920

	lw	t1, 0xb8008930
	sw	t1, 68(sp)
	or	t1, t2, t1
	sw	t1, 0xb8008930

	lw	t1, 0xb8008934
	sw	t1, 72(sp)
	or	t1, t2, t1
	sw	t1, 0xb8008934

	lw	t1, 0xb8008978
	sw	t1, 76(sp)
	or	t1, t2, t1
	sw	t1, 0xb8008978

	lw	t1, 0xb800897c
	sw	t1, 80(sp)
	or	t1, t2, t1
	sw	t1, 0xb800897c

	lw	t1, 0xb8008980
	sw	t1, 84(sp)
	or	t1, t2, t1
	sw	t1, 0xb8008980

	lw	t1, 0xb8008984
	sw	t1, 88(sp)
	or	t1, t2, t1
	sw	t1, 0xb8008984

	jal	long_sleep_delay
	nop
	/* Turn on 27M OSC */
	li	t1, 0x1
	sw	t1, 0xb8000018
	jal	long_sleep_delay
	nop

	/* Turn off 12V & 5V */
	/* If s5 < 0, no handle 12V/5V; if t6 > 31, handle GPIO 1; if t6 <= 31, handle GPIO 0 */
	bltz	s5, mars_no_12v5v_off
	nop
	li	a0, 'a'
	jal	print_char
	nop
	li	t5, 0x10000000
	and	t5, s5, t5		/* If t5 is "non-zero", sub-parameter is hion, otherwise sub-parameter is hioff. */
	li	t6, ~0x10000000
	and	t6, s5, t6		/* t6 stores 12V/5V GPIO number */
	li	t0, 31
	subu	t0, t6, t0
	bgtz	t0, mars_gp1_off
	nop

	li	t1, 0x1
	sll	t1, t1, t6
	/* Let the direction of corresponding GPIO bit be output. */
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DIR
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DIR
	/**********************************************************/
	bnez	t5, mars_handle_hion_0
	nop
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATO
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATO
	j	mars_no_12v5v_off
	nop
mars_handle_hion_0:
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATO
	or	t2, t2, t1
	xor	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATO
	j	mars_no_12v5v_off
	nop
mars_gp1_off:
	addiu	t0, -1
	li	t1, 0x1
	sll	t1, t1, t0
	/* Let the direction of corresponding GPIO bit be output. */
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DIR
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DIR
	/**********************************************************/
	bnez	t5, mars_handle_hion_1
	nop
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DATO
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DATO
	j	mars_no_12v5v_off
	nop
mars_handle_hion_1:
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DATO
	or	t2, t2, t1
	xor	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DATO
mars_no_12v5v_off:
	li	a0, '0'
	jal	print_char
	nop
	jal	long_sleep_delay
	nop



	/* Clock disable of Clock_EN2 */
	lw	t1, 0xb8000010
	sw	t1, 92(sp)
	sw	zero, 0xb8000010

	/* Reset1 */
	lw	t1, 0xb8000000
	sw	t1, 96(sp)
	li	t1, 0x01800001
	sw	t1, 0xb8000000

	/* Reset2 */
	lw	t1, 0xb8000004
	sw	t1, 100(sp)
	li	t2, 0x10
	sw	t2, 0xb8000004


	/* SCPU PLL */
	li	t1, 5
	sw	t1, 0xb8000104

	lw	t1, 0xb800000C
	sw	t1, 104(sp)
mars_resleep:
	lui	s0, 0xb801
	lui	t1, 0x1F
	ori	t1, 0xDE20
	sw	t1, VENUS_MIS_ISR(s0)	/* Clear all interrupts that could wake up system */

	lui	s0, 0xb800

	/* Clock disable of Clock_EN1. Mapping to SB1 clock of Venus */
	li	t1, 1
	sw	t1, 0xb800000C

	/* Disable SYS PLL. Mapping to BUS2 PLL of Venus */
	li	t1, 5
	sw	t1, 0xb800016c

	/* Turn off SB2 & SCPU clock */
	lw	t1, 0x14(s0)
	li	t2, ~0x3
	and	t1, t2, t1
	sw	t1, 0x14(s0)


	
	/**************************/
	nop
	nop
	nop
	nop

	lui	s0, 0xb801

/* When alarm shall happen, half-second and alarm interrupts may not appear
 * at the same time. Therefore, alarm interrupt may be lost. Add some delay
 * to prevent from this. */
	jal	sleep_delay
	nop

#if 0
	li	a0, ' '
	jal	print_char
	nop
	lw	a0, VENUS_MIS_ISR(s0)
	jal	print_registerr
	nop
	li	a0, ' '
	jal	print_char
	nop
#endif
	
mars_check_cec:
	jal     cec_wakeup_check          /* run cec wake up check */
	nop
	move    s6, v0	
	bne     s6, zero, mars_wake_up
	nop
	
mars_check_ir:
	/* Check IR interrupt here */
	lw	t1, VENUS_MIS_IR_SR(s0)
	andi	t1, 0x1
	beq	t1, zero, mars_check_front_panel
	nop
	li	t1, 0x1
	sw 	t1, VENUS_MIS_IR_SR(s0)
	lw	t1, VENUS_MIS_IR_RP(s0)
	/* if protocol id is RC6, shift-right for 10 bits */
	lw	t3, 140(sp)
	lui	t2, 0x0000
	ori	t2, 0x0006 /* RC6 = 6 */
	bne	t3, t2, mars_ir_examine
	lui	t4, 0x0000
	ori	t4, 0x000a
	srl	t1, t1, t4
	lui	t3, 0x003f
	ori	t3, 0xffdf
	and	t1, t1, t3
mars_ir_examine:
	lw	t2, 124(sp)
	beq	t1, t2, mars_wake_up		/* power key - IR */
	li	s6, 0
	lw	t2, 128(sp)
	beq	t1, t2, mars_wake_up		/* eject key - IR */
	li	s6, 1
mars_reread_irda:
	li	t1, 0x1
	sw	t1, VENUS_MIS_IR_SR(s0)
	lw	t1, VENUS_MIS_IR_RP(s0)
	lw	t1, VENUS_MIS_IR_SR(s0)
	andi	t1, 0x1
	bnez	t1, mars_reread_irda
	nop

mars_check_front_panel:
#if 0		/* Turn this on when VFD isn't plugged or it will send wrong data */
	j	mars_check_gpio_powerkey
	nop
#endif
	/* Check front panel interrupt here */
#ifndef CONFIG_PM_SLEEP_POLLING
	lw	t1, VENUS_MIS_ISR(s0)
	lui	t2, 0x1
	ori	t2, 0x8000
	and	t1, t2
	beq	t1, zero, mars_check_gpio_powerkey
	nop
#endif
	lw	t1, VENUS_MIS_UMSK_ISR_KPADAL(s0)
	lw	t2, VENUS_MIS_UMSK_ISR_KPADAL(s0)
	lui	t0, 0xFFFF
	ori	t0, 0xFFF0
	sw	t0, VENUS_MIS_UMSK_ISR_KPADAL(s0) 
	li	t0, 0x1
	bne	t0, s7, mars_general_frontpanel
	nop
mars_nucom_frontpanel:
	andi	t1, 0x200
	bnez	t1, mars_wake_up		/* power key - Front Panel */
	li	s6, 0
	andi	t2, 0x40
	bnez	t2, mars_wake_up		/* eject key - Front Panel */
	li	s6, 1
	b	mars_check_gpio_powerkey
	nop

mars_general_frontpanel:
	andi	t1, 0x10
	bnez	t1, mars_wake_up		/* power key - Front Panel */
	li	s6, 0
	andi	t2, 0x100
	bnez	t2, mars_wake_up		/* eject key - Front Panel */
	li	s6, 1

mars_check_gpio_powerkey:
	lw	t6, 132(sp)
	bltz	t6, mars_no_gpio_powerkey
	nop
	li	t7, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATI
	li	t0, 32
	move	t1, t6
mars_shif_gpio_powerkey:
	move	t6, t1
	addiu	t7, 4
	subu	t1, t6, t0
	bgez	t1, mars_shif_gpio_powerkey
	nop	
	addiu	t7, -4

	li	t2, 0x1
	sll	t2, t2, t6
	lw	t3, 0(t7)
	and	t3, t3, t2
	beq	t3, zero, mars_wake_up		/* power key - GPIO */
	li	s6, 0
mars_no_gpio_powerkey:

mars_check_gpio_ejectkey:
	lw	t0, 136(sp)
	bltz	t0, mars_no_gpio_ejectkey
	nop
	li	t7, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATI
	li	t0, 32
	move	t1, t6
mars_shif_gpio_ejectkey:
	move	t6, t1
	addiu	t7, 4
	subu	t1, t6, t0
	bgez	t1, mars_shif_gpio_ejectkey
	nop	
	addiu	t7, -4

	li	t2, 0x1
	sll	t2, t2, t6
	lw	t3, 0(t7)
	and	t3, t3, t2
	beq	t3, zero, mars_wake_up		/* eject key - GPIO */
	li	s6, 1
mars_no_gpio_ejectkey:

mars_check_rtc:
	lw	t1, VENUS_MIS_RTCCR(s0)
	lw	t2, VENUS_MIS_ISR(s0)
#ifdef CONFIG_PM_SLEEP_POLLING
	lw	t0, VENUS_MIS_RTCMIN(s0)
	lw	t3, VENUS_MIS_ALMMIN(s0)
	bne	t0, t3, mars_check_half_second
	nop
	lw	t0, VENUS_MIS_RTCHR(s0)
	lw	t4, VENUS_MIS_ALMHR(s0)
	bne	t0, t4, mars_check_half_second
	nop
	lw	t0, VENUS_MIS_RTCDATE1(s0)
	lw	t5, VENUS_MIS_ALMDATE1(s0)
	bne	t0, t5, mars_check_half_second
	nop
	lw	t0, VENUS_MIS_RTCDATE2(s0)
	lw	t6, VENUS_MIS_ALMDATE2(s0)
	bne	t0, t6, mars_check_half_second
	nop
	bne	t3, zero, mars_rtc_wake_up
	nop
	bne	t4, zero, mars_rtc_wake_up
	nop
	bne	t5, zero, mars_rtc_wake_up
	nop
	bne	t6, zero, mars_rtc_wake_up
	nop
	jal	mars_check_half_second
	nop
#else
	andi	t0, t1, 0x10
	beq	t0, zero, mars_check_half_second
	nop
	andi	t0, t2, 0x2000
	bnez	t0, mars_rtc_wake_up
	nop
#endif
mars_check_half_second:
	andi	t0, t2, 0x200
	bnez	t0, mars_half_second
	nop
//	lui	t1, 0x1
//	ori	t1, 0xBE20
//	sw	t1, VENUS_MIS_ISR(s0)	/* Who wakes CPU up? */
	li	a0, '?'
	jal	print_char
	nop
	li	a0, ' '
	jal	print_char
	nop
	lw	a0, VENUS_MIS_ISR(s0)
	jal	print_registerr
	nop
	li	a0, ' '
	jal	print_char
	nop
	b	mars_resleep
	nop
mars_half_second:
//	li	t0, 0x200
//	sw	t0, VENUS_MIS_ISR(s0)
	lw	t0, VENUS_MIS_RTCSEC(s0)
	andi	t0, 0x1
	bne	t0, zero, mars_resleep
	nop
mars_vfd_second:				/* RTC reaches one second interrupt and then refreshes vfd */
	li	a0, '.'			/* print '.' every second */
	jal	print_char
	nop
#ifdef CONFIG_REALTEK_VFD
	lw	a0, VENUS_MIS_RTCHR(s0)
	lw	a1, VENUS_MIS_RTCMIN(s0)
	lw	a2, VENUS_MIS_RTCSEC(s0)
	jal	update_vfd
	nop
#endif
	b	mars_resleep
	nop

mars_rtc_wake_up:
	ori	t1, 0x10
	xori	t1, 0x10
	sw	t1, VENUS_MIS_RTCCR(s0)
	li	s6, 2

mars_wake_up:
#ifdef CONFIG_PM_SLEEP_REBOOT
	andi	t1, s4, 0x1
	beq	t1, zero, mars_no_pm_sleep_reboot
	nop

/* 0xb801b56c bit2  bit1 bit0
 *              0     0    x   cold boot
 *              0     1    x   power key warm boot
 *              1     0    x   rtc warm boot
 */

	andi    t2, s6, 0x2
	beq     t2, zero, mars_no_rtc_wake_up
	nop

        lw      t1, 0xb801b56c
	andi    t1, 0xf9
        li      t2, 0x4
        or      t1, t2, t1
        sw      t1, 0xb801b56c
        j       watch_dog_reset
        nop
mars_no_rtc_wake_up:
        lw      t1, 0xb801b56c
        andi    t1, 0xf9
        li      t2, 0x2
        or      t1, t2, t1
        sw      t1, 0xb801b56c
 
watch_dog_reset:
	sw	zero, VENUS_MIS_TCWCR(s0)
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
mars_no_pm_sleep_reboot:
#endif

	lui	s0, 0xb800

	/* Turn off SB2 & SCPU clock */
	lw	t1, 0x14(s0)
	li	t2, 0x3
	or	t1, t2, t1
	sw	t1, 0x14(s0)

	/* SCPU PLL */
	li	t1, 0x4
	sw	t1, 0xb8000104
	jal	long_sleep_delay
	nop
	li	t1, 0x6
	sw	t1, 0xb8000104
	jal	long_sleep_delay
	nop
	li	t1, 0x2
	sw	t1, 0xb8000104

	/* Disable SYS PLL. Mapping to BUS2 PLL of Venus */
	li	t1, 0x4
	sw	t1, 0xb800016c
	jal	long_sleep_delay
	nop
	li	t1, 0x6
	sw	t1, 0xb800016c
	jal	long_sleep_delay
	nop
	li	t1, 0x2
	sw	t1, 0xb800016c

	/* Reset1 */
	lw	t1, 96(sp)
	sw	t1, 0xb8000000

	/* Reset2 */
	lw	t1, 100(sp)
	sw	t1, 0xb8000004

	/* Clock disable of Clock_EN1. Mapping to SB1 clock of Venus */
	lw	t1, 104(sp)
	sw	t1, 0xb800000C

	/* Clock disable of Clock_EN2 */
	lw	t1, 92(sp)
	sw	t1, 0xb8000010

	/* Turn on 27M OSC */
	sw	zero, 0xb8000018
	jal	long_sleep_delay
	nop

	li	a0, '1'
	jal	print_char
	nop

/* DDR goes out of self-refresh mode. */

	/* Turn off DDR PAD */
	lw	t1, 36(sp)
	sw	t1, 0xb80088d0

	lw	t1, 40(sp)
	sw	t1, 0xb80088d4

	lw	t1, 44(sp)
	sw	t1, 0xb80088dc

	lw	t1, 48(sp)
	sw	t1, 0xb80088e0

	lw	t1, 52(sp)
	sw	t1, 0xb80088e4

	lw	t1, 56(sp)
	sw	t1, 0xb8008904

	lw	t1, 60(sp)
	sw	t1, 0xb800891c

	lw	t1, 64(sp)
	sw	t1, 0xb8008920

	lw	t1, 68(sp)
	sw	t1, 0xb8008930

	lw	t1, 72(sp)
	sw	t1, 0xb8008934

	lw	t1, 76(sp)
	sw	t1, 0xb8008978

	lw	t1, 80(sp)
	sw	t1, 0xb800897c

	lw	t1, 84(sp)
	sw	t1, 0xb8008980

	lw	t1, 88(sp)
	sw	t1, 0xb8008984

	jal	long_sleep_delay
	nop

	/* disable DDR PLL */
	li	t1, 0x4
	sw	t1, 0x160(s0)
	jal long_sleep_delay
	nop
	li	t1, 0x6
	sw	t1, 0x160(s0)
	jal long_sleep_delay
	nop
	li	t1, 0x2
	sw	t1, 0x160(s0)
	jal long_sleep_delay
	nop

	/* DDR exit self-refresh */
	li	t1, 0x04
	sw	t1, 0x880c(s0)
	jal	long_sleep_delay
	nop
	/* DDR exit self-refresh */
	li	t1, 0x0
	sw	t1, 0x880c(s0)
	jal	sleep_delay
	nop

	/* flush data segment in D-cache */
	la	t1, _esleepdata
	addiu	t1, 16
	la	t2, _sleepdata
mars_flush2:
.set mips3
	cache	DCACHE_ADDR_WRITEBACK, 0(t2)
.set mips0
	sltu	t0, t2, t1
	bnez	t0, mars_flush2
	addiu	t2, 16

	/* flush stack in D-cache */
	li	t1, 256
	move	t2, sp
mars_flush3:
.set mips3
	cache	DCACHE_ADDR_WRITEBACK, 0(t2)
.set mips0
	addiu	t1, -1
	bnez	t1, mars_flush3
	addiu	t2, -16

	/* disable ACPU PLL */
	li	t1, 4
	sw	t1, 0x10c(s0)
	jal long_sleep_delay
	nop
	li	t1, 6
	sw	t1, 0x10c(s0)
	jal long_sleep_delay
	nop
	li	t1, 2
	sw	t1, 0x10c(s0)

	/* disable VCPU PLL */
	li	t1, 4
	sw	t1, 0x114(s0)
	jal long_sleep_delay
	nop
	li	t1, 6
	sw	t1, 0x114(s0)
	jal long_sleep_delay
	nop
	li	t1, 2
	sw	t1, 0x114(s0)


	/* disable SD PLL */
	li	t1, 4
	sw	t1, 0x124(s0)
	jal long_sleep_delay
	nop
	li	t1, 6
	sw	t1, 0x124(s0)
	jal long_sleep_delay
	nop
	li	t1, 2
	sw	t1, 0x124(s0)

	/* disable HD PLL */
	li	t1, 4
	sw	t1, 0x134(s0)
	jal long_sleep_delay
	nop
	li	t1, 6
	sw	t1, 0x134(s0)
	jal long_sleep_delay
	nop
	li	t1, 2
	sw	t1, 0x134(s0)


	/* disable Audio PLL */
	li	t1, 4
	sw	t1, 0x13c(s0)
	jal long_sleep_delay
	nop
	li	t1, 6
	sw	t1, 0x13c(s0)
	jal long_sleep_delay
	nop
	li	t1, 2
	sw	t1, 0x13c(s0)

	/* Turn off ETH Phy */
	li	t1, 0x84001000
	sw	t1, 0xb801605c

	/* Turn off HDMI_TMDS */
	li	t1, 0x8ca0f0
	sw	t1, 0xb800d040

	/* Turn off HDMI_PLL */
	li	t1, 0xa70
	sw	t1, 0xb800d044

	/* Mars has no Audio Codec */

	/* Mars has no ADC */

	/* Turn off DAC */
	li	t1, 0x7bbefb
	sw	t1, 0xb8018200
	li	t1, 0x7bbefb
	sw	t1, 0xb8018204

	/* Turn off SATA PHY0 */
/* Let sata driver turn off this by itself */
//	li	t1, 0x12de0201
//	sw	t1, 0xb8012be8
	/* Turn off SATA PHY1 */
//	li	t1, 0x12de0201
//	sw	t1, 0xb8012ce8
	/* Turn off SATA FUNC */
	li	t1, 3
	sw	t1, 0xb8000340

	/* Turn off USB Phy */
	li	t1, 0x40
	sw	t1, 0xb8013054


	/* Turn on 12V & 5V */
	/* If s5 < 0, no handle 12V/5V; if t6 > 31, handle GPIO 1; if t6 <= 31, handle GPIO 0 */
	/* We moved this section of "turning on 12V5V" here because it should be behind handling PLL and clock, or else there would be noise. */
	bltz	s5, mars_no_12v5v_on
	nop
	li	a0, 'b'
	jal	print_char
	nop
	li	t5, 0x10000000
	and	t5, s5, t5		/* If t5 is "non-zero", sub-parameter is hion, otherwise sub-parameter is hioff. */
	li	t6, ~0x10000000
	and	t6, s5, t6		/* t6 stores 12V/5V GPIO number */
	li	t0, 31
	subu	t0, t6, t0
	bgtz	t0, mars_gp1_on
	nop

	li	t1, 0x1
	sll	t1, t1, t6
	bnez	t5, mars_handle_hion_2
	nop
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATO
	or	t2, t2, t1
	xor	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATO
	j	mars_no_12v5v_on
	nop
mars_handle_hion_2:
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATO
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP0DATO
	j	mars_no_12v5v_on
	nop
mars_gp1_on:
	addiu	t0, -1
	li	t1, 0x1
	sll	t1, t1, t0
	bnez	t5, mars_handle_hion_3
	nop
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DATO
	or	t2, t2, t1
	xor	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DATO
	j	mars_no_12v5v_on
	nop
mars_handle_hion_3:
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DATO
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+MARS_MIS_GP1DATO
mars_no_12v5v_on:

//	.extern setup_boot_image
//	jal	setup_boot_image
//	nop

	move	v0, s6
	lw	s0, 0(sp)
	lw	s1, 4(sp)
	lw	s2, 8(sp)
	lw	s3, 12(sp)
	lw	s4, 16(sp)
	lw	s5, 20(sp)
	lw	s6, 24(sp)
	lw	s7, 28(sp)
	lw	ra, 32(sp)
	jr	ra
	addiu	sp, sp, 108
END(mars_cpu_suspend)

end:


