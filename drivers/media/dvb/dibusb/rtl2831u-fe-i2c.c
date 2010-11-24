/*
 * rl2831u-fe-i2c.c is part of the driver for mobile USB Budget DVB-T devices
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
#include "rtl2831u-fe-i2c.h" 


//--------------- internal used function 
int _rtl2831u_demod_fifo_reset(struct usb_dibusb* dib);
int _rtl2831u_streaming_on(struct usb_dibusb* dib, int on_off);

int _rtl2831u_usb_register_read(struct usb_dibusb *dib,unsigned short offset, unsigned char *data, unsigned short bytelength);
int _rtl2831u_usb_register_write(struct usb_dibusb *dib,unsigned short offset, unsigned char *data, unsigned short bytelength);
int _rtl2831u_sys_register_read(struct usb_dibusb *dib,unsigned short offset, unsigned char *data, unsigned short bytelength);
int _rtl2831u_sys_register_write(struct usb_dibusb *dib,unsigned short offset, unsigned char *data, unsigned short bytelength);
int _rtl2831u_demod_register_read(struct usb_dibusb *dib,unsigned char page, unsigned char offset, unsigned char *data, unsigned short bytelength);
int _rtl2831u_demod_register_write(struct usb_dibusb *dib,unsigned char page, unsigned char offset, unsigned char *data, unsigned short bytelength);
int _rtl2831u_tuner_register_read(struct usb_dibusb *dib,unsigned char device_address, unsigned char offset, unsigned char *data, unsigned short bytelength);
int _rtl2831u_tuner_register_write(struct usb_dibusb *dib,unsigned char device_address, unsigned char offset, unsigned char *data, unsigned short bytelength);                
//---------------------------------------------------------------


struct i2c_algorithm rtl2831u_algo = 
{
	.name			= "RTL2831U USB i2c algorithm",
	.id				= I2C_ALGO_BIT,
	.master_xfer	= rtl2831u_i2c_xfer,
	.functionality	= rtl2831u_i2c_func,
};


 

/*=======================================================
 * Func : rtl2831u_i2c_xfer 
 *
 * Desc : i2c transfer function for rtl2831u 
 *
 * Parm : adap : handle of i2c adapter
 *        msg  : i2c messages
 *        num  : number of messages
 *
 * Retn : 0 success / otehrs fail
 *=======================================================*/ 
int rtl2831u_i2c_xfer(
    struct i2c_adapter*         adap,
    struct i2c_msg*             msg,
    int                         num
    )
{
	struct usb_dibusb *dib = i2c_get_adapdata(adap);
	int i;	
	
#ifdef FIX_USB_1K_BOUNDRARY_LIMIT                     
    // 64B Data Buffer, no cross 1K boundrary
    unsigned char buffer[128];
    unsigned char *data = buffer;
    if (((unsigned long)data & 0x3FF) > 960)
        data = (unsigned char*) (((unsigned long) data + 0x400) & (~0x3FF));                
#endif                     
        
	if (down_interruptible(&dib->i2c_sem) < 0)
		return -EAGAIN;
        
	if (num > 2)
		RTL2831U_WARNING("more than 2 i2c messages at a time is not handled yet. TODO.");		
        
    if(msg[0].addr==dib->dibdev->dev_cl->demod->i2c_addrs[0])//demod register access
    {
        unsigned short offset;
        unsigned char *pBuf0=msg[0].buf; 
        offset=(pBuf0[2]<<8)|pBuf0[1];
    
        if(*pBuf0==RTD2831U_USB)
        {
            if(num>1)//read
                _rtl2831u_usb_register_read(dib,offset,msg[1].buf,1);
            else//write
                _rtl2831u_usb_register_write(dib,offset,pBuf0+3,1);
        }
        else if(*pBuf0==RTD2831U_SYS)
        {
            if(num>1)//read
                _rtl2831u_sys_register_read(dib,offset,msg[1].buf,1);
            else//write
                _rtl2831u_sys_register_write(dib,offset,pBuf0+3,1);
        }
        else
        {
            if(num>1)//read
                _rtl2831u_demod_register_read(dib,pBuf0[0],offset,msg[1].buf,1);
            else//write
                _rtl2831u_demod_register_write(dib,pBuf0[0],offset,pBuf0+3,1);
        }
    }			
    else if(msg[0].addr==dib->tuner->pll_addr)//demod register access
    {                
        unsigned char *pBuf0=msg[0].buf;     
        unsigned char i2crepeat;         
        
        //rtd2830_repeater(fe, 1);/////////////////////////////
        _rtl2831u_demod_register_read(dib,1,1,&i2crepeat,1);
        i2crepeat |= 8;
        _rtl2831u_demod_register_write(dib,1,1,&i2crepeat,1);
        //////////////////////////////////////////////////////	
        if(msg[0].flags==0xffff)
        {        	
#ifndef FIX_USB_1K_BOUNDRARY_LIMIT        
            unsigned char I2Cdata[4];
#else                       
            unsigned char *I2Cdata = data;     // using 64B buffer to instead       
#endif   
        	
        	I2Cdata[0]=pBuf0[0];
        	I2Cdata[1]=pBuf0[1];
        	I2Cdata[2]=pBuf0[2];//o_ctrB
        	I2Cdata[3]=pBuf0[3];//o_Bandswitch
            _rtl2831u_tuner_register_write(dib,(msg[0].addr<<1),I2Cdata[0], &I2Cdata[1], 3);
                        
            //    demod_repeat(1);
            //rtd2830_repeater(fe, 1);/////////////////////////////
            _rtl2831u_demod_register_read(dib,1,1,&i2crepeat,1);
            i2crepeat |= 8;
            _rtl2831u_demod_register_write(dib,1,1,&i2crepeat,1);
            //////////////////////////////////////////////////////	
            //I2Cdata[2]=0x9C;
            I2Cdata[2] &= 0xc7;
            I2Cdata[2] |= 0x18;
            
            I2Cdata[3]=pBuf0[4];//o_Aux       
            _rtl2831u_tuner_register_write(dib,(msg[0].addr<<1),I2Cdata[0], &I2Cdata[1], 3);
 
 
 
            mdelay(100);
            //    demod_repeat(1);
            //rtd2830_repeater(fe, 1);/////////////////////////////
            _rtl2831u_demod_register_read(dib,1,1,&i2crepeat,1);
            i2crepeat |= 8;
            _rtl2831u_demod_register_write(dib,1,1,&i2crepeat,1);
            //////////////////////////////////////////////////////	
            //I2Cdata[2]=0x9C;
            I2Cdata[3]=pBuf0[4]&0x7f;
            _rtl2831u_tuner_register_write(dib,(msg[0].addr<<1),I2Cdata[0], &I2Cdata[1], 3);
            //    demod_repeat(1);
            //rtd2830_repeater(fe, 1);/////////////////////////////
            _rtl2831u_demod_register_read(dib,1,1,&i2crepeat,1);
            i2crepeat |= 8;
            _rtl2831u_demod_register_write(dib,1,1,&i2crepeat,1);
            //////////////////////////////////////////////////////	
        	I2Cdata[0]=0x03;
        	if(pBuf0[5]==BANDWIDTH_8_MHZ)
                I2Cdata[1]=0x0C;
        	else//BANDWIDTH_6_MHZ,BANDWIDTH_7_MHZ
                I2Cdata[1]=0x08;
      
            _rtl2831u_tuner_register_write(dib,TUNER_I2C_ADDRESS_Thomson_if,I2Cdata[0], &I2Cdata[1], 1);
	        
	        //rtd2830_repeater(fe, 1);/////////////////////////////
            _rtl2831u_demod_register_read(dib,1,1,&i2crepeat,1);
            i2crepeat |= 8;
            _rtl2831u_demod_register_write(dib,1,1,&i2crepeat,1);
            //////////////////////////////////////////////////////	
        
            RTL2831U_DBG("Set Tuner Gain to Low\n");
            I2Cdata[0]=0x10;        // register address 0x1001
            I2Cdata[1]=0x10; 
            I2Cdata[2]=0x01;
            I2Cdata[3]=0x01;        // register value
            I2Cdata[4]=0xe1; 
            _rtl2831u_tuner_register_write(dib,TUNER_I2C_ADDRESS_Thomson_if,I2Cdata[0], &I2Cdata[1], 4);
        
	    }
        else//msg[0].flags=0
        {                        
            if(num>1)   //read           
                _rtl2831u_tuner_register_read(dib,(msg[0].addr<<1),pBuf0[0],msg[1].buf, 1);                        
            else        //write                
                _rtl2831u_tuner_register_write(dib,(msg[0].addr<<1),pBuf0[0], pBuf0+1, 1);	             
        }
    }
    else if(msg[0].flags==0xffff)//set PID filter
    {
        unsigned short length;
        unsigned short *ptemp;
        unsigned char i;
        unsigned char *pBuf0=msg[0].buf;
        unsigned long dwDataLength;
        
#ifndef FIX_USB_1K_BOUNDRARY_LIMIT        
        unsigned char data[64];                   
#endif                
                        
        length=1;
        _rtl2831u_demod_register_read(dib,0, 0xa6, data, length);
        RTL2831U_DBG("read PID filter clock 0xA6= 0x%x\n",data[0]); 
        
        _rtl2831u_demod_register_read(dib,1, 0x7d, data, length);
        RTL2831U_DBG("read Demod clock 0x7D= 0x%x\n",data[0]); 
        
        //the clock need the same between PIDfilter and demod.
        //	data[0]=0x99;//0x45
        length = 1;
        _rtl2831u_demod_register_write(dib,0, 0xA6, data, length);
        
        RTL2831U_DBG("write 0xA6= 0x%x\n",data[0]); 
        
        data[0]=0xff;
        length = 1; 
        
        _rtl2831u_demod_register_read(dib,0, 0xa6, data, length);
        
        RTL2831U_DBG("read 0xA6= 0x%x\n",data[0]); 	
                        
        memset(data,0,sizeof(data));
        
        dwDataLength=pBuf0[1];      // number of PID Filters
        
        RTL2831U_DBG("%s: %d: %u pid filter\n", __FUNCTION__, __LINE__, (unsigned int)dwDataLength);
        
        if(dwDataLength==0xff)//disable PIDfilter
        {
            data[0]=0x00; 	
            _rtl2831u_demod_register_write(dib,0, 0x61, data,1);		
        }
        else
        {
            if(dwDataLength<=8)
            {
                data[0]=0x80; 	
                data[1]=0;
                for(i=0;i<dwDataLength;i++)
                    data[1] |= (1<<i);
            }
            else if(dwDataLength<=16)
            {
                length=dwDataLength%8;
                data[0]=0x80; 	
                data[1]=0xff;
                data[2]=0;
                for(i=0;i<length;i++)
                    data[2] |= (1<<i);
            }
            else if(dwDataLength<=24)
            {
                length=dwDataLength%8;
                data[0]=0x80; 	
                data[1]=0xff;
                data[2]=0xff;
                data[3]=0;
                for(i=0;i<length;i++)
                    data[3] |= (1<<i);
            }
            else if(dwDataLength<=32)
            {
                length=dwDataLength%8;
                data[0]=0x80; 	
                data[1]=0xff;
                data[2]=0xff;
                data[3]=0xff;
                data[4]=0;
                for(i=0;i<length;i++)
                    data[4] |= (1<<i);
            }
            else
            {
                RTL2831U_DBG("Error:The set PID set over 32 !\n");
            }
            _rtl2831u_demod_register_write(dib,0, 0x61, data,5);
        
            ptemp=(unsigned short*)(pBuf0+2);
                 
            for(i=0;i<dwDataLength;i++)
            {
                RTL2831U_DBG("PID filter=%d\n",*(ptemp+i));
                data[i*2+1] = (*(ptemp+i)) & 0xff;
                data[i*2] = (*(ptemp+i))>>8 ;
            }
            
            length = dwDataLength*2;
        
            if (length){ // Richard add
                _rtl2831u_demod_register_write(dib,0, 0x66, data, 64);                
            }
        
            RTL2831U_DBG("set PID filter complete\n");                
        }      	
    }	
		
    if (num>1)
		i=2;
	else
		i=1;	  
				    
	up(&dib->i2c_sem);
	return i;
}


/*=======================================================
 * Func : rtl2831u_i2c_func 
 *
 * Desc : i2c function for rtl2831u 
 *
 * Parm : adap : handle of i2c adapter
 *
 * Retn : 0 success / otehrs fail
 *=======================================================*/ 
u32 rtl2831u_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C;
}




/*=======================================================
 * Func : rtl2831u_tuner_quirk 
 *
 * Desc : i2c tuner quirk function
 *
 * Parm : dib : handle dibusb
 *
 * Retn : 0
 *=======================================================*/ 
int rtl2831u_tuner_quirk(struct usb_dibusb *dib)
{
    int     i;
    int     tuner_count;
    struct  dibusb_tuner *pTuner = dib->tuner;	
	unsigned char   data[4];
    unsigned char   i2crepeat;        
	struct i2c_msg  msg[2];		
	unsigned char check_list[3][3] = 
	{
        {   DIBUSB_TUNER_COFDM_QT1010, 
            0x0F,
            0x2C
        },
        { 
            DIBUSB_TUNER_COFDM_MICROTUNE_MT2060,
            0x00,
            0x63
        },            
        {   DIBUSB_TUNER_COFDM_THOMSON_FE664, 
            0x00,
            0x00,
        },            
    };

	RTL2831U_DBG("Run rtl2831u_tuner_quirk\n");	    
    
    //========================================
	// [1] Set GPIO Direction
	// [2] Set GPIO Enable
	// [3] Set GPIO Value
	//========================================         
    data[0] = 0x0a;               
    _rtl2831u_sys_register_write(dib,GPD, data, 1);
        
    data[0] = 0x15;    
    _rtl2831u_sys_register_write(dib,GPOE, data, 1);        
                
    _rtl2831u_sys_register_write(dib,GPO, data, 1);	
	data[0] |= 0x01;	
	_rtl2831u_sys_register_write(dib,GPO, data, 1);	
    	
	_rtl2831u_sys_register_write(dib,GPO, data, 1);	
	data[0] |= 0x04;	
	_rtl2831u_sys_register_write(dib,GPO, data, 1);	
    
	//========================================
	//[1]Enable demod pll/ADC/HW Reset enbale
	//[2]open demod adc enbale
	//[3]change the add of internal demod module
	//========================================
	_rtl2831u_sys_register_read(dib,DEMOD_CTL, data, 4);
	data[0] |= 0xe0;
	_rtl2831u_sys_register_write(dib,DEMOD_CTL, data, 4);		  

	///////////////////////////////////////	
	
	tuner_count = sizeof(check_list)/3;
	
	for (i=0;i<tuner_count;i++)
	{	    
	    pTuner       = &dibusb_tuner[check_list[i][0]]; 
	    
    	data[0]       = check_list[i][1];
    	msg[0].addr  = pTuner->pll_addr;
    	msg[0].flags = 0;
    	msg[0].buf   = &data[0];
    	msg[0].len   = 1;
    		
    	msg[1].addr  = pTuner->pll_addr;
    	msg[1].flags = I2C_M_RD;
    	msg[1].buf   = &data[1];
    	msg[1].len   = 1;    	    	
                
	    _rtl2831u_demod_register_read(dib,1,1,&i2crepeat,1);
        i2crepeat |= 8;
        _rtl2831u_demod_register_write(dib,1,1,&i2crepeat,1);
                        						                  
        if (_rtl2831u_tuner_register_read(
                dib,
                (pTuner->pll_addr)<<1,
                check_list[i][1],
                &data[1],1) == EFAULT )
            RTL2831U_DBG("%02x, %02x, Fail\n",pTuner->pll_addr,data[0]);
        else{
            RTL2831U_DBG("%02x, %02x, (%02x/%02x)\n",pTuner->pll_addr,data[0],data[1],check_list[i][2]);
            if (data[1] == check_list[i][2] || (check_list[i][1]==0 && check_list[i][2]==0))
                break;                     
        }
    }		 
    
    switch (i){
    case 0:            
        RTL2831U_DBG("Run rtd2831u_tuner_quirk complete : Quantek QT1010\n");    
        dib->tuner = &dibusb_tuner[check_list[i][0]];    
        dib->fe->ext_flag = 0;
        break;
    case 1:
        RTL2831U_DBG("Run rtl2831u_tuner_quirk complete : MICROTUNE MT2060\n");                
        dib->tuner = &dibusb_tuner[check_list[i][0]];    
        dib->fe->ext_flag = 0;
        break;
    case 2:
        RTL2831U_DBG("Run rtl2831u_tuner_quirk complete : THOMSON FE664X\n");                
        dib->tuner = &dibusb_tuner[check_list[i][0]];    
        dib->fe->ext_flag = 1;  
        break;  
    default:
        RTL2831U_DBG("Run rtl2831u_tuner_quirk fail, using default : THOMSON FE664X \n");        
        dib->tuner = &dibusb_tuner[DIBUSB_TUNER_COFDM_THOMSON_FE664];    
        dib->fe->ext_flag = 1;
        break;            
    }                           
        
   //========================================
	// [1] Set GPIO Direction
	// [2] Set GPIO Enable
	// [3] Set GPIO Value
	//========================================         
	_rtl2831u_sys_register_write(dib,GPO, data, 1);	
	data[0] &= ~0x05;	
	_rtl2831u_sys_register_write(dib,GPO, data, 1);	
    		            
    data[0] = 0x00;    
    _rtl2831u_sys_register_write(dib,GPOE, data, 1);        
                    
	///////////////////////////////////////	        
    return 0;           		                    
}



