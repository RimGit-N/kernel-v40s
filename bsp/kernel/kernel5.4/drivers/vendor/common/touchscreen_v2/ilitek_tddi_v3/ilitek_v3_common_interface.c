/************************************************************************
*
* File Name: ilitek_common_interface.c
*
*  *   Version: v1.0
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/

#include "ilitek_v3.h"
#include <linux/kernel.h>

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/

#define MAX_FILE_NAME_LEN       64
#define MAX_FILE_PATH_LEN  64
#define MAX_NAME_LEN_20  20
#define FW_TP_STATUS_BUFFER 9

int ilitek_vendor_id = 0;
int ilitek_tptest_result = 0;
char ilitek_vendor_name[MAX_NAME_LEN_20] = { 0 };
char ilitek_firmware_name[MAX_FILE_NAME_LEN] = {0};
char ini_rq_path[MAX_FILE_NAME_LEN] = { 0 };

enum ilitek_cmd {
	HEADSET = 0,
	CHARGER,
	MROTATION,
};

struct tpvendor_t ilitek_vendor_l[] = {
	{ILI_VENDOR_ID_0, ILI_VENDOR_0_NAME},
	{ILI_VENDOR_ID_1, ILI_VENDOR_1_NAME},
	{ILI_VENDOR_ID_2, ILI_VENDOR_2_NAME},
	{ILI_VENDOR_ID_3, ILI_VENDOR_3_NAME},
	{VENDOR_END, "Unknown"},
};

int ilitek_get_fw(u8 vendor_id)
{
	int i = 0;
	int ret = 0;
	const char *panel_name = NULL;

	if (ILI_MODULE_NUM == 1) {
		ilitek_vendor_id = ILI_VENDOR_ID_0;
		strlcpy(ilitek_vendor_name, ILI_VENDOR_0_NAME, sizeof(ilitek_vendor_name));
		ret = 0;
		goto out;
	}
	for (i = 0; i < ARRAY_SIZE(ilitek_vendor_l) && i < ILI_MODULE_NUM; i++) {
		if (ilitek_vendor_l[i].vendor_id == vendor_id) {
			ilitek_vendor_id = ilitek_vendor_l[i].vendor_id;
			strlcpy(ilitek_vendor_name, ilitek_vendor_l[i].vendor_name,
				sizeof(ilitek_vendor_name));
			ret = 0;
			goto out;
		}
	}
	panel_name = get_lcd_panel_name();
	for (i = 0; i < ARRAY_SIZE(ilitek_vendor_l); i++) {
		if (strnstr(panel_name, ilitek_vendor_l[i].vendor_name, strlen(panel_name))) {
			ilitek_vendor_id = ilitek_vendor_l[i].vendor_id;
			strlcpy(ilitek_vendor_name, ilitek_vendor_l[i].vendor_name,
				sizeof(ilitek_vendor_name));
			ret = 0;
			goto out;
		}
	}
	strlcpy(ilitek_vendor_name, "Unknown", sizeof(ilitek_vendor_name));
	ret = -EIO;
out:
	snprintf(ilitek_firmware_name, sizeof(ilitek_firmware_name),
			"ilitek_firmware_%s.hex", ilitek_vendor_name);
	snprintf(ini_rq_path, sizeof(ini_rq_path), "ilitek_mp_%s.ini", ilitek_vendor_name);
	return ret;
}

void ilitek_update_module_info(void)
{
	ilits->md_name = ilitek_vendor_name;
	ilits->md_fw_rq_path = ilitek_firmware_name;
	ilits->md_ini_rq_path = ini_rq_path;
	ilits->md_ini_path = ini_rq_path;
	ilits->md_fw_ili_size = 0;
	ILI_INFO("Found %s module\n", ilits->md_name);
	ilits->tp_module = ilitek_vendor_id;

}

static int tpd_init_tpinfo(struct ztp_device *cdev)
{
	u8 vendor_id = 0;

#if (TDDI_INTERFACE == BUS_I2C)
	ilits->info_from_hex = DISABLE;
#endif
	if (atomic_read(&ilits->ice_stat))
		ili_ice_mode_ctrl(DISABLE, OFF);
	ili_ic_get_fw_ver();
 #if (TDDI_INTERFACE == BUS_I2C)
	ilits->info_from_hex = ENABLE;
#endif
	ILI_INFO("Firmware version = 0x%x.\n", ilits->chip->fw_ver);
	vendor_id = (ilits->chip->fw_ver >> 24) & 0xff;
	ilitek_get_fw(vendor_id);
	ilitek_update_module_info();
	strlcpy(cdev->ic_tpinfo.tp_name, "ilitek", sizeof(cdev->ic_tpinfo.tp_name));
	strlcpy(cdev->ic_tpinfo.vendor_name, ilitek_vendor_name, sizeof(cdev->ic_tpinfo.vendor_name));
	cdev->ic_tpinfo.chip_model_id = TS_CHIP_ILITEK;
	cdev->ic_tpinfo.firmware_ver = ilits->chip->fw_ver;
	cdev->ic_tpinfo.module_id = ilitek_vendor_id;
	cdev->ic_tpinfo.i2c_type = 0;
	return 0;
}

static int ilitek_tp_fw_upgrade(struct ztp_device *cdev, char *fw_name, int fwname_len)
{
	bool esd_en = ilits->wq_esd_ctrl, bat_en = ilits->wq_bat_ctrl;

	ILI_INFO(" to upgarde firmware\n");

	mutex_lock(&ilits->touch_mutex);
	if (esd_en)
		ili_wq_ctrl(WQ_ESD, DISABLE);
	if (bat_en)
		ili_wq_ctrl(WQ_BAT, DISABLE);

	ilits->force_fw_update = ENABLE;
	ilits->node_update = true;
	ilits->fw_open = TP_FIRMWARE_DATA;
	ili_fw_upgrade_handler(NULL);
	ilits->force_fw_update = DISABLE;
	ilits->node_update = false;
	if (esd_en)
		ili_wq_ctrl(WQ_ESD, ENABLE);
	if (bat_en)
		ili_wq_ctrl(WQ_BAT, ENABLE);

	mutex_unlock(&ilits->touch_mutex);
	return 0;
}

static void ilitek_reset_cmd_display_rotation(void)
{
	u8 edge_palm_para = 0;

	switch (ilits->display_rotation) {
	case mRotatin_0:
		edge_palm_para = 1;
		break;
	case mRotatin_90:
		edge_palm_para = 2;
		break;
	case mRotatin_180:
		edge_palm_para = 1;
		break;
	case mRotatin_270:
		edge_palm_para = 0;
		break;
	default:
		break;
	}
	ILI_INFO("edge_palm_para : 0X%x.\n", edge_palm_para);
	if (ili_ic_func_ctrl("edge_palm", edge_palm_para) < 0)
		ILI_ERR("Write edge_palm failed\n");
}

void ili_touch_reset_send_cmd(enum ilitek_cmd cmd)
{
	ili_tp_reset();
	ili_touch_release_all_point();

	if (atomic_read(&ilits->fw_stat)) {
		ILI_INFO("tp fw upgrading,ignore cmd..\n");
		return;
	}
	switch (cmd) {
	case HEADSET:
		if (ili_ic_func_ctrl("ear_phone", ilits->headset_mode) < 0)
			ILI_ERR("Write headset plug in failed\n");
		if (ilits->charger_mode) {
			msleep(20);
			if (ili_ic_func_ctrl("plug", ENABLE) < 0)
				ILI_ERR("Write plug out failed\n");
		}
		if (ilits->display_rotation) {
			msleep(20);
			ilitek_reset_cmd_display_rotation();
		}
		break;
	case CHARGER:
		if (ili_ic_func_ctrl("plug", ilits->charger_mode) < 0)
			ILI_ERR("Write plug out failed\n");
		if (ilits->headset_mode) {
			msleep(20);
			if (ili_ic_func_ctrl("ear_phone", ENABLE) < 0)
				ILI_ERR("Write headset plug in failed\n");
		}
		if (ilits->display_rotation) {
			msleep(20);
			ilitek_reset_cmd_display_rotation();
		}
		break;
	case MROTATION:
		ilitek_reset_cmd_display_rotation();
		if (ilits->headset_mode) {
			msleep(20);
			if (ili_ic_func_ctrl("ear_phone", ENABLE) < 0)
				ILI_ERR("Write headset plug in failed\n");
		}
		if (ilits->charger_mode) {
			msleep(20);
			if (ili_ic_func_ctrl("plug", ENABLE) < 0)
				ILI_ERR("Write plug out failed\n");
		}
		break;
	default:
		ILI_ERR("unknown cmd.\n");
	}
}

void ilitek_ex_mode_recovery(void)
{
	ILI_INFO("send ex cmd msleep 100ms");
	msleep(100);
	if (ilits->charger_mode) {
		if (ili_ic_func_ctrl("plug", ENABLE) < 0)
			ILI_ERR("Write plug out failed\n");
	}
	if (ilits->headset_mode) {
		if (ili_ic_func_ctrl("ear_phone", ENABLE) < 0)
			ILI_ERR("Write headset plug in failed\n");
	}
}

static int ilitek_headset_state_show(struct ztp_device *cdev)
{
	cdev->headset_state = ilits->headset_mode;
	return cdev->headset_state;
}

static int ilitek_set_headset_state(struct ztp_device *cdev, int enable)
{
	ilits->headset_mode = enable;
	ILI_INFO("%s: headset_state = %d.\n", __func__, ilits->headset_mode);
	if (!ilits->tp_suspend) {
		ili_touch_reset_send_cmd(HEADSET);
	}
	return ilits->headset_mode;
}

static int ilitek_set_display_rotation(struct ztp_device *cdev, int mrotation)
{

	ilits->display_rotation = mrotation;
	if (ilits->tp_suspend)
		return 0;
	ILI_INFO("%s: display_rotation = %d.\n", __func__, ilits->display_rotation);
	ili_touch_reset_send_cmd(MROTATION);
	return ilits->display_rotation;
}

static bool ilitek_get_charger_ststus(void)
{
	static struct power_supply *batt_psy;
	union power_supply_propval val = { 0, };
	bool status = false;

	if (batt_psy == NULL)
		batt_psy = power_supply_get_by_name("battery");
	if (batt_psy) {
		batt_psy->desc->get_property(batt_psy, POWER_SUPPLY_PROP_STATUS, &val);
	}
	if ((val.intval == POWER_SUPPLY_STATUS_CHARGING) ||
		(val.intval == POWER_SUPPLY_STATUS_FULL)) {
		status = true;
	} else {
		status = false;
	}
	ILI_INFO("charger status:%d", status);
	return status;
}

static void ilitek_work_charger_detect_work(struct work_struct *work)
{
	bool charger_mode_old = ilits->charger_mode;

	ilits->charger_mode = ilitek_get_charger_ststus();
	if (!ilits->tp_suspend && (ilits->charger_mode != charger_mode_old)) {
		ili_touch_reset_send_cmd(CHARGER);
		if (ili_ic_func_ctrl("plug", ilits->charger_mode) < 0)	/* charger plug in/out */
			ILI_ERR("Write charger plug in failed\n");
	}
}

