LINUX_KSRC_MODULE = /lib/modules/$(shell uname -r)/kernel/drivers/net/wireless/
RTL819x_DIR = $(shell pwd)
KVER  = $(shell uname -r)
KSRC = /lib/modules/$(KVER)/build
RTL819x_FIRM_DIR = $(RTL819x_DIR)/firmware
HAL_SUB_DIR = rtl8192u
MODULE_FILE = $(RTL819x_DIR)/ieee80211/Module.symvers

export LINUX_KSRC_MODULE_DIR RTL819x_FIRM_DIR

all: 
ifeq ($(shell uname -r|cut -d. -f1,2), 2.4)
	@make -C $(RTL819x_DIR)/ieee80211
	@make -C $(RTL819x_DIR)/HAL/$(HAL_SUB_DIR) 
else	
	@make -C $(KSRC) SUBDIRS=$(RTL819x_DIR)/ieee80211 modules 
	@test -f $(MODULE_FILE) && cp $(MODULE_FILE) $(RTL819x_DIR)/HAL/$(HAL_SUB_DIR) || echo > /dev/null 
	@make -C $(KSRC) SUBDIRS=$(RTL819x_DIR)/HAL/$(HAL_SUB_DIR) modules 
endif
install:
	@make -C ieee80211/ install
	@make -C HAL/$(HAL_SUB_DIR)/ install
uninstall:
	@make -C ieee80211/ uninstall
	@make -C HAL/$(HAL_SUB_DIR)/ uninstall
clean:
	@make -C HAL/$(HAL_SUB_DIR)/ clean
	@make -C ieee80211/ clean
