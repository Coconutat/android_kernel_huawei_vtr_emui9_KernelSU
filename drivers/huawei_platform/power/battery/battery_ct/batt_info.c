#include <huawei_platform/power/batt_info.h>
#include <huawei_platform/power/power_dsm.h>
#include <huawei_platform/power/huawei_charger.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>

#ifdef  HWLOG_TAG
#undef  HWLOG_TAG
#endif

#define HWLOG_TAG BATT_INFO
HWLOG_REGIST();

/*
 *Driver Limitations:
 *  Support 32 and 64 bits process only (u32 == unsigned int)
 *  Support ds28el15 only: only this IC spportted now
 */


/* battery certification operations */
static batt_ct_ops bco = {
    .get_ic_type        = NULL,
    .get_ic_id          = NULL,
    .get_batt_sn        = NULL,
    .check_ic_status    = NULL,
    .certification      = NULL,
    .get_ct_src         = NULL,
    .set_batt_safe_info = NULL,
    .get_batt_safe_info = NULL,
};

/* battery check result */
static battery_check_result batt_chk_rlt = {
    .ic_status          = IC_FAIL_UNKOWN,
    .key_status         = KEY_UNREADY,
    .sn_status          = SN_UNREADY,
#ifdef BATTBD_FORCE_MATCH
    .check_mode         = FACTORY_CHECK_MODE,
#else
    .check_mode         = COMMERCIAL_CHECK_MODE,
#endif

};

/* binding information on board */
static binding_info bbinfo = {
    .version = ILLEGAL_BIND_VERSION,
    .info = {0},
};

#ifdef CONFIG_HUAWEI_DSM
static const int battery_detect_err_count[] = {
    [DMD_INVALID]        = 0,
    [DMD_ROM_ID_ERROR]   = DSM_BATTERY_ROM_ID_CERTIFICATION_FAIL,
    [DMD_IC_STATE_ERROR] = DSM_BATTERY_IC_EEPROM_STATE_ERROR,
    [DMD_IC_KEY_ERROR]   = DSM_BATTERY_IC_KEY_CERTIFICATION_FAIL,
    [DMD_OO_UNMATCH]     = DSM_OLD_BOARD_AND_OLD_BATTERY_UNMATCH,
    [DMD_OBD_UNMATCH]    = DSM_OLD_BOARD_AND_NEW_BATTERY_UNMATCH,
    [DMD_OBT_UNMATCH]    = DSM_NEW_BOARD_AND_OLD_BATTERY_UNMATCH,
    [DMD_NV_ERROR]       = DSM_BATTERY_NV_DATA_READ_FAIL,
    [DMD_SERVICE_ERROR]  = DSM_CERTIFICATION_SERVICE_IS_NOT_RESPONDING,
};
static const char *battery_detect_err_str[] = {
    [DMD_INVALID]        = "",
    [DMD_ROM_ID_ERROR]   = "DSM_BATTERY_ROM_ID_CERTIFICATION_FAIL",
    [DMD_IC_STATE_ERROR] = "DSM_BATTERY_IC_EEPROM_STATE_ERROR",
    [DMD_IC_KEY_ERROR]   = "DSM_BATTERY_IC_KEY_CERTIFICATION_FAIL",
    [DMD_OO_UNMATCH]     = "DSM_OLD_BOARD_AND_OLD_BATTERY_UNMATCH",
    [DMD_OBD_UNMATCH]    = "DSM_OLD_BOARD_AND_NEW_BATTERY_UNMATCH",
    [DMD_OBT_UNMATCH]    = "DSM_NEW_BOARD_AND_OLD_BATTERY_UNMATCH",
    [DMD_NV_ERROR]       = "DSM_BATTERY_NV_DATA_READ_FAIL",
    [DMD_SERVICE_ERROR]  = "DSM_CERTIFICATION_SERVICE_IS_NOT_RESPONDING",
};
#endif

/* board free runtime limitation (min) */
static int free_runtime = -1;
static int runtime_step = -1;

/* record board runtime (min) */
static int board_runtime = -1;

/* record board runtime (min) */
static unsigned int free_cycles = -1;

/* record battery rematch ability on booting */
static enum BATT_MATCH_TYPE batt_rematch_abi_onboot = 0;

/* all delayed work declear */
static struct delayed_work check_last_result_dw;
static struct delayed_work send_src_to_srv_dw;
static struct delayed_work dmd_report_dw;

static int check_last_result_retry;
static int send_src_to_srv_retry;
static int dmd_report_retry;

/* DMD needs to report */
static int dmd_no[BATT_INFO_DMD_GROUPS] = {0};

/* set it when checking finished */
static int do_action = 0;

/* lock for key verify */
struct mutex key_lock;
static int key_ct_ready;

/*
 * All possible certification driver should be add to this head
 * for battery information scheduling and validation testing.
 */
LIST_HEAD(batt_ct_head);
/*-----------------------------------------------------------------------------------------------*/
/*-------------------------------------boot mode parse start-------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
enum phone_work_mode_t {
    NORMAL_MODE = 0,
    CHARGER_MODE,
    RECOVERY_MODE,
    ERECOVERY_MODE,
};

const static char *work_mode_str[] = {
    [NORMAL_MODE]    = "NORMAL",
    [CHARGER_MODE]   = "CHARGER",
    [RECOVERY_MODE]  = "RECOVERY",
    [ERECOVERY_MODE] = "ERECOVERY",
};

static enum phone_work_mode_t work_mode = NORMAL_MODE;

static void update_work_mode(void)
{
    if(strstr(saved_command_line, "androidboot.mode=charger")) {
        work_mode = CHARGER_MODE;
        goto print_work_mode;
    }
    if(strstr(saved_command_line, "enter_erecovery=1")) {
        work_mode = ERECOVERY_MODE;
        goto print_work_mode;
    }
    if(strstr(saved_command_line, "enter_recovery=1")) {
        work_mode = RECOVERY_MODE;
        goto print_work_mode;
    }
    work_mode = NORMAL_MODE;

print_work_mode:
    hwlog_info("work mode is %s.\n", work_mode_str[work_mode]);
}
/*-----------------------------------------------------------------------------------------------*/
/*--------------------------------------boot mode parse end--------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------*/
/*-------------------------------------Generic netlink start-------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
/* BATT_SERVICE_ON handler */
static int batt_srv_on_cb(void);
static void binding_check(void);
static void check_result(void);

