


#ifndef __OSDEP_LINUX_SERVICE_H_
#define __OSDEP_LINUX_SERVICE_H_

#include <drv_conf.h>
#include <basic_types.h>

#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <asm/semaphore.h>
#include <linux/sem.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <net/iw_handler.h>
	
#ifdef CONFIG_SDIO_HCI	
#include <linux/sdio/sdio_busdriver.h>
#endif

#define exit_thread(a,b) complete_and_exit(a,b);

#ifdef CONFIG_USB_HCI

typedef struct urb *  PURB;

#endif

typedef struct semaphore	_sema;
typedef	spinlock_t		_lock;
typedef struct semaphore	_rwlock;
typedef struct timer_list	_timer;

struct	__queue	{
	struct	list_head	queue;	
	_lock	lock;
};

typedef struct sk_buff		_pkt;
typedef struct __queue	_queue;
typedef struct list_head	_list;
typedef	int				_OS_STATUS;
typedef u32				_irqL;
typedef struct net_device * _nic_hdl;
	
typedef pid_t				_thread_hdl_;
typedef int				thread_return;
typedef void*			thread_context;


static __inline _list *get_next(_list	*list)
{
	return list->next;
}	

static __inline _list	*get_list_head(_queue	*queue)
{
	return (&(queue->queue));
}

	
#define LIST_CONTAINOR(ptr, type, member) \
        ((type *)((char *)(ptr)-(SIZE_T)(&((type *)0)->member)))	

        
static void __inline _enter_critical(_lock *plock, _irqL *pirqL)
{
	spin_lock_irqsave(plock, *pirqL);
}

static void __inline _exit_critical(_lock *plock, _irqL *pirqL)
{
	spin_unlock_irqrestore(plock, *pirqL);
}

static void __inline _enter_hwio_critical(_rwlock *prwlock, _irqL *pirqL)
{
		down(prwlock);
}

static void __inline _exit_hwio_critical(_rwlock *prwlock, _irqL *pirqL)
{
		up(prwlock);
}

static __inline void list_delete(_list *plist)
{
	list_del_init(plist);
}

static __inline void _init_timer(_timer *ptimer,_nic_hdl padapter,void *pfunc,PVOID cntx)
{
	ptimer->function = pfunc;
	ptimer->data = (u32)cntx;
	init_timer(ptimer);
}

static __inline void _set_timer(_timer *ptimer,u32 delay_time)
{	
	mod_timer(ptimer , (jiffies+(delay_time*HZ/1000)));	
}

static __inline void _cancel_timer(_timer *ptimer,u8 *bcancelled)
{
	del_timer_sync(ptimer); 	
	*bcancelled=  _TRUE;//TRUE ==1; FALSE==0
}

static __inline u8* _malloc(u32 sz)
{
	u8 	*pbuf;

	pbuf = 	kmalloc(sz, /*GFP_KERNEL*/GFP_ATOMIC);

	return pbuf;	
}

static __inline void	_mfree(u8 *pbuf, u32 sz)
{
	kfree(pbuf);
}

static __inline void _memcpy(void* dst, void* src, u32 sz)
{
	memcpy(dst, src, sz);
}

static __inline int	_memcmp(void *dst, void *src, u32 sz)
{
	if (!(memcmp(dst, src, sz)))
		return _TRUE;
	else
		return _FALSE;
}

static __inline void    _init_listhead(_list *list)
{
        INIT_LIST_HEAD(list);
}


/*
For the following list_xxx operations, 
caller must guarantee the atomic context.
Otherwise, there will be racing condition.
*/
static __inline u32	is_list_empty(_list *phead)
{
	if (list_empty(phead))
		return _TRUE;
	else
		return _FALSE;
}


static __inline void list_insert_tail(_list *plist, _list *phead)
{	
	list_add_tail(plist, phead);
}


/*

Caller must check if the list is empty before calling list_delete

*/


static __inline void _init_sema(_sema	*sema, int init_val)
{
	sema_init(sema, init_val);
}

static __inline void _up_sema(_sema	*sema)
{
	up(sema);
}

static __inline u32 _down_sema(_sema *sema)
{
	if (down_interruptible(sema))
		return _FAIL;
	else
		return _SUCCESS;
}

static __inline void	_rwlock_init(_rwlock *prwlock)
{
	init_MUTEX(prwlock);
}

static __inline void	_spinlock_init(_lock *plock)
{
	spin_lock_init(plock);
}

static __inline void	_spinlock(_lock	*plock)
{
	spin_lock(plock);	
}

static __inline void	_spinunlock(_lock *plock)
{
	spin_unlock(plock);
}

static __inline void	_init_queue(_queue	*pqueue)
{
	_init_listhead(&(pqueue->queue));

	_spinlock_init(&(pqueue->lock));
}

static __inline u32	  _queue_empty(_queue *pqueue)
{
	return (is_list_empty(&(pqueue->queue)));
}


static __inline u32 end_of_queue_search(_list *head, _list *plist)
{
	if (head == plist)
		return _TRUE;
	else
		return _FALSE;		
}


static __inline u32	get_current_time(void)
{
	return jiffies;
}

static __inline void sleep_schedulable(int ms)	
{
	u32 delta;
    
	delta = (ms * HZ)/1000;//(ms)
	if (delta == 0) {
		delta = 1;// 1 ms
	}
	set_current_state(TASK_INTERRUPTIBLE);
	if (schedule_timeout(delta) != 0) {
		return ;
	}
	return;
}

static __inline void msleep_os(int ms)
{
  	msleep((unsigned int)ms);
}

static __inline void usleep_os(int us)
{
       msleep((unsigned int)us);
}

static __inline void mdelay_os(int ms)
{
   	mdelay((unsigned long)ms);
}

static __inline void udelay_os(int us)
{
      udelay((unsigned long)us);
}

static __inline void daemonize_thread(void *context)
{
	struct net_device *pnetdev = (struct net_device *)context;
	daemonize("%s", pnetdev->name);
	allow_signal(SIGTERM);
}

static __inline void flush_signals_thread(void) 
{
	if (signal_pending (current)) 
	{
		flush_signals(current);
	}
}


#endif
