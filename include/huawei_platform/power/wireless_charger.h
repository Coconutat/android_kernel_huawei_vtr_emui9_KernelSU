#ifndef _WIRELESS_CHARGER
#define _WIRELESS_CHARGER

#define I2C_RETRY_CNT               3
#define I2C_OPS_SLEEP_TIME          5
#define BYTE_MASK                   0xff
#define WORD_MASK                   0xffff
#define BYTE_LEN                    1
#define WORD_LEN                    2

#define WIRELESS_CHANNEL_ON         1
#define WIRELESS_CHANNEL_OFF        0
#define WIRED_CHANNEL_ON            1
#define WIRED_CHANNEL_OFF           0

#define RX_EN_ENABLE                1
#define RX_EN_DISABLE               0
#define RX_SLEEP_EN_ENABLE          1
#define RX_SLEEP_EN_DISABLE         0

#define CERTI_SUCC                  0
#define CERTI_FAIL                  1

#define WIRELESS_CHRG_SUCC          0
#define WIRELESS_CHRG_FAIL          1

#define MSEC_PER_SEC                1000
#define MVOLT_PER_VOLT              1000
#define PERCENT                     100
#define RX_IOUT_MIN                 150
#define RX_IOUT_MID                 500
#define RX_VOUT_ERR_RATIO           81
#define TX_BOOST_VOUT               12000
#define TX_DEFAULT_VOUT             5000
#define RX_DEFAULT_VOUT             5500
#define RX_HIGH_VOUT                7000
#define RX_DEFAULT_IOUT             1000
#define CHANNEL_SW_TIME             50
#define CHANNEL_SW_TIME_2           200


#define RX_HIGH_IOUT                850
#define RX_LOW_IOUT                 300
#define RX_AVG_IOUT_TIME            30*1000
#define RX_IOUT_REG_STEP            100
#define RX_VRECT_LOW_RESTORE_TIME   10000
#define RX_VRECT_LOW_IOUT_MIN       300
#define RX_VOUT_ERR_CHECK_TIME      1000

#define TX_ID_HW                    0x8866

#define CONTROL_INTERVAL_NORMAL     300
#define CONTROL_INTERVAL_FAST       100
#define MONITOR_INTERVAL            100
#define MONITOR_LOG_INTERVAL        5000
#define DISCONN_WORK_DELAY_MS       1300

#define RX_IOUT_SAMPLE_LEN          10
#define WIRELESS_STAGE_STR_LEN      32
#define WIRELESS_TMP_STR_LEN        16

#define WIRELESS_DEFAULT_POWER      5000000 //mV*mA

#define TX_ID_ERR_CNT_MAX           3
#define TX_ABILITY_ERR_CNT_MAX      3
#define CERTI_ERR_CNT_MAX           3
#define BOOST_ERR_CNT_MAX           5


#define RX_OCP_CNT_MAX              3
#define RX_OVP_CNT_MAX              3
#define RX_OTP_CNT_MAX              3

#define SET_CURRENT_LIMIT_STEP      100
#define RX_SAMPLE_WORK_DELAY        500
#define SERIALNO_LEN                16
#define WIRELESS_RANDOM_LEN         8
#define WIRELESS_TX_KEY_LEN         8
#define WIRELESS_RX_KEY_LEN         8

/*rx charge state*/
#define WIRELESS_STATE_CHRG_FULL            BIT(0)
#define WIRELESS_STATE_CHRG_DONE            BIT(1)

#define WIRELESS_INTERFER_PARA_LEVEL        8
#define WIRELESS_SEGMENT_PARA_LEVEL         5
#define WIRELESS_IOUT_CTRL_PARA_LEVEL       15
#define WIRELESS_CHIP_INIT                  0
#define WILREESS_SC_CHIP_INIT               1

#define WIRELESS_INT_CNT_TH                 10
#define WIRELESS_INT_TIMEOUT_TH             15*1000 //ms

#define WIRELESS_MODE_TYPE_MAX              10
#define WIRELESS_TX_TYPE_MAX                20
#define WIRELESS_VOLT_MODE_TYPE_MAX         5

#define WIRELESS_NORMAL_CHARGE_FLAG         0
#define WIRELESS_FAST_CHARGE_FLAG           1
#define WIRELESS_SUPER_CHARGE_FLAG          2

