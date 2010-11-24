/*

	MIPS-16 Mode if CONFIG_MIPS16 is on

*/

#define _RTL8187_RELATED_FILES_C
#define _WLAN_SME_C_

#include <drv_types.h>
#include <wlan_util.h>
#include <wlan_mlme.h>
#include <wifi.h>

#include <rtl8192u_spec.h>
#include <rtl8192u_regdef.h>


unsigned char	cck_basicrate[LAZY_NumRates] ={0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, _11M_RATE_, _5M_RATE_, _2M_RATE_, _1M_RATE_, 0xff};
unsigned char	cck_datarate[LAZY_NumRates] ={0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, _11M_RATE_, _5M_RATE_, _2M_RATE_, _1M_RATE_, 0xff};	

unsigned char	ofdm_datarate[LAZY_NumRates] = {_54M_RATE_, _48M_RATE_, _36M_RATE_, _24M_RATE_, _18M_RATE_,_12M_RATE_, _9M_RATE_, _6M_RATE_, 0xff, 0xff, 0xff, 0xff, 0xff};
unsigned char	mixed_datarate[LAZY_NumRates] = {_54M_RATE_, _48M_RATE_, _36M_RATE_, _24M_RATE_, _18M_RATE_,_12M_RATE_, _9M_RATE_, _6M_RATE_, _11M_RATE_, _5M_RATE_, _2M_RATE_, _1M_RATE_, 0xff};


unsigned char	mixed_basicrate[NumRates] ={0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, _11M_RATE_, _5M_RATE_, _2M_RATE_, _1M_RATE_, 0xff};
void write_cam(_adapter *padapter, unsigned char entry, unsigned short ctrl, unsigned char *mac, unsigned char *key)
{
	//unsigned long flags;
	unsigned int i, j, val, addr;
	addr = entry * 6;
	
#ifdef CONFIG_RTL8192U
	addr = entry * 8;
#endif

	_down_sema(&(padapter->_sys_mib.cam_sema));
	//local_irq_save(flags);

	val = (ctrl | (mac[0] << 16) | (mac[1] << 24) );
	printk("write_cam(entry %d): %x\n",entry, val);
	write32(padapter, WCAMI, val);
	write32(padapter, RWCAM, BIT(31)|BIT(16)|(addr&0xff) );
	addr ++;
	
	val = (mac[2] | ( mac[3] << 8) | (mac[4] << 16) | (mac[5] << 24));
	printk("write_cam(entry %d): %x\n",entry, val);
	write32(padapter, WCAMI, val);
	write32(padapter, RWCAM,BIT(31)|BIT(16)|(addr&0xff));
	addr ++;

	for(j =0 ; j < 0x10; j++);	//wait for 100ns...

	for(i = 0; i < 16; i+=8) {
		val = (key[i] | (key[i+1] << 8) | (key[i+2] << 16) | (key[i+3] << 24));
		write32(padapter, WCAMI, val);
		write32(padapter, RWCAM, BIT(31)|BIT(16)|(addr&0xff) );
		addr ++;

		printk("write_cam(entry %d): %x\n",entry, val);
		val = (key[i+4] | (key[i+5] << 8) | (key[i+6] << 16) | (key[i+7] << 24));
		write32(padapter, WCAMI, val);
		write32(padapter, RWCAM, BIT(31)|BIT(16)|(addr&0xff) );
		addr ++;

		printk("write_cam(entry %d): %x\n",entry, val);	
		for(j =0 ; j < 0x10; j++);	//wait for 100ns...
	}

	//local_irq_restore(flags);
	_up_sema(&(padapter->_sys_mib.cam_sema));

}


void setup_assoc_sta(unsigned char *addr, int camid, int mode, unsigned short ctrl, unsigned char *key, _adapter* padapter)
{
	int id;
	struct sta_data *psta;
	
	psta = add_assoc_sta(addr, camid, mode, padapter);

	if (camid == -1)
		id = psta->info2->aid;
	else
		id = camid;
	
	if(camid<4)
	{
		printk("\n setup_assoc_sta: write_cam camid=%d id=0x%x ctrl=0x%x  addr=%.2x %.2x%.2x%.2x%.2x%.2x\n",camid,id,ctrl,addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
		if( !((padapter->_sys_mib.mac_ctrl.opmode & WIFI_ADHOC_STATE) && (camid == -1)) )
		{
			printk("setup_assoc_sta: write_cam\n");
			write_cam(padapter,id, ctrl, addr, key);
		}
	}
	else{
		if((camid ==4)&&(mode==4)){
			printk("setup_assoc_sta: write_cam\n");
			write_cam(padapter,id, ctrl, addr, key);
		}	
	}
}	

unsigned char	rate_tbl[] = {_54M_RATE_, _48M_RATE_, _36M_RATE_, _24M_RATE_, _18M_RATE_,_12M_RATE_, _9M_RATE_, _6M_RATE_, _11M_RATE_, _5M_RATE_, _2M_RATE_, _1M_RATE_};

int	wifirate2_ratetbl_inx(unsigned char rate)
{
	int	inx = 0;
	rate = rate & 0x7f;

	switch (rate) {

		case 54*2:
			inx = 0;
			break;

		case 48*2:
			inx = 1;
			break;

		case 36*2:
			inx = 2;
			break;

		case 24*2:
			inx = 3;
			break;
			
		case 18*2:
			inx = 4;
			break;

		case 12*2:
			inx = 5;
			break;

		case 9*2:
			inx = 6;
			break;
			
		case 6*2:
			inx = 7;
			break;

		case 11*2:
			inx = 8;
			break;
		case 11:
			inx = 9;
			break;

		case 2*2:
			inx = 10;
			break;
		
		case 1*2:
			inx = 11;
			break;

	}

	return inx;
	

}

unsigned char ratetbl_val_2wifirate(unsigned char rate)
{
	unsigned char val = 0;

	switch (rate & 0x7f) {

		case 0:
			val = IEEE80211_CCK_RATE_1MB;
			break;

		case 1:
			val = IEEE80211_CCK_RATE_2MB;
			break;

		case 2:
			val = IEEE80211_CCK_RATE_5MB;
			break;

		case 3:
			val = IEEE80211_CCK_RATE_11MB;
			break;
			
		case 4:
			val = IEEE80211_OFDM_RATE_6MB;
			break;

		case 5:
			val = IEEE80211_OFDM_RATE_9MB;
			break;

		case 6:
			val = IEEE80211_OFDM_RATE_12MB;
			break;
			
		case 7:
			val = IEEE80211_OFDM_RATE_18MB;
			break;

		case 8:
			val = IEEE80211_OFDM_RATE_24MB;
			break;
			
		case 9:
			val = IEEE80211_OFDM_RATE_36MB;
			break;

		case 10:
			val = IEEE80211_OFDM_RATE_48MB;
			break;
		
		case 11:
			val = IEEE80211_OFDM_RATE_54MB;
			break;

	}

	return val;

}



static __inline int  is_basicrate(_adapter *padapter, unsigned char rate)
{

	int i;
	unsigned char val;
	for(i = 0; i < NumRates; i++)
	{
		val =padapter->_sys_mib.mac_ctrl.basicrate[i];

		if ((val != 0xff) && (val != 0xfe))
			if (rate == ratetbl_val_2wifirate(val))
				return _TRUE;
	}
	return _FALSE;
}

/*

This will convert mac_ctrl.basicrate / mac_ctrl.datarate 
to form the rateset string.

*/
unsigned int  ratetbl2rateset(_adapter *padapter, unsigned char *rateset, int *basic_rate_cnt)
{
	int i;
	unsigned char rate;
	unsigned int	len = 0;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);

	for(i = NumRates - 1; i >= 0; i--)
	{
#if 1//FIXME
		rate = _sys_mib->mac_ctrl.datarate[i];
#else
		rate = 0;
#endif
		if ((rate != 0xff) && (rate != 0xfe))
		{
			rate = ratetbl_val_2wifirate(rate);

			if (is_basicrate(padapter, rate) == _TRUE)
			{
				(*basic_rate_cnt) += 1;
				rate |= IEEE80211_BASIC_RATE_MASK;
			}
			
			rateset[len] = rate;
			len ++;
		}
	}

	return len;
}




// bssrate_ie: _SUPPORTEDRATES_IE_ get supported rate set
// bssrate_ie: _EXT_SUPPORTEDRATES_IE_ get extended supported rate set
//int get_bssrate_set(int bssrate_ie, unsigned char *pbssrate, int *bssrate_len)
void get_bssrate_set(_adapter *padapter, int bssrate_ie, unsigned char *pbssrate, int *bssrate_len)
{
	int cnt, ratelen, basic_rate_cnt=0;
	unsigned char supportedrates[16];

	memset(supportedrates, 0, 16);
	ratelen = *bssrate_len = 0;


	ratelen = ratetbl2rateset(padapter, supportedrates, &basic_rate_cnt);

	if (bssrate_ie == _SUPPORTEDRATES_IE_) {

		_memcpy(pbssrate, supportedrates, basic_rate_cnt);

		*bssrate_len = basic_rate_cnt;

	}
	else {

		cnt = ratelen - basic_rate_cnt;

		if (cnt > 0)
		{
			_memcpy(pbssrate, supportedrates + basic_rate_cnt, cnt);
			*bssrate_len = cnt;
		}
	}
	
}


static void finalize_ratetbl(unsigned char *ratetbl)
{

	unsigned char	val;
	int i, top, last = 0;

	for(i = NumRates - 1; i >= 0; i--)
	{
		val = ratetbl[i];

		if (val != 0xff)
			break;
	}

	if (i <= 0)
		return;

	last = i;

	for(i = 0; i < NumRates -1; i++)
	{
		val = ratetbl[i];

		if (val != 0xff)
			break;
	}
	
	if (i >= NumRates)
		return;

	top = i;


	//modify any pattern starting from top to last with the val = 0xff to 0xfe.

	if ((top + 1 ) >= last)
		return;

	for( i = top + 1; i < last; i++)
	{
		val = ratetbl[i];

		if (val == 0xff)
			ratetbl[i] = 0xfe;
	}

}



void rateset2ratetbl(unsigned char *ptn, unsigned int ptn_sz, 
	unsigned char *basicrate_tbl, unsigned char *datarate_tbl)
{

	int i, inx;
	unsigned char	rate;
	
	for(i = 0; i < ptn_sz; i++)
	{

		rate = ptn[i] & 0x7f;

		inx = wifirate2_ratetbl_inx(rate);
		
		if ((ptn[i]) & 0x80)	{
			basicrate_tbl[inx] = rate_tbl[inx];
		}
		
		datarate_tbl[inx] =  rate_tbl[inx];
	}


	finalize_ratetbl(basicrate_tbl);
	finalize_ratetbl(datarate_tbl);

}


int update_rate(unsigned char *tgt, unsigned char *ptn, unsigned int direction)
{
	int i;
	
	_memcpy(tgt, ptn, NumRates);
	
	if (direction)
	{
		for(i = 0; i < NumRates; i++)
		{
			if (tgt[i] != 0xff)
				break;
		}
	}
	else
	{
		for(i = (NumRates - 1); i >= 0; i--)
		{
			if (tgt[i] != 0xff)
				break;
		}
	}
	
	return i;
	
}

void update_supported_rate(_adapter *padapter, unsigned char *da, unsigned char *ptn, unsigned int ptn_sz )
{	
	unsigned char	basicrates[NumRates];
	unsigned char	datarates[NumRates];
	struct sta_data *psta_data;
	struct stainfo	*info;
//	struct sta_priv *pstapriv = &(padapter->stapriv);
	struct mib_info *pmibinfo = &(padapter->_sys_mib);
	int i = 0;
	unsigned short	regTemp = 0 , val = 0;
	
	
	psta_data =search_assoc_sta(da,padapter);

	if (psta_data == NULL)
		return;

	memset(basicrates, 0xff, NumRates);
	memset(datarates,  0xff, NumRates);
	
	rateset2ratetbl(ptn, ptn_sz, basicrates, datarates);

	for(i=0;i<NumRates;i++){
		datarates[i]|=pmibinfo->mac_ctrl.datarate[i];
	}
	
	info = psta_data->info;
	info->txrate_inx = update_rate(info->txrate, datarates, 1);
	pmibinfo->mac_ctrl.basicrate_inx = update_rate(pmibinfo->mac_ctrl.basicrate, basicrates, 1);

#ifdef LOWEST_BASIC_RATE	
	pmibinfo->mac_ctrl.lowest_basicrate_inx = update_rate(pmibinfo->mac_ctrl.basicrate, basicrates, 0);
#endif

	//update ARFR & BRSR
	for (i = 0; i< NumRates; i++)
	{
		if ((pmibinfo->mac_ctrl.basicrate[i]) != 0xff)
			val |= BIT((11 - i));
	}

#ifdef CONFIG_RTL8187	

	write16(padapter, BRSR,  val);

#elif defined(CONFIG_RTL8192U)

	val = val & 0x15f;
	regTemp = PlatformEFIORead4Byte(padapter, RRSR);
	regTemp &= 0xffff0000;
	regTemp &=  (~(RRSR_MCS4 | RRSR_MCS5 | RRSR_MCS6 | RRSR_MCS7));

	PlatformEFIOWrite4Byte(padapter, RRSR, val |regTemp);
	PlatformEFIOWrite4Byte(padapter, RATR0+4*7, val);

#endif

	val = 0;

#ifdef CONFIG_RTL8187
	for (i = 0; i< NumRates; i++)
	{
		if ((info->txrate[i]) != 0xff)
			val |= BIT((11 - i));
	}

#ifdef TODO
	write16(padapter, ARFR , val);
#endif

#endif

}


