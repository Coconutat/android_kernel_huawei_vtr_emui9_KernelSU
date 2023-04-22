
#ifndef __OAL_PCIE_EDMA_H__
#define __OAL_PCIE_EDMA_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#include "oal_util.h"

/******************************************************************************
 *
 *  MACRO DEFINITIONS
 *
 ******************************************************************************/
#define PCIE_CTRL_IP_HI1103_4_70A   (0x001)
#define PCIE_CTRL_IP_HI1103_5_00A   (0x002)

#ifdef _PRE_PILOT
#define PCIE_CTRL_IP_VERSION    PCIE_CTRL_IP_HI1103_5_00A
#else
#define PCIE_CTRL_IP_VERSION    PCIE_CTRL_IP_HI1103_4_70A
#endif

#define PCIE_CONFIG_BASE_ADDRESS	(0x40102000) /*pcie config base address,4KB, 4.70a*/

#if (PCIE_CTRL_IP_VERSION == PCIE_CTRL_IP_HI1103_4_70A)
/*Hi1103 4.70a pcie dma address*/
#define PCIE_LOGIC_DMA_ADDRESS      (PCIE_CONFIG_BASE_ADDRESS+0x970) 
#elif (PCIE_CTRL_IP_VERSION == PCIE_CTRL_IP_HI1103_5_00A)
/*Hi1103 5.00a pcie dma address*/
#define PCIE_LOGIC_DMA_ADDRESS      (0x40106000)
#else
#error unknow pcie version
#endif

#define PCIE_GET_CONFIG_ADDR(offset)	(PCIE_CONFIG_BASE_ADDRESS + offset)
#define PCIE_GET_EDMA_ADDR(offset)	    (PCIE_LOGIC_DMA_ADDRESS + offset)

/*eDMA Features, 2 channels for fifo transfer*/
#define PCIE_EDMA_MAX_CHANNELS      (2)


/******************************************************************************
 *
 *  DATA STRUCTURE DEFINITIONS
 *
 ******************************************************************************/
/**
 * Physcial address.
 */
#ifndef TRUE
#define TRUE    (1)
#define FALSE   (0)
#endif

/*Debug Macro.*/

/*0xa6c  DMA_VIEWPORT_SEL_OFF Call-back Debug*/
#define PCIE_EDMA_VIEWPORT_REG_CALL_BACK    (1)

/*无效的element首地址，此地址压入FIFO，该通道不启动*/
#define PCIE_EDMA_INVAILD_CHANN_ADDR    (0xFFFFFFFFUL) 

/*新增硬化逻辑FIFO操作寄存器,WCPU看到的地址*/
#define PCIE_DMA_CTRL_BASE_ADDR                (0x40008000)/*对应PCIE_DMA_CTRL页*/
#define PCIE_CTRL_BASE_ADDR                    (0x40007000)/*对应PCIE_CTRL页*/

#define PCIE_REVISION_4_70A     (0x1)
#define PCIE_REVISION_5_00A     (0x2)

/*PCIE_CTRL_BASE_ADDR*/
/*MSG FIFO*/
#define PCIE_MSG_FIFO_STATUS            (0x478)
#define PCIE_MSG_FIFO_STATUS_IDLE       (0xaaaa)

#define PCIE_AUX_CLK_FREQ_OFF           (0xb40)

