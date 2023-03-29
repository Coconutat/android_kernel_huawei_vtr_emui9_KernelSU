#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/hisi/usb/hisi_usb.h>
#include "pogo_otg_id.h"
#include "dwc_otg_hicommon.h"
#include "hisi_usb_otg_type.h"
#include <linux/mfd/hisi_pmic.h>
#include <pmic_interface.h>
#include <linux/hisi/hisi_adc.h>
#include <huawei_platform/usb/huawei_pogopin.h>

static struct otg_gpio_id_dev *otg_gpio_id_dev_p = NULL;
static int startup_otg_sample = SAMPLE_DOING;
extern struct pogopin_device_ops  *g_pogopin_device_ops;
extern struct cc_detect_ops *cc_detect_ops;
extern struct completion dev_off_completion;

#define VBUS_IS_CONNECTED           0
#define DISABLE_USB_IRQ             0
#define FAIL                        (-1)
#define SAMPLING_TIME_OPTIMIZE_FLAG 1
#define SAMPLING_TIME_INTERVAL 10
#define SMAPLING_TIME_OPTIMIZE 5
#define VBATT_AVR_MAX_COUNT    10
#define ADC_VOLTAGE_LIMIT      150    //mV
#define ADC_VOLTAGE_MAX        1250   //mV
#define ADC_VOLTAGE_NEGATIVE   2000   //mV
#define USB_CHARGER_INSERTED   1
#define USB_CHARGER_REMOVE     0


/*************************************************************************************************
*  Function:       hw_is_usb_cable_connected
*  Discription:    Check whether Vbus has been exist.
*  Parameters:   void
*  return value:  zero:     Vbus isn't exist.
*                      no zero:  Vbus is exist.
**************************************************************************************************/
static int hw_is_usb_cable_connected(void)
{
	struct otg_gpio_id_dev *dev = otg_gpio_id_dev_p;
	int ret = 0;

	if (0 == dev->fpga_flag) {
		ret = (hisi_pmic_reg_read(PMIC_STATUS0_ADDR(0)) & (VBUS4P3_D10 | VBUS_COMP_VBAT_D20));
	}
	hw_usb_err("%s ret is %d !\n", __func__, ret);
	return ret;
}

/*************************************************************************************************
*  Function:       hw_otg_id_notifier_call
*  Description:    respond the charger_type events from USB PHY
*  Parameters:   usb_nb:usb notifier_block
*                      event:charger type event name
*                      data:unused
*  return value:  NOTIFY_OK-success or others
**************************************************************************************************/
static int hw_otg_id_notifier_call(struct notifier_block *usb_nb,
				    unsigned long event, void *data)
{
	if (DISABLE_USB_IRQ == event) {
		hw_usb_err("%s disable the otg irq!\n", __func__);
	} else {
		hw_usb_err("%s enable the otg irq!\n", __func__);
	}
	return NOTIFY_OK;
}


static int pogopin_event_notifier_call(struct notifier_block *usb_nb,
				    unsigned long event, void *data)
{
	int ret = -1;
	struct otg_dev *p = (struct otg_dev *)data;

	if((NULL == g_pogopin_device_ops) || (NULL == p))
		return ret;
	hw_usb_err("current_int_status = %d\n",g_pogopin_device_ops->di->current_int_status);
	switch(event){
        case CHARGER_TYPE_NONE:
            hw_usb_err("off mode!\n");
			if((TYPEC_INTERFACE == g_pogopin_device_ops->di->current_int_status) || 		//insert otg
					(POGOPIN_AND_TYPEC == g_pogopin_device_ops->di->current_int_status)) {//insert charger
				complete(&dev_off_completion);
			}
			msleep(50);
            break;
        case CHARGER_TYPE_SDP:
        case CHARGER_TYPE_CDP:
        case CHARGER_TYPE_DCP:
        case CHARGER_TYPE_UNKNOWN:
			hw_usb_err("charger mode!\n");
            break;
		case PLEASE_PROVIDE_POWER:
			hw_usb_err("otg mode!\n");
			break;
        default:
            hw_usb_err("%s ignore other types %u\n", __func__, event);
    }
	return NOTIFY_OK;
}
/*************************************************************************************************
*  Function:       hw_otg_id_adc_sampling
*  Description:    get the adc sampling value.
*  Parameters:   otg_gpio_id_dev_p: otg device ptr
*  return value:  -1: get adc channel fail
*                      other value: the adc value.
**************************************************************************************************/
static int hw_otg_id_adc_sampling(struct otg_gpio_id_dev *otg_gpio_id_dev_p)
{
	int avgvalue = 0;
	int vol_value = 0;
	int sum = 0;
	int i = 0;
	int sample_cnt = 0;

	/*get adc channel*/
	if (0 == otg_gpio_id_dev_p->otg_adc_channel){
		hw_usb_err("%s Get otg_adc_channel is fail!\n", __func__);
		return FAIL;
	}

	for(i = 0; i < VBATT_AVR_MAX_COUNT; i++){
		vol_value = hisi_adc_get_value(otg_gpio_id_dev_p->otg_adc_channel);
		/*add some interval*/
		if (SAMPLING_TIME_OPTIMIZE_FLAG == otg_gpio_id_dev_p->sampling_time_optimize && SAMPLE_DOING == startup_otg_sample) {
			udelay(SMAPLING_TIME_OPTIMIZE);
		} else {
			msleep(SAMPLING_TIME_INTERVAL);
		}
		if(vol_value < 0){
			hw_usb_err("%s The value from ADC is error", __func__);
			continue;
		}
		sum += vol_value;
		sample_cnt++;
	}

	if (0 == sample_cnt) {
		/* ESD cause the sample is always negative */
		avgvalue = ADC_VOLTAGE_NEGATIVE;
	}else {
		avgvalue = sum/sample_cnt;
	}
	hw_usb_err("%s The average voltage of ADC is %d\n", __func__, avgvalue);

	return avgvalue;
}


