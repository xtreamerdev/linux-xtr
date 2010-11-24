/*
   This is part of rtl818x pci OpenSource driver - v 0.1
   Copyright (C) Andrea Merello 2004-2005  <andreamrl@tiscali.it> 
   Released under the terms of GPL (General Public License)
   
   Parts of this driver are based on the GPL part of the official 
   Realtek driver.
   
   Parts of this driver are based on the rtl8180 driver skeleton 
   from Patric Schenke & Andres Salomon.

   Parts of this driver are based on the Intel Pro Wireless 2100 GPL driver.
   
   Parts of BB/RF code are derived from David Young rtl8180 netbsd driver.
   
   RSSI calc function from 'The Deuce'
   
   Some ideas borrowed from the 8139too.c driver included in linux kernel.
   
   We (I?) want to thanks the Authors of those projecs and also the 
   Ndiswrapper's project Authors.
   
   A big big thanks goes also to Realtek corp. for their help in my attempt to 
   add RTL8185 and RTL8225 support, and to David Young also.
*/

#if 0
double __floatsidf (int i) { return i; }
unsigned int __fixunsdfsi (double d) { return d; }
double __adddf3(double a, double b) { return a+b; }
double __addsf3(float a, float b) { return a+b; }
double __subdf3(double a, double b) { return a-b; }
double __extendsfdf2(float a) {return a;}
#endif



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

//#define CONFIG_RTL8180_IO_MAP

#include "r8180_hw.h"
#include "r8180.h"
#include "r8180_sa2400.h"  /* PHILIPS Radio frontend */
#include "r8180_max2820.h" /* MAXIM Radio frontend */
#include "r8180_gct.h"     /* GCT Radio frontend */
#include "r8180_rtl8225.h" /* RTL8225 Radio frontend */
#include "r8180_rtl8255.h" /* RTL8255 Radio frontend */
#include "r8180_93cx6.h"   /* Card EEPROM */
#include "r8180_wx.h"

#ifdef CONFIG_RTL8180_PM
#include "r8180_pm.h"
#endif

#ifndef PCI_VENDOR_ID_BELKIN
	#define PCI_VENDOR_ID_BELKIN 0x1799
#endif
#ifndef PCI_VENDOR_ID_DLINK
	#define PCI_VENDOR_ID_DLINK 0x1186
#endif

#ifdef __i386__
#define rdtsc(low,high)\
   __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high))
#endif
	

CHANNEL_LIST	DefaultChannelPlan[] = {
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,149,153,157,161,165},24},
	{{1,2,3,4,5,6,7,8,9,10,11},11},
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64},21},
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},				//sean fix (for cameo)
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},};//Ida add for 11a


static struct pci_device_id rtl8180_pci_id_tbl[] __devinitdata = {
        {
                .vendor = PCI_VENDOR_ID_REALTEK,
                .device = 0x8180,
                .subvendor = PCI_ANY_ID,
                .subdevice = PCI_ANY_ID,
                .driver_data = 0,
        },
        {
                .vendor = PCI_VENDOR_ID_BELKIN,
                .device = 0x6001,
                .subvendor = PCI_ANY_ID,
                .subdevice = PCI_ANY_ID,
                .driver_data = 1,
        },
        {       /* Belkin F5D6020 v3 */
	        .vendor = PCI_VENDOR_ID_BELKIN,
                .device = 0x6020,
                .subvendor = PCI_ANY_ID,
                .subdevice = PCI_ANY_ID,
                .driver_data = 2,
        },
        {       /* D-Link DWL-610 */
                .vendor = PCI_VENDOR_ID_DLINK,
                .device = 0x3300,
                .subvendor = PCI_ANY_ID,
                .subdevice = PCI_ANY_ID,
                .driver_data = 3,
        },
	{
		.vendor = PCI_VENDOR_ID_REALTEK,
		.device = 0x8185,
		.subvendor = PCI_ANY_ID,
		.subdevice = PCI_ANY_ID,
		.driver_data = 4,
	},
        {
                .vendor = 0,
                .device = 0,
                .subvendor = 0,
                .subdevice = 0,
                .driver_data = 0,
        }
};


static char* ifname = "wlan%d";
static int hwseqnum = 0;
static int hwwep = 0;
static int channels = 0x3fff;


MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(pci, rtl8180_pci_id_tbl);
MODULE_AUTHOR("Andrea Merello <andreamrl@tiscali.it>");
MODULE_DESCRIPTION("Linux driver for Realtek RTL8180 / RTL8185 WiFi cards");



/*
MODULE_PARM(ifname, "s");
MODULE_PARM_DESC(devname," Net interface name, wlan%d=default");

MODULE_PARM(hwseqnum,"i");
MODULE_PARM_DESC(hwseqnum," Try to use hardware 802.11 header sequence numbers. Zero=default");

MODULE_PARM(hwwep,"i");
MODULE_PARM_DESC(hwwep," Try to use hardware WEP support. Still broken and not available on all cards");

MODULE_PARM(channels,"i");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");
*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)
module_param(ifname, charp, S_IRUGO|S_IWUSR );
module_param(hwseqnum,int, S_IRUGO|S_IWUSR);
module_param(hwwep,int, S_IRUGO|S_IWUSR);
module_param(channels,int, S_IRUGO|S_IWUSR);
#else
MODULE_PARM(ifname, "s");
MODULE_PARM(hwseqnum,"i");
MODULE_PARM(hwwep,"i");
MODULE_PARM(channels,"i");
#endif

MODULE_PARM_DESC(devname," Net interface name, wlan%d=default");
MODULE_PARM_DESC(hwseqnum," Try to use hardware 802.11 header sequence numbers. Zero=default");
MODULE_PARM_DESC(hwwep," Try to use hardware WEP support. Still broken and not available on all cards");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");


static int __devinit rtl8180_pci_probe(struct pci_dev *pdev, 
				       const struct pci_device_id *id);

static void __devexit rtl8180_pci_remove(struct pci_dev *pdev);


static struct pci_driver rtl8180_pci_driver = {
	.name		= RTL8180_MODULE_NAME,	          /* Driver name   */
	.id_table	= rtl8180_pci_id_tbl,	          /* PCI_ID table  */
	.probe		= rtl8180_pci_probe,	          /* probe fn      */
	.remove		= __devexit_p(rtl8180_pci_remove),/* remove fn     */
#ifdef CONFIG_RTL8180_PM
	.suspend	= rtl8180_suspend,	          /* PM suspend fn */
	.resume		= rtl8180_resume,                 /* PM resume fn  */
#else
	.suspend	= NULL,			          /* PM suspend fn */
	.resume      	= NULL,			          /* PM resume fn  */
#endif
};


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


inline void force_pci_posting(struct net_device *dev)
{
	read_nic_byte(dev,EPROM_CMD);
#ifndef CONFIG_RTL8180_IO_MAP
	mb();
#endif
}


irqreturn_t rtl8180_interrupt(int irq, void *netdev, struct pt_regs *regs);
void set_nic_rxring(struct net_device *dev);
void set_nic_txring(struct net_device *dev);
static struct net_device_stats *rtl8180_stats(struct net_device *dev);
void rtl8180_commit(struct net_device *dev);
void rtl8180_start_tx_beacon(struct net_device *dev);

/****************************************************************************
   -----------------------------PROCFS STUFF-------------------------
*****************************************************************************/

static struct proc_dir_entry *rtl8180_proc = NULL;

static int proc_get_registers(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
//	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	int i,n;
			
	int max=0xff;
	
	/* This dump the current register page */
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


		
	*eof = 1;
	return len;

}

int get_curr_tx_free_desc(struct net_device *dev, int priority);

static int proc_get_stats_hw(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"NIC int: %lu\n"
		"Total int: %lu\n"
		"--------------------\n"
		"LP avail desc %d\n"
		"NP avail desc %d\n"
		"--------------------\n"
		"LP phys dma addr %x\n"
		"LP NIC ptr %x\n"
		"LP virt 32base %x\n"
		"LP virt 32tail %x\n"
		"--------------------\n"
		"NP phys dma addr %x\n"
		"NP NIC ptr %x\n"
		"NP virt 32base %x\n"
		"NP virt 32tail %x\n"
		"--------------------\n"
		"BP phys dma addr %x\n"
		"BP NIC ptr %x\n"
		"BP virt 32base %x\n"
		"BP virt 32tail %x\n",
		priv->stats.ints,
		priv->stats.shints,
		get_curr_tx_free_desc(dev,LOW_PRIORITY),
		get_curr_tx_free_desc(dev,NORM_PRIORITY),
		(u32)priv->txlpringdma,
		read_nic_dword(dev,TLPDA),
		(u32)priv->txlpring,
		(u32)priv->txlpringtail,
		(u32)priv->txnpringdma,
		read_nic_dword(dev,TNPDA),
		(u32)priv->txnpring,
		(u32)priv->txnpringtail,
		(u32)priv->txbeaconringdma,
		read_nic_dword(dev,TBDA),
		(u32)priv->txbeaconring,
		(u32)priv->txbeaconringtail);
	*eof = 1;
	return len;
}


static int proc_get_stats_rx(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
	/*	"RX descriptor not available: %lu\n"
		"RX incomplete (missing last descriptor): %lu\n"
		"RX not data: %lu\n"
		//"RX descriptor pointer reset: %lu\n"
		"RX descriptor pointer lost: %lu\n"
		//"RX pointer workaround: %lu\n"
		"RX error int: %lu\n"
		"RX fifo overflow: %lu\n"
		"RX int: %lu\n"
		"RX packet: %lu\n"
		"RX bytes: %lu\n"
		"RX DMA fail: %lu\n",
		priv->stats.rxrdu,
		priv->stats.rxnolast,
		priv->stats.rxnodata,
		//priv->stats.rxreset,
		priv->stats.rxnopointer,
		//priv->stats.rxwrkaround,
		priv->stats.rxerr,
		priv->stats.rxoverflow,
		priv->stats.rxint,
		priv->ieee80211->stats.rx_packets,
		priv->ieee80211->stats.rx_bytes,
		priv->stats.rxdmafail  */
		"RX OK: %lu\n"
		"RX Retry: %lu\n"
		"RX CRC Error(0-500): %lu\n"
		"RX CRC Error(500-1000): %lu\n"
		"RX CRC Error(>1000): %lu\n"
		"RX ICV Error: %lu\n",
		priv->stats.rxint,
		priv->stats.rxerr,
		priv->stats.rxcrcerrmin,
		priv->stats.rxcrcerrmid,
		priv->stats.rxcrcerrmax,
		priv->stats.rxicverr
		);
	
	*eof = 1;
	return len;
}

#if 0
static int proc_get_stats_ieee(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"TXed association requests: %u\n"
		"TXed authentication requests: %u\n"
		"RXed successful association response: %u\n"
		"RXed failed association response: %u\n"
		"RXed successful authentication response: %u\n"
		"RXed failed authentication response: %u\n"
		"Association requests without response: %u\n"
		"Authentication requests without response: %u\n"
		"TX probe response: %u\n"
		"RX probe request: %u\n"
		"TX probe request: %lu\n"
		"RX authentication requests: %lu\n"			
		"RX association requests: %lu\n"
		"Reassociations: %lu\n",
		priv->ieee80211->ieee_stats.tx_ass,
		priv->ieee80211->ieee_stats.tx_aut,
		priv->ieee80211->ieee_stats.rx_ass_ok,
		priv->ieee80211->ieee_stats.rx_ass_err,
		priv->ieee80211->ieee_stats.rx_aut_ok,
		priv->ieee80211->ieee_stats.rx_aut_err,
		priv->ieee80211->ieee_stats.ass_noresp,
		priv->ieee80211->ieee_stats.aut_noresp,
		priv->ieee80211->ieee_stats.tx_probe,
		priv->ieee80211->ieee_stats.rx_probe,
		priv->ieee80211->ieee_stats.tx_probe_rq,
		priv->ieee80211->ieee_stats.rx_auth_rq,
		priv->ieee80211->ieee_stats.rx_assoc_rq,
		priv->ieee80211->ieee_stats.reassoc);

	*eof = 1;
	return len;
}
#endif
#if 0
static int proc_get_stats_ap(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct mac_htable_t *list;
	int i;
	int len = 0;
	
	if(priv->ieee80211->iw_mode != IW_MODE_MASTER){
		len += snprintf(page + len, count - len,
		"Card is not acting as AP...\n"
		);
	}else{
		len += snprintf(page + len, count - len,
		"List of associated STA:\n"
		);
		
		for(i=0;i<MAC_HTABLE_ENTRY;i++)
			for (list = priv->ieee80211->assoc_htable[i]; list!=NULL; list = list->next){
				len += snprintf(page + len, count - len, 
					MACSTR"\n",MAC2STR(list->adr)); 
			} 
		
	}
	*eof = 1;
	return len;
}
#endif

static int proc_get_stats_tx(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	int len = 0;
	unsigned long totalOK;
	
	totalOK=priv->stats.txnpokint+priv->stats.txhpokint+priv->stats.txlpokint;
	len += snprintf(page + len, count - len,
	/*	"TX normal priority ok int: %lu\n"
		"TX normal priority error int: %lu\n"
		"TX high priority ok int: %lu\n"
		"TX high priority failed error int: %lu\n"
		"TX low priority ok int: %lu\n"
		"TX low priority failed error int: %lu\n"
		"TX bytes: %lu\n"
		"TX packets: %lu\n"
		"TX queue resume: %lu\n"
		"TX queue stopped?: %d\n"
		"TX fifo overflow: %lu\n"
		//"SW TX stop: %lu\n"
		//"SW TX wake: %lu\n"
		"TX beacon: %lu\n"
		"TX beacon aborted: %lu\n",
		priv->stats.txnpokint,
		priv->stats.txnperr,
		priv->stats.txhpokint,
		priv->stats.txhperr,
		priv->stats.txlpokint,
		priv->stats.txlperr,
		priv->ieee80211->stats.tx_bytes,
		priv->ieee80211->stats.tx_packets,
		priv->stats.txresumed,
		netif_queue_stopped(dev),
		priv->stats.txoverflow,
		//priv->ieee80211->ieee_stats.swtxstop,
		//priv->ieee80211->ieee_stats.swtxawake,
		priv->stats.txbeacon,
		priv->stats.txbeaconerr  */
		"TX OK: %lu\n"
		"TX Error: %lu\n"
		"TX Retry: %lu\n"
		"TX beacon OK: %lu\n"
		"TX beacon error: %lu\n",
		totalOK,
		priv->stats.txnperr+priv->stats.txhperr+priv->stats.txlperr,
		priv->stats.txretry,
		priv->stats.txbeacon,
		priv->stats.txbeaconerr		
	);
			
	*eof = 1;
	return len;
}		



static struct iw_statistics *r8180_get_wireless_stats(struct net_device *dev)
{
       struct r8180_priv *priv = ieee80211_priv(dev);

       return &priv->wstats;
}

void rtl8180_proc_module_init(void)
{	
	DMESG("Initializing proc filesystem");
	rtl8180_proc=create_proc_entry(RTL8180_MODULE_NAME, S_IFDIR, proc_net);
}


void rtl8180_proc_module_remove(void)
{
	remove_proc_entry(RTL8180_MODULE_NAME, proc_net);
}


void rtl8180_proc_remove_one(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	if (priv->dir_dev) {
		remove_proc_entry("stats-hw", priv->dir_dev);
		remove_proc_entry("stats-tx", priv->dir_dev);
		remove_proc_entry("stats-rx", priv->dir_dev);
//		remove_proc_entry("stats-ieee", priv->dir_dev);
//		remove_proc_entry("stats-ap", priv->dir_dev);
		remove_proc_entry("registers", priv->dir_dev);
		remove_proc_entry(dev->name, rtl8180_proc);
		priv->dir_dev = NULL;
	}
}


void rtl8180_proc_init_one(struct net_device *dev)
{
	struct proc_dir_entry *e;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dir_dev = create_proc_entry(dev->name, 
					  S_IFDIR | S_IRUGO | S_IXUGO, 
					  rtl8180_proc);
	if (!priv->dir_dev) {
		DMESGE("Unable to initialize /proc/net/rtl8180/%s\n",
		      dev->name);
		return;
	}
	
	e = create_proc_read_entry("stats-hw", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_hw, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8180/%s/stats-hw\n",
		      dev->name);
	}
	
	e = create_proc_read_entry("stats-rx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_rx, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8180/%s/stats-rx\n",
		      dev->name);
	}
	
	
	e = create_proc_read_entry("stats-tx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_tx, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8180/%s/stats-tx\n",
		      dev->name);
	}
	#if 0
	e = create_proc_read_entry("stats-ieee", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_ieee, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8180/%s/stats-ieee\n",
		      dev->name);
	}
	#endif
	#if 0
	e = create_proc_read_entry("stats-ap", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_ap, dev);
				   
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8180/%s/stats-ap\n",
		      dev->name);
	}
	#endif
	
	e = create_proc_read_entry("registers", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers, dev);
	
	if (!e) {
		DMESGE("Unable to initialize "
		      "/proc/net/rtl8180/%s/registers\n",
		      dev->name);
	}
}
/****************************************************************************
   -----------------------------MISC STUFF-------------------------
*****************************************************************************/
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


