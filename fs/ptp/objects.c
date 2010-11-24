/*
 * ptpfs filesystem for Linux.
 *
 * This file is released under the GPL.
 */


#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/string.h>
//#include <linux/locks.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm-mips/types.h>
//=======
#include <linux/seq_file.h>  
//=======
#include <linux/uio.h>
#include <linux/pagemap.h>
#include "ptp.h"              
#include "ptpfs.h"

#ifndef HAVE_ARCH_PAGE_BUG
#define PAGE_BUG(page) do { \
        printk("page BUG for page at %p\n", page); \
        BUG(); \
} while (0)
#endif

#define PASSPORT_FREE 0xff
static DECLARE_MUTEX (ptp_passport_mutex);
static unsigned char passport=PASSPORT_FREE;

static int ptpfs_get_dir_data(struct inode *inode)
{
    int x;
    struct ptpfs_inode_data* ptpfs_data = PTPFSINO(inode);

	//printk(KERN_INFO "===== %s ===== ino : %ld \n",  __FUNCTION__, inode->i_ino);
 
    if (ptpfs_data->data.dircache.file_info == NULL) //
	{ 
		struct ptp_object_handles objects;
       objects.n = 0;
       objects.handles = NULL;

		ptpfs_data->data.dircache.num_files = 0;
       ptpfs_data->data.dircache.file_info = NULL;

       if (ptpfs_data->type == INO_TYPE_DIR)
		{
           if (ptp_getobjecthandles(PTPFSSB(inode->i_sb), ptpfs_data->storage, 0x000000, inode->i_ino,&objects) != PTP_RC_OK)
			{
                return 0;
			}
			}
        	else
        		{
            	if (ptp_getobjecthandles(PTPFSSB(inode->i_sb), inode->i_ino, 0x000000, 0xffffffff ,&objects) != PTP_RC_OK)
            			{
                return 0;
            			}
        		}

        int size = objects.n*sizeof(struct ptpfs_dirinode_fileinfo);
        struct ptpfs_dirinode_fileinfo* finfo = (struct ptpfs_dirinode_fileinfo*)kmalloc(size, GFP_KERNEL);
        ptpfs_data->data.dircache.file_info = finfo;
        if (finfo == NULL)
        	{
            return 0;
        	}
        memset(ptpfs_data->data.dircache.file_info,0,size);
        for (x = 0; x < objects.n; x++)
       		 {
            struct ptp_object_info object;
            memset(&object,0,sizeof(object));
            if (ptp_getobjectinfo(PTPFSSB(inode->i_sb),objects.handles[x],&object)!=PTP_RC_OK)
            		{
                ptpfs_free_inode_data(inode);
                ptp_free_object_handles(&objects);
                return 0;
         		}

            if (ptpfs_data->type == INO_TYPE_STGDIR && 
                (object.storage_id != inode->i_ino || object.parent_object != 0))
            		  {                                                                   
                continue;
            		  }
            int mode = DT_REG;
            if (object.object_format==PTP_OFC_Association && object.association_type == PTP_AT_GenericFolder)
                mode = DT_DIR;

            finfo[ptpfs_data->data.dircache.num_files].filename = object.filename;
            finfo[ptpfs_data->data.dircache.num_files].handle = objects.handles[x];
            finfo[ptpfs_data->data.dircache.num_files].mode = mode;
            ptpfs_data->data.dircache.num_files++;
            object.filename = NULL;

            ptp_free_object_info(&object);
        	  }
        ptp_free_object_handles(&objects);
        }
    return 1;
}


static int ptpfs_readdir(struct file *filp, void *dirent, filldir_t filldir)
{

	//printk(KERN_INFO "===== %s ===== \n",  __FUNCTION__);
    
    struct inode *inode = filp->f_dentry->d_inode;
    struct dentry *dentry = filp->f_dentry;
    struct ptpfs_inode_data* ptpfs_data = PTPFSINO(inode);
    int offset = filp->f_pos;
    int offset2 = filp->f_pos;
    int file_numbers;
    ino_t ino;
	int x;

	/*
	if (filp->f_version == inode->i_version && offset)
	{
		return filp->f_pos;
	}
	*/
	filp->f_version = inode->i_version;

	switch (offset)
	{
		case 0:
			if (filldir(dirent, ".", 1, offset, inode->i_ino, DT_DIR) < 0) //add "." and ".." to dirent
			{				
       		break;
			}			
			offset++;
			filp->f_pos++;
	        // fall through 
		case 1:
			spin_lock(&dcache_lock);
			ino = dentry->d_parent->d_inode->i_ino;
			spin_unlock(&dcache_lock);  					//same as parent_ino
//			ino = parent_ino(dentry); 
			if (filldir(dirent, "..", 2, offset, ino, DT_DIR) < 0)
			{
				break;
			}
			filp->f_pos++;
			offset++;
		default:
			if (!ptpfs_get_dir_data(inode))
			{
				return filp->f_pos;
			}
	       filp->f_pos+=2;

    			file_numbers = ptpfs_data->data.dircache.num_files;
			/*
			if(offset2>=(file_numbers-1))
				return 0;
			*/
			if(offset2)
			{
			
	       			filp->f_pos-=2;
				offset2-=4;
			}
			for (x = offset2; x < ptpfs_data->data.dircache.num_files; x++)
			{
				if (filldir(dirent, 
                        ptpfs_data->data.dircache.file_info[x].filename, 
                        strlen(ptpfs_data->data.dircache.file_info[x].filename), 
                        filp->f_pos, 
                        ptpfs_data->data.dircache.file_info[x].handle ,
                        ptpfs_data->data.dircache.file_info[x].mode) < 0)
				{
					ptpfs_free_inode_data(inode);
					return filp->f_pos;
				}
				filp->f_pos++;
			}
			return filp->f_pos;
	}
	return filp->f_pos;
}

