#ifndef	HISI_HISMART_AR_H
#define	HISI_HISMART_AR_H

#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include "hisi_softtimer.h"

#define FLP_TAG_AR          2
#define FLP_IOCTL_TYPE_AR               (0x464C0010)

#define FLP_IOCTL_CMD_MASK              (0xFFFF00FF)

#define FLP_IOCTL_TAG_MASK              (0xFFFFFF00)
#define FLP_IOCTL_TAG_FLP               (0x464C0000)
#define FLP_IOCTL_TAG_GPS               (0x464C0100)
#define FLP_IOCTL_TAG_AR                (0x464C0200)
#define FLP_IOCTL_TAG_COMMON            (0x464CFF00)

#define FLP_AR_DATA	(0x1<<1)
#define FLP_ENVIRONMENT (0x1<<4)
#define FLP_IOCTL_TYPE_ENV		(0x464C0040)
#define FLP_IOCTL_TYPE_COMMON           (0x464C00F0)
#define CONTEXT_PRIVATE_DATA_MAX (64)
#define DATABASE_DATALEN (64*1024)
/*common ioctrl*/
#define FLP_IOCTL_COMMON_GET_UTC            (0x464C0000 + 0xFFF1)
#define FLP_IOCTL_COMMON_SLEEP              (0x464C0000 + 0xFFF2)
#define FLP_IOCTL_COMMON_AWAKE_RET          (0x464C0000 + 0xFFF3)
#define FLP_IOCTL_COMMON_SETPID             (0x464C0000 + 0xFFF4)
#define FLP_IOCTL_COMMON_CLOSE_SERVICE      (0x464C0000 + 0xFFF5)
#define FLP_IOCTL_COMMON_HOLD_WAKELOCK      (0x464C0000 + 0xFFF6)
#define FLP_IOCTL_COMMON_RELEASE_WAKELOCK   (0x464C0000 + 0xFFF7)

#define FLP_IOCTL_AR_START(x)           (0x464C0000 + ((x) * 0x100) + 0x11)
#define FLP_IOCTL_AR_STOP(x)            (0x464C0000 + ((x) * 0x100) + 0x12)
#define FLP_IOCTL_AR_READ(x)            (0x464C0000 + ((x) * 0x100) + 0x13)
#define FLP_IOCTL_AR_UPDATE(x)          (0x464C0000 + ((x) * 0x100) + 0x15)
#define FLP_IOCTL_AR_FLUSH(x)           (0x464C0000 + ((x) * 0x100) + 0x16)
#define FLP_IOCTL_AR_STATE(x)           (0x464C0000 + ((x) * 0x100) + 0x17)
#define FLP_IOCTL_AR_CONFIG(x)           (0x464C0000 + ((x) * 0x100) + 0x18)
#define FLP_IOCTL_AR_STATE_V2(x)           (0x464C0000 + ((x) * 0x100) + 0x1D)

#define FLP_IOCTL_ENV_CONFIG(x) (0x464C0000 + ((x) * 0x100) + 0x41)
#define FLP_IOCTL_ENV_STOP(x) (0x464C0000 + ((x) * 0x100) + 0x42)
#define FLP_IOCTL_ENV_READ(x) (0x464C0000 + ((x) * 0x100) + 0x43)
#define FLP_IOCTL_ENV_FLUSH(x) (0x464C0000 + ((x) * 0x100) + 0x46)
#define FLP_IOCTL_ENV_STATE(x) (0x464C0000 + ((x) * 0x100) + 0x47)
#define FLP_IOCTL_ENV_INIT(x) (0x464C0000 + ((x) * 0x100) + 0x4A)
#define FLP_IOCTL_ENV_EXIT(x) (0x464C0000 + ((x) * 0x100) + 0x4B)
#define FLP_IOCTL_ENV_ENABLE(x) (0x464C0000 + ((x) * 0x100) + 0x4C)
#define FLP_IOCTL_ENV_DISABLE(x) (0x464C0000 + ((x) * 0x100) + 0x4D)
#define FLP_IOCTL_ENV_DATABASE(x) (0x464C0000 + ((x) * 0x100) + 0x4E)
#define FLP_IOCTL_ENV_DATA(x) (0x464C0000 + ((x) * 0x100) + 0x4F)

#define FLP_IOCTL_TYPE_MASK             (0xFFFF00F0)
#define FLP_IOCTL_TYPE_ENV		(0x464C0040)
#define FLP_IOCTL_TYPE_COMMON           (0x464C00F0)

#define MAX_CONFIG_SIZE           (32 * 1024)