/*=======================================================
 * Func : rtl2831u_streaming 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int rtl2831u_streaming(void *dib,int onoff)
{   
    int ret; 
    
    if (onoff)
    {
        RTL2831U_DBG("%s : Streaming On \n",__FUNCTION__);
        
        _rtl2831u_demod_fifo_reset(dib);         // reset fifo
        _rtl2831u_streaming_on(dib,1);           // enable rtl2831u dma        
        ret = dibusb_streaming((struct usb_dibusb*) dib,1);
    }
    else 
    {     
        RTL2831U_DBG("%s : Streaming Off \n",__FUNCTION__);
        
        ret = dibusb_streaming((struct usb_dibusb*) dib,0);
        _rtl2831u_streaming_on(dib,0);           // disable rtl2831u dma
        _rtl2831u_demod_fifo_reset(dib);         // reset fifo
    }        
    
    return ret;
}




/*=======================================================
 * Func : _rtl2831u_demod_fifo_reset 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int _rtl2831u_demod_fifo_reset(
    struct usb_dibusb*  dib
    )
{
	unsigned char data[4];        
	
    //RTL2831U_DBG("%s \n",__FUNCTION__);
	
    if (_rtl2831u_usb_register_read(dib,0x0148, data, 4)<0){
        RTL2831U_DBG("%s : read EPA CTRL register fail\n",__FUNCTION__);
        return -EFAULT;
    }
    
    data[0] |= 0x20;
    data[1] |= 0x02;
    
    if (_rtl2831u_usb_register_write(dib,0x0148, data, 4)<0){
        RTL2831U_DBG("%s : set fifo reset fail\n",__FUNCTION__);
        return -EFAULT;
    }

    data[0] &= 0xDF;    
    data[1] &= 0xFD;    
    if (_rtl2831u_usb_register_write(dib, 0x0148, data,4)<0){
        RTL2831U_DBG("%s : reset fifo reset fail\n",__FUNCTION__);
        return -EFAULT;
    }
    
    return 0;
}


/*=======================================================
 * Func : _rtl2831u_streaming_on 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int _rtl2831u_streaming_on(
    struct usb_dibusb*          dib,
    int                         on_off
    )
{
    
    unsigned char data[4];        
	
    //RTL2831U_DBG("%s \n",__FUNCTION__);
	
    if (_rtl2831u_usb_register_read(dib,0x0000, data, 4)<0){
        RTL2831U_DBG("%s : read USB_SYSCTRL register fail\n",__FUNCTION__);
        return -EFAULT;
    }
    
    if (((data[0] & 0x01) && (on_off))||(!(data[0] & 0x01) && !(on_off)))
        return 0;
        
    if (on_off)
        data[0] |= 0x01;
    else
        data[0] &= ~0x01;
    
    if (_rtl2831u_usb_register_write(dib,0x0000, data, 4)<0){
        RTL2831U_DBG("%s : set USB_SYSCTRL fail\n",__FUNCTION__);
        return -EFAULT;
    }
    
    return 0;        
}





/*=======================================================
 * Func : _rtl2831u_usb_register_read 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int _rtl2831u_usb_register_read(
    struct usb_dibusb*          dib,
    unsigned short              offset, 
    unsigned char*              data, 
    unsigned short              bytelength
    )
{
	int ret = -ENOMEM;

	if ((ret = down_interruptible(&dib->usb_sem)))
		return ret;

	debug_dump(wbuf,wlen);
 
    ret = usb_control_msg(
            dib->udev,                  /* pointer to device */
            usb_rcvctrlpipe( dib->udev,dib->dibdev->dev_cl->pipe_cmd),   /* pipe to control endpoint */
            0,                          /* USB message request value */
            SKEL_VENDOR_IN,             /* USB message request type value */
            (USB_BASE_ADDRESS<<8) + offset,            /* USB message value */
            0x0100,                     /* USB message index value */
            data,                       /* pointer to the receive buffer */
            bytelength,                 /* length of the buffer */
            DIBUSB_I2C_TIMEOUT);        /* time to wait for the message to complete before timing out */
              
    if (ret == bytelength)	
		debug_dump(data,bytelength);    
    else
    {
        RTL2831U_DBG(" size != %d bytes\n", ret);
        ret = -EFAULT;    
    }    

	up(&dib->usb_sem);
	return ret; 
}


