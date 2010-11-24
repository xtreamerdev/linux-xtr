#include <linux/init.h>
#include <linux/swap.h>
#include <linux/file.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#include <linux/auth.h>
#include <linux/interrupt.h>
#include <venus.h>

MODULE_LICENSE("Dual BSD/GPL");

int auth_major = AUTH_MAJOR;

const char		*dvr_auth = "Realtek DVR";
unsigned long		map_start_addr;
struct radix_tree_root	dvr_tree;
spinlock_t              dvr_tree_lock;
struct address_space	*as_record_dirty = NULL;
unsigned long		*vm_record_dirty = NULL;

// variable declaration
extern long		swap_name_addr;
extern unsigned long	dvr_asid;

const char		*my_swap_file = "/mnt/rd/swap.img";
#ifdef	CONFIG_REALTEK_SCHED_LOG
unsigned int		*buf_head = NULL;
unsigned int		*buf_tail = NULL;
unsigned int		*buf_ptr = NULL;
unsigned int		sched_log_flag = 0;
#endif
#ifdef	CONFIG_REALTEK_DETECT_OCCUPY
task_occupy_info	occupy_info;
unsigned long		occupy_interval = 0;
#endif
#ifdef	CONFIG_REALTEK_MONITOR_PAGE
unsigned long		monitor_pfn = 0;
#endif
#ifdef	CONFIG_REALTEK_WATCHDOG
unsigned int		watchdog_task_addr = 0;
#endif

// function declaration
void print_pagevec_count(void);		// in mm/swap.c...
void start_remap(void);			// in mm/pageremap.c...
void my_print_tlb_content(void);
void free_all_memory(void);

// constant declaration
const int	pli_signature = 0x77000000;
const int	max_buddy_size = 1 << (12+MAX_ORDER-1);
const int	buddy_id = 0x10000;
const int	max_cache_size = 1 << 12;
const int	cache_id = 0x20000;

static int record_insert(int type, unsigned long addr)
{
	int *ptr;
	int error;
	
	ptr = kmalloc(8, GFP_KERNEL);
	ptr[0] = type;
	ptr[1] = addr;
	addr = (addr&0x0fffffff) >> 2;	// use WORD as minimum unit
	error = radix_tree_preload(GFP_KERNEL);
	if (error == 0) {
		spin_lock_irq(&dvr_tree_lock);
		error = radix_tree_insert(&dvr_tree, addr, ptr);
		if (error) {
			printk("radix tree insert error...\n");
			kfree((void *)ptr);
		}
		spin_unlock_irq(&dvr_tree_lock);
		radix_tree_preload_end();
	}

	return error;
}

static int record_lookup(unsigned long addr)
{
	int *ptr, ret = 0;

	addr = (addr&0x0fffffff) >> 2;	// use WORD as minimum unit
	spin_lock_irq(&dvr_tree_lock);
	ptr = radix_tree_lookup(&dvr_tree, addr);
	if (ptr)
		ret = *ptr;
	else
		printk("radix tree lookup error...\n");
	spin_unlock_irq(&dvr_tree_lock);

	return ret;
}

static void record_delete(unsigned long addr)
{
	int *ptr;

	addr = (addr&0x0fffffff) >> 2;	// use WORD as minimum unit
	spin_lock_irq(&dvr_tree_lock);
	ptr = radix_tree_delete(&dvr_tree, addr);
	spin_unlock_irq(&dvr_tree_lock);
	if (ptr)
		kfree((void *)ptr);
	else
		printk("radix tree delete error...\n");
}

static inline void __record_delete(unsigned long addr)
{
	int *ptr;

	addr = (addr&0x0fffffff) >> 2;	// use WORD as minimum unit
	ptr = radix_tree_delete(&dvr_tree, addr);
	if (ptr)
		kfree((void *)ptr);
	else
		printk("radix tree delete error...\n");
}

