/*
 * hi3xxx mailbox device driver
 *
 * Copyright (c) 2013-2014 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/mod_devicetable.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_irq_affinity.h>
#include <linux/kern_levels.h>

#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG	AP_MAILBOX_TAG

#define IPCBITMASK(n)				(1 << (n))
#define IPCMBxSOURCE(mdev)			((mdev) << 6)
#define IPCMBxDSET(mdev)			(((mdev) << 6) + 0x04)
#define IPCMBxDCLR(mdev)			(((mdev) << 6) + 0x08)
#define IPCMBxDSTATUS(mdev)			(((mdev) << 6) + 0x0C)
#define IPCMBxMODE(mdev)			(((mdev) << 6) + 0x10)
#define IPCMBxIMASK(mdev)			(((mdev) << 6) + 0x14)
#define IPCMBxICLR(mdev)			(((mdev) << 6) + 0x18)
#define IPCMBxSEND(mdev)			(((mdev) << 6) + 0x1C)
#define IPCMBxDATA(mdev, index)		(((mdev) << 6) + 0x20 + ((index) << 2))
#define IPCCPUxIMST(cpu)			(((cpu) << 3) + 0x800)
#define IPCCPUxIRST(cpu)			(((cpu) << 3) + 0x804)
#define IPCLOCK()				(0xA00)

#define FAST_MBOX				(1 << 0)
#define COMM_MBOX				(1 << 1)
#define SOURCE_MBOX				(1 << 2)
#define DESTINATION_MBOX			(1 << 3)

#define EVERY_LOOP_TIME_MS		5

#define IPC_UNLOCKED				0x00000000
#define IPCACKMSG				0x00000000
#define COMM_MBOX_IRQ				(-2)
#define AUTOMATIC_ACK_CONFIG			(1 << 0)
#define NO_FUNC_CONFIG				(0 << 0)

/* Optimize interrupts assignment */
#define IPC_IRQ_AFFINITY_CPU			(1)

#define SYS_RPROC_NUMBER 0x9

#define ISP_RPROC_NUMBER  0x2
#define AO_RPROC_NUMBER  0x3
#define NPU_RPROC_NUMBER  0x3
#define STATE_NUMBER  0x4

#define MAILBOX_ASYNC_UDELAY_CNT   (1000)

#define ISP_INDEX_BASE				100
#define AO_INDEX_BASE				200
#define NPU_INDEX_BASE				300
#define DEFAULT_MAILBOX_TIMEOUT	300
#define DEFAULT_FIFO_SIZE			256
#define DEFAULT_SCHED_PRIORITY	20
#define MAILBOX_NO_USED			0
#define MAX_AP_IPC_INDEX			99