/*=======================================================
 * Func : _rtl2831u_usb_register_write 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int _rtl2831u_usb_register_write(
    struct usb_dibusb*              dib,
    unsigned short                  offset, 
    unsigned char*                  data, 
    unsigned short                  bytelength
    )
{
	int ret = -ENOMEM;

	if ((ret = down_interruptible(&dib->usb_sem)))
		return ret;

    debug_dump(wbuf,wlen);
    
    ret = usb_control_msg(
            dib->udev,              /* pointer to device */
            usb_sndctrlpipe(dib->udev,dib->dibdev->dev_cl->pipe_cmd),   /* pipe to control endpoint */
            0,                      /* USB message request value */
            SKEL_VENDOR_OUT,        /* USB message request type value */
            (USB_BASE_ADDRESS<<8) + offset,            /* USB message value */
            0x0110,                 /* USB message index value */
            data,                   /* pointer to the receive buffer */
            bytelength,             /* length of the buffer */
            DIBUSB_I2C_TIMEOUT);    /* time to wait for the message to complete before timing out */
    	
    if (ret != bytelength)			  
    {
        RTL2831U_DBG(" size != %d bytes\n", ret);
        ret = -EFAULT;    
    }    
    
    up(&dib->usb_sem);
    return ret;
 }



/*=======================================================
 * Func : _rtl2831u_sys_register_read 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int _rtl2831u_sys_register_read(
    struct usb_dibusb*              dib,
    unsigned short                  offset, 
    unsigned char*                  data, 
    unsigned short                  bytelength
    )
{
	int ret = -ENOMEM;

	if ((ret = down_interruptible(&dib->usb_sem)))
		return ret;

	debug_dump(wbuf,wlen);		
 
    ret = usb_control_msg(dib->udev,    /* pointer to device */
            usb_rcvctrlpipe( dib->udev,dib->dibdev->dev_cl->pipe_cmd),   /* pipe to control endpoint */
            0,                          /* USB message request value */
            SKEL_VENDOR_IN,             /* USB message request type value */
            (SYS_BASE_ADDRESS<<8) + offset,            /* USB message value */
            0x0200,                     /* USB message index value */
            data,                       /* pointer to the receive buffer */
            bytelength,                 /* length of the buffer */
            DIBUSB_I2C_TIMEOUT);        /* time to wait for the message to complete before timing out */
		
    if (ret == bytelength)	
		debug_dump(data,bytelength);    
    else
    {
        RTL2831U_DBG(" size != %d bytes\n", ret);
        ret = -EFAULT;    
    }    

	up(&dib->usb_sem);
	return ret; 
}




