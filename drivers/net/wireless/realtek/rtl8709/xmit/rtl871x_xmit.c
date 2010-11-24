/******************************************************************************
* rtl871x_xmit.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTL871X_XMIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtl8711_byteorder.h>
#include <wifi.h>
#include <osdep_intf.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifdef  PLATFORM_LINUX

#include <linux/rtnetlink.h>

#endif


#ifdef CONFIG_USB_HCI
#include <usb_ops.h>
#endif

#include <circ_buf.h>

#include <hal_init.h>


static u8 P802_1H_OUI[P80211_OUI_LEN] = { 0x00, 0x00, 0xf8 };
static u8 RFC1042_OUI[P80211_OUI_LEN] = { 0x00, 0x00, 0x00 };

#define consider_bm_ff 1

#ifdef CONFIG_RTL8711
#define HWXMIT_ENTRY	4
#endif


#ifdef CONFIG_RTL8192U
#include <rtl8192u/rtl8192u_haldef.h>
#endif

u8* get_xmit_payload_start_addr(struct xmit_frame *pxmitframe)
{
	u8* pframe;
	u32 alignment=0;

	
#ifdef CONFIG_RTL8187
	pframe=(u8 *)(pxmitframe->mem) + WLANHDR_OFFSET+TXDESC_OFFSET;
#endif /*CONFIG_RTL8187*/


#ifdef CONFIG_RTL8192U
#ifdef PLATFORM_DVR
	alignment = ALIGNMENT -( (u32)((u8 *)pxmitframe->mem+WLANHDR_OFFSET)& (ALIGNMENT-1) );
#endif
	pframe=(u8 *)(pxmitframe->mem) + WLANHDR_OFFSET+TXDESC_OFFSET+TX_FWINFO_OFFSET;
	pframe += alignment;
	DEBUG_INFO(("add_mic : alignemnt :% d, addr : %p\n",alignment, (u8 *)pframe));
#endif /*CONFIG_RTL8192U*/


#ifdef CONFIG_RTL8711
	pframe=(u8 *)(pxmitframe->mem) + WLANHDR_OFFSET;
#endif /*CONFIG_RTL8711*/

	return pframe;
}

uint remainder_len(struct pkt_file *pfile)
{
	return (pfile->buf_len - ((u32)(pfile->cur_addr) - (u32)(pfile->buf_start)));
}

sint endofpktfile (struct pkt_file *pfile)
{
_func_enter_;
	if (pfile->pkt_len == 0){
_func_exit_;	
		return _TRUE;
	}
	else{
_func_exit_;	
		return _FALSE;
	}
}

void _init_txservq(struct tx_servq *ptxservq)
{
_func_enter_;
	_init_listhead(&ptxservq->tx_pending);
	_init_queue(&ptxservq->sta_pending);
_func_exit_;		
}

void	_init_sta_xmit_priv(struct sta_xmit_priv *psta_xmitpriv)
{
	int i;
_func_enter_;
	memset((unsigned char *)psta_xmitpriv, 0, sizeof (struct sta_xmit_priv));

	_spinlock_init(&psta_xmitpriv->lock);
	
	for(i = 0 ; i < MAX_NUMBLKS; i++)
		_init_txservq(&(psta_xmitpriv->blk_q[i]));

	_init_txservq(&psta_xmitpriv->be_q);
	_init_txservq(&psta_xmitpriv->bk_q);
	_init_txservq(&psta_xmitpriv->vi_q);
	_init_txservq(&psta_xmitpriv->vo_q);
	_init_listhead(&psta_xmitpriv->legacy_dz);
	_init_listhead(&psta_xmitpriv->apsd);
_func_exit_;	
}

sint _init_hw_txqueue(struct hw_txqueue* phw_txqueue, u8 ac_tag)
{
_func_enter_;
	phw_txqueue->ac_tag = ac_tag;
#ifdef CONFIG_RTL8711
	switch(ac_tag)
	{
		case BE_QUEUE_INX:
			phw_txqueue->ff_hwaddr = BEFFWP;
			phw_txqueue->cmd_hwaddr = BECMD;
			phw_txqueue->free_sz= BE_FF_SZ*4;
			break;

		case BK_QUEUE_INX:
			phw_txqueue->ff_hwaddr = BKFFWP;
			phw_txqueue->cmd_hwaddr = BKCMD;
			phw_txqueue->free_sz= BK_FF_SZ*4;
			break;
		
		case VI_QUEUE_INX:
			phw_txqueue->ff_hwaddr = VIFFWP;
			phw_txqueue->cmd_hwaddr = VICMD;
			phw_txqueue->free_sz= VI_FF_SZ*4;
			break;

		case VO_QUEUE_INX:
			phw_txqueue->ff_hwaddr = VOFFWP;
			phw_txqueue->cmd_hwaddr = VOCMD;
			phw_txqueue->free_sz= VO_FF_SZ*4;
			break;

		case TS_QUEUE_INX:
			phw_txqueue->ff_hwaddr = TSFFWP;
			phw_txqueue->cmd_hwaddr = TSCMD;
			phw_txqueue->free_sz= TS_FF_SZ*4;		
			break;

		case BMC_QUEUE_INX:
			phw_txqueue->ff_hwaddr = BMFFWP;
			phw_txqueue->cmd_hwaddr = BMCMD;
			phw_txqueue->free_sz= BMC_FF_SZ*4;
			break;

	}
#endif
_func_exit_;	
	return _SUCCESS;

}


sint update_attrib(_adapter *padapter, _pkt *pkt, struct pkt_attrib *pattrib)
{
	uint i;
	sint bmcast;
	struct pkt_file pktfile;
	struct sta_info *psta=NULL;
	struct ethhdr etherhdr;
	struct tx_cmd mp_txcmd;
	
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;	
	struct sta_priv *pstapriv = &padapter->stapriv;
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	struct qos_priv *pqospriv = &padapter->qospriv;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	sint	res=_SUCCESS;
 _func_enter_;   	

	_open_pktfile(pkt, &pktfile);
	
	i = _pktfile_read (&pktfile, (unsigned char*)&etherhdr, 14);
	pattrib->pktlen = pktfile.pkt_len;
	
	pattrib->ether_type = ntohs(etherhdr.h_proto);

	_memcpy(pattrib->dst, &etherhdr.h_dest, ETH_ALEN);
	_memcpy(pattrib->src, &etherhdr.h_source, ETH_ALEN);
	
	if ((check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE)) 
	{
		_memcpy(pattrib->ra, pattrib->dst, ETH_ALEN);
		_memcpy(pattrib->ta, pattrib->src, ETH_ALEN);

	}

	else if (check_fwstate(pmlmepriv, WIFI_STATION_STATE)) 
	{
		_memcpy(pattrib->ra, get_bssid(pmlmepriv), ETH_ALEN);
		_memcpy(pattrib->ta, pattrib->src, ETH_ALEN);
	}

	else if (check_fwstate(pmlmepriv, WIFI_AP_STATE)) 
	{
		_memcpy(pattrib->ra, pattrib->dst, ETH_ALEN);
		_memcpy(pattrib->ta, get_bssid(pmlmepriv), ETH_ALEN);
	}
	else if (check_fwstate(pmlmepriv, WIFI_MP_STATE))
	{
		 //firstly, filter packet not belongs to one mp sent
		 if(pattrib->ether_type != 0x8711)
	 	{
		 	res=_FAIL;
			DEBUG_ERR(("IN WIFI_MP_STATE but the ether_type !=0x8711!!!"));
			goto exit;
		 }

		 //for mp storing the txcmd per packet, 
		 //according to the info of txcmd to update pattrib
		 if (check_fwstate(pmlmepriv, WIFI_MP_LPBK_STATE) == _TRUE) 
			i = _pktfile_read (&pktfile, (u8*)&mp_txcmd, 16);//get 16 bytes txcmd per packet
			   
		 _memcpy(pattrib->ra, pattrib->dst, ETH_ALEN);
		 _memcpy(pattrib->ta, pattrib->src, ETH_ALEN);		 
		 
	}

	bmcast = IS_MCAST(pattrib->ra);

	// get sta_info
	if(bmcast)
	{
		psta = get_bcmc_stainfo(padapter);
		pattrib->mac_id = 4;
	}	
	else 
	{
		if (check_fwstate(pmlmepriv, WIFI_MP_STATE))
		{
#ifdef CONFIG_MP_INCLUDED		
			psta = get_stainfo(pstapriv, get_bssid(pmlmepriv));
			padapter->mppriv.tx_pktcount++;
			pattrib->mac_id = 5;
#endif
		}
		else
		{
			psta = get_stainfo(pstapriv, pattrib->ra);
			if(psta==NULL)
			{
				DEBUG_ERR(("\n update_attrib => get sta_info fail \n"));
				DEBUG_ERR(("\n ra:%x:%x:%x:%x:%x:%x\n", 
						pattrib->ra[0], pattrib->ra[1],
						pattrib->ra[2], pattrib->ra[3],
						pattrib->ra[4], pattrib->ra[5]));
				res = _FAIL;
				goto exit;
			}

			pattrib->mac_id = psta->mac_id;
		}
	}

	if (psta == NULL)	// if we cannot get psta => drrp the pkt
	{ 
		DEBUG_ERR(("\n update_attrib => get sta_info fail \n"));
		DEBUG_ERR(("\n ra:%x:%x:%x:%x:%x:%x\n", 
		pattrib->ra[0], pattrib->ra[1],
		pattrib->ra[2], pattrib->ra[3],
		pattrib->ra[4], pattrib->ra[5]));
		res =_FAIL;
		goto exit;
	}

	// get pktlen
	pattrib->ack_policy = NORMAL_ACK;
	pattrib->vcs_mode = pxmitpriv->vcs;

	if (pattrib->vcs_mode == RTS_CTS)
	{
		if(bmcast) 
			pattrib->vcs_mode  = NONE_VCS;
	}
	
	// get ether_hdr_len
	pattrib->pkt_hdrlen = 14 ;//(pattrib->ether_type == 0x8100) ? (14 + 4 ): 14; //vlan tag


#ifdef PLATFORM_OS_WINCE

	pqospriv->qos_option = 0;

#endif

	if(pqospriv->qos_option)
	{
		set_qos(&pktfile, pattrib);
	}
	else
	{
		pattrib->hdrlen = 24;
		pattrib->subtype = WIFI_DATA_TYPE;
		pattrib->priority = 2; //force to used BK queue, for debug.
	}

	
	if(psta->ieee8021x_blocked == _TRUE){
		DEBUG_INFO(("\n psta->ieee8021x_blocked == _TRUE \n"));
		pattrib->encrypt=0;
		if(pattrib->ether_type != 0x888e){
			DEBUG_INFO(("\n pattrib->ether_type =%x  \n",pattrib->ether_type ));
			res =_FAIL;
			goto exit;
		}
		DEBUG_INFO(("\n======= pattrib->ether_type =%x  \n",pattrib->ether_type ));
	}
	else
	{
		GET_ENCRY_ALGO(psecuritypriv, psta, pattrib->encrypt, bmcast);
	}
	
	switch(pattrib->encrypt)
	{
		case _WEP40_:
		case _WEP104_:
			pattrib->iv_len = 4;
			pattrib->icv_len = 4;
			break;

		case _TKIP_:
			pattrib->iv_len = 8;
			pattrib->icv_len = 4;

			break;
			
		case _AES_:
			pattrib->iv_len = 8;
			pattrib->icv_len = 8;
			break;
			
		default:
			pattrib->iv_len = 0;
			pattrib->icv_len = 0;
			break;

	}
#if 0
	if((pattrib->encrypt)&&(padapter->securitypriv.hw_decrypted==_FALSE))
	{
		pattrib->bswenc=_TRUE;
		DEBUG_ERR(("pattrib->encrypt=%d padapter->securitypriv.hw_decrypted=%d pattrib->bswenc=_TRUE\n"
			,pattrib->encrypt,padapter->securitypriv.hw_decrypted));
	}
	else
	{
		pattrib->bswenc=_FALSE;
		DEBUG_INFO(("pattrib->bswenc=_FALSE"));
	}
#endif

       //if in MP_STATE, update pkt_attrib from mp_txcmd, and overwrite some settings above.
       if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE) && 
	   	(check_fwstate(pmlmepriv, WIFI_MP_LPBK_STATE) == _TRUE) )
       {
		pattrib->vcs_mode = mp_txcmd.txcmd0.vcs;
		pattrib->priority = mp_txcmd.txcmd4.tid;            			

		if(mp_txcmd.txcmd4.non_qos)
		{
			pattrib->hdrlen = 24;
			pattrib->subtype = WIFI_DATA_TYPE;
		}
		else
		{
			pattrib->hdrlen = 26;
			pattrib->subtype = WIFI_QOS_DATA_TYPE;		
		}
       }

exit:
_func_exit_;
	return res;
}


