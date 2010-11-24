/*
 * Copyright (C) 1999, 2000, 2004, 2005  MIPS Technologies, Inc.
 *	All rights reserved.
 *	Authors: Carsten Langgaard <carstenl@mips.com>
 *		 Maciej W. Rozycki <macro@mips.com>
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * PROM library initialisation code.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/kernel.h>

#include <asm/bootinfo.h>
//#include <asm/gt64120.h>
#include <asm/io.h>
#include <asm/system.h>

#include <prom.h>
//#include <generic.h>
//#include <bonito64.h>
//#include <msc01_pci.h>

#include <linux/compile.h>
#include <venus.h>
#include <platform.h>
#include <linux/module.h>

#ifdef CONFIG_KGDB
extern int rs_kgdb_hook(int, int);
extern int rs_putDebugChar(char);
extern char rs_getDebugChar(void);

extern int kgdb_enabled;
#endif

#define __sleep __attribute__ ((__section__ (".sleep.text")))
#define __sleepdata __attribute__ ((__section__ (".sleep.data")))

int prom_argc;
int *_prom_argv, *_prom_envp;
platform_info_t __sleepdata platform_info;
EXPORT_SYMBOL(platform_info);

/*
 * YAMON (32-bit PROM) pass arguments and environment as 32-bit pointer.
 * This macro take care of sign extension, if running in 64-bit mode.
 */
#define prom_envp(index) ((char *)(long)_prom_envp[(index)])

int init_debug = 0;

unsigned int mips_revision_corid;

/* donot use this function after "mem_init" because 0~1M are recycled in mem_init and prom_envp is located in that region. */
char *prom_getenv(char *envname)
{
	/*
	 * Return a pointer to the given environment variable.
	 * In 64-bit mode: we're using 64-bit pointers, but all pointers
	 * in the PROM structures are only 32-bit, so we need some
	 * workarounds, if we are running in 64-bit mode.
	 */
	int i, index=0;

//If debugging with jtag (that means Linux runs without bootloader), there will be no kernel parameter passed.
//return NULL;
	i = strlen(envname);

	while (prom_envp(index)) {
		if(strncmp(envname, prom_envp(index), i) == 0) {
			return(prom_envp(index+1));
		}
		index += 2;
	}

	return NULL;
}

#ifdef CONFIG_SERIAL_8250_CONSOLE
static void __init console_config(void)
{
	char console_string[40];
	int baud = 0;
	char parity = '\0', bits = '\0', flow = '\0';
	char *s;

	if ((strstr(prom_getcmdline(), "console=ttyS")) == NULL) {
		s = prom_getenv("modetty0");
		if (s) {
			while (*s >= '0' && *s <= '9')
				baud = baud*10 + *s++ - '0';
			if (*s == ',') s++;
			if (*s) parity = *s++;
			if (*s == ',') s++;
			if (*s) bits = *s++;
			if (*s == ',') s++;
			if (*s == 'h') flow = 'r';
		}
		if (baud == 0)
			baud = 38400;
		if (parity != 'n' && parity != 'o' && parity != 'e')
			parity = 'n';
		if (bits != '7' && bits != '8')
			bits = '8';
		if (flow == '\0')
			flow = 'r';
		sprintf (console_string, " console=ttyS0,%d%c%c%c", baud, parity, bits, flow);
		strcat (prom_getcmdline(), console_string);
		prom_printf("Config serial console:%s\n", console_string);
	}
}
#endif

