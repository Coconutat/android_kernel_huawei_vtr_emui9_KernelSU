#ifndef _POWER_DEBUG_H_
#define _POWER_DEBUG_H_

#define POWER_DBG_NODE_NAME_LEN      (16)

/* power debugfs interface template file operations */
typedef ssize_t (*power_dgb_show)(void *, char *, ssize_t);
typedef ssize_t (*power_dgb_store)(void *, const char *, ssize_t);

struct power_dbg_attr {
	char name[POWER_DBG_NODE_NAME_LEN];
	void *dev_data;
	power_dgb_show show;
	power_dgb_store store;
	struct list_head list;
};

#ifdef CONFIG_HUAWEI_POWER_DEBUG
void power_dbg_ops_register(char *name, void *dev_data, power_dgb_show show, power_dgb_store store);

#else
static inline void power_dbg_ops_register(char *name, void *dev_data, power_dgb_show show, power_dgb_store store)
{
	return;
}
#endif /* end of CONFIG_HUAWEI_POWER_DEBUG */

#endif /* end of _POWER_DEBUG_H_ */