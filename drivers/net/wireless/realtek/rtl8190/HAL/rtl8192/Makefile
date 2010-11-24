NIC_SELECT = RTL8190P

ifeq ($(NIC_SELECT),RTL8190P)
  EXTRA_CFLAGS += -DRTL8190P  
  r8190_pci-objs := r8192E_core.o r8180_93cx6.o r8192E_wx.o r8190_rtl8256.o r819xE_phy.o r819xE_firmware.o r819xE_cmdpkt.o r8192E_dm.o r8192_pm.o
  obj-$(CONFIG_REALTEK_PCI_RTL8190) = r8190_pci.o  
endif

ifeq ($(NIC_SELECT),RTL8192E)
  EXTRA_CFLAGS+=-DRTL8192E
  r8192_pci-objs := r8192E_core.o r8180_93cx6.o r8192E_wx.o r8190_rtl8256.o r819xE_phy.o r819xE_firmware.o r819xE_cmdpkt.o r8192E_dm.o
  obj-$(CONFIG_REALTEK_PCI_RTL8190):= r8192_pci.o
endif

EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -std=gnu89
EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -mhard-float -DCONFIG_FORCE_HARD_FLOAT=y
EXTRA_CFLAGS += -DENABLE_DOT11D









