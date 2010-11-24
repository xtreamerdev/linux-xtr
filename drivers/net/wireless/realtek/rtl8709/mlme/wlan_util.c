
#define _RTL8187_RELATED_FILES_C
#define _WLAN_UTIL_C_

#include <drv_types.h>

/*
	
Caller must guarantee the atomic context to prevent racing condition
	
*/
void free_stadata(struct sta_data *psta)
{
	DEBUG_INFO(("free sta\n"));

	psta->status = -1;
	psta->stats->rx_pkts = 0;
	list_del_init(&(psta->assoc));
	list_del_init(&(psta->sleep));
	list_del_init(&(psta->hash));
	//invalidate_cam(psta->info2->aid);
	
}

struct	sta_data* search_assoc_sta(unsigned char *addr,  _adapter* padapter)
{
	//unsigned long		flags;
	unsigned int		hash;
	struct list_head	*plist;
	struct sta_data	*psta = NULL;
	struct stainfo2	*info2 = NULL;
	struct mib_info	*_sys_mib = &(padapter->_sys_mib);

	hash = wifi_mac_hash(addr);
	
//	local_irq_save(flags);
	
	plist = &(_sys_mib->hash_array[hash]);
	
	while((plist->next) != (&(_sys_mib->hash_array[hash]))) {
		plist = plist->next;
		psta = list_entry(plist, struct sta_data ,hash);
		
		info2 = psta->info2;
		
		if (!(memcmp((void *)info2->addr, (void *)addr, ETH_ALEN))) {
			//local_irq_restore(flags);
			return psta;
   		}
	}

//	local_irq_restore(flags);

	return NULL;
	
}

void del_assoc_sta(_adapter *padapter, unsigned char *addr)
{
#if 0
	struct sta_priv *pstapriv = &(padapter->stapriv);
	struct sta_info *pstainfo = NULL;

	pstainfo = get_stainfo(pstapriv, addr);
	if(pstainfo)
	{
		free_stadata(&(pstainfo->_sta_data));
	}
#endif
	
	struct sta_data *psta = search_assoc_sta(addr, padapter);

	if (psta) {
		free_stadata(psta);
	}
}

struct sta_data	*alloc_stadata(int id, _adapter* padapter )
{

	int j = 0;
	int i;
	struct stainfo2	*pstainfo2;
//	unsigned long jiffies = 0;
	struct sta_data *psta_data=(struct sta_data *)&padapter->_sys_mib.sta_data;

	
	if (id == -1) {
//FIXME		
#if 1		
		//find the first unused...
	
		//for(i = 0; i < MAX_STASZ; i++)
		for(i = 5; i < MAX_STASZ; i++)
		{
			if ((psta_data[i].status == -1))
			{
				psta_data[i].status = 1;	
				pstainfo2 = psta_data[i].info2;
				return &(psta_data[i]);
			} else {
					// find the oldest
					if (psta_data[i]._protected == 0) {
						if (psta_data[i].jiffies > jiffies) {
							jiffies = psta_data[i].jiffies;
							j = i;	
						}
					}
			}
			
		}
#endif				
		
		// remove the original sta...
		//invalidate_cam(j);
		psta_data[j].status = 1;
		list_del_init(&(psta_data[j].assoc));
		list_del_init(&(psta_data[j].sleep));
		list_del_init(&(psta_data[j].hash));
		return &(psta_data[j]);
	}
	else {
		// only protected mode can use fixed type allocation
		j = id;
		//invalidate_cam(j);
		psta_data[j].status = 1;
		psta_data[j]._protected = 1;
		list_del_init(&(psta_data[j].assoc));
		list_del_init(&(psta_data[j].sleep));
		list_del_init(&(psta_data[j].hash));
		return &(psta_data[j]);
	}
	
}

