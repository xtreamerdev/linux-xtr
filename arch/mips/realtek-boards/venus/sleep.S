/*
 * Venus Assembler Sleep/WakeUp Management Routines
 *
 * Copyright (c) 2006 Colin <colin@realtek.com.tw>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2006-05-23:	Colin	First version.
 */

//#include <linux/linkage.h>
//#include <asm/assembler.h>
//#include <asm/hardware.h>
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <venus.h>
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
 * venus_cpu_suspend()
 *
 * Make venus enter sleep mode
 *
 * Parameters:
 *   a0 stores board_id
 *   a1 stores 12V/5V GPIO parameter. When the bit 28 of a1 is 1, it means that 12V/5V will be on when 12V/5V GPIO is high. The other bits mean the GPIO number of 12V/5V.
 *   a2 stores options:
 *       bit0: 1 represents DRam goes into self-refresh while sleeping.
 *
 * These registers are for these purposes in this function:
 *   s0 is used to save base address
 *   s4 is used to save options
 *   s5 is used to save 12V/5V GPIO pin number
 *   s6 is used to save return value
 *   s7 is used to save board_id
 * 
 * Return 2 to represent that it is woken up by RTC alarm, 1 to represent that cdrom should be ejected, and 0 to represent that power on key is pressed.
 */
NESTED(venus_cpu_suspend, 0, sp)
	addiu	sp, sp, -36
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

/* If 12V/5V GPIO pin number is less than zero, we will check board_id and some boards have default value for it */
/* If it is over 35, we will print error message and let s5 = -1. */
	bltz	s5, next_board0
	nop
	li	t0, 36
	li	t1, ~0x10000000
	and	t1, s5, t1		/* t1 stores 12V/5V GPIO number */
	subu    t0, t1, t0
	bltz	t0, finish_12v5v
	nop
	li	s5, -1
	li      a0, '0'
	jal     print_errmsg
	nop
	j	finish_12v5v
	nop
next_board0:
	li	t0, 0x1
	bne	t0, s7, next_board1
	nop
	j	finish_12v5v
	li	s5, 33 | 0x10000000
next_board1:
	li	t0, 0xa
	bne	t0, s7, next_board2
	nop
	j	finish_12v5v
	li	s5, 34 | 0x10000000
next_board2:
	li	t0, 0xb
	bne	t0, s7, next_board3
	nop
	j	finish_12v5v
	li	s5, 34 | 0x10000000
next_board3:
	li	t0, 0x5000a
	bne	t0, s7, next_board4
	nop
	j	finish_12v5v
	li	s5, 34 | 0x10000000
next_board4:
	li	t0, 0x5
	bne	t0, s7, next_board5
	nop
	j	finish_12v5v
	li	s5, 33 | 0x10000000
next_board5:
	li	t0, 0x4
	bne	t0, s7, next_board6
	nop
	j	finish_12v5v
	li	s5, 12 | 0x10000000
next_board6:
	li	s5, -1
finish_12v5v:


/*********************************************************************************/

	/* Turn off ADC */
	lw	s1, 0xb80190fc
	li	s2, ~0x03c00000
	and	s1, s2, s1
	sw	s1, 0xb80190fc

	/* Turn off DAC */
	li	s1, 0
	sw	s1, 0xb80180d0
	sw	s1, 0xb80180d4

	lui	s0, 0xb800
	/* Turn off most clocks except SB1, DCU, and MISC */
	li	s1, 0x9002
	sw	s1, 0x4(s0)
	
	/* usb, RSTN_USB_PHY=0 */
	lw	s1, 0x0(s0)
	li	s2, ~0x68000
	and	s1, s2, s1
	//li	s1, 0x17fff
	sw	s1, 0x0(s0)

	/* disable ETN PLL */
	li	s1, 0x06
	sw	s1, 0x80(s0)
	
	/* disable DISP PLL */
	li	s1, 0x06
	sw	s1, 0xc0(s0)
	
	/* disable 1394 / ACPU PLL */
	li	s1, 0x2e
	sw	s1, 0x9c(s0)
	
/*	mfc0	s1, CP0_STATUS
	li	s2, ~0x1
	and	s1, s2, s1
	mtc0	s1, CP0_STATUS*/
	
	/* move code to I-cache */
	la	s1, _esleeptext
	addiu	s1, 16
	la	s2, _sleeptext

load1:
.set mips3
	cache	ICACHE_ADDR_FILL, 0(s2)
.set mips0
	sltu	s3, s2, s1
	bnez	s3, load1
	addiu	s2, 16

	/* move data to D-cache */