static void data_cache_flush(unsigned long addr, int size)
{
	for ( ; size > 0; size -= CACHE_LINE_LENGTH) {
		__asm__ __volatile__ ("cache 0x15, (%0);": :"r"(addr));
		addr += 16;
	}
}

static void list_all_memory_allocation(void)
{
	const int record_number = 10;
	int **pRecords = NULL;
	int tmp, cnt, start_index = 0;
	
	pRecords = (int **)kmalloc(4*record_number, GFP_KERNEL);
	
#ifdef DEBUG_MSG
	printk("Start to show memory allocated...\n");
#endif
	spin_lock_irq(&dvr_tree_lock);
	while (1) {
		tmp = radix_tree_gang_lookup(&dvr_tree,
			(void **)pRecords, start_index, record_number);
		for (cnt = 0; cnt < tmp; cnt++)
			printk(" allocation address: %x \n", pRecords[cnt][1]);
		
		if (tmp != record_number)
			break;
		start_index = pRecords[record_number-1][1];
		start_index = ((start_index&0x0fffffff) >> 2) + 1;
	}
	spin_unlock_irq(&dvr_tree_lock);
	
	kfree(pRecords);
}

static void free_all_memory_allocation(void)
{
	const int record_number = 10;
	int **pRecords = NULL;
	int tmp, cnt, start_index = 0;
	
	pRecords = (int **)kmalloc(4*record_number, GFP_KERNEL);
	
#ifdef DEBUG_MSG
	printk("Start to flush memory...\n");
#endif
	spin_lock_irq(&dvr_tree_lock);
	while (1) {
		tmp = radix_tree_gang_lookup(&dvr_tree,
			(void **)pRecords, start_index, record_number);
		for (cnt = 0; cnt < tmp; cnt++) {
			printk(" release address: %x \n", pRecords[cnt][1]);
			if ((pRecords[cnt][0] & 0x00ff0000) == cache_id) {
				kfree((void *)pRecords[cnt][1]);
			} else {
#ifdef	CONFIG_REALTEK_ADVANCED_RECLAIM
				int order = pRecords[cnt][0] & 0x000000ff;
				int count = pRecords[cnt][0] & 0x0000ff00;
				int addr = pRecords[cnt][1];
				struct page *page;

				count >>= 8;
				if (count != 0) {
					int i;

//					printk("free %d order...\n", count);
					for (i = 1; i <= count; i++) {
//						printk("free address: %x, order: %d \n", addr, order-i);
						if (i != 1) {
							page = virt_to_page((void *)addr);
							atomic_inc(&page->_count);
						}
						free_pages((unsigned long)addr, order-i);
						addr += (1 << (12+order-i));
					}
				} else {
					free_pages((unsigned long)addr, order);
				}
#else
				free_pages((unsigned long)pRecords[cnt][1], pRecords[cnt][0] & 0x000000ff);
#endif
			}
			start_index = pRecords[cnt][1];
			__record_delete(pRecords[cnt][1]);
		}
		
		if (tmp != record_number)
			break;
		start_index = ((start_index&0x0fffffff) >> 2) + 1;
	}
	spin_unlock_irq(&dvr_tree_lock);

	kfree(pRecords);
}

static int cal_order(int num)
{
	int cnt = 0, val = 1 << 12;

	while (num > val) {
		val <<= 1;
		cnt++;
	}

	return cnt;
}

static int do_auth(const char *str)
{
	AUTH_STR buf;
	int i = 0;
	
	copy_from_user(buf, str, sizeof(AUTH_STR));
	
	while (1) {
		if (dvr_auth[i] == buf[i]) {
			if (dvr_auth[i] == 0)
				return 1;
		} else
			return 0;
		i++;
	}
}

