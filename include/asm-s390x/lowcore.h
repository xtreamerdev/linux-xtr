
/*
 *  include/asm-s390/lowcore.h
 *
 *  S390 version
 *    Copyright (C) 1999 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *    Author(s): Hartmut Penner (hpenner@de.ibm.com),
 *               Martin Schwidefsky (schwidefsky@de.ibm.com),
 *               Denis Joseph Barrow (djbarrow@de.ibm.com,barrow_dj@yahoo.com)
 */

#ifndef _ASM_S390_LOWCORE_H
#define _ASM_S390_LOWCORE_H

#define __LC_EXT_OLD_PSW                0x0130
#define __LC_SVC_OLD_PSW                0x0140
#define __LC_PGM_OLD_PSW                0x0150
#define __LC_MCK_OLD_PSW                0x0160
#define __LC_IO_OLD_PSW                 0x0170
#define __LC_EXT_NEW_PSW                0x01b0
#define __LC_SVC_NEW_PSW                0x01c0
#define __LC_PGM_NEW_PSW                0x01d0
#define __LC_MCK_NEW_PSW                0x01e0
#define __LC_IO_NEW_PSW                 0x01f0
#define __LC_EXT_PARAMS                 0x080
#define __LC_CPU_ADDRESS                0x084
#define __LC_EXT_INT_CODE               0x086
#define __LC_SVC_ILC                    0x088
#define __LC_SVC_INT_CODE               0x08A
#define __LC_PGM_ILC                    0x08C
#define __LC_PGM_INT_CODE               0x08E
#define __LC_TRANS_EXC_ADDR             0x0a8
#define __LC_SUBCHANNEL_ID              0x0B8
#define __LC_SUBCHANNEL_NR              0x0BA
#define __LC_IO_INT_PARM                0x0BC
#define __LC_IO_INT_WORD                0x0C0
#define __LC_MCCK_CODE                  0x0E8

#define __LC_RETURN_PSW                 0x200
#define __LC_IRB			0x210
#define __LC_DIAG44_OPCODE		0x250

#define __LC_SAVE_AREA                  0xC00
#define __LC_KERNEL_STACK               0xD40
#define __LC_ASYNC_STACK                0xD48
#define __LC_CPUID                      0xD90
#define __LC_CPUADDR                    0xD98
#define __LC_IPLDEV                     0xDB8

#define __LC_JIFFY_TIMER		0xDC0

#define __LC_PANIC_MAGIC                0xE00

#define __LC_AREGS_SAVE_AREA            0x1340
#define __LC_CREGS_SAVE_AREA            0x1380

#define __LC_PFAULT_INTPARM             0x11B8

#ifndef __ASSEMBLY__

#include <linux/config.h>
#include <asm/processor.h>
#include <linux/types.h>
#include <asm/atomic.h>
#include <asm/sigp.h>

void restart_int_handler(void);
void ext_int_handler(void);
void system_call(void);
void pgm_check_handler(void);
void mcck_int_handler(void);
void io_int_handler(void);

