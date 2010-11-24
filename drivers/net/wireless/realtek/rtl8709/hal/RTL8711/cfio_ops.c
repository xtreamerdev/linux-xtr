/******************************************************************************
* cfio_ops.c                                                                                                                                 *
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
#include <rtl8711_spec.h>


#ifdef PLATFORM_LINUX

#endif

#ifdef PLATFORM_WINDOWS
//#include <io.h>
#include <osdep_intf.h>
//#include <xmit.h>

#endif

typedef enum {
       IO_READ,
       IO_WRITE,
} IO_TRANSFER_TYPE;

#define HAAR_POLLING_CNT	100

struct _SyncContext{
        struct intf_priv *pintfpriv;
	 u32 lbusaddr;	
	 u32 bytecnt;
	 u8 *pdata;
};

static ULONG HAARSetting(
	const u32 ByteCount, 
	const IO_TRANSFER_TYPE RW,
	const u32 PhysicalLexraBusAddr
	)
{

	ULONG value = 0;
	_func_enter_;
	//bit 31~30 Access Width
	switch(ByteCount)
	{
		case 4: //HAAR access width = 32
			value |= BIT(31);
			break;
		case 2: //HAAR access width = 16
			value |= BIT(30);
			break;
		case 1: //HAAR access width = 8		       
			break;
		default:
			//RT_TRACE(COMP_DBG, DBG_LOUD, ("HAARSetting: Parameter error\n"));
			break;
	}
	
	if(RW == IO_READ){
		value |= BIT(29);
	}

	value |= PhysicalLexraBusAddr;
	_func_exit_;
	return value;
}


#ifdef PLATFORM_WINDOWS

static uint cfbus_read(void *context)
{
#ifdef PLATFORM_WINDOWS

       u32 iobase_addr, len, addr, HAARValue, Read_Data;
	u8 *pdata;    
       USHORT readCF_HAAR;         
	u32 counter=0;   
	struct _SyncContext *psynccontext = (struct _SyncContext *)context;
       struct intf_priv *pintfpriv = (struct intf_priv *)psynccontext->pintfpriv;       
	struct dvobj_priv * pcfiodev = (struct dvobj_priv *)pintfpriv->intf_dev;
	//u8 *rwmem = (u8 *)pintfpriv->rw_mem;
	
	iobase_addr = pcfiodev->io_base_address;
	len = psynccontext ->bytecnt;
	addr = psynccontext->lbusaddr;	
	pdata = psynccontext->pdata;
	_func_enter_;
      //if the address is in HCI local register, to skip it
      if( addr >= RTL8711_HCICTRL_ && addr <= (RTL8711_HCICTRL_+0x1FFFF) )
      {
       	_func_exit_;
           return _FAIL;
       }

	HAARValue = 	HAARSetting(len, IO_READ, addr);		
	// 1.Write HAAR
	NdisRawWritePortUlong(
			(ULONG)(iobase_addr+HAAR), 
			HAARValue);
	
	// 2.Polling HAAR Ready	
	do
      {
	     if(counter > HAAR_POLLING_CNT){			
		       //RT_TRACE(COMP_DBG, DBG_LOUD, ("\n\nCFIOReadReg: Poll HAAR Ready too long, LBus = 0x%X checkRegRWCnt(1) = %d currentIRQL = %x, read HAAR=%x, HAARValue=%x.\n\n",c->LexraBusAddr, checkRegRWCnt, currentIRQL, test32, HAARValue ));							
		       _func_exit_;
                     return _FAIL;
		 }		

		if(addr == CR9346)
			NdisStallExecution(50); 

		//Considering CFIO default mode is 16-bit mode, we use 16-bit reading
		NdisRawReadPortUshort(
			(ULONG)(iobase_addr+HAAR+0x0002),
			&readCF_HAAR);
		
		counter++;
		
	}while(((readCF_HAAR >> 14) & 0x3) != 0x3);   //while READY has not been set

	// 3.Read HRADR
	NdisRawReadPortUlong(
		(ULONG)(iobase_addr+HRADR), (ULONG *)&Read_Data);

	NdisMoveMemory(pdata , (u8 *)&Read_Data, len);  
	
	

#endif	
	_func_exit_;
	return _SUCCESS;	

}


static uint cfbus_write(void *context)
{
#ifdef PLATFORM_WINDOWS

       u32 iobase_addr, len, addr, HAARValue, Write_Data;       
       USHORT readCF_HAAR;       
	u32 counter=0;   
       struct _SyncContext *psynccontext = (struct _SyncContext *)context;
       struct intf_priv *pintfpriv = (struct intf_priv *)psynccontext->pintfpriv;             
	struct dvobj_priv * pcfiodev = (struct dvobj_priv *)pintfpriv->intf_dev;
	//u8 *rwmem = (u8 *)pintfpriv->rw_mem;

	iobase_addr = pcfiodev->io_base_address;
	len = psynccontext ->bytecnt;
	addr = psynccontext->lbusaddr;	
	
	NdisMoveMemory((u8*)&Write_Data , psynccontext->pdata, len);

	_func_enter_;	
	//if the address is in HCI local register, to skip it
      if( addr >= RTL8711_HCICTRL_ && addr <= (RTL8711_HCICTRL_+0x1FFFF) )
      {
      	_func_exit_;
           return _FAIL;
}


	// 1.write data 	
	NdisRawWritePortUlong(	(ULONG)(iobase_addr+HWADR), Write_Data);

	
       HAARValue = 	HAARSetting(len, IO_WRITE, addr);	
	// 2.write HAAR
	NdisRawWritePortUlong(	(ULONG)(iobase_addr+HAAR), 
	                                   HAARValue);

	// 3.polling HAAR Ready	
	do
{
		if(counter > HAAR_POLLING_CNT){	
			//RT_TRACE(COMP_DBG, DBG_LOUD, ("\n\nCFIOWriteReg: Poll HAAR Ready too long, LBus = 0x%X checkRegRWCnt (1) = %d currentIRQL = %x, read HAAR=%x, HAARValue=%x.\n\n",c->LexraBusAddr, checkRegRWCnt, currentIRQL, test32, HAARValue ));			
			_func_exit_;
                     return _FAIL;
		}

		if(addr == CR9346) 
			NdisStallExecution(50); 

		//Considering CFIO default mode is 16-bit mode, we use 16-bit reading
		NdisRawReadPortUshort((ULONG)(iobase_addr+HAAR+0x0002),
			                              &readCF_HAAR);

		counter++;

	}
	while(((readCF_HAAR >> 14) & 0x3) != 0x3);   //while READY has not been set

#endif	
	_func_exit_;
	return _SUCCESS;

}


#endif


u8 cf_read8(struct intf_hdl *pintfhdl, u32 addr)
{
	_irqL irqL;
	uint	res;
	u8	val;
	struct _SyncContext synccontext;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;
	//u8 *rwmem = pintfpriv->io_rwmem;
	struct dvobj_priv * pcfiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);
	u32 iobase_addr = pcfiodev->io_base_address;
	

	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
	
	_func_enter_;
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);	

#ifdef PLATFORM_WINDOWS

     
      if( addr >= RTL8711_HCICTRL_ && addr <= (RTL8711_HCICTRL_+0x1FFFF) )
      {  //the address is in HCI local register 

          addr = (addr&0x00003FFF);
	   NdisRawReadPortUchar((u32)(iobase_addr+addr), (u8 *)&val);

		  
       }else{

          synccontext.pintfpriv = pintfpriv;
          synccontext.lbusaddr = addr;
	   synccontext.bytecnt = 1; // 1-byte
	   synccontext.pdata=(u8 *)&val;
		
	   irqL = KeGetCurrentIrql();	   

	   if ( irqL <= DISPATCH_LEVEL )
		res = NdisMSynchronizeWithInterrupt(&pcfiodev->interrupt, cfbus_read, (void *)&synccontext);				
	   else//IRQL > DISPATCH_LEVEL
	       res = cfbus_read((void *)&synccontext);

		//NdisMoveMemory(&val, rwmem, 1);
       }

#endif	
	
       _exit_hwio_critical(&pintfpriv->rwlock, &irqL);    
	_func_exit_;
	return val;	
	
}

u16 cf_read16(struct intf_hdl *pintfhdl, u32 addr)
{
	_irqL irqL;
	uint	res;
	u16 val;
	struct _SyncContext synccontext;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;	
	struct dvobj_priv * pcfiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);	
	u32 iobase_addr = pcfiodev->io_base_address;
	_func_enter_;
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);	

#ifdef PLATFORM_WINDOWS

      if( addr >= RTL8711_HCICTRL_ && addr <= (RTL8711_HCICTRL_+0x1FFFF) )
      {  //the address is in HCI local register 

          addr = (addr&0x00003FFF);
	   NdisRawReadPortUshort((u32)(iobase_addr+addr), (u16 *)&val);

	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
	
       }else{      
	   synccontext.pintfpriv = pintfpriv;
          synccontext.lbusaddr = addr;
	   synccontext.bytecnt = 2; // 2-byte
	   synccontext.pdata=(u8 *)&val;	       

	   irqL = KeGetCurrentIrql();	   

	   if ( irqL <= DISPATCH_LEVEL )
		res = NdisMSynchronizeWithInterrupt(&pcfiodev->interrupt, cfbus_read, (void *)&synccontext);				
	   else//IRQL > DISPATCH_LEVEL
	       res = cfbus_read((void *)&synccontext);      

       }

#endif		

       _exit_hwio_critical(&pintfpriv->rwlock, &irqL);    
	_func_exit_;
	return val;	
}

u32 cf_read32(struct intf_hdl *pintfhdl, u32 addr)
{
	_irqL irqL;
	uint	res;
	u32 val;
	struct _SyncContext synccontext;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;	
	struct dvobj_priv * pcfiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);	
	u32 iobase_addr = pcfiodev->io_base_address;
	_func_enter_;
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);	

#ifdef PLATFORM_WINDOWS    

      if( addr >= RTL8711_HCICTRL_ && addr <= (RTL8711_HCICTRL_+0x1FFFF) )
      {  //the address is in HCI local register 
      
          addr = (addr&0x00003FFF);
	   NdisRawReadPortUlong((u32)(iobase_addr+addr), (u32 *)&val);


       }else{      
      
          synccontext.pintfpriv = pintfpriv;
          synccontext.lbusaddr = addr;
	   synccontext.bytecnt = 4; // 4-byte
	   synccontext.pdata=(u8 *)&val;		

	   irqL = KeGetCurrentIrql();	   
	
	   if ( irqL <= DISPATCH_LEVEL )
		res = NdisMSynchronizeWithInterrupt(&pcfiodev->interrupt, cfbus_read, (void *)&synccontext);				
	   else//IRQL > DISPATCH_LEVEL
	       res = cfbus_read((void *)&synccontext);       

       }

#endif	

       _exit_hwio_critical(&pintfpriv->rwlock, &irqL);    
	_func_exit_;	
	return val;	
}


void cf_write8(struct intf_hdl *pintfhdl, u32 addr, u8 val)
{
	_irqL irqL;
	uint	res;
	struct _SyncContext synccontext;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;	
	//u8 *rwmem = pintfpriv->io_rwmem;
	struct dvobj_priv * pcfiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);
	u32 iobase_addr = pcfiodev->io_base_address;

	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
	_func_enter_;
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);	

#ifdef PLATFORM_WINDOWS

      if( addr >= RTL8711_HCICTRL_ && addr <= (RTL8711_HCICTRL_+0x1FFFF) )
      {  //the address is in HCI local register 
      
          addr = (addr&0x00003FFF);
	   NdisRawWritePortUchar((u32)(iobase_addr+addr), val);

		  
       }else{    
       
          synccontext.pintfpriv = pintfpriv;
          synccontext.lbusaddr = addr;
	   synccontext.bytecnt = 1; // 1-byte    
	   synccontext.pdata=(u8 *)&val;		
       
	   //NdisMoveMemory(rwmem ,&val, pintfpriv->len);
	
          irqL = KeGetCurrentIrql();	   

          if ( irqL <= DISPATCH_LEVEL )
		res = NdisMSynchronizeWithInterrupt(&pcfiodev->interrupt, cfbus_write, (void *)&synccontext);				
	   else//IRQL > DISPATCH_LEVEL
	       res = cfbus_write((void *)&synccontext);

       }

#endif	
	
       _exit_hwio_critical(&pintfpriv->rwlock, &irqL);
	_func_exit_;

}



void cf_write16(struct intf_hdl *pintfhdl, u32 addr, u16 val)
{
	_irqL irqL;
	uint	res;	
	struct _SyncContext synccontext;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;	
	struct dvobj_priv * pcfiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);	
	u32 iobase_addr = pcfiodev->io_base_address;
	_func_enter_;
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);	

#ifdef PLATFORM_WINDOWS       
	
      if( addr >= RTL8711_HCICTRL_ && addr <= (RTL8711_HCICTRL_+0x1FFFF) )
      {  //the address is in HCI local register 

          addr = (addr&0x00003FFF);
	   NdisRawWritePortUshort((u32)(iobase_addr+addr), val);


       }else{ 

          synccontext.pintfpriv = pintfpriv;
          synccontext.lbusaddr = addr;
	   synccontext.bytecnt = 2; // 2-byte    
	   synccontext.pdata=(u8 *)&val;			

          irqL = KeGetCurrentIrql();	   

          if ( irqL <= DISPATCH_LEVEL )
		res = NdisMSynchronizeWithInterrupt(&pcfiodev->interrupt, cfbus_write, (void *)&synccontext);				
	   else//IRQL > DISPATCH_LEVEL
	       res = cfbus_write((void *)&synccontext);
	
      }

#endif	

      _exit_hwio_critical(&pintfpriv->rwlock, &irqL); 
	_func_exit_;

}

void cf_write32(struct intf_hdl *pintfhdl, u32 addr, u32 val)
{
	_irqL irqL;
	uint	res;	
	struct _SyncContext synccontext;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;	
	struct dvobj_priv * pcfiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);	
	u32 iobase_addr = pcfiodev->io_base_address;
	_func_enter_;
	_enter_hwio_critical(&pintfpriv->rwlock, &irqL);	

#ifdef PLATFORM_WINDOWS       

      if( addr >= RTL8711_HCICTRL_ && addr <= (RTL8711_HCICTRL_+0x1FFFF) )
      {  //the address is in HCI local register 

          addr = (addr&0x00003FFF);
	   NdisRawWritePortUlong((u32)(iobase_addr+addr), val);
		  
       }else{ 
	
          synccontext.pintfpriv = pintfpriv;
          synccontext.lbusaddr = addr;
	   synccontext.bytecnt = 4; // 4-byte    
	   synccontext.pdata=(u8 *)&val;			

          irqL = KeGetCurrentIrql();	   

          if ( irqL <= DISPATCH_LEVEL )
		res = NdisMSynchronizeWithInterrupt(&pcfiodev->interrupt, cfbus_write, (void *)&synccontext);				
	   else//IRQL > DISPATCH_LEVEL
	       res = cfbus_write((void *)&synccontext);

       }

#endif		

       _exit_hwio_critical(&pintfpriv->rwlock, &irqL); 
	_func_exit_;

}


void cf_read_mem(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	_irqL irqL;
	uint	res;
	u32	r_cnt;	
	//u32 pageidx;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;
	//u16	maxlen =  pintfpriv->max_Regsz;
	_func_enter_;

	
#ifdef PLATFORM_WINDOWS

	// implement readMem

	//NdisMoveMemory(rmem, pintfpriv->rw_mem , cnt);


#endif
	_func_exit_;

}

void cf_read_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	_irqL irqL;
	u32 	i, r_cnt, loop_cnt;	
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;
	u32 *pdata = (u32 *)rmem;
	u32	maxlen =  pintfpriv->max_recvsz;
	struct dvobj_priv * pcfiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);	
	u32 iobase_addr = pcfiodev->io_base_address;
	
	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
	_func_enter_;
	if ((cnt == 0) || (addr != HRFF0DR))
		return;
	
#ifdef PLATFORM_WINDOWS

       addr = (addr&0x00003FFF);//Convert addr to CFIO Interface local offset addr
       do
	{
		r_cnt = (cnt > maxlen) ? maxlen: cnt;//actually cnt must be <= maxlen		
		loop_cnt = (r_cnt/4) + (((r_cnt%4) >0)?1:0);

              for(i=0; i<loop_cnt; i++)
              {
                  NdisRawReadPortUlong((u32)(iobase_addr+addr), (u32 *)pdata);
		    pdata++;				  
               }              	

		cnt -= r_cnt;		

	}while(cnt > 0);
	
#endif	
	_func_exit_;
	return;

}


void cf_write_mem(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{
	_irqL irqL;
	uint	res;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;
	//u16	maxlen =  pintfpriv->max_Regsz;


	_func_enter_;
#ifdef PLATFORM_WINDOWS




	//NdisMoveMemory(pintfpriv->rw_mem, wmem, cnt);


#endif

	_func_exit_;

}

uint cf_write_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{
       _irqL 		irqL;
	u32	i, w_cnt, loop_cnt;	
	_adapter* 		adapter = (_adapter*)(pintfhdl->adapter);
	struct intf_priv 	*pintfpriv = pintfhdl->pintfpriv;
	u32 *pdata = (u32 *)wmem;
	u32 	maxlen =  pintfpriv->max_xmitsz;
	struct dvobj_priv * pcfiodev = (struct dvobj_priv * )(pintfpriv->intf_dev);	
	u32 iobase_addr = pcfiodev->io_base_address;	

	// Please remember, you just can't only use lock/unlock to 
	// protect the rw functions...
	// since, i/o is quite common in call-back and isr routines...
	_func_enter_;
	if ((cnt == 0) || (addr != HWFF0DR)) {
		_func_exit_;
		return _SUCCESS;
	}
	
	//_enter_critical(&pintfpriv->rwlock, &irqL);

#ifdef PLATFORM_WINDOWS

	addr = (addr&0x00003FFF);//Convert addr to CFIO Interface local offset addr
	do
	{
		w_cnt = (cnt > maxlen) ? maxlen: cnt;//actually cnt must be <= maxlen		
		loop_cnt = (w_cnt/4) + (((w_cnt%4) >0)?1:0);
		
              for(i=0; i<loop_cnt; i++)
              {
                  NdisRawWritePortUlong((u32)(iobase_addr+addr), (u32)(*pdata));
		    pdata++;				  
               }              	
			  
		cnt -= w_cnt;		

	}while(cnt > 0);

#endif	
	
	//_exit_critical(&pintfpriv->rwlock, &irqL);
	_func_exit_;
	return _SUCCESS;

}




void cf_set_intf_option(u32 *poption)
{
	
	*poption = 0; //Sync, Direct rw 
	_func_enter_;
	_func_exit_;
	
}


void cf_intf_hdl_init(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	return;
}

void cf_intf_hdl_unload(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	return;
}
void cf_intf_hdl_open(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	return;
}

void cf_intf_hdl_close(u8 *priv)
{
	
	_func_enter_;
	_func_exit_;
	return;
	
}

void cf_set_intf_funs(struct intf_hdl *pintf_hdl)
{
	_func_enter_;
	pintf_hdl->intf_hdl_init = &cf_intf_hdl_init;
	pintf_hdl->intf_hdl_unload = &cf_intf_hdl_unload;
	pintf_hdl->intf_hdl_open = &cf_intf_hdl_open;
	pintf_hdl->intf_hdl_close = &cf_intf_hdl_close;

	_func_exit_;
}


/*

*/
uint cf_init_intf_priv(struct intf_priv *pintfpriv)
{
	_func_enter_;
       pintfpriv->max_xmitsz =  512;
	pintfpriv->max_recvsz =  512;
	pintfpriv->max_iosz =  128;
	pintfpriv->intf_status =  0;
	pintfpriv->io_wsz = 0;
	pintfpriv->io_rsz = 0;	

      _rwlock_init(&pintfpriv->rwlock);
	
	pintfpriv->allocated_io_rwmem = (volatile u8 *)_malloc(pintfpriv->max_xmitsz +4); 
	
       if (pintfpriv->allocated_io_rwmem == NULL)
    	goto cfio_init_intf_priv_fail;

	pintfpriv->io_rwmem = pintfpriv->allocated_io_rwmem +  4\
	- ( (u32)(pintfpriv->allocated_io_rwmem) & 3);

	_func_exit_;
	return _SUCCESS;

cfio_init_intf_priv_fail:
	
	if (pintfpriv->allocated_io_rwmem)
		_mfree((u8 *)pintfpriv->allocated_io_rwmem, pintfpriv->max_xmitsz +4);
	_func_exit_;
	return _FAIL; 

}

void cf_unload_intf_priv(struct intf_priv *pintfpriv)
{
	_func_enter_;
	_mfree((u8 *)pintfpriv->allocated_io_rwmem, pintfpriv->max_xmitsz +4);
	_func_exit_;
}



void cf_set_intf_ops(struct _io_ops	*pops)
{
	_func_enter_;
	memset((u8 *)pops, 0, sizeof(struct _io_ops));
	pops->_read8 = &cf_read8;
	pops->_read16 = &cf_read16;
	pops->_read32 = &cf_read32;
	pops->_read_mem = &cf_read_mem;
	pops->_read_port = &cf_read_port;
			
	pops->_write8 = &cf_write8;
	pops->_write16 = &cf_write16;
	pops->_write32 = &cf_write32;
	pops->_write_mem = &cf_write_mem;
	pops->_write_port = &cf_write_port;
	_func_exit_;
}


