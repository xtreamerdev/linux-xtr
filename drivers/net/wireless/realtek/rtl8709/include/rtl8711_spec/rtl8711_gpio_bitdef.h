#ifndef __RTL8711_GPIO_BITDEF_H__
#define __RTL8711_GPIO_BITDEF_H__

// PADIR/PBDIR: Port A/B Direction Register
#define _DRCA_MSK			0x00FF0000
#define _DRCA_SHITF			16

// PADAT/PBDAT: Port A/B Data Register
#define _PDA_MSK			0x00FF0000
#define _PDA_SHITF			16		

// PAISR/PBISR: Port A/B Interrupt Status Register
#define _PAIP_MSK			0x000000FF
#define _PAIP_SHITF			0		

// PAIMR/PBIMR: Port A/B Interrupt Mask Register
#define _PA0IM_MSK			0x0000C000
#define _PA1IM_MSK			0x00003000
#define _PA2IM_MSK			0x00000C00
#define _PA3IM_MSK			0x00000300
#define _PA4IM_MSK			0x000000C0
#define _PA5IM_MSK			0x00000030
#define _PA6IM_MSK			0x0000000C
#define _PA7IM_MSK			0x00000003
#define _PA0IM_SHIFT		14
#define _PA1IM_SHIFT		12
#define _PA2IM_SHIFT		10
#define _PA3IM_SHIFT		8
#define _PA4IM_SHIFT		6
#define _PA5IM_SHIFT		4
#define _PA6IM_SHIFT		2
#define _PA7IM_SHIFT		0



#endif	//__RTL8711_GPIO_BITDEF_H__