void buffer_free(struct net_device *dev,struct buffer **buffer,int len,short
consistent)
{
		
	struct buffer *tmp,*next;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev=priv->pdev;
	//int i;
	
	if(! *buffer) return;
	
	/*for(tmp=*buffer; tmp->next != *buffer; tmp=tmp->next)
		
	*/
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


void print_buffer(u32 *buffer, int len)
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
	struct r8180_priv *priv = ieee80211_priv(dev);
	u32* tail;
	u32* head;
	int ret;
	
	switch (priority){
	case LOW_PRIORITY:
		head = priv->txlpringhead;
		tail = priv->txlpringtail;
		break;
	case HI_PRIORITY:
		head = priv->txhpringhead;
		tail = priv->txhpringtail;
		break;
	case NORM_PRIORITY:
		head = priv->txnpringhead;
		tail = priv->txnpringtail;
		break;
	default:
		return -1;
	}
	
	//DMESG("%x %x", head, tail);

/* FIXME FIXME FIXME FIXME */

#if 0
	if( head <= tail ) return priv->txringcount-1 - (tail - head)/8;
	return (head - tail)/8/4;
#else
	if( head <= tail ) 
		ret = priv->txringcount - (tail - head)/8;
	else
		ret = (head - tail)/8;
	
	if(ret > priv->txringcount ) DMESG("BUG");
	return ret;
#endif
}


short check_nic_enought_desc(struct net_device *dev, int priority)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	int requiredbyte, required;
	requiredbyte = priv->ieee80211->fts + sizeof(struct ieee80211_header_data);
	required = requiredbyte / (priv->txbuffsize-4);
	if (requiredbyte % priv->txbuffsize) required++; 
	/* for now we keep two free descriptor as a safety boundary 
	 * between the tail and the head 
	 */
	 
	return (required+2 < get_curr_tx_free_desc(dev,priority));
}


/* This function is only for debuging purpose */ 
void check_tx_ring(struct net_device *dev, int pri)
{
	static int maxlog =3;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u32* tmp;
	struct buffer *buf;
	int i;
	int nic;
	u32* tail;
	u32* head;
	u32* begin;
	u32 nicbegin;
	struct buffer* buffer;
	
	maxlog --;
	if (maxlog <0 ) return;
	
	switch(pri) {
	case LOW_PRIORITY:
		tail = priv->txlpringtail;
		begin = priv->txlpring;
		head = priv->txlpringhead;
		buffer = priv->txlpbufs;
		nic = read_nic_dword(dev,TX_LOWPRIORITY_RING_ADDR);
		nicbegin = priv->txlpringdma;
		break;
		
	case HI_PRIORITY:
		tail = priv->txhpringtail;
		begin = priv->txhpring;
		head = priv->txhpringhead;
		nic = read_nic_dword(dev,TX_HIGHPRIORITY_RING_ADDR);
		buffer = priv->txhpbufs;
		nicbegin = priv->txhpringdma;
		break;
	  
	case NORM_PRIORITY:
		tail = priv->txnpringtail;
		begin = priv->txnpring;
		head = priv->txnpringhead;	
		nic = read_nic_dword(dev,TX_NORMPRIORITY_RING_ADDR);    
		buffer = priv->txnpbufs;
		nicbegin = priv->txnpringdma;
		break;
	default:
		return ;
		break;
	}
	
	if(!priv->txnpbufs) 
		DMESGE ("NIC TX ack, but TX queue corrupted!");
	else{
		
		for(i=0,buf=buffer, tmp=begin;
			tmp<begin+(priv->txringcount)*8;
			tmp+=8,buf=buf->next,i++)
			
			DMESG("BUF%d %s %x %s. Next : %x",i, 
			      *tmp & (1<<31) ? "filled" : "empty",
			      *(buf->buf),
			      *tmp & (1<<15)? "ok": "err", *(tmp+4));	
	}
	
	DMESG("nic at %d", 
		(nic-nicbegin) / 8 /4);
	DMESG("tail at %d", ((int)tail - (int)begin) /8 /4);
	DMESG("head at %d", ((int)head - (int)begin) /8 /4);
	DMESG("check free desc returns %d", check_nic_enought_desc(dev,pri));
	DMESG("free desc is %d\n", get_curr_tx_free_desc(dev,pri));
	//rtl8180_reset(dev);
	return;
}



/* this function is only for debugging purpose */
void check_rxbuf(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u32* tmp;
	struct buffer *buf;
	
	if(!priv->rxbuffer) 
		DMESGE ("NIC RX ack, but RX queue corrupted!");
	
	else{
		
		for(buf=priv->rxbuffer, tmp=priv->rxring;
		    tmp < priv->rxring+(priv->rxringcount)*4;
		    tmp+=4, buf=buf->next)
			
			DMESG("BUF %s %x", 
			      *tmp & (1<<31) ? "empty" : "filled",
			      *(buf->buf));		
	}
	
	return;
}


void dump_eprom(struct net_device *dev)
{
	int i;
	for(i=0; i<63; i++)
		DMESG("EEPROM addr %x : %x", i, eprom_read(dev,i));
}


void rtl8180_dump_reg(struct net_device *dev)
{
	int i;
	int n;
	int max=0xff;
	
	DMESG("Dumping NIC register map");	
	
	for(n=0;n<=max;)
	{
		printk( "\nD: %2x> ", n);
		for(i=0;i<16 && n<=max;i++,n++)
			printk("%2x ",read_nic_byte(dev,n));
	}
	printk("\n");
}


void fix_tx_fifo(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u32 *tmp;
	int i;
#ifdef DEBUG_TX_ALLOC
	DMESG("FIXING TX FIFOs");
#endif
	
	for (tmp=priv->txnpring, i=0;
	     i < priv->txringcount; 
	     tmp+=8, i++) {
		*tmp = *tmp &~ (1<<31);		
	}
	
	for (tmp=priv->txhpring, i=0;
	     i < priv->txringcount; 
	     tmp+=8,i++){
		*tmp = *tmp &~ (1<<31);		
	}
	
	for (tmp=priv->txlpring, i=0;
	     i < priv->txringcount;
	     tmp+=8, i++){
		*tmp = *tmp &~ (1<<31);		
	}
	
	for (tmp=priv->txbeaconring, i=0;
	     i < priv->txbeaconcount;
	     tmp+=8, i++){
		*tmp = *tmp &~ (1<<31);		
	}
#ifdef DEBUG_TX_ALLOC
	DMESG("TX FIFOs FIXED");
#endif
	
	priv->txlpringtail = priv->txlpring;
	priv->txlpringhead = priv->txlpring;
	priv->txlpbufstail = priv->txlpbufs;
	priv->txnpringtail = priv->txnpring;
	priv->txnpringhead = priv->txnpring;
	priv->txnpbufstail = priv->txnpbufs;
	priv->txhpringtail = priv->txhpring;
	priv->txhpringhead = priv->txhpring;
	priv->txhpbufstail = priv->txhpbufs;
	priv->txbeaconringtail = priv->txbeaconring;
	priv->txbeaconbufstail = priv->txbeaconbufs;
	set_nic_txring(dev);
	
	ieee80211_reset_queue(priv->ieee80211);
	priv->ack_tx_to_ieee = 0; 
}


void fix_rx_fifo(struct net_device *dev) 
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u32 *tmp;
	struct buffer *rxbuf;
	
#ifdef DEBUG_RXALLOC
	DMESG("FIXING RX FIFO");
	check_rxbuf(dev);
#endif
	
	for (tmp=priv->rxring, rxbuf=priv->rxbufferhead;
	     (tmp < (priv->rxring)+(priv->rxringcount)*4); 
	     tmp+=4,rxbuf=rxbuf->next){
		*(tmp+2) = rxbuf->dma;
		*tmp=*tmp &~ 0xfff;
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


/****************************************************************************
      ------------------------------HW STUFF---------------------------
*****************************************************************************/

unsigned char QUALITY_MAP[] = {
  0x64, 0x64, 0x64, 0x63, 0x63, 0x62, 0x62, 0x61, 
  0x61, 0x60, 0x60, 0x5f, 0x5f, 0x5e, 0x5d, 0x5c, 
  0x5b, 0x5a, 0x59, 0x57, 0x56, 0x54, 0x52, 0x4f, 
  0x4c, 0x49, 0x45, 0x41, 0x3c, 0x37, 0x31, 0x29, 
  0x24, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 
  0x22, 0x22, 0x21, 0x21, 0x21, 0x21, 0x21, 0x20,  
  0x20, 0x20, 0x20, 0x1f, 0x1f, 0x1e, 0x1e, 0x1e, 
  0x1d, 0x1d, 0x1c, 0x1c, 0x1b, 0x1a, 0x19, 0x19, 
  0x18, 0x17, 0x16, 0x15, 0x14, 0x12, 0x11, 0x0f, 
  0x0e, 0x0c, 0x0a, 0x08, 0x06, 0x04, 0x01, 0x00
};

unsigned char STRENGTH_MAP[] = { 
  0x64, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5f, 0x5e, 
  0x5d, 0x5c, 0x5b, 0x5a, 0x57, 0x54, 0x52, 0x50, 
  0x4e, 0x4c, 0x4a, 0x48, 0x46, 0x44, 0x41, 0x3f, 
  0x3c, 0x3a, 0x37, 0x36, 0x36, 0x1c, 0x1c, 0x1b, 
  0x1b, 0x1a, 0x1a, 0x19, 0x19, 0x18, 0x18, 0x17, 
  0x17, 0x16, 0x16, 0x15, 0x15, 0x14, 0x14, 0x13, 
  0x13, 0x12, 0x12, 0x11, 0x11, 0x10, 0x10, 0x0f, 
  0x0f, 0x0e, 0x0e, 0x0d, 0x0d, 0x0c, 0x0c, 0x0b, 
  0x0b, 0x0a, 0x0a, 0x09, 0x09, 0x08, 0x08, 0x07, 
  0x07, 0x06, 0x06, 0x05, 0x04, 0x03, 0x02, 0x00 
};

void rtl8180_RSSI_calc(struct net_device *dev, u8 *rssi, u8 *qual){
	//void Mlme_UpdateRssiSQ(struct net_device *dev, u8 *rssi, u8 *qual){
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);	
	u32 temp;
	u32 temp2;
	u32 temp3;
	u32 lsb;
	u32 q;
	u32 orig_qual;
	u8  _rssi;
	
	q = *qual;
	orig_qual = *qual;
	_rssi = 0; // avoid gcc complains..
	
	if (q <= 0x4e) {
		temp = QUALITY_MAP[q];
	} else {
		if( q & 0x80 ) {
			temp = 0x32;
		} else {
			temp = 1;
		}
	}

	*qual = temp;
	temp2 = *rssi;

	switch(priv->rf_chip){
	case RFCHIPID_RFMD:
		lsb = temp2 & 1;
		temp2 &= 0x7e;
		if ( !lsb || !(temp2 <= 0x3c) ) {
			temp2 = 0x64;
		} else {
			temp2 = 100 * temp2 / 0x3c;
		}
		*rssi = temp2 & 0xff;
		_rssi = temp2 & 0xff;
		break;
	case RFCHIPID_INTERSIL:
		lsb = temp2;
		temp2 &= 0xfffffffe;
		temp2 *= 251;
		temp3 = temp2;
		temp2 <<= 6;
		temp3 += temp2;
		temp3 <<= 1;
		temp2 = 0x4950df;
		temp2 -= temp3;
		lsb &= 1;
		if ( temp2 <= 0x3e0000 ) {
			if ( temp2 < 0xffef0000 )
				temp2 = 0xffef0000;
		} else {
			temp2 = 0x3e0000;
		}
		if ( !lsb ) {
			temp2 -= 0xf0000;
		} else {
			temp2 += 0xf0000;
		}

		temp3 = 0x4d0000;
		temp3 -= temp2;
		temp3 *= 100;
		temp3 = temp3 / 0x6d;
		temp3 >>= 0x10;
		_rssi = temp3 & 0xff;
		*rssi = temp3 & 0xff;
		break;
	case RFCHIPID_GCT:
	        lsb = temp2 & 1;
		temp2 &= 0x7e;
		if ( ! lsb || !(temp2 <= 0x3c) ){
			temp2 = 0x64;
		} else {
			temp2 = (100 * temp2) / 0x3c;
		}
		*rssi = temp2 & 0xff;
		_rssi = temp2 & 0xff;
		break;
	case RFCHIPID_PHILIPS:
		if( orig_qual <= 0x4e ){
			_rssi = STRENGTH_MAP[orig_qual];
			*rssi = _rssi;
		} else {
			orig_qual -= 0x80;
			if ( !orig_qual ){
				_rssi = 1;
				*rssi = 1;
			} else {
				_rssi = 0x32;
				*rssi = 0x32;
			}
		}
		break;
		
	/* case 4 */
	case RFCHIPID_MAXIM:
		lsb = temp2 & 1;
		temp2 &= 0x7e;
		temp2 >>= 1;
		temp2 += 0x42;
		if( lsb != 0 ){
			temp2 += 0xa;
		}
		*rssi = temp2 & 0xff;
		_rssi = temp2 & 0xff;
		break;
	}

	if ( _rssi < 0x64 ){
		if ( _rssi == 0 ) {
			*rssi = 1;
		}
	} else {
		*rssi = 0x64;
	}
	
	return;
}


void rtl8180_irq_enable(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);	
	priv->irq_enabled = 1;
/*
	write_nic_word(dev,INTA_MASK,INTA_RXOK | INTA_RXDESCERR | INTA_RXOVERFLOW |\ 
	INTA_TXOVERFLOW | INTA_HIPRIORITYDESCERR | INTA_HIPRIORITYDESCOK |\ 
	INTA_NORMPRIORITYDESCERR | INTA_NORMPRIORITYDESCOK |\
	INTA_LOWPRIORITYDESCERR | INTA_LOWPRIORITYDESCOK | INTA_TIMEOUT);
*/
	write_nic_word(dev,INTA_MASK, priv->irq_mask);
}


void rtl8180_irq_disable(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);	

	write_nic_word(dev,INTA_MASK,0);
	force_pci_posting(dev);
	priv->irq_enabled = 0;
}


void rtl8180_set_mode(struct net_device *dev,int mode)
{
	u8 ecmd;
	ecmd=read_nic_byte(dev, EPROM_CMD);
	ecmd=ecmd &~ EPROM_CMD_OPERATING_MODE_MASK;
	ecmd=ecmd | (mode<<EPROM_CMD_OPERATING_MODE_SHIFT);
	ecmd=ecmd &~ (1<<EPROM_CS_SHIFT);
	ecmd=ecmd &~ (1<<EPROM_CK_SHIFT);
	write_nic_byte(dev, EPROM_CMD, ecmd);
}

void rtl8180_adapter_start(struct net_device *dev);
void rtl8180_beacon_tx_enable(struct net_device *dev);

void rtl8180_update_msr(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	u8 msr;
	
	msr  = read_nic_byte(dev, MSR);
	msr &= ~ MSR_LINK_MASK;
	
	if(priv->ieee80211->state == IEEE80211_LINKED)
	{ 
		if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
			msr |= (MSR_LINK_ADHOC<<MSR_LINK_SHIFT);
		else if (priv->ieee80211->iw_mode == IW_MODE_MASTER)
			msr |= (MSR_LINK_MASTER<<MSR_LINK_SHIFT);
		else if (priv->ieee80211->iw_mode == IW_MODE_INFRA)
			msr |= (MSR_LINK_MANAGED<<MSR_LINK_SHIFT);
		else	
			msr |= (MSR_LINK_NONE<<MSR_LINK_SHIFT);
	}else		
		msr |= (MSR_LINK_NONE<<MSR_LINK_SHIFT);

	write_nic_byte(dev, MSR, msr);
	
}



void rtl8180_set_chan(struct net_device *dev,short ch)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	priv->chan=ch;
	
	priv->rf_set_chan(dev,priv->chan);
	
}


