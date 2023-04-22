

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/stat.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#endif
#include <linux/namei.h>

#include "oneimage.h"

#include "hisi_oneimage.h"
#include "board.h"
#include "plat_debug.h"

#ifdef CONFIG_HWCONNECTIVITY
/* global variables */
static char g_chip_type[HW_PROC_BUTT][BUFF_LEN];
struct hisi_proc_info g_proc_info_etc[] = {
    {.proc_type = HW_PROC_CHIPTYPE, .proc_node_name = DTS_COMP_HW_CONNECTIVITY_NAME, .proc_pro_name = HW_CONN_PROC_CHIPTYPE_FILE},
    {.proc_type = HW_PROC_SUPP, .proc_node_name = DTS_COMP_HW_HISI_SUPP_CONFIG_NAME, .proc_pro_name = HW_CONN_PROC_SUPP_FILE},
    {.proc_type = HW_PROC_P2P, .proc_node_name = DTS_COMP_HW_HISI_P2P_CONFIG_NAME, .proc_pro_name = HW_CONN_PROC_P2P_FILE},
    {.proc_type = HW_PROC_HOSTAPD, .proc_node_name = DTS_COMP_HW_HISI_HOSTAPD_CONFIG_NAME, .proc_pro_name = HW_CONN_PROC_HOSTAPD_FILE},
    {.proc_type = HW_PROC_FIRMWARE, .proc_node_name = DTS_COMP_HW_HISI_FIRMWARE_CONFIG_NAME, .proc_pro_name = HW_CONN_PROC_FRIMWARE},
    {},
};

/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
bool is_my_chip_etc(void)
{
    if (!isMyConnectivityChip(CHIP_TYPE_HI110X)) {
        PS_PRINT_ERR("cfg dev board chip type is not match hisi, skip driver init\n");
        return false;
    } else {
        PS_PRINT_INFO("cfg dev board type is matched with hisi, continue\n");
    }
    return true;
}

bool is_hisi_chiptype_etc(int32 chip)
{
    int32 ret= BOARD_FAIL;
    BOARD_INFO * bd_info = NULL;

    if(false == is_my_chip_etc())
    {
        return false;
    }

    ret = board_chiptype_init();
    if (BOARD_FAIL == ret)
    {
        PS_PRINT_ERR("sub chip type init fail\n");
         return false;
    }

    bd_info = get_hi110x_board_info_etc();
    if (unlikely(NULL == bd_info))
    {
        PS_PRINT_ERR("board info is null\n");
        return false;
    }

    if (chip != bd_info->chip_nr)
    {
        PS_PRINT_ERR("hi11xx sub chip type is not match, skip driver init\n");
        return false;
    }
    else
    {
        return true;
    }
}

EXPORT_SYMBOL(is_hisi_chiptype_etc);

bool is_my_nfc_chip_etc(void)
{
    if (!isMyNfcChip(NFC_CHIP_TYPE_HI110X)) {
        PS_PRINT_ERR("cfg dev board nfc chip type is not match, skip driver init\n");
        return false;
    } else {
        PS_PRINT_INFO("cfg dev board nfc type is matched with hisi_nfc, continue\n");
    }
    return true;
}
#endif
/*****************************************************************************
  3 Function Definition
*****************************************************************************/
/*Note:symbol printk() has arg.count conflict*/
/*lint -e515*/

int read_from_dts_etc(char *buf, int buf_len, char *node_name, char *property_name)
{
    struct device_node *np;
    char *dts_pro_name = NULL;
    int ret = 0;

    if (! buf || buf_len <= 0)
    {
        printk(KERN_ERR "[HW_CONN] invalid input parameters in %s\n", __func__);
        return -EINVAL;
    }

    np = of_find_compatible_node(NULL, NULL, node_name); // should be the same as dts node compatible property
    if (np == NULL) {
        /* if read fail, default is broadcom */
        printk(KERN_ERR "Unable to find hw_connectivity, using default {%s}\n", node_name);
        strncpy(buf, CHIP_TYPE_BCM, (size_t)(buf_len - 1));
        return 0;
    }

    ret = of_property_read_string(np, (const char*)property_name, (const char **)&dts_pro_name);
    if (ret < 0 || dts_pro_name == NULL) {
        /* if read fail, default is broadcom */
        printk(KERN_ERR "[HW_CONN] %s: get {%s}, ret:%d, using default\n", __func__, property_name, ret);
        strncpy(buf, CHIP_TYPE_BCM, (size_t)(buf_len - 1));
        ret = 0;
    } else {
        //printk(KERN_ERR "[HW_CONN] %s:  property:{%s}\n", __func__, property_name);
        strncpy(buf, dts_pro_name, (size_t)(buf_len - 1));
    }

    return ret;
}

