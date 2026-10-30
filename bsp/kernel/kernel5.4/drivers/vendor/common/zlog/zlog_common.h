#ifndef _ZLOG_COMMON_H
#define _ZLOG_COMMON_H

#define MAX_CLIENT_CNT			32

/* max client name length */
#define ZLOG_MAX_MSG_LEN			2048

/* max client name length */
#define ZLOG_MAX_LOG_LEN			256

/* max client name length */
#define ZLOG_LOG_CUSHION_LEN			256


/* max client name length */
#define ZLOG_CLIENT_NAME_LEN			32
/* max device name length */
#define ZLOG_MAX_DEVICE_NAME_LEN		32
/* max module name length */
#define ZLOG_MAX_MODULE_NAME_LEN	16
/* max ic name length */
#define ZLOG_MAX_IC_NAME_LEN			16

#define ZLOG_TIME_STAMP_LEN				"[2022-06-30 18:28:38]"
#define ZLOG_MODULE_NO_LEN				16
#define ZLOG_ERROR_NO_LEN				16
#define ZLOG_CONTENT_NO_LEN				16


#define BUFF_STATUS_NOT_INIT	0
#define BUFF_STATUS_READY		1
#define BUFF_STATUS_APPEND		2
#define BUFF_STATUS_NOTIFY		3
#define BUFF_STATUS_ERROR		4

#define ZLOG_MODULE_CHG			(1)
#define ZLOG_MODULE_TP			(2)
#define ZLOG_MODULE_LCD			(3)
#define ZLOG_MODULE_SENDOR		(4)

#define ZLOG_CHG_VBAT_OVP			1
#define ZLOG_CHG_IBAT_OCP			2
#define ZLOG_CHG_VBUS_OVP			3
#define ZLOG_CHG_IBUS_OCP			4
#define ZLOG_CHG_CONV_OCP			5
#define ZLOG_CHG_CONV_START			6
#define ZLOG_CHG_CONV_STOP			7
#define ZLOG_CHG_VBUS_TO_HIGH		8
#define ZLOG_CHG_VBUS_TO_LOW		9
#define ZLOG_CHG_WT_TIMEOUT			10
#define ZLOG_CHG_I2C_R_ERROR_NO		11
#define ZLOG_CHG_I2C_W_ERROR_NO		12


#define ZLOG_COMB_MODULE_ID(x)		(x << 24)


struct zlog_client_ops {
	int (*poll_state)(void);
	int (*dump_func)(int type, void *buff, int size);
};

struct zlog_mod_info {
	int module_no;
	const char *name;
	const char *device_name;
	const char *ic_name;
	const char *module_name;
	struct zlog_client_ops *fops;
};

struct zlog_client {
	int client_id;
	char client_name[ZLOG_CLIENT_NAME_LEN];
	char device_name[ZLOG_MAX_DEVICE_NAME_LEN];
	char ic_name[ZLOG_MAX_IC_NAME_LEN];
	char module_name[ZLOG_MAX_MODULE_NAME_LEN];
	struct zlog_client_ops *cops;
	struct mutex client_lock;
	int module_no;
	int error_no;
	unsigned long buff_flag;
	size_t read_size;
	size_t used_size;
	size_t buff_size;
	char *dump_buff;
	char initialized;
};

struct zlog_device {
	struct zlog_client client_list[MAX_CLIENT_CNT];
	struct workqueue_struct *handle_workqueue;
	struct delayed_work handle_work;
};

struct zlog_event {
	char *event_buff;
	size_t event_buff_len;
	size_t event_used_size;
};

#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION

int zlog_client_record(struct zlog_client *client, const char *fmt, ...);
void zlog_client_notify(struct zlog_client *client, int error_no);
void zlog_reset_client(struct zlog_client *client);
struct zlog_client *zlog_register_client(struct zlog_mod_info *mod_info);
void zlog_unregister_client(struct zlog_client *client);

#else/*CONFIG_VENDOR_ZTE_LOG_EXCEPTION*/

static inline int zlog_client_record(struct zlog_client *client, const char *fmt, ...)
{
	return -ENOSYS;
}

static inline void zlog_client_notify(struct zlog_client *client, int error_no)
{
	return;
}

static inline void zlog_reset_client(struct zlog_client *client)
{
	return;
}

static inline struct zlog_client *zlog_register_client(struct zlog_mod_info *mod_info)
{
	return NULL;
}

static inline void zlog_unregister_client(struct zlog_client *client)
{
	return;
}

#endif/*CONFIG_VENDOR_ZTE_LOG_EXCEPTION*/

#endif