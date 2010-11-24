/*
 *  Copyright (c) 2002-2005 Petko Manolov (petkan@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/sched.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/random.h>

//#define DEBUG

#include <linux/usb.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include<linux/unistd.h>


/* Version Information */
#define DRIVER_VERSION "v0.0.1 (2007/02/08)"
#define DRIVER_AUTHOR "Realtek UWB SD <peiyeh@realtek.com.tw>"
#define DRIVER_DESC "RTU7300: usb-based WiNET driver"

#define	RTL7300_REQT_READ		0xc0
#define	RTL7300_REQT_WRITE	0x40
#define	RTL7300_REQ_GET_REGS	0x05
#define	RTL7300_REQ_SET_REGS	0x05

#define	RTL7300_MTU		1540
#define	RTL7300_TX_TIMEOUT	(HZ)
#define	RX_SKB_POOL_SIZE	30
#define	RX_URB_NUM		10

#define 	INTBUFSIZE			8 // Is the value suitable for our hardware?

/* Define these values to match your device */
#define VENDOR_ID_REALTEK		0x0bda
#define PRODUCT_ID_RTL7300		0x7300
#define PRODUCT_ID_RTL8172		0x8172

#undef	EEPROM_WRITE

/* rtu7300 flags */
//#define	RTU7300_HW_CRC		0
//#define	RX_REG_SET		1
#define	RTU7300_UNPLUG		2
#define	RX_URB_FAIL		3
#define	RX_FIX_SCHEDULE		4

//CPU Register definition
#define CPU_IO_BASE					(0x1f970000|0xa0000000)	// 0x1f97-0000 ~ 0x1f97-ffff (CPU Register Base Addr)
#define CR_ENABLE					0x0000
#define CR_KEEP_CONNECT			0x0001
#define CR_INTER_OP_TEST			0x0002
#define CR_EUI48_CHANGE			0x0003
#define CR_EUI48						0x0004	// 4~9
#define CR_MKEN						0x000e
#define CR_MKID0					0x0010
#define CR_MK0						0x0020
#define CR_MKID1					0x0030
#define CR_MK1						0x0040
#define CR_MKID2					0x0050
#define CR_MK2						0x0060
#define CR_MKID3					0x0070
#define CR_MK3						0x0080
#define CR_DRP_REQ					0x0090
#define CR_DRP_REL					0x00a0
#define CR_DRP_REQ_CMD				0x00b0
#define CR_DRP_REL_CMD				0x00b1
#define CR_DRP_GET_REQ_STATUS		0x00b2
#define CR_RF_RW					0x0100	// 0x0100~0x01ff
#define CR_NOLBBCN					0xfffc
#define CR_FASTLB					0xfffd
#define CR_LBTEST					0xfffe
#define CR_HWREADY					0xffff

#define USB_TX_DESC_SIZE			8
#define USB_RX_DESC_SIZE			8

/* hardware minimum and maximum for a single frame's data payload */
#define RTU7300_MIN_MTU	60	/* TODO: allow lower, but pad */
#define RTU7300_MAX_MTU	4096

/* table of devices that work with this driver */
static struct usb_device_id rtu7300_table[] = {
	{USB_DEVICE( VENDOR_ID_REALTEK, PRODUCT_ID_RTL7300)},
	{USB_DEVICE( VENDOR_ID_REALTEK, PRODUCT_ID_RTL8172)},
	{}
};

MODULE_DEVICE_TABLE(usb, rtu7300_table);

typedef char boolean;

struct rtu7300 {
	unsigned long flags;
	struct usb_device *udev;
	struct tasklet_struct tl;
	struct net_device_stats stats;
	struct net_device *netdev;
	struct urb *rx_urb[RX_URB_NUM], *tx_urb,*ctrl_urb;
	struct sk_buff *rx_skb[RX_URB_NUM]; //for free
	u8 * tx_buf;
	struct sk_buff *rx_skb_pool[RX_SKB_POOL_SIZE];
	spinlock_t rx_pool_lock, rx_lock;
	u8 *ctrl_buff;
	u8 phy;
	u8 rx_urb_stat[RX_URB_NUM];
	int rxbuf_size;
	// Link speed
	boolean usb_high;	// 1=usb2.0
	unsigned short need0_lengthsize;
};

struct my_skb_data{
	struct rtu7300 *dev;
	u8 cur_urb;
};

typedef struct rtu7300 rtu7300_t;

static void fill_skb_pool(rtu7300_t *);
static void free_skb_pool(rtu7300_t *);
static inline struct sk_buff *pull_skb(rtu7300_t *);
static void rtu7300_disconnect(struct usb_interface *intf);
static int rtu7300_probe(struct usb_interface *intf,
			   const struct usb_device_id *id);

static const char driver_name [] = "rtu7300";
#define RT7300_IMAGE_FILE	"boot.img"

