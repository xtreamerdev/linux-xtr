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
#include "gl861-fe-i2c.h"

#define  GL861_I2C_ADDR     0x00

int chk_val_x(struct usb_dibusb* dib);


//------------------------------------------------------  
struct i2c_algorithm gl861_algo = 
{
	.name			= "GL861 USB i2c algorithm",
	.id				= I2C_ALGO_BIT,
	.master_xfer	= gl861_i2c_xfer,
	.functionality	= gl861_i2c_func,
};
//------------------------------------------------------    

    
int _gl861_register_read(
    struct usb_dibusb*      dib,
    unsigned char           addr,
    unsigned short          offset, 
    unsigned char*          data, 
    unsigned short          bytelength
    );
    
int _gl861_register_write(
    struct usb_dibusb*      dib,
    unsigned char           addr,
    unsigned short          offset, 
    unsigned char*          data, 
    unsigned short          bytelength
    );     
          
              
int _gl861_repeator_on(
    struct usb_dibusb*      dib
    );
        
int rtl2830_repeator_on(
    struct usb_dibusb*      dib
    );    

int rtl2820_repeator_on(
    struct usb_dibusb*      dib
    );             
    
int _gl861_set_pid_filter(
    struct usb_dibusb*      dib, 
    unsigned char           PIDSetPass,
    unsigned char           number,
    unsigned short*         pPIDs
    );

int _gl861_flush_on(
    struct usb_dibusb*      dib
    );    
    
int _gl861_video_stream_on(
    struct usb_dibusb*      dib,
    unsigned char           on_off
    );    
        
//------------------------------------------------------    
int init = 0;
    
/*=======================================================
 * Func : gl861_i2c_xfer 
 *
 * Desc : main transfer function of gl861 usb 2 i2c bridge
 *
 * Parm : 
 *
 * Retn : handle of comm 
 *=======================================================*/     
int gl861_i2c_xfer(
    struct i2c_adapter*     adap,
    struct i2c_msg*         msg,
    int                     num)
{
    struct usb_dibusb *dib = i2c_get_adapdata(adap);	
	int ret;	
    int i; 
    int demod_msg = 0;    	
 
	if (down_interruptible(&dib->i2c_sem) < 0)	    
		return -EAGAIN;    
        
	if (num > 2)
		GL861_WARNING("more than 2 i2c messages at a time is not handled yet. TODO.");			   
	
	
	switch(msg[0].flags)
	{
    //----	    
    case 0xffff:
        ret = (_gl861_set_pid_filter(dib, 1, msg[0].buf[1],(u16*) &msg[0].buf[2])==0) ? num : -1;
        break;
    
    //----        
    default:
    
        if (msg[0].addr == dib->tuner->pll_addr || msg[0].addr == dib->tuner->ext_pll_addr)                            
            _gl861_repeator_on(dib);
       
        if (num>1)
        {   
            /* Note : this function only can read 1 byte for each time */    
            if (msg[1].len > 1){
                GL861_WARNING("%s : %d : %s : warning : read length more than 1 (%d)\n",__FILE__,__LINE__,__FUNCTION__,msg[1].len);
            }
            ret = _gl861_register_read(
                    dib,                
                    msg[0].addr<<1,     // tuner address
                    msg[0].buf[0],      // offset address
                    msg[1].buf,         // receieve data buffer
                    msg[1].len);        // receieve data length                 
        }
        else{                                                
            ret = _gl861_register_write(
                    dib,
                    msg[0].addr<<1,     // tuner address
                    msg[0].buf[0],      // offset address
                    &msg[0].buf[1],     // data address
                    msg[0].len-1);	    // write data length                              
        }			        
        ret = (ret <0 ) ? -1 : (num>1) ? 2 : 1;  
    }	    
    
	up(&dib->i2c_sem);
	
	return ret;
}



/*=======================================================
 * Func : gl861_i2c_func 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : handle of comm 
 *=======================================================*/ 
u32 gl861_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C;
}



/*=======================================================
 * Func : gl861_streaming 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 
 *=======================================================*/ 
