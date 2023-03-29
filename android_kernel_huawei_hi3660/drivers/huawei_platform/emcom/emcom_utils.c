#include <linux/module.h>
#include <linux/types.h>
#include "emcom_utils.h"




#undef HWLOG_TAG
#define HWLOG_TAG emcom_utils
HWLOG_REGIST();
MODULE_LICENSE("GPL");

Emcom_Support_Enum g_Modem_emcom_support = MODEM_NOT_SUPPORT_EMCOM;
/******************************************************************************
   6 º¯ÊýÊµÏÖ
******************************************************************************/



void Emcom_Ind_Modem_Support(uint8_t enSupport)
{
    EMCOM_LOGD("Emcom_Ind_Modem_Support:%d\n",g_Modem_emcom_support);
    g_Modem_emcom_support = ( Emcom_Support_Enum )enSupport;
}


bool Emcom_Is_Modem_Support( void )
{
    if( MODEM_NOT_SUPPORT_EMCOM == g_Modem_emcom_support )
    {
        return false;
    }
    else
    {
        return true;
    }
}
