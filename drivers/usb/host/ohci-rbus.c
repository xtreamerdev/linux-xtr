/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * 
 * [ Initialisation is based on Linus'  ]
 * [ uhci code and gregs ohci fragments ]
 * [ (C) Copyright 1999 Linus Torvalds  ]
 * [ (C) Copyright 1999 Gregory P. Smith]
 * 
 * PCI Bus Glue
 *
 * This file is licenced under the GPL.
 */
 
#ifdef CONFIG_PMAC_PBOOK
#include <asm/machdep.h>
#include <asm/pmac_feature.h>
#include <asm/pci-bridge.h>
#include <asm/prom.h>
#ifndef CONFIG_PM
#	define CONFIG_PM
#endif
#endif

/*-------------------------------------------------------------------------*/

static int
ohci_pci_reset (struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);

	ohci_hcd_init (ohci);
	return ohci_init (ohci);
}

static int __devinit
ohci_pci_start (struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int		ret;

	/* NOTE: there may have already been a first reset, to
	 * keep bios/smm irqs from making trouble
	 */
	if ((ret = ohci_run (ohci)) < 0) {
		ohci_err (ohci, "can't start\n");
		ohci_stop (hcd);
		return ret;
	}
	return 0;
}

#if     defined(CONFIG_PM)

static int ohci_rbus_suspend (struct usb_hcd *hcd, pm_message_t message)
{
	struct ohci_hcd		*ohci = hcd_to_ohci (hcd);

	/* suspend root hub, hoping it keeps power during suspend */
	if (time_before (jiffies, ohci->next_statechange))
		msleep (100);

#ifdef	CONFIG_USB_SUSPEND
	(void) usb_suspend_device (hcd->self.root_hub, message);
#else
	usb_lock_device (hcd->self.root_hub);
	(void) ohci_hub_suspend (hcd);
	usb_unlock_device (hcd->self.root_hub);
#endif

	/* let things settle down a bit */
	msleep (100);
	
#ifdef CONFIG_PMAC_PBOOK
	if (_machine == _MACH_Pmac) {
	   	struct device_node	*of_node;
 
		/* Disable USB PAD & cell clock */
		of_node = pci_device_to_OF_node (to_pci_dev(hcd->self.controller));
		if (of_node)
			pmac_call_feature(PMAC_FTR_USB_ENABLE, of_node, 0, 0);
	}
#endif /* CONFIG_PMAC_PBOOK */
	return 0;
}


static int ohci_rbus_resume (struct usb_hcd *hcd)
{
	struct ohci_hcd		*ohci = hcd_to_ohci (hcd);
	int			retval = 0;

#ifdef CONFIG_PMAC_PBOOK
	if (_machine == _MACH_Pmac) {
		struct device_node *of_node;

		/* Re-enable USB PAD & cell clock */
		of_node = pci_device_to_OF_node (to_pci_dev(hcd->self.controller));
		if (of_node)
			pmac_call_feature (PMAC_FTR_USB_ENABLE, of_node, 0, 1);
	}
#endif /* CONFIG_PMAC_PBOOK */

	/* resume root hub */
	if (time_before (jiffies, ohci->next_statechange))
		msleep (100);
#ifdef	CONFIG_USB_SUSPEND
	/* get extra cleanup even if remote wakeup isn't in use */
	retval = usb_resume_device (hcd->self.root_hub);
#else
	usb_lock_device (hcd->self.root_hub);
	retval = ohci_hub_resume (hcd);
	usb_unlock_device (hcd->self.root_hub);
#endif

	return retval;
}

#endif	/* CONFIG_PM */

/*-------------------------------------------------------------------------*/

static const struct hc_driver ohci_pci_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"OHCI Host Controller",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_MEMORY | HCD_USB11,

	/*
	 * basic lifecycle operations
	 */
	.reset =		ohci_pci_reset,
	.start =		ohci_pci_start,
#if     defined(CONFIG_PM)
	.suspend =		ohci_rbus_suspend,
	.resume =		ohci_rbus_resume,
#endif
	.stop =			ohci_stop,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef	CONFIG_USB_SUSPEND
	.hub_suspend =		ohci_hub_suspend,
	.hub_resume =		ohci_hub_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
};

/*-------------------------------------------------------------------------*/
//cfyeh+ 2005/10/05
static int ohci_hcd_drv_probe(struct device *dev)
{
	return usb_hcd_rbus_probe(&ohci_pci_hc_driver, to_platform_device(dev));
}

static int ohci_hcd_drv_remove(struct device *dev)
{
	struct platform_device	*pdev = to_platform_device(dev);
	struct usb_hcd		*hcd = dev_get_drvdata(dev);
	//struct ohci_hcd		*ohci = hcd_to_ohci (hcd);

	usb_hcd_rbus_remove(hcd, pdev);
	/*
	if (ohci->transceiver) {
		(void) otg_set_host(ohci->transceiver, 0);
		put_device(ohci->transceiver->dev);
	}
	*/
	dev_set_drvdata(dev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int ohci_hcd_drv_suspend(struct device *dev, pm_message_t state, u32 level)
{
	struct platform_device	*pdev = to_platform_device(dev);
	struct usb_hcd		*hcd = dev_get_drvdata(dev);
	//struct ohci_hcd		*ohci = hcd_to_ohci (hcd);

	return usb_hcd_rbus_suspend(hcd, pdev, state, level);
}

static int ohci_hcd_drv_resume(struct device *dev, u32 level)
{
	struct platform_device	*pdev = to_platform_device(dev);
	struct usb_hcd		*hcd = dev_get_drvdata(dev);
	//struct ohci_hcd		*ohci = hcd_to_ohci (hcd);

	return usb_hcd_rbus_resume(hcd, pdev, level);
}
#endif

/* pci driver glue; this is a "new style" PCI driver module */
static struct device_driver ohci_hcd_driver = {
	.name =		(char *) hcd_name,
	.bus		= &platform_bus_type,

	.probe =	ohci_hcd_drv_probe,
	.remove =	ohci_hcd_drv_remove,

#ifdef CONFIG_PM
	.suspend =	ohci_hcd_drv_suspend,
	.resume =	ohci_hcd_drv_resume,
#endif
};

static struct platform_device *ohci_hcd_devs;
/*-------------------------------------------------------------------------*/
 
static int __init ohci_hcd_pci_init (void) 
{
	int ret; //cfyeh+ 2005/10/05
	
	printk (KERN_DEBUG "%s: " DRIVER_INFO " (PCI)\n", hcd_name);
	if (usb_disabled())
		return -ENODEV;

	pr_debug ("%s: block sizes: ed %Zd td %Zd\n", hcd_name,
		sizeof (struct ed), sizeof (struct td));

	//cfyeh+ 2005/10/05
	ohci_hcd_devs = platform_device_register_simple((char *)hcd_name,
			-1, NULL, 0);
	
	if (IS_ERR(ohci_hcd_devs)) {
		ret = PTR_ERR(ohci_hcd_devs);
		return ret;
	}

	return driver_register (&ohci_hcd_driver);
}
module_init (ohci_hcd_pci_init);

/*-------------------------------------------------------------------------*/

static void __exit ohci_hcd_pci_cleanup (void) 
{	
	//cfyeh+ 2005/10/05
	struct platform_device *hcd_dev = ohci_hcd_devs;
	ohci_hcd_devs = NULL;

	driver_unregister (&ohci_hcd_driver);
	platform_device_unregister(hcd_dev);
	//cfyeh- 2005/10/05
}
module_exit (ohci_hcd_pci_cleanup);