int gl861_streaming(void *dib,int onoff)
{
    int ret;
            
    if (onoff)
    {
        GL861_DBG("%s : Streaming On \n",__FUNCTION__);        
        
        _gl861_video_stream_on((struct usb_dibusb*)dib,0);      // Streaming Off        
        _gl861_flush_on((struct usb_dibusb*)dib);               // Flush FIFO
        ret = dibusb_streaming((struct usb_dibusb*) dib,onoff); // Submit URB
        _gl861_video_stream_on((struct usb_dibusb*)dib,1);      // Streaming On        
    }
    else
    {
        GL861_DBG("%s : Streaming Off \n",__FUNCTION__);        
        
        ret = dibusb_streaming((struct usb_dibusb*) dib,onoff); // Kill URB
        _gl861_video_stream_on((struct usb_dibusb*)dib,0);      // Streaming Off
        _gl861_flush_on((struct usb_dibusb*)dib);               // Flush FIFO
    }
    return ret;
}



/*=======================================================
 * Func : _gl861_register_read 
 *
 * Desc : read i2c via gl861
 *
 * Parm : dib   : handle of usb_dibusb 
 *        addr  : device address 
 *        offset: register offset
 *        data  : receieve data buffer
 *        bytelength : number of byte to read 
 *
 * Retn : handle of comm 
 *=======================================================*/ 
int _gl861_register_read(
    struct usb_dibusb*      dib,
    unsigned char           addr,
    unsigned short          offset, 
    unsigned char*          data, 
    unsigned short          bytelength
    )
{
	int ret = -1;

	if ((ret = down_interruptible(&dib->usb_sem))<0)
		return ret;
 
    ret = usb_control_msg(
            dib->udev,                      // pointer to device 
            usb_rcvctrlpipe( dib->udev,dib->dibdev->dev_cl->pipe_cmd),   // pipe to control endpoint 
            GL861_I2C_REG_BYTE_READ,        // request  
            SKEL_VENDOR_IN,                 // request type
            addr<<8,                        // request value : [I2C Addr][reserved]
            offset,                         // index : register offset
            data,                           // pointer to the receive buffer 
            bytelength,                     // length of receiece buffer 
            DIBUSB_I2C_TIMEOUT);            // time to wait for the message to complete before timing out 
              
    if (ret != bytelength){
        GL861_WARNING(" size != %d bytes\n", ret);
        ret = -1;    
    }    
    else
        ret = 0;

	up(&dib->usb_sem);
	return ret; 
}



/*=======================================================
 * Func : _gl861_register_write 
 *
 * Desc : write i2c via gl861 (multi byte)
 *
 * Parm : dib   : handle of usb_dibusb 
 *        addr  : device address
 *        offset: register offset
 *        data  : transfer data buffer
 *        bytelength : number of byte to write 
 *
 * Retn : handle of comm 
 *=======================================================*/ 
int _gl861_register_write(
    struct usb_dibusb*      dib,
    unsigned char           addr,    
    unsigned short          offset, 
    unsigned char*          data, 
    unsigned short          bytelength
    )
{
	int ret = -1;    

	if ((ret = down_interruptible(&dib->usb_sem))<0)
		return ret;
        
    ret = usb_control_msg(
            dib->udev,                      // pointer to device 
            usb_sndctrlpipe(dib->udev,dib->dibdev->dev_cl->pipe_cmd),   // pipe to control endpoint 
            GL861_I2C_REG_SHORT_BYTE_WRITE, // request 
            SKEL_VENDOR_OUT,                // request type
            addr<<8,                        // request value : i2c address of device
            offset,                         // index : register address
            data,                           // data buffer 
            bytelength,                     // length of data buffer 
            DIBUSB_I2C_TIMEOUT);            // time to wait for the message to complete before timing out 
    	
    if (ret != bytelength){
        GL861_WARNING(" size != %d bytes\n", ret);
        ret = -1;    
    }    
    else
        ret = 0;    
    
    up(&dib->usb_sem);
    return ret;
}
 
 

/*=======================================================
 * Func : _gl861_video_stream_on 
 *
 * Desc : write i2c via gl861
 *
 * Parm : dib   : handle of usb_dibusb       
 *        on_off: 0 : off , others : on  
 * Retn : 0/1
 *=======================================================*/ 
