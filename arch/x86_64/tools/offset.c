/* Written 2000 by Andi Kleen */
/* This program is never executed, just its assembly is examined for offsets 
   (this trick is needed to get cross compiling right) */  
/* $Id: offset.c,v 1.13 2002/01/08 15:19:57 ak Exp $ */
#define ASM_OFFSET_H 1
#ifndef __KERNEL__
#define __KERNEL__ 
#endif
#include <linux/sched.h> 
#include <linux/stddef.h>
#include <linux/errno.h> 
#include <asm/pda.h>
#include <asm/hardirq.h>
#include <asm/processor.h>
#include <asm/segment.h>
#include <asm/thread_info.h>

#define output(x) asm volatile ("--- " x)
#define outconst(x,y) asm volatile ("--- " x : : "i" (y)) 

int main(void) 
{ 
	output("/* Auto generated by arch/../tools/offset.c at " __DATE__ ". Do not edit. */\n"); 
	output("#ifndef ASM_OFFSET_H\n");
	output("#define ASM_OFFSET_H 1\n"); 

	// task struct entries needed by entry.S
#define ENTRY(entry) outconst("#define tsk_" #entry " %0", offsetof(struct task_struct, entry))
	ENTRY(state);
	ENTRY(flags); 
	ENTRY(thread); 
#undef ENTRY
#define ENTRY(entry) outconst("#define threadinfo_" #entry " %0", offsetof(struct thread_info, entry))
	ENTRY(flags);
	ENTRY(addr_limit);
	ENTRY(preempt_count);
#undef ENTRY
#define ENTRY(entry) outconst("#define pda_" #entry " %0", offsetof(struct x8664_pda, entry))
	ENTRY(kernelstack); 
	ENTRY(oldrsp); 
	ENTRY(pcurrent); 
	ENTRY(irqrsp);
	ENTRY(irqcount);
	ENTRY(irqstack); 
	ENTRY(pgd_quick);
	ENTRY(pmd_quick);
	ENTRY(pte_quick);
	ENTRY(pgtable_cache_sz);
	ENTRY(cpunumber);
	ENTRY(irqstackptr);
	ENTRY(me);
	ENTRY(__softirq_pending); 
	ENTRY(__local_irq_count);
	ENTRY(__local_bh_count);
	ENTRY(__ksoftirqd_task);
#undef ENTRY
	output("#ifdef __ASSEMBLY__"); 
#define CONST(t) outconst("#define " #t " %0", t)
	CONST(TASK_SIZE);
	CONST(SIGCHLD); 
	CONST(CLONE_VFORK); 
	CONST(CLONE_VM); 
#undef CONST
	output("#endif"); 

	output("#endif\n"); 

	return(0); 
} 