/* BATT_CERTIFICAION_MAC handler */
static int batt_mac_mesg_cb(unsigned char version, void * data, int len)
{
    /* certification MAC */
    hwlog_info("Battery-driver:Certification running!\n");
    mutex_lock(&key_lock);
    if(key_ct_ready) {
        mutex_unlock(&key_lock);
        return 0;
    } else {
        key_ct_ready = 1;
    }
    mutex_unlock(&key_lock);
    bco.certification(data, len, &batt_chk_rlt.key_status, MAC_RESOURCE_TPYE1);
    if(batt_chk_rlt.key_status == KEY_PASS) {
        hwlog_info("key certification success.\n");
        /* checking binding info if key is right, this is last step */
        binding_check();
    } else {
        hwlog_info("key certification failed(%u).\n", batt_chk_rlt.key_status);
        check_result();
    }

    return 0;
}

static int batt_shutdown_cb(unsigned char version, void * data, int len)
{
    hwlog_info("battery shutdown call back called!\n");
    if(len != strlen(BATT_SHUTDOWN_MAGIC2)) {
        return -1;
    }
    if(memcmp(BATT_SHUTDOWN_MAGIC2, data, len)) {
        return -1;
    }
    return 0;
}

#define BATT_INFO_NL_OPS_NUM    2

static const easy_cbs_t batt_ops[BATT_INFO_NL_OPS_NUM] = {
    {
     .cmd = BATT_MAXIM_SECRET_MAC,
     .doit = batt_mac_mesg_cb,
    },
    {
     .cmd = BATT_SHUTDOWN_CMD,
     .doit = batt_shutdown_cb,
    }
};

static power_mesg_node_t batt_info_node = {
    .target = POWERCT_PORT,
    .name = "BATT_INFO",
    .ops = batt_ops,
    .n_ops = BATT_INFO_NL_OPS_NUM,
    .srv_on_cb = batt_srv_on_cb,
};

/* BATT_SERVICE_ON handler */
static int batt_srv_on_cb(void)
{
    if(work_mode == NORMAL_MODE || work_mode == CHARGER_MODE) {
        schedule_delayed_work(&check_last_result_dw,0);
    }
    return 0;
}
/*-----------------------------------------------------------------------------------------------*/
/*--------------------------------------Generic netlink end--------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------*/
/*-------------------------------------NVME operations start-------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
static int is_new_board(int rt)
{
    if(free_runtime == -1) {
        return true;
    }
    return rt < free_runtime;
}

static int is_new_battery(void)
{
    return batt_rematch_abi_onboot == BATTERY_REMATCHABLE;
}

static int get_binding_id(void)
{
    int ret;
    struct hisi_nve_info_user nvinfo;

    nvinfo.nv_operation = NV_READ;
    nvinfo.nv_number = BBINFO_NV_NUMBER;
    strncpy(nvinfo.nv_name, BBINFO_NV_NAME, NV_NAME_LENGTH);
    nvinfo.valid_size = sizeof(binding_info);
    if(nvinfo.valid_size > NVE_NV_DATA_SIZE) {
        hwlog_err("%s: binding_info is too long to write or read from a single NV.\n", __func__);
        return BATTERY_DRIVER_FAIL;
    }
    ret = hisi_nve_direct_access(&nvinfo);
    if(ret) {
        hwlog_err("Read BBINFO NV failed in %s, error code %d.\n", __func__, ret);
        return BATTERY_DRIVER_FAIL;
    }
    memcpy(&bbinfo, nvinfo.nv_data, nvinfo.valid_size);
    return BATTERY_DRIVER_SUCCESS;
}

static int check_binding_id(void)
{
    int ret, i;
    const unsigned char *sn;
    unsigned int sn_len;
    char temp[MAX_SN_LEN] = {0};

    ret = bco.get_batt_sn(&sn, &sn_len, NULL);

    if(ret || !sn) {
        hwlog_err("Get sn from ic failed in %s.\n", __func__);
        batt_chk_rlt.sn_status = SN_FAIL_IC_TIMEOUT;
        return BATTERY_DRIVER_SUCCESS;
    }

    if(sn_len <= 0 || sn_len > MAX_SN_LEN) {
        hwlog_err("sn_len illegal found in %s.\n", __func__);
        batt_chk_rlt.sn_status = SN_FAIL_IC_TIMEOUT;
        return BATTERY_DRIVER_FAIL;
    }

    switch (bbinfo.version) {
    case RAW_BIND_VERSION:
        for(i = 0; i < MAX_SN_BUFF_LENGTH; i++) {
            if(!memcmp(bbinfo.info[i], sn, sn_len)) {
                batt_chk_rlt.sn_status = SN_PASS;
                hwlog_info("board and battery are well matched.\n");
                if(i > 0) {
                    memcpy(temp, bbinfo.info[i], MAX_SN_LEN);
                    for( ; i > 0; i--) {
                        memcpy(bbinfo.info[i], bbinfo.info[i - 1], MAX_SN_LEN);
                    }
                    memcpy(bbinfo.info[0], temp, MAX_SN_LEN);
                }
                return BATTERY_DRIVER_SUCCESS;
            }
        }
        break;
    default:
        break;
    }

    if(board_runtime < 0) {
        hwlog_err("Get board runtime fail(%d) in %s.\n", board_runtime, __func__);
        return BATTERY_DRIVER_FAIL;
    }

    if(!is_new_battery() && is_new_board(board_runtime)) {
        batt_chk_rlt.sn_status = SN_OBT_REMATCH;
    } else if(!is_new_board(board_runtime) && is_new_battery()) {
        batt_chk_rlt.sn_status = SN_OBD_REMATCH;
    } else if(is_new_battery() && is_new_board(board_runtime)) {
        batt_chk_rlt.sn_status = SN_NN_REMATCH;
    } else {
        batt_chk_rlt.sn_status = SN_OO_UNMATCH;
    }

    bbinfo.version = RAW_BIND_VERSION;
#ifdef BATTBD_FORCE_MATCH
    for(i = 0; i < MAX_SN_BUFF_LENGTH; i++) {
        memcpy(bbinfo.info[i], sn, sn_len);
    }
#else
    if (batt_chk_rlt.sn_status == SN_OBT_REMATCH || batt_chk_rlt.sn_status == SN_OBD_REMATCH ||
        batt_chk_rlt.sn_status == SN_NN_REMATCH) {
        for(i = MAX_SN_BUFF_LENGTH - 1; i > 0; i--) {
            memcpy(bbinfo.info[i], bbinfo.info[i - 1], MAX_SN_LEN);
        }
        memcpy(bbinfo.info[0], sn, sn_len);
    }
#endif

    return BATTERY_DRIVER_SUCCESS;
}

/*
 * nv operation in this function not retry
 * if the result is same as before, the function return directly without write operation.
 */
