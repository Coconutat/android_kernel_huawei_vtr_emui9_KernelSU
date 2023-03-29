#include "hisi_fb.h"
#include "hisi_fb_panel.h"
#include <huawei_platform/log/log_jank.h>
#include "include/lcd_common.h"

uint32_t gpio_lcd_btb = 0;
int error_floating_times = 0;
int error_highlever_times = 0;
struct lcd_btb_info btb_info = {GET_STATUS_FAILED, 0, 0};
struct semaphore lcd_cmd_sem;
struct timeval lcd_init_done;

int gpio_get_val(struct gpio_desc *cmds)
{
	int val=-1;
	struct gpio_desc *cm = NULL;

	cm = cmds;

	if ((cm == NULL) || (cm->label == NULL)) {
		HISI_FB_ERR("cm or cm->label is null!");
		return -1;
	}

	if (!gpio_is_valid(*(cm->gpio))) {
		HISI_FB_ERR("gpio invalid, dtype=%d, lable=%s, gpio=%d!\n",
			cm->dtype, cm->label, *(cm->gpio));
		return -1;
	}

	if (cm->dtype == DTYPE_GPIO_INPUT) {
		if (gpio_direction_input(*(cm->gpio)) != 0) {
			HISI_FB_ERR("failed to gpio_direction_input, lable=%s, gpio=%d!\n",
				cm->label, *(cm->gpio));
			return -1;
		}
		val = gpiod_get_value(gpio_to_desc(*(cm->gpio)));
	} else {
		HISI_FB_ERR("dtype=%x NOT supported\n", cm->dtype);
		return -1;
	}

	if (cm->wait) {
		if (cm->waittype == WAIT_TYPE_US) {
			udelay(cm->wait);
		} else if (cm->waittype == WAIT_TYPE_MS) {
			mdelay(cm->wait);
		} else {
			mdelay(cm->wait * 1000);
		}
	}

	return val;
}

static int btb_check_state(void)
{
	int ret = 0;
	struct lcd_btb_info *btb_info_pr = &btb_info;
	void * btb_vir_addr = 0;
	uint32_t btb_pull_data = 0;
	int error_floating = 0;
	int error_highlever = 0;
	int pulldown_read = 1;
	int pullup_read = 1;
	char buf[50] = {'\0'};

	gpio_cmds_tx(&lcd_gpio_request_btb, 1);

	btb_vir_addr = (void *)ioremap_wc(btb_info_pr->btb_con_addr, sizeof(btb_info_pr->btb_con_addr));	/* IO config address remap */
	if (btb_vir_addr == NULL) {
		HISI_FB_ERR("btb_con_addr ioremap error !\n");
		return CHECK_SKIPPED;
	}

	btb_pull_data = readl(btb_vir_addr);		/* config pull down and read */
	if((btb_pull_data & 0x3) != PULLDOWN) {
		btb_pull_data = (btb_pull_data & 0xFFFFFFFC) | (PULLDOWN & 0x3);
		writel(btb_pull_data, btb_vir_addr);
		mdelay(1);
	}
	pulldown_read = gpio_get_val(&lcd_gpio_read_btb);

	btb_pull_data = readl(btb_vir_addr);		/* config pull up and read */
	if((btb_pull_data & 0x3) != PULLUP) {
		btb_pull_data = (btb_pull_data & 0xFFFFFFFC) | (PULLUP & 0x3);
		writel(btb_pull_data, btb_vir_addr);
		mdelay(1);
	}
	pullup_read = gpio_get_val(&lcd_gpio_read_btb);

	gpio_cmds_tx(&lcd_gpio_free_btb, 1);
	iounmap(btb_vir_addr);		/* IO config address unmap */

	if(pulldown_read != pullup_read) {
		error_floating = 1;	/*make error flag*/
		if (error_floating_times < MAX_REPPORT_TIMES+1){	/*Device Radar error report is limited to 5, too many error records are not necessary*/
			error_floating_times++;
		}
		memcpy(buf, "LCD not connected!\n", sizeof("LCD not connected!\n"));
		HISI_FB_ERR("btb is floating status, LCD not connected!\n");
		goto check_error;
	} else if(pulldown_read == BTB_STATE_HIGH_LEVER || pullup_read == BTB_STATE_HIGH_LEVER) {
		error_highlever = 1;	/*make error flag*/
		if (error_highlever_times < MAX_REPPORT_TIMES+1){
			error_highlever_times++;
		}
		memcpy(buf, "LCD is connected error!\n", sizeof("LCD is connected error!\n"));
		HISI_FB_ERR("btb is high-lever status, LCD is connected error!\n");
		goto check_error;
	} else {
		return CHECKED_OK;
	}

check_error:
	if ((error_floating == 1 && error_floating_times <= MAX_REPPORT_TIMES)
		|| (error_highlever == 1 && error_highlever_times <= MAX_REPPORT_TIMES)) {	/*when error times is more than 5, don't report anymore*/
		ret = dsm_client_ocuppy(lcd_dclient);
		if ( !ret ) {
			dsm_client_record(lcd_dclient, buf);
			dsm_client_notify(lcd_dclient, DSM_LCD_BTB_CHECK_ERROR_NO);
		}else{
			HISI_FB_ERR("dsm_client_ocuppy ERROR:retVal = %d\n", ret);
		}
	}
	return CHECKED_ERROR;
}

