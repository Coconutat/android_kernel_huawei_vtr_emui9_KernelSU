/*
 * hi64xx_hifi_om.h -- om module for hi64xx
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __HI64XX_HIFI_ANC_BETA_H__
#define __HI64XX_HIFI_ANC_BETA_H__

extern void anc_beta_generate_path(hook_pos pos, char *base_path, char *full_path, unsigned long full_path_len);
extern void anc_beta_set_voice_hook_switch(unsigned short permission);
extern int anc_beta_start_hook(void);
extern int anc_beta_stop_hook(void);
extern int anc_beta_log_upload(void* data);
extern int dsm_beta_dump_file(void* data, bool create_dir);
extern int dsm_beta_log_upload(void* data);

#endif
