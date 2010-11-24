
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fcntl.h>


#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/smp_lock.h>

#include <linux/string.h>
#include <asm/uaccess.h>


#include <linux/usb.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/smp_lock.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/time.h>
#include <linux/statfs.h>
#include <linux/genhd.h>

#include <asm-mips/types.h>

#include <linux/kobject.h>
#include <linux/kobject_uevent.h>
#include <linux/mount.h>
#include "ptpfs.h"
#include "ptp.h"

MODULE_LICENSE("GPL");

#define MAX_DEVICES		8
#define PTPFS_MAGIC		0x245234da
#define NODEV				0

#define USB_INTERFACE_CLASS		6
#define USB_INTERFACE_SUBCLASS	1
#define USB_INTERFACE_PROTOCOL	1

#define BUFFER_SIZE 1024 /* buffer for the hotplug env */
#define NUM_ENVP 32 /* number of env pointers */ 

static struct ptpfs_usb_device_info  *ptp_devices[MAX_DEVICES];
static DECLARE_MUTEX (ptp_devices_mutex);
static DECLARE_MUTEX (ptp_mount_mutex);
static struct ptpfs_input *input_data;

void force_delete(struct inode *inode)
{
         /*
    * Kill off unused inodes ... iput() will unhash and
    * delete the inode if we set i_nlink to zero.
          */
	if (atomic_read(&inode->i_count) == 1)
		inode->i_nlink = 0;
}

static inline void ptp_usb_device_delete (struct ptpfs_usb_device_info *dev)
{
    ptp_devices[dev->minor] = NULL;
    kfree(dev);
}

void ptpfs_set_inode_info(struct inode *ino, struct ptp_object_info *object)
{

	if (object->object_format!=PTP_OFC_Association && object->association_type != PTP_AT_GenericFolder)
	{
        ino->i_size = object->object_compressed_size;
        ino->i_ctime.tv_sec = object->capture_date;
        ino->i_mtime.tv_sec = ino->i_atime.tv_sec = object->modification_date;
	}
	else
	{
		PTPFSINO(ino)->type = INO_TYPE_DIR;
	}
	PTPFSINO(ino)->storage = object->storage_id;
}

void ptpfs_free_inode_data(struct inode *ino)
{
    int x;
    struct ptpfs_inode_data* ptpfs_data = PTPFSINO(ino);
    switch (ptpfs_data->type)
	{
		case INO_TYPE_DIR:
		case INO_TYPE_STGDIR:
		for (x = 0; x < ptpfs_data->data.dircache.num_files; x++)
		{
            kfree(ptpfs_data->data.dircache.file_info[x].filename);
		}
		kfree(ptpfs_data->data.dircache.file_info);
		ptpfs_data->data.dircache.file_info = NULL;
		ptpfs_data->data.dircache.num_files= 0;
		break;
	}
}

static void ptpfs_put_inode(struct inode *ino)
{
//    printk(KERN_INFO "%s - %ld   count: %d\n",  __FUNCTION__,ino->i_ino,ino->i_count);
/*
	if((PTPFSINO(ino)->data.dircache.file_info)!=0)
		printk("========== ptpfs_put_inode name : %d =====================\n",PTPFSINO(ino)->data.dircache.file_info->handle);	
	else
		printk("========== ptpfs_put_inode name : %x =====================\n",PTPFSINO(ino)->data.dircache.file_info);	
*/
    ptpfs_free_inode_data(ino);
    force_delete(ino);
}

struct inode *ptpfs_get_inode(struct super_block *sb, int mode, int dev, int ino)
{
    //printk(KERN_INFO "%s -- %d\n",  __FUNCTION__, ino);
    struct inode * inode = new_inode(sb);

    if (inode)
	{
        inode->i_mode = mode;

        inode->i_uid = current->fsuid;
        inode->i_gid = current->fsgid;

        inode->i_blksize = PAGE_CACHE_SIZE;
        inode->i_blocks = 0;	
        inode->i_size = 0;
        inode->i_rdev = NODEV;
        inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
        inode->i_version = 1;
        inode->i_mapping->a_ops = &ptpfs_fs_aops;
        if (ino)
		{
            inode->i_ino = ino;
		}
	
		PTPFSINO(inode) = (struct ptpfs_inode_data *)kmalloc(sizeof(struct ptpfs_inode_data), GFP_KERNEL); 

        memset(PTPFSINO(inode),0,sizeof(struct ptpfs_inode_data));
        insert_inode_hash(inode);
        switch (mode & S_IFMT)
		{
		case S_IFREG:
			inode->i_fop = &ptpfs_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &ptpfs_dir_inode_operations;			
			inode->i_fop = &ptpfs_dir_operations;			
			break;
		case S_IFLNK:
			inode->i_op = &page_symlink_inode_operations;
			break;
		default:
			init_special_inode(inode, mode, dev);
			break;
        }
    }
    return inode;
}

static void ptpfs_put_super (struct super_block * sb)
{
    //printk(KERN_INFO "%s\n",  __FUNCTION__);
    printk("<ptp module> umount ptp device ST\n");

    ptp_free_device_info(PTPFSSB(sb)->deviceinfo);
    kfree(PTPFSSB(sb)->deviceinfo);

	//if disconnect, close_type = 2  and not necessary to closesession 
	if (PTPFSSB(sb)->usb_device->close_type != 2) 
	{
		if (ptp_closesession(PTPFSSB(sb))!=PTP_RC_OK)
		{
			printk(KERN_INFO "Could not close session\n");
		}
	}
	//disconnect will check that unmount is done.
	if (PTPFSSB(sb)->usb_device->close_type == 0)
	{
		PTPFSSB(sb)->usb_device->fs_already_mount = 0;
		PTPFSSB(sb)->usb_device->close_type = 1; 
	}	
	else if (PTPFSSB(sb)->usb_device->close_type == 2)  //disconnect is done , free it 
	{
		kfree(PTPFSSB(sb)->usb_device);
		/*
		**	these two pointers indicate same area, but ptp_probe will check ptp_devices[x]
		**	So, we need to set ptp_devices[x] = NULL.
		*/
		ptp_devices[PTPFSSB(sb)->usb_device->minor] = NULL;
	    PTPFSSB(sb)->usb_device->open_count--;
		// kill_sb will free sb, so usb_device is not allocated to any sb
		PTPFSSB(sb)->usb_device->alloc_devices = 0; 
		PTPFSSB(sb)->usb_device->fs_already_mount = 0;

		PTPFSSB(sb)->usb_device = NULL;
		if ( PTPFSSB(sb)->private_data ){
			ptp_free_data_buffer(PTPFSSB(sb)->private_data);
			kfree(PTPFSSB(sb)->private_data->ptp_temp);
			kfree(PTPFSSB(sb)->private_data);
		}
		if (PTPFSSB(sb)->buffer)
			kfree(PTPFSSB(sb)->buffer);

		kfree(PTPFSSB(sb)); 
		PTPFSSB(sb) = NULL; 
	}
	else	
		PTPFSSB(sb)->usb_device->fs_already_mount = 0;

    printk("<ptp module> umount ptp device SP\n");

}
//static int ptpfs_statfs(struct super_block *sb, struct statfs *buf)
static int ptpfs_statfs(struct super_block *sb, struct kstatfs *buf)
{
	//printk(KERN_INFO "%s\n",  __FUNCTION__);
    struct ptp_storage_ids storageids;
    int x;

    buf->f_type = PTPFS_MAGIC;
    buf->f_bsize = PAGE_CACHE_SIZE;
    buf->f_namelen = 255;

    buf->f_bsize = 1024;
    buf->f_blocks = 0;
    buf->f_bfree = 0;

    //down(&params->ptp_lock);
    if (ptp_getstorageids(PTPFSSB(sb), &storageids)!=PTP_RC_OK)
	{
        printk(KERN_INFO "Error getting storage ids\n");
        storageids.n = 0;
        storageids.storage = NULL;
        buf->f_blocks = 1024;
        buf->f_bfree = 0;
	}

    for (x = 0; x < storageids.n; x++)
	{
        struct ptp_storage_info storageinfo;
        if ((storageids.storage[x]&0x0000ffff)==0) continue;

        memset(&storageinfo,0,sizeof(storageinfo));
        if (ptp_getstorageinfo(PTPFSSB(sb), storageids.storage[x],&storageinfo)!=PTP_RC_OK)
	        {
            printk(KERN_INFO "Error getting storage info\n");
        	}
        else
	        {
            buf->f_blocks += storageinfo.max_capability;
            buf->f_bfree += storageinfo.free_space_in_bytes;
            ptp_free_storage_info(&storageinfo);
        	}

	}
    ptp_free_storage_ids(&storageids);
    //up(&params->ptp_lock);
    buf->f_blocks /= 1024;
    buf->f_bfree /= 1024;
    buf->f_bavail = buf->f_bfree;
    return 0;
}

