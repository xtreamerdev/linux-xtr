#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>                  
#include <asm/page.h>                  
#include "md.h"
#include "md_reg.h"

//#define dbg_char(x)                     writel((x) ,0xb801B200)
#define dbg_char(x)

static struct md_dev *dev = NULL;


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
		sema_init(&dev->sem, 0);
    }
	dev->size = SMQ_COMMAND_ENTRIES * sizeof(u32);

    dev->CachedCmdBuf = kmalloc(dev->size, GFP_KERNEL);                 // cached virt address
    dev->CmdBuf       =(void*)(((unsigned long)dev->CachedCmdBuf & 0x0FFFFFFF) | 0xA0000000);  // non cached virt address
    PhysAddr          = virt_to_phys(dev->CachedCmdBuf);                
    
	DBG_PRINT("[user land] CmdBuf virt addr = %p/%p, phy addr=%p\n", dev->CachedCmdBuf, (u32)dev->CmdBuf, (u32)PhysAddr);

    dev->v_to_p_offset = (int32_t)((u8 *)dev->CmdBuf - (u32)PhysAddr);

    dev->wrptr    = 0;
	dev->CmdBase  = (void *)PhysAddr;
	dev->CmdLimit = (void *)(PhysAddr + SMQ_COMMAND_ENTRIES*sizeof(u32));

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
                    
        down(&dev->sem);
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
void WriteCmd(
    struct md_dev*          dev, 
    u8*                     pBuf, 
    int                     nLen
    )
{
    int i;
    u8 *writeptr;
    u8 *pWptr;
    u8 *pWptrLimit;

    pWptrLimit = (u8 *)dev->CmdBuf + dev->size;
    pWptr      = (u8 *)dev->CmdBuf + dev->wrptr;

    writeptr = (u8*) readl(IOA_SMQ_CmdWrptr);
    
    dbg_char('*');    
    
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
    
    dbg_char('#');                
    
    return;
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
MD_CMD_HANDLE md_memcpy(
    void*                   lpDst, 
    void*                   lpSrc, 
    int                     len, 
    bool                    forward
    )
{        
    unsigned long tmp;
    unsigned long addrDst = (unsigned long) virt_to_phys(lpDst);
    unsigned long addrSrc = (unsigned long) virt_to_phys(lpSrc);
    u32 dwCmdWord[4];
    u64 counter64;   
    
    dbg_char('o');
    
    
    if (len < 512)    
    {
        memcpy(lpDst, lpSrc, len);        
        return 0;
    }
    
    
    up(&dev->sem);
       
    while(len)
    {
        //to cover md copy bus, see mantis 9801 & 9809
        int bytes_to_next_8bytes_align = (8 - (addrSrc & 0x7)) & 0x7;
    
        if(((len - bytes_to_next_8bytes_align) & 0xFF) == 8)
        {
            tmp = 8;
        }
        else
        {
            tmp = len;
        }

        dwCmdWord[0] = MOVE_DATA_SS;
        
        if(!forward) 
            dwCmdWord[0] |= 1 << 7;    //backward
 
        dwCmdWord[1] = (u32)addrDst;
        dwCmdWord[2] = (u32)addrSrc;
        dwCmdWord[3] = (u32)tmp;
 
        md_write((const char *)dwCmdWord, sizeof(dwCmdWord));
        len -= tmp;
        addrDst += tmp;
        addrSrc += tmp;
    }
    
    counter64 = dev->sw_counter.high;
    counter64 = (counter64 << 32) | dev->sw_counter.low;
    
    down(&dev->sem);    
    
    dbg_char('x');
    return counter64; 	
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
	u64 counter64;
	    
    up(&dev->sem);
    
	dwCmdWord[0] = MOVE_DATA_SS | (1 << 6);
	dwCmdWord[1] = (u32) virt_to_phys(lpDst);
	dwCmdWord[2] = (u32)data;
	dwCmdWord[3] = (u32)len;

    md_write((const char *)dwCmdWord, sizeof(dwCmdWord));

    counter64 = dev->sw_counter.high;
    counter64 = (counter64 << 32) | dev->sw_counter.low;

    down(&dev->sem);    
	return counter64;
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
void md_write(const char *buf, size_t count)
{
    WriteCmd(dev, (u8 *)buf, count);
}



static int __init md_init(void)
{
    md_open();
    return 0;	
}


static void __exit md_exit(void)
{
	md_close();
}



/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/
module_init(md_init);
module_exit(md_exit);
