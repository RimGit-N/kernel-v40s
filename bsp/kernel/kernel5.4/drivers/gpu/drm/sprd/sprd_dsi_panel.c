// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Unisoc Inc.
 */

#include <linux/backlight.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <video/of_display_timing.h>
#include <video/mipi_display.h>
#include <video/videomode.h>

#include <drm/drm_atomic_helper.h>

#include "sprd_crtc.h"
#include "sprd_dpu.h"
#include "sprd_dsi_panel.h"
#include "dsi/sprd_dsi_api.h"
#include "sysfs/sysfs_display.h"

#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
#include "zte_lcd_common.h"
extern struct sprd_panel *g_zte_ctrl_pdata;
#ifdef CONFIG_ZTE_LCDBL_I2C_CTRL_VSP_VSN
extern void tps65132b_set_vsp_vsn_level(u8 level);
extern struct sprd_dsi *g_dsi;
#endif
/*#define DRM_MODE_DPMS_ON	0
#define DRM_MODE_DPMS_STANDBY	1
#define DRM_MODE_DPMS_SUSPEND	2
#define DRM_MODE_DPMS_OFF	3
*/
#ifdef CONFIG_ZTE_LCD_HBM_CTRL
static char zte_drm_mode[][25] = {
{"DRM_MODE_DPMS_ON"},
{"DRM_MODE_DPMS_STANDBY"},
{"DRM_MODE_DPMS_SUSPEND"},
{"DRM_MODE_DPMS_OFF"}
};
#endif
#endif
#define SPRD_MIPI_DSI_FMT_DSC 0xff

#ifdef CONFIG_ZTE_LCD_ICNL9911C_SET_VOLTAGE
static int sprd_panel_send_cmds(struct mipi_dsi_device *dsi,
				const void *data, int size);
static u8 icnl9911c_set_voltage_data[] = {
0x39, 0x00, 0x00, 0x03, 0xF0, 0x5A, 0x59,
0x39, 0x00, 0x00, 0x03, 0xF1, 0xA5, 0xA6,
0x39, 0x00, 0x00, 0x02, 0xf6, 0x3f,
0x39, 0x00, 0x00, 0x03, 0xf1, 0x5a, 0x59,
0x39, 0x00, 0x00, 0x03, 0xF0, 0xA5, 0xA6};

#define ICNL9911C_SET_VOLTAGE_LEN (34) /*sizeof(icnl9911c_set_voltage_data)*/
static uint32_t icnl9911c_f6_value_from_uboot = 0;
static int __init icnl9911c_f6_value_get(char *str)
{
	int len = 0;

	len = kstrtou32(str, 10, &icnl9911c_f6_value_from_uboot);
	pr_info("icnl9911c_f6_value_from_uboot=0x%x\n", icnl9911c_f6_value_from_uboot);
	return 0;
}
__setup("icnl9911c_f6=", icnl9911c_f6_value_get);
static void zte_set_icnl9911c_voltage(struct mipi_dsi_device *dsi)
{

	if (icnl9911c_f6_value_from_uboot > 0) {
		icnl9911c_set_voltage_data[19] = icnl9911c_f6_value_from_uboot;
	} else {
		icnl9911c_set_voltage_data[19] = 0x3f;
	}

	sprd_panel_send_cmds(dsi, icnl9911c_set_voltage_data, ICNL9911C_SET_VOLTAGE_LEN);
}
#endif
#define host_to_dsi(host) \
	container_of(host, struct sprd_dsi, host)

static inline struct sprd_panel *to_sprd_panel(struct drm_panel *panel)
{
	return container_of(panel, struct sprd_panel, base);
}

static int sprd_panel_send_cmds(struct mipi_dsi_device *dsi,
				const void *data, int size)
{
	struct sprd_panel *panel;
	const struct dsi_cmd_desc *cmds = data;
	u16 len;

	if ((cmds == NULL) || (dsi == NULL))
		return -EINVAL;

	panel = mipi_dsi_get_drvdata(dsi);

	while (size > 0) {
		len = (cmds->wc_h << 8) | cmds->wc_l;

		if (panel->info.use_dcs)
			mipi_dsi_dcs_write_buffer(dsi, cmds->payload, len);
		else
			mipi_dsi_generic_write(dsi, cmds->payload, len);

		if (cmds->wait)
			msleep(cmds->wait);
		cmds = (const struct dsi_cmd_desc *)(cmds->payload + len);
		size -= (len + 4);
	}

	return 0;
}

int sprd_panel_unprepare(struct drm_panel *p)
{
	struct sprd_panel *panel = to_sprd_panel(p);
	struct gpio_timing *timing;
	int items, i;

	DRM_INFO("%s()\n", __func__);
	#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	if (g_zte_ctrl_pdata->lcd_suspend_power_on) {
		DRM_INFO("%s() tp suspend need power on always\n", __func__);
		return 0;
	}
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->reset_before_vsp) {
		if (panel->info.avee_gpio) {
			gpiod_direction_output(panel->info.avee_gpio, 0);
			mdelay(5);
		}

		if (panel->info.avdd_gpio) {
			gpiod_direction_output(panel->info.avdd_gpio, 0);
			mdelay(5);
		}
	}
	#endif

	if (panel->info.reset_gpio) {
		items = panel->info.rst_off_seq.items;
		timing = panel->info.rst_off_seq.timing;
		for (i = 0; i < items; i++) {
			gpiod_direction_output(panel->info.reset_gpio,
						timing[i].level);
			mdelay(timing[i].delay);
		}
	}
#ifdef CONFIG_TOUCHSCREEN_VENDOR
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_ctrl_tp_resetpin) {
		tpd_reset_gpio_output(0);
		ZTE_LCD_INFO("lcd driver set tp rst to 0\n");
	}
#endif

	if (panel->info.avee_gpio) {
		gpiod_direction_output(panel->info.avee_gpio, 0);
		mdelay(5);
	}

	if (panel->info.avdd_gpio) {
		gpiod_direction_output(panel->info.avdd_gpio, 0);
		mdelay(5);
	}

	regulator_disable(panel->supply);
#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	lcd_notifier_call_chain(LCD_POWER_OFF);
#endif

	return 0;
}

int sprd_panel_prepare(struct drm_panel *p)
{
	struct sprd_panel *panel = to_sprd_panel(p);
	struct gpio_timing *timing;
	int items, i, ret;

	DRM_INFO("%s()\n", __func__);
#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	if (g_zte_ctrl_pdata->lcd_suspend_power_on == false)
#endif
	{
		DRM_INFO("%s() tp suspend need power on always, don't need power on again\n", __func__);

		ret = regulator_enable(panel->supply);
		if (ret < 0)
			DRM_ERROR("enable lcd regulator failed\n");

		if (panel->info.avdd_gpio) {
			gpiod_direction_output(panel->info.avdd_gpio, 1);
			mdelay(5);
		}

		if (panel->info.avee_gpio) {
			gpiod_direction_output(panel->info.avee_gpio, 1);
			mdelay(5);
		}
	}
	#ifdef CONFIG_ZTE_LCDBL_I2C_CTRL_VSP_VSN
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_vsp_vsn_voltage != 0x0)
		tps65132b_set_vsp_vsn_level(g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_vsp_vsn_voltage);
	#endif