#define PCIE_MSG_MODE                      (0x204)
#define PCIE_LOW_POWER_CFG_OFF             (0x220)
#define PCIE_PCIE_STATUS0                  (0x224)
#define PCIE_MSG_INTR_OFF                  (0x2A8)
#define PCIE_MSG_INTR_STATUS_OFF           (0x2AC)
#define PCIE_MSG_INTR_CLR_OFF              (0x2B0)
#define PCIE_MSG_INTR_MASK_OFF             (0x2B4)
#define PCIE_CFG_OFF                       (0x2D0)
#define PCIE_HOST_DEVICE_REG0              (0x2E0)
#define PCIE_HOST_DEVICE_REG1              (0x2E4)
#define PCIE_HOST_INTR_MASK_OFF            (0x2E8)
#define PCIE_HOST_INTR_STATUS_OFF          (0x2EC)
#define PCIE_HOST_INTR_CLR                 (0x2F0)
#define PCIE_STATUS2_OFF                   (0x43C)
#define PCIE_TX_ABORT_STATUS_OFF           (0x47C)
#define PCIE_RX_ABORT_STATUS_OFF           (0x480)
#define PCIE_INTR_RAW_STATUS_OFF           (0x484)
#define PCIE_STAT_CNT_LTSSM_ENTER_RCVRY_OFF     (0x498)
#define PCIE_STAT_CNT_L1_ENTER_RCVRY_OFF        (0x49C)

/*Port Logic Reg*/
#define PCIE_ACK_F_ASPM_CTRL_OFF            (0x70c)


#define PCIE_H2D_DOORBELL_OFF                       (0x2D4)
#define PCIE_D2H_DOORBELL_OFF                       (0x2D4)
#define PCIE_H2D_TRIGGER_VALUE                      (1 << 4)
#define PCIE_H2D_DOORBELL_TRIGGER_VALUE             (1 << 5)
#define PCIE_D2H_DOORBELL_TRIGGER_VALUE             (1 << 6)

/*Count必须读，address为可选*/
/*remote*/
#define PCIE_FIFO_REMOTE_READ_FIFO0_DATA_OFF     (0x818)/*tx*/
#define PCIE_FIFO_REMOTE_READ_FIFO1_DATA_OFF     (0x824)/*tx*/
#define PCIE_FIFO_REMOTE_WRITE_FIFO0_DATA_OFF    (0x800)/*rx*/
#define PCIE_FIFO_REMOTE_WRITE_FIFO1_DATA_OFF    (0x80c)/*rx*/

/*local*/
#define PCIE_FIFO_LOCAL_READ_FIFO0_DATA_OFF     (0x840)/*tx*/
#define PCIE_FIFO_LOCAL_READ_FIFO1_DATA_OFF     (0x848)/*tx*/
#define PCIE_FIFO_LOCAL_WRITE_FIFO0_DATA_OFF    (0x830)/*rx*/
#define PCIE_FIFO_LOCAL_WRITE_FIFO1_DATA_OFF    (0x838)/*rx*/

/*PCIE_DMA_CTRL_BASE_ADDR*/
#define PCIE_SYS_CTL_ID_OFF                  (0x000)
#define PCIE_DMA_RX_LL_CHANLE0_HIGH_ADDR     (0x004)
#define PCIE_DMA_RX_LL_CHANLE1_HIGH_ADDR     (0x008)
#define PCIE_DMA_TX_LL_CHANLE0_HIGH_ADDR     (0x00C)
#define PCIE_DMA_TX_LL_CHANLE1_HIGH_ADDR     (0x010)
#define PCIE_DMA_RX_LL_CHANEL0_CTL_DATA      (0x014)
#define PCIE_DMA_RX_LL_CHANEL1_CTL_DATA      (0x018)
#define PCIE_DMA_TX_LL_CHANEL0_CTL_DATA      (0x01C)
#define PCIE_DMA_TX_LL_CHANEL1_CTL_DATA      (0x020)
#define PCIE_CFG_BASE_ADDR                   (0x024)

/*DMA LLT FIFO, 启动DMA*/
#define PCIE_CFG_BASE_ADD_OFF                (0x024)
#define PCIE_LLT_FIFO_STATUS                 (0x028)
#define PCIE_DMA_WRITE_FIFO0_LLT_OFF         (0x400)
#define PCIE_DMA_WRITE_FIFO1_LLT_OFF         (0x404)
#define PCIE_DMA_READ_FIFO0_LLT_OFF          (0x408)
#define PCIE_DMA_READ_FIFO1_LLT_OFF          (0x40C)
typedef union {
    struct {
        oal_uint32 LowPart;
        oal_uint32 HighPart;
    }bits;
    oal_uint64 QuadPart;
} PHYADDR;