static int get_check_result(battery_check_result *result)
{
    int ret;
    struct hisi_nve_info_user nvinfo;

    nvinfo.nv_operation = NV_READ;
    nvinfo.nv_number = BLIMSW_NV_NUMBER;
    strncpy(nvinfo.nv_name, BLIMSW_NV_NAME, NV_NAME_LENGTH);
    nvinfo.valid_size = NVE_NV_DATA_SIZE;
    ret = hisi_nve_direct_access(&nvinfo);
    if(ret) {
        hwlog_err("Read BLIMSW NV failed in %s, error code %d.\n", __func__, ret);
        return BATTERY_DRIVER_FAIL;
    }

    result->ic_status = nvinfo.nv_data[IC_STATUS_OFFSET];
    result->key_status = nvinfo.nv_data[KEY_STATUS_OFFSET];
    result->sn_status = nvinfo.nv_data[SN_STATUS_OFFSET];
    result->check_mode = nvinfo.nv_data[CHECK_MODE_OFFSET];

    return BATTERY_DRIVER_SUCCESS;
}
/*-----------------------------------------------------------------------------------------------*/
/*--------------------------------------NVME operations end--------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------*/
/*------------------------------------delayed work start-----------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
static int runtime_update(struct notifier_block *nb, unsigned long action, void *data)
{
    if(data) {
        board_runtime = *(int *)data;
        return NOTIFY_OK;
    }
    return NOTIFY_DONE;
}

static void binding_check(void)
{
    int ret;

    ret = get_binding_id();
    if(ret) {
        batt_chk_rlt.sn_status = SN_FAIL_NV_TIMEOUT;
        hwlog_err("get binding info from local failed.\n");
        check_result();
        return;
    }
    ret = check_binding_id();
    if(ret) {
        batt_chk_rlt.sn_status = SN_FAIL_NV_TIMEOUT;
        hwlog_err("get binding info from remote failed.\n");
        check_result();
        return;
    }
    hwlog_info("Checking binding infomation result is %d.\n", batt_chk_rlt.sn_status);
    check_result();
}

static void safe_action(int action)
{
    char *data;
    const unsigned char *sn;
    unsigned int sn_len;
    int data_len = strlen(BATT_SHUTDOWN_MAGIC1) + MAGIC_NUM_OFFSET + sizeof(bbinfo);

    data = kzalloc(data_len, GFP_KERNEL);
    if(!data) {
        hwlog_err("alloc for data failed in %s.\n", __func__);
        return ;
    }
    data[IC_STATUS_OFFSET] = batt_chk_rlt.ic_status;
    data[KEY_STATUS_OFFSET] = batt_chk_rlt.key_status;
    data[SN_STATUS_OFFSET] = batt_chk_rlt.sn_status;
    data[CHECK_MODE_OFFSET] = batt_chk_rlt.check_mode;

    if(action) {
        switch (work_mode) {
        case NORMAL_MODE:
            memcpy(data+MAGIC_NUM_OFFSET, BATT_SHUTDOWN_MAGIC1, strlen(BATT_SHUTDOWN_MAGIC1));
            break;
        case CHARGER_MODE:
            if(disable_chargers(0, 1, BATT_CERTIFICATION_TYPE)) {
                hwlog_err("stop all charge types failed.\n");
            }
            break;
        default:
            break;
        }
    }

    memcpy(data + BATT_SN_OFFSET, &bbinfo, sizeof(bbinfo));
    if(power_easy_send(&batt_info_node, BATT_SHUTDOWN_CMD, 0, data, data_len)) {
        hwlog_err("action mesg send failed in %s.\n", __func__);
    }
    kfree(data);
}

static int is_legal_status(const battery_check_result *result)
{
    if(result->ic_status == IC_PASS && result->key_status == KEY_PASS &&
       result->sn_status <= SN_NN_REMATCH) {
        return 1;
    }
    return 0;
}

static void safe_strategy(void)
{
    int retry = 0;

    if(work_mode == NORMAL_MODE) {
        schedule_delayed_work(&dmd_report_dw, 3*HZ);
    }

    safe_action(do_action);
}

/* this is the function checking battery information result and acting safe strategies */
static void check_result(void)
{
    hwlog_info("Checking battery certification result(key_status:%02x, ic_status:%02x, "
               "sn_status:%02x).\n", batt_chk_rlt.key_status, batt_chk_rlt.ic_status,
               batt_chk_rlt.sn_status);
    switch (batt_chk_rlt.ic_status)
    {
    case IC_FAIL_UNMATCH:
    case IC_FAIL_UNKOWN:
        dmd_no[IC_DMD_GROUP] = DMD_ROM_ID_ERROR;
        do_action = 1;
        break;
    case IC_FAIL_MEM_STATUS:
        dmd_no[IC_DMD_GROUP] = DMD_IC_STATE_ERROR;
        do_action = 1;
        break;
    case IC_PASS:
        break;
    default:
        hwlog_err("illegal IC checking result(%d).\n", batt_chk_rlt.ic_status);
        break;
    }

    if(do_action) {
        goto batt_info_cr_action;
    }

    switch(batt_chk_rlt.key_status) {
    case KEY_FAIL_TIMEOUT:
        dmd_no[KEY_DMD_GROUP] = DMD_SERVICE_ERROR;
        break;
    case KEY_FAIL_UNMATCH:
        dmd_no[KEY_DMD_GROUP] = DMD_IC_KEY_ERROR;
        do_action = 1;
        break;
    case KEY_PASS:
        break;
    default:
        hwlog_err("illegal KEY checking result(%d).\n", batt_chk_rlt.key_status);
        break;
    }

    if(do_action || batt_chk_rlt.key_status == KEY_FAIL_TIMEOUT ||
       batt_chk_rlt.key_status == KEY_FAIL_UNMATCH) {
        goto batt_info_cr_action;
    }

    switch (batt_chk_rlt.sn_status)
    {
    case SN_FAIL_NV_TIMEOUT:
        dmd_no[SN_DMD_GROUP] = DMD_NV_ERROR;
        break;
    case SN_FAIL_IC_TIMEOUT:
        dmd_no[IC_DMD_GROUP] = DMD_ROM_ID_ERROR;
        do_action = 1;
        break;
    case SN_OO_UNMATCH:
        dmd_no[SN_DMD_GROUP] = DMD_OO_UNMATCH;
        do_action = 1;
        break;
    case SN_OBD_REMATCH:
        dmd_no[SN_DMD_GROUP] = DMD_OBD_UNMATCH;
        break;
    case SN_OBT_REMATCH:
        dmd_no[SN_DMD_GROUP] = DMD_OBT_UNMATCH;
        break;
    case SN_NN_REMATCH:
        break;
    case SN_PASS:
        break;
    default:
        hwlog_err("illegal SN checking result(%d).\n", batt_chk_rlt.sn_status);
        break;
    }
batt_info_cr_action:
    safe_strategy();
}

