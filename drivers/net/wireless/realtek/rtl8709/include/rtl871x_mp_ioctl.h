#ifndef _RTL871X_MP_IOCTL_H
#define _RTL871X_MP_IOCTL_H
//#include <ndis.h>
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#define REALTEK_VENDER_ID  0x004c0e00


#define TESTFWCMDNUMBER		    1000000
#define TEST_H2CINT_WAIT_TIME		    500
#define TEST_C2HINT_WAIT_TIME		    500
#define HCI_TEST_SYSCFG_HWMASK   		 1
#define _BUSCLK_40M	(4 << 2)




typedef struct _RW_REG{
	uint offset;
	uint width;
	u32 value;
}mp_rw_reg,RW_Reg, *pRW_Reg;


//for OID_RT_PRO_READ16_EEPROM & OID_RT_PRO_WRITE16_EEPROM
typedef struct _EEPROM_RW_PARAM{
	uint offset;
	u16 value;
}eeprom_rw_param,EEPROM_RWParam, *pEEPROM_RWParam;

typedef struct _TXCMD_DESC{
	uint offset;
	union txcmd TxCMD;
}txcmd,TX_CMD_Desc, *pTX_CMD_Desc;

typedef struct _H2C_COMMAND_TUPLE{
              UCHAR	SeqNum;
	       UCHAR 	Result;
		USHORT 	CmdCode;
}H2C_cmd_Tuple, *pH2C_cmd_Tuple;

//for OID_RT_PRO_H2C_QUERY_RESULT in H2C CMD Mode and CMD_Response Mode
typedef struct _QUERY_H2C_CMD_RESULT{
  UINT	 PollingTimes;
  UCHAR	 SeqNum;
  UCHAR	 Result;
  USHORT CmdCode;  
  
  //ToDo: adding one param struct if it is CMD_Response Mode
  
}QryH2C_Result, *pQryH2C_Result;

typedef struct _BURST_RW_REG{
	uint offset;
	uint len;
	u8 Data[256];
}burst_rw_reg,Burst_RW_Reg, *pBurst_RW_Reg;	

typedef struct _RW_ACCESS_PROTOCOL{
	u8	NunOfTrans;
	u8	Length[10];
	u8	Write[10];
	u32	BusAddress[10];
	u8	Data[128];
}rw_protocol, RW_ACCESS_PROTOCOL,*pRW_ACCESS_PROTOCOL;

typedef struct _SCSI_DATA{
	u32	Length;
	u8	Data[512];
} scst_data,SCSI_DATA,*pSCSI_DATA;

typedef struct _USB_VendorReq{
       u8	bRequest;
	u16	wValue;
	u16  wIndex;
	u16	wLength;
	u8	u8Dir;//0:OUT, 1:IN
	u8	u8InData;	
}usb_vendor_req,USB_VendorReq, *pUSB_VendorReq;
	
typedef struct _USB_FIFO_WRITE_REQUEST{
	u8	FIFOIndex;
	u16	dataLength;
	u8	data[2048];
} usb_fifo_wt_req,USB_FIFO_WRITE_REQUEST;

typedef struct _USB_FIFO_READ_REQUEST{
	u8	FIFOIndex;
	u16	dataLength;
	u8	data[2048];
}usb_fifo_rd_req, USB_FIFO_READ_REQUEST;

typedef struct _IoCtrl_ReadPacket
{
	u16 PktLen;
	u8 data[2048];
}ioctl_rd_pkt,IoCtrl_ReadPacket, *pIoCtrl_ReadPacket;

typedef struct _AsyncRWIOTest
{
u8   mode;
u32   addr;   
u8   width;
u32   data;
u8   flush;
u8   cnt;
u8   last;

}async_rw_test,AsyncRWIOTest;

NDIS_STATUS	
mp_set_info(//MPPerformSetInformation(
	PADAPTER		Adapter,
	NDIS_OID		Oid,
	u8			*InformationBuffer,
	u32			InformationBufferLength,
	u32*			BytesRead,
	u32*			BytesNeeded
);

NDIS_STATUS 
mp_query_info(//MPPerformQueryInformation(
	PADAPTER		Adapter,
	NDIS_OID		Oid,
	u8			*InformationBuffer,
	u32			InformationBufferLength,
	u32*			BytesWritten,
	u32*			BytesNeeded
);



void TestC2HR_ISRHandler(PADAPTER		Adapter);

void TestC2HR_INT(PADAPTER		Adapter);
void TestH2CR_INT(PADAPTER		Adapter);


void TestHWSEM(PADAPTER		Adapter);
void TestHSSEL(PADAPTER		Adapter);
void TestCPWM_R_Interrupt(PADAPTER		Adapter);
void TestCPWM_R(PADAPTER		Adapter);
void TestRPWM_RW(PADAPTER		Adapter);
void TestFW_CMD (PADAPTER Adapter);
void TestFW_CMDRSP (PADAPTER Adapter);
void TestFW_CMDEvent(IN PADAPTER Adapter);
void StartFWLoopBackSystemThread(VOID *adapter);
void StopFWLoopBackSystemThread(VOID *adapter);
void TestMac1ByteRW(PADAPTER Adapter);
void TestMac4ByteRW(PADAPTER Adapter,  u32 patternOdd, u32 patternEven);
void Pattern4TestMac4ByteRWv1(IN PADAPTER Adapter);
void Pattern4TestMac4ByteRWv2(IN PADAPTER Adapter);
void TestInitMacRW(PADAPTER Adapter);
void TestInitMacRWV2(PADAPTER Adapter);
void TestInitMAC_BSSID(PADAPTER Adapter);
void Test_HCIRXEN(PADAPTER Adapter);


#ifdef CONFIG_CFIO_HCI
void CFIOTestAttributeMemory(PADAPTER Adapter);
void CFIOTestReset(PADAPTER Adapter);
void CFIOTestRSTMSKOFF(PADAPTER Adapter);
#endif

int Async_RW_Test(_adapter *padapter);

#endif
