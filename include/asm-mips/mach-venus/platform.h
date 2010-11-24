/*
 * This file defines the board id for Realtek dvr boards.
 * 
 */


#ifndef _MACH_BOARD_H
#define _MACH_BOARD_H


#define HDMI_KEY_LEN 571
#define SYSTEM_PARAMETERS_LEN 640




/* The format of "bootrev" is like "00.00.26c". The second byte of it stands for board id, and the upper half byte of board id stands for CPU id */
/* The new format of "bootrev" is extended to be "xxxx.xxxx.xx". We support both "xx.xx.xx" and "xxxx.xxxx.xx".  */
typedef enum {
	realtek_qa_board		=0x0,
	realtek_mk_board		=0x1,
	realtek_1261_demo_board		=0x2,		/* first demo board */
	realtek_1281_demo_board		=0x4,		/* little blue */
	realtek_mk2_board		=0x5,		/* little white */
	realtek_photoviewer_board	=0x7,
	realtek_avhdd_demo_board	=0x8,		/* little yellow */
	realtek_pvr_demo_board		=0x9,
	realtek_pvrbox_demo_board	=0xA,		
	realtek_avhdd2_demo_board	=0xB,		/* little yellow 2 */
	realtek_neptune_qa_board	=0x101,
	realtek_neptuneB_qa_board	=0x1101,
	realtek_neptune_demo_board	=0x102,
	realtek_neptuneB_demo_board	=0x1102,
	C01_avhdd_board			=0x10001,
	C01_photoviewer_board		=0x10007,
	C02_avhdd_board			=0x20008,
	C03_pvr_board			=0x30009,
	C03_pvr2_board			=0x3000B,
	C04_pvr_board			=0x40009,
	C04_pvr2_board			=0x4000B,
	C05_pvrbox_board		=0x5000A,
	C05_pvrbox2_board		=0x50001,
	C06_pvr_board			=0x60009,
	C07_avhdd_board			=0x70001, //AVHDD with J-Micron SATA.
	C07_pvrbox_board		=0x7000a, //PVR-BOX with PC Install.
	C07_pvrbox2_board		=0x7000b, //PVR-BOX without PC Install.
	C08_pvr_board			=0x80009,
	C09_pvrbox_board		=0x90001,
	C09_pvrbox2_board		=0x90002,
	C0A_pvr_board			=0xa0001, //2MB NOR and 128MB+ NAND.Behavior is the same with C0003.B000b
	C0B_dvr_board			=0xb1101, //Neptune 1282 DVR Demo Board.
	C0C_avhdd_board			=0xc0001,
	C0D_pvr_board			=0xd0001	//8 MB PVR Module.
} board_id_t;

/* The format of "bootrev" is like "00.00.26c". The first byte of it stands for company id */
typedef enum {
	company_realtek		=0x0,
	company_alpha		=0x1,
	company_C02		=0x2,
	company_C03		=0x3,
	company_C04		=0x4,
	company_C05		=0x5,
	company_C06		=0x6,
	company_C07		=0x7,
	company_C08		=0x8,
	company_C09		=0x9,
	company_C0A		=0xa,
	company_C0B		=0xb,
	company_C0C		=0xc,
	company_C0D		=0xd
} company_id_t;

typedef enum {
	realtek_venus_cpu	=0x0,
	realtek_venus2_cpu	=0x10,
	realtek_neptune_cpu	=0x1,
	realtek_neptuneB_cpu	=0x11
} cpu_id_t;

typedef enum {
	PAL,
	NTSC,
} tv_encoding_system_t;

typedef struct {
	board_id_t board_id;
	company_id_t company_id;
	cpu_id_t cpu_id;
	char kernel_source_code_info[64];
	char bootloader_version[12];
	char bootup_version[4];
	char ethaddr[20];			/* Ethernet Mac address */
	char usb_param[20];			/* USB parameter */
//	char vout_interface[1];			/* Video Out Interface */
	unsigned char hdmikey[HDMI_KEY_LEN];		/* maximum size 113. The last one may be '\0' */
	char system_parameters[SYSTEM_PARAMETERS_LEN];
	tv_encoding_system_t tv_encoding_system;
	char signature[129];
} platform_info_t;

extern platform_info_t platform_info;

#endif /* !(_MACH_BOARD_H) */


