#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>                  
#include <asm/page.h>                  
#include <asm/mach-venus/md.h>
#include <platform.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/devfs_fs_kernel.h>

#include "md_reg.h"


#ifdef CONFIG_REALTEK_MD_DEVICE_FILE
  #define MD_DEV_FILE_NAME       "md"
  static struct cdev md_cdev;
  static dev_t devno;
#endif


///////////////////// MACROS /////////////////////////////////
//#define dbg_char(x)                     writel((x) ,0xb801B200)
#define dbg_char(x)
static struct md_dev* dev = NULL;
static struct platform_device*  md_device = NULL;


///////////////////// MACROS /////////////////////////////////
#define _md_map_single(p_data, len, dir)      dma_map_single(&md_device->dev, p_data, len, dir)
#define _md_unmap_single(p_data, len, dir)    dma_unmap_single(&md_device->dev, p_data, len, dir)

//#define md_lock(md)                     spin_lock(&md->lock)
//#define md_unlock(md)                   spin_unlock(&md->lock)

#define md_lock(md)                     spin_lock_irqsave(&md->lock, md->spin_flags)
#define md_unlock(md)                   spin_unlock_irqrestore(&md->lock, md->spin_flags)

#define md_lock_irqsave(md, flags)      spin_lock_irqsave(&md->lock, flags)
#define md_lock_irqrestore(md, flags)   spin_unlock_irqrestore(&md->lock, flags)


/*-------------------------------------------------------------------- 
 * Func : md_open 
 *
 * Desc : open md engine
 *
 * Parm : N/A
 *
 * Retn : N/A
 --------------------------------------------------------------------*/  
void md_open(void)
{
	u32 PhysAddr;

	/* get correlated virtuall address of correlated io address */			

    //start MD
    writel(SMQ_GO | SMQ_CLR_WRITE_DATA | SMQ_CMD_SWAP, IOA_SMQ_CNTL);

    if (dev == NULL)
    {
        dev = (struct md_dev *)kmalloc(sizeof(struct md_dev), GFP_KERNEL);
        
        if (dev == NULL)        
            return ;
            
        memset(dev, 0, sizeof(struct md_dev));
    }
	dev->size = SMQ_COMMAND_ENTRIES * sizeof(u32);

    dev->CachedCmdBuf = kmalloc(dev->size, GFP_KERNEL);                 // cached virt address
    dev->CmdBuf       =(void*)(((unsigned long)dev->CachedCmdBuf & 0x0FFFFFFF) | 0xA0000000);  // non cached virt address
    PhysAddr          = virt_to_phys(dev->CachedCmdBuf);                
    
	DBG_PRINT("[user land] CmdBuf virt addr = %p/%p, phy addr=%p\n", dev->CachedCmdBuf, (void*)dev->CmdBuf, (void*)PhysAddr);

    dev->v_to_p_offset = (int32_t)((u8 *)dev->CmdBuf - (u32)PhysAddr);

    dev->wrptr    = 0;
	dev->CmdBase  = (void *)PhysAddr;
	dev->CmdLimit = (void *)(PhysAddr + SMQ_COMMAND_ENTRIES*sizeof(u32));
	spin_lock_init(&dev->lock);

	*(volatile unsigned int *)IOA_SMQ_CmdRdptr = 1;

  	writel((u32)dev->CmdBase,  IOA_SMQ_CmdBase);
    writel((u32)dev->CmdLimit, IOA_SMQ_CmdLimit);
	writel((u32)dev->CmdBase,  IOA_SMQ_CmdRdptr);
    writel((u32)dev->CmdBase,  IOA_SMQ_CmdWrptr);
    writel(0,                  IOA_SMQ_INST_CNT);

	//enable interrupt
    //writel(SMQ_SET_WRITE_DATA | INT_SYNC, IOA_SMQ_INT_ENABLE);

    __sync();

    //start SE
    writel(SMQ_GO | SMQ_SET_WRITE_DATA | SMQ_CMD_SWAP, IOA_SMQ_CNTL);    	
}


 
/*-------------------------------------------------------------------- 
 * Func : md_close 
 *
 * Desc : close md engine
 *
 * Parm : N/A
 *
 * Retn : N/A
 --------------------------------------------------------------------*/ 