#define MDEV_ERR(fmt, args ...)	\
	({				\
		pr_err(fmt "\n", ##args); \
	})

#define MDEV_INFO(fmt, args ...)	\
	({				\
		pr_info(fmt "\n", ##args); \
	})

/*MDEV_DEBUG used only in project  developing  phase*/
#define MDEV_DEBUG(fmt, args ...)	\
	({			\
		; \
	})


enum {
	RX_BUFFER_TYPE = 0,
	ACK_BUFFER_TYPE,
	MBOX_BUFFER_TYPE_MAX,
};

/*
 * Table for available remote processors. DTS sub-node, "remote_processor_type",
 * of node, "hisi_mdev", is configured according to the table.
 *
 * If the table was modified, DTS configiuration should be updated accordingly.
 */
typedef enum {
	GIC = 0,
	GIC_1 = 0,
	GIC_2,
	IOM3,
	LPM3,
	HIFI,
	MCPU,
	BBE16,
	IVP32,
	ISP,
	UNCERTAIN_REMOTE_PROCESSOR,
	NPU_IPC_GIC = 2,/*lint !e488*/
	HI3XXX_RP_TYPES/*lint !e488*/
} remote_processor_type_t;/*lint !e488*/


struct hisi_common_mbox_info {
	int gic_1_irq_requested;
	int gic_2_irq_requested;
	int cmbox_gic_1_irq;
	int cmbox_gic_2_irq;
	struct hisi_mbox_device *cmdev;
};

struct hisi_ipc_device {
	void __iomem *base;
	u32 unlock;
	mbox_msg_t *buf_pool;
	struct hisi_common_mbox_info *cmbox_info;
	struct hisi_mbox_device **mdev_res;
};

struct hisi_mbox_device_priv {
	u8 func;
	remote_processor_type_t src;
	remote_processor_type_t des;
	int mbox_channel;
	int irq;
	int capability;
	int used;
	unsigned int timeout;
	unsigned int fifo_size;
	unsigned int sched_priority;
	unsigned int sched_policy;
	unsigned int hardware_board_type;
	struct hisi_ipc_device *idev;
};

/*
**HiIPCV230 fixed all the communicate  processors to the unique bits:
**austin:
**00000001:A53
**00000010:Maia
**00000100:IOM7
**00001000:LPM3
**00010000:ASP
**00100000:Modem-A9
**01000000:Modem-bbe16
**10000000:IVP32
**
**chicago:
**000000001:A53
**000000010:Maia
**000000100:IOM7
**000001000:LPM3
**000010000:ASP
**000100000:Modem-A9
**001000000:Modem-bbe16
**010000000:IVP32
**100000000:ISP32
*/
char *sys_rproc_name[SYS_RPROC_NUMBER] = {
	"ACPU",
	"ACPU",
	"SENSORHUB",
	"LPMCU",
	"HIFI",
	"MODEM",
	"BBE16",
	"IVP",
	"ISP"
};

/* only used in austin and dallas */
char *isp_rproc_name[ISP_RPROC_NUMBER] = {
	"ACPU",
	"ISP"
};

char *ao_rproc_name[AO_RPROC_NUMBER] = {
	"SENSORHUB",
	"ACPU",
	"ISP"
};
/*on lite*/
char * npu_rproc_name[NPU_RPROC_NUMBER] = {
	"AICPU",
	"TSCPU",
	"AP_LIT_CLUSTER"
};
/*
**HiIPCV230 have a state machine, the state machine have 4 status:
**4'b0001:IDLE_STATE
**4'b0010:SOURCE_STATE
**4'b0100:DEST_STATE
**4'b1000:ACK_STATE
*/
char *ipc_state_name[STATE_NUMBER] = {
	"%s  is idle\n",
	"%s  is occupied\n",
	"%s  may be power off or freeze\n",
	"%s  have no time to handle ACK\n"
};

enum IPC_STATE_MACHINE {
	IDLE_STATE,
	SOURCE_STATE,
	DEST_STATE,
	ACK_STATE
};

extern int hisi_rproc_init(void);

unsigned char _rproc_find_index(const char *mdev_name, unsigned int pro_code)
{
	unsigned char index = 0;
	while (pro_code) {
		index++;
		pro_code >>= 1;
	}
	return index;
}

char *rproc_analysis(const char *mdev_name, unsigned int pro_code)
{
	unsigned char index = _rproc_find_index(mdev_name, pro_code);

	if (0 == index)
		return "ERR_RPROC";

	/*npu ipc's mailbox channel */
    if (NULL != strstr(mdev_name, "npu")) {
		if (likely(index < NPU_RPROC_NUMBER))
			return npu_rproc_name[index];
		else
			return "ERR_RPROC";
	}else if (NULL != strstr(mdev_name, "isp")) {
		if (likely(index < ISP_RPROC_NUMBER))
			return isp_rproc_name[index];
		else
			return "ERR_RPROC";
	} else if (NULL != strstr(mdev_name, "ao")){
		if (likely(index < AO_RPROC_NUMBER))
			return ao_rproc_name[index];
		else
			return "ERR_RPROC";
	} else {					/*isp  ips's mailbox channel */
		if (likely(index < SYS_RPROC_NUMBER))
			return sys_rproc_name[index];
		else
			return "ERR_RPROC";
	}

}

char *ipc_state_analysis(unsigned int mode, unsigned char *outp)
{
	unsigned char index = 0;
	mode >>= 4;					/*bit4~bit7 is the state machine index */
	while (mode) {
		index++;
		mode >>= 1;
	}
	if (likely(0 != index))
		index--;
	else
		return "%s ERR_STATE\n";

	*outp = index;

	if (likely(index < STATE_NUMBER))
		return ipc_state_name[index];
	else
		return "%s ERR_STATE\n";
}

static inline void __ipc_lock(void __iomem *base, unsigned int lock_key)
{
	__raw_writel(lock_key, base + IPCLOCK());
}

static inline void __ipc_unlock(void __iomem *base, unsigned int key)
{
	__raw_writel(key, base + IPCLOCK());
}

static inline unsigned int __ipc_lock_status(void __iomem *base)
{
	return __raw_readl(base + IPCLOCK());
}

static inline void __ipc_set_src(void __iomem *base, int source, int mdev)
{
	__raw_writel(IPCBITMASK(source), base + IPCMBxSOURCE(mdev));/*lint !e679*/
}

static inline unsigned int __ipc_read_src(void __iomem *base, int mdev)
{
	return __raw_readl(base + IPCMBxSOURCE(mdev));/*lint !e679*/
}

static inline void __ipc_set_des(void __iomem *base, int source, int mdev)
{
	__raw_writel(IPCBITMASK(source), base + IPCMBxDSET(mdev));/*lint !e679*/
}

static inline void __ipc_clr_des(void __iomem *base, int source, int mdev)
{
	__raw_writel(IPCBITMASK(source), base + IPCMBxDCLR(mdev));/*lint !e679*/
}

static inline unsigned int __ipc_des_status(void __iomem *base, int mdev)
{
	return __raw_readl(base + IPCMBxDSTATUS(mdev));/*lint !e679*/
}

static inline void __ipc_send(void __iomem *base, unsigned int tosend, int mdev)
{
	__raw_writel(tosend, base + IPCMBxSEND(mdev));/*lint !e679*/
}

static inline unsigned int __ipc_read(void __iomem *base, int mdev, int index)
{
	return __raw_readl(base + IPCMBxDATA(mdev, index));/*lint !e679*/
}

static inline void __ipc_write(void __iomem *base, u32 data, int mdev, int index)
{
	__raw_writel(data, base + IPCMBxDATA(mdev, index));/*lint !e679*/
}

static inline unsigned int __ipc_cpu_imask_get(void __iomem *base, int mdev)
{
	return __raw_readl(base + IPCMBxIMASK(mdev));/*lint !e679*/
}

static inline void __ipc_cpu_imask_clr(void __iomem *base, unsigned int toclr, int mdev)
{
	unsigned int reg;

	reg = __raw_readl(base + IPCMBxIMASK(mdev));/*lint !e679*/
	reg = reg & (~(toclr));

	__raw_writel(reg, base + IPCMBxIMASK(mdev));/*lint !e679*/
}

static inline void __ipc_cpu_imask_all(void __iomem *base, int mdev)
{
	__raw_writel((~0), base + IPCMBxIMASK(mdev));/*lint !e679*/
}

static inline void __ipc_cpu_iclr(void __iomem *base, unsigned int toclr, int mdev)
{
	__raw_writel(toclr, base + IPCMBxICLR(mdev));/*lint !e679*/
}

static inline int __ipc_cpu_istatus(void __iomem *base, int mdev)
{
	return __raw_readl(base + IPCMBxICLR(mdev));/*lint !e679*/
}

static inline unsigned int __ipc_mbox_istatus(void __iomem *base, int cpu)
{
	return __raw_readl(base + IPCCPUxIMST(cpu));/*lint !e679*/
}

static inline unsigned int __ipc_mbox_irstatus(void __iomem *base, int cpu)
{
	return __raw_readl(base + IPCCPUxIRST(cpu));/*lint !e679*/
}

static inline unsigned int __ipc_status(void __iomem *base, int mdev)
{
	return __raw_readl(base + IPCMBxMODE(mdev));/*lint !e679*/
}

static inline void __ipc_mode(void __iomem *base, unsigned int mode, int mdev)
{
	__raw_writel(mode, base + IPCMBxMODE(mdev));/*lint !e679*/
}

static int hisi_mdev_startup(struct hisi_mbox_device *mdev)
{
	/*
	 * nothing won't be done during suspend & resume flow for HI3xxx IPC.
	 * see dummy like SR function, hisi_mdev_suspend & hisi_mdev_resume.
	 * reserve runtime power management proceeding for further modification,
	 * if necessary.
	 */
	return 0;
}

static void hisi_mdev_shutdown(struct hisi_mbox_device *mdev)
{
	/*
	 * nothing won't be done during suspend & resume flow for HI3xxx IPC.
	 * see dummy like SR function, hisi_mdev_suspend & hisi_mdev_resume.
	 * reserve runtime power management proceeding for further modification,
	 * if necessary.
	 */
	return;
}

static void hisi_mdev_dump_status(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	/*the size 512 is the sumary max size of  sys_rproc_name and ipc_state_name */
	char finalfortmat[512] = { 0 };
	char statem = 0;
	char *src_name = rproc_analysis(mdev->name, __ipc_read_src(priv->idev->base, priv->mbox_channel));
	char *des_name = rproc_analysis(mdev->name, __ipc_des_status(priv->idev->base, priv->mbox_channel));
	/*\0013 is the  KERN_SOH KERN_ERR */
	char *direcstr = KERN_ERR "[ap_ipc]: [%s]-->[%s], ";
	char *machinestr = ipc_state_analysis(__ipc_status(priv->idev->base, priv->mbox_channel), (unsigned char *)&statem);

	memcpy(finalfortmat, direcstr, strlen(direcstr));

	strncat(finalfortmat, machinestr, strlen(machinestr));

	if (DEST_STATE == statem)
		printk(finalfortmat, src_name, des_name, des_name);
	else if (ACK_STATE == statem)
		printk(finalfortmat, src_name, des_name, src_name);
	else
		printk(finalfortmat, src_name, des_name, mdev->name);

	return;
}

static void hisi_mdev_dump_regs(struct hisi_mbox_device *mdev){
	/*
	struct hisi_mbox_device_priv *priv = mdev->priv;

	MDEV_ERR("%s CPU_IMST: 0x%08x",mdev->name, __ipc_mbox_istatus(priv->idev->base, priv->src));
	MDEV_ERR("%s CPU_IRST: 0x%08x",mdev->name, __ipc_mbox_irstatus(priv->idev->base, priv->src));
	*/
}

static int hisi_mdev_check(struct hisi_mbox_device *mdev, mbox_mail_type_t mtype, int mdev_index)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	int ret = RPUNACCESSIBLE;
	int index = priv->mbox_channel;
	if (NULL != strstr(mdev->name, "isp")) {
		index = index + ISP_INDEX_BASE;
		MDEV_DEBUG("isp-index is %d",index);
	}
	if (NULL != strstr(mdev->name, "ao")) {
		index = index + AO_INDEX_BASE;
		MDEV_DEBUG("ao-index is %d\n",index);
	}
	if (NULL != strstr(mdev->name, "npu")) {
		index = index + NPU_INDEX_BASE;
		MDEV_DEBUG("npu-index is %d\n",index);
	}


	if ((TX_MAIL == mtype) && (SOURCE_MBOX & priv->func) && (index == mdev_index) && (priv->used == 1))
		ret = RPACCESSIBLE;
	else if ((RX_MAIL == mtype) && (DESTINATION_MBOX & priv->func) && (index == mdev_index) && (priv->used == 1))
		ret = RPACCESSIBLE;

	return ret;
}

static void hisi_mdev_clr_ack(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	unsigned int imask;
	unsigned int toclr;

	imask = __ipc_cpu_imask_get(priv->idev->base, priv->mbox_channel);
	if (NULL != strstr(mdev->name, "npu-mailbox"))
	{
		toclr = IPCBITMASK(NPU_IPC_GIC) & (~imask);
	}
	else if (NULL != strstr(mdev->name, "ao")) {
		toclr = IPCBITMASK(GIC_2) & (~imask);
	} else {
		toclr = (IPCBITMASK(GIC_1) | IPCBITMASK(GIC_2)) & (~imask);
	}

	__ipc_cpu_iclr(priv->idev->base, toclr, priv->mbox_channel);
}

static void hisi_mdev_clr_irq_and_ack(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	unsigned int status = 0;
	unsigned int imask;
	unsigned int todo;
	int i;

	/*
	 * temporarily, local processor will clean msg register,
	 * and ack zero for an ipc from remote processors.
	 */
	for (i = 0; i < priv->capability; i++)
		__ipc_write(priv->idev->base, IPCACKMSG, priv->mbox_channel, i);

	imask = __ipc_cpu_imask_get(priv->idev->base, priv->mbox_channel);
	/*get the irq unmask core bits, and clear the irq according to the unmask core bits,
	 * because the irq to be sure triggered to the unmasked cores
	 */
	if (NULL != strstr(mdev->name, "npu-mailbox"))
	{
		todo = IPCBITMASK(NPU_IPC_GIC) & (~imask);
	}
	else if (NULL != strstr(mdev->name, "ao")) {
		todo = IPCBITMASK(GIC_2) & (~imask);
	} else {
		todo = (IPCBITMASK(GIC_1) | IPCBITMASK(GIC_2)) & (~imask);
	}

	__ipc_cpu_iclr(priv->idev->base, todo, priv->mbox_channel);

	status = __ipc_status(priv->idev->base, priv->mbox_channel);

	if ((DESTINATION_STATUS & status) && (!(AUTOMATIC_ACK_CONFIG & status))) {
		__ipc_send(priv->idev->base, todo, priv->mbox_channel);
	}

	return;
}

static void hisi_mdev_ack(struct hisi_mbox_device *mdev, mbox_msg_t *msg, mbox_msg_len_t len)
{
	return;
}

static mbox_msg_len_t hisi_mdev_hw_read(struct hisi_mbox_device *mdev, mbox_msg_t *msg)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	mbox_msg_len_t cap;
	int i;

	cap = priv->capability;
	for (i = 0; i < cap; i++)
		msg[i] = __ipc_read(priv->idev->base, priv->mbox_channel, i);

	return cap;
}

