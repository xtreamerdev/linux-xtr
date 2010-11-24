/*
 *  linux/init/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  GK 2/5/95  -  Changed to support mounting root fs via NFS
 *  Added initrd & change_root: Werner Almesberger & Hans Lermen, Feb '96
 *  Moan early if gcc is old, avoiding bogus kernels - Paul Gortmaker, May '96
 *  Simplified starting of init:  Michael A. Griffith <grif@acm.org> 
 */

#define __KERNEL_SYSCALLS__

#include <linux/config.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/utsname.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/smp_lock.h>
#include <linux/initrd.h>
#include <linux/hdreg.h>
#include <linux/bootmem.h>
#include <linux/tty.h>
#include <linux/gfp.h>
#include <linux/percpu.h>
#include <linux/kmod.h>
#include <linux/kernel_stat.h>
#include <linux/security.h>
#include <linux/workqueue.h>
#include <linux/profile.h>
#include <linux/rcupdate.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>
#include <linux/writeback.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/efi.h>
#include <linux/unistd.h>
#include <linux/rmap.h>
#include <linux/mempolicy.h>
#include <linux/key.h>
#include <mcp.h>
#include <linux/reboot.h>	// For machine_restart
#if CONFIG_REALTEK_TEXT_DEBUG || CONFIG_REALTEK_MEMORY_DEBUG_MODE || CONFIG_REALTEK_USER_DEBUG
#include <linux/interrupt.h>

#include <platform.h>
#include <venus.h>
#if CONFIG_REALTEK_TEXT_DEBUG || CONFIG_REALTEK_MEMORY_DEBUG_MODE
#define PREFETCH_BUFFER	0x400

extern unsigned int _etext;
extern unsigned int _edata;
extern unsigned int audio_addr;
#endif
#endif

#ifdef CONFIG_REALTEK_TEXT_DEBUG
atomic_t text_count;
#endif

#include <asm/io.h>
#include <asm/bugs.h>
#include <asm/setup.h>

/*
 * This is one of the first .c files built. Error out early
 * if we have compiler trouble..
 */
#if __GNUC__ == 2 && __GNUC_MINOR__ == 96
#ifdef CONFIG_FRAME_POINTER
#error This compiler cannot compile correctly with frame pointers enabled
#endif
#endif

#ifdef CONFIG_X86_LOCAL_APIC
#include <asm/smp.h>
#endif

/*
 * Versions of gcc older than that listed below may actually compile
 * and link okay, but the end product can have subtle run time bugs.
 * To avoid associated bogus bug reports, we flatly refuse to compile
 * with a gcc that is known to be too old from the very beginning.
 */
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 95)
#error Sorry, your GCC is too old. It builds incorrect kernels.
#endif

static int init(void *);

extern void init_IRQ(void);
extern void sock_init(void);
extern void fork_init(unsigned long);
extern void mca_init(void);
extern void sbus_init(void);
extern void sysctl_init(void);
extern void signals_init(void);
extern void buffer_init(void);
extern void pidhash_init(void);
extern void pidmap_init(void);
extern void prio_tree_init(void);
extern void radix_tree_init(void);
extern void free_initmem(void);
extern void populate_rootfs(void);
extern void driver_init(void);
extern void prepare_namespace(void);
#ifdef	CONFIG_ACPI
extern void acpi_early_init(void);
#else
static inline void acpi_early_init(void) { }
#endif

#ifdef CONFIG_TC
extern void tc_init(void);
#endif

#ifdef CONFIG_REALTEK_RESERVE_DVR
#define SYNC_STRUCT_ADDR 0xa2000000

extern int DVR_zone_disable(void);
extern int DVR_zone_enable(void);
#endif

enum system_states system_state;
EXPORT_SYMBOL(system_state);

/*
 * Boot command-line arguments
 */
#define MAX_INIT_ARGS CONFIG_INIT_ENV_ARG_LIMIT
#define MAX_INIT_ENVS CONFIG_INIT_ENV_ARG_LIMIT

extern void time_init(void);
/* Default late time init is NULL. archs can override this later. */
void (*late_time_init)(void);
extern void softirq_init(void);

/* Untouched command line (eg. for /proc) saved by arch-specific code. */
char saved_command_line[COMMAND_LINE_SIZE];

