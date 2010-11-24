attrib -r include\autoconf.h
copy autoconf_ce_sdio.h include\autoconf.h
attrib -r os_intf\windows\sources
copy os_intf\windows\sources_ce.sdinc os_intf\windows\sources
set _DRIVER_ROOT=%cd%

set SRC=sources_ce

set wincerel=1
set DRIVER_NAME=RTL8711
set _COMMONOAKROOT=%_WINCEROOT%\PUBLIC\COMMON\OAK
set _COMMONSDKROOT=%_WINCEROOT%\PUBLIC\COMMON\SDK
set _DRIVER_TARGET_DIR=$(_COMMONOAKROOT)\lib\$(_TGTCPU)\$(WINCEDEBUG)
set _WINBUILDROOT=%_MAKEENVROOT%

set _DRIVER_LIBS=
set _DRIVER_LIBS=%_DRIVER_LIBS% %_DRIVER_TARGET_DIR%\%DRIVER_NAME%_debug_%WINCEDEBUG%.lib
set _DRIVER_LIBS=%_DRIVER_LIBS% %_DRIVER_TARGET_DIR%\%DRIVER_NAME%_os_intf_%WINCEDEBUG%.lib
set _DRIVER_LIBS=%_DRIVER_LIBS% %_DRIVER_TARGET_DIR%\%DRIVER_NAME%_sta_mgt_%WINCEDEBUG%.lib

if %_TGTCPU% == x86 set X86_CPU=1

set _BUILD_MODULE_DEF=%_BUILD_MODULE_DEF% -DWINDOWS_CE
set _BUILD_MODULE_DEF= %_BUILD_MODULE_DEF% -DX86_CPU=%X86_CPU%

:_WINCE50
set _INCLUDES_DIRS=
set _INCLUDES_DIRS=%_COMMONOAKROOT%\inc;%_COMMONSDKROOT%\inc;%_DRIVER_ROOT%\include;
REM set _INCLUDES_DIRS=%_DRIVER_ROOT%\include;
REM Setup Library CE5.0 different from CE 4.2
set _WINCE_LIBS=%_PROJECTROOT%\cesysgen\sdk\lib\%_TGTCPU%\%WINCEDEBUG%\ndis.lib
set _WINCE_LIBS=%_WINCE_LIBS% %_PROJECTROOT%\cesysgen\sdk\lib\%_TGTCPU%\%WINCEDEBUG%\coredll.lib
set _WINCE_LIBS=%_WINCE_LIBS% %_PROJECTROOT%\cesysgen\oak\lib\%_TGTCPU%\%WINCEDEBUG%\ceddk.lib

set _SDIO_LIBS=%_PROJECTROOT%\cesysgen\oak\lib\%_TGTCPU%\%WINCEDEBUG%\sdcardlib.lib %_PROJECTROOT%\cesysgen\oak\lib\%_TGTCPU%\%WINCEDEBUG%\sdbus.lib
set _WINCE_LIBS=%_WINCE_LIBS% %_SDIO_LIBS%

set _DRIVER_TARGET_LIBS=%_DRIVER_LIBS% %_WINCE_LIBS%

@build -c


set DEVICE_ID=
set DRIVER_ID=
set DRIVER_NAME=
set mkdriver_target=
set BUILD_OPTIONS=
set mkdriver_driver_path=
