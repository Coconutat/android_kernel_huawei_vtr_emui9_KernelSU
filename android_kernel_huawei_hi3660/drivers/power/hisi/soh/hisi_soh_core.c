
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/reboot.h>
#include <linux/timer.h>
#include <hisi_soh_core.h>
#include <linux/power/hisi/coul/hisi_coul_event.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include <linux/mtd/hisi_nve_interface.h>
#include "securec.h"



static struct soh_acr_device_ops     *g_soh_acr_core_ops      = NULL;
static struct soh_dcr_device_ops     *g_soh_dcr_core_ops      = NULL;
static struct soh_pd_leak_device_ops *g_soh_pd_leak_core_ops  = NULL;
static struct soh_ovp_device_ops     *g_soh_ovp_core_ops      = NULL;

struct device *soh_dev  = NULL;
static struct hisi_soh_device *g_di = NULL;

/*default ovp threshold*/
static struct soh_ovp_temp_vol_threshold default_ovh_thres[SOH_OVH_THRED_NUM] = {{4200,60},{4300,55},{4350,50}};  /*temp & vol*/
static struct soh_ovp_temp_vol_threshold default_ovl_thres[SOH_OVL_THRED_NUM] = {{4100,60},{4200,55},{4300,50}};  /*temp & vol*/
static struct soh_ovp_temp_vol_threshold default_ovl_safe_thres               = {4100,50};                        /*temp | vol*/

/*get nv info from boot*/
static struct acr_nv_info *boot_acr_nv_info        = NULL;
static struct dcr_nv_info *boot_dcr_nv_info        = NULL;
static struct pd_leak_nv_info *boot_pdleak_nv_info = NULL;

/*battery cycle  increment*/
static int acr_cycle_inc = ACR_CAL_BAT_CYCLE_INC;
static int dcr_cycle_inc = DCR_CAL_BAT_CYCLE_INC;

/*depend on coul cmdline add*/
extern unsigned long nv_info_addr;

/*function declaration*/
int hisi_soh_drv_register_atomic_notifier(struct notifier_block *nb);
int hisi_soh_drv_unregister_atomic_notifier(struct notifier_block *nb);

/*get array max value and min value.*/
void max_min_value(int array[],u32 size, int *min, int *max)
{
    u32 i;
    int max_value;
    int min_value;
    if (!size || !max || !min)
        return;
    max_value = min_value = array[0];

    for(i = 1; i < size; i++)
    {
        hisi_soh_debug("[%s] temp[%u]=%d\n", __func__, i, array[i]);
        if (array[i] > max_value)
            max_value = array[i];

        if(array[i] < min_value)
            min_value = array[i];
    }

    *max = max_value;
    *min = min_value;
}
/**********************************************************
*  Function:      soh_wake_lock
*  Description:   apply soh wake_lock
*  Parameters:    NULL
*  return value:  NULL
**********************************************************/
static void soh_wake_lock(struct hisi_soh_device *di)
{
    if (!di)
        return ;
    if (!wake_lock_active(&di->soh_wake_lock)) {
        wake_lock(&di->soh_wake_lock);
        hisi_soh_info("soh core wake lock!\n");
    }
}/*lint !e454 !e456*/
/**********************************************************
*  Function:      soh_wake_unlock
*  Description:   release soh wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void soh_wake_unlock(struct hisi_soh_device *di)
{
    if (!di)
        return ;
    if (wake_lock_active(&di->soh_wake_lock)) {
        wake_unlock(&di->soh_wake_lock);/*lint !e455*/
        hisi_soh_info("soh core wake unlock!\n");
    }
}

/**********************************************************
*  Function:       soh_core_acr_ops_register
*  Discription:    register the acr handler ops from  chipIC
*  Parameters:     ops:operations interface of soh device
*  return value:   0-sucess or others-fail
**********************************************************/
int soh_core_drv_ops_register(void *ops, enum soh_drv_ops_type ops_type)
{
    int ret = 0;

    if (!ops)
        hisi_soh_err(" %s ops register null!\n",__func__);

    switch (ops_type) {
    case ACR_DRV_OPS:
        g_soh_acr_core_ops = (struct soh_acr_device_ops *)ops;
        break;
    case DCR_DRV_OPS:
        g_soh_dcr_core_ops = (struct soh_dcr_device_ops *)ops;
        break;
    case PD_LEAK_DRV_OPS:
        g_soh_pd_leak_core_ops = (struct soh_pd_leak_device_ops *)ops;
        break;
   case SOH_OVP_DRV_OPS:
        g_soh_ovp_core_ops = (struct soh_ovp_device_ops *)ops;
        break;
    default:
        hisi_soh_err("[%s] failed ops register!\n", __func__);
        ret = -1;
        break;
    }
    hisi_soh_info("[%s]ops_type = %d!\n",__func__,ops_type);
    return ret;
}

/**********************************************************
*  Function:       soh_drv_acr_ops_check
*  Discription:    judge soh acr drv ops register  success or fail.
*  Input:          hisi_soh_device *di
*  Output:         NA
*  return value:   0-sucess or others-fail
**********************************************************/
static int soh_drv_acr_ops_check(struct hisi_soh_device *di)
{
    if (!di)
        return -1;

    if (di->soh_acr_dev.acr_support) {
        if (!g_soh_acr_core_ops) {
            hisi_soh_err("[%s] acr_ops is not register!\n",__func__);
            return -1;
        }

        di->soh_acr_dev.acr_ops = g_soh_acr_core_ops;
        if (!di->soh_acr_dev.acr_ops->enable_acr    || !di->soh_acr_dev.acr_ops->calculate_acr        || !di->soh_acr_dev.acr_ops->clear_acr_flag ||
            !di->soh_acr_dev.acr_ops->clear_acr_ocp || !di->soh_acr_dev.acr_ops->get_acr_chip_temp    || !di->soh_acr_dev.acr_ops->get_acr_flag   ||
            !di->soh_acr_dev.acr_ops->get_acr_ocp   || !di->soh_acr_dev.acr_ops->get_acr_fault_status ||
            !di->soh_acr_dev.acr_ops->clear_acr_fault_status || !di->soh_acr_dev.acr_ops->io_ctrl_acr_chip_en) {
            hisi_soh_err("[%s] soh acr device ops is NULL!\n",__func__);
            return -1;
        }
    }
    hisi_soh_info("[%s] suc!\n",__func__);
    return 0;
}

/**********************************************************
*  Function:       soh_drv_dcr_ops_check
*  Discription:    judge soh drv dcr ops register  success or fail.
*  Input:          hisi_soh_device *di
*  Output:         NA
*  return value:   0-sucess or others-fail
**********************************************************/
static int soh_drv_dcr_ops_check(struct hisi_soh_device *di)
{
    if (!di)
        return -1;

    if (di->soh_dcr_dev.dcr_support) {
        if (!g_soh_dcr_core_ops) {
            hisi_soh_err("[%s] dcr_ops is not register!\n",__func__);
            return -1;
        }

        di->soh_dcr_dev.dcr_ops = g_soh_dcr_core_ops;
        if (!di->soh_dcr_dev.dcr_ops->enable_dcr    ||  !di->soh_dcr_dev.dcr_ops->clear_dcr_flag  ||
            !di->soh_dcr_dev.dcr_ops->get_dcr_flag  ||  !di->soh_dcr_dev.dcr_ops->get_dcr_info  ) {
            hisi_soh_err("[%s] soh dcr device ops func is NULL!\n",__func__);
            return -1;
        }
    }
    hisi_soh_info("[%s] suc!\n",__func__);
    return 0;
}

/**********************************************************
*  Function:       soh_drv_ovp_ops_check
*  Discription:    judge soh drv ovp ops register  success or fail.
*  Input:          hisi_soh_device *di
*  Output:         NA
*  return value:   0-sucess or others-fail
**********************************************************/
static int soh_drv_ovp_ops_check(struct hisi_soh_device *di)
{
    if (!di)
        return -1;

    if (di->soh_ovp_dev.ovp_support) {
        if (!g_soh_ovp_core_ops) {
            hisi_soh_err("%s ovp_ops is not register!\n",__func__);
            return -1;
        }

        di->soh_ovp_dev.soh_ovp_ops = g_soh_ovp_core_ops;
        if (!di->soh_ovp_dev.soh_ovp_ops->set_ovp_threshold    || !di->soh_ovp_dev.soh_ovp_ops->get_ovh_thred_cnt ||
            !di->soh_ovp_dev.soh_ovp_ops->enable_ovp            || !di->soh_ovp_dev.soh_ovp_ops->enable_dischg     ||
            !di->soh_ovp_dev.soh_ovp_ops->get_stop_dischg_state ) {
            hisi_soh_err("%s soh ovp device ops func is NULL!\n",__func__);
            return -1;
        }
    }
    hisi_soh_info("[%s] suc!\n",__func__);
    return 0;
}

/**********************************************************
*  Function:       soh_drv_pd_leak_ops_check
*  Discription:    judge soh drv pd leak ops register  success or fail.
*  Input:          hisi_soh_device *di
*  Output:         NA
*  return value:   0-sucess or others-fail
**********************************************************/
static int soh_drv_pd_leak_ops_check(struct hisi_soh_device *di)
{
    if (!di)
        return -1;

    if (di->soh_pd_leak_dev.pd_leak_support) {
        if (!g_soh_pd_leak_core_ops) {
            hisi_soh_err("%s pd_leak_ops is not register!\n",__func__);
            return -1;
        }

        di->soh_pd_leak_dev.pd_leak_ops = g_soh_pd_leak_core_ops;
        if (!di->soh_pd_leak_dev.pd_leak_ops->enable_pd_leak   || !di->soh_pd_leak_dev.pd_leak_ops->get_pd_leak_fifo_depth ||
            !di->soh_pd_leak_dev.pd_leak_ops->get_pd_leak_info  ) {
            hisi_soh_err("%s soh dcr device ops func is NULL!\n",__func__);
            return -1;
        }
    }
    hisi_soh_info("[%s] suc!\n",__func__);
    return 0;
}
/**********************************************************
*  Function:       soh_drv_ops_check
*  Discription:    judge soh drv ops register  success or fail.
*  Input:          hisi_soh_device *di
*                  enum soh_drv_ops_type ops_type : ops type.
*  Output:         NA
*  return value:   0-sucess or others-fail
**********************************************************/
static int soh_drv_ops_check(struct hisi_soh_device *di, enum soh_drv_ops_type ops_type)
{
    int ret = -1;

    if (!di)
        return -1;

    switch (ops_type) {
    case ACR_DRV_OPS:
          /*acr driver register check*/
        ret = soh_drv_acr_ops_check(di);
        break;
    case DCR_DRV_OPS:
        /*dcr driver register check*/
        ret = soh_drv_dcr_ops_check(di);
        break;
    case SOH_OVP_DRV_OPS:
        /*soh_ovp driver register check*/
        ret = soh_drv_ovp_ops_check(di);
        break;
   case PD_LEAK_DRV_OPS:
        /*sohu pd driver register check*/
        ret = soh_drv_pd_leak_ops_check(di);
        break;
    default:
        hisi_soh_err("[%s] failed ops register!\n", __func__);
        break;
    }
    return ret;
}
/*******************************************************
  Function:       soh_acr_rw_nv
  Description:    save(get) acr info to(from) NV
  Input:          struct hisi_soh_device *di   ---- soh device
                  enum nv_rw_type rw    -- read or write nv
  Output:         NULL
  Return:         0:success  other:fail
********************************************************/
static int soh_acr_rw_nv(struct hisi_soh_device *di, enum nv_rw_type rw)
{
    struct acr_nv_info acrinfo;
    struct hisi_nve_info_user nve;
    int ret;

    if (!di)
        return -1;
    memset_s(&nve, sizeof(nve), 0, sizeof(nve));
    strncpy_s(nve.nv_name, sizeof(nve.nv_name), SOH_ACR_NV_NAME, sizeof(SOH_ACR_NV_NAME));
    nve.nv_number = SOH_ACR_NV_NUM;
    nve.valid_size = sizeof(acrinfo);
    /*save battery info for acr*/
    if (NV_WRITE == rw) {
        /*get nv data from read nv to Block overwrite*/
        memcpy_s(&acrinfo, sizeof(struct acr_nv_info), &di->soh_acr_dev.acr_nv, sizeof(struct acr_nv_info));
        if (acrinfo.order_num < 0)
            acrinfo.order_num = -1;
        acrinfo.order_num = (acrinfo.order_num +1)%SOH_ACR_NV_DATA_NUM;
        memcpy_s(&acrinfo.soh_nv_acr_info[acrinfo.order_num],sizeof(struct acr_info), &di->soh_acr_dev.soh_acr_info, sizeof(struct acr_info));
        nve.nv_operation = NV_WRITE;
        memcpy_s(nve.nv_data, sizeof(acrinfo), &acrinfo, sizeof(acrinfo));
        ret = hisi_nve_direct_access(&nve);
        if (ret)
            hisi_soh_err("[%s]acr save nv fail, ret=%d\n", __func__, ret);
        else/*succ, writeback to acr_nv*/
            memcpy_s(&di->soh_acr_dev.acr_nv, sizeof(acrinfo), &acrinfo, sizeof(acrinfo));

    } else {/*read acr nv data*/
        nve.nv_operation = NV_READ;
        ret = hisi_nve_direct_access(&nve);
        if (ret)
            hisi_soh_err("acr read nv partion fail, ret=%d\n", ret);
        else
           memcpy_s(&di->soh_acr_dev.acr_nv, sizeof(struct acr_nv_info), nve.nv_data, sizeof(struct acr_nv_info));
    }
    return ret;
}

