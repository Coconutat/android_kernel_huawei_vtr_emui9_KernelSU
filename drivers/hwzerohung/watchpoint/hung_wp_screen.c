#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/time.h>
#include <linux/input.h>
#include <linux/jiffies.h>


#ifdef CONFIG_HW_ZEROHUNG
#include "chipset_common/hwzrhung/zrhung.h"
#include "chipset_common/hwzrhung/hung_wp_screen.h"
#include <log/log_usertype/log-usertype.h>
#endif

#ifdef CONFIG_HISI_BB
#include <linux/hisi/rdr_hisi_platform.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/hisi_bootup_keypoint.h>
#include <mntn_public_interface.h>
#endif

#ifdef CONFIG_KEYBOARD_HISI_GPIO_KEY
#include <linux/hisi/hisi_powerkey_event.h>
#endif

#ifndef CONFIG_KEYBOARD_HISI_GPIO_KEY
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#define OF_GPIO_ACTIVE_LOW 0x1
#define PON_KPDPWR 0
#define PON_RESIN 1
#define PON_KEYS 2
#endif

#define DEFAULT_TIMEOUT 10
#define CONFIG_LEN_MAX  64
#define COMAND_LEN_MAX  256
#define CMD_BUF_FMT_BETA "T=WindowManager,T=PowerManagerService,n=system_server,"\
    "n=surfaceflinger,n=android.hardware.graphics.composer@2.1-service,"\
    "n=android.hardware.graphics.composer@2.2-service,d=%d,e=%lu"
#define CMD_BUF_FMT "T=WindowManager,T=PowerManagerService,n=system_server,d=%d,e=%lu"
#define BUFFER_TIME_END (1)
#define BUFFER_TIME_START (BUFFER_TIME_END+10)
#define NANOS_PER_MILLISECOND (1000000)
#define NANOS_PER_SECOND (NANOS_PER_MILLISECOND * 1000)

#define POWERKEYEVENT_MAX_COUNT 10
#define POWERKEYEVENT_DEFAULT_COUNT 3
#define POWERKEYEVENT_MAX_TIMEWINDOW 20
#define POWERKEYEVENT_DEFAULT_TIMEWINDOW 5
#define POWERKEYEVENT_MAX_LIMIT_MS 2000
#define POWERKEYEVENT_DEFAULT_LIMIT_MS 300
#define POWERKEYEVENT_MAX_REPORT_MIN 3600
#define POWERKEYEVENT_DEFAULT_REPORT_MIN 2
#define POWERKEYEVENT_TIME_LEN (POWERKEYEVENT_MAX_COUNT + 2)

typedef struct _hung_wp_screen_config {
    int enable;
    uint32_t timeout;
} hung_wp_screen_config;
typedef struct _zrhung_powerkeyevent_config {
	int enable;
	unsigned int timewindow;
	unsigned int count;
	unsigned int ignore_limit;
	unsigned int report_limit;
} zrhung_powerkeyevent_config;

typedef struct _hung_wp_screen_data {
    hung_wp_screen_config *config;
    struct timer_list timer;
#ifndef CONFIG_KEYBOARD_HISI_GPIO_KEY
    struct timer_list long_press_timer;
#endif
    struct workqueue_struct *workq;
    struct work_struct config_work;
    struct work_struct send_work;
    spinlock_t lock;
    int bl_level;
    int check_id;
    int tag_id;
} hung_wp_screen_data;

static bool config_done = false;
static bool init_done= false;
static hung_wp_screen_config on_config;
static hung_wp_screen_config off_config;
static hung_wp_screen_data data;
static unsigned int vkeys_pressed = 0;

static bool prkyevt_init_done = false;
static bool prkyevt_config_done = false;
static zrhung_powerkeyevent_config event_config = {0};
static unsigned int lastreport_time = 0;
static unsigned int lastprkyevt_time = 0;
static unsigned int powerkeyevent_time[POWERKEYEVENT_TIME_LEN] = {0};
static unsigned int newevt = 0;
static unsigned int headevt = 0;
static unsigned int reservedevt = 0;
struct work_struct powerkeyevent_config;
struct work_struct powerkeyevent_sendwork;

