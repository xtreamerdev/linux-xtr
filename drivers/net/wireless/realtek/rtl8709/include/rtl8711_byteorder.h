#ifndef _RTL8711_BYTEORDER_H_
#define _RTL8711_BYTEORDER_H_


#if defined (CONFIG_LITTLE_ENDIAN)
#  include <byteorder/little_endian.h>
#elif defined (CONFIG_BIG_ENDIAN)
#  include <byteorder/big_endian.h>
#else
#  error "Must be LITTLE/BIG Endian Host"
#endif

#endif /* _RTL8711_BYTEORDER_H_ */