sint xmitframe_addmic(_adapter *padapter, struct xmit_frame *pxmitframe){
	sint	curfragnum, length=0;
	u32	datalen;
	u8	*pframe, *payload,mic[8];
	struct mic_data		micdata;
	struct sta_info		*stainfo;
	struct qos_priv		*pqospriv= &padapter->qospriv;
	struct pkt_attrib		*pattrib = &pxmitframe->attrib;
	struct xmit_priv		*pxmitpriv = &padapter->xmitpriv;
	u8	priority[4] = {0x0,0x0,0x0,0x0};

	stainfo=get_stainfo(&padapter->stapriv ,&pattrib->ra[0] );

_func_enter_;

	if(pattrib->encrypt ==_TKIP_)//if(psecuritypriv->dot11PrivacyAlgrthm==_TKIP_PRIVACY_) 
	{
		//4 encode mic code
		if(stainfo!= NULL)
		{
			u8 null_key[16]={0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};
			datalen=pattrib->pktlen-pattrib->hdrlen;

			pframe = get_xmit_payload_start_addr(pxmitframe);

			if(_memcmp(&stainfo->dot11tkiptxmickey.skey[0],null_key, 16)==_TRUE)
			{
				msleep_os(10);
			}

			//4 start to calculate the mic code
			secmicsetkey(&micdata, &stainfo->dot11tkiptxmickey.skey[0]);
			if(pframe[1]&1)  //ToDS==1
			{  
				secmicappend(&micdata, &pframe[16], 6);  //DA
				if(pframe[1]&2)  //From Ds==1
					secmicappend(&micdata, &pframe[24], 6);
				else
					secmicappend(&micdata, &pframe[10], 6);		
			}	
			else  //ToDS==0
			{	
				secmicappend(&micdata, &pframe[4], 6);   //DA
				if(pframe[1]&2)  //From Ds==1
					secmicappend(&micdata, &pframe[16], 6);
				else
					secmicappend(&micdata, &pframe[10], 6);

			}

			if(pqospriv->qos_option==1)
				priority[0]=(u8)pxmitframe->attrib.priority;

			secmicappend(&micdata, &priority[0], 4);
	
			payload=pframe;

			for(curfragnum=0;curfragnum<pattrib->nr_frags;curfragnum++)
			{
				payload=(u8 *)RND4((uint)(payload));
				DEBUG_ERR(("===curfragnum=%d, pframe= 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x,!!!\n",
					curfragnum,*payload, *(payload+1),*(payload+2),*(payload+3),*(payload+4),*(payload+5),*(payload+6),*(payload+7)));

				payload=payload+pattrib->hdrlen+pattrib->iv_len;
				DEBUG_ERR(("curfragnum=%d pattrib->hdrlen=%d pattrib->iv_len=%d",curfragnum,pattrib->hdrlen,pattrib->iv_len));
				if((curfragnum+1)==pattrib->nr_frags)
				{
					length=pattrib->last_txcmdsz-pattrib->hdrlen-pattrib->iv_len-( (pattrib->bswenc) ? pattrib->icv_len : 0);
					secmicappend(&micdata, payload,length);
					payload=payload+length;
				}
				else
				{
					length=pxmitpriv->frag_len-pattrib->hdrlen-pattrib->iv_len-( (pattrib->bswenc) ? pattrib->icv_len : 0);
					secmicappend(&micdata, payload, length);
					payload=payload+length+pattrib->icv_len;
					DEBUG_ERR(("curfragnum=%d length=%d pattrib->icv_len=%d",curfragnum,length,pattrib->icv_len));
				}
			}
			secgetmic(&micdata,&(mic[0]));
			DEBUG_INFO(("xmitframe_addmic: before add mic code!!!\n"));
			DEBUG_INFO(("xmitframe_addmic: pattrib->last_txcmdsz=%d!!!\n",pattrib->last_txcmdsz));
			DEBUG_INFO(("xmitframe_addmic: mic[0]=%x,mic[1]=%x,mic[2]=%x,mic[3]=%x!!!\n",
				mic[0],mic[1],mic[2],mic[3]));
			//4 add mic code  and add the mic code length in last_txcmdsz
			_memcpy(payload, &(mic[0]),8);
			pattrib->last_txcmdsz+=8;
			DEBUG_INFO(("\n ========last pkt========\n"));
			payload=payload-pattrib->last_txcmdsz+8;
			for(curfragnum=0;curfragnum<pattrib->last_txcmdsz;curfragnum=curfragnum+8)
				DEBUG_INFO((" %.2x,  %.2x,  %.2x,  %.2x,  %.2x,  %.2x,  %.2x,  %.2x ",
					*(payload+curfragnum), *(payload+curfragnum+1), *(payload+curfragnum+2),*(payload+curfragnum+3),
					*(payload+curfragnum+4),*(payload+curfragnum+5),*(payload+curfragnum+6),*(payload+curfragnum+7)));

		}
		else
		{
			DEBUG_ERR(("xmitframe_addmic: get_stainfo==NULL!!!\n"));
		}
	}
_func_exit_;

	return _SUCCESS;
}

sint xmitframe_swencrypt(_adapter *padapter, struct xmit_frame *pxmitframe){

	struct pkt_attrib		*pattrib = &pxmitframe->attrib;
	struct security_priv	*psecuritypriv = &padapter->securitypriv;
	
_func_enter_;
	if((psecuritypriv->sw_encrypt)||(pattrib->bswenc))
	{
		switch(pattrib->encrypt)
		{
			case _WEP40_:
			case _WEP104_:
				wep_encrypt(padapter, (u8 *)pxmitframe);
				break;
			case _TKIP_:
				tkip_encrypt(padapter, (u8 *)pxmitframe);
				break;
			case _AES_:
				aes_encrypt(padapter, (u8 * )pxmitframe);
				break;
			default:
				break;
		}
	}
_func_exit_;

	return _SUCCESS;
}
	
/*

This sub-routine will perform all the following:

1. remove 802.3 header.
2. create wlan_header, based on the info in pxmitframe
3. append sta's iv/ext-iv
4. append LLC
5. move frag chunk from pframe to pxmitframe->mem
6. apply sw-encrypt, if necessary. 

*/
sint xmitframe_coalesce(_adapter *padapter, _pkt *pkt, struct xmit_frame *pxmitframe)
{
	sint frg_inx, frg_len, mpdu_len, llc_sz, mem_sz;
	uint addr;
	u8	*pframe, *mem_start;
	struct pkt_file		pktfile;
	struct sta_info		*psta;
	struct sta_priv		*pstapriv = &padapter->stapriv;
	struct xmit_priv		*pxmitpriv = &padapter->xmitpriv;	
	struct pkt_attrib		*pattrib = &pxmitframe->attrib;
	struct security_priv	*psecuritypriv = &padapter->securitypriv;
	struct mlme_priv		*pmlmepriv = &(padapter->mlmepriv);	
	sint bmcst = IS_MCAST(pattrib->ra);
	sint res=_SUCCESS;	
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
	u8	*txdesc_addr;
#endif

_func_enter_;
	
	if ((make_wlanhdr(padapter,(unsigned char *)pxmitframe->mem, pattrib)) == _FAIL)
	{
		DEBUG_ERR(("xmitframe_coalesce : make_wlanhdr fail ; drop pkt\n"));
		res=_FAIL;
		goto exit;
	}

	_open_pktfile (pkt, &pktfile);
	_pktfile_read (&pktfile, NULL, pattrib->pkt_hdrlen);

        if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE) && 
	   	(check_fwstate(pmlmepriv, WIFI_MP_LPBK_STATE) == _TRUE) )
		_pktfile_read (&pktfile, NULL, 16);//truncate 16 bytes txcmd if at mp mode

	mem_start = get_xmit_payload_start_addr(pxmitframe);

	frg_inx = 0; 
	frg_len = pxmitpriv->frag_len - 4;
	DEBUG_INFO(("\n xmitframe_coalesce:frg_len =%d(pattrib->pktlen=%d pxmitpriv->frag_len=%d)\n",frg_len ,pattrib->pktlen,pxmitpriv->frag_len));

	if(pxmitpriv->frag_len ==0)
	{
		res=_FAIL;
		DEBUG_ERR(("\n xmitframe_coalesce:pxmitpriv->frag_len ==0,reset frag_len=2346\n"));
		pxmitpriv->frag_len=2346;
		frg_len = pxmitpriv->frag_len - 4;
		//goto exit;
	}
	if (pattrib->bswenc == 0)
		frg_len -= pattrib->icv_len;

	
	while (1)
	{
		llc_sz = 0;
		
		mpdu_len = frg_len;
		
		pframe = mem_start;
		
		_memcpy(pframe, (u8 *)(pxmitframe->mem), pattrib->hdrlen);
		SetMFrag(mem_start);
		pframe += pattrib->hdrlen;

		mpdu_len -= pattrib->hdrlen;
		
		//adding icv, if necessary...

		if (pattrib->iv_len) 
		{
			if (check_fwstate(pmlmepriv, WIFI_MP_STATE))
				psta = get_stainfo(pstapriv, get_bssid(pmlmepriv));
			else			
				psta = get_stainfo(pstapriv, pattrib->ra);

			if(psta != NULL)
			{
				switch(pattrib->encrypt)
				{
					case _WEP40_:
					case _WEP104_:
						WEP_IV(pattrib->iv, psta->dot11txpn,(u8) psecuritypriv->dot11PrivacyKeyIndex);	
						break;
					case _TKIP_:			
						TKIP_IV(pattrib->iv, psta->dot11txpn);
						break;			
					case _AES_:
						AES_IV(pattrib->iv, psta->dot11txpn);
						break;
				}
			}
			_memcpy(pframe, pattrib->iv, pattrib->iv_len);
			pframe += pattrib->iv_len;
			mpdu_len -= pattrib->iv_len;
		}
		
		if (frg_inx == 0) 
		{
			llc_sz= rtl8711_put_snap(pframe, pattrib->ether_type);
			pframe += llc_sz;
			mpdu_len -= llc_sz;
		}

		if ((pattrib->icv_len >0)&& (pattrib->bswenc)) 
		{
			mpdu_len -= pattrib->icv_len;
		}

		if(bmcst) // if bc/mc: no fragmentation : yitsen
		{
			mem_sz = _pktfile_read (&pktfile, pframe, pattrib->pktlen);
		}
		else		
			mem_sz = _pktfile_read (&pktfile, pframe, mpdu_len);


		pframe += mem_sz;

		if ((pattrib->icv_len >0 )&& (pattrib->bswenc))
		{
			_memcpy(pframe, pattrib->icv, pattrib->icv_len); 
			pframe += pattrib->icv_len;		
		}
		
		frg_inx++;

		//if(bmcst|| (mem_sz <= mpdu_len)) //yitsen for bc/mc
		if (bmcst || (endofpktfile(&pktfile) == _TRUE))
		{
			pattrib->nr_frags = frg_inx;

			pattrib->last_txcmdsz =  pattrib->hdrlen + pattrib->iv_len +((pattrib->nr_frags==1)? llc_sz:0) + 
					( (pattrib->bswenc) ? pattrib->icv_len : 0) + mem_sz;
			DEBUG_INFO(("xmit_coalece: nr_frags: %d , last_txcmdsz: %d\n",pattrib->nr_frags, pattrib->last_txcmdsz));
			ClearMFrag(mem_start);
			break;
		}		
		
		addr = (uint)(pframe);
		
			
#ifdef CONFIG_RTL8711
		mem_start = (unsigned char *)RND4(addr);
#endif /*CONFIG_RTL8711*/
			
#ifdef CONFIG_RTL8187
		txdesc_addr = (u8 *)RND4(addr);
		mem_start = txdesc_addr + TXDESC_OFFSET;
#endif


#ifdef CONFIG_RTL8192U

		txdesc_addr = (u8 *)RND4(addr);
		mem_start = txdesc_addr + TXDESC_OFFSET+TX_FWINFO_OFFSET;
#endif

	}
	
	xmitframe_addmic(padapter, pxmitframe);
	xmitframe_swencrypt(padapter, pxmitframe);
exit:	

	
_func_exit_;	
	return res;

}

#if 0
static void build_txcmd(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, int entry, u8 * pscsi_buf_entry)
{
	sint i,t, sz=0;
	_list	*plist, *phead;
	sint bmcast;	

	uint	val;
	struct xmit_frame	*pxmitframe;
	struct pkt_attrib	*pattrib;
	
	_adapter *padapter =pxmitpriv->adapter;
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	struct qos_priv	*pqospriv= &padapter->qospriv;

	u8 *p = pscsi_buf_entry;
	p += 8; //the first 8 bytes is kept as empty slots becasue of the number of entries in the tx_queue(vo, vi, be, bk, bm)

	for (i = 0; i < entry; i++, phwxmit++)
	{

		//for usb interface, write txcmd before bulk out tx packets
		phead = &phwxmit->pending;

		plist = get_next(phead);

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			pattrib = &pxmitframe->attrib;

			for(t = 0; t <pattrib->nr_frags; t ++)	
			{

			    	//VCS update
				//If longer than RTS threshold
				if(sz + 4 >= pxmitpriv->rts_thresh && pxmitpriv->vcs_setting == AUTO_VCS)
					pattrib->vcs_mode = pxmitpriv->vcs_type;
				else
					pattrib->vcs_mode  = pxmitpriv->vcs;

				bmcast = IS_MCAST(pattrib->ra);
				if(bmcast) 
					pattrib->vcs_mode  = NONE_VCS;

				if (t == (pattrib->nr_frags- 1))
				{
					val = BIT(31)| BIT(29) ;
					sz = pattrib->last_txcmdsz;
				}
				else
				{
					val = BIT(31)|BIT(30);
					sz = pxmitpriv->frag_len;
					sz = sz -4 - (psecuritypriv->sw_encrypt ? 0 : pattrib->icv_len);
				}

				if(pqospriv->qos_option)
				{
					val |= (( sz << 16)  | ( pattrib->mac_id << 10) | ( pattrib->priority << 5) );
				}
				else{
					val |= (( sz << 16)  | ( pattrib->mac_id << 10) | BIT(9));	
				}

				if(pattrib->bswenc==_TRUE){
					DEBUG_ERR(("\n dump_xmitframes:pattrib->bswenc=%d\n",pattrib->bswenc));
					val=val |BIT(27);
				}
				
				//endian issue is handled by FW 
				//val = cpu_to_be32(val);
				_memcpy((void *)p, &val, 4);
				p += 4;
			}
			
			plist = get_next(plist);
			
		}
	}
}

static void build_write_txpayload(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, int entry)
{
	sint i,t, sz=0;
	_list	*plist, *phead;
	u8	*mem_addr;

	struct xmit_frame	*pxmitframe;
	struct pkt_attrib	*pattrib;
	
	_adapter *padapter =pxmitpriv->adapter;
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	//struct qos_priv	*pqospriv= &padapter->qospriv;

	for (i = 0; i < entry; i++, phwxmit++)
	{
              //now... let's trace the list again, and bulk out  tx frames....
		phead = &phwxmit->pending;
		plist = get_next(phead);	

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			pattrib = &pxmitframe->attrib;
			pxmitframe->fragcnt = pattrib->nr_frags;	
			pxmitframe->irpcnt= pattrib->nr_frags;
			plist = get_next(plist);//get next plist before dequeue xmitframe 		
			list_delete(&pxmitframe->list);
			
			mem_addr = ((unsigned char *)pxmitframe->mem) + WLANHDR_OFFSET;

			for(t = 0; t < pattrib->nr_frags; t ++)
			{
				
				if (t != (pattrib->nr_frags - 1))
				{
					sz = pxmitpriv->frag_len;
					sz = sz -4 - (psecuritypriv->sw_encrypt ? 0 : pattrib->icv_len);					
					pxmitframe->last[t] = 0;				
				}
				else
				{     
					sz = pattrib->last_txcmdsz;				
					pxmitframe->last[t] = 1;					
				}
			
			       pxmitframe->mem_addr = mem_addr;

                            write_port(padapter, i, sz, (unsigned char*)pxmitframe);
				DEBUG_INFO(("++++++++++dump_xmitframes: xmit ff sz:%d\n", sz));

				mem_addr += sz;
			
				mem_addr = (u8 *)RND4(((uint)(mem_addr)));

			}					 
		}
	}
}
#endif

__inline u8 rtl8180_IsWirelessBMode(u16 rate)
{
	if( ((rate <= 110) && (rate != 60) && (rate != 90)) || (rate == 220) )
		return 1;
	else return 0;
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
	} 
	else 
	{	//802.11g DSSS-OFDM PLCP length field calculation.
		N_DBPS = N_DBPSOfRate(DataRate);
		Ceiling = (16 + 8*FrameLength + 6) / N_DBPS 
				+ (((16 + 8*FrameLength + 6) % N_DBPS) ? 1 : 0);
		FrameTime = (u16)(16 + 4 + 4*Ceiling + 6);
	}
	return FrameTime;
}


