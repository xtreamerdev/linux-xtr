/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 95, 96, 99, 2001 Ralf Baechle
 * Copyright (C) 1994, 1995, 1996 Paul M. Antoine.
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#ifndef _ASM_STACKFRAME_H
#define _ASM_STACKFRAME_H

#include <linux/config.h>
#include <linux/threads.h>

#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/offset.h>

		.macro	SAVE_AT
		.set	push
		.set	noat
		LONG_S	$1, PT_R1(sp)
		.set	pop
		.endm

		.macro	SAVE_TEMP
		mfhi	v1
#ifdef CONFIG_MIPS32
		LONG_S	$8, PT_R8(sp)
		LONG_S	$9, PT_R9(sp)
#endif
		LONG_S	v1, PT_HI(sp)
		mflo	v1
		LONG_S	$10, PT_R10(sp)
		LONG_S	$11, PT_R11(sp)
		LONG_S	v1,  PT_LO(sp)
		LONG_S	$12, PT_R12(sp)
		LONG_S	$13, PT_R13(sp)
		LONG_S	$14, PT_R14(sp)
		LONG_S	$15, PT_R15(sp)
		LONG_S	$24, PT_R24(sp)
		.endm

		.macro	SAVE_STATIC
		LONG_S	$16, PT_R16(sp)
		LONG_S	$17, PT_R17(sp)
		LONG_S	$18, PT_R18(sp)
		LONG_S	$19, PT_R19(sp)
		LONG_S	$20, PT_R20(sp)
		LONG_S	$21, PT_R21(sp)
		LONG_S	$22, PT_R22(sp)
		LONG_S	$23, PT_R23(sp)
		LONG_S	$30, PT_R30(sp)
		.endm

#ifdef CONFIG_SMP
		.macro	get_saved_sp	/* SMP variation */
#ifdef CONFIG_MIPS32
		mfc0	k0, CP0_CONTEXT
		lui	k1, %hi(kernelsp)
		srl	k0, k0, 23
		addu	k1, k0
		LONG_L	k1, %lo(kernelsp)(k1)
#endif
#if defined(CONFIG_MIPS64) && !defined(CONFIG_BUILD_ELF64)
		MFC0	k1, CP0_CONTEXT
		dsra	k1, 23
		lui	k0, %hi(pgd_current)
		addiu	k0, %lo(pgd_current)
		dsubu	k1, k0
		lui	k0, %hi(kernelsp)
		daddu	k1, k0
		LONG_L	k1, %lo(kernelsp)(k1)
#endif
#if defined(CONFIG_MIPS64) && defined(CONFIG_BUILD_ELF64)
		MFC0	k1, CP0_CONTEXT
		lui	k0, %highest(kernelsp)
		dsrl	k1, 23
		daddiu	k0, %higher(kernelsp)
		dsll	k0, k0, 16
		daddiu	k0, %hi(kernelsp)
		dsll	k0, k0, 16
		daddu	k1, k1, k0
		LONG_L	k1, %lo(kernelsp)(k1)
#endif
		.endm

		.macro	set_saved_sp stackp temp temp2
#ifdef CONFIG_MIPS32
		mfc0	\temp, CP0_CONTEXT
		srl	\temp, 23
#endif
#if defined(CONFIG_MIPS64) && !defined(CONFIG_BUILD_ELF64)
		lw	\temp, TI_CPU(gp)
		dsll	\temp, 3
#endif
#if defined(CONFIG_MIPS64) && defined(CONFIG_BUILD_ELF64)
		MFC0	\temp, CP0_CONTEXT
		dsrl	\temp, 23
#endif
		LONG_S	\stackp, kernelsp(\temp)
		.endm
#else
		.macro	get_saved_sp	/* Uniprocessor variation */
#if defined(CONFIG_MIPS64) && defined(CONFIG_BUILD_ELF64)
		lui	k1, %highest(kernelsp)
		daddiu	k1, %higher(kernelsp)
		dsll	k1, k1, 16
		daddiu	k1, %hi(kernelsp)
		dsll	k1, k1, 16
#else
		lui	k1, %hi(kernelsp)
#endif
		LONG_L	k1, %lo(kernelsp)(k1)
		.endm

		.macro	set_saved_sp stackp temp temp2
		LONG_S	\stackp, kernelsp
		.endm
#endif

#ifdef CONFIG_REALTEK_USE_DMEM
		.macro	DMEM_SAVE_SOME
		.set	push
		.set	noat
		.set	reorder
		mfc0	k0, CP0_STATUS
