#ifndef __RTL8711_SDIO_REGDEF_H__
#define __RTL8711_SDIO_REGDEF_H__

//--------------------------
//			Func 0
//--------------------------
#define Func0_CCCR  			0x0000
#define Func0_FBR1  			0x0100




#define Host_W_FIFO  		(RTL8711_HCICTRL_ + (0x0000))  //0x08000
#define Host_R_FIFO   		(RTL8711_HCICTRL_ + (0x0001))  //0x08001
#define Reg_W_FIFO   		(RTL8711_HCICTRL_ + (0x0002))  //0x08002
#define Reg_R_FIFO    		(RTL8711_HCICTRL_ + (0x0003))  //0x08003
#define TCP_W_FIFO   		(RTL8711_HCICTRL_ + (0x000E))  //0x0800E
#define TCP_R_FIFO    		(RTL8711_HCICTRL_ +(0x000F))  //0x0800F

#define RCA					(RTL8711_HCICTRL_ + (0x0004)) // 2 byte
#define OCR					(RTL8711_HCICTRL_ + (0x0006)) // 4 byte
#define CSR					(RTL8711_HCICTRL_ +(0x000A))  // 4 byte

#endif

