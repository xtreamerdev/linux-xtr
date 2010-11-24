/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
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
 * Defines of the Venus board specific address-MAP, registers, etc.
 *
 */
#ifndef _MACH_MARS_H
#define _MACH_MARS_H

#include <asm/addrspace.h>
#include <venus.h>


/*-------------------------------------------------------------
 * Mars CRT
 *-------------------------------------------------------------*/
#define MARS_CRT_BASE           0xB8000000
#define MARS_CRT_PCICK_SEL      0x0338


/*-------------------------------------------------------------
 * Mars MISC 
 *-------------------------------------------------------------*/

#define MARS_MIS_BASE           0xB8010000

#define MARS_MIS_ISR            VENUS_MIS_ISR
#define MARS_PCMCIA_INT         (0x00000001 <<21)

#define MARS_0_SYS_MUXPAD0	    0xB8000350
#define MARS_0_SYS_MUXPAD1	    0xB8000354
#define MARS_0_SYS_MUXPAD2	    0xB8000358
#define MARS_0_SYS_MUXPAD3	    0xB800035C
#define MARS_0_SYS_MUXPAD4	    0xB8000360
#define MARS_0_SYS_MUXPAD5	    0xB8000364
#define MARS_0_SYS_MUXPAD6	    0xB8000368
#define MARS_0_SYS_MUXPAD7	    0xB800036C
/*
 * Venus GPIO registers.
 */
#define MARS_MIS_GP0DIR			0xB100
#define MARS_MIS_GP1DIR			0xB104
#define MARS_MIS_GP2DIR			0xB108
#define MARS_MIS_GP3DIR			0xB10C
#define MARS_MIS_GP0DATO		0xB110
#define MARS_MIS_GP1DATO		0xB114
#define MARS_MIS_GP2DATO		0xB118
#define MARS_MIS_GP3DATO		0xB11C
#define MARS_MIS_GP0DATI		0xB120
#define MARS_MIS_GP1DATI		0xB124
#define MARS_MIS_GP2DATI		0xB128
#define MARS_MIS_GP3DATI		0xB12C
#define MARS_MIS_GP0IE			0xB130
#define MARS_MIS_GP1IE			0xB134
#define MARS_MIS_GP2IE			0xB138
#define MARS_MIS_GP3IE			0xB13C
#define MARS_MIS_GP0DP			0xB140
#define MARS_MIS_GP1DP			0xB144
#define MARS_MIS_GP2DP			0xB148
#define MARS_MIS_GP3DP			0xB14C
#define MARS_MIS_GPDEB			0xB150

/*-------------------------------------------------------------
 * Mars MISC UART register
 *-------------------------------------------------------------*/
#define MARS_MIS_U0RBR_THR_DLL  0xB200

/*-------------------------------------------------------------
 * Mars MISC PCMCIA register
 *-------------------------------------------------------------*/
#define MARS_PCMCIA_CMDFF       0xB800
#define MARS_PCMCIA_CTRL        0xB804
#define MARS_PCMCIA_STS         0xB808
#define MARS_PCMCIA_AMTC        0xB80C
#define MARS_PCMCIA_IOMTC       0xB810
#define MARS_PCMCIA_MATC        0xB814
#define MARS_PCMCIA_ACTRL       0xB818


//-- MARS_PCMCIA_CMDFF
#define PC_CT(x)                 (((x) & 0x01)<< 24)
#define PC_CT_READ               PC_CT(0)
#define PC_CT_WRITE              PC_CT(1)
#define PC_AT(x)                 (((x) & 0x01)<< 23)
#define PC_AT_IO                 PC_AT(0)
#define PC_AT_ATTRMEM            PC_AT(1)
#define PC_PA(x)                 (((x) & 0x7FFF) << 8)
#define PC_DF(x)                 (((x) & 0xFF))

