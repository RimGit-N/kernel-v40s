# SPDX-License-Identifier: GPL-2.0
ifeq ($(BSP_BUILD_DT_OVERLAY),y)

ifeq ($(BSP_BUILD_ANDROID_OS),y)

ifeq ($(BSP_BUILD_FAMILY),qogirl6)
dtbo-$(CONFIG_ARCH_SPRD) += 	ums9230-4h10-overlay.dtbo 

ums9230-4h10-overlay.dtbo-base := ums9230.dtb
endif

endif #//BSP_BUILD_ANDROID_OS

endif #//BSP_BUILD_DT_OVERLAY
always		:= $(dtb-y) $(dtbo-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo
