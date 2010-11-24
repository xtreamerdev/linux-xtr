/* ------------------------------------------------------------------------- 
   scd_mars.c  scd driver for Realtek Neptune/Mars           
   ------------------------------------------------------------------------- 
    Copyright (C) 2008 Kevin Wang <kevin_wang@realtek.com.tw>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    
----------------------------------------------------------------------------
Update List :
----------------------------------------------------------------------------
    1.0     |   20090312    |   Kevin Wang  | 1) create phase
----------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/io.h>
#include "scd_mars_priv.h"
#include "scd_mars_reg.h"

MODULE_LICENSE("GPL");

/* Basic operation */
#define rtd_inb(offset)             (*(volatile unsigned char  *)(offset))
#define rtd_inw(offset)             (*(volatile unsigned short *)(offset))
#define rtd_inl(offset)             (*(volatile unsigned long  *)(offset))
#define rtd_outb(offset,val)        (*(volatile unsigned char  *)(offset) = val)
#define rtd_outw(offset,val)        (*(volatile unsigned short *)(offset) = val)
#define rtd_outl(offset,val)        (*(volatile unsigned long  *)(offset) = val)

#define DBG_CHAR(x)                 rtd_outl(0xb801b200, x)


static const unsigned long          SC_BASE[]={SC_BASE0, SC_BASE1};
static const unsigned long          MIS_SC_ISR[]={MIS_SC0_INT, MIS_SC1_INT};

#define SET_MIS_ISR(val)            rtd_outl(MIS_ISR, val)
#define SET_SCFP(id,   val)         rtd_outl(SC_BASE[id] + SCFP,    val)
#define SET_SCCR(id,   val)         rtd_outl(SC_BASE[id] + SCCR,    val)
#define SET_SCPCR(id,  val)         rtd_outl(SC_BASE[id] + SCPCR,   val)
#define SET_SC_TXFIFO(id, val)      rtd_outl(SC_BASE[id] + SCTXFIFO,val)
#define SET_SCFCR(id,  val)         rtd_outl(SC_BASE[id] + SCFCR,   val)
#define SET_SCIRSR(id, val)         rtd_outl(SC_BASE[id] + SCIRSR,  val)
#define SET_SCIRER(id, val)         rtd_outl(SC_BASE[id] + SCIRER,  val)          


#define GET_MIS_ISR()               rtd_inl(MIS_ISR)
#define GET_SCFP(id)                rtd_inl(SC_BASE[id] + SCFP)
#define GET_SCCR(id)                rtd_inl(SC_BASE[id] + SCCR)
#define GET_SCPCR(id)               rtd_inl(SC_BASE[id] + SCPCR)
#define GET_SC_TXLENR(id)           rtd_inl(SC_BASE[id] + SCTXLENR)
#define GET_SC_RXFIFO(id)           rtd_inl(SC_BASE[id] + SCRXFIFO)
#define GET_SC_RXLENR(id)           rtd_inl(SC_BASE[id] + SCRXLENR)
#define GET_SCFCR(id)               rtd_inl(SC_BASE[id] + SCFCR)
#define GET_SCIRSR(id)              rtd_inl(SC_BASE[id] + SCIRSR)
#define GET_SCIRER(id)              rtd_inl(SC_BASE[id] + SCIRER)          

#define SYSTEM_CLK                  27000000
#define MAX_SC_CLK                   8500000
#define MIN_SC_CLK                   1000000
	
#define ISR_POLLING
	
#define ISR_POLLING_INTERVAL        (HZ)

static void mars_scd_timer(unsigned long arg);



