/*
 * drivers/usb/core/sysfs.c
 *
 * (C) Copyright 2002 David Brownell
 * (C) Copyright 2002,2004 Greg Kroah-Hartman
 * (C) Copyright 2002,2004 IBM Corp.
 *
 * All of the sysfs file attributes for usb devices and interfaces.
 *
 */


#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include "usb.h"
#include <platform.h>	// for get board id
#include "hub.h"	// for hub LED control

/* endpoint stuff */
struct ep_object {
	struct usb_endpoint_descriptor *desc;
	struct usb_device *udev;
	struct kobject kobj;
};

#define to_ep_object(_kobj) \
	container_of(_kobj, struct ep_object, kobj)

struct ep_attribute {
	struct attribute attr;
	ssize_t (*show)(struct usb_device *,
			struct usb_endpoint_descriptor *, char *);
};
#define to_ep_attribute(_attr) \
	container_of(_attr, struct ep_attribute, attr)

#define EP_ATTR(_name)						\
struct ep_attribute ep_##_name = {				\
	.attr = {.name = #_name, .owner = THIS_MODULE,		\
			.mode = S_IRUGO},			\
	.show = show_ep_##_name}

#define usb_ep_attr(field, format_string)			\
static ssize_t show_ep_##field(struct usb_device *udev,		\
		struct usb_endpoint_descriptor *desc, 		\
		char *buf)					\
{								\
	return sprintf(buf, format_string, desc->field);	\
}								\
static EP_ATTR(field);

usb_ep_attr(bLength, "%02x\n")
usb_ep_attr(bEndpointAddress, "%02x\n")
usb_ep_attr(bmAttributes, "%02x\n")
usb_ep_attr(bInterval, "%02x\n")

static ssize_t show_ep_wMaxPacketSize(struct usb_device *udev,
		struct usb_endpoint_descriptor *desc, char *buf)
{
	return sprintf(buf, "%04x\n",
			le16_to_cpu(desc->wMaxPacketSize) & 0x07ff);
}
static EP_ATTR(wMaxPacketSize);

static ssize_t show_ep_type(struct usb_device *udev,
		struct usb_endpoint_descriptor *desc, char *buf)
{
	char *type = "unknown";

	switch (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
	case USB_ENDPOINT_XFER_CONTROL:
		type = "Control";
		break;
	case USB_ENDPOINT_XFER_ISOC:
		type = "Isoc";
		break;
	case USB_ENDPOINT_XFER_BULK:
		type = "Bulk";
		break;
	case USB_ENDPOINT_XFER_INT:
		type = "Interrupt";
		break;
	}
	return sprintf(buf, "%s\n", type);
}
static EP_ATTR(type);

static ssize_t show_ep_interval(struct usb_device *udev,
		struct usb_endpoint_descriptor *desc, char *buf)
{
	char unit;
	unsigned interval = 0;
	unsigned in;

	in = (desc->bEndpointAddress & USB_DIR_IN);

	switch (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
	case USB_ENDPOINT_XFER_CONTROL:
		if (udev->speed == USB_SPEED_HIGH) 	/* uframes per NAK */
			interval = desc->bInterval;
		break;
	case USB_ENDPOINT_XFER_ISOC:
		interval = 1 << (desc->bInterval - 1);
		break;
	case USB_ENDPOINT_XFER_BULK:
		if (udev->speed == USB_SPEED_HIGH && !in)	/* uframes per NAK */
			interval = desc->bInterval;
		break;
	case USB_ENDPOINT_XFER_INT:
		if (udev->speed == USB_SPEED_HIGH)
			interval = 1 << (desc->bInterval - 1);
		else
			interval = desc->bInterval;
		break;
	}
	interval *= (udev->speed == USB_SPEED_HIGH) ? 125 : 1000;
	if (interval % 1000)
		unit = 'u';
	else {
		unit = 'm';
		interval /= 1000;
	}

	return sprintf(buf, "%d%cs\n", interval, unit);
}
static EP_ATTR(interval);

static ssize_t show_ep_direction(struct usb_device *udev,
		struct usb_endpoint_descriptor *desc, char *buf)
{
	char *direction;

	if ((desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
			USB_ENDPOINT_XFER_CONTROL)
		direction = "both";
	else if (desc->bEndpointAddress & USB_DIR_IN)
		direction = "in";
	else
		direction = "out";
	return sprintf(buf, "%s\n", direction);
}
static EP_ATTR(direction);

static struct attribute *ep_attrs[] = {
	&ep_bLength.attr,
	&ep_bEndpointAddress.attr,
	&ep_bmAttributes.attr,
	&ep_bInterval.attr,
	&ep_wMaxPacketSize.attr,
	&ep_type.attr,
	&ep_interval.attr,
	&ep_direction.attr,
	NULL,
};

static void ep_object_release(struct kobject *kobj)
{
	kfree(to_ep_object(kobj));
}

static ssize_t ep_object_show(struct kobject *kobj, struct attribute *attr,
		char *buf)
{
	struct ep_object *ep_obj = to_ep_object(kobj);
	struct ep_attribute *ep_attr = to_ep_attribute(attr);

	return (ep_attr->show)(ep_obj->udev, ep_obj->desc, buf);
}

static struct sysfs_ops ep_object_sysfs_ops = {
	.show =			ep_object_show,
};

static struct kobj_type ep_object_ktype = {
	.release =		ep_object_release,
	.sysfs_ops =		&ep_object_sysfs_ops,
	.default_attrs =	ep_attrs,
};

static void usb_create_ep_files(struct kobject *parent,
		struct usb_host_endpoint *endpoint,
		struct usb_device *udev)
{
	struct ep_object *ep_obj;
	struct kobject *kobj;

	ep_obj = kmalloc(sizeof(struct ep_object), GFP_KERNEL);
	memset(ep_obj, 0, sizeof(struct ep_object));
	if (!ep_obj)
		return;

	ep_obj->desc = &endpoint->desc;
	ep_obj->udev = udev;

	kobj = &ep_obj->kobj;
	kobject_set_name(kobj, "ep_%02x", endpoint->desc.bEndpointAddress);
	kobj->parent = parent;
	kobj->ktype = &ep_object_ktype;

	/* Don't use kobject_register, because it generates a hotplug event */
	kobject_init(kobj);
	if (kobject_add(kobj) == 0)
		endpoint->kobj = kobj;
	else
		kobject_put(kobj);
}

static void usb_remove_ep_files(struct usb_host_endpoint *endpoint)
{
	if (endpoint->kobj) {
		kobject_del(endpoint->kobj);
		kobject_put(endpoint->kobj);
		endpoint->kobj = NULL;
	}
}

//#ifdef CONFIG_REALTEK_VENUS_USB_TEST_MODE //cfyeh+ 2006/08/22
#include "hub.h"
#include "hcd.h"
#include <venus.h>
#include <asm/io.h>
#include <venus.h>

#define UsbTestModeNumber 6
#define UsbSuspendResume 1
#define UsbGetDescriptor 3

//for ehci root hub
static int gEnableHubOnBoard = -1; //default = -1
static int gEnableHubOnBoard_woone = -1; //default = -1
static int gUsbEhciRootHubPort = 1; //default = 1
static int gUsbEhciRootHubTestMode = 0;
static int gUsbForceHsMode = 1; //default = 1
int gUsbEhciHubSuspendResume[4] = {-1, -1, -1, -1}; // 0:Resume, 1:Suspend
EXPORT_SYMBOL(gUsbEhciHubSuspendResume);

//for ehci hub on board
static int gUsbEhciHubPort = 1; //default = 1
static int gUsbEhciHubTestMode = 0;
static int gUsbEhciHubPortSuspendResume = 0; // 0:Resume, 1:Suspend
static int usb_hub_port_power[4]={-1, -1, -1, -1};

//share w/ ehci root hub and ehci hub on boar
static int gUsbTestModeChanged = 0;

int gUsbGetDescriptor = 0;
EXPORT_SYMBOL(gUsbGetDescriptor);

int bForEhciDebug = 0;
EXPORT_SYMBOL(bForEhciDebug);

static int HubLEDControl_value = 0;

// copy from hub.c and rename
/*
 * USB 2.0 spec Section 11.24.2.2
 */
static int hub_clear_port_feature(struct usb_device *hdev, int port1, int feature)
{
	return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
			ClearPortFeature, USB_RT_PORT, feature, port1,
			NULL, 0, 1000);
}

/*
 * USB 2.0 spec Section 11.24.2.13
 */
static int hub_set_port_feature(struct usb_device *hdev, int port1, int feature)
{
	return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
			SetPortFeature, USB_RT_PORT, feature, port1,
			NULL, 0, 1000);
}

/*
 * USB 2.0 spec Section 11.24.2.7
 */
static int hub_get_port_status(struct usb_device *hdev, int port1, unsigned char *buf, int length)
{
	return usb_control_msg(hdev, usb_rcvctrlpipe(hdev, 0),
			GetPortStatus, USB_RT_PORT | USB_DIR_IN, 0, port1,
			buf, length, 1000);
}

/*
 * USB 2.0 spec Section 9.4.5
 */
static int roothub_get_port_status(struct usb_device *hdev, int port1, unsigned char *buf, int length)
{
	return usb_control_msg(hdev, usb_rcvctrlpipe(hdev, 0),
			USB_REQ_GET_STATUS, USB_RECIP_DEVICE | USB_DIR_IN, 0, port1,
			buf, length, 1000);
}

static int hub_check_port_suspend_resume(struct usb_device *hdev, int port1)
{
	unsigned char *data;
	int length = 4;
	int ret = -1;
	
	data = kmalloc(length, GFP_NOIO);
	if(data == NULL)
		return ret;
	memset(data, 0, length);
	hub_get_port_status(hdev, port1, data, length);
	ret = (data[0] >> 2) & 0x1; //bit 2
	kfree(data);
	
	return ret;
}

static int usb_test_mode_ehci_interrupt = 0;
static int ehci_disable_interrupt_reg(void)
{
	if(usb_test_mode_ehci_interrupt == 0)
	{
		usb_test_mode_ehci_interrupt = inl(VENUS_USB_EHCI_USBINTR);
		if(usb_test_mode_ehci_interrupt != 0)
		{
			printk("[cfyeh] %s(%d) VENUS_USB_EHCI_USBINTR = 0x%x\n", __func__, __LINE__, inl(VENUS_USB_EHCI_USBINTR));
			outl(0, VENUS_USB_EHCI_USBINTR);
			printk("[cfyeh] %s(%d) VENUS_USB_EHCI_USBINTR = 0x%x\n", __func__, __LINE__, inl(VENUS_USB_EHCI_USBINTR));
		}
		else
			printk("[cfyeh] %s(%d) VENUS_USB_EHCI_USBINTR = 0x%x\n", __func__, __LINE__, inl(VENUS_USB_EHCI_USBINTR));
	}

	return usb_test_mode_ehci_interrupt;
}

static int ehci_enable_interrupt_reg(void)
{
	if(usb_test_mode_ehci_interrupt != 0)
	{
		if(inl(VENUS_USB_EHCI_USBINTR) == 0)
		{
			printk("[cfyeh] %s(%d) VENUS_USB_EHCI_USBINTR = 0x%x\n", __func__, __LINE__, inl(VENUS_USB_EHCI_USBINTR));
			outl(usb_test_mode_ehci_interrupt, VENUS_USB_EHCI_USBINTR);
			printk("[cfyeh] %s(%d) VENUS_USB_EHCI_USBINTR = 0x%x\n", __func__, __LINE__, inl(VENUS_USB_EHCI_USBINTR));
		}
		else
		{
			printk("[cfyeh] %s(%d) VENUS_USB_EHCI_USBINTR = 0x%x\n", __func__, __LINE__, inl(VENUS_USB_EHCI_USBINTR));
		}
		usb_test_mode_ehci_interrupt = 0;
	}

	return usb_test_mode_ehci_interrupt;
}

