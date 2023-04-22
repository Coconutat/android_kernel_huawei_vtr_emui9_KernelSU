#include "lcdkit_dbg.h"

//mdss_dsi_status_check_ctl
void mdss_dsi_status_check_ctl(struct msm_fb_data_type *mfd, int sheduled)
{
	struct mdss_panel_data *pdata = NULL;

	if(!mfd)
	{
		LCDKIT_ERR("mfd not available\n");
		return ;
	}

	if(!pstatus_data)
	{
		LCDKIT_ERR("pstatus_data not available\n");
		return ;
	}

	pdata = dev_get_platdata(&mfd->pdev->dev);
	if (!pdata)
    {
		LCDKIT_ERR("Panel data not available\n");
		return;
	}

	/*add qcom debug switch so we can close the esd check at sys*/
	/*if panel not enable esd check switch in dtsi,we do not check bta*/
	if(!lcdkit_info.panel_infos.esd_support)
	{
		LCDKIT_DEBUG("esd_check_enable = %d, don't need check esd!\n",
                        (int)lcdkit_info.panel_infos.esd_support);
		return;
	}

	if (dsi_status_disable)
    {
		LCDKIT_INFO("DSI status disabled\n");
		return ;
	}

	LCDKIT_DEBUG("scheduled=%d\n",sheduled);

	pstatus_data->mfd = mfd;

	if(sheduled)
	{
		schedule_delayed_work(&pstatus_data->check_status,
			msecs_to_jiffies(interval));
	}
	else
	{
		cancel_delayed_work_sync(&pstatus_data->check_status);
	}

}