/*------------------------------------------------------------------
 * Func : mars_scd_set_state
 *
 * Desc : receive data and wait atr
 *
 * Parm : p_this : handle of mars cec 
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
int mars_scd_set_state(
    mars_scd*               p_this, 
    IFD_FSM                 fsm
    )
{
    unsigned char id = p_this->id;
    
    if (p_this->fsm == fsm)
        return 0;
        
    switch(fsm)
    {    	
    case IFD_FSM_DISABLE:
    
        SC_INFO("SC%d - FSM = DISABLE\n", id);
        SET_SCCR  (id, SC_FIFO_RST(1) | SC_SCEN(0) | SC_AUTO_ATR(1) | SC_CONV(0) | SC_PS(p_this->parity));
        SET_SCIRER(id, 0);                  // Disable ISR
        SET_SCIRSR(id, 0xFFFFFFFF);
        p_this->atr.length = 0;               
        break;
        
    case IFD_FSM_IDEL:                        
    
        SC_INFO("SC%d - FSM = IDEL\n", id);
        //SET_SCCR  (id, SC_FIFO_RST(1) | SC_SCEN(0) | SC_AUTO_ATR(1) | SC_CONV(0) |SC_PS(p_this->parity));        
        SET_SCCR  (id, SC_FIFO_RST(0) | SC_SCEN(1) | SC_AUTO_ATR(1) | SC_CONV(0) |SC_PS(p_this->parity));
        SET_SCIRER(id, SC_CPRES_INT);
        SET_SCIRSR(id, 0xFFFFFFFF);
        p_this->atr.length = 0;
        break;
        
    case IFD_FSM_RESET:
                            
        p_this->atr.length = 0;
                                    
        if (!mars_scd_card_detect(p_this))
        {
            SC_WARNING("SC%d - RESET mars scd failed, no ICC exist\n", id);                            
            mars_scd_set_state(p_this, IFD_FSM_IDEL);
            return -1;
        }
        
        SET_SCIRER(id, SC_CPRES_INT    | 
                       SC_ATRS_INT     | 
                       SC_RXP_INT      | 
                       SC_RX_FOVER_INT);
                       
        SET_SCCR  (id, SC_FIFO_RST(1)  |        // pull reset pin
                       SC_RST(1)       | 
                       SC_SCEN(1)      | 
                       SC_AUTO_ATR(1)  | 
                       SC_CONV(0)      |
                       SC_PS(p_this->parity));                                                      
        
        p_this->atr_timeout = jiffies + HZ;                    
        
        SC_INFO("SC%d - FSM = RESET & ATR\n", id);
        
        break;
        
    case IFD_FSM_ACTIVE:    
        SC_INFO("SC%d - FSM = ACTIVATE\n", id);
        break;
    default:
    case IFD_FSM_UNKNOWN:
        break;                    
    }    
    
    p_this->fsm = fsm;             
    
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_fsm_deactivate
 *
 * Desc : 
 *
 * Parm : p_this : handle of mars cec 
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void mars_scd_fsm_reset(mars_scd* p_this)
{
    unsigned char id = p_this->id;
    unsigned long event  = GET_SCIRSR(id);    
    unsigned long rx_len = 0;
    unsigned long i      = 0;		
	    
    SC_INT_DGB("SC_RISR=%08lx\n", event);
    	        	    				    
    if ((event & SC_RXP_INT) || !(event & SC_PRES)|| time_after(jiffies, p_this->atr_timeout))
	    goto err_atr_failed;	    
    		
	if (event & SC_RCV_INT)  
	{	  	    	    	    	    
	    if (p_this->atr.length==0 && !(event & SC_ATRS_INT))
	    {	    	        				
	        // unwanted data, drop it
	        rx_len = GET_SC_RXLENR(id);
	        
            for (i=0; i<rx_len; i++)
                GET_SC_RXFIFO(id);

            goto end_proc;                
        }
            
        // receive atr :                
        while(GET_SC_RXLENR(id))
        {
            if (p_this->atr.length >= MAX_ATR_SIZE)
                goto err_atr_failed;
                                      
            p_this->atr.data[p_this->atr.length++] = GET_SC_RXFIFO(id);                        
        }        
        
        // check atr 
        if (is_atr_complete(&p_this->atr))
        {            
            SC_INFO("SC%d - Got ATR Completed\n", p_this->id);
            
            if (decompress_atr(&p_this->atr, &p_this->atr_info)<0)
                goto err_atr_failed;                                                           
                                 
            mars_scd_set_state(p_this, IFD_FSM_ACTIVE);       // jump to active state
        }
    }		
	
end_proc:
	SET_SCIRSR(id, event);
    return;	
	
err_atr_failed:

    if (!(event & SC_PRES))
    {
        SC_WARNING("SC%d - RESET ICC failed, no ICC detected\n", p_this->id);
	}    
	
	if (event & SC_RXP_INT)
    {
        SC_WARNING("SC%d - RESET ICC failed, RX Parity Error\n", p_this->id);
	}
	
    if (time_after(jiffies, p_this->atr_timeout))
    {        
	   SC_WARNING("SC%d - RESET ICC failed, timeout\n", p_this->id); 	    			
	}    
		        	                                            
    mars_scd_set_state(p_this, IFD_FSM_IDEL);
    goto end_proc;
}



/*------------------------------------------------------------------
 * Func : mars_scd_fsm_active
 *
 * Desc : 
 *
 * Parm : p_this : handle of mars cec 
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void mars_scd_fsm_active(mars_scd* p_this)
{
    unsigned char id = p_this->id;
    unsigned long status = GET_SCIRSR(id);
    
	if (mars_scd_card_detect(p_this)==0)
    {                		                    	    
		mars_scd_set_state(p_this, IFD_FSM_IDEL);	    
		goto end_proc;
	}
		
	//-----------------
	
end_proc:
	SET_SCIRSR(id, status);
}


/*------------------------------------------------------------------
 * Func : mars_scd_work
 *
 * Desc : mars scd work routine
 *
 * Parm : p_this : handle of mars cec 
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void mars_scd_work(mars_scd* p_this)
{
    unsigned char id = p_this->id;
    unsigned long status = GET_SCIRSR(id);

    SC_INFO("SC%d - work!!\n", p_this->id);    			
    
	if (status & SC_CPRES_INT)            
	{           
	    if (mars_scd_card_detect(p_this)) 
            SC_INFO("SC%d - ICC detected!!\n", p_this->id);    			
        else            
            SC_INFO("SC%d - ICC removed!!\n", p_this->id);    			
            
	    complete(&p_this->card_detect_completion);	
	}
		
    switch (p_this->fsm)
	{	    
	case IFD_FSM_RESET:    mars_scd_fsm_reset(p_this);  break;
	case IFD_FSM_ACTIVE:   mars_scd_fsm_active(p_this); break;
	default:
	    break;
    }	

	SET_SCIRSR(id, status);
}



#ifdef ISR_POLLING     
/*------------------------------------------------------------------
 * Func : mars_scd_timer
 *
 * Desc : timer of mars scd
 *
 * Parm : arg : input param 
 *         
 * Retn : IRQ_NONE, IRQ_HANDLED
 *------------------------------------------------------------------*/
