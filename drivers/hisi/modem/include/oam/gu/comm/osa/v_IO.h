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
/* FileName: v_IO.c                                                          */
/*                                                                           */
/* Author: Yang Xiangqian                                                    */
/*                                                                           */
/* Version: 1.0                                                              */
/*                                                                           */
/* Date: 2006-10                                                             */
/*                                                                           */
/* Description: implement I/O                                                */
/*                                                                           */
/* Others:                                                                   */
/*                                                                           */
/* History:                                                                  */
/* 1. Date:                                                                  */
/*    Author:                                                                */
/*    Modification: Create this file                                         */
/*                                                                           */
/* 2. Date: 2006-10                                                          */
/*    Author: Xu Cheng                                                       */
/*    Modification: Standardize code                                         */
/*                                                                           */
/*****************************************************************************/

#ifndef _V_IO_H
#define _V_IO_H


#include "stdarg.h"
#include "dopra_def.h"
#include "vos_config.h"
#include "v_typdef.h"
#include "v_lib.h"

#if (VOS_VXWORKS== VOS_OS_VER)
#include "stdio.h"
#endif

#if (VOS_LINUX != VOS_OS_VER)
#include "math.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

/*lint -e830*/

#define LONGINT        0x01        /* long integer */
#define LONGDBL        0x02        /* long double; unimplemented */
#define SHORTINT       0x04        /* short integer */
#define ALT            0x08        /* alternate form */
#define LADJUST        0x10        /* left adjustment */
#define ZEROPAD        0x20        /* zero (as opposed to blank) pad */
#define HEXPREFIX      0x40            /* add 0x or 0X prefix */

#if (OSA_CPU_CCPU == VOS_OSA_CPU)
#define    FLOAT_SUPPORT
#endif

#define VOS_MAX_PRINT_LEN           1000

#ifndef isascii
#define isascii(c) (((unsigned char)(c))<=0x7f)
#endif

#ifndef isupper
#define isupper(c)      (((c) >= 'A') && ((c) <= 'Z'))
#endif

#ifndef islower
#define islower(c)      (((c) >= 'a') && ((c) <= 'z'))
#endif

#ifndef isalpha
#define isalpha(c)      (isupper(c) || (islower(c)))
#endif

#ifndef isdigit
#define isdigit(c)      (((c) >= '0') && ((c) <= '9'))
#endif

#ifndef isspace
#define isspace(c)      (((c) == ' ') || ((c) == '\t') || \
                        ((c) == '\r') || ((c) == '\n'))
#endif

#define VOS_IsLeap(y) (((((y) % 4) == 0) && ((y) % 100) != 0) || (((y) % 400) == 0))

#define FRMWRI_BUFSIZE           900

#define    Str_Length      FRMWRI_BUFSIZE
#define    NULL_PTR        0

/* 11-bit exponent (VAX G floating point) is 308 decimal digits */
#define    MAXEXP        308

/* 128 bit fraction takes up 39 decimal digits; max reasonable precision */
#define    MAXFRACT      39

/*lint -save -e750 -e752 -e745*/
#define    DEFPREC        7
#define    DEFLPREC       16

#define    BUF        (MAXEXP+MAXFRACT+1)    /* + decimal point */

/*lint -save -e737*/
/*lint -e830 -esym(830,*)*/
#define ARG(basetype) \
    ulLong = (VOS_UINT32)(lFlags&LONGINT ? va_arg(argp, long basetype) : \
        lFlags&SHORTINT ? (short basetype)va_arg(argp, int) : \
        va_arg(argp, int))

/*lint +e830 +esym(830,*)*/
#define    todigit(c)    ((c) - '0')
#define    tochar(n)    ((n) + '0')

typedef VOS_INT32 (* VOS_PRINT_HOOK)( VOS_CHAR * str );


VOS_VOID  comio(VOS_UINT8 output, VOS_VOID *number,VOS_CHAR *);
VOS_INT32 _C_formatter(const VOS_CHAR *format,
                               VOS_VOID   put_one_char(VOS_UINT8, VOS_VOID *,VOS_CHAR *),
                               VOS_VOID       *secret_pointer,
                               va_list     ap,     VOS_CHAR * str);