#ifdef USB_MARS_HOST_TEST_MODE_JK
static int bCall_USBPHY_SetReg_Default_3A_for_JK = 0;
extern void USBPHY_SetReg_Default_3A(void);
extern void USBPHY_SetReg_Default_3A_for_JK(int port);
#endif /* USB_MARS_HOST_TEST_MODE_JK */

static int ehci_root_hub_test_mode(struct usb_device *udev, int port1, int test_mode)
{
#define USB_TEST_FORCE_HS_MODE(x)	((x)<<16) //1 bit
#define USB_TEST_SIM_MODE(x)	((x)<<17) //1 bit
	
	struct usb_hcd *ehci_hcd = container_of(udev->bus, struct usb_hcd, self);
	unsigned int volatile tmp;
	unsigned int suspend_mux = 0;
	unsigned int port_address = 0;
	unsigned int usb_host_usbip_input_offset = 0;
	unsigned int usb_host_self_loop_back_offset = 0;
	
	//usb2 spec 4.14 p.114
	printk("Root Hub port %d, test mode = %d\n", port1, test_mode /*test[test_mode]*/);
	port_address = VENUS_USB_EHCI_PORTSC_0 + (port1 - 1) * 0x4;
	usb_host_usbip_input_offset = VENUS_USB_HOST_USBIP_INPUT + (port1 - 1) * 0x20;
	usb_host_self_loop_back_offset = VENUS_USB_HOST_SELF_LOOP_BACK + (port1 - 1) * 0x20;


	if(0 != test_mode)
	{
#ifdef USB_TEST_MODE_DISABLE_IRQ_2
		//disable irq
		printk("disable irq 2 ...\n");
		disable_irq(2);
#else
		ehci_disable_interrupt_reg();
#endif /* USB_TEST_MODE_DISABLE_IRQ_2 */
		mdelay(2000);
	}
	
	outl( inl(usb_host_usbip_input_offset) | (1 << 27) | (1 << 30), \
			usb_host_usbip_input_offset);  // UTMI_Suspend_mux = 1
	//outl( (inl(usb_host_usbip_input_offset) & ~(1 << 27)) | (1 << 30),
	//		usb_host_usbip_input_offset);  // UTMI_Suspend_mux = 0
	suspend_mux = ((inl(usb_host_usbip_input_offset) & (1 <<27)) == (1 <<27));
	printk("UTMI suspend mux (VENUS_USB_HOST_USBIP_INPUT bit27)= %d\n",suspend_mux);	

	if (5 == test_mode){ // occupy this mode
		unsigned int volatile tmp1, tmp0;

#if 0 // remove codes for test mode 5			
		// reset host controller
		printk("reset host controller\n");
			printk("...Clear RUN = 0\n");
			outl(inl(VENUS_USB_EHCI_USBCMD) & (unsigned int)~1,VENUS_USB_EHCI_USBCMD);	// clear RUN = 0
			
			printk("...Wait for HCHalted = 1");
			do{
				tmp0 = inl(VENUS_USB_EHCI_USBSTS); // polling USBSTS
			} while ((unsigned int)(1 << 12) != ((unsigned int)(1 << 12) & tmp0));  // wait until HCHalted = 1
			printk("...ok\n");
			
			printk("...Set HCRESET = 1\n");
			outl(inl(VENUS_USB_EHCI_USBCMD) & (unsigned int)(1 << 1),VENUS_USB_EHCI_USBCMD);	// set HCRESET = 1
			
			printk("...Wait for HCRESET = 0");
			do{
				tmp0 = inl(VENUS_USB_EHCI_USBCMD); // polling USBCMD
			} while ((unsigned int)(1 << 1) == ((unsigned int)(1 << 1) & tmp0));  // wait until HCRESET = 0
			printk("...ok\n");
		
		// initialize host controller
		printk("initialize host controller\n");
			printk("...Wait for HCHalted = 1");
			do{
				tmp0 = inl(VENUS_USB_EHCI_USBSTS); // polling USBSTS
			} while ((unsigned int)(1 << 12) != ((unsigned int)(1 << 12) & tmp0));  // wait until HCHalted = 1
			printk("...ok\n");
			
			printk("...Set RUN = 1\n");
			outl(inl(VENUS_USB_EHCI_USBCMD) | (unsigned int)1,VENUS_USB_EHCI_USBCMD);	// set RUN = 1
			
			printk("...Set ConfigFlag = 1\n");
			outl(inl(VENUS_USB_EHCI_CONFIGFLAG) | (unsigned int)1, VENUS_USB_EHCI_CONFIGFLAG);	// set ConfigFlag = 1
			
			printk("...Set PortPower = 1\n");
			outl(inl(port_address) | (unsigned int)(1 << 12),port_address);	// set PortPower = 1

		//use gUsbForceHsMode to emulate device attaching
		if (gUsbForceHsMode){
			outl(inl(usb_host_self_loop_back_offset) | USB_TEST_FORCE_HS_MODE(1), usb_host_self_loop_back_offset);
			printk("emulate device attaching \n");			
			mdelay(10);
		}

		// initialize port
		printk("initialize port\n");
			printk("...Wait for CurrentConnectStatus = 1");
			do{
				tmp0 = inl(port_address); // polling PORTSC
			} while ((unsigned int)1 != ((unsigned int)1 & tmp0));  // wait until CurrentConnectStatus = 1
			printk("...ok\n");
			
			printk("...Set PortReset = 1\n");
			outl(inl(port_address) | (unsigned int)(1 << 8),port_address);	// set PortReset = 1
			
			printk("...Wait 50 mS");
			mdelay(50);
			printk("...ok\n");
			
			printk("...Clear PortReset = 0\n");
			outl(inl(port_address) & (unsigned int)~(1 << 8),port_address);	// clear PortReset = 0
		
			printk("...Wait for PortEnabled = 1");
			do{
				tmp0 = inl(port_address); // polling PORTSC
			} while ((unsigned int)(1 << 2) != ((unsigned int)(1 << 2) & tmp0));  // wait until PortEnabled = 1
			printk("...ok\n");
			
                        printk("...Check Frame Index is going");
			do{
				tmp0 = inl(VENUS_USB_EHCI_FRINDEX); // polling FRINDEX
			} while (tmp0 == inl(VENUS_USB_EHCI_FRINDEX));  // wait until FRINDEX is going
			printk("...ok\n");

			mdelay(100);
#endif
			
		// Enter test mode TEST_FORCE_ENABLE (Fake)
		printk("Enter test mode TEST_FORCE_ENABLE (Fake)\n");
			printk("...Clear ASE = PSE = 0\n");
			outl(inl(VENUS_USB_EHCI_USBCMD) & (unsigned int)~(3 << 4),VENUS_USB_EHCI_USBCMD);	// clear ASE = PSE = 0
			
			printk("...Wait for ASS = PSS = 0");
			do{
				tmp0 = inl(VENUS_USB_EHCI_USBSTS); // polling USBSTS
			} while ((unsigned int)(3 << 14) == ((unsigned int)(3 << 14) & tmp0));  // wait until ASS = PSS = 0
			printk("...ok\n");

		// detect device removal
		printk("Detect device removal\n");
		
			tmp0 = (3<<10);
			while(1){
				tmp1 = inl(port_address) & ~(3<<10);
				if (tmp0 != tmp1)
				{
					printk("ehci regs (port status) connect ? %s - 0x%.8x     \n",
					((1<<0) == (tmp1 & (1<<0))) ? "Yes" : "NO ", tmp1);
					tmp0 = tmp1;
				}
				if((1<<0) != (tmp1 & (1<<0)))
					break;
			}
			printk("...Set RUN = 0\n");
			outl(inl(VENUS_USB_EHCI_USBCMD) & ~(unsigned int)1,VENUS_USB_EHCI_USBCMD);	// set RUN = 0
			return 0;
	}
					
	if(test_mode == 0)//HCReset
	{
#ifdef USB_MARS_HOST_TEST_MODE_JK
		if(is_mars_cpu())// for mars
		{
			if(bCall_USBPHY_SetReg_Default_3A_for_JK == 1)
			{
				printk("calling USBPHY_SetReg_Default_3A()\n");
				USBPHY_SetReg_Default_3A();
			}
		}
#endif /* USB_MARS_HOST_TEST_MODE_JK */
		//step 0 force hs mode turn off 
		tmp = inl(usb_host_self_loop_back_offset) & ~USB_TEST_FORCE_HS_MODE(0x1) & ~USB_TEST_SIM_MODE(0x1);
		tmp |= USB_TEST_SIM_MODE(0) | USB_TEST_FORCE_HS_MODE(0);
		outl(tmp, usb_host_self_loop_back_offset);
		printk("Step 0:  clear gUsbForceHsMode 0x%.8x -> 0x%.8x\n",tmp, inl(usb_host_self_loop_back_offset));

		do
		{
			tmp = inl(VENUS_USB_EHCI_USBSTS);
		}while(!(tmp & (1<<12)));

		//step 1
		tmp = inl(VENUS_USB_EHCI_USBCMD);
		tmp |= (1<<1);
		outl(tmp, VENUS_USB_EHCI_USBCMD);
		printk("Step 1: set Host reset ehci regs (command) = 0x%.8x -> 0x%.8x\n",tmp,inl(VENUS_USB_EHCI_USBCMD));
		do
		{
			tmp = inl(VENUS_USB_EHCI_USBCMD);
		}while(tmp & (1<<1));
	
		//step 2
		tmp = inl(VENUS_USB_EHCI_USBCMD);
		tmp |= (1<<0);
		outl(tmp, VENUS_USB_EHCI_USBCMD);
		
		ehci_hcd->driver->resume(ehci_hcd);//1-4
		printk("Step 2: run ehci_resume(), ehci command regs = 0x%.8x\n",inl(VENUS_USB_EHCI_USBCMD));
	
	        //clear gUsbForceHsMode setting
		tmp = inl(usb_host_self_loop_back_offset) & ~USB_TEST_FORCE_HS_MODE(0x1) & ~USB_TEST_SIM_MODE(0x1);
		outl(tmp, usb_host_self_loop_back_offset);
		
		mdelay(2000);

#ifdef USB_TEST_MODE_DISABLE_IRQ_2
		//enable irq
		printk("enable irq 2 ...\n");
		enable_irq(2);
#else
		ehci_enable_interrupt_reg();
#endif /* USB_TEST_MODE_DISABLE_IRQ_2 */
	}
	else
	{
		tmp = inl(usb_host_self_loop_back_offset) & ~USB_TEST_FORCE_HS_MODE(0x1) & ~USB_TEST_SIM_MODE(0x1);
		if (gUsbForceHsMode) tmp |= USB_TEST_FORCE_HS_MODE(1);
		outl(tmp, usb_host_self_loop_back_offset);
		printk("Step 0-1: set gUsbForceHsMode (VENUS_USB_HOST_SELF_LOOP_BACK) = 0x%.8x -> 0x%.8x\n",tmp,inl(usb_host_self_loop_back_offset));	

		mdelay(10);

		if (gUsbForceHsMode){
			////////////////////reset port////////////////////////////
			//set port reset=1 & port enable=0
			tmp = inl(port_address);
			printk("Step 0-2: before port reset ehci regs (port status) = 0x%.8x\n",tmp);
			tmp |= (1<<8);
			tmp &= ~(1<<2);
			outl(tmp, port_address);
			printk("Step 0-3: reseting port ehci regs (port status) = 0x%.8x -> 0x%.8x\n",tmp,inl(port_address));
	
			//wait 50ms
			mdelay(50);
			
			//clear port reset
			tmp = inl(port_address);
			tmp &= ~(1<<8);
			outl(tmp, port_address);
			//while (inl(port_address) & (1<<8));
			printk("Step 0-4: ehci regs (port status) = 0x%.8x -> 0x%.8x\n",tmp,inl(port_address));
		}
		
		//step 1 test mode procedure
		tmp = inl(VENUS_USB_EHCI_USBCMD);
		//schedule_enable = tmp & ((1<<5) | (1<<4))
		tmp &= ~((1<<5) | (1<<4));
		outl(tmp, VENUS_USB_EHCI_USBCMD);
		printk("Step 1: ehci regs (command) = 0x%.8x -> 0x%.8x\n",tmp,inl(VENUS_USB_EHCI_USBCMD));
		
		if(suspend_mux)  // UTMI_Suspend_mux = 1
		{
			//step 2
			tmp = inl(port_address);
			tmp |= (1<<7);
			outl(tmp, port_address);
			printk("Step 2: ehci regs (port status) = 0x%.8x -> 0x%.8x\n",tmp,inl(port_address));
		}
				
		//step 3
		tmp = inl(VENUS_USB_EHCI_USBCMD);
		tmp &= ~((1<<0));
		outl(tmp, VENUS_USB_EHCI_USBCMD);
		
		// ensure HC_Halted = 1
		printk("Step 3: ehci regs (command) = 0x%.8x -> 0x%.8x\n",tmp,inl(VENUS_USB_EHCI_USBCMD));

		//for delay 2mS
		{
			int i;
			for (i=0;i<3000;i++)
			{
				tmp = inl(VENUS_USB_EHCI_HCCAPBASE);
			}
		}
				
		if(!suspend_mux)  // UTMI_Suspend_mux = 0
		{
			//step 2
			tmp = inl(port_address);
			tmp |= (1<<7);
			outl(tmp, port_address);
			printk("Step 2: ehci regs (port status) = 0x%.8x -> 0x%.8x\n",tmp,inl(port_address));
		}
		
#ifdef USB_MARS_HOST_TEST_MODE_JK
		if(is_mars_cpu())// for mars
		{
			if((test_mode == 1) || (test_mode == 2))
			{
				printk("calling USBPHY_SetReg_Default_3A_for_JK()\n");
				USBPHY_SetReg_Default_3A_for_JK(port1);
				bCall_USBPHY_SetReg_Default_3A_for_JK = 1;
			}
		}
#endif /* USB_MARS_HOST_TEST_MODE_JK */

		//step 4
		tmp = inl(port_address);
		tmp &= ~(0xf << 16);
		tmp |= test_mode << 16;
		outl(tmp, port_address);
		printk("Step 4: ehci regs (port status) = 0x%.8x -> 0x%.8x\n",tmp,inl(port_address));
	}

	return 0;
}

static ssize_t  show_bDescriptor (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;
	unsigned char *data;
	int length = 0x12;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	data = kmalloc(length, GFP_NOIO);
	if(data == NULL)
		return sprintf (buf, "%d\n", -1);
	memset(data, 0, length);
	usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RECIP_DEVICE,
			USB_DT_DEVICE << 8, 0, data, length, 1000);
	length = sprintf (buf, "%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n" \
		       "%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n"
		       "%.2x %.2x\n", \
		       data[0], data[1], data[2], data[3], \
		       data[5], data[6], data[7], data[8], \
		       data[9], data[10], data[11], data[12], \
		       data[13], data[14], data[15], data[16], \
		       data[17], data[18]);
	kfree(data);
	return length;
}

static DEVICE_ATTR(bDescriptor, S_IRUGO, show_bDescriptor, NULL);

static ssize_t  show_bPortDescriptor (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if(udev->parent == NULL) // ehci root hub
		return sprintf (buf, "%d\n", gUsbGetDescriptor);
	else
		return sprintf (buf, "%d\n", gUsbGetDescriptor);

	return 0;
}

static ssize_t
set_bPortDescriptor (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;
	unsigned char		*data;
	int			size, i, port;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > UsbGetDescriptor)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf ((char *)buf, "%d\n", -1);

	if((config == 0) || (gUsbGetDescriptor != config))
	{
		gUsbGetDescriptor = config;
		if(udev->parent == NULL) // ehci root hub
		{
			port = gUsbEhciRootHubPort - 1;
		}
		else // ehci hub which is not root hub
		{
			port = gUsbEhciHubPort - 1;
		}
		size=0x12;
		data = (unsigned char*)kmalloc(size, GFP_KERNEL);
		memset (data, 0, size);
		usb_control_msg(udev->children[port], usb_rcvctrlpipe(udev->children[port], 0),
				USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
				USB_DT_DEVICE << 8, 0, data, size,
				USB_CTRL_GET_TIMEOUT);
	
		if(gUsbGetDescriptor == 0)
		{
			printk(" get device descriptor\n");
			for( i = 0; i < size; i++)
			{
				printk(" %.2x", data[i]);
				if((i % 15) == 0 && (i != 0))
					printk("\n<1>");
			}
			printk("\n");
		}

		if(gUsbGetDescriptor == 3)
			gUsbGetDescriptor = 0;
		kfree(data);
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortDescriptor, S_IRUGO | S_IWUSR, 
		show_bPortDescriptor, set_bPortDescriptor);

static ssize_t  show_bPortStatus (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;
	unsigned char *data;
	int length;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
	{
		length = 2;
		data = kmalloc(length, GFP_NOIO);
		if(data == NULL)
			return sprintf (buf, "%d\n", -1);
		memset(data, 0, length);
		roothub_get_port_status(udev, gUsbEhciRootHubPort, data, length);
		length = sprintf (buf, "%.2x %.2x\n", data[0], data[1]);
		kfree(data);
		return length;
	}
	else // ehci hub which is not root hub
	{
		length = 4;
		data = kmalloc(length, GFP_NOIO);
		if(data == NULL)
			return sprintf (buf, "%d\n", -1);
		memset(data, 0, length);
		hub_get_port_status(udev, gUsbEhciHubPort, data, length);
		length = sprintf (buf, "%.2x %.2x %.2x %.2x\n", data[0], data[1], data[2], data[3]);
		kfree(data);
		return length;
	}

	return 0;
}

static DEVICE_ATTR(bPortStatus, S_IRUGO, show_bPortStatus, NULL);

static ssize_t  show_bPortOverCurrent (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;
	unsigned char *data;
	int length;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
	{
		length = 2;
		data = kmalloc(length, GFP_NOIO);
		if(data == NULL)
			return sprintf (buf, "%d\n", -1);
		memset(data, 0, length);
		roothub_get_port_status(udev, gUsbEhciRootHubPort, data, length);
		length = sprintf (buf, "%d\n", (data[0] >> 3) & 0x1);
		kfree(data);
		return length;
	}
	else // ehci hub which is not root hub
	{
		length = 4;
		data = kmalloc(length, GFP_NOIO);
		if(data == NULL)
			return sprintf (buf, "%d\n", -1);
		memset(data, 0, length);
		hub_get_port_status(udev, gUsbEhciHubPort, data, length);
		length = sprintf (buf, "%d\n", (data[0] >> 3) & 0x1);
		kfree(data);
		return length;
	}

	return 0;
}

static DEVICE_ATTR(bPortOverCurrent, S_IRUGO, show_bPortOverCurrent, NULL);

static ssize_t  show_bPortSuspendResume (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
	{
		return sprintf (buf, "%d %d %d %d\n", gUsbEhciHubSuspendResume[0], \
				gUsbEhciHubSuspendResume[1], gUsbEhciHubSuspendResume[2], gUsbEhciHubSuspendResume[3]);
	}
	else // ehci hub which is not root hub
	{
		gUsbEhciHubPortSuspendResume = hub_check_port_suspend_resume(udev, gUsbEhciHubPort);
		return sprintf (buf, "%d\n", gUsbEhciHubPortSuspendResume);
	}

	return 0;
}

// hack by cfyeh for usb suspend/resume
int usb_ehci_suspend_flag = 0;
EXPORT_SYMBOL(usb_ehci_suspend_flag);

static ssize_t
set_bPortSuspendResume (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	struct usb_hcd		*ehci_hcd = container_of(udev->bus, struct usb_hcd, self);
	int			config, value;
	unsigned int		port1, port_address = 0;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > UsbSuspendResume)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(udev->parent == NULL) // ehci root hub
	{
		port1 = (unsigned int) gUsbEhciRootHubPort;
		if(gUsbEhciHubSuspendResume[port1 - 1] != config)
		{
			port_address = VENUS_USB_EHCI_PORTSC_0 + (port1 - 1) * 0x4;
			gUsbEhciHubSuspendResume[port1 - 1] = config;
			printk("Root Hub port %d - %s\n", port1, (gUsbEhciHubSuspendResume[port1 - 1] == 1) ? "Suspend" : "Resume");
			if(gUsbEhciHubSuspendResume[port1 - 1] == 1) //Suspend
			{
				usb_ehci_suspend_flag = 1;

#ifdef USB_ROOT_PORT_SUSPEND_RESUEM_BY_REGS
				usb_lock_device (ehci_hcd->self.root_hub);
				//ehci_disable_interrupt_reg();

				printk("Value of reg(0x%.8x) = 0x%.8x\n", port_address, inl(port_address));
				printk(" Set Suspend bit!!!!\n");
				if(is_mars_cpu() && ((port1 - 1) == 0))
				{
					printk(" For port 0 only !!!!\n");
					printk(" before Value of reg(0x%.8x) = 0x%.8x\n", VENUS_USB_HOST_BASE, inl(VENUS_USB_HOST_BASE));
					outl(0x41, VENUS_USB_HOST_BASE);
					printk("  after Value of reg(0x%.8x) = 0x%.8x\n", VENUS_USB_HOST_BASE, inl(VENUS_USB_HOST_BASE));
				}
				outl(inl(port_address) | (1 << 7), port_address);
				do{
					printk("Value of reg(0x%.8x) = 0x%.8x\n", port_address, inl(port_address));
				}while((inl(port_address) & (1 << 7)) != (1 << 7));
				printk("Value of reg(0x%.8x) = 0x%.8x\n", port_address, inl(port_address));
				usb_unlock_device (ehci_hcd->self.root_hub);
#else
				printk("call ehci_hub_suspend() to the root hub port %d...\n", gUsbEhciRootHubPort);
				usb_lock_device (ehci_hcd->self.root_hub);

				//printk("disable irq 2 ...\n");
				//disable_irq(2);
				//mdelay(1000);

				ehci_hcd->driver->hub_suspend(ehci_hcd);

				usb_unlock_device (ehci_hcd->self.root_hub);
				printk("call ehci_hub_suspend() OK !!!\n");
#endif /* USB_ROOT_PORT_SUSPEND_RESUEM_BY_REGS */

			}
			else //Resume
			{
				usb_ehci_suspend_flag = 0;

#ifdef USB_ROOT_PORT_SUSPEND_RESUEM_BY_REGS
				usb_lock_device (ehci_hcd->self.root_hub);

				printk("Value of reg(0x%.8x) = 0x%.8x\n", port_address, inl(port_address));
				printk(" Set Resume bit!!!!\n");
				outl(inl(port_address) | (1 << 6), port_address);
				msleep(20);
				do{
					printk("Value of reg(0x%.8x) = 0x%.8x\n", port_address, inl(port_address));
				}while((inl(port_address) & (1 << 6)) != (1 << 6));
				printk("Value of reg(0x%.8x) = 0x%.8x\n", port_address, inl(port_address));
				printk(" Clear Resume bit!!!!\n");
				outl(inl(port_address) & ~(1 << 6), port_address);

				do{
					printk("Value of reg(0x%.8x) = 0x%.8x\n", port_address, inl(port_address));
				}while((inl(port_address) & (1 << 6)) == (1 << 6));
				printk("Value of reg(0x%.8x) = 0x%.8x\n", port_address, inl(port_address));

				//ehci_enable_interrupt_reg();
				usb_unlock_device (ehci_hcd->self.root_hub);
#else
				printk("call ehci_hub_resume() to the root hub port %d...\n", gUsbEhciRootHubPort);
				usb_lock_device (ehci_hcd->self.root_hub);
				
				ehci_hcd->driver->hub_resume(ehci_hcd);

				//mdelay(1000);
				//printk("enable irq 2 ...\n");
				//enable_irq(2);
				
				usb_unlock_device (ehci_hcd->self.root_hub);
				printk("call ehci_hub_resume() OK !!!\n");
#endif /* USB_ROOT_PORT_SUSPEND_RESUEM_BY_REGS */

			}
			msleep(1000);
		}
	}
	else // ehci hub which is not root hub
	{
		gUsbEhciHubPortSuspendResume = hub_check_port_suspend_resume(udev, gUsbEhciHubPort);

		if(gUsbEhciHubPortSuspendResume != config)
		{
			gUsbEhciHubPortSuspendResume = config;
			if(gUsbEhciHubPortSuspendResume == 1) //Suspend
			{
				printk("set USB_PORT_FEAT_SUSPEND to the port %d of the hub ...\n", gUsbEhciHubPort);
				hub_set_port_feature(udev, gUsbEhciHubPort, USB_PORT_FEAT_SUSPEND);
				printk("set OK !!!\n");
			}
			else //Resume
			{
				printk("clear USB_PORT_FEAT_SUSPEND to the port %d of the hub ...\n", gUsbEhciHubPort);
				hub_clear_port_feature(udev, gUsbEhciHubPort, USB_PORT_FEAT_SUSPEND);
				printk("clear OK !!!\n");
			}
			msleep(1000);
		}
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortSuspendResume, S_IRUGO | S_IWUSR, 
		show_bPortSuspendResume, set_bPortSuspendResume);

