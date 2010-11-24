#ifndef AUTH_H
#define AUTH_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/sched.h>
#include <linux/dcache.h>
#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

//#define DEBUG_MSG

#define AUTH_MAJOR		8
#define AUTH_NAME		"auth device"

#define WATCHDOG_CTL_ADDR	0xb801b530
#define WATCHDOG_CLR_ADDR	0xb801b534

#define DEF_MAP_ADDR		0x40000000
#define MAX_MAP_ADDR		0x60000000
#define DEF_MEM_SIZE		0x10000000	// 256 MB
#define DEF_MAP_SIZE		0x30000000	// 768 MB

#define VPN2_MASK		0xffffe000

#define CACHE_LINE_LENGTH	16

typedef struct {
	int			fd;
	char			name[DNAME_INLINE_LEN_MIN];
	unsigned long		addr;
} dirty_page_struct;

#ifdef	CONFIG_REALTEK_SCHED_LOG
typedef struct {
        unsigned long           addr;
        unsigned long           size;
} sched_log_struct;

typedef char COMM_STR[TASK_COMM_LEN];
#endif

#ifdef	CONFIG_REALTEK_DETECT_OCCUPY
typedef struct {
	task_t			*task;
	unsigned long		time;
} task_occupy_info;

extern task_occupy_info        occupy_info;
extern unsigned long           occupy_interval;
#endif

extern unsigned long           dvr_asid;
extern const char              *my_swap_file;
extern struct semaphore        remap_sem;

typedef	char AUTH_STR[20];

void			my_tlb_init(void);
void			my_tlb_exit(void);
unsigned long		tlb_mmap(unsigned long addr);
unsigned long		tlb_munmap(unsigned long addr);
#ifdef CONFIG_REALTEK_PLI_DEBUG_MODE
unsigned long		pli_map_memory(unsigned long phy_addr, int size);
unsigned long		pli_unmap_memory(unsigned long vir_addr);
#endif

int auth_open (struct inode *inode, struct file *filp);
int auth_release (struct inode *inode, struct file *filp);
ssize_t auth_read (struct file *filp, char *buf, size_t count,
                   loff_t *f_pos);
ssize_t auth_write (struct file *filp, const char *buf, size_t count,
                    loff_t *f_pos);

int     auth_ioctl (struct inode *inode, struct file *filp,
                     unsigned int cmd, unsigned long arg);

void *dvr_malloc(size_t size);
void dvr_free(const void *arg);

/*
 * Ioctl definitions
 */

/* Use 'k' as magic number */
#define AUTH_IOC_MAGIC  'k'

/*
 * S means "Set"		: through a ptr,
 * T means "Tell"		: directly with the argument value
 * G means "Get"		: reply by setting through a pointer
 * Q means "Query"		: response is on the return value
 * X means "eXchange"		: G and S atomically
 * H means "sHift"		: T and Q atomically
 */
#define AUTH_IOCQMAP			_IOW(AUTH_IOC_MAGIC, 1, AUTH_STR)
#define AUTH_IOCHDUMP			_IO(AUTH_IOC_MAGIC, 2)
#define AUTH_IOCQALLOC			_IO(AUTH_IOC_MAGIC, 3)
#define AUTH_IOCQFREE			_IO(AUTH_IOC_MAGIC, 4)
#define AUTH_IOCQFREEALL		_IO(AUTH_IOC_MAGIC, 5)
#define AUTH_IOCQLISTALL		_IO(AUTH_IOC_MAGIC, 6)
#define AUTH_IOCQZONEINFO		_IO(AUTH_IOC_MAGIC, 7)
#define AUTH_IOCTDOREMAP		_IO(AUTH_IOC_MAGIC, 8)
#define AUTH_IOCQGETASID		_IO(AUTH_IOC_MAGIC, 9)

#define AUTH_IOCXDIRTYSTART		_IOWR(AUTH_IOC_MAGIC, 20, dirty_page_struct)
#define AUTH_IOCTDIRTYSTOP		_IO(AUTH_IOC_MAGIC, 21)
#define AUTH_IOCTSYNCMETADATA		_IO(AUTH_IOC_MAGIC, 22)
#define AUTH_IOCTFREEALLMEM		_IO(AUTH_IOC_MAGIC, 23)
#define AUTH_IOCTDETECTOCCUPY		_IO(AUTH_IOC_MAGIC, 24)
#define AUTH_IOCTMONITORPAGE		_IO(AUTH_IOC_MAGIC, 25)

#ifdef	CONFIG_REALTEK_SCHED_LOG
#define AUTH_IOCSINITLOGBUF		_IOW(AUTH_IOC_MAGIC, 11, sched_log_struct)
#define AUTH_IOCTFREELOGBUF		_IO(AUTH_IOC_MAGIC, 12)
#define AUTH_IOCTLOGSTART		_IO(AUTH_IOC_MAGIC, 13)
#define AUTH_IOCGLOGSTOP		_IOR(AUTH_IOC_MAGIC, 14, sched_log_struct)
#define AUTH_IOCXLOGNAME		_IOWR(AUTH_IOC_MAGIC, 15, COMM_STR)
#define AUTH_IOCSTHREADNAME		_IOW(AUTH_IOC_MAGIC, 16, COMM_STR)
#define AUTH_IOCTLOGEVENT		_IO(AUTH_IOC_MAGIC, 17)
#endif

#define AUTH_IOCTINITHWSEM		_IO(AUTH_IOC_MAGIC, 18)
#define AUTH_IOCTGETHWSEM		_IO(AUTH_IOC_MAGIC, 19)
#define AUTH_IOCTPUTHWSEM		_IO(AUTH_IOC_MAGIC, 20)

#endif // AUTH_H

