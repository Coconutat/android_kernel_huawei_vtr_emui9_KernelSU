#ifndef _SWITCH_FSA9685_H_
#define _SWITCH_FSA9685_H_

#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/usb/switch/usbswitch_common.h>

#define FSA9685_RD_BUF_SIZE               (32)
#define FSA9685_WR_BUF_SIZE               (64)

/* jig pin control for battery cut test */
#define JIG_PULL_DEFAULT_DOWN             (0)
#define JIG_PULL_UP                       (1)

/* usb state */
#define FSA9685_OPEN                      (0)
#define FSA9685_USB1_ID_TO_IDBYPASS       (1)
#define FSA9685_USB2_ID_TO_IDBYPASS       (2)
#define FSA9685_UART_ID_TO_IDBYPASS       (3)
#define FSA9685_MHL_ID_TO_CBUS            (4)
#define FSA9685_USB1_ID_TO_VBAT           (5)

enum err_oprt_reg_num {
	ERR_FSA9685_REG_MANUAL_SW_1 = 1,
	ERR_FSA9685_READ_REG_CONTROL = 2,
	ERR_FSA9685_WRITE_REG_CONTROL = 3,
};

struct fsa9685_device_info {
	struct device *dev;
	struct i2c_client *client;
	int device_id;

	struct mutex accp_detect_lock;
	struct mutex accp_adaptor_reg_lock;
	struct wake_lock usb_switch_lock;

	struct work_struct g_intb_work;
	struct delayed_work detach_delayed_work;

	u32 usbid_enable;
	u32 fcp_support;
	u32 scp_support;
	u32 mhl_detect_disable;
	u32 two_switch_flag; /* disable for two switch */
	u32 pd_support;
	u32 dcd_timeout_force_enable;
};

struct fsa9685_device_ops {
	int (*dump_regs)(char *buf);
	int (*jigpin_ctrl_store)(struct i2c_client *client, int jig_val);
	int (*jigpin_ctrl_show)(char *buf);
	int (*switchctrl_store)(struct i2c_client *client, int action);
	int (*switchctrl_show)(char *buf);

	int (*manual_switch)(int input_select);

	void (*detach_work)(void);
};

extern struct fsa9685_device_ops* usbswitch_fsa9685_get_device_ops(void);
extern struct fsa9685_device_ops* usbswitch_rt8979_get_device_ops(void);

extern int fsa9685_common_write_reg(int reg, int val);
extern int fsa9685_common_read_reg(int reg);
extern int fsa9685_common_write_reg_mask(int reg, int value, int mask);

#define ADAPTOR_BC12_TYPE_MAX_CHECK_TIME 100
#define WAIT_FOR_BC12_DELAY 5
#define ACCP_NOT_PREPARE_OK -1
#define ACCP_PREPARE_OK 0
#define BOOST_5V_CLOSE_FAIL -1
#define SET_DCDTOUT_FAIL -1
#define SET_DCDTOUT_SUCC 0

#endif /* end of _SWITCH_FSA9685_H_ */