enum {
    AR_ACTIVITY_VEHICLE         = 0x00,
    AR_ACTIVITY_RIDING          = 0x01,
    AR_ACTIVITY_WALK_SLOW       = 0x02,
    AR_ACTIVITY_RUN_FAST        = 0x03,
    AR_ACTIVITY_STATIONARY      = 0x04,
    AR_ACTIVITY_TILT            = 0x05,
    AR_ACTIVITY_END             = 0x10,
    AR_VE_BUS                   = 0x11, /* 大巴 */
    AR_VE_CAR                   = 0x12, /* 小车 */
    AR_VE_METRO                 = 0x13, /* 地铁 */
    AR_VE_HIGH_SPEED_RAIL       = 0x14, /* 高铁 */
    AR_VE_AUTO                  = 0x15, /* 公路交通 */
    AR_VE_RAIL                  = 0x16, /* 铁路交通 */
    AR_CLIMBING_MOUNT           = 0x17, /* 爬山*/
    AR_FAST_WALK                = 0x18, /* 快走*/
    AR_STOP_VEHICLE             = 0x19, /* STOP VEHICLE */
    AR_VEHICLE_UNKNOWN          = 0x1B, /* on vehicle, but not known which type */
    AR_ON_FOOT                  = 0x1C, /* ON FOOT */
    AR_OUTDOOR                  = 0x1F, /* outdoor:1, indoor:0 */
    AR_ELEVATOR                 = 0x20, /* elevator */
    AR_RELATIVE_STILL           = 0x21, /* relative still */
    AR_UNKNOWN                  = 0x3F,
    AR_STATE_BUTT               = 0x40,
};

typedef enum environment_type {
	AR_ENVIRONMENT_BEGIN,
	AR_ENVIRONMENT_HOME = AR_ENVIRONMENT_BEGIN,
	AR_ENVIRONMENT_OFFICE,
	AR_ENVIRONMENT_STATION,
	AR_ENVIRONMENT_TYPE_UNKNOWN,
	AR_ENVIRONMENT_END =0x20
} environment_type_t;

#define CONTEXT_TYPE_MAX (0x40)/*MAX(AR_STATE_BUTT, AR_ENVIRONMENT_END)*/

enum {
	AR_STATE_ENTER = 1,
	AR_STATE_EXIT = 2,
	AR_STATE_ENTER_EXIT = 3,
	AR_STATE_MAX
};

enum {
	ENV_CLOSE_FORCE = 0,
	ENV_CLOSE_NORMAL,
};

typedef struct ar_packet_header{
    unsigned char tag;
    unsigned char  cmd;
    unsigned char  resp : 1;
    unsigned char  rsv : 3;
    unsigned char  core : 4;   /*AP set to zero*/
    unsigned char  partial_order;
    unsigned short tranid;
    unsigned short length;
}ar_packet_header_t;   /* compatable to pkt_header_t */

typedef struct ar_activity_event {
    unsigned int        event_type;         /*0:disable*/
    unsigned int        activity;
    unsigned long       msec ;
}  __packed ar_activity_event_t ;

typedef struct ar_activity_config {
    unsigned int        event_type;         /*0:disable*/
    unsigned int        activity;
}  __packed ar_activity_config_t ;

typedef struct ar_start_config {
    unsigned int            report_interval;
    ar_activity_config_t    activity_list[AR_STATE_BUTT];
} ar_start_config_t;

typedef struct ar_start_hal_config {
    unsigned int            report_interval;
    unsigned int            event_num;
    ar_activity_config_t    *pevent;
} ar_start_hal_config_t ;

typedef struct ar_start_mcu_config {
    unsigned int            event_num;
    unsigned int            report_interval;
    ar_activity_config_t    activity_list[AR_STATE_BUTT];
} ar_start_mcu_config_t;

typedef struct ar_context_cfg_header {
	unsigned int	event_type;
	unsigned int	context;
	unsigned int	len;
}__packed ar_context_cfg_header_t;
/*KERNEL&HAL*/

typedef struct context_config {
	ar_context_cfg_header_t head;
	char buf[CONTEXT_PRIVATE_DATA_MAX];
} __packed context_config_t;

typedef struct context_hal_config {
	unsigned int	report_interval;
	unsigned int	context_num;
	context_config_t *context_addr;
}__packed context_hal_config_t;

/*KERNEL-->HUB*/
typedef struct context_iomcu_cfg {
	unsigned int report_interval;
	unsigned int context_num;
	context_config_t context_list[CONTEXT_TYPE_MAX];
} context_iomcu_cfg_t;
#define STATE_KERNEL_BUF_MAX	(1024)
typedef struct context_dev_info {
	unsigned int	usr_cnt;
	unsigned int	cfg_sz;
	context_iomcu_cfg_t   cfg;
	struct completion state_cplt;
	unsigned int state_buf_len;
	char state_buf[STATE_KERNEL_BUF_MAX];
}context_dev_info_t;

/*HUB-->KERNEL*/
#define GET_STATE_NUM_MAX (64)
typedef struct context_event {
	unsigned int event_type;
	unsigned int context;
	unsigned long long msec;/*long long :keep some with iomcu*/
	int confident;
	unsigned int buf_len;
	char buf[0];
} __packed context_event_t;

