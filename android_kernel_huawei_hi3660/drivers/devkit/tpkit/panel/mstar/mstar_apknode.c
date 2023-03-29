/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Johnson Yeh
 * Maintain: Luca Hsu, Tigers Huang, Dicky Chiang
 */

#include "mstar_apknode.h"
#include "mstar_mp.h"

struct apk_data_info *apk_info;

void mstar_apknode_open_leather_sheath(void)
{
    s32 rc = 0;
    u8 szTxData[3] = { 0 };
    u32 i = 0, count = 5;

    tskit_mstar_data->finger_touch_disable = TRUE; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x02;
    szTxData[2] = 0x01;

    mutex_lock(&tskit_mstar_data->mutex_common);

    while (i < count) {
        msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc >= 0) {
            apk_info->ans.g_IsEnableLeatherSheathMode = 1;

            TS_LOG_INFO("Open leather sheath mode success\n");
            break;
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("Open leather sheath mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&tskit_mstar_data->mutex_common);

    tskit_mstar_data->finger_touch_disable = FALSE; // Resume finger touch ISR handling after device driver have sent i2c command to firmware.
}

void mstar_apknode_close_leath_sheath(void)
{
    s32 rc = 0;
    u8 szTxData[3] = { 0 };
    u32 i = 0, count = 5;

    tskit_mstar_data->finger_touch_disable = TRUE; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x02;
    szTxData[2] = 0x00;

    mutex_lock(&tskit_mstar_data->mutex_common);

    while (i < count) {
        msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc >= 0) {
            apk_info->ans.g_IsEnableLeatherSheathMode = 0;

            TS_LOG_INFO("Close leather sheath mode success\n");
            break;
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("Close leather sheath mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&tskit_mstar_data->mutex_common);

    tskit_mstar_data->finger_touch_disable = FALSE; // Resume finger touch ISR handling after device driver have sent i2c command to firmware.
}

void mstar_apknode_open_glove(void)
{
    s32 rc = 0;
    u8 szTxData[3] = { 0 };
    u32 i = 0, count = 5;

    tskit_mstar_data->finger_touch_disable = TRUE; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x01;
    szTxData[2] = 0x01;

    mutex_lock(&tskit_mstar_data->mutex_common);

    while (i < count) {
        msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc >= 0) {
            apk_info->ans.g_IsEnableGloveMode = 1;

            TS_LOG_INFO("Open glove mode success\n");
            break;
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("Open glove mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&tskit_mstar_data->mutex_common);

    tskit_mstar_data->finger_touch_disable = FALSE; // Resume finger touch ISR handling after device driver have sent i2c command to firmware.
}

void mstar_apknode_close_glove(void)
{
    s32 rc = 0;
    u8 szTxData[3] = { 0 };
    u32 i = 0, count = 5;

    tskit_mstar_data->finger_touch_disable = TRUE; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x01;
    szTxData[2] = 0x00;

    mutex_lock(&tskit_mstar_data->mutex_common);

    while (i < count) {
        msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc >= 0) {
            apk_info->ans.g_IsEnableGloveMode = 0;

            TS_LOG_INFO("Close glove mode success\n");
            break;
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("Close glove mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&tskit_mstar_data->mutex_common);

    tskit_mstar_data->finger_touch_disable = FALSE; // Resume finger touch ISR handling after device driver have sent i2c command to firmware.
}

void mstar_apknode_get_glove_info(u8 * pGloveMode)
{
    u8 szTxData[3] = { 0 };
    u8 szRxData[2] = { 0 };
    u32 i = 0, count = 5;
    s32 rc = 0;

    tskit_mstar_data->finger_touch_disable = TRUE; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x01;
    szTxData[2] = 0x02;

    mutex_lock(&tskit_mstar_data->mutex_common);

    while (i < count) {
        msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc >= 0) {
            TS_LOG_INFO("Get glove info mstar_iic_write_data() success\n");
        }

        msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &szRxData[0], 1);
        if (rc >= 0) {
            TS_LOG_INFO("Get glove info mstar_iic_read_data() success\n");

            if (szRxData[0] == 0x00 || szRxData[0] == 0x01) {
                break;
            } else {
                i = 0;
            }
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("Get glove info failed, rc = %d\n", rc);
    }

    mutex_unlock(&tskit_mstar_data->mutex_common);

    *pGloveMode = szRxData[0];

    TS_LOG_INFO("*pGloveMode = 0x%x\n", *pGloveMode);

    tskit_mstar_data->finger_touch_disable = FALSE; // Resume finger touch ISR handling after device driver have sent i2c command to firmware.
}

static void mstar_set_film_mode(u8 nFilmtype)
{
    u8 szTxData[2] = { 0x13, nFilmtype };
    s32 rc;

    msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 2);
    if (rc >= 0) {
        TS_LOG_INFO("Set Film Mode success,\n");
    }
}

static int mstar_get_film_mode(void)
{
    u8 szTxData[1] = { 0x12 };
    u8 szRxData[3] = { 0 };
    s32 rc = 0;

    msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
    if (rc >= 0) {
        TS_LOG_INFO("Get firmware info mstar_iic_write_data() success\n");
    }
    msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
    rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &szRxData[0], 3);
    if (rc >= 0) {
        if (szRxData[1] == FEATURE_FILM_MODE_LOW_STRENGTH) {
            TS_LOG_INFO("Film mode: Low strength\n");
            return FEATURE_FILM_MODE_LOW_STRENGTH;
        } else if (szRxData[1] == FEATURE_FILM_MODE_HIGH_STRENGTH) {
            TS_LOG_INFO("Film mode: High strength\n");
            return FEATURE_FILM_MODE_HIGH_STRENGTH;
        } else if (szRxData[1] == FEATURE_FILM_MODE_DEFAULT) {
            TS_LOG_INFO("Film mode: Default\n");
            return FEATURE_FILM_MODE_DEFAULT;
        }
    }
    return -1;
}

static ssize_t mstar_jni_msg_tool_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    u8 nBusType = 0;
    u16 nReadLen = 0;
    u8 szCmdData[20] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        return 0;
    }

    nBusType = nCount & 0xFF;
    nReadLen = (nCount >> 8) & 0xFFFF;

    if (nReadLen > sizeof(szCmdData)) {
       TS_LOG_ERR("Count(%d) is larger than buffer size (%d)\n", nReadLen, sizeof(szCmdData));
       goto out;
    }

    if (nBusType == SLAVE_I2C_ID_DBBUS || nBusType == SLAVE_I2C_ID_DWI2C) {
		msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        mstar_iic_read_data(nBusType, &szCmdData[0], nReadLen);
    }

	if (copy_to_user(pBuffer, &szCmdData[0], nReadLen)) {
            TS_LOG_INFO("copy to user error\n");
            return -EFAULT;
        }
out:
    *pPos += nReadLen;
     return nReadLen;
}

static void mstar_get_leather_sheath_info(u8 * pLeatherSheathMode)  // used for MSG28xx only
{
    u8 szTxData[3] = { 0 };
    u8 szRxData[2] = { 0 };
    u32 i = 0, count = 5;
    s32 rc = 0;

    tskit_mstar_data->finger_touch_disable = TRUE; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x02;
    szTxData[2] = 0x02;

    mutex_lock(&tskit_mstar_data->mutex_common);

    while (i < count) {
        msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc >= 0) {
            TS_LOG_INFO("Get leather sheath info mstar_iic_write_data() success\n");
        }

        msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &szRxData[0], 1);
        if (rc >= 0) {
            TS_LOG_INFO("Get leather sheath info mstar_iic_read_data() success\n");

            if (szRxData[0] == 0x00 || szRxData[0] == 0x01) {
                break;
            } else {
                i = 0;
            }
        }

        i++;
    }
    if (i == count) {
        TS_LOG_INFO("Get leather sheath info failed, rc = %d\n", rc);
    }

    mutex_unlock(&tskit_mstar_data->mutex_common);

    *pLeatherSheathMode = szRxData[0];

    TS_LOG_INFO("*pLeatherSheathMode = 0x%x\n", *pLeatherSheathMode);

    tskit_mstar_data->finger_touch_disable = FALSE; // Resume finger touch ISR handling after device driver have sent i2c command to firmware.
}



#ifdef CONFIG_ENABLE_JNI_INTERFACE

u64 mstar_ptr_to_u64(u8 * pValue)
{
    uintptr_t nValue = (uintptr_t) pValue;
    return (u64) (0xFFFFFFFFFFFFFFFF & nValue);
}

u8 *U64ToPtr(u64 nValue)
{
    uintptr_t pValue = (uintptr_t) nValue;
    return (u8 *) pValue;
}

static void mstar_jni_reg_get_xbyte(MsgToolDrvCmd_t * pCmd)
{
    u16 nAddr = 0;
    nAddr = (apk_info->snd_cmd_data[1] << 8) | apk_info->snd_cmd_data[0];
    mstar_get_reg_xbit(nAddr, apk_info->rtn_cmd_data, pCmd->nRtnCmdLen, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
}

static void mstar_jni_clear_msgtool_mem(void)
{
    memset(apk_info->msg_tool_cmd_in, 0, sizeof(MsgToolDrvCmd_t));
    memset(apk_info->snd_cmd_data, 0, 1024);
    memset(apk_info->snd_cmd_data, 0, 1024);
}

static MsgToolDrvCmd_t *mstar_jni_trans_cmd_from_user(unsigned long nArg)
{
    long nRet;
    MsgToolDrvCmd_t tCmdIn;
    MsgToolDrvCmd_t *pTransCmd;
    mstar_jni_clear_msgtool_mem();

    pTransCmd = (MsgToolDrvCmd_t *) apk_info->msg_tool_cmd_in;
    nRet = copy_from_user(&tCmdIn, (void *)nArg, sizeof(MsgToolDrvCmd_t));
	if(nRet) {
		TS_LOG_ERR("%s:copy_from_user() failed\n",__func__);
	}
    pTransCmd->nCmdId = tCmdIn.nCmdId;

    if (tCmdIn.nSndCmdLen > 0) {
        pTransCmd->nSndCmdLen = tCmdIn.nSndCmdLen;
        nRet = copy_from_user(apk_info->snd_cmd_data, U64ToPtr(tCmdIn.nSndCmdDataPtr), pTransCmd->nSndCmdLen);
		if(nRet) {
			TS_LOG_ERR("%s:copy_from_user() failed\n",__func__);
		}
    }

    if (tCmdIn.nRtnCmdLen > 0) {
        pTransCmd->nRtnCmdLen = tCmdIn.nRtnCmdLen;
        nRet = copy_from_user(apk_info->rtn_cmd_data, U64ToPtr(tCmdIn.nRtnCmdDataPtr), pTransCmd->nRtnCmdLen);
		if(nRet) {
			TS_LOG_ERR("%s:copy_from_user() failed\n",__func__);
		}
    }

    return pTransCmd;
}

static void mstar_jni_trans_cmd_to_user(MsgToolDrvCmd_t * pTransCmd, unsigned long nArg)
{
    MsgToolDrvCmd_t tCmdOut;
    long nRet;

    nRet = copy_from_user(&tCmdOut, (void *)nArg, sizeof(MsgToolDrvCmd_t));
	if(nRet) {
		TS_LOG_ERR("%s:copy_from_user() failed\n",__func__);
	}

    nRet = copy_to_user(U64ToPtr(tCmdOut.nRtnCmdDataPtr), apk_info->rtn_cmd_data, tCmdOut.nRtnCmdLen);
	if(nRet) {
		TS_LOG_ERR("%s:copy_to_user() failed\n",__func__);
	}
}

void mstar_apknode_create_jni_msg(void)
{
    apk_info->msg_tool_cmd_in = (MsgToolDrvCmd_t *) kmalloc(sizeof(MsgToolDrvCmd_t), GFP_KERNEL);
    apk_info->snd_cmd_data = (u8 *) kmalloc(1024, GFP_KERNEL);
    apk_info->rtn_cmd_data = (u8 *) kmalloc(1024, GFP_KERNEL);
}

void mstar_apknode_delete_jni_msg(void)
{
    if (apk_info->msg_tool_cmd_in) {
        kfree(apk_info->msg_tool_cmd_in);
        apk_info->msg_tool_cmd_in = NULL;
    }

    if (apk_info->snd_cmd_data) {
        kfree(apk_info->snd_cmd_data);
        apk_info->snd_cmd_data = NULL;
    }

    if (apk_info->rtn_cmd_data) {
        kfree(apk_info->rtn_cmd_data);
        apk_info->rtn_cmd_data = NULL;
    }
}
#endif //CONFIG_ENABLE_JNI_INTERFACE

static struct proc_dir_entry *g_proc_class_entry = NULL;
static struct proc_dir_entry *g_proc_ts_msg20xx_entry = NULL;
static struct proc_dir_entry *g_proc_device_entry = NULL;
static struct proc_dir_entry *g_proc_chip_type_entry = NULL;
static struct proc_dir_entry *g_proc_fw_data_entry = NULL;
static struct proc_dir_entry *g_proc_fw_update_entry = NULL;
static struct proc_dir_entry *g_proc_cust_fw_ver_entry = NULL;
static struct proc_dir_entry *g_proc_plateform_fw_ver_entry = NULL;
static struct proc_dir_entry *g_proc_drv_ver_entry = NULL;
static struct proc_dir_entry *g_proc_sd_fw_update_entry = NULL;
static struct proc_dir_entry *g_proc_fw_debug_entry = NULL;
static struct proc_dir_entry *g_proc_fw_set_debug_value_entry = NULL;
static struct proc_dir_entry *g_proc_fw_smbus_debug_entry = NULL;
static struct proc_dir_entry *g_proc_fw_set_dqmem_value_entry = NULL;
static struct proc_dir_entry *g_proc_fw_mode_entry = NULL;
static struct proc_dir_entry *g_proc_fw_sensor_entry = NULL;
static struct proc_dir_entry *g_proc_fw_packet_header_entry = NULL;
static struct proc_dir_entry *g_proc_query_feature_support_status_entry = NULL;
static struct proc_dir_entry *g_proc_change_feature_support_status_entry = NULL;
static struct proc_dir_entry *g_proc_gesture_wakeup_entry = NULL;
static struct proc_dir_entry *g_proc_gesture_info_entry = NULL;
static struct proc_dir_entry *g_proc_glove_entry = NULL;
static struct proc_dir_entry *g_proc_open_glove_entry = NULL;
static struct proc_dir_entry *g_proc_close_glove_entry = NULL;
static struct proc_dir_entry *g_proc_leather_sheath_entry = NULL;
static struct proc_dir_entry *g_proc_film_entry = NULL;
#ifdef CONFIG_ENABLE_JNI_INTERFACE
static struct proc_dir_entry *g_proc_jni_method_entry = NULL;
#endif //CONFIG_ENABLE_JNI_INTERFACE
static struct proc_dir_entry *g_proc_selinux_fw_update_entry = NULL;
static struct proc_dir_entry *g_proc_force_fw_update_entry = NULL;
#ifdef MP_TEST_FUNCTION_2
static struct proc_dir_entry *g_proc_mp_test_customised_entry = NULL;
#endif
static struct proc_dir_entry *g_proc_trim_code_entry = NULL;

#ifdef MP_TEST_FUNCTION_2
static ssize_t mstar_apknode_mp_test_customised_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                               loff_t * pPos)
{
    return nCount;
}

static ssize_t mstar_apknode_mp_test_customised_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                              loff_t * pPos)
{
    return 0;
}
#endif