#ifdef CONFIG_TOUCHSCREEN_VENDOR
	mdelay(1);
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_ctrl_tp_resetpin) {
		tpd_reset_gpio_output(1);
		ZTE_LCD_INFO("lcd driver set tp rst to 1\n");
	}
#endif

#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	lcd_notifier_call_chain(LCD_POWER_ON);
#endif
	if (panel->info.reset_gpio) {
		items = panel->info.rst_on_seq.items;
		timing = panel->info.rst_on_seq.timing;
		for (i = 0; i < items; i++) {
			gpiod_direction_output(panel->info.reset_gpio,
						timing[i].level);
			mdelay(timing[i].delay);
		}
	}
#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	lcd_notifier_call_chain(LCD_RESET);
#endif

	return 0;
}

#ifdef CONFIG_ZTE_LCD_HBM_CTRL
extern int panel_hbm_ctrl_RM692C9display(u32 setHbm);
int sprd_panel_set_hbm(u32 setHbm)
{
	int ret = 0;

	mutex_lock(&g_zte_ctrl_pdata->lock);
	ret = panel_hbm_ctrl_RM692C9display(setHbm);
	mutex_unlock(&g_zte_ctrl_pdata->lock);

	return ret;
}
extern int panel_set_aod_brightness(u32 level);
int sprd_panel_set_aod_bl(u32 level)
{
	int ret = 0;

	mutex_lock(&g_zte_ctrl_pdata->lock);
	ret = panel_set_aod_brightness(level);
	mutex_unlock(&g_zte_ctrl_pdata->lock);

	return ret;
}

#ifdef CONFIG_ZTE_LCD_SUPPORT_SD5302
#define HDR_MAX_BACKLIGHT_LEVEL   0XC70
#else
#define HDR_MAX_BACKLIGHT_LEVEL   0XFFF
#endif
struct backlight_device *g_oeld_bdev = NULL;
#endif

void  sprd_panel_enter_doze(struct drm_panel *p)
{
	struct sprd_panel *panel = to_sprd_panel(p);

	DRM_INFO("%s() enter cmds_len=%d\n", __func__, panel->info.cmds_len[CMD_CODE_DOZE_IN]);

	mutex_lock(&panel->lock);

	if (panel->esd_work_pending) {
		cancel_delayed_work_sync(&panel->esd_work);
		panel->esd_work_pending = false;
	}

	sprd_panel_send_cmds(panel->slave,
	       panel->info.cmds[CMD_CODE_DOZE_IN],
	       panel->info.cmds_len[CMD_CODE_DOZE_IN]);
#ifdef CONFIG_ZTE_LCD_HBM_CTRL
	usleep_range(17000, 17010);
	panel_set_aod_brightness(g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_aod_brightness);
	g_zte_ctrl_pdata->zte_lcd_ctrl->exit_doze = 0;
#endif
#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	lcd_notifier_call_chain(LCD_ENTER_AOD);
#endif
	DRM_INFO("%s() end\n", __func__);
	mutex_unlock(&panel->lock);
}

void  sprd_panel_exit_doze(struct drm_panel *p)
{
	struct sprd_panel *panel = to_sprd_panel(p);

	DRM_INFO("%s() enter cmds_len=%d\n", __func__, panel->info.cmds_len[CMD_CODE_DOZE_OUT]);

	mutex_lock(&panel->lock);

	sprd_panel_send_cmds(panel->slave,
		panel->info.cmds[CMD_CODE_DOZE_OUT],
		panel->info.cmds_len[CMD_CODE_DOZE_OUT]);

	if (panel->info.esd_check_en) {
		schedule_delayed_work(&panel->esd_work,
				      msecs_to_jiffies(1000));
		panel->esd_work_pending = true;
	}
#ifdef CONFIG_ZTE_LCD_HBM_CTRL
	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_aod_brightness = 1;
	g_zte_ctrl_pdata->zte_lcd_ctrl->exit_doze = 1;
#endif
#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	lcd_notifier_call_chain(LCD_EXIT_AOD);
#endif
	mutex_unlock(&panel->lock);
}

int sprd_panel_disable(struct drm_panel *p)
{
	struct sprd_panel *panel = to_sprd_panel(p);

	DRM_INFO("%s() cmds_len=%d\n", __func__, panel->info.cmds_len[CMD_CODE_SLEEP_IN]);

	mutex_lock(&panel->lock);
	/*
	 * FIXME:
	 * The cancel work should be executed before DPU stop,
	 * otherwise the esd check will be failed if the DPU
	 * stopped in video mode and the DSI has not change to
	 * CMD mode yet. Since there is no VBLANK timing for
	 * LP cmd transmission.
	 */
#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	lcd_notifier_call_chain(LCD_CMD_OFF);
#endif
	#ifdef CONFIG_ZTE_LCD_SUPPORT_ER68577
	if ((g_zte_ctrl_pdata != NULL) && (g_zte_ctrl_pdata->zte_lcd_ctrl != NULL)
		&& g_zte_ctrl_pdata->zte_lcd_ctrl->is_support_er68577) {
		sprd_panel_send_cmds(panel->slave,
			     panel->info.cmds[CMD_CODE_SLEEP_IN],
			     panel->info.cmds_len[CMD_CODE_SLEEP_IN]);
	}
	#endif

	if (panel->esd_work_pending) {
		cancel_delayed_work_sync(&panel->esd_work);
		panel->esd_work_pending = false;
	}

#ifdef CONFIG_ZTE_LCD_HBM_CTRL
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode != 0) {
		DRM_INFO("%s() close hbm\n", __func__);
		panel_hbm_ctrl_RM692C9display(0);
		g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode = 0;
	}
	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_aod_brightness = 1;
	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on = 0;
	g_zte_ctrl_pdata->zte_lcd_ctrl->exit_doze = 0;
#endif

	if (panel->backlight) {
		panel->backlight->props.power = FB_BLANK_POWERDOWN;
		panel->backlight->props.state |= BL_CORE_FBBLANK;
		backlight_update_status(panel->backlight);
	}

	#ifdef CONFIG_ZTE_LCD_SUPPORT_ER68577
	if ((g_zte_ctrl_pdata != NULL) && (g_zte_ctrl_pdata->zte_lcd_ctrl != NULL)
		&& !g_zte_ctrl_pdata->zte_lcd_ctrl->is_support_er68577) {
		sprd_panel_send_cmds(panel->slave,
			     panel->info.cmds[CMD_CODE_SLEEP_IN],
			     panel->info.cmds_len[CMD_CODE_SLEEP_IN]);
	}
	#else
	sprd_panel_send_cmds(panel->slave,
			     panel->info.cmds[CMD_CODE_SLEEP_IN],
			     panel->info.cmds_len[CMD_CODE_SLEEP_IN]);
	#endif

	panel->enabled = false;
	mutex_unlock(&panel->lock);

	return 0;
}