/*


	caller must guarantee the atomix context...
	
	id: (-1) 	==> no specified entry
		(>=0) 	==> specified entry

	type: (-1)  ==> don't 	put into the assoc/hash search list
		  (>=0) ==> 		put into the assoc/hash search list


*/
struct sta_data* add_assoc_sta(unsigned char *addr, int id, int type, _adapter* padapter)
{
	//unsigned long flags;
	int hash;
	struct	sta_data	*psta = NULL;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	DEBUG_INFO(("+add_assoc_sta\n"));

	psta = alloc_stadata(id, padapter);
	if (psta) {
			_memcpy(psta->info2->addr, addr, ETH_ALEN);
	
			if (type != -1) {		
				hash = wifi_mac_hash(addr);
//				list_add_tail(&(psta->hash), &(_sys_mib->hash_array[hash]));

				list_add_tail(&(psta->hash), &(_sys_mib->hash_array[hash]));
				list_add_tail(&(psta->assoc), &(_sys_mib->assoc_entry));
			}
	}

	return psta;
}



#if 0
#include <config.h>
#include <sched.h>
#include <types.h>
#include <console.h>
#include <string.h>
#include <system32.h>
#include <sched.h>
#include <wlan.h>
#include <wlan_offset.h>
#include <timer.h>
#include <circ_buf.h>
#include <wifi.h>
#include <irp.h>
#include <wlan_sme.h>
#include <wireless.h>
#include <mp_pktinf.h>


//static u8 P802_1H_OUI[P80211_OUI_LEN] = { 0x00, 0x00, 0xf8 };
static u8 RFC1042_OUI[P80211_OUI_LEN] = { 0x00, 0x00, 0x00 };

unsigned char dot11_rate_table[]={2,4,11,22,12,18,24,36,48,72,96,108,0}; // last element must be zero!!

/*
if (id == -1)
	==> No specify Entry 
else
	id == the specified entry
	
Caller must guarantee the atomic context to prevent racing condition
	
*/


unsigned char get_rate_from_bit_value(int bit_val)
{
	int i;
	for (i=0; dot11_rate_table[i]; i++) {
		if (bit_val & (1<<i))
			return dot11_rate_table[i];
	}
	return 0;
}


int get_rate_index_from_ieee_value(unsigned char val)
{
	int i;
	for (i=0; dot11_rate_table[i]; i++) {
		if (val == dot11_rate_table[i]) {
			return i;
		}
	}
	DEBUG_INFO(("Local error, invalid input rate for get_rate_index_from_ieee_value() [%d]!!\n", val));
	return 0;
}


int get_bit_value_from_ieee_value(unsigned char val)
{
	int i=0;
	while(dot11_rate_table[i] != 0) {
		if (dot11_rate_table[i] == val)
			return BIT(i);
		i++;
	}
	return 0;
}




struct	sta_data* search_assoc_sta(unsigned char *addr)
{
	unsigned long		flags;
	unsigned int		hash;
	struct list_head	*plist;
	struct sta_data		*psta = NULL;
	struct stainfo2		*info2 = NULL;
	
	hash = wifi_mac_hash(addr);
	
	local_irq_save(flags);
	
	plist = &(sys_mib.hash_array[hash]);
	
	while((plist->next) != (&(sys_mib.hash_array[hash]))) {
		plist = plist->next;
		psta = list_entry(plist, struct sta_data ,hash);
		
		info2 = psta->info2;
		
		if (!(memcmp((void *)info2->addr, (void *)addr, ETH_ALEN))) {
			local_irq_restore(flags);
            return psta;
   		}
	}

	local_irq_restore(flags);

	return NULL;
	
}



void del_assoc_sta(_adapter *padapter, unsigned char *addr)
{

	struct	sta_data *psta;
	
	psta = search_assoc_sta(addr);

	if (psta) {
		free_stadata(psta);
	}
}



#if 0
void init_pktinfo(PACKET_INFO *pinfo)
{
	
	
	
}
#endif

