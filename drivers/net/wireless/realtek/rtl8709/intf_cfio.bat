attrib -r include\autoconf.h
copy autoconf_cfio.h include\autoconf.h

SET _BUS_LIB=
SET HCI=c
::=====os_intf\xp\======

SET _OS_INTFS_FILES=%_OS_INTFS_FILES% cfio_intf.c rtl8711c.rc 


::=====hal\rtl8711\======

SET _HAL_INTFS_FILES=%_HAL_INTFS_FILES% cfio_ops.c cfio_halinit.c


attrib -r os_intf\xp\sources
copy os_intf\xp\sources.cfinc os_intf\xp\sources_ddk