/*to judge the four kind machine status of the ip, they are idle,src,des,ack*/
static int hisi_mdev_is_stm(struct hisi_mbox_device *mdev, unsigned int stm)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	int is_stm = 0;

	if ((stm & __ipc_status(priv->idev->base, priv->mbox_channel)))
		is_stm = 1;

	return is_stm;
}

static mbox_msg_len_t hisi_mdev_receive_msg(struct hisi_mbox_device *mdev, mbox_msg_t **buf)
{
	mbox_msg_t *_buf = NULL;
	mbox_msg_len_t len = 0;

	if (hisi_mdev_is_stm(mdev, ACK_STATUS))
		_buf = mdev->ack_buffer;
	else
		_buf = mdev->rx_buffer;

	if (_buf)
		len = hisi_mdev_hw_read(mdev, _buf);
	*buf = _buf;

	hisi_mdev_clr_irq_and_ack(mdev);
	return len;
}

static int hisi_mdev_unlock(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	int retry = 3;

	do {
		__ipc_unlock(priv->idev->base, priv->idev->unlock);
		if (IPC_UNLOCKED == __ipc_lock_status(priv->idev->base))
			break;

		udelay(10);
		retry--;
	} while (retry);

	if (!retry)
		return -ENODEV;

	return 0;
}