/*******************************************************
  Function:       soh_dcr_rw_nv
  Description:    save(get) dcr info to(from) NV
  Input:
                  struct hisi_soh_device *di   ---- soh device
                  enum nv_rw_type rw    -- read or write nv
  Output:         NULL
  Return:         0:success  other:fail
********************************************************/
static int soh_dcr_rw_nv(struct hisi_soh_device *di, enum nv_rw_type rw)
{
    struct dcr_nv_info dcrinfo;
    struct hisi_nve_info_user nve;
    int ret;

    if (!di)
        return -1;

    memset_s(&nve, sizeof(nve), 0, sizeof(nve));
    strncpy_s(nve.nv_name, sizeof(nve.nv_name), SOH_DCR_NV_NAME, sizeof(SOH_DCR_NV_NAME));
    nve.nv_number = SOH_DCR_NV_NUM;
    nve.valid_size = sizeof(dcrinfo);
    /*save battery info for dcr*/
    if (NV_WRITE == rw) {
        /*get nv data from read nv to Block overwrite*/
        memcpy_s(&dcrinfo, sizeof(struct dcr_nv_info), &di->soh_dcr_dev.dcr_nv, sizeof(struct dcr_nv_info));

        if (dcrinfo.order_num < 0)
            dcrinfo.order_num = -1;
        dcrinfo.order_num = (dcrinfo.order_num +1)%SOH_DCR_NV_DATA_NUM;
        memcpy_s(&dcrinfo.soh_nv_dcr_info[dcrinfo.order_num], sizeof(struct dcr_info), &di->soh_dcr_dev.soh_dcr_info, sizeof(struct dcr_info));
        nve.nv_operation = NV_WRITE;
        memcpy_s(nve.nv_data, sizeof(dcrinfo), &dcrinfo, sizeof(dcrinfo));
        ret = hisi_nve_direct_access(&nve);
        if (ret)
            hisi_soh_err("[%s]dcr save nv fail, ret=%d\n", __func__, ret);
        else/*succ, writeback to dcr_nv*/
            memcpy_s(&di->soh_dcr_dev.dcr_nv, sizeof(dcrinfo), &dcrinfo, sizeof(dcrinfo));
    } else { /*read dcr nv data*/
        nve.nv_operation = NV_READ;
        ret = hisi_nve_direct_access(&nve);
        if (ret)
            hisi_soh_err("dcr read nv partion fail, ret=%d\n", ret);
        else
           memcpy_s(&di->soh_dcr_dev.dcr_nv, sizeof(struct dcr_nv_info), nve.nv_data, sizeof(struct dcr_nv_info));
    }
    return ret;
}

/*******************************************************
  Function:       soh_pd_leak_rw_nv
  Description:    save(get) pd info to(from) NV
  Input:
                  struct hisi_soh_device *di   ---- soh device
                  enum nv_rw_type rw    -- read or write nv
  Output:         NULL
  Return:         0:success  other:fail
********************************************************/
static int soh_pd_leak_rw_nv(struct hisi_soh_device *di, enum nv_rw_type rw)
{
    struct pd_leak_nv_info pdinfo;
    struct hisi_nve_info_user nve;
    int ret;

    if (!di)
        return -1;

    memset_s(&nve, sizeof(nve), 0, sizeof(nve));
    strncpy_s(nve.nv_name, sizeof(nve.nv_name), SOH_PD_LEAK_NV_NAME, sizeof(SOH_PD_LEAK_NV_NAME));
    nve.nv_number  = SOH_PD_LEAK_NV_NUM;
    nve.valid_size = sizeof(pdinfo);
    /*save battery data for pd*/
    if (NV_WRITE == rw) {
        /*get nv data from read nv to Block overwrite*/
        memcpy_s(&pdinfo, sizeof(struct pd_leak_nv_info), &di->soh_pd_leak_dev.pd_leak_nv, sizeof(struct pd_leak_nv_info));

        if (pdinfo.order_num < 0)
            pdinfo.order_num = -1;
        pdinfo.order_num = (pdinfo.order_num +1)%SOH_PD_NV_DATA_NUM;
        memcpy_s(&pdinfo.soh_nv_pd_leak_current_info[pdinfo.order_num], sizeof(struct pd_leak_current_info), &di->soh_pd_leak_dev.soh_pd_leak_current_info, sizeof(struct pd_leak_current_info));
        nve.nv_operation = NV_WRITE;
        memcpy_s(nve.nv_data, sizeof(pdinfo), &pdinfo, sizeof(pdinfo));
        ret = hisi_nve_direct_access(&nve);
        if (ret)
            hisi_soh_err("[%s]pd leak save nv fail, ret=%d\n", __func__, ret);
        else/*succ, writeback to pdinfo nv*/
            memcpy_s(&di->soh_pd_leak_dev.pd_leak_nv, sizeof(pdinfo), &pdinfo, sizeof(pdinfo));
    } else {/*read pd leak nv data*/
        nve.nv_operation = NV_READ;
        ret = hisi_nve_direct_access(&nve);
        if (ret)
            hisi_soh_err("pd read nv partion fail, ret=%d\n", ret);
        else
           memcpy_s(&di->soh_pd_leak_dev.pd_leak_nv, sizeof(struct pd_leak_nv_info), nve.nv_data, sizeof(struct pd_leak_nv_info));
    }

    return ret;
}
/*******************************************************
  Function:       soh_rw_nv_info
  Description:    save(get) soh info to(from) NV
  Input:
                  struct hisi_soh_device *di   ---- soh device
                  enum soh_type type    -- soh type
                  enum nv_rw_type rw    -- read or write nv
  Output:         NULL
  Return:         0:success  other:fail
********************************************************/
static int soh_rw_nv_info(struct hisi_soh_device *di, enum soh_type type, enum nv_rw_type rw)
{
    int ret;

    if (!di) {
        hisi_soh_err("[%s]NULL point!\n", __func__);
   	    return -1;
    }

    switch (type) {
    case SOH_ACR:
        ret = soh_acr_rw_nv(di, rw);
        break;

    case SOH_DCR:
        ret = soh_dcr_rw_nv(di, rw);
        break;

    case SOH_PD_LEAK:
        ret = soh_pd_leak_rw_nv(di, rw);
        break;

    default:
        hisi_soh_err("[%s] nv type err!!!\n", __func__);
        return -1;
    }
    if (!ret)
        hisi_soh_info("[%s] rw nv success!\n",__func__);
     return ret;
}

/*************************************************************************
  Function:        soh_get_nv_info_from_boot
  Description:     get nv info rom boot by reserved mem.
  Input:           void
  Output:          nv info from boot
  Return:          0
**************************************************************************/
static int __init soh_get_nv_info_from_boot(void)
{
    if (!nv_info_addr){
        hisi_soh_err("[%s] nv add from coul is null!\n",__func__);
        return 0;
    }
    boot_acr_nv_info    = (struct acr_nv_info*)ioremap_wc((nv_info_addr + RESERVED_MEM_FOR_PMU_COUL_ADDR_128), sizeof(struct acr_nv_info));
    boot_dcr_nv_info    = (struct dcr_nv_info*)ioremap_wc((nv_info_addr + RESERVED_MME_FOR_PMU_ACR_ADD_128), sizeof(struct dcr_nv_info));
    boot_pdleak_nv_info = (struct pd_leak_nv_info*)ioremap_wc((nv_info_addr + RESERVED_MME_FOR_PMU_DCR_ADD_128), sizeof(struct pd_leak_nv_info));

    hisi_soh_info("[%s]acrinfo = 0x%pK, dcrinfo = 0x%pK, pdinfo = 0x%pK!\n", __func__, boot_acr_nv_info, boot_dcr_nv_info, boot_pdleak_nv_info);

    return 0;
}
/*************************************************************************
  Function:        parse_soh_ovp_dts
  Description:     get ovp dts info.
  Input:           struct hisi_soh_device *di
  Output:          soh_ovp_dev info
  Return:          0:parse success
                   other: parse fail.
**************************************************************************/
static int parse_soh_ovp_dts(struct hisi_soh_device *di)
{
    struct device_node* np;
    int ret;

    if (!di)
        return -1;

    np = di->dev->of_node;
    if(!np){
        hisi_soh_err("%s np is null!\n", __func__);
        return -1;
    }

    /*get support flag*/
    if (of_property_read_u32(np, "soh_ovp_support",(u32 *)&di->soh_ovp_dev.ovp_support)){
	    di->soh_ovp_dev.ovp_support = 0;
		hisi_soh_err("[%s] get ovp support  fail!\n", __func__);
    }

    /*get ovp discharge  threshold*/
	ret = of_property_read_u32_array(np, "soh_ovh_thd", (u32 *)&di->soh_ovp_dev.soh_ovh_thres[0], sizeof(di->soh_ovp_dev.soh_ovh_thres)/sizeof(int));
	if (ret) {
        memcpy_s(&di->soh_ovp_dev.soh_ovh_thres, sizeof(default_ovh_thres), &default_ovh_thres[0], sizeof(default_ovh_thres));
		hisi_soh_err("[%s] get ovp dis thred  fail!\n", __func__);
	}
    hisi_soh_info("[%s] addr:soh_ovh_thres[0].temp[%d],[0].vol[%d], [1].temp[%d],[1].vol[%d],[2].temp[%d],[2].vol[%d]!\n",
                                __func__,di->soh_ovp_dev.soh_ovh_thres[0].temp,di->soh_ovp_dev.soh_ovh_thres[0].bat_vol_mv,
                                         di->soh_ovp_dev.soh_ovh_thres[1].temp,di->soh_ovp_dev.soh_ovh_thres[1].bat_vol_mv,
                                         di->soh_ovp_dev.soh_ovh_thres[2].temp,di->soh_ovp_dev.soh_ovh_thres[2].bat_vol_mv);

    /*get ovp stop discharge threshold*/
	ret = of_property_read_u32_array(np, "soh_ovl_thd", (u32 *)&di->soh_ovp_dev.soh_ovl_thres[0], sizeof(di->soh_ovp_dev.soh_ovl_thres)/sizeof(int));
	if (ret) {
        memcpy_s(&di->soh_ovp_dev.soh_ovl_thres, sizeof(default_ovl_thres), &default_ovl_thres[0], sizeof(default_ovl_thres));
		hisi_soh_err("[%s] get soh_ovl_thd fail!\n", __func__);
	}
    hisi_soh_info("[%s] addr:soh_ovl_thd[0].temp[%d],[0].vol[%d], [1].temp[%d],[1].vol[%d],[2].temp[%d],[2].vol[%d]!\n",
                                __func__,di->soh_ovp_dev.soh_ovl_thres[0].temp,di->soh_ovp_dev.soh_ovl_thres[0].bat_vol_mv,
                                         di->soh_ovp_dev.soh_ovl_thres[1].temp,di->soh_ovp_dev.soh_ovl_thres[1].bat_vol_mv,
                                         di->soh_ovp_dev.soh_ovl_thres[2].temp,di->soh_ovp_dev.soh_ovl_thres[2].bat_vol_mv);


    /*get ovp stop discharge safe threshold*/
	ret = of_property_read_u32_array(np, "soh_ovl_safe_thd", (u32 *)&di->soh_ovp_dev.soh_ovl_safe_thres, sizeof(struct soh_ovp_temp_vol_threshold)/sizeof(int));
	if (ret) {
        memcpy_s(&di->soh_ovp_dev.soh_ovl_safe_thres, sizeof(default_ovl_safe_thres), &default_ovl_safe_thres, sizeof(default_ovl_safe_thres));
		hisi_soh_err("[%s] get soh_ovl_safe_thres  fail!\n", __func__);
	}
    hisi_soh_info("[%s] addr:soh_ovl_safe_thres temp[%d],vol[%d] !\n", __func__,di->soh_ovp_dev.soh_ovl_safe_thres.temp,di->soh_ovp_dev.soh_ovl_safe_thres.bat_vol_mv);

    /*gte start discharge time with ovh interrupt received.*/
	ret = of_property_read_u32_array(np, "soh_ovp_start_min_thd", (u32 *)&di->soh_ovp_dev.soh_ovp_start_time, 1);
	if (ret) {
        di->soh_ovp_dev.soh_ovp_start_time = OVP_START_MIN_MINUTES;
		hisi_soh_err("[%s] get ovp start time fail!\n", __func__);
	}
    hisi_soh_info("[%s]soh start time = [%d]min !\n", __func__, di->soh_ovp_dev.soh_ovp_start_time);

    return 0;
}

/*************************************************************************
  Function:        parse_soh_acr_dts
  Description:     get acr dts info.
  Input:           struct hisi_soh_device *di
  Output:          soh_acr_dev info
  Return:          0:parse success
                   other: parse fail.
**************************************************************************/
static int parse_soh_acr_dts(struct hisi_soh_device *di)
{
    struct device_node* np;

    if (!di) {
        hisi_soh_err("%s di is null!\n", __func__);
        return -1;
    }

    np = di->dev->of_node;
    if(!np){
        hisi_soh_err("%s np is null!\n", __func__);
        return -1;
    }

    if (of_property_read_u32(np, "acr_support",(u32 *)&di->soh_acr_dev.acr_support)){
	    di->soh_acr_dev.acr_support = 0;
		hisi_soh_err("[%s] get acr support fail!\n", __func__);
    }
    hisi_soh_info("[%s] soh acr support = [%d]!\n", __func__, di->soh_acr_dev.acr_support);

    return 0;
}

/*************************************************************************
  Function:        parse_soh_dcr_dts
  Description:     get dcr dts info.
  Input:           struct hisi_soh_device *di
  Output:          soh_dcr_dev info
  Return:          0:parse success
                   other: parse fail.
**************************************************************************/
static int parse_soh_dcr_dts(struct hisi_soh_device *di)
{
    struct device_node* np;

    if (!di) {
        hisi_soh_err("%s di is null!\n", __func__);
        return -1;
    }

    np = di->dev->of_node;
    if(!np){
        hisi_soh_err("%s np is null!\n", __func__);
        return -1;
    }

    if (of_property_read_u32(np, "dcr_support",(u32 *)&di->soh_dcr_dev.dcr_support)){
	    di->soh_dcr_dev.dcr_support = 0;
		hisi_soh_err("[%s] get dcr support fail!\n", __func__);
    }
    hisi_soh_info("[%s] soh dcr support = [%d]!\n", __func__, di->soh_dcr_dev.dcr_support);

    return 0;
}

/*************************************************************************
  Function:        parse_soh_pd_leak_dts
  Description:     get pd dts info.
  Input:           struct hisi_soh_device *di
  Output:          soh_pd_dev info
  Return:          0:parse success
                   other: parse fail.
**************************************************************************/
static int parse_soh_pd_leak_dts(struct hisi_soh_device *di)
{
    struct device_node* np;

    if (!di) {
        hisi_soh_err("%s di is null!\n", __func__);
        return -1;
    }

    np = di->dev->of_node;
    if(!np){
        hisi_soh_err("%s np is null!\n", __func__);
        return -1;
    }

    if (of_property_read_u32(np, "pd_ocv_support",(u32 *)&di->soh_pd_leak_dev.pd_leak_support)){
	    di->soh_pd_leak_dev.pd_leak_support = 0;
		hisi_soh_err("[%s] get pd ocv support fail!\n", __func__);
    }
    if (of_property_read_u32(np, "soh_pd_leak_standby_leakage_current",(u32 *)&di->soh_pd_leak_dev.soh_pd_leak_current_info.sys_pd_leak_cc)){
	    di->soh_pd_leak_dev.soh_pd_leak_current_info.sys_pd_leak_cc = 0;
		hisi_soh_err("[%s] get pd cc fail!\n", __func__);
    }

    hisi_soh_info("[%s] soh pd leak support = [%d], pd_cc = %d!\n", __func__, di->soh_pd_leak_dev.pd_leak_support, di->soh_pd_leak_dev.soh_pd_leak_current_info.sys_pd_leak_cc);

    return 0;
}

#ifdef CONFIG_SYSFS
/*************************************************************************
  Function:        nv_ctrl
  Description:     syfs node for set soh ctrl nv.
  Input:           enum soh_type
  Output:          NA
  Return:          >=0:success
                   other: parse fail.
  Remark:          1 close soh function by nv ctrl set .
                   2 valid in ENG Version
**************************************************************************/
ssize_t nv_ctrl(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int status = count;
    long val = 0;
    struct hisi_soh_device *di = g_di;

    if (strict_strtol(buf, 0, &val) < 0)
        return -EINVAL;

    if (!di)
        return status;

#ifdef CONFIG_HISI_DEBUG_FS
    switch (val) {
    case SOH_ACR:
        di->soh_acr_dev.acr_nv.acr_control = 1;
        soh_rw_nv_info(di, SOH_ACR, NV_WRITE);
        break;
    case SOH_DCR:
        di->soh_dcr_dev.dcr_nv.dcr_control = 1;
        soh_rw_nv_info(di, SOH_DCR, NV_WRITE);
        break;
    case SOH_PD_LEAK:
        di->soh_pd_leak_dev.pd_leak_nv.pd_control = 1;
        soh_rw_nv_info(di, SOH_PD_LEAK, NV_WRITE);
        break;
    default:
        break;
    }
#endif
	return status;
}

/*************************************************************************
  Function:        nv_clear
  Description:     syfs node for clear soh  nv.
  Input:           enum soh_type
  Output:          NA
  Return:          >=0:success
                   other: parse fail.
  Remark:          1 clear soh nv .
                   2 valid in ENG Version
**************************************************************************/
ssize_t nv_clear(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int status = count;
    long val = 0;
    struct hisi_soh_device *di = g_di;

    if (strict_strtol(buf, 0, &val) < 0)
        return -EINVAL;

    if (!di)
        return status;

#ifdef CONFIG_HISI_DEBUG_FS
    switch (val) {
    case SOH_ACR:
        memset_s(&di->soh_acr_dev.acr_nv, sizeof(struct acr_nv_info), 0, sizeof(struct acr_nv_info));
        memset_s(&di->soh_acr_dev.soh_acr_info, sizeof(struct acr_info), 0, sizeof(struct acr_info));
        soh_rw_nv_info(di, SOH_ACR, NV_WRITE);
        break;
    case SOH_DCR:
        memset_s(&di->soh_dcr_dev.dcr_nv, sizeof(struct dcr_nv_info), 0, sizeof(struct dcr_nv_info));
        memset_s(&di->soh_dcr_dev.soh_dcr_info, sizeof(struct dcr_info), 0, sizeof(struct dcr_info));
        soh_rw_nv_info(di, SOH_DCR, NV_WRITE);
        break;
    case SOH_PD_LEAK:
        memset_s(&di->soh_pd_leak_dev.pd_leak_nv, sizeof(struct pd_leak_nv_info), 0, sizeof(struct pd_leak_nv_info));
        memset_s(&di->soh_pd_leak_dev.soh_pd_leak_current_info, sizeof(struct pd_leak_current_info), 0, sizeof(struct pd_leak_current_info));
        soh_rw_nv_info(di, SOH_PD_LEAK, NV_WRITE);
        break;
    default:
        break;
    }
#endif
	return status;
}

static enum soh_type nv_read_sel_flag;
/*************************************************************************
  Function:        nv_read_sel
  Description:     sel nv read soh type.
  Input:           enum soh_type
  Output:          NA
  Return:          >=0:success
                   other: parse fail.
  Remark:          sel soh type which is read by nv.
**************************************************************************/
ssize_t nv_read_sel (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int status = count;
    long val = 0;
    struct hisi_soh_device *di = g_di;

    if (strict_strtol(buf, 0, &val) < 0)
        return -EINVAL;

    if (!di)
        return status;

    switch (val) {
    case SOH_ACR:
        nv_read_sel_flag = SOH_ACR;
        break;
    case SOH_DCR:
        nv_read_sel_flag = SOH_DCR;
        break;
    case SOH_PD_LEAK:
        nv_read_sel_flag = SOH_PD_LEAK;
        break;
    default:
        break;
    }
    hisi_soh_info("[%s] sel = %d!\n", __func__, nv_read_sel_flag);

	return status;
}

void soh_nv_printf(struct hisi_soh_device *_di, enum soh_type type)
{
    int val0,val1,val2,val3,val4,val5,val6;
    int val7,val8,val9,val10,val11,val12,val13;
    int val14,val15,val16,val17,val18,val19,val20;
    int val21,val22,val23,val24;
    struct hisi_soh_device *di = _di;

    if (!di)
        return ;

    switch (type) {
    case SOH_ACR:
        val1   = di->soh_acr_dev.acr_nv.order_num;
        val2   = di->soh_acr_dev.acr_nv.acr_control;
        val3   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].batt_cycle;
        val4   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].batt_acr;
        val5   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].chip_temp[0];
        val6   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].chip_temp[1];
        val7   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].batt_vol;
        val8   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].batt_temp;
        val9   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].batt_cycle;
        val10  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].batt_acr;
        val11  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].chip_temp[0];
        val12  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].chip_temp[1];
        val13  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].batt_vol;
        val14  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].batt_temp;
        val15  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].batt_cycle;
        val16  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].batt_acr;
        val17  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].chip_temp[0];
        val18  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].chip_temp[1];
        val19  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].batt_vol;
        val20  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].batt_temp;
        hisi_soh_info("[%s]ACR:order[%d]  ctrl [%d]\n", __func__,val1,val2);
        hisi_soh_info("[%s]Arrary[0]: cycle:%d  acr:%d  chip_temp0:%d  chip_temp1:%d  vol:%d  bat_temp:%d\n", __func__,val3,val4,val5,val6,val7,val8);
        hisi_soh_info("[%s]Arrary[1]: cycle:%d  acr:%d  chip_temp0:%d  chip_temp1:%d  vol:%d  bat_temp:%d\n", __func__,val9,val10,val11,val12,val13,val14);
        hisi_soh_info("[%s]Arrary[2]: cycle:%d  acr:%d  chip_temp0:%d  chip_temp1:%d  vol:%d  bat_temp:%d\n", __func__,val15,val16,val17,val18,val19,val20);
        break;

    case SOH_DCR:
        val0  = di->soh_dcr_dev.dcr_nv.order_num;
        val1  = di->soh_dcr_dev.dcr_nv.dcr_control;
        val2  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_cycle;
        val3  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_dcr;
        val4  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_r0;
        val5  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_vol;
        val6  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_temp;
        val7  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_cycle;
        val8  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_dcr;
        val9  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_r0;
        val10 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_vol;
        val11 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_temp;
        val12 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_cycle;
        val13 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_dcr;
        val14 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_r0;
        val15 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_vol;
        val16 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_temp;
        hisi_soh_info("[%s]DCR:order[%d]  ctrl [%d]\n", __func__,val0,val1);
        hisi_soh_info("[%s]Array[0]:cycle[%d]  dcr[%d]  r0[%d]  vol[%d]  bat_temp[%d]\n", __func__,val2,val3,val4,val5,val6);
        hisi_soh_info("[%s]Array[1]:cycle[%d]  dcr[%d]  r0[%d]  vol[%d]  bat_temp[%d]\n", __func__,val7,val8,val9,val10,val11);
        hisi_soh_info("[%s]Array[2]:cycle[%d]  dcr[%d]  r0[%d]  vol[%d]  bat_temp[%d]\n", __func__,val12,val13,val14,val15,val16);
        break;
    case SOH_PD_LEAK:
        val1   = di->soh_pd_leak_dev.pd_leak_nv.order_num;
        val2   = di->soh_pd_leak_dev.pd_leak_nv.pd_control;
        val3   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_cycle;
        val4   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].leak_current_ma;
        val5   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].chip_temp[0];
        val6   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].chip_temp[1];
        val7   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_current[0];
        val8   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_current[1];
        val9   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_vol[0];
        val10   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_vol[1];
        val11   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].rtc_time[0];
        val12   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].rtc_time[1];
        val13   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].sys_pd_leak_cc;
        val14   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_cycle;
        val15  = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].leak_current_ma;
        val16  = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].chip_temp[0];
        val17  = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].chip_temp[1];
        val18   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_current[0];
        val19   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_current[1];
        val20   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_vol[0];
        val21   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_vol[1];
        val22   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].rtc_time[0];
        val23   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].rtc_time[1];
        val24   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].sys_pd_leak_cc;
        hisi_soh_info("[%s]PD_LEAK:order[%d]  ctrl [%d]\n", __func__,val1,val2);
        hisi_soh_info("[%s]Array[0]:cycle[%d]  leak_ma[%d]  chip_t0[%d]  chip_t1[%d]  cur0[%d]  cur1[%d] vol0[%d] vol1[%d] rtc0[%d] rtc1[%d] sys_cc[%d]\n",
                        __func__, val3,val4,val5,val6,val7,val8,val9,val10,val11,val12,val13);
        hisi_soh_info("[%s]Array[1]:cycle[%d]  leak_ma[%d]  chip_t0[%d]  chip_t1[%d]  cur0[%d]  cur1[%d] vol0[%d] vol1[%d] rtc0[%d] rtc1[%d] sys_cc[%d]\n",
                         __func__, val14,val15,val16,val17,val18,val19,val20,val21,val22,val23,val24);
        break;
    default:
        break;
    }
}

/*************************************************************************
  Function:        nv_read
  Description:     read nv Depend on the soh type by nv_read_sel .
  Input:           enum soh_type
  Output:          nv info
  Return:          >=0:success
                   other: parse fail.
**************************************************************************/
ssize_t nv_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val0,val1,val2,val3,val4,val5,val6;
    int val7,val8,val9,val10,val11,val12,val13;
    int val14,val15,val16,val17,val18,val19,val20;
    int val21,val22,val23,val24;
    struct hisi_soh_device *di = g_di;
    if (!di)
        return -1;

    switch (nv_read_sel_flag) {
    case SOH_ACR:
        soh_rw_nv_info(di, SOH_ACR, NV_READ);
        val1   = di->soh_acr_dev.acr_nv.order_num;
        val2   = di->soh_acr_dev.acr_nv.acr_control;
        val3   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].batt_cycle;
        val4   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].batt_acr;
        val5   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].chip_temp[0];
        val6   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].chip_temp[1];
        val7   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].batt_vol;
        val8   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].batt_temp;
        val9   = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].batt_cycle;
        val10  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].batt_acr;
        val11  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].chip_temp[0];
        val12  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].chip_temp[1];
        val13  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].batt_vol;
        val14  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[1].batt_temp;
        val15  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].batt_cycle;
        val16  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].batt_acr;
        val17  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].chip_temp[0];
        val18  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].chip_temp[1];
        val19  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].batt_vol;
        val20  = di->soh_acr_dev.acr_nv.soh_nv_acr_info[2].batt_temp;
        snprintf_s(buf, PAGE_SIZE, PAGE_SIZE, "ACR: order[%d]  ctrl [%d]  Arrary[0]: cycle:%d  acr:%d  chip_temp0:%d  chip_temp1:%d  vol:%d  bat_temp:%d"
                 "Arrary[1]: cycle:%d  acr:%d  chip_temp0:%d  chip_temp1:%d  vol:%d  bat_temp:%d"
                 "Arrary[2]: cycle:%d  acr:%d  chip_temp0:%d  chip_temp1:%d  vol:%d  bat_temp:%d",
                  val1,val2,val3,val4,val5,val6,val7,val8,val9,val10,val11,val12,val13,val14,val15,val16,val17,val18,val19,val20);
        break;
    case SOH_DCR:
        soh_rw_nv_info(di, SOH_DCR, NV_READ);
        val0  = di->soh_dcr_dev.dcr_nv.order_num;
        val1  = di->soh_dcr_dev.dcr_nv.dcr_control;
        val2  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_cycle;
        val3  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_dcr;
        val4  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_r0;
        val5  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_vol;
        val6  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_temp;
        val7  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_cycle;
        val8  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_dcr;
        val9  = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_r0;
        val10 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_vol;
        val11 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[1].batt_temp;
        val12 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_cycle;
        val13 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_dcr;
        val14 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_r0;
        val15 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_vol;
        val16 = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[2].batt_temp;
        snprintf_s(buf, PAGE_SIZE, PAGE_SIZE, "DCR:order[%d] ctrl[%d]  array[0]: cycle[%d]  dcr[%d]  r0[%d]  vol[%d]  bat_temp[%d]"
                "array[1]:cycle[%d]  dcr[%d]  r0[%d]  vol[%d]  bat_temp[%d] "
                "array[2]:cycle[%d]  dcr[%d]  r0[%d]  vol[%d]  bat_temp[%d] ",
                 val0,val1,val2,val3,val4,val5,val6,val7,val8,val9,val10,val11,val12,val13,val14,val15,val16);
        break;
    case SOH_PD_LEAK:
        soh_rw_nv_info(di, SOH_PD_LEAK, NV_READ);
        val1   = di->soh_pd_leak_dev.pd_leak_nv.order_num;
        val2   = di->soh_pd_leak_dev.pd_leak_nv.pd_control;
        val3   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_cycle;
        val4   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].leak_current_ma;
        val5   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].chip_temp[0];
        val6   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].chip_temp[1];
        val7   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_current[0];
        val8   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_current[1];
        val9   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_vol[0];
        val10   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_vol[1];
        val11   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].rtc_time[0];
        val12   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].rtc_time[1];
        val13   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].sys_pd_leak_cc;
        val14   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_cycle;
        val15  = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].leak_current_ma;
        val16  = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].chip_temp[0];
        val17  = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].chip_temp[1];
        val18   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_current[0];
        val19   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_current[1];
        val20   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_vol[0];
        val21   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].batt_vol[1];
        val22   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].rtc_time[0];
        val23   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].rtc_time[1];
        val24   = di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[1].sys_pd_leak_cc;
        snprintf_s(buf, PAGE_SIZE, PAGE_SIZE, "PD_LEAK order[%d]  ctrl[%d]"
            "array[0]: cycle[%d]  leak_ma[%d]  chip_t0[%d]  chip_t1[%d]  cur0[%d]  cur1[%d] vol0[%d] vol1[%d] rtc0[%d] rtc1[%d] sys_cc[%d]"
            "array[1]: cycle[%d]  leak_ma[%d]  chip_t0[%d]  chip_t1[%d]  cur0[%d]  cur1[%d] vol0[%d] vol1[%d] rtc0[%d] rtc1[%d] sys_cc[%d]",
            val1,val2,val3,val4,val5,val6,val7,val8,val9,val10,val11,val12,val13,val14,val15,val16,val17,val18,val19,val20,val21,val22,val23,val24);
        break;
    default:
        break;
    }
    return strlen(buf);
}

