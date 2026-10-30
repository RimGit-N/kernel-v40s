
/***********************
 * file : tpd_external_fun.c
 */
#include <linux/module.h>
#include <linux/err.h>
#include "tpd_sys.h"
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#define MAX_POINTS_SUPPORT 10
#define LONG_PRESS_MIN_COUNT 50
static void edge_point_report(int id);
static void is_not_edge_long_press(struct input_dev *input, u16 id);
static void point_report_reset(int id);
static bool is_have_other_point_down(int id);
static bool is_have_inside_point_down(void);
void tpd_touch_release(struct input_dev *input, u16 id);
struct notifier_block tpd_nb;

typedef struct point_info {
	int x;
	int y;
	u8 touch_major;
	u8 pressure;
} tpd_point_info_t;

typedef struct point_fifo {
	tpd_point_info_t point_data[2];
	tpd_point_info_t first_report_point_data;
	tpd_point_info_t last_point;
	tpd_point_info_t mistake_touch_check_point;
	bool is_report_point;
	bool save_first_down_point;
	bool is_moving_in_limit_area;
	bool finger_down;
	bool edge_finger_down;
	bool limit_area_log_print;
	bool is_inside_finger_down;
	bool jitter_check;
	bool mistake_touch_check;
	bool cancel_clean_edge_area_ponit;
	unsigned long touch_down_timer;
	unsigned long edge_down_timer;
	u16 long_press_count;
	struct input_dev *input;
} tpd_point_fifo_t;

tpd_point_fifo_t point_report_info[MAX_POINTS_SUPPORT];

#define tpd_idn_report_work(id)\
static void tpd_id##id##_report_work(struct work_struct *work)\
{\
	tpd_point_fifo_t *point = &point_report_info[id];\
	is_not_edge_long_press(point->input, id);\
}

tpd_idn_report_work(0)
tpd_idn_report_work(1)
tpd_idn_report_work(2)
tpd_idn_report_work(3)
tpd_idn_report_work(4)
tpd_idn_report_work(5)
tpd_idn_report_work(6)
tpd_idn_report_work(7)
tpd_idn_report_work(8)
tpd_idn_report_work(9)

#ifdef CONFIG_TP_DETECT_BY_LCDINFO
struct tp_ic_vendor_info tp_ic_vendor_info_l[] = {
	{TS_CHIP_SYNAPTICS, "synaptics"},
	{TS_CHIP_FOCAL, "focal"},
	{TS_CHIP_GOODIX, "goodix"	},
	{TS_CHIP_HIMAX, "himax"},
	{TS_CHIP_NOVATEK, "novatek"},
	{TS_CHIP_ILITEK, "ilitek"},
	{TS_CHIP_TLSC, "tlsc"},
	{TS_CHIP_CHIPONE, "chipone"},
	{TS_CHIP_GCORE, "galaxycore"},
	{TS_CHIP_OMNIVISION, "omnivision"},
	{TS_CHIP_MAX, "Unknown"},
};

int get_tp_chip_id(void)
{
	int i = 0;
	const char *panel_name = NULL;

	TPD_DMESG("%s:\n", __func__);
	tpd_fw_cdev.tp_chip_id = TS_CHIP_MAX;
	panel_name = get_lcd_panel_name();
	TPD_DMESG("%s: panel name %s.\n", __func__, panel_name);
	for (i = 0; i < ARRAY_SIZE(tp_ic_vendor_info_l); i++) {
		if (strnstr(panel_name, tp_ic_vendor_info_l[i].tp_ic_vendor_name, strlen(panel_name))) {
			tpd_fw_cdev.tp_chip_id = tp_ic_vendor_info_l[i].tp_chip_id;
			TPD_DMESG("%s: tp_chip_id is 0x%02x.\n", __func__, tpd_fw_cdev.tp_chip_id);
			return 0;
		}
	}
	return -EIO;
}
EXPORT_SYMBOL_GPL(get_tp_chip_id);
#endif

