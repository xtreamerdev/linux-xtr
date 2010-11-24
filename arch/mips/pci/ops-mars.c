/*
 * Copyright 2003 PMC-Sierra
 * Author: Manish Lachwani (lachwani@pmc-sierra.com)
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED	  ``AS	IS'' AND   ANY	EXPRESS OR IMPLIED
 *  WARRANTIES,	  INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO	EVENT  SHALL   THE AUTHOR  BE	 LIABLE FOR ANY	  DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED	  TO, PROCUREMENT OF  SUBSTITUTE GOODS	OR SERVICES; LOSS OF
 *  USE, DATA,	OR PROFITS; OR	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN	 CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include "pci-mars.h"

static int mars_read_config(struct pci_bus *bus, unsigned int devfn, int reg,
	int size, u32 * val)
{
	uint32_t address, tmp;
	int dev, busno, func;

	// read operation & disable all interrupts
	writel(0, DVR_CFG_EN);
	
	busno = bus->number;
	dev = PCI_SLOT(devfn);
	func = PCI_FUNC(devfn);
	
	address = (busno << 16) | (dev << 11) | (func << 8) |
	          (reg & 0xfc) ;

	writel(0x03, DVR_CFG_ST);	// clear flag bits

#if 0
	printk("busno = %x, dev = %x, func = %X, reg = %x, address = %x\n", busno, dev, func, reg, address);
#endif

	/* start the configuration cycle */
	writel(address, DVR_CFG_ADDR);
	writel(1, DVR_CFG_CT);
	while (readl(DVR_CFG_CT));
	if (readl(DVR_CFG_ST) & 0x02){
		printk("PCI: read %x timeout\n", address);
		return PCIBIOS_DEVICE_NOT_FOUND;
	}
	tmp = readl(DVR_CFG_RDATA);
#if 0 //cylee marked for debug
	printk("R, reg = %x, size = %d, val = %x, ", reg, size, tmp);
#endif

	if (size == 1)
		tmp = (tmp >> ((reg & 3) << 3)) & 0xff;
	else if (size == 2)
		tmp = (tmp >> ((reg & 3) << 3)) & 0xffff;
	
	*val = tmp;

#if 0 //cylee marked for debug
	printk("real val = %x\n", tmp);
#endif

	return PCIBIOS_SUCCESSFUL;
}

static int mars_write_config(struct pci_bus *bus, unsigned int devfn, int reg,
	int size, u32 val)
{
	uint32_t address, tmp, mask;
	int dev, busno, func;
	int i = 0;

	busno = bus->number;
	dev = PCI_SLOT(devfn);
	func = PCI_FUNC(devfn);
	
	// write operation & byte enable
	switch(size){
	case 1: mask = 0x1;
		break;
	case 2: mask = 0x3;
		break;
	case 4: mask = 0xf;	
		break;	
	}
	//setup byte_enable_field in DVR_CFG_EN(0x1801_7010)
	mask = (mask << (reg & 0x3)) << 4; 
	//write DVR_CFG_EN with write and byte_enable 
	writel(mask | 0x3, DVR_CFG_EN);
	
	//address = (busno << 16) | (dev << 11) | (func << 8) |
	//	(reg & 0xfc) | 0x80000000;
	
	address = (busno << 16) | (dev << 11) | (func << 8) |
	       	(reg & 0xfc);
#if 0 //cylee marked for debug
	printk("W, bus# = %d, dev = %d, reg = %x, size = %d, val = %x, ", busno, dev,reg, size, val);
#endif
	writel(0x03, DVR_CFG_ST);		// clear flag bits
	
	/* start the configuration cycle */
	writel(address, DVR_CFG_ADDR);
	
	/* write the data */
	writel(val<<(8*(reg & 0x3)), DVR_CFG_WDATA);
		
	writel(1, DVR_CFG_CT);
	while (readl(DVR_CFG_CT));
	
	if (readl(DVR_CFG_ST) & 0x02){
		printk("PCI: write %x timeout\n", address);
		return PCIBIOS_DEVICE_NOT_FOUND;
	}
#if 0 //cylee marked for debug
	printk("real writing = %x\n",val<<(8*(reg & 0x3)));
#endif

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops mars_pci_ops = {
	mars_read_config,
	mars_write_config,
};