static int hisi_mdev_occupy(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	int retry = 10;

	do {
		/*
		 * Hardware lock
		 * A hardware lock is needed here to lock a mailbox resource,
		 * which could be used by another remote proccessor, such as
		 * a HiIPCV230 common mailbox-25/mailbox-26.
		 */
		if (!(__ipc_status(priv->idev->base, priv->mbox_channel) & IDLE_STATUS)) {
			asm volatile ("wfe");
		} else {
			/*set the source processor bit, we set common mailbox's  source processor bit through dtsi */
			__ipc_set_src(priv->idev->base, priv->src, priv->mbox_channel);
			if (__ipc_read_src(priv->idev->base, priv->mbox_channel) & IPCBITMASK(priv->src))
				break;
		}
		retry--;
		/* Hardware unlock */
	} while (retry);

	if (!retry)
		return -ENODEV;

	return 0;
}

static int hisi_mdev_hw_send(struct hisi_mbox_device *mdev, mbox_msg_t *msg, mbox_msg_len_t len, int ack_mode)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	int i;
	unsigned int temp;

	/* interrupts unmask */
	__ipc_cpu_imask_all(priv->idev->base, priv->mbox_channel);

	if (AUTO_ACK == ack_mode)
		temp = IPCBITMASK(priv->des);
	else
		temp = IPCBITMASK(priv->src) | IPCBITMASK(priv->des);

	__ipc_cpu_imask_clr(priv->idev->base, temp, priv->mbox_channel);

	/* des config */

	__ipc_set_des(priv->idev->base, priv->des, priv->mbox_channel);

	/* ipc mode config */
	if (AUTO_ACK == ack_mode)
		temp = AUTOMATIC_ACK_CONFIG;
	else
		temp = NO_FUNC_CONFIG;

	__ipc_mode(priv->idev->base, temp, priv->mbox_channel);

	/* write data */
	for (i = 0; i < ((priv->capability < len) ? priv->capability : len); i++)
		__ipc_write(priv->idev->base, msg[i], priv->mbox_channel, i);

	/* enable sending */
	__ipc_send(priv->idev->base, IPCBITMASK(priv->src), priv->mbox_channel);
	return 0;
}

static void hisi_mdev_ensure_channel(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = NULL;
	int timeout = 0;
	int loop = 0;
	priv = mdev->priv;
	loop = priv->timeout / EVERY_LOOP_TIME_MS + MAILBOX_ASYNC_UDELAY_CNT;
	if (mdev->ops->is_stm(mdev, IDLE_STATUS)) {
		/*IDLE STATUS, return directly */
		return;
	}
	/*the ack status is reached, just release, the sync and async is mutexed by by mdev->dev_lock */
	else if (mdev->ops->is_stm(mdev, ACK_STATUS)) {
		/*ACK STATUS, release the channel directly */
		goto release;
	}
	/*DEST STATUS and SRC STATUS, the dest is processing, wait here */
	else {						/*if(mdev->ops->is_stm(mdev, DESTINATION_STATUS) || mdev->ops->is_stm(mdev, SOURCE_STATUS)) */
		/*the worst situation is to delay 1000*5us+60*5ms = 305ms */
		while (timeout < loop) {
			if (timeout < MAILBOX_ASYNC_UDELAY_CNT) {
				udelay(5);
			} else {
				/*the hifi may power off when send ipc msg, so the ack status may wait 20ms */
				usleep_range(3000, 5000);
				/*MDEV_ERR("mdev %s sleep 5ms, timeout = %d\n", mdev->name, timeout); */
			}
			/*if the ack status is ready, break out */
			if (mdev->ops->is_stm(mdev, ACK_STATUS)) {
				break;
			}
			timeout++;
		}

		if (unlikely(timeout == loop)) {
			MDEV_ERR("%s ipc_timeout...", mdev->name);

			if (mdev->ops->status)
				mdev->ops->status(mdev);
		}

		goto release;
	}

release:
	/*release the channel */
	mdev->ops->refresh(mdev);
}