void 
set_wlanhdr(unsigned char * pbuf, u8* da, u8 *sa, u8 *ra)
{

	//int	hdr_len;
	unsigned short *fctrl;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)pbuf;
	u8	*bssid = sys_mib.cur_network.MacAddress;
	u8	*mac = sys_mib.mac_ctrl.mac;

	fctrl = &(hdr->frame_ctl);

	*(fctrl) = 0;

	

	//hdr_len = sizeof (struct ieee80211_hdr_3addr);
	switch (sys_mib.mac_ctrl.opmode) {
		case IW_MODE_ADHOC:
			_memcpy(hdr->addr1, da, ETH_ALEN);
			_memcpy(hdr->addr2, mac, ETH_ALEN);
			_memcpy(hdr->addr3, bssid, ETH_ALEN);
			break;
		case IW_MODE_MASTER:
			SetFrDs(fctrl);
			_memcpy(hdr->addr1, da, ETH_ALEN);
			_memcpy(hdr->addr2, bssid, ETH_ALEN);
			_memcpy(hdr->addr3, sa, ETH_ALEN);
			break;
		case IW_MODE_REPEAT:
			SetFrDs(fctrl);
			SetToDs(fctrl);
			_memcpy(hdr->addr1, ra, ETH_ALEN);
			_memcpy(hdr->addr2, mac, ETH_ALEN);
			_memcpy(hdr->addr3, da, ETH_ALEN);
			_memcpy(hdr->addr4, sa, ETH_ALEN);
			break;
		case IW_MODE_INFRA:
		default:
			_memcpy(hdr->addr1, bssid, ETH_ALEN);
			_memcpy(hdr->addr2, mac, ETH_ALEN);
			_memcpy(hdr->addr3, da, ETH_ALEN);
			SetToDs(fctrl);
			break;
	}

	//marc
	hdr->duration_id = 0x0;
	hdr->seq_ctl	 = 0x0;

}

/*
 u8    dsap;         
 u8    ssap;         
 u8    ctrl;  
*/
void llc_hdr_gen(unsigned char *pbuf)
{

	//marc
	//struct ieee80211_snap_hdr *snap_hdr = (struct ieee80211_snap_hdr *)(pbuf + 3);
	struct ieee80211_snap_hdr *snap_hdr = (struct ieee80211_snap_hdr *)(pbuf);
	
	//marc
	//_memcpy(pbuf, RFC1042_OUI, 3);
	_memcpy(pbuf + 3, RFC1042_OUI, 3);
	
	snap_hdr->dsap = 0xaa;
	snap_hdr->ssap = 0xaa;
	snap_hdr->ctrl = 0x03;


}

#ifdef CONFIG_FW_WLAN_LBK

extern unsigned int tx_cnt;

int PayloadGen(PPACKET_INFO pPktInfo, u8 *pload){
    unsigned int i;
    //u16 ISeed = pPktInfo->initialSeed;
    //u16 dataLen = pPktInfo->payloadLength;
	
	if ((pPktInfo->payld_len) == 0) 
		return 0 ;
#if 0
	unsigned int sn = setGlobalSequenceNumber();

	// write Sequence Number
	*pload = ( sn & 0x000000FF);
	pload++;
	*pload = ((sn & 0x0000FF00) >> 8); 
	pload++;
	*pload = ((sn & 0x00FF0000) >> 16); 
	pload++;
	*pload = ((sn & 0xFF000000) >> 24); 
	pload++;

	// write DataType
	*pload = (pPktInfo->payloadType & 0x00FF); 
	pload++;
	*pload = ((pPktInfo->payloadType & 0xFF00) >> 8); 
	pload++;

	// write Len
	*pload = ((pPktInfo->payloadLength + PayloadFormat::getPayloadHeaderLength()) & 0x00FF); 
	pload++;
	*pload = (((pPktInfo->payloadLength + PayloadFormat::getPayloadHeaderLength()) & 0xFF00) >> 8); 
	pload++;

	// write InitSeed
	*pload = (pPktInfo->initialSeed & 0x00FF); 
	pload++;
	*pload = ((pPktInfo->initialSeed & 0xFF00) >> 8); 
	pload++;

	// write BurstType
	*pload = (pPktInfo->burstType & 0x00FF); 
	pload++;
	*pload = ((pPktInfo->burstType & 0xFF00) >> 8); 
	pload++;

	// write Current Burst
	*pload = pPktInfo->burstCount; 
	pload++;
	*pload = pPktInfo->burstCount; 
	pload++;

    //generate the pattern of payload depending on pPktInfo->_DataType ;
	switch(pPktInfo->payloadType) {
		case 0:
			for(i = 0 ; i < dataLen ; i++){
				*pload = 0;
				pload++;
			}
			break; 
		case 1:
			for(i = 0 ; i < dataLen ; i++){
				*pload = 0xFF;
				pload++;
			}
			break;
		case 2:
			for(i = 0 ; i < dataLen ; i++) {
				if ((i % 2) == 0) *pload = 0x55;
				else *pload = 0xAA;
				pload++;
			}  
			break;
		case 3:
			for(i = 0 ; i < dataLen ; i++) {
			   if ((i % 2) == 0) *pload = 0x5A;
			   else *pload = 0xA5;
			   pload++;
			}  
			break;
		case 4:
			i = 0;
			while(i < dataLen){
				*pload = (ISeed & 0x00FF);
				i++; pload++;
				if (i < dataLen){
					*pload = ((ISeed & 0xFF00) >> 8);
					i++; pload++;
				}
				ISeed++;
			}

			break;
		case 5:
			i = 0;
			while(i < dataLen){
				*pload = (ISeed & 0x00FF);
				i++; pload++;
				if (i < dataLen){
					*pload = ((ISeed & 0xFF00) >> 8);
					i++; pload++;
				}
				ISeed--;
			}
           break;
      default:
           break;
    }     
	
	// set Frame Length
	if (pPktInfo->cmdHWPC == 0){
		pPktInfo->wlanFrameLength = pPktInfo->wlanHeaderLength + pPktInfo->payloadLength + PayloadFormat::getPayloadHeaderLength() + FCS_LENGTH;
		switch(pPktInfo->securityType){
			case WEP:
				pPktInfo->wlanFrameLength += WEP_LENGTH;
				break;
			case TKIP_WITHOUT_MIC:
				pPktInfo->wlanFrameLength += TKIP_WITHOUT_MIC_LENGTH;
				break;
			case TKIP_WITH_MIC:
				pPktInfo->wlanFrameLength += TKIP_WITH_MIC_LENGTH;
				break;
			case CCMP:
				pPktInfo->wlanFrameLength += CCMP_LENGTH;
				break;
			case WEP_104:
				pPktInfo->wlanFrameLength += WEP104_LENGTH;
				break;
			case NONE:
			default:
				break;
		}

	}else{
		SHOWMSG("Calculate Hardware Packet Conversion.");
		pPktInfo->wlanFrameLength = pPktInfo->wlanHeaderLength + pPktInfo->payloadLength + PayloadFormat::getPayloadHeaderLength();
	}
#endif

	_memcpy(pload, &tx_cnt, sizeof(unsigned int));

	for(i = 0, pload += sizeof(unsigned int); i < (pPktInfo->payld_len) - sizeof(unsigned int); i++){
		*pload = (unsigned char)(i & 0xff);
		pload++;
	}
	
	return 1;
}


static __inline__ unsigned char *payld_offset(PACKET_INFO *pPktInfo)
{

	return (pPktInfo->pframe + pPktInfo->hdr_len + pPktInfo->iv_len + pPktInfo->llc_len);

}
	
void HeaderGen(PACKET_INFO *pinfo, u8 *pwheader);

void MPPacketGen(PACKET_INFO *pPktInfo)
{

#if 0 
	int idx = 0;
	
	if (pPktInfo->cmdHWPC == 1){

		// Generate Ether II or 802.3 frame for Hardware Packet Conversion
		
		if (pPktInfo->cmdToDS == 1){
			// Fill DS = Address3
			for (int i = 0; i < 6; i++){
				pPktInfo->pframe[idx++] = pPktInfo->wlanAddr3[i];
			}

			// Fill SA = Address2
			for (int i = 0; i < 6; i++){
				pPktInfo->pframe[idx++] = pPktInfo->wlanAddr2[i];
			}

		}else{

			// Fill DS = Address1
			for (int i = 0; i < 6; i++){
				pPktInfo->pframe[idx++] = pPktInfo->wlanAddr1[i];
			}

			// Fill SA = Address2
			for (int i = 0; i < 6; i++){
				pPktInfo->pframe[idx++] = pPktInfo->wlanAddr2[i];
			}
		}

		if (pPktInfo->cmdPacketType == 0){
			
			SHOWMSG("Generate Ether II frame for Hardware Packet Conversion.");

			// Fill Type
			if (pPktInfo->cmdOUI == 0){
				pPktInfo->pframe[idx++] = 0x80;
				pPktInfo->pframe[idx++] = 0xF3;
			}else{
				pPktInfo->pframe[idx++] = 0xFF;
				pPktInfo->pframe[idx++] = 0xFF;
			}

		}else{

			SHOWMSG("Generate 802.3 frame for Hardware Packet Conversion.");

			// Fill Len
			pPktInfo->pframe[idx++] = ((pPktInfo->payloadLength + PayloadFormat::getPayloadHeaderLength()) & 0x00FF00) >> 8;
			pPktInfo->pframe[idx++] = (pPktInfo->payloadLength + PayloadFormat::getPayloadHeaderLength()) & 0x00FF;

		}

		pPktInfo->wlanHeaderLength = idx;

		PayloadGen(pPktInfo, pPktInfo->pframe + pPktInfo->wlanHeaderLength);


	}else{

#endif		

		// Generate 802.11 Header and Body

		HeaderGen(pPktInfo, pPktInfo->pframe);

		switch(pPktInfo->securityType){
			case WEP:
			case WEP_104:
				// fill IV
				pPktInfo->pframe[pPktInfo->hdr_len] = pPktInfo->wlanInitVector[0];
				pPktInfo->pframe[pPktInfo->hdr_len + 1] = pPktInfo->wlanInitVector[1];
				pPktInfo->pframe[pPktInfo->hdr_len + 2] = pPktInfo->wlanInitVector[2];
				pPktInfo->pframe[pPktInfo->hdr_len + 3] = (pPktInfo->wlanKeyID << 6);
				//PayloadGen(pPktInfo, payld_offset(pPktInfo));
				break;

			case TKIP:
				// fill IV & EIV
				pPktInfo->pframe[pPktInfo->hdr_len] = pPktInfo->wlanTSC[1];
				pPktInfo->pframe[pPktInfo->hdr_len+1] = ((pPktInfo->wlanTSC[1] | 0x20) & 0x7F);
				pPktInfo->pframe[pPktInfo->hdr_len+2] = pPktInfo->wlanTSC[0];
				pPktInfo->pframe[pPktInfo->hdr_len+3] = (pPktInfo->wlanKeyID << 6) | 0x20;
				pPktInfo->pframe[pPktInfo->hdr_len+4] = pPktInfo->wlanTSC[2];
				pPktInfo->pframe[pPktInfo->hdr_len+5] = pPktInfo->wlanTSC[3];
				pPktInfo->pframe[pPktInfo->hdr_len+6] = pPktInfo->wlanTSC[4];
				pPktInfo->pframe[pPktInfo->hdr_len+7] = pPktInfo->wlanTSC[5];
				//PayloadGen(pPktInfo, pPktInfo->pframe + 
				//pPktInfo->hdr_len + pPktInfo->iv_len + pPktInfo->llc_len);
				break;

			case AES:
				// fill CCMP Header
				pPktInfo->pframe[pPktInfo->hdr_len] = pPktInfo->wlanPN[0];
				pPktInfo->pframe[pPktInfo->hdr_len + 1] = pPktInfo->wlanPN[1];
				pPktInfo->pframe[pPktInfo->hdr_len + 2] = 0x00;
				pPktInfo->pframe[pPktInfo->hdr_len + 3] = (pPktInfo->wlanKeyID << 6) | 0x20;
				pPktInfo->pframe[pPktInfo->hdr_len + 4] = pPktInfo->wlanPN[2];
				pPktInfo->pframe[pPktInfo->hdr_len + 5] = pPktInfo->wlanPN[3];
				pPktInfo->pframe[pPktInfo->hdr_len + 6] = pPktInfo->wlanPN[4];
				pPktInfo->pframe[pPktInfo->hdr_len + 7] = pPktInfo->wlanPN[5];
				//PayloadGen(pPktInfo, pPktInfo->pframe + 
				//pPktInfo->hdr_len + pPktInfo->iv_len + pPktInfo->llc_len);
				break;

			case NONE:
			default:
				//PayloadGen(pPktInfo, pPktInfo->pframe + 
				//pPktInfo->hdr_len + pPktInfo->llc_len);
				break;
		}
	//}


	if (pPktInfo->cmdHWPC == 0) {

		//add L2 Header...

		llc_hdr_gen(pPktInfo->pframe + pPktInfo->hdr_len + pPktInfo->iv_len);

	}
	
	PayloadGen(pPktInfo, payld_offset(pPktInfo));

	return;

	
}


void HeaderGen(PACKET_INFO *pinfo, u8 *pwheader)
{
	set_wlanhdr(pinfo->pframe, pinfo->da, pinfo->sa, pinfo->ra);
	
	//marc
	//SetFrameType(pinfo->pframe, pinfo->wlanFrameSubType);
	SetFrameSubType(pinfo->pframe, (pinfo->wlanFrameSubType | pinfo->wlanFrameType));
	
	if (pinfo->securityType)
		SetPrivacy(pinfo->pframe);
	
	if (pinfo->cmdQoS)
		SetQC(pinfo->pframe + pinfo->hdr_len - 2, (pinfo->priority));
	

}

extern volatile int	rx_cnt;
unsigned int rcv_cnt = 0;
//unsigned int rcv_cnt = 0x5c;

int PayloadCheck(unsigned char* payload_buf, unsigned int len)
{
	unsigned int i, cnt;
	unsigned int status = 0;
	unsigned long flags;
	
	_memcpy(&cnt, payload_buf, sizeof(unsigned int));

#if 1 	
	if (cnt != rcv_cnt)
	{
		
		rtl_outl(0xbd621000, 0xffffffff);
		
		DEBUG_ERR(("lost packet %x(%x)\n", rcv_cnt, cnt));
		while (1);
		
		return 1;
	}
#endif
	
#if 1
	payload_buf += sizeof(unsigned int);
	
	for (i = status = 0; i < (len - sizeof(unsigned int)); i++, payload_buf++)
	{
		if (*payload_buf != (unsigned char)(i & 0xff))
		{
			status = 1;
			rtl_outl(0xbd621000, 0xffffffff);
			printk("payload error @ %x(%x): \n", (*payload_buf), i);
			while (1);
		}
	}
#endif

	if ((rcv_cnt % 1024)== 0)
		DEBUG_INFO(("rx: %x!\n", rcv_cnt));
	
	rcv_cnt++;

	return status;
}

#define SWAP(x)	(((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24))

int MPPacketCheck(struct rxirp *rx_irp)
{
	unsigned char* packet_buf = (unsigned char *)Physical2NonCache(rx_irp->rxbuf_phyaddr);
#if 0	
	struct ieee80211_hdr_3addr* hdr = (struct ieee80211_hdr_3addr*)packet_buf;
#endif	
	unsigned int playloadLen, headerLen;

	if (rx_irp->tag == RXMGTIRPS_TAG)
		headerLen = 16 + sizeof(struct ieee80211_hdr_3addr);
	else
		headerLen = 16 + sizeof(struct ieee80211_hdr_3addr) + SNAP_SIZE;

#if 0
	if (GetPrivacy(hdr))
	{
		//calculate the header according to the security type
	}
#endif

	//check the frame control to decide the header length
	playloadLen = ((SWAP(*(unsigned int*)packet_buf)) & 0xfff) - (headerLen + 4 - 16);
	
	PayloadCheck(packet_buf + headerLen, playloadLen);

	return 0;	
}

void check_rx_lbkirps(void)
{
	
	struct rxirp *rx_irp;
	unsigned long	flags;
	unsigned int 	*pframe;
	unsigned int	i, crc;
	
	while(!(list_empty(&sys_mib.rx_datalist)))
	{
		//DEBUG_INFO(("received a rx frame!\n"));
		
		local_irq_save(flags);
		rx_irp = list_entry(sys_mib.rx_datalist.next, struct rxirp, list);	
		
		list_del_init(&(rx_irp->list));
		local_irq_restore(flags);
		
#ifdef CONFIG_CRC_TEST		
		pframe = get_rxpframe(rx_irp);
		crc = le32_to_cpu(getCRC32((unsigned char *)pframe, (rx_irp->rx_len - 20)));
		
		pframe -= 4;
		
		DEBUG_INFO(("crc: %x\n", crc));
			
		for (i = 0; i < rx_irp->rx_len / sizeof(unsigned int); i++)
		{
			DEBUG_INFO(("%x\n", *(pframe + i)));
		}
		
		if (crc != *(pframe + (rx_irp->rx_len / sizeof(unsigned int)) - 1))
		{
#if 0			
			DEBUG_INFO(("crc: %x\n", crc));
			
			for (i = 0; i < rx_irp->rx_len / sizeof(unsigned int); i++)
			{
				DEBUG_INFO(("%x\n", *(pframe + i)));
			}
#endif			
			while (1);
		}
		else
		{
			if ((rcv_cnt % 1024) == 0)
				DEBUG_INFO(("crc check ok[%x]\n", rcv_cnt));
			
			rcv_cnt++;
			
		}
#else
		MPPacketCheck(rx_irp);
#endif		
		
		free_rxirp(rx_irp);
		
		rx_cnt ++;
		
	}


}

#endif

unsigned char * get_sa(unsigned char *pframe)
{
	unsigned char 	*sa;
	unsigned int	to_fr_ds	= (GetToDs(pframe) << 1) | GetFrDs(pframe);

	switch (to_fr_ds) {
		case 0x00:	// ToDs=0, FromDs=0
			sa = GetAddr2Ptr(pframe);
			break;
		case 0x01:	// ToDs=0, FromDs=1
			sa = GetAddr3Ptr(pframe);
			break;
		case 0x02:	// ToDs=1, FromDs=0
			sa = GetAddr2Ptr(pframe);
			break;
		default:	// ToDs=1, FromDs=1
			sa = GetAddr4Ptr(pframe);
			break;
	}

	return sa;
}

unsigned char * get_da(unsigned char *pframe)
{
	unsigned char 	*da;
	unsigned int	to_fr_ds	= (GetToDs(pframe) << 1) | GetFrDs(pframe);

	switch (to_fr_ds) {
		case 0x00:	// ToDs=0, FromDs=0
			da = GetAddr1Ptr(pframe);
			break;
		case 0x01:	// ToDs=0, FromDs=1
			da = GetAddr1Ptr(pframe);
			break;
		case 0x02:	// ToDs=1, FromDs=0
			da = GetAddr3Ptr(pframe);
			break;
		default:	// ToDs=1, FromDs=1
			da = GetAddr3Ptr(pframe);
			break;
	}

	return da;
}

/* any station allocated can be searched by hash list */
struct	sta_data *get_stainfo(unsigned char *hwaddr)
{
	struct list_head	*plist;
	int	index;
	struct	sta_data	*psta;

	index = wifi_mac_hash(hwaddr);
	plist = &(sys_mib.hash_array[index]);

	while (plist->next != &(sys_mib.hash_array[index]))
	{
		plist = plist->next;
		psta = list_entry(plist, struct sta_data ,hash);
		if (!(memcmp((void *)psta->info2->addr, (void *)hwaddr, ETH_ALEN))) { // if found the matched address
			return psta;
		}
	}
	return (struct sta_data *)NULL;
}



#endif

