#include <linux/of.h>
#include <linux/of_address.h>
#include "inputhub_api.h"

int get_contexthub_dts_status(void)
{
	int len = 0;
	struct device_node* node = NULL;
	const char* status = NULL;
	static int ret = 0;
	static int once = 0;

	if(is_sensorhub_disabled()) {
		pr_err("[%s]is_sensorhub_disabled \n", __func__);
		return -1;
	}

	if (once){
		pr_info("[%s]status[%d]\n", __func__, ret);
		return ret;
	}

	node = of_find_compatible_node(NULL, NULL, "hisilicon,contexthub_status");
	if (node) {
		status = of_get_property(node, "status", &len);
		if(!status ) {
			pr_err("[%s]of_get_property status err\n", __func__);
			return -1;
		}

		if(strstr(status, "disabled")) {
			pr_info("[%s][disabled]\n", __func__);
			ret = -1;
		}
	}

	once = 1;
	pr_info("[%s][enabled]\n", __func__);
	return ret;
}
