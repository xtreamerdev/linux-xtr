/* 8139cp.c: A Linux PCI Ethernet driver for the RealTek 8139C+ chips. */
/*
	Copyright 2001-2004 Jeff Garzik <jgarzik@pobox.com>

	Copyright (C) 2001, 2002 David S. Miller (davem@redhat.com) [tg3.c]
	Copyright (C) 2000, 2001 David S. Miller (davem@redhat.com) [sungem.c]
	Copyright 2001 Manfred Spraul				    [natsemi.c]
	Copyright 1999-2001 by D onald Becker.			    [natsemi.c]
       	Written 1997-2001 by Donald Becker.			    [8139too.c]
	Copyright 1998-2001 by Jes Sorensen, <jes@trained-monkey.org>. [acenic.c]

	This software may be used and distributed according to the terms of
	the GNU General Public License (GPL), incorporated herein by reference.
	Drivers based on or derived from this code fall under the GPL and must
	retain the authorship, copyright and license notice.  This file is not
	a complete program and may only be used when the entire operating
	system is licensed under the GPL.

	See the file COPYING in this distribution for more information.

	Contributors:
	
		Wake-on-LAN support - Felipe Damasio <felipewd@terra.com.br>
		PCI suspend/resume  - Felipe Damasio <felipewd@terra.com.br>
		LinkChg interrupt   - Felipe Damasio <felipewd@terra.com.br>
			
	TODO:
	* Test Tx checksumming thoroughly
	* Implement dev->tx_timeout

	Low priority TODO:
	* Complete reset on PciErr
	* Consider Rx interrupt mitigation using TimerIntr
	* Investigate using skb->priority with h/w VLAN priority
	* Investigate using High Priority Tx Queue with skb->priority
	* Adjust Rx FIFO threshold and Max Rx DMA burst on Rx FIFO error
	* Adjust Tx FIFO threshold and Max Tx DMA burst on Tx FIFO error
	* Implement Tx software interrupt mitigation via
	  Tx descriptor bit
	* The real minimum of CP_MIN_MTU is 4 bytes.  However,
	  for this to be supported, one must(?) turn on packet padding.
	* Support external MII transceivers (patch available)

	NOTES:
	* TX checksumming is considered experimental.  It is off by
	  default, use ethtool to turn it on.

 */

#define DRV_NAME		"8139cplus"
#define DRV_VERSION		"1.2"
#define DRV_RELDATE		"Mar 22, 2004"


#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/if_vlan.h>
#include <linux/crc32.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/cache.h>
//#include <linux/byteorder/swab.h> //cyhuang add
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <platform.h>

/* VLAN tagging feature enable/disable */
#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
#define CP_VLAN_TAG_USED 1
#define CP_VLAN_TX_TAG(tx_desc,vlan_tag_value) \
	do { (tx_desc)->opts2 = (vlan_tag_value); } while (0)
#else
#define CP_VLAN_TAG_USED 0
#define CP_VLAN_TX_TAG(tx_desc,vlan_tag_value) \
	do { (tx_desc)->opts2 = 0; } while (0)
#endif

/* These identify the driver base version and may not be removed. */
static char version[] =
KERN_INFO DRV_NAME ": 10/100 PCI Ethernet driver v" DRV_VERSION " (" DRV_RELDATE ")\n";

MODULE_AUTHOR("Jeff Garzik <jgarzik@pobox.com>");
MODULE_DESCRIPTION("RealTek RTL-8139C+ series 10/100 PCI Ethernet driver");
MODULE_LICENSE("GPL");

static int debug = -1;
MODULE_PARM (debug, "i");
MODULE_PARM_DESC (debug, "8139cp: bitmapped message enable number");

/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
   The RTL chips use a 64 element hash table based on the Ethernet CRC.  */
static int multicast_filter_limit = 32;
MODULE_PARM (multicast_filter_limit, "i");
MODULE_PARM_DESC (multicast_filter_limit, "8139cp: maximum number of filtered multicast addresses");

#define PFX			DRV_NAME ": "

#ifndef TRUE
#define FALSE 0
#define TRUE (!FALSE)
#endif

#define CP_DEF_MSG_ENABLE	(NETIF_MSG_DRV		| \
				 NETIF_MSG_PROBE 	| \
				 NETIF_MSG_LINK)
#define CP_NUM_STATS		14	/* struct cp_dma_stats, plus one */
#define CP_STATS_SIZE		64	/* size in bytes of DMA stats block */
#define CP_REGS_SIZE		(0xff + 1)
#define CP_REGS_VER		1		/* version 1 */
#define CP_RX_RING_SIZE		64
#define CP_TX_RING_SIZE		32
#define DESC_ALIGN		0x100
#define UNCACHE_MASK		0xa0000000
#define KSEG_MASK               0xe0000000  

#define CP_RXRING_BYTES	( (sizeof(struct cp_desc) * (CP_RX_RING_SIZE+1)) + DESC_ALIGN)

#define CP_TXRING_BYTES	( (sizeof(struct cp_desc) * (CP_TX_RING_SIZE+1)) + DESC_ALIGN)
		

#define NEXT_TX(N)		(((N) + 1) & (CP_TX_RING_SIZE - 1))
#define NEXT_RX(N)		(((N) + 1) & (CP_RX_RING_SIZE - 1))
#define TX_HQBUFFS_AVAIL(CP)					\
	(((CP)->tx_hqtail <= (CP)->tx_hqhead) ?			\
	  (CP)->tx_hqtail + (CP_TX_RING_SIZE - 1) - (CP)->tx_hqhead :	\
	  (CP)->tx_hqtail - (CP)->tx_hqhead - 1)

#define PKT_BUF_SZ		2048 //1536	/* Size of each temporary Rx buffer.*/
#if 0 //cylee test start
#define RX_OFFSET		2
#else
#define RX_OFFSET		(0)
#endif

#if 0
#define CP_INTERNAL_PHY		1  //cyhuang reserved for chip
#else
#define CP_INTERNAL_PHY		17  //cyhuang reserved for FPGA
#endif
#if 0 //cyhuang reserved
/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024, 7==end of packet. */
#define RX_FIFO_THRESH		5	/* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST		4	/* Maximum PCI burst, '4' is 256 */
#define TX_DMA_BURST		6	/* Maximum PCI burst, '6' is 1024 */
#define TX_EARLY_THRESH		256	/* Early Tx threshold, in bytes */
#endif
/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT		(6*HZ)

/* hardware minimum and maximum for a single frame's data payload */
#define CP_MIN_MTU		60	/* TODO: allow lower, but pad */
#define CP_MAX_MTU		4096

enum PHY_REGS{
	TXFCE= 1<<7,
	RXFCE=1 <<6,
	SP100=1<<3,
	LINK=1<<2,
	TXPF=1<<1,
	RXPF=1<<0,
	FORCE_TX = 1<<5

};
	
enum {
	/* NIC register offsets */
	IDR0 = 0,			/* Ethernet ID */
	IDR1 = 0x1,			/* Ethernet ID */
	IDR2 = 0x2,			/* Ethernet ID */
	IDR3 = 0x3,			/* Ethernet ID */
	IDR4 = 0x4,			/* Ethernet ID */
	IDR5 = 0x5,			/* Ethernet ID */
	MAR0 = 0x8,			/* Multicast register */
	MAR1 = 0x9,
	MAR2 = 0xa,
	MAR3 = 0xb,
	MAR4 = 0xc,
	MAR5 = 0xd,
	MAR6 = 0xe,
	MAR7 = 0xf,
	TXOKCNT=0x10,
	RXOKCNT=0x12,
	TXERR = 0x14,
	RXERRR = 0x16,
	MISSPKT = 0x18,
	FAE=0x1A,
	TX1COL = 0x1c,
	TXMCOL=0x1e,
	RXOKPHY=0x20,
	RXOKBRD=0x22,
	RXOKMUL=0x24,
	TXABT=0x26,
	TXUNDRN=0x28,
	TRSR=0x34,
	CMD=0x3B,
	IMR=0x3C,
	ISR=0x3E,
	TCR=0x40,
	RCR=0x44,
	MSR=0x58,
	MIIAR=0x5C,
	TxFDP1=0x0100,
	TxCDO1=0x0104,
	TXCPO1=0x0108,
	TXPSA1=0x010A,
	TXCPA1=0x010C,
	LastTxCDO1=0x0110,
	TXPAGECNT1=0x0112,
	Tx1ScratchDes=0x0150,
	TxFDP2=0x0180,
	TxCDO2=0x0184,
	TXCPO2=0x0188,
	TXPSA2=0x018A,
	TXCPA2=0x018C,
	LASTTXCDO2=0x0190,
	TXPAGECNT2=0x0192,
	Tx2ScratchDes=0x01A0,
	RxFDP=0x01F0,
	RxCDO=0x01F4,
	RxRingSize=0x01F6,
	RxCPO=0x01F8,
	RxPSA=0x01FA,
	RxCPA=0x1FC,
	RXPLEN=0x200,
	//LASTRXCDO=0x1402,
	RXPFDP=0x0204,
	RXPAGECNT=0x0208,
	RXSCRATCHDES=0x0210,
	EthrntRxCPU_Des_Num=0x0230,
	EthrntRxCPU_Des_Wrap =	0x0231,
	Rx_Pse_Des_Thres = 	0x0232,	
	
