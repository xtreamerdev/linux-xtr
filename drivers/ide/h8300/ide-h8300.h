#ifndef _IDE_H8300_H
#define _IDE_H8300_H

/*
 * drivers/ide/ide-h8300.c
 * H8/300 generic IDE interface
 */
 
 //#define	PIO
/*
#include <linux/init.h>
#include <linux/ide.h>
#include <linux/config.h>

#include <asm/io.h>
#include <asm/irq.h>
*/
#include "../debug.h"

#define CP_NONE		0
#define CP_VIDI		1
#define CP_DECSS		2		
#define CP_CPPM		3
#define CP_CPRM		4


struct cp_key {
	u32 key[5];
};

struct venus_state {
	
	// ide controller register address	
	unsigned long vdev[2];		// ATA0_DEV0/1
	unsigned long pio[2];			// ATA0_PIO0/1
	unsigned long dma[2];			// ATA0_DMA0/1
	unsigned long dlen;				// ATA0_DLEN
	unsigned long ctl;				// ATA0_CTL
	unsigned long en;					// ATA0_EN
	unsigned long inadr;			// ATA0_INADR
	unsigned long outadr;			// ATA0_OUTADR
	unsigned long rst;				// ATA0_RST
	
	// content protection controller register address
	unsigned long cp_ctrl;
	unsigned long cp_cp_key[4];
	
	/* parent device... until the IDE core gets one of its own */
	struct device *dev;

	u8 *p_virt_single_buf;			// Virtual memory address
	dma_addr_t p_bus_addr_single_buf;	// Physical memory address
	unsigned int	 dma_buffer_length;		// DMA buffer length
	
	// variable for content protection controller
	u8 cp[2];				// requst for content protection
	u8 cp_type[2];			// 0: no action, 1: vidi, 2:decss, 3: cppm, 4: cprm 
	u8 decrypt[2];			// 0: decryption 1: encryption
	struct cp_key cpkey[2];			// decss_key[0][n] for device 0;  decss_key[1][n] for device 1

	u32 *wrap_dmatable;		//table address shall be in IDEW_PRDPTR
	unsigned int wrap_ctrl;
	unsigned int wrap_prdaddr;
	unsigned int wrap_prdnum;	
};

#endif /* _IDE_H8300_H */