static char *execute_command;

/* Setup configured maximum number of CPUs to activate */
static unsigned int max_cpus = NR_CPUS;

#ifdef CONFIG_PRINTK
void printk_setup(void);
void printk_setup_tail(void);
#endif

#if CONFIG_REALTEK_TEXT_DEBUG || CONFIG_REALTEK_MEMORY_DEBUG_MODE || CONFIG_REALTEK_USER_DEBUG
irqreturn_t sb2_dbg_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	int itr, epc;
//	printk("sb2 dbg number %d...\n", irq);

	itr = readl((void *)0xb801a4e0);
	if (itr & 0x410) {
		int addr = readl((void *)0xb801a4c0), mask=0x3fffffff;

		// prevent the false alarm of prefetch in audio memory region...
		if (!(addr&0x80000000) && ((addr&mask) >= audio_addr) && ((addr&mask) < audio_addr+PREFETCH_BUFFER)) {
			outl(0x0000400, SB2_DGB_INT);
			return IRQ_HANDLED;
		}

		asm ("mfc0 %0, $14;": "=r"(epc));
		printk("System got interrupt from SB2...\n");
		printk("SB2_DBG_INT:  0x%08x \n", readl((void *)0xb801a4e0));
		printk("SB2_DBG_ADDR: 0x%08x \n", readl((void *)0xb801a4c0));
		printk("Access Address: 0x%x \n", epc);
		show_regs(regs);
		dump_stack();
		
		local_irq_disable();
		while (1) ;
	} else {
	        return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static struct irqaction sb2_dbg_action = {
	.handler        = sb2_dbg_isr,
	.flags          = SA_INTERRUPT | SA_SHIRQ,
	.name           = "SB2_DBG",
};

void __init sb2_dbg_init(void)
{
	if (is_venus_cpu()) {
		printk("This is not Neptune platform. The SB2 Debug facility is disabled. \n");
		return;
	}
	
        setup_irq(VENUS_INT_SB2, &sb2_dbg_action);
//	request_irq(5, sb2_dbg_isr, SA_SHIRQ | SA_INTERRUPT, "SB2_DBG", (void *)SB2_DBG_ID);
/*
	writel(0x2000000, 0xb801a458);
	writel(0x4000000, 0xb801a478);
	writel(0x00003ff, 0xb801a498);
	writel(0x0000081, 0xb801a4e0);
*/
#ifdef CONFIG_REALTEK_TEXT_DEBUG
	printk("<<<<< Enable TEXT protector >>>>>\n");
	printk("\tsrc: %x \n", (zone_table[ZONE_TEXT]->zone_start_pfn)*4096);
	printk("\tdst: %x \n", (zone_table[ZONE_TEXT]->zone_start_pfn+zone_table[ZONE_TEXT]->spanned_pages)*4096-1);
	outl((zone_table[ZONE_TEXT]->zone_start_pfn)*4096, SB2_DGB_START_REG7);
	outl((zone_table[ZONE_TEXT]->zone_start_pfn+zone_table[ZONE_TEXT]->spanned_pages)*4096-1, SB2_DGB_END_REG7);
	outl(0x0003fdf, SB2_DGB_CTRL_REG7);
	// only enable the system interrupt
	outl(0x0000081, SB2_DGB_INT);
#endif
#ifdef CONFIG_REALTEK_MEMORY_DEBUG_MODE
	printk("<<<<< Enable memory protector >>>>>\n");
	// tlb handler (avoid trashing by audio & video)
	outl(0x00000000, SB2_DGB_START_REG5);
	outl(0x0000007f, SB2_DGB_END_REG5);
	outl(0x0003e93, SB2_DGB_CTRL_REG5);
	// kernel data (avoid trashing by audio & video)
	outl(0x00100000+PREFETCH_BUFFER, SB2_DGB_START_REG6);
	outl((unsigned int)&_edata-1, SB2_DGB_END_REG6);
	outl(0x0003e93, SB2_DGB_CTRL_REG6);
	// kernel text (avoid trashing by ourself) 
	outl(0x00100000+PREFETCH_BUFFER, SB2_DGB_START_REG7);
	outl((unsigned int)&_etext-1, SB2_DGB_END_REG7);
	outl(0x00003d3, SB2_DGB_CTRL_REG7);

	// only enable the system interrupt
	outl(0x0000081, SB2_DGB_INT);
#endif
}
#endif

/*
 * Setup routine for controlling SMP activation
 *
 * Command-line option of "nosmp" or "maxcpus=0" will disable SMP
 * activation entirely (the MPS table probe still happens, though).
 *
 * Command-line option of "maxcpus=<NUM>", where <NUM> is an integer
 * greater than 0, limits the maximum number of CPUs activated in
 * SMP mode to <NUM>.
 */
static int __init nosmp(char *str)
{
	max_cpus = 0;
	return 1;
}

__setup("nosmp", nosmp);

static int __init maxcpus(char *str)
{
	get_option(&str, &max_cpus);
	return 1;
}

__setup("maxcpus=", maxcpus);

static char * argv_init[MAX_INIT_ARGS+2] = { "init", NULL, };
char * envp_init[MAX_INIT_ENVS+2] = { "HOME=/", "TERM=linux", NULL, };
static const char *panic_later, *panic_param;

extern struct obs_kernel_param __setup_start[], __setup_end[];

static int __init obsolete_checksetup(char *line)
{
	struct obs_kernel_param *p;

	p = __setup_start;
	do {
		int n = strlen(p->str);
		if (!strncmp(line, p->str, n)) {
			if (p->early) {
				/* Already done in parse_early_param?  (Needs
				 * exact match on param part) */
				if (line[n] == '\0' || line[n] == '=')
					return 1;
			} else if (!p->setup_func) {
				printk(KERN_WARNING "Parameter %s is obsolete,"
				       " ignored\n", p->str);
				return 1;
			} else if (p->setup_func(line + n))
				return 1;
		}
		p++;
	} while (p < __setup_end);
	return 0;
}

/*
 * This should be approx 2 Bo*oMips to start (note initial shift), and will
 * still work even if initially too large, it will just take slightly longer
 */
unsigned long loops_per_jiffy = (1<<12);

EXPORT_SYMBOL(loops_per_jiffy);

static int __init debug_kernel(char *str)
{
	if (*str)
		return 0;
	console_loglevel = 10;
	return 1;
}

static int __init quiet_kernel(char *str)
{
	if (*str)
		return 0;
	console_loglevel = 4;
	return 1;
}

__setup("debug", debug_kernel);
__setup("quiet", quiet_kernel);

static int __init loglevel(char *str)
{
	get_option(&str, &console_loglevel);
	return 1;
}

__setup("loglevel=", loglevel);

/*
 * Unknown boot options get handed to init, unless they look like
 * failed parameters
 */
static int __init unknown_bootoption(char *param, char *val)
{
	/* Change NUL term back to "=", to make "param" the whole string. */
	if (val) {
		/* param=val or param="val"? */
		if (val == param+strlen(param)+1)
			val[-1] = '=';
		else if (val == param+strlen(param)+2) {
			val[-2] = '=';
			memmove(val-1, val, strlen(val)+1);
			val--;
		} else
			BUG();
	}

	/* Handle obsolete-style parameters */
	if (obsolete_checksetup(param))
		return 0;

	/*
	 * Preemptive maintenance for "why didn't my mispelled command
	 * line work?"
	 */
	if (strchr(param, '.') && (!val || strchr(param, '.') < val)) {
		printk(KERN_ERR "Unknown boot option `%s': ignoring\n", param);
		return 0;
	}

	if (panic_later)
		return 0;

	if (val) {
		/* Environment option */
		unsigned int i;
		for (i = 0; envp_init[i]; i++) {
			if (i == MAX_INIT_ENVS) {
				panic_later = "Too many boot env vars at `%s'";
				panic_param = param;
			}
			if (!strncmp(param, envp_init[i], val - param))
				break;
		}
		envp_init[i] = param;
	} else {
		/* Command line option */
		unsigned int i;
		for (i = 0; argv_init[i]; i++) {
			if (i == MAX_INIT_ARGS) {
				panic_later = "Too many boot init vars at `%s'";
				panic_param = param;
			}
		}
		argv_init[i] = param;
	}
	return 0;
}

static int __init init_setup(char *str)
{
	unsigned int i;

	execute_command = str;
	/*
	 * In case LILO is going to boot us with default command line,
	 * it prepends "auto" before the whole cmdline which makes
	 * the shell think it should execute a script with such name.
	 * So we ignore all arguments entered _before_ init=... [MJ]
	 */
	for (i = 1; i < MAX_INIT_ARGS; i++)
		argv_init[i] = NULL;
	return 1;
}
__setup("init=", init_setup);

/* Now we only support secure boot on partition /dev/mtdblock/1. If /dev/mtdblock/1 has been modified after installation, Linux kernel will know that and do reboot. */
char partition_hash_value[24]="";	// This hash value is encrypted by aes.
static int __init partition_hash(char *hash_str)
{
	char *hash_tmp_ptr, *endptr;
	char strtol_tmp[3];
	int i;

	if(strncmp(hash_str, "1:", 2))
		return 0;
	*(unsigned int*)(partition_hash_value+4)=simple_strtol(hash_str+2, &endptr, 10);
	if(endptr == hash_str+2)
		return 0;
	if(*endptr != ':')
		return 0;
	hash_tmp_ptr = endptr+1;
	if(strlen(hash_tmp_ptr)!=32)
		return 0;
	strtol_tmp[2]=0;
	for(i=0;i<16;i++) {
		strncpy(strtol_tmp, hash_tmp_ptr+2*i, 2);
		*(unsigned char*)(partition_hash_value+8+i) = simple_strtol(strtol_tmp, &endptr, 16);
		if(*endptr)
			return 0;
	}
	partition_hash_value[0]='1';
	return 1;
}

__setup("partition_hash=", partition_hash);

extern void setup_arch(char **);

#ifndef CONFIG_SMP

#ifdef CONFIG_X86_LOCAL_APIC
static void __init smp_init(void)
{
	APIC_init_uniprocessor();
}
#else
#define smp_init()	do { } while (0)
#endif

static inline void setup_per_cpu_areas(void) { }
static inline void smp_prepare_cpus(unsigned int maxcpus) { }

#else

#ifdef __GENERIC_PER_CPU
unsigned long __per_cpu_offset[NR_CPUS];

EXPORT_SYMBOL(__per_cpu_offset);

static void __init setup_per_cpu_areas(void)
{
	unsigned long size, i;
	char *ptr;
	/* Created by linker magic */
	extern char __per_cpu_start[], __per_cpu_end[];

	/* Copy section for each CPU (we discard the original) */
	size = ALIGN(__per_cpu_end - __per_cpu_start, SMP_CACHE_BYTES);
#ifdef CONFIG_MODULES
	if (size < PERCPU_ENOUGH_ROOM)
		size = PERCPU_ENOUGH_ROOM;
#endif

	ptr = alloc_bootmem(size * NR_CPUS);

	for (i = 0; i < NR_CPUS; i++, ptr += size) {
		__per_cpu_offset[i] = ptr - __per_cpu_start;
		memcpy(ptr, __per_cpu_start, __per_cpu_end - __per_cpu_start);
	}
}
#endif /* !__GENERIC_PER_CPU */

/* Called by boot processor to activate the rest. */
static void __init smp_init(void)
{
	unsigned int i;

	/* FIXME: This should be done in userspace --RR */
	for_each_present_cpu(i) {
		if (num_online_cpus() >= max_cpus)
			break;
		if (!cpu_online(i))
			cpu_up(i);
	}

	/* Any cleanup work */
	printk(KERN_INFO "Brought up %ld CPUs\n", (long)num_online_cpus());
	smp_cpus_done(max_cpus);
#if 0
	/* Get other processors into their bootup holding patterns. */

	smp_commence();
#endif
}

#endif

/*
 * We need to finalize in a non-__init function or else race conditions
 * between the root thread and the init thread may cause start_kernel to
 * be reaped by free_initmem before the root thread has proceeded to
 * cpu_idle.
 *
 * gcc-3.4 accidentally inlines this function, so use noinline.
 */

static void noinline rest_init(void)
	__releases(kernel_lock)
{
	kernel_thread(init, NULL, CLONE_FS | CLONE_SIGHAND);
	numa_default_policy();
	unlock_kernel();
	preempt_enable_no_resched();
	cpu_idle();
} 

/* Check for early params. */
static int __init do_early_param(char *param, char *val)
{
	struct obs_kernel_param *p;

	for (p = __setup_start; p < __setup_end; p++) {
		if (p->early && strcmp(param, p->str) == 0) {
			if (p->setup_func(val) != 0)
				printk(KERN_WARNING
				       "Malformed early option '%s'\n", param);
		}
	}
	/* We accept everything at this stage. */
	return 0;
}

/* Arch code calls this early on, or if not, just before other parsing. */
void __init parse_early_param(void)
{
	static __initdata int done = 0;
	static __initdata char tmp_cmdline[COMMAND_LINE_SIZE];

	if (done)
		return;

	/* All fall through to do_early_param. */
	strlcpy(tmp_cmdline, saved_command_line, COMMAND_LINE_SIZE);
	parse_args("early options", tmp_cmdline, NULL, 0, do_early_param);
	done = 1;
}

/*
 *	Activate the first processor.
 */

asmlinkage void __init start_kernel(void)
{
	char * command_line;
	extern struct kernel_param __start___param[], __stop___param[];
/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */
#ifdef CONFIG_PRINTK
#ifdef CONFIG_SHARED_PRINTK
	*(volatile int *)0xb801a000=0;
#endif
	printk_setup();
#endif
	lock_kernel();
	page_address_init();
	printk(KERN_NOTICE);
	printk(linux_banner);
	setup_arch(&command_line);
	setup_per_cpu_areas();

	/*
	 * Mark the boot cpu "online" so that it can call console drivers in
	 * printk() and can access its per-cpu storage.
	 */
	smp_prepare_boot_cpu();

	/*
	 * Set up the scheduler prior starting any interrupts (such as the
	 * timer interrupt). Full topology setup happens at smp_init()
	 * time - but meanwhile we still have a functioning scheduler.
	 */
	sched_init();
	/*
	 * Disable preemption - early bootup scheduling is extremely
	 * fragile until we cpu_idle() for the first time.
	 */
	preempt_disable();
	build_all_zonelists();
	page_alloc_init();
	printk(KERN_NOTICE "Kernel command line: %s\n", saved_command_line);
	parse_early_param();
	parse_args("Booting kernel", command_line, __start___param,
		   __stop___param - __start___param,
		   &unknown_bootoption);
	sort_main_extable();
	trap_init();
	rcu_init();
	init_IRQ();
	pidhash_init();
	init_timers();
	softirq_init();
	time_init();

#if CONFIG_REALTEK_TEXT_DEBUG || CONFIG_REALTEK_MEMORY_DEBUG_MODE || CONFIG_REALTEK_USER_DEBUG
	sb2_dbg_init();
#endif
	/*
	 * HACK ALERT! This is early. We're enabling the console before
	 * we've done PCI setups etc, and console_init() must be aware of
	 * this. But we do want output early, in case something goes wrong.
	 */
	console_init();
	if (panic_later)
		panic(panic_later, panic_param);
	profile_init();
	local_irq_enable();
#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start && !initrd_below_start_ok &&
			initrd_start < min_low_pfn << PAGE_SHIFT) {
		printk(KERN_CRIT "initrd overwritten (0x%08lx < 0x%08lx) - "
		    "disabling it.\n",initrd_start,min_low_pfn << PAGE_SHIFT);
		initrd_start = 0;
	}
#endif
	vfs_caches_init_early();
	mem_init();
#ifdef CONFIG_REALTEK_RESERVE_DVR
	/* reserve the DVR zone for boot-up audio & video */
	if (DVR_zone_disable()) {
		printk("reserve DVR zone...\n");
	}
#endif
	kmem_cache_init();
	numa_policy_init();
	if (late_time_init)
		late_time_init();
	calibrate_delay();
	pidmap_init();
	pgtable_cache_init();
	prio_tree_init();
	anon_vma_init();
#ifdef CONFIG_X86
	if (efi_enabled)
		efi_enter_virtual_mode();
#endif
	fork_init(num_physpages);
	proc_caches_init();
	buffer_init();
	unnamed_dev_init();
	key_init();
	security_init();
	vfs_caches_init(num_physpages);
	radix_tree_init();
	signals_init();
	/* rootfs populating might need page-writeback */
	page_writeback_init();
#ifdef CONFIG_PROC_FS
	proc_root_init();
#endif
	cpuset_init();

	check_bugs();

	acpi_early_init(); /* before LAPIC and SMP init */

	/* Do the rest non-__init'ed, we're now alive */
	rest_init();
}