/*	li	s1, 256
	la	s2, _sleepdata

load2:
.set mips3
	cache	DCACHE_ADDR_FETCH, 0(s2)
.set mips0
	addiu	s1, -1
	bnez	s1, load2
	addiu	s2, 16
	move	s6, s2
	addiu	s6, -32			// S6 will be the stack when the C function is invoked
*/
	/* move data to D-cache */
	la	s1, _esleepdata
	addiu	s1, 16
	la	s2, _sleepdata

load2:
.set mips3
	cache	DCACHE_ADDR_FETCH, 0(s2)
.set mips0
	sltu	s3, s2, s1
	bnez	s3, load2
	addiu	s2, 16

	/* move stack to D-cache */
	li	s1, 256
	move	s2, sp

load3:
.set mips3
	cache	DCACHE_ADDR_FETCH, 0(s2)
.set mips0
	addiu	s1, -1
	bnez	s1, load3
	addiu	s2, -16


start:
	/* down DCU PLL */	/* At the beginning, they want to use this to lower memory clock and save power, but I found that some boards will have problem after waking up. Disable this. */
//	li      s1, 0x00a1cc14
//	sw      s1, 0x94(s0)

	/* DDR go into self-refresh. In fact, this is not a real self-refresh */
/* Important!!! These 3 actions: 1. self-refresh, 2. DCU clock, 3. DDR PLL, are disabled because the layout problem of small black */
	andi	t0, s4, 0x1
	beq	t0, zero, no_dram_selfrefresh_on
	li	s1, 0x08
	sw	s1, 0x880c(s0)
	jal	sleep_delay
	nop
	
	/* Turn off DCU clock */
	li	s1, 0x8002
	sw	s1, 0x4(s0)
	jal	sleep_delay
	nop
	
	/* disable DDR PLL */
	li	s1, 0x06
	sw	s1, 0x98(s0)
	jal	sleep_delay
	nop
no_dram_selfrefresh_on:

	li	s1, 0x0C481001
	sw	s1, 0x8804(s0)
	
	jal long_sleep_delay
	nop

	/* Turn off PLL  , turn on 27M OSC*/
	li	s1, 0x01
	sw	s1, 0x8(s0)

	/* Turn off 12V & 5V */
	/* If s5 < 0, no handle 12V/5V; if t6 > 31, handle GPIO 1; if t6 <= 31, handle GPIO 0 */
	bltz	s5, no_12v5v_off
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
	bgtz	t0, gp1_off
	nop

	li	t1, 0x1
	sll	t1, t1, t6
	/* Let the direction of corresponding GPIO bit be output. */
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DIR
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DIR
	/**********************************************************/
	bnez	t5, handle_hion_0
	nop
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	j	no_12v5v_off
	nop
handle_hion_0:
	li	t2, -1
	xor	t1, t2, t1
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	and	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	j	no_12v5v_off
	nop
gp1_off:
	addiu	t0, -1
	li	t1, 0x1
	sll	t1, t1, t0
	/* Let the direction of corresponding GPIO bit be output. */
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DIR
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DIR
	/**********************************************************/
	bnez	t5, handle_hion_1
	nop
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DATO
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DATO
	j	no_12v5v_off
	nop
handle_hion_1:
	li	t2, -1
	xor	t1, t2, t1
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DATO
	and	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DATO
no_12v5v_off:
	li	a0, '0'
	jal	print_char
	nop

resleep:
	lui	s0, 0xb801
	lui	s1, 0x7
	ori	s1, 0xFE20
	sw	s1, VENUS_MIS_ISR(s0)	/* Clear all interrupts that could wake up system */

	lui	s0, 0xb800
	/* disable BUS2 PLL */
	li	s1, 0x06
	sw	s1, 0x90(s0)

	/* Turn off SB1 clock */
	li	s1, 0x1002
	sw	s1, 0x4(s0)
	
	/* Turn off SB2 & SCPU clock */
	li	s1, 0x00
	sw	s1, 0x10(s0)
	
	/**************************/
	nop
	nop
	nop
	nop

	lui	s0, 0xb801

check_ir:
	/* Check IR interrupt here */
	lw	s1, VENUS_MIS_IR_SR(s0)
	andi	s1, 0x1
	beq	s1, zero, check_front_panel
	nop
	lui	s1, 0x1
	sw 	s1, VENUS_MIS_IR_SR(s0)
	lw	s1, VENUS_MIS_IR_RP(s0)
	lui	s2, 0xFF00
	ori	s2, 0xFC03
	beq	s1, s2, wake_up		/* power key - IR */
	li	s6, 0
	lui	s3, 0xE817
	ori	s3, 0xFC03
	beq	s1, s3, wake_up		/* eject key - IR */
	li	s6, 1

