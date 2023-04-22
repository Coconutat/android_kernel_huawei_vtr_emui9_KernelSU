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
#include "icc_core.h"

static u32 dynamic_channel_id = ICC_STATIC_CHN_ID_MAX;
static u32 count = 0;
struct icc_control g_icc_ctrl = {0};
struct icc_dynamic_info dynamic_info[ICC_DYNAMIC_CHAN_NUM_MAX];

u32 fifo_put_with_header(struct icc_channel_fifo *fifo, u8 *head_buf, u32 head_len, u8 *data_buf, u32 data_len)
{
	s32 tail_idle_size = 0;
	u32 write           = fifo->write;
	u32 read            = fifo->read;
	char* base_addr       = (char*)((char*)fifo + sizeof(struct icc_channel_fifo)); 
	u32 buf_len         = fifo->size;

	/*空闲缓冲区大小*/
	if (read > write)
	{
		tail_idle_size = (s32)(read - write);
	}
	else
	{
		tail_idle_size = (s32)(buf_len - write);
	}

	/*先填充头部*/
	if (tail_idle_size < (s32)head_len)
	{
		(void)memcpy_s((void *)(write + base_addr), buf_len, (void *)head_buf, (u32)tail_idle_size);/*lint !e124 */
		(void)memcpy_s((void *)base_addr, buf_len, (void *)(head_buf + tail_idle_size), (head_len - (u32)tail_idle_size));/*lint !e124 */
		write          = head_len - (u32)tail_idle_size;
		tail_idle_size = 0;
	}
	else
	{
		(void)memcpy_s((void *)(write + base_addr), buf_len, (void *)head_buf, head_len);/*lint !e124 */
		tail_idle_size = tail_idle_size - (s32)head_len;
		write          = (tail_idle_size == 0) ? 0 : (write + head_len);
	}

	/*再填充负载*/
	if ( (0 == tail_idle_size) || (tail_idle_size > (s32)data_len) )
	{
		(void)memcpy_s((void *)(write + base_addr), buf_len, (void *)data_buf, data_len);/*lint !e124 */
		write += data_len;
	}
	else
	{
		(void)memcpy_s((void *)(write + base_addr), buf_len, (void *)data_buf, (u32)tail_idle_size);/*lint !e124 */
		(void)memcpy_s((void *)base_addr, buf_len, (void *)(data_buf + tail_idle_size), data_len - (u32)tail_idle_size);/*lint !e124 */
		write = data_len - (u32)tail_idle_size;
	}

	/*确保最后写完不停在缓冲区结束位置*/
	write = (write == buf_len) ? 0 : write;

	/*更新写指针*/
	mb();
	fifo->write = write;

	return data_len + head_len;
}

u32 fifo_get(struct icc_channel_fifo *fifo, u8 *data_buf, u32 data_len, u32 *read)
{
	s32 total_idle_size = 0;
	s32 tail_idle_size  = 0;/*lint !e14 */
	u32 write           = fifo->write;
	char* base_addr      = (char*)fifo + sizeof(struct icc_channel_fifo);
	u32 buf_len         = fifo->size;
	u32 readed_len      = 0;

	/*空闲缓冲区大小*/
	if (*read > write)
	{
		total_idle_size = (s32)(buf_len + write - *read);
		tail_idle_size  = (s32)(buf_len - *read);
	}
	else
	{
		total_idle_size = (s32)(write - *read);
		tail_idle_size  = total_idle_size;
	}

	readed_len = (u32)(ICC_MIN((s32)data_len, total_idle_size));
	if(readed_len > data_len)
	{
		(*(unsigned long*) (ICC_DBG_MSG_ADDR_IN_DDR )) = (unsigned long)(base_addr);
		(*(unsigned long*) (ICC_DBG_MSG_ADDR_IN_DDR + sizeof(base_addr) )) = (unsigned long)(write);
		(*(unsigned long*) (ICC_DBG_MSG_ADDR_IN_DDR + sizeof(base_addr)+4)) = (unsigned long)(*read);
		(*(unsigned long*) (ICC_DBG_MSG_ADDR_IN_DDR + sizeof(base_addr)+8)) = (unsigned long)(buf_len);
		(*(unsigned long*) (ICC_DBG_MSG_ADDR_IN_DDR + sizeof(base_addr)+12))= (unsigned long)(data_len);
		icc_system_error((u32)DRV_ERRNO_ICC_RING_BUFFER, 0, 0, (char *)ICC_DBG_MSG_ADDR_IN_DDR, (u32)ICC_DBG_MSG_LEN_IN_DDR);
	}

	if ((s32)readed_len <= tail_idle_size)
	{
		(void)memcpy_s((void *)data_buf, data_len, (void *)(*read + base_addr), readed_len);/*lint !e124 */
		*read += readed_len;
	}
	else
	{
		(void)memcpy_s((void *)data_buf, data_len, (void *)(*read + base_addr), (u32)tail_idle_size);/* [false alarm]:屏蔽Fortify误报 *//*lint !e124 */
		(void)memcpy_s((void *)(data_buf + tail_idle_size), data_len, (void *)base_addr, readed_len - (u32)tail_idle_size);/*lint !e124 */
		*read = readed_len - (unsigned int)tail_idle_size;
	}

	/*确保最后读完不停在缓冲区结束位置*/
	*read = (*read >= buf_len) ? (*read - buf_len) : (*read);

	return readed_len;

}