ssize_t acr_raw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct hisi_soh_device *di;
    struct soh_acr_device *acr_dev;

    di = g_di;
    if(!di) {
        return -1;
    }
    acr_dev = &di->soh_acr_dev;
    memcpy_s(buf, sizeof(struct acr_info), &acr_dev->soh_acr_info, sizeof(struct acr_info));
    return sizeof(struct acr_info);
}

ssize_t dcr_raw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct hisi_soh_device *di;
    struct soh_dcr_device *dcr_dev;

    di = g_di;
    if(!di) {
        return -1;
    }
    dcr_dev = &di->soh_dcr_dev;
    memcpy_s(buf, sizeof(struct dcr_info), &dcr_dev->soh_dcr_info, sizeof(struct dcr_info));
    return sizeof(struct dcr_info);
}

static DEVICE_ATTR(nv_ctrl, (S_IWUSR | S_IRUGO), NULL, nv_ctrl);
static DEVICE_ATTR(nv_clear,(S_IWUSR | S_IRUGO), NULL, nv_clear);
static DEVICE_ATTR(nv_read, (S_IWUSR | S_IRUGO), nv_read, nv_read_sel);
static DEVICE_ATTR_RO(acr_raw);
static DEVICE_ATTR_RO(dcr_raw);

static struct attribute *soh_attributes[] = {
    &dev_attr_nv_ctrl.attr,
    &dev_attr_nv_clear.attr,
    &dev_attr_nv_read.attr,
    &dev_attr_acr_raw.attr,
    &dev_attr_dcr_raw.attr,
    NULL,
};

static const struct attribute_group soh_attr_group = {
    .attrs = soh_attributes,
};


static int soh_create_sysfs_file(struct hisi_soh_device *di)
{
    int retval = 0;
    struct class *power_class;

    if (!di) {
        hisi_soh_err("%s input di is null.", __func__);
        return -1;
    }

    retval = sysfs_create_group(&di->dev->kobj, &soh_attr_group);
    if (retval) {
        hisi_soh_err("%s failed to create sysfs group!!!\n", __func__);
        return -1;
    }
    power_class = hw_power_get_class();
    if (power_class) {
        if (!soh_dev) {
            soh_dev = device_create(power_class, NULL, 0, "%s", "soh");
            if (IS_ERR(soh_dev)) {
                soh_dev = NULL;
            }
        }
        if (soh_dev) {
            retval = sysfs_create_link(&soh_dev->kobj, &di->dev->kobj, "soh_data");
            if (0 != retval)
                hisi_soh_err("%s failed to create sysfs link!!!\n", __func__);
        } else
            hisi_soh_err("%s failed to create new_dev!!!\n", __func__);
    }

    if (retval)
        sysfs_remove_group(&di->dev->kobj, &soh_attr_group);
    return retval;
}

#endif
/*************************************************************************
  Function:        soh_acr_start_check
  Description:     check acr start valid by temp current and bat cycle¡¢soc.
  Input:           NA
  Output:          NA
  Return:          0:start check success
                   other: start check fail.
  Remark:          1 battery temp        [0,55]
                   2 battery current     [-20,20mA]
                   3 battery cycle add   [1,]
                   4 battery soc         //[95%,100%]
                   5 chip temp           [0,70] in soh_acr_calculate_work
**************************************************************************/
int soh_acr_start_check(void)
{
    int t_bat;
    int i_bat;
    int cycle_bat_now;
    int cycle_bat_pre;
    int real_soc;
    struct hisi_soh_device *di = g_di;

    if (!di) {
        hisi_soh_err("%s di is NULL!\n",__func__ );
        return -1;
    }

    /*if dts not support,return success */
    if (!di->soh_acr_dev.acr_support) {
        hisi_soh_err("[%s] not support soh acr!\n", __func__);
        return -1;
    }

    /*acr nv close*/
    if (di->soh_acr_dev.acr_nv.acr_control) {
        hisi_soh_err("[%s] nv close acr!\n", __func__);
        return -1;
    }

    t_bat         = hisi_battery_temperature();
    i_bat         = hisi_battery_current();
    cycle_bat_now = hisi_battery_cycle_count();

    if (di->soh_acr_dev.acr_nv.order_num < 0 || di->soh_acr_dev.acr_nv.order_num >= SOH_ACR_NV_DATA_NUM)
        di->soh_acr_dev.acr_nv.order_num = -1;

    if (-1 == di->soh_acr_dev.acr_nv.order_num)
        cycle_bat_pre = 0;
    else
        cycle_bat_pre = di->soh_acr_dev.acr_nv.soh_nv_acr_info[di->soh_acr_dev.acr_nv.order_num].batt_cycle;

    real_soc = hisi_battery_unfiltered_capacity();

    /*check battery temperature*/
    if (t_bat > ACR_CAL_BAT_TEMP_MAX || t_bat <  ACR_CAL_BAT_TEMP_MIN) {
        hisi_soh_err("%s bat temp[%d]c  exceed the normal range!!\n",__func__ , t_bat);
        return -1;
    }

    /*check battery now current*/
    if (abs(i_bat) > ACR_CAL_BAT_CUR_MAX) {
        hisi_soh_err("%s bat current[%d]mA, exceed the normal range!!\n",__func__ , i_bat);
        return -1;
    }

    /*check battery cycle*/
    if (abs(cycle_bat_now - cycle_bat_pre) < acr_cycle_inc) {
        hisi_soh_err("%s cycle inc not enough  ,pre = [%d], now = [%d]!!\n",__func__ , cycle_bat_pre, cycle_bat_now);
        return -1;
    }

    /*check battery real soc*/
    if (real_soc  < ACR_CAL_BAT_SOC_MIN) {
        hisi_soh_err("%s read soc low  ,real_soc = [%d]!!\n",__func__ , real_soc);
        /*return -1;*/
    }

    hisi_soh_info("[%s]sucess, ibat = [%d], tbat = [%d], cycle_now = [%d], cycle_pre = [%d], soc =[%d]!!\n",
                                            __func__ , i_bat, t_bat, cycle_bat_now, cycle_bat_pre, real_soc);
    return 0;
}
/*******************************************************
  Function:        soh_acr_valid_check
  Description:     check acr data valid and get final acr value.
  Input:           acr_data[]:acr data array
                   data_cnt: acr data count
  Output:          di->soh_acr_info.batt_acr :final acr data
  Return:          -1:acr invalid
                   0:acr valid.
  Remark:          1 avg_data - acr_data > avg_data*5%, acr_data invalid.
                   2 two acr data is invlid, check fail.
********************************************************/
static int soh_acr_valid_check(struct soh_acr_device *di, int acr_data[], int data_cnt)
{
    int invaid_cnt = 0;
    int i;
    int avg_data;
    int sum_data = 0;
    int data_error;
    int max,min;

    if (!di || !data_cnt) {
        hisi_soh_err("%s input para is err,cnt[%d]!\n", __func__, data_cnt);
        return -1;
    }

    for (i = 0; i < data_cnt; i++) {
        hisi_soh_debug("[%s] acr_data[%d]=%d\n", __func__, i, acr_data[i]);
        sum_data += acr_data[i];
    }
    /*get max and min, average calculation removes maximum and minimum values*/
     // cppcheck-suppress *
    max_min_value(acr_data, CAL_CNT, &min, &max);
    avg_data = (sum_data - max - min)/(data_cnt - 2);

    data_error = avg_data/ACR_AVG_PERCENT;
    /*if invalid cnt is more than 2 by compare avg acr and sample value , acr is invalid*/
    for (i = 0; i < data_cnt; i++) {
        if (abs(avg_data - acr_data[i]) > data_error) {

            hisi_soh_err("[%s] data invalid,avg[%d],acr[%d],err[%d],cnt[%d]!\n", __func__,
                                           avg_data, acr_data[i], data_error, invaid_cnt);
            if (invaid_cnt ++ > ACR_DATA_INVALID_NUM)
                return -1;
        }
    }

    /*get final acr value*/
    di->soh_acr_info.batt_acr = avg_data;

    hisi_soh_info("[%s] acr valid, acr = [%d]!\n", __func__, avg_data);
    return 0;
}

/*******************************************************
  Function:        soh_acr_get_data
  Description:     get CAL_CYCLE sets of acr data .
  Input:           *di: acr device struct.
                   *acr_data: save acr data point
                   *acr_temp: save acr temp point
                   *err_int:  acr data err cnt point.
  Output:          acr_data
                   acr_temp
                   err_cnt
  Return:          enum acr_cal_result
  Remark:          1 get_acr_fault_status is 0 forever in mia.
********************************************************/
static enum acr_cal_result  soh_acr_get_data(struct soh_acr_device *di, int *acr_data, int *acr_temp, int *err_cnt)
{
    int i;
    u32 acr_flag = 0;
    u32 acr_ocp = 0;
    int retry = ACR_RETRY_NUM;
    int acr_data_tmp;

    /*get CAL_CYCLE*data for acr calcuate, acr can start again after acr_flag is cleared*/
    // cppcheck-suppress *
    for (i = 0; i < CAL_CNT; i++ ) {

        do {
            /*if acr fault occurrs , this acr cal is ended*/
            if (di->acr_ops->get_acr_fault_status()) {
                hisi_soh_err("[%s] fault err, exit!\n", __func__);
                return ACR_ERROR;
            }

            /*if not get acr_flag, stop acr check forever by writting nv control, reset pmu stop discharge*/
            if (!retry) {
                hisi_soh_err("%s acr flag not occur in 200s, fatel err!\n", __func__);
                return ACR_FATAL;
            }
            acr_flag = di->acr_ops->get_acr_flag();
            msleep(2);
            hisi_soh_err("[%s] wait acr flag cnt[%d]!\n", __func__, ACR_RETRY_NUM - retry);
            retry--;
        } while(!acr_flag);

       /*if acr ocp happen,this calculation is abandoned*/
        acr_ocp = di->acr_ops->get_acr_ocp();
        if (acr_ocp) {
            hisi_soh_err("[%s] acr ocp err, exit!\n", __func__);
            return ACR_ERROR;
        }

        /*if chip temperature exceeds ,this calculation is abandoned*/
        *(acr_temp + i) = di->acr_ops->get_acr_chip_temp();
        if (*(acr_temp + i) > ACR_CAL_CHIP_TEMP_MAX || *(acr_temp + i) < ACR_CAL_CHIP_TEMP_MIN) {
            hisi_soh_err("[%s] acr chip temp[%d]c exceed the normal range,exit!!\n", __func__, *(acr_temp + i));
            return ACR_ERROR;
        }

        acr_data_tmp = di->acr_ops->calculate_acr();

        /*if get acr err,shedule task and recalculate,*/
        if (acr_data_tmp < 0) {
            /*if err count exceeds max err, cancel this calculate */
            if (++(*err_cnt) >= ACR_DATA_ERR_RETRY_CNT)
                *err_cnt  = 0;
            hisi_soh_err("[%s] acr data[%d] err,cnt =%d, exit!!\n", __func__, acr_data_tmp, *err_cnt);
            return ACR_ERROR;

        } else {
            hisi_soh_info("[%s] acr data suc, acr_data_err_cnt =%d!!\n", __func__,*err_cnt);
            *(acr_data + i) = acr_data_tmp;
        }

        /*last check need disable acr before clear acr flag, otherwise discharging don't stop*/
        if (CAL_CNT - 1 == i) {
            di->acr_ops->enable_acr(0);
            di->acr_ops->clear_acr_flag();
            hisi_soh_debug("[%s] cnt = %d, last check need disable arc and clear acr flag!\n", __func__, i);
        } else
            /*clear acr flag, start next check*/
            di->acr_ops->clear_acr_flag();

        hisi_soh_info("[%s] cnt = [%d] acr = [%d], chip_temp = [%d]!!\n", __func__,i, *(acr_data + i), *(acr_temp + i));
    }
    hisi_soh_info("[%s] normal!\n", __func__);
    return ACR_NORMAL;
}

/*****************************************************************************************************
  Function:        acr_calculate_delayed_work
  Description:     calculate acr ,check acr validity and save data into nv.
  Input:           struct work_struct *work
  Output:          NULL
  Return:          NULL
  Remark:          1 acr flag indicates that one acr detection is complete.
                   2 acr detection starts again after acr flag is cleared.
                   3 acr cal work is stopped if  acr data err is more than ACR_DATA_ERR_RETRY_CNT.
                   4 acr cal work is stopped if chip temperature exceeds .
                   5 save the highest and lowest chip temperature
                   6 acr data is saved to NV if cal is success.
                   7 acr check is forbidden by nv forever if fatal err occurs.
                   8 If an acr exception(140_deb/ldo1 ocp) occurs, it needs to be pulled down 20 ms before the acr io are pulled up.
*******************************************************************************************************/
static void soh_acr_calculate_work(struct work_struct *work)
{
    struct soh_acr_device *di = container_of(work, struct soh_acr_device, acr_work.work);
    int ret;
    // cppcheck-suppress *
    int acr_data[CAL_CNT + 1] = {0};
    int acr_temp[CAL_CNT + 1] = {0};
    static int acr_data_err_cnt = 0;

    /*acr cal don't allow  pmu to enter eco model*/
    soh_wake_lock(g_di);

    /*get acr battery info*/
    di->soh_acr_info.batt_vol   = hisi_battery_voltage();
    di->soh_acr_info.batt_temp  = hisi_battery_temperature();;
    di->soh_acr_info.batt_cycle = hisi_battery_cycle_count();

    /*io enable must be closed before enabling Because of the chip requirements*/
    di->acr_ops->io_ctrl_acr_chip_en(0);
    di->acr_ops->io_ctrl_acr_chip_en(1);
    di->acr_ops->enable_acr(1);

    /*get acr data*/
    ret = soh_acr_get_data(di, &acr_data[0], &acr_temp[0], &acr_data_err_cnt);
    if (ACR_NORMAL == ret)
        acr_data_err_cnt = 0;
    else if (ACR_ERROR == ret) {
        memset_s(&di->soh_acr_info, sizeof(struct acr_info), 0, sizeof(struct acr_info));
        goto acr_exit;
    }
    else if (ACR_FATAL == ret) {
        memset_s(&di->soh_acr_info, sizeof(struct acr_info), 0, sizeof(struct acr_info));
        goto fatal_acr_not_stop;
    }

    /*get acr chip temp */
    max_min_value(acr_temp, CAL_CNT, &di->soh_acr_info.chip_temp[0], &di->soh_acr_info.chip_temp[1]);

    /*check acr data validity and get final acr value.*/
    ret = soh_acr_valid_check(di, acr_data, CAL_CNT);

    /*save acr data into nv*/
    if (!ret) {
        if (ACR_H_PRECISION == di->acr_prec_type)
            soh_rw_nv_info(g_di, SOH_ACR, NV_WRITE);
        else
            di->acr_prec_type = ACR_H_PRECISION;
        hisi_soh_info("[%s] acr cal success!!\n", __func__);
        sysfs_notify(&g_di->dev->kobj, NULL, "acr_raw");
    } else
        memset_s(&di->soh_acr_info, sizeof(struct acr_info), 0, sizeof(struct acr_info));

acr_exit:
    hisi_soh_err("%s acr exit!!\n", __func__);
    di->acr_ops->enable_acr(0);
    di->acr_ops->clear_acr_flag();
    di->acr_ops->clear_acr_ocp();
    di->acr_ops->clear_acr_fault_status();
    di->acr_ops->io_ctrl_acr_chip_en(0);
    soh_wake_unlock(g_di);
    /*if data err,schedule work to recalcualte*/
    if (acr_data_err_cnt)
        queue_delayed_work(system_power_efficient_wq, &di->acr_work,
    		    round_jiffies_relative(msecs_to_jiffies(ACR_RETRY_ACR_MS)) );
    else {
        /*set acr work end*/
        mutex_lock(&g_di->soh_mutex);
        di->acr_work_flag = 0;
        mutex_unlock(&g_di->soh_mutex);
    }

    return;
fatal_acr_not_stop:
    hisi_soh_err("%s acr fatal err!!\n", __func__);
    di->acr_ops->enable_acr(0);
    di->acr_ops->io_ctrl_acr_chip_en(0);
    /*acr check is forbidden forever if fatel err occurs*/
    di->acr_nv.acr_control = 1;
    soh_rw_nv_info(g_di, SOH_ACR, NV_WRITE);
    /*set acr work end*/
    mutex_lock(&g_di->soh_mutex);
    di->acr_work_flag = 0;
    mutex_unlock(&g_di->soh_mutex);
    soh_wake_unlock(g_di);
#ifdef CONFIG_HISI_BB
    rdr_syserr_process_for_ap(MODID_AP_S_PMU, 0, 0);
#else
    machine_restart("AP_S_PMU");
#endif
}

/**********************************************************
*  Function:       soh_acr_notifier_call
*  Discription:    respond the acr events from coul core
*  Input:          acr_nb:acr notifier_block
*                  event:fault event name
*                  data:unused
*  Output:         NULL
*  return value:  NOTIFY_OK-success or others
**********************************************************/
static int soh_acr_notifier_call(struct notifier_block *acr_nb,unsigned long event, void *data)
{
    struct soh_acr_device *di = container_of(acr_nb, struct soh_acr_device, soh_acr_notify);

    if ((HISI_SOH_ACR == event) && g_di) {
        if (0 == soh_acr_start_check()) {
            /*acr and dcr mutex*/
            mutex_lock(&g_di->soh_mutex);
            if (g_di->soh_dcr_dev.dcr_work_flag) {
                /*if dcr is working, acr will wait for next check*/
                mutex_unlock(&g_di->soh_mutex);
                hisi_soh_err("[%s] dcr is working!!\n", __func__);
                return NOTIFY_OK;
            } else {
                /*set acr working flag*/
                di->acr_work_flag = 1;
                mutex_unlock(&g_di->soh_mutex);
            }
            di->acr_prec_type = ACR_H_PRECISION;
            queue_delayed_work(system_power_efficient_wq, &di->acr_work, msecs_to_jiffies(0));
        }
    }
    return NOTIFY_OK;
}
/**********************************************************
*  Function:       soh_acr_init
*  Discription:    acr init.
*  Input:          struct hisi_soh_device *di  soh device.
*  Output:         NULL
*  return value:   0-success , others-fail.
*  Remark:         1 driver ops register.
*                  2 notify register
*                  3 acr work init.
**********************************************************/
static int soh_acr_init(struct hisi_soh_device *di)
{
    int ret;

    if (!di)
        return -1;

    /*get dts*/
    if (parse_soh_acr_dts(di))
        return -1;

    /*if dts not support,return success */
    if (!di->soh_acr_dev.acr_support) {
        hisi_soh_err("%s not support soh acr!\n", __func__);
        return 0;
    }

    /*get init data from NV*/
    /*ret = soh_rw_nv_info(di, SOH_ACR, NV_READ);*/
	if (boot_acr_nv_info) {
        memcpy_s(&di->soh_acr_dev.acr_nv, sizeof(struct acr_nv_info), boot_acr_nv_info, sizeof(struct acr_nv_info));
        soh_nv_printf(di, SOH_ACR);
        /*acr nv close*/
        if (di->soh_acr_dev.acr_nv.acr_control) {
            hisi_soh_err("[%s] nv close acr!\n", __func__);
            return 0;
        }
         /*acr first cal, order number init*/
        if (!di->soh_acr_dev.acr_nv.soh_nv_acr_info[0].batt_acr) {
            di->soh_acr_dev.acr_nv.order_num = -1;
            hisi_soh_err("[%s] order num init!\n", __func__);
        }
    } else {
        hisi_soh_err("[%s] nv rw fail!\n", __func__);
        return -1;
    }

    /*check drv ops register*/
    if (soh_drv_ops_check(di, ACR_DRV_OPS))
        return -1;

    /*register notifier for coul core*/
    di->soh_acr_dev.soh_acr_notify.notifier_call = soh_acr_notifier_call;
    ret = hisi_coul_register_blocking_notifier(&di->soh_acr_dev.soh_acr_notify);
    if (ret < 0) {
       hisi_soh_err("[%s]soh_acr_register_notifier failed!\n", __func__);
       return -1;
    }

    /* Init interrupt notifier work */
    INIT_DELAYED_WORK(&di->soh_acr_dev.acr_work, soh_acr_calculate_work);

    hisi_soh_info("[%s]suc!\n", __func__);
    return 0;
}

/**********************************************************
*  Function:       soh_acr_uninit
*  Discription:    acr uninit.
*  Input:          struct hisi_soh_device *di  soh device.
*  Output:         NULL
*  return value:  0-success ,others-fail.
**********************************************************/
static int soh_acr_uninit(struct hisi_soh_device *di)
{
    if (!di)
        return -1;

    /*if dts not support,return success */
    if (!di->soh_acr_dev.acr_support) {
        hisi_soh_err("%s not support soh acr!\n", __func__);
        return 0;
    }

    hisi_coul_unregister_blocking_notifier(&di->soh_acr_dev.soh_acr_notify);

    hisi_soh_info("[%s]suc!\n", __func__);
    return 0;
}

/**********************************************************
*  Function:       soh_ovp_notifier_call
*  Discription:    respond the ovp events from soh driver
*  Input:          acr_nb:acr notifier_block
*                  event:fault event name
*                  data:unused
*  Output:         NULL
*  return value:  NOTIFY_OK-success or others
**********************************************************/
static int soh_ovp_notifier_call(struct notifier_block *soh_ovp_nb,unsigned long event, void *data)
{
    struct soh_ovp_device *di = container_of(soh_ovp_nb, struct soh_ovp_device, soh_ovp_notify);

    di->ovp_type = event;

    /*if dts not support,return success */
    if (!di->ovp_support) {
        hisi_soh_err("[%s] not support soh ovp!\n", __func__);
        return NOTIFY_OK;
    }
    queue_delayed_work(system_power_efficient_wq, &di->soh_ovp_work, msecs_to_jiffies(0));

    return NOTIFY_OK;
}
/**********************************************************
*  Function:       soh_ovp_exit_check
*  Discription:    check ovp exit Conditions.
*  Input:          struct soh_ovp_device *di
*  Output:         NULL
*  return value:   0: not exit ovp, other:exit ovp.
**********************************************************/
static int soh_ovp_exit_check(struct soh_ovp_device *di)
{
    int v_bat;
    int t_bat;
    int i;

    if (!di)
        return -1;

    t_bat = hisi_battery_temperature();
    v_bat = hisi_battery_voltage();

    for(i = 0; i < SOH_OVL_THRED_NUM; i++) {
        if (t_bat <= di->soh_ovl_thres[i].temp && v_bat <= di->soh_ovl_thres[i].bat_vol_mv) {
            hisi_soh_err("[%s] ovp exit, temp = [%d], vol = [%d]!\n", __func__, t_bat, v_bat);
            return 1;
        }
    }

    hisi_soh_info("[%s] in ovp, temp = [%d], vol = [%d]!\n", __func__, t_bat, v_bat);
    return 0;
}

/**********************************************************
*  Function:       soh_ovp_check_delayed_work
*  Discription:    handler the ovp events from soh driver
*  Input:          work:vop workqueue
*  Output:         NULL
*  return value:   NULL
*  Remark:         1 ovl check  is valid by chip after discharge starts .
                   2 ovh check  is valid by chip after discharge stop.

**********************************************************/
static void soh_ovp_check_delayed_work(struct work_struct *work)
{
    struct soh_ovp_device *di = container_of(work, struct soh_ovp_device, soh_ovp_work.work);
    static int ovh_start_time_s = 0;
    static int ovp_dischg_flag = 0;
    int ovh_now_time_s;
    int ovp_state;

    hisi_soh_info("[%s] ovp type = [%d]!\n", __func__, di->ovp_type);

    /*get ovh enter time*/
    if (!ovh_start_time_s)
        ovh_start_time_s = hisi_getcurtime()/NSEC_PER_SEC;

    if (SOH_OVH == di->ovp_type) {

        /*ovh keep system awake for Power consumption to lower voltage*/
        soh_wake_lock(g_di);

        ovh_now_time_s = hisi_getcurtime()/NSEC_PER_SEC;

        /*ovh is more than 30min, start discharge*/
        if ((ovh_now_time_s - ovh_start_time_s > di->soh_ovp_start_time * SEC_PER_MIN) && !ovp_dischg_flag) {
            ovp_dischg_flag = 1;
            di->soh_ovp_ops->enable_dischg(1);
            msleep(OVP_DISCHARGE_MIN_MS);
            hisi_soh_err("[%s] ovh is more than 30 min, discharge!\n", __func__);
        }
        ovp_state = soh_ovp_exit_check(di);

        /*if ovh time is less than 30 min and vol and temp is back to normal, work id ended */
        if (ovp_state && !ovp_dischg_flag) {
            hisi_soh_err("[%s] ovh not dischg, vol is back to Normal,work end!\n", __func__);
            /* ovp need to be enabled, if ovh generate but not discharge*/
            goto force_stop_dicharge;
        }
        /*if ovh time is more than 30 min and vol and temp is back to normal, discharge will be forced to stop */
        if (ovp_state && ovp_dischg_flag) {
            hisi_soh_err("[%s] ovh dischg, vol is back to normal, force to stop discharge!\n", __func__);
            goto force_stop_dicharge;
        }

       goto continue_check;

    } else {/*SOH_OVL*/
        /*soh work will be ended if soh ovl occurs and discharge is stopped automatically.*/
        if (di->soh_ovp_ops->get_stop_dischg_state()) {
            ovp_dischg_flag = 0;
            ovh_start_time_s = 0;
            soh_wake_unlock(g_di);
            hisi_soh_err("[%s] soh ovl irq ,auto stop discharge,end work!\n", __func__);
            return;
        } else {/*discharge will be fored to stop  if discharge isn't stopped automatically.*/
            hisi_soh_err("[%s] soh ovl irq ,auto stop dischg fail!\n", __func__);
            goto force_stop_dicharge;
        }

    }
force_stop_dicharge:
    di->soh_ovp_ops->enable_dischg(0);
    /*enable ovp because enalbe_dischg will disable ovp after 30ms*/
    msleep(30);
    di->soh_ovp_ops->enable_ovp(1);
    ovh_start_time_s = 0;
    ovp_dischg_flag = 0;
    soh_wake_unlock(g_di);
    hisi_soh_err("[%s] force stop dischage!\n", __func__);
    return;
continue_check:
    hisi_soh_info("[%s] continue check!\n", __func__);
    queue_delayed_work(system_power_efficient_wq, &di->soh_ovp_work, msecs_to_jiffies(OVP_WORK_DELAY_MS));
}

/**********************************************************
*  Function:       soh_ovp_set_protect_threshold
*  Discription:    set vol and temp (ovh and ovl) threshold
*  Input:          struct hisi_soh_device *di :soh device struct.
*  Output:         NULL
*  return value:   NULL
*  Remark:         NA
**********************************************************/
static void soh_ovp_set_protect_threshold(struct hisi_soh_device *di)
{
    unsigned int  ovh_fifo_cnt = 0;
    int ret;

    if (!di)
        return ;

    /*set ovp start threshold*/
    ovh_fifo_cnt = di->soh_ovp_dev.soh_ovp_ops->get_ovh_thred_cnt();
    ovh_fifo_cnt = MIN(ovh_fifo_cnt, SOH_OVH_THRED_NUM);
    ret = di->soh_ovp_dev.soh_ovp_ops->set_ovp_threshold(SOH_OVH, &di->soh_ovp_dev.soh_ovh_thres[0], ovh_fifo_cnt);
    if (ret)
        hisi_soh_err("[%s] set ovp start thrd fail!\n", __func__);

    /*set ovp stop safe threshold*/
    ret = di->soh_ovp_dev.soh_ovp_ops->set_ovp_threshold(SOH_OVL, &di->soh_ovp_dev.soh_ovl_safe_thres,1);
    if (ret)
        hisi_soh_err("[%s] set ovp safe thrd fail!\n", __func__);
    else
        hisi_soh_info("[%s] suc!\n", __func__);
}

