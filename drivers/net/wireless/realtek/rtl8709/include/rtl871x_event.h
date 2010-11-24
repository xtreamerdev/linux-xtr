
#ifndef _RTL871x_EVENT_H_
#define _RTL871x_EVENT_H_
#include <drv_conf.h>
#include <osdep_service.h>

#ifndef CONFIG_RTL8711FW
#include <rtl871x_mlme.h>
#ifdef PLATFORM_LINUX

#include <wlan_bssdef.h>
#include <asm/semaphore.h>
#include <linux/sem.h>
#endif



#else

#include <wlan_bssdef.h>

#endif

#ifdef CONFIG_H2CLBK
#include <h2clbk.h>
#endif

/*
Used to report a bss has been scanned

*/
struct survey_event	{
	NDIS_WLAN_BSSID_EX bss;
};

/*
Used to report that the requested site survey has been done.

bss_cnt indicates the number of bss that has been reported.


*/
struct surveydone_event {
	unsigned int	bss_cnt;	
	
};

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
struct stassoc_event {
	unsigned char macaddr[6];
	unsigned char rsvd[2];
//	int	aid;
	
};

/* MARC 2006-1018
struct staassoc_event {
 unsigned char macaddr[6];
 unsigned char rsvd[2]; 
};
*/

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
	GEN_EVT_CODE(_DelSTA) ,	
#ifdef CONFIG_PWRCTRL
	GEN_EVT_CODE(_AtimDone),
#endif /*CONFIG_PWRCTRL*/	
#ifdef CONFIG_H2CLBK	
	GEN_EVT_CODE(_C2hLbk),
#endif
	MAX_C2HEVT
};



#ifndef CONFIG_RTL8711FW


	struct fwevent {
		u32	parmsize;
		void (*event_callback)(_adapter *dev, u8 *pbuf);
	};

#if 0	
	extern void survey_event_callback(unsigned char *dev, unsigned char *pbuf);
	extern void surveydone_event_callback(unsigned char *dev, unsigned char *pbuf);
	extern void joinbss_event_callback(unsigned char *dev, unsigned char *pbuf);
	extern int event_thread (void *data);
#endif

	#ifdef _RTL871X_CMD_C_
	
	#define GEN_FW_EVT_HANDLER(size, evt)	{size, &##evt##_callback},
	
	// TODO: Perry: should add 	_AtimDone event handler
	struct fwevent	wlanevents[] = {
			{0, &survey_event_callback},
		{sizeof (struct surveydone_event), &surveydone_event_callback},
		{0, &joinbss_event_callback},
		{sizeof(struct stassoc_event), &stassoc_event_callback},
		{sizeof(struct stadel_event), &stadel_event_callback},
#ifdef CONFIG_PWRCTRL
		{0, &atimdone_event_callback},
#endif /*CONFIG_PWRCTRL*/
	};
	#else
		extern struct fwevent  wlanevents[];
	#endif
#else


#define C2HEVENT_SZ			32	


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

#define NETWORK_QUEUE_SZ	4

struct	network_queue {
	volatile int	head;
	volatile int	tail;
	NDIS_WLAN_BSSID_EX networks[NETWORK_QUEUE_SZ];
};


extern void start_hw_event_posting(void);
extern void c2hclrdsr(_adapter *padapter);
extern int event_queuing (struct event_node *node);



#endif

#endif // _WLANEVENT_H_





