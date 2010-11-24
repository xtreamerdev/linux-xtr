/*
   This is part of rtl8192usb OpenSource driver - v 0.1
   Released under the terms of GPL (General Public License)
   
   
   Parts of this driver are based on the rtl8192 driver skeleton 
   from Patric Schenke & Andres Salomon.

   Parts of this driver are based on the Intel Pro Wireless 2*00 GPL drivers.
   
   some ideas might be derived from David Young rtl8192 netbsd driver.
   
   Parts of the usb code are from the r8150.c driver in linux kernel
   
   Some ideas borrowed from the 8139too.c driver included in linux kernel.
   
   We (I?) want to thanks the Authors of those projecs and also the 
   Ndiswrapper's project Authors.
   
   A special big thanks goes also to Realtek corp. for their help in my 
   attempt to add RTL8187 and RTL8225 support, and to David Young also. 

	- Please note that this file is a modified version from rtl8192-sa2400 
	drv. So some other people have contributed to this project, and they are
	thanked in the rtl8192-sa2400 CHANGELOG.
*/

#ifndef CONFIG_FORCE_HARD_FLOAT
double __floatsidf (int i) { return i; }
unsigned int __fixunsdfsi (double d) { return d; }
double __adddf3(double a, double b) { return a+b; }
double __addsf3(float a, float b) { return a+b; }
double __subdf3(double a, double b) { return a-b; }
double __extendsfdf2(float a) {return a;}
#endif

#undef LOOP_TEST
#undef DUMP_RX
#undef DUMP_TX
#undef DEBUG_TX_DESC2
#undef RX_DONT_PASS_UL
#undef DEBUG_EPROM
#undef DEBUG_RX_VERBOSE
#undef DUMMY_RX
#undef DEBUG_ZERO_RX
#undef DEBUG_RX_SKB
#undef DEBUG_TX_FRAG
#undef DEBUG_RX_FRAG
#undef DEBUG_TX_FILLDESC
#undef DEBUG_TX
#undef DEBUG_IRQ
#undef DEBUG_RX
#undef DEBUG_RXALLOC
#undef DEBUG_REGISTERS
#undef DEBUG_RING
#undef DEBUG_IRQ_TASKLET
#undef DEBUG_TX_ALLOC
#undef DEBUG_TX_DESC

//#define CONFIG_RTL8192_IO_MAP
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include "r8192E_hw.h"
#include "r8192E.h"
#include "r8190_rtl8256.h" /* RTL8225 Radio frontend */
#include "r8180_93cx6.h"   /* Card EEPROM */
#include "r8192E_wx.h"
#include "r819xE_phy.h" //added by WB 4.30.2008
#include "r819xE_phyreg.h"
#include "r819xE_cmdpkt.h"
#include "r8192E_dm.h"
//#include "r8192xU_phyreg.h"
//#include <linux/usb.h>
// FIXME: check if 2.6.7 is ok

#ifdef CONFIG_PM_RTL
#include "r8192_pm.h"
#endif

#ifdef ENABLE_DOT11D
#include "dot11d.h"
#endif

//set here to open your trace code. //WB
u32 rt_global_debug_component = \
		//		COMP_INIT    	|
			//	COMP_EPROM   	|
		//		COMP_PHY	|
		//		COMP_RF		|
				COMP_FIRMWARE	|
			//	COMP_TRACE	|
		//		COMP_DOWN	|
		//		COMP_SWBW	|
		//		COMP_SEC	|
//				COMP_QOS	|
//				COMP_RATE	|
		//		COMP_RECV	|
		//		COMP_SEND	|
				COMP_POWER	|
			//	COMP_EVENTS	|
			//	COMP_RESET	|
			//	COMP_CMDPKT	|
			//	COMP_POWER_TRACKING	|
				COMP_ERR ; //always open err flags on

#ifndef PCI_VENDOR_ID_REALTEK
#define PCI_VENDOR_ID_REALTEK		0x0bda
#endif
#ifndef PCI_VENDOR_ID_NETGEAR
#define PCI_VENDOR_ID_NETGEAR		0x0846
#endif

static struct pci_device_id rtl8192_pci_id_tbl[] __devinitdata = {
	{
		.vendor = PCI_VENDOR_ID_REALTEK,
                .device = 0x8192,
                .subvendor = PCI_ANY_ID,
                .subdevice = PCI_ANY_ID,
                .driver_data = 0,
	},
	{
		.vendor = PCI_VENDOR_ID_REALTEK,
                .device = 0x8190,
                .subvendor = PCI_ANY_ID,
                .subdevice = PCI_ANY_ID,
                .driver_data = 0,
	},
	{}
};

static char* ifname = "wlan%d";
#if 0
static int hwseqnum = 0;
static int hwwep = 0;
#endif
static int channels = 0x3fff;

MODULE_LICENSE("GPL");
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
MODULE_VERSION("V 1.1");
#endif
MODULE_DEVICE_TABLE(pci, rtl8192_pci_id_tbl);
MODULE_AUTHOR("Andrea Merello <andreamrl@tiscali.it>");
MODULE_DESCRIPTION("Linux driver for Realtek RTL819x WiFi cards");

#if 0
MODULE_PARM(ifname,"s");
MODULE_PARM_DESC(devname," Net interface name, wlan%d=default");

MODULE_PARM(hwseqnum,"i");
MODULE_PARM_DESC(hwseqnum," Try to use hardware 802.11 header sequence numbers. Zero=default");

MODULE_PARM(hwwep,"i");
MODULE_PARM_DESC(hwwep," Try to use hardware WEP support. Still broken and not available on all cards");

MODULE_PARM(channels,"i");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)
module_param(ifname, charp, S_IRUGO|S_IWUSR );
//module_param(hwseqnum,int, S_IRUGO|S_IWUSR);
//module_param(hwwep,int, S_IRUGO|S_IWUSR);
module_param(channels,int, S_IRUGO|S_IWUSR);
#else
MODULE_PARM(ifname, "s");
//MODULE_PARM(hwseqnum,"i");
//MODULE_PARM(hwwep,"i");
MODULE_PARM(channels,"i");
#endif

MODULE_PARM_DESC(devname," Net interface name, wlan%d=default");
//MODULE_PARM_DESC(hwseqnum," Try to use hardware 802.11 header sequence numbers. Zero=default");
//MODULE_PARM_DESC(hwwep," Try to use hardware WEP support. Still broken and not available on all cards");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");

static int __devinit rtl8192_pci_probe(struct pci_dev *pdev,
			 const struct pci_device_id *id);
static void __devexit rtl8192_pci_disconnect(struct pci_dev *pdev);

static struct pci_driver rtl8192_pci_driver = {
	.name		= RTL819xE_MODULE_NAME,	          /* Driver name   */
	.id_table	= rtl8192_pci_id_tbl,	          /* PCI_ID table  */
	.probe		= rtl8192_pci_probe,	          /* probe fn      */
	.remove		= __devexit_p(rtl8192_pci_disconnect),	  /* remove fn     */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 5, 0)
#ifdef CONFIG_PM_RTL
	.suspend	= rtl8192E_suspend,	          /* PM suspend fn */
	.resume		= rtl8192E_resume,                 /* PM resume fn  */
#else
	.suspend	= NULL,			          /* PM suspend fn */
	.resume      	= NULL,			          /* PM resume fn  */
#endif
#endif
};

#ifdef ENABLE_DOT11D

typedef struct _CHANNEL_LIST
{
	u8	Channel[32];
	u8	Len;
}CHANNEL_LIST, *PCHANNEL_LIST;

static CHANNEL_LIST ChannelPlan[] = {
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,149,153,157,161,165},24},  		//FCC
	{{1,2,3,4,5,6,7,8,9,10,11},11},                    				//IC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64},21},  	//ETSI
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},    //Spain. Change to ETSI. 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},  	//France. Change to ETSI.
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},	//MKK					//MKK
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},//MKK1
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},	//Israel.
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},			// For 11a , TELEC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64}, 22},    //MIC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14},14}					//For Global Domain. 1-11:active scan, 12-14 passive scan. //+YJ, 080626
};

static void rtl819x_set_channel_map(u8 channel_plan, struct r8192_priv* priv)
{
	int i, max_chan=-1, min_chan=-1;
	struct ieee80211_device* ieee = priv->ieee80211;
	switch (channel_plan)
	{
		case COUNTRY_CODE_FCC:
		case COUNTRY_CODE_IC:
		case COUNTRY_CODE_ETSI:
		case COUNTRY_CODE_SPAIN:
		case COUNTRY_CODE_FRANCE:
		case COUNTRY_CODE_MKK:
		case COUNTRY_CODE_MKK1:
		case COUNTRY_CODE_ISRAEL:
		case COUNTRY_CODE_TELEC:
		case COUNTRY_CODE_MIC:
		{
			Dot11d_Init(ieee);
			ieee->bGlobalDomain = false;
                        //acturally 8225 & 8256 rf chip only support B,G,24N mode
                        if ((priv->rf_chip == RF_8225) || (priv->rf_chip == RF_8256))
			{
				min_chan = 1;
				max_chan = 14;
			}
			else
			{
				RT_TRACE(COMP_ERR, "unknown rf chip, can't set channel map in function:%s()\n", __FUNCTION__);
			}
			if (ChannelPlan[channel_plan].Len != 0){
				// Clear old channel map
				memset(GET_DOT11D_INFO(ieee)->channel_map, 0, sizeof(GET_DOT11D_INFO(ieee)->channel_map));
				// Set new channel map
				for (i=0;i<ChannelPlan[channel_plan].Len;i++) 
				{
	                                if (ChannelPlan[channel_plan].Channel[i] < min_chan || ChannelPlan[channel_plan].Channel[i] > max_chan)
					    break; 	
					GET_DOT11D_INFO(ieee)->channel_map[ChannelPlan[channel_plan].Channel[i]] = 1;
				}
			}
			break;
		}
		case COUNTRY_CODE_GLOBAL_DOMAIN:
		{
			GET_DOT11D_INFO(ieee)->bEnabled = 0; //this flag enabled to follow 11d country IE setting, otherwise, it shall follow global domain setting
			Dot11d_Reset(ieee);
			ieee->bGlobalDomain = true;
			break;
		}
		default:
			break;
	}
}
#endif


#define eqMacAddr(a,b) ( ((a)[0]==(b)[0] && (a)[1]==(b)[1] && (a)[2]==(b)[2] && (a)[3]==(b)[3] && (a)[4]==(b)[4] && (a)[5]==(b)[5]) ? 1:0 )
/* 2007/07/25 MH Defien temp tx fw info. */
TX_FWINFO_T 	Tmp_TxFwInfo;


#define 	rx_hal_is_cck_rate(_pdrvinfo)\
			(_pdrvinfo->RxRate == DESC90_RATE1M ||\
			_pdrvinfo->RxRate == DESC90_RATE2M ||\
			_pdrvinfo->RxRate == DESC90_RATE5_5M ||\
			_pdrvinfo->RxRate == DESC90_RATE11M) &&\
			!_pdrvinfo->RxHT\


static void CamResetAllEntry(struct net_device *dev)
{
	//u8 ucIndex;
	u32 ulcommand = 0;

#if 1
	ulcommand |= BIT31|BIT30;
	write_nic_dword(dev, RWCAM, ulcommand); 
#else
        for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
                CAM_mark_invalid(dev, ucIndex);
        for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
                CAM_empty_entry(dev, ucIndex);
#endif
}


void write_cam(struct net_device *dev, u8 addr, u32 data)
{
        write_nic_dword(dev, WCAMI, data);
        write_nic_dword(dev, RWCAM, BIT31|BIT16|(addr&0xff) );
}
u32 read_cam(struct net_device *dev, u8 addr)
{
        write_nic_dword(dev, RWCAM, 0x80000000|(addr&0xff) );
        return read_nic_dword(dev, 0xa8);
}

////////////////////////////////////////////////////////////
#ifdef CONFIG_RTL8180_IO_MAP

u8 read_nic_byte(struct net_device *dev, int x) 
{
        return 0xff&inb(dev->base_addr +x);
}

u32 read_nic_dword(struct net_device *dev, int x) 
{
        return inl(dev->base_addr +x);
}

u16 read_nic_word(struct net_device *dev, int x) 
{
        return inw(dev->base_addr +x);
}

void write_nic_byte(struct net_device *dev, int x,u8 y)
{
        outb(y&0xff,dev->base_addr +x);
}

void write_nic_word(struct net_device *dev, int x,u16 y)
{
        outw(y,dev->base_addr +x);
}

void write_nic_dword(struct net_device *dev, int x,u32 y)
{
        outl(y,dev->base_addr +x);
}

#else /* RTL_IO_MAP */

u8 read_nic_byte(struct net_device *dev, int x) 
{
        return 0xff&readb((u8*)dev->mem_start +x);
}

u32 read_nic_dword(struct net_device *dev, int x) 
{
        return readl((u8*)dev->mem_start +x);
}

u16 read_nic_word(struct net_device *dev, int x) 
{
        return readw((u8*)dev->mem_start +x);
}

void write_nic_byte(struct net_device *dev, int x,u8 y)
{
        writeb(y,(u8*)dev->mem_start +x);
	udelay(20);
}

void write_nic_dword(struct net_device *dev, int x,u32 y)
{
        writel(y,(u8*)dev->mem_start +x);
	udelay(20);
}

void write_nic_word(struct net_device *dev, int x,u16 y) 
{
        writew(y,(u8*)dev->mem_start +x);
	udelay(20);
}

#endif /* RTL_IO_MAP */


///////////////////////////////////////////////////////////

//u8 read_phy_cck(struct net_device *dev, u8 adr);
//u8 read_phy_ofdm(struct net_device *dev, u8 adr);
/* this might still called in what was the PHY rtl8185/rtl8192 common code 
 * plans are to possibilty turn it again in one common code...
 */
inline void force_pci_posting(struct net_device *dev)
{
}


//warning message WB
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
void rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs);
#else
irqreturn_t rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs);
#endif
#else
irqreturn_t rtl8192_interrupt(int irq, void *netdev);
#endif
//void set_nic_rxring(struct net_device *dev);
//void set_nic_txring(struct net_device *dev);
//static struct net_device_stats *rtl8192_stats(struct net_device *dev);
void rtl8192_commit(struct net_device *dev);
//void rtl8192_restart(struct net_device *dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_restart(struct work_struct *work);
//void rtl8192_rq_tx_ack(struct work_struct *work);
#else 
 void rtl8192_restart(struct net_device *dev);
// //void rtl8192_rq_tx_ack(struct net_device *dev);
 #endif

void watch_dog_timer_callback(unsigned long data);

/****************************************************************************
   -----------------------------PROCFS STUFF-------------------------
*****************************************************************************/

static struct proc_dir_entry *rtl8192_proc = NULL;



static int proc_get_stats_ap(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	struct ieee80211_network *target;
	
	int len = 0;

        list_for_each_entry(target, &ieee->network_list, list) {

		len += snprintf(page + len, count - len,
                "%s ", target->ssid);

		if(target->wpa_ie_len>0 || target->rsn_ie_len>0){
	                len += snprintf(page + len, count - len,
        	        "WPA\n");
		}
		else{
                        len += snprintf(page + len, count - len,
                        "non_WPA\n");
                }
		 
        }
	
	*eof = 1;
	return len;
}

static int proc_get_registers(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n;
			
	int max=0xff;
	
	/* This dump the current register page */
	len += snprintf(page + len, count - len,
                        "\n####################page 0##################\n ");

	for(n=0;n<=max;)
	{
		//printk( "\nD: %2x> ", n);
		len += snprintf(page + len, count - len,
			"\nD:  %2x > ",n);

		for(i=0;i<16 && n<=max;i++,n++)
		len += snprintf(page + len, count - len,
			"%2x ",read_nic_byte(dev,n));

		//	printk("%2x ",read_nic_byte(dev,n));
	}
	len += snprintf(page + len, count - len,"\n");
	len += snprintf(page + len, count - len,
                        "\n####################page 1##################\n ");
        for(n=0;n<=max;)
        {
                //printk( "\nD: %2x> ", n);
                len += snprintf(page + len, count - len,
                        "\nD:  %2x > ",n);

                for(i=0;i<16 && n<=max;i++,n++)
                len += snprintf(page + len, count - len,
                        "%2x ",read_nic_byte(dev,0x100|n));

                //      printk("%2x ",read_nic_byte(dev,n));
        }

	len += snprintf(page + len, count - len,
                        "\n####################page 3##################\n ");
        for(n=0;n<=max;)
        {
                //printk( "\nD: %2x> ", n);
                len += snprintf(page + len, count - len,
                        "\nD:  %2x > ",n);

                for(i=0;i<16 && n<=max;i++,n++)
                len += snprintf(page + len, count - len,
                        "%2x ",read_nic_byte(dev,0x300|n));

                //      printk("%2x ",read_nic_byte(dev,n));
        }

		
	*eof = 1;
	return len;

}


#if 0
static int proc_get_cck_reg(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n;
			
	int max = 0x5F;
	
	/* This dump the current register page */
	for(n=0;n<=max;)
	{
		//printk( "\nD: %2x> ", n);
		len += snprintf(page + len, count - len,
			"\nD:  %2x > ",n);

		for(i=0;i<16 && n<=max;i++,n++)
		len += snprintf(page + len, count - len,
			"%2x ",read_phy_cck(dev,n));

		//	printk("%2x ",read_nic_byte(dev,n));
	}
	len += snprintf(page + len, count - len,"\n");

		
	*eof = 1;
	return len;
}

#endif

#if 0
static int proc_get_ofdm_reg(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{

	struct net_device *dev = data;
//	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n;
			
	//int max=0xff;
	int max = 0x40;
	
	/* This dump the current register page */
	for(n=0;n<=max;)
	{
		//printk( "\nD: %2x> ", n);
		len += snprintf(page + len, count - len,
			"\nD:  %2x > ",n);

		for(i=0;i<16 && n<=max;i++,n++)
		len += snprintf(page + len, count - len,
			"%2x ",read_phy_ofdm(dev,n));

		//	printk("%2x ",read_nic_byte(dev,n));
	}
	len += snprintf(page + len, count - len,"\n");


		
	*eof = 1;
	return len;
}

#endif

#if 0
static int proc_get_stats_hw(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"NIC int: %lu\n"
		"Total int: %lu\n",
		priv->stats.ints,
		priv->stats.shints);
			
	*eof = 1;
	return len;
}
#endif

static int proc_get_stats_tx(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"TX VI priority ok int: %lu\n"
//		"TX VI priority error int: %lu\n"
		"TX VO priority ok int: %lu\n"
//		"TX VO priority error int: %lu\n"
		"TX BE priority ok int: %lu\n"
//		"TX BE priority error int: %lu\n"
		"TX BK priority ok int: %lu\n"
//		"TX BK priority error int: %lu\n"
		"TX MANAGE priority ok int: %lu\n"
//		"TX MANAGE priority error int: %lu\n"
		"TX BEACON priority ok int: %lu\n"
		"TX BEACON priority error int: %lu\n"
		"TX CMDPKT priority ok int: %lu\n"
//		"TX high priority ok int: %lu\n"
//		"TX high priority failed error int: %lu\n"
//		"TX queue resume: %lu\n"
		"TX queue stopped?: %d\n"
		"TX fifo overflow: %lu\n"
//		"TX beacon: %lu\n"
//		"TX VI queue: %d\n"
//		"TX VO queue: %d\n"
//		"TX BE queue: %d\n"
//		"TX BK queue: %d\n"
//		"TX HW queue: %d\n"
//		"TX VI dropped: %lu\n"
//		"TX VO dropped: %lu\n"
//		"TX BE dropped: %lu\n"
//		"TX BK dropped: %lu\n"
		"TX total data packets %lu\n"		
		"TX total data bytes :%lu\n",
//		"TX beacon aborted: %lu\n",
		priv->stats.txviokint,
//		priv->stats.txvierr,
		priv->stats.txvookint,
//		priv->stats.txvoerr,
		priv->stats.txbeokint,
//		priv->stats.txbeerr,
		priv->stats.txbkokint,
//		priv->stats.txbkerr,
		priv->stats.txmanageokint,
//		priv->stats.txmanageerr,
		priv->stats.txbeaconokint,
		priv->stats.txbeaconerr,
		priv->stats.txcmdpktokint,
//		priv->stats.txhpokint,
//		priv->stats.txhperr,
//		priv->stats.txresumed,
		netif_queue_stopped(dev),
		priv->stats.txoverflow,
//		priv->stats.txbeacon,
//		atomic_read(&(priv->tx_pending[VI_QUEUE])),
//		atomic_read(&(priv->tx_pending[VO_QUEUE])),
//		atomic_read(&(priv->tx_pending[BE_QUEUE])),
//		atomic_read(&(priv->tx_pending[BK_QUEUE])),
//		read_nic_byte(dev, TXFIFOCOUNT),
//		priv->stats.txvidrop,
//		priv->stats.txvodrop,
		priv->ieee80211->stats.tx_packets,
		priv->ieee80211->stats.tx_bytes


//		priv->stats.txbedrop,
//		priv->stats.txbkdrop
			//	priv->stats.txdatapkt
//		priv->stats.txbeaconerr
		);
			
	*eof = 1;
	return len;
}		



static int proc_get_stats_rx(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"RX packets: %lu\n"
		"RX desc err: %lu\n"
		"RX rx overflow error: %lu\n"
		"RX invalid urb error: %lu\n",
		priv->stats.rxint,
		priv->stats.rxrdu,
		priv->stats.rxoverflow,
		priv->stats.rxurberr);
			
	*eof = 1;
	return len;
}		

#if 1
#if WIRELESS_EXT < 17
static struct iw_statistics *r8192_get_wireless_stats(struct net_device *dev)
{
       struct r8192_priv *priv = ieee80211_priv(dev);

       return &priv->wstats;
}

#endif
#endif
void rtl8192_proc_module_init(void)
{	
	RT_TRACE(COMP_INIT, "Initializing proc filesystem");
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
	rtl8192_proc=create_proc_entry(RTL819xE_MODULE_NAME, S_IFDIR, proc_net);
#else
	rtl8192_proc=create_proc_entry(RTL819xE_MODULE_NAME, S_IFDIR, init_net.proc_net);
#endif
}


void rtl8192_proc_module_remove(void)
{
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
	remove_proc_entry(RTL819xE_MODULE_NAME, proc_net);
#else
	remove_proc_entry(RTL819xE_MODULE_NAME, init_net.proc_net);
#endif
}


void rtl8192_proc_remove_one(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	printk("dev name=======> %s\n",dev->name);

	if (priv->dir_dev) {
	//	remove_proc_entry("stats-hw", priv->dir_dev);
		remove_proc_entry("stats-tx", priv->dir_dev);
		remove_proc_entry("stats-rx", priv->dir_dev);
	//	remove_proc_entry("stats-ieee", priv->dir_dev);
		remove_proc_entry("stats-ap", priv->dir_dev);
		remove_proc_entry("registers", priv->dir_dev);
	//	remove_proc_entry("cck-registers",priv->dir_dev);
	//	remove_proc_entry("ofdm-registers",priv->dir_dev);
		//remove_proc_entry(dev->name, rtl8192_proc);
		remove_proc_entry("wlan0", rtl8192_proc);
		priv->dir_dev = NULL;
	}
}


void rtl8192_proc_init_one(struct net_device *dev)
{
	struct proc_dir_entry *e;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dir_dev = create_proc_entry(dev->name, 
					  S_IFDIR | S_IRUGO | S_IXUGO, 
					  rtl8192_proc);
	if (!priv->dir_dev) {
		RT_TRACE(COMP_ERR, "Unable to initialize /proc/net/rtl8192/%s\n",
		      dev->name);
		return;
	}
	#if 0
	e = create_proc_read_entry("stats-hw", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_hw, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-hw\n",
		      dev->name);
	}
	#endif
	e = create_proc_read_entry("stats-rx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_rx, dev);
				   
	if (!e) {
		RT_TRACE(COMP_ERR,"Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-rx\n",
		      dev->name);
	}
	
	
	e = create_proc_read_entry("stats-tx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_tx, dev);
				   
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-tx\n",
		      dev->name);
	}
	#if 0
	e = create_proc_read_entry("stats-ieee", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_ieee, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-ieee\n",
		      dev->name);
	}
	
	#endif
	
	e = create_proc_read_entry("stats-ap", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_ap, dev);
				   
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-ap\n",
		      dev->name);
	}
	
	e = create_proc_read_entry("registers", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers\n",
		      dev->name);
	}
#if 0
	e = create_proc_read_entry("cck-registers", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_cck_reg, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/cck-registers\n",
		      dev->name);
	}
	
	e = create_proc_read_entry("ofdm-registers", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_ofdm_reg, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/ofdm-registers\n",
		      dev->name);
	}
#endif
}
/****************************************************************************
   -----------------------------MISC STUFF-------------------------
*****************************************************************************/

/* this is only for debugging */
static void print_buffer(u32 *buffer, int len)
{
	int i;
	u8 *buf =(u8*)buffer;
	
	printk("ASCII BUFFER DUMP (len: %x):\n",len);
	
	for(i=0;i<len;i++)
		printk("%c",buf[i]);
		
	printk("\nBINARY BUFFER DUMP (len: %x):\n",len);
	
	for(i=0;i<len;i++)
		printk("%x",buf[i]);

	printk("\n");
}

int get_curr_tx_free_desc(struct net_device *dev, int priority)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
        u32* tail;
        u32* head;
        int ret;
#if 1
        switch (priority){
        case MGNT_QUEUE:
                head = priv->txmapringhead->desc;
                tail = priv->txmapringtail->desc;
                break;
        case BK_QUEUE:
                head = priv->txbkpringhead->desc;
                tail = priv->txbkpringtail->desc;
                break;
        case BE_QUEUE:
                head = priv->txbepringhead->desc;
                tail = priv->txbepringtail->desc;
                break;
        case VI_QUEUE:
                head = priv->txvipringhead->desc;
                tail = priv->txvipringtail->desc;
                break;
		case VO_QUEUE:
			head = priv->txvopringhead->desc;
			tail = priv->txvopringtail->desc;
			break;

		case TXCMD_QUEUE:
			head = priv->txcmdringhead->desc;
			tail = priv->txcmdringtail->desc;
			break;
                default:
                        return -1;
        }
        //DMESG("%x %x", head, tail);

        /* FIXME FIXME FIXME FIXME */
#endif

        if( head <= tail )
                ret = priv->txringcount - (tail - head)/8;
        else
                ret = (head - tail)/8;

        if(ret > priv->txringcount) 
            DMESG("BUG");                
        
        return ret;
}


short check_nic_enough_desc(struct net_device *dev, int priority)
{
        //struct r8192_priv *priv = ieee80211_priv(dev);
        //struct ieee80211_device *ieee = netdev_priv(dev);

#if 0
        int requiredbyte, required;
        //requiredbyte = priv->ieee80211->fts + sizeof(struct ieee80211_header_data);
        requiredbyte = priv->ieee80211->fts; //+ sizeof(struct ieee80211_header_data);

        if(ieee->current_network.QoS_Enable) {
                requiredbyte += 2;
        };

        required = requiredbyte / (priv->txbuffsize-4);
        if (requiredbyte % priv->txbuffsize) required++;
        /* for now we keep two free descriptor as a safety boundary 
         * between the tail and the head 
         */
#endif
 //     return (required+2 < get_curr_tx_free_desc(dev,priority));

        /* for now we keep two free descriptor as a safety boundary 
         * between the tail and the head 
         */
        return (2 < get_curr_tx_free_desc(dev,priority));
}





static void tx_timeout(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//rtl8192_commit(dev);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	schedule_work(&priv->reset_wq);
#else
	schedule_task(&priv->reset_wq);
#endif
	printk("TXTIMEOUT");
}


/* this is only for debug */
static void dump_eprom(struct net_device *dev)
{
	int i;
	for(i=0; i<0xff; i++)
		RT_TRACE(COMP_INIT, "EEPROM addr %x : %x", i, eprom_read(dev,i));
}

/* this is only for debug */
void rtl8192_dump_reg(struct net_device *dev)
{
	int i;
	int n;
	int max=0x5ff;
	
	RT_TRACE(COMP_INIT, "Dumping NIC register map");	
	
	for(n=0;n<=max;)
	{
		printk( "\nD: %2x> ", n);
		for(i=0;i<16 && n<=max;i++,n++)
			printk("%2x ",read_nic_byte(dev,n));
	}
	printk("\n");
}

/****************************************************************************
      ------------------------------HW STUFF---------------------------
*****************************************************************************/


void rtl8192_irq_enable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);	
	priv->irq_enabled = 1;
/*
	write_nic_word(dev,INTA_MASK,INTA_RXOK | INTA_RXDESCERR | INTA_RXOVERFLOW |\ 
	INTA_TXOVERFLOW | INTA_HIPRIORITYDESCERR | INTA_HIPRIORITYDESCOK |\ 
	INTA_NORMPRIORITYDESCERR | INTA_NORMPRIORITYDESCOK |\
	INTA_LOWPRIORITYDESCERR | INTA_LOWPRIORITYDESCOK | INTA_TIMEOUT);
*/
	write_nic_dword(dev,INTA_MASK, priv->irq_mask);
}


void rtl8192_irq_disable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);	

	write_nic_dword(dev,INTA_MASK,0);
	force_pci_posting(dev);
	priv->irq_enabled = 0;
}


void rtl8192_set_mode(struct net_device *dev,int mode)
{
	u8 ecmd;
	ecmd=read_nic_byte(dev, EPROM_CMD);
	ecmd=ecmd &~ EPROM_CMD_OPERATING_MODE_MASK;
	ecmd=ecmd | (mode<<EPROM_CMD_OPERATING_MODE_SHIFT);
	ecmd=ecmd &~ (1<<EPROM_CS_SHIFT);
	ecmd=ecmd &~ (1<<EPROM_CK_SHIFT);
	write_nic_byte(dev, EPROM_CMD, ecmd);
}


void rtl8192_update_msr(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 msr;
	
	msr  = read_nic_byte(dev, MSR);
	msr &= ~ MSR_LINK_MASK;
	
	/* do not change in link_state != WLAN_LINK_ASSOCIATED.
	 * msr must be updated if the state is ASSOCIATING. 
	 * this is intentional and make sense for ad-hoc and
	 * master (see the create BSS/IBSS func)
	 */
	if (priv->ieee80211->state == IEEE80211_LINKED){ 
			
		if (priv->ieee80211->iw_mode == IW_MODE_INFRA)
			msr |= (MSR_LINK_MANAGED<<MSR_LINK_SHIFT);
		else if (priv->ieee80211->iw_mode == IW_MODE_ADHOC)
			msr |= (MSR_LINK_ADHOC<<MSR_LINK_SHIFT);
		else if (priv->ieee80211->iw_mode == IW_MODE_MASTER)
			msr |= (MSR_LINK_MASTER<<MSR_LINK_SHIFT);
		
	}else
		msr |= (MSR_LINK_NONE<<MSR_LINK_SHIFT);
		
	write_nic_byte(dev, MSR, msr);
}

void rtl8192_set_chan(struct net_device *dev,short ch)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
//	u32 tx;
	RT_TRACE(COMP_RF, "=====>%s()====ch:%d\n", __FUNCTION__, ch);	
	priv->chan=ch;
	#if 0
	if(priv->ieee80211->iw_mode == IW_MODE_ADHOC || 
		priv->ieee80211->iw_mode == IW_MODE_MASTER){
	
			priv->ieee80211->link_state = WLAN_LINK_ASSOCIATED;	
			priv->ieee80211->master_chan = ch;
			rtl8192_update_beacon_ch(dev); 
		}
	#endif
	
	/* this hack should avoid frame TX during channel setting*/


//	tx = read_nic_dword(dev,TX_CONF);
//	tx &= ~TX_LOOPBACK_MASK;

#ifndef LOOP_TEST	
//	write_nic_dword(dev,TX_CONF, tx |( TX_LOOPBACK_MAC<<TX_LOOPBACK_SHIFT));

	//need to implement rf set channel here WB

	if (priv->rf_set_chan)
	priv->rf_set_chan(dev,priv->chan);
//	mdelay(10);
//	write_nic_dword(dev,TX_CONF,tx | (TX_LOOPBACK_NONE<<TX_LOOPBACK_SHIFT));
#endif
}


#if 0
static int rtl8192_rx_initiate(struct net_device*dev)
{

        struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
        struct urb *entry;
        struct sk_buff *skb;
        struct rtl8192_rx_info *info;

	/* nomal packet rx procedure */
        while (skb_queue_len(&priv->rx_queue) < MAX_RX_URB) {
                skb = __dev_alloc_skb(RX_URB_SIZE, GFP_KERNEL);
                if (!skb)
                        break;
                entry = usb_alloc_urb(0, GFP_KERNEL);
                if (!entry) {
                        kfree_skb(skb);
                        break;
                }
//		printk("nomal packet IN request!\n");
                usb_fill_bulk_urb(entry, priv->udev,
                                  usb_rcvbulkpipe(priv->udev, 3), skb->tail,
                                  RX_URB_SIZE, rtl8192_rx_isr, skb);
                info = (struct rtl8192_rx_info *) skb->cb;
                info->urb = entry;
                info->dev = dev;
		info->out_pipe = 3; //denote rx normal packet queue
                skb_queue_tail(&priv->rx_queue, skb);
                usb_submit_urb(entry, GFP_KERNEL);
        }

	/* command packet rx procedure */
        while (skb_queue_len(&priv->rx_queue) < MAX_RX_URB + 3) {
//		printk("command packet IN request!\n");
                skb = __dev_alloc_skb(RX_URB_SIZE ,GFP_KERNEL);
                if (!skb)
                        break;
                entry = usb_alloc_urb(0, GFP_KERNEL);
                if (!entry) {
                        kfree_skb(skb);
                        break;
                }
                usb_fill_bulk_urb(entry, priv->udev,
                                  usb_rcvbulkpipe(priv->udev, 9), skb->tail,
                                  RX_URB_SIZE, rtl8192_rx_isr, skb);
                info = (struct rtl8192_rx_info *) skb->cb;
                info->urb = entry;
                info->dev = dev;
		   info->out_pipe = 9; //denote rx cmd packet queue
                skb_queue_tail(&priv->rx_queue, skb);
                usb_submit_urb(entry, GFP_KERNEL);
        }

        return 0;
}
#endif

#if 0
void rtl8192_set_rxconf(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u32 rxconf;
	
	rxconf=read_nic_dword(dev,RCR);
	rxconf = rxconf &~ MAC_FILTER_MASK;
	rxconf = rxconf | (1<<ACCEPT_MNG_FRAME_SHIFT);
	rxconf = rxconf | (1<<ACCEPT_DATA_FRAME_SHIFT);
	rxconf = rxconf | (1<<ACCEPT_BCAST_FRAME_SHIFT);
	rxconf = rxconf | (1<<ACCEPT_MCAST_FRAME_SHIFT);
	//rxconf = rxconf | (1<<ACCEPT_CTL_FRAME_SHIFT);	

	if (dev->flags & IFF_PROMISC) DMESG ("NIC in promisc mode");
	
	if(priv->ieee80211->iw_mode == IW_MODE_MONITOR || \
	   dev->flags & IFF_PROMISC){
		rxconf = rxconf | (1<<ACCEPT_ALLMAC_FRAME_SHIFT);
	} /*else if(priv->ieee80211->iw_mode == IW_MODE_MASTER){
		rxconf = rxconf | (1<<ACCEPT_ALLMAC_FRAME_SHIFT);
		rxconf = rxconf | (1<<RX_CHECK_BSSID_SHIFT);
	}*/else{
		rxconf = rxconf | (1<<ACCEPT_NICMAC_FRAME_SHIFT);
		rxconf = rxconf | (1<<RX_CHECK_BSSID_SHIFT);
	}
	
	
	if(priv->ieee80211->iw_mode == IW_MODE_MONITOR){
		rxconf = rxconf | (1<<ACCEPT_ICVERR_FRAME_SHIFT);
		rxconf = rxconf | (1<<ACCEPT_PWR_FRAME_SHIFT);
	}
	
	if( priv->crcmon == 1 && priv->ieee80211->iw_mode == IW_MODE_MONITOR)
		rxconf = rxconf | (1<<ACCEPT_CRCERR_FRAME_SHIFT);
	
	
	rxconf = rxconf &~ RX_FIFO_THRESHOLD_MASK;
	rxconf = rxconf | (RX_FIFO_THRESHOLD_NONE<<RX_FIFO_THRESHOLD_SHIFT);
	rxconf = rxconf &~ MAX_RX_DMA_MASK;
	rxconf = rxconf | (MAX_RX_DMA_2048<<MAX_RX_DMA_SHIFT);

	rxconf = rxconf | (1<<RX_AUTORESETPHY_SHIFT);
	rxconf = rxconf | RCR_ONLYERLPKT;
	
//	rxconf = rxconf &~ RCR_CS_MASK;
//	rxconf = rxconf | (1<<RCR_CS_SHIFT);

	write_nic_dword(dev, RX_CONF, rxconf);	
	
	#ifdef DEBUG_RX
	DMESG("rxconf: %x %x",rxconf ,read_nic_dword(dev,RX_CONF));
	#endif
}
#endif
//wait to be removed