#ifdef CONFIG_REALTEK_OPEN_CU0
		and	k0, 0x10	/* check um bit */ 
		.set	noreorder
		beqz	k0, 8f 
#else
		sll	k0, 3		/* extract cu0 bit */
		.set	noreorder
		bltz	k0, 8f
#endif
		move	k1, sp
		.set	reorder
		/* Called from user mode, new stack. */
		get_saved_sp
8:		move	k0, sp
		PTR_SUBU sp, k1, PT_SIZE
		li	k1, 0x1fff
		and	sp, k1
		lui	k1, 0x9000
		or	sp, k1
		/* From now on, sp points to DMEM */
		LONG_S	k0, PT_R29(sp)
		LONG_S	$3, PT_R3(sp)
		LONG_S	$0, PT_R0(sp)
		mfc0	v1, CP0_STATUS
		LONG_S	$2, PT_R2(sp)
		LONG_S	v1, PT_STATUS(sp)
		LONG_S	$4, PT_R4(sp)
		mfc0	v1, CP0_CAUSE
		LONG_S	$5, PT_R5(sp)
		LONG_S	v1, PT_CAUSE(sp)
		LONG_S	$6, PT_R6(sp)
		MFC0	v1, CP0_EPC
		LONG_S	$7, PT_R7(sp)
#ifdef CONFIG_MIPS64
		LONG_S	$8, PT_R8(sp)
		LONG_S	$9, PT_R9(sp)
#endif
		LONG_S	v1, PT_EPC(sp)
		LONG_S	$25, PT_R25(sp)
		LONG_S	$28, PT_R28(sp)
		LONG_S	$31, PT_R31(sp)
		get_saved_sp
		ori	$28, k1, _THREAD_MASK
		xori	$28, _THREAD_MASK
		.set	pop
		.endm

		.macro	DMEM_SAVE_ON_DEMAND
		.set	push
		.set	noat
		.set	reorder
		li	k0, 0xffff
		and	k0, sp		# k0 is the offset
		add	k1, k0, $28	# k1 is the address in ddr

		/* flush the cache */
		li	t0, PT_SIZE
		move	t1, k1
1:
		cache	0x11, 0(t1)	# invalidate cache
		subu	t0, 16
		addu	t1, 16
		bnez	t0, 1b
		nop
		cache	0x15, 0(t1)	# writeback & invalidate cache
		sync

		/* move data from dmem to ddr */
		li	k0, PT_SIZE
		li	t0, 0xb8003000
		li	t1, 0x7
		sw	sp, 0(t0)	# dmem addr
		sw	k1, 0x20(t0)	# ddr addr
		sw	k0, 0x40(t0)	# dma size
		sw	t1, 0x60(t0)
2:
		lw	t1, 0x80(t0)
		and	t1, 0x4
		beqz	t1, 2b

		/* restore the sp value */
		move	sp, k1
		.set    pop
		.endm
#endif

#ifdef CONFIG_REALTEK_USE_SHADOW_REGISTERS
                .macro  MY_SAVE_ON_DEMAND
                .set    push
                .set    noat
                .set    reorder
                .word   0x40806002      # mtc0 $0, $12, 2
                la      k0, bbb
                mtc0    k0, CP0_EPC
                ehb
                eret
bbb:            li      k0, 0x40
                .word   0x409a6002      # mtc0 k0, $12, 2
                nop
                .word   0x40806003      # mtc0 $0, $12, 3
                .word   0x415dd800      # rdpgpr k1, sp
                LONG_S  $3, PT_R3(k1)
                LONG_S  $0, PT_R0(k1)
                LONG_S  $2, PT_R2(k1)
                LONG_S  $4, PT_R4(k1)
                LONG_S  $5, PT_R5(k1)
                LONG_S  $6, PT_R6(k1)
                LONG_S  $7, PT_R7(k1)
                LONG_S  $25, PT_R25(k1)
                LONG_S  $28, PT_R28(k1)
                LONG_S  $29, PT_R29(k1)
                LONG_S  $31, PT_R31(k1)
                LONG_S  $1, PT_R1(k1)
                LONG_S  $8, PT_R8(k1)
                LONG_S  $9, PT_R9(k1)
                LONG_S  $10, PT_R10(k1)
                LONG_S  $11, PT_R11(k1)
                LONG_S  $12, PT_R12(k1)
                LONG_S  $13, PT_R13(k1)
                LONG_S  $14, PT_R14(k1)
                LONG_S  $15, PT_R15(k1)
                LONG_S  $24, PT_R24(k1)
                LONG_S  $16, PT_R16(k1)
                LONG_S  $17, PT_R17(k1)
                LONG_S  $18, PT_R18(k1)
                LONG_S  $19, PT_R19(k1)
                LONG_S  $20, PT_R20(k1)
                LONG_S  $21, PT_R21(k1)
                LONG_S  $22, PT_R22(k1)
                LONG_S  $23, PT_R23(k1)
                LONG_S  $30, PT_R30(k1)

                .word   0x415de800      # rdpgpr sp, sp
                .word   0x415ce000      # rdpgpr gp, gp
                .word   0x41442000      # rdpgpr a0, a0
                .word   0x41452800      # rdpgpr a1, a1
                .word   0x41463000      # rdpgpr a2, a2
                .word   0x41473800      # rdpgpr a3, a3
