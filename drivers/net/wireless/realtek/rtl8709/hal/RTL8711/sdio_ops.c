/******************************************************************************
* sdio_ops.c                                                                                                                                 *
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
#define _HCI_OPS_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>


#ifndef CONFIG_SDIO_HCI

#error  "CONFIG_SDIO_HCI shall be on!\n"

#endif


#ifdef PLATFORM_LINUX
#include <sdio_ops_linux.h>
#endif


#ifdef PLATFORM_MPIXEL
#include <sdio_ops_mpixel.h>
#endif


#ifdef PLATFORM_OS_WINCE

#include <sdio_ops_wince.h>

#endif

#ifdef PLATFORM_OS_XP

#include <sdio_ops_xp.h>

#endif

#include <rtl8711_byteorder.h>

static uint __inline get_deviceid(u32 addr)
{
	_func_enter_;
	switch( (addr & 0x1fff0000) )
	{
	case RTL8711_WLANFF_:
		_func_exit_;
		return DID_WLAN_FIFO;
		
	case RTL8711_WLANCTRL_:
		_func_exit_;
		return DID_WLAN_CTRL;
		
	case RTL8711_HCICTRL_:
		_func_exit_;
		return DID_HCI;
		
	case RTL8711_SYSCTRL_:
		_func_exit_;
		return DID_SYSCFG;
		
	case RTL8711_HINT_:
		_func_exit_;
		return DID_INT;
		
	default:
		break;
	}
	_func_exit_;
	return DID_UNDEFINE;	//Undefine Address. Register R/W Protocol is needed.
}




/*
Must use register R/W protocol if return value is false.
*/



static uint __inline _cvrt2ftaddr(const u32 addr, u32 *pftaddr)  
{
	_func_enter_;
	switch(get_deviceid(addr))
	{
				
		case DID_WLAN_CTRL:
			*pftaddr = ((DID_WLAN_CTRL << 14) | (addr & OFFSET_WLAN));
			break;
		
		case DID_SYSCFG:
			*pftaddr = ((DID_SYSCFG << 14) | (addr & OFFSET_CFG));
			break;

		case DID_INT:
			*pftaddr = ((DID_INT << 14) | (addr & OFFSET_INT));
			break;


		case DID_WLAN_FIFO:
			*pftaddr = ((DID_WLAN_FIFO << 14) | (addr & OFFSET_WLAN));
			
			break;

		case DID_HCI:
			*pftaddr = ((DID_HCI << 14) | (addr & OFFSET_HCI));
			
			break;

		default:
			_func_exit_;
			return _FAIL;
	}
	_func_exit_;
	return _SUCCESS;
}





void sdio_attrib_read(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	u8 i;

	u8 (*_cmd52r)(struct intf_priv *pintfpriv, u32 addr);
	
	struct _io_ops *pop = &pintfhdl->io_ops;
	
	_cmd52r = pop->_cmd52r;
	
	_func_enter_;
	
	for(i=0; i<cnt; i++)
		rmem[i] = _cmd52r( pintfhdl->pintfpriv, addr+i);

	_func_exit_;

	return;
}



void sdio_attrib_write(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{

	u8 i;

	void (*_cmd52w)(struct intf_priv *pintfpriv, u32 addr, u8 val8);

	struct _io_ops *pop = &pintfhdl->io_ops;

	_cmd52w = pop->_cmd52w;

	_func_enter_;

	for(i=0; i<cnt; i++)
	{
		_cmd52w( pintfhdl->pintfpriv, addr+i, wmem[i]);
	}

	_func_exit_;
	return;
}



#ifdef USE_ASYNC_IRP

//2006-11-02 yitsen
static u8 sdbus_async_read
(struct io_queue *pio_q , u32 addr, u32 cnt, PIO_COMPLETION_ROUTINE  CompletionRoutine)
{
	SD_RW_EXTENDED_ARGUMENT ioarg;
	PIO_STACK_LOCATION	nextStack;

	NDIS_STATUS status = 0;

	struct intf_priv * pintfpriv = pio_q->intf.pintfpriv;
	PMDL pmdl = pintfpriv->pmdl; // map to io_rwmem
	PIRP  pirp  = pintfpriv->piorw_irp;
	PSDBUS_REQUEST_PACKET sdrp = pintfpriv->sdrp;
	struct dvobj_priv * psdiodev = (struct dvobj_priv * )pintfpriv->intf_dev;
	
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
	ioarg.u.bits.WriteToDevice = 0;


	  //pirp = IoAllocateIrp( psdiodev->NextDeviceStackSize + 1, FALSE);

	nextStack = IoGetNextIrpStackLocation(pirp);
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SD_SUBMIT_REQUEST;
	nextStack->Parameters.DeviceIoControl.Type3InputBuffer = sdrp;	 

	//sdrp->UserContext[0] =  Adapter;

	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	sdrp->Parameters.DeviceCommand.Mdl = pmdl;

	status = 
	SdBusSubmitRequestAsync (psdiodev->sdbusinft.Context,
							sdrp,
							pirp,
							CompletionRoutine,
							(PVOID)pio_q);//(&callback_context));
							


	if(status!= STATUS_PENDING){
		DEBUG_ERR(("*** async cmd53 RD failed. ***\n "));
		DEBUG_ERR(("*** STATUS = %x ***\n ", status));
		DEBUG_ERR(("*** Addr = %x ***\n ",addr));
		DEBUG_ERR(("*** Length = %d ***\n ",cnt));
		_func_exit_;
		return _FAIL;
	}
	_func_exit_;
	return _SUCCESS;

}

//2006-11-02 yitsen
static u8 sdbus_async_write
(struct io_queue *pio_q , u32 addr, u32 cnt,PIO_COMPLETION_ROUTINE  CompletionRoutine)
{

	SD_RW_EXTENDED_ARGUMENT ioarg;
	PIO_STACK_LOCATION	nextStack;

	NDIS_STATUS status = 0;

	struct intf_priv * pintfpriv = pio_q->intf.pintfpriv;
	PMDL pmdl = pintfpriv->pmdl; // map to io_rwmem
	PIRP  pirp  = pintfpriv->piorw_irp;
	PSDBUS_REQUEST_PACKET sdrp = pintfpriv->sdrp;
	struct dvobj_priv * psdiodev = (struct dvobj_priv * )pintfpriv->intf_dev;
	
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


	  //pirp = IoAllocateIrp( psdiodev->NextDeviceStackSize + 1, FALSE);

	nextStack = IoGetNextIrpStackLocation(pirp);
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SD_SUBMIT_REQUEST;
	nextStack->Parameters.DeviceIoControl.Type3InputBuffer = sdrp;	 

	//sdrp->UserContext[0] =  Adapter;

	sdrp->Parameters.DeviceCommand.Argument = ioarg.u.AsULONG;
	sdrp->Parameters.DeviceCommand.Mdl = pmdl;

	status = 
	SdBusSubmitRequestAsync (psdiodev->sdbusinft.Context,
							sdrp,
							pirp,
							CompletionRoutine,
							(PVOID)pio_q);//(&callback_context));
							


	if(status!= STATUS_PENDING){
		DEBUG_ERR(("*** async cmd53 Write failed. ***\n "));
		DEBUG_ERR(("*** STATUS = %x ***\n ", status));
		DEBUG_ERR(("*** Addr = %x ***\n ",addr));
		DEBUG_ERR(("*** Length = %d ***\n ",cnt));
		_func_exit_;
		return _FAIL;
	}
	_func_exit_;
	return _SUCCESS;
}

#endif


static uint protocol_read(struct intf_priv *pintfpriv, u32 addr, u32 cnt,  u8 *r_mem)
{
	u32 i,res;
//	u16 TempChar;
	struct reg_protocol_rd	readreq;
	struct reg_protocol_wt	writereq;

	uint (*_sdbus_write)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);	

	uint (*_sdbus_read)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);


	struct dvobj_priv * psdiodev = (struct dvobj_priv * )pintfpriv->intf_dev;

	u32 ftaddr= 0, b_cnt=0;