static struct dentry * ptpfs_lookup(struct inode *dir, struct dentry *dentry,struct nameidata *nd)
{
    
    //printk(KERN_INFO "===== %s =====\n",  __FUNCTION__);


	int x;
    struct ptpfs_inode_data* ptpfs_data = PTPFSINO(dir);

	if (!ptpfs_get_dir_data(dir))
	{
		d_add(dentry, NULL);
		return NULL;
	}
	if (ptpfs_data->type != INO_TYPE_DIR  && ptpfs_data->type != INO_TYPE_STGDIR)
	{
		d_add(dentry, NULL);
		return NULL;
	}
	for (x = 0; x < ptpfs_data->data.dircache.num_files; x++)
	{
		if (!memcmp(ptpfs_data->data.dircache.file_info[x].filename,dentry->d_name.name,dentry->d_name.len))
		{
			struct ptp_object_info object;
			memset(&object,0,sizeof(object)); 
			if (ptp_getobjectinfo(PTPFSSB(dir->i_sb),ptpfs_data->data.dircache.file_info[x].handle,&object)!=PTP_RC_OK)
			{
				d_add(dentry, NULL);
				return NULL;
			}			

			int mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
			if (object.object_format==PTP_OFC_Association && object.association_type == PTP_AT_GenericFolder)
			{
				mode |= S_IFDIR; 
			}
			else
			{
				mode |= S_IFREG;
			}
			struct inode *newi = ptpfs_get_inode(dir->i_sb, mode  , 0,ptpfs_data->data.dircache.file_info[x].handle);
			PTPFSINO(newi)->parent = dir;
			ptpfs_set_inode_info(newi,&object);
			//atomic_inc(&newi->i_count);    // New dentry reference 
			d_add(dentry, newi);
			ptp_free_object_info(&object); //kfree(object->filename) & kfree(object->keywords) 
			return NULL;
		}
	}
	d_add(dentry, NULL);
	return NULL;

}

static int offset_read(struct ptpfs_sb_info *sb_info, struct ptp_data_buffer *buf, int start, int end, int reserve)
{
	int x;
	int k;	
	int ret;
	int r = buf->record_blocks;

	for ( k = 1; k <= MAX_SEG_NUM-1; k++ ){
		if ( buf->blocks[k].block_size != 0 && k != buf->record_blocks) {
			kfree(buf->blocks[k].block);
			buf->blocks[k].block_size = 0;
		}			
	}

//	printk("offset_read start : %d, end : %d\n", start, end);
	for (x = start; x <= end; x++ ){			
		memset(buf->blocks[r].block,0,MAX_SEG_SIZE);
		if (buf->blocks[r].block == NULL){
			printk("==== release function ! kmalloc error! ====\n");			
			return 0;
		}
		ret=ptp_io_read(sb_info,buf->blocks[r].block,MAX_SEG_SIZE);
		if (ret < 0){
			printk("==== release function ! ptp_io_read error ! ====\n");
			return 0;
		}

		if ( (reserve == 0) && (x == buf->num_seg-1) ) 
			kfree(buf->blocks[r].block);
	}
	return 1;
}

static int ptpfs_file_readpage(struct file *filp, struct page *page)
{
	int flag = 0;	// buffer IO
	char *buffer_d = NULL;

checkagain1:
        down(&ptp_passport_mutex);
        if(passport==PASSPORT_FREE)
        {
                passport=flag;
                up(&ptp_passport_mutex);
        }
        else if(passport!=flag)
        {
                up(&ptp_passport_mutex);
                msleep(1000);
                goto checkagain1;
        }
        else
                up(&ptp_passport_mutex);


	return ptpfs_file_readpages(filp, page, NULL, 0, flag);
}

