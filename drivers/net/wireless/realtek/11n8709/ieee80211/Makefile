include $(src)/../Makefile.in
NIC_SELECT ?= RTL8192U
#NIC_SELECT = RTL8192SU
DRIVER_RELEASE_MODE ?=
MODULE_SUFFIX ?=
EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -DFUNC_SUFFIX=$(MODULE_SUFFIX)
#it will fail to compile in suse linux enterprise 10 sp2. This flag is to solve this problem.
ifeq ($(shell uname -r | cut -d. -f1,2,3,4), 2.6.16.60-0)
EXTRA_CFLAGS += -DOPENSUSE_SLED
endif

ifeq ($(NIC_SELECT), RTL8192SU)
	EXTRA_CFLAGS += -DRTL8192S_DISABLE_FW_DM=0
	EXTRA_CFLAGS += -DRTL8192SU
	EXTRA_CFLAGS += -DENABLE_IPS
	EXTRA_CFLAGS += -DENABLE_LPS
	EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
	EXTRA_CFLAGS += -DTCP_CSUM_OFFLOAD_RX
endif

ifeq ($(NIC_SELECT),RTL8192U)
EXTRA_CFLAGS += -DRTL8192U
#EXTRA_CFLAGS += -DENABLE_AMSDU
EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT

endif
#
# For RTK DMP platform
EXTRA_CFLAGS += -DRTK_DMP_PLATFORM
#EXTRA_CFLAGS += -DWIFI_TEST

#EXTRA_CFLAGS += -DJOHN_NOCPY
EXTRA_CFLAGS += -DTHOMAS_TURBO
#flags to enable or disble 80211D feature
EXTRA_CFLAGS += -DENABLE_DOT11D
ieee80211-$(MODULE_SUFFIX)-objs := ieee80211_rx.o \
		      ieee80211_softmac.o \
		      ieee80211_tx.o \
		      ieee80211_wx.o \
		      ieee80211_module.o \
		      ieee80211_softmac_wx.o\
		      rtl819x_HTProc.o\
		      rtl819x_TSProc.o\
		      rtl819x_BAProc.o\
		      dot11d.o



ieee80211_crypt-$(MODULE_SUFFIX)-objs := ieee80211_crypt.o
ieee80211_crypt_tkip-$(MODULE_SUFFIX)-objs := ieee80211_crypt_tkip.o
ieee80211_crypt_ccmp-$(MODULE_SUFFIX)-objs := ieee80211_crypt_ccmp.o
ieee80211_crypt_wep-$(MODULE_SUFFIX)-objs := ieee80211_crypt_wep.o

obj-$(DRIVER_RELEASE_MODE) +=ieee80211-$(MODULE_SUFFIX).o
obj-$(DRIVER_RELEASE_MODE) +=ieee80211_crypt-$(MODULE_SUFFIX).o
obj-$(DRIVER_RELEASE_MODE) +=ieee80211_crypt_wep-$(MODULE_SUFFIX).o
obj-$(DRIVER_RELEASE_MODE) +=ieee80211_crypt_tkip-$(MODULE_SUFFIX).o
obj-$(DRIVER_RELEASE_MODE) +=ieee80211_crypt_ccmp-$(MODULE_SUFFIX).o

clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.mod.* *.ko *.o *~
	rm -rf .tmp_versions
	rm -rf Module.symvers

