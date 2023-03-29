

/*
 * 1 Header File Including
 */
#include <linux/version.h>
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
#define HISI_NVRAM_SUPPORT
#define HISI_DTS_SUPPORT
#endif

#include <linux/module.h>
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include <linux/kernel.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#endif
#endif
#include <linux/time.h>
#include <linux/fs.h>

#ifdef HISI_NVRAM_SUPPORT
#include <linux/mtd/hisi_nve_interface.h>
#endif

#include "hisi_ini.h"
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "board.h"
#endif
#include "oal_schedule.h"
#if (_PRE_PRODUCT_ID_HI1151==_PRE_PRODUCT_ID)
#include <linux/err.h>
#endif
#ifdef HISI_DTS_SUPPORT
#include "board.h"
#endif
/*
 * 2 Global Variable Definition
 */
#define CUST_PATH_INI_CONN             "/data/vendor/cust_conn/ini_cfg"     /*某运营商在不同产品的差异配置*/
/* mutex for open ini file */
struct mutex        file_mutex_etc;
#if (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
int8 g_ini_file_name_etc[INI_FILE_PATH_LEN] = "/system/bin/wifi_hisi/cfg_e5_hisi.ini";
#elif (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
int8 g_ini_file_name_etc[INI_FILE_PATH_LEN] = "/var/cfg_ont_hisi.ini";
#elif (_PRE_TARGET_PRODUCT_TYPE_5630HERA == _PRE_CONFIG_TARGET_PRODUCT)
int8 g_ini_file_name_etc[INI_FILE_PATH_LEN] = "/etc/cfg_hera_hisi.ini";
#else
int8 g_ini_file_name_etc[INI_FILE_PATH_LEN] = "/system/bin/wifi_hisi/cfg_e5_hisi.ini";
#endif
int8 g_ini_conn_file_name_etc[INI_FILE_PATH_LEN] = {0};
#define INI_FILE_PATH           (g_ini_file_name_etc)

INI_BOARD_VERSION_STRU g_board_version_etc = {{0}};
INI_PARAM_VERSION_STRU g_param_version_etc = {{0}};

#if (_PRE_PRODUCT_ID_HI1151==_PRE_PRODUCT_ID)
enum PLAT_LOGLEVLE{
	PLAT_LOG_ALERT = 0,
	PLAT_LOG_ERR = 1,
	PLAT_LOG_WARNING = 2,
	PLAT_LOG_INFO = 3,
	PLAT_LOG_DEBUG = 4,
};
#define DOWNLOAD_CHANNEL_LEN                (64)
int32 g_plat_loglevel_etc = PLAT_LOG_INFO;
#endif

int64 g_ini_file_time_sec = 0;
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
    while ((cnt < MAX_READ_LINE_NUM - 1) && ('\n' != auc_tmp[cnt]))
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

    UNREF_PARAM(para);
    fp = (INI_FILE *)filp_open(filename, O_RDONLY, 0);
    if (OAL_IS_ERR_OR_NULL(fp))
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


/*Note:symbol "fp" not accessed*/
/*lint -e550*/
static int32 ini_file_seek(INI_FILE *fp, long fp_pos)
{
    fp->f_pos += fp_pos;
    return INI_SUCC;
}
/*lint +e550*/

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