void rtl8180_rx_enable(struct net_device *dev)
{
	u8 cmd;
	u32 rxconf;
	/* for now we accept data, management & ctl frame*/
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	rxconf=read_nic_dword(dev,RX_CONF);
	rxconf = rxconf &~ MAC_FILTER_MASK;
	rxconf = rxconf | (1<<ACCEPT_MNG_FRAME_SHIFT);
	rxconf = rxconf | (1<<ACCEPT_DATA_FRAME_SHIFT);
	rxconf = rxconf | (1<<ACCEPT_BCAST_FRAME_SHIFT);
	rxconf = rxconf | (1<<ACCEPT_MCAST_FRAME_SHIFT);
//	rxconf = rxconf | (1<<ACCEPT_CRCERR_FRAME_SHIFT);
	if (dev->flags & IFF_PROMISC) DMESG ("NIC in promisc mode");
	
	if(priv->ieee80211->iw_mode == IW_MODE_MONITOR || \
	   dev->flags & IFF_PROMISC){
		rxconf = rxconf | (1<<ACCEPT_ALLMAC_FRAME_SHIFT);
	}else{
		rxconf = rxconf | (1<<ACCEPT_NICMAC_FRAME_SHIFT);
		if(priv->card_8185 == 0)
			rxconf = rxconf | (1<<RX_CHECK_BSSID_SHIFT);
	}
	
	/*if(priv->ieee80211->iw_mode == IW_MODE_MASTER){
		rxconf = rxconf | (1<<ACCEPT_ALLMAC_FRAME_SHIFT);
		rxconf = rxconf | (1<<RX_CHECK_BSSID_SHIFT);
	}*/
	
	if(priv->ieee80211->iw_mode == IW_MODE_MONITOR){
		rxconf = rxconf | (1<<ACCEPT_CTL_FRAME_SHIFT);	
		rxconf = rxconf | (1<<ACCEPT_ICVERR_FRAME_SHIFT);
		rxconf = rxconf | (1<<ACCEPT_PWR_FRAME_SHIFT);
	}
	
	if( priv->crcmon == 1 && priv->ieee80211->iw_mode == IW_MODE_MONITOR)
		rxconf = rxconf | (1<<ACCEPT_CRCERR_FRAME_SHIFT);
	
	//if(!priv->card_8185){
		rxconf = rxconf &~ RX_FIFO_THRESHOLD_MASK;
		rxconf = rxconf | (RX_FIFO_THRESHOLD_NONE<<RX_FIFO_THRESHOLD_SHIFT);
	//}
	
	rxconf = rxconf | (1<<RX_AUTORESETPHY_SHIFT);
	rxconf = rxconf &~ MAX_RX_DMA_MASK;
	rxconf = rxconf | (MAX_RX_DMA_2048<<MAX_RX_DMA_SHIFT);
	
	//if(!priv->card_8185)
		rxconf = rxconf | RCR_ONLYERLPKT;
	
	rxconf = rxconf &~ RCR_CS_MASK;
	if(!priv->card_8185)	
		rxconf |= (priv->rcr_csense<<RCR_CS_SHIFT);
//	rxconf &=~ 0xfff00000;
//	rxconf |= 0x90100000;//9014f76f;
	write_nic_dword(dev, RX_CONF, rxconf);
	
	fix_rx_fifo(dev);
	
#ifdef DEBUG_RX
	DMESG("rxconf: %x %x",rxconf ,read_nic_dword(dev,RX_CONF));
#endif
	cmd=read_nic_byte(dev,CMD);
	write_nic_byte(dev,CMD,cmd | (1<<CMD_RX_ENABLE_SHIFT));

	/* In rtl8139 driver seems that DMA threshold has to be written 
	 *  after enabling RX, so we rewrite RX_CONFIG register 
	 */
	//mdelay(100);
//	write_nic_dword(dev, RX_CONF, rxconf); 
	
}


void set_nic_txring(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
//		DMESG("ring %x %x", priv->txlpringdma,read_nic_dword(dev,TLPDA));

	write_nic_dword(dev, TX_LOWPRIORITY_RING_ADDR, priv->txlpringdma);
//		DMESG("ring %x %x", priv->txlpringdma,read_nic_dword(dev,TLPDA));
	write_nic_dword(dev, TX_NORMPRIORITY_RING_ADDR, priv->txnpringdma);
//		DMESG("ring %x %x", priv->txlpringdma,read_nic_dword(dev,TLPDA));

	write_nic_dword(dev, TX_HIGHPRIORITY_RING_ADDR, priv->txhpringdma);
//		DMESG("ring %x %x", priv->txlpringdma,read_nic_dword(dev,TLPDA));

	write_nic_dword(dev, TX_BEACON_RING_ADDR, priv->txbeaconringdma);
}


void rtl8180_conttx_enable(struct net_device *dev)
{
	u32 txconf;
	txconf = read_nic_dword(dev,TX_CONF);
	txconf = txconf &~ TX_LOOPBACK_MASK;
	txconf = txconf | (TX_LOOPBACK_CONTINUE <<TX_LOOPBACK_SHIFT);
	write_nic_dword(dev,TX_CONF,txconf);
}


void rtl8180_conttx_disable(struct net_device *dev)
{
	u32 txconf;
	txconf = read_nic_dword(dev,TX_CONF);
	txconf = txconf &~ TX_LOOPBACK_MASK;
	txconf = txconf | (TX_LOOPBACK_NONE <<TX_LOOPBACK_SHIFT);
	write_nic_dword(dev,TX_CONF,txconf);
}


void rtl8180_tx_enable(struct net_device *dev)
{
	u8 cmd;
	u8 tx_agc_ctl;
	u8 byte;
	u32 txconf;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	txconf= read_nic_dword(dev,TX_CONF);
	
	
	if(priv->card_8185){
		
	
		byte = read_nic_byte(dev,CW_CONF);
		byte &= ~(1<<CW_CONF_PERPACKET_CW_SHIFT);
		byte &= ~(1<<CW_CONF_PERPACKET_RETRY_SHIFT);
		write_nic_byte(dev, CW_CONF, byte);
		
		tx_agc_ctl = read_nic_byte(dev, TX_AGC_CTL);
		tx_agc_ctl &= ~(1<<TX_AGC_CTL_PERPACKET_GAIN_SHIFT);
		tx_agc_ctl &= ~(1<<TX_AGC_CTL_PERPACKET_ANTSEL_SHIFT);
		tx_agc_ctl |=(1<<TX_AGC_CTL_FEEDBACK_ANT);
		write_nic_byte(dev, TX_AGC_CTL, tx_agc_ctl);
		/*
		write_nic_word(dev, 0x5e, 0x01);	
		force_pci_posting(dev);
		mdelay(1);
		write_nic_word(dev, 0xfe, 0x10);
		force_pci_posting(dev);
		mdelay(1);
		write_nic_word(dev, 0x5e, 0x00);
		force_pci_posting(dev);
		mdelay(1);
		*/
		write_nic_byte(dev, 0xec, 0x3f); /* Disable early TX */
	}
	if(priv->card_8185){
		
		txconf = txconf &~ (1<<TCR_PROBE_NOTIMESTAMP_SHIFT);
	
	}else{
	
		if(hwseqnum) 
			txconf= txconf &~ (1<<TX_CONF_HEADER_AUTOICREMENT_SHIFT);
		else 
			txconf= txconf | (1<<TX_CONF_HEADER_AUTOICREMENT_SHIFT);
	}
	
	txconf = txconf &~ TX_LOOPBACK_MASK;
	txconf = txconf | (TX_LOOPBACK_NONE <<TX_LOOPBACK_SHIFT);
	txconf = txconf &~ TCR_DPRETRY_MASK;
	txconf = txconf &~ TCR_RTSRETRY_MASK;
	txconf = txconf | (priv->retry_data<<TX_DPRETRY_SHIFT);
	txconf = txconf | (priv->retry_rts<<TX_RTSRETRY_SHIFT);
	txconf = txconf &~ (1<<TX_NOCRC_SHIFT);
	
	if(priv->card_8185){
		if(priv->hw_plcp_len)
			txconf = txconf &~ TCR_PLCP_LEN;
		else
			txconf = txconf | TCR_PLCP_LEN;
	}else{
		txconf = txconf &~ TCR_SAT;
	}
	txconf = txconf &~ TCR_MXDMA_MASK;
	txconf = txconf | (TCR_MXDMA_2048<<TCR_MXDMA_SHIFT);
	txconf = txconf | TCR_CWMIN;
	txconf = txconf | TCR_DISCW;
	
//	if(priv->ieee80211->hw_wep)
//		txconf=txconf &~ (1<<TX_NOICV_SHIFT);
//	else
		txconf=txconf | (1<<TX_NOICV_SHIFT);
	
	write_nic_dword(dev,TX_CONF,txconf);
	
	fix_tx_fifo(dev);
	
#ifdef DEBUG_TX
	DMESG("txconf: %x %x",txconf,read_nic_dword(dev,TX_CONF));
#endif
	
	cmd=read_nic_byte(dev,CMD);
	write_nic_byte(dev,CMD,cmd | (1<<CMD_TX_ENABLE_SHIFT));		

//	mdelay(100);
	write_nic_dword(dev,TX_CONF,txconf);
//	#endif
/*
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev, TX_DMA_POLLING, priv->dma_poll_mask);
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
	*/
}


void rtl8180_beacon_tx_enable(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask &=~(1<<TX_DMA_STOP_BEACON_SHIFT);
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);	
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
}


void rtl8180_beacon_tx_disable(struct net_device *dev) 
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_BEACON_SHIFT);
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
}


void rtl8180_rtx_disable(struct net_device *dev)
{
	u8 cmd;
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	cmd=read_nic_byte(dev,CMD);
	write_nic_byte(dev, CMD, cmd &~ \
		       ((1<<CMD_RX_ENABLE_SHIFT)|(1<<CMD_TX_ENABLE_SHIFT)));
	force_pci_posting(dev);
	mdelay(10);
	/*while (read_nic_byte(dev,CMD) & (1<<CMD_RX_ENABLE_SHIFT))
	  udelay(10); 
	*/
	
	if(!priv->rx_skb_complete)
		dev_kfree_skb_any(priv->rx_skb);
}

#if 0
int alloc_tx_beacon_desc_ring(struct net_device *dev, int count)
{
	int i;
	u32 *tmp;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
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
	return 0;
}
#endif

short alloc_tx_desc_ring(struct net_device *dev, int bufsize, int count,
			 int addr)
{
	int i;
	u32 *desc;
	u32 *tmp;
	dma_addr_t dma_desc, dma_tmp;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev = priv->pdev;
	void *buf;
	
	if((bufsize & 0xfff) != bufsize)
	{ 
		DMESGE ("TX buffer allocation too large");
		return 0;
	}
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
	for (i=0;i<count;i++)
	{
		buf = (void*)pci_alloc_consistent(pdev,bufsize,&dma_tmp);
		if (buf == NULL) return -ENOMEM;

		switch(addr) {
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
			
		
		case TX_BEACON_RING_ADDR:
		        if(-1 == buffer_add(&(priv->txbeaconbufs),buf,dma_tmp,NULL)){
			DMESGE("Unable to allocate mem for buffer BP");
				return -ENOMEM;
			}
			break;
		}
		
		*tmp = *tmp &~ (1<<31); // descriptor empty, owned by the drv 
		*(tmp+2) = (u32)dma_tmp;
		*(tmp+3) = bufsize;

		if(i+1<count)
			*(tmp+4) = (u32)dma_desc+((i+1)*8*4);
		else
			*(tmp+4) = (u32)dma_desc;
		
		tmp=tmp+8;
	}
	
	switch(addr) {
	case TX_NORMPRIORITY_RING_ADDR:
		priv->txnpringdma=dma_desc;
		priv->txnpring=desc;
		break;
		
	case TX_LOWPRIORITY_RING_ADDR:
		priv->txlpringdma=dma_desc;
		priv->txlpring=desc;
		break;
		
	case TX_HIGHPRIORITY_RING_ADDR:
		priv->txhpringdma=dma_desc;
		priv->txhpring=desc;
		break;
		
	case TX_BEACON_RING_ADDR:
		priv->txbeaconringdma=dma_desc;
		priv->txbeaconring=desc;
		break;
		
	}
	
#ifdef DEBUG_TX
	DMESG("Tx dma physical address: %x",dma_desc);
#endif
	
	return 0;
}


void free_tx_desc_rings(struct net_device *dev)
{
	
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev=priv->pdev;
	
	int count = priv->txringcount;
	
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txlpring, priv->txlpringdma);
			    
	buffer_free(dev,&(priv->txlpbufs),priv->txbuffsize,1);

	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txhpring, priv->txhpringdma);
			    
	buffer_free(dev,&(priv->txhpbufs),priv->txbuffsize,1);

	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txnpring, priv->txnpringdma);
			    
	buffer_free(dev,&(priv->txnpbufs),priv->txbuffsize,1);

	count = priv->txbeaconcount;
	
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txbeaconring, priv->txbeaconringdma);
			    
	buffer_free(dev,&(priv->txbeaconbufs),priv->txbuffsize,1);
}

#if 0
void free_beacon_desc_ring(struct net_device *dev,int count)
{
	
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev=priv->pdev;
	
	pci_free_consistent(pdev, sizeof(u32)*8*count+256, 
			    priv->txbeaconring, priv->txbeaconringdma);
	
	if (priv->beacon_buf) 
		pci_free_consistent(priv->pdev,
			priv->master_beaconsize,priv->beacon_buf,priv->beacondmabuf);  

}
#endif
void free_rx_desc_ring(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev = priv->pdev;
	
	int count = priv->rxringcount;
		
	pci_free_consistent(pdev, sizeof(u32)*4*count+256, 
			    priv->rxring, priv->rxringdma);
			    
	buffer_free(dev,&(priv->rxbuffer),priv->rxbuffersize,0);
}


short alloc_rx_desc_ring(struct net_device *dev, u16 bufsize, int count)
{
	int i;
	u32 *desc;
	u32 *tmp;
	dma_addr_t dma_desc,dma_tmp;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct pci_dev *pdev=priv->pdev;
	void *buf;
	
	if((bufsize & 0xfff) != bufsize){ 
		DMESGE ("RX buffer allocation too large");
		return -1;
	}
	
	desc = (u32*)pci_alloc_consistent(pdev,sizeof(u32)*4*count+256,
					  &dma_desc);
	
	if(dma_desc & 0xff){ 
		
		/* 
		 * descriptor's buffer must be 256 byte aligned
		 * should never happen since we specify the DMA mask
		 */
		
		DMESGW("Fixing RX alignment"); 
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
		*tmp = *tmp |( bufsize&0xfff);
		*(tmp+2) = (u32)dma_tmp;
		*tmp = *tmp |(1<<31); // descriptor void, owned by the NIC
		
#ifdef DEBUG_RXALLOC
		DMESG("Alloc %x size buffer, DMA mem @ %x, virtual mem @ %x",
		      (u32)(bufsize&0xfff), (u32)dma_tmp, (u32)buf);
#endif
		
		tmp=tmp+4;
	}
	
	*(tmp-4) = *(tmp-4) | (1<<30); // this is the last descriptor
	
	
#ifdef DEBUG_RXALLOC
	DMESG("RX DMA physical address: %x",dma_desc);
#endif
	
	return 0;
}


void set_nic_rxring(struct net_device *dev)
{
	u8 pgreg;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	//rtl8180_set_mode(dev, EPROM_CMD_CONFIG);
	
	pgreg=read_nic_byte(dev, PGSELECT);
	write_nic_byte(dev, PGSELECT, pgreg &~ (1<<PGSELECT_PG_SHIFT));
	
	//rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
	
	write_nic_dword(dev, RXRING_ADDR,priv->rxringdma);
}


void rtl8180_reset(struct net_device *dev)
{
	//u32 txconf = 0x80e00707; //FIXME: Make me understandable
	u8 cr;
	
	//write_nic_dword(dev,TX_CONF,txconf);
	
	rtl8180_irq_disable(dev);
	
	cr=read_nic_byte(dev,CMD);
	cr = cr & 2;
	cr = cr | (1<<CMD_RST_SHIFT);
	write_nic_byte(dev,CMD,cr);
	
	force_pci_posting(dev);
	
	mdelay(200);
	
	if(read_nic_byte(dev,CMD) & (1<<CMD_RST_SHIFT)) 
		DMESGW("Card reset timeout!");
	else 
		DMESG("Card successfully reset");
	
	rtl8180_set_mode(dev,EPROM_CMD_LOAD);
	force_pci_posting(dev);
	mdelay(200);
}

