/*=============================================================
 * Copyright (c)      Realtek Semiconductor Corporation, 2004 		*
 * All rights reserved.                                       					*
 *============================================================*/

/*======================= Description ========================*/
/**
 * @file
 * This the PCMCIA verifer for CA.
 *
 * @author Moya Yu
 * @date { 2009/04/14 }
 * @version { 1.0 }
 *
 */

/*======================= CVS Headers =========================
  $Header: /cvsroot/dtv/linux-2.6/drivers/rtd2880/ci/pcmcia.c,v 1.5 2005/11/18 07:13:51 tzungder Exp $

  $Log: pcmcia.c,v $
  
  Revision 1.6  2009/04/14 07:13:51  kevin Wang 
  *: update to support rtd 1283
  
  Revision 1.5  2005/11/18 07:13:51  tzungder
  *: add ifdef for 5280 vecter int

  Revision 1.4  2005/11/10 02:57:53  ycchen
  :add more comment for hard reset

  Revision 1.3  2005/11/01 06:00:28  ycchen
  +: add prefix 'PCMCIA' in debug message
  +: add a hardware reset function

  Revision 1.2  2005/01/18 01:45:12  ghyu
  +: Move these header files to include directory.

  Revision 1.1  2005/01/13 02:40:56  ghyu
  +: Initail version for CI.

 *=============================================================*/

/*====================== Include files ========================*/
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach-venus/mars.h>
#include "ci.h"
#include "pcmcia.h"

/*======================== Constant ===========================*/
#define MIS_IRQ	                 3
#define PCM_DEV_ID		         0x5653   //!< Device ID for PCMCIA interface
#define RESET_TIME               500      //ms
#define PC_AT_IO_DRFR            PC_AT(0)
#define PC_AT_IO                 PC_AT(0)
#define PC_AT_ATTRMEM            PC_AT(1)

/*======================== Type Defines =======================*/


/*==================== Macro Definition =======================*/
#define rtd_outl(addr, val)    (writel((u32)(val), (volatile void __iomem*) (addr)))
#define rtd_inl(addr)          ((u32) readl((volatile void __iomem*) (addr)))

#define SET_MIS_ISR(x)	        rtd_outl(MARS_MIS_BASE + MARS_MIS_ISR, x)
#define GET_MIS_ISR()	        rtd_inl(MARS_MIS_BASE + MARS_MIS_ISR)

#define SET_MUXPAD5(x)	        rtd_outl(MARS_0_SYS_MUXPAD5, x)
#define GET_MUXPAD5()	        rtd_inl(MARS_0_SYS_MUXPAD5)

#define SET_PCICK_SEL(x)	    rtd_outl(MARS_CRT_BASE+ MARS_CRT_PCICK_SEL, x)
#define GET_PCICK_SEL()	        rtd_inl(MARS_CRT_BASE+ MARS_CRT_PCICK_SEL)

#define SET_PCMCIA_CMDFF(x)	    rtd_outl(MARS_MIS_BASE + MARS_PCMCIA_CMDFF, x)
#define SET_PCMCIA_CTRL(x)      rtd_outl(MARS_MIS_BASE + MARS_PCMCIA_CTRL,  x)
#define SET_PCMCIA_STS(x)       rtd_outl(MARS_MIS_BASE + MARS_PCMCIA_STS,   x)
#define SET_PCMCIA_AMTC(x)	    rtd_outl(MARS_MIS_BASE + MARS_PCMCIA_AMTC,  x)
#define SET_PCMCIA_IOMTC(x)     rtd_outl(MARS_MIS_BASE + MARS_PCMCIA_IOMTC, x)
#define SET_PCMCIA_MATC(x)      rtd_outl(MARS_MIS_BASE + MARS_PCMCIA_MATC,  x)
#define SET_PCMCIA_ACTRL(x)     rtd_outl(MARS_MIS_BASE + MARS_PCMCIA_ACTRL, x)