static int hisi_mdev_send_msg(struct hisi_mbox_device *mdev, mbox_msg_t *msg, mbox_msg_len_t len, int ack_mode)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	int err = 0;
	/*all the mailbox channel is treated as fast-mailbox */
	if (DESTINATION_MBOX & priv->func) {
		MDEV_ERR("mdev %s has no tx ability", mdev->name);
		err = -EMDEVCLEAN;
		goto out;
	}

	/*
	 * Whenever an ipc starts,
	 * ipc module has to be unlocked at the very beginning.
	 */
	if (hisi_mdev_unlock(mdev)) {
		MDEV_ERR("mdev %s can not be unlocked", mdev->name);
		err = -EMDEVCLEAN;
		goto out;
	}

	if (hisi_mdev_occupy(mdev)) {
		MDEV_ERR("mdev %s can not be occupied", mdev->name);
		err = -EMDEVCLEAN;
		goto out;
	}

	(void)hisi_mdev_hw_send(mdev, msg, len, ack_mode);

out:
	return err;
}

static void hisi_mdev_release(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;

	__ipc_cpu_imask_all(priv->idev->base, priv->mbox_channel);
	__ipc_set_src(priv->idev->base, priv->src, priv->mbox_channel);

	asm volatile ("sev");
	return;
}

static unsigned int hisi_mdev_board_type(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	return (priv->hardware_board_type);
}

static int hisi_mdev_irq_request(struct hisi_mbox_device *mdev, irq_handler_t handler, void *p)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;
	int ret = 0;

	if (priv->idev->cmbox_info->cmbox_gic_1_irq == priv->irq) {
		if (!priv->idev->cmbox_info->gic_1_irq_requested++) {
			#if LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
			ret = request_irq(priv->irq, handler, IRQF_DISABLED, mdev->name, (void *)priv->idev->cmbox_info->cmdev);
			#else
			if(!strncmp("ao-mailbox", mdev->name, strlen("ao-mailbox"))) {
				ret = request_irq(priv->irq, handler, IRQF_NO_SUSPEND, mdev->name, (void *)priv->idev->cmbox_info->cmdev);
			} else if(!strncmp("mailbox-4", mdev->name, strlen("mailbox-4"))) {
				ret = request_irq(priv->irq, handler, IRQF_NO_SUSPEND, mdev->name, (void *)priv->idev->cmbox_info->cmdev);
			} else {
				ret = request_irq(priv->irq, handler, 0, mdev->name, (void *)priv->idev->cmbox_info->cmdev);
			}
			#endif
			if (ret) {
				MDEV_ERR("fast source %s request gic_1_irq %d failed", mdev->name, priv->irq);
				priv->idev->cmbox_info->gic_1_irq_requested--;
				goto out;
			}

			hisi_irqaffinity_register(priv->irq, IPC_IRQ_AFFINITY_CPU);
		}
	} else if (priv->idev->cmbox_info->cmbox_gic_2_irq == priv->irq) {
		if (!priv->idev->cmbox_info->gic_2_irq_requested++) {
			#if LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
			ret = request_irq(priv->irq, handler, IRQF_DISABLED, mdev->name, (void *)priv->idev->cmbox_info->cmdev);
			#else
			if(!strncmp("ao-mailbox", mdev->name, strlen("ao-mailbox"))) {
				ret = request_irq(priv->irq, handler, IRQF_NO_SUSPEND, mdev->name, (void *)priv->idev->cmbox_info->cmdev);
			} else if(!strncmp("mailbox-4", mdev->name, strlen("mailbox-4"))) {
				ret = request_irq(priv->irq, handler, IRQF_NO_SUSPEND, mdev->name, (void *)priv->idev->cmbox_info->cmdev);
			} else {
				ret = request_irq(priv->irq, handler, 0, mdev->name, (void *)priv->idev->cmbox_info->cmdev);
			}
			#endif
			if (ret) {
				MDEV_ERR("fast source %s request gic_2_irq %d failed", mdev->name, priv->irq);
				priv->idev->cmbox_info->gic_2_irq_requested--;
				goto out;
			}

			hisi_irqaffinity_register(priv->irq, IPC_IRQ_AFFINITY_CPU);
		}
	} else {
		#if LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
		ret = request_irq(priv->irq, handler, IRQF_DISABLED, mdev->name, p);
		#else
			if(!strncmp("ao-mailbox", mdev->name, strlen("ao-mailbox"))) {
				ret = request_irq(priv->irq, handler, IRQF_NO_SUSPEND, mdev->name, p);
			} else if(!strncmp("mailbox-4", mdev->name, strlen("mailbox-4"))) {
				ret = request_irq(priv->irq, handler, IRQF_NO_SUSPEND, mdev->name, p);
			} else {
				ret = request_irq(priv->irq, handler, 0, mdev->name, p);
			}
		#endif
		if (ret) {
			MDEV_ERR("fast desitnation %s request irq %d failed", mdev->name, priv->irq);
			goto out;
		}

		hisi_irqaffinity_register(priv->irq, IPC_IRQ_AFFINITY_CPU);
	}

out:
	return ret;
}

static void hisi_mdev_irq_free(struct hisi_mbox_device *mdev, void *p)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;

	if (priv->idev->cmbox_info->cmbox_gic_1_irq == priv->irq) {
		if (!--priv->idev->cmbox_info->gic_1_irq_requested)
			free_irq(priv->irq, (void *)priv->idev->cmbox_info->cmdev);
	} else if (priv->idev->cmbox_info->cmbox_gic_2_irq == priv->irq) {
		if (!--priv->idev->cmbox_info->gic_2_irq_requested)
			free_irq(priv->irq, (void *)priv->idev->cmbox_info->cmdev);
	} else {
		free_irq(priv->irq, p);
	}

	return;
}

static void hisi_mdev_irq_enable(struct hisi_mbox_device *mdev)
{
	enable_irq((unsigned int)mdev->cur_irq);
}

static void hisi_mdev_irq_disable(struct hisi_mbox_device *mdev)
{
	disable_irq_nosync((unsigned int)mdev->cur_irq);
}

static struct hisi_mbox_device *hisi_mdev_irq_to_mdev(struct hisi_mbox_device *_mdev, struct list_head *list, int irq)
{
	struct hisi_mbox_device_priv *_priv = NULL;
	struct hisi_mbox_device *mdev = NULL;
	struct hisi_mbox_device_priv *priv = NULL;
	remote_processor_type_t src = UNCERTAIN_REMOTE_PROCESSOR;
	unsigned int regval = 0x0;

	if ((list_empty(list)) || (NULL == _mdev)) {
		MDEV_ERR("invalid input");
		goto out;
	}

	_priv = _mdev->priv;

	/* fast destination mailboxes use unique irq number */
	if ((DESTINATION_MBOX & _priv->func) && (FAST_MBOX & _priv->func)) {
		mdev = _mdev;
		goto out;
	}

	if (NULL != strstr(_mdev->name, "npu-mailbox")) {
		src = NPU_IPC_GIC;
	}
	else if (NULL != strstr(_mdev->name, "ao")) {
		src = GIC_2;
	} else {
		/* fast source & common mailboxes share GIC_1 & GIC_2 irq number */
		if (irq == _priv->idev->cmbox_info->cmbox_gic_1_irq) {
			src = GIC_1;
		} else if (irq == _priv->idev->cmbox_info->cmbox_gic_2_irq) {
			src = GIC_2;
		} else {
			MDEV_ERR("odd irq for hisi mailboxes");
			goto out;
		}
	}

	regval = __ipc_mbox_istatus(_priv->idev->base, src);
	if (0 == regval) {
		mdev = NULL;
		goto out;
	}
	list_for_each_entry(mdev, list, node) {
		priv = mdev->priv;

		if ((regval & IPCBITMASK(priv->mbox_channel)) && (priv->func & SOURCE_MBOX))
			goto out;
	}

out:
	/* it is nearly occured */
	return mdev;
}

static unsigned int hisi_mdev_timeout(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;/*lint !e838 */
	return priv->timeout;
}

static unsigned int hisi_mdev_fifo_size(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;/*lint !e838 */
	return priv->fifo_size;
}

static unsigned int hisi_mdev_sched_priority(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;/*lint !e838 */
	return priv->sched_priority;
}

static unsigned int hisi_mdev_sched_policy(struct hisi_mbox_device *mdev)
{
	struct hisi_mbox_device_priv *priv = mdev->priv;/*lint !e838 */
	return priv->sched_policy;
}

struct hisi_mbox_dev_ops hisi_mdev_ops = {
	.startup = hisi_mdev_startup,
	.shutdown = hisi_mdev_shutdown,
	.check = hisi_mdev_check,
	.recv = hisi_mdev_receive_msg,
	.send = hisi_mdev_send_msg,
	.ack = hisi_mdev_ack,
	.refresh = hisi_mdev_release,

	.get_timeout= hisi_mdev_timeout,
	.get_fifo_size = hisi_mdev_fifo_size,
	.get_sched_priority = hisi_mdev_sched_priority,
	.get_sched_policy = hisi_mdev_sched_policy,
	.read_board_type = hisi_mdev_board_type,
	.request_irq = hisi_mdev_irq_request,
	.free_irq = hisi_mdev_irq_free,
	.enable_irq = hisi_mdev_irq_enable,
	.disable_irq = hisi_mdev_irq_disable,
	.irq_to_mdev = hisi_mdev_irq_to_mdev,
	.is_stm = hisi_mdev_is_stm,
	.clr_ack = hisi_mdev_clr_ack,
	.ensure_channel = hisi_mdev_ensure_channel,
	.status = hisi_mdev_dump_status,
	.dump_regs = hisi_mdev_dump_regs,
};

static void hisi_mdev_put(struct hisi_ipc_device *idev)
{
	struct hisi_mbox_device **list = idev->mdev_res;
	struct hisi_mbox_device *mdev = NULL;
	int i;

	iounmap(idev->base);

	kfree(idev->cmbox_info);
	kfree(idev->buf_pool);

	for (i = 0; (mdev = list[i]); i++) {
		kfree(mdev->priv);
		kfree(mdev);
	}

	return;
}

static int hisi_mdev_remove(struct platform_device *pdev)
{
	struct hisi_ipc_device *idev = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	if (idev) {
		hisi_mbox_device_unregister(idev->mdev_res);
		hisi_mdev_put(idev);
		kfree(idev);
	}

	return 0;
}

