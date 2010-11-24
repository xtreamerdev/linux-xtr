#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <asm/mach-venus/mars.h>
#include <asm/mach-venus/mcp.h>
#include <asm/semaphore.h>
#include <asm/io.h>
#include <platform.h>
#include "mcp_reg.h"


#define MCP_DEV_FILE_NAME       "mcp"
#define mcp_desc_ENTRY_COUNT    10

//#define dbg_char(x)           writel((x) ,0xb801B200)
#define dbg_char(x)

static struct platform_device*  mcp_device;
static struct cdev      mcp_dev;
static dev_t            devno;
static spinlock_t       mcp_buffer_lock;

DECLARE_MUTEX(mcp_semaphore);


///////////////////// MACROS /////////////////////////////////
#define _mcp_map_single(p_data, len, dir)      dma_map_single(&mcp_device->dev, p_data, len, dir)
#define _mcp_unmap_single(p_data, len, dir)    dma_unmap_single(&mcp_device->dev, p_data, len, dir)



/*------------------------------------------------------------------
 * Func : mcp_init
 *
 * Desc : init mcp engine
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static int mcp_init(void)
{                   
    if (is_mars_cpu())
    {           		    
        spin_lock_init(&mcp_buffer_lock);            
        return 0;
    }
    
    printk("[MCP] Init MCP failed - invalid CPU\n");
    return -1;    
}



/*------------------------------------------------------------------
 * Func : mcp_uninit
 *
 * Desc : uninit mcp engine
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static void mcp_uninit(void)
{        
    SET_MCP_CTRL (0);        
    msleep(10);         // wait for hw stop            
    SET_MCP_BASE (0);
    SET_MCP_LIMIT(0);
    SET_MCP_RDPTR(0);
    SET_MCP_WRPTR(0);                
}


void _DumpDesciptor(mcp_desc* d)
{
    printk("\n\nXXXXXXXX desc (%p) XXXXXXXXXX\n",d);
    printk("flag=%08lx\n",d->flags);
    printk("Key0=%08lx\n",d->key[0]);
    printk("Key1=%08lx\n",d->key[1]);
    printk("Key2=%08lx\n",d->key[2]);
    printk("Key3=%08lx\n",d->key[3]);
    printk("Key4=%08lx\n",d->key[4]);
    printk("Key5=%08lx\n",d->key[5]);
    printk("IV0=%08lx\n",d->iv[0]);
    printk("IV1=%08lx\n",d->iv[1]);
    printk("IV2=%08lx\n",d->iv[2]);
    printk("IV3=%08lx\n",d->iv[3]);
    printk("di=%08lx\n",d->data_in);
    printk("do=%08lx\n",d->data_out);
    printk("len=%08lx\n",d->length);
    printk("XXXXXXXX desc end XXXXXXXXXX\n\n");
}



/*------------------------------------------------------------------ 
 * Func : _mcp_dump_desc_buffer
 *
 * Desc : set descriptors buffer
 *
 * Parm : N/A
 *
 * Retn : dma_addr
 *------------------------------------------------------------------*/
void _mcp_dump_desc_buffer(void)
{
    unsigned long event;
    
    spin_lock_irqsave(&mcp_buffer_lock, event);   
        
    printk("BASE=%08x, RP=%08x, WP=%08x, LIMIT=%08x\n",
            GET_MCP_BASE(), 
            GET_MCP_RDPTR(), 
            GET_MCP_WRPTR(), 
            GET_MCP_LIMIT());    
        
    spin_unlock_irqrestore(&mcp_buffer_lock, event);           
}


/*------------------------------------------------------------------ 
 * Func : _mcp_set_desc_buffer
 *
 * Desc : set descriptors buffer
 *
 * Parm : base  : base address of descriptor buffer
 *        limit : limit address of descriptor buffer
 *        rp    : read pointer of descriptor buffer
 *        wp    : write pointer of descriptor buffer
 *
 * Retn : 0
 *------------------------------------------------------------------*/