inline u16 ieeerate2rtlrate(int rate)
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
inline u16 rtl8180_rate2rate(short rate)
{
	if (rate >11) return 0;
	return rtl_rate[rate]; 
}
inline u8 rtl8180_IsWirelessBMode(u16 rate)
{
	if( ((rate <= 110) && (rate != 60) && (rate != 90)) || (rate == 220) )
		return 1;
	else return 0;
}
u16 N_DBPSOfRate(u16 DataRate);
u16 ComputeTxTime( 
	u16		FrameLength,
	u16		DataRate,
	u8		bManagementFrame,
	u8		bShortPreamble
)
{
	u16	FrameTime;
	u16	N_DBPS;
	u16	Ceiling;

	if( rtl8180_IsWirelessBMode(DataRate) )
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
u16 N_DBPSOfRate(u16 DataRate)
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
		

/* This is rough RX isr handling routine*/
void rtl8180_rx(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	struct sk_buff *tmp_skb;
	
	//struct sk_buff *skb;
	short first,last;
	u32 len;
	int lastlen;
	unsigned char quality, signal;
	u8 rate;
	u32 *prism_hdr;
	u32 *tmp,*tmp2;
	
	struct ieee80211_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
	//	.mac_time = jiffies,
		.freq = IEEE80211_24GHZ_BAND,
	};
	
	
	//if (!priv->rxbuffer) DMESG ("EE: NIC RX ack, but RX queue corrupted!");
	//else {
	
	if ((*(priv->rxringtail)) & (1<<31)) {
		
		/* we have got an RX int, but the descriptor
		 * we are pointing is empty*/
		 	
		priv->stats.rxnodata++;
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
				tmp  = priv->rxring + (priv->rxringcount )*4;
			else
				tmp -= 4;
				
			if(! (*tmp & (1<<31)))
				tmp2 = tmp;		
		}while(tmp != priv->rxring);
		
		
		if(tmp2) priv->rxringtail = tmp2;
		
		#endif
		
		
		//}
	}
		
	/* while there are filled descriptors */
	while( ! (*(priv->rxringtail) & (1<<31))){
		
		
		if(*(priv->rxringtail) & (1<<26)) 
			DMESGW("RX buffer overflow");
		if(*(priv->rxringtail) & (1<<12))
			priv->stats.rxicverr++;
			
		if(*(priv->rxringtail) & (1<<27)){
			priv->stats.rxdmafail++; 
			//DMESG("EE: RX DMA FAILED at buffer pointed by descriptor %x",(u32)priv->rxringtail);	
			goto drop;
		}
			
		pci_dma_sync_single_for_cpu(priv->pdev,
				    priv->rxbuffer->dma,
				    priv->rxbuffersize * \
				    sizeof(u8),
				    PCI_DMA_FROMDEVICE);
				
		first = *(priv->rxringtail) & (1<<29) ? 1:0;
		if(first) priv->rx_prevlen=0;
			
		last = *(priv->rxringtail) & (1<<28) ? 1:0;
		if(last){
			lastlen=((*priv->rxringtail) &0xfff);
				  
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

			if(*(priv->rxringtail) & (1<<13)) {
//lastlen=((*priv->rxringtail) &0xfff);
				if ((*(priv->rxringtail) & 0xfff) <500)
					priv->stats.rxcrcerrmin++;
				else if ((*(priv->rxringtail) & 0x0fff) >1000)
					priv->stats.rxcrcerrmax++;
				else 
					priv->stats.rxcrcerrmid++;
	
			}
			    
		}else{
			len = priv->rxbuffersize;
		}
		
		priv->rx_prevlen+=len;
				
		if(priv->rx_prevlen > MAX_FRAG_THRESHOLD + 100){
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

		signal=((*(priv->rxringtail+1))& (0xff0000))>>16;
		signal=(signal&0xfe)>>1;	// Modify by hikaru 6.6
				
		quality=((*(priv->rxringtail+1)) & (0xff));
			
		rate=((*(priv->rxringtail)) &
			((1<<23)|(1<<22)|(1<<21)|(1<<20)))>>20;
				
		stats.rate = rtl8180_rate2rate(rate);		
		//DMESG("%d",rate);
		
		stats.mac_time[0] = *(priv->rxringtail+2);
		stats.mac_time[1] = *(priv->rxringtail+3);
		
		if(!priv->card_8185){		
			rtl8180_RSSI_calc(dev,&signal,&quality);
				
		priv->wstats.qual.level = signal + 125;
		priv->wstats.qual.qual = quality;
		priv->wstats.qual.noise = 100 - quality;
		priv->wstats.qual.updated = 7;

		stats.signal =  signal+125;//- (signal+50) ;
		/*we use noise to carry quality info */
		stats.noise = quality;
		} else {
			if(!rtl8180_IsWirelessBMode(stats.rate) ) 
			{ // OFDM rate.
				if(signal>90)
					signal=90;
				else if(signal<25)
					signal=25;  
				signal = (90-signal)*100/65;
			}
			else
			{ // CCK rate.
				if(signal>95)
					signal = 95;
				else if(signal<30)
					signal = 30;  
				signal =(95-signal )*100/65;
			}
			priv->wstats.qual.level = signal;

			if(quality > 64)
				priv ->wstats.qual.qual = 0;
			else
				priv ->wstats.qual.qual = ((64-quality) * 100) / 64;  // SQ value is the SIGNAL_QUALITY returned to IORequest, 
						//and this value only appear when STA is associated to AP or 
						// STA is in IBSS mode
			priv->wstats.qual.noise = 100 - priv ->wstats.qual.qual;
			priv->wstats.qual.updated = 7;
		}
#ifndef DUMMY_RX	
			
		if(first){
				
			if(!priv->rx_skb_complete){
				/* seems that HW sometimes fails to reiceve and 
				   doesn't provide the last descriptor */
#ifdef DEBUG_RX_SKB
				DMESG("going to free incomplete skb");
#endif
				dev_kfree_skb_any(priv->rx_skb);
				priv->stats.rxnolast++;
#ifdef DEBUG_RX_SKB
				DMESG("free incomplete skb OK");
#endif			
			}
				
					
			/* support for prism header has been originally added by Christian */
				
			if(priv->prism_hdr && priv->ieee80211->iw_mode == IW_MODE_MONITOR){
				
				priv->rx_skb = dev_alloc_skb(len+2+PRISM_HDR_SIZE);
				if(! priv->rx_skb) goto drop;
					
				prism_hdr = (u32*) skb_put(priv->rx_skb,PRISM_HDR_SIZE);
				prism_hdr[0]=htonl(0x80211001);        //version
				prism_hdr[1]=htonl(0x40);              //length
				prism_hdr[2]=htonl(stats.mac_time[1]);    //mactime (HIGH)
				prism_hdr[3]=htonl(stats.mac_time[0]);    //mactime (LOW)
#ifdef __i386__				
				rdtsc(prism_hdr[5], prism_hdr[4]);         //hostime (LOW+HIGH)
#endif				
				prism_hdr[4]=htonl(prism_hdr[4]);          //Byte-Order aendern
				prism_hdr[5]=htonl(prism_hdr[5]);          //Byte-Order aendern
				prism_hdr[6]=0x00;                     //phytype
				prism_hdr[7]=htonl(priv->chan);        //channel
				prism_hdr[8]=htonl(stats.rate);        //datarate
				prism_hdr[9]=0x00;                     //antenna
				prism_hdr[10]=0x00;                    //priority
				prism_hdr[11]=0x00;                    //ssi_type
				prism_hdr[12]=htonl(stats.signal);     //ssi_signal
				prism_hdr[13]=htonl(stats.noise);      //ssi_noise
				prism_hdr[14]=0x00;                    //preamble
				prism_hdr[15]=0x00;                    //encoding

			}else{
					
				priv->rx_skb = dev_alloc_skb(len+2);
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
		if(!priv->rx_skb_complete)			 
			memcpy(skb_put(priv->rx_skb,len),
				priv->rxbuffer->buf,len);
#ifdef DEBUG_RX_SKB
		DMESG("current fragment skb copy complete");
#endif

		if(last && !priv->rx_skb_complete){

#ifdef DEBUG_RX_SKB
			DMESG("Got last fragment");
#endif

			if(priv->rx_skb->len > 4)				
				skb_trim(priv->rx_skb,priv->rx_skb->len-4);
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
#ifdef DEBUG_RX
			else{
					DMESG("Rcv frag");
			}
#endif
			priv->rx_skb_complete=1;
		}
			
#endif //DUMMY_RX

		pci_dma_sync_single_for_device(priv->pdev,
				    priv->rxbuffer->dma,
				    priv->rxbuffersize * \
				    sizeof(u8),
				    PCI_DMA_FROMDEVICE);
		
		
drop: // this is used when we have not enought mem

		/* restore the descriptor */		
		*(priv->rxringtail+2)=priv->rxbuffer->dma;
		*(priv->rxringtail)=*(priv->rxringtail) &~ 0xfff;
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
			
		priv->rxringtail+=4;
		if(priv->rxringtail >= 
		   (priv->rxring)+(priv->rxringcount )*4)
			priv->rxringtail=priv->rxring;
			
		//spin_unlock_irqrestore(&priv->irq_lock,flags);	
			

		priv->rxbuffer=(priv->rxbuffer->next);

	}
	
	
	
//	if(get_curr_tx_free_desc(dev,priority))
//	ieee80211_sta_ps_sleep(priv->ieee80211, &tmp, &tmp2);
	
	
	
}


void rtl8180_dma_kick(struct net_device *dev, int priority)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	
	switch(priority){
	
		case LOW_PRIORITY:
		
		write_nic_byte(dev,TX_DMA_POLLING,
		       (1<< TX_DMA_POLLING_LOWPRIORITY_SHIFT) |
			        priv->dma_poll_mask);
		break;
		
		case NORM_PRIORITY:
		
		write_nic_byte(dev,TX_DMA_POLLING,
		       (1<< TX_DMA_POLLING_NORMPRIORITY_SHIFT) |
			        priv->dma_poll_mask);
		break;
		
		case HI_PRIORITY:
		
		write_nic_byte(dev,TX_DMA_POLLING,
		       (1<< TX_DMA_POLLING_HIPRIORITY_SHIFT) |
			        priv->dma_poll_mask);
		break;
	
	}
	
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
	
	force_pci_posting(dev);
}

#if 0
void rtl8180_tx_queues_stop(struct net_device *dev)
{
	//struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u8 dma_poll_mask = (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_HIPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_NORMPRIORITY_SHIFT);
	dma_poll_mask |= (1<<TX_DMA_STOP_BEACON_SHIFT);

	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,dma_poll_mask);
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
}
#endif

void rtl8180_data_hard_stop(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
}


void rtl8180_data_hard_resume(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask &= ~(1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
}


/* this function TX data frames when the ieee80211 stack requires this.
 * It checks also if we need to stop the ieee tx queue, eventually do it
 */
void rtl8180_hard_data_xmit(struct sk_buff *skb,struct net_device *dev, int
rate)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	int mode;
	struct ieee80211_hdr_3addr  *h = (struct ieee80211_hdr_3addr  *) skb->data;
	short morefrag = (h->frame_ctl) & IEEE80211_FCTL_MOREFRAGS;
	unsigned long flags;
	
	mode = priv->ieee80211->iw_mode;
	
	rate = ieeerate2rtlrate(rate);
	/*
	* This function doesn't require lock because we make
	* sure it's called with the tx_lock already acquired.
	* this come from the kernel's hard_xmit callback (trought
	* the ieee stack, or from the try_wake_queue (again trought
	* the ieee stack.
	*/
	spin_lock_irqsave(&priv->tx_lock,flags);	
			
	
	if (!check_nic_enought_desc(dev, LOW_PRIORITY)){
		DMESG("Error: no descriptor left by previous TX (avail %d) ",
			get_curr_tx_free_desc(dev, LOW_PRIORITY));
		ieee80211_stop_queue(priv->ieee80211);
	}
	rtl8180_tx(dev, (u32*)skb->data, skb->len, LOW_PRIORITY, morefrag,0,rate);
	if (!check_nic_enought_desc(dev, LOW_PRIORITY))
		ieee80211_stop_queue(priv->ieee80211);
	
	//dev_kfree_skb_any(skb);	
	spin_unlock_irqrestore(&priv->tx_lock,flags);	
			
}

/* This is a rough attempt to TX a frame
 * This is called by the ieee 80211 stack to TX management frames.
 * If the ring is full packet are dropped (for data frame the queue
 * is stopped before this can happen). For this reason it is better
 * if the descriptors are larger than the largest management frame
 * we intend to TX: i'm unsure what the HW does if it will not found
 * the last fragment of a frame because it has been dropped...
 * Since queues for Management and Data frames are different we
 * might use a different lock than tx_lock (for example mgmt_tx_lock)
 */
/* these function may loops if invoked with 0 descriptors or 0 len buffer*/
int rtl8180_hard_start_xmit(struct sk_buff *skb,struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);

	unsigned long flags;
	
	spin_lock_irqsave(&priv->tx_lock,flags);
	
	rtl8180_tx(dev, (u32*)skb->data, skb->len, NORM_PRIORITY, 
		0, 0,ieeerate2rtlrate(priv->ieee80211->basic_rate));
	
	priv->ieee80211->stats.tx_bytes+=skb->len;
	priv->ieee80211->stats.tx_packets++;
	spin_unlock_irqrestore(&priv->tx_lock,flags);	
	
	dev_kfree_skb_any(skb);
	return 0;	
}

// longpre 144+48 shortpre 72+24
u16 rtl8180_len2duration(u32 len, short rate,short* ext)
{
	u16 duration;
	u16 drift;
	*ext=0;
	
	switch(rate){
	case 0://1mbps
		*ext=0;
		duration = ((len+4)<<4) /0x2;
		drift = ((len+4)<<4) % 0x2;
		if(drift ==0 ) break;
		duration++;
		break;
		
	case 1://2mbps
		*ext=0;
		duration = ((len+4)<<4) /0x4;
		drift = ((len+4)<<4) % 0x4;
		if(drift ==0 ) break;
		duration++;
		break;
		
	case 2: //5.5mbps
		*ext=0;
		duration = ((len+4)<<4) /0xb;
		drift = ((len+4)<<4) % 0xb;
		if(drift ==0 ) 
			break;
		duration++;
		break;
		
	default:
	case 3://11mbps				
		*ext=0;
		duration = ((len+4)<<4) /0x16;
		drift = ((len+4)<<4) % 0x16;
		if(drift ==0 ) 
			break;
		duration++;
		if(drift > 6) 
			break;
		*ext=1;
		break;
	}
	
	return duration;
}


void rtl8180_prepare_beacon(struct net_device *dev)
{

	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	struct sk_buff *skb;
	
	u16 word  = read_nic_word(dev, BcnItv);
	word &= ~BcnItv_BcnItv; // clear Bcn_Itv
	word |= cpu_to_le16(priv->ieee80211->current_network.beacon_interval);//0x64;	
	write_nic_word(dev, BcnItv, word);
	
	
	skb = ieee80211_get_beacon(priv->ieee80211);
	if(skb){
		rtl8180_tx(dev,(u32*)skb->data,skb->len,BEACON_PRIORITY,
			0,0,ieeerate2rtlrate(priv->ieee80211->basic_rate));
		dev_kfree_skb_any(skb);
	}
	#if 0
	//DMESG("size %x",len);
	if(*tail & (1<<31)){ 

		//DMESG("No more beacon TX desc");
		return ;	
		
	}
	//while(! *tail & (1<<31)){
		*tail= 0; // zeroes header
		
		*tail = *tail| (1<<29) ; //fist segment of the packet
		*tail = (*tail) | (1<<28); // last segment
	//	*tail = *tail | (1<<18); // this is  a beacon frame
		*(tail+3)=*(tail+3) &~ 0xfff; 
		*(tail+3)=*(tail+3) | len; // buffer lenght
		*tail = *tail |len; 
		// zeroes the second 32-bits dword of the descriptor
		*(tail+1)= 0; 
		*tail = *tail | (rate << 24);
		
			duration = rtl8180_len2duration(len,rate,&ext);
		
		*(tail+1) = *(tail+1) | ((duration & 0x7fff)<<16);
		
		*tail = *tail | (1<<31); 
		//^ descriptor ready to be txed
		if((tail - begin)/8 == priv->txbeaconcount-1)
			tail=begin; 
		else
			tail=tail+8;
	//}
#endif
}

/* This function do the real dirty work: it enqueues a TX command 
 * descriptor in the ring buffer, copyes the frame in a TX buffer  
 * and kicks the NIC to ensure it does the DMA transfer.
 */
