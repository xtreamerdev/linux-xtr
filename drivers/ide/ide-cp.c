/*
 * linux/drivers/ide/ide-cd.c
 *
 * Copyright (C) 1994, 1995, 1996  scott snyder  <snyder@fnald0.fnal.gov>
 * Copyright (C) 1996-1998  Erik Andersen <andersee@debian.org>
 * Copyright (C) 1998-2000  Jens Axboe <axboe@suse.de>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 *
 * ATAPI CD-ROM driver.  To be used with ide.c.
 * See Documentation/cdrom/ide-cd for usage information.
 *
 * Suggestions are welcome. Patches that work are more welcome though. ;-)
 * For those wishing to work on this driver, please be sure you download
 * and comply with the latest Mt. Fuji (SFF8090 version 4) and ATAPI 
 * (SFF-8020i rev 2.6) standards. These documents can be obtained by 
 * anonymous ftp from:
 * ftp://fission.dt.wdc.com/pub/standards/SFF_atapi/spec/SFF8020-r2.6/PS/8020r26.ps
 * ftp://ftp.avc-pioneer.com/Mtfuji4/Spec/Fuji4r10.pdf
 *
 * Drives that deviate from these standards will be accommodated as much
 * as possible via compile time or command-line options.  Since I only have
 * a few drives, you generally need to send me patches...
 *
 * ----------------------------------
 * TO DO LIST:
 * -Make it so that Pioneer CD DR-A24X and friends don't get screwed up on
 *   boot
 *
 * ----------------------------------
 * 1.00  Jan 18, 2006 -- Initial version.
 
 *************************************************************************/
 
#define IDECP_VERSION "1.00"

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/cdrom.h>
#include <linux/ide.h>
#include <linux/completion.h>

#include <scsi/scsi.h>	/* For SCSI -> ATAPI command conversion */

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/byteorder.h>
#include <asm/uaccess.h>
#include <asm/unaligned.h>


#include "debug.h"
#include "ide-cp.h"
#include "h8300/ide-h8300.h"





int ide_cp_ioctl(ide_drive_t *drive, 	unsigned int cmd, unsigned long arg)
{
	ide_hwif_t *hwif = HWIF(drive);
	struct venus_state *state = hwif->hwif_data;
	u8 unit = (drive->select.b.unit & 0x01);
	
	struct cp_key *s = &(state->cpkey[unit].key[0]);
	int size = sizeof(struct cp_key);

 	idecpinfo("ide_cp_ioctl\n");
 	
	switch (cmd) {
		case CPIO_DECSS_ON:
		{
			state->cp_type[unit]=CP_DECSS;
			state->decrypt[unit]=1;
			copy_from_user(s, (struct cp_key *)arg, size);
			return 0;
		}
		case CPIO_DECSS_OFF:
		{
			state->cp_type[unit]=CP_NONE;
			state->decrypt[unit]=0;
			return 0;
		}
		default:
			return -EINVAL;
	}
}

EXPORT_SYMBOL(ide_cp_ioctl);
