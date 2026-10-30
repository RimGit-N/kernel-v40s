#ifndef CUSTOM_FUNC_H__
#define CUSTOM_FUNC_H__
#define  ST_UPGRADE_FIRMWARE
#define  ST_MONITOR_THREAD
/* #define  ST_SMART_WAKE_UP */
#define  ST_TEST_RAW
/* #define  ST_GLOVE_SWITCH_MODE */
/* #define  ST_CASES_SWITCH_MODE */
#define  ST_DEBUG_EN
#define  ST_MAX_TOUCHES  10
#define MAX_RX_COUNT 32
#define MAX_TX_COUNT 16
#define MAX_KEY_COUNT 4
#define MAX_SENSOR_COUNT (MAX_RX_COUNT + MAX_TX_COUNT + MAX_KEY_COUNT)
#define ST_RAWTEST_LOGFILE
/* #define ST_SHORT_TEST_IS_1T2R		1 */
#define ST_SHORT_TEST_CFG_OFFSET 0xEFC0

#define STS_REPORT_BY_ZTE_ALGO

#ifdef STS_REPORT_BY_ZTE_ALGO
#define sts_left_edge_limit_v		6
#define sts_right_edge_limit_v		6
#define sts_left_edge_limit_h		6
#define sts_right_edge_limit_h		6
#define sts_left_edge_long_pess_v		20
#define sts_right_edge_long_pess_v	20
#define sts_left_edge_long_pess_h		70
#define sts_right_edge_long_pess_h	35
#define sts_long_press_max_count		60
#define sts_edge_long_press_check 1
#endif

#define STP_VENDOR_ID_0 0xc3
#define STP_VENDOR_ID_1 0
#define STP_VENDOR_ID_2 0
#define STP_VENDOR_ID_3 0
#define STP_VENDOR_0_NAME                         "lcetron"
#define STP_VENDOR_1_NAME                         "unknown"
#define STP_VENDOR_2_NAME                         "unknown"
#define STP_VENDOR_3_NAME                         "unknown"
#endif /* CUSTOM_FUNC_H__ */