static int ilitek_charger_notify_call(struct notifier_block *nb, unsigned long event, void *data)
{
	struct power_supply *psy = data;

	if (event != PSY_EVENT_PROP_CHANGED) {
		return NOTIFY_DONE;
	}

	if ((strcmp(psy->desc->name, "usb") == 0)
	    || (strcmp(psy->desc->name, "ac") == 0)) {
		queue_delayed_work(ilits->ilitek_ts_workqueue, &ilits->charger_work, msecs_to_jiffies(500));
	}

	return NOTIFY_DONE;
}

static int ilitek_init_charger_notifier(void)
{
	int ret = 0;

	ILI_INFO("Init Charger notifier");

	ilits->charger_notifier.notifier_call = ilitek_charger_notify_call;
	ret = power_supply_reg_notifier(&ilits->charger_notifier);
	return ret;
}

static void ilitek_tp_reset_gpio_output(bool value)
{
	ILI_INFO("ilitek tp reset gpio set value: %d", value);
	if (gpio_is_valid(ilits->tp_rst))
		gpio_direction_output(ilits->tp_rst, value);
}

static int ilitek_ts_resume(void * ilitek_data)
{
	return ili_sleep_handler(TP_RESUME);
}

static int ilitek_ts_suspend(void * ilitek_data)
{
	return ili_sleep_handler(TP_DEEP_SLEEP);
}

