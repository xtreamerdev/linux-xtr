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

extern char corepath[100];

int add_dvrfs_buffer(unsigned int addr, unsigned int size);
int free_dvrfs_buffer();

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
	struct pt_regs		*regs;

	task = find_task_by_pid(pid);
	if (task == NULL) {
		printk("Task %d doesn't exist...\n", pid);
		return;
	}
	regs = (struct pt_regs *)(((unsigned char *)task->thread_info)+THREAD_SIZE-32-sizeof(struct pt_regs));

	printk("task name: %s, priority: %d (epc == %x, ra == %x) \n", task->comm, task->prio, 
			(unsigned long)regs->cp0_epc, (unsigned long)regs->regs[31]);
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

static ssize_t update_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "%d \n", platform_info.update_mode);
}

static ssize_t update_store(struct subsystem *subsys, char *page, size_t count)
{
	char *p, buffer[20] = {0};
	int i, len, mode;

	p = memchr(page, '\n', count);
	len = p ? p - page : count;
	strncpy(buffer, page, len);
	for (i = 0; i < len; i++)
		if (!isdigit(buffer[i])) {
			printk("%c is not digit...\n", buffer[i]);
			return count;
		}
	sscanf(buffer, "%d", &mode);
	platform_info.update_mode = mode;

	return count;
}
REALTEK_BOARDS_ATTR_RW(update);

static ssize_t dvrfs_buffer_show(struct subsystem *subsys, char *page)
{
	return sprintf(page, "Please add command as parameter...\n");
}