/*
                .word   0x41508000      # rdpgpr s0, s0
                .word   0x41518800      # rdpgpr s1, s1
                .word   0x41529000      # rdpgpr s2, s2
                .word   0x41539800      # rdpgpr s3, s3
                .word   0x4154a000      # rdpgpr s4, s4
                .word   0x4155a800      # rdpgpr s5, s5
                .word   0x4156b000      # rdpgpr s6, s6
                .word   0x4157b800      # rdpgpr s7, s7
                .word   0x415ef000      # rdpgpr s8, s8
                .word   0x41421000      # rdpgpr v0, v0
                .word   0x41431800      # rdpgpr v1, v1
                .word   0x41484000      # rdpgpr t0, t0
                .word   0x41494800      # rdpgpr t1, t1
                .word   0x414a5000      # rdpgpr t2, t2
                .word   0x414b5800      # rdpgpr t3, t3
                .word   0x414c6000      # rdpgpr t4, t4
                .word   0x414d6800      # rdpgpr t5, t5
                .word   0x414e7000      # rdpgpr t6, t6
                .word   0x414f7800      # rdpgpr t7, t7
                .word   0x4158c000      # rdpgpr t8, t8
                .word   0x4159c800      # rdpgpr t9, t9
                .word   0x41410800      # rdpgpr at, at
                .word   0x415ff800      # rdpgpr ra, ra
*/
                .word   0x40806002      # mtc0 $0, $12, 2
                .set    pop
                .endm

                .macro  MY_SAVE_ALL
                .set    push
                .set    noat
                .set    reorder
                mfc0    k0, CP0_STATUS
#ifdef CONFIG_REALTEK_OPEN_CU0
                and     k0, 0x10        /* check um bit */
                beqz    k0, normal_save # from kernel mode 
#else
                sll     k0, 3           /* extract cu0 bit */
                bltz    k0, normal_save # from kernel mode
#endif

                lui     k1, %hi(kernelsp)
                LONG_L  k1, %lo(kernelsp)(k1)
                PTR_SUBU k1, k1, PT_SIZE
                mfc0    k0, CP0_EPC
                LONG_S  k0, PT_EPC(k1)
                mfc0    k0, CP0_STATUS
                LONG_S  k0, PT_STATUS(k1)
                ori     k0, 0x1f
                xori    k0, 0x1f
                mtc0    k0, CP0_STATUS  # prevent entering user mode
                li      k0, 0x40
                .word   0x409a6002      # mtc0 k0, $12, 2
                la      k0, aaa
                mtc0    k0, CP0_EPC
                ehb
                eret
aaa:            li      k0, 0x1000
                .word   0x409a6002      # mtc0 k0, $12, 2
                li      k0, 0x11111111
                .word   0x409a6003      # mtc0 k0, $12, 3
                .word   0x415be800      # rdpgpr sp, k1
                .word   0x415ce000      # rdpgpr gp, gp
                mfc0    v0, CP0_CAUSE
//              .word   0x40036002      # mfc0 v1, $12, 2
                LONG_S  v0, PT_CAUSE(sp)
//              LONG_S  v1, PT_SRSCTL(sp)
                mfhi    v0
                mflo    v1
                LONG_S  v0, PT_HI(sp)
                LONG_S  v1, PT_LO(sp)
                ori     $28, sp, _THREAD_MASK
                xori    $28, _THREAD_MASK
                b       out_save
                .set    pop
normal_save:
                SAVE_SOME
                SAVE_AT
                SAVE_TEMP
                SAVE_STATIC