int _mcp_set_desc_buffer(
    unsigned long           base, 
    unsigned long           limit, 
    unsigned long           rp, 
    unsigned long           wp
    )
{
    unsigned long event;
    spin_lock_irqsave(&mcp_buffer_lock, event);                           
                                        
    SET_MCP_BASE (base);
    SET_MCP_LIMIT(limit);
    SET_MCP_RDPTR(rp);
    SET_MCP_WRPTR(wp);
            
    spin_unlock_irqrestore(&mcp_buffer_lock, event);   
    
    return 0;
}



/*------------------------------------------------------------------ 
 * Func : _mcp_start_xfer
 *
 * Desc : Start Xfer
 *
 * Parm : N/A
 *
 * Retn : S_OK /  S_FALSE 
 *------------------------------------------------------------------*/
int _mcp_start_xfer(void)
{   
    int ret = -1;                   
    int WaitTime = 0x3FF << 2;                    

    SET_MCP_CTRL(MCP_CLEAR | MCP_WRITE_DATA);    // issue clear
        
    SET_MCP_EN(0xFE);               
    
    SET_MCP_STATUS(0xFE);    // clear status
            
    SET_MCP_CTRL(MCP_GO | MCP_WRITE_DATA);
                 
    while (WaitTime--)    
    {        
        //printk("STATUS=%08x, CTRL=%08x\n",GET_MCP_STATUS(), GET_MCP_CTRL());
        if (!(GET_MCP_CTRL() & MCP_GO) || GET_MCP_STATUS())                   
            break;
            
        if ((WaitTime & 0x3FF) ==0)
            msleep(10);                
    }
                
    ret = ((GET_MCP_STATUS() & ~MCP_RING_EMPTY)) ? -1 : 0;
    
    SET_MCP_CTRL(MCP_GO);               // clear go bit        
    
    SET_MCP_STATUS(0xFE);               // clear ring empty    
    
    return ret;
}



/*------------------------------------------------------------------ 
 * Func : mcp_do_command 
 *
 * Desc : Do Command
 *
 * Parm : p_desc : number of Descriptor to be Execute
 *        n_desc  : number of Descriptor to be Execute
 *
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int mcp_do_command(
    mcp_desc*               p_desc, 
    int                     n_desc
    )
{   
    dma_addr_t  addr;              
    int         ret = 0;    
    int         len = sizeof(mcp_desc) * n_desc;
            
    if (n_desc)        
    {                
        down(&mcp_semaphore);
        
        addr = _mcp_map_single((void*) p_desc, len, DMA_TO_DEVICE);
    
        _mcp_set_desc_buffer(addr, addr+len + sizeof(mcp_desc), addr, addr + len);
        
        //_mcp_dump_desc_buffer();        
        
        ret = _mcp_start_xfer(); 
        
        //_mcp_dump_desc_buffer();                     
        
        _mcp_set_desc_buffer(0, 0, 0, 0);
            
        _mcp_unmap_single(addr, len, DMA_TO_DEVICE);
                              
        up(&mcp_semaphore);
    }   
     
    return ret;
}


/***************************************************************************
               ------------------- APIS ----------------
****************************************************************************/



/********************************************************************************
 AES Hash
 ********************************************************************************/


/*------------------------------------------------------------------ 
 * Func : MCP_AES_H_HASH_INIT
 *
 * Desc : Init AES Hashing Descriptor
 *
 * Parm : pDesc      : Descriptor to be Load 
 *
 * Retn : 0 for success, others failed
 *------------------------------------------------------------------*/
int MCP_AES_H_HASH_INIT(mcp_desc* p_desc)
{
    memset(p_desc, 0, sizeof(mcp_desc));
    
    p_desc->flags = MARS_MCP_MODE(MCP_ALGO_AES_H) | MARS_MCP_BCM(MCP_BCM_ECB) | MARS_MCP_ENC(1);                        
                        
    p_desc->iv[0]   = 0x2DC2DF39;  
    p_desc->iv[1]   = 0x420321D0;
    p_desc->iv[2]   = 0xCEF1FE23;
    p_desc->iv[3]   = 0x74029D95;    
    return 0;
}



