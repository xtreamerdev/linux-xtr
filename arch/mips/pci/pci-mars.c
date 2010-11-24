/* ------------------------------------------------------------------------- 
 * pci-mars.c  mars pci-dvr bridge driver
 * ------------------------------------------------------------------------- 
 * Copyright (C) 2008 Kevin Wang <kevin_wang@realtek.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    
 * ------------------------------------------------------------------------- 
 * Update List :
 * ------------------------------------------------------------------------- 
 *     1.1     |   20080908    | register irq at mars-pci initialization stage
 * -------------------------------------------------------------------------*/ 
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include "pci-mars.h"

extern struct pci_ops mars_pci_ops;

static struct resource py_mem_resource = 
{
    "Realtek Mars PCI MEM", 0x12000000UL, 0x17ffffffUL, IORESOURCE_MEM //96MB
};

/*
 * PMON really reserves 16MB of I/O port space but that's stupid, nothing
 * needs that much since allocations are limited to 256 bytes per device
 * anyway.  So we just claim 64kB here.
 */
static struct resource py_io_resource = {
    //physical address = io addr + VENUS_IO_PORT_BASE (0x18010000)
    "Realtek Mars PCI IO", 0x00030000UL, 0x0003ffffUL, IORESOURCE_IO,  //64KB
};

static struct pci_controller py_controller = 
{
    .pci_ops        = &mars_pci_ops,
    .mem_resource   = &py_mem_resource,
    .mem_offset     = 0x00000000UL,
    .io_resource    = &py_io_resource,
    .io_offset      = 0x00030000UL
};


static char ioremap_failed[] __initdata = "Could not ioremap I/O port range";

#define RTL_W32(addr, val)      (writel((u32)(val), (volatile void __iomem*) (addr)))
#define RTL_R32(addr)           ((u32) readl((volatile void __iomem*) (addr)))



/*
 * Handle errors from the bridge.  This includes master and target aborts,
 * various command and address errors, and the interrupt test.  This gets
 * registered on the bridge error irq.  It's conceivable that some of these
 * conditions warrant a panic.  Anybody care to say which ones?
 */
static struct {
	unsigned int err_reg;
	unsigned int err_cnt;
	char err_msg[64];
} master_abort_msg[] = 
{ {DVR_DIR_ST, RERROR_ST,     "master abort caused by direct mapping read"},
  {DVR_DIR_ST, WERROR_ST,     "master abort caused by direct mapping write"},
  {DVR_CFG_ST, ERROR_ST,      "master abort caused by indirect configuration"},
  {DVR_MIO_ST, ERROR_ST,      "master abort caused by indirect mapping read or write"},
  {DVR_DMA_ST, PCI_ERROR_ST,  "master abort caused by master dma mapping: signle error"},
  {DVR_DMA_ST, DESC_ERROR_ST, "master abort caused by master dma mapping: descriptor error"},
  {0, 0, 0},
};


/*-------------------------------------------------------
 * Func  : pcibios_map_irq
 * 
 * Desc  : This function be invoked when the pci core about 
 *         to associates an irq to a pci device
 *
 * Param : dev  : specified pci device
 *         slot : which slot the pci device at
 *         pin  : which INT# pin does the pci device used  
 *
 * Retn  : associated irq
 *------------------------------------------------------*/
int __init 
pcibios_map_irq(
    struct pci_dev*         dev, 
    u8                      slot, 
    u8                      pin
    )
{
	if (pin==0)
		return -1;

	return 2;			/* Everything goes to one irq bit */
}



/*-------------------------------------------------------
 * Func  : pcibios_map_irq
 * 
 * Desc  : platform specific device initialization.
 *         this function will be invoked when PCI driver calling
 *         pc_enable_device()
 *
 * Param : dev  : device to be init
 *
 * Retn  : 0 for success
 *------------------------------------------------------*/
int pcibios_plat_dev_init(
    struct pci_dev*         dev
    )
{
    // FIXME : Do platform specific device initialization at pci_enable_device() time
	return 0;
}


