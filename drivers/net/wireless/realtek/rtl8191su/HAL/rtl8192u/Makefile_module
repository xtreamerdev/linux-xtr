NIC_SELECT = RTL8192U
CC = gcc
KVER  := $(shell uname -r)
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/$(NIC_SELECT)

ifneq ($(shell uname -r|cut -d. -f1,2), 2.4)
EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -std=gnu89
EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -mhard-float -DCONFIG_FORCE_HARD_FLOAT=y
EXTRA_CFLAGS += -DJACKSON_NEW_8187 -DJACKSON_NEW_RX
EXTRA_CFLAGS += -DTHOMAS_BEACON -DTHOMAS_TASKLET -DTHOMAS_SKB -DTHOMAS_TURBO
#EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
#EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
EXTRA_CFLAGS += -DUSE_ONE_PIPE
EXTRA_CFLAGS += -DENABLE_DOT11D

r8192_usb-objs := r8192U_core.o r8180_93cx6.o r8192U_wx.o r8190_rtl8256.o r819xU_phy.o r819xU_firmware.o r819xU_cmdpkt.o r8192U_dm.o r819xU_firmware_img.o
obj-m := r8192_usb.o

KSRC := /lib/modules/$(KVER)/build
INSTALL_PREFIX :=
IMG_DIR := /lib/firmware/
all: modules

modules:
	$(MAKE) -C $(KSRC) M=$(PWD) CC=$(CC) modules

install: modules
	install -p -m 644 r8192_usb.ko  $(MODDESTDIR)
	@depmod -a
	@#copy firmware img to target fold
	$(shell [ -d "$(IMG_DIR)/`uname -r`" ] && cp -fr $(RTL819x_FIRM_DIR)/$(NIC_SELECT) $(IMG_DIR)/`uname -r`/.)
	$(shell [ ! -d "$(IMG_DIR)/`uname -r`" ] && cp -fr $(RTL819x_FIRM_DIR)/$(NIC_SELECT) $(IMG_DIR)/.)
	
uninstall:
	$(shell [ -d $(MODDESTDIR) ] && rm -fr $(MODDESTDIR))
	depmod -a
	#delete the firmware img
	rm -fr /lib/firmware/$(NIC_SELECT)
	rm -fr /lib/firmware/`uname -r`/$(NIC_SELECT)
else
LD := ld
KSRC := /lib/modules/$(KVER)/build
CONFIG_FILE := $(KSRC)/include/linux/autoconf.h
CFLAGS += -DLINUX -D__KERNEL__ -DMODULE -O2 -pipe -Wall
CFLAGS += -I$(KSRC)/include -I.
CFLAGS += -DMODVERSIONS -DEXPORT_SYMTAB -include $(KSRC)/include/linux/modversions.h
#CFLAGS += -D__NO_VERSION__ -DEXPORT_SYMTAB
CFLAGS += -DJACKSON_NEW_8187 -DJACKSON_NEW_RX
CFLAGS += -DTHOMAS_BEACON -DTHOMAS_TASKLET -DTHOMAS_SKB -DTHOMAS_TURBO
#CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
CFLAGS += -DUSE_ONE_PIPE
#CFLAGS += -DENABLE_DOT11D
SMP := $(shell $(CC) $(CFLAGS) -E -dM $(CONFIG_FILE) | \
   grep CONFIG_SMP | awk '{print $$3}')
ifneq ($(SMP),1)
   SMP := 0
endif
ifeq ($(SMP),1)
   CFLAGS    += -D__SMP__
endif

OBJS := ${patsubst %.c, %.o, ${wildcard *.c}}
                                                                                
all:r8192_usb.o
                                                                                
r8192_usb.o:${OBJS}
	$(LD) -r $^ -o $@
install: r8192_usb.o
	install -p -m 644 r8192_usb.o  $(MODDESTDIR)
	@depmod -a
uninstall:
	$(shell [ -d $(MODDESTDIR) ] && rm -fr $(MODDESTDIR))
	@depmod -a
endif

.PHONY:clean
clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -rf .tmp_versions
	rm -fr Modules.symvers
	rm -fr Module.symvers