/*=======================================================
 * Func : _rtl2831u_sys_register_write 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int _rtl2831u_sys_register_write(
    struct usb_dibusb*              dib,
    unsigned short                  offset, 
    unsigned char*                  data, 
    unsigned short                  bytelength
    )
{ 
	int ret = -ENOMEM;

	if ((ret = down_interruptible(&dib->usb_sem)))
		return ret;

    ret = usb_control_msg(
            dib->udev,                  /* pointer to device */
            usb_sndctrlpipe( dib->udev,dib->dibdev->dev_cl->pipe_cmd),   /* pipe to control endpoint */
            0,                          /* USB message request value */
            SKEL_VENDOR_OUT,            /* USB message request type value */
            (SYS_BASE_ADDRESS<<8) + offset,            /* USB message value */
            0x0210,                     /* USB message index value */
            data,                       /* pointer to the receive buffer */
            bytelength,                 /* length of the buffer */
            DIBUSB_I2C_TIMEOUT);        /* time to wait for the message to complete before timing out */
                
    if (ret != bytelength)			  
    {
        RTL2831U_DBG(" size != %d bytes\n", ret);
        ret = -EFAULT;    
    }    

	up(&dib->usb_sem);
	return ret;
}





/*=======================================================
 * Func : _rtl2831u_demod_register_read 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int _rtl2831u_demod_register_read(
    struct usb_dibusb*                  dib,
    unsigned char                       page, 
    unsigned char                       offset, 
    unsigned char*                      data, 
    unsigned short                      bytelength
    )
{
	int ret = -ENOMEM;

	if ((ret = down_interruptible(&dib->usb_sem)))
		return ret;

	debug_dump(wbuf,wlen);
    
    ret = usb_control_msg(
        dib->udev,                          /* pointer to device */
        usb_rcvctrlpipe( dib->udev,dib->dibdev->dev_cl->pipe_cmd),   /* pipe to control endpoint */
        0,                                  /* USB message request value */
        SKEL_VENDOR_IN,                     /* USB message request type value */
        DEMOD_I2C_ADDRESS + (offset<<8),    /* USB message value */
        (0x0000 + page),                    /* USB message index value */
        data,                               /* pointer to the receive buffer */
        bytelength,                         /* length of the buffer */
        DIBUSB_I2C_TIMEOUT);                /* time to wait for the message to complete before timing out */

    if (ret == bytelength)	
		debug_dump(data,bytelength);    
    else
    {
        RTL2831U_DBG(" size != %d bytes\n", ret);
        ret = -EFAULT;    
    }    

	up(&dib->usb_sem);
	return ret;  
}



