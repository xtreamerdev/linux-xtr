#ifndef _ALPHA_SPINLOCK_H
#define _ALPHA_SPINLOCK_H

#include <asm/system.h>

#ifndef __SMP__

/* gcc 2.7.2 can crash initializing an empty structure.  */
typedef struct { int dummy; } spinlock_t;
#define SPIN_LOCK_UNLOCKED { 0 }

#define spin_lock_init(lock)			((void) 0)
#define spin_lock(lock)				((void) 0)
#define spin_trylock(lock)			((void) 0)
#define spin_unlock_wait(lock)			((void) 0)
#define spin_unlock(lock)			((void) 0)
#define spin_lock_irq(lock)			cli()
#define spin_unlock_irq(lock)			sti()

#define spin_lock_irqsave(lock, flags)		save_and_cli(flags)
#define spin_unlock_irqrestore(lock, flags)	restore_flags(flags)

/*
 * Read-write spinlocks, allowing multiple readers
 * but only one writer.
 *
 * NOTE! it is quite common to have readers in interrupts
 * but no interrupt writers. For those circumstances we
 * can "mix" irq-safe locks - any writer needs to get a
 * irq-safe write-lock, but readers can get non-irqsafe
 * read-locks.
 */
typedef struct { int dummy; } rwlock_t;
#define RW_LOCK_UNLOCKED { 0 }

#define read_lock(lock)				((void) 0)
#define read_unlock(lock)			((void) 0)
#define write_lock(lock)			((void) 0)
#define write_unlock(lock)			((void) 0)
#define read_lock_irq(lock)			cli()
#define read_unlock_irq(lock)			sti()
#define write_lock_irq(lock)			cli()
#define write_unlock_irq(lock)			sti()

#define read_lock_irqsave(lock, flags)		save_and_cli(flags)
#define read_unlock_irqrestore(lock, flags)	restore_flags(flags)
#define write_lock_irqsave(lock, flags)		save_and_cli(flags)
#define write_unlock_irqrestore(lock, flags)	restore_flags(flags)

#else /* __SMP__ */

#include <linux/kernel.h>
#include <asm/current.h>

#define DEBUG_SPINLOCK 1
#define DEBUG_RWLOCK 1

/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */

typedef struct {
	volatile unsigned long lock;
	void *previous;
	struct task_struct * task;
} spinlock_t;

#define SPIN_LOCK_UNLOCKED { 0, 0, 0 }

#define spin_lock_init(x) \
	((x)->lock = 0, (x)->previous = 0, (x)->task = 0)
#define spin_unlock_wait(x) \
	({ do { barrier(); } while(((volatile spinlock_t *)x)->lock); })

typedef struct { unsigned long a[100]; } __dummy_lock_t;
#define __dummy_lock(lock) (*(__dummy_lock_t *)(lock))

#if DEBUG_SPINLOCK
extern void spin_lock(spinlock_t * lock);
#else
static inline void spin_lock(spinlock_t * lock)
{
	long tmp;

	/* Use sub-sections to put the actual loop at the end
	   of this object file's text section so as to perfect
	   branch prediction.  */
	__asm__ __volatile__(
	"1:	ldq_l	%0,%1\n"
	"	blbs	%0,2f\n"
	"	or	%0,1,%0\n"
	"	stq_c	%0,%1\n"
	"	beq	%0,2f\n"
	"	mb\n"
	".section .text2,\"ax\"\n"
	"2:	ldq	%0,%1\n"
	"	blbs	%0,2b\n"
	"	br	1b\n"
	".previous"
	: "=r" (tmp), "=m" (__dummy_lock(lock))
	: "m"(__dummy_lock(lock)));
}
#endif /* DEBUG_SPINLOCK */

static inline void spin_unlock(spinlock_t * lock)
{
	mb();
	lock->lock = 0;
}

#define spin_trylock(lock) (!test_and_set_bit(0,(lock)))

#define spin_lock_irq(lock) \
  (__cli(), spin_lock(lock))
#define spin_unlock_irq(lock) \
  (spin_unlock(lock), __sti())
#define spin_lock_irqsave(lock, flags) \
  (__save_and_cli(flags), spin_lock(lock))
#define spin_unlock_irqrestore(lock, flags) \
  (spin_unlock(lock), __restore_flags(flags))

/***********************************************************/

typedef struct { volatile int write_lock:1, read_counter:31; } rwlock_t;

#define RW_LOCK_UNLOCKED { 0, 0 }

#if DEBUG_RWLOCK
extern void write_lock(rwlock_t * lock);
extern void read_lock(rwlock_t * lock);
#else
static inline void write_lock(rwlock_t * lock)
{
	long regx;

	__asm__ __volatile__(
	"1:	ldl_l	%1,%0\n"
	"	bne	%1,6f\n"
	"	or	$31,1,%1\n"
	"	stl_c	%1,%0\n"
	"	beq	%1,6f\n"
	"	mb\n"
	".section .text2,\"ax\"\n"
	"6:	ldl	%1,%0\n"
	"	bne	%1,6b\n"
	"	br	1b\n"
	".previous"
	: "=m" (__dummy_lock(lock)), "=&r" (regx)
	: "0" (__dummy_lock(lock))
	);
}

static inline void read_lock(rwlock_t * lock)
{
	long regx;

	__asm__ __volatile__(
	"1:	ldl_l	%1,%0\n"
	"	blbs	%1,6f\n"
	"	subl	%1,2,%1\n"
	"	stl_c	%1,%0\n"
	"	beq	%1,6f\n"
	"4:	mb\n"
	".section .text2,\"ax\"\n"
	"6:	ldl	%1,%0\n"
	"	blbs	%1,6b\n"
	"	br	1b\n"
	".previous"
	: "=m" (__dummy_lock(lock)), "=&r" (regx)
	: "0" (__dummy_lock(lock))
	);
}
#endif /* DEBUG_RWLOCK */

static inline void write_unlock(rwlock_t * lock)
{
	mb();
	*(volatile int *)lock = 0;
}

static inline void read_unlock(rwlock_t * lock)
{
	long regx;
	__asm__ __volatile__(
	"1:	ldl_l	%1,%0\n"
	"	addl	%1,2,%1\n"
	"	stl_c	%1,%0\n"
	"	beq	%1,6f\n"
	".section .text2,\"ax\"\n"
	"6:	br	1b\n"
	".previous"
	: "=m" (__dummy_lock(lock)), "=&r" (regx)
	: "0" (__dummy_lock(lock)));
}

#define read_lock_irq(lock)	(__cli(), read_lock(lock))
#define read_unlock_irq(lock)	(read_unlock(lock), __sti())
#define write_lock_irq(lock)	(__cli(), write_lock(lock))
#define write_unlock_irq(lock)	(write_unlock(lock), __sti())

#define read_lock_irqsave(lock, flags)	\
	(__save_and_cli(flags), read_lock(lock))
#define read_unlock_irqrestore(lock, flags) \
	(read_unlock(lock), __restore_flags(flags))
#define write_lock_irqsave(lock, flags)	\
	(__save_and_cli(flags), write_lock(lock))
#define write_unlock_irqrestore(lock, flags) \
	(write_unlock(lock), __restore_flags(flags))

#endif /* SMP */
#endif /* _ALPHA_SPINLOCK_H */
