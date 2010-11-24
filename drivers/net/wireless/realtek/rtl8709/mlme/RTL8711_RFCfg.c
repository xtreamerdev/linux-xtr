/*

	RTL8711 RF Config

*/


#define _RTL8711_RFCFG_C_

#include <drv_types.h>
#include <wlan.h>
#include <hal_init.h>



ULONG   RF_CHANNEL_TABLE_RTL8225[]={
#ifndef	CONFIG_RF_NATIVE	
	0,
	0x085c, //2412 channel 1
	0x08dc, //2417 channel 2
	0x95c,  //2422 channel 3
	0x9dc,  //2427 channel 4
	0xa5c,    //2432 channel 5
	0x0adc, //2437 channel 6
	0x0b5c, //2442 channel 7
	0x0bdc, //2447 channel 8
	0x0c5c, //2452 channel 9
	0x0cdc, //2457 channel 10
	0x0d5c, //2462 channel 11
	0x0ddc, //2467 channel 12
	0x0e5c, //2472 channel 13
	0x0f72, //2484MHz channel 14
#else
	0,
	0x004,	//channel_1
	0x008, 	//channel_2
	0x00C, 	//channel_3
	0x010, 	//channel_4
	0x014, 	//channel_5
	0x018, 	//channel_6
	0x01C, 	//channel_7
	0x020, 	//channel_8
	0x024, 	//channel_9
	0x028, 	//channel_10
	0x02C, 	//channel_11
	0x030, 	//channel_12
	0x034, 	//channel_13
	0x038,	//channel_14
#endif
};

void SwChnl( _adapter *padapter, UCHAR channel)
{
	int i;
	unsigned short	tx_power;
	struct registry_priv *pregpriv = &(padapter->registrypriv);
	struct mib_info *pmibinfo = &(padapter->_sys_mib);
		
	i = RF_CHANNEL_TABLE_RTL8225[channel];
	DEBUG_INFO(("ch val:%x\n", i));

	if (pregpriv->chip_version != RTL8711_FPGA)
	{
		
#ifdef CONFIG_POWERTRACKING

#ifdef THERMAL_METHOD
		thermal_meter = initial_thermal_meter;
	
		tx_power = pmibinfo->cur_class->channel_cck_power[channel -1] << 8 | pmibinfo->cur_class->channel_ofdm_power[channel - 1];
#else
	
		tx_power = ((unsigned char)(pmibinfo->cur_class->channel_cck_power[pmibinfo->cur_channel -1] + delta_of_tx_pwr) << 8) | 
				   ((unsigned char)(pmibinfo->cur_class->channel_ofdm_power[pmibinfo->cur_channel - 1] + delta_of_tx_pwr));
	
#endif	
	
#else	

		tx_power = pmibinfo->cur_class->channel_cck_power[channel -1] << 8 | pmibinfo->cur_class->channel_ofdm_power[channel - 1];

#endif
		
	}

}

void SelectChannel(_adapter *padapter, UCHAR channel)
{
	DEBUG_INFO(("###+SelectChannel\n"));

	#ifdef CONFIG_RTL8192U

		rtl8256_rf_set_chan(padapter, channel);	
	
		mdelay_os(10);
		
	#elif defined(CONFIG_RTL8187)

		rtl8225z2_rf_set_chan(padapter, channel);	

		mdelay_os(10);
	#else
	SwChnl(channel,adapter);
	#endif
	
	DEBUG_INFO(("###-SelectChannel\n"));
}


#if 0
#include <rtl8711_spec.h>
#include <osdep_service.h>
#include <wlan_bssdef.h>
#include <arch/byteorder.h>
#include <console.h>
#include <rtl8711_cmd.h>
#include <rtl8711_rf.h>
#include <wireless.h>
#include <wlan.h>
//#include <delay.h>
#include <sched.h>
#include <ieee80211.h>
#include <system32.h>

__init_data__ struct regulatory_class init_class_set[NUM_REGULATORYS] = 
{	
	{
		starting_freq: 2412,
		channel_set: {1,2,3,4,5,6,7,8,9,10,11},
        channel_cck_power: {18,18,18,18,18,18,18,18,18,18,18},
        channel_ofdm_power:{15,15,15,15,15,15,15,15,15,15,15},
        txpower_limit: 23,
        channel_spacing:5,
        modem: IEEE80211_CCK_MODULATION | IEEE80211_OFDM_MODULATION
	},
};

