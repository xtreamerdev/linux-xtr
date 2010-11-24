/* ------------------------------------------------------------------------- */
/* i2c-venus-priv.c  venus i2c hw driver for Realtek Venus DVR I2C           */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2008 Kevin Wang <kevin_wang@realtek.com.tw>

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
------------------------------------------------------------------------- 
Update List :
------------------------------------------------------------------------- 
    1.1     |   20080530    | Using for loop instead of while loop
-------------------------------------------------------------------------     
    1.1a    |   20080531    | Add 1 ms delay at the end of each xfer 
                            | for timing compatibility with old i2c driver
-------------------------------------------------------------------------     
    1.1b    |   20080531    | After Xfer complete, ISR will Disable All Interrupts 
                            | and Disable I2C first, then wakeup the caller   
-------------------------------------------------------------------------     
    1.1c    |   20080617    | Add API get_tx_abort_reason
-------------------------------------------------------------------------     
    1.1d    |   20080630    | Add I2C bus jammed recover feature.
            |               | send 10 bits clock with gpio 4 to recover bus jam problem.
-------------------------------------------------------------------------     
    1.1e    |   20080702    | Disable GP 4/5 interript during detect bus jam   
-------------------------------------------------------------------------     
    1.1f    |   20080707    | Only do bus jam detect/recover after timeout occurs
-------------------------------------------------------------------------     
    1.2     |   20080711    | modified to support mars i2c
-------------------------------------------------------------------------     
    1.2a    |   20080714    | modified the way of i2c/GPIO selection
-------------------------------------------------------------------------     
    1.3     |   20080729    | Support Non Stop Write Transfer 
-------------------------------------------------------------------------     
    1.3a    |   20080729    | Fix bug of non stop write transfer
            |               |    1) MSB first   
            |               |    2) no stop after write ack
-------------------------------------------------------------------------     
    1.3b    |   20080730    | Support non stop write in Mars            
            |               | Support bus jam recorver in Mars            
-------------------------------------------------------------------------     
    1.3c    |   20080807    | Support minimum delay feature      
-------------------------------------------------------------------------     
    1.4     |   20081016    | Mars I2C_1 Support
-------------------------------------------------------------------------     
    1.5     |   20090330    | Add Spin Lock to Protect venus_i2c data structure
-------------------------------------------------------------------------     
    1.6     |   20090407    | Add FIFO threshold to avoid timing issue caused 
            |               | by performance issue
-------------------------------------------------------------------------     
    1.7     |   20090423    | Add Suspen/Resume Feature       
-------------------------------------------------------------------------     
    2.0     |   20091019    | Add Set Speed feature
-------------------------------------------------------------------------     
    2.0a    |   20091020    | change speed of bus jam recover via spd argument
-------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/i2c.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include "i2c-venus-priv.h"
#include "i2c-venus-reg.h"


////////////////////////////////////////////////////////////////////
#define wr_reg(x,y)                     writel(y,(volatile unsigned int*)x)
#define rd_reg(x)                       readl((volatile unsigned int*)x)

#ifdef CONFIG_MARS_I2C_EN

#define GET_MUX_PAD0()                  rd_reg(MUX_PAD0)
#define SET_MUX_PAD0(x)                 wr_reg(MUX_PAD0, x)
#define GET_MUX_PAD3()                  rd_reg(MUX_PAD3)
#define SET_MUX_PAD3(x)                 wr_reg(MUX_PAD3, x)
#define GET_MUX_PAD5()                  rd_reg(MUX_PAD5)
#define SET_MUX_PAD5(x)                 wr_reg(MUX_PAD5, x)

#else

#define GET_MIS_PSELL()                 rd_reg(MIS_PSELL)
#define SET_MIS_PSELL(x)                wr_reg(MIS_PSELL, x)

#endif

#define GET_MIS_GP0DIR()                rd_reg(MIS_GP0DIR)
#define GET_MIS_GP0DATO()               rd_reg(MIS_GP0DATO)
#define GET_MIS_GP0DATI()               rd_reg(MIS_GP0DATI)
#define GET_MIS_GP0IE()                 rd_reg(MIS_GP0IE)
#define SET_MIS_GP0DIR(x)               wr_reg(MIS_GP0DIR, x)
#define SET_MIS_GP0DATO(x)              wr_reg(MIS_GP0DATO, x)
#define SET_MIS_GP0DATI(x)              wr_reg(MIS_GP0DATI, x)
#define SET_MIS_GP0IE(x)                wr_reg(MIS_GP0IE, x)

#define SET_MIS_ISR(x)                  wr_reg(MIS_ISR, x)
#define GET_MIS_ISR()                   rd_reg(MIS_ISR)


#define SET_IC_ENABLE(i,x)              wr_reg(IC_ENABLE[i],x)
#define SET_IC_CON(i,x)                 wr_reg(IC_CON[i], x)
#define GET_IC_CON(i)                   rd_reg(IC_CON[i])
#define SET_IC_SAR(i,x)                 wr_reg(IC_SAR[i], x)
#define SET_IC_TAR(i,x)                 wr_reg(IC_TAR[i], x)
#define GET_IC_TAR(i)                   rd_reg(IC_TAR[i])
#define SET_IC_DATA_CMD(i, x)           wr_reg(IC_DATA_CMD[i], x)
#define GET_IC_DATA_CMD(i)              rd_reg(IC_DATA_CMD[i])
                                        
#define SET_IC_SS_SCL_HCNT(i,x)         wr_reg(IC_SS_SCL_HCNT[i], x)
#define SET_IC_SS_SCL_LCNT(i,x)         wr_reg(IC_SS_SCL_LCNT[i], x)
                                        
#define GET_IC_STATUS(i)                rd_reg(IC_STATUS[i])
                                        
#define SET_IC_INTR_MASK(i,x)           wr_reg(IC_INTR_MASK[i], x)
#define GET_IC_INTR_MASK(i)             rd_reg(IC_INTR_MASK[i])
                                        
#define GET_IC_INTR_STAT(i)             rd_reg(IC_INTR_STAT[i])
#define GET_IC_RAW_INTR_STAT(i)         rd_reg(IC_RAW_INTR_STAT[i])
                                        
#define CLR_IC_INTR(i)     	            rd_reg(IC_CLR_INTR[i])
#define CLR_IC_RX_UNDER(i)     	        rd_reg(IC_CLR_RX_UNDER[i])
#define CLR_IC_TX_OVER(i)     	        rd_reg(IC_CLR_TX_OVER[i])
#define CLR_IC_RD_REQ(i)     	        rd_reg(IC_CLR_RD_REQ[i])
#define CLR_IC_RX_DONE(i)     	        rd_reg(IC_CLR_RX_DONE[i])
#define CLR_IC_ACTIVITY(i)     	        rd_reg(IC_CLR_ACTIVITY[i])
#define CLR_IC_GEN_CALL(i)     	        rd_reg(IC_CLR_GEN_CALL[i])
#define CLR_IC_TX_ABRT(i)     	        rd_reg(IC_CLR_TX_ABRT[i])
#define CLR_IC_STOP_DET(i)     	        rd_reg(IC_CLR_STOP_DET[i])
                                        
