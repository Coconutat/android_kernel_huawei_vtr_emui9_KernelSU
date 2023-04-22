
#include "mdss_dsi.h"
#include <linux/mdss_io_util.h>
#include "lcdkit_dbg.h"
#include "lcdkit_common.h"
#include <linux/lcdkit_dsm.h>

extern atomic_t mipi_path_status;
int lcdkit_lock(void *pdata)
{
    (void)pdata;
    return !atomic_read(&mipi_path_status);
}

void lcdkit_release(void *pdata)
{
    (void)pdata;
    return;
}

struct platform_device *lcdkit_get_dsi_ctrl_pdev(void);
void lcdkit_debug_set_vsp_vsn(void *preg, int cnt)
{
    int i, rc;
#ifdef CONFIG_ARCH_SDM632 
    struct mdss_vreg *vreg = preg;
#else
    struct dss_vreg *vreg = preg;
#endif
    struct platform_device* pdev = lcdkit_get_dsi_ctrl_pdev();

    for (i = 0; i < cnt; i++)
    {
        if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_BIAS_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_dbg.lcdkit_panel_bias);
            update_value(vreg[i].max_voltage, lcdkit_dbg.lcdkit_panel_bias);
        }
        else if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_LAB_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_dbg.lcdkit_panel_vsp);
            update_value(vreg[i].max_voltage, lcdkit_dbg.lcdkit_panel_vsp);
        }
        else if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_IBB_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_dbg.lcdkit_panel_vsn);
            update_value(vreg[i].max_voltage, lcdkit_dbg.lcdkit_panel_vsn);
        }
    }
    
#ifdef CONFIG_ARCH_SDM632 
    rc = msm_mdss_config_vreg(&pdev->dev, vreg, cnt, 1);
#else
    rc = msm_dss_config_vreg(&pdev->dev, vreg, cnt, 1);
#endif
    if (rc)
    {
        LCDKIT_ERR(": failed to init regulator, rc=%d\n", rc);
    }

    return ;

}
