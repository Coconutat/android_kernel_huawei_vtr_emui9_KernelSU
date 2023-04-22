

/*
 * 1 Header File Including
 */

#define HISI_NVRAM_SUPPORT

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/of.h>

#ifdef HISI_NVRAM_SUPPORT
#include <linux/mtd/hisi_nve_interface.h>
#endif

#include "hisi_ini.h"
#include "board.h"

/*
 * 2 Global Variable Definition
 */
#define CUST_COMP_NODE             "hi1102,customize"
#define PROC_NAME_INI_FILE_NAME    "ini_file_name"
#define CUST_PATH_INI_CONN             "/data/vendor/cust_conn/ini_cfg"     /*某运营商在不同产品的差异配置*/
/* mutex for open ini file */
struct mutex        file_mutex;
int8 g_ini_file_name[INI_FILE_PATH_LEN] = {0};
int8 g_ini_conn_file_name[INI_FILE_PATH_LEN] = {0};
#define INI_FILE_PATH           (g_ini_file_name)

INI_BOARD_VERSION_STRU g_board_version = {{0}};
INI_PARAM_VERSION_STRU g_param_version = {{0}};

/*
 * 3 Function Definition
 */

static int32 ko_read_line(INI_FILE *fp, char *addr)
{
    int32 l_ret;
    int8  auc_tmp[MAX_READ_LINE_NUM] = {0};
    int32 cnt = 0;

    l_ret = kernel_read(fp, fp->f_pos, auc_tmp, MAX_READ_LINE_NUM);
    if (0 > l_ret)
    {
        INI_ERROR("kernel_line read l_ret < 0");
        return INI_FAILED;
    }
    else if (0 == l_ret)
    {
        /*end of file*/
        return 0;
    }

    cnt = 0;
    while (('\n' != auc_tmp[cnt]) && (cnt < MAX_READ_LINE_NUM - 1))
    {
        *addr++ = auc_tmp[cnt++];
    }

    if ((MAX_READ_LINE_NUM - 1) >= cnt)
    {
        *addr = '\n';
    }
    else
    {
        INI_ERROR("ko read_line is unexpected");
        return INI_FAILED;
    }

    /* change file pos to next line */
    fp->f_pos += (cnt + 1);

    return l_ret;
}


static INI_FILE * ini_file_open(int8 * filename, int8 * para)
{
    INI_FILE * fp;

    fp = filp_open(filename, O_RDONLY, 0);
    if (IS_ERR_OR_NULL(fp))
    {
        fp = NULL;
    }

    return fp;
}


static int32 ini_file_close(INI_FILE *fp)
{
    filp_close(fp, NULL);
    fp = NULL;
    return INI_SUCC;
}


static bool ini_file_exist(int8 *file_path)
{
    INI_FILE *fp = NULL;

    if (NULL == file_path)
    {
        INI_ERROR("para file_path is NULL\n");
        return false;
    }

    fp = ini_file_open(file_path, "rt");
    if (NULL == fp)
    {
        INI_DEBUG("%s not exist\n", file_path);
        return false;
    }

    ini_file_close(fp);

    INI_DEBUG("%s exist\n", file_path);

    return true;
}


static int32 ini_file_seek(INI_FILE *fp, long fp_pos)
{
    fp->f_pos += fp_pos;
    return INI_SUCC;
}


static int32 ini_readline_func(INI_FILE *fp, int8 * rd_buf)
{
    int8 auc_tmp[MAX_READ_LINE_NUM];
    int32 l_ret;

    memset(auc_tmp, 0, MAX_READ_LINE_NUM);
    l_ret = ko_read_line(fp, auc_tmp);
    if (INI_FAILED == l_ret)
    {
        INI_ERROR("ko_read_line failed!!!");
        return INI_FAILED;
    }
    else if (0 == l_ret)
    {
        INI_ERROR("end of .ini file!!!");
        return INI_FAILED;
    }

    strncpy(rd_buf, auc_tmp, MAX_READ_LINE_NUM);

    return INI_SUCC;
}