static ssize_t dvrfs_buffer_store(struct subsystem *subsys, char *page, size_t count)
{
	char *p, buffer[30] = {0};
	int len, cmd;
	unsigned int addr = 0, size = 0;

	p = memchr(page, '\n', count);
	len = p ? p - page : count;
	strncpy(buffer, page, len);
	sscanf(buffer, "%d %x %x", &cmd, &addr, &size);
	if (cmd == 1) {
		printk("Add %x %x into dvrfs buffer...\n", addr, size);
		add_dvrfs_buffer(addr, size);
	} else if (cmd == 2) {
		printk("Free dvrfs buffer...\n");
		free_dvrfs_buffer();
	}

	return count;
}
REALTEK_BOARDS_ATTR_RW(dvrfs_buffer);

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
	} else if(len>0 && !strncmp(page, "2", 1)) {
		// set the default location of core dump...
		if(len>1 && !strncmp(page+1, " ", 1)) {
			strcpy(corepath, page+2);
			*strchr(corepath, '\n') = 0;
			printk("set corepath: %s \n", corepath);
		} else {
			printk("Error, you must specify the location...\n");
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

static ssize_t AES_CCMP_show(struct subsystem *subsys, char *page)
{
	if(platform_info.AES_CCMP_len>0) {
		memcpy(page, platform_info.AES_CCMP, platform_info.AES_CCMP_len);
		return platform_info.AES_CCMP_len;
	} else
		return 0;
}
REALTEK_BOARDS_ATTR_RO(AES_CCMP);

static ssize_t modelconfig_show(struct subsystem *subsys, char *page)
{
       if(platform_info.modelconfig_len>0) {
               memcpy(page, platform_info.modelconfig, platform_info.modelconfig_len);
               return platform_info.modelconfig_len;
       } else
               return 0;
}
REALTEK_BOARDS_ATTR_RO(modelconfig);

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
	&AES_CCMP_attr.attr,
	&signature_attr.attr,
	&modelconfig_attr.attr,
	&update_attr.attr,
	&dvrfs_buffer_attr.attr,
	NULL
};

static struct attribute_group realtek_boards_attr_group = {
	.attrs = realtek_boards_attrs,
};





/* This section is for updating logo image and parameters. */
#define LOGO_TARGET_ADDRESS	0xa00f0000
typedef struct logo_param {
	u32	logo_target_addr;
	u32	logo_len;
	u32	logo_reg_5370;
	u32	logo_reg_5374;
	u32	logo_reg_5378;
	u32	logo_reg_537c;
} logo_param_t;


static DECLARE_MUTEX(logo_param_lock);
static struct kobject logo_param_kobj;

logo_param_t logo_param = {
	.logo_target_addr	= LOGO_TARGET_ADDRESS,
	.logo_len		= 0,
	.logo_reg_5370		= 0,
	.logo_reg_5374		= 0,
	.logo_reg_5378		= 0,
	.logo_reg_537c		= 0,
};

static ssize_t logo_param_read(struct kobject *kobj,
			char *buffer, loff_t offset, size_t count) {
	ssize_t ret_count = 0;
	int len;

	down(&logo_param_lock);
	if(offset > 0) {
		ret_count = 0;
		goto out;
	}

	logo_param.logo_reg_5370 = *(volatile unsigned int *)0xb8005370;
	logo_param.logo_reg_5374 = *(volatile unsigned int *)0xb8005374;
	logo_param.logo_reg_5378 = *(volatile unsigned int *)0xb8005378;
	logo_param.logo_reg_537c = *(volatile unsigned int *)0xb800537c;
	logo_param.logo_len = (*(volatile unsigned int *)0xb8005534) - 0xf0000;

	memset(buffer, 0, count);
	len = snprintf(buffer, count-1,
				"target, length, reg5370, reg5374, reg5378, reg537c\n");
	len = snprintf(buffer+len, count-len-1, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
				logo_param.logo_target_addr,
				logo_param.logo_len,
				logo_param.logo_reg_5370,
				logo_param.logo_reg_5374,
				logo_param.logo_reg_5378,
				logo_param.logo_reg_537c);
	ret_count = strlen(buffer);

out:
	up(&logo_param_lock);

	return ret_count;
}

static void update_logo_param(logo_param_t *logo_param) {
	*(volatile unsigned int *)0xb8005370 = logo_param->logo_reg_5370;
	*(volatile unsigned int *)0xb8005374 = logo_param->logo_reg_5374;
	*(volatile unsigned int *)0xb8005378 = logo_param->logo_reg_5378;
	*(volatile unsigned int *)0xb800537c = logo_param->logo_reg_537c;
	*(volatile unsigned int *)0xb8005534 = 0xf0000 + logo_param->logo_len;
	return;
}

static ssize_t logo_param_write(struct kobject *kobj,
			char *buffer, loff_t offset, size_t count) {
	char *p;
	ssize_t ret_count = 0;

	down(&logo_param_lock);
	if((p = strsep(&buffer, " ")) == NULL) {
		ret_count = 0;
		goto out;
	}
	//logo_param.logo_target_addr = simple_strtoll(p, NULL, 0);

	if((p = strsep(&buffer, " ")) == NULL) {
		ret_count = 0;
		goto out;
	}
	logo_param.logo_len = simple_strtoll(p, NULL, 0);

	if((p = strsep(&buffer, " ")) == NULL) {
		ret_count = 0;
		goto out;
	}
	logo_param.logo_reg_5370 = simple_strtoll(p, NULL, 0);

	if((p = strsep(&buffer, " ")) == NULL) {
		ret_count = 0;
		goto out;
	}
	logo_param.logo_reg_5374 = simple_strtoll(p, NULL, 0);

	if((p = strsep(&buffer, " ")) == NULL) {
		ret_count = 0;
		goto out;
	}
	logo_param.logo_reg_5378 = simple_strtoll(p, NULL, 0);

	if((p = strsep(&buffer, " ")) == NULL) {
		ret_count = 0;
		goto out;
	}
	logo_param.logo_reg_537c = simple_strtoll(p, NULL, 0);

	ret_count = count;


	update_logo_param(&logo_param);

out:
	up(&logo_param_lock);
	return ret_count;
}

static struct bin_attribute logo_param_attr = {
	.attr = {
		.name = "param",
		.mode = 0644,
		.owner = THIS_MODULE,
	},
	.size = 0,
	.read = logo_param_read,
	.write = logo_param_write,
};

static ssize_t logo_image_read(struct kobject *kobj,
			char *buffer, loff_t offset, size_t count) {
	return 0;
}

static ssize_t logo_image_write(struct kobject *kobj,
			char *buffer, loff_t offset, size_t count) {
	ssize_t ret_count = 0;
	unsigned int *p = (unsigned int*)(logo_param.logo_target_addr+(u32)offset);

	down(&logo_param_lock);

	//printk("%s:%d p=%p, offset=%u, count=%u\n",
	//		__FUNCTION__, __LINE__, p, (u32)offset, count);
	memcpy(p, buffer, count);
	ret_count = count;

	up(&logo_param_lock);
	return ret_count;
}

static struct bin_attribute logo_img_attr = {
	.attr = {
		.name = "image",
		.mode = 0644,
		.owner = THIS_MODULE,
	},
	.size = 0,
	.read = logo_image_read,
	.write = logo_image_write,
};

static void logo_param_release(struct kobject *kobj) {
	return;
}

/*
static ssize_t logo_param_show(struct kobject * kobj,
			struct attribute * attr, char * buffer) {
	return 0;
}

static ssize_t logo_param_store(struct kobject * kobj,
			struct attribute * attr, const char * buffer, size_t count) {
	return 0;
}
*/


static struct sysfs_ops logo_param_sysfs_ops = {
/*
        .show = logo_param_show,
        .store = logo_param_store,
*/
};

static struct kobj_type logo_param_ktype = {
	.sysfs_ops = &logo_param_sysfs_ops,
	.release = logo_param_release,
};

static int __init realtek_sysfs_logo_param_init(struct kobject *parent_kobj) {
	struct kobject *kobj;
	int err = 0;

	kobj = &logo_param_kobj;
	kobject_set_name(kobj, "logo_param");
	kobj->ktype = &logo_param_ktype;
	kobj->parent = parent_kobj;
	kobj->kset = NULL;
	if((err = kobject_register(kobj))) {
		return err;
	}

	if((err = sysfs_create_bin_file(kobj, &logo_param_attr))) {
		return err;
	}

	if((err = sysfs_create_bin_file(kobj, &logo_img_attr))) {
		return err;
	}

	return err;
}

static int __init realtek_boards_sysfs_init(void)
{
	int error = subsystem_register(&realtek_boards_subsys);

	if (!error)
		error = sysfs_create_group(&realtek_boards_subsys.kset.kobj,
					   &realtek_boards_attr_group);

	realtek_sysfs_logo_param_init(&realtek_boards_subsys.kset.kobj);

//	platform_info.vout_interface[0] = 0;

	return error;
}


arch_initcall(realtek_boards_sysfs_init);