struct super_operations ptpfs_ops = {
	statfs:		ptpfs_statfs,
	put_super:		ptpfs_put_super,
	put_inode:		ptpfs_put_inode,
	drop_inode:	generic_delete_inode,
};

char * ___strtok;

char * strtok(char * s,const char * ct)
{
		char *sbegin, *send;

		sbegin  = s ? s : ___strtok;
       if (!sbegin) {
			return NULL;
		}
		sbegin += strspn(sbegin,ct);
		if (*sbegin == '\0') {
			___strtok = NULL;
			return( NULL );
		}
		send = strpbrk( sbegin, ct);
		if (send && *send != '\0')
			*send++ = '\0';
		___strtok = send;
		return (sbegin);
}

static int ptpfs_parse_options(char *options, uid_t *uid, gid_t *gid)
{
	char *this_char, *value, *rest;

	this_char = NULL;
	if ( options )
		this_char = strtok(options,",");
	for ( ; this_char; this_char = strtok(NULL,","))
	{
		if ((value = strchr(this_char,'=')) != NULL)		
			*value++ = 0;		
		else
		{
			printk(KERN_ERR 
			"ptpfs: No value for mount option '%s'\n",this_char);
			return 1;
		}

		if (!strcmp(this_char,"uid"))
		{
			if (!uid)
				continue;
			*uid = simple_strtoul(value,&rest,0);
			if (*rest)
				goto bad_val;
		}
		else if (!strcmp(this_char,"gid"))
		{
			if (!gid)
				continue;
			*gid = simple_strtoul(value,&rest,0);
			if (*rest)
				goto bad_val;
		}
		else
		{
			printk(KERN_ERR "ptpfs: Bad mount option %s\n",this_char);
			return 1;
		}
	}
	return 0;

	bad_val:
	printk(KERN_ERR "ptpfsf: Bad value '%s' for mount option '%s'\n",value, this_char);
	return 1;
}

int ptp_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	printk("<ptp module> ptp_probe ST\n");
	struct ptpfs_usb_device_info *pdev = NULL;
	int N_endpoints;
	int i;
	int j;
	int x; //the sn of ptp_devices , the same as minor -- add by evan
	struct usb_endpoint_descriptor *endpoint;
	struct usb_host_interface *iface_desc;
    iface_desc = interface->cur_altsetting;