#define WIRELESS_MODE_QUICK_JUDGE_CRIT      0  //for quick icon-display
#define WIRELESS_MODE_NORMAL_JUDGE_CRIT     1  //for recorecting icon-display
#define WIRELESS_MODE_FINAL_JUDGE_CRIT      2  //for judging power mode

#define WIRELESS_CHECK_UNKNOWN              -1
#define WIRELESS_CHECK_FAIL                 0
#define WIRELESS_CHECK_SUCC                 1

#define WAIT_AF_SRV_RTY_CNT                 3
#define WC_AF_INFO_NL_OPS_NUM		    1
#define WC_AF_WAIT_CT_TIMEOUT		    1000
#define WC_AF_TOTAL_KEY_NUM		    11

enum tx_power_state {
	TX_POWER_GOOD_UNKNOWN = 0,
	TX_POWER_GOOD,
	TX_NOT_POWER_GOOD,  //weak source tx
};
enum wireless_mode {
	WIRELESS_RX_MODE = 0,
	WIRELESS_TX_MODE,
};
enum tx_adaptor_type {
	WIRELESS_UNKOWN   = 0x00,
	WIRELESS_SDP      = 0x01,
	WIRELESS_CDP      = 0x02,
	WIRELESS_NON_STD  = 0x03,
	WIRELESS_DCP      = 0x04,
	WIRELESS_FCP      = 0x05,
	WIRELESS_SCP      = 0x06,
	WIRELESS_PD       = 0x07,
	WIRELESS_QC       = 0x08,
	WIRELESS_OTG_A 	  = 0x09,  //tx powered by battery
	WIRELESS_OTG_B 	  = 0x0A,  //tx powered by adaptor
	WIRELESS_TYPE_ERR = 0xff,
};
enum wireless_etp_type {
	WIRELESS_EPT_UNKOWN         = 0x00,
	WIRELESS_EPT_CHRG_COMPLETE  = 0x01,
	WIRELESS_EPT_INTERNAL_FAULT = 0x02,
	WIRELESS_EPT_OTP            = 0x03,
	WIRELESS_EPT_OVP            = 0x04,
	WIRELESS_EPT_OCP            = 0x05,
	WIRELESS_EPT_BATT_FAILURE   = 0x06,
	WIRELESS_EPT_RESERVED       = 0x07,
	WIRELESS_EPT_NO_RESPONSE    = 0x08,
	WIRELESS_EPT_ERR_VRECT      = 0xA0,
	WIRELESS_EPT_ERR_VOUT       = 0xA1,
};
enum wireless_charge_stage {
	WIRELESS_STAGE_DEFAULT = 0,
	WIRELESS_STAGE_CHECK_TX_ID,
	WIRELESS_STAGE_CHECK_TX_ABILITY,
	WIRELESS_STAGE_CABLE_DETECT,
	WIRELESS_STAGE_CERTIFICATION,
	WIRELESS_STAGE_CHECK_FWUPDATE,
	WIRELESS_STAGE_CHARGING,
	WIRELESS_STAGE_REGULATION,
	WIRELESS_STAGE_REGULATION_DC,
	WIRELESS_STAGE_TOTAL,
};
struct tx_capability {
	u8 type;
	int vout_max;
	int iout_max;
	bool can_boost;
	bool cable_ok;
	bool no_need_cert;
	bool support_scp;
	bool support_12v;
	bool support_extra_cap;
	bool support_fan;
	bool support_tec;
	bool support_Qval;
};

enum tx_cap_info {
	TX_CAP_TYPE = 1,
	TX_CAP_VOUT_MAX,
	TX_CAP_IOUT_MAX,
	TX_CAP_ATTR,
	TX_CAP_TOTAL,
};
enum tx_extra_cap_info {
	TX_EXTRA_CAP_ATTR1 = 1,
	TX_EXTRA_CAP_ATTR2,
	TX_EXTRA_CAP_ATTR3,
	TX_EXTRA_CAP_ATTR4,
	TX_EXTRA_CAP_TOTAL,
};
enum wireless_interfer_info {
	WIRELESS_INTERFER_SRC_OPEN = 0,
	WIRELESS_INTERFER_SRC_CLOSE,
	WIRELESS_INTERFER_TX_FIXED_FOP,
	WIRELESS_INTERFER_TX_VOUT_LIMIT,
	WIRELESS_INTERFER_RX_VOUT_LIMIT,
	WIRELESS_INTERFER_RX_IOUT_LIMIT,
	WIRELESS_INTERFER_TOTAL,
};
struct wireless_interfer_para {
	u8 src_open;
	u8 src_close;
	int tx_fixed_fop;
	int tx_vout_limit;
	int rx_vout_limit;
	int rx_iout_limit;
};
struct wireless_interfer_data {
	int total_src;
	u8 interfer_src_state;
	struct wireless_interfer_para
		interfer_para[WIRELESS_INTERFER_PARA_LEVEL];
};