	IO_CMD = 0x0234,
	RX_IntNum_Shift = 0x8,             /// ????
	TX_OWN = (1<<5),
	RX_OWN = (1<<4),
	MII_RE = (1<<3),
	MII_TE = (1<<2),
	TX_FNL = (1<1),
	TX_FNH = (1),
	/*TX_START= MII_RE | MII_TE | TX_FNL | TX_FNH,
	TX_START = 0x8113c,*/
	RXMII = MII_RE,
	TXMII= MII_TE,


	


};

enum CP_STATUS_REGS
{
	/* Tx and Rx status descriptors */
	DescOwn		= (1 << 31), /* Descriptor is owned by NIC */
	RingEnd		= (1 << 30), /* End of descriptor ring */
	FirstFrag	= (1 << 29), /* First segment of a packet */
	LastFrag	= (1 << 28), /* Final segment of a packet */
	TxError		= (1 << 23), /* Tx error summary */
	RxError		= (1 << 20), /* Rx error summary */
	IPCS		= (1 << 18), /* Calculate IP checksum */
	UDPCS		= (1 << 17), /* Calculate UDP/IP checksum */
	TCPCS		= (1 << 16), /* Calculate TCP/IP checksum */
	TxVlanTag	= (1 << 17), /* Add VLAN tag */
	RxVlanTagged	= (1 << 16), /* Rx VLAN tag available */
	IPFail		= (1 << 15), /* IP checksum failed */
	UDPFail		= (1 << 14), /* UDP/IP checksum failed */
	TCPFail		= (1 << 13), /* TCP/IP checksum failed */
	NormalTxPoll	= (1 << 6),  /* One or more normal Tx packets to send */
	PID1		= (1 << 17), /* 2 protocol id bits:  0==non-IP, */
	PID0		= (1 << 16), /* 1==UDP/IP, 2==TCP/IP, 3==IP */
	RxProtoTCP	= 1,
	RxProtoUDP	= 2,
	RxProtoIP	= 3,
	TxFIFOUnder	= (1 << 25), /* Tx FIFO underrun */
	TxOWC		= (1 << 22), /* Tx Out-of-window collision */
	TxLinkFail	= (1 << 21), /* Link failed during Tx of packet */
	TxMaxCol	= (1 << 20), /* Tx aborted due to excessive collisions */
	TxColCntShift	= 16,	     /* Shift, to get 4-bit Tx collision cnt */
	TxColCntMask	= 0x01 | 0x02 | 0x04 | 0x08, /* 4-bit collision count */
	RxErrFrame	= (1 << 27), /* Rx frame alignment error */
	RxMcast		= (1 << 26), /* Rx multicast packet rcv'd */
	RxErrCRC	= (1 << 18), /* Rx CRC error */
	RxErrRunt	= (1 << 19), /* Rx error, packet < 64 bytes */
	RWT		= (1 << 21), /* Rx  */
	E8023		= (1 << 22), /* Receive Ethernet 802.3 packet  */
	TxCRC		= (1 <<23),
	
	RxVlanOn	= (1 << 2),  /* Rx VLAN de-tagging enable */
	RxChkSum	= (1 << 1),  /* Rx checksum offload enable */		

};

enum CP_THRESHOLD_REGS{
	THVAL		= 16,
	RINGSIZE	= 2,         /* 0: 16 desc , 1: 32 desc , 2: 64 desc . */   

	LOOPBACK	= (0x3 << 8),
 	AcceptErr	= 0x20,	     /* Accept packets with CRC errors */
	AcceptRunt	= 0x10,	     /* Accept runt (<64 bytes) packets */
	AcceptBroadcast	= 0x08,	     /* Accept broadcast packets */
	AcceptMulticast	= 0x04,	     /* Accept multicast packets */
	AcceptMyPhys	= 0x02,	     /* Accept pkts with our MAC as dest */
	AcceptAllPhys	= 0x01,	     /* Accept all pkts w/ physical dest */
	AcceptAll = AcceptBroadcast | AcceptMulticast | AcceptMyPhys |  AcceptAllPhys | AcceptErr | AcceptRunt,
	AcceptNoBroad = AcceptMulticast |AcceptMyPhys |  AcceptAllPhys | AcceptErr | AcceptRunt,
	AcceptNoMulti =  AcceptMyPhys |  AcceptAllPhys | AcceptErr | AcceptRunt,
	NoErrAccept = AcceptBroadcast | AcceptMulticast | AcceptMyPhys,
	NoErrPromiscAccept = AcceptBroadcast | AcceptMulticast | AcceptMyPhys |  AcceptAllPhys,
	
};




enum CP_ISR_REGS{

	SW_INT 		= (1 <<10),
	TX_EMPTY	= (1 << 9),
	LINK_CHG	= (1 <<	8),
	TX_ERR		= (1 << 7),
	TX_OK		= (1 << 6),
	RX_EMPTY	= (1 << 5),
	RX_FIFOOVR	=(1 << 4),
	RX_ERR		=(1 << 2),
	RUNT_ERR 	=(1<<19),
	RX_OK		= (1 << 0),


};

enum CP_IOCMD_REG
{
	RX_MIT 		= 3,
	RX_TIMER 	= 1,
	RX_FIFO 	= 2,
	TX_FIFO		= 1,
	TX_MIT		= 3,	
	TX_POLL		= 1 << 0,
	CMD_CONFIG = 0x3c | RX_MIT << 8 | RX_FIFO << 11 |  RX_TIMER << 13 | TX_MIT << 16 | TX_FIFO<<19,
};

typedef enum
{
	FLAG_WRITE		= (1<<31),
	FLAG_READ		= (0<<31),
	
	MII_PHY_ADDR_SHIFT	= 26, 
	MII_REG_ADDR_SHIFT	= 16,
	MII_DATA_SHIFT		= 0,
}MIIAR_MASK;


static  const u32 cp_norx_intr_mask =  LINK_CHG | TX_OK | TX_ERR | TX_EMPTY ;
static  const u32 cp_rx_intr_mask = RX_OK | RX_ERR | RX_EMPTY | RX_FIFOOVR ;
static  const u32 cp_intr_mask = LINK_CHG | TX_OK | TX_ERR | TX_EMPTY | RX_OK | RX_ERR | RX_EMPTY | RX_FIFOOVR ;

/* hcy added */
#define CP_OFF 0 
#define CP_ON  1
#define CP_UNDER_INIT 2
static int cp_status = CP_ON ;

#if 0
static const unsigned int cp_rx_config =       //cyhuang reserved
	  (RX_FIFO_THRESH << RxCfgFIFOShift) |
	  (RX_DMA_BURST << RxCfgDMAShift);
#endif	  

struct cp_desc {
	u32		opts1;
	u32		addr;
	u32		opts2;
	u32		opts3;
};

struct ring_info {
	struct sk_buff		*skb;
	dma_addr_t		mapping;
	unsigned		frag;
};

struct cp_dma_stats {             //cyhuang reserved
	u64			tx_ok;
	u64			rx_ok;
	u64			tx_err;
	u32			rx_err;
	u16			rx_fifo;
	u16			frame_align;
	u32			tx_ok_1col;
	u32			tx_ok_mcol;
	u64			rx_ok_phys;
	u64			rx_ok_bcast;
	u32			rx_ok_mcast;
	u16			tx_abort;
	u16			tx_underrun;
} __attribute__((packed));

struct cp_extra_stats {
	unsigned long		rx_frags;
	unsigned long           tx_timeouts;
	unsigned long           tx_cnt;
};

struct cp_private {
	unsigned		tx_hqhead;    //add
	unsigned		tx_hqtail;    //add
	unsigned		tx_lqhead;    //add
	unsigned		tx_lqtail;    //add
	
	
	void			__iomem *regs;
	struct net_device	*dev;   
	spinlock_t		lock;
	u32			msg_enable;


	u32			rx_config;
	u16			cpcmd;         //cyhuang reserved

	struct net_device_stats net_stats;
	struct cp_extra_stats	cp_stats;
	

	unsigned		rx_tail		____cacheline_aligned;
	struct cp_desc		*rx_ring;
	struct ring_info	rx_skb[CP_RX_RING_SIZE];
	unsigned		rx_buf_sz;

	

       struct cp_desc		*tx_hqring;
       struct cp_desc          *tx_lqring;
       struct ring_info	tx_skb[CP_TX_RING_SIZE];
       dma_addr_t		ring_dma;
 
        struct sk_buff		*frag_skb;        //add
        unsigned		dropping_frag : 1;//add
        char*			rxdesc_buf;       //add
	char*			txdesc_buf;       //add

#if CP_VLAN_TAG_USED
	struct vlan_group	*vlgrp;
#endif

	unsigned int		wol_enabled : 1; /* Is Wake-on-LAN enabled? */
                                                 //cyhuang reserved
	struct mii_if_info	mii_if;
};

#define cpr8(reg)	readb(cp->regs + (reg))
#define cpr16(reg)	readw(cp->regs + (reg))
#define cpr32(reg)	readl(cp->regs + (reg))
#define cpw8(reg,val)	writeb((val), cp->regs + (reg))
#define cpw16(reg,val)	writew((val), cp->regs + (reg))
#define cpw32(reg,val)	writel((val), cp->regs + (reg))
#define cpw8_f(reg,val) do {			\
	writeb((val), cp->regs + (reg));	\
	readb(cp->regs + (reg));		\
	} while (0)
