/************************************************************************
*
* File Name: sitronix_common_interface.c
*
*  *   Version: v1.0
*
************************************************************************/
#include "sitronix_ts_custom_func.h"
#include "sitronix_ts.h"

#define MAX_FILE_NAME_LEN       64
#define MAX_FILE_PATH_LEN  64
#define MAX_NAME_LEN_20  20

char sitronix_vendor_name[MAX_NAME_LEN_20] = { 0 };
char sitronix_firmware_name[MAX_FILE_NAME_LEN] = {0};
char sitronix_cfg_test_name[MAX_FILE_NAME_LEN] = {0};
char g_stx_save_file_path[MAX_FILE_NAME_LEN] = {0};
int sitronix_vendor_id = 0;
int sitronix_tptest_result = 0;
const struct ts_firmware *adb_upgrade_firmware = NULL;
int sitronix_ts_suspend(struct i2c_client *client);
int sitronix_ts_resume(struct i2c_client *client);

struct tpvendor_t sitronix_vendor_l[] = {
	{STP_VENDOR_ID_0, STP_VENDOR_0_NAME},
	{STP_VENDOR_ID_1, STP_VENDOR_1_NAME},
	{STP_VENDOR_ID_2, STP_VENDOR_2_NAME},
	{STP_VENDOR_ID_3, STP_VENDOR_3_NAME},
	{VENDOR_END, "Unknown"},
};


int sitronix_get_fw(void)
{
	int i = 0;
	int ret = 0;
	const char *panel_name = NULL;

	panel_name = get_lcd_panel_name();
	for (i = 0; i < ARRAY_SIZE(sitronix_vendor_l); i++) {
		if (strnstr(panel_name, sitronix_vendor_l[i].vendor_name, strlen(panel_name))) {
			sitronix_vendor_id = sitronix_vendor_l[i].vendor_id;
			strlcpy(sitronix_vendor_name, sitronix_vendor_l[i].vendor_name,
				sizeof(sitronix_vendor_name));
			ret = 0;
			goto out;
		}
	}
	strlcpy(sitronix_vendor_name, "Unknown", sizeof(sitronix_vendor_name));
	ret = -EIO;
out:
	snprintf(sitronix_firmware_name, sizeof(sitronix_firmware_name),
			"sitronix_firmware_%s.dump", sitronix_vendor_name);
	snprintf(sitronix_cfg_test_name, sizeof(sitronix_cfg_test_name),
			"stx_cfg_test_%s.bin", sitronix_vendor_name);
	return ret;
}

const struct ts_firmware *sitronix_tp_requeset_firmware(char *file_name)
{
	struct file *file = NULL;
	char file_path[128] = { 0 };
	struct ts_firmware *firmware = NULL;
	int ret = 0;
	loff_t pos = 0;
	loff_t file_len = 0;

	snprintf(file_path, sizeof(file_path), "%s%s", "/sdcard/", file_name);
	file = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR(file)) {
		STX_ERROR("open %s file fail, try open /vendor/firmware/", file_path);
		snprintf(file_path, sizeof(file_path), "%s%s", "/vendor/firmware/", file_name);
		file = filp_open(file_path, O_RDONLY, 0);
		if (IS_ERR(file)) {
			STX_ERROR("open %s file fail", file_path);
			return NULL;
		}
	}

	firmware = kzalloc(sizeof(struct ts_firmware), GFP_KERNEL);
	if (firmware == NULL) {
		STX_ERROR("Request from file alloc struct firmware failed");
		goto err_close_file;
	}
	file_len = file_inode(file)->i_size;
	firmware->size = (int)file_len;
	STX_INFO("open %s file ,firmware->size:%d", file_path, firmware->size);
	firmware->data = vmalloc(firmware->size);
	if (firmware->data == NULL) {
		STX_ERROR("alloc firmware data failed");
		goto err_free_firmware;
	}

	pos = 0;
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
	ret = kernel_read(file, firmware->data, file_len, &pos);
#else
	ret = kernel_read(file, 0, firmware->data, file_len);
#endif
	if (ret < 0) {
		STX_ERROR("Request from fs read whole file failed %d", ret);
		goto err_free_firmware_data;
	}
	filp_close(file, NULL);
	return firmware;
err_free_firmware_data:
	vfree(firmware->data);
err_free_firmware:
	kfree(firmware);
err_close_file:
	filp_close(file, NULL);

	return NULL;
}

static int sitronix_tp_fw_upgrade(struct tpd_classdev_t *cdev, char *fw_name, int fwname_len)
{
	char fileName[128] = {0};
	int ret = 0;

	memset(fileName, 0, sizeof(fileName));
	snprintf(fileName, sizeof(fileName), "%s", fw_name);
	fileName[fwname_len - 1] = '\0';
	STX_INFO("%s: upgrade from file(%s) start!\n", __func__, fileName);
	adb_upgrade_firmware = sitronix_tp_requeset_firmware(fileName);
	if (adb_upgrade_firmware == NULL) {
		STX_ERROR("Request from file '%s' failed", fileName);
		goto error_fw_upgrade;
	}
	ret = st_upgrade_fw();
	vfree(adb_upgrade_firmware->data);
	kfree(adb_upgrade_firmware);
	return ret;
error_fw_upgrade:
	return -EIO;
}


