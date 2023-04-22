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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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

/*lint --e{533,830}*/
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>

#define HWLOG_TAG madapt
HWLOG_REGIST();



#define BSP_ERR_MADAPT_OK                    (0)
#define BSP_ERR_MADAPT_PARAM_ERR            (1)
#define BSP_ERR_MADAPT_MALLOC_FAIL            (2)
#define BSP_ERR_MADAPT_COPY_FROM_USER_ERR    (3)
#define BSP_ERR_MADAPT_OPEN_FILE_ERR        (4)
#define BSP_ERR_MADAPT_FILE_SIZE_ERR        (5)
#define BSP_ERR_MADAPT_READ_FILE_ERR        (6)
#define BSP_ERR_MADAPT_FILE_HDR_CHECK_ERR    (7)
#define BSP_ERR_MADAPT_NV_HDR_CHECK_ERR        (8)
#define BSP_ERR_MADAPT_WRITE_NV_ERR            (9)
#define BSP_ERR_MADAPT_TIMEOUT                (10)
#define BSP_ERR_MADAPT_PTR_NULL                (11)
#define BSP_ERR_MADAPT_READ_NV_ERR          (12)
#define BSP_ERR_MADAPT_REMOVE_FILE_ERR          (13)
#define BSP_ERR_MADAPT_COMMON_ERR            (100)
/*AR000AD08A sunjun/00290209 20180807 begin*/
#define BSP_ERR_MADAPT_NO_THIS_NV             (0x10100016)
/*AR000AD08A sunjun/00290209 20180807 end*/

#define MADAPT_DEVICE_NAME                "madapt"
#define MADAPT_DEVICE_CLASS                "madapt_class"
#define MADAPT_FILE_MAX_SIZE            (1024*1024)
#define MADAPT_MAX_USER_BUFF_LEN        (96)
#define MADAPT_FILE_END_FLAG            (0xFFFFFFFF)
#define MADAPT_FILE_NOTBIT_FLAG         (0xFFFFFFFF)
#define MADAPT_MIN_NV_MODEM_ID            (1)
#define MADAPT_MAX_NV_MODEM_ID            (3)
#define MADAPT_NVFILE_PATH    "/cust_spec/modemConfig/nvcfg/"
#define MADAPT_NVFILE_VERSION            (0x0601)
#define MADAPT_BIT_LENGTH_SIZE          (32)
#define MADAPT_MAX_NV_LENGTH_SIZE           (2048)
#define MADAPT_MODEMID_CONVERT(x)        (x-1)

#define MADAPT_MODEMID_INVALID          (0xFF)

#define NCFG_BIN_FILE_PATH_IN_MODEM_SEC "/mnt/modem/mnvm2:0/mbn_nv/sec_carrier.bin"
#define NCFG_BIN_FILE_PATH_IN_MODEM     "/mnt/modem/mnvm2:0/mbn_nv/carrier.bin"

struct madapt_file_stru {
    unsigned int    modem_id;
    unsigned int    len;
    char file[MADAPT_MAX_USER_BUFF_LEN];
};

struct madapt_header_type {
    char pad[256];
};

struct madapt_item_hdr_type {
    unsigned int     nv_item_number;
    unsigned int     nv_modem_id;
    unsigned int     nv_item_size;
};
struct madapt_item_ato_hdr_type {
    unsigned int     feature_index; //index
    unsigned int     nv_item_number; //nvid
    unsigned int     nv_modem_id;
    unsigned int     nv_item_byte_start; //字节偏移开始位置
    unsigned int     nv_item_byte_length; //长度
    unsigned int     nv_ato_bit_pos; // BIT位，如果为全0XFFFFFFFF，说明不是BIT类型，忽略
};
struct madapt_item_ato_hdr_bit_type {
    unsigned int     nv_item_number; //nvid
    unsigned int     nv_modem_id;
    unsigned int     nv_item_byte_start; //字节偏移开始位置
    unsigned int     nv_item_byte_length; //长度
    unsigned char    nv_bit_value[MADAPT_BIT_LENGTH_SIZE+1];
};

struct my_work_struct {
    int test;
    struct work_struct proc_nv;
};

struct nv_item_map {
    unsigned int start_num;
    unsigned int end_num;
};