#define cpw16_f(reg,val) do {			\
	writew((val), cp->regs + (reg));	\
	readw(cp->regs + (reg));		\
	} while (0)
#define cpw32_f(reg,val) do {			\
	writel((val), cp->regs + (reg));	\
	readl(cp->regs + (reg));		\
	} while (0)


static void __cp_set_rx_mode (struct net_device *dev);
static void cp_tx (struct cp_private *cp);
static void cp_clean_rings (struct cp_private *cp);
static void cp_stop_hw (struct cp_private *cp);
static void cp_start_hw (struct cp_private *cp);
static void cp_show_regs_datum (struct net_device *dev);

//static void cp_tx_timeout (struct net_device *dev); //add



static struct {                                     //cyhuang reserved
	const char str[ETH_GSTRING_LEN];
} ethtool_stats_keys[] = {
	{ "tx_ok" },
	{ "rx_ok" },
	{ "tx_err" },
	{ "rx_err" },
	{ "rx_fifo" },
	{ "frame_align" },
	{ "tx_ok_1col" },
	{ "tx_ok_mcol" },
	{ "rx_ok_phys" },
	{ "rx_ok_bcast" },
	{ "rx_ok_mcast" },
	{ "tx_abort" },
	{ "tx_underrun" },
	{ "rx_frags" },
};


#if CP_VLAN_TAG_USED
static void cp_vlan_rx_register(struct net_device *dev, struct vlan_group *grp)
{
	struct cp_private *cp = netdev_priv(dev);
	unsigned long flags;

	spin_lock_irqsave(&cp->lock, flags);
	cp->vlgrp = grp;
	cp->cpcmd |= RxVlanOn;
	cpw16(CMD, cp->cpcmd);
	spin_unlock_irqrestore(&cp->lock, flags);
}

static void cp_vlan_rx_kill_vid(struct net_device *dev, unsigned short vid)
{
	struct cp_private *cp = netdev_priv(dev);
	unsigned long flags;

	spin_lock_irqsave(&cp->lock, flags);
	cp->cpcmd &= ~RxVlanOn;
	cpw16(CMD, cp->cpcmd);
	if (cp->vlgrp)
		cp->vlgrp->vlan_devices[vid] = NULL;
	spin_unlock_irqrestore(&cp->lock, flags);
}
#endif /* CP_VLAN_TAG_USED */

static inline void cp_set_rxbufsize (struct cp_private *cp)
{
	unsigned int mtu = cp->dev->mtu;
	
	if (mtu > ETH_DATA_LEN)
		/* MTU + ethernet header + FCS + optional VLAN tag */
		cp->rx_buf_sz = mtu + ETH_HLEN + 8;
	else
		cp->rx_buf_sz = PKT_BUF_SZ;
}

static inline void cp_rx_skb (struct cp_private *cp, struct sk_buff *skb,
			      struct cp_desc *desc)
{
	skb->dev = cp->dev;  //cyhuang test
	skb->protocol = eth_type_trans (skb, cp->dev);

	cp->net_stats.rx_packets++;
	cp->net_stats.rx_bytes += skb->len;
	cp->dev->last_rx = jiffies;

#if CP_VLAN_TAG_USED
	if (cp->vlgrp && (desc->opts2 & RxVlanTagged)) {
		vlan_hwaccel_receive_skb(skb, cp->vlgrp,
					 be16_to_cpu(desc->opts2 & 0xffff));
	} else
#endif
		netif_receive_skb(skb);
}

static void cp_rx_err_acct (struct cp_private *cp, unsigned rx_tail,
			    u32 status, u32 len)
{
	if (netif_msg_rx_err (cp))
		printk (KERN_DEBUG
			"%s: rx err, slot %d status 0x%x len %d\n",
			cp->dev->name, rx_tail, status, len);
	cp->net_stats.rx_errors++;
	if (status & RxErrFrame)
		cp->net_stats.rx_frame_errors++;
	if (status & RxErrCRC)
		cp->net_stats.rx_crc_errors++;
	if (status & RxErrRunt) 
		cp->net_stats.rx_length_errors++;
	
}

static void cp_rx_frag (struct cp_private *cp, unsigned rx_tail,      //add whole function
			struct sk_buff *skb, u32 status, u32 len)
{
	struct sk_buff *copy_skb, *frag_skb = cp->frag_skb;
	unsigned orig_len = frag_skb ? frag_skb->len : 0;
	unsigned target_len = orig_len + len;
	unsigned first_frag = status & FirstFrag;
	unsigned last_frag = status & LastFrag;
	if (netif_msg_rx_status (cp))
		printk (KERN_DEBUG "%s: rx %s%sfrag, slot %d status 0x%x len %d\n",
			cp->dev->name,
			cp->dropping_frag ? "dropping " : "",
			first_frag ? "first " :
			last_frag ? "last " : "",
			rx_tail, status, len);

	cp->cp_stats.rx_frags++;

	if (!frag_skb && !first_frag)
		cp->dropping_frag = 1;
	if (cp->dropping_frag)
            goto drop_frag;

	copy_skb = dev_alloc_skb (target_len + RX_OFFSET);
	if (!copy_skb) {
		printk(KERN_WARNING "%s: rx slot %d alloc failed\n",
		       cp->dev->name, rx_tail);

		cp->dropping_frag = 1;
drop_frag:
		if (frag_skb) {
			dev_kfree_skb_irq(frag_skb);
			cp->frag_skb = NULL;
		}
		if (last_frag) {
			cp->net_stats.rx_dropped++;
			cp->dropping_frag = 0;
		}
		return;
	}

	copy_skb->dev = cp->dev;
	skb_reserve(copy_skb, RX_OFFSET);
	skb_put(copy_skb, target_len);
	if (frag_skb) {
		memcpy(copy_skb->data, frag_skb->data, orig_len);
      
		dev_kfree_skb_irq(frag_skb);
     
	}
      
       
	memcpy(copy_skb->data + orig_len, skb->data, len);
      

	copy_skb->ip_summed = CHECKSUM_NONE;

	if (last_frag) {
		if (status & (RxError)) {
			cp_rx_err_acct(cp, rx_tail, status, len);
			dev_kfree_skb_irq(copy_skb);
		} else
			cp_rx_skb(cp, copy_skb, &cp->rx_ring[rx_tail]);
		cp->frag_skb = NULL;
	} else {
		cp->frag_skb = copy_skb;
	}
}

static inline unsigned int cp_rx_csum_ok (u32 status)
{
	unsigned int protocol = (status >> 16) & 0x3;
	
	/* if no protocol , we dont sw check either */  
	if (!protocol)
		return 1;
	
	if (likely((protocol == RxProtoTCP) && (!(status & TCPFail))))
		return 1;
	else if ((protocol == RxProtoUDP) && (!(status & UDPFail)))
		return 1;
	else if ((protocol == RxProtoIP) && (!(status & IPFail)))
		return 1;
//	printk("ERR !!! etn cp_rx_csum error protocol=0x%x , status=0x%x!!!\n", 
//			protocol, status);
	return 0;
}



