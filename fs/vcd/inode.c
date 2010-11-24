/*
 *  linux/fs/isofs/inode.c
 *  (C) 1991  Linus Torvalds - minix filesystem
 *      1992, 1993, 1994  Eric Youngdale Modified for ISO 9660 filesystem.
 *      1994  Eberhard Moenkeberg - multi session handling.
 *      1995  Mark Dobie - allow mounting of some weird VideoCDs and PhotoCDs.
 *	1997  Gordon Chaffee - Joliet CDs
 *	1998  Eric Lammerts - ISO 9660 Level 3
 *	2004  Paul Serice - Inode Support pushed out from 4GB to 128GB
 *	2004  Paul Serice - NFS Export Operations
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/cdrom.h>
#include <linux/init.h>
#include <linux/nls.h>
#include <linux/ctype.h>
#include <linux/smp_lock.h>
#include <linux/parser.h>
/*
#include <linux/seq_file.h>
*/
#include <linux/statfs.h>
#include <linux/proc_fs.h>
#include "all.h"

#ifdef LEAK_CHECK
static int check_malloc;
static int check_bread;
#endif

static int isofs_hashi(struct dentry *parent, struct qstr *qstr);
static int isofs_hash(struct dentry *parent, struct qstr *qstr);
static int isofs_dentry_cmpi(struct dentry *dentry, struct qstr *a, struct qstr *b);
static int isofs_dentry_cmp(struct dentry *dentry, struct qstr *a, struct qstr *b);
static struct dentry * cdfs_lookup(struct inode *dir, struct dentry *dentry, struct nameidata *nd);
static void cdfs_read_inode(struct inode *i);
static void isofs_read_inode(struct inode * inode);
static void cdfs_umount(struct super_block *sb);
static int cdfs_statfs(struct super_block *sb, struct kstatfs *buf);
static int cdfs_readdir(struct file *filp, void *dirent, filldir_t filldir);
static struct proc_dir_entry *proc_file;
static struct proc_data proc_data_content;
static int find_cd_info_in_toc( struct super_block *sb );
int CD_TYPE;
int Mount_AVCD;
int First_AVCD;
int First_Video_LBA;
int SESSION_NO;
int TRACK_NO;
int VIDEOCD_NO;
int DVD_disc;
int EACD_disc;
int disc_mode;

static int proc_read_cd_info(char *page, char **start, off_t off, int count, int *eof, void *data){
        int len;
        struct proc_data *fb_data = (struct proc_data *)data;
        len = sprintf(page, "%s\n", fb_data->cd_type);
        return len;
}

static int find_cd_info_in_toc( struct super_block *sb )
{
	int rv = 0;
	extern int CD_TYPE;
	int data_track_number;
	char buffer[1];
	int audio_track;
	int video_track;
	cd * this_cd3 = cdfs_info( sb );
	data_track_number=0;
	audio_track=0;
	video_track=0;

        proc_file = create_proc_entry("cd_toc", 0, NULL);
        if(proc_file == NULL)
	{
		rv = -ENOMEM;
                goto no_proc_file;
	}

	data_track_number=this_cd3->tracks;

	if(DVD_disc)
	{
		CD_TYPE=DVD_DISC;
		strcpy(proc_data_content.cd_type, "DVD");
	}
	else if(EACD_disc)
	{
		CD_TYPE=EACD_DISC;
		if(disc_mode)
		{
			strcpy(proc_data_content.cd_type, "ACD");
			audio_track=data_track_number-1;
			TRACK_NO=data_track_number;
			CD_TYPE=ACD_DISC;
		}
		else
		{
			strcpy(proc_data_content.cd_type, "DATA");
			audio_track=data_track_number-1;
			TRACK_NO=data_track_number;
			CD_TYPE=DATA_DISC;
		}
	}
	else if(CD_TYPE==NO_DISC)
	{
		strcpy(proc_data_content.cd_type, "NO");
		video_track=data_track_number-1;
	}
	else if((CD_TYPE==VCD_DISC)&&VIDEOCD_NO)
	{
		strcpy(proc_data_content.cd_type, "VCD");
		video_track=data_track_number-1;
	}
	else if((CD_TYPE==SVCD_DISC)&&VIDEOCD_NO)
	{
		strcpy(proc_data_content.cd_type, "SVCD");
		video_track=data_track_number-1;
	}
	else if(CD_TYPE==ACD_DISC)
	{
		strcpy(proc_data_content.cd_type, "ACD");
		audio_track=data_track_number;
		TRACK_NO=audio_track;
	}
	else if((CD_TYPE==AVCD_DISC)&&VIDEOCD_NO)
	{
		strcpy(proc_data_content.cd_type, "AVCD");
		audio_track=data_track_number-First_AVCD+1;
		video_track=data_track_number-audio_track-1;
	}
	else if(CD_TYPE==Kodak_DISC)
	{
		strcpy(proc_data_content.cd_type, "Kodak");
	}
	else if(CD_TYPE==DATA_DISC)
	{
		strcpy(proc_data_content.cd_type, "DATA");
		audio_track=data_track_number;
		TRACK_NO=audio_track;
	}
	else
	{
		strcpy(proc_data_content.cd_type, "DATA");
		CD_TYPE=DATA_DISC;
	}
	strcat(proc_data_content.cd_type, " DATE 0718\n");
	sprintf(buffer,"%2d %2d %2d %2d",video_track,audio_track,TRACK_NO,SESSION_NO);
	strcat(proc_data_content.cd_type, buffer);
	strcat(proc_data_content.cd_type, "\n");
        proc_file->data = &proc_data_content;
        proc_file->read_proc = proc_read_cd_info;
        proc_file->owner = THIS_MODULE;
        return 0;

no_proc_file:	
	remove_proc_entry("cd_toc", NULL);
	return rv;
}

#ifdef CONFIG_JOLIET
static int isofs_hashi_ms(struct dentry *parent, struct qstr *qstr);
static int isofs_hash_ms(struct dentry *parent, struct qstr *qstr);
static int isofs_dentry_cmpi_ms(struct dentry *dentry, struct qstr *a, struct qstr *b);
static int isofs_dentry_cmp_ms(struct dentry *dentry, struct qstr *a, struct qstr *b);
#endif

/******************************************************************************
 * CDFS high-level file operations table                                      *
 ******************************************************************************/

//from root.c
static struct inode_operations cdfs_inode_operations = {
  .lookup   = cdfs_lookup
};

static struct file_operations cdfs_dir_operations = {
  .read     = generic_read_dir,
  .readdir  = cdfs_readdir,	//manipulate directory entries
};

static void isofs_put_super(struct super_block *sb)
{
	struct vcd_sb_info *sbi = ISOFS_SB(sb);
        cd * this_cd = cdfs_info(sb); //The same as : cdfs_info(sb)=sb->s_fs_info;
	printk("<vcd module> un-mount disc ST\n");
	#ifdef CONFIG_JOLIET
	if (sbi->s_nls_iocharset)
	{
		unload_nls(sbi->s_nls_iocharset);
		sbi->s_nls_iocharset = NULL;
	}
	#endif

	#ifdef LEAK_CHECK
	printk("Outstanding mallocs:%d, outstanding buffers: %d\n",
	       check_malloc, check_bread);
	#endif
  	PRINTM("M02 kfree   addr = 0x %x for this_cd->cache\n",(unsigned int)this_cd->cache);
  	kfree(this_cd->cache);	//M02

  	if (cdfs_info(sb))
  	{
		PRINTM("M01 kfree   addr = 0x %x for cdfs_info(sb)\n",(unsigned int)cdfs_info(sb));
  		kfree(cdfs_info(sb));	//M01
  	}
	PRINTM("M04 kfree   addr = 0x %x for sbi\n",(unsigned int)sbi);
	kfree(sbi);	//M04	//By vcd_main

	sb->s_fs_info = NULL;

	//¥H¤U¤T­ӬO FTIS
	////kfree(this_cd->cache);
	//kfree(cdfs_info(sb));

	remove_proc_entry("cd_toc", NULL);
	printk("<vcd module> un-mount disc SP OK!\n");
	
	return;
}

static int isofs_statfs (struct super_block *, struct kstatfs *);

static kmem_cache_t *isofs_inode_cachep;

static struct inode *isofs_alloc_inode(struct super_block *sb)
{
	struct iso_inode_info *ei;
	ei = (struct iso_inode_info *)kmem_cache_alloc(isofs_inode_cachep, SLAB_KERNEL);
	if (!ei)
	{
		return NULL;
	}
	return &ei->vfs_inode;
}

static void isofs_destroy_inode(struct inode *inode)
{
	kmem_cache_free(isofs_inode_cachep, ISOFS_I(inode));
	if(inode->u.generic_ip)
	{
		PRINTM("M08 kfree   addr = 0x %x for inode->u.generic_ip\n",(unsigned int)inode->u.generic_ip);
		kfree(inode->u.generic_ip);	//M08
	}
	//PRINT("(inode->u.generic_ip)=0x%x\n",(inode->u.generic_ip));
}

static void init_once(void * foo, kmem_cache_t * cachep, unsigned long flags)
{
	struct iso_inode_info *ei = (struct iso_inode_info *) foo;
	
	if ((flags & (SLAB_CTOR_VERIFY|SLAB_CTOR_CONSTRUCTOR)) == SLAB_CTOR_CONSTRUCTOR)
	{
		inode_init_once(&ei->vfs_inode);
	}
}
 
static int init_inodecache(void)
{
	isofs_inode_cachep = kmem_cache_create("vcdfs_inode_cache",
					     sizeof(struct iso_inode_info),
					     0, SLAB_RECLAIM_ACCOUNT,
					     init_once, NULL);
	if (isofs_inode_cachep == NULL)
		return -ENOMEM;
	return 0;
}

static void destroy_inodecache(void)
{
	if (kmem_cache_destroy(isofs_inode_cachep))
		printk(KERN_INFO "iso_inode_cache: not all structures were freed\n");
}

static int isofs_remount(struct super_block *sb, int *flags, char *data)
{
	/* we probably want a lot more here */
	*flags |= MS_RDONLY;	//files can only be read.
	return 0;
}

static struct super_operations isofs_sops = {
	.alloc_inode	= isofs_alloc_inode,
	.destroy_inode	= isofs_destroy_inode,
	.read_inode	= isofs_read_inode,
	.put_super	= isofs_put_super,
	.statfs		= isofs_statfs,
	.remount_fs	= isofs_remount,
};

static struct super_operations cdfs_ops = {
  .read_inode = cdfs_read_inode,
  .put_super  = cdfs_umount,
  .statfs     = cdfs_statfs	//get filesystem statistics
};

//static struct dentry_operations isofs_dentry_ops[] = {
struct dentry_operations isofs_dentry_ops[] = {
	{
		.d_hash		= isofs_hash,
		.d_compare	= isofs_dentry_cmp,
	},
	{
		.d_hash		= isofs_hashi,
		.d_compare	= isofs_dentry_cmpi,
	},
#ifdef CONFIG_JOLIET
	{
		.d_hash		= isofs_hash_ms,
		.d_compare	= isofs_dentry_cmp_ms,
	},
	{
		.d_hash		= isofs_hashi_ms,
		.d_compare	= isofs_dentry_cmpi_ms,
	}
#endif
};

struct all_options{
	char map;
	char rock;
	char joliet;
	char cruft;
	char unhide;
	char nocompress;
	unsigned char check;
	unsigned int blocksize;
	mode_t mode;
	gid_t gid;
	uid_t uid;
	char *iocharset;
	unsigned char utf8;
        /* LVE */
        s32 session;
        s32 sbsector;

        /* david from cdfs_parse_options */
	int single;
	int raw_audio;
	int toc_scsi;
	//uid_t uid;    
	//gid_t gid;
	//mode_t mode;
};

/* Compute the hash for the isofs name corresponding to the dentry. */
static int isofs_hash_common(struct dentry *dentry, struct qstr *qstr, int ms)
{
	const char *name;
	int len;
	len = qstr->len;
	name = qstr->name;
	if (ms)
	{
		while (len && name[len-1] == '.')
			len--;
	}
	qstr->hash = full_name_hash(name, len);
	return 0;
}

