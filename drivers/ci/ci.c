/*=============================================================
 * Copyright C        Realtek Semiconductor Corporation, 2004 *
 * All rights reserved.                                       *
 *============================================================*/

/*======================= Description ========================*/
/**
 * @file
 * For RTD2880's CI APIs.
 * Generic functions for EN50221 CA interfaces.
 *
 * @Author Moya Yu
 * @date 2004/12/12
 * @version { 1.0 }
 * @ingroup dr_ci
 *
 */

/*======================= CVS Headers =========================
  $Header: /cvsroot/dtv/linux-2.6/drivers/rtd2880/ci/ci.c,v 1.4 2005/11/01 06:02:06 ycchen Exp $

  $Log: ci.c,v $
  Revision 1.4  2005/11/01 06:02:06  ycchen
  +: add a prefix 'CI_DRV' in debug message
  +: get slot count from .config

  Revision 1.3  2005/01/18 01:45:12  ghyu
  +: Move these header files to include directory.


 *=============================================================*/

/*====================== Include files ========================*/
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/rwsem.h>
#include "ci.h"
#include "pcmcia.h"
#include "ci_dbg.h"

//add by keith
//#define SEND_MSG_TO_AP 1

#ifdef SEND_MSG_TO_AP
#include <rtd2880/sc/kScMsgTypes.h>
#include <rtd2880/sc/kSocket.h>
#include <rtd_messages.h>
#endif

#define CI_MAJOR       129


static void ci_thread_wakeup(struct dvb_ca_private *ca);
static int ci_read_data(int slot, u8 * ebuf, int ecount);
static int ci_write_data(int slot, u8 * ebuf, int ecount);

static struct dvb_ca_private *ca;

static const char*  cam_state_str[] = 
{
    "NONE",             // 0 : DVB_CA_SLOTSTATE_NONE
    "UNINITIALISED",    // 1 : DVB_CA_SLOTSTATE_UNINITIALISED
    "RUNNING",          // 2 : DVB_CA_SLOTSTATE_RUNNING
    "INVALID",          // 3 : DVB_CA_SLOTSTATE_INVALID
    "WAITREADY",        // 4 : DVB_CA_SLOTSTATE_WAITREADY
    "VALIDATE",         // 5 : DVB_CA_SLOTSTATE_VALIDATE
    "WAITFR",           // 6 : DVB_CA_SLOTSTATE_WAITFR
    "LINKINIT",         // 7 : DVB_CA_SLOTSTATE_LINKINIT    
};

#ifdef SEND_MSG_TO_AP

static int ci_sendMessageToAP(INT32 msg)
{
	SC_MSGBUF msgBuf;

	memset(&msgBuf, 0, sizeof(SC_MSGBUF));
	msgBuf.msgType = SC_CI_MSG;
	msgBuf.msgSubType = msg;	
	msgBuf.isValidate = TRUE;

	printk("ci_sendMessageToAP = %d \n", msgBuf.msgSubType);
	return Kernel_SendNlMsg( &msgBuf );	
}
#else
    #define ci_sendMessageToAP(msg)
#endif


/**
 * Safely find needle in haystack.
 *
 * @param haystack Buffer to look in.
 * @param hlen Number of bytes in haystack.
 * @param needle Buffer to find.
 * @param nlen Number of bytes in needle.
 * @return Pointer into haystack needle was found at, or NULL if not found.
 */
static u8 *findstr(u8 * haystack, int hlen, u8 * needle, int nlen)
{
	int i;

	if (hlen < nlen)
		return NULL;

	for (i = 0; i <= hlen - nlen; i++) {
		if (!strncmp(haystack + i, needle, nlen))
			return haystack + i;
	}

	return NULL;
}


/* ******************************************************************************** */
/* EN50221 physical interface functions */




int ci_update_slot_state(struct dvb_ca_private *ca, unsigned char slot, unsigned char new_state)
{
    if (slot >= ca->slot_count)
    {
        ci_warning("upadate slot%d state failed - invalid slot number\n", slot);
        return -1;
    }        
    
    if (ca->slot_info[slot].slot_state != new_state)
    {
        ci_info("slot%d state change : %d (%s) --> %d (%s)\n", 
                    slot, 
                    ca->slot_info[slot].slot_state, 
                    cam_state_str[ca->slot_info[slot].slot_state],
                    new_state, 
                    cam_state_str[new_state]
                );
                    
        ca->slot_info[slot].slot_state = new_state;
    }    
    
    return 0;
}


unsigned char ci_get_slot_state(struct dvb_ca_private *ca, unsigned char slot)
{
    if (slot >= ca->slot_count)
    {
        ci_warning("get slot%d state failed - invalid slot number\n", slot);
        return DVB_CA_SLOTSTATE_NONE;
    }        
            
    return ca->slot_info[slot].slot_state;
}



/**
 * Check CAM status.
 */
static 
int ci_check_camstatus(struct dvb_ca_private *ca, int slot)
{
	int slot_status=0;
	int cam_present_now;
	int cam_changed;

	/* IRQ mode */
	if (ca->flags & DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE) {
		return (atomic_read(&ca->slot_info[slot].camchange_count) != 0);
	}

	/* poll mode */
	// ghyu  need to do?!
    //slot_status = ca->pub->poll_slot_status(ca->pub, slot, ca->open);

	cam_present_now = (slot_status & DVB_CA_EN50221_POLL_CAM_PRESENT) ? 1 : 0;
	cam_changed = (slot_status & DVB_CA_EN50221_POLL_CAM_CHANGED) ? 1 : 0;
	
	if (!cam_changed) 
	{
		int cam_present_old = (ci_get_slot_state(ca,slot)!= DVB_CA_SLOTSTATE_NONE);
		cam_changed = (cam_present_now != cam_present_old);
	}

	if (cam_changed) 
	{
		if (!cam_present_now) 		
			ca->slot_info[slot].camchange_type = DVB_CA_EN50221_CAMCHANGE_REMOVED;
		else
			ca->slot_info[slot].camchange_type = DVB_CA_EN50221_CAMCHANGE_INSERTED;
		
		atomic_set(&ca->slot_info[slot].camchange_count, 1);
	} 
	else 
	{
		if ((ci_get_slot_state(ca, slot)==DVB_CA_SLOTSTATE_WAITREADY) && 
		    (slot_status & DVB_CA_EN50221_POLL_CAM_READY)) 
        {						
			ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_VALIDATE);
		}
	}

	return cam_changed;
}





/**
 * Wait for flags to become set on the STATUS register on a CAM interface,
 * checking for errors and timeout.
 *
 * @param ca CA instance.
 * @param slot Slot on interface.
 * @param waitfor Flags to wait for.
 * @param timeout_ms Timeout in milliseconds.
 *
 * @return 0 on success, nonzero on error.
 */
static int ci_wait_if_status(struct dvb_ca_private *ca, int slot,
					 u8 waitfor, int timeout_hz)
{
	unsigned long timeout;
	unsigned long start;

    ci_function_trace();

	/* loop until timeout elapsed */
	start = jiffies;
	timeout = jiffies + timeout_hz;
	while (1) 
	{
		/* read the status and check for error */
		int res = rtdpc_readIO( CTRLIF_STATUS, slot);
		if (res < 0)
			return -EIO;

		/* if we got the flags, it was successful! */
		if (res & waitfor) {
			ci_dbg("%s succeeded timeout:%lu\n", __FUNCTION__, jiffies - start);
			return 0;
		}

		/* check for timeout */
		if (time_after(jiffies, timeout)) {
			break;
		}

		/* wait for a bit */
		msleep(1);
	}

	ci_warning("%s failed timeout:%lu\n", __FUNCTION__, jiffies - start);

	/* if we get here, we've timed out */
	return -ETIMEDOUT;
}