/**
 * The direction of DMA transfer.
 */
typedef enum {
    DMA_WRITE = 0,  /**< Write /ep->rc*/
    DMA_READ = 1,        /**< Read rc->ep*/
    DMA_BUTT = 2
} DMA_DIRC;

/**
 * DMA transfer mode.
 */
typedef enum {
    DMA_SINGLE_BLOCK = 0,/**< Single block mode */
    DMA_MULTI_BLOCK      /**< Multi block mode */
} DMA_XFER_MODE;

/**
 * DMA interrupt type.
 */
typedef enum {
    DMA_INT_DONE = 0,   /**< DONE interrupt */
    DMA_INT_ABORT       /**< ABORT interrupt */
} DMA_INT_TYPE;

/**
 * DMA Channel status.
 */
typedef enum {
	CHAN_UNKNOWN = 0,   /**< Unknown */
    CHAN_RUNNING,       /**< Channel is running */
    CHAN_HALTED,        /**< Channel is halted */
    CHAN_STOPPED,       /**< Channel is stopped */
    CHAN_QUEUING	    /**< Queuing. Not a real HW status */
} DMA_CHAN_STATUS;

/**
 * DMA hardware error types.
 */
typedef enum {
    DMA_ERR_NONE,       /**< No DMA error found */
    DMA_ERR_WR,         /**< The DMA Write Channel has received an error
                         *   response from the AHB/AXI bus (or RTRGT1 interface
                         *   when the AHB/AXI Bridge is not used) while reading
                         *   data from it. It's fatal error. */
    DMA_ERR_RD,         /**< The DMA Read Channel has received an error response
                         *   from the AHB/AXI bus (or RTRGT1 interface when the
                         *   AHB/AXI Bridge is not used) while writing data to
                         *   it. It's fatal error.*/
    DMA_ERR_FETCH_LL,   /**< The DMA Write/Read Channel has received an error
                         *   response from the AHB/AXI bus (or RTRGT1 interface
                         *   when the AHB/AXI Bridge is not used) while reading
                         *   a Linked List Element from local memory. It's fatal
                         *   error. */
    DMA_ERR_UNSUPPORTED_RQST,
                        /**< The DMA Read Channel has received a PCIe
                         *   Unsupported Request CPL status from the remote
                         *   device in response to the MRd Request.*/
    DMA_ERR_COMPLETER_ABORT,
                        /**< The DMA Read Channel has received a PCIe Completer
                         *  Abort CPL status from the remote device in response
                         *  to the MRd Request. Non-fatal error.
                         */
    DMA_ERR_CPL_TIME_OUT,
                        /**< The DMA Read Channel has timed-out while waiting
                         * for the remote device to respond to the MRd Request,
                         * or a malformed CplD has been received. Non-fatal
                         * error. */
    DMA_ERR_DATA_POISONING,
                        /**< The DMA Read Channel has detected data poisoning
                         * in the CPL from the remote device in response to the
                         * MRd Request. Non-fatal error. */
    DMA_ERR_PCIE_BUTT
} DMA_ERR;

/******************************************************************************
 *
 *  DATA STRUCTURE DECLARATIONS
 *
 ******************************************************************************/
/**
 * The Channel Control Register for read and write.
 */