int _gl861_video_stream_on(
    struct usb_dibusb*      dib,
    unsigned char           on_off
    )
{
    int ret;                     
    
	if ((ret = down_interruptible(&dib->i2c_sem))<0){
	    GL861_WARNING("%s : %d : %s : fail0 \n",__FILE__,__LINE__,__FUNCTION__);
		return ret;
    }
		
	if ((ret = down_interruptible(&dib->usb_sem))<0){	    
        GL861_WARNING("%s : %d : %s : fail1n",__FILE__,__LINE__,__FUNCTION__);
        up(&dib->i2c_sem);
		return ret;	
    }

    GL861_DBG("%s : %d : %s : on_off = %d\n",__FILE__,__LINE__,__FUNCTION__,on_off);
    
    usb_control_msg(
            dib->udev,                      // pointer to device 
            usb_sndctrlpipe(dib->udev,dib->dibdev->dev_cl->pipe_cmd),   // pipe to control endpoint 
            GL861_VIDEO_STREAM,             // request 
            SKEL_VENDOR_OUT,                // request type
            (on_off) ? 1 : 0,               // request value : 1 start / 0 stop
            0,                              // index : register address
            NULL,                           // data buffer 
            0,                              // length of data buffer 
            DIBUSB_I2C_TIMEOUT);            // time to wait for the message to complete before timing out
    
    up(&dib->usb_sem);
    up(&dib->i2c_sem);
       
    return 0;
}    
    

/*=======================================================
 * Func : _gl861_repeator_on 
 *
 * Desc : write i2c via gl861
 *
 * Parm : dib   : handle of usb_dibusb       
 *
 * Retn : 0/-1
 *=======================================================*/ 
int _gl861_repeator_on(struct usb_dibusb* dib)
{
    int demod_id = (int) dib->dibdev->dev_cl->demod->id;
    switch (demod_id){
    case DEMOD_RTD2830:
        return rtl2830_repeator_on(dib);            
    case DEMOD_RTL2820:
        return rtl2820_repeator_on(dib);                 
    }           
    return 0; 
}    




    
/*=======================================================
 * Func : rtl2830_repeator_on 
 *
 * Desc : repeat on function for rtl2830
 *
 * Parm : dib   : handle of usb_dibusb       
 *
 * Retn : 0/-1
 *=======================================================*/     
int rtl2830_repeator_on(struct usb_dibusb* dib)
{
    int ret;
    unsigned char addr = dib->dibdev->dev_cl->demod->i2c_addrs[0];  
    unsigned char val;
                        
    // set page register to page 1
    val=1;    
    ret = _gl861_register_write(
                dib,                
                addr<<1,    // demod address
                0,          // offset address
                &val,       // data length
                1);         // length
                
    if (ret <0){
        GL861_WARNING("%s : %d : %s : set page register fail\n",__FILE__,__LINE__,__FUNCTION__);
        return ret;
    }
    
    // read register 1 
    ret = _gl861_register_read(
                dib,                
                addr<<1,                    // demod address
                1,                          // offset address
                &val,                       // data buffer
                1);                         // data length)

    if (ret <0){
        GL861_WARNING("%s : %d : %s : read register 1 fail\n",__FILE__,__LINE__,__FUNCTION__);
        return ret;
    }            
    
    // set page register to page 1
    val=1;    
    ret = _gl861_register_write(
                dib,                
                addr<<1,    // demod address
                0,          // offset address
                &val,       // data length
                1);
                
    if (ret <0){
        GL861_WARNING("%s : %d : %s : set page register fail\n",__FILE__,__LINE__,__FUNCTION__);
        return ret;
    }    
    
    // write register 1                                                            
    val |= 0x08;
    ret = _gl861_register_write(
                dib,                
                addr<<1,                    // demod address
                1,                          // offset address
                &val,
                1);
    
    if (ret <0)
        GL861_WARNING("%s : %d : %s : write page register fail\n",__FILE__,__LINE__,__FUNCTION__);    
    
    return ret;      
}
        

/*=======================================================
 * Func : rtl2830_repeator_on 
 *
 * Desc : repeat on function for rtl2830
 *
 * Parm : dib   : handle of usb_dibusb       
 *
 * Retn : 0/-1
 *=======================================================*/         
int rtl2820_repeator_on(struct usb_dibusb* dib)
{
    int ret;
    unsigned char addr = dib->dibdev->dev_cl->demod->i2c_addrs[0];  
    unsigned char val;
        
    GL861_WARNING("%s : %d : %s \n",__FILE__,__LINE__,__FUNCTION__);        
    // set page register to page 0
    val=0;    
    ret = _gl861_register_write(
                dib,                
                addr<<1,    // demod address
                0,          // offset address
                &val,       // data length
                1);         // length
                
    if (ret <0){
        GL861_WARNING("%s : %d : %s : set page register fail\n",__FILE__,__LINE__,__FUNCTION__);
        return ret;
    }
    
    // read register 1 
    ret = _gl861_register_read(
                dib,                
                addr<<1,                    // demod address
                6,                          // offset address
                &val,                       // data buffer
                1);                         // data length)

    if (ret <0){
        GL861_WARNING("%s : %d : %s : read register 1 fail\n",__FILE__,__LINE__,__FUNCTION__);
        return ret;
    }            
    
    // set page register to page 1
    
    // write register 1                                                            
    val |= 0x80;
    ret = _gl861_register_write(
                dib,                
                addr<<1,                    // demod address
                6,                          // offset address
                &val,
                1);
    
    if (ret <0)
        GL861_WARNING("%s : %d : %s : write page register fail\n",__FILE__,__LINE__,__FUNCTION__);    
    
    return ret;   
    
}        
        

#if 1
/*=======================================================
 * Func : _gl861_set_pid_filter 
 *
 * Desc : 
 *
 * Parm : dib        : handle of usb_dibusb       
 *        PIDSetPass : 0 : don't pass specified PID 
 *                     1 : only specified PID could pass 
 *        number     : number of PIDs           
 *        pPIDs      : PID List
 *          
 * Retn : 0/-1
 *=======================================================*/
int _gl861_set_pid_filter(
    struct usb_dibusb*      dib, 
    unsigned char           PIDSetPass,
    unsigned char           number,
    unsigned short*         pPIDs
    )
{
    int i;    
    unsigned char page;     
    unsigned char index;    
    unsigned char val;
          
    unsigned char mask[4][2] = {
                    {GL861_PIDFILTER_SET1_MASK, 0x00},
                    {GL861_PIDFILTER_SET2_MASK, 0x00},
                    {GL861_PIDFILTER_SET3_MASK, 0x00},
                    {GL861_PIDFILTER_SET4_MASK, 0x00},                    
                }; 
    unsigned char pid_reg[4][2] = {
                    {GL861_PIDFILTER_SET12_HB  , GL861_PIDFILTER_SET12_LB},
                    {GL861_PIDFILTER_SET12_HB+8, GL861_PIDFILTER_SET12_LB+8},
                    {GL861_PIDFILTER_SET34_HB  , GL861_PIDFILTER_SET34_LB},
                    {GL861_PIDFILTER_SET34_HB+8, GL861_PIDFILTER_SET34_LB+8},
                };
#if 0                     
    GL861_DBG("%s : %d : %s \n",__FILE__,__LINE__,__FUNCTION__);        
#endif    
            
    if (number == 0xFF || number > 32)
    {   
#if 0           
        GL861_DBG("----------------------------------\n");                         
        GL861_DBG("Mode : ALL Pass \n");
        GL861_DBG("----------------------------------\n");
#endif
        
        val=0x0;        //disable PID filter Hardware
        if (_gl861_register_write(dib, GL861_I2C_ADDR ,GL861_DTV_TMOD_CTL,&val,1)<0)
            goto err_set_gl861_reg_fail; 

        val=0x0;        //disable PID filter funciton
        if (_gl861_register_write(dib, GL861_I2C_ADDR ,GL861_DTV_MASK_EN,&val,1)<0)
            goto err_set_gl861_reg_fail;  
        
    }
    else
    {     
        
#if 0                                                   
        if (number == 0)
            GL861_DBG("Mode : No Pass \n");        
        else
            GL861_DBG("Mode : Band Pass \n");                            
#endif        
        
        //Enable the PID filter            
        val = 0x08;                        
        if (_gl861_register_write(dib, GL861_I2C_ADDR, GL861_DTV_TMOD_CTL, &val, 1)<0)        
            goto err_set_gl861_reg_fail;                
        
        //Enable the PID filter Setting Clock                
        val = (PIDSetPass==0) ? 0x40 : 0xC0;                 
        if (_gl861_register_write(dib, GL861_I2C_ADDR , GL861_DTV_MASK_EN, &val, 1)<0)
            goto err_set_gl861_reg_fail;                                
            
#if 0    
        GL861_DBG("----------------------------------\n");            
#endif        
        
        for (i=0; i<number; i++)
        {       
#if 0         
            GL861_DBG("[%d] : PID = 0x%04x (%d)\n",i,pPIDs[i],pPIDs[i]);  
#endif            
            page = i >> 3;                            
            index = i & 0x07;
                                            
            val = pPIDs[i]>>8;
            if (_gl861_register_write(dib, GL861_I2C_ADDR ,pid_reg[page][0]+index, &val, 1)<0)
                goto err_set_gl861_reg_fail;
                            
            val = pPIDs[i] & 0xFF;            
            if (_gl861_register_write(dib, GL861_I2C_ADDR ,pid_reg[page][1]+index, &val, 1)<0)
                goto err_set_gl861_reg_fail;
                                
            mask[page][1] |= (1<<index);
        }
        
        for (i=0; i<4; i++){            
            if (_gl861_register_write(dib, GL861_I2C_ADDR ,mask[i][0], &mask[i][1], 1)<0)
                goto err_set_gl861_reg_fail;
        }
                
        if (_gl861_register_read(dib, GL861_I2C_ADDR , GL861_DTV_MASK_EN, &val, 1)<0)
            goto err_set_gl861_reg_fail;     
        val &= ~0x40;
        if (_gl861_register_write(dib, GL861_I2C_ADDR , GL861_DTV_MASK_EN, &val, 1)<0)
            goto err_set_gl861_reg_fail;     
          
#if 0
        GL861_DBG("----------------------------------\n");      
#endif        
    }    
    
#if 0    
    //-----------------------    
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x33, &val, 1);
    GL861_DBG("0x33 = %02x\n", val);
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x37, &val, 1);
    GL861_DBG("0x37 = %02x\n", val);
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x38, &val, 1);
    GL861_DBG("0x38 = %02x\n", val);    
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x80, &val, 1);
    GL861_DBG("0x80 = %02x\n", val);
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x90, &val, 1);
    GL861_DBG("0x90 = %02x\n", val);
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x81, &val, 1);
    GL861_DBG("0x81 = %02x\n", val);
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x91, &val, 1);
    GL861_DBG("0x91 = %02x\n", val);
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x82, &val, 1);
    GL861_DBG("0x82 = %02x\n", val);
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x92, &val, 1);
    GL861_DBG("0x92 = %02x\n", val);
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x83, &val, 1);
    GL861_DBG("0x83 = %02x\n", val);
    _gl861_register_read(dib, GL861_I2C_ADDR ,0x93, &val, 1);
    GL861_DBG("0x93 = %02x\n", val);
