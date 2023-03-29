#include "lcdkit_panel.h"
#include "lcdkit_dbg.h"
#include "lcdkit_parse.h"

#define LCDKIT_READ_REG_WAIT_RESPONSE_TIME 10
#define INT_COUNT 4
#define REG_BIT_COUNT 8

struct lcdkit_debug lcdkit_dbg;
int lcdkit_msg_level = 7;
atomic_t mipi_path_status = ATOMIC_INIT(1);

lcdkit_dbg_cmds lcdkit_cmd_list[] =
{
    {LCDKIT_DBG_LEVEL_SET,                      "set_debug_level"            },
    {LCDKIT_DBG_MIPI_CLK,                       "set_mipi_clock"             },
    {LCDKIT_DBG_INIT_CODE,                      "set_init_param"             },
    {LCDKIT_DBG_PANEL_VSP_VSN,                  "set_panel_vsp_vsn"          },
    {LCDKIT_DBG_ESD_ENABLE,                     "lcd_esd_debug_enable"       },
    {LCDKIT_DBG_ESD_RECOVER_TEST,               "lcd_esd_recover_test"       },
    {LCDKIT_DBG_ESD_RESET,                      "lcd_esd_debug_reset"        },
    {LCDKIT_DBG_ESD_BL_ENABLE,                  "lcd_esd_debug_bl_enable"    },
    {LCDKIT_DBG_ESD_BL_SET,                     "lcd_esd_debug_bl_set"       },
    {LCDKIT_DBG_ESD_CHECK_REG,                  "lcd_esd_debug_check_reg"    },
    {LCDKIT_DBG_ESD_CHECK_VALUE,                "lcd_esd_debug_check_value"  },
};

static char lcdkit_hex_char_to_value(char ch)
{
    switch (ch)
    {
        case 'a' ... 'f':
            ch = 10 + (ch - 'a');
            break;

        case 'A' ... 'F':
            ch = 10 + (ch - 'A');
            break;

        case '0' ... '9':
            ch = ch - '0';
            break;
    }

    return ch;
}

static int lcdkit_fget_dtsi_style(unsigned char* buf, int max_num, int fd, off_t* fd_seek)
{
    int cur_seek = *fd_seek;
    unsigned char ch = '\0';
    unsigned char last_char = 'Z';
    int j = 0;

    sys_lseek(fd, (off_t)0, 0);

    while (j < max_num)
    {
        if ((unsigned)sys_read(fd, &ch, 1) != 1)
        {
            LCDKIT_DEBUG("it's end of file %d : len = %d\n",  __LINE__, j);
            return j;
        }
        else
        {
            if (!LCDKIT_IS_VALID_CHAR(ch))
            {
                last_char = 'Z';
                cur_seek++;
                sys_lseek(fd, (off_t)cur_seek, 0);
                continue;
            }

            if (last_char != 'Z')
            {
                /*two char value is possible like F0, so make it a single char*/
                --j;
                buf[j] = (buf[j] * LCDKIT_HEX_BASE) + lcdkit_hex_char_to_value(ch);
                last_char = 'Z';
            }
            else
            {
                buf[j] = lcdkit_hex_char_to_value(ch);
                last_char = buf[j];
            }

            j++;
            cur_seek++;
            sys_lseek(fd, (off_t)cur_seek, 0);
        }
    }

    if (j >= max_num)
    {
        LCDKIT_DEBUG("Buffer is not enough\n");
        j *= -1;
    }

    *fd_seek = cur_seek;
    return j;
}

static bool lcdkit_resolve_dtsi_config_file(int fd, void** para_table, uint32_t* para_num)
{
    off_t fd_seek = 0;
    int num = 0;
    unsigned char* lcd_config_table = NULL;
    lcd_config_table = kzalloc(LCDKIT_CONFIG_TABLE_MAX_NUM, 0);

    if (NULL == lcd_config_table)
    {
        goto kalloc_err;
    }

    sys_lseek(fd, (off_t)0, 0);

    num = lcdkit_fget_dtsi_style(lcd_config_table, LCDKIT_CONFIG_TABLE_MAX_NUM, fd, &fd_seek);

    if (num <= 0)
    {
        LCDKIT_ERR("read failed with error return:%d\n", num);
        goto debug_init_read_fail;
    }

    *para_num = num;
    *para_table = lcd_config_table;
    return TRUE;

debug_init_read_fail:
    kfree(lcd_config_table);
    //lcd_config_table = NULL;

kalloc_err:
    *para_table = NULL;
    *para_num = 0;
    return FALSE;
}

bool lcdkit_debug_malloc_dtsi_para(void** para_table, uint32_t* para_num)
{
    int ret = 0 ;
    int fd = 0 ;
    void* table_tmp = NULL;
    int num_tmp = 0 ;
    mm_segment_t fs;

    if (NULL == para_table)
    {
        return FALSE;
    }

    fs = get_fs();     /* save previous value */
    set_fs (get_ds()); /* use kernel limit */

    fd = sys_open((const char __force*) LCDKIT_INIT_TEST_PARAM, O_RDONLY, 0);

    if (fd < 0)
    {
        LCDKIT_ERR("%s file doesn't exsit\n", LCDKIT_INIT_TEST_PARAM);
        set_fs(fs);
        return FALSE;
    }

    LCDKIT_DEBUG( "Config file %s opened. \n", LCDKIT_INIT_TEST_PARAM);

    //resolve the config file
    ret = lcdkit_resolve_dtsi_config_file(fd, &table_tmp, &num_tmp);
    sys_close(fd);
    set_fs(fs);

    *para_table = table_tmp;
    *para_num = (uint32_t)num_tmp;

    if (FALSE == ret)
    {
        LCDKIT_ERR("failed to read the init code into memory\n");
        return FALSE;
    }

    *para_table = table_tmp;

    LCDKIT_DEBUG("init code is copied into memory\n");
    return TRUE;
}

void lcdkit_dump_buf(const char* buf, int cnt)
{
    int i;

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL\n");
        return;
    }
    //LCDKIT_DEBUG("================= dump buf start ===============\n");
    for (i = 0; i < cnt; i++)
    {
        LCDKIT_DEBUG("buf[%d]         = 0x%02x\n", i, buf[i]);
    }

    //LCDKIT_DEBUG("================= dump buf end   ===============\n");
}

void lcdkit_dump_buf_32(const u32* buf, int cnt)
{
    int i = 0;

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL\n");
        return;
    }

    for (i = 0; i < cnt; i++)
    {
        LCDKIT_DEBUG("buf[%d]         = 0x%02x\n", i, buf[i]);
    }

}

