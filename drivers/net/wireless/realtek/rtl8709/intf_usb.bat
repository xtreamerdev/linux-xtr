attrib -r include\autoconf.h
copy autoconf_usb.h include\autoconf.h

SET _BUS_LIB=$(DDK_LIB_PATH)\usbd.lib
SET HCI=u
::=====os_intf\xp\======

SET _OS_INTFS_FILES=%_OS_INTFS_FILES% usb_intf.c rtl8711u.rc 

::=====hal\rtl8711\======

SET _HAL_INTFS_FILES=%_HAL_INTFS_FILES% usb_ops.c usb_ops_xp.c usb_halinit.c