static void mars_scd_timer(unsigned long arg)        
{   
    mars_scd* p_this = (mars_scd*) arg;
                   
    unsigned long event = GET_MIS_ISR() & MIS_SC_ISR[p_this->id];                        
    
//    SC_INFO("irq=%08x, sc_isr%d=%08x, ire=%08x, cr=%08x\n",GET_MIS_ISR(), p_this->id, GET_SCIRSR(p_this->id), GET_SCIRER(p_this->id), GET_SCCR(p_this->id));                
           
    if (event)
        mars_scd_work(p_this);
                
    SET_MIS_ISR(event);
        
    mod_timer(&p_this->timer, jiffies + ISR_POLLING_INTERVAL);
}

#else


/*------------------------------------------------------------------
 * Func : mars_scd_isr
 *
 * Desc : isr of mars scd
 *
 * Parm : p_this : handle of mars scd 
 *        dev_id : handle of the mars_scd
 *        regs   :
 *         
 * Retn : IRQ_NONE, IRQ_HANDLED
 *------------------------------------------------------------------*/
static irqreturn_t
mars_scd_isr(
    int                     this_irq, 
    void*                   dev_id, 
    struct pt_regs*         regs
    )
{   
    mars_scd* p_this = (mars_scd*) dev_id;
                   
    unsigned long event = GET_MIS_ISR() & MIS_SC_ISR[p_this->id];            

    SC_INT_DBG("MIS ISR=%08x\n",GET_MIS_ISR());            
    
    if (!event)
        return IRQ_NONE;
         
    mars_scd_work(p_this);        
                
    SET_MIS_ISR(event);
                      
    return IRQ_HANDLED;    
}


