#ifndef __RTL8711_CFPLUS_REGDEF_H__
#define __RTL8711_CFPLUS_REGDEF_H__


/*---------------------------------------------------------------------------

			Attribute Memory Address Space
									
---------------------------------------------------------------------------*/

//IO BASE + offset
#define COR  			0x0000 //Configuration Option Register (BASE + 0x00)
#define	CCSR 		0x0002 //Card Configuration and Status Register (BASE + 0x02)
#define IOBASE0 		0x0010 //IO BASE 0 Register: 0x0 means IO address checking is disabled (BASE + 0x10)
#define	IOBASE1	0x0012 //IO BASE 1 Register: 0x0 means IO address checking is disabled (BASE + 0x12)
#define IOLIMIT		0x0018 //IO Limit Register: 0x0 means IO address checking is disabled (BASE + 0x18)


/*---------------------------------------------------------------------------

				     IO Registers
									
---------------------------------------------------------------------------*/

//For Register R/W (not including RTL8711_HCICTRL_)
#define HAAR			0x0000 //Host Accessing Address Register
#define HRADR			0x0004 //Host Read Accessing Data Register
#define HWADR 			0x0008 //Host Write Accessing Data Register

#define HWFF0DR		(RTL8711_HCICTRL_ + 0x0010) //Host Write FIFO0 Data Register
#define HRFF0DR 		(RTL8711_HCICTRL_ + 0x0014) //Host Read FIFO0 Data Register
#define TCPTXSZ		(RTL8711_HCICTRL_ + 0x0030) //TCP/IP Offload TX Size
#define TCPWFFDR	(RTL8711_HCICTRL_ + 0x0034) //TCP/IP Offload Write FIFO Data Register
#define TCPRFFDR	(RTL8711_HCICTRL_ + 0x0038) //TCP/IP Offload Read FIFO Data Register
	
	

#endif // __RTL8711_CFPLUS_REGDEF_H__