enum {
	/*SIE Control Registers*/
	USB_SIECTL_BASE =0x2000,	
	USB_SYSCTL = 0x00,		/*USB System Control Register*/
	USB_IRQSTAT = 0x08,		/*USB Interrupt Status*/
	USB_IRQEN = 0x0C,		/*USB Interrupt Enable Register*/
	USB_CTRL = 0x10,		/*USB Control Register*/
	USB_STAT = 0x14,		/*USB Status Register*/
	USB_DEVADDR = 0x18,	/*USB Device Address*/
	USB_TEST = 0x1C,		/*USB Test Mode*/
	FRAME_NUMBER = 0x20,	/*USB Frame Number Counter*/
	USB_FIFO_ADDR = 0x28,	/*USB FIFO Address*/
	USB_FIFO_CMD = 0x2A,	/*USB FIFO Access Command*/
	USB_FIFO_DATA = 0x30,	/*USB FIFO Data*/
	
	/*USB DMA Registers*/
	USB_DMA_BASE = 0x3000,
	USB_LB_MODE = 0x0,
	USB_DOWNLOAD_DATA	= 0xB0, /* Download data command */
	USB_DOWNLOAD_CTRL	= 0xB4, /* Download control command */		
	/* - 8051 4181 Register interface*/
	CPU_CTRL = 0xA0, 			/*Control Command*/
	CPU_RW_DATA_IN  = 0xA4,	/*8051 Write only*/
	CPU_RW_DATA_OUT =0xA8, 	/*8051 Read only*/	
};


/*
**
**	debug related part of the code
**
*/

void memDump (void *start, u32 size, char * strHeader){
#if 0
  int row, column, index, index2, max;
  unsigned char *buf, ascii[17];
  char empty = ' ';

  if(!start ||(size==0))
        return;
  buf = (unsigned char *) start;

  /*
     16 bytes per line
   */
  if (strHeader)
  {
    	printk ("%s", strHeader);
  }
  column = size % 16;
  row = (size / 16) + 1;
  for (index = 0; index < row; index++, buf += 16) {
      memset (ascii, 0, 17);
      printk ("\n%08x ", (u32) buf);
      max = (index == row - 1) ? column : 16;

      //Hex
      for (index2 = 0; index2 < max; index2++){
          if (index2 == 8)
            printk ("  ");
          printk ("%02x ", (unsigned char) buf[index2]);
          ascii[index2] = ((unsigned char) buf[index2] < 32) ? empty : buf[index2];
        }

      if (max != 16){
          if (max < 8)
            printk ("  ");
          for (index2 = 16 - max; index2 > 0; index2--)
            printk ("   ");
        }
      //ASCIIM
      printk ("  %s", ascii);
    }
  printk ("\n");
#endif
  return;
}


/*
**
**	device related part of the code
**
*/

static int get_registers(rtu7300_t * dev, u16 value, u16 size, void *data)
{
	return usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
			       RTL7300_REQ_GET_REGS, RTL7300_REQT_READ,
			       value, 0, data, size, HZ / 2);
}

static int set_registers(rtu7300_t * dev, u16 value, u16 size, void *data)
{
	return usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			       RTL7300_REQ_SET_REGS, RTL7300_REQT_WRITE,
			       value, 0, data, size, HZ / 2);
}

static int set_firmware(rtu7300_t * dev, u16 value, u16 size, void *data)
{
	return usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			       RTL7300_REQ_SET_REGS, RTL7300_REQT_WRITE,
			       value, 1, data, size, HZ / 2);
}

static int get_internal_registers(rtu7300_t *dev, u16 size, u32 offset, void *data)
{
	u32 tmp=0x80000000;
	static u32 retry=0;

	if(size==1)
		tmp|=0x00000000;
	else if(size==2)
		tmp|=0x01000000;
	else if(size==4)
		tmp|=0x02000000;
	else
		return (-1);

	offset&=0x00ffffff;
	tmp|=offset;

	set_registers(dev, USB_DMA_BASE|CPU_CTRL, 4, &tmp);

	do{
		get_registers(dev, USB_DMA_BASE|CPU_CTRL, 4, &tmp);
		retry++;
		if(retry > 10000000)
			break;
	} while(tmp & 0x80000000);

	get_registers(dev, USB_DMA_BASE|CPU_RW_DATA_OUT, size, data);

	return 0;
}

static int set_internal_registers(rtu7300_t *dev, u16 size, u32 offset, void *data)
{
        u32 tmp=0xc0000000;
        u32 retry=0;

        if(size==1)
                tmp|=0x00000000;
        else if(size==2)
                tmp|=0x01000000;
        else if(size==4)
                tmp|=0x02000000;
        else
                return (-1);

        offset&=0x00ffffff;
        tmp|=offset;

	set_registers( dev, USB_DMA_BASE|CPU_RW_DATA_IN, size, data);

	set_registers(dev, USB_DMA_BASE|CPU_CTRL, 4, &tmp);

        do{
		get_registers(dev, USB_DMA_BASE|CPU_CTRL, 4, &tmp);
		retry++;
		if(retry > 10000000)
			break;
        }while(tmp&0x80000000);

        return 0;

}

static int rtu7300_reset(rtu7300_t * dev)
{
// TODO:
/*	u8 data = 0x10;
	int i = HZ;

	set_registers(dev, CR, 1, &data);
	do {
		get_registers(dev, CR, 1, &data);
	} while ((data & 0x10) && --i);

	return (i > 0) ? 1 : 0;
*/
	return 0;
}