static int ilitek_set_tp_suspend(struct ztp_device *cdev, u8 suspend_node, int enable)
{
	if (enable) {
		change_tp_state(OFF);
	} else {
		change_tp_state(ON);
	}
	cdev->tp_suspend = ilits->tp_suspend;
	return cdev->tp_suspend;
}

static int ilitek_tp_suspend_show(struct ztp_device *cdev)
{
	cdev->tp_suspend = ilits->tp_suspend;
	return cdev->tp_suspend;
}

static int tpd_get_wakegesture(struct ztp_device *cdev)
{
	cdev->b_gesture_enable = ilits->gesture;
	return 0;
}

static int tpd_enable_wakegesture(struct ztp_device *cdev, int enable)
{
	if (ilits->tp_suspend) {
		cdev->tp_suspend_write_gesture = true;
	}
	ilits->gesture = enable;
	return enable;
}

static bool tpd_suspend_need_awake(struct ztp_device *cdev)
{
	if (!cdev->tp_suspend_write_gesture &&
		(atomic_read(&ilits->fw_stat) || ilits->gesture)) {
		ILI_INFO("tp suspend need awake.\n");
		return true;
	} else {
		ILI_INFO("tp suspend dont need awake.\n");
		return false;
	}
}

static int tpd_test_cmd_store(struct ztp_device *cdev)
{
	int ret = 0;
	bool esd_en = ilits->wq_esd_ctrl, bat_en = ilits->wq_bat_ctrl;
	unsigned char *g_user_buf = NULL;

	ILI_INFO("Run MP test with LCM on\n");

	mutex_lock(&ilits->touch_mutex);

	if (esd_en)
		ili_wq_ctrl(WQ_ESD, DISABLE);
	if (bat_en)
		ili_wq_ctrl(WQ_BAT, DISABLE);

	g_user_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (ERR_ALLOC_MEM(g_user_buf)) {
		ILI_ERR("Failed to allocate g_user_buf.\n");
		mutex_unlock(&ilits->touch_mutex);
		return 0;
	}
	ilitek_tptest_result = 0;
	ret = ili_mp_test_handler(g_user_buf, ON);
	ILI_INFO("MP TEST %s, Error code = %d\n", (ret < 0) ? "FAIL" : "PASS", ret);
	if (esd_en)
		ili_wq_ctrl(WQ_ESD, ENABLE);
	if (bat_en)
		ili_wq_ctrl(WQ_BAT, ENABLE);
	kfree(g_user_buf);
	mutex_unlock(&ilits->touch_mutex);

	return 0;
}