/**
 * Initialise the link layer connection to a CAM.
 *
 * @param ca CA instance.
 * @param slot Slot id.
 *
 * @return 0 on success, nonzero on failure.
 */
static int ci_link_init(struct dvb_ca_private *ca, int slot)
{
	int ret;
	int buf_size;
	u8 buf[2];

	ci_info("start link layer initialization for CAM %d\n", slot);

	/* we'll be determining these during this function */
	ca->slot_info[slot].da_irq_supported = 0;

	/* set the host link buffer size temporarily. it will be overwritten with the
	 * real negotiated size later. */
	ca->slot_info[slot].link_buf_size = 2;

	/* read the buffer size from the CAM */
	if ((ret = rtdpc_writeIO_DRFR( CTRLIF_COMMAND, CMDREG_SR, slot)) != R_ERR_SUCCESS)
		return ret;
	if ((ret = ci_wait_if_status(ca, slot, STATUSREG_DA, HZ / 10)) != 0)
		return ret;
	if ((ret = ci_read_data( slot, buf, 2)) != 2)
		return -EIO;
	if ((ret = rtdpc_writeIO( CTRLIF_COMMAND, IRQEN, slot)) != R_ERR_SUCCESS)
		return ret;

	/* store it, and choose the minimum of our buffer and the CAM's buffer size */
	buf_size = (buf[0] << 8) | buf[1];
	if (buf_size > HOST_LINK_BUF_SIZE)
		buf_size = HOST_LINK_BUF_SIZE;
	ca->slot_info[slot].link_buf_size = buf_size;
	buf[0] = buf_size >> 8;
	buf[1] = buf_size & 0xff;
	
	ci_info("link buffer size of CAM %d is %d\n", slot, buf_size);	

	/* write the buffer size to the CAM */
	if ((ret = rtdpc_writeIO_DRFR( CTRLIF_COMMAND, IRQEN | CMDREG_SW, slot)) != R_ERR_SUCCESS)
		return ret;
	if ((ret = ci_wait_if_status( ca, slot, STATUSREG_FR, HZ / 10)) != 0)
		return ret;
	if ((ret = ci_write_data( slot, buf, 2)) != 2)
		return -EIO;
	if ((ret = rtdpc_writeIO_DRFR( CTRLIF_COMMAND, IRQEN, slot)) != R_ERR_SUCCESS)
		return ret;
	
	ci_info("link layer initialization for CAM %d successed\n", slot);
	return 0;
}

/**
 * Read a tuple from attribute memory.
 *
 * @param ca CA instance.
 * @param slot Slot id.
 * @param address Address to read from. Updated.
 * @param tupleType Tuple id byte. Updated.
 * @param tupleLength Tuple length. Updated.
 * @param tuple Dest buffer for tuple (must be 256 bytes). Updated.
 *
 * @return 0 on success, nonzero on error.
 */
static int ci_read_tuple(struct dvb_ca_private *ca, int slot,
				     int *address, int *tupleType, int *tupleLength, u8 * tuple)
{
	int i;
	int _tupleType;
	int _tupleLength;
	int _address = *address;

	/* grab the next tuple length and type */

#if 0
	//TEST keith
	/*
	for(i=0; i<12800; i++){
		if(0x1d != rtdpc_readAttrMem(0x0 , slot) )
			break;
		if(0x08 != rtdpc_readAttrMem(0x8 , slot) )
			break;
		if(0x00 != rtdpc_readAttrMem(0x10, slot) )
			break;
	}
	printk("Times =  %d\n", i);
    
    for (i=0;i<128;i++) 
		printk("ADDR[0] = %02x\n", rtdpc_readAttrMem(0, slot));					
	*/
	printk("CIS\n");	
	for (i=0;i<128;i++) {
		printk("%02x ", (unsigned char) rtdpc_readAttrMem(i, slot));
		if (i%8 == 7) printk("\n");				
	}
	printk("\n");	
	//--TEST keith
#endif		
	
	if ((_tupleType = rtdpc_readAttrMem( _address, slot)) < 0)
		return _tupleType;
		
	if (_tupleType == 0xff) 
	{
	    ci_dbg("END OF CHAIN TUPLE type:0x%x\n", _tupleType);
	    
		*address += 2;
		*tupleType = _tupleType;
		*tupleLength = 0;
		return 0;
	}
	
	if ((_tupleLength = rtdpc_readAttrMem( _address + 2, slot)) < 0)
		return _tupleLength;
	_address += 4;

	ci_dbg("TUPLE type:0x%x length:%i\n", _tupleType, _tupleLength);

	/* read in the whole tuple */
	for (i = 0; i < _tupleLength; i++) 
	{
		tuple[i] = rtdpc_readAttrMem( _address + (i * 2), slot);
		ci_dbg("0x%02x: 0x%02x %c\n",
			i, tuple[i] & 0xff,
			((tuple[i] > 31) && (tuple[i] < 127)) ? tuple[i] : '.');
	}
	_address += (_tupleLength * 2);

	// success
	*tupleType = _tupleType;
	*tupleLength = _tupleLength;
	*address = _address;
	return 0;
}


/**
 * Parse attribute memory of a CAM module, extracting Config register, and checking
 * it is a DVB CAM module.
 *
 * @param ca CA instance.
 * @param slot Slot id.
 *
 * @return 0 on success, <0 on failure.
 */
static int ci_parse_attributes(struct dvb_ca_private *ca, int slot)
{
	int address = 0;
	int tupleLength;
	int tupleType;
	u8 tuple[257];
	char *dvb_str;
	int rasz;
	int status;
	int got_cftableentry = 0;
	int end_chain = 0;
	int i;
	u16 manfid = 0;
	u16 devid = 0;    

	// CISTPL_DEVICE_0A
	if ((status =
	     ci_read_tuple(ca, slot, &address, &tupleType, &tupleLength, tuple)) < 0)
		return status;
		
	if (tupleType != 0x1D)
		return -EINVAL;


	// CISTPL_DEVICE_0C
	if ((status =
	     ci_read_tuple(ca, slot, &address, &tupleType, &tupleLength, tuple)) < 0)
		return status;
	
	if (tupleType != 0x1C)
		return -EINVAL;
	


	// CISTPL_VERS_1
	if ((status =
	     ci_read_tuple(ca, slot, &address, &tupleType, &tupleLength, tuple)) < 0)
		return status;

	if (tupleType != 0x15)
		return -EINVAL;




	// CISTPL_MANFID
	if ((status = ci_read_tuple(ca, slot, &address, &tupleType,
						&tupleLength, tuple)) < 0)
		return status;
	
	if (tupleType != 0x20)
		return -EINVAL;
	
	if (tupleLength != 4)
		return -EINVAL;
	
	manfid = (tuple[1] << 8) | tuple[0];
	devid = (tuple[3] << 8) | tuple[2];


	// CISTPL_CONFIG
	if ((status = ci_read_tuple(ca, slot, &address, &tupleType,
						&tupleLength, tuple)) < 0)
		return status;

	if (tupleType != 0x1A)
		return -EINVAL;
	
	if (tupleLength < 3)
		return -EINVAL;


	/* extract the configbase */
	rasz = tuple[0] & 3;
	if (tupleLength < (3 + rasz + 14))
		return -EINVAL;
	
	ca->slot_info[slot].config_base = 0;
	for (i = 0; i < rasz + 1; i++) {
		ca->slot_info[slot].config_base |= (tuple[2 + i] << (8 * i));
	}

	/* check it contains the correct DVB string */
	dvb_str = findstr(tuple, tupleLength, "DVB_CI_V", 8);
	if (dvb_str == NULL)
		return -EINVAL;

	if (tupleLength < ((dvb_str - (char *) tuple) + 12))
		return -EINVAL;


	/* is it a version we support? */
	if (strncmp(dvb_str + 8, "1.00", 4)) 
	{
		ci_warning("Unsupported DVB CAM module version %c%c%c%c\n"
		      , dvb_str[8], dvb_str[9], dvb_str[10], dvb_str[11]);
		return -EINVAL;
	}
	

	/* process the CFTABLE_ENTRY tuples, and any after those */
	while ((!end_chain) && (address < 0x1000)) 
	{
		if ((status = ci_read_tuple(ca, slot, &address, &tupleType,
						        &tupleLength, tuple)) < 0)
			return status;

		switch (tupleType) 
		{
		case 0x1B:	// CISTPL_CFTABLE_ENTRY
			if (tupleLength < (2 + 11 + 17))
				break;

			/* if we've already parsed one, just use it */
			if (got_cftableentry)
				break;

			/* get the config option */
			ca->slot_info[slot].config_option = tuple[0] & 0x3f;

			/* OK, check it contains the correct strings */
			if ((findstr(tuple, tupleLength, "DVB_HOST", 8) == NULL) ||
			    (findstr(tuple, tupleLength, "DVB_CI_MODULE", 13) == NULL))
				break;

			got_cftableentry = 1;
			break;

		case 0x14:	// CISTPL_NO_LINK
			break;

		case 0xFF:	// CISTPL_END
			end_chain = 1;
			break;

		default:			   
			ci_warning("Skipping unknown tuple type:0x%x length:0x%x\n", tupleType, tupleLength);
			break;
		}
	}

	if ((address > 0x1000) || (!got_cftableentry))
		return -EINVAL;

	ci_info("Valid DVB CAM detected MANID:%x DEVID:%x CONFIGBASE:0x%x CONFIGOPTION:0x%x\n",
		    manfid, devid, ca->slot_info[slot].config_base, ca->slot_info[slot].config_option);

	return 0;
}