#define GET_IC_COMP_PARAM_1(i)          rd_reg(IC_COMP_PARAM_1[i])
                                        
#define GET_IC_TXFLR(i)                 rd_reg(IC_TXFLR[i])
#define GET_IC_RXFLR(i)                 rd_reg(IC_RXFLR[i])

#define GET_IC_RX_TL(i)                 rd_reg(IC_RX_TL[i])
#define GET_IC_TX_TL(i)                 rd_reg(IC_TX_TL[i])
#define SET_IC_RX_TL(i, x)              wr_reg(IC_RX_TL[i], x)
#define SET_IC_TX_TL(i, x)              wr_reg(IC_TX_TL[i], x)

                                        
#define GET_IC_TX_ABRT_SOURCE(i)        rd_reg(IC_TX_ABRT_SOURCE[i])
                                        
#define NOT_TXFULL(i)                   (GET_IC_STATUS(i) & ST_TFNF_BIT)
#define NOT_RXEMPTY(i)                  (GET_IC_STATUS(i) & ST_RFNE_BIT)
// for debug

#define i2c_print                       printk
#define dbg_char(x)                     wr_reg(0xb801b200, (unsigned long) (x))

#ifdef SPIN_LOCK_PROTECT_EN    
    #define LOCK_VENUS_I2C(a,b)         spin_lock_irqsave(a, b)     
    #define UNLOCK_VENUS_I2C(a, b)      spin_unlock_irqrestore(a, b)     
#else    
    #define LOCK_VENUS_I2C(a,b)         do { b = 1; }while(0)
    #define UNLOCK_VENUS_I2C(a, b)      do { b = 0; }while(0)
#endif

#ifdef I2C_PROFILEING_EN
#define LOG_EVENT(x)                    log_event(x)
#else
#define LOG_EVENT(x)                    
#endif

#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER           
int  venus_i2c_bus_jam_detect(venus_i2c* p_this);
void venus_i2c_bus_jam_recover_proc(venus_i2c* p_this);
#endif