static ssize_t hwconn_read_proc_chiptype(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < strlen(g_chip_type[HW_PROC_CHIPTYPE])) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is NULL or read length = %lu.\n",(unsigned long)len);
        return -EINVAL;
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %lu\n", (unsigned long)*off);
    if ((unsigned long)*off > 0) {
        return 0;
    }

    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_CHIPTYPE]);

    len = strlen(g_chip_type[HW_PROC_CHIPTYPE]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    if (copy_to_user(buffer, g_chip_type[HW_PROC_CHIPTYPE], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }

    *off += len;
    return len;
}

static ssize_t hwconn_read_proc_supp(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_INFO "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < strlen(g_chip_type[HW_PROC_SUPP])) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is NULL or read length = %lu.\n", (unsigned long)len);
        return -EINVAL;
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %lu\n", (unsigned long)*off);
    if ((unsigned long)*off > 0) {
        return 0;
    }

    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_SUPP]);

    len = strlen(g_chip_type[HW_PROC_SUPP]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    if (copy_to_user(buffer, g_chip_type[HW_PROC_SUPP], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }
    *off += len;
    return len;
}

static ssize_t hwconn_read_proc_p2p(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < strlen(g_chip_type[HW_PROC_P2P])) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is NULL or read length = %lu.\n", (unsigned long)len);
        return -EINVAL;
    }


    /*Note:symbol printk() has arg.count conflict*/
    /*lint -e516*/
    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %d\n", (int32)*off);
    /*lint +e516*/
    if ((unsigned long)*off > 0) {
        return 0;
    }

    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_P2P]);

    len = strlen(g_chip_type[HW_PROC_P2P]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    if (copy_to_user(buffer, g_chip_type[HW_PROC_P2P], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }
    *off += len;
    return len;
}

static ssize_t hwconn_read_proc_apd(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < strlen(g_chip_type[HW_PROC_HOSTAPD])) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is NULL or read length = %lu.\n", (unsigned long)len);
        return -EINVAL;
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %lu\n", (unsigned long)*off);
    if ((unsigned long)*off > 0) {
        return 0;
    }

    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_HOSTAPD]);

    len = strlen(g_chip_type[HW_PROC_HOSTAPD]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    if (copy_to_user(buffer, g_chip_type[HW_PROC_HOSTAPD], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }
    *off += len;
    return len;
}

static ssize_t hwconn_read_proc_firmware(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < strlen(g_chip_type[HW_PROC_FIRMWARE])) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is NULL or read length = %lu.\n", (unsigned long)len);
        return -EINVAL;
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %lu\n", (unsigned long)*off);
    if ((unsigned long)*off > 0) {
        return 0;
    }

    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_FIRMWARE]);

    len = strlen(g_chip_type[HW_PROC_FIRMWARE]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    if (copy_to_user(buffer, g_chip_type[HW_PROC_FIRMWARE], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }
    *off += len;
    return len;
}


static const struct file_operations hwconn_proc_fops[]= {
    {
       .owner = THIS_MODULE,
       .read = hwconn_read_proc_chiptype,
    },
    {
        .owner = THIS_MODULE,
        .read = hwconn_read_proc_supp,
    },
    {
        .owner = THIS_MODULE,
        .read = hwconn_read_proc_p2p,
    },
    {
        .owner = THIS_MODULE,
        .read = hwconn_read_proc_apd,
    },
        {
        .owner = THIS_MODULE,
        .read = hwconn_read_proc_firmware,
    },
    {},
};

int create_hwconn_proc_file(void)
{
    int ret = 0;
    int index;
    char node_path[NODE_PATH_LEN];
    struct path                         path;
    struct proc_dir_entry *hwconn_dir = NULL;
    struct proc_dir_entry *hwconn_chiptype_file = NULL;

    ret = kern_path(PROC_CONN_DIR, LOOKUP_FOLLOW, &path);
    if (ret)
    {
        hwconn_dir = proc_mkdir(HW_CONN_PROC_DIR, NULL);
        if (!hwconn_dir) {
            printk(KERN_ERR "[HW_CONN] proc dir create failed\n");
            ret = -ENOMEM;
            return ret;
        }

        index = HW_PROC_CHIPTYPE;
    }
    else
    {
        index = HW_PROC_SUPP;
    }

    for (; index < HW_PROC_BUTT; index++)
    {
        memset((void*)g_chip_type[index], 0, sizeof(g_chip_type[index]));
        ret = read_from_dts_etc(g_chip_type[index], sizeof(g_chip_type[index]), g_proc_info_etc[index].proc_node_name, g_proc_info_etc[index].proc_pro_name);
        if (ret < 0)
        {
            printk(KERN_INFO "[HW_CONN read info from dts fail\n");
            continue;
        }

        if (NULL != hwconn_dir)
        {
            hwconn_chiptype_file = proc_create(g_proc_info_etc[index].proc_pro_name, S_IRUGO, hwconn_dir, &hwconn_proc_fops[index]);
        }
        else
        {
            memset(node_path, 0, NODE_PATH_LEN);
            snprintf(node_path, NODE_PATH_LEN, "%s/%s", HW_CONN_PROC_DIR, g_proc_info_etc[index].proc_pro_name);
            printk(KERN_DEBUG "[HW_CONN node_path[%s]\n", node_path);
            hwconn_chiptype_file = proc_create(node_path, S_IRUGO, NULL, &hwconn_proc_fops[index]);
        }

        if (!hwconn_chiptype_file)
        {
            printk(KERN_ERR "[HW_CONN] proc info file create failed\n");
            ret = -ENOMEM;
            return ret;
        }
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn create proc file ok.\n");

    return ret;
}

static int hw_connectivity_probe(struct platform_device *pdev)
{
    int ret = 0;

    UNREF_PARAM(pdev);
    ret = create_hwconn_proc_file();
    if (ret)
    {
        printk(KERN_ERR "[HW_CONN] create proc file failed.\n");
        return ret;
    }

    return 0;
}

static void hw_connectivity_shutdown(struct platform_device *pdev)
{
    UNREF_PARAM(pdev);
    printk(KERN_INFO "[HW_CONN] hw_connectivity_shutdown!\n");
    return;
}

static const struct of_device_id hw_connectivity_match_table[] = {
    {
        .compatible = DTS_COMP_HW_CONNECTIVITY_NAME,   // compatible must match with which defined in dts
        .data = NULL,
    },
    {},
};

static struct platform_driver hw_connectivity_driver = {
    .probe          = hw_connectivity_probe,
    .suspend        = NULL,
    .remove         = NULL,
    .shutdown       = hw_connectivity_shutdown,
    .driver = {
        .name = DTS_COMP_HW_CONNECTIVITY_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(hw_connectivity_match_table), // dts required code
    },
};

int hw_misc_connectivity_init_etc(void)
{
    printk(KERN_ERR "[HW_CONN] hw_connectivity_init enter\n");
    return platform_driver_register(&hw_connectivity_driver);
}

void hw_misc_connectivity_exit_etc(void)
{
    platform_driver_unregister(&hw_connectivity_driver);
}

int read_nfc_conf_name_from_dts_etc(char *buf, int buf_len, char *node_name, char *property_name)
{
    return read_from_dts_etc(buf, buf_len, node_name, property_name);
}
/*lint +e515*/
//module_init(hw_misc_connectivity_init_etc);
//module_exit(hw_misc_connectivity_exit_etc);
#else
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
bool is_my_chip_etc(void)
{
    return true;
}

bool is_hisi_chiptype_etc(int32 chip)
{
    return true;
}

bool is_my_nfc_chip_etc(void)
{
    return true;
}
#endif

#endif

MODULE_AUTHOR("DRIVER_AUTHOR");
MODULE_DESCRIPTION("Huawei connectivity OneTrack driver");
MODULE_LICENSE("GPL");

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