static struct cdev cdev;
static unsigned int madapt_major;
static struct class *madapt_class;
static struct madapt_file_stru *kbuf;
static struct completion my_completion;
static struct my_work_struct test_work;
static ssize_t work_ret = BSP_ERR_MADAPT_OK;
static struct mutex madapt_mutex;

/*lint -save -e438*/
/*lint -save -e745 -e601 -e49 -e65 -e64 -e533 -e830*/
ssize_t madapt_dev_read(struct file *file, char __user *buf, size_t count,
                                loff_t *ppos);
ssize_t madapt_dev_write(struct file *file, const char __user *buf,
        size_t count, loff_t *ppos);

static const struct file_operations nv_fops = {
    .owner     = THIS_MODULE,
    .read     = madapt_dev_read,
    .write     = madapt_dev_write,
};

#ifdef CONFIG_HISI_BALONG_MODEM
#ifdef CONFIG_HISI_SECBOOT_IMG 
static unsigned int mdrv_nv_writeex(unsigned int modemid,
        unsigned int itemid, void *pdata, unsigned int ulLength)
{
    hwlog_err("mdrv_nv_writeex is null ,depend on FEATURE_NV_SEC_ON\n");
    return 1;
}
#else
extern unsigned int mdrv_nv_writeex(unsigned int modemid,
        unsigned int itemid, void *pdata, unsigned int ulLength);
#endif
extern unsigned int mdrv_nv_readex(unsigned int modemid,
        unsigned int itemid, void *pdata, unsigned int ulLength);
extern unsigned int mdrv_nv_get_length(unsigned int itemid, unsigned int *pulLength);
#else
static unsigned int mdrv_nv_writeex(unsigned int modemid,
        unsigned int itemid, void *pdata, unsigned int ulLength)
{
    hwlog_err("mdrv_nv_writeex is null ,depend on HISI_BALONG_MODEM\n");
    return 1;
}
static unsigned int mdrv_nv_readex(unsigned int modemid,
        unsigned int itemid, void *pdata, unsigned int ulLength)
{
    hwlog_err("mdrv_nv_readex is null ,depend on HISI_BALONG_MODEM\n");
    return 1;
}
static unsigned int mdrv_nv_get_length(unsigned int itemid, unsigned int *pulLength)
{
    hwlog_err("mdrv_nv_get_length is null ,depend on HISI_BALONG_MODEM\n");
    return 1;
}
#endif

/*lint -restore +e745 +e601 +e49 +e65 +e64 +e533 +e830*/
/*
data stru:
    ---------------------------------------------
    | NV ID | data length |    data      |
    ---------------------------------------------
*/