/*------------------------------------------------------------------
 * Func : venus_i2c_timer
 *
 * Desc : timer of venus i2c - this timer is used to stop a blocked xfer
 *
 * Parm : arg : handle of venus i2c xfer complete
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/
void venus_i2c_timer(unsigned long arg)
{             
    venus_i2c* p_this = (venus_i2c*) arg;
    unsigned char i = p_this->id;
    
    i2c_print("i2c_%d : time out\n", i);
    LOG_EVENT(EVENT_EXIT_TIMEOUT); 
    SET_IC_INTR_MASK(i, 0);
    SET_IC_ENABLE(i, 0);       
    complete(&p_this->xfer.complete);  // wake up complete
}



/*------------------------------------------------------------------
 * Func : venus_i2c_msater_write
 *
 * Desc : master write handler for venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *        event  : INT event of venus i2c
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_i2c_msater_write(venus_i2c* p_this, unsigned int event, unsigned int tx_abort_source)
{                                     
#define TxComplete()              (p_this->xfer.tx_len >= p_this->xfer.tx_buff_len)            
    
    unsigned char i = p_this->id;
    
    while(!TxComplete() && NOT_TXFULL(i))
    {
        SET_IC_DATA_CMD(i, p_this->xfer.tx_buff[p_this->xfer.tx_len++]);
    }
    
    if (TxComplete())		    
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~TX_EMPTY_BIT);     
    }
    
    if (event & TX_ABRT_BIT)
    {        
        p_this->xfer.ret = -ETXABORT;
        p_this->xfer.tx_abort_source = tx_abort_source;
    }        
    else if (event & STOP_DET_BIT)
    {
        p_this->xfer.ret = TxComplete() ? p_this->xfer.tx_len : -ECMDSPLIT;	
    }    
    
    if (p_this->xfer.ret)
    {        
        SET_IC_INTR_MASK(i, 0);
        SET_IC_ENABLE(i, 0);                 
        p_this->xfer.mode = I2C_IDEL;	// change to idle state        
	    complete(&p_this->xfer.complete);
    }          
        
#undef TxComplete
}



/*------------------------------------------------------------------
 * Func : venus_i2c_msater_read
 *
 * Desc : master read handler for venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_i2c_msater_read(venus_i2c* p_this, unsigned int event, unsigned int tx_abort_source)
{        
#define TxComplete()        (p_this->xfer.tx_len >= p_this->xfer.rx_buff_len)          
#define RxComplete()        (p_this->xfer.rx_len >= p_this->xfer.rx_buff_len)                
    
    unsigned char i = p_this->id;
        
    // TX Thread          
    while(!TxComplete() && NOT_TXFULL(i))
    {
        SET_IC_DATA_CMD(i, READ_CMD);  // send read command to rx fifo
        p_this->xfer.tx_len++;
    }
            
    // RX Thread         
    while (!RxComplete() && NOT_RXEMPTY(i))      
    {
        p_this->xfer.rx_buff[p_this->xfer.rx_len++] = (unsigned char)(GET_IC_DATA_CMD(i) & 0xFF); 
    }        
    
    if (TxComplete())
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~TX_EMPTY_BIT);     
    }
         
    if (event & TX_ABRT_BIT)
    {                
        p_this->xfer.ret = -ETXABORT;
        p_this->xfer.tx_abort_source = tx_abort_source;
    }                
    else if ((event & STOP_DET_BIT) || RxComplete())
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~RX_FULL_BIT);
        
        p_this->xfer.ret = RxComplete() ? p_this->xfer.rx_len : -ECMDSPLIT;	
    }
    
    if (p_this->xfer.ret)
    {        
        SET_IC_INTR_MASK(i, 0);
        SET_IC_ENABLE(i, 0);         
        p_this->xfer.mode = I2C_IDEL;	// change to idle state        
	    complete(&p_this->xfer.complete);
    }
    
#undef TxComplete
#undef RxComplete 
}



/*------------------------------------------------------------------
 * Func : venus_i2c_msater_read
 *
 * Desc : master read handler for venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_i2c_msater_random_read(venus_i2c* p_this, unsigned int event, unsigned int tx_abort_source)
{    
    
#define TxComplete()        (p_this->xfer.tx_len >= (p_this->xfer.rx_buff_len + p_this->xfer.tx_buff_len))    // it should add the same number of read command to tx fifo
#define RxComplete()        (p_this->xfer.rx_len >=  p_this->xfer.rx_buff_len)            
    
    unsigned char i = p_this->id;
        
    // TX Thread        
    while(!TxComplete() && NOT_TXFULL(i))
    {
        if (p_this->xfer.tx_len < p_this->xfer.tx_buff_len)
            SET_IC_DATA_CMD(i, p_this->xfer.tx_buff[p_this->xfer.tx_len]);
        else
            SET_IC_DATA_CMD(i, READ_CMD);  // send read command to rx fifo                        
            
        p_this->xfer.tx_len++;
    }
        
    // RX Thread
    while(!RxComplete() && NOT_RXEMPTY(i))        
    {
        p_this->xfer.rx_buff[p_this->xfer.rx_len++] = (unsigned char)(GET_IC_DATA_CMD(i) & 0xFF); 
    }        
    
    if TxComplete()
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~TX_EMPTY_BIT);     
    }        

    if (event & TX_ABRT_BIT)
    {        
        p_this->xfer.ret = -ETXABORT;
        p_this->xfer.tx_abort_source = tx_abort_source;
    }           
    else if ((event & STOP_DET_BIT) || RxComplete())
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~RX_FULL_BIT);  
        p_this->xfer.ret = RxComplete() ? p_this->xfer.rx_len : -ECMDSPLIT;	
    }    

    if (p_this->xfer.ret)
    {        
        SET_IC_INTR_MASK(i, 0);
        SET_IC_ENABLE(i, 0);         
        p_this->xfer.mode = I2C_IDEL;	// change to idle state        
	    complete(&p_this->xfer.complete);
    }          
    
#undef TxComplete
#undef RxComplete 
}




/*------------------------------------------------------------------
 * Func : venus_i2c_isr
 *
 * Desc : isr of venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/
static 
irqreturn_t 
venus_i2c_isr(
    int                     this_irq, 
    void*                   dev_id, 
    struct pt_regs*         regs
    )
{        
    const unsigned long I2C_INT[] = {I2C0_INT, I2C1_INT};
    venus_i2c* p_this = (venus_i2c*) dev_id;
    unsigned char i = p_this->id;       
    unsigned long flags;
    
    LOCK_VENUS_I2C(&p_this->lock, flags);                   
               
    if (!(GET_MIS_ISR() & I2C_INT[i]))    // interrupt belongs to I2C
    {
        UNLOCK_VENUS_I2C(&p_this->lock, flags);
	    return IRQ_NONE;
    }
	    
    LOG_EVENT(EVENT_ENTER_ISR);      
	
    unsigned int event = GET_IC_INTR_STAT(i);    
    unsigned int tx_abrt_source = GET_IC_TX_ABRT_SOURCE(i);
    
	CLR_IC_INTR(i);	                    // clear interrupts of i2c_x
                                                        
    switch (p_this->xfer.mode)
    {
    case I2C_MASTER_WRITE:
        venus_i2c_msater_write(p_this, event, tx_abrt_source);
        break;
        
    case I2C_MASTER_READ:       
        venus_i2c_msater_read(p_this, event, tx_abrt_source);       
        break;
    
    case I2C_MASTER_RANDOM_READ:
        venus_i2c_msater_random_read(p_this, event, tx_abrt_source);
        break;        

    default:
        printk("Unexcepted Interrupt\n");    
        SET_IC_ENABLE(i, 0);                 
        
    }    
              
    mod_timer(&p_this->xfer.timer, jiffies + I2C_TIMEOUT_INTERVAL);        
        
    SET_MIS_ISR(I2C_INT[i]);   // clear I2C Interrupt Flag
    UNLOCK_VENUS_I2C(&p_this->lock, flags);
    return IRQ_HANDLED;
}


/*------------------------------------------------------------------
 * Func : venus_i2c_set_tar
 *
 * Desc : set tar of venus i2c 
 *
 * Parm : p_this : handle of venus i2c
 *        addr   : address of sar
 *        mode
  : mode of sar
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/
int 
venus_i2c_set_tar(
    venus_i2c*          p_this, 
    unsigned short      addr, 
    ADDR_MODE           mode
    )
{    
    unsigned char i = p_this->id;   
                       
    if (mode==ADDR_MODE_10BITS)
    {        
        if (addr > 0x3FF)                    
            return -EADDROVERRANGE;
        
        SET_IC_TAR(i, addr & 0x3FF);
        SET_IC_CON(i, (GET_IC_CON(i) & (~IC_10BITADDR_MASTER)) | IC_10BITADDR_MASTER);  
    }
    else      
    {   
        if (addr > 0x7F)        
            return -EADDROVERRANGE;
        
        SET_IC_TAR(i, addr & 0x7F);   
        SET_IC_CON(i, GET_IC_CON(i) & (~IC_10BITADDR_MASTER));           
    }
    
    p_this->tar      = addr;
    p_this->tar_mode = mode;
    
    return 0;                    
}  
  


/*------------------------------------------------------------------
 * Func : venus_i2c_set_sar
 *
 * Desc : set sar of venus i2c 
 *
 * Parm : p_this : handle of venus i2c
 *        addr   : address of sar
 *        mode
  : mode of sar
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/      
int 
venus_i2c_set_sar(
    venus_i2c*          p_this, 
    unsigned short      addr, 
    ADDR_MODE           mode
    )
{                
    unsigned char i = p_this->id;   
           
    if (mode==ADDR_MODE_10BITS)
    {
        SET_IC_SAR(i, p_this->sar & 0x3FF);
        SET_IC_CON(i, (GET_IC_CON(i) & (~IC_10BITADDR_SLAVE)) | IC_10BITADDR_SLAVE);  
    }
    else      
    {           
        SET_IC_SAR(i, p_this->sar & 0x7F);   
        SET_IC_CON(i, GET_IC_CON(i) & (~IC_10BITADDR_SLAVE));           
    }
    
    p_this->sar      = addr;
    p_this->sar_mode = mode;
    
    return 0;                    
}  


    
/*------------------------------------------------------------------
 * Func : venus_i2c_set_spd
 *
 * Desc : set speed of venus i2c 
 *
 * Parm : p_this : handle of venus i2c
 *        KHz    : operation speed of i2c
 *         
 * Retn : 0
 *------------------------------------------------------------------*/    