static int tpd_init_tpinfo(struct tpd_classdev_t *cdev)
{
	struct sitronix_ts_data *ts = &stx_gpts;

	st_print_version(ts);
	strlcpy(cdev->ic_tpinfo.tp_name, "sitronix_ts", sizeof(cdev->ic_tpinfo.tp_name));
	strlcpy(cdev->ic_tpinfo.vendor_name, sitronix_vendor_name, sizeof(cdev->ic_tpinfo.vendor_name));
	cdev->ic_tpinfo.chip_model_id = TS_CHIP_SITRONIX;
	cdev->ic_tpinfo.firmware_ver = ts->ts_dev_info.fw_version[0];
	cdev->ic_tpinfo.module_id = sitronix_vendor_id;
	cdev->ic_tpinfo.i2c_type = 1;
	return 0;
}

static void sitronix_tp_resume_work(struct work_struct *work)
{
	struct sitronix_ts_data *ts = &stx_gpts;

	sitronix_ts_resume(ts->client);
}

static int sitronix_tp_suspend_show(struct tpd_classdev_t *cdev)
{
	struct sitronix_ts_data *ts = &stx_gpts;

	cdev->tp_suspend = ts->suspend_state;
	return cdev->tp_suspend;
}

static int sitronix_set_tp_suspend(struct tpd_classdev_t *cdev, u8 suspend_node, int enable)
{
	struct sitronix_ts_data *ts = &stx_gpts;

	if (enable) {
		sitronix_ts_suspend(ts->client);
	} else {
		queue_delayed_work(ts->sitronix_wq, &ts->resume_work, 0);
	}
	cdev->tp_suspend = ts->suspend_state;
	return cdev->tp_suspend;
}

static int tpd_test_save_file_path_store(struct tpd_classdev_t *cdev, const char *buf)
{
	memset(g_stx_save_file_path, 0, sizeof(g_stx_save_file_path));
	snprintf(g_stx_save_file_path, sizeof(g_stx_save_file_path), "%s", buf);

	STX_INFO("save file path:%s.", g_stx_save_file_path);

	return 0;
}

static int tpd_test_save_file_path_show(struct tpd_classdev_t *cdev, char *buf)
{
	ssize_t num_read_chars = 0;

	num_read_chars = snprintf(buf, PAGE_SIZE, "%s\n", g_stx_save_file_path);

	return num_read_chars;
}

static int tpd_test_cmd_store(struct tpd_classdev_t *cdev, const char *buf)
{
	STX_INFO("%s:enter, useless\n", __func__);
	return 0;
}

static int tpd_test_cmd_show(struct tpd_classdev_t *cdev, char *buf)
{
	ssize_t num_read_chars = 0;
	int i_len = 0;
	#ifdef ST_TEST_RAW
	int status = 0;
	#endif

	STX_INFO("%s", __func__);
	sitronix_tptest_result = 0;
	mutex_lock(&stx_gpts.dev_mutex);
#ifdef ST_TEST_RAW
	status = st_testraw_invoke();
	if (status)
		STX_ERROR("tpd  test fail:%d.\n", status);
#endif
	i_len = snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d", sitronix_tptest_result, stx_gpts.x_num, stx_gpts.y_num, 0);
	mutex_unlock(&stx_gpts.dev_mutex);
	STX_INFO("tpd  test:%s.\n", buf);

	num_read_chars = i_len;
	return num_read_chars;
}

int sitronix_register_fw_class(void)
{
	struct sitronix_ts_data *ts = &stx_gpts;

	sitronix_get_fw();
	tpd_fw_cdev.get_tpinfo = tpd_init_tpinfo;
	tpd_fw_cdev.tp_fw_upgrade = sitronix_tp_fw_upgrade;
	tpd_fw_cdev.tp_suspend_show = sitronix_tp_suspend_show;
	tpd_fw_cdev.set_tp_suspend = sitronix_set_tp_suspend;

	tpd_fw_cdev.tpd_test_set_save_filepath = tpd_test_save_file_path_store;
	tpd_fw_cdev.tpd_test_get_save_filepath = tpd_test_save_file_path_show;
	tpd_fw_cdev.tpd_test_set_cmd = tpd_test_cmd_store;
	tpd_fw_cdev.tpd_test_get_cmd = tpd_test_cmd_show;
	snprintf(g_stx_save_file_path, sizeof(g_stx_save_file_path), "%s", "/sdcard/");

#ifdef STS_REPORT_BY_ZTE_ALGO
	tpd_fw_cdev.max_x = ts->ts_dev_info.x_res;
	tpd_fw_cdev.max_y = ts->ts_dev_info.y_res;
	tpd_fw_cdev.edge_report_limit[0] = sts_left_edge_limit_v;
	tpd_fw_cdev.edge_report_limit[1] = sts_right_edge_limit_v;
	tpd_fw_cdev.edge_report_limit[2] = sts_left_edge_limit_h;
	tpd_fw_cdev.edge_report_limit[3] = sts_right_edge_limit_h;
	tpd_fw_cdev.long_pess_suppression[0] = sts_left_edge_long_pess_v;
	tpd_fw_cdev.long_pess_suppression[1] = sts_right_edge_long_pess_v;
	tpd_fw_cdev.long_pess_suppression[2] = sts_left_edge_long_pess_h;
	tpd_fw_cdev.long_pess_suppression[3] = sts_right_edge_long_pess_h;
	tpd_fw_cdev.long_press_max_count = sts_long_press_max_count;
	tpd_fw_cdev.edge_long_press_check = sts_edge_long_press_check;
#endif
	ts->sitronix_wq = create_singlethread_workqueue("sitronix wq");
	if (!ts->sitronix_wq) {
		STX_ERROR(" allocate sitronix wq failed\n");
	} else  {
		INIT_DELAYED_WORK(&ts->resume_work, sitronix_tp_resume_work);
	}
	return 0;
}




