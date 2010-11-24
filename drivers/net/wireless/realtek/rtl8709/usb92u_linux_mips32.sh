#!/bin/bash

set +v
rm -f cmd/i386 -r
rm -f cmd/objchk_wxp_x86 -r
rm -f cmd/objfre_wxp_x86 -r
rm -f crypto/i386 -r
rm -f crypto/objchk_wxp_x86 -r
rm -f crypto/objfre_wxp_x86 -r
rm -f debug/i386 -r
rm -f debug/objchk_wxp_x86 -r
rm -f debug/objfre_wxp_x86 -r
rm -f eeprom/i386 -r
rm -f eeprom/objchk_wxp_x86 -r
rm -f eeprom/objfre_wxp_x86 -r
rm -f hal/RTL8711/i386 -r
rm -f hal/RTL8711/objchk_wxp_x86 -r
rm -f hal/RTL8711/objfre_wxp_x86 -r
rm -f io/i386 -r
rm -f io/objchk_wxp_x86 -r
rm -f io/objfre_wxp_x86 -r
rm -f ioctl/i386 -r
rm -f ioctl/objchk_wxp_x86 -r
rm -f ioctl/objfre_wxp_x86 -r
rm -f mlme/i386 -r
rm -f mlme/objchk_wxp_x86 -r
rm -f mlme/objfre_wxp_x86 -r
rm -f mp/i386 -r
rm -f mp/objchk_wxp_x86 -r
rm -f mp/objfre_wxp_x86 -r
rm -f os_dep/windows/xp/i386 -r
rm -f os_dep/windows/xp/objchk_wxp_x86 -r
rm -f os_dep/windows/xp/objfre_wxp_x86 -r
rm -f os_intf/windows/objchk_wxp_x86 -r
rm -f os_intf/windows/objfre_wxp_x86 -r
rm -f pwrctrl/i386 -r
rm -f pwrctrl/objchk_wxp_x86 -r
rm -f pwrctrl/objfre_wxp_x86 -r
rm -f recv/i386 -r
rm -f recv/objchk_wxp_x86 -r
rm -f recv/objfre_wxp_x86 -r
rm -f rf/i386 -r
rm -f rf/objchk_wxp_x86 -r
rm -f rf/objfre_wxp_x86 -r
rm -f xmit/i386 -r
rm -f xmit/objchk_wxp_x86 -r
rm -f xmit/objfre_wxp_x86 -r
rm -f sta_mgt/i386 -r
rm -f sta_mgt/objchk_wxp_x86 -r
rm -f sta_mgt/objfre_wxp_x86 -r
rm -f objchk_wxp_x86 -r
rm -f objfre_wxp_x86 -r
echo ""
echo ">>Removed all object files complied."
echo ""
set -v

cp Makefile_linux_mips32_usb_92u Makefile

make ARCH=mips CROSS_COMPILE=mipsel-linux- 