int sprd_panel_enable(struct drm_panel *p)
{
	struct sprd_panel *panel = to_sprd_panel(p);

	DRM_INFO("%s() cmds_len=%d\n", __func__, panel->info.cmds_len[CMD_CODE_INIT]);

	mutex_lock(&panel->lock);
	sprd_panel_send_cmds(panel->slave,
			     panel->info.cmds[CMD_CODE_INIT],
			     panel->info.cmds_len[CMD_CODE_INIT]);
#ifdef CONFIG_ZTE_LCD_ICNL9911C_SET_VOLTAGE
	if (icnl9911c_f6_value_from_uboot > 0) {
		DRM_INFO("icnl9911c_f6=0x%x\n", icnl9911c_f6_value_from_uboot);
		zte_set_icnl9911c_voltage(panel->slave);
	} else {
		DRM_INFO("This panel ic is not icnl9911c\n");
	}
#endif
#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	if (g_zte_ctrl_pdata->dsi->ctx.dpms == DRM_MODE_DPMS_ON)
		lcd_notifier_call_chain(LCD_CMD_ON);
#endif
	if (panel->backlight) {
		panel->backlight->props.power = FB_BLANK_UNBLANK;
		panel->backlight->props.state &= ~BL_CORE_FBBLANK;
		backlight_update_status(panel->backlight);
	}

	if (panel->info.esd_check_en) {
		schedule_delayed_work(&panel->esd_work,
				      msecs_to_jiffies(1000));
		panel->esd_work_pending = true;
	}

	panel->enabled = true;
	mutex_unlock(&panel->lock);

	return 0;
}

static int sprd_panel_get_modes(struct drm_panel *p)
{
	struct drm_display_mode *mode;
	struct sprd_panel *panel = to_sprd_panel(p);
	struct sprd_dsi *dsi =
		container_of(p->connector, struct sprd_dsi, connector);
	struct device_node *np = panel->slave->dev.of_node;
	u32 surface_width = 0, surface_height = 0;
	int i, mode_count = 0;

	DRM_INFO("%s()\n", __func__);
	mode = drm_mode_duplicate(p->drm, &panel->info.mode);
	if (!mode) {
		DRM_ERROR("failed to alloc mode %s\n", panel->info.mode.name);
		return 0;
	}
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(p->connector, mode);
	mode_count++;

	for (i = 0; i < panel->info.num_buildin_modes - 1; i++) {
		mode = drm_mode_duplicate(p->drm,
			&(panel->info.buildin_modes[i]));
		if (!mode) {
			DRM_ERROR("failed to alloc mode %s\n",
				panel->info.buildin_modes[i].name);
			return 0;
		}

		mode->type = DRM_MODE_TYPE_DRIVER;
		drm_mode_probed_add(p->connector, mode);
		mode_count++;
	}

	of_property_read_u32(np, "sprd,surface-width", &surface_width);
	of_property_read_u32(np, "sprd,surface-height", &surface_height);
	if (surface_width && surface_height) {
		struct videomode vm = {};

		vm.hactive = surface_width;
		vm.vactive = surface_height;
		vm.pixelclock = surface_width * surface_height * 60;

		mode = drm_mode_create(p->drm);

		mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_USERDEF;
		mode->vrefresh = 60;
		drm_display_mode_from_videomode(&vm, mode);
		drm_mode_probed_add(p->connector, mode);
		mode_count++;
		dsi->ctx.surface_mode = true;
	}

	p->connector->display_info.width_mm = panel->info.mode.width_mm;
	p->connector->display_info.height_mm = panel->info.mode.height_mm;

	return mode_count;
}

static const struct drm_panel_funcs sprd_panel_funcs = {
	.get_modes = sprd_panel_get_modes,
	.enable = sprd_panel_enable,
	.disable = sprd_panel_disable,
	.prepare = sprd_panel_prepare,
	.unprepare = sprd_panel_unprepare,
};

static int sprd_panel_esd_check(struct sprd_panel *panel)
{
	struct panel_info *info = &panel->info;
	u8 read_val = 0;

	if (!panel->base.connector ||
		!panel->base.connector->encoder ||
		!panel->base.connector->encoder->crtc) {
		return 0;
	}

	/* FIXME: we should enable HS cmd tx here */
	mipi_dsi_set_maximum_return_packet_size(panel->slave, 1);
	mipi_dsi_dcs_read(panel->slave, info->esd_check_reg,
			  &read_val, 1);

	/*
	 * TODO:
	 * Should we support multi-registers check in the future?
	 */
	if (read_val != info->esd_check_val) {
		DRM_ERROR("esd check failed, read value = 0x%02x\n",
			  read_val);
		return -EINVAL;
	}

	return 0;
}

static int sprd_panel_te_check(struct sprd_panel *panel)
{
	struct sprd_dpu *dpu;
	struct sprd_crtc *crtc;
	int ret;
	bool irq_occur = false;

	if (!panel->base.connector ||
	    !panel->base.connector->encoder ||
	    !panel->base.connector->encoder->crtc) {
		return 0;
	}

	crtc = to_sprd_crtc(panel->base.connector->encoder->crtc);
	dpu = (struct sprd_dpu *)crtc->priv;

	/* DPU TE irq maybe enabled in kernel */
	if (!dpu->ctx.enabled)
		return 0;

	dpu->ctx.te_check_en = true;

	/* wait for TE interrupt */
	ret = wait_event_interruptible_timeout(dpu->ctx.te_wq,
		dpu->ctx.evt_te, msecs_to_jiffies(500));
	if (!ret) {
		/* double check TE interrupt through dpu_int_raw register */
		if (dpu->core && dpu->core->check_raw_int) {
			down(&dpu->ctx.lock);
			if (dpu->ctx.enabled)
				irq_occur = dpu->core->check_raw_int(&dpu->ctx,
					BIT_DPU_INT_TE);
			up(&dpu->ctx.lock);
			if (!irq_occur) {
				DRM_ERROR("TE esd timeout.\n");
				ret = -ETIMEDOUT;
			} else
				DRM_WARN("TE occur, but isr schedule delay\n");
		} else {
			DRM_ERROR("TE esd timeout.\n");
			ret = -ETIMEDOUT;
		}
	}

	dpu->ctx.te_check_en = false;
	dpu->ctx.evt_te = false;

	return ret < 0 ? ret : 0;
}

