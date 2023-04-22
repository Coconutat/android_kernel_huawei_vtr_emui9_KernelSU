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

// *****************************************************************************
// PROJECT   : MSP_CPV100R001C00
// SUBSYSTEM : AT
// MODULE    :
// OWNER     :
// *****************************************************************************

#ifndef __AT_LTE_UPGRADE_PROC_H__
#define __AT_LTE_UPGRADE_PROC_H__

#include "gen_msg.h"

#define UPGRADE_STR_BUF_SIZE 512

/*#define DLOAD_OK              0
 */
/*#define DLOAD_ERROR          (-1)
 */

#define AT_UPGRADE_DLOADVER          1 /*at^dloadver?
 */
#define AT_UPGRADE_DLOADINFO         2 /*at^dloadinfo?
 */
#define AT_UPGRADE_AUTHORITYVER      3 /*at^authorityver?
 */
#define AT_UPGRADE_AUTHORITYID       4 /*at^authorityid?
 */
#define AT_UPGRADE_GODLOAD           5 /*at^godload
 */
#define AT_UPGRADE_NVBACKUP          6 /*at^nvbackup
 */
#define AT_UPGRADE_NVRESTORE         7 /*at^nvrestore
 */
#define AT_UPGRADE_NVRSTSTTS         8 /*at^nvrststts
 */
#define AT_UPGRADE_RESET             9 /*at^reset
 */
#define AT_UPGRADE_DATAMODE          10/*at^datamode
 */


#endif