static void free_rx_urbs(rtu7300_t *dev)
{
	int i;
	
	for (i=0; i<RX_URB_NUM; i++)
	{
		if (dev->rx_urb[i])
			usb_free_urb(dev->rx_urb[i]);
	}
}


static int alloc_rx_urbs(rtu7300_t *dev)
{
	int i;

	for (i=0; i<RX_URB_NUM; i++)
	{
		dev->rx_urb[i]= usb_alloc_urb(0, GFP_KERNEL);
		//info("rx_urb[%d]= %x", i, dev->rx_urb[i]);
		if (!dev->rx_urb[i])
		{
			free_rx_urbs(dev);
			return 1;
		}
	}
	return 0;
}

static int alloc_all_urbs(rtu7300_t * dev)
{
	if (alloc_rx_urbs(dev))
		return 1;
	dev->tx_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->tx_urb) {
		free_rx_urbs( dev);
		return 1;
	}
	dev->ctrl_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->ctrl_urb) {
		free_rx_urbs( dev);
		usb_free_urb(dev->tx_urb);
		return 1;
	}

	return 0;
}

static void free_all_urbs(rtu7300_t * dev)
{
	free_rx_urbs(dev);
	usb_free_urb(dev->tx_urb);
	usb_free_urb(dev->ctrl_urb);
}

static void unlink_all_urbs(rtu7300_t * dev)
{
	int i;

	info("unlink_all_urb");
	for (i=0; i<RX_URB_NUM; i++)
	{
		usb_unlink_urb(dev->rx_urb[i]);
	}
	usb_unlink_urb(dev->tx_urb);
	usb_unlink_urb(dev->ctrl_urb);
}

static inline struct sk_buff *pull_skb(rtu7300_t *dev)
{
	struct sk_buff *skb;
	int i;

	for (i = 0; i < RX_SKB_POOL_SIZE; i++) {
		if (dev->rx_skb_pool[i]) {
			skb = dev->rx_skb_pool[i];
			dev->rx_skb_pool[i] = NULL;
			return skb;
		}
	}
	return NULL;
}


static int usb_write_firmware(rtu7300_t * rtdev)
{	// put firmware file in /lib/firmware 
	const struct firmware *fw_entry;
	int i;
	long int fw_len;
	u8* fw_ptr;
	u8 data_arr[128];
	u32 buf;
	u16 data_arr_len = 0;
        
        dbg("++usb_write_firmware\n");
 
         if(request_firmware(&fw_entry, RT7300_IMAGE_FILE, &(rtdev->udev->dev))!=0)
         {
                 printk(KERN_ERR
                        "firmware_sample_driver: Firmware not available\n");
                 return 1;
         }

	fw_ptr = fw_entry->data;
	fw_len = fw_entry->size;

	// enable and pulse the download for lomac/cpu to download form usb
	buf = (1<<31);
	dbg("enable download %x: %l",buf, fw_len);
	set_registers(rtdev, (USB_DMA_BASE | USB_DOWNLOAD_CTRL), 4, &buf);

	// write firmware
  	for(i=0; i<= fw_len; i++)
  	{
  		data_arr[data_arr_len++] = fw_ptr[i];
		if(data_arr_len==128)
		{
			set_firmware(rtdev, (USB_DMA_BASE | USB_DOWNLOAD_DATA), data_arr_len, data_arr);
			data_arr_len=0;
		}
  	}
	if(data_arr_len > 0)
	{
		set_firmware(rtdev, (USB_DMA_BASE | USB_DOWNLOAD_DATA), data_arr_len, data_arr);
		data_arr_len=0;
	}

	buf = 0;
	set_registers(rtdev, (USB_DOWNLOAD_CTRL | USB_DMA_BASE), 4, &buf);
	release_firmware(fw_entry);  

        dbg("--usb_write_firmware\n");

	return 0;	
}

static void usb_reset_hw(rtu7300_t *dev)
{

}

static void set_ethernet_addr(rtu7300_t *dev)
{
	u8 tmp;

	tmp = 0x06;
	
	// Set MAC address to netdev->dev_addr
	dbg("++set_ethernet_addr");

	dev->netdev->dev_addr[0]=0x00;
	dev->netdev->dev_addr[1]=0xe0;
	dev->netdev->dev_addr[2]=0x4c;	
	dev->netdev->dev_addr[3]=0x00;
	dev->netdev->dev_addr[4]= 0x00;
	dev->netdev->dev_addr[5]= (u8)((u32)get_random_int() & 0xff);

	set_internal_registers(dev, 4, CPU_IO_BASE|CR_EUI48, dev->netdev->dev_addr);
	set_internal_registers(dev, 2, CPU_IO_BASE|(CR_EUI48+0x4), dev->netdev->dev_addr+4);
	set_internal_registers(dev, 1, CPU_IO_BASE|CR_EUI48_CHANGE, &tmp);

	dbg("set MAC addr down\n");
	
	get_internal_registers(dev, 4, CPU_IO_BASE|CR_EUI48,  dev->netdev->dev_addr);
	get_internal_registers(dev, 2, CPU_IO_BASE|(CR_EUI48+0x4),  dev->netdev->dev_addr+4);

	memDump(dev->netdev->dev_addr, 6, "MAC Addr:");
	dbg("--set_ethernet_addr");
}

