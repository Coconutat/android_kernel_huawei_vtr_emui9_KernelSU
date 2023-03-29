#include "hisi_fb.h"
#include "lcdkit_panel.h"
#include "lcdkit_dbg.h"
#include "lcdkit_disp.h"
#include <huawei_ts_kit.h>

/* NT36860_PAGE20_GAMMA_REG_COUNT means the number of the gamma paramerter in cmd2 page20 for NT36860.*/
#define NT36860_PAGE20_GAMMA_REG_COUNT 19

/*checksum*/
extern uint32_t checksum_start;
ssize_t lcdkit_jdi_nt35696_5p5_gram_check_show(void* pdata, char* buf)
{

    ssize_t ret = 0;
    struct hisi_fb_data_type *hisifd = NULL;
    char __iomem *mipi_dsi0_base = NULL;
    uint32_t rd1[LCDKIT_CHECKSUM_SIZE] = {0};
    uint32_t rd2[LCDKIT_CHECKSUM_SIZE] = {0};
    int i = 0;
    char cmd3_enable[] = {0xFF, 0xF0};
    char crc_dc_select[] = {0xEA, 0x00};
    char crc_mipi_select1[] = {0x7B, 0x00};
    char crc_mipi_select2[] = {0x7B, 0x20};
    char checksum_read[] = {0x73};
    //0:success; 1:fail
    int checksum_result = 0;
    struct dsi_cmd_desc packet_size_set_cmd = {DTYPE_MAX_PKTSIZE, 0, 10, WAIT_TYPE_US, 1, NULL};
    struct dsi_cmd_desc lcd_cmd3_enable_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 50, WAIT_TYPE_MS,
            sizeof(cmd3_enable), cmd3_enable},
        {DTYPE_DCS_WRITE1, 0, 50, WAIT_TYPE_MS,
            sizeof(crc_dc_select), crc_dc_select},
    };
    struct dsi_cmd_desc lcd_crc_mipi_select1_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_MS,
            sizeof(crc_mipi_select1), crc_mipi_select1},
    };
    struct dsi_cmd_desc lcd_crc_mipi_select2_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_MS,
            sizeof(crc_mipi_select2), crc_mipi_select2},
    };
    struct dsi_cmd_desc lcd_check_reg[] = {
        {DTYPE_DCS_READ, 0, 20, WAIT_TYPE_US,
            sizeof(checksum_read), checksum_read},
    };

    static uint32_t expected_checksum1[3][8] = {
        {0xb9, 0x8e, 0x45, 0xb8, 0x05, 0xaf, 0x57, 0x90}, //runningtest1
        {0xe1, 0x1d, 0x11, 0xae, 0x04, 0x27, 0xad, 0xcc}, //runningtest2
        {0xbf, 0xbe, 0x33, 0xaa, 0x41, 0x6b, 0x7d, 0xd1}, //runningtest3
    };

    static uint32_t expected_checksum2[3][8] = {
        {0xd8, 0xe0, 0x3b, 0x22, 0xf7, 0xb9, 0x9d, 0x7d}, //runningtest1
        {0x6a, 0x97, 0xea, 0x9a, 0xa7, 0x33, 0x09, 0x6a}, //runningtest2
        {0x2a, 0x0d, 0x3f, 0x4c, 0x18, 0xd1, 0x1f, 0x71}, //runningtest3
    };

    hisifd = (struct hisi_fb_data_type*) pdata;

    mipi_dsi0_base = hisifd->mipi_dsi0_base;

    mipi_dsi_max_return_packet_size(&packet_size_set_cmd, mipi_dsi0_base);
    mipi_dsi_cmds_tx(lcd_cmd3_enable_cmds, \
        ARRAY_SIZE(lcd_cmd3_enable_cmds), mipi_dsi0_base);
    mipi_dsi_cmds_tx(lcd_crc_mipi_select1_cmds, \
        ARRAY_SIZE(lcd_crc_mipi_select1_cmds), mipi_dsi0_base);
    for (i = 0; i < LCDKIT_CHECKSUM_SIZE; i++) {
        char *data = lcd_check_reg[0].payload;
        *data = 0x73 + i;
        mipi_dsi_cmds_rx((rd1 + i), lcd_check_reg, \
            ARRAY_SIZE(lcd_check_reg), mipi_dsi0_base);

        if (rd1[i] != expected_checksum1[lcdkit_info.panel_infos.checksum_pic_num][i]) {
            checksum_result++;
        }
    }
    LCDKIT_INFO("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x, pic_num=%d\n", \
            rd1[0], rd1[1], rd1[2], rd1[3], rd1[4], rd1[5], rd1[6], rd1[7], lcdkit_info.panel_infos.checksum_pic_num);

    mipi_dsi_cmds_tx(lcd_crc_mipi_select2_cmds, \
        ARRAY_SIZE(lcd_crc_mipi_select2_cmds), mipi_dsi0_base);
    for (i = 0; i < LCDKIT_CHECKSUM_SIZE; i++) {
        char *data = lcd_check_reg[0].payload;
        *data = 0x73 + i;
        mipi_dsi_cmds_rx((rd2 + i), lcd_check_reg, \
            ARRAY_SIZE(lcd_check_reg), mipi_dsi0_base);

        if (rd2[i] != expected_checksum2[lcdkit_info.panel_infos.checksum_pic_num][i]) {
            checksum_result++;
        }
    }
    LCDKIT_INFO("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x, pic_num=%d\n", \
            rd2[0], rd2[1], rd2[2], rd2[3], rd2[4], rd2[5], rd2[6], rd2[7], lcdkit_info.panel_infos.checksum_pic_num);

    if (checksum_result){
        LCDKIT_ERR("checksum_result:%d\n", checksum_result);
        checksum_result = 1;
    }

    ret = snprintf(buf, PAGE_SIZE, "%d\n", checksum_result);

    if (checksum_result && (2 == lcdkit_info.panel_infos.checksum_pic_num)) {
        checksum_start = LCDKIT_CHECKSUM_END;
    }

    if (LCDKIT_CHECKSUM_END == checksum_start) {
        lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.checksum_disable_cmds);
        lcdkit_effect_switch_ctrl(pdata, 0);
        LCDKIT_INFO("gram checksum end, disable checksum.\n");
    }
    LCDKIT_DEBUG("fb%d, -.\n", hisifd->index);

    return ret;
}