typedef union { 
    struct {
        //LSB
        oal_uint32      CB          :1;    // 0
        oal_uint32      TCB         :1;    // 1
        oal_uint32      LLP         :1;    // 2
        oal_uint32      LIE         :1;    // 3
        oal_uint32      RIE         :1;    // 4
        oal_uint32      CS          :2;    // 5:6
        oal_uint32      Rsvd1       :1;    // 7
        oal_uint32      CCS         :1;    // 8
        oal_uint32      LLEN        :1;    // 9
        oal_uint32      b_64S       :1;    // 10
        oal_uint32      b_64D       :1;    // 11
        oal_uint32      PF          :5;    // 12:16
        oal_uint32      Rsvd2       :7;    // 17:23
        oal_uint32      SN          :1;    // 24
        oal_uint32      RO          :1;    // 25
        oal_uint32      TD          :1;    // 26
        oal_uint32      TC          :3;    // 27:29
        oal_uint32      AT          :2;    // 30:31
        //MSB
    }bits;
    oal_uint32 AsDword;
} CHAN_CTRL_LO;

/*PCIe msg*/
typedef union {
    struct {
        oal_uint32 soc_owrd_wake_err_status:1;/*0*/
        oal_uint32 soc_owrd_wake_cpu_act_status:1;/*1*/
        oal_uint32 soc_owrd_wake_obff_status:1;/*2*/
        oal_uint32 soc_owrd_wake_idle_status:1;/*3*/
        oal_uint32 soc_pcie_send_f_err_status:1;/*4*/
        oal_uint32 soc_pcie_send_nf_err_status:1;/*5*/
        oal_uint32 soc_pcie_send_cor_err_status:1;/*6*/
        oal_uint32 soc_radm_msg_cpu_active_status:1;/*7*/
        oal_uint32 soc_radm_msg_idle_status:1;/*8*/
        oal_uint32 soc_radm_msg_obff_status:1;/*9*/
        oal_uint32 soc_radm_msg_turnoff_status:1;/*10*/
        oal_uint32 soc_radm_msg_unlock_status:1;/*11*/
        oal_uint32 soc_radm_msg_ltr_status:1;/*12*/
        oal_uint32 soc_radm_vendor_msg_status:1;/*13*/
        oal_uint32 link_down_irq:1;/*14*/
        oal_uint32 reserved:17;/*15:31*/
    }bits;
    oal_uint32 AsDword;
}PCIE_MSG_INTR_STATUS;

typedef union {
    struct {
        oal_uint32 soc_owrd_wake_err_mask:1;/*0*/
        oal_uint32 soc_owrd_wake_cpu_act_mask:1;/*1*/
        oal_uint32 soc_owrd_wake_obff_mask:1;/*2*/
        oal_uint32 soc_owrd_wake_idle_mask:1;/*3*/
        oal_uint32 soc_pcie_send_f_err_mask:1;/*4*/
        oal_uint32 soc_pcie_send_nf_err_mask:1;/*5*/
        oal_uint32 soc_pcie_send_cor_err_mask:1;/*6*/
        oal_uint32 soc_radm_msg_cpu_active_mask:1;/*7*/
        oal_uint32 soc_radm_msg_idle_mask:1;/*8*/
        oal_uint32 soc_radm_msg_obff_mask:1;/*9*/
        oal_uint32 soc_radm_msg_turnoff_mask:1;/*10*/
        oal_uint32 soc_radm_msg_unlock_mask:1;/*11*/
        oal_uint32 soc_radm_msg_ltr_mask:1;/*12*/
        oal_uint32 soc_radm_vendor_msg_mask:1;/*13*/
        oal_uint32 reserved:18;/*14:31*/
    }bits;
    oal_uint32 AsDword;
}PCIE_MSG_INTR_MASK;

