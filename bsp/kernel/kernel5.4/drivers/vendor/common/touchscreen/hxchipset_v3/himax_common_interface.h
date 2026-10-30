
extern uint32_t  himax_tptest_result;
extern bool fw_updating;

void himax_tpd_register_fw_class(void);
#ifdef HX_PINCTRL_EN
int himax_platform_pinctrl_init(struct himax_i2c_platform_data *pdata);
#endif
int himax_get_fw_by_lcminfo(void);
int himax_save_failed_node(int failed_node);
void himax_tpd_test_result_check(uint32_t test_result);
#ifdef HX_ZERO_FLASH
extern struct firmware *adb_fw_entry;
extern const struct ts_firmware *adb_upgrade_firmware;
#endif