void update_protection(_adapter *padapter, u8 *ie, uint ie_len)
{

	uint	protection;
	u8	*perp;
	sint	 erp_len;
	struct	xmit_priv *pxmitpriv = &padapter->xmitpriv;
	struct	registry_priv *pregistrypriv = &padapter->registrypriv;
	
_func_enter_;
	
	
	switch(pxmitpriv->vcs_setting)
	{
		case DISABLE_VCS:
			pxmitpriv->vcs = NONE_VCS;
			break;
	
		case ENABLE_VCS:
			break;
	
		case AUTO_VCS:
		default:
			perp = get_ie(ie, _ERPINFO_IE_, &erp_len, ie_len);
			if(perp == NULL)
			{
			pxmitpriv->vcs = NONE_VCS;
	}
			else
			{
		protection = (*(perp + 2)) & BIT(1);
		if (protection)
				{
					if(pregistrypriv->vcs_type == RTS_CTS)
			pxmitpriv->vcs = RTS_CTS;
		else
						pxmitpriv->vcs = CTS_TO_SELF;
				}
				else
				pxmitpriv->vcs = NONE_VCS;
		}
			break;			
	
	}

_func_exit_;

}





/*
Calling context:
1. OS_TXENTRY
2. RXENTRY (rx_thread or RX_ISR/RX_CallBack)

If we turn on USE_RXTHREAD, then, no need for critical section.
Otherwise, we must use _enter/_exit critical to protect free_xmit_queue...

Must be very very cautious...

*/

struct xmit_frame *alloc_xmitframe(struct xmit_priv *pxmitpriv)//(_queue *pfree_xmit_queue)
{
	/*
		Please remember to use all the osdep_service api,
		and lock/unlock or _enter/_exit critical to protect 
		pfree_xmit_queue
	*/	
	
	_irqL irqL;
	struct xmit_frame *	pxframe=  NULL;
	_list	*plist, *phead;
	_queue *pfree_xmit_queue = &pxmitpriv->free_xmit_queue;
_func_enter_;	
	_enter_critical(&pfree_xmit_queue->lock, &irqL);

	if(_queue_empty(pfree_xmit_queue) == _TRUE)
	{
		DEBUG_ERR(("free_xmitframe_cnt:%d\n", pxmitpriv->free_xmitframe_cnt));
		pxframe =  NULL;
	
	}
	else
	{

		phead = get_list_head(pfree_xmit_queue);
		
		plist = get_next(phead);
		
		pxframe = LIST_CONTAINOR(plist, struct xmit_frame, list);

		list_delete(&(pxframe->list));

		memset( (u8*)pxframe->mem, 0, 96);

	}
	
	if(pxframe !=  NULL) 
		pxmitpriv->free_xmitframe_cnt--;

	_exit_critical(&pfree_xmit_queue->lock, &irqL);
_func_exit_;	
	return pxframe;

}

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
struct mgnt_frame *alloc_mgnt_xmitframe(struct xmit_priv *pxmitpriv)//(_queue *pfree_xmit_queue)
{			
		_irqL irqL;
		struct mgnt_frame * pxframe=  NULL;
		_list	*plist, *phead;
		_queue *pfree_mgnt_queue = &pxmitpriv->free_mgnt_queue;

		
	_func_enter_;	
		_enter_critical(&pfree_mgnt_queue->lock, &irqL);
	
		if(_queue_empty(pfree_mgnt_queue) == _TRUE)
		{
			DEBUG_ERR(("free_mgnt_frame_cnt:%d\n", pxmitpriv->free_mgnt_frame_cnt));
			pxframe =  NULL;
		}
		else
		{
	
			phead = get_list_head(pfree_mgnt_queue);
			
			plist = get_next(phead);
			
			pxframe = LIST_CONTAINOR(plist, struct mgnt_frame, list);

			memset((u8*)pxframe->mem, 0, MAX_MGNTBUF_SZ);
			
			list_delete(&(pxframe->list));
		}
		
		if(pxframe !=  NULL) 
			pxmitpriv->free_mgnt_frame_cnt--;
	
		_exit_critical(&pfree_mgnt_queue->lock, &irqL);
		if((pxmitpriv->free_mgnt_frame_cnt< 7) && (pxmitpriv->free_mgnt_frame_cnt != 0))
		{
			DEBUG_INFO(("alloc mgntframe : cnt : %d\n",  pxmitpriv->free_mgnt_frame_cnt));
		}
	_func_exit_;	
		return pxframe;

}


sint free_mgntframe(struct xmit_priv *pxmitpriv, struct mgnt_frame *pmgntframe)//(struct xmit_frame *pxmitframe, _queue *pfree_xmit_queue)
{

	_irqL irqL;
	_queue *	pfree_mgnt_queue = &pxmitpriv->free_mgnt_queue;
	_pkt*		pndis_pkt = NULL;

_func_enter_;	
	if(pmgntframe==NULL)
	{
		DEBUG_ERR(("free_mgntframe: pmgntframe == NULL\n"));
		goto exit;
	}
	_enter_critical(&pfree_mgnt_queue->lock, &irqL);
	
	list_delete(&pmgntframe->list);

	if (pmgntframe->pkt) 
	{
		pndis_pkt  = pmgntframe->pkt;              
		pmgntframe->pkt = NULL;
	}	
	
	list_insert_tail(&(pmgntframe->list), get_list_head(pfree_mgnt_queue));

	pxmitpriv->free_mgnt_frame_cnt++;
	
	_exit_critical(&pfree_mgnt_queue->lock, &irqL);	

	check_free_tx_packet(pxmitpriv,(struct xmit_frame *) pmgntframe);
exit:	   
_func_exit_;	 
	return _SUCCESS;
} 

#endif /*CONFIG_RTL8187 CONFIG_RTL8192U*/

#ifdef CONFIG_RTL8192U

extern void TsInitAddBA(PADAPTER Adapter);

VOID
TsStartAddBaProcess(
	PADAPTER		Adapter,
	struct xmit_frame *pxmitframe
	)
{
	PTX_TS_RECORD	pTxTS = &(Adapter->xmitpriv.TxTsRecord);
	if(pTxTS->bAddBaReqInProgress == _FALSE)
	{
		pTxTS->bAddBaReqInProgress = _TRUE;

		if(pTxTS->bAddBaReqDelayed)
		{
			_set_timer(&pTxTS->TsAddBaTimer, 60000); //TS_ADDBA_DELAY = 60000
		}
		else
		{
//			_set_timer(&pTxTS->TsAddBaTimer, 0);
			TsInitAddBA(Adapter);
		}
	}
}

#define SN_LESS(a, b)		((( (a) - (b) )&0x800)!=0)


/**
* Function:	MgntQuery_AggregationCapability
* 
* Overview:	Query whether the packet is allowed to be AMPDU aggregated.
*			Query the Aggregation Capability. Including MPDUFactor and AMPDUDensity.
* 
* Input:		
* 		PADAPTER	Adapter
*		pu1Byte		dstaddr
*		PRT_TCB		pTcb
* 			
* Output:		
*		PRT_TCB		pTcb
* Return:     	
*		None
*/
VOID
MgntQuery_AggregationCapability(
	_adapter	*padapter,
	struct xmit_frame *pxmitframe
	)
{
	struct pkt_attrib *pattrib = &(pxmitframe->attrib);
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	PTX_TS_RECORD pTxTs = &(padapter->xmitpriv.TxTsRecord);
		
	if(pattrib->subtype != WIFI_QOS_DATA_TYPE)
		return;

	if(IS_MCAST(pattrib->ra))
		return;

	// Note : pHTInfo->bCurrentAMPDUEnable should be enabled only when AMPDU is enabled 
	// in registry and current operation mode is HT. 
	if(pHTInfo->bCurrentAMPDUEnable)
	{
		//
		// This part shall be integrate to other function.
		// 
		if(pTxTs->TxAdmittedBARecord->bValid == _FALSE) 
		{
			//
			// Caution: Now we just establish ADDBA protocol and use A-MPDU for 
			// every transmission.
			//
			//if(pMgntInfo->SecurityInfo.SecLvl > RT_SEC_LVL_NONE && !SecIsTxKeyInstalled(Adapter, pMgntInfo->Bssid))
			if(padapter->securitypriv.dot11PrivacyAlgrthm > _NO_PRIVACY_)
			{
				DEBUG_INFO(("MgntQuery_AggregationCapability: no BA\n"));
			}
			else
			{
				DEBUG_INFO(("MgntQuery_AggregationCapability: TsStartAddBaProcess\n"));
				
				TsStartAddBaProcess(padapter, pxmitframe);
			}
			goto FORCED_AGG_SETTING;
		}

		else if(pTxTs->bUsingBa == _FALSE)
		{
			if(SN_LESS(pTxTs->TxAdmittedBARecord->BaStartSeqCtrl.Field.SeqNum, ((padapter->ieee80211_seq_ctl+1)&(4095)) ))
				pTxTs->bUsingBa = _TRUE;
			else
				goto FORCED_AGG_SETTING;
		}
#if 0
		//2//
		//2 //(1)Decide whether aggregation is required according to protocol handshake
		//2//
		
		if(pMgntInfo->mActingAsAp || pMgntInfo->mIbss)
		{
			PRT_WLAN_STA pEntry = AsocEntry_GetEntry(&Adapter->MgntInfo, dstaddr);
			if(pEntry && pEntry->HTInfo.bEnableHT)
			{
				pTcb->bAMPDUEnable = TRUE;
				pTcb->AMPDUFactor = pEntry->HTInfo.AMPDU_Factor;
				pTcb->AMPDUDensity = pEntry->HTInfo.MPDU_Density;
			}
			else
			{
				pTcb->bAMPDUEnable = FALSE;
			}
		}
		else
		{
			pTcb->bAMPDUEnable = TRUE;
			pTcb->AMPDUFactor = pHTInfo->CurrentAMPDUFactor;
			pTcb->AMPDUDensity = pHTInfo->CurrentMPDUDensity;
		}
#endif
		
	}
#if 0
	else if(pHTInfo->ForcedAMSDUMode == HT_AGG_FORCE_ENABLE)
	{
	
		// If driver does not use AMPDU by default but use A-MSDU instead, and packet 
		// size is small. We do this because we hope TCP ack packet can be aggregated
		if(pTcb->bAggregate == FALSE && pTcb->PacketLength <= 200 )	
		{			
			pTcb->bAMPDUEnable = TRUE;
			pTcb->AMPDUFactor = pHTInfo->CurrentAMPDUFactor;
			pTcb->AMPDUDensity = pHTInfo->CurrentMPDUDensity;
		}
	}
#endif
	
FORCED_AGG_SETTING:
	return;
#if 0	
	//2//
	//2 //(2) The OID control may overwrite protocol handshake 
	//2//
	switch(pHTInfo->ForcedAMPDUMode )
	{
		case HT_AGG_AUTO:
				// Do Nothing
				break;
				
		case HT_AGG_FORCE_ENABLE:
				pTcb->bAMPDUEnable = TRUE;
				pTcb->AMPDUDensity = pHTInfo->ForcedMPDUDensity;
				pTcb->AMPDUFactor = pHTInfo->ForcedAMPDUFactor;
				break;
				
		case HT_AGG_FORCE_DISABLE:
				pTcb->bAMPDUEnable = FALSE;
				pTcb->AMPDUDensity = 0;
				pTcb->AMPDUFactor = 0;
				break;
			
	}	
#endif	

}




VOID
MgntQuery_ProtectionFrame(struct xmit_frame *pxmitframe)
{

	u8	bmcst,bfrag,frametype;
	u8 *ptr = NULL;
	u32 index = 0 ,alignment = 0, fraglength = 0;
	struct pkt_attrib	*pattrib = &pxmitframe->attrib;	
	_adapter *padapter = pxmitframe->padapter;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);

#ifdef PLATFORM_DVR
	alignment = ALIGNMENT -( (u32)((u8 *)pxmitframe->mem+WLANHDR_OFFSET ) & (ALIGNMENT-1) );