short rtl8180_tx(struct net_device *dev, u32* txbuf, int len, int priority,
		 short morefrag, short descfrag, int rate)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	u32 *tail;
	u32 *begin;
	u32 *buf;
	int i;
	int remain;
	int buflen;
	int count;
	u16	AckCtsTime;
	u16	FrameTime;
	u16 duration;
	short ext;
	struct buffer* buflist;
	//unsigned long flags;
		
	switch(priority) {
	case LOW_PRIORITY:
		tail=priv->txlpringtail;
		begin=priv->txlpring;
		buflist = priv->txlpbufstail;	
		count = priv->txringcount;    
		break;
		
	case HI_PRIORITY:
		tail=priv->txhpringtail;
		begin=priv->txhpring;
		buflist = priv->txhpbufstail;
		count = priv->txringcount;
		break;
	  
	case NORM_PRIORITY:
		tail=priv->txnpringtail;
		begin=priv->txnpring;
		buflist = priv->txnpbufstail;
		count = priv->txringcount;	    
		break;
	  
	case BEACON_PRIORITY:
		tail=priv->txbeaconringtail;
		begin=priv->txbeaconring;
		buflist = priv->txbeaconbufstail;
		count = priv->txbeaconcount;	    
		break;
	
	default:
		return -1;
		break;
 	}

	buflen=priv->txbuffsize; 
	remain=len;
	while(remain!=0){
#ifdef DEBUG_TX_FRAG
		DMESG("TX iteration");
#endif
#ifdef DEBUG_TX	
		DMESG("TX: filling descriptor %x",(u32)tail);
#endif
		mb();
		if(!buflist){
			DMESGE("TX buffer error, cannot TX frames. pri %d.", priority);
			//spin_unlock_irqrestore(&priv->tx_lock,flags);
			return -1;
		}
		buf=buflist->buf;
		
		if( (*tail & (1<<31)) && (priority != BEACON_PRIORITY)){
		 
				DMESGW("No more TX desc, returning %x of %x",
				remain,len);
				priv->stats.txrdu++;
#ifdef DEBUG_TX_DESC
				check_tx_ring(dev,priority);
			//	netif_stop_queue(dev);
			//	netif_carrier_off(dev);
#endif
			//	spin_unlock_irqrestore(&priv->tx_lock,flags);
			
			return remain;	
							
		}
	
		*tail= 0; // zeroes header 
	
		
		if(priv->card_8185){
			
			//FIXME: this should be triggered by HW encryption parameters.
			*tail |= (1<<15); //no encrypt
			*tail |= (1<<30); //raise int when completed
		}
	//	*tail = *tail | (1<<16);
		if(remain==len && !descfrag) *tail = *tail| (1<<29) ; //fist segment of the packet
		
#ifdef DEBUG_TX_FRAG
		if(remain==len && !descfrag) DMESG("First descriptor");
#endif
		//rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
		
		for(i=0;i<buflen&& remain >0;i++,remain--){
			((u8*)buf)[i]=((u8*)txbuf)[i]; //copy data into descriptor pointed DMAble buffer
			if(remain == 4 && i+4 >= buflen) break; 
			/* ensure the last desc has at least 4 bytes payload */ 
			
		}
		txbuf = (u32*)((u8*)txbuf + i);
		*(tail+3)=*(tail+3) &~ 0xfff; 
		*(tail+3)=*(tail+3) | i; // buffer lenght
		*tail = *tail |(len); 
		// Use short preamble or not
		if (priv->ieee80211->current_network.capability&WLAN_CAPABILITY_SHORT_PREAMBLE)
			if (priv->plcp_preamble_mode==1 && rate!=0)	//  short mode now, not long!
				*tail |= (1<<16);				// enable short preamble mode.
			
		//Use RTS or not
		if ( (len>priv->rts) && priv->rts && priority==LOW_PRIORITY){
			*tail |= (1<<23);	//enalbe RTS function
			*tail |= (0<<19);	//use 1M bps send RTS packet
			AckCtsTime = ComputeTxTime(14, 10,0, 0);	// AckCTSLng = 14 use 1M bps send
			FrameTime = ComputeTxTime(len + 4, rtl8180_rate2rate(rate), 0, *tail&(1<<16));
			// RTS/CTS time is calculate as follow
			duration = FrameTime + 3*10 + 2*AckCtsTime;	//10us is the SifsTime;	
			*(tail+1) |= duration; 	//Need to edit here!  ----hikaru
		}else{
			*(tail+1)= 0; // zeroes the second 32-bits dword of the descriptor
		}
		*tail = *tail | (rate << 24);
		//DMESG("rate %d",rate);
		
		if(priv->card_8185){
			
			#if 0
			*(tail+5)&= ~(1<<24); /* tx ant 0 */ 
			
			*(tail+5) &= ~(1<<23); /* random tx agc 23-16 */
			*(tail+5) |= (1<<22)|(1<<21)|(1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<16);
			
			*(tail+5) &=
~((1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9)|(1<<8));
			*(tail+5) |= (7<<8); // Max retry limit
			
			*(tail+5) &= ~((1<<7)|(1<<6)|(1<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0));
			*(tail+5) |= (8<<4); // Max contention window
			*(tail+6) |= 4; // Min contention window 
			#endif
			*(tail+5) = 0;
		}
		
		/* hw_plcp_len is not used for rtl8180 chip */
		/* FIXME */
		if(priv->card_8185 == 0 || !priv->hw_plcp_len){
		
			duration = rtl8180_len2duration(len,
				rate,&ext);	
	
		
#ifdef DEBUG_TX
			DMESG("PLCP duration %d",duration );
			DMESG("drift %d",drift);
			DMESG("extension %s", (ext==1) ? "on":"off");
#endif
			*(tail+1) = *(tail+1) | ((duration & 0x7fff)<<16);
			if(ext) *(tail+1) = *(tail+1) |(1<<31); //plcp length extension
		}
		
		if(morefrag) *tail = (*tail) | (1<<17); // more fragment
		if(!remain) *tail = (*tail) | (1<<28); // last segment of frame
		
#ifdef DEBUG_TX_FRAG
		if(!remain)DMESG("Last descriptor");
		if(morefrag)DMESG("More frag");
#endif
		wmb();
		*tail = *tail | (1<<31); // descriptor ready to be txed

#ifdef DEBUG_TX_DESC2
		DMESG("%8x %8x %8x %8x %8x %8x %8x %8x", tail[0], tail[1], tail[2], tail[3], 
			tail[4], tail[5], tail[6], tail[7]);
#endif
		
		if((tail - begin)/8 == count-1)
			tail=begin; 
		
		else
			tail=tail+8;
		
		buflist=buflist->next;
		
		mb();
		
		switch(priority) {
			case LOW_PRIORITY:
				priv->txlpringtail=tail;
				priv->txlpbufstail=buflist;
				
				break;
			
			case HI_PRIORITY:
				priv->txhpringtail=tail;
				priv->txhpbufstail = buflist;
				
				break;
				
			case NORM_PRIORITY:
				priv->txnpringtail=tail;
				priv->txnpbufstail=buflist;
					
				break;
			
			case BEACON_PRIORITY:
				/* the HW seems to be happy with the 1st
				 * descriptor filled and the 2nd empty...
				 * So always update descriptor 1 and never
				 * touch 2nd
				 */
			//	priv->txbeaconringtail=tail;
			//	priv->txbeaconbufstail=buflist;
				
				break;
				
		}			
		
		rtl8180_dma_kick(dev,priority);
		
	}
	
	//spin_unlock_irqrestore(&priv->tx_lock,flags);
	
	return 0;
	
}


void rtl8180_irq_rx_tasklet(struct r8180_priv * priv);


void rtl8180_link_change(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	u16 beacon_interval;
		
	struct ieee80211_network *net = &priv->ieee80211->current_network;
//	rtl8180_adapter_start(dev);
	rtl8180_update_msr(dev);

	
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	
	write_nic_dword(dev,BSSID,((u32*)net->bssid)[0]);
	write_nic_word(dev,BSSID+4,((u16*)net->bssid)[2]);
		
	
	beacon_interval  = read_nic_dword(dev,BEACON_INTERVAL);
	beacon_interval &= ~ BEACON_INTERVAL_MASK;
	beacon_interval |= net->beacon_interval;
	write_nic_dword(dev, BEACON_INTERVAL, beacon_interval);
	
	rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
	
		
	/*
	u16 atim = read_nic_dword(dev,ATIM);
	u16 = u16 &~ ATIM_MASK;
	u16 = u16 | beacon->atim;
	*/
#if 0
	if (net->capability & WLAN_CAPABILITY_PRIVACY) {
		if (priv->hw_wep) {
			DMESG("Enabling hardware WEP support");	
			rtl8180_set_hw_wep(dev);
			priv->ieee80211->host_encrypt=0;
			priv->ieee80211->host_decrypt=0;
		}
#ifndef CONFIG_IEEE80211_NOWEP
		else {
			priv->ieee80211->host_encrypt=1;
			priv->ieee80211->host_decrypt=1;
		}
#endif
	}
#ifndef CONFIG_IEEE80211_NOWEP
	else{
		priv->ieee80211->host_encrypt=0;
		priv->ieee80211->host_decrypt=0;
	}
#endif
#endif
	
	
	if(priv->card_8185)
		rtl8180_set_chan(dev, priv->chan);
	

}

void rtl8180_rq_tx_ack(struct net_device *dev){
		
	struct r8180_priv *priv = ieee80211_priv(dev);
	write_nic_byte(dev,CONFIG4,read_nic_byte(dev,CONFIG4)|CONFIG4_PWRMGT);
	priv->ack_tx_to_ieee = 1;
}

short rtl8180_is_tx_queue_empty(struct net_device *dev){
		
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	u32* d;
	for (d = priv->txlpring;
		d < priv->txlpring + priv->txringcount;d+=8)
			if(*d & (1<<31)) return 0;
	
	for (d = priv->txhpring;
		d < priv->txhpring + priv->txringcount;d+=8)
			if(*d & (1<<31)) return 0;
	
	for (d = priv->txnpring;
		d < priv->txnpring + priv->txringcount;d+=8)
			if(*d & (1<<31)) return 0;
	
	return 1;
}
/* FIXME FIXME 5msecs is random */
#define HW_WAKE_DELAY 5

void rtl8180_hw_wakeup(struct net_device *dev)
{
	unsigned long flags;
	
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	spin_lock_irqsave(&priv->ps_lock,flags);
	//DMESG("Waken up!");
	write_nic_byte(dev,CONFIG4,read_nic_byte(dev,CONFIG4)&~CONFIG4_PWRMGT);
	
	if(priv->rf_wakeup)
		priv->rf_wakeup(dev);
	mdelay(HW_WAKE_DELAY);
	spin_unlock_irqrestore(&priv->ps_lock,flags);
}


void rtl8180_hw_sleep(struct net_device *dev, u32 th, u32 tl)
{

	struct r8180_priv *priv = ieee80211_priv(dev);

	u32 rb;
	unsigned long flags;
	
	spin_lock_irqsave(&priv->ps_lock,flags);
	
	/* Writing HW register with 0 equals to disable
	 * the timer, that is not really what we want
	 */
	tl -= HW_WAKE_DELAY*1000;
	 
	if(tl == 0) tl = 1;
	
	/* FIXME HACK FIXME HACK */
	force_pci_posting(dev);
	mdelay(1);
	
	rb = read_nic_dword(dev, TSFTR);
	
	//DMESG("sleep until %x, hw @:%x",tl,rb);
	
	/* If the interval in witch we are requested to sleep is too
	 * short then give up and remain awake
	 */
	if(((rb>tl)&& (tl-rb) < MIN_SLEEP_TIME)
		||((rb>tl)&& (tl-rb) < MIN_SLEEP_TIME)) {
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		return;
	}	
		
	write_nic_dword(dev, TimerInt, tl);
	//DMESG("sleep..");
	rb = read_nic_dword(dev, TSFTR);
	
	/* if we suspect the TimerInt is gone beyond tl 
	 * while setting it, then give up
	 */
	if(((tl > rb) && ((tl-rb) > MAX_SLEEP_TIME))||
		((tl < rb) && ((rb-tl) > MAX_SLEEP_TIME)))
		return;
	
	
	if(priv->rf_sleep)
		priv->rf_sleep(dev);
	spin_unlock_irqrestore(&priv->ps_lock,flags);	
}

#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
void rtl8180_restart_wq(struct net_device *dev);
#else
void rtl8180_restart_wq(struct work_struct *work);
#endif

