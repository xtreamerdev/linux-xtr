/* $Id: offset.c,v 1.10 1999/08/18 23:37:46 ralf Exp $
 *
 * offset.c: Calculate pt_regs and task_struct offsets.
 *
 * Copyright (C) 1996 David S. Miller
 * Copyright (C) 1997, 1998, 1999 Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */

#include <linux/types.h>
#include <linux/sched.h>

#include <asm/ptrace.h>
#include <asm/processor.h>

#define text(t) __asm__("\n@@@" t)
#define _offset(type, member) (&(((type *)NULL)->member))

#define offset(string, ptr, member) \
	__asm__("\n@@@" string "%0" : : "i" (_offset(ptr, member)))
#define size(string, size) \
	__asm__("\n@@@" string "%0" : : "i" (sizeof(size)))
#define linefeed text("")

text("/* DO NOT TOUCH, AUTOGENERATED BY OFFSET.C */");
linefeed;
text("#ifndef _MIPS_OFFSET_H");
text("#define _MIPS_OFFSET_H");
linefeed;

void output_ptreg_defines(void)
{
	text("/* MIPS pt_regs offsets. */");
	offset("#define PT_R0     ", struct pt_regs, regs[0]);
	offset("#define PT_R1     ", struct pt_regs, regs[1]);
	offset("#define PT_R2     ", struct pt_regs, regs[2]);
	offset("#define PT_R3     ", struct pt_regs, regs[3]);
	offset("#define PT_R4     ", struct pt_regs, regs[4]);
	offset("#define PT_R5     ", struct pt_regs, regs[5]);
	offset("#define PT_R6     ", struct pt_regs, regs[6]);
	offset("#define PT_R7     ", struct pt_regs, regs[7]);
	offset("#define PT_R8     ", struct pt_regs, regs[8]);
	offset("#define PT_R9     ", struct pt_regs, regs[9]);
	offset("#define PT_R10    ", struct pt_regs, regs[10]);
	offset("#define PT_R11    ", struct pt_regs, regs[11]);
	offset("#define PT_R12    ", struct pt_regs, regs[12]);
	offset("#define PT_R13    ", struct pt_regs, regs[13]);
	offset("#define PT_R14    ", struct pt_regs, regs[14]);
	offset("#define PT_R15    ", struct pt_regs, regs[15]);
	offset("#define PT_R16    ", struct pt_regs, regs[16]);
	offset("#define PT_R17    ", struct pt_regs, regs[17]);
	offset("#define PT_R18    ", struct pt_regs, regs[18]);
	offset("#define PT_R19    ", struct pt_regs, regs[19]);
	offset("#define PT_R20    ", struct pt_regs, regs[20]);
	offset("#define PT_R21    ", struct pt_regs, regs[21]);
	offset("#define PT_R22    ", struct pt_regs, regs[22]);
	offset("#define PT_R23    ", struct pt_regs, regs[23]);
	offset("#define PT_R24    ", struct pt_regs, regs[24]);
	offset("#define PT_R25    ", struct pt_regs, regs[25]);
	offset("#define PT_R26    ", struct pt_regs, regs[26]);
	offset("#define PT_R27    ", struct pt_regs, regs[27]);
	offset("#define PT_R28    ", struct pt_regs, regs[28]);
	offset("#define PT_R29    ", struct pt_regs, regs[29]);
	offset("#define PT_R30    ", struct pt_regs, regs[30]);
	offset("#define PT_R31    ", struct pt_regs, regs[31]);
	offset("#define PT_LO     ", struct pt_regs, lo);
	offset("#define PT_HI     ", struct pt_regs, hi);
	offset("#define PT_EPC    ", struct pt_regs, cp0_epc);
	offset("#define PT_BVADDR ", struct pt_regs, cp0_badvaddr);
	offset("#define PT_STATUS ", struct pt_regs, cp0_status);
	offset("#define PT_CAUSE  ", struct pt_regs, cp0_cause);
	size("#define PT_SIZE   ", struct pt_regs);
	linefeed;
}