#endif

	ptr = (u8 *)((u8 *)pxmitframe->mem+WLANHDR_OFFSET + TXDESC_OFFSET+TX_FWINFO_OFFSET+ alignment);
	DEBUG_INFO(("Protection: alignemnt :% d, addr :%p \n",alignment, ptr));
	
	bfrag = GetMFrag(ptr)  ;
	bmcst = IS_MCAST(pattrib->ra);
	frametype = GetFrameType(ptr);

	fraglength = (bfrag == 0 )? pattrib->last_txcmdsz:( pxmitpriv->frag_len-(pattrib->bswenc ? 0 : pattrib->icv_len));
		

	// Common Settings
	pattrib->bRTSSTBC		= _FALSE;
	pattrib->bRTSUseShortGI	= _FALSE; // Since protection frames are always sent by legacy rate, ShortGI will never be used.
	pattrib->bCTSEnable		= _FALSE; // Most of protection using RTS/CTS
	pattrib->RTSSC			= 0;		// 20MHz: Don't care;  40MHz: Duplicate.
	pattrib->bRTSBW			= _FALSE; // RTS frame bandwidth is always 20MHz

	// Check Control type frame: Control frame do not need protection!!

	index = GetFrameSubType(ptr) >> 4;
	
	if(index == WIFI_BEACON)
		goto NO_PROTECTION; 


	goto NO_PROTECTION;

	if( frametype!= WIFI_CTRL_TYPE )
	{
		// Check multicast/broadcast: Protection doesn't apply to broadcast and multicast frame!!
		if(bmcst)
		{
			pattrib->bNeedAckReply	= _FALSE;
			pattrib->bNeedCRCAppend	= _TRUE;
			goto NO_PROTECTION;
		}
		
		// Unicast packet case
		else
		{
			pattrib->bNeedAckReply	= _TRUE;
			pattrib->bNeedCRCAppend	= _TRUE;

	//		if(MacAddr_isBcst((pFrame+16)))
	//			goto NO_PROTECTION;

			#if 0
			// Forced Protection mode!!
			if(Adapter->MgntInfo.bForcedProtection)
			{
				pattrib->bRTSEnable 	= TRUE;
				pattrib->bCTSEnable 	= Adapter->MgntInfo.bForcedProtectCTSFrame;
				pattrib->bRTSBW		= Adapter->MgntInfo.bForcedProtectBW40MHz;
				pattrib->RTSSC		= Adapter->MgntInfo.ForcedProtectSubCarrier;
				pattrib->RTSRate		= Adapter->MgntInfo.ForcedProtectRate;
				return;
			}
			#endif
			// Legacy ABG protection case.
			if(_sys_mib->network_type < WIRELESS_11N_2G4)
			{	
				//
				// (1) RTS_Threshold is compared to the MPDU, not MSDU.
				// (2) If there are more than one frag in  this MSDU, only the first frag uses protection frame.
				//		Ohter fragments are protected by previous fragment.
				//		So we only need to check the length of first fragment.
				//
				if(fraglength>= pxmitpriv->rts_thresh && pxmitpriv->vcs_setting == AUTO_VCS)
				{
	
					pattrib->RTSRate	= (pxmitpriv->ieee_basic_rate*2)/10;//	= ComputeAckRate( 
										//	Adapter->MgntInfo.mBrates, 
										//	Adapter->MgntInfo.HighestBasicRate );
					pattrib->bRTSEnable = _TRUE;
				}

				
				else if( _sys_mib->dot11ErpInfo.protection  ) // We should not send CTS-to-Self for CCK packets. 2005.01.25, by rcnjko.
				{

					
					// Use CTS-to-SELF in protection mode.
//					if(ProtectionImpMode == 1)
					{
						pattrib->bCTSEnable	= _TRUE;
						pattrib->RTSRate 		= MGN_11M ;
						pattrib->bRTSEnable = _TRUE;
					}
					
					// Use RTS/CTS in protection mode, note that, RTS rate shall be CCK rate.
//					else
					#if 0
					{
						pattrib->RTSRate = MGN_11M; // Rate is 11Mbps.
						pattrib->bRTSEnable = _TRUE;
					}
					#endif
				}
				
				else
				{
					goto NO_PROTECTION;
				}
			}


			#if 1
			// 11n High throughput case.
			else
			{	
				PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
				BOOLEAN bIsPeerDynamicMimoPs	= _FALSE;

				while(_TRUE)
				{
					//3================================================
					 //3 Check ERP Use_Protection
					//3================================================
					if( _sys_mib->dot11ErpInfo.protection  )
					 {
//						if(ProtectionImpMode == 1)
						{
							pattrib->bCTSEnable = _TRUE;
							pattrib->RTSRate = MGN_11M;
							pattrib->bRTSEnable = _TRUE;
							break;
						}
//						else
						#if 0
						{
						
							pattrib->RTSRate = MGN_11M; // Rate is 11Mbps.
					 		pattrib->bRTSEnable = _TRUE;
							break;
						 }
						#endif
					}


					//3================================================
					//3Check HTInfo Operation Mode
					//3================================================
					if(pHTInfo->bCurrentHTSupport)
					{
						u1Byte HTOpMode = pHTInfo->CurrentOpMode;
	
						if((pHTInfo->bCurBW40MHz && (HTOpMode == 2 || HTOpMode == 3)) ||
							(!pHTInfo->bCurBW40MHz && HTOpMode == 3) )
						{
							pattrib->RTSRate = MGN_24M; // Rate is 24Mbps.
							pattrib->bRTSEnable = _TRUE;
							break;
						}
					}


					//3================================================
					 //3 Check RTS Threshold
					//3================================================
					if(fraglength>= pxmitpriv->rts_thresh && pxmitpriv->vcs_setting == AUTO_VCS)
					{
						pattrib->RTSRate = MGN_24M; // Rate is 24Mbps.
						pattrib->bRTSEnable = _TRUE;
						break;
					}


					//3================================================
					//3 Check Dynamic MIMO Power save condition
					//3================================================
					
					{
						if(pHTInfo->PeerMimoPs == MIMO_PS_DYNAMIC)
							bIsPeerDynamicMimoPs = _TRUE;
					}

					if(bIsPeerDynamicMimoPs)
					{
						pattrib->RTSRate = MGN_24M; // Rate is 24Mbps.
						pattrib->bRTSEnable = _TRUE;
						break;
					}


					//3================================================
					//3 Check A-MPDU aggregation for TXOP
					//3================================================
					#if 1
//					if(pattrib->bAMPDUEnable)
					{
						pattrib->RTSRate = MGN_24M; // Rate is 24Mbps.
						// According to 8190 design, firmware sends EF-End only if RTS/CTS is enabled. However, it degrads
						// throughput around 10M, so we disable of this mechanism. 2007.08.03 by Emily
						pattrib->bRTSEnable = _FALSE;
						break;
					}
					#endif
					// Totally no protection case!!
					goto NO_PROTECTION;
				}
			}
		#endif 	
		}
	}// End if( !IsFrameTypeCtrl(pFrame)  )	
	else
	{
		pattrib->bNeedAckReply		= _FALSE;
		pattrib->bNeedCRCAppend	= _FALSE;
		goto NO_PROTECTION;
	}
	
	
	// Determine RTS frame preamble mode.
	if(pattrib->RTSRate <= 2)
		pattrib->bRTSUseShortPreamble = _FALSE;
	else		
		pattrib->bRTSUseShortPreamble = _sys_mib->rf_ctrl.shortpreamble | !_sys_mib->dot11ErpInfo.longPreambleStaNum;

	return;

		NO_PROTECTION:
			pattrib->bRTSEnable	= _FALSE;
			pattrib->bCTSEnable	= _FALSE;
			pattrib->RTSRate		= 0;
			pattrib->RTSSC		= 0;
			pattrib->bRTSBW		= _FALSE;
}








void TxRateSelectMode(struct xmit_frame *pxmitframe)
{
	_adapter *padapter = pxmitframe->padapter;
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	struct pkt_attrib	*pattrib = &pxmitframe->attrib;
	u8 WirelessMode  = _sys_mib->network_type;
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;

	pattrib->bTxDisableRateFallBack = _TRUE;
	pattrib->bTxUseDriverAssingedRate = _TRUE;


	if (pxmitframe->frame_tag==TAG_MGNTFRAME)
	{
		pattrib->bTxDisableRateFallBack = _TRUE;
		pattrib->bTxUseDriverAssingedRate = _TRUE;
		pattrib->RATRIndex = 7;
		return;
	}

	if(!pattrib->bTxDisableRateFallBack || !pattrib->bTxUseDriverAssingedRate)
	{
		if(WirelessMode == WIRELESS_11A)
		{
			pattrib->RATRIndex = RATR_INDEX_A;
		}
		else if ((WirelessMode ==WIRELESS_11G) || (WirelessMode == WIRELESS_11BG ) )
		{
			pattrib->RATRIndex = RATR_INDEX_G;
		}
		else if(WirelessMode == WIRELESS_11B)
		{
			pattrib->RATRIndex = RATR_INDEX_B;
		}
		// TODO:  802.11 n mode
		#if 1
		else if(WirelessMode == WIRELESS_11N_2G4 || WirelessMode == WIRELESS_11N_5G)
		{
			if(pHTInfo->PeerMimoPs==MIMO_PS_STATIC)
			{
				pattrib->RATRIndex = RATR_INDEX_N_NO_2SS;
				DEBUG_INFO(("pHTInfo->PeerMimoPs = MIMO_PS_STATIC\n"));
			}
			else
			{
				pattrib->RATRIndex = RATR_INDEX_N_RATE;
				DEBUG_INFO(("pHTInfo->PeerMimoPs is not  MIMO_PS_STATIC\n"));
			}
		}
		else
		{
				pattrib->RATRIndex = RATR_INDEX_N_RATE;
		}
		#endif
	}

}

#if 0


u8 AMSDU_GetAggregatibleList(
	_adapter *padapter,
	struct xmit_frame	*pxmitframe,
	_list		pSendList,
	struct hw_xmit *phwxmit
	)
{
	
	u16		nMaxAMSDUSize;
	u16		AggrSize;
	u8		UsedDescNum;
	u8		nAggrNum = 0;
	_list *phead, *plist, *pAggrframe;
	struct xmit_frame	*pTmpframe = NULL;
	struct pkt_attrib *pTmpAttrib = NULL;
	struct pkt_attrib	*pattrib = &pxmitframe->attrib;
	
	struct mib_info *_sys_mib = &(padapter->_sys_mib);
	
	
	list_insert_tail(pSendList, &pxmitframe->list);
	nAggrNum++;

	//
	// Initialize local variables
	//
	AggrSize 		= (u16)pattrib->last_txcmdsz;
//	UsedDescNum	= (u1Byte)pTcb->BufferCount;
	phead = &phwxmit->pending;
	plist = get_next(phead);	
 

	//
	// Get A-MSDU aggregation threshold.
	//
	
	
		// TODO: IBSS Aggregation threshold needs to be considered.
		nMaxAMSDUSize = HAL_GET_MAXIMUN_AMSDU_SIZE(pMgntInfo->pHTInfo->nCurrent_AMSDU_MaxSize);
	
#if 0
	if(pMgntInfo->pHTInfo->ForcedAMSDUMode==HT_AGG_FORCE_ENABLE)
	{
		nMaxAMSDUSize = HAL_GET_MAXIMUN_AMSDU_SIZE(pMgntInfo->pHTInfo->ForcedAMSDUMaxSize);
	}
#endif

	//
	// Get Aggregation List. Only those frames with the same RA can be aggregated.
	// For Infrastructure mode, RA is always the AP MAC address so the frame can just be aggregated 
	// without checking RA.
	// For AP mode and IBSS mode, RA is the same as DA. Checking RA is needed before aggregation.
	//
	while (end_of_queue_search(phead, plist) == _FALSE)
	{

		pTmpframe = LIST_CONTAINOR(plist, struct xmit_frame, list);	
		pTmpAttrib =  &pTmpframe->attrib;
		plist = get_next(phead);	
 
		if (_sys_mib->mac_ctrl.opmode & WIFI_ADHOC_STATE)
		{
			if(_memcmp(pattrib->dst, pTmpAttrib->dst, 6) != 0)
			{
				
				continue;
			}
		}

		//
		// Three limitation shall be checked:
		// (1) A-MSDU size limitation
		// (2) TxDesc number
		// (3) Tcb buffer count.
		//
		


		/* reserve nAggrTcbNum for packet padding, david woo 2007/9/30 */
		if(pattrib->last_txcmdsz+ AggrSize < nMaxAMSDUSize) 
			{
				//1 Do aggregation
//				UsedDescNum += (u1Byte)pTmpTcb->BufferCount;
				AggrSize += (u16)pattrib->last_txcmdsz;
				
				pAggrframe = plist;
			
				list_delete(pAggrframe);
				list_insert_tail(pSendList, pAggrframe);
				nAggrNum++;
			}
			else
			{
				//1 Do not do aggregation because out of resources
				DEBUG_ERR(( ("Stop aggregation: "));
				if(!(pattrib->last_txcmdsz + AggrSize < nMaxAMSDUSize))
					DEBUG_ERR("[A-MSDU size limitation]"));

				break;
			}
		}
	return nAggrNum;
}






#endif


#endif /*CONFIG_RTL8192U*/










sint make_wlanhdr (_adapter *padapter , u8 *hdr, struct pkt_attrib *pattrib)
{


	u16   *qc;
	
	struct ieee80211_hdr *pwlanhdr = (struct ieee80211_hdr *)hdr;
	
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
#ifdef CONFIG_PWRCTRL	
	struct pwrctrl_priv *pwrpriv = &(padapter->pwrctrlpriv);
#endif
	sint	res=_SUCCESS;
	u16  *fctrl = &(pwlanhdr->frame_ctl);
_func_enter_;
	memset(hdr, 0, WLANHDR_OFFSET);

	SetFrameSubType(&(pwlanhdr->frame_ctl), pattrib->subtype);

	if (pattrib->subtype & WIFI_DATA_TYPE)
	{
		if ((check_fwstate(pmlmepriv,  WIFI_STATION_STATE) == _TRUE)) {
			//to_ds = 1, fr_ds = 0;
			SetToDs(fctrl);
			_memcpy(pwlanhdr->addr1, get_bssid(pmlmepriv), ETH_ALEN);
			_memcpy(pwlanhdr->addr2, pattrib->src, ETH_ALEN);
			_memcpy(pwlanhdr->addr3, pattrib->dst, ETH_ALEN);

		}else if ((check_fwstate(pmlmepriv,  WIFI_AP_STATE) == _TRUE) ) {
			//to_ds = 0, fr_ds = 1;
			SetFrDs(fctrl);
			_memcpy(pwlanhdr->addr1, pattrib->dst, ETH_ALEN);
			_memcpy(pwlanhdr->addr2, get_bssid(pmlmepriv), ETH_ALEN);
			_memcpy(pwlanhdr->addr3, pattrib->src, ETH_ALEN);
			
		}
		else if ((check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE)) {

			_memcpy(pwlanhdr->addr1, pattrib->dst, ETH_ALEN);
			_memcpy(pwlanhdr->addr2, pattrib->src, ETH_ALEN);
			_memcpy(pwlanhdr->addr3, get_bssid(pmlmepriv), ETH_ALEN);

		}
		else if ((check_fwstate(pmlmepriv,  WIFI_MP_STATE) == _TRUE)  ) {
			_memcpy(pwlanhdr->addr1, pattrib->dst, ETH_ALEN);
			_memcpy(pwlanhdr->addr2, pattrib->src, ETH_ALEN);
			_memcpy(pwlanhdr->addr3, get_bssid(pmlmepriv), ETH_ALEN);

		}
		
		else {
			DEBUG_ERR(("fw_state:%x is not allowed to xmit frame\n", get_fwstate(pmlmepriv)));
			res= _FAIL;
			goto exit;
	
		}
	
#ifdef CONFIG_PWRCTRL
		/* Perry:
			If driver is in busy, don't need to set power bit.
		*/
		if (pwrpriv->cpwm >= FW_PWR1 && !(padapter->mlmepriv.sitesurveyctrl.traffic_busy))
			SetPwrMgt(fctrl);
#else
		if ((get_fwstate(pmlmepriv)) & WIFI_SLEEP_STATE)
			SetPwrMgt(fctrl);
#endif		
	
		if (pattrib->encrypt)
			SetPrivacy(fctrl);

		qc = (unsigned short *)(hdr + pattrib->hdrlen - 2);


		if (pattrib->priority)
			SetPriority(qc, pattrib->priority);

		SetAckpolicy(qc, pattrib->ack_policy);
			
	}
	else {

	}
	
exit:		
_func_exit_;

	return res;
}



sint rtl8711_put_snap(u8 *data, u16 h_proto)
{
	struct ieee80211_snap_hdr *snap;
	u8 *oui;
_func_enter_;
	snap = (struct ieee80211_snap_hdr *)data;
	snap->dsap = 0xaa;
	snap->ssap = 0xaa;
	snap->ctrl = 0x03;

	if (h_proto == 0x8137 || h_proto == 0x80f3)
		oui = P802_1H_OUI;
	else
		oui = RFC1042_OUI;
	
	snap->oui[0] = oui[0];
	snap->oui[1] = oui[1];
	snap->oui[2] = oui[2];

	*(u16 *)(data + SNAP_SIZE) = htons(h_proto);
_func_exit_;
	return SNAP_SIZE + sizeof(u16);
}