#ifdef CONFIG_ZTE_LCD_ESD_ERROR_CTRL
struct backlight_device *g_bdev = NULL;
static void zte_set_lcd_ic_pwm(struct sprd_panel *panel)
{
	int level, brightness;
	struct sprd_oled *oled = NULL;

	if (g_zte_ctrl_pdata->zte_lcd_ctrl->backlight_ctrl_by_lcdic_pwm) {

		if (!g_bdev)
			return;

		oled = bl_get_data(g_bdev);

		mutex_lock(&panel->lock);
		if (!panel->enabled) {
			mutex_unlock(&panel->lock);
			DRM_WARN("oled panel has been powered off\n");
			return;
		}

		brightness = g_bdev->props.brightness;
		level = brightness * oled->max_level / 255;
		#if defined(CONFIG_ZTE_LCD_BACKLIGHT_LEVEL_CURVE)
			if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 12) {
				level = g_zte_ctrl_pdata->zte_lcd_ctrl->zte_convert_brightness(level, 4095);
			} else if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 11) {
				level = g_zte_ctrl_pdata->zte_lcd_ctrl->zte_convert_brightness(level, 2047);
			} else if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 10) {
				level = g_zte_ctrl_pdata->zte_lcd_ctrl->zte_convert_brightness(level, 1023);
			} else {
				level = g_zte_ctrl_pdata->zte_lcd_ctrl->zte_convert_brightness(level, 255);
			}
		#endif

		if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 10) {
			if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len_is_ilitek == 1) {
				oled->cmds[0]->payload[1] = (uint8_t) (level >> 6) & 0x0f;
				oled->cmds[0]->payload[2] = (uint8_t) (level << 2) & 0xfc;
			} else {
				oled->cmds[0]->payload[1] = (uint8_t) (level >> 2) & 0xff;
				oled->cmds[0]->payload[2] = (uint8_t) (level << 2) & 0x0C;
			}
		} else if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len > 8 &&
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len <= 12) {
			oled->cmds[0]->payload[1] = (uint8_t) (level >> 8) & 0x0f;
			oled->cmds[0]->payload[2] = (uint8_t) level & 0xff;
		} else {
			oled->cmds[0]->payload[1] = level;
		}

		#ifdef CONFIG_ZTE_LCD_SUPPORT_ER68577
		if ((g_zte_ctrl_pdata != NULL) && (g_zte_ctrl_pdata->zte_lcd_ctrl != NULL)
			&& g_zte_ctrl_pdata->zte_lcd_ctrl->is_support_er68577 && g_dsi != NULL) {
			sprd_dsi_lp_cmd_enable(g_dsi, true);
		}
		#endif
		sprd_panel_send_cmds(panel->slave, oled->cmds[0], oled->cmd_len);
		ZTE_LCD_INFO("%s:esd check set level=%d\n", __func__, level);
		mutex_unlock(&panel->lock);
	}
}
#endif
static int sprd_panel_mix_check(struct sprd_panel *panel)
{
	int ret;

	ret = sprd_panel_esd_check(panel);
	if (ret) {
		DRM_ERROR("mix check use read reg with error\n");
		return ret;
	}

	ret = sprd_panel_te_check(panel);
	if (ret) {
		DRM_ERROR("mix check use te signal with error\n");
		return ret;
	}

	return 0;
}

static void sprd_panel_esd_work_func(struct work_struct *work)
{
	struct sprd_panel *panel = container_of(work, struct sprd_panel,
						esd_work.work);
	struct panel_info *info = &panel->info;
	struct mipi_dsi_host *host = panel->slave->host;
	struct sprd_dsi *dsi = host_to_dsi(host);
	int ret;
	#ifdef CONFIG_ZTE_LCD_ESD_ERROR_CTRL
	static uint32_t esd_read_error_num = 0;
	#endif

	if ((!dsi->ctx.enabled) || (!panel->enabled)) {
		DRM_ERROR("dsi is suspended, skip esd work\n");
		return;
	}

	if (info->esd_check_mode == ESD_MODE_REG_CHECK)
		ret = sprd_panel_esd_check(panel);
	else if (info->esd_check_mode == ESD_MODE_TE_CHECK)
		ret = sprd_panel_te_check(panel);
	else if (info->esd_check_mode == ESD_MODE_MIX_CHECK)
		ret = sprd_panel_mix_check(panel);
	else {
		DRM_ERROR("unknown esd check mode:%d\n", info->esd_check_mode);
		return;
	}

	#ifdef CONFIG_ZTE_LCD_ESD_ERROR_CTRL
	if (ret) {
		esd_read_error_num++;
		DRM_INFO("ret=%d,esd_read_error_num=%d\n", ret, esd_read_error_num);
		if (esd_read_error_num >= 2) {
			esd_read_error_num = 0;
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_esd_num++;
			ret = -1;
		} else {
			ret = 0;
		}
	} else {
		esd_read_error_num = 0;
		ret = 0;
	}
	#endif

	if (ret && panel->base.connector && panel->base.connector->encoder) {
		const struct drm_encoder_helper_funcs *funcs;
		struct drm_encoder *encoder;

		encoder = panel->base.connector->encoder;
		funcs = encoder->helper_private;
		panel->esd_work_pending = false;

		if (!encoder->crtc || (encoder->crtc->state &&
		    !encoder->crtc->state->active)) {
			DRM_INFO("skip esd recovery during panel suspend\n");
			return;
		}

		DRM_INFO("====== esd recovery start ========\n");
		funcs->disable(encoder);
		funcs->enable(encoder);
		#ifdef CONFIG_ZTE_LCD_ESD_ERROR_CTRL
		zte_set_lcd_ic_pwm(panel);
		#endif
		DRM_INFO("======= esd recovery end =========\n");
	} else
		schedule_delayed_work(&panel->esd_work,
			msecs_to_jiffies(info->esd_check_period));
}

static int sprd_panel_gpio_request(struct device *dev,
			struct sprd_panel *panel)
{
	panel->info.avdd_gpio = devm_gpiod_get_optional(dev,
					"avdd", GPIOD_ASIS);
	if (IS_ERR_OR_NULL(panel->info.avdd_gpio))
		DRM_WARN("can't get panel avdd gpio: %ld\n",
				 PTR_ERR(panel->info.avdd_gpio));

	panel->info.avee_gpio = devm_gpiod_get_optional(dev,
					"avee", GPIOD_ASIS);
	if (IS_ERR_OR_NULL(panel->info.avee_gpio))
		DRM_WARN("can't get panel avee gpio: %ld\n",
				 PTR_ERR(panel->info.avee_gpio));

	panel->info.reset_gpio = devm_gpiod_get_optional(dev,
					"reset", GPIOD_ASIS);
	if (IS_ERR_OR_NULL(panel->info.reset_gpio))
		DRM_WARN("can't get panel reset gpio: %ld\n",
				 PTR_ERR(panel->info.reset_gpio));

	return 0;
}

static int of_parse_reset_seq(struct device_node *np,
				struct panel_info *info)
{
	struct property *prop;
	int bytes, rc;
	u32 *p;

	prop = of_find_property(np, "sprd,reset-on-sequence", &bytes);
	if (!prop) {
		DRM_ERROR("sprd,reset-on-sequence property not found\n");
		return -EINVAL;
	}