void lcdkit_dump_cmds_desc(struct lcdkit_dsi_cmd_desc* desc)
{
    if( NULL == desc)
    {
        LCDKIT_INFO("NULL point!\n");
        return ;
    }
    LCDKIT_DEBUG("dtype      = 0x%02x\n", desc->dtype);
    LCDKIT_DEBUG("last       = 0x%02x\n", desc->last);
    LCDKIT_DEBUG("vc         = 0x%02x\n", desc->vc);
    LCDKIT_DEBUG("ack        = 0x%02x\n", desc->ack);
    LCDKIT_DEBUG("wait       = 0x%02x\n", desc->wait);
    LCDKIT_DEBUG("waittype   = 0x%02x\n", desc->waittype);
    LCDKIT_DEBUG("dlen       = 0x%02x\n", desc->dlen);

    lcdkit_dump_buf(desc->payload, (int)(desc->dlen));
}

void lcdkit_dump_cmds(struct lcdkit_dsi_panel_cmds* cmds)
{
    int i;

    //LCDKIT_DEBUG("============= lcdkit cmds dump start ============\n");
    if( NULL == cmds)
    {
        LCDKIT_INFO("NULL point!\n");
        return ;
    }

    LCDKIT_DEBUG("blen       = 0x%02x\n", cmds->blen);
    LCDKIT_DEBUG("cmd_cnt    = 0x%02x\n", cmds->cmd_cnt);
    LCDKIT_DEBUG("link_state = 0x%02x\n", cmds->link_state);
    LCDKIT_DEBUG("flags      = 0x%02x\n", cmds->flags);

    lcdkit_dump_buf(cmds->buf, (int)(cmds->blen));

    for (i = 0; i < cmds->cmd_cnt; i++)
    {
        lcdkit_dump_cmds_desc(&cmds->cmds[i]);
    }

    //LCDKIT_DEBUG("============= lcdkit cmds dump end   ============\n");
}

void lcdkit_dump_array_data(struct lcdkit_array_data* array)
{
    //LCDKIT_DEBUG("============= array data dump start =============\n");
    if( NULL == array)
    {
        LCDKIT_INFO("NULL point!\n");
        return ;
    }
    LCDKIT_DEBUG("cnt        = 0x%x\n", array->cnt);
    lcdkit_dump_buf(array->buf, array->cnt);
    //LCDKIT_DEBUG("============= array data dump end   =============\n");
}

void lcdkit_dump_arrays_data(struct lcdkit_arrays_data* arrays)
{
    int i;

    if( NULL == arrays)
    {
        LCDKIT_ERR("NULL point!\n");
        return ;
    }

    //LCDKIT_DEBUG("============= arrays data dump start ============\n");
    LCDKIT_DEBUG("cnt        = 0x%x\n", arrays->cnt);

    for (i = 0; i < arrays->cnt; i++)
    { lcdkit_dump_array_data(&arrays->arry_data[i]); }

    //LCDKIT_DEBUG("============= arrays data dump end   ============\n");
}

int lcdkit_parse_dsi_cmds(struct lcdkit_dsi_panel_cmds* pcmds)
{
    int blen = 0, len = 0;
    char* buf = NULL, *bp = NULL;
    struct lcdkit_dsi_ctrl_hdr* dchdr;
    int i = 0, cnt = 0;

    memset(pcmds, 0, sizeof(struct lcdkit_dsi_panel_cmds));

    if (!lcdkit_debug_malloc_dtsi_para((void**)&buf, &blen))
    {
        LCDKIT_ERR("failed\n");
        return -ENOMEM;
    }

    /* scan dcs commands */
    bp = buf;
    len = blen;
    cnt = 0;

    while (len > sizeof(*dchdr))
    {
        dchdr = (struct lcdkit_dsi_ctrl_hdr*)bp;
        bp += sizeof(*dchdr);
        len -= sizeof(*dchdr);

        if (dchdr->dlen > len)
        {
            LCDKIT_ERR("dtsi cmd=%x error, len=%d, cnt=%d\n", dchdr->dtype, dchdr->dlen, cnt);
            return -ENOMEM;
        }

        bp += dchdr->dlen;
        len -= dchdr->dlen;
        cnt++;
    }

    if (len != 0)
    {
        LCDKIT_ERR("dcs_cmd=%x len=%d error!\n", buf[0], blen);
        kfree(buf);
        return -ENOMEM;
    }

    pcmds->cmds = kzalloc(cnt * sizeof(struct lcdkit_dsi_cmd_desc), GFP_KERNEL);

    if (!pcmds->cmds)
    {
        kfree(buf);
        return -ENOMEM;
    }

    pcmds->cmd_cnt = cnt;
    pcmds->buf = buf;
    pcmds->blen = blen;

    //need get value from input
    pcmds->link_state = LCDKIT_DSI_LP_MODE;
    pcmds->flags = LCDKIT_CMD_REQ_COMMIT;

    bp = buf;
    len = blen;

    for (i = 0; i < cnt; i++)
    {
        dchdr = (struct lcdkit_dsi_ctrl_hdr*)bp;
        len -= sizeof(*dchdr);
        bp += sizeof(*dchdr);
        pcmds->cmds[i].dtype = dchdr->dtype;
        pcmds->cmds[i].last = dchdr->last;
        pcmds->cmds[i].vc = dchdr->vc;
        pcmds->cmds[i].ack = dchdr->ack;
        pcmds->cmds[i].wait = dchdr->wait;
        pcmds->cmds[i].waittype = dchdr->waittype;
        pcmds->cmds[i].dlen = dchdr->dlen;
        pcmds->cmds[i].payload = bp;

        bp += dchdr->dlen;
        len -= dchdr->dlen;
    }

    lcdkit_dump_cmds(pcmds);

    return 0;
}

bool lcdkit_free_dsi_cmds(struct lcdkit_dsi_panel_cmds* pcmds)
{
    if (pcmds->cmds)
    {
        kfree(pcmds->cmds);
    }

    if (pcmds->buf)
    {
        kfree(pcmds->buf);
    }

    pcmds->cmds = NULL;
    pcmds->buf = NULL;
    LCDKIT_DEBUG("The new LCD config region has been freed\n");

    return TRUE;
}

static void lcdkit_dbg_set_mipiclk(int clk)
{
    lcdkit_dbg.lcdkit_g_mipiclk =     clk;
}

static void lcdkit_dbg_set_dbg_level(int level)
{
    lcdkit_msg_level = level;
}

static void lcdkit_dbg_enable_mipiclk(int enable)
{
    lcdkit_dbg.lcdkit_g_mipiclk_enable = enable;
}

static void lcdkit_dbg_enable_initcode(int enable)
{
    lcdkit_dbg.lcdkit_g_initcode_enable = enable;
}