/*

Calling context:
A. For SDIO with Synchronous IRP:
	xmit_thread  	(passive level)
B. For SDIO with Async. IRP:
	xmit_callback 	(dispatch level)

C. For USB (must always be Async. IRP)
	xmit_callback
	
D. For CF 
	xmit_callback
	
If don't take alloc_txobj into consideration, only lock/unlock are enough.

Therefore, must use _enter/_exit critical to protect the pfree_xmit_queue

Must be very cautious...

*/
sint	free_xmitframe(struct xmit_priv *pxmitpriv, struct xmit_frame *pxmitframe)//(struct xmit_frame *pxmitframe, _queue *pfree_xmit_queue)
{
	
	_irqL irqL;
	_queue *pfree_xmit_queue = &pxmitpriv->free_xmit_queue;
	_pkt		*pndis_pkt = NULL;

_func_enter_;	
	if(pxmitframe==NULL){
		goto exit;
	}
	_enter_critical(&pfree_xmit_queue->lock, &irqL);
	
	list_delete(&pxmitframe->list);

	if (pxmitframe->pkt) 
	{
        	pndis_pkt  = pxmitframe->pkt;              
		pxmitframe->pkt = NULL;
	}	
	
	list_insert_tail(&(pxmitframe->list), get_list_head(pfree_xmit_queue));

	pxmitpriv->free_xmitframe_cnt++;
	
	_exit_critical(&pfree_xmit_queue->lock, &irqL);	

	check_free_tx_packet(pxmitpriv, pxmitframe);

exit:	   
_func_exit_;	 
	return _SUCCESS;
} 

void free_xmitframe_queue(struct xmit_priv *pxmitpriv, _queue *pframequeue)//(_queue *pframequeue,  _queue *pfree_xmit_queue)
{

	_irqL irqL;
	_list	*plist, *phead;
	struct	xmit_frame 	*pxmitframe;
_func_enter_;	

	_enter_critical(&(pframequeue->lock), &irqL);

	phead = get_list_head(pframequeue);
	plist = get_next(phead);
	
	while (end_of_queue_search(phead, plist) == _FALSE)
	{
			
		pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);

		plist = get_next(plist); 
		
		free_xmitframe(pxmitpriv,pxmitframe);
			
	}
	_exit_critical(&(pframequeue->lock), &irqL);

_func_exit_;
}

static __inline struct tx_servq *get_sta_pending
	(_adapter *padapter, _queue **ppstapending, struct sta_info *psta, sint up)
{

	struct tx_servq *ptxservq;
_func_enter_;	
	if(IS_MCAST(psta->hwaddr))
	{
		ptxservq = &(psta->sta_xmitpriv.be_q); // we will use be_q to queue bc/mc frames in BCMC_stainfo
		*ppstapending = &padapter->xmitpriv.bm_pending; 
	}
	else
	{
	switch (up) {

		case 1:
		case 2:
			ptxservq = &(psta->sta_xmitpriv.bk_q);
			*ppstapending = &padapter->xmitpriv.bk_pending;
			DEBUG_INFO(("get_sta_pending : BK \n"));
			break;

		case 4:
		case 5:
			ptxservq = &(psta->sta_xmitpriv.vi_q);
			*ppstapending = &padapter->xmitpriv.vi_pending;
			DEBUG_INFO(("get_sta_pending : VI\n"));
			break;

		case 6:
		case 7:
			ptxservq = &(psta->sta_xmitpriv.vo_q);
			*ppstapending = &padapter->xmitpriv.vo_pending;
			DEBUG_INFO(("get_sta_pending : VO \n"));			
			break;

		case 0:
		case 3:
		default:
			ptxservq = &(psta->sta_xmitpriv.be_q);
			*ppstapending = &padapter->xmitpriv.be_pending;
			DEBUG_INFO(("get_sta_pending : BE \n"));			
			break;
	}

	}

_func_exit_;
	return ptxservq;	
		
}

sint xmit_classifier_direct(_adapter *padapter, struct xmit_frame *pxmitframe)
{

//	_irqL	 irqL;
	_queue* pq;
	u8 type, subtype;
	u8 * pkt_start;
	sint res = _SUCCESS;
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);
	struct	pkt_attrib *pattrib = &pxmitframe->attrib;
	sint 		bmcst = IS_MCAST(pattrib->ra);

	pkt_start = get_xmit_payload_start_addr(pxmitframe);

	type =  GetFrameType(pkt_start);
	subtype =  GetFrameSubType(pkt_start);

	DEBUG_INFO((" xmit_classifier_direct: type=%x subtype=%x\n", type, subtype));

_func_enter_;

	if(type == WIFI_MGT_TYPE) //mgnt
	{
		if(subtype == WIFI_BEACON)
			pq = &pxmitpriv->beacon_pending;
		else
			pq = &pxmitpriv->mgnt_pending;
	}

	else  //data
	{
		if(bmcst)
		{
			pq = &pxmitpriv->be_pending;//enqueue to bm_pending
		}
		else
		{
			switch(pattrib->priority)
			{
				case 1:
				case 2: 
					//enqueue to bk_pending;
					pq = &pxmitpriv->bk_pending;
					break;
					
				case 4:
				case 5: 
					pq = &pxmitpriv->vi_pending;
					break;
		
				case 6:
				case 7:
					pq = &pxmitpriv->vo_pending;
					break;
					
				case 0:
				case 3:
				default:	
					pq = &pxmitpriv->be_pending;
					break;
		
			}			
		}
	}

	list_delete(&pxmitframe->list);

	list_insert_tail(&pxmitframe->list, get_list_head(pq));				

	return res;
}

/*
Will enqueue pxmitframe to the proper queue, and indicate it to xx_pending list.....


*/
sint xmit_classifier(_adapter *padapter, struct xmit_frame *pxmitframe)
{

	_irqL	 irqL0, irqL1;
	_queue	*pstapending;
	struct	sta_info	*psta;
	struct	tx_servq	*ptxservq;
	struct	pkt_attrib *pattrib = &pxmitframe->attrib;
	struct	sta_priv *pstapriv = &padapter->stapriv;
	struct 	mlme_priv *pmlmepriv = &padapter->mlmepriv;	
	sint 		bmcst = IS_MCAST(pattrib->ra);
	sint		res=_SUCCESS;
_func_enter_;
	if(bmcst)
	{
		psta = get_bcmc_stainfo(padapter); // yitsen
		DEBUG_INFO(("xmit_classifier : get_bcmc_stainfo\n"));
	}
	else
	{
             if (check_fwstate(pmlmepriv, WIFI_MP_STATE))
	       	psta = get_stainfo(pstapriv, get_bssid(pmlmepriv));
		else
			psta = get_stainfo(pstapriv, pattrib->ra);
        }
	
	if (psta == NULL){
		res= _FAIL;
		DEBUG_ERR(("psta == NULL"));
		goto exit;
	}

	ptxservq = get_sta_pending(padapter, &pstapending, psta, pattrib->priority);


	_enter_critical(&pstapending->lock, &irqL0);

	//4 //register to xmit_priv in order to xmit pkts //yt
	if (is_list_empty(&ptxservq->tx_pending))
	{
		list_insert_tail(&ptxservq->tx_pending, get_list_head(pstapending));
	
	}


	_enter_critical(&ptxservq->sta_pending.lock, &irqL1);

	list_insert_tail(&pxmitframe->list, get_list_head(&ptxservq->sta_pending));

	_exit_critical(&ptxservq->sta_pending.lock, &irqL1);



	_exit_critical(&pstapending->lock, &irqL0);


exit:
_func_exit_;
	return res;

}

static __inline sint get_page_cnt (uint sz)
{

	return (sz >> 6) + ( (sz & 63) ? 1 : 0);
	
}


static __inline sint check_hwresource(struct xmit_priv *pxmitpriv, struct xmit_frame *pxmitframe, struct hw_txqueue *ptxhwqueue)
{
	_irqL irqL;
	sint i;
	sint nr_txcmds;
	sint n_pages;
	
	struct	pkt_attrib *pattrib = &pxmitframe->attrib;
_func_enter_;	
	nr_txcmds = pattrib->nr_frags;
	
	for(i = 0, n_pages = 0; i < pattrib->nr_frags; i++)
	{
		if (i == pattrib->nr_frags - 1)
			n_pages += pattrib->last_txcmdsz;
		else
			n_pages += pxmitpriv->frag_len;
	}

	n_pages = get_page_cnt (n_pages);

	if (ptxhwqueue->free_cmdsz > nr_txcmds)
	{
#ifndef CONFIG_USB_HCI
		if (ptxhwqueue->free_sz >= (n_pages ))
		{
			_enter_critical(&pxmitpriv->lock, &irqL);

			//ptxhwqueue->budget_sz -= n_pages;
			ptxhwqueue->free_sz -=  n_pages;

			ptxhwqueue->free_cmdsz -= nr_txcmds;
			
			_exit_critical(&pxmitpriv->lock, &irqL);

			return nr_txcmds;
		
		}
#else

		_enter_critical(&pxmitpriv->lock, &irqL);

			ptxhwqueue->free_cmdsz -= nr_txcmds;
			
			_exit_critical(&pxmitpriv->lock, &irqL);


			return nr_txcmds;
#endif


	}



_func_exit_;	

	return 0;
	
}

#ifdef CONFIG_RTL8711
static  u8 * checkout_scsibuffer(_adapter *padapter, struct SCSI_BUFFER * psb)
{

	u8 * res;
	
	if(CIRC_SPACE(psb->head, psb->tail, SCSI_BUFFER_NUMBER)==0 )
	{
		res = NULL;
		goto exit;
	}

	res = psb->pscsi_buffer_entry[psb->head].entry_memory;
 
	if ((psb->head+ 1) == (SCSI_BUFFER_NUMBER))
		psb->head  = 0;
	else
		psb->head ++;


exit:	
	return res;
}
#endif /*CONFIG_RTL8711*/

static int check_scsibuffer_entry(struct xmit_frame *pxmitframe, u8 * pscsi_buf_entry)
{
	int nr_txcmds;
	struct	pkt_attrib *pattrib = &pxmitframe->attrib;
	_adapter *padapter = pxmitframe->padapter;
	int i;
	int total = 0;
	int res;

	for(i=0; i<HWXMIT_ENTRY; i++)  //The first 4 bytes of pscsi_buf_entry defines the number of TXCMD for each AC queue.
		total += pscsi_buf_entry[i];

	nr_txcmds = pattrib->nr_frags;
	
	total += nr_txcmds;
	
	if(total < padapter->max_txcmd_nr)
	{
		res = _SUCCESS;
	}
	else
	{
		res = _FAIL;
	}
	

	return res;
}

sint dequeue_xmitframes_direct(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, sint entry, u8 * pscsi_buf_entry)
{
	sint	inx, tx_action = 0;
	_irqL irqL;
	_list	 *xmitframe_plist, *xmitframe_phead;
	struct	xmit_frame	*pxmitframe = NULL;
	
_func_enter_;

	_enter_critical(&pxmitpriv->lock, &irqL);	

	for(inx = 0; inx < entry; inx++, phwxmit++) //bcmc/vo/vi/be/bk
	{
		xmitframe_phead  = get_list_head(phwxmit->sta_queue);
		xmitframe_plist = get_next(xmitframe_phead);

		while ((end_of_queue_search(xmitframe_phead, xmitframe_plist)) == _FALSE)
		{
	
			pxmitframe= LIST_CONTAINOR(xmitframe_plist, struct xmit_frame ,list);
			
			//john
			if(check_scsibuffer_entry(pxmitframe, pscsi_buf_entry)==_SUCCESS)  //check if the entry is less than the number of MAX_TXCMD
			{				
				list_delete(&pxmitframe->list);
				list_insert_tail(&pxmitframe->list, &phwxmit->pending);
				pscsi_buf_entry[inx]++;
				tx_action = 1;
			}
			else
			{
				DEBUG_ERR(("schec_scsibuffer_entry!=_SUCCESS"));
				tx_action = 1;
				break;				
			}

			xmitframe_plist = get_next(xmitframe_phead);
		}
}

	_exit_critical(&pxmitpriv->lock, &irqL);	
	return tx_action;
_func_exit_;
}

sint dequeue_xmitframes_direct_87b(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, sint entry, u8* pscsi_buf_entry)
{
	sint inx, tx_action = _SUCCESS;
	_irqL irqL;
	_list	 *xmitframe_plist, *xmitframe_phead;
	struct	xmit_frame	*pxmitframe = NULL;

	DEBUG_INFO(("+dequeue_xmitframes_direct_87b\n"));

_func_enter_;

	_enter_critical(&pxmitpriv->lock, &irqL);	

	for(inx = 0; inx < entry; inx++, phwxmit++) //bcmc/vo/vi/be/bk
	{
		xmitframe_phead  = get_list_head(phwxmit->sta_queue);
		xmitframe_plist = get_next(xmitframe_phead);

		while ((end_of_queue_search(xmitframe_phead, xmitframe_plist)) == _FALSE)
		{
	
			pxmitframe= LIST_CONTAINOR(xmitframe_plist, struct xmit_frame ,list);
			list_delete(&pxmitframe->list);
			list_insert_tail(&pxmitframe->list, &phwxmit->pending);

			xmitframe_plist = get_next(xmitframe_phead);
		}
	}

	_exit_critical(&pxmitpriv->lock, &irqL);	
	DEBUG_INFO(("-dequeue_xmitframes_direct_87b\n"));
_func_exit_;

	return tx_action;
}


#ifdef CONFIG_USB_HCI
#ifdef CONFIG_RTL8711
static void dump_xmitframes(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, int entry, u8 * pscsi_buf_entry)
{
//	_irqL irqL;
	sint i,t, sz=0;
//	int p_sz;
	_list	*plist, *phead;
	sint bmcast;	
	u8	*mem_addr;

	uint	val;
	struct xmit_frame	*pxmitframe;
	struct pkt_attrib	*pattrib;
	
	//write fifo first... then write txcmd...
//	struct	hw_txqueue	*phwtxqueue;

	_adapter *adapter =pxmitpriv->adapter;
	
	struct security_priv *psecuritypriv = &adapter->securitypriv;
	struct qos_priv	*pqospriv= &adapter->qospriv;

	//new txcmd method declare, john 1114
//	struct SCSI_BUFFER *psb = adapter->pscsi_buf;
//	struct SCSI_BUFFER_ENTRY *writing_scsi_buffer = &(psb->pscsi_buffer_entry[psb->head]);
	u8 *p = pscsi_buf_entry;
//	int buffer_sz;
	struct hw_xmit *phwxmit_orig = phwxmit;
	p += 8; //the first 8 bytes is kept as empty slots becasue of the number of entries in the tx_queue(vo, vi, be, bk, bm)

_func_enter_;


#if 1
	for (i = 0; i < entry; i++, phwxmit++)
	{

		//for usb interface, write txcmd before bulk out tx packets
		phead = &phwxmit->pending;

		plist = get_next(phead);

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			pattrib = &pxmitframe->attrib;

			for(t = 0; t <pattrib->nr_frags; t ++)	
			{

			    	//VCS update
				//If longer than RTS threshold
				if(sz + 4 >= pxmitpriv->rts_thresh && pxmitpriv->vcs_setting == AUTO_VCS)
					pattrib->vcs_mode = pxmitpriv->vcs_type;
				else
					pattrib->vcs_mode  = pxmitpriv->vcs;

				bmcast = IS_MCAST(pattrib->ra);
				if(bmcast) 
					pattrib->vcs_mode  = NONE_VCS;

				if (t == (pattrib->nr_frags- 1))
				{
					val = BIT(31)| BIT(29) ;
					sz = pattrib->last_txcmdsz;
				}
				else{
					val = BIT(31)|BIT(30);
					sz = pxmitpriv->frag_len;
					sz = sz -4 - (pattrib->bswenc ? 0 : pattrib->icv_len);
				}

				if(pqospriv->qos_option)
				{
					val |= (( sz << 16)  | ( pattrib->mac_id << 10) | ( pattrib->priority << 5) );
				}
				else{
					val |= (( sz << 16)  | ( pattrib->mac_id << 10) | BIT(9));	
				}

				if(pattrib->bswenc==_TRUE){
					DEBUG_ERR(("\n dump_xmitframes:pattrib->bswenc=%d\n",pattrib->bswenc));
					val=val |BIT(27);
				}
				
				//endian issue is handled by FW 
				//val = cpu_to_be32(val);
					_memcpy((void *)p, &val, 4);
					p += 4;
			}
			
			plist = get_next(plist);
			
		}
	}
