

#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/huawei_charger.h>
#include <protocol.h>
#ifdef CONFIG_INPUTHUB
#include <inputhub_bridge.h>
#endif

#define HWLOG_TAG sensorhub
HWLOG_REGIST();


static struct charge_extra_ops *g_extra_ops;
struct charge_device_ops *g_ops;
#ifdef CONFIG_INPUTHUB
extern sys_status_t iom3_sr_status;
extern atomic_t iom3_rec_state;
#endif

int charge_extra_ops_register(struct charge_extra_ops *ops)
{
	int ret = 0;

	if (ops != NULL) {
		g_extra_ops = ops;
	} else {
		hwlog_err("charge extra ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

/**********************************************************
*  Function:       charge_ops_register
*  Description:    register the handler ops for chargerIC
*  Parameters:   ops:operations interface of charge device
*  return value:  0-sucess or others-fail
**********************************************************/
int charge_ops_register(struct charge_device_ops *ops)
{
	int ret = 0;

	if (ops != NULL) {
		g_ops = ops;
	} else {
		hwlog_err("charge ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

int get_charger_vbus_vol(void)
{
	int ret = 0;
	unsigned int vbus_vol = 0;

	if (NULL == g_ops || NULL == g_ops->get_vbus
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
	|| (IOM3_RECOVERY_UNINIT == atomic_read(&iom3_rec_state))
#endif
	) {
		hwlog_err("g_ops is NULL or sensorhub is sleep.\n");
		return 0;
	}

	ret = g_ops->get_vbus(&vbus_vol);
	if(ret){
		hwlog_err("get_vbus fail.\n");
	}
	return vbus_vol;
}

int get_charger_ibus_curr(void)
{
	int ret;

	if (NULL == g_ops || NULL == g_ops->get_ibus
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
	|| (IOM3_RECOVERY_UNINIT == atomic_read(&iom3_rec_state))
#endif
	) {
		hwlog_err("g_ops is NULL or sensorhub is sleep.\n");
		return -1;
	}

	ret = g_ops->get_ibus();
	if(ret < 0) {
		hwlog_err("get ibus current fail.\n");
	}

	return ret;
}

/**********************************************************
*  Function:       charge_check_input_dpm_state
*  Description:    check charger whether VINDPM or IINDPM
*  Parameters:     NULL
*  return value:   1 means charger VINDPM or IINDPM
*                  0 means charger NoT DPM
*                  -1 means chargeIC not support this function
**********************************************************/
int charge_check_input_dpm_state(void)
{
	if (NULL == g_ops || NULL == g_ops->check_input_dpm_state
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
#endif
	) {
		hwlog_err("g_ops is NULL or sensorhub is sleep.\n");
		return -1;
	}
	return g_ops->check_input_dpm_state();
}

/**********************************************************
*  Function:       charge_check_charger_plugged
*  Description:    detect whether USB or adaptor is plugged
*  Parameters:     NULL
*  return value:    1 means USB or adpator plugged
*                   0 means USB or adaptor not plugged
*                   -1 means chargeIC not support this function
**********************************************************/
int charge_check_charger_plugged(void)
{
	if (NULL == g_ops || NULL == g_ops->check_charger_plugged) {
		return -1;
	}
	return g_ops->check_charger_plugged();
}

bool charge_check_charger_ts(void)
{
    	if (NULL == g_extra_ops || NULL == g_extra_ops->check_ts) {
		hwlog_err("g_extra_ops->check_ts is NULL.\n");
		return false;
	}
	return g_extra_ops->check_ts();
}

bool charge_check_charger_otg_state(void)
{
        if (NULL == g_extra_ops || NULL == g_extra_ops->check_otg_state) {
		hwlog_err("g_extra_ops->check_otg_state is NULL.\n");
		return false;
	}
	return g_extra_ops->check_otg_state();
}

/**********************************************************
*  Function:       fcp_get_stage_status
*  Description:    get the stage of fcp charge
*  Parameters:     NULL
*  return value:   NULL
**********************************************************/
enum fcp_check_stage_type fcp_get_stage_status(void)
{
	if (NULL == g_extra_ops || NULL == g_extra_ops->get_stage) {
		hwlog_err("g_extra_ops->get_stage is NULL.\n");
		return FCP_STAGE_DEFAUTL;
	}
	return g_extra_ops->get_stage();
}

/**********************************************************
*  Function:       charge_get_charger_type
*  Description:    get the charger type
*  Parameters:     NULL
*  return value:   NULL
**********************************************************/
enum usb_charger_type charge_get_charger_type(void)
{
	if (NULL == g_extra_ops || NULL == g_extra_ops->get_charger_type) {
		hwlog_err("g_extra_ops->get_charger_type is NULL.\n");
		return CHARGER_REMOVED;
	}
	return g_extra_ops->get_charger_type();
}

/**********************************************************
*  Function:       charge_set_charge_state
*  Description:    set charge stop or enable
*  Parameters:     state:0 stop 1 enable
*  return value:   old state
**********************************************************/
int charge_set_charge_state(int state)
{
	if (NULL == g_extra_ops || NULL == g_extra_ops->set_state) {
		return -1;
	}
	return g_extra_ops->set_state(state);
}

/**********************************************************
*  Function:       get_charge_current_max
*  Description:    get charge current max
*  Parameters:     NULL
*  return value:   charge current
**********************************************************/
int get_charge_current_max(void)
{
	if (NULL == g_extra_ops || NULL == g_extra_ops->get_charge_current) {
		return 0;
	}
	return g_extra_ops->get_charge_current();
}