enum wireless_segment_info {
	WIRELESS_SEGMENT_PARA_SOC_MIN = 0,
	WIRELESS_SEGMENT_PARA_SOC_MAX,
	WIRELESS_SEGMENT_PARA_TX_VOUT_LIMIT,
	WIRELESS_SEGMENT_PARA_RX_VOUT_LIMIT,
	WIRELESS_SEGMENT_PARA_RX_IOUT_LIMIT,
	WIRELESS_SEGMENT_PARA_TOTAL,
};
struct wireless_segment_para {
	int soc_min;
	int soc_max;
	int tx_vout_limit;
	int rx_vout_limit;
	int rx_iout_limit;
};
struct wireless_segment_data {
	int segment_para_level;
	struct wireless_segment_para segment_para[WIRELESS_SEGMENT_PARA_LEVEL];
};
enum wireless_iout_ctrl_info {
	WIRELESS_ICTRL_IOUT_MIN = 0,
	WIRELESS_ICTRL_IOUT_MAX,
	WIRELESS_ICTRL_IOUT_SET,
	WIRELESS_ICTRL_TOTAL,
};
struct wireless_iout_ctrl_para {
	int iout_min;
	int iout_max;
	int iout_set;
};
struct wireless_iout_ctrl_data {
	int ictrl_para_level;
	struct wireless_iout_ctrl_para* ictrl_para;
};
enum wireless_ctrl_para_info {
	WIRELESS_CHARGE_TX_VOUT = 0,
	WIRELESS_CHARGE_RX_VOUT,
	WIRELESS_CHARGE_RX_IOUT,
	WIRELESS_CHARGE_PARA_TOTAL,
};
struct wireless_ctrl_para {
	int tx_vout;
	int rx_vout;
	int rx_iout;
};
enum wireless_mode_para_info {
	WIRELESS_MODE_NAME = 0,
	WIRELESS_MODE_TX_VOUT_MIN,
	WIRELESS_MODE_TX_IOUT_MIN,
	WIRELESS_MODE_TX_VOUT,
	WIRELESS_MODE_RX_VOUT,
	WIRELESS_MODE_RX_IOUT,
	WIRELESS_MODE_VRECT_LOW_TH,
	WIRELESS_MODE_TBATT,
	WIRELESS_MODE_EXPECT_CABLE_DETECT,
	WIRELESS_MODE_EXPECT_CERT,
	WIRELESS_MODE_ICON_TYPE,
	WIRELESS_MODE_MAX_TIME,
	WIRELESS_MODE_EXPECT_MODE,
	WIRELESS_MODE_INFO_TOTAL,
};
struct wireless_mode_para {
	char *mode_name;
	int tx_vout_min;
	int tx_iout_min;
	struct wireless_ctrl_para ctrl_para;
	int vrect_low_th;
	int tbatt;
	int max_time;
	s8 expect_cable_detect;
	s8 expect_cert;
	u8 icon_type;
	s8 expect_mode;
};
struct wireless_mode_data {
	int total_mode;
	struct wireless_mode_para *mode_para;
};
enum wireless_tx_prop_info {
	WIRELESS_TX_ADAPTOR_TYPE = 0,
	WIRELESS_TX_TYPE_NAME,
	WIRELESS_TX_NEED_CABLE_DETECT,
	WIRELESS_TX_NEED_CERT,
	WIRELESS_TX_DEFAULT_VOUT,
	WIRELESS_TX_DEFAULT_IOUT,
	WIRELESS_TX_PROP_TOTAL,
};
struct wireless_tx_prop_para {
	u8 tx_type;
	char *type_name;
	u8 need_cable_detect;
	u8 need_cert;
	int tx_default_vout;
	int tx_default_iout;
};
struct wireless_tx_prop_data {
	int total_prop_type;
	struct wireless_tx_prop_para *tx_prop;
};
enum wireless_volt_mode_info{
	WIRELESS_VOLT_MODE_TYPE = 0,
	WIRELESS_VOLT_MODE_TX_VOUT,
	WIRELESS_VOLT_MODE_TOTAL,
};
struct wireless_volt_mode_para {
	u8 mode_type;
	int tx_vout;
};
struct wireless_volt_mode_data {
	int total_volt_mode;
	struct wireless_volt_mode_para *volt_mode;
};
enum af_srv_state {
	AF_SRV_NOT_READY = 0,
	AF_SRV_NO_RESPONSE,
	AF_SRV_SUCC,
};
struct wireless_charge_device_ops {
	int (*chip_init)(int);
	int (*chip_reset)(void);
	void (*rx_enable)(int);
	void (*rx_sleep_enable)(int);
	bool (*check_tx_exist)(void);
	int (*send_chrg_state)(u8);
	int (*send_rx_qval)(u8);
	int (*kick_watchdog)(void);
	int (*get_rx_vrect)(void);
	int (*get_rx_vout)(void);
	int (*get_rx_vout_reg)(void);
	int (*get_tx_vout_reg)(void);
	int (*get_rx_iout)(void);
	int (*get_rx_fop)(void);
	int (*set_tx_vout)(int);
	int (*set_rx_vout)(int);
	int (*set_rx_fod_coef)(char *);
	int (*fix_tx_fop)(int);
	int (*unfix_tx_fop)(void);
	int (*get_tx_id)(void);
	u8* (*get_rx_chip_id)(void);
	u8* (*get_rx_fw_version)(void);
	char* (*get_rx_fod_coef)(void);
	enum tx_adaptor_type (*get_tx_adaptor_type)(void);
	int (*check_fwupdate)(enum wireless_mode);
	struct tx_capability * (*get_tx_capability)(void);
	u8* (*get_tx_fw_version)(void);
	int (*check_ac_power)(void);
	int (*send_ept)(enum wireless_etp_type);
	int (*stop_charging)(void);
	int (*get_tx_cert)(u8*, unsigned int, u8*, unsigned int);
	int (*send_msg_rx_vout)(int);
	int (*send_msg_rx_iout)(int);
	int (*send_msg_serialno)(char*, unsigned int);
	int (*send_msg_batt_temp)(int);
	int (*send_msg_batt_capacity)(int);
	int (*send_msg_cert_confirm)(bool);
	int (*send_msg_rx_boost_succ)(void);
	void (*pmic_vbus_handler)(bool);
#ifdef WIRELESS_CHARGER_FACTORY_VERSION
	int (*check_is_otp_exist)(void);
	int (*rx_program_otp)(void);
	int (*rx_check_otp)(void);
#endif

};
struct wireless_charge_sysfs_data {
	int en_enable;
	int permit_wldc;
	int tx_fixed_fop;
	int tx_vout_max;
	int rx_vout_max;
	int rx_iout_max;
};
struct wireless_charge_device_info {
	struct device *dev;
	struct notifier_block rx_event_nb;
	struct notifier_block chrg_event_nb;
	struct notifier_block pwrkey_nb;
	struct blocking_notifier_head wireless_charge_evt_nh;
	struct work_struct wired_vbus_connect_work;
	struct work_struct wired_vbus_disconnect_work;
	struct work_struct rx_program_otp_work;
	struct work_struct wireless_rx_event_work;
	struct delayed_work wireless_vbus_disconnect_work;
	struct delayed_work wireless_ctrl_work;
	struct delayed_work wireless_monitor_work;
	struct delayed_work rx_sample_work;
	struct tx_capability *tx_cap;
	struct wireless_charge_device_ops *ops;
	struct wireless_mode_data mode_data;
	struct wireless_ctrl_para product_para;
	struct wireless_tx_prop_data tx_prop_data;
	struct wireless_volt_mode_data volt_mode_data;
	struct wireless_charge_sysfs_data sysfs_data;
	struct wireless_interfer_data interfer_data;
	struct wireless_segment_data segment_data;
	struct wireless_iout_ctrl_data iout_ctrl_data;
	enum tx_adaptor_type standard_tx_adaptor;
	u8 cur_charge_state;
	u8 last_charge_state;
	u8 rx_qval;
	int standard_tx;
	int tx_vout_max;
	int rx_iout_min;
	int rx_iout_max;
	int rx_vout_max;
	int rx_iout_step;
	int rx_vout_err_ratio;
	enum wireless_charge_stage stage;
	int ctrl_interval;
	int monitor_interval;
	int tx_id_err_cnt;
	int tx_ability_err_cnt;
	int certi_err_cnt;
	int boost_err_cnt;
	int rx_event_type;
	int rx_event_data;
	int rx_iout_limit;
	int iout_avg;
	int iout_high_cnt;
	int iout_low_cnt;
	int cable_detect_succ_flag;
	int cert_succ_flag;
	int curr_tx_type_index;
	int curr_pmode_index;
	int curr_vmode_index;
	int curr_icon_type;
	unsigned long curr_power_time_out;
	enum tx_power_state tx_pg_state;
	struct completion wc_af_completion;
	u8 antifake_key_index;
};
enum wireless_charge_sysfs_type {
	WIRELESS_CHARGE_SYSFS_CHIP_ID = 0,
	WIRELESS_CHARGE_SYSFS_FW_VERSION,
#ifdef WIRELESS_CHARGER_FACTORY_VERSION
	WIRELESS_CHARGE_SYSFS_PROGRAM_OTP,
	WIRELESS_CHARGE_SYSFS_CHECK_OTP,
#endif
	WIRELESS_CHARGE_SYSFS_TX_ADAPTOR_TYPE,
	WIRELESS_CHARGE_SYSFS_VOUT,
	WIRELESS_CHARGE_SYSFS_IOUT,
	WIRELESS_CHARGE_SYSFS_VRECT,
	WIRELESS_CHARGE_SYSFS_EN_ENABLE,
	WIRELESS_CHARGE_SYSFS_WIRELESS_SUCC,
	WIRELESS_CHARGE_SYSFS_NORMAL_CHRG_SUCC,
	WIRELESS_CHARGE_SYSFS_FAST_CHRG_SUCC,
	WIRELESS_CHARGE_SYSFS_FOD_COEF,
	WIRELESS_CHARGE_SYSFS_INTERFERENCE_SETTING,
	WIRELESS_CHARGE_SYSFS_PERMIT_WLDC,
};
enum rx_event_type{
	WIRELESS_CHARGE_RX_POWER_ON = 0,
	WIRELESS_CHARGE_RX_READY,
	WIRELESS_CHARGE_GET_SERIAL_NO,
	WIRELESS_CHARGE_GET_BATTERY_TEMP,
	WIRELESS_CHARGE_GET_BATTERY_CAPACITY,
	WIRELESS_CHARGE_SET_CURRENT_LIMIT,
	WIRELESS_CHARGE_START_SAMPLE,
	WIRELESS_CHARGE_STOP_SAMPLE,
	WIRELESS_CHARGE_RX_OCP,
	WIRELESS_CHARGE_RX_OVP,
	WIRELESS_CHARGE_RX_OTP,
};

