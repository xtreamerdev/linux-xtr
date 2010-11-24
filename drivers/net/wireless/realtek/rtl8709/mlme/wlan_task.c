/*

	MIPS-16 Mode if CONFIG_MIPS16 is on

*/

#define _RTL8187_RELATED_FILES_C

#define _WLAN_TASK_C_
#include <drv_types.h>
#include <rtl8187/irp.h>

void irp2mgntframe(struct txirp *irp, struct mgnt_frame *pmgntframe)
{
	
	u8 *mgnt_payload_addr;

	struct pkt_attrib *pattrib = &(pmgntframe->attrib);
	mgnt_payload_addr = get_xmit_payload_start_addr((struct xmit_frame *)pmgntframe);
	_memcpy(mgnt_payload_addr, (void*)(irp->txbuf_phyaddr), irp->tx_len);

	//printk("irp2mgntframe: dump mgnt_payload_addr\n");
	pattrib->encrypt=0;	
	pattrib->nr_frags = 1;
	pattrib->last_txcmdsz = irp->tx_len;
	//ClearMFrag(mgnt_payload_addr);
}

//mgnt tx flow
//alloc_mgntframe --> transfer packet from irp to mgntframe --> enqueue
int	wlan_tx (_adapter *padapter, struct txirp *irp)
{

	_irqL 	irqL;
	int 	res = _SUCCESS;
	struct mgnt_frame *pmgntframe;
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);

	DEBUG_INFO(("wlan_tx\n"));

	pmgntframe = alloc_mgnt_xmitframe(pxmitpriv);
	if(pmgntframe==NULL)
	{
		free_txirp(padapter, irp);
		DEBUG_ERR(("wlan_tx: alloc_mgnt_xmitframe fail\n"));
		res = _FAIL;
		goto _wlan_tx_drop;
	}


	irp2mgntframe(irp, pmgntframe);
	free_txirp(padapter, irp);
	
	_enter_critical(&pxmitpriv->lock, &irqL);
	if ((txframes_pending_nonbcmc(padapter)) == _FALSE){
		if ((xmit_classifier_direct(padapter, (struct xmit_frame *)pmgntframe)) == _FAIL)		
		{
			DEBUG_ERR(("drop xmit pkt for classifier fail\n"));
			pmgntframe->pkt = NULL;
			_exit_critical(&pxmitpriv->lock, &irqL);	
			res = _FAIL;
			goto _wlan_tx_drop;
		}

#ifdef LINUX_TX_TASKLET
		tasklet_schedule(&padapter->irq_tx_tasklet);
#else
		DEBUG_ERR((" wlan_tx: _up_sema\n"));
		_up_sema(&pxmitpriv->xmit_sema);
#endif
	}
	else{
		if ((xmit_classifier_direct(padapter, (struct xmit_frame *)pmgntframe)) == _FAIL)		
		{
	
			DEBUG_ERR(("drop xmit pkt for classifier fail\n"));
			pmgntframe->pkt = NULL;
			_exit_critical(&pxmitpriv->lock, &irqL);
			res = _FAIL;
			goto _wlan_tx_drop;			
		}

#ifdef LINUX_TX_TASKLET
		tasklet_schedule(&padapter->irq_tx_tasklet);
#endif
	}

	_exit_critical(&pxmitpriv->lock, &irqL);
	
_wlan_tx_drop:
	return res;

}

#if 0
#include <rtl8711_spec.h>
#include <string.h>
#include <sched.h>
#include <types.h>
#include <console.h>
#include <system32.h>
#include <sched.h>
#include <wlan.h>
#include <wlan_mlme.h>
#include <wlan_util.h>
#include <wlan_sme.h>
#include <RTL8711_RFCfg.h>
#include <wlan_offset.h>
#include <timer.h>
#include <circ_buf.h>
#include <tasks.h>
#include <dma.h>
#include <irp.h>
#include <wireless.h>
#include <ieee80211.h>
#include <wifi.h>

#ifdef CONFIG_FW_WLAN_LBK
#include <mp_pktinf.h>
#endif

#if (MIB_STAINFOOFFSET != 24)
#error "StaInfo Offset must be 24\n"
#endif

//#define	MGT_CRTL_QUEUE

#ifdef CONFIG_POWERSAVING
#include <power_saving.h>
#endif

#ifdef CONFIG_RATEADAPTIVE
#include <RTL8711_rateadaptive.h>
#endif

#ifdef CONFIG_DIG
#include <RTL8711_dig.h>
#endif

struct	rxbufhead_pool	rxbufheadpool;
struct	txbufhead_pool	txbufheadpool;

unsigned char pwrstatus[(CAMSIZE/8)+1] ;

unsigned char cck_tssi[NUM_CHANNELS] = {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f};
unsigned char ofdm_tssi[NUM_CHANNELS] = {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f};

#ifdef CONFIG_TSSI_METHOD	
char	delta_of_tx_pwr = 0;
unsigned int	sum_of_cck_tssi = 0, sum_of_ofdm_tssi = 0;
unsigned int	cnt_of_cck_tssi = 0, cnt_of_ofdm_tssi = 0;
#endif

#ifdef CONFIG_SCSI_READ_FIFO
int Seq_Scsi = 0;
extern volatile unsigned int	c2hbuf[C2HSZ + 1];
#endif

//unsigned char	fwtxcmd[280] __fwtxcmd__ ;

// txcmd: 0~ 43F  ==> total 1088 bytes 1088/16= 68 entries 68 * 4= 272 bytes

/*
	The format of fwtxcmd:
	Bit31 ~ Bit27 	Mac-Id
	Bit23 ~ Bit16 	txrate_inx;
	Bit15 			tx_antenna
	Bit7  ~ Bit0  rsvd
*/


extern volatile unsigned int	_h2cbuf_[];
extern volatile unsigned int	_h2crsp_[];
extern volatile unsigned int	c2hbuf[];

extern volatile unsigned int	*h2cbuf;
extern volatile unsigned int	*h2crsp;

extern volatile unsigned char	lbk_mode;
extern volatile unsigned char	hci_sel;
extern volatile unsigned char	chip_version;
extern volatile unsigned char	dig;
extern volatile unsigned char	rate_adaptive;
extern volatile unsigned char	vcsMode;
extern volatile unsigned char	vcsType;

struct hwcmd_queue	*hwqueues[HW_QUEUE_ENTRY] = 
{&(sys_mib.vo_queue), &(sys_mib.vi_queue), &(sys_mib.be_queue), &(sys_mib.bk_queue),
 &(sys_mib.ts_queue), &(sys_mib.mgt_queue), &(sys_mib.bmc_queue), &(sys_mib.bcn_queue)};

int Scsi_Byte;
int Scsi_End;

#define ID2HWCMDQ(id)	((struct hwcmd_queue *)\
	((unsigned int)(&sys_mib.vo_queue) + (id) * sizeof (struct hwcmd_queue)))


#ifdef CONFIG_FW_WLAN_LBK
PACKET_INFO pktinfo;
unsigned char	 txqinx= 0;
#endif

#ifdef CONFIG_POWERTRACKING
#ifdef CONFIG_THERMAL_METHOD
unsigned int thermal_meter;
unsigned int initial_thermal_meter;
static char get_thermal_meter(void);
#endif
void pt_timer_hdl(unsigned char *);
#endif

//When TKIP/AES key mapping key, key are 16 bytes long.
// And they are derived from the following
/*
Key0 ~ Key15 = {mac0 ~ mac5: mac0 ^ 0xff ~ mac5 ^ 0xff: mac5: mac4: mac3:mac2 }


*/