/*------------------------------------------------------------------ 
 * Func : MCP_AES_H_IV_UPDATE
 *
 * Desc : Update Initial Vector of AES Hashing Descriptor
 *
 * Parm : p_desc      : Descriptor to be Load 
 *
 * Retn : 0 for success, others failed
 *------------------------------------------------------------------*/
int MCP_AES_H_IV_UPDATE(mcp_desc* p_desc, unsigned char hash[16])
{
    p_desc->iv[0]   = (hash[ 0]<<24) + (hash[ 1]<<16) + (hash[ 2]<<8) + hash[ 3]; 
    p_desc->iv[1]   = (hash[ 4]<<24) + (hash[ 5]<<16) + (hash[ 6]<<8) + hash[ 7];
    p_desc->iv[2]   = (hash[ 8]<<24) + (hash[ 9]<<16) + (hash[10]<<8) + hash[11];
    p_desc->iv[3]   = (hash[12]<<24) + (hash[13]<<16) + (hash[14]<<8) + hash[15];  
    return 0;
}




/*------------------------------------------------------------------ 
 * Func : MCP_AES_H_Hashing
 *
 * Desc : Do AES H Hashing
 *
 * Parm : pDesc      : Descriptor to be Load 
 *        DataIn     : Data to be hashing (in Physical Address) 
 *        Len        : number of data to be hashing 
 *        DataOut    : Physical address that used to store hash value
 *
 * Retn : S_OK  /  S_FALSE 
 *------------------------------------------------------------------*/
int MCP_AES_H_Hashing(
    mcp_desc*               p_desc, 
    unsigned char*          p_in,     
    unsigned long           len,
    unsigned char*          p_out
    )
{   
    int ret = 0;
             
    dma_addr_t  addr1 = _mcp_map_single((void*) p_in, len, DMA_TO_DEVICE);
    
    dma_addr_t  addr2 = _mcp_map_single((void*) p_out, 16, DMA_FROM_DEVICE);
        
    p_desc->data_in  = addr1;
    
    p_desc->data_out = addr2;
    
    p_desc->length   = len;
                
    ret = mcp_do_command(p_desc, 1);                
    
    _mcp_unmap_single(addr1, len, DMA_TO_DEVICE);                
    
    _mcp_unmap_single(addr2, 16, DMA_FROM_DEVICE);                
    
    return ret; 
}



/*------------------------------------------------------------------ 
 * Func : MCP_AES_H_DataHash
 *
 * Desc : Compute hash value of Data via 128 bits AES_H_Hasing
 *
 * Parm : pData         : Start address of data
 *        DataLen       : Length of data
 *        Hash[16]      : 128 bits hash value of the file 
 *        BlockSize     : Block Size that used to compute Hashing Value 
 *        first_block   : 1 : this block is first block
 *                        0 : this block is not first block
 *
 *           if this block is first block, the iv of the descriptor
 *           will be reset to default. if this block is not first block
 *           then uses the hash vaule as the default iv.        
 * 
 * Retn : S_OK  /  S_FALSE 
 *------------------------------------------------------------------*/
int MCP_AES_H_DataHash(
    unsigned char*          p_data,
    unsigned long           data_len,
    unsigned char           hash[16],
    unsigned long           block_size,
    unsigned char           first_block
    )
{           
    mcp_desc desc;                            
    int      ret = 0;

    MCP_AES_H_HASH_INIT(&desc);
    
    if (!first_block)
        MCP_AES_H_IV_UPDATE(&desc, hash);  // Update Hash Value
                    
    while(data_len && !ret)
    {                                                                                             
        if (data_len > block_size)            
        {
            ret       = MCP_AES_H_Hashing(&desc, p_data, block_size, hash);    
            data_len -= block_size;
            p_data   += block_size;
        }                                
        else
        {
            ret       = MCP_AES_H_Hashing(&desc, p_data, data_len, hash);    
            p_data   += data_len;
            data_len = 0;             
        }            

        MCP_AES_H_IV_UPDATE(&desc, hash);  // Update Hash Value
    }
        
    return ret;    
}