/**********************************************************
*  Function:       soh_ovp_init
*  Discription:    ovp init.
*  Input:          struct hisi_soh_device *di :soh device struct.
*  Output:         NULL
*  return value:   NULL
*  Remark:         NA
**********************************************************/
static int soh_ovp_init(struct hisi_soh_device *di)
{
    int ret;

    if (!di)
        return -1;

    /*get dts*/
    if (parse_soh_ovp_dts(di))
        return -1;

    /*if dts not support,return success */
    if (!di->soh_ovp_dev.ovp_support) {
        hisi_soh_err("%s not support soh ovp!\n", __func__);
        return 0;
    }

    /*check drv ops register*/
    if (soh_drv_ops_check(di, SOH_OVP_DRV_OPS))
        return -1;

    /*soh ovp register notifier for soh driver*/
    di->soh_ovp_dev.soh_ovp_notify.notifier_call = soh_ovp_notifier_call;
    ret = hisi_soh_drv_register_atomic_notifier(&di->soh_ovp_dev.soh_ovp_notify);
    if (ret < 0) {
       hisi_soh_err("[%s]soh_ovp_register_notifier failed!\n", __func__);
       return -1;
    }

    /* Init interrupt notifier work */
    INIT_DELAYED_WORK(&di->soh_ovp_dev.soh_ovp_work, soh_ovp_check_delayed_work);


    /*set ovh and ovl gear */
    soh_ovp_set_protect_threshold(di);

    /*set ovp enable*/
    di->soh_ovp_dev.soh_ovp_ops->enable_ovp(1);

    hisi_soh_info("[%s]suc!\n", __func__);
    return 0;
}
/**********************************************************
*  Function:       soh_ovp_uninit
*  Discription:    ovp uninit.
*  Input:          struct hisi_soh_device *di :soh device struct.
*  Output:         NULL
*  return value:   NULL
*  Remark:         1 unregister notifier.
**********************************************************/
static int soh_ovp_uninit(struct hisi_soh_device *di)
{
    if (!di)
        return -1;

    /*if dts not support,return success */
    if (!di->soh_ovp_dev.ovp_support) {
        hisi_soh_err("%s not support soh ovp!\n", __func__);
        return 0;
    }

    hisi_soh_drv_unregister_atomic_notifier(&di->soh_ovp_dev.soh_ovp_notify);

    hisi_soh_info("[%s]suc!\n", __func__);
    return 0;
}
/*************************************************************************
  Function:        soh_dcr_start_check
  Description:     check dcr start valid by temp current and bat cycle¡¢soc.
  Input:           NA
  Output:          NA
  Return:          0:start check success
                   other: start check fail.
  Remark:          1 battery temp        [0,55]
                   2 battery current     [-20,20mA]
                   3 battery cycle add   [2,]
                   4 battery soc         //[95%,100%]
**************************************************************************/
int soh_dcr_start_check(void)
{
    int t_bat;
    int i_bat;
    int cycle_bat_now;
    int cycle_bat_pre;
    int real_soc;
    struct hisi_soh_device *di = g_di;

    if (!di) {
        hisi_soh_err("%s di is NULL!\n",__func__ );
        return -1;
    }

    /*if dts not support,return success */
    if (!di->soh_dcr_dev.dcr_support) {
        hisi_soh_err("%s not support soh acr!\n", __func__);
        return -1;
    }
    /*dcr nv close*/
    if (di->soh_dcr_dev.dcr_nv.dcr_control) {
        hisi_soh_err("[%s] nv close dcr!\n", __func__);
        return -1;
    }

    t_bat         = hisi_battery_temperature();
    i_bat         = hisi_battery_current();
    cycle_bat_now = hisi_battery_cycle_count();

    /*Prevent array Overflow*/
    if (di->soh_dcr_dev.dcr_nv.order_num < 0 || di->soh_dcr_dev.dcr_nv.order_num >= SOH_DCR_NV_DATA_NUM)
        di->soh_dcr_dev.dcr_nv.order_num = -1;

    if (-1 == di->soh_dcr_dev.dcr_nv.order_num)
        cycle_bat_pre = 0;
    else
        cycle_bat_pre = di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[di->soh_dcr_dev.dcr_nv.order_num].batt_cycle;

    real_soc = hisi_battery_unfiltered_capacity();

    /*check battery temperature*/
    if (t_bat > DCR_CAL_BAT_TEMP_MAX || t_bat <  DCR_CAL_BAT_TEMP_MIN) {
        hisi_soh_err("%s bat temp[%d]c  exceed the normal range!!\n",__func__ , t_bat);
        return -1;
    } else
        di->soh_dcr_dev.soh_dcr_info.batt_temp = t_bat;


    /*check battery now current, Consider current accuracy error*/
    if (abs(i_bat) > DCR_CAL_BAT_CUR_INC_MAX) {
        hisi_soh_err("%s bat current[%d] mA exceed the normal range!!\n",__func__ , i_bat);
        return -1;
    }

    /*check battery cycle*/
    if (abs(cycle_bat_now - cycle_bat_pre) < dcr_cycle_inc) {
        hisi_soh_err("%s cycle inc not enough  ,pre = [%d], now = [%d]!!\n",__func__ ,cycle_bat_pre, cycle_bat_now);
        return -1;
    } else
        di->soh_dcr_dev.soh_dcr_info.batt_cycle = cycle_bat_now;

    /*check battery real soc*/
    if (real_soc  < DCR_CAL_BAT_SOC_MIN) {
        hisi_soh_err("%s read soc low  ,real_soc = [%d]!!\n",__func__ , real_soc);
        /*return -1;*/
    }

    hisi_soh_info("[%s]sucess, ibat = [%d], tbat = [%d], cycle_now = [%d], cycle_pre = [%d], soc =[%d]!!\n",
                                            __func__ , i_bat, t_bat, cycle_bat_now, cycle_bat_pre, real_soc);
    return 0;
}

/*************************************************************************
  Function:        soh_dcr_get_info
  Description:     get dcr info and Check the validity of the info.
  Input:           NA
  Output:          *vol: fifo0 vol mV
                   *current: fifo0 current mA
  Return:          0:info valid.
                   other: info invalid.
  Remark:          1 fifo current    [180, 220]mA
                   2 fifo vol diff   [-10, 10]mV
**************************************************************************/
STATIC int soh_dcr_get_info(int *i_ma, int *v_mv)
{
    struct hisi_soh_device *di = g_di;
    unsigned int fifo_depth;
    int i;
    int vol_mv, cur_ma;
    int last_vol_mv = 0;
    int ret;

    if (!di) {
        hisi_soh_err("[%s]di is NULL!\n",__func__ );
        return -1;
    }
    fifo_depth = di->soh_dcr_dev.dcr_ops->get_dcr_fifo_depth();

    if (fifo_depth > DCR_FIFO_MAX)
        return -1;

    for (i = fifo_depth - 1; i >= 0; i--) {

        ret = di->soh_dcr_dev.dcr_ops->get_dcr_info(&cur_ma, &vol_mv, i);
        if (ret)
            return -1;

        /*check fifo current */
        if (abs(cur_ma - DCR_CAL_DISCHARGE_BASE_CUR) > DCR_CAL_BAT_CUR_INC_MAX) {
            hisi_soh_err("[%s] dcr fifo current[%d]mA ,exceed the normal range!!\n",__func__ , cur_ma);
            return -1;
        }
        /*check fifo vol*/
        if (last_vol_mv  && abs(last_vol_mv - vol_mv) > DCR_CAL_BAT_VOL_INC_MAX) {
            hisi_soh_err("[%s] dcr fifo vol[%d]mV ,exceed the normal range!!\n",__func__ , vol_mv);
            return -1;
        }

        last_vol_mv = vol_mv;

    }

    *i_ma = cur_ma;
    *v_mv = vol_mv;
    hisi_soh_debug("[%s] current = %d ma, vol = %d mv!!\n",__func__ , *i_ma, *v_mv);
    return 0;
}

/*****************************************************************************************************
  Function:        soh_dcr_calculate_work
  Description:     calculate dcr ,check dcr validity and save data into nv.
  Input:           struct work_struct *work
  Output:          NULL
  Return:          NULL
  Remark:          1 the dcr detection is complete by dcr flag.
                   2 dcr data is saved to NV if cal is success.
                   3 dcr will be forbidden forever by write nv if it doesn't stop in within the specified time,
                   4 battery current        [180mA,220mA] in dcr discharge.
                   5 battery temp           [0,55] in dcr discharge.
                   6 battery temp change    [-5,5] in dcr discharge.
                   7 battery current by cc  [180mA,220mA] in dcr discharge,it'll exit if err is more than DCR_CURRENT_EXCD_MAX_CYCLE.
*******************************************************************************************************/
static void soh_dcr_calculate_work(struct work_struct *work)
{
    struct soh_dcr_device *di = container_of(work, struct soh_dcr_device, dcr_work.work);

    int ret;
    int v_bat0, v_bat1, v_bat2;
    int t_bat0, t_bat1, t_bat;
    int c_bat0, c_bat1;
    int i_bat;
    int retry = DCR_RETRY_NUM; /*500s*/
    u32 dcr_flag;
    int dcr_t;
    int cc_cur;
    int i_check_cycle = 0;

    if (!di) {
        hisi_soh_err("%s di is NULL!\n", __func__);
        return ;
    }

    dcr_t = di->dcr_ops->get_dcr_timer();
    if (!dcr_t) {
        /*set dcr work end*/
        mutex_lock(&g_di->soh_mutex);
        di->dcr_work_flag = 0;
        mutex_unlock(&g_di->soh_mutex);
        hisi_soh_err("[%s] dcr_t = 0 \n", __func__);
        return ;
    }

    /*acr cal don't allow  pmu to enter eco model*/
    soh_wake_lock(g_di);

    /*get bat info before dcr starts*/
    t_bat0 = hisi_battery_temperature();
    v_bat0 = hisi_battery_voltage();
    c_bat0 = hisi_battery_cc_uah();

    /*enable dcr check*/
    di->dcr_ops->enable_dcr(1);

    /*coul vol sample is 250ms*/
    mdelay(DCR_R0_WATI_MS);

    /*get r0 cal second vol*/
    v_bat1 = hisi_battery_voltage();

    /*get dcr flag*/
    do {
        if (!retry) {
            hisi_soh_err("[%s] dcr flag not occur in 500s, fatel err!\n", __func__);
            goto fatal_dcr_not_stop;
        }

        dcr_flag = di->dcr_ops->get_dcr_flag();

        t_bat = hisi_battery_temperature();

        /*check battery temperature*/
        if (t_bat > DCR_CAL_BAT_TEMP_MAX || t_bat <  DCR_CAL_BAT_TEMP_MIN) {
            hisi_soh_err("[%s] bat temp[%d]c exceed the normal range, exit!!\n",__func__ , t_bat);
            goto dcr_exit;
        }

        /*check battery now current, Consider current accuracy error*/
        i_bat = hisi_battery_current();
        if (abs(i_bat - DCR_CAL_DISCHARGE_BASE_CUR) > DCR_CAL_BAT_CUR_INC_MAX) {
            hisi_soh_err("[%s] bat current[%d]mA ,exceed the normal range!!\n",__func__ , i_bat);
            if (i_check_cycle ++ > DCR_CURRENT_EXCD_MAX_CYCLE)
                goto dcr_exit;
        }

        msleep(DCR_CHECK_FLAG_CYCLE_TIMER);
        hisi_soh_debug("[%s] dcr flag cnt[%d]!\n", __func__, DCR_RETRY_NUM - retry);
        retry--;
    } while(!dcr_flag);

    /*clear dcr flag for next check*/
    di->dcr_ops->clear_dcr_flag();

    /*get bat info after dcr ends*/
    c_bat1 = hisi_battery_cc_uah();
    t_bat1 = hisi_battery_temperature();

    /*get dcr info*/
    ret = soh_dcr_get_info(&i_bat,&v_bat2);
    if (ret) {
        hisi_soh_err("[%s]fifo cur or vol info err, exit!\n", __func__);
        goto dcr_exit;
    }

    hisi_soh_info("[%s] v_bat0[%d]mv, v_bat1[%d]mv, v_bat2[%d]mv, c_bat0[%d]mah, c_bat1[%d]mah, t_bat0[%d], t_bat1[%d]!\n",
                                                        __func__, v_bat0, v_bat1, v_bat2, c_bat0, c_bat1, t_bat0, t_bat1);

    /*check dcr info valid by cc*/
    cc_cur = abs((c_bat1 - c_bat0)*SEC_PER_H/(dcr_t * UAH_PER_MAH));/*lint !e647*/
    if (abs(cc_cur - DCR_CAL_DISCHARGE_BASE_CUR) > DCR_CAL_BAT_CUR_INC_MAX_BY_CC) {
        hisi_soh_err("[%s] cc_cur = [%d]mA,cc1[%d] - cc0[%d] current by cc exceed the normal range, exit!\n", __func__,cc_cur, c_bat1, c_bat0);
        goto dcr_exit;
    }

    hisi_soh_info("[%s]cc_cur = [%d]mA,cc0 = [%d]uah, cc1 = [%d]\n", __func__, cc_cur, c_bat0, c_bat1);

    /*check dcr temp valid */
    if (abs(t_bat1 - t_bat0) > DCR_CAL_BAT_TEMP_DIFF) {
        hisi_soh_err("[%s] t1[%d]-t0[%d] temp range is exceed the range,exit!\n", __func__, t_bat1, t_bat0);
        goto dcr_exit;
    }

    /*cal dcr*/
    if (cc_cur)
        di->soh_dcr_info.batt_dcr = (v_bat0 - v_bat2)*MOHM_PER_OHM/cc_cur;
    else
        di->soh_dcr_info.batt_dcr = 0;

    /*cal r0*/
    di->soh_dcr_info.batt_r0 = (v_bat0 -v_bat1)*MOHM_PER_OHM/DCR_CAL_DISCHARGE_BASE_CUR;

    /*get dcr other info*/
    di->soh_dcr_info.batt_cycle = hisi_battery_cycle_count();
    di->soh_dcr_info.batt_vol   = v_bat0;
    di->soh_dcr_info.batt_temp  = t_bat0;

    hisi_soh_info("[%s] r0 =[%d], dcr = [%d]\n", __func__, di->soh_dcr_info.batt_r0, di->soh_dcr_info.batt_dcr);
    /*save acr data into nv*/
    soh_rw_nv_info(g_di, SOH_DCR, NV_WRITE);
    sysfs_notify(&g_di->dev->kobj, NULL, "dcr_raw");
    hisi_soh_info("[%s] dcr cal success!!\n", __func__);

dcr_exit:
    hisi_soh_info("[%s] dcr exit!!\n", __func__);
    di->dcr_ops->enable_dcr(0);
    di->dcr_ops->clear_dcr_flag();
    /*set dcr work end*/
    mutex_lock(&g_di->soh_mutex);
    di->dcr_work_flag = 0;
    mutex_unlock(&g_di->soh_mutex);
    soh_wake_unlock(g_di);
    return;
fatal_dcr_not_stop:
    hisi_soh_err("[%s] dcr fatal err!!\n", __func__);
    di->dcr_ops->enable_dcr(0);
    di->dcr_ops->clear_dcr_flag();
    /*set dcr work end*/
    mutex_lock(&g_di->soh_mutex);
    di->dcr_work_flag = 0;
    mutex_unlock(&g_di->soh_mutex);
    /*acr check is forbidden forever if fatel err occurs*/
    di->dcr_nv.dcr_control = 1;
    soh_rw_nv_info(g_di, SOH_DCR, NV_WRITE);
    soh_wake_unlock(g_di);
}

