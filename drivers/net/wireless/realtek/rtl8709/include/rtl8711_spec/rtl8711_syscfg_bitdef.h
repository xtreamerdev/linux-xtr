#ifndef __RTL8711_SYSCFG_BITDEF_H__
#define __RTL8711_SYSCFG_BITDEF_H__

//SYSCLKR
#define _AFEPLLEN			BIT(9)
#define _RFPLLEN			BIT(8)
#define _BUSPLLEN			BIT(7)
#define _LXBUS0S			BIT(6)
#define _VCODISABLE			BIT(4)
#define _LXBUS1S			BIT(2)
#define _HSSEL				BIT(1)
#define _HWMASK				BIT(0)

//SYSCTRL
#define _AFERST				BIT(11)
#define _RFRST				BIT(10)
#define _WLANRST			BIT(8)
#define _FCRST				BIT(7)
#define _SCRST				BIT(6)
#define _UARTRST			BIT(5)
#define _HCIRXEN			BIT(1)
#define _CPURST				BIT(0)


//CR9346
#define _EEM_MSK			0xC0
#define _EEM_SHT			6
#define _STATUS_MSK			0x30
#define _STATUS_SHT			4
#define _EECS				BIT(3)
#define _EESK				BIT(2)
#define _EEDI				BIT(1)
#define _EEDO				BIT(0)

//SYSCMD
#define _MACSLP				BIT(1)
#define _BTCEEN				BIT(0)

//HRWFFSW
#define _SWEN				BIT(0)

//HWSEM
#define _FLOCK				BIT(1)
#define _HLOCK				BIT(0)

//H2CR
#define _HOWN				BIT(0)

//C2HR
#define _COWN				BIT(0)

//TKNR1
#define _HCITKN_MSK			0xFF000000
#define _HCITKN_SHT			24
#define _BDTKN_MSK			0xFF0000
#define _BDTKN_SHT			16

//BBCLKENABLE
#define _BBEN				BIT(0)

//MACCLKENABLE
#define _MACEN				BIT(0)

//USB_HFFR
#define _HWFF1_LEN_MSK		0x1FF80000
#define _HWFF1_LEN_SHT		19
#define _HRFF1_LEN_MSK		0x7F700
#define _HRFF1_LEN_SHT		9

//TXCMD_FWCLPTR
#define _BMWLOCK			BIT(27)
#define _MGWLOCK			BIT(23)
#define _TSWLOCK			BIT(19)
#define _BKWLOCK			BIT(15)
#define _BEWLOCK			BIT(11)
#define _VIWLOCK			BIT(7)
#define _VOWLOCK			BIT(3)

#define _BMPTR_MSK			0x7000000
#define _BMPTR_SHT			24
#define _MGTPTR_MSK			0x700000
#define _MGTPTR_SHT			20
#define _TSPTR_MSK			0x70000
#define _TSPTR_SHT			16
#define _BKPTR_MSK			0x7000
#define _BKPTR_SHT			12
#define _BEPTR_MSK			0x700
#define _BEPTR_SHT			8
#define _VIPTR_MSK			0x70
#define _VIPTR_SHT			4
#define _VOPTR_MSK			0x7
#define _VOPTR_SHT			0

#endif	//__RTL8711_SYSCFG_BITDEF_H__