void set_nic_rxring(struct net_device *dev)
{
	//u8 pgreg;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	//rtl8180_set_mode(dev, EPROM_CMD_CONFIG);
	
	//rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
	
	write_nic_dword(dev, RDQDA,priv->rxringdma);
}

void fix_rx_fifo(struct net_device *dev) 
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u32 *tmp;
	struct buffer *rxbuf;
	u8 rx_desc_size;

#ifdef CONFIG_RTL8185B
//	rx_desc_size = 8; // 4*8 = 32 bytes
#else
	rx_desc_size = 4;
#endif
	
	rx_desc_size = 4;
#ifdef DEBUG_RXALLOC
	DMESG("FIXING RX FIFO");
	check_rxbuf(dev);
#endif
	
	for (tmp=priv->rxring, rxbuf=priv->rxbufferhead;
	     (tmp < (priv->rxring)+(priv->rxringcount)*rx_desc_size); 
	     tmp+=rx_desc_size,rxbuf=rxbuf->next){
		*(tmp+3) = rxbuf->dma;
		*tmp=*tmp &~ 0x3fff;
		*tmp=*tmp | priv->rxbuffersize;
		*tmp |= (1<<31);
	}
	
	
#ifdef DEBUG_RXALLOC
	DMESG("RX FIFO FIXED");
	check_rxbuf(dev);
#endif

	priv->rxringtail=priv->rxring;
	priv->rxbuffer=priv->rxbufferhead;
	priv->rx_skb_complete=1;
	set_nic_rxring(dev);
}



void rtl8192_rx_enable(struct net_device *dev)
{
	//u8 cmd;

	//struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	fix_rx_fifo(dev);

//	rtl8192_rx_initiate(dev);	
	
//	rtl8192_set_rxconf(dev);	
#if 0
	if(NIC_8187 == priv->card_8187) {
		cmd=read_nic_byte(dev,CMD);
		write_nic_byte(dev,CMD,cmd | (1<<CMD_RX_ENABLE_SHIFT));
	}
	else {
		//write_nic_dword(dev, RX_CONF, priv->ReceiveConfig);
	}
#endif
}

void set_nic_txring(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	write_nic_dword(dev, MQDA, priv->txmapringdma);
	RT_TRACE(COMP_FIRMWARE,"ring %x %x",(u32)priv->txmapringdma ,read_nic_dword(dev,MQDA));
	write_nic_dword(dev, BKQDA, priv->txbkpringdma);
	RT_TRACE(COMP_FIRMWARE,"ring %x %x",(u32)priv->txbkpringdma ,read_nic_dword(dev,BKQDA));
	write_nic_dword(dev, BEQDA, priv->txbepringdma);
	RT_TRACE(COMP_FIRMWARE,"ring %x %x",(u32)priv->txbepringdma ,read_nic_dword(dev,BEQDA));
	write_nic_dword(dev, VIQDA, priv->txvipringdma);
	RT_TRACE(COMP_FIRMWARE,"ring %x %x",(u32)priv->txvipringdma ,read_nic_dword(dev,VIQDA));
	write_nic_dword(dev, VOQDA, priv->txvopringdma);
	RT_TRACE(COMP_FIRMWARE,"ring %x %x",(u32)priv->txvopringdma,read_nic_dword(dev,VOQDA));
	write_nic_dword(dev, CQDA, priv->txcmdringdma);
	RT_TRACE(COMP_FIRMWARE,"ring %x %x",(u32)priv->txcmdringdma,read_nic_dword(dev,CQDA));
	write_nic_dword(dev, BQDA, priv->txbeaconringdma);
	RT_TRACE(COMP_FIRMWARE,"ring %x %x",(u32)priv->txbeaconringdma,read_nic_dword(dev,BQDA));
}


void fix_tx_fifo(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	ptx_ring tmp;
	int i;
#ifdef DEBUG_TX_ALLOC
	DMESG("FIXING TX FIFOs");
#endif

	for (tmp=priv->txmapring, i=0;
	     i < priv->txringcount;
	     tmp= tmp->next, i++){
		*(tmp->desc) = *(tmp->desc) &~ (1<<31);		
	}
	
	for (tmp=priv->txbkpring, i=0;
	     i < priv->txringcount; 
	     tmp=tmp->next, i++) {
		*(tmp->desc) = *(tmp->desc) &~ (1<<31);		
	}
	
	for (tmp=priv->txbepring, i=0;
	     i < priv->txringcount;
	     tmp=tmp->next, i++){
		*(tmp->desc) = *(tmp->desc) &~ (1<<31);		
	}
	for (tmp=priv->txvipring, i=0;
	     i < priv->txringcount; 
	     tmp=tmp->next, i++) {
		*(tmp->desc) = *(tmp->desc) &~ (1<<31);		
	}
	
	for (tmp=priv->txvopring, i=0;
	     i < priv->txringcount;
	     tmp=tmp->next, i++){
		*(tmp->desc) = *(tmp->desc) &~ (1<<31);		
	}

	for (tmp=priv->txcmdring, i=0;
	     i < priv->txringcount; 
	     tmp=tmp->next,i++){
		*(tmp->desc) = *(tmp->desc) &~ (1<<31);		
	}
	for (tmp=priv->txbeaconring, i=0;
	     i < priv->txbeaconcount;
	     tmp=tmp->next, i++){
		*(tmp->desc) = *(tmp->desc) &~ (1<<31);		
	}
#ifdef DEBUG_TX_ALLOC
	DMESG("TX FIFOs FIXED");
#endif
	priv->txmapringtail = priv->txmapring;
	priv->txmapringhead = priv->txmapring;
	priv->txmapbufstail = priv->txmapbufs;
	
	priv->txbkpringtail = priv->txbkpring;
	priv->txbkpringhead = priv->txbkpring;
	priv->txbkpbufstail = priv->txbkpbufs;
	
	priv->txbepringtail = priv->txbepring;
	priv->txbepringhead = priv->txbepring;
	priv->txbepbufstail = priv->txbepbufs;
	
	priv->txvipringtail = priv->txvipring;
	priv->txvipringhead = priv->txvipring;
	priv->txvipbufstail = priv->txvipbufs;
	
	priv->txvopringtail = priv->txvopring;
	priv->txvopringhead = priv->txvopring;
	priv->txvopbufstail = priv->txvopbufs;
	
	priv->txcmdringtail = priv->txcmdring;
	priv->txcmdringhead = priv->txcmdring;
	priv->txcmdbufstail = priv->txcmdbufs;
	
	priv->txbeaconringtail = priv->txbeaconring;
	priv->txbeaconbufstail = priv->txbeaconbufs;
	set_nic_txring(dev);
	
	ieee80211_reset_queue(priv->ieee80211);
	//priv->ack_tx_to_ieee = 0; 
}

void rtl8192_tx_enable(struct net_device *dev)
{
	fix_tx_fifo(dev);

#if 0
	u8 cmd;
	u8 byte;
	u32 txconf;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	//test loopback
	//	priv->TransmitConfig |= (TX_LOOPBACK_BASEBAND<<TX_LOOPBACK_SHIFT);
	if(NIC_8187B == priv->card_8187){
		write_nic_dword(dev, TX_CONF, priv->TransmitConfig);	
		byte = read_nic_byte(dev, MSR);
		byte |= MSR_LINK_ENEDCA;
		write_nic_byte(dev, MSR, byte);
	} else {
		byte = read_nic_byte(dev,CW_CONF);
		byte &= ~(1<<CW_CONF_PERPACKET_CW_SHIFT);
		byte &= ~(1<<CW_CONF_PERPACKET_RETRY_SHIFT);
		write_nic_byte(dev, CW_CONF, byte);

		byte = read_nic_byte(dev, TX_AGC_CTL);
		byte &= ~(1<<TX_AGC_CTL_PERPACKET_GAIN_SHIFT);
		byte &= ~(1<<TX_AGC_CTL_PERPACKET_ANTSEL_SHIFT);
		byte &= ~(1<<TX_AGC_CTL_FEEDBACK_ANT);
		write_nic_byte(dev, TX_AGC_CTL, byte);

		txconf= read_nic_dword(dev,TX_CONF);


		txconf = txconf &~ TX_LOOPBACK_MASK;

#ifndef LOOP_TEST
		txconf = txconf | (TX_LOOPBACK_NONE<<TX_LOOPBACK_SHIFT);
#else
		txconf = txconf | (TX_LOOPBACK_BASEBAND<<TX_LOOPBACK_SHIFT);
#endif
		txconf = txconf &~ TCR_SRL_MASK;
		txconf = txconf &~ TCR_LRL_MASK;

		txconf = txconf | (priv->retry_data<<TX_LRLRETRY_SHIFT); // long
		txconf = txconf | (priv->retry_rts<<TX_SRLRETRY_SHIFT); // short

		txconf = txconf &~ (1<<TX_NOCRC_SHIFT);

		txconf = txconf &~ TCR_MXDMA_MASK;
		txconf = txconf | (TCR_MXDMA_2048<<TCR_MXDMA_SHIFT);

		txconf = txconf | TCR_DISReqQsize;
		txconf = txconf | TCR_DISCW;
		txconf = txconf &~ TCR_SWPLCPLEN;

		txconf=txconf | (1<<TX_NOICV_SHIFT);

		write_nic_dword(dev,TX_CONF,txconf);

#ifdef DEBUG_TX
		DMESG("txconf: %x %x",txconf,read_nic_dword(dev,TX_CONF));
#endif

		cmd=read_nic_byte(dev,CMD);
		write_nic_byte(dev,CMD,cmd | (1<<CMD_TX_ENABLE_SHIFT));		
	}
#endif
}

#if 0
void rtl8192_beacon_tx_enable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask &=~(1<<TX_DMA_STOP_BEACON_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);	
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
}


void rtl8192_
_disable(struct net_device *dev) 
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_BEACON_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
}

#endif


void rtl8192_rtx_disable(struct net_device *dev)
{
	u8 cmd;
	struct r8192_priv *priv = ieee80211_priv(dev);
	//struct sk_buff *skb;
	//struct rtl8192_rx_info *info;

	cmd=read_nic_byte(dev,CMDR);
	if(!priv->ieee80211->bSupportRemoteWakeUp) {
		write_nic_byte(dev, CMDR, cmd &~ \
				(CR_TE|CR_RE));
	}
	force_pci_posting(dev);
	mdelay(10);
	if(!priv->rx_skb_complete)
		dev_kfree_skb_any(priv->rx_skb);
#if 0
	while ((skb = skb_dequeue(&priv->rx_queue))) {
		info = (struct rtl8192_rx_info *) skb->cb;
		if (!info->urb)
			continue;

		usb_kill_urb(info->urb);
		kfree_skb(skb);
	}

	if (skb_queue_len(&priv->skb_queue)) {
		printk(KERN_WARNING "skb_queue not empty\n");
	}
#endif
	skb_queue_purge(&priv->skb_queue);
	return;
}


static int alloc_tx_beacon_desc_ring(struct net_device *dev, int count)
{
	#if 0
	int i;
	u32 *tmp;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	priv->txbeaconring = (u32*)pci_alloc_consistent(priv->pdev,
					  sizeof(u32)*8*count, 
					  &priv->txbeaconringdma);
	if (!priv->txbeaconring) return -1;
	for (tmp=priv->txbeaconring,i=0;i<count;i++){
		*tmp = *tmp &~ (1<<31); // descriptor empty, owned by the drv 
		/*
		*(tmp+2) = (u32)dma_tmp;
		*(tmp+3) = bufsize;
		*/
		if(i+1<count)
			*(tmp+4) = (u32)priv->txbeaconringdma+((i+1)*8*4);
		else
			*(tmp+4) = (u32)priv->txbeaconringdma;
		
		tmp=tmp+8;
	}
	#endif
	return 0;
}


void rtl8192_reset(struct net_device *dev)
{
	
	//struct r8192_priv *priv = ieee80211_priv(dev);
	//u8 cr;


	/* make sure the analog power is on before
	 * reset, otherwise reset may fail
	 */
	 rtl8192_irq_disable(dev);
#if 0
	if(NIC_8187 == priv->card_8187) {
		rtl8192_set_anaparam(dev, RTL8225_ANAPARAM_ON);
		rtl8185_set_anaparam2(dev, RTL8225_ANAPARAM2_ON);
		rtl8192_irq_disable(dev);
		mdelay(200);
		write_nic_byte_E(dev,0x18,0x10);
		write_nic_byte_E(dev,0x18,0x11);
		write_nic_byte_E(dev,0x18,0x00);
		mdelay(200);
	}
#endif
	printk("=====>reset?\n");
#if 0
	cr=read_nic_byte(dev,CMD);
	cr = cr & 2;
	cr = cr | (1<<CMD_RST_SHIFT);
	write_nic_byte(dev,CMD,cr);
	
	force_pci_posting(dev);
	
	mdelay(200);
	
	if(read_nic_byte(dev,CMD) & (1<<CMD_RST_SHIFT)) 
		RT_TRACE(COMP_ERR, "Card reset timeout!\n");
	else 
		RT_TRACE(COMP_DOWN, "Card successfully reset\n");
#endif
#if 0	
	if(NIC_8187 == priv->card_8187) {

		printk("This is RTL8187 Reset procedure\n");
		rtl8192_set_mode(dev,EPROM_CMD_LOAD);
		force_pci_posting(dev);
		mdelay(200);

		/* after the eeprom load cycle, make sure we have
		 * correct anaparams
		 */
		rtl8192_set_anaparam(dev, RTL8225_ANAPARAM_ON);
		rtl8185_set_anaparam2(dev, RTL8225_ANAPARAM2_ON);
	}
	else
#endif
		printk("This is RTL8190P Reset procedure\n");

}

static inline u16 ieeerate2rtlrate(int rate)
{
	switch(rate){
	case 10:	
	return 0;
	case 20:
	return 1;
	case 55:
	return 2;
	case 110:
	return 3;
	case 60:
	return 4;
	case 90:
	return 5;
	case 120:
	return 6;
	case 180:
	return 7;
	case 240:
	return 8;
	case 360:
	return 9;
	case 480:
	return 10;
	case 540:
	return 11;
	default:
	return 3;
	
	}
}
static u16 rtl_rate[] = {10,20,55,110,60,90,120,180,240,360,480,540};
inline u16 rtl8192_rate2rate(short rate)
{
	if (rate >11) return 0;
	return rtl_rate[rate]; 
}
		


u32	
rtl819xusb_rx_command_packet(
	struct net_device *dev,
	struct ieee80211_rx_stats *pstats
	)
{	
	u32	status;

	//RT_TRACE(COMP_RECV, DBG_TRACE, ("---> RxCommandPacketHandle819xUsb()\n"));

	RT_TRACE(COMP_EVENTS, "---->rtl819xusb_rx_command_packet()\n");
	status = cmpk_message_handle_rx(dev, pstats);
	if (status) 
	{
		DMESG("rxcommandpackethandle819xusb: It is a command packet\n");
	}
	else
	{
		//RT_TRACE(COMP_RECV, DBG_TRACE, ("RxCommandPacketHandle819xUsb: It is not a command packet\n"));
	}
	
	//RT_TRACE(COMP_RECV, DBG_TRACE, ("<--- RxCommandPacketHandle819xUsb()\n"));
	return status;
}

#if 0
void rtl8192_tx_queues_stop(struct net_device *dev)
{
	//struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u8 dma_poll_mask = (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_HIPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_NORMPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_BEACON_SHIFT);

	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
}
#endif

void rtl8192_data_hard_stop(struct net_device *dev)
{
	//FIXME !!
	#if 0
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}


void rtl8192_data_hard_resume(struct net_device *dev)
{
	// FIXME !!
	#if 0
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask &= ~(1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}

/* this function TX data frames when the ieee80211 stack requires this.
 * It checks also if we need to stop the ieee tx queue, eventually do it
 */
void rtl8192_hard_data_xmit(struct sk_buff *skb, struct net_device *dev, int rate)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	int ret;
	unsigned long flags;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	u8 queue_index = tcb_desc->queue_index;
	/* shall not be referred by command packet */
	assert(queue_index != TXCMD_QUEUE);

	spin_lock_irqsave(&priv->tx_lock,flags);

        memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
#if 0
	tcb_desc->RATRIndex = 7;
	tcb_desc->bTxDisableRateFallBack = 1;
	tcb_desc->bTxUseDriverAssingedRate = 1;
	tcb_desc->bTxEnableFwCalcDur = 1;
#endif
	skb_push(skb, priv->ieee80211->tx_headroom);
	ret = rtl8192_tx(dev, skb);

	dev->trans_start = jiffies;
	priv->ieee80211->stats.tx_bytes+=(skb->len - priv->ieee80211->tx_headroom);
	priv->ieee80211->stats.tx_packets++;

	spin_unlock_irqrestore(&priv->tx_lock,flags);

//	return ret;
	return;
}

/* This is a rough attempt to TX a frame
 * This is called by the ieee 80211 stack to TX management frames.
 * If the ring is full packet are dropped (for data frame the queue
 * is stopped before this can happen).
 */
int rtl8192_hard_start_xmit(struct sk_buff *skb,struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	
	int ret;
	unsigned long flags;
        cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
        u8 queue_index = tcb_desc->queue_index;


	spin_lock_irqsave(&priv->tx_lock,flags);
	
        memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
	if(queue_index == TXCMD_QUEUE) {
	//	skb_push(skb, USB_HWDESC_HEADER_LEN);
		rtl819xE_tx_cmd(dev, skb);
		ret = 1;
	        spin_unlock_irqrestore(&priv->tx_lock,flags);	
		return ret;
	} else {
	//	RT_TRACE(COMP_SEND, "To send management packet\n");
		tcb_desc->RATRIndex = 7;
		tcb_desc->bTxDisableRateFallBack = 1;
		tcb_desc->bTxUseDriverAssingedRate = 1;
		tcb_desc->bTxEnableFwCalcDur = 1;
		skb_push(skb, priv->ieee80211->tx_headroom);
		ret = rtl8192_tx(dev, skb);
	}
	
	priv->ieee80211->stats.tx_bytes+=skb->len;
	priv->ieee80211->stats.tx_packets++;
	
	spin_unlock_irqrestore(&priv->tx_lock,flags);	
	
	return ret;

}


void rtl8192_try_wake_queue(struct net_device *dev, int pri);

void rtl8192_tx_isr(struct net_device *dev, int pri,short error)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

#if 1
	ptx_ring tail; //tail virtual addr
	ptx_ring head; //head virtual addr 
	ptx_ring begin;//start of ring virtual addr
	u32 *nicv = NULL; //nic pointer virtual addr
//	u32 *txdv; //packet just TXed
	u32 nic; //nic pointer physical addr
	u32 nicbegin;// start of ring physical addr
//	short txed;
	unsigned long flag;
	/* physical addr are ok on 32 bits since we set DMA mask*/
	
	int offs;
	int j,i;
	int hd;
//	if (error) priv->stats.txretry++; //tony 20060601
	spin_lock_irqsave(&priv->tx_lock,flag);
	switch(pri) {
	case MGNT_QUEUE:
		tail = priv->txmapringtail;
		begin = priv->txmapring;
		head = priv->txmapringhead;
		nic = read_nic_dword(dev,MQDA);
		nicbegin = priv->txmapringdma;
		break;

	case BK_QUEUE:
		tail = priv->txbkpringtail;
		begin = priv->txbkpring;
		head = priv->txbkpringhead;
		nic = read_nic_dword(dev,BKQDA);
		nicbegin = priv->txbkpringdma;
		break;
	  
	case BE_QUEUE:
		tail = priv->txbepringtail;
		begin = priv->txbepring;
		head = priv->txbepringhead;	
		nic = read_nic_dword(dev,BEQDA);    
		nicbegin = priv->txbepringdma;
		break;
		
	case VI_QUEUE:
		tail = priv->txvipringtail;
		begin = priv->txvipring;
		head = priv->txvipringhead;
		nic = read_nic_dword(dev,VIQDA);
		nicbegin = priv->txvipringdma;
		break;
	  
	case VO_QUEUE:
		tail = priv->txvopringtail;
		begin = priv->txvopring;
		head = priv->txvopringhead;	
		nic = read_nic_dword(dev, VOQDA);    
		nicbegin = priv->txvopringdma;
		break;
	
	case TXCMD_QUEUE:
		tail = priv->txcmdringtail;
                begin = priv->txcmdring;
                head = priv->txcmdringhead;
                nic = read_nic_dword(dev, CQDA);
                nicbegin = priv->txcmdringdma;
		break;
#if 0	
	case HI_PRIORITY:
		tail = priv->txhpringtail;
		begin = priv->txhpring;
		head = priv->txhpringhead;
		nic = read_nic_dword(dev,TX_HIGHPRIORITY_RING_ADDR);
		nicbegin = priv->txhpringdma;
		break;
#endif		
	default:
		spin_unlock_irqrestore(&priv->tx_lock,flag);
		return ;
	}  
	
	nicv = (u32*) ((nic - nicbegin) + (u8*)(begin->desc));

	if(((head->desc) <= (tail->desc) && (nicv > (tail->desc) || nicv < (head->desc))) || 
		((head->desc) > (tail->desc) && (nicv > (tail->desc) && nicv < (head->desc)))){
			if(net_ratelimit())
			printk("nic has lost pointer:nic:%x, nicbegin:%x, begin:%p, nicv:%p, head:%p, tail:%p\n", nic, nicbegin, begin, nicv, head, tail);
#ifdef DEBUG_TX_DESC
		check_tx_ring(dev,pri);
#endif
			spin_unlock_irqrestore(&priv->tx_lock,flag);
		//	rtl8192_restart(dev);
			return;
		}
	
	/* we check all the descriptors between the head and the nic,
	 * but not the currenly pointed by the nic (the next to be txed)
	 * and the previous of the pointed (might be in process ??)
	*/
	//if (head == nic) return;
	//DMESG("%x %x",head,nic);
	offs = (nic - nicbegin);
	//DMESG("%x %x %x",nic ,(u32)nicbegin, (int)nic -nicbegin);
	
	offs = offs / 8 /4;
	
	hd = ((head->desc) - (begin->desc)) /8;
	
	if(offs >= hd)
		j = offs - hd;
	else
		j = offs + (priv->txringcount -1 -hd);
	//	j= priv->txringcount -1- (hd - offs);
	//j-=2;
	if(j<0)
	{
		RT_TRACE(COMP_ERR,"%s(): BUG! j < 0\n",__FUNCTION__);
		j=0;
	}
	//if(pri == TXCMD_QUEUE)	
	//	printk("=================>%s(): j = %d,tail is 0x%x, head is 0x%x\n",__FUNCTION__,j,tail,head);
	for(i=0;i<j;i++)
	{
//		printk("+++++++++++++check status desc\n");
		if((*(head->desc)) & (1<<31))
			break;
		if(head == tail){
			printk("%s(): BUG! head == tail\n",__FUNCTION__);
			break;
		}	

#if 0 
		if(((*head)&(0x10000000)) != 0){

//			printk("++++++++++++++last desc,retry count is %d\n",((*head) & (0x000000ff)));
			priv->CurrRetryCnt += (u16)((*head) & (0x000000ff));
			if(!error)
			{
				priv->NumTxOkTotal++;
//				printk("NumTxOkTotal is %d\n",priv->NumTxOkTotal++);
			}
			//	printk("in function %s:curr_retry_count is %d\n",__FUNCTION__,((*head) & (0x000000ff)));
		}
		if(!error){
			priv->NumTxOkBytesTotal += (*(head+3)) & (0x00000fff);
		}
#endif
//		printk("in function %s:curr_txokbyte_count is %d\n",__FUNCTION__,(*(head+3)) & (0x00000fff));
		*(head->desc) = (*(head->desc)) &(~ (1<<31));
		head->nStuckCount = 0;
	
		if(((head->desc) - (begin->desc))/8 == priv->txringcount-1)
			head=begin; 
	
		else
			head=head->next;
	}	
	
	//if(pri == TXCMD_QUEUE)	
	//	printk("then : tail is 0x%x,head is 0x%x\n",tail,head);	
#if 0	
	if(nicv == begin)
		txdv = begin + (priv->txringcount -1)*8; 
	else
		txdv = nicv - 8; 
	
	txed = !(txdv[0] &(1<<31));
	
	if(txed){
		if(!(txdv[0] & (1<<15))) error = 1;
		//if(!(txdv[0] & (1<<30))) error = 1;
		if(error)DMESG("%x",txdv[0]);
 	}
#endif
	//DMESG("%x",txdv[0]);
	/* the head has been moved to the last certainly TXed 
	 * (or at least processed by the nic) packet.
	 * The driver take forcefully owning of all these packets
	 * If the packet previous of the nic pointer has been
	 * processed this doesn't matter: it will be checked 
	 * here at the next round. Anyway if no more packet are 
	 * TXed no memory leak occour at all.
	 */
	 
	switch(pri) {
	case MGNT_QUEUE:
		priv->txmapringhead = head;

#if 0	
			//printk("1==========================================> priority check!\n");
		if(priv->ack_tx_to_ieee){
				// try to implement power-save mode 2008.1.22
		//	printk("2==========================================> priority check!\n");
			if(rtl8180_is_tx_queue_empty(dev)){
			//	printk("tx queue empty, after send null sleep packet, try to sleep !\n");
				priv->ack_tx_to_ieee = 0;
				ieee80211_ps_tx_ack(priv->ieee80211,!error);
			}
		}

#endif			
		break;
	case BK_QUEUE:
		priv->txbkpringhead = head;
		break;
		
	case BE_QUEUE:
		priv->txbepringhead = head;
		break;
		
	case VI_QUEUE:
		priv->txvipringhead = head;
		break;

	case VO_QUEUE:
		priv->txvopringhead = head;
		break;

	case TXCMD_QUEUE:
		priv->txcmdringhead = head;
		break;
#if 0	
	case HI_PRIORITY:
		priv->txhpringhead = head;
		break;
#endif
	}

#if 0
	if(priv->txmapringhead == priv->txmapringtail)
		printk("----------------------->haha no tx buffer\n");
	else
		printk("-----------------------> oh no ! tx buffered\n");
#endif
	/*DMESG("%x %x %x", (priv->txnpringhead - priv->txnpring) /8 , 
		(priv->txnpringtail - priv->txnpring) /8,
		offs );
	*/
	//while((/* skb waitqueue not empty */)&&(/* there's awailable tx descriptor */)) 

	tasklet_schedule(&priv->irq_tx_tasklet);
	
	spin_unlock_irqrestore(&priv->tx_lock,flag);
#endif
}

void rtl8192_beacon_stop(struct net_device *dev)
{
#if 0
	u8 msr, msrm, msr2;
	struct r8192_priv *priv = ieee80211_priv(dev);

	msr  = read_nic_byte(dev, MSR);
	msrm = msr & MSR_LINK_MASK;
	msr2 = msr & ~MSR_LINK_MASK;

	if ((msrm == (MSR_LINK_ADHOC<<MSR_LINK_SHIFT) ||
		(msrm == (MSR_LINK_MASTER<<MSR_LINK_SHIFT)))){
		write_nic_byte(dev, MSR, msr2 | MSR_LINK_NONE);
		write_nic_byte(dev, MSR, msr);	
	}
#endif
}
void rtl8192_config_rate(struct net_device* dev, u16* rate_config)
{
	 struct r8192_priv *priv = ieee80211_priv(dev);
	 struct ieee80211_network *net;
	 u8 i=0, basic_rate = 0;
	 net = & priv->ieee80211->current_network;
	 
	 for (i=0; i<net->rates_len; i++)
	 {
		 basic_rate = net->rates[i]&0x7f;
		 switch(basic_rate)
		 {
			 case MGN_1M:	*rate_config |= RRSR_1M;	break;
			 case MGN_2M:	*rate_config |= RRSR_2M;	break;
			 case MGN_5_5M:	*rate_config |= RRSR_5_5M;	break;
			 case MGN_11M:	*rate_config |= RRSR_11M;	break;
			 case MGN_6M:	*rate_config |= RRSR_6M;	break;
			 case MGN_9M:	*rate_config |= RRSR_9M;	break;
			 case MGN_12M:	*rate_config |= RRSR_12M;	break;
			 case MGN_18M:	*rate_config |= RRSR_18M;	break;
			 case MGN_24M:	*rate_config |= RRSR_24M;	break;
			 case MGN_36M:	*rate_config |= RRSR_36M;	break;
			 case MGN_48M:	*rate_config |= RRSR_48M;	break;
			 case MGN_54M:	*rate_config |= RRSR_54M;	break;
		 }
	 }
	 for (i=0; i<net->rates_ex_len; i++)
	 {
		 basic_rate = net->rates_ex[i]&0x7f;
		 switch(basic_rate)
		 {
			 case MGN_1M:	*rate_config |= RRSR_1M;	break;
			 case MGN_2M:	*rate_config |= RRSR_2M;	break;
			 case MGN_5_5M:	*rate_config |= RRSR_5_5M;	break;
			 case MGN_11M:	*rate_config |= RRSR_11M;	break;
			 case MGN_6M:	*rate_config |= RRSR_6M;	break;
			 case MGN_9M:	*rate_config |= RRSR_9M;	break;
			 case MGN_12M:	*rate_config |= RRSR_12M;	break;
			 case MGN_18M:	*rate_config |= RRSR_18M;	break;
			 case MGN_24M:	*rate_config |= RRSR_24M;	break;
			 case MGN_36M:	*rate_config |= RRSR_36M;	break;
			 case MGN_48M:	*rate_config |= RRSR_48M;	break;
			 case MGN_54M:	*rate_config |= RRSR_54M;	break;
		 }
	 }
}
			 
	 
#define SHORT_SLOT_TIME 9
#define NON_SHORT_SLOT_TIME 20

void rtl8192_update_cap(struct net_device* dev, u16 cap)
{
	u32 tmp = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_network *net = &priv->ieee80211->current_network;
	priv->short_preamble = cap & WLAN_CAPABILITY_SHORT_PREAMBLE;
	tmp = priv->basic_rate;
	if (priv->short_preamble)
		tmp |= BRSR_AckShortPmb;
	write_nic_dword(dev, RRSR, tmp);

	if (net->mode & (IEEE_G|IEEE_N_24G))
	{
		u8 slot_time = 0;
		if ((cap & WLAN_CAPABILITY_SHORT_SLOT)&&(!priv->ieee80211->pHTInfo->bCurrentRT2RTLongSlotTime))
		{//short slot time
			slot_time = SHORT_SLOT_TIME;
		}
		else //long slot time
			slot_time = NON_SHORT_SLOT_TIME;
		priv->slot_time = slot_time;
		write_nic_byte(dev, SLOT_TIME, slot_time);
	}

}
void rtl8192_net_update(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_network *net;
	u16 BcnTimeCfg = 0, BcnCW = 6, BcnIFS = 0xf;
	u16 rate_config = 0;
	net = &priv->ieee80211->current_network;
	//update Basic rate: RR, BRSR
	rtl8192_config_rate(dev, &rate_config);	
	// 2007.01.16, by Emily
	// Select RRSR (in Legacy-OFDM and CCK)
	// For 8190, we select only 24M, 12M, 6M, 11M, 5.5M, 2M, and 1M from the Basic rate.
	// We do not use other rates.
	 priv->basic_rate = rate_config &= 0x15f;
	//BSSID
	write_nic_dword(dev,BSSIDR,((u32*)net->bssid)[0]);
	write_nic_word(dev,BSSIDR+4,((u16*)net->bssid)[2]);
#if 0 
	//MSR
	rtl8192_update_msr(dev);
#endif


//	rtl8192_update_cap(dev, net->capability);
	if (priv->ieee80211->iw_mode == IW_MODE_ADHOC)
	{
		write_nic_word(dev, ATIMWND, 2);
		write_nic_word(dev, BCN_DMATIME, 256);	
	write_nic_word(dev, BCN_INTERVAL, net->beacon_interval);
	//	write_nic_word(dev, BcnIntTime, 100);
		write_nic_word(dev, BCN_DRV_EARLY_INT, 10);
		write_nic_byte(dev, BCN_ERR_THRESH, 100);

		BcnTimeCfg |= (BcnCW<<BCN_TCFG_CW_SHIFT);
	// TODO: BcnIFS may required to be changed on ASIC
	 	BcnTimeCfg |= BcnIFS<<BCN_TCFG_IFS;

		write_nic_word(dev, BCN_TCFG, BcnTimeCfg);
	}
	

}

void rtl819xusb_beacon_tx(struct net_device *dev,u16  tx_rate)
{

#if 0
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct sk_buff *skb;
	int i = 0;
	//u8 cr;

	rtl8192_net_update(dev);

	skb = ieee80211_get_beacon(priv->ieee80211);
		if(!skb){ 
			DMESG("not enought memory for allocating beacon");
			return;
		}


		write_nic_byte(dev, BQREQ, read_nic_byte(dev, BQREQ) | (1<<7));

		i=0;
		//while(!read_nic_byte(dev,BQREQ & (1<<7)))
		while( (read_nic_byte(dev, BQREQ) & (1<<7)) == 0 )
		{
			msleep_interruptible_rtl(HZ/2);
			if(i++ > 10){
				DMESGW("get stuck to wait HW beacon to be ready");
				return ;
			}
		}
	skb->cb[0] = 0;//NORM_PRIORITY;
	skb->cb[1] = 0; //morefragment = 0
	skb->cb[2] = ieeerate2rtlrate(tx_rate);

	rtl8192_tx(dev,skb);

#endif
}

inline u8 rtl8192_IsWirelessBMode(u16 rate)
{
	if( ((rate <= 110) && (rate != 60) && (rate != 90)) || (rate == 220) )
		return 1;
	else return 0;
}

static u16 N_DBPSOfRate(u16 DataRate);

static u16 ComputeTxTime( 
	u16		FrameLength,
	u16		DataRate,
	u8		bManagementFrame,
	u8		bShortPreamble
)
{
	u16	FrameTime;
	u16	N_DBPS;
	u16	Ceiling;

	if( rtl8192_IsWirelessBMode(DataRate) )
	{
		if( bManagementFrame || !bShortPreamble || DataRate == 10 )
		{	// long preamble
			FrameTime = (u16)(144+48+(FrameLength*8/(DataRate/10)));		
		}
		else
		{	// Short preamble
			FrameTime = (u16)(72+24+(FrameLength*8/(DataRate/10)));
		}
		if( ( FrameLength*8 % (DataRate/10) ) != 0 ) //Get the Ceilling
				FrameTime ++;
	} else {	//802.11g DSSS-OFDM PLCP length field calculation.
		N_DBPS = N_DBPSOfRate(DataRate);
		Ceiling = (16 + 8*FrameLength + 6) / N_DBPS 
				+ (((16 + 8*FrameLength + 6) % N_DBPS) ? 1 : 0);
		FrameTime = (u16)(16 + 4 + 4*Ceiling + 6);
	}
	return FrameTime;
}

static u16 N_DBPSOfRate(u16 DataRate)
{
	 u16 N_DBPS = 24;
	 
	 switch(DataRate)
	 {
	 case 60:
	  N_DBPS = 24;
	  break;
	 
	 case 90:
	  N_DBPS = 36;
	  break;
	 
	 case 120:
	  N_DBPS = 48;
	  break;
	 
	 case 180:
	  N_DBPS = 72;
	  break;
	 
	 case 240:
	  N_DBPS = 96;
	  break;
	 
	 case 360:
	  N_DBPS = 144;
	  break;
	 
	 case 480:
	  N_DBPS = 192;
	  break;
	 
	 case 540:
	  N_DBPS = 216;
	  break;
	 
	 default:
	  break;
	 }
	 
	 return N_DBPS;
}

