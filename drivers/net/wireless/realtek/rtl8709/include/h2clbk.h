


#define _H2CLBK_H_


#include <rtl8711_spec.h>
#include <TypeDef.h>


void _lbk_cmd(PADAPTER Adapter);

void _lbk_rsp(PADAPTER Adapter);

void _lbk_evt(IN PADAPTER Adapter);

void h2c_event_callback(unsigned char *dev, unsigned char *pbuf);
