

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/udp.h>
#include <net/sock.h>
#include "../emcom_netlink.h"
#include "../emcom_utils.h"
#include <huawei_platform/emcom/smartcare/smartcare.h>

#ifdef CONFIG_HW_SMARTCARE_FI
#include <huawei_platform/emcom/smartcare/fi/fi.h>
#endif


void smartcare_event_process(int32_t event, uint8_t *pdata, uint16_t len)
{
	switch (event)
	{
		#ifdef CONFIG_HW_SMARTCARE_FI
		case NETLINK_EMCOM_DK_SMARTCARE_FI_APP_LAUNCH:
		case NETLINK_EMCOM_DK_SMARTCARE_FI_APP_STATUS:
			fi_event_process(event, pdata, len);
			break;
		#endif

		default:
			EMCOM_LOGE(" : smartcare received unsupported message");
			break;
	}
}


void smartcare_init(void)
{
	#ifdef CONFIG_HW_SMARTCARE_FI
	fi_init();
	#endif

	return;
}


void smartcare_deinit(void)
{
	#ifdef CONFIG_HW_SMARTCARE_FI
	fi_deinit();
	#endif

	return;
}

EXPORT_SYMBOL(smartcare_event_process);
EXPORT_SYMBOL(smartcare_init);
EXPORT_SYMBOL(smartcare_deinit);

