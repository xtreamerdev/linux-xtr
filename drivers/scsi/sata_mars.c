/*
 *  sata_sis.c - Realtek SATA
 *
 *  Maintained by:  Frank Ting
 *  		    Please ALWAYS copy linux-ide@vger.kernel.org
 *		    on emails.
 *
 *  Copyright 2007 Frank Ting
 *
 *  The contents of this file are subject to the Open
 *  Software License version 1.1 that can be found at
 *  http://www.opensource.org/licenses/osl-1.1.txt and is included herein
 *  by reference.
 *
 *  Alternatively, the contents of this file may be used under the terms
 *  of the GNU General Public License version 2 (the "GPL") as distributed
 *  in the kernel source COPYING file, in which case the provisions of
 *  the GPL are applicable instead of the above.  If you wish to allow
 *  the use of your version of this file only under the terms of the
 *  GPL and not to allow others to use your version of this file under
 *  the OSL, indicate your decision by deleting the provisions above and
 *  replace them with the notice and other provisions required by the GPL.
 *  If you do not delete the provisions above, a recipient may use your
 *  version of this file under either the OSL or the GPL.
 *
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include "scsi.h"
#include <scsi/scsi_host.h>
#include <linux/libata.h>

#define DRV_NAME	"sata_mars"
#define DRV_VERSION	"0.1"

#define	MARS_SCR_BASE		  0xb8012b24  /* sata0 phy SCR registers */
#define	MARS_SATA1_OFS		0x100, 			/* offset from sata0->sata1 phy regs */

#define SATA_DEBUG
#ifdef	SATA_DEBUG
#define	satainfo(fmt, args...) printk(KERN_INFO "sata info: " fmt, ## args)
#else
#define satainfo(fmt, args...)
#endif

struct ata_probe_ent *ata_probe_ent_alloc(struct device *dev, struct ata_port_info *port);

static u32 mars_scr_read (struct ata_port *ap, unsigned int sc_reg);
static void mars_scr_write (struct ata_port *ap, unsigned int sc_reg, u32 val);

static int mars_1283_probe(struct device *dev);
static int mars_1283_remove(struct device *dev);


static int mars_bus_match(struct device *dev, struct device_driver *drv)
{
	satainfo("mars_bus_match\n");

	return 1;
}

struct bus_type mars_bus_type = {
		.name  = "MARS_BUS",
		.match = mars_bus_match,
};

void mars_sb1_release(struct device * dev)
{
		printk(KERN_INFO "sata_mars: mars_sb1_release() called\n");
}

static struct device mars_sb1 = {
	.bus_id		= "MARS_BUS",
	.release	= mars_sb1_release,
	.bus			= &mars_bus_type,
};



static struct device_driver mars_driver = {
		.name		= DRV_NAME,
		.bus		= &mars_bus_type,
		.probe		= mars_1283_probe,
		.remove		= mars_1283_remove,

};

static Scsi_Host_Template mars_sht = {
	.module			= THIS_MODULE,
	.name			= DRV_NAME,
	.ioctl			= ata_scsi_ioctl,
	.queuecommand		= ata_scsi_queuecmd,
	.eh_strategy_handler	= ata_scsi_error,
	.can_queue		= ATA_DEF_QUEUE,
	.this_id		= ATA_SHT_THIS_ID,
	.sg_tablesize		= ATA_MAX_PRD,
	.max_sectors		= ATA_MAX_SECTORS,
	.cmd_per_lun		= ATA_SHT_CMD_PER_LUN,
	.emulated		= ATA_SHT_EMULATED,
	.use_clustering		= ATA_SHT_USE_CLUSTERING,
	.proc_name		= DRV_NAME,
	.dma_boundary		= ATA_DMA_BOUNDARY,
	.slave_configure	= ata_scsi_slave_config,
	.bios_param		= ata_std_bios_param,
	.ordered_flush		= 1,
};