/**
 * Set CAM's configoption correctly.
 *
 * @param ca CA instance.
 * @param slot Slot containing the CAM.
 */
static int ci_set_configoption(struct dvb_ca_private *ca, int slot)
{
	int configoption;

	ci_function_trace();

	/* set the config option */
	rtdpc_writeAttrMem( ca->slot_info[slot].config_base,
				     ca->slot_info[slot].config_option, slot);

	/* check it */
	configoption = rtdpc_readAttrMem( ca->slot_info[slot].config_base, slot);
	ci_info("Set configoption 0x%x, read configoption 0x%x\n",
		    ca->slot_info[slot].config_option, configoption & 0x3f);
	
	return 0;

}


/**
 * This function talks to an EN50221 CAM control interface. It reads a buffer of
 * data from the CAM. The data can either be stored in a supplied buffer, or
 * automatically be added to the slot's rx_buffer.
 *
 * @param ca CA instance.
 * @param slot Slot to read from.
 * @param ebuf If non-NULL, the data will be written to this buffer. If NULL,
 * the data will be added into the buffering system as a normal fragment.
 * @param ecount Size of ebuf. Ignored if ebuf is NULL.
 *
 * @return Number of bytes read, or < 0 on error
 */
static int ci_read_data(int slot, u8 * ebuf, int ecount)
{
	int bytes_read;
	int status;
	u8 buf[HOST_LINK_BUF_SIZE];
	int i;

    ci_function_trace();

	/* check if we have space for a link buf in the rx_buffer */
	if (ebuf == NULL) 
	{
		int buf_free;

		down_read(&ca->slot_info[slot].sem);
		if (ca->slot_info[slot].rx_buffer.data == NULL) {
			up_read(&ca->slot_info[slot].sem);
			status = -EIO;
			goto exit;
		}
		buf_free = dvb_ringbuffer_free(&ca->slot_info[slot].rx_buffer);
		up_read(&ca->slot_info[slot].sem);

		if (buf_free < (ca->slot_info[slot].link_buf_size + DVB_RINGBUFFER_PKTHDRSIZE)) {
			status = -EAGAIN;
			goto exit;
		}
	}
	
#if 0
	/* check if there is data available */
	if ((status = rtdpc_readStatus() < 0))
		goto exit;

	if (!(status & STATUSREG_DA)) {
		/* no data */
		status = 0;
		goto exit;
	}
#endif
	/* read the amount of data */
	if ((status = rtdpc_readIO(CTRLIF_SIZE_HIGH, slot)) < 0)
		goto exit;

	bytes_read = status << 8;
	if ((status = rtdpc_readIO(CTRLIF_SIZE_LOW, slot)) < 0)
		goto exit;
	bytes_read |= status;

	/* check it will fit */
	if (ebuf == NULL) 
	{
		if (bytes_read > ca->slot_info[slot].link_buf_size) 
		{
			ci_rx_warning("CAM %d tried to send a buffer larger than the link buffer size (%i > %i)!\n",
			               slot, bytes_read, ca->slot_info[slot].link_buf_size);
			               
            ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_LINKINIT);			
			status = -EIO;
			goto exit;
		}
		
		if (bytes_read < 2) 
		{
			ci_rx_warning("CAM %d sent a buffer that was less than 2 bytes!\n", slot);			;
			ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_LINKINIT);
			status = -EIO;
			goto exit;
		}
	}
	else 
	{
		if (bytes_read > ecount) 
		{
			ci_rx_warning("CAM %d tried to send a buffer larger than the ecount size!\n", slot);
			status = -EIO;
			goto exit;
		}
	}

	/* fill the buffer */
	for (i = 0; i < bytes_read; i++) 
	{
		/* read byte and check */
		if ((status = rtdpc_readIO( CTRLIF_DATA, slot)) < 0)
			goto exit;

		/* OK, store it in the buffer */
		buf[i] = status;
	}

	/* check for read error (RE should now be 0) */
	if ((status = rtdpc_readIO( CTRLIF_STATUS, slot)) < 0)
		goto exit;
		
	if (status & STATUSREG_RE) 
	{
		ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_LINKINIT);
		status = -EIO;
		goto exit;
	}

	/* OK, add it to the receive buffer, or copy into external buffer if supplied */
	if (ebuf == NULL) 
	{
		down_read(&ca->slot_info[slot].sem);
		
		if (ca->slot_info[slot].rx_buffer.data == NULL) 
		{
			up_read(&ca->slot_info[slot].sem);
			status = -EIO;
			goto exit;
		}
		dvb_ringbuffer_pkt_write(&ca->slot_info[slot].rx_buffer, buf, bytes_read);
		up_read(&ca->slot_info[slot].sem);
	}
	else 
	{
		memcpy(ebuf, buf, bytes_read);
	}

	ci_rx_info("Received CA packet for slot %i connection id 0x%x last_frag:%i size:0x%x\n", slot,
		       buf[0], (buf[1] & 0x80) == 0, bytes_read);

	/* wake up readers when a last_fragment is received */
	if ((buf[1] & 0x80) == 0x00) 	
		wake_up_interruptible(&ca->wait_queue);
	
	status = bytes_read;
exit:

	return status;
}


/**
 * This function talks to an EN50221 CAM control interface. It writes a buffer of data
 * to a CAM.
 *
 * @param ca CA instance.
 * @param slot Slot to write to.
 * @param ebuf The data in this buffer is treated as a complete link-level packet to
 * be written.
 * @param count Size of ebuf.
 *
 * @return Number of bytes written, or < 0 on error.
 */