//typedef	unsigned long	ULONG;
//typedef unsigned short	UShort;
//typedef unsigned short	USHORT;
//typedef unsigned char	UCHAR;

struct  _RFRegStruc{

#ifdef __BIG_ENDIAN_BITFIELD
        UCHAR   Poll:1;
        UCHAR   ThreeWireMode:1;
        UCHAR   RFType:2;
        UCHAR   Address:4;
        UCHAR   DataHigh;
        USHORT  DataLow;
#else
        USHORT  DataLow;
        UCHAR   DataHigh;
        UCHAR   Address:4;
        UCHAR   RFType:2;
        UCHAR   ThreeWireMode:1;
        UCHAR   Poll:1;
#endif
};

typedef union   _RFReg{
	struct  _RFRegStruc struc;
	ULONG   longData;
}RFReg;

struct  _ThreeWireStruc{
#ifdef __BIG_ENDIAN_BITFIELD
        USHORT  resv1:12;
        USHORT  read_write:1;
        USHORT  enableB:1;
        USHORT  clk:1;
        USHORT  data:1;
#else
        USHORT  data:1;
        USHORT  clk:1;
        USHORT  enableB:1;
        USHORT  read_write:1;
        USHORT  resv1:12;
#endif
};

typedef	union	_ThreeWire{
	struct	_ThreeWireStruc struc;
	USHORT longData;
}ThreeWireReg;



#define WritePortUlong(offset, value)	rtl_outl((offset), value)
#define WritePortUshort(offset, value)	rtl_outw((offset), value)
#define WritePortUchar(offset, value)	rtl_outb((offset), value)

#define ReadPortUlong(offset)			(rtl_inl((offset)))
#define ReadPortUshort(offset)			(rtl_inw((offset)))
#define ReadPortUchar(offset)			(rtl_inb((offset)))

//#define delay(x)	mdelay(x)


//#define RFINTFO         (RTL8711_WLANCTRL_ + 0x256)
//#define RFINTFE         (RTL8711_WLANCTRL_ + 0x258)
//#define RFINTFS         (RTL8711_WLANCTRL_ + 0x25C)
//#define RFINTFI         (RTL8711_WLANCTRL_ + 0x25A)

struct	rfcmd	{
	//ULONG	rc_addr;
	//ULONG	rc_data;
	USHORT	rc_addr;
	USHORT	rc_data;
};


// initial for PLL

#include "AGC.h"

#if 0
// now the cck config is omitted for the default value is enough
#include "CCKCONF.h"
#endif

#include "OFDMCONF.h"
#include "RADIOCFG.h"

void RTL8711RF_Cfg(UCHAR channel);
void RTL8225_Config(void);
void OFDM_Config(void);
void CCK_Config(void);
void RFSerialWrite(ULONG, UCHAR, UCHAR);
void bb_write(UCHAR, UCHAR);
UCHAR bb_read(UCHAR);
void dump_phy_tables(UCHAR);


void 	RF_HWSI_WRITE(UCHAR offset, ULONG data);
ULONG 	RF_HWSI_READ(UCHAR offset);
	
void 	RF_HWPI_WRITE(UCHAR offset, ULONG data);
ULONG 	RF_HWPI_READ(UCHAR offset);

	
void RF_WriteReg(UCHAR, UCHAR, ULONG);

unsigned long cpu_speed;
extern volatile unsigned char	chip_version;

#ifdef CONFIG_POWERTRACKING
extern unsigned int thermal_meter;
extern unsigned int initial_thermal_meter;

#ifdef CONFIG_TSSI_METHOD
extern char	delta_of_tx_pwr;
#endif

#endif

volatile unsigned char tuning_cap_g = 3;

void set_cpu_rate(unsigned rate)
{
	unsigned long	flags;
	
	
	local_irq_save(flags);	
	
	cpu_speed = rate;

	local_irq_restore(flags);
}

//milli second delay
void delay(unsigned int ms)
{

#ifndef CONFIG_SC_ENV
	
	register unsigned int	cnt;

	switch (cpu_speed) {
		
		case CPU60MHZ:
			cnt = ms * 	CPU60MHZ_JIFFY;
			break;
		case CPU40MHZ:
			cnt = ms * 	CPU40MHZ_JIFFY;
			break;
		case CPU20MHZ:
			cnt = ms * 	CPU20MHZ_JIFFY;
			break;
		case CPU32KHZ:
			cnt = ms * 	CPU32KHZ_JIFFY;
			break;
		default:
			printk("Invalid CPU setting(%d)\n",cpu_speed);
			cnt = ms * 	CPU60MHZ_JIFFY;
	}

	while(--cnt);
#endif
}

