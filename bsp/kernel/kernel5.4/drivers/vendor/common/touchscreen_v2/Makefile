#
# Makefile for zte touchscreen_v2 driver.
#
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/goodix_berlin_driver_v2/firmware_config/$(CONFIG_TOUCHSCREEN_BOARD_NAME)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/chipone_tddi_v3/firmware_config/$(CONFIG_TOUCHSCREEN_BOARD_NAME)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/chsc5xxx/firmware_config/$(CONFIG_TOUCHSCREEN_BOARD_NAME)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/ilitek_tddi_v3/firmware_config/$(CONFIG_TOUCHSCREEN_BOARD_NAME)/

zte_tpd-y := ztp_core.o ztp_report_algo.o ztp_state_change.o
zte_tpd-$(CONFIG_TOUCHSCREEN_UFP_MAC) +=  ztp_ufp.o

######################################################
ifneq ($(filter y, $(CONFIG_TOUCHSCREEN_GOODIX_BRL_V2)),)
	ifneq ($(filter y, $(CONFIG_TOUCHSCREEN_GOODIX_BRL_I2C)),)
		zte_tpd-y += goodix_berlin_driver_v2/goodix_brl_i2c.o
	else
		zte_tpd-y += goodix_berlin_driver_v2/goodix_brl_spi.o
	endif
endif
zte_tpd-$(CONFIG_TOUCHSCREEN_GOODIX_BRL_V2) += \
	goodix_berlin_driver_v2/goodix_ts_core.o \
	goodix_berlin_driver_v2/goodix_brl_hw.o \
	goodix_berlin_driver_v2/goodix_cfg_bin.o \
	goodix_berlin_driver_v2/goodix_ts_utils.o \
	goodix_berlin_driver_v2/goodix_brl_fwupdate.o \
	goodix_berlin_driver_v2/goodix_ts_gesture.o \
	goodix_berlin_driver_v2/goodix_ts_inspect.o \
	goodix_berlin_driver_v2/goodix_ts_tools.o \
	goodix_berlin_driver_v2/goodix_common_interface.o

################### semi touch #########################
zte_tpd-$(CONFIG_TOUCHSCREEN_CHSC5XXX)	+= \
	chsc5xxx/basic_util.o \
	chsc5xxx/i2c_communication.o \
	chsc5xxx/qual_core.o \
	chsc5xxx/semi_touch_function.o \
	chsc5xxx/semi_touch_upgrade.o \
	chsc5xxx/semi_touch_apk.o \
	chsc5xxx/semi_touch_device.o \
	chsc5xxx/semi_touch_custom.o \
	chsc5xxx/semi_touch_test_5448.o \
	chsc5xxx/semi_common_interface.o
######################################################

###################CHIPONE_TDDI_V3#####################
zte_tpd-$(CONFIG_TOUCHSCREEN_CHIPONE_V3) += \
	chipone_tddi_v3/cts_driver.o \
	chipone_tddi_v3/cts_i2c_driver.o \
	chipone_tddi_v3/cts_spi_driver.o \
	chipone_tddi_v3/cts_core.o \
	chipone_tddi_v3/cts_sfctrlv2.o \
	chipone_tddi_v3/cts_spi_flash.o \
	chipone_tddi_v3/cts_firmware.o \
	chipone_tddi_v3/cts_test.o \
	chipone_tddi_v3/cts_strerror.o \
	chipone_tddi_v3/cts_charger_detect.o \
	chipone_tddi_v3/cts_earjack_detect.o \
	chipone_tddi_v3/cts_platform.o \
	chipone_tddi_v3/cts_tool.o \
	chipone_tddi_v3/cts_sysfs.o \
	chipone_tddi_v3/cts_ioctl.o \
	chipone_tddi_v3/cts_cdev.o \
	chipone_tddi_v3/cts_common_interface.o \
	chipone_tddi_v3/cts_selftest.o \
	chipone_tddi_v3/cts_selftest_ini.o
######################################################
########### ilitek tddi tp driver ###############
zte_tpd-$(CONFIG_TOUCHSCREEN_ILITEK_TDDI_V3) += \
	ilitek_tddi_v3/ilitek_v3.o \
	ilitek_tddi_v3/ilitek_v3_qcom.o \
	ilitek_tddi_v3/ilitek_v3_ic.o \
	ilitek_tddi_v3/ilitek_v3_touch.o \
	ilitek_tddi_v3/ilitek_v3_mp.o \
	ilitek_tddi_v3/ilitek_v3_node.o
zte_tpd-$(CONFIG_TOUCHSCREEN_ILITEK_TDDI_V3_I2C) += \
	ilitek_tddi_v3/ilitek_v3_i2c.o \
	ilitek_tddi_v3/ilitek_v3_flash.o

zte_tpd-$(CONFIG_TOUCHSCREEN_ILITEK_TDDI_V3_SPI) += \
	ilitek_tddi_v3/ilitek_v3_spi.o \
	ilitek_tddi_v3/ilitek_v3_hostdl.o \
	ilitek_tddi_v3/ilitek_v3_common_interface.o
########################################################

obj-$(CONFIG_TOUCHSCREEN_VENDOR_V2) += zte_tpd.o