static ssize_t ptp_direct_IO(int rw, struct kiocb *iocb,
                             const struct iovec *iov, loff_t offset, unsigned long nr_segs)
{
		struct file *file = iocb->ki_filp;
		struct inode *inode = file->f_mapping->host;
		int flag = 1;	// direct IO
		int total = 0;
		int ret;
		int read_nums = 0;
		int read_size;

		int err=0;

checkagain:
        down(&ptp_passport_mutex);
        if(passport==PASSPORT_FREE)
        {
                passport=flag;
                up(&ptp_passport_mutex);
        }
        else if(passport!=flag)
        {
                up(&ptp_passport_mutex);
                msleep(1000);
                goto checkagain;
        }
        else
                up(&ptp_passport_mutex);


		err = !access_ok(VERIFY_READ, (void __user*)iov->iov_base, iov->iov_len);
		if (err)
		{
			printk("access error : %d\n", err);
			return -EFAULT;
		}
		read_size = min ((int)(file->f_dentry->d_inode->i_size - offset), (int)iov->iov_len);

		if (read_size % (int)PAGE_SIZE == 0)
			read_nums = read_size / (int)PAGE_SIZE;
		else
			read_nums = read_size / (int)PAGE_SIZE + 1;

		int i;
		char *buffer = iov->iov_base;
		int pos = 0;

		for (i=1; i<=read_nums ; i++)	
		{
			pos = (i-1)*(int)PAGE_SIZE;
			ret = ptpfs_file_readpages(file, NULL, &buffer[pos], (int)offset+(i-1)*(int)PAGE_SIZE, flag);
			if (ret < 0)
				return ret;

			if (iov->iov_len - total >= (int)PAGE_SIZE)
				total += (int)PAGE_SIZE;			
			else
				total += iov->iov_len - total;
		}
		return total;
}