static int cp_rx_poll (struct net_device *dev, int *budget)
{
	struct cp_private *cp = netdev_priv(dev);
	unsigned rx_tail = cp->rx_tail;
	unsigned rx_work = dev->quota; /* quota = 8  cyhuang */
	unsigned rx;


        if (cpr16(ISR) & (RX_FIFOOVR ))
        {   
         unsigned i;
                           
            printk("net RX_FIFOOVR 1!! \n");//cy test          

	    
            local_irq_disable();
               
            cpw16_f(IMR, 0);

	    cpw32(IO_CMD, 0);
	    cpw16_f(ISR, 0xffff);
            memset(cp->tx_hqring, 0, sizeof(struct cp_desc) * CP_TX_RING_SIZE);
            cp->tx_hqring[CP_TX_RING_SIZE - 1].opts1 = (RingEnd);         

            for (i = 0; i < CP_RX_RING_SIZE; i++) {
                 if (i == (CP_RX_RING_SIZE - 1))
	             	cp->rx_ring[i].opts1 =
		        	(DescOwn | RingEnd | cp->rx_buf_sz);
		 		 else
		     		cp->rx_ring[i].opts1 =
		        	(DescOwn | cp->rx_buf_sz);
            } 
            cp->rx_tail = 0;
	    	cp->tx_hqhead = cp->tx_hqtail = 0; 
            cpw8(EthrntRxCPU_Des_Num,CP_RX_RING_SIZE-1);
            cpw16(RxCDO,0);

            cpw16_f(IMR, 0xffff);  	
            cp_start_hw(cp);      

                
            __netif_rx_complete(dev);
	    local_irq_enable();

	    return 0;	/* done */
 

        }
rx_status_loop: 
	
        cpw16(ISR, cp_rx_intr_mask); 

	rx = 0;
       
	while (1) {
		u32 status, len;
		dma_addr_t mapping;
		struct sk_buff *skb, *new_skb;
		struct cp_desc *desc;
		unsigned buflen;
		skb = cp->rx_skb[rx_tail].skb;
		if (!skb)
		{       
			printk("BUG , skb error\n");//cy test
			BUG();
				
		}	
		desc = &cp->rx_ring[rx_tail];
		
                
		
		status = (desc->opts1);

            
		
		if (status & DescOwn)   
		{      
		       break;
		}	
                len = (status & 0x0fff) - 4;

                


		
		mapping = cp->rx_skb[rx_tail].mapping;

		if ((status & (FirstFrag | LastFrag)) != (FirstFrag | LastFrag)) {
			/* we don't support incoming fragmented frames.
			 * instead, we attempt to ensure that the
			 * pre-allocated RX skbs are properly sized such
			 * that RX fragments are never encountered
			 */
			cp_rx_err_acct(cp, rx_tail, status, len);
			cp->net_stats.rx_dropped++;			
			cp->cp_stats.rx_frags++;
               		       
//		        cp_rx_frag(cp, rx_tail, skb, status, len); // cyhuang add 
			goto rx_next;
		
		}


		if (status & (RxError )) {
		                
			cp_rx_err_acct(cp, rx_tail, status, len);
			goto rx_next;
		}

                
		if (netif_msg_rx_status(cp))
			printk(KERN_DEBUG "%s: rx slot %d status 0x%x len %d\n",
			       cp->dev->name, rx_tail, status, len);
		buflen = cp->rx_buf_sz + RX_OFFSET;
		new_skb = dev_alloc_skb (buflen);
		if (!new_skb) {
		        
			cp->net_stats.rx_dropped++;     //see re8670.c to check
			printk("BUG , cant get skb from dev_alloc_skb\n");//cy test
			BUG();
			goto rx_next;
		}
                if ((u32)new_skb->data &0x3)            //cyhuang add
			printk(KERN_DEBUG "new_skb->data unaligment %8x\n",(u32)new_skb->data);
			
//		printk("cylee in %s, before reserve: %02x.%02x.%02x.%02x.%02x.%02x\n", new_skb->data[0], new_skb->data[1], new_skb->data[2], new_skb->data[3], new_skb->data[4], new_skb->data[5]);
//		printk("cylee in %s, before reserve: new_skb->data = %x\n", new_skb->data);
		skb_reserve(new_skb, RX_OFFSET);        //cyhuang reserved
//		printk("cylee in %s, after reserve: %02x.%02x.%02x.%02x.%02x.%02x\n", new_skb->data[0], new_skb->data[1], new_skb->data[2], new_skb->data[3], new_skb->data[4], new_skb->data[5]);
//		printk("cylee in %s, after reserve: new_skb->data = %x\n", new_skb->data);
		new_skb->dev = cp->dev;

		/* Handle checksum offloading for incoming packets. */
		if (cp_rx_csum_ok(status))
			skb->ip_summed = CHECKSUM_UNNECESSARY;
		else {
			/* still dont do sw check , or will decrease the performance */
			skb->ip_summed = CHECKSUM_UNNECESSARY; //CHECKSUM_NONE;
		}	
		
		skb_put(skb, len);


		mapping =
		cp->rx_skb[rx_tail].mapping = ((u32)new_skb->tail & ~KSEG_MASK); //chuang add
			
		cp->rx_skb[rx_tail].skb = new_skb;         

		cp_rx_skb(cp, skb, desc);
                
		rx++;                          
rx_next:
		cp->rx_ring[rx_tail].opts2 = 0;
		cp->rx_ring[rx_tail].addr = mapping;

		if (rx_tail == (CP_RX_RING_SIZE - 1))
			desc->opts1 = DescOwn | RingEnd |	
						  cp->rx_buf_sz;
		else
			desc->opts1 = DescOwn | cp->rx_buf_sz;
                
                cpw8(EthrntRxCPU_Des_Num,rx_tail);  //cyhuang added for flow control
		rx_tail = NEXT_RX(rx_tail);
                
                
                rx_work--;
		if (!rx_work)
			break;
               
	}
       

	cp->rx_tail = rx_tail;

	dev->quota -= rx;    
	*budget -= rx;        
	/* if we did not reach work limit, then we're done with
	 * this round of polling
	 */
	if (rx_work) {   

     

                
           
		if (cpr16(ISR) & cp_rx_intr_mask)
		{
                        
                        if (cpr16(ISR) & (RX_FIFOOVR )) 
                        {   
                            unsigned i;
                           

                           
                            local_irq_disable();
               
                            cpw16_f(IMR, 0);
	                    cpw32(IO_CMD, 0);
	                    cpw16_f(ISR, 0xffff);
                                             
                            memset(cp->tx_hqring, 0, sizeof(struct cp_desc) * CP_TX_RING_SIZE);
                            cp->tx_hqring[CP_TX_RING_SIZE - 1].opts1 = (RingEnd);    
                            for (i = 0; i < CP_RX_RING_SIZE; i++) {
                                 if (i == (CP_RX_RING_SIZE - 1))
	                             cp->rx_ring[i].opts1 =
		                       (DescOwn | RingEnd | cp->rx_buf_sz);
		                 else
		                     cp->rx_ring[i].opts1 =
		                       (DescOwn | cp->rx_buf_sz);
                            } 
                            cp->rx_tail = 0;
                       	    cp->tx_hqhead = cp->tx_hqtail = 0; 
                            cpw8(EthrntRxCPU_Des_Num,CP_RX_RING_SIZE-1);
                            cpw16(RxCDO,0);

                            cpw16_f(IMR, 0xffff);  	
                            cp_start_hw(cp);      

                
                            __netif_rx_complete(dev);
	                    local_irq_enable();

	                    return 0;	/* done */
                        } 
                        else {
			    goto rx_status_loop;
                        }
		}	

		local_irq_disable();

                cpw16_f(IMR, cp_intr_mask);   //cy test
		__netif_rx_complete(dev);
		local_irq_enable();
		return 0;	/* done */
	}
	return 1;		/* not done */
}

static irqreturn_t
cp_interrupt (int irq, void *dev_instance, struct pt_regs *regs)
{
	struct net_device *dev = dev_instance;
	struct cp_private *cp;
	u16 status;
	if (unlikely(dev == NULL))
		return IRQ_NONE;
	cp = netdev_priv(dev);


	
	status = cpr16(ISR);
	if (!status || (status == 0xFFFF))
		return IRQ_NONE;

	if (netif_msg_intr(cp))
		printk(KERN_DEBUG "%s: intr, status %04x cmd %02x \n",
		        dev->name, status, cpr8(CMD));

        cpw16(ISR, status & ~cp_rx_intr_mask);   //cy test	
	spin_lock(&cp->lock);
	/* close possible race's with dev_close */
	if (unlikely(!netif_running(dev))) {         //cyhuang reserved
		cpw16(IMR, 0);
		spin_unlock(&cp->lock);
		return IRQ_HANDLED;
	}
	if (status & RX_EMPTY)  {	   	
    	    printk("error:RX desc. unavailable ,status = 0x%x \n",status); 

        }
        if (status & RX_ERR)  	
    	    printk("error:RX runt ,status = 0x%x \n",status); 	
    	    
        if (status & RX_FIFOOVR) {                  
           
            printk("error:RX FIFO overflow , status = 0x%x \n",status);
            

        }

        if (status & (RX_OK | RX_ERR | RX_EMPTY |RX_FIFOOVR))
        {
		if (netif_rx_schedule_prep(dev)) {

                        cpw16_f(IMR, cp_norx_intr_mask);
			__netif_rx_schedule(dev);
		}
                
        }

check_tx:
      
	if (status & (TX_OK | TX_ERR | TX_EMPTY | SW_INT))
		cp_tx(cp);
	if (status & LINK_CHG)            
		mii_check_media(&cp->mii_if, netif_msg_link(cp), FALSE);
		
        
	spin_unlock(&cp->lock);

	return IRQ_HANDLED;
}

static void cp_tx (struct cp_private *cp)
{
	unsigned tx_head = cp->tx_hqhead;
	unsigned tx_tail = cp->tx_hqtail;

	
	while (tx_tail != tx_head) {
		struct sk_buff *skb;
		u32 status;

		rmb();
		status = (cp->tx_hqring[tx_tail].opts1);
		if (status & DescOwn)
			break;

		skb = cp->tx_skb[tx_tail].skb;
		if (!skb)
			BUG();

	

		if (status & LastFrag) {
			if (status & (TxError | TxFIFOUnder)) {
				if (netif_msg_tx_err(cp))
				printk(KERN_DEBUG "%s: tx err, status 0x%x\n",
					       cp->dev->name, status);
				//	printk(KERN_DEBUG "%s: tx err, status 0x%x\n",
				//	       cp->dev->name, status);
				cp->net_stats.tx_errors++;
				if (status & TxOWC)
					cp->net_stats.tx_window_errors++;
				if (status & TxMaxCol)
					cp->net_stats.tx_aborted_errors++;
				if (status & TxLinkFail)
					cp->net_stats.tx_carrier_errors++;
				if (status & TxFIFOUnder)
					cp->net_stats.tx_fifo_errors++;
			} else {
				cp->net_stats.collisions +=
					((status >> TxColCntShift) & TxColCntMask);
				cp->net_stats.tx_packets++;
				cp->net_stats.tx_bytes += skb->len;
				if (netif_msg_tx_done(cp))
				printk(KERN_DEBUG "%s: tx done, slot %d\n", cp->dev->name, tx_tail);
				//	printk(KERN_DEBUG "%s: tx done, slot %d\n", cp->dev->name, tx_tail);
			}
			dev_kfree_skb_irq(skb);
		}

		cp->tx_skb[tx_tail].skb = NULL;

		tx_tail = NEXT_TX(tx_tail);
	}

	cp->tx_hqtail = tx_tail;

	if (TX_HQBUFFS_AVAIL(cp) > (MAX_SKB_FRAGS + 1))
		netif_wake_queue(cp->dev);
}