static void lcdkit_dbg_enable_vsp_vsn(int enable)
{
    lcdkit_dbg.lcdkit_g_vsp_vsn_enable = enable;
}

int is_lcdkit_initcode_enable(void)
{
    return lcdkit_dbg.lcdkit_g_initcode_enable;
}

int is_lcdkit_mipiclk_enable(void)
{
    return lcdkit_dbg.lcdkit_g_mipiclk_enable;
}

int is_lcdkit_vsp_vsn_enable(void)
{
    return lcdkit_dbg.lcdkit_g_vsp_vsn_enable;
}

int get_lcdkit_mipiclk_dbg(void)
{
    return lcdkit_dbg.lcdkit_g_mipiclk;
}

/*for esd debug*/
int lcdkit_esd_debug(void* pdata)
{
    int ret = 0;
    uint32_t i = 0;
    uint32_t read_value = 0;

    /*check reg, read reg and compire the expect value*/
    for (i = 0; i < lcdkit_dbg.g_esd_debug.check_count; i++)
    {
        lcdkit_info.panel_infos.esd_dbg_cmds.cmds[0].payload = &lcdkit_dbg.g_esd_debug.esd_check_reg[i];
        lcdkit_dsi_rx(pdata, &read_value, 0, &lcdkit_info.panel_infos.esd_dbg_cmds);

        if (lcdkit_dbg.g_esd_debug.esd_check_reg[i] == 0x0e)
        {
            if (read_value & 0x80)
            {
                LCDKIT_DEBUG("Esd debug:Read reg:0x0e success, read value: %d\n", read_value);
            }
            else
            {
                LCDKIT_DEBUG("Esd debug:Read reg:0x0e failed, read value: %d\n", read_value);
                ret = 1;
                break;
            }
        }
        else
        {
            if (read_value != lcdkit_dbg.g_esd_debug.esd_reg_value[i])
            {
                LCDKIT_DEBUG("Esd debug:Read reg 0x%x does not match expect value,read value: 0x%x, expect value:0x%x\n", lcdkit_dbg.g_esd_debug.esd_check_reg[i], read_value, lcdkit_dbg.g_esd_debug.esd_reg_value[i]);
                ret = 1;
                break;
            }
            else
            {
                LCDKIT_DEBUG("Esd debug:Read 0x%x success, read value: 0x%x, expect value:0x%x\n", lcdkit_dbg.g_esd_debug.esd_check_reg[i], read_value, lcdkit_dbg.g_esd_debug.esd_reg_value[i]);
            }
        }
    }

    /*set backlight per 5s*/
    if (lcdkit_dbg.g_esd_debug.esd_bl_set)
    {

    }

    /*set bl enable per 5s*/
    if (lcdkit_dbg.g_esd_debug.esd_bl_enable)
    {

    }

    return ret;
}

/* convert string to lower case */
/* return: 0 - success, negative - fail */
static int lcdkit_str_to_lower(char* str)
{
    char* tmp = str;

    /* check param */
    if (NULL == tmp)
    {
        return -1;
    }

    while (*tmp != '\0')
    {
        *tmp = tolower(*tmp);
        tmp++;
    }

    return 0;
}

/* check if string start with sub string */
/* return: 0 - success, negative - fail */
static int lcdkit_str_start_with(char* str, char* sub)
{
    /* check param */
    if (NULL == str || NULL == sub)
    {
        return -EINVAL;
    }

    return (0 == strncmp(str, sub, strlen(sub)) ? 0 : -1);
}

/* open function */
static int lcdkit_dbg_open(struct inode* inode, struct file* file)
{
    /* non-seekable */
    file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
    return 0;
}

/* release function */
static int lcdkit_dbg_release(struct inode* inode, struct file* file)
{
    return 0;
}

/* read function */
/* show usage or print last read result */
static char lcdkit_debug_buf[2048];
static ssize_t lcdkit_dbg_read(struct file* file,  char __user* buff, size_t count, loff_t* ppos)
{
    int len = 0;
    int ret_len = 0;
    char* cur;
    int buf_len = sizeof(lcdkit_debug_buf);

    cur = lcdkit_debug_buf;

    if (*ppos)
    { return 0; }

    /* show usage */
    len = snprintf(cur, buf_len, "Usage: \n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"set_debug_level:8\" > lcdkit_dbg to open set debug level\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"set_init_param:1\" > lcdkit_dbg to open set init parameter\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"set_init_param:0\" > lcdkit_dbg to close set init parameter\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"set_mipi_clock:486\" > lcdkit_dbg to set mipi clock\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"lcd_esd_debug_enable:0/1\" > lcdkit_dbg to set mipi clock\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"set_panel_vsp_vsn:5400000\" > lcdkit_dbg to set vsp/vsn voltage\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"lcd_esd_debug_reset:1\" > lcdkit_dbg to reset esd debug\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"lcd_esd_debug_bl_enable:0/1\" > lcdkit_dbg to disable/enable write backlight enable\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"lcd_esd_debug_bl_set:0/1\" > lcdkit_dbg to disable/enable write backlight\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"lcd_esd_debug_check_reg:0x0a,0x0b,...\" > lcdkit_dbg to set check reg\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "\teg. echo \"lcd_esd_debug_check_value:0x9c,0x00,...\" > lcdkit_dbg to set reg value\n");
    buf_len -= len;
    cur += len;

    len = snprintf(cur, buf_len, "Usage: echo \"<[Dcs|Gen]>_<[write|read]>_1A_<[0..25]>P(reg, param0..paramN)_delayms\" > reg_dbg_mipi\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "Read or write lcd ic register using mipi interface.\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\teg. echo \"Dcs_read_1A_0P(0x52)_0\" > reg_dbg_mipi\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\teg. echo \"Dcs_write_1A_1P(0x51,0x00)_0\" > reg_dbg_mipi\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\teg. echo \"Gen_write_1A_3P(0xFF,0x80,0x09,0x01)_0\" > reg_dbg_mipi\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\teg. echo \"Dcs_write_1A_0P(0x28)_0x14\" > reg_dbg_mipi\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\tFor double bytes command ,to write offset first then the other byte like following\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\tto read 0xC680\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\teg. echo \"Dcs_write_1A_1P(0x00,0x80)_0\" > reg_dbg_mipi\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\teg. echo \"Dcs_read_1A_0P(0xC6)_0\" > reg_dbg_mipi\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\tthen cat reg_dbg_mipi.\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\tRead reg value more than one, please use Gen to write offset first then\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\tthe other byte like following to read 0xB8\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\teg. echo \"Gen_read_1A_7P(0xB8)_0\" > reg_dbg_mipi\n");
   buf_len -= len;
   cur += len;

   len = snprintf(cur, buf_len, "\tthen cat reg_dbg_mipi.\n");
   buf_len -= len;
   cur += len;

    ret_len = sizeof(lcdkit_debug_buf) - buf_len;

   //error happened!
   if (ret_len < 0)
      return 0;

    /* copy to user */
    if (copy_to_user(buff, lcdkit_debug_buf, ret_len))
    { return -EFAULT; }

    *ppos += ret_len;   // increase offset
    return ret_len;
}