/********************************************************************************
 AES
 ********************************************************************************/



/*====================================================================== 
 * Func : MCP_AES_DESC_INIT
 *
 * Desc : Init AES Descriptor
 *
 * Parm : pDesc      : Descriptor to be Load
 *        EnDe       : Encryption/Descryption
 *              0 for Decryption / 1 for Encryption
 *        Mode       : Operation Mode
 *        Key        : Key Value
 *        IV         : Initial Vector 
 *
 * Retn : S_OK /  S_FALSE 
 *======================================================================*/
int MCP_AES_DESC_INIT(
    mcp_desc*               pDesc, 
    unsigned char           EnDe,
    unsigned char           Mode,
    unsigned char           Key[16],
    unsigned char           IV[16]
    )
{            
    memset(pDesc, 0, sizeof(mcp_desc));
    
    switch (Mode)
    {
    case MCP_BCM_ECB:        
    case MCP_BCM_CBC:
    case MCP_BCM_CTR:                
        
        pDesc->flags = MARS_MCP_MODE(MCP_ALGO_AES) | MARS_MCP_BCM(Mode) | MARS_MCP_ENC(EnDe);
                
        if (Key)                                        
        {            
            pDesc->key[0]  = (Key[ 0]<<24) + (Key[ 1]<<16) + (Key[ 2]<<8) + Key[ 3];  
            pDesc->key[1]  = (Key[ 4]<<24) + (Key[ 5]<<16) + (Key[ 6]<<8) + Key[ 7];  
            pDesc->key[2]  = (Key[ 8]<<24) + (Key[ 9]<<16) + (Key[10]<<8) + Key[11];  
            pDesc->key[3]  = (Key[12]<<24) + (Key[13]<<16) + (Key[14]<<8) + Key[15];  
        }                    
        else
            pDesc->flags |= MARS_MCP_KEY_SEL(MCP_KEY_SEL_OTP);
                     
        if (IV)        
        {
            pDesc->iv[0]   = (IV[ 0]<<24) + (IV[ 1]<<16) + (IV[ 2]<<8) + IV[ 3];  
            pDesc->iv[1]   = (IV[ 4]<<24) + (IV[ 5]<<16) + (IV[ 6]<<8) + IV[ 7];  
            pDesc->iv[2]   = (IV[ 8]<<24) + (IV[ 9]<<16) + (IV[10]<<8) + IV[11];  
            pDesc->iv[3]   = (IV[12]<<24) + (IV[13]<<16) + (IV[14]<<8) + IV[15]; 
        }
        else
            pDesc->flags |= MARS_MCP_IV_SEL(MCP_IV_SEL_REG);
    
        return 0;                
        
    default:        
        printk("[MCP] Init AES descriptor failed - invalid mode (%d)\n", Mode);
        return -1;
    }        
}





/*====================================================================== 
 * Func : MCP_AES_Decryption
 *
 * Desc : Do AES Decryption
 *
 * Parm : mode       : Operation Mode
 *        key        : Key Value
 *        iv         : Initial Vector 
 *        p_in       : Data In 
 *        p_out      : Data Out
 *        len        : Data Length
 *
 * Retn : 0 for success, others failed
 *======================================================================*/