check_front_panel:
#if 0		/* Turn this on when VFD isn't plugged or it will send wrong data */
	j	check_rtc
	nop
#endif
	/* Check front panel interrupt here */
#ifndef CONFIG_PM_SLEEP_POLLING
	lw	s1, VENUS_MIS_ISR(s0)
	lui	s2, 0x1
	ori	s2, 0x8000
	and	s1, s2
	beq	s1, zero, check_rtc
	nop
#endif
	lw	s1, VENUS_MIS_UMSK_ISR_KPADAL(s0)
	lw	s2, VENUS_MIS_UMSK_ISR_KPADAL(s0)
	lw	s3, VENUS_MIS_UMSK_ISR_KPADAL(s0)
	lui	s3, 0xFFFF
	ori	s3, 0xFFF0
	sw	s3, VENUS_MIS_UMSK_ISR_KPADAL(s0) 
	li	t0, 0x1
	bne	t0, s7, general_frontpanel
	nop
nucom_frontpanel:
	andi	s1, 0x200
	bnez	s1, wake_up		/* power key - Front Panel */
	li	s6, 0
	andi	s2, 0x40
	bnez	s2, wake_up		/* eject key - Front Panel */
	li	s6, 1
	b	check_rtc
	nop

general_frontpanel:
	andi	s1, 0x10
	bnez	s1, wake_up		/* power key - Front Panel */
	li	s6, 0
	andi	s2, 0x100
	bnez	s2, wake_up		/* eject key - Front Panel */
	li	s6, 1

check_rtc:
	lw	s1, VENUS_MIS_RTCCR(s0)
	lw	s2, VENUS_MIS_ISR(s0)
#ifdef CONFIG_PM_SLEEP_POLLING
	lw	t0, VENUS_MIS_RTCMIN(s0)
	lw	t1, VENUS_MIS_ALMMIN(s0)
	bne	t0, t1, check_half_second
	nop
	lw	t0, VENUS_MIS_RTCHR(s0)
	lw	t1, VENUS_MIS_ALMHR(s0)
	bne	t0, t1, check_half_second
	nop
	lw	t0, VENUS_MIS_RTCDATE1(s0)
	lw	t1, VENUS_MIS_ALMDATE1(s0)
	bne	t0, t1, check_half_second
	nop
	lw	t0, VENUS_MIS_RTCDATE2(s0)
	lw	t1, VENUS_MIS_ALMDATE2(s0)
	bne	t0, t1, check_half_second
	nop
	jal	rtc_wake_up
	nop
#else
	andi	t0, s1, 0x10
	beq	t0, zero, check_half_second
	nop
	andi	t0, s2, 0x2000
	bnez	t0, rtc_wake_up
	nop
#endif
check_half_second:
	andi	t0, s2, 0x200
	bnez	t0, half_second	
//	lui	s5, 0x1
//	ori	s5, 0xBE20
//	sw	s5, VENUS_MIS_ISR(s0)	/* Who wakes CPU up? */
	li	a0, '?'
	jal	print_char
	nop
	b	resleep
	nop
half_second:
//	li	s3, 0x200
//	sw	s3, VENUS_MIS_ISR(s0)
	lw	s3, VENUS_MIS_RTCSEC(s0)
	andi	s3, 0x1
	bne	s3, zero, resleep
	nop
vfd_second:				/* RTC reaches one second interrupt and then refreshes vfd */
	li	a0, '.'			/* print '.' every second */
	jal	print_char
	nop
#ifdef CONFIG_REALTEK_VFD
//	move	s7, sp
//	move	sp, s6
	lw	a0, VENUS_MIS_RTCHR(s0)
	lw	a1, VENUS_MIS_RTCMIN(s0)
	lw	a2, VENUS_MIS_RTCSEC(s0)
	jal	update_vfd
	nop
//	move	sp, s7
#endif
	b	resleep
	nop

rtc_wake_up:
	ori	s1, 0x10
	xori	s1, 0x10
	sw	s1, VENUS_MIS_RTCCR(s0)
	li	s6, 2