#else
	build_txcmd(pxmitpriv, phwxmit, entry, pscsi_buf_entry);
#endif
	
#ifdef CONFIG_RTL8711
	//write txcmd to scsififo
	write_scsi(adapter, adapter->scsi_buffer_size, pscsi_buf_entry);
#endif

	//restore the address of phwxmit
	phwxmit = phwxmit_orig;
#if 1
	for (i = 0; i < entry; i++, phwxmit++)
	{
              //now... let's trace the list again, and bulk out  tx frames....
		phead = &phwxmit->pending;
		plist = get_next(phead);	

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			pattrib = &pxmitframe->attrib;
			pxmitframe->fragcnt = pattrib->nr_frags;//	
			pxmitframe->irpcnt= pattrib->nr_frags;
			plist = get_next(plist);//get next plist before dequeue xmitframe 		
			list_delete(&pxmitframe->list);//
			
			mem_addr =get_xmit_payload_start_addr(pxmitframe);

			for(t = 0; t < pattrib->nr_frags; t ++)
			{
				
				if (t != (pattrib->nr_frags - 1))
				{
					sz = pxmitpriv->frag_len;
					sz = sz -4 - (pattrib->bswenc ? 0 : pattrib->icv_len);					
					pxmitframe->last[t] = 0;				
				}
				else
				{     
					sz = pattrib->last_txcmdsz;				
					pxmitframe->last[t] = 1;					
				}
			
			       pxmitframe->mem_addr = mem_addr;

                            write_port(adapter, i, sz, (unsigned char*)pxmitframe);
				DEBUG_INFO(("++++++++++dump_xmitframes: xmit ff sz:%d\n", sz));

				mem_addr += sz;
			
				mem_addr = (u8 *)RND4(((uint)(mem_addr)));

			}			
			 
		}
	}
#else
	build_write_txpayload(pxmitpriv, phwxmit, entry);
#endif
		
_func_exit_;
}
#endif /*CONFIG_RTL8711*/

#if 0
static void printpayload(u8 *addr, u32 len)
{
int i = 0 ;
        printk("-----------PAYLOAD--------------\n");
        for(i = 0 ; i < len ; i++)
        {
                printk("%2x ", *(u8 *)(addr+i));

                if((i+1)%8 == 0 )
                        printk("\n");
        }
        printk("-----------END OF PAYLOAD-------\n");

}

#endif
#ifdef CONFIG_RTL8187
static u16 rtl_rate[] = {10,20,55,110,60,90,120,180,240,360,480,540};
inline u16 rtl8180_rate2rate(short rate)
{
	if (rate >11) return 0;
	return rtl_rate[rate]; 
}

static void dump_xmitframes_87b(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, int entry, u8* pscsi_buf_entry)
{
	u32 i;
	sint t, sz=0;
	u8 	bRTSEnable=_FALSE;
	_list	*plist, *phead;
	u8	*mem_addr;
	struct xmit_frame	*pxmitframe;
	struct pkt_attrib	*pattrib;
	
	//write fifo first... then write txcmd...
	_adapter *adapter =pxmitpriv->adapter;
	struct security_priv *psecuritypriv = &adapter->securitypriv;
	struct hw_xmit *phwxmit_orig = phwxmit;

	u8 	*txdesc_addr;	
	u32 *txdesc_addr_u32;
	u16 this_frame_time=0, txdesc_duration=0;
	short tx_rate;
	sint bmcst;
	u16		RtsDur = 0;
	u16 	Duration = 0; 

	struct ieee80211_hdr_3addr_qos *frag_hdr = NULL;

_func_enter_;

	DEBUG_INFO(("+dump_xmitframes_87b \n"));

	for (i = 0; i < entry; i++, phwxmit++)
	{

		if(i==MGNT_QUEUE_INX || i==BEACON_QUEUE_INX) 
			tx_rate = ieeerate2rtlrate(pxmitpriv->ieee_basic_rate);
		else
			tx_rate = ieeerate2rtlrate(adapter->_sys_mib.rate);

		//for usb interface, write txcmd before bulk out tx packets
		phead = &phwxmit->pending;

		plist = get_next(phead);

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			pattrib = &pxmitframe->attrib;
			bmcst = IS_MCAST(pattrib->ra);

			txdesc_addr = ((unsigned char *)pxmitframe->mem) + WLANHDR_OFFSET;
			txdesc_addr_u32 = (u32 *)txdesc_addr;
			mem_addr = get_xmit_payload_start_addr(pxmitframe);
			frag_hdr = (struct ieee80211_hdr_3addr_qos *)mem_addr;

			for(t = 0; t <pattrib->nr_frags; t ++)	
			{

				if (t == (pattrib->nr_frags- 1))
				{
					sz = pattrib->last_txcmdsz;
				}
				else
				{
					sz = pxmitpriv->frag_len;
					sz = sz -4 - (pattrib->bswenc ? 0 : pattrib->icv_len);
				}
			
				if(bmcst) 
				{
					Duration = 0;
					bRTSEnable = _FALSE;				
					pattrib->vcs_mode  = NONE_VCS;
					this_frame_time = ComputeTxTime(sz + sCrcLng, rtl8180_rate2rate(tx_rate), _FALSE, _FALSE/*bUseShortPreamble*/);
					txdesc_duration = this_frame_time;
				}
				else 
				{
					u16 AckTime;
					AckTime = ComputeTxTime(14, 10, _FALSE, _FALSE);	// AckCTSLng = 14 use 1M bps send
					
					//FIXME: CTS/RTS is not added yet
					//If longer than RTS threshold
					if(sz + 4 >= pxmitpriv->rts_thresh && pxmitpriv->vcs_setting == AUTO_VCS)
					{
						u16 RtsTime, CtsTime;

						bRTSEnable = _TRUE;
						
						// Rate and time required for RTS.
						RtsTime = ComputeTxTime( sAckCtsLng/8, pxmitpriv->ieee_basic_rate, _FALSE, _FALSE);
						// Rate and time required for CTS.
						CtsTime = ComputeTxTime(14, 10, _FALSE, _FALSE);	// AckCTSLng = 14 use 1M bps send
						
						// Figure out time required to transmit this frame.
						this_frame_time = ComputeTxTime(sz + sCrcLng, 
								rtl8180_rate2rate(tx_rate), 
								_FALSE, 
								_FALSE/*bUseShortPreamble*/);
						
						// RTS-CTS-ThisFrame-ACK.
						RtsDur = CtsTime + this_frame_time + AckTime + 3*aSifsTime;
						
						txdesc_duration = RtsTime + RtsDur;
						
					}
					else
					{
						bRTSEnable = _FALSE;
						RtsDur = 0;
						pattrib->vcs_mode  = pxmitpriv->vcs;
						
						this_frame_time = ComputeTxTime(sz + sCrcLng, rtl8180_rate2rate(tx_rate), _FALSE, _FALSE/*bUseShortPreamble*/);
						txdesc_duration = this_frame_time + aSifsTime + AckTime;
						
					}
					
					if(!(frag_hdr->frame_ctl & IEEE80211_FCTL_MOREFRAGS)) //no more fragment
					{ 
						// ThisFrame-ACK.
						Duration = aSifsTime + AckTime;
					} 
					else // One or more fragments remained.
					{ 
						u16 NextFragTime;
						NextFragTime = ComputeTxTime( sz + sCrcLng, //pretend following packet length equal current packet
								rtl8180_rate2rate(tx_rate), 
								_FALSE, 
								_FALSE/*bUseShortPreamble*/ );
		
						//ThisFrag-ACk-NextFrag-ACK.
						Duration = NextFragTime + 3*aSifsTime + 2*AckTime;
					}
		
				}		
				


//=================== fill the tx desriptor ========================

				txdesc_addr_u32[0] |= sz & 0xfff;  //skb->length

				if(pattrib->encrypt)
				{
					txdesc_addr_u32[0] &= 0xffff7fff;
					if(pattrib->encrypt == _AES_)
						txdesc_addr_u32[7] |= 0x2;//AES
					else
						txdesc_addr_u32[7] |= 0x1;//WEP & TKIP
				}	
				else			
					txdesc_addr_u32[0] |= (1<<15);	
				
				if(GetMFrag(mem_addr))  
					txdesc_addr_u32[0] |= (1<<17);

				txdesc_addr_u32[0] |= (tx_rate << 24); 

				frag_hdr->duration_id = Duration;

				if(adapter->ieee80211_seq_ctl == 0xfff)
					adapter->ieee80211_seq_ctl = 0;
				else 
					adapter->ieee80211_seq_ctl++;
				
				frag_hdr->seq_ctl = cpu_to_le16(adapter->ieee80211_seq_ctl<<4 | t);

			
				if(bRTSEnable) //rts enable
				{
					txdesc_addr_u32[0] |= ((ieeerate2rtlrate(pxmitpriv->ieee_basic_rate))<<19);//RTS RATE
					txdesc_addr_u32[0] |= (1<<23);//rts enable
					txdesc_addr_u32[1] |= RtsDur;//RTS Duration
				}	
				
				
				txdesc_addr_u32[3] |= (txdesc_duration<<16); //DURATION
			
				if( WLAN_FC_GET_STYPE(le16_to_cpu(frag_hdr->frame_ctl)) == IEEE80211_STYPE_PROBE_RESP )
					txdesc_addr_u32[5] |= (1<<8);//(priv->retry_data<<8); //retry lim ;	
				else
					txdesc_addr_u32[5] |= (11<<8);//(priv->retry_data<<8); //retry lim ; 

	 		}
			plist = get_next(plist);
		}
	}

	//restore the address of phwxmit
	phwxmit = phwxmit_orig;
	for (i = 0; i < entry; i++, phwxmit++)
	{
        //now... let's trace the list again, and bulk out  tx frames....
		phead = &phwxmit->pending;
		plist = get_next(phead);	

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			pattrib = &pxmitframe->attrib;
			pxmitframe->fragcnt = pattrib->nr_frags;//	
			pxmitframe->irpcnt= pattrib->nr_frags;
			plist = get_next(plist);//get next plist before dequeue xmitframe 		
			list_delete(&pxmitframe->list);//
			
			txdesc_addr = (u8 *)pxmitframe->mem + WLANHDR_OFFSET;
			mem_addr = get_xmit_payload_start_addr(pxmitframe);;

			for(t = 0; t < pattrib->nr_frags; t ++)
			{
				
				if (t != (pattrib->nr_frags - 1))
				{
					sz = pxmitpriv->frag_len;
					sz = sz -4 - (pattrib->bswenc ? 0 : pattrib->icv_len);					
					pxmitframe->last[t] = 0;				
				}
				else
				{     
					sz = pattrib->last_txcmdsz;				
					pxmitframe->last[t] = 1;					
				}
			
				pxmitframe->mem_addr = txdesc_addr;
				
				sz += TXDESC_OFFSET;

				DEBUG_INFO((" i(ac_tag)=%d, sz=%d\n",i ,sz));

					
				printpayload(pxmit->mem_addr+TXDESC_OFFSET, sz-TXDESC_OFFSET);
				write_port(adapter, i, sz, (unsigned char*)pxmitframe);
				
				DEBUG_INFO(("++++++++++dump_xmitframes: xmit ff sz:%d\n", sz));
				
				txdesc_addr = txdesc_addr + sz;
				txdesc_addr = (u8 *)RND4(((uint)(txdesc_addr)));
				mem_addr = txdesc_addr + TXDESC_OFFSET;
			}			
			 
		}
	}
		
	DEBUG_ERR(("-dump_xmitframes_87b \n"));
_func_exit_;
}
#endif /*CONFIG_RTL8187*/

#ifdef CONFIG_RTL8192U

//
// This function need to be modified because we do not consider AP mode and IBSS mode
// in this setting.
//
u8  MgntQuery_BandwidthMode(
	  _adapter *padapter, 
	struct xmit_frame *pxmitframe
	)
{
	u8 type, bmcst;
	struct pkt_attrib *pattrib;
	u8 bpacketbw =  _FALSE ;
	u32 alignment  = 0 ;
	PRT_HIGH_THROUGHPUT pHTInfo = &padapter->HTInfo;
	u8 * pkt = ((unsigned char *)pxmitframe->mem) + WLANHDR_OFFSET + TXDESC_OFFSET+TX_FWINFO_OFFSET;
	#ifdef PLATFORM_DVR
	alignment= ALIGNMENT -( (u32)((u8 *)pxmitframe->mem + WLANHDR_OFFSET) & (ALIGNMENT-1) ); ;

	#endif

	pkt +=alignment;

	DEBUG_INFO(("Bandwidth : alignemnt :% d, addr :%p \n",alignment,  pkt));
	type = GetFrameType(pkt);
	pattrib = &pxmitframe->attrib;	
	bmcst = IS_MCAST(pattrib->ra);

	if(!pHTInfo->bCurrentHTSupport)
		goto exit;

	if(type == WIFI_MGT_TYPE  || type == WIFI_CTRL_TYPE)
		goto exit;

	if(bmcst)
		goto exit;	

	//BandWidthAutoSwitch is for auto switch to 20 or 40 in long distance
	if(pHTInfo->bCurBW40MHz && pHTInfo->bCurTxBW40MHz )
		bpacketbw = _TRUE;	
   exit:
   	
	return bpacketbw ; 
}


u1Byte
QueryIsShort(
       _adapter *adapter, 
	u8           TxHT,
	u8           TxRate
	
	)
{
              u8   tmp_Short, bShortGI;
	 	struct mib_info *sys_mib = &adapter->_sys_mib;
 
      	  PRT_HIGH_THROUGHPUT pHTInfo = &adapter->HTInfo; 

		if(TxHT ==1)
		{
			bShortGI = (pHTInfo->bCurShortGI20MHz | pHTInfo->bCurShortGI40MHz)? 1: 0;
		}
		else 
		{
			bShortGI = (sys_mib->rf_ctrl.shortpreamble |!sys_mib->dot11ErpInfo.longPreambleStaNum)? 1:0 ; 
		}
			
			  
		tmp_Short = bShortGI;	  

		if(TxHT==1 && TxRate != DESC90_RATEMCS15)
			tmp_Short = 0;
		
		DEBUG_INFO(("QueryIsShort : %d\n",tmp_Short));
		return tmp_Short;	
}