#endif    
    return 0;        
            
//---
err_set_gl861_reg_fail:
    GL861_WARNING("%s : %d : %s : gl861 register access fail\n",__FILE__,__LINE__,__FUNCTION__);
    return -1;
}

#else

        
typedef struct{
    unsigned char   mask_en;
    unsigned char   mask[4];    
    unsigned char   reserved;    
    unsigned short  PID[32];  
}gl861_pid_status;


/*=======================================================
 * Func : _gl861_get_pid_status
 *
 * Desc : 
 *
 * Parm : dib           : handle of usb_dibusb       
 *        p_pid_status  : pointer of a pid status data structure
 *          
 * Retn : 0/-1
 *=======================================================*/
int _gl861_get_pid_status(
    struct usb_dibusb*          dib, 
    gl861_pid_status*  p_pid_status    
    )
{
    int i;
    int page;
    int offset;    
    unsigned char buf[64];
    unsigned char pid_reg[4][2] = {
                    {0x00, 0x10},
                    {0x08, 0x18},
                    {0x20, 0x30},
                    {0x28, 0x38},
                };             
     
    if (p_pid_status != NULL)
    {      
        if (_gl861_register_read(dib, GL861_I2C_ADDR, GL861_DTV_MASK_EN, buf, 1)<0)
            goto get_pid_status_fail;                                            
        p_pid_status->mask_en = buf[0]; 
                 
        if (_gl861_register_read(dib, GL861_I2C_ADDR, GL861_PIDFILTER_SET1_MASK, buf, 1)<0)
            goto get_pid_status_fail;            
        p_pid_status->mask[0] = buf[0];                             
        
        if (_gl861_register_read(dib, GL861_I2C_ADDR, GL861_PIDFILTER_SET2_MASK, buf, 1)<0)
            goto get_pid_status_fail;                    
        p_pid_status->mask[1] = buf[0]; 
            
        if (_gl861_register_read(dib, GL861_I2C_ADDR, GL861_PIDFILTER_SET3_MASK, buf, 1)<0)
            goto get_pid_status_fail;        
        p_pid_status->mask[2] = buf[0]; 
        
        if (_gl861_register_read(dib, GL861_I2C_ADDR, GL861_PIDFILTER_SET4_MASK, buf, 1)<0)
            goto get_pid_status_fail;
        p_pid_status->mask[3] = buf[0];                                                                              
        
        /*
        GL861_DBG"mask = %02x : %02x%02x%02x%02x\n",
            p_pid_status->mask_en,
            p_pid_status->mask[3],
            p_pid_status->mask[2],
            p_pid_status->mask[1],
            p_pid_status->mask[0]    
        );
        */      
                
        if (_gl861_register_read(dib, GL861_I2C_ADDR, GL861_PIDFILTER_SET12_HB, buf, 64)<0)
            goto get_pid_status_fail;                   
                    
        for (i=0; i<32;i++){                 
            page = i>>3;                     
            
            offset = pid_reg[page][0] + (i&0x07);
            p_pid_status->PID[i] = buf[offset]<<8;    // HB
            
            offset = pid_reg[page][1] + (i&0x07);
            p_pid_status->PID[i]+= buf[offset];       // LB                        
            
//            GL861_DBG("[%d] %d (%04x)\n",i,p_pid_status->PID[i],p_pid_status->PID[i]);
        }                
                            
        return 0;
    }         
    
get_pid_status_fail:
    return -1;
}


