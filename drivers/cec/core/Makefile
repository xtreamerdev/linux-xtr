obj-$(CONFIG_CEC_CHARDEV) += cec-dev.o
# include the adapter(s), but only if compiled as module
cec-adapters-$(CONFIG_CEC_CHARDEV)-$(CONFIG_MARS_CEC) += ../adapter/cec_mars.o
cec-dev-objs := cec_core.o cm_buff.o cec_dev.o $(cec-adapters-m-y)

