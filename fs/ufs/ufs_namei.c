/*
 *  linux/fs/ufs/ufs_namei.c
 *
 * Copyright (C) 1996
 * Adrian Rodriguez (adrian@franklins-tower.rutgers.edu)
 * Laboratory for Computer Science Research Computing Facility
 * Rutgers, The State University of New Jersey
 *
 * $Id: ufs_namei.c,v 1.1.1.1 1997/06/01 03:16:19 ralf Exp $
 *
 */

#include <linux/fs.h>
#include <linux/ufs_fs.h>

#include <linux/string.h>

/*
 * NOTE! unlike strncmp, ext2_match returns 1 for success, 0 for failure.
 * stolen from ext2fs
 */
static int ufs_match (int len, const char * const name, struct ufs_direct * d)
{
	if (!d || len > UFS_MAXNAMLEN) /* XXX - name space */
		return 0;
	/*
	 * "" means "." ---> so paths like "/usr/lib//libc.a" work
	 */
	if (!len && (ufs_swab16(d->d_namlen) == 1) && (d->d_name[0] == '.') &&
	   (d->d_name[1] == '\0'))
		return 1;
	if (len != ufs_swab16(d->d_namlen))
		return 0;
	return !memcmp(name, d->d_name, len);
}

/* XXX - this is a mess, especially for endianity */
int ufs_lookup (struct inode * dir, struct qstr *qname,
	        struct inode ** result)
{
	unsigned long int lfragno, fragno;
	struct buffer_head * bh;
	struct ufs_direct * d;
	const char *name = qname->name;
	int len = qname->len;

	if (dir->i_sb->u.ufs_sb.s_flags & UFS_DEBUG)
		printk("Passed name: %s\nPassed length: %d\n", name, len);

	/*
	 * Touching /xyzzy in a filesystem toggles debugging messages.
	 */
	if ((len == 5) && !(memcmp(name, "xyzzy", len)) &&
	    (dir->i_ino == UFS_ROOTINO)) {
	        dir->i_sb->u.ufs_sb.s_flags ^= UFS_DEBUG;
	        printk("UFS debugging %s\n",
	               (dir->i_sb->u.ufs_sb.s_flags & UFS_DEBUG) ?
	               "on": "off");
		iput(dir);
	        return(-ENOENT);
	}

	/*
	 * Touching /xyzzy.i in a filesystem toggles debugging for ufs_inode.c.
	 */
	if ((len == 7) && !(memcmp(name, "xyzzy.i", len)) &&
	    (dir->i_ino == UFS_ROOTINO)) {
	        dir->i_sb->u.ufs_sb.s_flags ^= UFS_DEBUG_INODE;
	        printk("UFS inode debugging %s\n",
	               (dir->i_sb->u.ufs_sb.s_flags & UFS_DEBUG_INODE) ?
	               "on": "off");
		iput(dir);
	        return(-ENOENT);
	}

	if ((len == 7) && !(memcmp(name, "xyzzy.n", len)) &&
	    (dir->i_ino == UFS_ROOTINO)) {
	        dir->i_sb->u.ufs_sb.s_flags ^= UFS_DEBUG_NAMEI;
	        printk("UFS namei debugging %s\n",
	               (dir->i_sb->u.ufs_sb.s_flags & UFS_DEBUG_NAMEI) ?
	               "on": "off");
		iput(dir);
	        return(-ENOENT);
	}

	if ((len == 7) && !(memcmp(name, "xyzzy.l", len)) &&
	    (dir->i_ino == UFS_ROOTINO)) {
	        dir->i_sb->u.ufs_sb.s_flags ^= UFS_DEBUG_LINKS;
	        printk("UFS symlink debugging %s\n",
	               (dir->i_sb->u.ufs_sb.s_flags & UFS_DEBUG_LINKS) ?
	               "on": "off");
		iput(dir);
	        return(-ENOENT);
	}

	if (dir->i_sb->u.ufs_sb.s_flags & (UFS_DEBUG|UFS_DEBUG_NAMEI)) {
	        printk("ufs_lookup: called for ino %lu  name %s\n",
	               dir->i_ino, name);
	}

	/* XXX - do I want i_blocks in 512-blocks or 1024-blocks? */
	for (lfragno = 0; lfragno < (dir->i_blocks)>>1; lfragno++) {
	        fragno = ufs_bmap(dir, lfragno);
	        /* XXX - ufs_bmap() call needs error checking */
	        /* XXX - s_blocksize is actually the UFS frag size */
	        if (dir->i_sb->u.ufs_sb.s_flags & UFS_DEBUG) {
	                printk("ufs_lookup: ino %lu lfragno %lu  fragno %lu\n",
	                       dir->i_ino, lfragno, fragno);
	        }
	        if (fragno == 0) {
	                /* XXX - bug bug bug */
			iput(dir);
	                return(-ENOENT);
	        }
	        bh = bread(dir->i_dev, fragno, dir->i_sb->s_blocksize);
	        if (bh == NULL) {
	                printk("ufs_lookup: bread failed: ino %lu, lfragno %lu",
	                       dir->i_ino, lfragno);
			iput(dir);
	                return(-EIO);
	        }
	        d = (struct ufs_direct *)(bh->b_data);
	        while (((char *)d - bh->b_data + ufs_swab16(d->d_reclen)) <=
	               dir->i_sb->s_blocksize) {
	                /* XXX - skip block if d_reclen or d_namlen is 0 */
	                if ((ufs_swab16(d->d_reclen) == 0) || (ufs_swab16(d->d_namlen) == 0)) {
	                        if (dir->i_sb->u.ufs_sb.s_flags & UFS_DEBUG) {
	                                printk("ufs_lookup: skipped space in directory, ino %lu\n",
	                                       dir->i_ino);
	                        }
	                        break;
	                }
	                if (dir->i_sb->u.ufs_sb.s_flags & UFS_DEBUG) {
	                        printk("lfragno 0x%lx  direct d 0x%x  d_ino %u  d_reclen %u  d_namlen %u  d_name `%s'\n",
	                               lfragno, (unsigned int)d, ufs_swab32(d->d_ino), ufs_swab16(d->d_reclen), ufs_swab16(d->d_namlen), d->d_name);
	                }
	                if ((ufs_swab16(d->d_namlen) == len) &&
	                    /* XXX - don't use strncmp() - see ext2fs */
	                    (ufs_match(len, name, d))) {
	                        /* We have a match */
	                        *result = iget(dir->i_sb, ufs_swab32(d->d_ino));
	                        brelse(bh);
				iput(dir);
	                        return(0);
	                } else {
	                        /* XXX - bounds checking */
	                        if (dir->i_sb->u.ufs_sb.s_flags & UFS_DEBUG) {
	                                printk("ufs_lookup: wanted (%s,%d) got (%s,%d)\n",
	                                       name, len, d->d_name, ufs_swab16(d->d_namlen));
	                        }
	                }
	                d = (struct ufs_direct *)((char *)d + ufs_swab16(d->d_reclen));
	        }
	        brelse(bh);
	}
	iput(dir);
	return(-ENOENT);
}

