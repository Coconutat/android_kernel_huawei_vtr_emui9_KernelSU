ifeq ($(CONFIG_DHD_STATIC_KO),y)
  obj-m += wlanfty.o
else
  obj-y += wlanfty.o
endif
ifeq ($(TARGET_VERSION_MODE),factory)
  ccflags-y += -DHW_WIFI_FACTORY_MODE
endif