static int cp_start_xmit (struct sk_buff *skb, struct net_device *dev)
{
	struct cp_private *cp = netdev_priv(dev);
	unsigned entry;
	u32 eor;
	
	
	cp->cp_stats.tx_cnt++;    //cyhuang add
#if CP_VLAN_TAG_USED
	u32 vlan_tag = 0;
#endif

	spin_lock_irq(&cp->lock);

	/* This is a hard error, log it. */
	if (TX_HQBUFFS_AVAIL(cp) <= (skb_shinfo(skb)->nr_frags + 1)) {
		netif_stop_queue(dev);
		spin_unlock_irq(&cp->lock);
		printk(KERN_ERR PFX "%s: BUG! Tx Ring full when queue awake!\n",
		       dev->name);
		return 1;
	}

#if CP_VLAN_TAG_USED
	if (cp->vlgrp && vlan_tx_tag_present(skb))
		vlan_tag = TxVlanTag | (vlan_tx_tag_get(skb));
#endif
        dma_cache_wback((u32)skb->data, skb->len); //cy test
	entry = cp->tx_hqhead;
	eor = (entry == (CP_TX_RING_SIZE - 1)) ? RingEnd : 0;
	if (skb_shinfo(skb)->nr_frags == 0) {
		struct cp_desc *txd = &cp->tx_hqring[entry];
		u32 len;
		dma_addr_t mapping;

		len = skb->len;
		mapping = (u32)skb->data & ~KSEG_MASK;
		CP_VLAN_TX_TAG(txd, vlan_tag);
		txd->addr = (mapping);


		wmb();

		txd->opts1 = (eor | len | DescOwn |
					FirstFrag | LastFrag | TxCRC);
		wmb();

		cp->tx_skb[entry].skb = skb;
		cp->tx_skb[entry].mapping = mapping;
		cp->tx_skb[entry].frag = 0;
		entry = NEXT_TX(entry);
	} else {
		struct cp_desc *txd;
		u32 first_len, first_eor;
		dma_addr_t first_mapping;
		int frag, first_entry = entry;
//		const struct iphdr *ip = skb->nh.iph;

		/* We must give this initial chunk to the device last.
		 * Otherwise we could race with the device.
		 */
		first_eor = eor;
		first_len = skb_headlen(skb);
		first_mapping = (u32)skb->data& ~KSEG_MASK;
		cp->tx_skb[entry].skb = skb;
		cp->tx_skb[entry].mapping = first_mapping;
		cp->tx_skb[entry].frag = 1;
		entry = NEXT_TX(entry);

		for (frag = 0; frag < skb_shinfo(skb)->nr_frags; frag++) {
			skb_frag_t *this_frag = &skb_shinfo(skb)->frags[frag];
			u32 len;
			u32 ctrl;
			dma_addr_t mapping;

			len = this_frag->size;
			mapping = (u32)this_frag->page_offset& ~KSEG_MASK; 
			eor = (entry == (CP_TX_RING_SIZE - 1)) ? RingEnd : 0;

			
			ctrl = eor | len | DescOwn | TxCRC;

			if (frag == skb_shinfo(skb)->nr_frags - 1)
				ctrl |= LastFrag;

			txd = &cp->tx_hqring[entry];
			CP_VLAN_TX_TAG(txd, vlan_tag);
			txd->addr = (mapping);
			wmb();

			txd->opts1 = (ctrl);
			wmb();

			cp->tx_skb[entry].skb = skb;
			cp->tx_skb[entry].mapping = mapping;
			cp->tx_skb[entry].frag = frag + 2;
			entry = NEXT_TX(entry);
		}

		txd = &cp->tx_hqring[first_entry];
		CP_VLAN_TX_TAG(txd, vlan_tag);
		txd->addr = (first_mapping);
		wmb();

		eor = (first_entry == (CP_TX_RING_SIZE - 1)) ? RingEnd : 0;
		
		txd->opts1 = (eor | first_len | TxCRC |
				FirstFrag | DescOwn);
		wmb();
	}
	cp->tx_hqhead = entry;
	if (netif_msg_tx_queued(cp))
		printk(KERN_DEBUG "%s: tx queued, slot %d, skblen %d\n",
		       dev->name, entry, skb->len);
	if (TX_HQBUFFS_AVAIL(cp) <= (MAX_SKB_FRAGS + 1))
		netif_stop_queue(dev);

	spin_unlock_irq(&cp->lock);

	cpw32(IO_CMD,CMD_CONFIG | TX_POLL );
      
	dev->trans_start = jiffies;

	return 0;
}

/* Set or clear the multicast filter for this adaptor.
   This routine is not state sensitive and need not be SMP locked. */

static void __cp_set_rx_mode (struct net_device *dev)
{
	struct cp_private *cp = netdev_priv(dev);
	u32 mc_filter[2];	/* Multicast hash filter */
	int i, rx_mode;
	u32 tmp;

	/* Note: do not reorder, GCC is clever about common statements. */
	if (dev->flags & IFF_PROMISC) {
		/* Unconditionally log net taps. */
		printk (KERN_NOTICE "%s: Promiscuous mode enabled.\n",
			dev->name);
		rx_mode =
		    AcceptBroadcast | AcceptMulticast | AcceptMyPhys |
		    AcceptAllPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	} else if ((dev->mc_count > multicast_filter_limit)
		   || (dev->flags & IFF_ALLMULTI)) {
		/* Too many to filter perfectly -- accept all multicasts. */
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	} else {
		struct dev_mc_list *mclist;
		rx_mode = AcceptBroadcast | AcceptMyPhys;
	
		mc_filter[1] = mc_filter[0] = 0;
		for (i = 0, mclist = dev->mc_list; mclist && i < dev->mc_count;
		     i++, mclist = mclist->next) {
			int bit_nr = ether_crc(ETH_ALEN, mclist->dmi_addr) >> 26;

			mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);

			rx_mode |= AcceptMulticast;
		}
	}

	/* We can safely update without stopping the chip. */
	tmp = rx_mode;
	if (cp->rx_config != tmp) {
		cpw32_f (RCR, tmp);
		cp->rx_config = tmp;
	}
	cpw32_f (MAR0 + 0, __cpu_to_be32(mc_filter[0]));   
	cpw32_f (MAR0 + 4, __cpu_to_be32(mc_filter[1]));   
}

static void cp_set_rx_mode (struct net_device *dev)
{
	unsigned long flags;
	struct cp_private *cp = netdev_priv(dev);

	spin_lock_irqsave (&cp->lock, flags);
	__cp_set_rx_mode(dev);
	spin_unlock_irqrestore (&cp->lock, flags);
}

static void __cp_get_stats(struct cp_private *cp)
{
	cp->net_stats.rx_missed_errors += (cpr16 (MISSPKT) & 0xffff);
	cpw32 (MISSPKT, 0);
}

static struct net_device_stats *cp_get_stats(struct net_device *dev)
{
	struct cp_private *cp = netdev_priv(dev);
	unsigned long flags;

	/* The chip only need report frame silently dropped. */
	spin_lock_irqsave(&cp->lock, flags);
 	if (netif_running(dev) && netif_device_present(dev))
 		__cp_get_stats(cp);
	spin_unlock_irqrestore(&cp->lock, flags);

	return &cp->net_stats;
}


static void cp_stop_hw (struct cp_private *cp)
{
	printk("cp_stop_hw *********************************\n"); //cy test
	
	cpw16_f(IMR, 0);
	cpw32(IO_CMD, 0);
	cpw16_f(ISR, 0xffff);

	cp->rx_config = 0;
	cp->rx_tail = 0;
	cp->tx_hqhead = cp->tx_hqtail = 0;
}
static void cp_reset_hw (struct cp_private *cp)
{
	unsigned work = 1000;

	cpw8(CMD, 0x1);      /*reset */

	while (work--) {
		if (!(cpr8(CMD) & 0x1))
		{  
			cpw8(CMD, 0x2) ;    /*checksum*/ //cyhuang add
			return;
		}	

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(10);
	}

	printk(KERN_ERR "%s: hardware reset timeout\n", cp->dev->name);
}





static inline void cp_start_hw (struct cp_private *cp)
{
	cpw32(IO_CMD, CMD_CONFIG);
	
}