extern struct blocking_notifier_head rx_event_nh;
extern int wireless_charge_ops_register(struct wireless_charge_device_ops *ops);
extern int register_wireless_charger_vbus_notifier(struct notifier_block *nb);
extern void wireless_charge_wired_vbus_connect_handler(void);
extern void wireless_charge_wired_vbus_disconnect_handler(void);
extern int wireless_charge_get_wireless_channel_state(void);
extern void wireless_charge_set_wireless_channel_state(int state);
extern int wireless_charge_get_wired_channel_state(void);
extern void direct_charger_disconnect_event(void);
extern int wireless_charge_get_fast_charge_flag(void);
extern int wireless_charge_get_rx_iout_limit(void);
extern bool wireless_charge_check_tx_exist(void);
extern void wireless_charger_pmic_vbus_handler(bool);

extern void wireless_charge_chip_init(int);
extern int wireless_charge_set_tx_vout(int);
extern int wireless_charge_set_rx_vout(int);
extern int wireless_charge_get_rx_vout(void);
extern int wireless_charge_get_rx_vrect(void);
extern int wireless_charge_get_rx_iout(void);
extern int wireless_charge_get_rx_fop(void);

extern int wireless_charge_get_rx_avg_iout(void);
extern void wireless_charge_restart_charging(enum wireless_charge_stage);
extern bool wireless_charge_mode_judge_criterion(int pmode_index, int crit_type);
extern int wireless_charge_get_power_mode(void);
extern void wireless_charge_update_max_vout_and_iout(bool ignore_cnt_flag);


#endif