unsigned int txqueue2outpipe(unsigned int tx_queue) {
	unsigned int outpipe = 0x04;	

	switch (tx_queue) {
		case VO_QUEUE://EP4
		   outpipe = 0x04;
		break;

		case VI_QUEUE://EP5
		   outpipe = 0x05;
		break;

		case BE_QUEUE://EP6
		   outpipe = 0x06;
		break;

		case BK_QUEUE://EP7
		   outpipe = 0x07;
		break;

		case HCCA_QUEUE://EP8
		   outpipe = 0x08;
		break;

		case BEACON_QUEUE://EPA
		   outpipe = 0x0A;
		break;

		case HIGH_QUEUE://EPB
		   outpipe = 0x0B;

		case MGNT_QUEUE://EPC
		   outpipe = 0x0C;
		break;

		case TXCMD_QUEUE://EPD
		   outpipe = 0x0D;
		break;

		default:
		  printk("Unknow queue index!\n");
		break;
	}

	return outpipe;
}

void rtl819xE_tx_cmd(struct net_device *dev, struct sk_buff *skb)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	//u8 queue_index = tcb_desc->queue_index;
	ptx_ring tail;//,*temp_tail;
	ptx_ring begin;
	u32 *buf;
	int i;
	int remain;
	int buflen;
	int count;
	static u8 txcmdcount=0;
	struct buffer* buflist;
	tx_desc_cmd_819x_pci *pdesc = NULL;
	tail=priv->txcmdringtail;
	begin=priv->txcmdring;
	buflist = priv->txcmdbufstail;
	count = priv->txringcount;
	txcmdcount++;	
	//printk("=======================>tx cmd %d\n",txcmdcount);
	pdesc = (tx_desc_cmd_819x_pci *)(tail->desc);

	if(!buflist){
			printk("TX buffer error, cannot TX frames. \n");
			//spin_unlock_irqrestore(&priv->tx_lock,flags);
			return ;
	}
	buf=buflist->buf;
	buflen=priv->txfwbuffersize; 
	remain=skb->len;
	if(*(tail->desc) & (1<<31))
	{
		printk("No more TX desc, returning \n");
		priv->stats.txrdu++;
	}

	memset(pdesc,0,12);
	pdesc->LINIP = tcb_desc->bLastIniPkt;
	pdesc->FirstSeg = 1;//first segment;
	pdesc->LastSeg = 1;//first segment;
	pdesc->CmdInit = tcb_desc->bCmdOrInit;
	pdesc->TxBufferSize = tcb_desc->txbuf_size;
	for(i=0;i<buflen&&remain>0;i++,remain--){
		((u8*)buf)[i] = skb->data[i];
		if(remain == 4 && i+4 >= buflen) {
			printk("error: there must be some where wrong\n");
			break;
		}
	}
	pdesc->OWN = 1;
#ifdef JOHN_DUMP_TXDESC
{	int i;
	printk("<Tx descriptor>");
	for (i = 0; i < 8; i++)
		printk("%8x ", (tail->desc)[i]);
	printk("\n");
}
#endif
	if(((tail->desc) - (begin->desc))/8 == count-1)
		tail=begin; 
	else
		tail=tail->next;
	buflist=buflist->next;
	mb();
	priv->txcmdringtail=tail;
	priv->txcmdbufstail=buflist;
	write_nic_word(dev,TPPoll,TPPoll_CQ);//0x01<<5);
	return;
}

/*
 * Mapping Software/Hardware descriptor queue id to "Queue Select Field"
 * in TxFwInfo data structure
 * 2006.10.30 by Emily
 *
 * \param QUEUEID       Software Queue       
*/
u8 MapHwQueueToFirmwareQueue(u8 QueueID)
{
	u8 QueueSelect = 0x0;       //defualt set to 

	switch(QueueID) {
		case BE_QUEUE:
			QueueSelect = QSLT_BE;  //or QSelect = pTcb->priority;
			break;

		case BK_QUEUE:
			QueueSelect = QSLT_BK;  //or QSelect = pTcb->priority;
			break;

		case VO_QUEUE:
			QueueSelect = QSLT_VO;  //or QSelect = pTcb->priority;
			break;

		case VI_QUEUE:
			QueueSelect = QSLT_VI;  //or QSelect = pTcb->priority;  
			break;
		case MGNT_QUEUE:
			QueueSelect = QSLT_MGNT;
			break;

		case BEACON_QUEUE:
			QueueSelect = QSLT_BEACON;
			break;

			// TODO: 2006.10.30 mark other queue selection until we verify it is OK
			// TODO: Remove Assertions 
//#if (RTL819X_FPGA_VER & RTL819X_FPGA_GUANGAN_070502)
		case TXCMD_QUEUE:
			QueueSelect = QSLT_CMD;
			break;
//#endif
		case HIGH_QUEUE:
			//QueueSelect = QSLT_HIGH;
			//break;

		default:
			RT_TRACE(COMP_ERR, "TransmitTCB(): Impossible Queue Selection: %d \n", QueueID);
			break;
	}
	return QueueSelect;
}

u8 MRateToHwRate8190Pci(u8 rate)
{
	u8  ret = DESC90_RATE1M;

	switch(rate) {
		case MGN_1M:	ret = DESC90_RATE1M;		break;
		case MGN_2M:	ret = DESC90_RATE2M;		break;
		case MGN_5_5M:	ret = DESC90_RATE5_5M;	break;
		case MGN_11M:	ret = DESC90_RATE11M;	break;
		case MGN_6M:	ret = DESC90_RATE6M;		break;
		case MGN_9M:	ret = DESC90_RATE9M;		break;
		case MGN_12M:	ret = DESC90_RATE12M;	break;
		case MGN_18M:	ret = DESC90_RATE18M;	break;
		case MGN_24M:	ret = DESC90_RATE24M;	break;
		case MGN_36M:	ret = DESC90_RATE36M;	break;
		case MGN_48M:	ret = DESC90_RATE48M;	break;
		case MGN_54M:	ret = DESC90_RATE54M;	break;

		// HT rate since here
		case MGN_MCS0:	ret = DESC90_RATEMCS0;	break;
		case MGN_MCS1:	ret = DESC90_RATEMCS1;	break;
		case MGN_MCS2:	ret = DESC90_RATEMCS2;	break;
		case MGN_MCS3:	ret = DESC90_RATEMCS3;	break;
		case MGN_MCS4:	ret = DESC90_RATEMCS4;	break;
		case MGN_MCS5:	ret = DESC90_RATEMCS5;	break;
		case MGN_MCS6:	ret = DESC90_RATEMCS6;	break;
		case MGN_MCS7:	ret = DESC90_RATEMCS7;	break;
		case MGN_MCS8:	ret = DESC90_RATEMCS8;	break;
		case MGN_MCS9:	ret = DESC90_RATEMCS9;	break;
		case MGN_MCS10:	ret = DESC90_RATEMCS10;	break;
		case MGN_MCS11:	ret = DESC90_RATEMCS11;	break;
		case MGN_MCS12:	ret = DESC90_RATEMCS12;	break;
		case MGN_MCS13:	ret = DESC90_RATEMCS13;	break;
		case MGN_MCS14:	ret = DESC90_RATEMCS14;	break;
		case MGN_MCS15:	ret = DESC90_RATEMCS15;	break;
		case (0x80|0x20): ret = DESC90_RATEMCS32; break;

		default:       break;
	}
	return ret;
}


u8 QueryIsShort(u8 TxHT, u8 TxRate, cb_desc *tcb_desc)
{
	u8   tmp_Short;

	tmp_Short = (TxHT==1)?((tcb_desc->bUseShortGI)?1:0):((tcb_desc->bUseShortPreamble)?1:0);

	if(TxHT==1 && TxRate != DESC90_RATEMCS15)
		tmp_Short = 0;

	return tmp_Short;
}

/* 
 * The tx procedure is just as following, 
 * skb->cb will contain all the following information,
 * priority, morefrag, rate, &dev.
 * */
short rtl8192_tx(struct net_device *dev, struct sk_buff* skb)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	ptx_ring tail,temp_tail;
	ptx_ring begin;
	u32 *buf;
	//u8* databuf;
	int i;
	int remain;
	int buflen;
	int count;
	u8* pda_addr = NULL;
	bool  multi_addr=false,broad_addr=false,uni_addr=false;
	struct buffer* buflist;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	unsigned int queueindex;
	tx_desc_819x_pci *pdesc = NULL;

	queueindex = tcb_desc->queue_index;
	switch(queueindex) {
		case MGNT_QUEUE:
			tail=priv->txmapringtail;
			begin=priv->txmapring;
			buflist = priv->txmapbufstail;
			count = priv->txringcount;	    
			break;

		case BK_QUEUE:
			tail=priv->txbkpringtail;
			begin=priv->txbkpring;
			buflist = priv->txbkpbufstail;	
			count = priv->txringcount;    
			break;

		case BE_QUEUE:
			tail=priv->txbepringtail;
			begin=priv->txbepring;
			buflist = priv->txbepbufstail;
			count = priv->txringcount;	    
			break;

		case VI_QUEUE:
			tail=priv->txvipringtail;
			begin=priv->txvipring;
			buflist = priv->txvipbufstail;	
			count = priv->txringcount;    
			break;

		case VO_QUEUE:
			tail=priv->txvopringtail;
			begin=priv->txvopring;
			buflist = priv->txvopbufstail;
			count = priv->txringcount;	    
			break;

		case BEACON_QUEUE:
			tail=priv->txbeaconringtail;
			begin=priv->txbeaconring;
			buflist = priv->txbeaconbufstail;
			count = priv->txbeaconcount;	    
			break;

		default:
			return -1;
			break;
	}

	pdesc = (tx_desc_819x_pci *)(tail->desc);
#if 0
	{
		int i;
		printk("=============>TX desc :%d\n",sizeof(tx_desc_819x_pci));
		for(i=0;i<sizeof(tx_desc_819x_pci);i++)
			printk("%2x ",((u8*)(pdesc))[i]);
		printk("\n");
	}
#endif
	buflen=priv->txbuffsize; 
	remain=skb->len;
	temp_tail = tail;

	pda_addr = ((u8*)skb->data) + sizeof(TX_FWINFO_8190PCI);
	if(is_multicast_ether_addr(pda_addr))
		multi_addr = true;
	else if(is_broadcast_ether_addr(pda_addr))
		broad_addr = true;
	else
		uni_addr = true;

	if(uni_addr)
		priv->stats.txbytesunicast += (u8)(skb->len) - sizeof(TX_FWINFO_8190PCI);
	else if(multi_addr)
		priv->stats.txbytesmulticast +=(u8)(skb->len) - sizeof(TX_FWINFO_8190PCI);
	else 
		priv->stats.txbytesbroadcast += (u8)(skb->len) - sizeof(TX_FWINFO_8190PCI);

	while(remain !=0){
		mb();
		if(!buflist){
			RT_TRACE(COMP_ERR,"TX buffer error, cannot TX frames. pri %d.", queueindex);
			//spin_unlock_irqrestore(&priv->tx_lock,flags);
			return -1;
		}
		buf=buflist->buf;
		if((pdesc->OWN == 1) && (queueindex != BEACON_QUEUE))
		{
			RT_TRACE(COMP_ERR,"No more TX desc, returning %x of %x",
				remain,skb->len);
			return remain;
		}
		if(pdesc->OWN == 1)
			printk("=========>err : NO more TX desc\n");
		memset((u8*)pdesc,0,12);
		//if(remain==(skb->len-sizeof(TX_FWINFO_8190PCI))) 
		if(1) 
		{
			PTX_FWINFO_8190PCI	pTxFwInfo = NULL;
			pTxFwInfo = (PTX_FWINFO_8190PCI)skb->data;
			memset(pTxFwInfo,0,sizeof(TX_FWINFO_8190PCI));
			pTxFwInfo->TxHT = (tcb_desc->data_rate&0x80)?1:0;
			pTxFwInfo->TxRate = MRateToHwRate8190Pci((u8)tcb_desc->data_rate);
			pTxFwInfo->EnableCPUDur = tcb_desc->bTxEnableFwCalcDur;
			pTxFwInfo->Short	= QueryIsShort(pTxFwInfo->TxHT, pTxFwInfo->TxRate, tcb_desc);
			//
			// Aggregation related
			//
			if(tcb_desc->bAMPDUEnable)
			{
				pTxFwInfo->AllowAggregation = 1;
				pTxFwInfo->RxMF = tcb_desc->ampdu_factor;
				pTxFwInfo->RxAMD = tcb_desc->ampdu_density;
			}
			else
			{
				pTxFwInfo->AllowAggregation = 0;
				pTxFwInfo->RxMF = 0;
				pTxFwInfo->RxAMD = 0;
			}
			
			//
			// Protection mode related
			//
			pTxFwInfo->RtsEnable =	(tcb_desc->bRTSEnable)?1:0;
			pTxFwInfo->CtsEnable =	(tcb_desc->bCTSEnable)?1:0;
			pTxFwInfo->RtsSTBC =	(tcb_desc->bRTSSTBC)?1:0;
			pTxFwInfo->RtsHT=		(tcb_desc->rts_rate&0x80)?1:0;
			pTxFwInfo->RtsRate =		MRateToHwRate8190Pci((u8)tcb_desc->rts_rate);
			pTxFwInfo->RtsBandwidth = 0;
			pTxFwInfo->RtsSubcarrier = tcb_desc->RTSSC;
			pTxFwInfo->RtsShort =	(pTxFwInfo->RtsHT==0)?(tcb_desc->bRTSUseShortPreamble?1:0):(tcb_desc->bRTSUseShortGI?1:0);
			//
			// Set Bandwidth and sub-channel settings.
			//
			if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
			{
				if(tcb_desc->bPacketBW)
				{
					pTxFwInfo->TxBandwidth = 1;
					#ifdef RTL8190P
					pTxFwInfo->TxSubCarrier = 3;
					#else
					pTxFwInfo->TxSubCarrier = 0;	//By SD3's Jerry suggestion, use duplicated mode, cosa 04012008
					#endif
				}
				else
				{
					pTxFwInfo->TxBandwidth = 0;
					pTxFwInfo->TxSubCarrier = priv->nCur40MhzPrimeSC;
				}
			}
			else
			{
				pTxFwInfo->TxBandwidth = 0;
				pTxFwInfo->TxSubCarrier = 0;
			}

			
			/* 2007/07/25 MH  Copy current TX FW info.*/
			memcpy((void*)(&Tmp_TxFwInfo), (void*)(pTxFwInfo), sizeof(TX_FWINFO_8190PCI));
			if (0)
			{
				printk("&&&&&&&&&&&&&&&&&&&&&&====>print out fwinf\n");
				printk("===>enable fwcacl:%d\n", Tmp_TxFwInfo.EnableCPUDur);
				printk("===>RTS STBC:%d\n", Tmp_TxFwInfo.RtsSTBC);
				printk("===>RTS Subcarrier:%d\n", Tmp_TxFwInfo.RtsSubcarrier);
				printk("===>Allow Aggregation:%d\n", Tmp_TxFwInfo.AllowAggregation);
				printk("===>TX HT bit:%d\n", Tmp_TxFwInfo.TxHT);
				printk("===>Tx rate:%d\n", Tmp_TxFwInfo.TxRate);
				printk("===>Received AMPDU Density:%d\n", Tmp_TxFwInfo.RxAMD);
				printk("===>Received MPDU Factor:%d\n", Tmp_TxFwInfo.RxMF);
				printk("===>TxBandwidth:%d\n", Tmp_TxFwInfo.TxBandwidth);
				printk("===>TxSubCarrier:%d\n", Tmp_TxFwInfo.TxSubCarrier);

				printk("<=====**********************out of print\n");

			}

			//(3)Fill necessary field in First Descriptor
			/*DWORD 0*/		
			pdesc->LINIP = 0;
			pdesc->CmdInit = 1;
			pdesc->Offset = sizeof(TX_FWINFO_8190PCI) + 8; //We must add 8!! Emily
			pdesc->PktSize = (u16)skb->len-sizeof(TX_FWINFO_8190PCI);


			/*DWORD 1*/
			pdesc->SecCAMID= 0;
			pdesc->RATid = tcb_desc->RATRIndex;
		
#if 0
	/* Fill security related */
	if( pTcb->bEncrypt && !Adapter->MgntInfo.SecurityInfo.SWTxEncryptFlag)
	{
		EncAlg = SecGetEncryptionOverhead(
				Adapter,
				&EncryptionMPDUHeadOverhead,
				&EncryptionMPDUTailOverhead,
				NULL,
				NULL,
				FALSE,
				FALSE);
		//2004/07/22, kcwu, EncryptionMPDUHeadOverhead has been added in previous code
		//MPDUOverhead = EncryptionMPDUHeadOverhead + EncryptionMPDUTailOverhead;
		MPDUOverhead = EncryptionMPDUTailOverhead;
		tx_desc->NoEnc = 0;
		RT_TRACE(COMP_SEC, DBG_LOUD, ("******We in the loop SecCAMID is %d SecDescAssign is %d The Sec is %d********\n",tx_desc->SecCAMID,tx_desc->SecDescAssign,EncAlg));
		//CamDumpAll(Adapter);
	}
	else
#endif
			{
				//MPDUOverhead = 0;
				pdesc->NoEnc = 1;
			}

			#if 0
	switch(EncAlg){
		case NO_Encryption:
			tx_desc->SecType = 0x0;
			break;
		case WEP40_Encryption:
		case WEP104_Encryption:
			tx_desc->SecType = 0x1;
			break;
		case TKIP_Encryption:
			tx_desc->SecType = 0x2;
			break;
		case AESCCMP_Encryption:
			tx_desc->SecType = 0x3;
			break;
		default:
			tx_desc->SecType = 0x0;
			break;
	}
#else
			pdesc->SecType = 0x0;
#endif
			if (tcb_desc->bHwSec)
			{
				static u8 tmp =0;
				if (!tmp)
				{
					printk("==>================hw sec\n");
					tmp = 1;
				}
				switch (priv->ieee80211->pairwise_key_type)
				{
					case KEY_TYPE_WEP40:
					case KEY_TYPE_WEP104:
						 pdesc->SecType = 0x1;
						 pdesc->NoEnc = 0;
						 break;
					case KEY_TYPE_TKIP:
						 pdesc->SecType = 0x2;
						 pdesc->NoEnc = 0;
						 break;
					case KEY_TYPE_CCMP:
						 pdesc->SecType = 0x3;
						 pdesc->NoEnc = 0;
						 break;
					case KEY_TYPE_NA:
						 pdesc->SecType = 0x0;
						 pdesc->NoEnc = 1;
						 break;
				}
			}

			//
			// Set Packet ID
			//
			pdesc->PktId = 0x0;

			pdesc->QueueSelect = MapHwQueueToFirmwareQueue(queueindex);
			pdesc->TxFWInfoSize = sizeof(TX_FWINFO_8190PCI);
		
			pdesc->DISFB = tcb_desc->bTxDisableRateFallBack;
			pdesc->USERATE = tcb_desc->bTxUseDriverAssingedRate;
			pdesc->FirstSeg =1;
		}
		else
			pdesc->FirstSeg =0;

		for(i=0;i<buflen&& remain >0;i++,remain--){
			((u8*)buf)[i]=skb->data[i]; //copy data into descriptor pointed DMAble buffer
			if(remain == 4 && i+4 >= buflen) break; 
			/* ensure the last desc has at least 4 bytes payload */ 
			
		}

				
		if(!remain)
		{
			dev_kfree_skb_any(skb);
			pdesc->LastSeg = 1;
		}
		else
			pdesc->LastSeg = 0;
		pdesc->TxBufferSize = (u16)(skb->len - remain);
		pdesc->TxBuffAddr = buflist->dma;
		pdesc->OWN = 1;
#if 0
	{
		int i;
		printk("=============>TX desc :%d\n",sizeof(tx_desc_819x_pci));
		for(i=0;i<sizeof(tx_desc_819x_pci);i++)
			printk("%2x ",((u8*)(pdesc))[i]);
		printk("\n");
	}
#endif

		if(((tail->desc) - (begin->desc))/8 == count-1)
			tail=begin; 
		
		else
			tail=tail->next;
		
		buflist=buflist->next;
		
		mb();
		
		switch(queueindex) {
			case MGNT_QUEUE:
				priv->txmapringtail=tail;
				priv->txmapbufstail=buflist;
				break;
			
			case BK_QUEUE:
				priv->txbkpringtail=tail;
				priv->txbkpbufstail=buflist;
				break;
				
			case BE_QUEUE:
				priv->txbepringtail=tail;
				priv->txbepbufstail=buflist;
				break;
			
			case VI_QUEUE:
				priv->txvipringtail=tail;
				priv->txvipbufstail=buflist;
				break;
				
			case VO_QUEUE:
				priv->txvopringtail=tail;
				priv->txvopbufstail=buflist;
				break;
			
			case BEACON_QUEUE:
				/* the HW seems to be happy with the 1st
				 * descriptor filled and the 2nd empty...
				 * So always update descriptor 1 and never
				 * touch 2nd
				 */
			//	priv->txbeaconringtail=tail;
			//	priv->txbeaconbufstail=buflist;
				
				break;
				
		}		
		
	}
	write_nic_word(dev,TPPoll,0x01<<queueindex);
	return 0;	
}

/*	
  FIXME: check if we can use some standard already-existent 
  data type+functions in kernel
*/

short buffer_add(struct buffer **buffer, u32 *buf, dma_addr_t dma,
		struct buffer **bufferhead)
{
#ifdef DEBUG_RING
	DMESG("adding buffer to TX/RX struct");
#endif
	
        struct buffer *tmp;
	
	if(! *buffer){ 

		*buffer = kmalloc(sizeof(struct buffer),GFP_KERNEL);

		if (*buffer == NULL) {
			DMESGE("Failed to kmalloc head of TX/RX struct");
			return -1;
		}
		(*buffer)->next=*buffer;
		(*buffer)->buf=buf;
		(*buffer)->dma=dma;
		if(bufferhead !=NULL)
			(*bufferhead) = (*buffer);
		return 0;
	}
	tmp=*buffer;
	
	while(tmp->next!=(*buffer)) tmp=tmp->next;
	if ((tmp->next= kmalloc(sizeof(struct buffer),GFP_KERNEL)) == NULL){
		DMESGE("Failed to kmalloc TX/RX struct");
		return -1;
	}
	tmp->next->buf=buf;
	tmp->next->dma=dma;
	tmp->next->next=*buffer;
	
	return 0;
}

short alloc_rx_desc_ring(struct net_device *dev, u16 bufsize, int count)
{
	int i;
	u32 *desc;
	u32 *tmp;
	dma_addr_t dma_desc,dma_tmp;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev=priv->pdev;
	void *buf;
	u8 rx_desc_size;


	rx_desc_size = 4; // 4*8 = 32 bytes


	if((bufsize & 0xffff) != bufsize){ 
		DMESGE ("RX buffer allocation too large");
		return -1;
	}

	desc = (u32*)pci_alloc_consistent(pdev,sizeof(u32)*rx_desc_size*count+256,
					  &dma_desc);

	if(dma_desc & 0xff){ 
		
		/* 
		 * descriptor's buffer must be 256 byte aligned
		 * should never happen since we specify the DMA mask
		 */
		
		DMESGW("Fixing RX alignment"); 
		desc = (u32*)((u8*)desc + 256);
#if (defined(CONFIG_HIGHMEM64G) || defined(CONFIG_64BIT_PHYS_ADDR))
		desc = (u32*)((u64)desc &((u64)(~ 0xff)));
		dma_desc = (dma_addr_t)((u8*)dma_desc + 256);
		dma_desc = (dma_addr_t)((u64)dma_desc &~ 0xff);
#else 
		desc = (u32*)((u32)desc &~ 0xff);
		dma_desc = (dma_addr_t)((u8*)dma_desc + 256);
		dma_desc = (dma_addr_t)((u32)dma_desc &~ 0xff);
#endif
	}
	
	priv->rxring=desc;
	priv->rxringdma=dma_desc;
	tmp=desc;
	
	for (i=0;i<count;i++){
		
		if ((buf= kmalloc(bufsize * sizeof(u8),GFP_ATOMIC)) == NULL){
			DMESGE("Failed to kmalloc RX buffer");
			return -1;
		}
		
		dma_tmp = pci_map_single(pdev,buf,bufsize * sizeof(u8), 
					 PCI_DMA_FROMDEVICE);
		
#ifdef DEBUG_ZERO_RX
		int j;
		for(j=0;j<bufsize;j++) ((u8*)buf)[i] = 0;
#endif
		
		//buf = (void*)pci_alloc_consistent(pdev,bufsize,&dma_tmp);
		if(-1 == buffer_add(&(priv->rxbuffer), buf,dma_tmp,
			   &(priv->rxbufferhead))){
			   DMESGE("Unable to allocate mem RX buf");
			   return -1;
		}
		*tmp = 0; //zero pads the header of the descriptor
		*tmp = *tmp |( bufsize&0x3fff);
		*(tmp+3) = (u32)dma_tmp;
		*tmp = *tmp |(1<<31); // descriptor void, owned by the NIC
		
#ifdef DEBUG_RXALLOC
		DMESG("Alloc %x size buffer, DMA mem @ %x, virtual mem @ %x",
		      (u32)(bufsize&0xfff), (u32)dma_tmp, (u32)buf);
#endif
		
		tmp=tmp+rx_desc_size;
	}
	
	*(tmp-rx_desc_size) = *(tmp-rx_desc_size) | (1<<30); // this is the last descriptor
	
	
#ifdef DEBUG_RXALLOC
	DMESG("RX DMA physical address: %x",dma_desc);
#endif
	
	return 0;
}

short alloc_tx_desc_ring(struct net_device *dev, int bufsize, int count,
			 int addr)
{
	int i,j;
	u32 *desc;
	u32 *tmp;
	ptx_ring tmp_ring = NULL, tmp_txring = NULL, current_ring = NULL, head_ring = NULL;
	dma_addr_t dma_desc, dma_tmp;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev = priv->pdev;
	void *buf;
	
//	if((bufsize & 0xfff) != bufsize) { 
//		DMESGE ("TX buffer allocation too large");
//		return 0;
//	}
	desc = (u32*)pci_alloc_consistent(pdev,
					  sizeof(u32)*8*count+256, &dma_desc);
	if(desc==NULL) return -1; 
	if(dma_desc & 0xff){ 
		
		/* 
		 * descriptor's buffer must be 256 byte aligned
		 * we shouldn't be here, since we set DMA mask ! 
		 */
		DMESGW("Fixing TX alignment"); 
		desc = (u32*)((u8*)desc + 256);
#if (defined(CONFIG_HIGHMEM64G) || defined(CONFIG_64BIT_PHYS_ADDR))
		desc = (u32*)((u64)desc &~ 0xff);
		dma_desc = (dma_addr_t)((u8*)dma_desc + 256);
		dma_desc = (dma_addr_t)((u64)dma_desc &~ 0xff);
#else 
		desc = (u32*)((u32)desc &~ 0xff);
		dma_desc = (dma_addr_t)((u8*)dma_desc + 256);
		dma_desc = (dma_addr_t)((u32)dma_desc &~ 0xff);
#endif
	}
	tmp=desc;
	
	for(j=0;j<count;j++)
	{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,13))
		tmp_ring = kzalloc(sizeof(tx_ring),GFP_KERNEL);
#else
		tmp_ring = kmalloc(sizeof(tx_ring),GFP_KERNEL);
		memset((u8*)tmp_ring,0,sizeof(tx_ring));
#endif
		if(tmp_ring == NULL){
			RT_TRACE(COMP_ERR,"%s(): ERR! can't alloc tx_ring\n",__FUNCTION__);
			return -1;
		} 
		tmp_ring->next = NULL;
		if(j == 0)
		{
			current_ring = tmp_ring;
			head_ring = tmp_ring;
		}
		else
		{
			current_ring->next = tmp_ring;
			current_ring = current_ring->next;
		}
		
	}
	printk("alloc tx_ring list\n");
	switch(addr) {
	case MQDA:
		priv->txmapring = head_ring;
		tmp_txring = priv->txmapring;
		break;
		
	case BKQDA:
		priv->txbkpring = head_ring;
		tmp_txring = priv->txbkpring;
		break;
		
	case BEQDA:
		priv->txbepring = head_ring;
		tmp_txring = priv->txbepring;
		break;
		
	case VIQDA:
		priv->txvipring = head_ring;
		tmp_txring = priv->txvipring;
		break;
		
	case VOQDA:
		priv->txvopring = head_ring;
		tmp_txring = priv->txvopring;
		break;
		
	case CQDA:
		priv->txcmdring = head_ring;
		tmp_txring = priv->txcmdring;
		break;
		
	case BQDA:
		priv->txbeaconring = head_ring;
		tmp_txring = priv->txbeaconring;
		break;
		
	}
#if 1		
	for (i=0;i<count;i++)
	{
		buf = (void*)pci_alloc_consistent(pdev,bufsize,&dma_tmp);
		if (buf == NULL){ 
			printk("do not alloc buf:%d\n",bufsize);
			return -ENOMEM;
		}

		switch(addr) {
#if 0			
		case TX_NORMPRIORITY_RING_ADDR:
			if(-1 == buffer_add(&(priv->txnpbufs),buf,dma_tmp,NULL)){
				DMESGE("Unable to allocate mem for buffer NP");
				return -ENOMEM;
			}
			break;
			
		case TX_LOWPRIORITY_RING_ADDR:
			if(-1 == buffer_add(&(priv->txlpbufs),buf,dma_tmp,NULL)){
				DMESGE("Unable to allocate mem for buffer LP");
				return -ENOMEM;
			}
			break;
			
		case TX_HIGHPRIORITY_RING_ADDR:
			if(-1 == buffer_add(&(priv->txhpbufs),buf,dma_tmp,NULL)){
				DMESGE("Unable to allocate mem for buffer HP");
				return -ENOMEM;
			}
			break;
#else 
		case MQDA:
			if(-1 == buffer_add(&(priv->txmapbufs),buf,dma_tmp,NULL)){
				DMESGE("Unable to allocate mem for buffer NP");
				return -ENOMEM;
			}
			break;
			
		case BKQDA:
			if(-1 == buffer_add(&(priv->txbkpbufs),buf,dma_tmp,NULL)){
				DMESGE("Unable to allocate mem for buffer LP");
				return -ENOMEM;
			}
			break;
		case BEQDA:
			if(-1 == buffer_add(&(priv->txbepbufs),buf,dma_tmp,NULL)){
				DMESGE("Unable to allocate mem for buffer NP");
				return -ENOMEM;
			}
			break;
			
		case VIQDA:
			if(-1 == buffer_add(&(priv->txvipbufs),buf,dma_tmp,NULL)){
				DMESGE("Unable to allocate mem for buffer LP");
				return -ENOMEM;
			}
			break;
		case VOQDA:
			if(-1 == buffer_add(&(priv->txvopbufs),buf,dma_tmp,NULL)){
				DMESGE("Unable to allocate mem for buffer NP");
				return -ENOMEM;
			}
			break;
#endif		
		case CQDA:
			if(-1 == buffer_add(&(priv->txcmdbufs),buf,dma_tmp,NULL)){
				DMESGE("Unable to allocate mem for buffer HP");
				return -ENOMEM;
			}
			break;
		case BQDA:
		        if(-1 == buffer_add(&(priv->txbeaconbufs),buf,dma_tmp,NULL)){
			DMESGE("Unable to allocate mem for buffer BP");
				return -ENOMEM;
			}
			break;
		}
		*tmp = *tmp &~ (1<<31); // descriptor empty, owned by the drv 
		//*(tmp+2) = (u32)dma_tmp;
		//*(tmp+3) = bufsize;
		*(tmp+3) = (u32)dma_tmp;

		if(i+1<count)
			*(tmp+4) = (u32)dma_desc+((i+1)*8*4);
		else
			*(tmp+4) = (u32)dma_desc;
		tmp_txring->desc = tmp;
		tmp_txring->nStuckCount = 0;
		tmp=tmp+8;
		tmp_txring=tmp_txring->next;
	}
	switch(addr) {
	case MQDA:
		priv->txmapringdma=dma_desc;
		priv->txmapring->desc=desc;
		break;
		
	case BKQDA:
		priv->txbkpringdma=dma_desc;
		priv->txbkpring->desc=desc;
		break;
		
	case BEQDA:
		priv->txbepringdma=dma_desc;
		priv->txbepring->desc=desc;
		break;
		
	case VIQDA:
		priv->txvipringdma=dma_desc;
		priv->txvipring->desc=desc;
		break;
		
	case VOQDA:
		priv->txvopringdma=dma_desc;
		priv->txvopring->desc=desc;
		break;
		
	case CQDA:
		priv->txcmdringdma=dma_desc;
		priv->txcmdring->desc=desc;
		break;
		
	case BQDA:
		priv->txbeaconringdma=dma_desc;
		priv->txbeaconring->desc=desc;
		break;
		
	}
	
#endif	
#ifdef DEBUG_TX
	DMESG("Tx dma physical address: %x",dma_desc);
#endif
	
	return 0;
}
short rtl8192_pci_initdescring(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	
	if (0!=alloc_rx_desc_ring(dev, priv->rxbuffersize, priv->rxringcount)) 
		return -ENOMEM;
	
	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txringcount,
				  MQDA))
		return -ENOMEM;
	
	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txringcount,
				 BKQDA))
		return -ENOMEM;
	
	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txringcount,
				 BEQDA))
		return -ENOMEM;
	
	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txringcount,
				  VIQDA))
		return -ENOMEM;
	
	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txringcount,
				  VOQDA))
		return -ENOMEM;

	if (0!=alloc_tx_desc_ring(dev, priv->txfwbuffersize, priv->txringcount,
				  CQDA))
		return -ENOMEM;
	
	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txbeaconcount,
				  BQDA))
		return -ENOMEM;
	return 0;	
}
#if 1
extern void rtl8192_update_ratr_table(struct net_device* dev);
void rtl8192_link_change(struct net_device *dev)
{
//	int i;
	
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	//write_nic_word(dev, BCN_INTR_ITV, net->beacon_interval);
	if (ieee->state == IEEE80211_LINKED)
	{
		rtl8192_net_update(dev);
		rtl8192_update_ratr_table(dev);
#if 1
		//add this as in pure N mode, wep encryption will use software way, but there is no chance to set this as wep will not set group key in wext. WB.2008.07.08
		if ((KEY_TYPE_WEP40 == ieee->pairwise_key_type) || (KEY_TYPE_WEP104 == ieee->pairwise_key_type))
		EnableHWSecurityConfig8192(dev);
#endif
	}
	/*update timing params*/
	//rtl8192_set_chan(dev, priv->chan);
	
	//rtl8192_set_rxconf(dev);
	// 2007/10/16 MH MAC Will update TSF according to all received beacon, so we have
	//	// To set CBSSID bit when link with any AP or STA.	
	if (ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_ADHOC)
	{
		u32 reg = 0;
		reg = read_nic_dword(dev, RCR);
		if (priv->ieee80211->state == IEEE80211_LINKED)
			priv->ReceiveConfig = reg |= RCR_CBSSID;
		else
			priv->ReceiveConfig = reg &= ~RCR_CBSSID;
		write_nic_dword(dev, RCR, reg);
	}
}
#endif


static struct ieee80211_qos_parameters def_qos_parameters = {
        {3,3,3,3},/* cw_min */
        {7,7,7,7},/* cw_max */
        {2,2,2,2},/* aifs */ 
        {0,0,0,0},/* flags */ 
        {0,0,0,0} /* tx_op_limit */
};

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192_update_beacon(struct work_struct * work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, update_beacon_wq.work);
        struct net_device *dev = priv->ieee80211->dev;
#else
void rtl8192_update_beacon(struct net_device *dev)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
#endif
 	struct ieee80211_device* ieee = priv->ieee80211;
	struct ieee80211_network* net = &ieee->current_network;

	if (ieee->pHTInfo->bCurrentHTSupport)
		HTUpdateSelfAndPeerSetting(ieee, net);
	ieee->pHTInfo->bCurrentRT2RTLongSlotTime = net->bssht.bdRT2RTLongSlotTime;
	rtl8192_update_cap(dev, net->capability);
}
/*
* background support to run QoS activate functionality
*/
int WDCAPARA_ADD[] = {EDCAPARA_BE,EDCAPARA_BK,EDCAPARA_VI,EDCAPARA_VO};
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192_qos_activate(struct work_struct * work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, qos_activate);
        struct net_device *dev = priv->ieee80211->dev;
#else
void rtl8192_qos_activate(struct net_device *dev)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
#endif
        struct ieee80211_qos_parameters *qos_parameters = &priv->ieee80211->current_network.qos_data.parameters;
        u8 mode = priv->ieee80211->current_network.mode;