char *exponent(register char *p,register int lexp,unsigned char fmtch);
int cvt(double number,register int prec,int flags,
                unsigned char fmtch,char *signp, char *startp, char*endp);
                /*lint -save -e750 -e752 -e745*/

char * vos_round(double fract,int *exp,
                    register char *start,
                    register char *end,
                       char ch,
                    char *signp);

#if (VOS_RTOSCK == VOS_OS_VER)
#define VOS_vsprintf_s vsprintf_s
#else
VOS_INT VOS_vsprintf_s(VOS_CHAR * str, VOS_SIZE_T ulDestSize, const VOS_CHAR *format,va_list argument);
#endif

#if (VOS_RTOSCK == VOS_OS_VER)
#define VOS_sprintf_s sprintf_s
#else
VOS_INT VOS_sprintf_s(VOS_CHAR *str, VOS_SIZE_T ulDestSize, const VOS_CHAR *fmt, ...);
#endif

#if (VOS_RTOSCK == VOS_OS_VER)
#define VOS_nvsprintf_s vsnprintf_s
#else
VOS_INT VOS_nvsprintf_s(VOS_CHAR * str, VOS_SIZE_T ulMaxStrLen, VOS_SIZE_T ulCount, const VOS_CHAR *format, va_list arguments);
#endif

#if (VOS_RTOSCK == VOS_OS_VER)
#define VOS_nsprintf_s snprintf_s
#else
VOS_INT VOS_nsprintf_s(VOS_CHAR *str, VOS_SIZE_T ulMaxStrLen, VOS_SIZE_T ulCount, const VOS_CHAR *fmt, ...);
#endif

typedef VOS_INT32 vos_quad_t;
#define QUAD_MAX    (0X7FFFFFFF)
#define QUAD_MIN    (-0X7FFFFFFF)

typedef VOS_UINT32   vos_u_quad_t;
#define UQUAD_MAX    (0XFFFFFFFF)
typedef vos_u_quad_t (*ccfntype)(const VOS_CHAR *, VOS_CHAR **, VOS_INT);

/*lint -save -e830 -specific(-e830)*/
/*lint -e830 -esym(830,*)*/
VOS_INT32  vos_printf( const VOS_CHAR * format, ... );
/*lint +e830 +esym(830,*)*/
/*lint -restore*/

VOS_VOID vos_assert( VOS_UINT32 ulFileID, VOS_INT LineNo);

#if (VOS_DEBUG == VOS_DOPRA_VER)
#define Print( fmt ) vos_printf((fmt))

#define Print1( fmt, larg1 ) vos_printf((fmt), (larg1))

#define Print2( fmt, larg1, larg2 ) vos_printf((fmt), (larg1), (larg2))

#define Print3( fmt, larg1, larg2, larg3)\
    vos_printf((fmt), (larg1), (larg2), (larg3))

#define Print4( fmt, larg1, larg2, larg3, larg4)\
    vos_printf((fmt), (larg1), (larg2), (larg3), (larg4))

#define Print5( fmt, larg1, larg2, larg3, larg4, larg5)\
    vos_printf((fmt), (larg1), (larg2), (larg3), (larg4), (larg5))
#else
#define Print( fmt ) ((VOS_VOID)0)

#define Print1( fmt, larg1 ) ((VOS_VOID)0)

#define Print2( fmt, larg1, larg2 ) ((VOS_VOID)0)

#define Print3( fmt, larg1, larg2, larg3) ((VOS_VOID)0)

#define Print4( fmt, larg1, larg2, larg3, larg4) ((VOS_VOID)0)

#define Print5( fmt, larg1, larg2, larg3, larg4, larg5) ((VOS_VOID)0)
#endif

#if (VOS_VXWORKS == VOS_OS_VER)

extern VOS_INT logMsg(VOS_CHAR * fmt, VOS_INT arg1, VOS_INT arg2,
                       VOS_INT arg3, VOS_INT arg4, VOS_INT arg5, VOS_INT arg6);

#if (VOS_DEBUG == VOS_DOPRA_VER)
#define LogPrint( fmt ) logMsg((fmt), 0, 0, 0, 0, 0, 0)

#define LogPrint1( fmt, larg1 ) logMsg((fmt), (larg1), 0, 0, 0, 0, 0)

#define LogPrint2( fmt, larg1, larg2 )\
    logMsg((fmt), (larg1), (larg2), 0, 0, 0, 0)

