#
# Makefile for the input core drivers.
#

# Each configuration option enables a list of files.
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/sitronix_ts/firmware_config/$(CONFIG_TS_FIRMWARE)/

obj-$(CONFIG_TOUCHSCREEN_SITRONIX_TS)  += sitronix_touch.o
sitronix_touch-objs := sitronix_ts.o sitronix_ts_monitor.o sitronix_ts_swu_gesture.o sitronix_ts_upgrade_fw.o sitronix_ts_i2c.o sitronix_ts_rawtest.o sitronix_ts_sysfs.o sitronix_ts_utility.o sitronix_common_interface.o
