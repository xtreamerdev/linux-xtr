// Demod I2C and functions

#include <linux/kernel.h>
#include "demod_rtl2830.h"





// Function 4 parameter table
//
const unsigned char Func4ParamTable[FUNC4_REG_VALUE_NUM][FUNC4_PARAM_LEN] =
{
	{
		255, 39,  255, 255, 198, 0,   240,
		255, 24,  10,  127, 222, 4,   160,
		0,   176, 26,  0,   166, 7,   144,
		5,   80,  28,  66,  58,  3,   208,
		13,  223, 237, 69,  135, 229, 48,
		63,  205, 94,  252, 110, 101, 176,
		2,   32,  164, 129, 106, 21,  208,
		6,   80,  43,  65,  102, 3,   240,
		4,   87,  254, 64,  185, 253, 80,
		1,   127, 242, 0,   29,  253, 16,
		255, 223, 248, 63,  223, 254, 176,
	},

	{
		255, 47,  254, 191, 216, 1,   160,
		0,   176, 11,  128, 152, 0,   128,
		1,   247, 234, 127, 143, 247, 144,
		249, 183, 248, 62,  124, 13,  160,
		3,   224, 108, 6,   48,  14,  192,
		79,  213, 215, 1,   226, 104, 144,
		18,  216, 68,  2,   149, 242, 144,
		255, 231, 189, 190, 191, 249, 192,
		252, 32,  15,  63,  236, 6,   80,
		1,   224, 10,  128, 91,  254, 192,
		0,   23,  248, 63,  209, 255, 48,
	},

	{
		0,   216, 2,   0,   29,  254, 32,
		254, 183, 249, 63,  144, 3,   64,
		1,   184, 22,  193, 25,  253, 64,
		255, 167, 205, 61,  197, 252, 96,
		249, 224, 101, 68,  234, 33,  128,
		86,  158, 28,  132, 142, 99,  32,
		20,  159, 240, 0,   125, 231, 192,
		248, 103, 229, 191, 20,  9,   80,
		2,   160, 27,  128, 187, 253, 112,
		255, 135, 238, 127, 159, 255, 240,
		255, 208, 7,   192, 52,  0,   128,
	},

	{
		0,   176, 4,   127, 229, 254, 48,
		255, 248, 12,  128, 85,  252, 0,
		253, 8,   12,  1,   36,  0,   208,
		250, 247, 225, 0,   194, 17,  48,
		3,   63,  146, 187, 90,  35,  208,
		103, 175, 94,  204, 62,  39,  160,
		244, 31,  120, 192, 30,  24,  144,
		4,   63,  198, 254, 172, 5,   80,
		4,   72,  3,   255, 97,  252, 32,
		0,   224, 16,  192, 17,  253, 0,
		255, 104, 6,   64,  49,  255, 48,
	},

	{
		255, 135, 250, 64,  58,  0,   224,
		254, 112, 0,   0,   141, 254, 48,
		253, 176, 19,  64,  111, 247, 128,
		255, 240, 51,  63,  55,  238, 224,
		9,   72,  84,  186, 75,  231, 224,
		104, 208, 101, 204, 211, 230, 192,
		239, 0,   94,  65,  189, 235, 176,
		254, 24,  64,  255, 207, 244, 64,
		2,   16,  30,  127, 101, 252, 0,
		2,   48,  4,   255, 160, 0,   96,
		0,   223, 251, 255, 230, 1,   128,
	},
};



// Function 4 register value threshold table
//
const unsigned long RegValueThdTable[CODE_RATE_NUM] =
{
	700,		// For "RX_C_RATE_HP == 0"
	300,		// For "RX_C_RATE_HP == 1"
	220,		// For "RX_C_RATE_HP == 2"
	120,		// For "RX_C_RATE_HP == 3"
	60,			// For "RX_C_RATE_HP == 4"
};





// Construct demod module.
//
// 1. Allocate demod module memory.
// 2. Set demod type.
// 3. Set demod I2C device address.
// 4. Initialize demod register table.
// 5. Set demod function pointers.
//
int
ConstructRtl2830Module(
	DEMOD_MODULE **ppDemod,
	unsigned char SubType,
	unsigned char DeviceAddr,
	unsigned long Func4PeriodMs
	)
{
    int i;
    
    if ((SubType != DEMOD_RTL2830) && (SubType != DEMOD_RTL2830B) && (SubType != DEMOD_RTL2831U) )
        return MODULE_CONSTRUCTING_ERROR;


	// Allocate demod module memory.
	//
	// Note: Also need to allocate demod register table memory.
	//
	if(AllocateMemory((void **)ppDemod, sizeof(DEMOD_MODULE))
		== MEMORY_ALLOCATING_ERROR)
		goto error_status_allocate_module_memory;

	if(AllocateMemory((void **)&(*ppDemod)->pRegTable,
		sizeof(DEMOD_REG_ENTRY) * RTL2830_REG_TABLE_LEN) == MEMORY_ALLOCATING_ERROR)
		goto error_status_allocate_register_table_memory;


	// Set demod type.
	//
	(*ppDemod)->DemodType = SubType;


	// Set demod I2C device address.
	//
	(*ppDemod)->DeviceAddr = DeviceAddr;


	// Initialize demod register table.
	//
	rtl2830_InitRegTable((*ppDemod)->pRegTable);

	
	// Initialize demod update procedure variables.
	//
	(*ppDemod)->Func2Executing    = ON;
	(*ppDemod)->Func3State        = 0;
	(*ppDemod)->Func3Executing    = ON;
	(*ppDemod)->Func4State        = 0;
	(*ppDemod)->Func4DelayCnt     = 0;
	(*ppDemod)->Func4DelayCntMax  = FUNC4_DELAY_MS / Func4PeriodMs;
	(*ppDemod)->Func4ParamSetting = NO;

	for(i = 0; i < FUNC4_REG_VALUE_NUM; i++)
		(*ppDemod)->Func4RegValue[i] = 0;

	(*ppDemod)->Func5State   = 0;
	(*ppDemod)->Func5QamBak  = 0xff;
	(*ppDemod)->Func5HierBak = 0xff;
	(*ppDemod)->Func5LpCrBak = 0xff;
	(*ppDemod)->Func5HpCrBak = 0xff;
	(*ppDemod)->Func5GiBak   = 0xff;
	(*ppDemod)->Func5FftBak  = 0xff;


	// Set demod function pointers.
	//
	(*ppDemod)->SetRegBytes       = rtl2830_SetRegBytes;
	(*ppDemod)->GetRegBytes       = rtl2830_GetRegBytes;
	(*ppDemod)->SetRegBits        = rtl2830_SetRegBits;
	(*ppDemod)->GetRegBits        = rtl2830_GetRegBits;
	(*ppDemod)->TestI2c           = rtl2830_TestI2c;
	(*ppDemod)->SoftwareReset     = rtl2830_SoftwareReset;
	(*ppDemod)->ForwardTunerI2C   = rtl2830_ForwardTunerI2C;
	(*ppDemod)->Initialize        = rtl2830_Initialize;
	(*ppDemod)->SetBandwidthMode  = rtl2830_SetBandwidthMode;
	(*ppDemod)->SetIfFreqHz       = rtl2830_SetIfFreqHz;
	(*ppDemod)->SetSpectrumMode   = rtl2830_SetSpectrumMode;
	(*ppDemod)->SetVtop           = rtl2830_SetVtop;
	(*ppDemod)->SetKrf            = rtl2830_SetKrf;
	(*ppDemod)->TestTpsLock       = rtl2830_TestTpsLock;
	(*ppDemod)->TestSignalLock    = rtl2830_TestSignalLock;
	(*ppDemod)->GetFsmStage       = rtl2830_GetFsmStage;
	(*ppDemod)->GetSignalStrength = rtl2830_GetSignalStrength;
	(*ppDemod)->GetSignalQuality  = rtl2830_GetSignalQuality;
	(*ppDemod)->GetSnr            = rtl2830_GetSnr;
	(*ppDemod)->GetBer            = rtl2830_GetBer;
	(*ppDemod)->GetEstEvm         = rtl2830_GetEstEvm;
	(*ppDemod)->GetRsdBerEst      = rtl2830_GetRsdBerEst;
	(*ppDemod)->GetRfAgc          = rtl2830_GetRfAgc;
	(*ppDemod)->GetIfAgc          = rtl2830_GetIfAgc;
	(*ppDemod)->GetDiAgc          = rtl2830_GetDiAgc;
	(*ppDemod)->GetConstellation  = rtl2830_GetConstellation;
	(*ppDemod)->GetHierarchy      = rtl2830_GetHierarchy;
	(*ppDemod)->GetCodeRateLp     = rtl2830_GetCodeRateLp;
	(*ppDemod)->GetCodeRateHp     = rtl2830_GetCodeRateHp;
	(*ppDemod)->GetGuardInterval  = rtl2830_GetGuardInterval;
	(*ppDemod)->GetFftMode        = rtl2830_GetFftMode;
	(*ppDemod)->UpdateFunction2   = rtl2830_UpdateFunction2;
	(*ppDemod)->UpdateFunction3   = rtl2830_UpdateFunction3;
	(*ppDemod)->ResetFunction4    = rtl2830_ResetFunction4;
	(*ppDemod)->UpdateFunction4   = rtl2830_UpdateFunction4;
	(*ppDemod)->ResetFunction5    = rtl2830_ResetFunction5;
	(*ppDemod)->UpdateFunction5   = rtl2830_UpdateFunction5;


	return MODULE_CONSTRUCTING_SUCCESS;


error_status_allocate_register_table_memory:
	FreeMemory(*ppDemod);

error_status_allocate_module_memory:
	return MODULE_CONSTRUCTING_ERROR;
}





// Destruct demod module.
//
// 1. Free demod module memory.
//
void
DestructRtl2830Module(
	DEMOD_MODULE *pDemod
	)
{
	// Free demod module memory.
	//
	// Note: Need to free demod register table memory first.
	//
	FreeMemory(pDemod->pRegTable);
	FreeMemory(pDemod);


	return;
}





// Initialize demod register table.
//
// 1. Define primary register table.
// 2. Convert primary register table to formal register table.
//    a. Generate Mask and Shift.
//    b. Sort primary register table by RegBitName key.
//
void
rtl2830_InitRegTable(
	DEMOD_REG_ENTRY *pRegTable
	)
{
	// Primary register entry only used in InitRegTable()
	//
	typedef struct
	{
		int RegBitName;
		unsigned char PageNo;
		unsigned char RegStartAddr;
		unsigned char ByteNum;
		unsigned char Msb;
		unsigned char Lsb;
	}
	DEMOD_PRIMARY_REG_ENTRY;



	unsigned int i, j;
	int RegBitName;
	unsigned char Msb, Lsb;
	unsigned long Mask;
	unsigned char Shift;

	const DEMOD_PRIMARY_REG_ENTRY PrimaryRegTable[RTL2830_REG_TABLE_LEN] =
	{

		// Software reset register
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{SOFT_RST,				1,			0x01,			1,			2,		2},


		// Tuner I2C forwording register
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{IIC_REPEAT,			1,			0x01,			1,			3,		3},


		// Registers for initialization
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{REG_PFREQ_1_0,			0,			0x0d,			1,			1,		0},
		{PD_DA8,				0,			0x0d,			1,			4,		4},
		{LOCK_TH,				1,			0x10,			1,			1,		0},
		{BER_PASS_SCAL,			1,			0x8c,			2,			11,		6},
		{TR_WAIT_MIN_8K,		1,			0x88,			2,			11,		2},
		{RSD_BER_FAIL_VAL,		1,			0x8f,			2,			15,		0},
		{CE_FFSM_BYPASS,		2,			0xd5,			1,			1,		1},
		{ALPHAIIR_N,			2,			0xf1,			1,			2,		1},
		{ALPHAIIR_DIF,			2,			0xf1,			1,			7,		3},
		{EN_TRK_SPAN,			1,			0x6d,			1,			0,		0},
		{EN_BK_TRK,				1,			0xa6,			1,			7,		7},
		{DAGC_TRG_VAL,			1,			0x12,			1,			7,		0},
		{AGC_TARG_VAL,			1,			0x03,			1,			7,		0},
		{REG_PI,				0,			0x0a,			1,			2,		0},
		{LOCK_TH_LEN,			1,			0x10,			1,			3,		2},
		{CCI_THRE,				1,			0x40,			1,			5,		2},
		{CCI_MON_SCAL,			1,			0x40,			1,			7,		6},
		{CCI_M0,				1,			0x5b,			1,			2,		0},
		{CCI_M1,				1,			0x5b,			1,			5,		3},
		{CCI_M2,				1,			0x5c,			1,			2,		0},
		{CCI_M3,				1,			0x5c,			1,			5,		3},


		// Registers for initialization according to mode
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{TRK_KS_P2,				1,			0x6f,			1,			2,		0},
		{TRK_KS_I2,				1,			0x70,			1,			5,		3},
		{TR_THD_SET2,			1,			0x72,			1,			3,		0},
		{TRK_KC_P2,				1,			0x73,			1,			5,		3},
		{TRK_KC_I2,				1,			0x75,			1,			2,		0},
		{CR_THD_SET2,			1,			0x76,			1,			7,		6},


		// Registers for initialization according to tuner type
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{PSET_IFFREQ,			1,			0x19,			3,			21,		0},
		{SPEC_INV,				1,			0x15,			1,			0,		0},
		{VTOP,					1,			0x06,			1,			5,		0},
		{KRF,					1,			0x07,			1,			5,		0},


		// Registers for bandwidth programming
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{BW_INDEX,				0,			0x08,			1,			2,		1},


		// FSM stage register
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{FSM_STAGE,				3,			0x51,			1,			6,		3},


		// TPS content registers
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{RX_CONSTEL,			3,			0x3c,			1,			3,		2},
		{RX_HIER,				3,			0x3c,			1,			6,		4},
		{RX_C_RATE_LP,			3,			0x3d,			1,			2,		0},
		{RX_C_RATE_HP,			3,			0x3d,			1,			5,		3},
		{GI_IDX,				3,			0x51,			1,			1,		0},
		{FFT_MODE_IDX,			3,			0x51,			1,			2,		2},


		// Performance measurement registers
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{RSD_BER_EST,			3,			0x4e,			2,			15,		0},
		{EST_EVM,				4,			0x0c,			2,			15,		0},


		// AGC registers
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{RF_AGC_VAL,			3,			0x5b,			2,			13,		0},
		{IF_AGC_VAL,			3,			0x59,			2,			13,		0},
		{DAGC_VAL,				3,			0x05,			1,			7,		0},


		// AGC relative registers
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{POLAR_RF_AGC,			0,			0x0e,			1,			1,		1},
		{POLAR_IF_AGC,			0,			0x0e,			1,			0,		0},
		{LOOP_GAIN_3_0,			1,			0x04,			1,			4,		1},
		{LOOP_GAIN_4,			1,			0x05,			1,			7,		7},
		{AAGC_HOLD,				1,			0x04,			1,			5,		5},
		{EN_RF_AGC,				1,			0x04,			1,			6,		6},
		{EN_IF_AGC,				1,			0x04,			1,			7,		7},
		{IF_AGC_MIN,			1,			0x08,			1,			7,		0},
		{IF_AGC_MAX,			1,			0x09,			1,			7,		0},
		{RF_AGC_MIN,			1,			0x0a,			1,			7,		0},
		{RF_AGC_MAX,			1,			0x0b,			1,			7,		0},
		{IF_AGC_MAN,			1,			0x0c,			1,			6,		6},
		{IF_AGC_MAN_VAL,		1,			0x0c,			2,			13,		0},
		{RF_AGC_MAN,			1,			0x0e,			1,			6,		6},
		{RF_AGC_MAN_VAL,		1,			0x0e,			2,			13,		0},


		// TS interface registers
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{CK_OUT_PAR,			1,			0x7b,			1,			5,		5},
		{CK_OUT_PWR,			1,			0x7b,			1,			6,		6},
		{SYNC_DUR,				1,			0x7b,			1,			7,		7},
		{ERR_DUR,				1,			0x7c,			1,			0,		0},
		{SYNC_LVL,				1,			0x7c,			1,			1,		1},
		{ERR_LVL,				1,			0x7c,			1,			2,		2},
		{VAL_LVL,				1,			0x7c,			1,			3,		3},
		{SERIAL,				1,			0x7c,			1,			4,		4},
		{SER_LSB,				1,			0x7c,			1,			5,		5},
		{CDIV_PH0,				1,			0x7d,			1,			3,		0},
		{CDIV_PH1,				1,			0x7d,			1,			7,		4},

		
		// FSM state-holding register
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{SM_BYPASS,				1,			0x93,			2,			11,		0},


		// Registers for function 2
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{UPDATE_REG_2,			2,			0xe4,			1,			7,		7},


		// Registers for function 3
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{BTHD_P3,				1,			0x65,			1,			2,		0},
		{BTHD_D3,				1,			0x68,			1,			5,		4},


		// Registers for function 4
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{FUNC4_REG0,			1,			0x93,			1,			3,		2},
		{FUNC4_REG1,			1,			0x65,			1,			7,		0},
		{FUNC4_REG2,			1,			0x68,			1,			7,		0},
		{FUNC4_REG3,			3,			0x3d,			1,			6,		6},
		{FUNC4_REG4,			3,			0x3d,			1,			7,		7},
		{FUNC4_REG5,			3,			0x50,			1,			0,		0},
		{FUNC4_REG6,			3,			0x50,			1,			1,		1},
		{FUNC4_REG7,			4,			0xad,			1,			3,		0},
		{FUNC4_REG8,			3,			0x28,			1,			6,		3},
		{FUNC4_REG9,			1,			0x64,			1,			0,		0},
		{FUNC4_REG10,			4,			0x10,			2,			13,		0},


		// Registers for functin 5
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{FUNC5_REG0,			3,			0x28,			1,			6,		3},
		{FUNC5_REG1,			3,			0x1a,			2,			15,		3},
		{FUNC5_REG2,			3,			0x1c,			2,			15,		3},
		{FUNC5_REG3,			3,			0x1e,			2,			15,		3},
		{FUNC5_REG4,			3,			0x20,			2,			15,		3},
		{FUNC5_REG5,			1,			0x41,			1,			2,		0},
		{FUNC5_REG6,			1,			0x41,			1,			6,		3},
		{FUNC5_REG7,			1,			0x41,			1,			7,		7},
		{FUNC5_REG8,			1,			0x40,			1,			0,		0},
		{FUNC5_REG9,			2,			0xdd,			1,			6,		0},
		{FUNC5_REG10,			1,			0x53,			2,			12,		0},
		{FUNC5_REG11,			1,			0x55,			2,			12,		0},
		{FUNC5_REG12,			1,			0x57,			2,			12,		0},
		{FUNC5_REG13,			1,			0x42,			3,			23,		7},
		{FUNC5_REG14,			1,			0x46,			3,			21,		5},
		{FUNC5_REG15,			1,			0x4a,			3,			19,		3},
		{FUNC5_REG16,			1,			0x44,			3,			22,		6},
		{FUNC5_REG17,			1,			0x48,			3,			20,		4},
		{FUNC5_REG18,			1,			0x4c,			3,			18,		2},


		// Test registers for test only
		//
		// RegBitName,			PageNo,		RegStartAddr,	ByteNum,	MSB,	LSB
		{TEST_REG_1,			1,			0x95,			1,			6,		2},
		{TEST_REG_2,			1,			0x96,			2,			13,		3},
		{TEST_REG_3,			1,			0x97,			3,			21,		0},
		{TEST_REG_4,			1,			0x98,			4,			29,		1},

	};





	// Convert primary register table to formal register table.
	//
	for(i = 0; i < RTL2830_REG_TABLE_LEN; i++)
	{
		// Get RegBitName, MSB, and LSB from primary register entry.
		//
		RegBitName = PrimaryRegTable[i].RegBitName;
		Msb = PrimaryRegTable[i].Msb;
		Lsb = PrimaryRegTable[i].Lsb;


		// Generate mask and shift.
		//
		Mask = 0;
		for(j = Lsb; j < (unsigned char)(Msb + 1); j++)
			Mask |= 0x1 << j;

		Shift = Lsb;


		// Set formal register entry according to primary one.
		// Note: foraml register table is sorted by RegBitName key.
		//
		pRegTable[RegBitName].PageNo       = PrimaryRegTable[i].PageNo;
		pRegTable[RegBitName].RegStartAddr = PrimaryRegTable[i].RegStartAddr;
		pRegTable[RegBitName].ByteNum      = PrimaryRegTable[i].ByteNum;
		pRegTable[RegBitName].Mask         = Mask;
		pRegTable[RegBitName].Shift        = Shift;
	}


	return;
}





// Demod I2C functions

// Set demod register bytes.
//
// 1. Set demod page register with PageNo.
// 2. Set demod registers sequentially with pWritingBytes.
//    The start register address is RegStartAddr.
//    The writing data length is ByteNum bytes.
//
int
rtl2830_SetRegBytes(
	DEMOD_MODULE *pDemod,
	unsigned char PageNo,
	unsigned char RegStartAddr,
	const unsigned char *pWritingBytes,
	unsigned char ByteNum
	)
{
	unsigned int i;
	unsigned char *pBuffer;



	// Set demod page register with PageNo.
	// I2C format:
	//     start_bit + (DeviceAddr | writing_bit) + PAGE_REG_ADDR + PageNo + stop_bit
	//
	if(AllocateMemory((void **)&pBuffer, LEN_2_BYTE) == MEMORY_ALLOCATING_ERROR)
		goto error_status_allocate_memory;

	pBuffer[0] = PAGE_REG_ADDR;
	pBuffer[1] = PageNo;

	if(SendWriteCommand(pDemod->DeviceAddr, pBuffer, LEN_2_BYTE) == I2C_ERROR)
		goto error_status_send_write_command;

	FreeMemory(pBuffer);


	// Set demod registers sequentially with pWritingBytes.
	// I2C format:
	//     start_bit + (DeviceAddr | writing_bit) + RegStartAddr + WritingBytes + stop_bit
	// Note: The start register address is RegStartAddr.
	//       The writing data length is ByteNum bytes.
	//
	if(AllocateMemory((void **)&pBuffer, LEN_1_BYTE + ByteNum) == MEMORY_ALLOCATING_ERROR)
		goto error_status_allocate_memory;
	
	pBuffer[0] = RegStartAddr;
	for(i = 0; i < ByteNum; i++)
		pBuffer[i + 1] = pWritingBytes[i];

	if(SendWriteCommand(pDemod->DeviceAddr, pBuffer, LEN_1_BYTE + ByteNum) == I2C_ERROR)
		goto error_status_send_write_command;
	
	FreeMemory(pBuffer);


	return REG_ACCESS_SUCCESS;


error_status_send_write_command:
	FreeMemory(pBuffer);

error_status_allocate_memory:
	return REG_ACCESS_ERROR;
}