#define LogPrint3( fmt, larg1, larg2, larg3)\
    logMsg((fmt), (larg1), (larg2), (larg3), 0, 0, 0)

#define LogPrint4( fmt, larg1, larg2, larg3, larg4)\
    logMsg((fmt), (larg1), (larg2), (larg3), (larg4), 0, 0)

#define LogPrint5( fmt, larg1, larg2, larg3, larg4, larg5)\
    logMsg((fmt), (larg1), (larg2), (larg3), (larg4), (larg5), 0)

#define LogPrint6( fmt, larg1, larg2, larg3, larg4, larg5, larg6)\
    logMsg((fmt), (larg1), (larg2), (larg3), (larg4), (larg5), (larg6))
#else
#define LogPrint( fmt ) ((VOS_VOID)0)

#define LogPrint1( fmt, larg1 ) ((VOS_VOID)0)

#define LogPrint2( fmt, larg1, larg2 ) ((VOS_VOID)0)

#define LogPrint3( fmt, larg1, larg2, larg3) ((VOS_VOID)0)

#define LogPrint4( fmt, larg1, larg2, larg3, larg4) ((VOS_VOID)0)

#define LogPrint5( fmt, larg1, larg2, larg3, larg4, larg5) ((VOS_VOID)0)

#define LogPrint6( fmt, larg1, larg2, larg3, larg4, larg5, larg6) ((VOS_VOID)0)
#endif

#endif

#if (VOS_WIN32 == VOS_OS_VER)
#define LogPrint( fmt ) ((VOS_VOID)0)

#define LogPrint1( fmt, larg1 ) ((VOS_VOID)0)

#define LogPrint2( fmt, larg1, larg2 ) ((VOS_VOID)0)

#define LogPrint3( fmt, larg1, larg2, larg3) ((VOS_VOID)0)

#define LogPrint4( fmt, larg1, larg2, larg3, larg4) ((VOS_VOID)0)

#define LogPrint5( fmt, larg1, larg2, larg3, larg4, larg5) ((VOS_VOID)0)

#define LogPrint6( fmt, larg1, larg2, larg3, larg4, larg5, larg6) ((VOS_VOID)0)
#endif

#if (VOS_NUCLEUS== VOS_OS_VER)

#if (VOS_DEBUG == VOS_DOPRA_VER)
#define LogPrint( fmt ) logMsg((fmt), 0, 0, 0, 0, 0, 0)

#define LogPrint1( fmt, larg1 ) logMsg((fmt), (larg1), 0, 0, 0, 0, 0)

#define LogPrint2( fmt, larg1, larg2 )\
    logMsg((fmt), (larg1), (larg2), 0, 0, 0, 0)

#define LogPrint3( fmt, larg1, larg2, larg3)\
    logMsg((fmt), (larg1), (larg2), (larg3), 0, 0, 0)

#define LogPrint4( fmt, larg1, larg2, larg3, larg4)\
    logMsg((fmt), (larg1), (larg2), (larg3), (larg4), 0, 0)

#define LogPrint5( fmt, larg1, larg2, larg3, larg4, larg5)\
    logMsg((fmt), (larg1), (larg2), (larg3), (larg4), (larg5), 0)

#define LogPrint6( fmt, larg1, larg2, larg3, larg4, larg5, larg6)\
    logMsg((fmt), (larg1), (larg2), (larg3), (larg4), (larg5), (larg6))
#else
#define LogPrint( fmt ) ((VOS_VOID)0)

#define LogPrint1( fmt, larg1 ) ((VOS_VOID)0)

#define LogPrint2( fmt, larg1, larg2 ) ((VOS_VOID)0)

#define LogPrint3( fmt, larg1, larg2, larg3) ((VOS_VOID)0)

#define LogPrint4( fmt, larg1, larg2, larg3, larg4) ((VOS_VOID)0)

#define LogPrint5( fmt, larg1, larg2, larg3, larg4, larg5) ((VOS_VOID)0)

#define LogPrint6( fmt, larg1, larg2, larg3, larg4, larg5, larg6) ((VOS_VOID)0)
#endif

#endif

#if (VOS_LINUX == VOS_OS_VER)
/*#define vos_printf printk*/

#if (VOS_DEBUG == VOS_DOPRA_VER)
#define LogPrint( fmt ) vos_printf((fmt))