/* Compute the hash for the isofs name corresponding to the dentry. */
static int isofs_hashi_common(struct dentry *dentry, struct qstr *qstr, int ms)
{
	const char *name;
	int len;
	char c;
	unsigned long hash;
	len = qstr->len;
	name = qstr->name;
	if (ms)
	{
		while (len && name[len-1] == '.')
			len--;
	}
	hash = init_name_hash();
	while (len--)
	{
		c = tolower(*name++);
		hash = partial_name_hash(tolower(c), hash);
	}
	qstr->hash = end_name_hash(hash);
	return 0;
}

/* Case insensitive compare of two isofs names. */
static int isofs_dentry_cmpi_common(struct dentry *dentry,struct qstr *a,struct qstr *b,int ms)
{
	int alen, blen;
	/* A filename cannot end in '.' or we treat it like it has none */
	alen = a->len;
	blen = b->len;
	if (ms)
	{
		while (alen && a->name[alen-1] == '.')
			alen--;
		while (blen && b->name[blen-1] == '.')
			blen--;
	}
	if (alen == blen)
	{
		if (strnicmp(a->name, b->name, alen) == 0)
		{
			return 0;
		}
	}
	return 1;
}

/* Case sensitive compare of two isofs names. */
static int isofs_dentry_cmp_common(struct dentry *dentry,struct qstr *a,struct qstr *b,int ms)
{
	int alen, blen;
	/* A filename cannot end in '.' or we treat it like it has none */
	alen = a->len;
	blen = b->len;
	if (ms)
	{
		while (alen && a->name[alen-1] == '.')
			alen--;
		while (blen && b->name[blen-1] == '.')
			blen--;
	}
	if (alen == blen)
	{
		if (strncmp(a->name, b->name, alen) == 0)
		{
			return 0;
		}
	}
	return 1;
}

static int isofs_hash(struct dentry *dentry, struct qstr *qstr)
{
	return isofs_hash_common(dentry, qstr, 0);
}

static int isofs_hashi(struct dentry *dentry, struct qstr *qstr)
{
	return isofs_hashi_common(dentry, qstr, 0);
}

static int isofs_dentry_cmp(struct dentry *dentry,struct qstr *a,struct qstr *b)
{
	return isofs_dentry_cmp_common(dentry, a, b, 0);
}

static int isofs_dentry_cmpi(struct dentry *dentry,struct qstr *a,struct qstr *b)
{
	return isofs_dentry_cmpi_common(dentry, a, b, 0);
}

#ifdef CONFIG_JOLIET
static int isofs_hash_ms(struct dentry *dentry, struct qstr *qstr)
{
	return isofs_hash_common(dentry, qstr, 1);
}

static int isofs_hashi_ms(struct dentry *dentry, struct qstr *qstr)
{
	return isofs_hashi_common(dentry, qstr, 1);
}

static int isofs_dentry_cmp_ms(struct dentry *dentry,struct qstr *a,struct qstr *b)
{
	return isofs_dentry_cmp_common(dentry, a, b, 1);
}

static int isofs_dentry_cmpi_ms(struct dentry *dentry,struct qstr *a,struct qstr *b)
{
	return isofs_dentry_cmpi_common(dentry, a, b, 1);
}
#endif

enum {
	Opt_block, Opt_check_r, Opt_check_s, Opt_cruft, Opt_gid, Opt_ignore,
	Opt_iocharset, Opt_map_a, Opt_map_n, Opt_map_o, Opt_mode, Opt_nojoliet,
	Opt_norock, Opt_sb, Opt_session, Opt_uid, Opt_unhide, Opt_utf8, Opt_err,
	Opt_nocompress,
};

static match_table_t tokens = {
	{Opt_norock, "norock"},
	{Opt_nojoliet, "nojoliet"},
	{Opt_unhide, "unhide"},
	{Opt_cruft, "cruft"},
	{Opt_utf8, "utf8"},
	{Opt_iocharset, "iocharset=%s"},
	{Opt_map_a, "map=acorn"},
	{Opt_map_a, "map=a"},
	{Opt_map_n, "map=normal"},
	{Opt_map_n, "map=n"},
	{Opt_map_o, "map=off"},
	{Opt_map_o, "map=o"},
	{Opt_session, "session=%u"},
	{Opt_sb, "sbsector=%u"},
	{Opt_check_r, "check=relaxed"},
	{Opt_check_r, "check=r"},
	{Opt_check_s, "check=strict"},
	{Opt_check_s, "check=s"},
	{Opt_uid, "uid=%u"},
	{Opt_gid, "gid=%u"},
	{Opt_mode, "mode=%u"},
	{Opt_block, "block=%u"},
	{Opt_ignore, "conv=binary"},
	{Opt_ignore, "conv=b"},
	{Opt_ignore, "conv=text"},
	{Opt_ignore, "conv=t"},
	{Opt_ignore, "conv=mtext"},
	{Opt_ignore, "conv=m"},
	{Opt_ignore, "conv=auto"},
	{Opt_ignore, "conv=a"},
	{Opt_nocompress, "nocompress"},
	{Opt_err, NULL}
};

static int parse_options(char *options, struct all_options * popt)
{	//inode.c:parse_options()return 1 when OK, return 0 when fail
	char *this_char,*value;
	int option;
	popt->map = 'n';
	popt->rock = 'y';
	popt->joliet = 'y';
	popt->cruft = 'n';
	popt->unhide = 'n';
	popt->check = 'u';		/* unset */
	popt->nocompress = 0;
	popt->blocksize = 1024;
	popt->mode = S_IRUGO | S_IXUGO; /* r-x for all.  The disc could be shared with DOS machines so
					   virtually anything could be a valid executable. */
	popt->gid = 0;
	popt->uid = 0;
	popt->iocharset = NULL;
	popt->utf8 = 0;
	popt->session=-1;
	popt->sbsector=-1;

	//popt->mode           = MODE;
	//popt->gid            = GID;
	//popt->uid            = UID;
	//popt->single         = false;
	popt->single         = 0;
	popt->raw_audio      = 0;
	//popt->toc_scsi       = FALSE;	¥Τ£¨ꈉ
	if (!options)
	{
		return 1;
	}

	while ((this_char = strsep(&options, ",")) != NULL)	//strsep:Split a string into tokens
	{
		int token;
		unsigned n;
		substring_t args[MAX_OPT_ARGS];

		if (!*this_char)
		{	//this place can't place a printk
			//PRINT("david: In the while-loop (!*p) holds\n");
			continue;
		}
		//from root.c
		if (!strcmp(this_char,"single"))
		{	//No one enters here
			popt->single=TRUE;
		}
		else if (!strcmp(this_char,"raw"))
		{	//No one enters here
			popt->raw_audio=TRUE;
		}
		else if (!strcmp(this_char,"toc_scsi"))
		{	//No one enters here
			popt->toc_scsi=TRUE;
		}
		else if (!strcmp(this_char,"disc"))
		{
			disc_mode=1;
			continue;
		}
		else if (!strcmp(this_char,"browser"))
		{
			disc_mode=0;
			continue;
		}
		else
		{	//No one enters here
			if ((value = strchr(this_char,'=')) != NULL)
			{	//No one enters here
				*value++ = 0;
			}
			if (value &&(!strcmp(this_char,"mode") || !strcmp(this_char,"uid") || !strcmp(this_char,"gid")))
			{	//No one enters here
				char * vpnt = value;
				unsigned int ivalue = simple_strtoul(vpnt, &vpnt, 0);
				if (*vpnt)
				{
					//return 0;	//continue searching other items
				}
				switch(*this_char)
				{
				case 'u':  popt->uid = ivalue;              break;
				case 'g':  popt->gid = ivalue;              break;
				case 'm':  popt->mode = ivalue | S_IFREG ;  break;
				}
			}
		}
		//end of copy from root.c

		token = match_token(this_char, tokens, args);
		switch (token)
		{
		case Opt_norock:
			popt->rock = 'n';
			break;
		case Opt_nojoliet:
			popt->joliet = 'n';
			break;
		case Opt_unhide:
			popt->unhide = 'y';
			break;
		case Opt_cruft:
			popt->cruft = 'y';
			break;
		case Opt_utf8:
			popt->utf8 = 1;
			break;
#ifdef CONFIG_JOLIET
		case Opt_iocharset:
			popt->iocharset = match_strdup(&args[0]);
			break;
#endif
		case Opt_map_a:
			popt->map = 'a';
			break;
		case Opt_map_o:
			popt->map = 'o';
			break;
		case Opt_map_n:
			popt->map = 'n';
			break;
		case Opt_session:
			if (match_int(&args[0], &option))
				return 0;
			n = option;
			if (n > 99)
				return 0;
			popt->session = n + 1;
			break;
		case Opt_sb:
			if (match_int(&args[0], &option))
				return 0;
			popt->sbsector = option;
			break;
		case Opt_check_r:
			popt->check = 'r';
			break;
		case Opt_check_s:
			popt->check = 's';
			break;
		case Opt_ignore:
			break;
		case Opt_uid:
			if (match_int(&args[0], &option))
				return 0;
			popt->uid = option;
			break;
		case Opt_gid:
			if (match_int(&args[0], &option))
				return 0;
			popt->gid = option;
			break;
		case Opt_mode:
			if (match_int(&args[0], &option))
				return 0;
			popt->mode = option;
			break;
		case Opt_block:
			if (match_int(&args[0], &option))
				return 0;
			n = option;
			if (n != 512 && n != 1024 && n != 2048)
				return 0;
			popt->blocksize = n;
			break;
		case Opt_nocompress:
			popt->nocompress = 1;
			break;
		default:
			return 0;
		}
	}
	return 1;
}

/* look if the driver can tell the multi session redirection value
 *
 * don't change this if you don't know what you do, please!
 * Multisession is legal only with XA disks.
 * A non-XA disk with more than one volume descriptor may do it right, but
 * usually is written in a nowhere standardized "multi-partition" manner.
 * Multisession uses absolute addressing (solely the first frame of the whole
 * track is #0), multi-partition uses relative addressing (each first frame of
 * each track is #0), and a track is not a session.
 *
 * A broken CDwriter software or drive firmware does not set new standards,
 * at least not if conflicting with the existing ones.
 *
 * emoenke@gwdg.de
 */
#define WE_OBEY_THE_WRITTEN_STANDARDS 1

static unsigned int isofs_get_last_session(struct super_block *sb,s32 session )
{
	struct cdrom_multisession ms_info;

	unsigned int vol_desc_start;
	struct block_device *bdev = sb->s_bdev;
	int i;
	vol_desc_start=0;
	ms_info.addr_format=CDROM_LBA;
	if(session >= 0 && session <= 99)
	{
		struct cdrom_tocentry Te;
		Te.cdte_track=session;
		Te.cdte_format=CDROM_LBA;
		i = ioctl_by_bdev(bdev, CDROMREADTOCENTRY, (unsigned long) &Te);
		if (!i)
		{
			printk(KERN_DEBUG "Session %d start %d type %d\n",
			       session, Te.cdte_addr.lba,
			       Te.cdte_ctrl&CDROM_DATA_TRACK);
			if ((Te.cdte_ctrl&CDROM_DATA_TRACK) == 4)
				return Te.cdte_addr.lba;
		}
		printk(KERN_ERR "Invalid session number or type of track\n");
	}
	i = ioctl_by_bdev(bdev, CDROMMULTISESSION, (unsigned long) &ms_info);

	if(session > 0) printk(KERN_ERR "Invalid session number\n");
#if 0
	printk("isofs.inode: CDROMMULTISESSION: rc=%d\n",i);
	if (i==0)
	{
		printk("isofs.inode: XA disk: %s\n",ms_info.xa_flag?"yes":"no");
		printk("isofs.inode: vol_desc_start = %d\n", ms_info.addr.lba);
		PRINT("david: cd-info: with i=0, ms_info.xa_flag=%d  ms_info.addr.lba=%d\n",ms_info.xa_flag,ms_info.addr.lba);
	}
#endif
	if (i==0)
#if WE_OBEY_THE_WRITTEN_STANDARDS
        if (ms_info.xa_flag) /* necessary for a valid ms_info.addr */
#endif
	vol_desc_start=ms_info.addr.lba;
	return vol_desc_start;
}