static struct notifier_block hung_wp_screen_powerkey_nb;


/*  ===========================================================================
** Function : zrhung_powerkeyevent_get_config
* @brief
*  Get configuration from zerohung_config.xml
** ==========================================================================*/
static int zrhung_powerkeyevent_get_config(zrhung_powerkeyevent_config *cfg)
{
	int ret = 0;
	char config_str[CONFIG_LEN_MAX] = {0};

	ret = zrhung_get_config(ZRHUNG_EVENT_POWERKEY, config_str, sizeof(config_str));
	if (0 != ret) {
		printk(KERN_ERR "%s: powerkeyevent read config fail: %d\n", __func__, ret);
		cfg->enable = 0;
		cfg->timewindow = POWERKEYEVENT_DEFAULT_TIMEWINDOW;
		cfg->count = POWERKEYEVENT_DEFAULT_COUNT;
		cfg->ignore_limit = POWERKEYEVENT_DEFAULT_LIMIT_MS;
		cfg->report_limit = POWERKEYEVENT_DEFAULT_REPORT_MIN;
		return ret;
	}
	sscanf(config_str,"%d,%d,%d,%d,%d", &cfg->enable, &cfg->timewindow,
            &cfg->count, &cfg->ignore_limit, &cfg->report_limit);
	printk(KERN_ERR "%s: ZRHUNG_EVENT_POWERKEY enable=%d, timewindow=%d,"
           " count=%d, ignore_limit=%d, report_limit=%d\n",
			__func__, cfg->enable, cfg->timewindow, cfg->count,
            cfg->ignore_limit, cfg->report_limit);
	if (cfg->timewindow <= 0 || cfg->timewindow > POWERKEYEVENT_MAX_TIMEWINDOW) {
		cfg->timewindow = POWERKEYEVENT_DEFAULT_TIMEWINDOW;
	}
    if (cfg->count <= 0 || cfg->count > POWERKEYEVENT_MAX_COUNT) {
        cfg->count = POWERKEYEVENT_DEFAULT_COUNT;
    }
    if (cfg->ignore_limit <= 0 || cfg->ignore_limit > POWERKEYEVENT_MAX_LIMIT_MS) {
        cfg->ignore_limit = POWERKEYEVENT_DEFAULT_LIMIT_MS;
    }
    if (cfg->report_limit <= 0 || cfg->report_limit > POWERKEYEVENT_MAX_REPORT_MIN) {
        cfg->report_limit = POWERKEYEVENT_DEFAULT_REPORT_MIN;
    }
	return ret;
}

/* ===========================================================================
** Function : zrhung_powerkeyevent_config_work
* @brief
*  Get configuration from zerohung_config.xml
** ==========================================================================*/
static void zrhung_powerkeyevent_config_work(struct work_struct *work)
{
	int ret = 0;

	ret = zrhung_powerkeyevent_get_config(&event_config);
	if (ret == 0) {
		prkyevt_config_done = true;
		printk(KERN_ERR "%s: read config success!\n", __func__);
	}
}
/* ===========================================================================
** Function : zrhung_powerkeyevent_send_work
* @brief
*  send event to zrhung when match out requirements model
** ==========================================================================*/
static void zrhung_powerkeyevent_send_work(struct  work_struct *work)
{
	printk(KERN_ERR "%s: ZRHUNG_EVENT_POWERKEY send to zerohung!\n", __func__);
    zrhung_send_event(ZRHUNG_EVENT_POWERKEY, NULL, NULL);
}
/* ===========================================================================
** Function : zrhung_powerkeyevent_report
* @brief
*  send event to zrhung when match out requirements model
** ==========================================================================*/
static void  zrhung_powerkeyevent_report(unsigned int dur, unsigned int end)
{
    unsigned int send_interval = 0;

    send_interval = end > lastreport_time ?
            ((end - lastreport_time) / 1000) : POWERKEYEVENT_DEFAULT_REPORT_MIN;
    if (unlikely(lastreport_time == 0))
        lastreport_time = end;
    else if (send_interval < event_config.report_limit) {
		printk(KERN_ERR "%s: powerkeyevent too fast to report: %d\n", __func__, end);
        return;
    }
    lastreport_time = end;
    queue_work(data.workq, &powerkeyevent_sendwork);
}
/* ===========================================================================
** Function : refresh_prkyevt_index
* @brief
*  just to build a loop in the array
** ==========================================================================*/
static unsigned int refresh_prkyevt_index(unsigned int event)
{
    unsigned int evt = 0;

    evt= event;
    if (evt < POWERKEYEVENT_MAX_COUNT)
        evt++;
    else
        evt = 0;
    return evt;
}

