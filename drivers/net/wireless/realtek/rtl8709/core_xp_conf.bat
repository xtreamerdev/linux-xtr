@echo off

SET _WINBUILDROOT=%NTMAKEENV%
SET SRC=sources_ddk

attrib -r ..\os_dep\windows
copy ..\os_dep\windows\DIR_xp ..\os_dep\windows\DIR

::=====os_intf\xp\======

SET _OS_INTFS_FILES= 
SET _OS_INTFS_FILES=%_OS_INTFS_FILES%  ..\osdep_service.c  os_intfs.c xp_osintf.c


::=====hal_rtl8711\=====
SET _HAL_INTFS_FILES=
SET _HAL_INTFS_FILES=%_HAL_INTFS_FILES%  hal_init.c



::=====SET LIB=====
SET _DRV_LIB=..\..\cmd\i386\rtl871x_cmd.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\crypto\i386\rtl871x_crypto.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\mlme\i386\rtl871x_mlme.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\eeprom\i386\rtl871x_eeprom.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\rf\i386\rtl871x_rf.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\os_dep\windows\xp\i386\osdep_xp.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\io\i386\rtl871x_io.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\sta_mgt\i386\rtl871x_sta_mgt.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\pwrctrl\i386\rtl871x_pwrctrl.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\hal\RTL8711\i386\hal_8711.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\debug\i386\rtl871x_debug.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\recv\i386\rtl871x_recv.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\xmit\i386\rtl871x_xmit.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\mp\i386\rtl871x_mp.lib
SET _DRV_LIB=%_DRV_LIB% ..\..\ioctl\i386\rtl871x_ioctl.lib