void md_close(void)
{
    int count = 0;
    
    while(readl(IOA_SMQ_CmdWrptr) != readl(IOA_SMQ_CmdRdptr))
    {
        msleep(100);
        if (count++ > 300) 
            return;
    }

	writel(SMQ_GO | SMQ_CLR_WRITE_DATA, IOA_SMQ_CNTL);

	if (dev) 
	{
        if (dev->CachedCmdBuf)
            kfree(dev->CachedCmdBuf);
                    
		kfree(dev);
        dev = NULL;
	}
    return;
}



/*-------------------------------------------------------------------- 
 * Func : WriteCmd 
 *
 * Desc : Write Command to Move Data Engine
 *
 * Parm : dev   :   md device
 *        pBuf  :   cmd
 *        nLen  :   cmd length in bytes
 *
 * Retn : N/A
 --------------------------------------------------------------------*/
static
u64 WriteCmd(
    struct md_dev*          dev, 
    u8*                     pBuf, 
    int                     nLen
    )
{
    int i;
    u8 *writeptr;
    u8 *pWptr;
    u8 *pWptrLimit;
    u64 counter64 = 0;
    md_lock(dev);

    pWptrLimit = (u8 *)dev->CmdBuf + dev->size;
    pWptr      = (u8 *)dev->CmdBuf + dev->wrptr;

    writeptr = (u8*) readl(IOA_SMQ_CmdWrptr);        
    
    if ((dev->wrptr+nLen) >= dev->size)
    {
        //to cover md copy bus, see mantis 9801 & 9809
        while(1)
        {
            u32 rp = (u32)readl(IOA_SMQ_CmdRdptr);
            if((u32)writeptr == rp) 
                break;
            udelay(1);
        }
    }

    u8 *readptr;
    while(1)
    {        
        readptr = (u8 *)readl(IOA_SMQ_CmdRdptr);
        if(readptr <= writeptr)
        {
            readptr += dev->size; 
        }
        if((writeptr+nLen) < readptr)
        {
            break;
        }
        udelay(1);
    }

    //Start writing command words to the ring buffer.
    for(i=0; i<nLen; i+=sizeof(u32))
    {        
        *(u32 *)((u32)pWptr) = endian_swap_32(*(u32 *)(pBuf + i));
        
        pWptr += sizeof(u32);
        
        if (pWptr >= pWptrLimit)
            pWptr = (u8 *)dev->CmdBuf;
    }    

    dev->wrptr += nLen;
    if (dev->wrptr >= dev->size)
        dev->wrptr -= dev->size;

    //convert to physical address
    pWptr -= dev->v_to_p_offset;

    __sync();

    writel((u32)pWptr, IOA_SMQ_CmdWrptr);    

    dev->sw_counter.low++;    
    if (dev->sw_counter.low==0)    
        dev->sw_counter.high++;
    
    counter64 = dev->sw_counter.high;
    counter64 = (counter64 << 32) | dev->sw_counter.low;                      
    
    md_unlock(dev);    
    
    return counter64;
}




/*-------------------------------------------------------------------- 
 * Func : md_checkfinish 
 *
 * Desc : check if the specified command finished. 
 *
 * Parm : handle   :   md command handle 
 *
 * Retn : true : finished, 0 : not finished
 --------------------------------------------------------------------*/
bool md_checkfinish(MD_CMD_HANDLE handle)
{
	u32 sw_counter = (u32)handle & 0xFFFF;
	u32 hw_counter;
	
	if (handle==0)
	    return true;
	    
    hw_counter = readl(IOA_SMQ_INST_CNT) & 0xFFFF;
	
	if (hw_counter >= sw_counter)
	{
	    if((hw_counter - sw_counter) < 0x8000)
            return true;
	}
    else
	{
	    if((hw_counter + 0x10000 - sw_counter) < 0x8000)
		    return true;
	}
	
    return false;
}