static void dump_xmitframes_8192(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, int entry, u8* pscsi_buf_entry)
{
	u8 	*mem_addr= NULL,*txdesc_addr, bpacketbw = _FALSE;	
	u32 i, alignment = 0;
	sint t, bmcst = 0,sz=0;
	_list	*plist, *phead;
	struct xmit_frame	*pxmitframe;
	struct pkt_attrib	*pattrib;
	_adapter *adapter =pxmitpriv->adapter;
	struct ieee80211_hdr_3addr_qos *frag_hdr = NULL;

	PTX_DESC_8192_USB	pDesc = NULL;
 	PTX_FWINFO_8192USB	pTxFwInfo = NULL;

	PRT_HIGH_THROUGHPUT pHTInfo = &adapter->HTInfo;

_func_enter_;

	DEBUG_INFO(("+dump_xmitframes_8192 \n"));

	for (i = 0; i < entry; i++, phwxmit++)
	{

	
		//for usb interface, write txcmd before bulk out tx packets
		phead = &phwxmit->pending;

		plist = get_next(phead);

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			pattrib = &pxmitframe->attrib;
			bmcst = IS_MCAST(pattrib->ra);
			plist = get_next(plist);
			list_delete(&pxmitframe->list);

		#ifdef PLATFORM_DVR
			alignment = ALIGNMENT -( (u32)((u8 *)pxmitframe->mem + WLANHDR_OFFSET) & (ALIGNMENT-1) );

		#endif 
			DEBUG_INFO(("pxmitframe->mem+WLANHDR_OFFSET :%p\n",(u8 *)pxmitframe->mem+WLANHDR_OFFSET));
			DEBUG_INFO(("dump_xmit :alignemnt :% d\n",alignment));

	//check AMSDU & Aggregation 
	#ifdef CONFIG_RTL8192U

		
	#if 0
		// TODO: check if supported AMSDU and filter out non-AMSDU frame
		if(SUUPORT_AMSDU)
		{
			_list	 SendList;
			_init_listhead(&SendList);
			if(AMSDU_GetAggregatibleList(adapter, pxmitframe, &SendList, phwxmit) > 1)
			AMSDU_Aggregation(adapter, &SendList);
		}
	#endif
	
	//		xmitframe_coalesce(adapter, pxmitframe->pkt, pxmitframe);
	#endif
			
			txdesc_addr = (u8 *)pxmitframe->mem + WLANHDR_OFFSET;
			txdesc_addr += alignment;
			mem_addr = txdesc_addr + TXDESC_OFFSET+ TX_FWINFO_OFFSET;
			DEBUG_INFO(("algnment : %p\n", txdesc_addr+alignment));	
			DEBUG_INFO(("txdesc_addr : %p\n", txdesc_addr));
			TxRateSelectMode(pxmitframe);
			MgntQuery_AggregationCapability(adapter, pxmitframe);
			MgntQuery_ProtectionFrame(pxmitframe);

			bpacketbw = MgntQuery_BandwidthMode(adapter, pxmitframe);
				
			pxmitframe->fragcnt = pattrib->nr_frags;		
		//	printk("dump8192_xmit: nr_frags: %d\n",pxmitframe->fragcnt);


			if(adapter->ieee80211_seq_ctl == 0xfff)
                               adapter->ieee80211_seq_ctl = 0;
                        
			 else
                                adapter->ieee80211_seq_ctl++;
	
			for(t = 0; t <pattrib->nr_frags; t ++)	
			{
				frag_hdr = (struct ieee80211_hdr_3addr_qos *)mem_addr;

				if (t == (pattrib->nr_frags- 1))
				{
					sz = pattrib->last_txcmdsz;
					pxmitframe->last[t] = 0;				
				}
				else
				{
					sz = pxmitpriv->frag_len;
					sz = sz -4 - (pattrib->bswenc ? 0 : pattrib->icv_len);
					pxmitframe->last[t] = 1;				
				}
			
				frag_hdr->duration_id = 0 ;
			

				frag_hdr->seq_ctl = cpu_to_le16(adapter->ieee80211_seq_ctl<<4 | t);
			
			//	printk("dump8192_xmit: seq: %d\n",adapter->ieee80211_seq_ctl);
// -------------------------8192 FWInfo ----------------------

				pTxFwInfo = (PTX_FWINFO_8192USB)(txdesc_addr+TXDESC_OFFSET);
				memset(pTxFwInfo,0 , sizeof(TX_FWINFO_8192USB));

				
				pTxFwInfo->TxHT = (pHTInfo->HTCurrentOperaRate&0x80)?1:0;	

				if(i==MGNT_QUEUE_INX || i==BEACON_QUEUE_INX) 
					pTxFwInfo->TxRate = ieeerate2rtlrate(pxmitpriv->ieee_basic_rate);
				else
					pTxFwInfo->TxRate = (pHTInfo->bCurrentHTSupport)?ieeerate2rtlrate(pHTInfo->HTCurrentOperaRate): ieeerate2rtlrate(adapter->_sys_mib.rate);

				if(pHTInfo->bCurrentHTSupport)
					DEBUG_INFO(("Tx rate : %x\n", pHTInfo->HTCurrentOperaRate));
			//	printk("dump8192_xmit: TxRate: %d, packet size: %d\n",adapter->_sys_mib.rate,sz);
				
				if(i == BEACON_QUEUE_INX)
					pTxFwInfo->EnableCPUDur  = _FALSE;
				else
				pTxFwInfo->EnableCPUDur = _TRUE;//_FALSE;//pTcb->bTxEnableFwCalcDur;

				
				 pTxFwInfo->Short	= QueryIsShort(adapter, pTxFwInfo->TxHT, pTxFwInfo->TxRate);	

				//AMPDU & AMSDU issue
				if(pHTInfo->bCurrentHTSupport && pHTInfo->bCurrentAMPDUEnable)
				{
					if(!(i==MGNT_QUEUE_INX || i==BEACON_QUEUE_INX))
					{
						pTxFwInfo->AllowAggregation = 1;
						pTxFwInfo->RxMF = pHTInfo->CurrentAMPDUFactor;
						pTxFwInfo->RxAMD = pHTInfo->CurrentMPDUDensity;
					}
				}
				else
				{
					pTxFwInfo->AllowAggregation = 0;
					pTxFwInfo->RxMF = 0; // pTcb->AMPDUFactor;
					pTxFwInfo->RxAMD = 0; //pTcb->AMPDUDensity;
				}

		//		printk("RxMF : %d , RxAMD: %d\n", pTxFwInfo->RxMF, pTxFwInfo->RxAMD);
				// Get from MgntQuery_ProtectionFrame();
				pTxFwInfo->RtsEnable = pattrib->bRTSEnable?1:0;
				pTxFwInfo->CtsEnable =  pattrib->bCTSEnable?1:0;
				pTxFwInfo->RtsSTBC =  pattrib->bRTSSTBC ? 1:0;
				pTxFwInfo->RtsHT= 	(pattrib->RTSRate&0x80)?1:0;
				pTxFwInfo->RtsRate = 	  ieeerate2rtlrate(pattrib->RTSRate*10/2);
				pTxFwInfo->RtsSubcarrier =	(pTxFwInfo->RtsHT==0)?(pattrib->RTSSC):0;
				pTxFwInfo->RtsBandwidth = (pTxFwInfo->RtsHT==1)?((pattrib->bRTSBW)?1:0):0;
				pTxFwInfo->RtsShort = (pTxFwInfo->RtsHT==0)?(pattrib->bRTSUseShortPreamble?1:0):(pattrib->bRTSUseShortGI?1:0);

#if 1  // For Test 
				DEBUG_INFO(("dump_xmitframe:TxHT : %d\n", pTxFwInfo->TxHT));			
				DEBUG_INFO(("dump_xmitframe: TxRate : %d\n", pTxFwInfo->TxRate));
				DEBUG_INFO(("dump_xmitframe: RtsRate : %d\n", pTxFwInfo->RtsRate));
				DEBUG_INFO(("dump_xmitframe: RtsHT : %d\n", pTxFwInfo->RtsHT));
				DEBUG_INFO(("dump_xmitframe: RtsBandwidth : %d\n", pTxFwInfo->RtsBandwidth));
				DEBUG_INFO(("TX fwinfo TxINFO_RSVD: %x\n", pTxFwInfo->Tx_INFO_RSVD));
#endif
				// Set Bandwidth and sub-channel settings.
				//
		
				if(pHTInfo->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
				{
				
					DEBUG_INFO(("dump_xmitframe: CurrentChannelBW : %d\n", pHTInfo->CurrentChannelBW));
					if(bpacketbw)
					{
						pTxFwInfo->TxBandwidth = 1;
						pTxFwInfo->TxSubCarrier = 3;
					}
					else
					{
						pTxFwInfo->TxBandwidth = 0;
						pTxFwInfo->TxSubCarrier = pHTInfo->nCur40MhzPrimeSC;
					}
				}
				else
				{
					pTxFwInfo->TxBandwidth = 0;
					pTxFwInfo->TxSubCarrier = 0;
				}



//=================== fill the tx desriptor ========================

			 	pDesc = (PTX_DESC_8192_USB)txdesc_addr;
				memset(pDesc,0 , sizeof(TX_DESC_8192_USB));
				pDesc->LINIP = 0;
				pDesc->CmdInit = 1;	
				pDesc->Offset = sizeof(TX_FWINFO_8192USB) + 8; 
				pDesc->PktSize =  sz ;
				pDesc->SecCAMID= 0;


				pDesc->RATid = pattrib->RATRIndex;
				pDesc->NoEnc = (pattrib->encrypt==0 || pattrib->bswenc ) ? 1 : 0 ;
								
				switch(pattrib->encrypt){
						case _NO_PRIVACY_:
							pDesc->SecType = 0x0;
							break;
						case _WEP40_:
						case _WEP104_:
							pDesc->SecType = 0x1;
							break;
						case _TKIP_:
							pDesc->SecType = 0x2;
							break;
						case _AES_:
							pDesc->SecType = 0x3;
							break;
						default:
							pDesc->SecType = 0x0;
							break;			
					}

				pDesc->QueueSelect = MapHwQueueToFirmwareQueue(i);
				pDesc->TxFWInfoSize = sizeof(TX_FWINFO_8192USB);	
				pDesc->DISFB = pattrib->bTxDisableRateFallBack;
				pDesc->USERATE 	= pattrib->bTxUseDriverAssingedRate;
				pDesc->FirstSeg = (t == 0 )? 1:0;
				pDesc->LastSeg = (t == (pattrib->nr_frags- 1))?1:0;
				DEBUG_INFO(("firstSeg: %d, Lastseg: %d", pDesc->FirstSeg, pDesc->LastSeg));
				pDesc->TxBufferSize= (u16)(sz+ sizeof(TX_FWINFO_8192USB));
				pDesc->OWN = 1;

				pxmitframe->mem_addr = txdesc_addr;

				if((u32)pxmitframe->mem_addr &(ALIGNMENT -1))
					DEBUG_INFO(("dump_xmitframe: pxmitframe->mem_addr : %p not 512 byte alignment !!!\n", pxmitframe->mem_addr));
				
				sz += TXDESC_OFFSET+TX_FWINFO_OFFSET;
				DEBUG_INFO((" i(ac_tag)=%d, sz=%d\n",i ,sz));

				write_port(adapter, i, sz, (unsigned char*)pxmitframe);
				DEBUG_INFO(("dump_xmitframe: TXDESC: %d, TX_FW_SIZE: %d", TXDESC_OFFSET, TX_FWINFO_OFFSET));	
				DEBUG_INFO(("++++++++++dump_xmitframes: xmit ff sz:%d\n", sz));
			//	printpayload(pxmitframe->mem_addr + (TXDESC_OFFSET+TX_FWINFO_OFFSET) ,32 );//sz-(TXDESC_OFFSET+TX_FWINFO_OFFSET));		
				txdesc_addr = txdesc_addr + sz;
				txdesc_addr = (u8 *)RND4(((uint)(txdesc_addr)));
				mem_addr = txdesc_addr + TXDESC_OFFSET+TX_FWINFO_OFFSET;
			
						
 			}
	
	  }
		
    }


	DEBUG_INFO(("-dump_xmitframes_8192\n"));
_func_exit_;
}

#endif/*CONFIG_RTL8192U*/



#else

