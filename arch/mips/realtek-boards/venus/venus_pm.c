/*
 * Venus Power Management Routines
 *
 * Copyright (c) 2006 Colin <colin@realtek.com.tw>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2006-05-22:	Colin	Preliminary power saving code.
 */
#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <venus.h>
#include <asm/io.h>
#include <platform.h>
#include <linux/parser.h>
#include <linux/kernel.h>
#include <prom.h>

#define LOGO_INFO_ADDR1	0xbfc6ff00
#define LOGO_INFO_ADDR2	0xbfc3ff00
#define LOGO_INFO_SIZE	128

typedef struct {
	int	mode;		// 0: NTSC, 1: PAL
	int	size;
	int	color[4];
} logo_info_struct;

static logo_info_struct logo_info;

extern unsigned long dvr_task;	/* dvr application's PID */
extern unsigned long (*rtc_get_time)(void);
extern int venus_cpu_suspend(int board_id, int gpio, unsigned int options);

int Suspend_ret;	/* The return value of venus_cpu_suspend. If Linux is woken up by RTC alarm, it is 2; if woken up by eject event, it is 1; for the other situation, it is 0. */

extern platform_info_t platform_info;

void setup_boot_image(void)
{
	unsigned int value;

	if (platform_info.board_id == realtek_pvr_demo_board)
		return;

	// change to "composite" mode for SCART
	value = *(volatile unsigned int *)0xb801b108;
	*(volatile unsigned int *)0xb801b108 = (value & 0xffbfffff) ;
	
	// change to AV mode for SCART
	if (platform_info.board_id == realtek_1261_demo_board) {
		value = *(volatile unsigned int *)0xb801b10c;
		*(volatile unsigned int *)0xb801b10c = (value & 0xfffffff3) ;
	}

	// reset video exception entry point
	*(volatile unsigned int *)0xa00000a4 = 0x00801a3c;
	*(volatile unsigned int *)0xa00000a8 = 0x00185a37;
	*(volatile unsigned int *)0xa00000ac = 0x08004003;
	*(volatile unsigned int *)0xa00000b0 = 0x00000000;

	if (logo_info.mode == 0) {
		// NTSC mode
		// set VO registers
		*(volatile unsigned int *)0xb8005350 = 0xfffffffe ;
		*(volatile unsigned int *)0xb8005350 = 0x3 ;
		*(volatile unsigned int *)0xb8005354 = 0x2cf ;
		*(volatile unsigned int *)0xb8005358 = 0x2ef5df ;
		*(volatile unsigned int *)0xb800535c = 0xffff3210 ;
		*(volatile unsigned int *)0xb8005360 = 0x0 ;

		// color lookup table of Sub-Picture , index0~index3
		*(volatile unsigned int *)0xb8005370 = logo_info.color[0];
		*(volatile unsigned int *)0xb8005374 = logo_info.color[1];
		*(volatile unsigned int *)0xb8005378 = logo_info.color[2];
		*(volatile unsigned int *)0xb800537c = logo_info.color[3];
		*(volatile unsigned int *)0xb80053b0 = 0xacf800 ;
		*(volatile unsigned int *)0xb80053b4 = 0x9df800 ;
		*(volatile unsigned int *)0xb80053bc = 0x4000 ;
		*(volatile unsigned int *)0xb80053c0 = 0x0 ;
		*(volatile unsigned int *)0xb80053c4 = 0x0 ;
		*(volatile unsigned int *)0xb80053c8 = 0x0 ;
		*(volatile unsigned int *)0xb80053cc = 0x0 ;
		*(volatile unsigned int *)0xb80053d0 = 0x200 ;
		*(volatile unsigned int *)0xb80053d8 = 0x4000 ;
		*(volatile unsigned int *)0xb80053dc = 0x0 ;
		*(volatile unsigned int *)0xb80053e0 = 0x0 ;
		*(volatile unsigned int *)0xb80053e4 = 0x0 ;
		*(volatile unsigned int *)0xb80053e8 = 0x0 ;
		*(volatile unsigned int *)0xb80053ec = 0x0 ;
		*(volatile unsigned int *)0xb80053f0 = 0x0 ;
		*(volatile unsigned int *)0xb80053f4 = 0x0 ;
		*(volatile unsigned int *)0xb80053f8 = 0x0 ;
		*(volatile unsigned int *)0xb80053fc = 0x400 ;
		*(volatile unsigned int *)0xb8005400 = 0xffd ;
		*(volatile unsigned int *)0xb8005404 = 0xf8f ;
		*(volatile unsigned int *)0xb8005408 = 0xf60 ;
		*(volatile unsigned int *)0xb800540c = 0xf50 ;
		*(volatile unsigned int *)0xb8005410 = 0xfa8 ;
		*(volatile unsigned int *)0xb8005414 = 0x207 ;
		*(volatile unsigned int *)0xb8005418 = 0x30a ;
		*(volatile unsigned int *)0xb800541c = 0x50b ;
		*(volatile unsigned int *)0xb80054b0 = 0x9dfacf ;
		*(volatile unsigned int *)0xb80054e8 = 0x2001400 ;
		*(volatile unsigned int *)0xb80054ec = 0x2001000 ;
		*(volatile unsigned int *)0xb80054f0 = 0x2001400 ;
		*(volatile unsigned int *)0xb80054f4 = 0x2001000 ;
		*(volatile unsigned int *)0xb80054f8 = 0x400 ;
		*(volatile unsigned int *)0xb80054fc = 0x20080200 ;
		*(volatile unsigned int *)0xb8005500 = 0xf010eb10 ;
		*(volatile unsigned int *)0xb8005564 = 0x0 ;
		*(volatile unsigned int *)0xb8005568 = 0x1 ;
		*(volatile unsigned int *)0xb800556c = 0x2 ;
		*(volatile unsigned int *)0xb8005570 = 0x3 ;
		*(volatile unsigned int *)0xb8005574 = 0x4 ;
		*(volatile unsigned int *)0xb8005578 = 0x5 ;
		*(volatile unsigned int *)0xb800557c = 0x6 ;
		*(volatile unsigned int *)0xb8005580 = 0x7 ;
		*(volatile unsigned int *)0xb8005584 = 0x8 ;

		// Top and Bot address of Sub-Picture 
		*(volatile unsigned int *)0xb8005530 = 0xf0000 ;
		*(volatile unsigned int *)0xb8005534 = 0xf0000+logo_info.size;

		// set TVE registers
		*(volatile unsigned int *)0xb8000084 = 0x101cd800 ;
		*(volatile unsigned int *)0xb800000c = 0x1 ;
		*(volatile unsigned int *)0xb80180c8 = 0x124 ;
		*(volatile unsigned int *)0xb80180cc = 0x188 ;
		*(volatile unsigned int *)0xb8018134 = 0x278000 ;
		*(volatile unsigned int *)0xb80180d0 = 0x1f8bbb ;
		*(volatile unsigned int *)0xb80180d4 = 0x1f8cbb ;
		*(volatile unsigned int *)0xb80180e8 = 0x1 ;
		*(volatile unsigned int *)0xb8018154 = 0x200 ;
		*(volatile unsigned int *)0xb8018880 = 0x1f ;
		*(volatile unsigned int *)0xb8018884 = 0x7c ;
		*(volatile unsigned int *)0xb8018888 = 0xf0 ;
		*(volatile unsigned int *)0xb801888c = 0x21 ;
		*(volatile unsigned int *)0xb8018890 = 0x0 ;
		*(volatile unsigned int *)0xb8018894 = 0x2 ;
		*(volatile unsigned int *)0xb8018898 = 0x2 ;
		*(volatile unsigned int *)0xb801889c = 0x3f ;
		*(volatile unsigned int *)0xb80188a0 = 0x2 ;
		*(volatile unsigned int *)0xb80188a8 = 0x8d ;
		*(volatile unsigned int *)0xb80188ac = 0x78 ;
		*(volatile unsigned int *)0xb80188b0 = 0x10 ;
		*(volatile unsigned int *)0xb80188b4 = 0x7 ;
		*(volatile unsigned int *)0xb80188b8 = 0x1c ;
		*(volatile unsigned int *)0xb80188b8 = 0x1c ;
		*(volatile unsigned int *)0xb8018984 = 0x20 ;
		*(volatile unsigned int *)0xb801898c = 0x2 ;
		*(volatile unsigned int *)0xb80188bc = 0x0 ;
		*(volatile unsigned int *)0xb80188c8 = 0xc8 ;
		*(volatile unsigned int *)0xb80188cc = 0x0 ;
		*(volatile unsigned int *)0xb80188d0 = 0x0 ;
		*(volatile unsigned int *)0xb80188e0 = 0x8 ;
		*(volatile unsigned int *)0xb80188e4 = 0x31 ;
		*(volatile unsigned int *)0xb80188e8 = 0x6 ;
		*(volatile unsigned int *)0xb80188ec = 0x6 ;
		*(volatile unsigned int *)0xb80188f0 = 0xb3 ;
		*(volatile unsigned int *)0xb80188f4 = 0x3 ;
		*(volatile unsigned int *)0xb80188f8 = 0x59 ;
		*(volatile unsigned int *)0xb80189c0 = 0x64 ;
		*(volatile unsigned int *)0xb80189c4 = 0x2d ;
		*(volatile unsigned int *)0xb80189c8 = 0x7 ;
		*(volatile unsigned int *)0xb80189cc = 0x14 ;
		*(volatile unsigned int *)0xb8018920 = 0x0 ;
		*(volatile unsigned int *)0xb8018924 = 0x3a ;
		*(volatile unsigned int *)0xb8018928 = 0x11 ;
		*(volatile unsigned int *)0xb801892c = 0x4b ;
		*(volatile unsigned int *)0xb8018930 = 0x11 ;
		*(volatile unsigned int *)0xb8018934 = 0x3c ;
		*(volatile unsigned int *)0xb8018938 = 0x1b ;
		*(volatile unsigned int *)0xb801893c = 0x1b ;
		*(volatile unsigned int *)0xb8018940 = 0x24 ;
		*(volatile unsigned int *)0xb8018944 = 0x7 ;
		*(volatile unsigned int *)0xb8018948 = 0xf8 ;
		*(volatile unsigned int *)0xb801894c = 0x0 ;
		*(volatile unsigned int *)0xb8018950 = 0x0 ;
		*(volatile unsigned int *)0xb8018954 = 0xf ;
		*(volatile unsigned int *)0xb8018958 = 0xf ;
		*(volatile unsigned int *)0xb801895c = 0x60 ;
		*(volatile unsigned int *)0xb8018960 = 0xa0 ;
		*(volatile unsigned int *)0xb8018964 = 0x54 ;
		*(volatile unsigned int *)0xb8018968 = 0xff ;
		*(volatile unsigned int *)0xb801896c = 0x3 ;
		*(volatile unsigned int *)0xb80180d8 = 0x40 ;
		*(volatile unsigned int *)0xb8018990 = 0x10 ;
		*(volatile unsigned int *)0xb80189d0 = 0xc ;
		*(volatile unsigned int *)0xb80189d4 = 0x4b ;
		*(volatile unsigned int *)0xb80189d8 = 0x7a ;
		*(volatile unsigned int *)0xb80189dc = 0x2b ;
		*(volatile unsigned int *)0xb80189e0 = 0x85 ;
		*(volatile unsigned int *)0xb80189e4 = 0xaa ;
		*(volatile unsigned int *)0xb80189e8 = 0x5a ;
		*(volatile unsigned int *)0xb80189ec = 0x62 ;
		*(volatile unsigned int *)0xb80189f0 = 0x84 ;
		*(volatile unsigned int *)0xb8018000 = 0xae832359 ;
		*(volatile unsigned int *)0xb8018004 = 0x306505 ;
		*(volatile unsigned int *)0xb80180ac = 0xe0b59 ;
		*(volatile unsigned int *)0xb8018048 = 0x8212353 ;
		*(volatile unsigned int *)0xb8018050 = 0x815904 ;
		*(volatile unsigned int *)0xb8018054 = 0x91ca0b ;
		*(volatile unsigned int *)0xb801804c = 0x8222358 ;
		*(volatile unsigned int *)0xb8018058 = 0x82aa09 ;
		*(volatile unsigned int *)0xb80180c4 = 0x7ec00 ;
		*(volatile unsigned int *)0xb80180ec = 0x19 ;
		*(volatile unsigned int *)0xb80180ec = 0x6 ;
		*(volatile unsigned int *)0xb80180ec = 0x11 ;
		*(volatile unsigned int *)0xb80180f0 = 0x22d484 ;
		*(volatile unsigned int *)0xb8018098 = 0x6a081000 ;
		*(volatile unsigned int *)0xb801809c = 0x27e1040 ;
		*(volatile unsigned int *)0xb8018148 = 0xa56d0d0 ;
		*(volatile unsigned int *)0xb8018040 = 0x1 ;
		*(volatile unsigned int *)0xb80180d8 = 0x3fe ;
		*(volatile unsigned int *)0xb80180d8 = 0x3ab ;
		*(volatile unsigned int *)0xb8018140 = 0x3fff000 ;
		*(volatile unsigned int *)0xb8018144 = 0x3fff000 ;
		*(volatile unsigned int *)0xb80180a8 = 0xff4810 ;
		*(volatile unsigned int *)0xb80180ac = 0x8ff359 ;
		*(volatile unsigned int *)0xb8018150 = 0x4c80 ;
		*(volatile unsigned int *)0xb8018084 = 0x8000886e ;
		*(volatile unsigned int *)0xb8018088 = 0x80008000 ;
		*(volatile unsigned int *)0xb801808c = 0x8df4937c ;
		*(volatile unsigned int *)0xb8018090 = 0xa014fd50 ;
		*(volatile unsigned int *)0xb8018110 = 0x200e ;
		*(volatile unsigned int *)0xb8018094 = 0x8845805f ;
		*(volatile unsigned int *)0xb80180fc = 0x27e1804 ;
		*(volatile unsigned int *)0xb80180a4 = 0x8202352 ;
		*(volatile unsigned int *)0xb8018100 = 0x909803 ;
		*(volatile unsigned int *)0xb8018104 = 0x800816 ;
		*(volatile unsigned int *)0xb8018108 = 0x90691d ;
		*(volatile unsigned int *)0xb80180a0 = 0x7e ;
		*(volatile unsigned int *)0xb8018000 = 0x30000000 ;

		// disable TVE colorbar, enable interrupt
		*(volatile unsigned int *)0xb80180ec = 0x10 ;
		*(volatile unsigned int *)0xb8005504 = 0xfffffffe ;
		*(volatile unsigned int *)0xb8005504 = 0xb ;
		*(volatile unsigned int *)0xb801810c = 0xa00a000 ;
		*(volatile unsigned int *)0xb80180e0 = 0x0 ;
		*(volatile unsigned int *)0xb80055e4 = 0x9 ;
		*(volatile unsigned int *)0xb80180dc = 0x1 ;
	} else if (logo_info.mode == 1) {
		// PAL_I mode
		// set VO registers
		*(volatile unsigned int *)0xb8005350 = 0xfffffffe ;
		*(volatile unsigned int *)0xb8005350 = 0x3 ;
		*(volatile unsigned int *)0xb8005354 = 0x2cf ;
		*(volatile unsigned int *)0xb8005358 = 0x31f63f ;
		*(volatile unsigned int *)0xb800535c = 0xffff3210 ;
		*(volatile unsigned int *)0xb8005360 = 0x0 ;
		
		// color lookup table of Sub-Picture , index0~index3
		*(volatile unsigned int *)0xb8005370 = logo_info.color[0] ;
		*(volatile unsigned int *)0xb8005374 = logo_info.color[1] ;
		*(volatile unsigned int *)0xb8005378 = logo_info.color[2] ;
		*(volatile unsigned int *)0xb800537c = logo_info.color[3] ;
		*(volatile unsigned int *)0xb80053b0 = 0xacf800 ;
		*(volatile unsigned int *)0xb80053b4 = 0xa3f800 ;
		*(volatile unsigned int *)0xb80053bc = 0x4000 ;
		*(volatile unsigned int *)0xb80053c0 = 0x0 ;
		*(volatile unsigned int *)0xb80053c4 = 0x0 ;
		*(volatile unsigned int *)0xb80053c8 = 0x0 ;
		*(volatile unsigned int *)0xb80053cc = 0x0 ;
		*(volatile unsigned int *)0xb80053d0 = 0x200 ;
		*(volatile unsigned int *)0xb80053d8 = 0x4000 ;
		*(volatile unsigned int *)0xb80053dc = 0x0 ;
		*(volatile unsigned int *)0xb80053e0 = 0x0 ;
		*(volatile unsigned int *)0xb80053e4 = 0x0 ;
		*(volatile unsigned int *)0xb80053e8 = 0x0 ;
		*(volatile unsigned int *)0xb80053ec = 0x0 ;
		*(volatile unsigned int *)0xb80053f0 = 0x0 ;
		*(volatile unsigned int *)0xb80053f4 = 0x0 ;
		*(volatile unsigned int *)0xb80053f8 = 0x0 ;
		*(volatile unsigned int *)0xb80053fc = 0x400 ;
		*(volatile unsigned int *)0xb8005400 = 0xffd ;
		*(volatile unsigned int *)0xb8005404 = 0xf8f ;
		*(volatile unsigned int *)0xb8005408 = 0xf60 ;
		*(volatile unsigned int *)0xb800540c = 0xf50 ;
		*(volatile unsigned int *)0xb8005410 = 0xfa8 ;
		*(volatile unsigned int *)0xb8005414 = 0x207 ;
		*(volatile unsigned int *)0xb8005418 = 0x30a ;
		*(volatile unsigned int *)0xb800541c = 0x50b ;
		*(volatile unsigned int *)0xb80054b0 = 0xa3facf ;
		*(volatile unsigned int *)0xb80054b4 = 0x16030100 ;
		*(volatile unsigned int *)0xb80054e8 = 0x2001400 ;
		*(volatile unsigned int *)0xb80054ec = 0x2001000 ;
		*(volatile unsigned int *)0xb80054f0 = 0x2001400 ;
		*(volatile unsigned int *)0xb80054f4 = 0x2001000 ;
		*(volatile unsigned int *)0xb80054f8 = 0x400 ;
		*(volatile unsigned int *)0xb80054fc = 0x20080200 ;
		*(volatile unsigned int *)0xb8005500 = 0xf010eb10 ;
		*(volatile unsigned int *)0xb8005564 = 0x0 ;
		*(volatile unsigned int *)0xb8005568 = 0x1 ;
		*(volatile unsigned int *)0xb800556c = 0x2 ;
		*(volatile unsigned int *)0xb8005570 = 0x3 ;
		*(volatile unsigned int *)0xb8005574 = 0x4 ;
		*(volatile unsigned int *)0xb8005578 = 0x5 ;
		*(volatile unsigned int *)0xb800557c = 0x6 ;
		*(volatile unsigned int *)0xb8005580 = 0x7 ;
		*(volatile unsigned int *)0xb8005584 = 0x8 ;
		
		// Top and Bot address of Sub-Picture 
		*(volatile unsigned int *)0xb8005530 = 0x000f0000 ;
		*(volatile unsigned int *)0xb8005534 = 0x000f0000+logo_info.size ;
		
		// set TVE registers
		*(volatile unsigned int *)0xb8000084 = 0x101cd800 ;
		*(volatile unsigned int *)0xb800000c = 0x1 ;
		*(volatile unsigned int *)0xb80180c8 = 0x124 ;
		*(volatile unsigned int *)0xb80180cc = 0x188 ;
		*(volatile unsigned int *)0xb8018134 = 0x278000 ;
		*(volatile unsigned int *)0xb80180d0 = 0x1f8bbb ;
		*(volatile unsigned int *)0xb80180d4 = 0x1f8cbb ;
		*(volatile unsigned int *)0xb80180e8 = 0x1 ;
		*(volatile unsigned int *)0xb8018154 = 0x205 ;
		*(volatile unsigned int *)0xb8018880 = 0xcb ;
		*(volatile unsigned int *)0xb8018884 = 0x8a ;
		*(volatile unsigned int *)0xb8018888 = 0x9 ;
		*(volatile unsigned int *)0xb801888c = 0x2a ;
		*(volatile unsigned int *)0xb8018890 = 0x0 ;
		*(volatile unsigned int *)0xb8018894 = 0x0 ;
		*(volatile unsigned int *)0xb8018898 = 0x0 ;
		*(volatile unsigned int *)0xb801889c = 0x9b ;
		*(volatile unsigned int *)0xb80188a0 = 0x2 ;
		*(volatile unsigned int *)0xb80188a8 = 0x78 ;
		*(volatile unsigned int *)0xb80188ac = 0x78 ;
		*(volatile unsigned int *)0xb80188b0 = 0x10 ;
		*(volatile unsigned int *)0xb80188b4 = 0x3 ;
		*(volatile unsigned int *)0xb80188b8 = 0x1d ;
		*(volatile unsigned int *)0xb80188b8 = 0x1d ;
		*(volatile unsigned int *)0xb8018984 = 0x20 ;
		*(volatile unsigned int *)0xb801898c = 0x2 ;
		*(volatile unsigned int *)0xb80188bc = 0x0 ;
		*(volatile unsigned int *)0xb80188c8 = 0xd7 ;
		*(volatile unsigned int *)0xb80188cc = 0x29 ;
		*(volatile unsigned int *)0xb80188d0 = 0x3 ;
		*(volatile unsigned int *)0xb80188e0 = 0x9 ;
		*(volatile unsigned int *)0xb80188e4 = 0x31 ;
		*(volatile unsigned int *)0xb80188e8 = 0x38 ;
		*(volatile unsigned int *)0xb80188ec = 0x6 ;
		*(volatile unsigned int *)0xb80188f0 = 0xbf ;
		*(volatile unsigned int *)0xb80188f4 = 0x3 ;
		*(volatile unsigned int *)0xb80188f8 = 0x5f ;
		*(volatile unsigned int *)0xb80189c0 = 0x5c ;
		*(volatile unsigned int *)0xb80189c4 = 0x40 ;
		*(volatile unsigned int *)0xb80189c8 = 0x24 ;
		*(volatile unsigned int *)0xb80189cc = 0x1c ;
		*(volatile unsigned int *)0xb8018920 = 0x0 ;
		*(volatile unsigned int *)0xb8018924 = 0x39 ;
		*(volatile unsigned int *)0xb8018928 = 0x22 ;
		*(volatile unsigned int *)0xb801892c = 0x5a ;
		*(volatile unsigned int *)0xb8018930 = 0x22 ;
		*(volatile unsigned int *)0xb8018934 = 0xa8 ;
		*(volatile unsigned int *)0xb8018938 = 0x1c ;
		*(volatile unsigned int *)0xb801893c = 0x34 ;
		*(volatile unsigned int *)0xb8018940 = 0x14 ;
		*(volatile unsigned int *)0xb8018944 = 0x3 ;
		*(volatile unsigned int *)0xb8018948 = 0xfe ;
		*(volatile unsigned int *)0xb801894c = 0x1 ;
		*(volatile unsigned int *)0xb8018950 = 0x54 ;
		*(volatile unsigned int *)0xb8018954 = 0xfe ;
		*(volatile unsigned int *)0xb8018958 = 0x7e ;
		*(volatile unsigned int *)0xb801895c = 0x60 ;
		*(volatile unsigned int *)0xb8018960 = 0x80 ;
		*(volatile unsigned int *)0xb8018964 = 0x47 ;
		*(volatile unsigned int *)0xb8018968 = 0x55 ;
		*(volatile unsigned int *)0xb801896c = 0x1 ;
		*(volatile unsigned int *)0xb80180d8 = 0x40 ;
		*(volatile unsigned int *)0xb8018990 = 0x0 ;
		*(volatile unsigned int *)0xb80189d0 = 0x0 ;
		*(volatile unsigned int *)0xb80189d4 = 0x3f ;
		*(volatile unsigned int *)0xb80189d8 = 0x71 ;
		*(volatile unsigned int *)0xb80189dc = 0x20 ;
		*(volatile unsigned int *)0xb80189e0 = 0x80 ;
		*(volatile unsigned int *)0xb80189e4 = 0xa4 ;
		*(volatile unsigned int *)0xb80189e8 = 0x50 ;
		*(volatile unsigned int *)0xb80189ec = 0x57 ;
		*(volatile unsigned int *)0xb80189f0 = 0x75 ;
		*(volatile unsigned int *)0xb8018000 = 0x2e9c235f ;
		*(volatile unsigned int *)0xb8018004 = 0x29ae6d ;
		*(volatile unsigned int *)0xb80180ac = 0xe0b1b ;
		*(volatile unsigned int *)0xb8018048 = 0x80fa30d ;
		*(volatile unsigned int *)0xb8018050 = 0x816935 ;
		*(volatile unsigned int *)0xb8018054 = 0x94fa6e ;
		*(volatile unsigned int *)0xb801804c = 0x81f234c ;
		*(volatile unsigned int *)0xb8018058 = 0x82ca6b ;
		*(volatile unsigned int *)0xb80180c4 = 0x7ec00 ;
		*(volatile unsigned int *)0xb80180ec = 0x19 ;
		*(volatile unsigned int *)0xb80180ec = 0x6 ;
		*(volatile unsigned int *)0xb80180ec = 0x11 ;
		*(volatile unsigned int *)0xb80180f0 = 0x22d43c ;
		*(volatile unsigned int *)0xb8018098 = 0x6a081000 ;
		*(volatile unsigned int *)0xb801809c = 0x27e1bb0 ;
		*(volatile unsigned int *)0xb8018148 = 0xa56d0d0 ;
		*(volatile unsigned int *)0xb8018040 = 0x1 ;
		*(volatile unsigned int *)0xb80180d8 = 0x3fe ;
		*(volatile unsigned int *)0xb80180d8 = 0x3ab ;
		*(volatile unsigned int *)0xb8018140 = 0x3fff000 ;
		*(volatile unsigned int *)0xb8018144 = 0x3fff000 ;
		*(volatile unsigned int *)0xb8018150 = 0x5c80 ;
		*(volatile unsigned int *)0xb8018084 = 0x80008807 ;
		*(volatile unsigned int *)0xb8018088 = 0x80008000 ;
		*(volatile unsigned int *)0xb801808c = 0x8d0c9266 ;
		*(volatile unsigned int *)0xb8018090 = 0xa0038110 ;
		*(volatile unsigned int *)0xb8018110 = 0x2003 ;
		*(volatile unsigned int *)0xb8018094 = 0x88008112 ;
		*(volatile unsigned int *)0xb80180fc = 0x27e1800 ;
		*(volatile unsigned int *)0xb80180a4 = 0x821a358 ;
		*(volatile unsigned int *)0xb8018100 = 0x938800 ;
		*(volatile unsigned int *)0xb8018104 = 0xa6f816 ;
		*(volatile unsigned int *)0xb8018108 = 0x93694f ;
		*(volatile unsigned int *)0xb80180a0 = 0x7e ;
		*(volatile unsigned int *)0xb8018000 = 0x30000000 ;
		
		// disable TVE colorbar, enable interrupt
		*(volatile unsigned int *)0xb80180ec = 0x10 ;
		*(volatile unsigned int *)0xb8005504 = 0xfffffffe ;
		*(volatile unsigned int *)0xb8005504 = 0xb ;
		*(volatile unsigned int *)0xb801810c = 0xa00a000 ;
		*(volatile unsigned int *)0xb80180e0 = 0x0 ;
		*(volatile unsigned int *)0xb80055f0 = 0x9 ;
		
		*(volatile unsigned int *)0xb80180dc = 0x1 ;
	} else {
		// PAL_M mode
		// set VO registers
		*(volatile unsigned int *)0xb8005350 = 0xfffffffe ;
		*(volatile unsigned int *)0xb8005350 = 0x3 ;
		*(volatile unsigned int *)0xb8005354 = 0x2cf ;
		*(volatile unsigned int *)0xb8005358 = 0x2ef5df ;
		*(volatile unsigned int *)0xb800535c = 0xffff3210 ;
		*(volatile unsigned int *)0xb8005360 = 0x0 ;

		// color lookup table of Sub-Picture , index0~index3
		*(volatile unsigned int *)0xb8005370 = logo_info.color[0] ;
		*(volatile unsigned int *)0xb8005374 = logo_info.color[1] ;
		*(volatile unsigned int *)0xb8005378 = logo_info.color[2] ;
		*(volatile unsigned int *)0xb800537c = logo_info.color[3] ;
		*(volatile unsigned int *)0xb80053b0 = 0xacf800 ;
		*(volatile unsigned int *)0xb80053b4 = 0x9df800 ;
		*(volatile unsigned int *)0xb80053bc = 0x4000 ;
		*(volatile unsigned int *)0xb80053c0 = 0x0 ;
		*(volatile unsigned int *)0xb80053c4 = 0x0 ;
		*(volatile unsigned int *)0xb80053c8 = 0x0 ;
		*(volatile unsigned int *)0xb80053cc = 0x0 ;
		*(volatile unsigned int *)0xb80053d0 = 0x200 ;
		*(volatile unsigned int *)0xb80053d8 = 0x4000 ;
		*(volatile unsigned int *)0xb80053dc = 0x0 ;
		*(volatile unsigned int *)0xb80053e0 = 0x0 ;
		*(volatile unsigned int *)0xb80053e4 = 0x0 ;
		*(volatile unsigned int *)0xb80053e8 = 0x0 ;
		*(volatile unsigned int *)0xb80053ec = 0x0 ;
		*(volatile unsigned int *)0xb80053f0 = 0x0 ;
		*(volatile unsigned int *)0xb80053f4 = 0x0 ;
		*(volatile unsigned int *)0xb80053f8 = 0x0 ;
		*(volatile unsigned int *)0xb80053fc = 0x400 ;
		*(volatile unsigned int *)0xb8005400 = 0xffd ;
		*(volatile unsigned int *)0xb8005404 = 0xf8f ;
		*(volatile unsigned int *)0xb8005408 = 0xf60 ;
		*(volatile unsigned int *)0xb800540c = 0xf50 ;
		*(volatile unsigned int *)0xb8005410 = 0xfa8 ;
		*(volatile unsigned int *)0xb8005414 = 0x207 ;
		*(volatile unsigned int *)0xb8005418 = 0x30a ;
		*(volatile unsigned int *)0xb800541c = 0x50b ;
		*(volatile unsigned int *)0xb80054b0 = 0x9dfacf ;
		*(volatile unsigned int *)0xb80054e8 = 0x2001400 ;
		*(volatile unsigned int *)0xb80054ec = 0x2001000 ;
		*(volatile unsigned int *)0xb80054f0 = 0x2001400 ;
		*(volatile unsigned int *)0xb80054f4 = 0x2001000 ;
		*(volatile unsigned int *)0xb80054f8 = 0x400 ;
		*(volatile unsigned int *)0xb80054fc = 0x20080200 ;
		*(volatile unsigned int *)0xb8005500 = 0xf010eb10 ;
		*(volatile unsigned int *)0xb8005564 = 0x0 ;
		*(volatile unsigned int *)0xb8005568 = 0x1 ;
		*(volatile unsigned int *)0xb800556c = 0x2 ;
		*(volatile unsigned int *)0xb8005570 = 0x3 ;
		*(volatile unsigned int *)0xb8005574 = 0x4 ;
		*(volatile unsigned int *)0xb8005578 = 0x5 ;
		*(volatile unsigned int *)0xb800557c = 0x6 ;
		*(volatile unsigned int *)0xb8005580 = 0x7 ;
		*(volatile unsigned int *)0xb8005584 = 0x8 ;

		// Top and Bot address of Sub-Picture 
		*(volatile unsigned int *)0xb8005530 = 0x000f0000 ;
		*(volatile unsigned int *)0xb8005534 = 0x000f0000+logo_info.size ;

		// set TVE registers
		*(volatile unsigned int *)0xb8000084 = 0x101cd800 ;
		*(volatile unsigned int *)0xb800000c = 0x1 ;
		*(volatile unsigned int *)0xb80180c8 = 0x124 ;
		*(volatile unsigned int *)0xb80180cc = 0x188 ;
		*(volatile unsigned int *)0xb8018134 = 0x3f8000 ;
		*(volatile unsigned int *)0xb80180d0 = 0x1f9aaa ;
		*(volatile unsigned int *)0xb80180d4 = 0x1f9aaa ;
		*(volatile unsigned int *)0xb80180e8 = 0x1 ;
		*(volatile unsigned int *)0xb8018154 = 0x200 ;
		*(volatile unsigned int *)0xb8018880 = 0xa3 ;
		*(volatile unsigned int *)0xb8018884 = 0xef ;
		*(volatile unsigned int *)0xb8018888 = 0xe6 ;
		*(volatile unsigned int *)0xb801888c = 0x21 ;
		*(volatile unsigned int *)0xb8018890 = 0x0 ;
		*(volatile unsigned int *)0xb8018894 = 0x0 ;
		*(volatile unsigned int *)0xb8018898 = 0x0 ;
		*(volatile unsigned int *)0xb801889c = 0x0 ;
		*(volatile unsigned int *)0xb80188a0 = 0x0 ;
		*(volatile unsigned int *)0xb80188a8 = 0x8d ;
		*(volatile unsigned int *)0xb80188ac = 0x78 ;
		*(volatile unsigned int *)0xb80188b0 = 0x10 ;
		*(volatile unsigned int *)0xb80188b4 = 0x7 ;
		*(volatile unsigned int *)0xb80188b8 = 0x1d ;
		*(volatile unsigned int *)0xb80188b8 = 0x1d ;
		*(volatile unsigned int *)0xb8018984 = 0x20 ;
		*(volatile unsigned int *)0xb801898c = 0x2 ;
		*(volatile unsigned int *)0xb80188bc = 0x0 ;
		*(volatile unsigned int *)0xb80188c8 = 0xd7 ;
		*(volatile unsigned int *)0xb80188cc = 0x29 ;
		*(volatile unsigned int *)0xb80188d0 = 0x3 ;
		*(volatile unsigned int *)0xb80188e0 = 0xa ;
		*(volatile unsigned int *)0xb80188e4 = 0x31 ;
		*(volatile unsigned int *)0xb80188e8 = 0x6 ;
		*(volatile unsigned int *)0xb80188ec = 0x6 ;
		*(volatile unsigned int *)0xb80188f0 = 0xb3 ;
		*(volatile unsigned int *)0xb80188f4 = 0x3 ;
		*(volatile unsigned int *)0xb80188f8 = 0x58 ;
		*(volatile unsigned int *)0xb80189c0 = 0x64 ;
		*(volatile unsigned int *)0xb80189c4 = 0x2d ;
		*(volatile unsigned int *)0xb80189c8 = 0x7 ;
		*(volatile unsigned int *)0xb80189cc = 0x18 ;
		*(volatile unsigned int *)0xb8018920 = 0x0 ;
		*(volatile unsigned int *)0xb8018924 = 0x3a ;
		*(volatile unsigned int *)0xb8018928 = 0x11 ;
		*(volatile unsigned int *)0xb801892c = 0x4b ;
		*(volatile unsigned int *)0xb8018930 = 0x11 ;
		*(volatile unsigned int *)0xb8018934 = 0x3c ;
		*(volatile unsigned int *)0xb8018938 = 0x1b ;
		*(volatile unsigned int *)0xb801893c = 0x1b ;
		*(volatile unsigned int *)0xb8018940 = 0x24 ;
		*(volatile unsigned int *)0xb8018944 = 0x7 ;
		*(volatile unsigned int *)0xb8018948 = 0xf8 ;
		*(volatile unsigned int *)0xb801894c = 0x0 ;
		*(volatile unsigned int *)0xb8018950 = 0x0 ;
		*(volatile unsigned int *)0xb8018954 = 0xf ;
		*(volatile unsigned int *)0xb8018958 = 0xf ;
		*(volatile unsigned int *)0xb801895c = 0x60 ;
		*(volatile unsigned int *)0xb8018960 = 0xa0 ;
		*(volatile unsigned int *)0xb8018964 = 0x54 ;
		*(volatile unsigned int *)0xb8018968 = 0xff ;
		*(volatile unsigned int *)0xb801896c = 0x3 ;
		*(volatile unsigned int *)0xb80180d8 = 0x40 ;
		*(volatile unsigned int *)0xb8018990 = 0x0 ;
		*(volatile unsigned int *)0xb80189d0 = 0xc ;
		*(volatile unsigned int *)0xb80189d4 = 0x4b ;
		*(volatile unsigned int *)0xb80189d8 = 0x7a ;
		*(volatile unsigned int *)0xb80189dc = 0x2b ;
		*(volatile unsigned int *)0xb80189e0 = 0x85 ;
		*(volatile unsigned int *)0xb80189e4 = 0xaa ;
		*(volatile unsigned int *)0xb80189e8 = 0x5a ;
		*(volatile unsigned int *)0xb80189ec = 0x62 ;
		*(volatile unsigned int *)0xb80189f0 = 0x84 ;
		*(volatile unsigned int *)0xb8018000 = 0x2e832359 ;
		*(volatile unsigned int *)0xb8018004 = 0x306505 ;
		*(volatile unsigned int *)0xb80180ac = 0xe0b59 ;
		*(volatile unsigned int *)0xb8018048 = 0x8212353 ;
		*(volatile unsigned int *)0xb8018050 = 0x815904 ;
		*(volatile unsigned int *)0xb8018054 = 0x91ca0b ;
		*(volatile unsigned int *)0xb801804c = 0x8222358 ;
		*(volatile unsigned int *)0xb8018058 = 0x82aa09 ;
		*(volatile unsigned int *)0xb80180c4 = 0x76800 ;
		*(volatile unsigned int *)0xb80180ec = 0x19 ;
		*(volatile unsigned int *)0xb80180ec = 0x6 ;
		*(volatile unsigned int *)0xb80180ec = 0x11 ;
		*(volatile unsigned int *)0xb80180f0 = 0x22d484 ;
		*(volatile unsigned int *)0xb80180d8 = 0x3fe ;
		*(volatile unsigned int *)0xb80180d8 = 0x3ab ;
		*(volatile unsigned int *)0xb8018084 = 0x80008800 ;
		*(volatile unsigned int *)0xb8018088 = 0x80008000 ;
		*(volatile unsigned int *)0xb801808c = 0x937c937c ;
		*(volatile unsigned int *)0xb8018090 = 0xa0038380 ;
		*(volatile unsigned int *)0xb8018110 = 0x2003 ;
		*(volatile unsigned int *)0xb8018094 = 0x88008380 ;
		*(volatile unsigned int *)0xb8018098 = 0x6a081000 ;
		*(volatile unsigned int *)0xb801809c = 0x2081bb0 ;
		*(volatile unsigned int *)0xb80180fc = 0x2781800 ;
		*(volatile unsigned int *)0xb80180a0 = 0x3e ;
		*(volatile unsigned int *)0xb80180a0 = 0x35 ;
		*(volatile unsigned int *)0xb80180a4 = 0x822e001 ;
		*(volatile unsigned int *)0xb8018100 = 0x909803 ;
		*(volatile unsigned int *)0xb8018104 = 0x800816 ;
		*(volatile unsigned int *)0xb8018108 = 0x90691d ;
		*(volatile unsigned int *)0xb8018000 = 0x30000000 ;

		// disable TVE colorbar, enable interrupt
		*(volatile unsigned int *)0xb80180ec = 0x10 ;
		*(volatile unsigned int *)0xb8005504 = 0xfffffffe ;
		*(volatile unsigned int *)0xb8005504 = 0xb ;
		*(volatile unsigned int *)0xb801810c = 0xa00a000 ;
		*(volatile unsigned int *)0xb80180e0 = 0x0 ;
		*(volatile unsigned int *)0xb80055e4 = 0x9 ;
		*(volatile unsigned int *)0xb80180dc = 0x1 ;
	}
}