/*=======================================================
 * Func : _rtl2831u_demod_register_write 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int _rtl2831u_demod_register_write(
    struct usb_dibusb*                  dib,
    unsigned char                       page, 
    unsigned char                       offset, 
    unsigned char*                      data, 
    unsigned short                      bytelength
    )
{
    int ret = -ENOMEM;
    
    if ((ret = down_interruptible(&dib->usb_sem)))
        return ret;
    
    debug_dump(wbuf,wlen);
    
    ret = usb_control_msg(
        dib->udev,           /* pointer to device */
        usb_sndctrlpipe( dib->udev,dib->dibdev->dev_cl->pipe_cmd),   /* pipe to control endpoint */
        0,                              /* USB message request value */
        SKEL_VENDOR_OUT,                /* USB message request type value */
        DEMOD_I2C_ADDRESS + (offset<<8),/* USB message value */
        (0x0010 + page),                /* USB message index value */
        data,                           /* pointer to the receive buffer */
        bytelength,                     /* length of the buffer */
        DIBUSB_I2C_TIMEOUT);            /* time to wait for the message to complete before timing out */
    
    if (ret != bytelength)    
    {
        RTL2831U_DBG(" size != %d bytes\n", ret);
        ret = -EFAULT;    
    }
    
    up(&dib->usb_sem);
    return ret;
}
 
 

