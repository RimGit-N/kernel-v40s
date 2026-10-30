#include "semi_touch_custom.h"
#include "semi_touch_function.h"
#include "semi_touch_upgrade.h"
#include "semi_config.h"
#include <linux/power_supply.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/i2c.h>
#if SEMI_TOUCH_FACTORY_TEST_EN
#include "semi_touch_test_5448.h"
#endif

#define MAX_NAME_LEN_20  20
#define MAX_NAME_LEN_50  50
#define MAX_FILE_NAME_LEN       64
#define MAX_ALLOC_BUFF 256
#define TP_TEST_START	2

extern const char *const mapping_ic_from_type(unsigned char ictype);
extern int semi_set_tp_suspend(struct ztp_device *cdev, u8 suspend_node, int enable);
extern int semi_touch_check_and_update(const unsigned char *udp, unsigned int len);
extern void semi_get_tpd_channel_info(unsigned char *rowsCnt, unsigned char *colsCnt);

char semi_vendor_name[MAX_NAME_LEN_20] = { 0 };
char semi_firmware_name[MAX_FILE_NAME_LEN] = {0};
char g_semi_save_file_path[MAX_NAME_LEN_50] = { 0 };
char g_semi_save_file_name[MAX_NAME_LEN_50] = { 0 };

struct tpvendor_t semi_vendor_l[] = {
	{SEMI_VENDOR_ID_0, SEMI_VENDOR_0_NAME},
	{SEMI_VENDOR_ID_1, SEMI_VENDOR_1_NAME},
	{SEMI_VENDOR_ID_2, SEMI_VENDOR_2_NAME},
	{SEMI_VENDOR_ID_3, SEMI_VENDOR_3_NAME},
	{VENDOR_END, "Unknown"},
};

int semi_get_vendor_and_firmware(void)
{
	int i = 0;
	int ret = 0;
	const char *panel_name = NULL;

	panel_name = get_lcd_panel_name();
	for (i = 0; i < ARRAY_SIZE(semi_vendor_l); i++) {
		if (strnstr(panel_name, semi_vendor_l[i].vendor_name, strlen(panel_name))) {
			strlcpy(semi_vendor_name, semi_vendor_l[i].vendor_name, sizeof(semi_vendor_name));
			ret = 0;
			goto out;
		}
	}
	ret = -1;
	strlcpy(semi_vendor_name, SEMI_VENDOR_0_NAME, sizeof(semi_vendor_name));

out:
	snprintf(semi_firmware_name, sizeof(semi_firmware_name),
			"semi_firmware_%s.bin", semi_vendor_name);
	kernel_log_d("firmware name :%s\n", semi_firmware_name);
	return ret;
}

static int tpd_init_tpinfo(struct ztp_device *cdev)
{
	int ret = 0;
	unsigned char readBuffer[8] = { 0 };

	kernel_log_d("enter!");

	ret = semi_touch_read_bytes(0x20000000 + 0x80, readBuffer, 8);
	check_return_if_fail(ret, NULL);

	snprintf(cdev->ic_tpinfo.tp_name, MAX_VENDOR_NAME_LEN, "chsc_%s", mapping_ic_from_type(readBuffer[0]));
	cdev->ic_tpinfo.config_ver = readBuffer[1];
	cdev->ic_tpinfo.module_id = (((readBuffer[3] << 8) + readBuffer[2]) << 8) + readBuffer[4];
	ret = semi_touch_read_bytes(0x20000000 + 0x10, readBuffer, 8);
	check_return_if_fail(ret, NULL);
	cdev->ic_tpinfo.firmware_ver =(readBuffer[5] << 8) + readBuffer[4];
	cdev->ic_tpinfo.chip_model_id = TS_CHIP_TLSC;
	cdev->ic_tpinfo.i2c_addr = 0x2e;
	strlcpy(cdev->ic_tpinfo.vendor_name, semi_vendor_name, sizeof(cdev->ic_tpinfo.vendor_name));
	return 0;

}