/* write function */
static ssize_t lcdkit_dbg_write(
    struct file* file,
    const char __user* buff,
    size_t count,
    loff_t* ppos)
{
    char* cur, *ptr1, *ptr2;
    int ret = 0;
    int cmd_type = -1;
    int cnt = 0, i = 0;
    int val;
    unsigned long temp = 0;

    char lcd_debug_buf[256];
    int length = sizeof(lcdkit_cmd_list) / sizeof(lcdkit_cmd_list[0]);

    cur = lcd_debug_buf;

    if ((count > 255))
    {
        return count;
    }

    if (copy_from_user(lcd_debug_buf, buff, count))
    { return -EFAULT; }

    lcd_debug_buf[count] = 0;

    /* convert to lower case */
    if (0 != lcdkit_str_to_lower(cur))
    {
        goto err_handle;
    }

    LCDKIT_DEBUG("cur=%s, count=%d!\n", cur, (int)count);

    /* get cmd type */
    for (i = 0; i < length; i++)
    {
        if (0 == lcdkit_str_start_with(cur, lcdkit_cmd_list[i].pstr))
        {
            cmd_type = lcdkit_cmd_list[i].type;
            cur += strlen(lcdkit_cmd_list[i].pstr);
            break;
        }

        LCDKIT_DEBUG("lcdkit_cmd_list[%d].pstr=%s\n", i, lcdkit_cmd_list[i].pstr);
    }

    if (i >= length)
    {
        LCDKIT_ERR("cmd type not find!\n");  // not support
        goto err_handle;
    }

    switch (cmd_type)
    {
        case LCDKIT_DBG_LEVEL_SET:
            cnt = sscanf(cur, ":%d", &val);

            if (cnt != 1)
            {
                LCDKIT_ERR("get param fail!\n");
                goto err_handle;
            }

            lcdkit_dbg_set_dbg_level(val);
            break;

        case LCDKIT_DBG_MIPI_CLK:
            cnt = sscanf(cur, ":%d", &val);

            if (cnt != 1)
            {
                LCDKIT_ERR("get param fail!\n");
                goto err_handle;
            }

            lcdkit_dbg_enable_mipiclk(1);
            lcdkit_dbg_set_mipiclk(val);
            break;

        case LCDKIT_DBG_REG_READ:


            break;

        case LCDKIT_DBG_REG_WRITE:


            break;

        case LCDKIT_DBG_INIT_CODE:
            cnt = sscanf(cur, ":%d", &val);

            if (cnt != 1)
            {
                LCDKIT_ERR("get param fail!\n");
                goto err_handle;
            }

            lcdkit_dbg_enable_initcode(val);
            lcdkit_free_dsi_cmds(&lcdkit_info.panel_infos.g_panel_cmds);

            if (val == 1)
            {
                ret = lcdkit_parse_dsi_cmds(&lcdkit_info.panel_infos.g_panel_cmds);

                if (ret != 0)
                {
                    lcdkit_free_dsi_cmds(&lcdkit_info.panel_infos.g_panel_cmds);
                    lcdkit_dbg_enable_initcode(0);
                    LCDKIT_ERR("decode parameter error!\n");
                    goto err_handle;
                }

                LCDKIT_DEBUG("decode parameter succ!\n");
            }

            break;

        case LCDKIT_DBG_PANEL_VSP_VSN:
            lcdkit_dbg_enable_vsp_vsn(1);
            cur++;
            ret = strict_strtoul(cur, 0, &temp);

            if (ret)
            {
                LCDKIT_ERR("strict_strtoul error!\n");
                goto err_handle;
            }

            lcdkit_dbg.lcdkit_panel_bias = temp;
            lcdkit_dbg.lcdkit_panel_vsp = temp;
            lcdkit_dbg.lcdkit_panel_vsn = temp;
            LCDKIT_DEBUG("lcdkit_dbg.lcdkit_panel_vsp = %d!\n", lcdkit_dbg.lcdkit_panel_vsp);
            break;

        case LCDKIT_DBG_ESD_ENABLE:
            cnt = sscanf(cur, ":%d", &val);

            if (cnt != 1)
            {
                LCDKIT_ERR("get param fail!\n");
                goto err_handle;
            }

            lcdkit_dbg.g_esd_debug.esd_enable = val;
            LCDKIT_DEBUG("g_esd_debug.esd_enable = %d\n", lcdkit_dbg.g_esd_debug.esd_bl_enable);
            break;

        case LCDKIT_DBG_ESD_RECOVER_TEST:
            cnt = sscanf(cur, ":%d", &val);

            if (cnt != 1)
            {
                LCDKIT_ERR("get param fail!\n");
                goto err_handle;
            }

            lcdkit_dbg.g_esd_debug.esd_recover_test = val;
            break;

        case LCDKIT_DBG_ESD_RESET:
            cnt = sscanf(cur, ":%d", &val);

            if (cnt != 1)
            {
                LCDKIT_ERR("get param fail!\n");
                goto err_handle;
            }

            if (val)
            {
                memset(&lcdkit_dbg.g_esd_debug, 0, sizeof(struct lcdkit_esd_debug));
            }

            LCDKIT_DEBUG("g_esd_debug reset\n");
            break;

        case LCDKIT_DBG_ESD_BL_ENABLE:
            cnt = sscanf(cur, ":%d", &val);

            if (cnt != 1)
            {
                LCDKIT_ERR("get param fail!\n");
                goto err_handle;
            }

            lcdkit_dbg.g_esd_debug.esd_bl_enable = val;
            LCDKIT_DEBUG("lcdkit_dbg.g_esd_debug.esd_bl_enable = %d\n", lcdkit_dbg.g_esd_debug.esd_bl_enable);
            break;

        case LCDKIT_DBG_ESD_BL_SET:
            cnt = sscanf(cur, ":%d", &val);

            if (cnt != 1)
            {
                LCDKIT_ERR("get param fail!\n");
                goto err_handle;
            }

            lcdkit_dbg.g_esd_debug.esd_bl_set = val;
            LCDKIT_DEBUG("g_esd_debug.esd_bl_set = %d\n", lcdkit_dbg.g_esd_debug.esd_bl_set);
            break;

        case LCDKIT_DBG_ESD_CHECK_REG:
            ptr1 = ptr2 = (cur + 1);
            ptr2 = strchr(ptr2, ',');

            if (ptr2)
            {
                *ptr2 = 0;
                ptr2++;
            }

            while (ptr1)
            {
                ret = strict_strtoul(ptr1, 0, &temp);

                if (ret)
                {
                    LCDKIT_ERR("strict_strtoul error, buf=%s", ptr1);
                    return ret;
                }

                if (lcdkit_dbg.g_esd_debug.check_count >= 8)
                {
                    LCDKIT_ERR("check reg 0x%lx is too much.\n", temp);
                    break;
                }

                lcdkit_dbg.g_esd_debug.esd_check_reg[lcdkit_dbg.g_esd_debug.check_count++] = temp;
                ptr1 = ptr2;

                if (ptr2)
                {
                    ptr2 = strchr(ptr2, ',');

                    if (ptr2)
                    {
                        *ptr2 = 0;
                        ptr2++;
                    }
                }

                LCDKIT_DEBUG("g_esd_debug.esd_check_reg=0x%lx.\n", temp);
            }

            break;

        case LCDKIT_DBG_ESD_CHECK_VALUE:
            ptr1 = ptr2 = (cur + 1);
            ptr2 = strchr(ptr2, ',');

            if (ptr2)
            {
                *ptr2 = 0;
                ptr2++;
            }

            while (ptr1)
            {
                ret = strict_strtoul(ptr1, 0, &temp);

                if (ret)
                {
                    LCDKIT_ERR("strict_strtoul error, buf=%s", ptr1);
                    return ret;
                }

                if (cnt >= 8)
                {
                    LCDKIT_ERR("check reg 0x%lx is too much.\n", temp);
                    break;
                }

                lcdkit_dbg.g_esd_debug.esd_reg_value[cnt++] = temp;
                ptr1 = ptr2;

                if (ptr2)
                {
                    ptr2 = strchr(ptr2, ',');

                    if (ptr2)
                    {
                        *ptr2 = 0;
                        ptr2++;
                    }
                }

                LCDKIT_DEBUG("g_esd_debug.esd_check_reg=0x%lx.\n", temp);
            }

            break;

        default:
            LCDKIT_ERR("cmd type not support!\n");  // not support
            ret = -1;
            break;
    }

    /* finish */
    if (ret)
    {
        LCDKIT_ERR("fail\n");
        goto err_handle;
    }
    else
    {
        return count;
    }

err_handle:
    return -EFAULT;
}