typedef struct {
	pkt_header_t pkt;
	unsigned int context_num;
	context_event_t context_event[0];
} __packed ar_data_req_t;

typedef struct {
	unsigned int context_num;
	context_event_t context_event[0];
} __packed ar_state_t;

typedef struct {
	unsigned int user_len;
	context_event_t context_event[CONTEXT_TYPE_MAX];
} __packed ar_state_shmen_t;

/********************************************
            define flp netlink
********************************************/
#define AR_GENL_NAME                   "TASKAR"
#define TASKAR_GENL_VERSION            0x01

enum {
    AR_GENL_ATTR_UNSPEC,
    AR_GENL_ATTR_EVENT,
    __AR_GENL_ATTR_MAX,
};
#define AR_GENL_ATTR_MAX (__AR_GENL_ATTR_MAX - 1)

enum {
    AR_GENL_CMD_UNSPEC,
    AR_GENL_CMD_PDR_DATA,
    AR_GENL_CMD_AR_DATA,
    AR_GENL_CMD_PDR_UNRELIABLE,
    AR_GENL_CMD_NOTIFY_TIMEROUT,
    AR_GENL_CMD_AWAKE_RET,
    AR_GENL_CMD_GEOFENCE_TRANSITION,
    AR_GENL_CMD_GEOFENCE_MONITOR,
    AR_GENL_CMD_GNSS_LOCATION,
    AR_GENL_CMD_IOMCU_RESET,
    AR_GENL_CMD_ENV_DATA,
    AR_GENL_CMD_ENV_DATABASE,
    __AR_GENL_CMD_MAX,
};
#define AR_GENL_CMD_MAX (__AR_GENL_CMD_MAX - 1)

#define HISI_AR_DEBUG KERN_INFO
#define HISI_AR_DEBUG_DUMP KERN_DEBUG
#define HISI_AR_DEBUG_DUMP_OPT   if(0)print_hex_dump
//#define HISI_AR_DATA_DEBUG
/*lint +e64*/
/*lint +e785*/
typedef struct ar_data_buf{
	char  *data_buf;
	unsigned int buf_size;
	unsigned int read_index;
	unsigned int write_index;
	unsigned int data_count;
	unsigned int  data_len;
} ar_data_buf_t;

typedef struct ar_hdr {
    unsigned char   core;
    unsigned char   rsv1;
    unsigned char   rsv2;
    unsigned char   sub_cmd;
}ar_hdr_t;

typedef struct ar_ioctl_cmd{
	pkt_header_t    hd;
	ar_hdr_t sub_cmd;
} ar_ioctl_cmd_t;

extern int inputhub_mcu_write_cmd_adapter(const void *buf, unsigned int length,
					  struct read_info *rd);


/*ENV -S*/
typedef struct env_init {
	environment_type_t context;
	char buf[CONTEXT_PRIVATE_DATA_MAX];
} __packed env_init_t;

typedef struct {
	ar_ioctl_cmd_t ar_cmd;
	env_init_t cfg;
}env_init_cmd_t;

typedef struct env_enable_header {
	unsigned int event_type;
	unsigned int context;
	unsigned int report_interval;
	unsigned int len;
}__packed env_enable_header_t;

typedef struct env_enable {
	env_enable_header_t head;
	char buf[CONTEXT_PRIVATE_DATA_MAX];
} __packed env_enable_t;

typedef struct ar_port {
	struct work_struct work;
	struct wake_lock wlock;
	struct softtimer_list   sleep_timer;
	struct list_head list;
	unsigned int channel_type;
	ar_data_buf_t ar_buf;
	ar_data_buf_t env_buf;
	context_iomcu_cfg_t  ar_config;
	unsigned int portid;
	unsigned int flushing;
	unsigned int need_awake;
	unsigned int need_hold_wlock;
	unsigned int work_para;
} ar_port_t;

typedef struct env_dev {
	env_init_t env_init[AR_ENVIRONMENT_END];
	env_enable_t env_enable[AR_ENVIRONMENT_END];
	ar_port_t *env_port;
} __packed envdev_priv_t;

/*ENV -E*/
typedef struct ar_device {
	struct list_head list;
	unsigned int service_type ;
	struct mutex lock;
	struct mutex state_lock;
	unsigned int denial_sevice;
	struct completion get_complete;
	ar_state_shmen_t activity_shemem;
	context_dev_info_t	 ar_devinfo;
	context_dev_info_t env_devinfo;
	envdev_priv_t envdev_priv;
} ar_device_t;

typedef struct hoc_database_head {
	int len;
	int cmd;
	int oper_type;
	int key_num;
}__packed env_database_head_t;

typedef struct env_data_download {
	unsigned int len;
	void __user *bufaddr;
}__packed env_data_download_t;

typedef struct {
	environment_type_t context;
	unsigned int event_type;
} __packed env_disable_cmd_t;

#endif
