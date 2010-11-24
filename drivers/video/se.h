#ifndef STREAM_ENGINE_H
#define STREAM_ENGINE_H

//#include <OSAL.h>
//#include <Util.h>
//#include <hresult.h>
//#include <simulator/dcu_mem.h>

typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;

#ifdef __cplusplus
extern "C" {
#endif

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

#define SE_COMMAND_ENTRIES 4096
#define INST_CNT_MASK 0xFFFF

#ifdef SIMULATOR
#define SE_REG(r) (r)
#else
#define SE_REG_BASE 0xB800C000  //TBD
#define SE_CNTL         ((volatile unsigned int *)0xB800C000)
#define SE_INT_STATUS   ((volatile unsigned int *)0xB800C004)
#define SE_INT_ENABLE   ((volatile unsigned int *)0xB800C008)
#define SE_INST_CNT     ((volatile unsigned int *)0xB800C010)
#define SE_DBG          ((volatile unsigned int *)0xB800C01C)
#define SE_CmdBase      ((volatile unsigned int *)0xB800C020)
#define SE_CmdLimit     ((volatile unsigned int *)0xB800C024)
#define SE_CmdRdptr     ((volatile unsigned int *)0xB800C028)
#define SE_CmdWrptr     ((volatile unsigned int *)0xB800C02C)
#define SE_CmdFifoState ((volatile unsigned int *)0xB800C030)
#define SE_AddrMap      ((volatile unsigned int *)0xB800C040)
#define SE_Color_Key    ((volatile unsigned int *)0xB800C044)
#define SE_Def_Alpha    ((volatile unsigned int *)0xB800C048)
#define SE_Rslt_Alpha   ((volatile unsigned int *)0xB800C04C)
#define SE_Src_Color    ((volatile unsigned int *)0xB800C050)
#define SE_Src_Alpha    ((volatile unsigned int *)0xB800C054)
#define SE_Clr_Format   ((volatile unsigned int *)0xB800C058)
#define SE_Input        ((volatile unsigned int *)0xB800C05C)
#define SE_REG(r) (r)
//#define SE_REG(r) (*(DWORD *)(r))
#endif

#define DONT_CARE 0

#define SE_CLR_WRITE_DATA 0
#define SE_SET_WRITE_DATA BIT0

//se control bits
typedef enum {
    //SE_WRITE_BIT=BIT0,
    //SE_CLR_WRITE_DATA=0,
    //SE_SET_WRITE_DATA=BIT0,
    SE_GO=BIT1,
    SE_IDLE=BIT2,
    SE_RESET=BIT3
} SE_CTRL;

//interrupt status and control bits
typedef enum {
    INT_COML_EMPTY=BIT3,
    INT_SYNC=BIT2,
    INT_COM_ERR=BIT1,
} INT_STATUS, INT_CTRL;

typedef enum {
    CONST_ALPHA=0,
    ONE_MINUS_CONST_ALPHA=1,
    DST_PIXEL_ALPHA=2,
    ONE_MINUS_DST_PIXEL_ALPHA=3,
    SRC_PIXEL_ALPHA=4,
    ONE_MINUS_SRC_PIXEL_ALPHA=5
} ALPHA_TYPE;

  
typedef enum {
    SEL_ALPHA_WITH_COLOR_KEY=0,
    SEL_ALPHA = 0,
    SEL_ROP = 1
} MODE_SELECT;
  
typedef enum {
    SEL_CONST=0,
    SEL_VECT=1
} SRC_SELECT;
  
typedef enum {
     NORMAL_ALPHA=0,
     RSLT_ALPHA = 1
} SEL_OUTPUT_ALPHA;

typedef enum {
    BLIT_FWD=0,
    BLIT_REV=1
} BLIT_DIR;

typedef enum {
    NO_COLOR_KEY = 0,
    SRC_COLOR_KEY = 1,
    DEST_COLOR_KEY = 2,
    RESVD_COLOR_KEY = 3
}COLOR_KEY_MODE;

typedef enum {
    DRAW_PEL=0x0,
    WRITE_REG=0x1,
    BITBLIT=0x2,
    //BM_AREA=0x3,
    MOVE_DATA=0x5,
    SYNC=0x7,
    NO_OF_OPCODES
} OPCODE;

typedef enum {
    REG_SE_AddrMap=0x0,
    REG_SE_Color_Key=0x1,
    REG_SE_Def_Alpha=0x2,
    REG_SE_Rslt_Alpha=0x3,
    REG_SE_Src_Alpha=0x4,
    REG_SE_Src_Color=0x5,
    REG_SE_Clr_Format=0x6,
    REG_SE_BG_Color=0x7,
    REG_SE_BG_Alpha=0x8
} REG_ADDR;

typedef int (*func_t)(DWORD cmd_word);

//OpCode related function pointer and command length
typedef struct {
    func_t func;
    DWORD dwCmdLen;
} OpFunc_t;

//A represents destination while B represents source
typedef enum {
    ROP_BLACK=0,
    ROP_AND, 
    ROP_A_AND_NB,
    ROP_A,
    ROP_NA_AND_B,
    ROP_B,
    ROP_X_OR,
    ROP_OR,
    ROP_NOR,
    ROP_XNOR,
    ROP_NB,
    ROP_A_OR_NB,
    ROP_NA,
    ROP_NA_OR_B,
    ROP_NAND,
    ROP_WHITE=0xF
}ROPCODE;

/* applyOp w/stored dst*/
#define SE_applyOp(op, src, pdst, type)     \
{                       \
    type d = (pdst);            \
    switch (op) {               \
    case ROP_A:             \
        *d = (src);         \
        break;              \
    case ROP_B:             \
        break;              \
    case ROP_X_OR:               \
        *d ^= (src);            \
        break;              \
    case ROP_AND:           \
        *d &= (src);            \
        break;              \
    case ROP_OR:                \
        *d |= (src);            \
        break;              \
    case ROP_BLACK:         \
        *d = 0;             \
        break;              \
    case ROP_WHITE:         \
        *d = ~0;            \
        break;              \
    case ROP_XNOR:              \
        *d = ~(*d ^ (src));     \
        break;              \
    case ROP_NOR:               \
        *d = ~(*d | (src));     \
        break;              \
    case ROP_NAND:              \
        *d = ~(*d & (src));     \
        break;              \
    case ROP_NB:            \
        *d = ~*d;           \
        break;              \
    case ROP_NA:    \
        *d = ~(src);            \
        break;              \
    case ROP_NA_OR_B:           \
        *d |= ~(src);           \
        break;              \
    case ROP_NA_AND_B:      \
        *d &= ~(src);           \
        break;              \
    case ROP_A_OR_NB:           \
        *d = ~*d | (src);       \
        break;              \
    case ROP_A_AND_NB:      \
        *d = ~*d & (src);       \
        break;              \
    }                   \
}

#define MAKE_ALPHA(sel_alpha, const_alpha, src_pixel_alpha, dst_pixel_alpha, result) \
        { \
            if((sel_alpha) == CONST_ALPHA) \
            { \
                if((const_alpha)) \
                    (result) = (const_alpha) + 1; \
                else \
                    (result) = 0; \
            } \
            else if((sel_alpha) == ONE_MINUS_CONST_ALPHA) \
            { \
                if((const_alpha)) \
                    (result) = 256 - (const_alpha) - 1; \
                else \
                    (result) = 256; \
            } \
            else if((sel_alpha) == DST_PIXEL_ALPHA) \
            { \
                if((dst_pixel_alpha)) \
                    (result) = (dst_pixel_alpha) + 1; \
                else \
                    (result) = 0; \
            } \
            else if((sel_alpha) == ONE_MINUS_DST_PIXEL_ALPHA) \
            { \
                if((dst_pixel_alpha)) \
                    (result) = 256 - (dst_pixel_alpha) - 1; \
                else \
                    (result) = 256; \
            } \
            else if((sel_alpha) == SRC_PIXEL_ALPHA) \
            { \
                if((src_pixel_alpha)) \
                    (result) = (src_pixel_alpha) + 1; \
                else \
                    (result) = 0; \
            } \
            else \
            { \
                if((src_pixel_alpha)) \
                    (result) = 256 - (src_pixel_alpha) - 1; \
                else \
                    (result) = 256; \
            } \
        }

#define ALPHA_BLEND_8(s, sa, d, da, dst) \
        { \
            WORD a, b, c; \
            a = ((((((d) << 6) & 0xC0)*(da)) >> 8) + (((((s) << 6) & 0xC0)*(sa)) >> 8)); \
            a += 0x20; \
            if(a & 0xFF00) a = 0xC0; \
            b =((((((d) << 3) & 0xE0)*(da)) >> 8) + (((((s) << 3) & 0xE0)*(sa)) >> 8)); \
            b += 0x10; \
            if(b & 0xFF00) b = 0xE0; \
            c =(((((d) & 0xE0)*(da)) >> 8) + ((((s) & 0xE0)*(sa)) >> 8)); \
            c += 0x10; \
            if(c & 0xFF00) c = 0xE0; \
            *(UCHAR *)(dst) = (c & 0xE0) | ((b >> 3)  & 0x1C) | ((a >> 6) & 0x3); \
        }

#define ALPHA_BLEND_16(s, sa, d, da, dst) \
        { \
            WORD a, b, c; \
            a = ((((((d) << 3) & 0xF8)*(da)) >> 8) + (((((s) << 3) & 0xF8)*(sa)) >> 8)); \
            a += 0x04; \
            if(a & 0xFF00) a = 0xF8; \
            b =((((((d) >> 3) & 0xFC)*(da)) >> 8) + (((((s) >> 3) & 0xFC)*(sa)) >> 8)); \
            b += 0x02; \
            if(b & 0xFF00) b = 0xFC; \
            c =((((((d) >> 8) & 0xF8)*(da)) >> 8) + (((((s) >> 8) & 0xF8)*(sa)) >> 8)); \
            c += 0x04; \
            if(c & 0xFF00) c = 0xF8; \
            *(WORD *)(dst) = ((c << 8) & 0xF800) | ((b << 3)  & 0x07E0) | ((a >> 3) & 0x001F); \
        }

#define ALPHA_BLEND_32(s, sa, d, da, dst) \
        { \
            DWORD a, b, c, e; \
            a = (((((d) & 0xFF)*(da)) >> 8) + ((((s) & 0xFF)*(sa)) >> 8)); \
            if(a & 0xFF00) a = 0xFF; \
            b =((((((d) >> 8) & 0xFF)*(da)) >> 8) + (((((s) >> 8) & 0xFF)*(sa)) >> 8)); \
            if(b & 0xFF00) b = 0xFF; \
            c =((((((d) >> 16) & 0xFF)*(da)) >> 8) + (((((s) >> 16) & 0xFF)*(sa)) >> 8)); \
            if(c & 0xFF00) c = 0xFF; \
            e =((((((d) >> 24) & 0xFF)*(da)) >> 8) + (((((s) >> 24) & 0xFF)*(sa)) >> 8)); \
            if(e & 0xFF00) e = 0xFF; \
            *(DWORD *)(dst) = ((e << 24) & 0xFF000000) | ((c << 16) & 0xFF0000) | ((b << 8)  & 0xFF00) | (a & 0xFF); \
        }

#ifdef __cplusplus
}
#endif
#endif

