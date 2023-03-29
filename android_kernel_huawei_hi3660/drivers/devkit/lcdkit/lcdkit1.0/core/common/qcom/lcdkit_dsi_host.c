#include "lcdkit_dbg.h"
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#include <linux/lcdkit_dsm.h>
#endif

#define ESD_FAIL     0
#define ESD_NOMAL    1

//mdss_dsi_bta_status_check
int mdss_dsi_bta_status_check(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	u32 i = 0;
	int ret = 0;

	if (ctrl_pdata == NULL)
    {
		LCDKIT_ERR("Invalid input data\n");
		/*
		 * This should not return error otherwise
		 * BTA status thread will treat it as dead panel scenario
		 * and request for blank/unblank
		 */
		return 0;
	}

	LCDKIT_DEBUG("Checking BTA status\n");

	/*if panel check error and enable the esd check bit in dtsi,report the event to hal layer*/
	if(lcdkit_info.panel_infos.esd_support)
	{
		for (i = 0; i < lcdkit_info.panel_infos.esd_check_num; i++)
		{
			ret = lcdkit_info.lcdkit_check_esd(ctrl_pdata);
			if (ret == 0)
			{
				break;
			}
		}

        // in qcom code: 0 - fail; 1 - ok, but lcdkit code: 1 - fail; 0 - ok
        // we need adapt here
        ret = (ret > 0) ? ESD_FAIL : ESD_NOMAL;
        #ifdef CONFIG_HUAWEI_DSM
        if (ret == ESD_FAIL)
        {
            lcdkit_report_dsm_err(DSM_LCD_ESD_STATUS_ERROR_NO, 0, 0, 0);
        }
        #endif
    }

	return ret;
}