/*

rate is the 802.11 operating rates set


*/
static __inline int is_cckrates_included(unsigned char *rate, int ratelen)
{
	int	i;
	
	
	for(i = 0; i < ratelen; i++)
	{
		if  (  (((rate[i]) & 0x7f) == 2)	|| (((rate[i]) & 0x7f) == 4) ||
			   (((rate[i]) & 0x7f) == 11)  || (((rate[i]) & 0x7f) == 22) )
		return _TRUE;	
	}
	

	return _FALSE;

}

static __inline int is_cckratesonly_included(unsigned char *rate, int ratelen)
{
	int	i;
	
	
	for(i = 0; i < ratelen; i++)
	{
		if  (  (((rate[i]) & 0x7f) != 2) && (((rate[i]) & 0x7f) != 4) &&
			   (((rate[i]) & 0x7f) != 11)  && (((rate[i]) & 0x7f) != 22) )
		return _FALSE;	
	}
	return _TRUE;
}

int judge_network_type(unsigned char *rate, int ratelen, int channel)
{
	if (channel > 14)
	{
		if ((is_cckrates_included(rate, ratelen)) == _TRUE)
			return WIRELESS_INVALID;
		else
			return WIRELESS_11A;
	}	
	else  // could be pure B, pure G, or B/G
	{
		if 	((is_cckratesonly_included(rate, ratelen)) == _TRUE)	
			return WIRELESS_11B;
		else if ((is_cckrates_included(rate, ratelen)) == _TRUE)
			return 	WIRELESS_11BG;
		else
			return WIRELESS_11G;
	}
	
}

void set_slot_time(int slot_time)
{
	
	
}

/*
This sub-routine will update hardware parameters, based on the info 

saved in sys_mib.cur_network


	SIFS,
	DIFS,
	PIFS, 
	EIFS,	
	ACKTO,
	BcnItv,
	AtimWnd,
	BintrItv,
	AtimtrItv,
	ARFR	//based on the supported rates set...
	BRSR	
	update cam to replace the previous AP's entry, this is very important
	update rate table
	AC_VO_PARAM
	AC_VI_PARAM
	AC_BE_PARAM
	AC_BK_PARAM
	BM_MGT_PARAM
	BE_RETRY
	BK_RETRY
	VI_RETRY
	VO_RETRY
	TS_RETRY
	MGT_RETRY
	
	update beacon_buf, for Ad-HoC mode...
	
*/
#ifdef CONFIG_RTL8187
void join_bss(_adapter *padapter)
{
	//unsigned long flags;
	u8	msr, cr;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	unsigned char *assoc_bssid = _sys_mib->cur_network.MacAddress;
	
	//local_irq_save(flags);
	if(_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE){
#ifdef TODO
		msr = MSR_LINK_ENEDCA;
#endif
		msr = 0;
		write8(padapter, MSR, msr|NOLINKMODE);
	}

#ifdef TODO
	// to turn on LED (LINK/ACT, infrastructure)
	write8(padapter, CR9346, 0xC0);
	write8(padapter, CONFIG1, read8(padapter, CONFIG1)|CONFIG1_LEDS);
	write8(padapter, CR9346, 0x00);
#endif

	printk("BSSID = %x %x %x %x %x %x\n",assoc_bssid[0],assoc_bssid[1],assoc_bssid[2],assoc_bssid[3],assoc_bssid[4],assoc_bssid[5]);
	write32(padapter, BSSIDR, (assoc_bssid[0] | (assoc_bssid[1] << 8)  | (assoc_bssid[2] << 16) | (assoc_bssid[3] << 24)));

	write16(padapter, BSSIDR+ 4, assoc_bssid[4] | (assoc_bssid[5] << 8));
	
	write16(padapter, ATIMWND, 2);

#ifdef TODO
	write16(padapter, AtimtrItv, 80);	
#endif

	write16(padapter, BCNITV, get_beacon_interval(&_sys_mib->cur_network));

#ifdef TODO
	write16(padapter, BcnDma, 0x3FF);
	write8(padapter, BcnInt, 1);
#endif

#ifdef TODO
	msr = MSR_LINK_ENEDCA;
#endif
	msr =0;

	if (_sys_mib->mac_ctrl.opmode & WIFI_AP_STATE)		
		write8(padapter, MSR, msr|APMODE); 	
	else if (_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)		
		write8(padapter, MSR,msr|ADHOCMODE);
	else 		
		write8(padapter, MSR,msr|STAMODE);

	switch(_sys_mib->network_type)
	{
		case WIRELESS_11B:
			_sys_mib->rate = 110;
			printk(KERN_INFO"Using B rates\n");
			break;
		case WIRELESS_11G:
		case WIRELESS_11BG:
			_sys_mib->rate = 540;
			printk(KERN_INFO"Using G rates\n");
			break;
	}

#ifdef CONFIG_RTL8187
	ActSetWirelessMode8187(padapter, _sys_mib->network_type);

#endif
	if( _sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE) {
		cr = read8(padapter, CMDR);
		cr = (cr & (CR_RE|CR_TE));
		write8(padapter, CMDR, cr);
		printk("Card successfully reset for ad-hoc\n");
	}

	if ((_sys_mib->mac_ctrl.opmode  & WIFI_ADHOC_STATE) ||
		(_sys_mib->mac_ctrl.opmode & WIFI_AP_STATE)) {

		_sys_mib->mac_ctrl.bcn_size = setup_bcnframe(padapter, _sys_mib->mac_ctrl.pbcn);
		
		printk("bcn size: %d\n", _sys_mib->mac_ctrl.bcn_size);
		

#ifdef CONFIG_BEACON_CNT
		_sys_mib->bcnlimit  = CONFIG_BEACON_CNT;
#else
		_sys_mib->bcnlimit  = 0;
#endif	
				
		//printk("beacon limit =%x\n", _sys_mib->bcnlimit);
	}

	//local_irq_restore(flags);
	
}
#endif	
				




void setup_rate_tbl(_adapter *padapter)
{

	int i;
	unsigned char	*basic_rate;
	unsigned char *data_rate;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	unsigned char cur_phy = get_phy(_sys_mib);

	if ( cur_phy == CCK_PHY) {
		basic_rate = cck_basicrate;
		data_rate = cck_datarate;
	}
#if 0	
	else if ( cur_phy == OFDM_PHY) {
		basic_rate = ofdm_basicrate;
		data_rate = ofdm_datarate;
	}
#endif		
	else {  
		basic_rate = mixed_basicrate;
		data_rate = mixed_datarate;
	}
	
	_sys_mib->mac_ctrl.basicrate_inx = 
	update_rate(_sys_mib->mac_ctrl.basicrate, basic_rate, 1);
#if 0
	if ((chip_version == RTL8711_1stCUT) && (_sys_mib->mac_ctrl.basicrate_inx == 8))
		_sys_mib->mac_ctrl.basicrate_inx = 9;
#endif
	//printk("basicrate_inx:%d\n", sys_mib.mac_ctrl.basicrate_inx);

#ifdef LOWEST_BASIC_RATE	
	_sys_mib->mac_ctrl.lowest_basicrate_inx = 
	update_rate(_sys_mib->mac_ctrl.basicrate, basic_rate, 0);
#endif

	printk("===== basic rate set=====\n");
	for(i = 0; i < NumRates; i++)
		printk("%x ", _sys_mib->mac_ctrl.basicrate[i]);
	printk("\n");

	update_rate(_sys_mib->mac_ctrl.datarate, data_rate, 1);

	printk("===== data rate set=====\n");
	for(i = 0; i < NumRates; i++)
		printk("%x ", _sys_mib->mac_ctrl.datarate[i]);
	printk("\n");
	

	_sys_mib->stainfos[0].txrate_inx = 
	update_rate(_sys_mib->stainfos[0].txrate, basic_rate, 0);

	_sys_mib->stainfos[1].txrate_inx = 
	update_rate(_sys_mib->stainfos[1].txrate, basic_rate, 0);
	
	_sys_mib->stainfos[2].txrate_inx = 
	update_rate(_sys_mib->stainfos[2].txrate, basic_rate, 0);

	_sys_mib->stainfos[3].txrate_inx = 
	update_rate(_sys_mib->stainfos[3].txrate, basic_rate, 0);

	_sys_mib->stainfos[4].txrate_inx = 
	update_rate(_sys_mib->stainfos[4].txrate, basic_rate, 0);

	_sys_mib->stainfos[5].txrate_inx = 
	update_rate(_sys_mib->stainfos[5].txrate, data_rate, 1);

}