static ssize_t mstar_apknode_chip_type_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    nLength = sprintf(nUserTempBuffer, "%d\n", tskit_mstar_data->chip_type);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength))
        return -EFAULT;

    TS_LOG_INFO("tskit_mstar_data->chip_type = 0x%x, tskit_mstar_data->chip_type_ori = 0x%x\n", tskit_mstar_data->chip_type, tskit_mstar_data->chip_type_ori);

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_chip_type_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                          loff_t * pPos)
{
    return nCount;
}

static ssize_t mstar_apknode_fw_data_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    TS_LOG_INFO("%s() g_fw_data_cont = %d\n", __func__, tskit_mstar_data->fw_update_data.fw_data_cont);
    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    *pPos += tskit_mstar_data->fw_update_data.fw_data_cont;

    return tskit_mstar_data->fw_update_data.fw_data_cont;
}

static ssize_t mstar_apknode_fw_data_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                        loff_t * pPos)
{
    u32 nNum = nCount / 1024;
    u32 nRemainder = nCount % 1024;
    u32 i;

     if( nCount > 1025){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        return nCount;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        return nCount;
    }

    if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data)) {
        TS_LOG_ERR("FW Count is zero\n");

        tskit_mstar_data->fw_update_data.two_dimen_fw_data = (u8 **) kmalloc(130 * sizeof(u8 *), GFP_KERNEL);
        if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data)) {
            TS_LOG_ERR("Failed to allocate FW buffer\n");
            goto out;
        }
    }

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG22XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        if (nNum > 0)   // nCount >= 1024
        {
            for (i = 0; i < nNum; i++) {
                memset(apk_info->debug_buf, 0, 1024);
                if (copy_from_user(apk_info->debug_buf, pBuffer + (i * 1024), 1024)) {
                    TS_LOG_INFO("copy_from_user() failed\n");

                    return -EFAULT;
                }

                tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont] = (u8 *) kmalloc(1024 * sizeof(u8), GFP_KERNEL);
                if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont])) {
                    TS_LOG_ERR("Failed to allocate FW buffer\n");
                    goto out;
                }

                memcpy(tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont], apk_info->debug_buf, 1024);
                if(tskit_mstar_data->fw_update_data.fw_data_cont < 130){
                    tskit_mstar_data->fw_update_data.fw_data_cont++;
                }
                else{
                    tskit_mstar_data->fw_update_data.fw_data_cont = 130;
                }
            }

            if (nRemainder > 0) // Handle special firmware size like MSG22XX(48.5KB)
            {
                TS_LOG_INFO("nRemainder = %d\n", nRemainder);
                memset(apk_info->debug_buf, 0, 1024);
                if (copy_from_user(apk_info->debug_buf, pBuffer + (i * 1024), nRemainder)) {
                    TS_LOG_INFO("copy_from_user() failed\n");

                    return -EFAULT;
                }

                tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont] = (u8 *) kmalloc(1024 * sizeof(u8), GFP_KERNEL);
                if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont])) {
                    TS_LOG_ERR("Failed to allocate FW buffer\n");
                    goto out;
                }

                memcpy(tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont], apk_info->debug_buf, nRemainder);

                if(tskit_mstar_data->fw_update_data.fw_data_cont < 130){
                    tskit_mstar_data->fw_update_data.fw_data_cont++;
                }
                else{
                    tskit_mstar_data->fw_update_data.fw_data_cont = 130;
                }
            }
        } else      // nCount < 1024
        {
            if (nCount > 0) {
                memset(apk_info->debug_buf, 0, 1024);
                if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
                    TS_LOG_INFO("copy_from_user() failed\n");

                    return -EFAULT;
                }

                tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont] = (u8 *) kmalloc(1024 * sizeof(u8), GFP_KERNEL);
                if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont])) {
                    TS_LOG_ERR("Failed to allocate FW buffer\n");
                    goto out;
                }

                memcpy(tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont], apk_info->debug_buf, nCount);

                if(tskit_mstar_data->fw_update_data.fw_data_cont < 130){
                    tskit_mstar_data->fw_update_data.fw_data_cont++;
                }
                else{
                    tskit_mstar_data->fw_update_data.fw_data_cont = 130;
                }
            }
        }
    } else {
        TS_LOG_INFO("Unsupported chip type = 0x%x\n", tskit_mstar_data->chip_type);
    }

    TS_LOG_INFO("g_fw_data_cont = %d\n", tskit_mstar_data->fw_update_data.fw_data_cont);

    if (apk_info->debug_buf != NULL) {
        TS_LOG_INFO("buf[0] = %c\n", apk_info->debug_buf[0]);
    }

    return nCount;

out:
    if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data)) {
        for (i = 0; i < 130; i++) {
            if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i])) {
                kfree(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i]);
                tskit_mstar_data->fw_update_data.two_dimen_fw_data[i] = NULL;
            }
        }

        kfree(tskit_mstar_data->fw_update_data.two_dimen_fw_data);
        tskit_mstar_data->fw_update_data.two_dimen_fw_data = NULL;
    }
    return nCount;
}

static ssize_t mstar_apknode_fw_update_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    nLength = sprintf(nUserTempBuffer, "%d", apk_info->update_complete);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength))
        return -EFAULT;

    TS_LOG_INFO("g_update_complete = %d\n", apk_info->update_complete);

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_update_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                          loff_t * pPos)
{
    int i = 0;
    s32 nRetVal = 0;

    TS_LOG_INFO("%s() g_fw_data_cont = %d\n", __func__, tskit_mstar_data->fw_update_data.fw_data_cont);

    if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data) || tskit_mstar_data->fw_update_data.fw_data_cont == 0) {
        TS_LOG_ERR("FW Buffer is NULL\n");
        goto out;
    }

    tskit_mstar_data->fw_update_data.one_dimen_fw_data = vmalloc(MSG28XX_FIRMWARE_WHOLE_SIZE * 1024 * sizeof(u8));
    if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.one_dimen_fw_data)) {
        TS_LOG_ERR("Failed to allocate FW buffer\n");
        goto out;
    }

    mstar_finger_touch_report_disable();

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG22XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        nRetVal = mstar_update_fw(tskit_mstar_data->fw_update_data.two_dimen_fw_data, EMEM_ALL);
    } else {
        TS_LOG_INFO("This chip type (0x%x) does not support update firmware by MTPTool APK\n", tskit_mstar_data->chip_type);
        nRetVal = -1;
    }

    if (0 != nRetVal) {
        apk_info->update_complete = 0;
        TS_LOG_INFO("Update FAILED\n");
    } else {
        apk_info->update_complete = 1;
        TS_LOG_INFO("Update SUCCESS\n");
    }

    mstar_finger_touch_report_enable();

out:
    if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data)) {
        for (i = 0; i < 130; i++) {
            if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i])) {
                kfree(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i]);
                tskit_mstar_data->fw_update_data.two_dimen_fw_data[i] = NULL;
            }
        }

        kfree(tskit_mstar_data->fw_update_data.two_dimen_fw_data);
        tskit_mstar_data->fw_update_data.two_dimen_fw_data = NULL;
    }
    if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.one_dimen_fw_data)) {
        vfree(tskit_mstar_data->fw_update_data.one_dimen_fw_data);
        tskit_mstar_data->fw_update_data.one_dimen_fw_data = NULL;
    }
    return nCount;
}

static ssize_t mstar_apknode_customer_fw_ver_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                           loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };
    char fw_version[32] = {0};
    int ret = 0;

    TS_LOG_INFO("%s() cust_ver = %s\n", __func__, tskit_mstar_data->fw_data.cust_ver);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }
    ret = mstar_get_customer_fw_ver(fw_version);
    if(ret < 0){
		TS_LOG_ERR("%s:read fw_ver fail, ret=%d\n", __func__, ret);
    }else{
		sprintf(tskit_mstar_data->fw_data.cust_ver,"%s",fw_version);
		snprintf(tskit_mstar_data->mstar_chip_data->version_name,MAX_STR_LEN-1,"%s",fw_version);
    }
    nLength = snprintf(nUserTempBuffer, sizeof(nUserTempBuffer), "%s\n", tskit_mstar_data->fw_data.cust_ver);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }
    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_customer_fw_ver_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                            loff_t * pPos)
{
    char fw_version[32] = {0};
    int res = 0;

    res = mstar_get_customer_fw_ver(fw_version);
    if(res < 0){
		TS_LOG_ERR("%s:read fw_ver fail, ret=%d\n", __func__, res);
    }else{
		sprintf(tskit_mstar_data->fw_data.cust_ver,"%s",fw_version);
		snprintf(tskit_mstar_data->mstar_chip_data->version_name,MAX_STR_LEN-1,"%s",fw_version);
    }
    TS_LOG_INFO("%s() cust_ver = %s\n", __func__, tskit_mstar_data->fw_data.cust_ver);

    return nCount;
}

