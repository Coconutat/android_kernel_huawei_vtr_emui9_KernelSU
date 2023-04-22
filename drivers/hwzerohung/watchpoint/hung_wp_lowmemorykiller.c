#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/rcupdate.h>
#include <linux/notifier.h>
#include <linux/atomic.h>

#include <chipset_common/hwzrhung/zrhung.h>
#ifdef  CONFIG_LOG_JANK
#include <huawei_platform/log/log_jank.h>
#endif
#include "hung_wp_lowmemorykiller.h"


static char lmkwp_logbuf[MAX_WP_BUFSIZE];
static lmkwp_main_t lmk_wp;
static int config_retry = 3;

static inline void lmkwp_config_show(lmkwp_config_t *this, const char *tag)
{
	LMKWP_WARN("%s: is_ready(%d) enabled(%d) threshold(%u) period(%u) silence(%u) debuggable(%d)\n",
			tag, this->is_ready, this->enabled, this->threshold,
			jiffies_to_msecs(this->period), jiffies_to_msecs(this->silence), this->debuggable);
}

static inline u8 lmkwp_config_is_ready(void)
{
	return lmk_wp.config.is_ready;
}

static inline u8 lmkwp_config_is_enabled(void)
{
	return lmk_wp.config.enabled;
}

static inline u8 lmkwp_config_is_debuggable(void)
{
	return lmk_wp.config.debuggable;
}

static inline u64 lmkwp_config_get_period(void)
{
	return lmk_wp.config.period;
}

static inline unsigned int lmkwp_config_get_threshold(void)
{
	return lmk_wp.config.threshold;
}

static int lmkwp_parse_config(char *str, uint32_t len, lmkwp_config_t *cfg_ptr)
{
	lmkwp_config_t config = {0};
	int ret = 0;
	int i_enable = 0;
	int i_debuggable = 0;

	if (str == NULL || len == 0 || cfg_ptr == NULL)
		return -1;

	ret = sscanf(str, " %1d,%llu,%u,%llu,%1d", &i_enable, /* the space is to skipped spaces in str*/
			&config.period, &config.threshold, &config.silence, &i_debuggable);
	if (ret != 5) {
		LMKWP_WARN("parse config failed parameters(%d)\n", ret);
		return -1;
	}

	if (cfg_ptr) {
		config.enabled = (i_enable!=0)?1:0;
		config.debuggable = (i_debuggable!=0)?1:0;
		cfg_ptr->debuggable = config.debuggable;
		cfg_ptr->enabled = config.enabled;
		cfg_ptr->period = msecs_to_jiffies(config.period);
		cfg_ptr->threshold = config.threshold;
		cfg_ptr->silence = msecs_to_jiffies(config.silence);
		lmkwp_config_show(cfg_ptr, "config_parser");
	}
	return 0;
}


static u8 lmkwp_config_load(void)
{
	char config_str[128] = {0};
	int ret = 0;
	lmkwp_config_t *config = &lmk_wp.config;

	ret = zrhung_get_config(ZRHUNG_WP_LMKD, config_str, sizeof(config_str));
	if (ret > 0) {
		LMKWP_WARN("zrhung config not ready, wait...\n");
		config->is_ready = false;
	} else if (ret < 0) {
		LMKWP_WARN("zrhung lmkwp is not enabled\n");
		config->is_ready = true;
		config->enabled = false;
	} else {
		LMKWP_INFO("config string(%s)\n", config_str);
		if (lmkwp_parse_config(config_str, sizeof(config_str), config) != 0)
			config->enabled = false; /* if config is false, disable */

		config->is_ready = true;
	}

	lmkwp_config_show(config, "config_load");

	return config->is_ready;
}

static void lmkwp_event_ctor(lmkwp_event_t *this,
		struct task_struct *selected, struct shrink_control *sc,
		long cache_size, long cache_limit, short adj, long free)
{
	this->stamp = get_jiffies_64();
	memcpy(&this->sc, sc, sizeof(struct shrink_control));

	/* copy selected process info */
	this->selected_pid = selected->pid;
	this->selected_tgid = selected->tgid;
	memcpy(this->selected_comm, selected->comm, TASK_COMM_LEN);

	/* copy current process info */
	this->cur_pid = current->pid;
	this->cur_tgid = current->tgid;
	memcpy(this->cur_comm, current->comm, TASK_COMM_LEN);

	/* current memory info */
	this->cache_size = cache_size;
	this->cache_limit = cache_limit;
	this->adj = adj;
	this->free = free;
}