struct _lowcore
{
        /* prefix area: defined by architecture */
	__u32        ccw1[2];                  /* 0x000 */
	__u32        ccw2[4];                  /* 0x008 */
	__u8         pad1[0x80-0x18];          /* 0x018 */
	__u32        ext_params;               /* 0x080 */
	__u16        cpu_addr;                 /* 0x084 */
	__u16        ext_int_code;             /* 0x086 */
        __u16        svc_ilc;                  /* 0x088 */
        __u16        svc_code;                 /* 0x08a */
        __u16        pgm_ilc;                  /* 0x08c */
        __u16        pgm_code;                 /* 0x08e */
	__u32        data_exc_code;            /* 0x090 */
	__u16        mon_class_num;            /* 0x094 */
	__u16        per_perc_atmid;           /* 0x096 */
	addr_t       per_address;              /* 0x098 */
	__u8         exc_access_id;            /* 0x0a0 */
	__u8         per_access_id;            /* 0x0a1 */
	__u8         op_access_id;             /* 0x0a2 */
	__u8         ar_access_id;             /* 0x0a3 */
	__u8         pad2[0xA8-0xA4];          /* 0x0a4 */
	addr_t       trans_exc_code;           /* 0x0A0 */
	addr_t       monitor_code;             /* 0x09c */
	__u16        subchannel_id;            /* 0x0b8 */
	__u16        subchannel_nr;            /* 0x0ba */
	__u32        io_int_parm;              /* 0x0bc */
	__u32        io_int_word;              /* 0x0c0 */
	__u8         pad3[0xc8-0xc4];          /* 0x0c4 */
	__u32        stfl_fac_list;            /* 0x0c8 */
	__u8         pad4[0xe8-0xcc];          /* 0x0cc */
	__u32        mcck_interruption_code[2]; /* 0x0e8 */
	__u8         pad5[0xf4-0xf0];          /* 0x0f0 */
	__u32        external_damage_code;     /* 0x0f4 */
	addr_t       failing_storage_address;  /* 0x0f8 */
	__u8         pad6[0x120-0x100];        /* 0x100 */
	psw_t        restart_old_psw;          /* 0x120 */
	psw_t        external_old_psw;         /* 0x130 */
	psw_t        svc_old_psw;              /* 0x140 */
	psw_t        program_old_psw;          /* 0x150 */
	psw_t        mcck_old_psw;             /* 0x160 */
	psw_t        io_old_psw;               /* 0x170 */
	__u8         pad7[0x1a0-0x180];        /* 0x180 */
	psw_t        restart_psw;              /* 0x1a0 */
	psw_t        external_new_psw;         /* 0x1b0 */
	psw_t        svc_new_psw;              /* 0x1c0 */
	psw_t        program_new_psw;          /* 0x1d0 */
	psw_t        mcck_new_psw;             /* 0x1e0 */
	psw_t        io_new_psw;               /* 0x1f0 */
        psw_t        return_psw;               /* 0x200 */
	__u8	     irb[64];		       /* 0x210 */
	__u32        diag44_opcode;            /* 0x250 */
        __u8         pad8[0xc00-0x254];        /* 0x254 */
        /* System info area */
	__u64        save_area[16];            /* 0xc00 */
        __u8         pad9[0xd40-0xc80];        /* 0xc80 */
 	__u64        kernel_stack;             /* 0xd40 */
	__u64        async_stack;              /* 0xd48 */
	/* entry.S sensitive area start */
	__u8         pad10[0xd80-0xd50];       /* 0xd64 */
	struct       cpuinfo_S390 cpu_data;    /* 0xd80 */
	__u32        ipl_device;               /* 0xdb8 */
	__u32        pad11;                    /* 0xdbc */
	/* entry.S sensitive area end */

        /* SMP info area: defined by DJB */
        __u64        jiffy_timer;              /* 0xdc0 */
	__u64        ext_call_fast;            /* 0xdc8 */
        __u8         pad12[0xe00-0xdd0];       /* 0xdd0 */

        /* 0xe00 is used as indicator for dump tools */
        /* whether the kernel died with panic() or not */
        __u32        panic_magic;              /* 0xe00 */

	__u8         pad13[0x1200-0xe04];      /* 0xe04 */

        /* System info area */ 

	__u64        floating_pt_save_area[16]; /* 0x1200 */
	__u64        gpregs_save_area[16];      /* 0x1280 */
	__u32        st_status_fixed_logout[4]; /* 0x1300 */
	__u8         pad14[0x1318-0x1310];      /* 0x1310 */
	__u32        prefixreg_save_area;       /* 0x1318 */
	__u32        fpt_creg_save_area;        /* 0x131c */
	__u8         pad15[0x1324-0x1320];      /* 0x1320 */
	__u32        tod_progreg_save_area;     /* 0x1324 */
	__u32        cpu_timer_save_area[2];    /* 0x1328 */
	__u32        clock_comp_save_area[2];   /* 0x1330 */
	__u8         pad16[0x1340-0x1338];      /* 0x1338 */ 
	__u32        access_regs_save_area[16]; /* 0x1340 */ 
	__u64        cregs_save_area[16];       /* 0x1380 */

	/* align to the top of the prefix area */

	__u8         pad17[0x2000-0x1400];      /* 0x1400 */
} __attribute__((packed)); /* End structure*/

#define S390_lowcore (*((struct _lowcore *) 0))
extern struct _lowcore *lowcore_ptr[];

extern __inline__ void set_prefix(__u32 address)
{
        __asm__ __volatile__ ("spx %0" : : "m" (address) : "memory" );
}

#define __PANIC_MAGIC           0xDEADC0DE

#endif

#endif