int tpd_gpio_shutdown_config(void)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->tpd_gpio_shutdown) {
		cdev->tpd_gpio_shutdown();
	}
	return 0;
}

int suspend_tp_need_awake(void)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->tpd_suspend_need_awake) {
		return cdev->tpd_suspend_need_awake(cdev);
	}
	return 0;
}

bool tp_esd_check(void)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->tpd_esd_check) {
		return cdev->tpd_esd_check(cdev);
	}
	return 0;
}

void set_lcd_reset_processing(bool enable)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (enable) {
		cdev->ignore_tp_irq = true;
	} else {
		cdev->ignore_tp_irq = false;
	}
	TPD_DMESG("cdev->ignore_tp_irq is %d.\n", cdev->ignore_tp_irq);
}

int set_gpio_mode(u8 mode)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->set_gpio_mode) {
		return cdev->set_gpio_mode(cdev, mode);
	}
	return -EIO;
}

static struct platform_device *tpd_platform_device = NULL;

static void tpd_report_uevent(u8 gesture_key)
{
	char *envp[2] = {NULL};

	switch (gesture_key) {
	case single_tap:
		TPD_DMESG("%s single tap gesture", __func__);
		envp[0] = "single_tap=true";
		break;
	case double_tap:
		TPD_DMESG("%s double tap gesture", __func__);
		envp[0] = "double_tap=true";
		break;
	case pen_low_batt:
		TPD_DMESG("%s pen low batt", __func__);
		envp[0] = "pen_capacity_low=true";
		break;
	default:
		TPD_DMESG("%s no such gesture key(%d)", __func__, gesture_key);
		return;
	}

	kobject_uevent_env(&(tpd_platform_device->dev.kobj), KOBJ_CHANGE, envp);
}

int tpd_uevent_init(void)
{
	int ret = 0;

	TPD_DMESG("%s", __func__);
	tpd_platform_device = platform_device_alloc("zte_touch", -1);
	if (!tpd_platform_device) {
		TPD_DMESG("%s failed to allocate platform device", __func__);
		ret = -ENOMEM;
		goto alloc_failed;
	}

	ret = platform_device_add(tpd_platform_device);
	if (ret < 0) {
		TPD_DMESG("%s failed to add platform device ret=%d", __func__, ret);
		goto register_failed;
	}

	tpd_fw_cdev.tpd_report_uevent = tpd_report_uevent;
	tpd_fw_cdev.platform_device = tpd_platform_device;
	return 0;

register_failed:
	tpd_platform_device->dev.release(&(tpd_platform_device->dev));
alloc_failed:
	tpd_fw_cdev.tpd_report_uevent = NULL;

	return ret;
}

void tpd_uevent_exit(void)
{
	if (tpd_fw_cdev.tpd_report_uevent != NULL) {
		tpd_platform_device->dev.release(&(tpd_platform_device->dev));
		platform_device_unregister(tpd_platform_device);
	}
}


void tpd_reset_gpio_output(bool value)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->tp_reset_gpio_output) {
		cdev->tp_reset_gpio_output(value);
	}
}

const char *get_lcd_panel_name(void)
{
	const char *panel_name = "Unknown_lcd";

#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	panel_name = zte_get_lcd_panel_name();
	if (panel_name == NULL)
		panel_name = "Unknown_lcd";
#endif
	return panel_name;
}

static bool point_in_report_judge_area(u16 x, u16 y)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->display_rotation == mRotatin_0) {
		if ((x < cdev->long_pess_suppression[0] * 3  / 2) || (x > cdev->max_x - cdev->long_pess_suppression[1] * 3 / 2))
			return true;
	}
	return false;
}