static int ci_write_data( int slot, u8 * buf, int bytes_write)
{
	int status;
	int i;
    
    ci_function_trace();

	// sanity check
	if (bytes_write > ca->slot_info[slot].link_buf_size)
		return -EINVAL;

	/* check if interface is actually waiting for us to read from it, or if a read is in progress */
	if ((status = rtdpc_readIO( CTRLIF_STATUS, slot)) < 0)
		goto exitnowrite;

	if (status & (STATUSREG_DA | STATUSREG_RE)) {
		status = -EAGAIN;
		goto exitnowrite;
	}

	/* OK, set HC bit */
	if ((status = rtdpc_writeIO_DRFR( CTRLIF_COMMAND,
						 IRQEN | CMDREG_HC, slot)) != R_ERR_SUCCESS)
		goto exit;

	/* check if interface is still free */
	if ((status = rtdpc_readIO( CTRLIF_STATUS, slot)) < 0)
		goto exit;

	if (!(status & STATUSREG_FR)) {
		/* it wasn't free => try again later */
		status = -EAGAIN;
		goto exit;
	}

	/* send the amount of data */
	if ((status = rtdpc_writeIO( CTRLIF_SIZE_HIGH, bytes_write >> 8, slot)) != R_ERR_SUCCESS)
		goto exit;
		
	if ((status = rtdpc_writeIO( CTRLIF_SIZE_LOW,
						 bytes_write & 0xff, slot)) != R_ERR_SUCCESS)
		goto exit;

	/* send the buffer */
	for (i = 0; i < bytes_write; i++) {
		if ((status = rtdpc_writeIO( CTRLIF_DATA, buf[i], slot)) != R_ERR_SUCCESS)
			goto exit;
	}

	/* check for write error (WE should now be 0) */
	if ((status = rtdpc_readIO( CTRLIF_STATUS, slot)) < 0)
		goto exit;

	if (status & STATUSREG_WE) 
	{		
		ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_LINKINIT);
		status = -EIO;
		goto exit;
	}

	status = bytes_write;

	ci_tx_info("Wrote CA packet for slot %i, connection id 0x%x last_frag:%i size:0x%x\n", slot,
		        buf[0], (buf[1] & 0x80) == 0, bytes_write);

exit:
	rtdpc_writeIO_DRFR( CTRLIF_COMMAND, IRQEN, slot);

exitnowrite:
	return status;
}




/***********************************************************************************
 * Block : EN50221 high level APIs functions 
 * Desc  : 
 ***********************************************************************************/



/*------------------------------------------------------------------
 * Func : ci_slot_shutdown
 *
 * Desc : shut down a slot
 *
 * Parm : ca        : CA instance.
 *        slot      : Slot to shut down. 
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/ 
static 
int ci_slot_shutdown(
    struct dvb_ca_private*  ca, 
    int                     slot
    )
{
	ci_function_trace();

	down_write(&ca->slot_info[slot].sem);	
		
	ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_NONE);
	
	if (ca->slot_info[slot].rx_buffer.data)
		vfree(ca->slot_info[slot].rx_buffer.data);
		
	ca->slot_info[slot].rx_buffer.data = NULL;
	up_write(&ca->slot_info[slot].sem);

	/* need to wake up all processes to check if they're now
	   trying to write to a defunct CAM */
	wake_up_interruptible(&ca->wait_queue);

	ci_info("slot %i shutdown\n", slot);
	
	return 0;
}


 
/*------------------------------------------------------------------
 * Func : ci_camchange_irq
 *
 * Desc : event handler for CAM change event
 *
 * Parm : slot        : which slot does the event occur
 *        change_type : event type
 *
 *          DVB_CA_EN50221_CAMCHANGE_INSERTED : CAM Inserted 
 *          DVB_CA_EN50221_CAMCHANGE_REMOVED : CAM Removed
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/   
void ci_camchange_irq(
    int                     slot, 
    int                     change_type
    )
{
	ci_info("CAMCHANGE IRQ slot %i, change_type %i\n", slot, change_type);

	switch (change_type) 
	{
	case DVB_CA_EN50221_CAMCHANGE_REMOVED:
		#ifdef SEND_MSG_TO_AP		
		if (slot==0 )
			ci_sendMessageToAP(MSG_DRV_CI_SLOT0_REMOVE);
		else if(slot ==1)
			ci_sendMessageToAP(MSG_DRV_CI_SLOT1_REMOVE);		
		#endif		
		break;
		
	case DVB_CA_EN50221_CAMCHANGE_INSERTED:
		#ifdef SEND_MSG_TO_AP
		if (slot==0 )
			ci_sendMessageToAP(MSG_DRV_CI_SLOT0_INSERT);
		else if(slot==1)
			ci_sendMessageToAP(MSG_DRV_CI_SLOT1_INSERT);
		#endif
		break;

	default:
		return;
	}

	ca->slot_info[slot].camchange_type = change_type;
	atomic_inc(&ca->slot_info[slot].camchange_count);
	ci_thread_wakeup(ca);
}


 

/*------------------------------------------------------------------
 * Func : ci_camready_irq
 *
 * Desc : event handler for CAM READY event
 *
 * Parm : slot   : which slot 
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/   
void ci_camready_irq(int slot)
{
	ci_info("CAMREADY IRQ slot %i\n", slot);

	if (ci_get_slot_state(ca, slot)==DVB_CA_SLOTSTATE_WAITREADY) 
	{	
		ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_VALIDATE);
		ci_thread_wakeup(ca);
	}
}



/*------------------------------------------------------------------
 * Func : ci_frda_irq
 *
 * Desc : event handler of FR/DA IRQ 
 *
 * Parm : slot   : which slot 
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/     
void ci_frda_irq(int slot)
{
	int flags;

	ci_info("got FR/DA IRQ from slot %i\n", slot);
		
	switch (ci_get_slot_state(ca, slot)) 
	{
	case DVB_CA_SLOTSTATE_LINKINIT:
		
		if (ca->slot_info[slot].da_irq_supported==1)
			return;
		
		flags = rtdpc_readIO( CTRLIF_STATUS, slot);
		
		if (flags & STATUSREG_DA) 
		{
			ci_info("CAM %d supports DA IRQ\n");
			ca->slot_info[slot].da_irq_supported = 1;
		}
		
		break;

	case DVB_CA_SLOTSTATE_RUNNING:
	
		if (ca->open)
			ci_read_data( slot, NULL, 0);

		break;
	}
}



/***********************************************************************************
 * Block : EN50221 thread functions 
 * Desc  : this function block is designed for control flow of EN50221 CA module
 *         Driver. 
 ***********************************************************************************/



/*------------------------------------------------------------------
 * Func : ci_thread_wakeup
 *
 * Desc : wake up ci thread
 *
 * Parm : ca   : CA instance 
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/    
static 
void ci_thread_wakeup(struct dvb_ca_private* ca)
{

    ci_function_trace();
	ca->wakeup = 1;
	mb();
	wake_up_interruptible(&ca->thread_queue);
}




/*------------------------------------------------------------------
 * Func : ci_thread_should_wakeup
 *
 * Desc : determine if an early wakeup is necessary
 *
 * Parm : ca   : CA instance 
 *         
 * Retn : 1 : ca module should wakeup early
 *        0 : ca module don't need to wakeup early
 *------------------------------------------------------------------*/   
static 
int ci_thread_should_wakeup(struct dvb_ca_private *ca)
{
	if (ca->wakeup) 
	{
		ca->wakeup = 0;
		return 1;
	}
	
	if (ca->exit)
		return 1;

	return 0;
}




/*------------------------------------------------------------------
 * Func : ci_thread_update_delay
 *
 * Desc : Update the delay used by the thread 
 *
 * Parm : ca   : private data of CA module driver 
 *         
 * Retn : 0
 *------------------------------------------------------------------*/   