static int fill_tracks_in_super(struct super_block *sb,int mounting_acd)
{
	int i, t;

	struct cdrom_tocentry   entry;   
	int no_audio, no_data;
	unsigned EACD_data_size;
	cd * this_cd;
	no_audio=0;
	no_data=0;
	VIDEOCD_NO=0;
	EACD_data_size=0;

	if(mounting_acd)
	{
		//0804 add this according to root.c by david	
		sb_set_blocksize(sb, CD_FRAMESIZE);
		sb->s_blocksize = CD_FRAMESIZE;
		sb->s_blocksize_bits = 11;
	}
	
	if (!(this_cd = cdfs_info(sb) = kmalloc(sizeof(cd), GFP_KERNEL)))	//M01
	{
		return -ENOMEM;
	}
	PRINTM("M01 kmalloc addr = 0x %x, size = %6d for this_cd\n",(unsigned int)this_cd,sizeof(cd));

	this_cd->mode           = MODE;
	this_cd->gid            = GID;
	this_cd->uid            = UID;
	this_cd->single         = FALSE;
	this_cd->raw_audio      = 0;
	//this_cd->toc_scsi       = FALSE;	//0804 maybe change to un-comment maybe ¥Τ£¨ꈊ	// Initialize cache for maximum sector size
	if (!(this_cd->cache = kmalloc(CD_FRAMESIZE_RAWER*CACHE_SIZE, GFP_KERNEL)))	//M02
	{
    		kfree(cdfs_info(sb));	//M01
		PRINTM("M01 kfree   addr = 0x %x for cdfs_info(sb)\n",(unsigned int)cdfs_info(sb));
		return -ENOMEM;
	}
	PRINTM("M02 kmalloc addr = 0x %x, size = %6d for this_cd->cache\n",(unsigned int)this_cd->cache,sizeof(CD_FRAMESIZE_RAWER*CACHE_SIZE));

	// Cache is still invalid
	this_cd->cache_sector = -CACHE_SIZE;
	
	//cdfs_parse_options((char *) data, this_cd);	//0804

	/* Populate CD info with '.' and '..' */
	strcpy(this_cd->track[1].name, ".");  this_cd->track[1].start_lba=0;
	strcpy(this_cd->track[2].name, ".."); this_cd->track[2].start_lba=0;
	this_cd->nr_iso_sessions = 0;
	this_cd->size            = 0;

	if (cdfs_toc_read(sb,mounting_acd))
	{
		goto invalid;
	}
	TRACK_NO=this_cd->tracks;
	/* Collect track info */
	entry.cdte_format = CDROM_LBA;

	for (t=this_cd->tracks; t>=0; t--)
	{
		i = T2I(t);	// T2I: Convert track to inode number, i=t+3
		if(!DVD_disc)
		{
			this_cd->track[i].track_size = this_cd->track[i+1].start_lba - this_cd->track[i].start_lba;  /* in sectors! */
			if(EACD_disc&&(t==(this_cd->tracks-2)))	//For Enhanced Music CD
			{
				this_cd->track[i].track_size-=11400;
			}
			PRINT("Start[%d]: %d\n", i, this_cd->track[i].start_lba);
		}
		if (t!=this_cd->tracks)
		{	/* all tracks but the LEADOUT */
			if (this_cd->track[i].type==DATA)
			{
				//int track=i;
				no_data++;
				this_cd->track[i].iso_info  = cdfs_get_iso_info(sb, i);	//calling iso.c:cdfs_get_iso_info()
				if (this_cd->track[i].iso_info)
				{
					this_cd->track[i].time      = cdfs_constructtime((char*)&(this_cd->track[i].iso_info->creation_date));
					this_cd->track[i].iso_size  = cdfs_constructsize((char*)&(this_cd->track[i].iso_info->volume_space_size));
					this_cd->track[i].iso_size  *= CD_FRAMESIZE;
					if(EACD_disc&&(t==(this_cd->tracks-1)))
					{
						EACD_data_size=this_cd->track[i].iso_size;
					}
					if (!this_cd->single)
					{
						this_cd->track[i].iso_size += this_cd->track[i].start_lba * CD_FRAMESIZE;
					}
					this_cd->track[i].track_size *= CD_FRAMESIZE;
					if(!DVD_disc)
						this_cd->track[i].size = this_cd->track[i+1].start_lba * CD_FRAMESIZE;
					else
						this_cd->track[i].size *= CD_FRAMESIZE;
					if(EACD_disc&&(t==(this_cd->tracks-1)))
					{
						this_cd->track[i].size=EACD_data_size;
					}
					sprintf(this_cd->track[i].name, this_cd->single ? DATA_NAME_SINGLE : DATA_NAME_ISO, t+1);
					//#define DATA_NAME_ISO    "sessions_1-%d.iso"
					//#define DATA_NAME_SINGLE "session-%d.iso"
					this_cd->lba_iso_sessions[this_cd->nr_iso_sessions].start = this_cd->track[i].start_lba;
					this_cd->lba_iso_sessions[this_cd->nr_iso_sessions].stop  = this_cd->track[i].iso_size/CD_FRAMESIZE;
					this_cd->nr_iso_sessions++;
					//SESSION_NO=this_cd->nr_iso_sessions;
					if (!strncmp(this_cd->track[i].iso_info->system_id,"CD-RTOS CD-BRIDGE",17))
					{	//For VCD
						this_cd->track[i].cd_bridge=1;
						CD_TYPE=VCD_DISC;
					}
					else if (!strncmp(this_cd->track[i].iso_info->system_id,"APPLE COMPUTER",14))
					{	//specifically for Kodak Picture CD
						this_cd->track[i].cd_bridge=0;
						CD_TYPE=Kodak_DISC;
					}
					else if (!strncmp(this_cd->track[i].iso_info->id,"CD001",5))
					{	//specifically for SVCD
						this_cd->track[i].cd_bridge=1;
						CD_TYPE=SVCD_DISC;
					}
					else
					{	//CDI-bridge, .cd_bridge=1
						this_cd->track[i].cd_bridge=0;
						CD_TYPE=DATA_DISC;
					}
					cdfs_get_hfs_info(sb, i);  // possibly also a HFS
					PRINTM("ISO01 kfree   addr = 0x %x for this_cd->track[%d].iso_info\n",(unsigned int)this_cd->track[i].iso_info,i);
					kfree(this_cd->track[i].iso_info);//ISO01
				}
				else
				{	// DATA, but no ISO -> either HFS or VideoCD
					if (cdfs_get_hfs_info(sb, i)==-1)
					{
						int ret;
						//printk("CHECKING VIDEOCD!!\n");
						VIDEOCD_NO++;
						this_cd->track[i].xa_data_size=2352;
						ret=cdfs_get_XA_info(sb, i);

						if(!ret) goto invalid; //disc not recognized => mount fails
						
						this_cd->track[i].time       = 0;
						this_cd->track[i].iso_size   = 0;
						this_cd->track[i].track_size = (this_cd->track[i].track_size-1) * this_cd->track[i].xa_data_size;
						this_cd->track[i].size       = this_cd->track[i].track_size;
						sprintf(this_cd->track[i].name, DATA_NAME_VCD, no_data);
						//#define DATA_NAME_VCD    "videocd-%d.mpeg"
						PRINT("CHECKING VIDEOCD!! track_size=%d size=%d xa_data_size=%d\n",this_cd->track[i].track_size,this_cd->track[i].size,this_cd->track[i].xa_data_size);
						PRINT("CHECKING VIDEOCD!! for name =%s size=%d\n",this_cd->track[i].name,this_cd->track[i].track_size);
					}
					else
					{	// HFS, no ISO, no VideoCD -> remove track
	    					this_cd->track[i].iso_info  = NULL;
	   					this_cd->track[i].type      = 0;
					}
				}
			}
			else
			{
				{
					no_audio++;
					this_cd->track[i].iso_info    = NULL;
					this_cd->track[i].type        = AUDIO;
					this_cd->track[i].time        = get_seconds();
					this_cd->track[i].iso_size    = 0;
					this_cd->track[i].track_size  = this_cd->track[i].track_size * CD_FRAMESIZE_RAW + ((this_cd->raw_audio==0)?WAV_HEADER_SIZE:0);
					this_cd->track[i].size        = this_cd->track[i].track_size;
					this_cd->track[i].avi         = 0;
					//Determine file names here, just fill the this_cd structure
					sprintf(this_cd->track[i].name, (this_cd->raw_audio)? RAW_AUDIO_NAME:AUDIO_NAME, t+1);
					if (this_cd->raw_audio)
					{	/* read the first sector. */
	    					//No one enters here
						struct cdrom_read_audio cdda;
						int status,k,j,prevk=0;
						char* buf;
						buf=kmalloc(CD_FRAMESIZE_RAW*2,GFP_KERNEL);	//M03
						if(buf==NULL)
						{
							printk(FSNAME ": kmalloc failed in root.c !\n");
							PRINTM("M02 kfree   addr = 0x %x for this_cd->cache\n",(unsigned int)this_cd->cache);
							kfree(this_cd->cache);//M02	//0118 add
							PRINTM("M01 kfree   addr = 0x %x for cdfs_info(sb)\n",(unsigned int)cdfs_info(sb));
							kfree(cdfs_info(sb));//M01	//0118 add
							return(-ENOMEM);
						}
						PRINTM("M03 kmalloc addr = 0x %x, size = %6d for buf\n",(unsigned int)buf,sizeof(CD_FRAMESIZE_RAW*2));
						for (j=0;j<20;j++)
						{
							cdda.addr_format = CDROM_LBA;
							cdda.nframes     = 1;
							cdda.buf         = buf+CD_FRAMESIZE_RAW;
							cdda.addr.lba = this_cd->track[i].start_lba+j;
							status = cdfs_ioctl(sb,CDROMREADAUDIO,(unsigned long)&cdda);
							if (status)
							{
								printk("cdfs_ioctl(CDROMREADAUDIO,%d) ioctl failed: %d\n", cdda.addr.lba, status);
								goto out;
							}
							/* search the first non-zero byte */
							for (k=0;k<CD_FRAMESIZE_RAW;k++)
								if (buf[k+CD_FRAMESIZE_RAW]) break;
								if (k<=CD_FRAMESIZE_RAW-4) break;
								prevk=k;
								if (k<CD_FRAMESIZE_RAW)
								for (k=0;k<CD_FRAMESIZE_RAW;k++)
								buf[k]=buf[k+CD_FRAMESIZE_RAW];
						}
						if (j==10) goto out;
						if ((j!=0)&&(prevk!=CD_FRAMESIZE_RAW))
						{
							k=prevk;
							j--;
						}
						else
							k+=CD_FRAMESIZE_RAW;
						this_cd->track[i].avi_offset = j*CD_FRAMESIZE_RAW+k-CD_FRAMESIZE_RAW;
						if ((buf[k]=='R')&&(buf[k+1]=='I')&&(buf[k+2]=='F')&&(buf[k+3]=='F'))
						{
							this_cd->track[i].avi = 1;
							this_cd->track[i].avi_swab = 0;
						}
						else if ((buf[k]=='I')&&(buf[k+1]=='R')&&(buf[k+2]=='F')&&(buf[k+3]=='F'))
						{
							this_cd->track[i].avi = 1;
							this_cd->track[i].avi_swab = 1;
						}
						if (this_cd->track[i].avi)
						{
							if ((this_cd->track[i].avi_offset&1)!=0)
							{
								printk("AVI offset is not even, error\n");
								this_cd->track[i].avi=0;
							}
							else
							{
								this_cd->track[i].track_size -= this_cd->track[i].avi_offset;
								sprintf(this_cd->track[i].name, AVI_AUDIO_NAME, t+1);
								//#define AVI_AUDIO_NAME       "track-%02d.avi"
							}
						}
	out:;
						//0118 add
						PRINTM("M02 kfree   addr = 0x %x for this_cd->cache\n",(unsigned int)this_cd->cache);
						kfree(this_cd->cache);//M02	//is this needed? david0804
						PRINTM("M01 kfree   addr = 0x %x for cdfs_info(sb)\n",(unsigned int)cdfs_info(sb));
						kfree(cdfs_info(sb));//M01	//is this needed? david0804
						PRINTM("Mxx_1 kfree   addr = 0x %x for buf\n",(unsigned int)buf);
						kfree(buf);	//Mxx
					}
				}
			}
			// Calculate total CD size
			this_cd->size += this_cd->track[i].track_size;
			PRINT("Size of Track %d = %d Bytes\n", t+1,  this_cd->track[i].size);
			if((t+1)==2)
			{
				//First_Video_LBA=this_cd->track[i].start_lba;
				First_Video_LBA=this_cd->track[i].start_lba-150; //Fix for a certain SVCD 0103
			}
		} // else CDROM_LEADOUT
	}
	/* take care to get disc id after the toc has been read. JP, 29-12-2001 */

 	return 0;

invalid:	//Doing fail conditions
	PRINTM("M02 kfree   addr = 0x %x for this_cd->cache\n",(unsigned int)this_cd->cache);
	kfree(this_cd->cache);	//M02
	PRINTM("M01 kfree   addr = 0x %x for cdfs_info(sb)\n",(unsigned int)cdfs_info(sb));
	kfree(cdfs_info(sb));	//M01
	return -EINVAL;
}

