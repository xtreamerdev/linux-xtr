NIC_SELECT = RTL8190P

EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -DJACKSON_NEW_8187 -DJACKSON_NEW_RX

ifeq ($(NIC_SELECT),RTL8192U)
EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
endif

#flags to enable or disble 80211D feature
EXTRA_CFLAGS += -DENABLE_DOT11D


ieee80211-rtl-objs := ieee80211_rx.o \
	      ieee80211_softmac.o \
	      ieee80211_tx.o \
	      ieee80211_wx.o \
	      ieee80211_module.o \
	      ieee80211_softmac_wx.o\
	      rtl819x_HTProc.o\
	      rtl819x_TSProc.o\
	      rtl819x_BAProc.o\
	      dot11d.o

ieee80211_crypt-rtl-objs := ieee80211_crypt.o
ieee80211_crypt_tkip-rtl-objs := ieee80211_crypt_tkip.o
ieee80211_crypt_ccmp-rtl-objs := ieee80211_crypt_ccmp.o
ieee80211_crypt_wep-rtl-objs := ieee80211_crypt_wep.o

obj-$(CONFIG_REALTEK_PCI_RTL8190) +=ieee80211-rtl.o
obj-$(CONFIG_REALTEK_PCI_RTL8190) +=ieee80211_crypt-rtl.o
obj-$(CONFIG_REALTEK_PCI_RTL8190) +=ieee80211_crypt_wep-rtl.o
obj-$(CONFIG_REALTEK_PCI_RTL8190) +=ieee80211_crypt_tkip-rtl.o
obj-$(CONFIG_REALTEK_PCI_RTL8190) +=ieee80211_crypt_ccmp-rtl.o