#ifndef _WIRELESS_TX
#define _WIRELESS_TX

#include <huawei_platform/power/wireless_charger.h>

#define WL_TX_FAIL                                    (-1)
#define WL_TX_SUCC                                    (0)
#define WL_TX_MONITOR_INTERVAL                        (300)

#define WL_TX_SOC_MIN                                 (20)
#define WL_TX_BATT_TEMP_MAX                           (50)
#define WL_TX_TBATT_DELTA_TH                          (15)
#define WL_TX_BATT_TEMP_MIN                           (-10)
#define WL_TX_BATT_TEMP_RESTORE                       (35)
#define WL_TX_STR_LEN_32                              (32)

#define WL_TX_VIN_MIN                                 (4500) //(mV)
#define WL_TX_VIN_MAX                                 (5800) //(mV)
#define WL_TX_VIN_RETRY_CNT                           (25)
#define WL_TX_VIN_SLEEP_TIME                          (20)
#define WL_TX_IIN_SAMPLE_LEN                          (10)
#define WL_TX_PING_TIMEOUT_1                          (120)
#define WL_TX_PING_TIMEOUT_2                          (119)
#define WL_TX_PING_CHECK_INTERVAL                     (20)
#define WL_TX_PING_VIN_OVP_CNT                        (3)
#define WL_TX_PING_VIN_UVP_CNT                        (10) //6s-10s
#define WL_TX_I2C_ERR_CNT                             (5)
#define WL_TX_IIN_LOW                                 (300) //mA
#define WL_TX_IIN_LOW_CNT                             (30*60*1000) //30min
#define WL_TX_MODE_ERR_CNT                            (10)

#define WL_TX_MONITOR_LOG_INTERVAL                    (3000)
#define WI_TX_CHARGER_TYPE_MAX                        (12)

enum wireless_tx_status_type {
	/*** tx normal status ***/
	WL_TX_STATUS_DEFAULT = 0x00,           // default
	WL_TX_STATUS_PING_SUCC,                // ping success
	WL_TX_STATUS_IN_CHARGING,              // in reverse charging status
	WL_TX_STATUS_CHARGE_DONE,              // charge done, stop transmittering power
	/*** tx error status ***/
	WL_TX_STATUS_FAULT_BASE = 0x10,        // base
	WL_TX_STATUS_TX_CLOSE,                 // chip fault hanppened,  or ui close TX mode
	WL_TX_STATUS_RX_DISCONNECT,            // monitor RX disconnect
	WL_TX_STATUS_PING_TIMEOUT,             // can not ping RX
	WL_TX_STATUS_TBATT_LOW,                // battery temperature too low
	WL_TX_STATUS_TBATT_HIGH,               // battery temperature too high
	WL_TX_STATUS_IN_WL_CHARGING,           // in wireless charging mode, can not open TX mode
	WL_TX_STATUS_SOC_ERROR,                // battery capacity is too low, stop transmittering power
};
enum wireless_tx_event_type {
	WL_TX_EVENT_GET_CFG,
	WL_TX_EVENT_HANDSHAKE_SUCC,
	WL_TX_EVENT_CHARGEDONE,
	WL_TX_EVENT_CEP_TIMEOUT,
	WL_TX_EVENT_EPT_CMD,
	WL_TX_EVENT_OVP,
	WL_TX_EVENT_OCP,
};
enum wireless_tx_stage {
	WL_TX_STAGE_DEFAULT = 0,
	WL_TX_STAGE_POWER_SUPPLY,
	WL_TX_STAGE_CHIP_INIT,
	WL_TX_STAGE_PING_RX,
	WL_TX_STAGE_REGULATION,
	WL_TX_STAGE_TOTAL,
};
enum wireless_tx_sysfs_type {
	WL_TX_SYSFS_TX_OPEN = 0,
	WL_TX_SYSFS_TX_STATUS,
	WL_TX_SYSFS_TX_IIN_AVG,
	WL_TX_SYSFS_DPING_FREQ,
	WL_TX_SYSFS_DPING_INTERVAL,
	WL_TX_SYSFS_MAX_FOP,
	WL_TX_SYSFS_MIN_FOP,
	WL_TX_SYSFS_TX_FOP,
};
struct wireless_tx_device_ops {
	int (*chip_reset)(void);
	void (*rx_enable)(int);
	void (*rx_sleep_enable)(int);
	void (*check_fwupdate)(enum wireless_mode);
	int (*kick_watchdog)(void);
	int (*tx_chip_init)(void);
	int (*tx_stop_config)(void);
	int (*enable_tx_mode)(bool);
	int (*get_tx_iin)(u16 *);
	int (*get_tx_vin)(u16 *);
	int (*get_tx_vrect)(u16 *);
	int (*get_chip_temp)(u8 *);
	int (*get_tx_fop)(u16*);
	int (*set_tx_max_fop)(u16);
	int (*get_tx_max_fop)(u16*);
	int (*set_tx_min_fop)(u16);
	int (*get_tx_min_fop)(u16*);
	int (*set_tx_ping_frequency)(u16);
	int (*get_tx_ping_frequency)(u16*);
	int (*set_tx_ping_interval)(u16);
	int (*get_tx_ping_interval)(u16*);
	bool (*check_rx_disconnect)(void);
	bool (*in_tx_mode)(void);
};

struct wireless_tx_device_info {
	struct device *dev;
	struct notifier_block tx_event_nb;
	struct work_struct wireless_tx_check_work;
	struct work_struct wireless_tx_evt_work;
	struct delayed_work wireless_tx_monitor_work;
	struct wireless_tx_device_ops *tx_ops;
	unsigned int monitor_interval;
	unsigned int ping_timeout;
	unsigned int tx_event_type;
	unsigned int tx_event_data;
	unsigned int tx_iin_avg;
	unsigned int i2c_err_cnt;
	unsigned int tx_mode_err_cnt;
	unsigned int tx_iin_low_cnt;
	bool standard_rx;
	bool stop_reverse_charge; //record  driver state
};

extern struct blocking_notifier_head tx_event_nh;
extern int wireless_tx_ops_register(struct wireless_tx_device_ops *tx_ops);
extern void wireless_tx_cancel_work(void);
extern void wireless_tx_start_check(void);
extern int wireless_tx_get_tx_iin_limit(enum usb_charger_type);
extern bool wireless_tx_get_tx_open_flag(void);

#endif