//	u32 opcode = 1;


	_adapter *padapter = psdiodev->padapter;
	
	_sdbus_write = padapter->pio_queue->intf.io_ops._sdbus_write;
	_sdbus_read = padapter->pio_queue->intf.io_ops._sdbus_read;
	
	
	memset(&readreq, 0, sizeof(struct reg_protocol_rd));
	memset(&writereq, 0, sizeof(struct reg_protocol_wt));
	
	//4 First: We submit a CMD53_Write with address =  Reg_W FIFO and Data = Reg_R_protocal
	//4 Second:  polling the HFFR register bit8
	//4 Third: if the ready bit == 1; submit a CMD53_Read with address =  Reg_R FIFO to read the data
	_func_enter_;
	
	readreq.NumOfTrans = 1;
	readreq.Reserved1 = 0;
	readreq.Reserved2 = 0;
	readreq.Reserved3 = 0;
	readreq.Reserved4= 0;
	readreq.BurstMode	= 0;
	readreq.ByteCount	= 0;
	readreq.WriteEnable = 0;
	readreq.FixOrContinuous = 0;
      	//readreq.Value= 0;
	switch(cnt){
		case 1:
			readreq.Byte1Access = 1;
			readreq.Byte2Access = 0;
			readreq.Byte4Access = 0;
			break;
		case 2:
			readreq.Byte1Access = 0;
			readreq.Byte2Access = 1;
			readreq.Byte4Access = 0;
			break;
		case 4:
			readreq.Byte1Access = 0;
			readreq.Byte2Access = 0;
			readreq.Byte4Access = 1;
			break;
		default:	
		readreq.BurstMode	= 1;
		readreq.ByteCount	= cnt;
			break;
	}

	//ReadReq.Reserved2	= 0;
	readreq.BusAddress	= cpu_to_le32(addr);

	b_cnt = sizeof(struct reg_protocol_rd);

	_cvrt2ftaddr(Reg_W_FIFO, &ftaddr);  

      _memcpy((u8 *)pintfpriv->io_rwmem, (u8 *)&readreq, sizeof(readreq));
	
	
	if ((_sdbus_write(pintfpriv, ftaddr, b_cnt)) == _FAIL)
	{
		#if 0
		DEBUG_ERR("=============SyncRegR Setp1 FAIL!============= \n ");
		#endif 
		_func_exit_;
		return _FAIL;
	}
	else
	{
		i = 100;
		b_cnt = 2 ;
#if 0
		do{// polling RRFF_READY bit; submit a read command to read the offset oo1Ah
						
			//if (addr == CR9346)
			//	NdisStallExecution(50);

			_cvrt2ftaddr(HFFR, &ftaddr); 
			if ((_sdbus_read(pintfpriv, ftaddr, 2)) == _SUCCESS) {				
				_memcpy(&TempChar, (u8 *)(pintfpriv->io_rwmem), 2);
			       	
							       	
				if ((TempChar ) & _HFFR_RRFF_READTY) { // submit the CMD53_R (Direct access)
#endif						
					if (cnt <= 4){
						b_cnt = sizeof(struct reg_protocol_wt); // when is not in BurstMode
						_cvrt2ftaddr(Reg_R_FIFO, &ftaddr); 
						if ((_sdbus_read(pintfpriv, ftaddr, b_cnt)) == _SUCCESS) {

							_memcpy(r_mem, (u8 *)(pintfpriv->io_rwmem + 0x0c), cnt);

							res = _SUCCESS;					
						} else {
							res = _FAIL;
							
							//RT_TRACE(COMP_FW_LB_ERROR, DBG_TRACE, ("=============SyncRegR Setp3 Fail!============= \n "));
						}
						_func_exit_;
						return res;
					}
							
					else { // burst mode : datalen:一定要是4的倍數
						if(cnt & 3) {
								
								//DEBUG_ERR("Error: (SDIO_SyncRegR_Func1: DataLen% 4)!=0  \n");
								return _FAIL  ;
						}

						ftaddr = 0;
						_cvrt2ftaddr(Reg_R_FIFO, &ftaddr);  
							
						if ((_sdbus_read(pintfpriv, ftaddr, (cnt + 12))) == _SUCCESS){
							
							_memcpy(r_mem, (u8 *)(pintfpriv->io_rwmem + 0x0c), cnt);
							res = _SUCCESS;
						}else{
							res = _FAIL ;
							
							//DEBUG_ERR(COMP_INIT, DBG_TRACE, ("=============SyncRegR Setp3 Burst mode Fail! =============\n "));
						}
						_func_exit_;
						return res;
					}
#if 0					
				}
				else
				{
					NdisStallExecution(100);
				}

				if(1 == i) {
					//DEBUG_ERR(COMP_FW_LB_ERROR, DBG_LOUD, ("=============SyncRegR Setp2 Read FA_FUNC1_HFFR is not ready! =============\n "));
				}
				
			}
			else
			{
				//DEBUG_ERR(COMP_FW_LB_ERROR, DBG_TRACE, ("=============SyncRegR Setp2 Read FA_FUNC1_HFFR FAIL!============= \n "));
			}
			
			i--;
			
		}while(i >0);
#endif		
		_func_exit_;

		return _FAIL ;
	}
}

