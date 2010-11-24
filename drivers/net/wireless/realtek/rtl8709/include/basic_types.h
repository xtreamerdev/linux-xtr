#ifndef __BASIC_TYPES_H__
#define __BASIC_TYPES_H__


#include <drv_conf.h>

#ifndef PLATFORM_LINUX
#ifndef BIT
	#define BIT(x)	( 1 << (x))
#endif
#endif

#define SUCCESS	0
#define FAIL	(-1)


#ifndef TRUE
	#define _TRUE		1
#else
	#define _TRUE	TRUE
#endif
		
#ifndef FALSE
	#define _FALSE		0
#else
	#define _FALSE	FALSE
#endif

#ifdef PLATFORM_WINDOWS

	typedef signed char s8;
	typedef unsigned char u8;

	typedef signed short s16;
	typedef unsigned short u16;

	typedef signed long s32;
	typedef unsigned long u32;
	
	typedef unsigned int	uint;
	typedef	signed int		sint;


	typedef signed long long s64;
	typedef unsigned long long u64;

	#ifdef NDIS50_MINIPORT
	
		#define NDIS_MAJOR_VERSION       5
		#define NDIS_MINOR_VERSION       0

	#endif

	#ifdef NDIS51_MINIPORT

		#define NDIS_MAJOR_VERSION       5
		#define NDIS_MINOR_VERSION       1

	#endif


#endif


#ifdef PLATFORM_LINUX

	#include <linux/types.h>
	#define IN
	#define OUT
	#define VOID void
	#define NDIS_OID uint
	#define NDIS_STATUS uint
	
	
	typedef	signed int sint;

#ifdef CONFIG_SDIO_HCI	
	#include <linux/sdio/ctsystem.h>	
#else
	#ifndef	PVOID
	#define PVOID	void *
	#endif

	#define UCHAR u8
	#define PUCHAR	u8*

	#define USHORT u16
	#define UINT u32
	#define ULONG u32

	typedef s8		s1Byte;
	typedef s8*		ps1Byte;
	
	typedef u8		u1Byte;
	typedef u8*		pu1Byte;
	typedef u16		u2Byte;
	typedef u16*	pu2Byte;
	typedef u32		u4Byte;
	typedef u32*	pu4Byte;
#endif
	
	typedef 	__kernel_size_t	SIZE_T;
	
#define FIELD_OFFSET(s,field)	((int)&((s*)(0))->field)
	
#endif


#define MEM_ALIGNMENT_OFFSET	(sizeof (SIZE_T))
#define MEM_ALIGNMENT_PADDING	(sizeof(SIZE_T) - 1)




#endif //__BASIC_TYPES_H__
