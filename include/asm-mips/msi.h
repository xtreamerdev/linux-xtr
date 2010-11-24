/*
 * Copyright (C) 2003-2004 Intel
 * Copyright (C) Tom Long Nguyen (tom.l.nguyen@intel.com)
 */

#ifndef ASM_MSI_H
#define ASM_MSI_H

#define NR_VECTORS		NR_IRQS
#define NR_IRQ_VECTORS		NR_VECTORS
#define FIRST_DEVICE_VECTOR 	2                // only irq 2 is reversed to msi
#define LAST_DEVICE_VECTOR	2                
static inline void set_intr_gate (int nr, void *func) {}
//#define IO_APIC_VECTOR(irq)	(irq)
#define ack_APIC_irq(x)		do{}while(0)       // we do not need to do this
//#define cpu_mask_to_apicid(mask) cpu_physical_id(first_cpu(mask))
#define MSI_DEST_MODE		MSI_PHYSICAL_MODE
#define MSI_TARGET_CPU	        0
#define MSI_TARGET_CPU_SHIFT	0

#define assign_irq_vector(x)    (2)
static inline int pci_vector_resources(int last, int nr_released) {return 0;}
#endif /* ASM_MSI_H */
