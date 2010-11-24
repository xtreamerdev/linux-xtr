#ifndef __MCP_H__
#define __MCP_H__

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

#define MARS_MCP_MODE(x)     (x & 0x1F)   

#define MCP_ALGO_DES         0x00
#define MCP_ALGO_3DES        0x01
#define MCP_ALGO_RC4         0x02
#define MCP_ALGO_MD5         0x03
#define MCP_ALGO_SHA_1       0x04
#define MCP_ALGO_AES         0x05
#define MCP_ALGO_AES_G       0x06
#define MCP_ALGO_AES_H       0x07
#define MCP_ALGO_CMAC        0x08

#define MARS_MCP_BCM(x)     ((x & 0x3) << 6)  
#define MCP_BCM_ECB          0x0
#define MCP_BCM_CBC          0x1
#define MCP_BCM_CTR          0x2

#define MARS_MCP_ENC(x)     ((x & 0x1) << 5)        


typedef struct 
{
    unsigned long flags;    
    unsigned long key[6];  
    unsigned long iv[4];  
    unsigned long data_in;      // data in : physical address
    unsigned long data_out;     // data out : physical address 
    unsigned long length;       // data length
}mcp_desc;

typedef struct 
{
    mcp_desc*     p_desc;
    unsigned long n_desc;    
}mcp_desc_set;


#define MCP_IOCTL_DO_COMMAND    0x70000001
#define MCP_IOCTL_TEST_AES_H    0x71000001

#if 1
#define DBG_PRINT(s, args...) printk(s, ## args)
#else
#define DBG_PRINT(s, args...)
#endif



/***************************************************************************
               ------------------- APIS ----------------
****************************************************************************/


/********************************************************************************
 AES_H Hashing
 ********************************************************************************/
#define AES_H_FILE_HASHING_BUFFER_SIZE        (512 * 1024)

int MCP_AES_H_HASH_INIT(mcp_desc* p_desc);
int MCP_AES_H_IV_UPDATE(mcp_desc* p_desc, unsigned char Hash[16]);
int MCP_AES_H_Hashing(mcp_desc* p_desc, unsigned char* p_in, unsigned long len, unsigned char* p_out);
int MCP_AES_H_DataHash(unsigned char* p_data, unsigned long data_len, unsigned char hash[16], unsigned long block_size, unsigned char first_block);


/********************************************************************************
 AES 
 ********************************************************************************/ 
int MCP_AES_DESC_INIT(mcp_desc* pDesc, unsigned char EnDe, unsigned char Mode, unsigned char Key[16], unsigned char IV[16]);
int MCP_AES_Decryption(unsigned char mode, unsigned char key[16], unsigned char iv[16], unsigned char* p_in, unsigned char* p_out, unsigned long len);
int MCP_AES_Encryption(unsigned char mode, unsigned char key[16], unsigned char iv[16], unsigned char* p_in, unsigned char* p_out, unsigned long len);
#define MCP_AES_ECB_Decryption(key, p_in, p_out, len)       MCP_AES_Decryption(MCP_BCM_ECB, key, NULL, p_in, p_out, len)
#define MCP_AES_ECB_Encryption(key, p_in, p_out, len)       MCP_AES_Encryption(MCP_BCM_ECB, key, NULL, p_in, p_out, len)
#define MCP_AES_CBC_Decryption(key, iv, p_in, p_out, len)   MCP_AES_Decryption(MCP_BCM_CBC, key, iv, p_in, p_out, len)
#define MCP_AES_CBC_Encryption(key, iv, p_in, p_out, len)   MCP_AES_Encryption(MCP_BCM_CBC, key, iv, p_in, p_out, len)
#define MCP_AES_CTR_Decryption(key, iv, p_in, p_out, len)   MCP_AES_Decryption(MCP_BCM_CBC, key, iv, p_in, p_out, len)
#define MCP_AES_CTR_Encryption(key, iv, p_in, p_out, len)   MCP_AES_Encryption(MCP_BCM_CBC, key, iv, p_in, p_out, len)


#endif  // __MCP_H__
