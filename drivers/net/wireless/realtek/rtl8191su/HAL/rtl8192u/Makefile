include $(src)/../../Makefile.in

NIC_SELECT ?= RTL8192U
#NIC_SELECT ?= RTL8192SU
DRIVER_RELEASE_MODE ?=
MODULE_SUFFIX ?=

EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -std=gnu89
EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -mhard-float -DCONFIG_FORCE_HARD_FLOAT=y

ifeq ($(NIC_SELECT),RTL8192SU)
EXTRA_CFLAGS += -DJACKSON_NEW_RX
EXTRA_CFLAGS += -DTHOMAS_BEACON -DTHOMAS_TURBO
#EXTRA_CFLAGS += -DUSE_ONE_PIPE
EXTRA_CFLAGS += -DENABLE_DOT11D

EXTRA_CFLAGS += -DRTL8192SU
EXTRA_CFLAGS += -DRTL8190_Download_Firmware_From_Header=1
EXTRA_CFLAGS += -DRTL8192S_PREPARE_FOR_NORMAL_RELEASE
EXTRA_CFLAGS += -DRTL8192SU_DISABLE_IQK=1

#EXTRA_CFLAGS += -DEEPROM_OLD_FORMAT_SUPPORT

EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
#EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE=0
#EXTRA_CFLAGS += -DRTL8192SU_DISABLE_CCK_RATE=0
EXTRA_CFLAGS += -DRTL8192S_DISABLE_FW_DM=0 
EXTRA_CFLAGS += -DDISABLE_BB_RF=0 
EXTRA_CFLAGS += -DRTL8192SU_USE_PARAM_TXPWR=0
EXTRA_CFLAGS += -DRTL8192SU_FPGA_UNSPECIFIED_NETWORK=0
EXTRA_CFLAGS += -DRTL8192SU_FPGA_2MAC_VERIFICATION=0
EXTRA_CFLAGS += -DRTL8192SU_ASIC_VERIFICATION=1
EXTRA_CFLAGS += -DRTL8192SU_USB_PHY_TEST=0

#EXTRA_CFLAGS += -DMUTIPLE_BULK_OUT
EXTRA_CFLAGS += -DCONFIG_RTL8192_PM
EXTRA_CFLAGS += -DENABLE_IPS
EXTRA_CFLAGS += -DENABLE_LPS

EXTRA_CFLAGS += -DTCP_CSUM_OFFLOAD_RX
#EXTRA_CFLAGS += -DCONFIG_MP_TOOL
#EXTRA_CFLAGS += -DBUILT_IN_CRYPTO
endif

ifeq ($(NIC_SELECT),RTL8192U)
EXTRA_CFLAGS += -DRTL8192U
EXTRA_CFLAGS += -DTHOMAS_TURBO
EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
EXTRA_CFLAGS += -DUSB_AGGR_BULKIN
EXTRA_CFLAGS += -DUSE_ONE_PIPE
#EXTRA_CFLAGS += -DWIFI_TEST # The USE_ONE_PIPE MUST NOT be defined if WIFI_TEST is defined. 
EXTRA_CFLAGS += -DENABLE_DOT11D
EXTRA_CFLAGS += -DCONFIG_RTL8192_PM
#EXTRA_CFLAGS += -DENABLE_AMSDU


endif

EXTRA_CFLAGS += -DUSE_FW_SOURCE_HEADER_FILE
#EXTRA_CFLAGS += -DBUILT_IN_IEEE80211
#EXTRA_CFLAGS += -DWIFI_TEST

#
# For RTK DMP platform
EXTRA_CFLAGS += -DRTK_DMP_PLATFORM
EXTRA_CFLAGS += -DENABLE_TX_ISR_TASKLET

ifeq ($(NIC_SELECT),RTL8192SU)
r8192s_usb-objs := r8180_93cx6.o r8192U_wx.o r8192S_phy.o r8192S_rtl6052.o r8192S_rtl8225.o r819xU_cmdpkt.o r8192U_dm.o r8192SU_HWImg.o r8192S_firmware.o r8192S_Efuse.o r8192U_core.o r8192U_pm.o r8192SU_led.o 
# BUILT_IN_IEEE80211
#r8192s_usb-objs += \
        ../../ieee80211/ieee80211_rx.o                \
        ../../ieee80211/ieee80211_softmac.o           \
        ../../ieee80211/ieee80211_tx.o                \
        ../../ieee80211/ieee80211_wx.o                \
        ../../ieee80211/ieee80211_module.o            \
        ../../ieee80211/ieee80211_softmac_wx.o        \
        ../../ieee80211/rtl819x_HTProc.o              \
        ../../ieee80211/rtl819x_TSProc.o              \
        ../../ieee80211/rtl819x_BAProc.o              \
        ../../ieee80211/dot11d.o                      \
        ../../ieee80211/ieee80211_crypt.o             \
        ../../ieee80211/ieee80211_crypt_tkip.o        \
        ../../ieee80211/ieee80211_crypt_ccmp.o        \
        ../../ieee80211/ieee80211_crypt_wep.o
#r8192s_usb-objs        += r8192u_mp.o

# start of BUILT_IN_CRYPTO
#r8192s_usb-objs += \
        ../../ieee80211/api.o                   \
        ../../ieee80211/compress.o              \
        ../../ieee80211/digest.o                \
        ../../ieee80211/cipher.o                \
        ../../ieee80211/scatterwalk.o           \
        ../../ieee80211/arc4.o                  \
        ../../ieee80211/michael_mic.o           \
        ../../ieee80211/aes.o

obj-$(DRIVER_RELEASE_MODE) := r8192s_usb.o
else
r8192_usb-objs := r8192U_core.o r8180_93cx6.o r8192U_wx.o r8190_rtl8256.o r819xU_phy.o r819xU_firmware.o r819xU_cmdpkt.o r8192U_dm.o r819xU_firmware_img.o r8192U_pm.o
# BUILT_IN_IEEE80211
#r8192_usb-objs += \
        ../../ieee80211/ieee80211_rx.o                \
        ../../ieee80211/ieee80211_softmac.o           \
        ../../ieee80211/ieee80211_tx.o                \
        ../../ieee80211/ieee80211_wx.o                \
        ../../ieee80211/ieee80211_module.o            \
        ../../ieee80211/ieee80211_softmac_wx.o        \
        ../../ieee80211/rtl819x_HTProc.o              \
        ../../ieee80211/rtl819x_TSProc.o              \
        ../../ieee80211/rtl819x_BAProc.o              \
        ../../ieee80211/dot11d.o                      \
        ../../ieee80211/ieee80211_crypt.o             \
        ../../ieee80211/ieee80211_crypt_tkip.o        \
        ../../ieee80211/ieee80211_crypt_ccmp.o        \
        ../../ieee80211/ieee80211_crypt_wep.o

obj-$(DRIVER_RELEASE_MODE) := r8192_usb.o
endif

clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -rf .tmp_versions
	rm -fr Modules.symvers
	rm -fr Module.symvers