static int rtu7300_set_mac_address(struct net_device *netdev, void *p)
{
#if 0
	struct sockaddr *addr = p;
	rtu7300_t *dev = netdev_priv(netdev);
	int i;

	info("set mac addr");
//	if (netif_running(netdev))
//		return -EBUSY;

	//for (i=0; i<netdev->addr_len, i--)

	info("mac addr= %x", addr->sa_data);

/*	memcpy(netdev->dev_addr, addr->sa_data, netdev->addr_len);
	dbg("%s: Setting MAC address to ", netdev->name);
	for (i = 0; i < 5; i++)
		dbg("%02X:", netdev->dev_addr[i]);
	dbg("%02X\n", netdev->dev_addr[i]);
	set_registers(dev, IDR, sizeof(netdev->dev_addr), netdev->dev_addr);
*/
#endif
	return 0;
}


static void usb_init_hw(rtu7300_t *dev)
{
	// USB_MPInitialize
	u32 data;

	dbg("++usb_init_hw");

	// TODO:
	usb_reset_hw(dev);
	
	get_registers(dev, (USB_SIECTL_BASE | USB_STAT), 4, &data);
	if (data&0x1)
	{
		dev->usb_high = 1; //high speed usb device
		dev->need0_lengthsize = 512;
	}
	else
	{
		dev->usb_high = 0;
		dev->need0_lengthsize = 64;
	}

	usb_write_firmware(dev);
	udelay(500);

	// config loopback mode
	data = 0x1;
	set_registers( dev, (USB_DMA_BASE | USB_LB_MODE), 1, (unsigned char *)&data);
	data = 0x1f;
        set_registers( dev, (USB_DMA_BASE | 0x3e), 1, (unsigned char *)&data);
	set_ethernet_addr(dev);

	dbg("--usb_init_hw");
}

static void send0Pkt_callback(struct urb *urb, struct pt_regs *regs)
{
	struct sk_buff *skb = (struct sk_buff *) urb->context;

	dbg("++%s", __FUNCTION__);
	
	if (urb->status)
		dbg("%s: Tx status %d", __FUNCTION__, urb->status);

	dev_kfree_skb_irq(skb);
	usb_free_urb(urb);	
}

static void write_bulk_callback(struct urb *urb, struct pt_regs *regs)
{
	rtu7300_t *dev;

	dev = urb->context;
	if (!dev)
		return;
	/* free up our allocated buffer */
	usb_buffer_free(urb->dev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);
	
	if (!netif_device_present(dev->netdev))
		return;
	if (urb->status)
		dbg("%s: Tx status %d", dev->netdev->name, urb->status);
	dev->netdev->trans_start = jiffies;
	netif_wake_queue(dev->netdev);
}

