#
# Makefile for USB Core files and filesystem
#

usbcore-objs	:= usb.o hub.o hcd.o urb.o message.o \
			config.o file.o buffer.o sysfs.o

CFLAGS_hub.o	+= -mno-explicit-relocs -mno-split-addresses

ifeq ($(_CONFIG_PCI),y)
	usbcore-objs	+= hcd-pci.o
endif

#cfyeh+ 2005/11/07
ifeq ($(CONFIG_REALTEK_VENUS_USB),y)
	usbcore-objs	+= hcd-rbus.o
endif
#cfyeh- 2005/11/07

ifeq ($(CONFIG_USB_DEVICEFS),y)
	usbcore-objs	+= devio.o inode.o devices.o
endif

obj-$(CONFIG_USB)	+= usbcore.o

ifeq ($(CONFIG_USB_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

