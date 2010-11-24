#
# Makefile for DWC_otg Highspeed USB controller driver
#

ifneq ($(KERNELRELEASE),)
CPPFLAGS	+= -DDEBUG 

# Use one of the following flags to compile the software in host-only or
# device-only mode.
ifeq ($(CONFIG_USB_OTG_REALTEK_HOST_ONLY_MODE),y)
	CPPFLAGS        += -DDWC_HOST_ONLY
endif
	
ifeq ($(CONFIG_USB_OTG_REALTEK_DEVICE_ONLY_MODE),y)	
	CPPFLAGS        += -DDWC_DEVICE_ONLY
endif

ifeq ($(CONFIG_USB_OTG_REALTEK_TEST_SUPPORT),y)
	CPPFLAGS	+= -Dlinux -DDWC_HS_ELECT_TST
endif

obj-y	:= dwc_otg.o lm.o floatlib.o
#obj-$(CONFIG_USB_OTG_REALTEK) := dwc_otg.o floatlib.o
#obj-y  += lm.o

dwc_otg-objs	:= dwc_otg_driver.o dwc_otg_attr.o 
dwc_otg-objs	+= dwc_otg_cil.o dwc_otg_cil_intr.o 
dwc_otg-objs	+= dwc_otg_pcd.o dwc_otg_pcd_intr.o 
dwc_otg-objs	+= dwc_otg_hcd.o dwc_otg_hcd_intr.o dwc_otg_hcd_queue.o

else

KDIR		:= /home/cyhuang/Mars/linux-2.6.12-mars-fpga-rev.115974
PWD		:= $(shell pwd)
CROSS_COMPILE 	:= /usr/bin/mipsel-linux-
ARCH 		:= mips

# Command paths
CTAGS		:= /usr/bin/ctags
DOXYGEN		:= /usr/bin/doxygen

default:
	$(MAKE) -C$(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

docs:	$(wildcard *.[hc]) doc/doxygen.cfg
	$(DOXYGEN) doc/doxygen.cfg

tags:	$(wildcard *.[hc])
	$(CTAGS) -e $(wildcard *.[hc]) $(wildcard linux/*.[hc]) $(wildcard $(KDIR)/include/linux/usb*.h)

endif

clean:
	rm -rf   *.o *.ko .*cmd *.mod.c .tmp_versions
