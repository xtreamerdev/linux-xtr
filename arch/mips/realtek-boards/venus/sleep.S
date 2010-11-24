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
 *   a1 stores 12V/5V GPIO parameter. If it is -1, it means that there is no value for 12V/5V GPIO parameter. When the bit 28 of a1 is 1, it represents that 12V/5V will be on when 12V/5V GPIO is high. Bits 0-27 store the GPIO number of 12V/5V, and the value of it will locate between 0 and 35.
 *   a2 stores options:
 *       bit0: 0 represents normal resuming procedure. 1 means we want to use "reboot" to replace normal resuming procedure.
 *   a3 stores HW info:
 *       bit0-3: CPU type. 0 represents Venus, 1 represents Neptune, and 2 represents Mars.
 *       bit4: Package type. 0 represents QFP, and 1 represents BGA.
 *   Parameter5, which means 52(sp), stores the IRRP value of IR power key:
 *   Parameter6, which means 56(sp), stores the IRRP value of IR eject key:
 *   Parameter7, which means 60(sp), stores the GPIO pin num of GPIO power key:
 *   Parameter8, which means 64(sp), stores the GPIO pin num of GPIO eject key:
 *   Parameter9, which means 68(sp), stores the IR type:
 *
 * These registers are used for these purposes in the whole function:
 *   s0 is used to save base address
 *   s2 is used to save the register for controlling DCU PAD
 *   s3 is used to save HW info
 *   s4 is used to save options
 *   s5 is used to save 12V/5V GPIO parameter
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
	move	s3, a3		/* HW info */


/*********************************************************************************/

	andi	t0, s3, 0xF			/* For Neptune, these components are inside the chip. */
	beq	t0, zero, no_neptune_components_off
	nop

	/* Turn off ETH Phy */
	li	t1, 0x84000800
	sw	t1, 0xb801605c

	/* Turn off HDMI_TMDS */
	li	t1, 0x463000
	sw	t1, 0xb800d02c

	/* Turn off HDMI_PLL */
	li	t1, 0x3f
	sw	t1, 0xb800d030

	/* Turn off Audio Codec */
	li	t1, 0xaa000000
	sw	t1, 0xb80061dc
no_neptune_components_off:

	/* Turn off ADC */
	lw	t1, 0xb80190fc
	li	t2, ~0x03c00000
	and	t1, t2, t1
	sw	t1, 0xb80190fc

	/* Turn off DAC */
	li	t1, 0
	sw	t1, 0xb80180d0
	sw	t1, 0xb80180d4

	lui	s0, 0xb800
	/* Turn off most clocks except SB1, DCU, and MISC */
	li	t1, 0x9002
	sw	t1, 0x4(s0)
	
	/* usb, RSTN_USB(1800_0000[bit15])=RSTN_USB_PHY(1800_0000[bit17])=0 */
	lw	t1, 0x0(s0)
	li	t2, ~0x28000
	and	t1, t2, t1
	//li	t1, 0x17fff
	sw	t1, 0x0(s0)

	/* disable ETN PLL */
	li	t1, 0x06
	sw	t1, 0x80(s0)
	
	/* disable DISP PLL */
	li	t1, 0x06
	sw	t1, 0xc0(s0)
	
	/* disable 1394 */
	li	t1, 0x2e
	sw	t1, 0x9c(s0)

	/* disable ACPU PLL */
	li	t1, 0x6
	sw	t1, 0xc8(s0)
	
/* For DRAM non-selfrefresh mode, putting code and data in cache is also necessary. If no doing this, system will halt on "turn on 27M OSC". */
	/* move code to I-cache */
	la	t1, _esleeptext
	addiu	t1, 16
	la	t2, _sleeptext
load1:
.set mips3
	cache	ICACHE_ADDR_FILL, 0(t2)
.set mips0
	sltu	t0, t2, t1
	bnez	t0, load1
	addiu	t2, 16

	/* move data to D-cache */
	la	t1, _esleepdata
	addiu	t1, 16
	la	t2, _sleepdata
load2:
.set mips3
	cache	DCACHE_ADDR_FETCH, 0(t2)
.set mips0
	sltu	t0, t2, t1
	bnez	t0, load2
	addiu	t2, 16

	/* move stack to D-cache */
	li	t1, 256
	move	t2, sp
	addi	t2, 36+36			/* Local variables and the parameters passed from upper function must be put into cache */