/*
	printk("========= ptp_probe ==================\n");	
	printk("========= ptp_probe interface : %x ==================\n",interface);	
	printk("========= ptp_probe id->idProduct : %x ==================\n",id->idProduct);	
	printk("========= ptp_probe id->idVendor : %x ==================\n",id->idVendor);				
	printk("========= ptp_probe id->bcdDevice_lo : %x ==================\n",id->bcdDevice_lo);	
	printk("========= ptp_probe id->bcdDevice_hi : %x ==================\n",id->bcdDevice_hi);	

	printk("=== ptp_probe iface_desc->desc.bLength : %d ===\n",iface_desc->desc.bLength);	
	printk("=== ptp_probe iface_desc->desc.bDescriptorType : %d ===\n",iface_desc->desc.bDescriptorType);
	printk("=== ptp_probe iface_desc->desc.bInterfaceNumber : %d ===\n",iface_desc->desc.bInterfaceNumber);	
	printk("== ptp_probe iface_desc->desc.bAlternateSetting : %d =\n",iface_desc->desc.bAlternateSetting);		
	printk("=== ptp_probe iface_desc->desc.bNumEndpoints : %d ===\n",iface_desc->desc.bNumEndpoints);	
	printk("=== ptp_probe iface_desc->desc.bInterfaceClass : %d ===\n",iface_desc->desc.bInterfaceClass);	
	printk("=== ptp_probe iface_desc->desc.bInterfaceSubClass : %d ==\n",iface_desc->desc.bInterfaceSubClass);	
	printk("=== ptp_probe iface_desc->desc.bInterfaceProtocol : %d ===\n",iface_desc->desc.bInterfaceProtocol);
	printk("=== ptp_probe iface_desc->desc.iInterface : %d ===\n",iface_desc->desc.iInterface);
*/

	N_endpoints = iface_desc->desc.bNumEndpoints; // N_endpoints = 3

	//check the disconnect DSCs are already unmount.
	for (j=0; j< MAX_DEVICES; j++)
	{
		if (ptp_devices[j])
		{			
			if (ptp_devices[j]->close_type == 2) //if disconnect is done but not unmount.
			{
				printk("=========== You must unmount ptp_devices[%d] first  =====================\n",j);
				printk("=========== You must unmount ptp_devices[%d] first  =====================\n",j);
				printk("=========== You must unmount ptp_devices[%d] first  =====================\n",j);
				printk("=========== You must unmount ptp_devices[%d] first  =====================\n",j);
				printk("=========== You must unmount ptp_devices[%d] first  =====================\n",j);
				printk("=========== You must unmount ptp_devices[%d] first  =====================\n",j);
				printk("=========== You must unmount ptp_devices[%d] first  =====================\n",j);
//				return -1;
			}	
		}
	}
		
	down(&ptp_devices_mutex);		
	for ( x=0 ; x < MAX_DEVICES ; x++)
	{
		if (ptp_devices[x] == NULL)
		{
			pdev = (struct ptpfs_usb_device_info *)kmalloc(sizeof(struct ptpfs_usb_device_info), GFP_KERNEL);
			memset(pdev,0,sizeof(struct ptpfs_usb_device_info));
			init_MUTEX (&pdev->sem);
	
			pdev->close_type = 0;
			pdev->alloc_devices = 0;
			pdev->fs_already_mount = 0;

			pdev->udev = interface_to_usbdev (interface);
			pdev->minor = x;
			interface->minor = pdev->minor; //minor could be 0-7								

			for ( i = 0; i < N_endpoints; ++i)
        		{
				endpoint = &iface_desc->endpoint[i].desc;
       
				if ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
				{
					if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
					{
						/*
						printk("========== Bulk_In =====================\n");
						printk("========== bmAttributes : %d =====================\n",endpoint->bmAttributes);
						printk("========== XFER_BULK : %d =====================\n",USB_ENDPOINT_XFER_BULK);
						printk("========== EpAddr : %x =====================\n",endpoint->bEndpointAddress);
						printk("========== USB_DIR_IN : %x =====================\n",USB_DIR_IN);
						*/
						pdev->inep = endpoint->bEndpointAddress;
					} //end if -- USB_DIR_IN
					else if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT)
					{
						/*
						printk("========== Bulk_OUT =====================\n");
						printk("========== bmAttributes : %d =====================\n",endpoint->bmAttributes);
						printk("========== XFER_BULK : %d =====================\n",USB_ENDPOINT_XFER_BULK);
						printk("========== EpAddr : %x =====================\n",endpoint->bEndpointAddress);
						printk("========== USB_DIR_OUT : %x =====================\n",USB_DIR_OUT);
						*/
						pdev->outep = endpoint->bEndpointAddress;
					} //end if -- USB_DIR_OUT					
				} //end if	-- USB_ENDPOINT_XFER_BULK 		
				else if ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
				{
					/*
					printk("========== Interrupt =====================\n");
					printk("========== bmAttributes : %d =====================\n",endpoint->bmAttributes);
					printk("========== XFER_BULK : %d =====================\n",USB_ENDPOINT_XFER_BULK);
					printk("========== EpAddr : %x =====================\n",endpoint->bEndpointAddress);
					printk("========== USB_ENDPOINT_XFER_INT : %x =====================\n",USB_ENDPOINT_XFER_INT);
					*/
					pdev->intep = endpoint->bEndpointAddress;
				} //end if -- USB_ENDPOINT_XFER_INT		
    		} //end for -- N_endpoints
			ptp_devices[x]=pdev;			
			break;
		} //end if	-- ptp_devices[x] == NULL			
	}  //end for -- x < MAX_DEVICES		

	up(&ptp_devices_mutex);
	if (pdev == NULL)
	{
		printk("Too many PTP devices plugged in, can not handle this device.\n");
		printk("<ptp module> ptp_probe SP fail\n");
		return -1;
	}
	else
	{
//		int index = x;
//		char dir_name[32];
//		sprintf(dir_name, "DSC_%c", 'a' + index % 26);
//		printk("==== DSC : %s , kobj_name : %s , k_name : %s ====\n",dir_name,interface->dev.kobj.name,interface->dev.kobj.k_name);

		char *name;
		struct device *dev = &interface->dev;

		ptp_devices[x]->kobj_name = 	interface->dev.kobj.k_name;
		printk("==== kobj_name : %s ====\n",ptp_devices[x]->kobj_name);		
		printk("<ptp module> ptp_probe SP\n");

		return 0;   	
	}
}

