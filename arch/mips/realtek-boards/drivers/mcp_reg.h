#ifndef __MCP_REG_H__
#define __MCP_REG_H__



#define MARS_MCP_CTRL             0xb8015100
    #define MCP_GO                (0x01<<1)
    #define MCP_IDEL              (0x01<<2)
    #define MCP_SWAP              (0x01<<3)
    #define MCP_CLEAR             (0x01<<4)
    #define MCP_CHK_SYS_HDR       (0x01<<5)

    
#define MARS_MCP_STATUS                 0xb8015104          

    #define MCP_WRITE_DATA       (0x01)
    #define MCP_RING_EMPTY       (0x01 <<1)
    #define MCP_ERROR            (0x01 <<2)
    #define MCP_COMPARE          (0x01 <<3)
    #define MCP_HDR_OTHER_PCAK   (0x01 <<4)
    #define MCP_HDP_OTHER_PCAK   (0x01 <<5)
    #define MCP_ACP_ERR          (0x01 <<6)
    #define MCP_SYS_HDR_ERR      (0x01 <<7)
    


#define MARS_MCP_EN                     0xb8015108
#define MARS_MCP_BASE                   0xb801510C
#define MARS_MCP_LIMIT                  0xb8015110
#define MARS_MCP_RDPTR                  0xb8015114
#define MARS_MCP_WRPTR                  0xb8015118
#define MARS_MCP_DES_COUNT              0xb8015148
#define MARS_MCP_DES_COMPARE            0xb8015138




#define SET_MCP_CTRL(x)                 writel((x), (volatile unsigned int*) MARS_MCP_CTRL)
#define SET_MCP_STATUS(x)               writel((x), (volatile unsigned int*) MARS_MCP_STATUS)
#define SET_MCP_EN(x)                   writel((x), (volatile unsigned int*) MARS_MCP_EN)
#define SET_MCP_BASE(x)                 writel((x), (volatile unsigned int*) MARS_MCP_BASE)
#define SET_MCP_LIMIT(x)                writel((x), (volatile unsigned int*) MARS_MCP_LIMIT)
#define SET_MCP_RDPTR(x)                writel((x), (volatile unsigned int*) MARS_MCP_RDPTR)
#define SET_MCP_WRPTR(x)                writel((x), (volatile unsigned int*) MARS_MCP_WRPTR)

#define GET_MCP_CTRL()                  readl((volatile unsigned int*) MARS_MCP_CTRL)
#define GET_MCP_STATUS()                readl((volatile unsigned int*) MARS_MCP_STATUS)
#define GET_MCP_EN()                    readl((volatile unsigned int*) MARS_MCP_EN)
#define GET_MCP_BASE()                  readl((volatile unsigned int*) MARS_MCP_BASE)
#define GET_MCP_LIMIT()                 readl((volatile unsigned int*) MARS_MCP_LIMIT)
#define GET_MCP_RDPTR()                 readl((volatile unsigned int*) MARS_MCP_RDPTR)
#define GET_MCP_WRPTR()                 readl((volatile unsigned int*) MARS_MCP_WRPTR)


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

#define MARS_MCP_KEY_SEL(x) ((x & 0x1) << 12)  
#define MCP_KEY_SEL_OTP      0x1
#define MCP_KEY_SEL_DESC     0x0

#define MARS_MCP_IV_SEL(x)  ((x & 0x1) << 11)  
#define MCP_IV_SEL_REG      0x1
#define MCP_IV_SEL_DESC     0x0


#endif  // __MCP_REG_H__