int venus_i2c_set_spd(venus_i2c* p_this, int KHz)
{                     
    unsigned char i = p_this->id;   
    unsigned int div_h = 0x7a;
    unsigned int div_l = 0x8f;
    
    if (KHz < 10 || KHz > 150)    
    {
        i2c_print("[I2C%d] warning, speed %d out of range, speed should between 10 ~ 150KHz\n", i, KHz);                
        return -1;
    }        
                     
    div_h = (div_h * 100)/KHz;
    div_l = (div_l * 100)/KHz;        
    
    if (div_h >= 0xFFFF || div_h==0 || 
        div_l >= 0xFFFF || div_l==0)
    {
        i2c_print("[I2C%d] fatal, set speed failed : divider divider out of range. div_h = %d, div_l = %d\n", i, div_h, div_l);        
        return -1;
    }            
    
    SET_IC_CON(i, (GET_IC_CON(i) & (~IC_SPEED)) | SPEED_SS);
    SET_IC_SS_SCL_HCNT(i, div_h);
    SET_IC_SS_SCL_LCNT(i, div_l);
    p_this->spd  = KHz;
    p_this->tick = 1000 / KHz;
    i2c_print("[I2C%d] i2c speed changed to %d KHz\n", i, p_this->spd);
    return 0;
}    



/*------------------------------------------------------------------
 * Func : venus_i2c_dump
 *
 * Desc : dump staus of venus i2c 
 *
 * Parm : p_this : handle of venus i2c
 *         
 * Retn : 0 for success
 *------------------------------------------------------------------*/    
int venus_i2c_dump(venus_i2c* p_this)
{   
    i2c_print("=========================\n"); 
    i2c_print("= VER : %s               \n", VERSION);
    i2c_print("=========================\n");
    i2c_print("= PHY : %d               \n", p_this->id);    
    i2c_print("= MODE: %s               \n", MODLE_NAME);
    i2c_print("= SPD : %d               \n", p_this->spd);
    i2c_print("= SAR : 0x%03x (%d bits) \n", p_this->sar, p_this->sar_mode);
    i2c_print("= TX FIFO DEPTH : %d     \n", p_this->tx_fifo_depth);
    i2c_print("= RX FIFO DEPTH : %d     \n", p_this->rx_fifo_depth);  
    i2c_print("= FIFO THRESHOLD: %d     \n", FIFO_THRESHOLD);
    
#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER           
    i2c_print("= BUS JAM RECORVER : ON  \n");     
#else    
    i2c_print("= BUS JAM RECORVER : OFF  \n");
#endif

#ifdef CONFIG_I2C_VENUS_NON_STOP_WRITE_XFER           
    i2c_print("= NON STOP WRITE : ON  \n");     
#else    
    i2c_print("= NON STOP WRITE : OFF  \n");
#endif

#ifdef SPIN_LOCK_PROTECT_EN    
    i2c_print("= SP PROTECT : ON  \n");
#else
    i2c_print("= SP PROTECT : OFF  \n");
#endif

    i2c_print("=========================\n");    
    return 0;
}