int MCP_AES_Decryption(    
    unsigned char           Mode,
    unsigned char           Key[16],
    unsigned char           IV[16],
    unsigned char*          p_in,
    unsigned char*          p_out,
    unsigned long           len
    )
{    
    mcp_desc    desc;
    int         ret;    
    dma_addr_t  addr1;
    dma_addr_t  addr2;

    MCP_AES_DESC_INIT(&desc, 0, Mode, Key, IV);

    addr1 = _mcp_map_single((void*) p_in, len, DMA_TO_DEVICE);    
    addr2 = _mcp_map_single((void*) p_out, len, DMA_FROM_DEVICE);    

    desc.data_in  = addr1;
    desc.data_out = addr2;
    desc.length   = len & ~0xF;    
    
    ret = mcp_do_command(&desc, 1);   
    
    _mcp_unmap_single(addr1, len, DMA_TO_DEVICE);                    
    _mcp_unmap_single(addr2, len, DMA_FROM_DEVICE);  
    
    return ret;
}




/*====================================================================== 
 * Func : MCP_AES_Encryption
 *
 * Desc : Do AES Encryption
 *
 * Parm : mode       : Operation Mode
 *        key        : Key Value
 *        iv         : Initial Vector 
 *        p_in       : Data In 
 *        p_out      : Data Out
 *        len        : Data Length
 *
 * Retn : 0 for success, others failed
 *======================================================================*/
int MCP_AES_Encryption(    
    unsigned char           mode,
    unsigned char           key[16],
    unsigned char           iv[16],
    unsigned char*          p_in,
    unsigned char*          p_out,
    unsigned long           len
    )
{    
    mcp_desc    desc;
    int         ret;    
    dma_addr_t  addr1;    
    dma_addr_t  addr2;    
    
    MCP_AES_DESC_INIT(&desc, 1, mode, key, iv);
    
    addr1 = _mcp_map_single((void*) p_in, len, DMA_TO_DEVICE);    
    addr2 = _mcp_map_single((void*) p_out, len, DMA_FROM_DEVICE);    
    
    desc.data_in  = addr1;
    desc.data_out = addr2;
    desc.length   = len & ~0xF;
    
    ret = mcp_do_command(&desc, 1);   
    
    _mcp_unmap_single(addr1, len, DMA_TO_DEVICE);                
    
    _mcp_unmap_single(addr2, len, DMA_FROM_DEVICE);  
    
    return ret; 
}



/***************************************************************************
            ------------------- Auto Test ----------------
****************************************************************************/



/*------------------------------------------------------------------ 
 * Func : MCP_AES_H_DataHash
 *
 * Desc : Compute hash value of Data via 128 bits AES_H_Hasing
 *
 * Parm : pData      : Start address of data
 *        DataLen    : Length of data
 *        Hash[16]   : 128 bits hash value of the file 
 *        BlockSize  : Block Size that used to compute Hashing Value 
 *
 * Retn : S_OK  /  S_FALSE 
 *------------------------------------------------------------------*/
void MCP_AES_H_DataHashTest(void)
{
    unsigned char Data[256];        
    unsigned char Hash[16]; 
    int i;                   
    
    memset(Data, 0x33, sizeof(Data));
    
    MCP_AES_H_DataHash(Data,  sizeof(Data), Hash, AES_H_FILE_HASHING_BUFFER_SIZE, 1);    

    printk("hash value : ");
    for (i=0; i<16; i++)
        printk("%02x ", Hash[i]);
    printk("\n");
}





/*------------------------------------------------------------------ 
 * Func : MCP_AES_H_DataHash
 *
 * Desc : Compute hash value of Data via 128 bits AES_H_Hasing
 *
 * Parm : pData      : Start address of data
 *        DataLen    : Length of data
 *        Hash[16]   : 128 bits hash value of the file 
 *        BlockSize  : Block Size that used to compute Hashing Value 
 *
 * Retn : S_OK  /  S_FALSE 
 *------------------------------------------------------------------*/
void MCP_AES_128_ECB_DataEncrypt(void)
{
    unsigned char Data[16]={0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 
                            0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00};        
    unsigned char Key[16]; 
    int i;                   
    
    memset(Key, 0xff, sizeof(Key));
    
    MCP_AES_ECB_Encryption(Key, Data, Data, 16);

    printk("encrypted value : ");
    for (i=0; i<16; i++)
        printk("%02x ", Data[i]);
    printk("\n");
        
    MCP_AES_ECB_Decryption(Key, Data, Data, 16);
    
    printk("decrypted value : ");
    for (i=0; i<16; i++)
        printk("%02x ", Data[i]);
        
    printk("\n");
}