/*-------------------------------------------------------
 * Func  : mars_pci_error_handler  
 *
 * Desc  : Interrupt Handler of Mars PCI Bridge
 *
 * Param : irq  : irq num
 *         dev  : handle of device
 *         reg  :
 * 
 * Retn  : IRQ_HANDLED / IRQ_NONE
 *------------------------------------------------------*/
static irqreturn_t 
mars_pci_error_handler(
    int                     irq, 
    void*                   dev,
    struct pt_regs*         regs
    )
{
	struct pci_dev *pdev;
	unsigned int dvr_gnr_int = readl(DVR_GNR_INT);
	unsigned int pci_gnr_int = readl(PCI_GNR_INT);
	unsigned int err_reg, err_cnt;
	unsigned char err = 0, m_abort = 0, t_abort = 0;

	struct resource *res;
	struct resource_list head, *list, *tmp;
	int idx;
	int i = 0;
			
	//printk("[PCI IRQ] DVR=%p, PCI=%p\n", dvr_gnr_int, pci_gnr_int);	
	
	if (!(dvr_gnr_int & INTP_PCI)) {
//		printk(" not belong to pci-dvr bridge \n");
		return IRQ_NONE;
	}
		
	switch (dvr_gnr_int) 
	{
	case (INTP_RDIR):	
		if (readl(DVR_DIR_ST) & (RERROR_ST))
		{
			err = 1;
			err_reg = DVR_DIR_ST;
			err_cnt = RERROR_ST;
		}
		break;
		
	case (INTP_WDIR):
		if (readl(DVR_DIR_ST) & WERROR_ST)
		{
			err = 1;
			err_reg = DVR_DIR_ST;
			err_cnt = WERROR_ST;
		}
		break;
		
	case (INTP_CFG):
		if (readl(DVR_CFG_ST) & ERROR_ST)
		{
			err = 1;
			err_reg = DVR_CFG_ST;
			err_cnt = ERROR_ST;
		}
		break;
		
	case (INTP_MIO):
		if (readl(DVR_MIO_ST) & ERROR_ST)
		{
			err = 1;
			err_reg = DVR_CFG_ST;
			err_cnt = ERROR_ST;
		}
		break;
		
	case (INTP_DMA):
	
		if (readl(DVR_DMA_ST) & PCI_ERROR_ST)
		{
			err = 1;
			err_reg = DVR_DMA_ST;
			err_cnt = PCI_ERROR_ST;
		}
		else if (readl(DVR_DMA_ST) & DESC_ERROR_ST)
		{
			err = 1;
			err_reg = DVR_DMA_ST;
			err_cnt = DESC_ERROR_ST;
		}
		break;
		
	default:  //include INTP_MST, INTP_SLV, INTP_PCI and others
		return IRQ_NONE;
		break;
	}

	if (err == 1)
	{
	    //printk("error with %x = %x\n", err_reg, err_cnt);
		
		if (readl(CFG(04)) & REC_MASTER_ABORT)
		{
	        int i = 0;
		    m_abort = 1;
					    		
		    while (master_abort_msg[i].err_reg != 0)
			{	
				if ((master_abort_msg[i].err_reg == err_reg) &&
				    (master_abort_msg[i].err_cnt == err_cnt))
				{
					printk( "%s [reg = %x, cnt = %x]\n",
					        master_abort_msg[i].err_msg,
					        master_abort_msg[i].err_reg,
					        master_abort_msg[i].err_cnt);
		            
		            break;
		        }
				
				i++;
			}
		    		    
		    writel(REC_MASTER_ABORT, CFG(04));
		    		    
		    writel(MASTER_ERROR, PCI_GNR_ST);
		}
		else if (readl(CFG(04)) & REC_TARGET_ABORT)
		{
			printk("target abort !!\n");
		    t_abort = 1;
		    		    
		    writel(REC_TARGET_ABORT, CFG(04));
		}
		else
			printk("no error information!!!!!!\n");
				
		writel(err_cnt, err_reg);
				
		//process master abort
		if (m_abort == 1)
		{
			u32 bar_reg;
						
			pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + (i*4), &bar_reg);
						
			for (i = 0; i < 6 ; i++)
			{
#if 1
                pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + (i*4), &bar_reg);
				printk("bar%d = %x\n", i, bar_reg);
#else
				//if BARx content consist to resource we allocated, just skip to next one
				if (bar_reg == pdev->resource[i].start)
					continue;
				else //inconsist!! BARx might be changed
				{
					if (pdev->resource[i].start <= pdev->resource[i].end)
						continue;
					else
					{
						u32 flag = 0x1 & pdev->resource[i].flags;
						printk("!!! device %s BAR%d changed from %x to original %x\n", pci_name(pdev), i, bar_reg, pdev->resource[i].start);
						pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + (i*4), (pdev->resource[i].start | flag));
					}
	
				}
#endif
			}
		}
		else if (t_abort == 1)
		{
			printk("!! do nothing for target abort \n");
			return IRQ_NONE;
		}				
	}

	return IRQ_NONE;
}



/*------------------------------------------
 * Func  : mars_pci_set_sb2_address_translation 
 *
 * Desc  : Set SB2 Address Translaction for PCI Bridge.
 *
 * Param : N/A
 *
 * Retn  : N/A
 *
 * Note  : SCPU should communicate with Mars PCI-Bridge via SB2.
 *         So we have to inform the SB2 which addresses are belong to
 *         the PCI-bridge. 
 *-----------------------------------------*/
static 
void __init 
mars_pci_set_sb2_address_translation()
{
    RTL_W32( PCI_BASE0,  0x12000000);    
    RTL_W32( PCI_MASK0,  0xfe000000);    
    RTL_W32( PCI_TRANS0, 0x12000000);
             
    RTL_W32( PCI_BASE1,  0x14000000);
    RTL_W32( PCI_MASK1,  0xfc000000);
    RTL_W32( PCI_TRANS1, 0x14000000);
             
    RTL_W32( PCI_BASE2,  0x18040000);    
    RTL_W32( PCI_MASK2,  0xffff0000);    
    RTL_W32( PCI_TRANS2, 0x00000000);
             
    RTL_W32( PCI_CTRL,   0x0000017f);
}



/*------------------------------------------
 * Func  : mars_pci_config_space_init 
 *
 * Desc  : Init config space of Mars PCI bridge
 *
 * Param : N/A
 *
 * Retn  : N/A  
 *-----------------------------------------*/
static 
void __init 
mars_pci_config_space_init()
{        
    //config the maps of MARS memory in 64M
    RTL_W32(0xb801711c, 0xf8000000);    
    
    //config PCI-Bridge BAR1 start address: 0xa000_0000(virtual adderss) = 0x0000_0000(physical address)
#if 0//PCI device write back addresses are physical address, so we must set physical address in BAR1
    RTL_W32(0xb8017214, 0xa0000000);            
#else
    RTL_W32(0xb8017214, 0x00000000);    // export out readable address  
#endif
    

#if 0 //we don't need to setup mars pci-bridge BAR' except DCUs'
    //config PCI-Bridge BAR0 physical address (0xb801_7080-0xb801_709c): 32Byte
    RTL_W32(0xb8017210, 0xb8017080);
    //config PCI-Bridge BAR2 physical address (0xb800_0000-0xb803_ffff): 256K
    RTL_W32(0xb8017218, 0xb8000000);
    //config PCI-Bridge BAR3 physical address (0xb100_0000-0xb100_0fff): 4K
    RTL_W32(0xb801721c, 0xb1000000);
#endif    
}


/*------------------------------------------
 * Func  : mars_pci_bus_reset
 *
 * Desc  : reset pci bus
 *
 * Param : N/A
 *
 * Retn  : 0 : success, others failed
 *-----------------------------------------*/
