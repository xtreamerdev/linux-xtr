#ifndef __KMD_H__
#define __KMD_H__

#include <asm/semaphore.h>

typedef u64 MD_CMD_HANDLE;

typedef struct se_cmd_counter_t {
    u32 low;
    u32 high;
} md_cmd_counter;

 
struct md_dev 
{
    void*   CmdBuf;  /* Pointer to first quantum set */
    void*   CachedCmdBuf;
    void*   CmdBase;
    void*   CmdLimit;
    int     wrptr;
    int     v_to_p_offset;
    int     size;
    md_cmd_counter sw_counter;
    struct semaphore sem;      
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
void md_write(const char *buf, size_t count);
bool md_checkfinish(MD_CMD_HANDLE handle);
MD_CMD_HANDLE md_memcpy(void* lpDst, void* lpSrc, int len, bool forward);
MD_CMD_HANDLE md_memset(void* lpDst, u8    data,  int len);


#endif  // __KMD_H__