/*------------------------------------------------------------------
 * Func : venus_i2c_probe
 *
 * Desc : probe venus i2c
 *
 * Parm : p_this : handle of venus i2c
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/    
int venus_i2c_probe(venus_i2c* p_this)
{                      
    if (p_this->id >= MAX_I2C_CNT)  
        return -ENODEV;
     
#ifdef CONFIG_MARS_I2C_EN    

    // MARS     
    if (p_this->id==0)
    {
        switch(GET_MUX_PAD0() & I2C0_LOC_MASK)
        {
        case I2C0_LOC_QFPA_BGA: 
        case I2C0_LOC_QFPB:
        
            if ((GET_MUX_PAD3() & I2C0_MASK)== I2C0_SDA_SCL) 
            {        
                p_this->gpio_map.scl_bit    = GP4_BIT;
                p_this->gpio_map.sda_bit    = GP5_BIT;
                p_this->gpio_map.reg.gpie   = MIS_GP0IE;
                p_this->gpio_map.reg.gpdato = MIS_GP0DATO;
                p_this->gpio_map.reg.gpdati = MIS_GP0DATI;
                p_this->gpio_map.reg.gpdir  = MIS_GP0DIR;                                 
            }
            else
            {
                i2c_print("FATAL : I2C %d pins have been occupied for other application\n", p_this->id);
                return -ENODEV;
            }            
            break;
            
        default:
            i2c_print("FATAL : I2C %d does not exist\n", p_this->id);
            return -ENODEV;    
        }
        
    }
    else
    {           
        switch(GET_MUX_PAD0() & I2C1_LOC_MASK)
        {
        case I2C1_LOC_MUX_PCI: 
                    
            switch(GET_MUX_PAD5() & PCI_MUX1_MASK)
            {
            case PCI_MUX1_I2C1:                
                p_this->gpio_map.scl_bit    = GP40_BIT;
                p_this->gpio_map.sda_bit    = GP41_BIT;
                p_this->gpio_map.reg.gpie   = MIS_GP1IE;
                p_this->gpio_map.reg.gpdato = MIS_GP1DATO;
                p_this->gpio_map.reg.gpdati = MIS_GP1DATI;
                p_this->gpio_map.reg.gpdir  = MIS_GP1DIR;
                break;
                
            case PCI_MUX1_PCI:
                i2c_print("FATAL : I2C %d pins have been occupied by PCI\n", p_this->id);
                return -ENODEV;                
                                    
            case PCI_MUX1_GPIO:
                i2c_print("FATAL : I2C %d pins have been occupied by GPIO[40:41]\n", p_this->id);
                return -ENODEV;                
                
            default:
                i2c_print("FATAL : I2C %d pins have been reserved for other application\n", p_this->id);
                return -ENODEV;                                            
            }  
            break;   
                    
        case I2C1_LOC_MUX_UR0:
            
            switch(GET_MUX_PAD3() & I2C1_MASK)
            {
            case I2C1_SDA_SCL:                
                p_this->gpio_map.scl_bit    = GP0_BIT;
                p_this->gpio_map.sda_bit    = GP1_BIT;
                p_this->gpio_map.reg.gpie   = MIS_GP0IE;
                p_this->gpio_map.reg.gpdato = MIS_GP0DATO;
                p_this->gpio_map.reg.gpdati = MIS_GP0DATI;
                p_this->gpio_map.reg.gpdir  = MIS_GP0DIR;                                
                break;
                
            case I2C1_UART1:
                i2c_print("FATAL : I2C %d pins have been occupied by UART1\n", p_this->id);
                return -ENODEV;                
                                    
            case I2C1_GP0_GP1:
                i2c_print("FATAL : I2C %d pins have been occupied by GPIO[1:0]\n", p_this->id);
                return -ENODEV;                
                
            default:
                i2c_print("FATAL : I2C %d pins have been reserved for other application\n", p_this->id);
                return -ENODEV;                                            
            }            
            break;
            
        default:
            i2c_print("FATAL : I2C %d does not exist\n", p_this->id);
            return -ENODEV;    
        }                                                           
    }
    
#else

    // VENUS / NEPTUNE
    if ((GET_MIS_PSELL() & 0x00000f00) != 0x00000500) 
    {
        i2c_print("FATAL : GPIO 4/5 pins are not defined as I2C\n");
        return -ENODEV;
    }
    
    p_this->gpio_map.scl_bit    = GP4_BIT;
    p_this->gpio_map.sda_bit    = GP5_BIT;
    p_this->gpio_map.reg.gpie   = MIS_GP0IE;
    p_this->gpio_map.reg.gpdato = MIS_GP0DATO;
    p_this->gpio_map.reg.gpdati = MIS_GP0DATI;
    p_this->gpio_map.reg.gpdir  = MIS_GP0DIR;                
    
#endif    

    return 0;
}



/*------------------------------------------------------------------
 * Func : venus_i2c_init
 *
 * Desc : init venus i2c
 *
 * Parm : p_this : handle of venus i2c
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/    
int venus_i2c_init(venus_i2c* p_this)
{               
    unsigned char i = p_this->id;  
        
    if (venus_i2c_probe(p_this)<0) 
        return -ENODEV;    
         
    if (request_irq(p_this->irq, venus_i2c_isr, SA_INTERRUPT | SA_SHIRQ, "Venus I2C", p_this) < 0) 
    {
        i2c_print("FATAL : Request irq%d failed\n", p_this->irq);	    		
        return -ENODEV;
    }            
    
    p_this->rx_fifo_depth = ((GET_IC_COMP_PARAM_1(i) >>  8) & 0xFF)+1;
    p_this->tx_fifo_depth = ((GET_IC_COMP_PARAM_1(i) >> 16) & 0xFF)+1; 
    spin_lock_init(&p_this->lock);
        
    SET_IC_ENABLE(i, 0);
    SET_IC_INTR_MASK(i, 0);                // disable all interrupt      
    SET_IC_CON(i, IC_SLAVE_DISABLE | IC_RESTART_EN | SPEED_SS | IC_MASTER_MODE);  
    SET_IC_TX_TL(i, FIFO_THRESHOLD);
    SET_IC_RX_TL(i, p_this->rx_fifo_depth - FIFO_THRESHOLD);
     
    venus_i2c_set_spd(p_this, p_this->spd);
    venus_i2c_set_sar(p_this, p_this->sar, p_this->sar_mode);    

#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER         
    if (venus_i2c_bus_jam_detect(p_this))
    {                        
        i2c_print("I2C%d Bus Status Check.... Error... Try to Recrver\n",p_this->id);
        venus_i2c_bus_jam_recover_proc(p_this);                                        
    }
    else
        i2c_print("I2C%d Bus Status Check.... OK\n",p_this->id);
#endif
    
    venus_i2c_dump(p_this);
    return 0;
}    



/*------------------------------------------------------------------
 * Func : venus_i2c_uninit
 *
 * Desc : uninit venus i2c
 *
 * Parm : p_this : handle of venus i2c
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/    
int venus_i2c_uninit(venus_i2c* p_this)
{       
    unsigned char i = p_this->id;
    
    SET_IC_ENABLE(i, 0);
    SET_IC_INTR_MASK(i, 0);    
    free_irq(p_this->irq, p_this);     
    return 0;
}    
 
 
 
 
enum {
    I2C_MODE    = 0,
    GPIO_MODE   = 1
};



/*------------------------------------------------------------------
 * Func : venus_i2c_gpio_selection
 *
 * Desc : select i2c/GP45 output
 *
 * Parm : p_this : handle of venus i2c
 *        mode : 0      : SDA / SCL
 *               others : GP4 / GP5 (I2C0)  GP0 / GP1 (I2C1)
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void venus_i2c_gpio_selection(venus_i2c* p_this, unsigned char mode)
{        
    
#ifdef CONFIG_MARS_I2C_EN                   
    
    if (!p_this->id)
    {
        if (mode==GPIO_MODE)
            SET_MUX_PAD3((GET_MUX_PAD3() & ~I2C0_MASK)| I2C0_GP4_GP5);         // GP4=GP4,GP5=GP5        
        else
            SET_MUX_PAD3((GET_MUX_PAD3() & ~I2C0_MASK)| I2C0_SDA_SCL);         // GP4=SCL,GP5=SDA        
    }
    else
    {
        switch(GET_MUX_PAD0() & I2C1_LOC_MASK)
        {
        case I2C1_LOC_MUX_PCI: 
                    
            if (mode==GPIO_MODE)
                SET_MUX_PAD5((GET_MUX_PAD5() & ~PCI_MUX1_MASK)| PCI_MUX1_GPIO);         
            else
                SET_MUX_PAD5((GET_MUX_PAD5() & ~PCI_MUX1_MASK)| PCI_MUX1_I2C1);           
            break;   
                    
        case I2C1_LOC_MUX_UR0:
            
            if (mode==GPIO_MODE)
                SET_MUX_PAD3((GET_MUX_PAD3() & ~I2C1_MASK)| I2C1_GP0_GP1);         
            else
                SET_MUX_PAD3((GET_MUX_PAD3() & ~I2C1_MASK)| I2C1_SDA_SCL);                
            break;
            
        default:
            i2c_print("FATAL : I2C %d does not exist\n", p_this->id);            
        }                
    }   
    
#else

    if (mode==GPIO_MODE)
        SET_MIS_PSELL (GET_MIS_PSELL() & ~0x00000f00);                          // GP4=GP4,GP5=GP5    
    else
        SET_MIS_PSELL((GET_MIS_PSELL() & ~0x00000f00) | 0x00000500);     	    // GP4=SCL,GP5=SDA           
        
#endif    
}
    
        
    
/*------------------------------------------------------------------
 * Func : venus_i2c_suspend
 *
 * Desc : suspend venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 for success
 *------------------------------------------------------------------*/    
int venus_i2c_suspend(venus_i2c* p_this)
{           
    unsigned long MIS_GPIE      = p_this->gpio_map.reg.gpie; 
    unsigned long MIS_GPDATI    = p_this->gpio_map.reg.gpdati; 
    unsigned long MIS_GPDIR     = p_this->gpio_map.reg.gpdir;            
    unsigned long SDA_BIT       = p_this->gpio_map.sda_bit;
    unsigned long SCL_BIT       = p_this->gpio_map.scl_bit;
    unsigned long SDA_SCL_MASK  =(SDA_BIT | SCL_BIT);
        
#ifdef GPIO_MODE_SUSPEND
    i2c_print("[I2C%d] suspend\n", p_this->id);    
    
    while (p_this->xfer.mode!=I2C_IDEL)    
        msleep(1);

    wr_reg(MIS_GPDIR, rd_reg(MIS_GPDIR) & ~SDA_SCL_MASK);                     // Dir = Input
    wr_reg(MIS_GPIE,  rd_reg(MIS_GPIE)  & ~SDA_SCL_MASK);                     // Interrupt Disable            
    venus_i2c_gpio_selection(p_this, GPIO_MODE); 
#endif
    return 0;
}