static uint protocol_write(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *w_mem)
{

	ULONG	b_cnt;
	struct reg_protocol_wt	writereq;

	uint (*_sdbus_write)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);	
	uint (*_sdbus_read)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);

	struct dvobj_priv * 		psdiodev = (struct dvobj_priv * )pintfpriv->intf_dev;
	
	_adapter   *padapter = psdiodev->padapter;

	ULONG					ftaddr = 0;

	_sdbus_write = padapter->pio_queue->intf.io_ops._sdbus_write;
	_sdbus_read = padapter->pio_queue->intf.io_ops._sdbus_read;
	
	
	memset(&writereq, 0, sizeof(struct reg_protocol_wt));
	_cvrt2ftaddr(Reg_W_FIFO, (u32*)&ftaddr);



     writereq.Reserved2= 0;
      writereq.Reserved1= 0;
	writereq.NumOfTrans = 1;
	
	writereq.WriteEnable	= 1;
	writereq.BusAddress	= cpu_to_le32(addr);
	
	writereq.FixOrContinuous= 0;//continuous mode
	writereq.Reserved3= 0;
	writereq.Reserved4 = 0;
	_func_enter_;

		
	if(cnt <= 4){
		writereq.ByteCount	= 0;			// no used if not in Burst Mode
		writereq.BurstMode	= 0;
		
		if (cnt==1){
			writereq.Byte1Access	= 1;
			writereq.Byte2Access	= 0;
			writereq.Byte4Access	= 0;
		}
		if (cnt==2){
			writereq.Byte1Access	= 0;
			writereq.Byte2Access	= 1;
			writereq.Byte4Access	= 0;
		}
		if (cnt==4){
			writereq.Byte1Access	= 0;
			writereq.Byte2Access	= 0;
			writereq.Byte4Access	= 1;
		}
		
		//writereq.Value		= *((ULONG*)&pintfpriv->io_rwmem);
		writereq.Value		= *((ULONG*)w_mem);

		b_cnt 			= sizeof(struct reg_protocol_wt);

		_memcpy((u8 *)pintfpriv->io_rwmem, &writereq, b_cnt);

		if ((_sdbus_write(pintfpriv, ftaddr, b_cnt)) == _FAIL){
			_func_exit_;
			return _FAIL;
		}

		
	}
	else
	{ // burst mode
		if((cnt&3)){

			//DEBUG_ERR("Error: (SDIO_SyncRegR_Func1: DataLen% 4)!=0  \n");
			_func_exit_;
			return _FAIL;
		}


		//NdisMoveMemory(&pintfpriv->io_rwmem[12], pintfpriv->io_rwmem, cnt);
		_memcpy((u8 *)(&(pintfpriv->io_rwmem[12])), w_mem, cnt);
		writereq.ByteCount	= cnt;
		writereq.BurstMode	= 1;
		b_cnt = cnt + 12;

		_memcpy((u8 *)pintfpriv->io_rwmem, &writereq, 12);
		if ((_sdbus_write(pintfpriv, ftaddr, b_cnt)) == _FAIL){
			_func_exit_;
			return _FAIL;
		}

	}
	_func_exit_;
	return _SUCCESS;
}




#ifdef USE_SYNC_IRP

u8 sdio_read8(struct intf_hdl *pintfhdl, u32 addr)
{
	_irqL irqL;
	u32 res;
	u8	val;

	uint (*_sdbus_read)(struct intf_priv *, u32 , u32);
	
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;
	
	u8 *rwmem = (u8 *)(pintfpriv->io_rwmem);
	
	u32 ftaddr = 0 ;

	_sdbus_read = pintfhdl->io_ops._sdbus_read;

	_func_enter_;
	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
	//DEBUG_ERR("sdio_read8\n");
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);

	if ((_cvrt2ftaddr(addr, &ftaddr)) == _SUCCESS)
	{
		res = _sdbus_read(pintfpriv, ftaddr, 1);
		_memcpy(&val, rwmem, 1);
	}
	else
	{
		res = protocol_read(pintfpriv, addr, 1, &val);
	}

	_exit_hwio_critical(&pintfpriv->rwlock, &irqL);
	_func_exit_;

	return val;	

}

u16 sdio_read16(struct intf_hdl *pintfhdl, u32 addr)
{
	_irqL irqL;
	u16 val;
	u32	res;

	uint (*_sdbus_read)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);

	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;

	u8 *rwmem = (u8 *)(pintfpriv->io_rwmem);

	u32 ftaddr = 0 ;


	_sdbus_read = pintfhdl->io_ops._sdbus_read;

	_func_enter_;
	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
	//DEBUG_ERR("sdio_read16\n");

	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);

	if ((_cvrt2ftaddr(addr, &ftaddr)) == _SUCCESS)
	{
		res = _sdbus_read(pintfpriv, ftaddr, 2);
		_memcpy(&val, rwmem, 2);
	}
	else
	{
		res = protocol_read(pintfpriv, addr, 2, (UCHAR*)(&val));
	}
	
	_exit_hwio_critical(&pintfpriv->rwlock, &irqL);

	val = le16_to_cpu(val);

	_func_exit_;

	return val;	
}

