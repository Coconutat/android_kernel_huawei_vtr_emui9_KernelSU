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
/* FileName: v_sprintf.c                                                     */
/*                                                                           */
/* Author:                                                                   */
/*                                                                           */
/* Version: 1.0                                                              */
/*                                                                           */
/* Date:                                                                     */
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
#define    THIS_FILE_ID        PS_FILE_ID_V_SPRINTF_C

/*****************************************************************************
 Function   : VOS_vsprintf_s
 Description:
 Input      :
            :
 Return     :
 Other      :
 *****************************************************************************/
VOS_INT VOS_vsprintf_s(VOS_CHAR * str, VOS_SIZE_T ulDestSize, const VOS_CHAR *format,va_list argument)
{
    if (( VOS_NULL_PTR == str ) || ( VOS_NULL_PTR == format ) )
    {
        return -1;
    }

    if ( ulDestSize > VOS_SECUREC_MEM_MAX_LEN )
    {
        return -1;
    }

    return (VOS_INT)vscnprintf(str, (VOS_UINT32)(ulDestSize), (const VOS_CHAR *) format, argument);
}

/*****************************************************************************
 Function   : VOS_sprintf_s
 Description:
 Input      :
            :
 Return     :
 Other      :
 *****************************************************************************/
VOS_INT VOS_sprintf_s(VOS_CHAR *str, VOS_SIZE_T ulDestSize, const VOS_CHAR *fmt, ...)
{
    /*lint -e530 -e830 */
    va_list arg;
    register VOS_INT nc;

    /* HY17247 */
    if (( VOS_NULL_PTR == str ) || ( VOS_NULL_PTR == fmt ) )
    {
        return -1;
    }

    if ( ulDestSize > VOS_SECUREC_MEM_MAX_LEN )
    {
        return -1;
    }

    /*lint -e586*/
    va_start(arg, fmt);
    /*lint +e586*/
    nc = (VOS_INT)vscnprintf(str, (VOS_UINT32)(ulDestSize), (const VOS_CHAR *) fmt, arg);

    /*lint -e586*/
    va_end(arg);
    /*lint +e586*/
    return (nc);
    /*lint +e530 +e830 */
}

#define LENGTH_OF_PRINT_LINE_BUF        (1024)

/*****************************************************************************
 Function   : vos_assert
 Description: Report an assert
 Input      : pcFileName -- File name of this assert occuring
              ulLineNo   -- Line number of this assert occuring
 Output     : None
 Return     : None
 *****************************************************************************/
VOS_VOID vos_assert( VOS_UINT32 ulFileID, VOS_INT LineNo)
{
    /*lint -e813 */
    VOS_CHAR vos_PrintBuf[LENGTH_OF_PRINT_LINE_BUF];
    /*lint +e813 */

    VOS_CHAR *String = "Assert";

    (VOS_VOID)VOS_sprintf_s( vos_PrintBuf, LENGTH_OF_PRINT_LINE_BUF, "%s File: %d, Line: %d", String, ulFileID, LineNo );

    vos_printf("\n %s.\r\n",vos_PrintBuf);

    return;
}