int32 ini_check_str(INI_FILE *fp, int8 * auc_tmp, int8 * puc_var)
{
    uint16 auc_len;
    uint16 curr_var_len;
    uint16 search_var_len;

    if ((NULL == fp)||(NULL == puc_var)||('\0' == puc_var[0]))
    {
        INI_ERROR("check if puc_var is NULL or blank");
        return INI_FAILED;
    }

    do{
        auc_len = strlen(auc_tmp);
        curr_var_len = 0;

        while ((curr_var_len < MAX_READ_LINE_NUM) && (auc_tmp[curr_var_len] != '\r') && (auc_tmp[curr_var_len] != '\n') &&
                (auc_tmp[curr_var_len] != 0))
        {
            curr_var_len++;
        }

        if (('#' == auc_tmp[0]) || (' ' == auc_tmp[0]) || ('\n' == auc_tmp[0]) || ('\r' == auc_tmp[0]))
        {
            break;
        }
        search_var_len = strlen(puc_var);
        if (search_var_len > curr_var_len)
        {
            search_var_len = curr_var_len;
        }
        if (0 == strncmp(auc_tmp, puc_var, search_var_len))
        {
            return INI_SUCC;
        }
        else
        {
            break;
        }
    }while(0);

    if (INI_FAILED == ini_file_seek(fp, -auc_len))
    {
        INI_ERROR("file seek failed!!!");
        return INI_FAILED;
    }
    if (INI_FAILED == ini_file_seek(fp, curr_var_len+1))
    {
        INI_ERROR("file seek failed!!!");
        return INI_FAILED;
    }
    if (auc_tmp[curr_var_len + 1] == '\n')
    {
        if (INI_FAILED == ini_file_seek(fp, 1))
        {
            INI_ERROR("file seek failed!!!");
            return INI_FAILED;
        }
    }
    return INI_FAILED;
}


static int32 ini_check_value(int8 *puc_value)
{
    uint32 ul_len;

    ul_len = strlen(puc_value);
    if (ul_len < 2)
    {
        INI_ERROR("ini_check_value fail, puc_value length %u < 2(min len)\n", ul_len);
        return INI_FAILED;
    }

    if (' ' == puc_value[0] || ' ' == puc_value[ul_len - 1] || '\n' == puc_value[0])
    {
        puc_value[0] = '\0';
        INI_ERROR("::%s has blank space or is blank::", puc_value);
        return INI_FAILED;
    }

    /* check \n of line */
    if ('\n' == puc_value[ul_len - 1])
    {
        puc_value[ul_len - 1] = '\0';
    }

    /* check \r of line */
    if ('\r' == puc_value[ul_len - 2])
    {
        puc_value[ul_len - 2] = '\0';
    }

    return INI_SUCC;
}