	p = kzalloc(bytes, GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	rc = of_property_read_u32_array(np, "sprd,reset-on-sequence",
					p, bytes / 4);
	if (rc) {
		DRM_ERROR("parse sprd,reset-on-sequence failed\n");
		kfree(p);
		return rc;
	}

	info->rst_on_seq.items = bytes / 8;
	info->rst_on_seq.timing = (struct gpio_timing *)p;

	prop = of_find_property(np, "sprd,reset-off-sequence", &bytes);
	if (!prop) {
		DRM_ERROR("sprd,reset-off-sequence property not found\n");
		return -EINVAL;
	}

	p = kzalloc(bytes, GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	rc = of_property_read_u32_array(np, "sprd,reset-off-sequence",
					p, bytes / 4);
	if (rc) {
		DRM_ERROR("parse sprd,reset-off-sequence failed\n");
		kfree(p);
		return rc;
	}

	info->rst_off_seq.items = bytes / 8;
	info->rst_off_seq.timing = (struct gpio_timing *)p;

	return 0;
}

static int of_parse_buildin_modes(struct panel_info *info,
	struct device_node *lcd_node)
{
	int i, rc, num_timings;
	struct device_node *timings_np;


	timings_np = of_get_child_by_name(lcd_node, "display-timings");
	if (!timings_np) {
		DRM_ERROR("%s: can not find display-timings node\n",
			lcd_node->name);
		return -ENODEV;
	}

	num_timings = of_get_child_count(timings_np);
	if (num_timings == 0) {
		/* should never happen, as entry was already found above */
		DRM_ERROR("%s: no timings specified\n", lcd_node->name);
		goto done;
	}

	info->buildin_modes = kzalloc(sizeof(struct drm_display_mode) *
				num_timings, GFP_KERNEL);

	for (i = 0; i < num_timings; i++) {
		rc = of_get_drm_display_mode(lcd_node,
			&info->buildin_modes[i], NULL, i);
		if (rc) {
			DRM_ERROR("get display timing failed\n");
			goto entryfail;
		}

		info->buildin_modes[i].width_mm = info->mode.width_mm;
		info->buildin_modes[i].height_mm = info->mode.height_mm;
		info->buildin_modes[i].vrefresh = info->mode.vrefresh;
	}
	info->num_buildin_modes = num_timings;
	DRM_INFO("info->num_buildin_modes = %d\n", num_timings);
	goto done;

entryfail:
	kfree(info->buildin_modes);
done:
	of_node_put(timings_np);

	return 0;
}

static int of_parse_oled_cmds(struct sprd_oled *oled,
		const void *data, int size)
{
	const struct dsi_cmd_desc *cmds = data;
	struct dsi_cmd_desc *p;
	u16 len;
	int i, total;

	if (cmds == NULL)
		return -EINVAL;

	/*
	 * TODO:
	 * Currently, we only support the same length cmds
	 * for oled brightness level. So we take the first
	 * cmd payload length as all.
	 */
	len = (cmds->wc_h << 8) | cmds->wc_l;
	total =  size / (len + 4);

	p = (struct dsi_cmd_desc *)kzalloc(size, GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	memcpy(p, cmds, size);
	for (i = 0; i < total; i++) {
		oled->cmds[i] = p;
		p = (struct dsi_cmd_desc *)(p->payload + len);
	}

	oled->cmds_total = total;
	oled->cmd_len = len + 4;

	return 0;
}

#ifdef CONFIG_ZTE_LCD_HBM_CTRL
int sprd_oled_set_brightness(struct backlight_device *bdev)
#else
static int sprd_oled_set_brightness(struct backlight_device *bdev)
#endif
{
	#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	u32 bl_preset = 0;
	static int last_backlevel = -1;
	#ifdef CONFIG_ZTE_LCD_DCSBL_CABC_GRADIENT
	static u8 lcd_cabc_53h = 0x2c;
	static int dimming_cnt = 2;
	#endif
	int level;
	#endif
	int brightness;
	struct sprd_oled *oled = bl_get_data(bdev);
	struct sprd_panel *panel = oled->panel;

	mutex_lock(&panel->lock);
	if (!panel->enabled) {
		mutex_unlock(&panel->lock);
		DRM_WARN("oled panel has been powered off\n");
		return -ENXIO;
	}

	#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	if (g_zte_ctrl_pdata == NULL || g_dsi == NULL) {
		mutex_unlock(&panel->lock);
		ZTE_LCD_INFO("%s: NULL point return!\n", __func__);
		return -ENXIO;
	}
	g_zte_ctrl_pdata->dsi = g_dsi;
	#ifdef CONFIG_ZTE_LCD_HBM_CTRL
	if (g_zte_ctrl_pdata->dsi->ctx.dpms == DRM_MODE_DPMS_SUSPEND
		|| g_zte_ctrl_pdata->dsi->ctx.dpms == DRM_MODE_DPMS_STANDBY) {
		mutex_unlock(&panel->lock);
		ZTE_LCD_INFO("%s: last_dpms=%s, panel is not on, skip\n",
					__func__, zte_drm_mode[g_zte_ctrl_pdata->dsi->ctx.last_dpms]);
		return -ENXIO;
	}
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->exit_doze == 1) {
		msleep(200);
		ZTE_LCD_INFO("%s: exit_doze backlight must delay 200mS to avoid screen flash\n", __func__);
		g_zte_ctrl_pdata->zte_lcd_ctrl->exit_doze = 0;
	}
	#endif
	#endif

	brightness = bdev->props.brightness;

#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	level = brightness * oled->max_level / 255;
	bl_preset = level;
#if defined(CONFIG_ZTE_LCD_BACKLIGHT_LEVEL_CURVE)
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 12) {
			level = g_zte_ctrl_pdata->zte_lcd_ctrl->zte_convert_brightness(level, 4095);
#ifdef CONFIG_ZTE_LCD_SET_ALS_MODE
			if ((ALS_MODE == ALS_LOW) && (level > ALS_HIGH_THD*ALS_BACKLIGHT_12bit)) {
				set_als_mode(ALS_HIGH);
				ALS_MODE = ALS_HIGH;
			} else if ((ALS_MODE == ALS_HIGH) && (level < ALS_LOW_THD*ALS_BACKLIGHT_12bit)) {
				set_als_mode(ALS_LOW);
				ALS_MODE = ALS_LOW;
			}
#endif
		} else if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 11) {
			level = g_zte_ctrl_pdata->zte_lcd_ctrl->zte_convert_brightness(level, 2047);
		} else if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 10) {
			level = g_zte_ctrl_pdata->zte_lcd_ctrl->zte_convert_brightness(level, 1023);
		} else {
			level = g_zte_ctrl_pdata->zte_lcd_ctrl->zte_convert_brightness(level, 255);
#ifdef CONFIG_ZTE_LCD_SET_ALS_MODE
			if ((ALS_MODE == ALS_LOW) && (level > ALS_HIGH_THD)) {
				set_als_mode(ALS_HIGH);
				ALS_MODE = ALS_HIGH;
			} else if ((ALS_MODE == ALS_HIGH) && (level < ALS_LOW_THD)) {
				set_als_mode(ALS_LOW);
				ALS_MODE = ALS_LOW;
			}
#endif
		}
#endif

	#ifdef CONFIG_ZTE_LCD_HBM_CTRL
	ZTE_LCD_INFO("%s: hdr_flg=%d brightness=%d level=%d --> convert_level = %d,last_dpms=%s dpms=%s\n",
					__func__, g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on,
					brightness, bl_preset, level,
					zte_drm_mode[g_zte_ctrl_pdata->dsi->ctx.last_dpms], zte_drm_mode[g_zte_ctrl_pdata->dsi->ctx.dpms]);
	#else
	DRM_INFO("%s level:%d --> convert_level:%d oled->cmds_total:%d bl_register_len:%d\n",
				__func__, bl_preset, level, oled->cmds_total,
				g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len);
	#endif

	#ifdef CONFIG_ZTE_LCD_DCSBL_CABC_GRADIENT
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->close_dynamic_dimming) {
		if (last_backlevel == 0) {
				ZTE_LCD_INFO("%s: close_dynamic_dimming\n", __func__);
		}
	} else {
		if (level != 0 && last_backlevel == 0) {/*set second backlight*/
			if (g_zte_ctrl_pdata->dsi->ctx.last_dpms == DRM_MODE_DPMS_ON) {
			msleep(100);
			ZTE_LCD_INFO("%s: set second backlight delay 100mS\n", __func__);
			}
		} else if (level == 0) {
			dimming_cnt = 0;
			lcd_cabc_53h = 0x24;
			sprd_panel_send_cmds(panel->slave,
				     panel->info.cmds[CMD_CODE_DIMMING_CLOSE],
				     panel->info.cmds_len[CMD_CODE_DIMMING_CLOSE]);
			ZTE_LCD_INFO("%s:lcd off from dtsi cmds_len=0x%x\n", __func__,
				panel->info.cmds_len[CMD_CODE_DIMMING_CLOSE]);
		}
	}
	#endif

	#ifdef CONFIG_ZTE_LCD_DELAY_OPEN_BL
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_delay_open_bl_value != 0) {
		if ((last_backlevel == 0) && (level != 0)) {
			ZTE_LCD_INFO("%s delay %dms\n", __func__,
				g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_delay_open_bl_value);
			msleep(g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_delay_open_bl_value);
		}
	}
	#endif

	#ifdef CONFIG_ZTE_LCD_HBM_CTRL
	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_backlight_level = level;
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on != 0) {
		level = level * HDR_MAX_BACKLIGHT_LEVEL / oled->max_level;
		if (level > HDR_MAX_BACKLIGHT_LEVEL)
			level = HDR_MAX_BACKLIGHT_LEVEL; /* 540nit, for HDR mode */
		ZTE_LCD_INFO("%s: lcd_hdr_on=%d old_level=%d hdr_level=%d\n",
					__func__, g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on,
					last_backlevel, level);
	}
	#endif

	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 12) {
		oled->cmds[0]->payload[1] = (uint8_t) (level >> 8) & 0x0f;
		oled->cmds[0]->payload[2] = (uint8_t) level & 0xff;
	} else if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 11) {
		oled->cmds[0]->payload[1] = (uint8_t) (level >> 8) & 0x0f;
		oled->cmds[0]->payload[2] = (uint8_t) level & 0xff;
	} else if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len == 10) {
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_register_len_is_ilitek == 1) {
			oled->cmds[0]->payload[1] = (uint8_t) (level >> 6) & 0x0f;
			oled->cmds[0]->payload[2] = (uint8_t) (level << 2) & 0xfc;
		} else {
			oled->cmds[0]->payload[1] = (uint8_t) (level >> 2) & 0xff;
			oled->cmds[0]->payload[2] = (uint8_t) (level << 2) & 0x0C;
		}
	} else {
		oled->cmds[0]->payload[1] = level;
	}

	#ifdef CONFIG_ZTE_LCD_SUPPORT_ER68577
	if ((g_zte_ctrl_pdata != NULL) && (g_zte_ctrl_pdata->zte_lcd_ctrl != NULL)
		&& g_zte_ctrl_pdata->zte_lcd_ctrl->is_support_er68577 && g_dsi != NULL) {
		sprd_dsi_lp_cmd_enable(g_dsi, true);
	}
	#endif
	sprd_panel_send_cmds(panel->slave, oled->cmds[0], oled->cmd_len);

	#ifdef CONFIG_ZTE_LCD_DCSBL_CABC_GRADIENT
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->close_dynamic_dimming) {
		if (last_backlevel == 0) {
				ZTE_LCD_INFO("%s: close_dynamic_dimming\n", __func__);
		}
	} else {
		if (level != 0) {
			if (lcd_cabc_53h == 0x2c) {/*set second backlight*/
				dimming_cnt++;
				if (dimming_cnt > 1) {/*set third backlight*/
					sprd_panel_send_cmds(panel->slave,
						     panel->info.cmds[CMD_CODE_DIMMING_OPEN],
						     panel->info.cmds_len[CMD_CODE_DIMMING_OPEN]);
					ZTE_LCD_INFO("%s:lcd on V0527_400 dimming_cnt=%d, cmds_len=0x%x\n", __func__,
						dimming_cnt, panel->info.cmds_len[CMD_CODE_DIMMING_OPEN]);
					lcd_cabc_53h = 0X24;
					dimming_cnt = 0;
				}
			}
			if (last_backlevel == 0)/*set first backlight*/
				lcd_cabc_53h = 0x2c;
		}
	}
	#endif

	last_backlevel = level;

	#else

	DRM_INFO("%s brightness: %d\n", __func__, brightness);

	sprd_panel_send_cmds(panel->slave,
			     panel->info.cmds[CMD_OLED_REG_LOCK],
			     panel->info.cmds_len[CMD_OLED_REG_LOCK]);

	if (oled->cmds_total == 1) {
		if (oled->cmds[0]->wc_l == 3) {
			oled->cmds[0]->payload[1] = brightness >> 8;
			oled->cmds[0]->payload[2] = brightness & 0xFF;
		} else
			oled->cmds[0]->payload[1] = brightness;

		sprd_panel_send_cmds(panel->slave,
			     oled->cmds[0],
			     oled->cmd_len);
	} else {
		if (brightness >= oled->cmds_total)
			brightness = oled->cmds_total - 1;

		sprd_panel_send_cmds(panel->slave,
			     oled->cmds[brightness],
			     oled->cmd_len);
	}

	sprd_panel_send_cmds(panel->slave,
			     panel->info.cmds[CMD_OLED_REG_UNLOCK],
			     panel->info.cmds_len[CMD_OLED_REG_UNLOCK]);
	#endif

	mutex_unlock(&panel->lock);

	return 0;
}

static const struct backlight_ops sprd_oled_backlight_ops = {
	.update_status = sprd_oled_set_brightness,
};

static int sprd_oled_backlight_init(struct sprd_panel *panel)
{
	struct sprd_oled *oled;
	struct device_node *oled_node;
	struct panel_info *info = &panel->info;
	const void *p;
	int bytes, rc;
	u32 temp;

	oled_node = of_get_child_by_name(info->of_node,
				"oled-backlight");
	if (!oled_node)
		return 0;

	oled = devm_kzalloc(&panel->dev,
			sizeof(struct sprd_oled), GFP_KERNEL);
	if (!oled)
		return -ENOMEM;

	oled->bdev = devm_backlight_device_register(&panel->dev,
			"sprd_backlight", &panel->dev, oled,
			&sprd_oled_backlight_ops, NULL);
	if (IS_ERR(oled->bdev)) {
		DRM_ERROR("failed to register oled backlight ops\n");
		return PTR_ERR(oled->bdev);
	}

	p = of_get_property(oled_node, "brightness-levels", &bytes);
	if (p) {
		info->cmds[CMD_OLED_BRIGHTNESS] = p;
		info->cmds_len[CMD_OLED_BRIGHTNESS] = bytes;
	} else
		DRM_ERROR("can't find brightness-levels property\n");

	p = of_get_property(oled_node, "sprd,reg-lock", &bytes);
	if (p) {
		info->cmds[CMD_OLED_REG_LOCK] = p;
		info->cmds_len[CMD_OLED_REG_LOCK] = bytes;
	} else
		DRM_INFO("can't find sprd,reg-lock property\n");

	p = of_get_property(oled_node, "sprd,reg-unlock", &bytes);
	if (p) {
		info->cmds[CMD_OLED_REG_UNLOCK] = p;
		info->cmds_len[CMD_OLED_REG_UNLOCK] = bytes;
	} else
		DRM_INFO("can't find sprd,reg-unlock property\n");

	rc = of_property_read_u32(oled_node, "default-brightness-level", &temp);
	if (!rc)
		oled->bdev->props.brightness = temp;
	else
		oled->bdev->props.brightness = 25;

	rc = of_property_read_u32(oled_node, "sprd,max-level", &temp);
	if (!rc)
		oled->max_level = temp;
	else
		oled->max_level = 255;

	oled->bdev->props.max_brightness = oled->max_level;
	oled->panel = panel;
	of_parse_oled_cmds(oled,
			panel->info.cmds[CMD_OLED_BRIGHTNESS],
			panel->info.cmds_len[CMD_OLED_BRIGHTNESS]);

	#ifdef CONFIG_ZTE_LCD_ESD_ERROR_CTRL
	g_bdev = oled->bdev;
	#endif

	#ifdef CONFIG_ZTE_LCD_HBM_CTRL
	g_oeld_bdev = oled->bdev;
	#endif

	DRM_INFO("%s() ok\n", __func__);

	return 0;
}