/*soc dma int mask*/
typedef union {
    struct {
        oal_uint32 pcie_edma_tx_intr_mask:1;/*0*/
        oal_uint32 pcie_edma_rx_intr_mask:1;/*1*/
        oal_uint32 pcie_hw_edma_tx_intr_mask:1;/*2*/
        oal_uint32 pcie_hw_edma_rx_intr_mask:1;/*3*/
        oal_uint32 device2host_intr_mask:1;/*4*/
        oal_uint32 link_down_intr_mask:1;/*5*/
        oal_uint32 pcie_msg_irq_mask:1;/*6*/
        oal_uint32 host2device_intr_mask:1;/*7*/
        oal_uint32 host2device_tx_doorbell_intr_mask:1; /*8*/
        oal_uint32 host2device_rx_doorbell_intr_mask:1; /*9*/
        oal_uint32 pcie_device_hw_edma_tx_intr_mask:1;  /*10*/
        oal_uint32 pcie_device_hw_edma_rx_intr_mask:1;  /*11*/
        oal_uint32 pcie_device_hw_edma_tx_abort_mask:1; /*12*/
        oal_uint32 pcie_device_hw_edma_rx_abort_mask:1; /*13*/
        oal_uint32 pcie_edma_rxch01_intr_mask:1;        /*14*/
        oal_uint32 pcie_edma_txch01_intr_mask:1;        /*15*/
        oal_uint32 reserved:16;/*16:31*/
    }bits;
    oal_uint32 AsDword;
} HOST_INTR_MASK;

/*状态位和中断清除位必须一一对应*/
typedef union {
    struct {
        oal_uint32 pcie_edma_tx_intr_status:1;/*0*/
        oal_uint32 pcie_edma_rx_intr_status:1;/*1*/
        oal_uint32 pcie_hw_edma_tx_intr_status:1;/*2*/
        oal_uint32 pcie_hw_edma_rx_intr_status:1;/*3*/
        oal_uint32 host2device_intr_status:1;/*4*/
        oal_uint32 host2device_tx_doorbell_intr_status:1;/*5*/
        oal_uint32 host2device_rx_doorbell_intr_status:1;/*6*/
        oal_uint32 pcie_device_hw_edma_tx_intr_status:1;/*7*/
        oal_uint32 pcie_device_hw_edma_rx_intr_status:1;/*8*/
        oal_uint32 pcie_device_hw_edma_tx_abort_status:1;/*9*/
        oal_uint32 pcie_device_hw_edma_rx_abort_status:1;/*10*/
        oal_uint32 reserved1:21;/*11:31*/
    }bits;
    oal_uint32 AsDword;
} HOST_INTR_STATUS;

typedef union {
    struct {
        oal_uint32 tx_app_wr_err:2;/*0~1*/
        oal_uint32 reserved1:2;/*2~3*/
        oal_uint32 tx_ll_fetch_err:2;/*4~5*/
        oal_uint32 reserved2:2;/*6~7*/
        oal_uint32 tx_unsupport_err:2;/*8~9*/
        oal_uint32 reserved3:2;/*10~11*/
        oal_uint32 tx_cpl_abort:2;/*12~13*/
        oal_uint32 reserved4:2;/*14~15*/
        oal_uint32 tx_cpl_timeout:2;/*16~17*/
        oal_uint32 reserved5:2;/*18~19*/
        oal_uint32 tx_data_poision:2;/*20~21*/
        oal_uint32 reserved6:2;/*22~23*/
        oal_uint32 reserved7:8;/*24~31*/
    }bits;
    oal_uint32 AsDword;
} PCIE_TX_ABORT_STATUS;

typedef union {
    struct {
        oal_uint32 rx_app_wr_err:2;/*0~1*/
        oal_uint32 reserved1:2;/*2~3*/
        oal_uint32 rx_ll_fetch_err:2;/*4~5*/
        oal_uint32 reserved2:2;/*6~7*/
        oal_uint32 reserved3:24;/*8~31*/
    }bits;
    oal_uint32 AsDword;
} PCIE_RX_ABORT_STATUS;