static const struct file_operations lcdkit_debug_fops =
{
    .open = lcdkit_dbg_open,
    .release = lcdkit_dbg_release,
    .read = lcdkit_dbg_read,
    .write = lcdkit_dbg_write,
};

/*
 * reg_dbg
 *
 */
/* get reg, param and delay */
/* return: 0 - success, negative - fail */
static int lcdkit_dbg_mipi_get_params(char *buf, int param_num,
                                int *reg, char *param_buf, int *delay_ms, int cmd_type)
{
   char *cur = buf;
   char *temp = NULL;
   char *write_pos = param_buf;
   int cnt = 0;
   int param = 0;
   int i = 0;

   /* get reg */
   /* input format like: 1a_1p(0x51,0x00)_0 or 1a_0p(0x11)_0x78 */
   temp = strchr(cur, '(');
   if (NULL == temp)
   {
      goto err_handle;
   }
   cur = temp;
   cnt = sscanf(cur, "(%x", reg);
   if (cnt != 1)
   {
      goto err_handle;
   }
   else
   {

      if ((param_num != 0) && (LCDKIT_OPER_WRITE == cmd_type))
      {
        /* when param_num != 0, get ',' position */
         temp = strchr(cur, ',');
         if (NULL == temp)
         {
            goto err_handle;
         }
         cur = temp;

        /* loop to get param */
        for (i = 0; i < param_num; i++)
        {
            /* get a param */
            cnt = sscanf(cur, ",%x", &param);
            if (cnt != 1)
            {
                goto err_handle;
            }
            else
            {
                /* save param to param_buf */
                *write_pos = param;
                write_pos++;

                /* if current is not the last param */
                if (i != param_num - 1)
                {
                    cur++;  // ignore ',' which already handled
                    temp = strchr(cur, ',');
                    if (NULL == temp)
                    {
                        goto err_handle;
                    }
                    cur = temp;  // move to next param
                }
            }
        }
    }
}

   /* get delay */
   temp = strchr(cur, ')');
   if (NULL == temp)
   {
      goto err_handle;
   }
   cur = temp;

   cnt = sscanf(cur, ")_%x,", delay_ms);
   if (cnt != 1)
   {
      goto err_handle;
   }

   return 0;

err_handle:
   LCDKIT_ERR("input illegal\n");
   return -1;
}

/* check whether mipi input is legal or not */
/* return: 0 - success, negative - fail */
static int lcdkit_is_mipi_input_legal(int op_type,int ic_reg,
                                int cmd_type, int param_num,char *buf)
{
   int ret = 0;
   if( (op_type != LCDKIT_OPER_READ) && (op_type != LCDKIT_OPER_WRITE))
   {
      ret = -1;
   }

   /* ic_reg must in [0x00, 0xff] */
   if ((unsigned int)ic_reg > 0xff)
   {
      ret = -1;
   }

   /* cmd_type must be 0x01 or 0x04 */
   if ((cmd_type != LCDKIT_MIPI_DCS_COMMAND)
        && (cmd_type != LCDKIT_MIPI_GEN_COMMAND))
   {
      ret = -1;
   }
   /* param_num must larger or equal 0 */
   if (param_num < 0)
   {
      ret = -1;
   }

   if(NULL == buf)
   {
      ret = -1;
   }
   return ret;
}