static bool point_is_in_limit_area(u16 x, u16 y)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->display_rotation == mRotatin_90 || cdev->display_rotation == mRotatin_270) {
		if ((x < cdev->edge_report_limit[0]) || (x > cdev->max_x - cdev->edge_report_limit[1]) ||
			(y < cdev->edge_report_limit[2]) || (y > cdev->max_y - cdev->edge_report_limit[3]))
			return true;
	} else {
		if ((x  < cdev->edge_report_limit[0]) || (x > cdev->max_x - cdev->edge_report_limit[1]))
			return true;
		if (point_in_report_judge_area(x, y) && is_have_inside_point_down()) {
			TPD_DMESG("have other point down and tpd Press in judge area: x = %d, y = %d\n", x, y);
			return true;
		}
		if (cdev->edge_limit_pixel_level > 0) {
			if ((y > cdev->user_edge_limit[1]) &&
				(((x < cdev->user_edge_limit[0]) || (x > cdev->max_x - cdev->user_edge_limit[0]))))
				return true;
		}
	}
	return false;
}

static bool point_in_long_pess_suppression_area(u16 x, u16 y)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if ((cdev->edge_long_press_check == false)
		|| (cdev->long_press_max_count < LONG_PRESS_MIN_COUNT))
		return false;
	if (cdev->display_rotation == mRotatin_90 || cdev->display_rotation == mRotatin_270) {
		if ((x < cdev->long_pess_suppression[0]) || (x > cdev->max_x - cdev->long_pess_suppression[1]) ||
			(y < cdev->long_pess_suppression[2]) || (y > cdev->max_y - cdev->long_pess_suppression[3]))
			return true;
		if (cdev->edge_limit_pixel_level > 0) {
			if ((y > cdev->user_edge_limit[1]) &&
				(((x < cdev->user_edge_limit[0]) || (x > cdev->max_x - cdev->user_edge_limit[0]))))
				return true;
		}
	} else {
		if ((x  < cdev->long_pess_suppression[0]) || (x > cdev->max_x - cdev->long_pess_suppression[1]))
			return true;
	}
	return false;
}

static bool is_need_clean_long_pess_area_down(tpd_point_fifo_t *point, u16 x, u16 y)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->edge_long_press_check == false || point->cancel_clean_edge_area_ponit) {
		return false;
	}
	if (cdev->display_rotation == mRotatin_90 || cdev->display_rotation == mRotatin_270) {
		return false;
	} else {
		if (x > cdev->max_x / 4 &&  x < cdev->max_x  * 3 / 4)
			point->cancel_clean_edge_area_ponit = true;
		if (point->first_report_point_data.x < cdev->max_x / 4 ||  point->first_report_point_data.x > cdev->max_x  * 3 / 4)
			return true;
	}
	return false;
}

static bool is_report_point(u16 x, u16 y, u16 id, u8 touch_major, u8  pressure)
{
	tpd_point_fifo_t *point = &point_report_info[id];
	u16 limit_pixel = 100;

	if (point->is_report_point) {
		if (is_need_clean_long_pess_area_down(point, x, y) == false)
			return true;
		if (point_in_long_pess_suppression_area(x, y)) {
			if (!point->mistake_touch_check) {
				point->mistake_touch_check_point.x = x;
				point->mistake_touch_check_point.y = y;
				point->mistake_touch_check = true;
				point->edge_down_timer = jiffies;
			}
			if(jiffies_to_msecs(jiffies - point->edge_down_timer) > 800) {
				if ((abs(point->mistake_touch_check_point.x  - x) > 20
					|| abs(point->mistake_touch_check_point.y - y) > 20)) {
					point->mistake_touch_check_point.x = x;
					point->mistake_touch_check_point.y = y;
					point->edge_down_timer = jiffies;
					return true;
				}
				tpd_touch_release(point->input,  id);
				point->mistake_touch_check = false;
				return false;
			} else {
				return true;
			}
		}
		point->mistake_touch_check = false;
		return true;
	}
	if (point_is_in_limit_area(x, y) || point_in_long_pess_suppression_area(x, y)) {
		if ((!point_is_in_limit_area(x, y) && point_in_long_pess_suppression_area(x, y))) {
			if (point->limit_area_log_print == false) {
				point->limit_area_log_print = true;
				TPD_DMESG("tpd Press in long pess suppression area: id = %d, x = %d, y = %d\n", id, x, y);
			}
			point->long_press_count++;
			if (is_have_inside_point_down()) {
				point->is_inside_finger_down = true;
			}
		} else {
			if (point->limit_area_log_print == false) {
				point->limit_area_log_print = true;
				TPD_DMESG("tpd Press in limit area: id = %d, x = %d, y = %d\n", id, x, y);
			}
		}
		if (point->save_first_down_point == false) {
			point->point_data[0].x = x;
			point->point_data[0].y = y;
			point->point_data[0].touch_major = touch_major;
			point->point_data[0].pressure = pressure;
			point->save_first_down_point = true;
			return false;
		}
		if (abs(point->point_data[0].x - x) > limit_pixel
			|| abs(point->point_data[0].y - y) > limit_pixel) {
			goto save_last_ponit;

		} else {
			return false;
		}
	}
save_last_ponit:
	if (point->save_first_down_point == false) {
		point->is_moving_in_limit_area = false;
		return true;
	}
	point->point_data[1].x = x;
	point->point_data[1].y = y;
	point->point_data[1].touch_major = touch_major;
	point->point_data[1].pressure = pressure;
	point->is_moving_in_limit_area = true;
	return true;
}