u32 sdio_read32(struct intf_hdl *pintfhdl, u32 addr)
{
	_irqL irqL;
	
	u32 val,res;
	
	uint (*_sdbus_read)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);
	
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;

	u8 *rwmem = (u8 *)pintfpriv->io_rwmem;

	u32 ftaddr = 0 ;

	_sdbus_read = pintfhdl->io_ops._sdbus_read;

	_func_enter_;
	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...

	//DEBUG_ERR("sdio_read32\n");
	
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);

	if ((_cvrt2ftaddr(addr, &ftaddr)) == _SUCCESS)
	{
		res = _sdbus_read(pintfpriv, ftaddr, 4);
		if(res==_SUCCESS)
			_memcpy(&val, rwmem, 4);
		else 
			val=0;
	}
	else
	{
		res = protocol_read(pintfpriv, addr, 4, (UCHAR*)(&val));
	}

	
	_exit_hwio_critical(&pintfpriv->rwlock, &irqL);
        val = le32_to_cpu(val);
	_func_exit_;
	
	return val;	
}


void sdio_write8(struct intf_hdl *pintfhdl, u32 addr, u8 val)
{
	_irqL irqL;

	u32	res;

	uint (*_sdbus_write)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);
	
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;

	u8 *rwmem = (u8 *)pintfpriv->io_rwmem;

	u32 ftaddr = 0 ;

	_sdbus_write = pintfhdl->io_ops._sdbus_write;

	_func_enter_;
	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
	//DEBUG_ERR("sdio_write8 \n\n");

	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);


	_memcpy(rwmem ,&val, 1);

	if ((_cvrt2ftaddr(addr, &ftaddr)) == _SUCCESS)
	{
		res = _sdbus_write(pintfpriv, ftaddr, 1);
	}
	else
	{
		res = protocol_write(pintfpriv, addr, 1, &val);
	}

	
	_exit_hwio_critical(&pintfpriv->rwlock, &irqL);

	_func_exit_;

}



void sdio_write16(struct intf_hdl *pintfhdl, u32 addr, u16 val)
{
	_irqL irqL;

	u32	res;

	uint (*_sdbus_write)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);

	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;

	u8 *rwmem = (u8 *)pintfpriv->io_rwmem;

	u32 ftaddr = 0 ;

	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
		//DEBUG_ERR("sdio_write16 \n\n");

	_sdbus_write = pintfhdl->io_ops._sdbus_write;

	_func_enter_;
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);

	val = cpu_to_le16(val);

	_memcpy(rwmem ,&val, 2);

	if ((_cvrt2ftaddr(addr, &ftaddr)) == _SUCCESS)
	{
		res = _sdbus_write(pintfpriv, ftaddr, 2);
	}
	else
	{
		res = protocol_write(pintfpriv, addr, 2, (UCHAR*)(&val));
	}
	
	_exit_hwio_critical(&pintfpriv->rwlock, &irqL);

	_func_exit_;

}




void sdio_write32(struct intf_hdl *pintfhdl, u32 addr, u32 val)
{
	_irqL irqL;
	u32	res;

	uint (*_sdbus_write)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);
	
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;

	u8 *rwmem = (u8 *)pintfpriv->io_rwmem;

	u32 ftaddr = 0 ;

	_sdbus_write = pintfhdl->io_ops._sdbus_write;

	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
		//DEBUG_ERR("sdio_write32 \n\n");

	_func_enter_;

	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);

	val = cpu_to_le32(val);

	_memcpy(rwmem ,&val, 4);

	if ((_cvrt2ftaddr(addr, &ftaddr)) == _SUCCESS)
	{
		res = _sdbus_write(pintfpriv, ftaddr, 4);
	}
	else
	{
		res = protocol_write(pintfpriv, addr, 4, (UCHAR*)(&val));
	}

	
	_exit_hwio_critical(&pintfpriv->rwlock, &irqL);

	_func_exit_;

}





void sdio_read_mem(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	_irqL irqL;
	
	u32	res, r_cnt;

	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;

	u32	maxlen =  pintfpriv->max_iosz;

	_func_enter_;
	
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);

	cnt = _RND4(cnt);


do{
	r_cnt = (cnt > maxlen) ? maxlen: cnt;
	
	res = protocol_read(pintfpriv, addr, r_cnt, rmem);

	cnt -= r_cnt;
	rmem += r_cnt;
	addr += r_cnt;

}while(cnt > 0);

	_exit_hwio_critical(&pintfpriv->rwlock, &irqL);

	_func_exit_;

}




void sdio_read_port_byte(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	
	u32	res, r_cnt, ftaddr=0;

	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;

	uint (*_sdbus_read_bytes_to_membuf)(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);
	
//	_adapter* 	padapter = (_adapter*)(pintfhdl->adapter);
	
	u32	maxlen =  pintfpriv->max_recvsz;

//	struct dvobj_priv *psdio_dev = 	(struct dvobj_priv*)&padapter->dvobjpriv;

	_sdbus_read_bytes_to_membuf = pintfhdl->io_ops._sdbus_read_bytes_to_membuf;
	
	_func_enter_;
	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...

	if  (cnt == 0) {
		_func_exit_;
		return;
		
	}
		
	_cvrt2ftaddr(addr, &ftaddr);

	do
	{
		r_cnt = (cnt > maxlen) ?  maxlen : cnt;

		if (r_cnt ) {

			res = _sdbus_read_bytes_to_membuf(pintfpriv, ftaddr, r_cnt, rmem);

			if (res == _FAIL)
				break;
		}

		rmem += r_cnt;
		
		cnt -= r_cnt;

	}while(cnt > 0);
	
	_func_exit_;

}

