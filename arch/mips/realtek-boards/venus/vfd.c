
#define __sleep __attribute__ ((__section__ (".sleep.text")))
#define __sleepdata __attribute__ ((__section__ (".sleep.data")))

#define MIS_VFD_WRCTL   ((volatile unsigned int *)0xb801b704)
#define MIS_VFD_VFDO    ((volatile unsigned int *)0xb801b708)
#define PERIOD_WRITEDONE        100

#include <asm/mach-venus/platform.h>

enum VFD_MODULE_TYPE {
	FUTABA_11BT236,
	FUTABA_11MT141,
	HNV_11MS41T,              //for eln_1262
};

static const char __sleepdata letterTable_11bt236[][14] = {
               {1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1 },  // 0
               {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0 },  // 1
               {1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1 },  // 2
               {1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1 },  // 3
               {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0 },  // 4
               {1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1 },  // 5
               {1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0 },  // 6
               {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1 },  // 7
               {1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1 },  // 8
               {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1 },  // 9
};

static const char __sleepdata letterTable_11mt141[][14] = {
		//	For Futaba 11-MT-141GNK
/*              d  c  e  n  r  p  g  m  j  h  k  f  b  a */
               {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },  // 0
               {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },  // 1
               {1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1 },  // 2
               {1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1 },  // 3
               {0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0 },  // 4
               {1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1 },  // 5
               {1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0 },  // 6
               {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },  // 7
               {1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1 },  // 8
               {0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1 },  // 9
};

static const char __sleepdata letterTable_11ms41t[][14] = {
	//	For HNV-11MS41T
/*     d  c  e  g  f  b  a */
	{1, 1, 1, 0, 1, 1, 1 },  // 0
	{0, 1, 0, 0, 0, 1, 0 },  // 1
	{1, 0, 1, 1, 0, 1, 1 },  // 2
	{1, 1, 0, 1, 0, 1, 1 },  // 3
	{0, 1, 0, 1, 1, 1, 0 },  // 4
	{1, 1, 0, 1, 1, 0, 1 },  // 5
	{1, 1, 1, 1, 1, 0, 0 },  // 6
	{0, 1, 0, 0, 0, 1, 1 },  // 7
	{1, 1, 1, 1, 1, 1, 1 },  // 8
	{0, 1, 0, 1, 1, 1, 1 },  // 9
};
static void __sleep writeAtOnce(unsigned int valueVFDO, unsigned int valueWRCTL) {
	int i;
	*(MIS_VFD_VFDO) = valueVFDO;
	*(MIS_VFD_WRCTL) = valueWRCTL;

	while(((*MIS_VFD_WRCTL) & 0x02) == 0x00) {
		for(i = 0 ; i < 1000 ; i++);
	}
}

static void __sleep nodeOn(unsigned char digit, unsigned char segment, char *displayRAM) {
	unsigned char addr = (digit-1) * 3 + (segment-1) / 8;

	if(displayRAM[addr] == (displayRAM[addr] | ((0x1) << ((segment-1) % 8))))
		return;

	displayRAM[addr] = displayRAM[addr] | ((0x1) << ((segment-1) % 8));
}

static void __sleep nodeOff(unsigned char digit, unsigned char segment, char *displayRAM) {
	unsigned char addr = (digit-1) * 3 + (segment-1) / 8;

	if(displayRAM[addr] == (displayRAM[addr] & (~((0x1) << ((segment-1) % 8)))))
		return;

	displayRAM[addr] = displayRAM[addr] & (~((0x1) << ((segment-1) % 8)));
}
static void __sleep showSymbol(char *displayRAM, int offset, char num, enum VFD_MODULE_TYPE type) {
	int i;

	switch(type) {
		case FUTABA_11BT236:
			for(i=2;i<=8;i++) {
				if(letterTable_11bt236[(int)num][15-i] == 1)
					displayRAM[offset] |= (0x1 << (i-1));
				else
					displayRAM[offset] &= (~(0x1 << (i-1)));
			}
			for(i=9;i<=15;i++) {
				if(letterTable_11bt236[(int)num][15-i] == 1)
					displayRAM[offset+1] |= (0x1 << (i-9));
				else
					displayRAM[offset+1] &= (~(0x1 << (i-9)));
			}
			break;
		case FUTABA_11MT141:
			for(i=0;i<=7;i++) {
				if(letterTable_11mt141[(int)num][13-i] == 1)
					displayRAM[offset] |= (0x1 << (i));
				else
					displayRAM[offset] &= (~(0x1 << (i)));
			}
			for(i=8;i<=13;i++) {
				if(letterTable_11mt141[(int)num][13-i] == 1)
					displayRAM[offset+1] |= (0x1 << (i-8));
				else
					displayRAM[offset+1] &= (~(0x1 << (i-8)));
			}
			break;
		case HNV_11MS41T:
			for(i=1;i<=7;i++) {
				if(letterTable_11ms41t[(int)num][7 - i] == 1)
					displayRAM[offset] |= (0x1 << (i-1));
				else
					displayRAM[offset] &= (~(0x1 << (i-1)));
			}
			break;
		default:
			break;
	}
}

