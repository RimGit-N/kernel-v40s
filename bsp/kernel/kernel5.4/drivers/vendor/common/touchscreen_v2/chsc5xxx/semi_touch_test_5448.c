#include <linux/delay.h>
#include <linux/slab.h>
#include "semi_touch_test_5448.h"
#include "semi_touch_function.h"
#include "ztp_common.h"

#if SEMI_TOUCH_FACTORY_TEST_EN

#define MAX_TX_NUM_5472                          32
#define MAX_RX_NUM_5472                          40
#define MAX_CAP_DATA_SIZE                        (MAX_TX_NUM_5472 * MAX_RX_NUM_5472 * 2)
#define SAVE_LOG_NAME                            "/sdcard/chsc_factory_test_result.txt"

#define SHORT_TEST_THL                           600
#define RAWDATA_TEST_THL                         30
#define TEST_BEYOND_MAX_LIMIT		0x0001
#define TEST_BEYOND_MIN_LIMIT		0x0002
#define TEST_GT_SHORT				0x0400
extern int semi_touch_run_ram_code(unsigned char code);

#define semi_touch_log_file(fmt, ...) do { memset(gFactory.catch_buffer, 0, sizeof(gFactory.catch_buffer)); sprintf(gFactory.catch_buffer, fmt, ##__VA_ARGS__); semi_touch_log_file_imp(gFactory.catch_buffer); } while (0)
const short rawdata_min[] = {
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
	3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000, 3000,
};
const short rawdata_max[] = {
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
	8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000,
};

struct factory_test_init{
	unsigned char rowsCnt;
	unsigned char colsCnt;
	unsigned short sensor_2_ic_map[MAX_RX_NUM_5472 + MAX_TX_NUM_5472];
	char catch_buffer[100];
	unsigned char read_buffer[MAX_CAP_DATA_SIZE];
    struct file *file;
	loff_t pos;
};
static struct factory_test_init gFactory;
int semi_touch_test_prepare(void)
{
	int ret = 0, index = 0;
	/* unsigned char read_buffer[256]; */
	struct m_ctp_cmd_std_t cmd_send_tp;
	struct m_ctp_rsp_std_t ack_from_tp;

	close_esd_function(st_dev.stc.custom_function_en);
	gFactory.pos = 0;

	msleep(50);
	cmd_send_tp.id = CMD_IDENTITY;
	ret = cmd_send_to_tp(&cmd_send_tp, &ack_from_tp, 2000);
	check_return_if_fail(ret, NULL);
	if ((ack_from_tp.d0 == 0xE902) && (ack_from_tp.d1 == 0x16fd)) {
		ret = semi_touch_read_bytes(0x20000080, gFactory.read_buffer, 256);
		check_return_if_fail(ret, NULL);
		gFactory.rowsCnt = gFactory.read_buffer[0x1a];
		gFactory.colsCnt = gFactory.read_buffer[0x19];
		for (index = 0; index < MAX_TX_NUM_5472; index++) {
			gFactory.sensor_2_ic_map[index] = gFactory.read_buffer[0x90 + index];
		}
		for (index = 0; index < MAX_RX_NUM_5472; index++) {
			gFactory.sensor_2_ic_map[index + MAX_TX_NUM_5472] = gFactory.read_buffer[0xb0 + index];
		}
	} else {
		ret = -SEMI_DRV_ERR_NO_INIT;
	}
		kernel_log_d("row = %d, col = %d\n", gFactory.rowsCnt, gFactory.colsCnt);
		return ret;
}

void semi_touch_log_file_imp(char *sztext)
{
	tpd_copy_to_tp_firmware_data(sztext);
}

void semi_touch_print_matrix(short *matrix, int rows, int cols)
{
	int row = 0, col = 0;

	for (row = 0; row < rows; row++) {
		for (col = 0; col < cols; col++) {
			semi_touch_log_file("%-6d", *matrix);
			matrix++;
		}
		semi_touch_log_file("\n");
	}
	semi_touch_log_file("\n");
}

int semi_touch_rawdata_test(void)
{
	int ret = 0, raw_averate = 0, failer_cnt = 0;
	int row = 0, col = 0, channels = 0, index = 0;
	short *rawdata_p = NULL;
	const short *max_p = rawdata_max, *min_p = rawdata_min;
	unsigned short *pTail = NULL;
	struct m_ctp_cmd_std_t cmd_send_tp;
	struct m_ctp_rsp_std_t ack_from_tp;

	semi_touch_log_file("\n\n----------------------MCap RawData Test------------------------------\n\n");
	channels = gFactory.rowsCnt * gFactory.colsCnt + gFactory.rowsCnt + gFactory.colsCnt;
	for (index = 0; index < 5; index++) {
		cmd_send_tp.id = CMD_CTP_SSCAN;
		cmd_send_tp.d0 = 0;
		ret = cmd_send_to_tp(&cmd_send_tp, &ack_from_tp, 200);
		check_return_if_fail(ret, NULL);
		msleep(50);
		ret = semi_touch_read_bytes(st_dev.stc.rawdata_addr, gFactory.read_buffer, (channels * 2 + 6 + 3) & 0xfffc);
		check_return_if_fail(ret, NULL);
		pTail = (unsigned short *)&gFactory.read_buffer[channels * 2];
		if (*pTail + *(pTail + 1) == 0xffff) {
			if (*pTail == caculate_checksum_u16((unsigned short *)gFactory.read_buffer, channels * 2)) {
				break;
			}
		}
		cmd_send_tp.id = CMD_CTP_SSCAN;
		cmd_send_tp.d0 = 1;
		ret = cmd_send_to_tp(&cmd_send_tp, &ack_from_tp, 200);
		check_return_if_fail(ret, NULL);
	}
	rawdata_p = (short *)gFactory.read_buffer;
	semi_touch_log_file("mcap_rawdata:\n");
	semi_touch_print_matrix(rawdata_p, gFactory.rowsCnt, gFactory.colsCnt);
	for (index = 0; index < gFactory.rowsCnt * gFactory.colsCnt; index++) {
		raw_averate += *rawdata_p;
		rawdata_p++;
	}
	raw_averate /= (gFactory.rowsCnt * gFactory.colsCnt);
	semi_touch_log_file("\navage = %d\n", raw_averate);
	rawdata_p = (short *)gFactory.read_buffer;
	for (row = 0; row < gFactory.rowsCnt; row++) {
		for (col = 0; col < gFactory.colsCnt; col++) {
			/* not realy used node */
			if (row == 0 && col == 6) {
			} else {
				if (*rawdata_p > *max_p) {
					failer_cnt++;
					semi_touch_log_file("node(%d, %d) out of range: %d(%d-%d)\n", row, col, *rawdata_p, *min_p, *max_p);
				} else if (*rawdata_p < *min_p) {
					failer_cnt++;
					semi_touch_log_file("node(%d, %d) out of range: %d(%d-%d)\n", row, col, *rawdata_p, *min_p, *max_p);
				}
			}
			min_p++;
			max_p++;
			rawdata_p++;
		}
	}
	if (failer_cnt) {
		semi_touch_log_file("\nrawdata test fail\n");
		return 1;
	}
	semi_touch_log_file("\nrawdata test pass\n");
	return 0;
}