#ifdef RD_BLK_MODE
void sdio_read_port_blk(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	
	u32	res, r_cnt, ftaddr;

	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;

	uint (*_sdbus_read_bytes_to_membuf)(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);
	uint (*_sdbus_read_blocks_to_membuf)(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);
	
	_adapter* 	padapter = (_adapter*)(pintfhdl->adapter);
	
	u32	maxlen =  pintfpriv->max_recvsz;

	struct dvobj_priv *psdio_dev = (struct dvobj_priv*)&padapter->dvobjpriv;

	_sdbus_read_bytes_to_membuf = pintfhdl->io_ops._sdbus_read_bytes_to_membuf;
	_sdbus_read_blocks_to_membuf = pintfhdl->io_ops._sdbus_read_blocks_to_membuf;
	
	_func_enter_;
	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...

	if  (cnt == 0)
	{
		_func_exit_;
		return;		
	}
		
	_cvrt2ftaddr(addr, &ftaddr);
	
	do {

		if (psdio_dev->block_mode)	
			r_cnt = ((cnt) >> 9) << 9;
		else
			r_cnt = (cnt > maxlen) ? maxlen:cnt;
		
		if (r_cnt)
		{

			if (psdio_dev->block_mode)	
				res = _sdbus_read_blocks_to_membuf(pintfpriv, ftaddr, r_cnt, rmem);
			else
				res = _sdbus_read_bytes_to_membuf(pintfpriv, ftaddr, r_cnt, rmem);

			if (res == _FAIL)
				break;
		}

		cnt -= r_cnt;
		rmem += r_cnt;

		if (psdio_dev->block_mode)	
		{
			if (cnt) 
				res = _sdbus_read_bytes_to_membuf(pintfpriv, ftaddr, cnt, rmem);
			break;
		}

	}while (cnt > 0);
	
	_func_exit_;

}
#endif

void sdio_read_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{

#ifdef RD_BLK_MODE	

	PADAPTER padapter = (PADAPTER)pintfhdl->adapter;
	struct registry_priv *pregistrypriv = &padapter->registrypriv;

	if ((pregistrypriv->chip_version == RTL8711_FPGA) ||(pregistrypriv->chip_version >= RTL8711_3rdCUT))
	{
		sdio_read_port_blk(pintfhdl, addr, cnt, rmem);
		return;
	}
	
#endif	
	
	sdio_read_port_byte(pintfhdl, addr, cnt, rmem);
		
}


void sdio_write_mem(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{

	_irqL irqL;

	u32	res, w_cnt;
	
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;
	u32	maxlen =  pintfpriv->max_iosz;
	_func_enter_;
	
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);

	cnt = _RND4(cnt);

	do{

		w_cnt = (cnt > maxlen) ? maxlen: cnt;

		res = protocol_write(pintfpriv, addr, w_cnt, wmem);

		cnt -= w_cnt;

		wmem += w_cnt;
		
		addr += w_cnt;
		
	}while(cnt > 0);

	_exit_hwio_critical(&pintfpriv->rwlock, &irqL);

	_func_exit_;

}



u32 sdio_write_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{

	u32		res,ftaddr=0,w_cnt;
	
	uint (*_sdbus_write_blocks_from_membuf)(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);

	uint (*_sdbus_write_bytes_from_membuf)(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);
		
	_adapter* 		adapter = (_adapter*)(pintfhdl->adapter);
	
	struct intf_priv 	*pintfpriv = pintfhdl->pintfpriv;

	u32 	maxlen =  pintfpriv->max_xmitsz;

	struct dvobj_priv	*psdio_dev = &adapter->dvobjpriv;

	_sdbus_write_blocks_from_membuf = pintfhdl->io_ops._sdbus_write_blocks_from_membuf;

	_sdbus_write_bytes_from_membuf  = pintfhdl->io_ops._sdbus_write_bytes_from_membuf;


	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
	
	_func_enter_;
	
	if (cnt == 0) {

		_func_exit_;

		return _SUCCESS;
	}

	_cvrt2ftaddr(addr, &ftaddr);


	do {

	
		if (psdio_dev->block_mode)	
			w_cnt = ((cnt) >> 9) << 9;
		else
			w_cnt = (cnt > maxlen) ? maxlen:cnt;
		
			if (w_cnt) {

				if (psdio_dev->block_mode)	
					res = _sdbus_write_blocks_from_membuf(pintfpriv, ftaddr, w_cnt, wmem);
				else
					res = _sdbus_write_bytes_from_membuf(pintfpriv, ftaddr, w_cnt, wmem);

				if (res == _FAIL)
					break;

			}

			cnt -= w_cnt;
			wmem += w_cnt;

			if (psdio_dev->block_mode)	{

				if (cnt) {

					res = _sdbus_write_bytes_from_membuf(pintfpriv, ftaddr, cnt, wmem);
					
				}
				break;
			}
				
		}while (cnt > 0);
	
	
	_func_exit_;

	return _SUCCESS;

}






void sdio_sync_protocol_rw(struct io_queue *pio_q)
{
      
      	_list	*phead, *plist;
//       u16 rff_rdy;
	u32 i, io_wrlen, ftaddr;
	u8 width;

	uint (*_sdbus_write)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);
	uint (*_sdbus_read)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);
	

	struct io_req	*pio_req;	

	struct intf_hdl *pintf = &(pio_q->intf);

	struct intf_priv *pintfpriv = pintf->pintfpriv;	

//	_adapter *padapter = (_adapter *)pintf->adapter;	

	u32	*pcmd = (u32 *)(pintfpriv->io_rwmem) + 1;

	_sdbus_write = pintf->io_ops._sdbus_write;

	_sdbus_read = pintf->io_ops._sdbus_read;
	

	_func_enter_;

	if ( (pintfpriv->intf_status != _IO_WAIT_COMPLETE) &&
		(pintfpriv->intf_status != _IO_WAIT_RSP) )
		return;//?
	
	io_wrlen = pintfpriv->io_wsz << 2;
	
	_cvrt2ftaddr(Reg_W_FIFO, &ftaddr);  	
	
	_sdbus_write(pintfpriv, ftaddr, io_wrlen);


	if(pintfpriv->intf_status == _IO_WAIT_RSP)		
	{
          i = 100;
#if 0		  
	   do{// polling RRFF_READY bit; submit a read command to read the offset oo1Ah
						
		     _cvrt2ftaddr(HFFR, &ftaddr); 
		     if ((_sdbus_read(pintfpriv, ftaddr, 2)) == _SUCCESS) {				
		 	      _memcpy(&rff_rdy, (u8 *)(pintfpriv->io_rwmem), 2);			       	
							       	
		  	 if ((rff_rdy ) & _HFFR_RRFF_READTY) {					
#endif			 	
					_cvrt2ftaddr(Reg_R_FIFO, &ftaddr); 
					_sdbus_read(pintfpriv, ftaddr, pintfpriv->io_rsz<<2);	
#if 0					
					break;
			}
			else	{
				NdisStallExecution(100);
			}				
		    }		   
			
		     i--;
			
		}while(i >0);
#endif
          pintfpriv->intf_status = _IO_WAIT_COMPLETE;
	}		


       phead = &(pio_q->processing);
	while(pintfpriv->intf_status == _IO_WAIT_COMPLETE)
	{
	       if ((is_list_empty(phead)) == _TRUE)
		   	break;		
		//free all irp in processing list...
		plist = get_next(phead);
		list_delete(plist);
		pio_req = LIST_CONTAINOR(plist, struct io_req, list);
		if (!(pio_req->command & _IO_WRITE_)) {

			pcmd += 2;
			// copy the contents to the read_buf...
			if (pio_req->command & _IO_BURST_)
			{
				_memcpy(pio_req->pbuf, (u8 *)pcmd,  (pio_req->command & _IOSZ_MASK_));
				pcmd += ((pio_req->command & _IOSZ_MASK_) >> 2);
			
			}
			else
			{
			       width = (u8)((pio_req->command & (_IO_BYTE_|_IO_HW_ |_IO_WORD_)) >> 10);
				_memcpy(pio_req->pbuf, (u8 *)pcmd, width);
				pcmd++;
			}

		}
		
            list_insert_tail(plist, &(pio_q->free_ioreqs));		

	}

       pintfpriv->intf_status = _IOREADY;
       pintf->do_flush = _FALSE;	