/* Initialize the superblock and read the root inode.
 * Note: a check_disk_change() has been done immediately prior
 * to this call, so we don't need to check again. */
static int vcd_main(struct super_block *s, void *data, int silent)
{
	struct buffer_head	      * bh = NULL, *pri_bh = NULL;
	struct hs_primary_descriptor  * h_pri = NULL;
	struct iso_primary_descriptor * pri = NULL;
	struct iso_supplementary_descriptor *sec = NULL;
	struct iso_directory_record   * rootp;
	int				joliet_level = 0;
	int				iso_blknum, block;
	int				orig_zonesize;
	int				table;
	unsigned int			vol_desc_start;
	unsigned long			first_data_zone;
	struct inode		      * inode;
	struct all_options		opt;
	struct vcd_sb_info	      * sbi = NULL;
	
	printk("<vcd module> mount disc with vcd module ST........\n");
	
	DVD_disc=find_cd_or_dvd(s);
	SESSION_NO=find_session_no(s);
	if(SESSION_NO!=1)
		printk("<vcd module> There are %d sessions in this disc.\n",SESSION_NO);
	EACD_disc=0;
	disc_mode=0;	//default is browser mode
	if((!DVD_disc)&&(SESSION_NO==2))
	{
		if(isEACD(s))
			EACD_disc=1;
	}
	sbi = kmalloc(sizeof(struct vcd_sb_info), GFP_KERNEL);		//M04
	
	if (!sbi)
	{
		return -ENOMEM;
	}
	PRINTM("M04 kmalloc addr = 0x %x, size = %6d for sbi\n",(unsigned int)sbi,sizeof(struct vcd_sb_info));
	s->s_fs_info = sbi;
	memset(sbi, 0, sizeof(struct vcd_sb_info));
	
	if (!parse_options((char *) data, &opt))
	{	//inode.c:parse_options()return 1 when OK, return 0 when fail
		goto out_freesbi;
	}
	disc_mode=EACD_disc&&disc_mode;

	/* First of all, get the hardware blocksize for this device.
	 * If we don't know what it is, or the hardware blocksize is
	 * larger than the blocksize the user specified, then use
	 * that value.
	 * What if bugger tells us to go beyond page size? */
	//opt.blocksize=2048;	//david
	opt.blocksize = sb_min_blocksize(s, opt.blocksize);
	sbi->s_high_sierra = 0; /* default is iso9660 */
	vol_desc_start = (opt.sbsector != -1) ?	opt.sbsector : isofs_get_last_session(s,opt.session);
	PRINTI("david: Parameters above: vol_desc_start=%d  opt.sbsector=%d\n",vol_desc_start,opt.sbsector);

	if(disc_mode)
	{
		PRINTI("disc_mode=%d for EACD\n",disc_mode);
		vol_desc_start=0;
	}
	
	for (iso_blknum = vol_desc_start+16; iso_blknum < vol_desc_start+100; iso_blknum++)
	{
	    struct hs_volume_descriptor   * hdp;
	    struct iso_volume_descriptor  * vdp;
	    block = iso_blknum << (ISOFS_BLOCK_BITS - s->s_blocksize_bits);
	    if (!(bh = sb_bread(s, block)))	//I01
	    {		//CDDA enters here, since buffer-head can't be found, bh=0
			if(block==17)
			{
				printk("<vcd module> sector 17 fails, but all right for (S)VCD if sector 16 is OK\n");

				break;
			}
			if(DVD_disc)
			{
				//DVD disc, but not data disc
				goto out_not_dvd_data;
			}
			goto out_audio_cd;
			//goto out_no_read;
	    }
	
	    vdp = (struct iso_volume_descriptor *)bh->b_data;
	    hdp = (struct  hs_volume_descriptor *)bh->b_data;

	    /* Due to the overlapping physical location of the descriptors, 
	     * ISO CDs can match hdp->id==HS_STANDARD_ID as well. To ensure 
	     * proper identification in this case, we first check for ISO. */
	    if (strncmp (vdp->id, ISO_STANDARD_ID, sizeof vdp->id) == 0)
		{
			if (isonum_711 (vdp->type) == ISO_VD_END)
			{
				//Go to root_found since primary iso descriprot found
				break;	//Leave the for-loop
			}
			if (isonum_711 (vdp->type) == ISO_VD_PRIMARY)
			{
				if (pri == NULL)
				{
					pri = (struct iso_primary_descriptor *)vdp;
					/* Save the buffer in case we need it . */
					pri_bh = bh;
					bh = NULL;
				}
			}
#ifdef CONFIG_JOLIET
			else if (isonum_711 (vdp->type) == ISO_VD_SUPPLEMENTARY)
			{
				//ISO9660 enters here
				sec = (struct iso_supplementary_descriptor *)vdp;
				if (sec->escape[0] == 0x25 && sec->escape[1] == 0x2f)
				{
					//ISO9660 enters here
					if (opt.joliet == 'y')
					{
						//ISO9660 enters here, joliet level\n");
						if (sec->escape[2] == 0x40)
						{
							joliet_level = 1;
						}
						else if (sec->escape[2] == 0x43)
						{
							joliet_level = 2;
						}
						else if (sec->escape[2] == 0x45)
						{
							joliet_level = 3;
						}
						//DATA CD enters here
						printk(KERN_DEBUG"ISO 9660 Extensions: Microsoft Joliet Level %d\n",joliet_level);
					}
					//PRINT("david: root_found_A\n");
					goto root_found;
				}
				else
				{
					/* Unknown supplementary volume descriptor */
					sec = NULL;
				}
			}
#endif
		}
		else
		{
	        if (strncmp (hdp->id, HS_STANDARD_ID, sizeof hdp->id) == 0)
	        {
				if (isonum_711 (hdp->type) != ISO_VD_PRIMARY)
				{
					goto out_freebh;
				}
				sbi->s_high_sierra = 1;
				opt.rock = 'n';
				h_pri = (struct hs_primary_descriptor *)vdp;
				//PRINT("david: root_found_B\n");
				goto root_found;
			}
		}
		/* Just skip any volume descriptors we don't recognize */
		brelse(bh);
		bh = NULL;
	}
	/* If we fall through, either no volume descriptor was found,
	 * or else we passed a primary descriptor looking for others.*/
	if (!pri)
	{
		goto out_unknown_format;
	}
	brelse(bh);
	bh = pri_bh;
	pri_bh = NULL;

root_found:
	if (fill_tracks_in_super(s,0))
	{
		printk(KERN_WARNING "isofs_fill_super_1: tracks not initialized\n");
		goto out_freesbi;
	}
	if (joliet_level && (pri == NULL || opt.rock == 'n'))
	{
	    //No one enters here
	    /* This is the case of Joliet with the norock mount flag.
	     * A disc with both Joliet and Rock Ridge is handled later */
	    pri = (struct iso_primary_descriptor *) sec;
	}

	if(sbi->s_high_sierra)
	{
		//No one enters here
		rootp = (struct iso_directory_record *) h_pri->root_directory_record;
		sbi->s_nzones = isonum_733 (h_pri->volume_space_size);
		sbi->s_log_zone_size = isonum_723 (h_pri->logical_block_size);
		sbi->s_max_size = isonum_733(h_pri->volume_space_size);
	}
	else
	{
		//Enters here
		if (!pri)
		{
			//No one enters here
			goto out_freebh;
		}
		rootp = (struct iso_directory_record *) pri->root_directory_record;
		sbi->s_nzones = isonum_733 (pri->volume_space_size);
		sbi->s_log_zone_size = isonum_723 (pri->logical_block_size);
		sbi->s_max_size = isonum_733(pri->volume_space_size);
	}
	sbi->s_ninodes = 0; /* No way to figure this out easily */
	orig_zonesize = sbi->s_log_zone_size;
	/* If the zone size is smaller than the hardware sector size,
	 * this is a fatal error.  This would occur if the disc drive
	 * had sectors that were 2048 bytes, but the filesystem had
	 * blocks that were 512 bytes (which should only very rarely
	 * happen.) */
	 
	/*
	if(orig_zonesize < opt.blocksize)
		goto out_bad_size;
		
	*/
	/* RDE: convert log zone size to bit shift */

	switch (sbi->s_log_zone_size)
	{
	case  512: sbi->s_log_zone_size =  9; break;
	case 1024: sbi->s_log_zone_size = 10; break;
	case 2048: sbi->s_log_zone_size = 11; break;
	default:
		goto out_bad_zone_size;
	}
	s->s_magic = ISOFS_SUPER_MAGIC;
	s->s_maxbytes = 0xffffffff; /* We can handle files up to 4 GB */

	/* The CDROM is read-only, has no nodes (devices) on it, and since
	   all of the files appear to be owned by root, we really do not want
	   to allow suid.  (suid or devices will not show up unless we have
	   Rock Ridge extensions) */

	s->s_flags |= MS_RDONLY /* | MS_NODEV | MS_NOSUID */;	//MS_RDONLY:files can only be read.

	/* Set this for reference. Its not currently used except on write
	   which we don't have .. */
	   
	first_data_zone = isonum_733 (rootp->extent) + isonum_711 (rootp->ext_attr_length);
	sbi->s_firstdatazone = first_data_zone;

	//DATA CD enters here
	printk(KERN_DEBUG "Max size:%ld   Log zone size:%ld\n",
	       sbi->s_max_size,
	       1UL << sbi->s_log_zone_size);
	printk(KERN_DEBUG "First datazone:%ld\n", sbi->s_firstdatazone);
	if(sbi->s_high_sierra)
		printk(KERN_DEBUG "Disc in High Sierra format.\n");

	/* If the Joliet level is set, we _may_ decide to use the
	 * secondary descriptor, but can't be sure until after we
	 * read the root inode. But before reading the root inode
	 * we may need to change the device blocksize, and would
	 * rather release the old buffer first. So, we cache the
	 * first_data_zone value from the secondary descriptor.  */
	if (joliet_level)
	{
		//DATA CD enters here
		pri = (struct iso_primary_descriptor *) sec;
		rootp = (struct iso_directory_record *)	pri->root_directory_record;
		first_data_zone = isonum_733 (rootp->extent) + isonum_711 (rootp->ext_attr_length);
	}

	/* We're all done using the volume descriptor, and may need
	 * to change the device blocksize, so release the buffer now. */
	brelse(pri_bh);
	brelse(bh);
	/* Force the blocksize to 512 for 512 byte sectors.  The file
	 * read primitives really get it wrong in a bad way if we don't
	 * do this.
	 *
	 * Note - we should never be setting the blocksize to something
	 * less than the hardware sector size for the device.  If we
	 * do, we would end up having to read larger buffers and split
	 * out portions to satisfy requests.
	 *
	 * Note2- the idea here is that we want to deal with the optimal
	 * zonesize in the filesystem.  If we have it set to something less,
	 * then we have horrible problems with trying to piece together
	 * bits of adjacent blocks in order to properly read directory
	 * entries.  By forcing the blocksize in this way, we ensure
	 * that we will never be required to do this. */
	sb_set_blocksize(s, orig_zonesize);
	sbi->s_nls_iocharset = NULL;

#ifdef CONFIG_JOLIET
	//3PRINT("david: joliet_level=%d  opt.utf8=%d\n",joliet_level,opt.utf8);
	if (joliet_level && opt.utf8 == 0)
	{
		//DATA CD enters here, typically joliet_level=3, opt.utf8=0
		char * p = opt.iocharset ? opt.iocharset : CONFIG_NLS_DEFAULT;
		sbi->s_nls_iocharset = load_nls(p);
		if (! sbi->s_nls_iocharset)
		{
			/* Fail only if explicit charset specified */
			if (opt.iocharset)
				goto out_freesbi;
			sbi->s_nls_iocharset = load_nls_default();
		}
	}
#endif
	PRINTI("david: chagne function pointer direction_VCD_mount_01, change to isofs_sops for s->s_op\n");
	s->s_op = &isofs_sops;
	//chagne function pointer direction
	PRINTI("david: chagne function pointer direction_VCD_mount_02, change to isofs_export_ops for s->s_export_op\n");
	s->s_export_op = &isofs_export_ops;	//defined in export.c
	//chagne function pointer direction
	sbi->s_mapping = opt.map;
	sbi->s_rock = (opt.rock == 'y' ? 2 : 0);
	sbi->s_rock_offset = -1; /* initial offset, will guess until SP is found*/
	sbi->s_cruft = opt.cruft;
	sbi->s_unhide = opt.unhide;
	sbi->s_uid = opt.uid;
	sbi->s_gid = opt.gid;
	sbi->s_utf8 = opt.utf8;
	sbi->s_nocompress = opt.nocompress;
	/* It would be incredibly stupid to allow people to mark every file
	 * on the disk as suid, so we merely allow them to set the default
	 * permissions. */
	sbi->s_mode = opt.mode & 0777;
	/* Read the root inode, which _may_ result in changing
	 * the s_rock flag. Once we have the final s_rock value,
	 * we then decide whether to use the Joliet descriptor. */
	inode = isofs_iget(s, sbi->s_firstdatazone, 0);
	/* If this disk has both Rock Ridge and Joliet on it, then we
	 * want to use Rock Ridge by default.  This can be overridden
	 * by using the norock mount option.  There is still one other
	 * possibility that is not taken into account: a Rock Ridge
	 * CD with Unicode names.  Until someone sees such a beast, it
	 * will not be supported. */
	if (sbi->s_rock == 1)
	{
		joliet_level = 0;
	}
	else if (joliet_level)
	{
		sbi->s_rock = 0;
		if (sbi->s_firstdatazone != first_data_zone)
		{
			//DATA CD enters here
			sbi->s_firstdatazone = first_data_zone;
			printk(KERN_DEBUG "ISOFS_1: changing to secondary root\n");
			iput(inode);
			inode = isofs_iget(s, sbi->s_firstdatazone, 0);
		}
	}
	if (opt.check == 'u')
	{
		/* Only Joliet is case insensitive by default */
		if (joliet_level) opt.check = 'r';
		else opt.check = 's';
	}
	sbi->s_joliet_level = joliet_level;
	/* check the root inode */
	if (!inode)
		goto out_no_root;
	if (!inode->i_op)
		goto out_bad_root;
	/* get the root dentry */
	s->s_root = d_alloc_root(inode);
	if (!(s->s_root))
		goto out_no_root;
	table = 0;
	if (joliet_level) table += 2;
	if (opt.check == 'r') table++;
	PRINTI("david: chagne function pointer direction_VCD_mount_05_end, change to isofs_dentry_ops for s->s_root->d_op\n");
	s->s_root->d_op = &isofs_dentry_ops[table];
	if (opt.iocharset)
	{
		PRINTM("Mxx_2 kfree   addr = 0x %x for opt.iocharset\n",(unsigned int)opt.iocharset);
		kfree(opt.iocharset);	//Mxx
	}
	//4PRINT("david: Make /proc _ST for VCD\n");
	find_cd_info_in_toc(s);
	//5PRINT("david: Make /proc _SP\n");
	printk("<vcd module> mount disc with vcd module SP OK for VCD/SVCD/Data........0718\n");
	switch (CD_TYPE)
	{
		case 1: printk("<vcd module> Disc type : VCD\n"); break;
		case 2: printk("<vcd module> Disc type : ACD\n"); break;
		case 3: printk("<vcd module> Disc type : AVCD\n"); break;
		case 4: printk("<vcd module> Disc type : SVCD\n"); break;
		case 5: printk("<vcd module> Disc type : EACD\n"); break;
		case 6: printk("<vcd module> Disc type : DATA_CD\n"); break;
		case 7: printk("<vcd module> Disc type : Kodak Picture CD\n"); break;
		case 8: printk("<vcd module> Disc type : DATA DVD\n"); break;
	default:
		printk("<vcd module> Disc type : Other Disc\n");
	}
	return 0;
	/* Display error messages and free resources. */
out_bad_root:
	printk(KERN_WARNING "isofs_fill_super: root inode not initialized\n");
	goto out_iput;
out_no_root:
	printk(KERN_WARNING "isofs_fill_super: get root inode failed\n");
out_iput:
	iput(inode);
#ifdef CONFIG_JOLIET
	if (sbi->s_nls_iocharset)
		unload_nls(sbi->s_nls_iocharset);
#endif
	goto out_freesbi;

	
out_audio_cd:
	CD_TYPE=ACD_DISC;

//Free that shoud be freed first
	if (opt.iocharset)
	{
		//CDDA will NOT enter here
		PRINTM("Mxx_3 kfree   addr = 0x %x for opt.iocharset\n",(unsigned int)opt.iocharset);
		kfree(opt.iocharset);	//Mxx
	}
	PRINTM("M04 kfree   addr = 0x %x for sbi\n",(unsigned int)sbi);
	kfree(sbi);		//M04
	s->s_fs_info = NULL;
	//Freed OK
	
	//0118 add
	brelse(bh);
	bh = pri_bh;
	pri_bh = NULL;
#ifdef CONFIG_JOLIET
	if (sbi->s_nls_iocharset)
		unload_nls(sbi->s_nls_iocharset);
#endif

	//add for all.h cdfs_indo() macro
	{
	struct vcd_sb_info	      * sbi=NULL;
	sbi = kmalloc(sizeof(struct vcd_sb_info), GFP_KERNEL);	//M05
	if (!sbi)
	{
		PRINT("kmalloc of sbi fails\n");
		return -ENOMEM;
	}
	PRINTM("M05 kmalloc addr = 0x %x, size = %6d for sbi\n",(unsigned int)sbi,sizeof(struct vcd_sb_info));
	s->s_fs_info = sbi;
	memset(sbi, 0, sizeof(struct vcd_sb_info));
	}
	
	if (fill_tracks_in_super(s,1))
	{
		printk(KERN_WARNING "isofs_fill_super_2: tracks not initialized\n");
		goto out_freesbi;
	}

	s->s_magic  = CDFS_MAGIC;
	s->s_flags |= MS_RDONLY;
	s->s_op     = &cdfs_ops;///aaaaa  0825
	//s->s_op = &isofs_sops;
	s->s_root   = d_alloc_root(iget(s, 0));
	
	PRINT("mount cdfs OK!\n");
	PRINT("david: Make /proc _ST for ACD\n");
	find_cd_info_in_toc(s);
	PRINT("david: Make /proc _SP\n");
	
	printk("<vcd module> mount disc with vcd module SP OK for Audio CD........0718\n");
	printk("<vcd module> Disc type : ACD\n");

	return 0;
	
	
/* Display error messages and free resources. */
//out_audio_cd
//Belows are fail conditions......
//out_no_read:
	//CDDA enters here with bread_failed
	printk(KERN_WARNING "isofs_fill_super: " "bread failed, dev=%s, iso_blknum=%d, block=%d\n", s->s_id, iso_blknum, block);
	goto out_freesbi;
out_bad_zone_size:
	printk(KERN_WARNING "Bad logical zone size %ld\n",
		sbi->s_log_zone_size);
	goto out_freebh;
//out_bad_size:
	printk(KERN_WARNING "Logical zone size(%d) < hardware blocksize(%u)\n",
		orig_zonesize, opt.blocksize);
	goto out_freebh;
out_unknown_format:
	if (!silent)
		printk(KERN_WARNING "Unable to identify CD-ROM format.\n");
out_freebh:
	brelse(bh);
out_freesbi:
	//CDDA enters here with bread_failed and 'out free struct vcd_sb_info *sbi'
	if (opt.iocharset)
	{
		//CDDA will NOT enter here
		PRINTM("Mxx_5 kfree   addr = 0x %x for opt.iocharset\n",(unsigned int)opt.iocharset);
		kfree(opt.iocharset);	//Mxx
	}
	PRINTM("M04 kfree   addr = 0x %x for sbi\n",(unsigned int)sbi);
	kfree(sbi);	//M04
	s->s_fs_info = NULL;
	printk("<vcd module> mount vcd module error!!!!!!!!!!!!!!!!A\n");
	//The end of mounting vcd file system() with fail
	return -EINVAL;

out_not_dvd_data:
//Free that shoud be freed first
	if (opt.iocharset)
	{
		//CDDA will NOT enter here
		PRINTM("Mxx_6 kfree   addr = 0x %x for opt.iocharset\n",(unsigned int)opt.iocharset);
		kfree(opt.iocharset);	//Mxx
	}
	PRINTM("M04 kfree   addr = 0x %x for sbi\n",(unsigned int)sbi);
	kfree(sbi);		//M04
	s->s_fs_info = NULL;
	//Freed OK
	printk("<vcd module> mount vcd module error!!!!!!!!!!!!!!!!B\n");
	//The end of mounting vcd file system() with fail
	return -EINVAL;
}