uint32_t g_page20_gamma_buf[LCD_REG_NUM][LCD_REG_LENGTH_MAX] = {0};
uint32_t g_page21_gamma_buf[LCD_REG_NUM][LCD_REG_LENGTH_MAX] = {0};
ssize_t lcdkit_jdi_nt35696_5p5_reg_read_show(void* pdata, char* buf)
{
    char lcd_reg_buf[LCD_REG_LENGTH_MAX] = {0};
    char str_tmp[LCD_REG_LENGTH_MAX] = {0};
    uint32_t *read_value = NULL;
    int i = 0;
    int ret = 0;
    int reg = 0;
    if (lcdkit_info.panel_infos.gama_cmd_page == 0x20)//page 20
    {
        for (i = 0; i < lcdkit_info.panel_infos.gama_correct_reg.cnt; i++)
        {
            if (lcdkit_info.panel_infos.gama_reg_addr == lcdkit_info.panel_infos.gama_correct_reg.buf[i])
                break;
        }
        reg = i;
        read_value = g_page20_gamma_buf[reg];
    }
    else if (lcdkit_info.panel_infos.gama_cmd_page == 0x21)
    {
        for (i = 0; i < lcdkit_info.panel_infos.gama_correct_reg.cnt; i++) {
            if (lcdkit_info.panel_infos.gama_reg_addr == lcdkit_info.panel_infos.gama_correct_reg.buf[i])
                break;
        }
        reg = i;
        read_value = g_page21_gamma_buf[reg];
    }

    if (read_value == NULL) {
        LCDKIT_ERR("read_value = NULL\n");
        ret = -1;
        return ret;
    }
    snprintf(lcd_reg_buf, sizeof(lcd_reg_buf), "1,");
    for (i = 0; i < (int)lcdkit_info.panel_infos.gama_reg_length; i++) {
        switch (i % 4) {
        case 0:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", read_value[i / 4] & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", read_value[i / 4] & 0xFF);
            }
            break;
        case 1:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", (read_value[i / 4] >> 8) & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", (read_value[i / 4] >> 8) & 0xFF);
            }
            break;
        case 2:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", (read_value[i / 4] >> 16) & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", (read_value[i / 4] >> 16) & 0xFF);
            }
            break;
        case 3:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", (read_value[i / 4] >> 24) & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", (read_value[i / 4] >> 24) & 0xFF);
            }
            break;
        default:
            break;
        }
        strncat(lcd_reg_buf, str_tmp, strlen(str_tmp));
    }
    LCDKIT_INFO("%s\n", lcd_reg_buf);
    ret = snprintf(buf, sizeof(lcd_reg_buf), "%s\n", lcd_reg_buf);
    return ret;
}