#ifdef CONFIG_KGDB
void __init kgdb_config (void)
{
	extern int (*generic_putDebugChar)(char);
	extern char (*generic_getDebugChar)(void);
	char *argptr;
	int line, speed;

	argptr = prom_getcmdline();
	if ((argptr = strstr(argptr, "kgdb=ttyS")) != NULL) {
		argptr += strlen("kgdb=ttyS");
		if (*argptr != '0' && *argptr != '1')
			printk("KGDB: Unknown serial line /dev/ttyS%c, "
			       "falling back to /dev/ttyS1\n", *argptr);
		line = *argptr == '0' ? 0 : 1;
		printk("KGDB: Using serial line /dev/ttyS%d for session\n", line);
		speed = 0;
		if (*++argptr == ',')
		{
			int c;
			while ((c = *++argptr) && ('0' <= c && c <= '9'))
				speed = speed * 10 + c - '0';
		}
		{
			speed = rs_kgdb_hook(line, speed);
			generic_putDebugChar = rs_putDebugChar;
			generic_getDebugChar = rs_getDebugChar;
		}

		prom_printf("KGDB: Using serial line /dev/ttyS%d at %d for session, "
			    "please connect your debugger\n", line ? 1 : 0, speed);

		{
			char *s;
			for (s = "Please connect GDB to this port\r\n"; *s; )
				generic_putDebugChar (*s++);
		}

		kgdb_enabled = 1;
		/* Breakpoint is invoked after interrupts are initialised */
	}
}
#endif

/* Setup the TV encoding system passed from bootcode. */
static int __init tv_encoding_system_setup(char *str)
{
	if(str && !strcmp(str, "PAL")) {
		platform_info.tv_encoding_system = PAL;
	} else { //Others, NTSC.
		platform_info.tv_encoding_system = NTSC;
	}
	prom_printf("Setup tv_encoding_system... %d(%s)\n",
				platform_info.tv_encoding_system, str);
	return 1;
}
__setup ("tv_encoding_system=", tv_encoding_system_setup);



/* Exp: parse_variable("ethaddr", platform_info.ethaddr, 20, "00.01.02.03.04.05")
		parse_variable will read the value of the variable in bootloader.
	variable_name: The name of variable
	store_ptr: The location to store the result
	max_len: The maximum length of the result
	default_ptr: If the variable cannot be found or be read and default_ptr is not NULL, use the default_str instead.
 */
int parse_variable(const char *variable_name, char *store_ptr, int max_len, const char *default_str)
{
	char *ptr;

	ptr = prom_getenv(variable_name);
	if(ptr) {
		if(strlen(ptr) < max_len) {
			strcpy(store_ptr, ptr);
			return 0;
		}
	}
	if(default_str)
		strcpy(store_ptr, default_str);
	return -1;
}

/* 
	Exp: parse_series_variable("system_parameters_", platform_info.hdmikey, 571, 4, 1)
		system_parameters_1, system_parameters_2, and ... will be read, joined, and put in platform_info.hdmikey
	variable_name: The prefix of variable name
	store_ptr: The location to store the result
	max_len: The maximum length of the result
	level: How many variables will be read
	contiguous: If "contiguous = 1", system_parameters_2 won't be parsed if system_parameters_1 doesn't exist. If "contiguous = 0", " " will be appended between variables.
 */
