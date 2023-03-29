#include "ilitek_common.h"
#include "ilitek_dts.h"
#include "ilitek_protocol.h"
#include "ilitek_config.h"
#include "ilitek_firmware.h"
#include "ilitek_flash.h"
#include "ilitek_upgrade.h"

#define ILITEK_UPGRADE_FW_SD_NAME              "ts/touch_screen_firmware.hex"
#define ILITEK_FW_VERSION_FLASH_ADDR           0xFFE0

static struct ilitek_upgrade *g_ilitek_upg = NULL;

static int ilitek_upgrade_get_fw_name(void)
{
    struct ts_kit_device_data *p_dev_data = g_ilitek_ts->ts_dev_data;

    if (!strnlen(p_dev_data->ts_platform_data->product_name, MAX_STR_LEN - 1)
        || !strnlen(p_dev_data->chip_name, MAX_STR_LEN - 1)
        || !strnlen(g_ilitek_ts->project_id, ILITEK_PROJECT_ID_LEN)
        || !strnlen(p_dev_data->module_name, MAX_STR_LEN - 1)) {
        ilitek_err("fw file name is not detected");
        return -EINVAL;
    }

    snprintf(g_ilitek_ts->fw_name, ILITEK_FILE_NAME_LEN, "ts/%s_%s_%s_%s.hex",
        p_dev_data->ts_platform_data->product_name,
        p_dev_data->chip_name,
        g_ilitek_ts->project_id,
        p_dev_data->module_name);

    ilitek_info("firmware file name = %s", g_ilitek_ts->fw_name);

    return 0;
}

static int ilitek_upgrade_check_fw_verison(u8 *fw_data, bool enforce)
{
    int ret = 0;
    int i = 0;
    int major =0, mid =0, minor = 0;
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;

    if (enforce) {
        ilitek_info("enforce to update firmware");
        return 0;
    }

    if (core_firmware->end_addr <= ILITEK_FW_VERSION_FLASH_ADDR) {
        ilitek_err("can't find firmware version in flash addr, end_addr=0x%x, addr=0x%x",
            core_firmware->end_addr, ILITEK_FW_VERSION_FLASH_ADDR);
        return -EINVAL;
    }

    /* mid of version >= 3, fw verison[3] use for debug */
    major = fw_data[ILITEK_FW_VERSION_FLASH_ADDR];
    mid = fw_data[ILITEK_FW_VERSION_FLASH_ADDR + 1];
    minor = fw_data[ILITEK_FW_VERSION_FLASH_ADDR + 2];

    ilitek_debug(DEBUG_FIRMWARE, "firmware version in flash addr = 0xx",
        ILITEK_FW_VERSION_FLASH_ADDR);

    ilitek_info("firmware verison in file is %d.%d.%d", major, mid, minor);
    ilitek_info("firmware verison in ic is %s", p_cfg->fw_ver.str);

    if (major != p_cfg->fw_ver.data[0] ||
        mid != p_cfg->fw_ver.data[1] ||
        minor != p_cfg->fw_ver.data[2]){
        ilitek_info("find new firmware");
        return 0;
    }

    /* same version, crc maybe different */
    ret = tddi_check_fw_upgrade();
    if (ret == NEED_UPDATE) {
        ilitek_info("FW CRC is different, doing upgrade\n");
        return 0;
    } else if (ret == NO_NEED_UPDATE) {
        ilitek_info("FW CRC is the same, doing nothing\n");
        return -EINVAL;
    } else {
        ilitek_info("FW check is incorrect, unexpected errors\n");
        return -EINVAL;
    }

    return -EINVAL;
}