#endif




/*------------------------------------------------------------------
 * Func : mars_scd_enable
 *
 * Desc : enable mars smart card interface
 *
 * Parm : p_this   : handle of mars scd  
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/ 
int mars_scd_enable(mars_scd* p_this, unsigned char on_off)
{
    if (on_off)        
    {
        if (p_this->fsm==IFD_FSM_DISABLE)
            return mars_scd_set_state(p_this, IFD_FSM_IDEL);
    }
    else    
        return mars_scd_set_state(p_this, IFD_FSM_DISABLE);    
    
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_set_clock
 *
 * Desc : set clock frequency of mars smart card interface
 *
 * Parm : p_this   : handle of mars scd  
 *        clk      : clock (in HZ)
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/ 
int mars_scd_set_clock(mars_scd* p_this, unsigned long clk)
{
    unsigned char id = p_this->id;
    unsigned long val; 
    unsigned long div = SYSTEM_CLK / clk / p_this->clock_div;    
	    
    if (clk > MAX_SC_CLK)
    {
        SC_WARNING("clock %lu out of range, using minimum value to instead %lu\n", (unsigned long)clk, (unsigned long)MAX_SC_CLK);        
        clk = MAX_SC_CLK;
    }
        
    if (clk < MIN_SC_CLK)
    {
        SC_WARNING("clock %lu out of range, using minimum value to instead %lu\n", (unsigned long)clk, (unsigned long)MIN_SC_CLK);
        clk = MIN_SC_CLK;
    }
    
    p_this->pre_clock_div = div;    
    
    val  = GET_SCFP(id) & ~SC_PRE_CLKDIV_MASK;
    
    val |= SC_PRE_CLKDIV(p_this->pre_clock_div);
    
    SET_SCFP(id, val);
    
    return 0;
}


/*------------------------------------------------------------------
 * Func : mars_scd_get_clock
 *
 * Desc : get clock frequency of mars smart card interface
 *
 * Parm : p_this   : handle of mars scd  
 *        p_clock  : clock (in HZ)
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/ 
int mars_scd_get_clock(mars_scd* p_this, unsigned long* p_clock)
{    
    *p_clock = SYSTEM_CLK / (p_this->pre_clock_div) / (p_this->clock_div);    	        
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_set_etu
 *
 * Desc : set etu of mars smart card interface
 *
 * Parm : p_this   : handle of mars scd  
 *        etu      : cycles
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/ 
int mars_scd_set_etu(mars_scd* p_this, unsigned long etu)
{
    unsigned char id = p_this->id;   
    unsigned long val; 
               
    p_this->baud_div2 = 31;         // to simplify, we always set the baud div2 to 31
    p_this->baud_div1 = etu * p_this->clock_div / p_this->baud_div2;     
    val = GET_SCFP(id) & ~SC_BAUDDIV_MASK;    
        
    val |= SC_BAUDDIV1((p_this->baud_div1-1)) | SC_BAUDDIV2(0);    
    
    SC_INFO("ETU = %lu. Baud Div2=31, Div1 = %lu\n", etu, p_this->baud_div1);
    SET_SCFP(id, val);    
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_get_etu
 *
 * Desc : set etu of mars smart card interface
 *
 * Parm : p_this   : handle of mars scd  
 *        etu      : cycles
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/ 
int mars_scd_get_etu(mars_scd* p_this, unsigned long* p_etu)
{    
    *p_etu = (p_this->baud_div2 * p_this->baud_div1) / p_this->clock_div; 
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_set_parity
 *
 * Desc : set parity of mars smart card interface
 *
 * Parm : p_this   : handle of mars scd  
 *        parity   : 0 : off,  others on
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/ 
int mars_scd_set_parity(mars_scd* p_this, unsigned char parity)
{
    unsigned char id = p_this->id;   
    unsigned long val; 
               
    p_this->parity = (parity) ? 1 : 0;                                 
    
    if (p_this->fsm==IFD_FSM_DISABLE)
        val = SC_FIFO_RST(0) | SC_SCEN(1) | SC_AUTO_ATR(1) | SC_CONV(0);
    else
        val = SC_FIFO_RST(0) | SC_SCEN(1) | SC_AUTO_ATR(1) | SC_CONV(0);            
        
    SET_SCCR(id, val | SC_PS(p_this->parity));
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_get_parity
 *
 * Desc : get parity setting of mars smart card interface
 *
 * Parm : p_this   : handle of mars scd  
 *        p_parity   : 0 : off,  others on
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/ 
int mars_scd_get_parity(mars_scd* p_this, unsigned char* p_parity)
{    
    *p_parity = p_this->parity;
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_activate
 *
 * Desc : activate an ICC
 *
 * Parm : p_this   : handle of mars scd  
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/        
int mars_scd_activate(mars_scd* p_this)
{    
    switch(p_this->fsm)
    {
    case IFD_FSM_DISABLE:
        SC_WARNING("activate ICC failed, please enable IFD first\n");
        return -1;
        
    case IFD_FSM_IDEL:
    
        if (p_this->reset(p_this)==0)
        {
            SC_INFO("activate ICC success\n");
            return 0;
        }
        else
        {
            SC_WARNING("activate ICC failed\n");
            return -1;
        }
        break;            
        
    case IFD_FSM_RESET:
        SC_INFO("ICC has is reseting\n");
        return 0;
        
    case IFD_FSM_ACTIVE:
        SC_INFO("ICC has been activated already\n");
        return 0;
        
    default:        
        SC_WARNING("activate ICC failed, unknown state\n");
        return -1;
    } 
}



/*------------------------------------------------------------------
 * Func : mars_scd_deactivate
 *
 * Desc : deactivate an ICC
 *
 * Parm : p_this   : handle of mars scd  
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/        
int mars_scd_deactivate(mars_scd* p_this)
{    
    if (p_this->fsm != IFD_FSM_DISABLE)
    {
        mars_scd_set_state(p_this, IFD_FSM_DISABLE);
        mars_scd_set_state(p_this, IFD_FSM_IDEL);
    }
    return 0;
}




/*------------------------------------------------------------------
 * Func : mars_scd_reset
 *
 * Desc : reset mars smart card interface
 *
 * Parm : p_this   : handle of mars scd  
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/        
int mars_scd_reset(mars_scd* p_this)
{
    unsigned long t;
    
    if (mars_scd_set_state(p_this, IFD_FSM_RESET))
        return -1;
    
    t = jiffies + (HZ<<2);
    
    while(!time_after(jiffies,t) && p_this->fsm == IFD_FSM_RESET)    
        msleep(100);    
    
    if (p_this->fsm!=IFD_FSM_ACTIVE)
    {
        SC_WARNING("SC%d - Reset ICC failed\n", p_this->id);
        mars_scd_set_state(p_this, IFD_FSM_IDEL);
        return -1;     
    }
        
    SC_INFO("SC%d - Reset ICC Complete\n", p_this->id);
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_get_atr
 *
 * Desc : s mars smart card interface
 *
 * Parm : p_this   : handle of mars scd  
 *        p_atr    : atr output buffer
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/        
int mars_scd_get_atr(mars_scd* p_this, scd_atr* p_atr)
{        
    if (p_this->fsm != IFD_FSM_ACTIVE)
    {    
        p_atr->length = 0;  
        return -1;
    }
                
    p_atr->length = p_this->atr.length;
    memcpy(p_atr->data, p_this->atr.data, p_this->atr.length);    
    return 0;    
}


/*------------------------------------------------------------------
 * Func : mars_scd_card_detect
 *
 * Desc : get card status
 *
 * Parm : p_this   : handle of mars scd  
 *         
 * Retn : 0 : no icc exists, others : icc exists
 *------------------------------------------------------------------*/   
int mars_scd_card_detect(mars_scd* p_this)
{    			
    unsigned char id = p_this->id;
    return (GET_SCIRSR(id) & SC_PRES) ? 1 : 0;
}



/*------------------------------------------------------------------
 * Func : mars_scd_poll_card_status
 *
 * Desc : poll card status change
 *
 * Parm : p_this   : handle of mars scd  
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/   
int mars_scd_poll_card_status(mars_scd* p_this)
{       
    init_completion(&p_this->card_detect_completion);
    wait_for_completion(&p_this->card_detect_completion);        
    return mars_scd_card_detect(p_this);   	 
}



/*------------------------------------------------------------------
 * Func : mars_scd_xmit
 *
 * Desc : xmit data via smart card bus
 *
 * Parm : p_this   : handle of mars scd  
 *        scb      : data to be xmit
 *         
 * Retn : SC_SUCCESS / SC_FAIL
 *------------------------------------------------------------------*/        
int mars_scd_xmit(mars_scd* p_this, sc_buff* scb)
{    
    unsigned char id = p_this->id;    
    int i;
    		
	SET_SCCR(id, GET_SCCR(id) | SC_FIFO_RST(1));     // Reset RX FIFO		
		
    if (scb->len <= 32)		
    {
        // fill data to fifo
        for(i = 0; i < scb->len; i++)                
			SET_SC_TXFIFO(id, scb->data[i]);				        
		
		SC_WARNING("[SC%d] TX FIFO LEN = %ul\n", GET_SC_TXLENR(id));
		
		scb_pull(scb, scb->len);
				
		SET_SCCR(id, GET_SCCR(id) | SC_TX_GO(1));          // Start Xmit
    }
    else
    {    				
        SC_WARNING("[SC%d] not supports sending data more than 32 bytes\n", id);
        return -1;
        /*        
		i=0;
		isTX = 0;
		while (i < scb->len)
		{				
            i++;				
            rtd_outl(SC_BASE+SCTXFIFO, *(cardBuffer++));    // put data to tx fifo			
			
			if (i==32 && isTX == 0)  //TX GO	
			{ 
				//printk("TX GO at %d.\n", i);
				isTX = 1;
				rtd_outl(SC_BASE+SCCR,(SC_TX_GO | SC_ENABLE | (temp&0x0fffffff)));	
			}
		} 
		*/   	
    }	

	msleep(5);
	
	return 0;
}                        