/*************************************************************************************************
*  Function:       hw_otg_id_intb_work
*  Discription:    Handle GPIO about OTG.
*  Parameters:   work: The work struct of otg.
*  return value:  void
**************************************************************************************************/
static void hw_otg_id_intb_work(struct work_struct *work)
{
	int gpio_value = -1;
	int avgvalue = 0;
	static bool is_otg_has_inserted = false;
	unsigned long timeout;

	if((NULL == g_pogopin_device_ops) || (NULL == cc_detect_ops))
		return;
	gpio_value = gpio_get_value(otg_gpio_id_dev_p->gpio);
	if (0 == gpio_value){
		avgvalue = hw_otg_id_adc_sampling(otg_gpio_id_dev_p);
		if((avgvalue >= 0) && (avgvalue <= ADC_VOLTAGE_LIMIT)){
			is_otg_has_inserted = true;
			g_pogopin_device_ops->di->current_int_status = g_pogopin_device_ops->get_interface_status();
			hw_usb_err("%s current_int_status = %d\n", __func__, g_pogopin_device_ops->di->current_int_status);
			hw_usb_err("%s-%d pogopin otg insert\n", __func__, __LINE__);
			hw_usb_err("1.cutoff CC\n");
			cc_detect_ops->typec_detect_disable(TRUE);
			hw_usb_err("2.waitfor typec off\n");
			if(g_pogopin_device_ops->di->current_int_status == TYPEC_INTERFACE){
				timeout = wait_for_completion_timeout(&dev_off_completion, msecs_to_jiffies(1000));
				hw_usb_err("%s  timeout = %ld\n", __func__, timeout);
			}else{
				timeout = wait_for_completion_timeout(&dev_off_completion, msecs_to_jiffies(200));
				hw_usb_err("%s  timeout = %ld\n", __func__, timeout);
			}
			hw_usb_err("3.start pogopin otg\n");
			g_pogopin_device_ops->enable_pogopin_otg();
			hw_usb_err("%s Send ID_FALL_EVENT\n", __func__);
			hisi_usb_otg_event(ID_FALL_EVENT);
		} else {
			hw_usb_err("%s avgvalue is %d.\n", __func__, avgvalue);
			is_otg_has_inserted = true;
		}
		reinit_completion(&dev_off_completion);
	} else {
		is_otg_has_inserted = false;
		hw_usb_err("1.cutoff pogopin otg\n");
		hw_usb_err("%s Send ID_RISE_EVENT\n", __func__);
		hisi_usb_otg_event(ID_RISE_EVENT);
		g_pogopin_device_ops->typec_charge_otg();
		hw_pogopin_dbg("2.waitfor pogopin otg off\n");
		timeout = wait_for_completion_timeout(&dev_off_completion, msecs_to_jiffies(800));
		hw_usb_err("%s  timeout = %ld\n", __func__, timeout);
		hw_pogopin_dbg("3.enable CC\n");
		cc_detect_ops->typec_detect_disable(FALSE);
		reinit_completion(&dev_off_completion);
		g_pogopin_device_ops->di->current_int_status = g_pogopin_device_ops->get_interface_status();
	}
	return;
}