static ssize_t mstar_apknode_platform_fw_ver_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                           loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };
    TS_LOG_INFO("%s() platform_inter_ver = %s\n", __func__, tskit_mstar_data->fw_data.platform_inter_ver);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }
    nLength = snprintf(nUserTempBuffer, sizeof(nUserTempBuffer), "%s", tskit_mstar_data->fw_data.platform_inter_ver);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_platform_fw_ver_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                            loff_t * pPos)
{
    int ret = NO_ERR;
    TS_LOG_INFO("%s() enter\n", __func__);
    ret = mstar_get_platform_fw_ver(&tskit_mstar_data->fw_data.platform_inter_ver);
    if(ret < 0){
		TS_LOG_ERR("%s:read platform fw ver fail, ret=%d\n", __func__, ret);
    }

    TS_LOG_INFO("%s() platform_inter_ver = %s\n", __func__, tskit_mstar_data->fw_data.platform_inter_ver);

    return nCount;
}

static ssize_t mstar_apknode_drive_ver_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };
    TS_LOG_INFO("%s() enter\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }
    nLength = sprintf(nUserTempBuffer, "%s", DEVICE_DRIVER_RELEASE_VERSION);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_drive_ver_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                          loff_t * pPos)
{
    return nCount;
}

static ssize_t mstar_apknode_fw_sdcard_update_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                            loff_t * pPos)
{
    char fw_version[32] = {0};
    int ret = 0;
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    ret = mstar_get_customer_fw_ver(fw_version);
    if(ret < 0){
		TS_LOG_ERR("%s:read fw_ver fail, ret=%d\n", __func__, ret);
    }else{
		sprintf(tskit_mstar_data->fw_data.cust_ver,"%s",fw_version);
		snprintf(tskit_mstar_data->mstar_chip_data->version_name,MAX_STR_LEN-1,"%s",fw_version);
    }

    TS_LOG_INFO("%s() cust_ver = %s\n", __func__, tskit_mstar_data->fw_data.cust_ver);

    nLength = snprintf(nUserTempBuffer, sizeof(nUserTempBuffer), "%s\n", tskit_mstar_data->fw_data.cust_ver);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_sdcard_update_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                             loff_t * pPos)
{
    char *pValid = NULL;
    char *pTmpFilePath = NULL;
    char szFilePath[100] = { 0 };
    char *pStr = NULL;

    if(nCount > 1024){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }
    memset(apk_info->debug_buf, 0, 1024);

    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }
    TS_LOG_INFO("pBuffer = %s\n", apk_info->debug_buf);
    pStr = apk_info->debug_buf;
    if (pStr != NULL) {
        pValid = strstr(pStr, ".bin");

        if (pValid) {
            pTmpFilePath = strsep((char **)&pStr, ".");
            TS_LOG_INFO("pTmpFilePath = %s\n", pTmpFilePath);
            if (pTmpFilePath != NULL) {
                strcat(szFilePath, pTmpFilePath);
                strcat(szFilePath, ".bin");
            }
            TS_LOG_INFO("szFilePath = %s\n", szFilePath);

            if (0 != mstar_update_fw_sdcard(szFilePath)) {
                TS_LOG_INFO("Update FAILED\n");
            } else {
                TS_LOG_INFO("Update SUCCESS\n");
            }
        } else {
            TS_LOG_INFO("The file type of the update firmware bin file is not a .bin file.\n");
        }
    } else {
        TS_LOG_INFO("The file path of the update firmware bin file is NULL.\n");
    }
out:
    return nCount;
}

static ssize_t mstar_apknode_selinux_fw_update_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                             loff_t * pPos)
{
    u32 nLength = 0;
    s32 nRetVal = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG22XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        TS_LOG_INFO("FIRMWARE_FILE_PATH_ON_SD_CARD = %s\n", FIRMWARE_FILE_PATH_ON_SD_CARD);
        nRetVal = mstar_fw_update_sdcard(FIRMWARE_FILE_PATH_ON_SD_CARD, 1);
    } else {
        TS_LOG_INFO("This chip type (0x%x) does not support selinux limit firmware update\n", tskit_mstar_data->chip_type);
        nRetVal = -1;
    }

    if (0 != nRetVal) {
        apk_info->update_complete = 0;
        TS_LOG_INFO("Update FAILED\n");
    } else {
        apk_info->update_complete = 1;
        TS_LOG_INFO("Update SUCCESS\n");
    }

    nLength = sprintf(nUserTempBuffer, "%d", apk_info->update_complete);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    TS_LOG_INFO("g_update_complete = %d\n", apk_info->update_complete);

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_force_fw_update_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                           loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };
    TS_LOG_INFO("%s() enter\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    TS_LOG_INFO("FORCE_TO_UPDATE_FIRMWARE_ENABLED = %d\n", apk_info->force_update_firmware_enable);

    apk_info->force_update_firmware_enable = 1;    // Enable force firmware update
    apk_info->feature_support_status = apk_info->force_update_firmware_enable;

    nLength = sprintf(nUserTempBuffer, "%d", apk_info->feature_support_status);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    TS_LOG_INFO("g_feature_support_status = %d\n", apk_info->feature_support_status);

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_debug_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    u32 i, nLength = 0;
    u8 nBank, nAddr;
    u16 szRegData[MAX_DEBUG_REGISTER_NUM] = { 0 };
    u8 szOut[MAX_DEBUG_REGISTER_NUM * 25] = { 0 }, szValue[10] = { 0 };


    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    mstar_dbbus_enter();

    for (i = 0; i < apk_info->debug_reg_count; i++) {
        szRegData[i] = mstar_get_reg_16bit(apk_info->debug_reg[i]);
    }

    mstar_dbbus_exit();

    for (i = 0; i < apk_info->debug_reg_count; i++) {
        nBank = (apk_info->debug_reg[i] >> 8) & 0xFF;
        nAddr = apk_info->debug_reg[i] & 0xFF;

        TS_LOG_INFO("reg(0x%02X,0x%02X)=0x%04X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%02X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%02X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("pBuffer is NULL\n");
        return -ENOMEM;
    }

    nLength = strlen(szOut);
    if (copy_to_user(pBuffer, szOut, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_debug_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                         loff_t * pPos)
{
    u32 i;
    char *pCh = NULL;
    char *pStr = NULL;

    if(nCount > 1024){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }

    memset(apk_info->debug_buf, 0, 1024);
        if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
            TS_LOG_INFO("copy_from_user() failed\n");
            return -EFAULT;
        }
    if (apk_info->debug_buf != NULL) {
        TS_LOG_INFO("pBuffer[0] = %c\n", apk_info->debug_buf[0]);
        TS_LOG_INFO("pBuffer[1] = %c\n", apk_info->debug_buf[1]);
        TS_LOG_INFO("pBuffer[2] = %c\n", apk_info->debug_buf[2]);
        TS_LOG_INFO("pBuffer[3] = %c\n", apk_info->debug_buf[3]);
        TS_LOG_INFO("pBuffer[4] = %c\n", apk_info->debug_buf[4]);
        TS_LOG_INFO("pBuffer[5] = %c\n", apk_info->debug_buf[5]);

        TS_LOG_INFO("nCount = %d\n", (int)nCount);

        apk_info->debug_buf[nCount] = '\0';
        pStr = apk_info->debug_buf;

        i = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (i < MAX_DEBUG_REGISTER_NUM)) {
            TS_LOG_INFO("pCh = %s\n", pCh);

            apk_info->debug_reg[i] = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));

            TS_LOG_INFO("apk_info->debug_reg[%d] = 0x%04X\n", i, apk_info->debug_reg[i]);
            i++;
        }
        apk_info->debug_reg_count = i;

        TS_LOG_INFO("debug_reg_count = %d\n", apk_info->debug_reg_count);
    }

out:
    return nCount;
}

static ssize_t mstar_apknode_fw_set_debug_value_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                              loff_t * pPos)
{
    u32 i, nLength = 0;
    u8 nBank, nAddr;
    u16 szRegData[MAX_DEBUG_REGISTER_NUM] = { 0 };
    u8 szOut[MAX_DEBUG_REGISTER_NUM * 25] = { 0 }, szValue[10] = {
    0};

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    mstar_dbbus_enter();

    for (i = 0; i < apk_info->debug_reg_count; i++) {
        szRegData[i] = mstar_get_reg_16bit(apk_info->debug_reg[i]);
    }

    mstar_dbbus_exit();

    for (i = 0; i < apk_info->debug_reg_count; i++) {
        nBank = (apk_info->debug_reg[i] >> 8) & 0xFF;
        nAddr = apk_info->debug_reg[i] & 0xFF;

        TS_LOG_INFO("reg(0x%02X,0x%02X)=0x%04X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%02X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%02X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }

    nLength = strlen(szOut);
    if (copy_to_user(pBuffer, szOut, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_set_debug_value_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                               loff_t * pPos)
{
    u32 i, j, k;
    char *pCh = NULL;
    char *pStr = NULL;
      if(nCount > 1024){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    memset(apk_info->debug_buf, 0, 1024);

    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }
    if (apk_info->debug_buf != NULL) {
        TS_LOG_INFO("pBuffer[0] = %c\n", apk_info->debug_buf[0]);
        TS_LOG_INFO("pBuffer[1] = %c\n", apk_info->debug_buf[1]);

        TS_LOG_INFO("nCount = %d\n", (int)nCount);
        apk_info->debug_buf[nCount] = '\0';
        pStr = apk_info->debug_buf;

        i = 0;
        j = 0;
        k = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 2)) {
            TS_LOG_INFO("pCh = %s\n", pCh);

            if ((i % 2) == 0) {
                apk_info->debug_reg[j] = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));
                TS_LOG_INFO("apk_info->debug_reg[%d] = 0x%04X\n", j, apk_info->debug_reg[j]);
                j++;
            } else  // (i%2) == 1
            {
                apk_info->debug_reg_value[k] = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));
                TS_LOG_INFO("g_debug_reg_value[%d] = 0x%04X\n", k, apk_info->debug_reg_value[k]);
                k++;
            }

            i++;
        }
        apk_info->debug_reg_count = j;

        TS_LOG_INFO("debug_reg_count = %d\n", apk_info->debug_reg_count);

        mstar_dbbus_enter();

        for (i = 0; i < apk_info->debug_reg_count; i++) {
            mstar_set_reg_16bit(apk_info->debug_reg[i], apk_info->debug_reg_value[i]);
            TS_LOG_INFO("apk_info->debug_reg[%d] = 0x%04X, g_debug_reg_value[%d] = 0x%04X\n", i, apk_info->debug_reg[i], i, apk_info->debug_reg_value[i]);  // add for debug
        }

        mstar_dbbus_exit();
    }
out:
    return nCount;
}