int32 ini_check_str_etc(INI_FILE *fp, int8 * auc_tmp, int8 * puc_var)
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
        auc_len = (uint16)strlen(auc_tmp);
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
        search_var_len = (uint16)strlen(puc_var);
        if (search_var_len > curr_var_len)
        {
            break;
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
    if (((curr_var_len + 1) < MAX_READ_LINE_NUM) && (auc_tmp[curr_var_len + 1] == '\n'))
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
    uint32 ul_len = 0x00;

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
    int8 auc_tmp[MAX_READ_LINE_NUM] = {0};
    int8 auc_modu[INI_STR_MODU_LEN] = {0};
    int32 ret;

    UNREF_PARAM(puc_var);
    UNREF_PARAM(puc_value);

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
        default :
            INI_ERROR("not suport tag type:%x!!!", tag_index);
            break;
    }

    /* find the value of mode var, such as ini_wifi_mode
     * every mode except PLAT mode has only one mode var */
    for(;;)
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

        ret = ini_check_str_etc(fp, auc_tmp, auc_modu);
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
    int8 auc_tmp[MAX_READ_LINE_NUM + 1] = {0};
    size_t search_var_len;

    /* find the modu of var, such as [HOST_WIFI_NORMAL] of wifi moduler*/
    ret = ini_find_modu(fp, tag_index, puc_var, puc_value);

    if (INI_FAILED == ret)
    {
        return INI_FAILED;
    }

    /* find the var in modu, such as [HOST_WIFI_NORMAL] of wifi moduler*/
    for(;;)
    {
        ret = ini_readline_func(fp, auc_tmp);
        if (INI_FAILED == ret)
        {
            INI_ERROR("have end of .ini file!!!");
            return INI_FAILED;
        }

        if ('[' == auc_tmp[0])
        {
        #ifndef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
            INI_ERROR("not find %s!!!, check if var in correct mode", puc_var);
        #endif
            return INI_FAILED;
        }

        search_var_len = strlen(puc_var);
        ret = ini_check_str_etc(fp, auc_tmp, puc_var);
        if ((INI_SUCC == ret) && ('=' == auc_tmp[search_var_len]))
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

int32 find_download_channel_etc(uint8* buff,int8 * puc_var)
{
    INI_FILE *fp = NULL;
    int8  version_buff[DOWNLOAD_CHANNEL_LEN] = {0};
    int32 l_ret;

    INI_MUTEX_LOCK(&file_mutex_etc);
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
    strncpy((int8*)buff, version_buff, DOWNLOAD_CHANNEL_LEN);
    buff[DOWNLOAD_CHANNEL_LEN - 1] = '\0';

    fp->f_pos = 0;
    ini_file_close(fp);
    INI_MUTEX_UNLOCK(&file_mutex_etc);
    return INI_SUCC;

read_ini_var_fail:
    fp->f_pos = 0;
    ini_file_close(fp);
open_ini_file_fail:
    INI_MUTEX_UNLOCK(&file_mutex_etc);
    return INI_FAILED;

}

int32 ini_find_var_value_by_path_etc(int8* path, int32 tag_index, int8 * puc_var, int8* puc_value, uint32 size)
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

    INI_MUTEX_LOCK(&file_mutex_etc);

    fp = ini_file_open(path, "rt");
    if (0 == fp)
    {
        INI_ERROR("open %s failed!!!", path);
        INI_MUTEX_UNLOCK(&file_mutex_etc);
        return INI_FAILED;
    }

    /* find puc_var in .ini return puc_value */
    l_ret = ini_find_var(fp, tag_index, puc_var, puc_value, size);
    if (INI_FAILED == l_ret)
    {
        puc_value[0] = '\0';
        ini_file_close(fp);
        INI_MUTEX_UNLOCK(&file_mutex_etc);
        return INI_FAILED;
    }

#ifdef INI_TIME_TEST
    do_gettimeofday(&tv[1]);
    INI_DEBUG("time take = %ld", (tv[1].tv_sec - tv[0].tv_sec) * 1000 + (tv[1].tv_usec - tv[0].tv_usec) / 1000);
#endif

    ini_file_close(fp);
    INI_MUTEX_UNLOCK(&file_mutex_etc);

    /* check blank space of puc_value */
    if (INI_SUCC == ini_check_value(puc_value))
    {
        INI_DEBUG("::return %s:%s::", puc_var, puc_value);
        return INI_SUCC;
    }

    return INI_FAILED;
}


int32 ini_find_var_value_etc(int32 tag_index, int8 * puc_var, int8* puc_value, uint32 size)
{
    /* read spec if exist */
    if (ini_file_exist(g_ini_conn_file_name_etc))
    {
        if (INI_SUCC == ini_find_var_value_by_path_etc(g_ini_conn_file_name_etc, tag_index, puc_var, puc_value, size))
        {
            return INI_SUCC;
        }
    }

    if (0 == ini_file_exist(INI_FILE_PATH))
    {
        INI_ERROR(" %s not exist!!!", INI_FILE_PATH);
        return INI_FAILED;
    }

    return ini_find_var_value_by_path_etc(INI_FILE_PATH, tag_index, puc_var, puc_value, size);
}