/*数据包完整性需要保证 */
u32 fifo_get_with_header(struct icc_channel_fifo *fifo, u8 *data_buf, u32 data_buf_len)
{
	u32 read_len = 0;
	u32 read     = fifo->read;
	struct icc_channel_packet packet = {0};

	read_len = fifo_get(fifo, (u8 *)&packet, sizeof(packet), &read);
	if(read_len != sizeof(packet))/*读包头错误*/
	{
		icc_print_error("get packet err, read_len:0x%x, packet_size: 0x%x \n", read_len, sizeof(packet));
		(void)icc_channel_packet_dump(&packet);
		return (u32)ICC_ERR;
	}
	if(data_buf_len < packet.len) /*传入len比实际包的长度小*/
	{
		icc_print_error("invalid data_len:0x%x, packet.len: 0x%x \n", data_buf_len, packet.len);
		return (u32)ICC_ERR;
	}

	read_len = fifo_get(fifo, data_buf, packet.len, &read);
	if(read_len != packet.len)   /*读数据错误*/
	{
		icc_print_error("get data err, read_len:0x%x, packet.len: 0x%x \n", read_len, packet.len);
		return (u32)ICC_ERR;
	}

	/*更新读指针*/
	rmb();
	fifo->read = read;

	return read_len;
}


/* fifo中还有多少空闲空间 */
static u32 fifo_write_space_get(struct icc_channel_fifo* fifo)
{
    u32 write   = fifo->write;
    u32 read    = fifo->read;
    u32 buf_len = fifo->size;

    /*空闲缓冲区大小*/
    if (read > write)
    {
        return (read - write);
    }
    else
    {
        return (buf_len - write + read);
    }
}


/* fifo中还有多少数据未读取 */
static u32 fifo_read_space_get(struct icc_channel_fifo* fifo)
{
    u32 write   = fifo->write;
    u32 read    = fifo->read;
    u32 buf_len = fifo->size;

    if (read > write)
    {
		return (buf_len + write - read);
    }
    else
    {
		return (write - read);
    }
}


static u32 fifo_skip(struct icc_channel_fifo* fifo,  u32 len)
{
	u32 space    = fifo_read_space_get(fifo);
	u32 data_len = ICC_MIN(len, space);
	u32 read     = fifo->read;
	read        += data_len;
	read         = (read >= fifo->size) ? (read - fifo->size) : (read);
	fifo->read   =read;

	return data_len;
}

static s32 data_send(u32 cpuid, u32 channel_id, u8* data, u32 data_len)
{
	s32 ret = ICC_OK;
	unsigned long flags = 0;
	u32 len = 0;
	struct icc_channel *channel = g_icc_ctrl.channels[GET_CHN_ID(channel_id)];
	struct icc_channel_packet packet = {0};

	packet.channel_id = channel_id;
	packet.src_cpu_id = ICC_THIS_CPU;
	packet.seq_num = 0;
	packet.is_responsed= 0;
	packet.need_responsed = 0;
	packet.len = data_len;
	packet.data = 0;

	UNUSED(flags);

	spin_lock_irqsave(&channel->write_lock, flags); /*lint !e123 */

	icc_debug_before_send(&packet);  /* 记录debug信息，将发送任务ID及时间戳放入包头中 */
	
	if((data_len + sizeof(struct icc_channel_packet)) >= fifo_write_space_get(channel->fifo_send))/*lint !e574 */
	{
		ret = ICC_INVALID_NO_FIFO_SPACE;
		goto err_fifo_full; /*lint !e801 */
	}

    /* 将包头及负载放入fifo */
	len = fifo_put_with_header(channel->fifo_send, (u8*)&packet, sizeof(struct icc_channel_packet), data, data_len);
	len -=  sizeof(struct icc_channel_packet);
	if(data_len != len)
	{
		ret = ICC_SEND_ERR;
		goto err_send; /*lint !e801 */
	}

	ret = bsp_ipc_int_send((IPC_INT_CORE_E)cpuid, (IPC_INT_LEV_E)channel->ipc_send_irq_id);
	if(ret != 0)
	{
		icc_print_error("ipc send fail,ret:0x%x \n", ret);
		goto err_send; /*lint !e801 */
	}
	
	icc_debug_after_send(channel, &packet, data);

	spin_unlock_irqrestore(&channel->write_lock, flags); /*lint !e123 */

	return (s32)len;
	
err_send:	
	spin_unlock_irqrestore(&channel->write_lock, flags); /*lint !e123 */
	return ret;
	
err_fifo_full:
	spin_unlock_irqrestore(&channel->write_lock, flags); /*lint !e123 */
	return ret;
}

void handle_channel_recv_data(struct icc_channel *channel)
{
	struct icc_channel_packet packet = {0};
	struct icc_channel_vector *vector = NULL;
	u32 read_len = 0;
	u32 read     = 0;
	unsigned long flags = 0;
	
	UNUSED(flags);

	spin_lock_irqsave(&(channel->read_lock), flags); /*lint !e123 */
	read     = channel->fifo_recv->read;
	read_len = fifo_get(channel->fifo_recv, (u8*)&packet, sizeof(packet), &read);

	if(read_len != sizeof(packet)) 
	{
		icc_print_error("notice: %s[%d], read=0x%x, write=0x%x, read_len=%d, packet_size=%d\n", 
            channel->name, channel->id, channel->fifo_recv->read, channel->fifo_recv->write, read_len, sizeof(packet));
		spin_unlock_irqrestore(&(channel->read_lock), flags); /*lint !e123 */
		(void)icc_channel_packet_dump(&packet);
		return;
	}

	if (GET_CHN_ID(packet.channel_id) >= ICC_CHN_ID_MAX)
	{
		icc_print_error("invalid packet, unrecovery\n");
		icc_packet_print(&packet);
		spin_unlock_irqrestore(&(channel->read_lock), flags); /*lint !e123 */
		icc_system_error((u32)DRV_ERRNO_ICC_RING_BUFFER, 0, 0, NULL, 0);
		return;
	}

	if(GET_FUNC_ID(packet.channel_id) >= channel->func_size)
	{
		icc_print_error("skip packet!channel_id:0x%x,task_id:0x%x\n", packet.channel_id, packet.task_id);
		/* 无效包跳过 */
		(void)fifo_skip(channel->fifo_recv, packet.len + read_len);
		spin_unlock_irqrestore(&(channel->read_lock), flags); /*lint !e123 */
		return;
	}

	vector = &channel->vector[GET_FUNC_ID(packet.channel_id)];

	if(!packet.len) /* 空包，跳过packet */
	{
		channel->fifo_recv->read = read;
	}

	spin_unlock_irqrestore(&(channel->read_lock), flags); /*lint !e123 */

	if (vector->read_cb)
	{
		icc_print_debug("cb func:%p id:0x%x\n", vector->read_cb, packet.channel_id);
		icc_debug_before_recv(&packet);
		(void)vector->read_cb(packet.channel_id, packet.len, vector->read_context);
		icc_debug_after_recv(&packet);
	}

	return;

}