// Get demod register bytes.
//
// 1. Set demod page register with PageNo.
// 2. Set the start register address to demod for reading.
//    The start register address is RegStartAddr.
// 3. Get demod registers sequentially and store register values to pReadingBytes.
//    The reading data length is ByteNum bytes.
//
int
rtl2830_GetRegBytes(
	DEMOD_MODULE *pDemod,
	unsigned char PageNo,
	unsigned char RegStartAddr,
	unsigned char *pReadingBytes,
	unsigned char ByteNum
	)
{
	unsigned char *pBuffer;



	// Set demod page register with PageNo.
	// I2C format:
	//     start_bit + (DeviceAddr | writing_bit) + PAGE_REG_ADDR + PageNo + stop_bit
	//
	if(AllocateMemory((void **)&pBuffer, LEN_2_BYTE) == MEMORY_ALLOCATING_ERROR)
		goto error_status_allocate_memory;

	pBuffer[0] = PAGE_REG_ADDR;
	pBuffer[1] = PageNo;

	if(SendWriteCommand(pDemod->DeviceAddr, pBuffer, LEN_2_BYTE) == I2C_ERROR)
		goto error_status_send_write_command;

	FreeMemory(pBuffer);


	// Set the start register address to demod.
	// I2C format:
	//     start_bit + (DeviceAddr | writing_bit) + RegStartAddr + stop_bit
	// Note: The start register address is RegStartAddr.
	//
	if(AllocateMemory((void **)&pBuffer, LEN_1_BYTE) == MEMORY_ALLOCATING_ERROR)
		goto error_status_allocate_memory;

	pBuffer[0] = RegStartAddr;

	if(SendWriteCommand(pDemod->DeviceAddr, pBuffer, LEN_1_BYTE) == I2C_ERROR)
		goto error_status_send_write_command;
	
	FreeMemory(pBuffer);


	// Get demod registers sequentially and store register values to pReadingBytes.
	// I2C format:
	//     start_bit + (DeviceAddr | reading_bit) + ReadingBytes + stop_bit
	// Note: The reading data length is ByteNum bytes.
	//
	if(SendReadCommand(pDemod->DeviceAddr, pReadingBytes, ByteNum) == I2C_ERROR)
		goto error_status_send_read_command;


	return REG_ACCESS_SUCCESS;


error_status_send_write_command:
	FreeMemory(pBuffer);

error_status_allocate_memory:
error_status_send_read_command:
	return REG_ACCESS_ERROR;
}





// Set demod register bits.
//
// 1. Find out register information from register table by RegBitName.
//    Register information: PageNo, RegStartAddr, ByteNum, Mask, and Shift.
// 2. Write register bits with WritingValue and reserve unmask bits at the same bytes.
//
// Important note: ByteNum must be 1 ~ 4.
//
int
rtl2830_SetRegBits(
	DEMOD_MODULE *pDemod,
	int RegBitName,
	unsigned long WritingValue
	)
{
	unsigned int i;
	unsigned char Buffer[LEN_4_BYTE];

	unsigned char PageNo;
	unsigned char RegStartAddr;
	unsigned char ByteNum;
	unsigned long Mask;
	unsigned char Shift;
	unsigned long Value;



	// Find out register information from register table by RegBitName.
	//
	PageNo       = pDemod->pRegTable[RegBitName].PageNo;
	RegStartAddr = pDemod->pRegTable[RegBitName].RegStartAddr;
	ByteNum      = pDemod->pRegTable[RegBitName].ByteNum;
	Mask         = pDemod->pRegTable[RegBitName].Mask;
	Shift        = pDemod->pRegTable[RegBitName].Shift;


	// Read ByteNum bytes from demod to Buffer.
	//
	if(pDemod->GetRegBytes(pDemod, PageNo, RegStartAddr, Buffer, ByteNum)
		== REG_ACCESS_ERROR)
		goto error_status_get_register_bytes;


	// Combine Buffer bytes into an unsgined integer Value.
	// Note: Lower address byte is in Value MSB.
	//       Upper address byte is in Value LSB.
	//
	Value = 0;
	for(i = 0; i < ByteNum; i++)
		Value |= (unsigned long)Buffer[i] << ((ByteNum - i - 1) * BYTE_SHIFT);


	// Reserve Value unmask bit with Mask and inlay WritingValue into Value.
	//
	Value &= ~Mask;
	Value |= WritingValue << Shift & Mask;


	// Separate Value into Buffer bytes.
	// Note: Lower address byte is in Value MSB.
	//       Upper address byte is in Value LSB.
	//
	for(i = 0; i < ByteNum; i++)
		Buffer[i] = (unsigned char)(Value >> ((ByteNum - i - 1) * BYTE_SHIFT) & BYTE_MASK);


	// Write ByteNum bytes from Buffer to demod.
	//
	if(pDemod->SetRegBytes(pDemod, PageNo, RegStartAddr, Buffer, ByteNum)
		== REG_ACCESS_ERROR)
		goto error_status_set_register_bytes;


	return REG_ACCESS_SUCCESS;


error_status_get_register_bytes:
error_status_set_register_bytes:
	return REG_ACCESS_ERROR;
}





// Get demod register bits.
//
// 1. Find out register information from register table by RegBitName.
//    Register information: PageNo, RegStartAddr, ByteNum, Mask, and Shift.
// 2. Read register bits into ReadingValue.
//
// Important note: ByteNum must be 1 ~ 4.
//
int
rtl2830_GetRegBits(
	DEMOD_MODULE *pDemod,
	int RegBitName,
	unsigned long *pReadingValue
	)
{
	unsigned int i;
	unsigned char Buffer[LEN_4_BYTE];

	unsigned char PageNo;
	unsigned char RegStartAddr;
	unsigned char ByteNum;
	unsigned long Mask;
	unsigned char Shift;
	unsigned long Value;



	// Find out register information from register table by RegBitName.
	//
	PageNo       = pDemod->pRegTable[RegBitName].PageNo;
	RegStartAddr = pDemod->pRegTable[RegBitName].RegStartAddr;
	ByteNum      = pDemod->pRegTable[RegBitName].ByteNum;
	Mask         = pDemod->pRegTable[RegBitName].Mask;
	Shift        = pDemod->pRegTable[RegBitName].Shift;


	// Read ByteNum bytes from demod to Buffer.
	//
	if(pDemod->GetRegBytes(pDemod, PageNo, RegStartAddr, Buffer, ByteNum)
		== REG_ACCESS_ERROR)
		goto error_status_get_register_bytes;


	// Combine Buffer bytes into an unsgined integer Value.
	// Note: Lower address byte is in Value MSB.
	//       Upper address byte is in Value LSB.
	//
	Value = 0;
	for(i = 0; i < ByteNum; i++)
		Value |= (unsigned long)Buffer[i] << ((ByteNum - i - 1) * BYTE_SHIFT);


	// Get register bits from Value to ReadingValue with Mask and Shift.
	//
	*pReadingValue = (Value & Mask) >> Shift;


	return REG_ACCESS_SUCCESS;


error_status_get_register_bytes:
	return REG_ACCESS_ERROR;
}





// Demod operating functions

// Test demod I2C interface.
//
// 1. Send read command.
// 2. Set I2cStatus according to the result of read command sending.
//
int
rtl2830_TestI2c(
	DEMOD_MODULE *pDemod,
	unsigned char *pI2cStatus
	)
{
	unsigned char Nothing;



	// Send read command.
	//
	// Note: The number of reading bytes must be greater than 0.
	//
	if(SendReadCommand(pDemod->DeviceAddr, &Nothing, LEN_1_BYTE) == I2C_ERROR)
		goto error_status_send_read_command;


	// Set I2cStatus with DEMOD_I2C_SUCCESS.
	//
	*pI2cStatus = DEMOD_I2C_SUCCESS;


	return FUNCTION_SUCCESS;


error_status_send_read_command:

	// Set I2cStatus with DEMOD_I2C_ERROR.
	//
	*pI2cStatus = DEMOD_I2C_ERROR;


	return FUNCTION_ERROR;
}





