/*
 *
 * BRIEF MODULE DESCRIPTION
 *      Board specific pci fixups for the Toshiba rbtx4927
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *              ppopov@mvista.com or source@mvista.com
 *
 * Copyright (C) 2000-2001 Toshiba Corporation 
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
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
#include <linux/init.h>

#include <asm/tx4927/tx4927.h>
#include <asm/tx4927/tx4927_pci.h>

#undef  DEBUG
#ifdef  DEBUG
#define DBG(x...)       printk(x)
#else
#define DBG(x...)
#endif

/* look up table for backplane pci irq for slots 17-20 by pin # */
static unsigned char backplane_pci_irq[4][4] = {
	/* PJ6 SLOT:  17, PIN: 1 */ {TX4927_IRQ_IOC_PCIA,
				     /* PJ6 SLOT:  17, PIN: 2 */
				     TX4927_IRQ_IOC_PCIB,
				     /* PJ6 SLOT:  17, PIN: 3 */
				     TX4927_IRQ_IOC_PCIC,
				     /* PJ6 SLOT:  17, PIN: 4 */
				     TX4927_IRQ_IOC_PCID},
	/* SB  SLOT:  18, PIN: 1 */ {TX4927_IRQ_IOC_PCIB,
				     /* SB  SLOT:  18, PIN: 2 */
				     TX4927_IRQ_IOC_PCIC,
				     /* SB  SLOT:  18, PIN: 3 */
				     TX4927_IRQ_IOC_PCID,
				     /* SB  SLOT:  18, PIN: 4 */
				     TX4927_IRQ_IOC_PCIA},
	/* PJ5 SLOT:  19, PIN: 1 */ {TX4927_IRQ_IOC_PCIC,
				     /* PJ5 SLOT:  19, PIN: 2 */
				     TX4927_IRQ_IOC_PCID,
				     /* PJ5 SLOT:  19, PIN: 3 */
				     TX4927_IRQ_IOC_PCIA,
				     /* PJ5 SLOT:  19, PIN: 4 */
				     TX4927_IRQ_IOC_PCIB},
	/* PJ4 SLOT:  20, PIN: 1 */ {TX4927_IRQ_IOC_PCID,
				     /* PJ4 SLOT:  20, PIN: 2 */
				     TX4927_IRQ_IOC_PCIA,
				     /* PJ4 SLOT:  20, PIN: 3 */
				     TX4927_IRQ_IOC_PCIB,
				     /* PJ4 SLOT:  20, PIN: 4 */
				     TX4927_IRQ_IOC_PCIC}
};

int pci_get_irq(struct pci_dev *dev, int pin)
{
	unsigned char irq = pin;

	DBG("pci_get_irq: pin is %d\n", pin);
	/* IRQ rotation */
	irq--;			/* 0-3 */
	if (dev->bus->parent == NULL &&
	    PCI_SLOT(dev->devfn) == TX4927_PCIC_IDSEL_AD_TO_SLOT(23)) {
		printk("Onboard PCI_SLOT(dev->devfn) is %d\n",
		       PCI_SLOT(dev->devfn));
		/* IDSEL=A23 is tx4927 onboard pci slot */
		irq = (irq + PCI_SLOT(dev->devfn)) % 4;
		irq++;		/* 1-4 */
		DBG("irq is now %d\n", irq);

		switch (irq) {
		case 1:
			irq = TX4927_IRQ_IOC_PCIA;
			break;
		case 2:
			irq = TX4927_IRQ_IOC_PCIB;
			break;
		case 3:
			irq = TX4927_IRQ_IOC_PCIC;
			break;
		case 4:
			irq = TX4927_IRQ_IOC_PCID;
			break;
		}
	} else {
		/* PCI Backplane */
		DBG("PCI Backplane PCI_SLOT(dev->devfn) is %d\n",
		    PCI_SLOT(dev->devfn));
		irq = backplane_pci_irq[PCI_SLOT(dev->devfn) - 17][irq];
	}
	DBG("assigned irq %d\n", irq);
	return irq;
}


#ifdef  TX4927_SUPPORT_PCI_66
extern int tx4927_pci66;
extern void tx4927_pci66_setup(void);
#endif
extern void tx4927_pci_setup(void);

#ifdef  TX4927_SUPPORT_PCI_66
int tx4927_pci66_check(void)
{
	struct pci_dev *dev;
	unsigned short stat;
	int cap66 = 1;

	if (tx4927_pci66 < 0)
		return 0;

	/* check 66MHz capability */
	pci_for_each_dev(dev) {
		if (cap66) {
			pci_read_config_word(dev, PCI_STATUS, &stat);
			if (!(stat & PCI_STATUS_66MHZ)) {
				printk(KERN_INFO
				       "PCI: %02x:%02x not 66MHz capable.\n",
				       dev->bus->number, dev->devfn);
				cap66 = 0;
			}
		}
	}
	return cap66;
}
#endif

/*
 * This is dead, rotten code, needs to be rewritten -- Ralf
 */
int __init pcibios_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	unsigned char irq;

#ifdef  TX4927_SUPPORT_PCI_66	/* Has no f*cking biz here  */
	if (tx4927_pci66_check()) {
		tx4927_pci66_setup();
		tx4927_pci_setup();	/* Reinitialize PCIC */
	}
#endif

	if (dev->vendor == PCI_VENDOR_ID_EFAR &&
	    dev->device == PCI_DEVICE_ID_EFAR_SLC90E66_1)
		irq = 14;
	else
		irq = 0;

	return irq;
}

/* Do platform specific device initialization at pci_enable_device() time */
int pcibios_plat_dev_init(struct pci_dev *dev)
{
	return 0;
}