static int32 ini_find_modu(INI_FILE *fp, int32 tag_index, int8 * puc_var, int8 *puc_value)
{
    int8 auc_tmp[MAX_READ_LINE_NUM];
    int8 auc_modu[INI_STR_MODU_LEN];
    int32 ret;

    switch (tag_index)
    {
        case INI_MODU_WIFI:
            strncpy(auc_modu, INI_STR_WIFI_NORMAL, INI_STR_MODU_LEN);
            break;
        case INI_MODU_POWER_FCC:
            strncpy(auc_modu, INI_STR_POWER_FCC, INI_STR_MODU_LEN);
            break;
        case INI_MODU_POWER_ETSI:
            strncpy(auc_modu, INI_STR_POWER_ETSI, INI_STR_MODU_LEN);
            break;
        case INI_MODU_POWER_JP:
            strncpy(auc_modu, INI_STR_POWER_JP, INI_STR_MODU_LEN);
            break;
        case INI_MODU_GNSS:
            strncpy(auc_modu, INI_STR_GNSS_NORMAL, INI_STR_MODU_LEN);
            break;
        case INI_MODU_BT:
            strncpy(auc_modu, INI_STR_BT_NORMAL, INI_STR_MODU_LEN);
            break;
        case INI_MODU_FM:
            strncpy(auc_modu, INI_STR_FM_NORMAL, INI_STR_MODU_LEN);
            break;
        case INI_MODU_PLAT:
            strncpy(auc_modu, INI_STR_PLAT, INI_STR_MODU_LEN);
            break;
        case INI_MODU_HOST_VERSION:
            strncpy(auc_modu, INT_STR_HOST_VERSION, INI_STR_MODU_LEN);
            break;
        case INI_MODU_WIFI_MAC:
            strncpy(auc_modu, INI_STR_WIFI_MAC, INI_STR_MODU_LEN);
            break;
        case INI_MODU_COEXIST:
            strncpy(auc_modu, INI_STR_COEXIST, INI_STR_MODU_LEN);
            break;
        case INI_MODU_DEV_WIFI:
            strncpy(auc_modu, INI_STR_DEVICE_WIFI, INI_STR_MODU_LEN);
            break;
        case INI_MODU_DEV_GNSS:
            strncpy(auc_modu, INI_STR_DEVICE_GNSS, INI_STR_MODU_LEN);
            break;
        case INI_MODU_DEV_BT:
            strncpy(auc_modu, INI_STR_DEVICE_BT, INI_STR_MODU_LEN);
            break;
        case INI_MODU_DEV_FM:
            strncpy(auc_modu, INI_STR_DEVICE_FM, INI_STR_MODU_LEN);
            break;
        case INI_MODU_DEV_BFG_PLAT:
            strncpy(auc_modu, INI_STR_DEVICE_BFG_PLAT, INI_STR_MODU_LEN);
            break;
        case INI_MODU_HOST_IR:
            strncpy(auc_modu, INI_STR_HOST_IR, INI_STR_MODU_LEN);
            break;
        default :
            INI_ERROR("not suport tag type:%x!!!", tag_index);
            break;
    }

    /* find the value of mode var, such as ini_wifi_mode
     * every mode except PLAT mode has only one mode var */
    while(1)
    {
        ret = ini_readline_func(fp, auc_tmp);
        if (INI_FAILED == ret)
        {
            INI_ERROR("have end of .ini file!!!");
            return INI_FAILED;
        }

        if (NULL != strstr(auc_tmp, INI_STR_DEVICE_BFG_PLAT))
        {
            INI_ERROR("not find %s!!!", auc_modu);
            return INI_FAILED;
        }

        ret = ini_check_str(fp, auc_tmp, auc_modu);
        if (INI_SUCC == ret)
        {
            INI_DEBUG("have found %s", auc_modu);
            break;
        }
        else
        {
            continue;
        }
    }

    return INI_SUCC;
}


static int32 ini_find_var(INI_FILE *fp, int32 tag_index, int8 * puc_var, int8 *puc_value, uint32 size)
{
    int32 ret;
    int8 auc_tmp[MAX_READ_LINE_NUM] = {0};
    size_t search_var_len;

    /* find the modu of var, such as [HOST_WIFI_NORMAL] of wifi moduler*/
    ret = ini_find_modu(fp, tag_index, puc_var, puc_value);
    if (INI_FAILED == ret)
    {
        return INI_FAILED;
    }

    /* find the var in modu, such as [HOST_WIFI_NORMAL] of wifi moduler*/
    while(1)
    {
        ret = ini_readline_func(fp, auc_tmp);
        if (INI_FAILED == ret)
        {
            INI_ERROR("have end of .ini file!!!");
            return INI_FAILED;
        }

        if ('[' == auc_tmp[0])
        {
            INI_ERROR("not find %s!!!, check if var in correct mode", puc_var);
            return INI_FAILED;
        }

        search_var_len = strlen(puc_var);
        ret = ini_check_str(fp, auc_tmp, puc_var);
        if (INI_SUCC == ret)
        {
            strncpy(puc_value, &auc_tmp[search_var_len+1], size);
            break;
        }
        else
        {
            continue;
        }
    }

    return INI_SUCC;
}