//        u32 size = sizeof(struct ieee80211_qos_parameters);
	u8  u1bAIFS;
	u32 u4bAcParam;
        int i;
        if (priv == NULL)
                return;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	down(&priv->mutex);
#else
        mutex_lock(&priv->mutex);
#endif
        if(priv->ieee80211->state != IEEE80211_LINKED)
		goto success;
	RT_TRACE(COMP_QOS,"qos active process with associate response received\n");
	/* It better set slot time at first */
	/* For we just support b/g mode at present, let the slot time at 9/20 selection */	
	/* update the ac parameter to related registers */
	for(i = 0; i <  QOS_QUEUE_NUM; i++) {
		//Mode G/A: slotTimeTimer = 9; Mode B: 20
		u1bAIFS = qos_parameters->aifs[i] * ((mode&(IEEE_G|IEEE_N_24G)) ?9:20) + aSifsTime; 
		u4bAcParam = ((((u32)(qos_parameters->tx_op_limit[i]))<< AC_PARAM_TXOP_LIMIT_OFFSET)|
				(((u32)(qos_parameters->cw_max[i]))<< AC_PARAM_ECW_MAX_OFFSET)|
				(((u32)(qos_parameters->cw_min[i]))<< AC_PARAM_ECW_MIN_OFFSET)|
				((u32)u1bAIFS << AC_PARAM_AIFS_OFFSET));
		printk("===>u4bAcParam:%x, ", u4bAcParam);
		//write_nic_dword(dev, WDCAPARA_ADD[i], u4bAcParam);
		write_nic_dword(dev, WDCAPARA_ADD[i], 0x005e4332);
	}

success:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	up(&priv->mutex);
#else
        mutex_unlock(&priv->mutex);
#endif
}

static int rtl8192_qos_handle_probe_response(struct r8192_priv *priv,
		int active_network,
		struct ieee80211_network *network)
{
	int ret = 0;
	u32 size = sizeof(struct ieee80211_qos_parameters);

	if(priv->ieee80211->state !=IEEE80211_LINKED)
                return ret;

        if ((priv->ieee80211->iw_mode != IW_MODE_INFRA))
                return ret;

	if (network->flags & NETWORK_HAS_QOS_MASK) {
		if (active_network &&
				(network->flags & NETWORK_HAS_QOS_PARAMETERS))
			network->qos_data.active = network->qos_data.supported;

		if ((network->qos_data.active == 1) && (active_network == 1) &&
				(network->flags & NETWORK_HAS_QOS_PARAMETERS) &&
				(network->qos_data.old_param_count !=
				 network->qos_data.param_count)) {
			network->qos_data.old_param_count =
				network->qos_data.param_count;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
			queue_work(priv->priv_wq, &priv->qos_activate);
#else
			schedule_task(&priv->qos_activate);
#endif
			RT_TRACE (COMP_QOS, "QoS parameters change call "
					"qos_activate\n");
		}
	} else {
		memcpy(&priv->ieee80211->current_network.qos_data.parameters,\
		       &def_qos_parameters, size);

		if ((network->qos_data.active == 1) && (active_network == 1)) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
			queue_work(priv->priv_wq, &priv->qos_activate);
#else
			schedule_task(&priv->qos_activate);
#endif
			RT_TRACE(COMP_QOS, "QoS was disabled call qos_activate \n");
		}
		network->qos_data.active = 0;
		network->qos_data.supported = 0;
	}

	return 0;
}

/* handle manage frame frame beacon and probe response */
static int rtl8192_handle_beacon(struct net_device * dev,
                              struct ieee80211_beacon * beacon,
                              struct ieee80211_network * network)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	rtl8192_qos_handle_probe_response(priv,1,network);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	queue_delayed_work(priv->priv_wq, &priv->update_beacon_wq, 0);
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	schedule_task(&priv->update_beacon_wq);
#else
	queue_work(priv->priv_wq, &priv->update_beacon_wq);
#endif
#endif
	return 0;

}

/*
* handling the beaconing responses. if we get different QoS setting
* off the network from the associated setting, adjust the QoS
* setting
*/
static int rtl8192_qos_association_resp(struct r8192_priv *priv,
                                    struct ieee80211_network *network)
{
        int ret = 0;
        unsigned long flags;
        u32 size = sizeof(struct ieee80211_qos_parameters);
        int set_qos_param = 0;

        if ((priv == NULL) || (network == NULL))
                return ret;

	if(priv->ieee80211->state !=IEEE80211_LINKED)
                return ret;

        if ((priv->ieee80211->iw_mode != IW_MODE_INFRA))
                return ret;

        spin_lock_irqsave(&priv->ieee80211->lock, flags);
	if(network->flags & NETWORK_HAS_QOS_PARAMETERS) {
		memcpy(&priv->ieee80211->current_network.qos_data.parameters,\
			 &network->qos_data.parameters,\
			sizeof(struct ieee80211_qos_parameters));
		priv->ieee80211->current_network.qos_data.active = 1;
#if 0
		if((priv->ieee80211->current_network.qos_data.param_count != \
					network->qos_data.param_count))
#endif
		 {
                        set_qos_param = 1;
			/* update qos parameter for current network */
			priv->ieee80211->current_network.qos_data.old_param_count = \
				 priv->ieee80211->current_network.qos_data.param_count;
			priv->ieee80211->current_network.qos_data.param_count = \
			     	 network->qos_data.param_count;
		}
        } else {
		memcpy(&priv->ieee80211->current_network.qos_data.parameters,\
		       &def_qos_parameters, size);
		priv->ieee80211->current_network.qos_data.active = 0;
		priv->ieee80211->current_network.qos_data.supported = 0;
                set_qos_param = 1;
        }

        spin_unlock_irqrestore(&priv->ieee80211->lock, flags);

	RT_TRACE(COMP_QOS, "%s: network->flags = %d,%d\n",__FUNCTION__,network->flags ,priv->ieee80211->current_network.qos_data.active);
	if (set_qos_param == 1)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
		queue_work(priv->priv_wq, &priv->qos_activate);
#else
		schedule_task(&priv->qos_activate);
#endif


        return ret;
}


static int rtl8192_handle_assoc_response(struct net_device *dev,
                                     struct ieee80211_assoc_response_frame *resp,
                                     struct ieee80211_network *network)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
        rtl8192_qos_association_resp(priv, network);
        return 0;
}


//updateRATRTabel for MCS only. Basic rate is not implement.
void rtl8192_update_ratr_table(struct net_device* dev)
	//	POCTET_STRING	posLegacyRate,
	//	u8*			pMcsRate)
	//	PRT_WLAN_STA	pEntry)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	u8* pMcsRate = ieee->dot11HTOperationalRateSet;
	//struct ieee80211_network *net = &ieee->current_network;
	u32 ratr_value = 0;
	u8 rate_index = 0;

	rtl8192_config_rate(dev, (u16*)(&ratr_value));
	ratr_value |= (*(u16*)(pMcsRate)) << 12;
//	switch (net->mode)
	switch (ieee->mode)
	{
		case IEEE_A:
			ratr_value &= 0x00000FF0;
			break;
		case IEEE_B:
			ratr_value &= 0x0000000F;
			break;
		case IEEE_G:
			ratr_value &= 0x00000FF7;
			break;
		case IEEE_N_24G:
		case IEEE_N_5G:
			if (ieee->pHTInfo->PeerMimoPs == 0) //MIMO_PS_STATIC
				ratr_value &= 0x0007F007;
			else{
				if (priv->rf_type == RF_1T2R)
					ratr_value &= 0x000FF007;
				else
					ratr_value &= 0x0F81F007;
			}
			break;
		default:
			break; 
	}
	ratr_value &= 0x0FFFFFFF;
	if(ieee->pHTInfo->bCurTxBW40MHz && ieee->pHTInfo->bCurShortGI40MHz){
		ratr_value |= 0x80000000;
	}else if(!ieee->pHTInfo->bCurTxBW40MHz && ieee->pHTInfo->bCurShortGI20MHz){
		ratr_value |= 0x80000000;
	}	
	write_nic_dword(dev, RATR0+rate_index*4, ratr_value);
	write_nic_byte(dev, UFWP, 1);
}

static u8 ccmp_ie[4] = {0x00,0x50,0xf2,0x04};
static u8 ccmp_rsn_ie[4] = {0x00, 0x0f, 0xac, 0x04};
bool GetNmodeSupportBySecCfg8190Pci(struct net_device*dev)
{
#if 1
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
        int wpa_ie_len= ieee->wpa_ie_len;
        struct ieee80211_crypt_data* crypt;
        int encrypt;
                             
        crypt = ieee->crypt[ieee->tx_keyidx];
        encrypt = ieee->host_encrypt && crypt && crypt->ops && (0 == strcmp(crypt->ops->name,"WEP"));

	/* simply judge  */
	if(encrypt && (wpa_ie_len == 0)) {
		/* wep encryption, no N mode setting */
		return false;
//	} else if((wpa_ie_len != 0)&&(memcmp(&(ieee->wpa_ie[14]),ccmp_ie,4))) {
	} else if((wpa_ie_len != 0)) {
		/* parse pairwise key type */
		//if((pairwisekey = WEP40)||(pairwisekey = WEP104)||(pairwisekey = TKIP)) 
		if (((ieee->wpa_ie[0] == 0xdd) && (!memcmp(&(ieee->wpa_ie[14]),ccmp_ie,4))) || ((ieee->wpa_ie[0] == 0x30) && (!memcmp(&ieee->wpa_ie[10],ccmp_rsn_ie, 4))))
			return true;
		else
			return false;
	} else {
		//RT_TRACE(COMP_ERR,"In %s The GroupEncAlgorithm is [4]\n",__FUNCTION__ );
		return true;
	}
	
#if 0
        //In here we discuss with SD4 David. He think we still can send TKIP in broadcast group key in MCS rate.
        //We can't force in G mode if Pairwie key is AES and group key is TKIP
        if((pSecInfo->GroupEncAlgorithm == WEP104_Encryption) || (pSecInfo->GroupEncAlgorithm == WEP40_Encryption)  ||
           (pSecInfo->PairwiseEncAlgorithm == WEP104_Encryption) ||
           (pSecInfo->PairwiseEncAlgorithm == WEP40_Encryption) || (pSecInfo->PairwiseEncAlgorithm == TKIP_Encryption))
        {
                return  false;
        }
        else
                return true;
#endif
	return true;
#endif
}

void rtl8192_refresh_supportrate(struct r8192_priv* priv)
{
	struct ieee80211_device* ieee = priv->ieee80211;
	//we donot consider set support rate for ABG mode, only HT MCS rate is set here.
	if (ieee->mode == WIRELESS_MODE_N_24G || ieee->mode == WIRELESS_MODE_N_5G)
	{
		memcpy(ieee->Regdot11HTOperationalRateSet, ieee->RegHTSuppRateSet, 16);
		//RT_DEBUG_DATA(COMP_INIT, ieee->RegHTSuppRateSet, 16);
		//RT_DEBUG_DATA(COMP_INIT, ieee->Regdot11HTOperationalRateSet, 16);
	}
	else
		memset(ieee->Regdot11HTOperationalRateSet, 0, 16);
	return;
}

u8 rtl8192_getSupportedWireleeMode(struct net_device*dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 ret = 0;
	switch(priv->rf_chip)
	{
		case RF_8225:
		case RF_8256:
		case RF_PSEUDO_11N:
			ret = (WIRELESS_MODE_N_24G|WIRELESS_MODE_G|WIRELESS_MODE_B);
			break;
		case RF_8258:
			ret = (WIRELESS_MODE_A|WIRELESS_MODE_N_5G);
			break;
		default:
			ret = WIRELESS_MODE_B;
			break;
	}
	return ret;
}
void rtl8192_SetWirelessMode(struct net_device* dev, u8 wireless_mode)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 bSupportMode = rtl8192_getSupportedWireleeMode(dev);

#if 1  
	if ((wireless_mode == WIRELESS_MODE_AUTO) || ((wireless_mode&bSupportMode)==0))
	{
		if(bSupportMode & WIRELESS_MODE_N_24G)
		{
			wireless_mode = WIRELESS_MODE_N_24G;
		}
		else if(bSupportMode & WIRELESS_MODE_N_5G)
		{
			wireless_mode = WIRELESS_MODE_N_5G;
		}
		else if((bSupportMode & WIRELESS_MODE_A))
		{
			wireless_mode = WIRELESS_MODE_A;
		}
		else if((bSupportMode & WIRELESS_MODE_G))
		{
			wireless_mode = WIRELESS_MODE_G;
		}
		else if((bSupportMode & WIRELESS_MODE_B))
		{
			wireless_mode = WIRELESS_MODE_B;
		}
		else{
			RT_TRACE(COMP_ERR, "%s(), No valid wireless mode supported, SupportedWirelessMode(%x)!!!\n", __FUNCTION__,bSupportMode);
			wireless_mode = WIRELESS_MODE_B;
		}
	}
#ifdef TO_DO_LIST //// TODO: this function doesn't work well at this time, we shoud wait for FPGA
	ActUpdateChannelAccessSetting( pAdapter, pHalData->CurrentWirelessMode, &pAdapter->MgntInfo.Info8185.ChannelAccessSetting );
#endif
	priv->ieee80211->mode = wireless_mode;
	
	if ((wireless_mode == WIRELESS_MODE_N_24G) ||  (wireless_mode == WIRELESS_MODE_N_5G))
		priv->ieee80211->pHTInfo->bEnableHT = 1;	
	else
		priv->ieee80211->pHTInfo->bEnableHT = 0;
	RT_TRACE(COMP_INIT, "Current Wireless Mode is %x\n", wireless_mode);
	rtl8192_refresh_supportrate(priv);
#endif	

}
//init priv variables here

bool GetHalfNmodeSupportByAPs819xPci(struct net_device* dev)
{
	bool			Reval;
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	
	if(ieee->bHalfWirelessN24GMode == true)
		Reval = true;
	else
		Reval =  false;

	return Reval;
}

static void rtl8192_init_priv_variable(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 i;
	priv->txbuffsize = 1600;//1024;
	priv->txfwbuffersize = 4096;
	priv->txringcount = 64;//32;
	priv->txbeaconcount = priv->txringcount;
	priv->rxbuffersize = 9100;//2048;//1024;
	priv->rxringcount = 32;//32; 
	priv->irq_enabled=0;
	priv->card_8192 = NIC_8192E;
	priv->rx_skb_complete = 1;
	priv->chan = 1; //set to channel 1
	priv->RegWirelessMode = WIRELESS_MODE_AUTO;
	priv->RegChannelPlan = 0xf;
	priv->nrxAMPDU_size = 0;
	priv->nrxAMPDU_aggr_num = 0;
	priv->last_rxdesc_tsf_high = 0;
	priv->last_rxdesc_tsf_low = 0;
	priv->ieee80211->mode = WIRELESS_MODE_AUTO; //SET AUTO 
	priv->ieee80211->iw_mode = IW_MODE_INFRA;
	priv->ieee80211->ieee_up=0;
	priv->retry_rts = DEFAULT_RETRY_RTS;
	priv->retry_data = DEFAULT_RETRY_DATA;
	priv->ieee80211->rts = DEFAULT_RTS_THRESHOLD;
	priv->ieee80211->rate = 110; //11 mbps
	priv->ieee80211->short_slot = 1;
	priv->promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	priv->bcck_in_ch14 = false;
	priv->btxpowerdata_readfromEEPORM = false;
	priv->bfsync_processing  = false;
	priv->CCKPresentAttentuation = 0;
	priv->rfa_txpowertrackingindex = 0;
	priv->rfc_txpowertrackingindex = 0;
	priv->CckPwEnl = 6;
	priv->ScanDelay = 50;//for Scan TODO
	//added by amy for silent reset
	priv->ResetProgress = RESET_TYPE_NORESET;
	priv->bForcedSilentReset = 0;
	priv->bDisableNormalResetCheck = false;
	priv->force_reset = false;
	//just for debug
	priv->txpower_checkcnt = 0;
	priv->thermal_readback_index =0;
	priv->txpower_tracking_callback_cnt = 0;
	priv->ccktxpower_adjustcnt_ch14 = 0;
	priv->ccktxpower_adjustcnt_not_ch14 = 0;
	
	priv->ieee80211->current_network.beacon_interval = DEFAULT_BEACONINTERVAL;	
	priv->ieee80211->iw_mode = IW_MODE_INFRA;
	priv->ieee80211->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE |
		IEEE_SOFTMAC_BEACONS;//added by amy 080604 //|  //IEEE_SOFTMAC_SINGLE_QUEUE;
	
	priv->ieee80211->active_scan = 1;
	priv->ieee80211->modulation = IEEE80211_CCK_MODULATION | IEEE80211_OFDM_MODULATION;
	priv->ieee80211->host_encrypt = 1;
	priv->ieee80211->host_decrypt = 1;
	priv->ieee80211->start_send_beacons = NULL;//rtl819xusb_beacon_tx;//-by amy 080604
	priv->ieee80211->stop_send_beacons = NULL;//rtl8192_beacon_stop;//-by amy 080604
	priv->ieee80211->softmac_hard_start_xmit = rtl8192_hard_start_xmit;
	priv->ieee80211->set_chan = rtl8192_set_chan;
	priv->ieee80211->link_change = rtl8192_link_change;
	priv->ieee80211->softmac_data_hard_start_xmit = rtl8192_hard_data_xmit;
	priv->ieee80211->data_hard_stop = rtl8192_data_hard_stop;
	priv->ieee80211->data_hard_resume = rtl8192_data_hard_resume;
	priv->ieee80211->init_wmmparam_flag = 0;
	priv->ieee80211->fts = DEFAULT_FRAG_THRESHOLD;
	priv->ieee80211->check_nic_enough_desc = check_nic_enough_desc;	
	priv->ieee80211->tx_headroom = sizeof(TX_FWINFO_8190PCI);
	priv->ieee80211->qos_support = 1;
	priv->ieee80211->dot11PowerSaveMode = 0;
	//added by WB
//	priv->ieee80211->SwChnlByTimerHandler = rtl8192_phy_SwChnl;
	priv->ieee80211->SetBWModeHandler = rtl8192_SetBWMode;
	priv->ieee80211->handle_assoc_response = rtl8192_handle_assoc_response;
	priv->ieee80211->handle_beacon = rtl8192_handle_beacon;
	//added by david
	priv->ieee80211->GetNmodeSupportBySecCfg = GetNmodeSupportBySecCfg8190Pci;
	priv->ieee80211->SetWirelessMode = rtl8192_SetWirelessMode;
	priv->ieee80211->GetHalfNmodeSupportByAPsHandler = GetHalfNmodeSupportByAPs819xPci;

	//added by amy
	priv->ieee80211->InitialGainHandler = InitialGain819xPci;
	
	priv->card_type = USB;
#ifdef TO_DO_LIST
	if(Adapter->bInHctTest)
  	{
		pHalData->ShortRetryLimit = 7;
		pHalData->LongRetryLimit = 7;
  	}
#endif
	{
		priv->ShortRetryLimit = 0x30;
		priv->LongRetryLimit = 0x30;
	}
	priv->EarlyRxThreshold = 7;
	priv->enable_gpio0 = 0;

	priv->TransmitConfig = 0;
	
	priv->ReceiveConfig = RCR_ADD3	|
		RCR_AMF | RCR_ADF |		//accept management/data
		RCR_AICV |			//accept control frame for SW AP needs PS-poll, 2005.07.07, by rcnjko.
		RCR_AB | RCR_AM | RCR_APM |	//accept BC/MC/UC
		RCR_AAP | ((u32)7<<RCR_MXDMA_OFFSET) |
		((u32)7 << RCR_FIFO_OFFSET) | RCR_ONLYERLPKT;

	priv->irq_mask = 	(u32)(IMR_ROK | IMR_VODOK | IMR_VIDOK | IMR_BEDOK | IMR_BKDOK |		\
								IMR_HCCADOK | IMR_MGNTDOK | IMR_COMDOK | IMR_HIGHDOK | 					\
								IMR_BDOK | IMR_RXCMDOK | IMR_TIMEOUT0 | IMR_RDU | IMR_RXFOVW	|			\
								IMR_TXFOVW | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);

	priv->AcmControl = 0;	
	priv->pFirmware = (rt_firmware*)vmalloc(sizeof(rt_firmware));
	if (priv->pFirmware)
	memset(priv->pFirmware, 0, sizeof(rt_firmware));

	/* rx related queue */
        skb_queue_head_init(&priv->rx_queue);
	skb_queue_head_init(&priv->skb_queue);

	/* Tx related queue */
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->ieee80211->skb_waitQ [i]);
	}
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->ieee80211->skb_aggQ [i]);
	}
	priv->rf_set_chan = rtl8192_phy_SwChnl;	
}	

//init lock here
static void rtl8192_init_priv_lock(struct r8192_priv* priv)
{
	spin_lock_init(&priv->tx_lock);
	spin_lock_init(&priv->irq_lock);//added by thomas
	spin_lock_init(&priv->irq_th_lock);
	spin_lock_init(&priv->rf_ps_lock);
	//spin_lock_init(&priv->rf_lock);
	sema_init(&priv->wx_sem,1);
	sema_init(&priv->rf_sem,1);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	sema_init(&priv->mutex, 1);
#else
	mutex_init(&priv->mutex);
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
extern  void    rtl819x_watchdog_wqcallback(struct work_struct *work);
#else
extern  void    rtl819x_watchdog_wqcallback(struct net_device *dev);
#endif

void rtl8192_irq_rx_tasklet(struct r8192_priv *priv);
void rtl8192_irq_tx_tasklet(struct r8192_priv *priv);
//init tasklet and wait_queue here. only 2.6 above kernel is considered
#define DRV_NAME "wlan0"
static void rtl8192_init_priv_task(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);	

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
#ifdef PF_SYNCTHREAD
	priv->priv_wq = create_workqueue(DRV_NAME,0);
#else	
	priv->priv_wq = create_workqueue(DRV_NAME);
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
//	INIT_WORK(&priv->reset_wq, (void(*)(void*)) rtl8192_restart);
	INIT_WORK(&priv->reset_wq,  rtl8192_restart);
//	INIT_DELAYED_WORK(&priv->watch_dog_wq, hal_dm_watchdog);
	INIT_DELAYED_WORK(&priv->watch_dog_wq, rtl819x_watchdog_wqcallback);
	INIT_DELAYED_WORK(&priv->txpower_tracking_wq,  dm_txpower_trackingcallback);
	INIT_DELAYED_WORK(&priv->rfpath_check_wq,  dm_rf_pathcheck_workitemcallback);
	INIT_DELAYED_WORK(&priv->update_beacon_wq, rtl8192_update_beacon);
	//INIT_WORK(&priv->SwChnlWorkItem,  rtl8192_SwChnl_WorkItem);
	//INIT_WORK(&priv->SetBWModeWorkItem,  rtl8192_SetBWModeWorkItem);
	INIT_WORK(&priv->qos_activate, rtl8192_qos_activate);
#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	tq_init(&priv->reset_wq, (void*)rtl8192_restart, dev);
	tq_init(&priv->watch_dog_wq, (void*)rtl819x_watchdog_wqcallback, dev);
	tq_init(&priv->txpower_tracking_wq, (void*)dm_txpower_trackingcallback, dev);
	tq_init(&priv->rfpath_check_wq, (void*)dm_rf_pathcheck_workitemcallback, dev);
	tq_init(&priv->update_beacon_wq, (void*)rtl8192_update_beacon, dev);
	//tq_init(&priv->SwChnlWorkItem, (void*) rtl8192_SwChnl_WorkItem, dev);
	//tq_init(&priv->SetBWModeWorkItem, (void*)rtl8192_SetBWModeWorkItem, dev);
	tq_init(&priv->qos_activate, (void *)rtl8192_qos_activate, dev);
#else
	INIT_WORK(&priv->reset_wq,(void(*)(void*)) rtl8192_restart,dev);
//	INIT_WORK(&priv->watch_dog_wq, (void(*)(void*)) hal_dm_watchdog,dev);
	INIT_WORK(&priv->watch_dog_wq, (void(*)(void*)) rtl819x_watchdog_wqcallback,dev);
	INIT_WORK(&priv->txpower_tracking_wq, (void(*)(void*)) dm_txpower_trackingcallback,dev);
	INIT_WORK(&priv->rfpath_check_wq, (void(*)(void*)) dm_rf_pathcheck_workitemcallback,dev);
	INIT_WORK(&priv->update_beacon_wq, (void(*)(void*))rtl8192_update_beacon,dev);
	//INIT_WORK(&priv->SwChnlWorkItem, (void(*)(void*)) rtl8192_SwChnl_WorkItem, dev);
	//INIT_WORK(&priv->SetBWModeWorkItem, (void(*)(void*)) rtl8192_SetBWModeWorkItem, dev);
	INIT_WORK(&priv->qos_activate, (void(*)(void *))rtl8192_qos_activate, dev);
#endif
#endif

	tasklet_init(&priv->irq_rx_tasklet,
	     (void(*)(unsigned long))rtl8192_irq_rx_tasklet,
	     (unsigned long)priv);
	tasklet_init(&priv->irq_tx_tasklet,
	     (void(*)(unsigned long))rtl8192_irq_tx_tasklet,
	     (unsigned long)priv);
}

static void rtl8192_get_eeprom_size(struct net_device* dev)
{
	u16 curCR = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	RT_TRACE(COMP_INIT, "===========>%s()\n", __FUNCTION__);	
	curCR = read_nic_dword(dev, EPROM_CMD);
	RT_TRACE(COMP_INIT, "read from Reg Cmd9346CR(%x):%x\n", EPROM_CMD, curCR);
	//whether need I consider BIT5?	
	priv->epromtype = (curCR & EPROM_CMD_9356SEL) ? EPROM_93c56 : EPROM_93c46;
	RT_TRACE(COMP_INIT, "<===========%s(), epromtype:%d\n", __FUNCTION__, priv->epromtype);	
}

//used to swap endian. as ntohl & htonl are not neccessary to swap endian, so use this instead.
static inline u16 endian_swap(u16* data)
{
	u16 tmp = *data;
	*data = (tmp >> 8) | (tmp << 8);
	return *data;	
}

/*
 *	Note:	Adapter->EEPROMAddressSize should be set before this function call.
 * 			EEPROM address size can be got through GetEEPROMSize8185()
*/
static void rtl8192_read_eeprom_info(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	u8			tempval;
#ifdef RTL8192E
	u8			ICVer8192, ICVer8256;
#endif
	u16			i,usValue, IC_Version;
	u16			EEPROMId;
#ifdef RTL8190P
   	u8			offset;//, tmpAFR;
    	u8      		EepromTxPower[100];
#endif
	u8 bMac_Tmp_Addr[6] = {0x00, 0xe0, 0x4c, 0x00, 0x00, 0x01};
	RT_TRACE(COMP_INIT, "====> rtl8192_read_eeprom_info\n");

	  
	// TODO: I don't know if we need to apply EF function to EEPROM read function
  
	//2 Read EEPROM ID to make sure autoload is success
	EEPROMId = eprom_read(dev, 0);
	if( EEPROMId != RTL8190_EEPROM_ID )
	{
		RT_TRACE(COMP_ERR, "EEPROM ID is invalid\n"); 
		priv->AutoloadFailFlag=true;
	}
	else
	{
		priv->AutoloadFailFlag=false;
	}
	
	//
	// Assign Chip Version ID
	//
	// Read IC Version && Channel Plan
	if(!priv->AutoloadFailFlag)
	{
		// VID, PID	 
		priv->eeprom_vid = eprom_read(dev, (EEPROM_VID >> 1));
		priv->eeprom_did = eprom_read(dev, (EEPROM_DID >> 1));

		usValue = eprom_read(dev, (u16)(EEPROM_Customer_ID>>1)) >> 8 ;
		priv->eeprom_CustomerID = (u8)( usValue & 0xff);	
		usValue = eprom_read(dev, (EEPROM_ICVersion_ChannelPlan>>1));
		priv->eeprom_ChannelPlan = usValue&0xff;
		IC_Version = ((usValue&0xff00)>>8);

#ifdef RTL8190P
		priv->card_8192_version = (VERSION_8190)(IC_Version);
#else
	#ifdef RTL8192E
		ICVer8192 = (IC_Version&0xf);		//bit0~3; 1:A cut, 2:B cut, 3:C cut...
		ICVer8256 = ((IC_Version&0xf0)>>4);//bit4~6, bit7 reserved for other RF chip; 1:A cut, 2:B cut, 3:C cut...
		RT_TRACE(COMP_INIT, "\nICVer8192 = 0x%x\n", ICVer8192);
		RT_TRACE(COMP_INIT, "\nICVer8256 = 0x%x\n", ICVer8256);
		if(ICVer8192 == 0x2)	//B-cut
		{
			if(ICVer8256 == 0x5) //E-cut
				priv->card_8192_version= VERSION_8190_BE;
		}
	#endif
#endif
		switch(priv->card_8192_version)
		{
			case VERSION_8190_BD:
			case VERSION_8190_BE:
				break;
			default:
				priv->card_8192_version = VERSION_8190_BD;
				break;
		}
		RT_TRACE(COMP_INIT, "\nIC Version = 0x%x\n", priv->card_8192_version);
	}
	else
	{
		priv->card_8192_version = VERSION_8190_BD;
		priv->eeprom_vid = 0;
		priv->eeprom_did = 0;
		priv->eeprom_CustomerID = 0;
		priv->eeprom_ChannelPlan = 0;
		RT_TRACE(COMP_INIT, "\nIC Version = 0x%x\n", 0xff);
	}
	
	RT_TRACE(COMP_INIT, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_INIT, "EEPROM DID = 0x%4x\n", priv->eeprom_did);
	RT_TRACE(COMP_INIT,"EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);
	
	//2 Read Permanent MAC address
	if(!priv->AutoloadFailFlag)
	{
		for(i = 0; i < 6; i += 2)
		{
			usValue = eprom_read(dev, (u16) ((EEPROM_NODE_ADDRESS_BYTE_0+i)>>1));
			*(u16*)(&dev->dev_addr[i]) = usValue;
		}
	}
	else
	{
		// when auto load failed,  the last address byte set to be a random one.
		// added by david woo.2007/11/7
		memcpy(dev->dev_addr, bMac_Tmp_Addr, 6);		
		#if 0
		for(i = 0; i < 6; i++)
		{
			Adapter->PermanentAddress[i] = sMacAddr[i];
			PlatformEFIOWrite1Byte(Adapter, IDR0+i, sMacAddr[i]);
		}
		#endif
	}

	RT_TRACE(COMP_INIT, "Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
			dev->dev_addr[0], dev->dev_addr[1], 
			dev->dev_addr[2], dev->dev_addr[3], 
			dev->dev_addr[4], dev->dev_addr[5]);  
	
		//2 TX Power Check EEPROM Fail or not
	if(priv->card_8192_version > VERSION_8190_BD)
	{
		priv->bTXPowerDataReadFromEEPORM = true;
	}
	else
	{
		priv->bTXPowerDataReadFromEEPORM = false;
	}

	// 2007/11/15 MH 8190PCI Default=2T4R, 8192PCIE dafault=1T2R
	priv->rf_type = RTL819X_DEFAULT_RF_TYPE;

	if(priv->card_8192_version > VERSION_8190_BD)
	{
		// Read RF-indication and Tx Power gain index diff of legacy to HT OFDM rate.
		if(!priv->AutoloadFailFlag)
		{
			tempval = (eprom_read(dev, (EEPROM_RFInd_PowerDiff>>1))) & 0xff;
			priv->EEPROMLegacyHTTxPowerDiff = tempval & 0xf;	// bit[3:0]

			if (tempval&0x80)	//RF-indication, bit[7]
				priv->rf_type = RF_1T2R;
			else
				priv->rf_type = RF_2T4R;
		}
		else
		{
			priv->EEPROMLegacyHTTxPowerDiff = EEPROM_Default_LegacyHTTxPowerDiff;
		}
		RT_TRACE(COMP_INIT, "EEPROMLegacyHTTxPowerDiff = %d\n", 
			priv->EEPROMLegacyHTTxPowerDiff);
		
		// Read ThermalMeter from EEPROM
		if(!priv->AutoloadFailFlag)
		{
			priv->EEPROMThermalMeter = (u8)(((eprom_read(dev, (EEPROM_ThermalMeter>>1))) & 0xff00)>>8);
		}
		else
		{
			priv->EEPROMThermalMeter = EEPROM_Default_ThermalMeter;
		}
		RT_TRACE(COMP_INIT, "ThermalMeter = %d\n", priv->EEPROMThermalMeter);
		//vivi, for tx power track
		priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;
		
		if(priv->epromtype == EPROM_93c46)
		{
		// Read antenna tx power offset of B/C/D to A and CrystalCap from EEPROM
		if(!priv->AutoloadFailFlag)
		{
				usValue = eprom_read(dev, (EEPROM_TxPwDiff_CrystalCap>>1));
				priv->EEPROMAntPwDiff = (usValue&0x0fff);
				priv->EEPROMCrystalCap = (u8)((usValue&0xf000)>>12);
		}
		else
		{
				priv->EEPROMAntPwDiff = EEPROM_Default_AntTxPowerDiff;
				priv->EEPROMCrystalCap = EEPROM_Default_TxPwDiff_CrystalCap;
		}
			RT_TRACE(COMP_INIT, "EEPROMAntPwDiff = %d\n", priv->EEPROMAntPwDiff);
			RT_TRACE(COMP_INIT, "EEPROMCrystalCap = %d\n", priv->EEPROMCrystalCap);
		
		//
		// Get per-channel Tx Power Level
		//
		for(i=0; i<14; i+=2)
		{
			if(!priv->AutoloadFailFlag)
			{
				usValue = eprom_read(dev, (u16) ((EEPROM_TxPwIndex_CCK+i)>>1) );
			}
			else
			{
				usValue = EEPROM_Default_TxPower;
			}
			*((u16*)(&priv->EEPROMTxPowerLevelCCK[i])) = usValue;
			RT_TRACE(COMP_INIT,"CCK Tx Power Level, Index %d = 0x%02x\n", i, priv->EEPROMTxPowerLevelCCK[i]);
			RT_TRACE(COMP_INIT, "CCK Tx Power Level, Index %d = 0x%02x\n", i+1, priv->EEPROMTxPowerLevelCCK[i+1]);
		}
		for(i=0; i<14; i+=2)
		{
			if(!priv->AutoloadFailFlag)
			{
				usValue = eprom_read(dev, (u16) ((EEPROM_TxPwIndex_OFDM_24G+i)>>1) );
			}
			else
			{
				usValue = EEPROM_Default_TxPower;
			}
			*((u16*)(&priv->EEPROMTxPowerLevelOFDM24G[i])) = usValue;
			RT_TRACE(COMP_INIT, "OFDM 2.4G Tx Power Level, Index %d = 0x%02x\n", i, priv->EEPROMTxPowerLevelOFDM24G[i]);
			RT_TRACE(COMP_INIT, "OFDM 2.4G Tx Power Level, Index %d = 0x%02x\n", i+1, priv->EEPROMTxPowerLevelOFDM24G[i+1]);
		}
		}
		else if(priv->epromtype== EPROM_93c56)
		{
		#ifdef RTL8190P
			// Read CrystalCap from EEPROM
			if(!priv->AutoloadFailFlag)
			{
				priv->EEPROMAntPwDiff = EEPROM_Default_AntTxPowerDiff;
				priv->EEPROMCrystalCap = (u8)(((eprom_read(dev, (EEPROM_C56_CrystalCap>>1))) & 0xf000)>>12);
			}
			else
			{
				priv->EEPROMAntPwDiff = EEPROM_Default_AntTxPowerDiff;
				priv->EEPROMCrystalCap = EEPROM_Default_TxPwDiff_CrystalCap;
			}
			RT_TRACE(COMP_INIT,"EEPROMAntPwDiff = %d\n", priv->EEPROMAntPwDiff);
			RT_TRACE(COMP_INIT, "EEPROMCrystalCap = %d\n", priv->EEPROMCrystalCap);

			// Get Tx Power Level by Channel
			if(!priv->AutoloadFailFlag)
			{
				    // Read Tx power of Channel 1 ~ 14 from EEPROM.
			       for(i = 0; i < 12; i+=2)
				{
					if (i <6)
						offset = EEPROM_C56_RfA_CCK_Chnl1_TxPwIndex + i;
					else
						offset = EEPROM_C56_RfC_CCK_Chnl1_TxPwIndex + i - 6;
					usValue = eprom_read(dev, (offset>>1));
				       *((u16*)(&EepromTxPower[i])) = usValue;
				}

			       for(i = 0; i < 12; i++)
			       	{
			       		if (i <= 2)
						priv->EEPROMRfACCKChnl1TxPwLevel[i] = EepromTxPower[i];
					else if ((i >=3 )&&(i <= 5))
						priv->EEPROMRfAOfdmChnlTxPwLevel[i-3] = EepromTxPower[i];
					else if ((i >=6 )&&(i <= 8))
						priv->EEPROMRfCCCKChnl1TxPwLevel[i-6] = EepromTxPower[i];
					else
						priv->EEPROMRfCOfdmChnlTxPwLevel[i-9] = EepromTxPower[i];						
				}
			}
			else
			{
				priv->EEPROMRfACCKChnl1TxPwLevel[0] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfACCKChnl1TxPwLevel[1] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfACCKChnl1TxPwLevel[2] = EEPROM_Default_TxPowerLevel;

				priv->EEPROMRfAOfdmChnlTxPwLevel[0] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfAOfdmChnlTxPwLevel[1] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfAOfdmChnlTxPwLevel[2] = EEPROM_Default_TxPowerLevel;
				
				priv->EEPROMRfCCCKChnl1TxPwLevel[0] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfCCCKChnl1TxPwLevel[1] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfCCCKChnl1TxPwLevel[2] = EEPROM_Default_TxPowerLevel;

				priv->EEPROMRfCOfdmChnlTxPwLevel[0] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfCOfdmChnlTxPwLevel[1] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfCOfdmChnlTxPwLevel[2] = EEPROM_Default_TxPowerLevel;
			}
			RT_TRACE(COMP_INIT, "priv->EEPROMRfACCKChnl1TxPwLevel[0] = 0x%x\n", priv->EEPROMRfACCKChnl1TxPwLevel[0]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfACCKChnl1TxPwLevel[1] = 0x%x\n", priv->EEPROMRfACCKChnl1TxPwLevel[1]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfACCKChnl1TxPwLevel[2] = 0x%x\n", priv->EEPROMRfACCKChnl1TxPwLevel[2]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfAOfdmChnlTxPwLevel[0] = 0x%x\n", priv->EEPROMRfAOfdmChnlTxPwLevel[0]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfAOfdmChnlTxPwLevel[1] = 0x%x\n", priv->EEPROMRfAOfdmChnlTxPwLevel[1]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfAOfdmChnlTxPwLevel[2] = 0x%x\n", priv->EEPROMRfAOfdmChnlTxPwLevel[2]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCCCKChnl1TxPwLevel[0] = 0x%x\n", priv->EEPROMRfCCCKChnl1TxPwLevel[0]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCCCKChnl1TxPwLevel[1] = 0x%x\n", priv->EEPROMRfCCCKChnl1TxPwLevel[1]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCCCKChnl1TxPwLevel[2] = 0x%x\n", priv->EEPROMRfCCCKChnl1TxPwLevel[2]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCOfdmChnlTxPwLevel[0] = 0x%x\n", priv->EEPROMRfCOfdmChnlTxPwLevel[0]);			
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCOfdmChnlTxPwLevel[1] = 0x%x\n", priv->EEPROMRfCOfdmChnlTxPwLevel[1]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCOfdmChnlTxPwLevel[2] = 0x%x\n", priv->EEPROMRfCOfdmChnlTxPwLevel[2]);
#endif

		}
		//
		// Update HAL variables.
		//
		if(priv->epromtype == EPROM_93c46)
		{
			for(i=0; i<14; i++)
			{
				priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK[i];
				priv->TxPowerLevelOFDM24G[i] = priv->EEPROMTxPowerLevelOFDM24G[i];
			}
			priv->LegacyHTTxPowerDiff = priv->EEPROMLegacyHTTxPowerDiff;
		// Antenna B gain offset to antenna A, bit0~3
			priv->AntennaTxPwDiff[0] = (priv->EEPROMAntPwDiff & 0xf);
		// Antenna C gain offset to antenna A, bit4~7
			priv->AntennaTxPwDiff[1] = ((priv->EEPROMAntPwDiff & 0xf0)>>4);
		// Antenna D gain offset to antenna A, bit8~11
			priv->AntennaTxPwDiff[2] = ((priv->EEPROMAntPwDiff & 0xf00)>>8);
		// CrystalCap, bit12~15
			priv->CrystalCap = priv->EEPROMCrystalCap;
		// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
			priv->ThermalMeter[0] = (priv->EEPROMThermalMeter & 0xf);
			priv->ThermalMeter[1] = ((priv->EEPROMThermalMeter & 0xf0)>>4);
		}
		else if(priv->epromtype == EPROM_93c56)
		{
			//char	cck_pwr_diff_a=0, cck_pwr_diff_c=0;

			//cck_pwr_diff_a = pHalData->EEPROMRfACCKChnl7TxPwLevel - pHalData->EEPROMRfAOfdmChnlTxPwLevel[1];
			//cck_pwr_diff_c = pHalData->EEPROMRfCCCKChnl7TxPwLevel - pHalData->EEPROMRfCOfdmChnlTxPwLevel[1];
			for(i=0; i<3; i++)	// channel 1~3 use the same Tx Power Level.
			{
				priv->TxPowerLevelCCK_A[i]  = priv->EEPROMRfACCKChnl1TxPwLevel[0];
				priv->TxPowerLevelOFDM24G_A[i] = priv->EEPROMRfAOfdmChnlTxPwLevel[0];
				priv->TxPowerLevelCCK_C[i] =  priv->EEPROMRfCCCKChnl1TxPwLevel[0];
				priv->TxPowerLevelOFDM24G_C[i] = priv->EEPROMRfCOfdmChnlTxPwLevel[0];
			}
			for(i=3; i<9; i++)	// channel 4~9 use the same Tx Power Level
			{
				priv->TxPowerLevelCCK_A[i]  = priv->EEPROMRfACCKChnl1TxPwLevel[1];
				priv->TxPowerLevelOFDM24G_A[i] = priv->EEPROMRfAOfdmChnlTxPwLevel[1];
				priv->TxPowerLevelCCK_C[i] =  priv->EEPROMRfCCCKChnl1TxPwLevel[1];
				priv->TxPowerLevelOFDM24G_C[i] = priv->EEPROMRfCOfdmChnlTxPwLevel[1];
			}
			for(i=9; i<14; i++)	// channel 10~14 use the same Tx Power Level
			{
				priv->TxPowerLevelCCK_A[i]  = priv->EEPROMRfACCKChnl1TxPwLevel[2];
				priv->TxPowerLevelOFDM24G_A[i] = priv->EEPROMRfAOfdmChnlTxPwLevel[2];
				priv->TxPowerLevelCCK_C[i] =  priv->EEPROMRfCCCKChnl1TxPwLevel[2];
				priv->TxPowerLevelOFDM24G_C[i] = priv->EEPROMRfCOfdmChnlTxPwLevel[2];
			}
			for(i=0; i<14; i++)
				RT_TRACE(COMP_INIT, "priv->TxPowerLevelCCK_A[%d] = 0x%x\n", i, priv->TxPowerLevelCCK_A[i]);
			for(i=0; i<14; i++)
				RT_TRACE(COMP_INIT,"priv->TxPowerLevelOFDM24G_A[%d] = 0x%x\n", i, priv->TxPowerLevelOFDM24G_A[i]);
			for(i=0; i<14; i++)
				RT_TRACE(COMP_INIT, "priv->TxPowerLevelCCK_C[%d] = 0x%x\n", i, priv->TxPowerLevelCCK_C[i]);
			for(i=0; i<14; i++)
				RT_TRACE(COMP_INIT, "priv->TxPowerLevelOFDM24G_C[%d] = 0x%x\n", i, priv->TxPowerLevelOFDM24G_C[i]);
			priv->LegacyHTTxPowerDiff = priv->EEPROMLegacyHTTxPowerDiff;
			priv->AntennaTxPwDiff[0] = 0;
			priv->AntennaTxPwDiff[1] = 0;
			priv->AntennaTxPwDiff[2] = 0;
			priv->CrystalCap = priv->EEPROMCrystalCap;
			// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
			priv->ThermalMeter[0] = (priv->EEPROMThermalMeter & 0xf);
			priv->ThermalMeter[1] = ((priv->EEPROMThermalMeter & 0xf0)>>4);
		}
	}

	if(priv->rf_type == RF_1T2R)
	{		
		RT_TRACE(COMP_INIT, "\n1T2R config\n");
	}
	else if (priv->rf_type == RF_2T4R)
	{
		RT_TRACE(COMP_INIT, "\n2T4R config\n");
	}

	// 2008/01/16 MH We can only know RF type in the function. So we have to init
	// DIG RATR table again.
	init_rate_adaptive(dev);
	
	//1 Make a copy for following variables and we can change them if we want
	
	priv->rf_chip= RF_8256;

	if(priv->RegChannelPlan == 0xf)
	{
		priv->ChannelPlan = priv->eeprom_ChannelPlan;
	}
	else
	{
		priv->ChannelPlan = priv->RegChannelPlan;
	}

	//
	//  Used PID and DID to Set CustomerID 
	//
	if( priv->eeprom_vid == 0x1186 &&  priv->eeprom_did == 0x3304 )
	{
		priv->CustomerID =  RT_CID_DLINK;
	}
	
	switch(priv->eeprom_CustomerID)
	{
		case EEPROM_CID_DEFAULT:
			priv->CustomerID = RT_CID_DEFAULT;
			break;
		case EEPROM_CID_CAMEO:
			priv->CustomerID = RT_CID_819x_CAMEO;
			break;
		case  EEPROM_CID_RUNTOP:
			priv->CustomerID = RT_CID_819x_RUNTOP;
			break;
		case EEPROM_CID_NetCore:
			priv->CustomerID = RT_CID_819x_Netcore;
			break;	
		case EEPROM_CID_TOSHIBA:        // Merge by Jacken, 2008/01/31
			priv->CustomerID = RT_CID_TOSHIBA;
			if(priv->eeprom_ChannelPlan&0x80)
				priv->ChannelPlan = priv->eeprom_ChannelPlan&0x7f;
			else
				priv->ChannelPlan = 0x0;
			RT_TRACE(COMP_INIT, "Toshiba ChannelPlan = 0x%x\n", 
				priv->ChannelPlan);
			break;
		case EEPROM_CID_Nettronix:
			priv->ScanDelay = 100;	//cosa add for scan
			priv->CustomerID = RT_CID_Nettronix;
			break;			
		case EEPROM_CID_Pronet:
			priv->CustomerID = RT_CID_PRONET;
			break;
		case EEPROM_CID_DLINK:
			priv->CustomerID = RT_CID_DLINK;
			break;

		case EEPROM_CID_WHQL:
			//Adapter->bInHctTest = TRUE;//do not supported

			//priv->bSupportTurboMode = FALSE;
			//priv->bAutoTurboBy8186 = FALSE;

			//pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
			//pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
			//pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
			
			break;
		default:
			// value from RegCustomerID
			break;
	}

	//Avoid the channel plan array overflow, by Bruce, 2007-08-27. 
	if(priv->ChannelPlan > CHANNEL_PLAN_LEN - 1)
		priv->ChannelPlan = 0; //FCC

	switch(priv->CustomerID)
	{
		case RT_CID_DEFAULT:
		#ifdef RTL8190P
			priv->LedStrategy = HW_LED;
		#else
			#ifdef RTL8192E
			priv->LedStrategy = SW_LED_MODE1;
			#endif
		#endif
			break;

		case RT_CID_819x_CAMEO:
			priv->LedStrategy = SW_LED_MODE2;
			break;

		case RT_CID_819x_RUNTOP:
			priv->LedStrategy = SW_LED_MODE3;
			break;

		case RT_CID_819x_Netcore:
			priv->LedStrategy = SW_LED_MODE4;
			break;
	
		case RT_CID_Nettronix:
			priv->LedStrategy = SW_LED_MODE5;
			break;
			
		case RT_CID_PRONET:
			priv->LedStrategy = SW_LED_MODE6;
			break;
			
		case RT_CID_TOSHIBA:   //Modify by Jacken 2008/01/31
			// Do nothing.
			//break;

		default:
		#ifdef RTL8190P
			priv->LedStrategy = HW_LED;
		#else
			#ifdef RTL8192E
			priv->LedStrategy = SW_LED_MODE1;
			#endif
		#endif
			break;
	}
/*
	//2008.06.03, for WOL
	if( priv->eeprom_vid == 0x1186 &&  priv->eeprom_did == 0x3304)
		priv->ieee80211->bSupportRemoteWakeUp = TRUE;
	else
		priv->ieee80211->bSupportRemoteWakeUp = FALSE;
*/
	RT_TRACE(COMP_INIT, "RegChannelPlan(%d)\n", priv->RegChannelPlan);
	RT_TRACE(COMP_INIT, "ChannelPlan = %d \n", priv->ChannelPlan);
	RT_TRACE(COMP_INIT, "LedStrategy = %d \n", priv->LedStrategy);
	RT_TRACE(COMP_TRACE, "<==== ReadAdapterInfo\n");
		
	return ;
}


short rtl8192_get_channel_map(struct net_device * dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
#ifdef ENABLE_DOT11D
	if(priv->ChannelPlan> COUNTRY_CODE_GLOBAL_DOMAIN){
		printk("rtl8180_init:Error channel plan! Set to default.\n");
		priv->ChannelPlan= 0;
	}
	RT_TRACE(COMP_INIT, "Channel plan is %d\n",priv->ChannelPlan);
		
	rtl819x_set_channel_map(priv->ChannelPlan, priv);
#else
	int ch,i;
	//Set Default Channel Plan
	if(!channels){
		DMESG("No channels, aborting");
		return -1;
	}
	ch=channels;
	priv->ChannelPlan= 0;//hikaru
	 // set channels 1..14 allowed in given locale
	for (i=1; i<=14; i++) {
		(priv->ieee80211->channel_map)[i] = (u8)(ch & 0x01);
		ch >>= 1;
	}
#endif
	return 0;
}
short rtl8192_init(struct net_device *dev)
{		
	struct r8192_priv *priv = ieee80211_priv(dev);
	memset(&(priv->stats),0,sizeof(struct Stats));
	rtl8192_init_priv_variable(dev);
	rtl8192_init_priv_lock(priv);
	rtl8192_init_priv_task(dev);
	rtl8192_get_eeprom_size(dev);
	rtl8192_read_eeprom_info(dev);
	rtl8192_get_channel_map(dev);
	init_hal_dm(dev);
	init_timer(&priv->watch_dog_timer);
	priv->watch_dog_timer.data = (unsigned long)dev;
	priv->watch_dog_timer.function = watch_dog_timer_callback;
	if(rtl8192_pci_initdescring(dev)!=0){ 
		printk("Endopoints initialization failed");
		return -ENOMEM;
	}
#if defined(IRQF_SHARED)
        if(request_irq(dev->irq, rtl8192_interrupt, IRQF_SHARED, dev->name, dev)){
#else
        if(request_irq(dev->irq, (void *)rtl8192_interrupt, SA_SHIRQ, dev->name, dev)){
#endif
		printk("Error allocating IRQ %d",dev->irq);
		return -1;
	}else{ 
		priv->irq=dev->irq;
		printk("IRQ %d",dev->irq);
	}
	//rtl8192_rx_enable(dev);
	//rtl8192_adapter_start(dev);
//#ifdef DEBUG_EPROM
//	dump_eprom(dev);
//#endif 
	//rtl8192_dump_reg(dev);	
	return 0;
}

/******************************************************************************
 *function:  This function actually only set RRSR, RATR and BW_OPMODE registers 
 *	     not to do all the hw config as its name says
 *   input:  net_device dev
 *  output:  none
 *  return:  none
 *  notice:  This part need to modified according to the rate set we filtered
 * ****************************************************************************/
void rtl8192_hwconfig(struct net_device* dev)
{
	u32 regRATR = 0, regRRSR = 0;
	u8 regBwOpMode = 0, regTmp = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);

// Set RRSR, RATR, and BW_OPMODE registers
	//
	switch(priv->ieee80211->mode)
	{
	case WIRELESS_MODE_B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK;
		regRRSR = RATE_ALL_CCK;
		break;
	case WIRELESS_MODE_A:
		regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_G:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_AUTO:
	case WIRELESS_MODE_N_24G:
		// It support CCK rate by default.
		// CCK rate will be filtered out only when associated AP does not support it.
		regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_N_5G:
		regBwOpMode = BW_OPMODE_5G;
		regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	}

	write_nic_byte(dev, BW_OPMODE, regBwOpMode);
	{
		u32 ratr_value = 0;
		ratr_value = regRATR;
		if (priv->rf_type == RF_1T2R)
		{
			ratr_value &= ~(RATE_ALL_OFDM_2SS);
		}
		write_nic_dword(dev, RATR0, ratr_value);
		write_nic_byte(dev, UFWP, 1);
	}	
	regTmp = read_nic_byte(dev, 0x313);
	regRRSR = ((regTmp) << 24) | (regRRSR & 0x00ffffff);
	write_nic_dword(dev, RRSR, regRRSR);

	//
	// Set Retry Limit here
	//
	write_nic_word(dev, RETRY_LIMIT, 
			priv->ShortRetryLimit << RETRY_LIMIT_SHORT_SHIFT | \
			priv->LongRetryLimit << RETRY_LIMIT_LONG_SHIFT);
	// Set Contention Window here		

	// Set Tx AGC

	// Set Tx Antenna including Feedback control
		
	// Set Auto Rate fallback control
	
	
}


RT_STATUS rtl8192_adapter_start(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
//	struct ieee80211_device *ieee = priv->ieee80211;
	u32 ulRegRead;
	RT_STATUS rtStatus = RT_STATUS_SUCCESS;
//	static char szMACPHYRegFile[] = RTL819X_PHY_MACPHY_REG;
//	static char szMACPHYRegPGFile[] = RTL819X_PHY_MACPHY_REG_PG;
	//u8 eRFPath;
#ifdef RTL8192E
	u8 tmpvalue;
#endif
	bool bfirmwareok = true;
#ifdef RTL8190P
	u8 ucRegRead;
#endif
	u32	tmpRegA, tmpRegC, TempCCk;
	int	i =0;
//	u32 dwRegRead = 0;
	
	RT_TRACE(COMP_INIT, "====>%s()\n", __FUNCTION__);
	
	// 2007/11/02 MH Before initalizing RF. We can not use FW to do RF-R/W.
	priv->Rf_Mode = RF_OP_By_SW_3wire;
	#ifdef RTL8192E
	//dPLL on	
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
	write_nic_byte(dev, ANAPAR, 0x37);
	// Accordign to designer's explain, LBUS active will never > 10ms. We delay 10ms
	// Joseph increae the time to prevent firmware download fail
	mdelay(500);
	}
	#endif
	//PlatformSleepUs(10000);
	// For any kind of InitializeAdapter process, we shall use system now!!
	priv->pFirmware->firmware_status = FW_STATUS_0_INIT;

	// Set to eRfoff in order not to count receive count.
	if(priv->RegRfOff == TRUE)
		priv->eRFPowerState = eRfOff;

	//
	//3 //Config CPUReset Register
	//3//
	//3 Firmware Reset Or Not
	ulRegRead = read_nic_dword(dev, CPU_GEN);	
	if(priv->pFirmware->firmware_status == FW_STATUS_0_INIT)
	{	//called from MPInitialized. do nothing
		ulRegRead |= CPU_GEN_SYSTEM_RESET;
	}else if(priv->pFirmware->firmware_status == FW_STATUS_5_READY)
		ulRegRead |= CPU_GEN_FIRMWARE_RESET;	// Called from MPReset
	else
		RT_TRACE(COMP_ERR, "ERROR in %s(): undefined firmware state(%d)\n", __FUNCTION__,   priv->pFirmware->firmware_status);

#ifdef RTL8190P
	//2008.06.03, for WOL 90 hw bug
	ulRegRead &= (~(CPU_GEN_GPIO_UART));	
#endif
	
	write_nic_dword(dev, CPU_GEN, ulRegRead);
	//mdelay(100);

	//3//
	//3// Initialize BB before MAC
	//3//
	//rtl8192_dump_reg(dev);	
	RT_TRACE(COMP_INIT, "BB Config Start!\n");
	rtStatus = rtl8192_BBConfig(dev);
	if(rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_ERR, "BB Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT,"BB Config Finished!\n");

	//rtl8192_dump_reg(dev);	
	//
	//3//Set Loopback mode or Normal mode
	//3//
	//2006.12.13 by emily. Note!We should not merge these two CPU_GEN register writings 
	//	because setting of System_Reset bit reset MAC to default transmission mode.
		//Loopback mode or not
	priv->LoopbackMode = RTL819X_NO_LOOPBACK;
	//priv->LoopbackMode = RTL819X_MAC_LOOPBACK;
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
	ulRegRead = read_nic_dword(dev, CPU_GEN);	
	if(priv->LoopbackMode == RTL819X_NO_LOOPBACK)
	{
		ulRegRead = ((ulRegRead & CPU_GEN_NO_LOOPBACK_MSK) | CPU_GEN_NO_LOOPBACK_SET);
	}
	else if (priv->LoopbackMode == RTL819X_MAC_LOOPBACK )
	{
		ulRegRead |= CPU_CCK_LOOPBACK;
	}
	else
	{
		RT_TRACE(COMP_ERR,"Serious error: wrong loopback mode setting\n");	
	}

	//2008.06.03, for WOL
	//ulRegRead &= (~(CPU_GEN_GPIO_UART));	
	write_nic_dword(dev, CPU_GEN, ulRegRead);
		
	// 2006.11.29. After reset cpu, we sholud wait for a second, otherwise, it may fail to write registers. Emily
	udelay(500);
	}
	//3Set Hardware(Do nothing now)
	rtl8192_hwconfig(dev);
	//2=======================================================
	// Common Setting for all of the FPGA platform. (part 1)
	//2=======================================================
	// If there is changes, please make sure it applies to all of the FPGA version
	//3 Turn on Tx/Rx
	write_nic_byte(dev, CMDR, CR_RE|CR_TE);

	//2Set Tx dma burst 
#ifdef RTL8190P
	write_nic_byte(dev, PCIF, ((MXDMA2_NoLimit<<MXDMA2_RX_SHIFT) | \
											(MXDMA2_NoLimit<<MXDMA2_TX_SHIFT) | \
											(1<<MULRW_SHIFT)));
#else
	#ifdef RTL8192E
	write_nic_byte(dev, PCIF, ((MXDMA2_NoLimit<<MXDMA2_RX_SHIFT) |\
				   (MXDMA2_NoLimit<<MXDMA2_TX_SHIFT) ));
	#endif
#endif
	//set IDR0 here
	write_nic_dword(dev, MAC0, ((u32*)dev->dev_addr)[0]);
	write_nic_word(dev, MAC4, ((u16*)(dev->dev_addr + 4))[0]);
	//set RCR
	write_nic_dword(dev, RCR, priv->ReceiveConfig);

	//3 Initialize Number of Reserved Pages in Firmware Queue
	#ifdef TO_DO_LIST 
	if(priv->bInHctTest)
	{
		PlatformEFIOWrite4Byte(Adapter, RQPN1,  NUM_OF_PAGE_IN_FW_QUEUE_BK_DTM << RSVD_FW_QUEUE_PAGE_BK_SHIFT |\
                                       	NUM_OF_PAGE_IN_FW_QUEUE_BE_DTM << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
					NUM_OF_PAGE_IN_FW_QUEUE_VI_DTM << RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
					NUM_OF_PAGE_IN_FW_QUEUE_VO_DTM <<RSVD_FW_QUEUE_PAGE_VO_SHIFT);												
		PlatformEFIOWrite4Byte(Adapter, RQPN2, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT);
		PlatformEFIOWrite4Byte(Adapter, RQPN3, APPLIED_RESERVED_QUEUE_IN_FW| \
					NUM_OF_PAGE_IN_FW_QUEUE_BCN<<RSVD_FW_QUEUE_PAGE_BCN_SHIFT|\
					NUM_OF_PAGE_IN_FW_QUEUE_PUB_DTM<<RSVD_FW_QUEUE_PAGE_PUB_SHIFT);
	}
	else
	#endif
	{
		write_nic_dword(dev, RQPN1,  NUM_OF_PAGE_IN_FW_QUEUE_BK << RSVD_FW_QUEUE_PAGE_BK_SHIFT |\
					NUM_OF_PAGE_IN_FW_QUEUE_BE << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
					NUM_OF_PAGE_IN_FW_QUEUE_VI << RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
					NUM_OF_PAGE_IN_FW_QUEUE_VO <<RSVD_FW_QUEUE_PAGE_VO_SHIFT);												
		write_nic_dword(dev, RQPN2, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT);
		write_nic_dword(dev, RQPN3, APPLIED_RESERVED_QUEUE_IN_FW| \
					NUM_OF_PAGE_IN_FW_QUEUE_BCN<<RSVD_FW_QUEUE_PAGE_BCN_SHIFT|\
					NUM_OF_PAGE_IN_FW_QUEUE_PUB<<RSVD_FW_QUEUE_PAGE_PUB_SHIFT);
	}

	rtl8192_tx_enable(dev);
	rtl8192_rx_enable(dev);
	//3Set Response Rate Setting Register
	// CCK rate is supported by default.
	// CCK rate will be filtered out only when associated AP does not support it.
	ulRegRead = (0xFFF00000 & read_nic_dword(dev, RRSR))  | RATE_ALL_OFDM_AG | RATE_ALL_CCK;
	write_nic_dword(dev, RRSR, ulRegRead);
	write_nic_dword(dev, RATR0+4*7, (RATE_ALL_OFDM_AG | RATE_ALL_CCK));

	//2Set AckTimeout
	// TODO: (it value is only for FPGA version). need to be changed!!2006.12.18, by Emily
	write_nic_byte(dev, ACK_TIMEOUT, 0x30);

	//rtl8192_actset_wirelessmode(dev,priv->RegWirelessMode);
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	rtl8192_SetWirelessMode(dev, priv->ieee80211->mode);
	//-----------------------------------------------------------------------------
	// Set up security related. 070106, by rcnjko:
	// 1. Clear all H/W keys.
	// 2. Enable H/W encryption/decryption.
	//-----------------------------------------------------------------------------
	CamResetAllEntry(dev);
	{
		u8 SECR_value = 0x0;
		SECR_value |= SCR_TxEncEnable;
		SECR_value |= SCR_RxDecEnable;
		SECR_value |= SCR_NoSKMC;
		write_nic_byte(dev, SECR, SECR_value);
	}
	//3Beacon related
	write_nic_word(dev, ATIMWND, 2);
	write_nic_word(dev, BCN_INTERVAL, 100);
	
	//
	// Switching regulator controller: This is set temporarily. 
	// It's not sure if this can be removed in the future.
	// PJ advised to leave it by default.
	//
	write_nic_byte(dev, 0xbe, 0xc0);

	//2=======================================================
	// Set PHY related configuration defined in MAC register bank
	//2=======================================================
	rtl8192_phy_configmac(dev);

	if (priv->card_8192_version > (u8) VERSION_8190_BD) {
		rtl8192_phy_getTxPower(dev);
		rtl8192_phy_setTxPower(dev, priv->chan);
	}
#if 1
	//Firmware download 
	RT_TRACE(COMP_INIT, "Load Firmware!\n");
	bfirmwareok = init_firmware(dev);
	if(bfirmwareok != true) {
		rtStatus = RT_STATUS_FAILURE;
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "Load Firmware finished!\n");
#endif
	//RF config
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
	RT_TRACE(COMP_INIT, "RF Config Started!\n");
	rtStatus = rtl8192_phy_RFConfig(dev);
	if(rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_ERR, "RF Config failed\n");
			return rtStatus;
	}	
	RT_TRACE(COMP_INIT, "RF Config Finished!\n");
	}
	rtl8192_phy_updateInitGain(dev);

	/*---- Set CCK and OFDM Block "ON"----*/
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bCCKEn, 0x1);
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, 0x1);

#ifdef RTL8192E
	//Enable Led 
	write_nic_byte(dev, 0x87, 0x0);
#endif
#ifdef RTL8190P
	//2008.06.03, for WOL		
	ucRegRead = read_nic_byte(dev, GPE);
	ucRegRead |= BIT0;
	write_nic_byte(dev, GPE, ucRegRead);

	ucRegRead = read_nic_byte(dev, GPO);
	ucRegRead &= ~BIT0;
	write_nic_byte(dev, GPO, ucRegRead);
#endif

#ifdef RTL8190P
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
		dm_initialize_txpower_tracking(dev);
		
		tmpRegA= rtl8192_QueryBBReg(dev,rOFDM0_XATxIQImbalance,bMaskDWord);
		tmpRegC= rtl8192_QueryBBReg(dev,rOFDM0_XCTxIQImbalance,bMaskDWord);
		
		for(i = 0; i<TxBBGainTableLength; i++)
		{
			if(tmpRegA == priv->txbbgain_table[i].txbbgain_value)
			{
				priv->rfa_txpowertrackingindex= (u8)i;
				priv->rfa_txpowertrackingindex_real= (u8)i;
				priv->rfa_txpowertracking_default = priv->rfa_txpowertrackingindex;
				break;
			}
		}
		for(i = 0; i<TxBBGainTableLength; i++)
		{
			if(tmpRegC == priv->txbbgain_table[i].txbbgain_value)
			{
				priv->rfc_txpowertrackingindex= (u8)i;
				priv->rfc_txpowertrackingindex_real= (u8)i;
				break;
			}
		}
		TempCCk = rtl8192_QueryBBReg(dev, rCCK0_TxFilter1, bMaskByte2);				

		for(i=0 ; i<CCKTxBBGainTableLength ; i++)
		{
			if(TempCCk == priv->cck_txbbgain_table[i].ccktxbb_valuearray[0])
			{
				priv->CCKPresentAttentuation_20Mdefault =(u8) i;
				break;
			}
		}
		priv->CCKPresentAttentuation_40Mdefault = 0;
		priv->CCKPresentAttentuation_difference = 0;
		priv->CCKPresentAttentuation = priv->CCKPresentAttentuation_20Mdefault;
		RT_TRACE(COMP_POWER_TRACKING, "priv->rfa_txpowertrackingindex_initial = %d\n", priv->rfa_txpowertrackingindex);
		RT_TRACE(COMP_POWER_TRACKING, "priv->rfa_txpowertrackingindex_real__initial = %d\n", priv->rfa_txpowertrackingindex_real);
		RT_TRACE(COMP_POWER_TRACKING, "priv->rfc_txpowertrackingindex_initial = %d\n", priv->rfc_txpowertrackingindex);
		RT_TRACE(COMP_POWER_TRACKING, "priv->rfc_txpowertrackingindex_real_initial = %d\n", priv->rfc_txpowertrackingindex_real);
		RT_TRACE(COMP_POWER_TRACKING, "priv->CCKPresentAttentuation_difference_initial = %d\n", priv->CCKPresentAttentuation_difference);
		RT_TRACE(COMP_POWER_TRACKING, "priv->CCKPresentAttentuation_initial = %d\n", priv->CCKPresentAttentuation);
	}
#else
	#ifdef RTL8192E
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
		//if D or C cut
		tmpvalue = read_nic_byte(dev, 0x301);
		if(tmpvalue ==0x03)
		{
			priv->bDcut = TRUE;
			RT_TRACE(COMP_POWER_TRACKING, "D-cut\n");
		}
		else
		{	
			priv->bDcut = FALSE;
			RT_TRACE(COMP_POWER_TRACKING, "C-cut\n");
		}
		dm_initialize_txpower_tracking(dev);

		if(priv->bDcut == TRUE)
		{
			tmpRegA= rtl8192_QueryBBReg(dev,rOFDM0_XATxIQImbalance,bMaskDWord);
			tmpRegC= rtl8192_QueryBBReg(dev,rOFDM0_XCTxIQImbalance,bMaskDWord);
			for(i = 0; i<TxBBGainTableLength; i++)
			{
				if(tmpRegA == priv->txbbgain_table[i].txbbgain_value)
				{
					priv->rfa_txpowertrackingindex= (u8)i;
					priv->rfa_txpowertrackingindex_real= (u8)i;
					priv->rfa_txpowertracking_default = priv->rfa_txpowertrackingindex;
					break;
				}
			}

		TempCCk = rtl8192_QueryBBReg(dev, rCCK0_TxFilter1, bMaskByte2);				

		for(i=0 ; i<CCKTxBBGainTableLength ; i++)
		{
			if(TempCCk == priv->cck_txbbgain_table[i].ccktxbb_valuearray[0])
			{
				priv->CCKPresentAttentuation_20Mdefault =(u8) i;
				break;
			}
		}
		priv->CCKPresentAttentuation_40Mdefault = 0;
		priv->CCKPresentAttentuation_difference = 0;
		priv->CCKPresentAttentuation = priv->CCKPresentAttentuation_20Mdefault;
			RT_TRACE(COMP_POWER_TRACKING, "priv->rfa_txpowertrackingindex_initial = %d\n", priv->rfa_txpowertrackingindex);
			RT_TRACE(COMP_POWER_TRACKING, "priv->rfa_txpowertrackingindex_real__initial = %d\n", priv->rfa_txpowertrackingindex_real);
			RT_TRACE(COMP_POWER_TRACKING, "priv->CCKPresentAttentuation_difference_initial = %d\n", priv->CCKPresentAttentuation_difference);
			RT_TRACE(COMP_POWER_TRACKING, "priv->CCKPresentAttentuation_initial = %d\n", priv->CCKPresentAttentuation);
			priv->btxpower_tracking = FALSE;//TEMPLY DISABLE
		}
	}
	#endif
#endif
	rtl8192_irq_enable(dev);
	return rtStatus;
#if 0
	#ifdef TODO
	//2=======================================================
	// RF Power Save
	//2=======================================================
		if(pMgntInfo->RegRfOff == TRUE)
		{ // User disable RF via registry.
			RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8190(): Turn off RF for RegRfOff ----------\n"));
			MgntActSet_RF_State(Adapter, eRfOff, RF_CHANGE_BY_SW);

			// Those action will be discard in MgntActSet_RF_State because off the same state
		for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
		
		}
		else if(pMgntInfo->RfOffReason > RF_CHANGE_BY_PS)
		{ // H/W or S/W RF OFF before sleep.
			RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8190(): Turn off RF for RfOffReason(%d) ----------\n", pMgntInfo->RfOffReason));
			MgntActSet_RF_State(Adapter, eRfOff, pMgntInfo->RfOffReason);
		}
		else
		{
			pHalData->eRFPowerState = eRfOn;
			pMgntInfo->RfOffReason = 0; 
		// LED conttol
		Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_POWER_ON);

		//
		// If inactive power mode is enabled, disable rf while in disconnected state.
		// But we should still tell upper layer we are in rf on state.
		// 2007.07.16, by shien chang.
		//
			if(!Adapter->bInHctTest)			
		IPSEnter(Adapter);				

		}
	#endif
#endif
}

/* this configures registers for beacon tx and enables it via
 * rtl8192_beacon_tx_enable(). rtl8192_beacon_tx_disable() might
 * be used to stop beacon transmission
 */
#if 0
void rtl8192_start_tx_beacon(struct net_device *dev)
{
	int i;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u16 word;	
	DMESG("Enabling beacon TX");
	//write_nic_byte(dev, TX_CONF,0xe6);// TX_CONF
	//rtl8192_init_beacon(dev);
	//set_nic_txring(dev);
//	rtl8192_prepare_beacon(dev);
	rtl8192_irq_disable(dev);
//	rtl8192_beacon_tx_enable(dev);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	//write_nic_byte(dev,0x9d,0x20); //DMA Poll
	//write_nic_word(dev,0x7a,0);
	//write_nic_word(dev,0x7a,0x8000);

	
	word  = read_nic_word(dev, BcnItv);
	word &= ~BcnItv_BcnItv; // clear Bcn_Itv
	write_nic_word(dev, BcnItv, word);

	write_nic_word(dev, AtimWnd, 
		       read_nic_word(dev, AtimWnd) &~ AtimWnd_AtimWnd);
	
	word  = read_nic_word(dev, BCN_INTR_ITV);
	word &= ~BCN_INTR_ITV_MASK;
	
	//word |= priv->ieee80211->beacon_interval * 
	//	((priv->txbeaconcount > 1)?(priv->txbeaconcount-1):1);
	// FIXME:FIXME check if correct ^^ worked with 0x3e8;
	
	write_nic_word(dev, BCN_INTR_ITV, word);
	
	//write_nic_word(dev,0x2e,0xe002);
	//write_nic_dword(dev,0x30,0xb8c7832e);
	for(i=0; i<ETH_ALEN; i++)
		write_nic_byte(dev, BSSID+i, priv->ieee80211->beacon_cell_ssid[i]);
	
//	rtl8192_update_msr(dev);

	
	//write_nic_byte(dev,CONFIG4,3); /* !!!!!!!!!! */
	
	rtl8192_set_mode(dev, EPROM_CMD_NORMAL);
	
	rtl8192_irq_enable(dev);
	
	/* VV !!!!!!!!!! VV*/
	/*
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,0x9d,0x00); 	
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
*/
}
#endif
/***************************************************************************
    -------------------------------NET STUFF---------------------------
***************************************************************************/
#if 0
static struct net_device_stats *rtl8192_stats(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	return &priv->ieee80211->stats;
}
#endif