static int __initdata initcall_debug;

static int __init initcall_debug_setup(char *str)
{
	initcall_debug = 1;
	return 1;
}
__setup("initcall_debug", initcall_debug_setup);

struct task_struct *child_reaper = &init_task;

extern initcall_t __initcall_start[], __initcall_end[];

static void __init do_initcalls(void)
{
	initcall_t *call;
	int count = preempt_count();

	for (call = __initcall_start; call < __initcall_end; call++) {
		char *msg;

		if (initcall_debug) {
			printk(KERN_DEBUG "Calling initcall 0x%p", *call);
			print_fn_descriptor_symbol(": %s()", (unsigned long) *call);
			printk("\n");
		}

		(*call)();

		msg = NULL;
		if (preempt_count() != count) {
			msg = "preemption imbalance";
			preempt_count() = count;
		}
		if (irqs_disabled()) {
			msg = "disabled interrupts";
			local_irq_enable();
		}
		if (msg) {
			printk(KERN_WARNING "error in initcall at 0x%p: "
				"returned with %s\n", *call, msg);
		}
	}

	/* Make sure there is no pending stuff from the initcall sequence */
	flush_scheduled_work();
}

/*
 * Ok, the machine is now initialized. None of the devices
 * have been touched yet, but the CPU subsystem is up and
 * running, and memory and process management works.
 *
 * Now we can finally start doing some real work..
 */