/*=======================================================
 * Func : _gl861_set_pid_filter 
 *
 * Desc : 
 *
 * Parm : dib        : handle of usb_dibusb       
 *        PIDSetPass : 0 : don't pass specified PID 
 *                     1 : only specified PID could pass 
 *        number     : number of PIDs           
 *        pPIDs      : PID List
 *          
 * Retn : 0/-1
 *=======================================================*/
int _gl861_set_pid_filter(
    struct usb_dibusb*      dib, 
    unsigned char           PIDSetPass,
    unsigned char           number,
    unsigned short*         pPIDs
    )
{
    int i;
    int j;
    unsigned char page;     
    unsigned char index;    
    unsigned char val;    
    gl861_pid_status gl861_pid;    
          
    unsigned char mask[4][2] = {
                    {GL861_PIDFILTER_SET1_MASK, 0x00},
                    {GL861_PIDFILTER_SET2_MASK, 0x00},
                    {GL861_PIDFILTER_SET3_MASK, 0x00},
                    {GL861_PIDFILTER_SET4_MASK, 0x00},                    
                }; 
    unsigned char pid_reg[4][2] = {
                    {GL861_PIDFILTER_SET12_HB  , GL861_PIDFILTER_SET12_LB},
                    {GL861_PIDFILTER_SET12_HB+8, GL861_PIDFILTER_SET12_LB+8},
                    {GL861_PIDFILTER_SET34_HB  , GL861_PIDFILTER_SET34_LB},
                    {GL861_PIDFILTER_SET34_HB+8, GL861_PIDFILTER_SET34_LB+8},
                };
                               
    //GL861_DBG("%s : %d : %s \n",__FILE__,__LINE__,__FUNCTION__);
    
    if (_gl861_get_pid_status(dib, &gl861_pid)<0){
        GL861_WARNING("%s : %d : %s : can not get pid status\n",__FILE__,__LINE__,__FUNCTION__);
        return -1;
    }
    
    /* for debug
    for (j=0;j<number;j++){
        GL861_DBG("PID [%d] = %d \n",j,pPIDs[j]); 
    }
    */        

    if (number == 0xFF || number > 32){                                                                                                 
        val=0x00;
        if (_gl861_register_write(dib, GL861_I2C_ADDR ,GL861_DTV_MASK_EN,&val,1)<0)
            goto err_set_gl861_reg_fail;                                                                   
        return 0;
    }   

    /****************************************
     * 1) disable all un-used pids from the list
     * 2) add new pids to the list & enable these pids     
     * 3) set Pass Mode if necessary    
     ****************************************/        

    //--- enable PID setting clock     
    val = (PIDSetPass) ? 0xC0 : 0x40;
    if (_gl861_register_write(dib, GL861_I2C_ADDR , GL861_DTV_MASK_EN, &val, 1)<0)
        goto err_set_gl861_reg_fail;
          
    // setep 1 :  disable all not used pids from the list
    for (i=0; i<number; i++){                                 
        for (j=0;j<32;j++){
            if (gl861_pid.PID[j]==pPIDs[i]){
                mask[j>>3][1] |= (1<<(j&0x07));     // enable the specified bits
                pPIDs[i] = 0xFFFF;                  // clear this PID
                break;                
            }
        }                        
    }
            
    for (i=0; i<4; i++){       
        if (mask[i][1] != gl861_pid.mask[i]){
            if (_gl861_register_write(dib, GL861_I2C_ADDR ,mask[i][0], &mask[i][1], 1)<0)
                goto err_set_gl861_reg_fail;
            gl861_pid.mask[i] = mask[i][1];   // update mask status
        }
    }
    
    /* for debug    
    GL861_DBG("new mask = %02x%02x%02x%02x\n",            
            mask[3][1],
            mask[2][1],
            mask[1][1],
            mask[0][1]    
    );      
    */
    
    // setep 2 :  add new pids to the list & enable these pids        
    for (i=0; i<number; i++)
    {
        if (pPIDs[i] != 0xFFFF)
        {
            for (j=0;j<32;j++)
            {
                page = j>>3;
                index = j & 0x07;
                
                if (!(mask[page][1] & (1<<index)))
                {                               
//                  GL861_DBG("write PID %d to entry %d\n",pPIDs[i],j);        
                        
                    //--- set PID 
                    val = pPIDs[i]>>8;
                    if (_gl861_register_write(dib, GL861_I2C_ADDR ,pid_reg[page][0]+index, &val, 1)<0)
                        goto err_set_gl861_reg_fail;
                        
                    val = pPIDs[i] & 0xFF;            
                    if (_gl861_register_write(dib, GL861_I2C_ADDR ,pid_reg[page][1]+index, &val, 1)<0)
                        goto err_set_gl861_reg_fail;                                                                                                                                        
                    
                    mask[page][1] |= (1<<index);
                    pPIDs[i] = 0xFFFF;
                    break;
                }
            }            
            
            if (pPIDs[i] != 0xFFFF)
            {                    
                GL861_WARNING("%s : %d : %s : out of pid entries ???\n",__FILE__,__LINE__,__FUNCTION__);
                break;
            }            
        }         
    }               
    
    /* for debug
    GL861_DBG("new mask = %02x%02x%02x%02x 2 \n",            
        mask[3][1],
        mask[2][1],
        mask[1][1],
        mask[0][1]    
    );  
    */
    
    for (i=0; i<4; i++)
    {       
        if (mask[i][1] != gl861_pid.mask[i])
        {
            if (_gl861_register_write(dib, GL861_I2C_ADDR ,mask[i][0], &mask[i][1], 1)<0)
                goto err_set_gl861_reg_fail;
            gl861_pid.mask[i] = mask[i][1];   // update mask status
        }
    }
    
    
    // 3) set Pass Mode if necessary
    val = (PIDSetPass) ? 0x80 : 0x00;                           
    if (_gl861_register_write(dib, GL861_I2C_ADDR , GL861_DTV_MASK_EN, &val, 1)<0)
        goto err_set_gl861_reg_fail;                                                                             
        
    return 0;        
            
//---
err_set_gl861_reg_fail:
    GL861_WARNING("%s : %d : %s : gl861 register access fail\n",__FILE__,__LINE__,__FUNCTION__);
    return -1;
}

#endif



/*=======================================================
 * Func : gl861_flush_on 
 *
 * Desc : flush gl861 TX Fifo
 *
 * Parm : dib   : handle of usb_dibusb       
 *       
 * Retn : 0/1
 *=======================================================*/ 
int _gl861_flush_on(struct usb_dibusb* dib)
{    
    int ret;
    unsigned char val;
    
    GL861_DBG("%s : %d : %s \n",__FILE__,__LINE__,__FUNCTION__);      
     
    ret = _gl861_register_read(
                dib,                
                0x0000,                    // demod address
                0x54,                      // offset address
                &val,                      // data buffer
                1);                        // data length
    
    
    if (ret <0){
        GL861_DBG("%s : %d : %s : read register 1 fail\n",__FILE__,__LINE__,__FUNCTION__);
        return ret;
    }      
    
    val |= 0x08;        // Set TX FF RST 
    
    ret = _gl861_register_write(
                dib,                
                0x0000,                    // demod address
                0x54,                      // offset address
                &val,                      // data buffer
                1);                        // data length
    
    if (ret <0){
        GL861_DBG("%s : %d : %s : write register 1 fail\n",__FILE__,__LINE__,__FUNCTION__);
        return ret;
    }      
    
    return 0;
    
}

