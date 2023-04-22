#include <linux/module.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/hisi/hisi_powerkey_event.h>
#include <linux/input.h>
#include <linux/notifier.h>
#include <log/log_usertype/log-usertype.h>

#define POWER_KEY_RELEASE		(0)
#define POWER_KEY_PRESS			(1)

static struct notifier_block  mini_bugreport_powerkey_nb;
static struct input_dev *key_dev = NULL;
static int mini_bugreport_powerkey_notifier_call(struct notifier_block *powerkey_nb,
							unsigned long event, void *data)
{
	/*we only focus on the event when press power key for 6s!*/
	if(event == HISI_PRESS_KEY_6S)
	{
#ifndef CONFIG_MINI_BUGREPORT_ENG
		if(BETA_USER != get_logusertype_flag())
		{
			return NOTIFY_DONE;
		}
#endif
		pr_info("[%s]response long press 6s interrupt!\n",__func__);
		input_report_key(key_dev, KEY_F23, POWER_KEY_PRESS);
		input_sync(key_dev);

		input_report_key(key_dev, KEY_F23, POWER_KEY_RELEASE);
		input_sync(key_dev);

		return NOTIFY_OK;
	}
	else
	{
		/*we ignore other event except PRESS_KEY_6S.*/
		return NOTIFY_DONE;
	}
}

static struct notifier_block  mini_bugreport_powerkey_nb = {
	.notifier_call = mini_bugreport_powerkey_notifier_call,
};

static int __init mini_bugreport_notifier_init(void)
{
	int ret = 0;
	key_dev=input_allocate_device();
	if (!key_dev) {
		printk("%s:Fail to allocate input device\n",__func__);
		ret = -ENOENT;
		return ret;
	}

	key_dev->name="minibugreport_key";
	key_dev->evbit[0] = BIT_MASK(EV_KEY);
	__set_bit(KEY_F23, key_dev->keybit);

	ret = input_register_device(key_dev);

	if(ret){
		printk("%s:Fail to register_device\n",__func__);
		input_free_device(key_dev);
		return ret;
	}

	hisi_powerkey_register_notifier(&mini_bugreport_powerkey_nb);

	return 0;

}

static void __exit mini_bugreport_notifier_exit(void)
{
	if(key_dev != NULL)
	{
		input_unregister_device(key_dev);
		input_free_device(key_dev);
		key_dev = NULL;
	}
	hisi_powerkey_unregister_notifier(&mini_bugreport_powerkey_nb);
}

module_init(mini_bugreport_notifier_init);
module_exit(mini_bugreport_notifier_exit);
