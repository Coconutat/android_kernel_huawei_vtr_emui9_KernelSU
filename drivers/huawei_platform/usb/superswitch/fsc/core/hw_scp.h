#ifndef __HW_SCP_H__
#define __HW_SCP_H__

/******************************************************************************
* Register addresses
******************************************************************************/

#define FCP_MAX_OUTPUT_VOL_NUM (2)
#define FCP_ADAPTOR_DETECT_OTHER -1
#define FCP_ADAPTOR_DETECT_SUCC 0
#define FCP_ADAPTOR_DETECT_FAIL 1

#define ACCP_ADAPTOR_DETECT_OTHER -1
#define ACCP_ADAPTOR_DETECT_SUCC 0
#define ACCP_ADAPTOR_DETECT_FAIL 1
#define ACCP_DETECT_MAX_COUT  (20)  /* accp detect MAX COUT */
#define ACCP_POLL_TIME        (100)   /* 100ms */
#define ACCP_RETRY_MAX_TIMES     (3)  /* accp retry max times */

#define FCP_CMD_SBRRD      0x0c
#define FCP_CMD_SBRWR      0x0b

#define FCP_SLAVE_REG_ID_OUT0               0x04
#define FCP_SLAVE_REG_DISCRETE_CAPABILITIES 0x21
#define FCP_SLAVE_REG_MAX_PWR               0x22
#define FCP_SLAVE_REG_OUTPUT_CONTROL        0x2b
#define FCP_SLAVE_REG_VOUT_CONFIG           0x2c

#define FCP_SLAVE_REG_DISCRETE_OUT_V(n)     (0x30+(n))

/* Register FCP_SLAVE_OUTPUT_CONTROL (0x2b)*/
#define FCP_SLAVE_SET_VOUT   (1<<0)

#define FCP_VOL_STEP         10
#define FCP_OUTPUT_VOL_5V     5
#define FCP_OUTPUT_VOL_9V     9
#define FCP_OUTPUT_VOL_12V   12


#define FCP_POLL_TIME        (100)   /* 100ms */
#define FCP_RETRY_MAX_TIMES     (2)  /* fcp retry max times */
#define ACCP_TRANSFER_POLLING_RETRY_TIMES (5)
#define MAX_U8 (0xff)

/******************************************************************************
* fcp definitions  end
******************************************************************************/

void FUSB3601_charge_register_callback(void);
void FUSB3601_scp_initialize(void);
int FUSB3601_vout_enable(int enable);
int FUSB3601_msw_enable(int enable);
#endif /* __DRIVERS_SWITCH_SMART_CHIP_H__ */