typedef union {
    struct {
        oal_uint32 rx_fifo0_full:1;/*0*/
        oal_uint32 rx_fifo0_empty:1;/*1*/
        oal_uint32 rx_fifo1_full:1;/*2*/
        oal_uint32 rx_fifo1_empty:1;/*3*/
        oal_uint32 tx_fifo0_full:1;/*4*/
        oal_uint32 tx_fifo0_empty:1;/*5*/
        oal_uint32 tx_fifo1_full:1;/*6*/
        oal_uint32 tx_fifo1_empty:1;/*7*/
        oal_uint32 reserved:24;/*8~31*/
    }bits;
    oal_uint32 AsDword;
} EDMA_FIFO_STAT;/*LLT EDMA_FIFO_STAT*/

typedef union {
    struct {
        oal_uint32 rx_msg_fifo0_full:1;/*0*/
        oal_uint32 rx_msg_fifo0_empty:1;/*1*/
        oal_uint32 rx_msg_fifo1_full:1;/*2*/
        oal_uint32 rx_msg_fifo1_empty:1;/*3*/
        oal_uint32 tx_msg_fifo0_full:1;/*4*/
        oal_uint32 tx_msg_fifo0_empty:1;/*5*/
        oal_uint32 tx_msg_fifo1_full:1;/*6*/
        oal_uint32 tx_msg_fifo1_empty:1;/*7*/
        oal_uint32 rx_cpu_msg_fifo0_full:1;/*8*/
        oal_uint32 rx_cpu_msg_fifo0_empty:1;/*9*/
        oal_uint32 rx_cpu_msg_fifo1_full:1;/*10*/
        oal_uint32 rx_cpu_msg_fifo1_empty:1;/*11*/
        oal_uint32 tx_cpu_msg_fifo0_full:1;/*12*/
        oal_uint32 tx_cpu_msg_fifo0_empty:1;/*13*/
        oal_uint32 tx_cpu_msg_fifo1_full:1;/*14*/
        oal_uint32 tx_cpu_msg_fifo1_empty:1;/*15*/
        oal_uint32 reserved:16;/*16:31*/
    }bits;
    oal_uint32 AsDword;
} MSG_FIFO_STAT;

typedef struct _PcieDMADataElement_
{
    volatile CHAN_CTRL_LO cctrlo;/*0~3 bytes*/
    volatile oal_uint32       xferSize;/*4~7 bytes*/
    volatile oal_uint32       srcLowPart;/*8~11 bytes*/
    volatile oal_uint32       srcHighPart;/*12~15 bytes*/
    volatile oal_uint32       dstLowPart;/*16~19 bytes*/
    volatile oal_uint32       dstHighPart;/*20~23 bytes*/
}PcieDMADataElement;

typedef struct _PcieDMALinkElement_
{
    volatile CHAN_CTRL_LO cctrlo;/*0~3 bytes*/
    volatile oal_uint32       rsvd0;/*4~7 bytes*/
    volatile oal_uint32       nxtLowPart;/*8~11 bytes*/
    volatile oal_uint32       nxtHighPart;/*12~15 bytes*/
    volatile oal_uint32       rsvd1;/*16~19 bytes*/
    volatile oal_uint32       rsvd2;/*20~23 bytes*/
}PcieDMALinkElement;

typedef union _PcieDMAElement_
{
    volatile PcieDMADataElement data;
    volatile PcieDMALinkElement link;
}PcieDMAElement;

/**
 * The object type which holds all DMA registers. It's transparent to the user
 * of dma.h.
 */
typedef struct DMA_REGS *PDMA_REGS;


/******************************************************************************
 *
 *  EXTERN FUNCTION DECLARATIONS
 *
 ******************************************************************************/
