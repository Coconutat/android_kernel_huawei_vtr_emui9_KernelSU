#ifndef __ILITEK_UPGRADE_H__
#define __ILITEK_UPGRADE_H__

struct ilitek_upgrade {
    struct ilitek_version fw_ver;
};
int ilitek_fw_update_boot(char *file_name);
int ilitek_fw_update_sd(void);
int ilitek_upgrade_init(void);
void ilitek_upgrade_exit(void);
#endif