static ssize_t  show_bPortReset (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
		return sprintf (buf, "%d\n", gUsbEhciRootHubPort);
	else // ehci hub which is not root hub
		return sprintf (buf, "%d\n", gUsbEhciHubPort);

	return 0;
}

static ssize_t
set_bPortReset (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config != 1)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	printk("reset device ...\n");

	if(udev->parent == NULL) // ehci root hub
	{
		unsigned int volatile tmp, port_address;
		port_address = VENUS_USB_EHCI_PORTSC_0 + (gUsbEhciRootHubPort - 1) * 0x4;
		//disable irq
		printk("disable irq\n");
		disable_irq(2);
		mdelay(2000);

		//set port reset=1 & port enable=0
		tmp = inl(port_address);		
		printk("Step 0: before ehci regs (port status) = 0x%.8x\n",tmp);
		
		tmp |= (1<<8);
		tmp &= ~(1<<2);
		outl(tmp, port_address);
		printk("Step 1: ehci regs (port status) = 0x%.8x\n",tmp);
		
		//wait 50ms
		mdelay(50);

		//clear port reset
		do
		{
			tmp = inl(port_address);		
			tmp &= ~(1<<8);
			outl(tmp, port_address);
			printk("Step 2: ehci regs (port status) = 0x%.8x\n",tmp);	
		}while (tmp & (1<<8));

		//enable irq
		mdelay(2000);
		printk("enable irq\n");
		enable_irq(2);
	}
	else // ehci hub which is not root hub
	{
#if 0 //don/t work to reset device on port of first class hub
		printk("set USB_PORT_FEAT_RESET to the port %d of the hub ...\n", gUsbEhciHubPort);
		hub_set_port_feature(udev, gUsbEhciHubPort, USB_PORT_FEAT_RESET);
		printk("set OK !!!\n");
#else
#  if 0 // reset bPortNumber
		if(gUsbEhciHubPort <= (udev->maxchild + 1))
			usb_reset_device(udev->children[gUsbEhciHubPort - 1]);
#  else
		usb_reset_device(udev);
#  endif
#endif
	}
	
	printk("reset OK !!!\n");

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortReset, S_IRUGO | S_IWUSR, 
		show_bPortReset, set_bPortReset);

///////////////////////////////////////usb proc count///////////////////////////////////////
//for setting VENUS_USB_HOST_WRAPPER bit5~bit1
//write for setting 
//read for getting setting
static ssize_t  show_bDebugForLA (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
	{
		return sprintf (buf, "0x%x\n", (inl(VENUS_USB_HOST_WRAPPER) & (0x1f << 1)) >> 1);
	}

	return 0;
}

static ssize_t
set_bDebugForLA (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || (config < 1) || (config > 31))
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(udev->parent == NULL) // ehci root hub
	{
		//change to usb debug mode
		writel(0x5ad, (void __iomem *)VENUS_0_SYS_DEBUG);
		//set usb wrapper
		outl(inl(VENUS_USB_HOST_WRAPPER) & ~(0x1f << 1), VENUS_USB_HOST_WRAPPER);
		outl(inl(VENUS_USB_HOST_WRAPPER) | (config << 1), VENUS_USB_HOST_WRAPPER);
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bDebugForLA, S_IRUGO | S_IWUSR, 
		show_bDebugForLA, set_bDebugForLA);

static ssize_t
set_bDeviceReset (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config != 1)
		return -EINVAL;

	if(udev->parent == NULL) // ehci root hub
	{
	}
	else // ehci hub which is not root hub
	{
		printk("reset device ...\n");
		usb_reset_device(udev);
		printk("reset OK !!!\n");
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bDeviceReset, S_IWUSR, NULL, set_bDeviceReset);

extern int usb_check_gpio20_board(void);
extern int usb_check_gpio6_board(void);
static ssize_t  show_bEnableHubOnBoard (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
		return sprintf (buf, "%d\n", gEnableHubOnBoard);
	else // ehci hub which is not root hub
		return sprintf (buf, "%d\n", -1);

	return 0;
}

static ssize_t
set_bEnableHubOnBoard (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device		*udev = udev = to_usb_device (dev);
	struct usb_hub_descriptor	descriptor;
	int				config, value, ret;

	if ((value = sscanf (buf, "%u", &config)) != 1)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(config > 0)
		config = 1;
	else
		config = 0;

	if(udev->parent == NULL) // ehci root hub
	{
		///////////////////////////////////////////////////////////////
		// use hub control 
		///////////////////////////////////////////////////////////////

		// device on root port 1
		if(udev->children[0])
		{
	                //printk("there is a device on root port 1.\n");
			if(udev->children[0]->descriptor.bDeviceClass != USB_CLASS_HUB)
			{
	                	//printk("the device isn't a hub.\n");
				goto out;
			}
			else
			{
	                	//printk("the device is a hub.\n");
			}
		}
		else
		{
	                //printk("there isn't any device on root port 1.\n");
			goto out;
		}

		ret = usb_control_msg(udev->children[0], usb_rcvctrlpipe(udev->children[0], 0),
			USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
			USB_DT_HUB << 8, 0, &descriptor, sizeof(struct usb_hub_descriptor),
			HZ * USB_CTRL_GET_TIMEOUT);

	        if (ret < 0) {
	                printk("can't read hub descriptor");
	                goto out;
	        }

/*
 * wHubCharacteristics (masks) 
 * See USB 2.0 spec Table 11-13, offset 3
 */
#define HUB_CHAR_LPSM           0x0003 /* D1 .. D0 */

		le16_to_cpus(&descriptor.wHubCharacteristics);
	        switch (descriptor.wHubCharacteristics & HUB_CHAR_LPSM) {
	                case 0x00:
	                        //printk("hub : ganged power switching\n");
	                        break;
	                case 0x01:
	                        //printk("hub : individual port power switching\n");
				if(gEnableHubOnBoard != config)
				{
					gEnableHubOnBoard = config;
					if(config == 1)
					{
						hub_set_port_feature(udev->children[0], 1, USB_PORT_FEAT_POWER);
						msleep(1000);
					}
					else if(config == 0)
					{
						hub_clear_port_feature(udev->children[0], 1, USB_PORT_FEAT_POWER);
						msleep(1000);
					}
					printk("[cfyeh] %s port 1 of Hub on board by port power control OK !\n", (gEnableHubOnBoard) ? "Enable" : "Disable");
					
				}
				goto out;
	                        break;
	                case 0x02:
	                case 0x03:
	                        //printk("hub : no power switching (usb 1.0)\n");
	                        break;
	        }

		///////////////////////////////////////////////////////////////
		// use gpio 
		///////////////////////////////////////////////////////////////

		//printk("gEnableHubOnBoard = %d, config = %d\n", (gEnableHubOnBoard), config);
		if(gEnableHubOnBoard != config)
		{
			
			gEnableHubOnBoard = config;
			if(is_venus_cpu())
			{	
			// gEnableHubOnBoard = 0 for gpio output pin
			// gEnableHubOnBoard = 1 for gpio input pin
			if(usb_check_gpio20_board())
			{
				if (gEnableHubOnBoard)
					outl(inl(VENUS_MIS_GP0DATO) & ~(0x1 << 20), VENUS_MIS_GP0DATO); // output latch = 0
				else
					outl(inl(VENUS_MIS_GP0DATO) | (0x1 << 20), VENUS_MIS_GP0DATO); // output latch = 1
			}
			else if(usb_check_gpio6_board())
			{
				if (gEnableHubOnBoard)
					outl(inl(VENUS_MIS_GP0DATO) & ~(0x1 << 6), VENUS_MIS_GP0DATO); // output latch = 0
				else
					outl(inl(VENUS_MIS_GP0DATO) | (0x1 << 6), VENUS_MIS_GP0DATO); // output latch = 1
			}
			}
			printk("[cfyeh] %s port 1 of Hub on board OK !\n", (gEnableHubOnBoard) ? "Enable" : "Disable");
		}
	}

out:
	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bEnableHubOnBoard, S_IRUGO | S_IWUSR, 
		show_bEnableHubOnBoard, set_bEnableHubOnBoard);

static ssize_t  show_bEnableHubOnBoard_woone (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
		return sprintf (buf, "%d\n", gEnableHubOnBoard_woone);
	else // ehci hub which is not root hub
		return sprintf (buf, "%d\n", -1);

	return 0;
}

#ifdef USB_HACK_DISABLE_PORT_POWER
extern int usb_disable_port_power_status;
#endif /* USB_HACK_DISABLE_PORT_POWER */

static ssize_t
set_bEnableHubOnBoard_woone (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device		*udev = udev = to_usb_device (dev);
	struct usb_hub_descriptor	descriptor;
	int				config, value, ret;
	int				i = 2;

	if ((value = sscanf (buf, "%u", &config)) != 1)
		return -EINVAL;

#ifdef USB_HACK_DISABLE_PORT_POWER
	if(usb_disable_port_power_status == 0)
		return value;
#endif /* USB_HACK_DISABLE_PORT_POWER */

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(config > 0)
		config = 1;
	else
		config = 0;

	if(udev->parent == NULL) // ehci root hub
	{
		///////////////////////////////////////////////////////////////
		// use hub control 
		///////////////////////////////////////////////////////////////

		// device on root port 1
		if(udev->children[0])
		{
	                //printk("there is a device on root port 1.\n");
			if(udev->children[0]->descriptor.bDeviceClass != USB_CLASS_HUB)
			{
	                	//printk("the device isn't a hub.\n");
				goto out;
			}
			else
			{
	                	//printk("the device is a hub.\n");
			}
		}
		else
		{
	                //printk("there isn't any device on root port 1.\n");
			goto out;
		}

		ret = usb_control_msg(udev->children[0], usb_rcvctrlpipe(udev->children[0], 0),
			USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
			USB_DT_HUB << 8, 0, &descriptor, sizeof(struct usb_hub_descriptor),
			HZ * USB_CTRL_GET_TIMEOUT);

	        if (ret < 0) {
	                printk("can't read hub descriptor");
	                goto out;
	        }

/*
 * wHubCharacteristics (masks) 
 * See USB 2.0 spec Table 11-13, offset 3
 */
#define HUB_CHAR_LPSM           0x0003 /* D1 .. D0 */

		le16_to_cpus(&descriptor.wHubCharacteristics);
	        if ((descriptor.wHubCharacteristics & HUB_CHAR_LPSM) < 2) {
			//if(gEnableHubOnBoard_woone != config)
			{
				gEnableHubOnBoard_woone = config;
				if(config == 1)
				{
					while(!hub_set_port_feature(udev->children[0], i++, USB_PORT_FEAT_POWER));
					msleep(1000);
				}
				else if(config == 0)
				{
					while(!hub_clear_port_feature(udev->children[0], i++, USB_PORT_FEAT_POWER));
					msleep(1000);
				}
				printk("[cfyeh] %s ports' power w/o port one (2-4) of Hub on board by port power control OK !\n", (gEnableHubOnBoard_woone) ? "Enable" : "Disable");
			}
	        }
	}

out:
	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bEnableHubOnBoard_woone, S_IRUGO | S_IWUSR, 
		show_bEnableHubOnBoard_woone, set_bEnableHubOnBoard_woone);

static ssize_t  show_bHubLEDControl (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
		return sprintf (buf, "%d\n", -1);
	else // ehci hub which is not root hub
		return sprintf (buf, "%d\n", HubLEDControl_value);

	return 0;
}

static ssize_t
set_bHubLEDControl (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device		*udev = to_usb_device (dev);
	struct usb_hub			*hub = (struct usb_hub *)container_of(udev, struct usb_hub, hdev);
	int				config, value;
	int				i = 2;

	if ((value = sscanf (buf, "%u", &config)) != 1)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(udev->parent == NULL) // ehci root hub
	{
	}
	else // ehci hub which is not root hub
	{
		if(HubLEDControl_value != config)
		{
			switch(config) {
			case HUB_LED_AMBER:	// 1
				HubLEDControl_value = hub->indicator[i] = HUB_LED_AMBER;
				break;
			case HUB_LED_GREEN:	// 2
				HubLEDControl_value = hub->indicator[i] = HUB_LED_GREEN;
				break;
			case HUB_LED_OFF:	// 3
				HubLEDControl_value = hub->indicator[i] = HUB_LED_OFF;
				break;
			case HUB_LED_AUTO:	// 0
				HubLEDControl_value = hub->indicator[i] = HUB_LED_AUTO;
				break;
			default:
				goto out;
			}
			
			hub_set_port_feature(udev, (HubLEDControl_value << 8) | gUsbEhciHubPort, USB_PORT_FEAT_INDICATOR);

		}
	}

out:
	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bHubLEDControl, S_IRUGO | S_IWUSR, 
		show_bHubLEDControl, set_bHubLEDControl);

static ssize_t  show_bEnumBus (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
		return sprintf (buf, "%d\n", gUsbEhciRootHubPort);
	else // ehci hub which is not root hub
		return sprintf (buf, "%d\n", gUsbEhciHubPort);

	return 0;
}

static ssize_t
set_bEnumBus (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	struct usb_hcd		*ehci_hcd = container_of(udev->bus, struct usb_hcd, self);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > udev->maxchild)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(udev->parent == NULL) // ehci root hub
	{
		printk("call ehci_resume() to the root hub...\n");
		ehci_hcd->driver->resume(ehci_hcd);
		printk("call ehci_resume() OK !!!\n");
	}
	else // ehci hub which is not root hub
	{
		printk("reset device ...\n");
		usb_reset_device(udev);
		printk("reset OK !!!\n");
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bEnumBus, S_IRUGO | S_IWUSR, 
		show_bEnumBus, set_bEnumBus);

static ssize_t
set_bHubPPE (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > udev->maxchild)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(udev->parent == NULL) // ehci root hub
	{
		printk("[cfyeh] set PPE=0 to the root hub ...\n");
                outl(0x2001001, VENUS_USB_OHCI_RH_DESC_A); // clear bit 9
                outl(0x8001, VENUS_USB_OHCI_RH_STATUS); // set bit 0
                outl(0x0, VENUS_USB_EHCI_PORTSC_0); // clear bit 12
                mdelay(10);
                outl(0x1000, VENUS_USB_EHCI_PORTSC_0); // set bit 12
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bHubPPE, S_IWUSR, NULL, set_bHubPPE);

static ssize_t  show_bPortNumber (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
		return sprintf (buf, "%d\n", gUsbEhciRootHubPort);
	else // ehci hub which is not root hub
		return sprintf (buf, "%d\n", gUsbEhciHubPort);

	return 0;
}

static ssize_t
set_bPortNumber (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > udev->maxchild)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(udev->parent == NULL) // ehci root hub
	{
		if(gUsbEhciRootHubPort != config)
		{
			gUsbEhciRootHubPort = config;
		}
	}
	else // ehci hub which is not root hub
	{
		if(gUsbEhciHubPort != config)
		{
			gUsbEhciHubPort = config;
		}
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortNumber, S_IRUGO | S_IWUSR, 
		show_bPortNumber, set_bPortNumber);

//#define USB_TEST_COUNT_DOWN

static int gUsbAddressOffsetPower = 8; //default = 16
//static int test_setcor_len = 1; // 512 bytes
//static int test_setcor_len = 20; // 512 bytes , 16K
static int test_setcor_len = 1;
static int lba_addr = 0x200000;
//static int test_buf_size = 0x10000;//64k
//static int test_buf_size = 0x2000;//8k
static int test_buf_size = 0x10000;

#ifdef USB_TEST_COUNT_DOWN
static int usb_test_offset = 0x1000 - 0xef0;
#else
static int usb_test_offset = 0x3f8;
#endif

static int usb_readwrite_blocks (struct device *dev, unsigned int lba_addr, 
		unsigned char *buf, size_t test_setcor_len, int b_read)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int actlen, ret = -1;
	int cmd_len = 31, data_len = test_setcor_len * 512, status_len = 13;
	unsigned char pcmd[31]={
		0x55, 0x53, 0x42, 0x43,//dCBWSignature
		0x48, 0xB1, 0x53, 0x82,//dCBWTag
		0x00, 0x02, 0x00, 0x00,//dCBWDataTransferLength(bytes)
		0x00,//in=0x80,0ut=0x00
		0x00,//lun
		0x0A, //bCBWCBLength
		//CBWCB 
		0x28, //read comman(0x28),write(0x2a)write and verify(0x2e)
		0x00, //lun<<5	
		0x00, 0x00, 0x00, 0x00,//LBA(17-20)
		0x00, 
		0x00, 0x01, //transfer length(sector)
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char *rbuf, *cmd, *status;
	int rbuf_len = 0x400;
	int sndbulkpipe = usb_sndbulkpipe(udev, 2);
	int rcvbulkpipe = usb_rcvbulkpipe(udev, 1);

	rbuf = kmalloc(rbuf_len, GFP_KERNEL);
	if (!rbuf)
		return (int)rbuf;
	cmd = (unsigned char *)(((unsigned int)rbuf + (0x100 - 1)) & ~(0x100 -1));
	status = (unsigned char *)(((unsigned int)rbuf + (0x200 - 1)) & ~(0x100 -1));
	memcpy(cmd, pcmd, cmd_len);
	
	if ((ret = down_interruptible(&udev->serialize)))
		return ret;
	
	// read or write command
	cmd[15] = b_read ? 0x28 : 0x2a;
	
	//LBA_ADDRdCBWDataTransferLength
	cmd[17]=((lba_addr>>24)&0xff);
	cmd[18]=((lba_addr>>16)&0xff);
	cmd[19]=((lba_addr>>8)&0xff);
	cmd[20]=((lba_addr)&0xff);

	//dCBWDataTransferLength
	cmd[8]=0x00; 
	cmd[9]=((test_setcor_len)&0xff);
	cmd[10]=(((test_setcor_len)>>8)&0xff);
	cmd[11]=(((test_setcor_len)>>16)&0xff);

	//transfer length(sector)
	cmd[22]=((test_setcor_len>>8)&0xff);
	cmd[23]=((test_setcor_len)&0xff);

#if 1
	// scsi command - command phase
	//printk("scsi command - command phase\n");
	ret = usb_bulk_msg(udev, sndbulkpipe, cmd, cmd_len, &actlen, HZ * 10);
	
	if (ret)
		err("command phase failed: %d (%d/%d) 0x%.3x",
				ret,cmd_len,actlen, ((unsigned int) buf) & 0xfff);
	else
		ret = actlen != cmd_len ? -1 : 0;
	
	mdelay(100);
	
	// scsi command - data phase
	if (!ret) {
#endif
		//printk("scsi command - data phase\n");
		ret = usb_bulk_msg(udev, b_read ? rcvbulkpipe : sndbulkpipe,
				buf, data_len, &actlen, HZ * 10);
		
		if (ret)
			err("data phase failed: %d (%d/%d) 0x%.3x"
					,ret,data_len,actlen, ((unsigned int) buf) & 0xfff);
		else
			ret = actlen != data_len ? -1 : 0;
#if 1
	}

	mdelay(100);
	
	// scsi command - status phase
	if (!ret) {
		//printk("scsi command - status phase\n");
		ret = usb_bulk_msg(udev, rcvbulkpipe,
				status, status_len, &actlen, HZ * 10);
		
		if (ret)
			err("status phase failed: %d (%d/%d) 0x%.3x",
					ret,status_len,actlen, ((unsigned int) buf) & 0xfff);
		else
			ret = actlen != status_len ? -1 : 0;
	}
#endif
	up(&udev->serialize);
	
	kfree(rbuf);
	mdelay(100);
	
	return ret;
}

static ssize_t  show_bAddressOffsetTest (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	unsigned char *rbuf, *pbuf;
	unsigned int end_address;
	int ret;
	unsigned int addr;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	printk(" Address Offset Test: Read at offset %d , size %d blocks!!!\n", 
			gUsbAddressOffsetPower, test_setcor_len);

	rbuf = kmalloc(test_buf_size, GFP_KERNEL);
	if (!rbuf)
		return (ssize_t)rbuf;
#ifdef USB_TEST_COUNT_DOWN //count down
	pbuf = (unsigned char *)(((unsigned int)rbuf + (0x2000 - 1)) & ~(0x1000 - 1)) - usb_test_offset;
	end_address = (unsigned int)pbuf - 0x1000;
#else
	pbuf = (unsigned char *)(((unsigned int)rbuf + (0x1000 - 1)) & ~(0x1000 - 1)) + usb_test_offset;
	end_address = (unsigned int)pbuf + 0x1000;
#endif
	addr = lba_addr;
	do
	{
		printk("buffer address = 0x%.8x\n", (unsigned int)pbuf);
		printk("lba address = 0x%.8x\n", addr);
		ret = usb_readwrite_blocks(dev, addr, pbuf, test_setcor_len, 1);
#ifdef USB_TEST_COUNT_DOWN //count down
		pbuf -= gUsbAddressOffsetPower;
#else
		pbuf += gUsbAddressOffsetPower;
#endif
		usb_test_offset += gUsbAddressOffsetPower;
		printk("\n");
#ifdef USB_TEST_COUNT_DOWN // count down
	}while((((unsigned int) pbuf) >= end_address) & (!ret));
#else
	}while((((unsigned int) pbuf) <= end_address) & (!ret));