void lcdkit_dump_reg_buf(const u32* buf, int cnt)
{
    if (NULL == buf){
        LCDKIT_ERR("input illegal.\n");
        return;
    }
    int i = 0;
    for (i = 0; i < cnt; i++) {
        switch (i % INT_COUNT) {
        case 0:
            if (i == cnt - 1) {
                LCDKIT_DEBUG( "0x%02x", buf[i / INT_COUNT] & 0xFF);
            } else {
                LCDKIT_DEBUG( "0x%02x,", buf[i / INT_COUNT] & 0xFF);
            }
            break;
        case 1:
            if (i == cnt - 1) {
                LCDKIT_DEBUG( "0x%02x", (buf[i / INT_COUNT] >> REG_BIT_COUNT) & 0xFF);
            } else {
                LCDKIT_DEBUG( "0x%02x,", (buf[i / INT_COUNT] >> REG_BIT_COUNT) & 0xFF);
            }
            break;
        case 2:
            if (i == cnt - 1) {
                LCDKIT_DEBUG( "0x%02x", (buf[i / INT_COUNT] >> (2 * REG_BIT_COUNT)) & 0xFF);
            } else {
                LCDKIT_DEBUG( "0x%02x,", (buf[i / INT_COUNT] >> (2 * REG_BIT_COUNT)) & 0xFF);
            }
            break;
        case 3:
            if (i == cnt - 1) {
                LCDKIT_DEBUG( "0x%02x", (buf[i / INT_COUNT] >> (3 * REG_BIT_COUNT)) & 0xFF);
            } else {
                LCDKIT_DEBUG( "0x%02x,", (buf[i / INT_COUNT] >> (3 * REG_BIT_COUNT)) & 0xFF);
            }
            break;
        default:
            break;
        }
    }
}

/**********************************************************************************
*function:process read and write commands  for lcd reg debug
*op_type:   read or write
*reg:      lcd register
*cmd_type:   DCS or GEN
*param_num:   the count of prameters to tranfer
*param_buf:   prameters
*read_value:  value from lcd panel
*delay_ms:   delay time
*return: 0 - success, negative - fail
**********************************************************************************/
int lcdkit_mipi_prcess_ic_reg(int op_type,int reg, int cmd_type,
            int param_num, char *param_buf,int *read_value, int delay_ms)
{
   uint32_t regvalue[LCDKIT_MAX_PARAM_BUF] = {0};
   int i = 0;
   void *ctrl;
   struct lcdkit_dsi_cmd_desc cmds_desc;
   static struct lcdkit_dsi_panel_cmds reg_cmds; // dsi cmd struct
   struct lcdkit_dsi_cmd_desc lcd_lreg_cmd;

   /* check if input legal */
   if (lcdkit_is_mipi_input_legal(op_type, reg,
                                cmd_type, param_num, param_buf))
   {
      LCDKIT_ERR("input illegal.\n");
      return -1;
   }

   ctrl = lcdkit_get_dsi_ctrl_pdata();
   if (NULL == ctrl)
   {
      LCDKIT_ERR("get  ctrl padata failed.\n");
      return -EFAULT;
   }

   /* translate cmd_type from huawei to qcom's format */
   switch (param_num)
   {
      case 0:
      {
         if(LCDKIT_OPER_READ == op_type)
         {
            /* DCS MODE */
            if (LCDKIT_MIPI_DCS_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_DCS_READ;
            }
            /* GEN MODE */
            else if (LCDKIT_MIPI_GEN_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_GEN_READ;
            }
         }
         else
         {
            /* DCS MODE */
            if (LCDKIT_MIPI_DCS_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_DCS_WRITE;
            }
            /* GEN MODE */
            else if (LCDKIT_MIPI_GEN_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_GEN_WRITE1;
            }
         }
         break;
      }

      case 1:
      {
         if(LCDKIT_OPER_READ == op_type)
         {
            /* DCS MODE */
            if (LCDKIT_MIPI_DCS_COMMAND == cmd_type)
            {
               LCDKIT_ERR("not support this kind of dcs read! \n");
               return -1;
            }
            /* GEN MODE */
            else if (LCDKIT_MIPI_GEN_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_GEN_READ1;
            }
         }
         else
         {
            /* DCS MODE */
            if (LCDKIT_MIPI_DCS_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_DCS_WRITE1;
            }
            /* GEN MODE */
            else if (LCDKIT_MIPI_GEN_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_GEN_WRITE2;
            }
         }
         break;
      }

      default:
      {
         if(LCDKIT_OPER_READ == op_type)
         {
            /* DCS MODE */
            if (LCDKIT_MIPI_DCS_COMMAND == cmd_type)
            {
               LCDKIT_ERR("not support this kind of dcs read! \n");
               return -1;
            }
            /* GEN MODE */
            else if (LCDKIT_MIPI_GEN_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_GEN_READ2;
            }
         }
         else
         {
            /* DCS MODE */
            if (LCDKIT_MIPI_DCS_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_DCS_LWRITE;
            }
            /* GEN MODE */
            else if (LCDKIT_MIPI_GEN_COMMAND == cmd_type)
            {
               cmd_type = DTYPE_GEN_LWRITE;
            }
         }
         break;
      }
   }

   /* insert reg into param_buf's beginning */
   memmove(param_buf + 1, param_buf, param_num);
   param_buf[0] = reg;
   param_num++;

   LCDKIT_DEBUG("op_type = %d, reg = 0x%02x, cmd_type = 0x%02x, "
                 "param_num = 0x%02x, delay_ms = 0x%02x\n",
                 op_type, reg, cmd_type, param_num, delay_ms);

   LCDKIT_DEBUG("print param_buf begin\n");
    lcdkit_dump_buf(param_buf, param_num);
   LCDKIT_DEBUG("print param_buf end\n");


   cmds_desc.dtype = cmd_type;
   cmds_desc.last = 1;
   cmds_desc.vc = 0;
    cmds_desc.waittype = 1;
   cmds_desc.dlen = param_num;
   cmds_desc.payload = param_buf;

    reg_cmds.blen = 0;
    reg_cmds.buf = NULL;
    reg_cmds.cmd_cnt = 1;
    reg_cmds.link_state = LCDKIT_DSI_LP_MODE;
    reg_cmds.cmds = &cmds_desc;

/*Cmd package for Gen read long reg */
    lcd_lreg_cmd.dtype = DTYPE_GEN_READ1;
    lcd_lreg_cmd.last = 0;
    lcd_lreg_cmd.vc = 0;
    lcd_lreg_cmd.ack = 0;
    lcd_lreg_cmd.wait = LCDKIT_READ_REG_WAIT_RESPONSE_TIME;
    lcd_lreg_cmd.waittype = LCDKIT_WAIT_TYPE_US;
    lcd_lreg_cmd.dlen = sizeof(reg);
    lcd_lreg_cmd.payload = &reg;

   switch(op_type)
   {
      case LCDKIT_OPER_READ:

         cmds_desc.ack = 1;
         cmds_desc.wait = 5;//5 ms
            reg_cmds.flags = LCDKIT_CMD_REQ_RX | LCDKIT_CMD_REQ_COMMIT;

            if(DTYPE_DCS_READ == cmd_type){
                lcdkit_dsi_rx(ctrl, regvalue, 1, &reg_cmds);
                param_num++;
            } else {
                lcdkit_lread_reg(ctrl, regvalue, &lcd_lreg_cmd, param_num - 1);
            }
            for (i = 0; i < param_num - 1; i++){
                read_value[i] = regvalue[i];
            }
            lcdkit_dump_reg_buf(read_value, param_num);
         break;

      case LCDKIT_OPER_WRITE:

         cmds_desc.ack = 0;
         cmds_desc.wait = delay_ms;//5 ms
            reg_cmds.flags = LCDKIT_CMD_REQ_COMMIT;


            lcdkit_dsi_tx(ctrl, &reg_cmds);

         break;
   }

   return 0;
}