#define LogPrint1( fmt, larg1 ) vos_printf((fmt), (larg1))

#define LogPrint2( fmt, larg1, larg2 ) vos_printf((fmt), (larg1), (larg2))

#define LogPrint3( fmt, larg1, larg2, larg3)\
    vos_printf((fmt), (larg1), (larg2), (larg3))

#define LogPrint4( fmt, larg1, larg2, larg3, larg4)\
    vos_printf((fmt), (larg1), (larg2), (larg3), (larg4))

#define LogPrint5( fmt, larg1, larg2, larg3, larg4, larg5)\
    vos_printf((fmt), (larg1), (larg2), (larg3), (larg4), (larg5))

#define LogPrint6( fmt, larg1, larg2, larg3, larg4, larg5, larg6)\
    vos_printf((fmt), (larg1), (larg2), (larg3), (larg4), (larg5), (larg6))
#else
#define LogPrint( fmt ) ((VOS_VOID)0)

#define LogPrint1( fmt, larg1 ) ((VOS_VOID)0)

#define LogPrint2( fmt, larg1, larg2 ) ((VOS_VOID)0)

#define LogPrint3( fmt, larg1, larg2, larg3) ((VOS_VOID)0)

#define LogPrint4( fmt, larg1, larg2, larg3, larg4) ((VOS_VOID)0)

#define LogPrint5( fmt, larg1, larg2, larg3, larg4, larg5) ((VOS_VOID)0)

#define LogPrint6( fmt, larg1, larg2, larg3, larg4, larg5, larg6) ((VOS_VOID)0)
#endif

#endif

#if (VOS_RTOSCK == VOS_OS_VER)

#if (VOS_DEBUG == VOS_DOPRA_VER)
#define LogPrint( fmt ) SRE_Printf((fmt), 0, 0, 0, 0, 0, 0)

#define LogPrint1( fmt, larg1 ) SRE_Printf((fmt), (larg1), 0, 0, 0, 0, 0)

#define LogPrint2( fmt, larg1, larg2 )\
    SRE_Printf((fmt), (larg1), (larg2), 0, 0, 0, 0)

#define LogPrint3( fmt, larg1, larg2, larg3)\
    SRE_Printf((fmt), (larg1), (larg2), (larg3), 0, 0, 0)

#define LogPrint4( fmt, larg1, larg2, larg3, larg4)\
    SRE_Printf((fmt), (larg1), (larg2), (larg3), (larg4), 0, 0)

#define LogPrint5( fmt, larg1, larg2, larg3, larg4, larg5)\
    SRE_Printf((fmt), (larg1), (larg2), (larg3), (larg4), (larg5), 0)

#define LogPrint6( fmt, larg1, larg2, larg3, larg4, larg5, larg6)\
    SRE_Printf((fmt), (larg1), (larg2), (larg3), (larg4), (larg5), (larg6))
#else
#define LogPrint( fmt ) ((VOS_VOID)0)

#define LogPrint1( fmt, larg1 ) ((VOS_VOID)0)

#define LogPrint2( fmt, larg1, larg2 ) ((VOS_VOID)0)

#define LogPrint3( fmt, larg1, larg2, larg3) ((VOS_VOID)0)

#define LogPrint4( fmt, larg1, larg2, larg3, larg4) ((VOS_VOID)0)

#define LogPrint5( fmt, larg1, larg2, larg3, larg4, larg5) ((VOS_VOID)0)

#define LogPrint6( fmt, larg1, larg2, larg3, larg4, larg5, larg6) ((VOS_VOID)0)
#endif

#endif

#if ((VOS_DEBUG == VOS_DOPRA_VER)||(VOS_DEBUG == VOS_DOPRA_VER))
#define VOS_ASSERT(exp) \
    if(!(exp))\
    {\
        vos_assert(__FILE__, __LINE__);\
        return; }
#define VOS_ASSERT_RTN(exp, ret) \
    if(!(exp))\
    {\
        vos_assert(__FILE__, __LINE__);\
        return ret; }
#else
#define VOS_ASSERT( exp ) ( (VOS_VOID)0 )
#define VOS_ASSERT_RTN(exp, ret) ( (VOS_VOID)0 )
#endif

/*lint +e830*/

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif /* _V_IO_H */