void print_device_version(void)
{
    INI_FILE *fp = NULL;
    int8  version_buff[INI_VERSION_STR_LEN] = {0};
    int32 l_ret;

    INI_MUTEX_LOCK(&file_mutex);

    fp = ini_file_open(INI_FILE_PATH, "rt");
    if (0 == fp)
    {
        fp = NULL;
        INI_ERROR("open %s failed!!!", INI_FILE_PATH);
        goto open_ini_file_fail;
    }

    l_ret = ini_find_var(fp, INI_MODU_PLAT, INI_VAR_PLAT_BOARD, version_buff, sizeof(version_buff));
    if (INI_FAILED == l_ret)
    {
        version_buff[0] = '\0';
        goto read_ini_var_fail;
    }

    if (INI_FAILED == ini_check_value(version_buff))
    {
        goto read_ini_var_fail;
    }
    strncpy(g_board_version.board_version, version_buff, INI_VERSION_STR_LEN);
    INI_INFO("::g_board_version.board_version = %s::", g_board_version.board_version);
    fp->f_pos = 0;

    l_ret = ini_find_var(fp, INI_MODU_PLAT, INI_VAR_PLAT_PARAM, version_buff, sizeof(version_buff));
    if (INI_FAILED == l_ret)
    {
        version_buff[0] = '\0';
        goto read_ini_var_fail;
    }

    if (INI_FAILED == ini_check_value(version_buff))
    {
        goto read_ini_var_fail;
    }

    strncpy(g_param_version.param_version, version_buff, INI_VERSION_STR_LEN);
    INI_INFO("::g_param_version.param_version = %s::", g_param_version.param_version);
    fp->f_pos = 0;
    ini_file_close(fp);
    INI_MUTEX_UNLOCK(&file_mutex);

    return;

read_ini_var_fail:
    fp->f_pos = 0;
    ini_file_close(fp);
open_ini_file_fail:
    INI_MUTEX_UNLOCK(&file_mutex);
    return;
}

int32 find_download_channel(uint8* buff,int8 * puc_var)
{
    INI_FILE *fp = NULL;
    int8  version_buff[DOWNLOAD_CHANNEL_LEN] = {0};
    int32 l_ret;

    INI_MUTEX_LOCK(&file_mutex);
    INI_INFO("ini file_name is %s", INI_FILE_PATH);
    fp = ini_file_open(INI_FILE_PATH, "rt");
    if (0 == fp)
    {
        fp = NULL;
        INI_ERROR("open %s failed!!!", INI_FILE_PATH);
        goto open_ini_file_fail;
    }

    /*find wlan download channel*/
    l_ret = ini_find_var(fp, INI_MODU_PLAT, puc_var, version_buff, DOWNLOAD_CHANNEL_LEN);
    if (INI_FAILED == l_ret)
    {
        version_buff[0] = '\0';
        goto read_ini_var_fail;
    }
    if (INI_FAILED == ini_check_value(version_buff))
    {
        goto read_ini_var_fail;
    }
    strncpy(buff, version_buff, DOWNLOAD_CHANNEL_LEN);

    fp->f_pos = 0;
    ini_file_close(fp);
    INI_MUTEX_UNLOCK(&file_mutex);
    return INI_SUCC;

read_ini_var_fail:
    fp->f_pos = 0;
    ini_file_close(fp);
open_ini_file_fail:
    INI_MUTEX_UNLOCK(&file_mutex);
    return INI_FAILED;

}

int32 ini_find_var_value_by_path(int8* path, int32 tag_index, int8 * puc_var, int8* puc_value, uint32 size)
{
    INI_FILE *fp = NULL;

#ifdef INI_TIME_TEST
    struct timeval tv[2];
#endif

    int32 l_ret;

    if (NULL == puc_var || '\0' == puc_var[0] || NULL == puc_value)
    {
        INI_ERROR("check if puc_var and puc_value is NULL or blank");
        return INI_FAILED;
    }

#ifdef INI_TIME_TEST
    do_gettimeofday(&tv[0]);
#endif

    INI_MUTEX_LOCK(&file_mutex);

    fp = ini_file_open(path, "rt");
    if (0 == fp)
    {
        fp = NULL;
        INI_ERROR("open %s failed!!!", path);
        INI_MUTEX_UNLOCK(&file_mutex);
        return INI_FAILED;
    }

    /* find puc_var in .ini return puc_value */
    l_ret = ini_find_var(fp, tag_index, puc_var, puc_value, size);
    if (INI_FAILED == l_ret)
    {
        puc_value[0] = '\0';
        ini_file_close(fp);
        INI_MUTEX_UNLOCK(&file_mutex);
        return INI_FAILED;
    }

#ifdef INI_TIME_TEST
    do_gettimeofday(&tv[1]);
    INI_DEBUG("time take = %ld", (tv[1].tv_sec - tv[0].tv_sec) * 1000 + (tv[1].tv_usec - tv[0].tv_usec) / 1000);
#endif

    ini_file_close(fp);
    INI_MUTEX_UNLOCK(&file_mutex);

    /* check blank space of puc_value */
    if (INI_SUCC == ini_check_value(puc_value))
    {
        INI_DEBUG("::return %s:%s::", puc_var, puc_value);
        return INI_SUCC;
    }

    return INI_FAILED;
}