static void read_bulk_callback(struct urb *urb, struct pt_regs *regs)
{
	rtu7300_t *dev;
	unsigned pkt_len, res;
	struct sk_buff *skb = (struct sk_buff *) urb->context;
	struct my_skb_data *entry = (struct my_skb_data *)skb->cb;
	struct net_device *netdev;
	u32 rx_stat;
	u32 flags;
	u8 cur_urb;
	u8 i;

	dbg("++read_bulk_callback");

	dev = entry->dev;
	if (!dev)
	{
		err("urb->context = NULL");
		return;
	}

	cur_urb = entry->cur_urb;
	
	spin_lock_irqsave(&dev->rx_lock, flags);
	dev->rx_urb_stat[cur_urb] = 0;
	spin_unlock_irqrestore(&dev->rx_lock, flags);
	
	if (test_bit(RTU7300_UNPLUG, &dev->flags))
		return;

	netdev = dev->netdev;
	if (!netif_device_present(netdev))
		return;

	switch (urb->status) {
	case 0:
		break;
	case -ENOENT:
		warn("Rx status: ENOENT");
		return;	/* the urb is in unlink state */
	case -ETIMEDOUT:
		warn("may be reset is needed?..");
		goto goon;
	default:
		warn("Rx status %d", urb->status);
		goto goon;
	}

	/* protect against short packets (tell me why we got some?!?) */
	if (urb->actual_length < 8)
		goto goon;

	res = urb->actual_length;
	pkt_len = res - USB_RX_DESC_SIZE;

	/* check packet */
		/* Rx descriptor structure{
			USHORT          	Length:14;
		    	USHORT               CRC:1;
			USHORT            	ICV:1;

			UCHAR          		RSSI;

			UCHAR            	RSVD_0:4;
		    	UCHAR            	LS:1;
			UCHAR            	FS:1;
			UCHAR            	EOR:1;
			UCHAR            	OWN:1;

			ULONG			RxRate: 5;
			ULONG			Decripted: 1;
			ULONG			ShortPLCP: 1;
			ULONG			RSVD_1: 1;
			ULONG			TimeStamp: 20;
			ULONG			Antenna: 1;
			ULONG			PAL: 2;
			ULONG			CpuReserved: 1;

			ULONG            	RxBufferAddress;

			ULONG            	RSVD_2;
		}*/
	rx_stat =  le32_to_cpu(*(__le32 *)(urb->transfer_buffer + pkt_len+4));
	if (rx_stat >>31)
	{
		// cpu reserved
		dbg("Drop it!");
		goto goon;
	}

	memDump(skb, res, "USB recieving");

	skb_put(skb, pkt_len);
	skb->protocol = eth_type_trans(dev->rx_skb[cur_urb], netdev);
	dev->rx_skb[cur_urb] = NULL;

	netif_rx(skb);

	dev->stats.rx_packets++;
	dev->stats.rx_bytes += pkt_len;

#if 0 //for debug
	int submited_num=0;
	for(i=0; i<RX_URB_NUM; i++)
	{
		if (dev->rx_urb_stat[i])
		{
			submited_num++;
			info("1");
		}
		else
		{
			info("0");
		}
	}
	info("submitted rx num=%d", submited_num);
#endif

	spin_lock_irqsave(&dev->rx_pool_lock, flags);
	skb = pull_skb(dev);
	spin_unlock_irqrestore(&dev->rx_pool_lock, flags);

	if (!skb)
		goto resched;


	entry = (struct my_skb_data *)skb->cb;
	entry->dev = dev;
	entry->cur_urb = cur_urb;
	dev->rx_skb[entry->cur_urb] = skb;
	
//	dev->rx_skb = skb;
goon:
	usb_fill_bulk_urb(dev->rx_urb[entry->cur_urb], dev->udev, usb_rcvbulkpipe(dev->udev, 5),
		      skb->data, dev->rxbuf_size, read_bulk_callback, skb);
	spin_lock_irqsave(&dev->rx_lock, flags);
	dev->rx_urb_stat[entry->cur_urb] = 1;
	if (usb_submit_urb(dev->rx_urb[entry->cur_urb], GFP_ATOMIC)) {
		dev->rx_urb_stat[entry->cur_urb]= 0;
		dev_kfree_skb_irq(skb);
		dev->rx_skb[entry->cur_urb] = NULL;
//		set_bit(RX_URB_FAIL, &dev->flags);
		warn("skb submit fail!");
//		goto resched;
	}
	spin_unlock_irqrestore(&dev->rx_lock, flags);

#if 0
	spin_lock_irqsave(&dev->rx_pool_lock, flags);
	fill_skb_pool(dev);
	spin_unlock_irqrestore(&dev->rx_pool_lock, flags);
#endif
	
//	dbg("--read_bulk_callback");
	return;
resched:
#if 0
	// try to improve the situation that performace degrated in fixup
	skb = dev_alloc_skb(dev->rxbuf_size+ 8);
	if (skb) 
	{
		skb->dev = dev->netdev;
		skb_reserve(skb, 8);
		
		entry = (struct my_skb_data *)skb->cb;
		entry->dev = dev;
		entry->cur_urb = cur_urb;
		dev->rx_skb[entry->cur_urb] = skb;
		usb_fill_bulk_urb(dev->rx_urb[entry->cur_urb], dev->udev, usb_rcvbulkpipe(dev->udev, 5),
			      skb->data, dev->rxbuf_size, read_bulk_callback, skb);
		spin_lock_irqsave(&dev->rx_lock, flags);
		dev->rx_urb_stat[entry->cur_urb] = 1;
		if (usb_submit_urb(dev->rx_urb[entry->cur_urb], GFP_ATOMIC)) {
			dev->rx_urb_stat[entry->cur_urb]= 0;
			dev_kfree_skb_irq(skb);
			dev->rx_skb[entry->cur_urb] = NULL;
		}
		spin_unlock_irqrestore(&dev->rx_lock, flags);
	}
	//==
#endif
	if (test_bit(RX_FIX_SCHEDULE, &dev->flags))
		return;

	set_bit(RX_FIX_SCHEDULE, &dev->flags);	
//	info("schedule rx fix up");
	tasklet_schedule(&dev->tl);
}