static ssize_t mstar_apknode_fw_smbus_debug_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                          loff_t * pPos)
{
    u32 i = 0, nLength = 0, count = 5;
    u8 szSmBusRxData[MAX_I2C_TRANSACTION_LENGTH_LIMIT] = { 0 };
    u8 szOut[MAX_I2C_TRANSACTION_LENGTH_LIMIT * 2] = { 0 };
    u8 szValue[10] = { 0 };
    s32 rc = 0;

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    mutex_lock(&tskit_mstar_data->mutex_common);

    while (i < count) {
        if (apk_info->debug_cmd_arg_count > 0)  // Send write command
        {
            TS_LOG_INFO("Execute I2C SMBUS write command\n");

            msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &apk_info->debug_cmd_argu[0], apk_info->debug_cmd_arg_count);
            if (rc >= 0) {
                TS_LOG_INFO("mstar_iic_write_data(0x%X, 0x%X, %d) success\n", SLAVE_I2C_ID_DWI2C,
                        apk_info->debug_cmd_argu[0], apk_info->debug_cmd_arg_count);

                if (apk_info->debug_read_data_size == 0) {
                    break;  // No need to execute I2C SMBUS read command later. So, break here.
                }
            }
        }

        if (apk_info->debug_read_data_size > 0) // Send read command
        {
            TS_LOG_INFO("Execute I2C SMBUS read command\n");

            msleep(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &szSmBusRxData[0], apk_info->debug_read_data_size);
            if (rc >= 0) {
                TS_LOG_INFO("mstar_iic_read_data(0x%X, 0x%X, %d) success\n", SLAVE_I2C_ID_DWI2C,
                        szSmBusRxData[0], apk_info->debug_read_data_size);
                break;
            }
        }

        i++;
    }
    if (i == count) {
        TS_LOG_INFO("mstar_iic_write_data() & mstar_iic_read_data() failed, rc = %d\n", rc);
    }

    for (i = 0; i < apk_info->debug_read_data_size; i++)    // Output format 2.
    {
        TS_LOG_INFO("szSmBusRxData[%d] = 0x%x\n", i, szSmBusRxData[i]);

        sprintf(szValue, "%02x", szSmBusRxData[i]);
        strcat(szOut, szValue);

        if (i < (apk_info->debug_read_data_size - 1)) {
            strcat(szOut, ",");
        }
    }

    mutex_unlock(&tskit_mstar_data->mutex_common);
    nLength = strlen(szOut);
    if (copy_to_user(pBuffer, szOut, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_smbus_debug_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                           loff_t * pPos)
{
    u32 i, j;
    char szCmdType[5] = { 0 };
    char *pCh = NULL;
    char *pStr = NULL;

     if(nCount > 1024){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }
    memset(apk_info->debug_buf, 0, 1024);

    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }
    if (apk_info->debug_buf != NULL) {
        TS_LOG_INFO("pBuffer[0] = %c \n", apk_info->debug_buf[0]);
        TS_LOG_INFO("pBuffer[1] = %c\n", apk_info->debug_buf[1]);
        TS_LOG_INFO("pBuffer[2] = %c\n", apk_info->debug_buf[2]);
        TS_LOG_INFO("pBuffer[3] = %c\n", apk_info->debug_buf[3]);
        TS_LOG_INFO("pBuffer[4] = %c\n", apk_info->debug_buf[4]);
        TS_LOG_INFO("pBuffer[5] = %c\n", apk_info->debug_buf[5]);

        TS_LOG_INFO("nCount = %d\n", (int)nCount);

        // Reset to 0 before parsing the adb command
        apk_info->debug_cmd_arg_count = 0;
        apk_info->debug_read_data_size = 0;

        apk_info->debug_buf[nCount] = '\0';
        pStr = apk_info->debug_buf;

        i = 0;
        j = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (j < MAX_DEBUG_COMMAND_ARGUMENT_NUM)) {
            TS_LOG_INFO("pCh = %s\n", pCh);

            if (strcmp(pCh, "w") == 0 || strcmp(pCh, "r") == 0) {
                memcpy(szCmdType, pCh, strlen(pCh));
            } else if (strcmp(szCmdType, "w") == 0) {
                apk_info->debug_cmd_argu[j] = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));
                TS_LOG_INFO("g_debug_cmd_argu[%d] = 0x%02X\n", j, apk_info->debug_cmd_argu[j]);

                j++;

                apk_info->debug_cmd_arg_count = j;
                TS_LOG_INFO("g_debug_cmd_arg_count = %d\n", apk_info->debug_cmd_arg_count);
            } else if (strcmp(szCmdType, "r") == 0) {
                sscanf(pCh, "%d", &apk_info->debug_read_data_size);
                TS_LOG_INFO("apk_info->debug_read_data_size = %d\n", apk_info->debug_read_data_size);
            } else {
                TS_LOG_INFO("Un-supported adb command format!\n");
            }

            i++;
        }
    }
out:
    return nCount;
}

static ssize_t mstar_apknode_fw_set_dq_mem_value_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                               loff_t * pPos)
{
    u32 i, nLength = 0;
    u8 nBank, nAddr;
    u32 szRegData[MAX_DEBUG_REGISTER_NUM] = { 0 };
    u8 szOut[MAX_DEBUG_REGISTER_NUM * 25] = { 0 }, szValue[10] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    for (i = 0; i < apk_info->debug_reg_count; i++) {
        szRegData[i] = mstar_read_dq_mem_value(apk_info->debug_reg[i]);
    }

    for (i = 0; i < apk_info->debug_reg_count; i++) {
        nBank = (apk_info->debug_reg[i] >> 8) & 0xFF;
        nAddr = apk_info->debug_reg[i] & 0xFF;

        TS_LOG_INFO("reg(0x%02X,0x%02X)=0x%08X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%02X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%02X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }
    nLength = strlen(szOut);
    if (copy_to_user(pBuffer, szOut, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_set_dq_mem_value_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                            loff_t * pPos)
{
    u32 i, j, k;
    char *pCh = NULL;
    char *pStr = NULL;
    u16 nRealDQMemAddr = 0;
    u32 nRealDQMemValue = 0;

      if(nCount > 1024){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }
    memset(apk_info->debug_buf, 0, 1024);

    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }
    if (apk_info->debug_buf != NULL) {
        TS_LOG_INFO("pBuffer[0] = %c\n", apk_info->debug_buf[0]);
        TS_LOG_INFO("pBuffer[1] = %c\n", apk_info->debug_buf[1]);

        TS_LOG_INFO("nCount = %d\n", (int)nCount);
        apk_info->debug_buf[nCount] = '\0';
        pStr = apk_info->debug_buf;

        i = 0;
        j = 0;
        k = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 2)) {
            TS_LOG_INFO("pCh = %s\n", pCh);

            if ((i % 2) == 0) {
                apk_info->debug_reg[j] = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));
                TS_LOG_INFO("apk_info->debug_reg[%d] = 0x%04X\n", j, apk_info->debug_reg[j]);
                j++;
            } else  // (i%2) == 1
            {
                apk_info->debug_reg_value[k] = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));
                TS_LOG_INFO("g_debug_reg_value[%d] = 0x%04X\n", k, apk_info->debug_reg_value[k]);
                k++;
            }

            i++;
        }
        apk_info->debug_reg_count = j;

        TS_LOG_INFO("debug_reg_count = %d\n", apk_info->debug_reg_count);

        if ((apk_info->debug_reg[0] % 4) == 0) {
            nRealDQMemAddr = apk_info->debug_reg[0];
            nRealDQMemValue = mstar_read_dq_mem_value(nRealDQMemAddr);
            apk_info->debug_reg[0] = nRealDQMemAddr;
            TS_LOG_INFO("nRealDQMemValue Raw = %X\n", nRealDQMemValue);
            nRealDQMemValue &= 0xFFFF0000;
            nRealDQMemValue |= apk_info->debug_reg_value[0];
            TS_LOG_INFO("nRealDQMemValue Modify = %X\n", nRealDQMemValue);
            mstar_write_dq_mem_value(nRealDQMemAddr, nRealDQMemValue);
        } else if ((apk_info->debug_reg[0] % 4) == 2) {
            nRealDQMemAddr = apk_info->debug_reg[0] - 2;
            nRealDQMemValue = mstar_read_dq_mem_value(nRealDQMemAddr);
            apk_info->debug_reg[0] = nRealDQMemAddr;
            TS_LOG_INFO("nRealDQMemValue Raw = %X\n", nRealDQMemValue);

            nRealDQMemValue &= 0x0000FFFF;
            nRealDQMemValue |= (apk_info->debug_reg_value[0] << 16);
            TS_LOG_INFO("nRealDQMemValue Modify = %X\n", nRealDQMemValue);
            mstar_write_dq_mem_value(nRealDQMemAddr, nRealDQMemValue);
        }
    }
out:
    return nCount;
}

static ssize_t mstar_apknode_fw_mode_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    int ret = NO_ERR;
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        ret = mstar_mutual_get_fw_info(&tskit_mstar_data->mutual_fw_info);
        if(ret < 0){
            TS_LOG_ERR("%s:mutual_get_fw_info fail, ret=%d\n", __func__, ret);
        }
        mutex_lock(&tskit_mstar_data->mutex_protect);
        tskit_mstar_data->fw_mode = tskit_mstar_data->mutual_fw_info.nFirmwareMode;
        mutex_unlock(&tskit_mstar_data->mutex_protect);
        TS_LOG_INFO("%s() firmware mode = 0x%x\n", __func__, tskit_mstar_data->mutual_fw_info.nFirmwareMode);

        nLength = sprintf(nUserTempBuffer, "%x", tskit_mstar_data->mutual_fw_info.nFirmwareMode);
    }
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }
    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_mode_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                        loff_t * pPos)
{
    u32 nMode = 0;

    if(nCount > 16){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }
    memset(apk_info->debug_buf, 0, 16);

    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }

    if (apk_info->debug_buf != NULL) {
        sscanf(apk_info->debug_buf, "%x", &nMode);
        TS_LOG_INFO("firmware mode = 0x%x\n", nMode);

        apk_info->switch_mode = 0;

        if (nMode == MSG28XX_FIRMWARE_MODE_DEMO_MODE)   //demo mode
        {
            tskit_mstar_data->fw_mode = mstar_change_fw_mode(MSG28XX_FIRMWARE_MODE_DEMO_MODE);
        } else if (nMode == MSG28XX_FIRMWARE_MODE_DEBUG_MODE)   //debug mode
        {
            tskit_mstar_data->fw_mode = mstar_change_fw_mode(MSG28XX_FIRMWARE_MODE_DEBUG_MODE);
            apk_info->switch_mode = 1;
            apk_info->debug_log_time_stamp = 0; // Set g_debug_log_time_stamp for filter duplicate packet on MTPTool APK
        } else {
            TS_LOG_INFO("Undefined Firmware Mode\n");
        }
    }

    TS_LOG_INFO("tskit_mstar_data->fw_mode = 0x%x\n", tskit_mstar_data->fw_mode);
out:
    return nCount;
}

static ssize_t mstar_apknode_fw_sensor_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        if (tskit_mstar_data->mutual_fw_info.nLogModePacketHeader == 0xA7) {
            nLength =
                sprintf(nUserTempBuffer, "%d,%d,%d,%d", tskit_mstar_data->mutual_fw_info.nMx, tskit_mstar_data->mutual_fw_info.nMy,
                    tskit_mstar_data->mutual_fw_info.nSs, tskit_mstar_data->mutual_fw_info.nSd);
        } else {
            TS_LOG_INFO("Undefined debug mode packet format : 0x%x\n",
                    tskit_mstar_data->mutual_fw_info.nLogModePacketHeader);
            nLength = 0;
        }
    }
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }
    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_sensor_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                          loff_t * pPos)
{
	int ret = 0;
	char cmd[512] = { 0 };
    if(nCount > sizeof(cmd)){
        TS_LOG_ERR("Count(%d) is larger than cmd size(%d)\n",nCount,sizeof(cmd));
        goto out;
    }
	if (pBuffer != NULL) {
		ret = copy_from_user(cmd, pBuffer, nCount - 1);
		if (ret < 0) {
			TS_LOG_ERR("copy data from user space, failed\n");
			return -1;
		}
	}

	TS_LOG_INFO("nCount = %d, cmd = %s\n", (int)nCount, cmd);

	if (nCount > ARRAY_SIZE(cmd)) {
		TS_LOG_ERR("The length of command sent by user is too long\n");
		return -1;
	}

	if (strncmp(cmd, "en_gesture_debug", nCount) == 0) {
		TS_LOG_INFO("Enable Gesture debug by sending self data\n");
		tskit_mstar_data->ges_self_debug = true;
	} else if (strncmp(cmd, "dis_gesture_debug", nCount) == 0) {
		TS_LOG_INFO("Disable Gesture debug by sending self data\n");
		tskit_mstar_data->ges_self_debug = false;
	}
out:
    return nCount;
}

static ssize_t mstar_apknode_fw_packet_header_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                            loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        nLength = sprintf(nUserTempBuffer, "%d", tskit_mstar_data->mutual_fw_info.nLogModePacketHeader);

    }
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }
    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_fw_packet_header_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                             loff_t * pPos)
{
    return nCount;
}

static ssize_t _DrvKObjectPacketShow(struct kobject *pKObj, struct kobj_attribute *pAttr, char *pBuf)
{
    u32 nLength = 0;

    if (strcmp(pAttr->attr.name, "packet") == 0) {
        if (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEMO_MODE) {
            if (tskit_mstar_data->demo_packet != NULL) {
                TS_LOG_INFO("tskit_mstar_data->fw_mode=%x, g_demo_packet[0]=%x, g_demo_packet[1]=%x\n", tskit_mstar_data->fw_mode,
                        tskit_mstar_data->demo_packet[0], tskit_mstar_data->demo_packet[1]);
                TS_LOG_INFO("g_demo_packet[2]=%x, g_demo_packet[3]=%x\n", tskit_mstar_data->demo_packet[2],
                        tskit_mstar_data->demo_packet[3]);
                TS_LOG_INFO("g_demo_packet[4]=%x, g_demo_packet[5]=%x\n", tskit_mstar_data->demo_packet[4],
                        tskit_mstar_data->demo_packet[5]);

                memcpy(pBuf, tskit_mstar_data->demo_packet, MUTUAL_DEMO_MODE_PACKET_LENGTH);

                nLength = MUTUAL_DEMO_MODE_PACKET_LENGTH;
                TS_LOG_INFO("nLength = %d\n", nLength);
            } else {
                TS_LOG_INFO("g_demo_packet is NULL\n");
            }
        } else
        {
            if (tskit_mstar_data->log_packet != NULL) {
                TS_LOG_INFO("tskit_mstar_data->fw_mode=%x, g_log_packet[0]=%x, g_log_packet[1]=%x\n", tskit_mstar_data->fw_mode,
                        tskit_mstar_data->log_packet[0], tskit_mstar_data->log_packet[1]);
                TS_LOG_INFO("g_log_packet[2]=%x, g_log_packet[3]=%x\n", tskit_mstar_data->log_packet[2],
                        tskit_mstar_data->log_packet[3]);
                TS_LOG_INFO("g_log_packet[4]=%x, g_log_packet[5]=%x\n", tskit_mstar_data->log_packet[4],
                        tskit_mstar_data->log_packet[5]);

                if ((tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA)
                    && (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEBUG_MODE) && (tskit_mstar_data->log_packet[0] == 0xA7)) {
                    memcpy(pBuf, tskit_mstar_data->log_packet, tskit_mstar_data->mutual_fw_info.nLogModePacketLength);

                    if (apk_info->debug_log_time_stamp >= 255) {
                        apk_info->debug_log_time_stamp = 0;
                    } else {
                        apk_info->debug_log_time_stamp++;
                    }

                    pBuf[tskit_mstar_data->mutual_fw_info.nLogModePacketLength] = apk_info->debug_log_time_stamp;
                    TS_LOG_INFO("g_debug_log_time_stamp=%d\n", pBuf[tskit_mstar_data->mutual_fw_info.nLogModePacketLength]);    // TODO : add for debug

                    nLength = tskit_mstar_data->mutual_fw_info.nLogModePacketLength + 1;
                    TS_LOG_INFO("nLength = %d\n", nLength);
                } else {
                    TS_LOG_INFO("CURRENT MODE IS NOT DEBUG MODE/WRONG DEBUG MODE HEADER\n");
                }
            } else {
                TS_LOG_INFO("g_log_packet is NULL\n");
            }
        }
    } else {
        TS_LOG_INFO("pAttr->attr.name = %s \n", pAttr->attr.name);
    }

    return nLength;
}

static ssize_t mstar_apknode_kobject_packet_store(struct kobject *pKObj, struct kobj_attribute *pAttr,
                           const char *pBuf, size_t nCount)
{
    return nCount;
}

static struct kobj_attribute packet_attr =
__ATTR(packet, 0664, _DrvKObjectPacketShow, mstar_apknode_kobject_packet_store);