load3:
.set mips3
	cache	DCACHE_ADDR_FETCH, 0(t2)
.set mips0
	addiu	t1, -1
	bnez	t1, load3
	addiu	t2, -16


start:

	/* Check if DRAM needs to go into self-refresh mode */
	/* If using Venus CPU and the package type is QFP, no go into self-refresh */
	andi	t0, s3, 0x1f
	beq	t0, zero, no_dram_selfrefresh_on
	nop

/* DDR goes into self-refresh mode. We cannot do this on Venus/QFP boards because there is no auto-refresh pin for them. */

	/* down DCU PLL */	/* At the beginning, they want to use this to lower memory clock and save power, but I found that some boards will have problem after waking up. Disable this. */
//	li      t1, 0x00a1cc14
//	sw      t1, 0x94(s0)

	/* These are 3 actions: 1. self-refresh, 2. DCU clock, 3. DDR PLL */
	li	t1, 0x08
	sw	t1, 0x880c(s0)
	jal	sleep_delay
	nop

	/* Turn off DCU clock */
	li	t1, 0x8002
	sw	t1, 0x4(s0)
	jal	sleep_delay
	nop
	
	/* disable DDR PLL */
	li	t1, 0x06
	sw	t1, 0x98(s0)
	jal	sleep_delay
	nop

	andi	t0, s3, 0xF
	beq	t0, zero, no_dram_selfrefresh_on	/* Venus doesn't need to turn off PADs in DCU */
	nop
/* According to tests, turning DCU pads off may cause system to be unstable */
/*	lw	s2, 0xB801a158
	li	t0, 0x124910
	sw	t0, 0xB801a158
	lw	t0, 0xB801a154
	li	t1, 0x40000000
	or	t0, t0, t1
	sw	t0, 0xB801a154
*/
	/* Let IDE PADs be input. Do this while handling DDR PADs. */
	lw	t0, 0xB8000110
	li	t1, 0xE0
	or	t0, t0, t1
	sw	t0, 0xB8000110
no_dram_selfrefresh_on:

	/* HW says that this is useless, but we still need it because IC bug on 27M OSC. */
	li	t1, 0x0C481001
	sw	t1, 0x8804(s0)
	jal long_sleep_delay
	nop
	/* Turn off PLL  , turn on 27M OSC*/
	li	t1, 0x01
	sw	t1, 0x8(s0)

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
	li	t4, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DIR
	li	t7, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	li	t0, 32
	move	t1, t6
shif_12v5v_off:
	move	t6, t1
	addiu	t4, 4
	addiu	t7, 4
	subu	t1, t6, t0
	bgez	t1, shif_12v5v_off
	nop	
	addiu	t4, -4
	addiu	t7, -4

	li	t1, 0x1
	sll	t1, t1, t6
	/* Let the direction of corresponding GPIO bit be output. */
	lw	t2, 0(t4)
	or	t2, t2, t1
	sw	t2, 0(t4)
	/**********************************************************/
	bnez	t5, handle_hion_0
	nop
	lw	t2, 0(t7)
	or	t2, t2, t1
	sw	t2, 0(t7)
	j	no_12v5v_off
	nop
handle_hion_0:
	lw	t2, 0(t7)
	or	t2, t2, t1
	xor	t2, t2, t1
	sw	t2, 0(t7)
no_12v5v_off:
	li	a0, '0'
	jal	print_char
	nop

resleep:
	lui	s0, 0xb801
	lui	t1, 0x1F
	ori	t1, 0xDE20
	sw	t1, VENUS_MIS_ISR(s0)	/* Clear all interrupts that could wake up system */

	lui	s0, 0xb800
	/* disable BUS2 PLL */
	li	t1, 0x06
	sw	t1, 0x90(s0)

	/* Turn off SB1 clock */
	andi	t0, s3, 0x1f
	beq	t0, zero, no_dram_sb1_dram_off
	li	t1, 0x1002
	li	t1, 0x0002
no_dram_sb1_dram_off:
	sw	t1, 0x4(s0)
	
	/* Turn off SB2 & SCPU clock */
	li	t1, 0x00
	sw	t1, 0x10(s0)
	
	/**************************/
	nop
	nop
	nop
	nop

	lui	s0, 0xb801