/*************************************************************************************************
*  Function:       hw_otg_id_irq_handle
*  Discription:    Handle the GPIO200 irq.
*  Parameters:   irq: The number of irq for GPIO200
*                      dev_id: The id of devices.
*  return value:  IRQ_HANDLED
**************************************************************************************************/
static irqreturn_t hw_otg_id_irq_handle(int irq, void *dev_id)
{
	int gpio_value = -1;

	disable_irq_nosync(otg_gpio_id_dev_p->irq);
	disable_irq_nosync(g_pogopin_device_ops->di->pogopin_int_irq);
	gpio_value = gpio_get_value(otg_gpio_id_dev_p->gpio);

	if (0 == gpio_value){
		irq_set_irq_type(otg_gpio_id_dev_p->irq, IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND);
	} else {
		irq_set_irq_type(otg_gpio_id_dev_p->irq, IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND);
	}
	enable_irq(g_pogopin_device_ops->di->pogopin_int_irq);
	enable_irq(otg_gpio_id_dev_p->irq);
	schedule_work(&otg_gpio_id_dev_p->otg_intb_work);
	return IRQ_HANDLED;
}

/*************************************************************************************************
*  Function:       pogopin_otg_id_probe
*  Discription:    otg probe function.
*  Parameters:   pdev: The porinter of pdev
*  return value:  IRQ_HANDLED
**************************************************************************************************/
static int pogopin_otg_id_probe(struct platform_device *pdev)
{
	int ret = -1;
	int avgvalue = 0;
	struct device_node *np = NULL;
	struct device* dev = NULL;

	hw_usb_err("Enter %s funtion\n", __func__);

	if (strstr(saved_command_line, "androidboot.mode=charger")) {
		hw_usb_err("%s in health mode charge!\n", __func__);
		return ret;
	}

	if (NULL == pdev) {
		hw_usb_err("%s The pdev is NULL !\n", __func__);
		return ret;
	}

	if(!is_pogopin_support()){
		hw_usb_err("%s not support pogopin!\n", __func__);
		return ret;
	}

	otg_gpio_id_dev_p = (struct otg_gpio_id_dev *)devm_kzalloc(&pdev->dev, sizeof(*otg_gpio_id_dev_p), GFP_KERNEL);
	if (NULL == otg_gpio_id_dev_p) {
		hw_usb_err("%s alloc otg_dev failed! not enough memory\n", __func__);
		return ret;
	}
	platform_set_drvdata(pdev, otg_gpio_id_dev_p);
	otg_gpio_id_dev_p->pdev = pdev;
	otg_gpio_id_dev_p->otg_nb.notifier_call = hw_otg_id_notifier_call;
	ret = hisi_usb_otg_irq_notifier_register(&otg_gpio_id_dev_p->otg_nb);
	if (ret < 0) {
		hw_usb_err("%s hisi_usb_otg_irq_notifier_register failed\n", __func__);
		goto err_notifier_register;
	}

	otg_gpio_id_dev_p->pogopin_nb.notifier_call = pogopin_event_notifier_call;
	ret = hisi_charger_type_notifier_register(&otg_gpio_id_dev_p->pogopin_nb);
	if (ret < 0) {
		hw_usb_err("%s hisi_charger_type_notifier_register failed\n", __func__);
		goto err_charger_type_notifier_register;
	}

	dev = &pdev->dev;
	np = dev->of_node;

	ret = of_property_read_u32(np, "sampling_time_optimize", &(otg_gpio_id_dev_p->sampling_time_optimize));
	if (0 != ret) {
		hw_usb_err("%s Get sampling_time_optimize failed !!! \n", __func__);
	}

	ret = of_property_read_u32(np, "otg_adc_channel", &(otg_gpio_id_dev_p->otg_adc_channel));
	if (0 != ret) {
		hw_usb_err("%s Get otg_adc_channel failed !!! \n", __func__);
		goto err_property_read_u32;
	}

	ret = of_property_read_u32(np, "fpga_flag", &(otg_gpio_id_dev_p->fpga_flag));
	if (0 != ret) {
		otg_gpio_id_dev_p->fpga_flag = 0;
		hw_usb_err("%s in asic mode!\n", __func__);
	}

	otg_gpio_id_dev_p->gpio = of_get_named_gpio(np, "otg-gpio", 0);
	if (otg_gpio_id_dev_p->gpio < 0) {
		hw_usb_err("%s of_get_named_gpio error!!! gpio=%d.\n", __func__, otg_gpio_id_dev_p->gpio);
		goto err_of_get_named_gpio;
	}

	/*init otg intr handle work funtion*/
	INIT_WORK(&otg_gpio_id_dev_p->otg_intb_work, hw_otg_id_intb_work);
	/*
	* init otg gpio process
	*/
	ret = gpio_request(otg_gpio_id_dev_p->gpio, "otg_gpio_irq");
	if (ret < 0) {
		hw_usb_err("%s gpio_request error!!! ret=%d. gpio=%d.\n", __func__, ret, otg_gpio_id_dev_p->gpio);
		goto err_gpio_request;
	}

	avgvalue = hw_otg_id_adc_sampling(otg_gpio_id_dev_p);
	startup_otg_sample = SAMPLE_DONE;
	if ((avgvalue > ADC_VOLTAGE_LIMIT) && (avgvalue <= ADC_VOLTAGE_MAX)) {
		hw_usb_err("%s Set gpio_direction_output, avgvalue is %d.\n", __func__, avgvalue);
		ret = gpio_direction_output(otg_gpio_id_dev_p->gpio,1);
	} else {
		ret = gpio_direction_input(otg_gpio_id_dev_p->gpio);
		if (ret < 0) {
			hw_usb_err("%s gpio_direction_input error!!! ret=%d. gpio=%d.\n", __func__, ret, otg_gpio_id_dev_p->gpio);
			goto err_set_gpio_direction;
		}
	}

	otg_gpio_id_dev_p->irq = gpio_to_irq(otg_gpio_id_dev_p->gpio);
	if (otg_gpio_id_dev_p->irq < 0) {
		hw_usb_err("%s gpio_to_irq error!!! dev_p->gpio=%d, dev_p->irq=%d.\n", __func__, otg_gpio_id_dev_p->gpio, otg_gpio_id_dev_p->irq);
		goto err_gpio_to_irq;
	} else {
		hw_usb_err("%s otg irq is %d.\n", __func__, otg_gpio_id_dev_p->irq);
		if (0 == !hw_is_usb_cable_connected()) {
			hw_otg_id_notifier_call(NULL, !USB_CHARGER_INSERTED, NULL);
		} else {
			hw_otg_id_notifier_call(NULL, !USB_CHARGER_REMOVE, NULL);
		}
	}

	ret = request_irq(otg_gpio_id_dev_p->irq, hw_otg_id_irq_handle,
		IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND | IRQF_ONESHOT, "otg_gpio_irq", NULL);
	if (ret < 0) {
		hw_usb_err("%s request otg irq handle funtion fail!! ret:%d\n", __func__, ret);
		goto err_request_irq;
	}

	/* check the otg status when the phone poweron*/
	ret = gpio_get_value(otg_gpio_id_dev_p->gpio);
	if(0 == ret){
		/*call work function to handle irq*/
		schedule_work(&otg_gpio_id_dev_p->otg_intb_work);
	}

	usb_dbg("Exit [%s]-\n", __func__);
	return 0;

err_set_gpio_direction:
err_gpio_to_irq:
err_request_irq:
	gpio_free(otg_gpio_id_dev_p->gpio);
err_property_read_u32:
err_of_get_named_gpio:
err_gpio_request:
	hisi_charger_type_notifier_unregister(&otg_gpio_id_dev_p->pogopin_nb);
err_charger_type_notifier_register:
	hisi_usb_otg_irq_notifier_unregister(&otg_gpio_id_dev_p->otg_nb);
err_notifier_register:
	devm_kfree(&pdev->dev, otg_gpio_id_dev_p);
	otg_gpio_id_dev_p = NULL;
	return 0;
}

