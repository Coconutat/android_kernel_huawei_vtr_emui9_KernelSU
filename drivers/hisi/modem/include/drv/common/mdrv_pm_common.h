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

#ifndef __MDRV_COMMON_PM_H__
#define __MDRV_COMMON_PM_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "mdrv_om_common.h"

struct mdrv_pm_profile
{
	unsigned int max_profile;
	unsigned int min_profile;
};

typedef enum tagPWC_COMM_MODEM_E
{
    PWC_COMM_MODEM_0 = 0,
    PWC_COMM_MODEM_1 = 1,
    PWC_COMM_MODEM_2 = 2,
    PWC_COMM_MODEM_BUTT
}PWC_COMM_MODEM_E;


/******************************************************
 * *睡眠投票ID ,从0开始，最多32个
 * *涉及投票的组件需要在此添加LOCK ID
 * *请同步修改wakelock_balong.c中的debug_wakelock
 * *******************************************************/
typedef enum tagPWC_CLIENT_ID_E
{
    PWRCTRL_SLEEP_BEGIN=0X100,
    PWRCTRL_SLEEP_TLPHY = PWRCTRL_SLEEP_BEGIN,   /*MSP--fuxin*/
    PWRCTRL_SLEEP_PS_G0,        /*GU--ganlan*/
    PWRCTRL_SLEEP_PS_W0,       /*GU--ganlan*/
    PWRCTRL_SLEEP_PS_G1,        /*GU--ganlan*/
    PWRCTRL_SLEEP_PS_W1,       /*GU--ganlan*/
    PWRCTRL_SLEEP_FTM,           /*GU--zhangyizhan*/
    PWRCTRL_SLEEP_FTM_1,       /*GU--zhangyizhan*/
    PWRCTRL_SLEEP_NAS,           /*GU--zhangyizhan*/
    PWRCTRL_SLEEP_NAS_1,       /*GU--zhangyizhan*/
    PWRCTRL_SLEEP_OAM,          /* GU--zhangyizhan */
    PWRCTRL_SLEEP_SCI0,            /* LTE --yangzhi */
    PWRCTRL_SLEEP_SCI1,            /* LTE --yangzhi */
    PWRCTRL_SLEEP_TLPS,            /* lte ps */
    PWRCTRL_SLEEP_TLPS1,           /* lte ps1 */
    PWRCTRL_SLEEP_DSFLOW,          /* NAS --zhangyizhan */
    PWRCTRL_SLEEP_TEST,            /* PM  ---shangmianyou */
    PWRCTRL_SLEEP_UART0,        /*UART0 -zhangliangdong */
    PWRCTRL_SLEEP_TDS,         /*TRRC&TL2----leixiantiao*/

    PWRCTRL_SLEEP_CDMAUART,         /*drv cdma uart 数传*/
    PWRCTRL_SLEEP_USIM,             /*oam*/
    PWRCTRL_SLEEP_DSPPOWERON,       /*v8r1 ccore 提供给GUTL DSP作为c核上电初始化投票用*/
    PWRCTRL_SLEEP_RESET,            /* RESET  ---nieluhua */
    PWRCTRL_SLEEP_PS_G2,        /*GU--ganlan*/
    PWRCTRL_SLEEP_FTM_2,       /*GU--zhangyizhan*/
    PWRCTRL_SLEEP_NAS_2,       /*GU--zhangyizhan*/
    PWRCTRL_SLEEP_1X,          /*CDMA--ganlan*/
    PWRCTRL_SLEEP_HRPD,        /*CDMA--ganlan;0x11A*/
    PWRCTRL_SLEEP_MSP,        /*cuijunqiang*/
    PWRCTRL_SLEEP_VOWIFI,        /*zhangdongfeng,xiamiaofang*/
    /*以下部分的ID已经不使用了，后期会删除*/
    PWRCTRL_SLEEP_RNIC,
    PWRCTRL_TEST_DEEPSLEEP  = 0x11f,
    PWRCTRL_SLEEP_END =0x120
}PWC_CLIENT_ID_E;

typedef enum tagPWC_COMM_MODE_E
{
    PWC_COMM_MODE_GSM=0,
    PWC_COMM_MODE_WCDMA=1,
    PWC_COMM_MODE_LTE=2,
    PWC_COMM_MODE_TDS=3,
    PWC_COMM_MODE_CDMA_1X=4,
    PWC_COMM_MODE_CDMA_HRPD=5,
    PWC_COMM_MODE_NUMBER,
    PWC_COMM_MODE_COMMON=PWC_COMM_MODE_NUMBER,
	PWC_COMM_MODE_REMOTE_CLK_W,
	PWC_COMM_MODE_REMOTE_CLK_G,
    PWC_COMM_MODE_BUTT
}PWC_COMM_MODE_E;
/*注意:枚举扩充时，若PWC_COMM_MODEM_BUTT*PWC_COMM_MODE_BUTT > 32时需要知会各投票组件修改投票代码
       不要在PWC_COMM_MODE_CDMA_HRPD之前插入其他外设*/


/*
 * PM_OM_MOD_ID_ENUM - 使用pmom(pm dump/pm log/pm info)的模块ID
 */
enum PM_OM_MOD_ID_ENUM
{
    PM_MOD_BEGIN     =  32,
    PM_MOD_AP_OSA  =  OM_AP_OSA,
    PM_MOD_CP_OSA  =  OM_CP_OSA,
    PM_MOD_CP_MSP  =  OM_CP_MSP_SLEEP,
    PM_OM_CAS_1X     =  33,
    PM_OM_CPROC_1X   =  34,
    PM_OM_CAS_EVDO   =  35,
    PM_OM_CPROC_EVDO =  36,
    PM_OM_TLPHY      =  37,
    PM_OM_TRRC       =  38,
    PM_OM_LRRC       =  39,
    PM_OM_WPHY       =  40,
    PM_OM_GPHY       =  41,
    PM_MOD_END       =  43,
	/* 上层模块ID不可以超过48 */
    PM_OM_MOD_ID_ENUM_MAX = 48
};

/*
 * PM_OM_MAGIC_ENUM - 与PM_OM_MOD_ID_ENUM对应的模块的魔数
 * 用四个字符的ascii码标识, 小端字节序
 */
enum PM_OM_MAGIC_ENUM
{
	PM_OM_MAGIC_AOSA = 0x41534F41, /*AOSA : OSA ACORE */
	PM_OM_MAGIC_COSA = 0x41534F43, /*COSA : OSA CCORE */
	PM_OM_MAGIC_CMSP = 0x50534D43, /*CMSP : MSP CCORE */
	PM_OM_MAGIC_CASX = 0x58534143, /*CASX : CAS 1X    */
	PM_OM_MAGIC_CPRX = 0x58525043, /*CPRX : CPROC 1X  */
	PM_OM_MAGIC_CASE = 0x45534143, /*CASE : CAS EVDO  */
	PM_OM_MAGIC_CPRE = 0x45525043, /*CPRE : CPROC EVDO*/
	PM_OM_MAGIC_TLPY = 0x48504C54, /*TLPH : TL PHY    */
	PM_OM_MAGIC_TRRC = 0x43525254, /*TRRC : T RRC     */
	PM_OM_MAGIC_LRRC = 0x4352524C, /*LRRC : L RRC     */
	PM_OM_MAGIC_WPHY = 0x59485057, /*WPHY : W PHY     */
	PM_OM_MAGIC_GPHY = 0x59485047  /*GPHY : G PHY     */
};

/*
 * struct pm_info_usr_data - 记录功耗状态信息的结构体定义
 * @mod_id	: 组件ID, 对应PM_OM_MOD_ID_ENUM
 * @magic	: 组件magic, 解析后的标识, 对应PM_OM_MAGIC_ENUM
 * @buf		: 组件记录功耗状态信息的的内存起始地址
 * @buf_len	: 组件记录功耗状态信息的的内存大小
 * @private	: 用户私有数据区
 */
struct pm_info_usr_data
{
	unsigned int mod_id;
	unsigned int magic;
	void         *buf;
	unsigned int buf_len;
	void         *context;
};

/* modem功耗状态信息收集回调函数指针 */
typedef int (*pm_info_cbfun)(struct pm_info_usr_data *usr_data);

/*****************************************************************************
* 函 数 名  : mdrv_pm_info_stat_register
*
* 功能描述  : 注册modem功耗状态信息收集回调函数和用户信息
*
* 输入参数  : pcbfun   modem功耗状态信息收集回调函数, 需调用mdrv_pm_log将数据上报
*             usr_data 用户数据, 不允许传入NULL指针
* 输出参数  :
*
* 返 回 值  : 0，执行成功；非零，失败
*
* 其它说明  :
*
*****************************************************************************/
int mdrv_pm_info_stat_register(pm_info_cbfun pcbfun, struct pm_info_usr_data *usr_data);

/*****************************************************************************
* 函 数 名  : mdrv_pm_log
*
* 功能描述  : 输出流程信息到log区
*
* 输入参数  : mod_id	模块id，使用enum PM_OM_MOD_ID_ENUM枚举类型定义的值
*            data_len	输出数据长度
*            data 		输出数据
* 输出参数  :
*
* 返 回 值  : 0，执行成功；非零，失败
*
* 其它说明  : 
*
*****************************************************************************/
int mdrv_pm_log(int mod_id,  unsigned int data_len , void *data);

#ifdef __cplusplus
}
#endif
#endif