/* When alarm shall happen, half-second and alarm interrupts may not appear
 * at the same time. Therefore, alarm interrupt may be lost. Add some delay
 * to prevent from this. */ 
	jal     sleep_delay
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

check_ir:
	/* Check IR interrupt here */
	lw	t1, VENUS_MIS_IR_SR(s0)
	andi	t1, 0x1
	beq	t1, zero, check_front_panel
	nop
	li	t1, 0x1
	sw 	t1, VENUS_MIS_IR_SR(s0)
	lw	t1, VENUS_MIS_IR_RP(s0)
	/* if protocol id is RC6, shift-right for 10 bits */
	lw	t3, 68(sp)
	lui	t2, 0x0000
	ori	t2, 0x0006 /* RC6 = 6 */
	bne	t3, t2, ir_examine
	lui	t4, 0x0000
	ori	t4, 0x000a
	srl	t1, t1, t4
	lui	t3, 0x003f
	ori	t3, 0xffdf
	and	t1, t1, t3
ir_examine:
	lw	t2, 52(sp)
	beq	t1, t2, wake_up		/* power key - IR */
	li	s6, 0
	lw	t2, 56(sp)
	beq	t1, t2, wake_up		/* eject key - IR */
	li	s6, 1
reread_irda:
	li	t1, 0x1
	sw	t1, VENUS_MIS_IR_SR(s0)
	lw	t1, VENUS_MIS_IR_RP(s0)
	lw	t1, VENUS_MIS_IR_SR(s0)
	andi	t1, 0x1
	bnez	t1, reread_irda
	nop

check_front_panel:
#if 0		/* Turn this on when VFD isn't plugged or it will send wrong data */
	j	check_gpio_powerkey
	nop
#endif
	/* Check front panel interrupt here */
#ifndef CONFIG_PM_SLEEP_POLLING
	lw	t1, VENUS_MIS_ISR(s0)
	lui	t2, 0x1
	ori	t2, 0x8000
	and	t1, t2
	beq	t1, zero, check_gpio_powerkey
	nop
#endif
	lw	t1, VENUS_MIS_UMSK_ISR_KPADAL(s0)
	lw	t2, VENUS_MIS_UMSK_ISR_KPADAL(s0)
	lui	t0, 0xFFFF
	ori	t0, 0xFFF0
	sw	t0, VENUS_MIS_UMSK_ISR_KPADAL(s0) 
	li	t0, 0x1
	bne	t0, s7, general_frontpanel
	nop
nucom_frontpanel:
	andi	t1, 0x200
	bnez	t1, wake_up		/* power key - Front Panel */
	li	s6, 0
	andi	t2, 0x40
	bnez	t2, wake_up		/* eject key - Front Panel */
	li	s6, 1
	b	check_gpio_powerkey
	nop

general_frontpanel:
	andi	t1, 0x10
	bnez	t1, wake_up		/* power key - Front Panel */
	li	s6, 0
	andi	t2, 0x100
	bnez	t2, wake_up		/* eject key - Front Panel */
	li	s6, 1

check_gpio_powerkey:
	lw	t6, 60(sp)
	bltz	t6, no_gpio_powerkey
	nop
	li	t7, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATI
	li	t0, 32
	move	t1, t6
shif_gpio_powerkey:
	move	t6, t1
	addiu	t7, 4
	subu	t1, t6, t0
	bgez	t1, shif_gpio_powerkey
	nop	
	addiu	t7, -4

	li	t2, 0x1
	sll	t2, t2, t6
	lw	t3, 0(t7)
	and	t3, t3, t2
	beq	t3, zero, wake_up		/* power key - GPIO */
	li	s6, 0
no_gpio_powerkey:

check_gpio_ejectkey:
	lw	t0, 64(sp)
	bltz	t0, no_gpio_ejectkey
	nop
	li	t7, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATI
	li	t0, 32
	move	t1, t6
shif_gpio_ejectkey:
	move	t6, t1
	addiu	t7, 4
	subu	t1, t6, t0
	bgez	t1, shif_gpio_ejectkey
	nop	
	addiu	t7, -4

	li	t2, 0x1
	sll	t2, t2, t6
	lw	t3, 0(t7)
	and	t3, t3, t2
	beq	t3, zero, wake_up		/* eject key - GPIO */
	li	s6, 1
no_gpio_ejectkey:

check_rtc:
	lw	t1, VENUS_MIS_RTCCR(s0)
	lw	t2, VENUS_MIS_ISR(s0)
#ifdef CONFIG_PM_SLEEP_POLLING
	lw	t0, VENUS_MIS_RTCMIN(s0)
	lw	t3, VENUS_MIS_ALMMIN(s0)
	bne	t0, t3, check_half_second
	nop
	lw	t0, VENUS_MIS_RTCHR(s0)
	lw	t4, VENUS_MIS_ALMHR(s0)
	bne	t0, t4, check_half_second
	nop
	lw	t0, VENUS_MIS_RTCDATE1(s0)
	lw	t5, VENUS_MIS_ALMDATE1(s0)
	bne	t0, t5, check_half_second
	nop
	lw	t0, VENUS_MIS_RTCDATE2(s0)
	lw	t6, VENUS_MIS_ALMDATE2(s0)
	bne	t0, t6, check_half_second
	nop
	bne	t3, zero, rtc_wake_up
	nop
	bne	t4, zero, rtc_wake_up
	nop
	bne	t5, zero, rtc_wake_up
	nop
	bne	t6, zero, rtc_wake_up
	nop
	jal	check_half_second
	nop
#else
	andi	t0, t1, 0x10
	beq	t0, zero, check_half_second
	nop
	andi	t0, t2, 0x2000
	bnez	t0, rtc_wake_up
	nop
#endif
check_half_second:
	andi	t0, t2, 0x200
	bnez	t0, half_second
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
	b	resleep
	nop
half_second:
//	li	t0, 0x200
//	sw	t0, VENUS_MIS_ISR(s0)
	lw	t0, VENUS_MIS_RTCSEC(s0)
	andi	t0, 0x1
	bne	t0, zero, resleep
	nop
vfd_second:				/* RTC reaches one second interrupt and then refreshes vfd */
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
	b	resleep
	nop

rtc_wake_up:
	ori	t1, 0x10
	xori	t1, 0x10
	sw	t1, VENUS_MIS_RTCCR(s0)
	li	s6, 2

wake_up:
#ifdef CONFIG_PM_SLEEP_REBOOT
	andi	t1, s4, 0x1
	beq	t1, zero, no_pm_sleep_reboot
	nop
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
no_pm_sleep_reboot:
#else
/* For Venus and Neptune, resuming may fail. Therefore, use watchdog timeout
 * to prevent from this. */
	sw	zero, VENUS_MIS_TCWCR(s0)
	jal	long_sleep_delay
	nop
	li	t1, 0x1
	sw	t1, VENUS_MIS_TCWTR(s0)
#endif

	lui	s0, 0xb800
	/* turn on BUS2 PLL 1 */
	li	t1, 0x03
	sw	t1, 0x90(s0)
	jal	sleep_delay
	nop

	/* turn on BUS2 PLL 2 */
	li	t1, 0x01
	sw	t1, 0x90(s0)
	
	/* Turn on PLL as clock. No use 27M OSC. */
	li	t1, 0x00
	sw	t1, 0x8(s0)

	/* HW says that this is useless, but we still need it because IC bug on 27M OSC. */
	li	t1, 0x0C485001
	sw	t1, 0x8804(s0)
	jal	long_sleep_delay
	nop

	li	a0, '1'
	jal	print_char
	nop

	/* Check if DRAM needs to go out of self-refresh mode */
	andi	t0, s3, 0x1f
	beq	t0, zero, no_dram_selfrefresh_off
	nop

/* DDR goes out of self-refresh mode. We cannot do this on Venus/QFP boards because there is no auto-refresh pin for them. */

	andi	t0, s3, 0xF
	beq	t0, zero, no_dcu_pad_restore	/* Venus doesn't need to turn off PADs in DCU */
	nop
/*	sw	s2, 0xB801a158
	lw	t0, 0xB801a154
	li	t1, ~0x40000000
	and	t0, t0, t1
	sw	t0, 0xB801a154
*/
	/* Restore IDE PADs. Do this while handling DDR PADs. */
	lw	t0, 0xB8000110
	li	t1, 0xE0
	xor	t0, t0, t1
	sw	t0, 0xB8000110
no_dcu_pad_restore:

	/* up DCU PLL */
//	li	t1, 0x01c1cc14
//	sw	t1, 0x94(s0)

	/* There are 3 actions: 1. self-refresh, 2. DCU clock, 3. DDR PLL */
	li	t1, 0x03
	sw	t1, 0x98(s0)
	jal	sleep_delay
	nop

	/* turn on DDR PLL 1 */
	li	t1, 0x01
	sw	t1, 0x98(s0)
	jal	sleep_delay
	nop
	
	/* Turn on DCU clock */
	li	t1, 0x1002
	sw	t1, 0x4(s0)
	jal	sleep_delay
	nop

	/* DDR exit self-refresh */
	li	t1, 0x04
	sw	t1, 0x880c(s0)
	jal	sleep_delay
	nop

no_dram_selfrefresh_off:

	/* flush data segment in D-cache */
	la	t1, _esleepdata
	addiu	t1, 16
	la	t2, _sleepdata
flush2:
.set mips3
	cache	DCACHE_ADDR_WRITEBACK, 0(t2)
.set mips0
	sltu	t0, t2, t1
	bnez	t0, flush2
	addiu	t2, 16

	/* flush stack in D-cache */
	li	t1, 256
	move	t2, sp
	addi	t2, 36+36			/* Local variables and the parameters passed from upper function must be put into cache */
flush3:
.set mips3
	cache	DCACHE_ADDR_WRITEBACK, 0(t2)
.set mips0
	addiu	t1, -1
	bnez	t1, flush3
	addiu	t2, -16

	/* enable CLK_EN_SB2 & CLK_EN_SCPU */
	/* these codes must be added for Neptune but it isn't necessary for venus*/
	li	t1, 0x03
	sw	t1, 0x10(s0)

	/* enable ETN PLL 1 */
	li	t1, 0x03
	sw	t1, 0x80(s0)

	jal	sleep_delay
	nop

	/* enable ETN PLL 2 */
	li	t1, 0x01
	sw	t1, 0x80(s0)

	/* enable DISP PLL 1 */
	li	t1, 0x03
	sw	t1, 0xc0(s0)
	
	jal	sleep_delay
	nop
	
	/* enable DISP PLL 2 */
	li	t1, 0x01
	sw	t1, 0xc0(s0)
	
	/* enable 1394 */
	li	t1, 0x2a		/* cy test */
	sw	t1, 0x9c(s0)
	li	t1, 0x3b
	sw	t1, 0x9c(s0)
	
	jal	sleep_delay
	nop
	
	/* 1394 OEB = 0 */
	li	t1, 0x11
	sw	t1, 0x9c(s0)

	/* enable ACPU PLL 1 */
	li	t1, 0x03
	sw	t1, 0xc8(s0)

	jal	sleep_delay
	nop

	/* enable ACPU PLL 2 */
	li	t1, 0x01
	sw	t1, 0xc8(s0)

	/*jimmy_start..reset USB initial RSTN_USB_PHY=RSTN_USB=CLK_EN_USB=0*/
	/* usb, RSTN_USB_PHY(1800_0000[bit17])=1*/
	lw	t1, 0(s0)
	li	t2, (1 << 17)
	or	t1, t1, t2
	sw	t1, 0(s0)

	/* usb, delay at least 1.2ms( 300k cycles) */
	jal	long_sleep_delay
	nop

	/* usb, CLK_EN_USB(1800_0004[bit3])=1*/
	lw	t1, 4(s0)
	li	t2, (1 << 3)
	or	t1, t1, t2
	sw	t1, 4(s0)
        /*wait*/
	jal	sleep_delay
	nop

	/* usb, CLK_EN_USB(1800_0004[bit3])=0*/
	lw	t1, 4(s0)
	li	t2, ~0x8
	and	t1, t2, t1
	sw	t1, 4(s0)
        /*wait*/
	jal	sleep_delay
	nop

        /* RSTN_USB(1800_0000[bit15])=1 */
	lw	t1, 0(s0)
	li	t2, (1 << 15)
	or	t1, t1, t2
	sw	t1, 0(s0)
        /*wait*/
	jal	sleep_delay
	nop

	/* usb, CLK_EN_USB(1800_0004[bit3])=1*/
	lw	t1, 4(s0)
	li	t2, (1 << 3)
	or	t1, t1, t2
	sw	t1, 4(s0)

	jal	sleep_delay
	nop
	/*jimmy_end..reset USB initial RSTN_USB_PHY=RSTN_USB=CLK_EN_USB=0*/

	/* 1394 OEB = 1 */
	li	t1, 0x13
	sw	t1, 0x9c(s0)

	/* disable clocks */
	li	t1, 0x0000
	sw	t1, 0x4(s0)

	jal	sleep_delay
	nop

	/* De-assert reset */
	// marked by Frank 96/10/13 for Neptune resume error
	/*
	lui	t1, 0x7
	ori	t1, 0xffff
	sw	t1, 0x0(s0)
	jal	sleep_delay
	nop*/

	/* 1394 OEB = 0 */
	li	t1, 0x11
	sw	t1, 0x9c(s0)
	
	/* Turn on clock */
	/* We need to long delay here because for some power supplies, turn on 12V & 5V will draw 3.3V down and if we turn on clock at the same time, 3.3V may be drawn down to 2.4V. */
	jal	long_sleep_delay
	nop
	li	t0, 0x18ffff
	sw	t0, 0x4(s0)

        /* Turn on ADC */
	lw	t1, 0xb80190fc
	li	t2, 0x03c00000
	or	t1, t2, t1
	sw	t1, 0xb80190fc

	/* Turn on DAC */
	li	t1, 0x1fbaaa
	sw	t1, 0xb80180d0
	sw	t1, 0xb80180d4

	andi	t0, s3, 0xF			/* For Neptune, these components are inside the chip. */
	beq	t0, zero, no_neptune_components_on
	nop

	/* Turn on ETH Phy */
	li	t1, 0x84001000
	sw	t1, 0xb801605c

	/* Turn on HDMI_TMDS */
	li	t1, 0x4630f0
	sw	t1, 0xb800d02c

	/* Turn on HDMI_PLL */
	li	t1, 0x7f
	sw	t1, 0xb800d030

	/* Turn on Audio Codec */
	li	t1, 0x1
	sw	t1, 0x61d4(s0)
	sw	t1, 0x61d8(s0)
	li	t1, 0xff7e8180
	sw	t1, 0x61dc(s0)
	li	t1, 0x3aaaa
	sw	t1, 0x61e0(s0)