void PcieEdmaSetOp(DMA_DIRC dirc, int enb);
void PcieEdmaSetWei(DMA_DIRC dirc, oal_uint8 chan, oal_uint8 wei);
void PcieEdmaSetDoorBell(DMA_DIRC dirc, oal_uint8 chan, oal_uint8 stop);
#ifdef PCIE_EDMA_SINGLE_BLOCK_TRANSFER
void PcieEdmaSetXferSize( DMA_DIRC dirc, oal_uint8 chan, oal_uint32 size);
void PcieEdmaSetSrc(DMA_DIRC dirc, oal_uint8 chan, PHYADDR addr );
void PcieEdmaSetDst(DMA_DIRC dirc, oal_uint8 chan, PHYADDR addr );
#endif
void PcieEdmaSetCtrl(DMA_DIRC dirc, oal_uint8 chan, DMA_XFER_MODE mode);
void PcieEdmaSetMSIAddr(DMA_DIRC dirc, DMA_INT_TYPE type, PHYADDR addr);
void PcieEdmaSetMSIData(DMA_DIRC dirc, oal_uint8 chan, oal_uint16 data);
void PcieEdmaSetLLIntErr( DMA_DIRC dirc, oal_uint8 chan, oal_uint8 loc, oal_uint8 enb);
void PcieEdmaSetLL( DMA_DIRC dirc, oal_uint8 chan, PHYADDR addr);
void PcieEdmaGetDataEle(oal_uint32 lladdr, oal_uint8 idx, oal_uint8 *pcs, oal_uint8 *intr,
        oal_uint32 *xferSize, PHYADDR *src, PHYADDR *dst);
void PcieEdmaGetDataEleDst(oal_uint32 lladdr, oal_uint8 idx, PHYADDR *dst);
void PcieEdmaGetDataEleSrc(oal_uint32 lladdr, oal_uint8 idx, PHYADDR *src);
void PcieEdmaSetDataEle(oal_uint32 lladdr, oal_uint8 idx, oal_uint8 pcs, oal_uint8 intr,
        oal_uint32 xferSize, PHYADDR src, PHYADDR dst);
void PcieEdmaSetLinkEleInt(oal_uint32 lladdr, oal_uint32 idx, oal_uint8 intr);
void PcieEdmaSetLinkEle(oal_uint32 lladdr, oal_uint8 idx, oal_uint8 pcs, PHYADDR nxt,
        oal_uint8 recyc);
void PcieEdmaGetLinkEle(oal_uint32 lladdr, oal_uint8 idx, oal_uint8 *pcs, PHYADDR *nxt);
void PcieEdmaGetChanNum( oal_uint8 *wrch, oal_uint8 *rdch);
void PcieEdmaGetChanStatus( DMA_DIRC dirc, oal_uint8 chan,
        DMA_CHAN_STATUS *status);
void PcieEdmaGetXferSize(  DMA_DIRC dirc, oal_uint8 chan, oal_uint32 *size);
void PcieEdmaGetElementPtr( DMA_DIRC dirc, oal_uint8 chan,
        PHYADDR *addr);
void PcieEdmaClrInt( DMA_DIRC dirc, oal_uint8 chan, DMA_INT_TYPE type);
void PcieEdmaClrIntChans( DMA_DIRC dirc, oal_uint8 chanMask, DMA_INT_TYPE type);
oal_uint8 PcieEdmaQueryIntSrc( DMA_DIRC *dirc, oal_uint8 *chan,
        DMA_INT_TYPE *type);
oal_uint8 PcieEdmaQueryErrSrc( DMA_DIRC dirc, oal_uint8 chan,
        DMA_ERR *type);
void PcieEdmaDumpReg(void);
void PcieEdmaLltPushFiFo(oal_uint32 addr, DMA_DIRC dirc, oal_uint8 chan);
oal_uint32 PcieEdmaEnFiFoIsFull(DMA_DIRC dirc);
oal_int32 PcieEdmaGetWriteDoneFiFo(oal_uint32 * addr, oal_uint32 *count);
oal_int32 PcieEdmaGetReadDoneFiFo(oal_uint32 * addr, oal_uint32 *count);
void PcieEdmaInit(void);
#ifdef _PRE_PLAT_FEATURE_PCIE_DEBUG
void PcieEdmaDumpEles(oal_uint32 lladdr, oal_uint8 nums);
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif
