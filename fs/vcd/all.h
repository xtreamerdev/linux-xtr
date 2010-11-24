/*
#define PRDEBUGT
#define PRDEBUGC
#define PRDEBUGI
#define PRDEBUG
#define PRDEBUGM
*/



#ifndef CONFIG_CDFS_VERSION
#define CONFIG_CDFS_VERSION "2.6.12"
#endif

#define FSNAME        "vcd_module"
#define FSNAME_all    "all"
#define FSNAME_hfs    "hfs"
#define FSNAME_inode  "inode"
#define FSNAME_toc    "toc"
#define FSNAME_cdxa   "cdxa"
#define FSNAME_cddata "cddata"
#define FSNAME_namei  "namei"
#define FSNAME_proc   "proc"
#define FSNAME_mem    "mem"
#define VERSION CONFIG_CDFS_VERSION

#ifdef PRDEBUGM
# define PRINTM(format, arg...) printk(FSNAME_mem "->" format, ## arg)
#else
# define PRINTM(format, arg...) 
#endif

#ifdef PRDEBUGT
# define PRINTT(format, arg...) printk(FSNAME_toc "->" format, ## arg)
#else
# define PRINTT(format, arg...) 
#endif

#ifdef PRDEBUGC
# define PRINTC(format, arg...) printk(FSNAME_cdxa "->" format, ## arg)
#else
# define PRINTC(format, arg...) 
#endif

#ifdef PRDEBUGI
# define PRINTI(format, arg...) printk(FSNAME_inode "->" format, ## arg)
#else
# define PRINTI(format, arg...) 
#endif

#ifdef PRDEBUG
# define PRINT(format, arg...) printk(FSNAME_all "->" format, ## arg)
#else
# define PRINT(format, arg...) 
#endif

#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/iso_fs.h>
#include <asm/unaligned.h>

#define CDFS_MAGIC 0xCDDA
#define CDFS_MAXFN 128

#define TRUE 1
#define FALSE 0

#define MAX_TRACKS 100
#define WAV_HEADER_SIZE 44
#define READ_AHEAD 0

#define NONE  0
#define AUDIO 1
#define DATA  2
#define BOOT  3
#define HFS   4

#define AUDIO_NAME       "track%02d.cdda"
#define RAW_AUDIO_NAME   "track%02d.raw"
#define AVI_AUDIO_NAME   "track%02d.avi"
#define DATA_NAME_ISO    "sessions_1%d.iso"
#define DATA_NAME_SINGLE "session%d.iso"
#define DATA_NAME_VCD    "videocd%d.mpeg"

#define UID   0
#define GID   0
#define MODE  (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH) 

/* from cdfs */
/*
#define cdfs_info(X)    ((X)->s_fs_info)
*/

/* from vcd */
#define cdfs_info2(X)    ((X)->s_fs_info)
#define cdfs_info(X)    (((struct vcd_sb_info *)((X)->s_fs_info))->s_cd_info)

#define cdfs_bread(S,O,L) __bread((S)->s_bdev,(O),(L))

#define CACHE_SIZE 1

/* Convert track to inode number, i=t+3 */
#define T2I(X) ((X)+3)

/*
 sb        ->  inode 0
 .         ->  inode 1
 ..        ->  inode 2
 track-01  ->  inode 3  (track 0!)
 track-02  ->  inode 4  (track 1!)
 etc.
*/

#define NO_DISC    0
#define VCD_DISC   1
#define ACD_DISC   2
#define AVCD_DISC  3
#define SVCD_DISC  4
#define EACD_DISC  5
#define DATA_DISC  6
#define Kodak_DISC 7
#define DVD_DISC   8
#define OTHER_DISC 9
#define BAD_DISC   10

struct proc_data
{
	char cd_type[50];
};

typedef struct _inode_info_ {
	int xa_data_size;                           /* only for xa data tracks */
	int xa_data_offset;                         /* only for xa data tracks */
} inode_info;

typedef struct _iso_track_ {
  unsigned start;
  unsigned stop;
} iso_track;

