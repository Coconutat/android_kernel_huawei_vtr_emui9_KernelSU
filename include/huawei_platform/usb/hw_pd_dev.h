

#ifndef __HW_PD_DEV_H__
#define __HW_PD_DEV_H__

#include <linux/device.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/completion.h>
#ifdef CONFIG_CONTEXTHUB_PD
#include <linux/hisi/contexthub/tca.h>
#endif

#define CONFIG_DPM_USB_PD_CUSTOM_DBGACC
#define CONFIG_DPM_TYPEC_CAP_DBGACC_SNK
#define CONFIG_DPM_TYPEC_CAP_CUSTOM_SRC
#ifdef CONFIG_CONTEXTHUB_PD
#define COMBPHY_MAX_PD_EVENT_COUNT 40
#define COMBPHY_PD_EVENT_INVALID_VAL -1
#endif
#define PD_DPM_HW_DOCK_SVID 0x12d1

#define PD_DPM_CC_CHANGE_COUNTER_THRESHOLD     (50)
#define PD_DPM_CC_CHANGE_INTERVAL              (2000)  /*ms*/
#define PD_DPM_CC_CHANGE_MSEC                  (1000)
#define PD_DPM_CC_CHANGE_BUF_SIZE              (10)

#define PD_DPM_CC_DMD_COUNTER_THRESHOLD        (50)
#define PD_DPM_CC_DMD_INTERVAL                 (24*60*60) /*s*/
#define PD_DPM_CC_DMD_BUF_SIZE                 (1024)

/* type-c inserted plug orientation */
enum pd_cc_orient{
    PD_DPM_CC_MODE_UFP = 0,
    PD_DPM_CC_MODE_DRP,
};
enum pd_cc_mode {
    PD_CC_ORIENT_DEFAULT = 0,
    PD_CC_ORIENT_CC1,
    PD_CC_ORIENT_CC2,
    PD_CC_NOT_READY,
};

enum pd_wait_typec_complete {
    NOT_COMPLETE,
    COMPLETE_FROM_VBUS_DISCONNECT,
    COMPLETE_FROM_TYPEC_CHANGE,
};
enum pd_connect_result {
    PD_CONNECT_NONE = 0,
    PD_CONNECT_TYPEC_ONLY,
    PD_CONNECT_TYPEC_ONLY_SNK_DFT,
    PD_CONNECT_TYPEC_ONLY_SNK,
    PD_CONNECT_TYPEC_ONLY_SRC,
    PD_CONNECT_PE_READY,
    PD_CONNECT_PE_READY_SNK,
    PD_CONNECT_PE_READY_SRC,

#ifdef CONFIG_DPM_USB_PD_CUSTOM_DBGACC
	PD_CONNECT_PE_READY_DBGACC_UFP,
	PD_CONNECT_PE_READY_DBGACC_DFP,
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */
};

enum pd_device_port_power_mode{
    PD_DEV_PORT_POWERMODE_SOURCE = 0,
    PD_DEV_PORT_POWERMODE_SINK,
    PD_DEV_PORT_POWERMODE_NOT_READY,
};

enum pd_device_port_data_mode{
    PD_DEV_PORT_DATAMODE_HOST = 0,
    PD_DEV_PORT_DATAMODE_DEVICE,
    PD_DEV_PORT_DATAMODE_NOT_READY,
};

enum pd_device_port_mode{
    PD_DEV_PORT_MODE_DFP = 0,
    PD_DEV_PORT_MODE_UFP,
    PD_DEV_PORT_MODE_NOT_READY,
};

enum {
    PD_DPM_PE_EVT_DIS_VBUS_CTRL,
    PD_DPM_PE_EVT_SOURCE_VCONN,
    PD_DPM_PE_EVT_SOURCE_VBUS,
    PD_DPM_PE_EVT_SINK_VBUS,
    PD_DPM_PE_EVT_PR_SWAP,
    PD_DPM_PE_EVT_DR_SWAP,
    PD_DPM_PE_EVT_VCONN_SWAP,
    PD_DPM_PE_EVT_TYPEC_STATE,
    PD_DPM_PE_EVT_PD_STATE,
    PD_DPM_PE_EVT_BC12,
    PD_DPM_PE_ABNORMAL_CC_CHANGE_HANDLER,
};

enum pd_typec_attach_type {
    PD_DPM_TYPEC_UNATTACHED = 0,
    PD_DPM_TYPEC_ATTACHED_SNK,
    PD_DPM_TYPEC_ATTACHED_SRC,
    PD_DPM_TYPEC_ATTACHED_AUDIO,
    PD_DPM_TYPEC_ATTACHED_DEBUG,

#ifdef CONFIG_DPM_TYPEC_CAP_DBGACC_SNK
	PD_DPM_TYPEC_ATTACHED_DBGACC_SNK,		/* Rp, Rp */
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_DPM_TYPEC_CAP_CUSTOM_SRC
	PD_DPM_TYPEC_ATTACHED_CUSTOM_SRC,		/* Same Rp */
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */
    PD_DPM_TYPEC_ATTACHED_VBUS_ONLY,
    PD_DPM_TYPEC_UNATTACHED_VBUS_ONLY,

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
	PD_DPM_TYPEC_ATTACHED_CUSTOM_SRC2,
#endif
};