void bb_write(UCHAR offset , UCHAR val)
{
	
	unsigned int	tmp;
	
	tmp = (0x01000000) | (offset) | (val << 8);
	
	rtl_outl(PhyAddr, tmp);
	
	
}

UCHAR bb_read(UCHAR offset)
{
	unsigned int	tmp;
	
	tmp = (unsigned int)offset;
	
	rtl_outl(PhyAddr, tmp);
	
	return (rtl_inb(PhyDataR));
	
}

volatile unsigned char rfintfs = SWSI;

void RTL8711RF_Cfg(UCHAR channel)
{
	if (rfintfs == SWSI) {
		printk("Config RF Interface to SW-3Wire \n");
		rtl_outb(RFINTFS, rtl_inb(RFINTFS) |  0x0F );
		//sunghau@0320: update the value
		rtl_outb(HSSIPAR + 4, 0xd2);
	}
	else {
		printk("Config RF Interface to HW-3Wire \n");

		rtl_outb(RFINTFS, rtl_inb(RFINTFS) & 0xF0 );

		if (rfintfs == HWSI) {
			printk("Config RF Interface to SI \n");
			//sunghau@0320: update the value
			rtl_outb(HSSIPAR + 4, 0xd2);

		} 
		else {
			printk("Config RF Interface to PI \n");
			//sunghau@0320: update the value
			rtl_outb(HSSIPAR + 4, 0xd0);
		}
	}

	switch (chip_version)
	{
		case RTL8711_3rdCUT:
		case RTL8711_2ndCUT:
			rtl_outl(HSSIPAR, 0x06184028);
			break;
			
		case RTL8711_1stCUT:
			rtl_outw(HSSIPAR, 0x4000);
			break;
	
		case RTL8711_FPGA:
		default:	
			rtl_outw(HSSIPAR, 0x0000);
			break;
	}
	
#ifndef CONFIG_RF_NATIVE
 
	RTL8225_Config();
	
	OFDM_Config();
	
	CCK_Config();
	
	SelectChannel(channel);

//	rtl_outb(0xbd200278, 0xd2);	//by scott 1127: move before RTL8225_Config() to select RFINTF type
	
#if 0	
	rtl_outl(PhyAddr, 0x01000302);

	rtl_outl(PhyAddr, 0x01000003);	//use Rssi GainTbl 0
	
	rtl_outl(PhyAddr, 0x01004617);
	
	rtl_outl(PhyAddr, 0x01003c39);
#endif	
	
	rtl_outl(RFSW_CTRL, 0x9A56);
	
	//rtl_outb(0xbd200245, 0x50);	//turn off CCK and turn on OFDM
	//rtl_outb(0xbd200245, 0x60);	//turn off CCK and OFDM	
	
	rtl_outb(PHYPARM, 0x28);	//enable ed_cca_mask
	
	rtl_outb(TX_AGC_ANT_CONTROL, 0x10);	
#else

	RTL8225_Config();

	OFDM_Config();
	
	CCK_Config();
	
	SelectChannel(channel);

#if 0
	rtl_outl(PhyAddr, 0x01000302);

	rtl_outl(PhyAddr, 0x01000003);	//use Rssi GainTbl 0
	
	rtl_outl(PhyAddr, 0x01004617);
	
	rtl_outl(PhyAddr, 0x0100bc39);
#endif
	
	rtl_outb(PHYPARM, 0x28);	//enable ed_cca_mask
	
	rtl_outb(TX_AGC_ANT_CONTROL, 0x10);	

#endif	

}


