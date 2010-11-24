#ifndef __DEMOD_RTL2830_H
#define __DEMOD_RTL2830_H


#include "foundation.h"





// Definitions

// Page register address
#define PAGE_REG_ADDR		0x00


// Definitions for initializing
#define INIT_TABLE_LEN				26
#define INIT_TABLE_LEN_WITH_MODE	6

// Registers for initializing
// - page no, start address, and length
#define MGD_THDX_PAGE		1
#define MGD_THDX_ADDR		0x95
#define MGD_THDX_LEN		8


// Definitions for bandwidth setting

// Registers for bandwidth programming
// - page no, start address, and length
#define H_LPF_X_PAGE		    1
#define H_LPF_X_ADDR		    0x1c
#define H_LPF_X_LEN			    34
#define RATIO_PAGE			    1
#define RATIO_ADDR		    	0x9d
#define RATIO_LEN			    6


// Definitions for Function 2
#define SELECT_INDEX_NUM		8
#define UPDATE_REG_1_PAGE		2


// Definitions for Function 3
#define FUNC3_STATE_NUM			6
#define ASSIGNED_P3VALUE		3
#define ASSIGNED_D3VALUE		0


// Definitions for Function 4
#define FUNC4_DELAY_MS			2000
#define FUNC4_VALUE_AVG_NUM		4
#define FUNC4_PARAM_PAGE		2
#define FUNC4_PARAM_ADDR		0x86
#define FUNC4_PARAM_LEN			77


// Definitions for BER and SNR
#define EST_EVM_MAX_VALUE		65535
#define SNR_NUM_MAX_VALUE		1000
#define SNR_DEN_VALUE			10
#define RSD_BER_EST_MAX_VALUE	19616
#define BER_DEN_VALUE			1000000


// Definitions for AGC
#define RF_AGC_REG_BIT_NUM		14
#define IF_AGC_REG_BIT_NUM		14


// Register table dependence

// Constant
#define RTL2830_REG_TABLE_LEN		110

// Demod register bit names
enum
{
	// Software reset register
	SOFT_RST,

	// Tuner I2C forwording register
	IIC_REPEAT,

	// Registers for initializing
	REG_PFREQ_1_0,
	PD_DA8,
	LOCK_TH,
	BER_PASS_SCAL,
	TR_WAIT_MIN_8K,
	RSD_BER_FAIL_VAL,
	CE_FFSM_BYPASS,
	ALPHAIIR_N,
	ALPHAIIR_DIF,
	EN_TRK_SPAN,
	EN_BK_TRK,
	DAGC_TRG_VAL,
	AGC_TARG_VAL,
	REG_PI,
	LOCK_TH_LEN,
	CCI_THRE,
	CCI_MON_SCAL,
	CCI_M0,
	CCI_M1,
	CCI_M2,
	CCI_M3,

	// Registers for initializing according to mode
	TRK_KS_P2,
	TRK_KS_I2,
	TR_THD_SET2,
	TRK_KC_P2,
	TRK_KC_I2,
	CR_THD_SET2,

	// Registers for initializing according to tuner type
	PSET_IFFREQ,
	SPEC_INV,
	VTOP,
	KRF,

	// Registers for bandwidth programming
	BW_INDEX,

	// FSM stage register
	FSM_STAGE,

	// TPS content registers
	RX_CONSTEL,
	RX_HIER,
	RX_C_RATE_LP,
	RX_C_RATE_HP,
	GI_IDX,
	FFT_MODE_IDX,
	
	// Performance measurement registers
	RSD_BER_EST,
	EST_EVM,

	// AGC registers
	RF_AGC_VAL,
	IF_AGC_VAL,
	DAGC_VAL,

	// AGC relative registers
	POLAR_RF_AGC,
	POLAR_IF_AGC,
	LOOP_GAIN_3_0,
	LOOP_GAIN_4,
	AAGC_HOLD,
	EN_RF_AGC,
	EN_IF_AGC,
	IF_AGC_MIN,
	IF_AGC_MAX,
	RF_AGC_MIN,
	RF_AGC_MAX,
	IF_AGC_MAN,
	IF_AGC_MAN_VAL,
	RF_AGC_MAN,
	RF_AGC_MAN_VAL,

	// TS interface registers
	CK_OUT_PAR,
	CK_OUT_PWR,
	SYNC_DUR,
	ERR_DUR,
	SYNC_LVL,
	ERR_LVL,
	VAL_LVL,
	SERIAL,
	SER_LSB,
	CDIV_PH0,
	CDIV_PH1,

	// FSM state-holding register
	SM_BYPASS,

	// Registers for function 2
	UPDATE_REG_2,

	// Registers for function 3
	BTHD_P3,
	BTHD_D3,

	// Registers for function 4
	FUNC4_REG0,
	FUNC4_REG1,
	FUNC4_REG2,
	FUNC4_REG3,
	FUNC4_REG4,
	FUNC4_REG5,
	FUNC4_REG6,
	FUNC4_REG7,
	FUNC4_REG8,
	FUNC4_REG9,
	FUNC4_REG10,

	// Registers for functin 5
	FUNC5_REG0,
	FUNC5_REG1,
	FUNC5_REG2,
	FUNC5_REG3,
	FUNC5_REG4,
	FUNC5_REG5,
	FUNC5_REG6,
	FUNC5_REG7,
	FUNC5_REG8,
	FUNC5_REG9,
	FUNC5_REG10,
	FUNC5_REG11,
	FUNC5_REG12,
	FUNC5_REG13,
	FUNC5_REG14,
	FUNC5_REG15,
	FUNC5_REG16,
	FUNC5_REG17,
	FUNC5_REG18,