/* Create a group of attributes so that we can create and destroy them all at once. */
static struct attribute *attrs[] = {
    &packet_attr.attr,
    NULL,           /* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory. If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
static struct attribute_group attr_group = {
    .attrs = attrs,
};

//------------------------------------------------------------------------------//

static ssize_t mstar_apknode_query_feature_support_status_read(struct file *pFile, char __user * pBuffer,
                                size_t nCount, loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    nLength = sprintf(nUserTempBuffer, "%d", apk_info->feature_support_status);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    TS_LOG_INFO("g_feature_support_status = %d\n", apk_info->feature_support_status);

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_query_feature_support_status_write(struct file *pFile, const char __user * pBuffer,
                                 size_t nCount, loff_t * pPos)
{
    int ret = NO_ERR;
    u32 nFeature = 0;

    if(nCount > 1024){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }
    memset(apk_info->debug_buf, 0, 1024);
    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }
    if (apk_info->debug_buf != NULL) {
        sscanf(apk_info->debug_buf, "%x", &nFeature);
        TS_LOG_INFO("nFeature = 0x%x\n", nFeature);

        if (nFeature == FEATURE_GESTURE_WAKEUP_MODE) {
            apk_info->feature_support_status = apk_info->gesture_wakeup_enable;
        } else if (nFeature == FEATURE_GESTURE_DEBUG_MODE) {
            apk_info->feature_support_status = apk_info->gesture_debug_mode_enable;
        } else if (nFeature == FEATURE_GESTURE_INFORMATION_MODE) {
            apk_info->feature_support_status = apk_info->gesture_information_mode_enable;
        } else if (nFeature == FEATURE_TOUCH_DRIVER_DEBUG_LOG) {
            //apk_info->feature_support_status = TOUCH_DRIVER_DEBUG_LOG_LEVEL;
        } else if (nFeature == FEATURE_FIRMWARE_DATA_LOG) {
            apk_info->feature_support_status = apk_info->firmware_log_enable;

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
            if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
                if (apk_info->feature_support_status == 1)  // If the debug mode data log function is supported, then get packet address and flag address for segment read finger touch data.
                {
                    ret = mstar_get_touch_packet_addr(&tskit_mstar_data->fw_data.packet_data_addr, &tskit_mstar_data->fw_data.packet_flag_addr);
                    if (ret < 0) {
                        TS_LOG_ERR("%s get touch packet addr error ret = %d\n", __func__, ret);
                    }
                }
            }
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
        } else if (nFeature == FEATURE_FORCE_TO_UPDATE_FIRMWARE) {
            apk_info->feature_support_status = apk_info->force_update_firmware_enable;
        } else if (nFeature == FEATURE_DISABLE_ESD_PROTECTION_CHECK) {
            apk_info->feature_support_status = apk_info->disable_esd_protection_check;
        } else if (nFeature == FEATURE_APK_PRINT_FIRMWARE_SPECIFIC_LOG) {
            apk_info->feature_support_status = apk_info->firmware_special_log_enable;
        } else if (nFeature == FEATURE_SELF_FREQ_SCAN) {
            TS_LOG_INFO("change to  FEATURE_SELF_FREQ_SCAN\n");
            apk_info->feature_support_status = apk_info->self_freq_scan_enable;
        } else {
            TS_LOG_INFO("Undefined Feature\n");
        }
    }

    TS_LOG_INFO("g_feature_support_status = %d\n", apk_info->feature_support_status);
out:
    return nCount;
}

static ssize_t mstar_apknode_change_feature_support_status_read(struct file *pFile, char __user * pBuffer,
                                 size_t nCount, loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    nLength = sprintf(nUserTempBuffer, "%d", apk_info->feature_support_status);
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    TS_LOG_INFO("g_feature_support_status = %d\n", apk_info->feature_support_status);

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_change_feature_support_status_write(struct file *pFile, const char __user * pBuffer,
                                  size_t nCount, loff_t * pPos)
{
    u32 i;
    u32 nFeature = 0, nNewValue = 0;
    char *pCh = NULL;
    char *pStr = NULL;

    memset(apk_info->debug_buf, 0, 1024);

    if(nCount > 1024){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }

    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }
    if (apk_info->debug_buf != NULL) {
        TS_LOG_INFO("nCount = %d\n", (int)nCount);
        apk_info->debug_buf[nCount] = '\0';
        pStr = apk_info->debug_buf;

        i = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 3)) {
            TS_LOG_INFO("pCh = %s\n", pCh);

            if (i == 0) {
                nFeature = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));
                TS_LOG_INFO("nFeature = 0x%04X\n", nFeature);
            } else if (i == 1) {
                nNewValue = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));
                TS_LOG_INFO("nNewValue = %d\n", nNewValue);
            } else {
                TS_LOG_INFO("End of parsing adb command.\n");
            }

            i++;
        }
        if (nFeature == FEATURE_GESTURE_WAKEUP_MODE) {
            apk_info->gesture_wakeup_enable = nNewValue;
            apk_info->feature_support_status = apk_info->gesture_wakeup_enable;
        } else if (nFeature == FEATURE_GESTURE_DEBUG_MODE) {
            apk_info->gesture_debug_mode_enable = nNewValue;
            apk_info->feature_support_status = apk_info->gesture_debug_mode_enable;
        } else if (nFeature == FEATURE_GESTURE_INFORMATION_MODE) {
            apk_info->gesture_information_mode_enable = nNewValue;
            apk_info->feature_support_status = apk_info->gesture_information_mode_enable;
        } else if (nFeature == FEATURE_TOUCH_DRIVER_DEBUG_LOG) {
            //TOUCH_DRIVER_DEBUG_LOG_LEVEL = nNewValue;
            //apk_info->feature_support_status = TOUCH_DRIVER_DEBUG_LOG_LEVEL;
        } else if (nFeature == FEATURE_FIRMWARE_DATA_LOG) {
            apk_info->firmware_log_enable = nNewValue;
            apk_info->feature_support_status = apk_info->firmware_log_enable;
        } else if (nFeature == FEATURE_FORCE_TO_UPDATE_FIRMWARE) {
            apk_info->force_update_firmware_enable = nNewValue;
            apk_info->feature_support_status = apk_info->force_update_firmware_enable;
        } else if (nFeature == FEATURE_DISABLE_ESD_PROTECTION_CHECK) {
            apk_info->disable_esd_protection_check = nNewValue;
            apk_info->feature_support_status = apk_info->disable_esd_protection_check;
        } else if (nFeature == FEATURE_APK_PRINT_FIRMWARE_SPECIFIC_LOG) {
            apk_info->firmware_special_log_enable = nNewValue;
            apk_info->feature_support_status = apk_info->firmware_special_log_enable;
        } else if (nFeature == FEATURE_SELF_FREQ_SCAN) {
            apk_info->self_freq_scan_enable = nNewValue;
            apk_info->feature_support_status = apk_info->self_freq_scan_enable;
        } else {
            TS_LOG_INFO("Undefined Feature\n");
        }

        TS_LOG_INFO("g_feature_support_status = %d\n", apk_info->feature_support_status);
    }
   out:
    return nCount;
}

//------------------------------------------------------------------------------//
static ssize_t mstar_apknode_gesture_wakeup_mode_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                               loff_t * pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
    TS_LOG_INFO("g_gesture_wakeup_mode = 0x%x, 0x%x\n", 
    tskit_mstar_data->gesture_data.wakeup_mode[0], 
    tskit_mstar_data->gesture_data.wakeup_mode[1]);

    nLength = sprintf(nUserTempBuffer, "%x,%x", 
		tskit_mstar_data->gesture_data.wakeup_mode[0], 
		tskit_mstar_data->gesture_data.wakeup_mode[1]);
#else
    TS_LOG_INFO("g_gesture_wakeup_mode = 0x%x\n", 
    tskit_mstar_data->gesture_data.wakeup_mode[0]);

    nLength = sprintf(nUserTempBuffer, "%x", 
		tskit_mstar_data->gesture_data.wakeup_mode[0]);
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_gesture_info_mode_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                             loff_t * pPos)
{
    u8 szOut[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH * 5] = { 0 }, szValue[10] = {
    0};
    u32 szLogGestureInfo[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH] = { 0 };
    u32 i = 0;
    u32 nLength = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    u32 g_log_gesture_count = 0;
    if (apk_info->log_gesture_info_type == FIRMWARE_GESTURE_INFORMATION_MODE_A)    //FIRMWARE_GESTURE_INFORMATION_MODE_A
    {
        for (i = 0; i < 2; i++) //0 EventFlag; 1 RecordNum
        {
            szLogGestureInfo[g_log_gesture_count] = tskit_mstar_data->gesture_data.log_info[4 + i];
            g_log_gesture_count++;
        }

        for (i = 2; i < 8; i++) //2~3 Xst Yst; 4~5 Xend Yend; 6~7 char_width char_height
        {
            szLogGestureInfo[g_log_gesture_count] = tskit_mstar_data->gesture_data.log_info[4 + i];
            g_log_gesture_count++;
        }
    } else if (apk_info->log_gesture_info_type == FIRMWARE_GESTURE_INFORMATION_MODE_B) //FIRMWARE_GESTURE_INFORMATION_MODE_B
    {
        for (i = 0; i < 2; i++) //0 EventFlag; 1 RecordNum
        {
            szLogGestureInfo[g_log_gesture_count] = tskit_mstar_data->gesture_data.log_info[4 + i];
            g_log_gesture_count++;
        }

        for (i = 0; i < tskit_mstar_data->gesture_data.log_info[5] * 2; i++) //(X and Y)*RecordNum
        {
            szLogGestureInfo[g_log_gesture_count] = tskit_mstar_data->gesture_data.log_info[12 + i];
            g_log_gesture_count++;
        }
    } else if (apk_info->log_gesture_info_type == FIRMWARE_GESTURE_INFORMATION_MODE_C) //FIRMWARE_GESTURE_INFORMATION_MODE_C
    {
        for (i = 0; i < 6; i++) //header
        {
            szLogGestureInfo[g_log_gesture_count] = tskit_mstar_data->gesture_data.log_info[i];
            g_log_gesture_count++;
        }

        for (i = 6; i < 86; i++) {
            szLogGestureInfo[g_log_gesture_count] = tskit_mstar_data->gesture_data.log_info[i];
            g_log_gesture_count++;
        }

        szLogGestureInfo[g_log_gesture_count] = tskit_mstar_data->gesture_data.log_info[86]; //dummy
        g_log_gesture_count++;
        szLogGestureInfo[g_log_gesture_count] = tskit_mstar_data->gesture_data.log_info[87]; //checksum
        g_log_gesture_count++;
    } else {
        TS_LOG_INFO("Undefined GESTURE INFORMATION MODE\n");
    }

    for (i = 0; i < g_log_gesture_count; i++) {
        sprintf(szValue, "%d", szLogGestureInfo[i]);
        strcat(szOut, szValue);
        strcat(szOut, ",");
    }

    nLength = strlen(szOut);

    if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
        TS_LOG_INFO("copy to user error\n");
        return -EFAULT;
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_gesture_info_mode_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                              loff_t * pPos)
{
    u32 nMode = 0;

    if(nCount > 16){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }
    memset(apk_info->debug_buf, 0, 16);

    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }
    if (apk_info->debug_buf != NULL) {
        sscanf(apk_info->debug_buf, "%x", &nMode);
        apk_info->log_gesture_info_type = nMode;
    }

    TS_LOG_INFO("g_log_gesture_infor_type type = 0x%x\n", apk_info->log_gesture_info_type);
out:
    return nCount;
}

static ssize_t mstar_apknode_glove_mode_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    u32 nLength = 0;
    u8 nGloveMode = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        mstar_finger_touch_report_disable();

        mstar_apknode_get_glove_info(&nGloveMode);

        mstar_finger_touch_report_enable();

        TS_LOG_INFO("Glove Mode = 0x%x\n", nGloveMode);

        nLength = sprintf(nUserTempBuffer, "%x", nGloveMode);
        if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
            TS_LOG_INFO("copy to user error\n");
            return -EFAULT;
        }

    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_glove_mode_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                           loff_t * pPos)
{
    u32 nGloveMode = 0;
    u32 i = 0;
    char *pCh = NULL;
    char *pStr = NULL;

    if(nCount > 16){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }
    memset(apk_info->debug_buf, 0, 16);
    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }

    pStr = apk_info->debug_buf;
    if (apk_info->debug_buf != NULL) {
        i = 0;
        while ((pCh = strsep((char **)&pStr, ",")) && (i < 1)) {
            TS_LOG_INFO("pCh = %s\n", pCh);

            nGloveMode = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));

            i++;
        }

        TS_LOG_INFO("Glove Mode = 0x%x\n", nGloveMode);

        mstar_finger_touch_report_disable();

        if (nGloveMode == 0x01) //open glove mode
        {
            mstar_apknode_open_glove();
        } else if (nGloveMode == 0x00)  //close glove mode
        {
            mstar_apknode_close_glove();
        } else {
            TS_LOG_INFO("Undefined Glove Mode\n");
        }
        TS_LOG_INFO("apk_info.ans.g_IsEnableGloveMode = 0x%x\n", apk_info->ans.g_IsEnableGloveMode);

        mstar_finger_touch_report_enable();
    }
out:
    return nCount;
}

static ssize_t mstar_apknode_glove_open_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    u32 nLength = 0;

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        mstar_finger_touch_report_disable();

        mstar_apknode_open_glove();

        mstar_finger_touch_report_enable();
    }
    TS_LOG_INFO("apk_info.ans.g_IsEnableGloveMode = 0x%x\n", apk_info->ans.g_IsEnableGloveMode);

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_glove_close_read(struct file *pFile, char __user * pBuffer, size_t nCount, loff_t * pPos)
{
    u32 nLength = 0;

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        mstar_finger_touch_report_disable();

        mstar_apknode_close_glove();

        mstar_finger_touch_report_enable();
    }
    TS_LOG_INFO("g_IsEnableGloveMode = 0x%x\n", apk_info->ans.g_IsEnableGloveMode);

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_leather_sheath_mode_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                               loff_t * pPos)
{
    u32 nLength = 0;
    u8 nLeatherSheathMode = 0;
    u8 nUserTempBuffer[16] = { 0 };

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0) {
        return 0;
    }

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        mstar_finger_touch_report_disable();

        mstar_get_leather_sheath_info(&nLeatherSheathMode);

        mstar_finger_touch_report_enable();

        TS_LOG_INFO("Leather Sheath Mode = 0x%x\n", nLeatherSheathMode);

        nLength = sprintf(nUserTempBuffer, "%x", nLeatherSheathMode);
        if (copy_to_user(pBuffer, nUserTempBuffer, nLength)) {
            TS_LOG_INFO("copy to user error\n");
            return -EFAULT;
        }

    }

    *pPos += nLength;

    return nLength;
}

static ssize_t mstar_apknode_leather_sheath_mode_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                            loff_t * pPos)
{
    u32 nLeatherSheathMode = 0;
    u32 i = 0;
    char *pCh = NULL;
    char *pStr = NULL;

    if(nCount > 16){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }
    memset(apk_info->debug_buf, 0, 16);
    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");
        return -EFAULT;
    }
    pStr = apk_info->debug_buf;
    if (pStr != NULL) {
        i = 0;
        while ((pCh = strsep((char **)&pStr, ",")) && (i < 1)) {
            TS_LOG_INFO("pCh = %s\n", pCh);

            nLeatherSheathMode = mstar_convert_char_to_hex_digit(pCh, strlen(pCh));

            i++;
        }

        TS_LOG_INFO("Leather Sheath Mode = 0x%x\n", nLeatherSheathMode);

        mstar_finger_touch_report_disable();

        if (nLeatherSheathMode == 0x01) //open leather sheath mode
        {
            mstar_apknode_open_leather_sheath();
        } else if (nLeatherSheathMode == 0x00)  //close leather sheath mode
        {
            mstar_apknode_close_leath_sheath();
        } else {
            TS_LOG_INFO("Undefined Leather Sheath Mode\n");
        }

        TS_LOG_INFO("apk_info.ans.g_IsEnableLeatherSheathMode = 0x%x\n", apk_info->ans.g_IsEnableLeatherSheathMode);

        mstar_finger_touch_report_enable();
    }
out:
    return nCount;
}


static long mstar_apknode_jni_msgtool_ioctl(struct file *pFile, unsigned int nCmd, unsigned long nArg)
{
    long nRet = 0;

    switch (nCmd) {
    case MSGTOOL_IOCTL_RUN_CMD:
        {
            MsgToolDrvCmd_t *pTransCmd;
            pTransCmd = mstar_jni_trans_cmd_from_user(nArg);

            switch (pTransCmd->nCmdId) {
            case MSGTOOL_RESETHW:
                mstar_dev_hw_reset();
                break;
            case MSGTOOL_REGGETXBYTEVALUE:
                mstar_jni_reg_get_xbyte(pTransCmd);
                mstar_jni_trans_cmd_to_user(pTransCmd, nArg);
                break;
            case MSGTOOL_FINGERTOUCH:
                if (pTransCmd->nSndCmdLen == 1) {
                    TS_LOG_INFO("JNI enable touch\n");
                    mstar_finger_touch_report_enable();
                    tskit_mstar_data->finger_touch_disable = FALSE; // Resume finger touch ISR handling after MTPTool APK have sent i2c command to firmware.
                } else if (pTransCmd->nSndCmdLen == 0) {
                    TS_LOG_INFO("JNI disable touch\n");
                    mstar_finger_touch_report_disable();
                    tskit_mstar_data->finger_touch_disable = TRUE; // Skip finger touch ISR handling temporarily for MTPTool APK can send i2c command to firmware.
                }
                break;
            case MSGTOOL_DEVICEPOWEROFF:
                mstar_enter_sleep_mode();
                break;
            case MSGTOOL_GETSMDBBUS:
                TS_LOG_INFO("  MSGTOOL_GETSMDBBUS  \n");
                apk_info->rtn_cmd_data[0] = SLAVE_I2C_ID_DBBUS & 0xFF;
                apk_info->rtn_cmd_data[1] = SLAVE_I2C_ID_DWI2C & 0xFF;
                mstar_jni_trans_cmd_to_user(pTransCmd, nArg);
                break;
            case MSGTOOL_SETIICDATARATE:
                TS_LOG_INFO("  MSGTOOL_SETIICDATARATE do nothing  \n");
                break;
            case MSGTOOL_ERASE_FLASH:
                TS_LOG_INFO("  MSGTOOL_ERASE_FLASH  \n");
                if (pTransCmd->nSndCmdDataPtr == 0) {
                    TS_LOG_INFO("  erase Main block  \n");
                    mstar_erase_emem(EMEM_MAIN);
                } else if (pTransCmd->nSndCmdDataPtr == 1) {
                    TS_LOG_INFO("  erase INFO block  \n");
                    mstar_erase_emem(EMEM_INFO);
                }
                break;
            default:
                break;
            }
        }
        break;

    default:
        nRet = -EINVAL;
        break;
    }

    return nRet;
}

static ssize_t mstar_apknode_jni_msgtool_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                        loff_t * pPos)
{
    long nRet = 0;
    u8 nBusType = 0;
    u16 nWriteLen = 0;
    u8 szCmdData[20] = { 0 };
    u8 buffer[128] = { 0 };
    TS_LOG_INFO("  nCount = %d  \n", (int)nCount);
    // copy data from user space
    nBusType = nCount & 0xFF;
    if (nBusType == SLAVE_I2C_ID_DBBUS || nBusType == SLAVE_I2C_ID_DWI2C) {
        nWriteLen = (nCount >> 8) & 0xFFFF;
         if (nWriteLen > sizeof(szCmdData)) {
            TS_LOG_ERR("Count(%d) is larger than buffer size (%d)\n", nWriteLen, sizeof(szCmdData));
            goto out;
        }
        nRet = copy_from_user(szCmdData, &pBuffer[0], nWriteLen);
        mstar_iic_write_data(nBusType, &szCmdData[0], nWriteLen);
    } else {
        if (nCount > sizeof(buffer)) {
            TS_LOG_ERR("Count(%d) is larger than buffer size (%d)\n", nCount, sizeof(buffer));
            goto out;
        }
        nRet = copy_from_user(buffer, pBuffer, nCount - 1);
        if (nRet < 0) {
            printk("%s, copy data from user space, failed", __func__);
            return -1;
        }
        if (strcmp(buffer, "erase_flash") == 0) {
            TS_LOG_INFO("start Erase Flash\n");
            mstar_erase_emem(EMEM_MAIN);
            TS_LOG_INFO("end Erase Flash\n");
        }
        else if (strcmp(buffer, "reset") == 0) {
            TS_LOG_INFO("start reset\n");
            mstar_dev_hw_reset();
            TS_LOG_INFO("end reset\n");
        }
    }
out:
    return nCount;
}

static ssize_t mstar_apknode_set_film_mode_write(struct file *pFile, const char __user * pBuffer, size_t nCount,
                          loff_t * pPos)
{
    u8 nFilmType = 0;

   if(nCount > 1024){
        TS_LOG_ERR("%s:The size of count(%d) is larger than its define\n",__func__,nCount);
        goto out;
    }

    if (pBuffer == NULL) {
        TS_LOG_ERR("%s:pBuffer is NULL, do nothing\n",__func__);
        goto out;
    }
    memset(apk_info->debug_buf, 0, 1024);

    if (copy_from_user(apk_info->debug_buf, pBuffer, nCount)) {
        TS_LOG_INFO("copy_from_user() failed\n");

        return -EFAULT;
    }
    if (apk_info->debug_buf != NULL) {
        TS_LOG_INFO("nCount = %d\n", (int)nCount);
        apk_info->debug_buf[nCount] = '\0';
        nFilmType = mstar_convert_char_to_hex_digit(apk_info->debug_buf, strlen(apk_info->debug_buf));
        TS_LOG_INFO("nFeature = 0x%02X\n", nFilmType);
        mstar_set_film_mode(nFilmType);
    }
out:
    return nCount;
}

static ssize_t mstar_apknode_set_film_mode_read(struct file *pFile, char __user * pBuffer, size_t nCount,
                         loff_t * pPos)
{
    u8 nFilmType = 0;
    nFilmType = mstar_get_film_mode();
    TS_LOG_INFO("  %s()  , nFilmType = %d\n", __func__, nFilmType);
    if (copy_to_user(pBuffer, &nFilmType, 1)) {
        return -EFAULT;
    }
    return 0;
}

static ssize_t class_ts_info_show(struct class *class, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "mstar class_ts_info_show test");
}

static CLASS_ATTR(ts_info, S_IRUSR | S_IWUSR, class_ts_info_show, NULL);

static struct class *touchscreen_class;

static ssize_t gesture_show(struct class *class, struct class_attribute *attr, char *buf)
{
    if (apk_info->ans.g_GestureState)
        return sprintf(buf, "gesture: on\n");
    else
        return sprintf(buf, "gesture: off\n");
}

static ssize_t gesture_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
    if (!strncmp(buf, "on", 2))
        apk_info->ans.g_GestureState = true;
    else if (!strncmp(buf, "off", 3))
        apk_info->ans.g_GestureState = false;
    TS_LOG_DEBUG("buf = %s, g_GestureState = %d, count = %zu\n", buf, apk_info->ans.g_GestureState, count);
    return count;
}

static CLASS_ATTR(gesture, S_IRUSR | S_IWUSR, gesture_show, gesture_store);


static ssize_t mstar_proc_debug_switch_read(struct file *pFile, char __user *buff, size_t nCount, loff_t *pPos)
{
	int res = 0;
       int i = 0;
	unsigned char user_buf[512] = { 0 };
	if (*pPos != 0)
		return 0;

	mutex_lock(&apk_info->debug_mutex);
	apk_info->debug_node_open = !apk_info->debug_node_open;
       if (apk_info->debug_node_open)
       {
            apk_info->fw_debug_buf = kcalloc(1024, sizeof(unsigned char *), GFP_KERNEL);
            if (!ERR_ALLOC_MEM(apk_info->fw_debug_buf))
            {
                for (i = 0; i < 1024; i++)
                {
                    apk_info->fw_debug_buf[i] = kcalloc(1024, sizeof(unsigned char), GFP_KERNEL);
                    if (ERR_ALLOC_MEM(apk_info->fw_debug_buf[i]))
                    {
                        TS_LOG_ERR("%s, Failed to allocate fw_debug_buf two_dimen i = %d\n", __func__, i);
                        apk_info->debug_node_open = 0;
                    }
                }
            }
            else
            {
                TS_LOG_ERR("%s, Failed to allocate fw_debug_buf\n", __func__);
                apk_info->debug_node_open = 0;
            }
       }
       else
       {
            if (!ERR_ALLOC_MEM(apk_info->fw_debug_buf))
            {
                for (i = 0; i < 1024; i++)
                {
                    if (!ERR_ALLOC_MEM(apk_info->fw_debug_buf[i]))
                    {
                        kfree(apk_info->fw_debug_buf[i]);
                        apk_info->fw_debug_buf[i] = NULL;
                    }
                }
                kfree(apk_info->fw_debug_buf);
                apk_info->fw_debug_buf = NULL;
            }
       }
	TS_LOG_INFO(" %s debug_flag message = %x\n", apk_info->debug_node_open ? "Enabled" : "Disabled", apk_info->debug_node_open);

	nCount = sprintf(user_buf, "debug_node_open : %s\n", apk_info->debug_node_open ? "Enabled" : "Disabled");

	*pPos += nCount;

	res = copy_to_user(buff, user_buf, nCount);
	if (res < 0) {
		TS_LOG_ERR("Failed to copy data to user space");
	}
	mutex_unlock(&apk_info->debug_mutex);
	return nCount;
}

static ssize_t mstar_proc_debug_message_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	unsigned char buffer[512] = { 0 };

	/* check the buffer size whether it exceeds the local buffer size or not */
	if (size > 512) {
		TS_LOG_ERR("buffer exceed 512 bytes\n");
		size = 512;
	}