int32 ini_find_var_value(int32 tag_index, int8 * puc_var, int8* puc_value, uint32 size)
{
    /* read spec if exist */
    if (ini_file_exist(g_ini_conn_file_name))
    {
        if (INI_SUCC == ini_find_var_value_by_path(g_ini_conn_file_name, tag_index, puc_var, puc_value, size))
        {
            return INI_SUCC;
        }
    }

    if (0 == ini_file_exist(INI_FILE_PATH))
    {
        INI_ERROR(" %s not exist!!!", INI_FILE_PATH);
        return INI_FAILED;
    }

    return ini_find_var_value_by_path(INI_FILE_PATH, tag_index, puc_var, puc_value, size);
}


int32 get_ini_file_name_from_dts(int8 *dts_prop, int8 *prop_value, uint32 size)
{
    int32  ret = 0;
    struct device_node *np;
    int32  len;
    int8   out_str[HISI_CUST_NVRAM_LEN] = {0};

    np = of_find_compatible_node(NULL, NULL, CUST_COMP_NODE);
    if (NULL == np)
    {
        INI_ERROR("dts node %s not found", CUST_COMP_NODE);
        return INI_FAILED;
    }

    len = of_property_count_u8_elems(np, dts_prop);
    if (len < 0)
    {
        INI_ERROR("can't get len of dts prop(%s)", dts_prop);
        return INI_FAILED;
    }

    len = INI_MIN(len, sizeof(out_str));
    INI_DEBUG("read len of dts prop %s is:%d", dts_prop, len);
    ret = of_property_read_u8_array(np, dts_prop, out_str, len);
    if (0 > ret)
    {
        INI_ERROR("read dts prop (%s) fail", dts_prop);
        return INI_FAILED;
    }

    len = INI_MIN(len, size);
    memcpy(prop_value, out_str, len);
    INI_DEBUG("dts prop %s value is:%s", dts_prop, prop_value);

    return INI_SUCC;
}


int32 read_conf_from_nvram(int8 * name, int8 * pc_out, uint32 size)
{
    struct hisi_nve_info_user  info;
    uint32 len = 0;
    int32 ret = -1;

    memset(&info, 0, sizeof(info));
    strncpy(info.nv_name, HISI_CUST_NVRAM_NAME, strlen(HISI_CUST_NVRAM_NAME) - 1);
    info.nv_name[strlen(HISI_CUST_NVRAM_NAME) - 1] = '\0';
    info.nv_number = HISI_CUST_NVRAM_NUM;
    info.valid_size = HISI_CUST_NVRAM_LEN;
    info.nv_operation = HISI_CUST_NVRAM_READ;

    ret = hisi_nve_direct_access( &info );
    if (ret < -1) {
        INI_ERROR("read nvm failed");
        return INI_FAILED;
    }
    else
    {
        len = INI_MIN(size, NUM_OF_NV_PARAMS);
        memcpy(pc_out, info.nv_data, len);
    }

    return INI_SUCC;
}