int sprd_panel_parse_lcddtb(struct device_node *lcd_node,
	struct sprd_panel *panel)
{
	struct panel_info *info = &panel->info;
	int bytes, rc;
	const void *p;
	const char *str;
	u32 val;

	if (!lcd_node) {
		DRM_ERROR("lcd node from dtb is null\n");
		return -ENODEV;
	}
	info->of_node = lcd_node;

	rc = of_property_read_u32(lcd_node, "sprd,dsi-work-mode", &val);
	if (!rc) {
		if (val == SPRD_DSI_MODE_CMD)
			info->mode_flags = 0;
		else if (val == SPRD_DSI_MODE_VIDEO_BURST)
			info->mode_flags = MIPI_DSI_MODE_VIDEO |
					   MIPI_DSI_MODE_VIDEO_BURST;
		else if (val == SPRD_DSI_MODE_VIDEO_SYNC_PULSE)
			info->mode_flags = MIPI_DSI_MODE_VIDEO |
					   MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
		else if (val == SPRD_DSI_MODE_VIDEO_SYNC_EVENT)
			info->mode_flags = MIPI_DSI_MODE_VIDEO;
	} else {
		DRM_ERROR("dsi work mode is not found! use video mode\n");
		info->mode_flags = MIPI_DSI_MODE_VIDEO |
				   MIPI_DSI_MODE_VIDEO_BURST;
	}

	if (of_property_read_bool(lcd_node, "sprd,dsi-non-continuous-clock"))
		info->mode_flags |= MIPI_DSI_CLOCK_NON_CONTINUOUS;

	rc = of_property_read_u32(lcd_node, "sprd,dsi-lane-number", &val);
	if (!rc)
		info->lanes = val;
	else
		info->lanes = 4;

	rc = of_property_read_string(lcd_node, "sprd,dsi-color-format", &str);
	if (rc)
		info->format = MIPI_DSI_FMT_RGB888;
	else if (!strcmp(str, "rgb888"))
		info->format = MIPI_DSI_FMT_RGB888;
	else if (!strcmp(str, "rgb666"))
		info->format = MIPI_DSI_FMT_RGB666;
	else if (!strcmp(str, "rgb666_packed"))
		info->format = MIPI_DSI_FMT_RGB666_PACKED;
	else if (!strcmp(str, "rgb565"))
		info->format = MIPI_DSI_FMT_RGB565;
	else if (!strcmp(str, "dsc"))
		info->format = SPRD_MIPI_DSI_FMT_DSC;
	else
		DRM_ERROR("dsi-color-format (%s) is not supported\n", str);

	rc = of_property_read_u32(lcd_node, "sprd,phy-bit-clock", &val);
	if (!rc)
		info->hs_rate = val;
	else
		info->hs_rate = 500000;

	rc = of_property_read_u32(lcd_node, "sprd,phy-escape-clock", &val);
	if (!rc)
		info->lp_rate = val > 20000 ? 20000 : val;
	else
		info->lp_rate = 20000;

	rc = of_property_read_u32(lcd_node, "sprd,width-mm", &val);
	if (!rc)
		info->mode.width_mm = val;
	else
		info->mode.width_mm = 68;

	rc = of_property_read_u32(lcd_node, "sprd,height-mm", &val);
	if (!rc)
		info->mode.height_mm = val;
	else
		info->mode.height_mm = 121;

	rc = of_property_read_u32(lcd_node, "sprd,esd-check-enable", &val);
	if (!rc)
		info->esd_check_en = val;

	rc = of_property_read_u32(lcd_node, "sprd,esd-check-mode", &val);
	if (!rc)
		info->esd_check_mode = val;
	else
		info->esd_check_mode = 1;

	rc = of_property_read_u32(lcd_node, "sprd,esd-check-period", &val);
	if (!rc)
		info->esd_check_period = val;
	else
		info->esd_check_period = 1000;

	rc = of_property_read_u32(lcd_node, "sprd,esd-check-register", &val);
	if (!rc)
		info->esd_check_reg = val;
	else
		info->esd_check_reg = 0x0A;

	rc = of_property_read_u32(lcd_node, "sprd,esd-check-value", &val);
	if (!rc)
		info->esd_check_val = val;
	else
		info->esd_check_val = 0x9C;

	if (of_property_read_bool(lcd_node, "sprd,use-dcs-write"))
		info->use_dcs = true;
	else
		info->use_dcs = false;

	rc = of_parse_reset_seq(lcd_node, info);
	if (rc)
		DRM_ERROR("parse lcd reset sequence failed\n");

	p = of_get_property(lcd_node, "sprd,initial-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_INIT] = p;
		info->cmds_len[CMD_CODE_INIT] = bytes;
	} else
		DRM_ERROR("can't find sprd,init-command property\n");

	p = of_get_property(lcd_node, "sprd,sleep-in-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_SLEEP_IN] = p;
		info->cmds_len[CMD_CODE_SLEEP_IN] = bytes;
	} else
		DRM_ERROR("can't find sprd,sleep-in-command property\n");

	p = of_get_property(lcd_node, "sprd,sleep-out-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_SLEEP_OUT] = p;
		info->cmds_len[CMD_CODE_SLEEP_OUT] = bytes;
	} else
		DRM_ERROR("can't find sprd,sleep-out-command property\n");

	#ifdef CONFIG_ZTE_LCD_DCSBL_CABC_GRADIENT
	p = of_get_property(lcd_node, "zte,dimming-open-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_DIMMING_OPEN] = p;
		info->cmds_len[CMD_CODE_DIMMING_OPEN] = bytes;
	} else
		DRM_ERROR("can't find zte,dimming-open-command property\n");

	p = of_get_property(lcd_node, "zte,dimming-close-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_DIMMING_CLOSE] = p;
		info->cmds_len[CMD_CODE_DIMMING_CLOSE] = bytes;
	} else
		DRM_ERROR("can't find zte,dimming-close-command property\n");
	#endif

	p = of_get_property(lcd_node, "sprd,doze-in-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_DOZE_IN] = p;
		info->cmds_len[CMD_CODE_DOZE_IN] = bytes;
	} else
		DRM_INFO("can't find sprd,doze-in-command property\n");

