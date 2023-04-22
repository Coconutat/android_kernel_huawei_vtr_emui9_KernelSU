#ifndef __FSC_VERSION_H__
#define __FSC_VERSION_H__

#include "platform.h"

/* Program Revision constant.  When updating firmware, change this.  */
#define FSC_TYPEC_CORE_FW_REV_UPPER  0
#define FSC_TYPEC_CORE_FW_REV_MIDDLE 0
#define FSC_TYPEC_CORE_FW_REV_LOWER  5

#ifdef FSC_DEBUG
FSC_U8 FUSB3601_core_get_rev_lower(void);
FSC_U8 FUSB3601_core_get_rev_middle(void);
FSC_U8 FUSB3601_core_get_rev_upper(void);
#endif // FSC_DEBUG

#endif // __FSC_VERSION_H_