bool HalTxCheckStuck8190Pci(struct net_device *dev)
{
	u16 				RegTxCounter = read_nic_word(dev, 0x128);
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool				bStuck = FALSE;
	RT_TRACE(COMP_RESET,"%s():RegTxCounter is %d,TxCounter is %d\n",__FUNCTION__,RegTxCounter,priv->TxCounter);
	if(priv->TxCounter==RegTxCounter)
		bStuck = TRUE;

	priv->TxCounter = RegTxCounter;

	return bStuck;
}

/*
*	<Assumption: RT_TX_SPINLOCK is acquired.>
*	First added: 2006.11.19 by emily
*/
RESET_TYPE
TxCheckStuck(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8			QueueID;
	ptx_ring		head=NULL,tail=NULL,txring = NULL;
	u8			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
	bool			bCheckFwTxCnt = false;
	//unsigned long flags;
	
	//
	// Decide Stuch threshold according to current power save mode
	//
	//printk("++++++++++++>%s()\n",__FUNCTION__);
	switch (priv->ieee80211->dot11PowerSaveMode)
	{
		// The threshold value  may required to be adjusted .
		case eActive:		// Active/Continuous access.
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_NORMAL;
			break;
		case eMaxPs:		// Max power save mode.
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
		case eFastPs:	// Fast power save mode.
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
	}
			
	//
	// Check whether specific tcb has been queued for a specific time
	//
	for(QueueID = 0; QueueID < MAX_TX_QUEUE; QueueID++)
	{

	
		if(QueueID == TXCMD_QUEUE)
			continue;

		switch(QueueID) {
		case MGNT_QUEUE:
			tail=priv->txmapringtail;
			head=priv->txmapringhead;
			break;

		case BK_QUEUE:
			tail=priv->txbkpringtail;
			head=priv->txbkpringhead;
			break;

		case BE_QUEUE:
			tail=priv->txbepringtail;
			head=priv->txbepringhead;
			break;

		case VI_QUEUE:
			tail=priv->txvipringtail;
			head=priv->txvipringhead;
			break;

		case VO_QUEUE:
			tail=priv->txvopringtail;
			head=priv->txvopringhead;
			break;

		default:
			tail=head=NULL;
			break;
		}
		
		if(tail == head)
			continue;
		else
		{
			printk("============>QueueID is %d\n",QueueID);
			txring = head;
			if(txring == NULL)
			{
				RT_TRACE(COMP_ERR,"%s():txring is NULL , BUG!\n",__FUNCTION__);
				continue;
			}
			txring->nStuckCount++;
			#if 0	
			if(txring->nStuckCount > ResetThreshold)
			{
				RT_TRACE( COMP_RESET, "<== TxCheckStuck()\n" );
				return RESET_TYPE_NORMAL;
			}
			#endif
			bCheckFwTxCnt = TRUE;
		}
	}
#if 1 
	if(bCheckFwTxCnt)
	{
		if(HalTxCheckStuck8190Pci(dev))
		{
			RT_TRACE(COMP_RESET, "TxCheckStuck(): Fw indicates no Tx condition! \n");
			return RESET_TYPE_SILENT;
		}
	}
#endif
	return RESET_TYPE_NORESET;
}


bool HalRxCheckStuck8190Pci(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u16 				RegRxCounter = read_nic_word(dev, 0x130);
	bool				bStuck = FALSE;
	static u8			rx_chk_cnt = 0;
	RT_TRACE(COMP_RESET,"%s(): RegRxCounter is %d,RxCounter is %d\n",__FUNCTION__,RegRxCounter,priv->RxCounter);
	// If rssi is small, we should check rx for long time because of bad rx.
	// or maybe it will continuous silent reset every 2 seconds.
	rx_chk_cnt++;
	if(priv->undecorated_smoothed_pwdb >= (RateAdaptiveTH_High+5))
	{
		rx_chk_cnt = 0;	//high rssi, check rx stuck right now.
	}
	else if(priv->undecorated_smoothed_pwdb < (RateAdaptiveTH_High+5) &&
		((priv->CurrentChannelBW!=HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb>=RateAdaptiveTH_Low_40M) ||
		(priv->CurrentChannelBW==HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb>=RateAdaptiveTH_Low_20M)) )

	{
		if(rx_chk_cnt < 2)
		{
			return bStuck;
		}
		else
		{
			rx_chk_cnt = 0;
		}
	}
	else if(((priv->CurrentChannelBW!=HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb<RateAdaptiveTH_Low_40M) ||
		(priv->CurrentChannelBW==HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb<RateAdaptiveTH_Low_20M)) &&
		priv->undecorated_smoothed_pwdb >= VeryLowRSSI)
	{
		if(rx_chk_cnt < 4)
		{
			//DbgPrint("RSSI < %d && RSSI >= %d, no check this time \n", RateAdaptiveTH_Low, VeryLowRSSI);
			return bStuck;
		}
		else
		{
			rx_chk_cnt = 0;
			//DbgPrint("RSSI < %d && RSSI >= %d, check this time \n", RateAdaptiveTH_Low, VeryLowRSSI);
		}
	}
	else
	{
		if(rx_chk_cnt < 8)
		{
			//DbgPrint("RSSI <= %d, no check this time \n", VeryLowRSSI);
			return bStuck;
		}
		else
		{
			rx_chk_cnt = 0;
			//DbgPrint("RSSI <= %d, check this time \n", VeryLowRSSI);
		}
	}

	if(priv->RxCounter==RegRxCounter)
		bStuck = TRUE;

	priv->RxCounter = RegRxCounter;

	return bStuck;
}

RESET_TYPE RxCheckStuck(struct net_device *dev)
{
	
	if(HalRxCheckStuck8190Pci(dev))
	{
		RT_TRACE(COMP_RESET, "RxStuck Condition\n");
		return RESET_TYPE_SILENT;
	}
	
	return RESET_TYPE_NORESET;
}

RESET_TYPE
rtl819x_ifcheck_resetornot(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	RESET_TYPE	TxResetType = RESET_TYPE_NORESET;
	RESET_TYPE	RxResetType = RESET_TYPE_NORESET;
	RT_RF_POWER_STATE 	rfState;
	
	rfState = priv->eRFPowerState;
	
	TxResetType = TxCheckStuck(dev);
#if 1 
	if( rfState != eRfOff || 
		/*ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FW_DOWNLOAD_FAILURE)) &&*/
		(priv->ieee80211->iw_mode != IW_MODE_ADHOC))
	{
		// If driver is in the status of firmware download failure , driver skips RF initialization and RF is 
		// in turned off state. Driver should check whether Rx stuck and do silent reset. And
		// if driver is in firmware download failure status, driver should initialize RF in the following 
		// silent reset procedure Emily, 2008.01.21

		// Driver should not check RX stuck in IBSS mode because it is required to
		// set Check BSSID in order to send beacon, however, if check BSSID is 
		// set, STA cannot hear any packet a all. Emily, 2008.04.12
		RxResetType = RxCheckStuck(dev);
	}
#endif
	RT_TRACE(COMP_RESET,"%s(): TxResetType is %d, RxResetType is %d\n",__FUNCTION__,TxResetType,RxResetType);
	if(TxResetType==RESET_TYPE_NORMAL || RxResetType==RESET_TYPE_NORMAL)
		return RESET_TYPE_NORMAL;
	else if(TxResetType==RESET_TYPE_SILENT || RxResetType==RESET_TYPE_SILENT)
		return RESET_TYPE_SILENT;
	else
		return RESET_TYPE_NORESET;

}


void CamRestoreAllEntry(	struct net_device *dev)
{
	u8 EntryId = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8*	MacAddr = priv->ieee80211->current_network.bssid;
	
	static u8	CAM_CONST_ADDR[4][6] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x03}};
	static u8	CAM_CONST_BROAD[] = 
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	RT_TRACE(COMP_SEC, "CamRestoreAllEntry: \n");

		
	if ((priv->ieee80211->pairwise_key_type == KEY_TYPE_WEP40)||
	    (priv->ieee80211->pairwise_key_type == KEY_TYPE_WEP104))
	{
		
		for(EntryId=0; EntryId<4; EntryId++)
		{
			{
				MacAddr = CAM_CONST_ADDR[EntryId];
				setKey(dev, 
						EntryId ,
						EntryId,
						priv->ieee80211->pairwise_key_type,
						MacAddr, 
						0,
						NULL);			
			}	
		}	
		
	}
	else if(priv->ieee80211->pairwise_key_type == KEY_TYPE_TKIP)
	{
		
		{
			if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type, 
						(u8*)dev->dev_addr, 
						0,
						NULL);		
			else
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type, 
						MacAddr, 
						0,
						NULL);			
		}
	}
	else if(priv->ieee80211->pairwise_key_type == KEY_TYPE_CCMP)
	{

		{
			if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type,
						(u8*)dev->dev_addr, 
						0,
						NULL);		
			else
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type,
						MacAddr, 
						0,
						NULL);				
		}
	}



	if(priv->ieee80211->group_key_type == KEY_TYPE_TKIP)
	{
		MacAddr = CAM_CONST_BROAD;
		for(EntryId=1 ; EntryId<4 ; EntryId++)
		{
			{
				setKey(dev, 
						EntryId, 
						EntryId,
						priv->ieee80211->group_key_type,
						MacAddr, 
						0,
						NULL);			
			}	
		}
		if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						0,
						0,
						priv->ieee80211->group_key_type,
						CAM_CONST_ADDR[0], 
						0,
						NULL);		
	}
	else if(priv->ieee80211->group_key_type == KEY_TYPE_CCMP)
	{
		MacAddr = CAM_CONST_BROAD;
		for(EntryId=1; EntryId<4 ; EntryId++)
		{
			{	
				setKey(dev, 
						EntryId , 
						EntryId,
						priv->ieee80211->group_key_type,
						MacAddr, 
						0,
						NULL);			
			}	
		}

		if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						0 , 
						0,
						priv->ieee80211->group_key_type,
						CAM_CONST_ADDR[0], 
						0,
						NULL);
	}
}

void rtl8192_cancel_deferred_work(struct r8192_priv* priv);
int _rtl8192_up(struct net_device *dev);
//////////////////////////////////////////////////////////////
// This function is used to fix Tx/Rx stop bug temporarily.
// This function will do "system reset" to NIC when Tx or Rx is stuck.
// The method checking Tx/Rx stuck of this function is supported by FW,
// which reports Tx and Rx counter to register 0x128 and 0x130.
//////////////////////////////////////////////////////////////
void rtl819x_ifsilentreset(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8	reset_times = 0;
	int reset_status = 0;
	struct ieee80211_device *ieee = priv->ieee80211;
	

	// 2007.07.20. If we need to check CCK stop, please uncomment this line.
	//bStuck = Adapter->HalFunc.CheckHWStopHandler(Adapter);
		
	if(priv->ResetProgress==RESET_TYPE_NORESET)
	{
RESET_START:		

		RT_TRACE(COMP_RESET,"=========>Reset progress!! \n");
		
		// Set the variable for reset.
		priv->ResetProgress = RESET_TYPE_SILENT;
//		rtl8192_close(dev);
#if 1	
		down(&priv->wx_sem);	
		if(priv->up == 0)
		{
			RT_TRACE(COMP_ERR,"%s():the driver is not up! return\n",__FUNCTION__);
			up(&priv->wx_sem);
			return ;
		}
		priv->up = 0;
		RT_TRACE(COMP_RESET,"%s():======>start to down the driver\n",__FUNCTION__);
//		if(!netif_queue_stopped(dev))
//			netif_stop_queue(dev);

		dm_backup_dynamic_mechanism_state(dev);

		rtl8192_rtx_disable(dev);
		rtl8192_irq_disable(dev);
		rtl8192_cancel_deferred_work(priv);
		deinit_hal_dm(dev);
		del_timer_sync(&priv->watch_dog_timer);	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		cancel_delayed_work(&priv->watch_dog_wq);
		cancel_delayed_work(&priv->update_beacon_wq);
#endif	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
		cancel_work_sync(&priv->qos_activate);
#else
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		cancel_delayed_work(&priv->qos_activate);
#endif
#endif
		ieee->sync_scan_hurryup = 1;
		if(ieee->state == IEEE80211_LINKED)
		{
			down(&ieee->wx_sem);
			printk("ieee->state is IEEE80211_LINKED\n");
			ieee80211_stop_send_beacons(priv->ieee80211);
			del_timer_sync(&ieee->associate_timer);
			#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
			cancel_delayed_work(&ieee->associate_retry_wq);	
			#endif	
			ieee80211_stop_scan(ieee);
			netif_carrier_off(dev);
			up(&ieee->wx_sem);
		}
		else{
			printk("ieee->state is NOT LINKED\n");
			ieee80211_softmac_stop_protocol(priv->ieee80211);		
		}
		up(&priv->wx_sem);
		RT_TRACE(COMP_RESET,"%s():<==========down process is finished\n",__FUNCTION__);	
		RT_TRACE(COMP_RESET,"%s():===========>start to up the driver\n",__FUNCTION__);
		reset_status = _rtl8192_up(dev);
		
		RT_TRACE(COMP_RESET,"%s():<===========up process is finished\n",__FUNCTION__);
		if(reset_status == -1)
		{
			if(reset_times < 3)
			{
				reset_times++;
				goto RESET_START;
			}
			else
			{
				RT_TRACE(COMP_ERR," ERR!!! %s():  Reset Failed!!\n",__FUNCTION__);
			}
		}
#endif
		ieee->is_silent_reset = 1;
#if 1 
		EnableHWSecurityConfig8192(dev);
#if 1 
		if(ieee->state == IEEE80211_LINKED && ieee->iw_mode == IW_MODE_INFRA)
		{
			ieee->set_chan(ieee->dev, ieee->current_network.channel);
		
#if 1 
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)		
			queue_work(ieee->wq, &ieee->associate_complete_wq);
#else
			schedule_task(&ieee->associate_complete_wq);
#endif
#endif
	
		}
		else if(ieee->state == IEEE80211_LINKED && ieee->iw_mode == IW_MODE_ADHOC)
		{
			ieee->set_chan(ieee->dev, ieee->current_network.channel);
			ieee->link_change(ieee->dev);
	
		//	notify_wx_assoc_event(ieee);
	
			ieee80211_start_send_beacons(ieee);
	
			if (ieee->data_hard_resume)
				ieee->data_hard_resume(ieee->dev);
			netif_carrier_on(ieee->dev);
		}
#endif
		
		CamRestoreAllEntry(dev);

		// Restore the previous setting for all dynamic mechanism
		dm_restore_dynamic_mechanism_state(dev);

		priv->ResetProgress = RESET_TYPE_NORESET;
		priv->reset_count++;

		priv->bForcedSilentReset =false;
		priv->bResetInProgress = false;

		// For test --> force write UFWP.
		write_nic_byte(dev, UFWP, 1);	
		RT_TRACE(COMP_RESET, "Reset finished!! ====>[%d]\n", priv->reset_count);
#endif
	}
}

void CAM_read_entry(
	struct net_device *dev,
	u32	 		iIndex 
)
{
 	u32 target_command=0;
	 u32 target_content=0;
	 u8 entry_i=0;
	 u32 ulStatus;
	s32 i=100;
//	printk("=======>start read CAM\n"); 
 	for(entry_i=0;entry_i<CAM_CONTENT_COUNT;entry_i++)
 	{
   	// polling bit, and No Write enable, and address
		target_command= entry_i+CAM_CONTENT_COUNT*iIndex;
		target_command= target_command | BIT31;

	//Check polling bit is clear
//	mdelay(1);
#if 1
		while((i--)>=0)
		{
			ulStatus = read_nic_dword(dev, RWCAM);
			if(ulStatus & BIT31){
				continue;
			}
			else{
				break;
			}
		}
#endif
  		write_nic_dword(dev, RWCAM, target_command);
   	 	RT_TRACE(COMP_SEC,"CAM_read_entry(): WRITE A0: %x \n",target_command);
   	 //	printk("CAM_read_entry(): WRITE A0: %lx \n",target_command);
  	 	target_content = read_nic_dword(dev, RCAMO);
  	 	RT_TRACE(COMP_SEC, "CAM_read_entry(): WRITE A8: %x \n",target_content);
  	 //	printk("CAM_read_entry(): WRITE A8: %lx \n",target_content);
 	}
	printk("\n");
}

void rtl819x_update_rxcounts(
	struct r8192_priv *priv,
	u32* TotalRxBcnNum,
	u32* TotalRxDataNum
)	
{
	u16 			SlotIndex;
	u8			i;

	*TotalRxBcnNum = 0;
	*TotalRxDataNum = 0;
	
	SlotIndex = (priv->ieee80211->LinkDetectInfo.SlotIndex++)%(priv->ieee80211->LinkDetectInfo.SlotNum);
	priv->ieee80211->LinkDetectInfo.RxBcnNum[SlotIndex] = priv->ieee80211->LinkDetectInfo.NumRecvBcnInPeriod;
	priv->ieee80211->LinkDetectInfo.RxDataNum[SlotIndex] = priv->ieee80211->LinkDetectInfo.NumRecvDataInPeriod;
	for( i=0; i<priv->ieee80211->LinkDetectInfo.SlotNum; i++ ){	
		*TotalRxBcnNum += priv->ieee80211->LinkDetectInfo.RxBcnNum[i];
		*TotalRxDataNum += priv->ieee80211->LinkDetectInfo.RxDataNum[i];
	}
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
extern	void	rtl819x_watchdog_wqcallback(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
       struct r8192_priv *priv = container_of(dwork,struct r8192_priv,watch_dog_wq);
       struct net_device *dev = priv->ieee80211->dev;
#else
extern	void	rtl819x_watchdog_wqcallback(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
#endif
	RESET_TYPE	ResetType = RESET_TYPE_NORESET;
      	static u8	check_reset_cnt=0;
	unsigned long flags;
	if(!priv->up)
		return;
	hal_dm_watchdog(dev);

	//added by amy for AP roaming
	{
		if(priv->ieee80211->state == IEEE80211_LINKED && priv->ieee80211->iw_mode == IW_MODE_INFRA)
		{
			u32	TotalRxBcnNum = 0;
			u32	TotalRxDataNum = 0;	

			rtl819x_update_rxcounts(priv, &TotalRxBcnNum, &TotalRxDataNum);
			if((TotalRxBcnNum+TotalRxDataNum) == 0)
			{
				#ifdef TODO
				if(rfState == eRfOff)
					RT_TRACE(COMP_ERR,"========>%s()\n",__FUNCTION__);
				#endif
				printk("===>%s(): AP is power off,connect another one\n",__FUNCTION__);
		//		Dot11d_Reset(dev);
				priv->ieee80211->state = IEEE80211_ASSOCIATING;
				notify_wx_assoc_event(priv->ieee80211);
                                RemovePeerTS(priv->ieee80211,priv->ieee80211->current_network.bssid);
                                priv->ieee80211->link_change(dev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
                                queue_work(priv->ieee80211->wq, &priv->ieee80211->associate_procedure_wq);
#else
                                schedule_task(&priv->ieee80211->associate_procedure_wq);
#endif

			}
		}
		priv->ieee80211->LinkDetectInfo.NumRecvBcnInPeriod=0;
                priv->ieee80211->LinkDetectInfo.NumRecvDataInPeriod=0;
		
	}
	
//	CAM_read_entry(dev,4);
	//check if reset the driver
	spin_lock_irqsave(&priv->tx_lock,flags);
	if(check_reset_cnt++ >= 3)
	{
    		ResetType = rtl819x_ifcheck_resetornot(dev);
		check_reset_cnt = 3;
		//DbgPrint("Start to check silent reset\n");
	}
	spin_unlock_irqrestore(&priv->tx_lock,flags);
	if(!priv->bDisableNormalResetCheck && ResetType == RESET_TYPE_NORMAL)
	{
		priv->ResetProgress = RESET_TYPE_NORMAL;
		RT_TRACE(COMP_RESET,"%s(): NOMAL RESET\n",__FUNCTION__);
		return;
	}
	/* disable silent reset temply 2008.9.11*/
#if 0 
	if( ((priv->force_reset) || (!priv->bDisableNormalResetCheck && ResetType==RESET_TYPE_SILENT))) // This is control by OID set in Pomelo
	{
		rtl819x_ifsilentreset(dev);
	}
#endif
	priv->force_reset = false;
	priv->bForcedSilentReset = false;
	priv->bResetInProgress = false;
	RT_TRACE(COMP_INIT, " <==RtUsbCheckForHangWorkItemCallback()\n");
	
}

void watch_dog_timer_callback(unsigned long data)
{
	struct r8192_priv *priv = ieee80211_priv((struct net_device *) data);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	queue_delayed_work(priv->priv_wq,&priv->watch_dog_wq,0);
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	schedule_task(&priv->watch_dog_wq);
#else
	queue_work(priv->priv_wq,&priv->watch_dog_wq);
#endif
#endif
	mod_timer(&priv->watch_dog_timer, jiffies + MSECS(IEEE80211_WATCH_DOG_TIME));

}
int _rtl8192_up(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//int i;
	RT_STATUS init_status = RT_STATUS_SUCCESS;
	priv->up=1;
	priv->ieee80211->ieee_up=1;	
	RT_TRACE(COMP_INIT, "Bringing up iface");
//	rtl8192_rx_enable(dev);
//	rtl8192_tx_enable(dev);

	init_status = rtl8192_adapter_start(dev);
	if(init_status != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
		return -1;
	}
	RT_TRACE(COMP_INIT, "start adapter finished\n");

	if(priv->ieee80211->state != IEEE80211_LINKED)
	ieee80211_softmac_start_protocol(priv->ieee80211);
	ieee80211_reset_queue(priv->ieee80211);
	watch_dog_timer_callback((unsigned long) dev);
	if(!netif_queue_stopped(dev))
		netif_start_queue(dev);
	else
		netif_wake_queue(dev);

	return 0;
}


int rtl8192_open(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);
	ret = rtl8192_up(dev);
	up(&priv->wx_sem);
	return ret;
	
}


int rtl8192_up(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	if (priv->up == 1) return -1;
	
	return _rtl8192_up(dev);
}


int rtl8192_close(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);

	ret = rtl8192_down(dev);
	
	up(&priv->wx_sem);
	
	return ret;

}

int rtl8192_down(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int i;

	if (priv->up == 0) return -1;
	
	priv->up=0;
	priv->ieee80211->ieee_up = 0;
	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
/* FIXME */
	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);
	
	rtl8192_rtx_disable(dev);
	rtl8192_irq_disable(dev);

        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->ieee80211->skb_waitQ [i]);
        }
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->ieee80211->skb_aggQ [i]);
        }
//	flush_scheduled_work();
	rtl8192_cancel_deferred_work(priv);
	deinit_hal_dm(dev);
	del_timer_sync(&priv->watch_dog_timer);	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
        cancel_delayed_work(&priv->watch_dog_wq);
	cancel_delayed_work(&priv->update_beacon_wq);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	cancel_work_sync(&priv->qos_activate);
#else
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	cancel_delayed_work(&priv->qos_activate);
#endif
#endif
	//cancel_delayed_work(&priv->SwChnlWorkItem);

	ieee80211_softmac_stop_protocol(priv->ieee80211);
	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

		return 0;
}


void rtl8192_commit(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	if (priv->up == 0) return ;


	ieee80211_softmac_stop_protocol(priv->ieee80211);
	
	rtl8192_irq_disable(dev);
	rtl8192_rtx_disable(dev);
	_rtl8192_up(dev);
}

/*
void rtl8192_restart(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_restart(struct work_struct *work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, reset_wq);
        struct net_device *dev = priv->ieee80211->dev;
#else
void rtl8192_restart(struct net_device *dev)
{

        struct r8192_priv *priv = ieee80211_priv(dev);
#endif

	down(&priv->wx_sem);
	
	rtl8192_commit(dev);
	
	up(&priv->wx_sem);
}

static void r8192_set_multicast(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	short promisc;

	//down(&priv->wx_sem);
	
	/* FIXME FIXME */
	
	promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	
	if (promisc != priv->promisc) {
		;
	//	rtl8192_commit(dev);
	}
	
	priv->promisc = promisc;
	
	//schedule_work(&priv->reset_wq);
	//up(&priv->wx_sem);
}


int r8192_set_mac_adr(struct net_device *dev, void *mac)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct sockaddr *addr = mac;
	
	down(&priv->wx_sem);
	
	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
		
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	schedule_work(&priv->reset_wq);
#else
	schedule_task(&priv->reset_wq);
#endif	
	up(&priv->wx_sem);
	
	return 0;
}

/* based on ipw2200 driver */
int rtl8192_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct iwreq *wrq = (struct iwreq *)rq;
	int ret=-1;
	struct ieee80211_device *ieee = priv->ieee80211;
	u32 key[4];
	u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	struct iw_point *p = &wrq->u.data;
	struct ieee_param *ipw = NULL;//(struct ieee_param *)wrq->u.data.pointer;

	down(&priv->wx_sem);


     if (p->length < sizeof(struct ieee_param) || !p->pointer){
             ret = -EINVAL;
             goto out;
     }

     ipw = (struct ieee_param *)kmalloc(p->length, GFP_KERNEL);
     if (ipw == NULL){
             ret = -ENOMEM;
             goto out;
     }
     if (copy_from_user(ipw, p->pointer, p->length)) {
            kfree(ipw);
            ret = -EFAULT;
            goto out;
     }

	switch (cmd) {
	    case RTL_IOCTL_WPA_SUPPLICANT:
		//parse here for HW security	
			if (ipw->cmd == IEEE_CMD_SET_ENCRYPTION)
			{
				if (ipw->u.crypt.set_tx)
				{
					if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
						ieee->pairwise_key_type = KEY_TYPE_CCMP;
					else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
						ieee->pairwise_key_type = KEY_TYPE_TKIP;
					else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
					{
						if (ipw->u.crypt.key_len == 13)
							ieee->pairwise_key_type = KEY_TYPE_WEP104;
						else if (ipw->u.crypt.key_len == 5)
							ieee->pairwise_key_type = KEY_TYPE_WEP40;
					}
					else
						ieee->pairwise_key_type = KEY_TYPE_NA;

					if (ieee->pairwise_key_type)
					{
						memcpy((u8*)key, ipw->u.crypt.key, 16);
						EnableHWSecurityConfig8192(dev);
					//we fill both index entry and 4th entry for pairwise key as in IPW interface, adhoc will only get here, so we need index entry for its default key serching!
					//added by WB.
						setKey(dev, 4, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key);
						if (ieee->mode == IW_MODE_ADHOC)
						setKey(dev, ipw->u.crypt.idx, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key);
					}
				}
				if (ipw->u.crypt.idx) //group key use idx > 0
				{
					memcpy((u8*)key, ipw->u.crypt.key, 16);
					if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
						ieee->group_key_type= KEY_TYPE_CCMP;
					else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
						ieee->group_key_type = KEY_TYPE_TKIP;
					else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
					{
						if (ipw->u.crypt.key_len == 13)
							ieee->group_key_type = KEY_TYPE_WEP104;
						else if (ipw->u.crypt.key_len == 5)
							ieee->group_key_type = KEY_TYPE_WEP40;
					}
					else
						ieee->group_key_type = KEY_TYPE_NA;

					if (ieee->group_key_type)
					{
							setKey(	dev, 
								ipw->u.crypt.idx,
								ipw->u.crypt.idx,		//KeyIndex 
						     		ieee->group_key_type,	//KeyType
						            	broadcast_addr,	//MacAddr
								0,		//DefaultKey
							      	key);		//KeyContent
					}
				}
			}
#ifdef JOHN_DEBUG
		//john's test 0711
	{
		int i;
		printk("@@ wrq->u pointer = ");
		for(i=0;i<wrq->u.data.length;i++){
			if(i%10==0) printk("\n");
			printk( "%8x|", ((u32*)wrq->u.data.pointer)[i] );
		}
		printk("\n");
	}
#endif /*JOHN_DEBUG*/
		ret = ieee80211_wpa_supplicant_ioctl(priv->ieee80211, &wrq->u.data);
		break; 

	    default:
		ret = -EOPNOTSUPP;
		break;
	}

	kfree(ipw);
out:
	up(&priv->wx_sem);
	
	return ret;
}

u8 HwRateToMRate90(bool bIsHT, u8 rate)
{
	u8  ret_rate = 0x02;

	if(!bIsHT) {
		switch(rate) {
			case DESC90_RATE1M:   ret_rate = MGN_1M;         break;
			case DESC90_RATE2M:   ret_rate = MGN_2M;         break;
			case DESC90_RATE5_5M: ret_rate = MGN_5_5M;       break;
			case DESC90_RATE11M:  ret_rate = MGN_11M;        break;
			case DESC90_RATE6M:   ret_rate = MGN_6M;         break;
			case DESC90_RATE9M:   ret_rate = MGN_9M;         break;
			case DESC90_RATE12M:  ret_rate = MGN_12M;        break;
			case DESC90_RATE18M:  ret_rate = MGN_18M;        break;
			case DESC90_RATE24M:  ret_rate = MGN_24M;        break;
			case DESC90_RATE36M:  ret_rate = MGN_36M;        break;
			case DESC90_RATE48M:  ret_rate = MGN_48M;        break;
			case DESC90_RATE54M:  ret_rate = MGN_54M;        break;

			default:
					      RT_TRACE(COMP_RECV, "HwRateToMRate90(): Non supported Rate [%x], bIsHT = %d!!!\n", rate, bIsHT);
					      break;
		}

	} else {
		switch(rate) {
			case DESC90_RATEMCS0:   ret_rate = MGN_MCS0;    break;
			case DESC90_RATEMCS1:   ret_rate = MGN_MCS1;    break;
			case DESC90_RATEMCS2:   ret_rate = MGN_MCS2;    break;
			case DESC90_RATEMCS3:   ret_rate = MGN_MCS3;    break;
			case DESC90_RATEMCS4:   ret_rate = MGN_MCS4;    break;
			case DESC90_RATEMCS5:   ret_rate = MGN_MCS5;    break;
			case DESC90_RATEMCS6:   ret_rate = MGN_MCS6;    break;
			case DESC90_RATEMCS7:   ret_rate = MGN_MCS7;    break;
			case DESC90_RATEMCS8:   ret_rate = MGN_MCS8;    break;
			case DESC90_RATEMCS9:   ret_rate = MGN_MCS9;    break;
			case DESC90_RATEMCS10:  ret_rate = MGN_MCS10;   break;
			case DESC90_RATEMCS11:  ret_rate = MGN_MCS11;   break;
			case DESC90_RATEMCS12:  ret_rate = MGN_MCS12;   break;
			case DESC90_RATEMCS13:  ret_rate = MGN_MCS13;   break;
			case DESC90_RATEMCS14:  ret_rate = MGN_MCS14;   break;
			case DESC90_RATEMCS15:  ret_rate = MGN_MCS15;   break;
			case DESC90_RATEMCS32:  ret_rate = (0x80|0x20); break;

			default:
						RT_TRACE(COMP_RECV, "HwRateToMRate90(): Non supported Rate [%x], bIsHT = %d!!!\n",rate, bIsHT);
						break;
		}
	}

	return ret_rate;
}

/**
 * Function:     UpdateRxPktTimeStamp
 * Overview:     Recored down the TSF time stamp when receiving a packet
 * 
 * Input:                
 *       PADAPTER        Adapter
 *       PRT_RFD         pRfd,
 *                       
 * Output:               
 *       PRT_RFD         pRfd
 *                               (pRfd->Status.TimeStampHigh is updated)
 *                               (pRfd->Status.TimeStampLow is updated)
 * Return:       
 *               None
 */
void UpdateRxPktTimeStamp8190 (struct net_device *dev, struct ieee80211_rx_stats *stats)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	if(stats->bIsAMPDU && !stats->bFirstMPDU) {
		stats->mac_time[0] = priv->LastRxDescTSFLow;
		stats->mac_time[1] = priv->LastRxDescTSFHigh;
	} else {
		priv->LastRxDescTSFLow = stats->mac_time[0];
		priv->LastRxDescTSFHigh = stats->mac_time[1];
	}
}

long rtl819x_translate_todbm(u8 signal_strength_index	)// 0-100 index.
{
	long	signal_power; // in dBm.

	// Translate to dBm (x=0.5y-95).
	signal_power = (long)((signal_strength_index + 1) >> 1); 
	signal_power -= 95; 

	return signal_power;
}

//
//	Description:
//		Update Rx signal related information in the packet reeived 
//		to RxStats. User application can query RxStats to realize 
//		current Rx signal status. 
//	
//	Assumption:
//		In normal operation, user only care about the information of the BSS 
//		and we shall invoke this function if the packet received is from the BSS.
//
void
rtl819x_update_rxsignalstatistics8190pci(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pprevious_stats
	)
{
	int weighting = 0;

	//2 <ToDo> Update Rx Statistics (such as signal strength and signal quality).

	// Initila state
	if(priv->stats.recv_signal_power == 0)
		priv->stats.recv_signal_power = pprevious_stats->RecvSignalPower;

	// To avoid the past result restricting the statistics sensitivity, weight the current power (5/6) to speed up the 
	// reaction of smoothed Signal Power.
	if(pprevious_stats->RecvSignalPower > priv->stats.recv_signal_power)
		weighting = 5;
	else if(pprevious_stats->RecvSignalPower < priv->stats.recv_signal_power)
		weighting = (-5);
	//
	// We need more correct power of received packets and the  "SignalStrength" of RxStats have been beautified or translated,
	// so we record the correct power in Dbm here. By Bruce, 2008-03-07. 
	//
	priv->stats.recv_signal_power = (priv->stats.recv_signal_power * 5 + pprevious_stats->RecvSignalPower + weighting) / 6;
}

void
rtl8190_process_cck_rxpathsel(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pprevious_stats
	)
{
#ifdef RTL8190P	//Only 90P 2T4R need to check
	char				last_cck_adc_pwdb[4]={0,0,0,0};
	u8				i;
//cosa add for Rx path selection
		if(priv->rf_type == RF_2T4R && DM_RxPathSelTable.Enable)
		{
			if(pprevious_stats->bIsCCK && 
				(pprevious_stats->bPacketToSelf ||pprevious_stats->bPacketBeacon))
			{
				/* record the cck adc_pwdb to the sliding window. */
				if(priv->stats.cck_adc_pwdb.TotalNum++ >= PHY_RSSI_SLID_WIN_MAX)
				{
					priv->stats.cck_adc_pwdb.TotalNum = PHY_RSSI_SLID_WIN_MAX;
					for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
					{
						last_cck_adc_pwdb[i] = priv->stats.cck_adc_pwdb.elements[i][priv->stats.cck_adc_pwdb.index];
						priv->stats.cck_adc_pwdb.TotalVal[i] -= last_cck_adc_pwdb[i];
					}
				}
				for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
				{
					priv->stats.cck_adc_pwdb.TotalVal[i] += pprevious_stats->cck_adc_pwdb[i];
					priv->stats.cck_adc_pwdb.elements[i][priv->stats.cck_adc_pwdb.index] = pprevious_stats->cck_adc_pwdb[i];
				}
				priv->stats.cck_adc_pwdb.index++;
				if(priv->stats.cck_adc_pwdb.index >= PHY_RSSI_SLID_WIN_MAX)
					priv->stats.cck_adc_pwdb.index = 0;

				for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
				{
					DM_RxPathSelTable.cck_pwdb_sta[i] = priv->stats.cck_adc_pwdb.TotalVal[i]/priv->stats.cck_adc_pwdb.TotalNum;
				}

				for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
				{
					if(pprevious_stats->cck_adc_pwdb[i]  > (char)priv->undecorated_smoothed_cck_adc_pwdb[i])
					{
						priv->undecorated_smoothed_cck_adc_pwdb[i] = 
							( (priv->undecorated_smoothed_cck_adc_pwdb[i]*(Rx_Smooth_Factor-1)) + 
							(pprevious_stats->cck_adc_pwdb[i])) /(Rx_Smooth_Factor);
						priv->undecorated_smoothed_cck_adc_pwdb[i] = priv->undecorated_smoothed_cck_adc_pwdb[i] + 1;
					}
					else
					{
						priv->undecorated_smoothed_cck_adc_pwdb[i] = 
							( (priv->undecorated_smoothed_cck_adc_pwdb[i]*(Rx_Smooth_Factor-1)) + 
							(pprevious_stats->cck_adc_pwdb[i])) /(Rx_Smooth_Factor);
					}
				}
			}
		}
#endif
}