int semi_touch_short_test(void)
{
	int ret = 0, tx = 0, rx = 0, loop = 0, failer_cnt = 0;
	short *short_p = NULL, ic_channle = 0;
	unsigned int u32_para_buff[4];
	short short_maxtrix[2][MAX_RX_NUM_5472];

	semi_touch_log_file("\n\n------------------------------Short Test------------------------------\n\n");
	ret = semi_touch_run_ram_code(1 /*RAM_CODE_SHORT_DATA_SHARE */);
	check_return_if_fail(ret, NULL);
	for (loop = 0; loop < 10; loop++) {
		msleep(30);
		u32_para_buff[0] = 0;

		semi_touch_read_bytes(0x20000000, (unsigned char *)u32_para_buff, 12);
		if (u32_para_buff[0] == 0x45000000) {
			break;
		}
	}
	if (u32_para_buff[0] == 0x45000000) {
		msleep(30);
		ret = semi_touch_read_bytes(u32_para_buff[1], gFactory.catch_buffer, (MAX_TX_NUM_5472 + MAX_RX_NUM_5472) * 2);
		check_return_if_fail(ret, NULL);
		short_p = (short *)gFactory.catch_buffer;
		for (tx = 0; tx < gFactory.rowsCnt; tx++) {
			ic_channle = gFactory.sensor_2_ic_map[tx];
			short_maxtrix[0][tx] = short_p[ic_channle];
		}
		for (rx = 0; rx < gFactory.colsCnt; rx++) {
			ic_channle = gFactory.sensor_2_ic_map[rx + MAX_TX_NUM_5472];
			short_maxtrix[1][rx] = short_p[ic_channle + MAX_TX_NUM_5472];
		}
		semi_touch_print_matrix(&short_maxtrix[0][0], 1, gFactory.rowsCnt);
		semi_touch_print_matrix(&short_maxtrix[1][0], 1, gFactory.colsCnt);
		for (tx = 0; tx < gFactory.rowsCnt; tx++) {
			if (short_maxtrix[0][tx] > SHORT_TEST_THL) {
				failer_cnt++;
				semi_touch_log_file("node(%d, %d) out of range: %d\n", 0, tx, short_maxtrix[0][tx]);
			}
		}
		for (rx = 0; rx < gFactory.colsCnt; rx++) {
			if (short_maxtrix[1][rx] > SHORT_TEST_THL) {
				failer_cnt++;
				semi_touch_log_file("node(%d, %d) out of range: %d\n", 1, rx, short_maxtrix[1][rx]);
			}
		}
	} else {
		failer_cnt++;
	}
	if (failer_cnt) {
		semi_touch_log_file("\nshort test fail\n");
		return 2;
	}
	semi_touch_log_file("\nshort test pass\n");
	return 0;
}

void semi_touch_factory_test_over(void)
{
	semi_touch_reset(do_report_after_reset);
	open_esd_function(st_dev.stc.custom_function_en);
}

void semi_get_tpd_channel_info(unsigned char *rowsCnt, unsigned char *colsCnt)
{
	*rowsCnt = gFactory.rowsCnt;
	*colsCnt = gFactory.colsCnt;
}

int semi_touch_start_factory_test(void)
{
	int ret = 0;

	ret = semi_touch_test_prepare();
	if (ret) {
		st_dev.tp_self_test_result = 1;
		return ret;
	}
	ret = semi_touch_rawdata_test();
	if (ret) 
		st_dev.tp_self_test_result = st_dev.tp_self_test_result | TEST_BEYOND_MIN_LIMIT |TEST_BEYOND_MAX_LIMIT;
	ret = semi_touch_short_test();
	if (ret) 
		st_dev.tp_self_test_result = st_dev.tp_self_test_result | TEST_GT_SHORT;
	semi_touch_factory_test_over();
	/* if(0 == ret)
		sprintf(detail, "rawdata test = %s\n, short test = %s\n", "PASS", "PASS");
	else if(1 == ret)
		sprintf(detail, "rawdata test = %s\n", "NG");
	else if(2 == ret)
		sprintf(detail, "short test = %s\n", "NG");
	else
		sprintf(detail, "exception, code = %d\n", ret); */
	return ret;
}

#endif