static int hisi_mdev_get(struct hisi_ipc_device *idev, struct hisi_mbox_device **mdevs, struct device_node *node)
{
	struct device_node *son = NULL;
	struct hisi_common_mbox_info *cmbox_info = NULL;
	struct hisi_mbox_device *mdev;
	struct hisi_mbox_device_priv *priv;
	remote_processor_type_t src_bit;
	remote_processor_type_t des_bit;
	mbox_msg_t *buf_pool = NULL;
	mbox_msg_len_t buf_pool_len = 0;
	const char *mdev_name = NULL;
	mbox_msg_t *rx_buffer = NULL;
	mbox_msg_t *ack_buffer = NULL;
	u8 func = 0;
	u32 output[3] = { 0 };
	int irq = 0;
	int i = 0;
	int mbox_channel;
	int mdev_index_temp = 0;/* some mailbox is not used */
	int ret = 0;
	unsigned int used = 0;
	int cm_gic_1_irq = -1;
	int cm_gic_2_irq = -1;
	int capability = 0;
	unsigned int hardware_board_type = 0;
	u32 unlock = 0;
	int mdev_num = 0;
	unsigned int timeout;
	unsigned int fifo_size;
	unsigned int sched_priority;
	unsigned int sched_policy;
	void __iomem *ipc_base = NULL;
	ipc_base = of_iomap(node, 0);
	if (!ipc_base) {
		MDEV_ERR("iomap error");
		ret = -ENOMEM;
		goto out;
	}

	MDEV_DEBUG("ipc_base: 0x%lx", (unsigned long)ipc_base);

	ret = of_property_read_u32(node, "capability", (u32 *)&capability);
	if (ret) {
		MDEV_ERR("prop \"capability\" error %d", ret);
		ret = -ENODEV;
		goto to_iounmap;
	}

	MDEV_DEBUG("capability: %d", (int)capability);

	ret = of_property_read_u32(node, "hardware_board_type", &hardware_board_type);
	if(ret) {
		MDEV_DEBUG("hardware_board_type: %d, it's not UDP & FPGA", (int)hardware_board_type);
		hardware_board_type = IPC_DEFAULT_BOARD_TYPE;
	}

	MDEV_DEBUG("hardware_board_type: %d", (int)hardware_board_type);

	ret = of_property_read_u32(node, "unlock_key", &unlock);
	if (ret) {
		MDEV_ERR("prop \"key\" error %d", ret);
		ret = -ENODEV;
		goto to_iounmap;
	}

	MDEV_DEBUG("unlock_key: 0x%x", (unsigned int)unlock);
	ret = of_property_read_u32(node, "mailboxes", (u32 *)&mdev_num);
	if (ret) {
		MDEV_ERR("prop \"mailboxes\" error %d", ret);
		ret = -ENODEV;
		goto to_iounmap;
	}

	MDEV_DEBUG("mailboxes: %d", (int)mdev_num);
	cmbox_info = kzalloc(sizeof(*cmbox_info), GFP_KERNEL);
	if (!cmbox_info) {
		ret = -ENOMEM;
		goto to_iounmap;
	}

	buf_pool_len = capability * MBOX_BUFFER_TYPE_MAX * mdev_num;
	buf_pool = kzalloc(sizeof(mbox_msg_t) * buf_pool_len, GFP_KERNEL);
	if (!buf_pool) {
		ret = -ENOMEM;
		goto free_cmbox;
	}

	MDEV_DEBUG("buffer pool: 0x%lx", (unsigned long)buf_pool);

	cm_gic_1_irq = irq_of_parse_and_map(node, 0);
	cm_gic_2_irq = irq_of_parse_and_map(node, 1);

	cmbox_info->gic_1_irq_requested = 0;
	cmbox_info->gic_2_irq_requested = 0;
	cmbox_info->cmbox_gic_1_irq = cm_gic_1_irq;
	cmbox_info->cmbox_gic_2_irq = cm_gic_2_irq;
	cmbox_info->cmdev = NULL;

	idev->cmbox_info = cmbox_info;
	idev->unlock = unlock;
	idev->base = ipc_base;
	idev->mdev_res = mdevs;
	idev->buf_pool = buf_pool;

	for (i = 0; (son = of_get_next_child(node, son)); i++) {/*lint !e441*/
		mdev = NULL;
		priv = NULL;
		mdev_name = NULL;
		func = 0;
		mbox_channel = -1;
		rx_buffer = NULL;
		ack_buffer = NULL;
		used = 0;

		ret = of_property_read_u32(son, "used", &used);
		if(ret) {
			MDEV_ERR("mailbox-%d has no tag <used>", mdev_index_temp);
			goto to_break;
		}
		if (MAILBOX_NO_USED == used) {
			MDEV_DEBUG("mailbox node %s is not used", son->name);
			continue;
		}

		mdev = kzalloc(sizeof(*mdev), GFP_KERNEL);
		if (!mdev) {
			ret = -ENOMEM;
			goto to_break;
		}

		priv = kzalloc(sizeof(*priv), GFP_KERNEL);
		if (!priv) {
			ret = -ENOMEM;
			goto free_mdev;
		}

		mdev_name = son->name;

		MDEV_DEBUG("mailbox node: %s", mdev_name);

		ret = of_property_read_u32(son, "src_bit", (u32 *)&src_bit);
		if (ret)
			goto free_priv;

		ret = of_property_read_u32(son, "des_bit", (u32 *)&des_bit);
		if (ret)
			goto free_priv;
		/* get software code-index to mbox_channel and calculate the right mbox_channel */
		ret = of_property_read_u32(son, "index", (u32 *)&mbox_channel);
		if (ret)
			goto free_priv;

		MDEV_DEBUG("index: %d", (int)mbox_channel);
		/* to distinguish different ipc and calculate the true mailbox-index */
		if(MAX_AP_IPC_INDEX < mbox_channel)
			mbox_channel = mbox_channel % 100;

		ret = of_property_read_u32(son, "timeout", &timeout);
		if (ret || 0 != timeout % EVERY_LOOP_TIME_MS)
			timeout = DEFAULT_MAILBOX_TIMEOUT;

		MDEV_DEBUG("timeout: %d", (int)timeout);

		ret = of_property_read_u32(son, "fifo_size", &fifo_size);
		if (ret)
			fifo_size = DEFAULT_FIFO_SIZE;

		MDEV_DEBUG("fifo_size: %d", (int)fifo_size);

		ret = of_property_read_u32(son, "sched_priority", &sched_priority);
		if (ret)
			sched_priority = DEFAULT_SCHED_PRIORITY;

		MDEV_DEBUG("sched_priority: %d", (int)sched_priority);

		ret = of_property_read_u32(son, "sched_policy", &sched_policy);
		if (ret)
			sched_policy = SCHED_RR;/* default sched_policy is SCHED_RR */

		MDEV_DEBUG("sched_policy: %d", (int)sched_policy);

		ret = of_property_read_u32_array(son, "func", output, 3);
		if (ret)
			goto free_priv;

		func |= (output[0] ? FAST_MBOX : COMM_MBOX);

		func |= (output[1] ? SOURCE_MBOX : 0);

		func |= (output[2] ? DESTINATION_MBOX : 0);

		if ((FAST_MBOX & func) && (DESTINATION_MBOX & func)) {
			MDEV_DEBUG("func FAST DES MBOX");
			irq = irq_of_parse_and_map(son, 0);
			MDEV_DEBUG("irq: %d", (int)irq);
		} else if ((FAST_MBOX & func) && (SOURCE_MBOX & func)) {
			MDEV_DEBUG("func FAST SRC MBOX");
			irq = (GIC_1 == src_bit) ? cm_gic_1_irq : cm_gic_2_irq;
			MDEV_DEBUG("irq: %d", (int)irq);
			/*set the cmdev, the cmdev will be used in acore't interrupts */
			if (NULL == cmbox_info->cmdev)
				cmbox_info->cmdev = mdev;
		} else {
			/* maybe GIC_1 OR GIC_2 */
			MDEV_DEBUG(" xxxxxxxxx we don't use comm-mailbox , we use it as fast-mailbox");
			/*we don't use comm-mailbox , we use it as fast-mailbox, please set the comm to fast in the dtsi */
			irq = COMM_MBOX_IRQ;
			cmbox_info->cmdev = mdev;
			MDEV_DEBUG("irq: %d\n", (int)irq);
		}

		rx_buffer = buf_pool + capability * RX_BUFFER_TYPE;
		ack_buffer = buf_pool + capability * ACK_BUFFER_TYPE;
		buf_pool = buf_pool + capability * MBOX_BUFFER_TYPE_MAX;/*lint !e679*/
		MDEV_DEBUG("rx_buffer: 0x%lx\nack_buffer: 0x%lx", (unsigned long)rx_buffer, (unsigned long)ack_buffer);

		priv->capability = capability;
		priv->hardware_board_type = hardware_board_type;
		priv->func = func;
		priv->src = src_bit;
		priv->des = des_bit;
		priv->irq = irq;
		priv->mbox_channel = mbox_channel;
		priv->idev = idev;
		priv->used = used;
		priv->timeout = timeout;
		priv->fifo_size = fifo_size;
		priv->sched_priority = sched_priority;
		priv->sched_policy = sched_policy;

		mdev->name = mdev_name;
		mdev->priv = priv;
		mdev->rx_buffer = rx_buffer;
		mdev->ack_buffer = ack_buffer;
		mdev->ops = &hisi_mdev_ops;

		mdevs[mdev_index_temp] = mdev;
		mdev_index_temp++;
		continue;
free_priv:
		kfree(priv);
free_mdev:
		kfree(mdev);
to_break:
		break;
	}

	if (ret)
		goto deinit_mdevs;

	return ret;

deinit_mdevs:
	while (i--) {
		kfree(mdevs[i]->priv);
		kfree(mdevs[i]);
	}

	kfree(idev->buf_pool);
free_cmbox:
	kfree(cmbox_info);
to_iounmap:
	iounmap(ipc_base);
out:
	return ret;
}