/*------------------------------------------------------------------
 * Func : venus_i2c_resume
 *
 * Desc : resumevenus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 for success
 *------------------------------------------------------------------*/    
int venus_i2c_resume(venus_i2c* p_this)
{   
#ifdef GPIO_MODE_SUSPEND
    i2c_print("[I2C%d] resume\n", p_this->id);                    
    venus_i2c_gpio_selection(p_this, I2C_MODE);   
#endif 
    return 0;
}



#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER 

/*------------------------------------------------------------------
 * Func : venus_i2c_bus_jam_recover
 *
 * Desc : recover i2c bus jam status 
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void venus_i2c_bus_jam_recover(venus_i2c* p_this)
{        
    unsigned long MIS_GPIE      = p_this->gpio_map.reg.gpie; 
    unsigned long MIS_GPDATO    = p_this->gpio_map.reg.gpdato; 
    unsigned long MIS_GPDIR     = p_this->gpio_map.reg.gpdir;    
    unsigned long SDA_BIT       = p_this->gpio_map.sda_bit;
    unsigned long SCL_BIT       = p_this->gpio_map.scl_bit;
    unsigned long SDA_SCL_MASK  =(SDA_BIT | SCL_BIT);            
    int i;
    int d = p_this->tick / 2;
                
    // Config GP
    wr_reg(MIS_GPIE,   rd_reg(MIS_GPIE)   & ~SDA_SCL_MASK);             // Disable GPIO Interrupt
    wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) & ~(SCL_BIT | SDA_BIT));      // pull low SCL & SDA    
    wr_reg(MIS_GPDIR,  rd_reg(MIS_GPDIR)  | SCL_BIT | SDA_BIT );        // mode : SCL : out, SDA : out
        
    venus_i2c_gpio_selection(p_this, GPIO_MODE);
    
    //Add Stop Condition
	udelay(10);
    wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) | SCL_BIT);                   // pull high SCL
	udelay(10);
    wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) | SDA_BIT);                   // pull high SDA
	udelay(10);
    
    wr_reg(MIS_GPDIR,  rd_reg(MIS_GPDIR)  & ~SDA_BIT);                  // mode : SCL : out, SDA : in
        
    // Output Clock Modify Clock Output from 10 to 9
    for (i=0; i<9; i++)
    {
        wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) & ~SCL_BIT);         
        udelay(d);
        wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) | SCL_BIT);                    
        udelay(d);
    }
    
    // Config GP 4/5    
    wr_reg(MIS_GPDIR, rd_reg(MIS_GPDIR) & ~SDA_SCL_MASK);

    venus_i2c_gpio_selection(p_this, I2C_MODE);
}



/*------------------------------------------------------------------
 * Func : venus_i2c_bus_jam_detect
 *
 * Desc : check if bus jam occurs
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 : bus not jammed, 1 : bus jammed 
 *------------------------------------------------------------------*/
int venus_i2c_bus_jam_detect(venus_i2c* p_this)
{               
    unsigned long MIS_GPIE      = p_this->gpio_map.reg.gpie; 
    unsigned long MIS_GPDATI    = p_this->gpio_map.reg.gpdati; 
    unsigned long MIS_GPDIR     = p_this->gpio_map.reg.gpdir;            
    unsigned long SDA_BIT       = p_this->gpio_map.sda_bit;
    unsigned long SCL_BIT       = p_this->gpio_map.scl_bit;
    unsigned long SDA_SCL_MASK  =(SDA_BIT | SCL_BIT);        
    int ret = 1;
    int i;            
              
    wr_reg(MIS_GPIE,  rd_reg(MIS_GPIE)  & ~SDA_SCL_MASK);       // GPIO Interrupt Disable
    wr_reg(MIS_GPDIR, rd_reg(MIS_GPDIR) & ~SDA_SCL_MASK);       // GPIO Dir=Input    
    
    venus_i2c_gpio_selection(p_this, GPIO_MODE);
		
    for(i=0; i<30; i++)
    {
        if ((rd_reg(MIS_GPDATI) & SDA_SCL_MASK)==SDA_SCL_MASK)	        // SDA && SCL == High
        {
            ret = 0;
            break;
        }
        msleep(1);
    }
    
    if (ret)
    {        
        SDA_SCL_MASK = rd_reg(MIS_GPDATI);        
        i2c_print("I2C %d Jamed, BUS Status: SDA=%d, SCL=%d\n", 
                p_this->id, 
                (SDA_SCL_MASK & SDA_BIT) ? 1 : 0,
                (SDA_SCL_MASK & SCL_BIT) ? 1 : 0);
    }        
                
    venus_i2c_gpio_selection(p_this, I2C_MODE);
    
    return ret;    
}




/*------------------------------------------------------------------
 * Func : venus_i2c_bus_jam_recover
 *
 * Desc : recover i2c bus jam status 
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void venus_i2c_bus_jam_recover_proc(venus_i2c* p_this)
{
    int i = 0;
    
    do
    {   
        if (venus_i2c_bus_jam_detect(p_this)==0)               
        {                 
            i2c_print("I2C%d Bus Recover successed\n",p_this->id);
            return ;
        }
                         
        i2c_print("Do I2C%d Bus Recover %d\n",p_this->id, i);
    
        venus_i2c_bus_jam_recover(p_this);
        
        msleep(200);                
            
    }while(i++ < 3);
    
    i2c_print("I2C%d Bus Recover failed\n",p_this->id);
}
#endif


/*------------------------------------------------------------------
 * Func : venus_i2c_start_xfer
 *
 * Desc : start xfer message
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/
int venus_i2c_start_xfer(venus_i2c* p_this)
{        
    unsigned char i = p_this->id;    
    unsigned long flags;
    int ret;
    int mode = p_this->xfer.mode;

    LOG_EVENT(EVENT_START_XFER);
    
    LOCK_VENUS_I2C(&p_this->lock, flags);
            
    switch (p_this->xfer.mode)
    {
    case I2C_MASTER_WRITE:  
        SET_IC_INTR_MASK(i, TX_EMPTY_BIT | TX_ABRT_BIT | STOP_DET_BIT);
        break;
        
    case I2C_MASTER_READ:    
    case I2C_MASTER_RANDOM_READ:   
    
        if (GET_IC_RXFLR(i)) 
        {
            printk("WARNING, RX FIFO NOT EMPRY\n");
            while(GET_IC_RXFLR(i))  
                 GET_IC_DATA_CMD(i);            
        }
    
        SET_IC_INTR_MASK(i, RX_FULL_BIT | TX_EMPTY_BIT | TX_ABRT_BIT | STOP_DET_BIT);    
        break;
            
    default:
        UNLOCK_VENUS_I2C(&p_this->lock, flags);       
        LOG_EVENT(EVENT_STOP_XFER);        
        return -EILLEGALMSG;
    }                

#ifdef MINIMUM_DELAY_EN

    UNLOCK_VENUS_I2C(&p_this->lock, flags);
    
    if (jiffies <= p_this->time_stamp)
        udelay(1000);

    LOCK_VENUS_I2C(&p_this->lock, flags);
    
#endif 
                        
    p_this->xfer.timer.expires  = jiffies + I2C_TIMEOUT_INTERVAL;    
    add_timer(&p_this->xfer.timer);
                                    
    SET_IC_ENABLE(i, 1);                   // Start Xfer
    
    UNLOCK_VENUS_I2C(&p_this->lock, flags);       
    
    wait_for_completion(&p_this->xfer.complete);
    
    LOCK_VENUS_I2C(&p_this->lock, flags);  
    
    del_timer(&p_this->xfer.timer);
    
    if (p_this->xfer.mode != I2C_IDEL)
    {
        p_this->xfer.ret  = -ETIMEOUT;
        
#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER

        UNLOCK_VENUS_I2C(&p_this->lock, flags);       

        // Bus Jammed Recovery Procedure
        if (venus_i2c_bus_jam_detect(p_this))
        {
            printk("WARNING, I2C Bus Jammed, Do Recorver\n");        
            venus_i2c_bus_jam_recover_proc(p_this);
            msleep(50);
        }

        LOCK_VENUS_I2C(&p_this->lock, flags);  
#endif
    }
    else if (p_this->xfer.ret==-ECMDSPLIT)
    {
        switch(mode)
        {
        case I2C_MASTER_WRITE:          
            printk("WARNING, Write Cmd Split, tx : %d/%d\n", 
                    p_this->xfer.tx_len, p_this->xfer.tx_buff_len);                     
            break;
        
        case I2C_MASTER_READ:                
            printk("WARNING, Read Cmd Split, tx : %d/%d rx : %d/%d\n", 
                    p_this->xfer.tx_len, p_this->xfer.tx_buff_len,
                    p_this->xfer.rx_len, p_this->xfer.rx_buff_len);
            break;                    
            
        case I2C_MASTER_RANDOM_READ:              
            printk("WARNING, Read Cmd Split, tx : %d/%d rx : %d/%d\n", 
                    p_this->xfer.tx_len, p_this->xfer.tx_buff_len + p_this->xfer.rx_buff_len,
                    p_this->xfer.rx_len, p_this->xfer.rx_buff_len);
            break;                                        
        }        
    }

#ifdef MINIMUM_DELAY_EN            
    p_this->time_stamp = (unsigned long) jiffies;
#endif    

    ret = p_this->xfer.ret;
    
    UNLOCK_VENUS_I2C(&p_this->lock, flags);       

#ifndef MINIMUM_DELAY_EN
    udelay(1200);
#endif

    if (ret==-ECMDSPLIT)
    {
        if (venus_i2c_probe(p_this)<0)
            printk("WARNING, I2C %d no longer exists\n", p_this->id);
    }    
    
    LOG_EVENT(EVENT_STOP_XFER);            
    
    return ret;
}




#ifdef CONFIG_I2C_VENUS_NON_STOP_WRITE_XFER


/*------------------------------------------------------------------
 * Func : venus_i2c_start_gpio_xfer
 *
 * Desc : venus_i2c_start_gpio_xfer
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A
 *
 * Note : this file using GPIO4/5 to out I2C protocol. where GP4 is SCLK
 *        GP5 is SDA 
 *------------------------------------------------------------------*/
int venus_i2c_start_gpio_xfer(
    venus_i2c*              p_this    
    )
{              
    unsigned long MIS_GPIE      = p_this->gpio_map.reg.gpie; 
    unsigned long MIS_GPDATI    = p_this->gpio_map.reg.gpdati; 
    unsigned long MIS_GPDATO    = p_this->gpio_map.reg.gpdato; 
    unsigned long MIS_GPDIR     = p_this->gpio_map.reg.gpdir;            
    unsigned long SDA_BIT       = p_this->gpio_map.sda_bit;
    unsigned long SCL_BIT       = p_this->gpio_map.scl_bit;
    unsigned long SDA_SCL_MASK  =(SDA_BIT | SCL_BIT);    
    int i;
    int j;
    int d = p_this->tick / 2;      
    unsigned char data;        
    
    if (p_this->xfer.mode != I2C_MASTER_WRITE) 
    {
        i2c_print("FATAL : %s supports write message only\n", __FUNCTION__);
        return -EILLEGALMSG;
    } 
    
    p_this->xfer.ret = 0;
    p_this->xfer.timer.expires  = jiffies + 2 * HZ;
             
    wr_reg(MIS_GPDIR, rd_reg(MIS_GPDIR) & ~SDA_SCL_MASK);                     // Dir = Input
    wr_reg(MIS_GPIE,  rd_reg(MIS_GPIE)  & ~SDA_SCL_MASK);                     // Interrupt Disable

    venus_i2c_gpio_selection(p_this, GPIO_MODE);
            
    //---- wait for bus free
    while((rd_reg(MIS_GPDATI)& SDA_SCL_MASK)!=SDA_SCL_MASK)
    {
        if (jiffies > p_this->xfer.timer.expires)
        {
            p_this->xfer.ret = -ETIMEOUT;                
            goto stop_xfer;
        }
    }     
    
    // Do Start 
    wr_reg(MIS_GPDATO, (rd_reg(MIS_GP0DATO) & ~SDA_SCL_MASK) | SCL_BIT);      // SDA = Low, SCL = High 
    wr_reg(MIS_GPDIR,   rd_reg(MIS_GP0DIR)  |  SDA_SCL_MASK);                 // SDA/SCL Dir = Out
    udelay(d);
            
    // Send data
    for (i=-1; i<p_this->xfer.tx_buff_len && !p_this->xfer.ret; i++)
    {                       
        data = (i<0) ? (p_this->tar <<1) : p_this->xfer.tx_buff[i];   
                     
        // Send Data Bits
        for (j=7; j>=0; j--)
        {               
            wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) & ~SCL_BIT);       // SCL = Low
            udelay(2);
                                                                       
            if ((data>>j) & 0x01)                
                wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) | SDA_BIT);    // SDA = High    
            else                
                wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) & ~SDA_BIT);   // SDA = Low                                
            udelay(d-2);
                                           
            wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) | SCL_BIT);        // SCL = High
            udelay(d);
        }

        // Get Ack Bit                                    
        wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) & ~SCL_BIT);           // SCL = Low        
        wr_reg(MIS_GPDIR,  rd_reg(MIS_GPDIR)  & ~SDA_BIT);           // SDA Dir = In
        udelay(d);
                                
        wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) | SCL_BIT);            // SCL = High
        udelay(d);
                
        if ((rd_reg(MIS_GPDATI)& SDA_BIT)!=0) 
        {
            p_this->xfer.ret = -ETXABORT;
            goto stop_xfer;
        }
                        
        wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) & ~SDA_BIT);           // SDA = Low
        wr_reg(MIS_GPDIR,  rd_reg(MIS_GPDIR)  | SDA_BIT);            // SDA Dir = Out            
    }
        
