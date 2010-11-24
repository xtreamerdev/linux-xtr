
#ifndef _IRP_H
#define _IRP_H

#include <wlan.h>

extern struct rxirp *alloc_rxirp(unsigned int rxtag);
extern void free_rxirp(struct rxirp *irp);

extern struct txirp *alloc_txirp(_adapter *padapter);
struct txirp *alloc_txdatabuf(_adapter *padapter, struct txirp* irp);
extern void free_txirp(_adapter *padapter, struct txirp *irp);

#endif