static int ptp_disconnect(struct usb_interface *intf)
{
	int x;
	printk("<ptp module> ptp_disconnect ST\n");

	if ( ptp_devices[intf->minor]->fs_already_mount == 0)  
	{
		kfree(ptp_devices[intf->minor]);
		ptp_devices[intf->minor]=NULL;
		usb_set_intfdata(intf, NULL);
		printk("<ptp module> ptp_disconnect SP 1\n");
		return 0;
	}
	
	if (ptp_devices[intf->minor]->close_type == 0) //disconnect and unmount are not done
	{	
		//unmount will check that disconnect is done. Let unmount free data.
		ptp_devices[intf->minor]->close_type = 2; 
	}
	else if (ptp_devices[intf->minor]->close_type == 1)  //unmount is done , disconnect free data now. 
	{	
		kfree(ptp_devices[intf->minor]);
		ptp_devices[intf->minor]=NULL;
	}	
	usb_set_intfdata(intf, NULL);
	printk("<ptp module> ptp_disconnect SP 2\n");
	return 0;
}

static struct usb_device_id ptp_table[] = {
	{	USB_INTERFACE_INFO( USB_INTERFACE_CLASS , USB_INTERFACE_SUBCLASS , USB_INTERFACE_PROTOCOL )},
	{	}	
};

static struct usb_driver ptpfs_usb_driver = {
	.name =		"ptp_driver",
	.probe =		ptp_probe,
	.id_table =	ptp_table,
	.disconnect =	ptp_disconnect,
};
MODULE_DEVICE_TABLE (usb,ptp_table);