static void rx_fixup(unsigned long data)
{
	rtu7300_t *dev;
	struct sk_buff *skb;
	struct my_skb_data *entry;
	register int i;
	u32 flags;

//	info("++rx_fixup");
	dev = (rtu7300_t *)data;

	if (!netif_device_present(dev->netdev))
		goto out;
	if (test_bit(RTU7300_UNPLUG, &dev->flags))
		goto out;

	spin_lock_irqsave(&dev->rx_pool_lock, flags);
	fill_skb_pool(dev);
	spin_unlock_irqrestore(&dev->rx_pool_lock, flags);

	clear_bit(RX_FIX_SCHEDULE, &dev->flags);

//	info("refill===");
	for(i=0; i<RX_URB_NUM; i++)
	{
		if (dev->rx_urb_stat[i])
		{
//			info("1");
			continue;
		}

//		info("0");
		spin_lock_irqsave(&dev->rx_pool_lock, flags);
		skb = pull_skb(dev);
		spin_unlock_irqrestore(&dev->rx_pool_lock, flags);
		if (skb == NULL)
			goto tlsched;
		entry = (struct my_skb_data *)skb->cb;
		entry->dev = dev;
		entry->cur_urb = i;
		dev->rx_skb[entry->cur_urb] = skb;
		usb_fill_bulk_urb(dev->rx_urb[entry->cur_urb], dev->udev, usb_rcvbulkpipe(dev->udev, 5),
			      skb->data, dev->rxbuf_size, read_bulk_callback, skb);
		spin_lock_irqsave(&dev->rx_lock, flags);
#ifdef DEBUG
		if (dev->rx_urb_stat[i] == 1)
		{
			err("duplicated use of urb");
		}
#endif
		dev->rx_urb_stat[i] = 1;
		if (usb_submit_urb(dev->rx_urb[entry->cur_urb], GFP_ATOMIC))
		{
			dev->rx_skb[entry->cur_urb] = NULL;
			dev->rx_urb_stat[i] = 0;
			dev_kfree_skb_any(skb);
			warn("skb submit fail!");
		}
		spin_unlock_irqrestore(&dev->rx_lock, flags);
	}
	dbg("--rx_fixup");
out:
	return;
tlsched:
//	info("schedule rx fix up again");
	if (test_bit(RX_FIX_SCHEDULE, &dev->flags))
		return;
	set_bit(RX_FIX_SCHEDULE, &dev->flags);
	tasklet_schedule(&dev->tl);
}

/*
**
**	network related part of the code
**
*/

static void fill_skb_pool(rtu7300_t *dev)
{
	struct sk_buff *skb;
	register int i;

	for (i = 0; i < RX_SKB_POOL_SIZE; i++) {
		if (dev->rx_skb_pool[i])
			continue;
		skb = dev_alloc_skb(dev->rxbuf_size+ 8);
		if (!skb) {
			info("there is not enough resource!!");
			return;
		}
		skb->dev = dev->netdev;
		skb_reserve(skb, 8);
		dev->rx_skb_pool[i] = skb;
	}
	
}

static void free_skb_pool(rtu7300_t *dev)
{
	int i;

	for (i = 0; i < RX_SKB_POOL_SIZE; i++)
	{
		if (dev->rx_skb_pool[i])
		{
			dev_kfree_skb(dev->rx_skb_pool[i]);
			dev->rx_skb_pool[i] = NULL;
		}
	}
}

static void send0pkt(rtu7300_t *dev)
{
	u32	*ul;
	struct urb *urb=NULL;
	struct sk_buff *skb;

	dbg("++%s: %d", __FUNCTION__, dev->need0_lengthsize);
	
	if (!(urb = usb_alloc_urb (0, GFP_ATOMIC))) {
             	err("%s: no urb", __FUNCTION__);
		return;
	}

	skb = dev_alloc_skb( USB_TX_DESC_SIZE );
	if (!skb) {
		usb_free_urb(urb);
		return;
	}
	skb->dev = dev->netdev;

	// fill Tx Desc
//	ul=(u32 *)&skb->data[0];
//	*ul=0;
	skb->data[0] = (u8)0;
	skb->data[1] = (u8)0;
	skb->data[2] = (u8)0;
	skb->data[3] = (u8)0;	

//	ul=(u32 *)&skb->data[4];
//	*ul=16;	// dummy size		
	skb->data[4]=16;
	skb->data[5]=0;
	skb->data[6]=0;
	skb->data[7]=0;

	urb->transfer_flags = URB_ZERO_PACKET;
	
	usb_fill_bulk_urb(urb, dev->udev, usb_sndbulkpipe(dev->udev, 1),
		      skb->data, 0, send0Pkt_callback, skb);

	if (usb_submit_urb(urb, GFP_ATOMIC)) {
		warn("%s: failed tx_urb", __FUNCTION__);
	} 	

	dbg("--%s", __FUNCTION__);
	return;
}

static int rtu7300_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	rtu7300_t *dev = netdev_priv(netdev);
	int count, res=0;
	u8 * buf;
	u32 *ul;
	u16 *us;
	
	dbg("++rtu7300_start_xmit");

	netif_stop_queue(netdev);
//	count = (skb->len < 60) ? 60 : skb->len;
//	count = (count & 0x3f) ? count : count + 1;
	count = skb->len;

	buf = usb_buffer_alloc(dev->udev, (count+USB_TX_DESC_SIZE), GFP_KERNEL, &dev->tx_urb->transfer_dma);
	if (!buf)
	{
		warn("can't alloc tx buf\n");
		res = -ENOMEM;
		goto tx_out;
	}
	dev->tx_buf = buf;
	memcpy(buf+USB_TX_DESC_SIZE, skb->data, count);	
 	
	// fill Tx Desc
	if (((u32)(buf)%4)==0)
	{
		ul=(u32 *)&buf[0];
		*ul=count;	

		ul=(u32 *)&buf[4];
		*ul=16;	// dummy size		
	}
	else if (((u32)(buf)%2)==0)
	{
		us = (u16 *)&buf[0];
		*us = (u16)(count&(0x0000ffff));
		us = (u16 *)&buf[2];
		*us = (u16)((count>>16)&(0x0000ffff));

		us = (u16 *)&buf[4];
		*us = 16;
		us = (u16 *)&buf[6];
		*us = 0;

	}
	else
	{
		buf[0] = (u8)(count&(0x000000ff));
		buf[1] = (u8)((count>>8)&(0x000000ff));
		buf[2] = (u8)((count>>16)&(0x000000ff));
		buf[3] = (u8)((count>>24)&(0x000000ff));	

		buf[4]=16;
		buf[5]=0;
		buf[6]=0;
		buf[7]=0;
	}
		
	usb_fill_bulk_urb(dev->tx_urb, dev->udev, usb_sndbulkpipe(dev->udev, 1),
		      buf, (count+USB_TX_DESC_SIZE), write_bulk_callback, dev);
	dev->tx_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	if ((res = usb_submit_urb(dev->tx_urb, GFP_ATOMIC))) {
		warn("failed tx_urb %d\n", res);
		dev->stats.tx_errors++;
		netif_start_queue(netdev);
	} else {
		dev->stats.tx_packets++;
		dev->stats.tx_bytes += count;
		netdev->trans_start = jiffies;
	}
	memDump(buf, count+USB_TX_DESC_SIZE, "Tx Pkt:");
	if ( (skb->len%dev->need0_lengthsize) == 0)
	{
		send0pkt(dev);
	}