short rtl8180_init(struct net_device *dev)
{
		
	struct r8180_priv *priv = ieee80211_priv(dev);
	int i, j;
	u16 word;
	int ch;
	u16 version;
	u8 hw_version;
	u8 config3;
	u8 chan_plan_index;
	
	//FIXME: these constants are placed in a bad pleace.

	priv->txbuffsize = 1024;
	priv->txringcount = 32;
	priv->rxbuffersize = 1024;
	priv->rxringcount = 32; 
	priv->txbeaconcount = 2;
	priv->rx_skb_complete = 1;
	//priv->txnp_pending.ispending=0; 
	/* ^^ the SKB does not containt a partial RXed
	 * packet (is empty)
	 */

#if 0	 
	if(!channels){
		DMESG("No channels, aborting");
		return -1;
	}
	ch=channels;
	priv->channel_plan = 0;//hikaru

	 // set channels 1..14 allowed in given locale
	for (i=1; i<=14; i++) {
		(priv->ieee80211->channel_map)[i] = (u8)(ch & 0x01);
		ch >>= 1;
	}
#endif	
	//memcpy(priv->stats,0,sizeof(struct Stats));
	
	priv->irq_enabled=0;
	
	priv->stats.rxdmafail=0;
	priv->stats.txrdu=0;
	priv->stats.rxrdu=0;
	priv->stats.rxnolast=0;
	priv->stats.rxnodata=0;
	//priv->stats.rxreset=0;
	//priv->stats.rxwrkaround=0;
	priv->stats.rxnopointer=0;
	priv->stats.txnperr=0;
	priv->stats.txresumed=0;
	priv->stats.rxerr=0;
	priv->stats.rxoverflow=0;
	priv->stats.rxint=0;
	priv->stats.txnpokint=0;
	priv->stats.txhpokint=0;
	priv->stats.txhperr=0;
	priv->stats.ints=0;
	priv->stats.shints=0;
	priv->stats.txoverflow=0;
	priv->stats.txbeacon=0;
	priv->stats.txbeaconerr=0;
	priv->stats.txlperr=0;
	priv->stats.txlpokint=0;
	priv->stats.txretry=0;//tony 20060601
	priv->stats.rxcrcerrmin=0;
	priv->stats.rxcrcerrmid=0;
	priv->stats.rxcrcerrmax=0;
	priv->stats.rxicverr=0;
	priv->ack_tx_to_ieee = 0;
	priv->ieee80211->current_network.beacon_interval = DEFAULT_BEACONINTERVAL;	
	priv->ieee80211->iw_mode = IW_MODE_INFRA;
	priv->ieee80211->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE;
	priv->ieee80211->active_scan = 1;
	priv->ieee80211->rate = 110; //11 mbps
	priv->ieee80211->modulation = IEEE80211_CCK_MODULATION;
	priv->ieee80211->host_encrypt = 1;
	priv->ieee80211->host_decrypt = 1;
	priv->ieee80211->sta_wake_up = rtl8180_hw_wakeup;
	priv->ieee80211->ps_request_tx_ack = rtl8180_rq_tx_ack;
	priv->ieee80211->enter_sleep_state = rtl8180_hw_sleep;
	priv->ieee80211->ps_is_queue_empty = rtl8180_is_tx_queue_empty;
	
	priv->hw_wep = hwwep;
	priv->prism_hdr=0;
	priv->dev=dev;
	priv->retry_rts = DEFAULT_RETRY_RTS;
	priv->retry_data = DEFAULT_RETRY_DATA;
	
	priv->promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	spin_lock_init(&priv->irq_lock);
	spin_lock_init(&priv->irq_th_lock);
	spin_lock_init(&priv->tx_lock);
	spin_lock_init(&priv->ps_lock);
	sema_init(&priv->wx_sem,1);

	#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
	    INIT_WORK(&priv->reset_wq,(void(*)(void*)) rtl8180_restart_wq,dev);
	#else
        INIT_WORK(&priv->reset_wq,(void(*)(void*))rtl8180_restart_wq);
    #endif
	
	tasklet_init(&priv->irq_rx_tasklet,
		     (void(*)(unsigned long)) rtl8180_irq_rx_tasklet,
		     (unsigned long)priv);
	
	//priv->ieee80211->func = 
	//	kmalloc(sizeof(struct ieee80211_helper_functions),GFP_KERNEL);
	//memset(priv->ieee80211->func, 0,
	  //     sizeof(struct ieee80211_helper_functions));
	
	priv->ieee80211->softmac_hard_start_xmit = rtl8180_hard_start_xmit;
	priv->ieee80211->set_chan = rtl8180_set_chan;
	priv->ieee80211->link_change = rtl8180_link_change;
	priv->ieee80211->softmac_data_hard_start_xmit = rtl8180_hard_data_xmit;
	priv->ieee80211->data_hard_stop = rtl8180_data_hard_stop;
	priv->ieee80211->data_hard_resume = rtl8180_data_hard_resume;
	priv->ieee80211->start_send_beacons = rtl8180_start_tx_beacon;
	priv->ieee80211->stop_send_beacons = rtl8180_beacon_tx_disable;
	priv->ieee80211->fts = DEFAULT_FRAG_THRESHOLD;
	
	hw_version =( read_nic_dword(dev, TCR) & TCR_HWVERID_MASK)>>TCR_HWVERID_SHIFT;
	
	switch (hw_version){
		case HW_VERID_R8185_ABC:
			DMESG("MAC controller is a RTL8185 b/g");	
			priv->card_8185 = 1;
			/* you should not find a card with 8225 PHY ver < C*/
			priv->phy_ver = 2;
			break;
			 
		case HW_VERID_R8185_D:
			DMESG("MAC controller is a RTL8185 b/g (V. D)");	
			priv->card_8185 = 2;
			/* you should not find a card with 8225 PHY ver < C*/
			priv->phy_ver = 2;
			break;
			
		case HW_VERID_R8180_ABCD:
			DMESG("MAC controller is a RTL8180");
			priv->card_8185 = 0;
			break;
		
		case HW_VERID_R8180_F:
			DMESG("MAC controller is a RTL8180 (v. F)");
			priv->card_8185 = 0;
			break;
		
		default:
			DMESGW("MAC chip not recognized: version %x. Assuming RTL8180",hw_version);
			priv->card_8185 = 0;
			break;
	}
	
	if(priv->card_8185){ 
		priv->ieee80211->modulation |= IEEE80211_OFDM_MODULATION;
		priv->ieee80211->short_slot = 1;
	}
	/* you should not found any 8185 Ver B Card */
	priv->card_8185_Bversion = 0;
	
	config3 = read_nic_byte(dev, CONFIG3);
	if(config3 & 0x8){
		priv->card_type = CARDBUS;
		DMESG("This is a CARDBUS NIC");
	}
	else if( config3 & 0x4){
		priv->card_type = MINIPCI;
		DMESG("This is a MINI-PCI NIC");
	}else{
		priv->card_type = PCI;
		DMESG("This is a PCI NIC");
	}
	priv->enable_gpio0 = 0;
	
	
	/* rtl8185 can calc plcp len in HW.*/
	priv->hw_plcp_len = 1;
	
	priv->plcp_preamble_mode = 2;
	/*the eeprom type is stored in RCR register bit #6 */ 
	if (RCR_9356SEL & read_nic_dword(dev, RCR)){
		priv->epromtype=EPROM_93c56;
		DMESG("Reported EEPROM chip is a 93c56 (2Kbit)");
	}else{
		priv->epromtype=EPROM_93c46;
		DMESG("Reported EEPROM chip is a 93c46 (1Kbit)");
	}
	
	dev->get_stats = rtl8180_stats;
	chan_plan_index = eprom_read(dev,EPROM_CHANNEL_PLAN) & 0xff;
	if(chan_plan_index & 0x80) {
		chan_plan_index &= 0x7f;
		if ((DefaultChannelPlan[chan_plan_index].Len != 0)&&(chan_plan_index < (sizeof(DefaultChannelPlan)/sizeof(DefaultChannelPlan[0])))){
			// force channel plan map
			for (i=0;i<DefaultChannelPlan[chan_plan_index].Len;i++) 
				priv->ieee80211->channel_map[DefaultChannelPlan[chan_plan_index].Channel[i]] = 1;
		} else {
			DMESG("No channels, aborting");
			return -1;
		}
	} else {
		//default channel plan setting
		if(!channels){
			DMESG("No channels, aborting");
			return -1;
		}
		ch=channels;
		// set channels 1..14 allowed in given locale
		for (i=1; i<=14; i++) {
			(priv->ieee80211->channel_map)[i] = (u8)(ch & 0x01);
			ch >>= 1;
		}
	}

	dev->dev_addr[0]=eprom_read(dev,MAC_ADR) & 0xff;
	dev->dev_addr[1]=(eprom_read(dev,MAC_ADR) & 0xff00)>>8;
	dev->dev_addr[2]=eprom_read(dev,MAC_ADR+1) & 0xff;
	dev->dev_addr[3]=(eprom_read(dev,MAC_ADR+1) & 0xff00)>>8;
	dev->dev_addr[4]=eprom_read(dev,MAC_ADR+2) & 0xff;
	dev->dev_addr[5]=(eprom_read(dev,MAC_ADR+2) & 0xff00)>>8;
	DMESG("Card MAC address is "MAC_FMT, MAC_ARG(dev->dev_addr));
	
	
	for(i=1,j=0; i<14; i+=2,j++){
		
		word = eprom_read(dev,EPROM_TXPW_CH1_2 + j);
		priv->chtxpwr[i]=word & 0xff;
		priv->chtxpwr[i+1]=(word & 0xff00)>>8;
#ifdef DEBUG_EPROM
		DMESG("tx word %x:%x",j,word);
		DMESG("ch %d pwr %x",i,priv->chtxpwr[i]);
		DMESG("ch %d pwr %x",i+1,priv->chtxpwr[i+1]);
#endif
	}
	if(priv->card_8185){
		for(i=1,j=0; i<14; i+=2,j++){
			
			word = eprom_read(dev,EPROM_TXPW_OFDM_CH1_2 + j);
			priv->chtxpwr_ofdm[i]=word & 0xff;
			priv->chtxpwr_ofdm[i+1]=(word & 0xff00)>>8;
#ifdef DEBUG_EPROM
			DMESG("ofdm tx word %x:%x",j,word);
			DMESG("ofdm ch %d pwr %x",i,priv->chtxpwr_ofdm[i]);
			DMESG("ofdm ch %d pwr %x",i+1,priv->chtxpwr_ofdm[i+1]);
#endif
		}
	}
	
	word = eprom_read(dev,EPROM_TXPW_BASE);
	priv->cck_txpwr_base = word & 0xf;
	priv->ofdm_txpwr_base = (word>>4) & 0xf;
	
	version = eprom_read(dev,EPROM_VERSION);
	DMESG("EEPROM version %x",version);
	if( (!priv->card_8185) && version < 0x0101){
		DMESG ("EEPROM version too old, assuming defaults");
		DMESG ("If you see this message *plase* send your \
DMESG output to andreamrl@tiscali.it THANKS");
		priv->digphy=1;
		priv->antb=0;
		priv->diversity=1;
		priv->cs_treshold=0xc;
		priv->rcr_csense=1;
		priv->rf_chip=RFCHIPID_PHILIPS;
	}else{
		if(!priv->card_8185){
			u8 rfparam = eprom_read(dev,RF_PARAM);
			DMESG("RfParam: %x",rfparam);
			
			priv->digphy = rfparam & (1<<RF_PARAM_DIGPHY_SHIFT) ? 0:1;
			priv->antb =  rfparam & (1<<RF_PARAM_ANTBDEFAULT_SHIFT) ? 1:0;
			
			priv->rcr_csense = (rfparam & RF_PARAM_CARRIERSENSE_MASK) >>
					RF_PARAM_CARRIERSENSE_SHIFT;
					
			priv->diversity = 
				(read_nic_byte(dev,CONFIG2)&(1<<CONFIG2_ANTENNA_SHIFT)) ? 1:0;
		}else{
			priv->rcr_csense = 3;
		}
		
		priv->cs_treshold = (eprom_read(dev,ENERGY_TRESHOLD)&0xff00) >>8;
		
		priv->rf_chip = 0xff & eprom_read(dev,RFCHIPID);
	}
	
	/* check RF frontend chipset */
	
	switch (priv->rf_chip) {
		
		case RFCHIPID_RTL8225:
		case RFCHIPID_RTL8225VF:
		
			if(priv->card_8185){
				DMESG("Card reports RF frontend Realtek 8225");
				DMESGW("This driver has EXPERIMENTAL support for this chipset.");
				DMESGW("use it with care and at your own risk and");
				DMESGW("**PLEASE** REPORT SUCCESS/INSUCCESS TO Realtek");
				if(rtl8225_is_V_z2(dev)){
					priv->rf_close = rtl8225z2_rf_close;
					priv->rf_init = rtl8225z2_rf_init;
					priv->rf_set_chan = rtl8225z2_rf_set_chan;
					priv->rf_set_sens = NULL;
					DMESG("This seems a new V2 radio");
				}else{
					priv->rf_close = rtl8225_rf_close;
					priv->rf_init = rtl8225_rf_init;
					priv->rf_set_chan = rtl8225_rf_set_chan;
					priv->rf_set_sens = NULL;
					priv->rf_sleep = rtl8225_rf_sleep;
					priv->rf_wakeup = rtl8225_rf_wakeup;
					DMESG("This seems a legacy 1st version radio");
				}

			}else{
			DMESGW("Detected RTL8225 radio on a card recognized as RTL8180");
			DMESGW("This could not be... something went wrong....");
			return -ENODEV;
		}
			break;	
	
		case RFCHIPID_RTL8255:
		if(priv->card_8185){
			DMESG("Card reports RF frontend Realtek 8255");
			DMESGW("This driver has EXPERIMENTAL support for this chipset.");
			DMESGW("use it with care and at your own risk and");
			DMESGW("**PLEASE** REPORT SUCCESS/INSUCCESS TO andreamrl@tiscali.it");
			
			priv->rf_close = rtl8255_rf_close;
			priv->rf_init = rtl8255_rf_init;
			priv->rf_set_chan = rtl8255_rf_set_chan;
			priv->rf_set_sens = NULL;
			priv->rf_sleep = NULL;
			priv->rf_wakeup = NULL;
			
		}else{
			DMESGW("Detected RTL8255 radio on a card recognized as RTL8180");
			DMESGW("This could not be... something went wrong....");
			return -ENODEV;
		}
			break;	
	
			
		case RFCHIPID_INTERSIL:
			DMESGW("Card reports RF frontend by Intersil.");
			DMESGW("This driver has NO support for this chipset.");
			return -ENODEV;
			break;
			
		case RFCHIPID_RFMD:
			DMESGW("Card reports RF frontend by RFMD.");
			DMESGW("This driver has NO support for this chipset.");
			return -ENODEV;
			break;
		
		case RFCHIPID_GCT:
			DMESGW("Card reports RF frontend by GCT.");
			DMESGW("This driver has EXPERIMENTAL support for this chipset.");
			DMESGW("use it with care and at your own risk and");
			DMESGW("**PLEASE** REPORT SUCCESS/INSUCCESS TO andreamrl@tiscali.it");
			priv->rf_close = gct_rf_close;
			priv->rf_init = gct_rf_init;
			priv->rf_set_chan = gct_rf_set_chan;
			priv->rf_set_sens = NULL;
			priv->rf_sleep = NULL;
			priv->rf_wakeup = NULL;
			break;
	
		case RFCHIPID_MAXIM:
			DMESGW("Card reports RF frontend by MAXIM.");
			DMESGW("This driver has EXPERIMENTAL support for this chipset.");
			DMESGW("use it with care and at your own risk and");
			DMESGW("**PLEASE** REPORT SUCCESS/INSUCCESS TO andreamrl@tiscali.it");
			priv->rf_close = maxim_rf_close;
			priv->rf_init = maxim_rf_init;
			priv->rf_set_chan = maxim_rf_set_chan;
			priv->rf_set_sens = NULL;
			priv->rf_sleep = NULL;
			priv->rf_wakeup = NULL;
			break;
			
		case RFCHIPID_PHILIPS:
			DMESG("Card reports RF frontend by Philips.");
			DMESG("OK! Philips SA2400 radio chipset is supported.");
			priv->rf_close = sa2400_rf_close;
			priv->rf_init = sa2400_rf_init;
			priv->rf_set_chan = sa2400_rf_set_chan;
			priv->rf_set_sens = sa2400_rf_set_sens;
			priv->sens = SA2400_RF_DEF_SENS; /* default sensitivity */
			priv->max_sens = SA2400_RF_MAX_SENS; /* maximum sensitivity */
			priv->rf_sleep = NULL;
			priv->rf_wakeup = NULL;
		
			if(priv->digphy){ 
				DMESGW("Digital PHY found");
				DMESGW("Philips DIGITAL PHY is untested! *Please*\
	report success/failure to <andreamrl@tiscali.it>");
			}else{ 
				DMESG ("Analog PHY found");
			}
			
			break;	
			
		default:
			DMESGW("Unknown RF module %x",priv->rf_chip);
			DMESGW("Exiting...");
			return -1;
		
	}
	
	DMESG("Energy threshold: %x",priv->cs_treshold);
	DMESG("PAPE from CONFIG2: %x",read_nic_byte(dev,CONFIG2)&0x7);

	if(!priv->card_8185){
		if(priv->antb) 
			DMESG ("Antenna B is default antenna");
		else 
			DMESG ("Antenna A is default antenna");
				
		if(priv->diversity) 
			DMESG ("Antenna diversity is enabled");
		else 
			DMESG("Antenna diversity is disabled");
		
		DMESG("Carrier sense %d",priv->rcr_csense);
	}
	
	if (0!=alloc_rx_desc_ring(dev, priv->rxbuffersize, priv->rxringcount)) 
		return -ENOMEM;
	
	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txringcount,
				  TX_NORMPRIORITY_RING_ADDR))
		return -ENOMEM;

	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txringcount,
				  TX_HIGHPRIORITY_RING_ADDR))
		return -ENOMEM;

	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txringcount,
				  TX_LOWPRIORITY_RING_ADDR))
		return -ENOMEM;
	
	if (0!=alloc_tx_desc_ring(dev, priv->txbuffsize, priv->txbeaconcount,
				  TX_BEACON_RING_ADDR))
		return -ENOMEM;
	
		
	//priv->beacon_buf=NULL;
	
	if(!priv->card_8185){
	
		if(read_nic_byte(dev, CONFIG0) & (1<<CONFIG0_WEP40_SHIFT)) 
			DMESG ("40-bit WEP is supported in hardware");
		else 
			DMESG ("40-bit WEP is NOT supported in hardware");
		
		if(read_nic_byte(dev,CONFIG0) & (1<<CONFIG0_WEP104_SHIFT)) 
			DMESG ("104-bit WEP is supported in hardware");
		else 
			DMESG ("104-bit WEP is NOT supported in hardware");
	}
	if(request_irq(dev->irq, rtl8180_interrupt, SA_SHIRQ, dev->name, dev)){
		DMESGE("Error allocating IRQ %d",dev->irq);
		return -1;
	}else{ 
		priv->irq=dev->irq;
		DMESG("IRQ %d",dev->irq);
	}
	
#ifdef DEBUG_EPROM
	dump_eprom(dev);
#endif 
	//rtl8180_disassociate(dev);
	
	return 0;

}


void rtl8180_no_hw_wep(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	if(!priv->card_8185)
	{
		u8 security;
	
		security  = read_nic_byte(dev, SECURITY);
		security &=~(1<<SECURITY_WEP_TX_ENABLE_SHIFT);
		security &=~(1<<SECURITY_WEP_RX_ENABLE_SHIFT);
	
		write_nic_byte(dev, SECURITY, security);
	
	}else{
	
		//FIXME!!!
	}
	/*	
	  write_nic_dword(dev,TX_CONF,read_nic_dword(dev,TX_CONF) | 
	  (1<<TX_NOICV_SHIFT) );
	*/
//	priv->ieee80211->hw_wep=0;
}


void rtl8180_set_hw_wep(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	u8 pgreg;
	u8 security;
	u32 key0_word4;
	
	pgreg=read_nic_byte(dev, PGSELECT);
	write_nic_byte(dev, PGSELECT, pgreg &~ (1<<PGSELECT_PG_SHIFT));
	
	key0_word4 = read_nic_dword(dev, KEY0+4+4+4);
	key0_word4 &= ~ 0xff;
	key0_word4 |= priv->key0[3]& 0xff;
	write_nic_dword(dev,KEY0,(priv->key0[0]));
	write_nic_dword(dev,KEY0+4,(priv->key0[1]));
	write_nic_dword(dev,KEY0+4+4,(priv->key0[2]));
	write_nic_dword(dev,KEY0+4+4+4,(key0_word4));
	
	/*
	  TX_CONF,read_nic_dword(dev,TX_CONF) &~(1<<TX_NOICV_SHIFT));
	*/
	
	security  = read_nic_byte(dev,SECURITY);
	security |= (1<<SECURITY_WEP_TX_ENABLE_SHIFT);
	security |= (1<<SECURITY_WEP_RX_ENABLE_SHIFT);
	security &= ~ SECURITY_ENCRYP_MASK;
	security |= (SECURITY_ENCRYP_104<<SECURITY_ENCRYP_SHIFT);
	
	write_nic_byte(dev, SECURITY, security);
	
	DMESG("key %x %x %x %x",read_nic_dword(dev,KEY0+4+4+4),
	      read_nic_dword(dev,KEY0+4+4),read_nic_dword(dev,KEY0+4),
	      read_nic_dword(dev,KEY0));

	//priv->ieee80211->hw_wep=1;
}


void rtl8185_rf_pins_enable(struct net_device *dev)
{
//	u16 tmp;
//	tmp = read_nic_word(dev, RFPinsEnable);
	write_nic_word(dev, RFPinsEnable, 0x1fff);// | tmp);
//	write_nic_word(dev, RFPinsEnable,7 | tmp);
}