/*-------------------------------------------------------------------- 
 * Func : md_release_copy_request 
 *
 * Desc : release a copy request
 *
 * Parm : 
 *
 * Retn : N/A
 --------------------------------------------------------------------*/
static inline void md_release_copy_request(md_copy_t* req)    
{
    if (req)
    {        
        if (req->request_id)
        {            
            _md_unmap_single(req->dma_src, req->len, DMA_TO_DEVICE);
            _md_unmap_single(req->dma_dst, req->len, DMA_FROM_DEVICE);              
        }            
        
        memset(req, 0, sizeof(md_copy_t));
    }        
}



/*-------------------------------------------------------------------- 
 * Func : md_alloc_copy_request 
 *
 * Desc : allocate a copy request
 *
 * Parm : 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
void md_check_copy_tasks(void)
{
    md_copy_t* req = NULL;    
    
    if (dev->finished_copy_counter < dev->copy_counter)
    {        
        md_lock(dev);
        
        for (; dev->finished_copy_counter < dev->copy_counter; dev->finished_copy_counter++)
        {
            req = &dev->copy_task[dev->finished_copy_counter % MAX_COPY_TASK];
            
            if (md_checkfinish(req->request_id)==0)        
                break;
                
            md_release_copy_request(req);                
        }
        md_unlock(dev);  
    }    
}




/*-------------------------------------------------------------------- 
 * Func : md_alloc_copy_request 
 *
 * Desc : allocate a copy request
 *
 * Parm : 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
static inline md_copy_t* md_alloc_copy_request(    
    void*                   dst,
    void*                   src,
    int                     len,
    int                     dir
    )
{
    md_copy_t* req = NULL;    
    
    md_check_copy_tasks();      // clean all completed tasks first
    
    md_lock(dev);
    
    req = &dev->copy_task[dev->copy_counter % MAX_COPY_TASK];
    
    if (req->request_id==0)        
    {
        // empty task
        req->dst     = dst;
        req->src     = src;
        req->len     = len;
        req->dir     = dir;     
        req->dma_dst = _md_map_single((void*) req->dst, req->len, DMA_FROM_DEVICE);    
        req->dma_src = _md_map_single((void*) req->src, req->len, DMA_TO_DEVICE);     
        req->copy_id = dev->copy_counter++;
    }
    else
    {
        printk("no more entry");
        req = NULL;
    }        
                        
    md_unlock(dev); 

    return req;
}





/*-------------------------------------------------------------------- 
 * Func : md_copy_start 
 *
 * Desc : doing memory copy via md
 *
 * Parm : req   :   md_copy_request 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
MD_COPY_HANDLE md_copy_start( 
    void*                   dst,
    void*                   src,
    int                     len,
    int                     dir
    )
{   
    md_copy_t* req = md_alloc_copy_request(dst, src, len, dir);     
    unsigned long tmp;
    dma_addr_t addrDst;
    dma_addr_t addrSrc;
    u32 dwCmdWord[4];                     
            
    if (req)
    {                          
        addrDst = req->dma_dst;
        addrSrc = req->dma_src;        
           
        while(len)
        {
            //to cover md copy bus, see mantis 9801 & 9809
            int bytes_to_next_8bytes_align = (8 - (addrSrc & 0x7)) & 0x7;
        
            if(((len - bytes_to_next_8bytes_align) & 0xFF) == 8)        
                tmp = 8;        
            else        
                tmp = len;                     
    
            dwCmdWord[0] = MOVE_DATA_SS;
            
            if(!req->dir) 
                dwCmdWord[0] |= 1 << 7;    //backward
     
            dwCmdWord[1] = (u32)addrDst;
            dwCmdWord[2] = (u32)addrSrc;
            dwCmdWord[3] = (u32)tmp;
     
            req->request_id = md_write((const char *)dwCmdWord, sizeof(dwCmdWord));
            len     -= tmp;
            addrDst += tmp;
            addrSrc += tmp;
        }
    }
    
    return (req) ? req->copy_id : 0;
}




/*-------------------------------------------------------------------- 
 * Func : md_copy_sync 
 *
 * Desc : sync md copy request
 *
 * Parm : md_copy_t
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
int md_copy_sync(MD_COPY_HANDLE handle)
{               
    u64 request_id;    
    
    //md_check_copy_tasks();
    
    if (handle >= dev->finished_copy_counter)   // task does not finished yet...
    {
        md_lock(dev);
        
        request_id = dev->copy_task[handle % MAX_COPY_TASK].request_id;
        
        md_unlock(dev);
        
        while(md_checkfinish(request_id)==0)                                                                              
            udelay(1);
        
        md_check_copy_tasks();
    }

    return 0;        
}





/*-------------------------------------------------------------------- 
 * Func : md_memcpy 
 *
 * Desc : doing memory copy via md
 *
 * Parm : lpDst   :   Destination
 *        lpSrc   :   Source
 *        len     :   number of bytes
 *        forward :   1 : Src to Dest, 0 Dest to Src
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
int md_memcpy(
    void*                   lpDst, 
    void*                   lpSrc, 
    int                     len, 
    bool                    forward
    )
{            
    MD_CMD_HANDLE mdh = md_copy_start(lpDst, lpSrc, len, forward);
    
    if (mdh)    
        md_copy_sync(mdh);      // wait complete         
        
    return (mdh) ? 0 : -1; 	
}





/*-------------------------------------------------------------------- 
 * Func : md_check_physical_alignment 
 *
 * Desc : check if this buff is physical alignment
 *
 * Parm : buff    :   buff      (kernel virtual address)
 *        len     :   length    
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
int md_check_physical_alignment(
    void*                   buff,     
    int                     len)
{   
    int pc = (len + ((u32)buff & PAGE_SIZE)) >> PAGE_SHIFT;
    int i;
    u32 phy_addr = (u32) virt_to_phys(buff);
    
    for (i=1; i<pc; i++)     
    {
        if ((u32) virt_to_phys(buff) != phy_addr)
            return 0;

        buff     += PAGE_SIZE;
        phy_addr += PAGE_SIZE;        
    }
    
    return 1;    
}




/*-------------------------------------------------------------------- 
 * Func : md_is_valid_address 
 *
 * Desc : check address for md access
 *
 * Parm : addr   :   Destination 
 *        len    :   number of bytes 
 *
 * Retn : 1 for valid address, 0 for invalid address space
 --------------------------------------------------------------------*/
