/*Emcom_utils.h
 *
 * Bastet Head File.
 *
 * Copyright (C) 2014 Huawei Device Co.,Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _EMCOM_UTILS_H
#define _EMCOM_UTILS_H
#include <huawei_platform/log/hw_log.h>


#undef HWLOG_TAG
#define HWLOG_TAG EMCOMK

HWLOG_REGIST();

#define EMCOM_DEBUG    1
#define EMCOM_INFO     1

/* network type */
typedef enum
{
	NETWORK_TYPE_UNKNOWN = 0,
	NETWORK_TYPE_2G,
	NETWORK_TYPE_3G,
	NETWORK_TYPE_4G,
	NETWORK_TYPE_WIFI,
} NETWORK_TYPE;

typedef enum
{
    MODEM_NOT_SUPPORT_EMCOM,
    MODEM_SUPPORT_EMCOM
}Emcom_Support_Enum;

#define EMCOM_LOGD(fmt, ...) \
    do { \
        if (EMCOM_DEBUG) { \
            hwlog_info("%s"fmt"\n", __func__, ##__VA_ARGS__); \
        } \
    } while (0)

#define EMCOM_LOGI(fmt, ...) \
    do { \
        if (EMCOM_INFO) { \
            hwlog_info("%s"fmt"\n", __func__, ##__VA_ARGS__); \
        } \
    } while (0)

#define EMCOM_LOGE(fmt, ...) \
    do { \
            hwlog_err("%s"fmt"\n", __func__, ##__VA_ARGS__); \
    } while (0)

bool Emcom_Is_Modem_Support( void );


#endif  /* _EMCOM_UTILS_H */