static void tpd_touch_report(struct input_dev *input, u16 x, u16 y, u16 id, u8 touch_major, u8  pressure)
{
#ifndef TYPE_A_PROTOCOL
	input_mt_slot(input, id);
	input_mt_report_slot_state(input, MT_TOOL_FINGER, true);
	input_report_key(input, BTN_TOUCH, 1);
	input_report_abs(input, ABS_MT_POSITION_X, x);
	input_report_abs(input, ABS_MT_POSITION_Y, y);
	if (pressure)
		input_report_abs(input, ABS_MT_PRESSURE, pressure);
	if (touch_major)
		input_report_abs(input, ABS_MT_TOUCH_MAJOR, touch_major);
#else
	input_report_key(input, BTN_TOUCH, 1);
	input_report_abs(input, ABS_MT_TRACKING_ID, id);
	input_report_abs(input, ABS_MT_POSITION_X, x);
	input_report_abs(input, ABS_MT_POSITION_Y, y);
	if (pressure)
		input_report_abs(input, ABS_MT_PRESSURE, pressure);
	if (touch_major)
		input_report_abs(input, ABS_MT_TOUCH_MAJOR, touch_major);
	input_mt_sync(input);
#endif
}

static bool is_have_other_point_down(int id)
{
	int i = 0;

	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		if (i == id)
			continue;
		if ((point_report_info[i].finger_down || point_report_info[i].edge_finger_down))
			return true;
	}
	return false;
}

static bool is_have_inside_point_down(void)
{
	int i = 0;

	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		if (point_report_info[i].finger_down)
			return true;
	}
	return false;
}

static void is_not_edge_long_press(struct input_dev *input, u16 id)
{
	tpd_point_fifo_t *point = &point_report_info[id];
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (is_have_inside_point_down()) {
		TPD_DMESG("%s:have inside point down", __func__);
		return;
	}
	mutex_lock(&cdev->report_mutex);
	tpd_touch_report(input, point->point_data[0].x, point->point_data[0].y,
				id, point->point_data[0].touch_major, point->point_data[0].pressure);
	input_sync(input);
	point->edge_finger_down = true;
	TPD_DMESG("%s:tpd touch down id: %d, coord [%d:%d]\n",
		__func__, id, point->point_data[0].x, point->point_data[0].y);
	mutex_unlock(&cdev->report_mutex);
	usleep_range(15000, 16000);
	if (point->edge_finger_down == false) {
		return;
	}
#ifndef TYPE_A_PROTOCOL
	mutex_lock(&cdev->report_mutex);
	input_mt_slot(input, id);
	input_mt_report_slot_state(input, MT_TOOL_FINGER, false);
	if (!is_have_other_point_down(id)) {
		input_report_key(input, BTN_TOUCH, 0);
	}
	input_sync(input);
	mutex_unlock(&cdev->report_mutex);
#endif
	point->edge_finger_down = false;
	TPD_DMESG("%s:tpd touch up id: %d, coord [%d:%d]\n",
		__func__, id, point->point_data[0].x, point->point_data[0].y);

}

