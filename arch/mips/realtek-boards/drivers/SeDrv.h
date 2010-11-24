//=================================================================================================
// Copyright (c) 2008 Realtek Semiconductor Corp. All Rights Reserved.
//
// Filename: SeDrv.h
// Abstract:
//   Related Definitions of Hardware Control Function for Columbus Streaming Engine
//
// History:
//	 2008-02-15			cyyang			Initial Version
//=================================================================================================

#ifndef _SE_DRV_H_
#define _SE_DRV_H_

#include <Platform_Lib/Graphics/se.h>
#include <sys/ioctl.h>
#include "dcu_reg.h"
#include "sb2_reg.h"
#include "se_local.h"
#include "se_export.h"

#ifndef _SE_REG_H_
#include "SeReg.h"
#endif

#ifndef _SE_CMD_H_
#include "SeCmd.h"
#endif

#ifndef _SE_LIB_H_
#include "SeLib.h"
#endif

//=================================================================================================

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

#define SE_REG(r) (r)

#define SE_CLR_WRITE_DATA 0
//#define SE_SET_WRITE_DATA BIT0

#define SeClearWriteData 0

//se control bits
typedef enum 
{
	SeWriteData = BIT0,
    SeGo = BIT1,
    SeEndianSwap = BIT2
    
} SE_CTRL_REG;

typedef enum 
{
	SeIdle = BIT0
    
} SE_IDLE_REG;


//interrupt status and control bits
typedef enum 
{
    SeIntCommandEmpty = BIT3,
    SeIntCommandError = BIT2,
    SeIntSync = BIT1
    
} SE_INT_STATUS_REG, SE_INT_CTRL_REG;


//=================================================================================================

HRESULT 		se_open(void);
HRESULT 		se_close(void);
size_t 			se_write(HANDLE hQueue, const int8_t *pbyCommandBuffer, size_t dwCommandLength, int inst_count);
HRESULT 		se_ioctl(uint32_t dwCommandCode, void *arg);
bool 			se_checkfinish(SE_CMD_HANDLE hCommandHandle);
SE_CMD_HANDLE 	se_memcpy(void *lpDst, void *lpSrc, int len, bool forward);
SE_CMD_HANDLE	se_memset(void *lpDst, uint8_t data, int len);

size_t 
se_write_ex(
	uint8_t			byCommandQueue,
	const int8_t 	*pbyCommandBuffer, 
	size_t 			dwCommandLength
	);

void 
SeWriteCommand(
	uint8_t			byCommandQueue,
	struct se_dev 	*pSeDeviceInfo, 
	uint8_t 		*pbyCommandBuffer, 
	uint32_t 		dwCommandLength
	);


//=================================================================================================

#endif

//=================================================================================================
// End of File
