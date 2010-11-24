// Fundamental interface


#include "foundation.h"





// Memory allocating and freeing function pointers
//
// Use AllocateMemory() to allocate ByteNum bytes memory to pBuffer pointer.
// Use FreeMemory() to free the memory pointed by pBuffer pointer.
//
// The requirements of the 2 function pointers are described as follows:
//
// 1. AllocateMemory() will set pBuffer with NULL if memory allocating is unsuccessful.
//
// 2. The return value definitions of AllocateMemory() are as follows:
//    MEMORY_ALLOCATING_SUCCESS
//    MEMORY_ALLOCATING_ERROR
//
// 3. FreeMemory() has no return value.
//
//
// Need to assign a memory allocating function to AllocateMemory and
// assign a memory freeing function to FreeMemory for demod module.
//
int (*AllocateMemory)(void **ppBuffer, unsigned int ByteNum);
void (*FreeMemory)(void *pBuffer);



// Waiting function pointer
//
// Use Wait() to wait WaitTimeMs ms.
//
// The requirements of Wait() are described as follows:
//
// 1. The input argument WaitTimeMs unit of Wait() must be ms.
//
// 2. Wait() has no return value.
//
//
// Need to assign a waiting function to Wait for demod module.
//
void (*Wait)(unsigned int WaitTimeMs);



// Basic I2C function pointers
//
// Use SendReadCommand() to read ByteNum bytes from device to pReadingBytes buffer.
// Use SendWriteCommand() to write ByteNum bytes from pWritingBytes buffer to device.
// Note that the I2C device address of the device is DeviceAddr.
// All dmoed I2C functions are based on the 2 basic I2C function pointers.
//
// The requirements of the 2 function pointers are described as follows:
//
// 1. The DeviceAddr is 8-bit format.
//
// 2. SendReadCommand() I2C format -
//    start_bit + (DeviceAddr | reading_bit) + reading_byte * ByteNum + stop_bit
//
// 3. SendWriteCommand() I2C format -
//    start_bit + (DeviceAddr | writing_bit) + writing_byte * ByteNum + stop_bit
//
// 4. The return value definitions are as follows:
//    I2C_SUCCESS
//    I2C_ERROR
//
//
// Need to assign I2C funtions to the basic I2C function pointers for demod module.
//
int
(*SendReadCommand)(
	unsigned char DeviceAddr,
	unsigned char *pReadingBytes,
	unsigned char ByteNum
	);
int
(*SendWriteCommand)(
	unsigned char DeviceAddr,
	const unsigned char *pWritingBytes,
	unsigned char ByteNum
	);





// Convert signed integer to binary.
//
// 1. Generate Mask according to BitNum.
// 2. Convert signed integer to binary with Mask.
//
// Note: Value must be -pow(2, BitNum - 1) ~ (pow(2, BitNum - 1) -1).
//
unsigned long
SignedIntToBin(
	long Value,
	unsigned char BitNum
	)
{
	unsigned int i;
	unsigned long Mask, Binary;


	// Generate Mask according to BitNum.
	//
	Mask = 0;
	for(i = 0; i < BitNum; i++)
		Mask |= 0x1 << i;

	// Convert signed integer to binary with Mask.
	//
	Binary = Value & Mask;


	return Binary;
}





// Convert binary to signed integer.
//
// 1. Calculate Shift.
// 2. Use integer left and right shift to extend signed bit.
//
// Note: Integer right shift will fill MSB part with signed bit.
//       Binary must be 0 ~ (pow(2, BitNum) - 1).
//
long
BinToSignedInt(
	unsigned long Binary,
	unsigned char BitNum
	)
{
	int i;

	unsigned char SignedBit;
	unsigned long SignedBitExtension;

	long Value;



	// Get signed bit.
	SignedBit = (unsigned char)((Binary >> (BitNum - 1)) & BIT_0_MASK);


	// Generate signed bit extension.
	SignedBitExtension = 0;

	for(i = BitNum; i < LONG_BIT_NUM; i++)
		SignedBitExtension |= SignedBit << i;


	// Combine binary value and signed bit extension to signed integer value.
	Value = (long)(Binary | SignedBitExtension);


	return Value;
}





// Log approximation with base 10
//
// 1. Use logarithm value table to approximate log10() function.
//
// Note: The Value must be 1 ~ 100000.
//       The unit of Value is 1.
//       The unit of return value is 0.0001.
//
long Log10Apx(unsigned long Value)
{
	long Result;

	// Note: The unit of Log10Table elements is 0.0001.
	//       The value of index 0 elements is 0.
	//
	const unsigned int Log10Table[] =
	{
		0,
		0,		3010,	4771,	6021,	6990,	7782,	8451,	9031,	9542,	10000,
		10414,	10792,	11139,	11461,	11761,	12041,	12304,	12553,	12788,	13010,
		13222,	13424,	13617,	13802,	13979,	14150,	14314,	14472,	14624,	14771,
		14914,	15051,	15185,	15315,	15441,	15563,	15682,	15798,	15911,	16021,
		16128,	16232,	16335,	16435,	16532,	16628,	16721,	16812,	16902,	16990,
		17076,	17160,	17243,	17324,	17404,	17482,	17559,	17634,	17709,	17782,
		17853,	17924,	17993,	18062,	18129,	18195,	18261,	18325,	18388,	18451,
		18513,	18573,	18633,	18692,	18751,	18808,	18865,	18921,	18976,	19031,
		19085,	19138,	19191,	19243,	19294,	19345,	19395,	19445,	19494,	19542,
		19590,	19638,	19685,	19731,	19777,	19823,	19868,	19912,	19956,	20000,
	};



	// Use logarithm value table to approximate log10() function.
	//
	if(Value < 100)
		Result = Log10Table[Value];
	else if(Value < 1000)
		Result = Log10Table[Value / 10]   + 10000;
	else if(Value < 10000)
		Result = Log10Table[Value / 100]  + 20000;
	else
		Result = Log10Table[Value / 1000] + 30000;

	return Result;
}











/*


Revision history.

Version format:  
x.x                                 .yyyy.mm.dd
(RTL2830 programming guide version) (release date)



1.3.2007.01.16

	1. First release.



Last Version: None





*/