_func_exit_;
}


#else





//2006-11-07 yitsen
/*
**sdio_asny_protocol_read_callback: 
**update io_status; 
** fill back read_result to io_req and call io_req->callback 
*/
NTSTATUS sdio_asny_protocol_read_callback(PDEVICE_OBJECT	pdevobj, PIRP piorw_irp, PVOID pcallback_cnxt)
{
	_irqL		irqL;
	_list 	phead, plist;
	struct io_req	*pio_req;
	struct io_queue *pio_q = (struct io_queue *) pcallback_cnxt;
	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	_adapter *padapter = (_adapter *)pintf->adapter;
	u32	*pcmd = (u32 *)(pintfpriv->io_rwmem) + 1;
	_func_enter_;
	if (pintfpriv->intf_status != _IO_WAIT_COMPLETE)
		return STATUS_MORE_PROCESSING_REQUIRED;

	_enter_critical(&(pio_q->lock), &irqL);

	pintfpriv->intf_status = _IOREADY;
	
	phead = &(pio_q->processing);

	while(1)
	{

		if ((is_list_empty(phead)) == _TRUE)
			break;
		
		plist = get_next(phead);

		list_delete(plist);

		pio_req = LIST_CONTAINOR(plist, struct io_req, list);

		if(!(pio_req->command& _IO_WRITE_))
		{
			pcmd += 2;
			if((pio_req->command & _IO_BURST_))
			{
				_memcpy(pio_req->pbuf, pcmd ,(pio_req->command & _IOSZ_MASK_));
				pcmd += ((pio_req->command & _IOSZ_MASK_) >> 2);
			}
			else{
				_memcpy((u8 *)&(pio_req->val), (u8 *)pcmd, 4);
				pcmd++;
			}

			if (pio_req->command & _IO_SYNC_)
				_up_sema(&pio_req->sema);
			else
			{
	                     if(pio_req->_async_io_callback !=NULL)
				          pio_req->_async_io_callback(padapter, pio_req, pio_req->cnxt);
				list_insert_tail(plist, &(pio_q->free_ioreqs));
			}
			
		}
		

	}


	if (pintf->do_flush == _TRUE) 
	{		
		if ((is_list_empty(&(pio_q->pending))) == _TRUE)
			pintf->do_flush = _FALSE;	
		else
			bus_async_io(pio_q);
	}
	
	_exit_critical(&(pio_q->lock), &irqL);
	_func_exit_;
}



//2006-11-07 yitsen
/*
**sdio_async_protocol_read: rd hw to io_rwmem asynchronously
** using pintfpriv->io_rwmem; pintfpriv->pmdl; pintfpriv->sdrp; pintfpriv->piorw_irp
*/
void sdio_async_protocol_read(struct io_queue *pio_q)
{
	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	
	//PIRP		piorw_irp  = pintfpriv->piorw_irp; 
	ULONG	ftaddr = 0;
	u8 status;
	_func_enter_;
	_cvrt2ftaddr(Reg_R_FIFO, &ftaddr);

#ifdef NDIS51_MINIPORT
	IoReuseIrp(piowrite_irp, STATUS_SUCCESS);
#endif

	status = sdbus_async_read( pio_q, ftaddr, (pintfpriv->io_rsz << 2),
						&sdio_asny_protocol_read_callback);

	if(!status)
	{
		_adapter *padapter = (_adapter *)(pintf->adapter);

		DEBUG_ERR(("sdio_async_protocol_read fail \n"));
		padapter->bDriverStopped = TRUE;
		padapter->bSurpriseRemoved = TRUE;
	}
	_func_exit_;
}


//2006-11-07 yitsen
/*
**sdio_asny_protocol_write_callback: 
** update io_status; 
** submit an asynchronous sdio_rd cmd;
*/
NTSTATUS sdio_asny_protocol_write_callback(PDEVICE_OBJECT	pdevobj, PIRP piorw_irp, PVOID pcallback_cnxt)
{
	_irqL		irqL;
	_list 	head, plist;
	struct io_req	*pio_req;
	struct io_queue *pio_q = (struct io_queue *) pcallback_cnxt;
	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	_adapter *padapter = (_adapter *)pintf->adapter;

	_func_enter_;
	_enter_critical(&(pio_q->lock), &irqL);

	head = &(pio_q->processing);

	if (pintfpriv->intf_status == _IO_WAIT_RSP){
		
		pintfpriv->intf_status = _IO_WAIT_COMPLETE;
		sdio_async_protocol_read(pio_q);
	}
	else if(pintfpriv->intf_status == _IO_WAIT_COMPLETE)
	{
		pintfpriv->intf_status = _IOREADY;
		
		while(is_list_empty(head) != _TRUE)
		{
			plist = get_next(head);

			list_delete(plist);

			pio_req = LIST_CONTAINOR(plist, struct io_req, list);

			if (pio_req->command & _IO_SYNC_)
				_up_sema(&pio_req->sema);
			else
			{
	                   if(pio_req->_async_io_callback !=NULL) // need to discuss with J; yitsen; 20061108
				          pio_req->_async_io_callback(padapter, pio_req, pio_req->cnxt);
				list_insert_tail(plist, &(pio_q->free_ioreqs));
			}
		}
				 
		if (pintf->do_flush == _TRUE) {

			if ((is_list_empty(&(pio_q->pending))) == _TRUE)
				pintf->do_flush = _FALSE;	
			else
				bus_async_io(pio_q);
		}
		 
	}
	else {
		DEBUG_ERR(("sdio_asny_protocol_write_callback: State Machine is not allowed to be here\n"));
	}
		
	_exit_critical(&(pio_q->lock), &irqL);
	_func_exit_;
	return STATUS_MORE_PROCESSING_REQUIRED;

}