#ifdef HISI_NVRAM_SUPPORT

int32 read_conf_from_nvram_etc(uint8 *pc_out, uint32 size, uint32 nv_number,  const char* nv_name)
{
    struct hisi_nve_info_user  info;
    int32 ret = INI_FAILED;

    OAL_MEMZERO(&info, sizeof(info));
    OAL_MEMZERO(pc_out, size);
    oal_strncpy(info.nv_name, nv_name, strlen(HISI_CUST_NVRAM_NAME));
    info.nv_name[strlen(HISI_CUST_NVRAM_NAME)] = '\0';
    info.nv_number = nv_number;
    info.valid_size = HISI_CUST_NVRAM_LEN;
    info.nv_operation = HISI_CUST_NVRAM_READ;

    ret = hisi_nve_direct_access(&info);
    if (size > sizeof(info.nv_data) || OAL_STRLEN(info.nv_data) >= size)
    {
        INI_ERROR("read nvm{%s}lenth[%d] longer than input[%d]", info.nv_data, (uint32)OAL_STRLEN(info.nv_data), size);
        return INI_FAILED;
    }
    if (INI_SUCC == ret)
    {
        memcpy(pc_out, info.nv_data, size);
        OAL_IO_PRINT("read_conf_from_nvram_etc::nvram id[%d] nv name[%s] get data{%s}, size[%d]\r\n!", nv_number, nv_name, info.nv_data, size);
    }
    else
    {
        INI_ERROR("read nvm [%d] %s failed", nv_number, nv_name);
        return INI_FAILED;
    }

    return INI_SUCC;
}