out_save:
                .endm

                .macro  MY_RESTORE_ALL
                .set    push
                .set    noat
                .set    reorder
                .word   0x40806003      # mtc0 $0, $12, 3
                LONG_L  k0, PT_LO(sp)
                LONG_L  k1, PT_HI(sp)
                mtlo    k0
                mthi    k1
                mfc0    a0, CP0_STATUS
                ori     a0, 0x1f
                xori    a0, 0x1f
                mtc0    a0, CP0_STATUS
                li      v1, 0xff00
                and     a0, v1
                LONG_L  v0, PT_STATUS(sp)
                nor     v1, $0, v1
                and     v0, v1
                or      v0, a0
                mtc0    v0, CP0_STATUS
                LONG_L  v1, PT_EPC(sp)
                MTC0    v1, CP0_EPC
                .word   0x40806002      # mtc0 $0, $12, 2
                .set    mips3
                eret
                .set    mips0
//              b       out_restore
                .set    pop
                .endm
#endif

#ifdef CONFIG_REALTEK_USE_FAST_INTERRUPT
                .macro  FAST_SAVE_ALL
                .set    push
                .set    noat
                .set    reorder
                mfc0    k0, CP0_STATUS
#ifdef CONFIG_REALTEK_OPEN_CU0
                and     k0, 0x10        /* check um bit */
                beqz    k0, normal_save # from kernel mode 
#else
                sll     k0, 3           /* extract cu0 bit */
                bltz    k0, normal_save # from kernel mode
#endif

//                li      k0, 0x61;               /*cy test*/
//                li      k1, 0xb801b200
//                sw      k0, (k1)

                lui     k1, %hi(kernelsp)
                LONG_L  k1, %lo(kernelsp)(k1)
use_shadow:     PTR_SUBU k1, k1, PT_SIZE
                mfc0    k0, CP0_EPC
                LONG_S  k0, PT_EPC(k1)
                mfc0    k0, CP0_STATUS
                LONG_S  k0, PT_STATUS(k1)
                ori     k0, 0x1f
                xori    k0, 0x1f
                mtc0    k0, CP0_STATUS  # prevent entering user mode
                li      k0, 0x40
                .word   0x409a6002      # mtc0 k0, $12, 2
                la      k0, aaa
                mtc0    k0, CP0_EPC
                ehb
                eret
aaa:            li      k0, 0x1000
                .word   0x409a6002      # mtc0 k0, $12, 2
                li      k0, 0x11111111
                .word   0x409a6003      # mtc0 k0, $12, 3
                .word   0x415be800      # rdpgpr sp, k1
                .word   0x415ce000      # rdpgpr gp, gp
                mfc0    v0, CP0_CAUSE
//              .word   0x40036002      # mfc0 v1, $12, 2
                LONG_S  v0, PT_CAUSE(sp)
//              LONG_S  v1, PT_SRSCTL(sp)
                LONG_S  $0, PT_BVADDR(sp)
                mfhi    v0
                mflo    v1
                LONG_S  v0, PT_HI(sp)
                LONG_S  v1, PT_LO(sp)
                ori     $28, sp, _THREAD_MASK
                xori    $28, _THREAD_MASK
                b       out_save
                .set    pop
normal_save:
		/* check if we can use shadow register */
                .word   0x401a6002      # mfc0 k0, $12, 2
                li	k1, 0xf
                and	k0, k1
                .set	noreorder
                beqz	k0, use_shadow
                move	k1, sp
                .set	reorder

                SAVE_SOME
                SAVE_AT
                SAVE_TEMP
                SAVE_STATIC

		li	k0, 0xffff
                LONG_S  k0, PT_BVADDR(sp)
out_save:
		.endm

                .macro  FAST_RESTORE_ALL
                .set    push
                .set    noat
                .set    reorder
                LONG_L  k0, PT_BVADDR(sp)
                bnez	k0, normal_restore
                .word   0x40806003      # mtc0 $0, $12, 3
                LONG_L  k0, PT_LO(sp)
                LONG_L  k1, PT_HI(sp)
                mtlo    k0
                mthi    k1
                mfc0    a0, CP0_STATUS
                ori     a0, 0x1f
                xori    a0, 0x1f
                mtc0    a0, CP0_STATUS
                li      v1, 0xff00
                and     a0, v1
                LONG_L  v0, PT_STATUS(sp)
                nor     v1, $0, v1
                and     v0, v1
                or      v0, a0
                mtc0    v0, CP0_STATUS
                LONG_L  v1, PT_EPC(sp)
                MTC0    v1, CP0_EPC
                .word   0x40806002      # mtc0 $0, $12, 2
                .set    mips3
                eret
                .set    mips0
                .set    pop