/***************************************************************************
         ------------------- File Operations ----------------
****************************************************************************/



/*------------------------------------------------------------------
 * Func : mcp_dev_open
 *
 * Desc : open function of mcp dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_dev_open(struct inode *inode, struct file *file)
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
int mcp_dev_release(
    struct inode*           inode, 
    struct file*            file
    )
{               
	return 0;
}



/*------------------------------------------------------------------
 * Func : mcp_dev_ioctl
 *
 * Desc : ioctl function of mcp dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *        cmd   : control command
 *        arg   : arguments
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_dev_ioctl(
    struct inode*           inode, 
    struct file*            file,
    unsigned int            cmd, 
    unsigned long           arg
    )
{                   
    mcp_desc_set desc_set;    
    mcp_desc     desc_list[mcp_desc_ENTRY_COUNT];    
    mcp_desc*    p_desc;    
    int          n_desc;
             
	switch (cmd)
    {
    case MCP_IOCTL_DO_COMMAND:                
                
        if (copy_from_user(&desc_set, (mcp_desc_set __user *)arg, sizeof(mcp_desc_set)))
			return -EFAULT;
				
        p_desc = desc_set.p_desc;                                
        
        while(desc_set.n_desc)         				
        {            
            n_desc = (desc_set.n_desc >= mcp_desc_ENTRY_COUNT) ? mcp_desc_ENTRY_COUNT 
                                                               : desc_set.n_desc;            
            
            if (copy_from_user(&desc_list, (mcp_desc __user *)p_desc, sizeof(mcp_desc) * n_desc))
                return -EFAULT;            
            
            if (mcp_do_command(desc_list, n_desc)<0)                            
                return -EFAULT;
            
            p_desc          += sizeof(mcp_desc) * n_desc;
            desc_set.n_desc -= n_desc;
        }                		  
        break;
        
    //-----------------------------------
    // for testing only        
    //-----------------------------------                
    case MCP_IOCTL_TEST_AES_H:
        MCP_AES_H_DataHashTest();
        break;
                                        
	default:		
		return -EFAULT;          
	}
	
    return 0;          
}



static struct file_operations mcp_ops = 
{
	.owner		= THIS_MODULE,	
	.ioctl		= mcp_dev_ioctl,
	.open		= mcp_dev_open,
	.release	= mcp_dev_release,
};



/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/



/*------------------------------------------------------------------
 * Func : mcp_init
 *
 * Desc : mcp module init function
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static int __init mcp_module_init(void)
{   
    if (mcp_init()<0)    
        return -ENODEV;    
                    
    cdev_init(&mcp_dev, &mcp_ops);            
                
    if (alloc_chrdev_region(&devno, 0, 1, MCP_DEV_FILE_NAME)!=0)    
    {
        cdev_del(&mcp_dev);
        return -EFAULT;
    }                                 
    
    if (cdev_add(&mcp_dev, devno, 1)<0)
        return -EFAULT;                          
                      
    devfs_mk_cdev(devno, S_IFCHR|S_IRUSR|S_IWUSR, MCP_DEV_FILE_NAME);         
    
    mcp_device = platform_device_register_simple("MCP", 0, NULL, 0);                           
    //MCP_AES_128_ECB_DataEncrypt();   
    //MCP_AES_H_DataHashTest();
        
    return 0;        
}



/*------------------------------------------------------------------
 * Func : mcp_module_exit
 *
 * Desc : mcp module exit function
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static void __exit mcp_module_exit(void)
{    	    
    platform_device_unregister(mcp_device);

    cdev_del(&mcp_dev);
    devfs_remove(MCP_DEV_FILE_NAME);
    unregister_chrdev_region(devno, 1);            
    mcp_uninit();
}


module_init(mcp_module_init);
module_exit(mcp_module_exit);