	ret = copy_from_user(buffer, buff, size - 1);
	if (ret < 0) {
		TS_LOG_ERR("copy data from user space, failed");
		return -1;
	}

	if (strcmp(buffer, "dbg_flag") == 0) {
		apk_info->debug_node_open = !apk_info->debug_node_open;
		TS_LOG_INFO(" %s debug_flag message(%X).\n", apk_info->debug_node_open ? "Enabled" : "Disabled",
			 apk_info->debug_node_open);
	}
	return size;
}

static ssize_t mstar_proc_debug_message_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	unsigned long p = *pPos;
	unsigned int count = size;
	int i = 0;
	int send_data_len = 0;
	size_t ret = 0;
	int data_count = 0;
	int one_data_bytes = 0;
	int need_read_data_len = 0;
	int type = 0;
	unsigned char *tmpbuf = NULL;
	unsigned char tmpbufback[128] = { 0 };

	mutex_lock(&apk_info->debug_read_mutex);

	while (apk_info->debug_data_frame <= 0) {
		if (filp->f_flags & O_NONBLOCK) {
	             mutex_unlock(&apk_info->debug_read_mutex);
			return -EAGAIN;
		}
		wait_event_interruptible(apk_info->inq, apk_info->debug_data_frame > 0);
	}

	mutex_lock(&apk_info->debug_mutex);

	tmpbuf = vmalloc(4096);	/* buf size if even */
	if (ERR_ALLOC_MEM(tmpbuf)) {
		TS_LOG_ERR("buffer vmalloc error\n");
		send_data_len += sprintf(tmpbufback + send_data_len, "buffer vmalloc error\n");
		ret = copy_to_user(buff, tmpbufback, send_data_len); /*apk_info->fw_debug_buf[0] */
	} else {
		if (apk_info->debug_data_frame > 0) {
			if (apk_info->fw_debug_buf[0][0] == 0x5A) {
				need_read_data_len = 43;
			} else if (apk_info->fw_debug_buf[0][0] == 0x7A) {
				type = apk_info->fw_debug_buf[0][3] & 0x0F;

				data_count = apk_info->fw_debug_buf[0][1] * apk_info->fw_debug_buf[0][2];

				if (type == 0 || type == 1 || type == 6) {
					one_data_bytes = 1;
				} else if (type == 2 || type == 3) {
					one_data_bytes = 2;
				} else if (type == 4 || type == 5) {
					one_data_bytes = 4;
				}
				need_read_data_len = data_count * one_data_bytes + 1 + 5;
			}

			send_data_len = 0;
			need_read_data_len = 2040;
			if (need_read_data_len <= 0) {
				TS_LOG_ERR("parse data err data len = %d\n", need_read_data_len);
				send_data_len +=
				    sprintf(tmpbuf + send_data_len, "parse data err data len = %d\n",
					    need_read_data_len);
			} else {
				for (i = 0; i < need_read_data_len; i++) {
					send_data_len += sprintf(tmpbuf + send_data_len, "%02X", apk_info->fw_debug_buf[0][i]);
					if (send_data_len >= 4096) {
						TS_LOG_ERR("send_data_len = %d set 4096 i = %d\n", send_data_len, i);
						send_data_len = 4096;
						break;
					}
				}
			}
			send_data_len += sprintf(tmpbuf + send_data_len, "\n\n");

			if (p == 5 || size == 4096 || size == 2048) {
				apk_info->debug_data_frame--;
				if (apk_info->debug_data_frame < 0) {
					apk_info->debug_data_frame = 0;
				}

				for (i = 1; i <= apk_info->debug_data_frame; i++) {
					memcpy(apk_info->fw_debug_buf[i - 1], apk_info->fw_debug_buf[i], 2048);
				}
			}
		} else {
			TS_LOG_ERR("no data send\n");
			send_data_len += sprintf(tmpbuf + send_data_len, "no data send\n");
		}

		/* Preparing to send data to user */
		if (size == 4096)
			ret = copy_to_user(buff, tmpbuf, send_data_len);
		else
			ret = copy_to_user(buff, tmpbuf + p, send_data_len - p);

		if (ret) {
			TS_LOG_ERR("copy_to_user err\n");
			ret = -EFAULT;
		} else {
			*pPos += count;
			ret = count;
			TS_LOG_DEBUG("Read %d bytes(s) from %ld\n", count, p);
		}
	}
	/* TS_LOG_ERR("send_data_len = %d\n", send_data_len); */
	if (send_data_len <= 0 || send_data_len > 4096) {
		TS_LOG_ERR("send_data_len = %d set 2048\n", send_data_len);
		send_data_len = 4096;
	}
	if (tmpbuf != NULL) {
		vfree(tmpbuf);
		tmpbuf = NULL;
	}

	mutex_unlock(&apk_info->debug_mutex);
	mutex_unlock(&apk_info->debug_read_mutex);
	return send_data_len;
}

static const struct file_operations g_fops_chip_type = {
    .read = mstar_apknode_chip_type_read,
    .write = mstar_apknode_chip_type_write,
};

static const struct file_operations g_fops_fw_data = {
    .read = mstar_apknode_fw_data_read,
    .write = mstar_apknode_fw_data_write,
};

static const struct file_operations g_fops_apk_fw_update = {
    .read = mstar_apknode_fw_update_read,
    .write = mstar_apknode_fw_update_write,
};

static const struct file_operations g_fops_customer_fw_ver = {
    .read = mstar_apknode_customer_fw_ver_read,
    .write = mstar_apknode_customer_fw_ver_write,
};

static const struct file_operations g_fops_platform_fw_ver = {
    .read = mstar_apknode_platform_fw_ver_read,
    .write = mstar_apknode_platform_fw_ver_write,
};

static const struct file_operations g_fops_drive_ver = {
    .read = mstar_apknode_drive_ver_read,
    .write = mstar_apknode_drive_ver_write,
};

static const struct file_operations g_fops_sdcard_fw_update = {
    .read = mstar_apknode_fw_sdcard_update_read,
    .write = mstar_apknode_fw_sdcard_update_write,
};

static const struct file_operations g_fops_fw_debug = {
    .read = mstar_apknode_fw_debug_read,
    .write = mstar_apknode_fw_debug_write,
};

static const struct file_operations g_fops_fw_set_debug_value = {
    .read = mstar_apknode_fw_set_debug_value_read,
    .write = mstar_apknode_fw_set_debug_value_write,
};

static const struct file_operations g_fops_fw_smbus_debug = {
    .read = mstar_apknode_fw_smbus_debug_read,
    .write = mstar_apknode_fw_smbus_debug_write,
};

static const struct file_operations g_fops_fw_set_dq_mem_value = {
    .read = mstar_apknode_fw_set_dq_mem_value_read,
    .write = mstar_apknode_fw_set_dq_mem_value_write,
};

static const struct file_operations g_fops_fw_mode = {
    .read = mstar_apknode_fw_mode_read,
    .write = mstar_apknode_fw_mode_write,
};

static const struct file_operations g_fops_fw_sensor = {
    .read = mstar_apknode_fw_sensor_read,
    .write = mstar_apknode_fw_sensor_write,
};

static const struct file_operations g_fops_fw_packet_header = {
    .read = mstar_apknode_fw_packet_header_read,
    .write = mstar_apknode_fw_packet_header_write,
};

static const struct file_operations g_fops_query_feature_support_status = {
    .read = mstar_apknode_query_feature_support_status_read,
    .write = mstar_apknode_query_feature_support_status_write,
};

static const struct file_operations g_fops_change_feature_support_status = {
    .read = mstar_apknode_change_feature_support_status_read,
    .write = mstar_apknode_change_feature_support_status_write,
};

static const struct file_operations g_fops_gesture_wakeup_mode = {
    .read = mstar_apknode_gesture_wakeup_mode_read,
};

static const struct file_operations g_fops_gesture_info_mode = {
    .read = mstar_apknode_gesture_info_mode_read,
    .write = mstar_apknode_gesture_info_mode_write,
};

static const struct file_operations g_fops_glove_mode = {
    .read = mstar_apknode_glove_mode_read,
    .write = mstar_apknode_glove_mode_write,
};

static const struct file_operations g_fops_open_glove_mode = {
    .read = mstar_apknode_glove_open_read,
};

static const struct file_operations g_fops_close_glove_mode = {
    .read = mstar_apknode_glove_close_read,
};

static const struct file_operations g_fops_leather_sheath_mode = {
    .read = mstar_apknode_leather_sheath_mode_read,
    .write = mstar_apknode_leather_sheath_mode_write,
};

