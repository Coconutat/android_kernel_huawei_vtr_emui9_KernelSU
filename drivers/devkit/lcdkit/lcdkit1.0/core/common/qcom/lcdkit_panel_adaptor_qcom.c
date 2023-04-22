#include "mdss_fb.h"
#include "lcdkit_panel.h"
#include "lcdkit_parse.h"
#include "lcdkit_dbg.h"
#include "mdss_dsi.h"
#include <linux/mdss_io_util.h>
extern int lcdkit_brightness_ddic_info;
extern struct device* lcd_dev;
extern struct msm_fb_data_type *get_mfd(struct device * dev);

void lcdkit_info_init(void* pdata)
{
    lcdkit_info.panel_infos.backlight_cmds.flags |= LCDKIT_CMD_CLK_CTRL;
    if (lcdkit_info.panel_infos.checksum_support)
        lcdkit_info.panel_infos.checksum_cmds.flags |= LCDKIT_CMD_REQ_RX;
    if (lcdkit_info.panel_infos.esd_support)
        lcdkit_info.panel_infos.esd_cmds.flags |= LCDKIT_CMD_REQ_RX;
    if (lcdkit_info.panel_infos.check_reg_support)
        lcdkit_info.panel_infos.check_reg_cmds.flags |= LCDKIT_CMD_REQ_RX;
    if (lcdkit_info.panel_infos.mipi_detect_support)
        lcdkit_info.panel_infos.mipi_detect_cmds.flags |= LCDKIT_CMD_REQ_RX;
    if (lcdkit_info.panel_infos.get_blmaxnit_type == GET_BLMAXNIT_FROM_DDIC)
        lcdkit_info.panel_infos.bl_maxnit_cmds.flags |= LCDKIT_CMD_REQ_RX;
}

void lcdkit_dsi_cmd_trans(struct dsi_panel_cmds *dsi_cmds,
        u32 *flags, struct lcdkit_dsi_panel_cmds *cmds)
{
    int i = 0;

    dsi_cmds->blen = cmds->blen;
    dsi_cmds->buf = cmds->buf;
    dsi_cmds->cmd_cnt = cmds->cmd_cnt;
    dsi_cmds->link_state = cmds->link_state;

    for (i = 0; i < cmds->cmd_cnt; i++)
    {
        dsi_cmds->cmds[i].dchdr.vc    = cmds->cmds[i].vc;
        dsi_cmds->cmds[i].dchdr.ack   = cmds->cmds[i].ack;
        dsi_cmds->cmds[i].dchdr.wait  = cmds->cmds[i].wait;
        dsi_cmds->cmds[i].dchdr.dlen  = cmds->cmds[i].dlen;
        dsi_cmds->cmds[i].dchdr.last  = cmds->cmds[i].last;
        dsi_cmds->cmds[i].dchdr.dtype = cmds->cmds[i].dtype;
        dsi_cmds->cmds[i].payload     = cmds->cmds[i].payload;

        //qcom no "waittype", discard "waittype" here!
        //dsi_cmds->cmds[i].dchdr.waittype = cmds.cmds[i].waittype;
    }

    if (flags)
        *flags = cmds->flags;

}

/*
     dsi send cmds
*/
//mdss_dsi_panel_cmds_send
void lcdkit_dsi_tx(void *pdata, struct lcdkit_dsi_panel_cmds *cmds)
{
    u32 flags;
    size_t size;
    struct dcs_cmd_req cmdreq;
	struct mdss_panel_info *pinfo;
    struct dsi_panel_cmds dsi_cmds;
    struct mdss_dsi_ctrl_pdata *ctrl = pdata;

    if (!cmds->cmd_cnt)
    {
        return;
    }

    lcdkit_dump_cmds(cmds);

	pinfo = &(ctrl->panel_data.panel_info);
	if (pinfo->dcs_cmd_by_left) {
		if (ctrl->ndx != DSI_CTRL_LEFT)
			return;
	}

    size = cmds->cmd_cnt * sizeof(struct dsi_cmd_desc);
    dsi_cmds.cmds = kzalloc(size, GFP_KERNEL);
    lcdkit_dsi_cmd_trans(&dsi_cmds, &flags, cmds);

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = dsi_cmds.cmds;
	cmdreq.cmds_cnt = dsi_cmds.cmd_cnt;
	cmdreq.flags = flags;//CMD_REQ_COMMIT;

	if (dsi_cmds.link_state == DSI_LP_MODE)
		cmdreq.flags  |= CMD_REQ_LP_MODE;
	else if (dsi_cmds.link_state == DSI_HS_MODE)
		cmdreq.flags |= CMD_REQ_HS_MODE;

	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	cmdreq.rbuf = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);

    kfree(dsi_cmds.cmds);
}