enum pd_dpm_cable_event_type {
    USB31_CABLE_IN_EVENT = 0,
    DP_CABLE_IN_EVENT,
    ANA_AUDIO_IN_EVENT,
    USB31_CABLE_OUT_EVENT,
    DP_CABLE_OUT_EVENT,
    ANA_AUDIO_OUT_EVENT,
};

enum pd_dpm_charger_event_type {
    PD_EVENT_CHARGER_TYPE_USB = 0,	/*SDP*/
    PD_EVENT_CHARGER_TYPE_BC_USB,	/*CDP*/
    PD_EVENT_CHARGER_TYPE_NON_STANDARD,	/*UNKNOW*/
    PD_EVENT_CHARGER_TYPE_STANDARD,	/*DCP*/
    PD_EVENT_CHARGER_TYPE_FCP,	/*FCP*/
};

enum {
        PD_DPM_USB_TYPEC_NONE = 0,
        PD_DPM_USB_TYPEC_DETACHED,
        PD_DPM_USB_TYPEC_DEVICE_ATTACHED,
        PD_DPM_USB_TYPEC_HOST_ATTACHED,
        PD_DPM_USB_TYPEC_AUDIO_ATTACHED,
        PD_DPM_USB_TYPEC_AUDIO_DETACHED,
};

enum pd_dpm_uevent_type {
    PD_DPM_UEVENT_START = 0,
    PD_DPM_UEVENT_COMPLETE,
};

enum pd_dpm_wake_lock_type {
    PD_WAKE_LOCK = 100,
    PD_WAKE_UNLOCK,
};

struct pd_dpm_typec_state {
    bool polarity;
    int cc1_status;
    int cc2_status;
    int old_state;
    int new_state;
};

struct pd_dpm_ops {
	void (*pd_dpm_hard_reset)(void*);
	bool (*pd_dpm_get_hw_dock_svid_exist)(void*);
	int (*pd_dpm_notify_direct_charge_status)(void*, bool mode);
	void (*pd_dpm_set_cc_mode)(int mode);
	void (*pd_dpm_set_voltage)(void*, int vol);
	int (*pd_dpm_get_cc_state)(void);
};
struct pd_dpm_pd_state {
	uint8_t connected;
};

struct pd_dpm_swap_state {
	uint8_t new_role;
};

enum pd_dpm_vbus_type {
    PD_DPM_VBUS_TYPE_TYPEC = 20,
    PD_DPM_VBUS_TYPE_PD,
};

enum pd_dpm_cc_voltage_type {
	PD_DPM_CC_VOLT_OPEN = 0,
	PD_DPM_CC_VOLT_RA = 1,
	PD_DPM_CC_VOLT_RD = 2,

	PD_DPM_CC_VOLT_SNK_DFT = 5,
	PD_DPM_CC_VOLT_SNK_1_5 = 6,
	PD_DPM_CC_VOLT_SNK_3_0 = 7,

	PD_DPM_CC_DRP_TOGGLING = 15,
};

struct pd_dpm_vbus_state {
    int mv;
    int ma;
    uint8_t vbus_type;
    bool ext_power;
    int remote_rp_level;
};

enum abnomal_change_type {
	PD_DPM_ABNORMAL_CC_CHANGE = 0,
	PD_DPM_UNATTACHED_VBUS_ONLY,
};

struct abnomal_change_info {
	enum abnomal_change_type event_id;
	bool first_enter;
	int change_counter;
	int dmd_counter;
	struct timespec64 ts64_last;
	struct timespec64 ts64_dmd_last;
	int change_data[PD_DPM_CC_CHANGE_BUF_SIZE];
	int dmd_data[PD_DPM_CC_CHANGE_BUF_SIZE];
};

#ifdef CONFIG_CONTEXTHUB_PD
struct pd_dpm_combphy_event {
	TCA_IRQ_TYPE_E irq_type;
	TCPC_MUX_CTRL_TYPE mode_type;
	TCA_DEV_TYPE_E dev_type;
	TYPEC_PLUG_ORIEN_E typec_orien;
};
#endif
struct pd_dpm_info{
    struct i2c_client *client;
    struct device *dev;
    struct mutex pd_lock;
    struct mutex sink_vbus_lock;

    struct dual_role_phy_instance *dual_role;
    struct dual_role_phy_desc *desc;

    enum hisi_charger_type charger_type;
    struct notifier_block usb_nb;
    struct notifier_block chrg_wake_unlock_nb;
    struct blocking_notifier_head pd_evt_nh;
    struct blocking_notifier_head pd_port_status_nh;
    struct atomic_notifier_head pd_wake_unlock_evt_nh;

    enum pd_dpm_uevent_type uevent_type;
    struct work_struct pd_work;

    const char *tcpc_name;
    int uart_use_sbu;
    int uart_rx_gpio;
    int uart_tx_gpio;

