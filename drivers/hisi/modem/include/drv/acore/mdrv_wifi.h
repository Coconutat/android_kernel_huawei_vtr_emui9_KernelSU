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

#ifndef __MDRV_ACORE_WIFI_H__
#define __MDRV_ACORE_WIFI_H__

/*************************WIFI START**********************************/


extern unsigned short BSP_WifiCalDataLen(unsigned short usLen);
#define DRV_WifiCalDataLen(usLen) BSP_WifiCalDataLen(usLen)

/*****************************************************************************
 函 数 名  : WIFI_TEST_CMD
 功能描述  : 测试命令
 输入参数  : cmdStr；命令字符串
 输出参数  : 无
 返回值    ：无
*****************************************************************************/
extern void WIFI_TEST_CMD(char * cmdStr);

/*****************************************************************************
 函 数 名  : WIFI_GET_TCMD_MODE
 功能描述  : 获取测试命令的模式
 输入参数  : 无
 输出参数  : 16：校准测试模式
             17：表示处于发射模式
             18：表示接收模式
 返回值    ：函数执行的状态结果值
*****************************************************************************/
extern int WIFI_GET_TCMD_MODE(void);

/*****************************************************************************
 函 数 名  : WIFI_POWER_START
 功能描述  : WIFI上电
 输入参数  : 无
 输出参数  : 无
 返回值    ： 0: execute ok
              1: execute failed
*****************************************************************************/
extern int WIFI_POWER_START(void);

/*****************************************************************************
 函 数 名  : WIFI_POWER_SHUTDOWN
 功能描述  : WIFI下电
 输入参数  : 无
 输出参数  : 无
 返回值    ： 0: execute ok
              1: execute failed
*****************************************************************************/
extern int WIFI_POWER_SHUTDOWN(void);

/*****************************************************************************
 函 数 名  : WIFI_GET_STATUS
 功能描述  : WIFI状态获取
 输入参数  : 无
 输出参数  : 无
 返回值    ： 0: wifi is off
              1: wifi is in normal mode
              2: wifi is in tcmd mode
*****************************************************************************/
extern int WIFI_GET_STATUS(void);

/*****************************************************************************
 函 数 名  : WIFI_GET_RX_DETAIL_REPORT
 功能描述  : get result of rx report: totalPkt, GoodPkt, ErrorPkt
 输入参数  : 无
 输出参数  : totalPkt、goodPkt、badPkt
 返回值    ：无
*****************************************************************************/
extern void WIFI_GET_RX_DETAIL_REPORT(int* totalPkt,int* goodPkt,int* badPkt);

/*****************************************************************************
 函 数 名  : WIFI_GET_RX_PACKET_REPORT
 功能描述  : get result of rx ucast&mcast packets
 输入参数  : 无
 输出参数  : ucastPkts、mcastPkts
 返回值    ：无
*****************************************************************************/
extern void  WIFI_GET_RX_PACKET_REPORT(unsigned int *ucastPkts, unsigned int *mcastPkts);

/*****************************************************************************
 函 数 名  : WIFI_GET_PA_CUR_MODE
 功能描述  : get the currrent PA mode of the wifi chip
 输入参数  : 无
 输出参数  : 无
 返回值    ：0:  (WIFI_ONLY_PA_MODE) WIFI chip is in PA mode
             1:  (WIFI_ONLY_NOPA_MODE) WIFI chip is in no PA mode
             -1: wifi chip is in abnormal mode
*****************************************************************************/
extern int WIFI_GET_PA_CUR_MODE(void);

/*****************************************************************************
 函 数 名  : WIFI_GET_PA_MODE
 功能描述  : get the support PA mode of wifi chip
 输入参数  : 无
 输出参数  : 无
 返回值    ：0:  (WIFI_ONLY_PA_MODE) WIFI suppport only PA mode
             1:  (WIFI_ONLY_NOPA_MODE) WIFI suppport only no PA mode
             2:  (WIFI_PA_NOPA_MODE) WIFI suppport both PA &  no PA mode
             -1: failed
*****************************************************************************/
extern int WIFI_GET_PA_MODE(void);

/*****************************************************************************
 函 数 名  : WIFI_SET_PA_MODE
 功能描述  : set the PA mode of wifi chip
 输入参数  : 0:  (WIFI_ONLY_PA_MODE) WIFI suppport only PA mode
             1:  (WIFI_ONLY_NOPA_MODE) WIFI suppport only no PA mode
             2:  (WIFI_PA_NOPA_MODE) WIFI suppport both PA &  no PA mode
 输出参数  : 无
 返回值    ：0: success
             -1: failed
*****************************************************************************/
extern int WIFI_SET_PA_MODE(int wifiPaMode);

/*****************************************************************************
 函 数 名  : DRV_WIFI_DATA_RESERVED_TAIL
 功能描述  : WIFI计算需要数据块内存
 输入参数  : usLen - 用户申请数据长度Len
 输出参数  : 无
 返 回 值  : 数据区的尾部预留的长度
*****************************************************************************/
extern unsigned int DRV_WIFI_DATA_RESERVED_TAIL(unsigned int len);

/*****************************************************************************
 函 数 名  : DRV_WIFI_SET_RX_FCTL
 功能描述  : 设置WIFI接收流控标识
 输入参数  : para1、para2
 输出参数  : 无
 返 回 值  : BSP_OK/BSP_ERROR
*****************************************************************************/
extern unsigned int DRV_WIFI_SET_RX_FCTL(unsigned int para1, unsigned int para2);

/*****************************************************************************
 函 数 名  : DRV_WIFI_CLR_RX_FCTL
 功能描述  : 清除WIFI接收流控标识
 输入参数  : para1、para2
 输出参数  : 无
 返 回 值  : 1 : 有
             0 : 无
*****************************************************************************/
extern unsigned int DRV_WIFI_CLR_RX_FCTL(unsigned int para1, unsigned int para2);

/*****************************************************************************
 函 数 名  : DRV_AT_GET_USER_EXIST_FLAG
 功能描述  : 返回当前是否有USB连接或者WIFI用户连接(C核调用)
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 1 : 有
             0 : 无
*****************************************************************************/
extern unsigned int  DRV_AT_GET_USER_EXIST_FLAG(void);


/*************************WIFI END************************************/

#endif