static int ptpfs_file_readpages
(struct file *filp, struct page *page, char *buffer_d, int offset_d,int flag)
{
	//	printk(KERN_INFO "%s\n",  __FUNCTION__);

	struct ptpfs_sb_info *sb_info = PTPFSSB(filp->f_dentry->d_sb);
	int ret = -EFAULT;
	int offset_back = 0;
	int read_over = 0; // if this is a new open file or just seek back, we read the transmitting data first.
	int offset_same = 0;

	int offset;
/*
checkagain:
	down(&ptp_passport_mutex);
	if(passport==PASSPORT_FREE)
	{
		passport=flag;
		up(&ptp_passport_mutex);
	}
	else if(passport!=flag)
	{
		up(&ptp_passport_mutex);
		msleep(1000);
		goto checkagain;
	}
	else
		up(&ptp_passport_mutex);
*/	
	if (flag == 0){
		offset = page->index << PAGE_CACHE_SHIFT;		// PAGE_CACHE_SHIFT = 4KB	
		if (!PageLocked(page))
			PAGE_BUG(page);
	}
	else{
		offset = offset_d;
	}
	struct inode *inode;	
	inode = filp->f_dentry->d_inode;

	if (sb_info->error_transmit == 1)
	{
		printk("go error\n");
		goto error;
	}

	/*	private_data is NULL, but ino is the same. Maybe this file is opened again. */
	/*	if offset is larger than the former one (same ino), we could copy the private_data. */
	/*	we consider this situation is like seeking to the target offset (rear) */

	if ( sb_info->filp_temp ) {
		if ( !filp->private_data && filp->f_dentry->d_inode->i_ino == sb_info->ino_temp ) {
			if (sb_info->private_data) {
				if (offset > sb_info->private_data->offset) 
				{
					filp->private_data = sb_info->private_data;				
				}
			}
		}
	}

	if (filp->private_data){
		if 	( (offset - ((struct ptp_data_buffer *)(filp->private_data))->offset == 0 )&&
			  (filp->f_dentry->d_inode->i_ino == sb_info->ino_temp ) ) 
			offset_same = 1;
		else if (offset - ((struct ptp_data_buffer *)(filp->private_data))->offset < 0){
			read_over = 1;	// read over and private data exits
			offset_back = 1;
		}
	}
	else if ( !filp->private_data ){	// !filp->private_data, a new open file
		if (sb_info->read_condition == 1){
			read_over = 2;	// read over and no private data 	
		}
		if ( offset!=0 )
			offset_back = 1;
	}
	/* store this inode number, in order to compare the next inode. */
	sb_info->ino_temp = filp->f_dentry->d_inode->i_ino;  



	if ( read_over ){
		struct ptp_data_buffer *buf = sb_info->private_data;
		int x;
		int k;
		int r = buf->record_blocks;				

		if (!offset_read(sb_info, buf, buf->num_blocks+1, buf->num_seg, 0)){
			printk("offset_read error !!\n");
			goto error;
		}
		sb_info->read_condition = 2;
		if (buf->num_blocks < buf->num_seg){	//if they are the same, response was already read.
			down(&sb_info->usb_device->sem);
			ret = ptp_usb_getresp(buf->sb_temp, buf->ptp_temp);
			if(ret < 0){
				printk(KERN_INFO "can't get response !!!!!\n");
				goto error;
			}
			up (&sb_info->usb_device->sem);
		}

		kfree(buf->ptp_temp);			
		kfree(buf->blocks);
		kfree(buf);
		buf = NULL;		
		sb_info->private_data = NULL;
		if (read_over == 1)		
			filp->private_data = NULL;
	}

	//the same filp , check if the page is sequential or not 
	if  ( (filp == sb_info->filp_temp) && (filp->private_data))  
	{
		// seek to hind file position
		if ( offset - ((struct ptp_data_buffer *)(filp->private_data))->offset > (int)PAGE_SIZE){	
			struct ptp_data_buffer *buf2 = filp->private_data;

			// 1st block size = 500
			int seek_block;
			int count_temp = 0;	
			int i;
			if( (offset-PTP_USB_BULK_PAYLOAD_LEN) % MAX_SEG_SIZE == 0 ){
				seek_block = (offset-PTP_USB_BULK_PAYLOAD_LEN)/MAX_SEG_SIZE+1;
				count_temp = PTP_USB_BULK_PAYLOAD_LEN + (seek_block-1)*MAX_SEG_SIZE;
			}
			else{
				seek_block = (offset-PTP_USB_BULK_PAYLOAD_LEN)/MAX_SEG_SIZE+2;
				count_temp = PTP_USB_BULK_PAYLOAD_LEN + (seek_block-2)*MAX_SEG_SIZE;
			}

			if (!offset_read(sb_info, buf2, buf2->num_blocks+1, seek_block, 1)){
				printk("offset_read error !!\n");
				goto error;
			}
			buf2->num_blocks = seek_block;
			buf2->count = count_temp;

			if (buf2->num_blocks == buf2->num_seg)
			{
				down(&PTPFSSB(inode->i_sb)->usb_device->sem);
				ret = ptp_usb_getresp(buf2->sb_temp, buf2->ptp_temp);				
				up (&PTPFSSB(inode->i_sb)->usb_device->sem);
				if (ret < 0)
				{
					printk("ptp_free_data_buffer(buf2)\n");
					goto error;
				}
			}
			sb_info->read_condition = 1;

			int h;
			for (h=1; h<=5; h++){
				if( h!=buf2->record_blocks )
					buf2->blocks[h].block_size = 0; 
			}
		}
	
	}

	//	get the requested object 
	struct ptp_data_buffer *data;

	if (!filp->private_data){
	    data=(struct ptp_data_buffer*)kmalloc(sizeof(struct ptp_data_buffer), GFP_KERNEL);
	    memset(data,0,sizeof(struct ptp_data_buffer));

		sb_info->read_condition = 0;
		sb_info->filp_temp = filp;

		ret = ptp_getobject(PTPFSSB(inode->i_sb),inode->i_ino,data);

		if (ret == PTP_RC_OK){
			filp->private_data = data;
			return 0;
		}
		else if(ret == PTP_NRC_GETOBJECT){
			filp->private_data = data;
			sb_info->private_data = data;
		}
		else{
			sb_info->error_transmit = 1;
			printk(KERN_INFO "ptp_getobject error !\n");
			goto error;
		}	
	}  //end if (filp->private_data)
	else{
		data = filp->private_data;
	}

	if (offset_back == 1 && offset > (int)PAGE_SIZE)	// offset position is back, read blocks to target offset
	{
			struct ptp_data_buffer *buf3 = (struct ptp_data_buffer *)(filp->private_data);
			int seek_block;	// seek_block indicates which block we have to find, not a range
			int remainder = (offset-PTP_USB_BULK_PAYLOAD_LEN) % MAX_SEG_SIZE;
			
			if( remainder == 0 ){
				seek_block = (offset-PTP_USB_BULK_PAYLOAD_LEN)/MAX_SEG_SIZE+1;	// +1 means include block[0]
			}
			else {
				seek_block = (offset-PTP_USB_BULK_PAYLOAD_LEN)/MAX_SEG_SIZE+2;	// +2 means include block[0] and remainder.
			}
			kfree(buf3->blocks[0].block);
			buf3->blocks[0].block_size = 0;

			int i;
			buf3->record_blocks = 1;
			int r = buf3->record_blocks;
			// block[0] was read in ptp_getobject

			buf3->blocks[r].block = kmalloc(MAX_SEG_SIZE,GFP_KERNEL);
			buf3->blocks[r].block_size = MAX_SEG_SIZE;


			int count_temp = PTP_USB_BULK_PAYLOAD_LEN;
			for (i = 1; i <= seek_block; i++ )
			{	
				memset(buf3->blocks[r].block,0,MAX_SEG_SIZE);
				ret=ptp_io_read(sb_info,buf3->blocks[r].block,MAX_SEG_SIZE);
				if(ret < 0)
					goto error;
			
				if (i != seek_block){
					count_temp += MAX_SEG_SIZE;
				}
			}
			sb_info->read_condition = 1;
			buf3->num_blocks = seek_block+1;	// +1 indicate including block[0]
			buf3->count = count_temp;
	}
	//=========  seek to head , file read block first ========================//
	ret = -EFAULT;

    /* work out how much to get and from where */

    int size   = min((size_t)(inode->i_size - offset),(size_t)PAGE_SIZE);  //4096 or remainder data
	char *buffer;
	if ( flag == 0 )
	{
		buffer = kmap_atomic(page,KM_USER0);  
		clear_page(buffer);
	}
	else
		buffer = buffer_d;

	int final_page = 0;
	// if cross_block = [block_num that we could free] , free that one
	int cross_block = -1; 
	int result = PTP_RC_OK;
	if(offset + size == inode->i_size)
		final_page = 1;

    /* read the contents of the file from the server into the page */
	int block = data->record_blocks;  
	data->offset = offset; 

	//when block = 0, to read next block first 
	if(block == 0)
	{
		data->blocks[1].block = kmalloc(MAX_SEG_SIZE,GFP_KERNEL);
		data->blocks[1].block_size = MAX_SEG_SIZE;
		memset(data->blocks[1].block,0,MAX_SEG_SIZE);

		if (data->blocks[1].block == NULL)
		{
			printk("===== ERROR_1 =====\n");
			ret = -EFAULT;
			goto error;
		}
		ret=ptp_io_read(PTPFSSB(inode->i_sb),data->blocks[1].block,MAX_SEG_SIZE);
		if (ret < 0)
		{
			printk("===== ERROR_2 =====\n");
			goto error;
		}
		data->num_blocks++;
		PTPFSSB(inode->i_sb)->read_condition = 1;
	}
	offset -= data->count;

	if (block == data->num_blocks)
	{
		kunmap_atomic(buffer,KM_USER0); 
		ret = -EFAULT;
		goto error;
	}
	int toCopy = min(size,data->blocks[block].block_size-offset);

	if (toCopy > 0)	
	{		
		if (offset_same == 1)
			memcpy(buffer,&sb_info->buffer,toCopy);
		else
		{
			memset(&sb_info->buffer, 0, sizeof((int)PAGE_SIZE));
			memcpy(buffer,&data->blocks[block].block[offset],toCopy);
			memcpy(&sb_info->buffer[0],data->blocks[block].block,toCopy);	
		}
	}

	if(block == 0)
	{
		cross_block = 0;
	}

	size -= toCopy;
	int pos = toCopy;
	block = (block % (MAX_SEG_NUM-1))+1; //the same as original block++

	int block_next = (block % (MAX_SEG_NUM-1))+1; //replace (block+1)
	if (data->blocks[block].block_size == 0)
	{
		block_next = block;
	}

	if(data->blocks[block_next].block_size == 0 && data->num_blocks < data->num_seg) //not yet read next block
	{
		data->blocks[block_next].block = kmalloc(MAX_SEG_SIZE,GFP_KERNEL);
		data->blocks[block_next].block_size = MAX_SEG_SIZE;
		memset(data->blocks[block_next].block,0,MAX_SEG_SIZE);
		if (data->blocks[block_next].block == NULL)
		{
			ret = -EFAULT;
			goto error;
		}
		ret=ptp_io_read(PTPFSSB(inode->i_sb),data->blocks[block_next].block,MAX_SEG_SIZE);
		if (ret < 0)
		{
			goto error;
		}
		data->num_blocks++;	

		if (data->num_blocks == data->num_seg)
		{
			down(&PTPFSSB(inode->i_sb)->usb_device->sem);
			ret = ptp_usb_getresp(data->sb_temp, data->ptp_temp);
			up (&PTPFSSB(inode->i_sb)->usb_device->sem);
			if (ret < 0)
			{
				goto error;
			}
		}
	}
	while (size && block < data->num_blocks)
	{
		//size != 0 means that last block is already read, we read next one. Free last block!
		cross_block = block-1;
		if(cross_block == 0 && data->num_seg > MAX_SEG_NUM && data->blocks[MAX_SEG_NUM-1].block_size != 0)
			cross_block = MAX_SEG_NUM-1;			

		toCopy = min(size,data->blocks[block].block_size);
		if (offset_same == 0)
		{
			memcpy(&buffer[pos],data->blocks[block].block,toCopy);
			memcpy(&sb_info->buffer[pos],data->blocks[block].block,toCopy);	
		}
		data->record_blocks = block;

		size -= toCopy;
		pos += toCopy;
		block++;
	}

	if (block == data->num_blocks && size > 0)
	{
		if (flag == 0)	
			kunmap_atomic(buffer, KM_USER0);
		ret = -EFAULT;
        goto error;
	}

	if (flag == 0)
	{
		kunmap_atomic(buffer, KM_USER0);
		flush_dcache_page(page);
		SetPageUptodate(page);
		unlock_page(page);
	}
	if(cross_block != -1)
	{
		kfree(data->blocks[cross_block].block);
		data->count += data->blocks[cross_block].block_size;
		data->blocks[cross_block].block_size = 0;
	}
	if(final_page == 1)
	{
		data->num_blocks = 0; //about release

		kfree(data->ptp_temp);
		kfree(data->blocks);
		kfree(data);
		filp->private_data = NULL;
		sb_info->private_data = NULL;
		sb_info->read_condition = 2;
		sb_info->filp_temp = NULL; 

		if(result < 0)
			printk("can't get response !!!!!\n");
	}	

    return 0;


    error:
	//sb_info->error_transmit = 1;
	if (flag == 0)
	{
		SetPageError(page);
		unlock_page(page);
	}
	return ret;
} 


