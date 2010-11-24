RTL8173_HAL := $(shell $(src)/checkout_source $(src) $(src)/../11n8709/)

obj-$(CONFIG_REALTEK_USB_RTL8191SU)       += HAL/rtl8192u/

clean:
	@make -C HAL/rtl8192u/ clean
	@make -C rtllib/ clean