typedef struct _track_info {
  unsigned size;                              /* in bytes */
  unsigned start_lba;
  unsigned stop_lba;
  unsigned hfs_offset;                        /* in 512 bytes */
  unsigned track_size;                        /* in bytes */
  unsigned iso_size;                          /* in bytes */
  unsigned type;                              /* audio, data of boot */
  char name[99];
  unsigned inode;
  time_t time;                                /* only for data tracks */
  struct iso_primary_descriptor * iso_info;   /* only for data tracks */
  int xa_data_size;                           /* only for xa data tracks */
  int xa_data_offset;                         /* only for xa data tracks */
  char bootID[99];
  int avi; 			/* only for raw audio tracks */
  int avi_offset;
  int avi_swab; 		/* swap audio bytes? */
  int cd_bridge;		/* CD-RTOS CD-BRIDGE */  
} track_info;

typedef struct _cd_ {
  mode_t mode;
  gid_t gid;
  uid_t uid;    
  int single;
  unsigned size;                      /* bytes */
  unsigned tracks;                    /* number of tracks */
  track_info track[MAX_TRACKS];       /* info per track */
  int nr_iso_sessions;
  iso_track lba_iso_sessions[MAX_TRACKS];
  char videocd_type[9];
  char videocd_title[17];
  char * cache;
  int cache_sector;
  int raw_audio;
  int toc_scsi;
} cd;

/*
 * vcd super-block data in memory
 */
struct vcd_sb_info {
	unsigned long s_ninodes;
	unsigned long s_nzones;
	unsigned long s_firstdatazone;
	unsigned long s_log_zone_size;
	unsigned long s_max_size;
	
	unsigned char s_high_sierra; /* A simple flag */
	unsigned char s_mapping;
	int           s_rock_offset; /* offset of SUSP fields within SU area */
	unsigned char s_rock;
	unsigned char s_joliet_level;
	unsigned char s_utf8;
	unsigned char s_cruft; /* Broken disks with high byte of length containing junk */
	unsigned char s_unhide;
	unsigned char s_nosuid;
	unsigned char s_nodev;
	unsigned char s_nocompress;

	mode_t s_mode;
	gid_t s_gid;
	uid_t s_uid;
	struct nls_table *s_nls_iocharset; /* Native language support table */
	
	cd *s_cd_info;		/* track information */
};

typedef unsigned char byte; 

int cdfs_read_proc(char *buf, char **start, off_t offset, int len, int *eof, void *data );

extern struct dentry_operations isofs_dentry_ops[];
extern struct file_operations cdfs_cdda_file_operations;
extern struct file_operations cdfs_cdXA_file_operations;
extern struct file_operations cdfs_cddata_file_operations;
extern struct file_operations cdfs_cdhfs_file_operations;

extern struct address_space_operations cdfs_cdda_aops;
extern struct address_space_operations cdfs_cdda_raw_aops;
extern struct address_space_operations cdfs_cdXA_aops;
extern struct address_space_operations cdfs_cddata_aops;
extern struct address_space_operations cdfs_cdhfs_aops;

int cdfs_toc_read(struct super_block *sb,int mounting_acd);
int cdfs_toc_read_cd(struct super_block *sb,int mounting_acd);
int cdfs_toc_read_dvd(struct super_block *sb);
int find_session_no(struct super_block *sb);
int find_track_info(struct super_block *sb);
int find_track_info2(struct super_block *sb, int track_no);
int find_contents_in_cd( struct super_block *sb );
int find_size_in_track_in_cd( struct super_block *sb );
int find_cd_or_dvd(struct super_block *s);
int find_disc_info(struct super_block *sb);
int isEACD(struct super_block *sb);

int readtocentry(struct super_block *sb);

