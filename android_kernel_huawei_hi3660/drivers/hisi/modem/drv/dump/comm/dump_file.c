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
 
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/kernel.h> 
#include <asm/string.h>
#include "bsp_rfile.h"
#include "osl_types.h"
#include "drv_comm.h"
#include "dump_print.h"

/*****************************************************************************
* º¯ Êý Ãû  : dump_save_file
* ¹¦ÄÜÃèÊö  : ÓÃÓÚÖ´ÐÐ±£´æÎÄ¼þµÄ¹¦ÄÜ
*
* ÊäÈë²ÎÊý  :
* Êä³ö²ÎÊý  :

* ·µ »Ø Öµ  :

*
* ÐÞ¸Ä¼ÇÂ¼  : 2016Äê1ÔÂ4ÈÕ17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_file(char * file_name, void * addr, u32 len)
{
    int ret = -1;
    int fd = -1;
    int bytes = -1;

    if(!file_name || !addr || !len)
    {
        return;
    }
    fd = bsp_open(file_name, O_CREAT|O_RDWR|O_SYNC, 0660);
    if(fd < 0)
    {
        dump_fetal("creat file %s failed\n", file_name);
        return;
    }

    bytes = bsp_write(fd, addr, len);
    if(bytes != len)
    {
        dump_fetal("write data to %s failed, bytes %d, len %d\n", file_name, bytes, len);
        (void)bsp_close(fd);
        return;
    }

    ret = bsp_close(fd);
    if(0 != ret)
    {
        dump_fetal("close file failed, ret = %d\n", ret);
        return;
    }

}

/*****************************************************************************
* º¯ Êý Ãû  : dump_create_dir
* ¹¦ÄÜÃèÊö  : ´´½¨reset¡.logµÄ±£´æÄ¿Â¼
*
* ÊäÈë²ÎÊý  :
* Êä³ö²ÎÊý  :

* ·µ »Ø Öµ  :

*
* ÐÞ¸Ä¼ÇÂ¼  : 2016Äê1ÔÂ4ÈÕ17:05:33   lixiaofan  creat
*
*****************************************************************************/
int dump_append_file(char * dir, char *filename, void * address, u32 length, u32 max_size)
{
    int ret = BSP_OK ;
    int fd  ;
    u32 bytes  ;
    int len ;

    fd = bsp_access((s8*)dir, 0);
    if(0 != fd)
    {
        fd  = bsp_mkdir((s8*)dir, 0660);
        if(fd < 0)
        {
            dump_fetal("create om dir failed! ret = %d\n", fd);

            return fd;
        }
    }

    if(BSP_OK != fd)
    {
        dump_fetal("create dir failed! ret = %d\n", ret);
        goto out;
    }

    ret = bsp_access((s8*)filename, 0);
    if(BSP_OK != ret)
    {
        fd = bsp_open((s8*)filename, RFILE_CREAT|RFILE_RDWR, 0755);
        if(fd < 0)
        {
            dump_fetal("open failed while mode is create, ret = %d\n", fd);
            goto out;
        }
    }
    else
    {
        fd = bsp_open((s8*)filename, RFILE_APPEND|RFILE_RDWR, 0755);
        if(fd < 0)
        {
            dump_fetal("open failed while mode is append, ret = %d\n", fd);
            goto out;
        }

    }

    len = bsp_lseek((u32)fd, 0, SEEK_END);
    if(ERROR == len)
    {
        dump_fetal("seek failed! ret = %d\n", len);
        goto out1;
    }

    if (len >= (int)max_size)
    {
        (void)bsp_close((u32)fd);
        ret = bsp_remove((s8*)filename);
        if (BSP_OK != ret)
        {
            dump_fetal("remove failed! ret = %d\n", ret);
            goto out;
        }

        fd = bsp_open((s8*)filename, RFILE_CREAT|RFILE_RDWR, 0755);
        if(fd < 0)
        {
            dump_fetal("create failed! ret = %d\n", fd);
            goto out;
        }
    }

    bytes = (u32)bsp_write((u32)fd, address, length);
    if(bytes != length)
    {
        dump_fetal("write data failed! ret = %d\n", bytes);
        ret = BSP_ERROR;
        goto out1;
    }

    (void)bsp_close((u32)fd);

    return BSP_OK;

out1:
    (void)bsp_close((u32)fd);
out:
    return ret;
}

/*****************************************************************************
* º¯ Êý Ãû  : dump_create_dir
* ¹¦ÄÜÃèÊö  : ´´½¨cp_logµÄÄ¿Â¼±£Ö¤hidpµÈ¹¤¾ßÄÜ¹»Õý³£µ¼log
*
* ÊäÈë²ÎÊý  :
* Êä³ö²ÎÊý  :

* ·µ »Ø Öµ  :

*
* ÐÞ¸Ä¼ÇÂ¼  : 2016Äê1ÔÂ4ÈÕ17:05:33   lixiaofan  creat
*
*****************************************************************************/
int dump_create_dir(char *path)
{
    int fd = -1;
    char* dir_name = NULL;
    u32 len = 0;

    if(path == NULL)
    {
        return BSP_ERROR;
    }
    dump_fetal("path = %s\n", path);

    fd = bsp_access(path, 0); 
    if(0 != fd)
    {
        /*rfile´´½¨Ä¿Â¼²»ÔÊÐíÊ¹ÓÃ/½áÊøÂ·¾¶*/
        len = strlen(path);
        if(path[len - 1] == '/')
        {
            dir_name = kmalloc((len + 1), GFP_KERNEL);
            if(dir_name == NULL)
            {
                dump_fetal("malloc memry fail\n");
                return BSP_ERROR;
            }
            memset(dir_name,'\0',(len + 1));
            memcpy(dir_name,path,(len - 1));
            fd  = bsp_mkdir(dir_name, 0770);
        }
        else
        {
            fd  = bsp_mkdir(path, 0770);
        }
        if(dir_name != NULL)
        {
            kfree(dir_name);
        }
        if(fd < 0)
        {
            dump_fetal("create dir fail,fd = %d\n ",fd);
            return fd;
        }
    }

    return BSP_OK;
}