__init_fun__ static void wlan_trx_swinit(void)
{
	int i,j;
	unsigned int	addr, starting;
	
	sys_mib.vo_queue.io_base = 	VOCMD;
	sys_mib.vo_queue.ff_port = 	VOFFWP;
	sys_mib.vo_queue.q_head  = &(sys_mib.voirp_list);
	sys_mib.vo_queue.tail = &(sys_mib.mac_ctrl.txcmds[0].tail);
	sys_mib.vo_queue.sz = VOCMD_ENTRYCNT;
	sys_mib.vo_queue.queue_id = VO_QUEUE_INX;
	sys_mib.vo_queue.dma_id = WLANTX_VO_DMA;
	sys_mib.vo_queue.dma_flying = &sys_mib.tx_vivo_dmaflying;
	
	
	sys_mib.vi_queue.io_base = 	VICMD;
	sys_mib.vi_queue.ff_port = 	VIFFWP;
	sys_mib.vi_queue.q_head  = &(sys_mib.viirp_list);
	sys_mib.vi_queue.tail = &(sys_mib.mac_ctrl.txcmds[1].tail);
	sys_mib.vi_queue.sz = VICMD_ENTRYCNT;
	sys_mib.vi_queue.queue_id = VI_QUEUE_INX;
	sys_mib.vi_queue.dma_id = WLANTX_VI_DMA;
	sys_mib.vi_queue.dma_flying = &sys_mib.tx_vivo_dmaflying;
	
	sys_mib.be_queue.io_base = 	BECMD;
	sys_mib.be_queue.ff_port = 	BEFFWP;
	sys_mib.be_queue.q_head  = &(sys_mib.beirp_list);
	sys_mib.be_queue.tail = &(sys_mib.mac_ctrl.txcmds[2].tail);
	sys_mib.be_queue.sz = BECMD_ENTRYCNT;
	sys_mib.be_queue.queue_id = BE_QUEUE_INX;
	sys_mib.be_queue.dma_id = WLANTX_BE_DMA;
	sys_mib.be_queue.dma_flying = &sys_mib.tx_bebk_dmaflying;
	
	sys_mib.bk_queue.io_base = 	BKCMD;
	sys_mib.bk_queue.ff_port = 	BKFFWP;
	sys_mib.bk_queue.q_head  = &(sys_mib.bkirp_list);
	sys_mib.bk_queue.tail = &(sys_mib.mac_ctrl.txcmds[3].tail);
	sys_mib.bk_queue.sz = BKCMD_ENTRYCNT;
	sys_mib.bk_queue.queue_id = BK_QUEUE_INX;
	sys_mib.bk_queue.dma_id = WLANTX_BK_DMA;
	sys_mib.bk_queue.dma_flying = &sys_mib.tx_bebk_dmaflying;
	
	sys_mib.ts_queue.io_base = 	TSCMD;
	sys_mib.ts_queue.ff_port = 	TSFFWP;
	sys_mib.ts_queue.q_head  = &(sys_mib.tsirp_list);
	sys_mib.ts_queue.tail = &(sys_mib.mac_ctrl.txcmds[4].tail);
	sys_mib.ts_queue.sz = TSCMD_ENTRYCNT;
	sys_mib.ts_queue.queue_id = TS_QUEUE_INX;
	sys_mib.ts_queue.dma_id = WLANTX_TS_DMA;
	sys_mib.ts_queue.dma_flying = &sys_mib.tx_ts_dmaflying;
	
	sys_mib.mgt_queue.io_base = MGTCMD;
	sys_mib.mgt_queue.ff_port = MGTFFWP;
	sys_mib.mgt_queue.q_head  = &(sys_mib.mgtirp_list);
	sys_mib.mgt_queue.tail = &(sys_mib.mac_ctrl.txcmds[5].tail);
	sys_mib.mgt_queue.sz = MGTCMD_ENTRYCNT;
	sys_mib.mgt_queue.queue_id = MGT_QUEUE_INX;
	sys_mib.mgt_queue.dma_id = WLANTX_MGTBCN_DMA;
	sys_mib.mgt_queue.dma_flying = &sys_mib.tx_mgt_dmaflying;
	
	sys_mib.bmc_queue.io_base = BMCMD;
	sys_mib.bmc_queue.ff_port = BMFFWP;
	sys_mib.bmc_queue.q_head  = &(sys_mib.bmcirp_list);
	sys_mib.bmc_queue.tail = &(sys_mib.mac_ctrl.txcmds[6].tail);
	sys_mib.bmc_queue.sz = BMCMD_ENTRYCNT;
	sys_mib.bmc_queue.queue_id = BMC_QUEUE_INX;
	sys_mib.bmc_queue.dma_id = WLANTX_BMC_DMA;
	sys_mib.bmc_queue.dma_flying = &sys_mib.tx_bmc_dmaflying;
	
	// For sta_data
	
	
	// For TX
	INIT_LIST_HEAD (&(sys_mib.voirp_list));
	INIT_LIST_HEAD (&(sys_mib.viirp_list));
	INIT_LIST_HEAD (&(sys_mib.beirp_list));
	INIT_LIST_HEAD (&(sys_mib.bkirp_list));
	INIT_LIST_HEAD (&(sys_mib.tsirp_list));
	INIT_LIST_HEAD (&(sys_mib.mgtirp_list));
	INIT_LIST_HEAD (&(sys_mib.bmcirp_list));
	
	
	
	INIT_LIST_HEAD (&(sys_mib.free_txirp_mgt));
	INIT_LIST_HEAD (&(sys_mib.free_txirp_small));
	INIT_LIST_HEAD (&(sys_mib.free_txirp_medium));
	INIT_LIST_HEAD (&(sys_mib.free_txirp_large));
#if 0	
	INIT_LIST_HEAD (&(sys_mib.free_txirp_clone));
#endif
	
	INIT_LIST_HEAD (&(sys_mib.tx_bebk_dmaflying));
	INIT_LIST_HEAD (&(sys_mib.tx_vivo_dmaflying));
	INIT_LIST_HEAD (&(sys_mib.tx_mgt_dmaflying));
	INIT_LIST_HEAD (&(sys_mib.tx_bmc_dmaflying));
	INIT_LIST_HEAD (&(sys_mib.tx_ts_dmaflying));



    // For RX
    INIT_LIST_HEAD (&(sys_mib.free_rxirp_mgt));
    INIT_LIST_HEAD (&(sys_mib.free_rxirp_small));
    INIT_LIST_HEAD (&(sys_mib.free_rxirp_medium));
    INIT_LIST_HEAD (&(sys_mib.free_rxirp_large));
	INIT_LIST_HEAD (&(sys_mib.free_rxirp_trash));

    INIT_LIST_HEAD (&(sys_mib.rx_datalist));
    INIT_LIST_HEAD (&(sys_mib.rx_mgtctrllist));

    INIT_LIST_HEAD (&(sys_mib.rx_dmaflying));
	
	//marc@0619
	INIT_LIST_HEAD (&(sys_mib.rx_dma_list));
    
#ifdef CONFIG_SCSI_READ_FIFO
	INIT_LIST_HEAD (&(sys_mib.rx_scsi_dma_list));
	INIT_LIST_HEAD (&(sys_mib.rx_scsi_dmaflying));  
#endif
    
#ifdef CONFIG_SCSI_WRITE_FIFO
	INIT_LIST_HEAD(&(sys_mib.rx_scsi_write_list));
	INIT_LIST_HEAD(&(sys_mib.rx_scsi_write_dmaflying));
#endif    
    
    //marc@0621
#ifdef CONFIG_IMEM    
	starting = addr = ( (unsigned int)(&_START_IMEM_)  & 0x3ffff) | (0x1fc00000);
#elif defined (CONFIG_DMEM)
	starting = addr = ( (unsigned int)(&_START_DMEM_)  & 0x3ffff) | (0x1fc00000);
#else    
    starting = addr = (unsigned int)(Virtual2Physical(&_end));
#endif    
	
	for(i = 0 ; i < TXMGTIRPS_MGT; i++, addr += TXMGTBUF_SZ)	//MGT Buf will take away 4KB
	{
		INIT_LIST_HEAD(&(txbufheadpool.mgt.irps[i].list));
		txbufheadpool.mgt.irps[i].tag = TXMGTIRPS_TAG;
		txbufheadpool.mgt.irps[i].txbuf_phyaddr = addr;
		list_add_tail(&(txbufheadpool.mgt.irps[i].list), &sys_mib.free_txirp_mgt);
	}

//marc@0621
#ifdef CONFIG_IMEM_4K
	#ifdef CONFIG_DMEM
	starting = addr = ( (unsigned int)(&_START_DMEM_) & 0x3ffff) | (0x1fc00000);
	#else
		starting = addr = (unsigned int)(Virtual2Physical(&_end));
	#endif

#elif (!defined (CONFIG_IMEM) && defined(CONFIG_DMEM_4K))
	starting = addr = (unsigned int)(Virtual2Physical(&_end));
#endif	

	for(i = 0 ; i < TXDATAIRPS_SMALL; i++, addr += TXDATAIRPS_SMALL_SZ)	//Small Buf will take away 4KB
	{
		INIT_LIST_HEAD(&(txbufheadpool.small.irps[i].list));
		txbufheadpool.small.irps[i].tag = TXDATAIRPS_SMALL_TAG;
		txbufheadpool.small.irps[i].txbuf_phyaddr = addr;
		list_add_tail(&(txbufheadpool.small.irps[i].list), &sys_mib.free_txirp_small);
	}
	
	//marc@0621
#ifdef CONFIG_IMEM_8K
	#ifdef CONFIG_DMEM_8K
		j = (8*1024/TXDATAIRPS_MEDIUM_SZ);
		starting = addr = ( (unsigned int)(&_START_DMEM_) & 0x3ffff) | (0x1fc00000);	
	#elif defined (CONFIG_DMEM_4K)
		j = (4*1024/TXDATAIRPS_MEDIUM_SZ);
		starting = addr = ( (unsigned int)(&_START_DMEM_) & 0x3ffff) | (0x1fc00000);	
	#else
		starting = addr = (unsigned int)(Virtual2Physical(&_end));
	#endif
#elif defined (CONFIG_IMEM_4K)
	#ifdef CONFIG_DMEM_8K
		j = (4*1024/TXDATAIRPS_MEDIUM_SZ);
	#elif defined (CONFIG_DMEM_4K)
		j = TXDATAIRPS_MEDIUM;
		starting = addr = (unsigned int)(Virtual2Physical(&_end));
	#endif
#else
	#ifdef CONFIG_DMEM_8K
	starting = addr = (unsigned int)(Virtual2Physical(&_end));
	#endif
	j = TXDATAIRPS_MEDIUM;

#endif

	for(i = 0; i < j; i++, addr += TXDATAIRPS_MEDIUM_SZ)
	{
		INIT_LIST_HEAD(&(txbufheadpool.medium.irps[i].list));
		txbufheadpool.medium.irps[i].tag = TXDATAIRPS_MEDIUM_TAG;
		txbufheadpool.medium.irps[i].txbuf_phyaddr = addr;
		list_add_tail(&(txbufheadpool.medium.irps[i].list), &sys_mib.free_txirp_medium);
	}

	//marc@0621
#if (defined(CONFIG_IMEM_8K) && defined(CONFIG_DMEM_4K)) || (defined(CONFIG_IMEM_4K) && defined(CONFIG_DMEM_8K))
	for(addr = (unsigned int)(Virtual2Physical(&_end)); i < TXDATAIRPS_MEDIUM; i++, addr += TXDATAIRPS_MEDIUM_SZ)
	{
		INIT_LIST_HEAD(&(txbufheadpool.medium.irps[i].list));
		txbufheadpool.medium.irps[i].tag = TXDATAIRPS_MEDIUM_TAG;
		txbufheadpool.medium.irps[i].txbuf_phyaddr = addr;
		list_add_tail(&(txbufheadpool.medium.irps[i].list), &sys_mib.free_txirp_medium);
	}

#endif

//marc@0621
#if (defined(CONFIG_IMEM_8K) && defined(CONFIG_DMEM_8K))
	starting = addr = (unsigned int)(Virtual2Physical(&_end));
#endif

	for(i = 0; i < TXDATAIRPS_LARGE; i++, addr += TXDATAIRPS_LARGE_SZ)
	{
		INIT_LIST_HEAD(&(txbufheadpool.large.irps[i].list));
		txbufheadpool.large.irps[i].tag = TXDATAIRPS_LARGE_TAG;
		txbufheadpool.large.irps[i].txbuf_phyaddr = addr;
		list_add_tail(&(txbufheadpool.large.irps[i].list), &sys_mib.free_txirp_large);
	}

#if 0
	for(i = 0; i < TXDATAIRPS_LARGE; i++)
	{
		INIT_LIST_HEAD(&(txbufheadpool.clone.irps[i].list));
		txbufheadpool.clone.irps[i].tag = TXDATAIRPS_CLONE_TAG;
		txbufheadpool.clone.irps[i].txbuf_phyaddr = 0;
		list_add_tail(&(txbufheadpool.clone.irps[i].list), &sys_mib.free_txirp_clone);
	}
#endif

	for(i = 0 ; i < RXMGTIRPS_MGT; i++, addr += RXMGTBUF_SZ)	//MGT Buf will take away 4KB
	{
		INIT_LIST_HEAD(&(rxbufheadpool.mgt.irps[i].list));
		rxbufheadpool.mgt.irps[i].tag = RXMGTIRPS_TAG;
		rxbufheadpool.mgt.irps[i].rxbuf_phyaddr = addr;
		list_add_tail(&(rxbufheadpool.mgt.irps[i].list), &sys_mib.free_rxirp_mgt);
	}
	
	for(i = 0 ; i < RXDATAIRPS_SMALL; i++, addr += RXDATAIRPS_SMALL_SIZE)	//MGT Buf will take away 4KB
	{
		INIT_LIST_HEAD(&(rxbufheadpool.small.irps[i].list));
		rxbufheadpool.small.irps[i].tag = RXDATAIRPS_SMALL_TAG;
		rxbufheadpool.small.irps[i].rxbuf_phyaddr = addr;
		list_add_tail(&(rxbufheadpool.small.irps[i].list), &sys_mib.free_rxirp_small);
	}

	for(i = 0 ; i < RXDATAIRPS_MEDIUM; i++, addr += RXDATAIRPS_MEDIUM_SIZE)	//MGT Buf will take away 4KB
	{
		INIT_LIST_HEAD(&(rxbufheadpool.medium.irps[i].list));
		rxbufheadpool.medium.irps[i].tag = RXDATAIRPS_MEDIUM_TAG;
		rxbufheadpool.medium.irps[i].rxbuf_phyaddr = addr;
		list_add_tail(&(rxbufheadpool.medium.irps[i].list), &sys_mib.free_rxirp_medium);
	}
	
	for(i = 0 ; i < RXDATAIRPS_LARGE; i++, addr += RXDATAIRPS_LARGE_SIZE)	//MGT Buf will take away 4KB
	{
		INIT_LIST_HEAD(&(rxbufheadpool.large.irps[i].list));
		rxbufheadpool.large.irps[i].tag = RXDATAIRPS_LARGE_TAG;
		rxbufheadpool.large.irps[i].rxbuf_phyaddr = addr;
		list_add_tail(&(rxbufheadpool.large.irps[i].list), &sys_mib.free_rxirp_large);
	}

	//marc@0923
	for(i = 0 ; i < RXDATAIRPS_TRASH; i++)
	{
		INIT_LIST_HEAD(&(rxbufheadpool.trash.irps[i].list));
		rxbufheadpool.trash.irps[i].tag = RXDATAIRPS_TRASH_TAG;
		rxbufheadpool.trash.irps[i].rxbuf_phyaddr = addr;
		list_add_tail(&(rxbufheadpool.trash.irps[i].list), &sys_mib.free_rxirp_trash);
	}

	_end = Physical2Virtual(addr);

	// Below is for initializing rx0swcmdq and rx1swcmdq
	
	sys_mib.rx0swcmdq.head = (volatile unsigned int)&(sys_mib.rx0swcmdq.swcmds[0]);
	sys_mib.rx0swcmdq.tail = (volatile unsigned int)&(sys_mib.rx0swcmdq.swcmds[0]);
	
	sys_mib.rx1swcmdq.head = (volatile unsigned int)&(sys_mib.rx1swcmdq.swcmds[0]);
	sys_mib.rx1swcmdq.tail = (volatile unsigned int)&(sys_mib.rx1swcmdq.swcmds[0]);



	// Below is for CAM management
	
	
	for(i = 0; i < MAX_STASZ; i++)
	{
		INIT_LIST_HEAD(&(sys_mib.hash_array[i]));	
		//sys_mib.cam_array[i].sta = NULL;
	}

	for(i = 0; i < MAX_STASZ; i++)
	{
		stadata[i].status = -1;
		stadata[i]._protected = 0;
		stadata[i].info = &(sys_mib.stainfos[i]);
		stadata[i].info2 = &(sys_mib.stainfo2s[i]);
		stadata[i].stats = &(sys_mib.stainfostats[i]);
		stadata[i].rxcache = &(sys_mib.rxcache[i]);
		sys_mib.stainfo2s[i].aid = i;
		
		INIT_LIST_HEAD (&(stadata[i].hash));
		INIT_LIST_HEAD (&(stadata[i].assoc));
		INIT_LIST_HEAD (&(stadata[i].sleep));
		
	}
	
}




/*

struct cmd_ctrl {
volatile	int	head;
volatile	int 	mask_head;
volatile	int	tail;
volatile	int	mask_tail;	// make the size of cmd_ctrl to be the multiples of 8
volatile    unsigned int	baseaddr;
};

*/