void tpd_touch_press(struct input_dev *input, u16 x, u16 y, u16 id, u8 touch_major, u8  pressure)
{
	int i = 0;
	tpd_point_fifo_t *point = NULL;
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->display_rotation) {
		if (x == 0)
			x = x + 1;
		if (y == 0)
			y = y + 1;
	}

	if (input == NULL || id >= MAX_POINTS_SUPPORT) {
		TPD_DMESG("%s:input is NULL? id = %d", __func__, id);
		return;
	}
	point = &point_report_info[id];
	point->input = input;
	if (is_report_point(x, y, id, touch_major, pressure) == false) {
		return;
	}

	point->is_report_point = true;
	point->long_press_count = 0;
	if (point->is_moving_in_limit_area) {
		for (i = 0; i < 2; i++) {
			if (point->finger_down == false) {
				point->finger_down = true;
				point->first_report_point_data.x = point->point_data[0].x;
				point->first_report_point_data.y = point->point_data[0].y;
				point_report_reset(id);
				TPD_DMESG("tpd touch down id: %d, coord [%d:%d]\n",
					id, point->point_data[i].x, point->point_data[i].y);
			}
			tpd_touch_report(input, point->point_data[i].x, point->point_data[i].y,
				id, touch_major, pressure);
			if (i == 0) {
				input_sync(input);
				usleep_range(1000, 1500);
			}
		}
	} else {
#if CONFIG_JITTER_PIXEL
		if (point->finger_down == false) {
				point->finger_down = true;
				point_report_reset(id);
				point->first_report_point_data.x = x;
				point->first_report_point_data.y = y;
				point->touch_down_timer = jiffies;
				point->jitter_check = true;
				TPD_DMESG("tpd touch down id: %d, coord [%d:%d]. jitter check %d pixe\n",
					id, x, y, CONFIG_JITTER_PIXEL);
				tpd_touch_report(input, x, y, id, touch_major, pressure);
		} else {
			if (point->jitter_check) {
				if (jiffies_to_msecs(jiffies - point->touch_down_timer) > 100) {
					if (abs(point->first_report_point_data.x - x) > CONFIG_JITTER_PIXEL
						|| abs(point->first_report_point_data.y - y) > CONFIG_JITTER_PIXEL) {
						tpd_touch_report(input, x, y, id, touch_major, pressure);
						point->jitter_check = false;
					}
				} else if (abs(point->first_report_point_data.x - x) > CONFIG_JITTER_PIXEL * 3
						|| abs(point->first_report_point_data.y - y) > CONFIG_JITTER_PIXEL * 3) {
						tpd_touch_report(input, x, y, id, touch_major, pressure);
						point->jitter_check = false;
				}
			} else {
				tpd_touch_report(input, x, y, id, touch_major, pressure);
			}
		}
#else
		if (point->finger_down == false) {
				point->finger_down = true;
				point->first_report_point_data.x = x;
				point->first_report_point_data.y = y;
				point_report_reset(id);
				TPD_DMESG("tpd touch down id: %d, coord [%d:%d]\n", id, x, y);
		}
		tpd_touch_report(input, x, y, id, touch_major, pressure);
#endif
	}
	point->last_point.x = x;
	point->last_point.y = y;
	point->is_moving_in_limit_area = false;
}
EXPORT_SYMBOL_GPL(tpd_touch_press);

void tpd_touch_release(struct input_dev *input, u16 id)
{
	tpd_point_fifo_t *point = &point_report_info[id];
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (input == NULL || id > MAX_POINTS_SUPPORT) {
		TPD_DMESG("%s:input is NULL? id = %d", __func__, id);
		return;
	}
#ifndef TYPE_A_PROTOCOL
	if (point->finger_down) {
		input_mt_slot(input, id);
		input_mt_report_slot_state(input, MT_TOOL_FINGER, false);
		TPD_DMESG("tpd touch up id: %d, coord [%d:%d]\n", id, point->last_point.x, point->last_point.y);
	}
#endif
	if (cdev->edge_long_press_check && !point->is_inside_finger_down && (point->long_press_count > 0) &&
		(point->long_press_count < tpd_fw_cdev.long_press_max_count)) {
		edge_point_report(id);
	}
	point->finger_down = false;
	point->is_report_point = false;
	point->save_first_down_point = false;
	point->limit_area_log_print = false;
	point->is_inside_finger_down = false;
	point->jitter_check = false;
	point->mistake_touch_check = false;
	point->long_press_count = 0;
	point->cancel_clean_edge_area_ponit = false;
}
EXPORT_SYMBOL_GPL(tpd_touch_release);

void tpd_clean_all_event(void)
{
	int i = 0;

	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		point_report_info[i].finger_down = false;
		point_report_info[i].edge_finger_down = false;
		point_report_info[i].is_report_point = false;
		point_report_info[i].save_first_down_point = false;
		point_report_info[i].is_moving_in_limit_area = false;
		point_report_info[i].limit_area_log_print = false;
		point_report_info[i].is_inside_finger_down = false;
		point_report_info[i].jitter_check = false;
		point_report_info[i].mistake_touch_check = false;
		point_report_info[i].long_press_count = 0;
		point_report_info[i].cancel_clean_edge_area_ponit = false;
	}
}
EXPORT_SYMBOL_GPL(tpd_clean_all_event);

static void edge_point_report(int id)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;
	tpd_point_fifo_t *point = &point_report_info[id];

	TPD_DMESG("%s:tpd id:%d", __func__, id);
	if (!cdev->tpd_report_wq) {
		TPD_DMESG("%s:tpd_report_wq is null", __func__);
		return;
	}
	if (point->finger_down) {
		TPD_DMESG("%s:tpd id[%d]  have reported inside screcenl.", __func__, id);
		return;
	}
	switch (id) {
	case 0:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work0, msecs_to_jiffies(5));
		break;
	case 1:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work1, msecs_to_jiffies(5));
		break;
	case 2:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work2, msecs_to_jiffies(5));
		break;
	case 3:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work3, msecs_to_jiffies(5));
		break;
	case 4:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work4, msecs_to_jiffies(5));
		break;
	case 5:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work5, msecs_to_jiffies(5));
		break;
	case 6:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work6, msecs_to_jiffies(5));
		break;
	case 7:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work7, msecs_to_jiffies(5));
		break;
	case 8:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work8, msecs_to_jiffies(5));
		break;
	case 9:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work9, msecs_to_jiffies(5));
		break;
	default:
		TPD_DMESG("%s:error id %d", __func__, id);
	}
}

static void point_report_reset(int id)
{
	tpd_point_fifo_t *point = &point_report_info[id];

	if (point->edge_finger_down) {
		TPD_DMESG("%s:tpd touch up id: %d\n",  __func__, id);
		point->edge_finger_down = false;
#ifndef TYPE_A_PROTOCOL
		input_mt_slot(point->input, id);
		input_mt_report_slot_state(point->input, MT_TOOL_FINGER, false);
		input_sync(point->input);
		usleep_range(1000, 1100);
#endif
	}
}

#ifdef CONFIG_TPD_UFP_MAC
extern void ufp_report_lcd_state(void);

static void ufp_report_lcd_state_work(struct work_struct *work)
{
	ufp_report_lcd_state();
}

void ufp_report_lcd_state_delayed_work(u32 ms)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	mod_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_lcd_state_work, msecs_to_jiffies(ms));

}

