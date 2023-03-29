/*
 * =====================================================================================
 *
 *       Filename:  bfm_hisi_dmd_info.c
 *
 *    Description:  get dmd info on hisi platform
 *
 *        Version:  1.0
 *        Created:  09/22/2016 11:16:02 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  
 *        Company:  Huawei OS
 *
 * =====================================================================================
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/list.h>

#include <log/log_usertype/log-usertype.h>

#define ERROR_NO "[Error no]:"

#define MAXSIZES 256
#define ERR_NO_SIZE  32
#define ERR_OCCUR_TIME  3
#define ERR_NUM_SIZE 12


#define B_DMD_PATH_0 "data/log/dmd_log/dmd_record_0.txt"
#define B_DMD_PATH_1 "data/log/dmd_log/dmd_record_1.txt"
#define DMD_PATH_0 "splash2/dmd_log/dmd_record_0.txt"
#define DMD_PATH_1 "splash2/dmd_log/dmd_record_1.txt"

struct list_head* readers;

struct DmdErrNum{
    char modName[ERR_NO_SIZE]; //  relative module name
    char errNum[ERR_NUM_SIZE];
};

typedef struct DmdErrInfo
{
    struct DmdErrNum modInfo;
    uint32_t errNum;                  //  error number
    uint32_t errCount;                //  error occur times
    struct list_head list;
} DmdErrInfo_t;

/*key-value entries: {relative module name, error number}*/
static struct DmdErrNum dmd_info_table[] = {
    {"BATTERY",        "920001003"},
    {"BATTERY",        "920001004"},
    {"BATTERY",        "920001010"},
    {"BATTERY",        "920001016"},
    {"BATTERY",        "920001017"},
    {"BATTERY",        "920001018"},
    {"BATTERY",        "920001019"},
    {"SMPL",             "920003000"},
    {"USCP",             "920004000"},
    {"USCP",             "920004001"},
    {"PMU_IRQ",        "920005000"},
    {"PMU_IRQ",        "920005001"},
    {"PMU_IRQ",        "920005002"},
    {"PMU_IRQ",        "920005003"},
    {"PMU_IRQ",        "920005004"},
    {"PMU_IRQ",        "920005005"},
    {"PMU_IRQ",        "920005006"},
    {"PMU_IRQ",        "920005007"},
    {"PMU_IRQ",        "920005008"},
    {"PMU_IRQ",        "920005011"},
    {"PMU_IRQ",        "920005012"},
    {"PMU_IRQ",        "920005013"},
    {"PMU_COUL",       "920006000"},
    {"PMU_OCP",        "920007000"},
    {"PMU_OCP",        "920007001"},
    {"PMU_OCP",        "920007002"},
    {"PMU_OCP",        "920007003"},
    {"PMU_OCP",        "920007004"},
    {"PMU_OCP",        "920007005"},
    {"PMU_OCP",        "920007006"},
    {"PMU_OCP",        "920007007"},
    {"PMU_OCP",        "920007008"},
    {"PMU_OCP",        "920007009"},
    {"PMU_OCP",        "920007010"},
    {"PMU_OCP",        "920007011"},
    {"PMU_OCP",        "920007012"},
    {"PMU_OCP",        "920007013"},
    {"PMU_OCP",        "920007014"},
    {"PMU_OCP",        "920007015"},
    {"PMU_OCP",        "920007016"},
    {"PMU_OCP",        "920007017"},
    {"PMU_OCP",        "920007018"},
    {"PMU_OCP",        "920007019"},
    {"PMU_OCP",        "920007020"},
    {"PMU_OCP",        "920007021"},
    {"PMU_OCP",        "920007022"},
    {"PMU_OCP",        "920007023"},
    {"PMU_OCP",        "920007024"},
    {"PMU_OCP",        "920007025"},
    {"PMU_OCP",        "920007026"},
    {"PMU_OCP",        "920007027"},
    {"PMU_OCP",        "920007028"},
    {"PMU_OCP",        "920007029"},
    {"PMU_OCP",        "920007030"},
    {"PMU_OCP",        "920007031"},
    {"PMU_OCP",        "920007032"},
    {"PMU_OCP",        "920007033"},
    {"PMU_OCP",        "920007034"},
    {"PMU_OCP",        "920007035"},
    {"PMU_OCP",        "920007036"},
    {"PMU_OCP",        "920007037"},
    {"PMU_OCP",        "920007038"},
    {"PMU_OCP",        "920007039"},
    {"PMU_OCP",        "920007040"},
    {"PMU_OCP",        "920007041"},
    {"PMU_OCP",        "920007042"},
    {"PMU_OCP",        "920007043"},
    {"PMU_OCP",        "920007044"},
    {"PMU_OCP",        "920007045"},
    {"PMU_OCP",        "920007046"},
    {"PMU_OCP",        "920007047"},
    {"PMU_OCP",        "920007048"},
    {"PMU_OCP",        "920007049"},
    {"CHARGE_MONITOR", "920008000"},
    {"CHARGE_MONITOR", "920008008"},
    {"LCD",            "922001001"},
    {"LCD",            "922001002"},
    {"LCD",            "922001003"},
    {"LCD",            "922001005"},
    {"LCD",            "922001007"},
    {"LCD",            "922001008"},
    {"KEY",            "926003000"},
    {"KEY",            "926003001"},
    {"EMMC",           "928002001"},
    {"EMMC",           "928002002"},
    {"EMMC",           "928002004"},
    {"EMMC",           "928002005"},
    {"EMMC",           "928002006"},
    {"EMMC",           "928002007"},
    {"EMMC",           "928002008"},
    {"EMMC",           "928002009"},
    {"EMMC",           "928002010"},
    {"EMMC",           "928002011"},
    {"EMMC",           "928002012"},
    {"EMMC",           "928002013"},
    {"EMMC",           "928002014"},
    {"EMMC",           "928002015"},
    {"EMMC",           "928002020"},
    {"EMMC",           "928002021"},
    {"EMMC",           "928002022"},
    {"EMMC",           "928002023"},
    {"EMMC",           "928002025"}
};

