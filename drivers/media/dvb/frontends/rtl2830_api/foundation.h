#ifndef __FOUNDATION_H
#define __FOUNDATION_H





// Definitions

// Constants
#define INVALID_POINTER_VALUE		0
#define LEN_0_BYTE					0
#define LEN_1_BYTE					1
#define LEN_2_BYTE					2
#define LEN_4_BYTE					4


// Byte mask and shift
#define BYTE_MASK				0xff
#define BIT_0_MASK				0x01
#define BYTE_SHIFT				8
#define BYTE_BIT_NUM			8
#define HEX_DIGIT_BIT_NUM		4
#define LONG_BIT_NUM			32


// Function 4
#define FUNC4_REG_VALUE_NUM			5



/// On/off status
enum ON_OFF_STATUS
{
	OFF,		///<   Off
	ON,			///<   On
};


/// Yes/no status
enum YES_NO_STATUS
{
	NO,			///<   No
	YES,		///<   Yes
};


// Return status
enum
{
	// Return status for memory allocating function
	MEMORY_ALLOCATING_SUCCESS,
	MEMORY_ALLOCATING_ERROR,

	// Return status for I2C reading and writing functions
	I2C_SUCCESS,
	I2C_ERROR,

	// Return status for demod register reading and writing functions
	REG_ACCESS_SUCCESS,
	REG_ACCESS_ERROR,

	// Return status for demod register reading and writing functions
	FUNCTION_SUCCESS,
	FUNCTION_ERROR,

	// Return status for module constructing function
	MODULE_CONSTRUCTING_SUCCESS,
	MODULE_CONSTRUCTING_ERROR,

	// Return status for module destructing function
	MODULE_DESTRUCTING_SUCCESS,
	MODULE_DESTRUCTING_ERROR,
};


// Standard mode
#define STANDARD_MODE_NUM		3
enum
{
	STANDARD_DVBT,
	STANDARD_ATSC,
	STANDARD_QAM,
};

// Crystal frequency
#define CRYSTAL_FREQ_NUM		2
enum
{
	CRYSTAL_FREQ_16000000HZ = 16000000,
	CRYSTAL_FREQ_28800000HZ = 28800000,
};

// I2C types
#define I2C_TYPE_NUM			1
enum
{
	I2C_PARALLELPORT,
};


// Demod types
//#define DEMOD_TYPE_NUM			1
enum
{
    DEMOD_NODEMOD,
	DEMOD_RTL2830,
	DEMOD_RTL2830B,
	DEMOD_RTL2831U,
	//------------
	DEMOD_TYPE_NUM,
};


// Tuner types
//#define TUNER_TYPE_NUM			2
enum
{
	TUNER_NOTUNER,
	TUNER_TDTMG252D,
	TUNER_MXL5005,
	//------------
	TUNER_TYPE_NUM,
};


// Demod initializing modes
#define INIT_MODE_NUM			2
enum
{
	DONGLE_APPLICATION,
	STB_APPLICATION,
};


// Demod initializing modes
enum{
    OUT_IF_PARALLEL,		   
    OUT_IF_SERIAL,
};
    
enum{
    OUT_FREQ_6MHZ,		   
    OUT_FREQ_10MHZ,
};    

// IF frequency
#define IF_FREQ_NUM				3
enum
{
	IF_FREQ_4571429HZ  =  4571429,
	IF_FREQ_36125000HZ = 36125000,
	IF_FREQ_36166667HZ = 36166667,
};


// Bandwidth modes
#define BANDWIDTH_NONE			-1
//#define BANDWIDTH_MODE_NUM		3
enum
{
	BANDWIDTH_6MHZ,
	BANDWIDTH_7MHZ,
	BANDWIDTH_8MHZ,
	//----------------------
	BANDWIDTH_MODE_NUM,
};


// Spectrum modes
#define SPECTRUM_MODE_NUM		2
enum
{
	NON_SPECTRUM_INVERSE,
	SPECTRUM_INVERSE,
};


// TPS information

// Constellation
#define CONSTELLATION_NUM		3
enum
{
	CONSTELLATION_QPSK,
	CONSTELLATION_16QAM,
	CONSTELLATION_64QAM,
};

// Hierarchy
#define HIERARCHY_NUM			4
enum
{
#if 1
    HIERARCHY_NONE_0,
#else    
	HIERARCHY_NONE,
#endif	
	HIERARCHY_ALPHA_1,
	HIERARCHY_ALPHA_2,
	HIERARCHY_ALPHA_4,
};