void cancel_report_lcd_state_delayed_work(void)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	cancel_delayed_work_sync(&cdev->tpd_report_lcd_state_work);

}
#endif
#ifdef CONFIG_TS_POINT_REPORT_CHECK
static void ts_point_report_check(struct work_struct *work)
{
	int id = 0;
	struct input_dev *input= point_report_info[0].input;

	if (input) {
		TPD_DMESG("Release all touch");

		for (id = MAX_POINTS_SUPPORT - 1; id >= 0; id--) {
			input_mt_slot(input, id);
			input_mt_report_slot_state(input, MT_TOOL_FINGER, false);
		}
		input_report_key(input, BTN_TOUCH, 0);
		input_sync(input);
		tpd_clean_all_event();
	}
}
#endif

void tpd_workquue_init(void)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	TPD_DMESG("%s enter", __func__);
	cdev->tpd_report_wq = create_singlethread_workqueue("tpd_report_wq");

	if (!cdev->tpd_report_wq) {
		goto err_create_tpd_report_wq_failed;
	}
	INIT_DELAYED_WORK(&cdev->tpd_report_work0, tpd_id0_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work1, tpd_id1_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work2, tpd_id2_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work3, tpd_id3_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work4, tpd_id4_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work5, tpd_id5_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work6, tpd_id6_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work7, tpd_id7_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work8, tpd_id8_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work9, tpd_id9_report_work);
#ifdef CONFIG_TPD_UFP_MAC
	INIT_DELAYED_WORK(&cdev->tpd_report_lcd_state_work, ufp_report_lcd_state_work);
#endif
#ifdef CONFIG_TS_POINT_REPORT_CHECK
	INIT_DELAYED_WORK(&cdev->point_report_check_work, ts_point_report_check);
#endif
	return;
err_create_tpd_report_wq_failed:
	TPD_DMESG("%s: create tpd report workqueue failed\n", __func__);
}

void tpd_workquue_deinit(void)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	TPD_DMESG("%s enter", __func__);
	cancel_delayed_work_sync(&cdev->tpd_report_work0);
	cancel_delayed_work_sync(&cdev->tpd_report_work1);
	cancel_delayed_work_sync(&cdev->tpd_report_work2);
	cancel_delayed_work_sync(&cdev->tpd_report_work3);
	cancel_delayed_work_sync(&cdev->tpd_report_work4);
	cancel_delayed_work_sync(&cdev->tpd_report_work5);
	cancel_delayed_work_sync(&cdev->tpd_report_work6);
	cancel_delayed_work_sync(&cdev->tpd_report_work7);
	cancel_delayed_work_sync(&cdev->tpd_report_work8);
	cancel_delayed_work_sync(&cdev->tpd_report_work9);
#ifdef CONFIG_TPD_UFP_MAC
	cancel_delayed_work_sync(&cdev->tpd_report_lcd_state_work);
#endif
#ifdef CONFIG_TS_POINT_REPORT_CHECK
	cancel_delayed_work_sync(&cdev->point_report_check_work);
#endif
	if (!cdev->tpd_report_wq) {
		destroy_workqueue(cdev->tpd_report_wq);
	}
}

