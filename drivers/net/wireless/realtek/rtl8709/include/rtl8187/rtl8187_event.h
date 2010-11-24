
#ifndef _RTL8187_EVENT_H_
#define _RTL8187_EVENT_H_

#include <drv_conf.h>
#include <wlan_bssdef.h>
#include <drv_types.h>

#define C2HEVENT_SZ			32	

#define NETWORK_QUEUE_SZ	4

struct	network_queue {
	volatile int	head;
	volatile int	tail;
	NDIS_WLAN_BSSID_EX networks[NETWORK_QUEUE_SZ];
};

struct event_node	{
	unsigned char *node;
	unsigned char evt_code;
	unsigned short evt_sz;
	volatile int	*caller_ff_tail;
	int	caller_ff_sz;
};

struct c2hevent_queue {
	volatile int	head;
	volatile int	tail;
	struct	event_node	nodes[C2HEVENT_SZ];
	unsigned char	seq;
};

#define DEL_STA_QUEUE_SZ	4

struct	del_sta_queue {
	volatile int	head;
	volatile int	tail;
	struct stadel_event del_station[DEL_STA_QUEUE_SZ];
};





extern void start_hw_event_posting(_adapter *padapter);
extern void c2hclrdsr(_adapter *padapter);
extern int event_queuing (_adapter *padapter,struct event_node *node);

#if 0
#ifdef CONFIG_POWERSAVING
/*
Used to report driver that atim has alreay sent.

*/
struct atim_done_event	{
	unsigned int txid[2];
};
#endif

/*
Used to report the link result of joinning the given bss


join_res:
-1: authentication fail
-2: association fail
> 0: TID

*/
struct joinbss_event {
	struct	wlan_network	network;
};

/*
Used to report a given STA has joinned the created BSS.
It is used in AP/Ad-HoC(M) mode.


*/
struct staassoc_event {
	unsigned char macaddr[6];
	unsigned char rsvd[2];
	
};


struct stadel_event {
	unsigned char macaddr[6];
	unsigned char rsvd[2];
	
};

#ifdef CONFIG_H2CLBK

struct c2hlbk_event	{
	unsigned char mac[6];
	unsigned short	s0;
	unsigned short	s1;
	unsigned int	w0;
	unsigned char	b0;
	unsigned short  s2;
	unsigned char	b1;
	unsigned int	w1;	
};

#endif

#define GEN_EVT_CODE(event)	event ## _EVT_
enum rtl8711_h2c_event {
	GEN_EVT_CODE(_Survey) ,
	GEN_EVT_CODE(_SurveyDone) ,
	GEN_EVT_CODE(_JoinBss) ,
	GEN_EVT_CODE(_AddSTA),
	GEN_EVT_CODE(_DelSTA),
#ifdef CONFIG_POWERSAVING
	GEN_EVT_CODE(_AtimDone) ,
#endif
#ifdef CONFIG_H2CLBK	
	GEN_EVT_CODE(_C2hLbk),
#endif	
	MAX_C2HEVT
};



#ifndef CONFIG_RTL8711FW

#include <rtl8711_mlme.h>

`
	struct fwevent {
		unsigned int	parmsize;
		void (*event_callback)(unsigned char *dev, unsigned char *pbuf);
	};

#if 0	
	extern void survey_event_callback(unsigned char *dev, unsigned char *pbuf);
	extern void surveydone_event_callback(unsigned char *dev, unsigned char *pbuf);
	extern void joinbss_event_callback(unsigned char *dev, unsigned char *pbuf);
	extern int event_thread (void *data);
#endif

	#ifdef _RTL8711_CMD_C_
	
	#define GEN_FW_EVT_HANDLER(size, evt)	{size, &##evt##_callback},
	
	struct fwevent	wlanevents[] = {		
		{0, &survey_event_callback},
		{sizeof (struct surveydone_event), &surveydone_event_callback},
		{0, &joinbss_event_callback},
	#if 0	
		GEN_FW_EVT_HANDLER(0, survey_event) 
		GEN_FW_EVT_HANDLER(sizeof (struct surveydone_event), surveydone_event) 
		GEN_FW_EVT_HANDLER(sizeof (struct joinbss_event), joinbss_event) 
	#endif
	};
	#else
		extern struct fwevent  wlanevents[];
	#endif
#else


#define NETWORK_QUEUE_SZ	4

struct	network_queue {
	volatile int	head;
	volatile int	tail;
	NDIS_WLAN_BSSID_EX networks[NETWORK_QUEUE_SZ];
};


#define DEL_STA_QUEUE_SZ	4

struct	del_sta_queue {
	volatile int	head;
	volatile int	tail;
	struct stadel_event del_station[DEL_STA_QUEUE_SZ];
};



#endif

#endif

#endif // _WLANEVENT_H_





