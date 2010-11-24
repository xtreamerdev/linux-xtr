/*
 * dvb-dibusb-fe-i2c.c is part of the driver for mobile USB Budget DVB-T devices
 * based on reference design made by DiBcom (http://www.dibcom.fr/)
 *
 * Copyright (C) 2004-5 Patrick Boettcher (patrick.boettcher@desy.de)
 *
 * see dvb-dibusb-core.c for more copyright details.
 *
 * This file contains functions for attaching, initializing of an appropriate
 * demodulator/frontend. I2C-stuff is also located here.
 *
 */
#include <linux/usb.h> 
#include <linux/delay.h> 
#include "dvb-dibusb.h"
#include "rtd2830_cypress-fe-i2c.h" 



struct i2c_algorithm rtd2830_algo = 
{
	.name			= "RTD2830 USB i2c algorithm",
	.id				= I2C_ALGO_BIT,
	.master_xfer	= rtd2830_i2c_xfer,
	.functionality	= rtd2830_i2c_func,
};




/* rt2830 usb 2 I2C master xfer function
 * we don't need to set demod2830 i2c repeater, FW will take care it. 
 */
int rtd2830_i2c_xfer(struct i2c_adapter *adap,struct i2c_msg *msg,int num)
{
    int i;
	struct usb_dibusb *dib = i2c_get_adapdata(adap);	
        
	if (down_interruptible(&dib->i2c_sem) < 0)
		return -EAGAIN;
        
	if (num > 2)
		warn("more than 2 i2c messages at a time is not handled yet. TODO.");
		    
    if (0xffff== msg[0].flags){
        if (rtd2830_user_cmd(dib, msg[0].buf, msg[0].len) < 0)
            i= 0;        
        else            
            i= num;        
    }
    else{
        for (i = 0; i < num; i++) {
            /* write/read request */
            if (i+1 < num && (msg[i+1].flags & I2C_M_RD)) 
            {      
                if (rtd2830_i2c_msg(dib, msg[i].addr, msg[i].buf,msg[i].len, msg[i+1].buf,msg[i+1].len) < 0){
                    err("write i2c error");
                    break;
                }
                i++;
            }
            else{                     
                if (rtd2830_i2c_msg(dib, msg[i].addr, msg[i].buf,msg[i].len,NULL,0) < 0){
                    err("write i2c error");
                    break;
                }                              
            }
        }
    }
    udelay(60);
	up(&dib->i2c_sem);
	return i;
}




int rtd2830_i2c_msg(
    struct usb_dibusb*      dib, 
    u8                      addr,
    u8*                     wbuf, 
    u16                     wlen, 
    u8*                     rbuf, 
    u16                     rlen)
{	
  
#if 0  
  	u8 sndbuf[wlen+4]; /* lead(1) devaddr,direction(1) addr(2) data(wlen) (len(2) (when reading)) */
	/* write only ? */
	int wo = (rbuf == NULL || rlen == 0),
		len = 2 + wlen + (wo ? 0 : 2);

	sndbuf[0] = wo ? DIBUSB_REQ_I2C_WRITE : DIBUSB_REQ_I2C_READ;
	sndbuf[1] = (addr << 1) | (wo ? 0 : 1);

	memcpy(&sndbuf[2],wbuf,wlen);

	if (!wo) {
		sndbuf[wlen+2] = (rlen >> 8) & 0xff;
		sndbuf[wlen+3] = rlen & 0xff;
	}

	return dibusb_readwrite_usb(dib,sndbuf,len,rbuf,rlen);


#else
  
  
    int len;
    //--------------------------------------------------    
    // Write I2C : [OPC(0)] [DevAddr(1)][Wlen(2)][ADDR(3)][Data(4...N)]
    // Read  I2C : [OPC(0)] [DevAddr(1)][Rlen(2)][ADDR(3)]
    
    u8 sndbuf[wlen+3]; // lead(1) devaddr,direction(1) addr(2) data(wlen) (len(2) (when reading)) 
    
    if (rbuf == NULL || rlen == 0)
    {        
        sndbuf[0] = DIBUSB_REQ_I2C_WRITE;	
	    sndbuf[1] =(addr << 1) ;                    // Addr
	    sndbuf[2] = wlen;                           // xxxxx
	    memcpy(&sndbuf[3],wbuf,wlen);               
	    len = wlen + 3;        
    }
    else{
        sndbuf[0] = DIBUSB_REQ_I2C_READ;	        // OPC
	    sndbuf[1] =(addr << 1) | 1;                 // Addr
	    sndbuf[2] = rlen;                           // xxxxx
	    memcpy(&sndbuf[3],wbuf,1);                  // Register Address
	    len = 4;        	    
    }    	      
    return dibusb_readwrite_usb(dib,sndbuf,len,rbuf,rlen);       
#endif    
}