void handle_channel_recv(struct icc_channel *channel)
{
	u32 read = 0;

	/* 防止快速掉电内存中内容不丢失，造成的对fifo的判断错误 */
	if(ICC_CHN_MAGIC_SIGN == channel->fifo_recv->magic)
	{
		channel->fifo_recv->magic = ICC_CHN_MAGIC_UNSIGN;
		channel->ready_recv = 1;
	}
	if (channel->ready_recv)
	{
		/* 把fifo中消息全部读走(需要等对方初始化后才能使用) */
		while(fifo_read_space_get(channel->fifo_recv) >= sizeof(struct icc_channel_packet))
		{
			read = channel->fifo_recv->read;
			handle_channel_recv_data(channel);
			if(channel->fifo_recv->read == read)
			{
				break;
			}
		}

		/*写回调默认使用子通道0 */
		if(fifo_write_space_get(channel->fifo_send) == channel->fifo_send->size && channel->vector->write_cb)
		{
			(void)channel->vector->write_cb(channel->id, NULL);
		}
	}
}

s32 icc_task_private_func(void *obj)
{
	struct icc_channel *channel = obj;

	/* coverity[INFINITE_LOOP] */
	/* coverity[no_escape] */
	for( ; ;)
	{
		handle_channel_recv(channel);
		osl_sem_down(&channel->private_task_sem);
	}
}

void icc_task_shared_func(void const *obj)

{
	struct icc_channel *channel = NULL;
	u32 i = 0;

	/* coverity[INFINITE_LOOP] */
	/* coverity[no_escape] */
	for( ; ;)
	{
		for(i = 0; i < ICC_CHN_ID_MAX; i++)
		{
			channel = g_icc_ctrl.channels[i];
			if(!channel)
			{
				continue;
			}
			
			if (channel->mode.union_stru.task_shared)
			{
				handle_channel_recv(channel);
			}
		}
		icc_wake_unlock(&g_icc_ctrl.wake_lock);
		osl_sem_down(&g_icc_ctrl.shared_task_sem);
	}
	return ; /*lint !e527 */
}

void icc_notask_sharedipc_func(void)
{
	struct icc_channel *channel = NULL;
	u32 i = 0;

	for(i = ICC_CHN_ACORE_CCORE_MIN; i < ICC_CHN_ID_MAX; i++)
	{
		channel = g_icc_ctrl.channels[i];
		if(!channel)
		{
			continue;
		}

		if((!channel->mode.union_stru.task_shared) && (!channel->mode.union_stru.no_task))
		{
			osl_sem_up(&(channel->private_task_sem)); /*lint !e661 */
		}
		else if(channel->mode.union_stru.no_task)
		{
			handle_channel_recv(channel);
		}
	}
}

void icc_ipc_isr(u32 data)
{
	struct icc_channel *channel = NULL;
	u32 channel_id = data;

	icc_debug_in_isr();

	if(channel_id == ICC_NOTASK_SHAREDIPC_IDX)
	{
		icc_notask_sharedipc_func();
	}
	else if(channel_id == ICC_SHAREDTASK_SHAREDIPC_IDX)
	{
		icc_wake_lock(&g_icc_ctrl.wake_lock);
		osl_sem_up(&g_icc_ctrl.shared_task_sem);
	}
	else
	{
		if(channel_id < ICC_CHN_ID_MAX)
			channel = g_icc_ctrl.channels[channel_id];
		else {
			icc_print_error("err,chan_id=0x%x\n",channel_id);
			return;
		}

		if((!channel->mode.union_stru.task_shared) && (!channel->mode.union_stru.no_task))
		{
			osl_sem_up(&(channel->private_task_sem)); /*lint !e661 */
		}
		else if(channel->mode.union_stru.no_task)
		{
			handle_channel_recv(channel);
		}
	}
}

/* 根据fifo头信息初始化通道 */
/*lint --e{578} */
struct icc_channel *icc_channel_init(struct icc_init_info *info, s32 *ret)
{
	struct icc_channel *channel = NULL;
	static int shared_init_flag = 0;

	*ret = ICC_OK;

	channel = (struct icc_channel*)osl_malloc(sizeof(struct icc_channel));
	if (!channel)
	{
		*ret = ICC_MALLOC_CHANNEL_FAIL;
		goto error_channel; /*lint !e801 */
	}
	(void)memset_s(channel, sizeof(struct icc_channel), 0, sizeof(struct icc_channel));

	channel->id       = info->real_channel_id; /* 直接使用real channel id */
	channel->name     = info->name;
	channel->mode.val = info->mode;

	/* 发送fifo本侧初始化，接收fifo对侧初始化 */
	channel->fifo_send = (struct icc_channel_fifo*)(info->send_addr);
	channel->fifo_recv = (struct icc_channel_fifo*)(info->recv_addr);
	icc_restore_recv_channel_flag(channel->fifo_recv);

	(void)memset_s(channel->fifo_send, sizeof(struct icc_channel_fifo), 0, sizeof(struct icc_channel_fifo));
	channel->fifo_send->size  = info->fifo_size;
	channel->fifo_send->magic = ICC_CHN_MAGIC_SIGN; /* 通知对方本核的该fifo是否初始化完成 */