void rtl8185_set_anaparam2(struct net_device *dev, u32 a)
{
	u8 conf3;

	rtl8180_set_mode(dev, EPROM_CMD_CONFIG);

	conf3 = read_nic_byte(dev, CONFIG3);
	write_nic_byte(dev, CONFIG3, conf3 | (1<<CONFIG3_ANAPARAM_W_SHIFT));
	write_nic_dword(dev, ANAPARAM2, a);

	conf3 = read_nic_byte(dev, CONFIG3);
	write_nic_byte(dev, CONFIG3, conf3 &~(1<<CONFIG3_ANAPARAM_W_SHIFT));
	rtl8180_set_mode(dev, EPROM_CMD_NORMAL);

}


void rtl8180_set_anaparam(struct net_device *dev, u32 a)
{
	u8 conf3;

	rtl8180_set_mode(dev, EPROM_CMD_CONFIG);

	conf3 = read_nic_byte(dev, CONFIG3);
	write_nic_byte(dev, CONFIG3, conf3 | (1<<CONFIG3_ANAPARAM_W_SHIFT));
	write_nic_dword(dev, ANAPARAM, a);

	conf3 = read_nic_byte(dev, CONFIG3);
	write_nic_byte(dev, CONFIG3, conf3 &~(1<<CONFIG3_ANAPARAM_W_SHIFT));
	rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
}


void rtl8185_tx_antenna(struct net_device *dev, u8 ant)
{
	write_nic_byte(dev, TX_ANTENNA, ant); 
	force_pci_posting(dev);
	mdelay(1);
}	


void rtl8185_write_phy(struct net_device *dev, u8 adr, u32 data)
{
	u8 phyr;
	u32 phyw;
	int i;
	
	adr |= 0x80;
	 
	phyw= ((data<<8) | adr);
#if 1	
	
	write_nic_dword(dev, PHY_ADR, phyw);
	
	//read_nic_dword(dev, PHY_ADR);
	for(i=0;i<10;i++){
		write_nic_dword(dev, PHY_ADR, 0xffffff7f & phyw);
		phyr = read_nic_byte(dev, PHY_READ);
		if(phyr == (data&0xff)) break;
			
	}
#else
	// Note that, we must write 0xff7c after 0x7d-0x7f to write BB register. 
	write_nic_byte(dev, 0x7f, ((phyw & 0xff000000) >> 24));
	write_nic_byte(dev, 0x7e, ((phyw & 0x00ff0000) >> 16));
	write_nic_byte(dev, 0x7d, ((phyw & 0x0000ff00) >> 8));
	write_nic_byte(dev, 0x7c, ((phyw & 0x000000ff) ));
#endif
	/* this is ok to fail when we write AGC table. check for AGC table might be
	 * done by masking with 0x7f instead of 0xff
	 */
	//if(phyr != (data&0xff)) DMESGW("Phy write timeout %x %x %x", phyr, data,adr);
}


inline void write_phy_ofdm (struct net_device *dev, u8 adr, u32 data)
{
	data = data & 0xff;
	rtl8185_write_phy(dev, adr, data);
}


void write_phy_cck (struct net_device *dev, u8 adr, u32 data)
{
	data = data & 0xff;
	rtl8185_write_phy(dev, adr, data | 0x10000);
}


/* 70*3 = 210 ms  
 * I hope this is enougth
 */ 
#define MAX_PHY 70 
void write_phy(struct net_device *dev, u8 adr, u8 data)
{
	u32 phy;
	int i;
	
	phy = 0xff0000;
	phy |= adr;
	phy |= 0x80; /* this should enable writing */
	phy |= (data<<8);

	//PHY_ADR, PHY_R and PHY_W  are contig and treated as one dword
	write_nic_dword(dev,PHY_ADR, phy); 
	
	phy= 0xffff00;
	phy |= adr;
	
	write_nic_dword(dev,PHY_ADR, phy);
	for(i=0;i<MAX_PHY;i++){
		phy=read_nic_dword(dev,PHY_ADR);
		phy= phy & 0xff0000;
		phy= phy >> 16;
		if(phy == data){ //SUCCESS!
			force_pci_posting(dev);
			mdelay(3); //random value
#ifdef DEBUG_BB
			DMESG("Phy wr %x,%x",adr,data);
#endif
			return;
		}else{
			force_pci_posting(dev);
			mdelay(3); //random value
		}
	}
	DMESGW ("Phy writing %x %x failed!", adr,data);
}

void rtl8185_set_rate(struct net_device *dev)
{
	int i;
	u16 word;
	int basic_rate,min_rr_rate,max_rr_rate;
	
//	struct r8180_priv *priv = ieee80211_priv(dev);
	
	//if (ieee80211_is_54g(priv->ieee80211->current_network) && 
//		priv->ieee80211->state == IEEE80211_LINKED){
	basic_rate = ieeerate2rtlrate(240);
	min_rr_rate = ieeerate2rtlrate(60);
	max_rr_rate = ieeerate2rtlrate(240);
	
//	
//	}else{
//		basic_rate = ieeerate2rtlrate(20);
//		min_rr_rate = ieeerate2rtlrate(10);
//		max_rr_rate = ieeerate2rtlrate(110);
//	}

	write_nic_byte(dev, RESP_RATE,
			max_rr_rate<<MAX_RESP_RATE_SHIFT| min_rr_rate<<MIN_RESP_RATE_SHIFT);

	word  = read_nic_word(dev, BRSR);
	word &= ~BRSR_MBR_8185;
		

	for(i=0;i<=basic_rate;i++)
		word |= (1<<i);

	write_nic_word(dev, BRSR, word);
	//DMESG("RR:%x BRSR: %x", read_nic_byte(dev,RESP_RATE),read_nic_word(dev,BRSR));
}


void rtl8180_adapter_start(struct net_device *dev)
{
        struct r8180_priv *priv = ieee80211_priv(dev);
	u32 anaparam;
	u16 word;
	u8 config3;
//	int i;
	
	rtl8180_rtx_disable(dev);
	rtl8180_reset(dev);
	
	/* seems that 0xffff or 0xafff will cause 
	 * HW interrupt line crash
	 */
	
	//priv->irq_mask = 0xafff;
//	priv->irq_mask = 0x4fcf;

	/* enable beacon timeout, beacon TX ok and err
	 * LP tx ok and err, HP TX ok and err, NP TX ok and err,
	 * RX ok and ERR, and GP timer */
	priv->irq_mask = 0x6fcf;
	
	priv->dma_poll_mask = 0;

	rtl8180_beacon_tx_disable(dev);
	
	if(priv->card_type == CARDBUS ){
		config3=read_nic_byte(dev, CONFIG3);
		write_nic_byte(dev,CONFIG3,config3 | CONFIG3_FuncRegEn);
		write_nic_word(dev,FEMR, FEMR_INTR | FEMR_WKUP | FEMR_GWAKE |
			read_nic_word(dev, FEMR));
	}
	rtl8180_set_mode(dev, EPROM_CMD_CONFIG);
	write_nic_dword(dev, MAC0, ((u32*)dev->dev_addr)[0]);
	write_nic_word(dev, MAC4, ((u32*)dev->dev_addr)[1] & 0xffff );
	rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
	
	rtl8180_update_msr(dev);
	
	if(!priv->card_8185){
		anaparam  = eprom_read(dev,EPROM_ANAPARAM_ADDRLWORD);
		anaparam |= eprom_read(dev,EPROM_ANAPARAM_ADDRHWORD)<<16;
		
		rtl8180_set_anaparam(dev,anaparam);
	}
	/* These might be unnecessary since we do in rx_enable / tx_enable */
	fix_rx_fifo(dev);
	fix_tx_fifo(dev);
	/*set_nic_rxring(dev);
	  set_nic_txring(dev);*/
	
	rtl8180_set_mode(dev, EPROM_CMD_CONFIG);
	
	/* 
	   The following is very strange. seems to be that 1 means test mode, 
	   but we need to acknolwledges the nic when a packet is ready 
	   altought we set it to 0 
	*/
	
	write_nic_byte(dev,
		       CONFIG2, read_nic_byte(dev,CONFIG2) &~\
		       (1<<CONFIG2_DMA_POLLING_MODE_SHIFT)); 
	//^the nic isn't in test mode
	if(priv->card_8185)
			write_nic_byte(dev,
		       CONFIG2, read_nic_byte(dev,CONFIG2)|(1<<4)); 
			
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
	
	write_nic_dword(dev,INT_TIMEOUT,0);
#ifdef DEBUG_REGISTERS
	rtl8180_dump_reg(dev);	
#endif
	
	if(!priv->card_8185)
	{
		/* 
		experimental - this might be needed to calibrate AGC, 
		anyway it shouldn't hurt 
		*/	
		write_nic_byte(dev, CONFIG5,
			read_nic_byte(dev, CONFIG5) | (1<<AGCRESET_SHIFT));
		read_nic_byte(dev, CONFIG5);
		udelay(15);
		write_nic_byte(dev, CONFIG5,
			read_nic_byte(dev, CONFIG5) &~ (1<<AGCRESET_SHIFT));
	}else{
	
		write_nic_byte(dev, WPA_CONFIG, 0);
		//write_nic_byte(dev, TESTR, 0xd);
	}
	
	rtl8180_no_hw_wep(dev);
	
	if(priv->card_8185){
		rtl8185_set_rate(dev);
		write_nic_byte(dev, RATE_FALLBACK, 0x81);
	//	write_nic_byte(dev, 0xdf, 0x15);
	}else{
		word  = read_nic_word(dev, BRSR);
		word &= ~BRSR_MBR; 
		word &= ~BRSR_BPLCP;
		word |= ieeerate2rtlrate(priv->ieee80211->basic_rate);
		write_nic_word(dev, BRSR, word);
	}
	
	
	if(priv->card_8185){
		write_nic_byte(dev, GP_ENABLE,read_nic_byte(dev, GP_ENABLE) & ~(1<<6)); 
	
		//FIXME cfg 3 ClkRun enable - isn't it ReadOnly ?
		rtl8180_set_mode(dev, EPROM_CMD_CONFIG);
		write_nic_byte(dev,CONFIG3, read_nic_byte(dev, CONFIG3)
|(1<<CONFIG3_CLKRUN_SHIFT));
		rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
		
	}
	
	priv->rf_init(dev);
	
	if(priv->rf_set_sens != NULL)
		priv->rf_set_sens(dev,priv->sens);	
	rtl8180_irq_enable(dev);
	
	netif_start_queue(dev);
	/*DMESG ("lfree %d",get_curr_tx_free_desc(dev,LOW_PRIORITY));
	
	DMESG ("nfree %d",get_curr_tx_free_desc(dev,NORM_PRIORITY));
	
	DMESG ("hfree %d",get_curr_tx_free_desc(dev,HI_PRIORITY));
	if(check_nic_enought_desc(dev,NORM_PRIORITY)) DMESG("NORM OK");
	if(check_nic_enought_desc(dev,HI_PRIORITY)) DMESG("HI OK");
	if(check_nic_enought_desc(dev,LOW_PRIORITY)) DMESG("LOW OK");*/
}



/* this configures registers for beacon tx and enables it via
 * rtl8180_beacon_tx_enable(). rtl8180_beacon_tx_disable() might
 * be used to stop beacon transmission
 */
void rtl8180_start_tx_beacon(struct net_device *dev)
{
//	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	u16 word;	
//	DMESG("ring %x %x", priv->txlpringdma,read_nic_dword(dev,TLPDA));

	DMESG("Enabling beacon TX");
	//write_nic_byte(dev, 0x42,0xe6);// TCR
//	set_nic_txring(dev);
//	fix_tx_fifo(dev);
	rtl8180_prepare_beacon(dev);
	rtl8180_irq_disable(dev);
	rtl8180_beacon_tx_enable(dev);
#if 0
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	//write_nic_byte(dev,0x9d,0x20); //DMA Poll
	//write_nic_word(dev,0x7a,0);
	//write_nic_word(dev,0x7a,0x8000);

#if 0
	word  = read_nic_word(dev, BcnItv);
	word &= ~BcnItv_BcnItv; // clear Bcn_Itv
	word |= priv->ieee80211->current_network.beacon_interval;//0x64;	
	write_nic_word(dev, BcnItv, word);
#endif
#endif	
	word = read_nic_word(dev, AtimWnd) &~ AtimWnd_AtimWnd;
	write_nic_word(dev, AtimWnd,word);// word |=
//priv->ieee80211->current_network.atim_window);
	
	word  = read_nic_word(dev, BintrItv);
	word &= ~BintrItv_BintrItv;
	word |= 1000;/*priv->ieee80211->current_network.beacon_interval * 
		((priv->txbeaconcount > 1)?(priv->txbeaconcount-1):1);
	// FIXME: check if correct ^^ worked with 0x3e8;
	*/
	write_nic_word(dev, BintrItv, word);
	
		
	rtl8180_set_mode(dev, EPROM_CMD_NORMAL);
	
//	rtl8180_beacon_tx_enable(dev);
	
	rtl8180_irq_enable(dev);
	
	/* VV !!!!!!!!!! VV*/
	/*
	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,0x9d,0x00); 	
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
*/				
//	DMESG("ring %x %x", priv->txlpringdma,read_nic_dword(dev,TLPDA));

}



/***************************************************************************
    -------------------------------NET STUFF---------------------------
***************************************************************************/
static struct net_device_stats *rtl8180_stats(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	
	return &priv->ieee80211->stats;
}


int _rtl8180_up(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	priv->up=1;
	
	DMESG("Bringing up iface");
	
	rtl8180_adapter_start(dev);
	
	rtl8180_rx_enable(dev);
	rtl8180_tx_enable(dev);
	
	ieee80211_softmac_start_protocol(priv->ieee80211);
	
	return 0;
}


int rtl8180_open(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);
	ret = rtl8180_up(dev);
	up(&priv->wx_sem);
	return ret;
	
}


int rtl8180_up(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	if (priv->up == 1) return -1;
	
	return _rtl8180_up(dev);
}


int rtl8180_close(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);
	ret = rtl8180_down(dev);
	up(&priv->wx_sem);
	
	return ret;

}

int rtl8180_down(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	if (priv->up == 0) return -1;
	
	priv->up=0;
	
	/* FIXME */
	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);
	
	rtl8180_rtx_disable(dev);
	rtl8180_irq_disable(dev);
	
	ieee80211_softmac_stop_protocol(priv->ieee80211);
	
	return 0;
}

#if(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8180_restart_wq(struct work_struct *work)
{
     struct r8180_priv*priv = container_of(work,struct r8180_priv,reset_wq);
     struct net_device *dev = priv->dev;
#else
void rtl8180_restart_wq(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
#endif	
	
	down(&priv->wx_sem);
	
	rtl8180_commit(dev);
	
	up(&priv->wx_sem);
}

void rtl8180_restart(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	//rtl8180_commit(dev);
	schedule_work(&priv->reset_wq);
	//DMESG("TXTIMEOUT");
}


void rtl8180_commit(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);

	if (priv->up == 0) return ;
		
	ieee80211_softmac_stop_protocol(priv->ieee80211);
	rtl8180_irq_disable(dev);
	rtl8180_rtx_disable(dev);
	_rtl8180_up(dev);
}


static void r8180_set_multicast(struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	short promisc;

	//down(&priv->wx_sem);
	
	promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	
	if (promisc != priv->promisc)
		rtl8180_restart(dev);
	
	priv->promisc = promisc;
	
	//up(&priv->wx_sem);
}

#if 0
/* this is called by the kernel when it needs to TX a 802.3 encapsulated frame*/
int rtl8180_8023_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	int ret;
	unsigned long flags;
	
	spin_lock_irqsave(&priv->tx_lock,flags);
	ret = ieee80211_r8180_8023_hardstartxmit(skb,priv->ieee80211);
	spin_unlock_irqrestore(&priv->tx_lock,flags);
	return ret;
}
#endif

int r8180_set_mac_adr(struct net_device *dev, void *mac)
{
	struct r8180_priv *priv = ieee80211_priv(dev);
	struct sockaddr *addr = mac;
	
	down(&priv->wx_sem);
	
	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
	
	if(priv->ieee80211->iw_mode == IW_MODE_MASTER)
		memcpy(priv->ieee80211->current_network.bssid, dev->dev_addr, ETH_ALEN);
	
	if (priv->up) {
		rtl8180_down(dev);
		rtl8180_up(dev);
	}
	
	up(&priv->wx_sem);
	
	return 0;
}

/* based on ipw2200 driver */
int rtl8180_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	struct iwreq *wrq = (struct iwreq *) rq;
	int ret=-1;
	switch (cmd) {
	    case RTL_IOCTL_WPA_SUPPLICANT:
		ret = ieee80211_wpa_supplicant_ioctl(priv->ieee80211, &wrq->u.data);
		return ret;

	    default:
		return -EOPNOTSUPP;
	}

	return -EOPNOTSUPP;
}



/****************************************************************************
     -----------------------------PCI STUFF---------------------------
*****************************************************************************/