static void show_buddy_info(void)
{
	pg_data_t *pgdat = &contig_page_data;
	struct zone *zone;
	struct zone *node_zones = pgdat->node_zones;
	unsigned long flags;
	int order;
	long cached;

	for (zone = node_zones; zone - node_zones < MAX_NR_ZONES; ++zone) {
		if (!zone->present_pages)
			continue;

		spin_lock_irqsave(&zone->lock, flags);
		printk("Node %d, zone %8s ", pgdat->node_id, zone->name);
		for (order = 0; order < MAX_ORDER; ++order)
			printk("%6lu ", zone->free_area[order].nr_free);
		printk("\nfree pages:%6lu\n", zone->free_pages);
		spin_unlock_irqrestore(&zone->lock, flags);
	}
	cached = get_page_cache_size() - total_swapcache_pages - nr_blockdev_pages();
	printk("cached size:%6lu\n", cached);
}

static void show_zone_info(void)
{
	struct zone *zone;
	int cnt = 0;

	for (zone = contig_page_data.node_zones; zone; zone = next_zone(zone)) {
		struct per_cpu_pageset *pset;
		int i;
		printk(KERN_WARNING "   zone%d\tfree size:%d\tpresent size:%d\n", cnt++, (int)zone->free_pages, (int)zone->present_pages);
		printk(KERN_WARNING "   min:%d\tlow:%d\t\thigh:%d\n", (int)zone->pages_min, (int)zone->pages_low, (int)zone->pages_high);
		printk(KERN_WARNING "   active:%d\tinactive:%d\n", (int)zone->nr_active, (int)zone->nr_inactive);

		pset = &zone->pageset[0];
		for (i = 0; i < ARRAY_SIZE(pset->pcp); i++) {
			struct per_cpu_pages *pcp;		
			pcp = &pset->pcp[i];
			printk(KERN_WARNING "   per_cpu_pages[%d]: %d\n", i, (int)pcp->count);
		}
	}
	print_pagevec_count();
}

struct file_operations auth_fops = {
//    llseek:     scull_llseek,
    read:       auth_read,
    write:      auth_write,
    ioctl:      auth_ioctl,
    open:       auth_open,
    release:    auth_release,
};

int auth_open(struct inode *inode, struct file *filp)
{
	filp->private_data = 0;
	return 0;
}

int auth_release(struct inode *inode, struct file *filp)
{
	unsigned long errno = 0;

	if ((unsigned long)filp->private_data == map_start_addr) {
		errno = tlb_munmap(map_start_addr);
		if (errno & 0x80000000)
			printk(KERN_WARNING "   Error code %d\n", (int)-errno);
	} else {
		printk("   error in release pli interface...\n");
	}

	return 0;
}

ssize_t auth_read(struct file *filp, char *buf, size_t count,
                loff_t *f_pos)
{
	return 0;
}

ssize_t auth_write(struct file *filp, const char *buf, size_t count,
                loff_t *f_pos)
{
	return 0;
}

int auth_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	static struct file *dvr_proc = NULL;
	int ret = 0, *ptr;
	int order, value;
	unsigned long errno = 0;
#ifdef	CONFIG_REALTEK_SCHED_LOG
	struct task_struct *task;
	sched_log_struct log_struct;
	unsigned long cause;
#endif
#ifdef DEBUG_MSG
	struct mm_struct *mm = current->mm;