time_t cdfs_constructtime(char * time);
unsigned cdfs_constructsize(char * size);
void cdfs_constructMSFsize(char * result, unsigned length);
int cdfs_ioctl(struct super_block *s, int cmd, unsigned long arg);
struct iso_primary_descriptor * cdfs_get_iso_info(struct super_block *sb, int track_no);
int cdfs_get_hfs_info(struct super_block *sb, unsigned track);
void cdfs_check_bootable(struct super_block *sb);
int cdfs_get_XA_info(struct super_block * sb, int inode);
int vcdfs_get_xa_sector_info(struct super_block * sb, unsigned  int start_lba, unsigned int *data_size, unsigned int *data_offset);
/* void cdfs_copy_from_cdXA(struct super_block * sb, int inode, unsigned int start, unsigned int stop, char * buf);*/
int cdfs_copy_from_cdXA(struct super_block * sb, struct inode * inode, unsigned int start, unsigned int stop, char * buf);
void cdfs_copy_from_cddata(struct super_block * sb, int inode, unsigned int start,
	unsigned int stop, char * buf);
void cdfs_copy_from_cdhfs(struct super_block * sb, int inode, unsigned int start,
	 unsigned int stop, char * buf);
int cdfs_cdda_file_read(struct inode * inode, char * buf, size_t count, unsigned start /*loff_t *ppos*/,int raw);
/*
extern int get_joliet_filename(struct iso_directory_record * de, unsigned char *outname, struct inode * inode);
*/
int kcdfsd_add_cdXA_request(struct file * file, struct page *page);
int kcdfsd_add_cddata_request(struct file * file, struct page *page);
int kcdfsd_add_cdda_request(struct file * file, struct page *page);
int kcdfsd_add_cdda_raw_request(struct file * file, struct page *page);
int kcdfsd_add_cdhfs_request(struct file * file, struct page *page);

int kcdfsd_add_request(struct dentry *dentry, struct page *page, unsigned type);
int kcdfsd_thread(void *unused);
void kcdfsd_cleanup_thread(void);
extern int kcdfsd_pid;

/* REQUEST TYPES */
#define CDDA_REQUEST     1
#define CDXA_REQUEST     2
#define CDHFS_REQUEST    3
#define CDDATA_REQUEST   4
#define CDDA_RAW_REQUEST 5

/* ----------------------------------------------------------------------- *
 *   
 *   Copyright 2001 H. Peter Anvin - All Rights Reserved
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 675 Mass Ave, Cambridge MA 02139,
 *   USA; either version 2 of the License, or (at your option) any later
 *   version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

/*
 * Prototypes for functions exported from the compressed isofs subsystem
 */

#ifdef CONFIG_ZISOFS
extern struct address_space_operations zisofs_aops;
extern int __init zisofs_init(void);
extern void zisofs_cleanup(void);
#endif

/* rock.h */
/* These structs are used by the system-use-sharing protocol, in which the
   Rock Ridge extensions are embedded.  It is quite possible that other
   extensions are present on the disk, and this is fine as long as they
   all use SUSP */

struct SU_SP{
  unsigned char magic[2];
  unsigned char skip;
} __attribute__((packed));

struct SU_CE{
  char extent[8];
  char offset[8];
  char size[8];
};

struct SU_ER{
  unsigned char len_id;
  unsigned char len_des;
  unsigned char len_src;
  unsigned char ext_ver;
  char data[0];
} __attribute__((packed));

struct RR_RR{
  char flags[1];
} __attribute__((packed));

struct RR_PX{
  char mode[8];
  char n_links[8];
  char uid[8];
  char gid[8];
};

struct RR_PN{
  char dev_high[8];
  char dev_low[8];
};


struct SL_component{
  unsigned char flags;
  unsigned char len;
  char text[0];
} __attribute__((packed));

struct RR_SL{
  unsigned char flags;
  struct SL_component link;
} __attribute__((packed));

struct RR_NM{
  unsigned char flags;
  char name[0];
} __attribute__((packed));

struct RR_CL{
  char location[8];
};

struct RR_PL{
  char location[8];
};

struct stamp{
  char time[7];
} __attribute__((packed));

struct RR_TF{
  char flags;
  struct stamp times[0];  /* Variable number of these beasts */
} __attribute__((packed));