static ssize_t ptpfs_write(struct file *filp, const char *buf, size_t count, loff_t *ppos)
{
    //printk(KERN_INFO "%s   pos: %d   count:%d\n",  __FUNCTION__,(int)*ppos,count);
    struct ptp_data_buffer *data=(struct ptp_data_buffer*)filp->private_data;

    struct ptp_block *blocks = kmalloc(sizeof(struct ptp_block)*(data->num_blocks+1), GFP_KERNEL);

    if (data->blocks) memcpy(blocks,data->blocks,sizeof(struct ptp_block)*data->num_blocks);
    blocks[data->num_blocks].block_size=count;
    blocks[data->num_blocks].block = kmalloc(count, GFP_KERNEL);
    memcpy(blocks[data->num_blocks].block,buf,count);
    if (data->blocks) kfree(data->blocks);
    data->blocks = blocks;
    data->num_blocks++;
    *ppos += count;
    return count;
}


static int ptpfs_release(struct inode *ino, struct file *filp)
{
	down(&ptp_passport_mutex);
	passport=PASSPORT_FREE;
	up(&ptp_passport_mutex);
    //printk(KERN_INFO "%s    object:%X    dcount: %d\n",  __FUNCTION__,ino->i_ino, filp->f_dentry->d_count);
	/*
	if (data)
	{
		switch (filp->f_flags & O_ACCMODE)
		{
			case O_RDWR:
				printk("======== O_RDWR =======\n");		//read, write
			case O_WRONLY:
			{
				printk("======== O_WRONLY =======\n");		//write only
				struct ptp_object_info object;
				memset(&object,0,sizeof(object));
				if (ptp_getobjectinfo(PTPFSSB(ino->i_sb),ino->i_ino,&object)!=PTP_RC_OK)
				{
					ptp_free_data_buffer(data);
					kfree(data);
					return -EPERM;
				}

				ptp_deleteobject(PTPFSSB(ino->i_sb),ino->i_ino,0); //
				int count = 0;
				int x;

				for (x = 0; x < data->num_blocks; x++)
				{
					count += data->blocks[x].block_size;
				}


                int storage = object.storage_id;
                int parent = object.parent_object;
                int handle = ino->i_ino;
                object.object_compressed_size=count;
                int ret = ptp_sendobjectinfo(PTPFSSB(ino->i_sb), &storage, &parent, &handle, &object); //
                ret = ptp_sendobject(PTPFSSB(ino->i_sb),data,count); //

                ino->i_size = count;
                ino->i_mtime = CURRENT_TIME;
                ptpfs_free_inode_data(ino);//uncache
                ptpfs_free_inode_data(PTPFSINO(ino) -> parent);//uncache

                //the inode number will change.  Kill this inode so next dir lookup will create a new one
                dput(filp->f_dentry);
                //iput(ino);
			}
			break;
		}
	
		if(data->get_object == 1 && data->num_blocks != 0)  //open,but not readpage
		{	
			printk("====== open , but not readpage ======\n");

			int result ;
			int x;
			int ret;
			//========================
			for (x = 1; x < data->num_seg; x++ )
			{				
				if (x == 1)
				{
					data->blocks[1].block = kmalloc(MAX_SEG_SIZE,GFP_KERNEL);
					data->blocks[1].block_size = MAX_SEG_SIZE;
				}

				data->blocks[1].block = kmalloc(MAX_SEG_SIZE,GFP_KERNEL);
				data->blocks[1].block_size = MAX_SEG_SIZE;
				
				memset(data->blocks[1].block,0,MAX_SEG_SIZE);

				if (data->blocks[1].block == NULL)
				{
					printk("==== release function ! kmalloc error! ====\n");
					ptp_free_data_buffer(data);
					return PTP_ERROR_IO;
				}
				ret=ptp_io_read(PTPFSSB(ino->i_sb),data->blocks[1].block,MAX_SEG_SIZE);
				if (ret < 0)
				{
					printk("==== release function ! ptp_io_read error ! ====\n");
					ptp_free_data_buffer(data);
					return PTP_ERROR_IO;
				}
				
				//data->num_blocks++;
				if (x == data->num_seg-1) 
					kfree(data->blocks[1].block);
			}

			down(&PTPFSSB(ino->i_sb)->usb_device->sem);
			result = ptp_usb_getresp(data->sb_temp, data->ptp_temp);
			up (&PTPFSSB(ino->i_sb)->usb_device->sem);
			//========================
			printk("========= result : %x =========\n",result);
			if(result < 0)
				printk("can't get response !!!!!\n");

			kfree(data->blocks[0].block);
			kfree(data->blocks);

			kfree(data->ptp_temp);			

			
		//	data->ptp_temp = NULL;
		}
		else
			ptp_free_data_buffer(data);

		kfree(data);
		data = NULL;
	}
	*/
	return 0;
}