static int cp_init_hw (struct cp_private *cp)
{
	struct net_device *dev = cp->dev;
	u8 status;
	u32 *hwaddr;
    
	cp_reset_hw(cp);
        

//        cp->mii_if.mdio_write(dev , 1, 0, 0x8000); 
//	cp->mii_if.mdio_write(dev , 1, 0, 0x0100); 
//	cp->mii_if.mdio_write(dev , 1, 4, 0x05e1); 

      
    cpw32(MIIAR, 0x84008000) ; 

    mdelay(15); //cy test	     	            	           
     
    /*
    if((platform_info.board_id == realtek_neptune_qa_board)
        &&( (*(volatile unsigned int*)(0xb801a204)&0xffff0000) == 0xa00000))    
        cpw32(MIIAR, 0x84000100) ;        	    
	else
		cpw32(MIIAR, 0x84001000) ;
    */
    mdelay(15); //cy test	     	    
        
    cpw32(MIIAR, 0x840405e1) ;
        
    mdelay(15); //cy test	     	    
	
	cpw16(ISR,0xffff);
	cpw16(IMR,cp_intr_mask);   	
	cpw32(RxFDP,(u32)(cp->rx_ring));	
#if DEBUG
	writecheck = RTL_R32(RxFDP);	
	if (writecheck != ( (u32)cp->rx_ring ))
		for (;;);

#endif	
	cpw16(RxCDO,0);
	cpw32(TxFDP1,(u32)(cp->tx_hqring));	
	
#if DEBUG
	writecheck = RTL_R32(TxFDP1);	
	if (writecheck != ( (u32)cp->tx_hqring))
		for (;;);
#endif
	cpw16(TxCDO1,0);	
	cpw32(TxFDP2,(u32)cp->tx_lqring);
	cpw16(TxCDO2,0);
	
	//RTL_W32(RCR,AcceptAll);   cyhuang reserved		
	cpw32(TCR,(u32)(0xC00));
	cpw8(Rx_Pse_Des_Thres,THVAL);
	cpw8(EthrntRxCPU_Des_Num,CP_RX_RING_SIZE-1);
//        cpw8(EthrntRxCPU_Des_Num,0); 
	cpw8(RxRingSize,RINGSIZE);	
	status = cpr8(MSR);
	status = status |(TXFCE | RXFCE | FORCE_TX);
	cpw8(MSR,status);
	
	hwaddr = (u32 *)(cp->dev->dev_addr);
	
	cpw32(IDR0, __cpu_to_be32(*hwaddr));
	hwaddr = ((u32 *)(cp->dev->dev_addr+4));
	
	cpw32(IDR4, __cpu_to_be32(*hwaddr));
	cp_start_hw(cp);

	__cp_set_rx_mode(dev);	
	
#if 0	
    cpw32(MIIAR, 0x84008000) ; 	
#endif
    
    return 0;
}
static int cp_refill_rx (struct cp_private *cp)
{
	unsigned i;
	for (i = 0; i < CP_RX_RING_SIZE; i++) {
		struct sk_buff *skb;
                
		skb = dev_alloc_skb(cp->rx_buf_sz + RX_OFFSET);
		
		if (!skb)
		{//       printk("refill e1\n");//cy test
			goto err_out;
		}	
                
		skb->dev = cp->dev;
		skb_reserve(skb, RX_OFFSET); //cyhuang reserved

//		printk("skb->head = 0x%x\n", skb->head);
//		printk("skb->tail = 0x%x\n", skb->tail);
//		printk("cp->rx_buf_sz = 0x%x\n", cp->rx_buf_sz);
		cp->rx_skb[i].mapping = (u32)skb->tail & ~KSEG_MASK; //cyhuang or skb->data ??	
		cp->rx_skb[i].skb = skb;
		cp->rx_skb[i].frag = 0;

		cp->rx_ring[i].opts2 = 0;
		cp->rx_ring[i].addr = (cp->rx_skb[i].mapping);
	
		if (i == (CP_RX_RING_SIZE - 1))
			cp->rx_ring[i].opts1 =
				(DescOwn | RingEnd | cp->rx_buf_sz);
		else
			cp->rx_ring[i].opts1 =
				(DescOwn | cp->rx_buf_sz);
		
	}
	return 0;

err_out:
	cp_clean_rings(cp);
	return -ENOMEM;
}

static int cp_init_rings (struct cp_private *cp)
{
	memset(cp->tx_hqring, 0, sizeof(struct cp_desc) * CP_TX_RING_SIZE);
	cp->tx_hqring[CP_TX_RING_SIZE - 1].opts1 = (RingEnd);              //cyhuang reserved

        memset(cp->rx_ring, 0, sizeof(struct cp_desc) * CP_RX_RING_SIZE); //cyhuang add
	cp->rx_tail = 0;
	cp->tx_hqhead = cp->tx_hqtail = 0;

	return cp_refill_rx (cp);
}
static int cp_alloc_rings (struct cp_private *cp)
{
	void*	pBuf;
	
	pBuf = dma_alloc_coherent(NULL, CP_RXRING_BYTES+CP_TXRING_BYTES,
	                          &cp->ring_dma, GFP_ATOMIC); //cyhuang added
//        pBuf = kmalloc(CP_RXRING_BYTES,GFP_KERNEL);
	cp->ring_dma = (dma_addr_t) (pBuf);
	if (!pBuf)
	{      
		printk("can't get page from dma_alloc_coherent \n");//cy test
	        BUG();	
		goto ErrMem;
	}	
	cp->rxdesc_buf = pBuf;
	printk("alloc rings cp->rxdesc_buf =0x%x \n",
		            (u32)cp->rxdesc_buf);//cy test
	
	memset(pBuf, 0, CP_RXRING_BYTES+CP_TXRING_BYTES);	
	/* 256 bytes alignment */
	pBuf = (void*)( (u32)(pBuf + DESC_ALIGN-1) &  ~(DESC_ALIGN -1) ) ; 	
	cp->rx_ring = (struct cp_desc*)((u32)(pBuf) | UNCACHE_MASK);	
	
	
	cp->tx_hqring = &cp->rx_ring[CP_RX_RING_SIZE];		
//	pBuf= kmalloc(CP_TXRING_BYTES, GFP_KERNEL);
//	if (!pBuf)
//	{       printk("alloc e2\n");//cy test 
//		goto ErrMem;
//	}	
        /* hcy omit 2007/11/07 */
	cp->txdesc_buf = pBuf;
//	memset(pBuf, 0, CP_TXRING_BYTES);
	/* 256 bytes alignment */
//	pBuf = (void*)( (u32)(pBuf + DESC_ALIGN-1) &  ~(DESC_ALIGN -1) ) ;
//	cp->tx_hqring = (struct cp_desc*)((u32)(pBuf) | UNCACHE_MASK);
	
	return cp_init_rings(cp);
ErrMem:
	return -ENOMEM;	 	   	
}

static void cp_clean_rings (struct cp_private *cp)
{
	unsigned i;

	memset(cp->rx_ring, 0, sizeof(struct cp_desc) * CP_RX_RING_SIZE);
	memset(cp->tx_hqring, 0, sizeof(struct cp_desc) * CP_TX_RING_SIZE);

	for (i = 0; i < CP_RX_RING_SIZE; i++) {
		if (cp->rx_skb[i].skb) {
			
			dev_kfree_skb(cp->rx_skb[i].skb);
		}
	}

	for (i = 0; i < CP_TX_RING_SIZE; i++) {
		if (cp->tx_skb[i].skb) {
			struct sk_buff *skb = cp->tx_skb[i].skb;
			
			dev_kfree_skb(skb);
			cp->net_stats.tx_dropped++;
		}
	}

	memset(&cp->rx_skb, 0, sizeof(struct ring_info) * CP_RX_RING_SIZE);
	memset(&cp->tx_skb, 0, sizeof(struct ring_info) * CP_TX_RING_SIZE);
}

static void cp_free_rings (struct cp_private *cp)
{
	cp_clean_rings(cp);
	printk("free rings cp->rxdesc_buf =0x%x \n",
			(u32)cp->rxdesc_buf);//cy test
	dma_free_coherent(NULL, CP_RXRING_BYTES+CP_TXRING_BYTES, cp->rx_ring, //cy added
	                   cp->ring_dma);
	
	cp->rx_ring = NULL;
	cp->tx_hqring = NULL;

}

static int cp_open (struct net_device *dev)
{
	struct cp_private *cp = netdev_priv(dev);
	int rc;
//        printk("cp_open\n");//cy test
	if (netif_msg_ifup(cp))
		printk(KERN_DEBUG "%s: enabling interface\n", dev->name);

			
	rc = cp_alloc_rings(cp);
	if (rc)
		return rc;
      
	rc = cp_init_hw(cp);
	if (rc)
		goto err_out_rings;
        	
	rc = request_irq(dev->irq, cp_interrupt, SA_SHIRQ, dev->name, dev);
	if (rc)
		goto err_out_hw;
     
	netif_carrier_off(dev);
	mii_check_media(&cp->mii_if, netif_msg_link(cp), TRUE); 
	netif_start_queue(dev);

	return 0;

err_out_hw:
       
	cp_stop_hw(cp);
err_out_rings:	
	cp_free_rings(cp);
	return rc;
}

static int cp_close (struct net_device *dev)
{
	struct cp_private *cp = netdev_priv(dev);
	unsigned long flags;
//        printk("cp_close\n");//cy test
	if (netif_msg_ifdown(cp))
		printk(KERN_DEBUG "%s: disabling interface\n", dev->name);

	spin_lock_irqsave(&cp->lock, flags);

	netif_stop_queue(dev);
	netif_carrier_off(dev);

	cp_stop_hw(cp);

	spin_unlock_irqrestore(&cp->lock, flags);

	synchronize_irq(dev->irq);
	free_irq(dev->irq, dev);

	cp_free_rings(cp);
	return 0;
}

