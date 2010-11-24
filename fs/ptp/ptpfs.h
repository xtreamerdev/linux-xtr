#ifndef _PTPFS_FS_SB
#define _PTPFS_FS_SB


// PTP datalayer byteorder
#define PTP_DL_BE			0xF0
#define	PTP_DL_LE			0x0F

#include <asm-mips/types.h>


/*
 * ptpfs super-block data in memory
 */
struct ptpfs_input
{
	/*	input kobj_name 
		ex : mount -t ptpfs 1-1.2:1.0 /mnt/camera_1 , kobj_name = 1-1.2:1.0
	*/ 
	char *kobj_name;
};

struct ptpfs_usb_device_info
{
    /* stucture lock */
    struct semaphore sem;

    /* users using this block */
    int open_count;     

    /* endpoints */
    int inep;
    int outep;
    int intep;

    /*	the usb device */
    struct usb_device *udev;
    __u8    minor;

	/*	store kobject_name to check which device will be mounted (fill_super) */
	char *kobj_name;

	/*	camera disconnect or not *///////add by evan
	int close_type;  // 0 : default , 1 : unmount first , 2 : disconnect first
	
	/*	check if sb is already allocated device or not */ //add by evan
	int alloc_devices;  //0 : not yet allocate , 1 : already allocated
	
	/*	check if the system be mounted or not */
	int fs_already_mount;  //add by evan




};
struct ptpfs_sb_info
{
    /* ptp transaction ID */
    __u32 transaction_id;
    /* ptp session ID */
    __u32 session_id;

    uid_t fs_uid;
    gid_t fs_gid;

    /* data layer byteorder */
    __u8  byteorder;

    struct ptpfs_usb_device_info *usb_device;

    struct ptp_device_info *deviceinfo;
//=======================
	struct ptp_data_buffer *private_data;
	int read_condition; 			// before reading: 0, in the middle of data stream: 1, read over: 2.
	int error_transmit;
//	struct ptp_data_buffer *data_buffer;
//	void *private_data;			// used for reading remainder data ,if the data stream is not read over.
	struct file *filp_temp;		// used for storing last file pointer.
	int ino_temp;					// store the last inode number.
	unsigned char *buffer;		// store the last page data. If offset is the same, we can use it directly.
//=======================
};



struct ptpfs_dirinode_fileinfo
{
    char *filename;
    int handle;
    int mode;
};
struct ptpfs_inode_data
{
    int type;
    __u32 storage;
    struct inode *parent;

    union
	{
		struct
		{
			int num_files;
			struct ptpfs_dirinode_fileinfo *file_info;
		} dircache;
	} data;
};



#define MAX_SEG_SIZE	 (4096*4)		
#define MAX_SEG_NUM 	 6				

#define INO_TYPE_DIR        1
#define INO_TYPE_STGDIR     2

//#define PTPFSSB(x) ((struct ptpfs_sb_info*)(x->u.generic_sbp))
#define PTPFSSB(x) ((struct ptpfs_sb_info *)(x->s_fs_info))
//#define PTPFSINO(x) ((struct ptpfs_inode_data *)(&x->private_data)) 
#define PTPFSINO(x) ((struct ptpfs_inode_data *)(x->u.generic_ip)) 


//free all the allocated data
extern void ptp_free_storage_info(struct ptp_storage_info* storageinfo);
extern void ptp_free_storage_ids(struct ptp_storage_ids* storageids);
extern void ptp_free_object_handles(struct ptp_object_handles *objects);
extern void ptp_free_object_info(struct ptp_object_info *object);
extern void ptp_free_data_buffer(struct ptp_data_buffer *buffer);
extern void ptp_free_device_info(struct ptp_device_info *di);

extern __u16 ptp_getdeviceinfo(struct ptpfs_sb_info *sb, struct ptp_device_info* deviceinfo);
extern int ptp_operation_issupported(struct ptpfs_sb_info *sb, __u16 operation);

// session support
extern __u16 ptp_opensession(struct ptpfs_sb_info *sb, __u32 session);
extern __u16 ptp_closesession(struct ptpfs_sb_info *sb);
// storage id support
extern __u16 ptp_getstorageids(struct ptpfs_sb_info *sb, struct ptp_storage_ids* storageids);
extern __u16 ptp_getstorageinfo(struct ptpfs_sb_info *sb, __u32 storageid, struct ptp_storage_info* storageinfo);
// object support
extern __u16 ptp_getobjecthandles (struct ptpfs_sb_info *sb, __u32 storage,
                                   __u32 objectformatcode, __u32 associationOH,
                                   struct ptp_object_handles* objecthandles);
extern __u16 ptp_getobjectinfo (struct ptpfs_sb_info *sb, __u32 handle,
                                struct ptp_object_info* objectinfo);
extern __u16 ptp_getobject (struct ptpfs_sb_info *sb, __u32 handle, struct ptp_data_buffer *data);

extern __u16 ptp_sendobjectinfo (struct ptpfs_sb_info *sb, __u32* store, 
                                 __u32* parenthandle, __u32* handle,
                                 struct ptp_object_info* objectinfo);
extern __u16 ptp_sendobject(struct ptpfs_sb_info *sb, struct ptp_data_buffer* object, __u32 size);
extern __u16 ptp_deleteobject(struct ptpfs_sb_info *sb, __u32 handle, __u32 ofc);

extern struct inode *ptpfs_get_inode(struct super_block *sb, int mode, int dev, int ino);
extern void ptpfs_set_inode_info(struct inode *ino, struct ptp_object_info *object);
extern void ptpfs_free_inode_data(struct inode *ino);
//========================
extern void force_delete(struct inode *inode);
extern int ptp_io_read(struct ptpfs_sb_info *sb, unsigned char *bytes, unsigned int size);
//========================
extern struct super_operations ptpfs_ops;
extern struct file_operations ptpfs_file_operations;
extern struct file_operations ptpfs_dir_operations;
extern struct address_space_operations ptpfs_fs_aops;
extern struct inode_operations ptpfs_dir_inode_operations;
extern struct file_operations ptpfs_rootdir_operations;
extern struct inode_operations ptpfs_rootdir_inode_operations;

#endif