int rtd2830_user_cmd(
    struct usb_dibusb*      dib, 
    u8*                     sndbuf, 
    u8                      snd_len)
{
	return dibusb_readwrite_usb(dib,sndbuf,snd_len, NULL, 0);
}



u32 rtd2830_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C;
}


int rtd2830_tuner_quirk(struct usb_dibusb *dib)
{    
    int     i;
    int     tuner_count;
    struct  dibusb_tuner *pTuner = dib->tuner;	
	unsigned char   data[4];
	struct i2c_msg  msg[2];		
	unsigned char check_list[3][4] = 
	{
        {   DIBUSB_TUNER_COFDM_QT1010, 
            0x0F,
            0x2C,                        
        },        
        {   DIBUSB_TUNER_COFDM_PHILIPS_TD1316, 
            0x00,
            0x00,            
        },   
        {   DIBUSB_TUNER_COFDM_LG_TDTMG252D, 
            0x00,
            0x00,            
        },   
    };        
	printk("Run rtd2830_tuner_quirk\n");	    

    ///////////////////////////////////////	
	
	tuner_count = sizeof(check_list)/3;
	
	for (i=0;i<tuner_count;i++)
	{	    
	    pTuner       = &dibusb_tuner[check_list[i][0]]; 
	    
    	data[0]      = check_list[i][1];
    	msg[0].addr  = pTuner->pll_addr;
    	msg[0].flags = 0;
    	msg[0].buf   = &data[0];
    	msg[0].len   = 1;
    		
    	msg[1].addr  = pTuner->pll_addr;
    	msg[1].flags = I2C_M_RD;
    	msg[1].buf   = &data[1];
    	msg[1].len   = 1;    	    	
                	
             
        if (rtd2830_i2c_xfer(&dib->i2c_adap,msg,2) != 2)        
            printk("%02x, %02x, Fail\n",pTuner->pll_addr,data[0]);
        else        
        {
            printk("%02x, %02x, (%02x/%02x)\n",pTuner->pll_addr,data[0],data[1],check_list[i][2]);
            if (data[1] == check_list[i][2] || (check_list[i][1]==0 && check_list[i][2]==0))
                break;                     
        }
    }		 
      
    switch (i){
    case 0:            
        printk("Run rtd2830_tuner_quirk complete : Quantek QT1010\n");    
        dib->tuner = &dibusb_tuner[check_list[i][0]];    
        dib->fe->ext_flag = 0;        
        break;      
    case 1:
        printk("Run rtd2830_tuner_quirk complete : Philips TD1316\n");    
        dib->tuner = &dibusb_tuner[check_list[i][0]];    
        dib->fe->ext_flag = 0;        
        break;      
    case 2:              
        printk("Run rtd2830_tuner_quirk complete : LG_TDTMG252D\n");    
        dib->tuner = &dibusb_tuner[check_list[i][0]];    
        dib->fe->ext_flag = 0;
        break;      
    default:
        printk("Run rtl2830_tuner_quirk fail, using default : Philips TD1316 \n");        
        dib->tuner = &dibusb_tuner[DIBUSB_TUNER_COFDM_PHILIPS_TD1316];          
        dib->fe->ext_flag = 0;
        break;
                    
    }                       
            
    return 0;           		                    
}



/*=======================================================
 * Func : rtd2830_streaming 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 
 *=======================================================*/ 
int rtd2830_streaming(void *dib,int onoff)
{
    if (onoff){
        RT2830_CYPRESS_DBG("%s : Streaming On \n",__FUNCTION__);                
    }
    else{
        RT2830_CYPRESS_DBG("%s : Streaming Off \n",__FUNCTION__);    
    }    
    return dibusb_streaming((struct usb_dibusb*) dib,onoff);    
}