#if 1        
    wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO)  & ~SCL_BIT);              // SCL = Low    
    udelay(2);
    wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) | SDA_BIT);               // SDA = High
    udelay(d-2);
#else
    /* this Stop has been removed, because of this function only invoked in non stop write
    // Do Stop
    wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) & ~SCL_BIT);               // SCL = Low                
    udelay(d);    
    wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) | SCL_BIT);                // SCL = High
    udelay(2);
    wr_reg(MIS_GPDATO, rd_reg(MIS_GPDATO) | SDA_BIT);                // SDA = High        
    */
#endif
        
stop_xfer:
    
    wr_reg(MIS_GPDIR, rd_reg(MIS_GP0DIR) & ~SDA_SCL_MASK);           // Dir = Input    
    
    venus_i2c_gpio_selection(p_this, I2C_MODE);
    
#ifndef MINIMUM_DELAY_EN
    p_this->time_stamp = jiffies-1; // non stop
#endif        

    return p_this->xfer.ret;
}

#endif



/*------------------------------------------------------------------
 * Func : venus_i2c_get_tx_abort_reason
 *
 * Desc : get reason of tx abort, this register will be clear when new message is loaded
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : tx about source
 *------------------------------------------------------------------*/    
unsigned int venus_i2c_get_tx_abort_reason(venus_i2c* p_this)
{               
    return p_this->xfer.tx_abort_source;    
}




/*------------------------------------------------------------------
 * Func : venus_i2c_load_message
 *
 * Desc : load a i2c message (just add this message to the queue)
 *
 * Parm : p_this : handle of venus i2c 
 *        
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/    
int venus_i2c_load_message(
    venus_i2c*              p_this,
    unsigned char           mode,
    unsigned char*          tx_buf, 
    unsigned short          tx_buf_len, 
    unsigned char*          rx_buf, 
    unsigned short          rx_buf_len 
    )
{           
    unsigned long flags;
    
    LOCK_VENUS_I2C(&p_this->lock, flags);     
    
    memset(&p_this->xfer, 0, sizeof(p_this->xfer));        
        
    p_this->xfer.mode           = mode;
    p_this->xfer.tx_buff        = tx_buf;
    p_this->xfer.tx_buff_len    = tx_buf_len;
    p_this->xfer.rx_buff        = rx_buf;
    p_this->xfer.rx_buff_len    = rx_buf_len;
    
    //reset timer
    init_timer(&p_this->xfer.timer);    
    p_this->xfer.timer.data     = (unsigned long) p_this;
    p_this->xfer.timer.expires  = jiffies + 4 * HZ;
    p_this->xfer.timer.function = venus_i2c_timer; 
    
    //reset timer
    init_completion(&p_this->xfer.complete);
    
    UNLOCK_VENUS_I2C(&p_this->lock, flags);     
    
    return 0;
}


/*------------------------------------------------------------------
 * Func : venus_i2c_read
 *
 * Desc : read data from sar
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/
int venus_i2c_read(
    venus_i2c*              p_this, 
    unsigned char*          tx_buf, 
    unsigned short          tx_buf_len, 
    unsigned char*          rx_buf, 
    unsigned short          rx_buf_len
    )
{
    venus_i2c_load_message(p_this, 
            (tx_buf_len) ? I2C_MASTER_RANDOM_READ : I2C_MASTER_READ,
            tx_buf, tx_buf_len, rx_buf, rx_buf_len);
    
    return venus_i2c_start_xfer(p_this);
}    



/*------------------------------------------------------------------
 * Func : venus_i2c_write
 *
 * Desc : write data to sar
 *
 * Parm : p_this : handle of venus i2c 
 *        tx_buf : data to write
 *        tx_buf_len : number of bytes to write
 *        wait_stop  : wait for stop of not (extension)
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/
int venus_i2c_write(
    venus_i2c*              p_this, 
    unsigned char*          tx_buf, 
    unsigned short          tx_buf_len,
    unsigned char           wait_stop
    )
{
    venus_i2c_load_message(p_this, I2C_MASTER_WRITE, 
                           tx_buf, tx_buf_len, NULL, 0);                               
                               
#ifdef CONFIG_I2C_VENUS_NON_STOP_WRITE_XFER
    
    if (!wait_stop)
    {
        return venus_i2c_start_gpio_xfer(p_this);
    }            
    
#endif       

    return venus_i2c_start_xfer(p_this);
}



static unsigned char flags = 0;



/*------------------------------------------------------------------
 * Func : create_venus_i2c_handle
 *
 * Desc : create handle of venus i2c
 *
 * Parm : N/A
 *         
 * Retn : handle of venus i2c
 * 
 *------------------------------------------------------------------*/
venus_i2c*  
create_venus_i2c_handle(
    unsigned char       id,
    unsigned short      sar,
    ADDR_MODE           sar_mode,
    unsigned int        spd,
    unsigned int        irq
    )
{
    venus_i2c* hHandle;
    
    if (id >= MAX_I2C_CNT || (flags>>id) & 0x01)
        return NULL;
        
    hHandle = kmalloc(sizeof(venus_i2c),GFP_KERNEL);        
    
    if (hHandle!= NULL)
    {
        hHandle->id           = id;
        hHandle->irq          = irq;
        hHandle->sar          = sar;               
        hHandle->sar_mode     = sar_mode;
        hHandle->spd          = spd;                
        hHandle->init         = venus_i2c_init;
        hHandle->uninit       = venus_i2c_uninit;
        hHandle->set_spd      = venus_i2c_set_spd;
        hHandle->set_tar      = venus_i2c_set_tar;
        hHandle->read         = venus_i2c_read;
        hHandle->write        = venus_i2c_write;        
        hHandle->dump         = venus_i2c_dump;      
        hHandle->suspend      = venus_i2c_suspend;        
        hHandle->resume       = venus_i2c_resume;  
        hHandle->get_tx_abort_reason = venus_i2c_get_tx_abort_reason;
        
        memset(&hHandle->xfer, 0, sizeof(venus_i2c_xfer));  
        
        flags |= (0x01 << id);
    }
    
    return hHandle;
}   




/*------------------------------------------------------------------
 * Func : destroy_venus_i2c_handle
 *
 * Desc : destroy handle of venus i2c
 *
 * Parm : N/A
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void destroy_venus_i2c_handle(venus_i2c* hHandle)
{    
    if (hHandle==NULL)
        return;    
        
    hHandle->uninit(hHandle);
    flags &= ~(0x01<<hHandle->id);    
    kfree(hHandle);             
}   