static void send_src_to_srv(struct work_struct *work)
{
    mac_resource mac_res;

    mutex_lock(&key_lock);
    if(key_ct_ready) {
        mutex_unlock(&key_lock);
        return;
    }
    mutex_unlock(&key_lock);

    if(bco.get_ct_src(&mac_res, MAC_RESOURCE_TPYE1)) {
        hwlog_err("Get mac resource for certification failed in %s.\n", __func__);
        batt_chk_rlt.ic_status = IC_FAIL_UNKOWN;
        check_result();
        return ;
    }

    if(power_easy_send(&batt_info_node, BATT_MAXIM_SECRET_MAC, 0, mac_res.datum, mac_res.len)) {
        hwlog_err("Send data to server failed(%d).\n", send_src_to_srv_retry);
        send_src_to_srv_retry++;
        if(send_src_to_srv_retry < 3) {
            schedule_delayed_work(&send_src_to_srv_dw, 0);
            return ;
        }
    } else {
        send_src_to_srv_retry++;
        if(send_src_to_srv_retry < 3) {
            schedule_delayed_work(&send_src_to_srv_dw, msecs_to_jiffies(500));
            return ;
        }
    }
    batt_chk_rlt.key_status = KEY_FAIL_TIMEOUT;
    check_result();
    return ;
}

static void check_battery(void)
{
    hwlog_info("check battery now really start.\n");
    /* check ic status first */
    bco.check_ic_status(&batt_chk_rlt.ic_status);
    if(batt_chk_rlt.ic_status != IC_PASS) {
        hwlog_err("Battery IC status is illegal(%u).\n", batt_chk_rlt.ic_status);
        check_result();
        return ;
    }

    /* go to check key */
    schedule_delayed_work(&send_src_to_srv_dw, 0);
}