ssize_t lcdkit_jdi_nt36860_5p88_reg_read_show(void* pdata, char* buf)
{
    char lcd_reg_buf[LCD_REG_LENGTH_MAX] = {0};
    char str_tmp[LCD_REG_LENGTH_MAX] = {0};
    uint32_t *read_value = NULL;
    int i = 0;
    int ret = 0;
    int reg = 0;
    if (lcdkit_info.panel_infos.gama_cmd_page == 0x20)
    {
        for (i = 0; i < NT36860_PAGE20_GAMMA_REG_COUNT; i++)
        {
            if (lcdkit_info.panel_infos.gama_reg_addr == lcdkit_info.panel_infos.gama_correct_reg.buf[i])
                break;
        }
        reg = i;
        read_value = g_page20_gamma_buf[reg];
    }
    else if (lcdkit_info.panel_infos.gama_cmd_page == 0x21)
    {
        for (i = 0; i < lcdkit_info.panel_infos.gama_correct_reg.cnt - NT36860_PAGE20_GAMMA_REG_COUNT; i++) {
            if (lcdkit_info.panel_infos.gama_reg_addr == lcdkit_info.panel_infos.gama_correct_reg.buf[i + NT36860_PAGE20_GAMMA_REG_COUNT])
                break;
        }
        reg = i;
        read_value = g_page21_gamma_buf[reg];
    }

    if (read_value == NULL) {
        LCDKIT_ERR("read_value = NULL\n");
        ret = -1;
        return ret;
    }
    snprintf(lcd_reg_buf, sizeof(lcd_reg_buf), "1,");
    for (i = 0; i < (int)lcdkit_info.panel_infos.gama_reg_length; i++) {
        switch (i % 4) {
        case 0:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", read_value[i / 4] & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", read_value[i / 4] & 0xFF);
            }
            break;
        case 1:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", (read_value[i / 4] >> 8) & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", (read_value[i / 4] >> 8) & 0xFF);
            }
            break;
        case 2:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", (read_value[i / 4] >> 16) & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", (read_value[i / 4] >> 16) & 0xFF);
            }
            break;
        case 3:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", (read_value[i / 4] >> 24) & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", (read_value[i / 4] >> 24) & 0xFF);
            }
            break;
        default:
            break;
        }
        strncat(lcd_reg_buf, str_tmp, strlen(str_tmp));
    }
    LCDKIT_INFO("%s\n", lcd_reg_buf);
    ret = snprintf(buf, sizeof(lcd_reg_buf), "%s\n", lcd_reg_buf);
    return ret;
}

int lcdkit_jdi_nt35696_5p5_gamma_reg_read(struct hisi_fb_data_type* hisifd)
{
    int len = 16;
    int ret = 0;
    int i = 0;
/*lint -save -e569 */
    char lcd_reg[] = {0xb0};
    char page20[] = {0xff, 0x20};
    char page21[] = {0xff, 0x21};
    char page10[] = {0xff, 0x10};
/*lint -restore */
    static int read_flag = 0;
    struct dsi_cmd_desc lcd_reg_cmd[] = {
        {DTYPE_GEN_READ1, 0, 10, WAIT_TYPE_US,
            sizeof(lcd_reg), lcd_reg},
    };
    struct dsi_cmd_desc lcd_page20_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_MS,
            sizeof(page20), page20},
    };
    struct dsi_cmd_desc lcd_page21_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_MS,
            sizeof(page21), page21},
    };
    struct dsi_cmd_desc lcd_page10_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_MS,
            sizeof(page10), page10},
    };

    if (read_flag) {
        LCDKIT_INFO("gamma reg have read!\n");
        return ret;
    }

    read_flag = 1;
    /*switch page20*/
    mipi_dsi_cmds_tx(lcd_page20_cmds, \
        ARRAY_SIZE(lcd_page20_cmds), hisifd->mipi_dsi0_base);
    /*read gamma reg*/
    for (i = 0; i < lcdkit_info.panel_infos.gama_correct_reg.cnt; i++)
    {
        lcd_reg[0] = lcdkit_info.panel_infos.gama_correct_reg.buf[i];
        len = lcdkit_info.panel_infos.gama_reg_len.buf[i];
        ret = mipi_dsi_lread_reg(g_page20_gamma_buf[i], lcd_reg_cmd, len, hisifd->mipi_dsi0_base);
        if (ret) {
            LCDKIT_INFO("read error, ret=%d\n", ret);
            return ret;
        }
    }
    /*switch page21*/
    mipi_dsi_cmds_tx(lcd_page21_cmds, \
        ARRAY_SIZE(lcd_page21_cmds), hisifd->mipi_dsi0_base);
    /*read gamma reg*/
    for (i = 0; i < lcdkit_info.panel_infos.gama_correct_reg.cnt; i++)
    {
        lcd_reg[0] = lcdkit_info.panel_infos.gama_correct_reg.buf[i];
        len = lcdkit_info.panel_infos.gama_reg_len.buf[i];
        ret = mipi_dsi_lread_reg(g_page21_gamma_buf[i], lcd_reg_cmd, len, hisifd->mipi_dsi0_base);
        if (ret) {
            LCDKIT_INFO("read error, ret=%d\n", ret);
            return ret;
        }
    }
    /*switch page10*/
    mipi_dsi_cmds_tx(lcd_page10_cmds, \
        ARRAY_SIZE(lcd_page10_cmds), hisifd->mipi_dsi0_base);
    return ret;
}

