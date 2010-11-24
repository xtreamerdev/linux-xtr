/******************************************************************************
* sdio_ops_xp.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _SDIO_OPS_XP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <io.h>
#include <osdep_intf.h>


#ifdef PLATFORM_OS_XP


u8 sdbus_cmd52r_xp(struct intf_priv *pintfpriv, u32 addr)
{

	SD_RW_DIRECT_ARGUMENT 	ioarg;
	struct dvobj_priv *psdiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);
	PSDBUS_REQUEST_PACKET  sdrp = pintfpriv->sdrp;
	NDIS_STATUS status = 0;
	
	const SDCMD_DESCRIPTOR ReadWriteIoDirectDesc = {
		  SDCMD_IO_RW_DIRECT,SDCC_STANDARD,SDTD_READ,SDTT_CMD_ONLY,SDRT_5};

	_func_enter_;

	RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));
	RtlZeroMemory(&ioarg, sizeof(SD_RW_DIRECT_ARGUMENT));
	
	sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	sdrp->Parameters.DeviceCommand.CmdDesc = ReadWriteIoDirectDesc;

	ioarg.u.AsULONG = 0;
	ioarg.u.bits.Address = addr;
	ioarg.u.bits.Function = 0;
	//sdIoArgDir.u.bits.WriteToDevice = ((RWFlag == SDTD_READ) ? 0 : 1);

	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	status = SdBusSubmitRequest(psdiodev->sdbusinft.Context, sdrp);

	_func_exit_;

	return  sdrp->ResponseData.AsUCHAR[0];
}


void sdbus_cmd52w_xp(struct intf_priv *pintfpriv, u32 addr, u8 val8)
{
	SD_RW_DIRECT_ARGUMENT 	ioarg;
	struct dvobj_priv * psdiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);
	PSDBUS_REQUEST_PACKET  sdrp = pintfpriv->sdrp;
	NDIS_STATUS status = 0;
	
	const SDCMD_DESCRIPTOR ReadWriteIoDirectDesc = {
								 SDCMD_IO_RW_DIRECT,
                                                    SDCC_STANDARD,
                                                    SDTD_WRITE,
                                                    SDTT_CMD_ONLY,
                                                    SDRT_5};
	_func_enter_;
	RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));
	RtlZeroMemory(&ioarg, sizeof(SD_RW_DIRECT_ARGUMENT));
	
	sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	sdrp->Parameters.DeviceCommand.CmdDesc = ReadWriteIoDirectDesc;

	ioarg.u.AsULONG = 0;
	ioarg.u.bits.Address = addr;
	ioarg.u.bits.Function = 0;
	ioarg.u.bits.Data = val8 & 0xFF;
	ioarg.u.bits.WriteToDevice = 1;
	
	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	status = SdBusSubmitRequest(psdiodev->sdbusinft.Context, sdrp);
	_func_exit_;
	return ;
}



uint sdbus_read_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt)
{

	SD_RW_EXTENDED_ARGUMENT ioarg;
	struct dvobj_priv * psdiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);
	PSDBUS_REQUEST_PACKET  sdrp = pintfpriv->sdrp;
	PMDL pmdl = pintfpriv->pmdl;
	NDIS_STATUS status = 0;
	
	const SDCMD_DESCRIPTOR ReadWriteIoExtendedDesc = {SDCMD_IO_RW_EXTENDED,
                                                    SDCC_STANDARD,
                                                    SDTD_READ,
                                                    SDTT_SINGLE_BLOCK,
                                                    SDRT_5};
	_func_enter_;
	RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

	sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	sdrp->Parameters.DeviceCommand.CmdDesc = ReadWriteIoExtendedDesc;
	
	ioarg.u.AsULONG = 0;
	
	if(cnt == 512)
	{
		ioarg.u.bits.Count = 0;
		sdrp->Parameters.DeviceCommand.Length = cnt;
	}
	else
	{
		ioarg.u.bits.Count = cnt;    
		sdrp->Parameters.DeviceCommand.Length = cnt;  
	}

	ioarg.u.bits.Address = addr;
	ioarg.u.bits.OpCode  = 1;
	ioarg.u.bits.Function = psdiodev->func_number;
	ioarg.u.bits.WriteToDevice = 0;

	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	sdrp->Parameters.DeviceCommand.Mdl = pmdl;
	sdrp->Parameters.DeviceCommand.Length = cnt;

	status = SdBusSubmitRequest(psdiodev->sdbusinft.Context, sdrp);

	if(!NT_SUCCESS(status)){
		#if 1
		DEBUG_ERR(("***************sdbus_read_xp  CMD53 Read failed. ***************\n "));
		DEBUG_ERR(("*************** STATUS = %x ***************\n ", status));
		DEBUG_ERR(("*************** Addr = %x ***************\n ",addr));
		DEBUG_ERR(("*************** Length = %d ***************\n ",cnt));
		#endif

		DEBUG_ERR(("CMD53 Read Fail\n"));
		_func_exit_;
		return _FAIL;
	}
	_func_exit_;
	return _SUCCESS;
	
}



uint sdbus_read_blocks_to_membuf_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf)
{


	SD_RW_EXTENDED_ARGUMENT ioarg;

	struct dvobj_priv * psdiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);

	PMDL pmdl ;

	PSDBUS_REQUEST_PACKET  sdrp = pintfpriv->recv_sdrp; 	

	NDIS_STATUS status = 0;
	
	const SDCMD_DESCRIPTOR ReadWriteIoExtendedDesc = {SDCMD_IO_RW_EXTENDED,
                                                    SDCC_STANDARD,
                                                    SDTD_READ,
                                                    SDTT_MULTI_BLOCK_NO_CMD12,
                                                    SDRT_5};


	_func_enter_;

	pmdl = IoAllocateMdl(pbuf, cnt , FALSE, FALSE, NULL);
	
	if ( pmdl == NULL ) {

		DEBUG_ERR(("RECV MDL is NULL.\n "));

		_func_exit_;

		return _FAIL;
	}

	MmBuildMdlForNonPagedPool(pmdl);
	
	RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

	sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	sdrp->Parameters.DeviceCommand.CmdDesc = ReadWriteIoExtendedDesc;
	
	ioarg.u.AsULONG = 0;	
	ioarg.u.bits.Address = addr;
	ioarg.u.bits.OpCode  = 0;
	ioarg.u.bits.Function = psdiodev->func_number;
	ioarg.u.bits.WriteToDevice = 0;

	ioarg.u.bits.BlockMode = 1;
	ioarg.u.bits.Count = cnt >> 9;
	
	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	sdrp->Parameters.DeviceCommand.Mdl = pmdl;
	sdrp->Parameters.DeviceCommand.Length = cnt;

	status = SdBusSubmitRequest(psdiodev->sdbusinft.Context, sdrp);

	if(!NT_SUCCESS(status)){

		DEBUG_ERR(("***************sdbus_write_blocks_from_membuf_xp CMD53 W failed. ***************\n "));
		DEBUG_ERR(("*************** STATUS = %x ***************\n ", status));
		DEBUG_ERR(("*************** Addr = %x ***************\n ",addr));
		DEBUG_ERR(("*************** Length = %d ***************\n ",cnt));

		IoFreeMdl(pmdl );
		_func_exit_;
		return _FAIL;
	}
	IoFreeMdl(pmdl );
	_func_exit_;
	return _SUCCESS;
	
}

uint sdbus_read_bytes_to_membuf_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf)
{

	SD_RW_EXTENDED_ARGUMENT ioarg;

	struct dvobj_priv * psdiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);

	PMDL pmdl;
	
	PSDBUS_REQUEST_PACKET  sdrp = pintfpriv->recv_sdrp; 	
	
	NDIS_STATUS status = 0;
	
	const SDCMD_DESCRIPTOR ReadWriteIoExtendedDesc = {SDCMD_IO_RW_EXTENDED,
                                                    SDCC_STANDARD,
                                                    SDTD_READ,
                                                    SDTT_SINGLE_BLOCK,
                                                    SDRT_5};
	_func_enter_;

	
	pmdl = IoAllocateMdl(pbuf, cnt , FALSE, FALSE, NULL);
	
	if(pmdl == NULL){

		DEBUG_ERR(("Read Port MDL is NULL.\n "));	

		_func_exit_;

		return _FAIL;
	}

	MmBuildMdlForNonPagedPool(pmdl);
	
	RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

	sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	
	sdrp->Parameters.DeviceCommand.CmdDesc = ReadWriteIoExtendedDesc;
	
	ioarg.u.AsULONG = 0;
	
	if(cnt == 512)
	{
		ioarg.u.bits.Count = 0;
		sdrp->Parameters.DeviceCommand.Length = cnt;
	}
	else
	{
		ioarg.u.bits.Count = cnt;    
		sdrp->Parameters.DeviceCommand.Length = cnt;  
	}

	ioarg.u.bits.Address = addr;
	ioarg.u.bits.OpCode  = 1;
	ioarg.u.bits.Function = psdiodev->func_number;
	ioarg.u.bits.WriteToDevice = 0;

	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	sdrp->Parameters.DeviceCommand.Mdl = pmdl;
	sdrp->Parameters.DeviceCommand.Length = cnt;

	status = SdBusSubmitRequest(psdiodev->sdbusinft.Context, sdrp);

	if(!NT_SUCCESS(status)){

		DEBUG_ERR(("***************sdbus_read_bytes_to_membuf_xp CMD53 Read failed. ***************\n "));
		DEBUG_ERR(("*************** STATUS = %x ***************\n ", status));
		DEBUG_ERR(("*************** Addr = %x ***************\n ",addr));
		DEBUG_ERR(("*************** Length = %d ***************\n ",cnt));

		_func_exit_;

		return _FAIL;
	}

	IoFreeMdl(pmdl);
	
	_func_exit_;
	return _SUCCESS;
	
}

uint sdbus_write_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt)
{
	
	SD_RW_EXTENDED_ARGUMENT ioarg;
	struct dvobj_priv * psdiodev = (struct dvobj_priv * )pintfpriv->intf_dev;
	PSDBUS_REQUEST_PACKET sdrp = pintfpriv->sdrp;
	PMDL pmdl = pintfpriv->pmdl;
	NDIS_STATUS status = 0;
	
	
	const SDCMD_DESCRIPTOR ReadWriteIoExtendedDesc = {SDCMD_IO_RW_EXTENDED,
                                                    SDCC_STANDARD,
                                                    SDTD_WRITE,
                                                    SDTT_SINGLE_BLOCK,
                                                    SDRT_5};
	_func_enter_;
	RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

	sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	sdrp->Parameters.DeviceCommand.CmdDesc = ReadWriteIoExtendedDesc;
	
	ioarg.u.AsULONG = 0;
	if (cnt == 512)
	{
		ioarg.u.bits.Count = 0;
		sdrp->Parameters.DeviceCommand.Length = 512;
	}
	else
	{
		ioarg.u.bits.Count = cnt;
		sdrp->Parameters.DeviceCommand.Length = cnt;  
	}
	
	ioarg.u.bits.Address = addr;
	ioarg.u.bits.OpCode  = 1;
	ioarg.u.bits.Function = psdiodev->func_number;
	ioarg.u.bits.WriteToDevice = 1;

	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	sdrp->Parameters.DeviceCommand.Mdl = pmdl;

	status = SdBusSubmitRequest(psdiodev->sdbusinft.Context, sdrp);

	if(!NT_SUCCESS(status)){
		DEBUG_ERR(("***************sdbus_write_xp CMD53 Write failed. ***************\n "));
		DEBUG_ERR(("*************** STATUS = %x ***************\n ", status));
		DEBUG_ERR(("*************** Addr = %x ***************\n ",addr));
		DEBUG_ERR(("*************** Length = %d ***************\n ",cnt));
		_func_exit_;
		return _FAIL;
	}
	_func_exit_;
	return _SUCCESS;

}

uint sdbus_write_blocks_from_membuf_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf)
{


	SD_RW_EXTENDED_ARGUMENT ioarg;

	struct dvobj_priv * psdiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);

	PMDL pmdl ;

	PSDBUS_REQUEST_PACKET  sdrp = pintfpriv->xmit_sdrp; 	

	NDIS_STATUS status = 0;
	
	const SDCMD_DESCRIPTOR ReadWriteIoExtendedDesc = {SDCMD_IO_RW_EXTENDED,
                                                    SDCC_STANDARD,
                                                    SDTD_WRITE,
                                                    SDTT_MULTI_BLOCK_NO_CMD12,
                                                    SDRT_5};


	_func_enter_;

	pmdl = IoAllocateMdl(pbuf, cnt , FALSE, FALSE, NULL);
	
	if ( pmdl == NULL ) {

		DEBUG_ERR(("XMIT MDL is NULL.\n "));

		_func_exit_;

		return _FAIL;
	}

	MmBuildMdlForNonPagedPool(pmdl);
	
	RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

	sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	sdrp->Parameters.DeviceCommand.CmdDesc = ReadWriteIoExtendedDesc;
	
	ioarg.u.AsULONG = 0;

	

	ioarg.u.bits.Address = addr;
	ioarg.u.bits.OpCode  = 0;
	ioarg.u.bits.Function = psdiodev->func_number;
	ioarg.u.bits.WriteToDevice = 1;

	ioarg.u.bits.BlockMode = 1;
	//ioarg.u.bits.Count = cnt >> 9;
	
	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	sdrp->Parameters.DeviceCommand.Mdl = pmdl;
	sdrp->Parameters.DeviceCommand.Length = cnt;

	status = SdBusSubmitRequest(psdiodev->sdbusinft.Context, sdrp);

	if(!NT_SUCCESS(status)){

		DEBUG_ERR(("***************sdbus_write_blocks_from_membuf_xp CMD53 W failed. ***************\n "));
		DEBUG_ERR(("*************** STATUS = %x ***************\n ", status));
		DEBUG_ERR(("*************** Addr = %x ***************\n ",addr));
		DEBUG_ERR(("*************** Length = %d ***************\n ",cnt));


		IoFreeMdl(pmdl );
		_func_exit_;
		return _FAIL;
	}
	IoFreeMdl(pmdl );
	_func_exit_;
	return _SUCCESS;
	
}

uint sdbus_write_bytes_from_membuf_xp(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf)
{

	SD_RW_EXTENDED_ARGUMENT ioarg;

	struct dvobj_priv * psdiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);

	PMDL pmdl ;
	PSDBUS_REQUEST_PACKET  sdrp = pintfpriv->xmit_sdrp; 	

	NDIS_STATUS status = 0;
	
	const SDCMD_DESCRIPTOR ReadWriteIoExtendedDesc = {SDCMD_IO_RW_EXTENDED,
                                                    SDCC_STANDARD,
                                                    SDTD_WRITE,
                                                    SDTT_SINGLE_BLOCK,
                                                    SDRT_5};


	_func_enter_;

	pmdl = IoAllocateMdl(pbuf, cnt , FALSE, FALSE, NULL);

	if ( pmdl == NULL ) {

		DEBUG_ERR(("XMIT MDL is NULL.\n "));

		_func_exit_;

		return _FAIL;
	}

	MmBuildMdlForNonPagedPool(pmdl);

	
	RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

	sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	sdrp->Parameters.DeviceCommand.CmdDesc = ReadWriteIoExtendedDesc;
	
	ioarg.u.AsULONG = 0;

	
	if(cnt == 512)
	{
		ioarg.u.bits.Count = 0;
		sdrp->Parameters.DeviceCommand.Length = cnt;
	}
	else
	{
		ioarg.u.bits.Count = cnt;    
		sdrp->Parameters.DeviceCommand.Length = cnt;  
	}
	ioarg.u.bits.Address = addr;
	ioarg.u.bits.OpCode  = 1;
	ioarg.u.bits.Function = psdiodev->func_number;
	ioarg.u.bits.WriteToDevice = 1;

	ioarg.u.bits.BlockMode = 0;
	
	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	sdrp->Parameters.DeviceCommand.Mdl = pmdl;
	sdrp->Parameters.DeviceCommand.Length = cnt;

	status = SdBusSubmitRequest(psdiodev->sdbusinft.Context, sdrp);

	if(!NT_SUCCESS(status)){

		DEBUG_ERR(("*************** sdbus_write_bytes_from_membuf_xp CMD53 W failed. ***************\n "));
		DEBUG_ERR(("*************** STATUS = %x ***************\n ", status));
		DEBUG_ERR(("*************** Addr = %x ***************\n ",addr));
		DEBUG_ERR(("*************** Length = %d ***************\n ",cnt));

		IoFreeMdl(pmdl );
		_func_exit_;
		return _FAIL;
	}

	IoFreeMdl(pmdl );

	_func_exit_;
	return _SUCCESS;
	
}

#endif