static void check_last_result(struct work_struct *work)
{
    battery_check_result result;
    int battery_removed;

    if(get_check_result(&result)) {
        hwlog_err("get last check result failed(%d).\n", check_last_result_retry);
        check_last_result_retry++;
        if(check_last_result_retry < 5) {
            schedule_delayed_work(&check_last_result_dw, msecs_to_jiffies(500));
            return ;
        }
    }
    hwlog_info("last check result: ic(%d), key(%d), sn(%d), mode(%d).\n", result.ic_status,
               result.key_status, result.sn_status, result.check_mode);
#ifdef BATTBD_FORCE_MATCH
    check_battery();
#else
    battery_removed = hisi_battery_removed_before_boot();
    if(battery_removed < 0) {
        hwlog_err("whether battery was removed between last boot and this boot is unknown.\n");
        battery_removed = 0;
    } else {
        hwlog_info("battery is %s removed.\n", battery_removed?"really":"not");
    }
    if(battery_removed || !is_legal_status(&result) || result.check_mode == FACTORY_CHECK_MODE) {
        check_battery();
    } else {
        memcpy(&batt_chk_rlt, &result, sizeof(battery_check_result));
    }
#endif
}
static void dmd_report(struct work_struct *work)
{
    int i;
    int ret;

    for(i = 0; i < BATT_INFO_DMD_GROUPS; i++) {
        if(dmd_no[i] != DMD_INVALID) {
            ret = power_dsm_dmd_report(POWER_DSM_BATTERY_DETECT, battery_detect_err_count[dmd_no[i]],
                                       (char *)battery_detect_err_str[dmd_no[i]]);
            dmd_no[i] = ret ? dmd_no[i] : DMD_INVALID;
        }
        if(dmd_no[i] != DMD_INVALID && dmd_report_retry < 20) {
            hwlog_info("%dth times dmd report failed.\n", dmd_report_retry);
            dmd_report_retry++;
            schedule_delayed_work(&dmd_report_dw, 3*HZ);
            return;
        }
    }

}
/*-----------------------------------------------------------------------------------------------*/
/*----------------------------------------work process end---------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------*/
/*----------------------------------------sys node start-----------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
static const char *ic_type2name[] = {
    [LOCAL_IC_TYPE]     = "INVALID_DEFAULT_IC",
    [MAXIM_SHA256_TYPE] = "DS28EL15",
};

static ssize_t ic_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int type;

    type = bco.get_ic_type();
    if(type < ARRAY_SIZE(ic_type2name) && type >= 0) {
        return snprintf(buf, PAGE_SIZE, "%s", ic_type2name[type]);
    } else {
        return snprintf(buf, PAGE_SIZE, "ERROR_TYPE");
    }
}

static ssize_t ic_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int i;
    int total_size = 0;
    const unsigned char *id;
    unsigned int id_len;
    int ret;

    ret = bco.get_ic_id(&id, &id_len);
    if(ret) {
        hwlog_err("Get ic id failed in %s.\n", __func__);
        return snprintf(buf, PAGE_SIZE, "ERROR:UNKOWN IC");
    }

    if(!id) {
        return snprintf(buf, PAGE_SIZE, "ERROR:NULL ID");
    }
    if(PAGE_SIZE <= (id_len * BATT_ID_PRINT_SIZE_PER_CHAR)) {
        return snprintf(buf, PAGE_SIZE, "ERROR:ID SIZE");
    }
    for(i = 0; i < id_len; i++) {
        total_size += snprintf(&(buf[total_size]), BATT_ID_PRINT_SIZE_PER_CHAR, "%02x", id[i]);
    }
    return total_size;
}

static ssize_t batt_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const unsigned char *sn;
    unsigned int sn_len;
    int ret;

    ret = bco.get_batt_sn(&sn, &sn_len, NULL);
    if(ret) {
        hwlog_err("Get battery SN failed in %s.\n", __func__);
        return snprintf(buf, PAGE_SIZE, "ERROR:UNKOWN IC");
    }
    if(!sn) {
        return snprintf(buf, PAGE_SIZE, "ERROR:NULL SN");
    }
    if(sn_len >= PAGE_SIZE) {
        return snprintf(buf, PAGE_SIZE, "SN is too long(%u) to print!", sn_len);
    }
    memcpy(buf, sn, sn_len);
    buf[sn_len] = 0;
    return sn_len;
}

static ssize_t batt_id_v_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned char sn_version;
    int ret;

    ret = bco.get_batt_sn(NULL, NULL, &sn_version);
    if(ret) {
        hwlog_err("Get battery SN failed in %s.\n", __func__);
        return snprintf(buf, PAGE_SIZE, "ERROR:UNKOWN IC");;
    }
    if(!sn_version) {
        return snprintf(buf, PAGE_SIZE, "ERROR:NULL SN");
    }
    return snprintf(buf, PAGE_SIZE, "%02x", sn_version);
}

static ssize_t ic_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d", batt_chk_rlt.ic_status);
}

static ssize_t key_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d", batt_chk_rlt.key_status);

}

static ssize_t sn_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d", batt_chk_rlt.sn_status);

}

static ssize_t official_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d", is_legal_status(&batt_chk_rlt));
}

static ssize_t check_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%02x", batt_chk_rlt.check_mode);
}

static ssize_t rt_lim_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d", free_runtime);
}

#ifdef BATTERY_LIMIT_DEBUG
static ssize_t bind_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    int i;
    (void) get_binding_id();
    for(i = 0; i < MAX_SN_BUFF_LENGTH; i++) {
        memcpy(buf + count, bbinfo.info[i], MAX_SN_LEN);
        count += MAX_SN_LEN;
        buf[count++] = '\n';
    }
    return count;
}

static ssize_t new_board_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%s", is_new_board(board_runtime)?"YES":"NO");
}

static ssize_t rematchable_onboot_show(struct device *dev,
                                                 struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d", batt_rematch_abi_onboot);
}

static ssize_t rematchable_battery_show(struct device *dev,
                                                   struct device_attribute *attr, char *buf)
{
    int ret;
    enum BATT_MATCH_TYPE temp;
    ret = bco.get_batt_safe_info(BATT_MATCH_ABILITY, &temp);
    if(ret) {
        return snprintf(buf, PAGE_SIZE, "%s", "Error");
    }
    return snprintf(buf, PAGE_SIZE, "%s", temp == BATTERY_REMATCHABLE ? "YES":"NO");
}

static ssize_t lock_battery_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret;
    enum BATT_MATCH_TYPE temp = BATTERY_UNREMATCHABLE;
    ret = bco.get_batt_safe_info(BATT_MATCH_ABILITY, &temp);
    if(ret) {
        return snprintf(buf, PAGE_SIZE, "%s", "Error");
    }
    return snprintf(buf, PAGE_SIZE, "%s", temp == BATTERY_UNREMATCHABLE?"Locked":"Unlocked");
}
static ssize_t lock_battery_store(struct device *dev, struct device_attribute *attr,
                                          const char *buf, size_t count)
{
    int ret;
    enum BATT_MATCH_TYPE temp = BATTERY_UNREMATCHABLE;
    if (count < 1 || buf[0] != '1')
    {
        return -1;
    }
    ret = bco.set_batt_safe_info(BATT_MATCH_ABILITY, &temp);
    if(ret) {
        return -1;
    }
    return count;
}

static ssize_t free_cycles_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d", free_cycles);
}
static ssize_t free_cycles_store(struct device *dev, struct device_attribute *attr,
                                        const char *buf, size_t count)
{
    if(buf[count - 1] == '\n' && !kstrtoint(buf, 10, &free_cycles)) {
        return -1;
    }
    return count;
}

static ssize_t free_runtime_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d", free_runtime);
}
static ssize_t free_runtime_store(struct device *dev, struct device_attribute *attr,
                                        const char *buf, size_t count)
{
    if(buf[count - 1] == '\n' && !kstrtoint(buf, 10, &free_runtime)) {
        return -1;
    }
    return count;
}

static ssize_t check_result_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d %d %d", batt_chk_rlt.ic_status, batt_chk_rlt.key_status,
                    batt_chk_rlt.sn_status);
}
static ssize_t check_result_store(struct device *dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    char *sub, *cur;
    int temp0, temp1, temp2;
    char str[64] = {0};
    size_t len;

    len = min_t(size_t, sizeof(str) - 1, count);
    memcpy(str, buf, len);
    cur = &str[0];
    sub = strsep(&cur, " ");
    if( !sub || kstrtoint(sub, 0, &temp0)) {
        return -1;
    }
    sub = strsep(&cur, " ");
    if( !sub || kstrtoint(sub, 0, &temp1)) {
        return -1;
    }
    if( !cur || kstrtoint(cur, 0, &temp2)) {
        return -1;
    }
    batt_chk_rlt.ic_status = temp0;
    batt_chk_rlt.key_status = temp1;
    batt_chk_rlt.sn_status = temp2;
    return count;
}
static ssize_t check_battery_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    key_ct_ready = 0;
    check_battery();
    return snprintf(buf, PAGE_SIZE, "check battery start...");
}
#endif

const static DEVICE_ATTR_RO(ic_type);
const static DEVICE_ATTR_RO(ic_id);
const static DEVICE_ATTR_RO(ic_status);
const static DEVICE_ATTR_RO(key_status);
const static DEVICE_ATTR_RO(sn_status);
const static DEVICE_ATTR_RO(batt_id);
const static DEVICE_ATTR_RO(batt_id_v);
const static DEVICE_ATTR_RO(official);
const static DEVICE_ATTR_RO(check_mode);
const static DEVICE_ATTR_RO(rt_lim);
#ifdef BATTERY_LIMIT_DEBUG
const static DEVICE_ATTR_RO(bind_info);
const static DEVICE_ATTR_RO(new_board);
const static DEVICE_ATTR_RO(rematchable_onboot);
const static DEVICE_ATTR_RO(rematchable_battery);
const static DEVICE_ATTR_RW(lock_battery);
const static DEVICE_ATTR_RW(free_runtime);
const static DEVICE_ATTR_RW(free_cycles);
const static DEVICE_ATTR_RW(check_result);
const static DEVICE_ATTR_RO(check_battery);
#endif


static const struct attribute *batt_info_attrs[] = {
    &dev_attr_ic_type.attr,
    &dev_attr_ic_id.attr,
    &dev_attr_ic_status.attr,
    &dev_attr_key_status.attr,
    &dev_attr_sn_status.attr,
    &dev_attr_batt_id.attr,
    &dev_attr_batt_id_v.attr,
    &dev_attr_official.attr,
    &dev_attr_check_mode.attr,
    &dev_attr_rt_lim.attr,
#ifdef BATTERY_LIMIT_DEBUG
    &dev_attr_bind_info.attr,
    &dev_attr_new_board.attr,
    &dev_attr_rematchable_onboot.attr,
    &dev_attr_rematchable_battery.attr,
    &dev_attr_lock_battery.attr,
    &dev_attr_free_runtime.attr,
    &dev_attr_free_cycles.attr,
    &dev_attr_check_result.attr,
    &dev_attr_check_battery.attr,
#endif
    NULL, /* sysfs_create_files need last one be NULL */
};