tx_out:
	dev_kfree_skb(skb);
	dbg("--rtu7300_start_xmit");
	return res;
}

static struct net_device_stats *rtu7300_netdev_stats(struct net_device *dev)
{
	return &((rtu7300_t *)netdev_priv(dev))->stats;
}

static void rtu7300_tx_timeout(struct net_device *netdev)
{
	rtu7300_t *dev = netdev_priv(netdev);

	warn("%s: Tx timeout.", netdev->name);
	rtu7300_close(netdev);
// TODO:
//	unlink_tx_urbs(&dev->txq);
	dev->stats.tx_errors++;
}

// this function is called when "ifconfig ethx xxx ....."
static int rtu7300_open(struct net_device *netdev)
{
	rtu7300_t *dev = netdev_priv(netdev);
	int res = 0;
	int i;
	struct sk_buff *skb;
      	struct my_skb_data *entry;
	long flags;

	dbg("%s: enabling interface\n", netdev->name);

	usb_init_hw(dev);

	for (i=0; i<RX_URB_NUM; i++)
	{
		skb = pull_skb(dev);
		//info("rx_skb= %x", skb);
		if (!skb)
			return -ENOMEM;
		entry = (struct my_skb_data *) skb->cb;
		entry->dev = dev;
		entry->cur_urb = i;
		dev->rx_skb[i] = skb;
		usb_fill_bulk_urb(dev->rx_urb[i], dev->udev, usb_rcvbulkpipe(dev->udev, 5),
			      skb->data, dev->rxbuf_size, read_bulk_callback, skb);
		spin_lock_irqsave(&dev->rx_lock, flags);
		dev->rx_urb_stat[i] = 1; //submitted
		if ((res = usb_submit_urb(dev->rx_urb[i], GFP_KERNEL)))
		{
			warn("%s: rx_urb submit failed: %d", __FUNCTION__, res);
			dev->rx_urb_stat[i] = 0;
			dev_kfree_skb_any(skb);
			dev->rx_skb[i] = NULL;
		}
		spin_unlock_irqrestore(&dev->rx_lock, flags);
	}
	
	netif_start_queue(netdev);
//ya	set_carrier(netdev);
	netif_device_attach(netdev);

	return res;
}

static int rtu7300_close(struct net_device *netdev)
{
	rtu7300_t *dev = netdev_priv(netdev);
	int res = 0;

	info("++%s", __FUNCTION__);
	netif_device_detach(netdev);
	netif_stop_queue(netdev);
//	if (!test_bit(RTU7300_UNPLUG, &dev->flags))
//		disable_net_traffic(dev);
	unlink_all_urbs(dev);

	dbg("--%s", __FUNCTION__);
	return res;
}

static int rtu7300_change_mtu(struct net_device *netdev, int new_mtu)
{
	rtu7300_t *dev = netdev_priv(netdev);
	int res=0;
	
	/* check for invalid MTU, according to hardware limits */
	if (new_mtu < RTU7300_MIN_MTU || new_mtu > RTU7300_MAX_MTU)
		return -EINVAL;

	/* if network interface not up, no need for complexity */
	if (!netif_running(netdev)) {
		netdev->mtu = new_mtu;
		dev->rxbuf_size = netdev->mtu+100;
		free_skb_pool(dev);
		fill_skb_pool(dev);
	}
	else
	{
	// TODO: allow change during interface is up
		return -EINVAL;
	}
	return res;
}


static int rtu7300_ioctl(struct net_device *netdev, struct ifreq *rq, int cmd)
{
//	rtu7300_t *dev = netdev_priv(netdev);
//	u16 *data = (u16 *) & rq->ifr_ifru;
	int res = 0;

	switch (cmd) {
/*
	case SIOCDEVPRIVATE:
		data[0] = dev->phy;
	case SIOCDEVPRIVATE + 1:
		read_mii_word(dev, dev->phy, (data[1] & 0x1f), &data[3]);
		break;
	case SIOCDEVPRIVATE + 2:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		write_mii_word(dev, dev->phy, (data[1] & 0x1f), data[2]);
		break;
*/
	default:
		res = -EOPNOTSUPP;
	}
	
	dbg("++rtu7300_ioctl");
	return res;
}

