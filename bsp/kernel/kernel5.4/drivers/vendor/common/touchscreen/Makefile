#
# Makefile for zte touchscreen driver.
#
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/ilitek_tddi_9881x/firmware_config/$(CONFIG_TS_FIRMWARE)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/hxchipset_v2/firmware_config/$(CONFIG_TS_FIRMWARE)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/chipone_tddi/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/chipone_tddi/firmware_config/$(CONFIG_TS_FIRMWARE)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/goodix_berlin_driver_v2/firmware_config/$(CONFIG_TS_FIRMWARE)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/omnivision_tcm/firmware_config/$(CONFIG_TS_FIRMWARE)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/chipone_tddi_v2/firmware_config/$(CONFIG_TS_FIRMWARE)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/chipone_tddi_v3/firmware_config/$(CONFIG_TS_FIRMWARE)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/focaltech_touch_V3_3/focaltech_test/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen/focaltech_touch_V3_3/firmware_config/$(CONFIG_TS_FIRMWARE)/

zte_tpd-y := tpd_sys.o tpd_external_fun.o tpd_ufp_mac.o
########### ilitek 9881x tp driver ###############
zte_tpd-$(CONFIG_TOUCHSCREEN_ILITEK_TDDI_9881X) += \
	ilitek_tddi_9881x/ili9881x.o \
	ilitek_tddi_9881x/ili9881x_qcom.o \
	ilitek_tddi_9881x/ili9881x_ic.o \
	ilitek_tddi_9881x/ili9881x_touch.o \
	ilitek_tddi_9881x/ili9881x_mp.o \
	ilitek_tddi_9881x/ili9881x_node.o \
	ilitek_tddi_9881x/ili9881x_common_interface.o

zte_tpd-$(CONFIG_TOUCHSCREEN_ILITEK_TDDI_9881X_I2C) += \
	ilitek_tddi_9881x/ili9881x_i2c.o \
	ilitek_tddi_9881x/ili9881x_flash.o

zte_tpd-$(CONFIG_TOUCHSCREEN_ILITEK_TDDI_9881X_SPI) += ilitek_tddi_9881x/ili9881x_hostdl.o
########################################################

#### himax tp driver v2 #############################

zte_tpd-$(CONFIG_TOUCHSCREEN_HIMAX_CHIPSET_V2) += \
	hxchipset_v2/himax_common.o \
	hxchipset_v2/himax_inspection.o \
	hxchipset_v2/himax_ic_incell_core.o \
	hxchipset_v2/himax_debug.o \
	hxchipset_v2/himax_common_interface.o

zte_tpd-$(CONFIG_TOUCHSCREEN_HIMAX_IC_HX83102) += hxchipset_v2/himax_ic_HX83102.o
zte_tpd-$(CONFIG_TOUCHSCREEN_HIMAX_IC_HX83112) += hxchipset_v2/himax_ic_HX83112.o
zte_tpd-$(CONFIG_TOUCHSCREEN_HIMAX_IC_HX83113) += hxchipset_v2/himax_ic_HX83113.o

ifneq ($(filter y m, $(CONFIG_TOUCHSCREEN_HIMAX_CHIPSET_V2)),)
	ifneq ($(filter y, $(CONFIG_FB)),)
		ccflags-y += -DHX_CONFIG_FB
	endif
	ifneq ($(filter y, $(CONFIG_TOUCHSCREEN_HIMAX_SPI)),)
		zte_tpd-y += hxchipset_v2/himax_platform_SPI.o
	else
		zte_tpd-y += hxchipset_v2/himax_platform.o
	endif
endif

######################################################
zte_tpd-$(CONFIG_TOUCHSCREEN_CHIPONE) += \
	chipone_tddi/cts_i2c_driver.o \
	chipone_tddi/cts_core.o \
	chipone_tddi/cts_platform.o \
	chipone_tddi/cts_sfctrlv2.o \
	chipone_tddi/cts_spi_flash.o \
	chipone_tddi/cts_firmware.o \
	chipone_tddi/cts_tool.o \
	chipone_tddi/cts_sysfs.o \
	chipone_tddi/cts_test.o \
	chipone_tddi/cts_selftest.o \
	chipone_tddi/cts_selftest_ini.o \
	chipone_tddi/chipone_common_interface.o

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

######################################################

###################OMNIVISION_TCM#####################
zte_tpd-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += \
	omnivision_tcm/omnivision_common_interface.o \
	omnivision_tcm/omnivision_tcm_core.o \
	omnivision_tcm/omnivision_tcm_touch.o \
	omnivision_tcm/omnivision_tcm_device.o \
	omnivision_tcm/omnivision_tcm_testing.o \
	omnivision_tcm/omnivision_tcm_recovery.o \
	omnivision_tcm/omnivision_tcm_diagnostics.o

zte_tpd-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM_SPI) += \
	omnivision_tcm/omnivision_tcm_spi.o \
	omnivision_tcm/omnivision_tcm_zeroflash.o

zte_tpd-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM_I2C) += \
	omnivision_tcm/omnivision_tcm_i2c.o \
	omnivision_tcm/omnivision_tcm_reflash.o
######################################################

###################CHIPONE_TDDI_V2#####################
zte_tpd-$(CONFIG_TOUCHSCREEN_CHIPONE_V2) += \
	chipone_tddi_v2/cts_i2c_driver.o \
	chipone_tddi_v2/cts_core.o \
	chipone_tddi_v2/cts_sfctrlv2.o \
	chipone_tddi_v2/cts_spi_flash.o \
	chipone_tddi_v2/cts_firmware.o \
	chipone_tddi_v2/cts_test.o \
	chipone_tddi_v2/cts_hostcomm.o \
	chipone_tddi_v2/cts_tcs.o \
	chipone_tddi_v2/cts_platform.o \
	chipone_tddi_v2/cts_tool.o \
	chipone_tddi_v2/cts_sysfs.o \
	chipone_tddi_v2/cts_common_interface.o \
	chipone_tddi_v2/cts_selftest.o \
	chipone_tddi_v2/cts_selftest_ini.o \
	chipone_tddi_v2/cts_strerror.o
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

###################FOCAL TOUCH V3_3#####################
zte_tpd-$(CONFIG_TOUCHSCREEN_FTS_V3_3) += \
	focaltech_touch_V3_3/focaltech_core.o \
	focaltech_touch_V3_3/focaltech_ex_fun.o \
	focaltech_touch_V3_3/focaltech_ex_mode.o \
	focaltech_touch_V3_3/focaltech_gesture.o \
	focaltech_touch_V3_3/focaltech_esdcheck.o \
	focaltech_touch_V3_3/focaltech_point_report_check.o \
	focaltech_touch_V3_3/focaltech_flash.o \
	focaltech_touch_V3_3/focaltech_common_interface.o \
	focaltech_touch_V3_3/focaltech_spi.o \
	focaltech_touch_V3_3/focaltech_test/focaltech_test.o \
	focaltech_touch_V3_3/focaltech_test/focaltech_test_ini.o \
	focaltech_touch_V3_3/focaltech_test/supported_ic/focaltech_test_ft8756.o

######################################################

obj-$(CONFIG_TOUCHSCREEN_VENDOR) += zte_tpd.o
BOARD_VENDOR_RAMDISK_KERNEL_MODULES += 	$(BSP_KERNEL_MODULES_OUT)/zte_tpd.ko