void RTL8225_Config(void)
{
	//setup for reading RF configuration
	// configuration for Zebra
	// csm 0116

	//ULONG	rc_data;
	
	int		i;
	unsigned long tmp;
	
	// Zebra C cut initialization procedure
	WritePortUshort(RFINTFO, 0x480);
	WritePortUshort(RFINTFE, 0x1bff);
	WritePortUshort(RFINTFS, 0x488);
	#if 0	//jackson 0621
	WritePortUchar(0x90, 0x0);
	WritePortUchar(0x91, 0x0);
	#endif 
	//delay(200);  //jackson: can be removed by the suggestion of Steven Lin

	i = 0;
	
	
	while (1)
	{
		if (radio_cmdarray[i].rc_addr == 0xff)
			break;
			
		if (radio_cmdarray[i].rc_addr == 0xfe)
			delay(100);		
		else if (radio_cmdarray[i].rc_addr == 0xfd)
			delay(200);
		else
			if (radio_cmdarray[i].rc_addr == 0xfc)	
			{
				//add by scott
				//printk("RF Calibration Duration 500ms !!");
				delay(500);	//RF calibration requires 500ms
			}
		else {
			RF_WriteReg((UCHAR)radio_cmdarray[i].rc_addr, 16, (ULONG)radio_cmdarray[i].rc_data);
			//printk("RF_WriteReg() with offset 0x%x, and value = 0x%x \n", radio_cmdarray[i].rc_addr, radio_cmdarray[i].rc_data);
		}
		
		i++;
	}
	
	//for tuning cap
	if (chip_version >= RTL8711_3rdCUT)
	{
		RF_WriteReg(0x0, 16, 0x08b);	// change RF page to page 3
		tmp = ((RF_ReadReg(0x01) & 0xffffffe0) | (tuning_cap_g & 0x1f));
		RF_WriteReg(0x1, 16, tmp);
		RF_WriteReg(0x0, 16, 0x088);
	}
	else
	{
		RF_WriteReg(0x0, 16, 0x08b);	// change RF page to page 3
		RF_WriteReg(0x1, 16, 0xa00);
		RF_WriteReg(0x0, 16, 0x088);
	}
	
	// set TX timing
	switch (chip_version)
	{
		case RTL8711_3rdCUT:
		case RTL8711_2ndCUT:
			WritePortUlong(RF_TIMING, 0x00008005);
			break;
			
		case RTL8711_1stCUT:
			WritePortUlong(RF_TIMING, 0x0000A008);
			break;
			
		case RTL8711_FPGA:
		default:
			WritePortUlong(RF_TIMING, 0x00084008);
			break;
			
	}
		
	//marc: for chiamin request @ 0807
	if (chip_version != RTL8711_1stCUT)
	{
		WritePortUlong(PHYMUX, 0x15c00108);
	}
	else
	{
		WritePortUlong(PHYMUX, 0x15c00008);
	}
	//printk("PHYMUX:%x\n", rtl_inl(PHYMUX));
	
	//jackson 0621 RF_PARA only takes 2 bytes long
	//WritePortUlong(RF_PARA, 0x20000044);
	if (chip_version != RTL8711_FPGA)
#ifdef CONFIG_HIGH_POWER
		WritePortUshort(RF_PARA, 0x0094);
#else
		WritePortUshort(RF_PARA, 0x0084);
#endif
	else
		WritePortUshort(RF_PARA, 0x0044);

	//WritePortUchar(0x50, 0xc0);	// no need for RTL8711 when writing configuration
	WritePortUchar(CONFIG_REG0, ((ANA_WE) | ReadPortUchar(CONFIG_REG0)));
	
	WritePortUlong(ANA_PARM1, 0x860dec11);
	
	//WritePortUlong(0x54, 0xa0000b59);
	WritePortUlong(ANA_PARM0, 0xa0000b59);
	
	
	//jackson 0621
	//WritePortUchar((RF_PARA+2), 0x10);

	// power control //moved to PWRBASE by scott 2007/04/01
//	WritePortUchar(CCK_TX_AGC, 0x03); // for C cut register shift one bit
//	WritePortUchar(OFDM_TX_AGC, 0x07); // correction 0709 PJ
	WritePortUchar(ANTSEL, 0x03);

	// setup baseband parameter
	//WritePortUlong(0x7c, 0x0000049c);
	
	
	i = 0;
#if 0
	while (1)
	{
		if (cck_cmdarray[i].rc_addr == 0xff)
			break;
		// 0x0100080   0x01: accessing to CCK PHY, 0x80: Write Enable		
		rc_data = (cck_cmdarray[i].rc_data << 8) | (0x01000080 + cck_cmdarray[i].rc_addr);
		WritePortUlong(PhyAddr, (rc_data));
		delay(1);
		
		i++;

	}
#endif

}



