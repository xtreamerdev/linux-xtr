#
# Makefile for the i2c algorithms
#

obj-$(CONFIG_I2C_ALGOBIT)	+= i2c-algo-bit.o
obj-$(CONFIG_I2C_ALGOPCF)	+= i2c-algo-pcf.o
obj-$(CONFIG_I2C_ALGOPCA)	+= i2c-algo-pca.o
obj-$(CONFIG_I2C_ALGOITE)	+= i2c-algo-ite.o
obj-$(CONFIG_I2C_ALGO_SIBYTE)	+= i2c-algo-sibyte.o
obj-$(CONFIG_I2C_ALGO_SGI)	+= i2c-algo-sgi.o

ifeq ($(CONFIG_I2C_VENUS_INT_MODE_XFER),y)
  obj-$(CONFIG_I2C_VENUS)	+= i2c-algo-venus-k.o
else
  obj-$(CONFIG_I2C_VENUS)	+= i2c-algo-venus.o
endif

ifeq ($(CONFIG_I2C_DEBUG_ALGO),y)
  EXTRA_CFLAGS += -DDEBUG
endif