static 
void ci_thread_update_delay(struct dvb_ca_private *ca)
{
	int delay;
	int curdelay = 100000000;
	int slot;

	for (slot = 0; slot < ca->slot_count; slot++) 
	{
		switch (ci_get_slot_state(ca, slot)) 
		{
		default:
		case DVB_CA_SLOTSTATE_NONE:
		case DVB_CA_SLOTSTATE_INVALID:
			delay = HZ * 60;
			if (!(ca->flags & DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE)) {
				delay = HZ / 10;
			}
			break;

		case DVB_CA_SLOTSTATE_UNINITIALISED:
		case DVB_CA_SLOTSTATE_WAITREADY:
		case DVB_CA_SLOTSTATE_VALIDATE:
		case DVB_CA_SLOTSTATE_WAITFR:
		case DVB_CA_SLOTSTATE_LINKINIT:
			delay = HZ / 10;
			break;

		case DVB_CA_SLOTSTATE_RUNNING:
			delay = HZ * 60;
			
			if (!(ca->flags & DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE)) 
			{
				delay = HZ / 10;
			}
			
			if (ca->open) 
			{
				if ((!ca->slot_info[slot].da_irq_supported) ||
				    (!(ca->flags & DVB_CA_EN50221_FLAG_IRQ_DA))) {
					delay = HZ / 100;
				}
			}
			break;
		}

		if (delay < curdelay)
			curdelay = delay;
	}
	ca->delay = curdelay;
}





/*------------------------------------------------------------------
 * Func : ci_thread
 *
 * Desc : main thread for CI module driver. this thread is used for
 *        monitor CAM status and performs data transfer.
 *
 * Parm : data   : private data of CA module driver 
 *         
 * Retn : 0
 *------------------------------------------------------------------*/   
static 
int ci_thread(void *data)
{
	struct dvb_ca_private *ca = (struct dvb_ca_private *) data;
	char name[15];
	int slot;
	int flags;
	int status;
	int pktcount;
	void *rxbuf;

    ci_function_trace();    
    
    snprintf(name, sizeof(name), "kdvb-ca");

    lock_kernel ();
    daemonize(name);
    sigfillset(&current->blocked);
    unlock_kernel ();
	
	ci_thread_update_delay(ca);
	
	while (!ca->exit) 
	{
		if (!ca->wakeup) 
		{
			flags = wait_event_interruptible_timeout(ca->thread_queue,
								 ci_thread_should_wakeup(ca),
								 ca->delay);
								 
			if ((flags == -ERESTARTSYS) || ca->exit) 			
				break;			
		}
		
		ca->wakeup = 0;

		/* go through all the slots processing them */
		for (slot = 0; slot < ca->slot_count; slot++) 
		{
			// check the cam status + deal with CAMCHANGEs
			while (ci_check_camstatus(ca, slot)) 
			{			    			    				
				if (ci_get_slot_state(ca, slot)!= DVB_CA_SLOTSTATE_NONE)
					ci_slot_shutdown(ca, slot);

				/* if a CAM is NOW present, initialise it */
				if (ca->slot_info[slot].camchange_type == DVB_CA_EN50221_CAMCHANGE_INSERTED) 				
				    ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_UNINITIALISED);									

				/* we've handled one CAMCHANGE */
				ci_thread_update_delay(ca);
				atomic_dec(&ca->slot_info[slot].camchange_count);
			}
			
			ci_dbg("slot %d slot state %d (%s)\n", slot, ci_get_slot_state(ca, slot), cam_state_str[ci_get_slot_state(ca, slot)]);

			// CAM state machine
			switch (ci_get_slot_state(ca, slot)) 
			{
			case DVB_CA_SLOTSTATE_NONE:
			case DVB_CA_SLOTSTATE_INVALID:
				// no action needed
				break;

			case DVB_CA_SLOTSTATE_UNINITIALISED:
				
				msleep(500);
				//Chuck add a hw reset because some CA module need it not sw reset on 20051031
				rtdpc_hw_reset(slot);

				// ca->pub->slot_reset(ca->pub, slot);
				// added by ghyu 2004/5/26
				// We just change the state to DVB_CA_SLOTSTATE_VALIDATE
				ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_VALIDATE);				
				ca->slot_info[slot].timeout = jiffies + (INIT_TIMEOUT_SECS * HZ);
				break;

			case DVB_CA_SLOTSTATE_WAITREADY:
				
				if (time_after(jiffies, ca->slot_info[slot].timeout)) 
				{
					ci_warning(" dvb_ca adaptor  PC card did not respond :(\n");					
					ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_INVALID);
					ci_thread_update_delay(ca);					
					ci_sendMessageToAP(MSG_DRV_CI_PCCARD_NOT_RESPOND);					
					break;
				}
				// no other action needed; will automatically change state when ready
				break;

			case DVB_CA_SLOTSTATE_VALIDATE:
			
				if (ci_parse_attributes(ca, slot) != 0) 
				{
					ci_warning("Invalid PC card inserted :(\n");					
					ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_INVALID);
					ci_thread_update_delay(ca);					
					ci_sendMessageToAP(MSG_DRV_CI_INVALID_PCCARD_INSERTED);					
					break;
				}
				

				if (ci_set_configoption(ca, slot) != 0) 
				{
					ci_warning("Unable to initialise CAM :(\n");					
					ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_INVALID);					
					ci_thread_update_delay(ca);
					ci_sendMessageToAP(MSG_DRV_CI_UNABLE_TO_INITIALISE_CAM);
					break;				
				}
				
				if (rtdpc_writeIO(CTRLIF_COMMAND, CMDREG_RS, slot) != R_ERR_SUCCESS) {
					printk("CI_DRV: Unable to reset CAM IF\n");
					ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_INVALID);					
					ci_thread_update_delay(ca);
					ci_sendMessageToAP(MSG_DRV_CI_UNABLE_TO_RESET_CAM_IF);
					break;
				}

				ci_sendMessageToAP(MSG_DRV_CI_CAM_VALIDATED_SUCCESS);

				ci_info("DVB CAM validated successfully\n");

				ca->slot_info[slot].timeout = jiffies + (INIT_TIMEOUT_SECS * HZ);				
				ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_WAITFR);
				ca->wakeup = 0;
				break;

			case DVB_CA_SLOTSTATE_WAITFR:
			
				if (time_after(jiffies, ca->slot_info[slot].timeout)) 
				{
					ci_warning("DVB CAM did not respond :(\n");
					ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_INVALID);					
					ci_thread_update_delay(ca);
					ci_sendMessageToAP(MSG_DRV_CI_CAM_NOT_RESPOND);
					break;
				}

				flags = rtdpc_readIO(CTRLIF_STATUS, slot);
				if (flags & STATUSREG_FR) 
				{
				    ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_LINKINIT);					
					ca->wakeup = 1;
				}
				break;

			case DVB_CA_SLOTSTATE_LINKINIT:
			
				if (ci_link_init(ca, slot) != 0) 
				{
					ci_warning("DVB CAM link initialisation failed :(\n");
                    ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_INVALID);
					ci_thread_update_delay(ca);
                    ci_sendMessageToAP(MSG_DRV_CI_CAM_LINK_INIT_FAIL);
					break;
				}

				rxbuf = vmalloc(RX_BUFFER_SIZE);
				if (rxbuf == NULL) 
				{
					ci_warning("Unable to allocate CAM rx buffer :(\n");					
					ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_INVALID);
					ci_thread_update_delay(ca);
					ci_sendMessageToAP(MSG_DRV_CI_UNABLE_TO_ALLOCATE_CAM_RX_BUFFER);
					break;
				}
				down_write(&ca->slot_info[slot].sem);
				dvb_ringbuffer_init(&ca->slot_info[slot].rx_buffer, rxbuf, RX_BUFFER_SIZE);
				up_write(&ca->slot_info[slot].sem);
				// ghyu need to do something?!
				// ca->pub->slot_ts_enable(ca->pub, slot);
				ci_update_slot_state(ca, slot, DVB_CA_SLOTSTATE_RUNNING);				
				ci_thread_update_delay(ca);
				ci_sendMessageToAP(MSG_DRV_CI_CAM_DETECTED_AND_INIT_SUCCESS);		
				ci_info("DVB CAM detected and initialised successfully\n");
				break;

			case DVB_CA_SLOTSTATE_RUNNING:
			
				if (!ca->open)
					continue;

				// no need to poll if the CAM supports IRQs
				if (ca->slot_info[slot].da_irq_supported)
					break;

				// poll mode
				pktcount = 0;
				while ((status = ci_read_data(slot, NULL, 0)) > 0) 
				{
					if (!ca->open)
						break;

					/* if a CAMCHANGE occurred at some point, do not do any more processing of this slot */
					if (ci_check_camstatus(ca, slot)) {
						// we dont want to sleep on the next iteration so we can handle the cam change
						ca->wakeup = 1;
						break;
					}

					/* check if we've hit our limit this time */
					if (++pktcount >= MAX_RX_PACKETS_PER_ITERATION) {
						// dont sleep; there is likely to be more data to read
						ca->wakeup = 1;
						break;
					}
				}
				break;
			}
		}
	}

	/* completed */
	ca->thread_pid = 0;
	mb();
	wake_up_interruptible(&ca->thread_queue);
	return 0;
}