void OFDM_Config(void)
{

	ULONG i;
	ULONG addr,data;
	UCHAR offset;
	

//#ifndef CONFIG_RF_NATIVE	//by scott for RF-only Test 
	rtl_outl(PhyAddr, 0x01001200);
	for (i=0; i<128; i++)
	{
		addr = (ULONG)agc_cmdarray[i].rc_addr;
		data = (ULONG)agc_cmdarray[i].rc_data;	
		data = data << 8;
		data = data | 0x0100000F;
		WritePortUlong(PhyAddr, data);

		addr = addr + (ULONG)0x80; // add 0x80 for positive edge trigger write_enable (bit7)
		addr = addr << 8;
		addr = addr | 0x0100000E;

		WritePortUlong(PhyAddr, addr);

		WritePortUlong(PhyAddr,0x0100000e); // pull down write_enable (bit7)
		//delay(1);

	}
	rtl_outl(PhyAddr, 0x01001000);
//#endif
	addr = 0x01000000;
	i = 0;
	while (ofdm_cmdarray[i].rc_addr != 0xff)
	{
		data = (ULONG)(ofdm_cmdarray[i].rc_data << 8);
		offset = (ULONG)ofdm_cmdarray[i].rc_addr;
		//printk("In OFDM_Config(), OFDM_PHY_Init Offset: %x ---> %x \n", offset, data);
		
		data = data | (addr + (ULONG)offset);
		
		WritePortUlong(PhyAddr, data);
		//delay(10);		//Steven suggest that 10ms can be reduced to 1ms.
		i++;
	}



}


void CCK_Config()
{
	switch (chip_version)
	{
		case RTL8711_3rdCUT:
		case RTL8711_2ndCUT:
			rtl_outl(PhyAddr, 0x0100d882);
			break;
			
		default:
			break;

	}

//#ifdef CONFIG_HIGH_POWER
	WritePortUlong(PhyAddr, 0x0100728E);
//#endif
}


void
RFSerialWrite(ULONG	data2Write, UCHAR totalLength, UCHAR low2high)
{
	
	ThreeWireReg	twreg;
	int				i;
	USHORT                   oval,oval2,oval3;
	USHORT                   test;
	ULONG			mask;

	oval	=	ReadPortUshort(RFINTFO);
	oval2	=	ReadPortUshort(RFINTFE);
	oval3	=	ReadPortUshort(RFINTFS);
	WritePortUshort(RFINTFE, oval2|0x000F);   // Set To Output Enable
	WritePortUshort(RFINTFS, oval3|0x000F);   // Set To SW Switch

	twreg.struc.enableB = 1;
	WritePortUshort(RFINTFO, twreg.longData|oval); 

	//jackson 0621
	//delay(4);

	for(i = 0; i< 0x5; i++);
	twreg.longData = 0;
	twreg.struc.enableB = 0;
	twreg.struc.read_write = 0;
	WritePortUshort(RFINTFO, twreg.longData|oval); 
	//delay(4);



	mask = (low2high)?0x01:((ULONG)0x01<<(totalLength-1));
	for(i=0; i< ( totalLength >> 1); i++)
	{
		twreg.struc.data = ((data2Write&mask)!=0) ? 1 : 0;
		WritePortUshort(RFINTFO, test=twreg.longData|oval);
		//delay(4);
		
		twreg.struc.clk = 1;
		WritePortUshort(RFINTFO, test=twreg.longData|oval);
		//delay(4);
		WritePortUshort(RFINTFO, test=twreg.longData|oval);
		//delay(4);

		mask = (low2high)?(mask<<1):(mask>>1);
		twreg.struc.data = ((data2Write&mask)!=0) ? 1 : 0;
		WritePortUshort(RFINTFO, test=twreg.longData|oval);
		//delay(4);
		
		WritePortUshort(RFINTFO, test=twreg.longData|oval);
		//delay(1);
		
		twreg.struc.clk = 0;
		WritePortUshort(RFINTFO, test=twreg.longData|oval);
		//delay(1);
		
		mask = (low2high)?(mask<<1):(mask>>1);
	}

	twreg.struc.enableB = 1;
	twreg.struc.clk = 0;
	twreg.struc.data = 0;
	twreg.struc.read_write = 1;
	WritePortUshort(RFINTFO, test=twreg.longData|oval);
	//delay(1);


	WritePortUshort(RFINTFE, oval2|0x8);   // Set To Output Enable
	WritePortUshort(RFINTFS, oval3|0x488);   // Set To SW Switch
	WritePortUshort(RFINTFO, 0x480);

}

