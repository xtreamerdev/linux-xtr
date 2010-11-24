#ifndef SE_REG_H
#define SE_REG_H

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

#define SE_REG_BASE 0xB800C000  //TBD
#define IOA_SE_CNTL         ((volatile unsigned int *)0xB800C000)
#define IOA_SE_INT_STATUS   ((volatile unsigned int *)0xB800C004)
#define IOA_SE_INT_ENABLE   ((volatile unsigned int *)0xB800C008)
#define IOA_SE_INST_CNT     ((volatile unsigned int *)0xB800C010)
#define IOA_SE_DBG          ((volatile unsigned int *)0xB800C01C)
#define IOA_SE_CmdBase      ((volatile unsigned int *)0xB800C020)
#define IOA_SE_CmdLimit     ((volatile unsigned int *)0xB800C024)
#define IOA_SE_CmdRdptr     ((volatile unsigned int *)0xB800C028)
#define IOA_SE_CmdWrptr     ((volatile unsigned int *)0xB800C02C)
#define IOA_SE_CmdFifoState ((volatile unsigned int *)0xB800C030)
#define IOA_SE_AddrMap      ((volatile unsigned int *)0xB800C040)
#define IOA_SE_Color_Key    ((volatile unsigned int *)0xB800C044)
#define IOA_SE_Def_Alpha    ((volatile unsigned int *)0xB800C048)
#define IOA_SE_Rslt_Alpha   ((volatile unsigned int *)0xB800C04C)
#define IOA_SE_Src_Color    ((volatile unsigned int *)0xB800C050)
#define IOA_SE_Src_Alpha    ((volatile unsigned int *)0xB800C054)
#define IOA_SE_Clr_Format   ((volatile unsigned int *)0xB800C058)
#define IOA_SE_Input        ((volatile unsigned int *)0xB800C05C)

#define SE_REG(r) (r)

#define SE_CLR_WRITE_DATA 0
#define SE_SET_WRITE_DATA BIT0

//se control bits
typedef enum {
    //SE_WRITE_BIT=BIT0,
    //SE_CLR_WRITE_DATA=0,
    //SE_SET_WRITE_DATA=BIT0,
    SE_GO=BIT1,
    SE_IDLE=BIT2,
    SE_RESET=BIT3,
    SE_CMD_SWAP=BIT4
} SE_CTRL;

//interrupt status and control bits
typedef enum {
    INT_COML_EMPTY=BIT3,
    INT_SYNC=BIT2,
    INT_COM_ERR=BIT1,
} INT_STATUS, INT_CTRL;

typedef enum {
    DRAW_PEL=0x0,
    WRITE_REG=0x1,
    BITBLIT=0x2,
    //BM_AREA=0x3,
    MOVE_DATA=0x5,
    SYNC=0x7,
    NO_OF_OPCODES
} OPCODE;

#ifdef __cplusplus
}
#endif
#endif

