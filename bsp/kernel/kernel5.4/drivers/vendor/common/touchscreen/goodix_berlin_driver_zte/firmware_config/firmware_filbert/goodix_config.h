/*
 * GTPTouchScreen oem config.
 */

#ifndef _GOODIX_CONFIG_H_
#define _GOODIX_CONFIG_H_

#define GTP_REPORT_BY_ZTE_ALGO
#ifdef GTP_REPORT_BY_ZTE_ALGO
#define gtp_left_edge_limit_v			10
#define gtp_right_edge_limit_v		10
#define gtp_left_edge_limit_h			10
#define gtp_right_edge_limit_h		10
#define gtp_left_edge_long_pess_v		30
#define gtp_right_edge_long_pess_v	30
#define gtp_left_edge_long_pess_h	50
#define gtp_right_edge_long_pess_h	30
#define gtp_long_press_max_count	150
#define gtp_edge_long_press_check	1
#endif
#define GET_TP_FW_BY_LCD_NAME

#define GTP_VENDOR_ID_0 0
#define GTP_VENDOR_ID_1 1
#define GTP_VENDOR_ID_2 2
#define GTP_VENDOR_ID_3 3

#define GTP_VENDOR_0_NAME                 "visionox"
#define GTP_VENDOR_1_NAME                 "unknown"
#define GTP_VENDOR_2_NAME                 "unknown"
#define GTP_VENDOR_3_NAME                 "unknown"

#endif

