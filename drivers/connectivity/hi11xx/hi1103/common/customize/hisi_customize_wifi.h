

#ifndef __HISI_CUSTOMIZE_WIFI_H__
#define __HISI_CUSTOMIZE_WIFI_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
#include "hisi_customize_wifi_hi110x.h"
#elif (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#include "hisi_customize_wifi_hi115x.h"
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif //hisi_customize_wifi.h

