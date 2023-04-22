#ifndef HW_BOOT_FAIL_CORE_H
#define HW_BOOT_FAIL_CORE_H

#include <linux/types.h>


/*******************************************************************************
Function:       save_hwbootfailInfo_to_file
Description:    save dfx and bfi partition's log to file
Input:          NA
Output:         NA
Return:         NA
********************************************************************************/
void save_hwbootfailInfo_to_file(void);

#ifdef CONFIG_HUAWEI_BFM
bool bfmr_is_enabled(void);
#else
static inline bool bfmr_is_enabled(void)
{
    return false;
}
#endif

#endif