#endif

	if ((cmd != AUTH_IOCQMAP) && (filp != dvr_proc)) {
		printk( "   no authorization...\n");
		return ret;
	}

	switch(cmd) {
		case AUTH_IOCQMAP:
			if (do_auth((const char *)arg))
				dvr_proc = filp;
			else
				return ret;

			// free all memory allocated previously
//			free_all_memory_allocation();

#ifdef DEBUG_MSG
			printk(KERN_WARNING "   memory count: %d\n", mm->map_count);
#endif
			map_start_addr = DEF_MAP_ADDR;
			while (map_start_addr < MAX_MAP_ADDR) {
				errno = tlb_mmap(map_start_addr);
				if (errno & 0x80000000)
					printk(KERN_WARNING "   Error code %d\n", (int)-errno);
				else {
					map_start_addr = errno;
					printk(KERN_WARNING "   Memory address 0x%x\n", (int)map_start_addr);
					break;
				}
				
				map_start_addr += DEF_MAP_SIZE;
			}

#ifdef DEBUG_MSG
			printk(KERN_WARNING "   memory count: %d\n", mm->map_count);
#endif
			if (errno & 0x80000000)
				return ret;					// return with error
			else {
				filp->private_data = (void *)map_start_addr;
#ifdef CONFIG_REALTEK_ENABLE_ADE
				current->thread.mflags &= ~MF_FIXADE;
#endif
				return map_start_addr;				// return normally
			}

		case AUTH_IOCHDUMP:
			// 'arg' contains physical memory address
			if (arg < DEF_MEM_SIZE) {
				ptr = (int*)((arg | 0x80000000) & 0xfffffffc);
				ret = *ptr;
			} else {
#ifdef DEBUG_MSG
				printk(KERN_WARNING "   error physical address 0x%x\n", (int)arg);
#endif
				return ret;
			}
			
#ifdef DEBUG_MSG
//			printk(KERN_WARNING "   address: 0x%x, value: 0x%x\n", (int)ptr, ret);
#endif
			return ret;

		case AUTH_IOCQALLOC:
			if (arg <= max_cache_size) {
				ret = (int)kmalloc(arg, GFP_KERNEL);
				if (!ret) {
					show_buddy_info();
					swap_name_addr = (long)my_swap_file;
					start_remap();
					ret = (int)kmalloc(arg, GFP_KERNEL);
					if (!ret)
						show_buddy_info();
				}
				if (ret) {
					if (record_insert(pli_signature | cache_id, (unsigned long)ret)) {
						kfree((void *)ret);
						return 0;
					}
					data_cache_flush(ret, arg);
				} else {
					return ret;
				}
			} else {
				order = cal_order(arg);
				ret = __get_free_pages(GFP_DVRUSER | __GFP_NOWARN | __GFP_EXHAUST | __GFP_HUGEFREE, order);
				if (!ret) {
					show_buddy_info();
					swap_name_addr = (long)my_swap_file;
					start_remap();
					ret = __get_free_pages(GFP_DVRUSER | __GFP_NOWARN | __GFP_EXHAUST | __GFP_HUGEFREE, order);
					if (!ret)
						show_buddy_info();
				}
				if (ret) {
#ifdef	CONFIG_REALTEK_ADVANCED_RECLAIM
					int diff;
					int tmp, cnt = 0;
					struct page *page;

					diff = (1 << (12+order)) - arg;
					if (diff >= 0x10000) {
						printk("=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*\n");
						printk("req: %x, get: %x \n", arg, 1 << (12+order));
						cnt = 1;
						tmp = (1 << (12+order-cnt));
						while (tmp < arg) {
//							printk("tmp value: %x \n", tmp);
							cnt++;
							tmp += (1 << (12+order-cnt));
						}
//						printk("tmp value: %x \n", tmp);
//						printk("cnt value: %d \n", cnt);
						page = virt_to_page((void *)ret+tmp);
						atomic_inc(&page->_count);
						free_pages(ret+tmp, order-cnt);
						printk("reclaim size: %x \n", 1 << (12+order-cnt));
						printk("=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*\n");
					}

					if (record_insert(pli_signature | buddy_id | (cnt << 8) | order, (unsigned long)ret)) {
#else
					if (record_insert(pli_signature | buddy_id | order, (unsigned long)ret)) {
#endif
						free_pages((unsigned long)ret, order);
						return 0;
					}
					data_cache_flush(ret, arg);
				} else {
					return ret;
				}
			}
			return (ret&0x0fffffff)+map_start_addr;

		case AUTH_IOCQFREE:
			arg = (arg-map_start_addr)|0x80000000;
			ret = 0;
			value = record_lookup((unsigned long)arg);
			if ((value & 0xff000000) != pli_signature) {
				ret = 1;
			} else {
				record_delete((unsigned long)arg);
				if ((value & 0x00ff0000) == cache_id) {
					kfree((void *)arg);
				} else {
#ifdef	CONFIG_REALTEK_ADVANCED_RECLAIM
					int cnt = value & 0x0000ff00;
					struct page *page;

					order = value & 0x000000ff;
					cnt >>= 8;
					if (cnt != 0) {
						int i;

//						printk("free %d order...\n", cnt);
						for (i = 1; i <= cnt; i++) {
//							printk("free address: %x, order: %d \n", arg, order-i);
							if (i != 1) {
								page = virt_to_page((void *)arg);
								atomic_inc(&page->_count);
							}
							free_pages((unsigned long)arg, order-i);
							arg += (1 << (12+order-i));
						}
					} else {
						free_pages((unsigned long)arg, order);
					}
#else
					order = value & 0x000000ff;
					free_pages((unsigned long)arg, order);
#endif
				}
			}
			return ret;

		case AUTH_IOCQFREEALL:
			free_all_memory_allocation();
			break;

		case AUTH_IOCTDETECTOCCUPY:
#ifdef	CONFIG_REALTEK_DETECT_OCCUPY
			occupy_interval = arg;
			occupy_info.task = current;
			occupy_info.time = jiffies;
#else
			printk("***This function is not supported***\n");
#endif
			break;

		case AUTH_IOCTMONITORPAGE:
#ifdef	CONFIG_REALTEK_MONITOR_PAGE
			{
				pgd_t   *pgd;
				pud_t   *pud;
				pmd_t   *pmd;
				pte_t   *pte;

				pgd = pgd_offset(current->mm, arg);
				pud = pud_offset(pgd, arg);
				pmd = pmd_offset(pud, arg);
				pte = pte_offset(pmd, arg);

				if (pte->pte == 0) {
					printk("error in get page frame...\n");
					ret = 0;
				} else {
//					printk("pfn value: %d \n", pte->pte >> PAGE_SHIFT);
					monitor_pfn = pte->pte >> PAGE_SHIFT;
					ret = monitor_pfn;
				}
			}
#else
			printk("***This function is not supported***\n");
#endif
			break;

		case AUTH_IOCQLISTALL:
			list_all_memory_allocation();
			break;

		case AUTH_IOCQZONEINFO:
			show_zone_info();
			printk(" get context value: 0x%x\n", (int)current->mm->context[0]);
			my_print_tlb_content();
			break;
			
		case AUTH_IOCTDOREMAP:
			swap_name_addr = arg;
			start_remap();
			break;
			
		case AUTH_IOCQGETASID:
			ret = dvr_asid;
			break;

		case AUTH_IOCXDIRTYSTART:
			{
				struct file *file;
				int fput_needed;
				dirty_page_struct dirty_struct;
		
				ret = copy_from_user(&dirty_struct, arg, sizeof(dirty_struct));
				if (ret)
					break;
				file = fget_light(dirty_struct.fd, &fput_needed);
				if (file) {
					as_record_dirty = file->f_dentry->d_inode->i_mapping;
					vm_record_dirty = (unsigned long *)((dirty_struct.addr-map_start_addr)|0x80000000);
					fput_light(file, fput_needed);
					copy_to_user(arg+4, file->f_dentry->d_iname, DNAME_INLINE_LEN_MIN);
				} else {
					printk("error file descriptor...\n");
					copy_to_user(arg+4, "NULL", 5);
					ret = 1;
				}
			}
			break;

		case AUTH_IOCTDIRTYSTOP:
			as_record_dirty = NULL;
			vm_record_dirty = NULL;
			break;

		case AUTH_IOCTSYNCMETADATA:
			{
				struct super_block *sb;
				struct file *file;
				int fput_needed;
				
				file = fget_light(arg, &fput_needed);
				if (file) {
					sb = file->f_dentry->d_inode->i_sb;
					fput_light(file, fput_needed);

					lock_super(sb);
					if (sb->s_dirt && sb->s_op->write_super)
						sb->s_op->write_super(sb);
					unlock_super(sb);
					if (sb->s_op->sync_fs)
						sb->s_op->sync_fs(sb, 1);
					sync_blockdev(sb->s_bdev);
					sync_inodes_sb(sb, 1);
					printk("sync super %s...\n", sb->s_type->name);
				} else {
					printk("error file descriptor...\n");
					ret = 1;
				}
			}
			break;

		case AUTH_IOCTFREEALLMEM:
			free_all_memory();
			break;

#ifdef	CONFIG_REALTEK_SCHED_LOG
		case AUTH_IOCSINITLOGBUF:
			ret = copy_from_user(&log_struct, arg, sizeof(log_struct));
			if (!ret) {
				buf_head = (unsigned long *)(log_struct.addr-map_start_addr+0x80000000);
				buf_tail = (unsigned long *)(log_struct.addr+log_struct.size-map_start_addr+0x80000000);
				buf_ptr = buf_head;
				printk("buf_head: %x, buf_tail: %x, buf_ptr: %x...\n", (int)buf_head, (int)buf_tail, (int)buf_ptr);
				sched_log_flag = 0;
			}
			break;

		case AUTH_IOCTFREELOGBUF:
			buf_head = NULL;
			buf_tail = NULL;
			buf_ptr = NULL;
			sched_log_flag = 0;
			break;

		case AUTH_IOCTLOGSTART:
			buf_ptr = buf_head;
			// enable count register
			asm ("mfc0 %0, $13;": "=r"(cause));
			cause &= 0xf7ffffff;
			asm ("mtc0 %0, $13;": : "r"(cause));
			sched_log_flag = 1;
			break;

		case AUTH_IOCGLOGSTOP:
			if (!(sched_log_flag & 0x1))
				break;
			sched_log_flag &= ~0x1;
			// disable count register
			asm ("mfc0 %0, $13;": "=r"(cause));
			cause |= 0x08000000;
			asm ("mtc0 %0, $13;": : "r"(cause));
			log_struct.addr = ((unsigned long)buf_ptr-0x80000000+map_start_addr);
			log_struct.size = sched_log_flag;
			copy_to_user(arg, &log_struct, sizeof(log_struct));
			break;

		case AUTH_IOCXLOGNAME:
			get_user(ret, (int *)arg);
//			printk("pid value: %d \n", ret);
			task = find_task_by_pid(ret);
			if (task != NULL) {
//				printk("task comm name: %s \n", task->comm);
				copy_to_user(arg, task->comm, TASK_COMM_LEN);
			} else {
//				printk("can't find task struct...\n");
				copy_to_user(arg, "NULL", 5);
			}
			break;

		case AUTH_IOCSTHREADNAME:
			value = strlen((const char *)arg);
			if (value >= TASK_COMM_LEN) {
				copy_from_user(current->comm, arg, TASK_COMM_LEN-1);
				current->comm[TASK_COMM_LEN-1] = '\0';
			} else {
				copy_from_user(current->comm, arg, value);
				current->comm[value] = '\0';
			}
			break;

		case AUTH_IOCTLOGEVENT:
			if (!(sched_log_flag & 0x1))
				break;

			printk("logging event ***%d*** \n", arg);
			log_event(arg);
			break;
#endif

		default:  /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}
	
	return ret;
}

#ifdef	CONFIG_REALTEK_WATCHDOG
/* return value: 1 => success, 0 => failure */
int kill_watchdog()
{
	if (watchdog_task_addr != 0) {
		force_sig_specific(SIGKILL, (struct task_struct *)watchdog_task_addr);
		return 1;
	} else {
		printk("no watchdog exist...\n");
		return 0;
	}
}

/*
 * define RTC_DEBUG can turn on the RTC debug facility to catch a long interval reaction in kernel
 * reboot facility will be unworkable.
 */
//#define RTC_DEBUG

#ifdef RTC_DEBUG
irqreturn_t rtc_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	char timeout_message[84];
	int i, j, mesg_len;

 	if(inl(VENUS_MIS_ISR) & 0x200) {
		outl(0x200, VENUS_MIS_ISR);
		/* When the first interrupt of half second comes, VENUS_MIS_RTCSEC is 0. */
		if(inl(VENUS_MIS_RTCSEC)%2) {
			sprintf(timeout_message, "Linux is too busy to reset RTC for 1 second. EPC:%x\n", read_c0_epc());
			mesg_len = strlen(timeout_message);
			for(i=0;i<mesg_len;i++) {
				outl(timeout_message[i], VENUS_MIS_U0RBR_THR_DLL);
				j=10000000;
				while(j>0) j--;
			}
		}
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}
#endif

int config_watchdog(void *p)
{
	struct sched_param param;

#ifdef RTC_DEBUG
	if (request_irq(7, rtc_isr, SA_SHIRQ, "rtc", (void *)rtc_isr) != 0)
		printk("can't register it...\n");
#endif
	daemonize("watchdog");
	param.sched_priority = 99;
	sched_setscheduler(current, SCHED_FIFO, &param);

	/* wait boot to complete */
	msleep(5000);

	printk("************************************\n");
	printk("*********watchdog mechanism*********\n");
	printk("************************************\n");
	/* enable watchdog */
#ifdef RTC_DEBUG
	outl(0x40, VENUS_MIS_RTCCR);
	outl(0x1, VENUS_MIS_RTCCR);
#else
	writel(0x20000000, (void *)WATCHDOG_CTL_ADDR);
#endif
	watchdog_task_addr = (unsigned long)current;
	while (1) {
#ifdef RTC_DEBUG
		msleep_interruptible(200);
		outl(0x40, VENUS_MIS_RTCCR);
		outl(0x1, VENUS_MIS_RTCCR);
#else
		msleep_interruptible(1000);
		writel(0x1, (void *)WATCHDOG_CLR_ADDR);
#endif
		if (current->flags & PF_FREEZE) {
			/* disable watchdog */
			writel(0x1e0000a5, (void *)WATCHDOG_CTL_ADDR);

			refrigerator(PF_FREEZE);

			/* enable watchdog */
			writel(0x20000000, (void *)WATCHDOG_CTL_ADDR);
		}
		if (signal_pending(current)) {
			printk("watchdog got terminating signal...\n");
			watchdog_task_addr = 0;
			flush_signals(current);
			break;
		}
	}
}
#endif