normal_restore:
//                li      k0, 0x62;               /*cy test*/
//                li      k1, 0xb801b200
//                sw      k0, (k1)
                RESTORE_TEMP
                RESTORE_AT
                RESTORE_STATIC
                RESTORE_SOME
                RESTORE_SP_AND_RET
                .endm
#endif

		.macro	SAVE_SOME
		.set	push
		.set	noat
		.set	reorder
		mfc0	k0, CP0_STATUS
#ifdef CONFIG_REALTEK_OPEN_CU0
		and	k0, 0x10	/* check um bit */ 
		.set	noreorder
		beqz	k0, 8f 
#else
		sll	k0, 3		/* extract cu0 bit */
		.set	noreorder
		bltz	k0, 8f
#endif
		move	k1, sp
		.set	reorder
		/* Called from user mode, new stack. */
		get_saved_sp
8:		move	k0, sp
		PTR_SUBU sp, k1, PT_SIZE
		LONG_S	k0, PT_R29(sp)
		LONG_S	$3, PT_R3(sp)
		LONG_S	$0, PT_R0(sp)
		mfc0	v1, CP0_STATUS
		LONG_S	$2, PT_R2(sp)
		LONG_S	v1, PT_STATUS(sp)
		LONG_S	$4, PT_R4(sp)
		mfc0	v1, CP0_CAUSE
		LONG_S	$5, PT_R5(sp)
		LONG_S	v1, PT_CAUSE(sp)
		LONG_S	$6, PT_R6(sp)
		MFC0	v1, CP0_EPC
		LONG_S	$7, PT_R7(sp)
#ifdef CONFIG_MIPS64
		LONG_S	$8, PT_R8(sp)
		LONG_S	$9, PT_R9(sp)
#endif
		LONG_S	v1, PT_EPC(sp)
		LONG_S	$25, PT_R25(sp)
		LONG_S	$28, PT_R28(sp)
		LONG_S	$31, PT_R31(sp)
#ifdef CONFIG_REALTEK_USE_DMEM
		get_saved_sp
		ori	$28, k1, _THREAD_MASK
#else
		ori	$28, sp, _THREAD_MASK
#endif
		xori	$28, _THREAD_MASK
		.set	pop
		.endm

#ifdef CONFIG_REALTEK_USE_DMEM
		.macro	DMEM_SAVE_ALL
		DMEM_SAVE_SOME
		SAVE_AT
		SAVE_TEMP
		SAVE_STATIC
		.endm
#endif

		.macro	SAVE_ALL
		SAVE_SOME
		SAVE_AT
		SAVE_TEMP
		SAVE_STATIC
		.endm

		.macro	RESTORE_AT
		.set	push
		.set	noat
		LONG_L	$1,  PT_R1(sp)
		.set	pop
		.endm

		.macro	RESTORE_TEMP
		LONG_L	$24, PT_LO(sp)
#ifdef CONFIG_MIPS32
		LONG_L	$8, PT_R8(sp)
		LONG_L	$9, PT_R9(sp)
#endif
		mtlo	$24
		LONG_L	$24, PT_HI(sp)
		LONG_L	$10, PT_R10(sp)
		LONG_L	$11, PT_R11(sp)
		mthi	$24
		LONG_L	$12, PT_R12(sp)
		LONG_L	$13, PT_R13(sp)
		LONG_L	$14, PT_R14(sp)
		LONG_L	$15, PT_R15(sp)
		LONG_L	$24, PT_R24(sp)
		.endm

		.macro	RESTORE_STATIC
		LONG_L	$16, PT_R16(sp)
		LONG_L	$17, PT_R17(sp)
		LONG_L	$18, PT_R18(sp)
		LONG_L	$19, PT_R19(sp)
		LONG_L	$20, PT_R20(sp)
		LONG_L	$21, PT_R21(sp)
		LONG_L	$22, PT_R22(sp)
		LONG_L	$23, PT_R23(sp)
		LONG_L	$30, PT_R30(sp)
		.endm