struct mime_map
{
    char *ext;
    int type;
};                    
const struct mime_map mime_map[] = {
    {"txt",PTP_OFC_Text},
    {"mp3",PTP_OFC_MP3},
    {"mpg",PTP_OFC_MPEG},
    {"wav",PTP_OFC_WAV},
    {"avi",PTP_OFC_AVI},
    {"asf",PTP_OFC_ASF},
    {"jpg",PTP_OFC_EXIF_JPEG},
    {"tif",PTP_OFC_TIFF},
    {"bmp",PTP_OFC_BMP},
    {"gif",PTP_OFC_GIF},
    {"pcd",PTP_OFC_PCD},
    {"pct",PTP_OFC_PICT},
    {"png",PTP_OFC_PNG},
    {0,0},
};




static inline int get_format(unsigned char *filename, int l)
{
    char buf[4];
    int x;

	//printk(KERN_INFO "===== %s =====\n",  __FUNCTION__);
    while (l)
    {
        l--;
        if (filename[l] == '.')
        {
            filename = &filename[l];
            l = 0;
            if (strlen(filename) > 4 || strlen(filename) == 1)
            {
                return PTP_OFC_Undefined;
            }
            filename++;
            for (x = 0; x < strlen(filename); x++)
            {
                buf[x] = __tolower(filename[x]);
            }
            buf[strlen(filename)] = 0;
            const struct mime_map *map = mime_map;
            while (map->ext)
            {
                if (!strcmp(buf,map->ext))
                {
                    return map->type;
                }
                map++;
            }
        }
    }
    return PTP_OFC_Undefined;
}