	p = of_get_property(lcd_node, "sprd,doze-out-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_DOZE_OUT] = p;
		info->cmds_len[CMD_CODE_DOZE_OUT] = bytes;
	} else
		DRM_INFO("can't find sprd,doze-out-command property\n");

	rc = of_get_drm_display_mode(lcd_node, &info->mode, 0,
				     OF_USE_NATIVE_MODE);
	if (rc) {
		DRM_ERROR("get display timing failed\n");
		return rc;
	}

	if (!info->mode_flags)
		info->mode.vrefresh = 60;
	else
		info->mode.vrefresh = drm_mode_vrefresh(&info->mode);
	#ifdef CONFIG_ZTE_LCD_SUPPORT_SD5302
	info->mode.vrefresh = 60;
	#endif
	of_parse_buildin_modes(info, lcd_node);

	return 0;
}

static int sprd_panel_parse_dt(struct device_node *np, struct sprd_panel *panel)
{
	struct device_node *lcd_node, *cmdline_node;
	const char *cmd_line, *lcd_name_p;
	char lcd_path[60];
	int rc;

	cmdline_node = of_find_node_by_path("/chosen");
	rc = of_property_read_string(cmdline_node, "bootargs", &cmd_line);
	if (!rc) {
		lcd_name_p = strstr(cmd_line, "lcd_name=");
		if (lcd_name_p) {
			sscanf(lcd_name_p, "lcd_name=%s", panel->lcd_name);
			DRM_INFO("lcd name: %s\n", panel->lcd_name);
		}
	} else {
		DRM_ERROR("can't not parse bootargs property\n");
		return rc;
	}

	sprintf(lcd_path, "/lcds/%s", panel->lcd_name);
	lcd_node = of_find_node_by_path(lcd_path);
	if (!lcd_node) {
		DRM_ERROR("%pOF: could not find %s node\n", np, panel->lcd_name);
		return -ENODEV;
	}
	rc = sprd_panel_parse_lcddtb(lcd_node, panel);
	if (rc)
		return rc;

	return 0;
}

static int sprd_panel_device_create(struct device *parent,
				    struct sprd_panel *panel)
{
	panel->dev.class = display_class;
	panel->dev.parent = parent;
	panel->dev.of_node = panel->info.of_node;
	dev_set_name(&panel->dev, "panel0");
	dev_set_drvdata(&panel->dev, panel);

	return device_register(&panel->dev);
}

static int sprd_panel_probe(struct mipi_dsi_device *slave)
{
	struct sprd_panel *panel;
	struct device_node *bl_node;
	int ret;

	panel = devm_kzalloc(&slave->dev, sizeof(*panel), GFP_KERNEL);
	if (!panel)
		return -ENOMEM;

	bl_node = of_parse_phandle(slave->dev.of_node,
					"sprd,backlight", 0);
	if (bl_node) {
		panel->backlight = of_find_backlight_by_node(bl_node);
		of_node_put(bl_node);

		if (panel->backlight) {
			panel->backlight->props.state &= ~BL_CORE_FBBLANK;
			panel->backlight->props.power = FB_BLANK_UNBLANK;
			backlight_update_status(panel->backlight);
		} else {
			DRM_WARN("backlight is not ready, panel probe deferred\n");
			return -EPROBE_DEFER;
		}
	} else
		DRM_WARN("backlight node not found\n");

	panel->supply = devm_regulator_get(&slave->dev, "power");
	if (IS_ERR(panel->supply)) {
		if (PTR_ERR(panel->supply) == -EPROBE_DEFER)
			DRM_ERROR("regulator driver not initialized, probe deffer\n");
		else
			DRM_ERROR("can't get regulator: %ld\n", PTR_ERR(panel->supply));

		return PTR_ERR(panel->supply);
	}

	INIT_DELAYED_WORK(&panel->esd_work, sprd_panel_esd_work_func);

	ret = sprd_panel_parse_dt(slave->dev.of_node, panel);
	if (ret) {
		DRM_ERROR("parse panel info failed\n");
		return ret;
	}

	ret = sprd_panel_gpio_request(&slave->dev, panel);
	if (ret) {
		DRM_WARN("gpio is not ready, panel probe deferred\n");
		return -EPROBE_DEFER;
	}

	ret = sprd_panel_device_create(&slave->dev, panel);
	if (ret)
		return ret;

	ret = sprd_oled_backlight_init(panel);
	if (ret)
		return ret;

	panel->base.dev = &panel->dev;
	panel->base.funcs = &sprd_panel_funcs;
	drm_panel_init(&panel->base);

	ret = drm_panel_add(&panel->base);
	if (ret) {
		DRM_ERROR("drm_panel_add() failed\n");
		return ret;
	}

	slave->lanes = panel->info.lanes;
	slave->format = panel->info.format;
	slave->hs_rate = panel->info.hs_rate;
	slave->lp_rate = panel->info.lp_rate;
	slave->mode_flags = panel->info.mode_flags;

	ret = mipi_dsi_attach(slave);
	if (ret) {
		DRM_ERROR("failed to attach dsi panel to host\n");
		drm_panel_remove(&panel->base);
		return ret;
	}
	panel->slave = slave;

	ret = sprd_panel_sysfs_init(&panel->dev);
	if (ret)
		return ret;

	mipi_dsi_set_drvdata(slave, panel);

	/*
	 * FIXME:
	 * The esd check work should not be scheduled in probe
	 * function. It should be scheduled in the enable()
	 * callback function. But the dsi encoder will not call
	 * drm_panel_enable() the first time in encoder_enable().
	 */
	if (panel->info.esd_check_en) {
		schedule_delayed_work(&panel->esd_work,
				      msecs_to_jiffies(2000) * 10);
		panel->esd_work_pending = true;
	}

	#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	zte_lcd_common_func(slave, panel);
	#endif
	panel->enabled = true;

	mutex_init(&panel->lock);

	return 0;
}

static int sprd_panel_remove(struct mipi_dsi_device *slave)
{
	struct sprd_panel *panel = mipi_dsi_get_drvdata(slave);
	int ret;

	DRM_INFO("%s()\n", __func__);

	sprd_panel_disable(&panel->base);
	sprd_panel_unprepare(&panel->base);

	ret = mipi_dsi_detach(slave);
	if (ret < 0)
		DRM_ERROR("failed to detach from DSI host: %d\n", ret);

	drm_panel_detach(&panel->base);
	drm_panel_remove(&panel->base);
#ifdef CONFIG_ZTE_LCD_COMMON_FUNCTION
	tpd_notify_unregister();
#endif
	return 0;
}

static const struct of_device_id panel_of_match[] = {
	{ .compatible = "sprd,generic-mipi-panel", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, panel_of_match);

struct mipi_dsi_driver sprd_panel_driver = {
	.driver = {
		.name = "sprd-mipi-panel-drv",
		.of_match_table = panel_of_match,
	},
	.probe = sprd_panel_probe,
	.remove = sprd_panel_remove,
};

MODULE_AUTHOR("Leon He <leon.he@unisoc.com>");
MODULE_AUTHOR("Kevin Tang <kevin.tang@unisoc.com>");
MODULE_DESCRIPTION("UNISOC MIPI DSI Panel Driver");
MODULE_LICENSE("GPL v2");