/*=======================================================
 * Func : _rtl2831u_tuner_register_read 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/ 
int _rtl2831u_tuner_register_read(
    struct usb_dibusb*                  dib,
    unsigned char                       device_address, 
    unsigned char                       offset, 
    unsigned char*                      data, 
    unsigned short                      bytelength
    )
{
	int ret = -ENOMEM;

	if ((ret = down_interruptible(&dib->usb_sem)))
		return ret;

	debug_dump(wbuf,wlen); 
 
    ret = usb_control_msg(dib->udev,           /* pointer to device */
    usb_rcvctrlpipe( 
        dib->udev,dib->dibdev->dev_cl->pipe_cmd),   /* pipe to control endpoint */
        0,                          /* USB message request value */
        SKEL_VENDOR_IN,             /* USB message request type value */
        device_address+(offset<<8), /* USB message value */
        0x0300,                     /* USB message index value */
        data,                       /* pointer to the receive buffer */
        bytelength,                 /* length of the buffer */
        DIBUSB_I2C_TIMEOUT);        /* time to wait for the message to complete before timing out */            
        
    if (ret == bytelength)        
        debug_dump(data,bytelength);    
	else {
        RTL2831U_DBG(" size != %d bytes\n", ret);
        ret =  -EFAULT;
    }
    
	up(&dib->usb_sem);
	return ret;    
}



/*=======================================================
 * Func : _rtl2831u_tuner_register_write 
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0
 *=======================================================*/
int _rtl2831u_tuner_register_write(
    struct usb_dibusb*                  dib,
    unsigned char                       device_address, 
    unsigned char                       offset, 
    unsigned char*                      data, 
    unsigned short                      bytelength
    )
{
	int ret = -ENOMEM;

	if ((ret = down_interruptible(&dib->usb_sem)))
		return ret;

	debug_dump(wbuf,wlen);        
	
#if 1
    RTL2831U_DBG("write tuner : %02X %02X", device_address, offset);    
    for (ret=0; ret < bytelength; ret++)
        RTL2831U_DBG(" %02X",data[ret]);
    RTL2831U_DBG("\n");        
#endif	
    
    ret = usb_control_msg(
            dib->udev,          /* pointer to device */
            usb_sndctrlpipe(dib->udev,dib->dibdev->dev_cl->pipe_cmd),   /* pipe to control endpoint */
            0,                  /* USB message request value */
            SKEL_VENDOR_OUT,    /* USB message request type value */
            device_address+(offset<<8),            /* USB message value */
            0x0310,             /* USB message index value */
            data,               /* pointer to the receive buffer */
            bytelength,         /* length of the buffer */
            DIBUSB_I2C_TIMEOUT);                            /* time to wait for the message to complete before timing out */

    if (ret != bytelength)
    {            
        RTL2831U_DBG(" size != %d bytes\n", ret);
        ret = -EFAULT;
    }

	up(&dib->usb_sem);
	return ret;
}