static int ptpfs_create(struct inode *dir,struct dentry *d,int i) 
{
    //printk(KERN_INFO "%s   %s %d\n",__FUNCTION__,d->d_name.name,i);
    __u32 storage;
    __u32 parent;
    __u32 handle;
    struct ptp_object_info objectinfo;
    memset(&objectinfo,0,sizeof(objectinfo));

    storage = PTPFSINO(dir)->storage;
    if (PTPFSINO(dir)->type == INO_TYPE_STGDIR)
    {
        parent = 0xffffffff;
    }
    else
    {
        parent = dir->i_ino;
    }

    objectinfo.filename = (unsigned char *)d->d_name.name;
    objectinfo.object_format=get_format((unsigned char *)d->d_name.name,d->d_name.len);
    objectinfo.object_compressed_size=0;

    int ret = ptp_sendobjectinfo(PTPFSSB(dir->i_sb), &storage, &parent, &handle, &objectinfo);

    if (ret == PTP_RC_OK)
    {
        struct ptp_data_buffer data;
        struct ptp_block block;
        unsigned char buf[10];

        memset(&data,0,sizeof(data));
        memset(&block,0,sizeof(block));
        data.blocks=&block;
        data.num_blocks = 1;
        block.block_size = 0;
        block.block = buf;

        ret = ptp_sendobject(PTPFSSB(dir->i_sb),&data,0);


        if (handle == 0)
        {
            //problem - it didn't return the right handle - need to do something find it
            struct ptp_object_handles objects;
            objects.n = 0;
            objects.handles = NULL;

            ptpfs_get_dir_data(dir);
            if (PTPFSINO(dir)->type == INO_TYPE_DIR)
            {
                ptp_getobjecthandles(PTPFSSB(dir->i_sb), PTPFSINO(dir)->storage, 0x000000, dir->i_ino,&objects);
            }
            else
            {
                ptp_getobjecthandles(PTPFSSB(dir->i_sb), dir->i_ino, 0x000000, 0xffffffff ,&objects);
            }

            int x,y;
            for (x = 0; x < objects.n && !handle; x++ )
            {
                for (y = 0; y < PTPFSINO(dir)->data.dircache.num_files; y++)
                {
                    if (PTPFSINO(dir)->data.dircache.file_info[y].handle == objects.handles[x])
                    {
                        objects.handles[x] = 0;
                        y = PTPFSINO(dir)->data.dircache.num_files;
                    }
                }
                if (objects.handles[x])
                {
                    handle = objects.handles[x]; 
                }
            }
            ptp_free_object_handles(&objects);
        }


        int mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_IFREG;
        struct inode *newi = ptpfs_get_inode(dir->i_sb, mode  , 0,handle);
        PTPFSINO(newi)->parent = dir;
        ptpfs_set_inode_info(newi,&objectinfo);
        atomic_inc(&newi->i_count);    /* New dentry reference */
        d_instantiate(d, newi);

        objectinfo.filename = NULL;
        ptp_free_object_info(&objectinfo);

        ptpfs_free_inode_data(dir);//uncache
        dir->i_version++;
        newi->i_mtime = CURRENT_TIME;
        return 0;
    }
    objectinfo.filename = NULL;
    ptp_free_object_info(&objectinfo);
    return -EPERM;


}