/**********************************************************
*  Function:       soh_dcr_notifier_call
*  Discription:    respond the dcr events from coul core
*  Input:          acr_nb:acr notifier_block
*                  event:fault event name
*                  data:unused
*  Output:         NULL
*  return value:  NOTIFY_OK-success or others
**********************************************************/
static int soh_dcr_notifier_call(struct notifier_block *dcr_nb,unsigned long event, void *data)
{
    struct soh_dcr_device *di = container_of(dcr_nb, struct soh_dcr_device, soh_dcr_notify);

    if ((HISI_SOH_DCR == event) && g_di) {
        if (0 == soh_dcr_start_check()) {
            /*acr and dcr mutex*/
            mutex_lock(&g_di->soh_mutex);
            if (g_di->soh_acr_dev.acr_work_flag) {
                /*if acr is working, acr will wait for next check*/
                mutex_unlock(&g_di->soh_mutex);
                hisi_soh_err("[%s] acr is working,wait for next!!\n", __func__);
                return NOTIFY_OK;
            } else {
                /*set dcr working flag*/
                di->dcr_work_flag = 1;
                mutex_unlock(&g_di->soh_mutex);
            }
            queue_delayed_work(system_power_efficient_wq, &di->dcr_work, msecs_to_jiffies(0));

        }

    }
    return NOTIFY_OK;
}
/**********************************************************
*  Function:       soh_dcr_init
*  Discription:    dcr init.
*  Input:          struct hisi_soh_device *di
*  Output:         NULL
*  return value:  0-success or others -fail.
**********************************************************/
static int soh_dcr_init(struct hisi_soh_device *di)
{
    int ret;

    if (!di)
        return -1;

    /*get dts*/
    if (parse_soh_dcr_dts(di))
        return -1;

    /*if dts not support,return success */
    if (!di->soh_dcr_dev.dcr_support) {
        hisi_soh_err("%s not support soh dcr!\n", __func__);
        return 0;
    }

    /*get init data from NV*/
    /*ret = soh_rw_nv_info(di, SOH_DCR, NV_READ);*/
	if (boot_dcr_nv_info) {
        memcpy_s(&di->soh_dcr_dev.dcr_nv, sizeof(struct dcr_nv_info), boot_dcr_nv_info, sizeof(struct dcr_nv_info));
        soh_nv_printf(di, SOH_DCR);

        /*dcr nv close*/
        if (di->soh_dcr_dev.dcr_nv.dcr_control) {
            hisi_soh_err("[%s] nv close dcr!\n", __func__);
            return 0;
        }

        if (!di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[0].batt_dcr) {
            /*dcr first cal, order number init*/
            di->soh_dcr_dev.dcr_nv.order_num = -1;
            hisi_soh_err("[%s] order num init!\n", __func__);
        }
    } else {
        hisi_soh_err("[%s] nv rw fail!\n", __func__);
        return -1;
    }

    /*check drv ops register*/
    if (soh_drv_ops_check(di, DCR_DRV_OPS))
        return -1;

    /*register notifier for coul core*/
    di->soh_dcr_dev.soh_dcr_notify.notifier_call = soh_dcr_notifier_call;
    ret = hisi_coul_register_blocking_notifier(&di->soh_dcr_dev.soh_dcr_notify);
    if (ret < 0) {
       hisi_soh_err("[%s]soh_dcr_register_notifier failed!\n", __func__);
       return -1;
    }
    /*set dcr sample timer*/
    di->soh_dcr_dev.dcr_ops->set_dcr_timer(DCR_TIMER_32);

     /* Init interrupt notifier work */
    INIT_DELAYED_WORK(&di->soh_dcr_dev.dcr_work, soh_dcr_calculate_work);
    hisi_soh_info("[%s]suc!\n", __func__);
    return 0;
}

/**********************************************************
*  Function:       soh_dcr_uninit
*  Discription:    dcr uninit.
*  Input:          struct hisi_soh_device *di
*  Output:         NULL
*  return value:  0-success or others -fail.
**********************************************************/
static int soh_dcr_uninit(struct hisi_soh_device *di)
{
    if (!di)
        return -1;

    /*if dts not support,return success */
    if (!di->soh_dcr_dev.dcr_support) {
        hisi_soh_err("%s not support soh dcr!\n", __func__);
        return 0;
    }

    hisi_coul_unregister_blocking_notifier(&di->soh_dcr_dev.soh_dcr_notify);

    hisi_soh_info("[%s]suc!\n", __func__);
    return 0;
}
/**********************************************************
*  Function:      soh_pd_get_valid_data
*  Discription:   get valid pd leak current data.
*  Input:         struct pd_leak_chip_info *pdata : pd data array
*                 size:  valid pd fifo data number
*  Output:        pd leak data.
*  return value:  0 suc or others fail .
*  remak:         1 ocv [2500,4500]mv
*                 2 The greater the ocv, the shorter the corresponding rtc time
**********************************************************/
static int soh_pd_get_valid_data_cal_pd_current(struct hisi_soh_device *di, struct pd_leak_chip_info *pdata, u32 size)
{
    u32 i;
    int ret;
    int leak_cc_ua = 0;
    int max_current_ocv_uah = 0;
    int min_current_ocv_uah = 0;
    int ocv_interval;
    int sys_leak_cc;
    int max_num = 0;
    int min_num = 0;
    struct pd_leak_chip_info *max_value;
    struct pd_leak_chip_info *min_value;

    if (!pdata || !di || size < PD_OCV_CAL_MIN_NUM) {
        hisi_soh_err("[%s] not start pd ocv sample, size = %d", __func__, size);
        return -1;
    }

    max_value = min_value = pdata;

    for (i = 0; i < size; i++) {
        if ((pdata+i)->ocv_cur_ua > max_value->ocv_cur_ua) {
            max_value = pdata+i;
            max_num = i;
        }

        if ((pdata+i)->ocv_cur_ua <= min_value->ocv_cur_ua) {
            min_value = pdata+i;
            min_num = i;
        }
    }

    hisi_soh_info("[%s] [num:%d]max_current = %d, [num:%d]min_current = %d!\n", __func__, max_num, max_value->ocv_cur_ua, min_num, min_value->ocv_cur_ua);

    /*check ocv valid*/
    if ((min_value->ocv_vol_uv/UV_PER_MV > PD_OCV_MAX || min_value->ocv_vol_uv/UV_PER_MV < PD_OCV_MIN ) ||
                (max_value->ocv_vol_uv/UV_PER_MV > PD_OCV_MAX || max_value->ocv_vol_uv/UV_PER_MV < PD_OCV_MIN )) {

        hisi_soh_err("[%s] err:ocv is wrong, min = %d, max = %d uv!\n", __func__, min_value->ocv_vol_uv, max_value->ocv_vol_uv );
        return -1;
    }

    /*check ocv and rtc*/
    if (max_value->ocv_vol_uv > min_value->ocv_vol_uv && max_value->ocv_rtc > min_value->ocv_rtc ) {

        hisi_soh_err("[%s] err:rtc not match ocv,min rtc = %d, max rtc = %d uv!\n", __func__, min_value->ocv_rtc, max_value->ocv_rtc);
        /*return -1;*/
    }

    /*get pd leak cal data*/
    di->soh_pd_leak_dev.soh_pd_leak_current_info.batt_current[0] = min_value->ocv_cur_ua;
    di->soh_pd_leak_dev.soh_pd_leak_current_info.batt_current[1] = max_value->ocv_cur_ua;
    di->soh_pd_leak_dev.soh_pd_leak_current_info.batt_vol[0]     = min_value->ocv_vol_uv;
    di->soh_pd_leak_dev.soh_pd_leak_current_info.batt_vol[1]     = max_value->ocv_vol_uv;
    di->soh_pd_leak_dev.soh_pd_leak_current_info.chip_temp[0]    = min_value->ocv_chip_temp;
    di->soh_pd_leak_dev.soh_pd_leak_current_info.chip_temp[1]    = max_value->ocv_chip_temp;
    di->soh_pd_leak_dev.soh_pd_leak_current_info.rtc_time[0]     = min_value->ocv_rtc;
    di->soh_pd_leak_dev.soh_pd_leak_current_info.rtc_time[1]     = max_value->ocv_rtc;
    di->soh_pd_leak_dev.soh_pd_leak_current_info.batt_cycle      = hisi_battery_cycle_count();

    hisi_soh_info("[%s] [num:%d].ocv = %d, .temp = %d, [num:%d].ocv = %d, .temp = %d!\n", __func__,
                            max_num, max_value->ocv_vol_uv, max_value->ocv_chip_temp, min_num, min_value->ocv_vol_uv, min_value->ocv_chip_temp);

    /*get uah by ocv*/
    ret  =  hisi_coul_cal_uah_by_ocv(max_value->ocv_vol_uv, &max_current_ocv_uah);
    ret |=  hisi_coul_cal_uah_by_ocv(min_value->ocv_vol_uv, &min_current_ocv_uah);
    if (ret) {
        hisi_soh_err("[%s]cal uah fail\n", __func__);
        return -1;
    }
    /*cal pd leak current ma*/
    ocv_interval = abs(max_value->ocv_rtc - min_value->ocv_rtc);
    sys_leak_cc  = di->soh_pd_leak_dev.soh_pd_leak_current_info.sys_pd_leak_cc;

    if (ocv_interval)
        leak_cc_ua = (abs(max_current_ocv_uah - min_current_ocv_uah) - sys_leak_cc) * SEC_PER_H / ocv_interval;

    di->soh_pd_leak_dev.soh_pd_leak_current_info.leak_current_ma = leak_cc_ua/UA_PER_MA;

    hisi_soh_info("[%s] suc, ocv_interval_s = %d, pd_current = [%d]mA!\n", __func__, ocv_interval, leak_cc_ua/UA_PER_MA);
    return 0;

}
/**********************************************************
*  Function:      soh_pd_leak_cal
*  Discription:   cal pd leak current.
*  Input:         struct hisi_soh_device *di
*  Output:        pd leak current
*  return value:  NA .
**********************************************************/
static void soh_pd_leak_cal(struct hisi_soh_device *di)
{
    u32 fifo_cnt;
    u32 i;
    int ret;
    struct pd_leak_chip_info *pd_chip_data;

    if (!di)
        return ;

   /*get fifo count of pd current and vol*/
    fifo_cnt =  di->soh_pd_leak_dev.pd_leak_ops->get_pd_leak_fifo_depth();

   if (fifo_cnt > PD_FIFO_MAX) {
        hisi_soh_err("[%s] fifo err!\n", __func__);
        return ;
    }
    pd_chip_data = (struct pd_leak_chip_info *)kzalloc((sizeof(struct pd_leak_chip_info)*fifo_cnt), GFP_KERNEL);
    if (!pd_chip_data) {
        hisi_soh_err("[%s] pd_chip_data null!\n", __func__);
        return ;
    }

    /*get pd data from pd driver*/
    for (i = 0; i < fifo_cnt; i++) {
        di->soh_pd_leak_dev.pd_leak_ops->get_pd_leak_info(pd_chip_data+i, i);
        if (0 == (pd_chip_data +i)->ocv_vol_uv ) {
            hisi_soh_info("[%s] num:[%d]pd ocv = 0, exit!\n", __func__, i);
            break;
        }
    }
    /*cal pd leak current and save*/
    ret = soh_pd_get_valid_data_cal_pd_current(di, pd_chip_data, i);
    if (!ret) {
        ret = soh_rw_nv_info(di, SOH_PD_LEAK, NV_WRITE);
        if (!ret)
            hisi_soh_info("[%s] suc!\n", __func__);
        else
            hisi_soh_err("[%s] nv w fail!\n", __func__);
    } else
        hisi_soh_err("[%s] fail!\n", __func__);

    kfree(pd_chip_data);
}

/*****************************************************************************************************
  Function:        soh_pd_leak_calculate_work
  Description:     calculate pd leak ,check pd leak validity and save data into nv.
  Input:           struct work_struct *work
  Output:          NULL
  Return:          NULL
  Remark:          1 nve not readt in kernel start .
*******************************************************************************************************/
static void soh_pd_leak_calculate_work(struct work_struct *work)
{
    struct hisi_soh_device *di = container_of(work, struct hisi_soh_device, soh_pd_leak_dev.pd_leak_work.work);
    /*get pd leak info*/
    soh_pd_leak_cal(di);
}
/**********************************************************
*  Function:       soh_pd_leak_init
*  Discription:    pd leak init.
*  Input:          struct hisi_soh_device *di
*  Output:         NULL
*  return value:  0-success or others -fail.
**********************************************************/
static int soh_pd_leak_init(struct hisi_soh_device *di)
{
    if (!di)
        return -1;

    /*get dts*/
    if (parse_soh_pd_leak_dts(di))
        return -1;

    /*if dts not support,return success */
    if (!di->soh_pd_leak_dev.pd_leak_support) {
        hisi_soh_err("%s not support soh pd leak!\n", __func__);
        return 0;
    }

    /*get init data from NV*/
    /*ret = soh_rw_nv_info(di, SOH_PD_LEAK, NV_READ);*/
	if (boot_pdleak_nv_info) {
        memcpy_s(&di->soh_pd_leak_dev.pd_leak_nv, sizeof(struct pd_leak_nv_info), boot_pdleak_nv_info, sizeof(struct pd_leak_nv_info));
        soh_nv_printf(di, SOH_PD_LEAK);
        /*pd nv close*/
        if (di->soh_pd_leak_dev.pd_leak_nv.pd_control) {
            hisi_soh_err("[%s] nv close pd!\n", __func__);
            return 0;
        }
        if (!di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[0].batt_vol[0]) {
            /*pd first cal, order number init*/
            di->soh_pd_leak_dev.pd_leak_nv.order_num = -1;
            hisi_soh_err("[%s] order num init!\n", __func__);
        }

    } else {
        hisi_soh_err("[%s] nv rw fail!\n", __func__);
        return -1;
    }

    /*check drv ops register*/
    if (soh_drv_ops_check(di, PD_LEAK_DRV_OPS))
        return -1;

    /*enable pd ocv*/
    di->soh_pd_leak_dev.pd_leak_ops->enable_pd_leak(SOH_EN);

    /* Init pd leak cal  work */
    INIT_DELAYED_WORK(&di->soh_pd_leak_dev.pd_leak_work, soh_pd_leak_calculate_work);
    queue_delayed_work(system_power_efficient_wq, &di->soh_pd_leak_dev.pd_leak_work, msecs_to_jiffies(PD_WORK_DELAY_MS));

    hisi_soh_info("[%s]suc!\n", __func__);
    return 0;
}