static struct ata_port_operations mars_ops = {
	.port_disable		= ata_port_disable,
	.tf_load		= ata_tf_load,
	.tf_read		= ata_tf_read,
	.check_status		= ata_check_status,
	.exec_command		= ata_exec_command,
	.dev_select		= ata_std_dev_select,
	.phy_reset		= sata_phy_reset,
	.bmdma_setup            = ata_bmdma_setup,
	.bmdma_start            = ata_bmdma_start,
	.bmdma_stop		= ata_bmdma_stop,
	.bmdma_status		= ata_bmdma_status,
	.qc_prep		= ata_qc_prep,
	.qc_issue		= ata_qc_issue_prot,
	.eng_timeout		= ata_eng_timeout,
	.irq_handler		= ata_interrupt,
	.irq_clear		= ata_bmdma_irq_clear,
	.scr_read			= mars_scr_read,
	.scr_write		= mars_scr_write,
	.port_start		= ata_port_start,
	.port_stop		= ata_port_stop,
	.host_stop		= ata_host_stop,
};

static struct ata_port_info mars_port_info = {
	.sht		= &mars_sht,
	//.host_flags	= ATA_FLAG_SATA | ATA_FLAG_SATA_RESET |
	//		  ATA_FLAG_NO_LEGACY,
	.host_flags	= ATA_FLAG_SATA | ATA_FLAG_NO_LEGACY |
			  ATA_FLAG_SATA_RESET | ATA_FLAG_MMIO |
			  ATA_FLAG_PIO_DMA,
		  
	.pio_mask	= 0x1f,
	.mwdma_mask	= 0x7,
	.udma_mask	= 0x7f,
	.port_ops	= &mars_ops,
};


MODULE_AUTHOR("Frank Ting");
MODULE_DESCRIPTION("low-level driver for Realtek SATA controller");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);


static u32 mars_scr_read (struct ata_port *ap, unsigned int sc_reg)
{
	if (sc_reg > SCR_CONTROL)
		return 0xffffffffU;

	return inl(ap->ioaddr.scr_addr + (sc_reg * 4));
}

static void mars_scr_write (struct ata_port *ap, unsigned int sc_reg, u32 val)
{
	if (sc_reg > SCR_CONTROL)
		return;


	outl(val, ap->ioaddr.scr_addr + (sc_reg * 4));
}



static int mars_1283_probe(struct device *dev)
{
	struct ata_probe_ent *probe_ent = NULL;
	int rc;

	satainfo("mars_1283_probe\n");
		
	probe_ent = ata_probe_ent_alloc(dev, &mars_port_info);

	if (!probe_ent) {
		rc = -ENOMEM;
		return rc;
	}
	
	probe_ent->irq_flags = SA_SHIRQ;

	ata_std_ports(&probe_ent->port[0]);
	ata_std_ports(&probe_ent->port[1]);
	
	/* FIXME: check ata_device_add return value */
	ata_device_add(probe_ent);
	kfree(probe_ent);

	return 0;

}
 
static int mars_1283_remove(struct device *dev)
{
	struct ata_host_set *host_set = dev_get_drvdata(dev);
	struct ata_port *ap;
	unsigned int i;

	satainfo("mars_bus_remove\n");
	
	for (i = 0; i < host_set->n_ports; i++) {
		ap = host_set->ports[i];

		scsi_remove_host(ap->host);
	}

	free_irq(host_set->irq, host_set);

	for (i = 0; i < host_set->n_ports; i++) {
		ap = host_set->ports[i];

		ata_scsi_release(ap->host);


		scsi_host_put(ap->host);
	}

	if (host_set->ops->host_stop)
		host_set->ops->host_stop(host_set);

	kfree(host_set);

	dev_set_drvdata(dev, NULL);
	
	return(0);
}



static int __init mars_init(void)
{
	int error;
	
	error = bus_register(&mars_bus_type);
	if (error) return error;
	else {
		device_register(&mars_sb1);
		error = driver_register(&mars_driver);
		return error;	
	}
	
}

static void __exit mars_exit(void)
{
	driver_unregister(&mars_driver);
	device_unregister(&mars_sb1);
	bus_unregister(&mars_bus_type);
}

module_init(mars_init);
module_exit(mars_exit);

