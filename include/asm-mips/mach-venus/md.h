#ifndef __KMD_H__
#define __KMD_H__

#include <linux/spinlock.h>

typedef u64     MD_CMD_HANDLE;
typedef u64     MD_COPY_HANDLE;


typedef struct se_cmd_counter_t {
    u32 low;
    u32 high;
} md_cmd_counter;


typedef struct {
    void*           dst;
    void*           src;
    int             len;
    int             dir;        
    
    //--------------------------
    MD_COPY_HANDLE  copy_id;

    MD_CMD_HANDLE   request_id;
    
    dma_addr_t      dma_dst;
    dma_addr_t      dma_src;
    
}md_copy_t;


 
struct md_dev 
{    
    void*           CmdBuf;  /* Pointer to first quantum set */
    void*           CachedCmdBuf;
    void*           CmdBase;
    void*           CmdLimit;
    int             wrptr;    
    int             v_to_p_offset;
    int             size;
    md_cmd_counter  sw_counter;    
    
    spinlock_t	    lock;    
    unsigned long   spin_flags;        
    
#define MAX_COPY_TASK   64
    u64             copy_counter;
    u64             finished_copy_counter;
    md_copy_t       copy_task[MAX_COPY_TASK];

};



#if 1
#define DBG_PRINT(s, args...) printk(s, ## args)
#else
#define DBG_PRINT(s, args...)
#endif

//#define endian_swap_32(a) (((a)>>24) | (((a)>>8) & 0xFF00) | (((a)<<8) & 0xFF0000) | (((a)<<24) & 0xFF000000))
#define endian_swap_32(a) (a)

void md_open(void);
void md_close(void);
MD_CMD_HANDLE md_write(const char *buf, size_t count);
bool md_checkfinish(MD_CMD_HANDLE handle);
MD_CMD_HANDLE md_memset(void* lpDst, u8 data,  int len);

// APIs for MD Copy    
int             md_is_valid_address (void* addr, unsigned long len);
MD_COPY_HANDLE  md_copy_start       (void* dst, void* src, int len, int dir);
int             md_copy_sync        (MD_COPY_HANDLE handle);
int             md_memcpy           (void* lpDst, void* lpSrc, int len, bool forward);


#ifdef CONFIG_REALTEK_MD_DEVICE_FILE

#define MD_IOCTL_COMMAND_BASE       0x1283A000
#define MD_IOCTL_COMMAND(x)        (MD_IOCTL_COMMAND_BASE + x)
#define MD_IOCTL_MEM_COPY          (MD_IOCTL_COMMAND(1))
#define MD_IOCTL_MEM_COPY_SYNC     (MD_IOCTL_COMMAND(2))

#define MD_FLAG_NON_BLOCKING        0x00000001    

typedef struct {    
    unsigned long   flags;
    void*           dst;
    void*           src;
    int             len;
    u64             request_id;
}MD_COPY_CMD;


#endif 

#endif  // __KMD_H__