/* read function */
/* show usage or print last read result */
static ssize_t lcdkit_mipi_reg_read(struct file *file,
                            char __user *buff, size_t count,loff_t *ppos)
{
   void *ctrl = NULL;
   int ret = 0;
   int ret_len = 0;
   char lcd_debug_buf[LCDKIT_MAX_PARAM_BUF] = {0};
   char tmp[LCDKIT_MAX_PARAM_BUF] = {0};
   int i = 0;

   if (*ppos)
      return 0;

   /* show usage */
   if (!lcdkit_dbg.lcdkit_ic_mipi_reg)
   {
        ret_len = snprintf(lcd_debug_buf, sizeof(lcd_debug_buf),
                            "reg value is invalid! Read first!\n");
   }
   /* show last read result */
   else
   {
        ctrl = lcdkit_get_dsi_ctrl_pdata();
        if (NULL == ctrl)
        {
            LCDKIT_ERR("get ctrl padata failed.\n");
            return -EFAULT;
        }

        ret = lcdkit_lock(ctrl);
        if(ret)
        {
            LCDKIT_ERR("the panel has been closed, please open it firstly.\n");
            ret_len = snprintf(lcd_debug_buf, sizeof(lcd_debug_buf),
                            "0x%02x = -1\n", lcdkit_dbg.lcdkit_ic_mipi_reg);
        }
        else
        {
            if (0 == lcdkit_dbg.lcdkit_g_param_num) {
                lcdkit_dbg.lcdkit_g_param_num = 1;  /*for Dcs_read*/
            }
            for (i = 0; i < lcdkit_dbg.lcdkit_g_param_num; i++) {
                switch (i % INT_COUNT) {
                case 0:
                    if (i == lcdkit_dbg.lcdkit_g_param_num - 1) {
                        snprintf( tmp, sizeof(tmp), "0x%02x", lcdkit_dbg.lcdkit_ic_mipi_value[i / INT_COUNT] & 0xFF);
                    } else {
                        snprintf( tmp, sizeof(tmp), "0x%02x,", lcdkit_dbg.lcdkit_ic_mipi_value[i / INT_COUNT] & 0xFF);
                    }
                    break;
                case 1:
                    if (i == lcdkit_dbg.lcdkit_g_param_num - 1) {
                        snprintf( tmp, sizeof(tmp), "0x%02x", (lcdkit_dbg.lcdkit_ic_mipi_value[i / INT_COUNT] >> REG_BIT_COUNT) & 0xFF);
                    } else {
                        snprintf( tmp, sizeof(tmp), "0x%02x,", (lcdkit_dbg.lcdkit_ic_mipi_value[i / INT_COUNT] >> REG_BIT_COUNT) & 0xFF);
                    }
                    break;
                case 2:
                    if (i == lcdkit_dbg.lcdkit_g_param_num - 1) {
                        snprintf( tmp, sizeof(tmp), "0x%02x", (lcdkit_dbg.lcdkit_ic_mipi_value[i / INT_COUNT] >> (2 * REG_BIT_COUNT)) & 0xFF);
                    } else {
                        snprintf( tmp, sizeof(tmp), "0x%02x,", (lcdkit_dbg.lcdkit_ic_mipi_value[i / INT_COUNT] >> (2 * REG_BIT_COUNT)) & 0xFF);
                    }
                    break;
                case 3:
                    if (i == lcdkit_dbg.lcdkit_g_param_num - 1) {
                        snprintf( tmp, sizeof(tmp), "0x%02x", (lcdkit_dbg.lcdkit_ic_mipi_value[i / INT_COUNT] >> (3 * REG_BIT_COUNT)) & 0xFF);
                    } else {
                        snprintf( tmp, sizeof(tmp), "0x%02x,", (lcdkit_dbg.lcdkit_ic_mipi_value[i / INT_COUNT] >> (3 * REG_BIT_COUNT)) & 0xFF);
                    }
                    break;
                default:
                    break;
                }
                strncat(lcd_debug_buf, tmp, strlen(tmp));
            }
            ret_len = snprintf(tmp, sizeof(tmp),
                            "0x%02x = %s\n", lcdkit_dbg.lcdkit_ic_mipi_reg, lcd_debug_buf);
            lcdkit_release(ctrl);
        }

   }

   /* copy to user */
   if (copy_to_user(buff, tmp, ret_len))
      return -EFAULT;

   *ppos += ret_len;   // increase offset
   return ret_len;
}