//2006-11-07 yitsen
/*
**sdio_async_protocol_write: wt io_rwmem to hw asynchronously
** using pintfpriv->io_rwmem; pintfpriv->pmdl; pintfpriv->sdrp; pintfpriv->piorw_irp
*/
void sdio_async_protocol_write	(struct io_queue *pio_q)
{
	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	
	//PIRP		piorw_irp  = pintfpriv->piorw_irp; 
	ULONG	ftaddr = 0;
	u8 status;

	_func_enter_;
	_cvrt2ftaddr(Reg_W_FIFO, &ftaddr);

#ifdef NDIS51_MINIPORT
	IoReuseIrp(piowrite_irp, STATUS_SUCCESS);
#endif

	status = sdbus_async_write( pio_q, ftaddr, (pintfpriv->io_wsz << 2),
						&sdio_asny_protocol_write_callback);

	if(!status)
	{
		_adapter *padapter = (_adapter *)(pintf->adapter);

		DEBUG_ERR(("sdio_async_protocol_write fail \n"));
		padapter->bDriverStopped = TRUE;
		padapter->bSurpriseRemoved = TRUE;
	}
	_func_exit_;
}


u32 sdio_write_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{
	_func_enter_;
	_func_exit_;
}


void sdio_read_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	_func_enter_;
	_func_exit_;
}

#endif





void sdio_set_intf_option(u32 *poption)
{
	_func_enter_;
	//*poption = ((*poption) | _INTF_ASYNC_);
	if (*poption & _INTF_ASYNC_)
		*poption = ((*poption) ^ _INTF_ASYNC_);
	_func_exit_;
	
}


void sdio_intf_hdl_init(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	return;
}

void sdio_intf_hdl_unload(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	return;
}
void sdio_intf_hdl_open(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	return;
}

void sdio_intf_hdl_close(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	return;
	
}

void sdio_set_intf_funs(struct intf_hdl *pintf_hdl)
{
	_func_enter_;
	pintf_hdl->intf_hdl_init = &sdio_intf_hdl_init;
	pintf_hdl->intf_hdl_unload = &sdio_intf_hdl_unload;
	pintf_hdl->intf_hdl_open = &sdio_intf_hdl_open;
	pintf_hdl->intf_hdl_close = &sdio_intf_hdl_close;
	_func_exit_;
}


/*

*/
uint sdio_init_intf_priv(struct intf_priv *pintfpriv)
{
	
	
//	struct dvobj_priv  *psdiodev = (struct dvobj_priv  *)pintfpriv->intf_dev;
	

#ifdef PLATFORM_OS_XP
	
	PMDL	pmdl = NULL;
	PSDBUS_REQUEST_PACKET  sdrp = NULL;

	PSDBUS_REQUEST_PACKET  recv_sdrp = NULL;

	PSDBUS_REQUEST_PACKET  xmit_sdrp = NULL;

#endif

	pintfpriv->intf_status = _IOREADY;
	pintfpriv->max_xmitsz =  512;
	pintfpriv->max_recvsz =  512;
	pintfpriv->max_iosz =  52; //64-12
	_func_enter_;
	_rwlock_init(&pintfpriv->rwlock);
	
#ifdef PLATFORM_MPIXEL
	pintfpriv->allocated_io_rwmem = (volatile u8 *)_malloc(pintfpriv->max_xmitsz ); 
	
    if (pintfpriv->allocated_io_rwmem == NULL)
    	goto sdio_init_intf_priv_fail;

	pintfpriv->io_rwmem = pintfpriv->allocated_io_rwmem ;
#else

	pintfpriv->allocated_io_rwmem = (volatile u8 *)_malloc(pintfpriv->max_xmitsz +4); 
	
    if (pintfpriv->allocated_io_rwmem == NULL)
    	goto sdio_init_intf_priv_fail;

	pintfpriv->io_rwmem = pintfpriv->allocated_io_rwmem +  4\
	- ( (u32)(pintfpriv->allocated_io_rwmem) & 3);
#endif


#ifdef PLATFORM_OS_XP

	pmdl = IoAllocateMdl((u8 *)pintfpriv->io_rwmem, pintfpriv->max_xmitsz , FALSE, FALSE, NULL);
	
	if(pmdl == NULL){
		//DEBUG_ERR(COMP_INIT, DBG_TRACE, ("MDL is NULL.\n "));	
		goto sdio_init_intf_priv_fail;
	}
	
	MmBuildMdlForNonPagedPool(pmdl);
	
	sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));

	 if(sdrp == NULL){
	 	//DEBUG_ERR(COMP_INIT, DBG_TRACE, ("SDRP is NULL.\n "));
		goto sdio_init_intf_priv_fail;
	 }

	pintfpriv->pmdl = pmdl;
	pintfpriv->sdrp = sdrp;

	recv_sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));

	if(recv_sdrp == NULL){
		goto sdio_init_intf_priv_fail;
	}


//	pintfpriv->recv_pmdl = NULL;

	pintfpriv->recv_sdrp = recv_sdrp;
	
	xmit_sdrp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));

	if(xmit_sdrp == NULL){
		goto sdio_init_intf_priv_fail;
	}


//	pintfpriv->xmit_pmdl = NULL;

	pintfpriv->xmit_sdrp = xmit_sdrp;
	