static void mars_pci_bus_reset()
{   
    if ((RTL_R32(0xb8000338) & 0x01))
    {
        printk("[PCI-B] PCI Bus Reset\n");
        
        RTL_W32(0xb8000000, RTL_R32(0xb8000000) & ~0x80000000);
        msleep(100);
        RTL_W32(0xb8000000, RTL_R32(0xb8000000) | 0x80000000);    
    }
    else
        printk("[PCI-B] WARNING, Unable reset bus when PCI Bridge Ack as a device\n");    
}



/*------------------------------------------
 * Func  : mars_pci_dump_config
 *
 * Desc  : dump config space of mars pci bridge
 *
 * Param : N/A
 *
 * Retn  : 0 : success, others failed
 *-----------------------------------------*/
static 
void mars_pci_dump_config()
{       
    int i;   
    u32 val;    
    

    printk(" MARS PCI Bridge Cfg \n");    
    printk("    |  B4  B3  B2  B1  \n");
    printk("----+------------------------\n");
    for (i=0; i<64; i+=4)
    {           
        switch(i)
        {
        case 0x20:
        case 0x24:
        case 0x28:
        case 0x30:
        case 0x34:
        case 0x38:
            printk("%02x  |  --  --  --  --\n", i);                
            continue;
        default:
            val = RTL_R32(CFG(i));
            printk("%02x  |  %02x  %02x  %02x  %02x\n", 
                    i,
                    (val >> 24) & 0xFF,
                    (val >> 16) & 0xFF,
                    (val >> 8) & 0xFF,
                    (val & 0xFF)
                );          
        }
    }
    printk("-----------------------------\n");    
}



/*------------------------------------------
 * Func  : mars_pci_bridge_init
 *
 * Desc  : init Mars PCI-Bridge
 *
 * Param : N/A
 *
 * Retn  : 0 : success, others failed
 *-----------------------------------------*/
static int __init mars_pci_bridge_init()
{        
    unsigned long host_mode_en;    
          
    if ((RTL_R32(0xb800000c) & 0x80)==0)
    {
        printk("[PCI-B] init PCI bridge failed, PCI Clock does not enable\n");      
        return -1;         
    }    
    
    host_mode_en = RTL_R32(PCICK_SEL) & 0x01;
    
    printk("[PCI-B] PCI Bridge Ack as a PCI %s\n", ((host_mode_en) ? "Host" : "Device") );
    
    if (!host_mode_en)  
        return -1;
        
    if (RTL_R32(0xb801A010)!=0x1F) 
    {
        printk("[PCI-B] warning, SB2 timeout not equal to 0x1F, Fix it\n");
        RTL_W32(0xb801A010, 0x1F);
    }
        
    RTL_W32(PFUNC_PCI5, 0x00400000);        // pci clk driving current = 8mA
            
    RTL_W32(DVR_TMP1_REG, 0x0010001c);      // PCI delay chain setting
        
    RTL_W32(DVR_GNR_MODE, 0x10000);         // Enable Slave Mode
            
    RTL_W32(CFG(0x04), 0x06);               // (bus_master, mm_space, io_space) = (0, 1, 0)
            
//    RTL_W32(DVR_GNR_EN, 0x10300);           // enable PCI configuration
    RTL_W32(DVR_GNR_EN, 0x10000);           // enable PCI configuration
    
    // enable DVR-PCI interrupt and Dbus translation(others reading our BAR1)
    RTL_W32(PCI_GNR_EN2, 0x70004);    
    
    mars_pci_config_space_init();      
    
    if (host_mode_en) 
    {      
        mars_pci_bus_reset();                       
        register_pci_controller(&py_controller);               
    }
       
    mars_pci_set_sb2_address_translation();    
       
    mars_pci_dump_config();
    
    request_irq(2, mars_pci_error_handler, SA_SHIRQ, "mars pci-dvr host error handler", &pci_devices);
    
    return 0;
}




arch_initcall(mars_pci_bridge_init);