/* write function */
/* handle read or write ic command */
static ssize_t lcdkit_mipi_reg_write(struct file *file,
                   const char __user *buff, size_t count, loff_t *ppos)
{
   int ret = 0;
   int cmd_type = 0;
   int op_type = 0;
   int cnt = 0;
   int reg_num = 0;
   int param_num = 0;
   int reg = 0;
   int delay_ms = 0;
   char lcd_debug_buf[256];
   char lcd_param_buf[256];
   void *ctrl;

   char *cur = lcd_debug_buf;

   ctrl = lcdkit_get_dsi_ctrl_pdata();
   if (NULL == ctrl)
   {
      LCDKIT_ERR("get ctrl padata failed.\n");
      return -EFAULT;
   }

   ret = lcdkit_lock(ctrl);
   if(ret)
   {
      LCDKIT_ERR("lock failed or panel off.\n");
      return -EFAULT;
   }

   if (count >= sizeof(lcd_debug_buf))
   {
      LCDKIT_ERR("input overflow \n");
      goto err_handle;
   }

   if (copy_from_user(lcd_debug_buf, buff, count))
      goto err_handle;

   lcd_debug_buf[count] = 0;   /* end of string */

   /* convert to lower case */
   if (0 != lcdkit_str_to_lower(cur))
   {
      goto err_handle;
   }

   /* get cmd type */
   /* input format like: dcs_write_1a_1p(0x51,0x00)_0
      or dcs_write_1a_0p(0x11)_0x78 */
   if (0 == lcdkit_str_start_with(cur, LCDKIT_DCS_STR))
   {
      cmd_type = LCDKIT_MIPI_DCS_COMMAND;
      cur += strlen(LCDKIT_DCS_STR);  // move cur behind
   }
   else if (0 == lcdkit_str_start_with(cur, LCDKIT_GEN_STR))
   {
      cmd_type = LCDKIT_MIPI_GEN_COMMAND;
      cur += strlen(LCDKIT_GEN_STR);  // move cur behind
   }
   else
   {
      LCDKIT_ERR("cmd type not support!\n");  // not support
      goto err_handle;
   }

   /* get op type */
   /* input format like: write_1a_1p(0x51,0x00)_0
      or write_1a_0p(0x11)_0x78 */
   if (0 == lcdkit_str_start_with(cur, LCDKIT_READ_STR))
   {
      op_type = LCDKIT_OPER_READ;
      cur += strlen(LCDKIT_READ_STR);  // move cur behind
   }
   else if (0 == lcdkit_str_start_with(cur, LCDKIT_WRITE_STR))
   {
      op_type = LCDKIT_OPER_WRITE;
      cur += strlen(LCDKIT_WRITE_STR);  // move cur behind
   }
   else
   {
      LCDKIT_ERR("op type not support!\n");  // not support
      goto err_handle;
   }

   /* get reg_num, param_num */
   /* input format like: 1a_1p(0x51,0x00)_0 or 1a_0p(0x11)_0x78 */
   cnt = sscanf(cur,"%da_%dp(", &reg_num, &param_num);
   if (cnt != 2)
   {
      LCDKIT_ERR("get reg_num, param_num fail!\n");
      goto err_handle;
   }
   else
   {
      /* check input */
      if (reg_num != 1 || param_num < 0 || param_num > LCDKIT_MAX_PARAM_NUM)
      {
         LCDKIT_ERR("input reg num or para num illegal, reg_num = %d, "
                        "param_num = %d\n", reg_num, param_num);  // not support

         goto err_handle;
      }
   }

   /* get reg, param and delay */
   /* input format like: 1a_1p(0x51,0x00)_0 or 1a_0p(0x11)_0x78 */
   ret = lcdkit_dbg_mipi_get_params(cur, param_num, &reg, lcd_param_buf, &delay_ms, op_type);
   if (ret)
   {
      LCDKIT_ERR("lcd_dbg_mipi_get_reg_param_delay fail! ret = %d\n", ret);
      goto err_handle;
   }

   /* handle read or write ic command */
   switch (op_type)
   {
      /* read ic */
      case LCDKIT_OPER_READ:
      {
         memset(lcdkit_dbg.lcdkit_ic_mipi_value, 0, LCDKIT_MAX_PARAM_BUF);
         ret = lcdkit_mipi_prcess_ic_reg(LCDKIT_OPER_READ, reg, cmd_type,
                param_num, lcd_param_buf, lcdkit_dbg.lcdkit_ic_mipi_value, 0);

         if (0 == ret)
         {
            lcdkit_dbg.lcdkit_ic_mipi_reg = reg;    // save read reg to global
            lcdkit_dbg.lcdkit_g_param_num = param_num;
         }
         else
         {
            LCDKIT_ERR("read ic reg fail! ret = %d\n", ret);  // read fail
            goto err_handle;
         }
         break;
      }

      /* write ic */
      case LCDKIT_OPER_WRITE:
      {
         ret = lcdkit_mipi_prcess_ic_reg(LCDKIT_OPER_WRITE, reg, cmd_type,
                                        param_num, lcd_param_buf, 0, delay_ms);
         if (0 == ret)
         {
            LCDKIT_INFO("write reg: 0x%02x success\n", reg);
         }
         else
         {
            LCDKIT_ERR("write ic reg fail! ret = %d\n", ret);  // write fail
            goto err_handle;
         }
         break;
      }

      /* error */
      default:
      {
         LCDKIT_ERR("op type not support! op = %d\n", op_type);
         ret = -1;
         break;
      }
   }

   /* finish */
   if (ret)
   {
      LCDKIT_ERR("mipi reg debug fail\n");
      goto err_handle;
   }
   else
   {
      lcdkit_release(ctrl);
      return count;
   }

err_handle:
   lcdkit_release(ctrl);
   return -EFAULT;
}

/* fops of lcd_reg_dbg_mipi */
static const struct file_operations lcdkit_dbg_reg_mipi_fops = {
   .open      = lcdkit_dbg_open,
   .release   = lcdkit_dbg_release,
   .read      = lcdkit_mipi_reg_read,
   .write      = lcdkit_mipi_reg_write,
};

static void lcdkit_dbg_init(void)
{
    lcdkit_dbg.lcdkit_panel_bias = 5400000;
    lcdkit_dbg.lcdkit_panel_vsp = 5400000;
    lcdkit_dbg.lcdkit_panel_vsn = 5400000;
    lcdkit_dbg.lcdkit_g_initcode_enable = FALSE;
    lcdkit_dbg.lcdkit_ic_mipi_reg = 0;
}

/*
 * debugfs
 *
 */
/* init lcd debugfs interface */
int lcdkit_debugfs_init(void)
{
    static char already_init = 0;  // internal flag
    struct dentry* dent = NULL;
    struct dentry* file = NULL;

#ifndef LCDKIT_DEBUG_ENABLE
	LCDKIT_ERR("TARGET_BUILD_VARIANT is user,lcd-dbg is not create\n");
	return 0;
#endif

    /* judge if already init */
    if (already_init)
    {
        LCDKIT_ERR("(%d): already init\n", __LINE__);
        return 0;
    }

    /* create dir */
    dent = debugfs_create_dir("lcd-dbg", NULL);

    if (IS_ERR_OR_NULL(dent))
    {
        LCDKIT_ERR("(%d): debugfs_create_dir fail, error %ld\n", __LINE__, PTR_ERR(dent));
        dent = NULL;
        goto err_create_dir;
    }

    /* create reg_dbg_mipi node */
	file = debugfs_create_file("lcdkit_dbg", 0644, dent, 0, &lcdkit_debug_fops);
    if (IS_ERR_OR_NULL(file))
    {
        LCDKIT_ERR("(%d): debugfs_create_file: lcdkit_dbg fail\n", __LINE__);
        goto err_create_mipi;
    }

    /* create reg_dbg_mipi node */
	file = debugfs_create_file("reg_dbg_mipi", 0644, dent, 0, &lcdkit_dbg_reg_mipi_fops);
    if (IS_ERR_OR_NULL(file))
    {
        LCDKIT_ERR("(%d): debugfs_create_file: reg_dbg fail\n", __LINE__);
        goto err_create_mipi;
    }

    already_init = 1;  // set flag

    lcdkit_dbg_init();

    return 0;

err_create_mipi:

    if (dent)
    {
        debugfs_remove_recursive(dent);
        dent = NULL;
    }

err_create_dir:
    return -1;
}