/*¶Á½Ó¿Ú*/
//mdss_dsi_panel_cmds_send
int lcdkit_dsi_rx(void *pdata, uint32_t *rbuf,
                int len, struct lcdkit_dsi_panel_cmds *cmds)
{
    u32 flags, i;
    size_t size;
	struct dcs_cmd_req cmdreq;
	struct mdss_panel_info *pinfo;
    struct dsi_panel_cmds dsi_cmds;
    struct mdss_dsi_ctrl_pdata *ctrl = pdata;

    if (!cmds->cmd_cnt)
    {
        return -1;
    }

    lcdkit_dump_cmds(cmds);

	pinfo = &(ctrl->panel_data.panel_info);
	if (pinfo->dcs_cmd_by_left) {
		if (ctrl->ndx != DSI_CTRL_LEFT)
			return -1;
	}

    size = cmds->cmd_cnt * sizeof(struct dsi_cmd_desc);
    dsi_cmds.cmds = kzalloc(size, GFP_KERNEL);
    lcdkit_dsi_cmd_trans(&dsi_cmds, &flags, cmds);

    memset(&cmdreq, 0, sizeof(cmdreq));
    cmdreq.cmds_cnt = 1;
    cmdreq.flags = flags;
    cmdreq.cb = NULL; /* call back */
    cmdreq.rlen = len;

    if (dsi_cmds.link_state == DSI_LP_MODE)
        cmdreq.flags  |= CMD_REQ_LP_MODE;
    else if (dsi_cmds.link_state == DSI_HS_MODE)
        cmdreq.flags |= CMD_REQ_HS_MODE;
    memset(rbuf, 0, dsi_cmds.cmd_cnt * len * sizeof(uint32_t));
    for (i = 0; i < dsi_cmds.cmd_cnt; i++)
    {
    	cmdreq.cmds = dsi_cmds.cmds + i;
        //big & little endian is different, may be not work here if big endian!
    	cmdreq.rbuf = (char*)rbuf + i * len * sizeof(uint32_t);
    	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
    }

    kfree(dsi_cmds.cmds);
    return 0;
}


#if 0
//mdss_dsi_read_status
int lcdkit_dsi_read_status(struct mdss_dsi_ctrl_pdata *ctrl)
{
    uint32_t i;
    uint32_t *readbuf = kzalloc(ctrl->status_cmds_rlen, GFP_KERNEL);
    if (!readbuf)
    {
        LCDKIT_ERR("alloc memory failed!\n");
		return -ENOMEM;
    }

    lcdkit_dsi_rx(ctrl, readbuf, ctrl->status_cmds_rlen, &ctrl->status_cmds);

    for (i = 0; i < ctrl->status_cmds_rlen; i++)
        ctrl->status_buf.data[i] = readbuf[i];

    return 0;
}
#endif

u32 lcdkit_panel_cmd_read(void *ctrl,
                char cmd0, char cmd1, uint32_t *rbuf, int len)
{
    char cmd_payload[2] = {0x54, 0x00};

    struct lcdkit_dsi_cmd_desc cmd_desc = {
	    DTYPE_DCS_READ, 1, 0, 1, 5, 0, sizeof(cmd_payload), cmd_payload
    };

    struct lcdkit_dsi_panel_cmds panelreadcmds = {
        NULL, 0, &cmd_desc, 1, LCDKIT_DSI_LP_MODE,
                            LCDKIT_CMD_REQ_RX | LCDKIT_CMD_REQ_COMMIT
    };

	cmd_payload[0] = cmd0;
	cmd_payload[1] = cmd1;

    lcdkit_dsi_rx(ctrl, rbuf, len, &panelreadcmds);

	return 0;
}

/*switch lp to hs or hs to lp*/
void lcdkit_switch_hs_lp(void *pdata, bool enable)
{
    //for adpator, do nothing here!
    return;
}

void lcdkit_mipi_dsi_max_return_packet_size(void* pdata,
        struct lcdkit_dsi_cmd_desc* cm)
{
    return;
}

void dfr_ctrl(struct platform_device* pdev, bool enable)
{
    return;
}