int32 write_conf_to_nvram(int8 * name, int8 * pc_arr)
{
    struct hisi_nve_info_user  info;
    int32 ret = -1;

    memset(&info, 0, sizeof(info));
    strncpy(info.nv_name, HISI_CUST_NVRAM_NAME, strlen(HISI_CUST_NVRAM_NAME) - 1);
    info.nv_name[strlen(HISI_CUST_NVRAM_NAME) - 1] = '\0';
    info.nv_number = HISI_CUST_NVRAM_NUM;
    info.valid_size = HISI_CUST_NVRAM_LEN;
    info.nv_operation = HISI_CUST_NVRAM_WRITE;
    memcpy(info.nv_data, pc_arr, HISI_CUST_NVRAM_LEN);

    ret = hisi_nve_direct_access( &info );
    if (ret < -1) {
        INI_ERROR("write nvm failed");
        return INI_FAILED;
    }

    return INI_SUCC;
}


int32 get_cust_conf_string(int32 tag_index, int8 * puc_var, int8* puc_value, uint32 size)
{
    int32 ret = 0;

    if (CUST_MODU_NVRAM == tag_index)
    {
        ret = read_conf_from_nvram(puc_var, puc_value, size);
        if (ret < 0)
        {
            INI_ERROR("read nv_conf failed, ret(%d)", ret);
            return INI_FAILED;
        }

        return INI_SUCC;
    }
    else
    {
        return ini_find_var_value(tag_index, puc_var, puc_value, size);
    }
}


int32 set_cust_conf_string(int32 tag_index, int8 * name, int8 * var)
{
    int32 ret = INI_FAILED;

    if (tag_index != CUST_MODU_NVRAM)
    {
        INI_ERROR("NOT SUPPORT MODU TO WRITE");
        return INI_FAILED;
    }

    ret = write_conf_to_nvram(name, var);

    return ret;
}


int32 get_cust_conf_int32(int32 tag_index, int8 * puc_var, int32* puc_value)
{
    int32 ret = 0;
    int8  out_str[INI_READ_VALUE_LEN] = {0};

    ret = ini_find_var_value(tag_index, puc_var, out_str, sizeof(out_str));
    if (ret < 0)
    {
        /* ini_find_var_value has error log, delete this log */
        INI_DEBUG("cust modu didn't get var of %s.", puc_var);
        return INI_FAILED;
    }

    if (!strncmp(out_str, "0x", strlen("0x")) || !strncmp(out_str, "0X", strlen("0X")))
    {
        INI_DEBUG("get hex of:%s.", puc_var);
        ret = sscanf(out_str, "%x", puc_value);
    }
    else
    {
        ret = sscanf(out_str, "%d", puc_value);
    }

    if(ret < 0)
    {
        INI_ERROR("%s trans to int failed", puc_var);
        return INI_FAILED;
    }
    INI_DEBUG("conf %s get vale:%d", puc_var, *puc_value);

    return INI_SUCC;
}

int ini_cfg_init(void)
{
    int32 ret;
    int8 auc_dts_ini_path[INI_FILE_PATH_LEN]  = {0};

    INI_INFO("hi110x ini config search init!\n");

    ret = get_ini_file_name_from_dts(PROC_NAME_INI_FILE_NAME, auc_dts_ini_path, sizeof(auc_dts_ini_path));
    if ( 0 > ret )
    {
        INI_ERROR("can't find dts proc %s\n", PROC_NAME_INI_FILE_NAME);
        return INI_FAILED;
    }

    INI_INIT_MUTEX(&file_mutex);

    snprintf(g_ini_file_name, sizeof(g_ini_file_name)-1, "%s", auc_dts_ini_path);
    snprintf(g_ini_conn_file_name, sizeof(g_ini_conn_file_name)-1, "%s", CUST_PATH_INI_CONN);

    INI_INFO("%s@%s\n", PROC_NAME_INI_FILE_NAME, g_ini_file_name);

    print_device_version();

    return INI_SUCC;
}

void ini_cfg_exit(void)
{
    INI_INFO("hi110x ini config search exit!\n");
}

//module_init(ini_cfg_init);
//module_exit(ini_cfg_exit);
//MODULE_AUTHOR("Hisilicon Connectivity ini config search Driver Group");
//MODULE_DESCRIPTION("Hi110x ini config search platform driver");
//MODULE_LICENSE("GPL");