no_neptune_components_on:

	/* Turn on 12V & 5V */
	/* If s5 < 0, no handle 12V/5V; if t6 > 31, handle GPIO 1; if t6 <= 31, handle GPIO 0 */
	/* We moved this section of "turning on 12V5V" here because it should be behind handling PLL and clock, or else there would be noise. */
	bltz	s5, no_12v5v_on
	nop
	li	a0, 'b'
	jal	print_char
	nop
	li	t5, 0x10000000
	and	t5, s5, t5		/* If t5 is "non-zero", sub-parameter is hion, otherwise sub-parameter is hioff. */
	li	t6, ~0x10000000
	and	t6, s5, t6		/* t6 stores 12V/5V GPIO number */
	li	t4, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DIR
	li	t7, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_GP0DATO
	li	t0, 32
	move	t1, t6
shif_12v5v_on:
	move	t6, t1
	addiu	t4, 4
	addiu	t7, 4
	subu	t1, t6, t0
	bgez	t1, shif_12v5v_on
	nop	
	addiu	t4, -4
	addiu	t7, -4

	li	t1, 0x1
	sll	t1, t1, t6
	bnez	t5, handle_hion_1
	nop
	lw	t2, 0(t7)
	or	t2, t2, t1
	xor	t2, t2, t1
	sw	t2, 0(t7)
	j	no_12v5v_on
	nop
handle_hion_1:
	lw	t2, 0(t7)
	or	t2, t2, t1
	sw	t2, 0(t7)
no_12v5v_on:


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

/* sleep_delay delays about 30k clocks, which is about 1 ms when clock is 27MHZ, and 0.146 ms when clock is 200MHZ */
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

/* long_sleep_delay delays about 1500k clocks, which is about 50 ms when clock is 27MHZ, and 7 ms when clock is 200MHZ */
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
LEAF(print_registerr)
	li	t0, 32
	li	t1, 0xf			/* mask */
	li	t2, 0xa
print_registerr_while:
	lw	t6, 0xA0000000+VENUS_IO_PORT_BASE+VENUS_MIS_U0LSR
	andi	t6, UART_LSR_THRE
	beqz	t6, print_registerr_while
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
	bnez	t0, print_registerr_while
	nop
	jr	ra
	nop
END(print_registerr)

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
end:


