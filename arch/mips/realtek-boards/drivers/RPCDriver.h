#ifndef _RPCDRIVER_H
#define _RPCDRIVER_H

#define KERNEL2_6

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

#undef PDEBUG
#ifdef RPC_DEBUG
 #ifdef __KERNEL__
  #define PDEBUG(fmt, args...) printk(KERN_ALERT "RPC: " fmt, ## args)
 #else
  #define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
 #endif // __KERNEL__
#else
 #define PDEBUG(fmt, args...)	/* empty macro */
#endif // RPC_DEBUG

#undef PDEBUGG
#define PDEBUGG(fmt, args...)	/* empty macro */

#ifndef RPC_ID
#define RPC_ID 0x5566
#endif

#ifndef RPC_MAJOR
#define RPC_MAJOR 240		/* dynamic major by default */
#endif

#ifndef RPC_NR_DEVS
#define RPC_NR_DEVS 8		/* total ring buffer number */
#endif

#ifndef RPC_NR_PAIR
#define RPC_NR_PAIR 2		/* for interrupt and polling mode */
#endif

#ifndef RPC_RING_SIZE
#define RPC_RING_SIZE 512	/* size of ring buffer */
#endif

#ifndef RPC_POLL_DEV_ADDR
#define RPC_POLL_DEV_ADDR 0xa1ffe000
#endif

#ifndef RPC_INTR_DEV_ADDR
#define RPC_INTR_DEV_ADDR (RPC_POLL_DEV_ADDR+RPC_RING_SIZE)
#endif

#ifndef RPC_RECORD_SIZE
#define RPC_RECORD_SIZE 64	/* size of ring buffer record */
#endif

#ifndef RPC_POLL_RECORD_ADDR
#define RPC_POLL_RECORD_ADDR 0xa1fff000
#endif

#ifndef RPC_INTR_RECORD_ADDR
#define RPC_INTR_RECORD_ADDR (RPC_POLL_RECORD_ADDR+RPC_RECORD_SIZE*(RPC_NR_DEVS/RPC_NR_PAIR))
#endif

extern struct file_operations rpc_poll_fops;   /* for poll mode */
extern struct file_operations rpc_intr_fops;   /* for intr mode */
extern struct file_operations rpc_ctrl_fops;   /* for ctrl mode */

extern struct file_operations *rpc_fop_array[];

/*
// read : nonblocking
// write: nonblocking
*/
typedef struct RPC_POLL_Dev {
	char				*ringBuf;		/* buffer for interrupt mode */
	char				*ringStart;		/* pointer to start of ring buffer */
	char				*ringEnd;		/* pointer to end   of ring buffer */
	char				*ringIn;		/* pointer to where next data will be inserted  in   the ring buffer */
	char				*ringOut;		/* pointer to where next data will be extracted from the ring buffer */
#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
	devfs_handle_t      handle;
#endif
#endif
//	unsigned int		ringSize;		/* size of ring buffer */
	wait_queue_head_t	dummyQueue;
	struct semaphore	readSem;		/* mutual exclusion semaphore */
	struct semaphore	writeSem;		/* mutual exclusion semaphore */
//	struct semaphore	sizeSem;		/* mutual exclusion semaphore */
} RPC_POLL_Dev;

/*
// read : nonblocking blocking
// write: nonblocking
*/
typedef struct RPC_INTR_Dev {
	char				*ringBuf;		/* buffer for interrupt mode */
	char				*ringStart;		/* pointer to start of ring buffer */
	char				*ringEnd;		/* pointer to end   of ring buffer */
	char				*ringIn;		/* pointer to where next data will be inserted  in   the ring buffer */
	char				*ringOut;		/* pointer to where next data will be extracted from the ring buffer */
#ifdef CONFIG_DEVFS_FS
#ifndef KERNEL2_6
	devfs_handle_t      handle;
#endif
#endif
//	unsigned int		ringSize;		/* size of ring buffer */
	wait_queue_head_t	waitQueue;		/* for blocking read */
	struct semaphore	readSem;		/* mutual exclusion semaphore */
	struct semaphore	writeSem;		/* mutual exclusion semaphore */
//	struct semaphore	sizeSem;		/* mutual exclusion semaphore */
} RPC_INTR_Dev;

/*
 * Prototypes for shared functions
 */
int	my_copy_user(int *des, int *src, int size);

int     rpc_poll_init(void);
void    rpc_poll_cleanup(void);
int     rpc_intr_init(void);
void    rpc_intr_cleanup(void);

extern RPC_POLL_Dev *rpc_poll_devices;
extern RPC_INTR_Dev *rpc_intr_devices;

/* Use 'k' as magic number */
#define RPC_IOC_MAGIC  'k'

/*
 * S means "Set"		: through a ptr,
 * T means "Tell"		: directly with the argument value
 * G means "Get"		: reply by setting through a pointer
 * Q means "Query"		: response is on the return value
 * X means "eXchange"	: G and S atomically
 * H means "sHift"		: T and Q atomically
 */
#define RPC_IOCTTIMEOUT _IO(RPC_IOC_MAGIC,  0)
#define RPC_IOCQTIMEOUT _IO(RPC_IOC_MAGIC,  1)
#define RPC_IOCTRESET _IO(RPC_IOC_MAGIC,  2)

#endif
