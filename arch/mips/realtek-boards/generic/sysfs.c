/*
 * arch/mips/realtek-boards/generic/sysfs.c - sysfs attributes in /sys/realtek_boards
 *
 * Copyright (C) 2006 Colin <colin@realtek.com.tw>
 * 
 */

#include <linux/config.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <platform.h>

#ifdef CONFIG_REALTEK_SHOW_STACK 
void show_trace(struct task_struct *task, unsigned long *stack);
#endif

#define REALTEK_BOARDS_ATTR_RO(_name) \
static struct subsys_attribute _name##_attr = __ATTR_RO(_name)

#define REALTEK_BOARDS_ATTR_RW(_name) \
static struct subsys_attribute _name##_attr = \
	__ATTR(_name, 0644, _name##_show, _name##_store)

static ssize_t kernel_source_code_info_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%s\n", platform_info.kernel_source_code_info);
}
REALTEK_BOARDS_ATTR_RO(kernel_source_code_info);

static ssize_t bootup_version_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%s\n", platform_info.bootup_version);
}
REALTEK_BOARDS_ATTR_RO(bootup_version);

static ssize_t bootloader_version_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%s\n", platform_info.bootloader_version);
}
REALTEK_BOARDS_ATTR_RO(bootloader_version);

static ssize_t board_id_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%x\n", platform_info.board_id);
}
REALTEK_BOARDS_ATTR_RO(board_id);

static ssize_t cpu_id_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%x\n", platform_info.cpu_id);
}
REALTEK_BOARDS_ATTR_RO(cpu_id);

static ssize_t tv_encoding_system_show(struct subsystem *subsys, char *page)
{
	unsigned char *name = NULL;
	switch(platform_info.tv_encoding_system) {
	case PAL:
		name = "PAL";
		break;
	case NTSC:
	default:
		name = "NTSC";
		break;
	}
	return sprintf(page, "%x (%s)\n", platform_info.tv_encoding_system, name);
}
REALTEK_BOARDS_ATTR_RO(tv_encoding_system);

#ifdef CONFIG_REALTEK_SHOW_STACK
static void show_task_stack(int pid)
{
	struct task_struct	*task;
	unsigned long		stack;

	task = find_task_by_pid(pid);
	if (task == NULL) {
		printk("Task %d doesn't exist...\n", pid);
		return;
	}

	printk("task name: %s, priority: %d \n", task->comm, task->prio);
	show_trace(task, NULL);
}

static ssize_t task_stack_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "Please add thread id as parameter...\n");
}

static ssize_t task_stack_store(struct subsystem *subsys, char *page, size_t count)
{
	char *p, buffer[20] = {0};
	int i, len, pid;

	p = memchr(page, '\n', count);
	len = p ? p - page : count;
	strncpy(buffer, page, len);
	for (i = 0; i < len; i++)
		if (!isdigit(buffer[i])) {
			printk("%c is not digit...\n", buffer[i]);
			return count;
		}
	sscanf(buffer, "%d", &pid);
	printk("pid value: %d \n", pid);
	show_task_stack(pid);

	return count;
}
REALTEK_BOARDS_ATTR_RW(task_stack);
#endif

static ssize_t misc_operations_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "0\n");
}

// We use "extern prom_printf" here because when printk is disabled, prom_printf will be defined as null if "prom.h" is included
extern void prom_printf(char *fmt, ...);

static ssize_t misc_operations_store(struct subsystem *subsys, char *page, size_t count)
{
	char *p;
	int len;

	p = memchr(page, '\n', count);
	len = p ? p - page : count;

	if(len>0 && !strncmp(page, "1", 1)) {
		if(len>1 && !strncmp(page+1, " ", 1))
			printk("This is the current time: %s", page+2);
		else {
		printk("--------- %d\n", len);
			printk("This is the current time.\n");
		}
	}
	return count;
}
REALTEK_BOARDS_ATTR_RW(misc_operations);

/*
static ssize_t vout_interface_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%d \n", (int)platform_info.vout_interface[0]);
}

static ssize_t vout_interface_store(struct subsystem *subsys, char *page, size_t count)
{
	char *p;
	int len;

	p = memchr(page, '\n', count);
	if (!p)
		return count;
	len = p - page;
	if (!strncmp(page, "0", len)) {
		printk("[SYS] setting vout to component...\n");
		sscanf(page, "%d", &platform_info.vout_interface[0]);
	} else if (!strncmp(page, "1", len)) {
		printk("[SYS] setting vout to SCART...\n");
		sscanf(page, "%d", &platform_info.vout_interface[0]);
	} else {
		printk("[SYS] Unknown vout setting...\n");
	}

	return count;
}
REALTEK_BOARDS_ATTR_RW(vout_interface);
*/

static ssize_t system_parameters_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%s\n", platform_info.system_parameters);
}
REALTEK_BOARDS_ATTR_RO(system_parameters);

static ssize_t hdmikey_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%s\n", platform_info.hdmikey);
}
REALTEK_BOARDS_ATTR_RO(hdmikey);

static ssize_t signature_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%s\n", platform_info.signature);
}
REALTEK_BOARDS_ATTR_RO(signature);

decl_subsys(realtek_boards, NULL, NULL);
EXPORT_SYMBOL(realtek_boards_subsys);

static struct attribute * realtek_boards_attrs[] = {
	&kernel_source_code_info_attr.attr,
	&bootup_version_attr.attr,
	&bootloader_version_attr.attr,
	&board_id_attr.attr,
	&cpu_id_attr.attr,
	&tv_encoding_system_attr.attr,
#ifdef CONFIG_REALTEK_SHOW_STACK
	&task_stack_attr.attr,
#endif
	&misc_operations_attr.attr,
//	&vout_interface_attr.attr,
	&system_parameters_attr.attr,
	&hdmikey_attr.attr,
	&signature_attr.attr,
	NULL
};

static struct attribute_group realtek_boards_attr_group = {
	.attrs = realtek_boards_attrs,
};

static int __init realtek_boards_sysfs_init(void)
{
	int error = subsystem_register(&realtek_boards_subsys);
	if (!error)
		error = sysfs_create_group(&realtek_boards_subsys.kset.kobj,
					   &realtek_boards_attr_group);

//	platform_info.vout_interface[0] = 0;

	return error;
}

arch_initcall(realtek_boards_sysfs_init);