/*
 *	venus_pm_enter - Actually enter a sleep state.
 *	@state:		State we're entering.
 *
 */
static int venus_pm_enter(suspend_state_t state)
{
	int gpio;
	int value = 0;
	char *value_ptr, *value_end_ptr;
	unsigned int options=0;

	switch (state)
	{
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
		value = inl(VENUS_MIS_TC2CR);
		value |= 0x20000000;
		outl(value, VENUS_MIS_TC2CR);		/* Disable external timer interrupt */
#endif
		value = inl(VENUS_MIS_RTCCR);
		value |= 0x1;
		outl(value, VENUS_MIS_RTCCR);		/* Enable RTC's half-second interrupt */
/* Here are 2 methods for power saving. One is polling that is to avoid Venus's bug of edge trigger interrupt in power saving mode. The other is interrupt mode. */
/* In fact, IrDA is still running in polling mode when using interrupt mode to wake up system. */
#ifdef CONFIG_PM_SLEEP_POLLING
/* We will here close RTC's alarm interrupt that is useless in polling mode */
		value = inl(VENUS_MIS_RTCCR);
		if(value&0x10) {
			value &= ~0x10;
			outl(value, VENUS_MIS_RTCCR);		/* RTC alarm should be checked by polling */
		} else {
			outl(0, VENUS_MIS_ALMMIN);
			outl(0, VENUS_MIS_ALMHR);
			outl(0, VENUS_MIS_ALMDATE1);
			outl(0, VENUS_MIS_ALMDATE2);
		}
#endif
		outl(0x7FE20, VENUS_MIS_ISR);		/* Clear VFD, IrDA, and all RTC's interrupts */
		while(inl(VENUS_MIS_IR_SR) & 0x01)
			inl(VENUS_MIS_IR_RP);
		value_ptr = parse_token(platform_info.system_parameters, "12V5V_GPIO");
		if(value_ptr && strlen(value_ptr)) {
			gpio = (int)simple_strtol(value_ptr, &value_end_ptr, 10);
			if(value_ptr == value_end_ptr) {
				prom_printf("The value of 12V5V_GPIO seems to have problem.\n");
				gpio = -1;
			}
			if(!strcmp(value_end_ptr, ",hion"))
				gpio |= 0x10000000;
			else if(strcmp(value_end_ptr, ",hioff")) {
				prom_printf("The value of 12V5V_GPIO doesn't have a sub-parameter of \"hion\" or \"hioff\"\n");
				gpio = -1;
			}
		} else {
			gpio = -1;
			prom_printf("No 12V5V_GPIO setting in system_parameters. Use board_id instead.\n");
		}
		if(value_ptr)
			kfree(value_ptr);
/* For Venus CPU, if bit0 of 0xb8008800 is equal to 1, it represents that the package of the chip is QFP, otherwise it is BGA. */
		if((platform_info.cpu_id == realtek_venus_cpu || platform_info.cpu_id == realtek_venus2_cpu) && ((*(volatile unsigned int *)0xb8008800) & 0x1))
			;
		else
			options |= 0x1;
		Suspend_ret = venus_cpu_suspend(platform_info.board_id, gpio, options);
		value = inl(VENUS_MIS_RTCCR);
		value &= ~0x1;
		outl(value, VENUS_MIS_RTCCR);
		outl(0x7FE20, VENUS_MIS_ISR);
#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
		value = inl(VENUS_MIS_TC2CR);
		value &= ~0x20000000;
		outl(value, VENUS_MIS_TC2CR);		/* Enable external timer interrupt */
#endif
#if 1
		// This is to check RTC time and Alarm time when schedule record is not waken up at right time
		prom_printf("\nRTC time: %x %x %x %x %x\nAlarm time: %x %x %x %x\n", 
			inl(VENUS_MIS_RTCDATE2), inl(VENUS_MIS_RTCDATE1), 
			inl(VENUS_MIS_RTCHR), inl(VENUS_MIS_RTCMIN), 
			inl(VENUS_MIS_RTCSEC), 
			inl(VENUS_MIS_ALMDATE2), inl(VENUS_MIS_ALMDATE1), 
			inl(VENUS_MIS_ALMHR), inl(VENUS_MIS_ALMMIN));
#endif
		break;
	case PM_SUSPEND_DISK:
		return -ENOTSUPP;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 *	venus_pm_prepare - Do preliminary suspend work.
 *	@state:		suspend state we're entering.
 *
 */
static int venus_pm_prepare(suspend_state_t state)
{
	switch (state)
	{
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		break;

	case PM_SUSPEND_DISK:
		return -ENOTSUPP;

	default:
		return -EINVAL;
	}
	return 0;
}

/**
 *	venus_pm_finish - Finish up suspend sequence.
 *	@state:		State we're coming out of.
 *
 *	This is called after we wake back up (or if entering the sleep state
 *	failed).
 */
static int venus_pm_finish(suspend_state_t state)
{
	struct timespec tv;
	switch (state)
	{
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		tv.tv_sec = rtc_get_time();
		tv.tv_nsec = 0;
		do_settimeofday(&tv);
		if (dvr_task != 0) {
			if(Suspend_ret==2)
				send_sig_info(SIGUSR_RTC_ALARM, (void *)2, (struct task_struct *)dvr_task);
			else if(Suspend_ret==1)
				send_sig_info(SIGUSR2, (void *)2, (struct task_struct *)dvr_task);
			else
				send_sig_info(SIGUSR1, (void *)2, (struct task_struct *)dvr_task);
		} else {
			printk("error condition, wrong dvr_task value...\n");
		} 

		break;

	case PM_SUSPEND_DISK:
		return -ENOTSUPP;

	default:
		return -EINVAL;
	}
	return 0;
}

/*
 * Set to PM_DISK_FIRMWARE so we can quickly veto suspend-to-disk.
 */
static struct pm_ops venus_pm_ops = {
	.pm_disk_mode	= PM_DISK_FIRMWARE,
	.prepare	= venus_pm_prepare,
	.enter		= venus_pm_enter,
	.finish		= venus_pm_finish,
};

static int __init venus_pm_init(void)
{
	char *strptr;
	int count = LOGO_INFO_SIZE;

	printk("Realtek Venus Power Management, (c) 2006 Realtek Semiconductor Corp.\n");
	printk("========== board id: %x ==========\n", platform_info.board_id);

/*
	if ((platform_info.board_id == realtek_avhdd_demo_board) || (platform_info.board_id == C02_avhdd_board)
	 || (platform_info.board_id == realtek_pvrbox_demo_board) || (platform_info.board_id == C05_pvrbox_board))
		strptr = (char *)LOGO_INFO_ADDR2;
	else
		strptr = (char *)LOGO_INFO_ADDR1;
*/
	switch(platform_info.board_id) {
	case realtek_avhdd_demo_board:
	case realtek_avhdd2_demo_board:
	case C02_avhdd_board:
	case realtek_pvrbox_demo_board:
	case C05_pvrbox_board:
	case C05_pvrbox2_board:
	case C07_avhdd_board:
	case C07_pvrbox_board:
	case C07_pvrbox2_board:
	case C09_pvrbox_board:
	case C09_pvrbox2_board:
	case C03_pvr2_board:
		strptr = (char *)LOGO_INFO_ADDR2;
		break;
	default:
		strptr = (char *)LOGO_INFO_ADDR1;
		break;
	}

        while (--count > 0) {
                if (!memcmp(strptr, "-l", 2))
			break;
		strptr++;
        }
//	printk("strptr value: %s \n", strptr);
	if (count != 0) {
		strptr += 3;
		sscanf(strptr, "%d %d %x %x %x %x", &logo_info.mode, &logo_info.size,
		&logo_info.color[0], &logo_info.color[1], &logo_info.color[2], &logo_info.color[3]);
	} else {
		printk("[INFO] logo info not found, use PAL as default...\n");
		logo_info.mode = 1;	// PAL
		logo_info.size = 2724;
		logo_info.color[0] = 0x6ba53f;
		logo_info.color[1] = 0x6da555;
		logo_info.color[2] = 0x749889;
		logo_info.color[3] = 0x8080eb;
	}
	printk("mode: %d \n", logo_info.mode);
	printk("size: %d \n", logo_info.size);
	printk("color1: 0x%x \n", logo_info.color[0]);
	printk("color2: 0x%x \n", logo_info.color[1]);
	printk("color3: 0x%x \n", logo_info.color[2]);
	printk("color4: 0x%x \n", logo_info.color[3]);

	pm_set_ops(&venus_pm_ops);
	return 0;
}

late_initcall(venus_pm_init);
