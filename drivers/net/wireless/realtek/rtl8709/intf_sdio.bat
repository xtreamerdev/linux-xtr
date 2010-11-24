attrib -r include\autoconf.h
copy autoconf_sdio.h include\autoconf.h

SET _BUS_LIB=$(DDK_LIB_PATH)\sdbus.lib
SET HCI=Sd
::=====os_intf\xp\======

SET _OS_INTFS_FILES=%_OS_INTFS_FILES%  sdio_intf.c rtl8711sd.rc 


::=====hal\rtl8711\======

SET _HAL_INTFS_FILES=%_HAL_INTFS_FILES% sdio_ops.c sdio_halinit.c sdio_ops_xp.c