/* Linux-specific extension for transparent decompression */
struct RR_ZF{
  char algorithm[2];
  char parms[2];
  char real_size[8];
};

/* These are the bits and their meanings for flags in the TF structure. */
#define TF_CREATE 1
#define TF_MODIFY 2
#define TF_ACCESS 4
#define TF_ATTRIBUTES 8
#define TF_BACKUP 16
#define TF_EXPIRATION 32
#define TF_EFFECTIVE 64
#define TF_LONG_FORM 128

struct rock_ridge{
  char signature[2];
  unsigned char len;
  unsigned char version;
  union{
    struct SU_SP SP;
    struct SU_CE CE;
    struct SU_ER ER;
    struct RR_RR RR;
    struct RR_PX PX;
    struct RR_PN PN;
    struct RR_SL SL;
    struct RR_NM NM;
    struct RR_CL CL;
    struct RR_PL PL;
    struct RR_TF TF;
    struct RR_ZF ZF;
  } u;
};

#define RR_PX 1   /* POSIX attributes */
#define RR_PN 2   /* POSIX devices */
#define RR_SL 4   /* Symbolic link */
#define RR_NM 8   /* Alternate Name */
#define RR_CL 16  /* Child link */
#define RR_PL 32  /* Parent link */
#define RR_RE 64  /* Relocation directory */
#define RR_TF 128 /* Timestamps */

enum isofs_file_format {
	isofs_file_normal = 0,
	isofs_file_sparse = 1,
	isofs_file_compressed = 2,
};
	
/*
 * iso fs inode data in memory
 */
struct iso_inode_info {
	unsigned long i_iget5_block;
	unsigned long i_iget5_offset;
	unsigned int i_first_extent;
	unsigned char i_file_format;
	unsigned char i_format_parm[3];
	unsigned long i_next_section_block;
	unsigned long i_next_section_offset;
	off_t i_section_size;
	struct inode vfs_inode;
};

/*
 * iso9660 super-block data in memory
 */
struct isofs_sb_info {
	unsigned long s_ninodes;
	unsigned long s_nzones;
	unsigned long s_firstdatazone;
	unsigned long s_log_zone_size;
	unsigned long s_max_size;
	
	unsigned char s_high_sierra; /* A simple flag */
	unsigned char s_mapping;
	int           s_rock_offset; /* offset of SUSP fields within SU area */
	unsigned char s_rock;
	unsigned char s_joliet_level;
	unsigned char s_utf8;
	unsigned char s_cruft; /* Broken disks with high
				  byte of length containing
				  junk */
	unsigned char s_unhide;
	unsigned char s_nosuid;
	unsigned char s_nodev;
	unsigned char s_nocompress;

	mode_t s_mode;
	gid_t s_gid;
	uid_t s_uid;
	struct nls_table *s_nls_iocharset; /* Native language support table */
};

static inline struct isofs_sb_info *ISOFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

static inline struct iso_inode_info *ISOFS_I(struct inode *inode)
{
	return container_of(inode, struct iso_inode_info, vfs_inode);
}

static inline int isonum_711(char *p)
{
	return *(u8 *)p;
}
static inline int isonum_712(char *p)
{
	return *(s8 *)p;
}
static inline unsigned int isonum_721(char *p)
{
	return le16_to_cpu(get_unaligned((__le16 *)p));
}
static inline unsigned int isonum_722(char *p)
{
	return be16_to_cpu(get_unaligned((__le16 *)p));
}
static inline unsigned int isonum_723(char *p)
{
	/* Ignore bigendian datum due to broken mastering programs */
	return le16_to_cpu(get_unaligned((__le16 *)p));
}
static inline unsigned int isonum_731(char *p)
{
	return le32_to_cpu(get_unaligned((__le32 *)p));
}
static inline unsigned int isonum_732(char *p)
{
	return be32_to_cpu(get_unaligned((__le32 *)p));
}
static inline unsigned int isonum_733(char *p)
{
	/* Ignore bigendian datum due to broken mastering programs */
	return le32_to_cpu(get_unaligned((__le32 *)p));
}
extern int iso_date(char *, int);