void output_task_defines(void)
{
	text("/* MIPS task_struct offsets. */");
	offset("#define TASK_STATE         ", struct task_struct, state);
	offset("#define TASK_FLAGS         ", struct task_struct, flags);
	offset("#define TASK_SIGPENDING    ", struct task_struct, sigpending);
	offset("#define TASK_NEED_RESCHED  ", struct task_struct, need_resched);
	offset("#define TASK_COUNTER       ", struct task_struct, counter);
	offset("#define TASK_PRIORITY      ", struct task_struct, priority);
	offset("#define TASK_MM            ", struct task_struct, mm);
	size("#define TASK_STRUCT_SIZE   ", struct task_struct);
	linefeed;
}

void output_thread_defines(void)
{
	text("/* MIPS specific thread_struct offsets. */");
	offset("#define THREAD_REG16   ", struct task_struct, tss.reg16);
	offset("#define THREAD_REG17   ", struct task_struct, tss.reg17);
	offset("#define THREAD_REG18   ", struct task_struct, tss.reg18);
	offset("#define THREAD_REG19   ", struct task_struct, tss.reg19);
	offset("#define THREAD_REG20   ", struct task_struct, tss.reg20);
	offset("#define THREAD_REG21   ", struct task_struct, tss.reg21);
	offset("#define THREAD_REG22   ", struct task_struct, tss.reg22);
	offset("#define THREAD_REG23   ", struct task_struct, tss.reg23);
	offset("#define THREAD_REG29   ", struct task_struct, tss.reg29);
	offset("#define THREAD_REG30   ", struct task_struct, tss.reg30);
	offset("#define THREAD_REG31   ", struct task_struct, tss.reg31);
	offset("#define THREAD_STATUS  ", struct task_struct, tss.cp0_status);
	offset("#define THREAD_FPU     ", struct task_struct, tss.fpu);
	offset("#define THREAD_BVADDR  ", struct task_struct, tss.cp0_badvaddr);
	offset("#define THREAD_BUADDR  ", struct task_struct, tss.cp0_baduaddr);
	offset("#define THREAD_ECODE   ", struct task_struct, tss.error_code);
	offset("#define THREAD_TRAPNO  ", struct task_struct, tss.trap_no);
	offset("#define THREAD_PGDIR   ", struct task_struct, tss.pg_dir);
	offset("#define THREAD_MFLAGS  ", struct task_struct, tss.mflags);
	offset("#define THREAD_CURDS   ", struct task_struct, tss.current_ds);
	offset("#define THREAD_TRAMP   ", struct task_struct, tss.irix_trampoline);
	offset("#define THREAD_OLDCTX  ", struct task_struct, tss.irix_oldctx);
	linefeed;
}

void output_mm_defines(void)
{
	text("/* Linux mm_struct offsets. */");
	offset("#define MM_COUNT      ", struct mm_struct, count);
	offset("#define MM_PGD        ", struct mm_struct, pgd);
	offset("#define MM_CONTEXT    ", struct mm_struct, context);
	linefeed;
}

void output_sc_defines(void)
{
	text("/* Linux sigcontext offsets. */");
	offset("#define SC_REGS       ", struct sigcontext, sc_regs);
	offset("#define SC_FPREGS     ", struct sigcontext, sc_fpregs);
	offset("#define SC_MDHI       ", struct sigcontext, sc_mdhi);
	offset("#define SC_MDLO       ", struct sigcontext, sc_mdlo);
	offset("#define SC_PC         ", struct sigcontext, sc_pc);
	offset("#define SC_STATUS     ", struct sigcontext, sc_status);
	offset("#define SC_OWNEDFP    ", struct sigcontext, sc_ownedfp);
	offset("#define SC_FPC_CSR    ", struct sigcontext, sc_fpc_csr);
	offset("#define SC_FPC_EIR    ", struct sigcontext, sc_fpc_eir);
	offset("#define SC_CAUSE      ", struct sigcontext, sc_cause);
	offset("#define SC_BADVADDR   ", struct sigcontext, sc_badvaddr);
	linefeed;
}

text("#endif /* !(_MIPS_OFFSET_H) */");
