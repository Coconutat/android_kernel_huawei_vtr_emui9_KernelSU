

#ifndef LINUX_SPI_FINGERPRINT_H
#define LINUX_SPI_FINGERPRINT_H

#include <huawei_platform/log/hw_log.h>

#define EVENT_CTS_HOME	172
#define EVENT_HOLD	28
#define EVENT_CLICK		174
#define EVENT_DCLICK	111
#define EVENT_UP	103
#define EVENT_DOWN	108
#define EVENT_LEFT	105
#define EVENT_RIGHT	106
#define EVENT_FINGER_UP     113
#define EVENT_FINGER_DOWN   117
#define EVENT_FINGER_IDENTIFY   118
#define EVENT_IDENTIFY_END   119
#define EVENT_FINGER_ENROLL   120

#define KEY_MIN  0
#define KEY_MAX  255

#define FP_MAX_MODULE_INFO_LEN      5
#define FP_MAX_SENSOR_ID_LEN        16
#define FP_MAX_CHIP_INFO_LEN        50
#define FP_DEFAULT_INFO_LEN         3
#define FP_RETURN_SUCCESS           0

//NAVIGATION_ADJUST_NOREVERSE: 默认值，适配后置指纹模组
#define NAVIGATION_ADJUST_NOREVERSE 0
//NAVIGATION_ADJUST_NOREVERSE: 适配前置指纹模组
#define NAVIGATION_ADJUST_REVERSE 1
#define NAVIGATION_ADJUST_NOTURN 0
#define NAVIGATION_ADJUST_TURN90 90
#define NAVIGATION_ADJUST_TURN180 180
#define NAVIGATION_ADJUST_TURN270 270

#define FPC_TTW_HOLD_TIME 1000
#define FP_DEV_NAME      "fingerprint"
#define FP_CLASS_NAME   "fpsensor"
#define FP_IOC_MAGIC     'f'  //define magic number


//define commands
#define  FP_IOC_CMD_ENABLE_IRQ	      _IO(FP_IOC_MAGIC, 1)
#define  FP_IOC_CMD_DISABLE_IRQ	      _IO(FP_IOC_MAGIC, 2)
#define  FP_IOC_CMD_SEND_UEVENT	      _IO(FP_IOC_MAGIC, 3)
#define  FP_IOC_CMD_GET_IRQ_STATUS     _IO(FP_IOC_MAGIC, 4)
#define  FP_IOC_CMD_SET_WAKELOCK_STATUS  _IO(FP_IOC_MAGIC, 5)
#define  FP_IOC_CMD_SEND_SENSORID      _IO(FP_IOC_MAGIC, 6)
#define  FP_IOC_CMD_SET_IPC_WAKELOCKS      _IO(FP_IOC_MAGIC, 7)
#define  FP_IOC_CMD_CHECK_HBM_STATUS      _IO(FP_IOC_MAGIC, 8)
#define  FP_IOC_CMD_RESET_HBM_STATUS      _IO(FP_IOC_MAGIC, 9)
#define  FP_IOC_CMD_SET_POWEROFF     _IO(FP_IOC_MAGIC,10)
#define  FP_IOC_CMD_GET_BIGDATA     _IO(FP_IOC_MAGIC,11)
//define sensor_id_ud length
#define MAX_SENSOR_ID_UD_LENGTH (20)

enum module_vendor_info
{
    MODULEID_LOW = 0,
    MODULEID_HIGT,
    MODULEID_FLOATING,
};

enum hbm_status
{
    HBM_ON = 0,
    HBM_NONE,
};
enum fp_irq_scheme
{
    FP_IRQ_SCHEME_ONE = 1,
};
enum fp_poweroff_scheme
{
    FP_POWEROFF_SCHEME_ONE = 1,
    FP_POWEROFF_SCHEME_TWO = 2,
};
enum fp_custom_timing_scheme
{
    fp_CUSTOM_TIMING_SCHEME_ONE = 1,
    fp_CUSTOM_TIMING_SCHEME_TWO = 2,
    fp_CUSTOM_TIMING_SCHEME_THREE = 3,
    fp_CUSTOM_TIMING_SCHEME_FOUR = 4,
};

// Defined in vendor/huawei/chipset_common/devkit/tpkit/huawei_ts_kit.h
typedef enum {
    TS_PEN_OUT_RANGE = 0,   /* pen out of range */
    TS_PEN_IN_RANGE,        /* pen in range */

    /* add event before here */
    TS_EVENT_MAX,           /* max event type */
} ts_notify_event_type;

typedef struct {
    int lcd_charge_time;
    int lcd_on_time;
    int cpu_wakeup_time;
} fingerprint_bigdata_t;

struct fp_data
{
    struct device* dev;
    struct cdev     cdev;
    struct class*    class;
    struct device*   device;
    dev_t             devno;
    struct platform_device* pf_dev;
    unsigned long finger_num;
    unsigned int nav_stat;
    struct wake_lock ttw_wl;
    int irq_gpio;
    int cs0_gpio;//UG
    int cs1_gpio;//UD
    int rst_gpio;//UG
    int rst1_gpio;//UD
    int power_en_gpio;
    int moduleID_gpio;
    char extern_ldo_name[32];
    char product_name[20];
    int extern_ldo_num;
    int extern_vol;
    int module_vendor_info;
    int navigation_adjust1;
    int navigation_adjust2;
    struct input_dev* input_dev;
    int irq_num;
    int qup_id;
    char idev_name[32];
    int event_type;
    int event_code;
    struct mutex lock;
    bool prepared;
    bool wakeup_enabled;
    bool read_image_flag;
    unsigned int sensor_id;
    char sensor_id_ud[MAX_SENSOR_ID_UD_LENGTH];
    struct pinctrl* pctrl;
    struct pinctrl_state* pins_default;
    struct pinctrl_state* pins_idle;
    char module_id[64];
    char module_id_ud[64];
    bool irq_enabled;
    bool irq_sensorhub_enabled;
    unsigned int pen_anti_enable;
    int hbm_status;
    wait_queue_head_t hbm_queue;
    unsigned int irq_custom_scheme;
    fingerprint_bigdata_t fingerprint_bigdata;
    int cts_home;
};

#ifdef CONFIG_LLT_TEST
struct LLT_fingprint_ops {
    ssize_t (*irq_get)(struct device* device, struct device_attribute* attribute, char* buffer);
    irqreturn_t (*fingerprint_irq_handler) (int irq, void* handle);
    int (*fingerprint_request_named_gpio)(struct fp_data* fingerprint, const char* label, int* gpio);
    int (*fingerprint_open)(struct inode* inode, struct file* file);
    int (*fingerprint_get_irq_status)(struct fp_data* fingerprint );
    long (*fingerprint_ioctl)(struct file* file, unsigned int cmd, unsigned long arg);
    int (*fingerprint_reset_gpio_init)(struct fp_data* fingerprint);
    int (*finerprint_get_module_info)(struct fp_data* fingerprint);
    int (*fingerprint_probe)(struct platform_device* pdev);
    int (*fingerprint_remove)(struct platform_device* pdev);
};
extern struct LLT_fingprint_ops LLT_fingerprint;
#endif

#if defined (CONFIG_HUAWEI_DSM)
typedef struct fp_sensor_info {
    unsigned int sensor_id;
    char sensor_name[FP_MAX_SENSOR_ID_LEN];
} fp_sensor_info;
#endif

#endif