int lcdkit_jdi_nt36860_5p88_gamma_reg_read(struct hisi_fb_data_type* hisifd)
{
    int len = 16;
    int ret = 0;
    int i = 0;
/*lint -save -e569 */
    char lcd_reg[] = {0xb0};
    char page20[] = {0xff, 0x20};
    char page21[] = {0xff, 0x21};
    char page10[] = {0xff, 0x10};
/*lint -restore */
    static int read_flag = 0;
    struct dsi_cmd_desc lcd_reg_cmd[] = {
        {DTYPE_GEN_READ1, 0, 10, WAIT_TYPE_US,
            sizeof(lcd_reg), lcd_reg},
    };
    struct dsi_cmd_desc lcd_page20_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_MS,
            sizeof(page20), page20},
    };
    struct dsi_cmd_desc lcd_page21_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_MS,
            sizeof(page21), page21},
    };
    struct dsi_cmd_desc lcd_page10_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_MS,
            sizeof(page10), page10},
    };

    if (read_flag) {
        LCDKIT_INFO("gamma reg have read!\n");
        return ret;
    }

    read_flag = 1;
    /*switch to page20*/
    mipi_dsi_cmds_tx(lcd_page20_cmds, \
        ARRAY_SIZE(lcd_page20_cmds), hisifd->mipi_dsi0_base);
    /*read gamma reg*/
    for (i = 0; i < NT36860_PAGE20_GAMMA_REG_COUNT; i++)
    {
        lcd_reg[0] = lcdkit_info.panel_infos.gama_correct_reg.buf[i];
        len = lcdkit_info.panel_infos.gama_reg_len.buf[i];
        ret = mipi_dsi_lread_reg(g_page20_gamma_buf[i], lcd_reg_cmd, len, hisifd->mipi_dsi0_base);
        if (ret) {
            LCDKIT_INFO("read error, ret=%d\n", ret);
            return ret;
        }
    }
    /*switch to page21*/
    mipi_dsi_cmds_tx(lcd_page21_cmds, \
        ARRAY_SIZE(lcd_page21_cmds), hisifd->mipi_dsi0_base);
    /*read gamma reg*/
    for (i = NT36860_PAGE20_GAMMA_REG_COUNT; i < lcdkit_info.panel_infos.gama_correct_reg.cnt; i++)
    {
        lcd_reg[0] = lcdkit_info.panel_infos.gama_correct_reg.buf[i];
        len = lcdkit_info.panel_infos.gama_reg_len.buf[i];
        ret = mipi_dsi_lread_reg(g_page21_gamma_buf[i - NT36860_PAGE20_GAMMA_REG_COUNT], lcd_reg_cmd, len, hisifd->mipi_dsi0_base);
        if (ret) {
            LCDKIT_INFO("read error, ret=%d\n", ret);
            return ret;
        }
    }
    /*switch page10*/
    mipi_dsi_cmds_tx(lcd_page10_cmds, \
        ARRAY_SIZE(lcd_page10_cmds), hisifd->mipi_dsi0_base);
    return ret;
}
