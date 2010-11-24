#ifndef __IOCTL_QUERY_H
#define __IOCTL_QUERY_H

#include <drv_conf.h>
#include <drv_types.h>

//SecurityType.h (include\os_dep)
#define		MAX_CIPHER_SUITE_NUM		4
#define		MAX_AUTH_SUITE_NUM	4






/*! \enum _RT_JOIN_NETWORKTYPE
	\brief The network type.
*/
typedef enum _RT_JOIN_NETWORKTYPE{
	RT_JOIN_NETWORKTYPE_INFRA = 1,
	RT_JOIN_NETWORKTYPE_ADHOC = 2,
	RT_JOIN_NETWORKTYPE_AUTO  = 3,
}RT_JOIN_NETWORKTYPE;


u8 query_802_11_capability(_adapter*	padapter,u8*	pucBuf,u32 *	pulOutLen);
#ifdef PLATFORM_WINDOWS
u8 query_802_11_association_information (_adapter * padapter, PNDIS_802_11_ASSOCIATION_INFORMATION pAssocInfo);
#endif





#endif // #ifndef __INC_MGNTACTQUERYPARAM_H
