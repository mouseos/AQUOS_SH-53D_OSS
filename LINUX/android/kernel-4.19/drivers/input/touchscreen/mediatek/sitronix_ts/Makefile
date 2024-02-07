# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/sitronix_ts/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

obj-$(CONFIG_TOUCHSCREEN_SITRONIX_TS)  += sitronix_touch.o
sitronix_touch-objs := sitronix_ts.o sitronix_ts_monitor.o sitronix_ts_swu_gesture.o sitronix_ts_upgrade_fw.o sitronix_ts_i2c.o sitronix_ts_rawtest.o sitronix_ts_sysfs.o sitronix_ts_utility.o