#ifdef BROKEN
static int cp_change_mtu(struct net_device *dev, int new_mtu)
{
	struct cp_private *cp = netdev_priv(dev);
	int rc;
	unsigned long flags;

	/* check for invalid MTU, according to hardware limits */
	if (new_mtu < CP_MIN_MTU || new_mtu > CP_MAX_MTU)
		return -EINVAL;

	/* if network interface not up, no need for complexity */
	if (!netif_running(dev)) {
		dev->mtu = new_mtu;
		cp_set_rxbufsize(cp);	/* set new rx buf size */
		return 0;
	}

	spin_lock_irqsave(&cp->lock, flags);

	cp_stop_hw(cp);			/* stop h/w and free rings */
	cp_clean_rings(cp);

	dev->mtu = new_mtu;
	cp_set_rxbufsize(cp);		/* set new rx buf size */
        printk("cp_change_mtu \n");//cy test
	rc = cp_init_rings(cp);		/* realloc and restart h/w */
	cp_start_hw(cp);

	spin_unlock_irqrestore(&cp->lock, flags);

	return rc;
}
#endif /* BROKEN */

static char mii_2_8139_map[8] = {
	1, /*BasicModeCtrl,*/
	1, /*BasicModeStatus,*/
	0,
	0,
	1, /*NWayAdvert,*/
	1, /*NWayLPAR,*/
	1, /*NWayExpansion,*/
	0
};

static int mdio_read(struct net_device *dev, int phy_id, int location)
{
	struct cp_private *cp = netdev_priv(dev);

#if 0
        if (location < 8 && mii_2_8139_map[location]) {	
	    cpw32(MIIAR, 0x04000000+ (location <<16));
            do
	    {		
	      mdelay(10);
	    }
	    while (!(cpr32(MIIAR) & 0x80000000));
	        
            return cpr32(MIIAR)&0xffff; 
	}    
	else
	    return 0;          

#else
	if (location < 8 && mii_2_8139_map[location]) {	
	    cpw32(MIIAR, 0x00000000 + (phy_id << 26)+ (location <<16));
            do
	    {		
	      mdelay(10);
	    }
	    while (!(cpr32(MIIAR) & 0x80000000));
	        
            return cpr32(MIIAR)&0xffff; 
	}    
	else
	    return 0;         
#endif
	       
}


static void mdio_write(struct net_device *dev, int phy_id, int location,
		       int value)
{
	struct cp_private *cp = netdev_priv(dev);
#if 0
        if (location < 8 && mii_2_8139_map[location]) {	
	    cpw32(MIIAR, 0x84000000+ (location <<16)+ value);
            do
	    {		
	      mdelay(10);
	    }
	    while (cpr32(MIIAR) & 0x80000000);  
	}
#else
        if (location < 8 && mii_2_8139_map[location]) {	
	    cpw32(MIIAR, 0x80000000 + (phy_id << 26) + (location <<16)+ value);
            do
	    {		
	      mdelay(10);
	    }
	    while (cpr32(MIIAR) & 0x80000000);  
	}
#endif
}

/* Set the ethtool Wake-on-LAN settings */
static int netdev_set_wol (struct cp_private *cp,
			   const struct ethtool_wolinfo *wol)
{

        //cyhuang reserved
	return 0;
}

/* Get the ethtool Wake-on-LAN settings */
static void netdev_get_wol (struct cp_private *cp,
	             struct ethtool_wolinfo *wol)
{
	//cyhuang reserved
}

static void cp_get_drvinfo (struct net_device *dev, struct ethtool_drvinfo *info)
{
//	struct cp_private *cp = netdev_priv(dev);

	strcpy (info->driver, DRV_NAME);
	strcpy (info->version, DRV_VERSION);
	
}

static int cp_get_regs_len(struct net_device *dev)
{
	return CP_REGS_SIZE;
}

static int cp_get_stats_count (struct net_device *dev)
{
	return CP_NUM_STATS;
}

static int cp_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct cp_private *cp = netdev_priv(dev);
	int rc;
	unsigned long flags;

	spin_lock_irqsave(&cp->lock, flags);
	rc = mii_ethtool_gset(&cp->mii_if, cmd);
	spin_unlock_irqrestore(&cp->lock, flags);

	return rc;
}

static int cp_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct cp_private *cp = netdev_priv(dev);
	int rc;
	unsigned long flags;

	spin_lock_irqsave(&cp->lock, flags);
	rc = mii_ethtool_sset(&cp->mii_if, cmd);
	spin_unlock_irqrestore(&cp->lock, flags);

	return rc;
}

static int cp_nway_reset(struct net_device *dev)
{
	struct cp_private *cp = netdev_priv(dev);
	return mii_nway_restart(&cp->mii_if);
}

static u32 cp_get_msglevel(struct net_device *dev)
{
	struct cp_private *cp = netdev_priv(dev);
	return cp->msg_enable;
}

static void cp_set_msglevel(struct net_device *dev, u32 value)
{
	struct cp_private *cp = netdev_priv(dev);
	cp->msg_enable = value;
}

static u32 cp_get_rx_csum(struct net_device *dev)
{
	struct cp_private *cp = netdev_priv(dev);  
	return (cpr16(CMD) & RxChkSum) ? 1 : 0;
}

static int cp_set_rx_csum(struct net_device *dev, u32 data)
{
	struct cp_private *cp = netdev_priv(dev);
	u16 cmd = cp->cpcmd, newcmd;

	newcmd = cmd;

	if (data)
		newcmd |= RxChkSum;
	else
		newcmd &= ~RxChkSum;

	if (newcmd != cmd) {
		unsigned long flags;

		spin_lock_irqsave(&cp->lock, flags);
		cp->cpcmd = newcmd;
		cpw16_f(CMD, newcmd);
		spin_unlock_irqrestore(&cp->lock, flags);
	}

	return 0;
}

static void cp_get_regs(struct net_device *dev, struct ethtool_regs *regs,
		        void *p)
{
	struct cp_private *cp = netdev_priv(dev);
	unsigned long flags;

	if (regs->len < CP_REGS_SIZE)
		return /* -EINVAL */;

	regs->version = CP_REGS_VER;

	spin_lock_irqsave(&cp->lock, flags);
	memcpy_fromio(p, cp->regs, CP_REGS_SIZE);
	spin_unlock_irqrestore(&cp->lock, flags);
}

static void cp_get_wol (struct net_device *dev, struct ethtool_wolinfo *wol)
{
	struct cp_private *cp = netdev_priv(dev);
	unsigned long flags;

	spin_lock_irqsave (&cp->lock, flags);
	netdev_get_wol (cp, wol);
	spin_unlock_irqrestore (&cp->lock, flags);
}

static int cp_set_wol (struct net_device *dev, struct ethtool_wolinfo *wol)
{
	struct cp_private *cp = netdev_priv(dev);
	unsigned long flags;
	int rc;

	spin_lock_irqsave (&cp->lock, flags);
	rc = netdev_set_wol (cp, wol);
	spin_unlock_irqrestore (&cp->lock, flags);

	return rc;
}

static void cp_get_strings (struct net_device *dev, u32 stringset, u8 *buf)
{
	switch (stringset) {
	case ETH_SS_STATS:
		memcpy(buf, &ethtool_stats_keys, sizeof(ethtool_stats_keys));
		break;
	default:
		BUG();
		break;
	}
}

static void cp_show_regs_datum (struct net_device *dev)
{
       struct cp_private *cp = netdev_priv(dev);
       
       {
                u8* p;
                int i;
                p = (u8*)cp->regs  ;
		printk("eth0 Regs : 0x%x ",(u32)cp->regs);                
	
                for (i=0; i<CP_REGS_SIZE; i++) {
			if (i%16 == 0)
				printk("\n %02x",*p++ ) ;
		        else
		                printk(" %02x",*p++ ) ;
		}
                printk("\n" ) ;
                p = (u8*)(cp->regs + 0x100)  ;
		printk("eth0 Regs : 0x%x ",(u32)cp->regs+0x100);
                for (i=0; i<CP_REGS_SIZE; i++) {
                        if (i%16 == 0)
                                printk("\n %02x",*p++ ) ;
                        else
                                printk(" %02x",*p++ ) ;
                }
                printk("\n" ) ;
		p = (u8*)(cp->regs + 0x200)  ;
		printk("eth0 Regs : 0x%x ",(u32)cp->regs+0x200);
		for (i=0; i<CP_REGS_SIZE; i++) {
			if (i%16 == 0)
				printk("\n %02x",*p++ ) ;
			else
				printk(" %02x",*p++ ) ;
		}
		printk("\n" ) ;
								
	        p = (u8*)(cpr32(RxFDP));
		printk("eth0 RxFDP : 0x%x ",(u32)p);
		for (i=0; i<CP_RX_RING_SIZE*16; i++) {
             		if (i%16 == 0)
				printk("\n %02x",*p++ ) ;
			else
				printk(" %02x",*p++ ) ;
		}
		printk("\n" ) ;
				



		
 	}
	
}

static void cp_get_ethtool_stats (struct net_device *dev,
				  struct ethtool_stats *estats, u64 *tmp_stats)
{
	struct cp_private *cp = netdev_priv(dev);
	int i;