static void lmkwp_event_show(lmkwp_event_t *this, bool is_free)
{
	char prefix[3] = {0};

	if (is_free)
		strcpy(prefix, ">>");

	if (this->stamp == 0)
		LMKWP_INFO("%slmkwp_event: stamp:0\n", prefix);
	else
		LMKWP_INFO("%slmkwp_event: stamp:%llu sc->gfp_mask:%u selected pid:%d, tgid:%d comm:%s\n" \
				"    current  pid:%d, tgid:%d comm:%s cache size:%ld, limit:%ld, free:%ld, adj:%d\n",
				prefix, this->stamp, this->sc.gfp_mask,
				this->selected_pid, this->selected_tgid, this->selected_comm,
				this->cur_pid, this->cur_tgid, this->cur_comm,
				this->cache_size, this->cache_limit, this->free, this->adj);
}

static int lmkwp_event_format(lmkwp_event_t *this, char *buf, size_t len)
{
	return scnprintf(buf, len,
			"Killing '%s'(%d), tgid=%d, adj %hd\n" \
			"   to free memory on behalf of '%s' (%d) because\n" \
			"   cache %ldkB is below limit %ldkB\n" \
			"   Free memory is %ldkB above reserved (0x%x)\n",
			this->selected_comm, this->selected_pid, this->selected_tgid, this->adj,
			this->cur_comm, this->cur_pid,
			this->cache_size, this->cache_limit,
			this->free, this->sc.gfp_mask);
}

static size_t lmkwp_format_evenets(unsigned int pos)
{
	unsigned int s = pos;
	size_t total_len = sizeof(lmkwp_logbuf);
	size_t written_len = 0;

	for (; s != lmk_wp.free; s = (s+1)%MAX_EVENTS) {
		size_t w_len = lmkwp_event_format(&lmk_wp.events[s], lmkwp_logbuf+written_len, total_len-written_len);

		written_len += w_len;
	}

	return written_len;
}

static bool lmkwp_check_threshold(unsigned int *pos)
{
	lmkwp_config_t *config = &lmk_wp.config;
	lmkwp_event_t *events = lmk_wp.events;
	unsigned int last = lmk_wp.free;
	unsigned int new = lmk_wp.free == 0 ? (MAX_EVENTS - 1) : (lmk_wp.free - 1);

	unsigned int distance = last >= config->threshold?last-config->threshold:MAX_EVENTS+last-config->threshold;

	bool need_report = false;

	if (events[distance].stamp == 0)
		return need_report;

	if (events[new].stamp > events[distance].stamp + lmkwp_config_get_period()) {
		LMKWP_INFO("events[%d, %llu] to events [%d, %llu] not to upload",
				distance, events[distance].stamp, new, events[new].stamp);
		return need_report;
	}

	LMKWP_INFO("events[%d, %llu] to events [%d, %llu] need to upload",
			distance, events[distance].stamp, new, events[new].stamp);
	need_report = true;

	if (pos)
		*pos = distance;

	return need_report;
}

static void lmkwp_show_events(void)
{
	int i;

	for (i = 0; i < MAX_EVENTS; i++) {
		lmkwp_event_show(lmk_wp.events+i, i==lmk_wp.free);
	}
}

static lmkwp_event_t *lmkwp_get_slot(void)
{
	lmkwp_event_t *ptr = lmk_wp.events+lmk_wp.free;

	lmk_wp.free++;
	lmk_wp.free = lmk_wp.free%MAX_EVENTS;

	return ptr;
}

int lmkwp_init(void)
{
	LMKWP_INFO("lmkwp init sucess\n");
	memset(&lmk_wp, 0, sizeof(lmkwp_main_t));

	/*lmkwp_config_load(); */

	return 0;
}

void lmkwp_report(struct task_struct *selected, struct shrink_control *sc, long cache_size, long cache_limit, short adj, long free)
{
	unsigned int pos = 0;
	lmkwp_event_t *cur_event = NULL;

	/* check if need to load configure */
	if (!lmkwp_config_is_ready()) {
		if (config_retry > 0) {
			lmkwp_config_load();
			config_retry--;
		} else {
			return;
		}
	}

	/* check if lmkwp is enabled */
	if (!lmkwp_config_is_enabled()) {
		LMKWP_INFO("report: lmkwp disabled ,exit\n");
		return;
	}

	/* get free slot event */
	cur_event = lmkwp_get_slot();
	lmkwp_event_ctor(cur_event, selected, sc, cache_size, cache_limit, adj, free);

	if (cur_event->stamp < lmk_wp.last_report_stamp + lmk_wp.config.silence) {
		LMKWP_INFO("report: curr(%llu) less then last report(%llu)+silence(%llu)\n",
				cur_event->stamp, lmk_wp.last_report_stamp, lmk_wp.config.silence);
		return;
	}

	/* check if need to report events */
	if (true == lmkwp_check_threshold(&pos)) {
		lmkwp_format_evenets(pos);

		//LMKWP_INFO("%s", lmkwp_logbuf);
		lmk_wp.last_report_stamp = cur_event->stamp;
		zrhung_send_event(ZRHUNG_WP_LMKD, "K,S", lmkwp_logbuf);
#ifdef  CONFIG_LOG_JANK
		LOG_JANK_D(349,"#ARG1:<%s>#ARG2:<%d>",
				current->comm,
				lmkwp_config_get_threshold());
#endif
	}
}

/* debug interface for every command */
/*#define LMKWP_LOAD_CONFIG_CMD "load_config"
#define LMKWP_SET_CONFIG_CMD  "set_config"
#define LMKWP_REPORT_EVENT_CMD  "report_event"
#define LMKWP_SHOW_EVENTS_CMD "show_events"
char debug_str[64] = {0};
static int lmkwp_debug_index;

int lmkwp_debug_set(const char *buffer, const struct kernel_param *kp)
{
	int ret = 0;

	if (!lmkwp_config_is_debuggable())
		return ret;

	ret = param_set_copystring(buffer, kp);

	LMKWP_INFO("debug string buffer(%s) = debug_str(%s)\n", buffer, debug_str);
	if (!strncmp(debug_str, LMKWP_LOAD_CONFIG_CMD, strlen(LMKWP_LOAD_CONFIG_CMD))) {
		lmkwp_config_load();

	} else if (!strncmp(debug_str, LMKWP_SET_CONFIG_CMD, strlen(LMKWP_SET_CONFIG_CMD))) {
		uint32_t len = strlen(debug_str + strlen(LMKWP_SET_CONFIG_CMD));

		lmkwp_parse_config(debug_str + strlen(LMKWP_SET_CONFIG_CMD), len, &lmk_wp.config) ;

	} else if (!strncmp(debug_str, LMKWP_REPORT_EVENT_CMD, strlen(LMKWP_REPORT_EVENT_CMD))) {
		struct shrink_control sc = {
			.gfp_mask = 0xff,
			.nr_to_scan = 10,
		};

		lmkwp_report(current, &sc, lmkwp_debug_index,
				lmkwp_debug_index, lmkwp_debug_index, lmkwp_debug_index);
		lmkwp_debug_index++;

	} else if (!strncmp(debug_str, LMKWP_SHOW_EVENTS_CMD, strlen(LMKWP_SHOW_EVENTS_CMD))) {
		lmkwp_show_events();

	}

	return ret;
}

static struct kernel_param_ops lmkwp_debug_ops = {
	.set = lmkwp_debug_set,
	.get = param_get_string,
};

static struct kparam_string lmk_param_string = {
	.maxlen = sizeof(debug_str),
	.string = debug_str,
};

module_param_cb(lmkwp_debug, &lmkwp_debug_ops, &lmk_param_string, S_IRUGO | S_IWUSR);
__MODULE_PARM_TYPE(lmkwp_debug, "lmkwp debug node");*/