/*********************************************************************************
 * Group : EN50221 IO interface functions 
 * Desc  : This Group of funcion is used to provide i/o interface to user interface
 **********************************************************************************/


/*------------------------------------------------------------------
 * Func : ci_io_do_ioctl
 *
 * Desc : Real ioctl implementation. Note: CA_SEND_MSG/CA_GET_MSG ioctls 
 *        have userspace buffers passed to them. 
 *
 * Parm : inode   : Inode concerned
 *        file    : File concerned
 *        cmd     : IOCTL command
 *        arg     : Associated argument.
 *         
 * Retn : 0 : success, < 0 : failed 
 *------------------------------------------------------------------*/    
static 
int ci_io_do_ioctl(
    struct inode*           inode, 
    struct file*            file,
    unsigned int            cmd, 
    void*                   parg
    )
{
//	struct dvb_device *dvbdev = (struct dvb_device *) file->private_data;
//	struct dvb_ca_private *ca = ca; // (struct dvb_ca_private *) dvbdev->priv;

	int err = 0;
	int slot;

	ci_function_trace();

	switch (cmd) 
	{
	case CA_RESET:
		
        ci_dbg("ioctl cmd is CA_RESET\n");
        
		for (slot = 0; slot < ca->slot_count; slot++) 
		{		     
			if (ci_get_slot_state(ca, slot)!=DVB_CA_SLOTSTATE_NONE) 
			{
				ci_slot_shutdown(ca, slot);
				if (ca->flags & DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE)
					ci_camchange_irq(slot, DVB_CA_EN50221_CAMCHANGE_INSERTED);
			}
		}
		
		ca->next_read_slot = 0;
		ci_thread_wakeup(ca);
		break;

	case CA_GET_CAP: {
	
		struct ca_caps *caps = (struct ca_caps *) parg;
        ci_dbg("ioctl cmd is CA_GET_CAP\n");
        
		caps->slot_num = ca->slot_count;
		caps->slot_type = CA_CI_LINK;
		caps->descr_num = 0;
		caps->descr_type = 0;
		break;	
    }
	case CA_GET_SLOT_INFO: {
	
		struct ca_slot_info *info = (struct ca_slot_info *) parg;
		ci_dbg("ioctl cmd is CA_GET_SLOT_INFO\n");

		if ((info->num > ca->slot_count) || (info->num < 0))
			return -EINVAL;

		info->type = CA_CI_LINK;
		info->flags = 0;
			
		if ((ci_get_slot_state(ca, info->num)!= DVB_CA_SLOTSTATE_NONE) &&
			(ci_get_slot_state(ca, info->num)!= DVB_CA_SLOTSTATE_INVALID)) 
        {
			info->flags = CA_CI_MODULE_PRESENT;
		}
		
		if (ci_get_slot_state(ca, info->num) == DVB_CA_SLOTSTATE_RUNNING) {
			info->flags |= CA_CI_MODULE_READY;
		}
		break;	
    }
	default:
		ci_warning("ioctl failed, cmd is unknown\n");
		err = -EINVAL;
		break;
	}

	return err;
}




/*------------------------------------------------------------------
 * Func : ci_io_ioctl
 *
 * Desc : Wrapper for ioctl implementation.
 *
 * Parm : inode   : Inode concerned
 *        file    : File concerned
 *        cmd     : IOCTL command
 *        arg     : Associated argument.
 *         
 * Retn : 0 : success, < 0 : failed 
 *------------------------------------------------------------------*/     
static int ci_io_ioctl(
    struct inode*           inode, 
    struct file*            file,
    unsigned int            cmd, 
    unsigned long           arg
    )
{
	return dvb_usercopy(inode, file, cmd, arg, ci_io_do_ioctl);
}




/* if the miracle happens and "generic_usercopy()" is included into
   the kernel, then this can vanish. please don't make the mistake and
   define this as video_usercopy(). this will introduce a dependecy
   to the v4l "videodev.o" module, which is unnecessary for some
   cards (ie. the budget dvb-cards don't need the v4l module...) */
int dvb_usercopy(struct inode *inode, struct file *file,
	             unsigned int cmd, unsigned long arg,
		     int (*func)(struct inode *inode, struct file *file,
		     unsigned int cmd, void *arg))
{
    char  sbuf[128];
    void* mbuf = NULL;
    void* parg = NULL;
    int   err  = -EINVAL;

    /*  Copy arguments into temp kernel buffer  */
    switch (_IOC_DIR(cmd)) 
    {
    case _IOC_NONE:
    	/*
    	 * For this command, the pointer is actually an integer
    	 * argument.
    	 */
    	parg = (void *) arg;
    	break;
    	
    case _IOC_READ: /* some v4l ioctls are marked wrong ... */
    case _IOC_WRITE:
    case (_IOC_WRITE | _IOC_READ):
    
        if (_IOC_SIZE(cmd) <= sizeof(sbuf)) 
        {
            parg = sbuf;
        } 
        else 
        {
            /* too big to allocate from stack */
            mbuf = kmalloc(_IOC_SIZE(cmd),GFP_KERNEL);
            if (NULL == mbuf)
                return -ENOMEM;
            parg = mbuf;
        }

        err = -EFAULT;
        
        if (copy_from_user(parg, (void __user *)arg, _IOC_SIZE(cmd)))
            goto out;
            
        break;
    }

    /* call driver */
    if ((err = func(inode, file, cmd, parg)) == -ENOIOCTLCMD)
        err = -EINVAL;

    if (err < 0)
        goto out;

    /*  Copy results into user buffer  */
    switch (_IOC_DIR(cmd))
    {
    case _IOC_READ:
    case (_IOC_WRITE | _IOC_READ):
        if (copy_to_user((void __user *)arg, parg, _IOC_SIZE(cmd)))
            err = -EFAULT;
        break;
    }

out:
    if (mbuf)
        kfree(mbuf);

    return err;
}