/**********************************************************
*  Function:       soh_pd_leak_uninit
*  Discription:    pd leak uninit.
*  Input:          struct hisi_soh_device *di
*  Output:         NULL
*  return value:  0-success or others -fail.
**********************************************************/
static int soh_pd_leak_uninit(struct hisi_soh_device *di)
{
    if (!di)
        return -1;

    /*if dts not support,return success */
    if (!di->soh_pd_leak_dev.pd_leak_support) {
        hisi_soh_err("%s not support soh pd leak!\n", __func__);
        return 0;
    }

    hisi_soh_info("[%s]suc!\n", __func__);
    return 0;
}

/**********************************************************
*  Function:       soh_get_acr_resistance
*  Discription:    get acr info interface
*  Input:          NA
*  Output:         acr_info *r :acr info pointer
*                  enum acr_type type:acr type
*  return value:  0-success or others -fail.
**********************************************************/
int soh_get_acr_resistance(struct acr_info *r, enum acr_type type)
{
    struct hisi_soh_device *di = g_di;

    if (!r) {
        hisi_soh_err("%s pointer null!\n", __func__);
        return -1;
    }

    if (!di) {
        hisi_soh_err("%s g_di null!\n", __func__);
        return -1;
    }

    switch (type) {
    case ACR_H_PRECISION:
        /*Prevent array Overflow*/
        if (di->soh_acr_dev.acr_nv.order_num < 0 || di->soh_acr_dev.acr_nv.order_num >= SOH_ACR_NV_DATA_NUM)
            di->soh_acr_dev.acr_nv.order_num = -1;

        if (-1 == di->soh_acr_dev.acr_nv.order_num)
            memset_s(r, sizeof(struct acr_info), 0, sizeof(struct acr_info));
        else
            memcpy_s(r, sizeof(struct acr_info), &di->soh_acr_dev.acr_nv.soh_nv_acr_info[di->soh_acr_dev.acr_nv.order_num], sizeof(struct acr_info));
        break;

    case ACR_L_PRECISION:
        memcpy_s(r, sizeof(struct acr_info), &di->soh_acr_dev.soh_acr_info, sizeof(struct acr_info));
        break;
    default:
        break;
    }

    hisi_soh_info("[%s] num = %d, acr = %d, cycle = %d, vol = %d, batt_temp = %d, chip_temp[0] = %d, chip_temp[1] = %d!\n", __func__,
                        di->soh_acr_dev.acr_nv.order_num, r->batt_acr, r->batt_cycle, r->batt_vol, r->batt_temp, r->chip_temp[0],r->chip_temp[1]);
    return 0;
}
/**********************************************************
*  Function:       soh_get_dcr_resistance
*  Discription:    get dcr info interface
*  Input:          NA
*  Output:         dcr_info *r :dcr info pointer.
*  return value:  0-success or others -fail.
**********************************************************/
int soh_get_dcr_resistance(struct dcr_info *r)
{
    struct hisi_soh_device *di = g_di;

    if (!r) {
        hisi_soh_err("%s pointer null!\n", __func__);
        return -1;
    }

    if (!di) {
        hisi_soh_err("%s g_di null!\n", __func__);
        return -1;
    }
    /*Prevent array Overflow*/
    if (di->soh_dcr_dev.dcr_nv.order_num < 0 || di->soh_dcr_dev.dcr_nv.order_num >= SOH_DCR_NV_DATA_NUM)
        di->soh_dcr_dev.dcr_nv.order_num = -1;

    if (-1 == di->soh_dcr_dev.dcr_nv.order_num)
        memset_s(r, sizeof(struct dcr_info), 0, sizeof(struct dcr_info));
    else
        memcpy_s(r, sizeof(struct dcr_info), &di->soh_dcr_dev.dcr_nv.soh_nv_dcr_info[di->soh_dcr_dev.dcr_nv.order_num], sizeof(struct dcr_info));

    hisi_soh_info("[%s] num = %d, r0 = %d, dcr = %d, cycle = %d, vol = %d, batt_temp = %d!\n", __func__,
                        di->soh_dcr_dev.dcr_nv.order_num, r->batt_r0,r->batt_dcr, r->batt_cycle, r->batt_vol, r->batt_temp);
    return 0;
}
/**********************************************************
*  Function:       soh_get_poweroff_electric_leakage
*  Discription:    get poweroff leakage current info interface
*  Input:          NA
*  Output:        pd_leak_current_info *current :pd leakage info pointer.
*  return value:  0-success or others -fail.
**********************************************************/
int soh_get_poweroff_leakage(struct pd_leak_current_info *i)
{
    struct hisi_soh_device *di = g_di;

    if (!i) {
        hisi_soh_err("%s pointer null!\n", __func__);
        return -1;
    }

    if (!di) {
        hisi_soh_err("%s g_di null!\n", __func__);
        return -1;
    }
    /*Prevent array Overflow*/
    if (di->soh_pd_leak_dev.pd_leak_nv.order_num < 0 || di->soh_pd_leak_dev.pd_leak_nv.order_num >= SOH_PD_NV_DATA_NUM)
        di->soh_pd_leak_dev.pd_leak_nv.order_num = -1;

    if (-1 == di->soh_pd_leak_dev.pd_leak_nv.order_num)
        memset_s(i, sizeof(struct pd_leak_current_info), 0, sizeof(struct pd_leak_current_info));
    else
        memcpy_s(i, sizeof(struct pd_leak_current_info), &di->soh_pd_leak_dev.pd_leak_nv.soh_nv_pd_leak_current_info[di->soh_pd_leak_dev.pd_leak_nv.order_num], sizeof(struct pd_leak_current_info));

    hisi_soh_info("[%s] num = %d, leak_ma = %d, cycle = %d, sys_cc =%d, curr[0] =%d, curr[1] = %d,"
                       "vol[0] = %d, vol[1]=%d, chip_temp[0]=%d, chip_temp[1]=%d, rtc[0]=%d, rtc[1]=%d!\n", __func__,
                       di->soh_pd_leak_dev.pd_leak_nv.order_num, i->leak_current_ma, i->batt_cycle,
                       i->sys_pd_leak_cc, i->batt_current[0], i->batt_current[1],i->batt_vol[0],
                       i->batt_vol[1], i->chip_temp[0], i->chip_temp[1], i->rtc_time[0],i->rtc_time[1]);
    return 0;
}


/**********************************************************
*  Function:       soh_acr_calculate_start
*  Discription:    start acr calculate regardless of acr check conditions.
*  Input:          NA
*  Output:         acr info.
*  return value:   NA
*  Remark:         for factory aging acr test and debug.
**********************************************************/
void soh_acr_low_precision_cal_start(void)
{
#ifdef CONFIG_HISI_DEBUG_FS
    struct hisi_soh_device *di = g_di;
    if (!di)
        return;

    di->soh_acr_dev.acr_prec_type = ACR_L_PRECISION;
    queue_delayed_work(system_power_efficient_wq, &di->soh_acr_dev.acr_work, msecs_to_jiffies(0));
#endif
}


/*only for test*/

#ifdef  CONFIG_HISI_DEBUG_FS

void test_acr_task_schedule(void)
{
    struct hisi_soh_device *di = g_di;
    if (!di)
        return;
    queue_delayed_work(system_power_efficient_wq, &di->soh_acr_dev.acr_work, msecs_to_jiffies(0));
}


void test_dcr_task_schedule(void)
{
    struct hisi_soh_device *di = g_di;
    if (!di)
        return;
    queue_delayed_work(system_power_efficient_wq, &di->soh_dcr_dev.dcr_work, msecs_to_jiffies(0));
}

void test_ovp_task_schedule(void)
{
    struct hisi_soh_device *di = g_di;
    if (!di)
        return;
    queue_delayed_work(system_power_efficient_wq, &di->soh_ovp_dev.soh_ovp_work, msecs_to_jiffies(0));
}

int test_get_acr( enum acr_type type)
{
    struct acr_info r;
    return soh_get_acr_resistance(&r, type);
}

int test_get_dcr(void)
{
    struct dcr_info r;
    return soh_get_dcr_resistance(&r);
}

int test_get_pd_leak(void)
{
    struct pd_leak_current_info i;
    return soh_get_poweroff_leakage(&i);
}


void test_set_ovp_thd_value(enum soh_ovp_type type, int order, int temp, int vol_mv)
{
    struct hisi_soh_device *di = g_di;
    if (!di)
        return;

    if (SOH_OVH == type) {
        if (order >= 3 || order < 0)
            return;
        di->soh_ovp_dev.soh_ovh_thres[order].bat_vol_mv = vol_mv;
        di->soh_ovp_dev.soh_ovh_thres[order].temp       = temp;
    } else if (SOH_OVL == type){
        if (order >= 3 || order < 0)
            return;
        di->soh_ovp_dev.soh_ovl_thres[order].bat_vol_mv = vol_mv;
        di->soh_ovp_dev.soh_ovl_thres[order].temp       = temp;
    } else {
        di->soh_ovp_dev.soh_ovl_safe_thres.bat_vol_mv   = vol_mv;
        di->soh_ovp_dev.soh_ovl_safe_thres.temp         = temp;
    }
}
void test_set_ovp_protect(void)
{
    soh_ovp_set_protect_threshold(g_di);

}

void test_set_ovp_start_time(int time)
{
    struct hisi_soh_device *di =g_di;
    if (!di)
        return;

    di->soh_ovp_dev.soh_ovp_start_time = time;
}

void test_soh_nv_printf(enum soh_type type)
{
   soh_nv_printf(g_di, type);
}

void test_set_bat_cyc_inc(enum soh_type type, int cycle)
{
    if (SOH_ACR == type)
        acr_cycle_inc = cycle;
    if (SOH_DCR == type)
        dcr_cycle_inc = cycle;
}
void test_soh_report(int type)
{
    struct hisi_soh_device *di =g_di;
    if (!di)
        return;

    switch(type) {
    case 0:
        sysfs_notify(&di->dev->kobj, NULL, "acr_raw");
        break;
    case 1:
        sysfs_notify(&di->dev->kobj, NULL, "dcr_raw");
        break;
    default:
        break;
    }
    return ;
}
#endif

/*******************************************************
  Function:        soh_probe
  Description:     soh probe function
  Input:           struct platform_device *pdev -platform device
  Output:          NULL
  Return:          0 success, others fail
********************************************************/
static int soh_probe(struct platform_device *pdev)
{
    struct hisi_soh_device *di = NULL;
    int ret;

    di = (struct hisi_soh_device *)devm_kzalloc(&pdev->dev,sizeof(*di), GFP_KERNEL);
    if (!di) {
		hisi_soh_err("%s failed to alloc di struct\n",__func__);
		return -1;
    }

    di->dev =&pdev->dev;

    platform_set_drvdata(pdev, di);

    wake_lock_init(&di->soh_wake_lock, WAKE_LOCK_SUSPEND, "soh_wakelock");

    mutex_init(&di->soh_mutex);

    /*acr init, it doesn't affect other soh  modules if it fails.*/
    ret = soh_acr_init(di);
    if (ret) {
        hisi_soh_err("[%s]soh acr init fail!\n", __func__);
        goto soh_fail_0;
    }

    /*dcr init*/
    ret = soh_dcr_init(di);
    if (ret) {
        hisi_soh_err("[%s]soh dcr init fail!\n", __func__);
        goto soh_fail_1;
    }

    /*ovp init*/
    ret = soh_ovp_init(di);
    if (ret) {
        hisi_soh_err("[%s]soh ovp init fail!\n", __func__);
        goto soh_fail_2;
    }

    /*pd leak init*/
    ret = soh_pd_leak_init(di);
    if (ret) {
        hisi_soh_err("[%s]soh pd leak init fail!\n", __func__);
        goto soh_fail_3;
    }
#ifdef CONFIG_SYSFS
    ret = soh_create_sysfs_file(di);
    if (ret)
        goto soh_fail_4;
#endif
    g_di = di;
    hisi_soh_info("[%s]ok!\n", __func__);
    return 0;

soh_fail_4:
    soh_pd_leak_uninit(di);
soh_fail_3:
    soh_ovp_uninit(di);
soh_fail_2:
    soh_dcr_uninit(di);
soh_fail_1:
    soh_acr_uninit(di);
soh_fail_0:
    wake_lock_destroy(&di->soh_wake_lock);
    mutex_destroy(&di->soh_mutex);
    platform_set_drvdata(pdev, NULL);
    return ret;
}

/*******************************************************
  Function:        soh_remove
  Description:     remove function
  Input:           struct platform_device *pdev
  Output:          NULL
  Return:          0 success,other fail
********************************************************/
static int  soh_remove(struct platform_device *pdev)
{
    struct hisi_soh_device *di = platform_get_drvdata(pdev);

	if (!di) {
		hisi_soh_err("[%s]di is NULL!\n", __func__);
		return -ENODEV;
	}
    soh_acr_uninit(di);
    soh_dcr_uninit(di);
    soh_ovp_uninit(di);
    soh_pd_leak_uninit(di);
	wake_lock_destroy(&di->soh_wake_lock);
    mutex_destroy(&di->soh_mutex);
	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev,di);
	g_di = NULL;
	return 0;
}

static struct of_device_id hisi_soh_core_match_table[] =
{
    {
          .compatible = "hisi,soh_core",
    },
    { /*end*/},
};

static void soh_shutdown(struct platform_device *pdev)
{
    struct hisi_soh_device *di = platform_get_drvdata(pdev);
    if (!di)
        return;
    hisi_soh_err("[%s]ok!\n", __func__);
}

static struct platform_driver hisi_soh_core_driver = {
	.probe		= soh_probe,
	.remove		= soh_remove,
	.shutdown   = soh_shutdown,
	.driver		= {
	.name		= "hisi_soh_core",
       .owner          = THIS_MODULE,
       .of_match_table = hisi_soh_core_match_table,
	},
};

int __init soh_core_init(void)
{
    return platform_driver_register(&hisi_soh_core_driver);
}

void __exit soh_core_exit(void)
{
    platform_driver_unregister(&hisi_soh_core_driver);
}

late_initcall(soh_core_init);
module_exit(soh_core_exit);

module_init(soh_get_nv_info_from_boot);

MODULE_AUTHOR("HISILICON");
MODULE_DESCRIPTION("hisi soh module");
MODULE_LICENSE("GPL v2");