#define GET_PCMCIA_CMDFF()	    rtd_inl(MARS_MIS_BASE + MARS_PCMCIA_CMDFF)
#define GET_PCMCIA_CTRL()       rtd_inl(MARS_MIS_BASE + MARS_PCMCIA_CTRL)
#define GET_PCMCIA_STS()        rtd_inl(MARS_MIS_BASE + MARS_PCMCIA_STS)
#define GET_PCMCIA_AMTC()	    rtd_inl(MARS_MIS_BASE + MARS_PCMCIA_AMTC)
#define GET_PCMCIA_IOMTC()      rtd_inl(MARS_MIS_BASE + MARS_PCMCIA_IOMTC)
#define GET_PCMCIA_MATC()       rtd_inl(MARS_MIS_BASE + MARS_PCMCIA_MATC)
#define GET_PCMCIA_ACTRL()      rtd_inl(MARS_MIS_BASE + MARS_PCMCIA_ACTRL)

#define pcmcia_info(fmt, args...)             printk("[PCMCIA] INFO, " fmt, ## args)
#define pcmcia_warning(fmt, args...)          printk("[PCMCIA] WARNING, " fmt, ## args)

#ifdef CONFIG_MARS_DEBUG_PC
#define dprintk(fmt, args...)                 printk("[PCMCIA] DBG, " fmt, ## args)
#define dbgchar(x)	                          rtd_outl(MARS_MIS_BASE + MARS_MIS_U0RBR_THR_DLL, x)
#else

#define dprintk(format, args...)
#define pcmcia_info(args...)  
#define dbgchar(x)

#endif
 
//Chuck: we don't use PC_PII1 and PC_PII2 interrupt because the CA module is saw as passive one
#define PC_INTMASK    (PC_PCII1 | PC_PCRI1 | PC_PCII2 | PC_PCRI2 | PC_AFI | PC_APFI) 
#define PC_INTMASK3   (PC_PCII1 | PC_PCRI1 | PC_PCII2 | PC_PCRI2 )
#define PC_INTMASK5   (PC_PII1  | PC_PCII1 | PC_PCRI1 | PC_PII2 | PC_PCII2 | PC_PCRI2)

// Timing Setting
#define	AMTC_CFG      (PC_TWE(0x13) | PC_THD(0x03) | PC_TAOE(0x11) | PC_THCE(0x02) | PC_TSU(0x04))
#define	IOMTC_CFG     (PC_TDIORD(0x08) | PC_TSUIO(0x02) | PC_TDINPACK(0x06) | PC_TDWT(0x02))
#define	MATC_CFG      (PC_TC(0x13) | PC_THDIO(0x02) | PC_TCIO(0x13) | PC_TWIOWR(0x08))

/*===================== Static Variable =======================*/
static ci_int_handler* p_ci_handler = NULL;
DECLARE_WAIT_QUEUE_HEAD(pcmcia_wait);
static volatile unsigned long runing_state = PCMCIA_IDEL;



 
/*------------------------------------------------------------------
 * Func : rtdpc_probe
 *
 * Desc : check is the pcmcia controller available or not
 *
 * Parm : N/A  
 *         
 * Retn : R_ERR_SUCCESS : PCMCIA Interface is Available
 *        R_ERR_FAILED : PCMCIA Interface is not Available
 *------------------------------------------------------------------*/ 
static __init int rtdpc_probe(void)
{
#if 1
    // disable pci host mode (this feature will conflict with pcmcia feature)
    SET_PCICK_SEL(0);       
    SET_MUXPAD5((GET_MUXPAD5() & ~0x3FFC0) | 0x11140);    
#endif
    
    if (GET_PCICK_SEL())
    {
        pcmcia_warning("PCMCIA might has problem when PCI host mode is set. (PCICK_SEL=%08x)\n", GET_PCICK_SEL());
        return R_ERR_FAILED;
    }
    
    if ((GET_MUXPAD5() & 0x3FFC0) != 0x0011140)
    {
        pcmcia_warning("PCMCIA is not available, pin occupied. (MUXPAD5=%08x)\n", GET_MUXPAD5());
        return R_ERR_FAILED;
    }                        
    
    pcmcia_info("PCMCIA Interface is available\n");
    return R_ERR_SUCCESS;
}




