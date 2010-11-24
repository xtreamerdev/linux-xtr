


#ifndef __OSDEP_XP_SERVICE_H_
#define __OSDEP_XP_SERVICE_H_

#include <drv_conf.h>
#include <basic_types.h>

#include <ndis.h>
#include <ntddk.h>
#include <ntddsd.h>
#include <ntddndis.h>
#include <ntdef.h>
	
#define exit_thread(a,b) 	PsTerminateSystemThread(STATUS_SUCCESS);

typedef KSEMAPHORE		_sema;
typedef	LIST_ENTRY		_list;
typedef NDIS_STATUS		_OS_STATUS;
	
typedef NDIS_SPIN_LOCK	_lock;

typedef KMUTEX			_rwlock;
	
typedef KIRQL			_irqL;
typedef NDIS_HANDLE		_nic_hdl;

typedef NDIS_MINIPORT_TIMER	_timer;

struct	__queue	{
	LIST_ENTRY	queue;	
	_lock	lock;
};

typedef NDIS_PACKET		_pkt;
typedef NDIS_BUFFER		_buffer;
typedef struct __queue	_queue;
	
typedef PKTHREAD		_thread_hdl_;
typedef void				thread_return;
typedef void*			thread_context;

#define SEMA_UPBND	(0x7FFFFFFF)   //8192
	
static __inline _list *get_next(_list	*list)
{
	return list->Flink;
}	

static __inline _list	*get_list_head(_queue	*queue)
{
	return (&(queue->queue));
}

#define LIST_CONTAINOR(ptr, type, member) CONTAINING_RECORD(ptr, type, member)

static __inline _enter_critical(_lock *plock, _irqL *pirqL)
{

	NdisAcquireSpinLock(plock);
	
}

static __inline _exit_critical(_lock *plock, _irqL *pirqL)
{
	NdisReleaseSpinLock(plock);	
}

static __inline _enter_hwio_critical(_rwlock *prwlock, _irqL *pirqL)
{
#ifndef CONFIG_CFIO_HCI	
	KeWaitForSingleObject(prwlock, Executive, KernelMode, FALSE, NULL);
#endif	
}

static __inline _exit_hwio_critical(_rwlock *prwlock, _irqL *pirqL)
{
#ifndef CONFIG_CFIO_HCI	
	KeReleaseMutex(prwlock, FALSE);
#endif
}

static __inline void list_delete(_list *plist)
{
	RemoveEntryList(plist);
	InitializeListHead(plist);
}

static __inline void _init_timer(_timer *ptimer,_nic_hdl padapter,void *pfunc,PVOID cntx)
{
	NdisMInitializeTimer(ptimer, padapter, pfunc, cntx);
}

static __inline void _set_timer(_timer *ptimer,u32 delay_time)
{	
 	NdisMSetTimer(ptimer,delay_time);	
}

static __inline void _cancel_timer(_timer *ptimer,u8 *bcancelled)
{
	NdisMCancelTimer(ptimer,bcancelled);
}

#define RT_TAG	'1178'

static __inline u8* _malloc(u32 sz)
{
	u8 	*pbuf;

	NdisAllocateMemoryWithTag(&pbuf,sz, RT_TAG);

	return pbuf;	
}

static __inline void	_mfree(u8 *pbuf, u32 sz)
{
	NdisFreeMemory(pbuf,sz, 0);
}

static __inline void _memcpy(void* dst, void* src, u32 sz)
{
	NdisMoveMemory(dst, src, sz);
}

static __inline int	_memcmp(void *dst, void *src, u32 sz)
{
	if (NdisEqualMemory (dst, src, sz))
		return _TRUE;
	else
		return _FALSE;
}

static __inline void    _init_listhead(_list *list)
{
	NdisInitializeListHead(list);
}


/*
For the following list_xxx operations, 
caller must guarantee the atomic context.
Otherwise, there will be racing condition.
*/
static __inline u32	is_list_empty(_list *phead)
{
	if (IsListEmpty(phead))
		return _TRUE;
	else
		return _FALSE;
}

static __inline void list_insert_tail(_list *plist, _list *phead)
{
	InsertTailList(phead, plist);
}


/*

Caller must check if the list is empty before calling list_delete

*/

static __inlinevoid _init_sema(_sema	*sema, int init_val)
{
	KeInitializeSemaphore(sema, init_val,  SEMA_UPBND); // count=0;
}

static __inline void _up_sema(_sema	*sema)
{
	KeReleaseSemaphore(sema, IO_NETWORK_INCREMENT, 1,  FALSE );
}

static __inline u32 _down_sema(_sema *sema)
{
	if(STATUS_SUCCESS == KeWaitForSingleObject(sema, Executive, KernelMode, TRUE, NULL))
		return  _SUCCESS;
	else
		return _FAIL;
}

static __inline void	_rwlock_init(_rwlock *prwlock)
{
	KeInitializeMutex(prwlock, 0);
}

static __inline void	_spinlock_init(_lock *plock)
{
	NdisAllocateSpinLock(plock);
}

static __inline void	_spinlock(_lock	*plock)
{
	NdisAcquireSpinLock(plock);
}

static __inline void	_spinunlock(_lock *plock)
{
	NdisReleaseSpinLock(plock);
}

static __inline void	_init_queue(_queue	*pqueue)
{
	_init_listhead(&(pqueue->queue));

	_spinlock_init(&(pqueue->lock));
}

static __inline u32	  _queue_empty(_queue	*pqueue)
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
	LARGE_INTEGER	SystemTime;
	NdisGetCurrentSystemTime(&SystemTime);
	return (u32)(SystemTime.LowPart);
}

static __inline void sleep_schedulable(int ms)	
{
	NdisMSleep(ms*1000); //(us)*1000=(ms)
}

static __inline void msleep_os(int ms)
{
	NdisMSleep(ms*1000); //(us)*1000=(ms)
}

static __inline void usleep_os(int us)
{
	NdisMSleep(us); //(us)
}

static __inline void mdelay_os(int ms)
{
	NdisStallExecution(ms*1000); //(us)*1000=(ms)
}

static __inline void udelay_os(int us)
{
	NdisStallExecution(us); //(us)
}

static __inline void daemonize_thread(void *context)
{

}

static __inline void flush_signals_thread() 
{
	
}

#endif