int md_is_valid_address(
    void*                   addr, 
    unsigned long           len
    )
{    
    u32 address = (u32) addr;    

    
    if (address > 0x7FFFFFFF && address < 0xC0000000)        
    {
        // unmapped kernel address (0x8000_0000 ~ 0xBFFF_FFFF)                                    
        return md_check_physical_alignment(addr, len);
    }        
    else
    {                
        struct vm_area_struct* vma; 
        
        vma = find_extend_vma(current->mm, address);
        if (vma && (vma->vm_flags & VM_DVR))
            return 1;           // memory allocated by pli
    }        

    return 0;    
}


/*-------------------------------------------------------------------- 
 * Func : md_memset 
 *
 * Desc : doing memory set via md
 *
 * Parm : lpDst   :   Destination 
 *        data    :   data to set
 *        len     :   number of bytes 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
MD_CMD_HANDLE md_memset(
    void*                   lpDst, 
    u8                      data, 
    int                     len
    )
{    
    u32 dwCmdWord[4];
    
	dwCmdWord[0] = MOVE_DATA_SS | (1 << 6);
	dwCmdWord[1] = (u32) virt_to_phys(lpDst);
	dwCmdWord[2] = (u32)data;
	dwCmdWord[3] = (u32)len;

    return (MD_CMD_HANDLE) md_write((const char *)dwCmdWord, sizeof(dwCmdWord));    
}



/*-------------------------------------------------------------------- 
 * Func : md_write 
 *
 * Desc : send command to MD
 *
 * Parm : lpDst   :   Destination 
 *        data    :   data to set
 *        len     :   number of bytes 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
MD_CMD_HANDLE md_write(const char *buf, size_t count)
{
    return WriteCmd(dev, (u8 *)buf, count);
}





/***************************************************************************
         ------------------- File Operations ----------------
****************************************************************************/