static int semi_tp_fw_upgrade(struct ztp_device *cdev, char *fw_name, int fwname_len)
{

	if (cdev->tp_firmware == NULL || cdev->tp_firmware->data == NULL) {
		kernel_log_e("cdev->tp_firmware is NULL");
		return -EIO;
	}

	st_dev.fw_force_update = true;
	semi_touch_check_and_update(cdev->tp_firmware->data , (unsigned int)cdev->tp_firmware->size);
	st_dev.fw_force_update = false;
	if (st_dev.fw_is_updated) {
		semi_touch_mode_init(&st_dev);
		semi_touch_resolution_adaption(&st_dev);
		st_dev.fw_is_updated = false;
	}
	return 0;
}

static int tpd_get_wakegesture(struct ztp_device *cdev)
{

	cdev->b_gesture_enable =is_guesture_function_en(st_dev.stc.custom_function_en);

	return 0;
}

static int tpd_enable_wakegesture(struct ztp_device *cdev, int enable)
{
	if (enable)
		open_guesture_function(st_dev.stc.custom_function_en);
	else
		close_guesture_function(st_dev.stc.custom_function_en);

	return 0;
}

int semi_set_tp_suspend(struct ztp_device *cdev, u8 suspend_node, int enable)
{
	if (enable)
		change_tp_state(OFF);
	else
		change_tp_state(ON);
	return 0;
}

int semi_tp_suspend(void *semi_data)
{
	struct sm_touch_dev *st_dev = (struct sm_touch_dev *)semi_data;

	semi_touch_suspend_entry(&st_dev->client->dev);
	return 0;
}

int semi_tp_resume(void *semi_data)
{
	struct sm_touch_dev *st_dev = (struct sm_touch_dev *)semi_data;

	semi_touch_resume_entry(&st_dev->client->dev);
	return 0;
}

#if SEMI_TOUCH_FACTORY_TEST_EN
static int tpd_test_cmd_store(struct ztp_device *cdev)
{
	int retval = 0;

	kernel_log_d("%s:start TP test.\n", __func__);
	st_dev.tp_self_test_result = 0;
	retval = semi_touch_start_factory_test();
	if (retval == 0) {
		kernel_log_d("Self_Test Pass\n");
	} else {
		kernel_log_e("Self_Test Fail\n");
	}
	return 0;
}

static int tpd_test_cmd_show(struct ztp_device *cdev, char *buf)
{
	int i_len;
	unsigned char rowsCnt = 0;
	unsigned char colsCnt = 0;
	ssize_t num_read_chars = 0;

	kernel_log_d("%s:enter\n", __func__);
	semi_get_tpd_channel_info(&rowsCnt, &colsCnt);
	i_len = snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d",	st_dev.tp_self_test_result, rowsCnt, rowsCnt, 0);
	kernel_log_d("tpd test result:%d.\n", st_dev.tp_self_test_result);
	kernel_log_d("tpd  test:%s.\n", buf);
	num_read_chars = i_len;
	return num_read_chars;
}
#endif

void semi_tpd_register_fw_class(struct sm_touch_dev *st_dev)
{

	kernel_log_d("entry");
	semi_get_vendor_and_firmware();
	tpd_cdev->private = (void *)st_dev;
	tpd_cdev->get_tpinfo = tpd_init_tpinfo;
	tpd_cdev->set_tp_suspend = semi_set_tp_suspend;
	tpd_cdev->tp_fw_upgrade = semi_tp_fw_upgrade;
	tpd_cdev->get_gesture = tpd_get_wakegesture;
	tpd_cdev->wake_gesture = tpd_enable_wakegesture;
	tpd_cdev->tp_data = st_dev;
	tpd_cdev->tp_resume_func = semi_tp_suspend;
	tpd_cdev->tp_suspend_func = semi_tp_resume;
#if SEMI_TOUCH_FACTORY_TEST_EN
	tpd_cdev->tp_self_test = tpd_test_cmd_store;
	tpd_cdev->get_tp_self_test_result = tpd_test_cmd_show;
#endif
	tpd_cdev->max_x = SEMI_TOUCH_SOLUTION_X;
	tpd_cdev->max_y = SEMI_TOUCH_SOLUTION_Y;

	kernel_log_d("end");
}