/**
 * Implementation of write() syscall.
 *
 * @param file File structure.
 * @param buf Source buffer.
 * @param count Size of source buffer.
 * @param ppos Position in file (ignored).
 *
 * @return Number of bytes read, or <0 on error.
 */
static 
ssize_t ci_io_write(
    struct file*            file,
    const char*             buf, 
    size_t                  count, 
    loff_t*                 ppos
    )
{
	u8 slot, connection_id;
	int status;
	char fragbuf[HOST_LINK_BUF_SIZE];
	int fragpos = 0;
	int fraglen;
	unsigned long timeout;
	int written;

	ci_function_trace();

	/* Incoming packet has a 2 byte header. hdr[0] = slot_id, hdr[1] = connection_id */
	if (count < 2)
		return -EINVAL;

	/* extract slot & connection id */
	if (copy_from_user(&slot, buf, 1))
		return -EFAULT;
		
	if (copy_from_user(&connection_id, buf + 1, 1))
		return -EFAULT;
		
	buf += 2;
	count -= 2;
	
	if (ci_get_slot_state(ca, slot)!= DVB_CA_SLOTSTATE_RUNNING)
		return -EINVAL;

	while (fragpos < count) 
	{
		fraglen = ca->slot_info[slot].link_buf_size - 2;
		
		if ((count - fragpos) < fraglen)
			fraglen = count - fragpos;

		fragbuf[0] = connection_id;
		fragbuf[1] = ((fragpos + fraglen) < count) ? 0x80 : 0x00;
		
		if ((status = copy_from_user(fragbuf + 2, buf + fragpos, fraglen)) != 0)
			goto exit;

		timeout = jiffies + HZ / 2;
		written = 0;
		
		while (!time_after(jiffies, timeout)) 
		{			
			if (ci_get_slot_state(ca, slot)!= DVB_CA_SLOTSTATE_RUNNING) 
			{
				status = -EIO;
				goto exit;
			}

			status = ci_write_data( slot, fragbuf, fraglen + 2);
			if (status == (fraglen + 2)) 
			{
				written = 1;
				break;
			}
			
			if (status != -EAGAIN)
				goto exit;

			msleep(1);
		}
		
		if (!written) 
		{
			status = -EIO;
			goto exit;
		}

		fragpos += fraglen;
	}
	status = count + 2;

exit:
	return status;
}




/**
 * Condition for waking up in ci_io_read_condition
 */
static 
int ci_io_read_condition(
    struct dvb_ca_private*  ca, 
    int*                    result,
    int*                    _slot
    )
{
	int slot;
	int slot_count = 0;
	int idx;
	int fraglen;
	int connection_id = -1;
	int found = 0;
	u8 hdr[2];

	slot = ca->next_read_slot;
	
	while ((slot_count < ca->slot_count) && (!found)) 
	{
		if (ci_get_slot_state(ca, slot)!= DVB_CA_SLOTSTATE_RUNNING)
			goto nextslot;

		down_read(&ca->slot_info[slot].sem);

		if (ca->slot_info[slot].rx_buffer.data == NULL) 
		{
			up_read(&ca->slot_info[slot].sem);
			return 0;
		}

		idx = dvb_ringbuffer_pkt_next(&ca->slot_info[slot].rx_buffer, -1, &fraglen);
		
		while (idx != -1) 
		{
			dvb_ringbuffer_pkt_read(&ca->slot_info[slot].rx_buffer, idx, 0, hdr, 2, 0);
			
			if (connection_id == -1)
				connection_id = hdr[0];
				
			if ((hdr[0] == connection_id) && ((hdr[1] & 0x80) == 0)) 
			{
				*_slot = slot;
				found = 1;
				break;
			}

			idx = dvb_ringbuffer_pkt_next(&ca->slot_info[slot].rx_buffer, idx, &fraglen);
		}

		if (!found)
			up_read(&ca->slot_info[slot].sem);

nextslot:
		slot = (slot + 1) % ca->slot_count;
		slot_count++;
	}

	ca->next_read_slot = slot;
	return found;
}




/**
 * Implementation of read() syscall.
 *
 * @param file File structure.
 * @param buf Destination buffer.
 * @param count Size of destination buffer.
 * @param ppos Position in file (ignored).
 *
 * @return Number of bytes read, or <0 on error.
 */
static 
ssize_t ci_io_read(
    struct file*            file, 
    char*                   buf,
    size_t                  count, 
    loff_t*                 ppos
    )
{
	int status;
	int result = 0;
	u8 hdr[2];
	int slot;
	int connection_id = -1;
	size_t idx, idx2;
	int last_fragment = 0;
	size_t fraglen;
	int pktlen;
	int dispose = 0;

	ci_function_trace();

	/* Outgoing packet has a 2 byte header. hdr[0] = slot_id, hdr[1] = connection_id */
	if (count < 2)
		return -EINVAL;

	/* wait for some data */
	if ((status = ci_io_read_condition(ca, &result, &slot)) == 0) 
	{
		/* if we're in nonblocking mode, exit immediately */
		if (file->f_flags & O_NONBLOCK)
			return -EWOULDBLOCK;

		/* wait for some data */
		status = wait_event_interruptible(ca->wait_queue,
						  ci_io_read_condition
						  (ca, &result, &slot));
	}
	
	if ((status < 0) || (result < 0)) {
		if (result)
			return result;
		return status;
	}

	idx = dvb_ringbuffer_pkt_next(&ca->slot_info[slot].rx_buffer, -1, &fraglen);
	pktlen = 2;
	
	do 
	{
		if (idx == -1) 
		{
			ci_rx_warning("read packet ended before last_fragment encountered\n");
			status = -EIO;
			goto exit;
		}

		dvb_ringbuffer_pkt_read(&ca->slot_info[slot].rx_buffer, idx, 0, hdr, 2, 0);
		
		if (connection_id == -1)
			connection_id = hdr[0];
			
		if (hdr[0] == connection_id) 
		{
			if (pktlen < count) 
			{
				if ((pktlen + fraglen - 2) > count) 				
					fraglen = count - pktlen;				
				else 				
					fraglen -= 2;				

				if ((status = dvb_ringbuffer_pkt_read(&ca->slot_info[slot].rx_buffer, idx, 2,
								      buf + pktlen, fraglen, 1)) < 0) {
					goto exit;
				}
				pktlen += fraglen;
			}

			if ((hdr[1] & 0x80) == 0)
				last_fragment = 1;
			dispose = 1;
		}

		idx2 = dvb_ringbuffer_pkt_next(&ca->slot_info[slot].rx_buffer, idx, &fraglen);
		
		if (dispose)
			dvb_ringbuffer_pkt_dispose(&ca->slot_info[slot].rx_buffer, idx);
			
		idx = idx2;
		dispose = 0;
		
	} while (!last_fragment);

	hdr[0] = slot;
	hdr[1] = connection_id;
	if ((status = copy_to_user(buf, hdr, 2)) != 0)
		goto exit;
		
	status = pktlen;

exit:
	up_read(&ca->slot_info[slot].sem);
	return status;
}


int dvb_generic_open(struct inode *inode, struct file *file)
{
    struct dvb_device *dvbdev = file->private_data;

    if (!dvbdev)
        return -ENODEV;

	if (!dvbdev->users)
        return -EBUSY;

	if ((file->f_flags & O_ACCMODE) == O_RDONLY) 
	{
        if (!dvbdev->readers)
            return -EBUSY;
		dvbdev->readers--;
	} 
	else 
	{
        if (!dvbdev->writers)
		    return -EBUSY;		    
		dvbdev->writers--;
	}

	dvbdev->users--;
	return 0;
}