/* ===========================================================================
** Function : zrhung_new_powerkeyevent
* @brief
*  handle each powerkey event and check if match our model
** ==========================================================================*/
static void zrhung_new_powerkeyevent(unsigned int tmescs)
{
    unsigned int prkyevt_interval = 0;
    unsigned int evt_index = 0;
	int diff = 0;

    powerkeyevent_time[newevt] = tmescs;
    evt_index = (newevt >= headevt) ?
            (newevt - headevt) : (newevt + POWERKEYEVENT_MAX_COUNT + 1 - headevt);
    if (evt_index < (event_config.count - 1)) {
		printk(KERN_ERR "%s: powerkeyevent not enough-%d.\n", __func__, event_config.count);
    } else {
		diff = powerkeyevent_time[newevt] - powerkeyevent_time[headevt];
		if (diff < 0) {
			printk(KERN_ERR "%s: powerkeyevent sth wrong in record time.\n", __func__);
			return;
		}
        prkyevt_interval = (unsigned int)(diff / 1000);
		if (prkyevt_interval <= event_config.timewindow) {
			printk(KERN_ERR "%s: powerkeyevent checking send to zerohung %d.\n", __func__, tmescs);
			zrhung_powerkeyevent_report(prkyevt_interval, tmescs);
		}

		headevt = refresh_prkyevt_index(headevt);
	}
    newevt = refresh_prkyevt_index(newevt);
}

/* ===========================================================================
** Function : zrhung_powerkeyevent_handler
* @brief
*  handle each powerkey event and check if match our model
** ==========================================================================*/
static void zrhung_powerkeyevent_handler(void)
{
    unsigned int curtime = 0;
    unsigned long curjiff = 0;

    if (!prkyevt_config_done) {
        printk(KERN_ERR "%s: powerkeyevent prkyevt_config_done=%d, read config first!\n",
            __func__, prkyevt_config_done);
        queue_work(data.workq, &powerkeyevent_config);
        return;
    }
	if (event_config.enable == 0) {
		printk(KERN_ERR "%s: powerkeyevent check disable.\n", __func__);
		return;
	}
	printk(KERN_ERR "%s: powerkeyevent check start.\n", __func__);
    curjiff = jiffies;
    curtime = jiffies_to_msecs(curjiff);
    if (unlikely(lastprkyevt_time > curtime)) {
		printk(KERN_ERR "%s: powerkeyevent check but time overflow.\n", __func__);
        lastprkyevt_time = curtime;
        return;
    } else if ((curtime - lastprkyevt_time) < event_config.ignore_limit) {
		printk(KERN_ERR "%s: powerkeyevent user press powerkey too fast-time:%d\n", __func__, curtime);
        return;
    }
    lastprkyevt_time = curtime;
    zrhung_new_powerkeyevent(curtime);
}

/* ===========================================================================
** Function : hung_wp_screen_has_boot_success
*  screen-off watchpoint should not work before vm finish starting.
*  -function get bootup infomation
** ==========================================================================*/
int hung_wp_screen_has_boot_success(void)
{
	static int boot_success = 0;
	if (!boot_success)
#ifdef CONFIG_HISI_BB
		boot_success = (STAGE_BOOTUP_END == get_boot_keypoint());
#else
		boot_success = 1;
#endif
	return boot_success;
}

/* ===========================================================================
** Function : hung_wp_screen_vkeys_cb
*  if there is a volumn key press  between powerkey press and release,
*  the event that screen being set to off would be stopped so screen-off
*  watchpoint should not report this normal situation.
*  -function call back from volume keys driver and get state to vkeys_pressed
** ==========================================================================*/
void hung_wp_screen_vkeys_cb(unsigned int knum, unsigned int value)
{
	if (knum == WP_SCREEN_VUP_KEY || knum == WP_SCREEN_VDOWN_KEY) {
		vkeys_pressed |= knum;
	} else {
		printk(KERN_ERR "%s: unkown volume keynum:%u\n",__func__,knum);
	}
	return;
}

/* ===========================================================================
** Function : hung_wp_screen_has_vkeys_pressed
* @brief
*  get a value which indicates if any vkeys has pressed
** ==========================================================================*/
static unsigned int hung_wp_screen_has_vkeys_pressed(void)
{
	return vkeys_pressed;
}

/* ===========================================================================
** Function : hung_wp_screen_clear_vkeys_pressed
* @brief
*  clear the pressed vkeys value
** ==========================================================================*/
static void hung_wp_screen_clear_vkeys_pressed(void)
{
	vkeys_pressed = 0;
	return;
}

/*===========================================================================
** Function : hung_wp_screen_get_config
* @brief
*  Get configuration from zerohung_config.xml
** ==========================================================================*/
static int hung_wp_screen_get_config(int id, hung_wp_screen_config *cfg)
{
    int ret = 0;
    char _config[CONFIG_LEN_MAX] = {0};
    ret = zrhung_get_config(id, _config, CONFIG_LEN_MAX);
    if (0 != ret) {
        printk(KERN_ERR "%s: read config:%d fail: %d\n",
                 __func__, id, ret);
        cfg->enable = 0;
        cfg->timeout = DEFAULT_TIMEOUT;
        return ret;
    }
    sscanf(_config,"%d,%d", &cfg->enable, &cfg->timeout);
    printk(KERN_ERR "%s: id=%d, enable=%d, timeout=%d\n",
             __func__, id, cfg->enable, cfg->timeout);
    if (cfg->timeout <= 0) {
        cfg->timeout = DEFAULT_TIMEOUT;
    }
    return ret;
}


/*===========================================================================
** Function : hung_wp_screen_config_work
* @brief
*  Get configuration from zerohung_config.xml
** ==========================================================================*/
static void hung_wp_screen_config_work(struct work_struct *work)
{
    int ret_on = 0;
    int ret_off = 0;
    ret_on = hung_wp_screen_get_config(ZRHUNG_WP_SCREENON,&on_config);
    ret_off = hung_wp_screen_get_config(ZRHUNG_WP_SCREENOFF,&off_config);
    if (ret_on <= 0 && ret_off <= 0) {
        config_done = true;
        printk(KERN_ERR "%s: init done!\n", __func__);
    }
}


/*===========================================================================
** Function : hung_wp_screen_setbl
* @brief
* Called from lcd driver when setting backlight for mark:
*   For hisi platform, it's "hisifb_set_backlight" in hisi_fb_bl.c
** ==========================================================================*/
void hung_wp_screen_setbl(int level)
{
#ifndef HUNG_WP_FACTORY_MODE
    unsigned long flags;

    if (!init_done) {
        return;
    }

    spin_lock_irqsave(&(data.lock), flags);
    data.bl_level = level;
    if (!config_done) {
        spin_unlock_irqrestore(&(data.lock), flags);
        return;
    }
    if ((ZRHUNG_WP_SCREENON == data.check_id && 0 != level) ||
       (ZRHUNG_WP_SCREENOFF == data.check_id && 0 == level)) {
        printk(KERN_ERR "%s: check_id=%d, level=%d\n", __func__,
            data.check_id, data.bl_level);
        del_timer(&data.timer);
        data.check_id = ZRHUNG_WP_NONE;
    }
    spin_unlock_irqrestore(&(data.lock), flags);
#endif
}


/*===========================================================================
** Function : hung_wp_screen_send_work
* @brief
* Send envent through zrhung_send_event
** ==========================================================================*/
void hung_wp_screen_send_work(struct work_struct *work)
{
    unsigned long flags;
    char cmd_buf[COMAND_LEN_MAX] = {'\0'};
    u64 cur_stamp = 0;

#ifdef CONFIG_HISI_TIME
    cur_stamp = hisi_getcurtime() / NANOS_PER_SECOND;
#else
    cur_stamp = local_clock() / NANOS_PER_SECOND;
#endif
    printk(KERN_ERR "%s: cur_stamp=%d\n", __func__, cur_stamp);
    if (get_logusertype_flag() == BETA_USER)
    {
        snprintf(cmd_buf, COMAND_LEN_MAX, CMD_BUF_FMT_BETA,
            data.config->timeout+BUFFER_TIME_START, cur_stamp+BUFFER_TIME_END);
    }
    else
    {
        snprintf(cmd_buf, COMAND_LEN_MAX, CMD_BUF_FMT,
            data.config->timeout+BUFFER_TIME_START, cur_stamp+BUFFER_TIME_END);
    }
    printk(KERN_ERR "%s: cmd_buf: %s\n", __func__, cmd_buf);
    zrhung_send_event(data.check_id, cmd_buf, "none");
    printk(KERN_ERR "%s: send event: %d\n",__func__,data.check_id);
    spin_lock_irqsave(&(data.lock), flags);
    data.check_id = ZRHUNG_WP_NONE;
    spin_unlock_irqrestore(&(data.lock), flags);
}


/*===========================================================================
** Function : hung_wp_screen_send
* @brief
* Send event when timeout
** ==========================================================================*/
static void hung_wp_screen_send(unsigned long pdata)
{
    del_timer(&data.timer);
    show_state_filter(TASK_UNINTERRUPTIBLE);
    printk(KERN_ERR "%s: hung_wp_screen_%d end !\n", __func__,data.tag_id);
    queue_work(data.workq, &data.send_work);
}


/*===========================================================================
** Function : hung_wp_screen_start
* @brief
* Start up the timer work for screen on delay check.
** ==========================================================================*/
void hung_wp_screen_start(int check_id)
{
    //if already in a check, return...
    if (ZRHUNG_WP_NONE != data.check_id) {
        printk(KERN_ERR "%s: already in check_id: %d\n",
            __func__,data.check_id);
        return;
    }

    //check config everytime
    data.config = (check_id == ZRHUNG_WP_SCREENON) ? &on_config : &off_config;
    if (!data.config->enable) {
        printk(KERN_ERR "%s: id=%d, enable=%d\n", __func__,
            check_id, data.config->enable);
        return;
    }

	/*check if bootup success*/
	if (!hung_wp_screen_has_boot_success()) {
		printk(KERN_ERR "%s: bootup not completed yet!\n", __func__);
		return;
	}

    data.check_id = check_id;
    //start up the check timer
    if (timer_pending(&data.timer)) {
        del_timer(&data.timer);
    }
    data.timer.expires = jiffies + msecs_to_jiffies(data.config->timeout*1000);
    add_timer(&data.timer);
    printk(KERN_ERR "%s: going to check ID=%d timeout=%d\n",
        __func__,check_id, data.config->timeout);

    return;

}

#ifndef CONFIG_KEYBOARD_HISI_GPIO_KEY
/*===========================================================================
** Function : hung_wp_screen_qcom_pkey_press
* @brief
* Store the powerkey and volume down key state for key state check.
** ==========================================================================*/
void * hung_wp_screen_qcom_pkey_press(int type, int state)
{
    static int keys[PON_KEYS] = {0};

    if ((type >= PON_KPDPWR) && (type < PON_KEYS)) {
        keys[type] = state;
    }

    return (void *)keys;
}