static int rtu7300_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	rtu7300_t *dev;
	struct net_device *netdev;
	int ifnum = intf->altsetting->desc.bInterfaceNumber;
#ifdef DEBUG	
	int i;
	struct usb_host_interface *alt ;	
#endif

	dbg("++%s", __FUNCTION__);

	if (ifnum != 0)
	{
		dbg("wrong interface");
		return -ENODEV;
	}

#ifdef DEBUG
	for (i=0; i<intf->num_altsetting; i++)
	{
		alt = &intf->altsetting[i];
		dbg("== interface %d ==", i);
		dbg("Descriptor Type: %02X", alt->desc.bDescriptorType);
		dbg("Interface Num: %02X", alt->desc.bInterfaceNumber);
		dbg("Alternate Setting: %02X", alt->desc.bAlternateSetting);
		dbg("NumEndpoints: %02X", alt->desc.bNumEndpoints);
	}
#endif

	if (intf->num_altsetting > 1)
	        usb_set_interface(udev, 0, 1);

	dbg("NumEndpoints: %02X", intf->cur_altsetting->desc.bNumEndpoints);

	netdev = alloc_etherdev(sizeof(rtu7300_t));
	if (!netdev) {
		err("Out of memory");
		return -ENOMEM;
	}

	dev = netdev_priv(netdev);
	memset(dev, 0, sizeof(rtu7300_t));

	tasklet_init(&dev->tl, rx_fixup, (unsigned long)dev);
	spin_lock_init(&dev->rx_pool_lock);
	spin_lock_init(&dev->rx_lock);
	
	dev->udev = udev;
	dev->netdev = netdev;
	SET_MODULE_OWNER(netdev);
	netdev->open = rtu7300_open;
	netdev->stop = rtu7300_close;
	netdev->hard_start_xmit = rtu7300_start_xmit;
	netdev->mtu = RTL7300_MTU;
	netdev->get_stats = rtu7300_netdev_stats;
	netdev->do_ioctl = rtu7300_ioctl;	
	netdev->change_mtu = rtu7300_change_mtu;
	netdev->set_mac_address = rtu7300_set_mac_address;
	netdev->watchdog_timeo = RTL7300_TX_TIMEOUT;
	netdev->tx_timeout = rtu7300_tx_timeout;
	
	dev->rxbuf_size = netdev->mtu+100;
 // wait to be completed	
#if 0
	netdev->set_multicast_list = rtu7300_set_multicast;
	SET_ETHTOOL_OPS(netdev, &ops);
	INIT_WORK(&dev->carrier_check, check_carrier, (void *)dev);
#endif

	if (alloc_all_urbs(dev)) {
		err("out of memory");
		goto out;
	}
	if (rtu7300_reset(dev)) {
		err("couldn't reset the device");
		goto out1;
	}
	fill_skb_pool(dev);
	usb_set_intfdata(intf, dev);
	SET_NETDEV_DEV(netdev, &intf->dev);
	strcpy(netdev->name, "uwb0");
	if (register_netdev(netdev) != 0) {
		err("couldn't register the device");
		goto out2;
	}
	usb_get_dev(udev);
	info("%s: rtu7300 is detected", netdev->name);
	return 0;

out2:
	usb_set_intfdata(intf, NULL);
	free_skb_pool(dev);
out1:
	free_all_urbs(dev);
out:
	free_netdev(netdev);
	return -EIO;
}

static void rtu7300_disconnect(struct usb_interface *intf)
{
	rtu7300_t *dev = usb_get_intfdata(intf);
	int i;

	dbg("++rtu7300_disconnect");
	
	usb_set_intfdata(intf, NULL);
	if (!dev) {
		warn("trying to unregister non-existent device\n");
		return;
	}

	set_bit( RTU7300_UNPLUG, &dev->flags);
	tasklet_disable(&dev->tl);
	tasklet_kill(&dev->tl);
	unregister_netdev(dev->netdev);
	usb_put_dev(interface_to_usbdev(intf));
	unlink_all_urbs(dev);
	free_all_urbs(dev);
	free_skb_pool(dev);
	for (i=0; i<RX_URB_NUM; i++)
	{
		if (dev->rx_skb[i])
			dev_kfree_skb(dev->rx_skb[i]);
	}
	free_netdev(dev->netdev);

	dbg("--rtu7300_disconnect");
}


static struct usb_driver rtu7300_driver = {
	.name =		driver_name,
	.probe =	rtu7300_probe,
	.disconnect =	rtu7300_disconnect,
	.id_table =	rtu7300_table,
};

static int __init usb_rtu7300_init(void)
{
	pr_info("%s: %s, " DRIVER_DESC "\n", driver_name, DRIVER_VERSION);
	return usb_register(&rtu7300_driver);
}

static void __exit usb_rtu7300_exit(void)
{
	usb_deregister(&rtu7300_driver);
}

module_init(usb_rtu7300_init);
module_exit(usb_rtu7300_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