#endif
	kfree(rbuf);

	if(usb_test_offset > 0x1000)
		usb_test_offset = 0;
	return 0;
}

static ssize_t
set_bAddressOffsetTest (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;
	
	unsigned char *rbuf, *pbuf;
	int tmp = 0;
	unsigned int end_address;
	int ret;
	unsigned int addr;
	int i = 0;

	if ((value = sscanf (buf, "%u", &config)) != 1 || 
			(config < 1) || (config > 4))
		return -EINVAL;

	tmp = 1;
	do
	{
		tmp *= 2;
		config--;		
	}while(config > 0);
	gUsbAddressOffsetPower = tmp;

	printk(" Address Offset Test: Write at offset %d , size %d blocks!!!\n", 
			gUsbAddressOffsetPower, test_setcor_len);

	rbuf = kmalloc(test_buf_size, GFP_KERNEL);
	if (!rbuf)
		return (ssize_t)rbuf;

	for(i=0;i<test_buf_size; i++)
		rbuf[i] = i & 0xff;
	
#ifdef USB_TEST_COUNT_DOWN //count down
	pbuf = (unsigned char *)(((unsigned int)rbuf + (0x2000 - 1)) & ~(0x1000 - 1)) - usb_test_offset;
	end_address = (unsigned int)pbuf - 0x1000;
#else
	pbuf = (unsigned char *)(((unsigned int)rbuf + (0x1000 - 1)) & ~(0x1000 - 1)) + usb_test_offset;
	end_address = (unsigned int)pbuf + 0x1000;
#endif
	addr = lba_addr;
	do
	{
		printk("buffer address = 0x%.8x\n", (unsigned int)pbuf);
		printk("lba address = 0x%.8x\n", addr);
		ret = usb_readwrite_blocks(dev, addr, pbuf, test_setcor_len, 0);
#ifdef USB_TEST_COUNT_DOWN //count down
		pbuf -= gUsbAddressOffsetPower;
#else
		pbuf += gUsbAddressOffsetPower;
#endif
		usb_test_offset += gUsbAddressOffsetPower;
		printk("\n");
#ifdef USB_TEST_COUNT_DOWN //count down
	}while((((unsigned int) pbuf) >= end_address) & (!ret));
#else
	}while((((unsigned int) pbuf) <= end_address) & (!ret));
#endif

	kfree(rbuf);
	if(usb_test_offset > 0x1000)
		usb_test_offset = 0;

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bAddressOffsetTest, S_IRUGO | S_IWUSR, 
		show_bAddressOffsetTest, set_bAddressOffsetTest);

static ssize_t  show_bPortTestMode (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
		return sprintf (buf, "%d,%d\n", gUsbEhciRootHubTestMode, gUsbForceHsMode);
	else // ehci hub which is not root hub
		return sprintf (buf, "%d\n", gUsbEhciHubTestMode);

	return 0;
}