/* create all node needed by driver */
static int batt_node_create(struct platform_device *pdev)
{
    if(sysfs_create_files(&pdev->dev.kobj, batt_info_attrs)) {
        hwlog_err("Can't create all expected nodes under %s in %s.\n",
                  pdev->dev.kobj.name, __func__);
        return BATTERY_DRIVER_FAIL;
    }
    return BATTERY_DRIVER_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------sys node end------------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------*/
/*----------------------------------default ic operations start----------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
static batt_ic_type local_get_ic_type(void)
{
    return LOCAL_IC_TYPE;
}
static int local_get_ic_id(const unsigned char **id, unsigned int *id_len)
{
    return -1;
}

static int local_get_batt_sn(const unsigned char **sn, unsigned int *sn_len_bits,
                                     unsigned char *sn_version)
{
    return -1;
}

static int local_certification(void *data, int data_len, enum KEY_CR *result, int type)
{
    *result = KEY_FAIL_UNMATCH;
    return 0;
}

static int local_get_ct_src(mac_resource *mac_rsc, unsigned int type)
{
    return -1;
}

static int local_set_batt_safe_info(batt_safe_info_type type, void *value)
{
    return -1;
}

static int local_get_batt_safe_info(batt_safe_info_type type, void *value)
{
    return -1;
}

static int local_check_ic_status(enum IC_CR *result)
{
    *result = IC_FAIL_UNKOWN;
    return 0;
}

static void using_local_ct_ops(void)
{
    bco.get_ic_type        = local_get_ic_type;
    bco.get_ic_id          = local_get_ic_id;
    bco.get_batt_sn        = local_get_batt_sn;
    bco.check_ic_status    = local_check_ic_status;
    bco.certification      = local_certification;
    bco.get_ct_src         = local_get_ct_src;
    bco.set_batt_safe_info = local_set_batt_safe_info;
    bco.get_batt_safe_info = local_get_batt_safe_info;
}
/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------default ic operations end-----------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

/*
 * Battery ID Interface
 * success return 0 name[0] = 'A'~'Z' name[1] = 'A'~'Z'
 * fail return 1
 */
int get_battery_type(unsigned char *name) {
    const unsigned char *sn;
    int ret;
    if (!bco.get_batt_sn) {
        hwlog_err("bco.get_batt_sn in %s.\n", __func__);
        return BATTERY_DRIVER_FAIL;
    }
    ret = bco.get_batt_sn(&sn, NULL, NULL);
    if(ret) {
        hwlog_err("Get sn failed in %s.\n", __func__);
        return BATTERY_DRIVER_FAIL;
    }
    *name = sn[BATTERY_PACK_FACTORY];
    name++;
    *name = sn[BATTERY_CELL_FACTORY];
    return BATTERY_DRIVER_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------*/
/*----------------------------------------driver init start--------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
static int battery_charge_cycles_cb(struct notifier_block *nb, unsigned long action, void *data)
{
    int charge_cycles;
    int ret;
    static int lock_battery_task_done = 0;
    enum BATT_MATCH_TYPE temp = BATTERY_UNREMATCHABLE;
    if(action == HISI_EEPROM_CYC) {
        if(!data) {
            hwlog_err("Null data point found in %s.\n", __func__);
	    return NOTIFY_BAD;
        }
        /* coul-data:charger_cycles is 100*real charge cycles */
        charge_cycles = (*(unsigned *)data)/100;
        if(lock_battery_task_done == 0 && batt_rematch_abi_onboot == BATTERY_REMATCHABLE &&
           free_cycles <= charge_cycles) {
            ret = bco.set_batt_safe_info(BATT_MATCH_ABILITY, &temp);
            if(ret) {
                hwlog_err("Set battery to unrematchable failed in %s.\n", __func__);
            } else {
                lock_battery_task_done = 1;
            }
        }
        return NOTIFY_OK;
    }
    return NOTIFY_DONE;
}