	/* 接收向量初始化 */
	channel->func_size = info->func_size;
	channel->vector = (struct icc_channel_vector*)osl_malloc(sizeof(struct icc_channel_vector) * channel->func_size);
	if (!channel->vector)
	{
		*ret = ICC_MALLOC_VECTOR_FAIL;
		goto error_vector; /*lint !e801 */
	}
	(void)memset_s(channel->vector, sizeof(struct icc_channel_vector) * channel->func_size, 
		0, sizeof(struct icc_channel_vector) * channel->func_size); /*lint !e665 */

	channel->ipc_send_irq_id = info->ipc_send_irq_id;
	channel->ipc_recv_irq_id = info->ipc_recv_irq_id;

	spin_lock_init(&channel->write_lock); /*lint !e123 */
	spin_lock_init(&channel->read_lock);  /*lint !e123 */

	if((!channel->mode.union_stru.task_shared) && (!channel->mode.union_stru.no_task))
	{
		icc_private_sem_init(&channel->private_task_sem);

		/* coverity[overwrite_var] */
		if(ICC_ERR == osl_task_init((char *)channel->name, ICC_TASK_PRIVATE_PRI, ICC_TASK_STK_SIZE,
			(void *)icc_task_private_func, (void*)channel, &channel->private_task_id)) /*lint !e611 */
		{
			*ret = ICC_CREATE_TASK_FAIL; /* [false alarm]:fortify */
			goto error_task; /*lint !e801 */
		}
	}

	channel->state           = ICC_CHN_OPENED;
	channel->ready_recv      = 0;

	/* 私有IPC中断源 */
	if(!channel->mode.union_stru.ipc_shared)
	{
		if(ICC_ERR == bsp_ipc_int_connect((IPC_INT_LEV_E)channel->ipc_recv_irq_id, (voidfuncptr)icc_ipc_isr, channel->id))
		{
			*ret = ICC_REGISTER_INT_FAIL;
			goto error_int; /*lint !e801 */
		}
		if(ICC_ERR == bsp_ipc_int_enable((IPC_INT_LEV_E)channel->ipc_recv_irq_id))
		{
			*ret = ICC_REGISTER_INT_FAIL;
			goto error_int; /*lint !e801 */
		}
	}

	/* 无任务或私有任务共享IPC中断 */
	if((channel->mode.union_stru.ipc_shared)
		&& ((channel->mode.union_stru.no_task)||((!channel->mode.union_stru.task_shared) && (!channel->mode.union_stru.no_task)))
		&& (shared_init_flag == 0)) {
		if(ICC_ERR == bsp_ipc_int_connect((IPC_INT_LEV_E)channel->ipc_recv_irq_id, (voidfuncptr)icc_ipc_isr, ICC_NOTASK_SHAREDIPC_IDX))
		{
			*ret = ICC_REGISTER_INT_FAIL;
			goto error_task; /*lint !e801 */
		}
		if(ICC_ERR == bsp_ipc_int_enable((IPC_INT_LEV_E)channel->ipc_recv_irq_id))
		{
			*ret = ICC_REGISTER_INT_FAIL;
			goto error_task; /*lint !e801 */
		}
		shared_init_flag = 1;
	}

	return channel;

error_int:
	kthread_stop(channel->private_task_id);
error_task:
	icc_safe_free(channel->vector);
error_vector:
	icc_safe_free(channel);
error_channel:
	icc_print_error("chan init errno=0x%x,chan_id=0x%x\n",ret, info->real_channel_id);
	return NULL;
}

/* 获取通道的fifo头信息然后初始化 */
static int icc_channels_node_init(struct device_node *node)
{
	int ret = ICC_ERR;
	const char *ref_type = NULL;
	struct device_node temp_node = {0};  /* M核上要使用dts必须定义这个变量 */
	struct device_node *child_node = &temp_node;  
	unsigned long last_channel_addr = 0;
	unsigned long last_ch_fifo_info_size = 0; /* fifo头长度 */
	unsigned long last_ch_fifo_size = 0;	  /* fifo本身长度，用来存放发送的数据包头+数据体 */
	struct icc_init_info init_info = {0};
	struct icc_channel *channel = NULL;
	struct channel_cfg dts_cfg = {0};
	char *icc_mem_addr_max = 0;
	u32 skip = 0;
	u32 unused_channel = 0;

	if (of_property_read_string(node, "ref", &ref_type)) 
	{
		icc_print_error("no ref\n");
		return ICC_ERR;
	}

	if (0 == memcmp((void*)ref_type, (void*)"secs", 4))
	{
		last_channel_addr = (unsigned long)ICC_SDDR_S_START_ADDR_ON_THIS_CORE; /*lint !e413 !e778 */
		icc_mem_addr_max = (char *)ICC_SDDR_S_ADDR_MAX;
	}
	else if (0 == memcmp((void*)ref_type, (void*)"sddr", 4))
	{
		last_channel_addr = (unsigned long)ICC_SDDR_START_ADDR_ON_THIS_CORE;
		icc_mem_addr_max = ICC_SDDR_ADDR_MAX;
	}
	else if (0 == memcmp((void*)ref_type, (void*)"sram", 4))
	{
		last_channel_addr = (unsigned long)ICC_SRAM_START_ADDR_ON_THIS_CORE;
		icc_mem_addr_max = ICC_SRAM_ADDR_MAX;
	}
	else
	{
		icc_print_error("invalid ref %s\n", ref_type);
		return ICC_ERR;
	}

	/* 有效的内存类型 */
	for_each_available_child_of_node(node, child_node)
	{
		if (!(of_property_read_u32_array(child_node, "unused", &unused_channel, 1)))
		{
			UNUSED(unused_channel);
			continue;
		}


		ret = of_property_read_string(child_node, "ch_name", &init_info.name);
		ret |= of_property_read_u32_array(child_node, "cfg", (unsigned int*)(&dts_cfg), (unsigned int)(sizeof(struct channel_cfg) / sizeof(unsigned int)));
		if (ret)
		{
			icc_print_error("get args fail\n");
			goto out; /*lint !e801 */
		}

		if (of_property_read_u32_array(child_node, "skip", &skip, 1))
		{
			skip = 0;
		}
		else
		{
			skip = 1;
		}
		
		
		init_info.real_channel_id = dts_cfg.id;
		init_info.fifo_size = dts_cfg.size;
		init_info.mode = dts_cfg.mode;
		init_info.ipc_send_irq_id = dts_cfg.tx_ipc;
		init_info.ipc_recv_irq_id = dts_cfg.rx_ipc;
		init_info.func_size = dts_cfg.func_size;



		if (1 == dts_cfg.rx_fifo_first) /* 本核上，本通道，先是接收后是发送 */
		{
			init_info.recv_addr = (void*)(last_channel_addr + 2 * last_ch_fifo_info_size + 2 * last_ch_fifo_size);
			init_info.send_addr = (void*)(init_info.recv_addr + sizeof(struct icc_channel_fifo) + init_info.fifo_size);

			last_channel_addr = (unsigned long)init_info.recv_addr;
			last_ch_fifo_info_size = sizeof(struct icc_channel_fifo);
			last_ch_fifo_size = init_info.fifo_size;

			if ((char *)(init_info.send_addr + sizeof(struct icc_channel_fifo) + init_info.fifo_size) > icc_mem_addr_max) /*lint !e613 */
			{
				icc_print_error("addr too big\n");
				ret = ICC_INIT_ADDR_TOO_BIG;
				goto out; /*lint !e801 */
			}
		}
		else  /* 本核上，本通道，先是发送后是接收 */
		{
			init_info.send_addr = (char*)(last_channel_addr + 2 * last_ch_fifo_info_size + 2 * last_ch_fifo_size);
			init_info.recv_addr = (char*)(init_info.send_addr + sizeof(struct icc_channel_fifo) + init_info.fifo_size);

			last_channel_addr = (unsigned long)init_info.send_addr;
			last_ch_fifo_info_size = sizeof(struct icc_channel_fifo);
			last_ch_fifo_size = init_info.fifo_size;

			if ((char *)(init_info.recv_addr + sizeof(struct icc_channel_fifo) + init_info.fifo_size) > icc_mem_addr_max) /*lint !e613 */
			{
				icc_print_error("addr too big\n");
				ret = ICC_INIT_ADDR_TOO_BIG;
				goto out; /*lint !e801 */
			}
		}

		icc_print_notice("========== channel cfg start============\n");
		icc_print_notice("ch_name %s\n", init_info.name);
		icc_print_notice("id 0x%x size 0x%x mode 0x%x\n", init_info.real_channel_id, init_info.fifo_size, init_info.mode);
		icc_print_notice("tx_ipc %d rx_ipc %d\n", init_info.ipc_send_irq_id, init_info.ipc_recv_irq_id);
		icc_print_notice("func_size 0x%x\n", init_info.func_size);
		icc_print_notice("fifo_send addr 0x%x fifo_recv addr 0x%x\n", init_info.send_addr, init_info.recv_addr);
		icc_print_notice("skip %d\n", skip);
		icc_print_notice("========== channel cfg end============\n");

		if (1 == skip)  /* 只需要地址信息，此通道不初始化 */
		{
			continue ;
		}

		/* 初始化通道 */
		channel = icc_channel_init(&init_info, &ret);
		if(NULL == channel)
		{
			ret = ICC_CHN_INIT_FAIL;
			goto out; /*lint !e801 */
		}

		g_icc_ctrl.channels[init_info.real_channel_id] = channel;
	}
out:
	return ret;
}

/* 本核相关所有通道初始化 */
int icc_channels_init(void)
{
	int ret = 0;
	struct device_node *node = NULL;
	struct device_node temp_node = {0};
	struct device_node *child_node = &temp_node;

	node = of_find_compatible_node(NULL, NULL, ICC_CHANNEL_INIT_COMPITABLE);
	if (!node)  /* 找不到节点 */
	{
		icc_print_error("find node err\n");
		return ICC_ERR;
	}

	for_each_available_child_of_node(node, child_node)
	{
		ret = icc_channels_node_init(child_node);
		if (ret)
		{
			icc_print_error("chan setup err\n");
			return ICC_ERR;
		}
	}
	
	return ICC_OK;
}

s32 icc_channel_empty(u32 channel_id)
{
	unsigned long flags = 0;
	u32 real_channel_id = GET_CHN_ID(channel_id);

	UNUSED(flags);
	if(real_channel_id >= ICC_CHN_ID_MAX)
	{
		return ICC_INVALID_PARA;
	}

	spin_lock_irqsave(&(g_icc_ctrl.channels[real_channel_id]->read_lock), flags);
	/*fifo中消息是否为空*/
	if(fifo_read_space_get(g_icc_ctrl.channels[real_channel_id]->fifo_recv) >= sizeof(struct icc_channel_packet))
	{
		spin_unlock_irqrestore(&(g_icc_ctrl.channels[real_channel_id]->read_lock), flags);
		return ICC_ERR;
	}
	spin_unlock_irqrestore(&(g_icc_ctrl.channels[real_channel_id]->read_lock), flags);

	return ICC_OK;
}