static ssize_t
set_bPortTestMode (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value, i, prev_config = -1;
	unsigned int		port1 = 0;
	
	if ((value = sscanf (buf, "%u", &config)) != 1 || config > UsbTestModeNumber)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(udev->parent == NULL) // ehci root hub
	{
		// there is no test mode 6,
		// it's just for toogle gUsbForceHsMode
		if(config == 6)
		{
			gUsbForceHsMode ? (gUsbForceHsMode = 0) : (gUsbForceHsMode = 1);
		}
		else
		{
			if(gUsbEhciRootHubTestMode != config)
			{
				if(!((gUsbEhciRootHubTestMode > 0) && (config > 0)))
				{
					prev_config = gUsbEhciRootHubTestMode;
					gUsbEhciRootHubTestMode = config;
					gUsbTestModeChanged = 1;
				}
			}
			else
			{
				if(config == 0)
					gUsbTestModeChanged = 1;
			}
			port1 = (unsigned int) gUsbEhciRootHubPort;
		}
	}
	else // ehci hub which is not root hub
	{
		// there is no test mode 6,
		// it's not usful for ehci hub which is not root hub
		if(config != 6)
		{
			if(gUsbEhciHubTestMode != config)
			{
			       if(!((gUsbEhciHubTestMode > 0) && (config > 0)))
				{
					prev_config = gUsbEhciHubTestMode;
					gUsbEhciHubTestMode = config;
					gUsbTestModeChanged = 1;
				}
			}
			else
			{
				if(config == 0)
					gUsbTestModeChanged = 1;
			}
			port1 = (unsigned int) gUsbEhciHubPort;
		}
	}

	if(gUsbTestModeChanged == 1)
	{
		gUsbTestModeChanged = 0;
		
		if(config != 0)
		{
			printk("Call usb_lock_device().\n");
			usb_lock_device(udev);
		}

		if(udev->parent == NULL) // ehci root hub
		{
			ehci_root_hub_test_mode(udev, port1, config);
		}
		else // ehci hub which is not root hub
		{
			if(config == 0)
			{
				printk("Leave test mode ...\n");
				
				printk("clear USB_PORT_FEAT_POWER to the parent of the hub\n");
				for (i=1;i<=udev->parent->maxchild;i++)
				{
					printk("processing port %d of %d...\n", i, udev->parent->maxchild);
					hub_clear_port_feature(udev->parent, i, USB_PORT_FEAT_POWER);
					msleep(1000);
				}
				
				printk("set USB_PORT_FEAT_POWER to the parent of the hub\n");
				for (i=1;i<=udev->parent->maxchild;i++)
				{
					printk("processing port %d of %d...\n", i, udev->parent->maxchild);
					hub_set_port_feature(udev->parent, i, USB_PORT_FEAT_POWER);
					msleep(1000);
				}

				printk("Leave test mode , OK !!!\n");
			}
			else
			{
				printk("Enter test mode ...\n");
				
				printk("set USB_PORT_FEAT_SUSPEND to all ports of the hub\n");
				for (i=1;i<=udev->maxchild;i++)
				{
					printk("processing port %d of %d...\n", i, udev->maxchild);
					hub_set_port_feature(udev, i, USB_PORT_FEAT_SUSPEND);
					msleep(1000);
				}
				
				printk("set USB_PORT_FEAT_TEST mode %d to port %d ...\n", config, port1);
				hub_set_port_feature(udev,(config << 8) | port1, USB_PORT_FEAT_TEST);
				msleep(1000);
					
				printk("Enter test mode , OK !!!\n");
			}
		}

		if(config == 0)
		{
			printk("Call usb_unlock_device().\n");
			usb_unlock_device(udev);
		}
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortTestMode, S_IRUGO | S_IWUSR, 
		show_bPortTestMode, set_bPortTestMode);

static ssize_t
set_bDeviceTestMode (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;
	unsigned int		port1 = 0;
	
	if ((value = sscanf (buf, "%u", &config)) != 1 || config > (UsbTestModeNumber - 1))
		return -EINVAL;

	if((udev->descriptor.bDeviceClass == USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	usb_lock_device(udev);
	printk("Enter test mode ...\n");
	printk("set USB_DEVICE_TEST_MODE mode %d to port %d ...\n", config, port1);
	usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
		USB_REQ_SET_FEATURE, 0, USB_DEVICE_TEST_MODE,
		config << 8, NULL, 0, 1000);
	msleep(1000);
	printk("Enter test mode , OK !!!\n");
	usb_unlock_device(udev);

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bDeviceTestMode, S_IWUSR, NULL, set_bDeviceTestMode);

static ssize_t  show_usbRegister (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;
	u32 regs;
	int i;
	int regs_num;

#define HCS_N_PORTS(p)          (((p)>>0)&0xf)  /* bits 3:0, ports on HC */
	regs_num= 21 + HCS_N_PORTS(inl(VENUS_USB_EHCI_HCSPARAMS)); 

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	printk("\n EHCI regs\n");
	regs=VENUS_USB_EHCI_USBBASE;
	for(i=0;i<regs_num;i++)
	{
		if((i%4)==0)
			printk("0x%.8x : ", regs);
		printk("%.8x ", inl(regs));
		regs+=4;
		if((i%4)==3)
			printk("\n");		
	}
	printk("\n");		

	printk("\n OHCI regs\n");
	regs=VENUS_USB_OHCI_USBBASE;
	for(i=0;i<regs_num;i++)
	{
		if((i%4)==0)
			printk("0x%.8x : ", regs);
		printk("%.8x ", inl(regs));
		regs+=4;
		if((i%4)==3)
			printk("\n");		
	}
	printk("\n");		

	printk("\n WARPPER regs\n");
	regs=VENUS_USB_HOST_BASE;
	if(is_venus_cpu() || is_neptune_cpu())
		regs_num= 8 * HCS_N_PORTS(inl(VENUS_USB_EHCI_HCSPARAMS)); 
	else if(is_mars_cpu())
		regs_num= 14;
	for(i=0;i<regs_num;i++)
	{
		if((i%4)==0)
			printk("0x%.8x : ", regs);
		printk("%.8x ", inl(regs));
		regs+=4;
		if((i%4)==3)
			printk("\n");		
	}
	printk("\n");

	if(is_mars_cpu())
	{
		printk("\n OTG Device regs\n");
		regs=0xb8038800;
		regs_num= (0xb8038900 - 0xb8038800) / 4; 
		for(i=0;i<regs_num;i++)
		{
			if((i%4)==0)
				printk("0x%.8x : ", regs);
			printk("%.8x ", readl((void __iomem *)regs));
			regs+=4;
			if((i%4)==3)
				printk("\n");		
		}
		printk("\n");
	}

	return 0;
}

static DEVICE_ATTR(usbRegister, S_IRUGO, show_usbRegister, NULL);


#if 0 // test for 4 bytes boundary for control pipe
extern int usb_get_descriptor(struct usb_device *dev, unsigned char type, unsigned char index, void *buf, int size);
#endif // test for 4 bytes boundary for control pipe

#if 0 // test for 4 bytes boundary for bulk pipe
static unsigned int test_offset = 0;
#endif  // test for 4 bytes boundary for bulk pipe

static ssize_t  show_bBoundaryTest (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

#if 0 // test for 4 bytes boundary for control pipe
{
	char data[5000], *ptr;
	int ret;
	int ii, dd = 4;

	ptr = data;

	for(ii = 0 ; ii<= (4096 / dd); ii++)
	{
		ptr += dd;
		printk("#### ptr address = 0x%x\n", ptr);
		ret = usb_get_descriptor(udev, USB_DT_DEVICE, 0, ptr, 64);
	}

	return ret;
}
#endif  // test for 4 bytes boundary for control pipe

#if 0 // test for 4 bytes boundary for bulk pipe
{
	unsigned char command[31] =    {0x55, 0x53, 0x42, 0x43, 0x19, 0x00, 0x00, 0x00,
					0x00, 0x02, 0x00, 0x00, 0x80, 0x00, 0x0a, 0x28,
					0x00, 0x00, 0x00, 0xa1, 0x34, 0x00, 0x00, 0x01,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char status[13];
	unsigned char data[0x1600], *ptr, *ptr_0x1000;
	int act_len, ii;
	unsigned int dd = 4;
	unsigned int diff;
	int ret;

#if 0
	printk("#### MARS_USB_HOST_VERSION = 0x%.8x\n", inl(MARS_USB_HOST_VERSION));
	outl(0x0, MARS_USB_HOST_VERSION);
	printk("#### MARS_USB_HOST_VERSION = 0x%.8x\n", inl(MARS_USB_HOST_VERSION));
#endif

	printk("####       data address = 0x%x - 0x%x\n", data, data + sizeof(data));
	ptr_0x1000 = data;
	//ptr_0x1000 = (unsigned char *)(((unsigned int)data + 0x400 - 1) & ~(0x400 - 1));
	diff = 0x400 - ((unsigned int) data & (0x400 - 1));
#if 1 // start from 0x400 boundary 
	ptr_0x1000 += (0x400 - 0x400 + diff);
#else // start from data pointer
	ptr_0x1000 += (diff - diff + 0x0);
#endif
	printk("#### ptr_0x1000 address = 0x%x\n", ptr_0x1000);

	for(ii = 0 ; ; ii++)
	{
		ptr = ptr_0x1000 + test_offset;
		command[4] = ((unsigned int)ptr >> (8* 3)) & 0xff;
		command[5] = ((unsigned int)ptr >> (8* 2)) & 0xff;
		command[6] = ((unsigned int)ptr >> (8* 1)) & 0xff;
		command[7] = ((unsigned int)ptr >> (8* 0)) & 0xff;
		command[7]++;
		command[20]++;
		printk("#### %4d : ptr address = 0x%x\n", ii, ptr);

		printk("    #### command ####\n");
		ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, 1),
			command, sizeof(command), &act_len, 5 * HZ);
		if(ret < 0)
			printk("usb_bulk_msg return %d\n", ret);
		//msleep(500);

		printk("    #### data ####\n");
		ret = usb_bulk_msg(udev, usb_rcvbulkpipe(udev, 2),
			ptr, command[23] * 512 , &act_len, 5 * HZ);
		if(ret < 0)
			printk("usb_bulk_msg return %d\n", ret);
		//msleep(500);

		printk("    #### status ####\n");
		ret = usb_bulk_msg(udev, usb_rcvbulkpipe(udev, 2),
			status, sizeof(status) , &act_len, 5 * HZ);
		if(ret < 0)
			printk("usb_bulk_msg return %d\n", ret);
		//msleep(1000);

		if (test_offset == 0x1000)
		{
			test_offset = 0;
			return 0;
		}
		test_offset += dd;
	}

	return 0;
}
#endif  // test for 4 bytes boundary for bulk pipe

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // ehci root hub
	{
		return sprintf (buf, "%s(%d) - root hub\n", __func__, __LINE__);
	}
	else // ehci hub which is not root hub
	{
		return sprintf (buf, "%s(%d) - hub\n", __func__, __LINE__);
	}

	return 0;
}

static ssize_t
set_bBoundaryTest (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;
	
	if ((value = sscanf (buf, "%u", &config)) != 1)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(udev->parent == NULL) // ehci root hub
	{
		printk("%s(%d) - root hub\n", __func__, __LINE__);
	}
	else // ehci hub which is not root hub
	{
		printk("%s(%d) - hub\n", __func__, __LINE__);
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bBoundaryTest, S_IRUGO | S_IWUSR, 
		show_bBoundaryTest, set_bBoundaryTest);

static ssize_t  show_bPortPower (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return sprintf (buf, "%d\n", -1);

	if(udev->parent != NULL) // ehci hub on root port
	{
		return sprintf (buf, "port %d on hub is power %s\n", gUsbEhciHubPort, usb_hub_port_power[gUsbEhciHubPort - 1]==0 ? "OFF" : "ON");
	}

	return 0;
}

static ssize_t
set_bPortPower (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;
	
	if ((value = sscanf (buf, "%u", &config)) != 1)
		return -EINVAL;

	if((udev->descriptor.bDeviceClass != USB_CLASS_HUB) || 
			(udev->speed != USB_SPEED_HIGH))
		return value;

	if(udev->parent != NULL) // ehci hub on root port
	{
		if(config == 1)
		{
			if(usb_hub_port_power[gUsbEhciHubPort - 1] != config)
			{
				usb_hub_port_power[gUsbEhciHubPort - 1] = config;
				hub_set_port_feature(udev, gUsbEhciHubPort, USB_PORT_FEAT_POWER);
				msleep(1000);
				printk ("port %d on hub is power %s\n", gUsbEhciHubPort, usb_hub_port_power[gUsbEhciHubPort - 1]==1 ? "ON" : "OFF");
			}
		}
		else if(config == 0)
		{
			if(usb_hub_port_power[gUsbEhciHubPort - 1] != config)
			{
				usb_hub_port_power[gUsbEhciHubPort - 1] = config;
				hub_clear_port_feature(udev, gUsbEhciHubPort, USB_PORT_FEAT_POWER);
				msleep(1000);
				printk ("port %d on hub is power %s\n", gUsbEhciHubPort, usb_hub_port_power[gUsbEhciHubPort - 1]==1 ? "ON" : "OFF");
			}
		}
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortPower, S_IRUGO | S_IWUSR, 
		show_bPortPower, set_bPortPower);

static ssize_t  show_bForEhciDebug (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	return sprintf (buf, "%d\n", bForEhciDebug);
}

static ssize_t
set_bForEhciDebug (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1)
		return -EINVAL;

	if(config != 0)
		config = 1;
	bForEhciDebug = config;

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bForEhciDebug, S_IRUGO | S_IWUSR, 
		show_bForEhciDebug, set_bForEhciDebug);

//#endif /* CONFIG_REALTEK_VENUS_USB_TEST_MODE */ //cfyeh+ 2006/08/22

static ssize_t  show_bDeviceSpeed (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if(udev->speed == USB_SPEED_HIGH) // high speed device
		return sprintf (buf, "%d\n", USB_SPEED_HIGH);
	else if (udev->speed == USB_SPEED_FULL) // full speed device
		return sprintf (buf, "%d\n", USB_SPEED_FULL);
	else // low speed device
		return sprintf (buf, "%d\n", USB_SPEED_LOW);

	return 0;
}

static DEVICE_ATTR(bDeviceSpeed, S_IRUGO, show_bDeviceSpeed, NULL);

/* Active configuration fields */
#define usb_actconfig_show(field, multiplier, format_string)		\
static ssize_t  show_##field (struct device *dev, struct device_attribute *attr, char *buf)		\
{									\
	struct usb_device *udev;					\
	struct usb_host_config *actconfig;				\
									\
	udev = to_usb_device (dev);					\
	actconfig = udev->actconfig;					\
	if (actconfig)							\
		return sprintf (buf, format_string,			\
				actconfig->desc.field * multiplier);	\
	else								\
		return 0;						\
}									\

#define usb_actconfig_attr(field, multiplier, format_string)		\
usb_actconfig_show(field, multiplier, format_string)			\
static DEVICE_ATTR(field, S_IRUGO, show_##field, NULL);

usb_actconfig_attr (bNumInterfaces, 1, "%2d\n")
usb_actconfig_attr (bmAttributes, 1, "%2x\n")
usb_actconfig_attr (bMaxPower, 2, "%3dmA\n")

static ssize_t show_configuration_string(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;
	if ((!actconfig) || (!actconfig->string))
		return 0;
	return sprintf(buf, "%s\n", actconfig->string);
}
static DEVICE_ATTR(configuration, S_IRUGO, show_configuration_string, NULL);

/* configuration value is always present, and r/w */
usb_actconfig_show(bConfigurationValue, 1, "%u\n");

static ssize_t
set_bConfigurationValue (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if (sscanf (buf, "%u", &config) != 1 || config > 255)
		return -EINVAL;
	usb_lock_device(udev);
	value = usb_set_configuration (udev, config);
	usb_unlock_device(udev);
	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bConfigurationValue, S_IRUGO | S_IWUSR, 
		show_bConfigurationValue, set_bConfigurationValue);

/* String fields */
#define usb_string_attr(name)						\
static ssize_t  show_##name(struct device *dev, struct device_attribute *attr, char *buf)		\
{									\
	struct usb_device *udev;					\
									\
	udev = to_usb_device (dev);					\
	return sprintf(buf, "%s\n", udev->name); \
}									\
static DEVICE_ATTR(name, S_IRUGO, show_##name, NULL);

usb_string_attr(product);
usb_string_attr(manufacturer);
usb_string_attr(serial);

static ssize_t
show_speed (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	char *speed;

	udev = to_usb_device (dev);

	switch (udev->speed) {
	case USB_SPEED_LOW:
		speed = "1.5";
		break;
	case USB_SPEED_UNKNOWN:
	case USB_SPEED_FULL:
		speed = "12";
		break;
	case USB_SPEED_HIGH:
		speed = "480";
		break;
	default:
		speed = "unknown";
	}
	return sprintf (buf, "%s\n", speed);
}
static DEVICE_ATTR(speed, S_IRUGO, show_speed, NULL);

static ssize_t
show_devnum (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;

	udev = to_usb_device (dev);
	return sprintf (buf, "%d\n", udev->devnum);
}
static DEVICE_ATTR(devnum, S_IRUGO, show_devnum, NULL);

static ssize_t
show_version (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	u16 bcdUSB;

	udev = to_usb_device(dev);
	bcdUSB = le16_to_cpu(udev->descriptor.bcdUSB);
	return sprintf(buf, "%2x.%02x\n", bcdUSB >> 8, bcdUSB & 0xff);
}
static DEVICE_ATTR(version, S_IRUGO, show_version, NULL);

static ssize_t
show_maxchild (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;

	udev = to_usb_device (dev);
	return sprintf (buf, "%d\n", udev->maxchild);
}
static DEVICE_ATTR(maxchild, S_IRUGO, show_maxchild, NULL);

/* Descriptor fields */
#define usb_descriptor_attr_le16(field, format_string)			\
static ssize_t								\
show_##field (struct device *dev, struct device_attribute *attr, char *buf)				\
{									\
	struct usb_device *udev;					\
									\
	udev = to_usb_device (dev);					\
	return sprintf (buf, format_string, 				\
			le16_to_cpu(udev->descriptor.field));		\
}									\
static DEVICE_ATTR(field, S_IRUGO, show_##field, NULL);

usb_descriptor_attr_le16(idVendor, "%04x\n")
usb_descriptor_attr_le16(idProduct, "%04x\n")
usb_descriptor_attr_le16(bcdDevice, "%04x\n")

#define usb_descriptor_attr(field, format_string)			\
static ssize_t								\
show_##field (struct device *dev, struct device_attribute *attr, char *buf)				\
{									\
	struct usb_device *udev;					\
									\
	udev = to_usb_device (dev);					\
	return sprintf (buf, format_string, udev->descriptor.field);	\
}									\
static DEVICE_ATTR(field, S_IRUGO, show_##field, NULL);

#ifdef USB_TO_NOTIFY_TIER
unsigned int usb_bHubOverTier = 0;

static ssize_t  show_bHubOverTier (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	return sprintf(buf, "%d\n", usb_bHubOverTier);
}

static DEVICE_ATTR(bHubOverTier, S_IRUGO, show_bHubOverTier, NULL);

static ssize_t  show_bHubTierNumber (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	return sprintf(buf, "%d\n", udev->tier);
}

static DEVICE_ATTR(bHubTierNumber, S_IRUGO, show_bHubTierNumber, NULL);

#endif /* USB_TO_NOTIFY_TIER */

#ifdef USB_MARS_OTG_VERIFY_TEST_CODE
static int OTGTestSpeedLoop = 1;
static ssize_t  show_bOTGTestSpeedLoop (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	return sprintf(buf, "%d\n", OTGTestSpeedLoop);
}

static ssize_t
set_bOTGTestSpeedLoop (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config < 0)
		return -EINVAL;

	OTGTestSpeedLoop = config;

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bOTGTestSpeedLoop, S_IRUGO | S_IWUSR, 
		show_bOTGTestSpeedLoop, set_bOTGTestSpeedLoop);

static int OTGTestSpeedSize = 4096;
static ssize_t  show_bOTGTestSpeedSize (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	return sprintf(buf, "%d\n", OTGTestSpeedSize);
}

static ssize_t
set_bOTGTestSpeedSize (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config < 0)
		return -EINVAL;

	OTGTestSpeedSize = config;

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bOTGTestSpeedSize, S_IRUGO | S_IWUSR, 
		show_bOTGTestSpeedSize, set_bOTGTestSpeedSize);

static ssize_t  show_bOTGTestSpeed (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	return 0;
}

static ssize_t
set_bOTGTestSpeed (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;
	unsigned char		*data;
	int			size, i;
	struct			timeval time1, time2;
	int			usec,sec;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > 2)
		return -EINVAL;

	size = OTGTestSpeedSize;
	data = (unsigned char*)kmalloc(size, GFP_KERNEL);
	memset (data, 0, size);
	usb_lock_device(udev);
#if 0 // bulk
	if(config == 0) // for write
	{
		printk("### bulk write ###\n");
		do_gettimeofday(&time1);
		for(i = 0; i < OTGTestSpeedLoop; i++)
		{
		usb_control_msg(udev, usb_sndbulkpipe(udev, 0),
				0x5b, (USB_DIR_OUT|USB_TYPE_VENDOR),
				0, 0, data, size,
				USB_CTRL_GET_TIMEOUT);
		}
		do_gettimeofday(&time2);
	}
	else // for read
	{
		printk("### bulk read ###\n");
		do_gettimeofday(&time1);
		for(i = 0; i < OTGTestSpeedLoop; i++)
		{
		usb_control_msg(udev, usb_rcvbulkpipe(udev, 0),
				0x5c, (USB_DIR_IN|USB_TYPE_VENDOR),
				0, 0, data, size,
				USB_CTRL_GET_TIMEOUT);
		}
		do_gettimeofday(&time2);
	}
#else // ctrl
	if(config == 0) // for write
	{
		printk("### ctrl write ###\n");
		do_gettimeofday(&time1);
		for(i = 0; i < OTGTestSpeedLoop; i++)
		{
		usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				0x5b, (USB_DIR_OUT|USB_TYPE_VENDOR),
				0, 0, data, size,
				USB_CTRL_GET_TIMEOUT);
		}
		do_gettimeofday(&time2);
	}
	else // for read
	{
		printk("### ctrl read ###\n");
		do_gettimeofday(&time1);
		for(i = 0; i < OTGTestSpeedLoop; i++)
		{
		usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
				0x5c, (USB_DIR_IN|USB_TYPE_VENDOR),
				0, 0, data, size,
				USB_CTRL_GET_TIMEOUT);
		}
		do_gettimeofday(&time2);
	}
#endif
	printk("### loop = 0x%x, size = 0x%x, totalsize = 0x%x\n", OTGTestSpeedLoop, OTGTestSpeedSize, OTGTestSpeedLoop * OTGTestSpeedSize);
	
	sec = time2.tv_sec - time1.tv_sec;
	usec = time2.tv_usec - time1.tv_usec;
	if(usec < 0)
	{
		sec--;
		usec+=1000000;
	}
	printk("### time : %d sec %d usec\n", sec, usec);

	usb_unlock_device(udev);

	kfree(data);

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bOTGTestSpeed, S_IRUGO | S_IWUSR, 
		show_bOTGTestSpeed, set_bOTGTestSpeed);
#endif /* USB_MARS_OTG_VERIFY_TEST_CODE */

usb_descriptor_attr (bDeviceClass, "%02x\n")
usb_descriptor_attr (bDeviceSubClass, "%02x\n")
usb_descriptor_attr (bDeviceProtocol, "%02x\n")
usb_descriptor_attr (bNumConfigurations, "%d\n")
usb_descriptor_attr (bMaxPacketSize0, "%d\n")

static struct attribute *dev_attrs[] = {
	/* current configuration's attributes */
	&dev_attr_bNumInterfaces.attr,
	&dev_attr_bConfigurationValue.attr,
	&dev_attr_bmAttributes.attr,
	&dev_attr_bMaxPower.attr,
	/* device attributes */
	&dev_attr_idVendor.attr,
	&dev_attr_idProduct.attr,
	&dev_attr_bcdDevice.attr,
	&dev_attr_bDeviceClass.attr,
	&dev_attr_bDeviceSubClass.attr,
	&dev_attr_bDeviceProtocol.attr,
	&dev_attr_bNumConfigurations.attr,
	&dev_attr_bMaxPacketSize0.attr,
	&dev_attr_speed.attr,
	&dev_attr_devnum.attr,
	&dev_attr_version.attr,
	&dev_attr_maxchild.attr,
//#if CONFIG_REALTEK_VENUS_USB_TEST_MODE //cfyeh+ 2006/08/22
	&dev_attr_bPortStatus.attr,
	&dev_attr_bPortOverCurrent.attr,
	&dev_attr_bDescriptor.attr,
	&dev_attr_bPortDescriptor.attr,
	&dev_attr_bPortSuspendResume.attr,
	&dev_attr_bPortReset.attr,
	&dev_attr_bDebugForLA.attr,
	&dev_attr_bDeviceReset.attr,
	&dev_attr_bEnableHubOnBoard.attr,
	&dev_attr_bEnableHubOnBoard_woone.attr,
	&dev_attr_bHubLEDControl.attr,
	&dev_attr_bEnumBus.attr,
	&dev_attr_bHubPPE.attr,
	&dev_attr_bPortNumber.attr,
	&dev_attr_bAddressOffsetTest.attr,
	&dev_attr_bPortTestMode.attr,
	&dev_attr_bDeviceTestMode.attr,
	&dev_attr_bBoundaryTest.attr,
	&dev_attr_bPortPower.attr,
	&dev_attr_usbRegister.attr,
	&dev_attr_bForEhciDebug.attr,
//#endif /* CONFIG_REALTEK_VENUS_USB_TEST_MODE */ //cfyeh+ 2006/08/22
	&dev_attr_bDeviceSpeed.attr,
#ifdef USB_MARS_OTG_VERIFY_TEST_CODE
	&dev_attr_bOTGTestSpeedLoop.attr,
	&dev_attr_bOTGTestSpeedSize.attr,
	&dev_attr_bOTGTestSpeed.attr,
#endif /* USB_MARS_OTG_VERIFY_TEST_CODE */
#ifdef USB_TO_NOTIFY_TIER
	&dev_attr_bHubOverTier.attr,
	&dev_attr_bHubTierNumber.attr,
#endif /* USB_TO_NOTIFY_TIER */
	NULL,
};
static struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
};