static void __init do_basic_setup(void)
{
	/* drivers will send hotplug events */
	init_workqueues();
	usermodehelper_init();
	driver_init();

#ifdef CONFIG_SYSCTL
	sysctl_init();
#endif

	/* Networking initialization needs a process context */ 
	sock_init();

	do_initcalls();
}

static void do_pre_smp_initcalls(void)
{
	extern int spawn_ksoftirqd(void);
#ifdef CONFIG_SMP
	extern int migration_init(void);

	migration_init();
#endif
	spawn_ksoftirqd();
}

static void run_init_process(char *init_filename)
{
	argv_init[0] = init_filename;
	execve(init_filename, argv_init, envp_init);
}

static inline void fixup_cpu_present_map(void)
{
#ifdef CONFIG_SMP
	int i;

	/*
	 * If arch is not hotplug ready and did not populate
	 * cpu_present_map, just make cpu_present_map same as cpu_possible_map
	 * for other cpu bringup code to function as normal. e.g smp_init() etc.
	 */
	if (cpus_empty(cpu_present_map)) {
		for_each_cpu(i) {
			cpu_set(i, cpu_present_map);
		}
	}
#endif
}

unsigned char hash_buffer[512*1024];
static int init(void * unused)
{
#ifdef CONFIG_REALTEK_RESERVE_DVR
	int aflag = 0, vflag = 0;
	char *ptr = (char *)SYNC_STRUCT_ADDR;
#endif

	lock_kernel();
	/*
	 * init can run on any cpu.
	 */
	set_cpus_allowed(current, CPU_MASK_ALL);
	/*
	 * Tell the world that we're going to be the grim
	 * reaper of innocent orphaned children.
	 *
	 * We don't want people to have to make incorrect
	 * assumptions about where in the task array this
	 * can be found.
	 */
	child_reaper = current;

	/* Sets up cpus_possible() */
	smp_prepare_cpus(max_cpus);

	do_pre_smp_initcalls();

	fixup_cpu_present_map();
	smp_init();
	sched_init_smp();

	cpuset_init_smp();

	/*
	 * Do this before initcalls, because some drivers want to access
	 * firmware files.
	 */
	populate_rootfs();

	do_basic_setup();

#ifdef CONFIG_PRINTK
	printk_setup_tail();
#endif
	/*
	 * check if there is an early userspace init.  If yes, let it do all
	 * the work
	 */
	if (sys_access((const char __user *) "/init", 0) == 0)
		execute_command = "/init";
	else
		prepare_namespace();

	/*
	 * Ok, we have completed the initial bootup, and
	 * we're essentially up and running. Get rid of the
	 * initmem segments and start the user-mode stuff..
	 */
	free_initmem();
	unlock_kernel();
	system_state = SYSTEM_RUNNING;
	numa_default_policy();

	if (sys_open((const char __user *) "/dev/console", O_RDWR, 0) < 0)
		printk(KERN_WARNING "Warning: unable to open an initial console.\n");

	(void) sys_dup(0);
	(void) sys_dup(0);
	
#ifdef CONFIG_REALTEK_RESERVE_DVR
	while (1) {
		if (!aflag) {
			// check audio flag...
			if (ptr[21]) {
				aflag = 1;
				printk("*****audio is ready...\n");
			}
		}
		if (!vflag) {
			// check video flag...
			if (ptr[20]) {
				vflag = 1;
				printk("*****video is ready...\n");
			}
		}

		if (aflag && vflag)
			break;

		msleep(100);
	}
	DVR_zone_enable();
#endif

/* Here we check the hash value of some partition, which is passed from bootloader, to make sure that partition is not modified.
    The format of that bootloader variable is partition_hash="[partition num]:[partition size]:[hash value]". Exp: go 0x80100000 ... partition_hash="1:1234:12341234123412341234123412341234" */
#ifdef CONFIG_REALTEK_SECURE_BOOT_PARTITION
	if(partition_hash_value[0] == '1') {
		unsigned char hash_array[16];
		int fd=sys_open("/dev/mtdblock/1", O_RDONLY, 0);
		int partitionsize, imagesize, hashcount;
		int firstblock;
		int i;
		
		if(fd<0) {
			printk(KERN_ALERT "Error! sys_open cannot open /dev/mtdblock/1.\n");
			machine_restart("Secure boot error!");
		}
		partitionsize = sys_lseek(fd, 0, 2);
		sys_lseek(fd, 0, 0);
		imagesize=*(unsigned int*)(partition_hash_value+4);
		firstblock=1;
		for(hashcount=0; imagesize>hashcount;) {
			int singlecount;
			int len;

			if((imagesize-hashcount)>=512*1024)
				singlecount = 512*1024;
			else
				singlecount = imagesize-hashcount;
			len = sys_read(fd, hash_buffer, singlecount);
			if(len != singlecount) {
				printk(KERN_ALERT "Error! sys_read didn't return the number of chars we expected.\n");
				sys_close(fd);
				machine_restart("Secure boot error!");
			}
			MCP_AES_H_DataHash(hash_buffer, singlecount, hash_array, 512*1024, firstblock);
			firstblock=0;
			hashcount+=singlecount;
		}
		MCP_AES_ECB_Encryption(NULL, hash_array, hash_array, 16);

		for(i=0;i<4;i++) {
			if(*(unsigned int*)(hash_array+4*i) != *(unsigned int*)(partition_hash_value+8+4*i)) {
				printk(KERN_ALERT "Error! Hash value of partition /dev/mtdblock/1 is not matched. That partition data in Nand Flash may be modified or damaged.\n");
				sys_close(fd);
				machine_restart("Secure boot error!");
			}
		}
		
		sys_lseek(fd, hashcount&(0-512*1024), 0);
		while(partitionsize>hashcount) {
			int len = sys_read(fd, hash_buffer, 512*1024);
			for(i=(hashcount&(512*1024-1));i<len;i++) {
				if(hash_buffer[i] != 0xff) {
					printk(KERN_ALERT "Error! Hash value of partition /dev/mtdblock/1 is not matched. That partition data in Nand Flash may be modified or damaged.\n");
					sys_close(fd);
					machine_restart("Secure boot error!");
				}
			}
			hashcount+=(len-(hashcount&(512*1024-1)));
		}
			
		printk("Secure boot succeeded. The partition /dev/mtdblock/1 is secure.\n");
		sys_close(fd);
	}
#endif

	/*
	 * We try each of these until one succeeds.
	 *
	 * The Bourne shell can be used instead of init if we are 
	 * trying to recover a really broken machine.
	 */

	if (execute_command)
		run_init_process(execute_command);

	run_init_process("/sbin/init");
	run_init_process("/etc/init");
	run_init_process("/bin/init");
	run_init_process("/bin/sh");

	panic("No init found.  Try passing init= option to kernel.");
}