static int isofs_statfs (struct super_block *sb, struct kstatfs *buf)
{
	printk("inode.c:isofs_statfs():AnyoneUseThis???\n");
	//buf->f_type = ISOFS_SUPER_MAGIC;
	buf->f_type = 2;
	buf->f_bsize = sb->s_blocksize;
	buf->f_blocks = (ISOFS_SB(sb)->s_nzones
                  << (ISOFS_SB(sb)->s_log_zone_size - sb->s_blocksize_bits));
	buf->f_bfree = 0;
	buf->f_bavail = 0;
	buf->f_files = ISOFS_SB(sb)->s_ninodes;
	buf->f_ffree = 0;
	buf->f_namelen = NAME_MAX;
	return 0;
}

/* Get a set of blocks; filling in buffer_heads if already allocated
 * or getblk() if they are not.  Returns the number of blocks inserted (0 == error.) */
int isofs_get_blocks(struct inode *inode, sector_t iblock_s, struct buffer_head **bh, unsigned long nblocks)
{
	unsigned long b_off;
	unsigned offset, sect_size;
	unsigned int firstext;
	unsigned long nextblk, nextoff;
	long iblock = (long)iblock_s;
	int section, rv;
	struct iso_inode_info *ei = ISOFS_I(inode);
	lock_kernel();

	rv = 0;

	if (iblock < 0 || iblock != iblock_s)
	{
		printk("isofs_get_blocks: block number too large\n");
		goto abort;
	}
	b_off = iblock;
	offset    = 0;
	firstext  = ei->i_first_extent;
	sect_size = ei->i_section_size >> ISOFS_BUFFER_BITS(inode);
	nextblk   = ei->i_next_section_block;
	nextoff   = ei->i_next_section_offset;
	section   = 0;
	while ( nblocks )
	{
		/* If we are *way* beyond the end of the file, print a message.
		 * Access beyond the end of the file up to the next page boundary
		 * is normal, however because of the way the page cache works.
		 * In this case, we just return 0 so that we can properly fill
		 * the page with useless information without generating any
		 * I/O errors. */
		if (b_off > ((inode->i_size + PAGE_CACHE_SIZE - 1) >> ISOFS_BUFFER_BITS(inode)))
		{
			printk("isofs_get_blocks: block >= EOF (%ld, %ld)\n",
			       iblock, (unsigned long) inode->i_size);
			goto abort;
		}
		if (nextblk)
		{
			while (b_off >= (offset + sect_size))
			{
				struct inode *ninode;
				offset += sect_size;
				if (nextblk == 0)
					goto abort;
				ninode = isofs_iget(inode->i_sb, nextblk, nextoff);	//inode->i_sb : pointer to superblock object

				if (!ninode)
					goto abort;
				firstext  = ISOFS_I(ninode)->i_first_extent;
				sect_size = ISOFS_I(ninode)->i_section_size >> ISOFS_BUFFER_BITS(ninode);
				nextblk   = ISOFS_I(ninode)->i_next_section_block;
				nextoff   = ISOFS_I(ninode)->i_next_section_offset;
				iput(ninode);
				if (++section > 100)
				{
					printk("isofs_get_blocks: More than 100 file sections ?!?, aborting...\n");
					printk("isofs_get_blocks: block=%ld firstext=%u sect_size=%u "
					       "nextblk=%lu nextoff=%lu\n",
					       iblock, firstext, (unsigned) sect_size,
					       nextblk, nextoff);
					goto abort;
				}
			}
		}
		if ( *bh )
		{
			map_bh(*bh, inode->i_sb, firstext + b_off - offset);	//inode->i_sb : pointer to superblock object
		}
		else
		{
			*bh = sb_getblk(inode->i_sb, firstext+b_off-offset);	//inode->i_sb : pointer to superblock object
			if ( !*bh )
				goto abort;
		}
		bh++;	/* Next buffer head */
		b_off++;	/* Next buffer offset */
		nblocks--;
		rv++;
	}
abort:
	unlock_kernel();
	return rv;
}

