/* 
 * QNX4 file system, Linux implementation.
 * 
 * Version : 0.2.1
 * 
 * Using parts of the xiafs filesystem.
 * 
 * History :
 * 
 * 28-05-1998 by Richard Frowijn : first release.
 * 21-06-1998 by Frank Denis : ugly changes to make it compile on Linux 2.1.99+
 */

/* THIS FILE HAS TO BE REWRITTEN */

#include <linux/string.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/qnx4_fs.h>
#include <linux/stat.h>

#include <asm/segment.h>
#include <asm/uaccess.h>

static int qnx4_readlink(struct dentry *, char *, int);
static struct dentry *qnx4_follow_link(struct dentry *, struct dentry *, unsigned int follow);

/*
 * symlinks can't do much...
 */
struct inode_operations qnx4_symlink_inode_operations =
{
	readlink:		page_readlink,
	follow_link:		page_follow_link,
	get_block:		qnx4_get_block,
	readpage:		block_read_full_page,
};