static int __devinit rtl8180_pci_probe(struct pci_dev *pdev, 
				       const struct pci_device_id *id)
{
	unsigned long ioaddr = 0;
	struct net_device *dev = NULL;
	struct r8180_priv *priv= NULL;
	
#ifdef CONFIG_RTL8180_IO_MAP
	unsigned long pio_start, pio_len, pio_flags;
#else
	unsigned long pmem_start, pmem_len, pmem_flags;
#endif //end #ifdef RTL_IO_MAP
	
	DMESG("Configuring chip resources");
	
	if( pci_enable_device (pdev) ){
		DMESG("Failed to enable PCI device");
		return -EIO;
	}
	
	pci_set_master(pdev);
	//pci_set_wmi(pdev);
	pci_set_dma_mask(pdev, 0xffffff00ULL);
	pci_set_consistent_dma_mask(pdev,0xffffff00ULL);
	dev = alloc_ieee80211(sizeof(struct r8180_priv));
	if (!dev)
		return -ENOMEM;
	priv = ieee80211_priv(dev);
	priv->ieee80211 = netdev_priv(dev);
	
	SET_MODULE_OWNER(dev);
	
	pci_set_drvdata(pdev, dev);
	SET_NETDEV_DEV(dev, &pdev->dev);
	
	priv = ieee80211_priv(dev);
//	memset(priv,0,sizeof(struct r8180_priv));
	priv->pdev=pdev;
	
	
#ifdef CONFIG_RTL8180_IO_MAP
	
	pio_start = (unsigned long)pci_resource_start (pdev, 0);
	pio_len = (unsigned long)pci_resource_len (pdev, 0);
	pio_flags = (unsigned long)pci_resource_flags (pdev, 0);
	
	if (!(pio_flags & IORESOURCE_IO)) {
		DMESG("region #0 not a PIO resource, aborting");
		goto fail;
	}
	
	DMESG("IO space @ 0x%08lx", pio_start );
	if( ! request_region( pio_start, pio_len, RTL8180_MODULE_NAME ) ){
		DMESG("request_region failed!");
		goto fail;
	}
	
	ioaddr = pio_start;
	dev->base_addr = ioaddr; // device I/O address
	
#else
	
	pmem_start = pci_resource_start(pdev, 1);
	pmem_len = pci_resource_len(pdev, 1);
	pmem_flags = pci_resource_flags (pdev, 1);
	
	if (!(pmem_flags & IORESOURCE_MEM)) {
		DMESG("region #1 not a MMIO resource, aborting");
		goto fail;
	}
	
	DMESG("Memory mapped space @ 0x%08lx ", pmem_start);
	if( ! request_mem_region(pmem_start, pmem_len, RTL8180_MODULE_NAME)) {
		DMESG("request_mem_region failed!");
		goto fail;
	}
	
	
	ioaddr = (unsigned long)ioremap_nocache( pmem_start, pmem_len);	
	if( ioaddr == (unsigned long)NULL ){
		DMESG("ioremap failed!");
	       // release_mem_region( pmem_start, pmem_len );
		goto fail1;
	}
	
	dev->mem_start = ioaddr; // shared mem start
	dev->mem_end = ioaddr + pci_resource_len(pdev, 0); // shared mem end
	
#endif //end #ifdef RTL_IO_MAP
	
	dev->irq = pdev->irq;
	priv->irq = 0;
	
	dev->open = rtl8180_open;
	dev->stop = rtl8180_close;
	//dev->hard_start_xmit = ieee80211_xmit;
	dev->tx_timeout = rtl8180_restart;
	dev->wireless_handlers = &r8180_wx_handlers_def;
	dev->do_ioctl = rtl8180_ioctl;
	dev->set_multicast_list = r8180_set_multicast;
	dev->set_mac_address = r8180_set_mac_adr;
	#if WIRELESS_EXT >= 12
        #if WIRELESS_EXT < 17
	    dev->get_wireless_stats = r8180_get_wireless_stats;
        #endif
            dev->wireless_handlers = (struct iw_handler_def *)&r8180_wx_handlers_def;
        #endif
	dev->type=ARPHRD_ETHER;
	
	if (dev_alloc_name(dev, ifname) < 0){
                DMESG("Oops: devname already taken! Trying wlan%%d...\n");
		ifname = "wlan%d";
		dev_alloc_name(dev, ifname);
        }
	
	
	if(rtl8180_init(dev)!=0){ 
		DMESG("Initialization failed");
		goto fail1;
	}
	
	netif_carrier_off(dev);
	
	register_netdev(dev);
	
	rtl8180_proc_init_one(dev);
	
	DMESG("Driver probe completed\n");
	return 0;	

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


static void __devexit rtl8180_pci_remove(struct pci_dev *pdev)
{
	struct r8180_priv *priv;
	struct net_device *dev = pci_get_drvdata(pdev);
 	if(dev){
		
		unregister_netdev(dev);
		
		priv=ieee80211_priv(dev);
		
		rtl8180_proc_remove_one(dev);
		rtl8180_down(dev);
		priv->rf_close(dev);
		rtl8180_reset(dev);
		rtl8180_rtx_disable(dev);
		rtl8180_irq_disable(dev);
		mdelay(10);
		//write_nic_word(dev,INTA,read_nic_word(dev,INTA));
		//force_pci_posting(dev);
		//mdelay(10);
		
		if(priv->irq){
			
			DMESG("Freeing irq %d",dev->irq);
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
	
	DMESG("wlan driver removed\n");
}


static int __init rtl8180_pci_module_init(void)
{
	printk(KERN_INFO "\nLinux kernel driver for RTL8180 \
/ RTL8185 based WLAN cards\n");
	printk(KERN_INFO "Copyright (c) 2004-2005, Andrea Merello\n");
	DMESG("Initializing module");
	DMESG("Wireless extensions version %d", WIRELESS_EXT);
	rtl8180_proc_module_init();
        #if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
	if(0!=pci_module_init(&rtl8180_pci_driver))
        #else
        if(0!=pci_register_driver(&rtl8180_pci_driver))
        #endif
	{
		DMESG("No device found");
		/*pci_unregister_driver (&rtl8180_pci_driver);*/
		return -ENODEV;
	}
	return 0;
}


static void __exit rtl8180_pci_module_exit(void)
{
	pci_unregister_driver (&rtl8180_pci_driver);
	rtl8180_proc_module_remove();
	DMESG("Exiting");
}


void rtl8180_try_wake_queue(struct net_device *dev, int pri)
{
	unsigned long flags;
	short enough_desc;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	spin_lock_irqsave(&priv->tx_lock,flags);
	enough_desc = check_nic_enought_desc(dev,pri);
	spin_unlock_irqrestore(&priv->tx_lock,flags);

	if(enough_desc)
		ieee80211_wake_queue(priv->ieee80211);
}

/*****************************************************************************
      -----------------------------IRQ STUFF---------------------------
******************************************************************************/

void rtl8180_tx_isr(struct net_device *dev, int pri,short error)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	
	u32 *tail; //tail virtual addr
	u32 *head; //head virtual addr 
	u32 *begin;//start of ring virtual addr
	u32 *nicv; //nic pointer virtual addr
//	u32 *txdv; //packet just TXed
	u32 nic; //nic pointer physical addr
	u32 nicbegin;// start of ring physical addr
//	short txed;
	unsigned long flag;
	/* physical addr are ok on 32 bits since we set DMA mask*/
	
	int offs;
	int j,i;
	int hd;
	if (error) priv->stats.txretry++; //tony 20060601
	spin_lock_irqsave(&priv->tx_lock,flag);
	switch(pri) {
	case LOW_PRIORITY:
		tail = priv->txlpringtail;
		begin = priv->txlpring;
		head = priv->txlpringhead;
		nic = read_nic_dword(dev,TX_LOWPRIORITY_RING_ADDR);
		nicbegin = priv->txlpringdma;
		break;
		
	case HI_PRIORITY:
		tail = priv->txhpringtail;
		begin = priv->txhpring;
		head = priv->txhpringhead;
		nic = read_nic_dword(dev,TX_HIGHPRIORITY_RING_ADDR);
		nicbegin = priv->txhpringdma;
		break;
	  
	case NORM_PRIORITY:
		tail = priv->txnpringtail;
		begin = priv->txnpring;
		head = priv->txnpringhead;	
		nic = read_nic_dword(dev,TX_NORMPRIORITY_RING_ADDR);    
		nicbegin = priv->txnpringdma;
		break;
	default:
		
		spin_unlock_irqrestore(&priv->tx_lock,flag);
		return ;
	}  
/*	DMESG("%x %s %x %x",((int)nic & 0xfff)/8/4, 
	*(priv->txnpring + ((int)nic&0xfff)/4/8) & (1<<31) ? "filled" : "empty",
	(priv->txnpringtail - priv->txnpring)/8,(priv->txnpringhead -
priv->txnpring)/8);
*/	 
	//nicv = (u32*) ((nic - nicbegin) + (int)begin);
	nicv = (u32*) ((nic - nicbegin) + (u8*)begin);
	if((head <= tail && (nicv > tail || nicv < head)) || 
		(head > tail && (nicv > tail && nicv < head))){
		
			DMESGW("nic has lost pointer");
#ifdef DEBUG_TX_DESC
			check_tx_ring(dev,NORM_PRIORITY);
#endif
			
			spin_unlock_irqrestore(&priv->tx_lock,flag);
			rtl8180_restart(dev);
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
	
	hd = (head - begin) /8;
	
	if(offs >= hd)
		j = offs - hd;
	else
		j = offs + (priv->txringcount -1 -hd);
	//	j= priv->txringcount -1- (hd - offs);

	j-=2;
	if(j<0) j=0;
	
	
	for(i=0;i<j;i++)
	{
		
		*head = *head &~ (1<<31);
	
		if((head - begin)/8 == priv->txringcount-1)
			head=begin; 
	
		else
			head+=8;
	}	
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
	
	case LOW_PRIORITY:
		priv->txlpringhead = head;
		break;
				
	case HI_PRIORITY:
		priv->txhpringhead = head;
		break;
	  
	case NORM_PRIORITY:
		priv->txnpringhead = head;
		
		if(priv->ack_tx_to_ieee){	
			if(rtl8180_is_tx_queue_empty(dev)){
				priv->ack_tx_to_ieee = 0;
				ieee80211_ps_tx_ack(priv->ieee80211,!error);
			}
		}
		
		break;
	}
	
	/*DMESG("%x %x %x", (priv->txnpringhead - priv->txnpring) /8 , 
		(priv->txnpringtail - priv->txnpring) /8,
		offs );
	*/
	
	spin_unlock_irqrestore(&priv->tx_lock,flag);
		
}


irqreturn_t rtl8180_interrupt(int irq, void *netdev, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device *) netdev;
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);
	unsigned long flags;
	u16 inta;
	
	/* We should return IRQ_NONE, but for now let me keep this */
	if(priv->irq_enabled == 0) return IRQ_HANDLED;
	
	spin_lock_irqsave(&priv->irq_th_lock,flags);
	
	inta = read_nic_word(dev,INTA);
	priv->stats.shints++;
	
	
	if(!inta){
		
		#if 0
	//	rtl8180_set_mode(dev,
		rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
		write_nic_byte(dev,TX_DMA_POLLING,
		       (1<< TX_DMA_POLLING_LOWPRIORITY_SHIFT) |
		       (1<< TX_DMA_POLLING_NORMPRIORITY_SHIFT) |
		       (1<< TX_DMA_POLLING_HIPRIORITY_SHIFT) |
		        priv->dma_poll_mask);
		
		rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
		//tasklet_schedule(&priv->irq_tx_tasklet);
		ieee80211_r8180_wake_queue(priv->ieee80211);
		#endif
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		return IRQ_HANDLED;  
	/* 
	   most probably we can safely return IRQ_NONE,
	   but for now is better to avoid problems
	*/
	}
	
	if(!(inta & priv->irq_mask)){
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		return IRQ_HANDLED;  
	}  
	
	if(inta == 0xffff){
			/* HW disappared */
			spin_unlock_irqrestore(&priv->irq_th_lock,flags);
			return IRQ_HANDLED;  
	}
	
	write_nic_word(dev,INTA,inta); // reset int situation

	priv->stats.ints++;
#ifdef DEBUG_IRQ
	DMESG("NIC irq %x",inta);
#endif
	//priv->irqpending = inta;
	
	
	if(!netif_running(dev)) {
		spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		return IRQ_HANDLED;
	}
		
	if(inta & ISR_TimeOut){
		write_nic_dword(dev, TimerInt, 0);
		//DMESG("waking up");
		//rtl8180_hw_wakeup(dev);
	}	
	
	if(inta & ISR_TBDOK){
		priv->stats.txbeacon++;
	}
	
	if(inta & ISR_TBDER){
		priv->stats.txbeaconerr++;
	}
	
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
		priv->stats.txhpokint++;
		rtl8180_tx_isr(dev,HI_PRIORITY,0);
	}
	/*
	if(inta & 0x7){ //low priority tx ok
		#ifdef DEBUG_TX
			DMESG ("TX low priority OK");
		#endif	
		priv->freetxlpdesc++;
		if(priv->txlppending){
			priv->txlppending=0;
			wake_up_interruptible(&priv->txlp_queue);
		}
	}
	*/	
	if(inta & ISR_RER) {
		priv->stats.rxerr++;
#ifdef DEBUG_RX
		DMESGW("RX error int");
#endif	
	}
	if(inta & ISR_TNPDER){
		priv->stats.txnperr++;
		priv->ieee80211->stats.tx_errors++;
#ifdef DEBUG_TX
		DMESGW("TX np error int");
#endif
		//tasklet_schedule(&priv->irq_tx_tasklet);
	
		rtl8180_tx_isr(dev,NORM_PRIORITY,1);
		
			
	}
	
	if(inta & ISR_TLPDER){
		priv->stats.txlperr++;
		priv->ieee80211->stats.tx_errors++;
#ifdef DEBUG_TX
		DMESGW("TX lp error int");
#endif
		
		
		rtl8180_tx_isr(dev,LOW_PRIORITY,1);
		//tasklet_schedule(&priv->irq_tx_tasklet);
		
		rtl8180_try_wake_queue(dev, LOW_PRIORITY);
	}
	
	if(inta & ISR_ROK){
#ifdef DEBUG_RX
		DMESG("Frame arrived !");
#endif
		priv->stats.rxint++;
		tasklet_schedule(&priv->irq_rx_tasklet);
	}
	
	if(inta & ISR_BcnInt) {
		//DMESG("Preparing Beacons");
		rtl8180_prepare_beacon(dev);
	}
	
	if(inta & ISR_RDU){
#ifdef DEBUG_RX
		DMESGW("No RX descriptor available");
		priv->stats.rxrdu++;
#endif
		tasklet_schedule(&priv->irq_rx_tasklet);
		/*queue_work(priv->workqueue ,&priv->restart_work);*/
		
	}
	if(inta & ISR_RXFOVW){
#ifdef DEBUG_RX
		DMESGW("RX fifo overflow");
#endif
		priv->stats.rxoverflow++;
		tasklet_schedule(&priv->irq_rx_tasklet);
		//queue_work(priv->workqueue ,&priv->restart_work);
	}
	
	if(inta & ISR_TXFOVW) priv->stats.txoverflow++;
	
	
	if(inta & ISR_TNPDOK){ //Normal priority tx ok
#ifdef DEBUG_TX
		DMESG ("TX normal priority OK");
#endif	
		//	priv->ieee80211->stats.tx_packets++;
		priv->stats.txnpokint++;
		rtl8180_tx_isr(dev,NORM_PRIORITY,0);
		
	}
	
	
	if(inta & ISR_TLPDOK){ //Low priority tx ok
#ifdef DEBUG_TX
		DMESG ("TX low priority OK");
#endif	
		//	priv->ieee80211->stats.tx_packets++;
		priv->stats.txlpokint++;
		rtl8180_tx_isr(dev,LOW_PRIORITY,0);
		rtl8180_try_wake_queue(dev, LOW_PRIORITY);
	}
	
	
	force_pci_posting(dev);
	
	
	spin_unlock_irqrestore(&priv->irq_th_lock,flags);
		
	return IRQ_HANDLED;
}


void rtl8180_irq_rx_tasklet(struct r8180_priv* priv)
{	
//	unsigned long flags;
	
/*	spin_lock_irqsave(&priv->irq_lock, flags);
	priv->irq_mask &=~IMR_ROK;
	priv->irq_mask &=~IMR_RDU;
	
	rtl8180_irq_enable(priv->dev); 
	spin_unlock_irqrestore(&priv->irq_lock, flags);
*/	
	rtl8180_rx(priv->dev);
	
/*	spin_lock_irqsave(&priv->irq_lock, flags);
	priv->irq_mask |= IMR_ROK;
	priv->irq_mask |= IMR_RDU;
	rtl8180_irq_enable(priv->dev); 
	spin_unlock_irqrestore(&priv->irq_lock, flags);
*/	
}



/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/
module_init(rtl8180_pci_module_init);
module_exit(rtl8180_pci_module_exit);