static int tpd_test_cmd_show(struct ztp_device *cdev, char *buf)
{
	ssize_t num_read_chars = 0;
	int i_len = 0;

	i_len = snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d", ilitek_tptest_result, ilits->stx, ilits->srx, 0);
	ILI_INFO("tpd  test:%s.\n", buf);
	num_read_chars = i_len;
	return num_read_chars;
}

int ilitek_register_fw_class(void)
{
	tpd_cdev->get_tpinfo = tpd_init_tpinfo;
	tpd_cdev->tp_fw_upgrade = ilitek_tp_fw_upgrade;
	tpd_cdev->tp_suspend_show = ilitek_tp_suspend_show;
	tpd_cdev->set_tp_suspend = ilitek_set_tp_suspend;
	tpd_cdev->headset_state_show = ilitek_headset_state_show;
	tpd_cdev->set_headset_state = ilitek_set_headset_state;
	tpd_cdev->set_display_rotation = ilitek_set_display_rotation;
	tpd_cdev->get_gesture = tpd_get_wakegesture;
	tpd_cdev->wake_gesture = tpd_enable_wakegesture;
	tpd_cdev->tpd_suspend_need_awake = tpd_suspend_need_awake;
	tpd_cdev->tp_reset_gpio_output = ilitek_tp_reset_gpio_output;
	tpd_cdev->max_x = TOUCH_SCREEN_X_MAX;
	tpd_cdev->max_y = TOUCH_SCREEN_Y_MAX;
	tpd_cdev->tp_self_test = tpd_test_cmd_store;
	tpd_cdev->get_tp_self_test_result = tpd_test_cmd_show;
	tpd_cdev->tp_resume_before_lcd_cmd = true;
	tpd_cdev->tp_data = ilits;
	tpd_cdev->tp_resume_func = ilitek_ts_resume;
	tpd_cdev->tp_suspend_func = ilitek_ts_suspend;
	tpd_init_tpinfo(tpd_cdev);
	ilits->ilitek_ts_workqueue = create_singlethread_workqueue("ilitek ts workqueue");
	if (!ilits->ilitek_ts_workqueue) {
		ILI_INFO(" ilitek ts workqueue failed\n");
	} else  {
		INIT_DELAYED_WORK(&ilits->charger_work, ilitek_work_charger_detect_work);
		queue_delayed_work(ilits->ilitek_ts_workqueue, &ilits->charger_work, msecs_to_jiffies(1000));
		ilitek_init_charger_notifier();
	}

	return 0;
}