void *lcdkit_get_dsi_panel_pdata(void);
bool lcdkit_is_cmd_panel(void)
{
   	struct mdss_panel_info *panel_info = lcdkit_get_dsi_panel_pdata();
    return panel_info->mipi.mode == DSI_CMD_MODE ? true : false;
}

void lcdkit_updt_porch(struct platform_device* pdev, int scence)
{
    return;
}

void lcdkit_effect_switch_ctrl(void* pdata, bool ctrl)
{
    return;
}

int adc_get_value(int channel)
{
    return 0;
}

int lcdkit_fake_update_bl(void *pdata, uint32_t bl_level)
{
    return 0;
}

ssize_t lcdkit_jdi_nt35696_5p5_gram_check_show(void* pdata, char* buf)
{
    return 0;
}

ssize_t lcdkit_jdi_nt35696_5p5_reg_read_show(void* pdata, char* buf)
{
    return 0;
}

void lcdkit_fps_scence_adaptor_handle(struct platform_device* pdev, uint32_t scence)
{
    return;
}
void lcdkit_fps_updt_adaptor_handle(void* pdata)
{
    return;
}
int lcdkit_lread_reg(void *pdata, uint32_t *out, struct lcdkit_dsi_cmd_desc* cmds, uint32_t len)
{
    return 0;
}

void lcdkit_fps_adaptor_ts_callback(void)
{
    return ;
}
void lcdkit_fps_timer_adaptor_init(void)
{
    return ;
}
ssize_t host_panel_oem_info_show(void* pdata, char *buf)
{
	return 0;
}

ssize_t host_panel_oem_info_store(void* pdata, char *buf)
{
	return 0;
}

/*for lcd btb check*/

/* lcdkit_gpio_cmds_tx - tx gpio command (GPIO_REQUEST/GPIO_READ/GPIO_FREE)
*
* btb_gpio	: gpio number to be operated
* gpio_optype	: gpio operate type (0:GPIO_REQUEST  1:GPIO_READ  2:GPIO_FREE)
* return		: when GPIO_READ, return GPIO_READ value, other type means nothing.
*/
int lcdkit_gpio_cmds_tx(unsigned int btb_gpio, int gpio_optype)
{
	return 0;
}

/* lcdkit_gpio_pulldown - set gpio pull-down
*
* btb_vir_addr: address needed to set gpio pull-down
* return		: 1-success  0-failure
*/
int lcdkit_gpio_pulldown(void * btb_vir_addr)
{
	return 1;
}

/* lcdkit_gpio_pullup - set gpio pull-up
*
* btb_vir_addr: address needed to set gpio pull-up
* return		: 1-success  0-failure
*/
int lcdkit_gpio_pullup(void * btb_vir_addr)
{
	return 1;
}
ssize_t lcdkit_set_bl_enhance_mode_reg(void* pdata)
{
    return 0;
}
ssize_t lcdkit_set_bl_normal_mode_reg(void* pdata)
{
    return 0;
}
void lcdkit_update_lcd_brightness_info(void)
{
	int read_count = 0;
	void *ctrl_pdata = lcdkit_get_dsi_ctrl_pdata();
	struct msm_fb_data_type *mdp = NULL;

	if (lcd_dev == NULL) {
		LCDKIT_INFO("lcd_dev is NULL\n");
		return;
	}

	mdp = get_mfd(lcd_dev);
	while ((lcdkit_info.panel_infos.get_blmaxnit_type == GET_BLMAXNIT_FROM_DDIC) && mdss_fb_is_power_on(mdp) && (lcdkit_brightness_ddic_info == 0) && (read_count < 3)) {
		lcdkit_dsi_tx(ctrl_pdata, &lcdkit_info.panel_infos.bl_befreadconfig_cmds);
		lcdkit_dsi_rx(ctrl_pdata, &lcdkit_brightness_ddic_info, 1, &lcdkit_info.panel_infos.bl_maxnit_cmds);
		lcdkit_dsi_tx(ctrl_pdata, &lcdkit_info.panel_infos.bl_aftreadconfig_cmds);
		read_count++;
	}

	if (read_count >= 3) {
		LCDKIT_INFO("lcdkit_brightness_ddic_info = %d, read_count = %d\n", lcdkit_brightness_ddic_info, read_count);
	}

	return;
}

ssize_t lcdkit_get_bl_resume_timmer(void* pdata, int *buf)
{
    return 0;
}