int dvb_generic_release(struct inode *inode, struct file *file)
{
    struct dvb_device *dvbdev = file->private_data;

	if (!dvbdev)
        return -ENODEV;

	if ((file->f_flags & O_ACCMODE) == O_RDONLY) {
		dvbdev->readers++;
	} else {
		dvbdev->writers++;
	}

	dvbdev->users++;
	return 0;
}


/**
 * Implementation of file open syscall.
 *
 * @param inode Inode concerned.
 * @param file File concerned.
 *
 * @return 0 on success, <0 on failure.
 */
static 
int ci_io_open(struct inode *inode, struct file *file)
{
	int i;

	ci_function_trace();

	for (i = 0; i < ca->slot_count; i++) 
	{
	    
		if (ci_get_slot_state(ca, i)== DVB_CA_SLOTSTATE_RUNNING) 
		{
			ci_info("solt %d running!!!\n", i);
			down_write(&ca->slot_info[i].sem);
			
			if (ca->slot_info[i].rx_buffer.data != NULL) 
			{
				dvb_ringbuffer_flush(&ca->slot_info[i].rx_buffer);
			}
			up_write(&ca->slot_info[i].sem);
		}
	}

	ca->open = 1;
	ci_thread_update_delay(ca);
	ci_thread_wakeup(ca);

	return 0;
}


/**
 * Implementation of file close syscall.
 *
 * @param inode Inode concerned.
 * @param file File concerned.
 *
 * @return 0 on success, <0 on failure.
 */
static 
int ci_io_release(struct inode *inode, struct file *file)
{
	int err = 0;

	ci_function_trace();

	/* mark the CA device as closed */
	ca->open = 0;
	ci_thread_update_delay(ca);

	err = dvb_generic_release(inode, file);
	return 0;
}


/**
 * Implementation of poll() syscall.
 *
 * @param file File concerned.
 * @param wait poll wait table.
 *
 * @return Standard poll mask.
 */
static 
unsigned int ci_io_poll(
    struct file*            file, 
    poll_table*             wait
    )
{
	unsigned int mask = 0;
	int slot;
	int result = 0;

	ci_function_trace();

	if (ci_io_read_condition(ca, &result, &slot) == 1)
	{
		up_read(&ca->slot_info[slot].sem);
		mask |= POLLIN;
	}

	/* if there is something, return now */
	if (mask)
		return mask;

	/* wait for something to happen */
	poll_wait(file, &ca->wait_queue, wait);

	if (ci_io_read_condition(ca, &result, &slot)==1) 
	{
		up_read(&ca->slot_info[slot].sem);
		mask |= POLLIN;
	}

	return mask;
}



/*********************************************************************************
 * Group : EN50221 module Init/Exit functions 
 * Desc  : This Group of funcion is used to bringup/cleanup CI Device Driver
 **********************************************************************************/


static ci_int_handler ci_isr = 
{
    .ci_camchange_irq = ci_camchange_irq,
    .ci_frda_irq      = ci_frda_irq
};



static 
struct file_operations cidev_fops = 
{
	.owner   = THIS_MODULE,
	.read    = ci_io_read,
	.write   = ci_io_write,
	.ioctl   = ci_io_ioctl,
	.open    = ci_io_open,
	.release = ci_io_release,
	.poll    = ci_io_poll,
};




/*------------------------------------------------------------------
 * Func : ci_module_init
 *
 * Desc : start DVB CA EN50221 interface device driver.
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/    
static __init 
int ci_module_init(void)
{
	int ret, res;
	int i;
	int slot_count;
	int flags =  DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE |
					  DVB_CA_EN50221_FLAG_IRQ_FR |
					  DVB_CA_EN50221_FLAG_IRQ_DA;

	ci_function_trace();

	//Chuck adjust slot count specified by 'make menuconfig' on 20051031
	//Note that the default value is 1.
	if(CONFIG_MARS_PCMCIA_SLOT_NUM == 2)
		slot_count = 2;
	else
		slot_count = 1;

	/* initialise the system data */
	
	ca = (struct dvb_ca_private *) kmalloc(sizeof(struct dvb_ca_private), GFP_KERNEL);
	
	if (ca == NULL) 
	{
	    ci_warning("init ci driver failed - out of memory\n");
		ret = -ENOMEM;
		goto error;
	}
	
	memset(ca, 0, sizeof(struct dvb_ca_private));
	ca->flags = flags;
	ca->slot_count = slot_count;
	ca->slot_info = kmalloc(sizeof(struct dvb_ca_slot) * slot_count, GFP_KERNEL);
	
	if (ca->slot_info==NULL) 
	{
	    ci_warning("init ci driver failed - alloc slot info failed\n");
		ret = -ENOMEM;
		goto error;
	}
	
	memset(ca->slot_info, 0, sizeof(struct dvb_ca_slot) * slot_count);
	init_waitqueue_head(&ca->wait_queue);
	ca->thread_pid = 0;
	init_waitqueue_head(&ca->thread_queue);
	ca->exit = 0;
	ca->open = 0;
	ca->wakeup = 0;
	ca->next_read_slot = 0;

	if (register_chrdev(CI_MAJOR,"ca",&cidev_fops)) 
	{
		ci_warning("init ci driver failed - unable to get major %d for ci device\n", CI_MAJOR);
		return -EIO;
	}

	/* now initialise each slot */
	for (i = 0; i < slot_count; i++) 
	{
		memset(&ca->slot_info[i], 0, sizeof(struct dvb_ca_slot));
		ci_update_slot_state(ca, i, DVB_CA_SLOTSTATE_NONE);	
		atomic_set(&ca->slot_info[i].camchange_count, 0);
		ca->slot_info[i].camchange_type = DVB_CA_EN50221_CAMCHANGE_REMOVED;
		init_rwsem(&ca->slot_info[i].sem);
	}

	if (signal_pending(current)) 
	{
		ret = -EINTR;
		goto error;
	}
	mb();
	
	/* create a kthread for monitoring this CA device */
	ret = kernel_thread(ci_thread, ca, 0);

	if (ret < 0) 
	{
		ci_warning("init ci module failed - failed to start kernel_thread (%d)\n", ret);
		goto error;
	}
	
	ca->thread_pid = ret;

    rtdpc_registerCiIntHandler(&ci_isr);
    
	return 0;

error:
	if (ca != NULL) 
	{
		if ((res = unregister_chrdev(CI_MAJOR,"ci"))) 
		{
			ci_warning("unable to release major %d for ci device\n", CI_MAJOR);
			return res;
		}

		if (ca->slot_info != NULL)
			kfree(ca->slot_info);
			
		kfree(ca);
	}
	return ret;
}


 
/*------------------------------------------------------------------
 * Func : ci_module_exit
 *
 * Desc : remove DVB CA EN50221 interface device driver.
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/   
static __exit
void ci_module_exit()
{
	int i, res;

	ci_function_trace();
	
	if (ca->thread_pid) 
	{
		if (kill_proc(ca->thread_pid, 0, 1) == -ESRCH) 
		{
			ci_warning("%s - dvb_ca_release thread PID %d already died\n", __FUNCTION__ ,ca->thread_pid);
		} 
		else 
		{
			ca->exit = 1;
			mb();
			ci_thread_wakeup(ca);
			wait_event_interruptible(ca->thread_queue, ca->thread_pid == 0);
		}
	}

	for (i = 0; i < ca->slot_count; i++) 	
		ci_slot_shutdown(ca, i);
	
	kfree(ca->slot_info);

	if ((res = unregister_chrdev(CI_MAJOR,"ci"))) 
	{
		ci_warning("%s - unable to release major %d for ci device\n", __FUNCTION__ ,CI_MAJOR);
	}

	kfree(ca);
}

module_init(ci_module_init);
module_exit(ci_module_exit);