#define		wait(x)	do { unsigned int i; for(i = 0; i < x; i++); } while(0)

void
RFSerialRead(
unsigned long                   data2Write,
unsigned char                   wLength,
unsigned long                   *data2Read,
unsigned char                   rLength,
unsigned char                   low2high)
{
	ThreeWireReg    twreg;
	unsigned int		i;
	USHORT          oval,oval2,oval3;
	ULONG           mask;
	ThreeWireReg    tdata;

	oval = ReadPortUshort(RFINTFO);
	oval2 = ReadPortUshort(RFINTFE);
	oval3 = ReadPortUshort(RFINTFS);

	WritePortUshort(RFINTFE, oval2 | 0xf);
	WritePortUshort(RFINTFS, oval3 | 0xf);

	*data2Read = 0;
        
	// avoid collision with hardware three-wire
	twreg.struc.enableB = 1;
	WritePortUshort(RFINTFO, twreg.longData | oval);
	wait(4);
	// PJ 6/10/04

	twreg.longData = 0;
	twreg.struc.enableB = 0;
	twreg.struc.clk = 0;
	twreg.struc.read_write = 1; //marc: for 8711RF and it works for z2, too
	WritePortUshort(RFINTFO, twreg.longData | oval);
	wait(5);

	mask = (low2high)?0x01:((ULONG)0x01<<(32-1));
	for(i = 0; i < wLength / 2; i++)
	{
		twreg.struc.data = ((data2Write&mask) != 0)? 1: 0;
		WritePortUshort(RFINTFO, twreg.longData|oval);
		wait(1);
		
		twreg.struc.clk = 1;
		WritePortUshort(RFINTFO, twreg.longData|oval);
		wait(2);

		WritePortUshort(RFINTFO, twreg.longData|oval);
		wait(2);

		if (i == 2)
		{
			WritePortUshort(RFINTFE, 0x8e);     // turn off data enable
			WritePortUshort(RFINTFS, 0x8e);

			twreg.struc.read_write = 1;
			WritePortUshort(RFINTFO, twreg.longData|oval);   //NdisStallExecution(2);
			wait(2);
			
			twreg.struc.clk = 0;
			WritePortUshort(RFINTFO, twreg.longData|oval);   //NdisStallExecution(2);
			wait(2);
			break;
		}

		mask = (low2high)?(mask<<1):(mask>>1);
		twreg.struc.data = ((data2Write&mask)!= 0)? 1: 0;
		WritePortUshort(RFINTFO, twreg.longData|oval);
		wait(2);
		
		WritePortUshort(RFINTFO, twreg.longData|oval);
		wait(2);

		twreg.struc.clk = 0;
		WritePortUshort(RFINTFO, twreg.longData|oval);
		wait(2);

		mask = (low2high)? (mask << 1): (mask >> 1);
	}

	twreg.struc.clk = 0;
	twreg.struc.data = 0;
	WritePortUshort(RFINTFO, twreg.longData|oval);
	wait(2);
	
	mask = (low2high)? 0x01: ((ULONG)0x01<<(12-1));
	for(i = 0; i < rLength; i++)
	{
		WritePortUshort(RFINTFO, twreg.longData|oval);   
		wait(2);
		
		twreg.struc.clk = 1;
		WritePortUshort(RFINTFO, twreg.longData|oval);    
		wait(2);
		
		WritePortUshort(RFINTFO, twreg.longData|oval);   
		wait(2);
		
		WritePortUshort(RFINTFO, twreg.longData|oval);   
		wait(2);

		tdata.longData = ReadPortUshort(RFINTFI);
		*data2Read |= tdata.struc.clk?mask:0;

		twreg.struc.clk = 0;
		WritePortUshort(RFINTFO, twreg.longData|oval);  
		wait(2);

		mask = (low2high)?(mask<<1):(mask>>1);
	}
        
	twreg.struc.enableB = 1;
	twreg.struc.clk = 0;
	twreg.struc.data = 0;
	twreg.struc.read_write = 1;
	WritePortUshort(RFINTFO, twreg.longData|oval);     
	wait(2);

	WritePortUshort(RFINTFE, oval2|0x8);   // Set To Output Enable
	WritePortUshort(RFINTFS, oval3|0x488);   // Set To SW Switch
	WritePortUshort(RFINTFO, 0x480);
}