#if defined(CONFIG_CPU_R3000) || defined(CONFIG_CPU_TX39XX)

		.macro	RESTORE_SOME
		.set	push
		.set	reorder
		.set	noat
		mfc0	a0, CP0_STATUS
		ori	a0, 0x1f
		xori	a0, 0x1f
		mtc0	a0, CP0_STATUS
		li	v1, 0xff00
		and	a0, v1
		LONG_L	v0, PT_STATUS(sp)
		nor	v1, $0, v1
		and	v0, v1
		or	v0, a0
		mtc0	v0, CP0_STATUS
		LONG_L	$31, PT_R31(sp)
		LONG_L	$28, PT_R28(sp)
		LONG_L	$25, PT_R25(sp)
#ifdef CONFIG_MIPS64
		LONG_L	$8, PT_R8(sp)
		LONG_L	$9, PT_R9(sp)
#endif
		LONG_L	$7,  PT_R7(sp)
		LONG_L	$6,  PT_R6(sp)
		LONG_L	$5,  PT_R5(sp)
		LONG_L	$4,  PT_R4(sp)
		LONG_L	$3,  PT_R3(sp)
		LONG_L	$2,  PT_R2(sp)
		.set	pop
		.endm

		.macro	RESTORE_SP_AND_RET
		.set	push
		.set	noreorder
		LONG_L	k0, PT_EPC(sp)
		LONG_L	sp, PT_R29(sp)
		jr	k0
		 rfe
		.set	pop
		.endm

#else

		.macro	RESTORE_SOME
		.set	push
		.set	reorder
		.set	noat
		mfc0	a0, CP0_STATUS
		ori	a0, 0x1f
		xori	a0, 0x1f
		mtc0	a0, CP0_STATUS
		li	v1, 0xff00
		and	a0, v1
		LONG_L	v0, PT_STATUS(sp)
		nor	v1, $0, v1
		and	v0, v1
		or	v0, a0
		mtc0	v0, CP0_STATUS
		LONG_L	v1, PT_EPC(sp)
		MTC0	v1, CP0_EPC
		LONG_L	$31, PT_R31(sp)
		LONG_L	$28, PT_R28(sp)
		LONG_L	$25, PT_R25(sp)
#ifdef CONFIG_MIPS64
		LONG_L	$8, PT_R8(sp)
		LONG_L	$9, PT_R9(sp)
#endif
		LONG_L	$7,  PT_R7(sp)
		LONG_L	$6,  PT_R6(sp)
		LONG_L	$5,  PT_R5(sp)
		LONG_L	$4,  PT_R4(sp)
		LONG_L	$3,  PT_R3(sp)
		LONG_L	$2,  PT_R2(sp)
		.set	pop
		.endm

		.macro	RESTORE_SP_AND_RET
		LONG_L	sp, PT_R29(sp)
		.set	mips3
		eret
		.set	mips0
		.endm

#endif

		.macro	RESTORE_SP
		LONG_L	sp, PT_R29(sp)
		.endm

		.macro	RESTORE_ALL
		RESTORE_TEMP
		RESTORE_STATIC
		RESTORE_AT
		RESTORE_SOME
		RESTORE_SP
		.endm

		.macro	RESTORE_ALL_AND_RET
		RESTORE_TEMP
		RESTORE_STATIC
		RESTORE_AT
		RESTORE_SOME
		RESTORE_SP_AND_RET
		.endm

/*
 * Move to kernel mode and disable interrupts.
 * Set cp0 enable bit as sign that we're running on the kernel stack
 */
		.macro	CLI
		mfc0	t0, CP0_STATUS
		li	t1, ST0_CU0 | 0x1f
		or	t0, t1
		xori	t0, 0x1f
		mtc0	t0, CP0_STATUS
		irq_disable_hazard
		.endm

/*
 * Move to kernel mode and enable interrupts.
 * Set cp0 enable bit as sign that we're running on the kernel stack
 */
		.macro	STI
		mfc0	t0, CP0_STATUS
		li	t1, ST0_CU0 | 0x1f
		or	t0, t1
		xori	t0, 0x1e
		mtc0	t0, CP0_STATUS
		irq_enable_hazard
		.endm

/*
 * Just move to kernel mode and leave interrupts as they are.
 * Set cp0 enable bit as sign that we're running on the kernel stack
 */
		.macro	KMODE
		mfc0	t0, CP0_STATUS
		li	t1, ST0_CU0 | 0x1e
		or	t0, t1
		xori	t0, 0x1e
		mtc0	t0, CP0_STATUS
		irq_disable_hazard
		.endm

#endif /* _ASM_STACKFRAME_H */