//static int ptp_fill_super(struct super_block *sb, void *data, int silent)
static long ptp_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode * inode;
    struct dentry * root;
    int mode   = S_IRWXUGO | S_ISVTX | S_IFDIR; 
    uid_t uid = 0;
    gid_t gid = 0;
    int x;
	printk("<ptp module> mount ptp device ST\n");

	for (x = 0; x < MAX_DEVICES; x++)
	{
		if (ptp_devices[x])
		{
			if (strcmp(input_data->kobj_name, ptp_devices[x]->kobj_name)==0 
			&& ptp_devices[x]->fs_already_mount !=1 )
			{
				printk("===== mount ptp_devices[%d] , kobj_name:%s =====\n",x,ptp_devices[x]->kobj_name);
				break;
			}
		}
		if (x == MAX_DEVICES-1)
		{
			printk("No such device or address\n");	
			return -ENXIO;
		}
	}

	if (ptpfs_parse_options (data,&uid, &gid))
		return 1;

    sb->s_blocksize = PAGE_CACHE_SIZE;
    sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
    sb->s_magic = PTPFS_MAGIC;
    sb->s_op = &ptpfs_ops;

	PTPFSSB(sb) = kmalloc(sizeof(struct ptpfs_sb_info), GFP_KERNEL); 
    memset(PTPFSSB(sb), 0, sizeof(struct ptpfs_sb_info));
    PTPFSSB(sb)->byteorder = PTP_DL_LE;
    PTPFSSB(sb)->fs_gid = gid;
    PTPFSSB(sb)->fs_uid = uid;

	PTPFSSB(sb)->buffer = kmalloc(sizeof((int)PAGE_SIZE), GFP_KERNEL); 
    memset(PTPFSSB(sb)->buffer, 0, sizeof((int)PAGE_SIZE));

    down(&ptp_devices_mutex);
	
	down(&ptp_devices[x]->sem);
	PTPFSSB(sb)->usb_device = ptp_devices[x];
	PTPFSSB(sb)->usb_device->open_count++;
	up(&ptp_devices[x]->sem);	

	// If our device does not open , there is no usb_device info.
    if (PTPFSSB(sb)->usb_device == NULL)
	{
        up(&ptp_devices_mutex);

        printk(KERN_INFO "Could not find a suitable device or mount the same mount point\n");
        return -ENXIO;	// No such device or address 
	}

	if (ptp_opensession(PTPFSSB(sb),1)!=PTP_RC_OK)
	{
		printk(KERN_INFO "Could not open session\n");
		goto error;
	}

    inode = ptpfs_get_inode(sb, S_IFDIR | mode, 0, 0);
    if (!inode) goto error;

    inode->i_op = &ptpfs_rootdir_inode_operations;
    inode->i_fop = &ptpfs_rootdir_operations;

    inode->i_uid = uid;
    inode->i_gid = gid;
    root = d_alloc_root(inode);
    if (!root)
	{
        iput(inode);
        goto error;
	}
    sb->s_root = root;


	PTPFSSB(sb)->deviceinfo = (struct ptp_device_info *)kmalloc(sizeof(struct ptp_device_info),GFP_KERNEL);
	memset(PTPFSSB(sb)->deviceinfo,0,sizeof(struct ptp_device_info));	

    if (ptp_getdeviceinfo(PTPFSSB(sb), PTPFSSB(sb)->deviceinfo)!=PTP_RC_OK)
	{
        printk(KERN_ERR "Could not get device info\nTry to reset the camera.\n");
        goto error;
	}
	PTPFSSB(sb)->usb_device->fs_already_mount = 1; 
    up(&ptp_devices_mutex);
	
	
	//	fs/super.c  =>error return minus number, ex: -1~-34
    printk("<ptp module> mount ptp device SP and OK\n");
    return 0;			

	// suspend.. 	change mount type
    error:
    down(&PTPFSSB(sb)->usb_device->sem);
    PTPFSSB(sb)->usb_device->open_count--;
    up(&PTPFSSB(sb)->usb_device->sem);
    up(&ptp_devices_mutex);
    printk("<ptp module> mount ptp device SP and fails\n");
    return -ENXIO;

}
static struct super_block *ptp_get_sb(struct file_system_type *fst, int flags, const char *name, void *data)
{
	down(&ptp_mount_mutex);
	void *get_sb_nodev_temp;
	input_data = (struct ptpfs_input *)kmalloc(sizeof(struct ptpfs_input),GFP_KERNEL);
	memset(input_data,0,sizeof(&input_data));

	input_data->kobj_name = name;
	get_sb_nodev_temp = get_sb_nodev(fst, flags, data, ptp_fill_super);	
	kfree(input_data);
	up(&ptp_mount_mutex);

	return get_sb_nodev_temp;
}

static void ptp_kill_sb(struct super_block *sb){
	/*
	if (sb->s_root)
		d_genocide(sb->s_root);
	*/
	kill_anon_super(sb);	
}


struct file_system_type ptpfs_fs_type = {
	.name =		"ptpfs",
	.get_sb =		ptp_get_sb,
	.kill_sb = 	ptp_kill_sb,
	.owner =		THIS_MODULE, 
};


static int init_ptp_driver(void)
{
	int driver_result;
	int fs_result;
	printk("<ptp module> insert ptp module ST\n");

    /* register this driver with the USB subsystem */

	driver_result = usb_register(&ptpfs_usb_driver);
	if (driver_result < 0)
	{
		printk("usb_register failed for the ptpfs driver. Error number %d\n", driver_result);
		return -1;
	}

	fs_result = register_filesystem(&ptpfs_fs_type);
	if (fs_result < 0)
	{
		printk("register_filesystem failed for the ptpfs driver. Error number %d\n", fs_result);
		return -1;
	}
	printk("<ptp module> insert ptp module OK\n");

	return 0;
}
static void exit_ptp_driver(void)
{
	printk("<ptp module> remove ptp module ST\n");
	unregister_filesystem(&ptpfs_fs_type);
	usb_deregister(&ptpfs_usb_driver);
	printk("<ptp module> remove ptp module SP\n");
}


module_init(init_ptp_driver);
module_exit(exit_ptp_driver);