	i = 0;
	
	tmp_stats[i++] = cpr16(TXOKCNT);
	tmp_stats[i++] = cpr16(RXOKCNT);
	tmp_stats[i++] = cpr16(TXERR);
	tmp_stats[i++] = cpr16(RXERRR);
	tmp_stats[i++] = cpr16(MISSPKT);
	tmp_stats[i++] = cpr16(FAE);
	tmp_stats[i++] = cpr16(TX1COL);
	tmp_stats[i++] = cpr16(TXMCOL);
	tmp_stats[i++] = cpr16(RXOKPHY);
	tmp_stats[i++] = cpr16(RXOKBRD);
	tmp_stats[i++] = cpr16(RXOKMUL);
	tmp_stats[i++] = cpr16(TXABT);
	tmp_stats[i++] = cpr16(TXUNDRN);
	tmp_stats[i++] = cp->cp_stats.rx_frags;
	if (i != CP_NUM_STATS)
		BUG();
}

static struct ethtool_ops cp_ethtool_ops = {
	.get_drvinfo		= cp_get_drvinfo,
	.get_regs_len		= cp_get_regs_len,
	.get_stats_count	= cp_get_stats_count,
	.get_settings		= cp_get_settings,
	.set_settings		= cp_set_settings,
	.nway_reset		= cp_nway_reset,
	.get_link		= ethtool_op_get_link,
	.get_msglevel		= cp_get_msglevel,
	.set_msglevel		= cp_set_msglevel,
	.get_rx_csum		= cp_get_rx_csum,
	.set_rx_csum		= cp_set_rx_csum,
	.get_tx_csum		= ethtool_op_get_tx_csum,
	.set_tx_csum		= ethtool_op_set_tx_csum, /* local! */
	.get_sg			= ethtool_op_get_sg,
	.set_sg			= ethtool_op_set_sg,
	.get_regs		= cp_get_regs,
	.get_wol		= cp_get_wol,
	.set_wol		= cp_set_wol,
	.get_strings		= cp_get_strings,
	.get_ethtool_stats	= cp_get_ethtool_stats,
	.show_regs              = cp_show_regs_datum,
};

static int cp_ioctl (struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct cp_private *cp = netdev_priv(dev);
	int rc;
	unsigned long flags;

	if (!netif_running(dev))
		return -EINVAL;

	spin_lock_irqsave(&cp->lock, flags);
	rc = generic_mii_ioctl(&cp->mii_if, if_mii(rq), cmd, NULL);
	spin_unlock_irqrestore(&cp->lock, flags);
	return rc;
}



static struct device_driver cp_driver;
static struct platform_device *network_devs;
static const char  drv_name [] = DRV_NAME;


static int cp_init_one (void)
{
	struct net_device *dev;
	struct cp_private *cp;
	int rc;
	void __iomem *regs;




#ifndef MODULE
	static int version_printed;
	if (version_printed++ == 0)
		printk("%s", version);
#endif

	dev = alloc_etherdev(sizeof(struct cp_private));
	if (!dev)
		return -ENOMEM;
	SET_MODULE_OWNER(dev);


	cp = netdev_priv(dev);

	cp->dev = dev;
	cp->msg_enable = (debug < 0 ? CP_DEF_MSG_ENABLE : debug);
	spin_lock_init (&cp->lock);
	cp->mii_if.dev = dev;
	cp->mii_if.mdio_read = mdio_read;
	cp->mii_if.mdio_write = mdio_write;
	cp->mii_if.phy_id = CP_INTERNAL_PHY;
	cp->mii_if.phy_id_mask = 0x1f;
	cp->mii_if.reg_num_mask = 0x1f;
	cp_set_rxbufsize(cp);

	
	regs =(void *)0xb8016000 ;
	dev->base_addr = (unsigned long) regs;
	cp->regs = regs;

	// cp_stop_hw(cp);

	
        /* default MAC address */
        ((u16 *) (dev->dev_addr))[0] =0x0000;
	((u16 *) (dev->dev_addr))[1] =0x0000;
        ((u16 *) (dev->dev_addr))[2] =0x0000;
	((u16 *) (dev->dev_addr))[3] =0x0000;	  
	/* read MAC address from FLASH */   
        {
	   char *env_base;	
	   env_base = platform_info.ethaddr;
	   printk("MAC address = 0x%s \n",env_base);//cy test
   	   
           {
                
                dev->dev_addr[0] = (u8)simple_strtoul( env_base, NULL, 16 );
               

                dev->dev_addr[1] = (u8)simple_strtoul( env_base+3, NULL, 16 );
              

                dev->dev_addr[2] = (u8)simple_strtoul( env_base+6, NULL, 16 );
               

                dev->dev_addr[3] = (u8)simple_strtoul( env_base+9, NULL, 16 );
               

          
                dev->dev_addr[4] = (u8)simple_strtoul( env_base+12, NULL, 16 );
              

                dev->dev_addr[5] = (u8)simple_strtoul( env_base+15, NULL, 16 );
               

                

                dev->dev_addr[6] = (u8)(0x00);
                dev->dev_addr[7] = (u8)(0x00);                               

           }       

         }



	 

	dev->open = cp_open;
	dev->stop = cp_close;
	dev->set_multicast_list = cp_set_rx_mode;
	dev->hard_start_xmit = cp_start_xmit;
	dev->get_stats = cp_get_stats;
	dev->do_ioctl = cp_ioctl;
	dev->poll = cp_rx_poll;
	dev->weight = 8;/*cy huang modified "64" */	/* arbitrary? from NAPI_HOWTO.txt. */
#ifdef BROKEN
	dev->change_mtu = cp_change_mtu;
#endif
	dev->ethtool_ops = &cp_ethtool_ops;
#if 0
	dev->tx_timeout = cp_tx_timeout;
	dev->watchdog_timeo = TX_TIMEOUT;
#endif

#if CP_VLAN_TAG_USED
	dev->features |= NETIF_F_HW_VLAN_TX | NETIF_F_HW_VLAN_RX;
	dev->vlan_rx_register = cp_vlan_rx_register;
	dev->vlan_rx_kill_vid = cp_vlan_rx_kill_vid;
#endif

	dev->features |= NETIF_F_HIGHDMA;

	dev->flags |= IFF_PROMISC;
	dev->irq = 2;    //cyhuang modified

	rc = register_netdev(dev);
	if (rc)
		goto err_out_iomap;

	printk (KERN_INFO "%s: RTL-8139C+ at 0x%lx, "
		"%02x:%02x:%02x:%02x:%02x:%02x, "
		"IRQ %d\n",
		dev->name,
		dev->base_addr,
		dev->dev_addr[0], dev->dev_addr[1],
		dev->dev_addr[2], dev->dev_addr[3],
		dev->dev_addr[4], dev->dev_addr[5],
		dev->irq);
#ifdef CONFIG_PM    	

        network_devs = platform_device_register_simple((char *)drv_name,
	                    -1, NULL, 0);
         
        if (IS_ERR(network_devs)) {
		rc = PTR_ERR(network_devs);
		return rc;
	}
	 
	dev_set_drvdata(&network_devs->dev, dev);  
	
	driver_register (&cp_driver);
#endif
	return 0;

err_out_iomap:
	iounmap(regs);


	free_netdev(dev);
	return rc;
}

#ifdef CONFIG_PM


static int cp_suspend (struct dev *p_dev, pm_message_t state)
{

	struct net_device *dev;
//	struct cp_private *cp;
//	unsigned long flags;
        if (cp_status == CP_ON) {
	dev = dev_get_drvdata (p_dev);
        cp_status = CP_OFF ;
	cp_close(dev);
       
	}
	
#if 0	
	cp  = netdev_priv(dev);

        if (!dev || !netif_running (dev)) return 0;
        netif_device_detach (dev);
        netif_stop_queue (dev);
        spin_lock_irqsave (&cp->lock, flags);
       /* Disable Rx and Tx */
	cpw16_f(IMR, 0);
	cpw32(IO_CMD, 0);
	cpw16_f(ISR, 0xffff);
        cp_clean_rings(cp);
	printk("close IO_CMD of net \n");//cy test
	
        spin_unlock_irqrestore (&cp->lock, flags);

#endif
        return 0;
}


static int cp_resume (struct dev *p_dev)
{
	
	struct net_device *dev;
//	struct cp_private *cp;
     
	     
	if (cp_status ==0) {
	dev = dev_get_drvdata (p_dev);
	if (!dev)
	      return 0;
	cp_status =CP_UNDER_INIT;
	cp_open(dev);
	cp_status = CP_ON;
	}
#if 0	
	cp  = netdev_priv(dev);

	netif_device_attach (dev);
	printk("init hardware of net\n");//cy test
	cp_init_rings(cp);
	printk("after init_rings \n");//cy test
        cp_init_hw (cp);
        netif_start_queue (dev);
#endif
    
        return 0;
}

#endif


static struct device_driver cp_driver = {
	        .name         = (char *)drv_name,
		.bus    =       &platform_bus_type,
#ifdef CONFIG_PM
                .resume       = cp_resume,
		.suspend      = cp_suspend,
#endif
};


static void __exit cp_exit (void)
{
	
}

module_init(cp_init_one);
module_exit(cp_exit);