unsigned long RF_SWSI_READ(unsigned char offset)
{
	ULONG   data2Write;
	//UCHAR   len;
	UCHAR   low2high;
	//ULONG   RF_Read = TRUE;
	ULONG   dataRead;
	UCHAR   wlen;
	UCHAR   rlen;

	//todo:if version before E, return false
	{
		data2Write = ((ULONG)(offset&0x0f)) << 27;
		wlen=6;
		rlen=12;
		low2high=FALSE;
		RFSerialRead(data2Write, wlen, &dataRead, rlen, low2high);
	}

	return dataRead;
}

void RF_HWSI_WRITE(UCHAR offset, ULONG data)
{
	int i;
	
	//marc@0713
	rtl_outw(SOFT3WIRE_DATA, (data << 4 ) | offset);
	rtl_outb(SOFT3WIRE_CTRL, BIT(1));
	
	for(i = 0; i <4; i++)
	{
		if (rtl_inb(SOFT3WIRE_CTRL) & BIT(2))
			break;
	}
	
	rtl_outb(SOFT3WIRE_CTRL, 0);
	
	if (i == 4)
		printk("RF_HWSI_WRITE for offset:%d, val:%x failed!\n", offset, data);
	
}


ULONG 	RF_HWSI_READ(UCHAR offset) 
{
	
	int i;
	unsigned short	val;
	
	//marc@0713
	rtl_outw(SOFT3WIRE_DATA, offset);
	rtl_outw(RFINTFS, (rtl_inw(RFINTFS) & ~BIT(3)));
	rtl_outb(SOFT3WIRE_CTRL, BIT(0));
	
	for(i = 0; i <4; i++)
	{
		if (rtl_inb(SOFT3WIRE_CTRL) & BIT(2))
			break;
	}
	
	rtl_outw(RFINTFS, (rtl_inw(RFINTFS) & BIT(3)));
	
	//marc@0713
	val = rtl_inw(SOFT3WIRE_SI_DATA);
	
	rtl_outb(SOFT3WIRE_CTRL, 0);
	
	if (i == 4)
		printk("RF_HWSI_READ for offset:%d failed!\n", offset);
		
	return ((ULONG)val);	
	
}
	

void RF_HWPI_WRITE(UCHAR offset, ULONG data)
{
	int i;
	//printk("RF_HWPI_WRITE for offset:%d, val:%x !!\n", offset, data);
	rtl_outw(SOFT3WIRE_DATA, (data << 4 ) | offset);
	rtl_outb(SOFT3WIRE_CTRL, BIT(1));
	
	for(i = 0; i <4; i++)
	{
		if (rtl_inb(SOFT3WIRE_CTRL) & BIT(2))
			break;
	}
	
	rtl_outb(SOFT3WIRE_CTRL, 0);
	
	if (i == 4)
		printk("RF_HWPI_WRITE for offset:%d, val:%x failed!\n", offset, data);
	
}

ULONG 	RF_HWPI_READ(UCHAR offset) 
{
	int i;
    unsigned short val;
	//printk("RF_HWPI_READ for offset:%d!!! \n", offset);
	rtl_outw(SOFT3WIRE_DATA, (offset<<12) );
	rtl_outb(SOFT3WIRE_CTRL, BIT(0) );
	
	for(i = 0; i <4; i++)
	{	
		if( rtl_inb(SOFT3WIRE_CTRL) & BIT(2) )
			break;
	}
	val = rtl_inw(SOFT3WIRE_PI_DATA);
	rtl_outb(SOFT3WIRE_CTRL, 0);
#if 0	
	printk("RF_HWPI_READ of offset:%d, val:%x !\n", offset, val);
	if (i == 4)
		printk("RF_HWPI_READ of offset:%d, val:%x failed!\n", offset, val);
#endif
	return val;
}

	

void RF_WriteReg(UCHAR offset, UCHAR codelen, ULONG data)
{
	
	ULONG	flags;		
	ULONG	data2Write;
	UCHAR	len;
	UCHAR	low2high;

	local_irq_save(flags);

#if 0
#ifndef CONFIG_HW_3WIRE

	codelen = 16;
	data2Write = (data<<4) | (offset&0x0f);
	len = codelen;
	low2high=0;
	

	RFSerialWrite(data2Write, len, low2high);
#else

	#ifndef CONFIG_PI_RF
		
		RF_HWSI_WRITE(offset, data);
		
	#else
	
		RF_HWPI_WRITE(offset, data);
	
	#endif	


#endif	
#endif


	if (rfintfs == SWSI) {
		codelen = 16;
		data2Write = (data<<4) | (offset&0x0f);
		len = codelen;
		low2high=0;
		//printk("Using SWSI For RF Write\n");
		RFSerialWrite(data2Write, len, low2high);
	}	
	else if (rfintfs == HWSI)
	{
		//printk("Using HWSI For RF Write\n");
		RF_HWSI_WRITE(offset, data);
	}
	else
	{
		//printk("Using HWPI For RF Write\n");
		RF_HWPI_WRITE(offset, data);
	}

	local_irq_restore(flags);
}