//-- MARS_PCMCIA_CTRL
#define PC_PSR                   (0x00000001 << 31)
#define PC_CE1_CARD1             (0x00000001 << 30)
#define PC_CE1_CARD2             (0x00000001 << 29)
#define PC_CE2_CARD1             (0x00000001 << 28)
#define PC_CE2_CARD2             (0x00000001 << 27)
#define PC_PCR1                  (0x00000001 << 26)
#define PC_PCR2                  (0x00000001 << 25)
#define PC_PCR1_OE               (0x00000001 << 24)
#define PC_PCR2_OE               (0x00000001 << 23)
#define PC_PIIE1                 (0x00000001 << 7)
#define PC_PCIIE1                (0x00000001 << 6)
#define PC_PCRIE1                (0x00000001 << 5)
#define PC_PIIE2                 (0x00000001 << 4)
#define PC_PCIIE2                (0x00000001 << 3)
#define PC_PCRIE2                (0x00000001 << 2)
#define PC_AFIE                  (0x00000001 << 1)
#define PC_APFIE                 (0x00000001)

//-- MARS_PCMCIA_STS
#define PC_PRES1                 (0x00000001 << 9)
#define PC_PRES2                 (0x00000001 << 8)
#define PC_PII1                  (0x00000001 << 7)
#define PC_PCII1                 (0x00000001 << 6)
#define PC_PCRI1                 (0x00000001 << 5)
#define PC_PII2                  (0x00000001 << 4)
#define PC_PCII2                 (0x00000001 << 3)
#define PC_PCRI2                 (0x00000001 << 2)
#define PC_AFI                   (0x00000001 << 1)
#define PC_APFI                  (0x00000001)

//-- MARS_PCMCIA_AMTC
#define PC_TWE(x)                (((x) & 0x3F) << 26)
#define PC_THD(x)                (((x) & 0x0F) << 22)
#define PC_TAOE(x)               (((x) & 0x1F) << 17)
#define PC_THCE(x)               (((x) & 0x1F) << 8)
#define PC_TSU(x)                (((x) & 0x1F))

//-- MARS_PCMCIA_IOMTC
#define PC_TDIORD(x)             (((x) & 0x1F) << 16)
#define PC_TSUIO(x)              (((x) & 0x0F) << 8)
#define PC_TDINPACK(x)           (((x) & 0x0F) << 4)
#define PC_TDWT(x)               (((x) & 0x0F))
 
//-- MARS_PCMCIA_MATC 
#define PC_TC(x)                 (((x) & 0xFF) << 24)
#define PC_THDIO(x)              (((x) & 0x0F) << 20)
#define PC_TCIO(x)               (((x) & 0xFF) << 8)
#define PC_TWIOWR(x)             (((x) & 0x3F))

//-- MARS_PCMCIA_MATC 
#define PC_OE(x)                 (((x) & 0x7FFF))


// USB Host Registers
//#define VENUS_USB_HOST_BASE			(VENUS_USB_BASE+0x800)
#define MARS_USB_HOST_VERSION			(VENUS_USB_HOST_BASE+0x14)
#define MARS_USB_HOST_WRAPP_2PORT		(VENUS_USB_HOST_BASE+0x20)
#define MARS_USB_HOST_VSTATUS_2PORT		(VENUS_USB_HOST_BASE+0x24)
#define MARS_USB_HOST_USBIPINPUT_2PORT		(VENUS_USB_HOST_BASE+0x28)
#define MARS_USB_HOST_RESET_UTMI_2PORT		(VENUS_USB_HOST_BASE+0x2C)
#define MARS_USB_HOST_SELF_LOOP_BACK_2PORT	(VENUS_USB_HOST_BASE+0x30)
#define MARS_USB_HOST_IPNEWINPUT_2PORT		(VENUS_USB_HOST_BASE+0x34)
#define MARS_USB_HOST_USBPHY_SLB0		(VENUS_USB_HOST_BASE+0x38)
#define MARS_USB_HOST_USBPHY_SLB1		(VENUS_USB_HOST_BASE+0x3C)
#define MARS_USB_HOST_OTG			(VENUS_USB_HOST_BASE+0x40)
#define MARS_USB_HOST_OTGMUX			(VENUS_USB_HOST_BASE+0x44)

#endif /* !(_MACH_MARS_H) */