__init_fun__ static void txcmd_swinit(struct cmd_ctrl *pcmdctrl, volatile	int	head, \
volatile	int mask_head, volatile int	tail,\
volatile int	baseaddr, volatile  unsigned int	mask_tail)
{
	
	pcmdctrl->head = head;
	pcmdctrl->mask_head = mask_head;
	pcmdctrl->tail = tail;
	pcmdctrl->baseaddr = baseaddr;
	pcmdctrl->mask_tail = mask_tail;	
	
	
} 

__init_fun__ void wlan_swinit(void)
{
	int i;
	
	h2cbuf = (volatile unsigned int *)(((unsigned int)_h2cbuf_) | 0xa0000000);
	h2crsp = (volatile unsigned int *)(((unsigned int)_h2crsp_) | 0xa0000000);

	txcmd_swinit(&(sys_mib.mac_ctrl.txcmds[0]), 
	0xBDE00000, 0xBDE00000, 0xBDE00000, 0xBDE00000, 0xBDE00080);

	txcmd_swinit(&(sys_mib.mac_ctrl.txcmds[1]), 	
	0xBDE00080, 0xBDE00080, 0xBDE00080, 0xBDE00080, 0xBDE00100);

	txcmd_swinit(&(sys_mib.mac_ctrl.txcmds[2]), 
	0xBDE00100, 0xBDE00100, 0xBDE00100, 0xBDE00100, 0xBDE00180);

	txcmd_swinit(&(sys_mib.mac_ctrl.txcmds[3]), 
	0xBDE00180, 0xBDE00180, 0xBDE00180, 0xBDE00180, 0xBDE00200);

	txcmd_swinit(&(sys_mib.mac_ctrl.txcmds[4]), 
	0xBDE00200, 0xBDE00200, 0xBDE00200, 0xBDE00200, 0xBDE00240);

	txcmd_swinit(&(sys_mib.mac_ctrl.txcmds[5]), 
	0xBDE00240, 0xBDE00240, 0xBDE00240, 0xBDE00240, 0xBDE00280);

	txcmd_swinit(&(sys_mib.mac_ctrl.txcmds[6]), 
	0xBDE00280, 0xBDE00280, 0xBDE00280, 0xBDE00280, 0xBDE002c0);

	sys_mib.mac_ctrl.rxcmds[0].head =		0xBDE00300;
	//sys_mib.mac_ctrl.rxcmds[0].mask_head=	0xFFFFFBFF;		/* 300 ~ 3FF */
	sys_mib.mac_ctrl.rxcmds[0].mask_head =	0xBDE00400; //0413

	sys_mib.mac_ctrl.rxcmds[1].head =		0xBDE00400;
	sys_mib.mac_ctrl.rxcmds[1].mask_head =	0xBDE00440; //0413

#ifdef CONFIG_RATEADAPTIVE
	/* sunghau@0328: init rate adaptive tables */
	for(i=0;i<NumRates;i++){
		sys_mib.rf_ctrl.ss_ForceUp[i] = ulevel[i];
		sys_mib.rf_ctrl.ss_ULevel[i] = dlevel[i];
		sys_mib.rf_ctrl.ss_DLevel[i] = dlevel[i];
		sys_mib.rf_ctrl.count_judge[i] = count_judge[i];
	}

	timer_init(&sys_mib.ra_timer, NULL, &ra_timer_hdl);
	//sys_mib.ra_timer.time_out = 30;
#endif

	switch (vcsMode)
	{
		case 2:
			dot11ErpInfo.protection = vcsType;
			break;
			
		case 1:
		case 0:
		default:
			dot11ErpInfo.protection = 0;
			break;
	}

	rf_init();

	setup_rate_tbl();

	mgt_init();
	
	wlan_trx_swinit();

#ifdef CONFIG_DIG
	dig_timer_init();
#endif

#if 0
	rf_init();

	setup_rate_tbl();
#endif
	
}
#define VERIFY_TRXCMD



#ifdef VERIFY_TRXCMD

#define SEED0	(0xffffffff)
#define SEED1	(0x78efab5a)
#define SEED2	(0x7f55cd43)
#define SEED3	(0xf92f420d)

__init_fun__ int verify_trxcmd(unsigned int	start_addr, unsigned int entry_cnt)
{
	unsigned int	i, val,ret = SUCCESS;
	unsigned int	addr = start_addr;
		
	for(i = 0 ; i < entry_cnt; i++){
		rtl_outl(addr, SEED0);
		addr += 4;
		rtl_outl(addr, SEED1);
		addr += 4;
		rtl_outl(addr, SEED2);
		addr += 4;
		rtl_outl(addr, SEED3);
		addr += 4;	
		
	}	

	addr = start_addr;
	
	for(i = 0 ; i < entry_cnt; i++){
		
		val = rtl_inl(addr);
		if (val != SEED0)
		{
			printk("%x R/W error! val:%x(SEED0)\n",addr, val);
			ret = FAIL;
		}
		addr += 4;
		
		val = rtl_inl(addr);
		if (val != SEED1)
		{
			printk("%x R/W error! val:%x(SEED1)\n",addr, val);
			ret = FAIL;
		}

		addr += 4;
		val = rtl_inl(addr);
		if (val != SEED2)
		{
			printk("%x R/W error! val:%x(SEED2)\n", addr, val);
			ret = FAIL;	
		}
		addr += 4;
				
		val = rtl_inl(addr);
		if (val != SEED3)
		{
			printk("%x R/W error! val:%x(SEED3)\n", addr, val);
			ret = FAIL;
		}
		
		addr += 4;
	}	

	return ret;
}	

#endif


__init_fun__ void wlan_hwinit(void)
{

	unsigned int i;
	unsigned int 		val;
	//unsigned short	ctrl;
	unsigned char 	*myaddr;

	//turn on MAC CLK and BB CLK
	rtl_outb(BBCLKENABLE, BIT(0));
	for(i = 0; i < 0x10; i++);
    
	rtl_outb(MACCLKENABLE, BIT(0));
	for(i = 0; i < 0x10; i++);

	rtl_outb(CR, APSDOFF | RST);
	for(i = 0; i< 0x40; i++);

	// to turn on LED (LINK/ACT, infrastructure)
	rtl_outb(CR, TXEN | RXEN |  BIT(1) | BIT(0));
	for(i = 0; i< 0x40; i++);
	
#ifdef VERIFY_TRXCMD

	printk("Beginning of TRXCMD Verify\n");

	if ((verify_trxcmd(VOCMD, VOCMD_ENTRYCNT)) == FAIL)
		printk("VOCMD Verify Fail\n");


	if ((verify_trxcmd(VICMD, VICMD_ENTRYCNT)) == FAIL)
		printk("VICMD Verify Fail\n");
		
	if ((verify_trxcmd(BECMD, BECMD_ENTRYCNT)) == FAIL)
		printk("BECMD Verify Fail\n");

	if ((verify_trxcmd(BKCMD, BKCMD_ENTRYCNT)) == FAIL)
		printk("BKCMD Verify Fail\n");		

	if ((verify_trxcmd(TSCMD, TSCMD_ENTRYCNT)) == FAIL)
		printk("TSCMD Verify Fail\n");

	if ((verify_trxcmd(MGTCMD, MGTCMD_ENTRYCNT)) == FAIL)
		printk("MGTCMD Verify Fail\n");

	if ((verify_trxcmd(BMCMD, BMCMD_ENTRYCNT)) == FAIL)
		printk("BMCMD Verify Fail\n");
		
	if ((verify_trxcmd(BCNCMD, BCNCMD_ENTRYCNT)) == FAIL)
		printk("BCNCMD Verify Fail\n");

	if ((verify_trxcmd(RXCMD0, RXCMD0_ENTRYCNT)) == FAIL)
		printk("RXCMD0 Verify Fail\n");

	if ((verify_trxcmd(RXCMD1, RXCMD1_ENTRYCNT)) == FAIL)
		printk("RXCMD1 Verify Fail\n");	

	
	// To Clear all the pending interrupt
	rtl_outw(SYSIMR, 0);
	val = rtl_inw(SYSISR); 
	
	printk("SYSISR =%x after TRXCMD R/W accessing\n", val);
	
	
	rtl_outl(FIMR, 0);
	rtl_inl(FISR);
	
	printk("FISR =%x after TRXCMD R/W accessing\n", val);
	

#endif	

	val = VOCMD;
	for(i = 0; i < (VOCMD_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0);
	}	

	val = VICMD;
	for(i = 0; i < (VICMD_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0);
	}

	val = BECMD;
	for(i = 0; i < (BECMD_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0);
	}
	
	val = BKCMD;
	for(i = 0; i < (BKCMD_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0);
	}
	
	val = TSCMD;
	for(i = 0; i < (TSCMD_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0);
	}
	
	val = MGTCMD;
	for(i = 0; i < (MGTCMD_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0);
	}
	
	val = BMCMD;
	for(i = 0; i < (BMCMD_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0);
	}
	
	val = BCNCMD;
	for(i = 0; i < (BCNCMD_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0);
	}
	
	val = RXCMD0;
	for(i = 0; i < (RXCMD0_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0x80000000);
	}

	val = RXCMD1;
	for(i = 0; i < (RXCMD1_ENTRYCNT << 4); i+= 4){			
		rtl_outl(val + i, 0x80000000);
	}

	//Below is for RCR/TCR...
#ifdef CONFIG_FW_WLAN_LBK
	if ((lbk_mode & RTL8711_MAC_LBK) || (lbk_mode & RTL8711_MAC_FW_LBK))
#else	
	if (lbk_mode & RTL8711_MAC_LBK) 
#endif		
	{
		rtl_outl(0xBD2000d0, 6);
	}
#ifdef CONFIG_FW_WLAN_LBK
	else if ((lbk_mode & RTL8711_BB_LBK) || (lbk_mode & RTL8711_BB_FW_LBK))
#else	
	else if (lbk_mode & RTL8711_BB_LBK)
#endif		
	{
		// PHY LBK Mode
		rtl_outb(0xBD200245,0x11); //testing: to turn on the bit(0) to avoid deadlock
		rtl_outl(0xBD200240, 0x01000b80);
		rtl_outl(0xBD200240, 0x010083c0);
		rtl_outl(0xBD200240, 0x01002000);

		rtl_outl(0xBD2000d0, 3);

		//Configure ofdmphy gain table
		rtl_outb(0xBD20025d, 0);
		rtl_outl(0xBD200240, 0x01003200);
		delay(1);
		rtl_outl(0xBD200240, 0x01001D0F);
		delay(1);
		rtl_outl(0xBD200240, 0x0100900E);
		delay(1);
		rtl_outl(0xBD200240, 0x0100100E);
		delay(1);
		rtl_outl(0xBD200240, 0x01003000);
		delay(1);
		rtl_outb(0xBD20025d, 0x20);

		// RF Timer
		rtl_outl(0xBD200268, 0x43214321);

		// RFPAR
		rtl_outw(0xBD20025c, 0x6856);
	
		// Configure CCK/OFDM BB
		rtl_outl(0xBD200240, 0x01001B80);
		rtl_outl(0xBD200240, 0x80);
	}
	else if (lbk_mode == RTL8711_AIR_TRX)
	{
		rtl_outl(0xBD2000d0, 0);
	}

	//to set the BIT(5) in order to disable HW TXOP duration 
	rtl_outl(TCR, BIT(5)); 

	if (lbk_mode != RTL8711_AIR_TRX)
	{
		val = BIT(21) | BIT(19) | BIT(17) | BIT(14) | BIT(13) |\
					BIT(12) | BIT(11) | BIT(7) | BIT(3) | BIT(2) |\
					BIT(1) | BIT(0);
	}
	else
	{
		val = BIT(22)| BIT(21) | BIT(19) | BIT(17) | BIT(14) | BIT(13) |\
					BIT(12) | BIT(11) | BIT(7) | BIT(3) | BIT(2) |\
					BIT(1);
	}

#ifndef CONFIG_NO_RXCMD0HAT

	val |= BIT(6);	

#endif

	rtl_outl(RCR, val);

	if ((hci_sel == RTL8711_SDIO) && (chip_version == RTL8711_1stCUT))
	{
		rtl_outl(ACCTRL, ENABLE_ALLQ | 0x80 );	
	}
	else
	{
		rtl_outl(ACCTRL, ENABLE_ALLQ);	
	}
	
	//rtl_outw(RXFFCMD, (RXEN1 | RXEN0));
	rtl_outw(RXFFCMD, (BIT(14) | BIT(13) | BIT(12) | BIT(11) | RXEN1 | RXEN0));
	rtl_outl(MAR, 0xffffffff);	
	rtl_outl(MAR + 4, 0xffffffff);


	myaddr = get_my_addr();
	
	rtl_outl(IDR, (myaddr[0] | (myaddr[1] << 8) | (myaddr[2] << 16) | (myaddr[3] << 24)));
	rtl_outl(IDR + 4, myaddr[4] | (myaddr[5] << 8));


	//Configure WPA_CONFIG
	//marc: to enable the T/RX encrypt/decrypt
	rtl_outw(WPA_CONFIG, BIT(1) | BIT(2));

#ifdef CONFIG_WLAN_HW

	for(i = 0; i< MAXCAMINX; i++)
		invalidate_cam(i);
	cam_config();
#endif


	rtl_outb(CCACRSTXSTOP, 0x46);

#ifndef CONFIG_FW_WLAN_RX0_LBK	
	rtl_outw(RXFLTMAP0, 0x3f3f);

#ifdef CONFIG_CRC_TEST		
	rtl_outw(RXFLTMAP1, 0xff00);
#endif
	
#else
	rtl_outw(RXFLTMAP0, 0x0);
#endif	
     
	switch (chip_version)
	{
		case RTL8711_FPGA:
			rtl_outw(TUBASE, 0x03e0);
			rtl_outl(SIFS, 0x00d6a8a5); 
			rtl_outw(PWRBASE, 0x0800);
			break;
		
		case RTL8711_1stCUT:
			rtl_outw(TUBASE, 0x0400);
			rtl_outl(SIFS, 0x00d6a8a5); 
			rtl_outb(0xbd200276, 0x00);
			rtl_outl(PhyAddr, 0x01004617);
			rtl_outw(RFSW_CTRL, 0x9A56);
			rtl_outw(RF_PARA, 0x0094);
			rtl_outw(PWRBASE, 0x0505);		
			break;
		
		case RTL8711_3rdCUT:
		case RTL8711_2ndCUT:
		default:
			rtl_outw(TUBASE, 0x03e8);
			rtl_outl(SIFS, 0x0016a8a5); 
			rtl_outb(0xbd200276, 0x00);
			rtl_outl(PhyAddr, 0x01004617);
			rtl_outw(RFSW_CTRL, 0x9A56);
			rtl_outw(RF_PARA, 0x0094);
			rtl_outw(PWRBASE, 0x1410);
			break;
	}
	
	//ofdm
	rtl_outl(AC_VO_PARAM, 0x002f3222);           	
	rtl_outl(AC_VI_PARAM, 0x005E4322);		
	rtl_outl(AC_BE_PARAM, 0x0000A42B);		
	//rtl_outl(AC_BK_PARAM, 0x00bcF420);
	//rtl_outl(AC_BK_PARAM, 0x00007320); // to set txop limit to zero
	rtl_outl(AC_BK_PARAM, 0x00027226); 
	rtl_outl(BM_MGT_PARAM, 0x22322232);	
	//cck
	rtl_outl(AC_VO_PARAM + 4, 0x0);           	
	rtl_outl(AC_VI_PARAM + 4, 0x0);		
	rtl_outl(AC_BE_PARAM + 4, 0x0);		
	rtl_outl(AC_BK_PARAM + 4, 0x0);
	
	join_bss(padapter);

}


	
__init_fun__ void wlan_lbkinit(void)
{
	//AddGrpStaInfo with entry 4

#ifdef CONFIG_FW_WLAN_LBK
#if 0
	//marc: tempoorary solution for HCI_RX_EN bug
	rtl_outw(0xbd2000b0, 0x3f3f);
	rtl_outw(0xbd2000b8, 0xff00);
	rtl_outl(0xbd2000c0, 0xffff);
#endif	

#endif
	//AddBSSIDStaInfo with entry 5	
}