/* 2008/01/22 MH We can not delcare RSSI/EVM total value of sliding window to
	be a local static. Otherwise, it may increase when we return from S3/S4. The
	value will be kept in memory or disk. We must delcare the value in adapter
	and it will be reinitialized when return from S3/S4. */
void rtl8192_process_phyinfo(struct r8192_priv * priv, u8* buffer,struct ieee80211_rx_stats * pprevious_stats, struct ieee80211_rx_stats * pcurrent_stats)
{
	bool bcheck = false;
	u8	rfpath;
	u32 nspatial_stream, tmp_val;
	//u8	i;
	static u32 slide_rssi_index=0, slide_rssi_statistics=0; 
	static u32 slide_evm_index=0, slide_evm_statistics=0;
	static u32 last_rssi=0, last_evm=0;
	//cosa add for rx path selection 
//	static long slide_cck_adc_pwdb_index=0, slide_cck_adc_pwdb_statistics=0;
//	static char last_cck_adc_pwdb[4]={0,0,0,0};
	//cosa add for beacon rssi smoothing
	static u32 slide_beacon_adc_pwdb_index=0, slide_beacon_adc_pwdb_statistics=0;
	static u32 last_beacon_adc_pwdb=0;

	struct ieee80211_hdr_3addr *hdr;
	u16 sc ;
	unsigned int frag,seq;
	hdr = (struct ieee80211_hdr_3addr *)buffer;
	sc = le16_to_cpu(hdr->seq_ctl);
	frag = WLAN_GET_SEQ_FRAG(sc);
	seq = WLAN_GET_SEQ_SEQ(sc);
	//cosa add 04292008 to record the sequence number
	pcurrent_stats->Seq_Num = seq;
	//
	// Check whether we should take the previous packet into accounting
	//
	if(!pprevious_stats->bIsAMPDU)
	{
		// if previous packet is not aggregated packet
		bcheck = true;
	}else 
	{
//remve for that we don't use AMPDU to calculate PWDB,because the reported PWDB of some AP is fault.
#if 0
		// if previous packet is aggregated packet, and current packet
		//	(1) is not AMPDU
		//	(2) is the first packet of one AMPDU
		// that means the previous packet is the last one aggregated packet
		if( !pcurrent_stats->bIsAMPDU || pcurrent_stats->bFirstMPDU)
			bcheck = true;
#endif
	}
		
	if(slide_rssi_statistics++ >= PHY_RSSI_SLID_WIN_MAX)
	{
		slide_rssi_statistics = PHY_RSSI_SLID_WIN_MAX;
		last_rssi = priv->stats.slide_signal_strength[slide_rssi_index];
		priv->stats.slide_rssi_total -= last_rssi;
	}
	priv->stats.slide_rssi_total += pprevious_stats->SignalStrength;
	
	priv->stats.slide_signal_strength[slide_rssi_index++] = pprevious_stats->SignalStrength;
	if(slide_rssi_index >= PHY_RSSI_SLID_WIN_MAX)
		slide_rssi_index = 0;
	
	// <1> Showed on UI for user, in dbm
	tmp_val = priv->stats.slide_rssi_total/slide_rssi_statistics;
	priv->stats.signal_strength = rtl819x_translate_todbm((u8)tmp_val);
	pcurrent_stats->rssi = priv->stats.signal_strength;
	//
	// If the previous packet does not match the criteria, neglect it
	//
	if(!pprevious_stats->bPacketMatchBSSID)
	{
		if(!pprevious_stats->bToSelfBA)
			return;
	}
	
	if(!bcheck)
		return;

	rtl8190_process_cck_rxpathsel(priv,pprevious_stats);

	//
	// Check RSSI
	//
	priv->stats.num_process_phyinfo++;
	
	/* record the general signal strength to the sliding window. */
	

	// <2> Showed on UI for engineering
	// hardware does not provide rssi information for each rf path in CCK
	if(!pprevious_stats->bIsCCK && pprevious_stats->bPacketToSelf)
	{
		for (rfpath = RF90_PATH_A; rfpath < RF90_PATH_C; rfpath++)
		{
			if (!rtl8192_phy_CheckIsLegalRFPath(priv->ieee80211->dev, rfpath))
				continue;
			RT_TRACE(COMP_DBG,"Jacken -> pPreviousstats->RxMIMOSignalStrength[rfpath]  = %d \n" ,pprevious_stats->RxMIMOSignalStrength[rfpath] );		 
			//Fixed by Jacken 2008-03-20
			if(priv->stats.rx_rssi_percentage[rfpath] == 0)
			{
				priv->stats.rx_rssi_percentage[rfpath] = pprevious_stats->RxMIMOSignalStrength[rfpath];
				//DbgPrint("MIMO RSSI initialize \n");
			}	
			if(pprevious_stats->RxMIMOSignalStrength[rfpath]  > priv->stats.rx_rssi_percentage[rfpath])
			{
				priv->stats.rx_rssi_percentage[rfpath] = 
					( (priv->stats.rx_rssi_percentage[rfpath]*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxMIMOSignalStrength[rfpath])) /(Rx_Smooth_Factor);
				priv->stats.rx_rssi_percentage[rfpath] = priv->stats.rx_rssi_percentage[rfpath]  + 1;
			}
			else
			{
				priv->stats.rx_rssi_percentage[rfpath] = 
					( (priv->stats.rx_rssi_percentage[rfpath]*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxMIMOSignalStrength[rfpath])) /(Rx_Smooth_Factor);
			}	
			RT_TRACE(COMP_DBG,"Jacken -> priv->RxStats.RxRSSIPercentage[rfPath]  = %d \n" ,priv->stats.rx_rssi_percentage[rfpath] );
		}		
	}
	
	
	//
	// Check PWDB.
	//
	//cosa add for beacon rssi smoothing by average.
	if(pprevious_stats->bPacketBeacon)
	{
		/* record the beacon pwdb to the sliding window. */
		if(slide_beacon_adc_pwdb_statistics++ >= PHY_Beacon_RSSI_SLID_WIN_MAX)
		{
			slide_beacon_adc_pwdb_statistics = PHY_Beacon_RSSI_SLID_WIN_MAX;	
			last_beacon_adc_pwdb = priv->stats.Slide_Beacon_pwdb[slide_beacon_adc_pwdb_index];
			priv->stats.Slide_Beacon_Total -= last_beacon_adc_pwdb;
			//DbgPrint("slide_beacon_adc_pwdb_index = %d, last_beacon_adc_pwdb = %d, Adapter->RxStats.Slide_Beacon_Total = %d\n", 
			//	slide_beacon_adc_pwdb_index, last_beacon_adc_pwdb, Adapter->RxStats.Slide_Beacon_Total);
		}
		priv->stats.Slide_Beacon_Total += pprevious_stats->RxPWDBAll;
		priv->stats.Slide_Beacon_pwdb[slide_beacon_adc_pwdb_index] = pprevious_stats->RxPWDBAll;
		//DbgPrint("slide_beacon_adc_pwdb_index = %d, pPreviousRfd->Status.RxPWDBAll = %d\n", slide_beacon_adc_pwdb_index, pPreviousRfd->Status.RxPWDBAll);
		slide_beacon_adc_pwdb_index++;
		if(slide_beacon_adc_pwdb_index >= PHY_Beacon_RSSI_SLID_WIN_MAX)
			slide_beacon_adc_pwdb_index = 0;
		pprevious_stats->RxPWDBAll = priv->stats.Slide_Beacon_Total/slide_beacon_adc_pwdb_statistics;
		if(pprevious_stats->RxPWDBAll >= 3)
			pprevious_stats->RxPWDBAll -= 3;
	}

	RT_TRACE(COMP_RXDESC, "Smooth %s PWDB = %d\n", 
				pprevious_stats->bIsCCK? "CCK": "OFDM",
				pprevious_stats->RxPWDBAll);

	if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA)
	{
		if(priv->undecorated_smoothed_pwdb < 0)	// initialize
		{
			priv->undecorated_smoothed_pwdb = pprevious_stats->RxPWDBAll;
			//DbgPrint("First pwdb initialize \n");
		}
#if 1
		if(pprevious_stats->RxPWDBAll > (u32)priv->undecorated_smoothed_pwdb)
		{
			priv->undecorated_smoothed_pwdb =	
					( ((priv->undecorated_smoothed_pwdb)*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxPWDBAll)) /(Rx_Smooth_Factor);
			priv->undecorated_smoothed_pwdb = priv->undecorated_smoothed_pwdb + 1;
		}
		else
		{
			priv->undecorated_smoothed_pwdb =	
					( ((priv->undecorated_smoothed_pwdb)*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxPWDBAll)) /(Rx_Smooth_Factor);
		}
#else
		//Fixed by Jacken 2008-03-20
		if(pPreviousRfd->Status.RxPWDBAll > (u32)pHalData->UndecoratedSmoothedPWDB)
		{
			pHalData->UndecoratedSmoothedPWDB = 	
					( ((pHalData->UndecoratedSmoothedPWDB)* 5) + (pPreviousRfd->Status.RxPWDBAll)) / 6;
			pHalData->UndecoratedSmoothedPWDB = pHalData->UndecoratedSmoothedPWDB + 1;
		}
		else
		{
			pHalData->UndecoratedSmoothedPWDB = 	
					( ((pHalData->UndecoratedSmoothedPWDB)* 5) + (pPreviousRfd->Status.RxPWDBAll)) / 6;
		}		
#endif
		rtl819x_update_rxsignalstatistics8190pci(priv,pprevious_stats);
	}

	//
	// Check EVM
	//	
	/* record the general EVM to the sliding window. */
	if(pprevious_stats->SignalQuality == 0)
	{
	}
	else
	{
		if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA){
			if(slide_evm_statistics++ >= PHY_RSSI_SLID_WIN_MAX){
				slide_evm_statistics = PHY_RSSI_SLID_WIN_MAX;
				last_evm = priv->stats.slide_evm[slide_evm_index];
				priv->stats.slide_evm_total -= last_evm;
			}
	
			priv->stats.slide_evm_total += pprevious_stats->SignalQuality;
	
			priv->stats.slide_evm[slide_evm_index++] = pprevious_stats->SignalQuality;
			if(slide_evm_index >= PHY_RSSI_SLID_WIN_MAX)
				slide_evm_index = 0;
	
			// <1> Showed on UI for user, in percentage.
			tmp_val = priv->stats.slide_evm_total/slide_evm_statistics;
			priv->stats.signal_quality = tmp_val;
			//cosa add 10/11/2007, Showed on UI for user in Windows Vista, for Link quality.
			priv->stats.last_signal_strength_inpercent = tmp_val;
		}

		// <2> Showed on UI for engineering
		if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA)
		{
			for(nspatial_stream = 0; nspatial_stream<2 ; nspatial_stream++) // 2 spatial stream
			{
				if(pprevious_stats->RxMIMOSignalQuality[nspatial_stream] != -1)
				{
					if(priv->stats.rx_evm_percentage[nspatial_stream] == 0)	// initialize
					{
						priv->stats.rx_evm_percentage[nspatial_stream] = pprevious_stats->RxMIMOSignalQuality[nspatial_stream];
					}
					priv->stats.rx_evm_percentage[nspatial_stream] = 
						( (priv->stats.rx_evm_percentage[nspatial_stream]* (Rx_Smooth_Factor-1)) + 
						(pprevious_stats->RxMIMOSignalQuality[nspatial_stream]* 1)) / (Rx_Smooth_Factor);
				}
			}
		}
	}
	
}

/*-----------------------------------------------------------------------------
 * Function:	rtl819x_query_rxpwrpercentage()
 *
 * Overview:	
 *
 * Input:		char		antpower
 *
 * Output:		NONE
 *
 * Return:		0-100 percentage
 *
 * Revised History:
 *	When		Who 	Remark
 *	05/26/2008	amy 	Create Version 0 porting from windows code.  
 *
 *---------------------------------------------------------------------------*/
static u8 rtl819x_query_rxpwrpercentage(
	char		antpower
	)
{
	if ((antpower <= -100) || (antpower >= 20))
	{
		return	0;
	}
	else if (antpower >= 0)
	{
		return	100;
	}
	else
	{
		return	(100+antpower);
	}
	
}	/* QueryRxPwrPercentage */

static u8 
rtl819x_evm_dbtopercentage(
	char value
	)
{
	char ret_val;
	
	ret_val = value;
	
	if(ret_val >= 0)
		ret_val = 0;
	if(ret_val <= -33)
		ret_val = -33;
	ret_val = 0 - ret_val;
	ret_val*=3;
	if(ret_val == 99)
		ret_val = 100;
	return(ret_val);
}

//
//	Description:
//	We want good-looking for signal strength/quality
//	2007/7/19 01:09, by cosa.
//
long
rtl819x_signal_scale_mapping(
	long currsig
	)
{
	long retsig;

	// Step 1. Scale mapping.
	if(currsig >= 61 && currsig <= 100)
	{
		retsig = 90 + ((currsig - 60) / 4);
	}
	else if(currsig >= 41 && currsig <= 60)
	{
		retsig = 78 + ((currsig - 40) / 2);
	}
	else if(currsig >= 31 && currsig <= 40)
	{
		retsig = 66 + (currsig - 30);
	}
	else if(currsig >= 21 && currsig <= 30)
	{
		retsig = 54 + (currsig - 20);
	}
	else if(currsig >= 5 && currsig <= 20)
	{
		retsig = 42 + (((currsig - 5) * 2) / 3);
	}
	else if(currsig == 4)
	{
		retsig = 36;
	}
	else if(currsig == 3)
	{
		retsig = 27;
	}
	else if(currsig == 2)
	{
		retsig = 18;
	}
	else if(currsig == 1)
	{
		retsig = 9;
	}
	else
	{
		retsig = currsig;
	}
	
	return retsig;
}

static void rtl8192_query_rxphystatus(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pstats,
	prx_desc_819x_pci  pdesc,	
	prx_fwinfo_819x_pci   pdrvinfo,
	struct ieee80211_rx_stats * precord_stats,
	bool bpacket_match_bssid,
	bool bpacket_toself,
	bool bPacketBeacon,
	bool bToSelfBA
	)
{	
	//PRT_RFD_STATUS		pRtRfdStatus = &(pRfd->Status); 
	phy_sts_ofdm_819xpci_t* pofdm_buf;
	phy_sts_cck_819xpci_t	*	pcck_buf;
	phy_ofdm_rx_status_rxsc_sgien_exintfflag* prxsc;
	u8				*prxpkt;
	u8				i,max_spatial_stream, tmp_rxsnr, tmp_rxevm, rxsc_sgien_exflg;
	char				rx_pwr[4], rx_pwr_all=0;
	//long				rx_avg_pwr = 0;
	char				rx_snrX, rx_evmX;
	u8				evm, pwdb_all;
	u32 			RSSI, total_rssi=0;//, total_evm=0;
//	long				signal_strength_index = 0;
	u8				is_cck_rate=0;
	u8				rf_rx_num = 0;

	/* 2007/07/04 MH For OFDM RSSI. For high power or not. */
	static	u8		check_reg824 = 0;
	static	u32		reg824_bit9 = 0;

	priv->stats.numqry_phystatus++;
	
	is_cck_rate = rx_hal_is_cck_rate(pdrvinfo);

	// Record it for next packet processing
	memset(precord_stats, 0, sizeof(struct ieee80211_rx_stats));
	pstats->bPacketMatchBSSID = precord_stats->bPacketMatchBSSID = bpacket_match_bssid;
	pstats->bPacketToSelf = precord_stats->bPacketToSelf = bpacket_toself;
	pstats->bIsCCK = precord_stats->bIsCCK = is_cck_rate;//RX_HAL_IS_CCK_RATE(pDrvInfo);
	pstats->bPacketBeacon = precord_stats->bPacketBeacon = bPacketBeacon;
	pstats->bToSelfBA = precord_stats->bToSelfBA = bToSelfBA;
	/*2007.08.30 requested by SD3 Jerry */
	if(check_reg824 == 0)
	{	
		reg824_bit9 = rtl8192_QueryBBReg(priv->ieee80211->dev, rFPGA0_XA_HSSIParameter2, 0x200);
		check_reg824 = 1;	
	}


	prxpkt = (u8*)pdrvinfo; 
	
	/* Move pointer to the 16th bytes. Phy status start address. */ 
	prxpkt += sizeof(rx_fwinfo_819x_pci);	
	
	/* Initial the cck and ofdm buffer pointer */
	pcck_buf = (phy_sts_cck_819xpci_t *)prxpkt;
	pofdm_buf = (phy_sts_ofdm_819xpci_t *)prxpkt;			
			
	pstats->RxMIMOSignalQuality[0] = -1;
	pstats->RxMIMOSignalQuality[1] = -1;
	precord_stats->RxMIMOSignalQuality[0] = -1;
	precord_stats->RxMIMOSignalQuality[1] = -1;
			
	if(is_cck_rate)
	{
		// 
		// (1)Hardware does not provide RSSI for CCK
		//

		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		u8 report;//, cck_agc_rpt;
#ifdef RTL8190P
		u8 tmp_pwdb; 
		char cck_adc_pwdb[4];
#endif
		priv->stats.numqry_phystatusCCK++;

#ifdef RTL8190P	//Only 90P 2T4R need to check
		if(priv->rf_type == RF_2T4R && DM_RxPathSelTable.Enable && bpacket_match_bssid)
		{
			for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
			{
				tmp_pwdb = pcck_buf->adc_pwdb_X[i];
				cck_adc_pwdb[i] = (char)tmp_pwdb;
				cck_adc_pwdb[i] /= 2;
				pstats->cck_adc_pwdb[i] = precord_stats->cck_adc_pwdb[i] = cck_adc_pwdb[i];
				//DbgPrint("RF-%d tmp_pwdb = 0x%x, cck_adc_pwdb = %d", i, tmp_pwdb, cck_adc_pwdb[i]);
			}
		}
#endif

		if(!reg824_bit9)
		{
			report = pcck_buf->cck_agc_rpt & 0xc0;
			report = report>>6;
			switch(report)
			{
				//Fixed by Jacken from Bryant 2008-03-20
				//Original value is -38 , -26 , -14 , -2
				//Fixed value is -35 , -23 , -11 , 6
				case 0x3:
					rx_pwr_all = -35 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x2:
					rx_pwr_all = -23 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x1:
					rx_pwr_all = -11 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x0:
					rx_pwr_all = 8 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
			}
		}
		else
		{
			report = pcck_buf->cck_agc_rpt & 0x60;
			report = report>>5;
			switch(report)
			{
				case 0x3:
					rx_pwr_all = -35 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x2:
					rx_pwr_all = -23 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x1:
					rx_pwr_all = -11 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x0:
					rx_pwr_all = -8 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
			}
		}

		pwdb_all = rtl819x_query_rxpwrpercentage(rx_pwr_all);
		pstats->RxPWDBAll = precord_stats->RxPWDBAll = pwdb_all;
		pstats->RecvSignalPower = rx_pwr_all;

		//
		// (3) Get Signal Quality (EVM)
		//
		//if(bpacket_match_bssid)
		{
			u8	sq;

			if(pstats->RxPWDBAll > 40)
			{
				sq = 100;
			}else
			{
				sq = pcck_buf->sq_rpt;
				
				if(pcck_buf->sq_rpt > 64)
					sq = 0;
				else if (pcck_buf->sq_rpt < 20)
					sq = 100;
				else
					sq = ((64-sq) * 100) / 44;
			}
			pstats->SignalQuality = precord_stats->SignalQuality = sq;
			pstats->RxMIMOSignalQuality[0] = precord_stats->RxMIMOSignalQuality[0] = sq;
			pstats->RxMIMOSignalQuality[1] = precord_stats->RxMIMOSignalQuality[1] = -1;
		}
	}
	else
	{
		priv->stats.numqry_phystatusHT++;
		// 
		// (1)Get RSSI for HT rate
		//
		for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
		{
			// 2008/01/30 MH we will judge RF RX path now.
			if (priv->brfpath_rxenable[i])
				rf_rx_num++;
			//else
				//continue;

			//Fixed by Jacken from Bryant 2008-03-20
			//Original value is 106
			#ifdef RTL8190P	   //Modify by Jacken 2008/03/31
			rx_pwr[i] = ((pofdm_buf->trsw_gain_X[i]&0x3F)*2) - 106;
			#else
				#ifdef RTL8192E
				rx_pwr[i] = ((pofdm_buf->trsw_gain_X[i]&0x3F)*2) - 110;
				#endif
			#endif

			//Get Rx snr value in DB
			tmp_rxsnr = pofdm_buf->rxsnr_X[i];
			rx_snrX = (char)(tmp_rxsnr);
			rx_snrX /= 2;
			priv->stats.rxSNRdB[i] = (long)rx_snrX;
			
			/* Translate DBM to percentage. */
			RSSI = rtl819x_query_rxpwrpercentage(rx_pwr[i]);	
			if (priv->brfpath_rxenable[i])
				total_rssi += RSSI;

			/* Record Signal Strength for next packet */
			if(bpacket_match_bssid)
			{
				pstats->RxMIMOSignalStrength[i] =(u8) RSSI;
				precord_stats->RxMIMOSignalStrength[i] =(u8) RSSI;
			}
		}
		
		
		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		//Fixed by Jacken from Bryant 2008-03-20
		//Original value is 106
		rx_pwr_all = (((pofdm_buf->pwdb_all ) >> 1 )& 0x7f) -106;
		pwdb_all = rtl819x_query_rxpwrpercentage(rx_pwr_all);	

		pstats->RxPWDBAll = precord_stats->RxPWDBAll = pwdb_all;
		pstats->RxPower = precord_stats->RxPower =	rx_pwr_all;
		pstats->RecvSignalPower = rx_pwr_all;
		//
		// (3)EVM of HT rate
		//
		if(pdrvinfo->RxHT && pdrvinfo->RxRate>=DESC90_RATEMCS8 &&
			pdrvinfo->RxRate<=DESC90_RATEMCS15)
			max_spatial_stream = 2; //both spatial stream make sense
		else
			max_spatial_stream = 1; //only spatial stream 1 makes sense

		for(i=0; i<max_spatial_stream; i++)
		{
			tmp_rxevm = pofdm_buf->rxevm_X[i];
			rx_evmX = (char)(tmp_rxevm);
			
			// Do not use shift operation like "rx_evmX >>= 1" because the compilor of free build environment
			// fill most significant bit to "zero" when doing shifting operation which may change a negative 
			// value to positive one, then the dbm value (which is supposed to be negative)  is not correct anymore.
			rx_evmX /= 2;	//dbm

			evm = rtl819x_evm_dbtopercentage(rx_evmX);
#if 0			
			EVM = SignalScaleMapping(EVM);//make it good looking, from 0~100
#endif
			//if(bpacket_match_bssid)
			{
				if(i==0) // Fill value in RFD, Get the first spatial stream only
					pstats->SignalQuality = precord_stats->SignalQuality = (u8)(evm & 0xff);
				pstats->RxMIMOSignalQuality[i] = precord_stats->RxMIMOSignalQuality[i] = (u8)(evm & 0xff);
			}
		}

		
		/* record rx statistics for debug */
		rxsc_sgien_exflg = pofdm_buf->rxsc_sgien_exflg;
		prxsc = (phy_ofdm_rx_status_rxsc_sgien_exintfflag *)&rxsc_sgien_exflg;
		if(pdrvinfo->BW)	//40M channel
			priv->stats.received_bwtype[1+prxsc->rxsc]++;
		else				//20M channel
			priv->stats.received_bwtype[0]++;
	}

	//UI BSS List signal strength(in percentage), make it good looking, from 0~100.
	//It is assigned to the BSS List in GetValueFromBeaconOrProbeRsp().
	if(is_cck_rate)
	{
		pstats->SignalStrength = precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping((long)pwdb_all));//PWDB_ALL;
		
	}
	else
	{
		//pRfd->Status.SignalStrength = pRecordRfd->Status.SignalStrength = (u1Byte)(SignalScaleMapping(total_rssi/=RF90_PATH_MAX));//(u1Byte)(total_rssi/=RF90_PATH_MAX);
		// We can judge RX path number now.
		if (rf_rx_num != 0)
			pstats->SignalStrength = precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping((long)(total_rssi/=rf_rx_num))); 	
	}
}	/* QueryRxPhyStatus8190Pci */

void
rtl8192_record_rxdesc_forlateruse(
	struct ieee80211_rx_stats * psrc_stats,
	struct ieee80211_rx_stats * ptarget_stats
)
{
	ptarget_stats->bIsAMPDU = psrc_stats->bIsAMPDU;
	ptarget_stats->bFirstMPDU = psrc_stats->bFirstMPDU;
	//ptarget_stats->Seq_Num = psrc_stats->Seq_Num;
}



void TranslateRxSignalStuff819xpci(struct net_device *dev, struct ieee80211_rx_stats * pstats, prx_desc_819x_pci pdesc,	prx_fwinfo_819x_pci pdrvinfo)
{
	// TODO: We must only check packet for current MAC address. Not finish
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	bool bpacket_match_bssid, bpacket_toself;
	bool bPacketBeacon=false, bToSelfBA=false;
	static struct ieee80211_rx_stats  previous_stats;
	struct ieee80211_hdr_3addr *hdr;
	u16 fc,type;
		
	// Get Signal Quality for only RX data queue (but not command queue)
		
	u8* tmp_buf;
	//u16 tmp_buf_len = 0;
	u8	*praddr;
	
	/* Get MAC frame start address. */		
	tmp_buf = ((u8*)priv->rxbuffer->buf) + pstats->RxDrvInfoSize + pstats->RxBufShift;// + get_rxpacket_shiftbytes_819xusb(pstats);

	hdr = (struct ieee80211_hdr_3addr *)tmp_buf;
	fc = le16_to_cpu(hdr->frame_ctl);
	type = WLAN_FC_GET_TYPE(fc);	
	praddr = hdr->addr1;

	/* Check if the received packet is acceptabe. */
	bpacket_match_bssid = ((IEEE80211_FTYPE_CTL != type) &&
											(eqMacAddr(priv->ieee80211->current_network.bssid,	(fc & IEEE80211_FCTL_TODS)? hdr->addr1 : (fc & IEEE80211_FCTL_FROMDS )? hdr->addr2 : hdr->addr3))
												 && (!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV));
	bpacket_toself =  bpacket_match_bssid & (eqMacAddr(praddr, priv->ieee80211->dev->dev_addr));
#if 1//cosa
		if(WLAN_FC_GET_FRAMETYPE(fc)== IEEE80211_STYPE_BEACON)
		{
			bPacketBeacon = true;
			//DbgPrint("Beacon 2, MatchBSSID = %d, ToSelf = %d \n", bPacketMatchBSSID, bPacketToSelf);
		}
		if(WLAN_FC_GET_FRAMETYPE(fc) == IEEE80211_STYPE_BLOCKACK)
		{
			if((eqMacAddr(praddr,dev->dev_addr)))
				bToSelfBA = true;
				//DbgPrint("BlockAck, MatchBSSID = %d, ToSelf = %d \n", bPacketMatchBSSID, bPacketToSelf);
		}

#endif	
	
	if(bpacket_match_bssid)
	{
		priv->stats.numpacket_matchbssid++;
	}
	if(bpacket_toself){
		priv->stats.numpacket_toself++;
	}
	//
	// Process PHY information for previous packet (RSSI/PWDB/EVM)
	//
	// Because phy information is contained in the last packet of AMPDU only, so driver 
	// should process phy information of previous packet	
	rtl8192_process_phyinfo(priv, tmp_buf,&previous_stats, pstats);
	rtl8192_query_rxphystatus(priv, pstats, pdesc, pdrvinfo, &previous_stats, bpacket_match_bssid,
		bpacket_toself ,bPacketBeacon, bToSelfBA);
	rtl8192_record_rxdesc_forlateruse(pstats, &previous_stats);
		
}


void rtl8192_tx_resume(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	struct sk_buff *skb;
	int queue_index;

	for(queue_index = BK_QUEUE; queue_index < TXCMD_QUEUE;queue_index++) {
		while((!skb_queue_empty(&ieee->skb_waitQ[queue_index]))&&
				(priv->ieee80211->check_nic_enough_desc(dev,queue_index) > 0)) {
			/* 1. dequeue the packet from the wait queue */	
			skb = skb_dequeue(&ieee->skb_waitQ[queue_index]);
			/* 2. tx the packet directly */	
			ieee->softmac_data_hard_start_xmit(skb,dev,0/* rate useless now*/);
			if(queue_index!=MGNT_QUEUE) {
				ieee->stats.tx_packets++;
				ieee->stats.tx_bytes += skb->len;
			}
			printk("wait queue process all the time!\n");
		}
	}
}

void rtl8192_irq_tx_tasklet(struct r8192_priv *priv)
{
       rtl8192_tx_resume(priv->ieee80211->dev);
}

/**
* Function:	UpdateReceivedRateHistogramStatistics
* Overview:	Recored down the received data rate
* 
* Input:		
* 	PADAPTER	Adapter
*	PRT_RFD		pRfd,
* 			
* Output:		
*	PRT_TCB		Adapter
*				(Adapter->RxStats.ReceivedRateHistogram[] is updated)
* Return:     	
*		None
*/
void UpdateReceivedRateHistogramStatistics8190(
	struct net_device *dev,
	struct ieee80211_rx_stats* pstats
	)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u32 rcvType=1;   //0: Total, 1:OK, 2:CRC, 3:ICV
	u32 rateIndex;
	u32 preamble_guardinterval;  //1: short preamble/GI, 0: long preamble/GI
	    
	/* 2007/03/09 MH We will not update rate of packet from rx cmd queue. */
	#if 0
	if (pRfd->queue_id == CMPK_RX_QUEUE_ID)
		return;
	#endif
	if(pstats->bCRC)
		rcvType = 2;
	else if(pstats->bICV)
		rcvType = 3;
	    
	if(pstats->bShortPreamble)
		preamble_guardinterval = 1;// short
	else
		preamble_guardinterval = 0;// long

	switch(pstats->rate)
	{
		//
		// CCK rate
		//
		case MGN_1M:    rateIndex = 0;  break;
	    	case MGN_2M:    rateIndex = 1;  break;
	    	case MGN_5_5M:  rateIndex = 2;  break;
	    	case MGN_11M:   rateIndex = 3;  break;
		//
		// Legacy OFDM rate
		//
	    	case MGN_6M:    rateIndex = 4;  break;
	    	case MGN_9M:    rateIndex = 5;  break;
	    	case MGN_12M:   rateIndex = 6;  break;
	    	case MGN_18M:   rateIndex = 7;  break;
	    	case MGN_24M:   rateIndex = 8;  break;
	    	case MGN_36M:   rateIndex = 9;  break;
	    	case MGN_48M:   rateIndex = 10; break;
	    	case MGN_54M:   rateIndex = 11; break;
		//
		// 11n High throughput rate
		//
	    	case MGN_MCS0:  rateIndex = 12; break;
	    	case MGN_MCS1:  rateIndex = 13; break;
	    	case MGN_MCS2:  rateIndex = 14; break;
	    	case MGN_MCS3:  rateIndex = 15; break;
	    	case MGN_MCS4:  rateIndex = 16; break;
	    	case MGN_MCS5:  rateIndex = 17; break;
	    	case MGN_MCS6:  rateIndex = 18; break;
	    	case MGN_MCS7:  rateIndex = 19; break;
	    	case MGN_MCS8:  rateIndex = 20; break;
	    	case MGN_MCS9:  rateIndex = 21; break;
	    	case MGN_MCS10: rateIndex = 22; break;
	    	case MGN_MCS11: rateIndex = 23; break;
	    	case MGN_MCS12: rateIndex = 24; break;
	    	case MGN_MCS13: rateIndex = 25; break;
	    	case MGN_MCS14: rateIndex = 26; break;
	    	case MGN_MCS15: rateIndex = 27; break;
		default:        rateIndex = 28; break;
	}
	priv->stats.received_preamble_GI[preamble_guardinterval][rateIndex]++;
	priv->stats.received_rate_histogram[0][rateIndex]++; //total
	priv->stats.received_rate_histogram[rcvType][rateIndex]++;
}

void rtl8192_rx(struct net_device *dev)

