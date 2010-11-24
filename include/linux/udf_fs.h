/*
 * udf_fs.h
 *
 * PURPOSE
 *  Included by fs/filesystems.c
 *
 * DESCRIPTION
 *  OSTA-UDF(tm) = Optical Storage Technology Association
 *  Universal Disk Format.
 *
 *  This code is based on version 2.50 of the UDF specification,
 *  and revision 3 of the ECMA 167 standard [equivalent to ISO 13346].
 *    http://www.osta.org/ *    http://www.ecma.ch/
 *    http://www.iso.org/
 *
 * CONTACTS
 *	E-mail regarding any portion of the Linux UDF file system should be
 *	directed to the development team mailing list (run by majordomo):
 *		linux_udf@hpesjro.fc.hp.com
 *
 * COPYRIGHT
 *	This file is distributed under the terms of the GNU General Public
 *	License (GPL). Copies of the GPL can be obtained from:
 *		ftp://prep.ai.mit.edu/pub/gnu/GPL
 *	Each contributing author retains all rights to their own work.
 *
 *  (C) 1999-2004 Ben Fennema
 *  (C) 1999-2000 Stelias Computing Inc
 *
 * HISTORY
 *
 */

#ifndef _UDF_FS_H
#define _UDF_FS_H 1

#define UDF_PREALLOCATE
#define UDF_DEFAULT_PREALLOC_BLOCKS	32

#define UDFFS_DATE			"2005/21/07"
#define UDFFS_VERSION			"0.9.9"

#define UDF_BACKUP			// If define this, some udf tables and all of the file meta data will be backup and we can later restore them when udf is crashing

#undef UDFFS_DEBUG

#ifdef UDFFS_DEBUG
#define udf_debug(f, a...) \
	{ \
		printk (KERN_DEBUG "UDF-fs DEBUG %s:%d:%s: ", \
			__FILE__, __LINE__, __FUNCTION__); \
		printk (f, ##a); \
	}
#else
#define udf_debug(f, a...) /**/
#endif

#define udf_info(f, a...) \
		printk (KERN_INFO "UDF-fs INFO " f, ##a);

#endif /* _UDF_FS_H */