static struct notifier_block battery_charge_cycles_listener = {
    .notifier_call = battery_charge_cycles_cb,
};

static int check_batt_ct_ops(void)
{
    int ret = BATTERY_DRIVER_SUCCESS;
    if(!bco.get_ic_id) {
        ret = BATTERY_DRIVER_FAIL;
        hwlog_err("battery information interface: get_ic_id is not valid!");
    }
    if(!bco.check_ic_status) {
        ret = BATTERY_DRIVER_FAIL;
        hwlog_err("battery information interface: check_ic_status is not valid!");
    }
    if(!bco.certification) {
        ret = BATTERY_DRIVER_FAIL;
        hwlog_err("battery information interface: certification is not valid!");
    }
    if(!bco.get_batt_sn) {
        ret = BATTERY_DRIVER_FAIL;
        hwlog_err("battery information interface: get_batt_sn is not valid!");
    }
    if(!bco.get_ct_src) {
        ret = BATTERY_DRIVER_FAIL;
        hwlog_err("battery information interface: get_ct_src is not valid!");
    }
    if(!bco.set_batt_safe_info) {
        ret = BATTERY_DRIVER_FAIL;
        hwlog_err("battery information interface: set_batt_safe_info is not valid!");
    }
    if(!bco.get_batt_safe_info) {
        ret = BATTERY_DRIVER_FAIL;
        hwlog_err("battery information interface: get_batt_safe_info is not valid!");
    }
    return ret;
}

static int batt_mesg_init(struct platform_device *pdev)
{
    int ret;

    ret = power_easy_node_register(&batt_info_node);
    if(ret) {
        hwlog_err("power_genl_add_op failed!\n");
        return ret;
    }

    return BATTERY_DRIVER_SUCCESS;
}

static int get_ic_ops(struct platform_device *pdev)
{
    int i;
    int ic_unready = -1;
    ct_ops_reg_list *pos;
    /* find ic first */
    for( i = 0; i < 5; i++) {
        list_for_each_entry(pos, &batt_ct_head, node) {
            if(pos->ct_ops_register != NULL) {
                ic_unready = pos->ct_ops_register(&bco);
            } else {
                ic_unready = BATTERY_DRIVER_FAIL;
            }
            if(!ic_unready) {
                hwlog_info("Find valid battery certification ic.\n");
                break;
            }
        }
        if(!ic_unready) {
            list_del_init(&pos->node);
            break;
        }
    }
	/* release useless memory allocated for ic */
    list_for_each_entry(pos, &batt_ct_head, node) {
        if(pos->ic_memory_release != NULL) {
            pos->ic_memory_release();
        }
    }

	if(ic_unready) {
        batt_chk_rlt.ic_status = IC_FAIL_UNKOWN;
        using_local_ct_ops();
        hwlog_info("None valid battery ic found in %s.\n", __func__);
    }

    /* check battery battery information interface */
    if(check_batt_ct_ops()) {
        batt_chk_rlt.ic_status = IC_FAIL_UNKOWN;
        using_local_ct_ops();
        hwlog_err("Using local default certification operations!/n");
    }
    return ic_unready;
}

