
#define _IRP_MISC_C_
#include <drv_types.h>



void free_rxirp(struct rxirp *irp)
{
	unsigned int flags;
	_adapter *padapter = irp->adapter;
	struct mib_info *pmibinfo = &(padapter->_sys_mib);

	_enter_critical(&(pmibinfo->free_rxirp_mgt_lock), &flags);
	list_insert_tail(&(irp->list), &(pmibinfo->free_rxirp_mgt));
	memset((u8*)irp->rxbuf_phyaddr, 0 , RXMGTBUF_SZ);
	irp->rx_len = 0;
	_exit_critical(&(pmibinfo->free_rxirp_mgt_lock), &flags);
}


/*
0	struct	list_head	free_txirp_mgt;
1	struct	list_head	free_txirp_small;
2	struct	list_head	free_txirp_medium;
3	struct	list_head	free_txirp_large;
*/
struct txirp *alloc_txirp(_adapter *padapter)
{
	_irqL flags;
	struct txirp *irp = NULL;
	struct mib_info *pmibinfo = &(padapter->_sys_mib);
	_list *head = &(pmibinfo->free_txirp_mgt);

	_enter_critical(&(pmibinfo->free_txirp_mgt_lock), &flags);

	if(is_list_empty(head)==_TRUE)
		goto ret;

	irp = LIST_CONTAINOR(head->next, struct txirp, list);
	list_delete(head->next);

	memset( (u8*)irp->txbuf_phyaddr, 0, TXMGTBUF_SZ);
	irp->tx_len = 0;
ret:
	_exit_critical(&(pmibinfo->free_txirp_mgt_lock), &flags);

	return irp;
}

void free_txirp(_adapter *padapter, struct txirp *irp)
{
	unsigned int flags;
	struct mib_info *pmibinfo = &(padapter->_sys_mib);
	
	_enter_critical(&(pmibinfo->free_txirp_mgt_lock), &flags);
	list_insert_tail(&(irp->list), &(pmibinfo->free_txirp_mgt));
	_exit_critical(&(pmibinfo->free_txirp_mgt_lock), &flags);
}

#if 0
#include <config.h>
#include <sched.h>
#include <types.h>

#include <console.h>
#include <system32.h>
#include <list.h>
#include <timer.h>
#include <irp.h>
#include <wlan.h>


//extern struct mib_info sys_mib;

/*
0	struct	list_head	free_rxirp_mgt;
1	struct	list_head	free_rxirp_small;
2	struct	list_head	free_rxirp_medium;
3	struct	list_head	free_rxirp_large;
*/
struct rxirp *alloc_rxirp(unsigned int rxtag)
{
	unsigned int flags;
	struct rxirp *irp = NULL;
	struct list_head *head = &(sys_mib.free_rxirp_mgt) + rxtag;

	local_irq_save(flags);

	if(list_empty(head))
		goto ret;

	irp = list_entry(head->next, struct rxirp, list);
	list_del_init(head->next);

ret:
	local_irq_restore(flags);
	return irp;
}

void free_rxirp(struct rxirp *irp)
{
	unsigned int flags;

	local_irq_save(flags);
	list_add(&(irp->list), &(sys_mib.free_rxirp_mgt) + irp->tag);
	local_irq_restore(flags);
}

struct txirp *alloc_txdatabuf(unsigned int sz)
{
	struct txirp *irp = NULL;
#if 0	
	if (sz <= TXDATAIRPS_SMALL_SZ)
		irp = alloc_txirp(TXDATAIRPS_SMALL_TAG);
	else if (sz <= TXDATAIRPS_MEDIUM_SZ)
		irp = alloc_txirp(TXDATAIRPS_MEDIUM_TAG);
	else
		irp = alloc_txirp(TXDATAIRPS_LARGE_TAG);
#endif
	return irp;
}


void free_txirp(struct txirp *irp)
{
	unsigned int flags;

	local_irq_save(flags);
	list_add(&(irp->list), &(sys_mib.free_txirp_mgt) + irp->tag);
	local_irq_restore(flags);
}

#endif