#ifdef CONFIG_REALTEK_MD_DEVICE_FILE


/*------------------------------------------------------------------
 * Func : md_dev_open
 *
 * Desc : open function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int md_dev_open(struct inode *inode, struct file *file)
{       
    return 0;    
}



/*------------------------------------------------------------------
 * Func : mcp_dev_release
 *
 * Desc : release function of mcp dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int md_dev_release(
    struct inode*           inode, 
    struct file*            file
    )
{               
	return 0;
}



/*------------------------------------------------------------------
 * Func : md_dev_ioctl
 *
 * Desc : ioctl function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *        cmd   : control command
 *        arg   : arguments
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int md_dev_ioctl(
    struct inode*           inode, 
    struct file*            file,
    unsigned int            cmd, 
    unsigned long           arg
    )
{                      
    MD_COPY_HANDLE  handle;
    MD_COPY_CMD     cp_cmd;

    //printk("cmd=%08x, arg=%08x\n", cmd, arg);
    
	switch (cmd)
    {
    case MD_IOCTL_MEM_COPY:                            
        
        if (copy_from_user((void __user *) &cp_cmd, (void __user *)arg, sizeof(MD_COPY_CMD)))
        {            
            printk("[MD] WARNING, do md copy failed, copy data from user failed\n");
			return -EFAULT;
        }
        if (!md_is_valid_address(cp_cmd.dst, cp_cmd.len) || !md_is_valid_address(cp_cmd.src, cp_cmd.len))   
        {
            printk("[MD] WARNING, do md copy failed, invalid address. MD copy only can be used in memory allocated by PLI\n");
            return -EFAULT;
        }
        
        if (cp_cmd.flags & MD_FLAG_NON_BLOCKING)    
        {
            cp_cmd.request_id = md_copy_start(cp_cmd.dst, cp_cmd.src, cp_cmd.len, 1);
            
            if (!cp_cmd.request_id)
            {
                printk("[MD] WARNING, do md copy failed, too much work in queue\n");
                return -EFAULT;
            }   
            
            return (copy_to_user((MD_COPY_CMD __user *)arg, &cp_cmd, sizeof(MD_COPY_CMD))) ? -EFAULT : 0;             
        }   
        else            
            return md_memcpy(cp_cmd.dst, cp_cmd.src, cp_cmd.len, 1);
                         
        break;
                
    case MD_IOCTL_MEM_COPY_SYNC:
        
        if (copy_from_user(&handle, (MD_COPY_HANDLE __user *)arg, sizeof(MD_COPY_HANDLE)))
			return -EFAULT;
			
        return md_copy_sync(handle);        
                                        
	default:		
	    printk("[MD] WARNING, unknown command\n");                
		return -EFAULT;          
	}	
    return 0;          
}



static struct file_operations md_ops = 
{
	.owner		= THIS_MODULE,	
	.ioctl		= md_dev_ioctl,
	.open		= md_dev_open,
	.release	= md_dev_release,
};


#endif


/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/

static int __init md_init(void)
{
    md_open();

#ifdef CONFIG_REALTEK_MD_DEVICE_FILE

    cdev_init(&md_cdev, &md_ops);            
                
    if (alloc_chrdev_region(&devno, 0, 1, MD_DEV_FILE_NAME)!=0)    
    {
        cdev_del(&md_cdev);
        return -EFAULT;
    }                                 
    
    if (cdev_add(&md_cdev, devno, 1)<0)
        return -EFAULT;                          
                      
    devfs_mk_cdev(devno, S_IFCHR|S_IRUSR|S_IWUSR, MD_DEV_FILE_NAME);         
    
#endif        
        
    md_device = platform_device_register_simple("MD", 0, NULL, 0);      
    return 0;	
}


static void __exit md_exit(void)
{
    platform_device_unregister(md_device);
   
#ifdef CONFIG_REALTEK_MD_DEVICE_FILE  
    cdev_del(&md_cdev);
    devfs_remove(MD_DEV_FILE_NAME);
    unregister_chrdev_region(devno, 1);     
#endif
    
	md_close();		   
}



module_init(md_init);
module_exit(md_exit);
