#ifndef _ASM_PARISC_THREAD_INFO_H
#define _ASM_PARISC_THREAD_INFO_H

#ifdef __KERNEL__

#ifndef __ASSEMBLY__
#include <asm/processor.h>

struct thread_info {
	struct task_struct *task;	/* main task structure */
	struct exec_domain *exec_domain;/* execution domain */
	__u32 flags;			/* thread_info flags (see TIF_*) */
	__u32 cpu;			/* current CPU */
	mm_segment_t addr_limit;	/* user-level address space limit */
	__s32 preempt_count;		/* 0=premptable, <0=BUG; will also serve as bh-counter */
};

#define INIT_THREAD_INFO(tsk)			\
{						\
	task:		&tsk,			\
	exec_domain:	&default_exec_domain,	\
	flags:		0,			\
	cpu:		0,			\
	addr_limit:	KERNEL_DS,		\
	preempt_count:	0,			\
}

#define init_thread_info        (init_thread_union.thread_info)
#define init_stack              (init_thread_union.stack)

/* thread information allocation */

#define THREAD_ORDER            2
/* Be sure to hunt all references to this down when you change the size of
 * the kernel stack */
#define THREAD_SIZE             (PAGE_SIZE << THREAD_ORDER)
#define THREAD_SHIFT            (PAGE_SHIFT + THREAD_ORDER)

#define alloc_thread_info() ((struct thread_info *) \
			__get_free_pages(GFP_KERNEL, THREAD_ORDER))
#define free_thread_info(ti)    free_pages((unsigned long) (ti), THREAD_ORDER)
#define get_thread_info(ti)     get_task_struct((ti)->task)
#define put_thread_info(ti)     put_task_struct((ti)->task)


/* how to get the thread information struct from C */
#define current_thread_info()	((struct thread_info *)mfctl(30))

#endif /* !__ASSEMBLY */

#define PREEMPT_ACTIVE          0x4000000

/*
 * thread information flags
 */
#define TIF_SYSCALL_TRACE	0	/* syscall trace active */
#define TIF_NOTIFY_RESUME	1	/* resumption notification requested */
#define TIF_SIGPENDING		2	/* signal pending */
#define TIF_NEED_RESCHED	3	/* rescheduling necessary */
#define TIF_POLLING_NRFLAG	4	/* true if poll_idle() is polling TIF_NEED_RESCHED */
#define TIF_32BIT               5       /* 32 bit binary */

#define TIF_WORK_MASK		0x7	/* like TIF_ALLWORK_BITS but sans TIF_SYSCALL_TRACE */
#define TIF_ALLWORK_MASK	0xf	/* bits 0..3 are "work to do on user-return" bits */

#define _TIF_SYSCALL_TRACE	(1 << TIF_SYSCALL_TRACE)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)
#define _TIF_POLLING_NRFLAG	(1 << TIF_POLLING_NRFLAG)
#define _TIF_32BIT		(1 << TIF_32BIT)

#define _TIF_USER_WORK_MASK     (_TIF_NOTIFY_RESUME | _TIF_SIGPENDING | \
                                 _TIF_NEED_RESCHED)

#endif /* __KERNEL__ */

#endif /* _ASM_PARISC_THREAD_INFO_H */
