#ifndef __ASM_MIPS_CURRENT_H
#define __ASM_MIPS_CURRENT_H

/*
 * Some architectures may want to do something "clever" here since
 * this is the most frequently accessed piece of data in the entire
 * kernel.  For an example, see the Sparc implementation where an
 * entire register is hard locked to contain the value of current.
 */
extern struct task_struct *current_set[NR_CPUS];
#define current (current_set[smp_processor_id()])	/* Current on this processor */

#endif /* __ASM_MIPS_CURRENT_H */