ULONG RF_ReadReg(UCHAR offset)
{

	
	ULONG	flags;		
	ULONG	val;	

	local_irq_save(flags);

#if 0
#ifndef CONFIG_HW_3WIRE

	return RF_SWSI_READ(offset);

#else

	#ifndef CONFIG_PI_RF
		
		return RF_HWSI_READ(offset);
		
	#else
	
		return RF_HWPI_READ(offset);
	
	#endif	


#endif	
#endif


	if (rfintfs == SWSI)
	{
		//printk("Using SWSI For RF Read\n");
		val = RF_SWSI_READ(offset);
	}
	else if (rfintfs == HWSI)
	{
		//printk("Using HWSI For RF Read\n");
		val = RF_HWSI_READ(offset);
	}
	else
	{
		//printk("Using HWPI For RF Read\n");
		val = RF_HWPI_READ(offset);
	}


	local_irq_restore(flags);
	
	return val;
}


void dump_phy_tables(UCHAR inx)
{
	int	offset;
	
	bb_write(0x3e, inx<<4);
	printk("---------------- Table contents ---------------\n");
		
	for(offset = 0; offset < 64; offset++)
	{
		bb_write(0x3f, offset);
		printk("offset:%d	val:%d\n",offset, bb_read(0x60));
	}
	
	printk("-------------     Table Ends    ---------------\n");	
	
	
}
 
ULONG   RF_CHANNEL_TABLE_RTL8225[]={
#ifndef	CONFIG_RF_NATIVE	
	0,
	0x085c, //2412 channel 1
	0x08dc, //2417 channel 2
	0x95c,  //2422 channel 3
	0x9dc,  //2427 channel 4
	0xa5c,    //2432 channel 5
	0x0adc, //2437 channel 6
	0x0b5c, //2442 channel 7
	0x0bdc, //2447 channel 8
	0x0c5c, //2452 channel 9
	0x0cdc, //2457 channel 10
	0x0d5c, //2462 channel 11
	0x0ddc, //2467 channel 12
	0x0e5c, //2472 channel 13
	0x0f72, //2484MHz channel 14
#else
	0,
	0x004,	//channel_1
	0x008, 	//channel_2
	0x00C, 	//channel_3
	0x010, 	//channel_4
	0x014, 	//channel_5
	0x018, 	//channel_6
	0x01C, 	//channel_7
	0x020, 	//channel_8
	0x024, 	//channel_9
	0x028, 	//channel_10
	0x02C, 	//channel_11
	0x030, 	//channel_12
	0x034, 	//channel_13
	0x038,	//channel_14
#endif
};


unsigned char def_nettype = WIRELESS_11BG;
//unsigned char def_nettype = WIRELESS_11B;
//unsigned char def_nettype = WIRELESS_11G;

__init_fun__ void rf_init(void)
{
	sys_mib.network_type = def_nettype;
	
	switch (sys_mib.network_type) {
		
		case WIRELESS_11B:
			sys_mib.cur_modem = CCK_PHY;
			sys_mib.cur_channel = 6;
			break;
		case WIRELESS_11A:	
			sys_mib.cur_modem = OFDM_PHY;
			sys_mib.cur_channel = 34;
			break;
		case WIRELESS_11G:
			sys_mib.cur_modem = OFDM_PHY;
			sys_mib.cur_channel = 6;
			break;
		case WIRELESS_11BG:
			sys_mib.cur_modem = MIXED_PHY;
			sys_mib.cur_channel = 6;
			break;
			
	}
	
	sys_mib.cur_channel = 6;

	memcpy((unsigned char *)(sys_mib.class_sets), (unsigned char *)(&init_class_set[0]), sizeof (struct regulatory_class));
	sys_mib.class_cnt = 1;
	sys_mib.cur_class = &(sys_mib.class_sets[0]);	

}

#endif