/*************************************************************************************************
*  Function:       pogopin_otg_id_remove
*  Description:   otg remove
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**************************************************************************************************/
static int pogopin_otg_id_remove(struct platform_device *pdev)
{
	if ((NULL == otg_gpio_id_dev_p) || (NULL == pdev)) {
		hw_usb_err("%s otg_gpio_id_dev_p or pdev is NULL!\n", __func__);
		return -ENODEV;
	}

	hisi_usb_otg_irq_notifier_unregister(&otg_gpio_id_dev_p->otg_nb);
	hisi_charger_type_notifier_unregister(&otg_gpio_id_dev_p->pogopin_nb);
	free_irq(otg_gpio_id_dev_p->irq, pdev);
	gpio_free(otg_gpio_id_dev_p->gpio);
	devm_kfree(&pdev->dev, otg_gpio_id_dev_p);
	otg_gpio_id_dev_p = NULL;
	return 0;
}

static struct of_device_id pogopin_otg_id_of_match[] = {
	{ .compatible = "huawei,pogopin-otg-by-id", },
	{ },
};

static struct platform_driver pogopin_otg_id_drv = {
	.probe		= pogopin_otg_id_probe,
	.remove     = pogopin_otg_id_remove,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "pogo_otg_id",
		.of_match_table	= pogopin_otg_id_of_match,
	},
};

static int __init pogopin_otg_id_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&pogopin_otg_id_drv);
	hw_usb_err("Enter [%s] function, ret is %d\n", __func__, ret);
	return ret;
}

static void __exit pogopin_otg_id_exit(void)
{
	platform_driver_unregister(&pogopin_otg_id_drv);
	return ;
}

late_initcall(pogopin_otg_id_init);
module_exit(pogopin_otg_id_exit);

MODULE_AUTHOR("huawei");
MODULE_DESCRIPTION("This module detect POGOPIN OTG connection/disconnection");
MODULE_LICENSE("GPL v2");