static void dump_xmitframes(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, sint entry)
{
	//_irqL irqL;

	sint i,t, sz, offset;//, initial;
	int p_sz;
	_list	*plist, *phead;
	sint bmcast;

	
	u8	*mem_addr;

	uint	val;
	struct xmit_frame	*pxmitframe;
	struct pkt_attrib	*pattrib;
	
	//write fifo first... then write txcmd...
	struct	hw_txqueue	*phwtxqueue;

	_adapter *adapter =pxmitpriv->adapter;

	struct qos_priv	*pqospriv= &adapter->qospriv;

	struct security_priv *psecuritypriv = &adapter->securitypriv;
_func_enter_;
	for (i = 0; i < entry; i++, phwxmit++)
	{

		phwtxqueue = phwxmit->phwtxqueue;
		//now... let's trace the list again, and write txcmds.... Wao, always absolutely!
		phead = &phwxmit->pending;

		plist = get_next(phead);

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			pattrib = &pxmitframe->attrib;

			for(t = 0; t <pattrib->nr_frags; t ++)	
			{
				offset = ( (phwtxqueue->head + t ) & 7 ) ;

#if consider_bm_ff
				if(i==0)			
					offset = ( (phwtxqueue->head + t ) & 3 ) ;
#endif	/*consider_bm_ff*/			
				                          
				if (t == (pattrib->nr_frags- 1))
				{
					val = BIT(31)| BIT(29) ;
					sz = pattrib->last_txcmdsz;
				}
				else{
					val = BIT(31)|BIT(30);
					sz = pxmitpriv->frag_len;
					//sz = sz -4 - (psecuritypriv->sw_encrypt ? 0 : pattrib->icv_len);
					sz = sz -4 - (pattrib->bswenc ? 0 : pattrib->icv_len);
				}

			       phwtxqueue->txsz[offset]=get_page_cnt(sz);
			       //DEBUG_ERR("ac=%d, freesz=%d, xmit p_cnt:%d, i:%d\n",  phwtxqueue->ac_tag, phwtxqueue->free_sz, phwtxqueue->txsz[offset],offset);   
				//val |= (( sz << 16)  | ( pattrib->mac_id << 10) | ( pattrib->priority << 5) | BIT(9));



			    	//VCS update
			    	
				//If longer than RTS threshold
				if(sz + 4 >= pxmitpriv->rts_thresh && pxmitpriv->vcs_setting == AUTO_VCS)
					pattrib->vcs_mode = pxmitpriv->vcs_type;
				else
					pattrib->vcs_mode  = pxmitpriv->vcs;

				bmcast = IS_MCAST(pattrib->ra);
				if(bmcast) 
					pattrib->vcs_mode  = NONE_VCS;


				//write32(adapter, phwtxqueue->cmd_hwaddr + (offset << 4) + 0, pattrib->vcs_mode);
                            	//ioreq_write32(adapter, phwtxqueue->cmd_hwaddr + (offset << 4) + 0, pattrib->vcs_mode);	



				if(pqospriv->qos_option)
				{
					val |= (( sz << 16)  | ( pattrib->mac_id << 10) | ( pattrib->priority << 5) );
				}
				else{
					val |= (( sz << 16)  | ( pattrib->mac_id << 10) | BIT(9));	
				}

				if(pattrib->bswenc==_TRUE){
					DEBUG_ERR(("\n dump_xmitframes:pattrib->bswenc=%d\n",pattrib->bswenc));
					val=val |BIT(27);
				}
				DEBUG_ERR(("\n dump_xmitframes:pattrib->bswenc=%d \n",pattrib->bswenc));

				//write32(adapter, phwtxqueue->cmd_hwaddr + (offset << 4) + 4, val);
				ioreq_write32(adapter, phwtxqueue->cmd_hwaddr + (offset << 4) + 4, val);	

			}
			
			plist = get_next(plist);
			phwtxqueue->head += pattrib->nr_frags;
			phwtxqueue->head  &= ( 8 - 1);
			
#if consider_bm_ff
			if(i==0)			
				phwtxqueue->head  &= 3;
#endif	/*consider_bm_ff*/			


                      //sync_ioreq_flush(adapter, (struct io_queue*)adapter->pio_queue);
		}

		phead = &phwxmit->pending;
		plist = get_next(phead);
		
		if (end_of_queue_search(phead, plist) == _FALSE)
		{
			sync_ioreq_flush(adapter, (struct io_queue*)adapter->pio_queue);
		}

		//phwtxqueue->budget_sz = 0;

		if((pxmitpriv->mapping_addr == 0) || (pxmitpriv->mapping_addr != phwtxqueue->ff_hwaddr))
		{			
		if (end_of_queue_search(phead, plist) == _FALSE)
			{
		   write32(adapter, HWFF0ADDR, phwtxqueue->ff_hwaddr);
			   pxmitpriv->mapping_addr = phwtxqueue->ff_hwaddr;
			}
		}
		

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			pattrib = &pxmitframe->attrib;
			mem_addr = ((unsigned char *)pxmitframe->mem) + WLANHDR_OFFSET;

			for(t = 0; t < pattrib->nr_frags; t ++)
			{
				
				if (t != (pattrib->nr_frags - 1))
				{
					sz = pxmitpriv->frag_len;
					sz = sz - 4  - ((pattrib->bswenc) ? 0 : pattrib->icv_len);
				}
				else
				{     
					sz = pattrib->last_txcmdsz;
				}

                            p_sz = (get_page_cnt(sz) << 6) ;
				if((pxmitpriv->pkt_sz== 0) || (pxmitpriv->pkt_sz != p_sz))
				{		
					  write16(adapter, PKTSZ, (unsigned short)p_sz);			
					  pxmitpriv->pkt_sz = p_sz;
				}
				
				//write_port(adapter, phwtxqueue->ff_hwaddr, _RND4(p_sz), mem_addr);
				write_port(adapter, Host_W_FIFO, _RND4( p_sz), mem_addr);			
                                DEBUG_ERR(("dump_xmitframes: xmit ff sz:%d\n", sz));		

#ifndef CONFIG_HWPKT_END

				write8(adapter, ACCTRL,  (1 << phwtxqueue->ac_tag) | 0x80);
				
#endif /*CONFIG_HWPKT_END*/
			
		
			
				mem_addr += (pxmitpriv->frag_len-4);//sz;
			
				mem_addr = (u8 *)RND4(((uint)(mem_addr)));

			}
			
			plist = get_next(plist); 
		}



	}

_func_exit_;
	
}
#endif /*CONFIG_USB_HCI*/

sint txframes_pending_nonbcmc(_adapter *padapter)
{

	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;	
	
#ifdef CONFIG_RTL8711	
	return ( (_queue_empty(&pxmitpriv->be_pending)== _FALSE) ||	
			 (_queue_empty(&pxmitpriv->bk_pending) == _FALSE) || 
			 (_queue_empty(&pxmitpriv->vi_pending) == _FALSE) ||
			 (_queue_empty(&pxmitpriv->vo_pending) == _FALSE)  );
#endif

#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)
return ( (_queue_empty(&pxmitpriv->be_pending)== _FALSE) || 
		 (_queue_empty(&pxmitpriv->bk_pending) == _FALSE) || 
		 (_queue_empty(&pxmitpriv->vi_pending) == _FALSE) ||
		 (_queue_empty(&pxmitpriv->vo_pending) == _FALSE) ||
		 (_queue_empty(&pxmitpriv->mgnt_pending) == _FALSE) ||
		 (_queue_empty(&pxmitpriv->beacon_pending) == _FALSE) );
#endif
	
}

static sint txframes_pending(_adapter *padapter)
{

	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;	

	return ( (_queue_empty(&pxmitpriv->be_pending)== _FALSE) ||	
			 (_queue_empty(&pxmitpriv->bk_pending) == _FALSE) || 
			 (_queue_empty(&pxmitpriv->vi_pending) == _FALSE) ||
			 (_queue_empty(&pxmitpriv->vo_pending) == _FALSE) ||
			 (_queue_empty(&pxmitpriv->bm_pending) == _FALSE) );
	
}

#ifndef CONFIG_USB_HCI
static void free_xmitframes(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, sint entry)
{

	sint i;

	_list	*plist, *phead;
	

	struct xmit_frame	*pxmitframe;
	struct	hw_txqueue	*phwtxqueue;
_func_enter_;
	for (i = 0; i < entry; i++, phwxmit++)
	{

		phwtxqueue = phwxmit->phwtxqueue;	

		phead = &phwxmit->pending;

		plist = get_next(phead);

		while (end_of_queue_search(phead, plist) == _FALSE)
		{
			
			pxmitframe = LIST_CONTAINOR(plist, struct xmit_frame, list);
			
			plist = get_next(plist);
			
			free_xmitframe(pxmitpriv,pxmitframe);

		}
	
	}
_func_exit_;	
}
#endif /*CONFIG_USB_HCI*/


#ifdef LINUX_TX_TASKLET
void xmit_tasklet(_adapter * padapter)
{

	sint dequeue_res;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
#ifdef LINUX_TX_TASKLET
	struct hw_xmit *hwxmits = pxmitpriv->hwxmits; 
#else
	struct hw_xmit *hwxmits;
	hwxmits = (struct hw_xmit *)_malloc(sizeof (struct hw_xmit) * HWXMIT_ENTRY);
#endif
	u8 * pscsi_buf_entry = NULL;
#ifdef CONFIG_RTL8711
	struct SCSI_BUFFER *psb = padapter->pscsi_buf;
#endif 
        pxmitpriv->vo_txqueue.head = 0;
#ifndef LINUX_TX_TASKLET
	hwxmits[0] .phwtxqueue = &pxmitpriv->vo_txqueue;
	hwxmits[0] .sta_queue = &pxmitpriv->vo_pending;

	pxmitpriv->vi_txqueue.head = 0;
	hwxmits[1] .phwtxqueue = &pxmitpriv->vi_txqueue;
	hwxmits[1] .sta_queue = &pxmitpriv->vi_pending;

	pxmitpriv->be_txqueue.head = 0;
	hwxmits[2] .phwtxqueue = &pxmitpriv->be_txqueue;
	hwxmits[2] .sta_queue = &pxmitpriv->be_pending;
	
	pxmitpriv->bk_txqueue.head = 0;
	hwxmits[3] .phwtxqueue = &pxmitpriv->bk_txqueue;
	hwxmits[3] .sta_queue = &pxmitpriv->bk_pending;
	
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

	hwxmits[4] .sta_queue = &pxmitpriv->mgnt_pending;

	hwxmits[5] .sta_queue = &pxmitpriv->beacon_pending;
	
#endif /*CONFIG_RTL8187*/

  
	pxmitpriv->hwxmits=hwxmits;
#endif
	while(1)
	{

		if ((txframes_pending_nonbcmc(padapter)) == _FALSE)
		{
			DEBUG_INFO(("xmit thread => no pending data\n"));
			break;
		}
#ifndef LINUX_TX_TASKLET			
		init_hwxmits(hwxmits, HWXMIT_ENTRY);
#endif
		
#ifdef CONFIG_RTL8711
		//Perry : check
		if (register_tx_alive(padapter) != _SUCCESS) 
		{
			DEBUG_ERR(("xmit thread => register_tx_alive; return xmit_thread\n"));
			break;
		}

		pscsi_buf_entry = checkout_scsibuffer(padapter, psb);
		if (pscsi_buf_entry ==NULL) 
		{
			DEBUG_ERR(("not enough scsi buffer space psb_head = %d, psb_tail = %d\n", psb->head, psb->tail));
			break;
		}

		dequeue_res = dequeue_xmitframes_direct(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
		if( dequeue_res <=0 )
			break;

		dump_xmitframes(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
#endif /*CONFIG_RTL8711*/

#ifdef CONFIG_RTL8187		
		dequeue_res = dequeue_xmitframes_direct_87b(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
		if( dequeue_res <=0 )
			break;

		dump_xmitframes_87b(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
#endif /*CONFIG_RTL8187*/
#ifdef CONFIG_RTL8192U

		dequeue_res = dequeue_xmitframes_direct_87b(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
		if( dequeue_res <=0 )
			break;

		dump_xmitframes_8192(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);


#endif/*CONFIG_RTL8192U*/








		// Free all the dumpped xmit_frames below...
#ifndef CONFIG_USB_HCI
		free_xmitframes(pxmitpriv, hwxmits, HWXMIT_ENTRY);
#endif /*CONFIG_USB_HCI*/

		//Perry : check

		
	}

}

#else

thread_return xmit_thread(thread_context context)
{

	sint dequeue_res;
	struct hw_xmit *hwxmits;
	u8 * pscsi_buf_entry = NULL;
	_adapter * padapter = (_adapter *)context;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	hwxmits = (struct hw_xmit *)_malloc(sizeof (struct hw_xmit) * HWXMIT_ENTRY);
#ifdef CONFIG_RTL8711
	struct SCSI_BUFFER *psb = padapter->pscsi_buf;
#endif
	

        pxmitpriv->vo_txqueue.head = 0;
	hwxmits[0] .phwtxqueue = &pxmitpriv->vo_txqueue;
	hwxmits[0] .sta_queue = &pxmitpriv->vo_pending;

	pxmitpriv->vi_txqueue.head = 0;
	hwxmits[1] .phwtxqueue = &pxmitpriv->vi_txqueue;
	hwxmits[1] .sta_queue = &pxmitpriv->vi_pending;

	pxmitpriv->be_txqueue.head = 0;
	hwxmits[2] .phwtxqueue = &pxmitpriv->be_txqueue;
	hwxmits[2] .sta_queue = &pxmitpriv->be_pending;
	
	pxmitpriv->bk_txqueue.head = 0;
	hwxmits[3] .phwtxqueue = &pxmitpriv->bk_txqueue;
	hwxmits[3] .sta_queue = &pxmitpriv->bk_pending;
	
#if defined(CONFIG_RTL8187)||defined(CONFIG_RTL8192U)

	hwxmits[4] .sta_queue = &pxmitpriv->mgnt_pending;

	hwxmits[5] .sta_queue = &pxmitpriv->beacon_pending;
	
#endif /*CONFIG_RTL8187*/

	daemonize_thread(padapter);
	pxmitpriv->hwxmits=hwxmits;
	while(1)
	{
		if ((_down_sema(&(pxmitpriv->xmit_sema))) == _FAIL)
		{
			DEBUG_INFO(("xmit thread : down xmit_sema failed\n"));
			if ((txframes_pending(padapter)) == _TRUE)
				DEBUG_ERR(("xmit_priv still has data but we can't down xmit_sema"));

			break;
		}
#ifdef CONFIG_POLLING_MODE
_xmit_next:
#endif
		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{
			DEBUG_ERR(("xmit thread => bDriverStopped or bSurpriseRemoved \n"));
			break;
		}

		if ((txframes_pending_nonbcmc(padapter)) == _FALSE)
		{
			DEBUG_INFO(("xmit thread => no pending data\n"));
			continue;
		}
			
		init_hwxmits(hwxmits, HWXMIT_ENTRY);
		
#ifdef CONFIG_RTL8711
		//Perry : check
		if (register_tx_alive(padapter) != _SUCCESS) 
		{
			DEBUG_ERR(("xmit thread => register_tx_alive; return xmit_thread\n"));
			continue;
		}

		pscsi_buf_entry = checkout_scsibuffer(padapter, psb);
		if (pscsi_buf_entry ==NULL) 
		{
			DEBUG_ERR(("not enough scsi buffer space psb_head = %d, psb_tail = %d\n", psb->head, psb->tail));
			continue;
		}
		DEBUG_INFO(("xmit thread : dequeue xmitframes\n"));
		dequeue_res = dequeue_xmitframes_direct(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
		if( dequeue_res <=0 )
		{
			DEBUG_INFO(("xmit thread: dequeue_res < =0\n"));
			continue;
		}
		dump_xmitframes(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
#endif /*CONFIG_RTL8711*/

#ifdef CONFIG_RTL8187		
		dequeue_res = dequeue_xmitframes_direct_87b(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
		if( dequeue_res <=0 )
			continue;

		dump_xmitframes_87b(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
#endif /*CONFIG_RTL8187*/
#ifdef CONFIG_RTL8192U

		dequeue_res = dequeue_xmitframes_direct_87b(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);
		if( dequeue_res <=0 )
		{
			DEBUG_INFO(("xmit_thread dequeue nothing!\n"));
			continue;
		}

		dump_xmitframes_8192(pxmitpriv, hwxmits, HWXMIT_ENTRY, pscsi_buf_entry);


#endif/*CONFIG_RTL8192U*/








		// Free all the dumpped xmit_frames below...
#ifndef CONFIG_USB_HCI
		free_xmitframes(pxmitpriv, hwxmits, HWXMIT_ENTRY);
#endif /*CONFIG_USB_HCI*/

		//Perry : check
		if ((txframes_pending_nonbcmc(padapter)) == _FALSE)
		{
			DEBUG_INFO(("xmit_thread unregister_tx_alive!\n"));
			unregister_tx_alive(padapter);
		}
		else
		{
#ifdef CONFIG_POLLING_MODE
			goto _xmit_next;			  	
#endif /*CONFIG_POLLING_MODE*/
		}

		flush_signals_thread();		
		
	}

	_mfree((u8 *)hwxmits, (sizeof (struct hw_xmit) * HWXMIT_ENTRY));
	_up_sema(&pxmitpriv->terminate_xmitthread_sema);

	DEBUG_INFO(("Exit xmit_thread\n"));

	exit_thread(NULL, 0);
}


#endif

