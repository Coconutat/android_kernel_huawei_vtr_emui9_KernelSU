#include "lcdkit_dbg.h"
#include "lcdkit_btb_check.h"
#include "lcdkit_parse.h"

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#include <linux/sched.h>
#include "lcdkit_dbg.h"
#endif

extern int lcdkit_gpio_cmds_tx(unsigned int gpio_lcdkit_btb, int gpio_optype);
extern int lcdkit_gpio_pulldown(void * btb_vir_addr);
extern int lcdkit_gpio_pullup(void * btb_vir_addr);
extern int lcdkit_dsi_rx(void* pdata, uint32_t* out, int len, struct lcdkit_dsi_panel_cmds* cmds);
#ifndef CONFIG_ARCH_QCOM
extern void lcdkit_hs_lp_switch(void* pdata, int mode);
#endif

extern struct dsm_client* lcd_dclient;

/*lcd btb check*/
int btb_floating_times = 0;
int btb_highlever_times = 0;
struct lcdkit_btb_info lcdkit_btb_inf = {0, 0, 0};

static int btb_check_state(void)
{
	int ret = 0;
	struct lcdkit_btb_info *btb_info_pr = &lcdkit_btb_inf;
	void * btb_vir_addr = 0;
	int error_floating = 0;
	int error_highlever = 0;
	int pulldown_read = 1;
	int pullup_read = 1;
	char buf[BUF_LENGTH] = {'\0'};

	if (btb_info_pr->gpio_lcdkit_btb == 0) {
		LCDKIT_ERR("get btb gpio failed!\n");
		return CHECK_SKIPPED;
	}
	lcdkit_gpio_cmds_tx(btb_info_pr->gpio_lcdkit_btb, BTB_GPIO_REQUEST);

	if (btb_info_pr->btb_con_addr == 0) {
		LCDKIT_ERR("get btb config address failed!\n");
		return CHECK_SKIPPED;
	}
	btb_vir_addr = (void *)ioremap_wc(btb_info_pr->btb_con_addr, sizeof(btb_info_pr->btb_con_addr));	/* IO config address remap */
	if (btb_vir_addr == NULL) {
		LCDKIT_ERR("btb_con_addr ioremap error !\n");
		return CHECK_SKIPPED;
	}

	if (!lcdkit_gpio_pulldown(btb_vir_addr)) {		/* config pull-down and read */
		LCDKIT_ERR("btb set gpio pulldown failed!\n");
		return CHECK_SKIPPED;
	}
	pulldown_read = lcdkit_gpio_cmds_tx(btb_info_pr->gpio_lcdkit_btb, BTB_GPIO_READ);

	if (!lcdkit_gpio_pullup(btb_vir_addr)) {		/* config pull-up and read */
		LCDKIT_ERR("btb set gpio pullup failed!\n");
		return CHECK_SKIPPED;
	}
	pullup_read = lcdkit_gpio_cmds_tx(btb_info_pr->gpio_lcdkit_btb, BTB_GPIO_READ);

	lcdkit_gpio_cmds_tx(btb_info_pr->gpio_lcdkit_btb, BTB_GPIO_FREE);
	iounmap(btb_vir_addr);		/* IO config address unmap */

	if(pulldown_read != pullup_read) {
		error_floating = 1;	/*make error flag*/
		if (btb_floating_times < MAX_REPPORT_TIMES+1){	/*Device Radar error report is limited to 5, too many error records are not necessary*/
			btb_floating_times++;
		}
		memcpy(buf, "LCD not connected!\n", sizeof("LCD not connected!\n"));
		LCDKIT_ERR("btb is floating status, LCD not connected!\n");
		goto check_error;
	} else if(pulldown_read == BTB_STATE_HIGH_LEVER || pullup_read == BTB_STATE_HIGH_LEVER) {
		error_highlever = 1;	/*make error flag*/
		if (btb_highlever_times < MAX_REPPORT_TIMES+1){
			btb_highlever_times++;
		}
		memcpy(buf, "LCD is connected error!\n", sizeof("LCD is connected error!\n"));
		LCDKIT_ERR("btb is high-lever status, LCD is connected error!\n");
		goto check_error;
	} else {
		return CHECKED_OK;
	}

check_error:
#if defined (CONFIG_HUAWEI_DSM)
	if ( NULL == lcd_dclient )
	{
		LCDKIT_ERR(": there is not lcd_dclient!\n");
		return CHECK_SKIPPED;
	}
	if ((error_floating == 1 && btb_floating_times <= MAX_REPPORT_TIMES)
		|| (error_highlever == 1 && btb_highlever_times <= MAX_REPPORT_TIMES)) {	/*when error times is more than 5, don't report anymore*/
		ret = dsm_client_ocuppy(lcd_dclient);
		if ( !ret ) {
			dsm_client_record(lcd_dclient, buf);
			dsm_client_notify(lcd_dclient, DSM_LCD_BTB_CHECK_ERROR_NO);
		}else{
			LCDKIT_ERR("dsm_client_ocuppy ERROR:retVal = %d\n", ret);
			return CHECK_SKIPPED;
		}
	}
#endif
	return CHECKED_ERROR;
}

int mipi_lcdkit_btb_check(void)
{
	struct lcdkit_btb_info *btb_info_pr = &lcdkit_btb_inf;

	if(btb_info_pr->btb_support == 0){	/*not support btb check*/
		LCDKIT_INFO("not support btb check\n");
		return CHECK_SKIPPED;
	}

	return btb_check_state();
}

#define MAX_ERROR_TIMES 100000000
#define CHECK_TIMES 0
#define CHECK_ERROR_TIMES 1
#define ERROR_TIMES 2
#define RECORD_ITEM_NUM 3
#define REG_READ_COUNT 1
void lcdkit_mipi_check(void* pdata)
{
    uint32_t read_value[MAX_REG_READ_COUNT] = {0};
    int i = 0;
    int len = REG_READ_COUNT;
    static int MIPI_CHECK_ERROR[MAX_REG_READ_COUNT][RECORD_ITEM_NUM]={{0}};    /*[i][0]-check times  [i][1]-check error times  [i][2]-mipi error times*/
    char* expect_ptr = lcdkit_info.panel_infos.mipi_check_value.buf;
#if defined (CONFIG_HUAWEI_DSM)
    struct timeval tv;
    int diskeeptime = 0;
    memset(&tv, 0, sizeof(struct timeval));
#endif
    char* panel_name = NULL;

    if (NULL == pdata || NULL == expect_ptr){
        LCDKIT_ERR("mipi check happened parameter error\n");
        return;
    }

    if (LCDKIT_DSI_LP_MODE
        == lcdkit_info.panel_infos.mipi_check_cmds.link_state){
#ifndef CONFIG_ARCH_QCOM
        lcdkit_hs_lp_switch(pdata, LCDKIT_DSI_LP_MODE);
#endif
    }
    lcdkit_dsi_rx(pdata, read_value, len, &lcdkit_info.panel_infos.mipi_check_cmds);
    if (LCDKIT_DSI_LP_MODE
        == lcdkit_info.panel_infos.mipi_check_cmds.link_state){
#ifndef CONFIG_ARCH_QCOM
        lcdkit_hs_lp_switch(pdata, LCDKIT_DSI_HS_MODE);
#endif
    }
    for (i = 0; i < lcdkit_info.panel_infos.mipi_check_cmds.cmd_cnt; i++)
    {
        if (MIPI_CHECK_ERROR[i][CHECK_TIMES] < MAX_ERROR_TIMES){
            MIPI_CHECK_ERROR[i][CHECK_TIMES]++;
        }
        if (read_value[i] != (uint32_t)expect_ptr[i]){
            if (MIPI_CHECK_ERROR[i][CHECK_ERROR_TIMES] < MAX_ERROR_TIMES){
                MIPI_CHECK_ERROR[i][CHECK_ERROR_TIMES]++;
            }
            if (MIPI_CHECK_ERROR[i][ERROR_TIMES] < MAX_ERROR_TIMES){
                MIPI_CHECK_ERROR[i][ERROR_TIMES] += (int)read_value[i];
            }

#if defined (CONFIG_HUAWEI_DSM)
            do_gettimeofday(&tv);
            if (tv.tv_sec >= lcdkit_info.panel_infos.display_on_record_time.tv_sec){
                diskeeptime = tv.tv_sec - lcdkit_info.panel_infos.display_on_record_time.tv_sec;
            }
            if (read_value[i] >= lcdkit_info.panel_infos.mipi_error_report_threshold){
                panel_name = lcdkit_info.panel_infos.panel_model ? \
                    lcdkit_info.panel_infos.panel_model : lcdkit_info.panel_infos.panel_name;
                if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient)) {
                    dsm_client_record(lcd_dclient, "%s:display_on_keep_time=%ds, reg_val[%d]=0x%x!\n",
                            panel_name,
                            diskeeptime,
                            lcdkit_info.panel_infos.mipi_check_cmds.cmds[i].payload[0],
                            read_value[i]);
                    dsm_client_notify(lcd_dclient, DSM_LCD_MIPI_TRANSMIT_ERROR_NO);
                }
            }
#endif
            LCDKIT_ERR("mipi check error[%d]: error times:%d! total error times:%d, check-error-times/check-times:%d/%d\n",
                i, read_value[i], MIPI_CHECK_ERROR[i][ERROR_TIMES], MIPI_CHECK_ERROR[i][CHECK_ERROR_TIMES],MIPI_CHECK_ERROR[i][CHECK_TIMES]);

            continue;
        }
        LCDKIT_INFO("mipi check nomal[%d]: total error times:%d, check-error-times/check-times:%d/%d\n",
                i,MIPI_CHECK_ERROR[i][ERROR_TIMES], MIPI_CHECK_ERROR[i][CHECK_ERROR_TIMES],MIPI_CHECK_ERROR[i][CHECK_TIMES]);
    }
}