static int ptpfs_mkdir(struct inode *ino,struct dentry *d,int i)
{
    //printk(KERN_INFO "%s   %s %d\n",__FUNCTION__,d->d_name.name,i);

    __u32 storage;
    __u32 parent;
    __u32 handle;
    struct ptp_object_info objectinfo;
    memset(&objectinfo,0,sizeof(objectinfo));



    storage = PTPFSINO(ino)->storage;
    if (PTPFSINO(ino)->type == INO_TYPE_STGDIR)
    {
        parent = 0xffffffff;
    }
    else
    {
        parent = ino->i_ino;
    }

    objectinfo.filename = (unsigned char *)d->d_name.name;
    objectinfo.object_format=PTP_OFC_Association;
    objectinfo.association_type = PTP_AT_GenericFolder;

    int ret = ptp_sendobjectinfo(PTPFSSB(ino->i_sb), &storage, &parent, &handle, &objectinfo);
    /*
    if (ret == PTP_RC_OK)
    {

        struct ptp_data_buffer data;
        memset(&data,0,sizeof(data));
        ret = ptp_sendobject(PTPFSSB(ino->i_sb),&data,0);
    }
    */


    objectinfo.filename = NULL;
    ptp_free_object_info(&objectinfo);

    if (ret == PTP_RC_OK)
    {
        ptpfs_free_inode_data(ino);//uncache
        ino->i_version++;
        return 0;
    }
    return -EPERM;
}

int ptpfs_unlink(struct inode *dir,struct dentry *d)
{
    //printk(KERN_INFO "%s   %s\n",__FUNCTION__,d->d_name.name);
    int x;

    struct ptpfs_inode_data* ptpfs_data = PTPFSINO(dir);

    if (!ptpfs_get_dir_data(dir))
    {
        return -EPERM;
    }
    if (ptpfs_data->type != INO_TYPE_DIR  && ptpfs_data->type != INO_TYPE_STGDIR)
    {
        return -EPERM;
    }


    for (x = 0; x < ptpfs_data->data.dircache.num_files; x++)
    {

        if (!memcmp(ptpfs_data->data.dircache.file_info[x].filename,d->d_name.name,d->d_name.len))
        {
            int ret = ptp_deleteobject(PTPFSSB(dir->i_sb),ptpfs_data->data.dircache.file_info[x].handle,0);
            if (ret == PTP_RC_OK)
            {
                ptpfs_free_inode_data(dir);//uncache
                dir->i_version++;
                return 0;

            }
        }
    }
    return -EPERM;
}
/*
loff_t ptp_file_llseek(struct file *file, loff_t offset, int origin)
{
		printk("======== generic_file_llseek ========\n");
		dump_stack();
        long long retval;
        struct inode *inode = file->f_mapping->host;

        down(&inode->i_sem);
        switch (origin) {
                case 2:
                        offset += inode->i_size;
                        break;
                case 1:
                        offset += file->f_pos;
        }
        retval = -EINVAL;
        if (offset>=0 && offset<=inode->i_sb->s_maxbytes) {
                if (offset != file->f_pos) {
                        file->f_pos = offset;
                        file->f_version = 0;
                }
                retval = offset;
        }
        up(&inode->i_sem);
        return retval;
}

*/

static int ptpfs_rmdir(struct inode *ino ,struct dentry *d)
{
	
//	printk(KERN_INFO "===== %s =====\n",  __FUNCTION__);
    return ptpfs_unlink(ino,d);
}
static int ptpfs_open(struct inode *ino, struct file *filp)
{
	clear_bit(AS_LIMIT_SIZE, &filp->f_mapping->flags);
	return 0;
}


struct address_space_operations ptpfs_fs_aops = {
	readpage:   	ptpfs_file_readpage,
	direct_IO:		ptp_direct_IO,
//	commit_write:	simple_commit_write,		//
};
struct file_operations ptpfs_dir_operations = {
    read:       generic_read_dir,
    readdir:    ptpfs_readdir,
};
struct file_operations ptpfs_file_operations = {
	read:		generic_file_read, 
	write:		ptpfs_write,
	open:		ptpfs_open,
	release:	ptpfs_release,
//	llseek:	ptp_file_llseek,

};
struct inode_operations ptpfs_dir_inode_operations = {
    lookup:     ptpfs_lookup,
    mkdir:      ptpfs_mkdir,
    rmdir:      ptpfs_rmdir,
    unlink:     ptpfs_unlink,
    create:     ptpfs_create,
};

/*
struct file_operations ptpfs_file_operations = {
    write:      ptpfs_file_write,
    mmap:       generic_file_mmap,
    fsync:      ptpfs_sync_file,
};
struct inode_operations ptpfs_dir_inode_operations = {
    create:     ptpfs_create,
    rename:     ptpfs_rename,
};
*/
