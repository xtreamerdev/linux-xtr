#ifndef	VENUS_SERIAL_FLASH_CONTROLLER
#define VENUS_SERIAL_FLASH_CONTROLLER

/*
	for HWSD, the serial flash address started from 0xbfcfffff,
	so for 16Mbit SST Serial Flash, the address ranges from 0xbfb0-0000 - 0xbfcf-ffff
*/
/* 2 MBytes */
#define FLASH_ADDR_END	0xbfd00000
//#define FLASH_SIZE		0x00200000
//#define FLASH_BASE		((volatile unsigned char *)(FLASH_ADDR_END-FLASH_SIZE))

#define SFC_OPCODE		((volatile unsigned int *)0xb801a800)
#define SFC_CTL			((volatile unsigned int *)0xb801a804)
#define SFC_SCK			((volatile unsigned int *)0xb801a808)
#define SFC_CE			((volatile unsigned int *)0xb801a80c)
#define SFC_WP			((volatile unsigned int *)0xb801a810)

#endif