#if 0
static int dequeue_ccode ((u16 *ccode)
{
	volatile int *head = &sys_mib.h2cqueue.head;
	volatile int *tail = &sys_mib.h2cqueue.tail;
	
	if (CIRC_CNT(*head, *tail, H2CCMDQUEUE_SZ)) {

		*ccode = sys_mib.h2cqueue.ccode[*tail];
		*tail = (*tail + 1) & (H2CCMDQUEUE_SZ - 1);
		return SUCCESS;
	}
	return FAIL;
}
#endif


#ifdef CONFIG_FW_WLAN_LBK

volatile int	rx_cnt;
volatile int	check_rx_cnt;

unsigned int	tx_cnt = 0;
//unsigned int	tx_cnt = 0x5c;

#endif

void seth2clbk_post(void);

#ifdef CONFIG_WLAN_TEST
void wlan_task(void)
{
	
#ifdef CONFIG_FW_WLAN_LBK

	unsigned int	i;
	
#define WLANFRLEN(x)	(x.hdr_len + x.iv_len + x.icv_len + x.mic_len + x.llc_len + x.payld_len)

	unsigned short txsz = 0;
	struct	txirp		*irp;

#endif

#ifdef CONFIG_H2CLBK

	unsigned short	ccode;
	volatile int *cmd_head = &sys_mib.h2cqueue.head;
	volatile int *cmd_tail = &sys_mib.h2cqueue.tail;

	void seth2clbk_evt_timer(unsigned char *pbuf);	
	INIT_LIST_HEAD(&sys_mib.lbkevt_timer.list);
	sys_mib.lbkevt_timer.func = &seth2clbk_evt_timer;
	sys_mib.lbkevt_timer.pbuf = NULL;
	sys_mib.lbkevt_timer.time_out = 0;
	sys_mib.lbkevt_cnt = 0;
	add_timer(&(sys_mib.lbkevt_timer));

#endif

	//marc
	local_irq_enable();

	for(;;)
	{

#ifdef CONFIG_TASK_ECHO

		printk("This is wlan_task speaking\n");

#endif

#ifdef CONFIG_H2CLBK
		while(CIRC_CNT((*cmd_head), (*cmd_tail), H2CCMDQUEUE_SZ))
		{
			ccode = sys_mib.h2cqueue.ccode[*cmd_tail];
			*cmd_tail = ((*cmd_tail) + 1) & (H2CCMDQUEUE_SZ - 1);
			
			switch (ccode) {
				case _SetH2cLbk_CMD_ :
					// This is the CMD EVENT mode loopback testing
					seth2clbk_post();
					break;					
				default:
					printk(" CMD(%d) is not allowed!\n", ccode);
				
			}	
		}
#endif

#ifdef CONFIG_FW_WLAN_LBK

	if (check_rx_cnt)
	{
		if (rx_cnt != CONFIG_TXCNT)
			goto _check_rx_frame_;	
		else
			rx_cnt = 0;
	}
	
	//printk("press any key to go on TX\n");
	//getchar();

#ifdef CONFIG_TX_RNDQ
	txqinx++;
	//if (txqinx == BMC_QUEUE_INX)
	if (txqinx == TS_QUEUE_INX)
		txqinx = VO_QUEUE_INX;
#else
	txqinx = CONFIG_TXQINX;
#endif
		
	for (i = 0; i < CONFIG_TXCNT; i++, tx_cnt++)
	{
#ifdef CONFIG_TX_RNDSZ
		txsz++;
#else
		//txsz = CONFIG_TXSZ + (tx_cnt % 307);
		//txsz = CONFIG_TXSZ + (tx_cnt % 1400);
		txsz = CONFIG_TXSZ;
#endif
		
		if (txqinx == MGT_QUEUE_INX) 
		{	
			if (txsz > TXMGTBUF_SZ)
				txsz = sizeof(struct ieee80211_hdr_3addr);
#ifndef CONFIG_CRC_TEST
			//for test of the mgt frame
			pktinfo.securityType = NONE;
			pktinfo.iv_len = 0;
			pktinfo.icv_len = 0;
			pktinfo.mic_len = 0;
			pktinfo.llc_len = 0;
			pktinfo.hdr_len =  sizeof(struct ieee80211_hdr_3addr);
			pktinfo.payld_len= txsz;

			pktinfo.wlanFrameType = IEEE80211_FTYPE_MGMT;
			pktinfo.wlanFrameSubType = IEEE80211_STYPE_PROBE_REQ;
#else
			//for test of the ctrl frame
			pktinfo.securityType = NONE;
			pktinfo.iv_len = 0;
			pktinfo.icv_len = 0;
			pktinfo.mic_len = 0;
			pktinfo.llc_len = 0;
			pktinfo.hdr_len =  sizeof(struct ieee80211_hdr_2addr);
			
			switch (tx_cnt % 5)
			{
				case 0:
					pktinfo.wlanFrameType = IEEE80211_FTYPE_CTL;
					pktinfo.wlanFrameSubType = IEEE80211_STYPE_PSPOLL;
					pktinfo.payld_len= 0;
					printk("tx: subtype IEEE80211_STYPE_PSPOLL\n");
					break;
				
				case 1:
					pktinfo.wlanFrameType = IEEE80211_FTYPE_CTL;
					pktinfo.wlanFrameSubType = IEEE80211_STYPE_CFEND;
					pktinfo.payld_len= 0;
					printk("tx: subtype IEEE80211_STYPE_CFEND\n");
					break;
					
				case 2:
					pktinfo.wlanFrameType = IEEE80211_FTYPE_CTL;
					pktinfo.wlanFrameSubType = IEEE80211_STYPE_CFENDACK;
					pktinfo.payld_len= 0;
					printk("tx: subtype IEEE80211_STYPE_CFENDACK\n");
					break;	
					
				case 3:
					pktinfo.wlanFrameType = IEEE80211_FTYPE_CTL;
					pktinfo.wlanFrameSubType = IEEE80211_STYPE_BLOCKACK_REQ;
					pktinfo.payld_len= 4;
					printk("tx: subtype IEEE80211_STYPE_BLOCKACK_REQ\n");
					break;
					
				case 4:
					pktinfo.wlanFrameType = IEEE80211_FTYPE_CTL;
					pktinfo.wlanFrameSubType = IEEE80211_STYPE_BLOCKACK;
					pktinfo.payld_len= 132;
					printk("tx: subtype IEEE80211_STYPE_BLOCKACK\n");
					break;		
			}
#endif

			if (WLANFRLEN(pktinfo) > TXMGTBUF_SZ) {
				pktinfo.payld_len = txsz = 0;
			}
			pktinfo.wlanFrameLength = WLANFRLEN(pktinfo);

			irp = alloc_txirp(TXMGTIRPS_TAG);
		}
		else {

				//pktinfo.securityType = CONFIG_ENCRYPT_ALGRTHM;
				pktinfo.securityType = 0;
				if  ((pktinfo.securityType) >= 5) {
					pktinfo.securityType = (get_random() % 5);
				}
				

				switch (pktinfo.securityType) {

					case NONE:
						pktinfo.iv_len = 0;
						pktinfo.icv_len = 0;
						pktinfo.mic_len = 0;			
						break;
					case WEP:
						pktinfo.iv_len = 4;
						pktinfo.icv_len = 4;
						pktinfo.mic_len = 0;
						break;
					case WEP_104:
						pktinfo.iv_len = 4;
						pktinfo.icv_len = 4;
						pktinfo.mic_len = 0;
						break;
					case TKIP:
						pktinfo.iv_len = 8;
						pktinfo.icv_len = 4;
						pktinfo.mic_len = 8;
						break;
					case AES:
						pktinfo.iv_len = 8;
						pktinfo.icv_len = 4;
						pktinfo.mic_len = 0;
						break;
				}

			pktinfo.hdr_len = sizeof(struct ieee80211_hdr_3addr);

			if ( (sys_mib.mac_ctrl.opmode) == IW_MODE_REPEAT)
			      	pktinfo.hdr_len = sizeof(struct ieee80211_hdr);

			pktinfo.wlanFrameType = IEEE80211_FTYPE_DATA;
			pktinfo.wlanFrameSubType = IEEE80211_STYPE_DATA;
			
			
			#ifdef CONFIG_TX_QOS
				pktinfo.cmdQoS = 1;
				pktinfo.priority = (get_random() & 7);
				pktinfo.hdr_len +=  2;
				pktinfo.wlanFrameSubType = IEEE80211_STYPE_QOS_DATA;
			#else
				pktinfo.cmdQoS = 0;
			#endif

			//marc
			//pktinfo.llc_len = 8;
			pktinfo.llc_len = SNAP_SIZE;
			
			//marc 0613
			pktinfo.payld_len = txsz;

			if (WLANFRLEN(pktinfo) > TXDATAIRPS_LARGE_SZ) {
				pktinfo.payld_len = txsz = 0;
			}
			txsz = pktinfo.wlanFrameLength = WLANFRLEN(pktinfo);
	
			irp = alloc_txdatabuf(txsz);
		}
		
		
		if (irp == NULL) {

			printk("Can't TX due to IRP shortage\n");
			break;
		}


		//now, we are going to make a frame...
		/*
			1. Mgt frame or Data frame
			2. ToDs/FrDs
			3. QoS/Non-QoS
		*/
		
#if 0
		hdr = (struct ieee80211_hdr_qos *)phy2cacheable(irp->txbuf_phyaddr);
		meset((unsigned char *)hdr, 0, sizeof(struct ieee80211_hdr_qos ));
#endif
		
		pktinfo.pframe = (unsigned char *)Physical2Virtual(irp->txbuf_phyaddr);
		irp->tx_len = pktinfo.wlanFrameLength;


		if (sys_mib.mac_ctrl.opmode == IW_MODE_INFRA)
			_memcpy(pktinfo.da, sys_mib.cur_network.MacAddress, ETH_ALEN);
		else
			_memcpy(pktinfo.da, assoc_sta0, ETH_ALEN);
			
		MPPacketGen(&pktinfo);

		wlan_tx(irp, 0);
		
	}
	wlan_tx(padapter, irp);

	check_rx_cnt = 1;


_check_rx_frame_:
	
	check_rx_lbkirps();

	
	
#endif

#if 0
	local_irq_save(flags);
	mgtctrl_dispatchers();
	local_irq_restore(flags);
#endif 		

		schedule_task((unsigned char *)&(sys_tasks[WLAN_TASKTAG].task.context),0,0,0);
	}
	
}
#endif

static __inline__ int priority2ac(int priority)
{
	if ((priority == 1) || (priority == 2))
		return BK_QUEUE_INX;

	if ((priority == 0) || (priority == 3))
		return BE_QUEUE_INX;

	if ((priority == 4) || (priority == 5))
		return VI_QUEUE_INX;

	if ((priority == 6) || (priority == 7))
		return VO_QUEUE_INX;

	return TS_QUEUE_INX;

}


/*
1. ==> MGT/CTRL type ==> always use MGT queue.

2. ==> Data Type:
a. if Multicast/Broadcast Address ==> always use BMC queue.
b. if QoS Data ==> Get TID from QC, then using priority2ac to get AC
c. if Non-QoS Data ==> always use BE queue.

*/

static __inline__ int get_txqueueinx (struct txirp *irp)
{

	unsigned short *fctrl, type;
	int	q_inx, tid;
#ifdef CONFIG_POWERSAVING
	unsigned short subtype;
#endif

	fctrl = (unsigned short *)get_txpframe(irp);

	type = GetFrameType(fctrl);

	switch (type) {

	case	WIFI_MGT_TYPE:
	case	WIFI_CTRL_TYPE:

#ifdef MGT_CRTL_QUEUE		
		q_inx = MGT_QUEUE_INX;
#else
		q_inx = BE_QUEUE_INX;
#endif		
		break;
		
	case	WIFI_DATA:
	default:
#ifdef CONFIG_POWERSAVING
		subtype = GetFrameSubType(fctrl);
		if((subtype == WIFI_DATA_NULL) || (subtype == WIFI_QOS_NULL)) {
			q_inx = BE_QUEUE_INX;	// Perry: the same as mgt frame.
			break;
		}
#endif
		
		if ((GetFrameSubType(fctrl)) & BIT(7)) {

			//QoS Data Grp	
			if ((GetToDs(fctrl)) && (GetFrDs(fctrl))) {
				tid = GetPriority((unsigned char*)fctrl + 30);
			}
			else {
				tid = GetPriority((unsigned char*)fctrl + 30);
			}
			q_inx = priority2ac(tid);
		}
		else {
			q_inx = BK_QUEUE_INX;
			
		}
			
		break;
		
	}
	
	return q_inx;
}

#if 0
unsigned int get_stainfo()
{



}
#endif
int get_camid(unsigned char *addr)
{
	struct	sta_data	*psta;

//get hash value, then stainfo, then camid
	psta = get_stainfo(addr);
	if (psta != NULL)	
		return (psta->info2->aid);
	else
		return (-1);
}



unsigned int fill_txcmd4(unsigned char *pframe, unsigned int sz)
{

	unsigned int	offset4= BIT(31) | BIT(29) | (sz << 16) | (0 << 10) | (BIT(9));
	//now, get MAC_ID...

	//marc 0613
	return offset4;


}




/*
Caller:
wlan_driver, TX Done DSR, DMA done


Caller must guarantee that all the hw resource are available...

This will be under atomic context, caller must guarantee.
*/


int	txffszcheck(int txlen, int qid)
{
	unsigned int	val = rtl_inl(FREETX);

	if (qid <= 3)
		val = (val >> (qid << 2)) & 0xf;
	else if (qid == 4)
		val = (val >> 16) & 0x1f;
	else if (qid == 5)
		val = (val >> 21) & 0x7;
	else
		val = (val >> 24) & 0xf;

	val = val << 8; //in units of 256 bytes...

	if (val > txlen)
		return SUCCESS;
	else
		return FAIL;
		

}


/*
In our firmware, 
txhw_fire could be called by:
1. TASK
2. WLAN TX Done DSR
3. DMA Done DSR

Some critical section protection is must!!!

*/

void _txhw_fire(struct hwcmd_queue *hwqueue)
{
	//int i;
	struct txirp	*irp;
	struct dma_cmd	dmacmd = {t_addr: hwqueue->ff_port, ch:hwqueue->dma_id, cmd:DMA_READ};
#ifdef MGT_CRTL_QUEUE		
	unsigned int	offset8, offset12;
	unsigned int	TXRate_idx, seqNum;
	unsigned char	TXRate;	
#endif
	
	volatile int *head = &(hwqueue->head);
	//marc
	volatile int tail = *(hwqueue->tail);
	int	sz = hwqueue->sz;

	//marc
	tail = (tail - (hwqueue->io_base)) >> 4;

dequeue_and_tx:

	if (list_empty(hwqueue->q_head))
		return;

	//check if DMA is free
	if ((GETDMA_STATUS(hwqueue->dma_id)) == DMA_BUSY)
		return;
	
	//check if TXCMD is available
	//marc
	if (CIRC_SPACE(*head, tail, sz) == 0)
		return;

	irp = list_entry((hwqueue->q_head)->next, struct txirp, list);

	//check if FF is enough... (tx ff size greater than txlen)
	//if ((txffszcheck(hwqueue->sz, hwqueue->dma_id)) == FAIL)
	if ((txffszcheck(irp->tx_len, hwqueue->queue_id)) == FAIL)
		return;
	
	list_del_init(&(irp->list));

	list_add_tail(&(irp->list), hwqueue->dma_flying);

	
	//now, every hardware is ready for transmission...
	//enable DMA first...
	dmacmd.m_addr = irp->txbuf_phyaddr;
	dmacmd.len = irp->tx_len;
	//then write the TXCMD...
	
#if 0
	printk("----going to tx a frame with len=%x, addr=%x, txcmd4=%x------\n", 
		irp->tx_len, irp->txbuf_phyaddr, irp->txcmd_offset4);
#if 0		
	for(i =0; i < irp->tx_len; i++)
	{
		printk("%x", *(unsigned char *)((Physical2Virtual(irp->txbuf_phyaddr)) + i));
		
		if (!(i & 7))
			printk("\n");  
		
		
	}
#endif


#endif
	
	start_dma(&dmacmd);

	//marc@0613
	//rtl_outl(hwqueue->io_base + (*head) * TXCMDENTRY_SIZE ,irp->txcmd_offset4);
	/* hwqueue->io_base: spec p.139 */
	rtl_outl(hwqueue->io_base + (*head) * TXCMDENTRY_SIZE + 4, irp->txcmd_offset4);
	
#ifdef MGT_CRTL_QUEUE
	//for mgt queue test
	if (irp->q_inx == MGT_QUEUE_INX)
	{
		//printk("mgt\n");
		offset8 = 0x80000000;
		offset12 = 0;

#ifndef LOWEST_BASIC_RATE		
		TXRate_idx = sys_mib.mac_ctrl.basicrate_inx;		
		TXRate = sys_mib.mac_ctrl.basicrate[TXRate_idx];		
		offset8 |= (((TXRate & 0x0f) << 4) | (TXRate & 0x0f));
#else
		if (GetFrameSubType(Physical2Virtual(irp->txbuf_phyaddr)) & (WIFI_PROBEREQ | WIFI_PROBERSP))
		{
			TXRate_idx = sys_mib.mac_ctrl.lowest_basicrate_inx;		
		}
		else
		{
			TXRate_idx = sys_mib.mac_ctrl.basicrate_inx;		
		}
		
		TXRate = sys_mib.mac_ctrl.basicrate[TXRate_idx];		
		offset8 |= (((TXRate & 0x0f) << 4) | (TXRate & 0x0f));
#endif
		
		offset8 |= (sys_mib.rf_ctrl.txagc[TXRate_idx] << 16);	
		if (sys_mib.rf_ctrl.tx_antset == 1)
		{
			offset8 |= sys_mib.rf_ctrl.fixed_txant;
		}
		else
		{
			offset8 |= sys_mib.stainfos[0].tx_swant;
		}

		rtl_outl(hwqueue->io_base + (*head) * TXCMDENTRY_SIZE + 8, offset8);
		
		seqNum = sys_mib.stainfo2s[0].nonqos_seq;		
		offset12 = seqNum << 16;
		rtl_outl(hwqueue->io_base + (*head) * TXCMDENTRY_SIZE + 12, offset12);
		
		seqNum = (seqNum & 0xfff) + 1;
		if (seqNum >= 0x1000)
		{
			seqNum = 0;
		}
		sys_mib.stainfo2s[0].nonqos_seq = seqNum;

		rtl_outb(TPPOLL, 0x20);
		
		sys_mib.mac_ctrl.txcmds[5].head += 16;
		if (sys_mib.mac_ctrl.txcmds[5].head == sys_mib.mac_ctrl.txcmds[5].mask_tail)
		{
			sys_mib.mac_ctrl.txcmds[5].head = sys_mib.mac_ctrl.txcmds[5].mask_head;
		}
		
	}
#endif	
	
	*head = (*head + 1) & (hwqueue->sz - 1);

	goto dequeue_and_tx;
	
}


/*

txhw_fire will check all the queue for TX

*/

//sunghau@0319: the parameter is never used
//void txhw_fire (int critical)
void txhw_fire (void)
{

	int i; 
	unsigned long flags;
	struct hwcmd_queue	*phwcmdqueue = &(sys_mib.vo_queue);

	//marc
	//for(i = 0; i <= WLANTX_BMC_DMA; i++, phwcmdqueue++)
	for(i = 0; i <= BMC_QUEUE_INX; i++, phwcmdqueue++)
	{

		/* Always protection */
		//if (critical)
			local_irq_save(flags);
		
			_txhw_fire(phwcmdqueue);

		//if (critical)
			local_irq_restore(flags);
	}

}

//DSR will be executed as parts of ISR. No infinite loop is allowed.
void wlantx_dsr(void)
{
	//sunghau@0319: the code is never used now...
	volatile unsigned char *val = &sys_mib.dma_status;
	*val = rtl_inb(DMACHSTATUS);  
	
	//printk("tx cmd seq:%x\n", sys_mib.stainfo2s[5].nonqos_seq);

#ifdef CONFIG_SCSI_WRITE_FIFO	
	//printk("tx done\n");
	scsi_write_irp_handler();
#endif
	
	local_irq_enable();

	if (TX_PENDING)
		txhw_fire ();
		// txhw_fire (0);
		// sunghau@0319: the parameter is never used
	
	local_irq_disable();

}

/*
	Caller Context :atomic 

	rx_queue: 0 ==> RXFF0
	rx_queue: 1 ==> RXFF1
	
*/
//marc@0626
//void rx_dmadata(int rx_queue)
void rx_dmadata(void)
{
	unsigned long flags;
	struct rxirp	*pirp;
	struct dma_cmd	dmacmd;
	//marc@0619
	//struct list_head	*rxlist = &(sys_mib.rx_datalist) + rx_queue;
	struct list_head	*rxlist = &(sys_mib.rx_dma_list);
	
	//local_irq_save(flags);

check_availability:
		
	if (GETDMA_STATUS(WLANRX_DMA) == DMA_BUSY) {
		//printk("B\n");
		goto rx_dmabusy;
	}

	if (list_empty(rxlist)) {
		//printk("E\n");
		goto rx_dmabusy;
	} else {
		//printk("M\n");
	}
	
	local_irq_save(flags);
	
	pirp = list_entry(rxlist->next, struct rxirp, list);

	list_del_init(&pirp->list);

	list_add_tail(&(pirp->list), &(sys_mib.rx_dmaflying));

	local_irq_restore(flags);
	
	dmacmd.m_addr = pirp->rxbuf_phyaddr;
	
	//marc@0626
	//dmacmd.t_addr =  Virtual2Physical((rx_queue != 0 ) ? RXFF1RP : RXFF0RP);
	//dmacmd.t_addr =  Virtual2Physical((pirp->tag == RXMGTIRPS_TAG) ? RXFF1RP : RXFF0RP);
	//temp for test RX0 FIFO 
	dmacmd.t_addr =  Virtual2Physical((pirp->RXFF_idx == 1) ? RXFF1RP : RXFF0RP);
	
	dmacmd.cmd = DMA_WRITE;
	dmacmd.ch = WLANRX_DMA;

#ifdef CONFIG_SCSI_READ_FIFO
	//RX_DMA_FRAG: to start DMA with len is less than 512	
	dmacmd.len = (pirp->rx_len > 512)? 512: pirp->rx_len;
	pirp->rx_dma_len = dmacmd.len;
#else
	dmacmd.len = pirp->rx_len;
#endif
	
	start_dma(&dmacmd);

	goto check_availability;

rx_dmabusy:
	//local_irq_restore(flags);

	return;

}

#ifdef CONFIG_POWERSAVING
extern void issue_QNULL(unsigned char pwr);
#endif

#ifdef CONFIG_SCSI_READ_FIFO
void rx_scsi_dmadata(void)
{
	unsigned long flags;
	struct rxirp	*pirp;
	struct dma_cmd	dmacmd;
		
	struct list_head	*rxlist = &(sys_mib.rx_scsi_dmaflying);
	int aligiment_length;
  
	if(!list_empty(rxlist)){
		local_irq_save(flags);
		pirp = list_entry(rxlist->next, struct rxirp, list);
		//printk("rx_scsi_dmadata (void)pirp->rxbuf_phyaddr: %X\n",pirp->rxbuf_phyaddr);
	
		dmacmd.m_addr = pirp->rxbuf_phyaddr+pirp->offset;
		dmacmd.t_addr = 0x1d600008; //?
		//dmacmd.len = (pirp->total_length>512)?512:(pirp->total_length);	
		dmacmd.len = 512;
		dmacmd.cmd = DMA_READ;
		dmacmd.ch = SCSI_READ;

		//printk("PATH is rx_dmadata pirp->rxbuf_phyaddr: %X t_addr: %X len:%d\n",pirp->rxbuf_phyaddr,dmacmd.t_addr,dmacmd.len);
		local_irq_restore(flags);	
	
		//aligiment_length = (dmacmd.len & 3) ? (((dmacmd.len) >> 2 << 2) + 4) : ((dmacmd.len) >> 2 << 2);	
		aligiment_length = 512;
		rtl_outl(0xbd62102c, (aligiment_length) << 9);
	
		start_scsi_dma(&dmacmd);

		//rtl_outl(0xbd62100c,pirp->total_length);
		rtl_outl(0xbd62100c,(Seq_Scsi<<16)|pirp->total_length);
		//c2hbuf[C2HSZ] = cpu_to_le32((Seq_Scsi << 16) | pirp->total_length);
		
		//c2hbuf[C2HSZ]=cpu_to_le32(pirp->rx_len);
		Scsi_Byte=(pirp->total_length>512)?512:(pirp->total_length);
		//Scsi_End=(pirp->total_length>512)?0:1;
		//printk("rx_scsi_dmadata(void) m_addr: %X t_addr:%X len: %d ch: %d aligiment_length: %d \n",dmacmd.m_addr,dmacmd.t_addr,dmacmd.len,dmacmd.ch,aligiment_length);
	
	}/*end of if(!list_empty(rxlist))*/

}
#endif

void wlanrx0_dsr(void)
{
 
	unsigned long	flags;
	int rxdma = 0;
	struct rx_swcmd	*pswcmd;
	struct rxirp	*prxirp = NULL;
	
	volatile unsigned int	*head = &(sys_mib.rx0swcmdq.head);
	volatile unsigned int	*tail = &(sys_mib.rx0swcmdq.tail);

	local_irq_enable();
	
	
	while (*head != *tail)
	{
		
		pswcmd = (struct rx_swcmd *)(*tail);
		(pswcmd->rxcmd_offset0) += 16;
		prxirp = NULL;
		
#ifdef CONFIG_POWERSAVING
		// TODO: Perry: WE should modify this section for handling CRC error packets.
		//dprintk("#### Host Receive Packet.size:%d seq:%d\n", (pswcmd->rxcmd_offset0 - 16) & 0xfff, (pswcmd->rxcmd_offsetc) & 0xfff);
		//dprintk("#### Host Receive Packet.size:%d \n", (pswcmd->rxcmd_offset0 - 16) & 0xfff);
		local_irq_disable();
		/* To fix 32K HISR & HIMR problems, FW must issue CPWM(P1 or P0) first, */
		ps_32K_isr_patch();

		if(RXCMD0_DATA(pswcmd->rxcmd_offset0)) {
			ps_rx_func[ps_mode](pswcmd->rxcmd_offset0, pswcmd->rxcmd_offset4);
			ps_rx++;
		}
		local_irq_enable();
#endif
		
		local_irq_save(flags);
		
		if (pswcmd->rxcmd_offset4 & 0x40)
		{
			//stadata[pswcmd->rxcmd_offset4 & 0x3f].stats->rx_pkts++;
			update_rxpkt(&stadata[pswcmd->rxcmd_offset4 & 0x3f]);
			
			//printk("%x\n", (pswcmd->rxcmd_offset4 & 0x3f));
			//printk("%x\n", pswcmd->rxcmd_offsetc);
			
		}
		
		local_irq_restore(flags);
		
		if (sys_mib.hcirxdisable)	{
			if ((pswcmd->rxcmd_offset0) & BIT(14))	//CRC error Frame
			{
				prxirp = alloc_rxirp(RXDATAIRPS_TRASH_TAG);
				printk("CRC error frame rcv 0\n");
				
				if (prxirp == NULL) {
					printk("RX: cannot allocate buf\n");
					(pswcmd->rxcmd_offset0) -= 16;
					goto end_of_rx0_dsr;
				}
				//sunghau@0319, the code can be put after getting TAG	
				//list_add_tail(&(prxirp->list),&sys_mib.rx_dma_list);
				//rxdma = 1;
			}
			else
			{
				if ((pswcmd->rxcmd_offset0) & BIT(28))	//Data Frame
				{
#ifndef CONFIG_SCSI_READ_FIFO					
					if (((pswcmd->rxcmd_offset0) & 0xfff) <= RXDATAIRPS_SMALL_SIZE)
						prxirp = alloc_rxirp(RXDATAIRPS_SMALL_TAG);
					else if (((pswcmd->rxcmd_offset0) & 0xfff) <= RXDATAIRPS_MEDIUM_SIZE)			
						prxirp = alloc_rxirp(RXDATAIRPS_MEDIUM_TAG);
					else 
#endif						
					if (((pswcmd->rxcmd_offset0) & 0xfff) <= RXDATAIRPS_LARGE_SIZE)
					{	
						prxirp = alloc_rxirp(RXDATAIRPS_LARGE_TAG);
					}
					else // receive a giant frame...
					{	
						prxirp = alloc_rxirp(RXDATAIRPS_LARGE_TAG);
						printk("A very large data frame is received\n");
					}	
		
					if (prxirp == NULL){
#if 1						
						printk("!\n");
						(pswcmd->rxcmd_offset0) -= 16;
						goto end_of_rx0_dsr;
#else						
						//for trash test
						prxirp = alloc_rxirp(RXDATAIRPS_TRASH_TAG);
						if (prxirp == NULL){
							printk("RX: cannot allocate buf\n");
						(pswcmd->rxcmd_offset0) -= 16;
						goto end_of_rx0_dsr;
					}
#endif						
					}
					
					//marc@0619
					//list_add_tail (&(prxirp->list),&sys_mib.rx_datalist);

					//sunghau@0319, the code can be put after getting TAG
					//list_add_tail (&(prxirp->list),&sys_mib.rx_dma_list);
					//rxdma = 1;
				}
				else //MGT or CTRL 
				{
					if (((pswcmd->rxcmd_offset0) & 0xfff) <= RXMGTBUF_SZ)
						prxirp = alloc_rxirp(RXMGTIRPS_TAG);
				
					if (prxirp == NULL)
					{
						prxirp = alloc_rxirp(RXDATAIRPS_TRASH_TAG);
				
						if (prxirp == NULL) {
							printk("RX: cannot allocate buf\n");
							(pswcmd->rxcmd_offset0) -= 16;
							goto end_of_rx0_dsr;
						}
					}
			
					//marc@0619	
					//list_add_tail (&(prxirp->list),&sys_mib.rx_datalist);
					
					//sunghau@0319, the code can be put after getting TAG
					//list_add_tail (&(prxirp->list),&sys_mib.rx_dma_list);
					//rxdma = 1;
				}
			}
			
			//sunghau@0319 
			list_add_tail (&(prxirp->list),&sys_mib.rx_dma_list);
			rxdma = 1;
			
			// don't worry the racing condition here!		
			prxirp->swcmd.rxcmd_offset0 = pswcmd->rxcmd_offset0;
			prxirp->swcmd.rxcmd_offset4 = pswcmd->rxcmd_offset4;
			prxirp->swcmd.rxcmd_offset8 = pswcmd->rxcmd_offset8;
			prxirp->swcmd.rxcmd_offsetc = pswcmd->rxcmd_offsetc;
			//prxirp->rx_len =  ((pswcmd->rxcmd_offset0 & 0xfff) - 16 - 4);
			prxirp->rx_len =  ((pswcmd->rxcmd_offset0 & 0xfff));
			prxirp->RXFF_idx = 0;
		}

		//rx_dmadata(0);	//jackson 0615


		*tail = *tail + (sizeof (struct rx_swcmd));

		if (*tail == (volatile unsigned int)(&sys_mib.rx0swcmdq.swcmds[0]) + RX0SWCMDQUEUE_SWCMD_SZ_LIMIT )
			*tail = (volatile unsigned int)(&sys_mib.rx0swcmdq.swcmds[0]);
	}
end_of_rx0_dsr:
	
	//start DMA, if resource is free...	
	if ((sys_mib.hcirxdisable) && (rxdma)){
		rx_dmadata();
	}
		
	local_irq_disable();
}

void wlanrx1_dsr(void)
{
	//unsigned long	flags;
	int rxdma = 0;
	struct rx_swcmd	*pswcmd;
	struct rxirp	*prxirp;
	
	volatile unsigned int	*head = &(sys_mib.rx1swcmdq.head);
	volatile unsigned int	*tail = &(sys_mib.rx1swcmdq.tail);

	local_irq_enable();
	
#ifdef CONFIG_FW_WLAN_RX0_LBK	
	printk("rx1 rcv packets \n");
	while (1);
#endif
	
	//printk("before dsr\n");
	while (*head != *tail)
	{
		prxirp = NULL;
		
		pswcmd = (struct rx_swcmd *)(*tail);
		(pswcmd->rxcmd_offset0) += 16;
		
#ifdef CONFIG_POWERSAVING
		local_irq_disable();
		if(RXCMD0_DATA(pswcmd->rxcmd_offset0)) {
			ps_rx_func[ps_mode](pswcmd->rxcmd_offset0, pswcmd->rxcmd_offset4);
			ps_rx++;
		}
		local_irq_enable();
#endif
		
		//printk("RX len: %x\n", (pswcmd->rxcmd_offset0) & 0xfff);
		if ((pswcmd->rxcmd_offset0) & BIT(14))	//CRC error Frame
		{
			prxirp = alloc_rxirp(RXDATAIRPS_TRASH_TAG);
			printk("CRC error frame rcv 1\n");
				
			if (prxirp == NULL) {
				printk("RX: cannot allocate buf\n");
				goto end_of_rx1_dsr;
			}
			//sunghau@0319, the code can be put after getting TAG	
			//list_add_tail(&(prxirp->list),&sys_mib.rx_dma_list);
			//rxdma = 1;
		}
		else
		{
#ifdef CONFIG_POWERSAVING
			local_irq_disable();
			// Perry: RX1 receives DATA frame??
			if(RXCMD0_DATA(pswcmd->rxcmd_offset0)) {
				ps_rx_func[ps_mode](pswcmd->rxcmd_offset0, pswcmd->rxcmd_offset4);
				ps_rx++;
			}
			local_irq_enable();
#endif
		
			if ((pswcmd->rxcmd_offset0) & BIT(28))	//Data Frame
			{
				if (((pswcmd->rxcmd_offset0) & 0xfff) <= RXDATAIRPS_SMALL_SIZE)
					prxirp = alloc_rxirp(RXDATAIRPS_SMALL_TAG);
				else if (((pswcmd->rxcmd_offset0) & 0xfff) <= RXDATAIRPS_MEDIUM_SIZE)			
					prxirp = alloc_rxirp(RXDATAIRPS_MEDIUM_TAG);
				else if (((pswcmd->rxcmd_offset0) & 0xfff) <= RXDATAIRPS_LARGE_SIZE)
					prxirp = alloc_rxirp(RXDATAIRPS_LARGE_TAG);
				else // receive a giant frame...
				{	
					prxirp = alloc_rxirp(RXDATAIRPS_LARGE_TAG);
					printk("A very large data frame is received\n");
				}	
		
				if (prxirp == NULL) {
					printk("RX: cannot allocate buf\n");
					(pswcmd->rxcmd_offset0) -= 16;
					goto end_of_rx1_dsr;
				}
			
				//marc@0619
				//list_add_tail (&(prxirp->list),&sys_mib.rx_datalist);
				
				//sunghau@0319, the code can be put after getting TAG
				//list_add_tail (&(prxirp->list),&sys_mib.rx_dma_list);
				//rxdma = 1;
			
			}
			else //MGT or CTRL
			{
				//printk("dsr mgt\n");
				if (((pswcmd->rxcmd_offset0) & 0xfff) <= RXMGTBUF_SZ) {
					prxirp = alloc_rxirp(RXMGTIRPS_TAG);
				}
			
				if (prxirp == NULL)
				{
					prxirp = alloc_rxirp(RXDATAIRPS_TRASH_TAG);
				
					if (prxirp == NULL) {
						printk("RX: cannot allocate buf\n");
						(pswcmd->rxcmd_offset0) -= 16;
						goto end_of_rx1_dsr;
					}
				}	 	
			
				//marc@0619
				//list_add_tail (&(prxirp->list),&sys_mib.rx_mgtctrllist);
				
				//sunghau@0319, the code can be put after getting TAG
				//list_add_tail (&(prxirp->list),&sys_mib.rx_dma_list);
				//rxdma = 1;
			}
		}
		
		//sunghau@0319, the code can be put after getting TAG
		list_add_tail(&(prxirp->list),&sys_mib.rx_dma_list);
		rxdma = 1;
		
		prxirp->swcmd.rxcmd_offset0 = pswcmd->rxcmd_offset0;
		prxirp->swcmd.rxcmd_offset4 = pswcmd->rxcmd_offset4;
		prxirp->swcmd.rxcmd_offset8 = pswcmd->rxcmd_offset8;
		prxirp->swcmd.rxcmd_offsetc = pswcmd->rxcmd_offsetc;
		//prxirp->rx_len =  ((pswcmd->rxcmd_offset0 & 0xfff) - 16 - 4);
		prxirp->rx_len = (pswcmd->rxcmd_offset0 & 0xfff);
		prxirp->RXFF_idx = 1;
		
		*tail = *tail + (sizeof (struct rx_swcmd));

		if (*tail == (volatile unsigned int)(&sys_mib.rx1swcmdq.swcmds[0]) + RX1SWCMDQUEUE_SWCMD_SZ_LIMIT)
			*tail = (volatile unsigned int)(&sys_mib.rx1swcmdq.swcmds[0]);
		//printk("end dsr mgt\n");
	}
end_of_rx1_dsr:
	//printk("before dma\n");
	//start DMA, if resource is free...
	if (rxdma)
		rx_dmadata();
	//printk("dsr end\n");	
	
	local_irq_disable();
	
	
}

#ifdef CONFIG_H2CLBK

static int init_lbk_event_node(struct event_node *node)
{
	volatile int	*head = &sys_mib.lbk_evtqueue.head;
	volatile int	*tail = &sys_mib.lbk_evtqueue.tail;
	struct seth2clbk_parm *pparm = &sys_mib.lbkparm;
	struct	c2hlbk_event *prsp= &sys_mib.lbk_evtqueue.events[*head];


	if ((CIRC_SPACE(*head, *tail , LBKEVENT_SZ)) == 0)
		return FAIL;
		
	prsp->mac[0]  =   pparm->mac[5];		
	prsp->mac[1]  =   pparm->mac[4];		
	prsp->mac[2]  =   sys_mib.lbkevt_cnt;
	prsp->mac[3]  =   pparm->mac[2];		
	prsp->mac[4]  =   pparm->mac[1];		
	prsp->mac[5]  =   pparm->mac[0];		
	prsp->s0	 =   cpu_to_le16(swab16(le16_to_cpu(pparm->s0)) - prsp->mac[2]);
	prsp->s1	 =   cpu_to_le16(le16_to_cpu(pparm->s1) + prsp->mac[2]);	
	prsp->w0 =   cpu_to_le32(swab32(le32_to_cpu(pparm->w0)));
	prsp->b0		= 	pparm->b1;
	prsp->s2		= 	cpu_to_le16(le16_to_cpu(pparm->s0) + 
					 prsp->mac[2]);		
	prsp->b1		= 	pparm->b0;
	prsp->w1	= 	 cpu_to_le32(swab32(le32_to_cpu(pparm->w1)) - prsp->mac[2]);
	*head = (*head + 1) & (LBKEVENT_SZ - 1);
	
	node->node = (unsigned char *)prsp;
	node->evt_code = _C2hLbk_EVT_;
	node->evt_sz = sizeof (*prsp);
	node->caller_ff_tail =  &sys_mib.lbk_evtqueue.tail;
	node->caller_ff_sz = H2CCMDQUEUE_SZ;
	
	return SUCCESS;
}

void seth2clbk_post(void)
{

	struct event_node node;
	
	// in the beginning, the head == tail (empty)

	if (sys_mib.lbk_evtqueue.head != sys_mib.lbk_evtqueue.tail) {

		printk("Event Loop Back Testing erro! head:%d, tail:%d\n",
			sys_mib.lbk_evtqueue.head, sys_mib.lbk_evtqueue.tail);	
		return;
	}

	
	sys_mib.lbkevt_cnt = 1;
	// now, setup the event contents...	
	if ((init_lbk_event_node(&node)) != SUCCESS) {
		printk("LBK error! New command can't begin until the end of previous cmd\n");
		sys_mib.lbkevt_cnt = 0;	
		return;
	}

	//assign the event node to the event queue.
	event_queuing(&node);

	if (sys_mib.lbkparm.mac[3] != 1) 	//jackson 0525
		sys_mib.lbkevt_timer.time_out = 2;
	else {
		printk("evtQ 1\n");
	}
	
		
}

void seth2clbk_evt_timer(unsigned char *pbuf)
{
	unsigned char old;
	struct event_node node;
	struct seth2clbk_parm *pparm = &sys_mib.lbkparm;

	old = sys_mib.lbkevt_cnt;
	if (sys_mib.lbkevt_cnt <  pparm->mac[3]) {
		sys_mib.lbkevt_cnt++;
		
		if ((init_lbk_event_node(&node)) == SUCCESS) {
			printk("evtQ %d\n", sys_mib.lbkevt_cnt); 
			event_queuing(&node);
		}
		else
		{
			printk("Host fetch event slowly...\n");
			sys_mib.lbkevt_cnt = old;
		}
		//jackson 0525
		if ((sys_mib.lbkevt_cnt) != pparm->mac[3])
			sys_mib.lbkevt_timer.time_out = 2;
	}
	else {
		//printk("CMD EVT has done for %d events\n", 	sys_mib.lbkevt_cnt);
	}

}

#endif


/* sunghau@0604: for dynamic initial gain function
 * */
#ifdef CONFIG_DIG
__init_fun__ void dig_timer_init(void)
{
	ofdm_false_cnt = 0;
	dig_up_count = 0;
	dig_down_count = 0;
	gain_up = 0x26;
	cca_table_index = 0;
	
	timer_init(&sys_mib.dig_timer, NULL, &dig_timer_hdl);
	
	if (dig == 1)
		sys_mib.dig_timer.time_out = DIG_TO;
	else
		sys_mib.dig_timer.time_out = 0;

	return;
}

/*
 * 1d2000e0: 	CCA_Counter[19:0], sel[26:24]
 * 		sel --> 0:ofdm 1:ofdm_ok 2:ofdm_fail 3:ofdm_false
 *			4:cck  5:cck_ok  6:cck_fail  7:cck_false
 *
 *                               
 */

void dig_timer_hdl(unsigned char *pbuf)
{
	unsigned int last_ofdm_false_cnt = ofdm_false_cnt;
	unsigned int false_alarm;
	unsigned int tmp = 0, orig_gain = 0;
	
	//in wlan_mlme.c, we check the signal strength. if signal strength < -82dbm, ss_weak is set. 
	//The initial gain is set 0x26 forcely, and dig_timer_hdl will not work until ss_weak = 0.
	if (ss_weak == 1)
		goto dig_end;

	/* OFDM */
	rtl_outl(CCA_COUNTER, 0x03000000);	//OFDM false alarm count
	ofdm_false_cnt = rtl_inl(CCA_COUNTER) & 0xFFFFF;

	if(ofdm_false_cnt >= last_ofdm_false_cnt)	// get ofdm false alarm count
		false_alarm = ofdm_false_cnt - last_ofdm_false_cnt;
	else
		false_alarm = (0xFFFFF - last_ofdm_false_cnt) + ofdm_false_cnt;	
		// the CCA_Counter is not read to clear, so we check the overflow of CCA count.
	
	//printk("OFDM false alarm = %x\n", false_alarm);
	//rtl_outl(0xbd621004, false_alarm);
	//if(txrate_inx <= 4){
	rtl_outl(0xbd200240, 0x00000017);

	tmp = rtl_inl(0xbd200240);
	tmp = tmp >>16;
	if (tmp < gain_up){
		tmp = gain_up;
		tmp = tmp << 8;
		tmp |= 0x01000017;
		rtl_outl(0xbd200240, tmp);
	}
		
	orig_gain = tmp;

	/* avoid gain switch between 2 gain values.
	 * This will help dig switch in 1 sec.
	 * */
	if(down_check >= 1)
	{
		if(down_check <=5)
			down_check+=1;
		else
			down_check = 0;

		if(false_alarm >= DIG_UP)
		{
			if(tmp < 0x56)
				tmp += 0x10;
			down_check = 0;
			goto change_gain;
		}
	}
		
	//printk("old gain is %x\n", tmp);
	
	//dig will up or down when false_alarm >= DIG_UP or <= DIG_DOWN 5 times continuously.
	if (false_alarm >= DIG_UP)
	{
		dig_up_count +=1;		
		if (dig_up_count >= DIG_TH)
		{
			dig_up_count = 0;
			dig_down_count = 0;
			if(tmp < 0x56)
				tmp += 0x10;
		}
	} 
	else if (false_alarm <= DIG_DOWN)
	{
		dig_down_count +=1;
		if (dig_down_count >= DIG_TH)
		{
			dig_down_count = 0;
			dig_up_count = 0;
			if(tmp > gain_up)
			{
				tmp -= 0x10;
				down_check = 1;	//avoid gain switch between 2 gain values
			}
		}
	} 
	else 
	{
		dig_up_count = 0;
		dig_down_count = 0;
	}
		
change_gain:	//update initial gain
	if(orig_gain != tmp){
//		printk("new gain is %x\n", tmp);
		tmp = tmp << 8;
		tmp |= 0x01000017;
		rtl_outl(0xbd200240, tmp);
	}

dig_end:

	tmp = cca_table[cca_table_index];
	tmp = tmp << 8;
	tmp |= 0x0100008a;
	rtl_outl(0xbd200240, tmp);
		
	if (dig == 1)
		sys_mib.dig_timer.time_out = DIG_TO;	// set dig timer
	else
		sys_mib.dig_timer.time_out = 0;

	return;
}

#endif

#ifdef CONFIG_RATEADAPTIVE
/*
 * Rate adaptive: calculate the ratio of (retry count / tx ok count)
 * 		  If the ratio reaches the up or down threshold of each rate, the txrate_inx will up or down
 */
void ra_timer_hdl(unsigned char *pbuf)
{
	int i=0, txrate_inx=0; //, txretry, txok;
	unsigned int tx_ratio = 0;
	
	local_irq_disable();

	if(!(OPMODE & WIFI_STATION_STATE)){
		goto ra_end;
	}

	//for(i=5;i<MAX_STASZ;i++)
	for (i = 5; i < 5; i++)
	{
		//ra_result: 2->up, 1->down, 0->no action
		//sys_mib.stainfo2s[i].ra_result = 0;
		//ra_result will be checked in int-handler.S
		ra_result = 0;	//rate adaptive result
		txrate_inx = sys_mib.stainfos[i].txrate_inx; // the txrate index of current rate
		ra_step = 0;	//if now is not in ra, we reset the counter.
		/*
		 * if there are packet transmittions, enter the rate adaptive algorithm.
		 * */
		if((sys_mib.stainfo2s[i].txretry_cnt > 0) && (sys_mib.stainfo2s[i].txok_cnt > 0)){
			tx_ratio = sys_mib.stainfo2s[i].txretry_cnt*100 / sys_mib.stainfo2s[i].txok_cnt;

			/*
			 * check if the rate need to be down
			 * */
			if(tx_ratio > dlevel_retry[txrate_inx])
			{
				ra_step = 1;	// now is in ra state
				/*
				if(txrate_inx == 6 || txrate_inx ==7){
					printk("txrate_error\n");
					sys_mib.stainfos[i].txrate_inx = 8;
				}
				*/
	//			printk("down rate, before ra_result %d\n", sys_mib.stainfo2s[i].ra_result);
	//			sys_mib.stainfo2s[i].ra_result = 1;

				ra_up_flag = 0;	// the up flag is clear due to we are in down rate state
				if(ra_down_flag !=0)
					ra_down_counter += 1;	//add down counter
			
				if(ra_down_flag == 0){
					ra_down_flag =1;	//set ra_down_flag
				}

				if(ra_down_counter >= 4){	//if down_counter >=4, reset the counter and set ra_result = down_rate
					ra_down_counter = 0;
					ra_up_counter = 0;
					ra_down_flag = 0;
					ra_up_flag = 0;
					ra_result = 1;
				}
#if 0
				printk("ra_down_flag = %d, ra_down_counter = %d\n", ra_down_flag, ra_down_counter);
#endif
	//			printk("after ra_result %d\n", sys_mib.stainfo2s[i].ra_result);
			}
			/*
			 * check if the rate need to be up
			 * */
			if(tx_ratio < ulevel_retry[txrate_inx])
			{
				ra_step = 1;
	//			printk("up rate\n");
			//	sys_mib.stainfo2s[i].ra_result = 2;
			/*
				if(txrate_inx == 6 || txrate_inx ==7){
					printk("txrate_error\n");
					sys_mib.stainfos[i].txrate_inx = 5;
				}
			*/

				ra_down_flag = 0;	// clear the down flag if we are in up rate state

				if(ra_up_flag !=0)	// add up counter
					ra_up_counter += 1;
			
				if(ra_up_flag == 0){	// set ra_up_flag
					ra_up_flag =1;
				}

				if(ra_up_counter >= count_judge[txrate_inx]){	// if up_counter >= threshold, set ra_result = up_rate
					ra_down_counter = 0;
					ra_up_counter = 0;
					ra_down_flag = 0;
					ra_up_flag = 0;
					ra_result = 2;
			}
#if 0				
				printk("ra_up_flag = %d, ra_up_counter = %d\n", ra_down_flag, ra_down_counter);
#endif
			}

		} else if((sys_mib.stainfo2s[i].txretry_cnt > 0) && (sys_mib.stainfo2s[i].txok_cnt == 0)){
			//sys_mib.stainfo2s[i].ra_result = 1;
			ra_result = 1;	// if only retry packet, set ra_result = down_rate
		} else if((sys_mib.stainfo2s[i].txretry_cnt == 0) && (sys_mib.stainfo2s[i].txok_cnt > 0)){
			//sys_mib.stainfo2s[i].ra_result = 2;
			ra_result = 2;	// if only tx ok packet, set ra_result = up_tate
		} else if((sys_mib.stainfo2s[i].txretry_cnt == 0) && (sys_mib.stainfo2s[i].txok_cnt == 0)){
			ra_nodata+=1;	//if the transmittion is stopped, the rate will up
			if(ra_nodata >=3){
				ra_result = 2;
				ra_nodata = 0;
			}
		}
#if 0
		printk("mac id=%d\n", i);
		printk("retry %d, ok %d\n", sys_mib.stainfo2s[i].txretry_cnt, sys_mib.stainfo2s[i].txok_cnt);
//		printk("tx_ratio = %d, dlevel = %d, ulevel = %d\n", tx_ratio, dlevel_retry[txrate_inx], ulevel_retry[txrate_inx]);
		printk("result %d, rate before update %d\n", ra_result, txrate_inx);
#endif
		if(ra_step == 0)	// if not in ra state, reset counter and flags
		{
			ra_down_flag = 0;
			ra_up_flag = 0;
			ra_down_counter = 0;
			ra_up_counter = 0;
		}
		sys_mib.stainfo2s[i].txretry_cnt = 0;
		sys_mib.stainfo2s[i].txok_cnt = 0;
		tx_ratio = 0;
		
		if(OPMODE & WIFI_STATION_STATE)
			break;
	}
	
ra_end:
	sys_mib.ra_timer.time_out = RA_TO;

	local_irq_enable();
	return;
}
#endif

#ifdef CONFIG_POWERTRACKING
/* SungHau@0911
 * power tracking: change TX power index with variety of temperature
 * 1. Write RF Reg00=08b
 * 2. Read RF Reg36[7:4]
 * 3. Write RF Reg00=088
 *
 */

#ifdef CONFIG_THERMAL_METHOD

static char get_thermal_meter(void)
{	// get thermal meter from RF[0x
	unsigned int meter;
	RF_WriteReg(0x0, 16, 0x08b);	// change RF page to page 3
	meter = RF_ReadReg(0x06);	// read RF[0x06 of page 3 => 0x36]
	meter = (meter >> 4) & 0xf;
	RF_WriteReg(0x0, 16, 0x088);

	return meter;
}

#endif

__init_fun__ void pt_timer_init(void)
{
	/*
	WriteRF(0x0, 0x08b);
	thermal_meter = readRF(0x36);
	thermal_meter = (meter_n >> 4) & 0xf;
	WriteRF(0x0, 0x088);
	*/
#ifdef CONFIG_THERMAL_METHOD	
	timer_init(&sys_mib.pt_timer, NULL, &pt_timer_hdl);
	thermal_meter = get_thermal_meter();		// get thermal meter
	initial_thermal_meter = thermal_meter;		// get initial thermal meter, we get the value in EEPROM.
	sys_mib.pt_timer.time_out = PT_TO;
#else

	//using TSSI method
	rtl_outl(0xbd2000f4, 0xf00af00a);
	//rtl_outw(0xbd2000f0, 0x7f01);
	
	timer_init(&sys_mib.pt_timer, NULL, &pt_timer_hdl);
	sys_mib.pt_timer.time_out = PT_TO;
	
#endif
}

void pt_timer_hdl(unsigned char *pbuf)
{	
#ifdef CONFIG_THERMAL_METHOD	

	unsigned int meter_n;
	int delta;
	unsigned int cck_txindex;
	unsigned int ofdm_txindex;
	unsigned int tmp;

	/*
	WriteRF(0x0, 0x08b);
	meter_n = readRF(0x36);
	meter_n = (meter_n >> 4) & 0xf;
	WriteRF(0x0, 0x088);
	*/
	meter_n = get_thermal_meter();	// get new thermal meter
	
	if(meter_n != thermal_meter){	// when new thermal meter != old thermal meter, we change TX power index by the result of new_meter - old_meter.
		delta  = meter_n - thermal_meter;	// get delta
		tmp = rtl_inw(0xbd2000fa);		// get TX power index of CCK & OFDM
		cck_txindex = (tmp >> 8) & 0xff;
		ofdm_txindex = tmp & 0xff;
		cck_txindex = cck_txindex - delta;	// new index = old index - delta
		ofdm_txindex = ofdm_txindex - delta;
		tmp = cck_txindex << 8 | ofdm_txindex;	// put new TX power index
		rtl_outw(0xbd2000fa, tmp);

		thermal_meter = meter_n;		// change the meter_n to old_meter
	}

#else

	//using TSSI method
	unsigned int	avg_of_tssi;
	unsigned char	tssi;
	unsigned short	tx_power;
	
	//read tssi
	rtl_outw(0xbd2000f0, 0x7f01);
	tssi = rtl_inb(0xbd2000f9);
	rtl_outw(0xbd2000f0, 0x0);
	
	if (tssi < 0x80)
	{
		//ofdm tssi
		DEBUG_INFO(("ofdm tssi: %x\n", tssi));
		sum_of_ofdm_tssi += tssi;
		cnt_of_ofdm_tssi++;
	}
	else
	{
		//cck tssi
		//printk("cck tssi: %x\n", (tssi & 0x7f));
		sum_of_cck_tssi += (tssi & 0x7f);
		cnt_of_cck_tssi++;
	}
	
	if (cnt_of_ofdm_tssi == 10)
	{
		avg_of_tssi = sum_of_ofdm_tssi / 5;
		
		if (avg_of_tssi > ofdm_tssi[sys_mib.cur_channel - 1])
		{
			if ((avg_of_tssi - ofdm_tssi[sys_mib.cur_channel - 1]) > 5)
			{
				delta_of_tx_pwr++;
			}
			
		}
		else if (avg_of_tssi < ofdm_tssi[sys_mib.cur_channel - 1])
		{
			if (( ofdm_tssi[sys_mib.cur_channel - 1] - avg_of_tssi) > 5)
			{
				delta_of_tx_pwr--;
			}
		}
		else
		{
			goto _END_OF_PWR_TRACKING_BY_TSSI_;
		}
		
		//printk("avg tssi: %x, delta pwr idx: %x\n", avg_of_tssi, delta_of_tx_pwr);
#if 1		

		tx_power = ((unsigned char)(sys_mib.cur_class->channel_cck_power[sys_mib.cur_channel -1] + delta_of_tx_pwr) << 8) | 
				   ((unsigned char)(sys_mib.cur_class->channel_ofdm_power[sys_mib.cur_channel - 1] + delta_of_tx_pwr));

		rtl_outw(0xbd2000f0, 0x0);

		//printk("pt tx pwr(%x): %x\n", PWRBASE, tx_power);

		rtl_outw(PWRBASE, tx_power);
		
		rtl_outw(0xbd2000f0, 0x7f01);
		
		//tx_power = rtl_inw(PWRBASE);
				
		//printk("pt(%x): %x\n", PWRBASE, rtl_inw(PWRBASE));
#endif
		cnt_of_ofdm_tssi = cnt_of_cck_tssi = 0;
		sum_of_ofdm_tssi = sum_of_cck_tssi = 0;	

		goto _END_OF_PWR_TRACKING_BY_TSSI_;

	}

	if (cnt_of_cck_tssi == 10)
	{
		avg_of_tssi = sum_of_cck_tssi / 5;
		
		if (avg_of_tssi > cck_tssi[sys_mib.cur_channel - 1])
		{
			if ((avg_of_tssi - cck_tssi[sys_mib.cur_channel - 1]) > 5)
			{
				delta_of_tx_pwr++;
				
			}
			
		}
		else if (avg_of_tssi < cck_tssi[sys_mib.cur_channel - 1])
		{
			if ((avg_of_tssi - cck_tssi[sys_mib.cur_channel - 1]) < -5)
			{
				delta_of_tx_pwr--;
			}
		}
		else
		{
			goto _END_OF_PWR_TRACKING_BY_TSSI_;
		}
		
		//printk("avg tssi: %x, delta pwr idx: %x\n", avg_of_tssi, delta_of_tx_pwr);
#if 1		
		tx_power = ((unsigned char)(sys_mib.cur_class->channel_cck_power[sys_mib.cur_channel -1] + delta_of_tx_pwr) << 8) | 
				   ((unsigned char)(sys_mib.cur_class->channel_ofdm_power[sys_mib.cur_channel - 1] + delta_of_tx_pwr));

		rtl_outw(0xbd2000f0, 0x0);

		//printk("pt tx pwr(%x): %x\n", PWRBASE, tx_power);

		rtl_outw(PWRBASE, tx_power);
		
		rtl_outw(0xbd2000f0, 0x7f01);
		
		tx_power = rtl_inw(PWRBASE);
	
		//printk("pt(%x): %x\n", PWRBASE, rtl_inw(PWRBASE));	
	
#endif				  
	cnt_of_ofdm_tssi = cnt_of_cck_tssi = 0;
	sum_of_ofdm_tssi = sum_of_cck_tssi = 0;	

	}
	
_END_OF_PWR_TRACKING_BY_TSSI_:
	
#endif

	sys_mib.pt_timer.time_out = PT_TO;

	return;
}
#endif

#endif
