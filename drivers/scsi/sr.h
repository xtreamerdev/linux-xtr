/*
 *      sr.h by David Giller
 *      CD-ROM disk driver header file
 *      
 *      adapted from:
 *      sd.h Copyright (C) 1992 Drew Eckhardt 
 *      SCSI disk driver header file by
 *              Drew Eckhardt 
 *
 *      <drew@colorado.edu>
 *
 *       Modified by Eric Youngdale eric@andante.org to
 *       add scatter-gather, multiple outstanding request, and other
 *       enhancements.
 */

#ifndef _SR_H
#define _SR_H

#include "scsi.h"
#include <linux/genhd.h>

/* The CDROM is fairly slow, so we need a little extra time */
/* In fact, it is very slow if it has to spin up first */
#define IOCTL_TIMEOUT 30*HZ

typedef struct scsi_cd {
	struct Scsi_Device_Template *driver;
	struct list_head list;
	unsigned capacity;	/* size in blocks                       */
	Scsi_Device *device;
	unsigned int vendor;	/* vendor code, see sr_vendor.c         */
	unsigned long ms_offset;	/* for reading multisession-CD's        */
	unsigned needs_sector_size:1;	/* needs to get sector size */
	unsigned use:1;		/* is this device still supportable     */
	unsigned xa_flag:1;	/* CD has XA sectors ? */
	unsigned readcd_known:1;	/* drive supports READ_CD (0xbe) */
	unsigned readcd_cdda:1;	/* reading audio data using READ_CD */
	struct cdrom_device_info cdi;
	struct gendisk *disk;
} Scsi_CD;

int sr_do_ioctl(Scsi_CD *, struct cdrom_generic_command *);

int sr_lock_door(struct cdrom_device_info *, int);
int sr_tray_move(struct cdrom_device_info *, int);
int sr_drive_status(struct cdrom_device_info *, int);
int sr_disk_status(struct cdrom_device_info *);
int sr_get_last_session(struct cdrom_device_info *, struct cdrom_multisession *);
int sr_get_mcn(struct cdrom_device_info *, struct cdrom_mcn *);
int sr_reset(struct cdrom_device_info *);
int sr_select_speed(struct cdrom_device_info *cdi, int speed);
int sr_audio_ioctl(struct cdrom_device_info *, unsigned int, void *);
int sr_dev_ioctl(struct cdrom_device_info *, unsigned int, unsigned long);

int sr_is_xa(Scsi_CD *);

/* sr_vendor.c */
void sr_vendor_init(Scsi_CD *);
int sr_cd_check(struct cdrom_device_info *);
int sr_set_blocklength(Scsi_CD *, int blocklength);

#endif