static int auth_init(void)
{
	int result;
	
#ifdef CONFIG_DEVFS_FS
	/* create /dev/auth to put files in there */
	devfs_mk_dir("auth");
#endif

	result = register_chrdev(auth_major, AUTH_NAME, &auth_fops);
	if (result < 0) {
		printk(KERN_WARNING "   auth: can't get major %d\n", auth_major);
		return result;
	}

#ifdef CONFIG_DEVFS_FS
	// Create corresponding node in device file system
	devfs_mk_cdev(MKDEV(auth_major, 0), S_IFCHR|S_IRUSR|S_IWUSR, "auth/%d", 0);
#endif

	my_tlb_init();
	
	// Initialize the radix tree used to record memory allocation
	INIT_RADIX_TREE(&dvr_tree, GFP_ATOMIC);
	spin_lock_init(&dvr_tree_lock);
	
#ifdef CONFIG_REALTEK_WATCHDOG
	kernel_thread(config_watchdog, NULL, CLONE_KERNEL);
#endif

	return 0; /* success */
}

static void auth_exit(void)
{
	my_tlb_exit();

#ifdef CONFIG_DEVFS_FS
	devfs_remove("auth");
#endif

	unregister_chrdev(auth_major, AUTH_NAME);
}

module_init(auth_init);
module_exit(auth_exit);