int rtd2830_urb_init(
    void*               dib1,
    u8*                 p_buffer,
    int                 urb_number,
    int                 urb_size
    )
{
	int                 i;
    int                 bufsize;
    int                 def_pid_parse= 1;
    struct usb_dibusb*  dib= (struct usb_dibusb*)dib1; 

	/*
	 * when reloading the driver w/o replugging the device
	 * a timeout occures, this helps
	 */
	 /*
	usb_clear_halt(dib->udev,usb_sndbulkpipe(dib->udev,dib->dibdev->dev_cl->pipe_cmd));
	usb_clear_halt(dib->udev,usb_rcvbulkpipe(dib->udev,dib->dibdev->dev_cl->pipe_cmd));
	usb_clear_halt(dib->udev,usb_rcvbulkpipe(dib->udev,dib->dibdev->dev_cl->pipe_data));	
    */
    // Begin Add by Richard
	bufsize                              = urb_number* urb_size;
	dib->buffer                          = p_buffer;
    dib->dibdev->dev_cl->urb_count       = urb_number;      // update urb number
    dib->dibdev->dev_cl->urb_buffer_size = urb_size;        // update urb size
    // End Add by Richard

	printk("====================== rtd2830_dibusb_urb_init =========================\n");
    printk("(p_buffer, urb_count, urb_buffer_size)= (0x%08x, %d, %d)\n",
            (int)p_buffer, dib->dibdev->dev_cl->urb_count, dib->dibdev->dev_cl->urb_buffer_size);
            
	/* allocate the array for the data transfer URBs */
	dib->urb_list = kmalloc(dib->dibdev->dev_cl->urb_count*sizeof(struct urb *),GFP_KERNEL);
	
	if (dib->urb_list == NULL)
		return -ENOMEM;
		
	memset(dib->urb_list, 0, dib->dibdev->dev_cl->urb_count*sizeof(struct urb *));

	dib->init_state |= DIBUSB_STATE_URB_LIST;

	deb_info("allocation complete\n");
	memset(dib->buffer,0,bufsize);

	// dib->init_state |= DIBUSB_STATE_URB_BUF;

	/* allocate and submit the URBs */
	for (i = 0; i < dib->dibdev->dev_cl->urb_count; i++) 
	{
        // printk("%d: Bad parameter: (p_buffer, buffer length, urb size)= (0x%08x, %d, %d)\n", i, (int)&dib->buffer[i*dib->dibdev->dev_cl->urb_buffer_size], len_buffer, urb_size);
		if (!(dib->urb_list[i] = usb_alloc_urb(0,GFP_ATOMIC))) {
			return -ENOMEM;
		}

		usb_fill_bulk_urb( 
		    dib->urb_list[i], 
		    dib->udev,
			usb_rcvbulkpipe(dib->udev,dib->dibdev->dev_cl->pipe_data),
			&dib->buffer[i*dib->dibdev->dev_cl->urb_buffer_size],
			dib->dibdev->dev_cl->urb_buffer_size,
			dibusb_urb_complete, 
			dib);

		dib->urb_list[i]->transfer_flags = 0;		
		dib->init_state |= DIBUSB_STATE_URB_INIT;
	}

	/* dib->pid_parse here contains the value of the module parameter */
	/* decide if pid parsing can be deactivated:
	 * is possible (by device type) and wanted (by user)
	 */
	/* from here on it contains the device and user decision */
	dib->pid_parse = def_pid_parse;    
    printk("====================== rtd2830_dibusb_urb_init complete ================\n");
	return 0;
}


int rtd2830_urb_exit(void* dib1)
{
	int                 i;
    struct usb_dibusb*  dib= (struct usb_dibusb*)dib1; 

	dibusb_urb_kill(dib);

	if ((dib->init_state & DIBUSB_STATE_URB_LIST)) 
	{
		for (i = 0; i < dib->dibdev->dev_cl->urb_count; i++) 
		{		    
			if (dib->urb_list[i] != NULL) 
			{
				deb_info("freeing URB no. %d.\n",i);
				/* free the URBs */
				usb_free_urb(dib->urb_list[i]);
			}
		}
		/* free the urb array */
		kfree(dib->urb_list);
		dib->init_state &= ~DIBUSB_STATE_URB_LIST;
	}

	dib->init_state &= ~DIBUSB_STATE_URB_BUF;
	dib->init_state &= ~DIBUSB_STATE_URB_INIT;
	return 0;
}



int rtd2830_urb_reset(void* dib1)
{
	int                 i;
    struct usb_dibusb*  dib= (struct usb_dibusb*)dib1; 

    // don't know if it's required or not
	/*
	 * when reloading the driver w/o replugging the device
	 * a timeout occures, this helps
	 */

	usb_clear_halt(dib->udev,usb_sndbulkpipe(dib->udev,dib->dibdev->dev_cl->pipe_cmd));
	usb_clear_halt(dib->udev,usb_rcvbulkpipe(dib->udev,dib->dibdev->dev_cl->pipe_cmd));
	usb_clear_halt(dib->udev,usb_rcvbulkpipe(dib->udev,dib->dibdev->dev_cl->pipe_data));    

    printk("====================== rtd2830_dibusb_urb_reset =========================\n");
    printk("(buffer, urb_count, urb_buffer_size)= (0x%08x, %d, %d)\n",
        (int)dib->buffer, dib->dibdev->dev_cl->urb_count, dib->dibdev->dev_cl->urb_buffer_size);

	/* allocate and submit the URBs */
	for (i = 0; i < dib->dibdev->dev_cl->urb_count; i++) {

        dib->urb_list[i]->transfer_buffer= (void*)&dib->buffer[i*dib->dibdev->dev_cl->urb_buffer_size];
		dib->urb_list[i]->transfer_flags = 0; 	
	}

	/* dib->pid_parse here contains the value of the module parameter */
	/* decide if pid parsing can be deactivated:
	 * is possible (by device type) and wanted (by user)
	 */
	/* from here on it contains the device and user decision */
	// dib->pid_parse = def_pid_parse;    
    printk("====================== rtd2830_dibusb_urb_reset complete ================\n");
	return 0;
}