wake_up:
	lui	s0, 0xb800
	/* turn on BUS2 PLL 1 */
	li	s1, 0x03
	sw	s1, 0x90(s0)
	
	jal	sleep_delay
	nop

	/* turn on BUS2 PLL 2 */
	li	s1, 0x01
	sw	s1, 0x90(s0)
	
	/* Turn on PLL as clock */
	li	s1, 0x00
	sw	s1, 0x8(s0)

	li	s1, 0x0C485001
	sw	s1, 0x8804(s0)
	
	/* Turn on 12V & 5V */
	/* If s5 < 0, no handle 12V/5V; if t6 > 31, handle GPIO 1; if t6 <= 31, handle GPIO 0 */
	bltz	s5, no_12v5v_on
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
	bgtz	t0, gp1_on
	nop

	li	t1, 0x1
	sll	t1, t1, t6
	bnez	t5, handle_hion_2
	nop
	li	t2, -1
	xor	t1, t2, t1
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	and	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	j	no_12v5v_on
	nop
handle_hion_2:
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	j	no_12v5v_on
	nop
gp1_on:
	addiu	t0, -1
	li	t1, 0x1
	sll	t1, t1, t0
	bnez	t5, handle_hion_3
	nop
	li	t2, -1
	xor	t1, t2, t1
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DATO
	and	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DATO
	j	no_12v5v_on
	nop
handle_hion_3:
	lw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DATO
	or	t2, t2, t1
	sw	t2, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP1DATO
no_12v5v_on:
	li	a0, '1'
	jal	print_char
	nop

	/* up DCU PLL */
//	li	s1, 0x01c1cc14
//	sw	s1, 0x94(s0)

	/* turn on DDR PLL 1 */
/* Important!!! These 3 actions: 1. self-refresh, 2. DCU clock, 3. DDR PLL, are disabled because the layout problem of small black */
	andi	t0, s4, 0x1
	beq	t0, zero, no_dram_selfrefresh_off
	li	s1, 0x03
	sw	s1, 0x98(s0)
	jal	sleep_delay
	nop

	/* turn on DDR PLL 1 */
	li	s1, 0x01
	sw	s1, 0x98(s0)
	jal	sleep_delay
	nop
	
	/* Turn on DCU clock */
	li	s1, 0x1002
	sw	s1, 0x4(s0)
	jal	sleep_delay
	nop

	/* DDR exit self-refresh */
	li	s1, 0x04
	sw	s1, 0x880c(s0)
	jal	sleep_delay
	nop
no_dram_selfrefresh_off:

	/* flush data segment in D-cache */
	la	s1, _esleepdata
	addiu	s1, 16
	la	s2, _sleepdata

flush2:
.set mips3
	cache	DCACHE_ADDR_WRITEBACK, 0(s2)
.set mips0
	sltu	s3, s2, s1
	bnez	s3, flush2
	addiu	s2, 16

	/* flush stack in D-cache */
	li	s1, 256
	move	s2, sp

flush3:
.set mips3
	cache	DCACHE_ADDR_WRITEBACK, 0(s2)
.set mips0
	addiu	s1, -1
	bnez	s1, flush3
	addiu	s2, -16

	/* enable CLK_EN_SB2 & CLK_EN_SCPU */
	/* these codes must be added for Neptune but itn't necessary for venus*/
	li	s1, 0x03
	sw	s1, 0x10(s0)

	/* enable ETN PLL 1 */
	li	s1, 0x03
	sw	s1, 0x80(s0)

	jal	sleep_delay
	nop

	/* enable ETN PLL 2 */
	li	s1, 0x01
	sw	s1, 0x80(s0)

	/* enable DISP PLL 1 */
	li	s1, 0x03
	sw	s1, 0xc0(s0)
	
	jal	sleep_delay
	nop
	
	/* enable DISP PLL 2 */
	li	s1, 0x01
	sw	s1, 0xc0(s0)
	
	/* enable 1394 / ACPU PLL */
	li	s1, 0x2a		/* cy test */
	sw	s1, 0x9c(s0)
	li	s1, 0x3b
	sw	s1, 0x9c(s0)
	
	jal	sleep_delay
	nop
	
	/* 1394 OEB = 0 */
	li	s1, 0x11
	sw	s1, 0x9c(s0)

	/*jimmy_start..reset USB initial RSTN_USB_PHY=RSTN_USB=CLK_EN_USB=0*/
	/* usb, RSTN_USB_PHY(1800_0000[bit17])=1*/
	lw	s1, 0(s0)
	li	s2, (1 << 17)
	or	s1, s1, s2
	sw	s1, 0(s0)

	/* usb, delay at least 1.2ms( 300k cycles) */
	li	s1, 100000