// Reset demod with software reset
//
// 1. Set SOFT_RST with 1.
// 2. Set SOFT_RST with 0.
//
// Note: Software reset does not reset all demod setting registers.
//
int
rtl2830_SoftwareReset(
	DEMOD_MODULE *pDemod
	)
{
	// Set SOFT_RST with 1. Then, set SOFT_RST with 0.
	//
	if(pDemod->SetRegBits(pDemod, SOFT_RST, 0x1) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, SOFT_RST, 0x0) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Enable demod tuner-I2C forwarding.
//
// 1. Set IIC_REPEAT to enable tuner-I2C forwarding.
//
// Note: a. Tuner-I2C forwarding start condition - Set IIC_REPEAT with 1.
//          In tuner-I2C forwarding mode, demod can not receive I2C commands.
//       b. Tuner-I2C forwarding stop condition - Stop bit appears in I2C.
//          Demod hardware will set IIC_REPEAT with 0 automatically.
//
int
rtl2830_ForwardTunerI2C(
	DEMOD_MODULE *pDemod
	)
{
	// Set IIC_REPEAT to enable tuner-I2C forwarding.
	//
	if(pDemod->SetRegBits(pDemod, IIC_REPEAT, 0x1) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Initialize demod.
//
// 1. Initialize demod registers according to the initializing table.
// 2. Initialize demod registers according to the initializing table with mode.
// 3. Initialize demod registers with input arguments based on tuner characteristics.
//
int
rtl2830_Initialize(
	DEMOD_MODULE *pDemod,
	int InitMode,
	unsigned long IfFreqHz,
	int SpectrumMode,
	unsigned char Vtop,
	unsigned char Krf,
	unsigned char OutputMode,
	unsigned char OutputFreq
	)
{
	// Initializing table entry only used in Initialize()
	//
	typedef struct
	{
		int RegBitName;
		unsigned long WritingValue;
	}
	INIT_TABLE_ENTRY;

	// Initializing table entry with mode only used in Initialize()
	//
	typedef struct
	{
		int RegBitName;
		unsigned long WritingValue[INIT_MODE_NUM];
	}
	INIT_TABLE_ENTRY_WITH_MODE;



	unsigned int i;

	const INIT_TABLE_ENTRY InitTable[INIT_TABLE_LEN] =
	{
		// RegBitName,			WritingValue
		{REG_PFREQ_1_0,			0x1		},
		{PD_DA8,				0x1		},
		{LOOP_GAIN_3_0,			0x0		},
		{LOOP_GAIN_4,			0x1		},
		{LOCK_TH,				0x2		},
		{CK_OUT_PWR,			0x0		},
		{CDIV_PH0,				0x5		},
		{CDIV_PH1,				0x5		},
		{BER_PASS_SCAL,			0x20	},
		{TR_WAIT_MIN_8K,		0x140	},
		{RSD_BER_FAIL_VAL,		0x2800	},
		{CE_FFSM_BYPASS,		0x1		},
		{ALPHAIIR_N,			0x1		},
		{ALPHAIIR_DIF,			0x4		},
		{EN_TRK_SPAN,			0x0		},
		{EN_BK_TRK,				0x0		},
		{DAGC_TRG_VAL,			0x28	},
		{AGC_TARG_VAL,			0x2d	},
		{REG_PI,				0x2		},
		{LOCK_TH_LEN,			0x2		},
		{CCI_THRE,				0x3		},
		{CCI_MON_SCAL,			0x1		},
		{CCI_M0,				0x5		},
		{CCI_M1,				0x5		},
		{CCI_M2,				0x5		},
		{CCI_M3,				0x5		},
	};

	const unsigned char MgdThdxInitTable[MGD_THDX_LEN] =
	{
		0x04,	0x06,	0x0a,	0x12,	0x0a,	0x12,	0x1e,	0x28,
	};

	const INIT_TABLE_ENTRY_WITH_MODE InitTableWithMode[INIT_TABLE_LEN_WITH_MODE] =
	{
		// RegBitName,			WritingValue for {Dongle, STB}
		{TRK_KS_P2,				{0x1,	0x4}},
		{TRK_KS_I2,				{0x3,	0x7}},
		{TR_THD_SET2,			{0xf,	0x6}},
		{TRK_KC_P2,				{0x1,	0x4}},
		{TRK_KC_I2,				{0x1,	0x6}},
		{CR_THD_SET2,			{0x0,	0x1}},				
	};
	
	const INIT_TABLE_ENTRY_WITH_MODE OutputFreqInitTableWithMode[2] =
	{
		// RegBitName,			WritingValue for {6MHz, 10MHz}
        {CDIV_PH0,				{0x09,  0x05}},
		{CDIV_PH1,				{0x09,	0x05}},							
	};
	
	const INIT_TABLE_ENTRY_WITH_MODE OutputFreqInitTableWithMode_RTL2830B[2] =
	{
		// RegBitName,			WritingValue for {6MHz, 10MHz}
		{CDIV_PH0,				{0x01,  0x05}},
		{CDIV_PH1,				{0x00,	0x05}},					
	};
	
	const INIT_TABLE_ENTRY_WITH_MODE OutputModeInitTableWithMode[7] =
	{
		// RegBitName,			WritingValue for {parallel, serial}
	    {SYNC_DUR,              {0x01,  0x01}},						
	    {ERR_DUR,				{0x01,  0x01}},
		{SYNC_LVL,				{0x00,  0x00}},
		{ERR_LVL,				{0x00,  0x00}},
		{VAL_LVL,				{0x00,  0x00}},		
		{SERIAL,				{0x00,  0x01}},
        {SER_LSB,				{0x00,  0x00}},				        
	};	

	// Initialize demod registers according to the initializing table.
	//
	// Note: Initialize MGD_THDX registers according to MGD_THDX initializing table.
	//
	for(i = 0; i < INIT_TABLE_LEN; i++)
	{
		if(pDemod->SetRegBits(pDemod, InitTable[i].RegBitName, InitTable[i].WritingValue)
			== REG_ACCESS_ERROR)
			goto error_status_set_registers;
	}

	if(pDemod->SetRegBytes(pDemod, MGD_THDX_PAGE, MGD_THDX_ADDR, MgdThdxInitTable,
		MGD_THDX_LEN) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	// Initialize demod registers according to the initializing table with mode.
	//

#if 1	
	if (InitMode == DONGLE_APPLICATION)
        printk("Init Mode : Dongle \n");
    else
        printk("Init Mode : STB  \n"); 
#endif    	
	for(i = 0; i < INIT_TABLE_LEN_WITH_MODE; i++)
	{
		if(pDemod->SetRegBits(pDemod, InitTableWithMode[i].RegBitName,
			InitTableWithMode[i].WritingValue[InitMode]) == REG_ACCESS_ERROR)
			goto error_status_set_registers;
	}


    // Output Mode Setting : Parallel / Serial
	// 

#if 1
	if (OutputMode== OUT_IF_PARALLEL)
        printk("Output Mode : Parallel \n");
    else
        printk("Output Mode : Serial \n"); 
#endif     
	for(i = 0; i < 7; i++)
	{
		if(pDemod->SetRegBits(pDemod, OutputModeInitTableWithMode[i].RegBitName,
			OutputModeInitTableWithMode[i].WritingValue[OutputMode]) == REG_ACCESS_ERROR)
			goto error_status_set_registers;
	}	

    // Output Freq Setting : 6M / 10M
	//
#if 1	
	if (OutputFreq == OUT_FREQ_10MHZ)
        printk("Output Freq : 10 MHz \n");
    else
        printk("Output Freq : 6MHz \n"); 
#endif  
  
    if (pDemod->DemodType == DEMOD_RTL2830)
    {
        // for RTL2830
	    for(i = 0; i < 2; i++) 
	    {
		    if(pDemod->SetRegBits(pDemod, OutputFreqInitTableWithMode[i].RegBitName,
			    OutputFreqInitTableWithMode[i].WritingValue[OutputFreq]) == REG_ACCESS_ERROR)
			    goto error_status_set_registers;
        }	
    }
    else
    {       
        // for RTL2830B/RTL2831U        
        for(i = 0; i < 2; i++) 
        {
		    if(pDemod->SetRegBits(pDemod, OutputFreqInitTableWithMode_RTL2830B[i].RegBitName,
			    OutputFreqInitTableWithMode[i].WritingValue[OutputFreq]) == REG_ACCESS_ERROR)
			    goto error_status_set_registers;
        }	
    }        

	// Initialize demod registers with input arguments based on tuner characteristics.
	//
	if(pDemod->SetIfFreqHz(pDemod, IfFreqHz) == FUNCTION_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetSpectrumMode(pDemod, SpectrumMode) == FUNCTION_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetVtop(pDemod, Vtop) == FUNCTION_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetKrf(pDemod, Krf) == FUNCTION_ERROR)
		goto error_status_set_registers;

#if 0
    if(pDemod->GetRegBytes(pDemod, 0x01, 0x7B, &Vtop, 1) == REG_ACCESS_ERROR)
		goto error_status_set_registers;
		
    printk("0x7B = %02x\n",Vtop);
    
    if(pDemod->GetRegBytes(pDemod, 0x01, 0x7C, &Vtop, 1) == REG_ACCESS_ERROR)
		goto error_status_set_registers;
		
    printk("0x7C = %02x\n",Vtop);
    
    if(pDemod->GetRegBytes(pDemod, 0x01, 0x7D, &Vtop, 1) == REG_ACCESS_ERROR)
		goto error_status_set_registers;    
    printk("0x7D = %02x\n",Vtop);
    
#endif

	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Set demod bandwidth mode.
//
// 1. Set BW_INDEX according to BandwidthMode.
// 2. Set H_LPF_X registers with HlpfxTable according to BandwidthMode.
// 3. Set RATIO registers with RatioTable according to BandwidthMode.
//
int
rtl2830_SetBandwidthMode(
	DEMOD_MODULE *pDemod,
	int BandwidthMode
	)
{
	const unsigned char BwIndexTable[BANDWIDTH_MODE_NUM] =
	{
		0x0,	0x1,	0x2,
	};

	const unsigned char HlpfxTable[BANDWIDTH_MODE_NUM][H_LPF_X_LEN] =
	{
		// H_LPF_X writing value for 6 MHz bandwidth
		{
			31,		240,	31,		240,	31,		250,	0,		23,		0,		65,
			0,		100,	0,		103,	0,		56,		31,		222,	31,		122,
			31,		71,		31,		124,	0,		48,		1,		75,		2,		130,
			3,		115,	3,		207,
		},

		// H_LPF_X writing value for 7 MHz bandwidth
		{
			31,		250,	31,		218,	31,		193,	31,		179,	31,		202,
			0,		7,		0,		77,		0,		109,	0,		64,		31,		202,
			31,		77,		31,		42,		31,		178,	0,		236,	2,		126,
			3,		208,	4,		83,
		},

		// H_LPF_X writing value for 8 MHz bandwidth
		{
			0,		16,		0,		14,		31,		247,	31,		201,	31,		160,
			31,		166,	31,		236,	0,		78,		0,		125,	0,		58,
			31,		152,	31,		16,		31,		64,		0,		117,	2,		95,
			4,		36,		4,		219,
		},
	};

	const unsigned char RatioTable[BANDWIDTH_MODE_NUM][RATIO_LEN] =
	{
		// RATIO writing value for 6 MHz bandwidth
		{
			195,	12,		68,		51,		51,		48,
		},

		// RATIO writing value for 7 MHz bandwidth
		{
			184,	227,	147,	153,	153,	152,
		},

		// RATIO writing value for 8 MHz bandwidth
		{
			174,	186,	243,	38,		102,	100,
		},
	};



	// Set BW_INDEX according to BandwidthMode.
	//
	if(pDemod->SetRegBits(pDemod, BW_INDEX, BwIndexTable[BandwidthMode])
		== REG_ACCESS_ERROR)
		goto error_status_set_registers;


	// Set H_LPF_X registers with HlpfxTable according to BandwidthMode.
	//
	if(pDemod->SetRegBytes(pDemod, H_LPF_X_PAGE, H_LPF_X_ADDR, HlpfxTable[BandwidthMode],
		H_LPF_X_LEN) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	// Set RATIO registers with RatioTable according to BandwidthMode.
	//
	if(pDemod->SetRegBytes(pDemod, RATIO_PAGE, RATIO_ADDR, RatioTable[BandwidthMode],
		RATIO_LEN) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Set demod IF frequency.
//
// 1. Determine PsetIffreq according to IF frequency.
// 2. Set PSET_IFFREQ with PsetIffreq.
//
int
rtl2830_SetIfFreqHz(
	DEMOD_MODULE *pDemod,
	unsigned long IfFreqHz
	)
{
	unsigned long PsetIffreq;



	// Determine PsetIfFreq according to IF frequency.
	//
	switch(IfFreqHz)
	{
		case IF_FREQ_4571429HZ:		PsetIffreq = 0x35d75e;		break;
		case IF_FREQ_36125000HZ:	PsetIffreq = 0x2fb8e4;		break;
		case IF_FREQ_36166667HZ:	PsetIffreq = 0x2fa130;		break;

		default:	PsetIffreq = 0x0;	break;
	}


	// Set PSET_IFFREQ with PsetIfFreq.
	//
	if(pDemod->SetRegBits(pDemod, PSET_IFFREQ, PsetIffreq) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Set demod spectrum mode.
//
// 1. Set SPEC_INV with SpecInv according to spectrum mode.
//
int
rtl2830_SetSpectrumMode(
	DEMOD_MODULE *pDemod,
	int SpectrumMode
	)
{
	unsigned long SpecInv[SPECTRUM_MODE_NUM] =
	{
		0,		// NON_SPECTRUM_INVERSE
		1,      // SPECTRUM_INVERSE
	};



	// Set SPEC_INV with SpecInv according to spectrum mode.
	//
	if(pDemod->SetRegBits(pDemod, SPEC_INV, SpecInv[SpectrumMode]) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Set demod VTOP register.
//
// 1. Set VTOP with Vtop.
//
int
rtl2830_SetVtop(
	DEMOD_MODULE *pDemod,
	unsigned char Vtop
	)
{
	// Set VTOP with Vtop.
	//
	if(pDemod->SetRegBits(pDemod, VTOP, Vtop) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Set demod KRF register.
//
// 1. Set KRF with Krf.
//
int
rtl2830_SetKrf(
	DEMOD_MODULE *pDemod,
	unsigned char Krf
	)
{
	// Set KRF with Krf.
	//
	if(pDemod->SetRegBits(pDemod, KRF, Krf) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Test demod TPS lock status.
//
// 1. Get FSM_STAGE to FsmStage.
// 2. Set TpsLockStatus according to FsmStage.
//
int
rtl2830_TestTpsLock(
	DEMOD_MODULE *pDemod,
	unsigned char *pTpsLockStatus
	)
{
	unsigned char FsmStage;



	// Get FSM_STAGE to FsmStage.
	//
	if(pDemod->GetFsmStage(pDemod, &FsmStage) == FUNCTION_ERROR)
		goto error_status_get_registers;


	//Set TpsLockStatus according to FsmStage.
	//
	if(FsmStage > 9)
		*pTpsLockStatus = TPS_LOCK;
	else
		*pTpsLockStatus = TPS_NON_LOCK;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Test demod signal lock status.
//
// 1. Get FSM_STAGE to FsmStage.
// 2. Set SignalLockStatus according to FsmStage.
//
int
rtl2830_TestSignalLock(
	DEMOD_MODULE *pDemod,
	unsigned char *pSignalLockStatus
	)
{
	unsigned char FsmStage;



	// Get FSM_STAGE to FsmStage.
	//
	if(pDemod->GetFsmStage(pDemod, &FsmStage) == FUNCTION_ERROR)
		goto error_status_get_registers;


	//Set SignalLockStatus according to FsmStage.
	//
	if(FsmStage == 11)
		*pSignalLockStatus = SIGNAL_LOCK;
	else
		*pSignalLockStatus = SIGNAL_NON_LOCK;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get FSM stage.
//
// 1. Get FSM_STAGE to FsmStage.
//
int
rtl2830_GetFsmStage(
	DEMOD_MODULE *pDemod,
	unsigned char *pFsmStage
	)
{
	unsigned long Value;



	// Get FSM_STAGE to FsmStage.
	//
	if(pDemod->GetRegBits(pDemod, FSM_STAGE, &Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	*pFsmStage = (unsigned char)Value;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod signal strength.
//
// 1. Get FSM stage and IF AGC value.
// 2. Determine signal strength according to FSM stage and BER.
//
// Note: Signal strength = {0, 10 ~ 100}.
//
int
rtl2830_GetSignalStrength(
	DEMOD_MODULE *pDemod,
	unsigned char *pSignalStrength
	)
{
	unsigned char FsmStage;
	int IfAgc;



	// Get FSM stage and IF AGC value.
	if(pDemod->GetFsmStage(pDemod, &FsmStage) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetIfAgc(pDemod, &IfAgc) == FUNCTION_ERROR)
		goto error_status_get_registers;


	//  Determine signal strength according to FSM stage and IF AGC value.
	//
	if(FsmStage > 9)
		*pSignalStrength = (unsigned char)(55 - IfAgc / 182);
	else
		*pSignalStrength = 0;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod signal quality.
//
// 1. Get FSM stage and BER.
// 2. Determine signal quality according to FSM stage and BER.
//
// Note: Signal quality = {0, 10 ~ 100}.
//
int
rtl2830_GetSignalQuality(
	DEMOD_MODULE *pDemod,
	unsigned char *pSignalQuality
	)
{
	unsigned char FsmStage;
	unsigned long BerNum, BerDen;



	// Get FSM stage and BER.
	if(pDemod->GetFsmStage(pDemod, &FsmStage) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetBer(pDemod, &BerNum, &BerDen) == FUNCTION_ERROR)
		goto error_status_get_registers;


	//  Determine signal quality according to FSM stage and BER.
	//
	if(FsmStage > 9)
		*pSignalQuality = (unsigned char)(100 - Log10Apx(BerNum) / 475);
	else
		*pSignalQuality = 0;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod SNR estimation.
//
// 1. Get FSM stage, EST_EVM, constellation, and hierarchy.
// 2. If FSM stage < 10, set EST_EVM with max value.
// 3. Calculate SNR estimation value according to ConstTable and above value.
//
// Note: EST_EVM = 0 when FSM stage < 10.
//
int
rtl2830_GetSnr(
	DEMOD_MODULE *pDemod,
	long *pSnrNum,
	long *pSnrDen
	)
{
	unsigned char FsmStage;
	unsigned int EstEvm;
	unsigned char Constellation, Hierarchy;

	// Note: The unit of ConstTable elements is 0.0001.
	//
	const long ConstTable[CONSTELLATION_NUM][HIERARCHY_NUM] =
	{
		{42144,		42144,		42144,		42144},
		{49134,		49134,		52144,		56294},
		{55366,		55366,		56915,		59468},
	};



	// Get FSM stage, EST_EVM, constellation, and hierarchy.
	//
	if(pDemod->GetFsmStage(pDemod, &FsmStage) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetEstEvm(pDemod, &EstEvm) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetConstellation(pDemod, &Constellation) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetHierarchy(pDemod, &Hierarchy) == FUNCTION_ERROR)
		goto error_status_get_registers;


	// If FSM stage < 10, set EST_EVM with max value.
	//
	if(FsmStage < 10)
		EstEvm = EST_EVM_MAX_VALUE;


	// Calculate SNR estimation value according to ConstTable and above value.
	//
	if(EstEvm == 0)
		*pSnrNum = SNR_NUM_MAX_VALUE;
	else
		*pSnrNum = (ConstTable[Constellation][Hierarchy] - Log10Apx(EstEvm)) / 100;
	
	*pSnrDen = SNR_DEN_VALUE;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod BER estimation.
//
// 1. Get FSM stage and RSD_BER_EST.
// 2. Determine BER estimation value according to FSM stage and RSD_BER_EST.
//
// Note: RSD_BER_EST = 0 when FSM stage < 10.
//
int
rtl2830_GetBer(
	DEMOD_MODULE *pDemod,
	unsigned long *pBerNum,
	unsigned long *pBerDen
	)
{
	unsigned char FsmStage;
	unsigned int RsdBerEst;



	// Get FSM stage and RSD_BER_EST.
	//
	if(pDemod->GetFsmStage(pDemod, &FsmStage) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetRsdBerEst(pDemod, &RsdBerEst) == FUNCTION_ERROR)
		goto error_status_get_registers;


	// Determine BER estimation value according to FSM stage and RSD_BER_EST.
	//
	if(FsmStage < 10)
		*pBerNum = RSD_BER_EST_MAX_VALUE;
	else
		*pBerNum = RsdBerEst;

	*pBerDen = BER_DEN_VALUE;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod EST_EVM register.
//
// 1. Get EST_EVM to EstEvm.
//
int
rtl2830_GetEstEvm(
	DEMOD_MODULE *pDemod,
	unsigned int *pEstEvm
	)
{
	unsigned long Value;



	// Get EST_EVM to EstEvm.
	//
	if(pDemod->GetRegBits(pDemod, EST_EVM, &Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	*pEstEvm = (unsigned int)Value;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod RSD_BER_EST register.
//
// 1. Get RSD_BER_EST to RsdBerEst.
//
int
rtl2830_GetRsdBerEst(
	DEMOD_MODULE *pDemod,
	unsigned int *pRsdBerEst
	)
{
	unsigned long Value;



	// Get RSD_BER_EST to RsdBerEst.
	//
	if(pDemod->GetRegBits(pDemod, RSD_BER_EST, &Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	*pRsdBerEst = (unsigned int)Value;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod RF AGC value.
//
// 1. Get RF_AGC_VAL to Value.
// 2. Convert Value to signed integer and store the signed integer to RfAgc.
//
int
rtl2830_GetRfAgc(
	DEMOD_MODULE *pDemod,
	int *pRfAgc
	)
{
	unsigned long Value;



	// Get RF_AGC_VAL to Value.
	//
	if(pDemod->GetRegBits(pDemod, RF_AGC_VAL, &Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;


	// Convert Value to signed integer and store the signed integer to RfAgc.
	//
	*pRfAgc = (int)BinToSignedInt(Value, RF_AGC_REG_BIT_NUM);


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod IF AGC value.
//
// 1. Get IF_AGC_VAL to Value.
// 2. Convert Value to signed integer and store the signed integer to IfAgc.
//
int
rtl2830_GetIfAgc(
	DEMOD_MODULE *pDemod,
	int *pIfAgc
	)
{
	unsigned long Value;



	// Get IF_AGC_VAL to Value.
	//
	if(pDemod->GetRegBits(pDemod, IF_AGC_VAL, &Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;


	// Convert Value to signed integer and store the signed integer to IfAgc.
	//
	*pIfAgc = (int)BinToSignedInt(Value, IF_AGC_REG_BIT_NUM);


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod digital AGC value
//
// 1. Get DAGC_VAL to DiAgc.
//
int
rtl2830_GetDiAgc(
	DEMOD_MODULE *pDemod,
	unsigned char *pDiAgc
	)
{
	unsigned long Value;



	// Get DAGC_VAL to DiAgc.
	//
	if(pDemod->GetRegBits(pDemod, DAGC_VAL, &Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	*pDiAgc = (unsigned char)Value;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod constellation information from TPS.
//
// 1. Get TPS constellation information from RX_CONSTEL.
//
int
rtl2830_GetConstellation(
	DEMOD_MODULE *pDemod,
	unsigned char *pConstellation
	)
{
	const unsigned char ConstellationTable[CONSTELLATION_NUM] =
	{
		CONSTELLATION_QPSK,
		CONSTELLATION_16QAM,
		CONSTELLATION_64QAM,
	};

	unsigned long ReadingValue;


	// Get TPS constellation information from RX_CONSTEL.
	//
	// Note: Limit ReadingValue maximum to be (CONSTELLATION_NUM - 1).
	//
	if(pDemod->GetRegBits(pDemod, RX_CONSTEL, &ReadingValue) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(ReadingValue > (CONSTELLATION_NUM - 1))
		ReadingValue = CONSTELLATION_NUM - 1;

	*pConstellation = ConstellationTable[ReadingValue];


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod hierarchy information from TPS.
//
// 1. Get TPS hierarchy infromation from RX_HIER.
//
int
rtl2830_GetHierarchy(
	DEMOD_MODULE *pDemod,
	unsigned char *pHierarchy
	)
{
	const unsigned char HierarchyTable[HIERARCHY_NUM] =
	{
#if 0	    
		HIERARCHY_NONE,
#else
        HIERARCHY_NONE_0,
#endif		
		HIERARCHY_ALPHA_1,
		HIERARCHY_ALPHA_2,
		HIERARCHY_ALPHA_4,
	};

	unsigned long ReadingValue;


	// Get TPS hierarchy infromation from RX_HIER.
	//
	// Note: Limit ReadingValue maximum to be (HIERARCHY_NUM - 1).
	//
	if(pDemod->GetRegBits(pDemod, RX_HIER, &ReadingValue) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(ReadingValue > (HIERARCHY_NUM - 1))
		ReadingValue = HIERARCHY_NUM - 1;

	*pHierarchy = HierarchyTable[ReadingValue];


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod low-priority code rate information from TPS.
//
// 1. Get TPS low-priority code rate infromation from RX_C_RATE_LP.
//
int
rtl2830_GetCodeRateLp(
	DEMOD_MODULE *pDemod,
	unsigned char *pCodeRateLp
	)
{
	const unsigned char CodeRateTable[CODE_RATE_NUM] =
	{
		CODE_RATE_1_OVER_2,
		CODE_RATE_2_OVER_3,
		CODE_RATE_3_OVER_4,
		CODE_RATE_5_OVER_6,
		CODE_RATE_7_OVER_8,
	};

	unsigned long ReadingValue;


	// Get TPS low-priority code rate infromation from RX_C_RATE_LP.
	//
	// Note: Limit ReadingValue maximum to be (CODE_RATE_NUM - 1).
	//
	if(pDemod->GetRegBits(pDemod, RX_C_RATE_LP, &ReadingValue) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(ReadingValue > (CODE_RATE_NUM - 1))
		ReadingValue = CODE_RATE_NUM - 1;

	*pCodeRateLp = CodeRateTable[ReadingValue];


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod high-priority code rate information from TPS.
//
// 1. Get TPS high-priority code rate infromation from RX_C_RATE_HP.
//
int
rtl2830_GetCodeRateHp(
	DEMOD_MODULE *pDemod,
	unsigned char *pCodeRateHp
	)
{
	const unsigned char CodeRateTable[CODE_RATE_NUM] =
	{
		CODE_RATE_1_OVER_2,
		CODE_RATE_2_OVER_3,
		CODE_RATE_3_OVER_4,
		CODE_RATE_5_OVER_6,
		CODE_RATE_7_OVER_8,
	};

	unsigned long ReadingValue;


	// Get TPS high-priority code rate infromation from RX_C_RATE_HP.
	//
	// Note: Limit ReadingValue maximum to be (CODE_RATE_NUM - 1).
	//
	if(pDemod->GetRegBits(pDemod, RX_C_RATE_HP, &ReadingValue) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(ReadingValue > (CODE_RATE_NUM - 1))
		ReadingValue = CODE_RATE_NUM - 1;

	*pCodeRateHp = CodeRateTable[ReadingValue];


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod guard interval information from TPS.
//
// 1. Get TPS guard interval infromation from GI_IDX.
//
int
rtl2830_GetGuardInterval(
	DEMOD_MODULE *pDemod,
	unsigned char *pGuardInterval
	)
{
	const unsigned char GuardItervalTable[GUARD_INTERVAL_NUM] =
	{
		GUARD_INTERVAL_1_OVER_32,
		GUARD_INTERVAL_1_OVER_16,
		GUARD_INTERVAL_1_OVER_8,
		GUARD_INTERVAL_1_OVER_4,
	};

	unsigned long ReadingValue;


	// Get TPS guard interval infromation from GI_IDX.
	//
	// Note: Limit ReadingValue maximum to be (GUARD_INTERVAL_NUM - 1).
	//
	if(pDemod->GetRegBits(pDemod, GI_IDX, &ReadingValue) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(ReadingValue > (GUARD_INTERVAL_NUM - 1))
		ReadingValue = GUARD_INTERVAL_NUM - 1;

	*pGuardInterval = GuardItervalTable[ReadingValue];


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get demod FFT mode information from TPS.
//
// 1. Get TPS FFT mode infromation from FFT_MODE_IDX.
//
int
rtl2830_GetFftMode(
	DEMOD_MODULE *pDemod,
	unsigned char *pFftMode
	)
{
	const unsigned char FftModeTable[FFT_MODE_NUM] =
	{
		FFT_MODE_2K,
		FFT_MODE_8K,
	};

	unsigned long ReadingValue;


	// Get TPS FFT mode infromation from FFT_MODE_IDX.
	//
	// Note: Limit ReadingValue maximum to be (FFT_MODE_NUM - 1).
	//
	if(pDemod->GetRegBits(pDemod, FFT_MODE_IDX, &ReadingValue) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(ReadingValue > (FFT_MODE_NUM - 1))
		ReadingValue = FFT_MODE_NUM - 1;

	*pFftMode = FftModeTable[ReadingValue];


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}





// Update demod registers with Function 2.
//
// 1. Get EST_EVM, constellation, and high-priority code rate.
// 2. Get SelectIndex according to EST_EVM, constellation, and high-priority code rate.
// 3. Get UpdateReg1Addr according to constellation and high-priority code rate.
// 4. Get UpdateValue1 and UpdateValue2 according to constellation, high-priority code rate
//    and SelectIndex.
// 5. Set UpdateReg1Addr with UpdateValue1, and set UPDATE_REG_2 with UpdateValue2.
//
int
rtl2830_UpdateFunction2(
	DEMOD_MODULE *pDemod
	)
{
	const unsigned int EvmTable[CONSTELLATION_NUM][CODE_RATE_NUM][SELECT_INDEX_NUM - 1] =
	{
		// Constellation QPSK
		{
			// Code rate 1/2
			{	60,		107,	268,	451,	900,	1796,	3584	},

			// Code rate 2/3
			{	82,		163,	250,	354,	594,	1186,	2368	},

			// Code rate 3/4
			{	65,		163,	259,	397,	561,	942,	1881	},

			// Code rate 5/6
			{	65,		103,	158,	223,	375,	748,	1494	},

			// Code rate 7/8
			{	51,		130,	185,	262,	371,	622,	1242	},
		},

		// Constellation 16 QAM
		{
			// Code rate 1/2
			{	205,	326,	516,	730,	1211,	2417,	4299	},

			// Code rate 2/3
			{	103,	205,	326,	516,	819,	1298,	2906	},

			// Code rate 3/4
			{	81,		205,	300,	425,	600,	1007,	2010	},

			// Code rate 5/6
			{	81,		163,	238,	337,	476,	800,	1597	},

			// Code rate 7/8
			{	25,		129,	259,	434,	614,	867,	1456	},
		},

		// Constellation 64 QAM
		{
			// Code rate 1/2
			{	172,	344,	577,	815,	1369,	2670,	4331	},

			// Code rate 2/3
			{	136,	344,	491,	694,	980,	1646,	3066	},

			// Code rate 3/4
			{	136,	344,	491,	694,	980,	1385,	2326	},

			// Code rate 5/6
			{	86,		172,	252,	356,	503,	844,	1685	},

			// Code rate 7/8
			{	86,		172,	289,	408,	577,	815,	1369	},
		},
	};


	const unsigned char UpdateReg1AddrTable[CONSTELLATION_NUM][CODE_RATE_NUM] =
	{
		// Constellation QPSK
		{	0xd6,	0xd7,	0xd8,	0xd9,	0xda	},

		// Constellation 16 QAM
		{	0xdb,	0xdc,	0xdd,	0xde,	0xdf	},

		// Constellation 64 QAM
		{	0xe0,	0xe1,	0xe2,	0xe3,	0xe4	},
	};


	const unsigned char UpdateValue1Table
		[CONSTELLATION_NUM][CODE_RATE_NUM][SELECT_INDEX_NUM] =
	{
		// Constellation QPSK
		{
			// Code rate 1/2
			{	127,	96,		64,		48,		32,		16,		8,		4	},

			// Code rate 2/3
			{	127,	96,		64,		48,		32,		16,		8,		4	},

			// Code rate 3/4
			{	127,	127,	96,		64,		48,		32,		16,		8	},

			// Code rate 5/6
			{	127,	96,		64,		48,		32,		16,		8,		4	},

			// Code rate 7/8
			{	127,	127,	96,		64,		48,		32,		16,		8	},
		},

		// Constellation 16 QAM
		{
			// Code rate 1/2
			{	127,	96,		64,		48,		32,		16,		8,		4	},

			// Code rate 2/3
			{	127,	127,	96,		64,		48,		32,		16,		8	},

			// Code rate 3/4
			{	127,	127,	96,		64,		48,		32,		16,		8	},

			// Code rate 5/6
			{	127,	127,	96,		64,		48,		32,		16,		8	},

			// Code rate 7/8
			{	127,	127,	127,	96,		64,		48,		32,		16	},
		},

		// Constellation 64 QAM
		{
			// Code rate 1/2
			{	127,	96,		64,		48,		32,		16,		8,		4	},

			// Code rate 2/3
			{	127,	127,	96,		64,		48,		32,		16,		8	},

			// Code rate 3/4
			{	127,	127,	127,	96,		64,		48,		32,		16	},

			// Code rate 5/6
			{	127,	127,	127,	96,		64,		48,		32,		16	},

			// Code rate 7/8
			{	127,	127,	127,	127,	96,		64,		48,		32	},
		},
	};

	
	const unsigned char UpdateValue2Table
		[CONSTELLATION_NUM][CODE_RATE_NUM][SELECT_INDEX_NUM] =
	{
		// Constellation QPSK
		{
			// Code rate 1/2
			{	0,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 2/3
			{	0,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 3/4
			{	1,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 5/6
			{	0,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 7/8
			{	1,	0,	0,	0,	0,	0,	0,	0	},
		},

		// Constellation 16 QAM
		{
			// Code rate 1/2
			{	0,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 2/3
			{	1,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 3/4
			{	1,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 5/6
			{	1,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 7/8
			{	1,	1,	0,	0,	0,	0,	0,	0	},
		},

		// Constellation 64 QAM
		{
			// Code rate 1/2
			{	0,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 2/3
			{	1,	0,	0,	0,	0,	0,	0,	0	},

			// Code rate 3/4
			{	1,	1,	0,	0,	0,	0,	0,	0	},

			// Code rate 5/6
			{	1,	1,	0,	0,	0,	0,	0,	0	},

			// Code rate 7/8
			{	1,	1,	1,	0,	0,	0,	0,	0	},
		},
	};


	unsigned int EstEvm;
	unsigned char Constellation, CodeRateHp;
	unsigned char SelectIndex;
	unsigned char UpdateReg1Addr, UpdateValue1, UpdateValue2;



	// If Function 2 executing flag is off, leave the Function 2.
	//
	if(pDemod->Func2Executing == OFF)
		goto success_status;


	// Get EST_EVM, constellation, and high-priority code rate.
	//
	if(pDemod->GetEstEvm(pDemod, &EstEvm) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetConstellation(pDemod, &Constellation) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetCodeRateHp(pDemod, &CodeRateHp) == FUNCTION_ERROR)
		goto error_status_get_registers;


	// Get SelectIndex according to EST_EVM, constellation, and high-priority code rate.
	//
	for(SelectIndex = 0; SelectIndex < SELECT_INDEX_NUM - 1; SelectIndex++)
	{
		if(EstEvm < EvmTable[Constellation][CodeRateHp][SelectIndex])
			break;
	}


	// Get UpdateReg1Addr according to constellation and high-priority code rate.
	//
	UpdateReg1Addr = UpdateReg1AddrTable[Constellation][CodeRateHp];


	// Get UpdateValue1 and UpdateValue2 according to constellation, high-priority code rate
	//    and SelectIndex.
	//
	UpdateValue1 = UpdateValue1Table[Constellation][CodeRateHp][SelectIndex];
	UpdateValue2 = UpdateValue2Table[Constellation][CodeRateHp][SelectIndex];


	// Set UpdateReg1Addr with UpdateValue1, and set UPDATE_REG_2 with UpdateValue2.
	//
	if(pDemod->SetRegBytes(pDemod, UPDATE_REG_1_PAGE, UpdateReg1Addr, &UpdateValue1,
		LEN_1_BYTE) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, UPDATE_REG_2, UpdateValue2) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


success_status:
	return FUNCTION_SUCCESS;


error_status_get_registers:
error_status_set_registers:
	return FUNCTION_ERROR;
}





// Update demod registers with Function 3.
//
// 1. Get FSM stage, RSD_BER_EST, FFT mode, and guard interval.
// 2. If FFT mode is 2k and guard interval is 1/32, 1/16, and 1/8, assign P3Value and D3Value directly.
//    Set NewFunc3State with zero.
//    Otherwise, set P3Value and D3Value in the following steps.
//    a. Get NewFunc3State acording to BER threshold table, RSD_BER_EST, and FSM stage.
//    b. Get P3Value and D3Value according to NewFunc3State.
// 4. Set BTHD_P3 with P3Value, and set BTHD_D3 with D3Value.
// 5. Set Func3State with NewFunc3State.
//
int
rtl2830_UpdateFunction3(
	DEMOD_MODULE *pDemod
	)
{
	const unsigned int BerThresholdTable[FUNC3_STATE_NUM] =
	{
		18000,	8000,	6000,	5000,	5000,	3000,
	};


	const unsigned char P3ValueTable[FUNC3_STATE_NUM] =
	{
		1,	1,	1,	2,	4,	5,
	};


	const unsigned char D3ValueTable[FUNC3_STATE_NUM] =
	{
		1,	2,	2,	3,	3,	3,
	};


	unsigned char FsmStage;
	unsigned int RsdBerEst;
	unsigned char FftMode;
	unsigned char GuardInterval;
	int OldFunc3State, NewFunc3State;
	unsigned char P3Value, D3Value;



	// If Function 3 executing flag is off, leave the Function 3.
	//
	if(pDemod->Func3Executing == OFF)
		goto success_status;


	// Get FSM stage, RSD_BER_EST, FFT mode, and guard interval.
	//
	if(pDemod->GetFsmStage(pDemod, &FsmStage) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetRsdBerEst(pDemod, &RsdBerEst) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetFftMode(pDemod, &FftMode) == FUNCTION_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetGuardInterval(pDemod, &GuardInterval) == FUNCTION_ERROR)
		goto error_status_get_registers;


	// If FFT mode is 2k and guard interval is 1/32, 1/16, and 1/8, assign P3Value and D3Value directly.
	// Set NewFunc3State with zero.
	// Otherwise, set P3Value and D3Value in the following steps.
	if((FftMode == FFT_MODE_2K) && (GuardInterval < GUARD_INTERVAL_1_OVER_4))
	{
		P3Value = ASSIGNED_P3VALUE;
		D3Value = ASSIGNED_D3VALUE;
		NewFunc3State = 0;
	}
	else
	{
		// Get NewFunc3State acording to BER threshold table, RSD_BER_EST, and FSM stage.
		//
		// Note: Need to limit NewFunc3State range to be 0 ~ 5.
		//
		OldFunc3State = pDemod->Func3State;

		if(FsmStage < 10)
			NewFunc3State = 0;
		else
		{
			if(RsdBerEst < BerThresholdTable[OldFunc3State])
				NewFunc3State = OldFunc3State + 1;
			else
				NewFunc3State = OldFunc3State - 1;
		}

		if(NewFunc3State < 0)
			NewFunc3State = 0;

		if(NewFunc3State > 5)
			NewFunc3State = 5;


		// Get P3Value and D3Value according to NewFunc3State.
		//
		P3Value = P3ValueTable[NewFunc3State];
		D3Value = D3ValueTable[NewFunc3State];
	}


	// Set BTHD_P3 with P3Value, and set BTHD_D3 with D3Value.
	//
	if(pDemod->SetRegBits(pDemod, BTHD_P3, P3Value) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, BTHD_D3, D3Value) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	// Set Func3State with NewFunc3State.
	//
	pDemod->Func3State = NewFunc3State;


success_status:
	return FUNCTION_SUCCESS;


error_status_get_registers:
error_status_set_registers:
	return FUNCTION_ERROR;
}





// Reset Function 4 variables and registers.
//
// Note: Need to execute function 4 reset function when change tuner RF frequency or demod parameters.
//       Function 4 update flow also employs function 4 reset function.
//
int
rtl2830_ResetFunction4(
	DEMOD_MODULE *pDemod
	)
{
	// Reset Function 4 state.
	// Note: Set Function 4 state with 0 for the tuner RF frequency or demod parameters changing condition.
	//
	pDemod->Func4State = 0;


	// Reset Function 4 variables.
	//
	if(pDemod->Func4State != 0)
		pDemod->Func4DelayCnt = 0;

	pDemod->Func4ParamSetting = NO;
	pDemod->Func3Executing    = ON;


	// Reset Function 4 registers.
	//
	if(pDemod->SetRegBits(pDemod, FUNC4_REG0, 0x3) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, FUNC4_REG9, 0x1) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[0], FUNC4_PARAM_LEN) ==
		REG_ACCESS_ERROR)
		goto error_status_set_registers;


	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Update demod registers with Function 4.
//
int
rtl2830_UpdateFunction4(
	DEMOD_MODULE *pDemod
	)
{
	int i;

	unsigned long Reg3Value, Reg4Value, Reg5Value, Reg6Value, Reg7Value, Reg8Value;

	unsigned char DiAgc;
	unsigned char FsmStage;
	unsigned char Constellation;
	unsigned char FftMode;
	unsigned char GuardInterval;
	unsigned char CodeRateHp;
	unsigned int  RsdBerEst;

	int Flag1, Flag2, Flag3, Flag4, Flag5;

	unsigned long RegValueThd;
	int RegValueMinIndex;
	unsigned long RegValueMin;



	// Get demod registers for Function 4 reset condition check.
	//
	if(pDemod->GetRegBits(pDemod, FUNC4_REG3, &Reg3Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetRegBits(pDemod, FUNC4_REG4, &Reg4Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetRegBits(pDemod, FUNC4_REG5, &Reg5Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetRegBits(pDemod, FUNC4_REG6, &Reg6Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if((Reg3Value == 1) || (Reg4Value == 1) || (Reg5Value == 0) || (Reg6Value == 0))
	{
		if(pDemod->ResetFunction4(pDemod) == FUNCTION_ERROR)
			goto error_status_execute_function;

		goto success_status;
	}

	
	// Get demod parameters for Function 4.
	//
	if(pDemod->GetRegBits(pDemod, FUNC4_REG7, &Reg7Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetRegBits(pDemod, FUNC4_REG8, &Reg8Value) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	if(pDemod->GetDiAgc(pDemod, &DiAgc) == FUNCTION_ERROR)
		goto error_status_execute_function;

	if(pDemod->GetFsmStage(pDemod, &FsmStage) == FUNCTION_ERROR)
		goto error_status_execute_function;

	if(pDemod->GetConstellation(pDemod, &Constellation) == FUNCTION_ERROR)
		goto error_status_execute_function;

	if(pDemod->GetFftMode(pDemod, &FftMode) == FUNCTION_ERROR)
		goto error_status_execute_function;

	if(pDemod->GetGuardInterval(pDemod, &GuardInterval) == FUNCTION_ERROR)
		goto error_status_execute_function;

	if(pDemod->GetCodeRateHp(pDemod, &CodeRateHp) == FUNCTION_ERROR)
		goto error_status_execute_function;

	if(pDemod->GetRsdBerEst(pDemod, &RsdBerEst) == FUNCTION_ERROR)
		goto error_status_execute_function;


	// Determine flags.
	//
	Flag1 = (pDemod->Func4DelayCnt == pDemod->Func4DelayCntMax) && (pDemod->Func3State == 0) && (Reg7Value < 11)
		    && (DiAgc > 4) && (DiAgc < 64) && (FsmStage > 8) && (Constellation == CONSTELLATION_64QAM)
			&& (FftMode == FFT_MODE_8K) && (Reg8Value == 0)
			&& ((GuardInterval == GUARD_INTERVAL_1_OVER_8) || (GuardInterval == GUARD_INTERVAL_1_OVER_4))
			&& ((CodeRateHp == CODE_RATE_2_OVER_3) || (CodeRateHp == CODE_RATE_3_OVER_4));

	Flag2 = (pDemod->Func4DelayCnt < pDemod->Func4DelayCntMax) && (pDemod->Func4ParamSetting == YES) && (FsmStage < 11);
	Flag3 = FsmStage > 9;
	Flag4 = RsdBerEst < 3000;
	Flag5 = pDemod->Func4DelayCnt == pDemod->Func4DelayCntMax;


	// Run FSM.
	switch(pDemod->Func4State)
	{
		case 0:

			if(Flag1 == 1)
			{
				if(pDemod->SetRegBits(pDemod, FUNC4_REG0, 0x0) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				pDemod->Func4State = 2;
			}
			else if(Flag2 == 1)
			{
				if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[0], FUNC4_PARAM_LEN)
					== REG_ACCESS_ERROR)
					goto error_status_set_registers;

				pDemod->Func4ParamSetting = NO;

				pDemod->Func4State = 1;
			}
			else
			{
				pDemod->Func4DelayCnt = (pDemod->Func4DelayCnt == pDemod->Func4DelayCntMax) ?
					                     0 : (pDemod->Func4DelayCnt + 1);

				pDemod->Func4State = 0;
			}

			break;


		case 1:

			pDemod->Func4State = 12;

			break;


		case 2:

			if(Flag3 == 1)
			{
				if(pDemod->SetRegBits(pDemod, FUNC4_REG1, 0x1) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC4_REG2, 0x2a) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				pDemod->Func4DelayCnt = 0;
				pDemod->Func3Executing = OFF;

				pDemod->Func4State = 3;
			}
			else
			{
				pDemod->Func4State = 2;
			}

			break;


		case 3:

			if(Flag4 == 1)
			{
				if(pDemod->SetRegBits(pDemod, FUNC4_REG0, 0x3) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				pDemod->Func4DelayCnt = 0;
				pDemod->Func3Executing = ON;

				pDemod->Func4State = 0;
			}
			else if(Flag5 == 1)
			{
				if(pDemod->SetRegBits(pDemod, FUNC4_REG9, 0x0) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[0], FUNC4_PARAM_LEN)
					== REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(rtl2830_GetFunc4RegValueAvg(pDemod, &pDemod->Func4RegValue[0]) == FUNCTION_ERROR)
					goto error_status_execute_function;

				pDemod->Func4State = 4;
			}
			else
			{
				pDemod->Func4DelayCnt = (pDemod->Func4DelayCnt == pDemod->Func4DelayCntMax) ?
					                     0 : (pDemod->Func4DelayCnt + 1);

				pDemod->Func4State = 3;
			}

			break;


		case 4:

			if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[1], FUNC4_PARAM_LEN) ==
				REG_ACCESS_ERROR)
				goto error_status_set_registers;

			if(rtl2830_GetFunc4RegValueAvg(pDemod, &pDemod->Func4RegValue[1]) == FUNCTION_ERROR)
				goto error_status_execute_function;

			pDemod->Func4State = 5;

			break;


		case 5:

			if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[2], FUNC4_PARAM_LEN) ==
				REG_ACCESS_ERROR)
				goto error_status_set_registers;

			if(rtl2830_GetFunc4RegValueAvg(pDemod, &pDemod->Func4RegValue[2]) == FUNCTION_ERROR)
				goto error_status_execute_function;

			pDemod->Func4State = 6;

			break;


		case 6:

			if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[3], FUNC4_PARAM_LEN) ==
				REG_ACCESS_ERROR)
				goto error_status_set_registers;

			if(rtl2830_GetFunc4RegValueAvg(pDemod, &pDemod->Func4RegValue[3]) == FUNCTION_ERROR)
				goto error_status_execute_function;

			pDemod->Func4State = 7;

			break;


		case 7:

			if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[4], FUNC4_PARAM_LEN) ==
				REG_ACCESS_ERROR)
				goto error_status_set_registers;

			if(rtl2830_GetFunc4RegValueAvg(pDemod, &pDemod->Func4RegValue[4]) == FUNCTION_ERROR)
				goto error_status_execute_function;

			pDemod->Func4State = 8;

			break;


		case 8:

			for(RegValueMinIndex = 0, i = 1; i < FUNC4_REG_VALUE_NUM; i++)
			{
				if(pDemod->Func4RegValue[i] < pDemod->Func4RegValue[RegValueMinIndex])
					RegValueMinIndex = i;
			}

			RegValueMin = pDemod->Func4RegValue[RegValueMinIndex];
			RegValueThd = RegValueThdTable[CodeRateHp];

			if( ((pDemod->Func4RegValue[1] * 8) < (RegValueMin * 9)) || (pDemod->Func4RegValue[1] < RegValueThd) )
			{
				if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[1], FUNC4_PARAM_LEN) ==
					REG_ACCESS_ERROR)
					goto error_status_set_registers;
			}
			else if( ((pDemod->Func4RegValue[0] * 8) < (RegValueMin * 9)) || (pDemod->Func4RegValue[0] < RegValueThd) )
			{
				if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[0], FUNC4_PARAM_LEN) ==
					REG_ACCESS_ERROR)
					goto error_status_set_registers;
			}
			else
			{
				if(pDemod->SetRegBytes(pDemod, FUNC4_PARAM_PAGE, FUNC4_PARAM_ADDR, Func4ParamTable[RegValueMinIndex],
					FUNC4_PARAM_LEN) == REG_ACCESS_ERROR)
					goto error_status_set_registers;
			}


			if(pDemod->SetRegBits(pDemod, FUNC4_REG9, 0x1) == REG_ACCESS_ERROR)
				goto error_status_set_registers;

			pDemod->Func4ParamSetting = YES;
			pDemod->Func3Executing    = ON;

			pDemod->Func4State = 9;

			break;


		case 9:

			pDemod->Func4State = 10;

			break;


		case 10:

			if(pDemod->SetRegBits(pDemod, FUNC4_REG0, 0x3) == REG_ACCESS_ERROR)
				goto error_status_set_registers;

			pDemod->Func4State = 11;

			break;


		case 11:

			pDemod->Func4State = 0;

			break;


		case 12:

			pDemod->Func4State = 0;

			break;


		default:

			pDemod->Func4State = 0;

	}


success_status:
	return FUNCTION_SUCCESS;


error_status_set_registers:
error_status_execute_function:
error_status_get_registers:
	return FUNCTION_ERROR;
}





// Reset Function 5 variables and registers.
//
// Note: Need to execute function 5 reset function when change tuner RF frequency or demod parameters.
//       Function 5 update flow also employs function 5 reset function.
//
int
rtl2830_ResetFunction5(
	DEMOD_MODULE *pDemod
	)
{
	// Reset variables and demod registers for Function 5.
	pDemod->Func2Executing = ON;
	pDemod->Func5State = 0;

	if(pDemod->SetRegBits(pDemod, CCI_THRE, 0x3) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, FUNC5_REG5, 0x3) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, CCI_M0, 0x3) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, CCI_M1, 0x3) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, CCI_M2, 0x3) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, CCI_M3, 0x3) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, FUNC5_REG6, 0xf) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, FUNC5_REG7, 0x1) == REG_ACCESS_ERROR)
		goto error_status_set_registers;

	if(pDemod->SetRegBits(pDemod, FUNC5_REG8, 0x1) == REG_ACCESS_ERROR)
		goto error_status_set_registers;


	return FUNCTION_SUCCESS;


error_status_set_registers:
	return FUNCTION_ERROR;
}





// Update demod registers with Function 5.
//
int
rtl2830_UpdateFunction5(
	DEMOD_MODULE *pDemod
	)
{
	int Flag1, Flag2, Flag3, Flag4, Flag5, Flag6, Flag7, Flag8;

	unsigned char FsmStage;

	unsigned long RegStatus;
	unsigned long RegStatusBit0, RegStatusBit1, RegStatusBit2, RegStatusBit3;
	unsigned long Reg0, Reg1, Reg2, Reg3;

	unsigned char Qam;
	unsigned char Hier;
	unsigned char HpCr;
	unsigned char LpCr;
	unsigned char Gi;
	unsigned char Fft;


	// Read demod registers.

	// Read FSM stage.
	if(pDemod->GetFsmStage(pDemod, &FsmStage) == FUNCTION_ERROR)
		goto error_status_execute_function;

	// Read QAM.
	if(pDemod->GetConstellation(pDemod, &Qam) == FUNCTION_ERROR)
		goto error_status_execute_function;

	// Read hierarchy.
	if(pDemod->GetHierarchy(pDemod, &Hier) == FUNCTION_ERROR)
		goto error_status_execute_function;

	// Read low-priority code rate.
	if(pDemod->GetCodeRateLp(pDemod, &LpCr) == FUNCTION_ERROR)
		goto error_status_execute_function;

	// Read high-priority code rate.
	if(pDemod->GetCodeRateHp(pDemod, &HpCr) == FUNCTION_ERROR)
		goto error_status_execute_function;

	// Read guard interval.
	if(pDemod->GetGuardInterval(pDemod, &Gi) == FUNCTION_ERROR)
		goto error_status_execute_function;

	// Read FFT mode.
	if(pDemod->GetFftMode(pDemod, &Fft) == FUNCTION_ERROR)
		goto error_status_execute_function;

	// Read RegStatus.
	if(pDemod->GetRegBits(pDemod, FUNC5_REG0, &RegStatus) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	RegStatusBit0 = (RegStatus & 0x1) >> 0;
	RegStatusBit1 = (RegStatus & 0x2) >> 1;
	RegStatusBit2 = (RegStatus & 0x4) >> 2;
	RegStatusBit3 = (RegStatus & 0x8) >> 3;

	// Read Reg0.
	if(pDemod->GetRegBits(pDemod, FUNC5_REG1, &Reg0) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	// Read Reg1.
	if(pDemod->GetRegBits(pDemod, FUNC5_REG2, &Reg1) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	// Read Reg2.
	if(pDemod->GetRegBits(pDemod, FUNC5_REG3, &Reg2) == REG_ACCESS_ERROR)
		goto error_status_get_registers;

	// Read Reg3.
	if(pDemod->GetRegBits(pDemod, FUNC5_REG4, &Reg3) == REG_ACCESS_ERROR)
		goto error_status_get_registers;


	// Determine flags.
	Flag1 = (FsmStage > 9) && (Qam == pDemod->Func5QamBak) && (Hier == pDemod->Func5HierBak) &&
		    (LpCr == pDemod->Func5LpCrBak) && (HpCr == pDemod->Func5HpCrBak) && (Gi == pDemod->Func5GiBak) &&
			(Fft == pDemod->Func5FftBak) && (RegStatusBit0 == 1);
	Flag2 = (Fft == FFT_MODE_2K) && ((Reg0 > 1432 ? Reg0 - 1432 : 1432 - Reg0) < 25);
	Flag3 = (Fft == FFT_MODE_8K) && ((Reg0 > 5736 ? Reg0 - 5736 : 5736 - Reg0) < 100);
	Flag4 = (( (RegStatusBit1 == 1) && ((Reg1 > 720 ? Reg1 - 720 : 720 - Reg1) < 25) ) ||
			 ( (RegStatusBit2 == 1) && ((Reg2 > 720 ? Reg2 - 720 : 720 - Reg2) < 25) ) ||
			 ( (RegStatusBit3 == 1) && ((Reg3 > 720 ? Reg3 - 720 : 720 - Reg3) < 25) )    ) &&
		    (Qam == CONSTELLATION_16QAM) && (HpCr == CODE_RATE_3_OVER_4);
	Flag5 = ( (RegStatusBit1 == 1) && ((Reg1 > 2900 ? Reg1 - 2900 : 2900 - Reg1) < 100) ) ||
			( (RegStatusBit2 == 1) && ((Reg2 > 2900 ? Reg2 - 2900 : 2900 - Reg2) < 100) ) ||
			( (RegStatusBit3 == 1) && ((Reg3 > 2900 ? Reg3 - 2900 : 2900 - Reg3) < 100) );

	Flag6 = Flag1 && (Flag2 || Flag3);
	Flag7 = Flag1 && Flag2 && Flag4;
	Flag8 = Flag1 && Flag3 && Flag5;


	// Backup current TPS information.
	pDemod->Func5QamBak  = Qam;
	pDemod->Func5HierBak = Hier;
	pDemod->Func5LpCrBak = LpCr;
	pDemod->Func5HpCrBak = HpCr;
	pDemod->Func5GiBak   = Gi;
	pDemod->Func5FftBak  = Fft;



	// Run CCI FSM.
	switch(pDemod->Func5State)
	{
		case 0:

			if(Flag6 == 1)
			{
				if(pDemod->SetRegBits(pDemod, CCI_THRE, 0x1) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG5, 0x1) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				pDemod->Func5State = 1;

				break;
			}

			break;


		case 1:

			if(Flag6 == 0)
			{
				if(pDemod->ResetFunction5(pDemod) == FUNCTION_ERROR)
					goto error_status_execute_function;

				break;
			}

			if(Flag7 == 1)
			{
				pDemod->Func2Executing = OFF;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG9, 0x8) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				pDemod->Func5State = 2;

				break;
			}

			if(Flag8 == 1)
			{
				if(pDemod->SetRegBits(pDemod, FUNC5_REG6, 0x7) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG7, 0x0) == REG_ACCESS_ERROR)
					goto error_status_set_registers;


				if(pDemod->SetRegBits(pDemod, CCI_M0, 7) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG10, 5736) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG13, 10087) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG16, 31176) == REG_ACCESS_ERROR)
					goto error_status_set_registers;


				if(pDemod->SetRegBits(pDemod, CCI_M1, 5) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG11, 2900) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG14, 19921) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG17, 105055) == REG_ACCESS_ERROR)
					goto error_status_set_registers;


				if(pDemod->SetRegBits(pDemod, CCI_M2, 3) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG12, 1490) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG15, 117472) == REG_ACCESS_ERROR)
					goto error_status_set_registers;

				if(pDemod->SetRegBits(pDemod, FUNC5_REG18, 101259) == REG_ACCESS_ERROR)
					goto error_status_set_registers;


				if(pDemod->SetRegBits(pDemod, FUNC5_REG8, 0x0) == REG_ACCESS_ERROR)
					goto error_status_set_registers;


				pDemod->Func5State = 3;

				break;
			}

			break;


		case 2:

			if(Flag7 == 0)
			{
				if(pDemod->ResetFunction5(pDemod) == FUNCTION_ERROR)
					goto error_status_execute_function;

				break;
			}

			break;


		case 3:

			if(Flag8 == 0)
			{
				if(pDemod->ResetFunction5(pDemod) == FUNCTION_ERROR)
					goto error_status_execute_function;

				break;
			}

			break;


		default:

			if(pDemod->ResetFunction5(pDemod) == FUNCTION_ERROR)
				goto error_status_execute_function;

			break;
	}


	return FUNCTION_SUCCESS;


error_status_set_registers:
error_status_execute_function:
error_status_get_registers:
	return FUNCTION_ERROR;
}





// Get averaged register value for Function 4.
//
int
rtl2830_GetFunc4RegValueAvg(
	DEMOD_MODULE *pDemod,
	unsigned long *pRegValueAvg
	)
{
	int i;
	unsigned long RegValue;
	unsigned long RegValueSum;


	// Get averaged register value.
	//
	RegValueSum = 0;

	for(i = 0; i < FUNC4_VALUE_AVG_NUM; i++)
	{
		if(pDemod->GetRegBits(pDemod, FUNC4_REG10, &RegValue) == REG_ACCESS_ERROR)
			goto error_status_get_registers;

		RegValueSum += RegValue;
	}

	*pRegValueAvg = RegValueSum / FUNC4_VALUE_AVG_NUM;


	return FUNCTION_SUCCESS;


error_status_get_registers:
	return FUNCTION_ERROR;
}