// Code rate
#define CODE_RATE_NUM			5
enum
{
	CODE_RATE_1_OVER_2,
	CODE_RATE_2_OVER_3,
	CODE_RATE_3_OVER_4,
	CODE_RATE_5_OVER_6,
	CODE_RATE_7_OVER_8,
};

// Guard interval
#define GUARD_INTERVAL_NUM		4
enum
{
	GUARD_INTERVAL_1_OVER_32,
	GUARD_INTERVAL_1_OVER_16,
	GUARD_INTERVAL_1_OVER_8,
	GUARD_INTERVAL_1_OVER_4,
};

// FFT mode
#define FFT_MODE_NUM			2
enum
{
	FFT_MODE_2K,
	FFT_MODE_8K,
};





// Memory allocating and freeing function pointers
extern int (*AllocateMemory)(void **ppBuffer, unsigned int ByteNum);
extern void (*FreeMemory)(void *pBuffer);

// Waiting function pointer
extern void (*Wait)(unsigned int WaitTimeMs);

// Basic I2C function pointers
extern int
(*SendReadCommand)(
	unsigned char DeviceAddr,
	unsigned char *pReadingBytes,
	unsigned char ByteNum
	);
extern int
(*SendWriteCommand)(
	unsigned char DeviceAddr,
	const unsigned char *pWritingBytes,
	unsigned char ByteNum
	);





// I2C module definitions

// I2C multi-module availability definitions
enum
{
	I2C_MULTI_MODULE_NON_AVAILABLE,
	I2C_MULTI_MODULE_AVAILABLE,
};

// I2C module structure
typedef struct I2C_MODULE_TAG I2C_MODULE;
struct I2C_MODULE_TAG
{
	// I2C type
	int I2cType;

	// Multi-module availability
	unsigned char MultiModuleAvailability;

	// I2C extra module
	void *pExtra;

	// I2C reading and writing
	int
	(*Read)(
		I2C_MODULE *pI2c,
		unsigned char DeviceAddr,
		unsigned char *pReadingBytes,
		unsigned char ByteNum
		);
	int
	(*Write)(
		I2C_MODULE *pI2c,
		unsigned char DeviceAddr,
		const unsigned char *pWritingBytes,
		unsigned char ByteNum
		);
};





// Demod module definitions

// Demod I2C status definitions
enum
{
	DEMOD_I2C_SUCCESS,
	DEMOD_I2C_ERROR,
};

// Demod TPS lock status definitions
enum
{
	TPS_NON_LOCK,
	TPS_LOCK,
};

// Demod signal lock status definitions
enum
{
	SIGNAL_NON_LOCK,
	SIGNAL_LOCK,
};

// Demod register entry structure
typedef struct
{
	unsigned char PageNo;
	unsigned char RegStartAddr;
	unsigned char ByteNum;
	unsigned long Mask;
	unsigned char Shift;
}
DEMOD_REG_ENTRY;

// Demod module structure
typedef struct DEMOD_MODULE_TAG DEMOD_MODULE;
struct DEMOD_MODULE_TAG
{
#if 1
    void* pExtra;
#endif     
	// Demod type
	int DemodType;		

	// Demod I2C device address
	unsigned char DeviceAddr;

	// Demod register table
	DEMOD_REG_ENTRY *pRegTable;

	// Demod update procedure variables
	unsigned char Func2Executing;
	unsigned char Func3State;
	unsigned char Func3Executing;
	unsigned char Func4State;
	unsigned long Func4DelayCnt;
	unsigned long Func4DelayCntMax;
	unsigned char Func4ParamSetting;
	unsigned long Func4RegValue[FUNC4_REG_VALUE_NUM];
	unsigned char Func5State;
	unsigned char Func5QamBak;
	unsigned char Func5HierBak;
	unsigned char Func5LpCrBak;
	unsigned char Func5HpCrBak;
	unsigned char Func5GiBak;
	unsigned char Func5FftBak;


	// Demod I2C function pointers
	int (*SetRegBytes)(DEMOD_MODULE *pDemod, unsigned char PageNo,
		unsigned char RegStartAddr, const unsigned char *pWritingBytes,
		unsigned char ByteNum);
	int (*GetRegBytes)(DEMOD_MODULE *pDemod, unsigned char PageNo,
		unsigned char RegStartAddr, unsigned char *pReadingBytes, unsigned char ByteNum);
	int (*SetRegBits)(DEMOD_MODULE *pDemod, int RegBitName, unsigned long WritingValue);
	int (*GetRegBits)(DEMOD_MODULE *pDemod, int RegBitName, unsigned long *pReadingValue);

	// Demod operating function pointers
	int (*TestI2c)(DEMOD_MODULE *pDemod, unsigned char *pI2cStatus);
	int (*SoftwareReset)(DEMOD_MODULE *pDemod);
	int (*ForwardTunerI2C)(DEMOD_MODULE *pDemod);
	int (*Initialize)(DEMOD_MODULE *pDemod, int InitMode, unsigned long IfFreqHz,
		int SpectrumMode, unsigned char Vtop, unsigned char Krf, unsigned char OutputMode,
	    unsigned char OutputFreq);
	int (*SetBandwidthMode)(DEMOD_MODULE *pDemod, int BandwidthMode);
	int (*SetIfFreqHz)(DEMOD_MODULE *pDemod, unsigned long IfFreqHz);
	int (*SetSpectrumMode)(DEMOD_MODULE *pDemod, int SpectrumMode);
	int (*SetVtop)(DEMOD_MODULE *pDemod, unsigned char Vtop);
	int (*SetKrf)(DEMOD_MODULE *pDemod, unsigned char Krf);
	int (*TestTpsLock)(DEMOD_MODULE *pDemod, unsigned char *pTpsLockStatus);
	int (*TestSignalLock)(DEMOD_MODULE *pDemod, unsigned char *pSignalLockStatus);
	int (*GetFsmStage)(DEMOD_MODULE *pDemod, unsigned char *pFsmStage);
	int (*GetSignalStrength)(DEMOD_MODULE *pDemod, unsigned char *pSignalStrength);
	int (*GetSignalQuality)(DEMOD_MODULE *pDemod, unsigned char *pSignalQuality);
	int (*GetSnr)(DEMOD_MODULE *pDemod, long *pSnrNum, long *pSnrDen);
	int (*GetBer)(DEMOD_MODULE *pDemod, unsigned long *pBerNum, unsigned long *pBerDen);
	int (*GetEstEvm)(DEMOD_MODULE *pDemod, unsigned int *pEstEvm);
	int (*GetRsdBerEst)(DEMOD_MODULE *pDemod, unsigned int *pRsdBerEst);
	int (*GetRfAgc)(DEMOD_MODULE *pDemod, int *pRfAgc);
	int (*GetIfAgc)(DEMOD_MODULE *pDemod, int *pIfAgc);
	int (*GetDiAgc)(DEMOD_MODULE *pDemod, unsigned char *pDiAgc);
	int (*GetConstellation)(DEMOD_MODULE *pDemod, unsigned char *pConstellation);
	int (*GetHierarchy)(DEMOD_MODULE *pDemod, unsigned char *pHierarchy);
	int (*GetCodeRateLp)(DEMOD_MODULE *pDemod, unsigned char *pCodeRateLp);
	int (*GetCodeRateHp)(DEMOD_MODULE *pDemod, unsigned char *pCodeRateHp);
	int (*GetGuardInterval)(DEMOD_MODULE *pDemod, unsigned char *pGuardInterval);
	int (*GetFftMode)(DEMOD_MODULE *pDemod, unsigned char *pFftMode);
	int (*UpdateFunction2)(DEMOD_MODULE *pDemod);
	int (*UpdateFunction3)(DEMOD_MODULE *pDemod);
	int (*ResetFunction4)(DEMOD_MODULE *pDemod);
	int (*UpdateFunction4)(DEMOD_MODULE *pDemod);
	int (*ResetFunction5)(DEMOD_MODULE *pDemod);
	int (*UpdateFunction5)(DEMOD_MODULE *pDemod);
};





// Tuner module definitions

// Tuner module structure
typedef struct TUNER_MODULE_TAG TUNER_MODULE;
struct TUNER_MODULE_TAG
{
	// Tuner type
	int TunerType;

	// Tuner I2C device address
	unsigned char DeviceAddr;

	// Demod module pointer
	DEMOD_MODULE **ppDemod;

	// Tuner extra module
	void *pExtra;

	// Tuner operating function pointers
	int (*Initialize)(TUNER_MODULE *pTuner, unsigned long IfFreqHz, int SpectrumMode);
	int (*SetRfFreqHz)(TUNER_MODULE *pTuner, unsigned long TargetRfFreqHz, unsigned long *pActualRfFreqHz);
	int (*SetIfFreqHz)(TUNER_MODULE *pTuner, unsigned long IfFreqHz);
	int (*SetBandwidthMode)(TUNER_MODULE *pTuner, int BandwidthMode);
	int (*SetSpectrumMode)(TUNER_MODULE *pTuner, int SpectrumMode);
};





// Math functions

// Binary and signed integer converter
unsigned long SignedIntToBin(long Value, unsigned char BitNum);
long BinToSignedInt(unsigned long Binary, unsigned char BitNum);

// Log10 approximation
long Log10Apx(unsigned long Value);











#endif
