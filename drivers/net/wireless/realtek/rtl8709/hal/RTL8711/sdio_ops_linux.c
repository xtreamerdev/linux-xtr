/******************************************************************************
* sdio_ops_linux.c                                                                                                                                 *
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
#define _SDIO_OPS_LINUX_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <osdep_intf.h>


#ifdef __SDIO_LIB_H___
#error "sdio lib is defined\n"
#endif

#include <linux/sdio/ctsystem.h>
#include <linux/sdio/sdio_busdriver.h>
#include <linux/sdio/_sdio_defs.h>
#include <linux/sdio/sdio_lib.h>


#ifdef PLATFORM_OS_LINUX

u8 sdbus_cmd52r(struct intf_priv *pintfpriv, u32 addr)
{
       SDIO_STATUS status;
       u8 readv8;
	struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)pintfpriv->intf_dev;
	PSDDEVICE psddev = pdvobjpriv->psddev;
        
	   
       _func_enter_;
	   
       /* read in 1 byte synchronously */
	status = SDLIB_IssueCMD52(psddev, 
	                              	    	 0,   /* function no. = 0, register space */
                                     	    	 (addr&0x1FFFF), /* 17-bit register address */
                                    	    	 &readv8, 
                                     	     	 1, /*1 byte*/
                                     	     	 _FALSE/*read*/
                                     	     	 );
	   
	  if (!SDIO_SUCCESS(status)) {

              DEBUG_ERR(("sdbus_cmd52r Synch CMD52 Read failed %x\n ", status));		
		DEBUG_ERR(("***** Addr = %x *****\n ",addr));		
                          
    	}  	

	_func_exit_;
	
	return  readv8;
	
}


void sdbus_cmd52w(struct intf_priv *pintfpriv, u32 addr, u8 val8)
{
	 SDIO_STATUS status;       
	 struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)pintfpriv->intf_dev;
	 PSDDEVICE psddev = pdvobjpriv->psddev;
	   
       _func_enter_;
	   
       /* write in 1 byte synchronously */
	status = SDLIB_IssueCMD52(psddev, 
	                              	    	0,    /* function 0 register space */
                                     	    	(addr&0x1FFFF), /* 17-bit register address */
                                    	    	 &val8, 
                                     	     	 1, /*1 byte*/
                                     	     	 _TRUE/*write*/
                                     	     	);
	   
	  if (!SDIO_SUCCESS(status)) {

              DEBUG_ERR(("sdbus_cmd52w Synch CMD52 Write failed %x\n ", status));		
		DEBUG_ERR(("***** Addr = %x *****\n ",addr));		
                          
    	} 
 	

	_func_exit_;
	
	return;
	
}


uint sdbus_read(struct intf_priv *pintfpriv, u32 addr, u32 cnt)
{
  	SDIO_STATUS status;
	PSDREQUEST  pReq = NULL;
	struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)pintfpriv->intf_dev;
      PSDDEVICE psddev = pdvobjpriv->psddev;      
      u8 *rwmem = (u8 *)pintfpriv->io_rwmem;
	  
        _func_enter_;
		
	 /* allocate request to send to host controller */
	pReq = SDDeviceAllocRequest(psddev);
    	if (pReq == NULL) {
			
           DEBUG_ERR(("SDIO_STATUS_NO_RESOURCES\n"));

           goto _sdbus_read_fail_exit;		  
           		          
   	}

	if (cnt > SDIO_MAX_LENGTH_BYTE_BASIS) {
		
           DEBUG_ERR(("SDIO_STATUS_INVALID_PARAMETER, %d\n",cnt));		             
			 
	    goto _sdbus_read_fail_exit;	
	}

	/* initialize the command bits, see CMD53 SDIO spec. */
    	SDIO_SET_CMD53_ARG(pReq->Argument,
                       			CMD53_READ,             /* read */
                       			SDDEVICE_GET_SDIO_FUNCNO(psddev), /* function number */
                       			CMD53_BYTE_BASIS,       /* set to byte mode */
                       			CMD53_INCR_ADDRESS,    /* or use CMD53_FIXED_ADDRESS */
                       			(addr&0x1FFFF),/* 17-bit register address */
                       			CMD53_CONVERT_BYTE_BASIS_BLK_LENGTH_PARAM(cnt)  /* bytes */
                       			);
	
    	pReq->pDataBuffer = rwmem;
    	pReq->Command = CMD53;
   	pReq->Flags = SDREQ_FLAGS_RESP_SDIO_R5 | SDREQ_FLAGS_DATA_TRANS;
    	pReq->BlockCount = 1;/* byte mode is always 1 block */
    	pReq->BlockLen = cnt ;

    	/* send the CMD53 out synchronously */
    	status = SDDEVICE_CALL_REQUEST_FUNC(psddev, pReq);
    	if (!SDIO_SUCCESS(status)) {

              DEBUG_ERR(("sdbus_read Synch CMD53 Read failed %x\n ", status));		
		DEBUG_ERR(("***** Addr = %x *****\n ",addr));
		DEBUG_ERR(("***** Length = %d *****\n ",cnt));	

		goto _sdbus_read_fail_exit;	
                          
    	}
		
       // free the request
       if (pReq != NULL)
    	      SDDeviceFreeRequest(psddev, pReq);

	_func_exit_;
       return _SUCCESS;       