/*------------------------------------------------------------------
 * Func : mars_scd_read
 *
 * Desc : read data via smart card bus
 *
 * Parm : p_this   : handle of mars scd   
 *         
 * Retn : sc_buff
 *------------------------------------------------------------------*/        
sc_buff* mars_scd_read(mars_scd* p_this)
{    
    unsigned char id = p_this->id;        
    sc_buff* scb = NULL;    
    int len, i;
            
    SC_WARNING("RX LEN = %lu\n", GET_SC_RXLENR(id));
          
    if ((len=GET_SC_RXLENR(id))>0)
    {
        scb = alloc_scb(len);
        if (!scb)
            SC_WARNING("read failed, alloc_scb failed\n");
                    
        for(i = 0; i < len; i++)
			scb->data[i] = (unsigned char) GET_SC_RXFIFO(id);
			
        scb_put(scb, len);
    }

    return scb;
}



/*------------------------------------------------------------------
 * Func : mars_scd_open
 *
 * Desc : open a mars scd device
 *
 * Parm : id  : channel id
 *         
 * Retn : handle of mars scd
 *------------------------------------------------------------------*/
mars_scd* mars_scd_open(unsigned char id)
{           
    mars_scd* p_this;
    
    if (id >= 2) return NULL;
        
#if 1              
    rtd_outl(0xb8000364, rtd_inl(0xb8000364) | 0x00003000); // Set Pin MUX        	 
#endif                     
        
    p_this = (mars_scd*) kmalloc(sizeof(mars_scd), GFP_KERNEL);    
    
    if (p_this)
    {
        memset(p_this, 0, sizeof(mars_scd));        
  
        p_this->id               = id;
        p_this->fsm              = IFD_FSM_UNKNOWN;
        p_this->atr.length       = 0;        
        init_completion(&p_this->card_detect_completion);                        
        
        // configurations ---
    	p_this->clock_div        = 1;         
    	p_this->pre_clock_div    = 8;
    	p_this->baud_div1        = 12;        // default etu = 372 = 31 * 12        
    	p_this->baud_div2        = 31;           
        p_this->parity           = 1;      
                
        // operations ---
        p_this->enable           = mars_scd_enable;                
        p_this->set_clock        = mars_scd_set_clock;
        p_this->get_clock        = mars_scd_get_clock;        
        p_this->set_etu          = mars_scd_set_etu;
        p_this->get_etu          = mars_scd_get_etu;        
        p_this->set_parity       = mars_scd_set_parity;                
        p_this->get_parity       = mars_scd_get_parity;        
        p_this->activate         = mars_scd_activate;
        p_this->deactivate       = mars_scd_deactivate;  
        p_this->reset            = mars_scd_reset;        
        p_this->get_atr          = mars_scd_get_atr;                
        p_this->xmit             = mars_scd_xmit;
        p_this->read             = mars_scd_read;             
        p_this->get_card_status  = mars_scd_card_detect;
        p_this->poll_card_status = mars_scd_poll_card_status;   	                			                                        
        
                        
        // Set All Register to the initial value         
        SET_SCFP  (id, SC_CLK_EN(1)    | 
                       SC_CLKDIV((p_this->clock_div-1))|                        
                       SC_BAUDDIV2(0)  |                    // fixed to 31
                       SC_BAUDDIV1((p_this->baud_div1-1))|                        
                       SC_PRE_CLKDIV((p_this->pre_clock_div-1)));        
                       
        SET_SCCR  (id, SC_FIFO_RST(1)  | 
                       SC_SCEN(1)      | 
                       SC_AUTO_ATR(1)  |
                       SC_CONV(0)      |
                       SC_PS(0));        
                       
        SET_SCPCR (id, SC_TXGRDT(0)    | 
                       SC_CWI(0)       | 
                       SC_BWI(0)       |  
                       SC_WWI(0)       | 
                       SC_BGT(0x16)    |  
                       SC_EDC_EN(0)    | 
                       SC_CRC(0)       |
                       SC_PROTOCOL_T(0)| 
                       SC_T0RTY(0)     |
                       SC_T0RTY_CNT(0) );        
                       
        SET_SCIRER(id, 0);                      
        
        SET_SCIRSR(id, 0xFFFFFFFF);                                                          
        
        // Set Interrupt or IRQ
        
#ifdef ISR_POLLING        
        init_timer(&p_this->timer);
        p_this->timer.data     = (unsigned long)p_this;
        p_this->timer.function = mars_scd_timer;
        p_this->timer.expires  = jiffies + ISR_POLLING_INTERVAL;
        add_timer(&p_this->timer);      // register timer
#else     
        SC_INFO("Request IRQ #%d\n", MISC_IRQ);	    	    
    
        if (request_irq(MISC_IRQ, mars_scd_isr, SA_INTERRUPT | SA_SHIRQ, "MARS SCD", p_this) < 0) 
        {
            SC_WARNING("scd : open mars scd failed, unable to request irq#%d\n", MISC_IRQ);	    		
            return -1;
        }        
#endif                
        
        mars_scd_set_state(p_this, IFD_FSM_DISABLE);
    }        

    return p_this;
}    


/*------------------------------------------------------------------
 * Func : mars_scd_close
 *
 * Desc : close a mars scd device
 *
 * Parm : p_this  : mars scd to be close
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void mars_scd_close(mars_scd* p_this)
{
#ifdef ISR_POLLING             
    del_timer(&p_this->timer);      // register timer
#else
    free_irq(MISC_IRQ ,p_this);    
#endif    

    mars_scd_set_state(p_this, IFD_FSM_DISABLE);    
    complete(&p_this->card_detect_completion);    
    kfree(p_this);        
}    