/* Used by the standard interfaces. */
static int isofs_get_block(struct inode *inode, sector_t iblock, struct buffer_head *bh_result, int create)
{
	if ( create )
	{
		printk("isofs_get_block: Kernel tries to allocate a block\n");
		return -EROFS;
	}
	return isofs_get_blocks(inode, iblock, &bh_result, 1) ? 0 : -EIO;
}

static int isofs_bmap(struct inode *inode, sector_t block)
{
	struct buffer_head dummy;
	int error;
	dummy.b_state = 0;
	dummy.b_blocknr = -1000;
	error = isofs_get_block(inode, block, &dummy, 0);
	if (!error)
		return dummy.b_blocknr;
	return 0;
}

struct buffer_head *isofs_bread(struct inode *inode, sector_t block)
{
	sector_t blknr ;
	blknr = isofs_bmap(inode, block);
	if (!blknr)
		return NULL;
	return sb_bread(inode->i_sb, blknr);	//inode->i_sb : pointer to superblock object	//I02
}

static int isofs_readpage(struct file *file, struct page *page)
{
	return block_read_full_page(page,isofs_get_block);
}

static sector_t _isofs_bmap(struct address_space *mapping, sector_t block)
{
	return generic_block_bmap(mapping,block,isofs_get_block);
}

static struct address_space_operations isofs_aops = {
	.readpage = isofs_readpage,
	.sync_page = block_sync_page,
	.bmap = _isofs_bmap
};

static inline void test_and_set_uid(uid_t *p, uid_t value)
{
	if(value)
	{
		*p = value;
	}
}

static inline void test_and_set_gid(gid_t *p, gid_t value)
{
	if(value)
	{
		*p = value;
	}
}

static int isofs_read_level3_size(struct inode * inode)
{
	unsigned long bufsize = ISOFS_BUFFER_SIZE(inode);
	int high_sierra = ISOFS_SB(inode->i_sb)->s_high_sierra;	//inode->i_sb : pointer to superblock object
	struct buffer_head * bh = NULL;
	unsigned long block, offset, block_saved, offset_saved;
	int i = 0;
	int more_entries = 0;
	struct iso_directory_record * tmpde = NULL;
	struct iso_inode_info *ei = ISOFS_I(inode);

	inode->i_size = 0;

	/* The first 16 blocks are reserved as the System Area.  Thus,
	 * no inodes can appear in block 0.  We use this to flag that
	 * this is the last section. */
	ei->i_next_section_block = 0;
	ei->i_next_section_offset = 0;

	block = ei->i_iget5_block;
	offset = ei->i_iget5_offset;

	do {
		struct iso_directory_record * de;
		unsigned int de_len;
		if (!bh)
		{
			bh = sb_bread(inode->i_sb, block);	//inode->i_sb : pointer to superblock object	//I03
			if (!bh)
				goto out_noread;
		}
		de = (struct iso_directory_record *) (bh->b_data + offset);
		de_len = *(unsigned char *) de;
		if (de_len == 0)
		{
			brelse(bh);
			bh = NULL;
			++block;
			offset = 0;
			continue;
		}
		block_saved = block;
		offset_saved = offset;
		offset += de_len;
		/* Make sure we have a full directory entry */
		if (offset >= bufsize)
		{
			int slop = bufsize - offset + de_len;
			if (!tmpde)
			{
				tmpde = kmalloc(256, GFP_KERNEL);	//M06
				if (!tmpde)
					goto out_nomem;
				PRINTM("M06 kmalloc addr = 0x %x, size = %6d for tmpde\n",(unsigned int)tmpde,256);
			}
			memcpy(tmpde, de, slop);
			offset &= bufsize - 1;
			block++;
			brelse(bh);
			bh = NULL;
			if (offset)
			{
				bh = sb_bread(inode->i_sb, block);	//inode->i_sb : pointer to superblock object	//I04
				if (!bh)
					goto out_noread;
				memcpy((void *) tmpde + slop, bh->b_data, offset);
			}
			de = tmpde;
		}
		inode->i_size += isonum_733(de->size);
		if (i == 1)
		{
			ei->i_next_section_block = block_saved;
			ei->i_next_section_offset = offset_saved;
		}
		more_entries = de->flags[-high_sierra] & 0x80;
		i++;
		if(i > 100)
			goto out_toomany;
	} while(more_entries);
out:
	if (tmpde)
	{
		PRINTM("Mxx_7 kfree   addr = 0x %x for tmpde\n",(unsigned int)tmpde);
		kfree(tmpde);	//Mxx
	}
	if (bh)
		brelse(bh);
	return 0;

out_nomem:
	if (bh)
		brelse(bh);
	return -ENOMEM;

out_noread:
	printk(KERN_INFO "ISOFS: unable to read i-node block %lu\n", block);
	if (tmpde)
	{
		PRINTM("Mxx_8 kfree   addr = 0x %x for tmpde\n",(unsigned int)tmpde);
		kfree(tmpde);	//Mxx
	}
	return -EIO;

out_toomany:
	printk(KERN_INFO "isofs_read_level3_size: "
		"More than 100 file sections ?!?, aborting...\n"
	  	"isofs_read_level3_size: inode=%lu\n",
		inode->i_ino);
	goto out;
}