static const struct file_operations g_fops_film_mode = {
    .read = mstar_apknode_set_film_mode_read,
    .write = mstar_apknode_set_film_mode_write,
};

#ifdef CONFIG_ENABLE_JNI_INTERFACE
static const struct file_operations g_fops_jni_method = {
    .read = mstar_jni_msg_tool_read,
    .write = mstar_apknode_jni_msgtool_write,
    .unlocked_ioctl = mstar_apknode_jni_msgtool_ioctl,
    .compat_ioctl = mstar_apknode_jni_msgtool_ioctl,
};
#endif
static const struct file_operations g_fops_selinux_fw_update = {
    .read = mstar_apknode_selinux_fw_update_read,
};

static const struct file_operations g_fops_force_fw_update = {
    .read = mstar_apknode_force_fw_update_read,
};


#ifdef MP_TEST_FUNCTION_2
static const struct file_operations g_fops_mp_test_customised = {
    .write = mstar_apknode_mp_test_customised_write,
    .read = mstar_apknode_mp_test_customised_read,
};
#endif

static struct file_operations proc_debug_message_fops = {
	.write = mstar_proc_debug_message_write,
	.read = mstar_proc_debug_message_read,
};

static struct file_operations proc_debug_message_switch_fops = {
	.read = mstar_proc_debug_switch_read,
};

struct procfs_table {
    char *node_name;
    struct proc_dir_entry *node;
    const struct file_operations *fops;
    bool is_created;
};

struct procfs_table p_table[] = {
#ifdef MP_TEST_FUNCTION_2
    {PROC_NODE_MP_TEST_CUSTOMISED, NULL, &g_fops_mp_test_customised, false},
#endif
    {PROC_NODE_CHIP_TYPE, NULL, &g_fops_chip_type, false},
    {PROC_NODE_FIRMWARE_DATA, NULL, &g_fops_fw_data, false},
    {PROC_NODE_FIRMWARE_UPDATE, NULL, &g_fops_apk_fw_update, false},
    {PROC_NODE_CUSTOMER_FIRMWARE_VERSION, NULL, &g_fops_customer_fw_ver, false},
    {PROC_NODE_PLATFORM_FIRMWARE_VERSION, NULL, &g_fops_platform_fw_ver, false},
    {PROC_NODE_DEVICE_DRIVER_VERSION, NULL, &g_fops_drive_ver, false},
    {PROC_NODE_SD_CARD_FIRMWARE_UPDATE, NULL, &g_fops_sdcard_fw_update, false},
    {PROC_NODE_FIRMWARE_DEBUG, NULL, &g_fops_fw_debug, false},
    {PROC_NODE_FIRMWARE_SET_DEBUG_VALUE, NULL, &g_fops_fw_set_debug_value, false},
    {PROC_NODE_FIRMWARE_SMBUS_DEBUG, NULL, &g_fops_fw_smbus_debug, false},
    {PROC_NODE_FIRMWARE_SET_DQMEM_VALUE, NULL, &g_fops_fw_set_dq_mem_value, false},
    {PROC_NODE_FIRMWARE_MODE, NULL, &g_fops_fw_mode, false},
    {PROC_NODE_FIRMWARE_SENSOR, NULL, &g_fops_fw_sensor, false},
    {PROC_NODE_FIRMWARE_PACKET_HEADER, NULL, &g_fops_fw_packet_header, false},
    {PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS, NULL, &g_fops_query_feature_support_status, false},
    {PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS, NULL, &g_fops_change_feature_support_status, false},
    {PROC_NODE_GESTURE_WAKEUP_MODE, NULL, &g_fops_gesture_wakeup_mode, false},
    {PROC_NODE_GESTURE_INFORMATION_MODE, NULL, &g_fops_gesture_info_mode, false},
    {PROC_NODE_GLOVE_MODE, NULL, &g_fops_glove_mode, false},
    {PROC_NODE_OPEN_GLOVE_MODE, NULL, &g_fops_open_glove_mode, false},
    {PROC_NODE_CLOSE_GLOVE_MODE, NULL, &g_fops_close_glove_mode, false},
    {PROC_NODE_LEATHER_SHEATH_MODE, NULL, &g_fops_leather_sheath_mode, false},
    {PROC_NODE_CONTROL_FILM_MODE, NULL, &g_fops_film_mode, false},
#ifdef CONFIG_ENABLE_JNI_INTERFACE
    {PROC_NODE_JNI_NODE, NULL, &g_fops_jni_method, false},
#endif
    {PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE, NULL, &g_fops_selinux_fw_update, false},
    {PROC_NODE_FORCE_FIRMWARE_UPDATE, NULL, &g_fops_force_fw_update, false},
    //{PROC_NODE_TRIM_CODE, NULL, &_gProcTrimCode, false},
#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
    {PROC_NODE_PROXIMITY_MODE, NULL, &g_fops_proximity, false},
#endif
	{PROC_NODE_DEBUG_MESSAGE, NULL, &proc_debug_message_fops, false},
	{PROC_NODE_DEBUG_MESSAGE_SWITCH, NULL, &proc_debug_message_switch_fops, false},

};

int mstar_apknode_create_procfs(void)
{
    int i = 0;
    s32 ret = 0;

    apk_info->ans.g_IsEnableLeatherSheathMode = 0;
    apk_info->ans.g_IsEnableGloveMode = 0;
    apk_info->ans.g_GestureState = false;

    g_proc_class_entry = proc_mkdir(PROC_NODE_CLASS, NULL);

    g_proc_ts_msg20xx_entry = proc_mkdir(PROC_NODE_MS_TOUCHSCREEN_MSG20XX, g_proc_class_entry);

    g_proc_device_entry = proc_mkdir(PROC_NODE_DEVICE, g_proc_ts_msg20xx_entry);

    /* A main loop to create apk nodes */
    for (i = 0; i < ARRAY_SIZE(p_table); i++) {
        p_table[i].node =
            proc_create(p_table[i].node_name, PROCFS_AUTHORITY, g_proc_device_entry, p_table[i].fops);

        if (p_table[i].node == NULL) {
            TS_LOG_ERR("Failed to allocate %s mem\n", p_table[i].node_name);
        }

        p_table[i].is_created = true;
        TS_LOG_DEBUG("Created -> %s node successfully\n", p_table[i].node_name);
    }

    /* create a kset with the name of "kset_example" which is located under /sys/kernel/ */
    apk_info->touch_kset = kset_create_and_add("kset_example", NULL, kernel_kobj);
    if (!apk_info->touch_kset) {
        TS_LOG_INFO("  kset_create_and_add() failed, ret = %d  \n", ret);
        return -ENOMEM;
    }

    apk_info->touch_kobj = kobject_create();
    if (!apk_info->touch_kobj) {
        TS_LOG_INFO("  kobject_create() failed, ret = %d  \n", ret);

        kset_unregister(apk_info->touch_kset);
        apk_info->touch_kset = NULL;
        return -ENOMEM;
    }

    apk_info->touch_kobj->kset = apk_info->touch_kset;

    ret = kobject_add(apk_info->touch_kobj, NULL, "%s", "kobject_example");
    if (ret != 0) {
        TS_LOG_INFO("  kobject_add() failed, ret = %d  \n", ret);

        kobject_put(apk_info->touch_kobj);
        apk_info->touch_kobj = NULL;
        kset_unregister(apk_info->touch_kset);
        apk_info->touch_kset = NULL;
        return -ENOMEM;
    }

    /* create the files associated with this kobject */
    ret = sysfs_create_group(apk_info->touch_kobj, &attr_group);
    if (ret != 0) {
        TS_LOG_INFO("  sysfs_create_file() failed, ret = %d  \n", ret);

        kobject_put(apk_info->touch_kobj);
        apk_info->touch_kobj = NULL;
        kset_unregister(apk_info->touch_kset);
        apk_info->touch_kset = NULL;
        return -ENOMEM;
    }

    touchscreen_class = class_create(THIS_MODULE, "touchscreen");
    if (IS_ERR_OR_NULL(touchscreen_class)) {
        TS_LOG_INFO("%s: create class error!\n", __func__);
        return -ENOMEM;
    }

    ret = class_create_file(touchscreen_class, &class_attr_ts_info);
    if (ret < 0) {
        TS_LOG_INFO("%s class_create_file failed!\n", __func__);
        class_destroy(touchscreen_class);
        return -ENOMEM;
    }

    ret = class_create_file(touchscreen_class, &class_attr_gesture);
    if (ret < 0) {
        TS_LOG_INFO("%s create gesture file failed!\n", __func__);
        class_destroy(touchscreen_class);
        return -ENOMEM;
    }
    return ret;
}

void mstar_apknode_remove_procfs(void)
{
    int i;

    /* A main loop to destroy nodes */
    for (i = 0; i < ARRAY_SIZE(p_table); i++) {
        if (p_table[i].is_created) {
            remove_proc_entry(p_table[i].node_name, g_proc_device_entry);
        }
    }

    /* Destroy device dir */
    if (g_proc_device_entry != NULL) {
        remove_proc_entry(PROC_NODE_DEVICE, g_proc_ts_msg20xx_entry);
        g_proc_device_entry = NULL;
        TS_LOG_INFO("Remove procfs file node(%s) OK!\n", PROC_NODE_DEVICE);
    }

    /* Destroy ms-touchscreen-msg20xx dir */
    if (g_proc_ts_msg20xx_entry != NULL) {
        remove_proc_entry(PROC_NODE_MS_TOUCHSCREEN_MSG20XX, g_proc_class_entry);
        g_proc_ts_msg20xx_entry = NULL;
        TS_LOG_INFO("Remove procfs file node(%s) OK!\n", PROC_NODE_MS_TOUCHSCREEN_MSG20XX);
    }

    /* Destroy class dir */
    if (g_proc_class_entry != NULL) {
        remove_proc_entry(PROC_NODE_CLASS, NULL);
        g_proc_class_entry = NULL;
        TS_LOG_INFO("Remove procfs file node(%s) OK!\n", PROC_NODE_CLASS);
    }
}

void mstar_apknode_remove_gesturenode(void)
{
    if (apk_info->gesture_kset) {
        kset_unregister(apk_info->gesture_kset);
        apk_info->gesture_kset = NULL;
    }

    if (apk_info->gesture_kobj) {
        kobject_put(apk_info->gesture_kobj);
        apk_info->gesture_kobj = NULL;
    }
}

void mstar_gesture_notify(u8 *packet)
{
    char *pEnvp[2];
    s32 nRetVal = 0;
    if (apk_info->gesture_kobj != NULL && (*packet) == PACKET_TYPE_GESTURE_DEBUG) {
        pEnvp[0] = "STATUS=GET_GESTURE_DEBUG";
        pEnvp[1] = NULL;

        nRetVal = kobject_uevent_env(apk_info->gesture_kobj, KOBJ_CHANGE, pEnvp);
        TS_LOG_DEBUG("kobject_uevent_env() STATUS=GET_GESTURE_DEBUG, nRetVal = %d\n", nRetVal);
    }
}

void mstar_apknode_function_control(void)
{
    if (apk_info->ans.g_IsEnableGloveMode == 1) {
        mstar_apknode_open_glove();
    }

    if (apk_info->ans.g_IsEnableLeatherSheathMode == 1) {
        mstar_apknode_open_leather_sheath();
    }
}

struct apk_data_info * mstar_malloc_apknode(void)
{
    apk_info = kzalloc(sizeof(struct apk_data_info), GFP_KERNEL);
    if (NULL == apk_info) {
        TS_LOG_ERR("%s:alloc mem for apk_info fail\n", __func__);
        return NULL;
    }
	return apk_info;
}

int mstar_apknode_para_init(void)
{
	if(apk_info == NULL){
		TS_LOG_ERR("%s:apk_info is NULL init fail\n", __func__);
		return  -ENOMEM;
	}  
	apk_info->firmware_log_enable = CONFIG_ENABLE_FIRMWARE_DATA_LOG;
	apk_info->firmware_special_log_enable = CONFIG_ENABLE_APK_PRINT_FIRMWARE_SPECIFIC_LOG;
	apk_info->self_freq_scan_enable = 0;
	apk_info->force_update_firmware_enable = 0;
	apk_info->disable_esd_protection_check = 0;
	apk_info->switch_mode = 0;
	apk_info->log_gesture_info_type = 0;
	init_waitqueue_head(&(apk_info->inq));
	mutex_init(&apk_info->debug_mutex);
	mutex_init(&apk_info->debug_read_mutex);
	return NO_ERR;
}