struct inode;		/* To make gcc happy */

extern int parse_rock_ridge_inode(struct iso_directory_record *, struct inode *);
extern int get_rock_ridge_filename(struct iso_directory_record *, char *, struct inode *);
extern int isofs_name_translate(struct iso_directory_record *, char *, struct inode *);

int get_joliet_filename(struct iso_directory_record *, unsigned char *, struct inode *);
int get_acorn_filename(struct iso_directory_record *, char *, struct inode *);

extern struct dentry *isofs_lookup(struct inode *, struct dentry *, struct nameidata *);
extern struct buffer_head *isofs_bread(struct inode *, sector_t);
extern int isofs_get_blocks(struct inode *, sector_t, struct buffer_head **, unsigned long);

extern struct inode *isofs_iget(struct super_block *sb,
                                unsigned long block,
                                unsigned long offset);

/* Because the inode number is no longer relevant to finding the
 * underlying meta-data for an inode, we are free to choose a more
 * convenient 32-bit number as the inode number.  The inode numbering
 * scheme was recommended by Sergey Vlasov and Eric Lammerts. */
static inline unsigned long isofs_get_ino(unsigned long block,
					  unsigned long offset,
					  unsigned long bufbits)
{
	return (block << (bufbits - 5)) | (offset >> 5);
}

/* Every directory can have many redundant directory entries scattered
 * throughout the directory tree.  First there is the directory entry
 * with the name of the directory stored in the parent directory.
 * Then, there is the "." directory entry stored in the directory
 * itself.  Finally, there are possibly many ".." directory entries
 * stored in all the subdirectories.
 *
 * In order for the NFS get_parent() method to work and for the
 * general consistency of the dcache, we need to make sure the
 * "i_iget5_block" and "i_iget5_offset" all point to exactly one of
 * the many redundant entries for each directory.  We normalize the
 * block and offset by always making them point to the "."  directory.
 *
 * Notice that we do not use the entry for the directory with the name
 * that is located in the parent directory.  Even though choosing this
 * first directory is more natural, it is much easier to find the "."
 * entry in the NFS get_parent() method because it is implicitly
 * encoded in the "extent + ext_attr_length" fields of _all_ the
 * redundant entries for the directory.  Thus, it can always be
 * reached regardless of which directory entry you have in hand.
 *
 * This works because the "." entry is simply the first directory
 * record when you start reading the file that holds all the directory
 * records, and this file starts at "extent + ext_attr_length" blocks.
 * Because the "." entry is always the first entry listed in the
 * directories file, the normalized "offset" value is always 0.
 *
 * You should pass the directory entry in "de".  On return, "block"
 * and "offset" will hold normalized values.  Only directories are
 * affected making it safe to call even for non-directory file
 * types. */
static inline void
isofs_normalize_block_and_offset(struct iso_directory_record* de,
				 unsigned long *block,
				 unsigned long *offset)
{
	/* Only directories are normalized. */
	if (de->flags[0] & 2) {
		*offset = 0;
		*block = (unsigned long)isonum_733(de->extent)
			+ (unsigned long)isonum_711(de->ext_attr_length);
	}
}

extern struct inode_operations isofs_dir_inode_operations;
extern struct file_operations isofs_dir_operations;
extern struct address_space_operations isofs_symlink_aops;
extern struct export_operations isofs_export_ops;

/* The following macros are used to check for memory leaks. */
#ifdef LEAK_CHECK
#define free_s leak_check_free_s
#define malloc leak_check_malloc
#define sb_bread leak_check_bread
#define brelse leak_check_brelse
extern void * leak_check_malloc(unsigned int size);
extern void leak_check_free_s(void * obj, int size);
extern struct buffer_head * leak_check_bread(struct super_block *sb, int block);
extern void leak_check_brelse(struct buffer_head * bh);
#endif /* LEAK_CHECK */