s32 bsp_icc_init(void)
{

	s32 ret = ICC_OK;
	u32 i = 0;

	(void)memset_s(&g_icc_ctrl, sizeof(struct icc_control), 0, sizeof(struct icc_control));
	(void)memset_s(&dynamic_info, (u32)ICC_DYNAMIC_CHAN_NUM_MAX * sizeof(struct icc_dynamic_info), 0, (u32)ICC_DYNAMIC_CHAN_NUM_MAX * sizeof(struct icc_dynamic_info));
	g_icc_ctrl.cpu_id = ICC_THIS_CPU;

	/* 不用通道指针置空，指针数组全部 */

	(void)memset_s(g_icc_ctrl.channels, ICC_CHN_ID_MAX* sizeof(struct icc_channel *), 0, ICC_CHN_ID_MAX* sizeof(struct icc_channel *)); /*lint !e665 */ /* [false alarm]:fortify */
	g_icc_ctrl.channel_size = ICC_CHN_ID_MAX;
	
	ret = icc_channels_init();/*lint !e838 */
	if (ret)
	{
	    icc_print_error("chan fifo init err\n");
		goto icc_channels_init_err; /*lint !e801 */
	}

	icc_wake_lock_init(&g_icc_ctrl.wake_lock, WAKE_LOCK_SUSPEND, "icc_wake");

	ret = icc_debug_init(ICC_STATIC_CHN_ID_MAX);
	if(ICC_OK != ret)
	{
		goto icc_channels_init_err; /*lint !e801 */
	}

	(void)icc_pm_init();

	icc_shared_sem_init();

	if(ICC_ERR == icc_shared_task_init())
	{
		ret = ICC_CREATE_TASK_FAIL;  /* [false alarm]:fortify */
		goto error_task; /*lint !e801 */
	}

	g_icc_ctrl.shared_recv_ipc_irq_id = ICC_RECV_IPC_SHARED;

	if (ICC_ERR == bsp_ipc_int_connect((IPC_INT_LEV_E)g_icc_ctrl.shared_recv_ipc_irq_id, (voidfuncptr)icc_ipc_isr, ICC_SHAREDTASK_SHAREDIPC_IDX))
	{
		ret = ICC_REGISTER_INT_FAIL;
		goto error_int; /*lint !e801 */
	}
	if (ICC_ERR == bsp_ipc_int_enable((IPC_INT_LEV_E)g_icc_ctrl.shared_recv_ipc_irq_id))
	{
		ret = ICC_REGISTER_INT_FAIL;
		goto error_int; /*lint !e801 */
	}

	icc_print_error("ok\n");

	g_icc_ctrl.state = ICC_INITIALIZED;

	ret = bsp_icc_event_register((ICC_CHN_DYNAMIC<<16), (read_cb_func)icc_dynamic_data_receive, NULL, NULL, NULL);
	if (ICC_OK != ret)
	{
	    icc_print_error("reg icc_test_cm_cb err ret=0x%08x\n", ret);
	    return ICC_REGISTER_DYNAMIC_CB_FAIL;
	}

	return ICC_OK;

error_int:
	kthread_stop((icc_task_id)g_icc_ctrl.shared_task_id); 

error_task:
	for(i = 0; i < ICC_CHN_ID_MAX; i++)
	{
        if (!g_icc_ctrl.channels[i])
        {
            continue ;
        }
        bsp_icc_channel_uninit(g_icc_ctrl.channels[i]->id);
	}

icc_channels_init_err:

	icc_print_error("icc init errno: 0x%x\n", ret);

	return ret;
}

s32 bsp_icc_dynamic_event_register(u32 destcore, struct icc_dynamic_para *parameter)
{
	int ret = ICC_ERR;
	int len = 0;
	int i;
	u32 channel_id;
	struct icc_channel *channel = NULL;
	struct icc_channel_vector *vector = NULL;
	struct icc_init_info init_info = {0};
	
	if(count > ICC_DYNAMIC_CHAN_NUM_MAX)
	{
		icc_print_error("dynamic_count larger than max dynamic channel number\n");
		return ICC_INVALID_PARA;
	}

	if(parameter->name == NULL || parameter->size == 0 || parameter->size > ICC_DYNAMIC_SIZE ||strlen(parameter->name) +1> 15)
	{
		icc_print_error("name or size invalid, please check your input name or size.\n");
		return ICC_INVALID_PARA;
	}

	for(i=0; i < ICC_DYNAMIC_CHAN_NUM_MAX; i++)
	{
		if(!strncmp(dynamic_info[i].name, parameter->name,30))
		{
			if(dynamic_info[i].real_channel_id != 0)
			{
				g_icc_ctrl.channels[dynamic_info[i].real_channel_id]->vector->read_cb = parameter->read_cb;
				g_icc_ctrl.channels[dynamic_info[i].real_channel_id]->vector->read_context = parameter->read_context;
				icc_print_error("channel id already exist: (%d), add callback function for this channel.\n", dynamic_info[i].real_channel_id);
				channel_id = dynamic_info[i].real_channel_id;
				return channel_id;/*lint !e713 */
			}
			else
			{
				icc_print_error("name already used, please check again\n");
				return ICC_INVALID_PARA;
			}
		}
	}

	init_info.name = parameter->name;                       /* 通道名称 */
	init_info.real_channel_id = dynamic_channel_id;
	init_info.mode = (ICC_IPC_SHARED << 1) | (ICC_IPC_SHARED);   /* 共享任务，共享ipc */
	init_info.fifo_size = parameter->size;
	init_info.ipc_send_irq_id = IPC_CCPU_INT_SRC_ACPU_ICC;
	init_info.ipc_recv_irq_id = IPC_ACPU_INT_SRC_CCPU_ICC;
	init_info.func_size = 20;
	
	/* coverity[overrun-local] */
	/* coverity[secure_coding] */
	memcpy(dynamic_info[count].name, parameter->name, (sizeof(dynamic_info[count].name)-1));
	dynamic_info[count].real_channel_id = dynamic_channel_id; /* [false alarm]:fortify */
	channel_id = dynamic_info[count].real_channel_id; /* [false alarm]:fortify */
	dynamic_info[count].fifo_size = parameter->size; /* [false alarm]:fortify */

	/* 本核上，本通道，先是接收后是发送 */
	if(count == 0)
	{
		init_info.recv_addr = (void*)(ICC_SDDR_DYNAMIC_START_ADDR_ON_THIS_CORE);
		init_info.send_addr = (void*)(init_info.recv_addr + sizeof(struct icc_channel_fifo) + dynamic_info[count].fifo_size); /* [false alarm]:fortify */
	}
	else
	{
		init_info.recv_addr = (void*)((unsigned long)(g_icc_ctrl.channels[dynamic_channel_id-1]->fifo_recv)+ 2 * (unsigned long)sizeof(struct icc_channel_fifo) + 2 * (unsigned long)dynamic_info[count-1].fifo_size);/*lint !e679 */
		init_info.send_addr = (void*)(init_info.recv_addr + sizeof(struct icc_channel_fifo) + dynamic_info[count].fifo_size); /* [false alarm]:fortify */
	}

	if ((char *)(init_info.send_addr + sizeof(struct icc_channel_fifo) + dynamic_info[count].fifo_size) >(char *) ICC_SDDR_ADDR_MAX) /*lint !e613 */ /* [false alarm]:fortify */
	{
		icc_print_error("total channel size too big, already more than the size of the memory can be used.\n");
		return ICC_INIT_ADDR_TOO_BIG;
	}
	
	/* 初始化通道 */
	channel = icc_channel_init(&init_info, &ret);/*lint !e838 */ /* [false alarm]:fortify */
	if ((NULL == channel) || (ICC_OK != ret)) 
	{
		icc_print_error("chan setup err\n");
		return ICC_CHN_INIT_FAIL;
	}

	g_icc_ctrl.channels[dynamic_channel_id] = channel;

	ret = icc_debug_channel_info_init(g_icc_ctrl.channels[dynamic_channel_id]);
	if (ret == ICC_ERR)
	{
		icc_print_error("icc debug channel init failed!\n");
		return ICC_CHN_INIT_FAIL;
	}
	/* coverity[operator_confusion] */
	/* coverity[remediation] */
	vector = &(g_icc_ctrl.channels[GET_CHN_ID((u32)(dynamic_channel_id<<16))]->vector[GET_FUNC_ID((u32)(dynamic_channel_id<<16))]);/*lint !e838 */
	if(vector->read_cb != NULL || vector->write_cb != NULL)
	{
		icc_print_error("vector callback function not NULL, cannot register\n");
		return ICC_REGISTER_CB_FAIL;
	}
	
	vector->read_cb = parameter->read_cb;
	vector->read_context = parameter->read_context;
	vector->write_cb = NULL;
	vector->write_context = NULL;

	icc_wake_lock(&g_icc_ctrl.wake_lock);
	osl_sem_up(&g_icc_ctrl.shared_task_sem);

	if(0xff != destcore)
	{
		len = bsp_icc_send(destcore, (ICC_CHN_DYNAMIC<<16), (unsigned char*)&dynamic_info[count], sizeof(struct icc_dynamic_info));/*lint !e747 *//* [false alarm]:fortify */
	        if((int)sizeof(struct icc_dynamic_info) != len)
	        {
	            icc_print_error("send len(%x) != expected len(%x).\n", len, sizeof(struct icc_dynamic_info));
	            return ICC_DYNAMIC_SEND_FAIL;
	        }
	}
	
	dynamic_channel_id++;
	count++;

	return channel_id;/*lint !e713 */
}

