/*
 * se.h -- definitions for the char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: se.h,v 1.15 2004/11/04 17:51:18 rubini Exp $
 */

#ifndef _SE_DRIVER_H_
#define _SE_DRIVER_H_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include "se.h"
#include "se_export.h"

/*
 * Macros to help debugging
 */

#undef PDEBUG             /* undef it, just in case */
#ifdef SE_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "se: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

#ifndef SE_MAJOR
#define SE_MAJOR 0   /* dynamic major by default */
#endif

#ifndef SE_NR_DEVS
#define SE_NR_DEVS 4    /* se0 through se3 */
#endif

#ifndef SE_P_NR_DEVS
#define SE_P_NR_DEVS 4  /* sepipe0 through sepipe3 */
#endif

/*
 * The bare device is a variable-length region of memory.
 * Use a linked list of indirect blocks.
 *
 * "se_dev->data" points to an array of pointers, each
 * pointer refers to a memory area of SE_QUANTUM bytes.
 *
 * The array (quantum-set) is SE_QSET long.
 */
#ifndef SE_QUANTUM
#define SE_QUANTUM 4000
#endif

#ifndef SE_QSET
#define SE_QSET    1000
#endif

/*
 * The pipe device is a simple circular buffer. Here its default size
 */
#ifndef SE_P_BUFFER
#define SE_P_BUFFER 4000
#endif

/*
 * Representation of se quantum sets.
 */
struct se_qset {
	void *data;
	int write_idx;
	int read_idx;
};

struct se_dev {
      void *CmdBuf;  /* Pointer to first quantum set */
	void *CmdBase;
	void *CmdLimit;
      int wrptr;
      int v_to_p_offset;
      int size;
      se_cmd_counter sw_counter;
      se_cmd_counter hw_counter;
      struct semaphore sem;     /* mutual exclusion semaphore     */
      struct semaphore empty_sem;     /* mutual exclusion semaphore     */
      struct cdev cdev;   /* Char device structure          */
};

/*
 * Split minors in two parts
 */
#define TYPE(minor)	(((minor) >> 4) & 0xf)	/* high nibble */
#define NUM(minor)	((minor) & 0xf)		/* low  nibble */


/*
 * The different configurable parameters
 */
extern int se_major;     /* main.c */
extern int se_nr_devs;
extern int se_quantum;
extern int se_qset;

extern int se_p_buffer;	/* pipe.c */


/*
 * Prototypes for shared functions
 */

int     se_p_init(dev_t dev);
void    se_p_cleanup(void);
int     se_access_init(dev_t dev);
void    se_access_cleanup(void);

int     se_trim(struct se_dev *dev);

ssize_t se_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos);
ssize_t se_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos);
loff_t  se_llseek(struct file *filp, loff_t off, int whence);
int     se_ioctl(struct inode *inode, struct file *filp,
                    unsigned int cmd, unsigned long arg);

#endif /* _SE_H_ */