_sdbus_read_fail_exit:

	_func_exit_;		  
       return _FAIL;

}

uint sdbus_read_bytes_to_recvbuf(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf)
{

       SDIO_STATUS status;
	PSDREQUEST  pReq = NULL;
	struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)pintfpriv->intf_dev;
       PSDDEVICE psddev = pdvobjpriv->psddev;          


        _func_enter_;
		
	 /* allocate request to send to host controller */
	pReq = SDDeviceAllocRequest(psddev);
    	if (pReq == NULL) {
			
           DEBUG_ERR(("SDIO_STATUS_NO_RESOURCES\n"));

           goto _sdbus_read_bytes_to_recvbuf_fail_exit;		  
           		          
   	}

	if (cnt > SDIO_MAX_LENGTH_BYTE_BASIS) {
		
           DEBUG_ERR(("SDIO_STATUS_INVALID_PARAMETER, %d\n",cnt));		             
			 
	    goto _sdbus_read_bytes_to_recvbuf_fail_exit;	
	}

	/* initialize the command bits, see CMD53 SDIO spec. */
    	SDIO_SET_CMD53_ARG(pReq->Argument,
                       			CMD53_READ,             /* read */
                       			SDDEVICE_GET_SDIO_FUNCNO(psddev), /* function number */
                       			CMD53_BYTE_BASIS,       /* set to byte mode */
                       			CMD53_INCR_ADDRESS,    /* or use CMD53_FIXED_ADDRESS */
                       			(addr&0x1FFFF),/* 17-bit register address */
                       			CMD53_CONVERT_BYTE_BASIS_BLK_LENGTH_PARAM(cnt)  /* bytes */
                       			);
	
    	pReq->pDataBuffer = pbuf;
    	pReq->Command = CMD53;
   	pReq->Flags = SDREQ_FLAGS_RESP_SDIO_R5 | SDREQ_FLAGS_DATA_TRANS;
    	pReq->BlockCount = 1;/* byte mode is always 1 block */
    	pReq->BlockLen = cnt ;

    	/* send the CMD53 out synchronously */
    	status = SDDEVICE_CALL_REQUEST_FUNC(psddev, pReq);
    	if (!SDIO_SUCCESS(status)) {

              DEBUG_ERR(("sdbus_read Synch CMD53 Read failed %x\n ", status));		
		DEBUG_ERR(("***** Addr = %x *****\n ",addr));
		DEBUG_ERR(("***** Length = %d *****\n ",cnt));	

		goto _sdbus_read_bytes_to_recvbuf_fail_exit;	
                          
    	}
		
       // free the request
       if (pReq != NULL)
    	      SDDeviceFreeRequest(psddev, pReq);

	_func_exit_;
       return _SUCCESS;       

_sdbus_read_bytes_to_recvbuf_fail_exit:

	_func_exit_;		  
       return _FAIL;


}


uint sdbus_read_blocks_to_recvbuf(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf)
{
       return _FAIL;
}


uint sdbus_write(struct intf_priv *pintfpriv, u32 addr, u32 cnt)
{

  	SDIO_STATUS status;
	PSDREQUEST  pReq = NULL;
	struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)pintfpriv->intf_dev;
      PSDDEVICE psddev = pdvobjpriv->psddev;      
      u8 *rwmem = (u8 *)pintfpriv->io_rwmem;


        _func_enter_;
		
	 /* allocate request to send to host controller */
	pReq = SDDeviceAllocRequest(psddev);
    	if (pReq == NULL) {
			
           DEBUG_ERR(("SDIO_STATUS_NO_RESOURCES\n"));

           goto _sdbus_write_fail_exit;		  
           		          
   	}

	if (cnt > SDIO_MAX_LENGTH_BYTE_BASIS) {
		
           DEBUG_ERR(("SDIO_STATUS_INVALID_PARAMETER, %d\n",cnt));		             
			 
	    goto _sdbus_write_fail_exit;	
	}

	/* initialize the command argument bits, see CMD53 SDIO spec. */
    	SDIO_SET_CMD53_ARG(pReq->Argument,
                                           CMD53_WRITE,             /* write */
                                           SDDEVICE_GET_SDIO_FUNCNO(psddev), /* function number */
                                           CMD53_BYTE_BASIS,       /* set to byte mode */
                                           CMD53_INCR_ADDRESS,    /* or use CMD53_FIXED_ADDRESS */
                                           (addr&0x1FFFF),/* 17-bit register address */
                                           CMD53_CONVERT_BYTE_BASIS_BLK_LENGTH_PARAM(cnt)  /* bytes */
                       			 );

	pReq->pDataBuffer = rwmem;
    	pReq->Command = CMD53;
    	pReq->Flags = SDREQ_FLAGS_RESP_SDIO_R5 | SDREQ_FLAGS_DATA_TRANS |
                  			SDREQ_FLAGS_DATA_WRITE;
		
    	pReq->BlockCount = 1; /* byte mode is always 1 block */
    	pReq->BlockLen = cnt;

    	/* send the CMD53 out synchronously */
    	status = SDDEVICE_CALL_REQUEST_FUNC(psddev, pReq);
    	if (!SDIO_SUCCESS(status)) {

              DEBUG_ERR(("sdbus_write Synch CMD53 Write failed %x\n ", status));		
		DEBUG_ERR(("***** Addr = %x *****\n ",addr));
		DEBUG_ERR(("***** Length = %d *****\n ",cnt));	

		goto _sdbus_write_fail_exit;	
                          
    	}
		
       // free the request
       if (pReq != NULL)
    	      SDDeviceFreeRequest(psddev, pReq);

	_func_exit_;
       return _SUCCESS;       

_sdbus_write_fail_exit:

	_func_exit_;		  
       return _FAIL;

}

uint sdbus_write_bytes_from_xmitbuf(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf)
{
     	SDIO_STATUS status;
	PSDREQUEST  pReq = NULL;
	struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)pintfpriv->intf_dev;
      PSDDEVICE psddev = pdvobjpriv->psddev;     


        _func_enter_;
		
	 /* allocate request to send to host controller */
	pReq = SDDeviceAllocRequest(psddev);
    	if (pReq == NULL) {
			
           DEBUG_ERR(("SDIO_STATUS_NO_RESOURCES\n"));

           goto _sdbus_write_bytes_from_xmitbuf_fail_exit;		  
           		          
   	}

	if (cnt > SDIO_MAX_LENGTH_BYTE_BASIS) {
		
           DEBUG_ERR(("SDIO_STATUS_INVALID_PARAMETER, %d\n",cnt));		             
			 
	    goto _sdbus_write_bytes_from_xmitbuf_fail_exit;	
	}

	/* initialize the command argument bits, see CMD53 SDIO spec. */
    	SDIO_SET_CMD53_ARG(pReq->Argument,
                                           CMD53_WRITE,             /* write */
                                           SDDEVICE_GET_SDIO_FUNCNO(psddev), /* function number */
                                           CMD53_BYTE_BASIS,       /* set to byte mode */
                                           CMD53_INCR_ADDRESS,    /* or use CMD53_FIXED_ADDRESS */
                                           (addr&0x1FFFF),/* 17-bit register address */
                                           CMD53_CONVERT_BYTE_BASIS_BLK_LENGTH_PARAM(cnt)  /* bytes */
                       			 );

	pReq->pDataBuffer = pbuf;
    	pReq->Command = CMD53;
    	pReq->Flags = SDREQ_FLAGS_RESP_SDIO_R5 | SDREQ_FLAGS_DATA_TRANS |
                  			SDREQ_FLAGS_DATA_WRITE;
		
    	pReq->BlockCount = 1; /* byte mode is always 1 block */
    	pReq->BlockLen = cnt;

    	/* send the CMD53 out synchronously */
    	status = SDDEVICE_CALL_REQUEST_FUNC(psddev, pReq);
    	if (!SDIO_SUCCESS(status)) {

              DEBUG_ERR(("sdbus_write Synch CMD53 Write failed %x\n ", status));		
		DEBUG_ERR(("***** Addr = %x *****\n ",addr));
		DEBUG_ERR(("***** Length = %d *****\n ",cnt));	

		goto _sdbus_write_bytes_from_xmitbuf_fail_exit;	
                          
    	}
		
       // free the request
       if (pReq != NULL)
    	      SDDeviceFreeRequest(psddev, pReq);

	_func_exit_;
       return _SUCCESS;       

_sdbus_write_bytes_from_xmitbuf_fail_exit:

	_func_exit_;		  
       return _FAIL;

}

uint sdbus_write_blocks_from_xmitbuf(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf)
{

      SDIO_STATUS status;
	   
	u16 BytesPerBlock =   (u16) 512;//
	u16 BlockCount = (u16) (cnt >> 9);//
		
	PSDREQUEST  pReq = NULL;
	struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)pintfpriv->intf_dev;
      PSDDEVICE psddev = pdvobjpriv->psddev;     
      

        _func_enter_;
		
	 /* allocate request to send to host controller */
	pReq = SDDeviceAllocRequest(psddev);
    	if (pReq == NULL) {
			
           DEBUG_ERR(("SDIO_STATUS_NO_RESOURCES\n"));

           goto _sdbus_write_blocks_fail_exit;		  
           		          
   	}

#if 0
	 status =  SDLIB_SetFunctionBlockSize(psddev, BytesPerBlock);

        if (!SDIO_SUCCESS(status)) {
           DBG_PRINT(SDDBG_WARN, ("SDIO Sample Function: block length set failed %d \n",
                                   status));
           goto _sdbus_write_blocks_fail_exit;
        }
#endif		
	
	SDIO_SET_CMD53_ARG(pReq->Argument,
                       			 CMD53_WRITE,             /* oepration */
                       			 SDDEVICE_GET_SDIO_FUNCNO(psddev), /* function number */
                       			 CMD53_BLOCK_BASIS,       /* set to block mode */
                       			 CMD53_INCR_ADDRESS,
                      			 (addr&0x1FFFF), /* 17-bit register address */
                       			 BlockCount  /* blocks */
                       			);

	pReq->pDataBuffer = pbuf;
    	pReq->Command = CMD53;
    	pReq->Flags = SDREQ_FLAGS_RESP_SDIO_R5 | SDREQ_FLAGS_DATA_TRANS |
                  			SDREQ_FLAGS_DATA_WRITE;       		
    			
	pReq->BlockCount = BlockCount;
	pReq->BlockLen = BytesPerBlock;
       

    	/* send the CMD53 out synchronously */
    	status = SDDEVICE_CALL_REQUEST_FUNC(psddev, pReq);
    	if (!SDIO_SUCCESS(status)) {

              DEBUG_ERR(("sdbus_write_blocks Synch CMD53 Write failed %x\n ", status));		
		DEBUG_ERR(("***** Addr = %x *****\n ",addr));
		DEBUG_ERR(("***** Length = %d *****\n ",cnt));	

		goto _sdbus_write_blocks_fail_exit;	
                          
    	}
		
       // free the request
       if (pReq != NULL)
    	      SDDeviceFreeRequest(psddev, pReq);

	_func_exit_;
       return _SUCCESS;       

_sdbus_write_blocks_fail_exit:

	_func_exit_;		  
       return _FAIL;

}


#endif