s32 icc_dynamic_data_receive(void)
{
	s32 ret = 0;
	int len = 0;
	int i = 0;
	u32 flag = 0xff;
	struct icc_dynamic_info dynamic_rec_info = {0};
	struct icc_dynamic_para parameter = {0};
	u32 channel_id = (ICC_CHN_DYNAMIC<<16);

	len = bsp_icc_read(channel_id, (u8*)&dynamic_rec_info, (u32)sizeof(struct icc_dynamic_info));/*lint !e838 */
	if(len != (int)sizeof(struct icc_dynamic_info))
	{
	    icc_print_error("read len(%x) != expected len(%x).\n", len, sizeof(struct icc_dynamic_info));
	    return ICC_ERR;
	}

	for (i = 0; i< ICC_DYNAMIC_CHAN_NUM_MAX; i++)/*lint !e838 */
	{
		if(dynamic_info[i].real_channel_id== dynamic_rec_info.real_channel_id)
		{
			icc_print_error("dynamic_channel number already used, existed channel no. is(%d), received dynamic channel no. is(%d).\n, init_info->real_channel_id, dynamic_init_info->real_channel_id");
			return ICC_INVALID_PARA;
		}
	}

	parameter.name = dynamic_rec_info.name;
	parameter.size = dynamic_rec_info.fifo_size;
	parameter.read_cb = NULL;
	parameter.read_context = NULL;
	
	ret = bsp_icc_dynamic_event_register(flag, &parameter);/*lint !e838 */
	if(ret == ICC_INVALID_PARA ||ret == ICC_INIT_ADDR_TOO_BIG ||ret == ICC_CHN_INIT_FAIL ||ret == ICC_REGISTER_CB_FAIL ||ret == ICC_DYNAMIC_SEND_FAIL)
	{
		icc_print_error("dynamic channel creat failed after received msg.\n");
	}
	return ICC_OK;
}

s32 bsp_icc_event_register(u32 channel_id, read_cb_func read_cb,   void *read_context,
	                                            write_cb_func write_cb, void *write_context)
{
	struct icc_channel_vector *vector = NULL;

	if(g_icc_ctrl.channels[GET_CHN_ID(channel_id)] == NULL)
	{
		icc_print_error("channel:0x%x is NULL\n", channel_id);
		return ICC_NULL_PTR;
	}

	if((GET_CHN_ID(channel_id)  >= ICC_CHN_ID_MAX) ||
	   (GET_FUNC_ID(channel_id) >= g_icc_ctrl.channels[GET_CHN_ID(channel_id)]->func_size))
	{
		icc_print_error("wrong para chan_id=0x%x\n", channel_id);
		return ICC_INVALID_PARA;
	}
	/*lint --e{409} */
	vector = &(g_icc_ctrl.channels[GET_CHN_ID(channel_id)]->vector[GET_FUNC_ID(channel_id)]);
	if(vector->read_cb != NULL || vector->write_cb != NULL)
	{
		icc_print_error("%p reged\n", read_cb);
		return ICC_REGISTER_CB_FAIL;
	}

	vector->read_cb = read_cb;
	vector->read_context = read_context;
	vector->write_cb = write_cb;
	vector->write_context = write_context;
	
	icc_wake_lock(&g_icc_ctrl.wake_lock);
	osl_sem_up(&g_icc_ctrl.shared_task_sem);

	return ICC_OK;
}

s32 bsp_icc_event_unregister(u32 channel_id)
{
	struct icc_channel_vector *vector = NULL;

	if(g_icc_ctrl.channels[GET_CHN_ID(channel_id)] == NULL)
	{
		icc_print_error("channel:0x%x is NULL\n", channel_id);
		return ICC_NULL_PTR;
	}

	if((GET_CHN_ID(channel_id) >= ICC_CHN_ID_MAX) ||
		(GET_FUNC_ID(channel_id) >= g_icc_ctrl.channels[GET_CHN_ID(channel_id)]->func_size))
	{
		icc_print_error("para err,chan_id=0x%x\n", channel_id);
		return ICC_INVALID_PARA;
	}

	vector = &(g_icc_ctrl.channels[GET_CHN_ID(channel_id)]->vector[GET_FUNC_ID(channel_id)]);
	if(!vector) /*lint !e774 */
	{
		icc_print_error("vector NULL\n");
		return ICC_NULL_PTR;
	}

	vector->read_cb = NULL;
	vector->read_context = NULL;
	vector->write_cb = NULL;
	vector->write_context = NULL;

	return ICC_OK;
}

s32 bsp_icc_ipc_extend_send(u32 enDstCore, u8 flag)
{
	int len = 0;
	u8 data = flag;
	len = bsp_icc_send((u32)enDstCore,(ICC_CHN_IPC_EXTEND << 16),&data,sizeof(data));/*lint !e838 *//*lint !e747 */
	if((int)sizeof(data) != len)
        {
            icc_print_error("send len(%x) != expected len(%x).\n", len, sizeof(data));
            return ICC_SEND_ERR;
        }
	return ICC_OK;
}

s32 bsp_icc_send(u32 cpuid, u32 channel_id, u8* data, u32 data_len)
{
	if(((cpuid >= ICC_CPU_MAX) || (cpuid == ICC_THIS_CPU)) || (GET_CHN_ID(channel_id) >= ICC_CHN_ID_MAX) || (!g_icc_ctrl.channels[GET_CHN_ID(channel_id)])
	   || (GET_FUNC_ID(channel_id) >= g_icc_ctrl.channels[GET_CHN_ID(channel_id)]->func_size) ||
	   (data_len + sizeof(struct icc_channel_packet) >= g_icc_ctrl.channels[GET_CHN_ID(channel_id)]->fifo_send->size) )
	{
		icc_print_error("para err,cpuid=0x%x, chan_id=0x%x, data=%p, data_len=0x%x\n", cpuid, channel_id, data, data_len);
		return ICC_INVALID_PARA;
	}

	if (1 == (icc_ccore_is_reseting(cpuid)))
	{
	    return BSP_ERR_ICC_CCORE_RESETTING;
	}

	return data_send(cpuid, channel_id, data,data_len);
}

s32 bsp_icc_read(u32 channel_id, u8 *buf, u32 buf_len)
{
	u32 read_len = 0;
	s32 ret = ICC_OK;/*lint !e14 */
	unsigned long flags = 0;
	u32 real_channel_id = GET_CHN_ID(channel_id);
	u32 func_id = GET_FUNC_ID(channel_id);

	UNUSED(flags);
	if(g_icc_ctrl.channels[GET_CHN_ID(channel_id)] == NULL)
	{
		icc_print_error("channel:0x%x is NULL\n", channel_id);
		return ICC_NULL_PTR;
	}

	if((!buf) || (real_channel_id >= ICC_CHN_ID_MAX) ||(func_id >= g_icc_ctrl.channels[real_channel_id]->func_size))
	{
		ret = ICC_INVALID_PARA;
		goto out; /*lint !e801 */
	}

	if(!(g_icc_ctrl.channels[real_channel_id]->ready_recv))
	{
		return 0;
	}

	spin_lock_irqsave(&(g_icc_ctrl.channels[real_channel_id]->read_lock), flags);
	/*fifo中消息不完整*/
	if(fifo_read_space_get(g_icc_ctrl.channels[real_channel_id]->fifo_recv) >= sizeof(struct icc_channel_packet))
	{
		read_len = fifo_get_with_header(g_icc_ctrl.channels[real_channel_id]->fifo_recv, buf, buf_len);
		icc_debug_in_read_cb(channel_id, buf, buf_len, g_icc_ctrl.channels[real_channel_id]->fifo_recv->read, g_icc_ctrl.channels[real_channel_id]->fifo_recv->write);
		if((u32)ICC_ERR == read_len)
		{
			ret = ICC_INVALID_PARA;
			spin_unlock_irqrestore(&(g_icc_ctrl.channels[real_channel_id]->read_lock), flags);
			goto out; /*lint !e801 */
		}
	}
	spin_unlock_irqrestore(&(g_icc_ctrl.channels[real_channel_id]->read_lock), flags);
	ret = (s32)read_len;

	return ret;

out:
	icc_print_error("errno=0x%x,buffer=%p,len%d,read_len%d\n", ret, buf, buf_len, read_len);

	return ret;
}

EXPORT_SYMBOL(bsp_icc_read);            /*lint !e19 */
EXPORT_SYMBOL(bsp_icc_send);            /*lint !e19 */
EXPORT_SYMBOL(bsp_icc_event_register);  /*lint !e19 */
EXPORT_SYMBOL(bsp_icc_event_unregister);/*lint !e19 */
arch_initcall(bsp_icc_init);            /*lint !e19 !e132 !e578*/

