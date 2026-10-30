
/************************************************************************
*
* File Name: himax_firmware_config.h
*
*  *   Version: v1.0
*
************************************************************************/
#ifndef _HIMAX_FIRMWARE_CONFIG_H_
#define _HIMAX_FIRMWARE_CONFIG_H_

/********************** Upgrade ***************************

  auto upgrade, please keep enable
*********************************************************/
/*#define HX_BOOT_UPGRADE */
#define HX_SMART_WAKEUP
/* #define HX_HIGH_SENSE */
#define HX_USB_DETECT_GLOBAL
#define HEADLINE_MODE
#define HX_DISPLAY_ROTATION
#define HX_EDGE_LIMIT
#define HX_REPORT_BY_ZTE_ALGO
#define HX_ZERO_FLASH
#define DELAY_TIME 100

#ifdef HX_REPORT_BY_ZTE_ALGO
#define hx_left_edge_limit_v		6
#define hx_right_edge_limit_v		6
#define hx_left_edge_limit_h		6
#define hx_right_edge_limit_h		6
#define hx_left_edge_long_pess_v		20
#define hx_right_edge_long_pess_v	20
#define hx_left_edge_long_pess_h		40
#define hx_right_edge_long_pess_h	20
#define hx_long_press_max_count		80
#define hx_edge_long_press_check 0
#endif

enum fix_touch_info {
	FIX_HX_RX_NUM = 48,
	FIX_HX_TX_NUM = 30,
	FIX_HX_BT_NUM = 0,
	FIX_HX_MAX_PT = 10,
	FIX_HX_INT_IS_EDGE = true,
	FIX_HX_STYLUS_FUNC = 0,
#if defined(HX_TP_PROC_2T2R)
	FIX_HX_RX_NUM_2 = 0,
	FIX_HX_TX_NUM_2 = 0,
#endif
};

enum himax_vendor_id {
	HX_VENDOR_ID_0	= 0x00,
	HX_VENDOR_ID_1,
	HX_VENDOR_ID_2,
	HX_VENDOR_ID_3,
	HX_VENDOR_ID_MAX		= 0xFF,
};

/*
 * Numbers of modules support
 */

#define HXTS_VENDOR_0_NAME	"unknown"
#define HXTS_VENDOR_1_NAME	"unknown"
#define HXTS_VENDOR_2_NAME	"unknown"
#define HXTS_VENDOR_3_NAME	"unknown"
#ifdef HX_ZERO_FLASH
/* this macro need be configured refer to module*/
#define HXTS_DEFAULT_FIRMWARE     "hxtp_k95_default_common_firmware"
#endif
#endif