int mipi_lcd_btb_check(void)
{
	struct lcd_btb_info *btb_info_pr = &btb_info;

	if(btb_info_pr->get_status == GET_STATUS_FAILED){	/*get information failed*/
		return CHECK_SKIPPED;
	}

	if(btb_info_pr->btb_support == 0){	/*not support btb check*/
		HISI_FB_INFO("not support btb check\n");
		return CHECK_SKIPPED;
	}

	return btb_check_state();
}

void lcd_btb_init(char *dts_comp_lcd_name)
{
	int ret = 0;
	struct lcd_btb_info *btb_info_pr = &btb_info;
	struct device_node *np = NULL;

	if (dts_comp_lcd_name == NULL) {
		HISI_FB_ERR("dts_comp_lcd_name is NULL\n");
		goto init_error;
	}

	np = of_find_compatible_node(NULL, NULL, dts_comp_lcd_name);
	if (!np) {
		HISI_FB_ERR("NOT FOUND device node %s!\n", dts_comp_lcd_name);
		goto init_error;
	}

	ret = of_property_read_u32(np, "btb-test-support", &btb_info_pr->btb_support);
	if (ret) {
		HISI_FB_ERR("get btb-test-support failed!\n");
		goto init_error;
	}

	ret = of_property_read_u32(np, "btb-cfg-addr", &btb_info_pr->btb_con_addr);
	if (ret || btb_info_pr->btb_con_addr == 0) {
		HISI_FB_ERR("get btb-cfg-addr failed!\n");
		goto init_error;
	}

	btb_info_pr->get_status = GET_STATUS_DONE;	/*get information succeed*/
	return;

init_error:
	btb_info_pr->get_status = GET_STATUS_FAILED;	/*get information failed*/
	btb_info_pr->btb_con_addr = 0;
	btb_info_pr->btb_support = 0;
}

int lcd_check_mipi_fifo_empty(char __iomem *dsi_base)
{
    unsigned long dw_jiffies = 0;
    uint32_t pkg_status = 0;
    uint32_t phy_status = 0;
    int is_timeout = 1;

    /*read status register*/
    dw_jiffies = jiffies + HZ;
    do {
        pkg_status = inp32(dsi_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
        phy_status = inp32(dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		/*finishing cmd sending and buffer is empty*/
        if ((pkg_status & 0x1) == 0x1 && !(phy_status & 0x2)){
            is_timeout = 0;
            break;
        }
    } while (time_after(dw_jiffies, jiffies));

    if (is_timeout) {
        HISI_FB_ERR("mipi check empty fail: \n \
            MIPIDSI_CMD_PKT_STATUS = 0x%x \n \
            MIPIDSI_PHY_STATUS = 0x%x \n \
            MIPIDSI_INT_ST1_OFFSET = 0x%x \n",
            inp32(dsi_base + MIPIDSI_CMD_PKT_STATUS_OFFSET),
            inp32(dsi_base + MIPIDSI_PHY_STATUS_OFFSET),
            inp32(dsi_base + MIPIDSI_INT_ST1_OFFSET));
        return -1;
    }
    return 0;
}
void hw_lcd_get_on_time(void)
{
	do_gettimeofday(&lcd_init_done);
}

void hw_lcd_delay_on(int lcd_panel_on_delay_time, uint32_t bl_level )
{
	/*set init backlight value not 0*/
	static int last_bl_level = 255;
	int interval = 0;
	struct timeval bl_on;

	if (last_bl_level == 0 && bl_level != 0) {
		if (lcd_panel_on_delay_time > 0) {
			do_gettimeofday(&bl_on);
			/*tv_sec mean seconds ,tv_usec mean Micro seconds,we need transform to  milliseconds*/
			interval = (bl_on.tv_sec - lcd_init_done.tv_sec)*1000 + (bl_on.tv_usec - lcd_init_done.tv_usec)/1000;
			if (interval < lcd_panel_on_delay_time && interval > 0) {
				mdelay (lcd_panel_on_delay_time - interval);
			}
			HISI_FB_DEBUG("interval=%d\n", interval);
		}
	}
	last_bl_level = bl_level;
}

void hw_lcd_delay_off(int lcd_panel_on_delay_time)
{
	int interval = 0;
	struct timeval panel_off;

	if (lcd_panel_on_delay_time > 0) {
		do_gettimeofday(&panel_off);
		/*tv_sec mean seconds ,tv_usec mean Micro seconds,we need transform to  milliseconds*/
		interval = (panel_off.tv_sec - lcd_init_done.tv_sec)*1000 + (panel_off.tv_usec - lcd_init_done.tv_usec)/1000;
		if (interval < lcd_panel_on_delay_time && interval > 0) {
			mdelay (lcd_panel_on_delay_time - interval);
		}
		HISI_FB_DEBUG("interval=%d\n", interval);
	}
}