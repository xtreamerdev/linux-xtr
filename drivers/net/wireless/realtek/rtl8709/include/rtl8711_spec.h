	


#ifndef __RTL8711_SPEC_H__
#define __RTL8711_SPEC_H__
#ifdef CONFIG_RTL8711
#include <drv_conf.h>

#define Virtual2Physical(x)		(((int)x) & 0x1fffffff)
#define Physical2Virtual(x)		(((int)x) | 0x80000000)
#define Virtual2NonCache(x)		(((int)x) | 0x20000000)
#define Physical2NonCache(x)		(((int)x) | 0xa0000000)

#define rtl_inb(offset) (*(volatile unsigned char *)(offset))
#define rtl_inw(offset) (*(volatile unsigned short *)(offset))
#define rtl_inl(offset) (*(volatile unsigned long *)(offset))

#define rtl_outb(offset,val)	(*(volatile unsigned char *)(offset) = (val))
#define rtl_outw(offset,val)	(*(volatile unsigned short *)(offset) = (val))
#define rtl_outl(offset,val)	(*(volatile unsigned long *)(offset) = (val))


#ifndef BIT
#define BIT(x)	( 1 << (x))
#endif

#define SUCCESS	0
#define FAIL	(-1)


#ifdef CONFIG_RTL8711FW
	#define RTL8711_IOBASE		0xBD000000
#else
	#define RTL8711_IOBASE		0x1D000000
#endif

#define RTL8711_MCTRL_		(RTL8711_IOBASE + 0x20000)
#define RTL8711_UART_		(RTL8711_IOBASE + 0x30000)
#define RTL8711_TIMER_		(RTL8711_IOBASE + 0x40000)
#define RTL8711_FINT_		(RTL8711_IOBASE + 0x50000)
#define RTL8711_HINT_		(RTL8711_IOBASE + 0x50000)
#define RTL8711_GPIO_		(RTL8711_IOBASE + 0x60000)
#define RTL8711_WLANCTRL_	(RTL8711_IOBASE + 0x200000)
#define RTL8711_WLANFF_		(RTL8711_IOBASE + 0xe00000)
#define RTL8711_HCICTRL_	(RTL8711_IOBASE + 0x600000)
#define RTL8711_SYSCFG_		(RTL8711_IOBASE + 0x620000)
#define RTL8711_SYSCTRL_	(RTL8711_IOBASE + 0x620000)
#define RTL8711_MCCTRL_		(RTL8711_IOBASE + 0x020000)


//---------

#define OFFSET_WLAN		0x3FFF
#define OFFSET_HCI			0x3FFF
#define OFFSET_INT			0x3FFF
#define OFFSET_CFG			0x3FFF


//Bit 16 15 14
#define DID_WLAN_FIFO		1	//    0  0  1
#define DID_WLAN_CTRL		0	//    0  0  0
#define DID_HCI				2	//    0  1  0
#define DID_SRAM			3	//    0  1  1
#define DID_SYSCFG			4   //    1  0  0
#define DID_INT				6	//    1  1  0
#define DID_UNDEFINE		7   //    1  1  1

#include <rtl8711_regdef.h>

#include <rtl8711_bitdef.h>

#include <basic_types.h>

#endif
#endif // __RTL8711_SPEC_H__