void usb_create_sysfs_dev_files (struct usb_device *udev)
{
	struct device *dev = &udev->dev;

	sysfs_create_group(&dev->kobj, &dev_attr_grp);

	if (udev->manufacturer)
		device_create_file (dev, &dev_attr_manufacturer);
	if (udev->product)
		device_create_file (dev, &dev_attr_product);
	if (udev->serial)
		device_create_file (dev, &dev_attr_serial);
	device_create_file (dev, &dev_attr_configuration);
	usb_create_ep_files(&dev->kobj, &udev->ep0, udev);
}

void usb_remove_sysfs_dev_files (struct usb_device *udev)
{
	struct device *dev = &udev->dev;

	usb_remove_ep_files(&udev->ep0);
	sysfs_remove_group(&dev->kobj, &dev_attr_grp);

	if (udev->manufacturer)
		device_remove_file(dev, &dev_attr_manufacturer);
	if (udev->product)
		device_remove_file(dev, &dev_attr_product);
	if (udev->serial)
		device_remove_file(dev, &dev_attr_serial);
	device_remove_file (dev, &dev_attr_configuration);
}

/* Interface fields */
#define usb_intf_attr(field, format_string)				\
static ssize_t								\
show_##field (struct device *dev, struct device_attribute *attr, char *buf)				\
{									\
	struct usb_interface *intf = to_usb_interface (dev);		\
									\
	return sprintf (buf, format_string, intf->cur_altsetting->desc.field); \
}									\
static DEVICE_ATTR(field, S_IRUGO, show_##field, NULL);

usb_intf_attr (bInterfaceNumber, "%02x\n")
usb_intf_attr (bAlternateSetting, "%2d\n")
usb_intf_attr (bNumEndpoints, "%02x\n")
usb_intf_attr (bInterfaceClass, "%02x\n")
usb_intf_attr (bInterfaceSubClass, "%02x\n")
usb_intf_attr (bInterfaceProtocol, "%02x\n")

static ssize_t show_interface_string(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf;
	struct usb_device *udev;
	int len;

	intf = to_usb_interface (dev);
	udev = interface_to_usbdev (intf);
	len = snprintf(buf, 256, "%s", intf->cur_altsetting->string);
	if (len < 0)
		return 0;
	buf[len] = '\n';
	buf[len+1] = 0;
	return len+1;
}
static DEVICE_ATTR(interface, S_IRUGO, show_interface_string, NULL);

static ssize_t show_modalias(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf;
	struct usb_device *udev;
	struct usb_host_interface *alt;

	intf = to_usb_interface(dev);
	udev = interface_to_usbdev(intf);
	alt = intf->cur_altsetting;

	return sprintf(buf, "usb:v%04Xp%04Xd%04Xdc%02Xdsc%02Xdp%02X"
			"ic%02Xisc%02Xip%02X\n",
			le16_to_cpu(udev->descriptor.idVendor),
			le16_to_cpu(udev->descriptor.idProduct),
			le16_to_cpu(udev->descriptor.bcdDevice),
			udev->descriptor.bDeviceClass,
			udev->descriptor.bDeviceSubClass,
			udev->descriptor.bDeviceProtocol,
			alt->desc.bInterfaceClass,
			alt->desc.bInterfaceSubClass,
			alt->desc.bInterfaceProtocol);
}
static DEVICE_ATTR(modalias, S_IRUGO, show_modalias, NULL);

static struct attribute *intf_attrs[] = {
	&dev_attr_bInterfaceNumber.attr,
	&dev_attr_bAlternateSetting.attr,
	&dev_attr_bNumEndpoints.attr,
	&dev_attr_bInterfaceClass.attr,
	&dev_attr_bInterfaceSubClass.attr,
	&dev_attr_bInterfaceProtocol.attr,
	&dev_attr_modalias.attr,
	NULL,
};
static struct attribute_group intf_attr_grp = {
	.attrs = intf_attrs,
};

static inline void usb_create_intf_ep_files(struct usb_interface *intf,
		struct usb_device *udev)
{
	struct usb_host_interface *iface_desc;
	int i;

	iface_desc = intf->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i)
		usb_create_ep_files(&intf->dev.kobj, &iface_desc->endpoint[i],
				udev);
}

static inline void usb_remove_intf_ep_files(struct usb_interface *intf)
{
	struct usb_host_interface *iface_desc;
	int i;

	iface_desc = intf->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i)
		usb_remove_ep_files(&iface_desc->endpoint[i]);
}

void usb_create_sysfs_intf_files (struct usb_interface *intf)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_host_interface *alt = intf->cur_altsetting;

	sysfs_create_group(&intf->dev.kobj, &intf_attr_grp);

	if (alt->string == NULL)
		alt->string = usb_cache_string(udev, alt->desc.iInterface);
	if (alt->string)
		device_create_file(&intf->dev, &dev_attr_interface);
	usb_create_intf_ep_files(intf, udev);
}

void usb_remove_sysfs_intf_files (struct usb_interface *intf)
{
	usb_remove_intf_ep_files(intf);
	sysfs_remove_group(&intf->dev.kobj, &intf_attr_grp);

	if (intf->cur_altsetting->string)
		device_remove_file(&intf->dev, &dev_attr_interface);
}