static int hung_wp_screen_get_qcom_volup_gpio(void)
{
    static int volup_gpio = 0;
    struct device_node* node = NULL;
    struct device_node* snode = NULL;
    // -1 means already read fail before, so just quit...
    if (-1 == volup_gpio)
        return -1;
    // if alread read , just return
    if (0 != volup_gpio)
        return volup_gpio;
    // else try to read from device tree
    node = of_find_compatible_node(NULL, NULL, "gpio-keys");
    if (NULL == node) {
        volup_gpio = -1;
        printk(KERN_ERR "%s:gpio-keys not found!\n",__func__);
        return -1;
    }
    snode = of_find_node_by_name(node, "vol_up");
        if (NULL == snode) {
        volup_gpio = -1;
        printk(KERN_ERR "%s:vol_up not found!\n",__func__);
        return -1;
    }
    volup_gpio = of_get_named_gpio(snode , "gpios", 0);
    if (!gpio_is_valid(volup_gpio)) {
        volup_gpio = -1;
        printk(KERN_ERR "%s:invalid gpio: %d!\n",__func__,volup_gpio);
        return -1;
    }
    printk(KERN_ERR "%s:volup gpio: %d!\n",__func__,volup_gpio);
    return volup_gpio;
}

static int hung_wp_screen_qcom_vkey_get(void)
{
    int volup_gpio = hung_wp_screen_get_qcom_volup_gpio();
    int vol_up_state = (gpio_get_value(volup_gpio) ? 1 : 0) ^ OF_GPIO_ACTIVE_LOW;
    //printk(KERN_ERR "%s: get qcom vol_up gpio state %d\n", __func__, vol_up_state);

    int *state = (int *)hung_wp_screen_qcom_pkey_press(PON_KEYS, 0);
    //printk(KERN_ERR "%s: get qcom powerkey state 0x%x\n", __func__, *state);
    int vol_state = *(state + PON_RESIN) | vol_up_state;
    //printk(KERN_ERR "%s: get qcom vol state 0x%x\n", __func__, vol_state);

    return vol_state;
}

static void hung_wp_screen_qcom_lpress(unsigned long pdata)
{
    unsigned long flags;
    int *state = (int *)hung_wp_screen_qcom_pkey_press(PON_KEYS, 0);
    printk(KERN_ERR "%s: get powerkey state:%d\n", __func__, *state);

    spin_lock_irqsave(&(data.lock), flags);
    if (*state){
        printk(KERN_ERR "%s: check_id=%d, level=%d\n", __func__,
            data.check_id, data.bl_level);
        //don't check screen off when powerkey long pressed...
        int *check_off = (int *)pdata;
        *check_off = 0;
    }
    spin_unlock_irqrestore(&(data.lock), flags);

    del_timer(&data.long_press_timer);
}
#endif

/*===========================================================================
** Function : hung_wp_screen_powerkey_ncb
* @brief
* Call back from powerkey driver.
** ==========================================================================*/
void hung_wp_screen_powerkey_ncb(unsigned long event)
{
#ifndef HUNG_WP_FACTORY_MODE
    static int check_off = 0;
    int volkeys = 0;
    unsigned int vkeys_pressed_tmp = 0;
    unsigned long flags;

    if (!init_done) {
        return;
    }

    //return if not init
    if (!config_done) {
        printk(KERN_ERR "%s: config_done=%d, read config first!\n",
                        __func__, config_done);
        queue_work(data.workq, &data.config_work);
        return;
    }
    //wx524439
    volkeys = vkeys_pressed > 0 ?true:false;
/*
#ifdef CONFIG_KEYBOARD_HISI_GPIO_KEY
    volkeys = gpio_key_vol_updown_press_get();
#else
    volkeys = hung_wp_screen_qcom_vkey_get();
#endif
*/
    spin_lock_irqsave(&(data.lock), flags);
    if (WP_SCREEN_PWK_PRESS == event) {
        printk(KERN_ERR "%s: hung_wp_screen_%d start ! "
            "bllevel=%d, volkeys=%d\n", __func__,
            ++data.tag_id, data.bl_level, volkeys);
        check_off = 0;
        if (0 == data.bl_level) {
            hung_wp_screen_start(ZRHUNG_WP_SCREENON);
        } else if (0 == volkeys) {
			hung_wp_screen_clear_vkeys_pressed();
            check_off = 1;
#ifndef CONFIG_KEYBOARD_HISI_GPIO_KEY
            //start powerkey long press check timer
            printk(KERN_ERR "start longpress test timer!\n");
            data.long_press_timer.data = (unsigned long)&check_off;
            data.long_press_timer.expires = jiffies + msecs_to_jiffies(1000);
            if (!timer_pending(&data.long_press_timer))
                add_timer(&data.long_press_timer);
#endif
        }
        zrhung_powerkeyevent_handler();
    } else if (check_off){
        check_off = 0;
#ifndef CONFIG_KEYBOARD_HISI_GPIO_KEY
        del_timer(&data.long_press_timer);
#endif
        if (WP_SCREEN_PWK_RELEASE == event && 0 == volkeys) {
			vkeys_pressed_tmp = hung_wp_screen_has_vkeys_pressed();
			if (vkeys_pressed_tmp)
				printk(KERN_ERR "%s: vkeys_pressed_tmp:0x%x\n",__func__,vkeys_pressed_tmp);
			else
				hung_wp_screen_start(ZRHUNG_WP_SCREENOFF);
        }
    }
    spin_unlock_irqrestore(&(data.lock), flags);
#endif
}