#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
static int tpd_lcd_notifier_callback(struct notifier_block *self,
	unsigned long event, void *data)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	switch (event) {
	case LCD_POWER_ON:
		TPD_DMESG("%s: lcd power on\n", __func__);
		set_lcd_reset_processing(true);
		tpd_reset_gpio_output(1);
		break;
	case LCD_RESET:
		TPD_DMESG("%s: lcd reset\n", __func__);
		if (cdev->tp_resume_before_lcd_cmd)
			 change_tp_state(ON);
		break;
	case LCD_CMD_ON:
		TPD_DMESG("%s: lcd cmd on\n", __func__);
		if (!cdev->tp_resume_before_lcd_cmd)
			change_tp_state(ON);
		set_lcd_reset_processing(false);
		tpd_fw_cdev.tp_suspend_write_gesture = false;
		break;
	case LCD_CMD_OFF:
		TPD_DMESG("%s: lcd cmd off\n", __func__);
		if (suspend_tp_need_awake())
			tpd_notifier_call_chain(LCD_SUSPEND_POWER_ON);
		else
			tpd_notifier_call_chain(LCD_SUSPEND_POWER_OFF);
		change_tp_state(OFF);

		break;
	case LCD_POWER_OFF:
		TPD_DMESG("%s: lcd power off\n", __func__);
		break;
	case LCD_POWER_OFF_RESET_LOW:
		TPD_DMESG("%s: lcd power off reset low\n", __func__);
		 tpd_reset_gpio_output(0);
		break;
	case LCD_SHUTDOWN:
		TPD_DMESG("%s: lcd shutdown\n", __func__);
		 tpd_reset_gpio_output(0);
		break;
	case LCD_ENTER_AOD:
		TPD_DMESG("%s: lcd enter aod\n", __func__);
#ifdef CONFIG_TPD_UFP_MAC
		ufp_notifier_cb(true);
		ufp_report_lcd_state_delayed_work(50);
#endif
		break;
	case LCD_EXIT_AOD:
		TPD_DMESG("%s: lcd exit aod\n", __func__);
#ifdef CONFIG_TPD_UFP_MAC
		ufp_notifier_cb(false);
#endif
		break;
	default:
		TPD_DMESG("%s: lcd state unknown\n", __func__);
		break;
	}
	return 0;
}

void lcd_notify_register(void)
{
	int ret = 0;

	tpd_nb.notifier_call = tpd_lcd_notifier_callback;
	ret = lcd_notifer_register_client(&tpd_nb);
	if (ret) {
		TPD_DMESG(" Unable to register fb_notifier: %d\n", ret);
	}
}
void lcd_notify_unregister(void)
{
	lcd_notifer_unregister_client(&tpd_nb);
}

void tp_free_tp_firmware_data(void)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	if (cdev->tp_firmware != NULL) {
		if (cdev->tp_firmware->data != NULL) {
			vfree(cdev->tp_firmware->data);
			cdev->tp_firmware->data = NULL;
			cdev->tp_firmware->size = 0;
		}
		kfree(cdev->tp_firmware);
		cdev->tp_firmware = NULL;
	}
	cdev->fw_data_pos = 0;
}


int tp_alloc_tp_firmware_data(int buf_size)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	tp_free_tp_firmware_data();
	cdev->tp_firmware = kzalloc(sizeof(struct firmware), GFP_KERNEL);
	if (cdev->tp_firmware == NULL) {
		pr_err("tpd:alloc struct firmware failed");
		return -ENOMEM;
	}
	cdev->tp_firmware->data = vmalloc(sizeof(*cdev->tp_firmware) + buf_size);
	if (!cdev->tp_firmware->data) {
		pr_err("tpd: alloc tp_firmware->data failed");
		kfree(cdev->tp_firmware);
		return -ENOMEM;
	}
	cdev->tp_firmware->size = buf_size;
	memset((char *)cdev->tp_firmware->data, 0x00, sizeof(*cdev->tp_firmware) + buf_size);
	return 0;
}

int  tpd_copy_to_tp_firmware_data(char *buf)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;
	int len = 0;

	if (!cdev->tp_firmware || !cdev->tp_firmware->data) {
		pr_err("Need set fw image size first");
		return -ENOMEM;
	}

	if (cdev->tp_firmware->size == 0) {
		pr_err("Invalid firmware size");
		return -EINVAL;
	}

	if (cdev->fw_data_pos >= cdev->tp_firmware->size)
		return 0;
	len = strlen(buf);
	if (cdev->fw_data_pos + len > cdev->tp_firmware->size)
		len = cdev->tp_firmware->size - cdev->fw_data_pos;
	memcpy((u8 *)&cdev->tp_firmware->data[cdev->fw_data_pos], buf, len);
	cdev->fw_data_pos += len;
	return len;
}

void  tpd_reset_fw_data_pos_and_size(void)
{
	struct tpd_classdev_t *cdev = &tpd_fw_cdev;

	cdev->tp_firmware->size = cdev->fw_data_pos;
	cdev->fw_data_pos = 0;
}
#endif
