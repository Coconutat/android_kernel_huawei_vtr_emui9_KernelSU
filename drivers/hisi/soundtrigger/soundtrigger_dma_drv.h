/*
 *	soundtrigger_dma_drv is a kernel driver which is used to manager dma
 *	Copyright (C) 2015	Hisilicon

 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.

 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.

 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _AUDIO_AUTO_SOUNDTRIGGER_DMA_DRV_H
#define _AUDIO_AUTO_SOUNDTRIGGER_DMA_DRV_H

#include "soc_acpu_baseaddr_interface.h"
/*slimbus register addr*/
#define HI3xxx_SLIMBUS_BASE_REG 							(SOC_ACPU_SLIMBUS_BASE_ADDR)
#define HI3xxx_SLIMBUS_REG_SIZE 							(100)
#define HI3xxx_SLIMBUS_PORT0_REG_0							(0x100)
#define HI3xxx_SLIMBUS_PORT0_REG_1							(0x104)
#define HI3xxx_SLIMBUS_PORT1_REG_0							(0x108)
#define HI3xxx_SLIMBUS_PORT1_REG_1							(0x10C)
#define HI3xxx_SLIMBUS_PORT2_REG_0							(0x110)
#define HI3xxx_SLIMBUS_PORT2_REG_1							(0x114)
#define HI3xxx_SLIMBUS_PORT3_REG_0							(0x118)
#define HI3xxx_SLIMBUS_PORT3_REG_1							(0x11C)
#define HI3xxx_SLIMBUS_PORT4_REG_0							(0x120)
#define HI3xxx_SLIMBUS_PORT4_REG_1							(0x124)
#define HI3xxx_SLIMBUS_PORT5_REG_0							(0x128)
#define HI3xxx_SLIMBUS_PORT5_REG_1							(0x12C)
#define HI3xxx_SLIMBUS_PORT6_REG_0							(0x130)
#define HI3xxx_SLIMBUS_PORT6_REG_1							(0x134)
#define HI3xxx_SLIMBUS_PORT7_REG_0							(0x138)
#define HI3xxx_SLIMBUS_PORT7_REG_1							(0x13C)
#define HI3xxx_SLIMBUS_PORT8_REG_0							(0x140)
#define HI3xxx_SLIMBUS_PORT8_REG_1							(0x144)
#define HI3xxx_SLIMBUS_PORT9_REG_0							(0x148)
#define HI3xxx_SLIMBUS_PORT9_REG_1							(0x14C)
#define HI3xxx_SLIMBUS_PORT10_REG_0 						(0x150)
#define HI3xxx_SLIMBUS_PORT10_REG_1 						(0x154)
#define HI3xxx_SLIMBUS_PORT11_REG_0 						(0x158)
#define HI3xxx_SLIMBUS_PORT11_REG_1 						(0x15C)

#define DRV_DMA_LLI_LINK(uwAddr)		(((uwAddr) & 0xffffffe0UL) | (0x2UL))

#define CH_0_UNMASK_BIT 			0x0
#define DMA_CH_UNMASK				0x198
#define DMA_FAST_LEFT_CH_UNMASK 	0x80
#define DMA_FAST_RIGHT_CH_UNMASK	0x100
#define DMA_NORMAL_LEFT_CH_UNMASK	0x8
#define DMA_NORMAL_RIGHT_CH_UNMASK	0x10

#define DMA_FAST_LEFT_CH_NUM 	4
#define DMA_FAST_RIGHT_CH_NUM	8
#define DMA_NORMAL_LEFT_CH_NUM	3
#define DMA_NORMAL_RIGHT_CH_NUM	7

#define DMA_CH_INT_CLR 0x1
#define DMA_ENABLE_BIT 0

#define PCM_SWAP_BUFFER_NUM 	(6)

#define FAST_CHANNEL_TIMEOUT_READ_COUNT 	200
#define NORMAL_CHANNEL_TIMEOUT_READ_COUNT	200
#define NORMAL_CHANNEL_TIMEOUT_TRAN_COUNT	1000



typedef int ST_ERROR_NUM_INT32;
typedef int ST_IRQ_STATE_INT32;

enum codec_hifi_type {
	CODEC_HI6402 = 0,
	CODEC_HI6403,
	CODEC_HI640X_MAX,
} ;

enum fast_tran_state {
	FAST_TRAN_NOT_COMPLETE = 0,
	FAST_TRAN_COMPLETE,
};

enum read_state {
	READ_NOT_COMPLETE = 0,
	READ_COMPLETE,
};

enum find_state {
	FRAME_NOT_FIND = 0,
	FRAME_FIND,
};

enum dma_drv_state {
	DMA_DRV_NO_INIT = 0,
	DMA_DRV_INIT,
};

enum soundtrigger_irq_state {
	IRQ_ERR = 0,
	IRQ_FINISH,
};

enum {
	SOUNDTRIGGER_PCM_FAST = 0,
	SOUNDTRIGGER_PCM_NORMAL,
	SOUNDTRIGGER_PCM_CHAN_NUM,
};

struct soundtrigger_io_sync_param {
	unsigned int para_in_l;
	unsigned int para_in_h;
	unsigned int para_size_in;

	unsigned int para_out_l;
	unsigned int para_out_h;
	unsigned int para_size_out;
};

struct st_fast_status{
	int fm_status;
	int fast_status;
};

#define INT_TO_ADDR(low,high) (void*) (unsigned long)((unsigned long long)(low) | ((unsigned long long)(high)<<32))

#define HWLOCK_WAIT_TIME	50

#define SOUNDTRIGGER_CMD_DMA_OPEN  _IO('S',  0x1)
#define SOUNDTRIGGER_CMD_DMA_CLOSE  _IO('S',  0x2)
#define SOUNDTRIGGER_CMD_DMA_READY  _IO('S',  0x3)

int32_t hi64xx_soundtrigger_init(enum codec_hifi_type type);
void hi64xx_soundtrigger_dma_close(void);

#endif /* end of _AUDIO_AUTO_TEST_ASP_CFG_H */