static int hung_wp_screen_powerkey_reg_ncb(struct notifier_block *powerkey_nb,
 unsigned long event, void *data)
{
#ifdef CONFIG_KEYBOARD_HISI_GPIO_KEY
    if (event == HISI_PRESS_KEY_DOWN) {
        hung_wp_screen_powerkey_ncb(WP_SCREEN_PWK_PRESS);
    } else if (event == HISI_PRESS_KEY_UP) {
        hung_wp_screen_powerkey_ncb(WP_SCREEN_PWK_RELEASE);
    } else if (event == HISI_PRESS_KEY_1S) {
        hung_wp_screen_powerkey_ncb(WP_SCREEN_PWK_LONGPRESS);
    }
#endif
    return 0;
}

/*===========================================================================
** Function : hwhungtask_get_backlight
* @brief
*  return backlight to huawei_hung_task
**==========================================================================*/
int hwhungtask_get_backlight(void)
{
	return data.bl_level;
}

/*===========================================================================
** Function : hung_wp_screen_init
* @brief
*  Configuration init....
** ==========================================================================*/
static int __init hung_wp_screen_init(void)
{
    init_done = false;
    config_done = false;
#ifdef HUNG_WP_FACTORY_MODE
    printk(KERN_ERR "%s: factory mode, not init!\n", __func__);
#else
    printk(KERN_ERR "%s: +\n", __func__);
    data.bl_level = 1;
    data.tag_id = 0;
    data.check_id = ZRHUNG_WP_NONE;
    spin_lock_init(&(data.lock));
    init_timer(&data.timer);
    data.timer.function = hung_wp_screen_send;
    data.timer.data = 0;
#ifndef CONFIG_KEYBOARD_HISI_GPIO_KEY
    init_timer(&data.long_press_timer);
    data.long_press_timer.function = hung_wp_screen_qcom_lpress;
    data.long_press_timer.data = 0;
    hung_wp_screen_get_qcom_volup_gpio();
#endif
    data.workq = create_workqueue("hung_wp_screen_workq");
    if (NULL == data.workq) {
        printk(KERN_ERR "%s: create workq failed!\n", __func__);
        return -1;
    }
    INIT_WORK(&data.config_work, hung_wp_screen_config_work);
    INIT_WORK(&data.send_work, hung_wp_screen_send_work);
    INIT_WORK(&powerkeyevent_config, zrhung_powerkeyevent_config_work);
    INIT_WORK(&powerkeyevent_sendwork, zrhung_powerkeyevent_send_work);
#ifdef CONFIG_KEYBOARD_HISI_GPIO_KEY
    hung_wp_screen_powerkey_nb.notifier_call = hung_wp_screen_powerkey_reg_ncb;
    hisi_powerkey_register_notifier(&hung_wp_screen_powerkey_nb);
#endif
    init_done = true;
    printk(KERN_ERR "%s: -\n", __func__);
#endif
    return 0;
}

module_init(hung_wp_screen_init);