#ifndef FCLOSE_FP
#define FCLOSE_FP(fp) do{ \
    if(NULL != fp) { \
        filp_close(fp, NULL); \
        fp= NULL; \
    } \
}while(0)
#endif

//get index in the table by errNum
static int find_index_by_err_no(char* pErrNum) {
    int i = 0;
    int maxLenth = sizeof(dmd_info_table)/sizeof(struct DmdErrNum);

    if (NULL == pErrNum) {
        return -1;
    }

    for (i = 0; i < maxLenth; i++) {
        if (0 == strncmp(dmd_info_table[i].errNum, pErrNum, strlen(dmd_info_table[i].errNum))) {
            return i;
        }
    }
    return -1;
}

static int get_the_max_err (struct list_head* plistHead, DmdErrInfo_t* pMaxErrInfo)
{
    struct DmdErrInfo* curDmdErrInfo = NULL;
    struct DmdErrInfo* nextDmdErrInfo = NULL;
    uint32_t maxCount = 0;

    //find error node in the list, with the max count.
    if(NULL == pMaxErrInfo) {
        printk("%s():%d:pMaxErrInfo is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(plistHead->next == plistHead) {
        printk("%s():%d:list is empty\n", __func__, __LINE__);
        return -1;
    }

    list_for_each_entry_safe(curDmdErrInfo, nextDmdErrInfo, plistHead, list) {
        int index = 0;

        if(NULL == curDmdErrInfo) {
            break;
        }

        index = find_index_by_err_no(curDmdErrInfo->modInfo.errNum);
        if(-1 != index) {
            if(curDmdErrInfo->errCount > maxCount) {
                strncpy(pMaxErrInfo->modInfo.modName, dmd_info_table[index].modName, ERR_NO_SIZE-1);
                pMaxErrInfo->errNum = curDmdErrInfo->errNum;
                pMaxErrInfo->errCount = curDmdErrInfo->errCount;
                maxCount = curDmdErrInfo->errCount;
            }
        }
        list_del(&curDmdErrInfo->list);
        kfree(curDmdErrInfo);
    }
    return 0;  
}

static void create_new_list_node(char* pErrNum, struct list_head* plistHead) {
    DmdErrInfo_t* pDmdErrInfo = NULL;

    pDmdErrInfo = (DmdErrInfo_t*)kmalloc(sizeof(DmdErrInfo_t), GFP_KERNEL);
    if (pDmdErrInfo) {
        memset(pDmdErrInfo, 0, sizeof(DmdErrInfo_t));
        strncpy(pDmdErrInfo->modInfo.errNum, pErrNum, ERR_NUM_SIZE-1);
        pDmdErrInfo->errNum = (uint32_t)simple_strtoul(pDmdErrInfo->modInfo.errNum, NULL, 10);
        pDmdErrInfo->errCount = 1;
        list_add(&pDmdErrInfo->list, plistHead);
    }
    else {
        printk("%s():%d:can not vmalloc.\n", __func__, __LINE__);
    }
}

static void check_list_for_each (char* pErrNum, struct list_head* plistHead)
{
    //if the pErrNum is in the list,count++. Else, add it to the list. 
    int isExisted = 0;

    if(list_empty(plistHead)) {
        create_new_list_node(pErrNum,plistHead);
        return;
    } else {
        struct DmdErrInfo* curDmdErrInfo = NULL;

        list_for_each_entry(curDmdErrInfo, plistHead, list) {
            if (0 == strncmp(curDmdErrInfo->modInfo.errNum, pErrNum, strlen(curDmdErrInfo->modInfo.errNum))) {
                curDmdErrInfo->errCount++;
                isExisted = 1;
                break;
            } 
        }

        if(0 == isExisted) {
            create_new_list_node(pErrNum,plistHead);
        }

        return;
    }
}

/************************************************************
Return:         1:   line over the max lenth.
                    0:   Success
                    -1: end of the file.
************************************************************/
static int read_line(struct file* fp, char* buffer, loff_t* pPos) {
    ssize_t ret = 0;
    char pBuff[MAXSIZES] = {0};
    loff_t pos =  *pPos;
    int read_lenth = 0;

    if ((ret = vfs_read(fp, pBuff, MAXSIZES - 1, &pos)) > 0) {
        char* pStr = NULL;
        
        read_lenth = (int)(pos - *pPos);

        pStr = strchr(pBuff, '\n');
        if (NULL == pStr) {
            *pPos = pos;
            return 1;
        }

        *pStr = 0;
        strncpy(buffer, pBuff, MAXSIZES-1);
        *pPos = pos - (read_lenth - strlen(buffer)-1);
        return 0;
    }
    else {
        return -1;
    }
}

static void count_err_num(struct file* fp, struct list_head* plistHead)
{
    mm_segment_t fs;
    loff_t pos;
    ssize_t ret = 1;
    char* p = NULL;
    char pBuff[MAXSIZES] = {0};

    fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;

    while (1) {
        ret = read_line(fp, pBuff, &pos);

        if(-1 == ret) {
            printk("%s():%d:fail to read\n", __func__, __LINE__);
            break;
        }
        else if (1 == ret) {
            continue;
        }

        p = strstr(pBuff, ERROR_NO);
        if (p) {
            char errNumber[ERR_NO_SIZE] = {0};
            strncpy(errNumber, p+strlen(ERROR_NO), ERR_NO_SIZE-1);
            check_list_for_each(errNumber, plistHead);
        }
    } 

    set_fs(fs);
}

//if any dmd file is finded,return 0;else return -1
static int count_dmd_file(struct list_head* plistHead) {
    struct file* fp0 = NULL;
    struct file* fp1 = NULL;
    char* dmdPath_0 = NULL;
    char* dmdPath_1 = NULL;

    unsigned int type = get_logusertype_flag();

    if (type == BETA_USER) {
        dmdPath_0 = B_DMD_PATH_0;
        dmdPath_1 = B_DMD_PATH_1;
    }
    else {
        dmdPath_0 = DMD_PATH_0;
        dmdPath_1 = DMD_PATH_1;
    }

    fp0 = filp_open(dmdPath_0, O_RDONLY, 0);
    if (IS_ERR(fp0)) {
        printk("%s():%d:Can not find file:%s\n", __func__, __LINE__, DMD_PATH_0);
        return -1;
    } else {
        count_err_num(fp0, plistHead);
        FCLOSE_FP(fp0);
    }

    fp1 = filp_open(dmdPath_1, O_RDONLY, 0);
    if (!IS_ERR(fp1)) {
        count_err_num(fp1, plistHead);
        FCLOSE_FP(fp1);
    }
    return 0;
}

/************************************************************
Function:       get_dmd_err_num
Description:   get the most err number from DMD file about boot fail.
Input:           NA
Output:         errNumber     the dmd err number
                    count            the times of err occur 
                    errName       the mod name of this err
Return:         0:   find adapt err number
                    -1: not find.
************************************************************/
int get_dmd_err_num(unsigned int* errNum, unsigned int* count, char* errName) {
    
    struct list_head listHead;
    DmdErrInfo_t dmdErrInfo;

    if (NULL == errNum) {
        goto err;
    }

    INIT_LIST_HEAD(&listHead);

    if (-1 == count_dmd_file(&listHead)){
        printk("%s():%d:not found DMD file.\n", __func__, __LINE__);
        goto err;
    }

    if (-1 == get_the_max_err(&listHead, &dmdErrInfo)) {
        printk("%s():%d:not found DMD err info.\n", __func__, __LINE__);
        goto err;
    }
    else {
        if (dmdErrInfo.errCount < ERR_OCCUR_TIME) {
            printk("%s():%d: no more than ERR_OCCUR_TIME.\n", __func__, __LINE__);
            goto err;
        }
        *errNum = dmdErrInfo.errNum;
        if (NULL != count) {
            *count = dmdErrInfo.errCount;
        }
        if (NULL != errName) {
            strncpy(errName, dmdErrInfo.modInfo.modName, ERR_NO_SIZE-1);
        }
    }

    return 0;
err:
    return -1;
}