delay300K:
	addi	s1, -1
	bnez	s1, delay300K
	nop

	/* usb, CLK_EN_USB(1800_004[bit3])=1*/
	lw	s1, 4(s0)
	li	s2, (1 << 3)
	or	s1, s1, s2
	sw	s1, 4(s0)
        /*wait*/
	jal	sleep_delay
	nop

	/* usb, CLK_EN_USB(1800_004[bit3])=0*/
	lw	s1, 4(s0)
	li	s2, ~0x8
	and	s1, s2, s1
	sw	s1, 4(s0)
        /*wait*/
	jal	sleep_delay
	nop

        /* RSTN_USB(1800_0000[bit15])=1 */
	lw	s1, 0(s0)
	li	s2, (1 << 15)
	or	s1, s1, s2
	sw	s1, 0(s0)
        /*wait*/
	jal	sleep_delay
	nop
	/* usb, CLK_EN_USB(1800_004[bit3])=1*/
	lw	s1, 4(s0)
	li	s2, (1 << 3)
	or	s1, s1, s2
	sw	s1, 4(s0)

	jal	sleep_delay
	nop
	/*jimmy_end..reset USB initial RSTN_USB_PHY=RSTN_USB=CLK_EN_USB=0*/

	/* 1394 OEB = 1 */
	li	s1, 0x13;
	sw	s1, 0x9c(s0)

	/* disable clocks */
	li	s1, 0x0000
	sw	s1, 0x4(s0)

	jal	sleep_delay
	nop

	/* De-assert reset */
	// marked by Frank 96/10/13 for Neptune resume error
	/*
	lui	s1, 0x7
	ori	s1, 0xffff
	sw	s1, 0x0(s0)
	jal sleep_delay
	nop*/

	/* 1394 OEB = 0 */
	li	s1, 0x11;
	sw	s1, 0x9c(s0)
	
	/* Turn on clock */
	/* We need to long delay here because for some power supplies, turn on 12V & 5V will draw 3.3V down and if we turn on clock at the same time, 3.3V may be drawn down to 2.4V. */
	jal long_sleep_delay
	nop
	li	t0, 0xffff;
	sw	t0, 0x4(s0)

/*	mfc0	s1, CP0_STATUS
	li	s2, 0x1
	or	s1, s2, s1
	mtc0	s1, CP0_STATUS
	nop*/

        /* Turn on ADC */
	lw	s1, 0xb80190fc
	li	s2, 0x03c00000
	or	s1, s2, s1
	sw	s1, 0xb80190fc

	/* Turn on DAC */
	li	s1, 0x1fbaaa
	sw	s1, 0xb80180d0
	sw	s1, 0xb80180d4

	.extern setup_boot_image
	jal	setup_boot_image
	nop

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
	addiu	sp, sp, 36
END(venus_cpu_suspend)

LEAF(sleep_delay)
	/* delay nops */
	li	t0, 10000
sleep_while:
	addi	t0, -1
	bnez	t0, sleep_while
	nop
	jr	ra
	nop
END(sleep_delay)

LEAF(long_sleep_delay)
	/* delay nops */
	li	t0, 500000
long_sleep_while:
	addi	t0, -1
	bnez	t0, long_sleep_while
	nop
	jr	ra
	nop
END(long_sleep_delay)

/* This is to print the character in the register a0 */
LEAF(print_char)
print_char_while:
	lw	t6, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_U0LSR
	andi	t6, UART_LSR_THRE
	beqz	t6, print_char_while
	nop
	sw	a0, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_U0RBR_THR_DLL
	jr	ra
	nop
END(print_char)

/* This is to print the content of the register a0 */
LEAF(print_register)
	li	t0, 32
	li	t1, 0xf			/* mask */
	li	t2, 0xa
print_register_while:
	lw	t6, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_U0LSR
	andi	t6, UART_LSR_THRE
	beqz	t6, print_register_while
	nop

	addiu	t0, -4
	srl	t3, a0, t0
	and	t3, t3, t1
	blt	t3, t2, number
	nop
	addiu	t3, 0x27
number:
	addiu	t3, 0x30
	sw	t3, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_U0RBR_THR_DLL
	bnez	t0, print_register_while
	nop
	jr	ra
	nop
END(print_register)
end:

/* This is to print the string, "Error %c". %c represents the data in the register a0 */
LEAF(print_errmsg)
	addiu   sp, sp, -4
	sw      ra, 0(sp)

	move	t0, a0
	li	a0, 'E'
	jal	print_char
	nop
	li	a0, 'r'
	jal	print_char
	nop
	li	a0, 'r'
	jal	print_char
	nop
	li	a0, 'o'
	jal	print_char
	nop
	li	a0, 'r'
	jal	print_char
	nop
	li	a0, ' '
	jal	print_char
	nop
	move	a0, t0
	jal	print_char
	nop
	li	a0, ' '
	jal	print_char
	nop

	lw      ra, 0(sp)
	jr	ra
	addiu   sp, sp, 4
END(print_errmsg)