	// Test registers for test only
	TEST_REG_1,
	TEST_REG_2,
	TEST_REG_3,
	TEST_REG_4,
};

// Register table initializing
void rtl2830_InitRegTable(DEMOD_REG_ENTRY *pRegTable);





// Demod module constructor and destructor
int
ConstructRtl2830Module(
	DEMOD_MODULE **ppDemod,
	unsigned char SubType,
	unsigned char DeviceAddr,
	unsigned long Func4PeriodMs
	);

void
DestructRtl2830Module(
	DEMOD_MODULE *pDemod
	);





// Demod I2C functions
int
rtl2830_SetRegBytes(
	DEMOD_MODULE *pDemod,
	unsigned char PageNo,
	unsigned char RegStartAddr,
	const unsigned char *pWritingBytes,
	unsigned char ByteNum
	);

int
rtl2830_GetRegBytes(
	DEMOD_MODULE *pDemod,
	unsigned char PageNo,
	unsigned char RegStartAddr,
	unsigned char *pReadingBytes,
	unsigned char ByteNum
	);

int
rtl2830_SetRegBits(
	DEMOD_MODULE *pDemod,
	int RegBitName,
	unsigned long WritingValue
	);

int
rtl2830_GetRegBits(
	DEMOD_MODULE *pDemod,
	int RegBitName,
	unsigned long *pReadingValue
	);





// Demod operating functions

// Demod I2C test
int rtl2830_TestI2c(DEMOD_MODULE *pDemod, unsigned char *pI2cStatus);

// Demod software reset
int rtl2830_SoftwareReset(DEMOD_MODULE *pDemod);

// Demod tuner-I2C forwarding
int rtl2830_ForwardTunerI2C(DEMOD_MODULE *pDemod);

// Demod initializing
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
	);

// Demod bandwidth mode setting
int rtl2830_SetBandwidthMode(DEMOD_MODULE *pDemod, int BandwidthMode);

// Demod parameter setting 
int rtl2830_SetIfFreqHz(DEMOD_MODULE *pDemod, unsigned long IfFreqHz);
int rtl2830_SetSpectrumMode(DEMOD_MODULE *pDemod, int SpectrumMode);
int rtl2830_SetVtop(DEMOD_MODULE *pDemod, unsigned char Vtop);
int rtl2830_SetKrf(DEMOD_MODULE *pDemod, unsigned char Krf);

// Demod TPS and signal lock test
int rtl2830_TestTpsLock(DEMOD_MODULE *pDemod, unsigned char *pTpsLockStatus);
int rtl2830_TestSignalLock(DEMOD_MODULE *pDemod, unsigned char *pSignalLockStatus);

// Demod FSM stage getting
int rtl2830_GetFsmStage(DEMOD_MODULE *pDemod, unsigned char *pFsmStage);

// Demod signal strength and signal quality getting
int rtl2830_GetSignalStrength(DEMOD_MODULE *pDemod, unsigned char *pSignalStrength);
int rtl2830_GetSignalQuality(DEMOD_MODULE *pDemod, unsigned char *pSignalQuality);

// Demod SNR and BER getting
int rtl2830_GetSnr(DEMOD_MODULE *pDemod, long *pSnrNum, long *pSnrDen);
int rtl2830_GetBer(DEMOD_MODULE *pDemod, unsigned long *pBerNum, unsigned long *pBerDen);
int rtl2830_GetEstEvm(DEMOD_MODULE *pDemod, unsigned int *pEstEvm);
int rtl2830_GetRsdBerEst(DEMOD_MODULE *pDemod, unsigned int *pRsdBerEst);

// Demod AGC getting
int rtl2830_GetRfAgc(DEMOD_MODULE *pDemod, int *pRfAgc);
int rtl2830_GetIfAgc(DEMOD_MODULE *pDemod, int *pIfAgc);
int rtl2830_GetDiAgc(DEMOD_MODULE *pDemod, unsigned char *pDiAgc);

// Demod TPS information getting
int rtl2830_GetConstellation(DEMOD_MODULE *pDemod, unsigned char *pConstellation);
int rtl2830_GetHierarchy(DEMOD_MODULE *pDemod, unsigned char *pHierarchy);
int rtl2830_GetCodeRateLp(DEMOD_MODULE *pDemod, unsigned char *pCodeRateLp);
int rtl2830_GetCodeRateHp(DEMOD_MODULE *pDemod, unsigned char *pCodeRateHp);
int rtl2830_GetGuardInterval(DEMOD_MODULE *pDemod, unsigned char *pGuardInterval);
int rtl2830_GetFftMode(DEMOD_MODULE *pDemod, unsigned char *pFftMode);

// Demod register updating for performance
int rtl2830_UpdateFunction2(DEMOD_MODULE *pDemod);
int rtl2830_UpdateFunction3(DEMOD_MODULE *pDemod);
int rtl2830_ResetFunction4(DEMOD_MODULE *pDemod);
int rtl2830_UpdateFunction4(DEMOD_MODULE *pDemod);
int rtl2830_ResetFunction5(DEMOD_MODULE *pDemod);
int rtl2830_UpdateFunction5(DEMOD_MODULE *pDemod);



// Demod tools
int rtl2830_GetFunc4RegValueAvg(DEMOD_MODULE *pDemod, unsigned long *pRegValueAvg);









#endif