static void isofs_read_inode(struct inode * inode)
{
	struct super_block *sb = inode->i_sb;	//inode->i_sb : pointer to superblock object
	struct vcd_sb_info *sbi = ISOFS_SB(sb);	//struct vcd_sb_info is defined in all.h
	unsigned long bufsize = ISOFS_BUFFER_SIZE(inode);
	unsigned long block;
	int high_sierra = sbi->s_high_sierra;
	struct buffer_head * bh = NULL;
	struct iso_directory_record * de;
	struct iso_directory_record * tmpde = NULL;
	unsigned int de_len;
	unsigned long offset;
	struct iso_inode_info *ei = ISOFS_I(inode);

	block = ei->i_iget5_block;
	bh = sb_bread(inode->i_sb, block);	//inode->i_sb : pointer to superblock object	//I05
	if (!bh)
		goto out_badread;

	offset = ei->i_iget5_offset;

	de = (struct iso_directory_record *) (bh->b_data + offset);
	de_len = *(unsigned char *) de;
	if (offset + de_len > bufsize)
	{
		int frag1 = bufsize - offset;
		
		tmpde = kmalloc(de_len, GFP_KERNEL);	//M07
		if (tmpde == NULL)
		{
			printk(KERN_INFO "isofs_read_inode: out of memory\n");
			goto fail;
		}
		PRINTM("M07 kmalloc addr = 0x %x, size = %6d for sbi\n",(unsigned int)tmpde,de_len);
		memcpy(tmpde, bh->b_data + offset, frag1);
		brelse(bh);
		bh = sb_bread(inode->i_sb, ++block);	//inode->i_sb : pointer to superblock object	//I06
		if (!bh)
			goto out_badread;
		memcpy((char *)tmpde+frag1, bh->b_data, de_len - frag1);
		de = tmpde;
	}
	inode->i_ino = isofs_get_ino(ei->i_iget5_block, ei->i_iget5_offset, ISOFS_BUFFER_BITS(inode));

	/* Assume it is a normal-format file unless told otherwise */
	ei->i_file_format = isofs_file_normal;
	if (de->flags[-high_sierra] & 2)
	{
		inode->i_mode = S_IRUGO | S_IXUGO | S_IFDIR;
		inode->i_nlink = 1; /* Set to 1.  We know there are 2, but
				       the find utility tries to optimize
				       if it is 2, and it screws up.  It is
				       easier to give 1 which tells find to
				       do it the hard way. */
	}
	else
	{
		// ISO enters here
 		/* Everybody gets to read the file. */
		inode->i_mode = sbi->s_mode;
		inode->i_nlink = 1;
		inode->i_mode |= S_IFREG;
	}
	inode->i_uid = sbi->s_uid;
	inode->i_gid = sbi->s_gid;
	inode->i_blocks = inode->i_blksize = 0;

	ei->i_format_parm[0] = 0;
	ei->i_format_parm[1] = 0;
	ei->i_format_parm[2] = 0;

	ei->i_section_size = isonum_733 (de->size);
	if(de->flags[-high_sierra] & 0x80)
	{
		if(isofs_read_level3_size(inode))
			goto fail;
	}
	else
	{
		//Enters here
		ei->i_next_section_block = 0;
		ei->i_next_section_offset = 0;
		inode->i_size = isonum_733 (de->size);
	}

	/*
	 * Some dipshit decided to store some other bit of information
	 * in the high byte of the file length.  Truncate size in case
	 * this CDROM was mounted with the cruft option.
	 */

	if (sbi->s_cruft == 'y')
	{
		//No one enters here
		inode->i_size &= 0x00ffffff;
	}

	if (de->interleave[0])
	{
		//No one enters here
		printk("Interleaved files not (yet) supported.\n");
		inode->i_size = 0;
	}

	/* I have no idea what file_unit_size is used for, so
	   we will flag it for now */
	if (de->file_unit_size[0] != 0)
	{
		//No one enters here
		printk("File unit size != 0 for ISO file (%ld).\n", inode->i_ino);
	}

	/* I have no idea what other flag bits are used for, so
	   we will flag it for now */
	#ifdef DEBUG
	if((de->flags[-high_sierra] & ~2)!= 0)
	{
		//No one enters here
		printk("Unusual flag settings for ISO file (%ld %x).\n", inode->i_ino, de->flags[-high_sierra]);
	}
	#endif
	
	inode->i_mtime.tv_sec =
	inode->i_atime.tv_sec =
	inode->i_ctime.tv_sec = iso_date(de->date, high_sierra);
	inode->i_mtime.tv_nsec =
	inode->i_atime.tv_nsec =
	inode->i_ctime.tv_nsec = 0;

	ei->i_first_extent = (isonum_733 (de->extent) + isonum_711 (de->ext_attr_length));

	/* Set the number of blocks for stat() - should be done before RR */
	inode->i_blksize = PAGE_CACHE_SIZE; /* For stat() only */
	inode->i_blocks  = (inode->i_size + 511) >> 9;

	/* Now test for possible Rock Ridge extensions which will override
	 * some of these numbers in the inode structure. */

	if (!high_sierra)
	{
		parse_rock_ridge_inode(de, inode);
		/* if we want uid/gid set, override the rock ridge setting */
		test_and_set_uid(&inode->i_uid, sbi->s_uid);
		test_and_set_gid(&inode->i_gid, sbi->s_gid);
	}
	/* Install the inode operations vector */
	if (S_ISREG(inode->i_mode))
	{
		int i, t;
		track_info * this_track;
		cd * this_cd = cdfs_info(sb);
		i=0;
		// added by Frank  94/9/6
		for (t=this_cd->tracks; t>=0; t--)	//(this_cd->tracks)ª���³o¤򄥀¦³´X­Ҵrack
		{
			i = T2I(t);// T2I: Convert track to inode number, i=t+3
			//PRINT("david1129:²¥d­yªº°_©lLBA¡GStart[%d]: %d\n", i-2, i, this_cd->track[i].start_lba);
			//PRINT("Track name¡Gthis_cd->track[i].name: %s\n",this_cd->track[i].name);
			if (ei->i_first_extent >= this_cd->track[i].start_lba)	//(this_cd->track[i])ª���²é­yªº¸ꯆ¤º®e
			{
				//PRINT("david0526: Got the LBA_ST for this track lba_st=%d\n",this_cd->track[i].start_lba);
				//printk("david0601 You want to read file¡Gthis_cd->track[i].name: %s lba_st=%d\n",this_cd->track[i].name,this_cd->track[i].start_lba);
				break;
			}
		}
		this_track = &(this_cd->track[i]);

		//According file data type, chagne function pointer direction

		if(DVD_disc)
			this_track->type=DATA;

		if (this_track->type==DATA)
		{
			// isofs data track
			if ((CD_TYPE==DVD_DISC)||((this_track->iso_info)&&(this_track->cd_bridge==0)))	//(this_track->cd_bridge==0) => ¬OCDI-bridgeªº±¡ªp => ISO9660
			//if ((CD_TYPE==DVD_DISC)||(CD_TYPE==DATA_DISC)||(CD_TYPE==Kodak_DISC)||((this_track->iso_info)&&(this_track->cd_bridge==0)))	//(this_track->cd_bridge==0) => ¬OCDI-bridgeªº±¡ªp => ISO9660
			{
				//ISO9660 default generic file read
				//printk("change to generic_ro_fops for inode->i_fop\n");
				inode->i_fop = &generic_ro_fops;	//inode->i_fop : default file operations
				//chagne function pointer direction
				switch ( ei->i_file_format )
				{

				#ifdef CONFIG_ZISOFS
				case isofs_file_compressed:
					inode->i_data.a_ops = &zisofs_aops;
					break;
				#endif
				default:
					PRINTI("david: chagne function pointer direction, change to isofs_aops for inode->i_data.a_ops\n");
					inode->i_data.a_ops = &isofs_aops;
					break;
				}
			}
			else
			{
				inode_info *p;
				if (!(p=inode->u.generic_ip = (inode_info *)kmalloc(sizeof(inode_info), GFP_KERNEL)))	//M08
				{
					printk(KERN_INFO "isofs_read_inode: out of memory(inode_info)\n");
					goto fail;
				}
				else
				{
					PRINTM("M08 kmalloc addr = 0x %x, size = %6d for p=inode->u.generic_ip\n",(unsigned int)inode->u.generic_ip,sizeof(inode_info));
					//if((CD_TYPE==DATA_DISC)||(CD_TYPE==Kodak_DISC)||(CD_TYPE==DVD_DISC))
					if((CD_TYPE==DATA_DISC)||(CD_TYPE==Kodak_DISC))
					{
						p->xa_data_size=2048;
						p->xa_data_offset=-1;
					}
					else
					{
						vcdfs_get_xa_sector_info(sb, ei->i_first_extent, &(p->xa_data_size), &(p->xa_data_offset));
						if (p->xa_data_size!=2048)
						{
							inode->i_size	= (inode->i_size>>11)*p->xa_data_size;
							inode->i_blocks	= (inode->i_size + 511) >> 9;
						}
					}
					PRINTI("david: chagne function pointer direction_VCD_ls_cp_03, change to cdfs_cdXA_file_operations for inode->i_fop\n");
					inode->i_fop = &cdfs_cdXA_file_operations;	//inode->i_fop : default file operations
					PRINTI("david: chagne function pointer direction_VCD_ls_cp_04, change to cdfs_cdXA_aops for inode->i_data.a_ops\n");
					inode->i_data.a_ops = &cdfs_cdXA_aops;
					//PRINT("david: at M2, inode->i_size=%lu  inode->i_blocks=%d\n",inode->i_size,inode->i_blocks);
				}				
			}
		}
		else if (this_track->type==AUDIO)
		{
			//VCD with audio tracks in it => AVCD
			//int table;
			//table=0;
			// audio track
			//PRINT("david1128_resize1: t=%d First_AVCD=%d\n",t,First_AVCD);
			if((t+1)==First_AVCD)
			{
				inode->i_size=(inode->i_size/2048+150)*2352;
			}
			else
			{
				inode->i_size=inode->i_size/2048*2352;
			}
			// §A_ST
			inode->i_ino=i;
		
			PRINTI("david: chagne function pointer direction_ACD_mount_03_08_ls_cp, change to cdfs_inode_operations for i->i_op\n");
			inode->i_op         = &cdfs_inode_operations;	//inode operations
			inode->i_fop        = NULL;			//default file operations
			inode->i_data.a_ops = NULL;


			PRINTI("david: chagne function pointer direction_AVCD_xx1, change to cdfs_cdda_file_operations for inode->i_fop\n");
			inode->i_fop          = &cdfs_cdda_file_operations;	//inode->i_fop : default file operations
			if (this_cd->raw_audio)
			{
				PRINTI("david: chagne function pointer direction_AVCD_xx2, change to cdfs_cdda_raw_aops for inode->i_data.a_ops\n");
				inode->i_data.a_ops   = &cdfs_cdda_raw_aops;
			}
			else
			{
				PRINTI("david: chagne function pointer direction_AVCD_xx3, change to cdfs_cdda_aops for inode->i_data.a_ops\n");
				inode->i_data.a_ops   = &cdfs_cdda_aops;
			}
		}
	}
	else if (S_ISDIR(inode->i_mode))	//Enters here
	{
		PRINT("david: Later will use namei.c:struct dentry *isofs_lookup\n");
		PRINTI("david: chagne function pointer direction_VCD_mount_03_ls_cp_01, change to isofs_dir_inode_operations for inode->i_op\n");
		inode->i_op = &isofs_dir_inode_operations;//using isofs_lookup() in dir.c
		inode->i_fop = &isofs_dir_operations;	//inode->i_fop : default file operations
	}
	else if (S_ISLNK(inode->i_mode))
	{
		//Seems never enters here.
		printk("change to page_symlink_inode_operations for inode->i_op_AnyoneUseThis???\n");
		inode->i_op = &page_symlink_inode_operations;
		printk("change to isofs_symlink_aops for inode->i_data.a_ops_AnyoneUseThis???\n");
		inode->i_data.a_ops = &isofs_symlink_aops;
	}
	else
	{
		/* XXX - parse_rock_ridge_inode() had already set i_rdev. */
		init_special_inode(inode, inode->i_mode, inode->i_rdev);
		//@fs/inode.c:init_special_inode()
	}
 out:
	if (tmpde)
	{
		PRINTM("Mxx_9 kfree   addr = 0x %x for tmpde\n",(unsigned int)tmpde);
		kfree(tmpde);	//Mxx
	}
	if (bh)
		brelse(bh);
	return;
 out_badread:
	printk(KERN_WARNING "ISOFS: unable to read i-node block\n");
 fail:
	make_bad_inode(inode);
	goto out;
}

struct isofs_iget5_callback_data
{
	unsigned long block;
	unsigned long offset;
};

static int isofs_iget5_test(struct inode *ino, void *data)
{
	struct iso_inode_info *i = ISOFS_I(ino);
	struct isofs_iget5_callback_data *d = (struct isofs_iget5_callback_data*)data;
	return (i->i_iget5_block == d->block) && (i->i_iget5_offset == d->offset);
}

static int isofs_iget5_set(struct inode *ino, void *data)
{
	struct iso_inode_info *i = ISOFS_I(ino);
	struct isofs_iget5_callback_data *d =
		(struct isofs_iget5_callback_data*)data;
	i->i_iget5_block = d->block;
	i->i_iget5_offset = d->offset;
	return 0;
}

/* Store, in the inode's containing structure, the block and block
 * offset that point to the underlying meta-data for the inode.  The
 * code below is otherwise similar to the iget() code in
 * include/linux/fs.h */
struct inode *isofs_iget(struct super_block *sb, unsigned long block, unsigned long offset)
{
	unsigned long hashval;
	struct inode *inode;
	struct isofs_iget5_callback_data data;
	
	if (offset >= 1ul << sb->s_blocksize_bits)
		return NULL;
	data.block = block;
	data.offset = offset;
	hashval = (block << sb->s_blocksize_bits) | offset;
	inode = iget5_locked(sb,     //obtain an inode from a mounted file system 
			     hashval,
			     &isofs_iget5_test,
			     &isofs_iget5_set,
			     &data);
	if (inode && (inode->i_state & I_NEW))
	{
		sb->s_op->read_inode(inode);
		unlock_new_inode(inode);
	}
	return inode;
}

#ifdef LEAK_CHECK
#undef malloc
#undef free_s
#undef sb_bread
#undef brelse

void * leak_check_malloc(unsigned int size)
{
  void * tmp;
  check_malloc++;
  tmp = kmalloc(size, GFP_KERNEL);	//M09
  PRINTM("M09 kmalloc addr = 0x %x, size = %6d for tmp\n",(unsigned int)tmp,size);
  return tmp;
}

void leak_check_free_s(void * obj, int size)
{
  check_malloc--;
  PRINTM("Mxx_10 kfree   addr = 0x %x for obj\n",(unsigned int)obj);
  return kfree(obj);	//Mxx
}

struct buffer_head * leak_check_bread(struct super_block *sb, int block)
{
  check_bread++;
  return sb_bread(sb, block);	//I07
}

void leak_check_brelse(struct buffer_head * bh)
{
  check_bread--;
  return brelse(bh);
}

#endif

static struct super_block *vcd_get_sb(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
	return get_sb_bdev(fs_type, flags, dev_name, data, vcd_main);
}

static struct file_system_type iso9660_fs_type = {
	.owner		= THIS_MODULE,		//struct module*
	.name		= "vcd",		//const char*
	.get_sb		= vcd_get_sb,		//struct super_block **
	.kill_sb	= kill_block_super,	//void*
	.fs_flags	= FS_REQUIRES_DEV,	//int*		//Any filesystem of this type must be located on a physical disk device.
};