	/* usb state update */
    struct mutex usb_lock;
    int pending_usb_event;
    int last_usb_event;
    struct workqueue_struct *usb_wq;
    struct delayed_work usb_state_update_work;

#ifdef CONFIG_CONTEXTHUB_PD
    struct pd_dpm_combphy_event last_combphy_notify_event;
    struct mutex pd_combphy_notify_lock;
    struct workqueue_struct *pd_combphy_wq;
    struct delayed_work pd_combphy_event_work;
#endif

    int vconn_en;
    bool bc12_finish_flag;
    bool pd_finish_flag;
    bool pd_source_vbus;
    unsigned long bc12_event;
    struct pd_dpm_vbus_state bc12_sink_vbus_state;
    int cur_usb_event;
};

struct cc_check_ops {
	int (*is_cable_for_direct_charge)(void);
};
int cc_check_ops_register(struct cc_check_ops*);

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
struct cable_vdo_ops {
	int (*is_cust_src2_cable)(void);
};
int pd_dpm_cable_vdo_ops_register(struct cable_vdo_ops *);
#endif

/* for chip layer to get class created by core layer */
struct class *hw_pd_get_class(void);

extern struct tcpc_device *tcpc_dev_get_by_name(const char *name);

extern int register_pd_dpm_notifier(struct notifier_block *nb);
extern int unregister_pd_dpm_notifier(struct notifier_block *nb);
extern int register_pd_wake_unlock_notifier(struct notifier_block *nb);
extern int unregister_pd_wake_unlock_notifier(struct notifier_block *nb);
extern int register_pd_dpm_portstatus_notifier(struct notifier_block *nb);
extern int unregister_pd_dpm_portstatus_notifier(struct notifier_block *nb);
extern int pd_dpm_handle_pe_event(unsigned long event, void *data);
extern bool pd_dpm_get_pd_finish_flag(void);
extern bool pd_dpm_get_pd_source_vbus(void);
extern void pd_dpm_get_typec_state(int *typec_detach);
int pd_dpm_get_analog_hs_state(void);
extern void pd_dpm_get_charge_event(unsigned long *event, struct pd_dpm_vbus_state *state);
extern bool pd_dpm_get_high_power_charging_status(void);
extern bool pd_dpm_get_high_voltage_charging_status(void);
extern bool pd_dpm_get_optional_max_power_status(void);
extern bool pd_dpm_get_cc_orientation(void);
extern int pd_dpm_get_cc_state_type(void);
#ifdef CONFIG_CONTEXTHUB_PD
extern int pd_dpm_handle_combphy_event(struct pd_dpm_combphy_event event);
#endif
void pd_dpm_set_cc_voltage(int type);
enum pd_dpm_cc_voltage_type pd_dpm_get_cc_voltage(void);
int pd_dpm_ops_register(struct pd_dpm_ops *ops, void*client);
void pd_dpm_hard_reset(void);
bool pd_dpm_get_hw_dock_svid_exist(void);
int pd_dpm_get_pd_reset_adapter(void);
void pd_dpm_set_pd_reset_adapter(int ra);

#ifdef CONFIG_TCPC_CLASS
extern void pd_dpm_wakelock_ctrl(unsigned long event);/*PD_WAKE_LOCK,PD_WAKE_UNLOCK*/
extern void pd_dpm_vbus_ctrl(unsigned long event);/*CHARGER_TYPE_NONE,PLEASE_PROVIDE_POWER*/
#else
static inline void pd_dpm_wakelock_ctrl(unsigned long event) {};
static inline void pd_dpm_vbus_ctrl(unsigned long event) {};
#endif

extern void pd_dpm_ignore_vbus_only_event(bool flag);
extern bool pd_dpm_ignore_vbus_event(void);
extern void pd_dpm_set_ignore_vbus_event(bool);
extern bool pd_dpm_ignore_bc12_event_when_vbusoff(void);
extern bool pd_dpm_ignore_bc12_event_when_vbuson(void);
extern void pd_dpm_set_ignore_vbus_event_when_vbusoff(bool _ignore_vbus_event);
extern void pd_dpm_set_ignore_bc12_event_when_vbuson(bool _ignore_bc12_event);
extern bool pd_dpm_ignore_vbuson_event(void);
extern bool pd_dpm_ignore_vbusoff_event(void);
extern void pd_dpm_set_ignore_vbuson_event(bool _ignore_vbus_on_event);
extern void pd_dpm_set_ignore_vbusoff_event(bool _ignore_vbus_off_event);


#ifdef CONFIG_CONTEXTHUB_PD
extern bool pd_dpm_get_last_hpd_status(void);
extern void pd_dpm_set_last_hpd_status(bool hpd_status);
extern void pd_dpm_set_combphy_status(TCPC_MUX_CTRL_TYPE mode);
extern TCPC_MUX_CTRL_TYPE pd_dpm_get_combphy_status(void);
#endif

extern int pd_dpm_get_cur_usb_event(void);
extern void pd_dpm_audioacc_sink_vbus(unsigned long event, void *data);
#endif