static int hisi_mdev_probe(struct platform_device *pdev)
{
	struct hisi_ipc_device *idev = NULL;
	struct hisi_mbox_device **mdev_res = NULL;
	struct device_node *node = pdev->dev.of_node;
	int mdev_num = 0;
	int ret = 0;

	if (!node) {
		MDEV_ERR("dts[%s] node not found", "hisilicon,HiIPCV230");
		ret = -ENODEV;
		goto out;
	}

	idev = kzalloc(sizeof(*idev), GFP_KERNEL);
	if (!idev) {
		MDEV_ERR("no mem for ipc resouce");
		ret = -ENOMEM;
		goto out;
	}

	ret = of_property_read_u32(node, "mailboxes", (u32 *)&mdev_num);
	if (ret) {
		MDEV_ERR("no mailboxes resources");
		ret = -ENODEV;
		goto free_idev;
	}

	mdev_res = kzalloc((sizeof(*mdev_res) * (mdev_num + 1)), GFP_KERNEL);
	if (!mdev_res) {
		ret = -ENOMEM;
		goto free_idev;
	}
	mdev_res[mdev_num] = NULL;

	ret = hisi_mdev_get(idev, mdev_res, node);
	if (ret) {
		MDEV_ERR("can not get ipc resource");
		ret = -ENODEV;
		goto free_mdevs;
	}

	ret = hisi_mbox_device_register(&pdev->dev, mdev_res);
	if (ret) {
		MDEV_ERR("mdevs register failed");
		ret = -ENODEV;
		goto put_res;
	}

	platform_set_drvdata(pdev, idev);

	MDEV_DEBUG("HiIPCV230 mailboxes are ready");

	hisi_rproc_init();			/*we call it here to let the pl011_init can use the rproc send function  */

	return 0;

put_res:
	hisi_mdev_put(idev);
free_mdevs:
	kfree(idev->mdev_res);
free_idev:
	kfree(idev);
out:
	return ret;
}

static int hisi_mdev_suspend(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct hisi_ipc_device *idev = platform_get_drvdata(pdev);

	MDEV_INFO("%s: suspend +", __func__);
	if (idev)
		hisi_mbox_device_deactivate(idev->mdev_res);
	MDEV_INFO("%s: suspend -", __func__);
	return 0;
}

static int hisi_mdev_resume(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct hisi_ipc_device *idev = platform_get_drvdata(pdev);

	MDEV_INFO("%s: resume +", __func__);
	if (idev)
		hisi_mbox_device_activate(idev->mdev_res);
	MDEV_INFO("%s: resume -", __func__);
	return 0;
}

static const struct of_device_id hisi_mdev_of_match[] = {
	{.compatible = "hisilicon,HiIPCV230",},
	{},
};

MODULE_DEVICE_TABLE(of, hisi_mdev_of_match);

static const struct dev_pm_ops hisi_mdev_pm_ops = {
	.suspend_late = hisi_mdev_suspend,
	.resume_early = hisi_mdev_resume,
};

static struct platform_driver hisi_mdev_driver = {
	.probe = hisi_mdev_probe,
	.remove = hisi_mdev_remove,
	.driver = {
			   .name = "HiIPCV230-mailbox",
			   .owner = THIS_MODULE,
			   .of_match_table = of_match_ptr(hisi_mdev_of_match),
			   .pm = &hisi_mdev_pm_ops,
		},
};

static int __init hisi_mdev_init(void)
{
	MDEV_ERR("%s init!", __func__);

	platform_driver_register(&hisi_mdev_driver);
	return 0;
}

core_initcall(hisi_mdev_init);

static void __exit hisi_mdev_exit(void)
{
	platform_driver_unregister(&hisi_mdev_driver);
	return;
}

module_exit(hisi_mdev_exit);

MODULE_DESCRIPTION("HiIPCV230 ipc, mailbox device driver");
MODULE_LICENSE("GPL V2");