static void cdfs_read_inode(struct inode *i)
{
  cd * this_cd = cdfs_info(i->i_sb);//The same as : cdfs_info(sb)=sb->s_fs_info;

  //printk("this_cd = 0x%x\n\n\n", (unsigned)this_cd);
  //¦C¦L¦즽­n±j¨´«¦¨unsigned

  i->i_uid        = this_cd->uid;		//owner identifier
  i->i_gid        = this_cd->gid;		//group identifier
  i->i_nlink      = 1;				//number of hard links
  PRINTI("david: chagne function pointer direction_ACD_mount_03_08_ls_cp, change to cdfs_inode_operations for i->i_op\n");
  i->i_op         = &cdfs_inode_operations;	//inode operations
  						//chagne function pointer direction
  i->i_fop        = NULL;			//default file operations
  i->i_data.a_ops = NULL;

  PRINT("read inode %ld   which is inode number\n", i->i_ino);	//i->i_ino is inode number
  //According file data type, chagne function pointer direction
  if (i->i_ino <= 2)					/* . and .. */
  {
	  i->i_size  = 0;				/* Uuugh ?? */
	  i->i_mtime = i->i_atime = i->i_ctime = CURRENT_TIME;
	  i->i_mode  = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP |  S_IXGRP | S_IROTH | S_IXOTH;
	  PRINTI("david: chagne function pointer direction_ACD_mount_09_end, change to cdfs_dir_operations for i->i_fop\n");
	  i->i_fop   = &cdfs_dir_operations;//chagne function pointer direction
  }
  else
  {                                          /* file */
	  i->i_size  = this_cd->track[i->i_ino].size;
    i->i_mtime.tv_sec = i->i_atime.tv_sec = i->i_ctime.tv_sec = this_cd->track[i->i_ino].time;
    i->i_mtime.tv_nsec = i->i_atime.tv_nsec = i->i_ctime.tv_nsec = 0;
    i->i_mode  = this_cd->mode;
    if ((this_cd->track[i->i_ino].type==DATA) && this_cd->track[i->i_ino].iso_size)
    {
	    //printk("change to cdfs_cddata_file_operations for i->i_fop_AnyoneUseThis???\n");
	    i->i_fop          = &cdfs_cddata_file_operations; 	//chagne function pointer direction
	    //printk("change to cdfs_cddata_aops for i->i_data.a_ops_AnyoneUseThis???\n");
	    i->i_data.a_ops   = &cdfs_cddata_aops;
    }
    else if (this_cd->track[i->i_ino].type==AUDIO)
    {
	    PRINTI("david: chagne function pointer direction_ACD_ls_cp, change to cdfs_cdda_file_operations for i->i_fop\n");
	    i->i_fop          = &cdfs_cdda_file_operations;
	    if (this_cd->raw_audio)
	    {
		    printk("change to cdfs_cdda_raw_aops for i->i_data.a_ops_AnyoneUseThis???\n");
		    i->i_data.a_ops   = &cdfs_cdda_raw_aops;
	    }
	    else
	    {
		    PRINTI("david: chagne function pointer direction_ACD_ls_cp, change to cdfs_cdda_aops for i->i_data.a_ops\n");
		    i->i_data.a_ops   = &cdfs_cdda_aops;
	    }
    }
    else if (this_cd->track[i->i_ino].type==BOOT)
    {
	    //printk("change to cdfs_cddata_file_operations for i->i_fop_AnyoneUseThis???\n");
	    i->i_fop          = &cdfs_cddata_file_operations;
	    //printk("change to cdfs_cddata_aops for i->i_data.a_ops_fop_AnyoneUseThis???\n");
	    i->i_data.a_ops   = &cdfs_cddata_aops;
    }
    else if (this_cd->track[i->i_ino].type==HFS)
    {
	    if (this_cd->track[i->i_ino].hfs_offset)
	    {
		    printk("change to cdfs_cdhfs_file_operations for i->i_fop_AnyoneUseThis???\n");
		    i->i_fop        = &cdfs_cdhfs_file_operations; /* Bummer, this partition isn't properly aligned... */
		    printk("change to cdfs_cdhfs_aops for i->i_data.a_ops_AnyoneUseThis???\n");
		    i->i_data.a_ops = &cdfs_cdhfs_aops;
	    }
	    else
	    {
		    //printk("change to cdfs_cddata_file_operations for i->i_fop_AnyoneUseThis???\n");
		    i->i_fop        = &cdfs_cddata_file_operations;
		    //printk("change to cdfs_cddata_aops for i->i_data.a_ops_AnyoneUseThis???\n");
		    i->i_data.a_ops = &cdfs_cddata_aops;
	    }
    }
    else
    {
	    PRINTI("david: chagne function pointer direction_ACD_mount_04, change to cdfs_cdXA_file_operations for i->i_fop\n");
	    i->i_fop          = &cdfs_cdXA_file_operations;
	    PRINTI("david: chagne function pointer direction_ACD_mount_05, change to cdfs_cdXA_aops for i->i_data.a_ops\n");
	    i->i_data.a_ops   = &cdfs_cdXA_aops;
	    if(CD_TYPE==AVCD_DISC)
	    {
		    //For AVCD audio track file reading...
		    PRINTI("david: chagne function pointer direction_ACD_ls_cp, change to cdfs_cdda_file_operations for i->i_fop\n");
		    i->i_fop          = &cdfs_cdda_file_operations;
		    PRINTI("david: chagne function pointer direction_ACD_ls_cp, change to cdfs_cdda_aops for i->i_data.a_ops\n");
		    i->i_data.a_ops   = &cdfs_cdda_aops;
	    }
    }
  }
}

static struct dentry * cdfs_lookup(struct inode *dir, struct dentry *dentry, struct nameidata *nd)
{
  struct inode * inode;
  int i;
  cd * this_cd = cdfs_info(dir->i_sb);

  for(i=0; i<T2I(this_cd->tracks); i++)
  // T2I: Convert track to inode number, i=t+3
  {
  	  //compare file name, in fact track name
	  //PRINT("this_cd->track[i].name: %s  dentry->d_name.name: %s\n", this_cd->track[i].name, dentry->d_name.name);
	  if (!(strcmp(this_cd->track[i].name, dentry->d_name.name)))	//¤񹼀ɦW¬O§_¬ۦP¡A¥ª¬°inode¤W¡A¥k¬°¨t²έn¨D
	  {
		  if ((inode=iget(dir->i_sb, i))==NULL)
		  {
			  PRINT("Can not get inode.....\n");
			  return ERR_PTR(-EACCES);
		  }
		  else
		  {
			  d_add(dentry, inode);
			  return ERR_PTR(0);
		  }
	  }
  }
  return ERR_PTR(-ENOENT);
}


static void cdfs_umount(struct super_block *sb)
{
  int t;
  struct vcd_sb_info *sbi = ISOFS_SB(sb);
  cd * this_cd = cdfs_info(sb);//The same as : cdfs_info(sb)=sb->s_fs_info;
  for (t=0; t<=this_cd->tracks; t++)
  {
	  if ((this_cd->track[T2I(t)].type == DATA) && this_cd->track[T2I(t)].iso_info)
	  {
		  //CDDA will NOT enter here
		  //David modified 0826 for EACD problem
  		  //PRINTM("Mxx_12 kfree   addr = 0x %x for this_cd->track[T2I(t)].iso_info\n",(unsigned int)this_cd->track[T2I(t)].iso_info);
		  //kfree(this_cd->track[T2I(t)].iso_info);	//Mxx
	  }
  }
  // Free & invalidate cache
  PRINTM("M02 kfree   addr = 0x %x for this_cd->cache\n",(unsigned int)this_cd->cache);
  kfree(this_cd->cache);	//M02
  this_cd->cache_sector = -CACHE_SIZE;	//CACHE_SIZE=1

  PRINT("david: Remove /proc _ST\n");
  remove_proc_entry("cd_toc", NULL);
  PRINT("david: Remove /proc _SP\n");

	//0118 add
	#ifdef CONFIG_JOLIET
	if (sbi->s_nls_iocharset)
	{
		unload_nls(sbi->s_nls_iocharset);
		sbi->s_nls_iocharset = NULL;
	}
	#endif

  	if (cdfs_info(sb))
  	{
  		PRINTM("M01 kfree   addr = 0x %x for cdfs_info(sb)\n",(unsigned int)cdfs_info(sb));
  		kfree(cdfs_info(sb));	//M01
  	}
  	PRINTM("M05 kfree   addr = 0x %x for sbi\n",(unsigned int)sbi);
	kfree(sbi);	//M05		//By vcd_main
	sb->s_fs_info = NULL;
	printk("<vcd module> un-mount disc SP for ACD OK!\n");
}

static int cdfs_statfs(struct super_block *sb, struct kstatfs *buf)
{
  cd * this_cd = cdfs_info(sb); //The same as : cdfs_info(sb)=sb->s_fs_info;
  buf->f_type    = CDFS_MAGIC;	//#define CDFS_MAGIC 0xCDDA
  buf->f_bsize   = CD_FRAMESIZE;//#define CD_FRAMESIZE 2048 /* bytes per frame, "cooked" mode */
  buf->f_blocks  = this_cd->size/CD_FRAMESIZE;
  buf->f_namelen = CDFS_MAXFN;	//#define CDFS_MAXFN 128
  buf->f_files   = this_cd->tracks;
  return 0;
}

static int cdfs_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
  struct inode *inode = filp->f_dentry->d_inode;
  int i;
  cd * this_cd = cdfs_info(inode->i_sb);
  for(i=filp->f_pos; i<T2I(this_cd->tracks); i++)
  // T2I: Convert track to inode number, i=t+3
  {
    //PRINT("david: i=%d  filp->f_pos=%d  this_cd->tracks=%d  T2I(this_cd->tracks)=%d\n",i,filp->f_pos,this_cd->tracks,T2I(this_cd->tracks));
    if (filldir(dirent, this_cd->track[i].name, strlen(this_cd->track[i].name), i, i, DT_UNKNOWN) < 0) 
	{
	  return 0;
	}
	filp->f_pos++;	//modify file position
  }
  return 1;
}

static int __init init_iso9660_fs(void)
{
	int err;
	CD_TYPE=NO_DISC;
	err = init_inodecache();
	if (err)
		goto out;
#ifdef CONFIG_ZISOFS
	err = zisofs_init();
	if (err)
		goto out1;
#endif
	err = register_filesystem(&iso9660_fs_type);
	if (err)
		goto out2;

	// start kernel thread
	// added by Frank Ting 8/27/05
	if ((kcdfsd_pid = kernel_thread(kcdfsd_thread, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGHAND)) >0 )
	{
		printk("<vcd module> insert vcd module OK\n");
		return 0;
	}
	else
	{
		printk(FSNAME" kernel_thread failed.\n");
		unregister_filesystem(&iso9660_fs_type);
		destroy_inodecache();
   		return -1;
	}	
	printk("<vcd module> insert vcd module OK\n");
	return 0;
out2:
#ifdef CONFIG_ZISOFS
	zisofs_cleanup();
out1:
#endif
	destroy_inodecache();
out:
	printk("<vcd module> insert vcd module fails........\n");
	return err;
}

static void __exit exit_iso9660_fs(void)
{
	kcdfsd_cleanup_thread();		// added by Frank Ting 8/27/05
	unregister_filesystem(&iso9660_fs_type);
#ifdef CONFIG_ZISOFS
	zisofs_cleanup();
#endif
	destroy_inodecache();
	printk("<vcd module> remove vcd module OK!\n");
}

int find_cd_or_dvd(struct super_block *s)
{
	struct cdrom_generic_command cgc;
	char buffer[16];
	int ret, mmc3_profile;
	memset(&cgc, 0, sizeof(struct cdrom_generic_command));
	if (buffer)
		memset(buffer, 0, sizeof(buffer));
	cgc.buffer = (char *) buffer;
	cgc.buflen = sizeof(buffer);
	cgc.data_direction = CGC_DATA_READ;
	cgc.cmd[0]=GPCMD_GET_CONFIGURATION;	//Operation code==GPCMD_GET_CONFIGURATION==0x46
	cgc.cmd[1]=0;		//no use
	cgc.cmd[2]=0;		/* Starting Feature Number */
	cgc.cmd[3]=0;		/* Starting Feature Number */
	cgc.cmd[8] = sizeof(buffer);		/* Allocation Length */
	cgc.quiet=1;
	ret=cdfs_ioctl( s, CDROM_SEND_PACKET, (unsigned int)&cgc );
	if (ret)
	{
		printk("<vcd module> find_cd_or_dvd error !!!!!!!! ret=0x%x\n",ret);
		mmc3_profile = 0xffff;
	}
	else
		mmc3_profile = (buffer[6] << 8) | buffer[7];
	if (mmc3_profile <= 10)
	{
		//printk("This is a CD for mmc3_profile=%d\n",mmc3_profile);
		return 0;
	}
	else if (mmc3_profile <= 100)
	{
		//printk("This is a DVD for mmc3_profile=%d\n",mmc3_profile);
		return 1;
	}
	else
	{
		return 2;
	}
}
		
module_init(init_iso9660_fs)
module_exit(exit_iso9660_fs)
MODULE_LICENSE("GPL");
/* Actual filesystem name is iso9660, as requested in filesystems.c */
MODULE_ALIAS("vcd");

