dvb-dibusb-objs = dvb-dibusb-core.o \
	dvb-dibusb-dvb.o \
	dvb-dibusb-fe-i2c.o \
	dvb-dibusb-firmware.o \
	dvb-dibusb-remote.o \
	dvb-dibusb-usb.o \
	dvb-fe-dtt200u.o \
	gl861-fe-i2c.o	\
	rtl2831u-fe-i2c.o	\
	rtd2830_cypress-fe-i2c.o	\
	qt1010.o	\
	thomson_fe664x.o \
	thomson_ddt7680x.o \
    thomson_dpt78010.o \
	philips_td1316.o \
	tdtm_g252d.o \
	microtune_mt2060.o\
	mt2060_api/mt2060.o \
	mt2060_api/mt_userdef.o \
	mt2060_api/mt_spuravoid.o

obj-$(CONFIG_DVB_DIBUSB) += dvb-dibusb.o

EXTRA_CFLAGS = -Idrivers/media/dvb/dvb-core/ -Idrivers/media/dvb/frontends/ 
EXTRA_CFLAGS+= -UGL861_DBG_EN -DRT2830_CYPRESS_DBG_EN -DRTL2831U_DBG_EN -DFIX_USB_1K_BOUNDRARY_LIMIT

ifdef CONFIG_DVB_REALTEK_2830
EXTRA_CFLAGS+= -DDVB_RTL2830_EN 
endif 


####################################
# Define of RTD2820
####################################
      
ifdef CONFIG_ATSC_RTD2820_DDT768x
RTD2820_FLAG = -DATSC_RTD2820_EN
endif 

ifdef RTD2820_FLAG
EXTRA_CFLAGS+= $(RTD2820_FLAG)
endif