int parse_series_variable(const char *variable_name, char *store_ptr, int max_len, int level, int contiguous)
{
	int i, len=0;
	char *ptr;
	char ext_variable_name[32];

	strcpy(store_ptr, "");
	if(strlen(variable_name)>32-2) {
		prom_printf("%s,%d: Variable name too long!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if(level<1 || level > 9) {
		prom_printf("%s,%d: Parameter error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	for(i=1;i<=level;i++) {
		sprintf(ext_variable_name, "%s%d", variable_name, i);
		ptr = prom_getenv(ext_variable_name);
		if(ptr) {
			len+=strlen(ptr);
			if((len < max_len-1) || (((i == 1) || contiguous) && (len < max_len))) {
				if(!contiguous && i>1) {
					strcat(store_ptr, " ");
					len += 1;
				}
				strcat(store_ptr, ptr);
			} else {
				prom_printf("%s,%d: Overflow. Skip the other data!\n", __FUNCTION__, __LINE__);
				break;
			}
		} else if(contiguous)
			break;
	}

	return 0;
}

void __init prom_init(void)
{
//unsigned char c;
	char *ptr;
	char id1[6], id2[6];

	prom_argc = fw_arg0;
	_prom_argv = (int *) fw_arg1;
	_prom_envp = (int *) fw_arg2;

//	set_io_port_base((unsigned long)ioremap_nocache(0x18010000, 0x10000));
	set_io_port_base(KSEG1ADDR(VENUS_IO_PORT_BASE));

// This will clear all RTC's interrupt.
	outl(0, VENUS_MIS_RTCCR);
//	outl(0x3E00, VENUS_MIS_ISR);
// Here we want to disable most of the interrupts of devices. If no doing this, it will have problem when entering Linux shell and the corresponding driver is not loaded.
	outl(0x3FFC, VENUS_MIS_ISR);
	outw(0x0, 0x603c);		// Disable ethernet interrupts

// These should have been set up in bootloader.	
/*	outl(0x155, 0xb000);
	outl(0x3, 0xb20c);
	outl(0x0, 0xb204);
	outl(0x0, 0xb208);
	outl(0x3, 0xb210);
	c = inl(0xb20c);
	outl( c | 0x80, 0xb20c);
	outl(0xe, 0xb200);
	outl(0x0, 0xb204);
	outl(c & ~0x80, 0xb20c);
*/

	prom_printf("\nRealtek LINUX started...\n");
	prom_printf("Venus setting:\n\tROSs have %d bytes RAM.\n", CONFIG_REALTEK_RTOS_MEMORY_SIZE);
// Turn off the interrupts of both com1 and com2 because that 1. when sending data through them from PC, System will crash; 2. when running linux and audio firmware at the same time, an unknown interrupt will be triggered on com1.
	outl(inl(VENUS_MIS_U0LCR)&~0x80, VENUS_MIS_U0LCR);
	outl(0, VENUS_MIS_U0IER_DLH);
	outl(inl(VENUS_MIS_U1LCR)&~0x80, VENUS_MIS_U1LCR);
	outl(0, VENUS_MIS_U1IER_DLH);
#ifdef CONFIG_SERIAL_8250
#ifdef CONFIG_REALTEK_SYSTEM_OWNS_SECOND_UART
	prom_printf("\tSystem CPU has 2 UARTs.\n");
#else
	prom_printf("\tSystem CPU has 1 UART.\n");
#endif
#else
	outl(inl(VENUS_MIS_U1LCR)&~0x80, VENUS_MIS_U1LCR);
	outl(0, VENUS_MIS_U1IER_DLH);
	outl(inl(VENUS_MIS_U0LCR)&~0x80, VENUS_MIS_U0LCR);
	outl(0, VENUS_MIS_U0IER_DLH);
#endif
#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
	prom_printf("\tSystem CPU uses external timer interrupt.\n");
#else
	prom_printf("\tSystem CPU uses internal timer interrupt.\n");
#endif

	sprintf(platform_info.kernel_source_code_info, "%s\n%s", LINUX_SOURCE_CODE_SVN, UTS_VERSION);
// It seems that we donot have bootup now.
//	parse_variable("uprev", platform_info.bootup_version, 4, "");
//	prom_printf("\tBootup version: %s\n", platform_info.bootup_version);
	ptr = prom_getenv("bootrev");
	if(ptr) {
		int new=0, state=0;
		unsigned int chip_id, chip_info;
		char *pos1, *pos2;
		
		strcpy(platform_info.bootloader_version, ptr);
		pos1 = strchr(ptr, '.');
		pos2 = strrchr(ptr, '.');
		if(pos1 == (ptr+2) && pos2 == (ptr+5)) {
/* Old format: 01.23.45c */
			prom_printf("\tBootloader version: %s. This version string is of old format.\n", ptr);
			strncpy(id1, ptr, 2);
			id1[2] = 0;
			strncpy(id2, ptr+3, 2);
			id2[2]=0;
		} else if(pos1 == (ptr+4) && pos2 == (ptr+9)) {
/* New format: 0001.0203.45c */
			new = 1;
			prom_printf("\tBootloader version: %s. This version string is of new format.\n", ptr);
			strncpy(id1, ptr, 4);
			id1[4] = 0;
			strncpy(id2, ptr+5, 4);
			id2[4]=0;
		} else {
			strcpy(id1, "00");
			strcpy(id2, "00");
			prom_printf("\tThe format of \"Bootloader version\" is unidentified. Reset it to default.\n");
		}
		platform_info.company_id = simple_strtol(id1, (char **)NULL, 16);
		platform_info.board_id = simple_strtol(id2, (char **)NULL, 16);
		if(!new) {
			platform_info.cpu_id = (platform_info.board_id & 0xf0) >>4;
			platform_info.board_id = (platform_info.board_id&0xf) + (platform_info.cpu_id<<8) + (platform_info.company_id<<16);
		} else {
			platform_info.cpu_id = (platform_info.board_id & 0xff00)>>8;
			platform_info.board_id += (platform_info.company_id<<16);
		}


		chip_id = inl(VENUS_SB2_CHIP_ID)&0xffff;
		chip_info = (inl(VENUS_SB2_CHIP_INFO)&0xffff0000)>>16;
		if(platform_info.cpu_id == realtek_venus_cpu) {
			if(chip_id != 0x1281)
				state = 1;
			else if(chip_info == 0xA0)
				;
			else if(chip_info == 0xA1)
				platform_info.cpu_id = realtek_venus2_cpu;
			else
				state = 1;
		} else if(platform_info.cpu_id == realtek_neptune_cpu) {
			if(chip_id != 0x1282)
				state = 1;
			else if(chip_info == 0xA0)
				;
			else
				state = 1;
		} else
			state = 1;
		if(state)
			prom_printf("\tWe donot understand the SB2_CHIP_ID or SB2_CHIP_INFO right now. Linux is too old?\n");
		prom_printf("\tThe information of this board: Company ID:0x%X    CPU ID: 0x%X    Board ID: 0x%X\n", platform_info.company_id, platform_info.cpu_id, platform_info.board_id);
	} else {
		prom_printf("\t\"Bootloader version\" is unidentified. Reset it to default.\n");
		platform_info.board_id = realtek_qa_board;
		platform_info.cpu_id = realtek_venus_cpu;
		platform_info.company_id = company_realtek;
		strcpy(platform_info.bootloader_version, "NULL");
	}
	parse_variable("ethaddr", platform_info.ethaddr, 20, "00.01.02.03.04.05");
	prom_printf("\tEthernet Mac address: %s\n", platform_info.ethaddr);
	if(!parse_variable("usb_param", platform_info.usb_param, 20, ""))
		prom_printf("\tUSB parameters: %s\n", platform_info.usb_param);
	
	parse_series_variable("hdmikey_", platform_info.hdmikey, HDMI_KEY_LEN, 6, 1);
	parse_series_variable("system_parameters_", platform_info.system_parameters, SYSTEM_PARAMETERS_LEN, 4, 0);
	parse_series_variable("signature_", platform_info.signature, 129, 2, 1);

/*
// These lines are for debugging when you encounter "irq 3: nobody cared!"
	prom_printf("Registers: VENUS_MIS_UMSK_ISR: 0x%08X\tVENUS_MIS_ISR: 0x%08X\n", inl(VENUS_MIS_UMSK_ISR), inl(VENUS_MIS_ISR));
	outl(inl(VENUS_MIS_U0LCR)&~0x80, VENUS_MIS_U0LCR);
	prom_printf("Registers: VENUS_MIS_U0IIR_FCR: 0x%08X\tVENUS_MIS_U0IER_DLH: 0x%08X\n", inl(VENUS_MIS_U0IIR_FCR), inl(VENUS_MIS_U0IER_DLH));
	outl(inl(VENUS_MIS_U1LCR)&~0x80, VENUS_MIS_U1LCR);
	prom_printf("Registers: VENUS_MIS_U1IIR_FCR: 0x%08X\tVENUS_MIS_U1IER_DLH: 0x%08X\n", inl(VENUS_MIS_U1IIR_FCR), inl(VENUS_MIS_U1IER_DLH));
*/
	prom_init_cmdline();
	prom_meminit();
#ifdef CONFIG_SERIAL_8250_CONSOLE
	console_config();
#endif
}