int32 write_conf_to_nvram_etc(int8 * name, int8 * pc_arr)
{
    struct hisi_nve_info_user  info;
    int32 ret = -1;

    UNREF_PARAM(name);

    memset(&info, 0, sizeof(info));
    strncpy(info.nv_name, HISI_CUST_NVRAM_NAME, sizeof(info.nv_name));
    info.nv_name[sizeof(info.nv_name) - 1] = '\0';
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
#endif

int32 get_cust_conf_string_etc(int32 tag_index, int8 * puc_var, int8* puc_value, uint32 size)
{
    OAL_MEMZERO(puc_value, size);
    return ini_find_var_value_etc(tag_index, puc_var, puc_value, size);
}


int32 set_cust_conf_string_etc(int32 tag_index, int8 * name, int8 * var)
{
    int32 ret = INI_FAILED;

    if (tag_index != CUST_MODU_NVRAM)
    {
        INI_ERROR("NOT SUPPORT MODU TO WRITE");
        return INI_FAILED;
    }
#ifdef HISI_NVRAM_SUPPORT
    ret = write_conf_to_nvram_etc(name, var);
#endif
    return ret;
}



int32 get_cust_conf_int32_etc(int32 tag_index, int8 * puc_var, int32* puc_value)
{
    int32 ret = 0;
    int8  out_str[INI_READ_VALUE_LEN] = {0};

    ret = ini_find_var_value_etc(tag_index, puc_var, out_str, sizeof(out_str));

    if (ret < 0)
    {
        /* ini_find_var_value_etc has error log, delete this log */
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


static int32 get_ini_file(int8 *file_path, INI_FILE **fp)
{
    if (NULL == file_path)
    {
        INI_INFO("para file_path is NULL\n");
        return INI_FAILED;
    }

    *fp = ini_file_open(file_path, "rt");
    if (NULL == *fp)
    {
        INI_INFO("inifile %s not exist\n", file_path);
        return INI_FAILED;
    }

    return INI_SUCC;
}


static int32 ini_file_check_timespec(INI_FILE *fp)
{
    if (NULL == fp)
    {
        INI_ERROR("para file is NULL\n");
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    if (NULL == fp->f_path.dentry)
    {
        INI_ERROR("file dentry is NULL\n");
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    if (g_ini_file_time_sec != INF_FILE_GET_CTIME(fp->f_path.dentry))
    {
        INI_INFO("ini_file time_secs changed from [%ld]to[%ld]\n", g_ini_file_time_sec, INF_FILE_GET_CTIME(fp->f_path.dentry));
        g_ini_file_time_sec = INF_FILE_GET_CTIME(fp->f_path.dentry);

        return INI_FILE_TIMESPEC_RECONFIG;
    }
    else
    {
        INI_INFO("ini file is not upadted time_secs[%ld]\n", g_ini_file_time_sec);
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }
}


int32 ini_file_check_conf_update(void)
{
    INI_FILE *fp = NULL;
    int32 ret;

    /* read spec if exist */
    if ((INI_SUCC == get_ini_file(g_ini_conn_file_name_etc, &fp)) && (INI_FILE_TIMESPEC_RECONFIG == ini_file_check_timespec(fp)))
    {
        INI_INFO("%s ini file is updated\n", g_ini_conn_file_name_etc);
        ret = INI_FILE_TIMESPEC_RECONFIG;
    }
    else if ((INI_SUCC == get_ini_file(INI_FILE_PATH, &fp)) && (INI_FILE_TIMESPEC_RECONFIG == ini_file_check_timespec(fp)))
    {
        INI_INFO("%s ini file is updated\n", INI_FILE_PATH);
        ret = INI_FILE_TIMESPEC_RECONFIG;
    }
    else
    {
        INI_INFO("no ini file is updated\n");
        ret = INI_FILE_TIMESPEC_UNRECONFIG;
    }

    if (NULL != fp)
    {
        ini_file_close(fp);
    }

    return ret;
}

#ifdef HISI_DTS_SUPPORT
int32 get_ini_file_name_from_dts_etc(int8 *dts_prop, int8 *prop_value, uint32 size)
{
#ifdef _PRE_CONFIG_USE_DTS
    return g_board_info_etc.bd_ops.get_ini_file_name_from_dts_etc(dts_prop,prop_value,size);
#endif
    return INI_SUCC;
}
#endif

STATIC int32 bin_mem_check(int8 *pc_dest, int8 *pc_src, uint16 us_lenth)
{
    int16 loop;
    if (NULL == pc_dest || NULL == pc_src)
    {
        INI_ERROR("pointer is NULL!");
        return INI_FAILED;
    }

    if (0 == us_lenth)
    {
        return INI_SUCC;
    }

    for(loop = 0; loop < us_lenth;loop++)
    {
        if (pc_dest[loop] != pc_src[loop])
        {
            return INI_FAILED;
        }
    }
    return INI_SUCC;
}

int8 *get_str_from_file(int8 *pc_file_path, const int8 *pc_mask_str, const int8 *pc_target_str)
{
    INI_FILE *fp;
    int32 ret, loop, l_len;
    int8 ac_read_buf[INI_KERNEL_READ_LEN];
    int8 *pc_find_str = NULL;
    int8 *data;
    uint8 uc_str_check_len;
    INI_INFO("%s", __func__);

    if (unlikely(NULL == pc_file_path || NULL == pc_mask_str || NULL == pc_target_str))
    {
        INI_ERROR("arg is NULL!");
        return NULL;
    }

    fp = ini_file_open(pc_file_path, "rt");
    if (unlikely(NULL == fp))
    {
        INI_ERROR("open file %s fail!", pc_file_path);
        return NULL;
    }
    INI_INFO("open file %s success to find str \"%s\"!", pc_file_path, pc_mask_str);
    uc_str_check_len = OAL_STRLEN(pc_mask_str);
    /* 由于每次比较都会留uc_str_check_len不比较所以不是0 */
    while (uc_str_check_len != (ret =kernel_read(fp, fp->f_pos, ac_read_buf, INI_KERNEL_READ_LEN)))
    {
        for (loop = 0; loop < INI_KERNEL_READ_LEN-uc_str_check_len; loop++)
        {
            /* 判断首尾减少bin_mem_check调用次数 */
            if (pc_mask_str[0] == ac_read_buf[loop]
                    && pc_mask_str[uc_str_check_len-1] == ac_read_buf[loop+uc_str_check_len-1]
                    && !(bin_mem_check(&ac_read_buf[loop], (int8 *)pc_mask_str, uc_str_check_len)))
            {
                fp->f_pos += loop;
                INI_INFO("find device sw version file local = %ld ",(long)fp->f_pos);
                oal_memset(ac_read_buf, 0, INI_KERNEL_READ_LEN);
                /* 读取到‘\n’或者最大192B数据到ac_read_buf */
                ko_read_line(fp, ac_read_buf);
                /* 定位所需字符串位置 */
                pc_find_str = OAL_STRSTR(ac_read_buf, pc_target_str);
                if (NULL == pc_find_str)
                {
                    fp->f_pos += uc_str_check_len;
                    continue ;
                }
                l_len = OAL_STRLEN(pc_find_str);
                data = (int8 *)kmalloc(l_len+1, GFP_KERNEL);
                if (unlikely(NULL == data))
                {
                    INI_ERROR("find device sw version:%s, but memory alloc fail!", pc_find_str);
                    ini_file_close(fp);
                    return NULL;
                }
                oal_memcopy(data, pc_find_str, l_len+1);
                ini_file_close(fp);
                INI_INFO("find device sw version:%s", pc_find_str);
                return data;
            }
        }
        fp->f_pos += (ret-uc_str_check_len);
    }
    ini_file_close(fp);
    INI_ERROR("find no data to device sw version in %s!", pc_file_path);
    return NULL;
}

int ini_cfg_init_etc(void)
{
#ifdef HISI_DTS_SUPPORT
    int32 ret;
    int8 auc_dts_ini_path[INI_FILE_PATH_LEN]  = {0};
#endif


#ifndef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    INI_INFO("hi110x ini config search init!\n");
#endif

#ifdef HISI_DTS_SUPPORT
    ret = get_ini_file_name_from_dts_etc(PROC_NAME_INI_FILE_NAME, auc_dts_ini_path, sizeof(auc_dts_ini_path));
    if ( 0 > ret )
    {
        INI_ERROR("can't find dts proc %s\n", PROC_NAME_INI_FILE_NAME);
        return INI_FAILED;
    }
#endif
    INI_INIT_MUTEX(&file_mutex_etc);

#ifdef HISI_DTS_SUPPORT
    snprintf(g_ini_file_name_etc, sizeof(g_ini_file_name_etc)-1, "%s", auc_dts_ini_path);
    g_board_info_etc.ini_file_name=g_ini_file_name_etc;
    /*Note:"symbol snprintf()"has arg.count conflict(5 vs 4)*/
    /*lint -e515*/
#endif
    snprintf(g_ini_conn_file_name_etc, sizeof(g_ini_conn_file_name_etc)-1, "%s", CUST_PATH_INI_CONN);
    OAL_IO_PRINT("ini_file_name@%s\n",g_ini_file_name_etc);

#ifdef HISI_DTS_SUPPORT
    /*lint +e515*/
    INI_INFO("%s@%s\n", PROC_NAME_INI_FILE_NAME, g_ini_file_name_etc);
#else
    INI_INFO("ini_file_name@%s\n",g_ini_file_name_etc);
#endif
    return INI_SUCC;
}

void ini_cfg_exit_etc(void)
{
    INI_INFO("hi110x ini config search exit!\n");
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/*lint -e578*//*lint -e132*//*lint -e745*//*lint -e101*//*lint -e49*//*lint -e601*/
oal_module_param_string(g_ini_file_name_etc, g_ini_file_name_etc, INI_FILE_PATH_LEN, OAL_S_IRUGO);
/*lint +e578*//*lint +e132*//*lint +e745*//*lint +e101*//*lint +e49*//*lint +e601*/
#endif

//module_init(ini_cfg_init_etc);
//module_exit(ini_cfg_exit_etc);
//MODULE_AUTHOR("Hisilicon Connectivity ini config search Driver Group");
//MODULE_DESCRIPTION("Hi110x ini config search platform driver");
//MODULE_LICENSE("GPL");