static int board_runtime_init(struct platform_device *pdev)
{
    int ret;

    /* board runtime limitation  */
    ret = of_property_read_u32(pdev->dev.of_node, "free-runtime", &free_runtime);
    if(ret || free_runtime < 0) {
        hwlog_info("Using default board free minutes value(illegal value: %d).\n", free_runtime);
        board_runtime = 0;
        return -1;
    };
    ret = of_property_read_u32(pdev->dev.of_node, "runtime-step", &runtime_step);
    if(ret || runtime_step < 0) {
        hwlog_info("Using default board runtime step value(illegal value: %d).\n", runtime_step);
        board_runtime = 0;
        return -1;
    };
    return 0;
}

static void battery_cycles_limit_init(struct platform_device *pdev)
{
    int type;
    int ret;

    /* battery charge cycles limitation */
    ret = of_property_read_u32(pdev->dev.of_node, "free-cycles", &free_cycles);
    if(ret || free_cycles < 0) {
        hwlog_info("Using default battery free cycles value(illegal value: %d).\n", free_cycles);
        free_cycles = -1;
    };

    type = bco.get_ic_type();
    switch (type) {
    case MAXIM_SHA256_TYPE:
/* if not on factory mode */
#ifndef BATTBD_FORCE_MATCH
        ret = bco.get_batt_safe_info(BATT_MATCH_ABILITY, &batt_rematch_abi_onboot);
        if(ret) {
            hwlog_err("Get battery matchable status fail(%d) in %s.\n", ret, __func__);
            batt_rematch_abi_onboot = BATTERY_UNREMATCHABLE;
        }
        /* register notifier list for update battery to unrematchable */
        if(batt_rematch_abi_onboot == BATTERY_REMATCHABLE) {
            if(hisi_coul_register_blocking_notifier(&battery_charge_cycles_listener)) {
                hwlog_err("[%s]battery_charge_cycles_listener failed!\n", __func__);
            }
        }
#endif
        break;
    default:
        break;
    }
}

static int battery_driver_probe(struct platform_device *pdev)
{
    int ret = -1;
    char batt_vendor[3] = {0};
    int local_ic;
    struct notifier_block *runtime_nb;

    hwlog_info("Battery information driver is going to probing...\n");

    /* ic ops should get first */
    local_ic = get_ic_ops(pdev);
    /* try get battery vendor here */
    if(local_ic || get_battery_type(batt_vendor)) {
        hwlog_err("get_battery_type failed in %s(%s).\n", __func__, local_ic?"local ops":"dev ops");
    } else {
        hwlog_info("battery vendor is %s.\n", batt_vendor);
    }
    /* battery node initialization */
    if(batt_node_create(pdev)) {
        hwlog_err("battery information nodes create failed(%d) in %s.\n", ret, __func__);
    }
	/* find mode need battery checking */
    update_work_mode();
    /* under recovery mode no need further check */
    if(work_mode == RECOVERY_MODE || work_mode == ERECOVERY_MODE) {
        hwlog_info("Recovery mode not support now.\n");
        return BATTERY_DRIVER_SUCCESS;
    }
    INIT_DELAYED_WORK(&check_last_result_dw, check_last_result);
    INIT_DELAYED_WORK(&send_src_to_srv_dw, send_src_to_srv);
    INIT_DELAYED_WORK(&dmd_report_dw, dmd_report);
    check_last_result_retry = 0;
    send_src_to_srv_retry = 0;
    dmd_report_retry = 0;
    mutex_init(&key_lock);
    /* get borad runtime which distingush the old from the new */
    if(!board_runtime_init(pdev)) {
        runtime_nb = devm_kzalloc(&pdev->dev, sizeof(struct notifier_block), GFP_KERNEL);
        if(!runtime_nb) {
            hwlog_err("runtime_nb devm_kzalloc failed in %s.", __func__);
        } else {
            runtime_nb->notifier_call = runtime_update;
            if(board_runtime_register(runtime_step, free_runtime, runtime_nb)) {
                board_runtime = 0;
                hwlog_err("board_runtime_register failed in %s.", __func__);
            }
        }
    }
    /* get battery cycles which distingush the old from the new */
    battery_cycles_limit_init(pdev);
	/* now init mesg interface which is used to communicate with native server */
    if(batt_mesg_init(pdev)) {
        hwlog_err("general netlink initialize failed in %s.\n", __func__);
    }

    hwlog_info("Battery information driver was probed successfully.\n");
    return BATTERY_DRIVER_SUCCESS;
}

static int  battery_driver_remove(struct platform_device *pdev)
{
    return BATTERY_DRIVER_SUCCESS;
}

static struct of_device_id huawei_battery_match_table[] = {
    {
        .compatible = "huawei,battery-information",
    },
    { /*end*/},
};

static struct platform_driver huawei_battery_driver = {
    .probe      = battery_driver_probe,
    .remove     = battery_driver_remove,
    .driver     = {
        .name = "huawei_battery",
        .owner = THIS_MODULE,
        .of_match_table = huawei_battery_match_table,
    },
};

int __init battery_driver_init(void)
{
    hwlog_info("battery information driver init...\n");
    return platform_driver_register(&huawei_battery_driver);
}

void __exit battery_driver_exit(void)
{
    hwlog_info("battery information driver exit...\n");
    platform_driver_unregister(&huawei_battery_driver);
}

subsys_initcall_sync(battery_driver_init);
module_exit(battery_driver_exit);
/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------driver init end---------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

MODULE_LICENSE("GPL");