#ifdef USE_ASYNC_IRP//yitsen; 20061106

	pintfpriv->piowrite_irp = IoAllocateIrp((psdiodev->NextDeviceStackSize + 1) , FALSE);
	if(!pintfpriv->piowrite_irp)
		goto sdio_init_intf_priv_fail;
#endif


#endif	


	_func_exit_;
	return _SUCCESS;
			
sdio_init_intf_priv_fail:
	
	if (pintfpriv->allocated_io_rwmem)
		_mfree((u8 *)(pintfpriv->allocated_io_rwmem), pintfpriv->max_xmitsz +4);

#ifdef PLATFORM_OS_XP

	if (pmdl)
		IoFreeMdl(pmdl);

	if (sdrp)
		ExFreePool(sdrp);	

	if (recv_sdrp)
		ExFreePool(recv_sdrp);

	if (xmit_sdrp)
		ExFreePool(xmit_sdrp);

#endif
	_func_exit_;
	return _FAIL;
		
}

void sdio_unload_intf_priv(struct intf_priv *pintfpriv)
{
	_func_enter_;

#ifdef PLATFORM_OS_XP

	if (pintfpriv->pmdl) {
		IoFreeMdl(pintfpriv->pmdl);
		pintfpriv->pmdl = NULL;
	}
	
	if (pintfpriv->sdrp) {	
		ExFreePool(pintfpriv->sdrp);
		pintfpriv->sdrp = NULL;
	}
	
/*	if (pintfpriv->recv_pmdl) {
		IoFreeMdl(pintfpriv->recv_pmdl);
		pintfpriv->recv_pmdl = NULL;
	}
*/	
	if (pintfpriv->recv_sdrp) {	
		ExFreePool(pintfpriv->recv_sdrp);
		pintfpriv->recv_sdrp = NULL;
	}
/*
	if (pintfpriv->xmit_pmdl) {
		IoFreeMdl(pintfpriv->xmit_pmdl);
		pintfpriv->xmit_pmdl = NULL;
	}
*/	
	if (pintfpriv->xmit_sdrp) {	
		ExFreePool(pintfpriv->xmit_sdrp);
		pintfpriv->xmit_sdrp = NULL;
	}
#endif
	
	_mfree((u8 *)(pintfpriv->allocated_io_rwmem), pintfpriv->max_xmitsz +4);

	_func_exit_;
	
}



void sdio_set_intf_ops(struct _io_ops	*pops)
{
	memset((u8 *)pops, 0, sizeof(struct _io_ops));
	_func_enter_;
	
#ifdef PLATFORM_OS_LINUX

	pops->_cmd52r = &sdbus_cmd52r;
	pops->_cmd52w = &sdbus_cmd52w;
	pops->_sdbus_read = &sdbus_read;
	pops->_sdbus_read_bytes_to_membuf = &sdbus_read_bytes_to_recvbuf;	
        pops->_sdbus_read_blocks_to_membuf = &sdbus_read_blocks_to_recvbuf;		

	pops->_sdbus_write = &sdbus_write;
	pops->_sdbus_write_blocks_from_membuf = &sdbus_write_blocks_from_xmitbuf;
	pops->_sdbus_write_bytes_from_membuf = &sdbus_write_bytes_from_xmitbuf;

#endif

	

#ifdef PLATFORM_OS_MPIXEL

	pops->_cmd52r = &sdbus_cmd52r;
	pops->_cmd52w = &sdbus_cmd52w;
	pops->_sdbus_read = &sdbus_read;
	pops->_sdbus_read_bytes_to_membuf = &sdbus_read_bytes_to_recvbuf;
	pops->_sdbus_write = &sdbus_write;
	pops->_sdbus_write_blocks_from_membuf = &sdbus_write_blocks_from_xmitbuf;
	pops->_sdbus_write_bytes_from_membuf = &sdbus_write_bytes_from_xmitbuf;

#endif




#ifdef PLATFORM_OS_WINCE

	pops->_cmd52r = &sdbus_cmd52r_wince;
	pops->_cmd52w = &sdbus_cmd52w_wince;
	pops->_sdbus_read = &sdbus_read_wince;
	pops->_sdbus_read_bytes_to_membuf = &sdbus_read_bytes_to_membuf_wince;
	pops->_sdbus_write = &sdbus_write_wince;
	pops->_sdbus_write_blocks_from_membuf = &sdbus_write_blocks_from_membuf_wince;
	pops->_sdbus_write_bytes_from_membuf = &sdbus_write_bytes_from_membuf_wince;

#endif

#ifdef PLATFORM_OS_XP

	pops->_cmd52r = &sdbus_cmd52r_xp;
	pops->_cmd52w = &sdbus_cmd52w_xp;
	pops->_sdbus_read = &sdbus_read_xp;
	pops->_sdbus_read_bytes_to_membuf = &sdbus_read_bytes_to_membuf_xp;
	pops->_sdbus_read_blocks_to_membuf = &sdbus_read_blocks_to_membuf_xp;
	
	pops->_sdbus_write = &sdbus_write_xp;
	pops->_sdbus_write_blocks_from_membuf = &sdbus_write_blocks_from_membuf_xp;
	pops->_sdbus_write_bytes_from_membuf = &sdbus_write_bytes_from_membuf_xp;

#endif	


#ifdef USE_SYNC_IRP


	pops->_attrib_read = &sdio_attrib_read;
	pops->_read8 = &sdio_read8;
	pops->_read16 = &sdio_read16;
	pops->_read32 = &sdio_read32;
	pops->_read_mem = &sdio_read_mem;
			
	pops->_attrib_write = &sdio_attrib_write;
	pops->_write8 = &sdio_write8;
	pops->_write16 = &sdio_write16;
	pops->_write32 = &sdio_write32;
	pops->_write_mem = &sdio_write_mem;

	
	pops->_sync_irp_protocol_rw = &sdio_sync_protocol_rw;

#endif

	pops->_read_port = &sdio_read_port;
	pops->_write_port = &sdio_write_port;


#ifdef USE_ASYNC_IRP
      pops->_async_irp_protocol_read = &sdio_async_protocol_read;
      pops->_async_irp_protocol_write = &sdio_async_protocol_write;

#endif

	_func_exit_;
}