/*lint -save --e{529,527}*/
ssize_t madapt_dev_read(struct file *file, char __user *buf, size_t count,
                                loff_t *ppos){
#if 0
    ssize_t ret;
    struct nv_data_stru *kbuf = NULL;


    if ((NULL == buf)
        || (count <= NV_HEAD_LEN)
        || (count > NV_MAX_USER_BUFF_LEN)) {
        return BSP_ERR_NV_INVALID_PARAM;
    }

    kbuf = kmalloc(count+1, GFP_KERNEL);
    if (NULL == kbuf) {
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    memset(kbuf, 0, (count+1));
    if (copy_from_user(kbuf, buf, count)) {
        kfree(kbuf);
        return -1;
    }

    /* coverity[tainted_data] */
    ret = (ssize_t)bsp_nvm_read(kbuf->nvid, kbuf->data, kbuf->len);
    if (ret) {
        kfree(kbuf);
        return ret;
    }

    ret = (ssize_t)copy_to_user(buf, kbuf, count);
    if (ret) {
        kfree(kbuf);
        return ret;
    }

    kfree(kbuf);
    return (ssize_t)count;
#endif
    return 0;
}
/*lint -restore*/


unsigned int nv_buffer_update(unsigned int ulOffset,void *nv_buffer, void *pdata, unsigned int ulLength)
{
    unsigned char *nv_offset = (unsigned char *)nv_buffer;
    if(nv_buffer==NULL|| pdata==NULL)
        return 1;
    nv_offset += ulOffset;
    if(nv_offset==NULL){
        return 1;
    }else{
        memcpy(nv_offset,(unsigned char *)pdata,ulLength);
        return 0;
    }
}

unsigned int nv_read_from_buffer(unsigned int ulOffset,void *nv_buffer, void *pdata, unsigned int ulLength)
{
    unsigned char *nv_offset = (unsigned char *)nv_buffer;
    if(nv_buffer==NULL|| pdata==NULL)
        return 1;
    nv_offset += ulOffset;
    if(nv_offset==NULL){
        return 1;
    }else{
        memcpy(pdata,nv_offset,ulLength);
        return 0;
    }
}

void setBitValue(unsigned char *nv_value_byte,int pos,unsigned char* nValue)
{
    if(*nValue == 0x1)
    {
        nv_value_byte[pos/8] |= 1UL<<(pos%8);
    }else if (*nValue  ==  0x0){
        nv_value_byte[pos/8] &= ~(1UL<<(pos%8));
    }else{
        hwlog_err("no changed\n");
        return ;
    }

}

static int madapt_parse_and_writeatonv(char *ptr, int len)
{
    char *nv_buffer = NULL;
    unsigned int ret = 0;
    struct madapt_item_ato_hdr_type nv_header;
    unsigned int nv_item_size = 0;
    unsigned int nv_item_length = 0;
    unsigned int lastNV = 0xFFFFFFFF;
    unsigned int currentNV=0xFFFFFFFF;
    unsigned char nv_bitType_value[MADAPT_BIT_LENGTH_SIZE+1];
    memset(&nv_header, 0, sizeof(struct madapt_item_ato_hdr_type));
    memset(nv_bitType_value, '\0', sizeof(nv_bitType_value));
    if(NULL == ptr){
        return BSP_ERR_MADAPT_NV_HDR_CHECK_ERR;
    }
    //NV最大不超过2048字节
    nv_buffer = (char *)vmalloc(MADAPT_MAX_NV_LENGTH_SIZE*sizeof(char));
    if (NULL == nv_buffer) {
        hwlog_err("madapt_parse_and_writeatonv_new, vmalloc error\n");
        return BSP_ERR_MADAPT_MALLOC_FAIL;
    }
    do {
        nv_header.feature_index
            = ((struct madapt_item_ato_hdr_type *)ptr)->feature_index;
        //文件结束标志位
        if (MADAPT_FILE_END_FLAG == nv_header.feature_index) {
            //调用kernel的函数写入NV
            if(lastNV!=0xFFFFFFFF){
                ret = mdrv_nv_writeex(
                    MADAPT_MODEMID_CONVERT(nv_header.nv_modem_id),
                                lastNV,
                                nv_buffer,
                                nv_item_length);
                //check
                if(0!=ret){
                    vfree(nv_buffer);
                    return BSP_ERR_MADAPT_WRITE_NV_ERR;
                }
            }
            hwlog_err("madapt_parse_and_writeatonv, find nv file end flag.\n");
            break;
        }
        //得到要修改的nv相关信息
        nv_header.nv_item_number
                    = ((struct madapt_item_ato_hdr_type *)ptr)->nv_item_number;
        nv_header.nv_modem_id
            = ((struct madapt_item_ato_hdr_type *)ptr)->nv_modem_id;
        nv_header.nv_item_byte_start
            = ((struct madapt_item_ato_hdr_type *)ptr)->nv_item_byte_start;
        nv_header.nv_item_byte_length
            = ((struct madapt_item_ato_hdr_type *)ptr)->nv_item_byte_length;
        nv_header.nv_ato_bit_pos
            = ((struct madapt_item_ato_hdr_type *)ptr)->nv_ato_bit_pos;

        ptr += sizeof(struct madapt_item_ato_hdr_type);
        nv_item_size = nv_header.nv_item_byte_length;

        // modem id: 1<=id<=3
        if ( (nv_header.nv_modem_id < MADAPT_MIN_NV_MODEM_ID)
         || (nv_header.nv_modem_id > MADAPT_MAX_NV_MODEM_ID)
         || (0 == nv_item_size)
         ||(nv_item_size >= MADAPT_MAX_NV_LENGTH_SIZE) ) {
             hwlog_err("madapt_parse_and_writeatonv, nv header: nv id: [%d], nv modemid: [%d], nv size: [%d].\n",
                    nv_header.nv_item_number,
                    nv_header.nv_modem_id,
                    nv_item_size);
             vfree(nv_buffer);
             return BSP_ERR_MADAPT_NV_HDR_CHECK_ERR;
        }

        //最后一个NV特性的长度和文件剩余长度不符
        if(len<(sizeof(struct madapt_item_ato_hdr_type)+nv_item_size)){
            hwlog_err("Maybe file is broken\n");
            vfree(nv_buffer);
            return BSP_ERR_MADAPT_NV_HDR_CHECK_ERR;
        }
        currentNV = nv_header.nv_item_number;
        if(currentNV!=lastNV)
        {
           // hwlog_err("lastNV = %x,nv_item_length=%d\n",lastNV,nv_item_length);
            if(lastNV!=0xFFFFFFFF){
                //调用kernel的函数写入NV
                ret = mdrv_nv_writeex(
                    MADAPT_MODEMID_CONVERT(nv_header.nv_modem_id),
                                lastNV,
                                nv_buffer,
                                nv_item_length);
                //check
                if(0!=ret){
                    vfree(nv_buffer);
                    return BSP_ERR_MADAPT_WRITE_NV_ERR;
                }
            }
            memset(nv_buffer,'\0',MADAPT_MAX_NV_LENGTH_SIZE);
            ret = mdrv_nv_get_length(nv_header.nv_item_number,&nv_item_length);
	    /*AR000AD08A sunjun/00290209 20180807 begin*/
	    //是否存在这个NV，如果不存在就continue
	    if (BSP_ERR_MADAPT_NO_THIS_NV == ret){
	        hwlog_err("read curNV = %x,not exist, goto next\n",nv_header.nv_item_number);
		lastNV = 0xFFFFFFFF;
		goto NEXT_NV_PTR;
	    }
	    /*AR000AD08A sunjun/00290209 20180807 end*/
            //起始位加上修改字节的长度要小于该NV的总长度
            if(nv_header.nv_item_byte_start<MADAPT_MAX_NV_LENGTH_SIZE
                    &&(nv_header.nv_item_byte_start+nv_item_size)<=nv_item_length
                    && nv_item_length<MADAPT_MAX_NV_LENGTH_SIZE
                    && ret==0){
                //调用kernel的函数读取NV
                ret = mdrv_nv_readex(
                    MADAPT_MODEMID_CONVERT(nv_header.nv_modem_id),
                                nv_header.nv_item_number,
                                nv_buffer,
                                nv_item_length);
                nv_buffer[nv_item_length]='\0';
                lastNV = currentNV;
                //check
                if(0!=ret){
                    vfree(nv_buffer);
                    return BSP_ERR_MADAPT_WRITE_NV_ERR;
                }
            }else{
                hwlog_err("madapt_parse_and_writeatonv, nv[%d] with modemid[%d] and size[%d] write fail, ret=%d.\n",
                    nv_header.nv_item_number,
                    nv_header.nv_modem_id,
                    nv_item_length,
                    ret);
                vfree(nv_buffer);
                return BSP_ERR_MADAPT_READ_NV_ERR;
            }

        }
        if(MADAPT_FILE_NOTBIT_FLAG == nv_header.nv_ato_bit_pos){
            //hwlog_err("currentNV = %d,offest =%d\n",currentNV,nv_header.nv_item_byte_start);
            ret = nv_buffer_update(
                       nv_header.nv_item_byte_start,
                       nv_buffer,
                       ptr,
                       nv_item_size);
        }else{
            //hwlog_err("currentNV = %d,offest =%d\n",currentNV,nv_header.nv_item_byte_start);
            ret = nv_read_from_buffer(
                      nv_header.nv_item_byte_start,
                      nv_buffer,
                      nv_bitType_value,
                      nv_item_size);
            setBitValue(nv_bitType_value,nv_header.nv_ato_bit_pos,(unsigned char *)ptr);
            ret = nv_buffer_update(
                      nv_header.nv_item_byte_start,
                      nv_buffer,
                      nv_bitType_value,
                      nv_item_size);

        }
        if (0 != ret) {
            hwlog_err("madapt_parse_and_writeatonv, nv_buffer_update error,ret=%d.\n",ret);
            vfree(nv_buffer);
            return BSP_ERR_MADAPT_WRITE_NV_ERR;
        }
	/*AR000AD08A sunjun/00290209 20180807 begin*/
	NEXT_NV_PTR:
	/*AR000AD08A sunjun/00290209 20180807 end*/
        len -= (sizeof(struct madapt_item_ato_hdr_type)
            + nv_item_size);
        ptr += nv_item_size;
    } while (len > 0);
    vfree(nv_buffer);
    return BSP_ERR_MADAPT_OK;
}

static int madapt_parse_and_writenv(char *ptr, int len, unsigned int modem_id)
{
    unsigned int ret = 0;
    struct madapt_item_hdr_type nv_header;

    memset(&nv_header, 0, sizeof(struct madapt_item_hdr_type));
    do {
        nv_header.nv_item_number
            = ((struct madapt_item_hdr_type *)ptr)->nv_item_number;
        if (MADAPT_FILE_END_FLAG == nv_header.nv_item_number) {
            hwlog_err("madapt_parse_and_writenv, find nv file end flag.\n");
            break;
        }
        nv_header.nv_modem_id = modem_id;
        nv_header.nv_item_size
            = ((struct madapt_item_hdr_type *)ptr)->nv_item_size;
        ptr += sizeof(struct madapt_item_hdr_type);

        if ((nv_header.nv_modem_id < MADAPT_MIN_NV_MODEM_ID)
         || (nv_header.nv_modem_id > MADAPT_MAX_NV_MODEM_ID)
         || (0 == nv_header.nv_item_size)
         || (nv_header.nv_item_size >= MADAPT_FILE_MAX_SIZE)   /* nv size must less than file Max size */
         || ((sizeof(struct madapt_item_hdr_type) + nv_header.nv_item_size) > len) /* nv data size + head size must less than file length */
         ) {
             hwlog_err("madapt_parse_and_writenv, invalid nv header: nv id: [%d], nv modemid: [%d], nv size: [%d].\n",
                    nv_header.nv_item_number,
                    nv_header.nv_modem_id,
                    nv_header.nv_item_size);
             return BSP_ERR_MADAPT_NV_HDR_CHECK_ERR;
        }

        ret = mdrv_nv_writeex(
            MADAPT_MODEMID_CONVERT(nv_header.nv_modem_id),
                        nv_header.nv_item_number,
                        ptr,
                        nv_header.nv_item_size);
        if (0 != ret) {
	    /*AR000AD08A sunjun/00290209 20180807 begin*/
	    if (BSP_ERR_MADAPT_NO_THIS_NV == ret){
	        hwlog_err("write curNV = %x,not exist, goto next\n",nv_header.nv_item_number);
	    }
	    else{
	        /*AR000AD08A sunjun/00290209 20180807 end*/
                hwlog_err("madapt_parse_and_writenv, nv[%d] with modemid[%d] and size[%d] write fail, ret=%d.\n",
                    nv_header.nv_item_number,
                    nv_header.nv_modem_id,
                    nv_header.nv_item_size,
                    ret);
                return BSP_ERR_MADAPT_WRITE_NV_ERR;
            }
	}
        ptr += nv_header.nv_item_size;
        len -= (sizeof(struct madapt_item_hdr_type)
            + nv_header.nv_item_size);
    } while (len > 0);

    return BSP_ERR_MADAPT_OK;
}

static int madapt_remove_mbn_carrirer_file(void)
{
    mm_segment_t old_fs = {0};
    int ret = BSP_ERR_MADAPT_OK;

    old_fs = get_fs();
    set_fs((unsigned long)KERNEL_DS);
    ret = sys_unlink(NCFG_BIN_FILE_PATH_IN_MODEM);
    if( ret < 0)
    {
        hwlog_err("remove carrier bin file failed. ret=%d", ret);
        ret= BSP_ERR_MADAPT_REMOVE_FILE_ERR;
    }
    set_fs(old_fs);

    return ret;
}
static int madapt_write_mbn_carrirer_file(char *file_buffer, size_t buffer_Size)
{
    struct file *fp = NULL;
    mm_segment_t old_fs = {0};
    loff_t pos = 0;

    fp = filp_open(NCFG_BIN_FILE_PATH_IN_MODEM, O_RDWR|O_CREAT,0644);
    if (IS_ERR(fp)) {
        hwlog_err("madapt_write_mbn_carrirer_file, create file error! \n");
        return BSP_ERR_MADAPT_OPEN_FILE_ERR;
    }

    old_fs = get_fs();
    set_fs((unsigned long)KERNEL_DS);

    pos =0;
    vfs_write(fp, file_buffer, buffer_Size, &pos);

    filp_close(fp,NULL);
    set_fs(old_fs);
    hwlog_err("madapt_write_mbn_carrirer_file, copy file success! \n");

    return BSP_ERR_MADAPT_OK;
}

static int parse_write_file(struct madapt_file_stru *file)
{
    struct file *fp = NULL;
    mm_segment_t fs = {0};
    loff_t pos = 0;
    int ret_size = 0;
    int size = 0;
    int ret = BSP_ERR_MADAPT_OK;
    char *k_buffer = NULL;

    if (NULL == file) {
        hwlog_err("parse_write_file, NULL ptr!\n");
        return BSP_ERR_MADAPT_PTR_NULL;
    }

    if (file->len > MADAPT_MAX_USER_BUFF_LEN) {
        hwlog_err("parse_write_file, file->len(%d) too large!\n",
                    file->len);
        return BSP_ERR_MADAPT_PARAM_ERR;
    }

    if (MADAPT_MODEMID_INVALID == file->modem_id )
    {
        ret = madapt_remove_mbn_carrirer_file();
    }

    if (file->file) {
        if (file->file == strstr(file->file, "/odm") ||
            file->file == strstr(file->file, "/hw_odm") ||
            file->file == strstr(file->file, "/data/cota")) {
            hwlog_err("%s, file path valid\n", __func__);
        } else {
            hwlog_err("%s, file path invalid, return err\n", __func__);
            return BSP_ERR_MADAPT_PARAM_ERR;
        }
    } else {
        hwlog_err("%s, file->file is null, return err\n", __func__);
        return BSP_ERR_MADAPT_PARAM_ERR;
    }

    fp = filp_open(file->file, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        hwlog_err("parse_write_file, open file error!\n");
        return BSP_ERR_MADAPT_OPEN_FILE_ERR;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    size = file_inode(fp)->i_size;
    if (size <= 0 || size >= MADAPT_FILE_MAX_SIZE) {
        hwlog_err("parse_write_file, file size(%d) error!\n", size);
        ret = BSP_ERR_MADAPT_FILE_SIZE_ERR;
        goto FILE_PROC_OUT;
    } else {
        hwlog_err("parse_write_file, get nvbin file size(%d)!\n", size);
    }

    pos = 0;
    k_buffer = kmalloc((size), GFP_KERNEL);
    if (NULL == k_buffer) {
        hwlog_err("parse_write_file, kmalloc error\n");
        ret = BSP_ERR_MADAPT_MALLOC_FAIL;
        goto FILE_PROC_OUT;
    }

    memset(k_buffer, 0, size);
    ret_size = vfs_read(fp, k_buffer, size, &pos);
    if (size != ret_size) {
        hwlog_err("parse_write_file, error vfs_read ret: %d, readsize: %d\n",
            ret_size, (int)(size));
        ret = BSP_ERR_MADAPT_READ_FILE_ERR;
        goto MEM_PROC_OUT;
    }

    if (MADAPT_MODEMID_INVALID == file->modem_id )
    {
        /*speical for copy NV BIN file from odm to mnvm2:0 */
        ret = madapt_write_mbn_carrirer_file(k_buffer, size);
    }
    else
    {
        if(NULL != strstr(file->file,".ato")){
            //decode nv ato file
            ret = madapt_parse_and_writeatonv(k_buffer, size);
        }else{
            ret = madapt_parse_and_writenv(k_buffer, size, file->modem_id);
        }
    }

MEM_PROC_OUT:
    kfree(k_buffer);
    k_buffer = NULL;
FILE_PROC_OUT:
    filp_close(fp, NULL);
    set_fs(fs);
    return ret;
}

static void do_proc_nv_file(struct work_struct *p_work)
{
    work_ret = parse_write_file(kbuf);
    if (BSP_ERR_MADAPT_OK != work_ret) {
        hwlog_err("do_proc_nv_file, parse_write_file fail with errcode(%d)!\n",
                    (int)work_ret);
    } else {
        hwlog_err("do_proc_nv_file, proc all success\n");
    }
    kfree(kbuf);
    kbuf = NULL;
    complete(&(my_completion));
}

/*lint -save --e{529,527}*/
ssize_t madapt_dev_write(struct file *file,
                        const char __user *buf,
                        size_t count, loff_t *ppos) {
    ssize_t ret = BSP_ERR_MADAPT_OK;
    int ticks_left = 0;

    if (NULL == buf) {
        return BSP_ERR_MADAPT_PARAM_ERR;
    }

    /*hwlog_err("madapt_dev_write, invalid parameter count(%d)!\n",
                (int)count);*/

    if ((count <= sizeof(unsigned int))
        || (count >= sizeof(struct madapt_file_stru))) {
        return BSP_ERR_MADAPT_PARAM_ERR;
    }

    mutex_lock(&madapt_mutex);
    kbuf = kmalloc(sizeof(struct madapt_file_stru), GFP_KERNEL);
    if (NULL == kbuf) {
        mutex_unlock(&madapt_mutex);
        return BSP_ERR_MADAPT_MALLOC_FAIL;
    }

    memset(kbuf, 0, sizeof(struct madapt_file_stru));
    if (copy_from_user(kbuf, buf, count)) {
        ret = BSP_ERR_MADAPT_COPY_FROM_USER_ERR;
        kfree(kbuf);
        kbuf = NULL;
        goto OUT;
    }

    /* schedule work */
    schedule_work(&(test_work.proc_nv));

    ticks_left = wait_for_completion_timeout(&(my_completion), HZ*30);
    if (0 == ticks_left) {
        ret = BSP_ERR_MADAPT_TIMEOUT;
    } else if (BSP_ERR_MADAPT_OK == work_ret) {
        ret = work_ret;
    } else {
        ret = work_ret;
    }

OUT:
    mutex_unlock(&madapt_mutex);
    return ret;
}


/*lint -save --e{529}*/
int madapt_init(void)
{
    int ret = 0;
    dev_t dev = 0;
    unsigned int devno;

    /*dynamic dev num use*/
    ret = alloc_chrdev_region(&dev, 0, 1, MADAPT_DEVICE_NAME);
    madapt_major = MAJOR(dev);

    if (ret) {
        hwlog_err("madapt_init, madapt failed alloc :%d\n",
            madapt_major);
        return ret;
    }

    devno = MKDEV(madapt_major, 0);

    /*setup dev*/
    memset(&cdev, 0, sizeof(struct cdev));

    cdev_init(&cdev, &nv_fops);
    cdev.owner = THIS_MODULE;
    cdev.ops = &nv_fops;

    ret = cdev_add(&cdev, devno, 1);
    if (ret) {
        hwlog_err("madapt_init, cdev_add fail!\n");
        return ret;
    }

    madapt_class = class_create(THIS_MODULE, MADAPT_DEVICE_CLASS);
    if (IS_ERR(madapt_class)) {
        hwlog_err("madapt_init, class create failed!\n");
        return ret;
    }

    device_create(madapt_class, NULL, devno, NULL, MADAPT_DEVICE_NAME);
    /* madapt_mutex init */
    mutex_init(&madapt_mutex);

    INIT_WORK(&(test_work.proc_nv), do_proc_nv_file);
    init_completion(&(my_completion));

    hwlog_err("madapt_init, complete!\n");
    return BSP_ERR_MADAPT_OK;
}
/*lint -restore*/


void madapt_exit(void)
{
    cdev_del(&cdev);
    class_destroy(madapt_class);
    unregister_chrdev_region(MKDEV(madapt_major, 0), 1);
}

module_init(madapt_init);
module_exit(madapt_exit);
MODULE_AUTHOR("Hisilicon Drive Group");
MODULE_DESCRIPTION("madapt driver for Hisilicon");
MODULE_LICENSE("GPL");

/*lint -restore*/