int __sleep update_vfd(int hour, int minute, int second) {
	int i, j;
	unsigned int vfdo_value, wrctl_value;
	unsigned char displayRAM[48];
	enum VFD_MODULE_TYPE vfdType;

	if(platform_info.board_id == realtek_mk_board)
		vfdType = FUTABA_11MT141;
	else if (platform_info.board_id == C02_1262_Neptune_avhdd_board)
		vfdType = HNV_11MS41T;
	else
		vfdType = FUTABA_11BT236;

	for(i=0;i<48;i++)
		displayRAM[i] = 0;

	switch(vfdType) {
		case FUTABA_11BT236:
			nodeOn(6, 16, displayRAM); // :
			nodeOn(4, 16, displayRAM); // :
			break;
		case FUTABA_11MT141:
			nodeOn(6, 15, displayRAM);
//			nodeOn(4, 15, displayRAM);
			break;
		case HNV_11MS41T:
			//nodeOn(9, 8, displayRAM);
			second = second >> 1;
			//if(fgIsOn == 0)
			if((second%2) == 0)
			{
				nodeOn(10, 8, displayRAM);
				//fgIsOn = 1;
			} else
			{
				nodeOff(10, 8, displayRAM);
				//unsigned char addr = (10-1) * 3 + (8-1) / 8;

				//fgIsOn = 0;
				//if(displayRAM[addr] == (displayRAM[addr] & (~((0x1) << ((8-1) % 8)))))
				//	break;
				//displayRAM[addr] = displayRAM[addr] & (~((0x1) << ((8-1) % 8)));
			}
			break;
		default:
			break;
	}

	// for hour
	if(vfdType == HNV_11MS41T)
	{
		showSymbol(displayRAM, 28, hour/10, vfdType);
		showSymbol(displayRAM, 27, hour%10, vfdType);
	} else
	{
	showSymbol(displayRAM, 18, hour/10, vfdType);
	showSymbol(displayRAM, 15, hour%10, vfdType);
	}

	// for minute
	if(vfdType == HNV_11MS41T)
	{
		showSymbol(displayRAM, 31, minute/10, vfdType);
		showSymbol(displayRAM, 30, minute%10, vfdType);
	} else
	{
	showSymbol(displayRAM, 12, minute/10, vfdType);
	showSymbol(displayRAM, 9, minute%10, vfdType);
	}

	// for second
	if (vfdType == FUTABA_11BT236)
	{
    	second = second >> 1;
    	showSymbol(displayRAM, 6, second/10, vfdType);
    	showSymbol(displayRAM, 3, second%10, vfdType);
    }
    
	// [Byte1 : Address setting command, set to 0x10 * i]
	// 0xc0 = 1100-0000
	// [Byte0 : Data setting command, normal operation, incremental, write to display memory]
	// 0x40 = 0100-0000
	for(i = 0 ; i < 3 ; i++) {
		vfdo_value = ((0xc0 | (0x10 * i)) << 8) | 0x40;
		wrctl_value = 0x0330;
		writeAtOnce(vfdo_value, wrctl_value);

		for(j = 0 ; j < 3 ; j ++) {
			vfdo_value = (unsigned int)(displayRAM[16*i+j*4]) |
					((unsigned int)(displayRAM[16*i+j*4+1]) << 8) |
					((unsigned int)(displayRAM[16*i+j*4+2]) << 16) |
					((unsigned int)(displayRAM[16*i+j*4+3]) << 24);

			wrctl_value = 0xf0;
			writeAtOnce(vfdo_value, wrctl_value);
		}

		vfdo_value = (unsigned int)(displayRAM[16*i+12]) |
				((unsigned int)(displayRAM[16*i+13]) << 8) |
				((unsigned int)(displayRAM[16*i+14]) << 16) |
				((unsigned int)(displayRAM[16*i+15]) << 24);
		wrctl_value = 0x000010f0;
		writeAtOnce(vfdo_value, wrctl_value);
	}

	// send command 1 & 4
	// dimming quality = 2(4/16), 11 digit
	vfdo_value = 0x8a0a;
	wrctl_value = 0x00001330;
	writeAtOnce(vfdo_value, wrctl_value);

	if (vfdType == HNV_11MS41T) //set POWER led
	{
		vfdo_value = (0x41) | (0x1500); //1d -> 15
		wrctl_value = 0x1130;
		writeAtOnce(vfdo_value, wrctl_value);
	}
	return 0;
}
int __sleep update_power_led(void)
{
	unsigned int vfdo_value, wrctl_value;
	
        vfdo_value = (0x41) | (0xfc00);

        wrctl_value = 0x1130;
        writeAtOnce(vfdo_value, wrctl_value);
	return 0;	
}