{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct sk_buff *tmp_skb;
	u8* receivedata;
	//struct sk_buff *skb;
	short first,last;
	u32 len;
	int lastlen;
	//unsigned char quality, signal;
	//u8 rate;
	//u32 *prism_hdr;
	u32 *tmp,*tmp2;
	u8 rx_desc_size;
	u8 padding = 0;
	prx_desc_819x_pci pdesc = NULL;
	prx_fwinfo_819x_pci pDrvInfo = NULL;
	struct ieee80211_hdr_1addr *ieee80211_hdr = NULL;
	bool unicast_packet = false;
	//u32 count=0;	
	struct ieee80211_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
	//	.mac_time = jiffies,
		.freq = IEEE80211_24GHZ_BAND,
	};

	stats.nic_type = NIC_8192E;
	rx_desc_size = 4;

	//if (!priv->rxbuffer) DMESG ("EE: NIC RX ack, but RX queue corrupted!");
	//else {
	
	if ((*(priv->rxringtail)) & (1<<31)) {
		
		/* we have got an RX int, but the descriptor
		 * we are pointing is empty*/
			
	//	priv->stats.rxnodata++;
		priv->ieee80211->stats.rx_errors++;
		
	/*	if (! *(priv->rxring) & (1<<31)) {
		
			priv->stats.rxreset++;
			priv->rxringtail=priv->rxring;
			priv->rxbuffer=priv->rxbufferhead;
		
		}else{*/
		
		#if 0
		/* Maybe it is possible that the NIC has skipped some descriptors or
		 * it has reset its internal pointer to the beginning of the ring
		 * we search for the first filled descriptor in the ring, or we break
		 * putting again the pointer in the old location if we do not found any.
		 * This is quite dangerous, what does happen if the nic writes
		 * two descriptor (say A and B) when we have just checked the descriptor
		 * A and we are going to check the descriptor B..This might happen if the
		 * interrupt was dummy, there was not really filled descriptors and
		 * the NIC didn't lose pointer 
		 */
		
		//priv->stats.rxwrkaround++; 
		
		tmp = priv->rxringtail; 
		while (*(priv->rxringtail) & (1<<31)){
			
			priv->rxringtail+=4;
			
			if(priv->rxringtail >= 
				(priv->rxring)+(priv->rxringcount )*4)
				priv->rxringtail=priv->rxring;
			
			priv->rxbuffer=(priv->rxbuffer->next);
			
			if(priv->rxringtail == tmp ){
				//DMESG("EE: Could not find RX pointer");
				priv->stats.rxnopointer++;
				break;
			}
		}
		#else
		
		tmp2 = NULL;
		tmp = priv->rxringtail;
		do{
			if(tmp == priv->rxring)
				//tmp  = priv->rxring + (priv->rxringcount )*rx_desc_size; xiong-2006-11-15
				tmp  = priv->rxring + (priv->rxringcount - 1)*rx_desc_size;
			else
				tmp -= rx_desc_size;

			if(! (*tmp & (1<<31)))
				tmp2 = tmp; 	
		}while(tmp != priv->rxring);
		
		if(tmp2) priv->rxringtail = tmp2;
		#endif
		//}
	}
		
	/* while there are filled descriptors */
	while(!(*(priv->rxringtail) & (1<<31))){
		pdesc = (rx_desc_819x_pci *)priv->rxringtail;
		stats.Length = pdesc->Length;
		stats.RxDrvInfoSize = pdesc->RxDrvInfoSize;
		stats.RxBufShift = ((pdesc->Shift)&0x03);
		stats.bICV = pdesc->ICV;
		stats.bCRC = pdesc->CRC32;
		stats.bHwError = pdesc->CRC32 | pdesc->ICV;
		stats.Decrypted = !pdesc->SWDec;

		if(stats.Length < 24)
			stats.bHwError |= 1;
		
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		pci_dma_sync_single_for_cpu(priv->pdev,
#else		
		pci_dma_sync_single(priv->pdev,
#endif
					priv->rxbuffer->dma,
					priv->rxbuffersize * \
					sizeof(u8),
					PCI_DMA_FROMDEVICE);
		
		pDrvInfo = (rx_fwinfo_819x_pci *)((u8*)(priv->rxbuffer->buf) + stats.RxBufShift);
		if(!stats.bHwError)
			stats.rate = HwRateToMRate90((bool)pDrvInfo->RxHT, (u8)pDrvInfo->RxRate);

		stats.bShortPreamble = pDrvInfo->SPLCP;

		// 	TODO: it is debug only. It should be disabled in released driver. 2007.1.11 by Emily
		UpdateReceivedRateHistogramStatistics8190(dev, &stats);
	
		stats.bIsAMPDU = (pDrvInfo->PartAggr==1);
		stats.bFirstMPDU = (pDrvInfo->PartAggr==1) && (pDrvInfo->FirstAGGR==1);

		//UpdateRxAMPDUHistogramStatistics8190(Adapter, pRfd);
		stats.TimeStampLow = pDrvInfo->TSFL;
		stats.TimeStampHigh = read_nic_dword(dev, TSFR+4);
		UpdateRxPktTimeStamp8190(dev, &stats);

		//
		// Get Total offset of MPDU Frame Body 
		//
		if((stats.RxBufShift + stats.RxDrvInfoSize) > 0)
			stats.bShift = 1;

		stats.RxIs40MHzPacket = pDrvInfo->BW;

		TranslateRxSignalStuff819xpci(dev, &stats, pdesc, pDrvInfo);
		
		//
		// Rx A-MPDU
		//
		if(pDrvInfo->FirstAGGR==1 || pDrvInfo->PartAggr == 1)
			RT_TRACE(COMP_RXDESC, "pDrvInfo->FirstAGGR = %d, pDrvInfo->PartAggr = %d\n", pDrvInfo->FirstAGGR, pDrvInfo->PartAggr);


		//
		// Get signal quality for this packet, and record aggregation information for next packet
		//
		//TranslateRxSignalStuff8190(Adapter, pRfd, pDesc, pDrvInfo);
		if(!stats.bHwError)
		{
			stats.packetlength = stats.Length-4;
			stats.fraglength = stats.packetlength;
			stats.fragoffset = 0;
			stats.ntotalfrag = 1;
			
		}
		else
		{
			stats.bShift = false;
			goto drop;
		}

		receivedata = (u8 *)(priv->rxbuffer->buf) + stats.RxDrvInfoSize + stats.RxBufShift;
		//CountRxErrStatistics(Adapter, pRfd);
		{
			ieee80211_hdr = (struct ieee80211_hdr_1addr *)receivedata;
			unicast_packet = false;
			if(is_broadcast_ether_addr(ieee80211_hdr->addr1)) {
			//TODO
			}else if(is_multicast_ether_addr(ieee80211_hdr->addr1)){
			//TODO
			}else {
			/* unicast packet */
				unicast_packet = true;
			}
		}
		first = pdesc->FirstSeg? 1:0;
		if(first) priv->rx_prevlen=0;
			
		last = pdesc->LastSeg? 1:0;
		if(last){
			lastlen=pdesc->Length - 4;
				  
			/* if the last descriptor (that should
			 * tell us the total packet len) tell
			 * us something less than the descriptors
			 * len we had until now, then there is some
			 * problem..
			 * workaround to prevent kernel panic
			 */
			if(lastlen < priv->rx_prevlen)
				len=0;
			else
				len=lastlen-priv->rx_prevlen; 

			if(pdesc->CRC32) {
//lastlen=((*priv->rxringtail) &0xfff);
				if (pdesc->Length <500)
					priv->stats.rxcrcerrmin++;
				else if (pdesc->Length >1000)
					priv->stats.rxcrcerrmax++;
				else 
					priv->stats.rxcrcerrmid++;
	
			}
				
		}else{
			len = priv->rxbuffersize;
		}
		
		
		priv->rx_prevlen+=len;
				
	//	if(priv->rx_prevlen > MAX_FRAG_THRESHOLD + 100){
		if(priv->rx_prevlen > 9100){ //to avoid misdiscard AMSDU packets
			/* HW is probably passing several buggy frames
			* without FD or LD flag set.
			* Throw this garbage away to prevent skb
			* memory exausting
			*/
			if(!priv->rx_skb_complete)	
				dev_kfree_skb_any(priv->rx_skb);
			priv->rx_skb_complete = 1;	 
		}
				
#ifdef DEBUG_RX_FRAG
		DMESG("Iteration.. len %x",len);
		if(first) DMESG ("First descriptor");
		if(last) DMESG("Last descriptor");
				
#endif
#ifdef DEBUG_RX_VERBOSE
		print_buffer( priv->rxbuffer->buf, len);
#endif


#ifndef DUMMY_RX	
		if(first){
			if(!priv->rx_skb_complete){
				/* seems that HW sometimes fails to reiceve and 
				   doesn't provide the last descriptor */
#ifdef DEBUG_RX_SKB
				DMESG("going to free incomplete skb");
#endif
				dev_kfree_skb_any(priv->rx_skb);
				//priv->stats.rxnolast++;
#ifdef DEBUG_RX_SKB
				DMESG("free incomplete skb OK");
#endif			
			}
			/* support for prism header has been originally added by Christian */
#if 0
			if(priv->prism_hdr && priv->ieee80211->iw_mode == IW_MODE_MONITOR){
				
				priv->rx_skb = dev_alloc_skb(len+2+PRISM_HDR_SIZE);
				if(! priv->rx_skb) goto drop;
					
				prism_hdr = (u32*) skb_put(priv->rx_skb,PRISM_HDR_SIZE);
				prism_hdr[0]=htonl(0x80211001); 	   //version
				prism_hdr[1]=htonl(0x40);			   //length
				prism_hdr[2]=htonl(stats.mac_time[1]);	  //mactime (HIGH)
				prism_hdr[3]=htonl(stats.mac_time[0]);	  //mactime (LOW)
				rdtsc(prism_hdr[5], prism_hdr[4]);		   //hostime (LOW+HIGH)
				prism_hdr[4]=htonl(prism_hdr[4]);		   //Byte-Order aendern
				prism_hdr[5]=htonl(prism_hdr[5]);		   //Byte-Order aendern
				prism_hdr[6]=0x00;					   //phytype
				prism_hdr[7]=htonl(priv->chan); 	   //channel
				prism_hdr[8]=htonl(stats.rate); 	   //datarate
				prism_hdr[9]=0x00;					   //antenna
				prism_hdr[10]=0x00; 				   //priority
				prism_hdr[11]=0x00; 				   //ssi_type
				prism_hdr[12]=htonl(stats.signal);	   //ssi_signal
				prism_hdr[13]=htonl(stats.noise);	   //ssi_noise
				prism_hdr[14]=0x00; 				   //preamble
				prism_hdr[15]=0x00; 				   //encoding

			}
			else
#endif
			{
				priv->rx_skb = dev_alloc_skb(len+2);//amy ask why?
				if( !priv->rx_skb) goto drop;
#ifdef DEBUG_RX_SKB
				DMESG("Alloc initial skb %x",len+2);
#endif
			}
				
			priv->rx_skb_complete=0;	
			priv->rx_skb->dev=dev;
		}else{
			/* if we are here we should  have already RXed 
			* the first frame.
			* If we get here and the skb is not allocated then
			* we have just throw out garbage (skb not allocated)
			* and we are still rxing garbage....
			*/ 
			if(!priv->rx_skb_complete){
					
				tmp_skb= dev_alloc_skb(priv->rx_skb->len +len+2);
					
				if(!tmp_skb) goto drop;
					
				tmp_skb->dev=dev;
#ifdef DEBUG_RX_SKB
				DMESG("Realloc skb %x",len+2);
#endif

#ifdef DEBUG_RX_SKB
				DMESG("going copy prev frag %x",priv->rx_skb->len);
#endif
				memcpy(skb_put(tmp_skb,priv->rx_skb->len),
					priv->rx_skb->data,
					priv->rx_skb->len);
#ifdef DEBUG_RX_SKB
				DMESG("skb copy prev frag complete");
#endif

				dev_kfree_skb_any(priv->rx_skb);
#ifdef DEBUG_RX_SKB
				DMESG("prev skb free ok");
#endif
				 
				priv->rx_skb=tmp_skb;
			}
		}
#ifdef DEBUG_RX_SKB
		DMESG("going to copy current payload %x",len);
#endif
		if(!priv->rx_skb_complete) {
			if(padding) { 
				memcpy(skb_put(priv->rx_skb,len),
					(((unsigned char *)receivedata) + 2),len);
			} else {
				memcpy(skb_put(priv->rx_skb,len),
					receivedata,len);
			}
	
		}
#ifdef DEBUG_RX_SKB
		DMESG("current fragment skb copy complete");
#endif

		if(last && !priv->rx_skb_complete){

#ifdef DEBUG_RX_SKB
			DMESG("Got last fragment");
#endif

	//		if(priv->rx_skb->len > 4)				
		//		skb_trim(priv->rx_skb,priv->rx_skb->len-4);
#ifdef DEBUG_RX_SKB
			DMESG("yanked out crc, passing to the upper layer");
#endif

#ifndef RX_DONT_PASS_UL
			if(!ieee80211_rx(priv->ieee80211, 
					 priv->rx_skb, &stats)){
#ifdef DEBUG_RX
				DMESGW("Packet not consumed");
#endif
#endif // RX_DONT_PASS_UL
				
				dev_kfree_skb_any(priv->rx_skb);
#ifndef RX_DONT_PASS_UL
			}
#endif
			else{
				priv->stats.rxok++;
				if(unicast_packet) {
					priv->stats.rxbytesunicast += priv->rx_skb->len;
				}
			}
#ifdef DEBUG_RX
			else{
					DMESG("Rcv frag");
			}
#endif
			priv->rx_skb_complete=1;
		}
			
#endif //DUMMY_RX

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		pci_dma_sync_single_for_device(priv->pdev,
					priv->rxbuffer->dma,
					priv->rxbuffersize * \
					sizeof(u8),
					PCI_DMA_FROMDEVICE);
#endif		
		
drop: // this is used when we have not enought mem

		/* restore the descriptor */		
		*(priv->rxringtail+3)=priv->rxbuffer->dma;
		*(priv->rxringtail)=*(priv->rxringtail) &~ 0x3fff;
		*(priv->rxringtail)=
			*(priv->rxringtail) | priv->rxbuffersize;
			
		*(priv->rxringtail)=
			*(priv->rxringtail) | (1<<31); 
			//^empty descriptor

			//wmb();	
			
#ifdef DEBUG_RX
		DMESG("Current descriptor: %x",(u32)priv->rxringtail);
#endif
		//unsigned long flags;
		//spin_lock_irqsave(&priv->irq_lock,flags);

		priv->rxringtail+=rx_desc_size;
		if(priv->rxringtail >= 
		   (priv->rxring)+(priv->rxringcount )*rx_desc_size)
			priv->rxringtail=priv->rxring;

		//spin_unlock_irqrestore(&priv->irq_lock,flags);	
			

		priv->rxbuffer=(priv->rxbuffer->next);

	}
	
	
	
//	if(get_curr_tx_free_desc(dev,priority))
//	ieee80211_sta_ps_sleep(priv->ieee80211, &tmp, &tmp2);
	
	
}

void rtl8192_irq_rx_tasklet(struct r8192_priv *priv)
{
    rtl8192_rx(priv->ieee80211->dev);   
    write_nic_dword(priv->ieee80211->dev, INTA_MASK,read_nic_dword(priv->ieee80211->dev, INTA_MASK) | IMR_RDU); // unmask RDU
}


/****************************************************************************
     ---------------------------- USB_STUFF---------------------------
*****************************************************************************/

static int __devinit rtl8192_pci_probe(struct pci_dev *pdev,
			 const struct pci_device_id *id)
{
	unsigned long ioaddr = 0;
	struct net_device *dev = NULL;
	struct r8192_priv *priv= NULL;
	u8 unit = 0;
	
#ifdef CONFIG_RTL8192_IO_MAP
	unsigned long pio_start, pio_len, pio_flags;
#else
	unsigned long pmem_start, pmem_len, pmem_flags;
#endif //end #ifdef RTL_IO_MAP

	RT_TRACE(COMP_INIT,"Configuring chip resources");
	
	if( pci_enable_device (pdev) ){
		RT_TRACE(COMP_ERR,"Failed to enable PCI device");
		return -EIO;
	}

	pci_set_master(pdev);
	//pci_set_wmi(pdev);
	pci_set_dma_mask(pdev, 0xffffff00ULL);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	pci_set_consistent_dma_mask(pdev,0xffffff00ULL);
#endif	
	dev = alloc_ieee80211(sizeof(struct r8192_priv));
	if (!dev)
		return -ENOMEM;
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	SET_MODULE_OWNER(dev);
#endif

	pci_set_drvdata(pdev, dev);	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	SET_NETDEV_DEV(dev, &pdev->dev);
#endif
	priv = ieee80211_priv(dev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	priv->ieee80211 = netdev_priv(dev);
#else
	priv->ieee80211 = (struct ieee80211_device *)dev->priv;
#endif
	priv->pdev=pdev;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	if((pdev->subsystem_vendor == PCI_VENDOR_ID_DLINK)&&(pdev->subsystem_device == 0x3304)){
		priv->ieee80211->bSupportRemoteWakeUp = 1;
	} else 
#endif
	{
		priv->ieee80211->bSupportRemoteWakeUp = 0;
	}

#ifdef CONFIG_RTL8192_IO_MAP
	
	pio_start = (unsigned long)pci_resource_start (pdev, 0);
	pio_len = (unsigned long)pci_resource_len (pdev, 0);
	pio_flags = (unsigned long)pci_resource_flags (pdev, 0);
 	
      	if (!(pio_flags & IORESOURCE_IO)) {
		RT_TRACE(COMP_ERR,"region #0 not a PIO resource, aborting");
		goto fail;
	}
	
	//DMESG("IO space @ 0x%08lx", pio_start );
	if( ! request_region( pio_start, pio_len, RTL819xE_MODULE_NAME ) ){
		RT_TRACE(COMP_ERR,"request_region failed!");
		goto fail;
	}
	
	ioaddr = pio_start;
	dev->base_addr = ioaddr; // device I/O address
	
#else
	
	pmem_start = pci_resource_start(pdev, 1);
	pmem_len = pci_resource_len(pdev, 1);
	pmem_flags = pci_resource_flags (pdev, 1);
	
	if (!(pmem_flags & IORESOURCE_MEM)) {
		RT_TRACE(COMP_ERR,"region #1 not a MMIO resource, aborting");
		goto fail;
	}
	
	//DMESG("Memory mapped space @ 0x%08lx ", pmem_start);
	if( ! request_mem_region(pmem_start, pmem_len, RTL819xE_MODULE_NAME)) {
		RT_TRACE(COMP_ERR,"request_mem_region failed!");
		goto fail;
	}
	
	
	ioaddr = (unsigned long)ioremap_nocache( pmem_start, pmem_len);	
	if( ioaddr == (unsigned long)NULL ){
		RT_TRACE(COMP_ERR,"ioremap failed!");
	       // release_mem_region( pmem_start, pmem_len );
		goto fail1;
	}
	
	dev->mem_start = ioaddr; // shared mem start
	dev->mem_end = ioaddr + pci_resource_len(pdev, 0); // shared mem end
	
#endif //end #ifdef RTL_IO_MAP

        /* We disable the RETRY_TIMEOUT register (0x41) to keep
         * PCI Tx retries from interfering with C3 CPU state */
         pci_write_config_byte(pdev, 0x41, 0x00);


	pci_read_config_byte(pdev, 0x05, &unit);
	pci_write_config_byte(pdev, 0x05, unit & (~0x04));

	dev->irq = pdev->irq;
	priv->irq = 0;
	
	dev->open = rtl8192_open;
	dev->stop = rtl8192_close;
	//dev->hard_start_xmit = rtl8192_8023_hard_start_xmit;
	dev->tx_timeout = tx_timeout;
	dev->wireless_handlers = &r8192_wx_handlers_def;
	dev->do_ioctl = rtl8192_ioctl;
	dev->set_multicast_list = r8192_set_multicast;
	dev->set_mac_address = r8192_set_mac_adr;
	
         //DMESG("Oops: i'm coming\n");
#if WIRELESS_EXT >= 12
#if WIRELESS_EXT < 17
        dev->get_wireless_stats = r8192_get_wireless_stats;
#endif
        dev->wireless_handlers = (struct iw_handler_def *) &r8192_wx_handlers_def;
#endif
       //dev->get_wireless_stats = r8192_get_wireless_stats;
	dev->type=ARPHRD_ETHER;

	dev->watchdog_timeo = HZ*3;	//modified by john, 0805

	if (dev_alloc_name(dev, ifname) < 0){
                RT_TRACE(COMP_INIT, "Oops: devname already taken! Trying wlan%%d...\n");
		ifname = "wlan%d";
		dev_alloc_name(dev, ifname);
        }
	
	RT_TRACE(COMP_INIT, "Driver probe completed1\n");
	if(rtl8192_init(dev)!=0){ 
		RT_TRACE(COMP_ERR, "Initialization failed");
		goto fail;
	}
	
	netif_carrier_off(dev);
	netif_stop_queue(dev);
	
	register_netdev(dev);
	RT_TRACE(COMP_INIT, "dev name=======> %s\n",dev->name);
	rtl8192_proc_init_one(dev);
	
	
	RT_TRACE(COMP_INIT, "Driver probe completed\n");
//#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
//	return dev;
//#else
	return 0;	
//#endif

fail1:
	
#ifdef CONFIG_RTL8180_IO_MAP
		
	if( dev->base_addr != 0 ){
			
		release_region(dev->base_addr, 
	       pci_resource_len(pdev, 0) );
	}
#else
	if( dev->mem_start != (unsigned long)NULL ){
		iounmap( (void *)dev->mem_start );
		release_mem_region( pci_resource_start(pdev, 1), 
				    pci_resource_len(pdev, 1) );
	}
#endif //end #ifdef RTL_IO_MAP
	
fail:
	if(dev){
		
		if (priv->irq) {
			free_irq(dev->irq, dev);
			dev->irq=0;
		}
		free_ieee80211(dev);
	}
	
	pci_disable_device(pdev);
	
	DMESG("wlan driver load failed\n");
	pci_set_drvdata(pdev, NULL);
	return -ENODEV;
	
}

void buffer_free(struct net_device *dev,struct buffer **buffer,int len,short
consistent)
{

        struct buffer *tmp,*next;
        struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
        struct pci_dev *pdev=priv->pdev;
        
        if(! *buffer) return;

        tmp=*buffer;
        do{
                next=tmp->next;
                if(consistent){
                        pci_free_consistent(pdev,len,
                                    tmp->buf,tmp->dma);
                }else{
                        pci_unmap_single(pdev, tmp->dma,
                        len,PCI_DMA_FROMDEVICE);
                        kfree(tmp->buf);
                }
                kfree(tmp);
                tmp = next;
        }
        while(next != *buffer);

        *buffer=NULL;
}


void free_rx_desc_ring(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev = priv->pdev;
	
	int count = priv->rxringcount;


	pci_free_consistent(pdev, sizeof(rx_desc_819x_pci)*count+256, 
			    priv->rxring, priv->rxringdma);

	buffer_free(dev,&(priv->rxbuffer),priv->rxbuffersize,0);
}

void free_tx_desc_rings(struct net_device *dev)
{
	
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev=priv->pdev;
	int count = priv->txringcount;
	
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txmapring->desc, priv->txmapringdma);
	buffer_free(dev,&(priv->txmapbufs),priv->txbuffsize,1);
	kfree(priv->txmapring);
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txbkpring->desc, priv->txbkpringdma);
	buffer_free(dev,&(priv->txbkpbufs),priv->txbuffsize,1);

	kfree(priv->txbkpring);
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txbepring->desc, priv->txbepringdma);
	buffer_free(dev,&(priv->txbepbufs),priv->txbuffsize,1);
	
	kfree(priv->txbepring);
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txvipring->desc, priv->txvipringdma);
	buffer_free(dev,&(priv->txvipbufs),priv->txbuffsize,1);

	kfree(priv->txvipring);
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txvopring->desc, priv->txvopringdma);
	buffer_free(dev,&(priv->txvopbufs),priv->txbuffsize,1);

	kfree(priv->txvopring);
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txcmdring->desc, priv->txcmdringdma);
	buffer_free(dev,&(priv->txcmdbufs),priv->txbuffsize,1);
	
	kfree(priv->txcmdring);
	count = priv->txbeaconcount;
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txbeaconring->desc, priv->txbeaconringdma);
	buffer_free(dev,&(priv->txbeaconbufs),priv->txbuffsize,1);
	kfree(priv->txbeaconring);
}


//detach all the work and timer structure declared or inititialize in r8192U_init function.
void rtl8192_cancel_deferred_work(struct r8192_priv* priv)
{
	
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
	cancel_work_sync(&priv->reset_wq);
	//cancel_work_sync(&priv->SetBWModeWorkItem);
	//cancel_work_sync(&priv->SwChnlWorkItem);
#else
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	cancel_delayed_work(&priv->reset_wq);
	//cancel_delayed_work(&priv->SetBWModeWorkItem);
	//cancel_delayed_work(&priv->SwChnlWorkItem);
#endif
#endif

}


static void __devexit rtl8192_pci_disconnect(struct pci_dev *pdev)
{
	struct net_device *dev = pci_get_drvdata(pdev);
	struct r8192_priv *priv ;
 	if(dev){
		
		unregister_netdev(dev);
		
		priv=ieee80211_priv(dev);
		
		rtl8192_proc_remove_one(dev);
		
		rtl8192_down(dev);
		if (priv->pFirmware)
		{
			vfree(priv->pFirmware);
			priv->pFirmware = NULL;
		}
	//	priv->rf_close(dev);
	//	rtl8192_usb_deleteendpoints(dev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		destroy_workqueue(priv->priv_wq);
#endif
		rtl8192_irq_disable(dev);
		rtl8192_reset(dev);
		mdelay(10);
		if(priv->irq){
			
			printk("Freeing irq %d\n",dev->irq);
			free_irq(dev->irq, dev);
			priv->irq=0;
			
		}
		free_rx_desc_ring(dev);
		free_tx_desc_rings(dev);
	//	free_beacon_desc_ring(dev,priv->txbeaconcount);
		
#ifdef CONFIG_RTL8180_IO_MAP
		
		if( dev->base_addr != 0 ){
			
			release_region(dev->base_addr, 
				       pci_resource_len(pdev, 0) );
		}
#else
		if( dev->mem_start != (unsigned long)NULL ){
			iounmap( (void *)dev->mem_start );
			release_mem_region( pci_resource_start(pdev, 1), 
					    pci_resource_len(pdev, 1) );
		}
#endif /*end #ifdef RTL_IO_MAP*/
		free_ieee80211(dev);

	}

	pci_disable_device(pdev);
	RT_TRACE(COMP_DOWN, "wlan driver removed\n");
}


static int __init rtl8192_pci_module_init(void)
{
	printk(KERN_INFO "\nLinux kernel driver for RTL8192 based WLAN cards\n");
	printk(KERN_INFO "Copyright (c) 2007-2008, Realsil Wlan 0830\n");
	RT_TRACE(COMP_INIT, "Initializing module");
	RT_TRACE(COMP_INIT, "Wireless extensions version %d", WIRELESS_EXT);
	rtl8192_proc_module_init();
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
      if(0!=pci_module_init(&rtl8192_pci_driver))
#else
      if(0!=pci_register_driver(&rtl8192_pci_driver))
#endif
	{
		DMESG("No device found");
		/*pci_unregister_driver (&rtl8192_pci_driver);*/
		return -ENODEV;
	}
	return 0;
}


static void __exit rtl8192_pci_module_exit(void)
{
	pci_unregister_driver(&rtl8192_pci_driver);

	RT_TRACE(COMP_DOWN, "Exiting");
	rtl8192_proc_module_remove();
}

//warning message WB
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
void rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs)
#else
irqreturn_t rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs)
#endif
#else
irqreturn_t rtl8192_interrupt(int irq, void *netdev)
#endif
{
	struct net_device *dev = (struct net_device *) netdev;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	unsigned long flags;
	u32 inta;
	/* We should return IRQ_NONE, but for now let me keep this */
	if(priv->irq_enabled == 0){
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		return;
#else
		return IRQ_HANDLED;
#endif
	}
	
	spin_lock_irqsave(&priv->irq_th_lock,flags);

	//ISR: 4bytes
 
	inta = read_nic_dword(dev, ISR);// & priv->IntrMask;
	write_nic_dword(dev,ISR,inta); // reset int situation

	priv->stats.shints++;
	//DMESG("Enter interrupt, ISR value = 0x%08x", inta);	
	if(!inta){
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		return;
#else
		return IRQ_HANDLED;  
#endif
	/* 
	   most probably we can safely return IRQ_NONE,
	   but for now is better to avoid problems
	*/
	}
	
	if(inta == 0xffff){
			/* HW disappared */
			spin_unlock_irqrestore(&priv->irq_th_lock,flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
			return;
#else
			return IRQ_HANDLED;  
#endif
	}

	priv->stats.ints++;
#ifdef DEBUG_IRQ
	DMESG("NIC irq %x",inta);
#endif
	//priv->irqpending = inta;
	
	
	if(!netif_running(dev)) {
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		return;
#else
		return IRQ_HANDLED;
#endif
	}
		
	if(inta & IMR_TIMEOUT0){
//		write_nic_dword(dev, TimerInt, 0);
		//DMESG("=================>waking up");
//		rtl8180_hw_wakeup(dev);
	}	
	
	if(inta & IMR_TBDOK){
		priv->stats.txbeaconokint++;
	}
	
	if(inta & IMR_TBDER){
		priv->stats.txbeaconerr++;
	}
	
	if(inta  & IMR_MGNTDOK ) {
//		priv->NumTxOkTotal++;
	//	printk("=====================>haha: TX management frame OK\n");
		priv->stats.txmanageokint++;
		rtl8192_tx_isr(dev,MGNT_QUEUE,0);
//			schedule_work(&priv->tx_irq_wq);

	}
	if(inta & IMR_COMDOK)
	{
		//printk("=====================>%s(): tx cmpkt ok\n",__FUNCTION__);
		priv->stats.txcmdpktokint++;
		rtl8192_tx_isr(dev,TXCMD_QUEUE,0);
	}
#if 0
	if(inta & ISR_THPDER){
#ifdef DEBUG_TX
		DMESG ("TX high priority ERR");
#endif	
		priv->stats.txhperr++;
		rtl8180_tx_isr(dev,HI_PRIORITY,1);
		priv->ieee80211->stats.tx_errors++;
	}
	
	if(inta & ISR_THPDOK){ //High priority tx ok
#ifdef DEBUG_TX
		DMESG ("TX high priority OK");
#endif	
//		priv->NumTxOkTotal++;
		priv->NumTxOkInPeriod++;
		priv->stats.txhpokint++;
		rtl8180_tx_isr(dev,HI_PRIORITY,0);
	}
#endif
	
	if(inta & IMR_ROK){
#ifdef DEBUG_RX
		DMESG("Frame arrived !");
#endif
//		priv->NumRxOkInPeriod++;
		priv->stats.rxint++;
		tasklet_schedule(&priv->irq_rx_tasklet);
	}
	
	if(inta & IMR_BcnInt) {
		DMESG("Preparing Beacons");
		//rtl8180_prepare_beacon(dev);
	}
	
	if(inta & IMR_RDU){
//DMESGW("No RX descriptor available");
//#ifdef DEBUG_RX
		DMESGW("No RX descriptor available");
		priv->stats.rxrdu++;

		write_nic_dword(dev,
				INTA_MASK,
				read_nic_dword(dev, INTA_MASK) & (~IMR_RDU)); // unmask RDU

		tasklet_schedule(&priv->irq_rx_tasklet);
		/*queue_work(priv->workqueue ,&priv->restart_work);*/		
	}
	if(inta & IMR_RXFOVW){
#ifdef DEBUG_RX
		DMESGW("RX fifo overflow");
#endif
		priv->stats.rxoverflow++;
		tasklet_schedule(&priv->irq_rx_tasklet);
		//queue_work(priv->workqueue ,&priv->restart_work);
	}
	
	if(inta & IMR_TXFOVW) priv->stats.txoverflow++;

	if(inta & IMR_BKDOK){ //corresponding to BK_QUEUE
		priv->stats.txbkokint++;
#ifdef DEBUG_TX
		printk("TX bk priority ok\n");
#endif
//		priv->NumTxOkTotal++;
		rtl8192_tx_isr(dev,BK_QUEUE,0);
		rtl8192_try_wake_queue(dev, BK_QUEUE);
	}
	
	if(inta & IMR_BEDOK){ //corresponding to BE_QUEUE
		priv->stats.txbeokint++;
#ifdef DEBUG_TX
		DMESGW("TX be priority ok\n");
#endif
//		priv->NumTxOkTotal++;
		rtl8192_tx_isr(dev,BE_QUEUE,0);
		rtl8192_try_wake_queue(dev, BE_QUEUE);
	}

	if(inta & IMR_VIDOK){ //corresponding to BK_QUEUE
		priv->stats.txviokint++;
#ifdef DEBUG_TX
		DMESGW("TX bk priority ok");
#endif
//		priv->NumTxOkTotal++;
		rtl8192_tx_isr(dev,VI_QUEUE,0);
		rtl8192_try_wake_queue(dev, VI_QUEUE);
	}
	
	if(inta & IMR_VODOK){ //corresponding to BE_QUEUE
		priv->stats.txvookint++;
#ifdef DEBUG_TX
		DMESGW("TX be priority ok");
#endif
//		priv->NumTxOkTotal++;
		rtl8192_tx_isr(dev,VO_QUEUE,0);
		rtl8192_try_wake_queue(dev, VO_QUEUE);
	}
	
	force_pci_posting(dev);
	spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	return;
#else
	return IRQ_HANDLED;
#endif
}



void rtl8192_try_wake_queue(struct net_device *dev, int pri)
{
	unsigned long flags;
	short enough_desc;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	spin_lock_irqsave(&priv->tx_lock,flags);
	enough_desc = check_nic_enough_desc(dev,pri);
        spin_unlock_irqrestore(&priv->tx_lock,flags);	
	
	if(enough_desc)
		ieee80211_wake_queue(priv->ieee80211);
}


void EnableHWSecurityConfig8192(struct net_device *dev)
{
        u8 SECR_value = 0x0;
	// struct ieee80211_device* ieee1 = container_of(&dev, struct ieee80211_device, dev);
	 //printk("==>ieee1:%p, dev:%p\n", ieee1, dev); 
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	 struct ieee80211_device* ieee = priv->ieee80211; 
	 //printk("==>ieee:%p, dev:%p\n", ieee, dev); 
	SECR_value = SCR_TxEncEnable | SCR_RxDecEnable;
#if 1
	if ((KEY_TYPE_WEP40 == ieee->pairwise_key_type) || (KEY_TYPE_WEP104 == ieee->pairwise_key_type))
	{
		SECR_value |= SCR_RxUseDK;
		SECR_value |= SCR_TxUseDK;
	}
	else if ((ieee->iw_mode == IW_MODE_ADHOC) && (ieee->pairwise_key_type & (KEY_TYPE_CCMP | KEY_TYPE_TKIP)))
	{
		SECR_value |= SCR_RxUseDK;
		SECR_value |= SCR_TxUseDK;
	}

#endif

        //add HWSec active enable here.
//default using hwsec. when peer AP is in N mode only and pairwise_key_type is none_aes(which HT_IOT_ACT_PURE_N_MODE indicates it), use software security. when peer AP is in b,g,n mode mixed and pairwise_key_type is none_aes, use g mode hw security. WB on 2008.7.4
	ieee->hwsec_active = 1;
	
	if ((ieee->pHTInfo->IOTAction&HT_IOT_ACT_PURE_N_MODE) || !ieee->hwsec_support) //add hwsec_support flag to totol control hw_sec on/off
	{
		printk("===>pure N MODE, using software encryption\n");
		ieee->hwsec_active = 0;
		SECR_value &= ~SCR_RxDecEnable;
	}
	
	printk("=====>hwsec:%d, SECR_value:%x\n", ieee->hwsec_active, SECR_value);
	{
                write_nic_byte(dev, SECR,  SECR_value);//SECR_value |  SCR_UseDK ); 
        }

}
#define TOTAL_CAM_ENTRY 32
//#define CAM_CONTENT_COUNT 8
void setKey(	struct net_device *dev, 
		u8 EntryNo,
		u8 KeyIndex, 
		u16 KeyType, 
		u8 *MacAddr, 
		u8 DefaultKey, 
		u32 *KeyContent )
{
	u32 TargetCommand = 0;
	u32 TargetContent = 0;
	u16 usConfig = 0;
	u8 i;
	if (EntryNo >= TOTAL_CAM_ENTRY)
		RT_TRACE(COMP_ERR, "cam entry exceeds in setKey()\n");

	RT_TRACE(COMP_SEC, "====>to setKey(), dev:%p, EntryNo:%d, KeyIndex:%d, KeyType:%d, MacAddr"MAC_FMT"\n", dev,EntryNo, KeyIndex, KeyType, MAC_ARG(MacAddr));
	
	if (DefaultKey)
		usConfig |= BIT15 | (KeyType<<2);
	else
		usConfig |= BIT15 | (KeyType<<2) | KeyIndex;
//	usConfig |= BIT15 | (KeyType<<2) | (DefaultKey<<5) | KeyIndex;


	for(i=0 ; i<CAM_CONTENT_COUNT; i++){
		TargetCommand  = i+CAM_CONTENT_COUNT*EntryNo;
		TargetCommand |= BIT31|BIT16;

		if(i==0){//MAC|Config
			TargetContent = (u32)(*(MacAddr+0)) << 16|
					(u32)(*(MacAddr+1)) << 24|
					(u32)usConfig;

			write_nic_dword(dev, WCAMI, TargetContent); 
			write_nic_dword(dev, RWCAM, TargetCommand);
	//		printk("setkey cam =%8x\n", read_cam(dev, i+6*EntryNo));
		}
		else if(i==1){//MAC
                        TargetContent = (u32)(*(MacAddr+2)) 	 |
                                        (u32)(*(MacAddr+3)) <<  8|
                                        (u32)(*(MacAddr+4)) << 16|
                                        (u32)(*(MacAddr+5)) << 24;
			write_nic_dword(dev, WCAMI, TargetContent); 
			write_nic_dword(dev, RWCAM, TargetCommand);
		}
		else {	//Key Material
			if(KeyContent != NULL)
			{
			write_nic_dword(dev, WCAMI, (u32)(*(KeyContent+i-2)) ); 
			write_nic_dword(dev, RWCAM, TargetCommand);
		}
	}
	}

}
// This function seems not ready! WB 
void CamPrintDbgReg(struct net_device* dev)
{
	unsigned long rvalue;
	unsigned char ucValue;
	write_nic_dword(dev, DCAM, 0x80000000);
	msleep(40);
	rvalue = read_nic_dword(dev, DCAM);	//delay_ms(40);
	RT_TRACE(COMP_SEC, " TX CAM=%8lX ",rvalue);
	if((rvalue & 0x40000000) != 0x4000000) 
		RT_TRACE(COMP_SEC, "-->TX Key Not Found      ");
	msleep(20);
	write_nic_dword(dev, DCAM, 0x00000000);	//delay_ms(40);
	rvalue = read_nic_dword(dev, DCAM);	//delay_ms(40);
	RT_TRACE(COMP_SEC, "RX CAM=%8lX ",rvalue);
	if((rvalue & 0x40000000) != 0x4000000) 
		RT_TRACE(COMP_SEC, "-->CAM Key Not Found   ");
	ucValue = read_nic_byte(dev, SECR);
	RT_TRACE(COMP_SEC, "WPA_Config=%x \n",ucValue);
}


/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/
module_init(rtl8192_pci_module_init);
module_exit(rtl8192_pci_module_exit);
