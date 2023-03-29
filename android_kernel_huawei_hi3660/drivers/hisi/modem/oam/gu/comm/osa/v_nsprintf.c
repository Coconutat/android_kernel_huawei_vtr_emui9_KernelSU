/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*****************************************************************************/
/*                                                                           */
/*                Copyright 1999 - 2003, Huawei Tech. Co., Ltd.              */
/*                           ALL RIGHTS RESERVED                             */
/*                                                                           */
/* FileNmae: v_nsprintf.c                                                    */
/*                                                                           */
/* Author:Tong ChaoZhu                                                       */
/*                                                                           */
/* Version: 1.0                                                              */
/*                                                                           */
/* Date:  2001-12-26                                                         */
/*                                                                           */
/* Description: copy this file from Dopra                                    */
/*                                                                           */
/*                                                                           */
/* Others:                                                                   */
/*                                                                           */
/* History:                                                                  */
/* 1. Date:                                                                  */
/*    Author:                                                                */
/*    Modification:                                                          */
/*                                                                           */
/*****************************************************************************/


#include "v_IO.h"
#include "v_private.h"

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_V_NSPRINTF_C

/*****************************************************************************
 Function   : VOS_nvsprintf_s
 Description:
 Input      :
            :
 Return     :
 Other      :
 *****************************************************************************/
VOS_INT VOS_nvsprintf_s(VOS_CHAR * str, VOS_SIZE_T ulMaxStrLen, VOS_SIZE_T ulCount, const VOS_CHAR *format, va_list arguments)
{
    VOS_SIZE_T                          ulPrintLen;

    if ( ulCount > VOS_SECUREC_MEM_MAX_LEN )
    {
        return -1;
    }

    if (( NULL == str ) || ( NULL == format ) || (ulMaxStrLen == 0))
    {
        return -1;
    }
    else
    {
        if ( VOS_NULL_PTR == VOS_MemSet_s(str, ulMaxStrLen, 0, ulMaxStrLen) )
        {
            mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);

            return -1;
        }
    }

    ulPrintLen = (ulMaxStrLen > ulCount)?(ulCount + 1) : ulMaxStrLen;

    return (VOS_INT)vscnprintf(str, (VOS_UINT32)(ulPrintLen), (const VOS_CHAR *) format, arguments);

}

/*****************************************************************************
 Function   : VOS_nsprintf_s
 Description:
 Input      :
            :
 Return     :
 Other      :
 *****************************************************************************/
VOS_INT VOS_nsprintf_s(VOS_CHAR *str, VOS_SIZE_T ulMaxStrLen, VOS_SIZE_T ulCount, const VOS_CHAR *fmt, ...)
{
    VOS_SIZE_T                          ulPrintLen;

    /*lint -e530 -e830 */
    va_list arg;
    register VOS_INT nc;

    if ( ulCount > VOS_SECUREC_MEM_MAX_LEN )
    {
        return -1;
    }

    if (( NULL == str ) || ( NULL == fmt ) || (ulMaxStrLen == 0))
    {
        return -1;
    }
    else
    {
        if ( VOS_NULL_PTR == VOS_MemSet_s(str, ulMaxStrLen, 0, ulMaxStrLen) )
        {
            mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, (VOS_INT)ulMaxStrLen, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);

            return -1;
        }
    }

    /*lint -e586*/
    va_start(arg, fmt);
    /*lint +e586*/
    ulPrintLen = (ulMaxStrLen > ulCount)?(ulCount + 1) : ulMaxStrLen;

    nc = (VOS_INT)vscnprintf(str, (VOS_UINT32)(ulPrintLen), (const VOS_CHAR *) fmt, arg);

    /*lint -e586*/
    va_end(arg);
    /*lint +e586*/
    return (nc);
    /*lint +e530 +e830 */
}


/*****************************************************************************
 Function   : vos_printf
 Description: Print function
 Input      : format -- Format string to print
 Output     : None
 Return     : VOS_OK on success and VOS_ERROR on error
 *****************************************************************************/
VOS_INT32 vos_printf( const VOS_CHAR * format, ... )
{
    VOS_UINT32 ulReturn = VOS_OK;

    /*lint -e530 -e830 */
    va_list    argument;
    VOS_CHAR   output_info[VOS_MAX_PRINT_LEN + 4] = {0};

    /*lint -e586*/
    va_start( argument, format );
    /*lint +e586*/

    if ( VOS_NULL_PTR != format )
    {
    (VOS_VOID)vscnprintf(output_info, VOS_MAX_PRINT_LEN, format, argument);
    }

    /*lint -e586*/
    va_end( argument );
    /*lint +e586*/
    /*lint +e530 +e830 */

    output_info[VOS_MAX_PRINT_LEN - 1] = '\0';



    (VOS_VOID)printk( "%s",output_info );


    return (VOS_INT32)ulReturn;
}