static int ilitek_upgrade_fw(char *file_name, bool enforce)
{
    int ret = 0;
    struct device *p_dev = NULL;
    struct firmware *p_fw = NULL;

    p_dev = &g_ilitek_ts->pdev->dev;

    core_firmware->isUpgrading = true;
    core_firmware->update_status = 0;

    if (IS_ERR_OR_NULL(flashtab)) {
        ilitek_err("flash table didn't create");
        ret = -ENOMEM;
        goto err_out;
    }

    /* 2. request firmware */
    ret = request_firmware(&p_fw, file_name, p_dev);
    if (ret) {
        ilitek_err("request firmware failed, ret = %d, fw_name = %s",
            ret, file_name);
        goto err_out;
    }

    ilitek_info("firmware size = %d", p_fw->size);

    flash_fw = kzalloc(flashtab->mem_size, GFP_KERNEL);
    if (IS_ERR_OR_NULL(flash_fw)) {
        ilitek_err("alloc flash_fw failed, %ld", PTR_ERR(flash_fw));
        ret = -ENOMEM;
        goto err_release_fw;
    }

    memset(flash_fw, 0xff, sizeof(u8) * flashtab->mem_size);

    g_total_sector = flashtab->mem_size / flashtab->sector;
    if (g_total_sector <= 0) {
        ilitek_err("flash configure is wrong");
        ret = -EINVAL;
        goto err_free_flash_fw;
    }

    g_flash_sector = kzalloc(g_total_sector * sizeof(*g_flash_sector), GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_flash_sector)) {
        ilitek_err("alloc g_flash_sector failed, %ld", PTR_ERR(g_flash_sector));
        ret = -ENOMEM;
        goto err_free_flash_sector;
    }

    ret = convert_hex_file(p_fw->data, p_fw->size, false);
    if (ret) {
        ilitek_err("covert firmware data failed, ret = %d", ret);
        goto err_free_flash_sector;
    }

    /* 3. compare firmware version */
    ret = ilitek_upgrade_check_fw_verison(flash_fw, enforce);
    if (ret) {
        ilitek_info("no need to update firmware");
        ret = 0;
        goto err_free_flash_sector;
    } else {
        ilitek_info("going to update firmware");
    }

    /* 4. firmware update */
    /* calling that function defined at init depends on chips. */
    ret = core_firmware->upgrade_func(false);
    if (ret) {
        ilitek_err("upgrade firmware failed, ret = %d", ret);
        goto err_free_flash_sector;
    }

    /* 5. reload ic info */
    ilitek_info("update ic information");
    ilitek_config_read_ic_info(ILITEK_FW_INFO);
    ilitek_config_read_ic_info(ILITEK_PRO_INFO);
    ilitek_config_read_ic_info(ILITEK_CORE_INFO);
    ilitek_config_read_ic_info(ILITEK_TP_INFO);
    ilitek_config_read_ic_info(ILITEK_KEY_INFO);

    ilitek_info("upgrade firmware succeeded");

err_free_flash_sector:
    if (g_flash_sector) {
        kfree(g_flash_sector);
        g_flash_sector = NULL;
    }
err_free_flash_fw:
    if (flash_fw) {
        kfree(flash_fw);
        flash_fw = NULL;
    }
err_release_fw:
    if (p_fw) {
        release_firmware(p_fw);
        p_fw = NULL;
    }
err_out:
    core_firmware->isUpgrading = false;
    return ret;
}

int ilitek_fw_update_boot(char *file_name)
{
    int ret = 0;

    /* 1. get firmware name */
    ret = ilitek_upgrade_get_fw_name();
    if (ret) {
        ilitek_err("get firmware name failed");
        return ret;
    }

    ret = ilitek_upgrade_fw(g_ilitek_ts->fw_name, false);
    if (ret) {
        ilitek_err("boot update firmware failed");
        return ret;
    }

    return 0;
}

int ilitek_fw_update_sd(void)
{
    int ret = 0;

    ret = ilitek_upgrade_fw(ILITEK_UPGRADE_FW_SD_NAME, true);
    if (ret) {
        ilitek_err("sd update firmware failed");
        return ret;
    }

    return 0;
}

int ilitek_upgrade_init(void)
{
    int ret = 0;

    ret = core_firmware_init();
    if (ret) {
        ilitek_err("core firmware init failed");
        return ret;
    }

    g_ilitek_upg = kzalloc(sizeof(*g_ilitek_upg), GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_upg)) {
        ilitek_err("alloc g_ilitek_upg failed");
        return -ENOMEM;
    }

    g_ilitek_ts->upg = g_ilitek_upg;

    ilitek_info("upgrade init succeeded");

    return 0;
}

void ilitek_upgrade_exit(void)
{
    core_firmware_remove();

    if (g_ilitek_upg) {
        kfree(g_ilitek_upg);
        g_ilitek_upg = NULL;
    }

    g_ilitek_ts->upg = NULL;

    ilitek_info("upgrade exit succeeded");
}
