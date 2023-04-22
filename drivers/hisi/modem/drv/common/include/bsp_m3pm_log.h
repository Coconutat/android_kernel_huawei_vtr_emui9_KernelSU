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

#ifndef _BSP_M3PM_M3DPM_LOG_H_
#define _BSP_M3PM_M3DPM_LOG_H_

enum m3_mdm_dpm_log_mode
{
    m3_mdm_dpm_log_begin,
    m3_mdm_dpm_log_temp,
    m3_mdm_dpm_log_ipf,
    m3_mdm_dpm_log_butt
};

enum m3_mdm_pm_log_mode
{
    m3_mdm_pm_log_a9,
    m3_mdm_pm_log_a9_sr,
    m3_mdm_dpm_log_fail
};


#define SHM_MEM_M3PM_LOG_ADDR           ((unsigned long)SHM_BASE_ADDR + SHM_OFFSET_M3PM_LOG)
#define SHM_MEM_M3PM_LOG_SIZE           (0x44)
#define M3PM_AP_SHOW_ADDR               (SHM_MEM_M3PM_LOG_ADDR+SHM_MEM_M3PM_LOG_SIZE)
#define M3PM_AP_SHOW_SIZE               (0x3C)

/* k3平台 m3上modem相关 log */
#define M3PM_LOG_MODEM_DOWN_OFFSET      0
#define M3PM_LOG_MODEM_DOWN_SIZE        0x8

#define M3PM_LOG_MODEM_UP_OFFSET        /*lint -save -e778 -e835*/(M3PM_LOG_MODEM_DOWN_OFFSET+M3PM_LOG_MODEM_DOWN_SIZE)/*lint -restore */
#define M3PM_LOG_MODEM_UP_SIZE          (0xC)

#define M3PM_LOG_MODEM_SUSPEND_OFFSET   (M3PM_LOG_MODEM_UP_OFFSET+M3PM_LOG_MODEM_UP_SIZE)
#define M3PM_LOG_MODEM_SUSPEND_SIZE     (0x10)

#define M3PM_LOG_MODEM_RESUME_OFFSET    (M3PM_LOG_MODEM_SUSPEND_OFFSET+M3PM_LOG_MODEM_SUSPEND_SIZE)
#define M3PM_LOG_MODEM_RESUME_SIZE      (0x8)

#define M3PM_LOG_MODEM_RESUME_OK_OFFSET (M3PM_LOG_MODEM_RESUME_OFFSET+M3PM_LOG_MODEM_RESUME_SIZE)
#define M3PM_LOG_MODEM_RESUME_OK_SIZE   (0xC)

#define M3PM_LOG_MODEM_DPM_OFFSET       (M3PM_LOG_MODEM_RESUME_OK_OFFSET+M3PM_LOG_MODEM_RESUME_OK_SIZE)
#define M3PM_LOG_MODEM_DPM_SIZE         (0x4*m3_mdm_dpm_log_butt) 

#define M3PM_LOG_MDM_A9_SIZE            (M3PM_LOG_MODEM_DOWN_SIZE+M3PM_LOG_MODEM_UP_SIZE)
#define M3PM_LOG_MDM_SR_SIZE            (M3PM_LOG_MODEM_SUSPEND_SIZE+M3PM_LOG_MODEM_RESUME_SIZE+M3PM_LOG_MODEM_RESUME_OK_SIZE)
#define M3PM_LOG_MDM_A9_SR_SIZE         (M3PM_LOG_MDM_A9_SIZE+M3PM_LOG_MDM_SR_SIZE)

/* mbb m3pm相关信息 */

/* mbb m3pm   M3PM_AP_SHOW_ADDR*/
#define M3PM_AP_SHOW_OFFSET        (0)
#if ((defined CONFIG_M3PM)&&(defined M3PM_LOG_AP_SHOW))
#define DUMP_M3PM_SIZE          (0x100)
#else
#define DUMP_M3PM_SIZE          (0x0)
#endif


#define M3PM_AP_SHOW_MAGIC_NUM          0x5a5a5a33

#define M3PM_AP_SHOW_MAGIC_0            (M3PM_AP_SHOW_OFFSET)
#define M3PM_AP_SHOW_M3_DPM_FAIL_CNT    (M3PM_AP_SHOW_MAGIC_0+4)
#define M3PM_AP_SHOW_M3_SLEEP_CNT       (M3PM_AP_SHOW_M3_DPM_FAIL_CNT+4)
#define M3PM_AP_SHOW_M3_SLEEP_NOSM_CNT  (M3PM_AP_SHOW_M3_SLEEP_CNT+4)

#define M3PM_AP_SHOW_MAGIC_1           (M3PM_AP_SHOW_M3_SLEEP_NOSM_CNT+4)
#define M3PM_AP_SHOW_BEFORE_DPM_S      (M3PM_AP_SHOW_MAGIC_1+4)
#define M3PM_AP_SHOW_AFTER_DPM_S       (M3PM_AP_SHOW_BEFORE_DPM_S+4)
#define M3PM_AP_SHOW_BUCKOFF_S_END     (M3PM_AP_SHOW_AFTER_DPM_S+4)
#define M3PM_AP_SHOW_BUCKOFF_R_BEGIN   (M3PM_AP_SHOW_BUCKOFF_S_END+4)
#define M3PM_AP_SHOW_BEFORE_DPM_R      (M3PM_AP_SHOW_BUCKOFF_R_BEGIN+4)
#define M3PM_AP_SHOW_AFTER_DPM_R       (M3PM_AP_SHOW_BEFORE_DPM_R+4)
#define M3PM_AP_SHOW_END               (M3PM_AP_SHOW_AFTER_DPM_R+4)

#define M3PM_AP_SHOW_MAGIC_2           (M3PM_AP_SHOW_END+4)


#ifdef CONFIG_PM_OM
void m3_mdm_sr_update(void);
void m3_mdm_pm_dpm_log(void);

#else
static inline void m3_mdm_sr_update(void){return;}
static inline void m3_mdm_pm_dpm_log(void){return;}

#endif

/*  acore  */
#ifdef M3PM_LOG_AP_SHOW
u32 m3_deepsleep_times(void);
#else
static inline void print_m3_lopower_info(void){return;}
static inline u32 m3_deepsleep_times(void){return 0;}

#endif

#endif