/*------------------------------------------------------------------
 * Func : rtdpc_reset
 *
 * Desc : software reset the pcmcia interface
 *
 * Parm : N/A   : reset fsm of pcmcia controller
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/    
void rtdpc_reset(void)
{    
    SET_PCMCIA_CTRL(GET_PCMCIA_CTRL() | PC_PSR);	
}



/*------------------------------------------------------------------
 * Func : rtdpc_card_status
 *
 * Desc : check the card status
 *
 * Parm : slot   : which slot to control 
 *         
 * Retn : 1 : card exists
 *        0 : card doesn't exist
 *------------------------------------------------------------------*/ 
int rtdpc_card_status(unsigned char slot)
{
    unsigned long PRES[2]   = {PC_PRES1, PC_PRES2};                
    return (slot <=2 && (GET_PCMCIA_STS() & PRES[slot])) ? 1 : 0;
}



/*------------------------------------------------------------------
 * Func : rtdpc_port_enable
 *
 * Desc : enable / disable mars pcmcia slot
 *
 * Parm : slot   : which slot to control
 *        enable : enable or disable
 *         
 * Retn : R_ERR_SUCCESS / R_ERR_FAILED
 *------------------------------------------------------------------*/ 
int rtdpc_port_enable(unsigned char slot, unsigned char enable)
{    
    unsigned long PCR_OE[2] = {PC_PCR1_OE, PC_PCR2_OE};
    
    if (slot >= 2)
        return R_ERR_FAILED;

    if (enable)
    {
        if (!rtdpc_card_status(slot))
        {
            pcmcia_warning("enable card %d failed, no card detected\n", slot);        
            return R_ERR_FAILED;
        }
        
        pcmcia_info("Card %d Enabled\n", slot);                    
                
        SET_PCMCIA_CTRL(GET_PCMCIA_CTRL() | PCR_OE[slot]);                                  
    }
    else
    {
        pcmcia_info("Card %d Disabled\n", slot);        
        SET_PCMCIA_CTRL(GET_PCMCIA_CTRL() & ~PCR_OE[slot]);
    }

    return R_ERR_SUCCESS;
}


/*------------------------------------------------------------------
 * Func : rtdpc_dump_cis
 *
 * Desc : dump card information structure (for debug only)
 *
 * Parm : slot   : which slot to dump cis structure
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/   
void rtdpc_dump_cis(UINT32 slot)
{
    int i;
    printk("\n--------------------------------");
    printk("\n ADDR |  0  2  4  6  8  A  C  E");
    printk("\n--------------------------------");
	for (i=0;i<256;i+=2)
	{
        if (!(i& 0xF))
            printk("\n  %02x  | ", i);	
            
		printk("%02x ", (unsigned char) rtdpc_readAttrMem(i, slot));
	}
	printk("\n----------------------------------\n");
}


    

/*------------------------------------------------------------------
 * Func : rtdpc_hw_reset
 *
 * Desc : hardware reset the pcmcia interface
 *
 * Parm : slot   : which slot to reset
 *         
 * Retn : one byte data
 *------------------------------------------------------------------*/   
void rtdpc_hw_reset(UINT32 slot)
{               
    unsigned long timeout;    
    unsigned long PRES[2] = {PC_PRES1, PC_PRES2};
    unsigned long PCR[2]  = {PC_PCR1,  PC_PCR2};
    unsigned long PII[2]  = {PC_PII1,  PC_PII2};

    
    if (slot >=2)
    {
        pcmcia_warning("hardware reset pcmcia (%d) failed - invalid slot number\n", slot);        
        return ;
    }
    
    if (!(GET_PCMCIA_STS() & PRES[slot]))
    {
        pcmcia_warning("hardware reset pcmcia (%d) failed - no card detected\n", slot);        
        return ;
    }
          
    SET_PCMCIA_STS(PII[slot]);
    
    timeout = jiffies + RESET_TIME;

    // Start Reset
    SET_PCMCIA_CTRL(GET_PCMCIA_CTRL() | PCR[slot]);
    
    while (GET_PCMCIA_CTRL() & PCR[slot]);
    {
        if (time_after(jiffies, timeout)) 
        {
            pcmcia_warning("hardware reset pcmcia (%d) failed - timeout\n", slot);
            return ;
        }        
    }        

    // wait for request
    while (GET_PCMCIA_STS()& PII[slot]) 
    {        
        if (time_after(jiffies, timeout)) 
        {
            pcmcia_warning("hardware reset pcmcia (%d) failed - timeout\n", slot);
            return ;
        }    
    }
    
    SET_PCMCIA_STS(PII[slot]);      // clear the PC_PIIx bit    
	
	pcmcia_info("hardware reset pcmcia (%d) successed\n", slot);

    //rtdpc_dump_cis(slot);
}




/*------------------------------------------------------------------
 * Func : rtdpc_do_cmd
 *
 * Desc : do read/write command via pcmcia interface
 *
 * Parm : cmd   : command to issue 
 *        slot   : which slot to do command
 *         
 * Retn : one byte data
 *------------------------------------------------------------------*/  
ErrCode rtdpc_do_cmd(unsigned long cmd, unsigned char slot)
{    
    unsigned long PC_CE1[2]   = {PC_CE1_CARD1, PC_CE1_CARD2};
    unsigned long PC_PCR_OE[2]= {PC_PCR1_OE,  PC_PCR2_OE};
    unsigned long ACCESSS_INT = (PC_AFI | PC_APFI);
    int ret = PCMCIA_ACCESS_OK;   
    
    if (slot >=2)
    {
        pcmcia_warning("access pcmcia failed - invalid slot number (%d)\n", slot);        
        return R_ERR_FAILED;
    }

    if (!(GET_PCMCIA_CTRL() & PC_PCR_OE[slot]))
    {
        pcmcia_warning("access pcmcia failed - no card exists!!!\n");
        return R_ERR_FAILED;
    }
                                
    wait_event_interruptible(pcmcia_wait, runing_state==PCMCIA_IDEL);
        
    SET_PCMCIA_STS(ACCESSS_INT);
    wmb();
    SET_PCMCIA_CTRL(GET_PCMCIA_CTRL() | PC_CE1[slot] | ACCESSS_INT);    // enable card 
    wmb();
    SET_PCMCIA_CMDFF(cmd);
    
    runing_state = PCMCIA_RUNING;             
    
    wait_event_interruptible_timeout(pcmcia_wait, runing_state!=PCMCIA_RUNING, ACCESS_TIME_OUT);
    		            
    switch(runing_state)
    {
    case PCMCIA_ACCESS_OK:
        ret = R_ERR_SUCCESS;
        break;
        
    case PCMCIA_ACCESS_FAIL:
        pcmcia_warning("access pcmcia failed - Access Failed!!!\n");
        ret = R_ERR_FAILED;
        break;
        
    case PCMCIA_RUNING:
        pcmcia_warning("access pcmcia failed - Access Time Out!!!\n");
        ret = R_ERR_FAILED;
        SET_PCMCIA_STS(ACCESSS_INT);
        rtdpc_reset(); 
        break;    
    }   
    runing_state = PCMCIA_IDEL;    
        
    SET_PCMCIA_CTRL(GET_PCMCIA_CTRL() & ~ACCESSS_INT);    	
    wmb();
    udelay(1000);    
    return ret;
}



/*------------------------------------------------------------------
 * Func : rtdpc_readMem
 *
 * Desc : read memory via pcmcia interface
 *
 * Parm : addr   : memory address
 *        mode   : target memory space
 *              1 : attribute memory
 *              0 : io memory 
 *        slot   : which slot to read
 *         
 * Retn : one byte data
 *------------------------------------------------------------------*/  
UINT8 rtdpc_readMem(UINT32 addr, UINT32 mode, UINT32 slot)
{	
    unsigned long cmd = PC_CT_READ | PC_AT(mode) | PC_PA(addr);   
    return (rtdpc_do_cmd(cmd, slot)==R_ERR_SUCCESS) ? (GET_PCMCIA_CMDFF() & 0xFF) : 0;     
}




/*------------------------------------------------------------------
 * Func : rtdpc_writeMem
 *
 * Desc : write memory via pcmcia interface
 *
 * Parm : addr   : memory address
 *        data   : data to be written
 *        mode   : target memory space
 *              1 : attribute memory
 *              0 : io memory 
 *        slot   : which slot to write
 *         
 * Retn : one byte data
 *------------------------------------------------------------------*/   
ErrCode rtdpc_writeMem(UINT32 addr, UINT8 data, UINT32 mode, UINT32 slot)
{    
	return rtdpc_do_cmd(PC_CT_WRITE | PC_AT(mode) | PC_PA(addr) | PC_DF(data), slot);
}




/*------------------------------------------------------------------
 * Func : rtdpc_registerCiIntHandler
 *
 * Desc : register a CI Interrupt handler
 *
 * Parm : p_isr : ci handler to be registered 
 *         
 * Retn : R_ERR_SUCCESS / R_ERR_FAILED
 *------------------------------------------------------------------*/ 
ErrCode rtdpc_registerCiIntHandler(ci_int_handler* p_isr)
{
    int i;
    
    if (p_ci_handler || !p_isr)             
        return R_ERR_FAILED;        
    
    p_ci_handler = p_isr;
    
    for (i=0; i<2; i++)                
    {
        if (rtdpc_card_status(i))
        {
            pcmcia_info("Card %d Inserted\n", i);
            rtdpc_port_enable(i, 1);
        
            if (p_ci_handler->ci_camchange_irq)
                p_ci_handler->ci_camchange_irq(i, DVB_CA_EN50221_CAMCHANGE_INSERTED);
        }        
    }    
    return R_ERR_SUCCESS;                
}



/*------------------------------------------------------------------
 * Func : rtdpc_unregisterCiIntHandler
 *
 * Desc : unregister a CI Interrupt handler
 *
 * Parm : p_isr : ci handler to be unregistered 
 *         
 * Retn : R_ERR_SUCCESS / R_ERR_FAILED
 *------------------------------------------------------------------*/ 
ErrCode rtdpc_unregisterCiIntHandler(ci_int_handler* p_isr)
{
    if (p_ci_handler==p_isr) {
        p_ci_handler = NULL;        
        return R_ERR_SUCCESS;
    }
    
    return R_ERR_FAILED;
}



/*------------------------------------------------------------------
 * Func : rtdpc_isr
 *
 * Desc : isr for mars pcmcia module
 *
 * Parm : irq    : which irq to use
 *        dev_id : device id
 *        regs   : 
 *         
 * Retn : IRQ_HANDLED / IRQ_NONE
 *------------------------------------------------------------------*/ 
irqreturn_t rtdpc_isr(INT32 irq, void *dev_id, struct pt_regs *regs)
{	        
    unsigned long event = GET_PCMCIA_STS();
    
	if (!(GET_MIS_ISR() & MARS_PCMCIA_INT))
        return IRQ_NONE;
	
	//----------------------------------------------------
	// CARD1 Insert Remove	                    	    	    	
	//----------------------------------------------------
    if (event & (PC_PCII1 | PC_PCRI1))
    {
        if (rtdpc_card_status(0)) 
        {
            pcmcia_info("Card 0 Inserted\n");
            rtdpc_port_enable(0, 1);           
            
            if (p_ci_handler && p_ci_handler->ci_camchange_irq)
                p_ci_handler->ci_camchange_irq(0, DVB_CA_EN50221_CAMCHANGE_INSERTED);     
        }
        else
        {
            pcmcia_info("Card 0 Removed\n");
            rtdpc_port_enable(0, 0);
            
            if (p_ci_handler && p_ci_handler->ci_camchange_irq)
                p_ci_handler->ci_camchange_irq(0, DVB_CA_EN50221_CAMCHANGE_REMOVED);     
        }

    }            
    
    //----------------------------------------------------
	// CARD2 Insert Remove	                    	    	    	
	//----------------------------------------------------    
	if (event & (PC_PCII2|PC_PCRI2))
    {
        if (rtdpc_card_status(1))
        {
            pcmcia_info("Card 1 Inserted\n");
            rtdpc_port_enable(1, 1);

            if (p_ci_handler && p_ci_handler->ci_camchange_irq)
                p_ci_handler->ci_camchange_irq(1, DVB_CA_EN50221_CAMCHANGE_INSERTED);
        }
        else
        {
            pcmcia_info("Card 1 Removed\n");
            rtdpc_port_enable(1, 0);        
            if (p_ci_handler && p_ci_handler->ci_camchange_irq)
                p_ci_handler->ci_camchange_irq(1, DVB_CA_EN50221_CAMCHANGE_REMOVED);
        }                
    }        
    
    //----------------------------------------------------
	// PCMCIA ACCESS
	//----------------------------------------------------        
    if (event & PC_APFI)
    {
        runing_state = PCMCIA_ACCESS_OK;		    
	    wake_up_interruptible(&pcmcia_wait);
    }

    if (event & PC_AFI) 
    {
	    runing_state = PCMCIA_ACCESS_FAIL;
	    rtdpc_reset();
	    wake_up_interruptible(&pcmcia_wait);
    }
    
    //----------------------------------------------------
	// PCMCIA INTR#
	//----------------------------------------------------        
	if (event & PC_PII1) 
	{
	    dprintk("Card1 IRQ#1 interrupt\n");
	    
	    if (p_ci_handler && p_ci_handler->ci_frda_irq)
		    p_ci_handler->ci_frda_irq(0);		
	}
	
	if (event & PC_PII2) 
	{
	    dprintk("Card2 IRQ#2 interrupt\n");
	    
	    if (p_ci_handler && p_ci_handler->ci_frda_irq)
		    p_ci_handler->ci_frda_irq(1);		
	}	            
    
    SET_PCMCIA_STS(0xff);           // clear PCMCIA Interrupt    
    SET_MIS_ISR(MARS_PCMCIA_INT);	// clear MIS Interrupt                        
	    
    return IRQ_HANDLED;
}



/*------------------------------------------------------------------
 * Func : mars_pcmcia_module_init
 *
 * Desc : init mars pcmcia module
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static int __init mars_pcmcia_module_init(void)
{       
    if (rtdpc_probe()==R_ERR_SUCCESS)
    {		    
        /* using default value
        SET_PCMCIA_AMTC(AMTC_CFG);	
	    SET_PCMCIA_IOMTC(IOMTC_CFG);
	    SET_PCMCIA_MATC(MATC_CFG);
	    */
        SET_PCMCIA_CTRL(PC_PSR);	
        SET_PCMCIA_STS(0xFF);		    

	    if (request_irq(MIS_IRQ, rtdpc_isr, SA_INTERRUPT | SA_SHIRQ, "pcmcia", (void *) PCM_DEV_ID)<0)
	    {
	        pcmcia_warning("init pcmcia module failed, request IRQ #%d failed\n", MIS_IRQ);
		    return R_ERR_IRQ_NOT_AVALIABLE;
        }
        
	    SET_PCMCIA_CTRL(PC_INTMASK3);
           
        return 0;
    }        

    return -ENODEV;
}



/*------------------------------------------------------------------
 * Func : mars_pcmcia_module_exit
 *
 * Desc : remove mars pcmcia module
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static void __exit mars_pcmcia_module_exit(void)
{
    free_irq(MIS_IRQ, (void *)(PCM_DEV_ID));        
    SET_PCMCIA_CTRL(0);
}

module_init(mars_pcmcia_module_init);
module_exit(mars_pcmcia_module_exit);
EXPORT_SYMBOL(rtdpc_writeMem);
EXPORT_SYMBOL(rtdpc_readMem);
EXPORT_SYMBOL(rtdpc_registerCiIntHandler);
EXPORT_SYMBOL(rtdpc_unregisterCiIntHandler);
